cmake_minimum_required(VERSION 3.24)
project(ChipmunkTest)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig)
pkg_check_modules(TEST REQUIRED sdl2 freetype2)

add_executable(ChipmunkTest main.cpp)
target_link_libraries(ChipmunkTest PRIVATE chipmunk ${TEST_LIBRARIES})
target_compile_options(ChipmunkTest PRIVATE ${TEST_CFLAGS_OTHER})
target_include_directories(ChipmunkTest PRIVATE ${TEST_INCLUDE_DIRS})
