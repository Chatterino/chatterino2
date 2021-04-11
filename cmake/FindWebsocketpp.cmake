include(FindPackageHandleStandardArgs)

find_path(Websocketpp_INCLUDE_DIRS websocketpp/version.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/websocketpp)

find_package_handle_standard_args(Websocketpp DEFAULT_MSG Websocketpp_INCLUDE_DIRS)

if (Websocketpp_FOUND)
    add_library(websocketpp::websocketpp INTERFACE IMPORTED)
    set_target_properties(websocketpp::websocketpp PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${Websocketpp_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(Websocketpp_INCLUDE_DIRS)
