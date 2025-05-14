import numpy as np
import matplotlib.pyplot as plt

# Read data from file
data = np.loadtxt('signal.dat')
print(data.shape)
time = data[:, 0] / 128  # Convert sample index to seconds (fs=128)
signal = data[:, 1]
print(time.shape)

x = []
y = []
with open("stft_fft_res.txt", "r") as file:
    for line in file:
        index, value = line.split()
        x.append(int(index))
        y.append(float(value))
x = x[:len(x)//2]
y = y[:len(y)//2]

# Create the plot
plt.figure(figsize=(8, 2))

plt.subplot(1, 2, 1)
plt.plot(time, signal, c='k')
plt.title('Signal Composition')
plt.xlim([min(time), max(time)])
plt.xlabel('Time (seconds)')
plt.ylabel('Amplitude')
# plt.grid(True)

plt.subplot(1, 2, 2)
plt.plot(x, y, label="|F(k)|^2", marker='o')
plt.xlim([0, 25])
plt.ylim([0, 1.5])
plt.title("Frequency Spectrum (abs_F)")
plt.xlabel("Frequency Index (k)")
plt.ylabel("|F(k)|^2")
plt.grid(True)
plt.legend()


# Add vertical lines to show the different frequency segments
T = 4
plt.axvline(x=T/4, color='r', linestyle='--', alpha=0.5)
plt.axvline(x=T/2, color='r', linestyle='--', alpha=0.5)
plt.axvline(x=3*T/4, color='r', linestyle='--', alpha=0.5)

plt.tight_layout()
plt.savefig('fft_stft_in_sig&fft.png', dpi=300, bbox_inches='tight')
