// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "threads.hpp"

#include <semaphore>
#include <barrier>

namespace busuto::sync {
using duration = std::chrono::milliseconds;

template <std::ptrdiff_t Value>
using semaphore = std::counting_semaphore<Value>;

template <typename Completion = void (*)()>
class barrier_scope final {
public:
    explicit barrier_scope(std::barrier<Completion>& barrier)
    requires std::invocable<Completion>
        : barrier_(&barrier) {}

    barrier_scope(barrier_scope&& other) noexcept : barrier_(std::exchange(other.barrier_, nullptr)), dropped_(std::exchange(other.dropped_, true)), waited_(std::exchange(other.waited_, true)) {}

    ~barrier_scope() {
        if (barrier_ && !dropped_ && !waited_) {
            barrier_->arrive_and_wait();
        }
    }

    auto operator=(barrier_scope&& other) noexcept -> barrier_scope& {
        if (this == &other) return *this;
        drop();
        barrier_ = std::exchange(other.barrier_, nullptr);
        dropped_ = std::exchange(other.dropped_, true);
        waited_ = std::exchange(other.waited_, true);
        return *this;
    }

    void drop() {
        if (!dropped_ && !waited_) {
            barrier_->arrive_and_drop();
            dropped_ = true;
        }
    }

    void wait() {
        if (!dropped_ && !waited_) {
            barrier_->arrive_and_wait();
            waited_ = true;
        }
    }

    barrier_scope(const barrier_scope&) = delete;
    auto operator=(const barrier_scope&) -> barrier_scope& = delete;

private:
    std::barrier<Completion> *barrier_{nullptr};
    bool dropped_{false};
    bool waited_{false};
};

class event final {
public:
    explicit event() : bin_(std::make_shared<std::binary_semaphore>(0)) {}

    void wait() {
        bin_->acquire();
    }

    void signal() {
        bin_->release();
    }

    auto wait_for(const duration& rel_time) {
        return bin_->try_acquire_for(rel_time);
    }

    auto wait_until(const timepoint_t& abs_time) {
        return bin_->try_acquire_until(abs_time);
    }

    auto try_wait() {
        return bin_->try_acquire();
    }

    event(const event&) = default;
    auto operator=(const event&) -> event& = default;
    event(event&&) noexcept = default;
    auto operator=(event&&) noexcept -> event& = default;

private:
    std::shared_ptr<std::binary_semaphore> bin_;
};

template <std::ptrdiff_t Value>
class semaphore_scope final {
public:
    explicit semaphore_scope(semaphore<Value>& sem) : sem_(&sem) {}

    semaphore_scope(semaphore_scope&& other) noexcept : sem_(std::exchange(other.sem_, nullptr)) {}

    auto operator=(semaphore_scope&& other) noexcept -> semaphore_scope& {
        if (this == &other) return *this;
        release(); // release current
        sem_ = std::exchange(other.sem_, nullptr);
        return *this;
    }

    semaphore_scope(const semaphore_scope&) = delete;
    auto operator=(const semaphore_scope&) -> semaphore_scope& = delete;

    ~semaphore_scope() {
        release();
    }

private:
    void release() {
        if (sem_) {
            sem_->release();
            sem_ = nullptr;
        }
    }

    semaphore<Value> *sem_{nullptr};
};

class wait_group final {
public:
    explicit wait_group(unsigned init = 0) noexcept : count_(init) {}

    wait_group(const wait_group&) = delete;
    auto operator=(const wait_group&) -> wait_group& = delete;
    wait_group(wait_group&&) noexcept = delete;
    auto operator=(wait_group&&) noexcept -> wait_group& = delete;

    ~wait_group() {
        wait();
    }

    auto operator++() noexcept -> wait_group& {
        add(1);
        return *this;
    }

    auto operator+=(unsigned count) noexcept -> wait_group& {
        add(count);
        return *this;
    }

    void add(unsigned count) noexcept {
        const std::lock_guard<std::mutex> lock(lock_);
        count_ += count;
    }

    auto release() noexcept {
        const std::lock_guard<std::mutex> lock(lock_);
        if (count_ == 0) return true;
        if (--count_ == 0) {
            cond_.notify_all();
            return true;
        }
        return false;
    }

    void wait() noexcept {
        std::unique_lock<std::mutex> lock(lock_);
        cond_.wait(lock, [this] { return count_ == 0; });
    }

    auto wait_for(std::chrono::milliseconds timeout) noexcept {
        std::unique_lock<std::mutex> lock(lock_);
        return cond_.wait_for(lock, timeout, [this] { return count_ == 0; });
    }

    auto wait_until(std::chrono::steady_clock::time_point tp) noexcept {
        std::unique_lock<std::mutex> lock(lock_);
        return cond_.wait_until(lock, tp, [this] { return count_ == 0; });
    }

    auto count() const noexcept {
        const std::lock_guard<std::mutex> lock(lock_);
        return count_;
    }

private:
    unsigned count_{0};
    mutable std::mutex lock_;
    std::condition_variable cond_;
};

class group_scope final {
public:
    explicit group_scope(wait_group& wg) noexcept : wg_(&wg) {}

    group_scope(group_scope&& other) noexcept : wg_(std::exchange(other.wg_, nullptr)) {}

    auto operator=(group_scope&& other) noexcept -> group_scope& {
        if (this == &other) return *this;
        release();
        wg_ = std::exchange(other.wg_, nullptr);
        return *this;
    }

    group_scope(const group_scope&) = delete;
    auto operator=(const group_scope&) -> group_scope& = delete;

    ~group_scope() {
        release();
    }

private:
    void release() noexcept {
        if (wg_) {
            wg_->release();
            wg_ = nullptr;
        }
    }

    wait_group *wg_{nullptr};
};
} // namespace busuto::sync
