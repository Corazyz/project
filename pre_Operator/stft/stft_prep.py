# import os
# import numpy as np
# import matplotlib
# from matplotlib import pyplot as plt

# Fs = 128
# duration = 10
# omega1 = 1
# omega2 = 5
# N = int(duration * Fs)
# print("number of sampled signal N = %d" % N)
# t = np.arange(N)/Fs
# t1 = t[:N//2]
# t2 = t[N//2:]

# x1 = 1.0 * np.sin(2 * np.pi * omega1 * t1 + 0.01 * np.pi)
# x2 = 0.7 * np.sin(2 * np.pi * omega2 * t2 + 0.05 * np.pi)
# x = np.concatenate((x1, x2))

# plt.figure(figsize=(8, 2))
# plt.subplot(1, 2, 1)
# plt.plot(t, x, c='k')
# plt.xlim([min(t),max(t)])
# plt.xlabel("Time (seconds)")

# plt.subplot(1, 2, 2)
# X = np.abs(np.fft.fft(x)) / Fs
# freq = np.fft.fftfreq(N, d=1/Fs)
# # print(freq[N//2])
# # X = X[:N//2]
# # freq = freq[:N//2]
# plt.plot(freq, np.abs(X), c='k')
# plt.xlim([-7, 7])
# plt.ylim([0, 3])
# plt.xlabel('Frequency (Hz)')
# plt.tight_layout()

# plt.savefig("time_frequancy_plot.png", dpi=300, bbox_inches='tight')
# plt.close()

# def windowed_ft(t, x, Fs, w_pos_sec, w_len):
#     N = len(x)
#     w_pos = int(Fs * w_pos_sec)
#     w_padded = np.zeros(N)
#     # w_padded[w_pos:w_pos + w_len] = 1
#     for i in range(w_len):
#         w_padded[w_pos + i] = 0.5 * (1 - np.cos(2 * np.pi * i / (w_len - 1)))
#     x = x * w_padded
#     plt.figure(figsize=(8, 2))

#     plt.subplot(1, 2, 1)
#     plt.plot(t, x, c='k')
#     plt.plot(t, w_padded, c='r')
#     plt.xlim([min(t), max(t)])
#     plt.ylim([-1.1, 1.1])
#     plt.xlabel('Time(seconds)')

#     plt.subplot(1, 2, 2)
#     X = np.abs(np.fft.fft(x))/Fs
#     freq = np.abs(np.fft.fftfreq(N, d=1/Fs))
#     X = X[:N//2]
#     freq = freq[:N//2]
#     plt.plot(freq, X, c='k')
#     plt.xlim([0, 7])
#     plt.ylim([0, 3])
#     plt.xlabel('Frequency (Hz)')
#     plt.tight_layout()

#     filename = f'windowed_signal_pos_{w_pos_sec}_sec.png'
#     plt.savefig(filename, dpi=300, bbox_inches='tight')
#     plt.close()

# w_len = 4 * Fs
# windowed_ft(t, x, Fs, w_pos_sec=1, w_len=w_len)
# windowed_ft(t, x, Fs, w_pos_sec=3, w_len=w_len)
# windowed_ft(t, x, Fs, w_pos_sec=5, w_len=w_len)
# # print('Interactive interface for experimenting with different window shifts:')
# # interact(windowed_ft, )

import os
import numpy as np
import matplotlib
from matplotlib import pyplot as plt
import scipy
import scipy.signal

scipy.signal.stft()

# Fs = 128
# duration = 10
# omega1 = 1
# omega2 = 5
# N = int(duration * Fs)
# print("number of sampled signal N = %d" % N)
# t = np.arange(N)/Fs
# t1 = t[:N//2]
# t2 = t[N//2:]

# x1 = 1.0 * np.sin(2 * np.pi * omega1 * t1 + 0.01 * np.pi)
# x2 = 0.7 * np.sin(2 * np.pi * omega2 * t2 + 0.05 * np.pi)
Fs = 100
duration = 4
omega1 = 5
omega2 = 10
omega3 = 15
omega4 = 20
N = int(duration * Fs)
t = np.arange(N)/Fs
t1 = t[:N//4]
t2 = t[N//4:N//2]
t3 = t[N//2:N*3//4]
t4 = t[N*3//4:]

x1 = np.sin(2 * np.pi * omega1 * t1)
x2 = 0.5 * np.sin(2 * np.pi * omega2 * t2)
x3 = 0.7 * np.sin(2 * np.pi * omega3 * t3)
x4 = 1.2 * np.sin(2 * np.pi * omega4 * t4)
x = np.concatenate((x1, x2, x3, x4))

plt.figure(figsize=(8, 2))
plt.subplot(1, 2, 1)
plt.plot(t, x, c='k')
plt.xlim([min(t),max(t)])
plt.xlabel("Time (seconds)")

plt.subplot(1, 2, 2)
X = np.abs(np.fft.fft(x)) / Fs
freq = np.fft.fftfreq(N, d=1/Fs)
# print(freq[N//2])
X = X[:N//2]
freq = freq[:N//2]
plt.plot(freq, np.abs(X), c='k')
plt.xlim([0, 25])
plt.ylim([0, 1])
plt.xlabel('Frequency (Hz)')
plt.tight_layout()

plt.savefig("time_frequancy_plot.png", dpi=300, bbox_inches='tight')
plt.close()

def windowed_ft(t, x, Fs, w_pos_sec, w_len):
    N = len(x)
    w_pos = int(Fs * w_pos_sec)
    w_padded = np.zeros(N)
    # w_padded[w_pos:w_pos + w_len] = 1
    for i in range(w_len):
        w_padded[w_pos + i] = 0.5 * (1 - np.cos(2 * np.pi * i / (w_len - 1)))
    x = x * w_padded
    plt.figure(figsize=(8, 2))

    plt.subplot(1, 2, 1)
    plt.plot(t, x, c='k')
    plt.plot(t, w_padded, c='r')
    plt.xlim([min(t), max(t)])
    plt.ylim([-1.1, 1.1])
    plt.xlabel('Time(seconds)')

    plt.subplot(1, 2, 2)
    X = np.abs(np.fft.fft(x))/Fs
    freq = np.abs(np.fft.fftfreq(N, d=1/Fs))
    X = X[:N//2]
    freq = freq[:N//2]
    plt.plot(freq, X, c='k')
    plt.xlim([0, 25])
    plt.ylim([0, 1])
    plt.xlabel('Frequency (Hz)')
    plt.tight_layout()

    filename = f'windowed_signal_pos_{w_pos_sec}_sec.png'
    plt.savefig(filename, dpi=300, bbox_inches='tight')
    plt.close()

w_len = Fs
windowed_ft(t, x, Fs, w_pos_sec=1, w_len=w_len)
windowed_ft(t, x, Fs, w_pos_sec=2, w_len=w_len)
windowed_ft(t, x, Fs, w_pos_sec=3, w_len=w_len)
windowed_ft(t, x, Fs, w_pos_sec=2.5, w_len=w_len)
# print('Interactive interface for experimenting with different window shifts:')
# interact(windowed_ft, )