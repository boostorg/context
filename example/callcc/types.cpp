
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>
#include <string>

#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

ctx::continuation f1( ctx::continuation && c) {
    int i = 3;
    c = c( i);
    std::string s{ "abc" };
    c = c( s);
    i = 7; s = "xyz";
    c = c( i, s);
    c = c();
    return std::move( c);
}

int main() {
    ctx::continuation c = ctx::callcc( f1);
    int i = ctx::get_data< int >( c);
    std::cout << "f1: returned : " << i << std::endl;
    c = c();
    std::string s = ctx::get_data< std::string >( c);
    std::cout << "f1: returned : " << s << std::endl;
    c = c();
    std::tie(i,s)=ctx::get_data< int, std::string >( c);
    std::cout << "f1: returned : " << i << ", " << s << std::endl;
    c = c();
    std::cout << std::boolalpha;
    std::cout << "f1: returned data : " << ctx::data_available( c) << std::endl;
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
