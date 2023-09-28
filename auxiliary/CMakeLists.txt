if(BUILD_WITH_CRASHPAD)
    if(WIN32)
        # crash-handler is only used on Windows for now.
        add_subdirectory(crash-handler)
    else()
        message(WARNING "Crashpad was enabled but the custom crash handler is Windows only!")
    endif()
endif()
