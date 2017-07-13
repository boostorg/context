
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_EXCEPTION_H
#define BOOST_CONTEXT_DETAIL_EXCEPTION_H

#include <boost/config.hpp>

#if defined(BOOST_USE_UCONTEXT) || defined(BOOST_USE_WINFIB)
namespace boost {
namespace context {
namespace detail {
struct activation_record;
}}}
#else
#include <boost/context/detail/fcontext.hpp>
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {
namespace detail {

struct forced_unwind {

#if defined(BOOST_USE_UCONTEXT) || defined(BOOST_USE_WINFIB)
    activation_record  *   from{ nullptr };
    forced_unwind( activation_record * from_) noexcept :
        from{ from_ } {
    }
#else
    fcontext_t  fctx{ nullptr };
    forced_unwind( fcontext_t fctx_) noexcept :
        fctx( fctx_ ) {
    }
#endif
    bool        caught{ false };

    forced_unwind() = default;

    ~forced_unwind() {
        BOOST_ASSERT( caught );
    }
    
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_DETAIL_EXCEPTION_H
