#ifndef BIPOLAR_FUTURES_ADAPTOR_HPP_
#define BIPOLAR_FUTURES_ADAPTOR_HPP_

#include <algorithm>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include "bipolar/core/movable.hpp"
#include "bipolar/core/option.hpp"
#include "bipolar/core/void.hpp"
#include "bipolar/futures/async_result.hpp"
#include "bipolar/futures/context.hpp"
#include "bipolar/futures/traits.hpp"

#include <boost/callable_traits.hpp>

namespace bipolar {
// forward
template <typename Continuation>
class PromiseImpl;

template <typename Promise>
class FutureImpl;

namespace internal {
// MoveOnlyHandler
//
// Interposer type that provides uniform move constructors/assignment for
// callable types that may or may not be move assignable. Lambdas are not
// typically move assignable, even though they may be move constructible.
//
// This type has a well-defined empty state. Instance of this type that are
// the source of move operation are left in the empty state.
//
// NOTE:
// Since C++20, Non-capturing lambdas can be move assignable, while capturing
// lambdas not.
template <typename Handler>
class MoveOnlyHandler : public Movable {
    static_assert(std::is_move_constructible_v<Handler>,
                  "Handler must be move constructible");

public:
    constexpr MoveOnlyHandler() noexcept = default;
    constexpr MoveOnlyHandler(MoveOnlyHandler&&) noexcept(
        std::is_nothrow_move_constructible_v<Handler>) = default;
    constexpr MoveOnlyHandler& operator=(MoveOnlyHandler&&) noexcept(
        std::is_nothrow_move_constructible_v<Handler>) = default;

    ~MoveOnlyHandler() = default;

    template <typename U,
              std::enable_if_t<
                  !std::is_same_v<std::remove_cv_t<std::remove_reference_t<U>>,
                                  MoveOnlyHandler> &&
                      std::is_constructible_v<Handler, U>,
                  int> = 0>
    constexpr MoveOnlyHandler(U&& handler) noexcept(
        std::is_nothrow_constructible_v<Handler, U>) {
        handler_.emplace(std::forward<U>(handler));
    }

    template <typename U,
              std::enable_if_t<
                  !std::is_same_v<std::remove_cv_t<std::remove_reference_t<U>>,
                                  MoveOnlyHandler> &&
                      std::is_constructible_v<Handler, U>,
                  int> = 0>
    constexpr MoveOnlyHandler& operator=(U&& handler) noexcept(
        std::is_nothrow_constructible_v<Handler, U>) {
        handler_.clear();
        handler_.emplace(std::forward<U>(handler));
        return *this;
    }

    // Invokes it
    //
    // Throws `OptionEmptyException` when `static_cast<bool>(*this)` is false
    template <typename... Args>
    constexpr auto operator()(Args&&... args)
        -> std::invoke_result_t<Handler, Args...> {
        return (handler_.value())(std::forward<Args>(args)...);
    }

    // Checks if the handler has a calable object
    constexpr explicit operator bool() const noexcept {
        return handler_.has_value();
    }

    // Resets to empty state
    constexpr void reset() noexcept {
        handler_.clear();
    }

private:
    Option<Handler> handler_;
};

// Wraps a handler function and adapts its return type to a `AsyncResult`
// via its specializations
template <typename Handler, typename T, typename E,
          typename ReturnType = boost::callable_traits::return_type_t<Handler>,
          bool callable_result = is_functor_v<ReturnType>>
class ResultAdaptor;

// NonCallableResultAdaptor
//
// Handlers returning `AsyncPending`, `AsyncOk`, `AsyncError`, or `AsyncResult`
// should inherit this base class
template <typename Handler, typename T, typename E>
class NonCallableResultAdaptor : public Movable {
public:
    using result_type = AsyncResult<T, E>;

    constexpr explicit NonCallableResultAdaptor(
        MoveOnlyHandler<Handler> handler)
        : handler_(std::move(handler)) {}

