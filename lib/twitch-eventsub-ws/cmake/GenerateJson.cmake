cmake_path(GET CMAKE_CURRENT_LIST_DIR PARENT_PATH _eventsub_lib_root)

# _validate_all_args_are_passed(<prefix> [<required-args>...])
function(_validate_all_args_are_parsed prefix)
    if(DEFINED ${prefix}_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown arguments: (${${prefix}_UNPARSED_ARGUMENTS})")
    endif()

    foreach(_name ${ARGN})
        if(NOT DEFINED ${prefix}_${_name})
            message(FATAL_ERROR "Missing required argument ${_name}")
        endif()
    endforeach()
endfunction()

function(_make_and_use_venv)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "" "VENV_PATH;REQUIREMENTS;OUT_PYTHON_EXE" ""
    )
    _validate_all_args_are_parsed(arg VENV_PATH REQUIREMENTS OUT_PYTHON_EXE)

    if(NOT Python3_EXECUTABLE)
        message(FATAL_ERROR "Python must be available to create a venv")
    endif()

    set(_venv_path "${CMAKE_CURRENT_BINARY_DIR}/eventsub-venv")

    message(STATUS "Creating venv in ${arg_VENV_PATH}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m venv "${arg_VENV_PATH}"
        RESULT_VARIABLE _venv_output
    )
    if(NOT _venv_output EQUAL 0)
        return()
    endif()

    # update the environment with VIRTUAL_ENV variable (mimic the activate script)
    set(ENV{VIRTUAL_ENV} "${arg_VENV_PATH}")
    # change the context of the search
    set(Python3_FIND_VIRTUALENV FIRST)
    # unset Python3_EXECUTABLE because it is also an input variable
    unset(Python3_EXECUTABLE)
    unset(Python3_FOUND)
    unset(Python3_Interpreter_FOUND)
    set(Python3_FIND_REGISTRY NEVER)
    # Launch a new search
    find_package(Python3 REQUIRED COMPONENTS Interpreter)

    message(STATUS "Installing requirements from ${arg_REQUIREMENTS}")
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -m pip install -r "${arg_REQUIREMENTS}"
        RESULT_VARIABLE _pip_output
        OUTPUT_QUIET
        ERROR_QUIET
    )
    if(NOT _pip_output EQUAL 0)
        return()
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${arg_REQUIREMENTS}")

    set(${arg_OUT_PYTHON_EXE} "${Python3_EXECUTABLE}" PARENT_SCOPE)
endfunction()

function(_setup_and_check_venv)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "" "INCLUDES;OUT_PYTHON_EXE;OUT_CHECK" ""
    )
    _validate_all_args_are_parsed(arg OUT_PYTHON_EXE OUT_CHECK INCLUDES)
    _make_and_use_venv(
        VENV_PATH "${CMAKE_CURRENT_BINARY_DIR}/eventsub-venv"
        REQUIREMENTS "${_eventsub_lib_root}/ast/requirements.txt"
        OUT_PYTHON_EXE _python3_path
    )
    if(NOT _python3_path)
        return() # venv failed
    endif()

    cmake_path(SET _check_script NORMALIZE "${_eventsub_lib_root}/ast/check-clang.py")
    execute_process(
        COMMAND "${_python3_path}" "${_check_script}" "${arg_INCLUDES}"
        OUTPUT_VARIABLE _check_output
        ERROR_VARIABLE _check_output
        RESULT_VARIABLE _check_result
        WORKING_DIRECTORY "${arg_BASE_DIRECTORY}"
    )

    set(${arg_OUT_PYTHON_EXE} "${_python3_path}" PARENT_SCOPE)
    if (NOT _check_result EQUAL 0)
        set(${arg_OUT_CHECK} "exit code: ${_check_result}\noutput:\n${_check_output}" PARENT_SCOPE)
    endif()
endfunction()

