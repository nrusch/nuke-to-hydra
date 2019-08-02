cmake_minimum_required(VERSION 2.8)

project(NukeToHydra)


set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" "$ENV{CMAKE_MODULE_PATH}")

find_package(Nuke REQUIRED)


include(pxrTargets)

message(STATUS "USD_ROOT: ${USD_ROOT}")


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-DUSE_GLEW -fPIC -msse -Wall)


add_subdirectory(src/HydraRenderOp)

