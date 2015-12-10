
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
#if defined(BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE)
#include <boost/context/detail/index_sequence.hpp>
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace context {
namespace detail {

#if defined(BOOST_CONTEXT_NO_CXX14_INTEGER_SEQUENCE)
template< std::size_t ... I >
using _index_sequence = index_sequence< I ... >;
template< std::size_t I >
using _make_index_sequence = make_index_sequence< I >;
#else
template< std::size_t ... I >
using _index_sequence = std::index_sequence< I ... >;
template< std::size_t I >
using _make_index_sequence = std::make_index_sequence< I >;
#endif

template< typename Fn, typename Tpl, std::size_t ... I >
#if defined( BOOST_NO_CXX14_DECLTYPE_AUTO)
auto
#else
decltype(auto)
#endif
apply_impl( Fn && fn, Tpl && tpl, _index_sequence< I ... >) 
#if defined( BOOST_NO_CXX14_DECLTYPE_AUTO)
    -> decltype( invoke( fn,
                         std::forward< decltype( std::get< I >( std::declval< typename std::decay< Tpl >::type >() ) ) >(
                           std::get< I >( std::forward< typename std::decay< Tpl >::type >( tpl) ) ) ... ) )
#endif
    {
    return invoke( fn,
                   std::forward< decltype( std::get< I >( std::declval< typename std::decay< Tpl >::type >() ) ) >(
                       std::get< I >( std::forward< typename std::decay< Tpl >::type >( tpl) ) ) ... );
}


template< typename Fn, typename Tpl >
#if defined( BOOST_NO_CXX14_DECLTYPE_AUTO)
auto
#else
decltype(auto)
#endif
apply( Fn && fn, Tpl && tpl) 
#if defined( BOOST_NO_CXX14_DECLTYPE_AUTO)
    -> decltype( apply_impl( std::forward< Fn >( fn),
                 std::forward< Tpl >( tpl),
                 _make_index_sequence< std::tuple_size< typename std::decay< Tpl >::type >::value >{}) )
#endif
    {
    constexpr auto Size = std::tuple_size< typename std::decay< Tpl >::type >::value;
    return apply_impl( std::forward< Fn >( fn),
                       std::forward< Tpl >( tpl),
                       _make_index_sequence< Size >{});
}

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_DETAIL_APPLY_H
