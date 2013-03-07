cmake_minimum_required(VERSION 2.6)
project(fontRender)
find_package(PkgConfig)
pkg_check_modules(PC REQUIRED libpng15 freetype2 harfbuzz cairo gl glew glfw3)

list(APPEND CMAKE_C_FLAGS "-std=c99")

include_directories(${PC_INCLUDE_DIRS})

add_executable(ft2_char_cairo ft2_char_cairo.c)
target_link_libraries(ft2_char_cairo ${PC_LIBRARIES})

add_executable(ft2_char_libpng ft2_char_libpng.c)
target_link_libraries(ft2_char_libpng ${PC_LIBRARIES})

add_executable(ft2_char_gl ft2_char_gl.c)
target_link_libraries(ft2_char_gl ${PC_LIBRARIES})

add_executable(harfbuzz-ft2 harfbuzz-ft2.c)
target_link_libraries(harfbuzz-ft2 ${PC_LIBRARIES})