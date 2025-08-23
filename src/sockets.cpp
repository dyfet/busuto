// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "sockets.hpp"
#include "threads.hpp"

using namespace busuto;

namespace {
constexpr auto is_zero(const void *addr, std::size_t size) -> bool {
    auto ptr = static_cast<const std::byte *>(addr);
    while (size--) {
        if (*ptr != std::byte(0)) return false;
        ++ptr;
    }
    return true;
}
} // end namespace

void socket::address::assign(const struct sockaddr *from) noexcept {
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

void socket::address::port_if(uint16_t value) {
    if (!port())
        port(value);
}

void socket::address::port(uint16_t value) {
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

auto socket::address::from_string(const std::string& s, uint16_t port) -> socket::address {
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

void socket::release(int so) noexcept {
#ifdef _WIN32
    ::shutdown(so, SD_BOTH);
    ::closesocket(so);
#else
    ::shutdown(so, SHUT_RDWR);
    ::close(so);
#endif
}

auto socket::join(int so, const struct sockaddr *member, unsigned ifindex) noexcept -> int {
    if (so < 0) return EBADF;
    auto res = 0;
    multicast_t multicast;
    memset(&multicast, 0, sizeof(multicast));
    switch (member->sa_family) {
    case AF_INET:
        multicast.ipv4.imr_interface.s_addr = INADDR_ANY;
        multicast.ipv4.imr_multiaddr = in4_cast(member)->sin_addr;
        if (setsockopt(so, IPPROTO_IP, IP_ADD_MEMBERSHIP, option_cast(&multicast), sizeof(multicast.ipv4)) == -1)
            res = errno;
        break;
    case AF_INET6:
        multicast.ipv6.ipv6mr_interface = ifindex;
        multicast.ipv6.ipv6mr_multiaddr = in6_cast(member)->sin6_addr;
        if (setsockopt(so, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, option_cast(&multicast), sizeof(multicast.ipv6)) == -1)
            res = errno;
        break;
    default:
        res = EAI_FAMILY;
    }
    return res;
}

auto socket::drop(int so, const struct sockaddr *member, unsigned ifindex) noexcept -> int {
    if (so < 0) return EBADF;
    auto res = 0;
    multicast_t multicast;
    memset(&multicast, 0, sizeof(multicast));
    switch (member->sa_family) {
    case AF_INET:
        multicast.ipv4.imr_interface.s_addr = INADDR_ANY;
        multicast.ipv4.imr_multiaddr = in4_cast(member)->sin_addr;
        if (setsockopt(so, IPPROTO_IP, IP_DROP_MEMBERSHIP, option_cast(&multicast), sizeof(multicast.ipv4)) == -1)
            res = errno;
        break;
    case AF_INET6:
        multicast.ipv6.ipv6mr_interface = ifindex;
        multicast.ipv6.ipv6mr_multiaddr = in6_cast(member)->sin6_addr;
        if (setsockopt(so, IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, option_cast(&multicast), sizeof(multicast.ipv6)) == -1)
            res = errno;
        break;
    default:
        res = EAI_FAMILY;
    }
    return res;
}

auto socket::to_string(const struct sockaddr *sa) -> std::string {
    char buf[INET6_ADDRSTRLEN] = {};
    const uint16_t to_port = port(sa);
    const char *err = nullptr;
    if (sa->sa_family == AF_UNSPEC) return {"*"};
    if (sa->sa_family == AF_INET) {
        auto sin = in4_cast(sa);
        err = inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf));
    } else if (sa->sa_family == AF_INET6) {
        auto sin6 = in6_cast(sa);
        err = inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof(buf));
    }
    if (!err) throw error("unknown or invalid address");
    if (to_port && sa->sa_family == AF_INET6)
        return "[" + std::string(buf) + "]:" + std::to_string(to_port);
    if (to_port && sa->sa_family == AF_INET)
        return std::string(buf) + ":" + std::to_string(to_port);
    return {buf};
}

auto socket::is_any(const struct sockaddr *sa) -> bool {
    switch (sa->sa_family) {
    case AF_UNSPEC:
        return true;
    case AF_INET:
        return is_zero(&(in4_cast(sa))->sin_addr.s_addr, 4);
    case AF_INET6:
        return is_zero(&(in6_cast(sa))->sin6_addr.s6_addr, 16);
    default:
        return false;
    }
}
