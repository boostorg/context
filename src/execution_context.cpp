
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/context/detail/config.hpp>

#if ! defined(BOOST_CONTEXT_NO_EXECUTION_CONTEXT)

# include "boost/context/execution_context.hpp"

# include <boost/config.hpp>
# include <boost/thread/tss.hpp>

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
# endif

namespace boost {
namespace context {

static detail::activation_record * main_rec() {
    static boost::thread_specific_ptr<detail::activation_record> rec;
    if (!rec.get()) rec.reset(new detail::activation_record);
    return rec.get();
}

detail::activation_record::ptr_t
detail::activation_record::current_rec = main_rec();

}}

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
# endif

#endif
