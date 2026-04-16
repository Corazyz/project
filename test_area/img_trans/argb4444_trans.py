import numpy as np
import cv2

# =================== 配置区 ===================
width, height = 1280, 720  # 输入图像尺寸
input_file = "car_1280x720_argb4444.bin"
output_file = "car_1920x1080_argb4444_rot180.bin"

# ✅ 自定义输出尺寸（可自由修改）
output_width = 1920   # 输出图像宽度
output_height = 1080  # 输出图像高度

# =================== 读取二进制数据 ===================
with open(input_file, "rb") as f:
    data = np.frombuffer(f.read(), dtype=np.uint16)  # 每像素2字节

# 验证数据大小
expected_size = width * height
assert len(data) == expected_size, f"数据大小不匹配: {len(data)} != {expected_size}"

# =================== 解码 ARGB4444 到 BGR（8-bit） ===================
def argb4444_to_bgr(pixel):
    a = (pixel >> 12) & 0xF
    r = (pixel >> 8) & 0xF
    g = (pixel >> 4) & 0xF
    b = pixel & 0xF
    # 扩展到 8-bit（乘以 17，因为 4-bit -> 8-bit，16级→256级）
    r8 = r * 17
    g8 = g * 17
    b8 = b * 17
    return np.array([b8, g8, r8], dtype=np.uint8)

# 逐像素转换
img_bgr = np.zeros((height, width, 3), dtype=np.uint8)
for y in range(height):
    for x in range(width):
        idx = y * width + x
        img_bgr[y, x] = argb4444_to_bgr(data[idx])

# =================== 缩放图像 ===================
# OpenCV resize 使用 (width, height)
resized_bgr = cv2.resize(img_bgr, (output_width, output_height), interpolation=cv2.INTER_LINEAR)
# ========================
# 180度旋转（上下左右同时翻转）
# ========================
resized_rotated = cv2.rotate(resized_bgr, cv2.ROTATE_180)

# =================== 转回 ARGB4444 ===================
def bgr_to_argb4444(bgr):
    b, g, r = bgr
    # 压缩到 4-bit
    b4 = b // 17
    g4 = g // 17
    r4 = r // 17
    a = 15  # 全不透明（可调整）
    return (a << 12) | (r4 << 8) | (g4 << 4) | b4

# 转换回 16-bit 数组
output_data = np.zeros((output_height, output_width), dtype=np.uint16)  # 注意：numpy 是 (height, width)
for y in range(output_height):
    for x in range(output_width):
        output_data[y, x] = bgr_to_argb4444(resized_rotated[y, x])

# =================== 保存输出文件 ===================
output_data.tofile(output_file)

print(f"✅ ARGB4444 转换完成(180度): {output_file}")
print(f"输出尺寸: {output_width}x{output_height}")
