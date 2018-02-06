
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/all.hpp>

namespace ctx = boost::context;
namespace v1 = ctx::v1;

v1::execution_context * ctx1 = nullptr;
v1::execution_context * ctxm = nullptr;

void f1( void *) {
    std::cout << "f1: entered first time" << std::endl;
    ( * ctxm)();
    std::cout << "f1: entered second time" << std::endl;
    ( * ctxm)();
    std::cout << "f1: entered third time" << std::endl;
    ( * ctxm)();
}

void * f2( void * data) {
    std::cout << "f2: entered" << std::endl;
    return data;
}

int main() {
    v1::execution_context ctx1_( f1);
    ctx1 = & ctx1_;
    v1::execution_context ctx_( v1::execution_context::current() );
    ctxm = & ctx_;

    ( * ctx1)();
    std::cout << "f1: returned first time" << std::endl;
    ( * ctx1)();
    std::cout << "f1: returned second time" << std::endl;
    ( * ctx1)( ctx::exec_ontop_arg, f2);
    std::cout << "f1: returned third time" << std::endl;

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
