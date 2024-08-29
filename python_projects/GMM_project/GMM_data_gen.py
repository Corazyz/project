# import numpy as np

# # 设置随机种子以确保结果可重复
# np.random.seed(42)

# # 定义高斯分布的均值和协方差矩阵
# means = [[2, 2], [7, 7], [12, 12], [17, 17], [22, 22]]
# covs = [[[0.5, 0], [0, 0.5]],
#         [[0.8, 0], [0, 0.8]],
#         [[1.0, 0], [0, 1.0]],
#         [[1.2, 0], [0, 1.2]],
#         [[1.5, 0], [0, 1.5]]]

# # 生成数据
# data = []
# for mean, cov in zip(means, covs):
#     data.append(np.random.multivariate_normal(mean, cov, 100))

# # 合并所有数据
# data = np.vstack(data)

# # 将数据写入data.txt文件
# with open("data.txt", 'w') as file:
#     for point in data:
#         file.write(f"{point[0]} {point[1]}\n")

# print("data.txt 文件已生成")

import numpy as np
import random

a = np.ones((6, 3, 2))
b = np.ones((6, 3, 2))/3
c = np.abs(a-b)
for i in range(6):
    for j in range(3):
        a[i, j, 0] += random.random()
        a[i, j, 1] += random.random()
# print(a, b, c)
# print(a)
match0 = np.abs(a) <= 1.5
print(match0)
match = np.max(match0, axis=2)
print(match, match.shape)