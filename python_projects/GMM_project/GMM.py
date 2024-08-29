# import numpy as np
# import random

# def calc_prob(X, K, pMu, pSigma):
#     N = X.shape[0]
#     D = X.shape[1]
#     Px = np.zeros((N, K))
#     for i in range(K):
#         Xshift = X - np.tile(pMu[i], (N, 1))
#         lambda_flag = np.e**(-5)
#         conv = pSigma[i] + lambda_flag*np.eye(D)
#         inv_pSigma = np.linalg.inv(conv)
#         tmp = np.sum(np.dot(Xshift, inv_pSigma)*Xshift, axis=1)
#         coef = (2*np.pi)**(-D/2)*np.sqrt(np.linalg.det(inv_pSigma))
#         Px[:, i] = coef*np.e**(-1/2*tmp)
#     return Px

# def gmm(X, K):
#     threshold = np.e**(-15)
#     N = X.shape[0]
#     D = X.shape[1]
#     print("N and D are:\n", N, D)
#     rndp = random.sample(np.arange(N).tolist(), K)
#     print("rndp is:\n", rndp)
#     centroids = X[rndp, :]
#     print("centroids are:\n", centroids)
#     pMu = centroids
#     pPi = np.zeros((1, K))
#     print("pPi is:\n", pPi)
#     pSigma = np.zeros((K, D, D))
#     print("pSigma are:\n", pSigma)
#     dist = np.tile(np.sum(X*X, axis=1).reshape(N, 1), (1, K)) + np.tile(np.sum(pMu * pMu, axis=1), (N, 1))-2*np.dot(X, pMu.T)
#     labels = np.argmin(dist, axis = 1)
#     for i in range(K):
#         index = labels == i
#         Xk = X[index, :]
#         pPi[:, i] = (Xk.shape[0])/N
#         pSigma[i] = np.cov(Xk.T)
#     Loss = -float("inf")
#     while True:
#         Px = calc_prob(X, K, pMu, pSigma)
#         pGamma = Px * np.tile(pPi, (N, 1))
#         pGamma = pGamma/np.tile(np.sum(pGamma, axis=1).reshape(N,1), (1, K))
#         Nk = np.sum(pGamma, axis=0)
#         pMu = np.dot(np.dot(np.diag(1/Nk), pGamma.T), X)
#         pPi = Nk/N
#         for i in range(K):
#             Xshift = X - np.tile(pMu[i], (N, 1))
#             pSigma[i] = np.dot(Xshift.T, np.dot(np.diag(pGamma[:, i]), Xshift))/Nk[i]
#         L = np.sum(np.log(np.dot(Px, pPi.T)), axis=0)
#         if L-Loss < threshold:
#             break
#         Loss = L
#     return Px, pMu, pSigma, pPi

# if __name__ == "__main__":
#     Data_list = []
#     with open("data.txt", 'r') as file:
#         for line in file.readlines():
#             point = []
#             point.append(float(line.split()[0]))
#             point.append(float(line.split()[1]))
#             Data_list.append(point)
#     Data = np.array(Data_list)
#     Px, pMu, pSigma, pPi = gmm(Data, 2)

# import cv2
# import numpy as np
# import glob

# GMM_MAX_COMPONT = 5
# SIGMA = 30
# gmm_thr_sumw = 0.6
# train_num = 2
# WEIGHT = 0.05
# T = 0.7
# alpha = 0.005
# eps = pow(10, -10)
# channel = 3
# m_weight = [[] for i in range(GMM_MAX_COMPONT * channel)]
# m_mean = [[] for i in range(GMM_MAX_COMPONT * channel)]
# m_sigma = [[] for i in range(GMM_MAX_COMPONT * channel)]
# m_fit_num = None

# def init(img):
#     row, col, channel = img.shape
#     global m_fit_num
#     for i in range(GMM_MAX_COMPONT * channel):
#         m_weight[i] = np.zeros((row, col), dtype='float32')
#         m_mean[i] = np.zeros((row, col), dtype='float32')
#         m_sigma[i] = np.zeros((row, col), dtype='float32')
#         m_sigma[i] *= SIGMA

#     m_fit_num = np.zeros((row, col), dtype='int32')

import cv2
import numpy as np
import random

# Read the first frame as the background frame
I = cv2.imread('./frame/00000.bmp')
fr_bw = cv2.cvtColor(I, cv2.COLOR_BGR2GRAY)


