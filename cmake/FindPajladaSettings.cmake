include(FindPackageHandleStandardArgs)

find_path(PajladaSettings_INCLUDE_DIR pajlada/settings.hpp)
find_library(PajladaSettings_LIBRARY PajladaSettings)

find_package_handle_standard_args(PajladaSettings DEFAULT_MSG PajladaSettings_INCLUDE_DIR PajladaSettings_LIBRARY)

if (PajladaSettings_FOUND)
    add_library(Pajlada::Settings INTERFACE IMPORTED)
    set_target_properties(Pajlada::Settings PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PajladaSettings_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${PajladaSettings_LIBRARY}"
            )
endif ()

mark_as_advanced(PajladaSettings_INCLUDE_DIR PajladaSettings_LIBRARY)
