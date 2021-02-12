include(SourceFileUtils)

project(chatterino)

IF(MSVC)
    ADD_DEFINITIONS("/EHsc")
ENDIF(MSVC)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false "*.hpp" "*.cpp" "*.ui" "*.qrc")

generate_source_groups(${SOURCE_FILES})

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE CHATTERINO UNICODE USEWINSDK AB_CUSTOM_THEME AB_CUSTOM_SETTINGS IRC_STATIC IRC_NAMESPACE=Communi) #
target_compile_definitions(${PROJECT_NAME} PRIVATE CHATTERINO_GIT_HASH=\"XD\" CHATTERINO_GIT_RELEASE=\"XD\" CHATTERINO_GIT_COMMIT=\"XD\")

target_include_directories(${PROJECT_NAME} PRIVATE . "${RapidJSON_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME}
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Multimedia
        Qt5::Svg
        Qt5::Concurrent
        CONAN_PKG::boost
        CONAN_PKG::QtKeychain
        CONAN_PKG::rapidjson
        CONAN_PKG::openssl
        CONAN_PKG::communi
        CONAN_PKG::serialize
        CONAN_PKG::signals
        CONAN_PKG::WebSocketpp
        )

target_precompile_headers(${PROJECT_NAME} PRIVATE PrecompiledHeader.hpp)