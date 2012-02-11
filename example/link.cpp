
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/context/all.hpp>

void fn1()
{
    std::cout << "inside fn1(): when fn1() returns fn2() of next context (linked) will be entered"  << std::endl;
}

boost::contexts::context ctx2;

void fn2()
{
    std::cout << "first time inside fn2()" << std::endl;
    ctx2.suspend();
    std::cout << "second time inside fn2(), returns to main()" << std::endl;
}

int main( int argc, char * argv[])
{
    {
        ctx2 = boost::contexts::context(
            fn2, 
            boost::contexts::default_stacksize(),
			boost::contexts::no_stack_unwind, boost::contexts::return_to_caller);
        boost::contexts::context ctx1(
            fn1, 
            boost::contexts::default_stacksize(),
			boost::contexts::no_stack_unwind,
            ctx2);

        ctx1.start();
    }

    std::cout << "main(): ctx1 is destructed\n";

    std::cout << "main(): resume ctx2\n";
    ctx2.resume();

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
