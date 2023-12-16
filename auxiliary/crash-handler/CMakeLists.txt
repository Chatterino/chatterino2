project(chatterino-crash-handler 
        VERSION 0.1.0
        LANGUAGES CXX
)
set(PROJECT_LIB "${PROJECT_NAME}-lib")

set(LIB_SOURCE_FILES
    src/CommandLine.hpp
    src/CommandLine.cpp
    src/Recovery.hpp
    src/Recovery.cpp
)

set(EXE_SOURCE_FILES
    src/main.cpp
)

if(WIN32)
    list(APPEND LIB_SOURCE_FILES
        src/WinSupport.hpp
        src/WinSupport.cpp
    )
endif()

add_library(${PROJECT_LIB} OBJECT ${LIB_SOURCE_FILES})
target_link_libraries(${PROJECT_LIB}
    PUBLIC
        crashpad_handler_lib
        crashpad_tools
        MagicEnum
)
target_include_directories(${PROJECT_LIB} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src")

add_executable(${PROJECT_NAME} WIN32 ${EXE_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_LIB})
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/crashpad"
        OUTPUT_NAME "crashpad_handler"
)

if(WIN32)
    target_compile_definitions(${PROJECT_LIB} PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Configure compilers
if(MSVC)
    target_compile_options(${PROJECT_LIB} PUBLIC /EHsc /bigobj /utf-8)
    target_compile_options(${PROJECT_LIB} PUBLIC
        /W4
        # 5038 - warnings about initialization order
        /w15038
    )
else()
    message(WARNING "No warnings configured for this compiler!")
endif()

if(CHATTERINO_ENABLE_LTO)
    message(STATUS "Enabling LTO for ${PROJECT_NAME}")
    set_property(TARGET ${PROJECT_NAME}
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# TESTS

if(TARGET gtest)
    message(STATUS "Tests enabled for ${PROJECT_NAME}")

    set(PROJECT_TESTS "${PROJECT_NAME}-test")
    set(TEST_FILES 
        tests/CommandLineTest.cpp
        tests/RecoveryTest.cpp
    )
    if(WIN32)
        list(APPEND TEST_FILES
            tests/WinSupportTest.cpp
        )
    endif()

    add_executable(${PROJECT_TESTS} ${TEST_FILES})
    target_link_libraries(${PROJECT_TESTS} PRIVATE ${PROJECT_LIB} GTest::gtest_main gmock)
    set_target_properties(${PROJECT_TESTS}
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
    gtest_discover_tests(${PROJECT_TESTS})
endif()
