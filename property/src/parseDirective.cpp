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
#include <regex>
#include <sstream>
#include <vector>
#include <boost/regex.hpp>

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

	// replace - to _ as some DBs doesn't like '-' in a field name
	std::replace( this->fieldName_.begin(), this->fieldName_.end(), '-', '_' );

	this->fieldType_ = this->determineType( this->fieldName_ );
}


bool returnMatch( const std::string & str, const boost::regex & re ) {
	try {
		if ( boost::regex_match( str, re ) ) {
			return( true );
		} else {
			return( false );
		}
	} catch (std::regex_error& e) {
		// Syntax error in the regular expression
		std::cerr << "ERROR Syntax error in the regular expression for match on Building Description\nData = "
				<< str
				<< "\nREGEX = "
				<< re
				<< std::endl;

		std::exit( 1 );
	}

	return( false );
}


std::string returnSearch( const std::string & str, const boost::regex & re ) {
	boost::smatch match;

	try {
		if ( boost::regex_search( str, match, re ) ) {
			return( match.str(1) );
		} else {
			return( "" );
		}
	} catch (std::regex_error& e) {
		// Syntax error in the regular expression
		std::cerr << "ERROR Syntax error in the regular expression\nData = "
				<< str
				<< "\nREGEX = "
				<< re
				<< std::endl;

		std::exit( 1 );
	}

	return( "" );
}


// remove above string
void removeStr( std::string & str, const std::string & match ) {
	std::string::size_type i = str.find( match );

	if ( i != std::string::npos ) {
		str.erase( i, match.length() );
	}
}


/*
# 10. BUILDING DESCRIPTION: (O)
# Codes describe the physical characteristics of the property. The information should be listed in the
# following order: stories, exterior structural material, style, number of stalls and type of garage.
#   STORIES
#       S Prefix S with the number of stories.
#   STRUCTURE:
#       AL Aluminum siding
#       B Brick
#       CB Concrete Block
#       F Frame
#       M Metal
#       RC Reinforced concrete
#       S Stucco
#       SS Structured Steel
#       ST Stone
#       W Wood
#   STYLE:
#       A Commercial
#       B Industrial
#       C Apartments
#       D Dutch Colonial
#       E English Tudor
#       L Colonial
#       M Mobile Home
#       R Rancher
#       S Split Level
#       T Twin
#       W Row Home
#       X Duplex
#       Z Raised Rancher
#       O Other
#       2 Bi-Level
#       3 Tri-Level
#   GARAGE:
#       G Garage (My comment based upon a data review`)
#       AG Attached Garage
#       UG Unattached Garage
#       (Note: number of cars is a prefixed to code)
*/
void parseDirective::generateBuildingDescription( std::string str ) {
	if( str.length() == 0 ) {
		this->data_ = "";

		return;
	}

	std::string stories = "";
	std::string word = "";
	std::string garage = "";
	std::string structure = "";
	std::string style = "";

	boost::regex pattern( "(\\d\\.\\d|\\d)S" );

	// number of STORIES
	if ( ( word = returnSearch( str, pattern ) ) != "" ) {
		stories = word + "S";

		removeStr( str, stories );	// remove the previously found str
	} else {
		stories = "1S";
	}

	if( str.length() == 0 ) {
		this->data_ = stories + "-" + structure + "-" + style + "-" + garage;

		return;
	}

	// Search for GARAGE tokens
	std::string searchTokens = "(AG) (\\d?G) (UG)";

	std::istringstream iss1( searchTokens, std::istringstream::in );

	while( iss1 >> word ) {
		pattern = word;

		if( ( word = returnSearch( str, pattern ) ) != "" ) {
			garage = word;

			removeStr( str, word );	// remove the previously found str

			if( str.length() == 0 ) {
				this->data_ = stories + "-" + structure + "-" + style + "-" + garage;

				return;
			}

			break; // Assumption there is only ONE garage token
		}
	}


	// Search for STRUCTURE tokens
	searchTokens = "AL B CB F M RC SS S ST W";

	std::istringstream iss2( searchTokens, std::istringstream::in );

	while( iss2 >> word ) {
		pattern = word;

		if( returnMatch( str, pattern ) ) {
			structure = word;

			removeStr( str, structure );	// remove the previously found str

			if( str.length() == 0 ) {
				this->data_ = stories + "-" + structure + "-" + style + "-" + garage;

				return;
			}

			break; // Assumption there is only ONE structure token
		}
	}


	// Search for STYLE tokens
	searchTokens = "A B C D E L M R S T W X Z 2 3";

	std::istringstream iss3( searchTokens, std::istringstream::in );

	while( iss3 >> word ) {
		pattern = word;

		if( returnMatch( str, pattern ) ) {
			style = word;

			removeStr( str, style );	// remove the previously found str

			break; // Assumption there is only ONE structure token
		}
	}

	this->data_ = stories + "-" + structure + "-" + style + "-" + garage;
}


int parseDirective::parse( const std::string & line ) {
	this->data_ = line.substr( this->start_ - 1, this->length_ );
	this->trim( this->data_ );
	this->data_ = this->escapeString( this->data_ );

	// Handle Special Cases first
	if( this->fieldName_ == "LAT" || this->fieldName_ == "LONG" ) {
		this->data_ = "0.0";

		return( 0 );
	}

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


// Add escape characters to the SQL depending on the database connecting with
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


// remove spaces from the left side
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

	if( field.find( "NUMBER" ) != std::string::npos || field.find( "CALCULATED" ) != std::string::npos ) {
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

	if( field.find( "LAT" ) != std::string::npos || field.find( "LONG" ) != std::string::npos ) {
		return PD::FLOAT;
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
