include(FindPackageHandleStandardArgs)

find_path(Signals_INCLUDE_DIR pajlada/signals/signal.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/signals/include)

find_package_handle_standard_args(Signals DEFAULT_MSG Signals_INCLUDE_DIR)

if (Signals_FOUND)
    add_library(Signals::Signals INTERFACE IMPORTED)
    set_target_properties(Signals::Signals PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Signals_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(Signals_INCLUDE_DIR)