
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXTS_DETAIL_CONTEXT_OBJECT_H
#define BOOST_CONTEXTS_DETAIL_CONTEXT_OBJECT_H

#include <cstddef>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/utility/base_from_member.hpp>

#include <boost/context/detail/config.hpp>
#include <boost/context/detail/context_base.hpp>
#include <boost/context/flags.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace contexts {
namespace detail {

template< typename Fn, typename Allocator >
class context_object : private base_from_member< Fn >,
                       private base_from_member< Allocator >,
                       public context_base
{
private:
	typedef base_from_member< Fn >			fn_t;
	typedef base_from_member< Allocator >	alloc_t;

    context_object( context_object &);
    context_object & operator=( context_object const&);

public:
#ifndef BOOST_NO_RVALUE_REFERENCES
    context_object( Fn & fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, flag_return_t do_return) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, do_return)
    {}

    context_object( Fn & fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, typename context_base::ptr_t nxt) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, nxt)
    {}

    context_object( Fn && fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, flag_return_t do_return) :
        fn_t( static_cast< Fn && >( fn) ), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, do_return)
    {}

    context_object( Fn && fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, typename context_base::ptr_t nxt) :
        fn_t( static_cast< Fn && >( fn) ), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, nxt)
    {}
#else
    context_object( Fn fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, flag_return_t do_return) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, do_return)
    {}

    context_object( Fn fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, typename context_base::ptr_t nxt) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, nxt)
    {}

    context_object( BOOST_RV_REF( Fn) fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, flag_return_t do_return) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, do_return)
    {}

    context_object( BOOST_RV_REF( Fn) fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, typename context_base::ptr_t nxt) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, nxt)
    {}
#endif

	~context_object()
	{ cleanup( alloc_t::member); }

    void exec()
    { fn_t::member(); }
};

template< typename Fn, typename Allocator >
class context_object< reference_wrapper< Fn >, Allocator > : private base_from_member< Fn & >,
                                                             private base_from_member< Allocator >,
                                                             public context_base
{
private:
	typedef base_from_member< Fn & >		fn_t;
	typedef base_from_member< Allocator >	alloc_t;

    context_object( context_object &);
    context_object & operator=( context_object const&);

public:
    context_object( reference_wrapper< Fn > fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, flag_return_t do_return) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, do_return)
    {}

    context_object( reference_wrapper< Fn > fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, typename context_base::ptr_t nxt) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, nxt)
    {}

	~context_object()
	{ cleanup( alloc_t::member); }

    void exec()
    { fn_t::member(); }
};

template< typename Fn, typename Allocator >
class context_object< const reference_wrapper< Fn >, Allocator > : private base_from_member< Fn & >,
                                                                   private base_from_member< Allocator >,
                                                                   public context_base
{
private:
	typedef base_from_member< Fn & >		fn_t;
	typedef base_from_member< Allocator >	alloc_t;

    context_object( context_object &);
    context_object & operator=( context_object const&);

public:
    context_object( const reference_wrapper< Fn > fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, flag_return_t do_return) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, do_return)
    {}

    context_object( const reference_wrapper< Fn > fn, Allocator const& alloc, std::size_t size, flag_unwind_t do_unwind, typename context_base::ptr_t nxt) :
        fn_t( fn), alloc_t( alloc),
        context_base( alloc_t::member, size, do_unwind, nxt)
    {}

	~context_object()
	{ cleanup( alloc_t::member); }

    void exec()
    { fn_t::member(); }
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXTS_DETAIL_CONTEXT_OBJECT_H
