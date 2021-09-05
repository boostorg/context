
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FIBER_H
#define BOOST_CONTEXT_FIBER_H

#include <boost/context/detail/config.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <ostream>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/move/move.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/explicit_operator_bool.hpp>

#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
#include <boost/context/detail/exchange.hpp>
#endif
#if defined(BOOST_NO_CXX17_STD_INVOKE)
#include <boost/context/detail/invoke.hpp>
#endif
#include <boost/context/detail/disable_overload.hpp>
#include <boost/context/detail/exception.hpp>
#include <boost/context/detail/fcontext.hpp>
#include <boost/context/fixedsize_stack.hpp>
#include <boost/context/flags.hpp>
#include <boost/context/preallocated.hpp>
#include <boost/context/segmented_stack.hpp>
#include <boost/context/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4702)
#endif

namespace boost {
namespace context {
namespace detail {

inline
transfer_t fiber_unwind( transfer_t t) {
    throw forced_unwind( t.fctx);
    return transfer_t( BOOST_CONTEXT_NULLPTR, BOOST_CONTEXT_NULLPTR);
}

template< typename Rec >
transfer_t fiber_exit( transfer_t t) BOOST_NOEXCEPT_OR_NOTHROW {
    Rec * rec = static_cast< Rec * >( t.data);
    // destroy context stack
    rec->deallocate();
    return transfer_t( BOOST_CONTEXT_NULLPTR, BOOST_CONTEXT_NULLPTR);
}

template< typename Rec >
void fiber_entry( transfer_t t) BOOST_NOEXCEPT_OR_NOTHROW {
    // transfer control structure to the context-stack
    Rec * rec = static_cast< Rec * >( t.data);
    BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != t.fctx);
    BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != rec);
    try {
        // jump back to `create_context()`
        t = jump_fcontext( t.fctx, BOOST_CONTEXT_NULLPTR);
        // start executing
        t.fctx = rec->run( t.fctx);
    } catch ( forced_unwind const& ex) {
        t = transfer_t( ex.fctx, BOOST_CONTEXT_NULLPTR);
    }
    BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != t.fctx);
    // destroy context-stack of `this`context on next context
    ontop_fcontext( t.fctx, rec, fiber_exit< Rec >);
    BOOST_ASSERT_MSG( false, "context already terminated");
}

template< typename Ctx, typename Fn >
transfer_t fiber_ontop( transfer_t t) {
    BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != t.data);
    auto p = *static_cast< Fn * >( t.data);
    t.data = BOOST_CONTEXT_NULLPTR;
    // execute function, pass fiber via reference
    Ctx c = p( Ctx( t.fctx) );
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
    return transfer_t( exchange( c.fctx_, BOOST_CONTEXT_NULLPTR), BOOST_CONTEXT_NULLPTR);
#else
    return transfer_t( std::exchange( c.fctx_, BOOST_CONTEXT_NULLPTR), BOOST_CONTEXT_NULLPTR);
#endif
}

template< typename Ctx, typename StackAlloc, typename Fn >
class fiber_record {
private:
    stack_context                                       sctx_;
    typename boost::decay< StackAlloc >::type           salloc_;
    typename boost::decay< Fn >::type                   fn_;

    static void destroy( fiber_record * p) BOOST_NOEXCEPT_OR_NOTHROW {
        typename boost::decay< StackAlloc >::type salloc = boost::move( p->salloc_);
        stack_context sctx = p->sctx_;
        // deallocate fiber_record
        p->~fiber_record();
        // destroy stack with stack allocator
        salloc.deallocate( sctx);
    }

public:
    fiber_record( stack_context sctx, BOOST_RV_REF( StackAlloc) salloc,
            BOOST_RV_REF( Fn) fn) BOOST_NOEXCEPT_OR_NOTHROW :
        sctx_( sctx),
        salloc_( boost::forward< StackAlloc >( salloc) ),
        fn_( boost::forward< Fn >( fn) ) {
    }

    BOOST_DELETED_FUNCTION( fiber_record( fiber_record const&) );
    BOOST_DELETED_FUNCTION( fiber_record & operator=( fiber_record const&) );

