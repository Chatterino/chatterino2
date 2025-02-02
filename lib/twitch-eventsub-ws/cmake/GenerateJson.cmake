function(generate_json_impls)
    set(_one_value_opts SRC_TARGET GEN_TARGET)
    cmake_parse_arguments(PARSE_ARGV 0 arg
        "" "${_one_value_opts}" ""
    )
    if (DEFINED arg_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "generate_json_impls: Missing ${arg_KEYWORDS_MISSING_VALUES}")
    endif()

    set(_venv_path "${CMAKE_CURRENT_BINARY_DIR}/eventsub-venv")
    if(WIN32)
        set(_venv_python_path "${_venv_path}/Scripts/python.exe")
        set(_venv_pip_path "${_venv_path}/Scripts/pip.exe")
    else()
        set(_venv_python_path "${_venv_path}/bin/python3")
        set(_venv_pip_path "${_venv_path}/bin/pip3")
    endif()

    cmake_path(SET _requirements NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/ast/requirements.txt")
    cmake_path(SET _gen_script NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/ast/generate-and-replace-dir.py")
    cmake_path(SET _search_dir NORMALIZE "${CMAKE_CURRENT_LIST_DIR}/include/twitch-eventsub-ws")

    get_target_property(_inc_dirs ${arg_SRC_TARGET} INCLUDE_DIRECTORIES)
    list(APPEND _inc_dirs ${Boost_INCLUDE_DIRS})
    list(APPEND _inc_dirs ${OPENSSL_INCLUDE_DIR})
    list(APPEND _inc_dirs ${Qt${MAJOR_QT_VERSION}Core_INCLUDE_DIRS})
    list(JOIN _inc_dirs ";" _inc_dir)

    find_package(Python3 COMPONENTS Interpreter)
    if(NOT Python3_FOUND)
        return()
    endif()

    add_custom_target(${arg_GEN_TARGET}_python_venv_create
        COMMENT "Creating python virtual environment"
        COMMAND "${Python3_EXECUTABLE}" -m venv "${_venv_path}"
        VERBATIM
    )

    add_custom_target(${arg_GEN_TARGET}_python_venv_install
        COMMENT "Installing python dependencies"
        COMMAND "${_venv_pip_path}" install -r "${_requirements}"
        DEPENDS "${_requirements}" ${arg_GEN_TARGET}_python_venv_create
        VERBATIM
    )

    add_custom_target(${arg_GEN_TARGET}
        COMMENT "Generating JSON deserializers"
        DEPENDS ${arg_GEN_TARGET}_python_venv_install
        COMMAND "${_venv_python_path}" "${_gen_script}" "${_search_dir}/messages" --includes "${_inc_dir}"
        COMMAND "${_venv_python_path}" "${_gen_script}" "${_search_dir}/payloads" --includes "${_inc_dir}"
        VERBATIM
    )
endfunction()
