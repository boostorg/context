
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <tuple>

#include <boost/context/fiber_context.hpp>

namespace ctx = boost::context;

ctx::fiber_context f1( ctx::fiber_context && f) {
    std::cout << "f1: entered first time"  << std::endl;
    f = std::move( f).resume();
    std::cout << "f1: entered second time" << std::endl;
    f = std::move( f).resume();
    std::cout << "f1: entered third time" << std::endl;
    return std::move( f);
}

ctx::fiber_context f2( ctx::fiber_context && f) {
    std::cout << "f2: entered" << std::endl;
    return std::move( f);
}

int main() {
    ctx::fiber_context f{ f1 };
    f = std::move( f).resume();
    std::cout << "f1: returned first time" << std::endl;
    f = std::move( f).resume();
    std::cout << "f1: returned second time" << std::endl;
    f = std::move( f).resume_with( f2);
    std::cout << "f1: returned third time" << std::endl;

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
