include(FindPackageHandleStandardArgs)

find_path(Serialize_INCLUDE_DIR pajlada/serialize.hpp)

find_package_handle_standard_args(Serialize REQUIRED_VARS Serialize_INCLUDE_DIR)

if (Serialize_FOUND)
    add_library(Serialize::Serialize INTERFACE IMPORTED)
    set_target_properties(Serialize::Serialize PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Serialize_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(Serialize_INCLUDE_DIR)