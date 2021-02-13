include(FindPackageHandleStandardArgs)

find_path(RapidJSON_INCLUDE_DIRS rapidjson/rapidjson.h HINTS ${CMAKE_SOURCE_DIR}/lib/rapidjson/include)

find_package_handle_standard_args(Rapidjson DEFAULT_MSG RapidJSON_INCLUDE_DIRS)

if (Rapidjson_FOUND)
    add_library(Rapidjson::Rapidjson INTERFACE IMPORTED)
    set_target_properties(Rapidjson::Rapidjson PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${RapidJSON_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(RapidJSON_INCLUDE_DIRS)