// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "threads.hpp"

namespace busuto::lock {
template <typename T>
class exclusive {
public:
    template <typename... Args>
    explicit exclusive(Args&&...args) : data_(std::forward<Args>(args)...) {}

protected:
    T data_{};

private:
    template <typename U>
    friend class exclusive_ptr;
    template <typename U>
    friend class exclusive_guard;
    mutable std::mutex lock_;
};

template <typename T>
class shared {
public:
    template <typename... Args>
    explicit shared(Args&&...args) : data_(std::forward<Args>(args)...) {}

protected:
    T data_{};

private:
    template <typename U>
    friend class reader_ptr;
    template <typename U>
    friend class writer_ptr;
    mutable std::shared_mutex lock_;
};

template <typename U>
class exclusive_ptr final : public std::unique_lock<std::mutex> {
public:
    exclusive_ptr() = delete;
    exclusive_ptr(const exclusive_ptr&) = delete;
    auto operator=(const exclusive_ptr&) -> exclusive_ptr& = delete;

    explicit exclusive_ptr(exclusive<U>& obj) : unique_lock(obj.lock_), ptr_(&obj.data_) {}

    exclusive_ptr(exclusive_ptr&& from) noexcept : std::unique_lock<std::mutex>(std::move(from)), ptr_(std::exchange(from.ptr_, nullptr)) {}

    ~exclusive_ptr() = default;

    auto operator->() {
        if (!ptr_) throw error("exclusive lock error");
        return ptr_;
    }

    auto operator*() -> U& {
        if (!ptr_) throw error("exclusive lock error");
        return *ptr_;
    }

    auto operator=(exclusive_ptr&& from) noexcept -> exclusive_ptr& {
        if (this == &from) return *this;
        if (owns_lock()) unlock();
        std::unique_lock<std::mutex>::operator=(std::move(from));
        ptr_ = std::exchange(from.ptr_, nullptr);
        return *this;
    }

    template <typename I>
    auto operator[](const I& index) -> decltype(std::declval<U&>()[index]) {
        if (!ptr_) throw error("exclusive lock error");
        return ptr_->operator[](index);
    }

private:
    U *ptr_{nullptr};
};

template <typename U>
class exclusive_guard final {
public:
    exclusive_guard() = delete;
    exclusive_guard(const exclusive_guard&) = delete;
    auto operator=(const exclusive_guard&) -> exclusive_guard& = delete;

    explicit exclusive_guard(exclusive<U>& obj) : obj_(obj) {
        obj_.lock_.lock();
    }

    ~exclusive_guard() {
        obj_.lock_.unlock();
    }

    auto operator->() {
        return &obj_.data_;
    }

    auto operator*() -> U& {
        return obj_.data_;
    }

    template <typename I>
    auto operator[](const I& index) -> decltype(std::declval<U&>()[index]) {
        return obj_.data_.operator[](index);
    }

private:
    exclusive<U>& obj_;
};

template <typename U>
class reader_ptr final : public std::shared_lock<std::shared_mutex> {
public:
    reader_ptr() = delete;
    reader_ptr(const reader_ptr&) = delete;
    auto operator=(const reader_ptr&) -> reader_ptr& = delete;

    explicit reader_ptr(shared<U>& obj) : std::shared_lock<std::shared_mutex>(obj.lock_), ptr_(&obj.data_) {}

    reader_ptr(reader_ptr&& from) noexcept : std::shared_lock<std::shared_mutex>(std::move(from)), ptr_(std::exchange(from.ptr_, nullptr)) {}

    ~reader_ptr() = default;

    auto operator->() const -> const U * {
        if (!ptr_) throw error("reader lock error");
        return ptr_;
    }

    auto operator*() const -> const U& {
        if (!ptr_) throw error("reader lock error");
        return *ptr_;
    }

    auto operator=(reader_ptr&& from) noexcept -> reader_ptr& {
        if (this == &from) return *this;
        if (owns_lock()) unlock();
        std::shared_lock<std::shared_mutex>::operator=(std::move(from));
        ptr_ = std::exchange(from.ptr_, nullptr);
        return *this;
    }

    template <typename I>
    auto operator[](const I& index) const -> decltype(std::declval<const U&>()[index]) {
        if (!ptr_) throw error("reader lock error");
        return ptr_->operator[](index);
    }

    template <typename I>
    auto at(const I& index) const {
        if (!ptr_) throw error("reader lock error");
        return ptr_->at(index);
    }

private:
    const U *ptr_{nullptr};
};

template <typename U>
class writer_ptr final : public std::unique_lock<std::shared_mutex> {
public:
    writer_ptr() = delete;
    writer_ptr(const writer_ptr&) = delete;
    auto operator=(const writer_ptr&) -> writer_ptr& = delete;

    explicit writer_ptr(shared<U>& obj) : std::unique_lock<std::shared_mutex>(obj.lock_), ptr_(&obj.data_) {}

    writer_ptr(writer_ptr&& from) noexcept : std::unique_lock<std::shared_mutex>(std::move(from)), ptr_(std::exchange(from.ptr_, nullptr)) {}

    ~writer_ptr() = default;

    auto operator->() {
        if (!ptr_) throw error("writer lock error");
        return ptr_;
    }

    auto operator*() -> U& {
        if (!ptr_) throw error("writer lock error");
        return *ptr_;
    }

    auto operator=(writer_ptr&& from) noexcept -> writer_ptr& {
        if (this == &from) return *this;
        if (owns_lock()) unlock();
        std::unique_lock<std::shared_mutex>::operator=(std::move(from));
        ptr_ = std::exchange(from.ptr_, nullptr);
        return *this;
    }

    template <typename I>
    auto operator[](const I& index) -> decltype(std::declval<U&>()[index]) {
        if (!ptr_) throw error("writer lock error");
        return ptr_->operator[](index);
    }

private:
    U *ptr_{nullptr};
};
} // namespace busuto::lock
