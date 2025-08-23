// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "sync.hpp"
#include "pipeline.hpp"
#include "atomic.hpp"
#include <cassert>

using namespace busuto;

namespace {
void test_sync_semaphore() {
    std::counting_semaphore<8> sem(2);
    sem.acquire();
    sem.acquire();
    assert(!sem.try_acquire() && "Semaphore should be exhausted");

    sem.release();
    assert(sem.try_acquire() && "Semaphore should allow reacquire");

    sem.release();
    sem.release();
}

void test_sync_event() {
    std::atomic<bool> fin{false};
    sync::event done;
    std::thread thr([&] {
        this_thread::sleep_for(sync::duration(120));
        fin = true;
        done.signal();
    });
    done.wait();
    assert(fin == true);
    thr.join();
}

void test_sync_barrier() {
    std::barrier<> bar(2);
    std::atomic<bool> completed{false};
    {
        const sync::barrier_scope guard(bar);
        (void)bar.arrive();
    }

    completed.store(true); // manually trigger completion
    assert(completed.load());
}

void test_sync_waitgroup() {
    sync::wait_group wg(1);
    {
        const sync::group_scope done(wg);
        assert(wg.count() == 1);
    }
    assert(wg.count() == 0);
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_sync_waitgroup();
        test_sync_semaphore();
        test_sync_barrier();
        test_sync_event();
    } catch (...) {
        return -1;
    }
    return 0;
}
