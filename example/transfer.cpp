
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/context/all.hpp>


boost::contexts::context ctx;

void fn()
{
	  int i = 7;
      std::cout << "fn(): local variable i == " << i << std::endl;

      // save current context
      // transfer execution control back to caller
	  // pass pointer to local variable i
      intptr_t vp = ctx.suspend( i);
	  int j = vp;

      std::cout << "transfered value: " << j << std::endl;
}

int main( int argc, char * argv[])
{
    ctx = boost::contexts::context( fn,
		boost::contexts::default_stacksize(),
		boost::contexts::no_stack_unwind,
		boost::contexts::return_to_caller);

    std::cout << "main() calls context ctx" << std::endl;

	// start the context ctx for the first time
	// enter fn()
	intptr_t vp = ctx.start();
	int x = vp;

    std::cout << "transfered value: " << x << std::endl;
	x = 10;

	// ctx.suspend() was called so we returned from start()
    ctx.resume( x);

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
