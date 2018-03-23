
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
    std::cout << "main-thread: " << std::this_thread::get_id() << std::endl;
    ctx::fiber f{
            [](ctx::fiber && m){
                m = std::move( m).resume();
                return std::move( m);
            }};
    std::cout << "hosting thread ID before resumption: " << f.previous_thread() << std::endl;
    f = std::move( f).resume();
    std::cout << "hosting thread ID after resumption: " << f.previous_thread() << std::endl;
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
