import numpy as np
# n = np.arange(-10, 21)
# delta = np.concatenate((np.zeros(10), [1], np.zeros(20)))

# plt.stem(n, delta)
# plt.grid(True)

# plt.xlabel('Time index n')
# plt.ylabel('Amplitude')
# plt.title('Unit Sample Sequence')

# plt.axis([-10, 20, 0, 1.2])

# plt.savefig('dst.png', dpi=300, bbox_inches='tight')
def impseq(nd, ns, nf):
    if ns > nd or nd > nf:
        raise ValueError("arg must satisfy ns <= nd <= nf")

    n = np.arange(ns, nf + 1)

    x = (n==nd).astype(int)

    return x, n
