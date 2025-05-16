cmake_minimum_required(VERSION 3.0)

project(KCPServer)
add_compile_options(--std=c++11)
add_definitions("-Wall -g")

include_directories(include)
aux_source_directory(src SRC_LIST)

add_executable(main main.cpp ${SRC_LIST})