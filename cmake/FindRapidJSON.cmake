if(USE_SYSTEM_RAPIDJSON)
    # Some distros' RapidJSON CMake config (e.g. Debian's rapidjson-dev) omits
    # the exported-targets file it include()s, so find_package() can fail
    # even though the (header-only) library is present. Fall back to a
    # find_path()-based interface target in that case.
    find_package(RapidJSON CONFIG QUIET)
    if(NOT TARGET RapidJSON)
        find_path(RapidJSON_INCLUDE_DIR NAMES rapidjson/rapidjson.h REQUIRED)
        add_library(RapidJSON INTERFACE IMPORTED)
        target_include_directories(RapidJSON INTERFACE ${RapidJSON_INCLUDE_DIR})
    endif()

    target_compile_definitions(RapidJSON
        INTERFACE
            RAPIDJSON_HAS_STDSTRING=1)

    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
    return()
endif()

add_library(RapidJSON INTERFACE IMPORTED)

target_include_directories(RapidJSON
    INTERFACE
        ${TR_THIRD_PARTY_SOURCE_DIR}/rapidjson/include)

target_compile_definitions(RapidJSON
    INTERFACE
        RAPIDJSON_HAS_STDSTRING=1)
