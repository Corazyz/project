# Skill: 性能验收闸门 (Benchmark Gating)

## 触发条件
当需要验证性能优化结果、报告最终 MFU 时

## 核心原则 (经验 #4)
**板端性能结论必须基于: 短测 + 长稳态 + 多次复测**

## 三步验收流程

### Step 1: 短测 (快速筛选)
```bash
docker exec <container> env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  <OP>_TEST_SHAPE=<shape> \
  <OP>_WARMUP=5 <OP>_ITERS=20 \
  python /tmp/<op>_bench.py
```
- 目的: 快速判断优化方向是否正确
- 耗时: ~10 秒
- 精度: ±5% 波动正常

### Step 2: 长稳态 (确认稳定)
```bash
docker exec <container> env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  <OP>_TEST_SHAPE=<shape> \
  <OP>_WARMUP=10 <OP>_ITERS=800 \
  python /tmp/<op>_bench.py
```
- 目的: 确认性能无衰减、无波动
- 耗时: ~2 分钟
- 精度: ±1% 内应稳定

### Step 3: 多次复测 (排除干扰)
```bash
# 至少执行 3 次，取中位数
for i in 1 2 3; do
  docker exec <container> env ... python /tmp/<op>_bench.py
done
```
- 目的: 排除板卡温度、其他进程干扰
- 判定: 3 次结果的标准差 < 2%

## 验收标准

| 条件 | 动作 |
|------|------|
| MFU 提升 > 1% 且三步均通过 | **接受**，合入代码 |
| MFU 提升 > 1% 但长稳态衰减 | **拒绝**，排查原因 |
| MFU 变化 < 1% | **拒绝**，策略无效，记录经验 |
| MFU 回退 | **立即回退**到上一稳定版本 |

## 注意事项
- 对比执行时确保无其他任务占用板卡
- 不同 shape 的 MFU 不能直接对比
- 首次上板的 MFU 作为基线，后续所有优化相对基线报告
- MFU 计算公式: `FLOPS / (time * peak_TFLOPS * 1e12) * 100%`

## 产出格式
```
=== Benchmark Report ===
Shape: (M, K, N)
dtype: fp16
Condition: [causal/nomask/...]

Short test (20 iters):  avg_ms=X.XXX, TFLOPS=Y.YYY, MFU=Z.ZZ%
Long test (800 iters):  avg_ms=X.XXX, TFLOPS=Y.YYY, MFU=Z.ZZ%
Repeat x3 median:      avg_ms=X.XXX, TFLOPS=Y.YYY, MFU=Z.ZZ%

vs Baseline: +/-X.X% MFU
Decision: ACCEPT / REJECT
```
