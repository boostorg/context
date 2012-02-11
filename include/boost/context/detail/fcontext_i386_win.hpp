
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXTS_DETAIL_FCONTEXT_I386H
#define BOOST_CONTEXTS_DETAIL_FCONTEXT_I386H

#include <boost/config.hpp>
#include <boost/cstdint.hpp>

#include <boost/context/detail/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

extern "C" {

#define BOOST_CONTEXT_CALLDECL __cdecl

typedef struct boost_fcontext_stack    boost_fcontext_stack_t;
struct boost_fcontext_stack
{
    void    *   ss_base;
    void    *   ss_limit;
};

typedef struct boost_fcontext boost_fcontext_t;
struct boost_fcontext
{
    boost::uint32_t         fc_greg[6];
    boost::uint32_t         fc_freg[2];
    boost_fcontext_stack_t  fc_stack;
    boost_fcontext_t     *  fc_link;
    void                 *  fc_excpt_lst;
    void                 *  fc_local_storage;
};

}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXTS_DETAIL_FCONTEXT_I386_H
