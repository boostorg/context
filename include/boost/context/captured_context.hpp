
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_CAPTURED_CONTEXT_H
#define BOOST_CONTEXT_CAPTURED_CONTEXT_H

#include <boost/context/detail/config.hpp>

#if ! defined(BOOST_CONTEXT_NO_CXX11)

# include <algorithm>
# include <cstddef>
# include <cstdint>
# include <cstdlib>
# include <functional>
# include <memory>
# include <ostream>
# include <tuple>
# include <utility>

# include <boost/assert.hpp>
# include <boost/config.hpp>
# include <boost/intrusive_ptr.hpp>

# include <boost/context/detail/apply.hpp>
# include <boost/context/detail/disable_overload.hpp>
# include <boost/context/detail/exception.hpp>
# include <boost/context/detail/exchange.hpp>
# include <boost/context/detail/fcontext.hpp>
# include <boost/context/fixedsize_stack.hpp>
# include <boost/context/flags.hpp>
# include <boost/context/preallocated.hpp>
# include <boost/context/segmented_stack.hpp>
# include <boost/context/stack_context.hpp>

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
# endif

namespace boost {
namespace context {
namespace detail {

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
    BOOST_ASSERT( nullptr != rec);
    transfer_t t = { nullptr, nullptr };
    try {
        // jump back to `context_create()`
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

template< typename Ctx, typename Fn, typename Tpl >
transfer_t context_ontop( transfer_t t) {
    auto tpl = static_cast< std::tuple< void *, Fn, Tpl > * >( t.data);
    BOOST_ASSERT( nullptr != tpl);
    auto data = std::get< 0 >( * tpl);
    typename std::decay< Fn >::type fn = std::forward< Fn >( std::get< 1 >( * tpl) );
    auto args = std::forward< Tpl >( std::get< 2 >( * tpl) );
    Ctx ctx{ t.fctx };
    // execute function
    std::tie( ctx, data) = apply(
            fn,
            std::tuple_cat(
                args,
                std::forward_as_tuple( std::move( ctx) ),
                std::tie( data) ) );
    return { exchange( ctx.fctx_, nullptr), data };
}

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Args >
class record {
private:
    StackAlloc                                          salloc_;
    stack_context                                       sctx_;
    typename std::decay< Fn >::type                     fn_;
    std::tuple< typename std::decay< Args >::type ... > args_;

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
            Fn && fn, Args && ... args) noexcept :
        salloc_( salloc),
        sctx_( sctx),
        fn_( std::forward< Fn >( fn) ),
        args_( std::forward< Args >( args) ... ) {
    }

    record( record const&) = delete;
    record & operator=( record const&) = delete;

    void deallocate() noexcept {
        destroy( this);
    }

    transfer_t run( transfer_t t) {
        Ctx from{ t.fctx };
        // invoke context-function
        Ctx cc = apply(
                fn_,
                std::tuple_cat(
                    args_,
                    std::forward_as_tuple( std::move( from) ),
                    std::tie( t.data) ) );
        return { exchange( cc.fctx_, nullptr), nullptr };
    }
};

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Args >
fcontext_t context_create( StackAlloc salloc, Fn && fn, Args && ... args) {
    typedef record< Ctx, StackAlloc, Fn, Args ... >  record_t;

    auto sctx = salloc.allocate();
    // reserve space for control structure
#if defined(BOOST_NO_CXX11_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    const std::size_t size = sctx.size - sizeof( record_t);
    void * sp = static_cast< char * >( sctx.sp) - sizeof( record_t);
#else
    constexpr std::size_t func_alignment = 64; // alignof( record_t);
    constexpr std::size_t func_size = sizeof( record_t);
    // reserve space on stack
    void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    const std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
    // create fast-context
    const fcontext_t fctx = make_fcontext( sp, size, & context_entry< record_t >);
    BOOST_ASSERT( nullptr != fctx);
    // placment new for control structure on context-stack
    auto rec = ::new ( sp) record_t{
            sctx, salloc, std::forward< Fn >( fn), std::forward< Args >( args) ... };
    // transfer control structure to context-stack
    return jump_fcontext( fctx, rec).fctx;
}

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Args >
fcontext_t context_create( preallocated palloc, StackAlloc salloc, Fn && fn, Args && ... args) {
    typedef record< Ctx, StackAlloc, Fn, Args ... >  record_t;

    // reserve space for control structure
#if defined(BOOST_NO_CXX11_CONSTEXPR) || defined(BOOST_NO_CXX11_STD_ALIGN)
    const std::size_t size = palloc.size - sizeof( record_t);
    void * sp = static_cast< char * >( palloc.sp) - sizeof( record_t);
#else
    constexpr std::size_t func_alignment = 64; // alignof( record_t);
    constexpr std::size_t func_size = sizeof( record_t);
    // reserve space on stack
    void * sp = static_cast< char * >( palloc.sp) - func_size - func_alignment;
    // align sp pointer
    std::size_t space = func_size + func_alignment;
    sp = std::align( func_alignment, func_size, sp, space);
    BOOST_ASSERT( nullptr != sp);
    // calculate remaining size
    const std::size_t size = palloc.size - ( static_cast< char * >( palloc.sp) - static_cast< char * >( sp) );
#endif
    // create fast-context
    const fcontext_t fctx = make_fcontext( sp, size, & context_entry< record_t >);
    BOOST_ASSERT( nullptr != fctx);
    // placment new for control structure on context-stack
    auto rec = ::new ( sp) record_t{
            palloc.sctx, salloc, std::forward< Fn >( fn), std::forward< Args >( args) ... };
    // transfer control structure to context-stack
    return jump_fcontext( fctx, rec).fctx;
}

}

class BOOST_CONTEXT_DECL captured_context {
private:
    template< typename Ctx, typename StackAlloc, typename Fn, typename ... Args >
    friend class detail::record;

