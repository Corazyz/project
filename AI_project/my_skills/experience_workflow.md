# 经验文档 (Experience Workflow)

## 使用说明
- 每条经验格式: `#编号 "规则" (触发条件)`
- Agent 在匹配触发条件时必须遵守对应规则
- 新经验从实际踩坑中提炼，追加到末尾
- 被证伪的经验标记 [已废弃] 并说明原因

---

## 通用开发经验

#1 "编译回退时立即恢复到稳定版本，不继续冒险"
   触发: 当编译失败或板端结果异常时
   来源: FA 开发 Round 8 — 冒险修改导致连续回退浪费时间

#2 "三段式迭代闭环: cmodel 正确性 → 板端硬件性能 → 公式建模偏差对比"
   触发: 每轮优化开始时
   来源: FA 开发全流程验证

#3 "先判定 TIU-bound / DMA-bound 再选择优化方向"
   触发: 当开始性能优化时
   来源: FA Step 8 — 盲目优化 TIU 但实际瓶颈在 DMA，浪费 2 轮

#4 "板端性能结论必须基于短测 + 长稳态 + 多次复测"
   触发: 当报告性能数据时
   来源: FA Step 17 — 短测数据波动大，800 iters 才稳定

#5 "快路径要和回退路径共存，通用路径必须保持可回退"
   触发: 当新增优化路径时
   来源: FA Round 6 — 快路径崩溃时无法回退到通用路径

---

## 环境与工具经验

#6 "PPL 编译需设置: USING_PPLUSE_PPL=1, PPL_RUNTIME_PATH, PPL_PROJECT_ROOT"
   触发: 当开始 PPL 编译时
   来源: FA 首次编译环境配置

#7 "wheel install 后必须重启 Docker 容器验证 import"
   触发: 当部署新 wheel 时
   来源: 旧 .so 缓存导致 import 成功但实际用的旧代码

#8 "cmodel 运行需设置: USE_PPL=1, DISABLE_CACHE=ON, TPU_CACHE_LAUNCH_BATCH=1"
   触发: 当运行 cmodel 验证时
   来源: 缓存导致使用旧编译结果

---

## Kernel 编写经验

#9 "tile 选择器必须匹配 kernel 实际 cost model"
   触发: 当实现 choose_tiles() 函数时
   来源: FA Step 6 — 理论模型与实际 kernel 不匹配，选出次优 tile

#10 "Pipeline 设计先行: 改 kernel 前须先输出 pipeline 方案"
    触发: 当重构 kernel 计算流程时
    来源: FA Step 3 — 未规划 pipeline 直接写导致多次返工

#11 "Tile 扩展需有公式过滤 + 板端回归双闸门"
    触发: 当调整 tile 大小时
    来源: FA Round 5 — tile_n 扩展带来最大单次性能跳跃，但也最危险

#12 "FP16 matmul 内部用 FP32 累加，大 K 时精度衰减严重"
    触发: 当实现矩阵乘 kernel 且 K > 256 时
    来源: GEMM 大 shape 精度问题

#13 "编译回退时不要 amend，创建新 commit 保留历史"
    触发: 当需要回退代码时
    来源: amend 导致丢失中间尝试的记录

---

## 待积累 (开发过程中填充)

# 下一条编号: #14
# 格式: #14 "规则" \n 触发: ... \n 来源: ...
