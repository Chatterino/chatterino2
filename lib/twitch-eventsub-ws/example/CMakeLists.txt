cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0087 NEW) # evaluates generator expressions in `install(CODE/SCRIPT)`
cmake_policy(SET CMP0091 NEW) # select MSVC runtime library through `CMAKE_MSVC_RUNTIME_LIBRARY`

project(twitch-eventsub-ws-example VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME} main.cpp)

add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/.." twitch-eventsub-ws)

find_library(LIBRT rt)
find_package(Threads REQUIRED)

target_link_libraries(${PROJECT_NAME}
        PUBLIC

        Threads::Threads
        twitch-eventsub-ws
        )

# Set the output of TARGET to be 
# - CMAKE_BIN_DIR/lib for libraries
# - CMAKE_BIN_DIR/bin for BINARIES
# an additional argument specifies the subdirectory.
function(set_target_directory_hierarchy TARGET)
    set_target_properties(${TARGET}
        PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ARGV1}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/${ARGV1}"
    )
endfunction()

set_target_directory_hierarchy(${PROJECT_NAME})

if (MSVC)
    target_compile_options(${PROJECT_NAME} PUBLIC /EHsc /bigobj)
endif ()

if (LIBRT)
    target_link_libraries(${PROJECT_NAME}
        PUBLIC
        ${LIBRT}
        )
endif ()
