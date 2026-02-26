import numpy as np
import matplotlib.pyplot as plt
# import scipy
# import scipy.signal

# scipy.signal.stft

# input_signal = np.loadtxt("input_signal.txt")

frequency_vector = np.loadtxt("frequency_vector.txt")
time_vector = np.loadtxt("time_vector.txt")

stft_result = np.loadtxt("stft_magnitude.txt")
# stft_result = np.loadtxt("stft_result.txt")

# plt.figure(figsize=(10, 4))
# plt.plot(input_signal)
# plt.title("Input Signal")
# plt.xlabel("Sample Index")
# plt.ylabel("Amplitude")
# plt.savefig("input_signal.png")

plt.figure(figsize=(10, 4))
plt.imshow(stft_result.T, aspect='auto', extent=[time_vector[0], time_vector[-1], frequency_vector[0], frequency_vector[-1]], origin='lower', cmap='jet')
plt.colorbar(label="Magnitude")
plt.title("STFT Magnitude")
plt.xlabel("Time (s)")
plt.ylabel("frequency (Hz)")
plt.savefig("stft_result.png")