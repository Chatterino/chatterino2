include(FindPackageHandleStandardArgs)

find_path(MagicEnum_INCLUDE_DIR magic_enum/magic_enum.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/magic_enum/include)

find_package_handle_standard_args(MagicEnum DEFAULT_MSG MagicEnum_INCLUDE_DIR)

if (MagicEnum_FOUND)
    add_library(MagicEnum INTERFACE IMPORTED)
    set_target_properties(MagicEnum PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${MagicEnum_INCLUDE_DIR}"
        )
endif ()

mark_as_advanced(MagicEnum_INCLUDE_DIR)
