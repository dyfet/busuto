// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "scan.hpp"
#include "print.hpp"

#include <cassert>

using namespace busuto;

namespace {
void test_hex_parsing() {
    assert(parse_hex("f0") == 240);
    assert(parse_hex<uint16_t>("fff0") == 65520);
    assert(parse_hex<uint16_t>("0xfff0") == 65520);
    assert(parse_hex<uint16_t>("$fff0") == 65520);
}

void test_unsigned_parsing() {
    auto myint = parse_unsigned<uint16_t>("23");
    assert(myint == 23);
    assert(sizeof(myint) == 2); // NOLINT
}

void test_try_fallbacks() {
    std::string_view input = "-1";
    auto fallback = 42U;
    auto result = try_function([&] {
        return parse_unsigned<>(input);
    },
    fallback);
    assert(result == fallback);
}

void test_scan_yesno() {
    auto text = "true";
    assert(parse_bool(text) == true);

    text = "Off";
    assert(parse_bool(text) == false);
}

void test_scan_duration() {
    auto text = "5m";
    assert(parse_duration(text) == 300);

    text = "300";
    assert(parse_duration(text) == 300);
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_hex_parsing();
        test_unsigned_parsing();
        test_try_fallbacks();
        test_scan_yesno();
        test_scan_duration();
    } catch (const std::exception& e) {
        print("ERR: {}\n", e.what());
        return -1;
    }
    return 0;
}
