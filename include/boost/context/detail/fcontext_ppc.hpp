
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXTS_DETAIL_FCONTEXT_PPC_H
#define BOOST_CONTEXTS_DETAIL_FCONTEXT_PPC_H

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

extern "C" {

#define BOOST_CONTEXT_CALLDECL

typedef struct boost_fcontext_stack    boost_fcontext_stack_t;
struct boost_fcontext_stack
{
    void    *   ss_base;
    void    *   ss_limit;
};

typedef struct boost_fcontext boost_fcontext_t;
struct boost_fcontext
{
# if defined(__powerpc64__)
    boost::uint64_t         fc_greg[23];
# else
    boost::uint32_t         fc_greg[23];
# endif
    boost::uint64_t         fc_freg[19];
    boost_fcontext_stack_t  fc_stack;
    boost_fcontext_t     *  fc_link;
};

}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXTS_DETAIL_FCONTEXT_PPC_H
