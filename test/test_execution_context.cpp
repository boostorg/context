
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
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
ctx::execution_context * mctx = nullptr;

void fn1() {
    value1 = 3;
    ( * mctx)();
}

void fn2( int i) {
    value1 = i;
    ( * mctx)();
}

void fn3( const char * what) {
    try
    { throw std::runtime_error( what); }
    catch ( std::runtime_error const& e)
    { value2 = e.what(); }
    ( * mctx)();
}

void fn4( double d) {
    d += 3.45;
    value3 = d;
    ( * mctx)();
}

void fn6( ctx::execution_context * ctx) {
    value1 = 3;
    ( * ctx)();
}

void fn5() {
    std::cout << "fn5: entered" << std::endl;
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, fn6, & ctx);
    ectx();
    value3 = 3.14;
    ( * mctx)();
}

struct X {
    int foo( int i) {
        value1 = i;
        ( * mctx)();
        return i;
    }
};

void test_ectx() {
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    value1 = 0;
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, fn1);
    ectx();
    BOOST_CHECK_EQUAL( 3, value1);
}

void test_variadric() {
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    value1 = 0;
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, fn2, 5);
    ectx();
    BOOST_CHECK_EQUAL( 5, value1);
}

void test_memfn() {
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    value1 = 0;
    X x;
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, & X::foo, x, 7);
    ectx();
    BOOST_CHECK_EQUAL( 7, value1);
}

void test_exception() {
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    const char * what = "hello world";
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, fn3, what);
    ectx();
    BOOST_CHECK_EQUAL( std::string( what), value2);
}

void test_fp() {
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    double d = 7.13;
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, fn4, d);
    ectx();
    BOOST_CHECK_EQUAL( 10.58, value3);
}

void test_stacked() {
    value1 = 0;
    value3 = 0.;
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    ctx::fixedsize_stack alloc;
    ctx::execution_context ectx( alloc, fn5);
    ectx();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( 3.14, value3);
}

void test_prealloc() {
    boost::context::execution_context ctx( boost::context::execution_context::current() );
    mctx = & ctx;
    value1 = 0;
    ctx::fixedsize_stack alloc;
    ctx::stack_context sctx( alloc.allocate() );
    void * sp = static_cast< char * >( sctx.sp) - 10;
    std::size_t size = sctx.size - 10;
    ctx::execution_context ectx( ctx::preallocated( sp, size, sctx), alloc, fn2, 7);
    ectx();
    BOOST_CHECK_EQUAL( 7, value1);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: execution_context test suite");

    test->add( BOOST_TEST_CASE( & test_ectx) );
    test->add( BOOST_TEST_CASE( & test_variadric) );
    test->add( BOOST_TEST_CASE( & test_memfn) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_prealloc) );

    return test;
}
