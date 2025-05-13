import numpy as np
import matplotlib.pyplot as plt

# 参数设置
K = 2
n = np.arange(0, 41)  # n从0到40
c = -1/12 + (np.pi/6)*1j  # 复数参数

# 生成复指数序列
x = K * np.exp(c * n)

# 绘图
plt.figure(figsize=(10, 8))

# 实部
# plt.subplot(3, 1, 1)
plt.stem(n, np.real(x))
plt.grid(True)
plt.title('Real part')
plt.savefig('real.png')

# 虚部
# plt.subplot(3, 1, 2)
plt.stem(n, np.imag(x))
plt.grid(True)
plt.title('Imaginary part')
plt.savefig('complex.png')

# 幅度
# plt.subplot(3, 1, 3)
plt.stem(n, np.abs(x))
plt.grid(True)
plt.title('Amplitude part')
plt.savefig('Amplitude.png')

# 显示图形
plt.tight_layout()
plt.show()
