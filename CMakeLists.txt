cmake_minimum_required(VERSION 3.12)
project(myduduo)

#设置输出路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++11")

#定义编译的源文件
aux_source_directory(. SRC_LIST)

#编译生成动态库
add_library(mymuduo SHARED ${SRC_LIST})