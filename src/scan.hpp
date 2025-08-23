// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "strings.hpp"

#include <string_view>
#include <string>
#include <cstdint>
#include <cctype>
#include <cmath>

namespace busuto::scan {
constexpr std::string_view hex_digits = "0123456789abcdef";

constexpr auto decode_hex(char ch) noexcept {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    auto pos = hex_digits.find(ch);
    return pos < 16 ? static_cast<uint8_t>(pos) : 0xFF; // invalid sentinel
}

constexpr auto match_prefix(std::string_view& text, std::string_view prefix) noexcept {
    if (text.starts_with(prefix)) {
        text.remove_prefix(prefix.size());
        return true;
    }
    return false;
}

constexpr auto consume_prefix(std::string_view& input, std::string_view prefix) noexcept {
    if (input.starts_with(prefix)) {
        input.remove_prefix(prefix.size());
        return true;
    }
    if (!input.empty() && input.front() == '$') {
        input.remove_prefix(1);
        return true;
    }
    return false;
}

constexpr auto get_hex(std::string_view& input, unsigned digits = 8) noexcept -> uint64_t {
    std::uint64_t result = 0;
    if (digits > 8) return 0;
    while (digits-- && !input.empty()) {
        const auto nibble = decode_hex(input.front());
        if (nibble == 0xFF) return 0;
        result = (result << 4) | nibble;
        input.remove_prefix(1);
    }
    return result;
}

constexpr auto get_integer(std::string_view& input, std::uint64_t max = 2147483647U) noexcept {
    std::uint64_t result = 0;
    while (!input.empty()) {
        const char ch = input.front();
        if (!std::isdigit(static_cast<unsigned char>(ch)))
            break;

        const auto digit = static_cast<std::uint64_t>(ch - '0');
        const auto next = (result * 10) + digit;
        if (next > max)
            break;

        result = next;
        input.remove_prefix(1);
    }
    return result;
}

constexpr auto get_decimal(std::string_view& input, std::uint64_t max = 2147483647U) noexcept {
    const std::uint64_t integer = get_integer(input, max);
    double fraction = 0.0;
    double scale = 1.0;
    if (!input.empty() && input.front() == '.') {
        input.remove_prefix(1);
        while (!input.empty()) {
            const char ch = input.front();
            if (!std::isdigit(static_cast<unsigned char>(ch)))
                break;

            scale /= 10.0;
            fraction += static_cast<double>(ch - '0') * scale;
            input.remove_prefix(1);
        }
    }
    return static_cast<double>(integer) + fraction;
}

constexpr auto get_double(std::string_view& input, std::uint64_t max = 2147483647U) {
    double number = get_decimal(input, max);
    if (!input.empty() && (input.front() == 'e' || input.front() == 'E')) {
        input.remove_prefix(1);
        bool negative = false;
        if (!input.empty() && (input.front() == '+' || input.front() == '-')) {
            negative = (input.front() == '-');
            input.remove_prefix(1);
        }

        const std::uint64_t exponent = get_integer(input);
        const double scale = std::pow(10.0, static_cast<double>(exponent));
        number *= negative ? 1.0 / scale : scale;
    }
    return number;
}
} // namespace busuto::scan

