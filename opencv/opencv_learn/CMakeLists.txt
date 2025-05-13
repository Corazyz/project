cmake_minimum_required(VERSION 3.16)
project(create_mat)
set(CMAKE_CXX_STANDARD 17)
# 查找OpenCV包
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})

message(STATUS "OpenCV_DIR: ${OpenCV_DIR}")
message(STATUS "OpenCV_INCLUDE_DIRS: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV_LIBS: ${OpenCV_LIBS}")
# 添加可执行文件
add_executable(create_mat create_mat.cpp)

# 链接OpenCV库
target_link_libraries(create_mat ${OpenCV_LIBS})
