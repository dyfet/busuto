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
#include <array>

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

template <typename T, std::size_t N, std::size_t Offset = 0>
class slots {
public:
    using reference = T&;
    using const_reference = const T&;
    using size_type = typename std::size_t;

    constexpr slots() = default;
    slots(const slots& from) = delete;
    auto operator=(const slots& from) -> slots& = delete;

    constexpr auto operator[](size_type index) -> T& {
        if (index < Offset || index >= N + Offset) throw range("Index out of range");
        return data_[index - Offset];
    }

    constexpr auto operator[](size_type index) const -> const T& {
        if (index < Offset || index >= N + Offset) throw range("Index out of range");
        return data_[index - Offset];
    }

    constexpr auto at(size_type index) -> T& {
        if (index < Offset || index >= N + Offset) throw range("Index out of range");
        return data_[index - Offset];
    }

    constexpr auto at(size_type index) const -> const T& {
        if (index < Offset || index >= N + Offset) throw range("Index out of range");
        return data_[index - Offset];
    }

    constexpr auto min() const noexcept { return Offset; }
    constexpr auto max() const noexcept { return size_type(Offset + N - 1); }

private:
    T data_[N]{}; // assumes initalized data...
};

class streambuf final : public std::streambuf {
public:
    void output(char *buf, std::size_t size) {
        setg(nullptr, nullptr, nullptr); // empty input
        setp(buf, buf + size);           // allocated output
    }

    void input(const char *buf, std::size_t size) {
        auto chr = const_cast<char *>(buf);
        setg(chr, chr, chr + size); // full input
        setp(nullptr, nullptr);     // empty output
    }

    auto readable() const noexcept {
        return gptr() < egptr();
    }

    auto writable() const noexcept {
        return pptr() != epptr();
    }

    auto zb_getbody(std::size_t n) -> std::string_view {
        if ((static_cast<std::size_t>(egptr() - gptr())) >= n) {
            auto ptr = gptr();
            gbump(int(n));
            return {ptr, n};
        }
        return {};
    }

    auto zb_getview(std::string_view delim = "\r\n") -> std::string_view {
        auto *start = gptr();
        auto *end = start;
        while (true) {
            auto avail = static_cast<size_t>(egptr() - end);
            if (avail < delim.size()) return {};
            if (std::string_view(end, delim.size()) == delim) {
                gbump(static_cast<int>((end - start) + delim.size()));
                return {start, static_cast<std::size_t>(end - start)};
            }
            ++end;
        }
    }

private:
    auto underflow() -> int_type final {
        if (gptr() < egptr()) return traits_type::to_int_type(*gptr());
        return traits_type::eof();
    }

    auto overflow(int_type ch) -> int_type final {
        if (pptr() == epptr() || ch == traits_type::eof())
            return traits_type::eof();
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
        return traits_type::not_eof(ch);
    }

    auto sync() -> int final {
        return 0;
    }
};

auto memset(void *ptr, int value, std::size_t size) noexcept -> void *;
auto copy(char *cp, std::size_t max, std::string_view view) noexcept -> std::size_t;
auto append(char *cp, std::size_t max, ...) noexcept -> bool;

template <typename T>
inline void zero(T *ptr) noexcept {
    safe::memset(ptr, 0, sizeof(T));
}

template <typename T>
inline void freep(T **ptr) noexcept {
    if (!ptr || !*ptr) return;
    ::free(*ptr); // NOLINT
    *ptr = nullptr;
}

template <typename T>
inline void newp(T **ptr, std::size_t extra = 0) noexcept {
    if (!ptr) return;
    freep(ptr);
    *ptr = static_cast<T *>(::malloc(sizeof(T) + extra)); // NOLINT
}

template <typename T>
inline void newarray(T **ptr, std::size_t count = 1) noexcept {
    freep(ptr);
    if (count && ptr)
        *ptr = static_cast<T *>(::malloc(sizeof(T) * count)); // NOLINT
}
} // namespace busuto::safe

namespace busuto {
class input_buffer : public std::istream {
public:
    input_buffer() = delete;

    input_buffer(const void *mem, std::size_t size) : std::istream(&buf_) {
        buf_.input(static_cast<const char *>(mem), size);
    }

    auto is_open() const noexcept { return buf_.readable(); }
    auto getbody(size_t n) { return buf_.zb_getbody(n); }
    auto getview(std::string_view delim = "\r\n") { return buf_.zb_getview(delim); }

    template <util::readable_binary Binary>
    explicit input_buffer(const Binary& bin) : std::istream(&buf_) {
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

    auto is_open() const noexcept { return buf_.writable(); }

    template <util::writable_binary Binary>
    explicit output_buffer(Binary& bin) : std::ostream(&buf_) {
        buf_.output(static_cast<char *>(bin.data()), bin.size());
    }

private:
    safe::streambuf buf_;
};
} // namespace busuto
