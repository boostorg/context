
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/context/fiber_context.hpp>

namespace ctx = boost::context;

int main() {
    try {
        std::cout << "f0: entered" << std::endl;
        ctx::fiber_context f1{
                [](ctx::fiber_context && f0_){
                    try {
                        std::cout << "f1: entered" << std::endl;
                        ctx::fiber_context f0 = std::move( f0_);
                        ctx::fiber_context f2{
                                [](ctx::fiber_context && f1_){
                                    try {
                                        std::cout << "f2: entered" << std::endl;
                                        ctx::fiber_context f1 = std::move( f1_);
                                        ctx::fiber_context f3{
                                            [](ctx::fiber_context && f2){
                                                std::cout << "f3: entered" << std::endl;
                                                std::cout << "f3: unwinding f2" << std::endl;
                                                ctx::unwind_fiber( std::move( f2) );
                                                if ( ! f2) {
                                                    std::cout << "f3: no valid fiber_context so calling `std::exit(1)`" << std::endl;
                                                    std::exit( 1);
                                                }
                                                std::cout << "f3: done" << std::endl;
                                                return std::move( f2);
                                            }};
                                        std::cout << "f2: resume f3" << std::endl;
                                        f3 = std::move( f3).resume();
                                        std::cout << "f2: done" << std::endl;
                                    } catch (...) {
                                        std::cout << "f2: catch unwind exception" << std::endl;
                                        throw;
                                    }
                                    return std::move( f1_);
                                }};
                        std::cout << "f1: resume f2" << std::endl;
                        f2 = std::move( f2).resume();
                        std::cout << "f1: done" << std::endl;
                    } catch (...) {
                        std::cout << "f1: catch unwind exception" << std::endl;
                        throw;
                    }
                    return std::move( f0_);
                }};
        std::cout << "f0: resume f1" << std::endl;
        f1 = std::move( f1).resume();
        std::cout << "f0: done" << std::endl;
    } catch (...) {
        std::cout << "f0: catch unwind exception" << std::endl;
        throw;
    }
    return EXIT_SUCCESS;
}
