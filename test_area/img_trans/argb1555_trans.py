import numpy as np
import cv2

# ========================
# 配置参数 - 可自定义修改
# ========================
width, height = 1280, 720  # 输入图像尺寸
input_file = "car_1280x720_argb1555.bin"
output_file = "car_1920x1080_argb1555_rot180.bin"

# 自定义输出尺寸（可修改为任意值，如 1080x1920, 800x600 等）
output_width = 1920
output_height = 1080

# ========================
# 读取原始二进制数据
# ========================
with open(input_file, "rb") as f:
    data = np.frombuffer(f.read(), dtype=np.uint16)  # 16-bit per pixel

# 验证数据大小
assert len(data) == width * height, f"数据大小不匹配: {len(data)} != {width * height}"

# ========================
# 解码 ARGB1555 到 BGR（OpenCV 使用 BGR）
# ========================
def argb1555_to_bgr(pixel):
    a = (pixel >> 15) & 1
    r = (pixel >> 10) & 0x1F
    g = (pixel >> 5) & 0x1F
    b = pixel & 0x1F
    # 扩展到 8-bit（乘以 8，因为 5-bit -> 8-bit）
    r8 = r * 8
    g8 = g * 8
    b8 = b * 8
    return np.array([b8, g8, r8], dtype=np.uint8)

# 逐像素转换
img_bgr = np.zeros((height, width, 3), dtype=np.uint8)
for y in range(height):
    for x in range(width):
        idx = y * width + x
        img_bgr[y, x] = argb1555_to_bgr(data[idx])

# ========================
# 调整大小到自定义输出尺寸
# ========================
resized = cv2.resize(img_bgr, (output_width, output_height), interpolation=cv2.INTER_LINEAR)

# ========================
# 180度旋转（上下左右同时翻转）
# ========================
resized_rotated = cv2.rotate(resized, cv2.ROTATE_180)

# ========================
# 转回 ARGB1555 格式
# ========================
def bgr_to_argb1555(bgr):
    b, g, r = bgr
    # 从 8-bit 压缩回 5-bit
    b5 = b // 8
    g5 = g // 8
    r5 = r // 8
    a = 1  # 假设 Alpha 为 1，可按需调整
    return (a << 15) | (r5 << 10) | (g5 << 5) | b5

# 转换回 16-bit 数组
output_data = np.zeros((output_height, output_width), dtype=np.uint16)
for y in range(output_height):
    for x in range(output_width):
        output_data[y, x] = bgr_to_argb1555(resized_rotated[y, x])

# ========================
# 保存为二进制文件
# ========================
output_data.tofile(output_file)

print(f"✅ 转换完成（180度反转）: {output_file}")
