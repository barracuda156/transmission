if(USE_SYSTEM_FAST_FLOAT)
    find_package(FastFloat CONFIG REQUIRED)
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
    return()
endif()

add_library(FastFloat::fast_float INTERFACE IMPORTED)

target_include_directories(FastFloat::fast_float
    INTERFACE
        ${TR_THIRD_PARTY_SOURCE_DIR}/fast_float/include)
