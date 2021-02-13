if (EXISTS ${CMAKE_SOURCE_DIR}/lib/WinToast/src/wintoastlib.cpp)
    set(WinToast_FOUND TRUE)
    add_library(WinToast ${CMAKE_SOURCE_DIR}/lib/WinToast/src/wintoastlib.cpp)
    target_include_directories(WinToast PUBLIC "${CMAKE_SOURCE_DIR}/lib/WinToast/src/")
else ()
    set(WinToast_FOUND FALSE)
    message("WinToast submodule not found!")
endif ()




