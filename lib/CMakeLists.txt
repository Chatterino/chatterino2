cmake_minimum_required(VERSION 3.2)

# ------------------------------------------------------------------------------------------------#
# The intention of this script is to provide a simple solution for building all dependencies
# required for Chatterino2. It is in fact a super- or meta-build project which makes use of
# the ExternalProject module of CMake to build the individual dependencies.
# ------------------------------------------------------------------------------------------------#

project(chatterino2-dependencies VERSION 1)


# ------------------------------------------------------------------------------------------------#
# Options to enable/disable building specific dependencies
# ------------------------------------------------------------------------------------------------#

# Options with same default value on all platforms
option(BUILD_HUMANIZE "Build Humanize IRC framework" ON)
option(BUILD_LIBCOMMUNI "Build LibCommuni IRC framework" OFF)
option(BUILD_QTKEYCHAIN "Build QtKeychain" ON)
option(BUILD_RAPIDJSON "Build RapidJSON library" ON)
option(BUILD_SERIALIZE "Build Serialize library" ON)
option(BUILD_SIGNALS "Build Signals library" ON)
option(BUILD_WEBSOCKETPP "Build WebSocketpp library" ON)

# ------------------------------------------------------------------------------------------------#
# General setup and initialization
# ------------------------------------------------------------------------------------------------#

include(ExternalProject)

# Set the EP_BASE directory property to setup the build directory structure (see the
# ExternalProject documentation for more information)
set_property(DIRECTORY PROPERTY EP_BASE ${CMAKE_BINARY_DIR})

# Determine the name for the output directory where dependencies are going to be installed
if (WIN32)
    set(DEPENDENCIES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Dependencies_${CMAKE_SYSTEM_NAME}_${CMAKE_GENERATOR})
    string(REPLACE " " "-" DEPENDENCIES_OUTPUT_DIR ${DEPENDENCIES_OUTPUT_DIR})
else ()
    set(DEPENDENCIES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/Dependencies_${CMAKE_SYSTEM_NAME})
endif ()

if (MSVC)
    set(DEPENDENCIES_INCLUDE_DIR ${DEPENDENCIES_OUTPUT_DIR}/include)
    set(DEPENDENCIES_LIB_DIR ${DEPENDENCIES_OUTPUT_DIR}/lib)
    set(DEPENDENCIES_BIN_DIR ${DEPENDENCIES_OUTPUT_DIR}/bin)
else ()
    set(DEPENDENCIES_INCLUDE_DIR ${DEPENDENCIES_OUTPUT_DIR}/include)
    set(DEPENDENCIES_LIB_DIR ${DEPENDENCIES_OUTPUT_DIR}/lib/${CMAKE_BUILD_TYPE})
    set(DEPENDENCIES_BIN_DIR ${DEPENDENCIES_OUTPUT_DIR}/bin/${CMAKE_BUILD_TYPE})
endif ()

file(MAKE_DIRECTORY ${DEPENDENCIES_INCLUDE_DIR})
file(MAKE_DIRECTORY ${DEPENDENCIES_LIB_DIR})
file(MAKE_DIRECTORY ${DEPENDENCIES_BIN_DIR})

# On Linux some packages rely on pkgconfig to be found correctly. Therefore the environment variable
# needs to be set up accordingly. In addition when building depedencies which need to find other
# dependencies (e.g. MyGUI searching for OGRE) PKG_CONFIG_USE_CMAKE_PREFIX_PATH needs to be set to TRUE.
if (UNIX)
    set(ENV{PKG_CONFIG_PATH} "$ENV{PKG_CONFIG_PATH}:${DEPENDENCIES_LIB_DIR}/pkgconfig")
endif ()

# ------------------------------------------------------------------------------------------------#
#  Humanize
# ------------------------------------------------------------------------------------------------#

