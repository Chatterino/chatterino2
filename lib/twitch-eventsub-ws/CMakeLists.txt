cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    )

project(twitch-eventsub-ws VERSION 0.1.0)

# Find boost on the system
find_package(Boost REQUIRED OPTIONAL_COMPONENTS json)

# Find OpenSSL on the system
find_package(OpenSSL REQUIRED)

option(BUILD_WITH_QT6 "Build with Qt6" On)
option(FORCE_JSON_GENERATION "Make sure JSON implementations are generated at build time" Off)
option(SKIP_JSON_GENERATION "Skip JSON implementations generation at build time" Off)

if (BUILD_WITH_QT6)
    set(MAJOR_QT_VERSION "6")
else()
    set(MAJOR_QT_VERSION "5")
endif()
find_package(Qt${MAJOR_QT_VERSION} REQUIRED COMPONENTS Core)
find_package(Python3 COMPONENTS Interpreter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(cmake/GenerateJson.cmake)

add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
