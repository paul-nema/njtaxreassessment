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
    SQLINTEGER	i = 0;
    SQLINTEGER	native;
    SQLCHAR		state[ 10 ];
    SQLCHAR		text[ 256 ];
    SQLSMALLINT	len;
    SQLRETURN	ret;

    do {
        ret = SQLGetDiagRec( type, handle, ++i, (SQLCHAR*)state, &native, (SQLCHAR*)text, 256, &len );
        // if ( SQL_SUCCEEDED( ret ) ) {
        	std::cerr << "state: " << state << " Num: " << i << " native: " << native << " " << text << std::endl;
        // }

    } while( ret == SQL_SUCCESS );
}


// Database connectoin
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
	if ( (erg != SQL_SUCCESS) && ( erg != SQL_SUCCESS_WITH_INFO ) )
	{
		std::cerr << "Failure in AllocStatement " << erg << std::endl;
		extract_error( hdbc, SQL_HANDLE_DBC );

	    SQLDisconnect( hdbc );

	    SQLFreeHandle( SQL_HANDLE_DBC, hdbc );
		SQLFreeHandle( SQL_HANDLE_ENV, env );

		exit(0);
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

		if (! (iss >> fieldLevel >> fieldName >> picture >> fld >> start >> end >> length ) ) {
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
		"CREATE INDEX indx_PROPERTY_LOCATION ON property (PROPERTY_LOCATION);\n",
		"CREATE INDEX indx_TRANSACTION_DATE_MMDDYY ON property (TRANSACTION_DATE_MMDDYY);\n",
		"CREATE INDEX indx_OWNER_NAME ON property (OWNER_NAME);\n"
	};

	return( indexes );
}


// SQL to drop indexes from table: property
std::vector< std::string > * dropIndexesSQL( commandLineArgs &cmdLnArgs ) {
	std::vector< std::string > indexes = {
		"indx_COUNTY_DISTRICT",
		"indx_PROPERTY_LOCATION",
		"indx_TRANSACTION_DATE_MMDDYY",
		"indx_OWNER_NAME"
	};

	std::vector< std::string > * returnVec = new std::vector< std::string >();

	// Create statement for each database type
	for( const auto &str: indexes ) {
		std::string *sql = new std::string( "DROP INDEX " );

		if( cmdLnArgs.dbName() == cmdLnArgs.mysql() ) {
			*sql += str + " ON property;\n";
		} else if ( cmdLnArgs.dbName() == cmdLnArgs.sqlite3() ) {
			*sql += "IF EXISTS " + str + ";\n";
		} else {
			*sql += "IF EXISTS " + str + " ON property;\n";
		}

		returnVec->push_back( *sql );
	}

	return( returnVec );
}


// Execute SQL to drop indexes
void dropIndexes( commandLineArgs &cmdLnArgs, SQLHENV &env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {
	int returnCd = 0;

	std::vector< std::string > * drops = dropIndexesSQL( cmdLnArgs );

	for( const auto &sql: *drops ) {
		returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );

		if( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
			std::cerr << "dropIndexes: SQL Drop Indexes error:" << std::endl;

			extract_error( hstmt, SQL_HANDLE_STMT );

			std::cerr << "\nSQL:\n\n" << sql << std::endl;

			SQLDisconnect( hdbc );

			exit( returnCd );
		}
	}

	// Delete all new memory
	std::vector< std::string > vec;
	drops->swap( vec );

	delete drops;

}