if (BUILD_HUMANIZE)
    ExternalProject_Add(
            Humanize
            DOWNLOAD_COMMAND ""
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/humanize"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/humanize/include/humanize ${DEPENDENCIES_OUTPUT_DIR}/include/humanize
    )
endif ()


# ------------------------------------------------------------------------------------------------#
#  LibCommuni
# ------------------------------------------------------------------------------------------------#

if (BUILD_LIBCOMMUNI)
    ExternalProject_Add(
            LibCommuni
            BUILD_IN_SOURCE 1
            #DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/libcommuni ${CMAKE_BINARY_DIR}/Source/LibCommuni
            #UPDATE_COMMAND ""
            GIT_REPOSITORY https://github.com/communi/libcommuni.git
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            #PATCH_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/libcommuni-patch.pro ${CMAKE_BINARY_DIR}/Source/LibCommuni/libcommuni.pro
            #SOURCE_DIR "${CMAKE_SOURCE_DIR}/libcommuni"
            CONFIGURE_COMMAND qmake
            CONFIG+=$<UPPER_CASE:$<CONFIG>>
            IRC_INSTALL_BINS=${DEPENDENCIES_OUTPUT_DIR}/bin
            IRC_INSTALL_HEADERS=${DEPENDENCIES_OUTPUT_DIR}/include
            IRC_INSTALL_LIBS=${DEPENDENCIES_OUTPUT_DIR}/lib
            IRC_INSTALL_FEATURES=${DEPENDENCIES_OUTPUT_DIR}/features
            BUILD_COMMAND $<IF:$<BOOL:WIN32>,nmake,make>
            INSTALL_COMMAND $<IF:$<BOOL:WIN32>,nmake,make> install
    )
endif ()

# ------------------------------------------------------------------------------------------------#
#  QtKeychain
# ------------------------------------------------------------------------------------------------#
if (BUILD_QTKEYCHAIN)
    ExternalProject_Add(
            QtKeychain
            DOWNLOAD_COMMAND ""
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/qtkeychain"
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_OUTPUT_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif ()


# ------------------------------------------------------------------------------------------------#
#  RapidJSON
# ------------------------------------------------------------------------------------------------#
if (BUILD_RAPIDJSON)
    ExternalProject_Add(
            RapidJSON
            DOWNLOAD_COMMAND ""
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/rapidjson"
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_OUTPUT_DIR}
            -DINCLUDE_INSTALL_DIR=${DEPENDENCIES_OUTPUT_DIR}/include # rapidjson will add "rapidjson" subdirectory
            -DRAPIDJSON_BUILD_DOC=NO
            -DRAPIDJSON_BUILD_EXAMPLES=NO
            -DRAPIDJSON_BUILD_TESTS=NO
            -DRAPIDJSON_BUILD_THIRDPARTY_GTEST=NO
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif ()

# ------------------------------------------------------------------------------------------------#
#  Serialize
# ------------------------------------------------------------------------------------------------#

if (BUILD_SERIALIZE)
    ExternalProject_Add(
            Serialize
            DOWNLOAD_COMMAND ""
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/serialize"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/serialize/include ${DEPENDENCIES_OUTPUT_DIR}/include
    )
endif ()

# ------------------------------------------------------------------------------------------------#
#  Signals
# ------------------------------------------------------------------------------------------------#

if (BUILD_SIGNALS)
    ExternalProject_Add(
            Signals
            DOWNLOAD_COMMAND ""
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/signals"
            CONFIGURE_COMMAND ""
            BUILD_COMMAND ""
            INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/signals/include ${DEPENDENCIES_OUTPUT_DIR}/include
    )
endif ()

# ------------------------------------------------------------------------------------------------#
#  WebSocketpp
# ------------------------------------------------------------------------------------------------#

if (BUILD_WEBSOCKETPP)
    ExternalProject_Add(
            WebSocketpp
            DOWNLOAD_COMMAND ""
            SOURCE_DIR "${CMAKE_SOURCE_DIR}/websocketpp"
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_OUTPUT_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif ()


