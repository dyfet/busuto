// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include <variant>

namespace busuto {
template <typename T, typename E>
class expected {
public:
    constexpr expected() = default;
    constexpr expected(const expected&) = default;
    constexpr explicit expected(const T& value) : value_(value) {}
    constexpr explicit expected(const E& error) : value_(error) {}

    constexpr auto operator=(const expected&) -> expected& = default;

    constexpr auto operator*() const -> const T& {
        return value();
    }

    constexpr auto operator*() -> T& {
        return value();
    }

    constexpr auto operator->() -> T * {
        return &(value());
    }

    constexpr auto operator->() const -> const T * {
        return &(value());
    }

    constexpr explicit operator bool() const {
        return has_value();
    }

    constexpr auto operator!() const {
        return !has_value();
    }

    constexpr auto has_value() const {
        return std::holds_alternative<T>(value_);
    }

    constexpr auto value() -> T& {
        return std::get<T>(value_);
    }

    constexpr auto value() const -> const T& {
        return std::get<T>(value_);
    }

    constexpr auto value_or(T& alt) -> T& {
        if (has_value()) return value();
        return alt;
    }

    constexpr auto error() const -> const E& {
        return std::get<E>(value_);
    }

private:
    std::variant<T, E> value_;
};
} // namespace busuto
