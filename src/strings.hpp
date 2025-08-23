// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "common.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <cstring>
#include <vector>

namespace busuto::strings {
template <typename T>
inline constexpr auto is_string_type_v = std::is_convertible_v<T, std::string_view>;

template <typename T>
inline constexpr bool is_string_vector_v =
std::is_move_constructible_v<T> &&
std::is_move_assignable_v<T> &&
std::is_destructible_v<T> &&
!std::is_pointer_v<T>;

template <typename T>
constexpr auto to_string_view(const T& s) {
    if constexpr (std::is_convertible_v<T, std::string_view>) {
        return std::string_view{s};
    } else if constexpr (std::is_pointer_v<T> && std::is_same_v<std::remove_cv_t<std::remove_pointer_t<T>>, char>) {
        return std::string_view{s, std::strlen(s)};
        //    } else if constexpr (std::is_array_v<T> && std::is_same_v<std::remove_extent_t<T>, char>) {
        //        return std::string_view{s, std::strlen(s)};
    } else {
        static_assert(sizeof(T) == 0, "Unsupported type for to_string_view");
    }
}

template <typename S>
inline auto to_upper(const S& input) {
    auto sv = to_string_view(input);
    using CharT = typename decltype(sv)::value_type;
    using Traits = std::char_traits<CharT>;
    std::basic_string<CharT, Traits> result;
    result.reserve(sv.size());
    for (CharT ch : sv) {
        auto ic = Traits::to_int_type(ch);
        result.push_back(Traits::to_char_type(std::toupper(ic)));
    }
    return result;
}

template <typename S>
inline auto to_lower(const S& input) {
    auto sv = to_string_view(input);
    using CharT = typename decltype(sv)::value_type;
    using Traits = std::char_traits<CharT>;
    std::basic_string<CharT, Traits> result;
    result.reserve(sv.size());
    for (CharT ch : sv) {
        auto ic = Traits::to_int_type(ch);
        result.push_back(Traits::to_char_type(std::tolower(ic)));
    }
    return result;
}

template <typename S, typename P>
inline auto starts_case(const S& source, const P& prefix) {
    auto sv_source = to_string_view(source);
    auto sv_prefix = to_string_view(prefix);
    using CharT = typename decltype(sv_source)::value_type;
    using Traits = std::char_traits<CharT>;
    using namespace std::views;
    if (sv_prefix.empty() || sv_source.size() < sv_prefix.size()) return false;

    auto norm = [](CharT c) {
        auto ic = Traits::to_int_type(c);
        return Traits::to_char_type(std::tolower(ic));
    };

    auto head_norm = sv_source.substr(0, sv_prefix.size()) | transform(norm);
    auto prefix_norm = sv_prefix | transform(norm);
    return std::ranges::equal(head_norm, prefix_norm);
}

template <typename S, typename P>
inline auto ends_case(const S& source, const P& suffix) {
    auto sv_source = to_string_view(source);
    auto sv_suffix = to_string_view(suffix);
    using CharT = typename decltype(sv_source)::value_type;
    using Traits = std::char_traits<CharT>;
    using namespace std::views;
    if (sv_suffix.empty() || sv_source.size() < sv_suffix.size()) return false;

    auto norm = [](CharT c) {
        auto ic = Traits::to_int_type(c);
        return Traits::to_char_type(std::tolower(ic));
    };

    auto tail_norm = sv_source.substr(sv_source.size() - sv_suffix.size()) | transform(norm);
    auto suffix_norm = sv_suffix | transform(norm);
    return std::ranges::equal(tail_norm, suffix_norm);
}

template <typename S, typename P>
inline auto starts_with(const S& source, const P& prefix) {
    const auto sv_source = to_string_view(source);
    const auto sv_prefix = to_string_view(prefix);
    return sv_source.find(sv_prefix) == 0;
}

template <typename S, typename P>
inline auto ends_with(const S& source, const P& suffix) {
    const auto sv_source = to_string_view(source);
    const auto sv_suffix = to_string_view(suffix);
    if (sv_source.size() < sv_suffix.size()) return false;
    auto pos = sv_source.rfind(sv_suffix);
    if (pos > sv_source.size()) return false;
    return sv_source.substr(pos) == sv_suffix;
}

template <typename S>
inline auto contains(const S& from, const S& find) {
    const auto sv_from = to_string_view(from);
    const auto sv_find = to_string_view(find);
    return sv_from.find(sv_find) <= sv_from.size();
}

template <typename S = std::string>
constexpr auto trim(const S& from, std::string_view sep = " \t\f\v\n\r") {
    auto str = to_string_view(from);
    auto last = str.find_last_not_of(sep);
    if (last > str.size()) return str.substr(0, 0);
    return str.substr(0, ++last);
}

template <typename S = std::string>
constexpr auto strip(const S& from, std::string_view sep = " \t\f\v\n\r") {
    auto str = to_string_view(from);
    const std::size_t first = str.find_first_not_of(sep);
    const std::size_t last = str.find_last_not_of(sep);
    if (last > str.size()) return str.substr(0, 0);
    return str.substr(first, (last - first + 1));
}

template <typename S = std::string>
constexpr auto unquote(const S& from, std::string_view pairs = R"(""''{})") {
    auto str = to_string_view(from);
    if (str.empty()) return str;
    auto pos = pairs.find_first_of(str[0]);
    if (pos > str.size() || (pos & 0x01)) return str;
    auto len = str.size();
    if (--len < 1) return str;
    if (str[len] == pairs[++pos]) return str.substr(1, --len);
    return str;
}

template <typename S = std::string>
inline auto split(const S& from, std::string_view delim = " ", unsigned max = 0) {
    static_assert(is_string_vector_v<S>, "S must be a string vector type");
    auto str = to_string_view(from);
    std::vector<S> result;
    std::size_t current{}, prev{};
    unsigned count = 0;
    current = str.find_first_of(delim);
    while ((!max || ++count < max) && current != std::string_view::npos) {
        result.emplace_back(str.substr(prev, current - prev));
        prev = current + 1;
        current = str.find_first_of(delim, prev);
    }
    result.emplace_back(str.substr(prev));
    return result;
}

template <typename S = std::string>
constexpr auto join(const std::vector<S>& list, const std::string_view& delim = ",") -> S {
    static_assert(is_string_type_v<S>, "S must be a string type");
    S separator{}, result;
    for (const auto& str : list) {
        result = result + separator + str;
        separator = delim;
    }
    return result;
}

template <typename S = std::string>
inline auto tokenize(const S& from, std::string_view delim = " ", std::string_view quotes = R"(""''{})") {
    static_assert(is_string_vector_v<S>, "S must be a string vector type");
    auto str = to_string_view(from);
    std::vector<S> result;
    auto current = str.find_first_of(delim);
    auto prev = str.find_first_not_of(delim);
    if (prev == std::string_view::npos)
        prev = 0;

    while (current != std::string_view::npos) {
        auto lead = quotes.find_first_of(str[prev]);
        if (lead != std::string_view::npos) {
            auto tail = str.find_first_of(quotes[lead + 1], prev + 1);
            if (tail != std::string_view::npos) {
                current = tail;
                result.emplace_back(str.substr(prev, current - prev + 1));
                goto finish; // NOLINT
            }
        }
        result.emplace_back(str.substr(prev, current - prev));
    finish:
        prev = str.find_first_not_of(delim, ++current);
        if (prev == std::string_view::npos)
            prev = current;
        current = str.find_first_of(delim, prev);
    }
    if (prev < str.size())
        result.emplace_back(str.substr(prev));
    return result;
}

constexpr auto is_line(const std::string_view str) {
    if (str.empty()) return false;
    if (str[str.size() - 1] == '\n') return true;
    return false;
}

constexpr auto is_quoted(const std::string_view str, std::string_view pairs = R"(""''{})") {
    if (str.size() < 2) return false;
    auto pos = pairs.find_first_of(str[0]);
    if (pos == std::string_view::npos || (pos & 0x01)) return false;
    auto len = str.size();
    return str[--len] == pairs[++pos];
}

constexpr auto is_unsigned(const std::string_view str) {
    auto len = str.size();
    if (!len) return false;
    auto pos = std::size_t(0);
    while (pos < len) {
        if (str[pos] < '0' || str[pos] > '9') return false;
        ++pos;
    }
    return true;
}

constexpr auto is_integer(const std::string_view str) {
    if (str.empty()) return false;
    if (str[0] == '-') return is_unsigned(str.substr(1, str.size() - 1));
    return is_unsigned(str);
}
} // namespace busuto::strings
