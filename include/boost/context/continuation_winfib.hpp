
//          Copyright Oliver Kowalke 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_CONTINUATION_H
#define BOOST_CONTEXT_CONTINUATION_H

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

#if defined(BOOST_NO_CXX17_STD_APPLY)
#include <boost/context/detail/apply.hpp>
#endif
#include <boost/context/detail/disable_overload.hpp>
#include <boost/context/detail/exception.hpp>
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
#include <boost/context/detail/exchange.hpp>
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
static VOID WINAPI entry_func( LPVOID data) noexcept {
    Record * record = static_cast< Record * >( data);
    BOOST_ASSERT( nullptr != record);
    // start execution of toplevel context-function
    record->run();
}

struct BOOST_CONTEXT_DECL activation_record {
    thread_local static activation_record   *   current_rec;

    LPVOID                      fiber{ nullptr };
    stack_context               sctx{};
    bool                        main_ctx{ true };
    void                    *   data{ nullptr };
    activation_record       *   from{ nullptr };
    std::function< void() >     ontop{};
    bool                        terminated{ false };
    bool                        force_unwind{ false };

    static activation_record *& current() noexcept;

    // used for toplevel-context
    // (e.g. main context, thread-entry context)
    activation_record() noexcept {
#if ( _WIN32_WINNT > 0x0600)
        if ( ::IsThreadAFiber() ) {
            fiber = ::GetCurrentFiber();
        } else {
            fiber = ::ConvertThreadToFiber( nullptr);
        }
#else
        fiber = ::ConvertThreadToFiber( nullptr);
        if ( BOOST_UNLIKELY( nullptr == fiber) ) {
            DWORD err = ::GetLastError();
            BOOST_ASSERT( ERROR_ALREADY_FIBER == err);
            fiber = ::GetCurrentFiber(); 
            BOOST_ASSERT( nullptr != fiber);
            BOOST_ASSERT( reinterpret_cast< LPVOID >( 0x1E00) != fiber);
        }
#endif
    }

    activation_record( stack_context sctx_) noexcept :
        sctx{ sctx_ },
        main_ctx{ false } {
    } 

    virtual ~activation_record() {
        if ( BOOST_UNLIKELY( main_ctx) ) {
            ::ConvertFiberToThread();
        } else {
            ::DeleteFiber( fiber);
        }
    }

    activation_record( activation_record const&) = delete;
    activation_record & operator=( activation_record const&) = delete;

    bool is_main_context() const noexcept {
        return main_ctx;
    }

    activation_record * resume( void * vp) {
        data = vp;
        from = current();
        from->data = nullptr;
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        current() = this;
        // context switch from parent context to `this`-context
        // context switch
        ::SwitchToFiber( fiber);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return detail::exchange( current()->from, nullptr);
#else
        return std::exchange( current()->from, nullptr);
#endif
    }

    template< typename Ctx, typename Fn, typename Tpl >
    activation_record * resume_with( Fn && fn, Tpl * tpl) {
        data = nullptr;
        from = current();
        from->data = nullptr;
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        // returned by continuation::current()
        current() = this;
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
        auto from_ = current()->from;
        current()->ontop = std::bind(
                [tpl,from_](typename std::decay< Fn >::type & fn){
                    current()->data = tpl;
                    * tpl = helper< Tpl >::convert( fn( Ctx{ from_ } ) );
                },
                std::forward< Fn >( fn) );
#else
        current()->ontop = [fn=std::forward<Fn>(fn),tpl,from=current()->from]() {
            current()->data = tpl;
            * tpl = helper< Tpl >::convert( fn( Ctx{ from } ) );
        };
#endif
        // context switch
        ::SwitchToFiber( fiber);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return detail::exchange( current()->from, nullptr);
#else
        return std::exchange( current()->from, nullptr);
#endif
    }

