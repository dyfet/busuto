// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "threads.hpp"
#include <cassert>

using namespace busuto;

namespace {
void test_sleep() {
    try {
        this_thread::sleep(10);
    } catch (...) {
        std::quick_exit(-1);
    }
}

void test_atomic() {
    std::atomic<int> total = 0;
    thread::parallel_func(3, [&total] {
        total.fetch_add(2);
    });
    assert(total == 6);
}

void test_notify() {
    system::notify_t notifier;
    assert(notifier.wait(0) == false);
    notifier.signal();
    assert(notifier.wait(0) == true);
    notifier.clear();
    assert(notifier.wait(0) == false);
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    assert(std::at_quick_exit([] {}) == 0);
    test_sleep();
    test_atomic();
    test_notify();
    std::quick_exit(0);
    return 1;
}
