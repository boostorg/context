
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>

#include <boost/context/fixedsize.hpp>

#include "coroutine.hpp"

typedef coroutine< void >    coro_t;

int main( int argc, char * argv[]) {
    {
        coro_t::push_type sink( boost::context::fixedsize(),
                                [](coro_t::pull_type & source){
                                    while ( source) {
                                        std::cout << "foo: entered" << std::endl;
                                        source();
                                    }
                                });
        sink();
        std::cout << "main: back" << std::endl;
        sink();
    }
    std::cout << "main: back" << std::endl;
    {
        coro_t::pull_type sink( boost::context::fixedsize(),
                                [](coro_t::push_type & source){
                                    std::cout << "bar: entered" << std::endl;
                                    source();
                                    std::cout << "bar: entered" << std::endl;
                                    source();
                                    std::cout << "bar: entered" << std::endl;
                                });
        while ( sink) {
            sink();
            std::cout << "main: back" << std::endl;
        }
    }
    std::cout << "done" << std::endl;

    return EXIT_SUCCESS;
}
