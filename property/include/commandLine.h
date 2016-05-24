/*
 * commandLine.h
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#ifndef INC_COMMANDLINE_H_
#define INC_COMMANDLINE_H_

#include <string>

namespace OO {
	enum outputOption {
		FILE_FLAT,
		FILE_SQL,
		POSTGRES,
		SQLITE3,
		MYSQL
	};
}


class commandLineArgs {
public:
	commandLineArgs( int argc, char **argv );

	void usage( char * );

	char delimiter() { return this->delimiter_; }

	const std::string & dbFile()	{ return( this->database_ ); }
	const std::string & outputFile()	{ return( this->output_ ); }
	const std::string & dataFile()	{ return( this->dataFile_ ); }
	const std::string & recordLayoutFile()	{ return( this->recordLayoutFile_ ); }
	const std::string & mysql()		{ return this->mysql_; }
	const std::string & postgres()	{ return this->postgres_; }
	const std::string & sqlite3()	{ return this->sqlite3_; }
	const std::string & fileFlat()	{ return this->fileflat_; }
	const std::string & fileSQL()	{ return this->filesql_; }
	const std::string & login()		{ return this->login_; }
	const std::string & password()	{ return this->password_; }
	const std::string & dbName()	{ return this->dbName_; }

	bool useDB()	{ return this->dbFlag_; }
	bool dropTable()	{ return this->dropTableFirst_; }
	bool createFile()	{ return this->createFile_; }
	bool genPropertyTable() { return this->outputTbl_; }
	bool genFileFlat()	{ return this->createFileFlat_; }
	bool genFileSQL()	{ return this->createFileSQL_; }

	int commitCount()	{ return this->count_; }

private:
	bool dbFlag_ = false;
	bool dropTableFirst_ = false;
	bool createFile_ = false;
	bool createFileFlat_ = false;
	bool createFileSQL_ = false;
	bool outputTbl_ = false;
	bool useDiffOutputName_ = false;

	int count_ = 1000;

	char delimiter_ = '\t';

	OO::outputOption outOpt_ = OO::FILE_FLAT;

	std::string database_	= "property.db";
	std::string output_		= "propertyTblSQL.txt";
	std::string fileFlatName_	= "propertyFlat.txt";
	std::string fileSQLName_	= "propertySQL.txt";
	std::string dataFile_	= "";
	std::string recordLayoutFile_	= "./recordLayout.txt";
	std::string login_		= "property_user";
	std::string password_	= "first*1";
	std::string dbName_	= "";

	const std::string mysql_	= "mysql";
	const std::string postgres_	= "postgres";
	const std::string sqlite3_	= "sqlite3";
	const std::string fileflat_	= "fileflat";
	const std::string filesql_	= "filesql";
};



#endif /* INC_COMMANDLINE_H_ */
