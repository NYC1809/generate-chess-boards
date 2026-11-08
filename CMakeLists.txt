cmake_minimum_required(VERSION 4.2)
project(C___Generate_Chess_puzzles C)

set(CMAKE_C_STANDARD 23)

add_executable(C___Generate_Chess_puzzles main.c)
target_link_libraries(C___Generate_Chess_puzzles ws2_32)
