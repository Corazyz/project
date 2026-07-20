# 算子开发流水线定义

## 适用范围
BM1690 TPU 算子开发 (GEMM / Attention / Softmax / RMSNorm 等)

## 阶段定义

### 阶段 1: Python Reference
- 输入: 算子数学定义 (公式 + shape)
- 动作: NumPy 实现 + 多 shape 测试
- 验证: 所有 test case PASS
- 产出: `reference/<op>_reference.py`
- 耗时预估: 0.5 天

### 阶段 2: PPL Kernel
- 输入: reference + tile 配置
- 动作: 编写 .pl 文件，定义计算流水
- 验证: ppl_compile 编译通过 (无 error)
- 产出: `kernel/<op>.pl`
- 耗时预估: 1 天
- 前置条件: 阶段 1 通过

### 阶段 3: CModel 验证
- 输入: .pl + reference 生成的 npz
- 动作: ppl_compile → gen_ref → npz compare
- 验证: max_diff < 阈值 (FP16: 1e-2, FP32: 1e-5)
- 产出: "npz compare PASSED" 日志
- 耗时预估: 0.5 天
- 前置条件: 阶段 2 通过

### 阶段 4: Torch-TPU 注册
- 输入: .pl + API 定义 (输入输出 tensor)
- 动作: 编写 .cpp 注册文件 + .inc 声明
- 验证: 整体编译通过 (无 -Werror)
- 产出: `cpp/<Op>.cpp`
- 耗时预估: 0.5 天
- 前置条件: 阶段 3 通过

### 阶段 5: Wheel 构建部署
- 输入: 全部源码
- 动作: source envsetup.sh → rebuild_TPU1686 → wheel install → Docker 部署
- 验证: `import torch_tpu` 无报错，算子可调用
- 产出: .whl 文件 + Docker 容器就绪
- 耗时预估: 0.5 天
- 前置条件: 阶段 4 通过

### 阶段 6: 板端精度验证
- 输入: wheel + accuracy 脚本
- 动作: 多 shape 精度对比 (TPU vs CPU)
- 验证: allclose=True, max_diff < 阈值
- 产出: 精度报告
- 耗时预估: 0.5 天
- 前置条件: 阶段 5 通过

### 阶段 7: 板端性能 Benchmark
- 输入: wheel + bench 脚本
- 动作: 短测 + 长稳态 + 多次复测
- 验证: MFU 达到目标值
- 产出: 性能报告 (avg_ms, TFLOPS, MFU%)
- 耗时预估: 0.5 天
- 前置条件: 阶段 6 通过

### 阶段 8: 性能优化迭代
- 输入: Profile 数据 + 经验文档
- 动作: 识别瓶颈 → 优化 → 验证 (循环)
- 验证: MFU 持续提升，精度不回退
- 产出: 优化后的 kernel + 更新的经验
- 耗时预估: 3-7 天
- 前置条件: 阶段 7 完成基线

---

## 阶段间的闸门规则

1. **不允许跳阶段**: 必须按顺序执行，上一阶段未通过不得进入下一阶段
2. **回退规则**: 任何阶段失败，回退到上一个稳定状态，不继续冒险
3. **经验反哺**: 每个阶段遇到的问题必须记录到 experience_workflow.md
