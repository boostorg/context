
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2017,2021 Oliver Kowalke (oliver dot kowalke at gmail dot com)
// Copyright (c) 2019 Casey Bodley (cbodley at redhat dot com)
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_IMPL_SPAWN_H
#define BOOST_CONTEXT_IMPL_SPAWN_H

#include <atomic>
#include <memory>
#include <tuple>

#include <boost/optional.hpp>
#include <boost/system/system_error.hpp>

#include <boost/context/detail/net.hpp>
#include <boost/context/detail/is_stack_allocator.hpp>
#include <boost/context/fiber.hpp>

namespace boost {
namespace context {
namespace detail {

class spawn_context {
public:
    fiber_context       ctx_;
    std::exception_ptr  eptr_{};

    spawn_context() = default;

    template< typename StackAlloc, typename Fn >
    spawn_context( std::allocator_arg_t, StackAlloc && salloc, Fn && fn) :
            ctx_{
                std::allocator_arg,
                std::forward< StackAlloc >( salloc),
                std::forward< Fn >( fn) } {
    }

    void resume() {
        ctx_ = std::move( ctx_).resume();
        if ( eptr_) {
            std::rethrow_exception( std::move( eptr_) );
        }
    }
};

template< typename Handler, typename ...Ts >
class coro_handler {
public:
    coro_handler( basic_yield_context< Handler > ctx) :
        callee_{ ctx.callee_.lock() },
        caller_{ ctx.caller_ },
        handler_{ ctx.handler_ },
        ready_{ 0 },
        ec_{ ctx.ec_ },
        value_{ 0 } {
    }

    void operator()( Ts... values) {
        *ec_ = boost::system::error_code{};
        *value_ = std::forward_as_tuple( std::move( values) ...);
        if ( --*ready_ == 0) {
            callee_->resume();
        }
    }

    void operator()( boost::system::error_code ec, Ts... values) {
        *ec_ = ec;
        *value_ = std::forward_as_tuple( std::move( values) ...);
        if ( --*ready_ == 0) {
            callee_->resume();
        }
    }

//private:
    std::shared_ptr< spawn_context >            callee_;
    spawn_context    &                          caller_;
    Handler                                     handler_;
    std::atomic< long > *                       ready_;
    boost::system::error_code *                 ec_;
    boost::optional< std::tuple< Ts... > > *    value_;
};

template< typename Handler, typename T >
class coro_handler< Handler, T > {
public:
    coro_handler( basic_yield_context< Handler > ctx) :
        callee_{ ctx.callee_.lock() },
        caller_{ ctx.caller_ },
        handler_{ ctx.handler_ },
        ready_{ 0 },
        ec_{ ctx.ec_ },
        value_{ 0 } {
    }

    void operator()( T value) {
        *ec_ = boost::system::error_code();
        *value_ = std::move( value);
        if ( --*ready_ == 0) {
            callee_->resume();
        }
    }

    void operator()( boost::system::error_code ec, T value) {
        *ec_ = ec;
        *value_ = std::move( value);
        if ( --*ready_ == 0) {
            callee_->resume();
        }
    }

//private:
    std::shared_ptr< spawn_context >    callee_;
    spawn_context    &                  caller_;
    Handler                             handler_;
    std::atomic< long > *               ready_;
    boost::system::error_code *         ec_;
    boost::optional< T > *              value_;
};

template< typename Handler >
class coro_handler< Handler, void > {
public:
    coro_handler( basic_yield_context< Handler > ctx) :
        callee_{ ctx.callee_.lock() },
        caller_{ ctx.caller_ },
        handler_{ ctx.handler_ },
        ready_{ 0 },
        ec_{ ctx.ec_ } {
    }

    void operator()() {
        *ec_ = boost::system::error_code();
        if ( --*ready_ == 0) {
            callee_->resume();
        }
    }

