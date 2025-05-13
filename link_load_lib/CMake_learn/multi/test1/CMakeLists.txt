cmake_minimum_required(VERSION 3.16)
project(test1)

#搜索源文件
aux_source_directory(./ SRC)
include_directories(${HEADPATH})
# set(LIBRARY_OUTPUT_PATH ${LIBPATH})
# add_library(${CALCLIB} STATIC ${SRC})
# link_directories(${LIBPATH})
# link_libraries(${CALCLIB})
# set(EXECUTABLE_OUTPUT_PATH ${EXECPATH})
add_executable(${APPNAME1} ${SRC})
# add_dependencies(${APPNAME1} calc)
target_link_libraries(${APPNAME1} calc)