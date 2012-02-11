
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>

#include "continuation.h"

void echo( continuation & c, int i)
{
    std::cout << i; 
    c.suspend();
}

void runit( continuation & c)
{
    std::cout << "started! ";
    for ( int i = 0; i < 10; ++i)
    {
        c.join( boost::bind( echo, _1, i) );
        c.suspend();
    }
}

int main( int argc, char * argv[])
{
    {
        continuation c( boost::bind( runit, _1) );
        while ( ! c.is_complete() ) {
            std::cout << "-";
            c.resume();
        }
    }

    std::cout << "\nDone" << std::endl;

    return EXIT_SUCCESS;
}
