# GEMM 算子开发指南 (基于 Skill 流程)

> 目标: 使用 Skill 体系开发 BM1690 TPU 上的 GEMM (General Matrix Multiplication) 算子
> 参考流程: Skill 16 阶段流水线

---

## 1. GEMM 算子定义

```
C = alpha * A @ B + beta * C

A: [M, K]  (或转置 [K, M])
B: [K, N]  (或转置 [N, K])
C: [M, N]

支持参数:
- transA: bool — 是否转置 A
- transB: bool — 是否转置 B
- alpha: float — 缩放因子 (默认 1.0)
- beta: float — 累加因子 (默认 0.0)
- dtype: fp16 / fp32 / bf16
```

---

## 2. 开发阶段规划

### Phase 1: Python Reference + CModel 验证 (Day 1)

| 步骤 | 产出 | 验证标准 |
|------|------|---------|
| NumPy 参考实现 | gemm_reference.py | 正确性基准 |
| PPL Kernel 编写 | gemm_simple.pl | 编译通过 |
| CModel 比对 | npz compare PASSED | max_diff < 1e-3 |

### Phase 2: 上板跑通 (Day 2)

| 步骤 | 产出 | 验证标准 |
|------|------|---------|
| Torch-TPU 注册 | GemmSimple.cpp | 编译通过 |
| Wheel 构建 | rebuild_TPU1686 | 无 -Werror |
| 板端精度测试 | gemm_accuracy.py | allclose=True |

### Phase 3: 性能优化 (Day 3+)

| 步骤 | 产出 | 验证标准 |
|------|------|---------|
| Tiling 优化 | choose_gemm_tiles() | MFU 提升 |
| Pipeline 重构 | load/compute/store 重叠 | 利用率提升 |
| Multicore 扩展 | 多核并行 | 线性加速比 |

---

## 3. Step-by-Step 可执行步骤

### Step 0: 创建项目目录结构

```bash
mkdir -p /home/zyz/projects/test_project/AI_project/gemm_operator/{reference,kernel,cpp,test,logs}
```

### Step 1: Python Reference 实现

文件: `reference/gemm_reference.py`

```python
import numpy as np

def gemm_reference(A, B, C=None, alpha=1.0, beta=0.0, transA=False, transB=False):
    """
    GEMM: C = alpha * op(A) @ op(B) + beta * C
    A: [M, K] or [K, M] if transA
    B: [K, N] or [N, K] if transB
    C: [M, N]
    """
    opA = A.T if transA else A
    opB = B.T if transB else B
    
    result = alpha * (opA @ opB)
    if C is not None and beta != 0.0:
        result += beta * C
    return result


def test_gemm_reference():
    """验证参考实现正确性"""
    np.random.seed(42)
    
    # Case 1: 基础矩阵乘
    M, K, N = 64, 128, 256
    A = np.random.randn(M, K).astype(np.float32)
    B = np.random.randn(K, N).astype(np.float32)
    
    result = gemm_reference(A, B)
    expected = A @ B
    assert np.allclose(result, expected, atol=1e-5), "Basic GEMM failed"
    
    # Case 2: 带 alpha/beta
    C = np.random.randn(M, N).astype(np.float32)
    result = gemm_reference(A, B, C, alpha=0.5, beta=1.0)
    expected = 0.5 * (A @ B) + 1.0 * C
    assert np.allclose(result, expected, atol=1e-5), "Alpha/Beta GEMM failed"
    
    # Case 3: transA + transB
    A_t = np.random.randn(K, M).astype(np.float32)
    B_t = np.random.randn(N, K).astype(np.float32)
    result = gemm_reference(A_t, B_t, transA=True, transB=True)
    expected = A_t.T @ B_t.T
    assert np.allclose(result, expected, atol=1e-5), "Trans GEMM failed"
    
    # Case 4: FP16 精度
    A_f16 = np.random.randn(M, K).astype(np.float16)
    B_f16 = np.random.randn(K, N).astype(np.float16)
    result = gemm_reference(A_f16, B_f16)
    expected = (A_f16 @ B_f16)
    assert np.allclose(result, expected, atol=1e-2), "FP16 GEMM failed"
    
    print("All GEMM reference tests PASSED!")
    print(f"  Shapes tested: ({M},{K})x({K},{N}), FP32 + FP16")
    print(f"  Modes: basic, alpha/beta, transA, transB")


if __name__ == "__main__":
    test_gemm_reference()
```

