# https://cmake.org/cmake-tutorial/

cmake_minimum_required(VERSION 3.0)
project(MyPy)

set (MyPy_VERSION_MAJOR 1)
set (MyPy_VERSION_MINOR 0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(mypy
  src/vm_main.cpp 
  src/pycode.cpp 
  src/pyinterpreter.cpp
  src/pyframe.cpp
  src/pyvalue_helpers.cpp
  src/builtins/builtins.cpp
  lib/oplist.cpp
  lib/base64.cpp
)

# Boost 
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.66.0 COMPONENTS filesystem system program_options )
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(mypy ${Boost_LIBRARIES})
endif()