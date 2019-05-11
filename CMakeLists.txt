cmake_minimum_required(VERSION 3.8)

project(chatterino)

include_directories(src)

set(chatterino_SOURCES
    src/common/UsernameSet.cpp
    )

find_package(Qt5Widgets CONFIG REQUIRED)
find_package(Qt5 5.9.0 REQUIRED COMPONENTS
    Core
    )

# set(CMAKE_AUTOMOC ON)

if (BUILD_TESTS)
    message("++ Tests enabled")
    find_package(GTest)
    enable_testing()

    add_executable(chatterino-test
        ${chatterino_SOURCES}

        tests/src/main.cpp
        tests/src/UsernameSet.cpp
        )

    target_link_libraries(chatterino-test Qt5::Core)

    target_link_libraries(chatterino-test gtest gtest_main)

    gtest_discover_tests(chatterino-test)
else()
    message(FATAL_ERROR "This cmake file is only intended for tests right now. Use qmake to build chatterino2")
endif()
