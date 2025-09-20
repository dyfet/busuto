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
template <typename T>
concept CharBuffer = requires(const T& t) {
    { t.data() } -> std::convertible_to<const char *>;
    { t.size() } -> std::convertible_to<std::size_t>;
};

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

    constexpr auto begin() const -> T * { return data_; }
    constexpr auto end() const -> T * { return data_ + N; }
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
        data_ = buf;
        size_ = 0;
    }

    void input(const char *buf, std::size_t size) {
        auto chr = const_cast<char *>(buf);
        setg(chr, chr, chr + size); // full input
        setp(nullptr, nullptr);     // empty output
        data_ = buf;
        size_ = size;
    }

    auto data() const noexcept {
        return data_;
    }

    auto size() const noexcept { // current size
        if (size_) return size_;
        return static_cast<std::size_t>(pptr() - data_);
    }

    auto used() const noexcept { // consumption count
        if (size_) return static_cast<std::size_t>(gptr() - data_);
        return size_t(0);
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
    const char *data_{nullptr};
    std::size_t size_{0};

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

constexpr auto strsize(const char *cp, std::size_t max = 256) -> std::size_t {
    std::size_t count = 0;
    while (cp && *cp && count < max) {
        ++count;
        ++cp;
    }
    return count;
}

auto memset(void *ptr, int value, std::size_t size) noexcept -> void *;
auto strcopy(char *cp, std::size_t max, const char *dp) noexcept -> std::size_t;
auto strcat(char *cp, std::size_t max, ...) noexcept -> std::size_t;
void strupper(char *cp, std::size_t max);
void strlower(char *cp, std::size_t max);
auto getline(std::istream& from, char *data, std::size_t size, char delim = '\n') -> std::size_t;

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

    auto data() const noexcept { return buf_.data(); }
    auto size() const noexcept { return buf_.size(); }
    auto used() const noexcept { return buf_.used(); }
    auto begin() const noexcept { return data(); }
    auto end() const noexcept { return data() + size(); }

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

    auto data() const noexcept { return buf_.data(); }
    auto size() const noexcept { return buf_.size(); }
    auto begin() const noexcept { return data(); }
    auto end() const noexcept { return data() + size(); }

private:
    safe::streambuf buf_;
};

template <std::size_t S>
class format_buffer final : public output_buffer {
public:
    format_buffer() : output_buffer(data_, S) {}
    explicit operator bool() { return size() > 0; }
    operator std::string() noexcept { return to_string(); }
    auto operator!() { return size() == 0; }
    auto operator()() -> std::string { return to_string(); }
    auto operator*() -> char * { return c_str(); }

    auto c_str() noexcept -> char * {
        data_[size()] = 0;
        return data_;
    }

    auto to_string() noexcept -> std::string {
        return std::string(c_str());
    }

private:
    char data_[S + 1]{0};
};

template <std::size_t S>
class stringbuf {
public:
    stringbuf() = default;
    ~stringbuf() { safe::memset(data_, 0, S); }

    template <safe::CharBuffer T>
    explicit stringbuf(const T& from) noexcept : size_(S) {
        if (from.size() < S) size_ = from.size();
        size_ = safe::strcopy(data_, size_ + 1, from.data());
        data_[size_] = 0;
    }

    explicit stringbuf(char ch, std::size_t s = S) noexcept : size_(s) {
        if (ch == 0) {
            size_ = 0;
            return;
        }
        safe::memset(data_, ch, size_);
        data_[size_] = 0;
    }

    explicit stringbuf(const char *cp) noexcept {
        size_ = std::min(S, safe::strsize(cp, S));
        size_ = std::min(S, safe::strcopy(data_, size_ + 1, cp));
        data_[size_] = 0;
    }

    auto operator-=(std::size_t size) -> stringbuf& { return trim(size); }
    operator char *() const noexcept { return data(); }
    operator std::string() const noexcept { return std::string(data_); }
    explicit operator std::string_view() const noexcept { return std::string_view(data_, size); }

    template <safe::CharBuffer T>
    auto operator=(const T& from) noexcept -> stringbuf& {
        size_ = std::min(S, from.size());
        size_ = safe::strcopy(data_, size_, from.data());
        data_[size_] = 0;
        return *this;
    }

    template <safe::CharBuffer T>
    auto operator+=(const T& from) -> stringbuf& {
        if (size_ == S) throw range("stringbuf full");
        size_ += safe::strcat(data_ + size_, S - size_, from.data());
        data_[size_] = 0;
        return *this;
    }

    auto operator+=(char ch) -> stringbuf& {
        if (size_ == S) throw range("stringbuf full");
        data_[size_++] = ch;
        data_[size_] = 0;
        return *this;
    }

