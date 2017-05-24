
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#if defined(BOOST_USE_UCONTEXT)
#include "boost/context/continuation_ucontext.hpp"
#elif defined(BOOST_USE_WINFIB)
#include "boost/context/continuation_winfib.hpp"
#else
#include "boost/context/execution_context.hpp"
#endif

#include <boost/config.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

#if defined(BOOST_USE_UCONTEXT) || \
    defined(BOOST_USE_WINFIB) || \
    (defined(BOOST_EXECUTION_CONTEXT) && (BOOST_EXECUTION_CONTEXT == 1))
namespace boost {
namespace context {
namespace detail {

thread_local
#if defined(BOOST_USE_UCONTEXT) || defined(BOOST_USE_WINFIB)
activation_record *
#elif defined(BOOST_EXECUTION_CONTEXT) && (BOOST_EXECUTION_CONTEXT == 1)
activation_record::ptr_t
#endif
activation_record::current_rec;

// zero-initialization
thread_local static std::size_t counter;

// schwarz counter
activation_record_initializer::activation_record_initializer() noexcept {
    if ( 0 == counter++) {
#if defined(BOOST_USE_UCONTEXT) || defined(BOOST_USE_WINFIB)
        activation_record::current_rec = new activation_record();
#elif defined(BOOST_EXECUTION_CONTEXT) && (BOOST_EXECUTION_CONTEXT == 1)
        activation_record::current_rec.reset( new activation_record() );
#endif
    }
}

activation_record_initializer::~activation_record_initializer() {
    if ( 0 == --counter) {
        BOOST_ASSERT( activation_record::current_rec->is_main_context() );
#if defined(BOOST_USE_UCONTEXT) || defined(BOOST_USE_WINFIB)
        delete activation_record::current_rec;
#elif defined(BOOST_EXECUTION_CONTEXT) && (BOOST_EXECUTION_CONTEXT == 1)
        delete activation_record::current_rec.detach();
#endif
    }
}

}

#if defined(BOOST_USE_UCONTEXT) || defined(BOOST_USE_WINFIB)
namespace detail {

activation_record *&
activation_record::current() noexcept {
    // initialized the first time control passes; per thread
    thread_local static activation_record_initializer initializer;
    return activation_record::current_rec;
}

}
#elif defined(BOOST_EXECUTION_CONTEXT) && (BOOST_EXECUTION_CONTEXT == 1)
execution_context
execution_context::current() noexcept {
    // initialized the first time control passes; per thread
    thread_local static detail::activation_record_initializer initializer;
    return execution_context();
}
#endif

}}
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif
