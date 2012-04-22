
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
#include <boost/program_options.hpp>

#ifndef BOOST_WINDOWS
#include <ucontext.h>
#endif

#include "bind_processor.hpp"
#include "cycle.hpp"

namespace ctx = boost::ctx;
namespace po = boost::program_options;

bool preserve_fpu = true;

#define CALL_UCONTEXT(z,n,unused) \
    ::swapcontext( & ucm, & uc);

#define CALL_FCONTEXT(z,n,unused) \
    ctx::jump_fcontext( & fcm, & fc, 7, preserve_fpu);

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
        ctx::jump_fcontext( & fc, & fcm, 7, preserve_fpu);
}

#ifndef BOOST_WINDOWS
cycle_t test_ucontext( cycle_t ov)
{
    ctx::stack_allocator alloc;

    ::getcontext( & uc);
    uc.uc_stack.ss_sp = 
        static_cast< char * >( alloc.allocate(ctx::default_stacksize() ) )
        - ctx::default_stacksize();
    uc.uc_stack.ss_size = ctx::default_stacksize();
    ::makecontext( & uc, f2, 7);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_UCONTEXT, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}
#endif

cycle_t test_fcontext( cycle_t ov)
{
    ctx::stack_allocator alloc;
    fc.fc_stack.base = alloc.allocate(ctx::default_stacksize());
    fc.fc_stack.limit =
        static_cast< char * >( fc.fc_stack.base) - ctx::default_stacksize();
	ctx::make_fcontext( & fc, f1);

    ctx::jump_fcontext( & fcm, & fc, 7, preserve_fpu);

    // cache warum-up
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)

    cycle_t start( cycles() );
BOOST_PP_REPEAT_FROM_TO( 0, BOOST_PP_LIMIT_MAG, CALL_FCONTEXT, ~)
    cycle_t total( cycles() - start);

    // we have two jumps and two measuremt-overheads
    total -= ov; // overhead of measurement
    total /= BOOST_PP_LIMIT_MAG; // per call
    total /= 2; // 2x jump_to c1->c2 && c2->c1

    return total;
}

int main( int argc, char * argv[])
{
    try
    {
        po::options_description desc("allowed options");
        desc.add_options()
            ("help,h", "help message")
            ("preserve-fpu,p", po::value< bool >( & preserve_fpu), "preserve floating point env");

        po::variables_map vm;
        po::store(
            po::parse_command_line(
                argc,
                argv,
                desc),
            vm);
        po::notify( vm);

        if ( vm.count("help") )
        {
            std::cout << desc << std::endl;
            return EXIT_SUCCESS;
        }
        bind_to_processor( 0);

        cycle_t ov( overhead() );
        std::cout << "overhead for rdtsc == " << ov << " cycles" << std::endl;

        unsigned int res = test_fcontext( ov);
        std::cout << "fcontext: average of " << res << " cycles per switch" << std::endl;
#ifndef BOOST_WINDOWS
        res = test_ucontext( ov);
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
