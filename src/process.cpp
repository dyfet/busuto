// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

#include "process.hpp"

using namespace busuto;

auto process::detach(const process::args_t& args, std::string argv0, const process::args_t& env, void (*init)()) -> pid_t {
    if (args.empty()) return -1;
    auto argv = make_argv(args);
    if (argv0.empty())
        argv0 = args[0];
    auto child = fork();
    if (!child) {
#if defined(SIGTSTP) && defined(TIOCNOTTY)
        if (setpgid(0, getpid()) == -1)
            ::_exit(-1);

        auto fd = open("/dev/tty", O_RDWR);
        if (fd >= 0) {
            ::ioctl(fd, TIOCNOTTY, nullptr);
            ::close(fd);
        }
#else
        if (setpgid(0, getpid()) == -1)
            ::_exit(-1);

        if (getppid() != 1) {
            auto pid = fork();
            if (pid < 0)
                ::_exit(-1);
            else if (pid > 0)
                ::_exit(0);
        }
#endif
        init();
        if (!env.empty()) {
            auto envp = make_argv(env);
            execvpe(argv0.c_str(), argv.get(), envp.get());
        } else {
            execvp(argv0.c_str(), argv.get());
        }
        ::_exit(-1);
    }
    return child;
}

auto process::spawn(const process::args_t& args, std::string argv0, const process::args_t& env, void (*init)()) -> pid_t {
    if (args.empty()) return -1;
    auto argv = make_argv(args);
    if (argv0.empty())
        argv0 = args[0];
    auto child = fork();
    if (!child) {
        init();
        if (!env.empty()) {
            auto envp = make_argv(env);
            execvpe(argv0.c_str(), argv.get(), envp.get());
        } else {
            execvp(argv0.c_str(), argv.get());
        }
        ::_exit(-1);
    }
    return child;
}

auto process::system(const process::args_t& args, std::string argv0, const process::args_t& env, void (*init)()) -> int {
    if (args.empty()) return -1;
    auto argv = make_argv(args);
    if (argv0.empty())
        argv0 = args[0];
    auto child = fork();
    if (!child) {
        init();
        if (!env.empty()) {
            auto envp = make_argv(env);
            execvpe(argv0.c_str(), argv.get(), envp.get());
        } else {
            execvp(argv0.c_str(), argv.get());
        }
        ::_exit(-1);
    }
    return process::wait(child);
}

auto process::env(const std::string& id, std::size_t max) noexcept -> std::optional<std::string> {
    auto buf = std::make_unique<char[]>(max);
    const char *out = getenv(id.c_str());
    if (!out) return {};
    buf[max - 1] = 0;
    strncpy(&buf[0], out, max);
    if (buf[max - 1] != 0) return {};
    return std::string(&buf[0]);
}
