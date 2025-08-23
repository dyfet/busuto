// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "binary.hpp"

using namespace busuto;

namespace {
constexpr std::array<uint8_t, 256> b64_lookup = [] {
    std::array<uint8_t, 256> table{};
    table.fill(0xFF); // invalid by default

    const char *alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
    for (uint8_t i = 0; i < 64; ++i)
        table[static_cast<unsigned char>(alphabet[i])] = i;

    return table;
}();

constexpr std::array<uint8_t, 256> hex_lookup = [] {
    std::array<uint8_t, 256> table{};
    table.fill(0xFF);

    for (char c = '0'; c <= '9'; ++c)
        table[static_cast<unsigned char>(c)] = c - '0';
    for (char c = 'A'; c <= 'F'; ++c)
        table[static_cast<unsigned char>(c)] = c - 'A' + 10;
    for (char c = 'a'; c <= 'f'; ++c)
        table[static_cast<unsigned char>(c)] = c - 'a' + 10;

    return table;
}();
} // end namespace

auto util::is_utf8(const std::byte *data, std::size_t len) -> bool {
    const auto *bytes = reinterpret_cast<const uint8_t *>(data);
    std::size_t i = 0;

    while (i < len) {
        const uint8_t byte = bytes[i];
        const std::size_t rem = len - i;
        if (byte <= 0x7F) {
            ++i; // ASCII
        } else if ((byte >> 5) == 0x6 && rem >= 2 &&
                   (bytes[i + 1] >> 6) == 0x2) {
            i += 2; // 2-byte sequence
        } else if ((byte >> 4) == 0xE && rem >= 3 &&
                   (bytes[i + 1] >> 6) == 0x2 &&
                   (bytes[i + 2] >> 6) == 0x2) {
            i += 3; // 3-byte sequence
        } else if ((byte >> 3) == 0x1E && rem >= 4 &&
                   (bytes[i + 1] >> 6) == 0x2 &&
                   (bytes[i + 2] >> 6) == 0x2 &&
                   (bytes[i + 3] >> 6) == 0x2) {
            i += 4; // 4-byte sequence
        } else {
            return false;
        }
    }
    return true;
}

auto util::is_utf8(const std::span<const std::byte>& data) -> bool {
    return is_utf8(data.data(), data.size());
}

auto util::is_utf8(const std::string_view& view) -> bool {
    return is_utf8(reinterpret_cast<const std::byte *>(view.data()), view.size());
}

auto util::encode_b64(std::span<const std::byte> input) -> std::string {
    static constexpr char alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string out;
    const auto *data = reinterpret_cast<const uint8_t *>(input.data());
    const std::size_t len = input.size();
    out.reserve(((len + 2) / 3) * 4);

    std::size_t i = 0;
    while (i + 2 < len) {
        const uint32_t triple = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
        out.push_back(alphabet[(triple >> 18) & 0x3F]);
        out.push_back(alphabet[(triple >> 12) & 0x3F]);
        out.push_back(alphabet[(triple >> 6) & 0x3F]);
        out.push_back(alphabet[triple & 0x3F]);
        i += 3;
    }

    if (i < len) {
        uint32_t triple = data[i] << 16;
        if (i + 1 < len) triple |= data[i + 1] << 8;

        out.push_back(alphabet[(triple >> 18) & 0x3F]);
        out.push_back(alphabet[(triple >> 12) & 0x3F]);
        out.push_back(i + 1 < len ? alphabet[(triple >> 6) & 0x3F] : '=');
        out.push_back('=');
    }
    return out;
}

auto util::encode_hex(std::span<const std::byte> input) -> std::string {
    constexpr char hex[] = "0123456789ABCDEF";
    std::string out;
    out.reserve(input.size() * 2);
    for (const auto& b : input) {
        auto val = std::to_integer<uint8_t>(b);
        out.push_back(hex[val >> 4]);
        out.push_back(hex[val & 0x0F]);
    }
    return out;
}

auto util::decode_b64(const std::string& in) -> std::vector<std::byte> {
    std::vector<std::byte> out;
    const std::size_t len = in.size();
    if (len % 4 != 0) throw invalid{"Invalid base64 length"};

    for (std::size_t i = 0; i < len; i += 4) {
        uint32_t val = 0;
        int pad = 0;
        for (int j = 0; j < 4; ++j) {
            const char c = in[i + j];
            if (c == '=') {
                val <<= 6;
                ++pad;
            } else {
                const uint8_t v = b64_lookup[static_cast<unsigned char>(c)];
                if (v == 0xFF) throw invalid{"Invalid base64 character"};
                val = (val << 6) | v;
            }
        }
        if (pad < 3) out.push_back(std::byte{static_cast<uint8_t>(val >> 16)});
        if (pad < 2) out.push_back(std::byte{static_cast<uint8_t>((val >> 8) & 0xFF)});
        if (pad < 1) out.push_back(std::byte{static_cast<uint8_t>(val & 0xFF)});
    }

    return out;
}

auto util::decode_hex(const std::string& in) -> std::vector<std::byte> {
    if (in.size() % 2 != 0)
        throw invalid{"Hex string must have even length"};

    std::vector<std::byte> out;
    out.reserve(in.size() / 2);

    for (std::size_t i = 0; i < in.size(); i += 2) {
        const uint8_t hi = hex_lookup[static_cast<unsigned char>(in[i])];
        const uint8_t lo = hex_lookup[static_cast<unsigned char>(in[i + 1])];
        if (hi == 0xFF || lo == 0xFF)
            throw invalid{"Invalid hex character"};
        out.push_back(std::byte{static_cast<uint8_t>((hi << 4) | lo)});
    }

    return out;
}
