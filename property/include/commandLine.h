/*
 * commandLine.h
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#ifndef INC_COMMANDLINE_H_
#define INC_COMMANDLINE_H_

#include <string>

class commandLineArgs {
public:
	commandLineArgs(int, char *[] );

	void usage( char * );

	const std::string & dbFile();
	const std::string & outputFile();
	const std::string & dataFile();
	const std::string & recordLayoutFile();

	bool useDB();
	bool dropTable();
	bool createFile();
	bool genPropertyTable();

	int commitCount();

private:
	bool dbFlag_ = true;
	bool dropTableFirst_ = false;
	bool createFile_ = true;
	bool outputTbl_ = false;

	int count_ = 1000;

	std::string database_ = "./property.db";
	std::string outputFile_ = "./propertyTblSQL.txt";
	std::string dataFile_ = "";
	std::string recordLayoutFile_ = "./recordLayout.txt";
};



#endif /* INC_COMMANDLINE_H_ */
