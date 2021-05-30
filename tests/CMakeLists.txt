project(chatterino-test)

set(main_dir ${CMAKE_SOURCE_DIR}/src)

include_directories(${main_dir})

set(test_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/AccessGuard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkRequest.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ChatterSet.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/HighlightPhrase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Emojis.cpp
    )

add_executable(${PROJECT_NAME}
    ${test_SOURCES}
    $<TARGET_OBJECTS:chatterino-lib>
    )
add_sanitizers(${PROJECT_NAME})

# Enable autogeneration of Qts MOC/RCC/UIC
set_target_properties(${PROJECT_NAME}
    PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
    )

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
    gtest gtest_main
    Pajlada::Serialize
    Pajlada::Settings
    Pajlada::Signals
    Threads::Threads
    LRUCache
    chatterino-lib
    )

target_compile_definitions(${PROJECT_NAME} PRIVATE
    CHATTERINO
    CHATTERINO_TEST
    UNICODE
    AB_CUSTOM_THEME
    AB_CUSTOM_SETTINGS
    IRC_STATIC
    IRC_NAMESPACE=Communi
    CHATTERINO_GIT_HASH=\"${GIT_HASH}\"
    CHATTERINO_GIT_RELEASE=\"${GIT_RELEASE}\"
    CHATTERINO_GIT_COMMIT=\"${GIT_COMMIT}\"
    )

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        USEWINSDK
        )
endif ()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    )

# gtest_add_tests manages to discover the tests because it looks through the source files
# HOWEVER, it fails to run, because we have some bug that causes the QApplication exit to stall when no network requests have been made.
# ctest runs each test individually, so for now we require that testers just run the ./bin/chatterino-test binary without any filters applied
# gtest_add_tests(
#     TARGET ${PROJECT_NAME}
#     SOURCES ${test_SOURCES}
#     )
