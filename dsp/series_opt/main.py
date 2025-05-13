import numpy as np
import matplotlib.pyplot as plt
from series_add import seqadd
from delta import impseq
from series_mult import seqmult
from series_flip import seqfold
from scipy.io import wavfile
from awgn import awgn
from moving_avg import moving_average_filter

if __name__ == "__main__":
    fs, x = wavfile.read('sample-6s.wav')
    y, fd = seqfold(x, fs)

    # 绘制原始音频波形
    plt.figure(1)
    plt.plot(x)
    plt.grid(True)
    plt.title('Original Audio')
    plt.savefig('src_wave.png', dpi=300)

    # 绘制反转后的音频波形
    # plt.figure(2)
    # plt.plot(y)
    # plt.grid(True)
    # plt.title('Reversed Audio')
    # plt.savefig('dst_wave.png', dpi=300)
    test1 = [1, 2, 3, 4, 5]
    test2 = test1 / np.max(np.abs(test1))
    test3 = test2 ** 2
    test4 = np.mean(test3)
    print("test2 = ", test2)
    print("test3 = ", test3)
    print("test4 = ", test4)
    # 添加噪声
    snr = 30
    waveData2 = awgn(x, snr, out = 'signal', method = 'vectorized', axis = 0)
    filtered_signal = moving_average_filter(waveData2)
    plt.figure(2)
    plt.plot(waveData2)
    plt.grid(True)
    plt.title('awgn_signal')
    plt.savefig('awgn.png', dpi=300)

    plt.figure(3)
    plt.plot(filtered_signal)
    plt.grid(True)
    plt.title('filtered_signal')
    plt.savefig('filtered.png', dpi=300)

    # 保存反转后的音频文件
    # wavfile.write('w4.wav', fs, y)
    wavfile.write('filtered_snr_30.wav', fs, waveData2)
    # nd = 2
    # ns = 0
    # nf = 5
    # x, n = impseq(nd, ns, nf)
    # print("n:", n)
    # print("x:", x)
    # plt.stem(n, x)
    # plt.grid(True)

    # plt.xlabel('Time index n')
    # plt.ylabel('Amplitude')
    # plt.title('Unit Sample Sequence')

    # plt.axis([ns, nf, 0, 2.3])
    # plt.savefig('dst.png', dpi=300, bbox_inches='tight')

    # x1 = np.array([1, 2, 3])
    # n1 = np.array([0, 1, 2])
    # x2 = np.array([4, 5, 6])
    # n2 = np.array([1, 2, 3])

    # # y, n = seqmult(x1, n1, x2, n2)
    # y, n = seqfold(x1, n1)

    # print("y:", y)
    # print("n:", n)
    # plt.stem(n, y)
    # plt.grid(True)
    # plt.xlabel('Time index n')
    # plt.ylabel('Amplitude')
    # plt.title('Added Sequence')

    # plt.axis([-4, 4, 0, 4])
    # plt.savefig('dst.png', dpi=300, bbox_inches='tight')