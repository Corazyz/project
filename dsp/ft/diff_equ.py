import numpy as np
from scipy.signal import lfilter
import matplotlib.pyplot as plt

b = [0.5]
a = [1, -0.5]

x = np.ones(100)
y = lfilter(b, a, x)

print("input signal x(n):", x)
print("output signal y(n):", y)

plt.figure(figsize=(10, 5))
plt.plot(x, label='Input x(n)')
plt.plot(y, label='Output y(n)')
plt.xlabel('n')
plt.ylabel('Amplitude')
plt.title('Defference Equation Response')
plt.legend()
plt.grid()
plt.savefig('diff_equ.png')