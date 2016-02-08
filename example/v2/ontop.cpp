
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <tuple>

#include <boost/context/all.hpp>

namespace ctx = boost::context;

ctx::execution_context< int > f1( ctx::execution_context< int > ctx, int data) {
    std::cout << "f1: entered first time: " << data  << std::endl;
    std::tie( ctx, data) = ctx( data);
    std::cout << "f1: entered second time: " << data  << std::endl;
    std::tie( ctx, data) = ctx( data);
    std::cout << "f1: entered third time: " << data << std::endl;
    return ctx;
}

ctx::execution_context< int > f2( ctx::execution_context< int > ctx, int data) {
    std::cout << "f2: entered: " << data << std::endl;
    return ctx;
}

int main() {
    int data = 3;
    ctx::execution_context< int > ctx( f1);
    std::tie( ctx, data) = ctx( data);
    std::cout << "f1: returned first time: " << data << std::endl;
    std::tie( ctx, data) = ctx( data);
    std::cout << "f1: returned second time: " << data << std::endl;
    std::tie( ctx, data) = ctx( ctx::exec_ontop_arg, f2, data + 2);
    std::cout << "f1: returned third time: " << data << std::endl;

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
