cmake_minimum_required(VERSION 3.28.3)

project(kcpclient)
add_compile_options(-std=c++20)
add_definitions("-Wall -g")

include_directories(include)
aux_source_directory(src SRC_LIST)

# 生成动态库
add_library(kcpclient SHARED ${SRC_LIST})
# add_library(kcpclient STATIC ${SRC_LIST}) # 生成静态库
# 设置库的版本号
set_target_properties(kcpclient PROPERTIES VERSION 1.0.0 SOVERSION 1)
# 将库放置到 src 同级目录下的 lib 路径中
set_target_properties(kcpclient PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)


# 生成可执行文件
add_executable(main main.cpp)
target_include_directories(main PUBLIC include)
target_link_libraries(main kcpclient)