    template <typename... Args>
    constexpr result_type operator()(Context& ctx, Args&&... args) {
        (void)ctx;
        return handler_(std::forward<Args>(args)...);
    }

private:
    MoveOnlyHandler<Handler> handler_;
};

// Supports handlers that return `AsyncPending`
template <typename Handler, typename T, typename E>
class ResultAdaptor<Handler, T, E, AsyncPending, false>
    : public NonCallableResultAdaptor<Handler, T, E> {
public:
    using NonCallableResultAdaptor<Handler, T, E>::NonCallableResultAdaptor;
};

// Supports handlers that return `AsyncOk`
template <typename Handler, typename T, typename E, typename U>
class ResultAdaptor<Handler, T, E, AsyncOk<U>, false>
    : public NonCallableResultAdaptor<Handler, U, E> {
public:
    using NonCallableResultAdaptor<Handler, U, E>::NonCallableResultAdaptor;
};

// Supports handlers that return `AsyncError`
template <typename Handler, typename T, typename E, typename V>
class ResultAdaptor<Handler, T, E, AsyncError<V>, false>
    : public NonCallableResultAdaptor<Handler, T, V> {
public:
    using NonCallableResultAdaptor<Handler, T, V>::NonCallableResultAdaptor;
};

// Supports handlers that return `AsyncResult`
template <typename Handler, typename T, typename E, typename U, typename V>
class ResultAdaptor<Handler, T, E, AsyncResult<U, V>, false>
    : public NonCallableResultAdaptor<Handler, U, V> {
public:
    using NonCallableResultAdaptor<Handler, U, V>::NonCallableResultAdaptor;
};

// Supports handlers that return `Continuation`/`Promise`
template <typename Handler, typename T, typename E, typename R>
class ResultAdaptor<Handler, T, E, R, true> : public Movable {
    using continuation_type = typename continuation_traits<R>::type;

public:
    using result_type = typename continuation_traits<R>::result_type;

    constexpr explicit ResultAdaptor(MoveOnlyHandler<Handler> handler)
        : handler_(std::move(handler)) {}

    template <typename... Args>
    constexpr result_type operator()(Context& ctx, Args&&... args) {
        if (handler_) {
            cont_ = handler_(std::forward<Args>(args)...);
            handler_.reset();
        }
        if (!cont_) {
            return AsyncPending{};
        }
        return cont_(ctx);
    }

private:
    MoveOnlyHandler<Handler> handler_;
    MoveOnlyHandler<continuation_type> cont_;
};

// Get the first argument type of argument tuple, then wrap it with `std::tuple`
// When the tuple is empty, use `std::tuple<>` as default.
template <typename>
struct first_or_empty_type_helper;

template <>
struct first_or_empty_type_helper<std::tuple<>> {
    using type = std::tuple<>;
};

template <typename T, typename... Ts>
struct first_or_empty_type_helper<std::tuple<T, Ts...>> {
    using type = std::tuple<T>;
};

// Wraps a handler that may or may not have a `Context&` as first argument.
// This is determined by checking the first argument type.
//
// Signatures with a `Context&` argument:
// - (Context&)
// - (Context&, value_type&)
// - (Context&, const value_type&)
//
// Signatures without a `Context&` argument:
// - ()
// - (value_type&)
// - (const value_type&)
template <typename Handler, typename T, typename E>
class ContextAdaptor {
    using adaptor_type = ResultAdaptor<Handler, T, E>;

    static constexpr bool has_context_arg =
        std::is_same_v<typename first_or_empty_type_helper<
                           boost::callable_traits::args_t<Handler>>::type,
                       std::tuple<Context&>>;

public:
    using result_type = typename adaptor_type::result_type;
    static constexpr std::size_t next_arg_index = has_context_arg ? 1 : 0;

    constexpr explicit ContextAdaptor(Handler handler)
        : adaptor_(std::move(handler)) {}

    template <typename... Args>
    constexpr result_type operator()(Context& ctx, Args&&... args) {
        if constexpr (has_context_arg) {
            return adaptor_(ctx, ctx, std::forward<Args>(args)...);
        } else {
            return adaptor_(ctx, std::forward<Args>(args)...);
        }
    }

private:
    adaptor_type adaptor_;
};

// Wraps a handler that may accept a context argument
// e.g.
// - ()
// - (Context&)
template <typename Handler>
class ContextHandlerInvoker {
    using adaptor_type = ContextAdaptor<Handler, Void, Void>;

public:
    using result_type = typename adaptor_type::result_type;

    constexpr explicit ContextHandlerInvoker(Handler handler)
        : adaptor_(std::move(handler)) {}

    constexpr result_type operator()(Context& ctx) {
        return adaptor_(ctx);
    }

private:
    adaptor_type adaptor_;
};

// Wraps a handler that may accept a context and result argument.
// e.g.
// - (Result&)
// - (const Result&)
// - (Context&, Result&)
// - (Context&, const Result&)
template <typename Handler, typename Result>
class ResultHandlerInvoker {
    using adaptor_type = ContextAdaptor<Handler, Void, Void>;

