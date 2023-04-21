
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_FIBER_H
#define BOOST_CONTEXT_FIBER_H

#include <boost/predef/os.h>
#if BOOST_OS_MACOS
#define _XOPEN_SOURCE 600
#endif

extern "C" {
#include <ucontext.h>
}

#include <boost/predef.h>
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
#include <thread>
#include <utility>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/predef.h>

#include <boost/context/detail/disable_overload.hpp>
#include <boost/context/detail/externc.hpp>
#include <boost/context/fixedsize_stack.hpp>
#include <boost/context/flags.hpp>
#include <boost/context/preallocated.hpp>
#if defined(BOOST_USE_SEGMENTED_STACKS)
#include <boost/context/segmented_stack.hpp>
#endif
#include <boost/context/stack_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

#ifdef BOOST_USE_TSAN
#include <sanitizer/tsan_interface.h>
#endif

namespace std {
namespace detail {

// tampoline function
// entered if the execution context
// is resumed for the first time
template <typename Record>
#ifdef BOOST_OS_MACOS
static void fiber_context_entry_func(std::uint32_t data_high,
                             std::uint32_t data_low) noexcept {
  auto data =
      reinterpret_cast<void *>(std::uint64_t(data_high) << 32 | data_low);
#else
static void fiber_context_entry_func(void *data) noexcept {
#endif
  Record *record = static_cast<Record *>(data);
  BOOST_ASSERT(nullptr != record);
  // start execution of toplevel context-function
  record->run();
}

struct BOOST_CONTEXT_DECL fiber_context_activation_record {
    ucontext_t                                                  uctx{};
    stack_context                                               sctx{};
    bool                                                        main_ctx{ true };
	fiber_context_activation_record                                       *	from{ nullptr };
    std::function< fiber_context_activation_record*(fiber_context_activation_record*&) >    ontop{};
    bool                                                        terminated{ false };
    bool                                                        force_unwind{ false };
    std::thread::id                                             id{ std::this_thread::get_id() };
#if defined(BOOST_USE_ASAN)
    void                                                    *   fake_stack{ nullptr };
    void                                                    *   stack_bottom{ nullptr };
    std::size_t                                                 stack_size{ 0 };
#endif

#if defined(BOOST_USE_TSAN)
    void * tsan_fiber_context{ nullptr };
    bool destroy_tsan_fiber_context{ true };
#endif

    static fiber_context_activation_record *& current() noexcept;

    // used for toplevel-context
    // (e.g. main context, thread-entry context)
    fiber_context_activation_record() {
        if ( BOOST_UNLIKELY( 0 != ::getcontext( & uctx) ) ) {
            throw std::system_error(
                    std::error_code( errno, std::system_category() ),
                    "getcontext() failed");
        }

#if defined(BOOST_USE_TSAN)
        tsan_fiber_context = __tsan_get_current_fiber_context();
        destroy_tsan_fiber_context = false;
#endif
    }

    fiber_context_activation_record( stack_context sctx_) noexcept :
        sctx( sctx_ ),
        main_ctx( false ) {
    } 

    virtual ~fiber_context_activation_record() {
#if defined(BOOST_USE_TSAN)
        if (destroy_tsan_fiber_context)
            __tsan_destroy_fiber_context(tsan_fiber_context);
#endif
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
#if defined(BOOST_USE_SEGMENTED_STACKS)
        // adjust segmented stack properties
        __splitstack_getcontext( from->sctx.segments_ctx);
        __splitstack_setcontext( sctx.segments_ctx);
#endif
#if defined(BOOST_USE_ASAN)
        if ( terminated) {
            __sanitizer_start_switch_fiber_context( nullptr, stack_bottom, stack_size);
        } else {
            __sanitizer_start_switch_fiber_context( & from->fake_stack, stack_bottom, stack_size);
        }
#endif
#if defined (BOOST_USE_TSAN)
        __tsan_switch_to_fiber_context(tsan_fiber_context, 0);
#endif
        // context switch from parent context to `this`-context
        ::swapcontext( & from->uctx, & uctx);
#if defined(BOOST_USE_ASAN)
        __sanitizer_finish_switch_fiber_context( current()->fake_stack,
                                         (const void **) & current()->from->stack_bottom,
                                         & current()->from->stack_size);
#endif
        return std::exchange( current()->from, nullptr);
    }

    template< typename Ctx, typename Fn >
    fiber_context_activation_record * resume_with( Fn && fn) {
		from = current();
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        // returned by fiber_context::current()
        current() = this;
        current()->ontop = [fn=std::forward<Fn>(fn)](fiber_context_activation_record *& ptr){
            Ctx c{ ptr };
            c = fn( std::move( c) );
            if ( ! c) {
                ptr = nullptr;
            }
            return std::exchange( c.ptr_, nullptr);
        };
#if defined(BOOST_USE_SEGMENTED_STACKS)
        // adjust segmented stack properties
        __splitstack_getcontext( from->sctx.segments_ctx);
        __splitstack_setcontext( sctx.segments_ctx);
#endif
#if defined(BOOST_USE_ASAN)
        __sanitizer_start_switch_fiber_context( & from->fake_stack, stack_bottom, stack_size);
#endif
#if defined (BOOST_USE_TSAN)
        __tsan_switch_to_fiber_context(tsan_fiber_context, 0);
#endif
        // context switch from parent context to `this`-context
        ::swapcontext( & from->uctx, & uctx);
#if defined(BOOST_USE_ASAN)
        __sanitizer_finish_switch_fiber_context( current()->fake_stack,
                                         (const void **) & current()->from->stack_bottom,
                                         & current()->from->stack_size);
#endif
        return std::exchange( current()->from, nullptr);
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

    forced_unwind( fiber_context_activation_record * from_) noexcept :
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
        fiber_context_activation_record{ sctx },
        salloc_{ std::forward< StackAlloc >( salloc) },
        fn_( std::forward< Fn >( fn) ) {
    }

    void deallocate() noexcept override final {
        BOOST_ASSERT( main_ctx || ( ! main_ctx && terminated) );
        destroy( this);
    }

    void run() {
#if defined(BOOST_USE_ASAN)
        __sanitizer_finish_switch_fiber_context( fake_stack,
                                         (const void **) & from->stack_bottom,
                                         & from->stack_size);
#endif
        Ctx c{ from };
        try {
            // invoke context-function
            c = std::invoke( fn_, std::move( c) );
        } catch ( forced_unwind const& ex) {
            c = Ctx{ ex.from };
        }
        // this context has finished its task
		from = nullptr;
        ontop = nullptr;
        terminated = true;
        force_unwind = false;
        id = std::thread::id{};
        std::move( c).resume();
        BOOST_ASSERT_MSG( false, "fiber_context already terminated");
    }
};

template< typename Ctx, typename StackAlloc, typename Fn >
static fiber_context_activation_record * create_fiber_context1( StackAlloc && salloc, Fn && fn) {
    typedef fiber_context_capture_record< Ctx, StackAlloc, Fn >  capture_t;

    auto sctx = salloc.allocate();
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( capture_t) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    capture_t * record = new ( storage) capture_t{
            sctx, std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) };
    // stack bottom
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sctx.size) );
    // create user-context
    if ( BOOST_UNLIKELY( 0 != ::getcontext( & record->uctx) ) ) {
        record->~capture_t();
        salloc.deallocate( sctx);
        throw std::system_error(
                std::error_code( errno, std::system_category() ),
                "getcontext() failed");
    }
#if BOOST_OS_BSD_FREE
    // because FreeBSD defines stack_t::ss_sp as char *
    record->uctx.uc_stack.ss_sp = static_cast< char * >( stack_bottom);
#else
    record->uctx.uc_stack.ss_sp = stack_bottom;
#endif
    // 64byte gap between control structure and stack top
    record->uctx.uc_stack.ss_size = reinterpret_cast< uintptr_t >( storage) -
            reinterpret_cast< uintptr_t >( stack_bottom) - static_cast< uintptr_t >( 64);
    record->uctx.uc_link = nullptr;
#ifdef BOOST_OS_MACOS
    const auto integer = std::uint64_t(record);
    ::makecontext(&record->uctx, (void (*)()) & fiber_context_entry_func<capture_t>, 2,
                  std::uint32_t((integer >> 32) & 0xFFFFFFFF),
                  std::uint32_t(integer));
#else
    ::makecontext(&record->uctx, (void (*)()) & fiber_context_entry_func<capture_t>, 1,
                  record);
#endif
#if defined(BOOST_USE_ASAN)
    record->stack_bottom = record->uctx.uc_stack.ss_sp;
    record->stack_size = record->uctx.uc_stack.ss_size;
#endif
#if defined (BOOST_USE_TSAN)
    record->tsan_fiber_context = __tsan_create_fiber_context(0);
#endif
    return record;
}

template< typename Ctx, typename StackAlloc, typename Fn >
static fiber_context_activation_record * create_fiber_context2( preallocated palloc, StackAlloc && salloc, Fn && fn) {
    typedef fiber_context_capture_record< Ctx, StackAlloc, Fn >  capture_t; 

    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( palloc.sp) - static_cast< uintptr_t >( sizeof( capture_t) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    capture_t * record = new ( storage) capture_t{
            palloc.sctx, std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) };
    // stack bottom
    void * stack_bottom = reinterpret_cast< void * >(
            reinterpret_cast< uintptr_t >( palloc.sctx.sp) - static_cast< uintptr_t >( palloc.sctx.size) );
    // create user-context
    if ( BOOST_UNLIKELY( 0 != ::getcontext( & record->uctx) ) ) {
        record->~capture_t();
        salloc.deallocate( palloc.sctx);
        throw std::system_error(
                std::error_code( errno, std::system_category() ),
                "getcontext() failed");
    }
#if BOOST_OS_BSD_FREE
    // because FreeBSD defines stack_t::ss_sp as char *
    record->uctx.uc_stack.ss_sp = static_cast< char * >( stack_bottom);
#else
    record->uctx.uc_stack.ss_sp = stack_bottom;
#endif
    // 64byte gap between control structure and stack top
    record->uctx.uc_stack.ss_size = reinterpret_cast< uintptr_t >( storage) -
            reinterpret_cast< uintptr_t >( stack_bottom) - static_cast< uintptr_t >( 64);
    record->uctx.uc_link = nullptr;
#ifdef BOOST_OS_MACOS
    const auto integer = std::uint64_t(record);
    ::makecontext(&record->uctx, (void (*)()) & fiber_context_entry_func<capture_t>, 2,
                  std::uint32_t((integer >> 32) & 0xFFFFFFFF),
                  std::uint32_t(integer));
#else
    ::makecontext(&record->uctx, (void (*)()) & fiber_context_entry_func<capture_t>, 1,
                  record);
#endif
#if defined(BOOST_USE_ASAN)
    record->stack_bottom = record->uctx.uc_stack.ss_sp;
    record->stack_size = record->uctx.uc_stack.ss_size;
#endif
#if defined (BOOST_USE_TSAN)
    record->tsan_fiber_context = __tsan_create_fiber_context(0);
#endif
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
        fiber_context{
                std::allocator_arg,
#if defined(BOOST_USE_SEGMENTED_STACKS)
                segmented_stack(),
#else
                fixedsize_stack(),
#endif
                std::forward< Fn >( fn) } {
    }

    template< typename StackAlloc, typename Fn >
    fiber_context( std::allocator_arg_t, StackAlloc && salloc, Fn && fn) :
        ptr_{ detail::create_fiber_context1< fiber_context >(
                std::forward< StackAlloc >( salloc), std::forward< Fn >( fn) ) } {
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
        detail::fiber_context_activation_record * ptr = std::exchange( ptr_, nullptr)->resume();
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
        detail::fiber_context_activation_record * ptr =
            std::exchange( ptr_, nullptr)->resume_with< fiber_context >( std::forward< Fn >( fn) );
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
            return std::this_thread::get_id() == ptr_->id;
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

}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_FIBER_H
