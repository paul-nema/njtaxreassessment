/*
 * parseDirective.h
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#ifndef INC_PARSEDIRECTIVE_H_
#define INC_PARSEDIRECTIVE_H_

#include <string>

namespace PD {

	enum FieldType {
		CHAR,
		DATE,
		NUMBER,
		MONEY
	};
}

class parseDirective {
public:
	parseDirective();
	parseDirective( int, std::string &, std::string &,
			int , int , int , int  );

	void fieldLevel( int );
	int fieldLevel();

	const std::string &fieldName();

	const std::string &picture();

	int parse( const std::string &);  // parse the input line

	const std::string &data(); // return the parsed data

	int length();

	int fieldType();

	PD::FieldType determineType( const std::string & );

private:
	void escapeString( std::string & );

	bool isNumber( const std::string & );
	bool isZeros( const std::string &  );

	std::string & ltrim( std::string & );
	std::string & rtrim( std::string & );
	std::string & trim( std::string & );

	int fieldLevel_;
	int fld_;
	int start_;
	int end_;
	int length_;

	PD::FieldType fieldType_;

	std::string fieldName_;
	std::string picture_;
	std::string data_;
};



#endif /* INC_PARSEDIRECTIVE_H_ */
