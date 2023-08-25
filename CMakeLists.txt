cmake_minimum_required (VERSION 3.22)
project(Scheme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

find_package(Boost REQUIRED)

add_compile_options(
	$<$<CXX_COMPILER_ID:MSVC>:/W4>										# warning level 4
	$<$<CXX_COMPILER_ID:MSVC>:/WX>)										# warnings as errors

include_directories(
	${Boost_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR})

add_compile_definitions(
	$<$<CONFIG:Debug>:DEBUG>)

add_executable(Repl Repl.cpp Scheme.h)
