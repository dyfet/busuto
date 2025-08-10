// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "expected.hpp"

#include <string>
#include <cassert>

using namespace busuto;

namespace {
auto ret_error() -> expected<std::string, int> {
    return expected<std::string, int>(23);
}

auto ret_string() -> expected<std::string, int> {
    return expected<std::string, int>("hello");
}

void test_expected() {
    auto e1 = ret_error();
    auto e2 = ret_string();

    assert(e1.error() == 23);
    assert(e2.value() == "hello");
}
} // namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_expected();
    } catch (...) {
        return -1;
    }
    return 0;
}
