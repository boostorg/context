
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>

#include "continuation.h"

void first( continuation & c)
{
    std::cout << "started first! ";
    for ( int i = 0; i < 10; ++i)
    {
        c.suspend();
        std::cout << "a" << i;
    }
}

void second( continuation & c)
{
    std::cout << "started second! ";
    for ( int i = 0; i < 10; ++i)
    {
        c.suspend();
        std::cout << "b" << i;
    }
}

int main( int argc, char * argv[])
{
    {
        continuation c1( boost::bind( first, _1) );
        continuation c2( boost::bind( second, _1) );
        while ( ! c1.is_complete() && ! c2.is_complete() ) {
            c1.resume();
            std::cout << " ";
            c2.resume();
            std::cout << " ";
        }
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
