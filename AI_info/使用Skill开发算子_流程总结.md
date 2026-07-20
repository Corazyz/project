# 使用 Skill 开发算子 — 流程总结

> 来源: `/home/zyz/projects/test_project/AI_project/skill_lib/使用Skill开发算子.pdf`
> 基于 Codex 会话记录 (2026-03-10 ~ 2026-03-31)，记录 54000 行 JSONL

---

## 1. 概述

本文档记录了使用 **Skill 体系** (经验文档 + 衍生工具 Skill) 在 Codex Agent 辅助下开发 BM1690 TPU 上 Flash Attention 算子的完整过程。最终成果：MFU 从 0% 提升至 **62.96%**，超越手写 multicore 版本 (+21.7 MFU 百分点)。

---

## 2. Skill 16 阶段开发流水线

开发一个算子的标准流程：

| 阶段 | 内容 | 关键产出 |
|------|------|---------|
| #1 | PPL 参考实现 | Shape [BH,M,1,K] |
| #2-4 | PPL Kernel 开发 | flash_attention_simple.pl (含 MHA: shape, mask/causal) |
| #5 | Torch-TPU C++ 注册 | FlashAttentionSimple.cpp |
| #6-8 | CModel 编译验证 | ppl_compile.py → gen_ref → "npz compare PASSED" |
| #9-11 | Wheel 构建 | rebuild_TPU1686, new_clean, new_build |
| #12-15 | Docker 部署 + 板端测试 | torch_tpu_py312, 172.28.143.24 |
| #16 | Benchmark | 性能评测 + MFU 计算 |

---

## 3. 用户指令模板

初始开发指令：
- "仅根据 skill 的辅助，开发一个 [算子名] 算子"
- "以越简单实现越好为主，最好可以一次性实现一个可跑通的版本"
- "确认开始，注意不要参考原有算子，直接当作是新算子来开发"

FA_AI 进阶指令：
- "直接开始实现完整的 [算子]，把注册接口改成 [新名称]，以区分 AI-skill 生成的算子"
- "不要参考已有的手写算子的实现方案，完全按照 bm1690-llm-operator-dev skill 的流程来完整开发"

---

## 4. 环境配置关键变量

```bash
# PPL 编译相关
USING_PPLUSE_PPL=1
PPL_RUNTIME_PATH=<path>
PPL_PROJECT_ROOT=<path>
LD_LIBRARY_PATH=<path>

# Docker 环境
torch_tpu_py312  # Docker 镜像
172.28.143.24    # 板端服务器

# CModel 运行
USE_PPL=1
DISABLE_CACHE=ON
TPU_CACHE_LAUNCH_BATCH=1

# Benchmark
CHIP_ARCH=sg2260
USE_PPL=1
FA_TEST_SHAPE=1,4096,5,128
FA_CAUSAL=1
FA_WARMUP=5
FA_ITERS=20
```

---

## 5. 关键 Skill 经验文档 (高频触发 Top 5)

| 排名 | Skill 文档 | 触发次数 | 核心内容 |
|------|-----------|--------|--------|
| 1 | `05_attention_optimization.md` | 7 | 可编译性闸门、Pipeline 先行、三段式迭代闭环 |
| 2 | `04_kernel` (快路径共存) | 5 | 快路径要和回退路径共存、通用路径必须保持可回退 |
| 3 | 经验 #39, #40 | 4 | tile_n 扩展限制 + 公式过滤+板端回归双闸门 |
| 4 | `03_perf` (TIU/DMA 判定) | 4 | 先判定 TIU-bound / DMA-bound 再优化 |
| 5 | `05_attention#8` (Pipeline 先行) | 3 | 改 Attention kernel 前须输出 pipeline 方案 |

---

## 6. 板端测试命令

```bash
# FA_AI benchmark
docker exec torch_312_sj env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  FA_TEST_SHAPE=1,4096,5,128 FA_CAUSAL=1 \
  FA_WARMUP=5 FA_ITERS=20 \
  python /tmp/flash_attention_ai_bench.py

# Multicore benchmark (对比基线)
docker exec torch_312_sj env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  FA_TEST_SHAPE=1,4096,5,128 FA_CAUSAL=1 \
  python /tmp/flash_attention_mc_mfu.py
```

---

## 7. 关键文件路径

```
FA_AI 核心:
  /workspace/tpu-train_fa_ai/torch_tpu/csrc/ops/my_ops/flash_attention_ai.pl
  /workspace/tpu-train_fa_ai/torch_tpu/csrc/ops/my_ops/FlashAttentionAi.cpp

测试脚本:
  flash_attention_ai_bench.py      (单核 benchmark)
  flash_attention_ai_accuracy.py   (精度验证)
  flash_attention_mc_mfu.py        (multicore MFU)

日志:
  ~/fa_logs/new_opt*_hw_perf_20260313.log   (Simple 阶段)
  ~/workspace/fa_ai_cmp_20260325_*/          (FA_AI 编译)
  ~/workspace/fa_ai_runs/fa_ai_bench_*/      (FA_AI 测试)
```

---

## 8. MFU 优化轨迹 (FA_AI, shape=1,4096,5,128)

```
S1:  12.94%  — 首次上板
S2:  25.46%  — 动态 tiling (+96.7%)
S3:  29.31%  — causal_fast
S4:  34.74%  — scratch pipeline
S10: 38.46%  — causal_widem
S12: 45.39%  — 首次超越手写 causal
S14: 46.3%   — MaskAnalysis
S17: 62.96%  — mask_window_headpack (最终)
```

---

## 9. Skill 体系核心价值

1. **经验沉淀**: 47 条经验实时指导，避免重复踩坑
2. **流水线驱动**: 16 阶段标准流程，Agent 不走弯路
3. **A/B 实验**: 公式过滤 + 板端回归双闸门，快速迭代 2-3 天
4. **加速效果**: Agent 12 轮/4 小时完成优化 (vs 手写 1-2 天)
5. **性能建模**: TIU/DMA/Mixed 分析，"先诊断 DMA 还是 TIU 瓶颈"

---

## 10. 开发注意事项

- **编译回退时立即回退到稳定版本**，不继续冒险
- **板端性能结论必须基于短测 + 长稳态 + 多次复测**
- **tile_n 并非越大越好**，board 端回归是必要的安全网
- **快路径要和回退路径共存**，通用路径必须保持可回退
- **公式过滤 + 板端回归双闸门**：先用理论模型筛选，再上板验证
