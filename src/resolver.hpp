// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "sockets.hpp"
#include "sync.hpp"

#include <semaphore>
#include <future>

#ifndef BUSUTO_RESOLVER_COUNT
#define BUSUTO_RESOLVER_COUNT 8 // NOLINT
#endif

namespace busuto::socket {
using name_t = std::pair<std::string, std::string>;
using addr_t = std::pair<const struct sockaddr *, socklen_t>;

class service {
public:
    using reference = struct addrinfo&;
    using pointer = struct addrinfo *;
    using const_reference = const struct addrinfo&;
    using size_type = std::size_t;
    using value_type = struct addrinfo;

    constexpr static struct addrinfo *npos = nullptr;

    class iterator {
    public:
        using value_type = struct addrinfo;
        using pointer = addrinfo *;
        using reference = addrinfo&;
        using difference_type = std::ptrdiff_t;
        using iterator_concept = std::forward_iterator_tag;

        constexpr iterator() = default;
        constexpr explicit iterator(pointer ptr) : ptr_(ptr) {}

        constexpr auto operator*() const -> reference { return *ptr_; }
        constexpr auto operator->() const -> pointer { return ptr_; }

        constexpr auto operator++() -> iterator& {
            ptr_ = ptr_ ? ptr_->ai_next : nullptr;
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

    constexpr service() = default;
    constexpr explicit service(struct addrinfo *list) : list_(list) {}
    constexpr service(const service& from) = delete;
    constexpr auto operator=(const service& from) -> service& = delete;
    constexpr service(service&& from) noexcept : list_(std::exchange(from.list_, nullptr)) {}
    ~service() { release(); }

    constexpr operator struct sockaddr *() noexcept {
        if (!list_) return nullptr;
        return list_->ai_addr;
    }

    constexpr operator const struct sockaddr *() noexcept {
        if (!list_) return nullptr;
        return list_->ai_addr;
    }

    constexpr operator addr_t() const noexcept { return addr(); }
    constexpr operator const struct addrinfo *() const noexcept { return list_; }
    constexpr explicit operator bool() const noexcept { return !empty(); }
    constexpr auto operator*() const noexcept { return list_; }
    constexpr auto operator!() const noexcept { return empty(); }

    constexpr auto operator=(service&& from) noexcept -> service& {
        if (this->list_ == from.list_) return *this;
        release();
        list_ = std::exchange(from.list_, nullptr);
        return *this;
    }

    constexpr auto operator=(struct addrinfo *addr) noexcept -> service& {
        if (list_ != addr) {
            release();
            list_ = addr;
        }
        return *this;
    }

    constexpr auto first() const noexcept -> const struct addrinfo * {
        return list_;
    }

    constexpr auto front() const noexcept -> const struct addrinfo * {
        return list_;
    }

    constexpr auto addr() const noexcept -> addr_t {
        if (!list_) return {};
        return {list_->ai_addr, list_->ai_addrlen};
    }

    constexpr auto c_sockaddr() const noexcept -> const struct sockaddr * {
        if (!list_) return nullptr;
        return list_->ai_addr;
    }

    constexpr auto empty() const noexcept -> bool { return list_ == nullptr; }
    constexpr auto begin() const noexcept { return iterator{list_}; }
    constexpr auto end() const noexcept { return iterator{nullptr}; } // NOLINT

    template <typename Func>
    requires std::invocable<Func, addrinfo *> && std::convertible_to<std::invoke_result_t<Func, addrinfo *>, bool>
    auto count(Func func) const {
        auto visited = 0U;
        auto list = list_;
        while (list) {
            if (func(list))
                ++visited;
            list = list->ai_next;
        }
        return visited;
    }

    template <typename Func>
    requires std::invocable<Func, addrinfo *> && std::convertible_to<std::invoke_result_t<Func, addrinfo *>, bool>
    auto find(Func func) const {
        auto list = list_;
        while (list) {
            if (func(list))
                return list;
            list = list->ai_next;
        }
        return npos;
    }

protected:
    struct addrinfo *list_{npos};

    void release() noexcept {
        if (list_) {
            ::freeaddrinfo(list_);
            list_ = npos;
        }
    }
};

auto lookup(const name_t& name = name_t("*", ""), int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) -> socket::service;
auto lookup(const addr_t& info, int flags = 0) -> name_t;

inline auto lookup(const address& addr, int flags = 0) -> name_t {
    return lookup(addr_t(addr.data(), addr.size()), flags);
}

inline auto from_addr(const address& addr) {
    return addr_t(addr.data(), addr.size());
}

inline auto from_host(const std::string& host) {
    return name_t(host, "");
}
} // namespace busuto::socket

namespace busuto {
extern std::counting_semaphore<BUSUTO_RESOLVER_COUNT> resolvers;

class resolver_timeout final : public std::runtime_error {
public:
    resolver_timeout() noexcept : std::runtime_error("resolver timeout") {}
};

inline auto async_resolver(const socket::name_t& name, int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0, int timeout = -1) -> std::future<socket::service> {
    if (timeout < 0)
        resolvers.acquire();
    else if (!timeout) {
        if (!resolvers.try_acquire())
            throw resolver_timeout();
    } else if (!resolvers.try_acquire_for(std::chrono::milliseconds(timeout)))
        throw resolver_timeout();

    return std::async(std::launch::async, [name, family, type, protocol] {
        const sync::semaphore_scope scope(resolvers);
        return socket::lookup(name, family, type, protocol);
    });
}

inline auto async_resolver(const socket::addr_t& info, int flags = 0, int timeout = -1) -> std::future<socket::name_t> {
    if (timeout < 0)
        resolvers.acquire();
    else if (!timeout) {
        if (!resolvers.try_acquire())
            throw resolver_timeout();
    } else if (!resolvers.try_acquire_for(std::chrono::milliseconds(timeout)))
        throw resolver_timeout();

    return std::async(std::launch::async, [info, flags] {
        const sync::semaphore_scope scope(resolvers);
        return socket::lookup(info, flags);
    });
}

inline auto defer_resolver(const socket::name_t& name = socket::name_t("*", ""), int family = AF_UNSPEC, int type = SOCK_STREAM, int protocol = 0) -> std::future<socket::service> {
    return std::async(std::launch::deferred, [name, family, type, protocol] {
        return socket::lookup(name, family, type, protocol);
    });
}

inline auto defer_resolver(const socket::addr_t& info, int flags = 0) {
    return std::async(std::launch::deferred, [info, flags] {
        return socket::lookup(info, flags);
    });
}
} // namespace busuto
