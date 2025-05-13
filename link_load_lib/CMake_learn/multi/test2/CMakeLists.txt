cmake_minimum_required(VERSION 3.16)
project(test2)

#搜索源文件
aux_source_directory(./ SRC)
include_directories(${HEADPATH})
# set(LIBRARY_OUTPUT_PATH ${LIBPATH})
# add_library(${CALCLIB} STATIC ${SRC})
link_directories(${LIBPATH})
link_libraries(${SORTLIB})
# set(EXECUTABLE_OUTPUT_PATH ${EXECPATH})
add_executable(${APPNAME2} ${SRC})
add_dependencies(${APPNAME2} sort)