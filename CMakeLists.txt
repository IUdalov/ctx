cmake_minimum_required(VERSION 3.10)
project(ctx)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -O2 -Wall -Wextra -pedantic -Werror")

add_executable(ctx main.cpp coro_multiplexer.h bsearch.h bsearch.cpp)

target_link_libraries(ctx boost_context-mt)