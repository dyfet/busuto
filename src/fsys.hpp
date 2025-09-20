// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "system.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <dirent.h>

#if defined(__OpenBSD__)
#define stat64 stat   // NOLINT
#define fstat64 fstat // NOLINT
#endif

namespace busuto::fsys {
using namespace std::filesystem;
using dirent_t = struct dirent *;

template <typename F>
concept directory_predicate =
std::invocable<F, const fsys::directory_entry&> &&
std::convertible_to<std::invoke_result_t<F, const fsys::directory_entry&>, bool>;

template <typename F>
concept file_predicate =
std::invocable<F, std::string_view> &&
std::convertible_to<std::invoke_result_t<F, std::string_view>, bool>;

template <typename F>
concept prefix_predicate =
std::invocable<F, const dirent_t> &&
std::convertible_to<std::invoke_result_t<F, const dirent_t>, bool>;

// lightweight alternative to filesystem dir
class dir_t final {
public:
    constexpr dir_t() = default;
    constexpr dir_t(const dir_t& from) = delete;
    constexpr dir_t(dir_t&& from) noexcept : dir_(std::exchange(from.dir_, nullptr)) {}
    explicit dir_t(int handle) noexcept : dir_(fdopendir(handle)) {}
    explicit dir_t(const std::string& path) noexcept : dir_(opendir(path.c_str())) {}

    ~dir_t() { release(); }

    constexpr operator bool() const noexcept { return is_open(); }
    constexpr auto operator!() const noexcept { return !is_open(); }
    constexpr auto operator*() noexcept -> dirent_t { return dir_ ? readdir(dir_) : nullptr; }
    constexpr auto operator=(const dir_t& from) -> dir_t& = delete;

    auto operator=(dir_t&& from) noexcept -> dir_t& {
        release();
        dir_ = std::exchange(from.dir_, nullptr);
        return *this;
    }

    auto operator=(const std::string& path) noexcept -> dir_t& {
        release();
        dir_ = opendir(path.c_str());
        return *this;
    }

    auto operator=(int handle) noexcept -> dir_t& {
        release();
        dir_ = fdopendir(handle);
        return *this;
    }

    constexpr auto is_open() const noexcept -> bool { return dir_ != nullptr; }

    auto get() noexcept -> dirent_t { return dir_ ? readdir(dir_) : nullptr; }

private:
    DIR *dir_{nullptr};

    void release() {
        if (dir_)
            ::closedir(std::exchange(dir_, nullptr));
    }
};
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

template <fsys::prefix_predicate Func>
inline auto scan_prefix(const std::string& path, Func func) {
    std::size_t count = 0;
    fsys::dir_t dir(path);
    fsys::dirent_t entry{nullptr};
    while ((entry = dir.get()) != nullptr) {
        func(entry);
    }
    return count;
}
} // namespace busuto
