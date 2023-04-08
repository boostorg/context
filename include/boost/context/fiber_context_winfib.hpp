
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FIBER_H
#define BOOST_CONTEXT_FIBER_H

#include <windows.h>

#include <boost/context/detail/config.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <ostream>
#include <system_error>
#include <tuple>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/detail/disable_overload.hpp>
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
#include <boost/context/detail/exchange.hpp>
#endif
#if defined(BOOST_NO_CXX17_STD_INVOKE)
#include <boost/context/detail/invoke.hpp>
#endif
#include <boost/context/fixedsize_stack.hpp>
#include <boost/context/flags.hpp>
#include <boost/context/preallocated.hpp>
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

// tampoline function
// entered if the execution context
// is resumed for the first time
template< typename Record >
static VOID WINAPI fiber_context_entry_func( LPVOID data) noexcept {
    Record * record = static_cast< Record * >( data);
    BOOST_ASSERT( nullptr != record);
    // start execution of toplevel context-function
    record->run();
}

struct BOOST_CONTEXT_DECL fiber_context_activation_record {
    LPVOID                                                      fiber_context{ nullptr };
    stack_context                                               sctx{};
    bool                                                        main_ctx{ true };
    fiber_context_activation_record                                       *   from{ nullptr };
    std::function< fiber_context_activation_record*(fiber_context_activation_record*&) >    ontop{};
    bool                                                        terminated{ false };
    bool                                                        force_unwind{ false };

    static fiber_context_activation_record *& current() noexcept;

    // used for toplevel-context
    // (e.g. main context, thread-entry context)
    fiber_context_activation_record() noexcept {
#if ( _WIN32_WINNT > 0x0600)
        if ( ::IsThreadAFiber() ) {
            fiber_context = ::GetCurrentFiber();
        } else {
            fiber_context = ::ConvertThreadToFiber( nullptr);
        }
#else
        fiber_context = ::ConvertThreadToFiber( nullptr);
        if ( BOOST_UNLIKELY( nullptr == fiber_context) ) {
            BOOST_ASSERT( ERROR_ALREADY_FIBER == ::GetLastError());
            fiber_context = ::GetCurrentFiber(); 
            BOOST_ASSERT( nullptr != fiber_context);
            BOOST_ASSERT( reinterpret_cast< LPVOID >( 0x1E00) != fiber_context);
        }
#endif
    }

    fiber_context_activation_record( stack_context sctx_) noexcept :
        sctx{ sctx_ },
        main_ctx{ false } {
    } 

    virtual ~fiber_context_activation_record() {
        if ( BOOST_UNLIKELY( main_ctx) ) {
            ::ConvertFiberToThread();
        } else {
            ::DeleteFiber( fiber_context);
        }
    }

    fiber_context_activation_record( fiber_context_activation_record const&) = delete;
    fiber_context_activation_record & operator=( fiber_context_activation_record const&) = delete;

    bool is_main_context() const noexcept {
        return main_ctx;
    }

    fiber_context_activation_record * resume() {
        from = current();
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        current() = this;
        // context switch from parent context to `this`-context
        // context switch
        ::SwitchToFiber( fiber_context);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return detail::exchange( current()->from, nullptr);
#else
        return std::exchange( current()->from, nullptr);
#endif
    }

    template< typename Ctx, typename Fn >
    fiber_context_activation_record * resume_with( Fn && fn) {
        from = current();
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        // returned by fiber_context::current()
        current() = this;
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
        current()->ontop = std::bind(
                [](typename std::decay< Fn >::type & fn, fiber_context_activation_record *& ptr){
                    Ctx c{ ptr };
                    c = fn( std::move( c) );
                    if ( ! c) {
                        ptr = nullptr;
                    }
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
                    return exchange( c.ptr_, nullptr);
#else
                    return std::exchange( c.ptr_, nullptr);
#endif
                },
                std::forward< Fn >( fn),
                std::placeholders::_1);
#else
        current()->ontop = [fn=std::forward<Fn>(fn)](fiber_context_activation_record *& ptr){
            Ctx c{ ptr };
            c = fn( std::move( c) );
            if ( ! c) {
                ptr = nullptr;
            }
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
            return exchange( c.ptr_, nullptr);
#else
            return std::exchange( c.ptr_, nullptr);
#endif
        };
#endif
        // context switch
        ::SwitchToFiber( fiber_context);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return detail::exchange( current()->from, nullptr);
#else
        return std::exchange( current()->from, nullptr);
#endif
    }

