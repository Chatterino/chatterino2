cmake_minimum_required(VERSION 3.8)
include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_BINARY_DIR})

project(Chatterino2)

option(BUILD_APP "Build Chatterino2" ON)
option(BUILD_TESTS "Build the tests for Chatterino2" OFF)
option(USE_PACKAGE_MANAGER "Use conan for managing packages" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})

if (WIN32)
    # fix executable paths for windows
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_OUTPUT_DIRECTORY})
endif ()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network Multimedia Svg Concurrent)

if (WIN32)
    find_package(WinToast REQUIRED)
endif()

if (USE_PACKAGE_MANAGER)
    include(pmm)
    pmm(CONAN)
else (USE_PACKAGE_MANAGER)
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/dependencies" CACHE PATH "Path to the dependencies")

    find_package(Boost REQUIRED)
    find_package(OpenSSL REQUIRED)

    find_package(LibCommuni REQUIRED)
    find_package(Qt5keychain REQUIRED)
    find_package(Rapidjson REQUIRED)
    find_package(Signals REQUIRED)
    find_package(Settings REQUIRED)
    find_package(Serialize REQUIRED)
    find_package(Websocketpp REQUIRED)

    if (BUILD_TESTS)
        find_package(GTest REQUIRED)
    endif ()
endif (USE_PACKAGE_MANAGER)


if (BUILD_APP)
    add_subdirectory(src)
endif ()
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

feature_summary(WHAT ALL)