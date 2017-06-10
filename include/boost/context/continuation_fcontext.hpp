
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_CONTINUATION_H
#define BOOST_CONTEXT_CONTINUATION_H

#include <boost/context/detail/config.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <ostream>
#include <tuple>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

#if defined(BOOST_NO_CXX17_STD_APPLY)
#include <boost/context/detail/apply.hpp>
#endif
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
#include <boost/context/detail/exchange.hpp>
#endif
#if defined(BOOST_NO_CXX17_STD_INVOKE)
#include <boost/context/detail/invoke.hpp>
#endif
#include <boost/context/detail/disable_overload.hpp>
#include <boost/context/detail/exception.hpp>
#include <boost/context/detail/fcontext.hpp>
#include <boost/context/detail/tuple.hpp>
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

template< typename U >
struct helper {
    template< typename T >
    static T convert( T && t) noexcept {
        return std::forward< T >( t);
    }
};

template< typename U >
struct helper< std::tuple< U > > {
    template< typename T >
    static std::tuple< T > convert( T && t) noexcept {
        return std::make_tuple( std::forward< T >( t) );
    }
};

inline
transfer_t context_unwind( transfer_t t) {
    throw forced_unwind( t.fctx);
    return { nullptr, nullptr };
}

template< typename Rec >
transfer_t context_exit( transfer_t t) noexcept {
    Rec * rec = static_cast< Rec * >( t.data);
    // destroy context stack
    rec->deallocate();
    return { nullptr, nullptr };
}

template< typename Rec >
void context_entry( transfer_t t_) noexcept {
    // transfer control structure to the context-stack
    Rec * rec = static_cast< Rec * >( t_.data);
    BOOST_ASSERT( nullptr != t_.fctx);
    BOOST_ASSERT( nullptr != rec);
    transfer_t t = { nullptr, nullptr };
    try {
        // jump back to `create_context()`
        t = jump_fcontext( t_.fctx, nullptr);
        // start executing
        t = rec->run( t);
    } catch ( forced_unwind const& e) {
        t = { e.fctx, nullptr };
    }
    BOOST_ASSERT( nullptr != t.fctx);
    // destroy context-stack of `this`context on next context
    ontop_fcontext( t.fctx, rec, context_exit< Rec >);
    BOOST_ASSERT_MSG( false, "context already terminated");
}

template<
    typename Ctx,
    typename StackAlloc,
    typename Fn
>
class record {
private:
    stack_context                                       sctx_;
    StackAlloc                                          salloc_;
    typename std::decay< Fn >::type                     fn_;

    static void destroy( record * p) noexcept {
        StackAlloc salloc = p->salloc_;
        stack_context sctx = p->sctx_;
        // deallocate record
        p->~record();
        // destroy stack with stack allocator
        salloc.deallocate( sctx);
    }

public:
    record( stack_context sctx, StackAlloc const& salloc,
            Fn && fn) noexcept :
        sctx_( sctx),
        salloc_( salloc),
        fn_( std::forward< Fn >( fn) ) {
    }

    record( record const&) = delete;
    record & operator=( record const&) = delete;

    void deallocate() noexcept {
        destroy( this);
    }

    transfer_t run( transfer_t t) {
        Ctx from{ t };
        // invoke context-function
#if defined(BOOST_NO_CXX17_STD_INVOKE)
        Ctx cc = invoke( fn_, std::move( from) );
#else
        Ctx cc = std::invoke( fn_, std::move( from) );
#endif
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return { exchange( cc.t_.fctx, nullptr), nullptr };
#else
        return { std::exchange( cc.t_.fctx, nullptr), nullptr };
#endif
    }
};

template< typename Record, typename StackAlloc, typename Fn >
fcontext_t create_context1( StackAlloc salloc, Fn && fn) {
    auto sctx = salloc.allocate();
    // reserve space for control structure
	void * storage = reinterpret_cast< void * >(
			( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( Record) ) )
            & ~static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    Record * record = new ( storage) Record{
            sctx, salloc, std::forward< Fn >( fn) };
    // 64byte gab between control structure and stack top
    // should be 16byte aligned
    void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( storage) - static_cast< uintptr_t >( 64) );
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sctx.size) );
    // create fast-context
    const std::size_t size = reinterpret_cast< uintptr_t >( stack_top) - reinterpret_cast< uintptr_t >( stack_bottom);
    const fcontext_t fctx = make_fcontext( stack_top, size, & context_entry< Record >);
    BOOST_ASSERT( nullptr != fctx);
    // transfer control structure to context-stack
    return jump_fcontext( fctx, record).fctx;
}

template< typename Record, typename StackAlloc, typename Fn >
fcontext_t create_context2( preallocated palloc, StackAlloc salloc, Fn && fn) {
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( palloc.sp) - static_cast< uintptr_t >( sizeof( Record) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context-stack
    Record * record = new ( storage) Record{
            palloc.sctx, salloc, std::forward< Fn >( fn) };
    // 64byte gab between control structure and stack top
    void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( storage) - static_cast< uintptr_t >( 64) );
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( palloc.sctx.sp) - static_cast< uintptr_t >( palloc.sctx.size) );
    // create fast-context
    const std::size_t size = reinterpret_cast< uintptr_t >( stack_top) - reinterpret_cast< uintptr_t >( stack_bottom);
    const fcontext_t fctx = make_fcontext( stack_top, size, & context_entry< Record >);
    BOOST_ASSERT( nullptr != fctx);
    // transfer control structure to context-stack
    return jump_fcontext( fctx, record).fctx;
}

