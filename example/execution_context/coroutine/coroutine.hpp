
//          Copyright Oliver Kowalke 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef ASYMMETRIC_COROUTINE_HPP
#define ASYMMETRIC_COROUTINE_HPP

#include <exception>

#include <boost/assert.hpp>
#include <boost/config.hpp>

#include <boost/context/execution_context.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

namespace detail {

struct forced_unwind {};

struct synthesized_t {};
synthesized_t synthesized;

enum class state_t : unsigned int {
    complete = 1 << 1,
    unwind   = 1 << 2
};

template< typename T >
class pull_coroutine;

template< typename T >
class push_coroutine;

template< typename T >
class pull_coroutine {
private:
    template< typename X >
    friend class push_coroutine;

    boost::context::execution_context   caller_;
    boost::context::execution_context   callee_;
    int                                 state_;
    std::exception_ptr                  except_;
    T                               **  t_;

    pull_coroutine( synthesized_t,
                    boost::context::execution_context const& caller,
                    boost::context::execution_context const& callee,
                    T ** t) :
        caller_( caller),
        callee_( callee),
        state_( 0),
        except_(),
        t_( t) {
    }

public:
    template< typename StackAllocator, typename Fn >
    explicit pull_coroutine( StackAllocator salloc, Fn && fn_);

    pull_coroutine( pull_coroutine const&) = delete;
    pull_coroutine & operator=( pull_coroutine const&) = delete;

    void operator()() {
        callee_.jump_to();
        if ( except_) {
            std::rethrow_exception( except_);
        }
    }

    explicit operator bool() const noexcept {
        return nullptr != t_ && 0 == ( state_ & static_cast< int >( state_t::complete) );
    }

    bool operator!() const noexcept {
        return nullptr == t_ || 0 != ( state_ & static_cast< int >( state_t::complete) );
    }

    T get() const noexcept {
        return ** t_;
    }
};

template<>
class pull_coroutine< void > {
private:
    template< typename X >
    friend class push_coroutine;

    boost::context::execution_context   caller_;
    boost::context::execution_context   callee_;
    int                                 state_;
    std::exception_ptr                  except_;

    pull_coroutine( synthesized_t,
                    boost::context::execution_context const& caller,
                    boost::context::execution_context const& callee) :
        caller_( caller),
        callee_( callee),
        state_( 0),
        except_() {
    }

public:
    template< typename StackAllocator, typename Fn >
    explicit pull_coroutine( StackAllocator salloc, Fn && fn_);

    pull_coroutine( pull_coroutine const&) = delete;
    pull_coroutine & operator=( pull_coroutine const&) = delete;

    void operator()() {
        callee_.jump_to();
        if ( except_) {
            std::rethrow_exception( except_);
        }
    }

    explicit operator bool() const noexcept {
        return 0 == ( state_ & static_cast< int >( state_t::complete) );
    }

    bool operator!() const noexcept {
        return 0 != ( state_ & static_cast< int >( state_t::complete) );
    }
};

template< typename T >
class push_coroutine {
private:
    template< typename X >
    friend class pull_coroutine;

    boost::context::execution_context   caller_;
    boost::context::execution_context   callee_;
    int                                 state_;
    std::exception_ptr                  except_;
    T                               *   t_;

    push_coroutine( synthesized_t,
                    boost::context::execution_context const& caller,
                    boost::context::execution_context const& callee,
                    T *** t) :
        caller_( caller),
        callee_( callee),
        state_( 0),
        except_(),
        t_( nullptr) {
        * t = & t_;
    }

public:
    template< typename StackAllocator, typename Fn >
    push_coroutine( StackAllocator salloc, Fn && fn_);

    push_coroutine( push_coroutine const&) = delete;
    push_coroutine & operator=( push_coroutine const&) = delete;

    void operator()( T t) {
        t_ = & t;
        callee_.jump_to();
        if ( except_) {
            std::rethrow_exception( except_);
        }
    }

    explicit operator bool() const noexcept {
        return 0 == ( state_ & static_cast< int >( state_t::complete) );
    }

    bool operator!() const noexcept {
        return 0 != ( state_ & static_cast< int >( state_t::complete) );
    }
};

template<>
class push_coroutine< void > {
private:
    template< typename X >
    friend class pull_coroutine;

    boost::context::execution_context   caller_;
    boost::context::execution_context   callee_;
    int                                 state_;
    std::exception_ptr                  except_;

    push_coroutine( synthesized_t,
                    boost::context::execution_context const& caller,
                    boost::context::execution_context const& callee) :
        caller_( caller),
        callee_( callee),
        state_( 0),
        except_() {
    }

public:
    template< typename StackAllocator, typename Fn >
    push_coroutine( StackAllocator salloc, Fn && fn_);

    push_coroutine( push_coroutine const&) = delete;
    push_coroutine & operator=( push_coroutine const&) = delete;

