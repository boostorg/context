
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/fiber_context>

std::fiber_context bar( std::fiber_context && f) {
    do {
        std::cout << "bar\n";
        f = std::move( f).resume();
    } while ( f);
    return std::move( f);
}

int main() {
    std::fiber_context f{ bar };
    do {
        std::cout << "foo\n";
        f = std::move( f).resume();
    } while ( f);
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
