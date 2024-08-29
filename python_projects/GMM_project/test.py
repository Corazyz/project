import numpy as np
import cv2

#SECTION - background frame:
I = cv2.imread('./frame/00000.bmp')
fr_bw = cv2.cvtColor(I, cv2.COLOR_BGR2GRAY)
# cv2.imwrite('bw_grayscale_image.png', fr_bw)
height, width = fr_bw.shape
# print(height, width)

#SECTION - Number of Gaussian models per pixel(usually 3-5)
num_GM = 3

#SECTION - Initialize the mean for each Guassian component
mu0 = np.zeros((height, width, num_GM))
# print(mu0)
for i in range(num_GM):
    mu0[:, :, i] = fr_bw.astype(np.float64)
    # print(mu0[:,:,i])

# cv2.imwrite('test_mu.png', mu0[:, :, 0])

#SECTION - Set initial parameters
C = num_GM
T = 5                                           # threshold
D = 2.5                                         # Deviation threshold
alpha = 0.001                                   # Learning rate
thresh = 0.5                                    # Foreground threshold
sd0 = 15                                        # Initial standard deviation
w = np.ones((height, width, num_GM))/num_GM     # Initialize the weight matrix      w = 0.33    w.shape = (120, 320, 3)
# print(w.shape)
sd = sd0 * np.ones((height, width, num_GM))     # Initialize the standard deviation     sd = 15     sd.shape = (120, 320, 3)
# print(sd.shape)
p = alpha / w                                   # Initialize the p variable for updating mean and standard deviation    p = 0.003   p.shpe = (120, 320, 3)
# print(p.shape, p)
rank = np.zeros(num_GM)                         # Priority of each Gaussian distribution (w/sd)

frame_num = 200

for n in range(1, 2):
    a = ''
    if n < 10:
        a = '0000'
    elif n < 100:
        a = '000'
    else:
        a = '00'

    image = f'./frame/{a}{n}.bmp'
    I1 = cv2.imread(image)
    #SECTION - Initialize current frame with grayscale values
    fr_bw0 = cv2.cvtColor(I1, cv2.COLOR_BGR2GRAY)
    fr_bw = fr_bw0.astype(np.float64)
    # print(fr_bw.shape)        # (120, 320)
    # cv2.imwrite('test_current_fr.png', fr_bw)
    #SECTION - Broadcast fr_bw to have the same number of channels as mu0
    fr_bw = np.expand_dims(fr_bw, axis = 2)
    fr_bw = np.repeat(fr_bw, repeats=num_GM, axis=2)

    #SECTION - Compute the abs distance between the new pixel and the Gaussian model mean
    u_diff = np.abs(fr_bw - mu0)
    # print(u_diff.shape, u_diff)

    #SECTION - Update the Guassian model params
    match0 = np.abs(u_diff) <= np.maximum(D * sd, T)
    # print(match0)
    match = np.max(match0, axis=2)
    # print(match0.shape, match.shape)

    # Update w
    # p = alpha / w
    wmatch1 = (1 - alpha) * w * match0 + alpha * match0
    # print(wmatch1[0,0,:])
    wmatch0 = (1 - alpha) * w * (1-match0)
    # print(wmatch0[0,0,:])
    w = wmatch1 + wmatch0
    # print(w[0,0,:])

    # Update sd
    sdmatch1 = np.sqrt((1-alpha) * (sd*match0) ** 2 + p * (fr_bw * match0 - mu0 * match0) ** 2)
    # print(sdmatch1[0,0,:])
    sd = sdmatch1 + sd * (1 - match0)
    # print(sd[0,0,:])

    # Update mu
    mu0match1 = (1 - alpha) * mu0 * match0 + p * fr_bw * match0
    mu0 = mu0match1 + mu0 * (1 - match0)

    # Compute the priority of each Gaussian distribution
    wsd = w / sd
    sum1 = np.sum(wsd)
    # print(wsd, sum1)

    A = np.array([np.sum(wsd[:, :, i]) / sum1 for i in range(C)])
    # print(A)
    A_sorted_indices = np.argsort(A)[::-1]      # 将 w 降序排列

    thresh0 = 0
    k = 0
    for i in range(C):
        thresh0 += A[A_sorted_indices[M - i -1]]
        if thresh0 > thresh:
            k = i + 1
            break

    fground = np.ones((height, width))      #初始化背景为黑色
    for i in range(k):
        idx = A_sorted_indices[M - i - 1]
        fground *= (np.abs(fr_bw[:, :, 0] - u0[:, :, idx]) > D * sd[:, :, idx])



# h = np.ones((3, 3)) / 9
