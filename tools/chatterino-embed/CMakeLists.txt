project(chatterino-embed
        VERSION 0.1.0
        LANGUAGES CXX
)

include(GNUInstallDirs)

# All public headers
set(HEADERS
    ChatterinoApp.hpp
    ChatterinoAppBuilder.hpp
    ChatterinoConfig.hpp
)
# All private sources
set(SOURCES
    ChatterinoApp.cpp
    ChatterinoAppBuilder.cpp
)

list(TRANSFORM HEADERS PREPEND include/chatterino-embed/)
list(TRANSFORM SOURCES PREPEND src/)

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

# Ubuntu 22.04 doesn't have CMake 3.23 yet (introduction of file sets).
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    target_sources(${PROJECT_NAME} PUBLIC
        FILE_SET HEADERS
        BASE_DIRS include
        FILES
            ${HEADERS}
    )
    set_target_properties(${PROJECT_NAME}
        PROPERTIES
            VERIFY_INTERFACE_HEADER_SETS ON
    )
else()
    message(FATAL_ERROR "Unimplemented...")
    target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
endif()

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        # Mostly a hack for RUNTIME_DEPENDENCIES to pick up the required DLLs on
        # Windows + conan.
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        CXX_STANDARD 23
        AUTOMOC ON
        AUTORCC ON
        AUTOUIC ON
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    chatterino-lib
)

target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_CHATTERINO_EMBED_LIB)

set(_runtime_args)
if (WIN32)
    # Include required third party DLLs (OpenSSL)
    set(_runtime_args
        RUNTIME_DEPENDENCIES 
            PRE_EXCLUDE_REGEXES "^api-ms-" "^ext-ms-" "^[qQ][tT]"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )
endif()

install(
    TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-config
        ${_runtime_args}
        COMPONENT
            chatterino-embed
        LIBRARY
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME
        FILE_SET HEADERS
)
install(
    EXPORT ${PROJECT_NAME}-config
    COMPONENT
        chatterino-embed
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

if(CHATTERINO_ENABLE_LTO)
    message(STATUS "Enabling LTO for ${PROJECT_NAME}")
    set_property(TARGET ${PROJECT_NAME}
                 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
