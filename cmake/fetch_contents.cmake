include_guard(GLOBAL)

# rapidjson
FetchContent_Declare(
    RapidJSON
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/rapidjson
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS
)
set(RAPIDJSON_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(RAPIDJSON_BUILD_TESTS OFF CACHE INTERNAL "")

# pajlada/signals
FetchContent_Declare(
    PajladaSignals
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/signals
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS 0.1.3 CONFIG
)

# pajlada/serialize
FetchContent_Declare(
    PajladaSerialize
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/serialize
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS 0.3.0 CONFIG
)

# pajlada/settings
FetchContent_Declare(
    PajladaSettings
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/lib/settings
    EXCLUDE_FROM_ALL
    FIND_PACKAGE_ARGS 0.5.1 CONFIG
)

# Make available the fetched contents
FetchContent_MakeAvailable(RapidJSON PajladaSignals PajladaSerialize PajladaSettings)
