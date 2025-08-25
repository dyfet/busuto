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

    auto handle() noexcept -> handle_t& { return handle_; }
    auto zb_data() const noexcept { return gptr(); }
    auto zb_size() const noexcept { return static_cast<std::size_t>(egptr() - gptr()); }

    auto reaable() const noexcept {
        return gptr() < egptr() || handle_.readable();
    }

    auto writable() const noexcept {
        return handle_.writable();
    }

    auto zb_getbody(std::size_t n) -> std::string_view {
        while (static_cast<size_t>(egptr() - gptr()) < n) {
            if (zb_underflow() == traits_type::eof()) {
                return {}; // Not enough data, fail
            }
        }

        auto *start = gptr();
        gbump(static_cast<int>(n));
        return {start, n};
    }

    auto zb_getview(std::string_view delim = "\r\n") -> std::string_view {
        while (true) {
            auto *start = gptr();
            auto *end = start;
            while (true) {
                auto avail = static_cast<size_t>(egptr() - end);
                if (avail < delim.size()) {
                    if (avail == 0) {
                        if (zb_underflow() == traits_type::eof()) return {};
                        break;
                    }

                    auto tail = avail;
                    if (std::string_view(end, tail) == delim.substr(0, tail)) {
                        if (zb_underflow() == traits_type::eof()) return {};
                        break;
                    }
                    ++end;
                    continue;
                }
                if (std::string_view(end, delim.size()) == delim) {
                    gbump(static_cast<int>((end - start) + delim.size()));
                    return {start, end - start};
                }
                ++end;
            }
        }
    }

    // This is useful to re-frame input data when streaming packets
    auto zb_reset(std::size_t consume = 0) -> bool {
        auto *start = gptr();
        auto *end = egptr();
        if (start + consume <= end)
            start += consume;
        else
            return false;

        auto unread = static_cast<size_t>(end - start);
        if (unread && start > inbuf_)
            std::memmove(inbuf_, start, unread);
        setg(inbuf_, inbuf_, inbuf_ + unread);
        if (unread >= S) return true;
        auto n = sys_read(inbuf_ + unread, S - unread);
        if (n <= 0) return unread > 0;
        setg(inbuf_, inbuf_, inbuf_ + unread + n);
        return true;
    }

protected:
    virtual auto sys_read(void *buf, std::size_t n) -> ssize_t { return ::read(handle_, buf, n); }
    virtual auto sys_write(const void *buf, std::size_t n) -> ssize_t { return ::write(handle_, buf, n); }

    auto underflow() -> int_type override {
        if (gptr() < egptr()) return traits_type::to_int_type(*gptr());
        if (!handle_.readable()) return traits_type::eof();
        ssize_t n = sys_read(inbuf_, S);
        if (n <= 0) return traits_type::eof();
        setg(inbuf_, inbuf_, inbuf_ + n);
        return traits_type::to_int_type(*gptr());
    }

    auto overflow(int ch) -> int override {
        if (ch == traits_type::eof()) {
            if (sync() == 0) return traits_type::not_eof(ch);
            return traits_type::eof();
        }

        if (pptr() == epptr() && sync() != 0) return traits_type::eof();
        *pptr() = traits_type::to_char_type(ch);
        pbump(1);
        return ch;
    }

    auto sync() -> int override {
        auto n = pptr() - pbase();
        if (n == 0) return 0; // nothing to flush
        if (!handle_.writable()) return -1;
        auto written = sys_write(pbase(), n);
        if (written < 0 || written > n) return -1;
        if (written < n) {
            std::memmove(outbuf_, pbase() + written, n - written);
            setp(outbuf_ + (n - written), outbuf_ + S);
            return -1;
        }

        setp(outbuf_, outbuf_ + S);
        return 0;
    }

    auto xsputn(const char_type *s, std::streamsize count) -> std::streamsize override {
        std::streamsize written = 0;
        while (written < count) {
            std::streamsize space = epptr() - pptr();
            if (space == 0) {
                if (sync()) break;
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

    auto zb_underflow() -> int_type {
        if (!handle_.readable()) return traits_type::eof();
        auto *start = gptr();
        auto *end = egptr();
        auto unread = static_cast<size_t>(end - start);
        if (unread > 0 && start != inbuf_) {
            std::memmove(inbuf_, start, unread);
        }

        auto n = sys_read(inbuf_ + unread, S - unread);
        if (n <= 0) return traits_type::eof();
        setg(inbuf_, inbuf_, inbuf_ + unread + n);
        return traits_type::to_int_type(*gptr());
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

    auto readable() const noexcept {
        return !static_cast<bool>(!buf_.readable() || eof());
    }

    auto writable() const noexcept {
        if (fail()) return false;
        return buf_.writable();
    }

    auto data() const noexcept { return buf_.zb_data(); }
    auto size() const noexcept { return buf_.zb_size(); }
    auto begin() const { return buf_.zb_data(); }
    auto end() const { return buf_.zb_data() + buf_.zb_size(); }
    auto reset(std::size_t size = 0) { return buf_.zb_reset(size); }
    void close() { buf_.handle().close(); }
    auto getbody(size_t n) { return buf_.zb_getbody(n); }
    auto getview(std::string_view delim = "\r\n") { return buf_.zb_getview(delim); }

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
