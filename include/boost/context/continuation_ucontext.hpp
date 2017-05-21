
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_CONTINUATION_H
#define BOOST_CONTEXT_CONTINUATION_H

extern "C" {
#include <ucontext.h>
}

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
#include <boost/context/detail/exception.hpp>

#if defined(BOOST_NO_CXX17_STD_APPLY)
#include <boost/context/detail/apply.hpp>
#endif
#include <boost/context/detail/disable_overload.hpp>
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
#include <boost/context/detail/exchange.hpp>
#endif
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

#if defined(BOOST_USE_ASAN)
extern "C" {
void __sanitizer_start_switch_fiber( void **, const void *, size_t);
void __sanitizer_finish_switch_fiber( void *, const void **, size_t *);
}
#endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C" {
void __splitstack_getcontext( void * [BOOST_CONTEXT_SEGMENTS]);
void __splitstack_setcontext( void * [BOOST_CONTEXT_SEGMENTS]);
}
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

// tampoline function
// entered if the execution context
// is resumed for the first time
template< typename Record >
static void entry_func( void * data) noexcept {
    Record * record = static_cast< Record * >( data);
    BOOST_ASSERT( nullptr != record);
    // start execution of toplevel context-function
    record->run();
}

struct BOOST_CONTEXT_DECL activation_record {
    thread_local static activation_record   *   current_rec;

    ucontext_t                  uctx{};
    stack_context               sctx{};
    bool                        main_ctx{ true };
    void                    *   data{ nullptr };
	activation_record       *	from{ nullptr };
    std::function< void() >     ontop{};
    bool                        terminated{ false };
    bool                        force_unwind{ false };
#if defined(BOOST_USE_ASAN)
    void                    *   fake_stack{ nullptr };
    void                    *   stack_bottom{ nullptr };
    std::size_t                 stack_size{ 0 };
#endif

    static activation_record *& current() noexcept;

    // used for toplevel-context
    // (e.g. main context, thread-entry context)
    activation_record() {
        if ( 0 != ::getcontext( & uctx) ) {
            throw std::system_error(
                    std::error_code( errno, std::system_category() ),
                    "getcontext() failed");
        }
#if defined(BOOST_USE_ASAN)
        stack_bottom = uctx.uc_stack.ss_sp;
        stack_size = uctx.uc_stack.ss_size;
#endif
    }

    activation_record( stack_context sctx_) noexcept :
        sctx{ sctx_ },
        main_ctx{ false } {
    } 

    virtual ~activation_record() {
	}

    activation_record( activation_record const&) = delete;
    activation_record & operator=( activation_record const&) = delete;

    bool is_main_context() const noexcept {
        return main_ctx;
    }

    detail::activation_record * resume( void * vp) {
        data = vp;
		from = current();
        from->data = nullptr;
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        current() = this;
#if defined(BOOST_USE_SEGMENTED_STACKS)
        // adjust segmented stack properties
        __splitstack_getcontext( from->sctx.segments_ctx);
        __splitstack_setcontext( sctx.segments_ctx);
#endif
#if defined(BOOST_USE_ASAN)
        __sanitizer_start_switch_fiber( & fake_stack, stack_bottom, stack_size);
#endif
        // context switch from parent context to `this`-context
        ::swapcontext( & from->uctx, & uctx);
#if defined(BOOST_USE_ASAN)
         __sanitizer_finish_switch_fiber( current()->fake_stack, (const void **) & current()->stack_bottom,
                                          & current()->stack_size);
#endif
        return current()->from;
    }

    template< typename Ctx, typename Fn, typename Tpl >
    detail::activation_record * resume_with( Fn && fn, Tpl * tpl) {
        data = nullptr;
		from = current();
        from->data = nullptr;
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        // returned by continuation::current()
        current() = this;
        current()->ontop = [fn=std::forward<Fn>(fn),tpl,from=current()->from]() {
            current()->data = tpl;
            * tpl = helper< Tpl >::convert( fn( Ctx{ from } ) );
        };
#if defined(BOOST_USE_SEGMENTED_STACKS)
        // adjust segmented stack properties
        __splitstack_getcontext( from->sctx.segments_ctx);
        __splitstack_setcontext( sctx.segments_ctx);
#endif
#if defined(BOOST_USE_ASAN)
        __sanitizer_start_switch_fiber( & fake_stack, stack_bottom, stack_size);
#endif
        // context switch from parent context to `this`-context
        ::swapcontext( & from->uctx, & uctx);
#if defined(BOOST_USE_ASAN)
         __sanitizer_finish_switch_fiber( current()->fake_stack, (const void **) & current()->stack_bottom,
                                          & current()->stack_size);
#endif
        return current()->from;
    }

