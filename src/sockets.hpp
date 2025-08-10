// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "streams.hpp"

#include <string>
#include <string_view>
#include <cstring>
#include <ostream>
#include <memory>
#include <utility>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <poll.h>
#include <ifaddrs.h>

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IP_ADD_MEMBERSHIP
#endif

#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP IP_DROP_MEMBERSHIP
#endif

namespace busuto::socket {
using storage_t = struct sockaddr_storage;
using multicast_t = union {
    struct ip_mreq ipv4;
    struct ipv6_mreq ipv6;
};

inline constexpr socklen_t maxlen = sizeof(sockaddr_storage);

template <typename T>
struct is_sockaddr : std::false_type {};

template <>
struct is_sockaddr<sockaddr> : std::true_type {};

#ifdef AF_UNIX
template <>
struct is_sockaddr<sockaddr_un> : std::true_type {};
#endif

template <>
struct is_sockaddr<sockaddr_in> : std::true_type {};

template <>
struct is_sockaddr<sockaddr_in6> : std::true_type {};

template <>
struct is_sockaddr<sockaddr_storage> : std::true_type {};

template <typename T>
inline constexpr bool is_sockaddr_v =
busuto::socket::is_sockaddr<std::remove_cv_t<std::remove_pointer_t<T>>>::value;

template <typename T>
inline auto option_cast(T from) noexcept {
    static_assert(std::is_pointer_v<T>, "Option must be pointer");
    if constexpr (std::is_const_v<std::remove_pointer_t<T>>) {
        return reinterpret_cast<const char *>(from);
    } else {
        return reinterpret_cast<char *>(from);
    }
}

template <typename T>
inline auto in4_cast(T *addr) noexcept {
    static_assert(is_sockaddr_v<T>, "Not sockaddr castable");
    if constexpr (std::is_const_v<T>) {
        return addr->sa_family == AF_INET || addr->sa_family == AF_UNSPEC
               ? reinterpret_cast<const struct sockaddr_in *>(addr)
               : nullptr;
    } else {
        return addr->sa_family == AF_INET || addr->sa_family == AF_UNSPEC
               ? reinterpret_cast<struct sockaddr_in *>(addr)
               : nullptr;
    }
}

template <typename T>
inline auto in6_cast(T *addr) noexcept {
    static_assert(is_sockaddr_v<T>, "Not sockaddr castable");
    if constexpr (std::is_const_v<T>) {
        return addr->sa_family == AF_INET6 || addr->sa_family == AF_UNSPEC
               ? reinterpret_cast<const struct sockaddr_in6 *>(addr)
               : nullptr;
    } else {
        return addr->sa_family == AF_INET6 || addr->sa_family == AF_UNSPEC
               ? reinterpret_cast<struct sockaddr_in6 *>(addr)
               : nullptr;
    }
}

inline auto to_storage(struct sockaddr *addr) noexcept {
    return reinterpret_cast<struct sockaddr_storage *>(addr);
}

inline auto to_sockaddr(struct sockaddr_storage *addr) noexcept {
    return reinterpret_cast<struct sockaddr *>(addr);
}

inline auto to_sockaddr(const struct sockaddr_storage *addr) noexcept {
    return reinterpret_cast<const struct sockaddr *>(addr);
}

#ifdef AF_UNIX
template <typename T>
inline auto un_cast(T *addr) noexcept {
    static_assert(is_sockaddr_v<T>, "Not sockaddr castable");
    if constexpr (std::is_const_v<T>) {
        return addr->sa_family == AF_UNIX
               ? reinterpret_cast<const struct sockaddr_un *>(addr)
               : nullptr;
    } else {
        return addr->sa_family == AF_UNIX
               ? reinterpret_cast<struct sockaddr_un *>(addr)
               : nullptr;
    }
}
#endif

class address {
public:
    constexpr address() noexcept = default;

    explicit address(const storage_t& src) noexcept {
        std::memcpy(&storage, &src, sizeof(storage_t));
    }

    // cppcheck-suppress noExplicitConstructor
    address(const struct sockaddr *from) noexcept {
        assign(from);
    }

    explicit address(const struct addrinfo *from) noexcept {
        if (from && from->ai_addr && from->ai_addrlen)
            ::memcpy(&storage, from->ai_addr, from->ai_addrlen);
    }

    operator const struct sockaddr *() const noexcept {
        return to_sockaddr(&storage);
    }

    operator struct sockaddr *() noexcept {
        return to_sockaddr(&storage);
    }

    explicit operator bool() const noexcept {
        return is_valid();
    }

    auto operator!() const noexcept {
        return !is_valid();
    }

    auto operator*() const noexcept {
        return to_sockaddr(&storage);
    }

    auto operator*() noexcept {
        return to_sockaddr(&storage);
    }

    auto operator=(const struct sockaddr *from) noexcept -> address& {
        assign(from);
        return *this;
    }

    auto operator=(const struct addrinfo *from) noexcept -> address& {
        if (from && from->ai_addr && from->ai_addrlen)
            ::memcpy(&storage, from->ai_addr, from->ai_addrlen);
        return *this;
    }

    auto operator=(const address& other) noexcept -> address& {
        if (this != &other)
            std::memcpy(&storage, &other.storage, sizeof(storage_t));
        return *this;
    }

    constexpr auto is_valid() const noexcept -> bool {
        if (family() == AF_UNSPEC) return false;
        if (((family() == AF_INET) || (family() == AF_INET6)) && !port()) return false;
        return true;
    }

