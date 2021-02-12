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
option(BUILD_QTKEYCHAIN "Build QtKeychain" ON)
option(BUILD_RAPIDJSON "Build RapidJSON library" ON)
option(BUILD_SERIALIZE "Build Serialize library" ON)
option(BUILD_SETTINGS "Build Settings library" ON)
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
#  LibCommuni
# ------------------------------------------------------------------------------------------------#

if (BUILD_LIBCOMMUNI)
    ExternalProject_Add(
            LibCommuni
            GIT_REPOSITORY https://github.com/AnotherFoxGuy/libcommuni-cmake.git
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_OUTPUT_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif ()

# ------------------------------------------------------------------------------------------------#
#  QtKeychain
# ------------------------------------------------------------------------------------------------#
if (BUILD_QTKEYCHAIN)
    ExternalProject_Add(
            QtKeychain
            GIT_REPOSITORY https://github.com/Chatterino/qtkeychain.git
            GIT_TAG 308ea7e709113dc277be1653fe2044bb20236836
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
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
#  Settings
# ------------------------------------------------------------------------------------------------#

if (BUILD_SETTINGS)
    ExternalProject_Add(
            Settings
            GIT_REPOSITORY https://github.com/AnotherFoxGuy/settings.git
            GIT_SUBMODULES_RECURSE TRUE
            GIT_TAG cmake-fix
            GIT_PROGRESS TRUE
            CMAKE_ARGS
            -DBUILD_TESTS=OFF
            -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_OUTPUT_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif ()

# ------------------------------------------------------------------------------------------------#
#  WebSocketpp
# ------------------------------------------------------------------------------------------------#

if (BUILD_WEBSOCKETPP)
    ExternalProject_Add(
            WebSocketpp
            GIT_REPOSITORY https://github.com/ziocleto/websocketpp.git
            GIT_TAG 1e0138c7ccedc6be859d28270ccd6195f235a94e
            GIT_SHALLOW TRUE
            GIT_PROGRESS TRUE
            CMAKE_ARGS
            -DCMAKE_INSTALL_PREFIX=${DEPENDENCIES_OUTPUT_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
            -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    )
endif ()


