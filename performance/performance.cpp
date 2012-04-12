
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
#include <boost/context/all.hpp>
#include <boost/program_options.hpp>

#include "bind_processor.hpp"
#include "performance.hpp"

namespace ctx = boost::ctx;
namespace po = boost::program_options;

ctx::fcontext_t fc, fcm;

void fn( intptr_t param)
{
    while ( param)
        ctx::jump_fcontext( & fc, & fcm, 0);
}

void test( unsigned int iterations)
{
    cycle_t total( 0);
    cycle_t overhead( get_overhead() );
    std::cout << "overhead for rdtsc == " << overhead << " cycles" << std::endl;

    // cache warum-up
    {
        ctx::stack_allocator alloc;

        fc.fc_stack.base = alloc.allocate(ctx::minimum_stacksize());
        fc.fc_stack.limit =
            static_cast< char * >( fc.fc_stack.base) - ctx::minimum_stacksize();
		ctx::make_fcontext( & fc, fn, 1);
        ctx::start_fcontext( & fcm, & fc);
        ctx::jump_fcontext( & fcm, & fc, 1);
    }

    for ( unsigned int i = 0; i < iterations; ++i)
    {
        cycle_t start( get_cycles() );
        ctx::jump_fcontext( & fcm, & fc, 1);
        cycle_t diff( get_cycles() - start);

        // we have two jumps and two measuremt-overheads
        diff -= overhead; // overhead of measurement
        diff /= 2; // 2x jump_to c1->c2 && c2->c1

        BOOST_ASSERT( diff >= 0);
        total += diff;
    }
    std::cout << "average of " << total/iterations << " cycles per switch" << std::endl;
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

        test( iterations);

        return EXIT_SUCCESS;
    }
    catch ( std::exception const& e)
    { std::cerr << "exception: " << e.what() << std::endl; }
    catch (...)
    { std::cerr << "unhandled exception" << std::endl; }
    return EXIT_FAILURE;
}
