
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
v1::execution_context * ctx2 = nullptr;
v1::execution_context * ctxm = nullptr;

void f1( int i, void *) {
    std::cout << "f1: entered" << std::endl;
    std::cout << "i == " << i << std::endl;
    ( * ctx2)();
    std::cout << "f1: re-entered" << std::endl;
    ( * ctx2)();
}

void f2( void *) {
    std::cout << "f2: entered" << std::endl;
    ( * ctx1)();
    std::cout << "f2: re-entered" << std::endl;
    ( * ctxm)();
}

int main() {
    {
        v1::execution_context ctx1_( f1, 3);
        ctx1 = & ctx1_;
        v1::execution_context ctx2_( f2);
        ctx2 = & ctx2_;
        v1::execution_context ctx_( v1::execution_context::current() );
        ctxm = & ctx_;
        
        ( * ctx1)();
    }
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
