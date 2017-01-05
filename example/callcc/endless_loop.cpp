
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

ctx::continuation foo( ctx::continuation && c) {
    for (;;) {
        std::cout << "foo\n";
        c = ctx::resume( std::move( c) );
    }
    return std::move( c);
}

int main() {
    ctx::continuation c = ctx::callcc( foo);
    for (;;) {
        std::cout << "bar\n";
        c = ctx::resume( std::move( c) );
    }
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