void generateTblSQL( commandLineArgs &cmdLnArgs, parseDirectiveVec &pdVec, std::ofstream &outFile, SQLHENV &env, SQLHDBC &hdbc, SQLHSTMT &hstmt ) {
	int returnCd = 0;

	std::string sql = "USE property";

	// For MySQL choose the correct D: property
	if( cmdLnArgs.dbName() == cmdLnArgs.mysql() ) {
		returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );
		if ((returnCd == SQL_SUCCESS_WITH_INFO) || (returnCd == SQL_ERROR)) {
			std::cerr << "generateTblSQL: USE property: " << std::endl;

			extract_error( hstmt, SQL_HANDLE_STMT );

			SQLDisconnect( hdbc );

			exit( returnCd );
		}
	}

	// Drop table if requested
	if( cmdLnArgs.dropTable() ) {
		std::string drop = "DROP TABLE IF EXISTS property;";

		returnCd = SQLExecDirect( hstmt, (SQLCHAR*)drop.c_str(), drop.length() );
		if ((returnCd == SQL_SUCCESS_WITH_INFO) || (returnCd == SQL_ERROR)) {
			std::cerr << "generateTblSQL: SQL DROP TABLE error:" << std::endl;

			extract_error( hstmt, SQL_HANDLE_STMT );

			SQLDisconnect( hdbc );

			exit( returnCd );
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

		if( cnt != ( pdVec.size() - 1 ) ) {
			lineOut << ",\n";
		} else {
			lineOut << "\n";
		}

		sql += lineOut.str();
		lineOut.str( "" );	// Clear the sstream
	}

	sql += ");\n";

	// Output table DDL to standard out then ewe're done
	if( cmdLnArgs.genPropertyTable() ) {
		for( const auto &sql: *dropIndexesSQL( cmdLnArgs ) ) {
			std::cout << sql << "\n";
		}

		for( const auto &str: generateTblIndexes() ) {
			std::cout << str;
		}

		std::cout << std::endl;

		return;
	}

	// Write table DDL to the output file
	cmdLnArgs.createFile() && outFile << sql;

	// Create the table in the DB
	if( cmdLnArgs.dropTable() ) {
		returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );
		if ( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
			std::cerr << "generateTblSQL: SQL CREATE TABLE error:" << std::endl;

			extract_error( hstmt, SQL_HANDLE_STMT );

			std::cerr << "\nSQL:\n\n" << sql << std::endl;

			SQLDisconnect( hdbc );

			exit( returnCd );
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
		int returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );

		if ( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
			std::cerr << "generateSQL: SQL INSERT error:" << std::endl;

			extract_error( hstmt, SQL_HANDLE_STMT );

			std::cerr << "\nSQL:\n\n" << sql << std::endl;

			SQLDisconnect( hdbc );

			exit( returnCd );
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

	if( ! dataFile.is_open() ) {
		std::cerr << "Unable to open file: " << cmdLnArgs.dataFile() << std::endl;
		exit( 1 );
	}

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
		if( ( len = line.length() ) != recordLength ) {
			if( ( len == recordLength + 1 ) && line[ recordLength ] == '\r' ) {
				line.erase( std::remove(line.begin(), line.end(), '\r' ), line.end() );
			} else {
				std::cout << "UNKNOWN Line.  Doens't match correct length of 700:\nLength = "
						<< len << "\n" << line << "\n";
				exit(1);
			}
		}

		// Parse line according to the parse directive
		for( unsigned cnt = 0; cnt < pdVec.size(); cnt++ ) {
			pdVec[ cnt ].parse( line );
		}

		if ( cmdLnArgs.genFileFlat() ) {
			generateFileFlat( cmdLnArgs, pdVec, outFile );

			continue;
		}

		// If using the database, CREATE Table at commitCnt == 0 then
		//	BEGIN TRANSACTION at the next iteration commitCnt == 1
		if( commitCnt == 1 && cmdLnArgs.useDB() ) {
			returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sqlBegin.c_str(), sqlBegin.length() );
			if ( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
				std::cerr << "getData: " << sqlBegin << std::endl;

				extract_error( hstmt, SQL_HANDLE_STMT );

				SQLDisconnect( hdbc );

				exit( returnCd );
			}

			::inTransaction = true;
		}

		if( cmdLnArgs.useDB() ) {
			generateSQL( cmdLnArgs, pdVec, outFile, env, hdbc, hstmt );
		}

		//  Exec the END TRANSACTION at cmdLnArgs->commitCount()
		if( commitCnt++ == cmdLnArgs.commitCount() && cmdLnArgs.useDB() ) {
			returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sqlEnd.c_str(), sqlEnd.length() );
			if ( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
				std::cerr << "getData: " << sqlEnd << std::endl;

				extract_error( hstmt, SQL_HANDLE_STMT );

				SQLDisconnect( hdbc );

				exit( returnCd );
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
			exit( 1 );
		}
	}

	// generate parsing rules
	getRecLayout( cmdLnArgs, pdVec );

	// give property table DDL if requested then exit
	if( cmdLnArgs.genPropertyTable() == true ) {
		std::ofstream dummy;

		generateTblSQL( cmdLnArgs, pdVec, dummy, env, hdbc, hstmt );

		exit( 0 );
	}

	getData( cmdLnArgs, pdVec, env, hdbc, hstmt );

	if( cmdLnArgs.useDB() ) {
		std::string sql = "COMMIT;";

		if( ::inTransaction == true ) {
			returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );

			if ( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
				std::cerr << "main: SQL COMMIT error:" << std::endl;

				extract_error( hstmt, SQL_HANDLE_STMT );

				std::cerr << "\nSQL:\n\n" << sql << std::endl;

				SQLDisconnect( hdbc );

				exit( returnCd );
			}
		}

		// Recreate indexes
		for( const auto &sql: generateTblIndexes() ) {
			returnCd = SQLExecDirect( hstmt, (SQLCHAR*)sql.c_str(), sql.length() );

			if ( ( returnCd == SQL_SUCCESS_WITH_INFO ) || ( returnCd == SQL_ERROR ) ) {
				std::cerr << "main: SQL CREATE INDEX error:" << std::endl;

				extract_error( hstmt, SQL_HANDLE_STMT );

				std::cerr << "\nSQL:\n\n" << sql << std::endl;

				SQLDisconnect( hdbc );

				exit( returnCd );
			}
		}

		SQLDisconnect( hdbc );
	}

	exit( 0 );
}
