// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "system.hpp"

#include <cstring>

namespace busuto::system {
template <std::size_t S>
class streambuf : public std::streambuf {
public:
    explicit streambuf(int fd, close_t fn = [](int fd) { ::close(fd); }) : handle_(fd, fn) {
        setg(inbuf_, inbuf_, inbuf_);
        setp(outbuf_, outbuf_ + S);
    }

    ~streambuf() override {
        this->sync();
    }

protected:
    auto flush_output() {
        ssize_t n = pptr() - pbase();
        if (n > 0) {
            ssize_t written = write(handle_, pbase(), n);
            if (written != n) return false;
        }
        setp(outbuf_, outbuf_ + S);
        return true;
    }

    auto underflow() -> int_type override {
        ssize_t n = read(handle_, inbuf_, S);
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

private:
    system::streambuf<S> buf_;
};

template <std::size_t S = 1024>
class input_stream : public std::istream {
public:
    explicit input_stream(int fd = 0, close_t fn = [](int fd) { ::close(fd); }) : buf_(fd, fn), std::istream(&buf_) {}

    input_stream(const input_stream&) = delete;
    auto operator=(const input_stream&) -> input_stream& = delete;

private:
    system::streambuf<S> buf_;
};

template <std::size_t S = 1024>
class output_stream : public std::ostream {
public:
    explicit output_stream(int fd = 1, close_t fn = [](int fd) { ::close(fd); }) : buf_(fd, fn), std::istream(&buf_) {}

    output_stream(const output_stream&) = delete;
    auto operator=(const output_stream&) -> output_stream& = delete;

private:
    system::streambuf<S> buf_;
};

template <std::size_t S = 1024>
inline auto make_stream(int fd, close_t fn = [](int fd) { ::close(fd); }) {
    return system_stream<S>(fd, fn);
}

template <std::size_t S = 1024>
inline auto make_input(int fd = 0, close_t fn = [](int fd) { ::close(fd); }) {
    return input_stream<S>(fd, fn);
}

template <std::size_t S = 1024>
inline auto make_output(int fd = 1, close_t fn = [](int fd) { ::close(fd); }) {
    return output_stream<S>(fd, fn);
}
}; // namespace busuto
