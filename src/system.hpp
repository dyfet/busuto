// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "common.hpp"

#ifdef _WIN32
#if _WIN32_WINNT < 0x0600 && !defined(_MSC_VER)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // NOLINT
#endif
#define USE_CLOSESOCKET
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#ifdef AF_UNIX
#include <afunix.h>
#endif
#endif

#include <chrono>
#include <string>
#include <memory>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <ctime>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>

#ifndef _WIN32
#include <termios.h>
#include <poll.h>
#endif

#if __has_include(<sys/eventfd.h>)
#include <sys/eventfd.h>
#endif

namespace busuto::util {
struct file_closer {
    void operator()(FILE *f) const noexcept {
        if (f) fclose(f);
    }
};

struct pipe_closer {
    void operator()(FILE *f) const noexcept {
        if (f) pclose(f);
    }
};
} // namespace busuto::util

namespace busuto::system {
using timepoint = std::chrono::steady_clock::time_point;
using duration = std::chrono::steady_clock::duration;
using file_ptr = std::unique_ptr<FILE, util::file_closer>;
using pipe_ptr = std::unique_ptr<FILE, util::pipe_closer>;

#ifndef _WIN32
class notify_t final {
public:
    notify_t(const notify_t& from) = delete;

    notify_t() noexcept {
#ifdef EFD_NONBLOCK
        pipe_[0] = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if (pipe_[0] == -1) return;
        pipe_[1] = pipe_[0];
#else
        if (pipe(pipe_) == -1) return;
        fcntl(pipe_[0], F_SETFL, O_NONBLOCK);
        fcntl(pipe_[1], F_SETFL, O_NONBLOCK);
#endif
    }

    ~notify_t() { release(); }

    auto is_open() const noexcept { return pipe_[0] != -1; }
    auto handle() const noexcept { return pipe_[0]; }

    auto clear() noexcept {
        if (pipe_[0] == -1) return false;
#ifdef EFD_NONBLOCK
        uint64_t count{0};
        const auto rtn = int(::read(pipe_[0], &count, sizeof(count)));
#else
        char buf[64]{};
        const auto rtn = int(::read(pipe_[0], buf, sizeof(buf)));
#endif
        if (rtn < 0) {
            release();
            return false;
        }
        return rtn > 0;
    }

    auto wait(int timeout = -1) noexcept {
        if (pipe_[0] == -1) return false;
        struct pollfd pfd = {.fd = pipe_[0], .events = POLLIN};
        auto rtn = ::poll(&pfd, 1, timeout);
        if (rtn < 0) {
            release();
            return false;
        }
        return rtn > 0;
    }

    auto signal() noexcept {
        if (pipe_[0] == -1) return false;
#ifdef EFD_NONBLOCK
        uint64_t one = 1;
        const auto rtn = int(::write(pipe_[1], &one, sizeof(one)));
#else
        const auto rtn = int(::write(pipe_[1], "x", 1));
#endif
        if (rtn < 0) {
            release();
            return false;
        }
        return rtn > 0;
    }

    auto operator=(const notify_t& from) -> notify_t& = delete;
    operator int() const noexcept { return pipe_[0]; } // select / poll

private:
    int pipe_[2]{-1, -1};

    void release() noexcept {
        if (pipe_[0] == -1) return;
        if (pipe_[0] != pipe_[1]) ::close(pipe_[1]);
        ::close(pipe_[0]);
        pipe_[0] = pipe_[1] = -1;
    }
};
#endif

#ifndef _WIN32
inline auto page_size() noexcept -> off_t {
    return sysconf(_SC_PAGESIZE);
}
#endif

inline void time_of_day(struct timeval *tp) noexcept {
    gettimeofday(tp, nullptr);
}

inline auto steady_time() noexcept {
    return std::chrono::steady_clock::now();
}

inline auto is_expired(const timepoint& deadline) {
    auto now = steady_time();
    return deadline < now;
}

inline auto put_timeval(struct timeval *tv, const timepoint& deadline) {
    using namespace std::chrono;
    if (tv == nullptr) return false;
    tv->tv_sec = tv->tv_usec = 0;

    auto now = steady_clock::now();
    if (deadline < now) return false; // expired

    auto delta = deadline - now;
    auto secs = duration_cast<seconds>(delta);
    auto usecs = duration_cast<microseconds>(delta - secs);

    tv->tv_sec = static_cast<time_t>(secs.count());
#ifdef _WIN32
    tv->tv_usec = usecs.count();
#else
    tv->tv_usec = static_cast<suseconds_t>(usecs.count());
#endif
    return true;
}

inline auto get_timeout(const timepoint& deadline) -> int {
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto delta = deadline > now ? deadline - now : steady_clock::duration::zero();
    auto ms = duration_cast<milliseconds>(delta).count();
    return ms > std::numeric_limits<int>::max() ? std::numeric_limits<int>::max() : static_cast<int>(ms);
}

inline auto local_time(const std::time_t& time) noexcept {
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &time);
#else
    localtime_r(&time, &local);
#endif
    return local;
}

