project(chatterino-mocks)

add_library(chatterino-mocks INTERFACE)

target_include_directories(chatterino-mocks INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(${PROJECT_NAME} INTERFACE gmock)
