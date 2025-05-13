import numpy as np

def seqfold(x, n):
    y = np.flip(x)
    n = -np.flip(n)
    return y, n