### Step 2: PPL Kernel 骨架

文件: `kernel/gemm_simple.pl`

```perl
# GEMM Simple Kernel for BM1690
# Shape: A[M,K] x B[K,N] = C[M,N]
# Tiling: tile_m, tile_k, tile_n

# 初始 Tile 配置 (保守值，确保首次跑通)
tile_m = 64
tile_k = 64
tile_n = 64

# Pipeline: 2 级流水
#   Stage 0: DMA load A_tile, B_tile
#   Stage 1: TIU matmul + store C_tile

# 核心循环结构:
# for m_idx in range(0, M, tile_m):
#   for n_idx in range(0, N, tile_n):
#     acc = 0
#     for k_idx in range(0, K, tile_k):
#       A_tile = load(A[m_idx:m_idx+tile_m, k_idx:k_idx+tile_k])
#       B_tile = load(B[k_idx:k_idx+tile_k, n_idx:n_idx+tile_n])
#       acc += matmul(A_tile, B_tile)
#     store(C[m_idx:m_idx+tile_m, n_idx:n_idx+tile_n], acc * alpha + beta * C_tile)
```

### Step 3: Tile 选择器

文件: `kernel/choose_gemm_tiles.py`

```python
def choose_gemm_tiles(M, K, N, dtype='fp16'):
    """
    基于 V7.1 cycle 模型选择最优 tile 配置
    评分 = matmul_cycles + overhead + dma_penalty
    """
    LMEM_SIZE = 256 * 1024  # BM1690 LMEM per core
    dtype_bytes = 2 if dtype == 'fp16' else 4
    
    candidates = []
    for tile_m in [32, 64, 128, 256]:
        for tile_k in [32, 64, 128, 256]:
            for tile_n in [32, 64, 128, 256]:
                # LMEM 容量检查: A_tile + B_tile + C_tile
                mem_needed = (tile_m * tile_k + tile_k * tile_n + tile_m * tile_n) * dtype_bytes
                if mem_needed > LMEM_SIZE:
                    continue
                
                # 计算循环次数
                m_tiles = (M + tile_m - 1) // tile_m
                k_tiles = (K + tile_k - 1) // tile_k
                n_tiles = (N + tile_n - 1) // tile_n
                
                # 估算 cycle (简化模型)
                matmul_cycles = m_tiles * n_tiles * k_tiles * (tile_m * tile_k * tile_n)
                dma_cycles = m_tiles * n_tiles * k_tiles * (tile_m * tile_k + tile_k * tile_n) * dtype_bytes
                
                score = max(matmul_cycles, dma_cycles)  # bound by slower one
                candidates.append((score, tile_m, tile_k, tile_n))
    
    candidates.sort()
    best = candidates[0] if candidates else (0, 64, 64, 64)
    return {'tile_m': best[1], 'tile_k': best[2], 'tile_n': best[3]}


if __name__ == "__main__":
    # 典型 LLM shape 测试
    shapes = [
        (4096, 4096, 4096),    # Square
        (1, 4096, 11008),      # FFN up
        (4096, 4096, 128),     # Attention QK^T (per head)
        (8192, 8192, 8192),    # Large
    ]
    for M, K, N in shapes:
        tiles = choose_gemm_tiles(M, K, N)
        print(f"Shape ({M},{K},{N}): tile_m={tiles['tile_m']}, tile_k={tiles['tile_k']}, tile_n={tiles['tile_n']}")
```

### Step 4: 板端 Benchmark 脚本

文件: `test/gemm_bench.py`

```python
import torch
import time
import os

def gemm_benchmark(M, K, N, dtype=torch.float16, warmup=5, iters=20):
    """GEMM 板端性能测试"""
    A = torch.randn(M, K, dtype=dtype).to('tpu')
    B = torch.randn(K, N, dtype=dtype).to('tpu')
    
    # Warmup
    for _ in range(warmup):
        C = torch.matmul(A, B)
    torch.tpu.synchronize()
    
    # Benchmark
    start = time.perf_counter()
    for _ in range(iters):
        C = torch.matmul(A, B)
    torch.tpu.synchronize()
    elapsed = (time.perf_counter() - start) / iters
    
    # MFU 计算
    flops = 2 * M * K * N  # GEMM FLOPs = 2*M*K*N
    peak_tflops = 131.072  # BM1690 FP16 Cube peak
    tflops = flops / elapsed / 1e12
    mfu = tflops / peak_tflops * 100
    
    print(f"Shape ({M},{K},{N}): avg_ms={elapsed*1000:.3f}, TFLOPS={tflops:.3f}, MFU={mfu:.2f}%")
    return {'avg_ms': elapsed*1000, 'tflops': tflops, 'mfu': mfu}


if __name__ == "__main__":
    test_shape = os.environ.get('GEMM_TEST_SHAPE', '4096,4096,4096')
    M, K, N = map(int, test_shape.split(','))
    warmup = int(os.environ.get('GEMM_WARMUP', '5'))
    iters = int(os.environ.get('GEMM_ITERS', '20'))
    
    gemm_benchmark(M, K, N, warmup=warmup, iters=iters)
```

