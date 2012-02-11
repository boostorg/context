
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/context/all.hpp>
#include <boost/move/move.hpp>

int x = 7;
void fn( int);

boost::contexts::context ctx( fn, x, boost::contexts::default_stacksize(), boost::contexts::no_stack_unwind, boost::contexts::return_to_caller);

void fn( int j)
{
    for( int i = 0; i < j; ++i)
    {
        std::cout << "fn(): local variable i == " << i << std::endl;
        ctx.suspend();
    }
}

int main( int argc, char * argv[])
{
    std::cout << "main() calls context ctx" << std::endl;
    ctx.start();
    while ( ! ctx.is_complete() )
    {
        std::cout << "main() calls context ctx" << std::endl;
        ctx.resume();
    }

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
