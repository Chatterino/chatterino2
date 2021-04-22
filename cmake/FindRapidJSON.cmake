include(FindPackageHandleStandardArgs)

find_path(RapidJSON_INCLUDE_DIRS rapidjson/rapidjson.h HINTS ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)

find_package_handle_standard_args(RapidJSON DEFAULT_MSG RapidJSON_INCLUDE_DIRS)

if (RapidJSON_FOUND)
    add_library(RapidJSON::RapidJSON INTERFACE IMPORTED)
    set_target_properties(RapidJSON::RapidJSON PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(RapidJSON_INCLUDE_DIRS)
