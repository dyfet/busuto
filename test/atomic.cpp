// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "atomic.hpp"

#include <string>
#include <cassert>

using namespace busuto;

namespace {
void test_atomic_once() {
    const atomic::once_t once;
    assert(is(once));
    assert(!is(once));
    assert(!once);
}

void test_atomic_sequence() {
    atomic::sequence_t<uint8_t> bytes(3);
    assert(*bytes == 3);
    assert(static_cast<uint8_t>(bytes) == 4);
}

void test_atomic_dictionary() {
    atomic::dictionary_t<int, std::string> dict;
    dict.insert_or_assign(1, "one");
    dict.insert_or_assign(2, "two");
    assert(dict.find(1).value() == "one"); // NOLINT
    assert(dict.size() == 2);
    assert(dict.contains(2));
    dict.remove(1);
    assert(!dict.contains(1));
    assert(dict.size() == 1);
    dict.each([](const int& key, std::string& value) {
        assert(key == 2);
        assert(value == "two");
        value = "two two";
    });
    assert(dict.find(2).value() == "two two"); // NOLINT
}
} // namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_atomic_once();
        test_atomic_sequence();
        test_atomic_dictionary();
    } catch (...) {
        return -1;
    }
    return 0;
}
