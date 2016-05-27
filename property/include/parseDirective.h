/*
 * parseDirective.h
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#ifndef INC_PARSEDIRECTIVE_H_
#define INC_PARSEDIRECTIVE_H_

#include <string>

#include <commandLine.h>

namespace PD {

	enum FieldType {
		CHAR,
		DATE,
		NUMBER,
		MONEY,
		FLOAT
	};
}


// Class that has the parse directive for one field and holds the parsed data
class parseDirective {
public:
	parseDirective( commandLineArgs *, int, std::string &, std::string &, int , int , int , int  );

	void fieldLevel( int fieldLevel )	{ this->fieldLevel_ = fieldLevel; }
	void generateBuildingDescription( std::string );

	const std::string &fieldName() { return( this->fieldName_ ); }
	const std::string &picture()	{ return( this->picture_ ); }
	const std::string &data()	{ return( this->data_ ); }	// return the parsed data

	int parse( const std::string & );	// parse the input line
	int fieldLevel()	{ return( this->fieldLevel_ ); }
	int length()	{ return( this->length_ ); }
	int fieldType()	{ return( this->fieldType_ ); }

	PD::FieldType determineType( const std::string & );

private:
	bool isNumber( const std::string & );
	bool isZeros( const std::string &  );

	std::string & ltrim( std::string & );
	std::string & rtrim( std::string & );
	std::string & trim( std::string & );
	std::string escapeString( std::string & );
	std::string escapeStringSlash( std::string & );

	int fieldLevel_;
	int fld_;
	int start_;
	int end_;
	int length_;

	PD::FieldType fieldType_;

	std::string fieldName_;
	std::string picture_;
	std::string data_;

	commandLineArgs *cmdLnArgs_;
};


#endif /* INC_PARSEDIRECTIVE_H_ */
