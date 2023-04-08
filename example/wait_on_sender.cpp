#include <span>
#include <cstdlib>
#include <utility>
#include <cassert>
#include <atomic>
#include <semaphore>
#include <stop_token>
#include <type_traits>
#include <concepts>
#include <functional>
#include <variant>
#include <optional>
#include <exception>
#include <memory>

#include <stdexec/execution.hpp>

#include <boost/context/fiber_context>

///////////////////////////////////////////
// Implementation of P2300 fiber_context integration
//
// stdexec::fiber_sender(scheduler auto s, invocable auto f) -> sender_of<invoke_result_t<decltype(f)>>;
// - Runs f() in a fiber that always resumes on the the specified scheduler.
//
// stdexec::fiber_wait(sender auto s) -> std::optional<std::tuple<Values...>>
// - suspends the current fiber and launches the async operation, s.
// - resumes the current fiber when 's' completes and returns the result of 's'

namespace stdexec {

struct __fiber_op_base;

inline thread_local __fiber_op_base* __current_fiber_op = nullptr;

struct __fiber_op_base : __immovable {
    // Function that should be called to schedule resumption of the
    // child fiber. Only valid to call when the child fiber is currently
    // suspended.
    using __schedule_fn = void(__fiber_op_base*) noexcept;
    __schedule_fn* __schedule_;

    // When the child fiber is suspended this holds the fiber_context
    // of the child fiber. When the child fiber is running, this holds
    // the fiber_context of the fiber that last resumed this fiber.
    std::fiber_context __child_or_main_fiber_;

    // The __fiber_op_base associated with the main fiber_context when
    // the child fiber is active.
    __fiber_op_base* __main_fiber_op_ = nullptr;

    void __resume_child() noexcept {
        std::fiber_context __child = std::move(__child_or_main_fiber_);
        [[maybe_unused]] std::fiber_context __result =
            std::move(__child).resume_with([this](std::fiber_context __main) noexcept -> std::fiber_context {
                assert(__child_or_main_fiber_.empty());
                __child_or_main_fiber_ = std::move(__main);

                assert(__main_fiber_op_ == nullptr);
                __main_fiber_op_ = std::exchange(__current_fiber_op, this);

                return std::fiber_context();
            });
        assert(__result.empty());
    }

    template<typename Func>
    void __resume_main_with(Func&& func) noexcept {
        std::fiber_context __main = std::move(__child_or_main_fiber_);
        [[maybe_unused]] std::fiber_context __result =
            std::move(__main).resume_with([&](std::fiber_context __child) noexcept -> std::fiber_context {
                assert(__child_or_main_fiber_.empty());
                __child_or_main_fiber_ = std::move(__child);

                assert(__current_fiber_op == this);
                __current_fiber_op = std::exchange(__main_fiber_op_, nullptr);

                static_assert(noexcept(std::invoke(static_cast<Func&&>(func))));
                std::invoke(static_cast<Func&&>(func));

                return std::fiber_context();
            });
        assert(__result.empty());
    }
};

namespace __fiber_sender {

template<typename _Func, typename _Scheduler, typename _Receiver>
struct __fiber_op_state {
    struct __t : private __fiber_op_base {
        using __result_type = std::invoke_result_t<std::decay_t<_Func>>;
    public:
        using __id = __fiber_op_state;
    __t(_Func&& __func, _Scheduler __sched, _Receiver __r)
    : __receiver_(std::move(__r))
    , __scheduler_(std::move(__sched))
    {
        this->__schedule_ = [](__fiber_op_base* __base) noexcept {
            static_cast<__t*>(__base)->__schedule_resume();
        };
        this->__child_or_main_fiber_ =
            std::fiber_context([__func=std::forward<_Func>(__func), this](std::fiber_context) noexcept -> std::fiber_context {
                if (__started_) {
                    constexpr bool __is_nothrow = std::is_nothrow_invocable_v<std::decay_t<_Func>>;
                    try {
                        __result_.template emplace<1>(
                            __conv{
                                [&]() noexcept(__is_nothrow) -> __non_void_result_type {
                                    if constexpr (std::is_void_v<__result_type>) {
                                        std::invoke(std::move(__func));
                                        return {};
                                    } else {
                                        return std::invoke(std::move(__func));
                                    }
                                }});
                    } catch (...) {
                        __result_.template emplace<2>(std::current_exception());
                    }

                    __deliver_result();
                }

                __current_fiber_op = std::exchange(this->__main_fiber_op_, nullptr);
                return std::move(this->__child_or_main_fiber_);
            });
    }

