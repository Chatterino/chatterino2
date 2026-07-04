include_guard(GLOBAL)

# Build options
option(BUILD_SHARED_LIBS "" OFF)
option(BUILD_APP "Build Chatterino" ON)
option(BUILD_TESTS "Build the tests for Chatterino" OFF)
option(BUILD_BENCHMARKS "Build the benchmarks for Chatterino" OFF)
option(BUILD_WITH_CRASHPAD "Build chatterino with crashpad" OFF)
option(BUILD_WITH_QT6 "Build with Qt6" ON)
option(BUILD_WITH_LIBNOTIFY "Build with libnotify" ON)

# Use system libraries options
option(USE_SYSTEM_PAJLADA_SETTINGS "Use system pajlada settings library" OFF)
option(USE_SYSTEM_LIBCOMMUNI "Use system communi library" OFF)
option(USE_SYSTEM_MINIAUDIO "Build Chatterino with your system miniaudio" OFF)
option(USE_PRECOMPILED_HEADERS "Use precompiled headers (Temporarily not supported on macOS)" ON)

# Chatterino options
option(CHATTERINO_GENERATE_COVERAGE "Generate coverage files" OFF)
option(CHATTERINO_LTO "Enable LTO for all targets" OFF)
option(CHATTERINO_PLUGINS "Enable ALPHA plugin support in Chatterino" ON)
option(CHATTERINO_USE_GDI_FONTENGINE "Use the legacy GDI fontengine instead of the new DirectWrite one on Windows (Qt 6.8.0 and later)" ON)
option(CHATTERINO_ALLOW_PRIVATE_QT_API "Allow uses of Qt's private API - when enabling this, Chatterino must use the EXACT Qt version it was compiled against" OFF)
option(CHATTERINO_SPELLCHECK "Enable spellchecking in Chatterino (requires Hunspell)" OFF)
option(CHATTERINO_SANITIZER_SUPPORT "Attempt to enable Sanitizer support on the test and app targets. Actual sanitizers can then be enabled with the SANITIZE_* options." OFF)
mark_as_advanced(CHATTERINO_SANITIZER_SUPPORT)
add_feature_info("Chatterino code sanitizer support" CHATTERINO_SANITIZER_SUPPORT "")
option(CHATTERINO_NIGHTLY_BUILD "Specifies whether this is a nightly build." OFF)
set(CHATTERINO_EXTRA_BUILD_STRING "" CACHE STRING "Provide an extra string to show in the about page under the Chatterino version. This string allows the use of Qt's HTML subset.")

# CMake doesn't detect LTO for some compilers (e.g. clang-cl: https://gitlab.kitware.com/cmake/cmake/-/issues/21635),
# so this skips the check and assumes LTO works.
cmake_dependent_option(CHATTERINO_FORCE_LTO "Skip check if LTO is supported" OFF CHATTERINO_LTO OFF)
option(CHATTERINO_FORCE_LTO "Don't check if LTO is supported" OFF)

option(CHATTERINO_UPDATER "Enable update checks" ON)
mark_as_advanced(CHATTERINO_UPDATER)
