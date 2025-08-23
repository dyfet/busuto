# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

include(CheckCXXSourceCompiles)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(FindPkgConfig)

if(MSVC)
    message(FATAL_ERROR "Requires MingW32 with Posix support for Windows.")
endif()

if(WIN32)
    set(BUSUTO_EXTRA_LIBS "-static -static-libgcc -static-libstdc++ -lws2_32")
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    set(BUILD_DEBUG true)
    add_compile_definitions(DEBUG=1)
else()
    add_compile_definitions(NDEBUG=1)
endif()