    ~__t() {
        if (!__started_) {
            // Need to resume the child in order for it to run to completion
            // and destroy itself.
            this->__resume_child();
        }
        assert(this->__child_or_main_fiber_.empty());
        assert(this->__main_fiber_op_ == nullptr);
    }

    friend void tag_invoke(start_t, __t& __self) noexcept {
        __self.__started_ = true;
        __self.__schedule_resume();
    }

    private:
        // Receiver passed to schedule() operation that resumes the fiber.
        struct __schedule_receiver {
            __t& __op_;

            friend void tag_invoke(set_value_t, __schedule_receiver&& __self) noexcept {
                // Take a copy of '__op_' member, since call to 'reset()' will
                // destroy __self.
                auto& __op = __self.__op_;
                __op.__schedule_op_.reset();
                __op.__resume_child();
            }

            template<typename E>
            [[noreturn]] friend void tag_invoke(set_error_t, __schedule_receiver&&, E&&) noexcept {
                std::terminate();
            }

            [[noreturn]] friend void tag_invoke(set_stopped_t, __schedule_receiver&&) noexcept {
                std::terminate();
            }

            // TODO: Should this return something that forwards queries to
            // the fiber_op_state::receiver object?
            friend empty_env tag_invoke(get_env_t, const __schedule_receiver&) noexcept {
                return {};
            }
        };

        // Schedule a call to __resume_child() to occur on the associated scheduler's context.
        // The child fiber must be suspended before calling this.
        void __schedule_resume() noexcept {
            stdexec::start(
                __schedule_op_.emplace(__conv{[&]() {
                    return stdexec::connect(
                        stdexec::schedule(__scheduler_),
                        __schedule_receiver{*this});
                }}));
        }

        // Call the fiber-op's receiver with the return value of 'func()'
        // Must be called from the child fiber's context.
        void __deliver_result() noexcept {
            this->__resume_main_with([&]() noexcept {
                // Take a copy of '*this' before resuming the child as resuming the child
                // will potentially destroy the temporary lambda we are currently executing
                // within.
                auto& __self = *this;

                // Resume the child fiber to let it run to completion and destroy itself.
                // This ensures that `child_or_main_fiber` is an empty fiber_context before
                // we run the op-state destructor (which could technically be run at any
                // time, from any thread after we invoke the completion handler).
                __self.__resume_child();

                // Then, once we are back on the main context, we can invoke the receiver
                // with the result.
                if (__self.__result_.index() == 2) {
                    stdexec::set_error(std::move(__self.__receiver_), std::get<2>(std::move(__self.__result_)));
                } else {
                    if constexpr (std::is_void_v<__result_type>) {
                        stdexec::set_value(std::move(__self.__receiver_));
                    } else {
                        stdexec::set_value(std::move(__self.__receiver_), std::get<1>(std::move(__self.__result_)));
                    }
                }
            });
        }

        struct __empty {};
        using __non_void_result_type = std::conditional_t<std::is_void_v<__result_type>, __empty, __result_type>;

        _Receiver __receiver_;
        _Scheduler __scheduler_;
        bool __started_ = false;
        std::variant<std::monostate, __non_void_result_type, std::exception_ptr> __result_;

        using __schedule_sender = decltype(stdexec::schedule(std::declval<_Scheduler&>()));
        using __schedule_op_state = stdexec::connect_result_t<__schedule_sender, __schedule_receiver>;
        std::optional<__schedule_op_state> __schedule_op_;
    };
};

template<typename _Scheduler, typename _Func>
struct __sender {
    struct __t {
        using __id = __sender;

        using is_sender = void;

        using completion_signatures =
            std::conditional_t<
                std::is_nothrow_invocable_v<_Func>,
                completion_signatures<set_value_t(std::invoke_result_t<_Func>)>,
                completion_signatures<set_value_t(std::invoke_result_t<_Func>), set_error_t(std::exception_ptr)>>;

        _Scheduler __scheduler_;
        _Func __main_func_;

        template<typename _Receiver>
            requires std::move_constructible<_Func>
        friend stdexec::__t<__fiber_op_state<_Func, _Scheduler, _Receiver>> tag_invoke(connect_t, __t&& __self, _Receiver __r) {
            return {std::move(__self.__main_func_), std::move(__self.__scheduler_), std::move(__r)};
        }

        template<typename _Receiver>
            requires std::copy_constructible<_Func>
        friend stdexec::__t<__fiber_op_state<_Func, _Scheduler, _Receiver>> tag_invoke(connect_t, const __t& __self, _Receiver __r) {
            return {std::move(__self.__main_func_), std::move(__self.__scheduler_), std::move(__r)};
        }
    };
};

} // namespace __fiber_sender

template<typename _Scheduler, typename _Func>
__t<__fiber_sender::__sender<_Scheduler, _Func>> let_fiber(_Scheduler __sched, _Func __func) {
    return {std::move(__sched), std::move(__func)};
}

namespace __fiber_wait {

