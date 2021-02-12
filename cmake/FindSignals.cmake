include(FindPackageHandleStandardArgs)

find_path(Signals_INCLUDE_DIR signals/signal.hpp PATH_SUFFIXES pajlada)

find_package_handle_standard_args(Signals REQUIRED_VARS Signals_INCLUDE_DIR)

if (Signals_FOUND)
    add_library(Signals::Signals INTERFACE IMPORTED)
    set_target_properties(Signals::Signals PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Signals_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(Signals_INCLUDE_DIR)