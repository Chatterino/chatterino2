include(FindPackageHandleStandardArgs)

find_path(LRUCache_INCLUDE_DIR lrucache/lrucache.hpp HINTS ${CMAKE_SOURCE_DIR}/lib/lrucache)

find_package_handle_standard_args(LRUCache DEFAULT_MSG LRUCache_INCLUDE_DIR)

if (LRUCache_FOUND)
    add_library(LRUCache INTERFACE IMPORTED)
    set_target_properties(LRUCache PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LRUCache_INCLUDE_DIR}"
        )
endif ()

mark_as_advanced(LRUCache_INCLUDE_DIR)
