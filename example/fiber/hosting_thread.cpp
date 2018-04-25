
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define UNW_LOCAL_ONLY

#include <cstdlib>
#include <iostream>
#include <thread>

#include <boost/context/fiber.hpp>

namespace ctx = boost::context;

int main() {
    ctx::fiber_handle f{
            [](ctx::fiber_handle && m){
                std::cout << "m in main thread: " << std::boolalpha << m.can_resume() << std::endl;
                ctx::fiber_handle * pm = & m;
                std::thread{ [pm]{ std::cout << "m in other thread: " << std::boolalpha << pm->can_resume() << std::endl; }}.join();
                m = std::move( m).resume();
                return std::move( m);
            }};
    std::cout << "f: before resumption: " << std::boolalpha << f.can_resume() << std::endl;
    f = std::move( f).resume();
    std::cout << "f: after resumption: " << std::boolalpha << f.can_resume() << std::endl;
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
