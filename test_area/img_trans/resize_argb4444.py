import numpy as np
import cv2

def argb4444_to_argb8888(data, w, h):
    """将 argb4444 (16bit) 转为 argb8888 (32bit)"""
    pixels = np.frombuffer(data, dtype=np.uint16).reshape(h, w)
    a = ((pixels >> 12) & 0xF).astype(np.uint8) * 17  # 4bit -> 8bit (0-15 → 0-255)
    r = ((pixels >> 8) & 0xF).astype(np.uint8) * 17
    g = ((pixels >> 4) & 0xF).astype(np.uint8) * 17
    b = (pixels & 0xF).astype(np.uint8) * 17
    # 合成 ARGB8888 (OpenCV 默认是 BGR，但这里我们用 ARGB)
    argb8888 = np.stack([a, r, g, b], axis=2).astype(np.uint8)
    return argb8888

def argb8888_to_argb4444(img):
    """将 argb8888 (32bit) 转回 argb4444 (16bit)"""
    a = (img[:,:,0] // 17).astype(np.uint16) << 12
    r = (img[:,:,1] // 17).astype(np.uint16) << 8
    g = (img[:,:,2] // 17).astype(np.uint16) << 4
    b = (img[:,:,3] // 17).astype(np.uint16)
    argb4444 = (a | r | g | b).astype(np.uint16)
    return argb4444.tobytes()

# 读取原始文件
input_file = "rainbow_4096x2160.argb4444"
width, height = 4096, 2160

with open(input_file, "rb") as f:
    data = f.read()

# 转为 ARGB8888
argb8888 = argb4444_to_argb8888(data, width, height)

# 缩放（使用 OpenCV）
scaled = cv2.resize(argb8888, (1920, 1080), interpolation=cv2.INTER_AREA)

# 转回 argb4444
output_data = argb8888_to_argb4444(scaled)

# 保存
output_file = "rainbow_1920x1080.argb4444"
with open(output_file, "wb") as f:
    f.write(output_data)

print(f"✅ 已保存为 {output_file}，大小: {len(output_data)} 字节")
