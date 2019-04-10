
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <stdio.h>
#include <stdlib.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>
#include <boost/variant.hpp>

#include <boost/context/fiber.hpp>
#include <boost/context/detail/config.hpp>

#ifdef BOOST_WINDOWS
#include <windows.h>
#endif

#if defined(BOOST_MSVC)
# pragma warning(push)
# pragma warning(disable: 4702 4723 4996)
#endif

typedef boost::variant<int,std::string> variant_t;

namespace ctx = boost::context;

int value1 = 0;
std::string value2;
double value3 = 0.;

struct X {
    ctx::fiber_context foo( ctx::fiber_context && f, int i) {
        value1 = i;
        return std::move( f);
    }
};

struct Y {
    Y() {
        value1 = 3;
    }

    Y( Y const&) = delete;
    Y & operator=( Y const&) = delete;

    ~Y() {
        value1 = 7;
    }
};

class moveable {
public:
    bool    state;
    int     value;

    moveable() :
        state( false),
        value( -1) {
    }

    moveable( int v) :
        state( true),
        value( v) {
    }

    moveable( moveable && other) :
        state( other.state),
        value( other.value) {
        other.state = false;
        other.value = -1;
    }

    moveable & operator=( moveable && other) {
        if ( this == & other) return * this;
        state = other.state;
        value = other.value;
        other.state = false;
        other.value = -1;
        return * this;
    }

    moveable( moveable const& other) = delete;
    moveable & operator=( moveable const& other) = delete;

    void operator()() {
        value1 = value;
    }
};

struct my_exception : public std::runtime_error {
    ctx::fiber_context   f;
    my_exception( ctx::fiber_context && f_, char const* what) :
        std::runtime_error( what),
        f{ std::move( f_) } {
    }
};

#ifdef BOOST_MSVC
// Optimizations can remove the integer-divide-by-zero here.
#pragma optimize("", off)
void seh( bool & catched) {
    __try {
        int i = 1;
        i /= 0;
    } __except( EXCEPTION_EXECUTE_HANDLER) {
        catched = true;
    }
}
#pragma optimize("", on)
#endif

void test_move() {
    value1 = 0;
    int i = 1;
    BOOST_CHECK_EQUAL( 0, value1);
    ctx::fiber_context f1{
        [&i](ctx::fiber_context && f) {
            value1 = i;
            f = std::move( f).resume();
            value1 = i;
            return std::move( f);
        }};
    f1 = std::move( f1).resume();
    BOOST_CHECK_EQUAL( 1, value1);
    BOOST_CHECK( f1);
    ctx::fiber_context f2;
    BOOST_CHECK( ! f2);
    f2 = std::move( f1);
    BOOST_CHECK( ! f1);
    BOOST_CHECK( f2);
    i = 3;
    f2 = std::move( f2).resume();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK( ! f1);
    BOOST_CHECK( ! f2);
}

void test_bind() {
    value1 = 0;
    X x;
    ctx::fiber_context f{ std::bind( & X::foo, x, std::placeholders::_1, 7) };
    f = std::move( f).resume();
    BOOST_CHECK_EQUAL( 7, value1);
}

void test_exception() {
    {
        const char * what = "hello world";
        ctx::fiber_context f{
            [&what](ctx::fiber_context && f) {
                try {
                    throw std::runtime_error( what);
                } catch ( std::runtime_error const& e) {
                    value2 = e.what();
                }
                return std::move( f);
            }};
        f = std::move( f).resume();
        BOOST_CHECK_EQUAL( std::string( what), value2);
        BOOST_CHECK( ! f);
    }
#ifdef BOOST_MSVC
    {
        bool catched = false;
        std::thread([&catched](){
                ctx::fiber_context f{ [&catched](ctx::fiber_context && f){
                            seh( catched);
                            return std::move( f);
                        }};
            BOOST_CHECK( f);
            f = std::move( f).resume();
        }).join();
        BOOST_CHECK( catched);
    }
#endif
}

void test_fp() {
    value3 = 0.;
    double d = 7.13;
    ctx::fiber_context f{
        [&d]( ctx::fiber_context && f) {
            d += 3.45;
            value3 = d;
            return std::move( f);
        }};
    f = std::move( f).resume();
    BOOST_CHECK_EQUAL( 10.58, value3);
    BOOST_CHECK( ! f);
}

