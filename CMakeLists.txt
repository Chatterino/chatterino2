cmake_minimum_required(VERSION 3.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_SOURCE_DIR}/lib ${CMAKE_BINARY_DIR})
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/dependencies" CACHE PATH "Path to the dependencies")
include(FeatureSummary)

project(Chatterino2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network Multimedia Svg Concurrent)

#include(pmm)
#pmm(CONAN REMOTES BINCRAFTERS)

add_subdirectory(src)
add_subdirectory(tests)


feature_summary(WHAT ALL)