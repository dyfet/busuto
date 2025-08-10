// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "safe.hpp"
#include <cassert>

using namespace busuto::safe;
using namespace busuto;

namespace {
void test_safe_eq() {
    char yes[] = {'y', 'e', 's', 0};
    yes[0] = 'y';
    assert(!eq("yes", "no")); // NOLINT
    assert(eq("yes", yes));
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_safe_eq();
    } catch (...) {
        return -1;
    }
    return 0;
}
