
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include "enumerator.h"

class power : public enumerator< int >
{
private:
    void enumerate()
    {
        int counter = 0;
        int result = 1;
        while ( counter++ < exponent_)
        {
                result = result * number_;
                yield_return( result);
        }
    }

    int     number_;
    int     exponent_;

public:
    power( int number, int exponent) :
        number_( number), exponent_( exponent)
    {}
};

int main()
{
    {
        power pw( 2, 8);
        int i;
        while ( pw.get( i) ) {
            std::cout << i <<  " ";
        }
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
