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
    ${CMAKE_SOURCE_DIR}/source/MathFunctions.cpp
    ${CMAKE_SOURCE_DIR}/source/SequenceFunctions.cpp
    ${CMAKE_SOURCE_DIR}/source/Error.cpp
    ${CMAKE_SOURCE_DIR}/source/StringFunctions.cpp
    ${CMAKE_SOURCE_DIR}/source/Function.cpp
    ${CMAKE_SOURCE_DIR}/source/FArgs.cpp
    ${CMAKE_SOURCE_DIR}/source/ListFunctions.cpp
    ${CMAKE_SOURCE_DIR}/source/ConsCell.cpp
    ${CMAKE_SOURCE_DIR}/source/MacroFunctions.cpp
    ${CMAKE_SOURCE_DIR}/source/SymbolObject.cpp
    ${CMAKE_SOURCE_DIR}/source/SymbolFunctions.cpp
    ${CMAKE_SOURCE_DIR}/source/StringObject.cpp
    ${CMAKE_SOURCE_DIR}/source/ValueObject.cpp
    ${CMAKE_SOURCE_DIR}/source/ConsCellObject.cpp
    ${CMAKE_SOURCE_DIR}/source/SharedValueObject.cpp
    )
else()
  add_definitions(-DALISP_SINGLE_HEADER)
endif()

add_definitions(-std=c++17)
add_executable(ALisp ${SOURCE_FILES})
