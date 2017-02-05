cmake_minimum_required(VERSION 2.8)

project(cppwhiletrue)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX /analyze")
else()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -std=c++1y -Wconversion -Wsign-conversion -Wnull-dereference -Wstrict-overflow=5 -Wshadow")
endif()

enable_testing()

add_subdirectory(dev)
add_subdirectory(test)
