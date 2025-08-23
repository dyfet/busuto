// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "binary.hpp"
#include <cassert>

using namespace busuto;

namespace {
void test_hex_codec() {
    const byte_array src{"hello", 5};
    auto hex = src.to_hex();
    assert(hex == "68656C6C6F");

    auto restored = busuto::from_hex(hex);
    assert(restored == src);
}

void test_base64_codec() {
    const byte_array src{"world", 5};
    auto b64 = busuto::to_b64(src); // uses base64 now
    assert(b64 == "d29ybGQ=");

    auto restored = busuto::from_b64(b64);
    assert(restored == src);
}

void test_utf8_utils() {
    assert(util::is_utf8("\xc3\xb1"));
    assert(!util::is_utf8("\xa0\xa1"));
}

void test_subspan_and_span_mutation() {
    byte_array arr{"foobar", 6};
    auto sub = arr.subspan(3, 3); // "bar"
    assert(sub.size() == 3);
    assert(std::to_integer<char>(sub[0]) == 'b');

    auto mspan = arr.span_mut();
    mspan[0] = std::byte{'F'};
    assert(arr[0] == std::byte{'F'});
}

void test_u8data_and_conversion() {
    byte_array arr{"abc", 3};
    const uint8_t *ptr = arr.u8data();
    assert(ptr[0] == 'a');

    auto vec = arr.to_u8vector();
    assert(vec.size() == 3);
    assert(vec[2] == 'c');
}

void test_swap_and_slice() {
    byte_array a{"123456", 6};
    byte_array b{"ABCDEF", 6};
    a.swap(b);

    assert(a.to_hex() == "414243444546"); // hex of "ABCDEF"
    assert(b.to_hex() == "313233343536"); // hex of "123456"

    auto sliced = a.slice(1, 4);
    assert(sliced.to_hex() == "424344");
}

void test_invalid_decode_inputs() {
    try {
        auto ba = busuto::from_hex("ABC"); // NOLINT
        assert(false && "Should throw on odd hex length");
    } catch (const std::invalid_argument&) {} // NOLINT

    try {
        auto ba = busuto::from_b64("****"); // NOLINT
        assert(false && "Should throw on bad base64");
    } catch (const std::invalid_argument&) {} // NOLINT
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_hex_codec();
        test_base64_codec();
        test_subspan_and_span_mutation();
        test_u8data_and_conversion();
        test_swap_and_slice();
        test_invalid_decode_inputs();
        test_utf8_utils();
    } catch (...) {
        return -1;
    }
    return 0;
}
