
//          Copyright Casey Brodley 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_ALLOCATOR_H
#define BOOST_CONTEXT_DETAIL_ALLOCATOR_H

#include <type_traits>

#include <boost/type_traits/make_void.hpp>

#include <boost/context/stack_context.hpp>

namespace boost {
namespace context {
namespace detail {

template< typename T, typename = void >
struct is_stack_allocator : public std::false_type {
};

template< typename T >
struct is_stack_allocator<
    T,
    boost::void_t<
        decltype(
            // boost::context::stack_context c = salloc.allocate();
            std::declval<boost::context::stack_context>() = std::declval<T&>().allocate(),
            // salloc.deallocate(c);
            std::declval<T&>().deallocate(std::declval<boost::context::stack_context&>())
        )
    >
> : public std::true_type {
};

}}}

#endif // BOOST_CONTEXT_DETAIL_ALLOCATOR_H