void test_stacked() {
    value1 = 0;
    value3 = 0.;
    ctx::fiber_context f{
        [](ctx::fiber_context && f) {
            ctx::fiber_context f1{
                [](ctx::fiber_context && f) {
                    value1 = 3;
                    return std::move( f);
                }};
            f1 = std::move( f1).resume();
            value3 = 3.14;
            return std::move( f);
        }};
    f = std::move( f).resume();
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( 3.14, value3);
    BOOST_CHECK( ! f);
}

void test_ontop() {
    {
        int i = 3;
        ctx::fiber_context f{ [&i](ctx::fiber_context && f) {
                    for (;;) {
                        i *= 10;
                        f = std::move( f).resume();
                    }
                    return std::move( f);
                }};
        f = std::move( f).resume();
        f = std::move( f).resume_with(
               [&i](ctx::fiber_context && f){
                   i -= 10;
                   return std::move( f);
               });
        BOOST_CHECK( f);
        BOOST_CHECK_EQUAL( i, 200);
    }
    {
        ctx::fiber_context f1;
        ctx::fiber_context f{ [&f1](ctx::fiber_context && f) {
                    f = std::move( f).resume();
                    BOOST_CHECK( ! f);
                    return std::move( f1);
                }};
        f = std::move( f).resume();
        f = std::move( f).resume_with(
               [&f1](ctx::fiber_context && f){
                   f1 = std::move( f);
                   return std::move( f);
               });
    }
}

void test_ontop_exception() {
    value1 = 0;
    value2 = "";
    ctx::fiber_context f{ [](ctx::fiber_context && f){
            for (;;) {
                value1 = 3;
                try {
                    f = std::move( f).resume();
                } catch ( my_exception & ex) {
                    value2 = ex.what();
                    return std::move( ex.f); 
                }
            }
            return std::move( f);
    }};
    f = std::move( f).resume();
    BOOST_CHECK_EQUAL( 3, value1);
    const char * what = "hello world";
    f = std::move( f).resume_with(
       [what](ctx::fiber_context && f){
            throw my_exception( std::move( f), what);
            return std::move( f);
       });
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( std::string( what), value2);
}

void test_termination() {
    {
        value1 = 0;
        ctx::fiber_context f{
            [](ctx::fiber_context && f){
                Y y;
                f = std::move( f).resume();
                return std::move(f);
            }};
        f = std::move( f).resume();
        BOOST_CHECK_EQUAL( 3, value1);
    }
    BOOST_CHECK_EQUAL( 7, value1);
    {
        value1 = 0;
        BOOST_CHECK_EQUAL( 0, value1);
        ctx::fiber_context f{
            [](ctx::fiber_context && f) {
                value1 = 3;
                return std::move( f);
            }};
        f = std::move( f).resume();
        BOOST_CHECK_EQUAL( 3, value1);
        BOOST_CHECK( ! f);
    }
    {
        value1 = 0;
        BOOST_CHECK_EQUAL( 0, value1);
        int i = 3;
        ctx::fiber_context f{
            [&i](ctx::fiber_context && f){
                value1 = i;
                f = std::move( f).resume();
                value1 = i;
                return std::move( f);
            }};
        f = std::move( f).resume();
        BOOST_CHECK( f);
        BOOST_CHECK_EQUAL( i, value1);
        BOOST_CHECK( f);
        i = 7;
        f = std::move( f).resume();
        BOOST_CHECK( ! f);
        BOOST_CHECK_EQUAL( i, value1);
    }
}

void test_sscanf() {
    ctx::fiber_context{
		[]( ctx::fiber_context && f) {
			{
				double n1 = 0;
				double n2 = 0;
				sscanf("3.14 7.13", "%lf %lf", & n1, & n2);
				BOOST_CHECK( n1 == 3.14);
				BOOST_CHECK( n2 == 7.13);
			}
			{
				int n1=0;
				int n2=0;
				sscanf("1 23", "%d %d", & n1, & n2);
				BOOST_CHECK( n1 == 1);
				BOOST_CHECK( n2 == 23);
			}
			{
				int n1=0;
				int n2=0;
				sscanf("1 jjj 23", "%d %*[j] %d", & n1, & n2);
				BOOST_CHECK( n1 == 1);
				BOOST_CHECK( n2 == 23);
			}
			return std::move( f);
	}}.resume();
}

