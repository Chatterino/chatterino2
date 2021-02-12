cmake_minimum_required(VERSION 3.8)
include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

project(Chatterino2)

include(pmm)
pmm(CONAN REMOTES BINCRAFTERS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network Multimedia Svg Concurrent)

add_subdirectory(src)
#add_subdirectory(tests)


feature_summary(WHAT ALL)