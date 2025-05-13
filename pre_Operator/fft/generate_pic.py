# import numpy as np
# import matplotlib.pyplot as plt

# # 设置单位圆
# theta = np.linspace(0, 2 * np.pi, 500)
# x = np.cos(theta)
# y = np.sin(theta)

# # 定义复数 ζ = e^(-2πi/N)
# N = 8  # 例如 N=8
# k = 1  # 选择第一个点
# zeta_real = np.cos(-2 * np.pi * k / N)
# zeta_imag = np.sin(-2 * np.pi * k / N)

# # 创建图形
# plt.figure(figsize=(6, 6))

# # 绘制单位圆
# plt.plot(x, y, label="单位圆", color="gold")

# # 绘制坐标轴
# plt.axhline(0, color='black', linewidth=0.8, linestyle="--")
# plt.axvline(0, color='black', linewidth=0.8, linestyle="--")

# # 绘制复数 ζ 的向量
# plt.quiver(0, 0, zeta_real, zeta_imag, angles='xy', scale_units='xy', scale=1, color="gray", label=r"$\zeta$")

# # 标注单位圆上的关键点
# plt.text(1.1, 0, "1", fontsize=12)
# plt.text(-1.3, 0, "-1", fontsize=12)
# plt.text(0.1, 1.1, "i", fontsize=12)
# plt.text(0.1, -1.3, "-i", fontsize=12)

# # 添加公式 ζ = e^(-2πi/N)
# plt.text(0.5, -1.5, r"$\zeta = e^{-2\pi i / N}$", fontsize=14, color="black")

# # 设置图形范围和比例
# plt.xlim(-1.5, 1.5)
# plt.ylim(-1.5, 1.5)
# plt.gca().set_aspect('equal', adjustable='box')

# # 隐藏网格线
# plt.grid(False)

# # 保存为文件
# plt.savefig("complex_unit_circle.png", dpi=300, bbox_inches='tight', transparent=True)
#   # 保存为 PNG 文件，分辨率为 300 DPI

# # 如果不需要显示图片，可以注释掉 plt.show()
# # plt.show()

import numpy as np
import matplotlib.pyplot as plt

# 创建一个新的图形
plt.figure(figsize=(6, 6))

# 创建一个单位圆
circle = plt.Circle((0, 0), 1, fill=False, color='yellow')
fig = plt.gcf()
fig.gca().add_artist(circle)

# 计算8次单位根
n = 8
roots = np.exp(2j * np.pi * np.arange(n) / n)

# 绘制箭头和标注
for i, root in enumerate(roots):
    plt.arrow(0, 0, root.real, root.imag, head_width=0.05, head_length=0.1, fc='k', ec='k')
    plt.text(root.real + 0.1, root.imag + 0.1, f'$\zeta^{i}$', fontsize=12)

# 设置坐标轴范围和网格
plt.xlim(-1.5, 1.5)
plt.ylim(-1.5, 1.5)
plt.grid(True)

# 显示图形
plt.savefig("complex_unit_circle1.png", dpi=300, bbox_inches='tight', transparent=True)
