# CLAUDE.md — 算子开发项目配置

## 开发流程
开发算子时，严格按照 `my_skills/pipeline.md` 的阶段顺序执行。不允许跳阶段。

## 经验约束
每次做优化决策前，必须检查 `my_skills/experience_workflow.md` 中是否有相关经验。
遇到新问题时，解决后立即追加新经验到文档末尾。

## Skill 使用
- 编译验证 → `my_skills/skills/01_compile_check.md`
- 精度验证 → `my_skills/skills/02_accuracy_test.md`
- 性能分析 → `my_skills/skills/03_perf_profiling.md`
- Kernel 编写 → `my_skills/skills/04_kernel_guidelines.md`
- 优化策略 → `my_skills/skills/05_optimization.md`
- 性能验收 → `my_skills/skills/06_benchmark_gating.md`

## 关键规则
1. 编译失败 → 立即回退，不继续冒险
2. 先诊断瓶颈类型 (TIU/DMA)，再选优化方向
3. 性能数据必须经过三步验证 (短测+长稳态+复测)
4. 每轮优化记录: 策略→结果→经验
