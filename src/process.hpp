// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#pragma once

#include "system.hpp"
#include "strings.hpp"

#include <optional>
#include <memory>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#endif

#ifdef __FreeBSD__
#define execvpe(p, a, e) exect(p, a, e)
#endif

namespace busuto::process {
using args_t = std::vector<std::string>;

#ifdef _WIN32
inline constexpr auto dso_suffix = ".dll";
#else
inline constexpr auto dso_suffix = ".so";
#endif

#ifndef _WIN32
// posix systems only can do this...
template <typename F, typename... Args>
requires std::invocable<F, Args...> && std::convertible_to<std::invoke_result_t<F, Args...>, int>
inline auto at_fork(F func, Args... args) -> pid_t {
    auto child = fork();
    if (!child) {
        ::_exit(func(args...));
    }
    return child;
}
#endif

inline auto make_argv(const args_t& args) {
    auto argv = std::make_unique<char *[]>(args.size() + 1);
    for (auto pos = 0U; pos < args.size(); ++pos)
        argv[pos] = const_cast<char *>(args[pos].c_str());
    argv[args.size() + 1] = nullptr;
    return argv;
}

#ifndef _WIN32
inline auto wait(pid_t pid) noexcept {
    int status{-1};
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
}

inline auto stop(pid_t pid) noexcept {
    return !kill(pid, SIGTERM);
}

inline void env(const std::string& id, const std::string& value) {
    setenv(id.c_str(), value.c_str(), 1);
}
#endif

auto env(const std::string& id, std::size_t max = 256) noexcept -> std::optional<std::string>;
auto detach(const args_t& args, std::string argv0 = "", const args_t& env = {}, void (*init)() = [] {}) -> pid_t;
auto spawn(const args_t& args, std::string argv0 = "", const args_t& env = {}, void (*init)() = [] {}) -> pid_t;
auto system(const args_t& args, std::string argv0 = "", const args_t& env = {}, void (*init)() = [] {}) -> int;

inline auto system(const std::string& cmd, void (*init)() = [] {}) -> int {
    auto args = strings::tokenize(cmd);
    return system(args, args[0], {}, init);
}
} // namespace busuto::process
