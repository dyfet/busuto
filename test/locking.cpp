// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "locking.hpp"
#include "print.hpp"
#include <cassert>

using namespace busuto;

struct test {
    int v1{2};
    // int v2{7};
};

namespace {
lock::exclusive<std::unordered_map<std::string, std::string>> mapper;
lock::exclusive<int> counter(3);
lock::shared<std::unordered_map<std::string, std::string>> tshared;
lock::shared<struct test> testing;
lock::shared<std::array<int, 10>> tarray;
} // namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        {
            lock::exclusive_ptr map(mapper);
            assert(map->empty());
            map["here"] = "there";
            assert(map->size() == 1);
            assert(map["here"] == "there");
        }
        {
            lock::writer_ptr map(tshared);
            map["here"] = "there";
        }
        {
            const lock::reader_ptr map(tshared);
            assert(map.at("here") == "there");
        }
        {
            lock::writer_ptr map(tarray);
            map[2] = 17;
        }
        {
            const lock::reader_ptr map(tarray);
            assert(map[2] == 17);
        }

        lock::exclusive_ptr count(counter);
        assert(*count == 3);
        ++*count;
        assert(*count == 4);
        count.unlock();

        lock::exclusive_guard fixed(counter);
        assert(*fixed == 4);
        {
            lock::writer_ptr modtest(testing);
            ++modtest->v1;
        }
        const lock::reader_ptr<struct test> tester(testing);
        assert(tester->v1 == 3);

    } catch (std::exception& e) {
        print("ERR: {}\n", e.what());
        return -1;
    }
    return 0;
}
