# CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# project(Demo1)

# aux_source_directory(. DIR_SRC)

# add_executable(Demo ${DIR_SRC})

# add_subdirectory(concatenate_yuv helloworld)

# target_link_libraries(concatenate hello)
# target_link_libraries(Demo ${EXTRA_LIBS})

cmake_minimum_required(VERSION 3.8)
project(demo_cpp)
set(OpenCV_DIR ./opencv_zip/opencv-4.10.0/build/)
find_package(OpenCV)
include_directories(${OpenCV_INCLUDE_DIRS})
message(${OpenCV_INCLUDE_DIRS})
message(${OpenCV_LIBS})

add_executable(main src/main.cpp)
target_link_libraries(main ${OpenCV_LIBS})