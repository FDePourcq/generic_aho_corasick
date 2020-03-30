cmake_minimum_required(VERSION 3.13)
project(generic_aho_corasick)

set(CMAKE_CXX_STANDARD 11)

include_directories(.)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -Wl,-O3 -O3 -m64 -g3 -pthread -Wall -Wextra -Wvla")

add_executable(test_aho_corasick
        aho_corasick.hpp
        test.cpp)