    void operator()( boost::system::error_code ec) {
        *ec_ = ec;
        if ( --*ready_ == 0) {
            callee_->resume();
        }
    }

//private:
    std::shared_ptr< spawn_context >    callee_;
    spawn_context    &                  caller_;
    Handler                             handler_;
    std::atomic< long > *               ready_;
    boost::system::error_code *         ec_;
};

template< typename Handler, typename ...Ts >
class coro_async_result {
public:
    using completion_handler_type = coro_handler< Handler, Ts... >;
    using return_type = std::tuple< Ts... >;

    explicit coro_async_result( completion_handler_type & h) :
            handler_{ h },
            caller_{ h.caller_ },
            ready_{ 2 } {
        h.ready_ = & ready_;
        out_ec_ = h.ec_;
        if ( ! out_ec_) {
            h.ec_ = & ec_;
        }
        h.value_ = & value_;
    }

    return_type get() {
        // Must not hold shared_ptr while suspended.
        handler_.callee_.reset();
        if ( --ready_ != 0) {
            caller_.resume(); // suspend caller
        }
        if ( ! out_ec_ && ec_) {
            throw boost::system::system_error( ec_);
        }
        return std::move( * value_);
    }

private:
    completion_handler_type &       handler_;
    spawn_context    &               caller_;
    std::atomic< long >             ready_;
    boost::system::error_code *     out_ec_;
    boost::system::error_code       ec_;
    boost::optional< return_type >  value_;
};

template< typename Handler, typename T >
class coro_async_result< Handler, T > {
public:
    using completion_handler_type = coro_handler< Handler, T >;
    using return_type = T;

    explicit coro_async_result( completion_handler_type & h) :
            handler_{ h },
            caller_{ h.caller_ },
            ready_{ 2 } {
        h.ready_ = & ready_;
        out_ec_ = h.ec_;
        if ( ! out_ec_) {
            h.ec_ = & ec_;
        }
        h.value_ = & value_;
    }

    return_type get() {
        // Must not hold shared_ptr while suspended.
        handler_.callee_.reset();
        if ( --ready_ != 0) {
            caller_.resume(); // suspend caller
        }
        if ( ! out_ec_ && ec_) {
            throw boost::system::system_error( ec_);
        }
        return std::move( * value_);
    }

private:
    completion_handler_type &       handler_;
    spawn_context    &              caller_;
    std::atomic< long >             ready_;
    boost::system::error_code *     out_ec_;
    boost::system::error_code       ec_;
    boost::optional< return_type >  value_;
};

template< typename Handler >
class coro_async_result< Handler, void > {
public:
    using completion_handler_type = coro_handler< Handler, void >;
    using return_type = void;

    explicit coro_async_result( completion_handler_type & h) :
            handler_{ h },
            caller_{ h.caller_ },
            ready_{ 2 } {
        h.ready_ = & ready_;
        out_ec_ = h.ec_;
        if ( ! out_ec_) {
            h.ec_ = & ec_;
        }
    }

    void get() {
        // Must not hold shared_ptr while suspended.
        handler_.callee_.reset();
        if ( --ready_ != 0) {
            caller_.resume(); // suspend caller
        }
        if ( ! out_ec_ && ec_) {
            throw boost::system::system_error( ec_);
        }
    }

private:
    completion_handler_type &   handler_;
    spawn_context    &          caller_;
    std::atomic< long >         ready_;
    boost::system::error_code * out_ec_;
    boost::system::error_code   ec_;
};

} // namespace detail
} // namespace context

#if !defined(GENERATING_DOCUMENTATION)

template< typename Handler, typename ReturnType >
class CONTEXT_NET_NAMESPACE::async_result< context::basic_yield_context< Handler >, ReturnType() > :
    public context::detail::coro_async_result< Handler, void > {
public:
    explicit async_result(
            typename context::detail::coro_async_result< Handler, void >::completion_handler_type & h) :
        context::detail::coro_async_result< Handler, void >{ h } {
    }
};

template< typename Handler, typename ReturnType, typename ...Args >
class CONTEXT_NET_NAMESPACE::async_result< context::basic_yield_context< Handler >, ReturnType( Args...) > :
    public context::detail::coro_async_result< Handler, typename std::decay< Args >::type... > {
public:
    explicit async_result(
            typename context::detail::coro_async_result< Handler, typename std::decay< Args >::type... >::completion_handler_type & h) :
        context::detail::coro_async_result< Handler, typename std::decay< Args >::type... >{ h } {
    }
};

template< typename Handler, typename ReturnType >
class CONTEXT_NET_NAMESPACE::async_result< context::basic_yield_context< Handler >, ReturnType( boost::system::error_code) > :
    public context::detail::coro_async_result< Handler, void > {
public:
    explicit async_result(
            typename context::detail::coro_async_result< Handler, void>::completion_handler_type & h) :
        context::detail::coro_async_result< Handler, void >{ h } {
    }
};

template< typename Handler, typename ReturnType, typename ...Args >
class CONTEXT_NET_NAMESPACE::async_result< context::basic_yield_context< Handler >, ReturnType( boost::system::error_code, Args...) > :
    public context::detail::coro_async_result< Handler, typename std::decay< Args >::type... > {
public:
    explicit async_result(
            typename context::detail::coro_async_result< Handler, typename std::decay< Args >::type...>::completion_handler_type & h) :
        context::detail::coro_async_result< Handler, typename std::decay< Args >::type... >{ h } {
    }
};

template< typename Handler, typename Allocator, typename ...Ts >
struct CONTEXT_NET_NAMESPACE::associated_allocator< context::detail::coro_handler< Handler, Ts... >, Allocator > {
    using type = associated_allocator_t< Handler, Allocator >;

