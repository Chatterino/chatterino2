##
## PMM - The Package Manager Manager
## https://github.com/AnotherFoxGuy/pmm
##

## MIT License
##
## Copyright (c) 2018 vector-of-bool
## Copyright (c) 2019-2020 Edgar (Edgar@AnotherFoxGuy.com)
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.

# Bump this version to change what PMM version is downloaded
set(PMM_VERSION_INIT 1.9.6)

# Helpful macro to set a variable if it isn't already set
macro(_pmm_set_if_undef varname)
    if (NOT DEFINED "${varname}")
        set("${varname}" "${ARGN}")
    endif ()
endmacro()

## Variables used by this script
# The version:
_pmm_set_if_undef(PMM_VERSION ${PMM_VERSION_INIT})
# The base URL we download PMM from:
_pmm_set_if_undef(PMM_URL_BASE "https://anotherfoxguy.com/pmm/")
# The real URL we download from (Based on the version)
_pmm_set_if_undef(PMM_URL "${PMM_URL_BASE}/${PMM_VERSION}")
# The directory where we store our downloaded files
_pmm_set_if_undef(PMM_DIR_BASE "${CMAKE_BINARY_DIR}/_pmm")
_pmm_set_if_undef(PMM_DIR "${PMM_DIR_BASE}/${PMM_VERSION}")
# The location of the current file
_pmm_set_if_undef(PMM_MODULE "${CMAKE_CURRENT_LIST_FILE}")

# The file that we first download
set(_PMM_ENTRY_FILE "${PMM_DIR}/entry.cmake")

if (NOT EXISTS "${_PMM_ENTRY_FILE}" OR PMM_ALWAYS_DOWNLOAD)
    file(
            DOWNLOAD "${PMM_URL}/entry.cmake"
            "${_PMM_ENTRY_FILE}.tmp"
            STATUS pair
    )
    list(GET pair 0 rc)
    list(GET pair 1 msg)
    if (rc)
        message(FATAL_ERROR "Failed to download PMM entry file: ${msg}")
    endif ()
    file(RENAME "${_PMM_ENTRY_FILE}.tmp" "${_PMM_ENTRY_FILE}")
endif ()

# ^^^ DO NOT CHANGE THIS LINE vvv
set(_PMM_BOOTSTRAP_VERSION 2)
# ^^^ DO NOT CHANGE THIS LINE ^^^

include("${_PMM_ENTRY_FILE}")
