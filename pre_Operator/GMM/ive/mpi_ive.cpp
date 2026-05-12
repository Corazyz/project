// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "mpi_ive.h"
// #include "cvi_ive_hw.h"

// CVI_S32 CVI_MPI_IVE_GMM(IVE_HANDLE *pIveHandle, COMMON_IMAGE_S *pstSrc, COMMON_IMAGE_S *pstFg,
// 	COMMON_IMAGE_S *pstBg, IVE_MEM_INFO_S *pstModel, IVE_GMM_CTRL_S *pstGmmCtrl, CVI_BOOL bInstant)
// {
// 	UNUSED(pIveHandle);
// 	UNUSED(bInstant);

// 	//printf("CVI_MPI_IVE_GMM\n");

// 	//
// 	// TODO: Here shall be real MPI IVE driver code, which fill in register command, then pass to hw/cmodel
// 	// FIXME: call CVI_HW_XXX() directly for now
// 	//
// 	CVI_HW_GMM(
// 			(CVI_U8 *)pstSrc->au64VirAddr[0], (CVI_U16)pstSrc->au32Stride[0],
// 			(CVI_U16)pstSrc->u32Width, (CVI_U16)pstSrc->u32Height,
// 			(CVI_U8 *)pstModel->u64VirAddr, pstGmmCtrl->u8ModelNum,
// 			(CVI_U8 *)pstFg->au64VirAddr[0], (CVI_U16)pstFg->au32Stride[0],
// 			(CVI_U8 *)pstBg->au64VirAddr[0], (CVI_U16)pstBg->au32Stride[0],
// 			pstGmmCtrl->u0q16LearnRate, pstGmmCtrl->u0q16BgRatio, pstGmmCtrl->u8q8VarThr,
// 			pstGmmCtrl->u22q10NoiseVar, pstGmmCtrl->u22q10MaxVar, pstGmmCtrl->u22q10MinVar,
// 			pstGmmCtrl->u0q16InitWeight,
// 			0, // enDetectShadow
// 			0, // u0q8ShadowThr
// 			8, // u8SnsFactor, as from GMM2 sample
// 			(CVI_U8)((pstSrc->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? 1 : 0));
// 	return CVI_SUCCESS;
// }

// CVI_S32 CVI_MPI_IVE_GMM2(IVE_HANDLE *pIveHandle,IVE_SRC_IMAGE_S *pstSrc,IVE_SRC_IMAGE_S *pstFactor,
// 	IVE_DST_IMAGE_S *pstFg,IVE_DST_IMAGE_S *pstBg,IVE_DST_IMAGE_S *pstMatchModelInfo,
// 	IVE_MEM_INFO_S  *pstModel,IVE_GMM2_CTRL_S *pstGmm2Ctrl,CVI_BOOL bInstant)
// {
// 	UNUSED(pIveHandle);
// 	UNUSED(bInstant);

// 	//printf("CVI_MPI_IVE_GMM2\n");

// 	//
// 	// TODO: Here shall be real MPI IVE driver code, which fill in register command, then pass to hw/cmodel
// 	// FIXME: call CVI_HW_XXX() directly for now
// 	//
// 	CVI_HW_GMM2(
// 			(CVI_U8 *)pstSrc->au64VirAddr[0], (CVI_U16)pstSrc->au32Stride[0],
// 			(CVI_U16 *)pstFactor->au64VirAddr[0], (CVI_U16)pstFactor->au32Stride[0],
// 			(CVI_U8 *)pstFg->au64VirAddr[0], (CVI_U16)pstFg->au32Stride[0],
// 			(CVI_U8 *)pstBg->au64VirAddr[0], (CVI_U16)pstBg->au32Stride[0],
// 			(CVI_U8 *)pstMatchModelInfo->au64VirAddr[0], (CVI_U16)pstMatchModelInfo->au32Stride[0],
// 			(CVI_U8 *)pstModel->u64VirAddr,
// 			pstGmm2Ctrl->u8ModelNum, pstGmm2Ctrl->u16LifeThr,
// 			pstGmm2Ctrl->u16FreqInitVal, pstGmm2Ctrl->u16FreqReduFactor,
// 			pstGmm2Ctrl->u16FreqAddFactor, pstGmm2Ctrl->u16FreqThr,
// 			pstGmm2Ctrl->u16VarRate, pstGmm2Ctrl->u9q7MaxVar, pstGmm2Ctrl->u9q7MinVar,
// 			pstGmm2Ctrl->u8GlbSnsFactor, pstGmm2Ctrl->enSnsFactorMode,
// 			pstGmm2Ctrl->u16GlbLifeUpdateFactor, pstGmm2Ctrl->enLifeUpdateFactorMode,
// 			(CVI_U16)pstSrc->u32Width, (CVI_U16)pstSrc->u32Height,
// 			(CVI_U8)((pstSrc->enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? 1 : 0));
// 	return CVI_SUCCESS;
// }

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
// #include "cvi_ive_hw.h"