    virtual void deallocate() noexcept {
    }
};

struct BOOST_CONTEXT_DECL fiber_context_activation_record_initializer {
    fiber_context_activation_record_initializer() noexcept;
    ~fiber_context_activation_record_initializer();
};

struct forced_unwind {
    fiber_context_activation_record  *  from{ nullptr };

    explicit forced_unwind( fiber_context_activation_record * from_) :
        from{ from_ } {
    }
};

template< typename Ctx, typename StackAlloc, typename Fn >
class fiber_context_capture_record : public fiber_context_activation_record {
private:
    typename std::decay< StackAlloc >::type             salloc_;
    typename std::decay< Fn >::type                     fn_;

    static void destroy( fiber_context_capture_record * p) noexcept {
        typename std::decay< StackAlloc >::type salloc = std::move( p->salloc_);
        stack_context sctx = p->sctx;
        // deallocate activation record
        p->~fiber_context_capture_record();
        // destroy stack with stack allocator
        salloc.deallocate( sctx);
    }

public:
    fiber_context_capture_record( stack_context sctx, StackAlloc && salloc, Fn && fn) noexcept :
        fiber_context_activation_record( sctx),
        salloc_( std::forward< StackAlloc >( salloc)),
        fn_( std::forward< Fn >( fn) ) {
    }

    void deallocate() noexcept override final {
        BOOST_ASSERT( main_ctx || ( ! main_ctx && terminated) );
        destroy( this);
    }

    void run() {
        Ctx c{ from };
        try {
            // invoke context-function
#if defined(BOOST_NO_CXX17_STD_INVOKE)
            c = boost::context::detail::invoke( fn_, std::move( c) );
#else
            c = std::invoke( fn_, std::move( c) );
#endif  
        } catch ( forced_unwind const& ex) {
            c = Ctx{ ex.from };
        }
        // this context has finished its task
        from = nullptr;
        ontop = nullptr;
        terminated = true;
        force_unwind = false;
        std::move( c).resume();
        BOOST_ASSERT_MSG( false, "fiber_context already terminated");
    }
};

template< typename Ctx, typename StackAlloc, typename Fn >
static fiber_context_activation_record * create_fiber_context1( StackAlloc && salloc, Fn && fn) {
    typedef fiber_context_capture_record< Ctx, StackAlloc, Fn >  capture_t;

    auto sctx = salloc.allocate();
    BOOST_ASSERT( ( sizeof( capture_t) ) < sctx.size);
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( capture_t) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    capture_t * record = new ( storage) capture_t{
            sctx, std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) };
    // create user-context
    record->fiber_context = ::CreateFiber( sctx.size, & detail::fiber_context_entry_func< capture_t >, record);
    return record;
}

template< typename Ctx, typename StackAlloc, typename Fn >
static fiber_context_activation_record * create_fiber_context2( preallocated palloc, StackAlloc && salloc, Fn && fn) {
    typedef fiber_context_capture_record< Ctx, StackAlloc, Fn >  capture_t; 

    BOOST_ASSERT( ( sizeof( capture_t) ) < palloc.size);
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( palloc.sp) - static_cast< uintptr_t >( sizeof( capture_t) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    capture_t * record = new ( storage) capture_t{
            palloc.sctx, std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) };
    // create user-context
    record->fiber_context = ::CreateFiber( palloc.sctx.size, & detail::fiber_context_entry_func< capture_t >, record);
    return record;
}

}

class BOOST_CONTEXT_DECL fiber_context {
private:
    friend struct detail::fiber_context_activation_record;

    template< typename Ctx, typename StackAlloc, typename Fn >
    friend class detail::fiber_context_capture_record;

    template< typename Ctx, typename StackAlloc, typename Fn >
    friend detail::fiber_context_activation_record * detail::create_fiber_context1( StackAlloc &&, Fn &&);

    template< typename Ctx, typename StackAlloc, typename Fn >
    friend detail::fiber_context_activation_record * detail::create_fiber_context2( preallocated, StackAlloc &&, Fn &&);

    detail::fiber_context_activation_record   *   ptr_{ nullptr };

