find_package(libavif)

if (libavif_FOUND)
    set(kimageformats_SRC ${CMAKE_SOURCE_DIR}/lib/kimageformats/src/imageformats)

    add_library(kimageformats STATIC ${kimageformats_SRC}/avif.cpp)
    set_target_properties(kimageformats PROPERTIES AUTOMOC ON)
    target_link_libraries(kimageformats PRIVATE Qt${MAJOR_QT_VERSON}::Gui)

    if (WIN32 OR NOT CHATTERINO_STATIC_AVIF)
        target_link_libraries(kimageformats PRIVATE avif)
    else()
        # See https://github.com/desktop-app/cmake_helpers/blob/af968dc8eab6bde381ad62ef6a516bdfccb7d038/target_link_static_libraries.cmake
        find_library(static_lib_avif libavif.a)
        if (${static_lib_avif} STREQUAL static_lib_avif-NOTFOUND)
            message(FATAL_ERROR "Could not find static library libavif.a. Chatterino will always statically link to libavif. You can link to the shared libavif by setting CHATTERINO_STATIC_AVIF to OFF in CMake. You can disable the AVIF plugin by defining CHATTERINO_NO_AVIF_PLUGIN in CMake.")
        endif()
        target_include_directories(kimageformats PRIVATE ${libavif_INCLUDE_DIR})
        target_link_libraries(kimageformats PRIVATE ${static_lib_avif})
    endif()

    target_compile_definitions(kimageformats PRIVATE QT_STATICPLUGIN)
endif()
