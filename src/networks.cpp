// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#ifndef _WIN32
#include "networks.hpp"

#include <net/if.h>

using namespace busuto;

auto socket::networks::find(std::string_view id, int family, bool multicast) const noexcept -> iface_t {
    for (auto entry = list_; entry != nullptr; entry = entry->ifa_next) {
        if (multicast && !(entry->ifa_flags & IFF_MULTICAST)) continue;
        if (!entry->ifa_addr || !entry->ifa_name) continue;

        auto ifa_family = int(entry->ifa_addr->sa_family);
        if (family == AF_UNSPEC && (ifa_family == AF_INET || ifa_family == AF_INET6)) ifa_family = AF_UNSPEC;
        if (id == entry->ifa_name && family == ifa_family) return entry;
    }
    return nullptr;
}

auto socket::networks::find(const struct sockaddr *from) const noexcept -> iface_t {
    using namespace busuto::socket;
    if (!from || !list_) return nullptr;
    for (auto entry = list_; entry != nullptr; entry = entry->ifa_next) {
        auto family = entry->ifa_addr->sa_family;
        if (!entry->ifa_addr || !entry->ifa_netmask) continue;
        if (family != from->sa_family) continue;
        if (family == AF_INET) {
            auto *t = in4_cast(from);
            auto *a = in4_cast(entry->ifa_addr);
            auto *m = in4_cast(entry->ifa_netmask);
            if ((a->sin_addr.s_addr & m->sin_addr.s_addr) == (t->sin_addr.s_addr & m->sin_addr.s_addr)) return entry;
        } else if (family == AF_INET6) {
            auto *a6 = in6_cast(entry->ifa_addr);
            auto *m6 = in6_cast(entry->ifa_netmask);
            auto *t6 = in6_cast(from);
            auto match = true;
            for (int i = 0; i < 16; ++i) {
                if ((a6->sin6_addr.s6_addr[i] & m6->sin6_addr.s6_addr[i]) != (t6->sin6_addr.s6_addr[i] & m6->sin6_addr.s6_addr[i])) {
                    match = false;
                    break;
                }
            }
            if (match) return entry;
        }
    }
    return nullptr;
}

auto busuto::bind_address(const networks_t& nets, const std::string& id, uint16_t port, int family, bool multicast) -> address_t {
    address_t any;
    if (id == "[*]" && family == AF_UNSPEC) {
        any.family_if(AF_INET6);
        any.port(port);
        return any;
    }
    if (id == "*") {
        if (family == AF_UNSPEC)
            family = AF_INET;
        any.family_if(family);
        any.port(port);
        return any;
    }
    if ((family == AF_INET || family == AF_UNSPEC) && id.find('.') != std::string_view::npos)
        return socket::address::from_string(id, port);
    if ((family == AF_INET6 || family == AF_UNSPEC) && id.find(':') != std::string_view::npos)
        return socket::address::from_string(id, port);
    auto ifa = nets.find(id, family, multicast);
    if (ifa && ifa->ifa_addr) {
        socket::address a(ifa->ifa_addr);
        a.port(port);
        return a;
    }
    return any;
}

auto busuto::multicast_index(const networks_t& nets, const std::string& id, int family) -> unsigned {
    if (id == "*" && (family == AF_INET || family == AF_UNSPEC)) return ~0U;
    auto ifa = nets.find(id, family, true);
    if (ifa && ifa->ifa_addr) {
        if (ifa->ifa_addr->sa_family == AF_INET) return ~0U;
        return if_nametoindex(ifa->ifa_name);
    }
    return 0U;
}
#endif
