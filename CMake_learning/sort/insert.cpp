#include <iostream>
#include <vector>

// 插入排序函数
void insertionSort(std::vector<int>& vec) {
    for (size_t i = 1; i < vec.size(); ++i) {
        int key = vec[i];
        int j = i - 1;
        while (j >= 0 && vec[j] > key) {
            vec[j + 1] = vec[j];
            --j;
        }
        vec[j + 1] = key;
    }
}

// 测试函数
void testInsertionSort() {
    std::vector<int> numbers = {5, 3, 8, 1, 2, 9, 4, 7, 6};
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    insertionSort(numbers);

    if (numbers == expected) {
        std::cout << "插入排序测试成功！" << std::endl;
    } else {
        std::cout << "插入排序测试失败！" << std::endl;
    }
}

// 主函数
//int main() {
    //testInsertionSort();
  //  return 0;
//}
