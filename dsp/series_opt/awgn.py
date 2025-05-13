import numpy as np

def awgn(x, snr, out='signal', method='vectorized', axis=0):
    # Normalize the input signal to avoid numerical issues
    # scale = np.max(np.abs(x))
    x = x / np.max(np.abs(x))

    # Calculate signal power
    if method == 'vectorized':
        Ps = np.mean(x ** 2)
    elif method == 'max_en':
        Ps = np.max(np.mean(x ** 2, axis=axis))
    elif method == 'axial':
        Ps = np.mean(x ** 2, axis=axis)
    else:
        raise ValueError(f'method "{method}" not recognized.')

    print("Ps = ", Ps)
    # Calculate noise power
    Pn = Ps / (10 ** (snr / 10))
    n = np.sqrt(Pn) * np.random.normal(0, 1, x.shape)

    # Output the desired result
    if out == 'signal':
        # return (x + n) * scale
        return (x + n)
    elif out == 'noise':
        return n
    elif out == 'both':
        return x + n, n
    else:
        return x + n
