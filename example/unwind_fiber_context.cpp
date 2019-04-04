
//          Copyright Oliver Kowalke 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unwind.h>

#include <boost/context/fiber_context.hpp>

namespace ctx = boost::context;

struct X {
    int i;
    X( int i_) : i{ i_ } { std::printf("X(%d)\n", i); }
    ~X() { std::printf("~X(%d)\n", i); }
};

void bar( int i, ctx::fiber_context && fc) {
	if ( 0 < i) {
		X x{ i };
		bar( --i, std::move( fc) );
	} else {
        try {
            std::move( fc).resume();
        } catch (...) {
            std::printf("catch-all: needs throw\n");
            throw;
        }
	}
	std::printf("bar returns\n");
}

ctx::fiber_context foo( ctx::fiber_context && fc) {
	bar( 5, std::move( fc) );
	std::printf("foo returns\n");
	return {};
};

void throw_int() {
    try {
        throw 1;
    } catch (...) {
        std::printf("int catched\n");
    }
}

void throw_exception() {
    try {
        throw std::runtime_error("abc");
    } catch (...) {
        std::printf("std::runtime_error() catched\n");
    }
}

int main( int argc, char * argv[]) {
    throw_int();
    throw_exception();
    {
        ctx::fiber_context f{ foo };
        f = std::move( f).resume();
    }
    std::printf("main: done\n");
    return EXIT_SUCCESS;
}
