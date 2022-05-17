cmake_minimum_required(VERSION 3.10)

project(pinyin_font)

set(CMAKE_CXX_STANDARD 11)

# pinyinfont
add_library(pinyinfont
    source/status.h
    source/utility.h
    source/ot_font.h
    source/ot_font.cpp
    source/ot_font_parser.h
    source/ot_font_parser.cpp)
target_include_directories(pinyinfont
    PRIVATE "include")
if (MSVC)
    target_compile_definitions(pinyinfont PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

add_executable(pinyinfont_test
    source/test.cpp)
target_link_libraries(pinyinfont_test
    pinyinfont)
