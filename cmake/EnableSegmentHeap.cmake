# SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
#
# SPDX-License-Identifier: CC0-1.0

if(NOT DEFINED ENV{VSINSTALLDIR})
    message(WARNING "Missing VSINSTALLDIR environment variable - not enabling segment heap.")
    return()
endif()

set(segment_heap_path "$ENV{VSINSTALLDIR}Common7/IDE/CommonExtensions/Microsoft/CMake/cmake/Microsoft/SegmentHeap.cmake")
if (NOT EXISTS "${segment_heap_path}")
    message(STATUS "Missing '${segment_heap_path}'. Segment heap will be disabled - consider updating Visual Studio.")
    return()
endif()

message(STATUS "Including segment heap script from '${segment_heap_path}'.")
include("${segment_heap_path}")
