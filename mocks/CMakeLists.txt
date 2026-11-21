project(chatterino-mocks)

add_library(chatterino-mocks INTERFACE)

target_include_directories(chatterino-mocks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    target_sources(chatterino-mocks INTERFACE
        FILE_SET HEADERS
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/include"
        FILES
            include/mocks/BaseApplication.hpp
            include/mocks/Channel.hpp
            include/mocks/ChatterinoBadges.hpp
            include/mocks/DisabledStreamerMode.hpp
            include/mocks/EmoteController.hpp
            include/mocks/EmptyApplication.hpp
            include/mocks/Helix.hpp
            include/mocks/LinkResolver.hpp
            include/mocks/Logging.hpp
            include/mocks/TwitchIrcServer.hpp
            include/mocks/TwitchUsers.hpp
            include/mocks/UserData.hpp
    )
    set_target_properties(chatterino-mocks
        PROPERTIES
            VERIFY_INTERFACE_HEADER_SETS ON
    )
endif()

target_link_libraries(${PROJECT_NAME} INTERFACE gmock chatterino-lib)
