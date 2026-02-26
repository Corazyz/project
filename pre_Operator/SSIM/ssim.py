# import cv2
# from skimage.metrics import structural_similarity as ssim
# import numpy as np
# import time

# # decoded img
# img1 = cv2.imread('10350+.jpg')
# img2 = cv2.imread('background.jpg')

# # # 转换为 RGB（OpenCV 读取的是 BGR）
# # img1_rgb = cv2.cvtColor(img1, cv2.COLOR_BGR2RGB)
# # img2_rgb = cv2.cvtColor(img2, cv2.COLOR_BGR2RGB)

# # # 分离通道
# # r1_channel = img1_rgb[:, :, 0]  # Red
# # g1_channel = img1_rgb[:, :, 1]  # Green
# # b1_channel = img1_rgb[:, :, 2]  # Blue
# # r2_channel = img2_rgb[:, :, 0]  # Red
# # g2_channel = img2_rgb[:, :, 1]  # Green
# # b2_channel = img2_rgb[:, :, 2]  # Blue

# # # 将三个通道按 planar 格式拼接：R 通道 + G 通道 + B 通道
# # planar_data1 = np.concatenate([
# #     r1_channel.flatten(),
# #     g1_channel.flatten(),
# #     b1_channel.flatten()
# # ], axis=0)
# # planar_data2 = np.concatenate([
# #     r2_channel.flatten(),
# #     g2_channel.flatten(),
# #     b2_channel.flatten()
# # ], axis=0)

# # # 保存为 raw 文件（无压缩、无头信息）
# # planar_data1.tofile('img1_rgb_planar.raw')
# # planar_data2.tofile('img2_rgb_planar.raw')

# # 灰度图 SSIM
# gray1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
# gray2 = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)
# # gray1.tofile('gray1.raw')
# # gray2.tofile('gray2.raw')


# ## customized data
# # with open('image.txt', 'r') as f:
# #     lines = f.readlines()
# # data1 = []
# # for line in lines:
# #     row = [float(x) for x in line.strip().split()]
# #     data1.append(row)
# # img = np.array(data1, dtype=np.float32)


# # with open('image2.txt', 'r') as f:
# #     lines = f.readlines()
# # data2 = []
# # for line in lines:
# #     row = [float(x) for x in line.strip().split()]
# #     data2.append(row)
# # img2 = np.array(data2, dtype=np.float32)

# # gray1 = img.astype(np.uint8)
# # gray2 = img2.astype(np.uint8)



# ## raw nv21 img data
# # # 图像参数
# # width = 1920
# # height = 1080

# # # 解析 NV12 数据
# # y_size = width * height
# # uv_size = width * height // 2  # UV 平面是 Y 的一半

# # # 读取 NV21 文件（二进制模式）
# # with open('1920x1080_nv12.bin', 'rb') as f:
# #     nv12_data = np.frombuffer(f.read(), dtype=np.uint8)

# # y_plane = nv12_data[:y_size].reshape(height, width)
# # uv_plane = nv12_data[y_size:y_size+uv_size].reshape(height//2, width)

# # # 仅使用 Y 平面作为灰度图（因为 NV12 的 Y 是亮度）
# # gray1 = y_plane.astype(np.uint8)

# # # 读取 NV21 文件（类似，但 UV 顺序是 VU）
# # with open('output_nv21.bin', 'rb') as f:
# #     nv21_data = np.frombuffer(f.read(), dtype=np.uint8)

# # y_plane2 = nv21_data[:y_size].reshape(height, width)
# # uv_plane2 = nv21_data[y_size:y_size+uv_size].reshape(height//2, width)

# # gray2 = y_plane2.astype(np.uint8)



# # ## raw gray img data
# # # 图像参数
# # width = 1920
# # height = 1080

# # # 解析 gray 数据
# # y_size = width * height