### Step 5: 精度验证脚本

文件: `test/gemm_accuracy.py`

```python
import torch
import numpy as np

def gemm_accuracy_test():
    """精度验证: TPU vs CPU"""
    shapes = [
        (64, 64, 64),
        (128, 256, 512),
        (1024, 1024, 1024),
        (4096, 4096, 4096),
        (1, 4096, 11008),
    ]
    
    all_pass = True
    for M, K, N in shapes:
        A = torch.randn(M, K, dtype=torch.float16)
        B = torch.randn(K, N, dtype=torch.float16)
        
        # CPU reference
        C_ref = torch.matmul(A, B)
        
        # TPU
        A_tpu = A.to('tpu')
        B_tpu = B.to('tpu')
        C_tpu = torch.matmul(A_tpu, B_tpu).cpu()
        
        # Compare
        max_diff = (C_ref - C_tpu).abs().max().item()
        passed = max_diff < 1e-2  # FP16 tolerance
        status = "PASS" if passed else "FAIL"
        print(f"[{status}] Shape ({M},{K},{N}): max_diff={max_diff:.6f}")
        
        if not passed:
            all_pass = False
    
    print(f"\n{'='*50}")
    print(f"Overall: {'ALL PASSED' if all_pass else 'SOME FAILED'}")
    return all_pass


if __name__ == "__main__":
    gemm_accuracy_test()
```

---

## 4. 经验文档初始化 (experience_workflow.md)

基于 Flash Attention 开发中的通用经验，GEMM 可直接复用：

```
#1  PPL Kernel 编写时设置 USING_PPLUSE_PPL=1
#2  编译回退时立即回退到稳定版本，不继续冒险
#3  三段式迭代闭环：cmodel 正确性 → 板端硬件性能 → 公式建模偏差对比
#4  环境/编译顺序：PPL_RUNTIME_PATH, PPL_PROJECT_ROOT, LD_LIBRARY_PATH
#5  快路径要和回退路径共存，通用路径必须保持可回退
#6  先判定 TIU-bound / DMA-bound 再选择优化方向
#7  tile 选择器必须匹配 kernel 实际 cost model
#8  板端性能结论必须基于短测 + 长稳态 + 多次复测
```

---

## 5. GEMM 特有优化方向

| 优化点 | 策略 | 预期收益 |
|--------|------|---------|
| Tile 大小 | 增大 tile_m/tile_n 提高计算密度 | MFU +20~40% |
| K 维度流水 | 双缓冲 A/B tile，load 与 compute 重叠 | 隐藏 DMA 延迟 |
| 转置优化 | transB 时 B 预转置到 LMEM 友好布局 | 减少 DMA 次数 |
| Multicore | 按 M 或 N 维度切分到多核 | 近线性加速 |
| 累加精度 | 内部用 FP32 累加，输出时 cast 回 FP16 | 精度提升 |

---

## 6. MFU 计算公式

```
FLOPS = 2 * M * K * N                    # GEMM 浮点运算数
Peak_TFLOPS = 131.072                     # BM1690 FP16 Cube 峰值
MFU = (FLOPS / time_seconds) / (Peak_TFLOPS * 1e12) * 100%
```

---

## 7. 板端测试命令模板

```bash
# GEMM benchmark
docker exec torch_312_sj env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  GEMM_TEST_SHAPE=4096,4096,4096 \
  GEMM_WARMUP=5 GEMM_ITERS=20 \
  python /tmp/gemm_bench.py

# 精度验证
docker exec torch_312_sj env \
  CHIP_ARCH=sg2260 USE_PPL=1 \
  python /tmp/gemm_accuracy.py
```
