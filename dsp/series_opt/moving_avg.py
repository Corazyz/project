import numpy as np

def moving_average_filter(signal):
    buffer = [0, 0, 0, 0]
    filtered_signal = []

    for x in signal:
        buffer.pop(0)
        buffer.append(x)

        y = sum(buffer) / 4
        filtered_signal.append(y)

    return filtered_signal