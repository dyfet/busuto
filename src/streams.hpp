// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "system.hpp"

#include <cstring>

namespace busuto::system {
template <std::size_t S>
class streambuf : public std::streambuf {
public:
    explicit streambuf(int fd, close_t fn) : handle_(fd, fn) {
        setg(inbuf_, inbuf_, inbuf_);
        setp(outbuf_, outbuf_ + S);
    }

    explicit streambuf(int fd) : handle_(fd) {
        setg(inbuf_, inbuf_, inbuf_);
        setp(outbuf_, outbuf_ + S);
    }

    ~streambuf() override {
        this->sync();
    }

    auto handle() noexcept -> handle_t& { return handle_; }

    auto is_reaable() const noexcept {
        return gptr() < egptr() || handle_.is_readable();
    }

    auto is_writable() const noexcept {
        return handle_.is_writable();
    }

    auto zb_getbody(size_t n) -> std::string_view {
        while (static_cast<size_t>(this->egptr() - this->gptr()) < n) {
            if (this->zb_underflow() == traits_type::eof()) {
                return {}; // Not enough data, fail
            }
        }

        auto *start = this->gptr();
        this->gbump(static_cast<int>(n));
        return {start, n};
    }

    auto zb_getview(std::string_view delim = "\r\n") -> std::string_view {
        auto *start = this->gptr();
        auto *end = start;
        while (true) {
            auto avail = static_cast<size_t>(this->egptr() - end);
            if (avail < delim.size()) {
                auto tail = std::min(avail, delim.size() - 1);
                if (std::string_view(end, tail) == delim.substr(0, tail)) {
                    if (this->zb_underflow() == traits_type::eof()) return {};
                    continue;
                }
                ++end;
                continue;
            }

            if (std::string_view(end, delim.size()) == delim) {
                this->gbump(static_cast<int>((end - start) + delim.size()));
                return {start, end - start};
            }
            ++end;
        }
    }

protected:
    virtual auto sys_read(void *buf, size_t n) -> ssize_t { return ::read(handle_, buf, n); }
    virtual auto sys_write(const void *buf, size_t n) -> ssize_t { return ::write(handle_, buf, n); }

    auto flush_output() {
        auto n = pptr() - pbase();
        if (n > 0) {
            if (!handle_.is_writable()) return false;
            auto written = this->sys_write(pbase(), n);
            if (written != n) return false;
        }
        setp(outbuf_, outbuf_ + S);
        return true;
    }

    virtual auto zb_underflow() -> int_type {
        if (!handle_.is_readable()) return traits_type::eof();
        auto *start = gptr();
        auto *end = egptr();
        auto unread = static_cast<size_t>(end - start);
        if (unread > 0 && start != inbuf_) {
            std::memmove(inbuf_, start, unread);
        }

        auto n = this->sys_read(inbuf_ + unread, S - unread);
        if (n <= 0) return traits_type::eof();
        setg(inbuf_, inbuf_, inbuf_ + unread + n);
        return traits_type::to_int_type(*gptr());
    }

    auto underflow() -> int_type override {
        if (!handle_.is_readable()) return traits_type::eof();
        ssize_t n = this->sys_read(inbuf_, S);
        if (n <= 0) return traits_type::eof();
        setg(inbuf_, inbuf_, inbuf_ + n);
        return traits_type::to_int_type(*gptr());
    }

    auto overflow(int_type ch) -> int_type override {
        if (ch != traits_type::eof()) {
            *this->pptr() = ch;
            this->pbump(1);
        }
        return flush_output() ? ch : traits_type::eof();
    }

    auto sync() -> int override {
        return flush_output() ? 0 : -1;
    }

    auto xsputn(const char_type *s, std::streamsize count) -> std::streamsize override {
        std::streamsize written = 0;
        while (written < count) {
            std::streamsize space = epptr() - pptr();
            if (space == 0) {
                if (!flush_output()) break;
                space = epptr() - pptr();
            }

            std::streamsize chunk = std::min(space, count - written);
            std::memcpy(pptr(), s + written, chunk);
            pbump(static_cast<int>(chunk));
            written += chunk;
        }
        return written;
    }

    auto xsgetn(char_type *s, std::streamsize count) -> std::streamsize override {
        std::streamsize total = 0;
        while (total < count) {
            std::streamsize avail = egptr() - gptr();
            if (avail == 0) {
                if (underflow() == traits_type::eof())
                    break;
                avail = egptr() - gptr();
                if (avail == 0)
                    break; // underflow didn't refill
            }

            std::streamsize chunk = std::min(avail, count - total);
            std::memcpy(s + total, gptr(), chunk);
            gbump(static_cast<int>(chunk));
            total += chunk;
        }
        return total;
    }

private:
    handle_t handle_;
    char inbuf_[S]{};
    char outbuf_[S]{};
};
} // namespace busuto::system

namespace busuto {
template <std::size_t S = 1024>
class system_stream : public std::iostream {
public:
    explicit system_stream(int fd, close_t fn = [](int fd) { ::close(fd); }) : buf_(fd, fn), std::iostream(&buf_) {}

    system_stream(const system_stream&) = delete;
    auto operator=(const system_stream&) -> system_stream& = delete;

    auto is_readable() const noexcept {
        return !static_cast<bool>(!buf_.is_readable() || this->eof());
    }

    auto is_writable() const noexcept {
        if (this->fail()) return false;
        return buf_.is_writable();
    }

    auto getbody(size_t n) -> std::string_view { return buf_.zb_getbody(n); }
    auto getview(std::string_view delim = "\r\n") -> std::string_view { return buf_.zb_getview(delim); }

    template <typename F>
    auto apply(F func)
    requires requires(F f, handle_t& h) { f(h); }
    {
        return func(buf_.handle());
    }

private:
    system::streambuf<S> buf_;
};

template <std::size_t S = 1024>
inline auto make_stream(int fd, close_t fn = [](int fd) { ::close(fd); }) {
    return system_stream<S>(fd, fn);
}
}; // namespace busuto
