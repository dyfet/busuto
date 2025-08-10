// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "system.hpp"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <cstring>

#include <sched.h>

namespace busuto::this_thread {
using namespace std::this_thread;

inline auto priority(int priority) -> int {
    auto tid = pthread_self();
    struct sched_param sp{};
    int policy{};

    std::memset(&sp, 0, sizeof(sp));
    policy = SCHED_OTHER;
#ifdef SCHED_FIFO
    if (priority > 0) {
        policy = SCHED_FIFO;
        priority = sched_get_priority_min(policy) + priority - 1;
        priority = std::min(priority, sched_get_priority_max(policy));
    }
#endif
#ifdef SCHED_BATCH
    if (priority < 0) {
        policy = SCHED_BATCH;
        priority = 0;
    }
#endif
    if (policy == SCHED_OTHER)
        priority = 0;

    sp.sched_priority = priority;
    return pthread_setschedparam(tid, policy, &sp) == 0;
}

inline void sleep(unsigned msec) {
    sleep_for(std::chrono::milliseconds(msec));
}
} // namespace busuto::this_thread

namespace busuto::thread {
inline auto concurrency(unsigned count) {
    if (!count) count = std::thread::hardware_concurrency();
    if (!count) return 1U;
    return std::min(count, std::thread::hardware_concurrency());
}

template <typename Func, typename... Args>
requires std::invocable<Func, Args...>
inline void parallel_func(std::size_t count, Func&& func, Args&&...args) {
    auto task = std::bind_front(std::forward<Func>(func), std::forward<Args>(args)...);
    std::vector<std::thread> threads;
    count = concurrency(count);
    threads.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        threads.emplace_back(task);
    for (auto& thread : threads)
        thread.join();
}
} // namespace busuto::thread

namespace busuto {
#if (defined(_LIBCPP_VERSION) && (!defined(_LIBCPP_STD_VER) || _LIBCPP_STD_VER < 20 || defined(_LIBCPP_HAS_NO_EXPERIMENTAL_STOP_TOKEN)))
#define BUSUTO_BSD_THREADS
class thread_t final {
public:
    thread_t() = default;
    thread_t(thread_t&& other) noexcept = default;
    thread_t(const thread_t&) = delete;
    auto operator=(const thread_t&) -> thread_t& = delete;
    auto operator=(thread_t&&) -> thread_t& = default;

    template <typename Callable, typename... Args>
    requires(!std::is_same_v<std::decay_t<Callable>, thread_t>)
    explicit thread_t(Callable&& f, Args&&...args) : thread_(std::forward<Callable>(f), std::forward<Args>(args)...) {}

    ~thread_t() {
        if (thread_.joinable())
            thread_.join();
    }

    operator std::thread::id() const noexcept { return thread_.get_id(); }

    auto native_handle() { return thread_.native_handle(); }
    auto get_id() const noexcept { return thread_.get_id(); }
    auto joinable() const noexcept -> bool { return thread_.joinable(); }

    void join() {
        if (thread_.joinable())
            thread_.join();
    }

    void swap(thread_t& other) noexcept {
        std::swap(thread_, other.thread_);
    }

    void detach() noexcept {
        if (thread_.joinable())
            thread_.detach();
    }

private:
    std::thread thread_;
};
#else
#define BUSUTO_HAS_JTHREAD
using thread_t = std::jthread;
#endif
} // namespace busuto
