generate_json_impls(
    OUTPUT_SOURCES eventsub_generated_sources
    BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.."
    FORCE ${FORCE_JSON_GENERATION}
    SKIP ${SKIP_JSON_GENERATION}
    HEADERS
        include/twitch-eventsub-ws/messages/metadata.hpp

        include/twitch-eventsub-ws/payloads/automod-message-hold-v2.hpp
        include/twitch-eventsub-ws/payloads/automod-message-update-v2.hpp
        include/twitch-eventsub-ws/payloads/automod-message.hpp
        include/twitch-eventsub-ws/payloads/channel-ban-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-chat-message-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-chat-notification-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-chat-user-message-hold-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-chat-user-message-update-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-moderate-v2.hpp
        include/twitch-eventsub-ws/payloads/channel-suspicious-user-message-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-suspicious-user-update-v1.hpp
        include/twitch-eventsub-ws/payloads/channel-update-v1.hpp
        include/twitch-eventsub-ws/payloads/session-welcome.hpp
        include/twitch-eventsub-ws/payloads/stream-offline-v1.hpp
        include/twitch-eventsub-ws/payloads/stream-online-v1.hpp
        include/twitch-eventsub-ws/payloads/structured-message.hpp
        include/twitch-eventsub-ws/payloads/subscription.hpp
        include/twitch-eventsub-ws/payloads/suspicious-users.hpp
)

set(SOURCE_FILES
    session.cpp

    chrono.cpp

    json.cpp
    string.cpp
    errors.cpp

    # Subscription types (only additional functions)
    payloads/channel-ban-v1.cpp
    payloads/channel-moderate-v2.cpp
    # Add your new subscription type source file above this line

    ${eventsub_generated_sources}
    )

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# Generate source groups for use in IDEs
# source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME}
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/date/include"
)

target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Qt${MAJOR_QT_VERSION}::Core
        Boost::headers
        OpenSSL::SSL
        OpenSSL::Crypto
        )

target_compile_definitions(${PROJECT_NAME} PUBLIC
    $<$<BOOL:${MSVC}>:BOOST_JSON_NO_LIB>
    $<$<BOOL:${MSVC}>:BOOST_CONTAINER_NO_LIB>
)

if (MSVC)
    # Add bigobj
    target_compile_options(${PROJECT_NAME} PRIVATE /EHsc /bigobj)

    # Change flags for RelWithDebInfo

    # Default: "/debug /INCREMENTAL"
    # Changes:
    #   - Disable incremental linking to reduce padding
    #   - Enable all optimizations - by default when /DEBUG is specified,
    #     these optimizations will be disabled. We need /DEBUG to generate a PDB.
    # See https://gitlab.kitware.com/cmake/cmake/-/issues/20812 for more details.
    set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "/DEBUG /INCREMENTAL:NO /OPT:REF,ICF,LBR")

    # Use the function inlining level from 'Release' mode (2).
    string(REPLACE "/Ob1" "/Ob2" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    # Add nativs to PDB
    set(_natvis_path "${CMAKE_CURRENT_LIST_DIR}/../docs/twitch-eventsub-ws.natvis")
    cmake_path(ABSOLUTE_PATH _natvis_path NORMALIZE)
    target_link_options(${PROJECT_NAME} INTERFACE "/NATVIS:${_natvis_path}")

    # Configure warnings

    # Someone adds /W3 before we add /W4.
    # This makes sure, only /W4 is specified.
    string(REPLACE "/W3" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    target_compile_options(${PROJECT_NAME} PUBLIC
        /W4
        # 5038 - warnings about initialization order
        /w15038
        # 4855 - implicit capture of 'this' via '[=]' is deprecated
        /w14855
        # Disable the following warnings (see reasoning above)
        /wd4505
        /wd4100
        /wd4267
    )
    # Disable min/max macros from Windows.h
    target_compile_definitions(${PROJECT_NAME} PUBLIC NOMINMAX)
else ()
    target_compile_options(${PROJECT_NAME} PUBLIC
        -Wall
        # Disable the following warnings
        -Wno-unused-function
        -Wno-switch
        -Wno-deprecated-declarations
        -Wno-sign-compare
        -Wno-unused-variable

        # Disabling strict-aliasing warnings for now, although we probably want to re-enable this in the future
        -Wno-strict-aliasing

        -Werror=return-type
        -Werror=reorder
    )

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${PROJECT_NAME} PUBLIC
            -Wno-unused-local-typedef
            -Wno-unused-private-field
            -Werror=inconsistent-missing-override
            -Werror=final-dtor-non-final-class
            -Werror=ambiguous-reversed-operator

        )
    else ()
        target_compile_options(${PROJECT_NAME} PUBLIC
            -Wno-class-memaccess
        )
    endif()
endif ()
