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

add_executable(pinyin_db_smoke
    tests/pinyin_db_smoke.cpp)
target_include_directories(pinyin_db_smoke
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS})
target_link_libraries(pinyin_db_smoke
    pinyinfont)
add_test(
    NAME pinyin_db_smoke
    COMMAND pinyin_db_smoke "${PINYINFONT_TEST_FIXTURE_DIR}")

add_executable(pinyin_components_test
    tests/pinyin_components_test.cpp
    tests/test_font_fixture.cpp)
target_include_directories(pinyin_components_test
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS})
target_link_libraries(pinyin_components_test
    pinyinfont)
add_test(
    NAME pinyin_components_test
    COMMAND pinyin_components_test "${PINYINFONT_TEST_FIXTURE_DIR}")

add_executable(pinyin_synthesis_test
    tests/pinyin_synthesis_test.cpp
    tests/test_font_fixture.cpp)
target_include_directories(pinyin_synthesis_test
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS} "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_link_libraries(pinyin_synthesis_test
    pinyinfont)
add_test(
    NAME pinyin_synthesis_test
    COMMAND pinyin_synthesis_test "${PINYINFONT_TEST_FIXTURE_DIR}")

add_executable(cmap_format4_test
    tests/cmap_format4_test.cpp
    tools/cmap_integrity.cpp)
target_include_directories(cmap_format4_test
    PRIVATE ${PINYINFONT_INTERNAL_INCLUDE_DIRS}
            "${CMAKE_CURRENT_SOURCE_DIR}/tools")
target_link_libraries(cmap_format4_test
    pinyinfont)
add_test(
    NAME cmap_format4_test
    COMMAND cmap_format4_test)
