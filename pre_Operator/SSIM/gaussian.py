import numpy as np
from scipy.ndimage import gaussian_filter
import matplotlib.pyplot as plt

# 生成测试图像
image_loaded = np.loadtxt("image.txt")
print(f"\n 读取图像形状：{image_loaded.shape}")

# Python 高斯滤波
sigma = 1.5
truncate = 3.5
output_py = gaussian_filter(image_loaded, sigma=sigma, mode='reflect', cval=0, truncate=truncate)

print("\nPython scipy 结果：")
print(output_py)

# 保存为文件供 C 读取（可选）
np.savetxt("output_py.txt", output_py, fmt="%.2f")
