// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "resolver.hpp"
#include "threads.hpp"
#include "sync.hpp"

#include <semaphore>
#include <chrono>

#include <net/if.h>
#include <ifaddrs.h>

using namespace busuto;

namespace {
struct ifaddrs *ifs{nullptr};

void reload_ifs() {
    ifaddrs *update{nullptr};
    getifaddrs(&update);
    if (update) {
        if (ifs)
            freeifaddrs(ifs);
        ifs = update;
    }
}

auto ifa_address(std::string_view id, unsigned family = AF_UNSPEC, bool mcast = false) -> const struct ifaddrs * {
    if (!ifs) return nullptr;
    for (auto entry = ifs; entry != nullptr; entry = entry->ifa_next) {
        if (mcast && !(entry->ifa_flags & IFF_MULTICAST)) continue;
        if (!entry->ifa_addr || !entry->ifa_name) continue;

        auto ifa_family = entry->ifa_addr->sa_family;
        if (family == AF_UNSPEC && (ifa_family == AF_INET || ifa_family == AF_INET6)) ifa_family = AF_UNSPEC;
        if (id == entry->ifa_name && family == ifa_family) return entry;
    }
    return nullptr;
}

auto ifa_subnet(const struct sockaddr *from) -> const struct ifaddrs * {
    using namespace busuto::socket;
    if (!from || !ifs) return nullptr;
    for (auto entry = ifs; entry != nullptr; entry = entry->ifa_next) {
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

std::shared_mutex ifs_lock;
std::atomic<bool> ifs_refreshed{false};
} // end namespace

std::counting_semaphore<BUSUTO_RESOLVER_COUNT> busuto::resolvers(BUSUTO_RESOLVER_COUNT);

void busuto::refresh_networks() noexcept {
    const std::lock_guard lock(ifs_lock);
    reload_ifs();
    ifs_refreshed = true;
}

auto busuto::multicast_to(const struct sockaddr *addr) -> unsigned {
    if (!addr) return 0U;
    if (!ifs_refreshed.exchange(true)) refresh_networks();
    const std::shared_lock lock(ifs_lock);
    if (ifs) {
        auto ifa = ifa_subnet(addr);
        if (!ifa) return 0U;
        if (!(ifa->ifa_flags & IFF_MULTICAST)) return 0U;
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) return ~0U;
        if (ifa && ifa->ifa_name) return if_nametoindex(ifa->ifa_name);
    }
    return ~0U;
}

auto busuto::network_to(const struct sockaddr *addr) -> std::string {
    if (!addr) return "none";
    if (!ifs_refreshed.exchange(true)) refresh_networks();
    const std::shared_lock lock(ifs_lock);
    if (ifs) {
        auto ifa = ifa_subnet(addr);
        if (ifa && ifa->ifa_name) return ifa->ifa_name;
    }
    return "default";
}

auto busuto::address_to(const struct sockaddr *addr) -> socket::address {
    socket::address any;
    if (!addr) return any;
    if (!ifs_refreshed.exchange(true)) refresh_networks();
    const std::shared_lock lock(ifs_lock);
    if (ifs) {
        auto ifa = ifa_subnet(addr);
        if (ifa && ifa->ifa_addr) any = ifa->ifa_addr;
    }
    const socket::address remote = addr;
    if (remote.family() != AF_UNSPEC)
        any.family_if(remote.family());
    any.port_if(remote.port());
    return any;
}

auto busuto::bind_multicast(const std::string& id, int family) -> unsigned {
    if (id == "*" && (family == AF_INET || family == AF_UNSPEC)) return ~0U;
    if (!ifs_refreshed.exchange(true)) refresh_networks();
    const std::shared_lock lock(ifs_lock);
    if (ifs) {
        auto ifa = ifa_address(id, family, true);
        if (ifa && ifa->ifa_addr) {
            if (ifa->ifa_addr->sa_family == AF_INET) return ~0U;
            return if_nametoindex(ifa->ifa_name);
        }
    }
    return 0U;
}

auto busuto::bind_address(const std::string& id, uint16_t port, int family, bool multicast) -> socket::address {
    socket::address any;
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
    if (!ifs_refreshed.exchange(true)) refresh_networks();
    const std::shared_lock lock(ifs_lock);
    if (ifs) {
        auto ifa = ifa_address(id, family, multicast);
        if (ifa && ifa->ifa_addr) {
            socket::address a(ifa->ifa_addr);
            a.port(port);
            return a;
        }
    }
    return any;
}

auto socket::lookup(const addr_t& info, int flags) -> socket::name_t {
    const auto& [addr, len] = info;
    if (!addr) return {"", ""};
    char hostbuf[256];
    char portbuf[64];
    if (!getnameinfo(addr, len, hostbuf, sizeof(hostbuf), portbuf, sizeof(portbuf), flags)) {
        return {hostbuf, portbuf};
    }
    return {"", ""};
}

auto socket::lookup(const socket::name_t& name, int family, int type, int protocol) -> socket::service {
    const auto& [host, service] = name;
    struct addrinfo *list{nullptr};
    auto svc = service.c_str();
    if (service.empty() || service == "0")
        svc = nullptr;

    struct addrinfo hint{};
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = family;
    hint.ai_socktype = type;
    hint.ai_protocol = protocol;

    auto port{0};
    try {
        if (svc) port = std::stoi(service);
    } catch (...) {
        port = 0;
    }

    if (port > 0 && port < 65536)
        hint.ai_flags |= NI_NUMERICSERV;

    auto addr = host.c_str();
    if (host.empty() || host == "*")
        addr = nullptr;
    else if (host == "[*]") {
        addr = nullptr;
        hint.ai_family = AF_INET6;
    } else if (strchr(addr, ':'))
        hint.ai_flags |= AI_NUMERICHOST;

    if (protocol)
        hint.ai_flags |= AI_PASSIVE;

    auto err = getaddrinfo(addr, svc, &hint, &list);
    if (err && list) {
        freeaddrinfo(list);
        list = nullptr;
    }
    return socket::service(list);
}
