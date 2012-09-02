
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_CONTEXT_SOURCE
#define NOMINMAX

#include <boost/context/stack_utils.hpp>

extern "C" {
#include <windows.h>
}

#include <cmath>
#include <csignal>
#include <algorithm>

#include <boost/assert.hpp>

namespace {

static SYSTEM_INFO system_info_()
{
    SYSTEM_INFO si;
    ::GetSystemInfo( & si);
    return si;
}

static SYSTEM_INFO system_info()
{
    static SYSTEM_INFO si = system_info_();
    return si;
}

static std::size_t compute_default_stacksize_()
{
    std::size_t size = 256 * 1024; // 256 kB
    if ( boost::ctx::is_stack_unbound() )
        return std::max( size, boost::ctx::minimum_stacksize() );
    
    BOOST_ASSERT( boost::ctx::maximum_stacksize() >= boost::ctx::minimum_stacksize() );
    return boost::ctx::maximum_stacksize() == boost::ctx::minimum_stacksize()
        ? boost::ctx::minimum_stacksize()
        : std::min( size, boost::ctx::maximum_stacksize() );
}

}

namespace boost {
namespace ctx {

BOOST_CONTEXT_DECL
std::size_t pagesize()
{
    static std::size_t size =
        static_cast< std::size_t >( system_info().dwPageSize);
    return size;
}

BOOST_CONTEXT_DECL
std::size_t page_count( std::size_t stacksize)
{
    return static_cast< std::size_t >(
        std::ceil(
            static_cast< float >( stacksize) / pagesize() ) );
}

// Windows seams not to provide a limit for the stacksize
BOOST_CONTEXT_DECL
bool is_stack_unbound()
{ return true; }

BOOST_CONTEXT_DECL
std::size_t maximum_stacksize()
{
    BOOST_ASSERT( ! is_stack_unbound() );
    static std::size_t size = 1 * 1024 * 1024 * 1024; // 1GB
    return size;
}

BOOST_CONTEXT_DECL
std::size_t minimum_stacksize()
{
    // space for guard page added
    static std::size_t size =
        static_cast< std::size_t >( system_info().dwAllocationGranularity)
        + pagesize();
    return size;
}

BOOST_CONTEXT_DECL
std::size_t default_stacksize()
{
    static std::size_t size = compute_default_stacksize_();
    return size;
}

}}
