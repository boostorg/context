
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXTS_FCONTEXT_H
#define BOOST_CONTEXTS_FCONTEXT_H

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

// Windows
#if defined(BOOST_WINDOWS)
// i386
# if defined(_WIN32) && ! defined(_WIN64)
#  include <boost/context/detail/fcontext_i386_win.hpp>
// x86_64
# elif defined(_WIN32) && defined(_WIN64)
#  include <boost/context/detail/fcontext_x86_64_win.hpp>
# else
#  error "platform not supported"
# endif
// POSIX
#else
// i386
# if defined(__i386__)
#  include <boost/context/detail/fcontext_i386.hpp>
// x86_64
# elif defined(__x86_64__)
#  include <boost/context/detail/fcontext_x86_64.hpp>
// arm
# elif defined(__arm__)
#  include <boost/context/detail/fcontext_arm.hpp>
// mips
# elif defined(__mips__)
#  include <boost/context/detail/fcontext_mips.hpp>
// powerpc
# elif defined(__powerpc__)
#  include <boost/context/detail/fcontext_ppc.hpp>
# else
#  error "platform not supported"
# endif
#endif

extern "C" {

BOOST_CONTEXT_DECL void * BOOST_CONTEXT_CALLDECL boost_fcontext_align( void * vp);
BOOST_CONTEXT_DECL intptr_t BOOST_CONTEXT_CALLDECL boost_fcontext_start( boost_fcontext_t * ofc, boost_fcontext_t const* nfc);
BOOST_CONTEXT_DECL intptr_t BOOST_CONTEXT_CALLDECL boost_fcontext_jump( boost_fcontext_t * ofc, boost_fcontext_t const* nfc, intptr_t vp);
BOOST_CONTEXT_DECL void BOOST_CONTEXT_CALLDECL boost_fcontext_make( boost_fcontext_t * fc, void (* fn)( intptr_t), intptr_t vp);

}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXTS_FCONTEXT_H