    template< typename Ctx, typename Fn >
    detail::activation_record * resume_with( Fn && fn) {
        data = nullptr;
		from = current();
        from->data = nullptr;
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        // returned by continuation::current()
        current() = this;
        current()->ontop = [fn=std::forward<Fn>(fn),from=current()->from]() {
            fn( Ctx{ from } );
            current()->data = nullptr;
        };
#if defined(BOOST_USE_SEGMENTED_STACKS)
        // adjust segmented stack properties
        __splitstack_getcontext( from->sctx.segments_ctx);
        __splitstack_setcontext( sctx.segments_ctx);
#endif
#if defined(BOOST_USE_ASAN)
        __sanitizer_start_switch_fiber( & fake_stack, stack_bottom, stack_size);
#endif
        // context switch from parent context to `this`-context
        ::swapcontext( & from->uctx, & uctx);
#if defined(BOOST_USE_ASAN)
         __sanitizer_finish_switch_fiber( current()->fake_stack, (const void **) & current()->stack_bottom,
                                          & current()->stack_size);
#endif
        return current()->from;
    }

    virtual void deallocate() noexcept {
    }
};

struct BOOST_CONTEXT_DECL activation_record_initializer {
    activation_record_initializer() noexcept;
    ~activation_record_initializer();
};

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
class capture_record : public activation_record {
private:
    typename std::decay< Fn >::type                     fn_;
    std::tuple< Arg ... >                               arg_;
    StackAlloc                                          salloc_;

    static void destroy( capture_record * p) noexcept {
        StackAlloc salloc = p->salloc_;
        stack_context sctx = p->sctx;
        // deallocate activation record
        p->~capture_record();
        // destroy stack with stack allocator
        salloc.deallocate( sctx);
    }

public:
    capture_record( stack_context sctx, StackAlloc const& salloc,
                    Fn && fn, Arg ... arg) noexcept :
        activation_record{ sctx },
        fn_( std::forward< Fn >( fn) ),
        arg_( std::forward< Arg >( arg) ... ),
        salloc_{ salloc } {
    }

    void deallocate() noexcept override final {
        BOOST_ASSERT( main_ctx || ( ! main_ctx && terminated) );
        destroy( this);
    }

    void run() {
        Ctx c{ from };
        auto tpl = std::tuple_cat(
                    std::forward_as_tuple( std::move( c) ),
                    std::move( arg_) );
        try {
            // invoke context-function
#if defined(BOOST_NO_CXX17_STD_APPLY)
            c = apply( std::move( fn_), std::move( tpl) );
#else
            c = std::apply( std::move( fn_), std::move( tpl) );
#endif  
        } catch ( forced_unwind const&) {
            BOOST_ASSERT( nullptr != current()->from);
            c = Ctx{ current()->from };
        }
        // this context has finished its task
        data = nullptr;
		from = nullptr;
        ontop = nullptr;
        terminated = true;
        force_unwind = false;
        c.resume();
        BOOST_ASSERT_MSG( false, "continuation already terminated");
    }
};

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
static activation_record * create_context( StackAlloc salloc, Fn && fn, Arg ... arg) {
    typedef capture_record< Ctx, StackAlloc, Fn, Arg ... >  capture_t;

    auto sctx = salloc.allocate();
    // reserve space for control structure
    void * storage = static_cast< char * >( sctx.sp) - sizeof( capture_t);
    // placment new for control structure on fast-context stack
    capture_t * record = new ( storage) capture_t{
            sctx, salloc, std::forward< Fn >( fn), std::forward< Arg >( arg) ... };
    // create user-context
    if ( 0 != ::getcontext( & record->uctx) ) {
        throw std::system_error(
                std::error_code( errno, std::system_category() ),
                "getcontext() failed");
    }
    record->uctx.uc_stack.ss_size = sctx.size - sizeof(capture_t) - 64;
    record->uctx.uc_stack.ss_sp = static_cast< char * >( sctx.sp) - sctx.size;
    record->uctx.uc_link = nullptr;
    ::makecontext( & record->uctx, ( void (*)() ) & detail::entry_func< capture_t >, 1, record);
#if defined(BOOST_USE_ASAN)
    record->stack_bottom = record->uctx.uc_stack.ss_sp;
    record->stack_size = record->uctx.uc_stack.ss_size;
#endif
    return record;
}

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
static activation_record * create_context( preallocated palloc, StackAlloc salloc,
                                                Fn && fn, Arg ... arg) {
    typedef capture_record< Ctx, StackAlloc, Fn, Arg ... >  capture_t; 

    // reserve space for control structure
    void * storage = static_cast< char * >( palloc.sp) - sizeof( capture_t);
    // placment new for control structure on fast-context stack
    capture_t * record = new ( storage) capture_t{
            palloc.sctx, salloc, std::forward< Fn >( fn), std::forward< Arg >( arg) ... };
    // create user-context
    if ( 0 != ::getcontext( & record->uctx) ) {
        throw std::system_error(
                std::error_code( errno, std::system_category() ),
                "getcontext() failed");
    }
    record->uctx.uc_stack.ss_size = palloc.size - sizeof(capture_t) - 64;
    record->uctx.uc_stack.ss_sp = static_cast< char * >( palloc.sctx.sp) - palloc.sctx.size;
    record->uctx.uc_link = nullptr;
    ::makecontext( & record->uctx,  ( void (*)() ) & detail::entry_func< capture_t >, 1, record);
#if defined(BOOST_USE_ASAN)
    record->stack_bottom = record->uctx.uc_stack.ss_sp;
    record->stack_size = record->uctx.uc_stack.ss_size;
#endif
    return record;
}

