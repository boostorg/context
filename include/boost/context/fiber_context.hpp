
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FIBER_CONTEXT_H
#define BOOST_CONTEXT_FIBER_CONTEXT_H

#define UNW_LOCAL_ONLY

#include <boost/context/detail/config.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <utility>
#include <variant>

#include <unwind.h>
#include <libunwind.h>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/intrusive_ptr.hpp>

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

constexpr _Unwind_Exception_Class __gxx_fiber_exception_class
= ((((((((_Unwind_Exception_Class) 'G'
     << 8 | (_Unwind_Exception_Class) 'N')
    << 8 | (_Unwind_Exception_Class) 'U')
       << 8 | (_Unwind_Exception_Class) 'F')
      << 8 | (_Unwind_Exception_Class) 'I')
     << 8 | (_Unwind_Exception_Class) 'B')
    << 8 | (_Unwind_Exception_Class) 'E')
   << 8 | (_Unwind_Exception_Class) 'R');

#define __GXX_INIT_FIBER_EXCEPTION_CLASS(c) c = __gxx_fiber_exception_class

namespace boost {
namespace context {
namespace detail {

class foreign_unwind_ex : public _Unwind_Exception {
private:
    static void fiber_unwind_cleanup( _Unwind_Reason_Code code, _Unwind_Exception * exc) {
        // We only want to be called through _Unwind_DeleteException.
        if ( _URC_FOREIGN_EXCEPTION_CAUGHT != code) {
            std::terminate();
        }
        delete exc;
    }

public:
    foreign_unwind_ex() noexcept {
        __GXX_INIT_FIBER_EXCEPTION_CLASS( exception_class);
        exception_cleanup = fiber_unwind_cleanup;
    }
};

struct data {
    enum flag_t {
        flag_side_stack = 0,
        flag_hosting_thread
    };

    flag_t                                  f;
    std::variant< bool, std::thread::id >   v;
};

inline
bool fiber_uses_side_stack() {
    unw_cursor_t cursor;
    unw_context_t context;
    unw_getcontext( & context);
    unw_init_local( & cursor, & context);
    while ( 0 < unw_step( & cursor) );
    unw_word_t offset;
    char sym[256];
    int err = unw_get_proc_name( & cursor, sym, sizeof( sym), & offset);
    if ( 0 != err) {
        return false;
    }
    return std::string( sym) == "make_fcontext";
}

template< typename Ctx >
_Unwind_Reason_Code fiber_unwind_stop( int version,
        _Unwind_Action actions,
        _Unwind_Exception_Class exc_class,
        _Unwind_Exception * exc,
        _Unwind_Context * context,
        void * param) {
    if ( actions & _UA_END_OF_STACK) {
        _Unwind_DeleteException( exc);
        // end of stack: switch back to calling fiber
        Ctx{ param }.resume();
        __builtin_unreachable();
    }
    // This is not the destination from. The unwind runtime will call frame's
    // personality routine with the _UA_FORCE_UNWIND and _UA_CLEANUP_PHASE flag
    // set in actions, and then unwind to the next frame and call the stop()
    // function again.
    return _URC_NO_REASON;
}

template< typename Ctx >
transfer_t fiber_unwind( transfer_t t) {
    _Unwind_ForcedUnwind( new foreign_unwind_ex{}, fiber_unwind_stop< Ctx >, t.fctx);
    __builtin_unreachable();
}

template< typename Rec >
transfer_t fiber_exit( transfer_t t) noexcept {
    Rec * rec = static_cast< Rec * >( t.data);
    // destroy context stack
    rec->deallocate();
    return { nullptr, nullptr };
}

template< typename Rec >
void fiber_entry( transfer_t t) {
    // transfer control structure to the context-stack
    Rec * rec = static_cast< Rec * >( t.data);
    BOOST_ASSERT( nullptr != t.fctx);
    BOOST_ASSERT( nullptr != rec);
rep:
    // jump back to `create_context()`
    t = jump_fcontext( t.fctx, nullptr);
    // test if stack walk was requested
    if ( nullptr != t.data) {
        data * d = static_cast< data * >( t.data);
        if ( data::flag_side_stack == d->f) {
            d->v = fiber_uses_side_stack();
        } else if ( data::flag_hosting_thread == d->f) {
            d->v = std::thread::id{};
        }
        goto rep;
    }
    // start executing
    t.fctx = rec->run( t.fctx);
    BOOST_ASSERT( nullptr != t.fctx);
    // destroy context-stack of `this`context on next context
    ontop_fcontext( t.fctx, rec, fiber_exit< Rec >);
    BOOST_ASSERT_MSG( false, "context already terminated");
}

template< typename Ctx, typename Fn >
transfer_t fiber_ontop( transfer_t t) {
    auto p = static_cast< std::tuple< Fn > * >( t.data);
    BOOST_ASSERT( nullptr != p);
    typename std::decay< Fn >::type fn = std::get< 0 >( * p);
    t.data = nullptr;
    // execute function, pass fiber_context via reference
    Ctx c = fn( Ctx{ t.fctx } );
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
    return { exchange( c.fctx_, nullptr), nullptr };
#else
    return { std::exchange( c.fctx_, nullptr), nullptr };
#endif
}

template< typename Ctx, typename StackAlloc, typename Fn >
class fiber_record {
private:
    stack_context                                       sctx_;
    typename std::decay< StackAlloc >::type             salloc_;
    typename std::decay< Fn >::type                     fn_;

