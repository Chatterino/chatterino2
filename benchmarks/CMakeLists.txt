project(chatterino-benchmark)

set(benchmark_SOURCES
    src/main.cpp
    resources/bench.qrc

    src/Emojis.cpp
    src/FormatTime.cpp
    src/Helpers.cpp
    src/LimitedQueue.cpp
    src/LinkParser.cpp
    src/RecentMessages.cpp
    # Add your new file above this line!
    )

add_executable(${PROJECT_NAME} ${benchmark_SOURCES})
add_sanitizers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE chatterino-lib)
target_link_libraries(${PROJECT_NAME} PRIVATE chatterino-mocks)

target_link_libraries(${PROJECT_NAME} PRIVATE benchmark::benchmark)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    AUTORCC ON
    )

if (CHATTERINO_STATIC_QT_BUILD)
    qt_import_plugins(${PROJECT_NAME} INCLUDE_BY_TYPE
        platforms Qt::QXcbIntegrationPlugin
        Qt::QMinimalIntegrationPlugin
    )
endif ()
