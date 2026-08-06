if(USE_SYSTEM_UTFCPP)
    find_package(utf8cpp CONFIG REQUIRED)
    add_library(utf8::cpp INTERFACE IMPORTED)
    target_link_libraries(utf8::cpp INTERFACE utf8cpp::utf8cpp)
    set(${CMAKE_FIND_PACKAGE_NAME}_FOUND TRUE)
    return()
endif()

add_library(utf8::cpp INTERFACE IMPORTED)

target_include_directories(utf8::cpp
    INTERFACE
        ${TR_THIRD_PARTY_SOURCE_DIR}/utfcpp/source)
