# https://cmake.org/cmake-tutorial/

cmake_minimum_required(VERSION 3.0)
project(MyPy)

set (MyPy_VERSION_MAJOR 1)
set (MyPy_VERSION_MINOR 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

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
