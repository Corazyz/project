"""
GEMM Reference Implementation (NumPy)
C = alpha * op(A) @ op(B) + beta * C

用途: 作为 PPL Kernel / 板端实现的正确性基准
"""
import numpy as np


def gemm_reference(A, B, C=None, alpha=1.0, beta=0.0, transA=False, transB=False):
    opA = A.T if transA else A
    opB = B.T if transB else B

    result = alpha * (opA.astype(np.float32) @ opB.astype(np.float32))
    if C is not None and beta != 0.0:
        result += beta * C.astype(np.float32)
    return result.astype(A.dtype)


def test_gemm_reference():
    np.random.seed(42)

    # Case 1: 基础矩阵乘
    M, K, N = 64, 128, 256
    A = np.random.randn(M, K).astype(np.float32)
    B = np.random.randn(K, N).astype(np.float32)
    result = gemm_reference(A, B)
    expected = A @ B
    assert np.allclose(result, expected, atol=1e-5), "Basic GEMM failed"

    # Case 2: alpha/beta
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

    # Case 4: FP16
    A_f16 = np.random.randn(M, K).astype(np.float16)
    B_f16 = np.random.randn(K, N).astype(np.float16)
    result = gemm_reference(A_f16, B_f16)
    expected = (A_f16.astype(np.float32) @ B_f16.astype(np.float32)).astype(np.float16)
    assert np.allclose(result.astype(np.float32), expected.astype(np.float32), atol=1e-1), "FP16 GEMM failed"

    # Case 5: 典型 LLM shapes
    llm_shapes = [
        (4096, 4096, 4096),
        (1, 4096, 11008),
        (32, 4096, 128),
        (8192, 8192, 8192),
    ]
    for m, k, n in llm_shapes:
        a = np.random.randn(m, k).astype(np.float16)
        b = np.random.randn(k, n).astype(np.float16)
        r = gemm_reference(a, b)
        assert r.shape == (m, n), f"Shape mismatch: expected ({m},{n}), got {r.shape}"

    print("=" * 50)
    print("All GEMM reference tests PASSED!")
    print(f"  Basic shapes: (64,128)x(128,256)")
    print(f"  LLM shapes: {llm_shapes}")
    print(f"  Modes: basic, alpha/beta, transA, transB, FP16")
    print("=" * 50)


if __name__ == "__main__":
    test_gemm_reference()