    static type get( context::detail::coro_handler< Handler, Ts... > const& h, Allocator const& a = Allocator{} ) noexcept {
        return associated_allocator< Handler, Allocator >::get( h.handler_, a);
    }
};

template< typename Handler, typename Executor, typename ...Ts >
struct CONTEXT_NET_NAMESPACE::associated_executor< context::detail::coro_handler< Handler, Ts... >, Executor> {
    using type = associated_executor_t< Handler, Executor >;

    static type get( context::detail::coro_handler< Handler, Ts... > const& h, Executor const& ex = Executor{} ) noexcept {
        return associated_executor< Handler, Executor >::get( h.handler_, ex);
    }
};

namespace context {
namespace detail {

template< typename Handler, typename Function, typename StackAllocator >
struct spawn_data {
    template< typename Hand, typename Func, typename Stack >
    spawn_data( Hand && handler, bool call_handler, Func && function, Stack && salloc) :
        handler_{ std::forward< Hand >( handler) },
        call_handler_{ call_handler },
        function_{ std::forward< Func >( function) },
        salloc_{ std::forward< Stack >( salloc) } {
    }

    spawn_data( spawn_data const&) = delete;
    spawn_data & operator=( spawn_data const&) = delete;

    Handler         handler_;
    bool            call_handler_;
    Function        function_;
    StackAllocator  salloc_;
    spawn_context   caller_;
};

template< typename Handler, typename Function, typename StackAllocator >
struct spawn_helper {
    void operator()() {
        callee_.reset(
            new spawn_context{
                std::allocator_arg,
                std::move( data_->salloc_),
                [this] (fiber_context && f) {
                    std::shared_ptr< spawn_data< Handler, Function, StackAllocator > > data = data_;
                    data->caller_.ctx_ = std::move( f);
                    const basic_yield_context< Handler > yh{ callee_, data->caller_, data->handler_ };
                    try {
                        ( data->function_)( yh);
                        if ( data->call_handler_) {
                            ( data->handler_)();
                        }
                    } catch ( boost::context::detail::forced_unwind const& e) {
                        throw; // must allow forced_unwind to propagate
                    } catch (...) {
                        auto callee = yh.callee_.lock();
                        if ( callee) {
                            callee->eptr_ = std::current_exception();
                        }
                    }
                    fiber_context caller = std::move( data->caller_.ctx_);
                    data.reset();
                    return caller;
                } } );
        callee_->ctx_ = std::move( callee_->ctx_).resume();
        if ( callee_->eptr_) {
            std::rethrow_exception( std::move( callee_->eptr_) );
        }
    }

