# import numpy as np

# def cal_mean(file):
#     """
#     计算指定文件中所有数值的均值
#     """
#     width = 3830
#     height = 2150
#     # 读取文件内容
#     with open(file, 'r') as f:
#         lines = f.readlines()

#     # 收集所有数值
#     all_vals = []

#     for line in lines:
#         # 解析每行的数值（假设用空格分隔）
#         try:
#             vals = [float(x) for x in line.strip().split()]
#             all_vals.extend(vals)
#         except ValueError:
#             # 忽略无法转换为 float 的行（可选：也可抛出异常或记录警告）
#             continue

#     print("Python 前10个值:", [f"{x:.6f}" for x in all_vals[:10]])

#         # ✅ 断言数据量正确
#     assert len(all_vals) == width * height, f"数据量错误: {len(all_vals)} != {width * height}"

#     # ✅ 写入文件：每行 3830 个数
#     with open('test_val_py.txt', 'w') as f:
#         for i in range(0, len(all_vals), width):
#             row = all_vals[i:i + width]
#             line = ' '.join([f"{x:.2f}" for x in row])
#             f.write(line + '\n')

#     # 计算均值
#     if len(all_vals) == 0:
#         print(f"警告：文件 {file} 中未找到有效数值。")
#         mean_file = 0.0
#     else:
#         mean_file = sum(all_vals) / len(all_vals)
#         print(f"sum_val: {sum(all_vals):.6f}")
#         print(f"{file} 数值均值: {mean_file:.6f}")
#         print(f"Total values read from {file}: {len(all_vals)}")

#     return mean_file  # 可选：返回均值供后续使用


# # 使用示例
# if __name__ == "__main__":
#     mean = cal_mean("pad_S_c.txt")
#     print(f"返回的均值: {mean:.6f}")




import numpy as np

def cal_mean(file):
    width = 3830
    height = 2150  # 和 C 一致
    total_count = width * height

    all_vals = []
    with open(file, 'r') as f:
        for line in f:
            vals = line.strip().split()
            for v in vals:
                try:
                    all_vals.append(float(v))
                except ValueError:
                    continue
            if len(all_vals) >= total_count:
                break

    all_vals = all_vals[:total_count]  # 确保只取 383000 个

    print("Python 前10个值:", [f"{x:.6f}" for x in all_vals[:10]])
    assert len(all_vals) == total_count, f"数据量错误: {len(all_vals)} != {total_count}"

    mean_file = sum(all_vals) / len(all_vals)
    print(f"sum_val: {sum(all_vals):.6f}")
    print(f"均值: {mean_file:.6f}")
    return mean_file

if __name__ == "__main__":
    mean = cal_mean("pad_S_c.txt")
    print(f"返回的均值: {mean:.6f}")
