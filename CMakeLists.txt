cmake_minimum_required (VERSION 3.21.1)
project(Scheme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(WIN32)
	add_compile_options(/W3 /WX)
endif(WIN32)

find_package(Boost REQUIRED)

include_directories(
	${Boost_INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR})

add_compile_definitions($<$<CONFIG:Debug>:DEBUG>)

add_executable(Repl Repl.cpp Scheme.h)
