// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "output.hpp"
#include "sockets.hpp"
#include "system.hpp"
#include "binary.hpp"
#include "fsys.hpp"

#include <format>

namespace busuto {
template <class... Args>
void print(std::string_view fmt, Args&&...args) {
    auto bound = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);
    std::apply([&](auto&&...unpacked) {
        std::cout << std::vformat(fmt, std::make_format_args(unpacked...));
    },
    bound);
}

template <typename Stream, typename... Args>
requires std::is_base_of_v<std::ostream, std::decay_t<Stream>>
auto print(Stream&& out, std::string_view fmt, Args&&...args) -> Stream& { // NOLINT
    auto bound = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);
    std::apply([&](auto&&...unpacked) {
        out << std::vformat(fmt, std::make_format_args(unpacked...));
    },
    bound);
    return out;
}

#ifdef NDEBUG
template <typename... Args>
void debug(std::string_view fmt, Args&&...args) {} // NOLINT
#else
template <typename... Args>
void debug(std::string_view fmt, Args&&...args) { // NOLINT
    auto bound = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);
    std::apply([&](auto&&...unpacked) {
        output::debug() << std::vformat(fmt, std::make_format_args(unpacked...));
    },
    bound);
}
#endif
} // namespace busuto

namespace std {
template <>
struct formatter<busuto::socket::address, char> {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const busuto::socket::address& b, std::format_context& ctx) const { // NOLINT
        return std::format_to(ctx.out(), "{}", b.to_string());
    }
};

template <>
struct formatter<busuto::fsys::path, char> { // NOLINT
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const busuto::fsys::path& p, std::format_context& ctx) const { // NOLINT
        return std::format_to(ctx.out(), "{}", p.string());
    }
};

template <>
struct formatter<busuto::byte_array, char> {
    static constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const busuto::byte_array& b, std::format_context& ctx) const { // NOLINT
        if (!is(b))
            return std::format_to(ctx.out(), "nil");
        return std::format_to(ctx.out(), "{}", b.to_hex());
    }
};
} // namespace std