height, width = fr_bw.shape

# Number of Gaussian models per pixel (usually 3-5)
C = 3

# Initialize the mean for each Gaussian component
u0 = np.zeros((height, width, C))
for i in range(C):
    u0[:, :, i] = fr_bw.astype(np.float64)

# Set initial parameters
M = C  # Number of models representing the background (initially equal to C)
T = 5  # Threshold
D = 2.5  # Deviation threshold
alpha = 0.001  # Learning rate
thresh = 0.5  # Foreground threshold
sd0 = 15  # Initial standard deviation
w = np.ones((height, width, C)) / C  # Initialize the weight matrix
sd = np.ones((height, width, C))  # Initialize the standard deviation
for i in range(height):
    for j in range(width):
        for k in range(C):
            sd[i, j, k] *= random.random() * 15
print(sd)
p = alpha / w  # Initialize the p variable for updating mean and standard deviation
rank = np.zeros(C)  # Priority of each Gaussian distribution (w/sd)

frame_num = 200  # Number of frames

# Process each frame
for n in range(1, frame_num + 1):
    a = ''
    if n < 10:
        a = '0000'
    elif n < 100:
        a = '000'
    else:
        a = '00'

    image = f'./frame/{a}{n}.bmp'
    I1 = cv2.imread(image)
    fr_bw0 = cv2.cvtColor(I1, cv2.COLOR_BGR2GRAY)

    # Apply a Gaussian filter
    # h = np.ones((3, 3)) / 9
    # fr_bw0 = cv2.filter2D(fr_bw0, -1, h)

    # Initialize the current frame with the grayscale values
    # No need to create a 3D array, just use fr_bw0 directly
    fr_bw = fr_bw0.astype(np.float64)

    # Broadcast fr_bw to have the same number of channels as u0
    fr_bw = np.expand_dims(fr_bw, axis=2)
    fr_bw = np.repeat(fr_bw, repeats=C, axis=2)

    # Compute the absolute distance between the new pixel and the Gaussian model mean
    u_diff = np.abs(fr_bw - u0)

    # Update the Gaussian model parameters
    match0 = np.abs(u_diff) <= np.maximum(D * sd, T)
    match = np.max(match0, axis=2)

    p = alpha / w
    wmatch1 = (1 - alpha) * w * match0 + alpha * match0
    wmatch0 = (1 - alpha) * w * (1 - match0)
    w = wmatch1 + wmatch0

    sdmatch1 = np.sqrt((1 - alpha) * (sd * match0) ** 2 + p * (fr_bw * match0 - u0 * match0) ** 2)
    sd = sdmatch1 + sd * (1 - match0)
    print(sd)

    u0match1 = (1 - alpha) * u0 * match0 + p * fr_bw * match0
    u0 = u0match1 + u0 * (1 - match0)

    # Add new Gaussian distributions if none match and there is room
    if np.min(match) == 0 and M < C:  # Ensure we don't exceed the number of channels
        M += 1
        u0[:, :, M - 1] = fr_bw0
        sd[:, :, M - 1] = sd0 * np.ones((height, width))
        w[:, :, M - 1] = alpha * np.ones((height, width))

    # Compute the priority of each Gaussian distribution
    wsd = w / sd
    sum1 = np.sum(wsd)
    A = np.array([np.sum(wsd[:, :, i]) / sum1 for i in range(M)])
    print(A)
    A_sorted_indices = np.argsort(A)[::-1]

    thresh0 = 0
    k = 0
    for i in range(M):
        thresh0 += A[A_sorted_indices[M - i - 1]]
        if thresh0 > thresh:
            k = i + 1
            break

    fground = np.ones((height, width))
    for i in range(k):
        idx = A_sorted_indices[M - i - 1]
        fground *= (np.abs(fr_bw[:, :, 0] - u0[:, :, idx]) > D * sd[:, :, idx])

    # Save the original image and the processed foreground
    filename_frame = 'Frame.png'
    # filename_foreground = 'Foreground_with_filter.png'
    filename_foreground = 'Foreground.png'

    # Save the original image
    cv2.imwrite(filename_frame, I1)

    # Save the processed foreground image
    foreground_image = (fground * 255).astype(np.uint8)
    cv2.imwrite(filename_foreground, foreground_image)

# Close all windows
cv2.destroyAllWindows()