#define MAX2(a, b)  (( (a) > (b) )? (a) : (b) )
#define MIN2(a, b)  (( (a) < (b) )? (a) : (b) )

/*
https://github.com/opencv/opencv/blob/2.4/modules/video/src/bgfg_gaussmix.cpp
*/

int CVI_HW_SingleGMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor)
{
    int result = 0;
    int min_k_Km1;
    uint8_t swapbuf[7];
    uint32_t One_div_wscale, sortKey_k1p1, sortKey_k1;
    int64_t diff;
    int k1, k, kForeground, kHit;
    uint8_t pix;
    uint32_t dw, wsum;
    uint64_t dist2, var;
    uint16_t x, y, w, mu;
    uint8_t *pstBg0;
    uint8_t *mptr;
    uint8_t *pstFg0;
    uint8_t *pstSrc0;

    pstSrc0 = pstSrc;
    pstFg0 = pstFg;
    pstBg0 = pstBg;
    for ( y = 0; y < Src_u16Height ; ++y )
    {
        result = y;
        for ( x = 0; x < Src_u16Width ; ++x )
        {
            kHit = -1;
            kForeground = -1;
            pix = pstSrc0[x];
            wsum = 0;
            mptr = pstModel;
            for ( k = 0; k < u8ModelNum; ++k )
            {
                mptr = &pstModel[7 * k];              // v41 = &mptr[i*7], 7 bytes
                w = mptr[0] + (mptr[1] << 8);
                wsum += w;
                if ( !w )
                    break;  // break condition #1: k will be less than u8ModelNum if break
                mu = mptr[2] + (mptr[3] << 8);
                var = mptr[4] + (mptr[5] << 8) + (mptr[6] << 16);
                diff = (pix << 8) - mu;
                dist2 = (diff * diff + 32) >> 6;
                if ( dist2 < (u8q8VarThr * var + 128) >> 8 )
                {
                    wsum -= w;
                    dw = ((0xFFFF - w) * (uint32_t)u0q16LearnRate + 0x8000) >> 16;
                    w += dw;
                    *(uint16_t *)mptr = w;               // mptr[k].weight = w+dw
                    mu += ((u0q16LearnRate * diff) >> 16);
                    *((uint16_t *)mptr + 1) = mu;        // mptr[k].mean = mu + alpha * diff
                    var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
                    var = MIN2(var, u22q10MaxVar);
                    var = MAX2(var, u22q10MinVar);
                    mptr[4] = (uint8_t)(var & 0xFF);
                    mptr[5] = (uint8_t)((var >> 8) & 0xFF);
                    mptr[6] = (uint8_t)((var >> 16) & 0xFF);
                    for ( k1 = k - 1; k1 >= 0; --k1 )
                    {
                        // if( mptr[k1].sortKey >= mptr[k1+1].sortKey )
                        //     break;
                        // std::swap( mptr[k1], mptr[k1+1] );
                        //
                        sortKey_k1   = pstModel[7 * k1] + (pstModel[7 * k1 + 1] << 8);
                        sortKey_k1p1 = pstModel[7 * k1 + 7] + (pstModel[7 * k1 + 8] << 8);
                        if ( sortKey_k1 > sortKey_k1p1 )
                            break;
                        memcpy(swapbuf, &pstModel[7 * k1], 7);
                        memcpy(&pstModel[7 * k1], &pstModel[7 * k1 + 7], 7);
                        memcpy(&pstModel[7 * k1 + 7], swapbuf, 7);
                    }
                    kHit = k1 + 1;
                    break;
                }
            }
            // k < u8ModelNum: break condition #1
            if ( kHit >= 0 )
            {
                while ( k < u8ModelNum )
                {
                    // wsum += mptr[k].weight;
                    w = pstModel[7 * k] + (pstModel[7 * k + 1] << 8);
                    wsum += w;
                    ++k;
                }
            }
            else
            {
                // kHit = k = std::min(k, K-1);
                // wsum += w0 - mptr[k].weight;
                // mptr[k].weight = w0;
                // mptr[k].mean = pix;
                // mptr[k].var = var0;
                // mptr[k].sortKey = sk0;
                //
                if ( k <= u8ModelNum - 1 )
                    min_k_Km1 = k;
                else
                    min_k_Km1 = u8ModelNum - 1;
                k = min_k_Km1;
                kHit = min_k_Km1;
                w = pstModel[7 * min_k_Km1] + (pstModel[7 * min_k_Km1 + 1] << 8);
                wsum += u0q16InitWeight - w;
                pstModel[7 * k] = u0q16InitWeight;
                pstModel[7 * k + 1] = (u0q16InitWeight >> 8);
                pstModel[7 * k + 3] = pix;
                pstModel[7 * k + 2] = 0;
                var = 4 * u22q10NoiseVar;
                pstModel[7 * k + 6] = (uint8_t)((var >> 16) & 0xFF);
                pstModel[7 * k + 5]  = (uint8_t)((var >> 8) & 0xFF);
                pstModel[7 * k + 4]  = (uint8_t)(var & 0xFF);
            }
            One_div_wscale = wsum;                    // float wscale = 1.f/wsum;
            wsum = 0;
            for ( k = 0; k < u8ModelNum; ++k )
            {
                // wsum += mptr[k].weight *= wscale;
                w = pstModel[7 * k] + (pstModel[7 * k + 1] << 8);
                w = 0xFFFF * (uint32_t)w / One_div_wscale;
                *(uint16_t *)&pstModel[7 * k] = w;         // mptr[k].weight *= wscale;
                wsum += w;
                if ( wsum > u0q16BgRatio && kForeground < 0 )
                    kForeground = k + 1;
            }
            if ( enDetectShadow )
            {
                // pstFg0[x] = (kHit < kForeground)? 0 : 0xFF;
                // if ( (uint8_t)pstFg0[x] == 0xFF
                //         && CVI_HW_DetectGMMShadow(
                //             (uint8_t *)&pstSrc0[x],
                //             pstModel,
                //             u8ModelNum,
                //             0,
                //             u0q16BgRatio,
                //             u0q8ShadowThr,
                //             u8SnsFactor) )
                // {
                //     pstFg0[x] = 127;
                // }
            }
            else
            {
                pstFg0[x] = (kHit >= kForeground)? 0xFF : 0;
            }
            pstBg0[x] = pstModel[3];             // what is it?
            pstModel += 7 * u8ModelNum;
        }
        pstSrc0 += Src_u16Stride;
        pstFg0 += Fg_u16Stride;
        pstBg0 += Bg_u16Stride;
    }
    return result;
}

