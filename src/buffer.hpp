// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "binary.hpp"
#include <istream>
#include <ostream>

namespace busuto::util {
class memorybuf final : public std::streambuf {
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
} // namespace busuto::util

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
    util::memorybuf buf_{};
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
    util::memorybuf buf_;
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
} // namespace busuto
