
//          Copyright Nat Goodspeed, Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <boost/context/continuation.hpp>

namespace ctx = boost::context;

std::string with_single( ctx::continuation && caller) {
    std::string single;
    if ( ! caller.data_available() ) {
        single.clear();
    } else {
        single = caller.get_data< std::string >();
    }
    std::cout << "with_single() retrieved '" << single << "'" << std::endl;
    return single;
}

ctx::continuation get_single( ctx::continuation && caller) {
    std::cout << "entering get_single()" << std::endl;
    while ( caller.data_available() ) {
        std::string single = caller.get_data< std::string >();
        std::cout << "get_single() retrieved '" << single << "'" << std::endl;
        caller = caller.resume();
    }
    std::cout << "exiting get_single()" << std::endl;
    return std::move( caller);
}

int main() {
    ctx::continuation single = ctx::callcc( get_single, std::string("first") );
    single = single.resume_with( with_single, std::string("second") );
    single = single.resume_with( with_single, std::string("third") );
    single = single.resume_with( with_single);
    std::cout << "get_single() is";
    if ( single) {
        std::cout << " not";
    }
    std::cout << " done" << std::endl;
    return 0;
}
