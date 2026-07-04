include_guard(GLOBAL)

include(options) # ensure options are defined if this module is used standalone

# Code coverage
if(CHATTERINO_GENERATE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
        add_link_options(-fprofile-instr-generate -fcoverage-mapping)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-coverage)
        add_link_options(-coverage)
    endif()
endif()

# Link-time optimization
if(CHATTERINO_LTO)
    if(CHATTERINO_FORCE_LTO)
        set(CHATTERINO_ENABLE_LTO ON)
        message(STATUS "LTO: Enabled (Force)")
    else()
        include(CheckIPOSupported)
        check_ipo_supported(RESULT CHATTERINO_ENABLE_LTO OUTPUT IPO_ERROR)
        message(STATUS "LTO: Enabled (Supported: ${CHATTERINO_ENABLE_LTO} - ${IPO_ERROR})")
    endif()
else()
    message(STATUS "LTO: Disabled")
endif()

# Sanitizers
if(CHATTERINO_SANITIZER_SUPPORT)
    list(APPEND CMAKE_MODULE_PATH
        "${PROJECT_SOURCE_DIR}/cmake/sanitizers-cmake/cmake"
    )
    find_package(Sanitizers REQUIRED)
endif()

set(USE_ALTERNATE_LINKER "" CACHE STRING "Use alternate linker. Leave empty for system default. CMake 3.29 users can use CMAKE_LINKER_TYPE instead.")
if(USE_ALTERNATE_LINKER)
    find_program(LINKER_EXECUTABLE ld.${USE_ALTERNATE_LINKER} ${USE_ALTERNATE_LINKER})
    if(LINKER_EXECUTABLE)
        message(STATUS "Using alternate linker '${USE_ALTERNATE_LINKER}'")
        add_link_options("-fuse-ld=${USE_ALTERNATE_LINKER}")
    else()
        message(FATAL_ERROR "Alternate linker ${USE_ALTERNATE_LINKER} could not be found under ld.${USE_ALTERNATE_LINKER} or ${USE_ALTERNATE_LINKER}")
    endif()
endif()
