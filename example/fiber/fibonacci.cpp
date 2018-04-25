
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <memory>

#include <boost/context/fiber.hpp>

namespace ctx = boost::context;

class F {
private:
    int a;
    ctx::fiber f;
    
public:
    F() :
        a{ 0 },
        f{ [this](ctx::fiber && f){
                a=0;
                int b=1;
                for(;;){
                    f = std::move( f).resume();
                    int next=a+b;
                    a=b;
                    b=next;
                }
                return std::move( f);
            }} {
    }

    int call() {
        f = std::move( f).resume();
        return a;
    }
};

int main() {
    F f;
    for ( int j = 0; j < 10; ++j) {
        std::cout << f.call() << " ";
    }
    std::cout << std::endl;
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
