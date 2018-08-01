cmake_minimum_required(VERSION 3.12)
project(chatty)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

FILE(GLOB sources src/*.c)
add_executable(client ${sources})
