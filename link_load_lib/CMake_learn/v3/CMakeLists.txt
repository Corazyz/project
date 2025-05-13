#[[
多行注释
]]
cmake_minimum_required(VERSION 3.10)
project(cmake_learn)
# add_executable(app main.cpp add.cpp sub.cpp mult.cpp div.cpp)

#[[ set 的 使用：
#set(var [value] [cache type docstring [force]/])
# var: 变量名
# value：变量值，使用空格或分号

# set(SRC_LIST main.cpp add.cpp sub.cpp mult.cpp div.cpp)
# add_executable(app ${SRC_LIST})

# -std=c++11 指定使用c++11标准，c++标准对应一个宏：DCMAKE_CXX_STANDARD，CMake中指定c++标准有两种方式，只能指定一个：
# 在CMakeLists中通过set指定
# 增加 -std=c++11

set(CMAKE_CXX_STANDARD 11)
# # 增加 -std=c++14
# set(CMAKE_CXX_STANDARD 14)
# # 增加 -std=c++17
# set(CMAKE_CXX_STANDARD 17)
# 执行cmake命令的时候指定这个宏的值
#cmake 文件路径 -DCMAKE_CXX_STANDARD=11

# 指定输出路径，对应一个宏：EXECUTABLE_OUTPUT_PATH，值通过set设置
# 定义一个变量用于存储一个绝对路径
set(HOME /home/zyz/projects/test_project/link_load_lib/CMake_learn)
set(EXECUTABLE_OUTPUT_PATH ${HOME}/build/aa/bb)
]]
set(CMAKE_CXX_STANDARD 11)
set(HOME /home/zyz/projects/test_project/link_load_lib/CMake_learn/v3)
set(EXECUTABLE_OUTPUT_PATH ${HOME}/build/aa)

# 如果在build上层目录生成了CMakeFile等文件，在build中执行cmake不会再生成对应的文件

#[[ 搜索文件
# 搜索文件 aux_source_directory 和 file
# aux_source_directory(目录 变量）将从目录下搜索到的源文件列表存储到变量中
# file: file(GLOB/GLOB_RECURSE 变量名 要搜索的文件路径和文件类型)
# aux_source_directory(${PROJECT_SOURCE_DIR} SRC)
# PROJECT_SOURCE_DIR CMakeLists所在的目录
# file(GLOB_RECURSE SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
]]
# file(GLOB SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB SRC *.cpp)

#[[ 指定头文件路径]]
include_directories(${PROJECT_SOURCE_DIR}/include)

#[[ 指定库文件生成路径
宏：LIBRARY_OUTPUT_PATH, 适用于动态库和静态库
动态库也可以使用 EXECUTATBLE_OUTPUT_PATH
]]
# set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build/lib)

#[[ CMake 制作库文件
add_library(库名称 STATIC 源文件1 源文件2 ...)
静态库名称分为三部分：lib + 库名字 + .a 只需要指定库名字就可以了，另外两部分在生成该文件的时候会自动填充
.so 动态库
.a 静态库
]]
# add_library(calc SHARED ${SRC})
# add_library(calc_s STATIC ${SRC})

#[[链接静态库：link_libraries(<static_lib> <static_lib> ...)
系统提供：只需要指定名字
自定义：将静态库的路径指定出来 link_directories(<lib path>)
]]
link_libraries(calc_s)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib1)

add_executable(app ${SRC})

#[[链接动态库：
target_link_libraries(<target> <PRIVATE|PUBLIC|INTERFACE> <item>...)
target: 指定要加载动态库的文件的名字，可以是 源文件，动态库文件，可执行文件
PRIVATE|PUBLIC|INTERFACE：动态库访问权限，默认为 PUBLIC PUBLIC>PRIVATE>INTERFACE
动态库的链接具有传递性

mmu 把虚拟内存地址映射到物理内存上
]]
# target_link_libraries(app calc)