template< typename ... Arg >
struct result_type {
    typedef std::tuple< Arg ... >   type;

    static
    type get( detail::transfer_t & t) {
        auto p = static_cast< std::tuple< Arg ... > * >( t.data);
        return std::move( * p);
    }
};

template< typename Arg >
struct result_type< Arg > {
    typedef Arg     type;

    static
    type get( detail::transfer_t & t) {
        auto p = static_cast< std::tuple< Arg > * >( t.data);
        return std::forward< Arg >( std::get< 0 >( * p) );
    }
};

}

template< typename Ctx, typename Fn, typename Arg >
detail::transfer_t context_ontop( detail::transfer_t t) {
    auto p = static_cast< Arg * >( t.data);
    BOOST_ASSERT( nullptr != p);
    typename std::decay< Fn >::type fn = std::forward< Fn >( std::get< 0 >( * p) );
    t.data = & std::get< 1 >( * p);
    Ctx c{ t };
    // execute function, pass continuation via reference
    typedef typename std::decay< decltype( std::get< 1 >( * p) )>::type Tpl;
    std::get< 1 >( * p) = detail::helper< Tpl >::convert( fn( std::move( c) ) );
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
    return { detail::exchange( c.t_.fctx, nullptr), & std::get< 1 >( * p) };
#else
    return { std::exchange( c.t_.fctx, nullptr), & std::get< 1 >( * p) };
#endif
}

template< typename Ctx, typename Fn >
detail::transfer_t context_ontop_void( detail::transfer_t t) {
    auto p = static_cast< std::tuple< Fn > * >( t.data);
    BOOST_ASSERT( nullptr != p);
    typename std::decay< Fn >::type fn = std::forward< Fn >( std::get< 0 >( * p) );
    Ctx c{ t };
    // execute function, pass continuation via reference
    fn( std::move( c) );
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
    return { detail::exchange( c.t_.fctx, nullptr), nullptr };
#else
    return { std::exchange( c.t_.fctx, nullptr), nullptr };
#endif
}

class continuation {
private:
    template< typename Ctx, typename StackAlloc, typename Fn >
    friend class detail::record;

    template< typename Ctx, typename Fn, typename Arg >
    friend detail::transfer_t
    context_ontop( detail::transfer_t);

    template< typename Ctx, typename Fn >
    friend detail::transfer_t
    context_ontop_void( detail::transfer_t);

    template< typename StackAlloc, typename Fn, typename ... Arg >
    friend continuation
    callcc( std::allocator_arg_t, StackAlloc, Fn &&, Arg ...);

    template< typename StackAlloc, typename Fn, typename ... Arg >
    friend continuation
    callcc( std::allocator_arg_t, preallocated, StackAlloc, Fn &&, Arg ...);

    template< typename StackAlloc, typename Fn >
    friend continuation
    callcc( std::allocator_arg_t, StackAlloc, Fn &&);

    template< typename StackAlloc, typename Fn >
    friend continuation
    callcc( std::allocator_arg_t, preallocated, StackAlloc, Fn &&);

    detail::transfer_t  t_{ nullptr, nullptr };

    continuation( detail::fcontext_t fctx) noexcept :
        t_{ fctx, nullptr } {
    }

    continuation( detail::transfer_t t) noexcept :
        t_{ t.fctx, t.data } {
    }

public:
    continuation() noexcept = default;

    ~continuation() {
        if ( BOOST_UNLIKELY( nullptr != t_.fctx) ) {
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
            detail::ontop_fcontext( detail::exchange( t_.fctx, nullptr), nullptr, detail::context_unwind);
#else
            detail::ontop_fcontext( std::exchange( t_.fctx, nullptr), nullptr, detail::context_unwind);
#endif
        }
    }

    continuation( continuation && other) noexcept :
        t_{ other.t_.fctx, other.t_.data } {
        other.t_ = { nullptr, nullptr };
    }

