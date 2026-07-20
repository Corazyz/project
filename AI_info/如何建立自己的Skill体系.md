# 如何建立自己的 Skill 体系

## 1. Skill 体系的三层结构

```
┌─────────────────────────────────────────────────┐
│  Layer 3: 衍生工具 Skill                          │
│  (bm1690-bigtpuprofile, benchmark-gating 等)     │
│  → 封装特定工具的使用流程                           │
├─────────────────────────────────────────────────┤
│  Layer 2: 经验文档 (experience_workflow.md)        │
│  → 编号规则 #1~#N，Agent 开发时自动触发             │
├─────────────────────────────────────────────────┤
│  Layer 1: 流水线阶段定义                           │
│  → 16 阶段标准流程，约束 Agent 的执行顺序           │
└─────────────────────────────────────────────────┘
```

---

## 2. 从零建立 Skill 的步骤

### Step 1: 定义你的开发流水线

把你的算子开发过程拆成有序阶段。每个阶段有明确的：
- 输入 (上一阶段产出)
- 动作 (这一步做什么)
- 验证标准 (怎么算通过)
- 产出 (交给下一阶段什么)

示例 (GEMM 算子):

```markdown
## 开发流水线

阶段1: Python Reference
  输入: 算子数学定义
  动作: NumPy 实现 + 单元测试
  验证: test_gemm_reference() 全部 PASS
  产出: gemm_reference.py

阶段2: PPL Kernel
  输入: reference + tile 配置
  动作: 编写 .pl 文件
  验证: ppl_compile 编译通过
  产出: gemm_simple.pl

阶段3: CModel 验证
  输入: .pl + reference npz
  动作: gen_ref → npz compare
  验证: max_diff < 阈值
  产出: PASSED 日志

阶段4: Torch-TPU 注册
  输入: .pl + API 定义
  动作: 编写 .cpp 注册接口
  验证: 编译无 error
  产出: GemmSimple.cpp

阶段5: Wheel 构建
  输入: 全部源码
  动作: rebuild + wheel install
  验证: import torch_tpu 无报错
  产出: .whl 文件

阶段6: 板端验证
  输入: wheel + bench 脚本
  动作: Docker 部署 + benchmark
  验证: 精度通过 + MFU 达标
  产出: 性能数据
```

### Step 2: 创建经验文档 (核心)

这是 Skill 体系最重要的部分。格式：

```markdown
# experience_workflow.md

## [领域名] 经验库

#编号 "一句话规则" (触发条件描述)

#1 "编译回退时立即恢复到稳定版本" (当编译失败或板端回退时触发)
#2 "先判定 TIU-bound / DMA-bound 再选优化方向" (当开始性能优化时触发)
#3 "tile_n 并非越大越好，board 端回归是必要的安全网" (当调整 tile 参数时触发)
#4 "快路径要和回退路径共存" (当新增快路径优化时触发)
#5 "板端性能结论必须基于短测 + 长稳态 + 多次复测" (当报告性能数据时触发)
```

**关键原则:**
- 每条经验必须可操作 (不是"注意性能"，而是"先 profile 判定瓶颈类型")
- 必须写清触发条件 (什么情况下应用这条)
- 来源于真实踩坑 (不是理论推导，是实际失败后的教训)

### Step 3: 创建衍生工具 Skill

把重复性操作封装成独立的 Skill 文档：

```markdown
# skill: bm1690-benchmark-gating

## 触发条件
当需要验证性能优化结果时调用

## 执行流程
1. 短测: WARMUP=5, ITERS=20 → 获取初始 MFU
2. 长稳态: ITERS=800 → 确认无性能衰减
3. 多次复测: 至少 3 次 → 取中位数
4. 对比执行: 避免同时运行其他任务干扰

## 验收标准
- MFU 提升 > 1% → 接受
- MFU 回退 → 立即恢复稳定版本
- 波动 > 3% → 数据不可信，需排查

## 输出格式
avg_ms=X.XXX TFLOPS=Y.YYY MFU=Z.ZZ%
```

### Step 4: 建立反哺机制

开发过程中持续向经验文档添加新规则：

```
开发中遇到问题 → 解决问题 → 提炼为一条经验 → 写入 experience_workflow.md
                                              ↓
                                    下次 Agent 开发时自动触发
```

---

## 3. 文件组织结构

```
my_skill_system/
├── pipeline.md                    # 流水线阶段定义
├── experience_workflow.md         # 经验库 (核心，持续增长)
├── skills/                        # 衍生工具 Skill
│   ├── 01_compile_check.md       # 编译验证流程
│   ├── 02_accuracy_test.md       # 精度验证流程
│   ├── 03_perf_profiling.md      # 性能分析流程
│   ├── 04_kernel_guidelines.md   # Kernel 编写规范
│   ├── 05_optimization.md        # 优化策略集
│   └── 06_benchmark_gating.md    # 性能验收闸门
└── templates/                     # 代码模板
    ├── reference_template.py
    ├── kernel_template.pl
    └── bench_template.py
```

---

## 4. 如何让 Agent 使用你的 Skill

### 方式 A: 写入 CLAUDE.md (Claude Code 环境)

在项目根目录创建 CLAUDE.md，引用 Skill：

```markdown
# CLAUDE.md

## 开发流程
开发算子时，严格按照 pipeline.md 的阶段顺序执行。

## 经验约束
每次做优化决策前，必须检查 experience_workflow.md 中是否有相关经验。

## 工具使用
- 性能验证: 按照 skills/06_benchmark_gating.md 执行
- 编译检查: 按照 skills/01_compile_check.md 执行
```

### 方式 B: 作为 System Prompt 的一部分 (API 调用)

```python
system_prompt = f"""
你是一个算子开发助手。

## 开发流水线
{open('pipeline.md').read()}

## 经验约束 (必须遵守)
{open('experience_workflow.md').read()}

## 当前阶段: {current_phase}
"""
```

### 方式 C: 作为 Codex Agent 的 Skill 配置

将经验文档放入 Agent 可读取的路径，Agent 在触发条件匹配时自动引用。

---

## 5. 经验文档的生长规律 (来自 PDF 的数据)

| 阶段 | 经验数量 | 积累速度 |
|------|---------|---------|
| Simple 版本开发 (3天) | 41 条 | ~14条/天 |
| FA_AI 版本开发 (7天) | +6 条 (总47) | ~1条/天 |

**规律:**
- 前期(踩坑期)经验增长快
- 后期(优化期)更多是验证已有经验、偶尔修正
- 最终稳定在 40-50 条左右是一个成熟领域的典型规模

---

## 6. 好的经验 vs 坏的经验

### 好的经验 (可操作、有触发条件)
```
#5 "快路径要和回退路径共存：热形状可增加专用快路径但须有明确触发条件；
    通用路径必须保持可回退" (当新增优化路径时触发)
```

### 坏的经验 (太模糊、不可操作)
```
# "注意性能"              → 不知道什么时候做什么
# "代码要写好"            → 没有具体标准
# "优化要小心"            → 不知道小心什么
```

---

## 7. 快速启动模板

现在就可以执行:

```bash
# 创建你的 Skill 体系骨架
mkdir -p ~/my_skills/{skills,templates}
touch ~/my_skills/pipeline.md
touch ~/my_skills/experience_workflow.md
touch ~/my_skills/skills/{01_compile,02_accuracy,03_perf,04_kernel,05_optimization,06_benchmark}.md
```

然后从第一次真实开发开始积累经验——每次踩坑就写一条。
