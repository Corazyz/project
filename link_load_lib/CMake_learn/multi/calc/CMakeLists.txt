cmake_minimum_required(VERSION 3.16)
project(calc)

#搜索源文件
aux_source_directory(./ SRC)
include_directories(${HEADPATH})
# set(LIBRARY_OUTPUT_PATH ${LIBPATH})
add_library(${CALCLIB} STATIC ${SRC})

