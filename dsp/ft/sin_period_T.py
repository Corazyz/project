import numpy as np
import matplotlib.pyplot as plt

A = 1
m = 0.2
wL = 0.01 * np.pi
wH = 0.2 * np.pi

n = np.arange(1000)

x = A * (1 + m * np.cos(wL * n)) * np.cos(wH * n)
# x = A * np.cos(wH * n)
# x = A * np.cos(wL * n)

plt.stem(n, x, linefmt = 'r', markerfmt = 'go', basefmt = 'k')
# plt.stem(n, x)
plt.grid(False)
plt.xlabel('n')
plt.ylabel('x(n)')
plt.title('Discrete Signal')
plt.savefig('cos_peried.png')