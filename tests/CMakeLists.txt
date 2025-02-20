project(chatterino-test)

option(CHATTERINO_TEST_USE_PUBLIC_HTTPBIN "Use public httpbin for testing network requests" OFF)

set(test_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp
    ${CMAKE_CURRENT_LIST_DIR}/resources/test-resources.qrc
    ${CMAKE_CURRENT_LIST_DIR}/src/Test.hpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Test.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ChannelChatters.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/AccessGuard.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkCommon.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkRequest.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NetworkResult.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ChatterSet.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/HighlightPhrase.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Emojis.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ExponentialBackoff.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Helpers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/RatelimitBucket.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Hotkeys.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/UtilTwitch.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/IrcHelpers.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/TwitchPubSubClient.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/IrcMessageHandler.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/HighlightController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/FormatTime.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/LimitedQueue.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/BasicPubSub.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/SeventvEventAPI.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/BttvLiveUpdates.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Updates.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Filters.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/LinkParser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/InputCompletion.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Literals.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/XDGDesktopFile.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/XDGHelper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Selection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/NotebookTab.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/SplitInput.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/LinkInfo.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MessageLayout.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/QMagicEnum.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/ModerationAction.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Scrollbar.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Commands.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/FlagsEnum.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/MessageLayoutContainer.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/CancellationToken.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/Plugins.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/TwitchIrc.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/IgnoreController.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/OnceFlag.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/IncognitoBrowser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/EventSubMessages.cpp

    ${CMAKE_CURRENT_LIST_DIR}/src/lib/Snapshot.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/lib/Snapshot.hpp
    # Add your new file above this line!
    )

add_executable(${PROJECT_NAME} ${test_SOURCES})
add_sanitizers(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE chatterino-lib)
target_link_libraries(${PROJECT_NAME} PRIVATE chatterino-mocks)

target_link_libraries(${PROJECT_NAME} PRIVATE gtest gmock)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    )

if(CHATTERINO_ENABLE_LTO)
    message(STATUS "Enabling LTO for ${PROJECT_NAME}")
    set_property(TARGET ${PROJECT_NAME}
        PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(CHATTERINO_TEST_USE_PUBLIC_HTTPBIN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE CHATTERINO_TEST_USE_PUBLIC_HTTPBIN)
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    AUTORCC ON
    )

if (CHATTERINO_STATIC_QT_BUILD)
    qt_import_plugins(${PROJECT_NAME} INCLUDE_BY_TYPE
        platforms Qt::QXcbIntegrationPlugin
        Qt::QMinimalIntegrationPlugin
    )
endif ()

gtest_discover_tests(${PROJECT_NAME})
