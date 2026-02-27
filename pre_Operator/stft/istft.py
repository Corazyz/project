# import numpy as np
# import matplotlib.pyplot as plt

# def read_idft_result(filename):
#     """Read the IDFT result file and return the data"""
#     indices = []
#     real_parts = []
#     imag_parts = []

#     with open(filename, 'r') as f:
#         for line in f:
#             if not line.startswith('#'):  # Skip comment lines
#                 parts = line.strip().split('\t')
#                 if len(parts) == 3:  # Ensure we have index, real, imag
#                     indices.append(int(parts[0]))
#                     real_parts.append(float(parts[1]))
#                     imag_parts.append(float(parts[2]))

#     return np.array(indices), np.array(real_parts), np.array(imag_parts)

# def plot_idft_result(filename):
#     """Plot the IDFT result from the data file"""
#     indices, real_parts, imag_parts = read_idft_result(filename)

#     plt.figure(figsize=(12, 6))

#     # Plot real and imaginary parts
#     plt.subplot(1, 2, 1)
#     plt.plot(indices, real_parts, 'b-', label='Real part')
#     plt.xlabel('Index')
#     plt.ylabel('Amplitude')
#     plt.title('Real Part of IDFT Result')
#     plt.grid(True)
#     plt.legend()

#     plt.subplot(1, 2, 2)
#     plt.plot(indices, imag_parts, 'r-', label='Imaginary part')
#     plt.xlabel('Index')
#     plt.ylabel('Amplitude')
#     plt.title('Imaginary Part of IDFT Result')
#     plt.grid(True)
#     plt.legend()

#     plt.tight_layout()
#     plt.savefig("idft_result.png", dpi=300, bbox_inches='tight')

# # Plot the IDFT result
# plot_idft_result('idft_result.dat')

import numpy as np
import matplotlib.pyplot as plt

def plot_reconstructed_signal():
    # Read data from file
    try:
        data = np.loadtxt('reconstructed_signal.txt')
    except FileNotFoundError:
        print("Error: File 'reconstructed_signal.txt' not found.")
        return
    except Exception as e:
        print(f"Error reading file: {e}")
        return

    # Create time axis (assuming sampling rate is known)
    # If you know the actual sampling rate, replace 1.0 with your fs
    fs = 200  # Default to 1 Hz if not specified
    time = np.arange(len(data)) / fs

    # Create the plot
    plt.figure(figsize=(12, 6))
    plt.plot(time, data, linewidth=1)
    plt.title('Reconstructed Signal from ISTFT')
    plt.xlabel('Time (seconds)')
    plt.ylabel('Amplitude')
    plt.grid(True)

    # Save the plot to a file
    plt.savefig('reconstructed_signal_plot.png', dpi=300, bbox_inches='tight')
    print("Plot saved as 'reconstructed_signal_plot.png'")

    # Optionally show the plot
    plt.show()

if __name__ == "__main__":
    plot_reconstructed_signal()
