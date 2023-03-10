# This script will set the following variables:
# GIT_HASH
#   If the git binary is found and the git work tree is intact, GIT_HASH is worked out using the `git rev-parse --short HEAD` command
#   The value of GIT_HASH can be overriden by defining the GIT_HASH environment variable
# GIT_COMMIT
#   If the git binary is found and the git work tree is intact, GIT_COMMIT is worked out using the `git rev-parse HEAD` command
#   The value of GIT_COMMIT can be overriden by defining the GIT_COMMIT environment variable
# GIT_RELEASE
#   If the git binary is found and the git work tree is intact, GIT_RELEASE is worked out using the `git describe` command
#   The value of GIT_RELEASE can be overriden by defining the GIT_RELEASE environment variable
# GIT_MODIFIED
#   If the git binary is found and the git work tree is intact, GIT_MODIFIED is worked out by checking if output of `git status --porcelain -z` command is empty
#   The value of GIT_MODIFIED cannot be overriden

find_package(Git)

set(GIT_HASH "GIT-REPOSITORY-NOT-FOUND")
set(GIT_COMMIT "GIT-REPOSITORY-NOT-FOUND")
set(GIT_RELEASE "${PROJECT_VERSION}")
set(GIT_MODIFIED 0)

if(DEFINED ENV{CHATTERINO_SKIP_GIT_GEN})
    return()
endif()

if(GIT_EXECUTABLE)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE GIT_REPOSITORY_NOT_FOUND
        OUTPUT_QUIET
        ERROR_QUIET
    )

    if(GIT_REPOSITORY_NOT_FOUND)
        set(GIT_REPOSITORY_FOUND 0)
    else()
        set(GIT_REPOSITORY_FOUND 1)
    endif()

    if(GIT_REPOSITORY_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_RELEASE
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        execute_process(
            COMMAND ${GIT_EXECUTABLE} status --porcelain -z
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_MODIFIED_OUTPUT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
    endif(GIT_REPOSITORY_FOUND)
endif(GIT_EXECUTABLE)

if(GIT_MODIFIED_OUTPUT)
    if(DEFINED ENV{CHATTERINO_REQUIRE_CLEAN_GIT})
        message(STATUS "git status --porcelain -z\n${GIT_MODIFIED_OUTPUT}")
        message(FATAL_ERROR "Git repository was expected to be clean, but modifications were found!")
    endif()

    set(GIT_MODIFIED 1)
endif()

if(DEFINED ENV{GIT_HASH})
    set(GIT_HASH "$ENV{GIT_HASH}")
endif()

if(DEFINED ENV{GIT_COMMIT})
    set(GIT_COMMIT "$ENV{GIT_COMMIT}")
endif()

if(DEFINED ENV{GIT_RELEASE})
    set(GIT_RELEASE "$ENV{GIT_RELEASE}")
endif()

message(STATUS "Injected git values: ${GIT_COMMIT} (${GIT_RELEASE}) modified: ${GIT_MODIFIED}")
