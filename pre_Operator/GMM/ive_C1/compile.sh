#!/bin/bash

CXX=g++
CXXFLAGS="-std=c++11 -O2 -Wall"
INCLUDES="-I/usr/include/opencv4"
LIBS="-lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs -lstdc++fs"

if [ "$1" = "clean" ]; then
    rm -f *.o gmm_sample
    exit 0
fi

CPP_FILES=$(find . -maxdepth 1 -name "*.cpp")

for cpp in $CPP_FILES; do
    obj="${cpp%.cpp}.o"
    echo "Compiling $cpp -> $obj"
    $CXX $CXXFLAGS $INCLUDES -c $cpp -o $obj
done

OBJ_FILES=$(find . -maxdepth 1 -name "*.o")
echo "Linking -> gmm_sample"
$CXX $CXXFLAGS $OBJ_FILES -o gmm_sample $LIBS

rm -f *.o

echo "Build done."
