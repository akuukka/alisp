cmake_minimum_required(VERSION 3.11)
project(ALisp)

option(SINGLE_HEADER "Build test program using the single header version" OFF)

set(SOURCE_FILES
  ${CMAKE_SOURCE_DIR}/source/main.cpp
  )

if (NOT SINGLE_HEADER)
  set(SOURCE_FILES
    ${SOURCE_FILES}
    ${CMAKE_SOURCE_DIR}/source/Object.cpp
    ${CMAKE_SOURCE_DIR}/source/Machine.cpp
    ${CMAKE_SOURCE_DIR}/source/ConsCell.cpp
    ${CMAKE_SOURCE_DIR}/source/SymbolObject.cpp
    ${CMAKE_SOURCE_DIR}/source/StringObject.cpp
    ${CMAKE_SOURCE_DIR}/source/ConsCellObject.cpp
    ${CMAKE_SOURCE_DIR}/source/SharedDataObject.cpp
    )
else()
  add_definitions(-DALISP_SINGLE_HEADER)
endif()

add_definitions(-std=c++17)
add_executable(ALisp ${SOURCE_FILES})
