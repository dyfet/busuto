# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2025 David Sugar <tychosoft@gmail.com>

cmake_minimum_required(VERSION 3.16.0)
project(Busuto VERSION 0.3.2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

file(
    GLOB LINT_SOURCES
    RELATIVE ${PROJECT_SOURCE_DIR}
    src/*.hpp src/*.cpp test/*.cpp)

include(cmake/custom.cmake OPTIONAL)
include(cmake/project.cmake)
include(cmake/features.cmake)
include(cmake/coverage.cmake)
include(cmake/linting.cmake)
include(cmake/deploy.cmake OPTIONAL)

file(GLOB optional .git[a-z]* *.json *.in *.yml .clang* cmake/*)
file(GLOB markdown *.md)
file(GLOB headers CONFIGURE_DEPENDS src/*.hpp)
file(GLOB sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB extras CONFIGURE_DEPENDS extra/*.hpp)

# Setup headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/busuto.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/busuto.pc
    @ONLY
)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
add_library(busuto ${sources} ${headers})
target_link_libraries(busuto PUBLIC ${BUSUTO_EXTRA_LIBS} Threads::Threads)
if(BUILD_SHARED_LIBS AND CMAKE_BUILD_TYPE MATCHES "^Rel")
    set_target_properties(busuto PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
    )
endif()

# Testing
enable_testing()

add_executable(test_atomic test/atomic.cpp src/atomic.hpp)
add_test(NAME test-atomic COMMAND test_atomic)
target_link_libraries(test_atomic PRIVATE busuto)

add_executable(test_binary test/binary.cpp src/binary.hpp)
add_test(NAME test-binary COMMAND test_binary)
target_link_libraries(test_binary PRIVATE busuto)

add_executable(test_expected test/expected.cpp src/expected.hpp)
add_test(NAME test-expected COMMAND test_expected)
target_link_libraries(test_expected PRIVATE busuto)

add_executable(test_locking test/locking.cpp src/common.hpp src/locking.hpp)
add_test(NAME test-locking COMMAND test_locking)
target_link_libraries(test_locking PRIVATE busuto)

add_executable(test_scan test/scan.cpp src/common.hpp src/scan.hpp)
add_test(NAME test-scan COMMAND test_scan)
target_link_libraries(test_scan PRIVATE busuto)

add_executable(test_sync test/sync.cpp src/common.hpp src/sync.hpp)
add_test(NAME test-sync COMMAND test_sync)
target_link_libraries(test_sync PRIVATE busuto)

add_executable(test_threads test/threads.cpp src/system.hpp src/threads.hpp)
add_test(NAME test-threads COMMAND test_threads)
target_link_libraries(test_threads PRIVATE busuto)

add_executable(test_sockets test/sockets.cpp src/system.hpp src/sockets.hpp)
add_test(NAME test-sockets COMMAND test_sockets)
target_link_libraries(test_sockets PRIVATE busuto)

add_executable(test_safe test/safe.cpp src/safe.hpp)
add_test(NAME test-safe COMMAND test_safe)
target_link_libraries(test_safe PRIVATE busuto)

add_executable(test_service test/service.cpp src/service.hpp)
add_test(NAME test-service COMMAND test_service)
target_link_libraries(test_service PRIVATE busuto)

add_executable(test_strings test/service.cpp src/service.hpp)
add_test(NAME test-strings COMMAND test_strings)
target_link_libraries(test_strings PRIVATE busuto)

# Extras amd install...
add_custom_target(header-files SOURCES ${headers} ${extras})
add_custom_target(support-files SOURCES ${markdown} ${optional})

install(FILES ${headers} ${extras}  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/busuto")

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/busuto.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

install(TARGETS busuto
    EXPORT busutoTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/busuto
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/busuto
)

install(EXPORT busutoTargets
    FILE busutoTargets.cmake
    NAMESPACE busuto::
    DESTINATION lib/cmake/busuto
)

