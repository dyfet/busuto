// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "safe.hpp"
#include <cassert>
#include <iostream>

using namespace busuto::safe;
using namespace busuto;

namespace {
void test_safe_eq() {
    char yes[] = {'y', 'e', 's', 0};
    yes[0] = 'y';
    assert(!eq("yes", "no")); // NOLINT
    assert(eq("yes", yes));
}

void test_safe_output() {
    format_buffer<32> output;
    output << "hi " << "there";
    assert(eq(*output, "hi there"));
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_safe_eq();
        test_safe_output();
    } catch (...) {
        return -1;
    }
    return 0;
}
