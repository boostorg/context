
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <memory>

#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

int main() {
    ctx::continuation c;
    int i=0;
    c=ctx::callcc(
        std::allocator_arg,
        ctx::fixedsize_stack(),
        [](ctx::continuation && c){
            int a=0;
            int b=1;
            for(;;){
                c=ctx::callcc(std::move(c),a);
                auto next=a+b;
                a=b;
                b=next;
            }
            return std::move( c);
        },
        0);
    for ( int j = 0; j < 10; ++j) {
        c=ctx::callcc(std::move(c),0);
        i=ctx::data<int>(c);
        std::cout<<i<<" ";
    }
    std::cout<<std::endl;
    std::cout << "main: done" << std::endl;
}
