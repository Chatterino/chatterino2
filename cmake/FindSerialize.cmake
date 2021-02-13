include(FindPackageHandleStandardArgs)

find_path(Serialize_INCLUDE_DIR pajlada/serialize.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/serialize/include)

find_package_handle_standard_args(Serialize DEFAULT_MSG Serialize_INCLUDE_DIR)

if (Serialize_FOUND)
    add_library(Serialize::Serialize INTERFACE IMPORTED)
    set_target_properties(Serialize::Serialize PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Serialize_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(Serialize_INCLUDE_DIR)