    template< typename Ctx, typename Fn >
    activation_record * resume_with( Fn && fn) {
        data = nullptr;
        from = current();
        from->data = nullptr;
        // store `this` in static, thread local pointer
        // `this` will become the active (running) context
        // returned by continuation::current()
        current() = this;
#if defined(BOOST_NO_CXX14_GENERIC_LAMBDAS)
        auto from_ = current()->from;
        current()->ontop = std::bind(
                [from_](typename std::decay< Fn >::type & fn){
                    fn( Ctx{ from_ } );
                    current()->data = nullptr;
                },
                std::forward< Fn >( fn) );
#else
        current()->ontop = [fn=std::forward<Fn>(fn),from=current()->from]() {
            fn( Ctx{ from } );
            current()->data = nullptr;
        };
#endif
        // context switch
        ::SwitchToFiber( fiber);
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        return detail::exchange( current()->from, nullptr);
#else
        return std::exchange( current()->from, nullptr);
#endif
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
    StackAlloc                                          salloc_;
    typename std::decay< Fn >::type                     fn_;
    std::tuple< Arg ... >                               arg_;

    static void destroy( capture_record * p) noexcept {
        StackAlloc salloc = p->salloc_;
        stack_context sctx = p->sctx;
        // deallocate activation record
        p->~capture_record();
        // destroy stack with stack allocator
        salloc.deallocate( sctx);
    }

public:
    capture_record( stack_context sctx, StackAlloc salloc,
                    Fn && fn, Arg ... arg) noexcept :
        activation_record( sctx),
        salloc_( salloc),
        fn_( std::forward< Fn >( fn) ),
        arg_( std::forward< Arg >( arg) ... ) {
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
        } catch ( forced_unwind & ex) {
            ex.caught = true;
            c = Ctx{ ex.from };
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
static activation_record * create_context1( StackAlloc salloc, Fn && fn, Arg ... arg) {
    typedef capture_record< Ctx, StackAlloc, Fn, Arg ... >  capture_t;

    auto sctx = salloc.allocate();
    BOOST_ASSERT( ( sizeof( capture_t) ) < sctx.size);
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( sctx.sp) - static_cast< uintptr_t >( sizeof( capture_t) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    capture_t * record = new ( storage) capture_t{
            sctx, salloc, std::forward< Fn >( fn), std::forward< Arg >( arg) ... };
    // create user-context
    record->fiber = ::CreateFiber( sctx.size, & detail::entry_func< capture_t >, record);
    return record;
}

template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
static activation_record * create_context2( preallocated palloc, StackAlloc salloc,
                                                Fn && fn, Arg ... arg) {
    typedef capture_record< Ctx, StackAlloc, Fn, Arg ... >  capture_t; 

    BOOST_ASSERT( ( sizeof( capture_t) ) < palloc.size);
    // reserve space for control structure
    void * storage = reinterpret_cast< void * >(
            ( reinterpret_cast< uintptr_t >( palloc.sp) - static_cast< uintptr_t >( sizeof( capture_t) ) )
            & ~ static_cast< uintptr_t >( 0xff) );
    // placment new for control structure on context stack
    capture_t * record = new ( storage) capture_t{
            palloc.sctx, salloc, std::forward< Fn >( fn), std::forward< Arg >( arg) ... };
    // create user-context
    record->fiber = ::CreateFiber( palloc.sctx.size, & detail::entry_func< capture_t >, record);
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
    friend detail::activation_record * detail::create_context1( StackAlloc, Fn &&, Arg ...);

    template< typename Ctx, typename StackAlloc, typename Fn, typename ... Arg >
    friend detail::activation_record * detail::create_context2( preallocated, StackAlloc, Fn &&, Arg ...);

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
        if ( BOOST_UNLIKELY( nullptr != ptr_) && ! ptr_->main_ctx) {
            if ( BOOST_LIKELY( ! ptr_->terminated) ) {
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
        if ( BOOST_UNLIKELY( this == & other) ) return * this;
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
        if ( BOOST_UNLIKELY( detail::activation_record::current()->force_unwind) ) {
            throw detail::forced_unwind{ ptr};
        } else if ( BOOST_UNLIKELY( nullptr != detail::activation_record::current()->ontop) ) {
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
        if ( BOOST_UNLIKELY( detail::activation_record::current()->force_unwind) ) {
            throw detail::forced_unwind{ ptr};
        } else if ( BOOST_UNLIKELY( nullptr != detail::activation_record::current()->ontop) ) {
            detail::activation_record::current()->ontop();
            detail::activation_record::current()->ontop = nullptr;
        }
        return continuation{ ptr };
    }

    continuation resume() {
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::activation_record * ptr = detail::exchange( ptr_, nullptr)->resume( nullptr);
#else
        detail::activation_record * ptr = std::exchange( ptr_, nullptr)->resume( nullptr);
#endif
        if ( BOOST_UNLIKELY( detail::activation_record::current()->force_unwind) ) {
            throw detail::forced_unwind{ ptr};
        } else if ( BOOST_UNLIKELY( nullptr != detail::activation_record::current()->ontop) ) {
            detail::activation_record::current()->ontop();
            detail::activation_record::current()->ontop = nullptr;
        }
        return continuation{ ptr };
    }

    template< typename Fn >
    continuation resume_with( Fn && fn) {
#if defined(BOOST_NO_CXX14_STD_EXCHANGE)
        detail::activation_record * ptr =
            detail::exchange( ptr_, nullptr)->resume_with< continuation >( std::forward< Fn >( fn) );
#else
        detail::activation_record * ptr =
            std::exchange( ptr_, nullptr)->resume_with< continuation >( std::forward< Fn >( fn) );
#endif
        if ( BOOST_UNLIKELY( detail::activation_record::current()->force_unwind) ) {
            throw detail::forced_unwind{ ptr};
        } else if ( BOOST_UNLIKELY( nullptr != detail::activation_record::current()->ontop) ) {
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
            fixedsize_stack(),
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
        detail::create_context1< continuation >(
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
        detail::create_context2< continuation >(
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
            fixedsize_stack(),
            std::forward< Fn >( fn) );
}

template< typename StackAlloc, typename Fn >
continuation
callcc( std::allocator_arg_t, StackAlloc salloc, Fn && fn) {
    return continuation{
        detail::create_context1< continuation >(
                salloc, std::forward< Fn >( fn) ) }.resume();
}

template< typename StackAlloc, typename Fn >
continuation
callcc( std::allocator_arg_t, preallocated palloc, StackAlloc salloc, Fn && fn) {
    return continuation{
        detail::create_context2< continuation >(
                palloc, salloc, std::forward< Fn >( fn) ) }.resume();
}

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
