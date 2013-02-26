cmake_minimum_required(VERSION 2.6)
project(fontRender)
find_package(PkgConfig)
pkg_check_modules(PC REQUIRED freetype2 harfbuzz cairo)

include_directories(${PC_INCLUDE_DIRS})

add_executable(ft2_char ft2_char.c)
target_link_libraries(ft2_char ${PC_LIBRARIES})