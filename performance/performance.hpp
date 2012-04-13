
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef PERFORMANCE_H
#define PERFORMANCE_H

// x86_64
// test x86_64 before i386 because icc might
// define __i686__ for x86_64 too
#if defined(__x86_64__) || defined(__x86_64) \
    || defined(__amd64__) || defined(__amd64) \
    || defined(_M_X64) || defined(_M_AMD64)
# if defined(BOOST_WINDOWS)
#  error "this platform is not supported"
# else
#  include "performance_gcc_x86-64.hpp"
# endif
// i386
#elif defined(i386) || defined(__i386__) || defined(__i386) \
    || defined(__i486__) || defined(__i586__) || defined(__i686__) \
    || defined(__X86__) || defined(_X86_) || defined(__THW_INTEL__) \
    || defined(__I86__) || defined(__INTEL__) || defined(__IA32__) \
    || defined(_M_IX86) || defined(_I86_)
# if defined(BOOST_WINDOWS)
#  include "performance_msvc_i386.hpp"
# else
#  include "performance_gcc_i386.hpp"
# endif

#else
#error "this platform is not supported"
#endif

#endif // PERFORMANCE_H
