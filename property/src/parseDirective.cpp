/*
 * parseDirective.cpp
 *
 *  Created on: Apr 9, 2016
 *      Author: pnema
 */

#include <algorithm>
#include <iostream>
#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>

#include <parseDirective.h>

parseDirective::parseDirective( commandLineArgs *cmdLnArgs, int fieldLevel, std::string &fieldName, std::string &picture,
		int fld, int start, int end, int length ) {
	this->fieldLevel_ = fieldLevel;
	this->fieldName_ = fieldName;
	this->picture_ = picture;
	this->fld_ = fld;
	this->start_ = start;
	this->end_ = end;
	this->length_ = length;
	this->cmdLnArgs_ = cmdLnArgs;

	int charStart = 0;

	charStart = this->fieldName_.find( '(' );
	if( charStart != -1 ) {
		this->fieldName_.erase( charStart, 1 );
	}

	charStart = this->fieldName_.find( ')' );
	if( charStart != -1 ) {
		this->fieldName_.erase( charStart, 1 );
	}

	// replace - to _ as DB doesn't like '-' in a field name
	std::replace( this->fieldName_.begin(), this->fieldName_.end(), '-', '_' );

	this->fieldType_ = this->determineType( this->fieldName_ );
}


int parseDirective::parse( const std::string & line ) {
	this->data_ = line.substr( this->start_ - 1, this->length_ );
	this->trim( this->data_ );
	this->data_ = this->escapeString( this->data_ );

	if( this->fieldType_ == PD::DATE ) {
		if( this->data_.length() != 6 || this->isZeros( this->data_ ) == true ) {
			this->data_ = "0000-00-00";
			return( 0 );
		}

		std::string MM = this->data_.substr( 0, 2 );
		std::string DD = this->data_.substr( 2, 2 );
		std::string YY = this->data_.substr( 4, 2 );

		if( ! this->isNumber( DD ) ) {
			std::cerr << "UNKNOWN DD Number: " << MM
					<< "\nOn record:\n" << line << std::endl;
			return( 1 );
		} else if( this->isZeros( DD ) == true ) {
			DD = "01";
		}

		if( ! this->isNumber( MM ) ) {
			std::cerr << "UNKNOWN MM Number: " << MM
					<< "\nOn record:\n" << line << std::endl;
			return( 1 );
		} else if( this->isZeros( MM ) == true ) {
			MM = "01";
		}

		if( ! this->isNumber( YY ) ) {
			std::cerr << "UNKNOWN YY Number: " << MM
					<< "\nOn record:\n" << line << std::endl;
			return( 1 );
		}

		if( atoi( this->data_.substr( 4, 2 ).c_str() ) < 20 ) {
			YY = "20" + this->data_.substr( 4, 2 );
		} else {
			YY = "19" + this->data_.substr( 4, 2 );
		}

		this->data_ = YY + "-" + MM + "-" + DD;
	}

	return( 0 );
}

std::string parseDirective::escapeString( std::string &str ) {
	std::string ttmp;

	for( auto const &ch: str ) {
		if( ch == '\'' ) {
			if( this->cmdLnArgs_->dbName() == this->cmdLnArgs_->sqlite3() ) {
				ttmp += "''";
			} else {
				ttmp += "\\'";
			}
		} else if ( ch == '\\' ) {
			ttmp += "\\\\";
		} else {
			ttmp += ch;
		}
	}

	return( ttmp );
}


std::string &parseDirective::ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
        return s;
}

// trim from end
std::string &parseDirective::rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
        return s;
}

// trim from both ends
std::string &parseDirective::trim( std::string &s ) {
        return this->ltrim(this->rtrim(s));
}

PD::FieldType parseDirective::determineType( const std::string & field ) {
	if( field.find( "_DATE_" ) != std::string::npos || field.find( "_DATE" ) != std::string::npos ) {
		return PD::DATE;
	}

	if( field.find( "NUMBER" ) != std::string::npos ) {
		if( field.find( "TAX_ACCOUNT_NUMBER" ) != std::string::npos ) {
			return PD::CHAR;
		}

		return PD::NUMBER;
	}

	if( field.find( "VALUE" ) != std::string::npos ||
			field.find( "AMOUNT" ) != std::string::npos ||
			field.find( "PRICE" ) != std::string::npos ) {

		if( field.find( "SALES_PRICE_CODE" ) != std::string::npos ) {
			return PD::CHAR;
		}

		return PD::MONEY;
	}

	return PD::CHAR;
}


bool parseDirective::isNumber( const std::string & str ) {
    std::string::const_iterator it = str.begin();

    while ( it != str.end() && std::isdigit( *it ) )
    	++it;
    return( !str.empty() && it == str.end() );
}

bool parseDirective::isZeros( const std::string & str ) {
    std::string::const_iterator it = str.begin();

    while (it != str.end() && *it == '0' )
    	++it;
    return( !str.empty() && it == str.end() );
}
