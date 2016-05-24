/*
 * property.cpp
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

// Standard Libs
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <algorithm>

// Third Party Libs
#include <sql.h>
#include <sqlext.h>

// App Libs
#include <parseDirective.h>
#include <commandLine.h>

typedef std::vector< parseDirective > parseDirectiveVec;

static int genCreateTblSQL = 0;
static int recordLength = 700;

static bool inTransaction = false;


// ODBC error handlers that prints all error messages
void extract_error( SQLHANDLE handle, SQLSMALLINT type ) {
	const int	bufSize = 256;
	const int	stateSize = 10;

    SQLINTEGER	i = 0;
    SQLINTEGER	native;
    SQLCHAR		state[ stateSize ];
    SQLCHAR		text[ bufSize ];
    SQLSMALLINT	length;
    SQLRETURN	returnCd = 0;

    do {
    	returnCd = SQLGetDiagRec( type, handle, ++i, (SQLCHAR*)state, &native, (SQLCHAR*)text, bufSize, &length );

        std::cerr << "returnCd: " << returnCd << " state: " << state
        		<< " Num: " << i << " native: " << native << " " << text << std::endl;

    } while( returnCd == SQL_SUCCESS );
}


// Execute no return type SQL statements
int ExecSQL( SQLHDBC &hdbc, SQLHSTMT &hstmt, std::string sql ) {
	int returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );

	if( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
		extract_error( hstmt, SQL_HANDLE_STMT );

		std::cerr << "\nSQL:\n\n" << sql << std::endl;

		SQLDisconnect( hdbc );
	}

	return( returnCd );
}


// Database connection
bool DBConnect( commandLineArgs &cmdLnArgs, SQLHENV &env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {

	long	erg;     // result of functions

    // 1. allocate Environment handle and register version
	erg = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env );
	if ( erg != SQL_SUCCESS && erg != SQL_SUCCESS_WITH_INFO ) {
		std::cerr << "SQLAllocHandle Error AllocHandle\n";

		return( false );
	}

	erg = SQLSetEnvAttr( env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0 );
	if ( erg != SQL_SUCCESS && erg != SQL_SUCCESS_WITH_INFO ) {
		std::cerr << "SQLSetEnvAttr Error SetEnv\n";

		SQLFreeHandle( SQL_HANDLE_ENV, env );

		return( false );
	}

	// 2. allocate connection handle, set timeout
	erg = SQLAllocHandle( SQL_HANDLE_DBC, env, &hdbc );
	if ( erg != SQL_SUCCESS && erg != SQL_SUCCESS_WITH_INFO ) {
		std::cerr << "SQLAllocHandle Error AllocHDB " << erg << "\n";

		SQLFreeHandle( SQL_HANDLE_ENV, env );

		return( false );
	}

	SQLSetConnectAttr( hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0 );

	// 3. Connect to the datasource
	erg = SQLConnect( hdbc, (SQLCHAR*) cmdLnArgs.dbName().c_str(), SQL_NTS, (SQLCHAR*) cmdLnArgs.login().c_str(), SQL_NTS,
			(SQLCHAR*) cmdLnArgs.password().c_str(), SQL_NTS);
	if ( erg != SQL_SUCCESS && erg != SQL_SUCCESS_WITH_INFO ) {
		std::cerr << "SQLConnect Error SQLConnect " << erg << "\n";

		extract_error( hdbc, SQL_HANDLE_DBC );

		SQLFreeHandle( SQL_HANDLE_DBC, hdbc );
		SQLFreeHandle( SQL_HANDLE_ENV, env );

		return( false );
	}

	// Allocate a statement handle
	erg = SQLAllocHandle( SQL_HANDLE_STMT, hdbc, &hstmt );
	if ( ( erg != SQL_SUCCESS ) && ( erg != SQL_SUCCESS_WITH_INFO ) )
	{
		std::cerr << "Failure in AllocStatement " << erg << std::endl;
		extract_error( hdbc, SQL_HANDLE_DBC );

	    SQLDisconnect( hdbc );

	    SQLFreeHandle( SQL_HANDLE_DBC, hdbc );
		SQLFreeHandle( SQL_HANDLE_ENV, env );

		std::exit(0);
	}

	return( true );
}


// Create parse directive objects.
void getRecLayout( commandLineArgs &cmdLnArgs, parseDirectiveVec &pdVec ) {
	parseDirective *pD;

	int fieldLevel;
	int fld;
	int start;
	int end;
	int length;

	std::string line;
	std::string fieldName;
	std::string picture;

	std::ifstream recordLayout( cmdLnArgs.recordLayoutFile().c_str() );

	if( ! recordLayout.is_open() ) {
		std::cerr << "Unable to open file: " << cmdLnArgs.recordLayoutFile() << std::endl;
		std::exit( 1 );
	}

	while( getline( recordLayout, line ) ) {
		if( ! isdigit( line[0] ) ) {
			continue;
		}

		std::istringstream iss( line );

		if ( ! ( iss >> fieldLevel >> fieldName >> picture >> fld >> start >> end >> length ) ) {
			continue;
		}

		// Don't include this field
		if( fieldName == "FILLER" or picture == "GROUP" || fieldName == "SOCIAL-SECURITY-NO" ) {
			continue;
		}

		// Create new parse objective object
		pD = new parseDirective( &cmdLnArgs, fieldLevel, fieldName, picture, fld, start, end, length );

		// Add the new object to the vector
		pdVec.push_back( *pD );
	}

	recordLayout.close();
}


// SQL to create table: property indexes
std::vector< std::string > generateTblIndexes() {
	std::vector< std::string > indexes = {
		"CREATE INDEX indx_COUNTY_DISTRICT ON property (COUNTY_DISTRICT);\n",
		"CREATE INDEX indx_STREET_ADDRESS ON property (STREET_ADDRESS);\n",
		"CREATE INDEX indx_TRANSACTION_DATE_MMDDYY ON property (TRANSACTION_DATE_MMDDYY);\n",
		"CREATE INDEX indx_OWNER_NAME ON property (OWNER_NAME);\n",
		"CREATE INDEX indx_DEED_DATE_MMDDYY ON property (DEED_DATE_MMDDYY);\n",
		"CREATE INDEX indx_ZIP_CODE ON property (ZIP_CODE);\n"
	};

	return( indexes );
}


// SQL to drop indexes from table: property
std::vector< std::string > dropIndexesSQL( commandLineArgs &cmdLnArgs ) {
	std::vector< std::string > indexes = {
		"indx_COUNTY_DISTRICT",
		"indx_STREET_ADDRESS",
		"indx_TRANSACTION_DATE_MMDDYY",
		"indx_OWNER_NAME",
		"indx_DEED_DATE_MMDDYY",
		"indx_ZIP_CODE"
	};

	//  Create vector to hold the actual SQL statements
	std::vector< std::string > rtn( indexes.size() );

	// Create statement for each database type
	for( const auto &str: indexes ) {
		std::string sql;

		if( cmdLnArgs.dbName() == cmdLnArgs.mysql() ) {
			sql += str + " ON property;\n";
		} else if ( cmdLnArgs.dbName() == cmdLnArgs.sqlite3() ) {
			sql += "IF EXISTS " + str + ";\n";
		} else {
			sql += "IF EXISTS " + str + " ON property;\n";
		}

		rtn.push_back( sql );
		sql.erase();
	}

	return( rtn );
}


// Execute SQL to drop indexes
void dropIndexes( commandLineArgs &cmdLnArgs, SQLHENV &env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {
	int returnCd = 0;

	// std::vector< std::string > * drops = dropIndexesSQL( cmdLnArgs );

	for( const auto &sql: dropIndexesSQL( cmdLnArgs ) ) {
		if( ( returnCd = ExecSQL( hdbc, hstmt, sql ) ) != 0 ) {
			std::cerr << "dropIndexes: SQL Drop Indexes error:" << std::endl;
			std::exit( returnCd );
		}
	}
}


void generateTblSQL( commandLineArgs &cmdLnArgs, parseDirectiveVec &pdVec, std::ofstream &outFile, SQLHENV &env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {
	int returnCd = 0;

	std::string sql = "USE property";

	// For MySQL choose the correct D: property
	if( cmdLnArgs.dbName() == cmdLnArgs.mysql() ) {
		if( ( returnCd = ExecSQL( hdbc, hstmt, sql ) ) != 0 ) {
			std::cerr << "generateTblSQL: USE property: " << std::endl;
			std::exit( returnCd );
		}
	}

	// Drop table if requested
	if( cmdLnArgs.dropTable() ) {
		std::string drop = "DROP TABLE IF EXISTS property;";

		if( ( returnCd = ExecSQL( hdbc, hstmt, drop ) ) != 0 ) {
			std::cerr << "generateTblSQL: SQL DROP TABLE error:" << std::endl;
			std::exit( returnCd );
		}
	}

	sql.clear();	// Re-use sql variable

	std::stringstream lineOut;

	lineOut << "CREATE TABLE IF NOT EXISTS property (" << std::endl;

	for( unsigned int cnt = 0; cnt < pdVec.size(); cnt++ ) {
		lineOut << "\t" << pdVec[ cnt ].fieldName();

		switch( pdVec[ cnt ].fieldType() ) {
		case PD::DATE:
			lineOut << "\tDATE";
			break;

		case PD::NUMBER:
			lineOut << "\tINTEGER";
			break;

		case PD::MONEY:
			lineOut << "\tREAL";
			break;

		default:	// CHAR
			lineOut << "\tCHAR(" << pdVec[ cnt ].length() << ")";
		}

		lineOut << ",\n";

		sql += lineOut.str();
		lineOut.str( "" );	// Clear the sstream
	}

	sql += "\tLAT\tFLOAT NOT NULL DEFAULT 0,\n";
	sql += "\tLNG\tFLOAT NOT NULL DEFAULT 0";
	sql += "\n);\n";

	// Output table DDL to standard out then ewe're done
	if( cmdLnArgs.genPropertyTable() ) {
		std::cout << sql << std::endl;

		for( const auto &sql: dropIndexesSQL( cmdLnArgs ) ) {
			std::cout << sql;
		}

		for( const auto &sql: generateTblIndexes() ) {
			std::cout << sql;
		}

		std::cout << std::endl;

		return;
	}

	// Write table DDL to the output file
	cmdLnArgs.createFile() && outFile << sql;

	// Create the table in the DB
	if( cmdLnArgs.dropTable() ) {
		if( ( returnCd = ExecSQL( hdbc, hstmt, sql ) ) != 0 ) {
			std::cerr << "generateTblSQL: SQL CREATE TABLE error:" << std::endl;
			std::exit( returnCd );
		}
	}
}


// Exec INSERT SQL for each record
void generateSQL( commandLineArgs &cmdLnArgs, parseDirectiveVec &pdVec, std::ofstream &outFile, SQLHENV	&env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {
	std::string columns = "INSERT INTO property (";
	std::string values = "VALUES (";

	for( unsigned cnt = 0; cnt < pdVec.size(); cnt++ ) {
		if( ::genCreateTblSQL == 0 && cmdLnArgs.useDB() == true ) {
			genCreateTblSQL++;
			generateTblSQL( cmdLnArgs, pdVec, outFile, env, hdbc, hstmt );

			if( cmdLnArgs.dropTable() == false ) {
				dropIndexes( cmdLnArgs, env, hdbc, hstmt );
			}
		}

		columns += pdVec[ cnt ].fieldName();
		values += "\'" + pdVec[ cnt ].data();

		if( cnt != pdVec.size() - 1 ) {
			columns += ",";
			values += "\',";
		}
	}

	columns += ") ";
	values += "\');";

	if( cmdLnArgs.createFile() ) {
		outFile << columns << values << std::endl;
	}

	std::string sql = columns + values;

	if( cmdLnArgs.useDB() ) {
		int returnCd = 0;

		if( ( returnCd = ExecSQL( hdbc, hstmt, sql ) ) != 0 ) {
			std::cerr << "generateSQL: SQL INSERT error:" << std::endl;
			std::exit( returnCd );
		}
	}
}


// Command line option to only output into a flat file
void generateFileFlat( commandLineArgs &cmdLnArgs, parseDirectiveVec &pdVec, std::ofstream &outFile ) {
	for( unsigned cnt = 0; cnt < pdVec.size() - 1; cnt++ ) {
		outFile << pdVec[ cnt ].data() << cmdLnArgs.delimiter();
	}
	outFile << pdVec[ pdVec.size() -1 ].data() << "\n";
}


// Read each line form the input file. Initiate the begin/end transaction
void getData( commandLineArgs &cmdLnArgs, parseDirectiveVec &pdVec, SQLHENV	&env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {
	int returnCd = 0;
	int commitCnt = 0;

	std::string line;
	std::string sqlBegin = "BEGIN;";
	std::string sqlEnd = "COMMIT;";

	std::ofstream outFile;

	std::ifstream dataFile( cmdLnArgs.dataFile().c_str() );

	// Open input data file
	if( ! dataFile.is_open() ) {
		std::cerr << "Unable to open file: " << cmdLnArgs.dataFile() << std::endl;
		std::exit( 1 );
	}

	// Open output file if writing to a file
	if( cmdLnArgs.createFile() ) {
		outFile.open( cmdLnArgs.outputFile().c_str(), std::ios::out );

		if( ! outFile.is_open() ) {
			std::cerr << "Unable to open output file: " << cmdLnArgs.outputFile() << std::endl;

			if( cmdLnArgs.useDB() ) {
				SQLDisconnect( hdbc );
			}

			std::exit( 1 );
		}
	}

	int len = 0;

	while( getline( dataFile, line ) ) {
		// Ensure record is the correct length. Account for extra DOS characters too
		if( ( len = line.length() ) != recordLength ) {

			// Record NOT the correct length. Determine if bad length due to DOS \r character
			if( ( len == recordLength + 1 ) && line[ recordLength ] == '\r' ) {
				line.erase( line.length() - 2 ); // Get rid of '\r'
			} else {
				std::cout << "UNKNOWN Line.  Doens't match correct length of 700:\nLength = "
						<< len << "\n" << line << "\n";
				std::exit(1);
			}
		}

		bool dropRecord = false;

		// Parse line according to the parse directive
		for( unsigned cnt = 0; cnt < pdVec.size(); cnt++ ) {
			pdVec[ cnt ].parse( line );

			// If this is not property_class == 2 (Residential) then do not keep
			if ( pdVec[ cnt ].fieldType() == PD::CHAR && pdVec[ cnt ].fieldName() == "PROPERTY_CLASS" ) {
				if ( pdVec[ cnt ].data() != "2" ) {
					dropRecord = true;

					break;
				}
			}
		}

		// Found a non-residential property. Drop it.
		if ( dropRecord == true ) {
			dropRecord = false;

			continue;
		}

		// Write to fileflat if required
		if ( cmdLnArgs.genFileFlat() ) {
			generateFileFlat( cmdLnArgs, pdVec, outFile );

			continue;
		}

		// write to filesql if required
		if ( cmdLnArgs.genFileSQL() ) {
			generateSQL( cmdLnArgs, pdVec, outFile, env, hdbc, hstmt );

			continue;
		}

		// If using the database, CREATE Table at commitCnt == 0 then
		//	BEGIN TRANSACTION at the next iteration commitCnt == 1
		if( commitCnt == 1 && cmdLnArgs.useDB() ) {
			if( ( returnCd = ExecSQL( hdbc, hstmt, sqlBegin ) ) != 0 ) {
				std::cerr << "getData: Begin Transaction " << std::endl;
				std::exit( returnCd );
			}

			::inTransaction = true;
		}

		if( cmdLnArgs.useDB() ) {
			generateSQL( cmdLnArgs, pdVec, outFile, env, hdbc, hstmt );
		}

		//  Exec the END TRANSACTION at cmdLnArgs->commitCount()
		if( commitCnt++ == cmdLnArgs.commitCount() && cmdLnArgs.useDB() ) {
			if( ( returnCd = ExecSQL( hdbc, hstmt, sqlEnd ) ) != 0 ) {
				std::cerr << "getData: END Transcation " << std::endl;
				std::exit( returnCd );
			}

			commitCnt = 0;	// restart counter
			::inTransaction = false;
		}
	}

	dataFile.close();

	if( cmdLnArgs.createFile() ) {
		outFile.close();
	}
}


int main( int argc, char **argv ) {
	int returnCd;

	SQLHENV		env = SQL_NULL_HENV;     // Handle ODBC environment

	SQLHDBC		hdbc = SQL_NULL_HDBC;    // Handle connection

	SQLHSTMT	hstmt = SQL_NULL_HSTMT;

	parseDirectiveVec pdVec;


	// START the program!
	// Get the command line arguments
	commandLineArgs cmdLnArgs( argc, argv );

	// Open the database connection
	if( cmdLnArgs.useDB() ) {
		if( DBConnect( cmdLnArgs, env, hdbc, hstmt ) == false ) {
			std::cerr << "Unable to open database: " << cmdLnArgs.dbFile() << std::endl;
			std::exit( 1 );
		}
	}

	// generate parsing rules
	getRecLayout( cmdLnArgs, pdVec );

	// give property table DDL if requested then std::exit
	if( cmdLnArgs.genPropertyTable() == true ) {
		std::ofstream dummy;

		generateTblSQL( cmdLnArgs, pdVec, dummy, env, hdbc, hstmt );

		std::exit( 0 );
	}

	getData( cmdLnArgs, pdVec, env, hdbc, hstmt );

	if( cmdLnArgs.useDB() ) {
		std::string sql = "COMMIT;";

		if( ::inTransaction == true ) {
			if( ( returnCd = ExecSQL( hdbc, hstmt, sql ) ) != 0 ) {
				std::cerr << "main: SQL COMMIT error:" << std::endl;
				std::exit( returnCd );
			}
		}

		// Recreate indexes
		for( const auto &sql: generateTblIndexes() ) {
			if( ( returnCd = ExecSQL( hdbc, hstmt, sql ) ) != 0 ) {
				std::cerr << "main: SQL CREATE INDEX error:" << std::endl;
				std::exit( returnCd );
			}
		}

		SQLDisconnect( hdbc );
	}

	std::exit( 0 );
}
