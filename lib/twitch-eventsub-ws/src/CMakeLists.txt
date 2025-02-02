set(SOURCE_FILES
    session.cpp

    chrono.cpp

    json.cpp
    string.cpp

    payloads/subscription.cpp
    payloads/session-welcome.cpp

    # Subscription types
    payloads/channel-ban-v1.cpp
    payloads/stream-online-v1.cpp
    payloads/stream-offline-v1.cpp
    payloads/channel-update-v1.cpp
    payloads/channel-chat-notification-v1.cpp
    payloads/channel-chat-message-v1.cpp
    payloads/channel-moderate-v2.cpp
    # Add your new subscription type source file above this line

    messages/metadata.cpp
    )

add_library(${PROJECT_NAME} STATIC ${SOURCE_FILES})

# Generate source groups for use in IDEs
# source_group(TREE ${CMAKE_SOURCE_DIR} FILES ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")


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

# Hack to get the include directories from Python
get_target_property(_inc_dirs ${PROJECT_NAME} INCLUDE_DIRECTORIES)
list(APPEND _inc_dirs ${Boost_INCLUDE_DIRS})
list(APPEND _inc_dirs ${OPENSSL_INCLUDE_DIR})
list(JOIN _inc_dirs ";" _inc_dir)
add_custom_target(_ast_includes echo "@@INCLUDE_DIRS=${_inc_dir}")

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
