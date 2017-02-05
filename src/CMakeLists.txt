cmake_minimum_required(VERSION 2.8)

project(cppwhiletrue)

if("${CMAKE_CXX_COMPILER}" MATCHES "clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi -lc++")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /WX /analyze")
else()
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wpedantic -Wfatal-errors -std=c++1z -Wconversion -Wsign-conversion")
endif()

enable_testing()

add_subdirectory(dev)
add_subdirectory(test)
