cmake_minimum_required(VERSION 3.2)

project ( fcgiserver CXX )

if (NOT DEFINED CMAKE_BUILD_TYPE)
	    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build")
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/")

#message("Compiler ID : ${CMAKE_CXX_COMPILER_ID}")
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

	include(Catch)
	macro(catchtest name)
		if (DEFINED CMAKE_TESTING_ENABLED)
			add_executable(test_${name} ${ARGN})
			target_link_libraries(test_${name} PRIVATE Catch2::Catch2WithMain ${name})
			set_target_properties(test_${name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
			catch_discover_tests(test_${name})
		endif ()
	endmacro()
endif()

add_subdirectory(src)
