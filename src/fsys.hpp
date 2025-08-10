// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "system.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdio>

#if defined(__OpenBSD__)
#define stat64 stat   // NOLINT
#define fstat64 fstat // NOLINT
#endif

namespace busuto::fsys {
using namespace std::filesystem;

template <typename F>
concept directory_predicate =
std::invocable<F, const fsys::directory_entry&> &&
std::convertible_to<std::invoke_result_t<F, const fsys::directory_entry&>, bool>;

template <typename F>
concept file_predicate =
std::invocable<F, std::string_view> &&
std::convertible_to<std::invoke_result_t<F, std::string_view>, bool>;

class file_t final {
public:
    explicit file_t(FILE *file) : file_(system::file_ptr(file)) {}
    explicit file_t(system::file_ptr&& file) : file_(std::move(file)) {}

    operator FILE *() const noexcept { return file_.get(); }
    explicit operator bool() const noexcept { return is_open(); }
    auto operator!() const noexcept { return !is_open(); }
    auto is_open() const noexcept -> bool { return file_.get() != nullptr; }

private:
    system::file_ptr file_{nullptr};
};

class pipe_t final {
public:
    explicit pipe_t(system::pipe_ptr&& pipe) : pipe_(std::move(pipe)) {}

    operator FILE *() const noexcept { return pipe_.get(); }
    explicit operator bool() const noexcept { return is_open(); }
    auto operator!() const noexcept { return !is_open(); }
    auto is_open() const noexcept -> bool { return pipe_.get() != nullptr; }

    auto join() noexcept -> int {
        auto fp = pipe_.release();
        if (!fp) return -1;
        return pclose(fp);
    }

private:
    system::pipe_ptr pipe_{nullptr};
};

inline auto make_file(const std::string& path, const std::string& mode = "rb") {
    return file_t(system::file_ptr(fopen(path.c_str(), mode.c_str())));
}

inline auto make_pipe(const std::string& cmd, const std::string& mode = "r") {
    return pipe_t(system::make_pipe(cmd, mode));
}
} // namespace busuto::fsys

namespace busuto {
template <fsys::file_predicate Func>
inline auto scan_stream(std::istream& input, Func func) {
    std::string line;
    std::size_t count{0};
    while (std::getline(input, line)) {
        if (!func(line)) break;
        ++count;
    }
    return count;
}

template <fsys::file_predicate Func>
inline auto scan_file(const fsys::path& path, Func func) {
    std::fstream input(path);
    std::string line;
    std::size_t count{0};
    while (std::getline(input, line)) {
        if (!func(line)) break;
        ++count;
    }
    return count;
}

template <fsys::file_predicate Func>
inline auto scan_command(const std::string& cmd, Func func, std::size_t size = 0) {
    auto pipe = busuto::system::make_pipe(cmd, "r");
    if (!pipe.get()) return std::size_t(0);

    auto count = scan_file(pipe.get(), func, size);
    return count;
}

template <fsys::directory_predicate Func>
inline auto scan_directory(const fsys::path& path, Func func) {
    std::error_code ec;
    const fsys::directory_iterator dir(path, ec);
    if (ec) return 0;
    return std::count_if(begin(dir), end(dir), func);
}

template <fsys::directory_predicate Func>
inline auto scan_recursive(const fsys::path& path, Func func) {
    std::error_code ec;
    const fsys::recursive_directory_iterator dir(path, ec);
    if (ec) return 0;
    return std::count_if(begin(dir), end(dir), func);
}
} // namespace busuto
