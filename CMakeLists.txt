cmake_minimum_required(VERSION 3.22..4.3.2)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
# cmake include modules are included early to ensure that all features are available before project() is called
include(FeatureSummary)
include(FetchContent)
include(CMakeDependentOption)

# cmake/options.cmake is doing a lot of heavy lifting, so it's best to include it early to ensure that all options are set before project() is called
include(options)

# CMake upgrade notes:
#  - In CMake >= 3.29, we should deprecate USE_ALTERNATE_LINKER in favour of CMake's CMAKE_LINKER_TYPE
set(CMAKE_EXPORT_NO_PACKAGE_REGISTRY ON) # For rapidjson
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW) # file(GET_RUNTIME_DEPENDENCIES) normalizes paths before matching
endif()

# For legacy non-CMakePresets.json builds, set the C++ standard to 23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Necessary for vcpkg manifest mode
if(BUILD_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()
if(BUILD_BENCHMARKS)
    list(APPEND VCPKG_MANIFEST_FEATURES "benchmarks")
endif()

project(chatterino
    VERSION 2.5.5
    DESCRIPTION "Chat client for twitch.tv"
    HOMEPAGE_URL "https://chatterino.com/"
)

# Determine Qt version
if(BUILD_WITH_QT6)
    find_package(Qt6 QUIET COMPONENTS Core)
    if(NOT Qt6_FOUND)
        message(FATAL_ERROR "BUILD_WITH_QT6=ON but Qt6 was not found")
    endif()
    set(MAJOR_QT_VERSION 6)
else()
    find_package(Qt5 QUIET COMPONENTS Core)
    if(NOT Qt5_FOUND)
        message(FATAL_ERROR "BUILD_WITH_QT6=OFF but Qt5 was not found")
    endif()
    set(MAJOR_QT_VERSION 5)
    message(WARNING "Qt6 is not found or selected, defaulting to Qt5. WARNING: This may result in build errors or unexpected behavior. Continue building at your own risk")
endif()
message(STATUS "Qt version: ${Qt${MAJOR_QT_VERSION}_VERSION}")

include(ccache)
include(fetch_contents)
include(GIT)
include(resources/generate_resources) # Generate resource files
include(vcpkg)

# Locate and acquire all dependencies: core + feature-gated options
# Design by contract: Requires MAJOR_QT_VERSION  and the vcpkg module for Boost
include(dependencies)

# Configure compiler, instrumentation, LTO, sanitizers
# Can stil be broken into smaller modules
include(toolchain)

if(WIN32)
    add_subdirectory(lib/WinToast EXCLUDE_FROM_ALL)
endif()

if(BUILD_TESTS)
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
endif()

# Used to provide a date of build in the About page (for nightly builds). Getting the actual time of
# compilation in CMake is a more involved, as documented in https://stackoverflow.com/q/24292898.
# For CI runs, however, the date of build file generation should be consistent with the date of
# compilation so this approximation is "good enough" for our purpose.
if(DEFINED ENV{CHATTERINO_SKIP_DATE_GEN})
    set(cmake_gen_date "1970-01-01")
else()
    string(TIMESTAMP cmake_gen_date "%Y-%m-%d")
endif()

add_subdirectory(lib/twitch-eventsub-ws)
add_subdirectory(src)

if(BUILD_TESTS OR BUILD_BENCHMARKS)
    add_subdirectory(mocks)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

feature_summary(WHAT ALL)
