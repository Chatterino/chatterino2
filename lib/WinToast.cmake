add_library(WinToast ${CMAKE_SOURCE_DIR}/lib/WinToast/src/wintoastlib.cpp)
target_include_directories(WinToast PUBLIC "${CMAKE_SOURCE_DIR}/lib/WinToast/src/")