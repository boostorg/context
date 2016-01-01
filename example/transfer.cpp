
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

typedef std::pair< int, int >   pair_t;

void f1( ctx::transfer_t t_) {
    pair_t * p = ( pair_t *) t_.data;

    int result =  p->first + p->second;
    ctx::transfer_t t = ctx::jump_fcontext( t_.fctx, & result);
    p = ( pair_t *) t.data;

    result =  p->first + p->second;
    ctx::jump_fcontext( t.fctx, & result);
}

int main( int argc, char * argv[]) {
    stack_allocator alloc;

    void * sp = alloc.allocate( stack_allocator::default_stacksize() );
    ctx::fcontext_t ctx = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), f1);

    pair_t p( std::make_pair( 2, 7) );
    ctx::transfer_t t = ctx::jump_fcontext( ctx, & p);
    std::cout << p.first << " + " << p.second << " == " << * ( int *)t.data << std::endl;

    p = std::make_pair( 5, 6);
    t = ctx::jump_fcontext( t.fctx, & p);
    std::cout << p.first << " + " << p.second << " == " << * ( int *)t.data << std::endl;

    std::cout << "main: done" << std::endl;

    return EXIT_SUCCESS;
}
