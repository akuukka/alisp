cmake_minimum_required(VERSION 3.11)
project(ALisp)

set(SOURCE_FILES
  ${CMAKE_SOURCE_DIR}/source/main.cpp
  )

add_definitions(-std=c++17)
add_executable(ALisp ${SOURCE_FILES})
