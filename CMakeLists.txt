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

# set lib as an include directory so we can use #include <filename>
include_directories("lib")

# TODO: have a cmake file per directory which adds its own sources
# see https://stackoverflow.com/questions/7046956/populating-srcs-from-cmakelists-txt-in-subdirectories

# identify the mypy libraries
file(GLOB_RECURSE SRCS_LIB
  "lib/*.cpp"
  "lib/*.hpp")

# identify the mypy sources 
file(GLOB_RECURSE SRCS_MYPY 
  "src/*.cpp"
  "src/*.hpp")

list(FILTER SRCS_MYPY EXCLUDE REGEX ".*vm_main.cpp$") # remove vm_main.cpp from it, this is a separate binary


#
# create a static library for mypy sources
# 
add_library( mypycore ${SRCS_MYPY} ${SRCS_LIB} )

#
# create mypy executable
# 
add_executable(mypy
  src/vm_main.cpp 
  ${SRCS_LIB}
)

target_link_libraries(mypy mypycore)


#
# create mypy tests
# 
file(GLOB_RECURSE SRCS_tests
  "test/*.cpp"
  "test/*.hpp")

add_executable(cmake_test
  ${SRCS_tests}
  ${SRCS_LIB}
)

target_link_libraries(cmake_test mypycore)

# add_custom_command(TARGET cmake_test POST_BUILD COMMAND ./cmake_test) 