
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef PERFORMANCE_GCC_X86_64_H
#define PERFORMANCE_GCC_X86_64_H

#include <algorithm>
#include <numeric>
#include <cstddef>
#include <vector>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>

typedef boost::uint64_t cycle_t;

inline
cycle_t get_cycles()
{
#if defined(__INTEL_COMPILER) || defined(__ICC) || defined(_ECC) || defined(__ICL)
    return __rdtsc();
#else
    boost::uint32_t res[2];
    
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"
        ::: "%rax", "%rbx", "%rcx", "%rdx"
    );
    __asm__ __volatile__ ("rdtsc" : "=a" (res[0]), "=d" (res[1]) );
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"
        ::: "%rax", "%rbx", "%rcx", "%rdx"
    );
    
    return * ( cycle_t *)res;
#endif
}

struct measure
{
    cycle_t operator()()
    {
        cycle_t start( get_cycles() );
        return get_cycles() - start;
    }
};

inline
cycle_t get_overhead()
{
    std::size_t iterations( 10);
    std::vector< cycle_t >  overhead( iterations, 0);
    for ( std::size_t i( 0); i < iterations; ++i)
        std::generate(
            overhead.begin(), overhead.end(),
            measure() );
    BOOST_ASSERT( overhead.begin() != overhead.end() );
    return std::accumulate( overhead.begin(), overhead.end(), 0) / iterations;
}

#endif // PERFORMANCE_GCC_X86_64_H
