import matplotlib.pyplot as plt

def compare_dat_files(file_a, file_b, output_file='diff_result.txt', plot_comparison=True):
    """
    比较两个.dat文件中的幅值差值，并将结果保存到输出文件中

    参数:
        file_a (str): 第一个数据文件路径
        file_b (str): 第二个数据文件路径
        output_file (str): 结果输出文件路径
        plot_comparison (bool): 是否绘制比较图表
    """
    try:
        # 读取文件a的数据，跳过可能存在的标题行
        with open(file_a, 'r') as fa:
            lines_a = [line.strip() for line in fa if line.strip()]
            # 检查第一行是否是标题
            if lines_a[0].lower().startswith(('frequency', '#')):
                lines_a = lines_a[1:]
            data_a = []
            freqs_a = []
            for line in lines_a:
                parts = line.split()
                freqs_a.append(float(parts[0]))
                data_a.append(float(parts[1]))

        # 读取文件b的数据
        with open(file_b, 'r') as fb:
            lines_b = [line.strip() for line in fb if line.strip()]
            if lines_b[0].lower().startswith(('frequency', '#')):
                lines_b = lines_b[1:]
            data_b = []
            freqs_b = []
            for line in lines_b:
                parts = line.split()
                freqs_b.append(float(parts[0]))
                data_b.append(float(parts[1]))

        # 检查频率点是否匹配
        freq_mismatch = False
        if len(freqs_a) != len(freqs_b):
            freq_mismatch = True
        else:
            for fa, fb in zip(freqs_a, freqs_b):
                if abs(fa - fb) > 1e-3:  # 允许小的浮点误差
                    freq_mismatch = True
                    break

        if freq_mismatch:
            print("警告: 文件的频率点不完全匹配，将只比较共有的频率点")
            # 找出共同的频率点
            common_freqs = set(freqs_a) & set(freqs_b)
            # 重新组织数据
            data_a = [data_a[freqs_a.index(f)] for f in common_freqs]
            data_b = [data_b[freqs_b.index(f)] for f in common_freqs]
            freqs = sorted(common_freqs)
        else:
            freqs = freqs_a

        # 计算差值并统计结果
        differences = []
        max_diff = 0.01
        max_diff_index = 0
        max_diff_freq = 0
        sum_diff = 0
        count = 0
        for i, (a, b) in enumerate(zip(data_a, data_b)):
            diff = abs(a - b)
            differences.append(diff)
            sum_diff += diff
            if diff > max_diff:
                count += 1
                max_diff = diff
                max_diff_index = i
                max_diff_freq = freqs[i]

        avg_diff = sum_diff / len(differences) if differences else 0

        # 写入结果到文件
        with open(output_file, 'w') as fout:
            fout.write(f"比较结果: {file_a} vs {file_b}\n")
            fout.write(f"总数据点: {len(differences)}\n")
            fout.write(f"平均差值: {avg_diff:.6f}\n")
            fout.write(f"最大差值: {max_diff:.6f} (位于频率 {max_diff_freq:.6f} Hz)\n")
            fout.write("\n详细差值:\n")
            fout.write("Frequency(Hz)\tMagnitude_A\tMagnitude_B\tDifference\n")
            for i, diff in enumerate(differences):
                fout.write(f"{freqs[i]:.6f}\t{data_a[i]:.6f}\t{data_b[i]:.6f}\t{diff:.6f}\n")

        print(f"比较完成，结果已保存到 {output_file}")
        print(f"平均差值: {avg_diff:.6f}")
        print(f"最大差值: {max_diff:.6f} (位于频率 {max_diff_freq:.6f} Hz)")

        # 绘制比较图表
        if plot_comparison:
            plt.figure(figsize=(12, 6))

            # 绘制原始数据
            plt.subplot(1, 2, 1)
            plt.plot(freqs, data_a, 'b-', label=file_a)
            plt.plot(freqs, data_b, 'r--', label=file_b)
            plt.xlabel('Frequency (Hz)')
            plt.ylabel('Magnitude')
            plt.title('Magnitude Comparison')
            plt.legend()
            plt.grid(True)

            # 绘制差值
            plt.subplot(1, 2, 2)
            plt.plot(freqs, differences, 'g-')
            plt.xlabel('Frequency (Hz)')
            plt.ylabel('Magnitude Difference')
            plt.title('Magnitude Differences')
            plt.grid(True)

            plt.tight_layout()
            plt.savefig('magnitude_comparison.png')
            plt.close()
            print("比较图表已保存为 magnitude_comparison.png")

    except Exception as e:
        print(f"发生错误: {str(e)}")

# 使用示例
if __name__ == "__main__":
    compare_dat_files('frequency_domain.dat', 'stft_frequency_domain.dat')

# import numpy as np
# import matplotlib.pyplot as plt

# def read_dat_file(filename):
#     """读取.dat文件，返回频率和幅值数组"""
#     data = np.loadtxt(filename, skiprows=1)  # 跳过标题行
#     freq = data[:, 0]
#     mag = data[:, 1]
#     return freq, mag

# def compare_dat_files(file1, file2):
#     """比较两个.dat文件并计算差值"""
#     # 读取数据
#     freq1, mag1 = read_dat_file(file1)
#     freq2, mag2 = read_dat_file(file2)

#     # 检查频率点是否相同
#     if not np.allclose(freq1, freq2):
#         print("警告：两个文件的频率点不完全相同")

#     # 计算差值
#     mag_diff = mag1 - mag2

#     # 打印结果
#     print("频率(Hz)\t文件1幅值\t文件2幅值\t差值")
#     for f, m1, m2, diff in zip(freq1, mag1, mag2, mag_diff):
#         print(f"{f:.6f}\t{m1:.6f}\t{m2:.6f}\t{diff:.6f}")

#     # 绘制对比图
#     plt.figure(figsize=(10, 6))
#     plt.plot(freq1, mag1, 'b-', label='文件1')
#     plt.plot(freq2, mag2, 'r--', label='文件2')
#     plt.plot(freq1, mag_diff, 'g:', label='差值')
#     plt.xlabel('Frequency (Hz)')
#     plt.ylabel('Magnitude')
#     plt.title('文件1、文件2及差值对比')
#     plt.legend()
#     plt.grid(True)
#     plt.show()

# # 使用示例
# file1 = 'frequency_domain.dat'  # 替换为第一个文件路径
# file2 = 'stft_frequency_domain.dat'  # 替换为第二个文件路径
# compare_dat_files(file1, file2)
