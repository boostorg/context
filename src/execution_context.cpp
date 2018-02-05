
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "boost/context/execution_context.hpp"

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {
namespace detail {

#if !defined(BOOST_NO_CXX11_THREAD_LOCAL)
thread_local
ecv1_activation_record::ptr_t
ecv1_activation_record::current_rec;

// zero-initialization
thread_local static std::size_t counter;

// schwarz counter
ecv1_activation_record_initializer::ecv1_activation_record_initializer() noexcept {
    if ( 0 == counter++) {
        ecv1_activation_record::current_rec.reset( new ecv1_activation_record() );
    }
}

ecv1_activation_record_initializer::~ecv1_activation_record_initializer() {
    if ( 0 == --counter) {
        BOOST_ASSERT( ecv1_activation_record::current_rec->is_main_context() );
        delete ecv1_activation_record::current_rec.detach();
    }
}

}

namespace v1 {

execution_context
execution_context::current() noexcept {
    // initialized the first time control passes; per thread
    thread_local static detail::ecv1_activation_record_initializer initializer;
    return execution_context();
}

}

#endif

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif
