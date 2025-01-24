cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`
include(FeatureSummary)

list(APPEND CMAKE_MODULE_PATH
    "${CMAKE_SOURCE_DIR}/cmake"
    )

project(twitch-eventsub-ws VERSION 0.1.0)

# Find boost on the system
find_package(Boost REQUIRED OPTIONAL_COMPONENTS json)

# Find OpenSSL on the system
find_package(OpenSSL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)

include(cmake/GenerateJson.cmake)

generate_json_impls(SRC_TARGET twitch-eventsub-ws GEN_TARGET twitch-eventsub-ws-gen)

feature_summary(WHAT ALL)
