// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "threads.hpp"
#include "system.hpp"
#include "print.hpp"

#include <mutex>
#include <atomic>
#include <map>
#include <queue>
#include <ranges>

#if __has_include(<syslog.h>)
#define USE_SYSLOG
#include <syslog.h>
#else
#ifndef LOG_SYSLOG
#define LOG_SYSLOG
constexpr auto LOG_AUTH = 0;
constexpr auto LOG_AUTHPRIV = 0;
constexpr auto LOG_DAEMON = 0;
constexpr auto LOG_EMERG = 0;
constexpr auto LOG_CRIT = 0;
constexpr auto LOG_INFO = 0;
constexpr auto LOG_WARNING = 0;
constexpr auto LOG_NOTICE = 0;
constexpr auto LOG_ERR = 0;
constexpr auto LOG_DEBUG = 0;
constexpr auto LOG_NDELAY = 0;
constexpr auto LOG_NOWAIT = 0;
constexpr auto LOG_PERROR = 0;
constexpr auto LOG_PID = 0;
#endif
#endif

namespace busuto::service {
using notify_t = void (*)(const std::string&, const char *type);
using error_t = void (*)(const std::exception&);
using task_t = std::function<void()>;

class tasks {
public:
    using timeout_strategy = std::function<std::chrono::milliseconds()>;
    using shutdown_strategy = std::function<void()>;

    explicit tasks(timeout_strategy timeout = &default_timeout, shutdown_strategy shutdown = []() {}) noexcept : timeout_(std::move(timeout)), shutdown_(std::move(shutdown)) {}

    tasks(const tasks&) = delete;
    auto operator=(const tasks&) -> auto& = delete;

    ~tasks() {
        shutdown();
    };

    explicit operator bool() const noexcept {
        const std::lock_guard lock(mutex_);
        return running_;
    }

    auto operator!() const noexcept {
        const std::lock_guard lock(mutex_);
        return !running_;
    }

    auto priority(task_t task) {
        std::unique_lock lock(mutex_);
        if (!running_) return false;
        tasks_.push_front(std::move(task));
        lock.unlock();
        cvar_.notify_one();
        return true;
    }

    auto dispatch(task_t task, std::size_t max = 0) {
        const std::lock_guard lock(mutex_);
        if (!running_) return false;
        if (max && tasks_.size() >= max) return false;
        tasks_.push_back(std::move(task));
        cvar_.notify_one();
        return true;
    }

    void notify() {
        const std::unique_lock lock(mutex_);
        if (running_)
            cvar_.notify_one();
    }

    void startup() {
        const std::unique_lock lock(mutex_);
        if (running_) return;
        running_ = true;
        thread_ = std::thread(&tasks::process, this);
    }

    void shutdown() {
        std::unique_lock lock(mutex_);
        if (!running_) return;
        running_ = false;
        lock.unlock();
        cvar_.notify_all();
        if (thread_.joinable())
            thread_.join();
    }

    auto shutdown(shutdown_strategy handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if (running_) throw std::runtime_error("cannot modify running task queue");
        shutdown_ = handler;
        return *this;
    }

    auto timeout(timeout_strategy handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if (running_) throw std::runtime_error("cannot modify running task queue");
        timeout_ = handler;
        return *this;
    }

    auto errors(error_t handler) -> auto& {
        const std::lock_guard lock(mutex_);
        if (running_) throw std::runtime_error("cannot modify running task queue");
        errors_ = std::move(handler);
        return *this;
    }

    void clear() noexcept {
        const std::lock_guard lock(mutex_);
        tasks_.clear();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(mutex_);
        if (!running_) return true;
        return tasks_.empty();
    }

    auto size() const noexcept {
        const std::lock_guard lock(mutex_);
        return tasks_.size();
    }

    auto active() const noexcept {
        const std::lock_guard lock(mutex_);
        return running_;
    }

protected:
    timeout_strategy timeout_{default_timeout};
    shutdown_strategy shutdown_{[]() {}};
    error_t errors_{[](const std::exception& e) {}};
    std::deque<task_t> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cvar_;
    std::thread thread_;
    volatile bool running_{false};

    static auto default_timeout() -> std::chrono::milliseconds {
        return std::chrono::minutes(1);
    }

