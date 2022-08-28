find_path(IrcCore_INCLUDE_DIR irc.h PATH_SUFFIXES IrcCore)
find_library(IrcCore_LIBRARY Core)

find_path(IrcModel_INCLUDE_DIR ircmodel.h PATH_SUFFIXES IrcModel)
find_library(IrcModel_LIBRARY Model)

find_path(IrcUtil_INCLUDE_DIR ircutil.h PATH_SUFFIXES IrcUtil)
find_library(IrcUtil_LIBRARY Util)

set(LibCommuni_INCLUDE_DIRS ${IrcCore_INCLUDE_DIR} ${IrcModel_INCLUDE_DIR} ${IrcUtil_INCLUDE_DIR})
set(LibCommuni_LIBRARIES ${IrcCore_LIBRARY} ${IrcModel_LIBRARY} ${IrcUtil_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibCommuni
        DEFAULT_MSG IrcCore_LIBRARY IrcModel_LIBRARY IrcUtil_LIBRARY IrcCore_INCLUDE_DIR IrcModel_INCLUDE_DIR IrcUtil_INCLUDE_DIR
        )

if (LibCommuni_FOUND)
    add_library(LibCommuni::LibCommuni INTERFACE IMPORTED)
    set_target_properties(LibCommuni::LibCommuni PROPERTIES
            INTERFACE_LINK_LIBRARIES "${LibCommuni_LIBRARIES}"
            INTERFACE_INCLUDE_DIRECTORIES "${LibCommuni_INCLUDE_DIRS}"
            )
endif ()

mark_as_advanced(LibCommuni_INCLUDE_DIRS LibCommuni_LIBRARIES
        IrcCore_LIBRARY IrcModel_LIBRARY IrcUtil_LIBRARY
        IrcCore_INCLUDE_DIR IrcModel_INCLUDE_DIR IrcUtil_INCLUDE_DIR)
