
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <emmintrin.h>

#include <boost/bind.hpp>
#include <boost/context/all.hpp>

#include "simple_stack_allocator.hpp"

namespace ctx = boost::context;

typedef ctx::simple_stack_allocator<
    8 * 1024 * 1024, // 8MB
    64 * 1024, // 64kB
    8 * 1024 // 8kB
>       stack_allocator;

void echoSSE( int i) {
    __m128i xmm;
    xmm = _mm_set_epi32( i, i + 1, i + 2, i + 3);
    uint32_t v32[4];
    memcpy( & v32, & xmm, 16);
    std::cout << v32[0]; 
    std::cout << v32[1]; 
    std::cout << v32[2]; 
    std::cout << v32[3]; 
}

void echo( ctx::transfer_t t_) {
    ctx::transfer_t t = ctx::jump_fcontext( t_.fctx, 0);
    for (;;) {
        int i = * ( int *) t.data;
        std::cout << i;
        echoSSE( i);
        std::cout << " ";
        t = ctx::jump_fcontext( t.fctx, 0);
    }
}

int main( int argc, char * argv[]) {
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::default_stacksize() );
    ctx::fcontext_t ctx = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), echo);
    ctx::transfer_t t =ctx::jump_fcontext( ctx, 0);
    for ( int i = 0; i < 10; ++i) {
        t = ctx::jump_fcontext( t.fctx, & i);
    }
    std::cout << "\nDone" << std::endl;
    return EXIT_SUCCESS;
}
