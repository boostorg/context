
// Copyright (c) 2003-2019 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2019 Casey Bodley (cbodley at redhat dot com)
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <boost/context/spawn.hpp>

#include <boost/asio/io_context.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>

// make assertions about async_result::return_type with different signatures
// this is a compilation test only

template< typename Sig>
struct yield_result : boost::asio::async_result<boost::context::yield_context, Sig> {};

template< typename T, typename Sig>
struct yield_returns : std::is_same<T, typename yield_result<Sig>::return_type> {};

// no return value
static_assert(yield_returns< void, void() >::value,
              "wrong return value for void()");
static_assert(yield_returns< void, void(boost::system::error_code) >::value,
              "wrong return value for void(error_code)");
// single-parameter return value
static_assert(yield_returns<int, void(int) >::value,
              "wrong return value for void(int)");
static_assert(yield_returns<int, void(boost::system::error_code, int) >::value,
              "wrong return value for void(error_code, int)");
// multiple-parameter return value
static_assert(yield_returns<std::tuple<int, std::string>,
                            void(int, std::string) >::value,
              "wrong return value for void(int, string)");
static_assert(yield_returns<std::tuple<int, std::string>,
                            void(boost::system::error_code, int, std::string) >::value,
              "wrong return value for void(error_code, int, string)");
// single-tuple-parameter return value
static_assert(yield_returns<std::tuple<int, std::string>,
                            void(std::tuple<int, std::string>) >::value,
              "wrong return value for void(std::tuple<int>)");
static_assert(yield_returns<std::tuple<int, std::string>,
                            void(boost::system::error_code, std::tuple<int, std::string>) >::value,
              "wrong return value for void(error_code, std::tuple<int>)");
// single-pair-parameter return value
static_assert(yield_returns<std::pair<int, std::string>,
                            void(std::pair<int, std::string>) >::value,
              "wrong return value for void(std::tuple<int>)");
static_assert(yield_returns<std::pair<int, std::string>,
                            void(boost::system::error_code, std::pair<int, std::string>) >::value,
              "wrong return value for void(error_code, std::tuple<int>)");

boost::context::protected_fixedsize_stack with_stack_allocator() {
    return boost::context::protected_fixedsize_stack{ 65536 };
}

struct counting_handler {
    int &   count;

    counting_handler( int & count) :
        count(count) {
    }

    void operator()() {
      ++count;
    }

    template< typename T >
    void operator()( boost::context::basic_yield_context< T >) {
        ++count;
    }
};

void spawnFunction() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn( counting_handler{ called } );
    BOOST_CHECK_EQUAL(0, ioc.run()); // runs in system executor
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnBoundFunction() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn( bind_executor( ioc.get_executor(), counting_handler{ called } ) );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnFunctionStackAllocator() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn(
            counting_handler{ called },
            with_stack_allocator() );
    BOOST_CHECK_EQUAL(0, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnHandler() {
    boost::asio::io_context ioc;
    boost::asio::strand< boost::asio::io_context::executor_type > strand{ ioc.get_executor() };
    int called = 0;
    boost::context::spawn(
            bind_executor( strand, counting_handler{ called } ),
            counting_handler{ called } );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(2, called);
}

void spawnHandlerStackAllocator() {
    boost::asio::io_context ioc;
    typedef boost::asio::io_context::executor_type executor_type;
    boost::asio::strand< executor_type > strand{ ioc.get_executor() };
    int called = 0;
    boost::context::spawn(
            bind_executor( strand, counting_handler{ called } ),
            counting_handler{ called },
            with_stack_allocator() );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(2, called);
}

struct spawn_counting_handler {
    int &   count;

    spawn_counting_handler( int & count) :
        count{ count } {
    }

    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        boost::context::spawn( y, counting_handler{ count } );
        ++count;
    }
};

void spawnYieldContext() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn(
            bind_executor( ioc.get_executor(), counting_handler{ called } ),
            spawn_counting_handler{ called } );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(3, called);
}

struct spawn_alloc_counting_handler {
    int &   count;

    spawn_alloc_counting_handler( int & count) :
        count{ count } {
    }

    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        boost::context::spawn(
                y,
                counting_handler{ count },
                with_stack_allocator() );
        ++count;
    }
};

void spawnYieldContextStackAllocator() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn(
            bind_executor( ioc.get_executor(), counting_handler{ called } ),
            spawn_alloc_counting_handler{ called } );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(3, called);
}

