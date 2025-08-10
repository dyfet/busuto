// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "sockets.hpp"
#include "threads.hpp"

using namespace busuto;

auto socket::address::to_string() const -> std::string {
    auto sa = to_sockaddr(&storage);
    char buf[INET6_ADDRSTRLEN] = {};
    const uint16_t to_port = port();
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

void socket::release(int so) noexcept {
    ::shutdown(so, SHUT_RDWR);
    ::close(so);
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
