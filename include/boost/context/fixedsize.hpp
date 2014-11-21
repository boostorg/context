
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FIXEDSIZE_H
#define BOOST_CONTEXT_FIXEDSIZE_H

#include <cstddef>
#include <cstdlib>
#include <new>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/context/stack_traits.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {

template< typename traitsT >
class basic_fixedsize {
private:
    std::size_t     size_;

public:
    typedef traitsT traits_type;

    basic_fixedsize( std::size_t size = traits_type::default_size() ) :
        size_( size) {
        BOOST_ASSERT( traits_type::minimum_size() <= size_);
        BOOST_ASSERT( traits_type::is_unbounded() || ( traits_type::maximum_size() >= size_) );
    }

    stack_context allocate() {
        void * vp = std::malloc( size_);
        if ( ! vp) throw std::bad_alloc();

        stack_context sctx;
        sctx.size = size_;
        sctx.sp = static_cast< char * >( vp) + sctx.size;
        return sctx;
    }

    void deallocate( stack_context & sctx) {
        BOOST_ASSERT( sctx.sp);
        BOOST_ASSERT( traits_type::minimum_size() <= sctx.size);
        BOOST_ASSERT( traits_type::is_unbounded() || ( traits_type::maximum_size() >= sctx.size) );

        void * vp = static_cast< char * >( sctx.sp) - sctx.size;
        std::free( vp);
    }
};

typedef basic_fixedsize< stack_traits >  fixedsize;

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_FIXEDSIZE_H
