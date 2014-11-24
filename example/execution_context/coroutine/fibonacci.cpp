
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/fixedsize.hpp>

#include "coroutine.hpp"

typedef coroutine< int >    coro_t;

int main() {
    int n = 10;
    coro_t::pull_type seq(
        boost::context::fixedsize(),
        [&n]( coro_t::push_type & yield) {
            int first = 1, second = 1;
            yield( first);
            yield( second);
            for ( int i = 0; i < n - 2; ++i) {
                int third = first + second;
                first = second;
                second = third;
                yield( third);
            }
        });

    while ( seq) {
        int x = seq.get();
        std::cout << x << std::endl;
        seq();
    }

    std::cout << "Done" << std::endl;

    return EXIT_SUCCESS;
}
