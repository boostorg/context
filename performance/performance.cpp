
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/config.hpp>
#include <boost/context/all.hpp>
#include <boost/program_options.hpp>

#ifndef BOOST_WINDOWS
#include <ucontext.h>
#endif

#include "bind_processor.hpp"
#include "performance.hpp"

namespace ctx = boost::ctx;
namespace po = boost::program_options;

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
unsigned int test_ucontext( unsigned int iterations)
{
    cycle_t total( 0);
    cycle_t overhead( get_overhead() );
    std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

    // cache warum-up
    {
        ctx::stack_allocator alloc;

        ::getcontext( & uc);
        uc.uc_stack.ss_sp = 
            static_cast< char * >( alloc.allocate(ctx::default_stacksize() ) )
            - ctx::default_stacksize();
        uc.uc_stack.ss_size = ctx::default_stacksize();
        ::makecontext( & uc, f2, 0);
        swapcontext( & ucm, & uc);
        swapcontext( & ucm, & uc);
    }

    for ( unsigned int i = 0; i < iterations; ++i)
    {
        cycle_t start( get_cycles() );
        swapcontext( & ucm, & uc);
        cycle_t diff( get_cycles() - start);

        // we have two jumps and two measuremt-overheads
        diff -= overhead; // overhead of measurement
        diff /= 2; // 2x jump_to c1->c2 && c2->c1

        BOOST_ASSERT( diff >= 0);
        total += diff;
    }
    return total/iterations;
}
#endif

unsigned int test_fcontext( unsigned int iterations)
{
    cycle_t total( 0);
    cycle_t overhead( get_overhead() );
    std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

    // cache warum-up
    {
        ctx::stack_allocator alloc;

        fc.fc_stack.base = alloc.allocate(ctx::default_stacksize());
        fc.fc_stack.limit =
            static_cast< char * >( fc.fc_stack.base) - ctx::default_stacksize();
		ctx::make_fcontext( & fc, f1, 0);
        ctx::start_fcontext( & fcm, & fc);
        ctx::jump_fcontext( & fcm, & fc, 0);
    }

    for ( unsigned int i = 0; i < iterations; ++i)
    {
        cycle_t start( get_cycles() );
        ctx::jump_fcontext( & fcm, & fc, 0);
        cycle_t diff( get_cycles() - start);

        // we have two jumps and two measuremt-overheads
        diff -= overhead; // overhead of measurement
        diff /= 2; // 2x jump_to c1->c2 && c2->c1

        BOOST_ASSERT( diff >= 0);
        total += diff;
    }
    return total/iterations;
}

int main( int argc, char * argv[])
{
    try
    {
        unsigned int iterations( 0);

        po::options_description desc("allowed options");
        desc.add_options()
            ("help,h", "help message")
            ("iterations,i", po::value< unsigned int >( & iterations), "iterations");

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

        if ( 0 >= iterations) throw std::invalid_argument("iterations must be greater than zero");

        bind_to_processor( 0);

        unsigned int res = test_fcontext( iterations);
        std::cout << "fcontext: average of " << res << " cycles per switch" << std::endl;
#ifndef BOOST_WINDOWS
        res = test_ucontext( iterations);
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
