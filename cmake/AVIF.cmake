find_package(libavif)

if (libavif_FOUND)
    set(kimageformats_SRC ${CMAKE_SOURCE_DIR}/lib/kimageformats/src/imageformats)

    add_library(kimageformats STATIC ${kimageformats_SRC}/avif.cpp)
    set_target_properties(kimageformats PROPERTIES AUTOMOC ON)
    target_link_libraries(kimageformats PRIVATE Qt${MAJOR_QT_VERSON}::Gui)

    target_link_libraries(kimageformats PRIVATE avif)
    target_compile_definitions(kimageformats PRIVATE QT_STATICPLUGIN)
endif()
