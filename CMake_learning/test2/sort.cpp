#include "insert.cpp"
#include "select.cpp"

#include <iostream>
#include <vector>

// 主函数
int main() {
    // 测试插入排序
    std::vector<int> insertNumbers = {5, 3, 8, 1, 2, 9, 4, 7, 6};
    insertionSort(insertNumbers);
    std::cout << "插入排序后的数组: ";
    for (int num : insertNumbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    // 测试选择排序
    std::vector<int> selectNumbers = {5, 3, 8, 1, 2, 9, 4, 7, 6};
    selectionSort(selectNumbers);
    std::cout << "选择排序后的数组: ";
    for (int num : selectNumbers) {
        std::cout << num << " ";
    }
    std::cout << std::endl;

    // 运行测试函数
    testInsertionSort();
    testSelectionSort();

    return 0;
}































































