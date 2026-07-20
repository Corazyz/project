# GEMM 算子开发经验文档
# 用于反哺 Skill 体系，Agent 开发时自动引用

## 通用经验 (继承自 FA 开发)

#1 PPL Kernel 编写时设置 USING_PPLUSE_PPL=1
#2 编译回退时立即回退到稳定版本，不继续冒险
#3 三段式迭代闭环：cmodel 正确性 → 板端硬件性能 → 公式建模偏差对比
#4 环境变量：PPL_RUNTIME_PATH, PPL_PROJECT_ROOT, LD_LIBRARY_PATH
#5 快路径要和回退路径共存，通用路径必须保持可回退
#6 先判定 TIU-bound / DMA-bound 再选择优化方向
#7 tile 选择器必须匹配 kernel 实际 cost model
#8 板端性能结论必须基于短测 + 长稳态 + 多次复测

## GEMM 专用经验

#9  GEMM 是计算密集型 (TIU-bound)，优先增大 tile 提高计算密度
#10 K 维度双缓冲是标配：当 tile 在 K 方向累加时，下一组 A/B tile 应并行 load
#11 transB 场景需预转置或使用 stride 访问，避免 DMA 碎片化
#12 FP16 matmul 内部用 FP32 累加器，最终 cast 回 FP16，否则大 K 精度衰减严重
#13 Multicore 切分优先按 M 维度（行切分），避免 N 维度切分导致的 partial sum 同步
