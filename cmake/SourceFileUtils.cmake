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

