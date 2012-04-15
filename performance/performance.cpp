
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_PP_LIMIT_MAG  10

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/context/all.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#ifndef BOOST_WINDOWS
#include <ucontext.h>
#endif

#include "bind_processor.hpp"
#include "performance.hpp"

namespace ctx = boost::ctx;

#define CALL_UCONTEXT(z,n,unused) \
    ::swapcontext( & ucm, & uc);

#define CALL_FCONTEXT(z,n,unused) \
    ctx::jump_fcontext( & fcm, & fc, 0);

#ifndef BOOST_WINDOWS
ucontext_t uc, ucm;
#endif
ctx::fcontext_t fc, fcm;

#ifndef BOOST_WINDOWS
static void f2()
{
    while ( true)
        ::swapcontext( & uc, & ucm);
}
#endif

static void f1( intptr_t)
{
    while ( true)
        ctx::jump_fcontext( & fc, & fcm, 0);
}

#ifndef BOOST_WINDOWS
unsigned int test_ucontext()
{
    cycle_t overhead( get_overhead() );
    std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

    ctx::stack_allocator alloc;

    ::getcontext( & uc);
    uc.uc_stack.ss_sp = 
        static_cast< char * >( alloc.allocate(ctx::default_stacksize() ) )
        - ctx::default_stacksize();
    uc.uc_stack.ss_size = ctx::default_stacksize();
    ::makecontext( & uc, f2, 0);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)

    cycle_t start( get_cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)
    cycle_t total( get_cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= overhead; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
#endif

unsigned int test_fcontext()
{
    cycle_t overhead( get_overhead() );
    std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

    ctx::stack_allocator alloc;
    fc.fc_stack.base = alloc.allocate(ctx::default_stacksize());
    fc.fc_stack.limit =
        static_cast< char * >( fc.fc_stack.base) - ctx::default_stacksize();
	ctx::make_fcontext( & fc, f1, 0);

    ctx::start_fcontext( & fcm, & fc);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)

    cycle_t start( get_cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)
    cycle_t total( get_cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= overhead; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}

int main( int argc, char * argv[])
{
    try
    {
        bind_to_processor( 0);

        unsigned int res = test_fcontext();
        std::cout << "fcontext: average of " << res << " cycles per switch" << std::endl;
#ifndef BOOST_WINDOWS
        res = test_ucontext();
        std::cout << "ucontext: average of " << res << " cycles per switch" << std::endl;
#endif

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}

#undef CALL_FCONTEXT
#undef CALL_UCONTEXT
