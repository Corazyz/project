"""
GEMM Tile 选择器
基于 LMEM 容量和简化 cycle 模型选择最优 tile 配置
"""


def choose_gemm_tiles(M, K, N, dtype='fp16', lmem_size=256*1024):
    dtype_bytes = 2 if dtype in ('fp16', 'bf16') else 4

    candidates = []
    for tile_m in [32, 64, 128, 256]:
        for tile_k in [32, 64, 128, 256]:
            for tile_n in [32, 64, 128, 256]:
                # LMEM check: A_tile + B_tile + C_tile (双缓冲 x2)
                mem_single = (tile_m * tile_k + tile_k * tile_n + tile_m * tile_n) * dtype_bytes
                mem_double_buffer = mem_single * 2
                if mem_double_buffer > lmem_size:
                    continue

                m_tiles = (M + tile_m - 1) // tile_m
                k_tiles = (K + tile_k - 1) // tile_k
                n_tiles = (N + tile_n - 1) // tile_n
                total_tiles = m_tiles * k_tiles * n_tiles

                # Per-tile TIU cycles: matmul compute density
                # Larger tiles = better utilization of Cube unit
                tiu_per_tile = tile_m * tile_n * tile_k
                tiu_total = total_tiles * tiu_per_tile

                # DMA: each tile incurs setup overhead + data transfer
                DMA_SETUP_CYCLES = 200  # per-tile DMA descriptor overhead
                bytes_per_tile = (tile_m * tile_k + tile_k * tile_n) * dtype_bytes
                dma_total = total_tiles * (bytes_per_tile + DMA_SETUP_CYCLES)

                # Pipeline efficiency: larger tiles hide DMA behind compute
                compute_per_tile = tile_m * tile_n * tile_k
                pipeline_ratio = min(1.0, compute_per_tile / max(1, bytes_per_tile + DMA_SETUP_CYCLES))
                effective_time = tiu_total + dma_total * (1 - pipeline_ratio * 0.8)

                # Padding penalty
                pad_m = m_tiles * tile_m - M
                pad_k = k_tiles * tile_k - K
                pad_n = n_tiles * tile_n - N
                wasted_compute = (pad_m * K * N + pad_k * M * N + pad_n * M * K)
                effective_time += wasted_compute * 0.5

                score = effective_time

                candidates.append({
                    'score': score,
                    'tile_m': tile_m,
                    'tile_k': tile_k,
                    'tile_n': tile_n,
                    'total_tiles': total_tiles,
                    'mem_bytes': mem_double_buffer,
                    'bound': 'TIU' if tiu_total >= dma_total else 'DMA',
                })

    if not candidates:
        return {'tile_m': 32, 'tile_k': 32, 'tile_n': 32, 'bound': 'unknown'}

    candidates.sort(key=lambda x: x['score'])
    return candidates[0]


if __name__ == "__main__":
    print("GEMM Tile Selection Results")
    print("=" * 70)
    print(f"{'Shape':<25} {'tile_m':>6} {'tile_k':>6} {'tile_n':>6} {'tiles':>6} {'bound':>5} {'mem_KB':>7}")
    print("-" * 70)

    shapes = [
        (4096, 4096, 4096, "Square 4K"),
        (1, 4096, 11008, "FFN up proj"),
        (4096, 4096, 128, "Attn QK^T"),
        (8192, 8192, 8192, "Square 8K"),
        (2048, 2048, 2048, "Square 2K"),
        (128, 4096, 4096, "Batch=128"),
    ]

    for M, K, N, label in shapes:
        result = choose_gemm_tiles(M, K, N)
        print(f"({M},{K},{N}) {label:<10} "
              f"{result['tile_m']:>6} {result['tile_k']:>6} {result['tile_n']:>6} "
              f"{result['total_tiles']:>6} {result['bound']:>5} "
              f"{result['mem_bytes']/1024:>6.1f}")
