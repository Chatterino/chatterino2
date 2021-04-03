include(FindPackageHandleStandardArgs)

find_path(PajladaSerialize_INCLUDE_DIR pajlada/serialize.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/serialize/include)

find_package_handle_standard_args(PajladaSerialize DEFAULT_MSG PajladaSerialize_INCLUDE_DIR)

if (PajladaSerialize_FOUND)
    add_library(Pajlada::Serialize INTERFACE IMPORTED)
    set_target_properties(Pajlada::Serialize PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PajladaSerialize_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(PajladaSerialize_INCLUDE_DIR)
