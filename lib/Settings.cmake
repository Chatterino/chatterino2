project(PajladaSettings)

add_library(Settings ${CMAKE_SOURCE_DIR}/lib/settings/src/settings/settingdata.cpp
        ${CMAKE_SOURCE_DIR}/lib/settings/src/settings/settingmanager.cpp
        ${CMAKE_SOURCE_DIR}/lib/settings/src/settings/detail/realpath.cpp)

target_include_directories(Settings PRIVATE ${RapidJSON_INCLUDE_DIRS} ${Serialize_INCLUDE_DIR})
target_include_directories(Settings PUBLIC ${CMAKE_SOURCE_DIR}/lib/settings/include/)
target_link_libraries(Settings Serialize::Serialize)