    void operator()() {
        callee_.jump_to();
        if ( except_) {
            std::rethrow_exception( except_);
        }
    }

    explicit operator bool() const noexcept {
        return 0 == ( state_ & static_cast< int >( state_t::complete) );
    }

    bool operator!() const noexcept {
        return 0 != ( state_ & static_cast< int >( state_t::complete) );
    }
};

template< typename T >
template< typename StackAllocator, typename Fn >
pull_coroutine< T >::pull_coroutine( StackAllocator salloc, Fn && fn_) :
    caller_( boost::context::execution_context::current() ),
    callee_( salloc,
             [=,&fn_](){
                // create synthesized push_coroutine< void >
                push_coroutine< T > push_coro( synthesized, callee_, caller_, & t_);
                try {
                    // store coroutine-fn
                    Fn fn( std::forward< Fn >( fn_) );
                    // call coroutine-fn with synthesized pull_coroutine as argument
                    fn( push_coro);
                } catch ( forced_unwind const&) {
                    // do nothing for unwinding exception
                } catch (...) {
                    // store other exceptions in exception-pointer
                    except_ = std::current_exception();
                }
                // set termination flags
                state_ |= static_cast< int >( state_t::complete);
                // jump back to caller
                caller_.jump_to();
                BOOST_ASSERT_MSG( false, "pull_coroutine is complete");
            }),
    state_( static_cast< int >( state_t::unwind) ),
    except_(),
    t_( nullptr) {
    callee_.jump_to();
}

template< typename StackAllocator, typename Fn >
pull_coroutine< void >::pull_coroutine( StackAllocator salloc, Fn && fn_) :
    caller_( boost::context::execution_context::current() ),
    callee_( salloc,
             [=,&fn_](){
                // create synthesized push_coroutine< void >
                push_coroutine< void > push_coro( synthesized, callee_, caller_);
                try {
                    // store coroutine-fn
                    Fn fn( std::forward< Fn >( fn_) );
                    // call coroutine-fn with synthesized pull_coroutine as argument
                    fn( push_coro);
                } catch ( forced_unwind const&) {
                    // do nothing for unwinding exception
                } catch (...) {
                    // store other exceptions in exception-pointer
                    except_ = std::current_exception();
                }
                // set termination flags
                state_ |= static_cast< int >( state_t::complete);
                // jump back to caller
                caller_.jump_to();
                BOOST_ASSERT_MSG( false, "pull_coroutine is complete");
            }),
    state_( static_cast< int >( state_t::unwind) ),
    except_() {
    callee_.jump_to();
}

template< typename T >
template< typename StackAllocator, typename Fn >
push_coroutine< T >::push_coroutine( StackAllocator salloc, Fn && fn_) :
    caller_( boost::context::execution_context::current() ),
    callee_( salloc,
             [=,&fn_](){
                // create synthesized pull_coroutine< T >
                pull_coroutine< T > pull_coro( synthesized, callee_, caller_, & t_);
                try {
                    // store coroutine-fn
                    Fn fn( std::forward< Fn >( fn_) );
                    // call coroutine-fn with synthesized pull_coroutine as argument
                    fn( pull_coro);
                } catch ( forced_unwind const&) {
                    // do nothing for unwinding exception
                } catch (...) {
                    // store other exceptions in exception-pointer
                    except_ = std::current_exception();
                }
                // set termination flags
                state_ |= static_cast< int >( state_t::complete);
                // jump back to caller
                caller_.jump_to();
                BOOST_ASSERT_MSG( false, "push_coroutine is complete");
            }),
    state_( static_cast< int >( state_t::unwind) ),
    except_(),
    t_( nullptr) {
}

template< typename StackAllocator, typename Fn >
push_coroutine< void >::push_coroutine( StackAllocator salloc, Fn && fn_) :
    caller_( boost::context::execution_context::current() ),
    callee_( salloc,
             [=,&fn_](){
                // create synthesized pull_coroutine< void >
                pull_coroutine< void > pull_coro( synthesized, callee_, caller_);
                try {
                    // store coroutine-fn
                    Fn fn( std::forward< Fn >( fn_) );
                    // call coroutine-fn with synthesized pull_coroutine as argument
                    fn( pull_coro);
                } catch ( forced_unwind const&) {
                    // do nothing for unwinding exception
                } catch (...) {
                    // store other exceptions in exception-pointer
                    except_ = std::current_exception();
                }
                // set termination flags
                state_ |= static_cast< int >( state_t::complete);
                // jump back to caller
                caller_.jump_to();
                BOOST_ASSERT_MSG( false, "push_coroutine is complete");
            }),
    state_( static_cast< int >( state_t::unwind) ),
    except_() {
}

}

template< typename T >
struct coroutine {
    typedef detail::pull_coroutine< T >     pull_type;
    typedef detail::push_coroutine< T >     push_type;
};

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif // ASYMMETRIC_COROUTINE_HPP
