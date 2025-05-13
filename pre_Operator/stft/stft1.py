# import numpy as np
# import matplotlib.pyplot as plt

# def plot_time_domain():
#     """Plot the complete time domain signal"""
#     data = np.loadtxt('time_domain.dat', comments='#')
#     t, x = data[:, 0], data[:, 1]

#     fig, ax = plt.subplots(figsize=(8, 3))
#     ax.plot(t, x, 'k', linewidth=0.8)
#     ax.set_xlabel('Time (seconds)')
#     ax.set_ylabel('Amplitude')
#     ax.set_title('Time Domain Signal')
#     fig.tight_layout()
#     fig.savefig('stft_time_domain.png')
#     plt.close(fig)

# def plot_frequency_domain():
#     """Plot the complete frequency spectrum"""
#     data = np.loadtxt('stft_frequency_domain.dat', comments='#')
#     freq, X = data[:, 0], data[:, 1]

#     fig, ax = plt.subplots(figsize=(8, 3))
#     ax.plot(freq, X, 'k', linewidth=0.8)
#     ax.set_xlabel('Frequency (Hz)')
#     ax.set_ylabel('Magnitude')
#     ax.set_title('Frequency Spectrum')
#     ax.set_xlim(0, 25)
#     ax.set_ylim(0, 1.5)
#     fig.tight_layout()
#     fig.savefig('stft_frequency_domain.png')
#     plt.close(fig)

# def load_data():
#     # Load input signal
#     input_data = np.loadtxt('input_signal.txt')
#     time = input_data[:, 0]
#     signal = input_data[:, 1]

#     # Load STFT magnitude
#     stft_mag = np.loadtxt('stft_magnitude.txt')

#     # Load frequency vector
#     freq = np.loadtxt('frequency_vector.txt')

#     # Load time vector
#     t = np.loadtxt('time_vector.txt')

#     return time, signal, stft_mag, freq, t

# def plot_results(time, signal, stft_mag, freq, t):
#     plt.figure(figsize=(12, 8))

#     # Plot input signal
#     plt.subplot(2, 1, 1)
#     plt.plot(time, signal)
#     plt.title('Input Signal (50Hz + 120Hz from sample 50-150)')
#     plt.xlabel('Time (s)')
#     plt.ylabel('Amplitude')
#     plt.grid(True)

#     # Plot STFT spectrogram
#     plt.subplot(2, 1, 2)
#     extent = [t[0], t[-1], freq[0], freq[-1]]
#     plt.imshow(stft_mag.T, aspect='auto', origin='lower', extent=extent)
#     plt.title('STFT Magnitude Spectrogram')
#     plt.xlabel('Time (s)')
#     plt.ylabel('Frequency (Hz)')
#     plt.colorbar(label='Magnitude')
#     plt.tight_layout()

#     # Save the figure as a file
#     plt.savefig('signal_analysis.png', dpi=300, bbox_inches='tight')
#     plt.close()  # Close the figure to free memory

# def main():
#     print("Generating plots from data files...")
#     plot_time_domain()
#     plot_frequency_domain()
#     time, signal, stft_mag, freq, t = load_data()
#     plot_results(time, signal, stft_mag, freq, t)
#     print("Plots saved as PNG files")

# if __name__ == "__main__":
#     main()

# stft_visualization.py
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from pathlib import Path
from scipy import signal

# ------------------------------------------------------------
# 1. 读取数据
# ------------------------------------------------------------
files = {
    'time_domain'  : 'stft_time_domain.dat',        # t, x(t)
    'stft_mag'     : 'stft_magnitude.txt',     # L × nfft
    'freq_vector'  : 'frequency_vector.txt',   # nfft
    'time_vector'  : 'time_vector.txt'         # L
}

# 检查文件是否存在
for k, v in files.items():
    if not Path(v).is_file():
        raise FileNotFoundError(f'未找到文件: {v}')

time, x = np.loadtxt(files['time_domain'], comments='#', unpack=True)
f_vec      = np.loadtxt(files['freq_vector'])
t_vec      = np.loadtxt(files['time_vector'])
stft_mag   = np.loadtxt(files['stft_mag'])       # shape: (L, nfft)

# stft_mag 的大小检查（简易）
if stft_mag.shape != (t_vec.size, f_vec.size):
    raise ValueError(
        f'STFT 形状 {stft_mag.shape} 与时间/频率向量 {t_vec.size, f_vec.size} 不匹配'
    )

# ------------------------------------------------------------
# 2. 绘图
# ------------------------------------------------------------
plt.rcParams.update({'font.size': 11})

fig, axes = plt.subplots(3, 1, figsize=(10, 12), gridspec_kw={'height_ratios': [1, 3, 1]})

# ---- 2.1 时域信号 ----
axes[0].plot(time, x, lw=1)
axes[0].set_title('Time-Domain Signal')
axes[0].set_xlabel('Time  [s]')
axes[0].set_ylabel('Amplitude')
axes[0].grid(True, ls=':')

# ---- 2.2 幅度谱图 (Spectrogram) ----
# imshow 需要 (freq, time)，因此转置
extent = [t_vec[0], t_vec[-1], f_vec[0], f_vec[-1]]   # [xmin, xmax, ymin, ymax]
im = axes[1].imshow(
    stft_mag.T, origin='lower', aspect='auto',
    extent=extent, cmap='jet'
)
axes[1].set_title('STFT Magnitude Spectrogram')
axes[1].set_xlabel('Time  [s]')
axes[1].set_ylabel('Frequency  [Hz]')
cbar = fig.colorbar(im, ax=axes[1], pad=0.01)
cbar.set_label('Magnitude')

# y 轴刻度美化：只显示整数频率（如需要，可注释掉）
axes[1].yaxis.set_major_locator(ticker.MaxNLocator(integer=True))

# ---- 2.3 帧功率 vs. 时间（可选） ----
frame_power = np.mean(stft_mag**2, axis=1)   # 或 np.sum
axes[2].plot(t_vec, frame_power, '-k')
axes[2].set_title('Frame Power (mean of |STFT|²)')
axes[2].set_xlabel('Time  [s]')
axes[2].set_ylabel('Power')
axes[2].grid(True, ls=':')

plt.tight_layout()
plt.savefig("signal_stft_freq.png", dpi=300, bbox_inches='tight')
