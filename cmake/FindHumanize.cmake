include(FindPackageHandleStandardArgs)

find_path(Humanize_INCLUDE_DIR humanize/format_string.hpp)

find_package_handle_standard_args(Humanize REQUIRED_VARS Humanize_INCLUDE_DIR)

if (Humanize_FOUND)
    add_library(Humanize::Humanize INTERFACE IMPORTED)
    set_target_properties(Humanize::Humanize PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Humanize_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(Humanize_INCLUDE_DIR)