
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

    X( X const&) = delete;
    X& operator=( X const&) = delete;

    X( X &&) = delete;
    X& operator=( X &&) = delete;
};

ctx::fiber_context bar( ctx::fiber_context && f_foo) {
	std::printf("bar entered\n");
    X x2{ 2 };
	std::printf("bar resumes foo\n");
    f_foo = std::move( f_foo).resume();
	std::printf("bar returns\n");
    return std::move( f_foo);
}

ctx::fiber_context foo( ctx::fiber_context && f_main) {
	std::printf("foo entered\n");
    ctx::fiber_context f_bar{ bar }; 
	std::printf("foo resumes bar\n");
    f_bar = std::move( f_bar).resume();
	std::printf("foo destroys main\n");
    ctx::unwind_fiber( std::move( f_main) );
	std::printf("foo resumes bar\n");
    std::move( f_bar).resume();
	std::printf("foo terminates application\n");
    std::exit( 0);
	return {};
};

int main( int argc, char * argv[]) {
    X x1{ 1 };
    ctx::fiber_context f{ foo };
	std::printf("main resumes foo\n");
    f = std::move( f).resume();
    return EXIT_SUCCESS;
}
