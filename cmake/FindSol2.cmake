include(FindPackageHandleStandardArgs)

find_path(Sol2_INCLUDE_DIR sol/sol.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/sol2/include)

find_package_handle_standard_args(Sol2 DEFAULT_MSG Sol2_INCLUDE_DIR)

if (Sol2_FOUND)
    add_library(Sol2 INTERFACE IMPORTED)
    set_target_properties(Sol2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Sol2_INCLUDE_DIR}"
    )
    target_compile_definitions(Sol2 INTERFACE 
        SOL_ALL_SAFETIES_ON=1
        SOL_USING_CXX_LUA=1
        SOL_NO_NIL=0
    )
    target_link_libraries(Sol2 INTERFACE lua)
    add_library(sol2::sol2 ALIAS Sol2)
endif ()

mark_as_advanced(Sol2_INCLUDE_DIR)
