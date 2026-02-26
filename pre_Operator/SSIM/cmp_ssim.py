def compare_files(file1, file2):
    """
    比较两个文件中的数值差异，并计算 file1 的数值均值
    """
    # 读取文件内容
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        lines1 = f1.readlines()
        lines2 = f2.readlines()

    # 检查行数是否相同
    if len(lines1) != len(lines2):
        print(f"文件行数不同: {file1} 有 {len(lines1)} 行, {file2} 有 {len(lines2)} 行")
        return

    total_diff = 0
    max_diff = 0
    max_diff_pos = (0, 0)
    diff_count = 0

    # 新增：收集 file 的所有数值用于计算均值
    all_vals1 = []
    all_vals2 = []

    print(f"正在比较 {file1} 和 {file2}...")
    print("-" * 80)

    for i, (line1, line2) in enumerate(zip(lines1, lines2)):
        # 解析每行的数值
        vals1 = [float(x) for x in line1.strip().split()]
        vals2 = [float(x) for x in line2.strip().split()]

        # 收集 file 的所有数值
        all_vals1.extend(vals1)
        all_vals2.extend(vals2)

        # 检查每行数值个数是否相同
        if len(vals1) != len(vals2):
            print(f"第 {i+1} 行数值个数不同: {file1} 有 {len(vals1)} 个, {file2} 有 {len(vals2)} 个")
            continue

        # 比较每个数值
        for j, (v1, v2) in enumerate(zip(vals1, vals2)):
            diff = abs(v1 - v2)
            if diff > 0:
                total_diff += diff
                diff_count += 1
                if diff > max_diff:
                    max_diff = diff
                    max_diff_pos = (i + 1, j + 1)

                # 可选：打印所有差异
                # print(f"位置 ({i+1},{j+1}): {file1}={v1:.6f}, {file2}={v2:.6f}, 差异={diff:.6f}")

    # 计算 file 的均值
    print(f"sum_val1: {sum(all_vals1):.6f}")
    print(f"sum_val2: {sum(all_vals2):.6f}")
    mean_file1 = sum(all_vals1) / len(all_vals1) if len(all_vals1) > 0 else 0.0
    print(f"{file1} 数值均值: {mean_file1:.6f}")
    mean_file2 = sum(all_vals2) / len(all_vals2) if len(all_vals2) > 0 else 0.0
    print(f"{file2} 数值均值: {mean_file2:.6f}")
    print(f"Total values read from {file1}: {len(all_vals1)}")
    print(f"Total values read from {file2}: {len(all_vals2)}")


    # 输出统计结果
    if diff_count > 0:
        avg_diff = total_diff / diff_count
        print(f"发现 {diff_count} 处差异")
        print(f"最大差异: {max_diff:.6f} (位置: 第 {max_diff_pos[0]} 行, 第 {max_diff_pos[1]} 列)")
        print(f"平均差异: {avg_diff:.6f}")
        print(f"总差异: {total_diff:.6f}")
    else:
        print("两个文件完全相同！")

    print("-" * 80)

# 使用示例
if __name__ == "__main__":
    # compare_files("pad_S_c.txt", "pad_S_py.txt")
    compare_files("mssim_c.txt", "mssim_py.txt")
    # compare_files("test_val_c.txt", "test_val_py.txt")