namespace busuto {
template <std::unsigned_integral T = unsigned>
inline auto parse_hex(std::string_view input, T min = T{0}, T max = std::numeric_limits<T>::max()) {
    scan::consume_prefix(input, "0x");
    constexpr std::size_t max_digits = sizeof(T) * 2;
    const std::uint64_t value = scan::get_hex(input, max_digits);
    if (value == std::numeric_limits<std::uint64_t>::max() || !input.empty())
        throw overflow("Hex value too large or contains invalid digits");

    if (value < min || value > max)
        throw range("Hex value outside allowed bounds");
    return static_cast<T>(value);
}

template <std::integral T = int>
inline auto parse_integer(std::string_view input, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
    const auto raw = static_cast<std::int64_t>(scan::get_integer(input, static_cast<std::uint64_t>(max)));
    if (!input.empty())
        throw overflow("Trailing characters after integer value");

    if (raw < static_cast<std::int64_t>(min) || raw > static_cast<std::int64_t>(max))
        throw range("Parsed integer outside valid range");

    return static_cast<T>(raw);
}

template <std::unsigned_integral T = unsigned>
inline auto parse_unsigned(std::string_view input, T min = T{0}, T max = std::numeric_limits<T>::max()) {
    if (!input.empty() && input.front() == '-')
        throw invalid("Negative sign not allowed for unsigned");

    const std::uint64_t value = scan::get_integer(input, static_cast<std::uint64_t>(max));
    if (!input.empty())
        throw overflow("Trailing characters after unsigned value");

    if (value < min || value > max)
        throw range("Parsed value outside allowed range");

    return static_cast<T>(value);
}

template <std::floating_point T = double>
inline auto parse_decimal(std::string_view input) {
    bool negative = false;
    if (!input.empty() && input.front() == '-') {
        negative = true;
        input.remove_prefix(1);
    }

    const auto value = T(scan::get_decimal(input));
    if (!input.empty())
        throw invalid("Trailing characters after decimal value");

    return negative ? -value : value;
}

template <std::floating_point T = double>
inline auto parse_double(std::string_view input) {
    bool negative = false;
    if (!input.empty() && input.front() == '-') {
        negative = true;
        input.remove_prefix(1);
    }

    const auto value = T(scan::get_double(input));
    if (!input.empty())
        throw invalid("Trailing characters after decimal value");

    return negative ? -value : value;
}

inline auto parse_size(std::string_view input) -> std::size_t {
    if (!input.empty() && input.front() == '-')
        throw invalid("Negative sign not allowed for size");

    auto value = scan::get_integer(input, static_cast<std::uint64_t>(std::numeric_limits<uint64_t>::max()));
    if (!input.empty() && isdigit(input.front()))
        throw overflow("Trailing characters after size");

    if (input.empty()) return static_cast<std::size_t>(value);
    if (input.size() == 1) {
        switch (tolower(input.front())) {
        case 'b':
            return static_cast<std::size_t>(value);
        case 'k':
            return static_cast<std::size_t>(value * 1024ULL);
        case 'm':
            return static_cast<std::size_t>(value * 1024ULL * 1024ULL);
        case 'g':
            return static_cast<unsigned>(value * 1024ULL * 1024ULL * 1024ULL);
        default:
            break;
        }
    }
    throw invalid("Trailing characters after duration");
}

inline auto parse_duration(std::string_view input, bool ms = false) -> unsigned {
    if (!input.empty() && input.front() == '-')
        throw invalid("Negative sign not allowed for duration");

    auto value = scan::get_integer(input, static_cast<std::uint64_t>(std::numeric_limits<unsigned>::max()));
    if (!input.empty() && isdigit(input.front()))
        throw overflow("Trailing characters after duration");

    auto scale = 1U;
    if (ms) scale = 1000U;
    if (input.empty()) return static_cast<unsigned>(value);
    if (input == "ms" && ms) return static_cast<unsigned>(value);
    if (input.size() == 1) {
        switch (tolower(input.front())) {
        case 's':
            return static_cast<unsigned>(value * scale);
        case 'm':
            return static_cast<unsigned>(value * scale * 60U);
        case 'h':
            return static_cast<unsigned>(value * scale * 3600U);
        case 'd':
            return static_cast<unsigned>(value * scale * 86400U);
        default:
            break;
        }
    }
    throw invalid("Trailing characters after duration");
}

inline auto parse_bool(std::string_view input) -> bool {
    auto text = strings::to_lower(input);
    if (text == "true") return true;
    if (text == "yes") return true;
    if (text == "on") return true;
    if (text == "t") return true;
    if (text == "y") return true;
    if (text == "false") return false;
    if (text == "no") return false;
    if (text == "off") return false;
    if (text == "f") return false;
    if (text == "n") return false;
    throw invalid("Invalid bool value");
}
} // namespace busuto
