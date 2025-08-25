// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "common.hpp"

#include <array>
#include <algorithm>
#include <limits>
#include <span>
#include <ranges>
#include <vector>
#include <ostream>
#include <cstring>
#include <cstddef>
#include <string_view>
#include <bit>

namespace busuto::util {
template <typename T>
concept PointerTo = std::is_pointer_v<T>;

template <typename T>
concept ReferenceTo = std::is_reference_v<T>;

template <typename T>
concept writable_binary = requires(T obj) {
    { obj.data() } -> std::convertible_to<void *>;
    { obj.size() } -> std::convertible_to<std::size_t>;
};

template <typename T>
concept readable_binary = requires(const T obj) {
    { obj.data() } -> std::convertible_to<const void *>;
    { obj.size() } -> std::convertible_to<std::size_t>;
};

// Himitsu C++17 style code compatible traits...
template <typename, typename = void>
struct is_writable_binary : std::false_type {};

template <typename T>
struct is_writable_binary<T, std::void_t<
                             decltype(std::declval<T>().data()),
                             decltype(std::declval<T>().size())>> : std::integral_constant<bool,
                                                                    std::is_convertible_v<decltype(std::declval<T>().data()), void *> &&
                                                                    std::is_convertible_v<decltype(std::declval<T>().size()), std::size_t>> {};

template <typename T>
constexpr bool is_writable_binary_v = is_writable_binary<T>::value;

template <typename, typename = void>
struct is_readable_binary : std::false_type {};

template <typename T>
struct is_readable_binary<T, std::void_t<
                             decltype(std::declval<const T>().data()),
                             decltype(std::declval<const T>().size())>> : std::integral_constant<bool,
                                                                          std::is_convertible_v<decltype(std::declval<const T>().data()), const void *> &&
                                                                          std::is_convertible_v<decltype(std::declval<const T>().size()), std::size_t>> {};

template <typename T>
constexpr bool is_readable_binary_v = is_readable_binary<T>::value;

auto is_utf8(const std::byte *data, std::size_t len) -> bool;
auto is_utf8(const std::span<const std::byte>& data) -> bool;
auto is_utf8(const std::string_view& view) -> bool;
auto encode_hex(std::span<const std::byte> input) -> std::string;
auto encode_b64(std::span<const std::byte> input) -> std::string;
auto decode_b64(const std::string& in) -> std::vector<std::byte>;
auto decode_hex(const std::string& in) -> std::vector<std::byte>;

constexpr auto big_endian() {
    return std::endian::native == std::endian::big;
}

constexpr auto little_endian() {
    return std::endian::native == std::endian::little;
}

inline auto swap16(uint16_t value) -> uint16_t {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(value);
#else
    return ((value & 0x00FF) << 8) | ((value & 0xFF00) >> 8);
#endif
}

inline auto swap32(uint32_t value) -> uint32_t {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(value);
#else
    return ((value & 0x000000FF) << 24) |
           ((value & 0x0000FF00) << 8) |
           ((value & 0x00FF0000) >> 8) |
           ((value & 0xFF000000) >> 24);
#endif
}

inline auto swap64(uint64_t value) -> uint64_t {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(value);
#else
    return ((value & 0x00000000000000FFULL) << 56) |
           ((value & 0x000000000000FF00ULL) << 40) |
           ((value & 0x0000000000FF0000ULL) << 24) |
           ((value & 0x00000000FF000000ULL) << 8) |
           ((value & 0x000000FF00000000ULL) >> 8) |
           ((value & 0x0000FF0000000000ULL) >> 24) |
           ((value & 0x00FF000000000000ULL) >> 40) |
           ((value & 0xFF00000000000000ULL) >> 56);
#endif
}

template <readable_binary Binary>
inline auto binary_ptr(Binary obj) {
    return reinterpret_cast<std::byte *>(obj.data());
}

template <typename T, std::size_t N>
requires PointerTo<T>
inline auto offset_at(T where, const std::array<std::remove_pointer_t<T>, N>& array) {
    return where - array.data();
}

template <typename T, std::size_t N>
requires ReferenceTo<T>
inline auto offset_at(T where, const std::array<std::remove_reference_t<T>, N>& array) {
    return &where - array.data();
}
} // namespace busuto::util

namespace busuto {
using byte_span = std::span<const std::byte>;

class byte_array {
public:
    using reference = std::byte&;
    using pointer = std::byte *;
    using const_reference = const std::byte&;
    using size_type = std::size_t;
    using value_type = std::byte;
    using iterator = typename std::vector<std::byte>::iterator;
    using const_iterator = typename std::vector<std::byte>::const_iterator;
    using reverse_iterator = typename std::vector<std::byte>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<std::byte>::const_reverse_iterator;