    auto is_any() const noexcept {
        auto sa = to_sockaddr(&storage);
        switch (sa->sa_family) {
        case AF_UNSPEC:
            return true;
        case AF_INET:
            return zero(&(in4_cast(sa))->sin_addr.s_addr, 4);
        case AF_INET6:
            return zero(&(in6_cast(sa))->sin6_addr.s6_addr, 16);
        default:
            return false;
        }
    }

    void assign(const struct sockaddr *from) noexcept {
        if (!from) {
            memset(&storage, 0, sizeof(storage));
            return;
        }
        socklen_t len = 0;
        switch (from->sa_family) {
        case AF_INET:
            len = sizeof(sockaddr_in);
            break;
        case AF_INET6:
            len = sizeof(sockaddr_in6);
            break;
        case AF_UNIX:
            len = sizeof(sockaddr_un);
            break;
        default:
            len = std::size_t(0);
            break;
        }

        if (len && len <= sizeof(storage)) {
            std::memcpy(&storage, from, len);
        }
    }

    auto c_sockaddr() const noexcept {
        return reinterpret_cast<const struct sockaddr *>(&storage);
    }

    auto data() const noexcept {
        return reinterpret_cast<const struct sockaddr *>(&storage);
    }

    auto data() noexcept {
        return reinterpret_cast<struct sockaddr *>(&storage);
    }

    constexpr auto size() const noexcept -> socklen_t {
        switch (storage.ss_family) {
        case AF_INET:
            return sizeof(sockaddr_in);
            break;
        case AF_INET6:
            return sizeof(sockaddr_in6);
            break;
        case AF_UNIX:
            return sizeof(sockaddr_un);
            break;
        default:
            return std::size_t(0);
            break;
        }
    }

    constexpr auto family() const noexcept -> int {
        return storage.ss_family;
    }

    void family_if(int changed) noexcept {
        if (family() == AF_UNSPEC)
            storage.ss_family = changed;
    }

    auto port() const noexcept -> uint16_t {
        auto sa = to_sockaddr(&storage);
        switch (storage.ss_family) {
        case AF_INET:
            return ntohs(in4_cast(sa)->sin_port);
        case AF_INET6:
            return ntohs(in6_cast(sa)->sin6_port);
        default:
            return 0;
        }
    }

    void port_if(uint16_t value) {
        if (!port())
            port(value);
    }

    void port(uint16_t value) {
        auto sa = to_sockaddr(&storage);
        switch (storage.ss_family) {
        case AF_INET:
            (in4_cast(sa))->sin_port = htons(value);
            return;
        case AF_INET6:
            (in6_cast(sa))->sin6_port = htons(value);
            return;
        default:
            throw error("unknown address type");
        }
    }

    auto to_string() const -> std::string;

    operator std::string() const { return to_string(); }

    auto operator==(const address& other) const noexcept {
        auto len = size();
        if (len != other.size()) return false;
        return memcmp(&storage, &other.storage, len) == 0;
    }

    auto operator!=(const address& other) const noexcept {
        auto len = size();
        if (len != other.size()) return false;
        return memcmp(&storage, &other.storage, len) != 0;
    }

    static auto from_string(const std::string& s, uint16_t port = 0) {
        address a;
        auto a4 = in4_cast(*a);
        auto a6 = in6_cast(*a);
        if (s == "*") {
            a.storage.ss_family = AF_INET;
            a.port(port);
            return a;
        }

        if (s == "[*]") {
            a.storage.ss_family = AF_INET6;
            a.port(port);
            return a;
        }

        if (s.find(':') != std::string::npos) {
            if (inet_pton(AF_INET6, s.c_str(), &a6->sin6_addr) == 1) {
                a6->sin6_family = AF_INET6;
                a6->sin6_port = htons(port);
                return a;
            }
        } else {
            if (inet_pton(AF_INET, s.c_str(), &a4->sin_addr) == 1) {
                a4->sin_family = AF_INET;
                a4->sin_port = htons(port);
                return a;
            }
        }
        throw error("invalid address format: " + s);
    }

protected:
    struct sockaddr_storage storage{};

    constexpr static auto zero(const void *addr, std::size_t size) -> bool {
        auto ptr = static_cast<const std::byte *>(addr);
        while (size--) {
            if (*ptr != std::byte(0)) return false;
            ++ptr;
        }
        return true;
    }
};

inline auto operator<<(std::ostream& out, const address& addr) -> std::ostream& {
    out << addr.to_string();
    return out;
}

void release(int so) noexcept;
auto join(int so, const struct sockaddr *member, unsigned ifindex = 0) noexcept -> int;
auto drop(int so, const struct sockaddr *member, unsigned ifindex = 0) noexcept -> int;

inline auto make_socket(int family, int type = SOCK_STREAM, int protocol = 0) {
    return handle_t(::socket(family, type, protocol), &release);
}

template <std::size_t S = 576>
inline auto make_stream(int so) {
    return system_stream(so, &release);
}
} // namespace busuto::socket

namespace std {
template <>
struct hash<busuto::socket::address> {
    auto operator()(const busuto::socket::address& a) const noexcept {
        const auto *bytes = reinterpret_cast<const std::byte *>(a.data());
        return std::hash<std::string_view>{}(std::string_view(
        reinterpret_cast<const char *>(bytes),
        a.size()));
    }
};
} // namespace std