void spawnExecutor() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn( ioc.get_executor(), counting_handler{ called } );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnExecutorStackAllocator() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn(
            ioc.get_executor(),
            counting_handler{ called },
            with_stack_allocator() );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnStrand() {
    boost::asio::io_context ioc;
    typedef boost::asio::io_context::executor_type executor_type;
    int called = 0;
    boost::context::spawn(
            boost::asio::strand< executor_type >{ ioc.get_executor() },
            counting_handler{ called } );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnStrandStackAllocator() {
    boost::asio::io_context ioc;
    typedef boost::asio::io_context::executor_type executor_type;
    int called = 0;
    boost::context::spawn(
            boost::asio::strand< executor_type >{ ioc.get_executor() },
            counting_handler{ called },
            with_stack_allocator() );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnExecutionContext() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn( ioc, counting_handler{ called } );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

void spawnExecutionContextStackAllocator() {
    boost::asio::io_context ioc;
    int called = 0;
    boost::context::spawn(
            ioc,
            counting_handler{ called },
            with_stack_allocator() );
    BOOST_CHECK_EQUAL(1, ioc.run());
    BOOST_CHECK(ioc.stopped());
    BOOST_CHECK_EQUAL(1, called);
}

typedef boost::asio::system_timer timer_type;

struct spawn_wait_handler {
    timer_type &    timer;

    spawn_wait_handler( timer_type & timer) :
        timer{ timer } {
    }

    template< typename T >
    void operator()( boost::context::basic_yield_context< T > yield) {
        timer.async_wait( yield);
    }
};

void spawnTimer() {
    int called = 0;
    {
        boost::asio::io_context ioc;
        timer_type timer{ ioc, boost::asio::chrono::hours{ 0 } };
        boost::context::spawn(
                bind_executor( ioc.get_executor(), counting_handler{ called } ),
                spawn_wait_handler{ timer } );
        BOOST_CHECK_EQUAL(2, ioc.run() );
        BOOST_CHECK( ioc.stopped() );
    }
    BOOST_CHECK_EQUAL(1, called);
}

void spawnTimerDestruct() {
    int called = 0;
    {
        boost::asio::io_context ioc;
        timer_type timer{ ioc, boost::asio::chrono::hours{ 65536 } };
        boost::context::spawn(
                bind_executor( ioc.get_executor(), counting_handler{ called } ),
                spawn_wait_handler{ timer } );
        BOOST_CHECK_EQUAL(1, ioc.run_one() );
        BOOST_CHECK(!ioc.stopped() );
    }
    BOOST_CHECK_EQUAL(0, called);
}

using boost::system::error_code;

template< typename Handler, typename ...Args >
void post( Handler & h, Args && ...args) {
    auto ex = boost::asio::get_associated_executor( h);
    //auto a = boost::asio::get_associated_allocator( h);
    auto b = std::bind( std::move( h), std::forward< Args >( args) ...);
    boost::asio::post( ex, std::move( b));
    //ex.post(std::move(b), a);
}

struct single_tuple_handler {
    std::tuple< int, std::string > &  result;

    void operator()( boost::context::yield_context y) {
        using Signature = void(std::tuple<int, std::string>);
        boost::asio::async_completion< boost::context::yield_context, Signature > init{ y };
        post(
            init.completion_handler,
            std::make_tuple( 42, std::string{ "test" } ) );
        result = init.result.get();
    }
};

void returnSingleTuple() {
    boost::asio::io_context ioc;
    std::tuple< int, std::string > result;
    boost::context::spawn( ioc, single_tuple_handler{ result } );
    BOOST_CHECK_EQUAL(2, ioc.poll() );
    BOOST_CHECK(std::make_tuple(42, std::string{"test"}) == result);
}

struct multiple2_handler {
    std::tuple<int, std::string>& result;
    void operator()( boost::context::yield_context y) {
        using Signature = void(error_code, int, std::string);
        boost::asio::async_completion< boost::context::yield_context, Signature > init{ y };
        post(
            init.completion_handler,
            error_code{},
            42,
            std::string{"test"} );
        result = init.result.get();
    }
};

void returnMultiple2() {
    boost::asio::io_context ioc;
    std::tuple< int, std::string > result;
    boost::context::spawn( ioc, multiple2_handler{result});
    BOOST_CHECK_EQUAL(2, ioc.poll() );
    BOOST_CHECK(std::make_tuple(42, std::string{"test"}) == result);
}

struct multiple2_with_moveonly_handler {
    std::tuple< int, std::unique_ptr< int > > & result;

    void operator()( boost::context::yield_context y) {
        using Signature = void(int, std::unique_ptr<int>);
        boost::asio::async_completion< boost::context::yield_context, Signature > init{ y };
        std::unique_ptr< int > ptr{ new int(42) };
        init.completion_handler( 42, std::move( ptr) );
        result = init.result.get();
    }
};

void returnMultiple2MoveOnly() {
    boost::asio::io_context ioc;
    std::tuple< int, std::unique_ptr< int > > result;
    boost::context::spawn( ioc, multiple2_with_moveonly_handler{ result } );
    BOOST_CHECK_EQUAL(1, ioc.poll() );
    BOOST_CHECK_EQUAL(42, std::get<0>(result) );
    BOOST_CHECK(std::get<1>(result) );
    BOOST_CHECK_EQUAL(42, *std::get<1>(result) );
}

struct multiple3_handler {
    std::tuple< int, std::string, double > &    result;

    void operator()( boost::context::yield_context y) {
        using Signature = void(error_code, int, std::string, double);
        boost::asio::async_completion< boost::context::yield_context, Signature > init{ y };
        post(
            init.completion_handler,
            error_code{},
            42,
            std::string{"test"},
            2.0);
        result = init.result.get();
    }
};

void returnMultiple3() {
    boost::asio::io_context ioc;
    std::tuple< int, std::string, double > result;
    boost::context::spawn( ioc, multiple3_handler{result});
    BOOST_CHECK_EQUAL(2, ioc.poll() );
    BOOST_CHECK(std::make_tuple( 42, std::string{"test"}, 2.0) == result);
}

struct non_default_constructible {
    non_default_constructible() = delete;
    non_default_constructible( std::nullptr_t) {
    }
};

struct non_default_constructible_handler {
    boost::optional< non_default_constructible > &  result;

    void operator()( boost::context::yield_context y) {
        using Signature = void(non_default_constructible);
        boost::asio::async_completion< boost::context::yield_context, Signature > init{ y };
        post(
            init.completion_handler,
            non_default_constructible{ nullptr } );
        result = init.result.get();
    }
};

void returnNonDefaultConstructible() {
    boost::asio::io_context ioc;
    boost::optional< non_default_constructible > result;
    boost::context::spawn( ioc, non_default_constructible_handler{ result } );
    BOOST_CHECK_EQUAL(2, ioc.poll() );
    BOOST_CHECK(result);
}

struct multiple_non_default_constructible_handler {
    boost::optional< std::tuple< int, non_default_constructible > > &   result;

    void operator()( boost::context::yield_context y) {
        using Signature = void(error_code, int, non_default_constructible);
        boost::asio::async_completion< boost::context::yield_context, Signature > init{ y };
        post(
            init.completion_handler,
            error_code{},
            42,
            non_default_constructible{ nullptr } );
        result = init.result.get();
    }
};

void returnMultipleNonDefaultConstructible() {
    boost::asio::io_context ioc;
    boost::optional< std::tuple< int, non_default_constructible > > result;
    boost::context::spawn( ioc, multiple_non_default_constructible_handler{ result } );
    BOOST_CHECK_EQUAL(2, ioc.poll() );
    BOOST_CHECK(result);
}

struct throwing_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T >) {
        throw std::runtime_error{ "" };
    }
};

