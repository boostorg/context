
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <tuple>

#include <boost/context/all.hpp>

namespace ctx = boost::context;

ctx::execution_context f1( ctx::execution_context ctx, void * ignored) {
    std::cout << "f1: entered first time" << std::endl;
    std::tie( ctx, ignored) = ctx();
    std::cout << "f1: entered second time" << std::endl;
    std::tie( ctx, ignored) = ctx();
    std::cout << "f1: entered third time" << std::endl;
    return ctx;
}

std::tuple< ctx::execution_context, void * > f2( ctx::execution_context ctx, void * data) {
    std::cout << "f2: entered" << std::endl;
    return std::make_tuple( std::move( ctx), data);
}

int main() {
    void * ignored;
    ctx::execution_context ctx( f1);
    std::tie( ctx, ignored) = ctx();
    std::cout << "f1: returned first time" << std::endl;
    std::tie( ctx, ignored) = ctx();
    std::cout << "f1: returned second time" << std::endl;
    std::tie( ctx, ignored) = ctx( ctx::exec_ontop_arg, f2);
    std::cout << "f1: returned third time" << std::endl;

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
