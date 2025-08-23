// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "networks.hpp"
#include "resolver.hpp"
#include "service.hpp"
#include "print.hpp"
#include "args.hpp"
#include <cassert>

using namespace busuto;

namespace {
socket::address a;

void test_socket_any() {
    assert(a.is_any());
    a = socket::address::from_string("127.0.0.1");
    assert(!a.is_any());
}

void test_socket_addr() {
    struct sockaddr *sa = a;
    assert(sa->sa_family == AF_INET);
    socket::address b(sa);
    assert(!b.is_any());
    assert(socket::in4_cast(sa)->sin_port == 0);
    assert(b.family() == AF_INET);
    b.port_if(2);
    assert(b.port() == 2);
    assert(b.to_string() == "127.0.0.1:2");
}

void test_socket_bind() {
#ifndef _WIN32
    const networks_t nets;
    assert(!nets.empty());
    auto b1 = bind_address(nets, "*");
    assert(!is(b1));
    b1 = bind_address(nets, "[*]", 5060);
    assert(is(b1));
    b1 = bind_address(nets, "127.0.0.1", 5060);
#ifndef __FreeBSD__
    assert(std::format("{}", b1) == std::string("127.0.0.1:5060"));
    fsys::path path = "/hello";
    assert(std::format("{}", path) == std::string("/hello"));
#endif
    assert(is(b1));
#endif
}

void test_socket_resolver() {
    auto list = socket::lookup(socket::from_host("localhost"), AF_INET);
    assert(list.front() != nullptr);
    auto fut = async_resolver({"localhost", ""}, AF_INET);
    list = fut.get();
    assert(list.front() != nullptr);
    const socket::address addr = static_cast<struct sockaddr *>(list);
    assert(addr.to_string() == "127.0.0.1");
    assert(addr.port() == 0);
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_socket_any();
        test_socket_addr();
        test_socket_bind();
        test_socket_resolver();
    } catch (const std::exception& e) {
        output::exit(-1) << "ERR " << e.what();
    }
    output::null() << "test\n";
    debug("any error {}", "here");
    print(output(), "done");
}
