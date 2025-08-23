// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "service.hpp"

#if __has_include(<sys/ioctl.h>)
#include <sys/ioctl.h>
#endif

#include <fcntl.h>

using namespace busuto;

std::atomic<bool> busuto::running{false};
busuto::service::logger busuto::system_logger;
busuto::service::timer busuto::system_timer;
busuto::service::pool busuto::system_pool;

#ifndef _WIN32
auto busuto::is_service() noexcept -> bool {
    return getpid() == 1 || getppid() == 1 || getuid() == 0;
}

auto busuto::background() noexcept -> bool {
    if (getpid() == 1 || getppid() == 1) return true;
    auto child = fork();
    if (child < 0) return false;
    if (child > 0) std::quick_exit(0);
#if defined(SIGTSTP) && defined(TIOCNOTTY)
    if (setpgid(0, getpid()) == -1)
        return false;

    auto fd = open("/dev/tty", O_RDWR);
    if (fd >= 0) {
        ::ioctl(fd, TIOCNOTTY, nullptr);
        ::close(fd);
    }
#else
#if defined(__linux__)
    if (setpgid(0, getpid()) == -1)
        return false;
#else
    if (setpgrp() == -1)
        return false;
#endif
    auto pid = fork();
    if (pid < 0)
        std::quick_exit(-1);
    else if (pid > 0)
        std::quick_exit(0);
#endif
    return true;
}
#endif
