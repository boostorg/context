
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

#include <boost/array.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/utility.hpp>
#include <boost/variant.hpp>

#include <boost/context/continuation.hpp>
#include <boost/context/detail/config.hpp>

#if defined(BOOST_MSVC)
#include <windows.h>
# pragma warning(push)
# pragma warning(disable: 4723)
#endif

typedef boost::variant<int,std::string> variant_t;

namespace ctx = boost::context;

int value1 = 0;
std::string value2;
double value3 = 0.;

struct X {
    ctx::continuation foo( ctx::continuation && c, int i) {
        value1 = i;
        return std::move( c);
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
    my_exception( char const* what) :
        std::runtime_error( what) {
    }
};

#ifdef BOOST_MSVC
void seh( bool & catched) {
    __try {
        int i = 1;
        i /= 0;
    } __except( EXCEPTION_EXECUTE_HANDLER) {
        catched = true;
    }
}
#endif

ctx::continuation fn1( ctx::continuation && c, int i) {
    value1 = i;
    return std::move( c);
}

ctx::continuation fn2( ctx::continuation && c, const char * what) {
    try {
        throw std::runtime_error( what);
    } catch ( std::runtime_error const& e) {
        value2 = e.what();
    }
    return std::move( c);
}

ctx::continuation fn3( ctx::continuation && c, double d) {
    d += 3.45;
    value3 = d;
    return std::move( c);
}

ctx::continuation fn5( ctx::continuation && c) {
    value1 = 3;
    return std::move( c);
}

ctx::continuation fn4( ctx::continuation && c) {
    ctx::continuation c1 = ctx::callcc< void >( fn5);
    value3 = 3.14;
    return std::move( c);
}

ctx::continuation fn6( ctx::continuation && c) {
    try {
        value1 = 3;
        c = ctx::callcc< void >( std::move( c) );
        value1 = 7;
        c = ctx::callcc< void >( std::move( c) );
    } catch ( my_exception & e) {
        value2 = e.what();
    }
    return std::move( c);
}

ctx::continuation fn7( ctx::continuation && c) {
    Y y;
    return ctx::callcc< void >( std::move( c) );
}

ctx::continuation fn8( ctx::continuation && c, int i) {
    value1 = i;
    return std::move( c);
}

ctx::continuation fn9( ctx::continuation && c, int i) {
    value1 = i;
    c = ctx::callcc< int >( std::move( c), i);
    value1 = ctx::get_data< int >( c);
    return std::move( c);
}

ctx::continuation fn10( ctx::continuation && c, int & i) {
    return ctx::callcc< int & >( std::move( c), i);
}

ctx::continuation fn11( ctx::continuation && c, moveable m) {
    c = ctx::callcc< moveable >( std::move( c), std::move( m) );
    m = ctx::get_data< moveable >( c);
    return ctx::callcc< moveable >( std::move( c), std::move( m) );
}

ctx::continuation fn12( ctx::continuation && c, int i, std::string str) {
    return ctx::callcc< int, std::string >( std::move( c), i, str);
}

ctx::continuation fn13( ctx::continuation && c, int i, moveable m) {
    return ctx::callcc< int, moveable >( std::move( c), i, std::move( m) );
}

ctx::continuation fn14( ctx::continuation && c, variant_t data) {
    int i = boost::get< int >( data);
    data = boost::lexical_cast< std::string >( i);
    return ctx::callcc< variant_t >( std::move( c), data);
}

ctx::continuation fn15( ctx::continuation && c, Y * py) {
    return ctx::callcc< Y * >( std::move( c), py);
}

ctx::continuation fn16( ctx::continuation && c, int i) {
    value1 = i;
    c = ctx::callcc< int >( std::move( c), i);
    value1 = ctx::get_data< int >( c);
    return std::move( c);
}

ctx::continuation fn17( ctx::continuation && c, int i, int j) {
    for (;;) {
        c = ctx::callcc< int, int >( std::move( c), i, j);
        std::tie( i, j) = ctx::get_data< int, int >( c);
    }
    return std::move( c);
}


void test_move() {
    value1 = 0;
    ctx::continuation c;
    BOOST_CHECK( ! c );
    ctx::continuation c1 = ctx::callcc< int >( fn9, 1);
    ctx::continuation c2 = ctx::callcc< int >( fn9, 3);
    BOOST_CHECK( c1 );
    BOOST_CHECK( c2 );
    c1 = std::move( c2);
    BOOST_CHECK( c1 );
    BOOST_CHECK( ! c2 );
    BOOST_CHECK_EQUAL( 3, value1);
    ctx::callcc< int >( std::move( c1), 0);
    BOOST_CHECK_EQUAL( 0, value1);
    BOOST_CHECK( ! c1 );
    BOOST_CHECK( ! c2 );
}

void test_bind() {
    value1 = 0;
    X x;
    ctx::continuation c = ctx::callcc< int >( std::bind( & X::foo, x, std::placeholders::_1, std::placeholders::_2), 7);
    BOOST_CHECK_EQUAL( 7, value1);
}

void test_exception() {
    {
        const char * what = "hello world";
        ctx::continuation c = ctx::callcc< const char * >( fn2, what);
        BOOST_CHECK_EQUAL( std::string( what), value2);
        BOOST_CHECK( ! c );
    }
#ifdef BOOST_MSVC
    {
        bool catched = false;
        std::thread([&catched](){
                ctx::continuation c = ctx::callcc< void >([&catched](ctx::continuation && c){
                c = ctx::callcc< void >( std::move( c) );
                            seh( catched);
                            return std::move( c);
                        });
            BOOST_CHECK( c );
            ctx::callcc< void >( std::move( c) );
        }).join();
        BOOST_CHECK( catched);
    }
#endif
}

void test_fp() {
    double d = 7.13;
    ctx::continuation c = ctx::callcc< double >( fn3, d);
    BOOST_CHECK_EQUAL( 10.58, value3);
    BOOST_CHECK( ! c );
}

void test_stacked() {
    value1 = 0;
    value3 = 0.;
    ctx::continuation c = ctx::callcc< void >( fn4);
    BOOST_CHECK_EQUAL( 3, value1);
    BOOST_CHECK_EQUAL( 3.14, value3);
    BOOST_CHECK( ! c );
}

void test_prealloc() {
    value1 = 0;
    ctx::default_stack alloc;
    ctx::stack_context sctx( alloc.allocate() );
    void * sp = static_cast< char * >( sctx.sp) - 10;
    std::size_t size = sctx.size - 10;
    ctx::continuation c = ctx::callcc< int >( std::allocator_arg, ctx::preallocated( sp, size, sctx), alloc, fn1, 7);
    BOOST_CHECK_EQUAL( 7, value1);
    BOOST_CHECK( ! c );
}

void test_ontop() {
    {
        int i = 3, j = 0;
        ctx::continuation c = ctx::callcc< int >([](ctx::continuation && c, int x) {
                    for (;;) {
                        c = ctx::callcc< int >( std::move( c), x*10);
                        x = ctx::get_data< int >( c);
                    }
                    return std::move( c);
                }, i);
        c = ctx::callcc< int >( std::move( c), ctx::exec_ontop_arg, [](int x){ return x-10; }, i);
        j = ctx::get_data< int >( c);
        BOOST_CHECK( c );
        BOOST_CHECK_EQUAL( j, -70);
    }
    {
        int i = 3, j = 1;
        ctx::continuation c;
        c = ctx::callcc< int, int >( fn17, i, j);
        std::tie( i, j) = ctx::get_data< int, int >( c);
        c = ctx::callcc< int, int >( std::move( c), ctx::exec_ontop_arg, [](int x,int y) { return std::make_tuple( x - y, x + y); }, i, j);
        std::tie( i, j) = ctx::get_data< int, int >( c);
        BOOST_CHECK_EQUAL( i, 2);
        BOOST_CHECK_EQUAL( j, 4);
    }
    {
        moveable m1( 7), m2, dummy;
        ctx::continuation c =  ctx::callcc< moveable >( fn11, std::move( dummy) );
        BOOST_CHECK( 7 == m1.value);
        BOOST_CHECK( m1.state);
        BOOST_CHECK( -1 == m2.value);
        BOOST_CHECK( ! m2.state);
        c = ctx::callcc< moveable >( std::move( c),
                                        ctx::exec_ontop_arg,
                                        [](moveable m){
                                            BOOST_CHECK( m.state);
                                            BOOST_CHECK( 7 == m.value);
                                            return std::move( m);
                                        },
                                        std::move( m1) );
        m2 = ctx::get_data< moveable >( c);
        BOOST_CHECK( ! m1.state);
        BOOST_CHECK( -1 == m1.value);
        BOOST_CHECK( m2.state);
        BOOST_CHECK( 7 == m2.value);
    }
}

void test_ontop_exception() {
    {
        value1 = 0;
        value2 = "";
        ctx::continuation c = ctx::callcc< void >([](ctx::continuation && c){
                for (;;) {
                    value1 = 3;
                    try {
                        c = ctx::callcc< void >( std::move( c) );
                    } catch ( ctx::ontop_error const& e) {
                        try {
                            std::rethrow_if_nested( e);
                        } catch ( my_exception const& ex) {
                            value2 = ex.what();
                        }
                        return e.get_continuation();
                    }
                }
                return std::move( c);
        });
        c = ctx::callcc< void >( std::move( c) );
        BOOST_CHECK_EQUAL( 3, value1);
        const char * what = "hello world";
        ctx::callcc< void >( std::move( c), ctx::exec_ontop_arg, [what](){ throw my_exception( what); });
        BOOST_CHECK_EQUAL( 3, value1);
        BOOST_CHECK_EQUAL( std::string( what), value2);
    }
    {
        value2 = "";
        int i = 3, j = 1;
        ctx::continuation c;
        c = ctx::callcc< int, int >([]( ctx::continuation && c, int x, int y) {
                for (;;) {
                    try {
                            c = ctx::callcc< int, int >( std::move( c), x+y,x-y);
                            std::tie( x, y) = ctx::get_data< int, int >( c);
                    } catch ( boost::context::ontop_error const& e) {
                        try {
                            std::rethrow_if_nested( e);
                        } catch ( my_exception const& ex) {
                            value2 = ex.what();
                        }
                        return e.get_continuation();
                    }
                }
                return std::move( c);
            },
            i, j);
        BOOST_CHECK( c );
        std::tie( i, j) = ctx::get_data< int, int >( c);
        BOOST_CHECK_EQUAL( i, 4);
        BOOST_CHECK_EQUAL( j, 2);
        const char * what = "hello world";
        c = ctx::callcc< int, int >( std::move( c),
                ctx::exec_ontop_arg,
                [what](int x, int y) {
                    throw my_exception(what);
                    return std::make_tuple( x*y, x/y);
                },
                i, j);
        BOOST_CHECK( ! c );
        BOOST_CHECK_EQUAL( i, 4);
        BOOST_CHECK_EQUAL( j, 2);
        BOOST_CHECK_EQUAL( std::string( what), value2);
    }
}

void test_termination() {
    {
        value1 = 0;
        ctx::continuation c = ctx::callcc< void >( fn7);
        BOOST_CHECK_EQUAL( 3, value1);
    }
    BOOST_CHECK_EQUAL( 7, value1);
    {
        value1 = 0;
        BOOST_CHECK_EQUAL( 0, value1);
        ctx::continuation c = ctx::callcc< void >( fn5);
        BOOST_CHECK_EQUAL( 3, value1);
        BOOST_CHECK( ! c );
    }
    {
        value1 = 0;
        BOOST_CHECK_EQUAL( 0, value1);
        int i = 3;
        ctx::continuation c;
        BOOST_CHECK( ! c );
        c = ctx::callcc< int >( fn9, i);
        BOOST_CHECK( c );
        i = ctx::get_data< int >( c);
        BOOST_CHECK_EQUAL( i, value1);
        BOOST_CHECK( c );
        i = 7;
        c = ctx::callcc< int >( std::move( c), i);
        BOOST_CHECK( ! c );
        BOOST_CHECK_EQUAL( i, value1);
    }
}

void test_one_arg() {
    {
        int dummy = 0;
        value1 = 0;
        ctx::continuation c;
        c = ctx::callcc< int >( fn8, 7);
        BOOST_CHECK_EQUAL( 7, value1);
    }
    {
        int i = 3, j = 0;
        ctx::continuation c;
        c = ctx::callcc< int >( fn9, i);
        j = ctx::get_data< int >( c);
        BOOST_CHECK_EQUAL( i, j);
    }
    {
        int i_ = 3, j_ = 7;
        int & i = i_;
        int & j = j_;
        BOOST_CHECK_EQUAL( i, i_);
        BOOST_CHECK_EQUAL( j, j_);
        ctx::continuation c;
        c = ctx::callcc< int & >( fn10, i);
        j = ctx::get_data< int & >( c);
        BOOST_CHECK_EQUAL( i, i_);
        BOOST_CHECK_EQUAL( j, i_);
    }
    {
        Y y;
        Y * py = nullptr;
        ctx::continuation c;
        c = ctx::callcc< Y * >( fn15, & y);
        py = ctx::get_data< Y * >( c);
        BOOST_CHECK( py == & y);
    }
    {
        moveable m1( 7), m2;
        BOOST_CHECK( 7 == m1.value);
        BOOST_CHECK( m1.state);
        BOOST_CHECK( -1 == m2.value);
        BOOST_CHECK( ! m2.state);
        ctx::continuation c;
        c = ctx::callcc< moveable >( fn11, std::move( m1) );
        m2 = ctx::get_data< moveable >( c);
        BOOST_CHECK( -1 == m1.value);
        BOOST_CHECK( ! m1.state);
        BOOST_CHECK( 7 == m2.value);
        BOOST_CHECK( m2.state);
    }
}

void test_two_args() {
    {
        int i1 = 3, i2 = 0;
        std::string str1("abc"), str2;
        ctx::continuation c;
        c = ctx::callcc< int, std::string >( fn12, i1, str1);
        std::tie( i2, str2) = ctx::get_data< int, std::string >( c);
        BOOST_CHECK_EQUAL( i1, i2);
        BOOST_CHECK_EQUAL( str1, str2);
    }
    {
        int i1 = 3, i2 = 0;
        moveable m1( 7), m2;
        BOOST_CHECK( 7 == m1.value);
        BOOST_CHECK( m1.state);
        BOOST_CHECK( -1 == m2.value);
        BOOST_CHECK( ! m2.state);
        ctx::continuation c;
        c = ctx::callcc< int, moveable >( fn13, i1, std::move( m1) );
        std::tie( i2, m2) = ctx::get_data< int, moveable >( c);
        BOOST_CHECK_EQUAL( i1, i2);
        BOOST_CHECK( -1 == m1.value);
        BOOST_CHECK( ! m1.state);
        BOOST_CHECK( 7 == m2.value);
        BOOST_CHECK( m2.state);
    }
}

void test_variant() {
    {
        int i = 7;
        variant_t data1 = i, data2;
        ctx::continuation c;
        c = ctx::callcc< variant_t >( fn14, data1);
        data2 = ctx::get_data< variant_t >( c);
        std::string str = boost::get< std::string >( data2);
        BOOST_CHECK_EQUAL( std::string("7"), str);
    }
}

#ifdef BOOST_WINDOWS
void test_bug12215() {
        ctx::continuation c = ctx::callcc< void >(
            [](ctx::continuation && c) {
                char buffer[MAX_PATH];
                GetModuleFileName( nullptr, buffer, MAX_PATH);
                return std::move( c);
            });
}
#endif

boost::unit_test::test_suite * init_unit_test_suite( int, char* [])
{
    boost::unit_test::test_suite * test =
        BOOST_TEST_SUITE("Boost.Context: callcc test suite");

    test->add( BOOST_TEST_CASE( & test_move) );
    test->add( BOOST_TEST_CASE( & test_bind) );
    test->add( BOOST_TEST_CASE( & test_exception) );
    test->add( BOOST_TEST_CASE( & test_fp) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_stacked) );
    test->add( BOOST_TEST_CASE( & test_prealloc) );
    test->add( BOOST_TEST_CASE( & test_ontop) );
    test->add( BOOST_TEST_CASE( & test_ontop_exception) );
    test->add( BOOST_TEST_CASE( & test_termination) );
    test->add( BOOST_TEST_CASE( & test_one_arg) );
    test->add( BOOST_TEST_CASE( & test_two_args) );
    test->add( BOOST_TEST_CASE( & test_variant) );
#ifdef BOOST_WINDOWS
    test->add( BOOST_TEST_CASE( & test_bug12215) );
#endif

    return test;
}

#if defined(BOOST_MSVC)
# pragma warning(pop)
#endif
