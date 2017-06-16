
//          Copyright Nat Goodspeed, Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <tuple>
#include <string>
#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

std::tuple< std::string, int > with_tuple( ctx::continuation && caller) {
    std::tuple< std::string, int > tuple;
    if ( ! caller.data_available() ) {
        tuple = std::tuple< std::string, int >{};
    } else {
        tuple = caller.get_data< std::string, int >();
    }
    std::cout << "with_tuple() retrieved ('" << std::get< 0 >( tuple) << "', " << std::get< 1 >( tuple) << ")"
              << std::endl;
    return tuple;
}

ctx::continuation get_tuple( ctx::continuation && caller) {
    std::cout << "entering get_tuple()" << std::endl;
    while ( caller.data_available() ) {
        std::tuple< std::string, int > tuple = caller.get_data< std::string, int >();
        std::cout << "get_tuple() retrieved ('" << std::get< 0 >( tuple) << "', " << std::get< 1 >( tuple) << ")"
                  << std::endl;
        caller = caller.resume();
    }
    std::cout << "exiting get_tuple()" << std::endl;
    return std::move( caller);
}

int main() {
    ctx::continuation tuple = ctx::callcc( get_tuple, std::string("first"), 1);
    tuple = tuple.resume_with( with_tuple, std::string("second"), 2);
    tuple = tuple.resume_with( with_tuple, std::string("third"), 3 );
    tuple = tuple.resume_with( with_tuple);
    std::cout << "get_tuple() is";
    if ( tuple) {
        std::cout << " not";
    }
    std::cout << " done" << std::endl;
    return 0;
}
