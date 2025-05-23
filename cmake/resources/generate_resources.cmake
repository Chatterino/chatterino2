set(RES_DIR "${CMAKE_SOURCE_DIR}/resources")
# Note: files in this ignorelist should be relative to ${RES_DIR}
set(
        RES_IGNORED_FILES
        .gitignore
        qt.conf
        resources.qrc
        resources_autogenerated.qrc
        themes/ChatterinoTheme.schema.json
)
set(RES_EXCLUDE_FILTER ^raw)
set(RES_IMAGE_EXCLUDE_FILTER "^(buttons/(update|clearSearch)|avatars|icon|settings|raw)")

file(GLOB_RECURSE RES_ALL_FILES RELATIVE "${RES_DIR}" LIST_DIRECTORIES false CONFIGURE_DEPENDS "${RES_DIR}/*")
file(GLOB_RECURSE RES_IMAGE_FILES RELATIVE "${RES_DIR}" LIST_DIRECTORIES false CONFIGURE_DEPENDS "${RES_DIR}/*.png")

list(REMOVE_ITEM RES_ALL_FILES ${RES_IGNORED_FILES})
list(FILTER RES_ALL_FILES EXCLUDE REGEX ${RES_EXCLUDE_FILTER})
list(FILTER RES_IMAGE_FILES EXCLUDE REGEX ${RES_IMAGE_EXCLUDE_FILTER})

###############################
# Generate resources_autogenerated.qrc
###############################
set(WINDOWS_ARGUMENTS "@Invalid()") # empty QVariant() in QSettings
if (CHATTERINO_USE_GDI_FONTENGINE AND Qt${MAJOR_QT_VERSION}_VERSION VERSION_GREATER_EQUAL "6.8.0")
    message(STATUS "Using legacy GDI fontengine")
    set(WINDOWS_ARGUMENTS "fontengine=gdi")
endif ()

configure_file("${CMAKE_CURRENT_LIST_DIR}/qt.conf.in" "${CMAKE_BINARY_DIR}/autogen/qt.conf")
set(RES_QT_CONF_PATH "${CMAKE_BINARY_DIR}/autogen/qt.conf")

message(STATUS "Generating resources_autogenerated.qrc")
foreach (_file ${RES_ALL_FILES})
    list(APPEND RES_RESOURCES_CONTENT "    <file alias=\"${_file}\">${RES_DIR}/${_file}</file>")
endforeach ()
list(JOIN RES_RESOURCES_CONTENT "\n" RES_RESOURCES_CONTENT)
configure_file("${CMAKE_CURRENT_LIST_DIR}/resources_autogenerated.qrc.in" "${CMAKE_BINARY_DIR}/autogen/resources_autogenerated.qrc" @ONLY)
list(APPEND RES_AUTOGEN_FILES "${CMAKE_BINARY_DIR}/autogen/resources_autogenerated.qrc")

###############################
# Generate ResourcesAutogen.cpp
###############################
message(STATUS "Generating ResourcesAutogen.cpp")
foreach (_file ${RES_IMAGE_FILES})
    get_filename_component(_ext "${_file}" EXT)
    string(REPLACE "${_ext}" "" _var_name ${_file})
    string(REPLACE "/" "." _var_name ${_var_name})
    list(APPEND RES_SOURCE_CONTENT "    this->${_var_name} = QPixmap(\":/${_file}\")\;")
    list(APPEND RES_VAR_NAMES "${_var_name}")
endforeach ()
list(JOIN RES_SOURCE_CONTENT "\n" RES_SOURCE_CONTENT)
configure_file(${CMAKE_CURRENT_LIST_DIR}/ResourcesAutogen.cpp.in ${CMAKE_BINARY_DIR}/autogen/ResourcesAutogen.cpp @ONLY)

###############################
# Generate ResourcesAutogen.hpp
###############################
message(STATUS "Generating ResourcesAutogen.hpp")
set(_struct_list "")

foreach (_file ${RES_IMAGE_FILES})
    get_filename_component(_dir "${_file}" DIRECTORY)
    get_filename_component(_name "${_file}" NAME_WE)
    string(REPLACE "/" "_" _dir "${_dir}")
    if (NOT _dir)
        set(_dir "root")
    endif ()
    list(APPEND ${_dir} "${_name}")
    list(APPEND _struct_list "${_dir}")
endforeach ()

list(REMOVE_DUPLICATES _struct_list)

foreach (_str_name ${_struct_list})
    if (NOT "${_str_name}" STREQUAL "root")
        list(APPEND RES_HEADER_CONTENT "    struct {")
        set(_indent "        ")
    else ()
        set(_indent "    ")
    endif ()
    foreach (_name ${${_str_name}})
        list(APPEND RES_HEADER_CONTENT "${_indent}QPixmap ${_name}\;")
    endforeach ()
    if (NOT "${_str_name}" STREQUAL "root")
        list(APPEND RES_HEADER_CONTENT "    } ${_str_name}\;")
    endif ()
endforeach ()

list(JOIN RES_HEADER_CONTENT "\n" RES_HEADER_CONTENT)
configure_file(${CMAKE_CURRENT_LIST_DIR}/ResourcesAutogen.hpp.in ${CMAKE_BINARY_DIR}/autogen/ResourcesAutogen.hpp @ONLY)

if (WIN32)
    if (NOT PROJECT_VERSION_TWEAK)
        set(PROJECT_VERSION_TWEAK 0)
    endif()
    string(TIMESTAMP CURRENT_YEAR "%Y")
    configure_file(${CMAKE_CURRENT_LIST_DIR}/windows.rc.in ${CMAKE_BINARY_DIR}/autogen/windows.rc @ONLY)
    list(APPEND RES_AUTOGEN_FILES "${CMAKE_BINARY_DIR}/autogen/windows.rc")
endif ()

list(APPEND RES_AUTOGEN_FILES
        "${CMAKE_BINARY_DIR}/autogen/ResourcesAutogen.cpp"
        "${CMAKE_BINARY_DIR}/autogen/ResourcesAutogen.hpp"
        )
