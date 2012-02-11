
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include "enumerator.h"

class string_enumerator : public enumerator< char >
{
private:	
	void enumerate()
	{
        const char * c( str_.c_str() );
        while ( '\0' != * c) {
			const char tmp( * c);
			yield_return( tmp);
            ++c;
		}
	}

	std::string		const&	str_;

public:
	string_enumerator( std::string const& str) :
		str_( str)
	{}
};

int main( int argc, char * argv[])
{
	{
		std::string str("Hello World!");
		string_enumerator se( str);
		char tmp;
		while ( se.get( tmp) ) {
			std::cout << tmp <<  " ";
		}
	}

	std::cout << "\nDone" << std::endl;

	return EXIT_SUCCESS;
}
