cmake_minimum_required(VERSION 3.2)

project ( fcgiserver CXX )

if (NOT DEFINED CMAKE_BUILD_TYPE)
	    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")

message("Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX_EXTENSIONS NO)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_NOCOVERAGE ${CMAKE_CXX_FLAGS_DEBUG})
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "-pipe -Wall -march=native -fvisibility=hidden")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 --coverage")
endif()

find_package(Catch2 QUIET)
if (NOT Catch2_DIR STREQUAL "Catch2_DIR-NOTFOUND")
	enable_testing()
endif()

add_subdirectory(test)
add_subdirectory(src)