template< typename ... Arg >
struct result_type {
    typedef std::tuple< Arg ... >   type;

    static
    type get( void * data) {
        auto p = static_cast< std::tuple< Arg ... > * >( data);
        return std::move( * p);
    }
};

template< typename Arg >
struct result_type< Arg > {
    typedef Arg     type; 

    static
    type get( void * data) {
        auto p = static_cast< std::tuple< Arg > * >( data);
        return std::forward< Arg >( std::get< 0 >( * p) );
    }
};

}

class BOOST_CONTEXT_DECL continuation {
private:
    friend struct detail::activation_record;

    template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
    friend class detail::capture_record;

	template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
	friend detail::activation_record * detail::create_context( StackAlloc, Fn &&, Arg ...);

	template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
	friend detail::activation_record * detail::create_context( preallocated, StackAlloc, Fn &&, Arg ...);

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

    detail::activation_record   *   ptr_{ nullptr };

    continuation( detail::activation_record * ptr) noexcept :
        ptr_{ ptr } {
    }

public:
    continuation() = default;

    ~continuation() {
        if ( nullptr != ptr_ && ! ptr_->main_ctx) {
            if ( ! ptr_->terminated) {
                ptr_->force_unwind = true;
                ptr_->resume( nullptr);
                BOOST_ASSERT( ptr_->terminated);
            }
            ptr_->deallocate();
        }
    }

    continuation( continuation const&) = delete;
    continuation & operator=( continuation const&) = delete;

    continuation( continuation && other) noexcept :
        ptr_{ nullptr } {
        swap( other);
    }

    continuation & operator=( continuation && other) noexcept {
        if ( this == & other) return * this;
        continuation tmp{ std::move( other) };
        swap( tmp);
        return * this;
    }

    template< typename ... Arg >
    continuation resume( Arg ... arg) {
        auto tpl = std::make_tuple( std::forward< Arg >( arg) ... );
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::activation_record * ptr = detail::exchange( ptr_, nullptr)->resume( & tpl);
#else
        detail::activation_record * ptr = std::exchange( ptr_, nullptr)->resume( & tpl);
#endif
        if ( detail::activation_record::current()->force_unwind) {
            BOOST_ASSERT( nullptr != detail::activation_record::current()->from);
            throw detail::forced_unwind{};
        } else if ( detail::activation_record::current()->ontop) {
            detail::activation_record::current()->ontop();
            detail::activation_record::current()->ontop = nullptr;
        }
        return continuation{ ptr };
    }
    
    template< typename Fn, typename ... Arg >
    continuation resume_with( Fn && fn, Arg ... arg) {
        auto tpl = std::make_tuple( std::forward< Arg >( arg) ... );
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::activation_record * ptr =
            detail::exchange( ptr_, nullptr)->resume_with< continuation >( std::forward< Fn >( fn), & tpl);
#else
        detail::activation_record * ptr =
            std::exchange( ptr_, nullptr)->resume_with< continuation >( std::forward< Fn >( fn), & tpl);
#endif
        if ( detail::activation_record::current()->force_unwind) {
            BOOST_ASSERT( nullptr != detail::activation_record::current()->from);
            throw detail::forced_unwind{};
        } else if ( detail::activation_record::current()->ontop) {
            detail::activation_record::current()->ontop();
            detail::activation_record::current()->ontop = nullptr;
        }
        return continuation{ ptr };
    }

