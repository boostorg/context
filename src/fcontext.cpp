
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_CONTEXT_SOURCE

#include <boost/context/fcontext.hpp>

#include <cstddef>

#include <boost/cstdint.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace contexts {

extern "C" BOOST_CONTEXT_DECL void * BOOST_CONTEXT_CALLDECL boost_fcontext_align( void * vp)
{
	void * base = vp;
    if ( 0 != ( ( ( uintptr_t) base) & 15) )
        base = ( char * )(
            ( ( ( ( uintptr_t) base) - 16) >> 4) << 4);
	return base;
}

# if !defined(__arm__) && !defined(__powerpc__)
extern "C" BOOST_CONTEXT_DECL intptr_t BOOST_CONTEXT_CALLDECL boost_fcontext_start( boost_fcontext_t * ofc, boost_fcontext_t const* nfc)
{ return boost_fcontext_jump( ofc, nfc, 0); }
#endif

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif
