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

find_package(Git)

set(GIT_HASH "GIT-REPOSITORY-NOT-FOUND")
set(GIT_COMMIT "GIT-REPOSITORY-NOT-FOUND")
set(GIT_RELEASE "${PROJECT_VERSION}")

if (GIT_EXECUTABLE)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-parse --is-inside-work-tree
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE GIT_REPOSITORY_NOT_FOUND
            ERROR_QUIET
    )
    if (GIT_REPOSITORY_NOT_FOUND)
        set(GIT_REPOSITORY_FOUND 0)
    else ()
        set(GIT_REPOSITORY_FOUND 1)
    endif()

    if (GIT_REPOSITORY_FOUND)
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
    endif (GIT_REPOSITORY_FOUND)
endif (GIT_EXECUTABLE)

if (DEFINED ENV{GIT_HASH})
    set(GIT_HASH "$ENV{GIT_HASH}")
endif ()
if (DEFINED ENV{GIT_COMMIT})
    set(GIT_COMMIT "$ENV{GIT_COMMIT}")
endif ()
if (DEFINED ENV{GIT_RELEASE})
    set(GIT_RELEASE "$ENV{GIT_RELEASE}")
endif ()

message(STATUS "Injected git values: ${GIT_COMMIT} (${GIT_RELEASE}) ${GIT_HASH}")
