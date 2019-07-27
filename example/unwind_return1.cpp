
//          Copyright Oliver Kowalke 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <memory>

#include <boost/context/fiber_context.hpp>

namespace ctx = boost::context;

class fibonacci_generator {
private:
    bool                terminate{ false };
    ctx::fiber_context  fcallee;
    ctx::fiber_context  fexit;
    uint64_t            a;

    void run_(ctx::fiber_context && fcaller) {
        a=0;
        int b=1;
        for(;;){
            fcaller = std::move( fcaller).resume();
            if ( terminate) {
                return;
            }
            int next=a+b;
            a=b;
            b=next;
        }
    }

public:
    fibonacci_generator() :
        fcallee{
            [this](ctx::fiber_context && fcaller) -> ctx::fiber_context {
                run_( std::move( fcaller) );
                return std::move( fexit);
        } } {
    }

    ~fibonacci_generator() {
        if ( fcallee) {
            std::move( fcallee).resume_with(
                    [this](ctx::fiber_context && f) -> ctx::fiber_context {
                        terminate = true;
                        fexit = std::move( f);
                        return {};
                    });
        }
    }

    fibonacci_generator( fibonacci_generator const&) = delete;
    fibonacci_generator & operator=( fibonacci_generator const&) = delete;

    uint64_t operator()() {
        fcallee = std::move( fcallee).resume();
        return a;
    }
};

int main() {
    fibonacci_generator generator;
    for ( int j = 0; j < 10; ++j) {
        std::cout << generator() << " ";
    }
    std::cout << std::endl;
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
