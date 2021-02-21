cmake_minimum_required(VERSION 3.8)
cmake_policy(SET CMP0087 NEW)
include(FeatureSummary)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_BINARY_DIR})

project(Chatterino2 VERSION 2.2.3)

option(BUILD_APP "Build Chatterino2" ON)
option(BUILD_TESTS "Build the tests for Chatterino2" OFF)
option(USE_PACKAGE_MANAGER "Use conan for managing packages" OFF)
option(USE_SYSTEM_PAJLADA_SETTINGS "Use system pajlada settings library" OFF)
option(USE_SYSTEM_LIBCOMMUNI "Use system communi library" OFF)
option(USE_SYSTEM_QT5KEYCHAIN "Use system Qt5Keychain library" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

SET(RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
SET(LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")
SET(EXECUTABLE_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
SET(LIBRARY_OUTPUT_PATH ${RUNTIME_OUTPUT_DIRECTORY})
SET(CHATTERINO2_VERSION ${PROJECT_VERSION})

if (WIN32)
    # fix executable paths for windows
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${RUNTIME_OUTPUT_DIRECTORY})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${RUNTIME_OUTPUT_DIRECTORY})
endif ()

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message("Using ${CCACHE_PROGRAM} for speeding up build")
endif ()

find_package(Git)

if (GIT_EXECUTABLE)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_REPOSITORY_NOT_FOUND
            ERROR_QUIET
    )
endif ()

if (GIT_EXECUTABLE AND NOT GIT_REPOSITORY_NOT_FOUND)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_RELEASE
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )
else ()
    set(GIT_HASH "GIT-REPOSITORY-NOT-FOUND")
    set(GIT_COMMIT "GIT-REPOSITORY-NOT-FOUND")
    set(GIT_RELEASE "${PROJECT_VERSION}")
endif ()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network Multimedia Svg Concurrent)

if (WIN32)
    find_package(WinToast REQUIRED)
endif ()

if (USE_PACKAGE_MANAGER)
    include(pmm)
    pmm(CONAN REMOTES anotherfoxguy-artifactory https://anotherfoxguy.jfrog.io/artifactory/api/conan/chatterino)
else (USE_PACKAGE_MANAGER)
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/dependencies" CACHE PATH "Path to the dependencies")

    find_package(Boost REQUIRED)
    find_package(OpenSSL REQUIRED)

    # XXX: Will this fail on e.g. Windows or macOS?
    find_package(Threads REQUIRED)

    find_package(PajladaSerialize REQUIRED)
    find_package(PajladaSignals REQUIRED)
    if (USE_SYSTEM_PAJLADA_SETTINGS)
        find_package(PajladaSettings REQUIRED)
    else()
        if (NOT EXISTS "${CMAKE_SOURCE_DIR}/lib/settings/CMakeLists.txt")
            message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/settings/CMakeLists.txt")
        endif()

        add_subdirectory("${CMAKE_SOURCE_DIR}/lib/settings" EXCLUDE_FROM_ALL)
    endif()

    if (BUILD_APP)
        if (USE_SYSTEM_LIBCOMMUNI)
            find_package(LibCommuni REQUIRED)
        else()
            set(LIBCOMMUNI_ROOT_LIB_FOLDER "${CMAKE_SOURCE_DIR}/lib/libcommuni-cmake")
            if (NOT EXISTS "${LIBCOMMUNI_ROOT_LIB_FOLDER}/CMakeLists.txt")
                message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/libcommuni/CMakeLists.txt")
            endif()

            add_subdirectory("${LIBCOMMUNI_ROOT_LIB_FOLDER}" EXCLUDE_FROM_ALL)
        endif()
        if (USE_SYSTEM_QT5KEYCHAIN)
            find_package(Qt5Keychain REQUIRED)
        else()
            set(QT5KEYCHAIN_ROOT_LIB_FOLDER "${CMAKE_SOURCE_DIR}/lib/qtkeychain")
            if (NOT EXISTS "${QT5KEYCHAIN_ROOT_LIB_FOLDER}/CMakeLists.txt")
                message(FATAL_ERROR "Submodules probably not loaded, unable to find lib/qtkeychain/CMakeLists.txt")
            endif()

            add_subdirectory("${QT5KEYCHAIN_ROOT_LIB_FOLDER}" EXCLUDE_FROM_ALL)
            if (NOT TARGET qt5keychain)
                message(FATAL_ERROR "qt5keychain target was not created :@")
            endif()
        endif()
        find_package(Rapidjson REQUIRED)
        find_package(Websocketpp REQUIRED)
    endif ()
    if (BUILD_TESTS)
        find_package(GTest REQUIRED)
    endif ()
endif (USE_PACKAGE_MANAGER)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (BUILD_APP)
    add_subdirectory(src)
endif ()
if (BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

message("Injected git values: ${GIT_COMMIT} (${GIT_RELEASE}) ${GIT_HASH}")

feature_summary(WHAT ALL)