    void deallocate() BOOST_NOEXCEPT_OR_NOTHROW {
        destroy( this);
    }

    fcontext_t run( fcontext_t fctx) {
        // invoke context-function
#if defined(BOOST_NO_CXX17_STD_INVOKE)
        Ctx c = boost::context::detail::invoke( fn_, Ctx( fctx) );
#else
        Ctx c = std::invoke( fn_, Ctx{ fctx } );
#endif
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return exchange( c.fctx_, BOOST_CONTEXT_NULLPTR);
#else
        return std::exchange( c.fctx_, BOOST_CONTEXT_NULLPTR);
#endif
    }
};

template< typename Record, typename StackAlloc, typename Fn >
fcontext_t create_fiber1( BOOST_RV_REF( StackAlloc) salloc, BOOST_RV_REF( Fn) fn) {
    auto sctx = salloc.allocate();
    // reserve space for control structure
	void * storage = reinterpret_cast< void * >(
		( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( Record) ) )
        & ~static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    Record * record = new ( storage) Record(
            sctx, boost::forward< StackAlloc >( salloc), boost::forward< Fn >( fn) );
    // 64byte gab between control structure and stack top
    // should be 16byte aligned
    void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( storage) - static_cast< uintptr_t >( 64) );
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sctx.size) );
    // create fast-context
    const std::size_t size = reinterpret_cast< uintptr_t >( stack_top) -
        reinterpret_cast< uintptr_t >( stack_bottom);
    const fcontext_t fctx = make_fcontext( stack_top, size, & fiber_entry< Record >);
    BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != fctx);
    // transfer control structure to context-stack
    return jump_fcontext( fctx, record).fctx;
}

template< typename Record, typename StackAlloc, typename Fn >
fcontext_t create_fiber2( preallocated palloc, BOOST_RV_REF( StackAlloc) salloc,
        BOOST_RV_REF( Fn) fn) {
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
        ( reinterpret_cast< uintptr_t >( palloc.sp) - static_cast< uintptr_t >( sizeof( Record) ) )
        & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context-stack
    Record * record = new ( storage) Record(
            palloc.sctx, boost::forward< StackAlloc >( salloc), boost::forward< Fn >( fn) );
    // 64byte gab between control structure and stack top
    void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( storage) - static_cast< uintptr_t >( 64) );
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( palloc.sctx.sp) -
            static_cast< uintptr_t >( palloc.sctx.size) );
    // create fast-context
    const std::size_t size = reinterpret_cast< uintptr_t >( stack_top) -
        reinterpret_cast< uintptr_t >( stack_bottom);
    const fcontext_t fctx = make_fcontext( stack_top, size, & fiber_entry< Record >);
    BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != fctx);
    // transfer control structure to context-stack
    return jump_fcontext( fctx, record).fctx;
}

}

class fiber {
private:
    template< typename Ctx, typename StackAlloc, typename Fn >
    friend class detail::fiber_record;

    template< typename Ctx, typename Fn >
    friend detail::transfer_t
    detail::fiber_ontop( detail::transfer_t);

    detail::fcontext_t  fctx_;

    fiber( detail::fcontext_t fctx) BOOST_NOEXCEPT_OR_NOTHROW :
        fctx_( fctx) {
    }

public:
    fiber() BOOST_NOEXCEPT_OR_NOTHROW :
        fctx_( BOOST_CONTEXT_NULLPTR) {
    }

    template< typename Fn, typename = detail::disable_overload< fiber, Fn > >
    fiber( BOOST_RV_REF( Fn) fn) :
        fctx_( detail::create_fiber1< detail::fiber_record< fiber, fixedsize_stack, Fn > >(
                fixedsize_stack(), boost::forward< Fn >( fn) ) ) {
    }

    template< typename StackAlloc, typename Fn >
    fiber( std::allocator_arg_t, BOOST_RV_REF( StackAlloc) salloc, BOOST_RV_REF( Fn) fn) :
        fctx_( detail::create_fiber1< detail::fiber_record< fiber, StackAlloc, Fn > >(
                boost::forward< StackAlloc >( salloc), boost::forward< Fn >( fn) ) ) {
    }