/*
https://github.com/opencv/opencv/blob/2.4/modules/video/src/bgfg_gaussmix.cpp
*/
int CVI_HW_RGBGMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor)
{
    int result = 0;
    int min_k_Km1;
    uint8_t swapbuf[11];
    uint32_t One_div_wscale, sortKey_k1p1, sortKey_k1;
    int64_t diff_c0, diff_c1, diff_c2;
    int k1, k, kForeground, kHit;
    uint8_t pix_c0, pix_c1, pix_c2;
    uint32_t dw, wsum;
    uint64_t dist2, var;
    uint16_t x, y, w, mu_c0, mu_c1, mu_c2;
    uint8_t *pstBg0;
    uint8_t *mptr;
    uint8_t *pstFg0;
    uint8_t *pstSrc0;

    pstSrc0 = pstSrc;
    pstFg0 = pstFg;
    pstBg0 = pstBg;
    for ( y = 0; y < Src_u16Height ; ++y )
    {
        result = y;
        for ( x = 0; x < Src_u16Width ; ++x )
        {
            kHit = -1;
            kForeground = -1;
            pix_c0 = pstSrc0[3 * x];
            pix_c1 = pstSrc0[3 * x + 1];
            pix_c2 = pstSrc0[3 * x + 2];
            wsum = 0;
            mptr = pstModel;
            for ( k = 0; k < u8ModelNum; ++k )
            {
                mptr = &pstModel[11 * k];
                w = mptr[0] + (mptr[1] << 8);
                wsum += w;
                if ( !w )
                    break;  // break condition #1: k will be less than u8ModelNum if break
                mu_c0 = mptr[2] + (mptr[3] << 8);
                mu_c1 = mptr[4] + (mptr[5] << 8);
                mu_c2 = mptr[6] + (mptr[7] << 8);
                var = mptr[8] + (mptr[9] << 8) + (mptr[10] << 16);
                diff_c0 = (pix_c0 << 8) - mu_c0;
                diff_c1 = (pix_c1 << 8) - mu_c1;
                diff_c2 = (pix_c2 << 8) - mu_c2;
                dist2 = (diff_c2 * diff_c2 + diff_c1 * diff_c1 + diff_c0 * diff_c0 + 32) >> 6;
                if ( dist2 < (u8q8VarThr * var + 128) >> 8 )
                {
                    wsum -= w;
                    dw = ((0xFFFF - w) * (uint32_t)u0q16LearnRate + 0x8000) >> 16;
                    w += dw;
                    *(uint16_t *)mptr = w;
                    mu_c0 += ((uint32_t)u0q16LearnRate * diff_c0) >> 16;
                    mu_c1 += ((uint32_t)u0q16LearnRate * diff_c1) >> 16;
                    mu_c2 += ((uint32_t)u0q16LearnRate * diff_c2) >> 16;
                    *((uint16_t *)mptr + 1) = mu_c0;
                    *((uint16_t *)mptr + 2) = mu_c1;
                    *((uint16_t *)mptr + 3) = mu_c2;
                    var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
                    var = MIN2(var, u22q10MaxVar);
                    var = MAX2(var, u22q10MinVar);
                    mptr[8] = (uint8_t)(var & 0xFF);
                    mptr[9] = (uint8_t)((var >> 8) & 0xFF);
                    mptr[10] = (uint8_t)((var >> 16) & 0xFF);
                    for ( k1 = k - 1; k1 >= 0; --k1 )
                    {
                        // if( mptr[k1].sortKey >= mptr[k1+1].sortKey )
                        //     break;
                        // std::swap( mptr[k1], mptr[k1+1] );
                        //
                        sortKey_k1 = pstModel[11 * k1] + (pstModel[11 * k1 + 1] << 8);
                        sortKey_k1p1 = pstModel[11 * k1 + 11] + (pstModel[11 * k1 + 12] << 8);
                        if ( sortKey_k1 > sortKey_k1p1 )
                            break;
                        memcpy(swapbuf, &pstModel[11 * k1], sizeof(swapbuf));
                        memcpy(&pstModel[11 * k1], &pstModel[11 * k1 + 11], 0xB);
                        memcpy(&pstModel[11 * k1 + 11], swapbuf, 0xB);
                    }
                    kHit = k1 + 1;
                    break;
                }
            }
            // k < u8ModelNum: break condition #1
            if ( kHit >= 0 )
            {
                while ( k < u8ModelNum )
                {
                    // wsum += mptr[k].weight;
                    w = pstModel[11 * k] + (pstModel[11 * k + 1] << 8);
                    wsum += w;
                    ++k;
                }
            }
            else
            {
                // kHit = k = std::min(k, K-1);
                // wsum += w0 - mptr[k].weight;
                // mptr[k].weight = w0;
                // mptr[k].mean = pix;
                // mptr[k].var = var0;
                // mptr[k].sortKey = sk0;
                //
                if ( k <= u8ModelNum - 1 )
                    min_k_Km1 = k;
                else
                    min_k_Km1 = u8ModelNum - 1;
                k = min_k_Km1;
                kHit = min_k_Km1;
                w = pstModel[11 * min_k_Km1] + (pstModel[11 * min_k_Km1 + 1] << 8);
                wsum += u0q16InitWeight - w;
                pstModel[11 * k] = u0q16InitWeight;
                pstModel[11 * k + 1] = (u0q16InitWeight >> 8);
                pstModel[11 * k + 3] = pix_c0;
                pstModel[11 * k + 2] = 0;
                pstModel[11 * k + 5] = pix_c1;
                pstModel[11 * k + 4] = 0;
                pstModel[11 * k + 7] = pix_c2;
                pstModel[11 * k + 6] = 0;
                var = 4 * u22q10NoiseVar;
                pstModel[11 * k + 10] = (uint8_t)((var >> 16) & 0xFF);
                pstModel[11 * k + 9]  = (uint8_t)((var >> 8) & 0xFF);
                pstModel[11 * k + 8]  = (uint8_t)(var & 0xFF);
            }
            One_div_wscale = wsum;
            wsum = 0;
            for ( k = 0; k < u8ModelNum; ++k )
            {
                // wsum += mptr[k].weight *= wscale;
                w = pstModel[11 * k] + (pstModel[11 * k + 1] << 8);
                w = 0xFFFF * (uint32_t)w / One_div_wscale;
                *(uint16_t *)&pstModel[11 * k] = w;
                wsum += w;
                if ( wsum > u0q16BgRatio && kForeground < 0 )
                    kForeground = k + 1;
            }
            if ( enDetectShadow )
            {
                // pstFg0[x] = (kHit < kForeground) - 1;
                // if ( (uint8_t)pstFg0[x] == 255
                //         && CVI_HW_DetectGMMShadow(
                //             (uint8_t *)&pstSrc0[3 * x],
                //             pstModel,
                //             u8ModelNum,
                //             1,
                //             u0q16BgRatio,
                //             u0q8ShadowThr,
                //             u8SnsFactor) )
                // {
                //     pstFg0[x] = 127;
                // }
            }
            else
            {
                pstFg0[x] = (kHit >= kForeground)? 0xFF : 0;
            }
            pstBg0[3 * x] = pstModel[3];
            pstBg0[3 * x + 1] = pstModel[5];
            pstBg0[3 * x + 2] = pstModel[7];
            pstModel += 11 * u8ModelNum;
        }
        pstSrc0 += 3 * Src_u16Stride;
        pstFg0 += Fg_u16Stride;
        pstBg0 += 3 * Bg_u16Stride;
    }
    return result;
}