    continuation resume() {
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        activation_record * ptr = detail::exchange( ptr_, nullptr)->resume( nullptr);
#else
        detail::activation_record * ptr = std::exchange( ptr_, nullptr)->resume( nullptr);
#endif
        if ( detail::activation_record::current()->force_unwind) {
            BOOST_ASSERT( nullptr != detail::activation_record::current()->from);
            throw detail::forced_unwind{};
        } else if ( detail::activation_record::current()->ontop) {
            detail::activation_record::current()->ontop();
            detail::activation_record::current()->ontop = nullptr;
        }
        return continuation{ ptr };
    }

    template< typename Fn >
    continuation resume_with( Fn && fn) {
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::activation_record * ptr =
            detal::exchange( ptr_, nullptr)->resume_with< continuation >( std::forward< Fn >( fn) );
#else
        detail::activation_record * ptr =
            std::exchange( ptr_, nullptr)->resume_with< continuation >( std::forward< Fn >( fn) );
#endif
        if ( detail::activation_record::current()->force_unwind) {
            BOOST_ASSERT( nullptr != detail::activation_record::current()->from);
            throw detail::forced_unwind{};
        } else if ( detail::activation_record::current()->ontop) {
            detail::activation_record::current()->ontop();
            detail::activation_record::current()->ontop = nullptr;
        }
        return continuation{ ptr };
    }

    bool data_available() noexcept {
        return * this && nullptr != detail::activation_record::current()->data;
    }

    template< typename ... Arg >
    typename detail::result_type< Arg ... >::type get_data() {
        BOOST_ASSERT( data_available() );;
        return detail::result_type< Arg ... >::get( detail::activation_record::current()->data);
    }

    explicit operator bool() const noexcept {
        return nullptr != ptr_ && ! ptr_->terminated;
    }

    bool operator!() const noexcept {
        return nullptr == ptr_ || ptr_->terminated;
    }

    bool operator==( continuation const& other) const noexcept {
        return ptr_ == other.ptr_;
    }

    bool operator!=( continuation const& other) const noexcept {
        return ptr_ != other.ptr_;
    }

    bool operator<( continuation const& other) const noexcept {
        return ptr_ < other.ptr_;
    }

    bool operator>( continuation const& other) const noexcept {
        return other.ptr_ < ptr_;
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
        if ( nullptr != other.ptr_) {
            return os << other.ptr_;
        } else {
            return os << "{not-a-context}";
        }
    }

    void swap( continuation & other) noexcept {
        std::swap( ptr_, other.ptr_);
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
			std::allocator_arg,
#if defined(BOOST_USE_SEGMENTED_STACKS)
			segmented_stack(),
#else
			fixedsize_stack(),
#endif
			std::forward< Fn >( fn), std::forward< Arg >( arg) ...);
}

template<
    typename StackAlloc,
    typename Fn,
    typename ... Arg
>
continuation
callcc( std::allocator_arg_t, StackAlloc salloc, Fn && fn, Arg ... arg) {
	return continuation{
		detail::create_context< continuation >(
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
	return continuation{
		detail::create_context< continuation >(
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
			std::allocator_arg,
#if defined(BOOST_USE_SEGMENTED_STACKS)
			segmented_stack(),
#else
			fixedsize_stack(),
#endif
			std::forward< Fn >( fn) );
}

template< typename StackAlloc, typename Fn >
continuation
callcc( std::allocator_arg_t, StackAlloc salloc, Fn && fn) {
	return continuation{
		detail::create_context< continuation >(
				salloc, std::forward< Fn >( fn) ) }.resume();
}

template< typename StackAlloc, typename Fn >
continuation
callcc( std::allocator_arg_t, preallocated palloc, StackAlloc salloc, Fn && fn) {
	return continuation{
		detail::create_context< continuation >(
				palloc, salloc, std::forward< Fn >( fn) ) }.resume();
}

inline
void swap( continuation & l, continuation & r) noexcept {
    l.swap( r);
}

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_CONTINUATION_H