    constexpr byte_array() = default;

    constexpr byte_array(const void *data, std::size_t size) : buffer_{static_cast<const std::byte *>(data), static_cast<const std::byte *>(data) + size} {}

    constexpr explicit byte_array(std::size_t size) : buffer_(size) {}

    constexpr explicit byte_array(std::span<const std::byte> view) : buffer_{view.begin(), view.end()} {}

    explicit byte_array(const std::u8string& s) : buffer_(reinterpret_cast<const std::byte *>(s.data()), reinterpret_cast<const std::byte *>(s.data()) + s.size()) {}

    byte_array(const char *cstr, std::size_t size) : buffer_{reinterpret_cast<const std::byte *>(cstr), reinterpret_cast<const std::byte *>(cstr) + size} {}

    constexpr byte_array(const byte_array&) = default;
    constexpr auto operator=(const byte_array&) -> byte_array& = default;
    constexpr byte_array(byte_array&&) noexcept = default;
    constexpr auto operator=(byte_array&&) noexcept -> byte_array& = default;

    operator std::string() const {
        return to_hex();
    }

    explicit operator bool() const noexcept { return !empty(); }
    auto operator!() const noexcept { return empty(); }

    constexpr auto operator==(const byte_array& other) const noexcept -> bool {
        return buffer_ == other.buffer_;
    }

    constexpr auto operator!=(const byte_array& other) const noexcept -> bool {
        return !(*this == other);
    }

    auto operator+=(const byte_array& other) noexcept -> byte_array& {
        append(other);
        return *this;
    }

    auto operator+(const byte_array& other) const noexcept {
        auto result = *this;
        result.append(other);
        return result;
    }

    auto u8data() const noexcept -> const uint8_t * {
        return reinterpret_cast<const uint8_t *>(data());
    }

    auto u8data() noexcept -> uint8_t * {
        return reinterpret_cast<uint8_t *>(data());
    }

    constexpr auto data() const noexcept -> const std::byte * { return buffer_.data(); }
    constexpr auto data() noexcept -> std::byte * { return buffer_.data(); }
    constexpr auto size() const noexcept -> std::size_t { return buffer_.size(); }
    constexpr auto capacity() const noexcept -> std::size_t { return buffer_.capacity(); }

    constexpr auto span() const noexcept -> std::span<const std::byte> {
        return std::span<const std::byte>{buffer_.data(), buffer_.size()};
    }

    auto span_mut() noexcept -> std::span<std::byte> {
        return std::span<std::byte>{buffer_.data(), buffer_.size()};
    }

    auto view() const noexcept -> std::string_view {
        return std::string_view{reinterpret_cast<const char *>(buffer_.data()), buffer_.size()};
    }

    constexpr void swap(byte_array& other) noexcept {
        buffer_.swap(other.buffer_);
    }

    void remove_prefix(std::size_t n) {
        if (n >= buffer_.size()) {
            buffer_.clear();
        } else {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<std::vector<std::byte>::difference_type>(n));
        }
    }

    void remove_suffix(std::size_t n) {
        if (n >= buffer_.size()) {
            buffer_.clear();
        } else {
            buffer_.resize(buffer_.size() - n);
        }
    }

    void append(const void *src, std::size_t n) {
        const auto *p = static_cast<const std::byte *>(src);
        buffer_.insert(buffer_.end(), p, p + n);
    }

    void append(const byte_array& other) {
        if (!other.empty())
            buffer_.insert(buffer_.end(), other.data(), other.data() + other.size());
    }

    constexpr auto begin() noexcept -> std::vector<std::byte>::iterator { return buffer_.begin(); }
    constexpr auto end() noexcept -> std::vector<std::byte>::iterator { return buffer_.end(); }

    constexpr auto begin() const noexcept -> std::vector<std::byte>::const_iterator { return buffer_.begin(); }
    constexpr auto end() const noexcept -> std::vector<std::byte>::const_iterator { return buffer_.end(); }

    constexpr auto empty() const noexcept -> bool { return buffer_.empty(); }

    auto slice(std::size_t start = 0, std::size_t end = std::numeric_limits<std::size_t>::max()) const -> byte_array {
        const auto actual_end = std::min(end, size());
        if (start > actual_end)
            throw range{"Invalid slice range"};
        return byte_array{data() + start, actual_end - start};
    }

