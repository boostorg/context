
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_RESERVE_H
#define BOOST_CONTEXT_RESERVE_H

#include <cstddef>

#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {

struct BOOST_CONTEXT_DECL reserve
{
    void    **  vp;
    std::size_t size;

    reserve( void ** vp_, std::size_t size_) :
        vp( vp_), size( size_) {
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_RESERVE_H
