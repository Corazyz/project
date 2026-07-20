# Skill: 性能分析流程 (Profiling)

## 触发条件
当 MFU 未达预期，需要诊断性能瓶颈时

## 核心原则 (经验 #3)
**必须先判定瓶颈类型，再选择优化方向**

## 瓶颈类型判定

### 方法: 使用 bigtpuprofile 采集数据
```bash
docker exec <container> env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  ENABLE_PROFILE=1 \
  python /tmp/<op>_bench.py
```

### 关键指标
| 指标 | 含义 | 判定标准 |
|------|------|---------|
| TiuWorkingRatio | TIU 利用率 | > 50% → TIU-bound |
| arith.copy cycle | DMA 搬运开销 | 远大于 TIU → DMA-bound |
| Parallelism | 指令并行度 | < 50% → Pipeline 不充分 |

### 瓶颈判定决策树
```
TiuWorkingRatio > 50%?
├── Yes → TIU-bound
│   优化方向: 增大 tile、减少冗余计算、提高 Cube 利用率
└── No → 检查 arith.copy
    arith.copy 占比 > 40%?
    ├── Yes → DMA-bound
    │   优化方向: 增大 tile 减少搬运次数、双缓冲、数据复用
    └── No → Mixed / Pipeline-bound
        优化方向: 重构 pipeline、load/compute/store 重叠
```

## 优化方向速查

| 瓶颈类型 | 优化策略 | 典型收益 |
|---------|---------|---------|
| DMA-bound | 增大 tile_m/tile_n | +50~100% |
| DMA-bound | K/V 双缓冲 | +10~30% |
| TIU-bound | 消除冗余计算 (cast等) | +5~15% |
| Pipeline | load/compute/store 三级流水 | +20~50% |
| Mixed | 综合策略 | 逐项改进 |

## 产出
- Profile 数据: `logs/profile_<timestamp>/`
- 瓶颈诊断: TIU-bound / DMA-bound / Mixed
- 推荐优化方向: 具体到哪个参数/结构需要改
