# from sklearn.datasets import make_blobs
# from sklearn.mixture import GaussianMixture

# def test():
#     # 1. 构建数据
#     x, y = make_blobs(n_samples=1000, n_features=2, centers=((0, 1.5), (1, 0.5)), cluster_std=(0.4, 0.5), random_state=42)  # x: 数据集，y: 数据所属类别

#     # 2. 训练模型
#     estimator = GaussianMixture(n_components=2, random_state=42)
#     estimator.fit(x)
#     # 3. 数据聚类
#     y_pred = estimator.predict(x)
#     print(y_pred)



#     # 4. 数据增强
#     data = estimator.sample(3)
#     print(data)


# if __name__=='__main__':
#     test()

from sklearn.datasets import make_blobs
from sklearn.mixture import GaussianMixture
import matplotlib.pyplot as plt
import numpy as np

def test():
    # 1. 构建数据
    x, y = make_blobs(n_samples=1000, n_features=2, centers=((0, 1.5), (1, 0.5)),
                      cluster_std=(0.4, 0.5), random_state=42)

    # 2. 训练模型
    estimator = GaussianMixture(n_components=2, random_state=42)
    estimator.fit(x)

    # 3. 数据聚类
    y_pred = estimator.predict(x)
    print("聚类结果前10个样本:", y_pred[:10])

    # 4. 数据增强（采样）
    data, _ = estimator.sample(3)
    print("生成的3个新样本:\n", data)

    # 5. 可视化：原始数据 + 聚类结果
    plt.figure(figsize=(12, 5))

    # 子图1：原始数据（按真实标签着色）
    plt.subplot(1, 2, 1)
    plt.scatter(x[:, 0], x[:, 1], c=y, cmap='viridis', s=30, edgecolors='k', linewidth=0.5)
    plt.title('Original Data (True Labels)')
    plt.xlabel('Feature 1')
    plt.ylabel('Feature 2')
    plt.grid(True, alpha=0.3)

    # 子图2：聚类结果
    plt.subplot(1, 2, 2)
    plt.scatter(x[:, 0], x[:, 1], c=y_pred, cmap='viridis', s=30, edgecolors='k', linewidth=0.5)
    # 绘制聚类中心
    centers = estimator.means_
    plt.scatter(centers[:, 0], centers[:, 1], c='red', marker='x', s=100, linewidths=3, label='Cluster Centers')
    plt.title('Gaussian Mixture Clustering Result')
    plt.xlabel('Feature 1')
    plt.ylabel('Feature 2')
    plt.legend()
    plt.grid(True, alpha=0.3)

    # 保存图像
    plt.tight_layout()
    plt.savefig('gaussian_mixture_clusters.png', dpi=300, bbox_inches='tight')
    print("\n✅ 可视化结果已保存为 'gaussian_mixture_clusters.png'")

if __name__ == '__main__':
    test()
