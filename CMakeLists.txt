cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`
if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW) # find Boost's own CMake config file
endif ()
include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    "${CMAKE_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
    )

option(BUILD_APP "Build Chatterino" ON)
option(BUILD_TESTS "Build the tests for Chatterino" OFF)
option(BUILD_BENCHMARKS "Build the benchmarks for Chatterino" OFF)
option(USE_SYSTEM_PAJLADA_SETTINGS "Use system pajlada settings library" OFF)
option(USE_SYSTEM_LIBCOMMUNI "Use system communi library" OFF)
option(USE_SYSTEM_QTKEYCHAIN "Use system QtKeychain library" OFF)
option(BUILD_WITH_QTKEYCHAIN "Build Chatterino with support for your system key chain" ON)
option(USE_SYSTEM_MINIAUDIO "Build Chatterino with your system miniaudio" OFF)
option(BUILD_WITH_CRASHPAD "Build chatterino with crashpad" OFF)
option(USE_PRECOMPILED_HEADERS "Use precompiled headers" ON)
option(BUILD_WITH_QT6 "Build with Qt6" On)
option(BUILD_WITH_LIBNOTIFY "Build with libnotify" ON)
option(CHATTERINO_GENERATE_COVERAGE "Generate coverage files" OFF)
# We don't use translations, and we don't want qtkeychain to build translations
option(BUILD_TRANSLATIONS "" OFF)
option(BUILD_SHARED_LIBS "" OFF)
option(CHATTERINO_LTO "Enable LTO for all targets" OFF)
option(CHATTERINO_PLUGINS "Enable ALPHA plugin support in Chatterino" OFF)
option(CHATTERINO_USE_GDI_FONTENGINE "Use the legacy GDI fontengine instead of the new DirectWrite one on Windows (Qt 6.8.0 and later)" ON)

option(CHATTERINO_UPDATER "Enable update checks" ON)
mark_as_advanced(CHATTERINO_UPDATER)

if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()
if(BUILD_BENCHMARKS)
    list(APPEND VCPKG_MANIFEST_FEATURES "benchmarks")
endif()

project(chatterino
    VERSION 2.5.2
    DESCRIPTION "Chat client for twitch.tv"
    HOMEPAGE_URL "https://chatterino.com/"
)

if(CHATTERINO_GENERATE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-coverage)
        add_link_options(-coverage)
    endif()
endif()

if(CHATTERINO_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT CHATTERINO_ENABLE_LTO OUTPUT IPO_ERROR)
    message(STATUS "LTO: Enabled (Supported: ${CHATTERINO_ENABLE_LTO} - ${IPO_ERROR})")
else()
    message(STATUS "LTO: Disabled")
endif()

if (BUILD_WITH_QT6)
    set(MAJOR_QT_VERSION "6")
else()
    message(WARNING "Qt5 is not supported, continue building at your own risk")
    set(MAJOR_QT_VERSION "5")
endif()

find_program(CCACHE_PROGRAM ccache)
find_program(SCCACHE_PROGRAM sccache)
if (SCCACHE_PROGRAM)
    set(_compiler_launcher ${SCCACHE_PROGRAM})
elseif (CCACHE_PROGRAM)
    set(_compiler_launcher ${CCACHE_PROGRAM})
endif ()


# Alternate linker code taken from heavyai/heavydb
# https://github.com/heavyai/heavydb/blob/0517d99b467806f6af7b4c969e351368a667497d/CMakeLists.txt#L87-L103
macro(set_alternate_linker linker)
  find_program(LINKER_EXECUTABLE ld.${USE_ALTERNATE_LINKER} ${USE_ALTERNATE_LINKER})
  if(LINKER_EXECUTABLE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND "${CMAKE_CXX_COMPILER_VERSION}" VERSION_LESS 12.0.0)
      add_link_options("-ld-path=${USE_ALTERNATE_LINKER}")
    else()
      add_link_options("-fuse-ld=${USE_ALTERNATE_LINKER}")
    endif()
  else()
    set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker" FORCE)
  endif()
endmacro()

set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker. Leave empty for system default; alternatives are 'gold', 'lld', 'bfd', 'mold'")
if(NOT "${USE_ALTERNATE_LINKER}" STREQUAL "")
  set_alternate_linker(${USE_ALTERNATE_LINKER})
endif()

if (_compiler_launcher)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${_compiler_launcher}" CACHE STRING "CXX compiler launcher")
    message(STATUS "Using ${_compiler_launcher} for speeding up build")

    if (MSVC)
        # /Zi can't be used with (s)ccache
        # Use /Z7 instead (debug info in object files)
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
        elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            string(REPLACE "/Zi" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
        endif()
    endif()
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GIT.cmake)

find_package(Qt${MAJOR_QT_VERSION} REQUIRED
    COMPONENTS
    Core
    Widgets
    Gui
    Network
    Svg
    Concurrent
    )

message(STATUS "Qt version: ${Qt${MAJOR_QT_VERSION}_VERSION}")

if (WIN32)
    add_subdirectory(lib/WinToast EXCLUDE_FROM_ALL)
endif ()

find_package(Sanitizers QUIET)

# Find boost on the system
find_package(Boost REQUIRED OPTIONAL_COMPONENTS headers)

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
    if (USE_SYSTEM_QTKEYCHAIN)
        find_package(Qt${MAJOR_QT_VERSION}Keychain REQUIRED)
    else()
        set(QTKEYCHAIN_ROOT_LIB_FOLDER "${CMAKE_SOURCE_DIR}/lib/qtkeychain")
        if (NOT EXISTS "${QTKEYCHAIN_ROOT_LIB_FOLDER}/CMakeLists.txt")
            message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/qtkeychain/CMakeLists.txt")
        endif()

        set(_prev_testing ${BUILD_TESTING})
        set(BUILD_TESTING Off)
        add_subdirectory("${QTKEYCHAIN_ROOT_LIB_FOLDER}" EXCLUDE_FROM_ALL)
        set(BUILD_TESTING ${_prev_testing})

        if (NOT TARGET qt${MAJOR_QT_VERSION}keychain)
            message(FATAL_ERROR "qt${MAJOR_QT_VERSION}keychain target was not created :@")
        endif()
        if (MSVC AND "${MAJOR_QT_VERSION}" STREQUAL "5")
            target_compile_definitions(qt5keychain PRIVATE UNICODE)
            target_compile_options(qt5keychain PRIVATE /utf-8)
            set_target_properties(qt5keychain PROPERTIES CXX_STANDARD 17)
        endif()
    endif()
endif()

find_package(RapidJSON REQUIRED)

find_package(Websocketpp REQUIRED)

if (BUILD_TESTS)
    include(GoogleTest)
    # For MSVC: Prevent overriding the parent project's compiler/linker settings
    # See https://github.com/google/googletest/blob/main/googletest/README.md#visual-studio-dynamic-vs-static-runtimes
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/lib/googletest" "lib/googletest")

    mark_as_advanced(
            BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
            gmock_build_tests gtest_build_samples gtest_build_tests
            gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
    )

    set_target_properties(gtest PROPERTIES FOLDER lib)
    set_target_properties(gtest_main PROPERTIES FOLDER lib)
    set_target_properties(gmock PROPERTIES FOLDER lib)
    set_target_properties(gmock_main PROPERTIES FOLDER lib)
endif ()

if (BUILD_BENCHMARKS)
    # Include system benchmark (Google Benchmark)
    find_package(benchmark REQUIRED)
endif ()

find_package(PajladaSerialize REQUIRED)
find_package(PajladaSignals REQUIRED)
find_package(LRUCache REQUIRED)
find_package(MagicEnum REQUIRED)
find_package(Doxygen)
find_package(BoostCertify REQUIRED)

if (USE_SYSTEM_PAJLADA_SETTINGS)
    find_package(PajladaSettings REQUIRED)
else()
    if (NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/settings/CMakeLists.txt")
        message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/settings/CMakeLists.txt")
    endif()

    add_subdirectory("${CMAKE_SOURCE_DIR}/lib/settings" EXCLUDE_FROM_ALL)
endif()

if (CHATTERINO_PLUGINS)
    set(LUA_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/lib/lua/src")
    add_subdirectory(lib/lua)

    find_package(Sol2 REQUIRED)
endif()

if (BUILD_WITH_CRASHPAD)
    add_subdirectory("${CMAKE_SOURCE_DIR}/tools/crash-handler")
endif()

add_subdirectory(lib/twitch-eventsub-ws)

# Used to provide a date of build in the About page (for nightly builds). Getting the actual time of
# compilation in CMake is a more involved, as documented in https://stackoverflow.com/q/24292898.
# For CI runs, however, the date of build file generation should be consistent with the date of
# compilation so this approximation is "good enough" for our purpose.
if (DEFINED ENV{CHATTERINO_SKIP_DATE_GEN})
    set(cmake_gen_date "1970-01-01")
else ()
    string(TIMESTAMP cmake_gen_date "%Y-%m-%d")
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate resource files
include(cmake/resources/generate_resources.cmake)

add_subdirectory(src)

if (BUILD_TESTS OR BUILD_BENCHMARKS)
    add_subdirectory(mocks)
endif ()

if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif ()

feature_summary(WHAT ALL)
