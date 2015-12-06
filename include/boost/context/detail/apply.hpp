
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_DETAIL_APPLY_H
#define BOOST_CONTEXT_DETAIL_APPLY_H

#include <functional>
#include <type_traits>
#include <utility>

#include <boost/config.hpp>

#include <boost/context/detail/config.hpp>
#include <boost/context/detail/invoke.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {
namespace detail {

template< typename Fn, typename Tpl, std::size_t... I >
decltype( auto) apply_impl( Fn && fn, Tpl && tpl, std::index_sequence< I ... >) {
    return invoke( fn,
                   std::forward< decltype( std::get< I >( std::declval< std::decay_t< Tpl > >() ) ) >(
                       std::get< I >( std::forward< std::decay_t< Tpl > >( tpl) ) ) ... );
}


template< typename Fn, typename Tpl >
decltype( auto) apply( Fn && fn, Tpl && tpl) {
    constexpr auto Size = std::tuple_size< std::decay_t< Tpl > >::value;
    return apply_impl( std::forward< Fn >( fn),
                       std::forward< Tpl >( tpl),
                       std::make_index_sequence< Size >{});
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_DETAIL_APPLY_H
