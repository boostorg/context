
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/context/all.hpp>
#include <boost/move/move.hpp>

void fn( double d)
{
    d += 3.45;
    std::cout << "d == " << d << std::endl;
}

int main( int argc, char * argv[])
{
    boost::contexts::context ctx(
        fn, 7.34,
        boost::contexts::default_stacksize(),
        boost::contexts::stack_unwind, boost::contexts::return_to_caller);
    ctx.start();

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
