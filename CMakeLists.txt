cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0087 NEW)
include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    "${CMAKE_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
    )

project(chatterino VERSION 2.3.4)

option(BUILD_APP "Build Chatterino" ON)
option(BUILD_TESTS "Build the tests for Chatterino" OFF)
option(BUILD_BENCHMARKS "Build the benchmarks for Chatterino" OFF)
option(USE_SYSTEM_PAJLADA_SETTINGS "Use system pajlada settings library" OFF)
option(USE_SYSTEM_LIBCOMMUNI "Use system communi library" OFF)
option(USE_SYSTEM_QTKEYCHAIN "Use system QtKeychain library" OFF)
option(BUILD_WITH_QTKEYCHAIN "Build Chatterino with support for your system key chain" ON)
option(USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)
option(BUILD_WITH_QT6 "Use Qt6 instead of default Qt5" OFF)

option(USE_CONAN "Use conan" OFF)

if (BUILD_WITH_QT6)
    set(MAJOR_QT_VERSION "6")
else()
    set(MAJOR_QT_VERSION "5")
endif()

if (USE_CONAN OR CONAN_EXPORTED)
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
    conan_basic_setup(TARGETS NO_OUTPUT_DIRS)
endif()

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message("Using ${CCACHE_PROGRAM} for speeding up build")
endif ()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GIT.cmake)

find_package(Qt${MAJOR_QT_VERSION} REQUIRED
    COMPONENTS
    Core
    Widgets
    Gui
    Network
    Multimedia
    Svg
    Concurrent
    )

if (WIN32)
    find_package(WinToast REQUIRED)
endif ()

find_package(Sanitizers)

# Find boost on the system
find_package(Boost REQUIRED)

# Find OpenSSL on the system
find_package(OpenSSL REQUIRED)

find_package(Threads REQUIRED)

find_library(LIBRT rt)

if (USE_SYSTEM_LIBCOMMUNI)
    find_package(LibCommuni REQUIRED)
else()
    set(LIBCOMMUNI_ROOT_LIB_FOLDER "${CMAKE_SOURCE_DIR}/lib/libcommuni")
    if (NOT EXISTS "${LIBCOMMUNI_ROOT_LIB_FOLDER}/CMakeLists.txt")
        message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/libcommuni/CMakeLists.txt")
    endif()

    add_subdirectory("${LIBCOMMUNI_ROOT_LIB_FOLDER}" EXCLUDE_FROM_ALL)
endif()

if (BUILD_WITH_QTKEYCHAIN)
    # Link QtKeychain statically
    option(QTKEYCHAIN_STATIC "" ON)
    if (USE_SYSTEM_QTKEYCHAIN)
        find_package(Qt${MAJOR_QT_VERSION}Keychain REQUIRED)
    else()
        set(QTKEYCHAIN_ROOT_LIB_FOLDER "${CMAKE_SOURCE_DIR}/lib/qtkeychain")
        if (NOT EXISTS "${QTKEYCHAIN_ROOT_LIB_FOLDER}/CMakeLists.txt")
            message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/qtkeychain/CMakeLists.txt")
        endif()

        add_subdirectory("${QTKEYCHAIN_ROOT_LIB_FOLDER}" EXCLUDE_FROM_ALL)
        if (NOT TARGET qt${MAJOR_QT_VERSION}keychain)
            message(FATAL_ERROR "qt${MAJOR_QT_VERSION}keychain target was not created :@")
        endif()
    endif()
endif()

find_package(RapidJSON REQUIRED)

find_package(Websocketpp REQUIRED)

if (BUILD_TESTS)
    find_package(GTest REQUIRED)
endif ()

if (BUILD_BENCHMARKS)
    # Include system benchmark (Google Benchmark)
    find_package(benchmark REQUIRED)
endif ()

find_package(PajladaSerialize REQUIRED)
find_package(PajladaSignals REQUIRED)
find_package(LRUCache REQUIRED)

if (USE_SYSTEM_PAJLADA_SETTINGS)
    find_package(PajladaSettings REQUIRED)
else()
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/settings/CMakeLists.txt")
        message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/settings/CMakeLists.txt")
    endif()

    add_subdirectory("${CMAKE_SOURCE_DIR}/lib/settings" EXCLUDE_FROM_ALL)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (BUILD_TESTS OR BUILD_BENCHMARKS)
    add_definitions(-DCHATTERINO_TEST)
endif ()

add_subdirectory(src)

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()

feature_summary(WHAT ALL)
