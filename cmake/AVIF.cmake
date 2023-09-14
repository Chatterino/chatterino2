find_package(libavif)

if (libavif_FOUND)
    set(kimageformats_SRC ${CMAKE_SOURCE_DIR}/lib/kimageformats/src/imageformats)

    add_library(kimageformats STATIC ${kimageformats_SRC}/avif.cpp)
    set_target_properties(kimageformats PROPERTIES AUTOMOC ON)
    target_link_libraries(kimageformats PRIVATE Qt${MAJOR_QT_VERSON}::Gui)

    if (WIN32)
        target_link_libraries(kimageformats PRIVATE avif)
    else()
        get_target_property(_avif_dir avif INTERFACE_INCLUDE_DIRECTORIES)
        target_include_directories(kimageformats PRIVATE ${_avif_dir})
        # See https://github.com/desktop-app/cmake_helpers/blob/491a7fdbae6629dd06a53fc17ac06e6827f4b295/target_link_static_libraries.cmake#L16
        target_link_libraries(kimageformats PRIVATE "-Wl,--push-state,-Bstatic,-lavif,--pop-state")
    endif()

    target_compile_definitions(kimageformats PRIVATE QT_STATICPLUGIN)
endif()