    auto operator=(const char *cp) noexcept -> stringbuf& {
        size_ = std::min(S, safe::strsize(cp, S));
        size_ = std::min(S, safe::strcopy(data_, size_ + 1, cp));
        data_[size_] = 0;
        return *this;
    }

    auto operator+=(const char *cp) -> stringbuf& {
        if (size_ == S) throw range("stringbuf full");
        size_ += safe::strcat(data_ + size_, S - size_ + 1, cp);
        data_[size_] = 0;
        return *this;
    }

    template <safe::CharBuffer T>
    auto operator==(const T& from) const noexcept -> bool {
        if (from.size() != size_) return false;
        if (!size_) return true;
        return memcmp(data_, from.data(), size_);
    }

    template <safe::CharBuffer T>
    auto operator!=(const T& from) const noexcept -> bool {
        return !operator==(from);
    }

    auto operator==(const char *cp) const noexcept -> bool {
        size_t len = safe::strsize(cp, S);
        if (len != size_) return false;
        if (!len) return true;
        return memcmp(data_, cp, size_) == 0;
    }

    auto operator!=(const char *cp) const noexcept -> bool {
        return !operator==(cp);
    }

    constexpr auto operator[](size_t index) -> char& {
        if (index >= size_) throw range("index out of bounds");
        return data_[index];
    }

    constexpr auto operator[](size_t index) const -> const char& {
        if (index >= size_) throw range("index out of bounds");
        return data_[index];
    }

    auto upper() noexcept -> stringbuf& {
        safe::strupper(data_, size_);
        return *this;
    }

    auto lower() noexcept -> stringbuf& {
        safe::strlower(data_, size_);
        return *this;
    }

    auto getline(std::istream& from, char delim = '\n') -> stringbuf& {
        size_ = safe::getline(from, data_, S + 1, delim);
        return *this;
    }

    constexpr auto first() const {
        if (!size_) throw range("stringbuf empty");
        return data_[0];
    }

    constexpr auto last() const {
        if (!size_) throw range("stringbuf empty");
        return data_[size_ - 1];
    }

    constexpr auto begin() const -> char * { return data_; }
    constexpr auto end() const -> char * { return data_ + size_; }
    constexpr auto data() noexcept -> char * { return data_; }
    constexpr auto data() const noexcept -> char * { return data_; }
    constexpr auto size() const noexcept { return size_; }
    constexpr auto capacity() const noexcept { return S; }
    constexpr auto empty() const noexcept { return !size_; }
    constexpr auto full() const noexcept { return size_ >= S; }

    auto clear() noexcept -> stringbuf& {
        size_ = 0;
        data_[0] = 0;
        return *this;
    }

    auto chop(std::size_t prefix) noexcept -> stringbuf& {
        if (prefix >= size_)
            size_ = 0;
        else if (prefix) {
            memmove(data_, data_ + prefix, size_ - prefix);
            size_ -= prefix;
        }
        data_[size_] = 0;
        return *this;
    }

    auto trim(std::size_t size) -> stringbuf& {
        if (size > size_) throw range("trim too large");
        size_ -= size;
        data_[size_] = 0;
        return *this;
    }

    auto start_with(const char *list) const noexcept -> char * {
        if (!list) return data_;
        std::size_t pos{0};
        while (pos < size_) {
            if (strchar(list, data_[pos]))
                return data_ + pos;
            ++pos;
        }
        return data_ + size_;
    }

    auto start_after(const char *list) const noexcept -> char * {
        if (!list) return data_;
        std::size_t pos{0};
        while (pos < size_) {
            if (!strchar(list, data_[pos]))
                return data_ + pos;
            ++pos;
        }
        return data_ + size_; // null byte at end, valid ptr
    }

    auto trim(const char *str) noexcept -> stringbuf& {
        if (!str) return *this;
        ;
        auto tsize = safe::strsize(str, S);
        if (tsize > size_) return false;
        if (memcmp(data_ + size_ - tsize, str, tsize) != 0) return false;
        size_ -= tsize;
        data_[size_] = 0;
        return *this;
    }

    auto output() {
        return output_buffer(data_, S);
    }

    auto input() const {
        return input_buffer(data_, size_);
    }

    auto update(const output_buffer& buffer) {
        if (buffer.data() == data_) {
            size_ = buffer.size();
            data_[size_] = 0;
            return true;
        }
        return false;
    }

    auto consume(const input_buffer& buffer) {
        if (buffer.data() == data_) {
            chop(buffer.used());
            return true;
        }
        return false;
    }

    template <typename Func>
    requires requires(Func f, char *d, std::size_t s) {{ f(d, s) }; }
    auto apply(Func func) {
        auto result = func(data_, S);
        size_ = safe::strsize(data_, S);
        data_[size_] = 0;
        return result;
    }

private:
    std::size_t size_{0};
    char data_[S + 1]{0};
};

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
} // namespace busuto
