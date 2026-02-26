# import numpy as np

# # 生成 256x256 随机图像，像素值在 [0, 255) 之间（浮点数）
# image = np.random.randint(0, 25, (64, 64)).astype(np.float64)

# # 保存为文本文件，保留6位小数
# np.savetxt("image.txt", image, fmt="%.2f")

# print("已生成 image.txt")


import numpy as np

# 读取 image.txt 文件
image = np.loadtxt("image.txt")

# 对每个像素值执行 *0.5 + 10 的操作
image2 = image * 0.5 + 10

# 保存为 image2.txt，保留2位小数
np.savetxt("image2.txt", image2, fmt="%.2f")

print("已生成 image2.txt")
