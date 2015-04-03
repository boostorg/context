
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
# include <stack>
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

        thread_local static activation_record       toplevel_rec;
        thread_local static ptr_t                   current_rec;

        std::size_t             use_count;
        fcontext_t              fctx;
        stack_context           sctx;
        std::stack< ptr_t >     parents;
        bool                    terminated;
        std::exception_ptr      except;
        bool                    preserve_fpu;
        bool                    use_segmented_stack;

        activation_record() noexcept :
            use_count( 1),
            fctx( nullptr),
            sctx(),
            parents(),
            terminated( false),
            except(),
            preserve_fpu( false),
            use_segmented_stack( false) {
        } 

        activation_record( fcontext_t fctx_, stack_context sctx_, bool use_segmented_stack_) noexcept :
            use_count( 0),
            fctx( fctx_),
            sctx( sctx_),
            parents(),
            terminated( false),
            except(),
            preserve_fpu( false),
            use_segmented_stack( use_segmented_stack_) {
        } 

        virtual ~activation_record() noexcept = default;

        void resume( bool fpu = false) {
            // get parent context
            if ( ! current_rec->terminated)  {
                parents.push( current_rec);
            }
            // store current activation record in local variable
            activation_record * from = current_rec.get();
            // store `this` in static, thread local pointer
            // `this` will become the active (running) context
            // returned by execution_context::current()
            current_rec = this;
            // set FPU flag
            from->preserve_fpu = fpu;
            this->preserve_fpu = fpu;
# if defined(BOOST_USE_SEGMENTED_STACKS)
            if ( use_segmented_stack) {
                // adjust segmented stack properties
                __splitstack_getcontext( from->sctx.segments_ctx);
                __splitstack_setcontext( sctx.segments_ctx);
                // context switch from parent context to `this`-context
                jump_fcontext( & from->fctx, fctx, reinterpret_cast< intptr_t >( this), preserve_fpu);
                // parent context resumed
                // adjust segmented stack properties
                __splitstack_setcontext( from->sctx.segments_ctx);
            } else {
                // context switch from parent context to `this`-context
                jump_fcontext( & from->fctx, fctx, reinterpret_cast< intptr_t >( this), preserve_fpu);
                // parent context resumed
            }
# else
            // context switch from parent context to `this`-context
            jump_fcontext( & from->fctx, fctx, reinterpret_cast< intptr_t >( this), preserve_fpu);
            // parent context resumed
# endif
            // re-throw exception in parent's context
            // if running context throws an exception
            // the context is terminated and the parent context
            // (context which has resumed the throwin context by
            // calling execution_context::operator()) is resumed
            if ( from->except) {
                // store local copy of exception_ptr
                std::exception_ptr ex( from->except);
                // reset exception_ptr of parent context
                from->except = nullptr;
                // re-throw excpetion
                // the exception is thrown out of
                // `throwing context`->execution_context::operator()
                // in parent's context
                std::rethrow_exception( ex);
            }
        }

        virtual void deallocate() {}

        friend void intrusive_ptr_add_ref( activation_record * ar) {
            ++ar->use_count;
        }

        friend void intrusive_ptr_release( activation_record * ar) {
            BOOST_ASSERT( nullptr != ar);

            if ( 0 == --ar->use_count) {
                ar->deallocate();
            }
        }
    };

    template< typename Fn, typename StackAlloc >
    class capture_record : public activation_record {
    private:
        StackAlloc      salloc_;
        Fn              fn_;

        static void destroy( capture_record * p) {
            StackAlloc salloc( p->salloc_);
            stack_context sctx( p->sctx);
            // deallocate activation record
            p->~capture_record();
            // destroy stack with stack allocator
            salloc.deallocate( sctx);
        }

    public:
        explicit capture_record( stack_context sctx, StackAlloc const& salloc, fcontext_t fctx, Fn && fn, bool use_segmented_stack) noexcept :
            activation_record( fctx, sctx, use_segmented_stack),
            salloc_( salloc),
            fn_( std::forward< Fn >( fn) ) {
        }

        void deallocate() override final {
            destroy( this);
        }

        void run() noexcept {
            try {
                fn_();
            } catch (...) {
                BOOST_ASSERT( ! parents.empty() );
                // store exception in parent's exception_ptr
                // because exception is re-thrown in parent's context
                parents.top()->except = std::current_exception();
            }
            // set termination flag
            terminated = true;
            BOOST_ASSERT( ! parents.empty() );
            activation_record * parent = nullptr;
            do {
                parent = parents.top().get();
                parents.pop();
                BOOST_ASSERT( nullptr != parent);
            } while ( parent->terminated && ! parents.empty() );
            // return to parent context
            // use preserve_fpu flag of parent because it
            // is resumed and the current context has terminated
            parent->resume( parent->preserve_fpu);
        }
    };

    // tampoline function
    // entered if the execution context
    // is resumed for the first time
    template< typename AR >
    static void entry_func( intptr_t p) noexcept {
        BOOST_ASSERT( 0 != p);

        AR * ar( reinterpret_cast< AR * >( p) );
        BOOST_ASSERT( nullptr != ar);

        // start execution of toplevel context-function
        ar->run();
    }

    typedef boost::intrusive_ptr< activation_record >    ptr_t;

    ptr_t   ptr_;

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( StackAlloc salloc, Fn && fn, bool use_segmented_stack) {
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
        return new ( sp) capture_t( sctx, salloc, fctx, std::forward< Fn >( fn), use_segmented_stack);
    }

    template< typename StackAlloc, typename Fn >
    static activation_record * create_context( preallocated palloc, StackAlloc salloc, Fn && fn, bool use_segmented_stack) {
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
        return new ( sp) capture_t( palloc.sctx, salloc, fctx, std::forward< Fn >( fn), use_segmented_stack);
    }

    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    static activation_record * create_capture_record( StackAlloc salloc,
                                                      Fn && fn_, Tpl && tpl_,
                                                      std::index_sequence< I ... >,
                                                      bool use_segmented_stack) {
        return create_context( salloc,
                               // lambda, executed in new execution context
                               [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable {
                                       fn(
                                           // non-type template parameter pack used to extract the
                                           // parameters (arguments) from the tuple and pass them to fn
                                           // via parameter pack expansion
                                           // std::tuple_element<> does not perfect forwarding
                                           std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                                                std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
                               },
                               use_segmented_stack);
    }

    template< typename StackAlloc, typename Fn, typename Tpl, std::size_t ... I >
    static activation_record * create_capture_record( preallocated palloc, StackAlloc salloc,
                                                      Fn && fn_, Tpl && tpl_,
                                                      std::index_sequence< I ... >,
                                                      bool use_segmented_stack) {
        return create_context( palloc, salloc,
                               // lambda, executed in new execution context
                               [fn=std::forward< Fn >( fn_),tpl=std::forward< Tpl >( tpl_)] () mutable {
                                       fn(
                                           // non-type template parameter pack used to extract the
                                           // parameters (arguments) from the tuple and pass them to fn
                                           // via parameter pack expansion
                                           // std::tuple_element<> does not perfect forwarding
                                           std::forward< decltype( std::get< I >( std::declval< Tpl >() ) ) >(
                                                std::get< I >( std::forward< Tpl >( tpl) ) ) ... );
                               },
                               use_segmented_stack);
    }

    execution_context() :
        // default constructed with current activation_record
        ptr_( activation_record::current_rec) {
    }

public:
    static execution_context current() noexcept {
        return execution_context();
    }

# if defined(BOOST_USE_SEGMENTED_STACKS)
    template< typename Fn, typename ... Args >
    explicit execution_context( segmented_stack salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), true) ) {
    }

    template< typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, segmented_stack salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( palloc, salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), true) ) {
    }
# endif

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( StackAlloc salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), false) ) {
    }

    template< typename StackAlloc, typename Fn, typename ... Args >
    explicit execution_context( preallocated palloc, StackAlloc salloc, Fn && fn, Args && ... args) :
        // deferred execution of fn and its arguments
        // arguments are stored in std::tuple<>
        // non-type template parameter pack via std::index_sequence_for<>
        // preserves the number of arguments
        // used to extract the function arguments from std::tuple<>
        ptr_( create_capture_record( palloc, salloc,
                                     std::forward< Fn >( fn),
                                     std::make_tuple( std::forward< Args >( args) ... ),
                                     std::index_sequence_for< Args ... >(), false) ) {
    }

    execution_context & operator()( bool preserve_fpu = false) {
        ptr_->resume( preserve_fpu);
        return * this;
    }

    explicit operator bool() const noexcept {
        return nullptr != ptr_.get() && ! ptr_->terminated;
    }

    bool operator!() const noexcept {
        return ! ( nullptr != ptr_.get() && ! ptr_->terminated);
    }
};

}}

# ifdef BOOST_HAS_ABI_HEADERS
# include BOOST_ABI_SUFFIX
# endif

#endif

#endif // BOOST_CONTEXT_EXECUTION_CONTEXT_H
