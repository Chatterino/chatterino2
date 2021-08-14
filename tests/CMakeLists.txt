project(chatterino-test)

set(test_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/AccessGuard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkRequest.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ChatterSet.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/HighlightPhrase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Emojis.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ExponentialBackoff.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/TwitchAccount.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Helpers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/RatelimitBucket.cpp
    # Add your new file above this line!
    )

add_executable(${PROJECT_NAME} ${test_SOURCES})
add_sanitizers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE chatterino-lib)

target_link_libraries(${PROJECT_NAME} PRIVATE gtest)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    CHATTERINO_TEST
    )

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
