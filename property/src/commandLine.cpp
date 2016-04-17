/*
 * commandLine.cpp
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#include <unistd.h>
#include <stdlib.h>
#include <iostream>

#include <commandLine.h>

commandLineArgs::commandLineArgs( int argc, char **argv ) {
	int opt = 0;
	char* p;
	long converted = 0;

	while ( ( opt = getopt( argc, argv, "r:i:o:u:p:d:n:tcz?" ) ) != -1 ) {
		switch ( opt ) {
			case '?':
			case 'h':
				this->usage( argv[0] );
				exit( 0 );

				break;
			case 'r':
				this->recordLayoutFile_ = optarg;

				break;
			case 'i':
				this->dataFile_ = optarg;

				break;
			case 'o':
				if( optarg == this->mysql_ || optarg == this->postgres_ || optarg == this->sqlite3_  ) {
					this->dbFlag_ = true;
					this->dbName_ = optarg;
				} else {
					if( optarg == this->fileflat_ ) {
						this->createFileFlat_ = true;
					} else if( optarg == this->filesql_ ) {
						this->createFileSQL_ = true;
					} else {
						std::cerr << "Command line argument error\n"
								"\tOption -o must be one of the following [mysql|postgres|sqlite3|flatfile|flatsql]\n\n";
						this->usage( argv[0] );
						exit( 1 );
					}

					this->createFile_ = true;
				}

				this->output_ = optarg;

				break;
			case 'p':
				this->password_ = optarg;

				break;
			case 'u':
				this->login_ = optarg;

				break;
			case 's':
				this->database_ = optarg;

				break;
			case 'c':
				this->dropTableFirst_ = true;

				break;
			case 'd':
				this->delimiter_ = optarg[0];

				break;
			case 'n':
				converted = strtol(optarg, &p, 10);
				if ( *p ) {
					std::cerr << "option -t requires a valid number\n";

					this->usage( argv[0] );

					exit( 1 );
				}

				this->count_ = converted;

				break;
			case 't':
				this->outputTbl_ = true;
				this->dbFlag_ = false;
				this->createFile_ = false;

				break;
			default:
				this->usage( argv[0] );
				exit( 1 );
		}
	}

	if ( this->dbFlag_ == false and this->dropTableFirst_ == true ) {
		std::cerr << "Command line argument error\n"
			"\tCannot use option -c without using a -o database option\n";
		this->usage( argv[0] );
		exit( 1 );
	}

	if( this->delimiter_ != '\t' && ( this->output_ != this->fileflat_ && this->output_ != this->filesql_ ) ) {
		std::cerr << "Command line argument error\n"
			"\tCannot use option -d without using a -o option of [fileflat|filesql]\n\n";
		this->usage( argv[0] );
		exit( 1 );
	}
}

void commandLineArgs::usage( char *programName ) {
	std::cerr << "Usage: " << programName << "\n"
		"\t-h print this usage\n"
		"\t-r recordLayoutFile (default filename is ./recordLayout.txt)\n"
		"\t-i inputDataFile (example ./Bergen15.txt)\n"
		"\t-o [mysql|postgres|sqlite3|fileflat|filesql] choose output form\n"
			"\t\tflatfile uses default delimiter of TAB\n"
			"\t\tflatsql creates INSERT SQL statements\n"
		"\t-d delimiter (if -o is flatfile change default to 'delimiter'. Must be one character)\n"
		"\t-l databaseLogin\n"
		"\t-p databasePassword\n"
		"\t-c before inserting data into table: property, DROP (if exist) it then re-CREATE table property\n"
		"\t-n numOfTransactions before executing an SQL COMMIT\n"
		"\t\tdefault transactions is 1000\n"
		"\t-t output property table definition ONLY\n";
}
