
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_EXECUTION_CONTEXT_H
#define BOOST_CONTEXT_EXECUTION_CONTEXT_H

#if __cplusplus < 201103L
# error "execution_context requires C++11 support!"
#endif

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <memory>

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/context/fcontext.hpp>
#include <boost/intrusive_ptr.hpp>

#include <boost/context/detail/config.hpp>
#include <boost/context/detail/rref.hpp>
#include <boost/context/stack_context.hpp>
#include <boost/context/segmented_stack.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_PREFIX
#endif

#if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C" {

void __splitstack_getcontext( void * [BOOST_CONTEXT_SEGMENTS]);

void __splitstack_setcontext( void * [BOOST_CONTEXT_SEGMENTS]);

}
#endif

namespace boost {
namespace context {

struct preallocated {
    void        *   sp;
    std::size_t     size;
    stack_context   sctx;

    preallocated( void * sp_, std::size_t size_, stack_context sctx_) noexcept :
        sp( sp_), size( size_), sctx( sctx_) {
    }
};

class BOOST_CONTEXT_DECL execution_context {
private:
    struct fcontext {
        std::size_t     use_count;
        fcontext_t      fctx;
        stack_context   sctx;

        // main-context
        fcontext() noexcept :
            use_count( 1),
            fctx( nullptr),
            sctx() {
        } 

        // worker-context
        fcontext( fcontext_t fctx_, stack_context const& sctx_) noexcept :
            use_count( 0),
            fctx( fctx_),
            sctx( sctx_) {
        } 

        virtual ~fcontext() noexcept {
        }

        virtual void deallocate() {
        }

        virtual void run() noexcept {
        }

        friend void intrusive_ptr_add_ref( fcontext * ctx) {
            ++ctx->use_count;
        }

        friend void intrusive_ptr_release( fcontext * ctx) {
            BOOST_ASSERT( nullptr != ctx);

            if ( 0 == --ctx->use_count) {
                ctx->~fcontext();
            }
        }
    };

    template< typename Fn, typename StackAlloc >
    class worker_fcontext : public fcontext {
    private:
        StackAlloc      salloc_;
        Fn              fn_;

        static void destroy( worker_fcontext * p) {
            StackAlloc salloc( p->salloc_);
            stack_context sctx( p->sctx);
            p->~worker_fcontext();
            salloc.deallocate( sctx);
        }

    public:
        explicit worker_fcontext( stack_context sctx, StackAlloc const& salloc, fcontext_t fctx, Fn && fn) noexcept :
            fcontext( fctx, sctx),
            salloc_( salloc),
            fn_( std::forward< Fn >( fn) ) {
        }

        void deallocate() override final {
            destroy( this);
        }

        void run() noexcept override final {
            fn_();
        }
    };

    static void entry_func( intptr_t p) noexcept {
        BOOST_ASSERT( 0 != p);

        fcontext * bp( reinterpret_cast< fcontext * >( p) );
        BOOST_ASSERT( nullptr != bp);

        bp->run();
    }

    typedef boost::intrusive_ptr< fcontext >    ptr_t;

    thread_local static fcontext                main_ctx_;
    thread_local static ptr_t                   current_ctx_;

    boost::intrusive_ptr< fcontext >            ptr_;
#if defined(BOOST_USE_SEGMENTED_STACKS)
    bool                                        use_segmented_stack_ = false;
#endif

    template< typename StackAlloc, typename Fn >
    static fcontext * create_context( StackAlloc salloc, Fn && fn) {
        typedef worker_fcontext< Fn, StackAlloc >  func_t;

        stack_context sctx( salloc.allocate() );
        // reserve space for control structure
        std::size_t size = sctx.size - sizeof( func_t);
        void * sp = static_cast< char * >( sctx.sp) - sizeof( func_t);
        // create fast-context
        fcontext_t fctx = make_fcontext( sp, size, & execution_context::entry_func);
        BOOST_ASSERT( nullptr != fctx);
        // placment new for control structure on fast-context stack
        return new ( sp) func_t( sctx, salloc, fctx, std::forward< Fn >( fn) );
    }