    void process() noexcept {
        for (;;) {
            std::unique_lock lock(mutex_);
            if (!running_) break;
            if (tasks_.empty())
                cvar_.wait_for(lock, timeout_());

            if (tasks_.empty()) continue;
            try {
                auto func(std::move(tasks_.front()));
                tasks_.pop_front();

                // unlock before running task
                lock.unlock();
                func();
            } catch (const std::exception& e) {
                errors_(e);
            }
        }

        // run shutdown strategy in this context before joining...
        shutdown_();
    }
};

class timer {
public:
    using id_t = uint64_t;
    using period_t = std::chrono::milliseconds;
    using timepoint_t = std::chrono::steady_clock::time_point;

    static constexpr period_t zero = std::chrono::milliseconds(0);
    static constexpr period_t second = std::chrono::milliseconds(1000);
    static constexpr period_t minute = second * 60;
    static constexpr period_t hour = minute * 60;
    static constexpr period_t day = hour * 24;

    explicit timer(error_t handler = [](const std::exception& e) {}) noexcept : errors_(std::move(handler)) {}

    timer(const timer&) = delete;
    auto operator=(const timer&) -> auto& = delete;

    ~timer() {
        shutdown();
    }

    explicit operator bool() const noexcept {
        return !stop_.load();
    }

    auto operator!() const noexcept {
        return stop_.load();
    }

    void startup(task_t init = [] {}) noexcept {
        if (!thread_.joinable()) {
            startup_ = init;
            thread_ = std::thread(&timer::run, this);
        }
    }

    void shutdown() {
        std::unique_lock lock(lock_);
        if (stop_) return;
        stop_ = true;
        lock.unlock();
        cond_.notify_all();
        if (thread_.joinable())
            thread_.join(); // sync on thread exit
    }

    auto at(const timepoint_t& expires, task_t task) {
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period_t(0), task));
        cond_.notify_all();
        return id;
    }

    auto at(time_t expires, task_t task) {
        const auto now = std::chrono::system_clock::now();
        const auto when = std::chrono::system_clock::from_time_t(expires);
        const auto delay = when - now;
        const auto target = std::chrono::steady_clock::now() +
                            std::chrono::duration_cast<std::chrono::steady_clock::duration>(delay);
        return at(target, std::move(task));
    }

    auto once(const period_t period, task_t task) {
        const auto expires = std::chrono::steady_clock::now() + period;
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period_t(0), task));
        cond_.notify_all();
        return id;
    }

    auto once(uint32_t period, task_t task) {
        return once(std::chrono::milliseconds(period), task);
    }

    auto periodic(uint32_t period, task_t task, uint32_t shorten = 0U) {
        const auto expires = std::chrono::steady_clock::now() + std::chrono::milliseconds(period - shorten);
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period, task));
        cond_.notify_all();
        return id;
    }

    auto periodic(const period_t& period, task_t task, const period_t& shorten = zero) {
        const auto expires = std::chrono::steady_clock::now() + period - shorten;
        const std::lock_guard lock(lock_);
        const auto id = next_++;
        timers_.emplace(expires, std::make_tuple(id, period, task));
        cond_.notify_all();
        return id;
    }

    auto repeats(id_t tid) const {
        const std::lock_guard lock(lock_);
        auto match = std::ranges::find_if(timers_, [&](const auto& pair) {
            const auto& [id, period, task] = pair.second;
            return id == tid;
        });
        return (match != timers_.end()) ? std::get<1>(match->second) : zero;
    }

    auto repeats(id_t tid, const period_t& period) {
        const std::lock_guard lock(lock_);
        for (auto& it : timers_) {
            if (std::get<0>(it.second) != tid) continue;
            std::get<1>(it.second) = period;
            return true;
        }
        return false;
    }

    auto finish(id_t tid) {
        return repeats(tid, zero);
    }

    auto cancel(id_t tid) {
        const std::lock_guard lock(lock_);
        for (auto it = timers_.begin(); it != timers_.end(); ++it) {
            if (std::get<0>(it->second) == tid) {
                timers_.erase(it);
                cond_.notify_all();
                return true;
            }
        }
        return false;
    }

    auto contains(id_t id) const noexcept {
        const std::lock_guard lock(lock_);
        return std::ranges::any_of(timers_, [&](const auto& pair) {
            return std::get<0>(pair.second) == id;
        });
    }

    auto finishes(id_t id) const noexcept {
        const std::lock_guard lock(lock_);
        for (const auto& [expires, item] : timers_) {
            if (std::get<0>(item) == id) return expires;
        }
        return timepoint_t::min();
    }

    void clear() noexcept {
        const std::lock_guard lock(lock_);
        timers_.clear();
    }

    auto size() const noexcept {
        const std::lock_guard lock(lock_);
        return timers_.size();
    }

    auto empty() const noexcept {
        const std::lock_guard lock(lock_);
        if (stop_) return true;
        return timers_.empty();
    }

    auto reset(id_t tid, const period_t& offset = zero, const period_t& interval = zero) {
        const std::lock_guard lock(lock_);
        auto it = std::ranges::find_if(timers_, [&](const auto& pair) {
            const auto& [id, period, task] = pair.second;
            return tid == id;
        });
        if (it != timers_.end()) {
            auto& [id, period, task] = it->second;
            const auto expires = std::chrono::steady_clock::now() + offset;
            if (interval != zero)
                period = interval;

            timers_.erase(it);
            timers_.emplace(expires, std::make_tuple(id, period, task));
            cond_.notify_all();
            return true;
        }
        return false;
    }

    auto refresh(id_t tid) {
        const std::lock_guard lock(lock_);
        for (auto it = timers_.begin(); it != timers_.end(); ++it) {
            const auto& [id, period, task] = it->second;
            if (tid == id) {
                auto result = true;
                if (period == zero) return false;
                const auto current = std::chrono::steady_clock::now();
                const auto expires = current + period;
                const auto when = it->first;
                timers_.erase(it);
                if (when > current) { // if hasnt expired, refresh...
                    result = true;
                    timers_.emplace(expires, std::make_tuple(id, period, task));
                }
                cond_.notify_all();
                return result;
            }
        }
        return false;
    }