    using args = boost::callable_traits::args_t<Handler>;
    static_assert(std::tuple_size_v<args> == 1 || std::tuple_size_v<args> == 2,
                  "The provided handler has wrong arguments number");

    using result_arg_type =
        std::tuple_element_t<adaptor_type::next_arg_index, args>;
    static_assert(std::is_same_v<result_arg_type, Result&> ||
                      std::is_same_v<result_arg_type, const Result&>,
                  "The provided handler's last argument was expected to be of "
                  "type Result<T, E>& or const Result<T, E>&");

public:
    using result_type = typename adaptor_type::result_type;

    constexpr explicit ResultHandlerInvoker(Handler handler)
        : adaptor_(std::move(handler)) {}

    constexpr result_type operator()(Context& ctx, Result& result) {
        return adaptor_(ctx, result);
    }

private:
    adaptor_type adaptor_;
};

// Wraps a handler that may accept a context and value argument.
// e.g.
// - (value_type&)
// - (const value_type&)
// - (Context&, value_type&)
// - (Context&, const value_type&)
template <typename Handler, typename Result,
          typename T = typename Result::value_type>
class ValueHandlerInvoker {
    using adaptor_type =
        ContextAdaptor<Handler, Void, typename Result::error_type>;

    using args = boost::callable_traits::args_t<Handler>;
    static_assert(std::tuple_size_v<args> == 1 || std::tuple_size_v<args> == 2,
                  "The provided handler has wrong arguments number");

    using value_arg_type =
        std::tuple_element_t<adaptor_type::next_arg_index, args>;
    static_assert(std::is_same_v<value_arg_type, T&> ||
                      std::is_same_v<value_arg_type, const T&>,
                  "The provided handler's last argument was expected to be of "
                  "type T& or const T&");

public:
    using result_type = typename adaptor_type::result_type;

    constexpr explicit ValueHandlerInvoker(Handler handler)
        : adaptor_(std::move(handler)) {}

    constexpr result_type operator()(Context& ctx, Result& result) {
        return adaptor_(ctx, result.value());
    }

private:
    adaptor_type adaptor_;
};

// Wraps a handler that may accept a context and error argument.
// e.g.
// - (error_type&)
// - (const error_type&)
// - (Context&, error_type&)
// - (Context&, const error_type&)
template <typename Handler, typename Result,
          typename E = typename Result::error_type>
class ErrorHandlerInvoker {
    using adaptor_type =
        ContextAdaptor<Handler, typename Result::value_type, Void>;

    using args = boost::callable_traits::args_t<Handler>;
    static_assert(std::tuple_size_v<args> == 1 || std::tuple_size_v<args> == 2,
                  "The provided handler has wrong arguments number");

    using error_arg_type =
        std::tuple_element_t<adaptor_type::next_arg_index, args>;
    static_assert(std::is_same_v<error_arg_type, E&> ||
                      std::is_same_v<error_arg_type, const E&>,
                  "The provided handler's last argument was expected to be of"
                  "type E& or const E&");

public:
    using result_type = typename adaptor_type::result_type;

    constexpr explicit ErrorHandlerInvoker(Handler handler)
        : adaptor_(std::move(handler)) {}

    constexpr result_type operator()(Context& ctx, Result& result) {
        return adaptor_(ctx, result.error());
    }

private:
    adaptor_type adaptor_;
};

// The continuation produced by `Promise::then()`
template <typename Promise, typename ResultHandler>
class ThenContinuation {
    using invoker_type =
        ResultHandlerInvoker<ResultHandler, typename Promise::result_type>;

public:
    constexpr ThenContinuation(Promise p, ResultHandler handler)
        : future_(std::move(p)), invoker_(std::move(handler)) {}

    constexpr typename invoker_type::result_type operator()(Context& ctx) {
        if (!future_(ctx)) {
            return AsyncPending{};
        }
        return invoker_(ctx, future_.result());
    }

private:
    FutureImpl<Promise> future_;
    invoker_type invoker_;
};

// The continuation produced by `Promise::and_then()`
template <typename Promise, typename ValueHandler>
class AndThenContinuation {
    using invoker_type =
        ValueHandlerInvoker<ValueHandler, typename Promise::result_type>;

public:
    constexpr AndThenContinuation(Promise p, ValueHandler handler)
        : future_(std::move(p)), invoker_(std::move(handler)) {}

