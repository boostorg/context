
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <memory>

#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

int main() {
    ctx::continuation c=ctx::callcc(
        [](ctx::continuation && c){
            int a=0;
            int b=1;
            for(;;){
                c=ctx::resume(std::move(c),a);
                auto next=a+b;
                a=b;
                b=next;
            }
            return std::move( c);
        });
    for ( int j = 0; j < 10; ++j) {
        std::cout << ctx::transfer_data<int>(c) << " ";
        c=ctx::resume(std::move(c));
    }
    std::cout << std::endl;
    std::cout << "main: done" << std::endl;
}