    static void destroy( fiber_record * p) noexcept {
        typename std::decay< StackAlloc >::type salloc = std::move( p->salloc_);
        stack_context sctx = p->sctx_;
        // deallocate fiber_record
        p->~fiber_record();
        // destroy stack with stack allocator
        salloc.deallocate( sctx);
    }

public:
    fiber_record( stack_context sctx, StackAlloc && salloc,
            Fn && fn) noexcept :
        sctx_( sctx),
        salloc_( std::forward< StackAlloc >( salloc)),
        fn_( std::forward< Fn >( fn) ) {
    }

    fiber_record( fiber_record const&) = delete;
    fiber_record & operator=( fiber_record const&) = delete;

    void deallocate() noexcept {
        destroy( this);
    }

    fcontext_t run( fcontext_t fctx) {
        // invoke context-function
#if defined(BOOST_NO_CXX17_STD_INVOKE)
        Ctx c = invoke( fn_, Ctx{ fctx } );
#else
        Ctx c = std::invoke( fn_, Ctx{ fctx } );
#endif
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return exchange( c.fctx_, nullptr);
#else
        return std::exchange( c.fctx_, nullptr);
#endif
    }
};

template< typename Record, typename Fn >
fcontext_t create_fiber( Fn && fn) {
    fixedsize_stack salloc;
    auto sctx = salloc.allocate();
    // reserve space for control structure
	void * storage = reinterpret_cast< void * >(
			( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( Record) ) )
            & ~static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    Record * record = new ( storage) Record{
            sctx, std::forward< fixedsize_stack >( salloc), std::forward< Fn >( fn) };
    // 64byte gab between control structure and stack top
    // should be 16byte aligned
    void * stack_top = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( storage) - static_cast< uintptr_t >( 64) );
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sctx.size) );
    // create fast-context
    const std::size_t size = reinterpret_cast< uintptr_t >( stack_top) - reinterpret_cast< uintptr_t >( stack_bottom);
    const fcontext_t fctx = make_fcontext( stack_top, size, & fiber_entry< Record >);
    BOOST_ASSERT( nullptr != fctx);
    // transfer control structure to context-stack
    return jump_fcontext( fctx, record).fctx;
}

}

class fiber_context;
void unwind_fiber( fiber_context &&);

class fiber_context {
private:
    template< typename Ctx, typename StackAlloc, typename Fn >
    friend class detail::fiber_record;

    template< typename Ctx, typename Fn >
    friend detail::transfer_t
    detail::fiber_ontop( detail::transfer_t);

    template< typename Ctx >
    friend _Unwind_Reason_Code detail::fiber_unwind_stop( int,
        _Unwind_Action,
        _Unwind_Exception_Class,
        _Unwind_Exception *,
        _Unwind_Context *,
        void *);

    friend void unwind_fiber( fiber_context &&);

    detail::fcontext_t  fctx_{ nullptr };

    fiber_context( detail::fcontext_t fctx) noexcept :
        fctx_{ fctx } {
    }

    void unwind_fiber_() {
        detail::ontop_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                detail::exchange( fctx_, nullptr),
#else
                std::exchange( fctx_, nullptr),
#endif
                nullptr,
                detail::fiber_unwind< fiber_context >);
    }

