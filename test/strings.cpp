// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#undef NDEBUG
#include "strings.hpp"
#include "keyfile.hpp"
#include "print.hpp"
#include <cassert>

using namespace busuto::strings;
using namespace busuto;

namespace {
void test_case_conversions() {
    assert(to_lower("hi There") == "hi there");
    assert(starts_case(std::string("Belong"), "be"));
    assert(ends_case("beloNg", "ong"));
    assert(!starts_case("belong", "tr"));
}

void test_string_trim() {
    auto trimmed = trim(std::string("last text \n"));
    assert(trimmed == "last text");
    static_assert(trim("hello stuff \t\n") == "hello stuff");
    std::string test = "\t\tsome space   ";
    test = strip(test);
    assert(unquote(test) == "some space");
}

auto test_string_unquote() {
    static_assert(unquote("'able '") == "able ");
    static_assert(unquote("'able ") == "'able ");
    assert(unquote(std::string("'able ")) == "'able ");
    assert(unquote(std::string("'able '")) == "able ");

    const char *qt = "'hello '";
    assert(unquote(qt) == "hello ");
}

auto test_string_split() {
    const std::string text = "hi,bye,gone";
    auto list = split(text, ",");
    assert(list.size() == 3);
    assert(list[0] == "hi");
    assert(list[1] == "bye");
    assert(list[2] == "gone");

    const std::string_view text2 = "hello:bye";
    const auto list2 = split(text2, ":");
    assert(list2.size() == 2);
    static_assert(!is_string_vector_v<const char *>, "Test raw vector pointer");
    static_assert(is_string_vector_v<std::string>, "Test string vector object");
    static_assert(is_string_vector_v<std::string_view>, "Verify string view");
}

auto test_string_is_expressions() {
    static_assert(!is_quoted(";able'"));
    assert(is_quoted(std::string("'able'")));
    char qt[] = {'{', 'a', '}', 0};
    qt[1] = 'b';
    assert(is_quoted(qt));

    static_assert(!is_unsigned("23e"));
    static_assert(is_unsigned("246"));
    assert(is_unsigned(std::string("246")) == true);
    assert(is_integer(std::string("-246")) == true);
}

auto test_string_tokenizer() {
    const std::string cmd = "this is a ' command group ' line ";
    auto args = strings::tokenize(cmd);
    assert(args.size() == 5);
    assert(args[3] == "' command group '");
    assert(args[4] == "line");
}
} // end namespace

auto main(int /* argc */, char ** /* argv */) -> int {
    try {
        test_case_conversions();
        test_string_trim();
        test_string_unquote();
        test_string_split();
        test_string_is_expressions();
        test_string_tokenizer();
    } catch (...) {
        return -1;
    }
    return 0;
}
