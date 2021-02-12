# The MIT License (MIT)
# 
# Copyright (c) 2016 Fabian Killus
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
# associated documentation files (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge, publish, distribute,
# sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all copies or
# substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
# NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# ================================================================================================ #
#  EXPAND_FILE_EXTENSIONS( RESULT <file_pattern>...                                                #
#                                                                                                  #
# This function expands filenames specified with multiple extensions into separate filenames for
# each provided extension. For example
#
#   path/to/file.{hpp,cpp}
#   path/to/definitions.h
#
# is transformed into
#
#   path/to/file.h
#   path/to/file.cpp
#   path/to/definitions.h
#
# Intention: For the purpose of building only source files are actually required to be specified.
# However, it often makes sense to additionally provide accompanying header files if an IDE such
# as Visual Studio is used (otherwise the header files are not shown in the IDE). The downside is
# that it becomes annoying to explicitely specify both (header and source) files. This helper
# function allows to specify this information in a compact manner.

function(expand_file_extensions RESULT)

    foreach (entry IN LISTS ARGN)
        # Check for file extension pattern
        string(REGEX MATCH "\\.{((hpp|cpp),)+(hpp|cpp)}$" EXT_PATTERN ${entry})
        if (NOT EXT_PATTERN)
            list(APPEND EXPANDED_SOURCES ${entry})
        else ()
            string(LENGTH ${entry} ENTRY_LEN)
            string(LENGTH ${EXT_PATTERN} EXT_LEN)
            math(EXPR BASE_LEN "${ENTRY_LEN}-${EXT_LEN}")

            # Obtain basename without extension pattern (e.g. "path/to/file")
            string(SUBSTRING ${entry} 0 ${BASE_LEN} BASENAME)

            # Convert extension pattern to list (e.g. ".{hpp,cpp}" -> "hpp;cpp")
            math(EXPR EXT_LEN "${EXT_LEN}-3")
            string(SUBSTRING ${EXT_PATTERN} 2 ${EXT_LEN} EXT_PATTERN)
            string(REPLACE "," ";" EXT_PATTERN ${EXT_PATTERN})

            # Append concatenated entries for each extension to expanded sources
            foreach (e IN LISTS EXT_PATTERN)
                list(APPEND EXPANDED_SOURCES ${BASENAME}.${e})
            endforeach ()
        endif ()
    endforeach ()

    set(${RESULT} ${EXPANDED_SOURCES} PARENT_SCOPE)
endfunction()


# ================================================================================================ #
#  GENERATE_SOURCE_GROUPS( SOURCE_FILES )                                                          #
#                                                                                                  #
# In CMake the source_group() command can be used to provide guidance for IDEs such as Visual
# Studio or XCode how to organize source files.
# This function takes a list of source files and creates sensible source groups based on the
# path provided for each file. In addition header and source files are sorted into separate super
# groups. For example:
#
#   path/to/file.cpp
#   path/to/file.h
#
# might be displayed in an IDE somewhat like this
#
#   |-Header Files
#   ||-path
#   | |-to
#   |  |-file.h
#   |-Source Files
#   ||-path
#   | |-to
#   |  |-file.cpp
#

function(generate_source_groups)

    set(SOURCES_GROUP "Source Files")
    set(HEADERS_GROUP "Header Files")

    foreach (f IN LISTS ARGN)
        # Split filename into its components
        get_filename_component(filename ${f} NAME)
        get_filename_component(filedir ${f} DIRECTORY)
				
        # Extract group name from directory path
        if (filedir AND NOT ARG_FLAT_STRUCTURE)
			string(REPLACE "${CMAKE_SOURCE_DIR}/" "" tmp_dir ${filedir})	
			string(REPLACE "/" "\\\\" groupdir ${tmp_dir})
        else ()
            set(groupdir "")
        endif ()
        list(APPEND GROUPDIRS ${groupdir})

        # sort according to filetype
        if (${filename} MATCHES ".*\\.(c|cc|cpp|cxx)")
            list(APPEND ${groupdir}_SOURCES ${f})
        elseif (${filename} MATCHES ".*\\.(h|hpp|hxx|txx)")
            list(APPEND ${groupdir}_HEADERS ${f})
        endif ()
    endforeach ()

    list(REMOVE_DUPLICATES GROUPDIRS)

    foreach (groupdir IN LISTS GROUPDIRS)
        source_group("${SOURCES_GROUP}\\${groupdir}" FILES ${${groupdir}_SOURCES})
        source_group("${HEADERS_GROUP}\\${groupdir}" FILES ${${groupdir}_HEADERS})
    endforeach ()

endfunction()

