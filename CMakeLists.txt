cmake_minimum_required(VERSION 3.14)
project(assignment4 C)

set(CMAKE_C_STANDARD 11)

add_executable(assignment4 Briyani.c)
target_link_libraries(assignment4 -pthread)