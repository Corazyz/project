# CMakeLists.txt file
# #[[ ]] block comment
# CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# project(Demo1)


# part 1
# aux_source_directory(. DIR_SRC)

# add_executable(Demo ${DIR_SRC})

# add_executable(Demo add.cpp sub.cpp div.cpp multi.cpp main.cpp)

# aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)
# aux_source_directory(. DIR_SRC)
# add_executable(app ${DIR_SRC} ${SRC_LIST})

# part 2: set
# set 指令的语法：
    # [] 中的参数为可选项，如不需要可以不写
# set(VAR [VALUE] [CACHE TYPE DOCSTRING [FORCE]])

# 方式1： 各个源文件之间使用空格间隔
# set(SRC_LIST add.cpp div.cpp multi.cpp main.cpp sub.cpp)

# 方式2： 各个源文件之间使用分号间隔
# set(SRC_LIST add.cpp; div.cpp; multi.cpp; main.cpp; sub.cpp)

# set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# aux_source_directory(${SRC} SRC_LIST)
# aux_source_directory(. DIR_SRC)
# add_executable(app ${SRC_LIST} ${DIR_SRC})

# include_directories(${PROJECT_SOURCE_DIR}/include)
# include_directories(${PROJECT_SOURCE_DIR})

# set(HOME ../../helloworld)
# set(EXECUTABLE_OUTPUT_PATH ${HOME}/build)     # 指定可执行程序的输出路径

# include_directories(${PROJECT_SOURCE_DIR}/include)

# part 3: 完整示例
# cmake_minimum_required(VERSION 3.0)
# project(Demo)
# set(CMAKE_CXX_STANDARD 11)
# set(HOME /home/zyz/projects/concate_yuv)
# set(EXECUTABLE_OUTPUT_PATH ${HOME}/build)
# include_directories(${PROJECT_SOURCE_DIR})
# aux_source_directory(. DIR_SRC)
# add_executable(demo ${DIR_SRC})
# file(GLOB SRC_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
# file(GLOB MAIN_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# add_executable(demo ${SRC_LIST} ${MAIN_LIST})
# add_executable(demo ${MAIN_LIST})

# part 4: aux_source_directory & file(GLOB/GLOB_RECURSE 变量名 要搜索的文件路径和文件类型)
# aux_source_directory 命令可以查找某个路径下的所有源文件, 命令格式为： aux_source_directory(< dir > < variable >)
# GLOB: 将指定目录下搜索到的满足条件的所有文件名生成一个列表，并将其存储到变量中。
# GLOB_RECURSE：递归搜索指定目录，将搜索到的满足条件的文件名生成一个列表，并将其存储到变量中



# part 5: 制作动态库或静态库
## 制作静态库：
### add_library(库名称 STATIC 源文件1 [源文件2] ...)
### linux中，静态库名字分为三部分：lib + 库名字 + .a

# add_library(calc STATIC ${SRC_LIST})

## 制作动态库
### add_library(库名称 SHARED 源文件1 [源文件2] ...)
### linux中，动态库名字分为三部分：lib + 库名字 + .so

# add_library(calc SHARED ${SRC_LIST})

## 指定输出的路径
### 方式1：适用于动态库
#### 设置动态库生成路径：
# set(EXECUTABLE_OUTPUT_PATH ${HOME}/lib)
# add_library(calc_shared SHARED ${SRC_LIST})
### 方式2：都适用
#### 设置动态库/静态库生成路径
# set(LIBRARY_OUTPUT_PATH ${HOME}/lib)
# add_library(calc SHARED ${SRC_LIST})
# add_library(calc_static STATIC ${SRC_LIST})

# link_directories(${PROJECT_SOURCE_DIR}/lib)
# link_libraries(calc_static)
# add_executable(demo ${MAIN_LIST})
# target_link_libraries(demo pthread calc_shared)

# message(STATUS "source path: ${PROJECT_SOURCE_DIR}")
cmake_minimum_required(VERSION 3.0)
project(test)
set(LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(HEAD_PATH ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(CALC_LIB calc)
set(SORT_LIB sort)

set(APP_NAME_1 test1)
set(APP_NAME_2 test2)

add_subdirectory(calc)
# add_subdirectory(sort)
add_subdirectory(test1)
# add_subdirectory(test2)