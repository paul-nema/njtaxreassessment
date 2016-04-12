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

commandLineArgs::commandLineArgs( int argc, char *argv[] ) {
	int opt = 0;
	char* p;
	long converted = 0;

	while ((opt = getopt(argc, argv, "hr:d:o:ps:cqn:z")) != -1) {
		switch (opt) {
			case 'h':
				this->usage( argv[0] );
				exit( 0 );

				break;
			case 'r':
				this->recordLayoutFile_ = optarg;

				break;
			case 'd':
				this->dataFile_ = optarg;

				break;
			case 'o':
				this->outputFile_ = optarg;

				break;
			case 'p':
				this->createFile_ = false;

				break;
			case 's':
				this->database_ = optarg;

				break;
			case 'c':
				this->dropTableFirst_ = true;

				break;
			case 'q':
				this->dbFlag_ = false;

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
			case 'z':
				this->outputTbl_ = true;

				break;
			default:	// '?'
				std::cerr << "Invalid option" << std::endl;
				this->usage( argv[0] );
				exit( 1 );
		}
	}

	if ( this->dbFlag_ == false and this->dropTableFirst_ == true ) {
		std::cerr << "Command line argument error\n"
			"\tCannot use option -c without -s sqlite3File";
		this->usage( argv[0] );
		exit( 1 );
	}

	if ( this->dbFlag_ == false and this->createFile_ == false ) {
		std::cerr << "Command line argument error\n"
			"\tOutput must be either writing to a file or sqlite3 - you have neither\n";
		this->usage( argv[0] );
		exit( 1 );
	}

	if ( this->outputTbl_ ) {
		this->dbFlag_ = false;
		this->createFile_ = false;
	}
}

void commandLineArgs::usage( char *programName ) {
	std::cerr << "Usage: " << programName << "\n"
		"\t-h print this usage\n"
		"\t-r recordLayoutFile (default filename is ./recordLayout.txt)\n"
		"\t-d inputDataFile (default filename is ./Bergen15.txt\n"
		"\t-o outputFile (data converted into SQL INSERT statements)\n"
		"\t-p do not produce a text file ( as per the -o option )\n"
		"\t-s sqlite3File\n"
		"\t-c before inserting data into table: property, DROP (if exist) it then re-CREATE table property\n"
		"\t-q do not write to sqlite3 (if this flag is not use default is to write to DB\n"
		"\t-n numOfTransactions ( when writing to sqlite3 before a commit )\n"
		"\t\tdefault transactions is 1000\n"
		"\t-z output property table definition ONLY\n";
}

const std::string & commandLineArgs::dbFile() {
	return( this->database_ );
}

const std::string & commandLineArgs::outputFile() {
	return( this->outputFile_ );
}

const std::string & commandLineArgs::dataFile() {
	return( this->dataFile_ );
}

const std::string & commandLineArgs::recordLayoutFile() {
	return( this->recordLayoutFile_ );
}

bool commandLineArgs::useDB() {
	return( this->dbFlag_ );
}

bool commandLineArgs::dropTable() {
	return( this->dropTableFirst_ );
}

bool commandLineArgs::createFile() {
	return( this->createFile_ );
}

bool commandLineArgs::genPropertyTable() {
	return( this->outputTbl_ );
}

int commandLineArgs::commitCount() {
	return( this->count_ );
}
