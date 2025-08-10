// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#define BUSUTO_CRYPTO_SODIUM

#undef NDEBUG
#include "hash.hpp"
#include <cassert>

using namespace busuto;

namespace {
void test_random_sodium() {
    byte_array b1(20);
    byte_array b2(20);
    crypto::random_context rng;
    rng.fill(b1);
    rng.fill(b2);
    assert(b1 != b2);
}

void test_hash_ring() {
    crypto::ring64<> ring;
    assert(ring.insert("nodeA"));
    /*    assert(ring.insert("nodeB"));
        assert(ring.insert("nodeC"));
        assert(ring.size() == 3);
        assert(ring.usage() > 200);

        const std::string key = "user:67";
        assert(ring.get(key) == "nodeC");

        assert(ring.remove("nodeB"));
        assert(!ring.remove("nodeD"));
        assert(ring.usage() <= 200);
    */
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        crypto::startup();
        test_random_sodium();
        test_hash_ring();
    } catch (...) {
        return -1;
    }
    return 0;
}
