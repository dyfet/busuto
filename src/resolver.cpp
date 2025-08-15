// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "resolver.hpp"

#include <semaphore>
#include <chrono>

using namespace busuto;

std::counting_semaphore<BUSUTO_RESOLVER_COUNT> busuto::resolvers(BUSUTO_RESOLVER_COUNT);

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
