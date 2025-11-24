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

# If invoked by `cmake -DC2_GIT_PRINT_VERSION=On -P`, we only print the version.
get_cmake_property(_cmake_role CMAKE_ROLE)
if(_cmake_role STREQUAL "SCRIPT" AND C2_GIT_PRINT_VERSION)
    set(_c2_git_find_package_args QUIET)
    set(_c2_git_print_version ON)
else()
    set(_c2_git_find_package_args "")
    set(_c2_git_print_version OFF)
endif()

find_package(Git ${_c2_git_find_package_args})

set(GIT_HASH "GIT-REPOSITORY-NOT-FOUND")
set(GIT_COMMIT "GIT-REPOSITORY-NOT-FOUND")
set(GIT_RELEASE "${PROJECT_VERSION}")
set(GIT_MODIFIED 0)

if(DEFINED ENV{CHATTERINO_SKIP_GIT_GEN})
    return()
endif()

if(GIT_EXECUTABLE)
    function(run_git OUTPUT_VAR)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" ${ARGN}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            RESULT_VARIABLE _cmd_result
            OUTPUT_VARIABLE _cmd_out
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        set(${OUTPUT_VAR} ${_cmd_out} PARENT_SCOPE)
        set(${OUTPUT_VAR}_result ${_cmd_result} PARENT_SCOPE)
    endfunction()

    run_git(_git_repo_found rev-parse --is-inside-work-tree)
    if(_git_repo_found_result)
        set(GIT_REPOSITORY_FOUND 0)
    else()
        set(GIT_REPOSITORY_FOUND 1)
    endif()

    if(GIT_REPOSITORY_FOUND)
        run_git(GIT_HASH rev-parse --short HEAD)
        run_git(GIT_COMMIT rev-parse HEAD)
        run_git(GIT_MODIFIED_OUTPUT status --porcelain -z)
        # Any release e.g. 2.3.4
        run_git(GIT_RELEASE describe --tags --abbrev=0 --match v*)
        # The commit of the release
        run_git(_git_release_commit rev-list -n 1 "${GIT_RELEASE}")

        run_git(_git_upstream remote get-url upstream)
        if(_git_upstream_result)
            set(_git_preferred_remote upstream)
        else()
            set(_git_preferred_remote origin)
        endif()

        # Get the last commit from master in our current branch (if this exists).
        run_git(_git_merge_base merge-base HEAD ${_git_preferred_remote}/master)
        if(_git_merge_base_result EQUAL 0)
            # Get commits from master that are in our branch as well since the tag
            run_git(GIT_NIGHTLY_COMMITS rev-list "${_git_release_commit}...${_git_merge_base}" "^${_git_release_commit}" --count)
            # All commits since the tag in our branch
            run_git(_total_commits rev-list "${_git_release_commit}...HEAD" "^${_git_release_commit}" --count)
            math(EXPR GIT_FORK_COMMITS "${_total_commits} - ${GIT_NIGHTLY_COMMITS}")
        endif()
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

if(DEFINED ENV{GIT_NIGHTLY_COMMITS})
    set(GIT_NIGHTLY_COMMITS "$ENV{GIT_NIGHTLY_COMMITS}")
endif()

if(DEFINED ENV{GIT_FORK_COMMITS})
    set(GIT_FORK_COMMITS "$ENV{GIT_FORK_COMMITS}")
endif()

if(NOT GIT_NIGHTLY_COMMITS MATCHES "^[0-9]+$")
    set(GIT_NIGHTLY_COMMITS 0)
endif()
if(NOT GIT_FORK_COMMITS MATCHES "^[0-9]+$")
    set(GIT_FORK_COMMITS 0)
endif()

set(_version_msg "${GIT_RELEASE}")
if (NOT GIT_NIGHTLY_COMMITS EQUAL 0)
    set(_version_msg "${_version_msg}-${GIT_NIGHTLY_COMMITS}")
endif()
if (NOT GIT_FORK_COMMITS EQUAL 0)
    set(_version_msg "${_version_msg}+${GIT_FORK_COMMITS}")
endif()

if (_c2_git_print_version)
    # print to stdout instead of stderr
    execute_process(COMMAND ${CMAKE_COMMAND} -E echo "${_version_msg}")
else()
    message(STATUS "Injected git values: ${GIT_COMMIT} (${_version_msg}) modified: ${GIT_MODIFIED}")
endif()
