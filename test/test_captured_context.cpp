
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>

#include <boost/context/all.hpp>
#include <boost/context/detail/config.hpp>

namespace ctx = boost::context;

int value1 = 0;
std::string value2;
double value3 = 0.;

struct X {
    ctx::captured_context foo( int i, ctx::captured_context ctx, void * ignored) {
        value1 = i;
        return ctx;
    }
};

struct Y {
    Y() {
        value1 = 3;
    }

    ~Y() {
        value1 = 7;
    }
};

struct my_exception : public std::runtime_error {
    ctx::captured_context   ctx;

    my_exception( char const* what, ctx::captured_context && ctx_) :
        std::runtime_error( what),
        ctx( std::forward< ctx::captured_context >( ctx_) ) {
    }
};

ctx::captured_context fn1( int i, ctx::captured_context ctx, void * ignored) {
    value1 = i;
    return ctx;
}

ctx::captured_context fn2( const char * what, ctx::captured_context ctx, void * ignored) {
    try {
        throw std::runtime_error( what);
    } catch ( std::runtime_error const& e) {
        value2 = e.what();
    }
    return ctx;
}

ctx::captured_context fn3( double d, ctx::captured_context ctx, void * ignored) {
    d += 3.45;
    value3 = d;
    return ctx;
}

ctx::captured_context fn5( ctx::captured_context ctx, void * ignored) {
    value1 = 3;
    return ctx;
}

ctx::captured_context fn4( ctx::captured_context ctx, void * ignored) {
    ctx::captured_context ctx1( fn5);
    ctx1();
    value3 = 3.14;
    return ctx;
}

ctx::captured_context fn6( ctx::captured_context ctx, void * ignored) {
    try {
        value1 = 3;
        std::tie( ctx, ignored) = ctx();
        value1 = 7;
        std::tie( ctx, ignored) = ctx( ignored);
    } catch ( my_exception & e) {
        value2 = e.what();
        ctx = std::move( e.ctx);
    }
    return ctx;
}

ctx::captured_context fn7( ctx::captured_context ctx, void * ignored) {
    Y y;
    std::tie( ctx, ignored) = ctx();
    return ctx;
}

void test_move() {
    value1 = 0;
    ctx::captured_context ctx;
    BOOST_CHECK( ! ctx);
    ctx::captured_context ctx1( fn1, 1);
    ctx::captured_context ctx2( fn1, 3);
    BOOST_CHECK( ctx1);
    BOOST_CHECK( ctx2);
    ctx1 = std::move( ctx2);
    BOOST_CHECK( ctx1);
    BOOST_CHECK( ! ctx2);
    BOOST_CHECK_EQUAL( 0, value1);
    ctx1();
    BOOST_CHECK_EQUAL( 3, value1);
}

void test_memfn() {
    value1 = 0;
    X x;
    ctx::captured_context ctx( & X::foo, x, 7);
    ctx();
    BOOST_CHECK_EQUAL( 7, value1);
}

void test_exception() {
    const char * what = "hello world";
    ctx::captured_context ctx( fn2, what);
    ctx();
    BOOST_CHECK_EQUAL( std::string( what), value2);
}

void test_fp() {
    double d = 7.13;
    ctx::captured_context ctx( fn3, d);
    ctx();
    BOOST_CHECK_EQUAL( 10.58, value3);
}

void test_stacked() {
    value1 = 0;
    value3 = 0.;
    ctx::captured_context ctx( fn4);
    ctx();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( 3.14, value3);
}

void test_prealloc() {
    value1 = 0;
    ctx::default_stack alloc;
    ctx::stack_context sctx( alloc.allocate() );
    void * sp = static_cast< char * >( sctx.sp) - 10;
    std::size_t size = sctx.size - 10;
    ctx::captured_context ctx( std::allocator_arg, ctx::preallocated( sp, size, sctx), alloc, fn1, 7);
    ctx();
    BOOST_CHECK_EQUAL( 7, value1);
}

void test_ontop() {
    value1 = 0;
    value2 = "";
    void * ignored;
    ctx::captured_context ctx( fn6);
    std::tie( ctx, ignored) = ctx();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK( value2.empty() );
    std::string str("abc");
    int i = 3;
    std::tie( ctx, ignored) = ctx( & i, ctx::exec_ontop_arg,
                                   [str](ctx::captured_context ctx, void * data){
                                        value2 = str;
                                        return std::make_tuple( std::move( ctx), data);
                                    } );
    BOOST_CHECK_EQUAL( 7, value1);
    BOOST_CHECK_EQUAL( str, value2);
    BOOST_CHECK_EQUAL( ignored, & i);
    BOOST_CHECK_EQUAL( i, *( int*) ignored);
}

void test_ontop_exception() {
    value1 = 0;
    value2 = "";
    void * ignored;
    ctx::captured_context ctx( fn6);
    std::tie( ctx, ignored) = ctx();
    BOOST_CHECK_EQUAL( 3, value1);
    const char * what = "hello world";
    ctx( ctx::exec_ontop_arg,
         [what](ctx::captured_context ctx, void * data){
            throw my_exception( what, std::move( ctx) );
            return std::make_tuple( std::move( ctx), data);
         });
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( std::string( what), value2);
}

void test_termination() {
    value1 = 0;
    {
        void * ignored;
        ctx::captured_context ctx( fn7);
        BOOST_CHECK_EQUAL( 0, value1);
        std::tie( ctx, ignored) = ctx();
        BOOST_CHECK_EQUAL( 3, value1);
    }
    BOOST_CHECK_EQUAL( 7, value1);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: captured_context test suite");

    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_memfn) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_prealloc) );
    test->add( BOOST_TEST_CASE( & test_ontop) );
    test->add( BOOST_TEST_CASE( & test_ontop_exception) );
    test->add( BOOST_TEST_CASE( & test_termination) );

    return test;
}
