cmake_minimum_required(VERSION 3.8)
include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_BINARY_DIR})

option(USE_PACKAGE_MANAGER "Use conan for managing packages" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

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
endif()

project(Chatterino2)

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
endif (USE_PACKAGE_MANAGER)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network Multimedia Svg Concurrent)

add_subdirectory(src)
#add_subdirectory(tests)


feature_summary(WHAT ALL)