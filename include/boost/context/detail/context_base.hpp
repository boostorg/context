
//          Copyright Oliver Kowalke 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXTS_DETAIL_CONTEXT_BASE_H
#define BOOST_CONTEXTS_DETAIL_CONTEXT_BASE_H

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/utility.hpp>

#include <boost/context/detail/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/context/flags.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace boost {
namespace contexts {
namespace detail {

struct forced_unwind {};

template< typename Ctx >
void trampoline( intptr_t vp)
{
    BOOST_ASSERT( vp);

    Ctx * ctx( reinterpret_cast< Ctx * >( vp) );

    try
    { ctx->exec(); }
    catch ( forced_unwind const&)
    {
        ctx->flags_ |= Ctx::flag_complete;
        boost_fcontext_jump( & ctx->ctx_callee_, & ctx->ctx_caller_, 0);   
    }
    catch (...)
    { std::terminate(); }

    ctx->flags_ |= Ctx::flag_complete;

    // in order to return to the code invoked the context
    // nxt_->caller_ hast to set to the first one
    if ( ctx->nxt_)
    {
		Ctx * nxt( dynamic_cast< Ctx * >( ctx->nxt_.get() ) );
		BOOST_ASSERT( nxt);
        std::swap( nxt->ctx_caller_, ctx->ctx_caller_);
        if ( 0 != ( nxt->flags_ & Ctx::flag_do_return) )
            nxt->ctx_callee_.fc_link = & nxt->ctx_caller_;
        nxt->flags_ |= Ctx::flag_running;
        nxt->flags_ |= Ctx::flag_started;
    }
}

class context_base : private noncopyable
{
public:
    typedef intrusive_ptr< context_base >   ptr_t;

	template< typename Allocator >
    context_base(
			Allocator & alloc, std::size_t size,
			flag_unwind_t do_unwind, flag_return_t do_return) :
        use_count_( 0), base_( alloc.allocate( size) ), ctx_caller_(), ctx_callee_(), nxt_(),
        flags_( stack_unwind == do_unwind ? flag_force_unwind : flag_dont_force_unwind)
    {
        BOOST_ASSERT( base_);

        std::memset( & ctx_caller_, 0, sizeof( ctx_caller_) );
        std::memset( & ctx_callee_, 0, sizeof( ctx_callee_) );
        ctx_callee_.fc_stack.ss_base = base_;
        ctx_callee_.fc_stack.ss_limit =
            static_cast< char * >( ctx_callee_.fc_stack.ss_base) - size;

        if ( return_to_caller == do_return)
        {
            flags_ |= flag_do_return;
            ctx_callee_.fc_link = & ctx_caller_;
        }

        boost_fcontext_make(
            & ctx_callee_, trampoline< context_base >, reinterpret_cast< intptr_t >( this) );
    }

	template< typename Allocator >
    context_base( Allocator & alloc, std::size_t size, flag_unwind_t do_unwind, ptr_t nxt) :
        use_count_( 0), base_( alloc.allocate( size) ), ctx_caller_(), ctx_callee_(), nxt_( nxt),
        flags_( stack_unwind == do_unwind ? flag_force_unwind : flag_dont_force_unwind)
    {
        BOOST_ASSERT( base_);
        BOOST_ASSERT( ! nxt_->is_complete() );

        std::memset( & ctx_callee_, 0, sizeof( ctx_callee_) );
        std::memset( & ctx_caller_, 0, sizeof( ctx_caller_) );
        ctx_callee_.fc_stack.ss_base = base_;
        ctx_callee_.fc_stack.ss_limit =
            static_cast< char * >( ctx_callee_.fc_stack.ss_base) - size;
        ctx_callee_.fc_link = & dynamic_pointer_cast< context_base >( nxt_)->ctx_callee_;

        boost_fcontext_make(
            & ctx_callee_, trampoline< context_base >, reinterpret_cast< intptr_t >( this) );
    }

	virtual ~context_base() {}

	template< typename Allocator >
    void cleanup( Allocator & alloc)
    {
        if ( ! is_complete()
                && ( is_started() || is_resumed() )
                && ( unwind_requested() ) )
            unwind_stack();
		std::size_t size = static_cast< char * >( ctx_callee_.fc_stack.ss_base) -
			static_cast< char * >( ctx_callee_.fc_stack.ss_limit);
		alloc.deallocate( base_, size);
    }

    bool unwind_requested() const
    { return 0 != ( flags_ & flag_force_unwind); }

    bool is_complete() const
    { return 0 != ( flags_ & flag_complete); }

    bool is_started() const
    { return 0 != ( flags_ & flag_started); }

    bool is_resumed() const
    { return 0 != ( flags_ & flag_started); }

    bool is_running() const
    { return 0 != ( flags_ & flag_running); }

    intptr_t start()
    {
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( ! is_started() );
        BOOST_ASSERT( ! is_running() );

        flags_ |= flag_started;
        flags_ |= flag_running;
        return boost_fcontext_start( & ctx_caller_, & ctx_callee_);
    }

    intptr_t resume( intptr_t vp)
    {
        BOOST_ASSERT( is_started() );
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( ! is_running() );

        flags_ |= flag_resumed;
        flags_ |= flag_running;
        return boost_fcontext_jump( & ctx_caller_, & ctx_callee_, vp);
    }

    intptr_t suspend( intptr_t vp)
    {
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( is_running() );

        flags_ &= ~flag_running;
        intptr_t res = boost_fcontext_jump( & ctx_callee_, & ctx_caller_, vp);
        if ( 0 != ( flags_ & flag_unwind_stack) )
            throw forced_unwind();
		return res;
    }

    void unwind_stack()
    {
        BOOST_ASSERT( ! is_complete() );
        BOOST_ASSERT( ! is_running() );

        flags_ |= flag_unwind_stack;
        boost_fcontext_jump( & ctx_caller_, & ctx_callee_, 0);
        flags_ &= ~flag_unwind_stack;
        BOOST_ASSERT( is_complete() );
    }

    virtual void exec() = 0;

    friend inline void intrusive_ptr_add_ref( context_base * p)
    { ++p->use_count_; }

    friend inline void intrusive_ptr_release( context_base * p)
    { if ( --p->use_count_ == 0) delete p; }

private:
    template< typename T >
    friend void trampoline( intptr_t vp);

    enum flag_t
    {
        flag_started                = 1 << 1,
        flag_resumed                = 1 << 2,
        flag_running                = 1 << 3,
        flag_complete               = 1 << 4,
        flag_unwind_stack           = 1 << 5,
        flag_force_unwind           = 1 << 6,
        flag_dont_force_unwind      = 1 << 7,
        flag_do_return              = 1 << 8,
    };

    std::size_t				use_count_;
	void				*	base_;
    boost_fcontext_t        ctx_caller_;
    boost_fcontext_t        ctx_callee_;
    ptr_t                   nxt_;
    short                   flags_;
};

}}}

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXTS_DETAIL_CONTEXT_BASE_H