protected:
    using timer_t = std::tuple<id_t, period_t, task_t>;
    error_t errors_{[](const std::exception& e) {}};
    std::multimap<timepoint_t, timer_t> timers_;
    mutable std::mutex lock_;
    std::condition_variable cond_;
    std::thread thread_;
    std::atomic<bool> stop_{false};
    task_t startup_{[] {}};
    id_t next_{0};

    void run() noexcept {
        startup_();
        for (;;) {
            std::unique_lock lock(lock_);
            if (!stop_ && timers_.empty())
                cond_.wait(lock);

            if (stop_) break;
            if (timers_.empty()) continue;

            auto it = timers_.begin();
            auto expires = it->first;
            const auto now = std::chrono::steady_clock::now();
            if (expires <= now) {
                const auto item(std::move(it->second));
                const auto& [id, period, task] = item;
                timers_.erase(it);
                if (period != zero) {
                    expires += period;
                    timers_.emplace(expires, std::make_tuple(id, period, task));
                }
                lock.unlock();
                try {
                    task();
                } catch (const std::exception& e) {
                    errors_(e);
                }
                continue;
            }
            cond_.wait_until(lock, expires);
        }
    }
};

class pool {
public:
    pool() = default;
    pool(const pool&) = delete;
    auto operator=(const pool&) -> auto& = delete;

    explicit pool(std::size_t count, task_t init = [] {}) {
        start(count, init);
    }

    ~pool() {
        drain();
    }

    explicit operator bool() const noexcept {
        const std::lock_guard lock(mutex_);
        return started_;
    }

    auto operator!() const noexcept {
        const std::lock_guard lock(mutex_);
        return !started_;
    }

    auto size() const noexcept {
        const std::lock_guard lock(mutex_);
        return workers_.size();
    }

    void resize(std::size_t count) {
        drain();
        if (count) start(count);
    }

    void start(std::size_t count = 0, task_t init = [] {}) {
        if (count == 0)
            count = std::thread::hardware_concurrency();
        if (count == 0)
            count = 1;
        const std::lock_guard lock(mutex_);
        if (started_) return;
        accepting_ = true;
        started_ = true;
        workers_.clear();
        workers_.reserve(count);
        startup_ = init;
        for (std::size_t i = 0; i < count; ++i) {
            workers_.emplace_back([this] {
                startup_();
                while (true) {
                    std::function<void()> task;
                    std::unique_lock lock(mutex_);
                    cvar_.wait(lock, [this] {
                        return !accepting_ || !tasks_.empty();
                    });

                    if (!accepting_ && tasks_.empty()) return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                    lock.unlock();
                    task();
                }
            });
        }
    }

    auto dispatch(task_t task) {
        const std::lock_guard lock(mutex_);
        if (!accepting_) return false;
        tasks_.push(std::move(task));
        cvar_.notify_one();
        return true;
    }

    void startup() noexcept {
        start();
    }

