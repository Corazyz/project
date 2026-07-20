# Skill: 编译验证流程

## 触发条件
当 PPL Kernel (.pl) 编写完成，需要验证编译正确性时

## 前置检查
- [ ] .pl 文件语法无明显错误
- [ ] 环境变量已设置 (USING_PPLUSE_PPL=1, PPL_RUNTIME_PATH, PPL_PROJECT_ROOT)
- [ ] reference npz 已生成

## 执行流程

### 1. 生成参考数据
```bash
python reference/<op>_reference.py --save-npz
# 产出: input.npz, output_ref.npz
```

### 2. PPL 编译
```bash
export USING_PPLUSE_PPL=1
ppl_compile.py --input kernel/<op>.pl --output build/<op>.bin
```

### 3. CModel 运行
```bash
export USE_PPL=1 DISABLE_CACHE=ON TPU_CACHE_LAUNCH_BATCH=1
gen_ref --model build/<op>.bin --input input.npz --output output_cmodel.npz
```

### 4. NPZ 比对
```bash
npz_compare output_ref.npz output_cmodel.npz --atol 1e-2 --rtol 1e-3
# 期望输出: "npz compare PASSED"
```

## 失败处理
- 编译 error → 检查 .pl 语法，参考经验 #1 回退
- max_diff 超标 → 检查数据类型 cast、累加精度 (经验 #12)
- 段错误 → 检查 tile 配置是否超出 LMEM (经验 #9)

## 产出
- 编译日志: `logs/compile_<timestamp>.log`
- 比对结果: PASSED / FAILED + max_diff 值
