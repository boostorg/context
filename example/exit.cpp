
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <iostream>

#include <boost/assert.hpp>
#include <boost/context/all.hpp>

#include "simple_stack_allocator.hpp"

namespace ctx = boost::context;

typedef ctx::simple_stack_allocator<
    8 * 1024 * 1024, // 8MB
    64 * 1024, // 64kB
    8 * 1024 // 8kB
>       stack_allocator;

void f1( ctx::transfer_t t) {
    std::cout << "f1: entered" << std::endl;
    std::cout << "f1: call jump_fcontext( ctx2, 0)" << std::endl;
    ctx::jump_fcontext( t.data, 0);
    std::cout << "f1: return" << std::endl;
}

void f2( ctx::transfer_t t) {
    std::cout << "f2: entered" << std::endl;
    std::cout << "f2: call jump_fcontext( ctx, 0)" << std::endl;
    ctx::jump_fcontext( t.fctx, t.data);
    BOOST_ASSERT( false && ! "f2: never returns");
}

int main( int argc, char * argv[]) {
    std::cout << "size: 0x" << std::hex << sizeof( ctx::fcontext_t) << std::endl;

    stack_allocator alloc;

    void * sp1 = alloc.allocate( stack_allocator::default_stacksize());
    ctx::fcontext_t ctx1 = ctx::make_fcontext( sp1, stack_allocator::default_stacksize(), f1);

    void * sp2 = alloc.allocate( stack_allocator::default_stacksize());
    ctx::fcontext_t ctx2 = ctx::make_fcontext( sp2, stack_allocator::default_stacksize(), f2);

    std::cout << "main: call start_fcontext( ctx1, 0)" << std::endl;
    ctx::jump_fcontext( ctx1, ctx2);

    std::cout << "main: done" << std::endl;
    BOOST_ASSERT( false && ! "main: never returns");

    return EXIT_SUCCESS;
}
