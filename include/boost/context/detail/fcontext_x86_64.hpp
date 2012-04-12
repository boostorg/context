
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CTX_DETAIL_FCONTEXT_X86_64_H
#define BOOST_CTX_DETAIL_FCONTEXT_X86_64_H

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace ctx {

extern "C" {

#define BOOST_CONTEXT_CALLDECL

struct stack_t
{
    void    *   base;
    void    *   limit;
};

struct fcontext_t
{
    boost::uint64_t     fc_greg[8];
    boost::uint32_t     fc_freg[2];
    stack_t				fc_stack;
    fcontext_t		*	fc_link;
};

}

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CTX_DETAIL_FCONTEXT_X86_64_H
