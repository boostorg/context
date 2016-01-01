
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <cstring>
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

void f2( ctx::transfer_t t) {
    std::cout << "f2: entered" << std::endl;
    ctx::jump_fcontext( t.fctx, t.data);
}

void f1( ctx::transfer_t t_) {
    ctx::transfer_t t = t_;
    std::cout << "f1: entered" << std::endl;
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::default_stacksize());
    ctx::fcontext_t ctx_ = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), f2);
    ctx::jump_fcontext( ctx_, 0);
    ctx::jump_fcontext( t.fctx, 0);
}

int main( int argc, char * argv[]) {
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::default_stacksize());
    ctx::fcontext_t ctx = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), f1);

    std::cout << "main: call start_fcontext( ctx, 0)" << std::endl;
    ctx::jump_fcontext( ctx, 0);

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