# # # 读取 gray 文件（二进制模式）
# # gray1 = np.frombuffer(open('gray.bin', 'rb').read(), dtype=np.uint8)
# # assert len(gray1) == width * height, "Data size mismatch"
# # gray1 = gray1.reshape(height, width)

# # # 读取 gray 文件
# # gray2 = np.frombuffer(open('gray_out.bin', 'rb').read(), dtype=np.uint8)
# # assert len(gray2) == width * height, "Data size mismatch"
# # gray2 = gray2.reshape(height, width)





# # np.savetxt('gray1_py.txt', gray1, fmt='%.2f', delimiter=' ')
# # np.savetxt('gray2_py.txt', gray2, fmt='%.2f', delimiter=' ')

# # ssim_gray, diff_map = ssim(gray1, gray2, data_range=255, full=True, gaussian_weights=False)






# ## load saved raw rgb data


# # # 假设你已知图像尺寸
# # height, width = 2160, 3840  # 示例尺寸，请替换为你实际的尺寸！

# # # 读取 planar raw 文件
# # planar_data1 = np.fromfile('img1_rgb_planar.raw', dtype=np.uint8)
# # planar_data2 = np.fromfile('img2_rgb_planar.raw', dtype=np.uint8)

# # # 每个通道大小
# # channel_size = height * width

# # # 分割为 R, G, B 通道
# # r1 = planar_data1[:channel_size].reshape((height, width))
# # g1 = planar_data1[channel_size:2*channel_size].reshape((height, width))
# # b1 = planar_data1[2*channel_size:].reshape((height, width))

# # r2 = planar_data2[:channel_size].reshape((height, width))
# # g2 = planar_data2[channel_size:2*channel_size].reshape((height, width))
# # b2 = planar_data2[2*channel_size:].reshape((height, width))

# # # 合并为 RGB 图像
# # img1 = np.stack([r1, g1, b1], axis=-1)  # H x W x 3
# # img2 = np.stack([r2, g2, b2], axis=-1)  # H x W x 3

# # 注意：OpenCV 默认是 BGR，但你之前转成了 RGB，所以这里保持 RGB
# # 如果你后续要用 OpenCV 显示，可能需要转回 BGR

# start_time = time.perf_counter()
# ssim_gray, diff_map = ssim(gray1, gray2, data_range=255, full=True, gaussian_weights=False)
# # ssim_gray, diff_map = ssim(img1, img2, channel_axis=-1, data_range=255, full=True, gaussian_weights=False, win_size=11)
# end_time = time.perf_counter()
# elapsed_time = end_time - start_time

# print(f"灰度图 SSIM: {ssim_gray:.6f}")
# diff_map_scaled = (diff_map * 255).astype("uint8")
# cv2.imwrite('diff_map.png', diff_map_scaled)
# print("diff_map saved as 'diff_map.png")
# np.save('diff_map.npy', diff_map)
# print("diff_map data saved as 'diff_map.npy")
# print(f"consuming time = {elapsed_time:.4f} s")


# ## raw rgb planar img data
# # # 图像参数
# # width = 1920
# # height = 1080

# # # 解析 gray 数据
# # y_size = width * height
# # total_size = 3 * y_size

# # def load_rgb_planar(filename):
# #     data = np.frombuffer(open(filename, 'rb').read(), dtype=np.uint8)
# #     assert len(data) == total_size, f"Data size mismatch in {filename}: expected {total_size}, got {len(data)}"

# #     R = data[0:y_size].reshape(height, width)
# #     G = data[y_size:2*y_size].reshape(height, width)
# #     B = data[2*y_size:3*y_size].reshape(height, width)

# #     rgb = np.stack([R, G, B], axis=2)
# #     return rgb

# # # load input and output
# # rgb1 = load_rgb_planar('rgbp.bin')
# # rgb2 = load_rgb_planar('rgbp_out.bin')