public:
    fiber_context() noexcept = default;

    template< typename Fn, typename = detail::disable_overload< fiber_context, Fn > >
    fiber_context( Fn && fn) :
        fctx_{
            detail::create_fiber< detail::fiber_record< fiber_context, fixedsize_stack, Fn > >(
                    std::forward< Fn >( fn) ) } {
    }

    ~fiber_context() {
        if ( BOOST_UNLIKELY( nullptr != fctx_) ) {
            unwind_fiber_();
        }
    }

    fiber_context( fiber_context && other) noexcept {
        swap( other);
    }

    fiber_context & operator=( fiber_context && other) noexcept {
        if ( BOOST_LIKELY( this != & other) ) {
            fiber_context tmp = std::move( other);
            swap( tmp);
        }
        return * this;
    }

    fiber_context( fiber_context const& other) noexcept = delete;
    fiber_context & operator=( fiber_context const& other) noexcept = delete;

    fiber_context resume() && {
        if ( ! can_resume() ) {
            throw std::domain_error("fiber can not resume from any thread");
        }
        return std::move( * this).resume_from_any_thread();
    }

    fiber_context resume_from_any_thread() && {
        BOOST_ASSERT( nullptr != fctx_);
        auto tid = std::this_thread::get_id();
rep:
        auto t = detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, nullptr),
#else
                    std::exchange( fctx_, nullptr),
#endif
                    nullptr);
        if ( nullptr != t.data) {
            detail::data * d = static_cast< detail::data * >( t.data);
            if ( detail::data::flag_side_stack == d->f) {
                d->v = detail::fiber_uses_side_stack();
            } else if ( detail::data::flag_hosting_thread == d->f) {
                d->v = tid;
            }
            fctx_ = t.fctx;
            goto rep;
        }
        return { t.fctx };
    }

    template< typename Fn >
    fiber_context resume_with( Fn && fn) && {
        if ( ! can_resume() ) {
            throw std::domain_error("fiber can not resume from any thread");
        }
        return std::move( * this).resume_from_any_thread_with( std::forward< Fn >( fn) );
    }

    template< typename Fn >
    fiber_context resume_from_any_thread_with( Fn && fn) && {
        BOOST_ASSERT( nullptr != fctx_);
        auto tid = std::this_thread::get_id();
        auto p = std::make_tuple( std::forward< Fn >( fn) );
        auto t = detail::ontop_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, nullptr),
#else
                    std::exchange( fctx_, nullptr),
#endif
                    & p,
                    detail::fiber_ontop< fiber_context, Fn >);
        while ( nullptr != t.data) {
            detail::data * d = static_cast< detail::data * >( t.data);
            if ( detail::data::flag_side_stack == d->f) {
                d->v = detail::fiber_uses_side_stack();
            } else if ( detail::data::flag_hosting_thread == d->f) {
                d->v = tid;
            }
            fctx_ = t.fctx;
            t = detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, nullptr),
#else
                    std::exchange( fctx_, nullptr),
#endif
                    nullptr);
        }
        return { t.fctx };
    }

    bool can_resume_from_any_thread() noexcept {
        BOOST_ASSERT( * this);
        detail::data d = { detail::data::flag_side_stack };
        auto t = detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, nullptr),
#else
                    std::exchange( fctx_, nullptr),
#endif
                    & d);
        fctx_ = t.fctx;
        return std::get< bool >( d.v);
    }

    bool can_resume() noexcept {
        BOOST_ASSERT( * this);
        detail::data d = { detail::data::flag_hosting_thread };
        auto t = detail::jump_fcontext(
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    detail::exchange( fctx_, nullptr),
#else
                    std::exchange( fctx_, nullptr),
#endif
                    & d);
        fctx_ = t.fctx;
        return (std::this_thread::get_id() == std::get< std::thread::id >( d.v)) ||
               (std::thread::id{} == std::get< std::thread::id >( d.v));
    }

    explicit operator bool() const noexcept {
        return nullptr != fctx_;
    }

    bool operator!() const noexcept {
        return nullptr == fctx_;
    }

    bool operator<( fiber_context const& other) const noexcept {
        return fctx_ < other.fctx_;
    }

    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber_context const& other) {
        if ( nullptr != other.fctx_) {
            return os << other.fctx_;
        } else {
            return os << "{not-a-fiber}";
        }
    }

    void swap( fiber_context & other) noexcept {
        std::swap( fctx_, other.fctx_);
    }
};

inline
void unwind_fiber( fiber_context && other) {
    other.unwind_fiber_();
}

inline
void swap( fiber_context & l, fiber_context & r) noexcept {
    l.swap( r);
}

}}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_FIBER_CONTEXT_H
