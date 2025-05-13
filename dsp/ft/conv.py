import numpy as np
import matplotlib.pyplot as plt

# nx = np.arange(0, 2)
# x = np.array([1, 2])

# nh = np.arange(0, 3)
# h = np.array([3, 2, 1])

# y = np.convolve(x, h)
# ny = np.arange(0, len(y))

# plt.figure(figsize = (10, 8))

# plt.subplot(311)
# plt.stem(nx, x, linefmt='b-', markerfmt='bo', basefmt='k-')
# plt.title('x(n)')
# plt.xlabel('n')
# plt.ylabel('x(n)')
# plt.axis([min(nx), max(nx), 0, max(x) + 1])
# plt.grid(True)

# plt.subplot(312)
# plt.stem(nh, h, linefmt='g-', markerfmt='go', basefmt='k-')
# plt.title('h(n)')
# plt.xlabel('n')
# plt.ylabel('h(n)')
# plt.axis([min(nh), max(nh), 0, max(h) + 1])
# plt.grid(True)

# # 绘制 y(n)
# plt.subplot(313)
# plt.stem(ny, y, linefmt='r-', markerfmt='ro', basefmt='k-')
# plt.title('y(n)')
# plt.xlabel('n')
# plt.ylabel('y(n)')
# plt.axis([min(ny), max(ny), 0, max(y) + 1])
# plt.grid(True)

# plt.tight_layout()
# plt.savefig('convolution_plt.png')

# cross correlation
x = np.array([3, 5, -7, 2, -1, -3, 2])
n0 = 2
y0 = np.roll(x, n0)
y0[:n0] = 0

w = np.random.randn(len(y0))
y = y0 + w
ryx = np.correlate(y, x, mode = 'full')
nryx = np.arange(-len(x) + 1, len(x))

plt.stem(nryx, ryx, 'r', basefmt=' ')
plt.xlabel('nryx')
plt.ylabel('ryx')
plt.title('cross correlation')
plt.grid()
plt.savefig('cross_cor.png')