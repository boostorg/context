
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_CONTEXT_EXECUTION_CONTEXT_H
#define BOOST_CONTEXT_EXECUTION_CONTEXT_H

#include <boost/context/detail/config.hpp>

#if ! defined(BOOST_CONTEXT_NO_EXECUTION_CONTEXT)

# include <cstddef>
# include <cstdint>
# include <cstdlib>
# include <exception>
# include <memory>
# include <tuple>
# include <utility>

# include <boost/assert.hpp>
# include <boost/config.hpp>
# include <boost/context/fcontext.hpp>
# include <boost/intrusive_ptr.hpp>

# include <boost/context/stack_context.hpp>
# include <boost/context/segmented_stack.hpp>

# ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
# endif

# if defined(BOOST_USE_SEGMENTED_STACKS)
extern "C" {

void __splitstack_getcontext( void * [BOOST_CONTEXT_SEGMENTS]);

void __splitstack_setcontext( void * [BOOST_CONTEXT_SEGMENTS]);

}
# endif

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
    struct activation_record {
        typedef boost::intrusive_ptr< activation_record >    ptr_t;

        thread_local static activation_record       main_ar;
        thread_local static ptr_t                   current_ar;

        std::size_t             use_count;
        fcontext_t              fctx;
        activation_record   *   parent_ar;
# if defined(BOOST_USE_SEGMENTED_STACKS)
        bool                    use_segmented_stack = false;
# endif

        activation_record() noexcept :
            use_count( 1),
            fctx( nullptr),
            parent_ar( nullptr) {
        } 

# if defined(BOOST_USE_SEGMENTED_STACKS)
        activation_record( fcontext_t fctx_, bool use_segmented_stack_) noexcept :
            use_count( 0),
            fctx( fctx_),
            parent_ar( nullptr),
            use_segmented_stack( use_segmented_stack_) {
        } 
# endif

        activation_record( fcontext_t fctx_) noexcept :
            use_count( 0),
            fctx( fctx_),
            parent_ar( nullptr) {
        } 

        virtual ~activation_record() noexcept {
        }

        void resume( bool preserve_fpu = false) noexcept {
            activation_record * parent( current_ar.get() );
            parent_ar = parent;
            current_ar = this;
# if defined(BOOST_USE_SEGMENTED_STACKS)
            if ( use_segmented_stack) {
                __splitstack_getcontext( parent->sctx.segments_ctx);
                __splitstack_setcontext( sctx.segments_ctx);

                jump_fcontext( & parent->fctx, fctx, reinterpret_cast< intptr_t >( this), preserve_fpu);

                __splitstack_setcontext( parent->sctx.segments_ctx);
            } else {
                jump_fcontext( & parent->fctx, fctx, reinterpret_cast< intptr_t >( this), preserve_fpu);
            }
# else
            jump_fcontext( & parent->fctx, fctx, reinterpret_cast< intptr_t >( this), preserve_fpu);
# endif
        }

        friend void intrusive_ptr_add_ref( activation_record * ar) {
            ++ar->use_count;
        }

        friend void intrusive_ptr_release( activation_record * ar) {
            BOOST_ASSERT( nullptr != ar);

            if ( 0 == --ar->use_count) {
                ar->~activation_record();
            }
        }
    };

    template< typename Fn, typename StackAlloc >
    class capture_record : public activation_record {
    private:
        StackAlloc      salloc_;
        stack_context   sctx_;
        Fn              fn_;

        static void destroy( capture_record * p) {
            StackAlloc salloc( p->salloc_);
            stack_context sctx( p->sctx_);
            p->~capture_record();
            salloc.deallocate( sctx);
        }

    public:
# if defined(BOOST_USE_SEGMENTED_STACKS)
        explicit capture_record( stack_context sctx, segmented_stack const& salloc, fcontext_t fctx, Fn && fn) noexcept :
            activation_record( fctx, true),
            salloc_( salloc),
            sctx_( sctx),
            fn_( std::forward< Fn >( fn) ) {
        }
# endif

        explicit capture_record( stack_context sctx, StackAlloc const& salloc, fcontext_t fctx, Fn && fn) noexcept :
            activation_record( fctx),
            salloc_( salloc),
            sctx_( sctx),
            fn_( std::forward< Fn >( fn) ) {
        }

        void deallocate() {
            destroy( this);
        }

        void run() noexcept {
            fn_();
        }
    };

    template< typename AR >
    static void entry_func( intptr_t p) noexcept {
        BOOST_ASSERT( 0 != p);

        AR * ar( reinterpret_cast< AR * >( p) );
        BOOST_ASSERT( nullptr != ar);

        ar->run();

        BOOST_ASSERT( nullptr != ar->parent_ar);
        ar->parent_ar->resume();
    }

    typedef boost::intrusive_ptr< activation_record >    ptr_t;

    ptr_t   ptr_;

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( StackAlloc salloc, Fn && fn) {
        typedef capture_record< Fn, StackAlloc >  capture_t;

        stack_context sctx( salloc.allocate() );
        // reserve space for control structure
        std::size_t size = sctx.size - sizeof( capture_t);
        void * sp = static_cast< char * >( sctx.sp) - sizeof( capture_t);
#if 0
        constexpr std::size_t func_alignment = 64; // alignof( capture_t);
        constexpr std::size_t func_size = sizeof( capture_t);
        // reserve space on stack
        void * sp = static_cast< char * >( sctx.sp) - func_size - func_alignment;
        // align sp pointer
        sp = std::align( func_alignment, func_size, sp, func_size + func_alignment);
        BOOST_ASSERT( nullptr != sp);
        // calculate remaining size
        std::size_t size = sctx.size - ( static_cast< char * >( sctx.sp) - static_cast< char * >( sp) );
#endif
        // create fast-context
        fcontext_t fctx = make_fcontext( sp, size, & execution_context::entry_func< capture_t >);
        BOOST_ASSERT( nullptr != fctx);
        // placment new for control structure on fast-context stack
        return new ( sp) capture_t( sctx, salloc, fctx, std::forward< Fn >( fn) );
    }

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( preallocated palloc, StackAlloc salloc, Fn && fn) {
        typedef capture_record< Fn, StackAlloc >  capture_t;

        // reserve space for control structure
        std::size_t size = palloc.size - sizeof( capture_t);
        void * sp = static_cast< char * >( palloc.sp) - sizeof( capture_t);
#if 0
        constexpr std::size_t func_alignment = 64; // alignof( capture_t);
        constexpr std::size_t func_size = sizeof( capture_t);
        // reserve space on stack
        void * sp = static_cast< char * >( palloc.sp) - func_size - func_alignment;
        // align sp pointer
        sp = std::align( func_alignment, func_size, sp, func_size + func_alignment);
        BOOST_ASSERT( nullptr != sp);
        // calculate remaining size
        std::size_t size = palloc.size - ( static_cast< char * >( palloc.sp) - static_cast< char * >( sp) );
#endif
        // create fast-context
        fcontext_t fctx = make_fcontext( sp, size, & execution_context::entry_func< capture_t >);
        BOOST_ASSERT( nullptr != fctx);
        // placment new for control structure on fast-context stack
        return new ( sp) capture_t( palloc.sctx, salloc, fctx, std::forward< Fn >( fn) );
    }

    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    static activation_record * create_capture_record( StackAlloc salloc,
                                             Fn && fn_, Tpl && tpl_,
                                             std::index_sequence< I ... >) {
        return create_context( salloc,
                               [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable {
                                   try {
                                       fn(
                                           // std::tuple_element<> does not perfect forwarding
                                           std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                                                std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
                                   } catch (...) {
                                       std::terminate();
                                   }
                               });
    }

    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    static activation_record * create_capture_record( preallocated palloc, StackAlloc salloc,
                                             Fn && fn_, Tpl && tpl_,
                                             std::index_sequence< I ... >) {
        return create_context( palloc, salloc,
                               [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable {
                                   try {
                                       fn(
                                           // std::tuple_element<> does not perfect forwarding
                                           std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                                                std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
                                   } catch (...) {
                                       std::terminate();
                                   }
                               });
    }

    execution_context() :
        ptr_( activation_record::current_ar) {
    }

public:
    static execution_context current() noexcept {
        return execution_context();
    }

# if defined(BOOST_USE_SEGMENTED_STACKS)
    template< typename Fn, typename ... Args >
    explicit execution_context( segmented_stack salloc, Fn && fn, Args && ... args) :
        ptr_( create_capture_record( salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >() ) ) {
    }

    template< typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, segmented_stack salloc, Fn && fn, Args && ... args) :
        ptr_( create_capture_record( palloc, salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >() ) ) {
    }
# endif

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( StackAlloc salloc, Fn && fn, Args && ... args) :
        ptr_( create_capture_record( salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >() ) ) {
    }

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, StackAlloc salloc, Fn && fn, Args && ... args) :
        ptr_( create_capture_record( palloc, salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >() ) ) {
    }

    void resume( bool preserve_fpu = false) noexcept {
        ptr_->resume( preserve_fpu);
    }
};

}}

# ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
# endif

#endif

#endif // BOOST_CONTEXT_EXECUTION_CONTEXT_H
