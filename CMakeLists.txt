# https://cmake.org/cmake-tutorial/

cmake_minimum_required(VERSION 2.6)
project(MyPy)

set (MyPy_VERSION_MAJOR 1)
set (MyPy_VERSION_MINOR 0)

add_executable(main main.cpp)


# Boost 
set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.66.0 COMPONENTS filesystem system )
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(main ${Boost_LIBRARIES})
endif()