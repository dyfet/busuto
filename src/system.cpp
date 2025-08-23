// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "system.hpp"

#ifndef _WIN32
#include <sys/socket.h>
#endif

#include <sys/stat.h>

using namespace busuto;

void busuto::handle_t::access() noexcept {
    if (handle_ < 0) {
        access_ = O_RDWR;
    } else if (handle_ == 0) {
        access_ = O_RDONLY;
    } else if (handle_ < 3) {
        access_ = O_WRONLY;
    } else {
#ifndef _WIN32
        auto mode = fcntl(handle_, F_GETFL);
        if (mode > -1)
            access_ = mode & O_ACCMODE;
        else
#endif
            access_ = O_RDWR;
    }
}

auto busuto::handle_t::reset() noexcept -> bool {
    if (type_ != TERMIO || handle_ < 0) return false;
#ifndef _WIN32
    struct termios t{};
    memcpy(&t, &restore_, sizeof(t));
    t.c_lflag &= ~(ICANON | ECHO | ISIG);
    t.c_iflag &= ~(IXON | ICRNL);
    t.c_oflag &= ~(OPOST);
    t.c_cc[VMIN] = 1;  // read returns after 1 byte
    t.c_cc[VTIME] = 0; // no timeout
    tcsetattr(handle_, TCSANOW, &t);
#endif
    return true;
}

void busuto::handle_t::setup() noexcept {
    if (handle_ < 0) return;
    access();
    auto tty = isatty(handle_);
    if (tty && (handle_ == 0 || handle_ > 2) && readable()) {
        type_ = TERMIO;
#ifndef _WIN32
        tcgetattr(handle_, &restore_);
#endif
        reset();
    } else if (!tty && handle_ > 2) {
        struct stat ino{};
#ifdef S_ISSOCK
        if (!fstat(handle_, &ino) && S_ISSOCK(ino.st_mode))
            type_ = SOCKET;
#endif
    } else
        type_ = GENERIC;
}

void busuto::handle_t::close() noexcept {
    if (handle_ > -1) {
        auto handle = std::exchange(handle_, -1); // prevents race
        switch (type_) {
        case SOCKET:
#ifdef  _WIN32
            shutdown(handle_, SD_BOTH);
            closesocket(handle_);
            access_ = O_RDWR;
            return;
#else
            if (access_ == O_RDWR)
                shutdown(handle, SHUT_RDWR);
            else if (access_ == O_RDONLY)
                shutdown(handle, SHUT_RD);
            else if (access_ == O_WRONLY)
                shutdown(handle, SHUT_WR);
            break;
#endif
        case TERMIO:
#ifndef _WIN32
            tcsetattr(handle, TCSANOW, &restore_);
#endif
            break;
        default:
            break;
        }
        access_ = O_RDWR;
        exit_(handle);
    }
}
