#!/bin/bash

# 编译选项
CXX=g++
CXXFLAGS="-std=c++11 -O2 -Wall"
INCLUDES="-I/usr/include/opencv4"
LIBS="-lopencv_core -lopencv_highgui -lopencv_imgproc -lopencv_videoio -lopencv_imgcodecs -lstdc++fs"


# 清理功能
if [ "\$1" = "clean" ]; then
    rm -f *.o gmm_sample
    echo "🗑️ 已清理所有中间文件和可执行文件"
    exit 0
fi

# 获取当前目录下所有 .cpp 文件（不包括 compile.sh 本身）
CPP_FILES=$(find . -maxdepth 1 -name "*.cpp" -not -name "compile.sh")

# 检查是否找到源文件
if [ -z "$CPP_FILES" ]; then
    echo "错误：未找到任何 .cpp 源文件！"
    exit 1
fi

echo "正在编译以下源文件："
echo "$CPP_FILES"

# 编译每个源文件为 .o 文件
for file in $CPP_FILES; do
    obj="${file%.cpp}.o"
    $CXX $CXXFLAGS $INCLUDES -c "$file" -o "$obj"
    if [ $? -ne 0 ]; then
        echo "编译失败: $file"
        exit 1
    fi
done

# 收集所有 .o 文件
OBJ_FILES=$(find . -maxdepth 1 -name "*.o" -not -name "compile.sh" | tr '\n' ' ')

# 链接生成可执行文件
$CXX $CXXFLAGS $INCLUDES $OBJ_FILES -o gmm_sample $LIBS

if [ $? -eq 0 ]; then
    echo "✅ 编译完成，可执行文件: gmm_sample"
    # 删除所有 .o 中间文件
    rm -f *.o
    echo "🗑️ 已清理中间文件 (.o)"
else
    echo "❌ 链接失败！"
    exit 1
fi
