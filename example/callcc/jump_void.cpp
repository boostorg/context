
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

ctx::continuation f1( ctx::continuation && cm) {
    std::cout << "f1: entered first time" << std::endl;
    cm = ctx::callcc< void >( std::move( cm) );
    std::cout << "f1: entered second time" << std::endl;
    return std::move( cm);
}

int main() {
    ctx::continuation c = ctx::callcc< void >( f1);
    std::cout << "f1: returned first time" << std::endl;
    c = ctx::callcc< void >( std::move( c) );
    std::cout << "f1: returned second time" << std::endl;

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
