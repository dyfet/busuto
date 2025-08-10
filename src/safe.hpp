// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "common.hpp"

#include <cstring>
#include <cstdlib>
#include <cstdarg>

namespace busuto::safe {
constexpr auto eq(const char *p1, const char *p2) {
    if (!p1 && !p2) return true;
    if (!p1 || !p2) return false;
    return strcmp(p1, p2) == 0;
}

constexpr auto eq(const char *p1, const char *p2, std::size_t len) {
    if (!p1 && !p2) return true;
    if (!p1 || !p2) return false;
    return strncmp(p1, p2, len) == 0;
}

constexpr auto size(const char *cp, std::size_t max = 256) -> std::size_t {
    std::size_t count = 0;
    while (cp && *cp && count < max) {
        ++count;
        ++cp;
    }
    return count;
}

auto memset(void *ptr, int value, size_t size) noexcept -> void *;
auto copy(char *cp, std::size_t max, std::string_view view) noexcept -> std::size_t;
auto append(char *cp, std::size_t max, ...) noexcept -> bool;
} // namespace busuto::safe
