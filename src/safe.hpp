// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "binary.hpp"

#include <cstring>
#include <cstdlib>
#include <cstdarg>
#include <streambuf>
#include <istream>
#include <ostream>

namespace busuto::safe {
constexpr auto eq(const char *p1, const char *p2) {
    if (!p1 && !p2) return true;
    if (!p1 || !p2) return false;
    return strcmp(p1, p2) == 0;
}

constexpr auto eq(const char *p1, const char *p2, std::size_t len) {
    if (!p1 && !p2) return true;
    if (!p1 || !p2) return false;
    return strncmp(p1, p2, len) == 0;
}

constexpr auto size(const char *cp, std::size_t max = 256) -> std::size_t {
    std::size_t count = 0;
    while (cp && *cp && count < max) {
        ++count;
        ++cp;
    }
    return count;
}

class streambuf final : public std::streambuf {
public:
    void output(char *buf, size_t size) {
        setg(nullptr, nullptr, nullptr); // empty input
        setp(buf, buf + size);           // allocated output
    }

    void input(const char *buf, size_t size) {
        auto chr = const_cast<char *>(buf);
        setg(chr, chr, chr + size); // full input
        setp(nullptr, nullptr);     // empty output
    }

private:
    auto underflow() -> int_type final {
        if (gptr() < egptr()) return traits_type::to_int_type(*this->gptr());
        return traits_type::eof();
    }

    auto overflow(int_type ch) -> int_type final {
        if (pptr() == epptr() || ch == traits_type::eof())
            return traits_type::eof();
        if (ch != traits_type::eof()) {
            *pptr() = traits_type::to_char_type(ch);
            this->pbump(1);
        }
        return traits_type::not_eof(ch);
    }

    auto sync() -> int final {
        return 0;
    }
};

auto memset(void *ptr, int value, size_t size) noexcept -> void *;
auto copy(char *cp, std::size_t max, std::string_view view) noexcept -> std::size_t;
auto append(char *cp, std::size_t max, ...) noexcept -> bool;
} // namespace busuto::safe

namespace busuto {
class input_buffer : public std::istream {
public:
    input_buffer() = delete;

    input_buffer(const void *mem, std::size_t size) : std::istream(&buf_) {
        buf_.input(static_cast<const char *>(mem), size);
    }

    template <util::readable_binary Binary>
    explicit input_buffer(Binary& bin) : std::istream(&buf_) {
        buf_.input(static_cast<const char *>(bin.data()), bin.size());
    }

private:
    safe::streambuf buf_{};
};

class output_buffer : public std::ostream {
public:
    output_buffer() = delete;

    output_buffer(void *mem, std::size_t size) : std::ostream(&buf_) {
        buf_.output(static_cast<char *>(mem), size);
    }

    template <util::writable_binary Binary>
    explicit output_buffer(Binary& bin) : std::ostream(&buf_) {
        buf_.output(static_cast<char *>(bin.data()), bin.size());
    }

private:
    safe::streambuf buf_;
};
} // namespace busuto