    std::shared_ptr< spawn_context >                                    callee_;
    std::shared_ptr< spawn_data< Handler, Function, StackAllocator > >  data_;
};

inline
void default_spawn_handler() {
}

} // namespace detail

template< typename Function, typename StackAllocator >
auto spawn( Function && function, StackAllocator && salloc)
    -> typename std::enable_if<
            context::detail::is_stack_allocator< typename std::decay< StackAllocator >::type >::value
        >::type {
    auto ex = context::detail::net::get_associated_executor( function);
    spawn( ex, std::forward< Function >( function), std::forward< StackAllocator >( salloc) );
}

template< typename Handler, typename Function, typename StackAllocator >
auto spawn( Handler && handler, Function && function, StackAllocator && salloc)
    -> typename std::enable_if<
            ! context::detail::net::is_executor< typename std::decay< Handler >::type >::value &&
            ! std::is_convertible< Handler &, context::detail::net::execution_context & >::value &&
            ! context::detail::is_stack_allocator< typename std::decay< Function >::type >::value &&
            context::detail::is_stack_allocator< typename std::decay< StackAllocator >::type >::value
        >::type {
    using handler_type = typename std::decay< Handler >::type;
    using function_type = typename std::decay< Function >::type;

    auto ex = context::detail::net::get_associated_executor( handler);
    auto a = context::detail::net::get_associated_allocator( handler);
    context::detail::spawn_helper< handler_type, function_type, StackAllocator > helper;
    helper.data_ = std::make_shared<
        context::detail::spawn_data< handler_type, function_type, StackAllocator > >(
                std::forward< Handler >( handler), true,
                std::forward< Function >( function),
                std::forward< StackAllocator >( salloc) );
    ex.dispatch( helper, a);
}

template< typename Handler, typename Function, typename StackAllocator >
auto spawn( context::basic_yield_context< Handler > ctx, Function && function, StackAllocator && salloc)
    -> typename std::enable_if<
            context::detail::is_stack_allocator< typename std::decay< StackAllocator >::type >::value
        >::type {
    using function_type = typename std::decay< Function >::type;

    Handler handler{ ctx.handler_ }; // Explicit copy that might be moved from.
    auto ex = context::detail::net::get_associated_executor( handler);
    auto a = context::detail::net::get_associated_allocator( handler);
    context::detail::spawn_helper< Handler, function_type, StackAllocator > helper;
    helper.data_ = std::make_shared<
        context::detail::spawn_data< Handler, function_type, StackAllocator > >(
                std::forward< Handler >( handler), false,
                std::forward< Function >( function),
                std::forward< StackAllocator >( salloc) );
    ex.dispatch( helper, a);
}

template< typename Function, typename Executor, typename StackAllocator >
auto spawn( Executor const& ex, Function && function, StackAllocator && salloc)
    -> typename std::enable_if<
            context::detail::net::is_executor< Executor >::value &&
            context::detail::is_stack_allocator< typename std::decay< StackAllocator >::type >::value
        >::type {
    spawn( context::detail::net::strand< Executor >{ ex },
            std::forward< Function >( function),
            std::forward< StackAllocator >( salloc) );
}

template< typename Function, typename Executor, typename StackAllocator >
auto spawn( context::detail::net::strand< Executor > const& ex, Function && function, StackAllocator && salloc)
    -> typename std::enable_if<
            context::detail::is_stack_allocator< typename std::decay< StackAllocator >::type >::value
        >::type {
    spawn( bind_executor( ex, & context::detail::default_spawn_handler),
            std::forward< Function >( function),
            std::forward< StackAllocator >( salloc) );
}

template< typename Function, typename ExecutionContext, typename StackAllocator >
auto spawn( ExecutionContext & ctx, Function && function, StackAllocator && salloc)
    -> typename std::enable_if<
            std::is_convertible< ExecutionContext &, context::detail::net::execution_context & >::value &&
            context::detail::is_stack_allocator< typename std::decay< StackAllocator >::type >::value
        >::type {
    spawn( ctx.get_executor(),
            std::forward< Function >( function),
            std::forward< StackAllocator >( salloc) );
}

#endif // !defined(GENERATING_DOCUMENTATION)

}}

#endif // BOOST_CONTEXT_IMPL_SPAWN_H