    fiber_context( detail::fiber_context_activation_record * ptr) noexcept :
        ptr_{ ptr } {
    }

public:
    fiber_context() = default;

    template< typename Fn, typename = detail::disable_overload< fiber_context, Fn > >
    fiber_context( Fn && fn) :
        fiber_context{ std::allocator_arg,
               fixedsize_stack(),
               std::forward< Fn >( fn) } {
    }

    template< typename StackAlloc, typename Fn >
    fiber_context( std::allocator_arg_t, StackAlloc && salloc, Fn && fn) :
        ptr_{ detail::create_fiber_context1< fiber_context >(
                std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) ) } {;
    }

    template< typename StackAlloc, typename Fn >
    fiber_context( std::allocator_arg_t, preallocated palloc, StackAlloc && salloc, Fn && fn) :
        ptr_{ detail::create_fiber_context2< fiber_context >(
                palloc, std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) ) } {
    }

    ~fiber_context() {
        if ( BOOST_UNLIKELY( nullptr != ptr_) && ! ptr_->main_ctx) {
            if ( BOOST_LIKELY( ! ptr_->terminated) ) {
                ptr_->force_unwind = true;
                ptr_->resume();
                BOOST_ASSERT( ptr_->terminated);
            }
            ptr_->deallocate();
        }
    }

    fiber_context( fiber_context const&) = delete;
    fiber_context & operator=( fiber_context const&) = delete;

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

    fiber_context resume() && {
        BOOST_ASSERT( nullptr != ptr_);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::fiber_context_activation_record * ptr = detail::exchange( ptr_, nullptr)->resume();
#else
        detail::fiber_context_activation_record * ptr = std::exchange( ptr_, nullptr)->resume();
#endif
        if ( BOOST_UNLIKELY( detail::fiber_context_activation_record::current()->force_unwind) ) {
            throw detail::forced_unwind{ ptr};
        } else if ( BOOST_UNLIKELY( nullptr != detail::fiber_context_activation_record::current()->ontop) ) {
            ptr = detail::fiber_context_activation_record::current()->ontop( ptr);
            detail::fiber_context_activation_record::current()->ontop = nullptr;
        }
        return { ptr };
    }

    template< typename Fn >
    fiber_context resume_with( Fn && fn) && {
        BOOST_ASSERT( nullptr != ptr_);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::fiber_context_activation_record * ptr =
            detail::exchange( ptr_, nullptr)->resume_with< fiber_context >( std::forward< Fn >( fn) );
#else
        detail::fiber_context_activation_record * ptr =
            std::exchange( ptr_, nullptr)->resume_with< fiber_context >( std::forward< Fn >( fn) );
#endif
        if ( BOOST_UNLIKELY( detail::fiber_context_activation_record::current()->force_unwind) ) {
            throw detail::forced_unwind{ ptr};
        } else if ( BOOST_UNLIKELY( nullptr != detail::fiber_context_activation_record::current()->ontop) ) {
            ptr = detail::fiber_context_activation_record::current()->ontop( ptr);
            detail::fiber_context_activation_record::current()->ontop = nullptr;
        }
        return { ptr };
    }

    bool can_resume() noexcept {
        if ( ! empty() ) {
            // TODO:
            // *this has no owning thread
            // calling thread is the owning thread represented by *this
            return true;
        }
        return false;
    }

    explicit operator bool() const noexcept {
        return ! empty();
    }

    bool empty() const noexcept {
        return nullptr == ptr_ || ptr_->terminated;
    }

    void swap( fiber_context & other) noexcept {
        std::swap( ptr_, other.ptr_);
    }

    #if !defined(BOOST_EMBTC)

    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber_context const& other) {
        if ( nullptr != other.ptr_) {
            return os << other.ptr_;
        } else {
            return os << "{not-a-context}";
        }
    }

    #else

    template< typename charT, class traitsT >
    friend std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber_context const& other);

    #endif
};

#if defined(BOOST_EMBTC)

    template< typename charT, class traitsT >
    inline std::basic_ostream< charT, traitsT > &
    operator<<( std::basic_ostream< charT, traitsT > & os, fiber_context const& other) {
        if ( nullptr != other.ptr_) {
            return os << other.ptr_;
        } else {
            return os << "{not-a-context}";
        }
    }

#endif

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

#endif // BOOST_CONTEXT_FIBER_H
