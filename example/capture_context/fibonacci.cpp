
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <memory>

#include <boost/context/all.hpp>

namespace ctx = boost::context;

int main() {
    int n=35;
    void * data;
    ctx::captured_context source(
        [n](ctx::captured_context sink,void* ignored)mutable->ctx::captured_context{
            int a=0;
            int b=1;
            while(n-->0){
                std::tie(sink,ignored)=sink(&a);
                auto next=a+b;
                a=b;
                b=next;
            }
            return sink;
        });
    for(int i=0;i<10;++i){
        std::tie(source,data)=source();
        std::cout<<*(int*)data<<" ";
    }
    std::cout<<std::endl;

    std::cout << "main: done" << std::endl;
}
