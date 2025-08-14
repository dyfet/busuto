// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "safe.hpp"

using namespace busuto;

auto safe::copy(char *cp, std::size_t max, std::string_view view) noexcept -> std::size_t {
    if (!cp) return std::size_t(0);
    auto count = view.size();
    auto dp = view.data();
    if (count >= max)
        count = max - 1;

    max = count;
    while (max--)
        *(cp++) = *(dp++);

    *cp = 0;
    return count;
}

auto safe::append(char *cp, std::size_t max, ...) noexcept -> bool { // NOLINT
    va_list list{};
    auto pos = safe::size(cp, max);
    va_start(list, max);
    for (;;) {
        auto sp = va_arg(list, const char *); // NOLINT
        if (!sp) break;
        if (pos >= max) continue;
        auto chars = safe::size(sp);
        if (chars + pos >= max) {
            pos = max;
            continue;
        }

        safe::copy(cp + pos, max - pos, sp);
        pos += chars;
    }
    va_end(list);
    return pos < max;
}

auto safe::memset(void *ptr, int value, std::size_t size) noexcept -> void * {
    if (!ptr) return nullptr;
    auto p = reinterpret_cast<volatile uint8_t *volatile>(ptr);
    for (size_t i = 0; i < size; ++i) {
        p[i] = static_cast<uint8_t>(value);
    }
    return ptr;
}
