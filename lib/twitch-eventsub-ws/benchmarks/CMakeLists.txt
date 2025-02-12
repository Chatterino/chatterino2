project(twitch-eventsub-ws-benchmark)

set(SOURCES
    src/main.cpp
    resources/bench.qrc

    src/parse.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
add_sanitizers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE 
    twitch-eventsub-ws
    benchmark::benchmark
    Qt${MAJOR_QT_VERSION}::Core # for QFile
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        CXX_STANDARD 20
        AUTORCC ON
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /bigobj)
endif()
