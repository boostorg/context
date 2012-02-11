
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/context/all.hpp>
#include <boost/move/move.hpp>

void fn()
{
    std::cout << "inside function fn(): fn() returns return to main()" << std::endl;
}

int main( int argc, char * argv[])
{
    {
        boost::contexts::context ctx(
			fn,
        	boost::contexts::default_stacksize(),
			boost::contexts::no_stack_unwind, boost::contexts::return_to_caller);

        ctx.start();
    }

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
