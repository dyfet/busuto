// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include <type_traits>
#include <concepts>
#include <functional>
#include <stdexcept>
#include <utility>
#include <cstdint>

#if __cplusplus < 202001L
#error C++20 compliant compiler (or later) required
#endif

namespace busuto {
using error = std::runtime_error;
using range = std::out_of_range;
using invalid = std::invalid_argument;
using overflow = std::overflow_error;

template <typename T>
constexpr auto is(const T& object) {
    return static_cast<bool>(object);
}

template <typename T>
constexpr auto is_null(const T& ptr) {
    if constexpr (std::is_pointer_v<T>)
        return ptr == nullptr;
    else
        return !static_cast<bool>(ptr);
}

template <typename Func, typename... Args>
requires std::invocable<Func, Args...>
inline auto try_function(Func&& func, std::invoke_result_t<Func, Args...> fallback, Args&&...args) -> std::invoke_result_t<Func, Args...> {
    try {
        return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    } catch (...) {
        return fallback;
    }
}
} // namespace busuto

namespace busuto::util {
template <typename T = void>
constexpr auto offset_ptr(void *base, std::size_t offset) -> T * {
    return reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}

template <typename T>
constexpr auto is_within_bounds(const T *ptr, const T *base, std::size_t count) {
    return ptr >= base && ptr < base + count;
}

template <typename Range, typename T>
constexpr auto count(const Range& range, const T& value) noexcept -> std::size_t {
    std::size_t result = 0;
    for (const auto& element : range)
        // cppcheck-suppress useStlAlgorithm
        result += static_cast<std::size_t>(element == value);
    return result;
}

template <typename T>
constexpr auto pow(T base, T exp) {
    static_assert(std::is_integral_v<T>, "pow requires integral types");
    T result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

class init final {
public:
    using run_t = void (*)();

    init(const init&) = delete;
    auto operator=(const init&) -> init& = delete;

    explicit init(const run_t start, const run_t stop = {[]() {}}) : exit_(stop) {
        (start)();
    }

    ~init() {
        (exit_)();
    }

private:
    const run_t exit_{[]() {}};
};

template <typename Func, typename... Args>
requires std::is_invocable_v<Func, Args...>
class defer_scope final {
public:
    explicit defer_scope(Func func, Args&&...args) noexcept((std::is_nothrow_constructible_v<Func, Func&&> && ... && std::is_nothrow_constructible_v<Args, Args&&>)) : func_(std::move(func)), args_(std::forward<Args>(args)...) {}

    ~defer_scope() noexcept(std::is_nothrow_invocable_v<Func, Args...>) {
        if constexpr (sizeof...(Args) == 0) {
            std::move(func_)();
        } else {
            std::apply([this](auto&&...a) {
                std::invoke(std::move(func_), std::forward<decltype(a)>(a)...);
            },
            std::move(args_));
        }
    }

    defer_scope(const defer_scope&) = delete;
    defer_scope(defer_scope&&) = delete;
    auto operator=(const defer_scope&) -> defer_scope& = delete;
    auto operator=(defer_scope&&) -> defer_scope& = delete;

private:
    Func func_;
    std::tuple<Args...> args_;
};
} // namespace busuto::util