    template< typename StackAlloc, typename Fn >
    static fcontext * create_context( preallocated palloc, StackAlloc salloc, Fn && fn) {
        typedef worker_fcontext< Fn, StackAlloc >  func_t;

        // reserve space for control structure
        std::size_t size = palloc.size - sizeof( func_t);
        void * sp = static_cast< char * >( palloc.sp) - sizeof( func_t);
        // create fast-context
        fcontext_t fctx = make_fcontext( sp, size, & execution_context::entry_func);
        BOOST_ASSERT( nullptr != fctx);
        // placment new for control structure on fast-context stack
        return new ( sp) func_t( palloc.sctx, salloc, fctx, std::forward< Fn >( fn) );
    }

    template< typename StackAlloc, typename Fn, typename ... Args >
    static fcontext * create_worker_fcontext( StackAlloc salloc,
                                              detail::fn_rref< Fn > fn,
                                              detail::arg_rref< Args > ... args) {
        return create_context( salloc,
                               [=] () mutable {
                                   try {
                                       fn( args ...);
                                   } catch (...) {
                                       std::terminate();
                                   }
                               });
    }

    template< typename StackAlloc, typename Fn, typename ... Args >
    static fcontext * create_worker_fcontext( preallocated palloc,
                                              StackAlloc salloc,
                                              detail::fn_rref< Fn > fn,
                                              detail::arg_rref< Args > ... args) {
        return create_context( palloc, salloc,
                               [=] () mutable {
                                   try {
                                       fn( args ...);
                                   } catch (...) {
                                       std::terminate();
                                   }
                               });
    }

    execution_context() :
        ptr_( current_ctx_) {
    }

public:
    static execution_context current() noexcept {
        return execution_context();
    }

#if defined(BOOST_USE_SEGMENTED_STACKS)
    template< typename Fn, typename ... Args >
    explicit execution_context( segmented_stack salloc, Fn && fn, Args && ... args) :
        ptr_( create_worker_fcontext( salloc,
                                      detail::fn_rref< Fn >( std::forward< Fn >( fn) ),
                                      detail::arg_rref< Args >( std::forward< Args >( args) ) ... ) ),
        use_segmented_stack_( true) {
    }

    template< typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, segmented_stack salloc, Fn && fn, Args && ... args) :
        ptr_( create_worker_fcontext( palloc, salloc,
                                      detail::fn_rref< Fn >( std::forward< Fn >( fn) ),
                                      detail::arg_rref< Args >( std::forward< Args >( args) ) ... ) ),
        use_segmented_stack_( true) {
    }
#endif

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( StackAlloc salloc, Fn && fn, Args && ... args) :
        ptr_( create_worker_fcontext( salloc,
                                      detail::fn_rref< Fn >( std::forward< Fn >( fn) ),
                                      detail::arg_rref< Args >( std::forward< Args >( args) ) ... ) ) {
    }

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, StackAlloc salloc, Fn && fn, Args && ... args) :
        ptr_( create_worker_fcontext( palloc, salloc,
                                      detail::fn_rref< Fn >( std::forward< Fn >( fn) ),
                                      detail::arg_rref< Args >( std::forward< Args >( args) ) ... ) ) {
    }

    void jump_to( bool preserve_fpu = false) noexcept {
        BOOST_ASSERT( * this);
        fcontext * tmp( current_ctx_.get() );
        current_ctx_ = ptr_;
#if defined(BOOST_USE_SEGMENTED_STACKS)
        if ( use_segmented_stack_) {
            __splitstack_getcontext( tmp->sctx.segments_ctx);
            __splitstack_setcontext( ptr_->sctx.segments_ctx);

            jump_fcontext( & tmp->fctx, ptr_->fctx, reinterpret_cast< intptr_t >( ptr_.get() ), preserve_fpu);

            __splitstack_setcontext( tmp->sctx.segments_ctx);
        } else {
            jump_fcontext( & tmp->fctx, ptr_->fctx, reinterpret_cast< intptr_t >( ptr_.get() ), preserve_fpu);
        }
#else
        jump_fcontext( & tmp->fctx, ptr_->fctx, reinterpret_cast< intptr_t >( ptr_.get() ), preserve_fpu);
#endif
    }

    explicit operator bool() const noexcept {
        return nullptr != ptr_;
    }

    bool operator!() const noexcept {
        return nullptr == ptr_;
    }
};

}}

#ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
#endif

#endif // BOOST_CONTEXT_EXECUTION_CONTEXT_H
