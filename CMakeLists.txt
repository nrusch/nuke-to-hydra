cmake_minimum_required(VERSION 3.0)

project(NukeToHydra)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
                      ${CMAKE_CURRENT_SOURCE_DIR}/cmake
                      $ENV{CMAKE_MODULE_PATH})


set(BOOST_PYTHON_COMPONENT_NAME "python27"
    CACHE STRING
    "The Boost component name to pass to the `find_package` command.")

set(HDNUKE_LIB_NAME HdNuke)


find_package(Nuke REQUIRED)
find_package(USD 0.20.2 REQUIRED)

find_package(Boost REQUIRED
    COMPONENTS ${BOOST_PYTHON_LIBNAME})
find_package(PythonInterp 2.7 REQUIRED)
find_package(PythonLibs 2.7 REQUIRED)
find_package(TBB REQUIRED
    COMPONENTS tbb)


include_directories(SYSTEM
    ${Boost_INCLUDE_DIRS}
    ${PYTHON_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS})

link_directories(${USD_LIBRARY_DIR})


set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-fPIC -msse -Wall -Wno-deprecated)


add_subdirectory(src/hdNuke)
add_subdirectory(src/ops)

install(FILES src/menu.py
    DESTINATION plugins)
