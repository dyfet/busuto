// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "chario.hpp"
#include "streams.hpp"

namespace busuto::device {
class serial_t final {
public:
    serial_t() = delete;

    explicit serial_t(int tty) : handle_(tty) {
        if (handle_ > -1 && isatty(handle_)) {
            tcgetattr(handle_, &original_);
            reset();
        } else {
            handle_ = -1;
        }
    }

    ~serial_t() { // this form of release only when object is destroyed
        if (handle_.is_open())
            tcsetattr(handle_, TCSANOW, &original_);
    }

    operator int() const noexcept { return handle_.get(); }
    explicit operator bool() const noexcept { return handle_.is_open(); }
    auto operator!() const noexcept { return !handle_.is_open(); }
    auto get() const noexcept { return handle_.get(); }
    auto fetch() noexcept -> struct termios * { return &current_; }
    auto release() noexcept { return std::exchange(handle_, -1); }

    void update(const struct termios *term = nullptr) noexcept {
        if (term)
            memcpy(&current_, term, sizeof(current_));
        tcsetattr(handle_, TCSANOW, &current_);
    }

    void reset() {
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

    template <std::size_t S = 80>
    auto input() { // stream destruction does not close
        return make_input(handle_, []([[maybe_unused]] int fd) {});
    }

    template <std::size_t S = 256>
    auto output() { // stream destruction does not close
        return make_output(handle_, []([[maybe_unused]] int fd) {});
    }

private:
    handle_t handle_{-1};
    struct termios original_{}, current_{};
};

template <busuto::util::readable_binary Binary>
inline auto csum8(const Binary& from) {
    auto size = from.size();
    auto data = reinterpret_cast<const uint8_t *>(from.data());
    uint8_t sum = 0;
    while (size--)
        sum += *data++;
    return sum;
}

template <busuto::util::readable_binary Binary>
inline auto crc16(const Binary& from) {
    auto size = from.size();
    auto data = reinterpret_cast<const uint8_t *>(from.data());
    uint16_t crc = 0x0000;
    while (size--) {
        crc ^= *data++ << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x8005;
            else
                crc <<= 1;
        }
    }
    return crc;
}

template <busuto::util::readable_binary Binary>
inline auto crc32(const Binary& from) {
    auto size = from.size();
    auto data = reinterpret_cast<const uint8_t *>(from.data());
    uint32_t table[256]{0};
    uint32_t crc = 0xffffffff;
    for (std::size_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (std::size_t j = 0; j < 8; j++)
            c = (c & 1) ? 0xedb88320 ^ (c >> 1) : c >> 1;
        table[i] = c;
    }

    for (std::size_t i = 0; i < size; i++)
        crc = table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
    return crc ^ 0xffffffff;
}

inline auto make_serial(const std::string& path, int mode = O_RDWR, int perms = 0664) {
    return serial_t(::open(path.c_str(), mode, perms));
}
} // namespace busuto::device