inline auto gmt_time(const std::time_t& time) noexcept {
    std::tm local{};
#ifdef _WIN32
    gmtime_s(&local, &time);
#else
    gmtime_r(&time, &local);
#endif
    return local;
}

inline auto make_file(const std::string& path, const std::string& mode = "r") {
    return system::file_ptr(fopen(path.c_str(), mode.c_str()));
}

inline auto make_pipe(const std::string& cmd, const std::string& mode = "r") {
    return system::pipe_ptr(popen(cmd.c_str(), mode.c_str()));
}

inline auto hostname() noexcept -> std::string {
    char buf[1024]{0};
    auto result = gethostname(buf, sizeof(buf));

    if (result != 0) return {};
    buf[sizeof(buf) - 1] = 0;
    return {buf};
}

inline auto prefix(const std::string& dir) noexcept {
    return chdir(dir.c_str()) == 0;
}
} // namespace busuto::system

namespace busuto {
using close_t = void (*)(int);
using timepoint_t = system::timepoint;
using duration_t = system::duration;

class handle_t final {
public:
    handle_t() = default;

    handle_t(handle_t&& from) noexcept : handle_(std::exchange(from.handle_, -1)), exit_(from.exit_) {}

    // cppcheck-suppress noExplicitConstructor
    handle_t(int handle) noexcept : handle_(handle) { setup(); }
    explicit handle_t(close_t fn) noexcept : exit_(fn) {}
    handle_t(int handle, close_t fn) noexcept : handle_(handle), exit_(fn) { access(); }
    ~handle_t() { close(); }

    handle_t(const handle_t&) = delete;
    auto operator=(const handle_t&) -> handle_t& = delete;

    operator int() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ > -1; }
    auto operator!() const noexcept { return handle_ < 0; }

    auto operator=(int handle) noexcept -> handle_t& {
        close();
        handle_ = handle;
        if (type_ != DEFAULT)
            setup();
        else
            access();
        return *this;
    }

    auto operator=(handle_t&& other) noexcept -> handle_t& {
        if (this == &other) return *this;
        close();
        handle_ = std::exchange(other.handle_, -1);
        exit_ = other.exit_;
        return *this;
    }

    auto readable() const noexcept { return handle_ > -1 && ((access_ == O_RDONLY) || (access_ == O_RDWR)); }
    auto writable() const noexcept { return handle_ > -1 && ((access_ == O_WRONLY) || (access_ == O_RDWR)); }
    auto is_open() const noexcept { return handle_ > -1; }
    auto get() const noexcept { return handle_; }
    auto release() noexcept { return std::exchange(handle_, -1); }
    auto clone() const noexcept { return dup(handle_); }
    auto reset() noexcept -> bool;
    void close() noexcept;

private:
    int handle_{-1};
    close_t exit_{[](int fd) { ::close(fd); }};
#ifndef _WIN32
    struct termios restore_{};
#endif
    int access_{O_RDWR};

    enum {
        TERMIO,
        SOCKET,
        GENERIC,
        DEFAULT,
    } type_{DEFAULT};

    void access() noexcept;
    void setup() noexcept;
};

inline auto make_handle(const std::string& path, int mode, int perms = 0664) {
    return handle_t(::open(path.c_str(), mode, perms));
}
} // namespace busuto
