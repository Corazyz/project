import numpy as np
import matplotlib.pyplot as plt

n = np.arange(0, 41)
x = 1.5 * np.sin(0.2 * np.pi * n)

plt.stem(n, x)
plt.axis([0, 40, -2, 2])
plt.grid(True)
plt.title('Sinusoidal Sequence')
plt.xlabel('Time index n')
plt.ylabel('Amplitude')

plt.savefig('sin_sampling.png')