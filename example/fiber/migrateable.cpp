
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define UNW_LOCAL_ONLY

#include <cstdlib>
#include <iostream>

#include <boost/context/fiber.hpp>

namespace ctx = boost::context;

int main() {
    ctx::fiber f{
            [](ctx::fiber && m){
                std::cout << "fiber `m`: " << std::boolalpha << m.uses_system_stack() << std::endl;
                m = std::move( m).resume();
                return std::move( m);
            }};
    std::cout << "fiber `f`: " << std::boolalpha << f.uses_system_stack() << std::endl;
    f = std::move( f).resume();
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