# # # 保存为文本文件（可选：每个通道单独保存）
# # np.savetxt('rgb1_R_py.txt', rgb1[:, :, 0], fmt='%.2f', delimiter=' ')
# # np.savetxt('rgb1_G_py.txt', rgb1[:, :, 1], fmt='%.2f', delimiter=' ')
# # np.savetxt('rgb1_B_py.txt', rgb1[:, :, 2], fmt='%.2f', delimiter=' ')

# # np.savetxt('rgb2_R_py.txt', rgb2[:, :, 0], fmt='%.2f', delimiter=' ')
# # np.savetxt('rgb2_G_py.txt', rgb2[:, :, 1], fmt='%.2f', delimiter=' ')
# # np.savetxt('rgb2_B_py.txt', rgb2[:, :, 2], fmt='%.2f', delimiter=' ')

# # # 计算每个通道的 SSIM
# # ssim_r, diff_r = ssim(rgb1[:, :, 0], rgb2[:, :, 0], data_range=255, full=True, gaussian_weights=True)
# # ssim_g, diff_g = ssim(rgb1[:, :, 1], rgb2[:, :, 1], data_range=255, full=True, gaussian_weights=True)
# # ssim_b, diff_b = ssim(rgb1[:, :, 2], rgb2[:, :, 2], data_range=255, full=True, gaussian_weights=True)

# # # 平均 SSIM
# # ssim_avg = (ssim_r + ssim_g + ssim_b) / 3.0

# # print(f"RGB 通道 SSIM: R={ssim_r:.6f}, G={ssim_g:.6f}, B={ssim_b:.6f}")
# # print(f"平均 SSIM: {ssim_avg:.6f}")

# # # 生成差值图（每个通道独立缩放）
# # diff_map_r = (diff_r * 255).astype("uint8")
# # diff_map_g = (diff_g * 255).astype("uint8")
# # diff_map_b = (diff_b * 255).astype("uint8")
# # diff_map_rgb = cv2.merge([diff_map_r, diff_map_g, diff_map_b])

# # cv2.imwrite('diff_map_rgb.png', diff_map_rgb)
# # print("RGB差值图保存为 'diff_map_rgb.png'")

# # # 保存差值图原始数据（浮点格式）
# # diff_map_full = np.stack([diff_r, diff_g, diff_b], axis=2)
# # np.save('diff_map_rgb.npy', diff_map_full)
# # print("差值图数据保存为 'diff_map_rgb.npy'")

# # RGB 通道平均 SSIM
# # ssim_r = ssim(img1[:,:,2], img2[:,:,2], data_range=255)  # R
# # ssim_g = ssim(img1[:,:,1], img2[:,:,1], data_range=255)  # G
# # ssim_b = ssim(img1[:,:,0], img2[:,:,0], data_range=255)  # B
# # ssim_rgb = (ssim_r + ssim_g + ssim_b) / 3.0
# # print(f"RGB 平均 SSIM: {ssim_rgb:.6f}")



from skimage.metrics import structural_similarity as ssim
import cv2
import numpy as np
import gc
import time
#import imagehash
#from PIL import Image

# SSIM
# img1 = cv2.imread('image1_resized.png', 0).astype(np.float32) / 255.0
# img2 = cv2.imread('sharpened_resized.png', 0).astype(np.float32) / 255.0
img1 = cv2.imread('10350+.jpg')
img2 = cv2.imread('background.jpg')
gray1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
gray2 = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)
#img1 = cv2.resize(img1, (512, 512))
#img2 = cv2.resize(img2, (512, 512))

start_time = time.perf_counter()

# ssim_value, diff_map = ssim(img1, img2, channel_axis=-1, data_range=255, full=True, gaussian_weights=False, win_size=11)
ssim_value, diff_map = ssim(gray1, gray2, data_range=255, full=True, gaussian_weights=False, win_size=11)

end_time = time.perf_counter()
elapsed_time = end_time - start_time

#ssim_value = ssim(img1, img2, data_range=255)
print(f"SSIM = {ssim_value:.6f}")
print(f"consuming time = {elapsed_time:.4f} s")
del img1, img2, diff_map
gc.collect()