cmake_minimum_required(VERSION 3.10)

project(pinyin_font)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PINYINFONT_INTERNAL_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/opentype"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/pinyin"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/synthesis")

# pinyinfont
add_library(pinyinfont
    src/opentype/status.h
    src/opentype/utility.h
    src/opentype/scope_guard.h
    src/opentype/mac_glyph_names.h
    src/opentype/mac_glyph_names.cpp
    src/opentype/ot_font.h
    src/opentype/ot_font.cpp
    src/opentype/ot_font_builder.h
    src/opentype/ot_font_builder.cpp
    src/opentype/ot_font_parser.h
    src/opentype/ot_font_parser.cpp
    src/opentype/ot_cmap.h
    src/opentype/ot_cmap_format0.cpp
    src/opentype/ot_cmap_format4.cpp
    src/opentype/ot_cmap_format6.cpp
    src/opentype/ot_cmap_format12.cpp
    src/opentype/ot_cmap_encoder.h
    src/opentype/ot_cmap_encoder.cpp
    src/opentype/ot_font_writer.h
    src/opentype/ot_font_writer.cpp
    src/pinyin/pinyin_db.h
    src/pinyin/pinyin_db.cpp
    src/synthesis/pinyin_components.h
    src/synthesis/pinyin_components.cpp
    src/synthesis/pinyin_layout.h
    src/synthesis/pinyin_layout.cpp
    src/synthesis/pinyin_font_builder.h
    src/synthesis/pinyin_font_builder.cpp)
target_include_directories(pinyinfont
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS})
if (MSVC)
    target_compile_definitions(pinyinfont PRIVATE "_CRT_SECURE_NO_WARNINGS")
    # https://docs.microsoft.com/en-us/cpp/build/reference/source-charset-set-source-character-set?view=msvc-160
    target_compile_options(pinyinfont PRIVATE /source-charset:utf-8)
endif()

add_executable(pinyinfont_cli
    cli/pinyinfont.cpp)
target_include_directories(pinyinfont_cli
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS})
target_link_libraries(pinyinfont_cli
    pinyinfont)
set_target_properties(pinyinfont_cli PROPERTIES OUTPUT_NAME pinyinfont)

add_executable(font_tool
    tools/font_tool.cpp
    tools/cmap_integrity.cpp)
target_include_directories(font_tool
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools")
target_link_libraries(font_tool
    pinyinfont)
if (MSVC)
    target_compile_definitions(font_tool PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()

enable_testing()

set(PINYINFONT_TEST_FIXTURE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/test-fixtures")
file(MAKE_DIRECTORY "${PINYINFONT_TEST_FIXTURE_DIR}")

add_executable(pinyinfont_tests
    tests/test_runner.cpp
    tests/pinyin_db_smoke.cpp
    tests/pinyin_components_test.cpp
    tests/pinyin_synthesis_test.cpp
    tests/cmap_format4_test.cpp
    tests/test_font_fixture.cpp
    tools/cmap_integrity.cpp)
target_include_directories(pinyinfont_tests
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS}
            "${CMAKE_CURRENT_SOURCE_DIR}/tests"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools")
target_link_libraries(pinyinfont_tests
    pinyinfont)
add_test(
    NAME pinyinfont_tests
    COMMAND pinyinfont_tests "${PINYINFONT_TEST_FIXTURE_DIR}")
