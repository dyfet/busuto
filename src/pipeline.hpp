// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "threads.hpp"

#include <atomic>

namespace busuto::system {
template <typename T, std::size_t S>
class pipeline {
public:
    explicit operator bool() const noexcept { return !closed_; }
    auto operator!() const noexcept { return closed_; }
    auto capacity() const noexcept { return S; }
    virtual ~pipeline() { close(); }

    auto is_open() const noexcept {
        return !closed_;
    }

    auto empty() const noexcept {
        const guard_t lock(lock_);
        return count_ == 0;
    }

    auto count() const noexcept {
        const guard_t lock(lock_);
        return count_;
    }

    void clear() {
        const guard_t lock(lock_);
        auto prior = count_;
        while (count_ && drop_head(false))
            ;
        if (prior > count_ && !closed_)
            input_.notify_one();
    }

    void close() {
        if (!closed_.exchange(true)) {
            output_.notify_all();
            input_.notify_all();
            std::this_thread::yield();
            clear();
        }
    }

    auto drop() {
        const guard_t lock(lock_);
        return drop_head(count_ == S);
    }

    auto drop_if() { // drop if full
        const guard_t lock(lock_);
        if (count_ == S) return drop_head(true);
        return false;
    }

    auto push(T&& data) {
        lock_t lock(lock_);
        while (!closed_) {
            if (count_ < S) {
                data_[tail_] = std::move(data);
                tail_ = (tail_ + 1) % S;
                if (count_++ == 0) // notify no longer empty
                    output_.notify_one();
                return true;
            }
            full(lock);
        }
        return false;
    }

    auto push(const T& data) {
        lock_t lock(lock_);
        while (!closed_) {
            if (count_ < S) {
                data_[tail_] = data;
                tail_ = (tail_ + 1) % S;
                if (count_++ == 0) // notify no longer empty
                    output_.notify_one();
                return true;
            }
            full(lock);
        }
        return false;
    }

    auto pull(T& out) {
        lock_t lock(lock_);
        while (!closed_) {
            if (count_ > 0) {
                out = std::move(data_[head_]);
                clear_item(data_[head_], false); // moved...
                head_ = (head_ + 1) % S;
                if (count_-- == S) // notify only when was full...
                    input_.notify_one();
                return true;
            }
            wait(lock);
        }
        return false;
    }

    template <typename Func>
    requires std::invocable<Func, const T&>
    auto peek(Func func) const -> bool {
        const guard_t lock(lock_);
        if (!count_) return false;
        func(data_[head_]);
        return true;
    }

    auto operator<<(T&& data) -> pipeline& {
        if (!push(std::move(data)))
            throw invalid("Pipeline closed");
        return *this;
    }

    auto operator<<(const T& data) -> pipeline& {
        if (!push(data))
            throw invalid("Pipeline closed");
        return *this;
    }

    auto operator>>(T& out) -> pipeline& {
        if (!pull(out))
            throw invalid("Pipeline closed");
        return *this;
    }

protected:
    static_assert(S > 0, "pipeline size must be positive");
    using lock_t = std::unique_lock<std::mutex>;
    using guard_t = std::lock_guard<std::mutex>;

    mutable std::mutex lock_;
    std::condition_variable input_, output_;
    T data_[S]{};
    unsigned head_{0}, tail_{0}, count_{0};
    std::atomic<bool> closed_{false};

    virtual void wait(lock_t& lock) {
        output_.wait(lock, [&] { return count_ > 0; });
    }

    virtual void full(lock_t& lock) {
        input_.wait(lock, [&] { return count_ < S; });
    }

    virtual void drop([[maybe_unused]] const T& obj) {}

    void clear_item(T& data, bool destroy = true) {
        if constexpr (std::is_pointer_v<T>) {
            if (destroy)
                delete data;
            data = nullptr;
        } else {
            data = T{};
        }
    }

    auto drop_head(bool notify = true) {
        if (!count_) return false;
        clear_item(data_[head_], true);
        head_ = (head_ + 1) % S;
        count_--;
        if (notify)
            input_.notify_one();
        return true;
    }
};

template <typename T, std::size_t S>
class drop_pipeline : public pipeline<T, S> {
public:
    drop_pipeline() = default;

private:
    using lock_t = std::unique_lock<std::mutex>;

    void full([[maybe_unused]] lock_t& lock) final {
        this->drop(this->data_[this->head_]); // optional drop fast proc
        this->drop_head(false);               // silent drop...
    }
};

template <typename T, std::size_t S>
class throw_pipeline : public pipeline<T, S> {
public:
    throw_pipeline() = default;

private:
    using lock_t = std::unique_lock<std::mutex>;

    void full([[maybe_unused]] lock_t& lock) final {
        throw invalid("Pipeline ful");
    }
};
} // namespace busuto::system