    continuation & operator=( continuation && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            continuation tmp = std::move( other);
            swap( tmp);
        }
        return * this;
    }

    continuation( continuation const& other) noexcept = delete;
    continuation & operator=( continuation const& other) noexcept = delete;

    template< typename ... Arg >
    continuation resume( Arg ... arg) {
        BOOST_ASSERT( nullptr != t_.fctx);
        auto tpl = std::make_tuple( std::forward< Arg >( arg) ... );
        return detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( t_.fctx, nullptr),
#else
                    std::exchange( t_.fctx, nullptr),
#endif
                    & tpl);
    }

    template< typename Fn, typename ... Arg >
    continuation resume_with( Fn && fn, Arg ... arg) {
        BOOST_ASSERT( nullptr != t_.fctx);
        auto tpl = std::make_tuple( std::forward< Fn >( fn), std::make_tuple( std::forward< Arg >( arg) ... ));
        return detail::ontop_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( t_.fctx, nullptr),
#else
                    std::exchange( t_.fctx, nullptr),
#endif
                    & tpl,
                    context_ontop< continuation, Fn, decltype(tpl) >);
    }

    continuation resume() {
        BOOST_ASSERT( nullptr != t_.fctx);
        return detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( t_.fctx, nullptr),
#else
                    std::exchange( t_.fctx, nullptr),
#endif
                    nullptr);
    }

    template< typename Fn >
    continuation resume_with( Fn && fn) {
        BOOST_ASSERT( nullptr != t_.fctx);
        auto p = std::make_tuple( std::forward< Fn >( fn) );
        return detail::ontop_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( t_.fctx, nullptr),
#else
                    std::exchange( t_.fctx, nullptr),
#endif
                    & p,
                    context_ontop_void< continuation, Fn >);
    }

    bool data_available() noexcept {
        return * this && nullptr != t_.data;
    }

    template< typename ... Arg >
    typename detail::result_type< Arg ... >::type get_data() {
        BOOST_ASSERT( nullptr != t_.data);
        return detail::result_type< Arg ... >::get( t_);
    }

    explicit operator bool() const noexcept {
        return nullptr != t_.fctx;
    }

    bool operator!() const noexcept {
        return nullptr == t_.fctx;
    }

    bool operator==( continuation const& other) const noexcept {
        return t_.fctx == other.t_.fctx;
    }

    bool operator!=( continuation const& other) const noexcept {
        return t_.fctx != other.t_.fctx;
    }

    bool operator<( continuation const& other) const noexcept {
        return t_.fctx < other.t_.fctx;
    }

    bool operator>( continuation const& other) const noexcept {
        return other.t_.fctx < t_.fctx;
    }

    bool operator<=( continuation const& other) const noexcept {
        return ! ( * this > other);
    }

    bool operator>=( continuation const& other) const noexcept {
        return ! ( * this < other);
    }

    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, continuation const& other) {
        if ( nullptr != other.t_.fctx) {
            return os << other.t_.fctx;
        } else {
            return os << "{not-a-context}";
        }
    }

    void swap( continuation & other) noexcept {
        std::swap( t_, other.t_);
    }
};

// Arg
template<
    typename Fn,
    typename ... Arg,
    typename = detail::disable_overload< continuation, Fn >,
    typename = detail::disable_overload< std::allocator_arg_t, Fn >
>
continuation
callcc( Fn && fn, Arg ... arg) {
    return callcc(
            std::allocator_arg, fixedsize_stack(),
            std::forward< Fn >( fn), std::forward< Arg >( arg) ...);
}

template<
    typename StackAlloc,
    typename Fn,
    typename ... Arg
>
continuation
callcc( std::allocator_arg_t, StackAlloc salloc, Fn && fn, Arg ... arg) {
    using Record = detail::record< continuation, StackAlloc, Fn >;
    return continuation{
                        detail::create_context1< Record >(
                               salloc, std::forward< Fn >( fn) ) }.resume(
                   std::forward< Arg >( arg) ... );
}

template<
    typename StackAlloc,
    typename Fn,
    typename ... Arg
>
continuation
callcc( std::allocator_arg_t, preallocated palloc, StackAlloc salloc, Fn && fn, Arg ... arg) {
    using Record = detail::record< continuation, StackAlloc, Fn >;
    return continuation{
                        detail::create_context2< Record >(
                               palloc, salloc, std::forward< Fn >( fn) ) }.resume(
                   std::forward< Arg >( arg) ... );
}

// void
template<
    typename Fn,
    typename = detail::disable_overload< continuation, Fn >
>
continuation
callcc( Fn && fn) {
    return callcc(
            std::allocator_arg, fixedsize_stack(),
            std::forward< Fn >( fn) );
}

template< typename StackAlloc, typename Fn >
continuation
callcc( std::allocator_arg_t, StackAlloc salloc, Fn && fn) {
    using Record = detail::record< continuation, StackAlloc, Fn >;
    return continuation{
                detail::create_context1< Record >(
                        salloc, std::forward< Fn >( fn) ) }.resume();
}

template< typename StackAlloc, typename Fn >
continuation
callcc( std::allocator_arg_t, preallocated palloc, StackAlloc salloc, Fn && fn) {
    using Record = detail::record< continuation, StackAlloc, Fn >;
    return continuation{
                detail::create_context2< Record >(
                        palloc, salloc, std::forward< Fn >( fn) ) }.resume();
}

#if defined(BOOST_USE_SEGMENTED_STACKS)
template<
    typename Fn,
    typename ... Arg
>
continuation
callcc( std::allocator_arg_t, segmented_stack, Fn &&, Arg ...);

template<
    typename StackAlloc,
    typename Fn,
    typename ... Arg
>
continuation
callcc( std::allocator_arg_t, preallocated, segmented_stack, Fn &&, Arg ...);
#endif

// swap
inline
void swap( continuation & l, continuation & r) noexcept {
    l.swap( r);
}

}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_CONTINUATION_H
