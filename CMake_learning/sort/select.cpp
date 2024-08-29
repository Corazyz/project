#include <iostream>
#include <vector>

// 选择排序函数
void selectionSort(std::vector<int>& vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        size_t minIndex = i;
        for (size_t j = i + 1; j < vec.size(); ++j) {
            if (vec[j] < vec[minIndex]) {
                minIndex = j;
            }
        }
        if (minIndex != i) {
            std::swap(vec[i], vec[minIndex]);
        }
    }
}

// 测试函数
void testSelectionSort() {
    std::vector<int> numbers = {5, 3, 8, 1, 2, 9, 4, 7, 6};
    std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8, 9};

    selectionSort(numbers);

    if (numbers == expected) {
        std::cout << "选择排序测试成功！" << std::endl;
    } else {
        std::cout << "选择排序测试失败！" << std::endl;
    }
}

// 主函数
//int main() {
    //testSelectionSort();
  //  return 0;
//}
