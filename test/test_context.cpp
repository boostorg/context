
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

#include "../example/simple_stack_allocator.hpp"

namespace ctx = boost::context;

typedef ctx::simple_stack_allocator<
    8 * 1024 * 1024, // 8MB
    64 * 1024, // 64kB
    8 * 1024 // 8kB
>       stack_allocator;

ctx::fcontext_t fcm = 0;
ctx::fcontext_t fc = 0;
ctx::fcontext_t fc1 = 0;
ctx::fcontext_t fc2 = 0;
int value1 = 0;
std::string value2;
double value3 = 0.;

void f1( void *) {
    ++value1;
    ctx::jump_fcontext( & fc, fcm, 0);
}

void f3( void *) {
    ++value1;
    ctx::jump_fcontext( & fc, fcm, 0);
    ++value1;
    ctx::jump_fcontext( & fc, fcm, 0);
}

void f4( void *) {
    int i = 7;
    ctx::jump_fcontext( & fc, fcm, & i);
}

void f5( void * arg) {
    ctx::jump_fcontext( & fc, fcm, arg);
}

void f6( void * arg) {
    std::pair< int, int > data = * ( std::pair< int, int > * ) arg;
    int res = data.first + data.second;
    data = * ( std::pair< int, int > *)
        ctx::jump_fcontext( & fc, fcm, & res);
    res = data.first + data.second;
    ctx::jump_fcontext( & fc, fcm, & res);
}

void f7( void * arg) {
    try {
        throw std::runtime_error( ( char *) arg);
    } catch ( std::runtime_error const& e) {
        value2 = e.what();
    }
    ctx::jump_fcontext( & fc, fcm, arg);
}

void f8( void * arg) {
    double d = * ( double *) arg;
    d += 3.45;
    value3 = d;
    ctx::jump_fcontext( & fc, fcm, 0);
}

void f10( void *) {
    value1 = 3;
    ctx::jump_fcontext( & fc2, fc1, 0);
}

void f9( void *) {
    std::cout << "f1: entered" << std::endl;
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::default_stacksize());
    fc2 = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), f10);
    ctx::jump_fcontext( & fc1, fc2, 0);
    ctx::jump_fcontext( & fc1, fcm, 0);
}

void test_setup() {
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f1);
    BOOST_CHECK( fc);
}

void test_start() {
    value1 = 0;
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f1);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( 0, value1);
    ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 1, value1);
}

void test_jump() {
    value1 = 0;
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f3);
    BOOST_CHECK( fc);
    BOOST_CHECK_EQUAL( 0, value1);
    ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 1, value1);
    ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 2, value1);
}

void test_result() {
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f4);
    BOOST_CHECK( fc);
    int result = * ( int *) ctx::jump_fcontext( & fcm, fc, 0);
    BOOST_CHECK_EQUAL( 7, result);
}

void test_arg() {
    stack_allocator alloc;
    int i = 7;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f5);
    BOOST_CHECK( fc);
    int result = * ( int *) ctx::jump_fcontext( & fcm, fc, & i);
    BOOST_CHECK_EQUAL( i, result);
}

void test_transfer() {
    stack_allocator alloc;
    std::pair< int, int > data = std::make_pair( 3, 7);
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f6);
    BOOST_CHECK( fc);
    int result = * ( int *) ctx::jump_fcontext( & fcm, fc, & data);
    BOOST_CHECK_EQUAL( 10, result);
    data = std::make_pair( 7, 7);
    result = * ( int *) ctx::jump_fcontext( & fcm, fc, & data);
    BOOST_CHECK_EQUAL( 14, result);
}

void test_exception() {
    stack_allocator alloc;
    const char * what = "hello world";
    void * sp = alloc.allocate( stack_allocator::default_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f7);
    BOOST_CHECK( fc);
    ctx::jump_fcontext( & fcm, fc, ( void *) what);
    BOOST_CHECK_EQUAL( std::string( what), value2);
}

void test_fp() {
    stack_allocator alloc;
    double d = 7.13;
    void * sp = alloc.allocate( stack_allocator::minimum_stacksize() );
    fc = ctx::make_fcontext( sp, stack_allocator::minimum_stacksize(), f8);
    BOOST_CHECK( fc);
    ctx::jump_fcontext( & fcm, fc, & d);
    BOOST_CHECK_EQUAL( 10.58, value3);
}

void test_stacked() {
    value1 = 0;
    stack_allocator alloc;
    void * sp = alloc.allocate( stack_allocator::default_stacksize());
    fc1 = ctx::make_fcontext( sp, stack_allocator::default_stacksize(), f9);
    ctx::jump_fcontext( & fcm, fc1, 0);
    BOOST_CHECK_EQUAL( 3, value1);
}

boost::unit_test::test_suite * init_unit_test_suite( int, char* []) {
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: context test suite");
    test->add( BOOST_TEST_CASE( & test_setup) );
    test->add( BOOST_TEST_CASE( & test_start) );
    test->add( BOOST_TEST_CASE( & test_jump) );
    test->add( BOOST_TEST_CASE( & test_result) );
    test->add( BOOST_TEST_CASE( & test_arg) );
    test->add( BOOST_TEST_CASE( & test_transfer) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    return test;
}
