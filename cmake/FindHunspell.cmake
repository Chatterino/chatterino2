find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(PC_HUNSPELL QUIET hunspell)
endif()

find_path(Hunspell_INCLUDE_DIR
    NAMES hunspell/hunspell.h
    HINTS ${PC_HUNSPELL_INCLUDEDIR} ${PC_HUNSPELL_INCLUDE_DIRS}
)

find_library(Hunspell_LIBRARY
    NAMES hunspell hunspell-1.9 hunspell-1.8 hunspell-1.7 hunspell-1.6 hunspell-1.5
    HINTS ${PC_HUNSPELL_LIBDIR} ${PC_HUNSPELL_LIBRARY_DIRS}
)

set(Hunspell_INCLUDE_DIRS ${Hunspell_INCLUDE_DIR})
set(Hunspell_LIBRARIES ${Hunspell_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hunspell
    DEFAULT_MSG
    Hunspell_LIBRARY Hunspell_INCLUDE_DIR)

mark_as_advanced(Hunspell_INCLUDE_DIR Hunspell_LIBRARY)

if (Hunspell_FOUND AND NOT TARGET hunspell::hunspell)
    add_library(hunspell::hunspell UNKNOWN IMPORTED)
    set_target_properties(hunspell::hunspell PROPERTIES
        IMPORTED_LOCATION "${Hunspell_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Hunspell_INCLUDE_DIR}"
    )
endif()
