
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/context/detail/config.hpp>

#if ! defined(BOOST_CONTEXT_NO_EXECUTION_CONTEXT)

# include "boost/context/execution_context.hpp"

# include <boost/config.hpp>

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
# endif

namespace boost {
namespace context {

static detail::activation_record * main_rec() {
    thread_local static detail::activation_record rec;
    return & rec;
}

thread_local
detail::activation_record::ptr_t
detail::activation_record::current_rec = main_rec();

}}

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
# endif

#endif
