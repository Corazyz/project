import numpy as np

def seqmult(x1, n1, x2, n2):
    n_min = min(min(n1), min(n2))
    n_max = max(max(n1), max(n2))
    n = np.arange(n_min, n_max + 1)

    y1 = np.zeros(len(n))
    y2 = np.zeros(len(n))

    y1[(n >= min(n1)) & (n <= max(n1))] = x1
    y2[(n >= min(n2)) & (n <= max(n2))] = x2

    y = y1 * y2

    return y, n