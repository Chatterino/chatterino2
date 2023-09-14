include(${CMAKE_SOURCE_DIR}/cmake/AVIF.cmake)

add_library(chatterino-qt-plugins OBJECT ${CMAKE_CURRENT_LIST_DIR}/QtPlugins.cpp)
target_link_libraries(chatterino-qt-plugins PRIVATE Qt${MAJOR_QT_VERSION}::Core Qt${MAJOR_QT_VERSION}::Gui)
target_compile_definitions(chatterino-qt-plugins PRIVATE QT_STATICPLUGIN)

if (libavif_FOUND AND NOT APPLE)
    message(STATUS "Adding AVIF plugin")
    target_compile_definitions(chatterino-qt-plugins PRIVATE WITH_AVIF)
    target_link_libraries(chatterino-qt-plugins PRIVATE kimageformats)
endif()
