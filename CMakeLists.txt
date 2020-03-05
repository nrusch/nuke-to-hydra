cmake_minimum_required(VERSION 3.0)

project(NukeToHydra)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake
                      $ENV{CMAKE_MODULE_PATH})

find_package(Nuke REQUIRED)


include(pxrConfig)

message(STATUS "USD_ROOT: ${USD_ROOT}")

set(HDNUKE_LIB_NAME HdNuke)


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-fPIC -msse -Wall -Wno-deprecated)


add_subdirectory(src/hdNuke)
add_subdirectory(src/ops)

install(FILES src/menu.py
    DESTINATION plugins)
