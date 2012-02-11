
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include <boost/assert.hpp>
#include <boost/context/all.hpp>

boost_fcontext_t fcm, fc1, fc2;

void f1( intptr_t p)
{
        (void) p;
        fprintf(stderr,"f1() stated\n");
        fprintf(stderr,"f1: call boost_fcontext_jump( & fc1, & fc2)\n");
        boost_fcontext_jump( & fc1, & fc2, 0);
        fprintf(stderr,"f1() returns\n");
}

void f2( intptr_t p)
{
        (void) p;
        fprintf(stderr,"f2() stated\n");
        fprintf(stderr,"f2: call boost_fcontext_jump( & fc2, & fc1)\n");
        boost_fcontext_jump( & fc2, & fc1, 0);
        BOOST_ASSERT( false && ! "f2() never returns");
}

int main( int argc, char * argv[])
{
        boost::contexts::stack_allocator alloc;

        fc1.fc_stack.ss_base = alloc.allocate(262144);
        fc1.fc_stack.ss_limit =
            static_cast< char * >( fc1.fc_stack.ss_base) - 262144;
        fc1.fc_link = & fcm;
        boost_fcontext_make( & fc1, f1, 0);

        fc2.fc_stack.ss_base = alloc.allocate(262144);
        fc2.fc_stack.ss_limit =
            static_cast< char * >( fc2.fc_stack.ss_base) - 262144;
        boost_fcontext_make( & fc2, f2, 0);

        fprintf(stderr,"main: call boost_fcontext_start( & fcm, & fc1)\n");
        boost_fcontext_start( & fcm, & fc1);

        fprintf( stderr, "main() returns\n");
        return EXIT_SUCCESS;
}
