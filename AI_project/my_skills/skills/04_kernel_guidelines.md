# Skill: Kernel 编写规范

## 触发条件
当开始编写新的 PPL Kernel (.pl) 时

## 文件结构模板

```perl
# <Operator Name> Kernel for BM1690
# Shape: <输入输出 shape 定义>
# Author: <name>
# Date: <date>

# ============ 参数定义 ============
# 输入 tensor
# A: [shape] dtype
# B: [shape] dtype
# 输出 tensor
# C: [shape] dtype

# ============ Tile 配置 ============
# tile_xxx = N  (选择依据: choose_tiles() 输出)

# ============ Pipeline 定义 ============
# Stage 0: DMA load
# Stage 1: TIU compute
# Stage 2: DMA store

# ============ 核心循环 ============
# for ... (外层循环描述)
#   for ... (内层循环描述)
#     load → compute → store
```

## 编写规则

### 规则 1: Tile 选择 (经验 #9, #11)
- 必须通过 choose_tiles() 获取初始配置
- 不允许硬编码 tile 值
- tile 变更必须经过公式过滤 + 板端验证

### 规则 2: Pipeline (经验 #10)
- 先画出 pipeline 时序图再写代码
- 最少 2 级流水 (load/compute)
- 理想 3 级流水 (load/compute/store)

### 规则 3: 路径设计 (经验 #5)
- 快路径 + 通用路径必须共存
- 快路径必须有明确的触发条件
- 通用路径作为 fallback 始终可用

### 规则 4: 精度 (经验 #12)
- FP16 输入 → FP32 累加 → FP16 输出
- 避免在循环中反复 cast
- Softmax 路径避免重复 exp 计算

### 规则 5: LMEM 管理
- 声明所有 buffer 的 LMEM 占用
- 双缓冲时需 x2 计算
- 预留 10% 余量给编译器临时变量

## 常见错误 checklist
- [ ] tile 超出 LMEM → 段错误
- [ ] 未处理 tail 块 (非整除情况) → 精度错误
- [ ] pipeline enable 后 lambda 不支持 → 编译失败 (用内联宏展开)
- [ ] 循环内 cast 过多 → 性能劣化
