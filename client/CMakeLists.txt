cmake_minimum_required(VERSION 3.28.3)

project(kcpclient)
add_compile_options(-std=c++20)
add_definitions("-Wall -g")

include_directories(include)
aux_source_directory(src SRC_LIST)

add_executable(main main.cpp ${SRC_LIST})