    auto subspan(std::size_t offset, std::size_t count = std::numeric_limits<std::size_t>::max()) const -> std::span<const std::byte> {
        const auto actual_end = std::min(offset + count, size());
        if (offset > actual_end)
            throw range{"Invalid subspan range"};
        return std::span<const std::byte>{data() + offset, actual_end - offset};
    }

    auto subview(std::size_t offset, std::size_t count = std::numeric_limits<std::size_t>::max()) const -> std::string_view {
        const auto actual_end = std::min(offset + count, size());
        if (offset > actual_end)
            throw range{"Invalid subspan range"};
        return std::string_view{reinterpret_cast<const char *>(data()) + offset, actual_end - offset};
    }

    void clear() noexcept { buffer_.clear(); }
    void resize(std::size_t n) { buffer_.resize(n); }
    void reserve(std::size_t n) { buffer_.reserve(n); }
    void shrink_to_fit() { buffer_.shrink_to_fit(); }
    void push_back(std::byte b) { buffer_.push_back(b); }
    void pop_back() { buffer_.pop_back(); }

    auto operator[](std::size_t i) -> std::byte& { return buffer_[i]; }
    constexpr auto operator[](std::size_t i) const -> const std::byte& { return buffer_[i]; }
    auto front() -> std::byte& { return buffer_.front(); }
    auto back() -> std::byte& { return buffer_.back(); }
    constexpr auto front() const -> const std::byte& { return buffer_.front(); }
    constexpr auto back() const -> const std::byte& { return buffer_.back(); }

    auto c_str() const -> const char * {
        return reinterpret_cast<const char *>(buffer_.data());
    }

    void fill(std::byte value) {
        std::ranges::fill(buffer_, value);
    }

    void replace(std::byte from, std::byte to) {
        std::ranges::replace(buffer_, from, to);
    }

    void reverse() {
        std::ranges::reverse(buffer_);
    }

    auto to_u8vector() const -> std::vector<uint8_t> {
        std::vector<uint8_t> out(buffer_.size());
        std::ranges::transform(buffer_, out.begin(), [](std::byte b) { return std::to_integer<uint8_t>(b); });
        return out;
    }

    auto to_string() const -> std::string {
        return to_hex();
    }

    auto to_u8string() const -> std::u8string {
        const auto *p = reinterpret_cast<const char8_t *>(buffer_.data());
        return std::u8string{p, buffer_.size()};
    }

    auto to_hex() const -> std::string {
        constexpr char hex[] = "0123456789ABCDEF";
        std::string out;
        out.reserve(size() * 2);
        for (const auto& b : buffer_) {
            auto val = std::to_integer<unsigned char>(b);
            out.push_back(hex[val >> 4]);
            out.push_back(hex[val & 0x0F]);
        }
        return out;
    }

private:
    std::vector<std::byte> buffer_;

    friend void swap(byte_array& a, byte_array& b) noexcept {
        a.swap(b);
    }
};

inline auto operator<<(std::ostream& out, const byte_array& bytes) -> std::ostream& {
    if (is(bytes))
        out << bytes.to_hex();
    else
        out << "nil";
    return out;
}

template <util::readable_binary Binary>
inline auto to_byte_span(const Binary& obj) {
    return byte_span(reinterpret_cast<const std::byte *>(obj.data()), obj.size());
}

inline auto to_string(const byte_array& ba) {
    return ba.to_hex();
}

template <util::readable_binary Binary>
inline auto to_b64(const Binary& bin) {
    return util::encode_b64(to_byte_span(bin));
}

template <util::readable_binary Binary>
inline auto to_hex(const Binary& bin) {
    return util::encode_hex(to_byte_span(bin));
}

inline auto from_hex(const std::string& in) {
    return byte_array(util::decode_hex(in));
}

inline auto from_b64(const std::string& in) {
    return byte_array(util::decode_b64(in));
}

template <typename T>
requires(
std::is_trivially_constructible_v<T> &&
std::is_trivially_copyable_v<T> &&
!requires(T t) { t.data(); })
inline auto make_binary(const T& obj) {
    return byte_span(reinterpret_cast<std::byte *>(&obj), sizeof(obj));
}
} // namespace busuto

namespace std {
template <>
struct hash<busuto::byte_array> {
    auto operator()(const busuto::byte_array& b) const noexcept {
        if (!is(b))
            return std::size_t(0);
        const auto *bytes = b.data();
        return std::hash<std::string_view>{}(std::string_view(
        reinterpret_cast<const char *>(bytes),
        b.size()));
    }
};
} // namespace std
