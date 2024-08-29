import numpy as np
import matplotlib.pyplot as plt
from scipy.stats import multivariate_normal


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

# 高斯分布概率密度函数
def phi(Y, mu_k, cov_k):
    norm = multivariate_normal(mean = mu_k, cov = cov_k)
    return norm.pdf(Y)

def E_step(Y, mu, cov, alpha):
    # N为样本数，D为数据维度
    N, D = Y.shape
    K = alpha.shape[0]
    gamma = np.empty([N, K])
    for k in range(K):
        gamma[:, k] = alpha[k] * phi(Y, mu[k], cov[k])
    for j in range(N):
        gamma[j, :] = gamma[j, :]/np.sum(gamma[j, :])

    return gamma

def M_step(Y, gamma):
    N, D = Y.shape
    K = gamma.shape[1]
    mu = np.empty([K, D])
    cov = np.empty([K, D, D])
    alpha = np.empty([K])

    for k in range(K):
        Nk = np.sum(gamma[:, k])
        mu[k, :] = np.sum(gamma[:, k].reshape(N, -1) * Y, axis = 0)/Nk
        cov[k, :, :] = np.dot((Y-mu[k]).T, gamma[:, k].reshape(N, -1) * (Y - mu[k]))/Nk
        alpha[k] = Nk/N

    return mu, cov, alpha

def init_params(N, D, K):
    mu = np.random.rand(K, D)
    cov = np.array([np.eye(D)]*K)
    alpha = np.array([1.0/K]*K)

    return mu, cov, alpha

def GMM_EM(Y, K, times):
    N, D = Y.shape
    mu, cov, alpha = init_params(N, D, K)
    for i in range(times):
        gamma = E_step(Y, mu, cov, alpha)
        mu, cov, alpha = M_step(Y, gamma)

    print("{sep}Result{sep}".format(sep = "-"*20))
    print("mu:", mu, "cov:", cov, "alpha:", alpha, sep="\n")

    return mu, cov, alpha

def main():
    cov1 = np.array([[0, 0.4], [0.9, 0.1]])
    cov2 = np.array([[0.7, 0.3], [0, 0.8]])
    mu1 = np.array([0, 1])
    mu2 = np.array([2, 5])

    sample = np.zeros([200, 2])
    sample[:100,:] = np.random.multivariate_normal(mean=mu1, cov=cov1, size=100)
    sample[100:, :] = np.random.multivariate_normal(mean=mu2, cov=cov2, size=100)

    K = 2
    mu, cov, alpha = GMM_EM(sample, K, 500)

if __name__ == "__main__":
    main()
