project(twitch-eventsub-ws-test)

set(SOURCES
    src/parse.cpp
    src/string.cpp
    src/chrono.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

if(CHATTERINO_SANITIZER_SUPPORT)
    if(COMMAND add_sanitizers)
        add_sanitizers(${PROJECT_NAME})
    else()
        message(FATAL_ERROR "CHATTERINO_SANITIZER_SUPPORT enabled, but did not find add_sanitizers command")
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE twitch-eventsub-ws GTest::gtest_main)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        CXX_STANDARD 20
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /bigobj)
endif()

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
