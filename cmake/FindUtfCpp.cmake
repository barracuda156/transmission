if(USE_SYSTEM_UTFCPP)
    find_package(utf8cpp CONFIG REQUIRED)
    if(NOT TARGET utf8::cpp)
        add_library(utf8::cpp ALIAS utf8cpp::utf8cpp)
    endif()
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
    return()
endif()

add_library(utf8::cpp INTERFACE IMPORTED)

target_include_directories(utf8::cpp
    INTERFACE
        ${TR_THIRD_PARTY_SOURCE_DIR}/utfcpp/source)
