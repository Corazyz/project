# Skill: 精度验证流程

## 触发条件
当算子首次上板或优化后需要验证精度时

## 执行流程

### 1. 多 Shape 测试矩阵
```
必测 shape 列表:
- 最小: (1, 64, 64)         — 边界条件
- 典型: (4096, 4096, 4096)  — 常规场景
- 非对齐: (127, 513, 1023)  — 非 tile 整数倍
- 极端: (1, 4096, 11008)    — 长窄矩阵
- 大规模: (8192, 8192, 8192) — 内存压力
```

### 2. 精度标准
| 数据类型 | allclose atol | max_diff 上限 |
|---------|--------------|--------------|
| FP32    | 1e-5         | 1e-4         |
| FP16    | 1e-2         | 5e-2         |
| BF16    | 1e-2         | 5e-2         |

### 3. 对比方法
```python
# TPU 结果 vs CPU 结果
max_diff = abs(tpu_output - cpu_output).max()
allclose = numpy.allclose(tpu_output, cpu_output, atol=ATOL)
```

### 4. 回归检查
优化后必须重跑全部 shape，确认无精度回退:
```bash
python test/<op>_accuracy.py --all-shapes --save-report
# 对比上次报告，任何 shape 的 max_diff 增大 > 2x 需要排查
```

## 失败处理
- 单个 shape 失败 → 检查 tile 边界处理 (padding / tail 块)
- 全部 shape 失败 → 检查 kernel 逻辑错误，回退到 CModel 阶段
- 大 shape 失败小 shape 通过 → 检查累加精度 (经验 #12)

## 产出
- 精度报告: `logs/accuracy_<timestamp>.txt`
- 状态: ALL PASSED / FAILED (列出失败 shape)