void spawnThrowInHelper() {
    boost::asio::io_context ioc;
    boost::context::spawn( ioc, throwing_handler{} );
    BOOST_CHECK_THROW(ioc.run_one(), std::runtime_error); // spawn->throw
}

struct noop_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T >) {
    }
};

struct throwing_completion_handler {
    void operator()() {
        throw std::runtime_error{ "" };
    }
};

void spawnHandlerThrowInHelper() {
    boost::asio::io_context ioc;
    boost::context::spawn(
            bind_executor( ioc.get_executor(), throwing_completion_handler{} ),
            noop_handler{} );
    BOOST_CHECK_THROW( ioc.run_one(), std::runtime_error); // spawn->throw
}

template< typename CompletionToken >
auto async_yield( CompletionToken && token) -> BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, void() ) {
    boost::asio::async_completion< CompletionToken, void() > init{ token };
    boost::asio::post( std::move( init.completion_handler) );
    return init.result.get();
}

struct yield_throwing_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        async_yield( y); // suspend and resume before throwing
        throw std::runtime_error{ "" };
    }
};

void spawnThrowAfterYield() {
    boost::asio::io_context ioc;
    boost::context::spawn( ioc, yield_throwing_handler{} );
    BOOST_CHECK_NO_THROW(ioc.run_one() ); // yield_throwing_handler suspend
    BOOST_CHECK_THROW(ioc.run_one(), std::runtime_error); // resume + throw
}

struct yield_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        async_yield( y);
    }
};

