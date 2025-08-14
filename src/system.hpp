// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "common.hpp"

#include <chrono>
#include <string>
#include <memory>
#include <cstdlib>
#include <csignal>
#include <cstdio>
#include <ctime>

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>

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
using file_ptr = std::unique_ptr<FILE, util::file_closer>;
using pipe_ptr = std::unique_ptr<FILE, util::pipe_closer>;

inline auto page_size() noexcept -> off_t {
    return sysconf(_SC_PAGESIZE);
}

inline void time_of_day(struct timeval *tp) noexcept {
    gettimeofday(tp, nullptr);
}

inline auto steady_time() noexcept {
    return std::chrono::steady_clock::now();
}

inline auto local_time(const std::time_t& time) noexcept {
    std::tm local{};
    localtime_r(&time, &local);
    return local;
}

inline auto gmt_time(const std::time_t& time) noexcept {
    std::tm local{};
    gmtime_r(&time, &local);
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

class handle_t final {
public:
    handle_t() = default;

    handle_t(handle_t&& from) noexcept : handle_(std::exchange(from.handle_, -1)), exit_(from.exit_) {}

    // cppcheck-suppress noExplicitConstructor
    handle_t(int handle) noexcept : handle_(handle) { setup(); }
    explicit handle_t(close_t fn) noexcept : exit_(fn) {}
    handle_t(int handle, close_t fn) noexcept : handle_(handle), exit_(fn) { access(); }
    ~handle_t() { cancel(); }

    handle_t(const handle_t&) = delete;
    auto operator=(const handle_t&) -> handle_t& = delete;

    operator int() const noexcept { return handle_; }
    explicit operator bool() const noexcept { return handle_ > -1; }
    auto operator!() const noexcept { return handle_ < 0; }

    auto operator=(int handle) noexcept -> handle_t& {
        cancel();
        handle_ = handle;
        if (type_ != DEFAULT)
            setup();
        else
            access();
        return *this;
    }

    auto operator=(handle_t&& other) noexcept -> handle_t& {
        if (this == &other) return *this;
        cancel();
        handle_ = std::exchange(other.handle_, -1);
        exit_ = other.exit_;
        return *this;
    }

    auto is_readable() const noexcept { return handle_ > -1 && ((access_ == O_RDONLY) || (access_ == O_RDWR)); }
    auto is_writable() const noexcept { return handle_ > -1 && ((access_ == O_WRONLY) || (access_ == O_RDWR)); }
    // auto is_rdwr() const noexcept { return access_ == O_RDWR; }
    auto is_open() const noexcept { return handle_ > -1; }
    auto get() const noexcept { return handle_; }
    auto release() noexcept { return std::exchange(handle_, -1); }
    auto clone() const noexcept { return dup(handle_); }
    auto reset() noexcept -> bool;
    void cancel() noexcept;

private:
    int handle_{-1};
    close_t exit_{[](int fd) { ::close(fd); }};
    struct termios restore_{};
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