    template< typename StackAlloc, typename Fn >
    fiber( std::allocator_arg_t, preallocated palloc, BOOST_RV_REF( StackAlloc) salloc,
            BOOST_RV_REF( Fn) fn) :
        fctx_( detail::create_fiber2< detail::fiber_record< fiber, StackAlloc, Fn > >(
                palloc, boost::forward< StackAlloc >( salloc), boost::forward< Fn >( fn) ) ) {
    }

    ~fiber() {
        if ( BOOST_UNLIKELY( BOOST_CONTEXT_NULLPTR != fctx_) ) {
            detail::ontop_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, BOOST_CONTEXT_NULLPTR),
#else
                    std::exchange( fctx_, BOOST_CONTEXT_NULLPTR),
#endif
                   BOOST_CONTEXT_NULLPTR,
                   detail::fiber_unwind);
        }
    }

    fiber( BOOST_RV_REF( fiber) other) BOOST_NOEXCEPT_OR_NOTHROW :
        fctx_( BOOST_CONTEXT_NULLPTR) {
        swap( other);
    }

    fiber & operator=( BOOST_RV_REF( fiber) other) BOOST_NOEXCEPT_OR_NOTHROW {
        if ( BOOST_LIKELY( this != & other) ) {
            fiber tmp = boost::move( other);
            swap( tmp);
        }
        return * this;
    }

    BOOST_DELETED_FUNCTION( fiber( fiber const& other) BOOST_NOEXCEPT_OR_NOTHROW);
    BOOST_DELETED_FUNCTION( fiber & operator=( fiber const& other) BOOST_NOEXCEPT_OR_NOTHROW);

    fiber resume()
#if ! defined(BOOST_NO_CXX11_REF_QUALIFIERS)
    &&
#endif
    {
        BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != fctx_);
        return fiber( detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, BOOST_CONTEXT_NULLPTR),
#else
                    std::exchange( fctx_, BOOST_CONTEXT_NULLPTR),
#endif
                    BOOST_CONTEXT_NULLPTR).fctx);
    }

    template< typename Fn >
    fiber resume_with( BOOST_RV_REF( Fn) fn)
#if ! defined(BOOST_NO_CXX11_REF_QUALIFIERS)
    &&
#endif
    {
        BOOST_ASSERT( BOOST_CONTEXT_NULLPTR != fctx_);
        auto p = boost::forward< Fn >( fn);
        return fiber( detail::ontop_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, BOOST_CONTEXT_NULLPTR),
#else
                    std::exchange( fctx_, BOOST_CONTEXT_NULLPTR),
#endif
                    & p,
                    detail::fiber_ontop< fiber, decltype(p) >).fctx); // FIXME: decltype
    }

    BOOST_EXPLICIT_OPERATOR_BOOL();

    bool operator!() const BOOST_NOEXCEPT_OR_NOTHROW {
        return BOOST_CONTEXT_NULLPTR == fctx_;
    }

    bool operator<( fiber const& other) const BOOST_NOEXCEPT_OR_NOTHROW {
        return fctx_ < other.fctx_;
    }

    #if !defined(BOOST_EMBTC)
    
    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber const& other) {
        if ( BOOST_CONTEXT_NULLPTR != other.fctx_) {
            return os << other.fctx_;
        } else {
            return os << "{not-a-context}";
        }
    }

    #else
    
    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber const& other);

    #endif

    void swap( fiber & other) BOOST_NOEXCEPT_OR_NOTHROW {
        std::swap( fctx_, other.fctx_);
    }
};

#if defined(BOOST_EMBTC)

    template< typename charT, class traitsT >
    inline std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber const& other) {
        if ( BOOST_CONTEXT_NULLPTR != other.fctx_) {
            return os << other.fctx_;
        } else {
            return os << "{not-a-context}";
        }
    }

#endif
    
inline
void swap( fiber & l, fiber & r) BOOST_NOEXCEPT_OR_NOTHROW {
    l.swap( r);
}

typedef fiber fiber_context;

}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_FIBER_H
