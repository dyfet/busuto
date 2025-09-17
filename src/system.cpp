// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "system.hpp"

#include <sys/socket.h>
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
        auto mode = fcntl(handle_, F_GETFL);
        if (mode > -1)
            access_ = mode & O_ACCMODE;
        else
            access_ = O_RDWR;
    }
}

auto busuto::handle_t::reset() noexcept -> bool {
    if (type_ != TERMIO || handle_ < 0) return false;
    struct termios t{};
    memcpy(&t, &restore_, sizeof(t));
    t.c_lflag &= ~(ICANON | ECHO | ISIG);
    t.c_iflag &= ~(IXON | ICRNL);
    t.c_oflag &= ~(OPOST);
    t.c_cc[VMIN] = 1;  // read returns after 1 byte
    t.c_cc[VTIME] = 0; // no timeout
    tcsetattr(handle_, TCSANOW, &t);
    return true;
}

void busuto::handle_t::setup() noexcept {
    if (handle_ < 0) return;
    access();
    auto tty = isatty(handle_);
    if (tty && (handle_ == 0 || handle_ > 2) && readable()) {
        type_ = TERMIO;
        tcgetattr(handle_, &restore_);
        reset();
#ifdef S_ISSOCK
    } else if (!tty && handle_ > 2) {
        struct stat ino{};
        if (!fstat(handle_, &ino)) {
            // cppcheck-suppress syntaxError
            if (S_ISSOCK(ino.st_mode))
                type_ = SOCKET;
        }
#endif
    } else
        type_ = GENERIC;
}

void busuto::handle_t::close() noexcept {
    if (handle_ > -1) {
        auto handle = std::exchange(handle_, -1); // prevents race
        switch (type_) {
        case SOCKET:
            if (access_ == O_RDWR)
                shutdown(handle, SHUT_RDWR);
            else if (access_ == O_RDONLY)
                shutdown(handle, SHUT_RD);
            else if (access_ == O_WRONLY)
                shutdown(handle, SHUT_WR);
            break;
        case TERMIO:
            tcsetattr(handle, TCSANOW, &restore_);
            break;
        default:
            break;
        }
        access_ = O_RDWR;
        exit_(handle);
    }
}