    template< typename Ctx, typename Fn, typename Tpl >
    friend detail::transfer_t detail::context_ontop( detail::transfer_t);

    detail::fcontext_t  fctx_{ nullptr };

    captured_context( detail::fcontext_t fctx) noexcept :
        fctx_( fctx) {
    }

    template< typename Fn, typename ... Args >
    detail::transfer_t resume_ontop_( void * data, Fn && fn, Args && ... args) {
        typedef std::tuple< typename std::decay< Args >::type ... > tpl_t;
        tpl_t tpl{ std::forward< Args >( args) ... };
        std::tuple< void *, Fn, tpl_t > p = std::forward_as_tuple( data, fn, tpl);
        return detail::ontop_fcontext(
                detail::exchange( fctx_, nullptr),
                & p,
                detail::context_ontop< captured_context, Fn, tpl_t >);
    }

public:
    constexpr captured_context() noexcept = default;

# if defined(BOOST_USE_SEGMENTED_STACKS)
    // segmented-stack requires to preserve the segments of the `current` context
    // which is not possible (no global pointer to current context)
    template< typename Fn, typename ... Args >
    captured_context( std::allocator_arg_t, segmented_stack, Fn &&, Args && ...) = delete;

    template< typename Fn, typename ... Args >
    captured_context( std::allocator_arg_t, preallocated, segmented_stack, Fn &&, Args && ...) = delete;
# else
    template< typename Fn,
              typename ... Args,
              typename = detail::disable_overload< captured_context, Fn >
    >
    captured_context( Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        fctx_( detail::context_create< captured_context >(
                    fixedsize_stack(),
                    std::forward< Fn >( fn),
                    std::forward< Args >( args) ... ) ) {
    }

    template< typename StackAlloc,
              typename Fn,
              typename ... Args
    >
    captured_context( std::allocator_arg_t, StackAlloc salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        fctx_( detail::context_create< captured_context >(
                    salloc,
                    std::forward< Fn >( fn),
                    std::forward< Args >( args) ... ) ) {
    }

    template< typename StackAlloc,
              typename Fn,
              typename ... Args
    >
    captured_context( std::allocator_arg_t, preallocated palloc, StackAlloc salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        fctx_( detail::context_create< captured_context >(
                    palloc, salloc,
                    std::forward< Fn >( fn),
                    std::forward< Args >( args) ... ) ) {
    }
# endif

    ~captured_context() {
        if ( nullptr != fctx_) {
            detail::ontop_fcontext( detail::exchange( fctx_, nullptr), nullptr, detail::context_unwind);
        }
    }

    captured_context( captured_context && other) noexcept :
        fctx_( other.fctx_) {
        other.fctx_ = nullptr;
    }

    captured_context & operator=( captured_context && other) noexcept {
        if ( this != & other) {
            captured_context tmp = std::move( other);
            swap( tmp);
        }
        return * this;
    }

    captured_context( captured_context const& other) noexcept = delete;
    captured_context & operator=( captured_context const& other) noexcept = delete;

    std::tuple< captured_context, void * > operator()( void * data = nullptr) {
        BOOST_ASSERT( nullptr != fctx_);
        detail::transfer_t t = detail::jump_fcontext( detail::exchange( fctx_, nullptr), data);
        return std::make_tuple( captured_context( t.fctx), t.data);
    }

    template< typename Fn, typename ... Args >
    std::tuple< captured_context, void * > operator()( exec_ontop_arg_t, Fn && fn, Args && ... args) {
        BOOST_ASSERT( nullptr != fctx_);
        detail::transfer_t t = resume_ontop_( nullptr,
                                      std::forward< Fn >( fn),
                                      std::forward< Args >( args) ... );
        return std::make_tuple( captured_context( t.fctx), t.data);
    }

    template< typename Fn, typename ... Args >
    std::tuple< captured_context, void * > operator()( void * data, exec_ontop_arg_t, Fn && fn, Args && ... args) {
        BOOST_ASSERT( nullptr != fctx_);
        detail::transfer_t t = resume_ontop_( data,
                                      std::forward< Fn >( fn),
                                      std::forward< Args >( args) ... );
        return std::make_tuple( captured_context( t.fctx), t.data);
    }

    explicit operator bool() const noexcept {
        return nullptr != fctx_;
    }

    bool operator!() const noexcept {
        return nullptr == fctx_;
    }

    bool operator==( captured_context const& other) const noexcept {
        return fctx_ == other.fctx_;
    }

    bool operator!=( captured_context const& other) const noexcept {
        return fctx_ != other.fctx_;
    }

    bool operator<( captured_context const& other) const noexcept {
        return fctx_ < other.fctx_;
    }

    bool operator>( captured_context const& other) const noexcept {
        return other.fctx_ < fctx_;
    }

    bool operator<=( captured_context const& other) const noexcept {
        return ! ( * this > other);
    }

    bool operator>=( captured_context const& other) const noexcept {
        return ! ( * this < other);
    }

    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, captured_context const& other) {
        if ( nullptr != other.fctx_) {
            return os << other.fctx_;
        } else {
            return os << "{not-a-context}";
        }
    }

    void swap( captured_context & other) noexcept {
        std::swap( fctx_, other.fctx_);
    }
};

inline
void swap( captured_context & l, captured_context & r) noexcept {
    l.swap( r);
}

}}

# ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
# endif

#endif // BOOST_CONTEXT_NO_CPP11

#endif // BOOST_CONTEXT_CAPTURED_CONTEXT_H