    template <class... _Values>
    struct __state {
      using _Tuple = std::tuple<_Values...>;
      std::variant<std::monostate, _Tuple, std::exception_ptr, set_stopped_t> __data_{};
      __fiber_op_base* __fiber_op_ = nullptr;
    };

    template <class... _Values>
    struct __receiver {
      struct __t {
        using __id = __receiver;
        __state<_Values...>* __state_;

        void __finish() noexcept {
            auto* __op = __state_->__fiber_op_;
            __op->__schedule_(__op);
        }

        template <class _Error>
        void __set_error(_Error __err) noexcept {
          if constexpr (__decays_to<_Error, std::exception_ptr>)
            __state_->__data_.template emplace<2>((_Error&&) __err);
          else if constexpr (__decays_to<_Error, std::error_code>)
            __state_->__data_.template emplace<2>(
              std::make_exception_ptr(std::system_error(__err)));
          else
            __state_->__data_.template emplace<2>(std::make_exception_ptr((_Error&&) __err));

          __finish();
        }

        template <class... _As>
          requires constructible_from<std::tuple<_Values...>, _As...>
        friend void tag_invoke(stdexec::set_value_t, __t&& __rcvr, _As&&... __as) noexcept {
          try {
            __rcvr.__state_->__data_.template emplace<1>((_As&&) __as...);
            __rcvr.__finish();
          } catch (...) {
            __rcvr.__set_error(std::current_exception());
          }
        }

        template <class _Error>
        friend void tag_invoke(stdexec::set_error_t, __t&& __rcvr, _Error __err) noexcept {
          __rcvr.__set_error((_Error&&) __err);
        }

        friend void tag_invoke(stdexec::set_stopped_t __d, __t&& __rcvr) noexcept {
          __rcvr.__state_->__data_.template emplace<3>(__d);
          __rcvr.__finish();
        }

        friend empty_env tag_invoke(stdexec::get_env_t, const __t& __rcvr) noexcept {
          return {};
        }
      };
    };

    struct fiber_wait_t {
      template <class _Sender>
      using __receiver_t = __t<__sync_wait::__sync_wait_result_impl<_Sender, __q<__receiver>>>;

      template <__single_value_variant_sender<empty_env> _Sender>
        requires sender_in<_Sender, empty_env>
             && sender_to<_Sender, __receiver_t<_Sender>>
      static auto __impl(_Sender&& __sndr) -> std::optional<__sync_wait::__sync_wait_result_t<_Sender>> {
        auto* __fiber_op = __current_fiber_op;
        if (__fiber_op == nullptr) {
            std::terminate();
        }

        using state_t = __sync_wait::__sync_wait_result_impl<_Sender, __q<__state>>;
        state_t __state{};
        __state.__fiber_op_ = __fiber_op;

        // Launch the sender with a continuation that will fill in a variant
        // and notify a condition variable.
        auto __op_state = connect((_Sender&&) __sndr, __receiver_t<_Sender>{&__state});

        // Need to launch the sender only after suspending the current fiber.
        __fiber_op->__resume_main_with([&]() noexcept {
            start(__op_state);
        });

        if (__state.__data_.index() == 2)
          std::rethrow_exception(std::move(std::get<2>(__state.__data_)));

        if (__state.__data_.index() == 3)
          return std::nullopt;

        return std::move(std::get<1>(__state.__data_));
      }

      template <__single_value_variant_sender<empty_env> _Sender>
        requires sender_in<_Sender, empty_env>
             && sender_to<_Sender, __receiver_t<_Sender>>
      __attribute__((always_inline)) auto operator()(_Sender&& __sndr) const -> std::optional<__sync_wait::__sync_wait_result_t<_Sender>> {
        return __impl(static_cast<_Sender&&>(__sndr));
      }
};

} // namespace __fiber_wait

using __fiber_wait::fiber_wait_t;
inline constexpr fiber_wait_t fiber_wait{};

} // namespace stdexec

int fiber_main() noexcept {
    auto [x] = *stdexec::fiber_wait(stdexec::just(123));
    return x;
}

int main() {
    using namespace stdexec;
    std::optional<std::tuple<int>> result = sync_wait(
        let_value(get_scheduler(), [](auto sched) {
            return let_fiber(sched, &fiber_main);
        }));
    return std::get<0>(result.value());
}