# generate_json_impls(
#   [OUTPUT_SOURCES <out-var>]
#   [BASE_DIRECTORY <path>]
#   [FORCE <force>]
#   [SKIP <skip>]
#   HEADERS <header>...
# )
#
# `OUTPUT_SOURCES`
#   A variable to store a list of the generated sources in.
# `BASE_DIRECTORY`
#   The directory to which paths in `HEADERS` are relative to. By default,
#   they're relative to the directory of the calling file.
# `FORCE`
#   Always generate sources and error if it's not possible.
# `SKIP`
#   Never try to generate sources.
# `HEADERS`
#   A list of header files for which implmenetations will be generated.
function(generate_json_impls)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "" "OUTPUT_SOURCES;BASE_DIRECTORY;FORCE;SKIP" "HEADERS"
    )
    _validate_all_args_are_parsed(arg HEADERS)
    if(NOT DEFINED arg_BASE_DIRECTORY)
        set(arg_BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")
    endif()

    cmake_path(SET _gen_script NORMALIZE "${_eventsub_lib_root}/ast/generate.py")

    # get includes
    get_target_property(_qt_inc_dirs Qt${MAJOR_QT_VERSION}::Core INCLUDE_DIRECTORIES)
    get_target_property(_qt_iinc_dirs Qt${MAJOR_QT_VERSION}::Core INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(_qt_isinc_dirs Qt${MAJOR_QT_VERSION}::Core INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)
    list(APPEND _inc_dirs ${_qt_inc_dirs} ${_qt_iinc_dirs} ${_qt_isinc_dirs})
    list(APPEND _inc_dirs "${_eventsub_lib_root}/include/twitch-eventsub-ws")
    list(APPEND _inc_dirs ${Boost_INCLUDE_DIRS})
    list(APPEND _inc_dirs ${OPENSSL_INCLUDE_DIR})
    list(JOIN _inc_dirs ";" _inc_dir)

    if(NOT arg_SKIP)
        if(arg_FORCE AND NOT Python3_EXECUTABLE)
            message(FATAL_ERROR "Missing python3")
        endif()

        # setup venv (<build>/lib/twitch-eventsub-ws/eventsub-venv)
        if(Python3_EXECUTABLE)
            _setup_and_check_venv(
                INCLUDES "${_inc_dirs}"
                OUT_PYTHON_EXE _python3_path
                OUT_CHECK _check_output
            )

            if(NOT _check_output AND _python3_path)
                set(_generation_supported On)
            else()
                set(_generation_supported Off)
                if (arg_FORCE)
                    message(FATAL_ERROR "Generation of JSON implementation not supported, because the parser can't parse a simple TU:\n${_check_output}")
                endif()
            endif()
        else()
            set(_generation_supported Off)
        endif()

        if(_generation_supported)
            message(STATUS "Generation of JSON implementation is supported")
        else()
            message(STATUS "Generation of JSON implementation is not supported (use FORCE_JSON_GENERATION=On to get more information)")
        endif()
    else()
        message(STATUS "Generation of JSON implementation is skipped because the SKIP_JSON_GENERATION option is On")
        set(_generation_supported Off)
    endif()

    # get all dependencies
    file(GLOB _gen_deps "${_eventsub_lib_root}/ast/lib/*.py")
    file(GLOB _ast_templates "${_eventsub_lib_root}/ast/lib/templates/*.tmpl")
    list(APPEND _gen_deps ${_ast_templates} "${_gen_script}")

    foreach(_header ${arg_HEADERS})
        # keep in sync with generate.py
        string(REGEX REPLACE \.hpp$ .inc _def_path "${_header}")
        string(REGEX REPLACE \.hpp$ .cpp _source_path "${_header}")
        string(REGEX REPLACE \.hpp$ .timestamp _timestamp_path "${_header}")
        string(REGEX REPLACE include[/\\]twitch-eventsub-ws[/\\] src/generated/ _source_path "${_source_path}")

        cmake_path(ABSOLUTE_PATH _timestamp_path BASE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/eventsub-deps")
        cmake_path(ABSOLUTE_PATH _header BASE_DIRECTORY "${arg_BASE_DIRECTORY}")
        cmake_path(ABSOLUTE_PATH _def_path BASE_DIRECTORY "${arg_BASE_DIRECTORY}")
        cmake_path(ABSOLUTE_PATH _source_path BASE_DIRECTORY "${arg_BASE_DIRECTORY}")
        list(APPEND _all_sources "${_source_path}")

        if(_generation_supported)
            add_custom_command(
                OUTPUT "${_timestamp_path}" "${_source_path}" "${_def_path}"
                DEPENDS ${_gen_deps} "${_header}"
                COMMENT "Generating implementation for ${_header}"
                COMMAND "${_python3_path}" "${_gen_script}" "${_header}" --includes "${_inc_dirs}" --timestamp "${_timestamp_path}"
                WORKING_DIRECTORY "${arg_BASE_DIRECTORY}"
                VERBATIM
            )
        endif()
    endforeach()

    if(arg_OUTPUT_SOURCES)
        set(${arg_OUTPUT_SOURCES} "${_all_sources}" PARENT_SCOPE)
    endif()
endfunction()
