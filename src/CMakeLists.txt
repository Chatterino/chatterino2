include(SourceFileUtils)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(Humanize REQUIRED)
#find_package(LibCommuni REQUIRED)
find_package(Qt5keychain REQUIRED)
find_package(Rapidjson REQUIRED)
find_package(Signals REQUIRED)
find_package(Serialize REQUIRED)
find_package(Websocketpp REQUIRED)

include(Settings)
include(LibCommuni)
include(WinToast)

project(chatterino)

IF(MSVC)
    ADD_DEFINITIONS("/EHsc")
ENDIF(MSVC)

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS LIST_DIRECTORIES false "*.hpp" "*.cpp" "*.ui" "*.qrc")

generate_source_groups(${SOURCE_FILES})

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE CHATTERINO UNICODE USEWINSDK AB_CUSTOM_THEME AB_CUSTOM_SETTINGS IRC_STATIC IRC_NAMESPACE=Communi) #
target_compile_definitions(${PROJECT_NAME} PRIVATE CHATTERINO_GIT_HASH=\"XD\" CHATTERINO_GIT_RELEASE=\"XD\" CHATTERINO_GIT_COMMIT=\"XD\")

target_include_directories(${PROJECT_NAME} PRIVATE . "${RapidJSON_INCLUDE_DIRS}")

target_link_libraries(${PROJECT_NAME} Settings WinToast qt5keychain)

target_link_libraries(${PROJECT_NAME}
        Qt5::Gui
        Qt5::Widgets
        Qt5::Network
        Qt5::Multimedia
        Qt5::Svg
        Qt5::Concurrent
        Boost::Boost
        OpenSSL::OpenSSL
        Humanize::Humanize
        LibCommuni::LibCommuni
        Serialize::Serialize
        Signals::Signals
        websocketpp::websocketpp
        )

target_precompile_headers(${PROJECT_NAME} PRIVATE PrecompiledHeader.hpp)