void spawnHandlerThrowAfterYield() {
    boost::asio::io_context ioc;
    boost::context::spawn(
            bind_executor( ioc.get_executor(), throwing_completion_handler{} ),
            yield_handler{} );
    BOOST_CHECK_NO_THROW(ioc.run_one() ); // yield_handler suspend
    BOOST_CHECK_THROW(ioc.run_one(), std::runtime_error); // resume + throw
}

struct nested_throwing_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        boost::context::spawn( y, throwing_handler{} );
    }
};

void spawnThrowInNestedHelper() {
    boost::asio::io_context ioc;
    boost::context::spawn( ioc, nested_throwing_handler{} );
    BOOST_CHECK_THROW(ioc.run_one(), std::runtime_error); // spawn->spawn->throw
}

struct yield_nested_throwing_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        async_yield( y); // suspend and resume before spawning
        boost::context::spawn( y, yield_throwing_handler{} );
    }
};

void spawnThrowAfterNestedYield() {
    boost::asio::io_context ioc;
    boost::context::spawn( ioc, yield_nested_throwing_handler{} );
    BOOST_CHECK_NO_THROW(ioc.run_one() ); // yield_nested_throwing_handler suspend
    BOOST_CHECK_NO_THROW(ioc.run_one() ); // yield_throwing_handler suspend
    BOOST_CHECK_THROW(ioc.run_one(), std::runtime_error); // resume + throw
}

struct yield_throw_after_nested_handler {
    template< typename T >
    void operator()( boost::context::basic_yield_context< T > y) {
        async_yield( y); // suspend and resume before spawning
        boost::context::spawn( y, yield_handler{} );
        throw std::runtime_error{ "" };
    }
};

void spawnThrowAfterNestedSpawn() {
    boost::asio::io_context ioc;
    boost::context::spawn( ioc, yield_throw_after_nested_handler() );
    BOOST_CHECK_NO_THROW(ioc.run_one() ); // yield_throw_after_nested_handler suspend
    BOOST_CHECK_THROW(ioc.run_one(), std::runtime_error); // resume + throw
    BOOST_CHECK_EQUAL(1, ioc.poll() ); // yield_handler resume
    BOOST_CHECK(ioc.stopped() );
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: spawn test suite");
    test->add( BOOST_TEST_CASE( & spawnFunction) );
    test->add( BOOST_TEST_CASE( & spawnBoundFunction) );
    test->add( BOOST_TEST_CASE( & spawnFunctionStackAllocator) );
    test->add( BOOST_TEST_CASE( & spawnHandler) );
    test->add( BOOST_TEST_CASE( & spawnHandlerStackAllocator) );
    test->add( BOOST_TEST_CASE( & spawnYieldContext) );
    test->add( BOOST_TEST_CASE( & spawnYieldContextStackAllocator) );
    test->add( BOOST_TEST_CASE( & spawnExecutor) );
    test->add( BOOST_TEST_CASE( & spawnExecutorStackAllocator) );
    test->add( BOOST_TEST_CASE( & spawnStrand) );
    test->add( BOOST_TEST_CASE( & spawnStrandStackAllocator) );
    test->add( BOOST_TEST_CASE( & spawnExecutionContext) );
    test->add( BOOST_TEST_CASE( & spawnExecutionContextStackAllocator) );
    test->add( BOOST_TEST_CASE( & spawnTimer) );
    test->add( BOOST_TEST_CASE( & spawnTimerDestruct) );
    test->add( BOOST_TEST_CASE( & returnSingleTuple) );
    test->add( BOOST_TEST_CASE( & returnMultiple2) );
    test->add( BOOST_TEST_CASE( & returnMultiple2MoveOnly) );
    test->add( BOOST_TEST_CASE( & returnMultiple3) );
    test->add( BOOST_TEST_CASE( & returnNonDefaultConstructible) );
    test->add( BOOST_TEST_CASE( & returnMultipleNonDefaultConstructible) );
    test->add( BOOST_TEST_CASE( & spawnThrowInHelper) );
    test->add( BOOST_TEST_CASE( & spawnHandlerThrowInHelper) );
    test->add( BOOST_TEST_CASE( & spawnThrowAfterYield) );
    test->add( BOOST_TEST_CASE( & spawnHandlerThrowAfterYield) );
    test->add( BOOST_TEST_CASE( & spawnThrowInNestedHelper) );
    test->add( BOOST_TEST_CASE( & spawnThrowAfterNestedYield) );
    test->add( BOOST_TEST_CASE( & spawnThrowAfterNestedSpawn) );
    return test;
}
