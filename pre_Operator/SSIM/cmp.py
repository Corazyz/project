import numpy as np
from skimage.filters import gaussian
from scipy.ndimage import gaussian_filter

# 读取图像
with open('image.txt', 'r') as f:
    lines = f.readlines()
data = []
for line in lines:
    row = [float(x) for x in line.strip().split()]
    data.append(row)
img = np.array(data, dtype=np.float32)

# skimage 版本 —— 注意：2D 图像不需要 channel_axis
filtered_sk = gaussian(
    img,
    sigma=1.5,
    mode='reflect',
    truncate=3.5,
    preserve_range=True
)

# scipy 版本 —— 2D 图像，sigma 为标量或 (1, 1)
filtered_sc = gaussian_filter(
    img,
    sigma=1.5,           # 或 sigma=(1, 1)
    mode='reflect',
    truncate=3.5
)

np.savetxt('filtered_sk.txt', filtered_sk, fmt='%.2f', delimiter=' ')
np.savetxt('filtered_sc.txt', filtered_sc, fmt='%.2f', delimiter=' ')
# 比较
diff = np.abs(filtered_sk - filtered_sc)
print("最大差异:", diff.max())  # 应该非常小（< 1e-10）