    constexpr typename invoker_type::result_type operator()(Context& ctx) {
        if (!future_(ctx)) {
            return AsyncPending{};
        } else if (future_.is_error()) {
            return AsyncError(future_.take_error());
        }
        return invoker_(ctx, future_.result());
    }

private:
    FutureImpl<Promise> future_;
    invoker_type invoker_;
};

// The continuation produced by `Promise::or_else()`
template <typename Promise, typename ErrorHandler>
class OrElseContinuation {
    using invoker_type =
        ErrorHandlerInvoker<ErrorHandler, typename Promise::result_type>;

public:
    constexpr OrElseContinuation(Promise p, ErrorHandler handler)
        : future_(std::move(p)), invoker_(std::move(handler)) {}

    constexpr typename invoker_type::result_type operator()(Context& ctx) {
        if (!future_(ctx)) {
            return AsyncPending{};
        } else if (future_.is_ok()) {
            return AsyncOk(future_.take_value());
        }
        return invoker_(ctx, future_.result());
    }

private:
    FutureImpl<Promise> future_;
    invoker_type invoker_;
};

// The continuation produced by `Promise::inspect()`
template <typename Promise, typename InspectHandler>
class InspectContinuation {
public:
    constexpr InspectContinuation(Promise p, InspectHandler handler)
        : promise_(std::move(p)), inspector_(std::move(handler)) {}

    constexpr typename Promise::result_type operator()(Context& ctx) {
        auto result = promise_(ctx);
        if (result) {
            if constexpr (std::is_invocable_v<
                              InspectHandler,
                              std::add_lvalue_reference_t<
                                  typename Promise::result_type>>) {
                inspector_(result);
            } else {
                inspector_(ctx, result);
            }
        }
        return result;
    }

private:
    Promise promise_;
    InspectHandler inspector_;
};

// The continuation produced by `Promise::discard_result()`
template <typename Promise>
class DiscardResultContinuation {
public:
    constexpr explicit DiscardResultContinuation(Promise p)
        : promise_(std::move(p)) {}

    constexpr AsyncResult<Void, Void> operator()(Context& ctx) {
        if (!promise_(ctx)) {
            return AsyncPending{};
        }
        return AsyncOk(Void{});
    }

private:
    Promise promise_;
};

// The continuation produced by `make_promise()`
template <typename PromiseHandler>
using PromiseContinuation = ContextHandlerInvoker<PromiseHandler>;

// The continuation produced by `make_result_promise()`
template <typename T, typename E>
class ResultContinuation {
public:
    constexpr explicit ResultContinuation(AsyncResult<T, E> result)
        : result_(std::move(result)) {}

    constexpr AsyncResult<T, E> operator()(Context& ctx) {
        (void)ctx;
        return std::move(result_);
    }

private:
    AsyncResult<T, E> result_;
};

// The continuation produced by `join_promises()`
template <typename... Promises>
class JoinContinuation {
public:
    constexpr JoinContinuation(Promises... promises)
        : futures_(std::move(promises)...) {}

    constexpr auto operator()(Context& ctx) {
        return helper(ctx, std::index_sequence_for<Promises...>{});
    }

private:
    template <std::size_t... Is>
    constexpr auto helper(Context& ctx, std::index_sequence<Is...>)
        -> AsyncResult<std::tuple<typename Promises::result_type...>, Void> {
        bool comps[] = {std::get<Is>(futures_)(ctx)...};
        if (std::all_of(std::begin(comps), std::end(comps),
                        [](bool b) { return b; })) {
            return AsyncOk(
                std::make_tuple(std::get<Is>(futures_).take_result()...));
        }
        return AsyncPending{};
    }

private:
    std::tuple<FutureImpl<Promises>...> futures_;
};

// The continuation produced by `join_promise_vector()`
template <typename Promise>
class JoinVectorContinuation {
public:
    constexpr explicit JoinVectorContinuation(std::vector<Promise> promises)
        : promises_(std::move(promises)), results_(promises_.size()) {}

    constexpr auto operator()(Context& ctx)
        -> AsyncResult<std::vector<typename Promise::result_type>, Void> {
        bool done = true;
        for (std::size_t i = 0; i < promises_.size(); ++i) {
            if (!results_[i]) {
                results_[i] = promises_[i](ctx);
                done &= static_cast<bool>(results_[i]);
            }
        }

        if (done) {
            return AsyncOk(std::move(results_));
        }
        return AsyncPending{};
    }

private:
    std::vector<Promise> promises_;
    std::vector<typename Promise::result_type> results_;
};

} // namespace internal
} // namespace bipolar

#endif
