// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "binary.hpp"
#include "system.hpp"

#include <termios.h>

namespace busuto::io {
class system_input final {
public:
    system_input() : handle_(0) {
        if (isatty(0)) {
            tcgetattr(handle_, &original_);
            reset();
        } else {
            handle_ = -1;
        }
    }

    ~system_input() {
        if (handle_.is_open())
            tcsetattr(handle_, TCSANOW, &original_);
        handle_ = -1; // force invalid before lower layer
    }

    system_input(const system_input&) = delete;
    auto operator=(const system_input&) -> system_input& = delete;

    operator int() const noexcept { return handle_.get(); }
    explicit operator bool() const noexcept { return handle_.is_open(); }
    auto operator!() const noexcept { return !handle_.is_open(); }
    auto get() const noexcept { return handle_.get(); }

    auto fetch() noexcept -> struct termios * {
        return &current_;
    }

    void update(const struct termios *term = nullptr) noexcept {
        if (term)
            memcpy(&current_, term, sizeof(current_));
        tcsetattr(handle_, TCSANOW, &current_);
    }

    void reset() noexcept {
        if (handle_ < 0) return;
        current_ = original_;
        auto io_flags = fcntl(handle_, F_GETFL);
        current_.c_oflag = current_.c_lflag = 0;
        current_.c_cflag = CLOCAL | CREAD | HUPCL;
        current_.c_iflag = IGNBRK;

        memset(&current_.c_cc, 0, sizeof(current_.c_cc));
        current_.c_cc[VMIN] = 1;
        current_.c_cflag |= original_.c_cflag & (CRTSCTS | CSIZE | PARENB | PARODD | CSTOPB);
        current_.c_iflag |= original_.c_iflag & (IXON | IXANY | IXOFF);

        tcsetattr(handle_, TCSANOW, &current_);
        fcntl(handle_, F_SETFL, io_flags & ~O_NDELAY);
    }

private:
    handle_t handle_{-1};
    struct termios original_{}, current_{};
};

inline auto putch(int fd, int code) -> ssize_t {
    if (code < 9 || code > 255) return 0;
    auto buf = uint8_t(code & 0xff);
    return ::write(fd, &buf, 1);
}

inline auto getch(int fd, bool echo = false, int code = EOF, int eol = EOF) -> int {
    uint8_t buf{0};
    auto rtn = ::read(fd, &buf, 1);
    if (rtn < 1) return EOF;
    if (echo && code != EOF && (eol == EOF || (static_cast<int>(buf)) != eol))
        putch(fd, code);
    else if (echo)
        putch(fd, buf);
    return static_cast<int>(buf);
}

inline auto puts(int fd, const char *str) -> ssize_t {
    if (!str || !*str) return 0;
    return ::write(fd, str, strlen(str));
}

template <busuto::util::readable_binary Binary>
inline auto put(int fd, const Binary& from) {
    return ::write(fd, from.data(), from.size());
}

template <busuto::util::writable_binary Binary>
inline auto get(int fd, Binary& to) {
    return ::read(fd, to.data(), to.size());
}

inline auto getline(int fd, char *buf, std::size_t max, int eol = '\n', bool echo = false, int echo_code = EOF, const char *ignore = nullptr) {
    *buf = 0;
    --max;

    auto count = std::size_t(0);
    while (count < max) {
        auto code = getch(fd, echo, echo_code, eol);
        if (code == EOF) return count;
        if (ignore && strchr(ignore, code)) continue;
        buf[count++] = static_cast<char>(code);
        if (eol && code == eol) break;
    }
    buf[count] = 0;
    return count;
}

inline auto expect(int fd, const std::string_view& match) {
    auto count = 0U;
    while (count < match.size()) {
        auto code = getch(fd);
        if (code == EOF) return false;
        // strip lead-in noise...
        if (!count && match[0] != code) continue;
        if (match[count] != code) return false;
        ++count;
    }
    return true;
}

inline auto until(int fd, int match = EOF, unsigned max = 1) {
    auto count = 0U;
    while (count < max) {
        auto code = getch(fd);
        if (code == EOF) return false;
        if (match == EOF || code == match)
            ++count;
    };
    return true;
}
} // namespace busuto::io