    void shutdown() noexcept {
        drain();
    }

protected:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    mutable std::mutex mutex_;
    std::condition_variable cvar_;
    task_t startup_{[] {}};
    std::atomic<bool> accepting_{false};
    volatile bool started_{false};

    void drain() noexcept {
        std::unique_lock lock(mutex_);
        accepting_ = false;
        cvar_.notify_all();
        lock.unlock();

        // joins are outside lock so we dont block if waiting to join
        for (auto& worker : workers_) {
            if (worker.joinable())
                worker.join();
        }

        lock.lock();
        workers_.clear();
        started_ = false;
    }
};

class logger final {
public:
    class stream final : public std::ostringstream {
    public:
        stream(const stream&) = delete;

        ~stream() final {
            const std::lock_guard lock(from_.locker_);
#ifdef USE_SYSLOG
            if (from_.opened_)
                ::syslog(type_, "%s", str().c_str());
#endif
            from_.notify_(str(), prefix_);
            if (from_.verbose_ >= level_)
                std::cerr << prefix_ << ": " << str() << std::endl;
            if (exit_)
                std::quick_exit(exit_);
        }

    private:
        friend class logger;

        stream(unsigned level, int type, const char *prefix, logger& from, int ex = 0) : from_(from), level_{level}, type_{type}, prefix_{prefix}, exit_{ex} {}

        logger& from_;
        unsigned level_;
        int type_;
        const char *prefix_;
        int exit_{0};
    };

    auto verbose() const noexcept {
        return verbose_;
    }

    void verbose(unsigned verbose) noexcept {
        verbose_ = verbose;
    }

    void set(unsigned verbose, notify_t notify = [](const std::string& str, const char *type) {}) {
        verbose_ = verbose;
        notify_ = notify;
    }

#ifdef USE_SYSLOG
    void open(const char *id, int level = LOG_INFO, int facility = LOG_DAEMON, int flags = LOG_CONS | LOG_NDELAY) {
        ::openlog(id, flags, facility);
        ::setlogmask(LOG_UPTO(level));
        opened_ = true;
    }

    void close() {
        ::closelog();
        opened_ = false;
    }
#else
    void open(const char *id, int level = LOG_INFO, int facility = 0, int flags = 0) {}
    void close() {}
#endif

#ifdef NDEBUG
    auto debug([[maybe_unused]] unsigned level) {
        return output::null();
    }
#else
    auto debug(unsigned level) {
        return stream(level, LOG_DEBUG, "debug", *this);
    }
#endif

    auto fatal(int ex = -1) {
        return stream(0, LOG_CRIT, "fatal", *this, ex);
    }

    auto error() {
        return stream(0, LOG_ERR, "error", *this);
    }

    auto warning() {
        return stream(1, LOG_WARNING, "warn", *this);
    }

    auto notice() {
        return stream(1, LOG_NOTICE, "note", *this);
    }

    auto info() {
        return stream(2, LOG_INFO, "info", *this);
    }

private:
    notify_t notify_{[](const std::string& str, const char *type) {}};
    unsigned verbose_{1};
    std::mutex locker_;
#ifdef USE_SYSLOG
    bool opened_{false};
#endif
};

inline void parallel(std::size_t count, task_t task) {
    count = thread::concurrency(count);
    std::vector<std::thread> threads;
    threads.reserve(count);
    for (std::size_t i = 0; i < count; ++i)
        threads.emplace_back(task);
    for (auto& thread : threads)
        thread.join();
}
} // namespace busuto::service

namespace busuto {
auto is_service() noexcept -> bool;
auto background() noexcept -> bool;

extern std::atomic<bool> running;
extern service::logger system_logger;
extern service::timer system_timer;
extern service::pool system_pool;

// use timer to delay socket or file close to delay descriptor reuse...
inline void delayed_close(int handle, uint32_t msecs = 1000) {
    system_timer.once(msecs, [&handle] { ::close(handle); });
}

#ifdef DEBUG
template <typename... Args>
void debug(unsigned level, std::string_view fmt, Args&&...args) { // NOLINT
    if (system_logger.verbose() >= level) {
        auto bound = std::make_tuple(std::decay_t<Args>(std::forward<Args>(args))...);
        std::apply([&](auto&&...unpacked) {
            output::debug() << std::vformat(fmt, std::make_format_args(unpacked...));
        },
        bound);
    }
}
#else
template <typename... Args>
void debug(unsigned level, std::string_view fmt, Args&&...args) { // NOLINT
}
#endif
} // namespace busuto
