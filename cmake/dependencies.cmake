include_guard(GLOBAL)

include(options) # ensure options are defined if this module is used standalone

find_package(Qt${MAJOR_QT_VERSION} REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Network
    Svg
    SvgWidgets
    Concurrent
)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
find_package(LRUCache REQUIRED)
find_package(MagicEnum REQUIRED)
find_package(Doxygen)
find_package(BoostCertify REQUIRED)

find_library(LIBRT rt)

if(BUILD_BENCHMARKS)
    # Google Benchmark
    find_package(benchmark REQUIRED)
endif()

if(USE_SYSTEM_LIBCOMMUNI)
    find_package(LibCommuni REQUIRED)
else()
    set(LIBCOMMUNI_ROOT_LIB_FOLDER "${PROJECT_SOURCE_DIR}/lib/libcommuni")
    if(NOT EXISTS "${LIBCOMMUNI_ROOT_LIB_FOLDER}/CMakeLists.txt")
        message(FATAL_ERROR "Failed to find libcommuni submodule in path \"${LIBCOMMUNI_ROOT_LIB_FOLDER}/CMakeLists.txt\". Be sure to run \"git submodule update --init --recursive\" to load the submodules.")
    endif()

    add_subdirectory("${LIBCOMMUNI_ROOT_LIB_FOLDER}" EXCLUDE_FROM_ALL)
endif()

if(CHATTERINO_ALLOW_PRIVATE_QT_API)
    # Only exists since 6.10 and prints a warning regarding private headers
    find_package(Qt${MAJOR_QT_VERSION} COMPONENTS GuiPrivate)
endif()

if(CHATTERINO_SPELLCHECK)
    # Prefer the package-manager config, fall back to pkg-config
    find_package(hunspell QUIET CONFIG)
    if(NOT hunspell_FOUND)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(hunspell REQUIRED IMPORTED_TARGET hunspell)
        add_library(hunspell::hunspell ALIAS PkgConfig::hunspell)
    endif()
endif()

if(CHATTERINO_PLUGINS)
    set(LUA_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/lib/lua/src")
    add_subdirectory(lib/lua)

    find_package(Sol2 REQUIRED)
endif()

if(BUILD_WITH_CRASHPAD)
    add_subdirectory("${PROJECT_SOURCE_DIR}/tools/crash-handler")
endif()
