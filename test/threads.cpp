// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "threads.hpp"
#include <cassert>

using namespace busuto;

auto main(int /* argc */, char ** /* argv */) -> int {
    assert(std::at_quick_exit([] {}) == 0);
    try {
        this_thread::sleep(10);
    } catch (...) {
        return -1;
    }

    std::atomic<int> total = 0;
    thread::parallel_func(3, [&total] {
        total.fetch_add(2);
    });
    assert(total == 6);

    std::quick_exit(0);
    return 1;
}
