/*
 * property.cpp
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <stdio.h>
#include <algorithm>

#include <sqlite3.h>

#include <sql.h>
#include <sqlext.h>

#include <parseDirective.h>
#include <commandLine.h>

typedef std::vector< parseDirective > parseDirectiveVec;

static int genCreateTblSQL = 0;
static int recordLength = 700;

static bool inTransaction = false;

static commandLineArgs *cmdLnArgs;


static int callback( void *NotUsed, int argc, char **argv, char **azColName ) {
   for( int i = 0; i < argc; i++ ) {
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }

   printf("\n");

   return 0;
}

void getRecLayout( const std::string &file, parseDirectiveVec &pdVec ) {
	std::string line;
	std::string fieldName;
	std::string picture;

	std::ifstream recordLayout( file.c_str() );

	parseDirective *pD;

	int fieldLevel;
	int fld;
	int start;
	int end;
	int length;

	if( ! recordLayout.is_open() ) {
		std::cerr << "Unable to open file: " << file << std::endl;
		std::exit( 1 );
	}

	while( getline( recordLayout, line ) ) {
		if( ! isdigit( line[0] ) ) {
			continue;
		}

		std::istringstream iss( line );

		if (! (iss >> fieldLevel >> fieldName >> picture >> fld >> start >> end >> length ) ) {
			std::cerr << "Throw away record in line: " << line << std::endl;
			continue;
		}

		if( fieldName == "FILLER" or picture == "GROUP" ||
				fieldName == "SOCIAL-SECURITY-NO" ) {	// Don't include this field
			std::cerr << "Throw away record in line: " << line << std::endl;
			continue;
		}

		pD = new parseDirective( fieldLevel, fieldName, picture, fld, start, end, length );

		pdVec.push_back( *pD );
	}

	recordLayout.close();
}

std::string generateTblIndexes() {
	std::string sql;
	sql += "CREATE INDEX indx_COUNTY_DISTRICT ON property (COUNTY_DISTRICT);\n";
	sql += "CREATE INDEX indx_PROPERTY_LOCATION ON property (PROPERTY_LOCATION);\n";
	sql += "CREATE INDEX indx_TRANSACTION_DATE_MMDDYY ON property (TRANSACTION_DATE_MMDDYY);\n";
	sql += "CREATE INDEX indx_OWNER_NAME ON property (OWNER_NAME);\n";

	return( sql );
}

std::string dropIndexesSQL() {
	std::string sql;
	sql += "DROP INDEX IF EXISTS indx_COUNTY_DISTRICT;\n";
	sql += "DROP INDEX IF EXISTS indx_PROPERTY_LOCATION;\n";
	sql += "DROP INDEX IF EXISTS indx_TRANSACTION_DATE_MMDDYY;\n";
	sql += "DROP INDEX IF EXISTS indx_OWNER_NAME;\n";

	return( sql );
}

void dropIndexes( sqlite3 *db ) {
	char *zErrMsg = 0;

	int returnCd = sqlite3_exec( db, dropIndexesSQL().c_str(), ::callback, 0, &zErrMsg );

	if( returnCd != 0 ) {
		std::cerr << "SQL CREATE TABLE error: " << zErrMsg << std::endl;
		std::cerr << "SQL: " << dropIndexesSQL() << std::endl;

		sqlite3_close( db );

		exit( returnCd );
	}
}

void generateTblSQL( parseDirectiveVec &pdVec, std::ofstream &outFile, sqlite3 *db ) {
	char *zErrMsg = 0;

	int returnCd = 0;

	if( cmdLnArgs->dropTable() ) {
		std::string drop = "DROP TABLE IF EXISTS property;";

		returnCd = sqlite3_exec( db, drop.c_str(), ::callback, 0, &zErrMsg );

		if( returnCd != 0 ) {
			std::cerr << "SQL DROP TABLE error:\n\t" << zErrMsg << std::endl;

			sqlite3_close( db );

			exit( returnCd );
		}
	}

	std::string sql;

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

	if( cmdLnArgs->genPropertyTable() ) {
		std::cout << sql << dropIndexesSQL() << generateTblIndexes() << std::endl;
		return;
	}

	cmdLnArgs->createFile() && outFile << sql;

	if( cmdLnArgs->dropTable() ) {
		returnCd = sqlite3_exec( db, sql.c_str(), ::callback, 0, &zErrMsg );

		if( returnCd != 0 ) {
			std::cerr << "SQL CREATE TABLE error: " << zErrMsg << std::endl;
			std::cerr << "SQL: " << sql << std::endl;

			sqlite3_close( db );

			exit( returnCd );
		}
	}
}

void generateSQL( parseDirectiveVec &pdVec, std::ofstream &outFile, sqlite3 *db ) {
	std::string columns = "INSERT INTO property (";
	std::string values = "VALUES (";

	for( unsigned cnt = 0; cnt < pdVec.size(); cnt++ ) {
		if( ::genCreateTblSQL == 0 && cmdLnArgs->useDB() == true ) {
			genCreateTblSQL++;
			generateTblSQL( pdVec, outFile, db );
			dropIndexes( db );
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

	if( cmdLnArgs->createFile() ) {
		outFile << columns << values << std::endl;
	}

	std::string sql = columns + values;
	char *zErrMsg = 0;

	if( cmdLnArgs->useDB() ) {
		int returnCd = sqlite3_exec( db, sql.c_str(), ::callback, 0, &zErrMsg );

		if( returnCd != 0 ) {
			std::cerr << "SQL INSERT error: " << zErrMsg << std::endl;
			std::cerr << "SQL: "  << sql << std::endl;

			sqlite3_close( db );

			exit( returnCd );
		}
	}
}

void getData( const std::string &file, const std::string &outputFile, parseDirectiveVec &pdVec, sqlite3 *db ) {
	int returnCd = 0;
	int commitCnt = 0;

	char *zErrMsg = 0;

	std::string line;
	std::string sqlBegin = "BEGIN TRANSACTION;";
	std::string sqlEnd = "END TRANSACTION;";

	std::ofstream outFile;

	std::ifstream dataFile( file.c_str() );

	if( ! dataFile.is_open() ) {
		std::cerr << "Unable to open file: " << file << std::endl;
		exit( 1 );
	}

	if( cmdLnArgs->createFile() ) {
		outFile.open( outputFile.c_str(), std::ios::out );

		if( ! outFile.is_open() ) {
			std::cerr << "Unable to open output file: " << outputFile << std::endl;

			cmdLnArgs->useDB() && sqlite3_close( db );
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

		// If using the database, CREATE Table at commitCnt == 0 then
		//	BEGIN TRANSACTION at the next iteration commitCnt == 1
		if( commitCnt == 1 && cmdLnArgs->useDB() ) {
			returnCd = sqlite3_exec( db, sqlBegin.c_str(), ::callback, 0, &zErrMsg );

			if( returnCd != 0 ) {
				std::cerr << "SQL BEGIN error: " << zErrMsg << std::endl;
				std::cerr << "SQL: "  << sqlBegin << std::endl;

				cmdLnArgs->useDB() && sqlite3_close( db );

				exit( returnCd );
			}

			::inTransaction = true;
		}

		generateSQL( pdVec, outFile, db );

		//  Exec the END TRANSACTIO at cmdLnArgs->commitCount()
		if( commitCnt++ == cmdLnArgs->commitCount() && cmdLnArgs->useDB() ) {
			returnCd = sqlite3_exec( db, sqlEnd.c_str(), ::callback, 0, &zErrMsg );

			if( returnCd != 0 ) {
				std::cerr << "SQL END error: " << zErrMsg << std::endl;
				std::cerr << "SQL: "  << sqlEnd << std::endl;

				sqlite3_close( db );

				exit( returnCd );
			}

			commitCnt = 0;	// restart counter
			::inTransaction = false;
		}
	}

	dataFile.close();

	if( cmdLnArgs->createFile() ) {
		outFile.close();
	}
}

void testDB() {

	SQLHENV			 V_OD_Env;     // Handle ODBC environment
	long			 V_OD_erg;     // result of functions
	SQLHDBC			 V_OD_hdbc;    // Handle connection

	char			 V_OD_stat[10]; // Status SQL
	SQLINTEGER		 V_OD_err, V_OD_rowanz, V_OD_id;
	SQLSMALLINT		 V_OD_mlen;
	char             V_OD_msg[200], V_OD_buffer[200];

    // 1. allocate Environment handle and register version
	V_OD_erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&V_OD_Env);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		printf("Error AllocHandle\n");
		exit(0);
	}

	V_OD_erg=SQLSetEnvAttr(V_OD_Env, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		printf("Error SetEnv\n");
		SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
		exit(0);
	}

	// 2. allocate connection handle, set timeout
	V_OD_erg = SQLAllocHandle(SQL_HANDLE_DBC, V_OD_Env, &V_OD_hdbc);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		printf("Error AllocHDB %d\n", (int)V_OD_erg);
		SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
		exit(0);
	}

	SQLSetConnectAttr(V_OD_hdbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER *)5, 0);
	// 3. Connect to the datasource "web"
	V_OD_erg = SQLConnect(V_OD_hdbc, (SQLCHAR*) "OnMySQL", SQL_NTS,
                                     (SQLCHAR*) "property_user", SQL_NTS,
                                     (SQLCHAR*) "prop3rt7R3@ss", SQL_NTS);
	if ((V_OD_erg != SQL_SUCCESS) && (V_OD_erg != SQL_SUCCESS_WITH_INFO))
	{
		printf("Error SQLConnect %d\n", (int)V_OD_erg);
		SQLGetDiagRec(SQL_HANDLE_DBC, V_OD_hdbc, 1, (SQLCHAR*)V_OD_stat, &V_OD_err, (SQLCHAR*)V_OD_msg, 100, &V_OD_mlen);
		printf("%s (%d)\n", V_OD_msg, V_OD_err);
		SQLFreeHandle(SQL_HANDLE_DBC, V_OD_hdbc);
		SQLFreeHandle(SQL_HANDLE_ENV, V_OD_Env);
		exit(0);
	}


	SQLDisconnect(V_OD_hdbc);


	exit(3);
}


int main( int argc, char **argv ) {
	int returnCd;

	char *zErrMsg = 0;

	sqlite3 *db;

	parseDirectiveVec pdVec;

	// testDB();

	// START the program!
	cmdLnArgs = new commandLineArgs( argc, argv );

	// Open the sqlite3 database
	if( cmdLnArgs->useDB() ) {
		returnCd = sqlite3_open( cmdLnArgs->dbFile().c_str(), &db );

		if( returnCd != 0 ) {
			std::cerr << "Unable to open database: " << cmdLnArgs->dbFile() << std::endl;
			return( returnCd );
		}
	}

	// Do all the work here
	getRecLayout( cmdLnArgs->recordLayoutFile(), pdVec );
	if( cmdLnArgs->genPropertyTable() == true ) {
		std::ofstream dummy;
		generateTblSQL( pdVec, dummy, db );
		exit( 0 );
	}
	getData( cmdLnArgs->dataFile(), cmdLnArgs->outputFile(), pdVec, db );


	std::cout << "DONE!" << std::endl;

	if( cmdLnArgs->useDB() ) {
		if( ::inTransaction == true ) {
			returnCd = sqlite3_exec( db, "COMMIT;", ::callback, 0, &zErrMsg );

			if( returnCd != 0 ) {
				std::cerr << "ERROR: SQL COMMIT error: " << zErrMsg << std::endl;
				std::cerr << "Closing DB" << std::endl;

				sqlite3_close( db );

				exit( returnCd );
			}
		}

		// Recreate indexes
		returnCd = sqlite3_exec( db, generateTblIndexes().c_str(), ::callback, 0, &zErrMsg );

		if( returnCd != 0 ) {
			std::cerr << "ERROR: SQL CREATE INDEX error: " << zErrMsg << std::endl;
			std::cerr << "SQL: " << generateTblIndexes() << std::endl;

			sqlite3_close( db );

			exit( returnCd );
		}

		sqlite3_close( db );
	}

	exit( 0 );
}
