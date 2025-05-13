import numpy as np
import matplotlib.pyplot as plt
import glob

# Basic matplotlib styling
plt.rcParams['figure.dpi'] = 150
plt.rcParams['savefig.dpi'] = 300
plt.rcParams['font.size'] = 10
plt.rcParams['axes.labelsize'] = 10
plt.rcParams['axes.titlesize'] = 11
plt.rcParams['xtick.labelsize'] = 9
plt.rcParams['ytick.labelsize'] = 9
plt.rcParams['axes.grid'] = True
plt.rcParams['grid.alpha'] = 0.3

def plot_time_domain():
    """Plot the complete time domain signal"""
    data = np.loadtxt('stft_time_domain.dat', comments='#')
    t, x = data[:, 0], data[:, 1]

    fig, ax = plt.subplots(figsize=(8, 3))
    ax.plot(t, x, 'k', linewidth=0.8)
    ax.set_xlabel('Time (seconds)')
    ax.set_ylabel('Amplitude')
    ax.set_title('Time Domain Signal')
    fig.tight_layout()
    fig.savefig('stft_time_domain.png')
    plt.close(fig)

def plot_frequency_domain():
    """Plot the complete frequency spectrum"""
    data = np.loadtxt('stft_frequency_domain.dat', comments='#')
    freq, X = data[:, 0], data[:, 1]

    fig, ax = plt.subplots(figsize=(8, 3))
    ax.plot(freq, X, 'k', linewidth=0.8)
    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Magnitude')
    ax.set_title('Frequency Spectrum')
    ax.set_xlim(0, 25)
    ax.set_ylim(0, 1.5)
    fig.tight_layout()
    fig.savefig('stft_frequency_domain.png')
    plt.close(fig)

def plot_windowed_analysis():
    """Plot all windowed analyses"""
    time_files = sorted(glob.glob('stft_windowed_time_pos_*_sec.dat'))

    for time_file in time_files:
        # Extract window position from filename
        pos = time_file.split('_')[4]
        freq_file = f'stft_windowed_freq_pos_{pos}_sec.dat'

        # Load time domain data
        t_data = np.loadtxt(time_file, comments='#')
        t, x, w = t_data[:, 0], t_data[:, 1], t_data[:, 2]

        # Load frequency domain data
        f_data = np.loadtxt(freq_file, comments='#')
        freq, X = f_data[:, 0], f_data[:, 1]

        # Create figure
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4))

        # Time domain plot
        ax1.plot(t, x, 'k', linewidth=0.8, label='Signal')
        ax1.plot(t, w, 'r', linewidth=0.8, label='Window')
        ax1.set_xlabel('Time (seconds)')
        ax1.set_ylabel('Amplitude')
        ax1.set_title(f'Windowed Signal at {pos} sec')
        ax1.set_ylim(-1.1, 1.1)
        ax1.legend()

        # Frequency domain plot
        ax2.plot(freq, X, 'k', linewidth=0.8)
        ax2.set_xlabel('Frequency (Hz)')
        ax2.set_ylabel('Magnitude')
        ax2.set_title('Windowed Spectrum')
        ax2.set_xlim(0, 25)
        ax2.set_ylim(0, 1.5)

        fig.tight_layout()
        fig.savefig(f'stft_windowed_analysis_pos_{pos}_sec.png')
        plt.close(fig)

def main():
    print("Generating plots from data files...")
    plot_time_domain()
    plot_frequency_domain()
    plot_windowed_analysis()
    print("Plots saved as PNG files")

if __name__ == "__main__":
    main()
