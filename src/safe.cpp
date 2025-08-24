// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "safe.hpp"

#include <cctype>

using namespace busuto;

auto safe::strcopy(char *cp, std::size_t max, const char *dp) noexcept -> std::size_t {
    if (!cp || !dp) return std::size_t(0);
    auto count = safe::strsize(dp, max);
    if (count >= max)
        count = max - 1;

    max = count;
    while (max--)
        *(cp++) = *(dp++);

    *cp = 0;
    return count;
}

auto safe::strcat(char *cp, std::size_t max, ...) noexcept -> std::size_t { // uNOLINT
    va_list list{};
    auto pos = safe::strsize(cp, max);
    std::size_t count{0};
    va_start(list, max);
    for (;;) {
        auto sp = va_arg(list, const char *); // NOLINT
        if (!sp) break;
        if (pos >= max) continue;
        auto chars = safe::strsize(sp);
        if (chars + pos >= max) {
            pos = max;
            continue;
        }

        count += safe::strcopy(cp + pos, max - pos, sp);
        pos += chars;
    }
    va_end(list);
    return count;
}

auto safe::memset(void *ptr, int value, std::size_t size) noexcept -> void * {
    if (!ptr) return nullptr;
    auto p = reinterpret_cast<volatile uint8_t *volatile>(ptr);
    for (size_t i = 0; i < size; ++i) {
        p[i] = static_cast<uint8_t>(value);
    }
    return ptr;
}

void safe::strupper(char *cp, std::size_t max) {
    auto count = safe::strsize(cp, max);
    while (count--) {
        *cp = static_cast<char>(toupper(*cp));
        ++cp;
    }
}

void safe::strlower(char *cp, std::size_t max) {
    auto count = safe::strsize(cp, max);
    while (count--) {
        *cp = static_cast<char>(tolower(*cp));
        ++cp;
    }
}