void test_snprintf() {
    ctx::fiber_context{
		[]( ctx::fiber_context && f) {
            {
                const char *fmt = "sqrt(2) = %f";
                char buf[19];
                snprintf( buf, sizeof( buf), fmt, std::sqrt( 2) );
                BOOST_CHECK( 0 < sizeof( buf) );
                BOOST_ASSERT( std::string("sqrt(2) = 1.41") == std::string( buf, 14) );
            }
            {
                std::uint64_t n = 0xbcdef1234567890;
                const char *fmt = "0x%016llX";
                char buf[100];
                snprintf( buf, sizeof( buf), fmt, n);
                BOOST_ASSERT( std::string("0x0BCDEF1234567890") == std::string( buf, 18) );
            }
			return std::move( f);
	}}.resume();
}

void test_can_resume_from_any_thread() {
    {
        ctx::fiber_context f{
            []( ctx::fiber_context && m) {
                BOOST_CHECK( m);
                BOOST_CHECK( ! m.can_resume_from_any_thread() );
                m = std::move( m).resume();
                BOOST_CHECK( ! m.can_resume_from_any_thread() );
                return std::move( m);
        }};
        BOOST_CHECK( f);
        BOOST_CHECK( f.can_resume_from_any_thread() );
        f = std::move( f).resume();
        BOOST_CHECK( f);
        BOOST_CHECK( f.can_resume_from_any_thread() );
        f = std::move( f).resume();
        BOOST_CHECK( ! f);
    }
    {
        ctx::fiber_context f{
            []( ctx::fiber_context && m) {
                BOOST_CHECK( m);
                BOOST_CHECK( ! m.can_resume_from_any_thread() );
                m = std::move( m).resume();
                BOOST_CHECK( m);
                BOOST_CHECK( ! m.can_resume_from_any_thread() );
                m = std::move( m).resume();
                BOOST_CHECK( m);
                BOOST_CHECK( ! m.can_resume_from_any_thread() );
                return std::move( m);
        }};
        BOOST_CHECK( f);
        BOOST_CHECK( f.can_resume_from_any_thread() );
        f = std::move( f).resume();
        BOOST_CHECK( f);
        BOOST_CHECK( f.can_resume_from_any_thread() );
        f = std::move( f).resume_with(
            []( ctx::fiber_context && m) {
                BOOST_CHECK( m);
                BOOST_CHECK( ! m.can_resume_from_any_thread() );
                return std::move( m);
            });
        BOOST_CHECK( f);
        BOOST_CHECK( f.can_resume_from_any_thread() );
        f = std::move( f).resume();
        BOOST_CHECK( ! f);
    }
}

void test_hosting_thread() {
    ctx::fiber_context f{
            [](ctx::fiber_context && m){
                BOOST_CHECK( m.can_resume() );
                ctx::fiber_context * pm = & m;
                std::thread{ [pm]{ BOOST_CHECK( ! pm->can_resume() ); }}.join();
                m = std::move( m).resume();
                return std::move( m);
            }};
    BOOST_CHECK( f);
    BOOST_CHECK( f.can_resume() );
    f = std::move( f).resume();
    BOOST_CHECK( f.can_resume() );

}

#ifdef BOOST_WINDOWS
void test_bug12215() {
        ctx::fiber_context{
            [](ctx::fiber_context && f) {
                char buffer[MAX_PATH];
                GetModuleFileName( nullptr, buffer, MAX_PATH);
                return std::move( f);
            }}.resume();
}
#endif

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: fiber test suite");

    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_bind) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_ontop) );
    test->add( BOOST_TEST_CASE( & test_ontop_exception) );
    test->add( BOOST_TEST_CASE( & test_termination) );
    test->add( BOOST_TEST_CASE( & test_sscanf) );
    test->add( BOOST_TEST_CASE( & test_snprintf) );
    test->add( BOOST_TEST_CASE( & test_can_resume_from_any_thread) );
    test->add( BOOST_TEST_CASE( & test_hosting_thread) );
#ifdef BOOST_WINDOWS
    test->add( BOOST_TEST_CASE( & test_bug12215) );
#endif

    return test;
}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif
