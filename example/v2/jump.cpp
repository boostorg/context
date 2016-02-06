
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/all.hpp>

namespace ctx = boost::context;

ctx::execution_context f1( ctx::execution_context ctxm, void * data) {
    std::cout << "f1: entered first time" << std::endl;
    std::tie( ctxm, data) = ctxm();
    std::cout << "f1: entered second time" << std::endl;
    ctx::execution_context ctx2 = std::move( * static_cast< ctx::execution_context * >( data) );
    ctx2( & ctxm);
    return ctxm;
}

ctx::execution_context f2( ctx::execution_context ctx1, void * data) {
    std::cout << "f2: entered first time" << std::endl;
    ctx::execution_context ctxm = std::move( * static_cast< ctx::execution_context * >( data) );
    return ctxm;
}

int main() {
    ctx::execution_context ctx1( f1);
    void * ignored;
    std::tie( ctx1, ignored) = ctx1();
    ctx::execution_context ctx2( f2);
    std::tie( ctx1, ignored) = ctx1( & ctx2);

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