int CVI_HW_GMM(
    uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
    uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor, uint8_t IsU8C3)
{
    int result;

    if ( IsU8C3 )
        result = CVI_HW_RGBGMM(
                     pstSrc,
                     Src_u16Stride,
                     Src_u16Width,
                     Src_u16Height,
                     pstModel,
                     u8ModelNum,
                     pstFg,
                     Fg_u16Stride,
                     pstBg,
                     Bg_u16Stride,
                     u0q16LearnRate,
                     u0q16BgRatio,
                     u8q8VarThr,
                     u22q10NoiseVar,
                     u22q10MaxVar,
                     u22q10MinVar,
                     u0q16InitWeight,
                     enDetectShadow,
                     u0q8ShadowThr,
                     u8SnsFactor);
    else
        result = CVI_HW_SingleGMM(
                     pstSrc,
                     Src_u16Stride,
                     Src_u16Width,
                     Src_u16Height,
                     pstModel,
                     u8ModelNum,
                     pstFg,
                     Fg_u16Stride,
                     pstBg,
                     Bg_u16Stride,
                     u0q16LearnRate,
                     u0q16BgRatio,
                     u8q8VarThr,
                     u22q10NoiseVar,
                     u22q10MaxVar,
                     u22q10MinVar,
                     u0q16InitWeight,
                     enDetectShadow,
                     u0q8ShadowThr,
                     u8SnsFactor);
    return result;
}

int CVI_MPI_IVE_GMM(uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
    uint8_t *pstFg, uint16_t Fg_u16Stride,
    uint8_t *pstBg, uint16_t Bg_u16Stride,
    uint8_t *pstModel, uint8_t u8ModelNum,
    uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
    uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
    uint16_t u0q16InitWeight, uint8_t IsU8C3)
{
    CVI_HW_GMM(
            pstSrc, Src_u16Stride,
            Src_u16Width, Src_u16Height,
            pstModel, u8ModelNum,
            pstFg, Fg_u16Stride,
            pstBg, Bg_u16Stride,
            u0q16LearnRate, u0q16BgRatio, u8q8VarThr,
            u22q10NoiseVar, u22q10MaxVar, u22q10MinVar,
            u0q16InitWeight,
            0, 0, 8, IsU8C3);
    return 0; // CVI_SUCCESS
}

// Note: CVI_HW_DetectGMMShadow is not defined in the provided code, so it's assumed to be implemented elsewhere.
// If needed, you should define or include its implementation.