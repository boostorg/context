
//          Copyright Nat Goodspeed, Oliver Kowalke 2019.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>

#include <boost/context/fiber_context.hpp>

namespace ctx = boost::context;

// cancellable is a wrapper class that binds a cancellation-function along
// with an associated fiber_context. It uses the tactic seen earlier in the
// example 'filament' class to continually update the fiber_context
// representing the fiber of interest.
class cancellable {
private:
    ctx::fiber_context                                          f_;
    std::function<ctx::fiber_context(ctx::fiber_context&&)>     cancel_;

public:
    cancellable() :
        f_{},
        cancel_{} {
    }

    template< typename CANCEL >
    cancellable(ctx::fiber_context&& f, CANCEL&& cancel):
        f_{std::move(f)},
        cancel_{std::forward<CANCEL>(cancel)} {
    }

    ~cancellable() {
        if (f_) {
            std::move(f_).resume_with([this](ctx::fiber_context&& f)->ctx::fiber_context{
                return cancel_(std::move(f));
            });
        }
    }

    cancellable(cancellable const&) = delete;
    cancellable & operator=(cancellable const&) = delete;

    cancellable(cancellable&& other) :
        f_{},
        cancel_{} {
        f_ = std::move(other.f_);
        cancel_ = std::move(other.cancel_);
    }

    cancellable & operator=(cancellable&& other) {
        if ( this != & other) {
            f_ = std::move(other.f_);
            cancel_ = std::move(other.cancel_);
        }
        return * this;
    }

    void resume_next(cancellable& cable){
        std::move(cable.f_).resume_with([this](ctx::fiber_context&& f)->ctx::fiber_context{
            if ( nullptr == cancel_) {
                // implicit fiber_context
                return std::move(f);
            }
            f_=std::move(f);
            return {};
        });
    }
};

// unwind_exception is an exception used internally by launch() to unwind the
// stack of a suspended fiber when the referencing fiber_context instance is
// destroyed.
class unwind_exception: public std::runtime_error {
public:
    unwind_exception(ctx::fiber_context&& previous):
        std::runtime_error{"unwinding std::fiber_context"},
        previous_{std::make_shared<ctx::fiber_context>(std::move(previous))}
    {}

    std::shared_ptr<ctx::fiber_context> get_previous() const {
        return previous_;
    }

private:
    // Directly storing fiber_context would make unwind_exception move-only.
    // Instead, store a shared_ptr so unwind_exception remains copyable.
    std::shared_ptr<ctx::fiber_context> previous_;
};

// launch() is a factory function that returns a cancellable representing a
// new fiber that will run the passed entry_function. It implicitly provides a
// top-level wrapper and a cancellation-function. If the cancellable
// representing this new fiber is eventually destroyed, ~cancellable() will
// call this cancellation-function, which will throw unwind_exception. The
// top-level wrapper will catch it and return the bound fiber_context, thereby
// resuming the fiber that called ~cancellable().
template <typename Fn>
cancellable launch(Fn&& entry_function) {
    return cancellable{ctx::fiber_context(
        // entry-function passed to std::fiber_context constructor binds
        // entry_function, calls it within try/catch, catches
        // unwind_exception, extracts its shared_ptr<fiber_context>,
        // dereferences it and returns that fiber_context.
        [entry=std::move(entry_function)]
        (ctx::fiber_context&& previous) {
            try {
                return entry(std::move(previous));
            }
            catch (unwind_exception const& unwind) {
                return std::move(*unwind.get_previous());
            }
        }),
        // cancellation-function passed to cancellable constructor
        // throws unwind_exception, binding passed fiber_context instance.
        [](ctx::fiber_context&& previous)->ctx::fiber_context{
            throw unwind_exception{std::move(previous)};
            return {};
        }};
}

int main() {
    cancellable c = launch([](ctx::fiber_context && f){
            std::cout << "cancellable entered" << std::endl;
            return std::move(f);
    });
    cancellable m;
    m.resume_next(c);
    std::cout << "main: done" << std::endl;
    return EXIT_SUCCESS;
}
