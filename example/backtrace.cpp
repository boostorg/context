
//          Copyright Oliver Kowalke 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define UNW_LOCAL_ONLY

#include <unwind.h>
#include <cxxabi.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <libunwind.h>

#include <boost/context/fiber_context.hpp>

#define _U_VERSION      1

namespace ctx = boost::context;

struct X {
    int     i;
    X(int i_) : i{ i_ } { std::cout << "X(" << i << ")" << std::endl; }
    ~X() { std::cout << "~X(" << i << ")" << std::endl; }
};

void print_data( unw_cursor_t * cursor) {
    unw_proc_info_t pi;
    if ( 0 > unw_get_proc_info( cursor, & pi) ) {
        std::fprintf( stderr, "unw_get_proc_info failed\n");
        return;
    }
    char sym[256];
    unw_word_t offset;
    if ( 0 > unw_get_proc_name( cursor, sym, sizeof( sym), & offset) ) {
        std::fprintf( stderr, "unw_get_proc_name failed\n");
        return;
    }
    char * nameptr = sym;
    int status;
    char * demangled = abi::__cxa_demangle( sym, nullptr, nullptr, & status);
    if ( 0 == status) {
        nameptr = demangled;
    }
    std::printf("(%s\n", nameptr);
    std::free( demangled);
    std::printf("lsda=0x%lx, personality=0x%lx\n", pi.lsda, pi.handler);
}

void backtrace( ctx::fiber_context && c) {
    unw_cursor_t cursor;
	unw_context_t context;
	unw_getcontext( & context);
	unw_init_local( & cursor, & context);
   // print_data( & cursor);
    while ( 0 < unw_step( & cursor) ) {
       print_data( & cursor);
       std::printf("\n");
	}
    std::printf("unwinding done\n");
    std::move( c).resume();
}

void  /* __attribute__((optimize("O0"))) */ bar( ctx::fiber_context && c) {
    X x{ 3 };
	backtrace( std::move( c) );
    std::cout << "returned from unwind()" << std::endl;
}

void /*  __attribute__((optimize("O0"))) */ foo( ctx::fiber_context && c) {
    X x{ 2 };
	bar( std::move( c) );
    std::cout << "returned from bar()" << std::endl;
}

ctx::fiber_context /* __attribute__((optimize("O0"))) */ f( ctx::fiber_context && c) {
    X x{ 1 };
    foo( std::move( c) );
    std::cout << "returned from foo()" << std::endl;
    return std::move( c);
}

int main() {
    ctx::fiber_context{ f }.resume();
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
