
//          Copyright 2026.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

// Regression test: make_fcontext must leave a *null* frame-pointer terminator in the initial
// context, so that a frame-pointer stack walker (macOS backtrace()/__thread_stack_pcs, sample,
// lldb, crash reporters) stops at the fiber entry instead of following an uninitialized frame
// pointer off the stack and crashing.
//
// We create a fiber on a stack that is poisoned with a pointer into a guard page and then, from
// inside the fiber, walk the frame-pointer chain the way such a walker does (dereferencing each
// frame). If make_fcontext leaves the terminator uninitialized, the walk reaches the poisoned
// slot and faults in the guard page (test crashes). With the terminator zeroed, the walk stops
// cleanly at the fiber entry and the test passes.
//
// Meaningful on arm64 (aapcs frame-pointer chain via x29); a no-op elsewhere.

#include <boost/context/detail/fcontext.hpp>
#include <boost/core/lightweight_test.hpp>

#if defined(__aarch64__) || defined(__arm64__)

#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

namespace ctx = boost::context::detail;

// Walk the frame-pointer chain from the current frame, dereferencing each frame like a naive
// stack walker. A 0 terminator stops the walk; a poisoned (guard-page) terminator faults here.
static void walk_fiber_fp_chain( ctx::transfer_t t) {
    void ** fp = static_cast< void ** >( __builtin_frame_address( 0) );
    for ( int i = 0; i < 100000 && fp != nullptr; ++i) {
        void * saved_fp = fp[0];   // dereference: faults if fp points into the guard page
        void * saved_lr = fp[1];
        (void)saved_lr;
        fp = static_cast< void ** >( saved_fp);
    }
    ctx::jump_fcontext( t.fctx, nullptr);
}

int main() {
    const std::size_t size = 256 * 1024;
    const std::size_t page = 16 * 1024;

    char * base = static_cast< char * >(
        ::mmap( nullptr, size + page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0) );
    BOOST_TEST( base != MAP_FAILED);
    // guard page directly above the usable stack: any read past the stack top faults here
    ::mprotect( base + size, page, PROT_NONE);

    // Poison the whole usable stack with an (aligned) pointer into the guard page, so that an
    // *uninitialized* frame-pointer terminator would send the walk into the guard.
    const std::uintptr_t poison = reinterpret_cast< std::uintptr_t >( base + size + 64);
    for ( std::size_t i = 0; i < size / sizeof( std::uintptr_t); ++i)
        reinterpret_cast< std::uintptr_t * >( base)[ i] = poison;

    ctx::fcontext_t fctx = ctx::make_fcontext( base + size, size, walk_fiber_fp_chain);
    ctx::jump_fcontext( fctx, nullptr);   // crashes here if the terminator was not zeroed

    // Reached only if the frame-pointer walk terminated cleanly at the fiber entry.
    BOOST_TEST( true);
    return boost::report_errors();
}

#else

int main() { return 0; }

#endif
