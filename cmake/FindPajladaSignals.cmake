include(FindPackageHandleStandardArgs)

find_path(PajladaSignals_INCLUDE_DIR pajlada/signals/signal.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/signals/include)

find_package_handle_standard_args(PajladaSignals DEFAULT_MSG PajladaSignals_INCLUDE_DIR)

if (PajladaSignals_FOUND)
    add_library(Pajlada::Signals INTERFACE IMPORTED)
    set_target_properties(Pajlada::Signals PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${PajladaSignals_INCLUDE_DIR}"
            )
endif ()

mark_as_advanced(PajladaSignals_INCLUDE_DIR)
