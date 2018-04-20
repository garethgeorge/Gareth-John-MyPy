# https://cmake.org/cmake-tutorial/
cmake_minimum_required(VERSION 3.11)
project(MyPy)

set(CMAKE_C_COMPILER "gcc-7")
set(CMAKE_CXX_COMPILER "g++-7")

set (MyPy_VERSION_MAJOR 1)
set (MyPy_VERSION_MINOR 0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories("lib")

# TODO: have a cmake file per directory which adds its own sources
# see https://stackoverflow.com/questions/7046956/populating-srcs-from-cmakelists-txt-in-subdirectories
file(GLOB_RECURSE SRCS_LIB
  "lib/*.cpp"
  "lib/*.hpp")

file(GLOB_RECURSE SRCS_MYPY 
  "src/*.cpp"
  "src/*.hpp")

list(FILTER SRCS_MYPY EXCLUDE REGEX ".*vm_main.cpp$")

add_executable(mypy
  src/vm_main.cpp 
  ${SRCS_LIB}
  ${SRCS_MYPY}
)

file(GLOB_RECURSE SRCS_tests
  "test/*.cpp"
  "test/*.hpp")

add_executable(cmake_test
  ${SRCS_tests}
  ${SRCS_LIB}
  ${SRCS_MYPY}
)

# add_custom_command(TARGET cmake_test POST_BUILD COMMAND ./cmake_test) 