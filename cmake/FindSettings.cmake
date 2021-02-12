include(FindPackageHandleStandardArgs)

find_path(Settings_INCLUDE_DIR pajlada/settings.hpp)
find_library(Settings_LIBRARY PajladaSettings)

find_package_handle_standard_args(Settings REQUIRED_VARS Settings_INCLUDE_DIR Settings_LIBRARY)

if (Settings_FOUND)
    add_library(Settings::Settings INTERFACE IMPORTED)
    set_target_properties(Settings::Settings PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Settings_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${Settings_LIBRARY}"
            )
endif ()

mark_as_advanced(Settings_INCLUDE_DIR Settings_LIBRARY)