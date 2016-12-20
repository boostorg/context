
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <tuple>

#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

ctx::continuation f1( ctx::continuation && c, int data) {
    std::cout << "f1: entered first time: " << data  << std::endl;
    std::tie( c, data) = ctx::callcc< int >( std::move( c), data + 1);
    std::cout << "f1: entered second time: " << data  << std::endl;
    std::tie( c, data) = ctx::callcc< int >( std::move( c), data + 1);
    std::cout << "f1: entered third time: " << data << std::endl;
    return std::move( c);
}

int f2( int data) {
    std::cout << "f2: entered: " << data << std::endl;
    return -1;
}

int main() {
    ctx::continuation c;
    int data = 0;
    std::tie( c, data) = ctx::callcc< int >( f1, data + 1);
    std::cout << "f1: returned first time: " << data << std::endl;
    std::tie( c, data) = ctx::callcc< int >( std::move( c), data + 1);
    std::cout << "f1: returned second time: " << data << std::endl;
    std::tie( c, data) = ctx::callcc< int >( std::move( c), ctx::exec_ontop_arg, f2, data + 1);
    std::cout << "f1: returned third time" << std::endl;
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
