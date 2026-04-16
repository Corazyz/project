import numpy as np
import cv2

# ================== 配置参数 ==================
width, height = 1280, 720  # 输入图像尺寸
input_file = "car_1280x720_argb8888.bin"
output_file = "car_1920x1080_argb8888_rot180.bin"

# ✅ 自定义输出尺寸（可修改）
output_width = 1920
output_height = 1080

# ================== 读取二进制数据 ==================
with open(input_file, "rb") as f:
    data = np.frombuffer(f.read(), dtype=np.uint8)

# 验证数据大小
expected_size = width * height * 4
assert len(data) == expected_size, f"数据大小不匹配: {len(data)} != {expected_size}"

# 重塑为 (H, W, 4) 的 ARGB 数组
img_argb = data.reshape((height, width, 4))

# 转换为 BGR（OpenCV 使用 BGR）
img_bgr = img_argb[:, :, [2, 1, 0]]  # 取 R,G,B（忽略 A，或保留用于后续处理）

# 缩放至目标尺寸
resized_bgr = cv2.resize(img_bgr, (output_width, output_height), interpolation=cv2.INTER_LINEAR)
# ========================
# 180度旋转（上下左右同时翻转）
# ========================
resized_rotated = cv2.rotate(resized_bgr, cv2.ROTATE_180)

# 重建 ARGB8888（添加 Alpha 通道，设为 255 不透明）
resized_argb = np.zeros((output_height, output_width, 4), dtype=np.uint8)
resized_argb[:, :, 0] = 255  # Alpha 通道（全不透明）
resized_argb[:, :, 1] = resized_rotated[:, :, 2]  # R 通道
resized_argb[:, :, 2] = resized_rotated[:, :, 1]  # G 通道
resized_argb[:, :, 3] = resized_rotated[:, :, 0]  # B 通道

# 保存为二进制文件
resized_argb.tofile(output_file)

print(f"✅ ARGB8888 转换完成(180度): {output_file} (尺寸: {output_width}x{output_height})")
