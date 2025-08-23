// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#ifndef _WIN32
#include "sockets.hpp"
#include "sync.hpp"

#include <ifaddrs.h>

namespace busuto::socket {
using iface_t = struct ifaddrs *;

class networks {
public:
    using reference = struct ifaddrs&;
    using pointer = struct ifaddrs *;
    using const_reference = const struct ifaddrs&;
    using size_type = std::size_t;
    using value_type = struct ifaddrs;

    constexpr static struct ifaddrs *npos = nullptr;

    class iterator {
    public:
        using value_type = struct ifaddrs;
        using pointer = struct ifaddrs *;
        using reference = struct ifaddrs&;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::forward_iterator_tag;

        constexpr iterator() = default;
        constexpr explicit iterator(pointer ptr) : ptr_(ptr) {}

        constexpr auto operator*() const -> reference { return *ptr_; }
        constexpr auto operator->() const -> pointer { return ptr_; }

        constexpr auto operator++() -> iterator& {
            ptr_ = ptr_ ? ptr_->ifa_next : nullptr;
            return *this;
        }

        constexpr auto operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }

        friend constexpr auto operator==(const iterator& lhs, const iterator& rhs) {
            return lhs.ptr_ == rhs.ptr_;
        }

        friend constexpr auto operator!=(const iterator& lhs, const iterator& rhs) {
            return !(lhs == rhs);
        }

    private:
        pointer ptr_{nullptr};
    };

    networks() { ::getifaddrs(&list_); }
    constexpr explicit networks(struct ifaddrs *list) : list_(list) {}
    constexpr networks(const networks& from) = delete;
    constexpr auto operator=(const networks& from) -> networks& = delete;
    constexpr networks(networks&& from) noexcept : list_(std::exchange(from.list_, nullptr)) {}
    ~networks() { release(); }

    constexpr explicit operator bool() const noexcept { return !empty(); }
    constexpr auto operator!() const noexcept { return empty(); }

    constexpr auto operator=(networks&& from) noexcept -> networks& {
        if (this->list_ == from.list_) return *this;
        release();
        list_ = std::exchange(from.list_, nullptr);
        return *this;
    }

    constexpr auto operator=(struct ifaddrs *addr) noexcept -> networks& {
        if (list_ != addr) {
            release();
            list_ = addr;
        }
        return *this;
    }

    constexpr auto first() const noexcept -> iface_t {
        return list_;
    }

    constexpr auto front() const noexcept -> iface_t {
        return list_;
    }

    constexpr auto empty() const noexcept -> bool { return list_ == nullptr; }
    constexpr auto begin() const noexcept { return iterator{list_}; }
    constexpr auto end() const noexcept { return iterator{nullptr}; } // NOLINT

    auto find(std::string_view id, int family = AF_UNSPEC, bool multicast = false) const noexcept -> iface_t;
    auto find(const struct sockaddr *from) const noexcept -> iface_t;

protected:
    iface_t list_{nullptr};

    void release() noexcept {
        if (list_) {
            ::freeifaddrs(list_);
            list_ = npos;
        }
    }
};
} // namespace busuto::socket

namespace busuto {
using networks_t = socket::networks;

auto bind_address(const networks_t& nets, const std::string& id, uint16_t port = 0, int family = AF_UNSPEC, bool multicast = false) -> address_t;

auto multicast_index(const networks_t& nets, const std::string& id, int family = AF_UNSPEC) -> unsigned;

} // namespace busuto
#endif
