import numpy as np
import matplotlib.pyplot as plt

# 读取输入信号
input_signal = np.loadtxt("input_signal.txt")
time = input_signal[:, 0]
signal = input_signal[:, 1]

# 读取 STFT 幅度
stft_magnitude = np.loadtxt("stft_magnitude.txt")

# 绘制输入信号
plt.figure(figsize=(12, 6))
plt.subplot(2, 1, 1)
plt.plot(time, signal, label="Input Signal")
plt.title("Input Signal")
plt.xlabel("Time (s)")
plt.ylabel("Amplitude")
plt.grid()
plt.legend()

# 绘制 STFT 幅度
plt.subplot(2, 1, 2)
plt.plot(stft_magnitude, label="STFT Magnitude")
plt.title("STFT Magnitude")
plt.xlabel("Frequency Bin")
plt.ylabel("Magnitude")
plt.grid()
plt.legend()

plt.tight_layout()
plt.savefig("fft_stft.png")
plt.close()
