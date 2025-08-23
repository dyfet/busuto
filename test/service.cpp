// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "print.hpp"
#include "service.hpp"
#include <cassert>

using namespace busuto;

namespace {
void test_timer() {
    int fast = 0; // scope of function...
    int slow = 0;
    service::timer timer; // a private timer!
    timer.startup();
    // timer.startup([]{print("started timer thread\n");});
    timer.periodic(std::chrono::milliseconds(150), [&slow] {
        ++slow;
    });
    auto id = timer.periodic(std::chrono::milliseconds(50), [&fast] {
        ++fast;
    });

    this_thread::sleep(400);
    assert(timer.size() == 2);
    assert(timer.contains(id));
    timer.cancel(id);
    assert(!timer.contains(id));
    assert(timer.size() == 1);
    auto saved = fast;
    auto prior = slow;
    this_thread::sleep(200);
    assert(fast == saved); // cppcheck-suppress knownConditionTrueFalse
    timer.shutdown();
    assert(slow >= 2 && fast > slow && slow <= 5);
    assert(slow > prior); // cppcheck-suppress knownConditionTrueFalse
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_timer();
    } catch (...) {
        return -1;
    }
    return 0;
}
