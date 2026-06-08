// # CVI_HW_SingleGMM fixed-point calculation walkthrough in tpu_gmm.cpp

// **Session ID:** ses_1bc9680c4ffesBkKP7zyGJOWWt
// **Created:** 5/20/2026, 11:24:07 AM
// **Updated:** 5/20/2026, 2:31:52 PM

// ---

// ## User

// 解释/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/host/tpu_gmm.cpp中的CVI_HW_SingleGMM代码，说明每步的定点数计算

// ---

// ## Assistant (Build · Kimi-K2.6 · 4.4s)

// **Tool: read**

// **Input:**
// ```json
// {
//   "filePath": "/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/host/tpu_gmm.cpp"
// }
// ```

// **Output:**
// ```
// <path>/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/host/tpu_gmm.cpp</path>
// <type>file</type>
// <content>
// 1: #include <stdio.h>
// 2: #include <stdlib.h>
// 3: #include <string.h>
// 4: #include <math.h>
// 5: #include <float.h>
// 6: #include <stdint.h>
// 7: #include <stdbool.h>
// 8: #ifdef _WIN32
// 9: #include <windows.h>
// 10: #else
// 11: #include <sys/time.h>
// 12: #include <unistd.h>
// 13: #endif
// 14:
// 15: #include "tpu_api_protocol.h"
// 16: #include "bmlib_runtime.h"
// 17: #include "bmcv_api_ext.h"
// 18: #include "kernel_module_data.h"
// 19:
// 20: static bm_handle_t handle;
// 21:
// 22: typedef struct sg_api_cv_gmm {
// 23:     u64 src_addr;
// 24:     u64 fg_addr;
// 25:     u64 bg_addr;
// 26:     u64 model_addr;
// 27:     u32 width;
// 28:     u32 height;
// 29:     // u32 src_stride;
// 30:     // u32 fg_stride;
// 31:     // u32 bg_stride;
// 32:     // u32 model_size;
// 33:     u16 u0q16LearnRate;
// 34:     u16 u0q16BgRatio;
// 35:     u16 u8q8VarThr;
// 36:     u32 u22q10NoiseVar;
// 37:     u32 u22q10MaxVar;
// 38:     u32 u22q10MinVar;
// 39:     u16 u0q16InitWeight;
// 40:     u8  u8ModelNum;
// 41: } __attribute__((packed)) sg_api_cv_gmm_t;
// 42:
// 43: #ifdef _WIN32
// 44:     #include <direct.h>
// 45:     #define MKDIR(path) _mkdir(path)
// 46: #else
// 47:     #include <sys/stat.h>
// 48:     #include <sys/types.h>
// 49:     #define MKDIR(path) mkdir(path, 0777)
// 50: #endif
// 51:
// 52: enum {
// 53: 	CVGRAY_U8C1 = 0,
// 54: 	U8C1_CVGRAY = 3,
// 55: };
// 56:
// 57: #define MAX2(a, b)  (( (a) > (b) )? (a) : (b) )
// 58: #define MIN2(a, b)  (( (a) < (b) )? (a) : (b) )
// 59:
// 60: typedef struct _MEM_INFO_S {
// 61: 	uint8_t* pData;
// 62: 	uint32_t u32Size;
// 63: } MEM_INFO_S;
// 64:
// 65: typedef struct _GMM_CTRL_S {
// 66: 	uint16_t u0q16LearnRate;
// 67: 	uint16_t u0q16BgRatio;
// 68: 	uint16_t u8q8VarThr;
// 69: 	uint32_t u22q10NoiseVar;
// 70: 	uint32_t u22q10MaxVar;
// 71: 	uint32_t u22q10MinVar;
// 72: 	uint16_t u0q16InitWeight;
// 73: 	uint8_t  u8ModelNum;
// 74: } GMM_CTRL_S;
// 75:
// 76: typedef struct _IMAGE_S {
// 77: 	uint64_t  au64PhyAddr[3];
// 78: 	uint64_t  au64VirAddr[3];
// 79: 	uint32_t  au32Stride[3];
// 80: 	uint32_t  u32Width;
// 81: 	uint32_t  u32Height;
// 82: } IMAGE_S;
// 83:
// 84: bool directoryExists(const char* path) {
// 85:     #ifdef _WIN32
// 86:         struct _stat info;
// 87:         return (_stat(path, &info) == 0 && (info.st_mode & _S_IFDIR));
// 88:     #else
// 89:         struct stat info;
// 90:         return (stat(path, &info) == 0 && (info.st_mode & S_IFDIR));
// 91:     #endif
// 92: }
// 93:
// 94: void createDirectory(const char* path) {
// 95:     if (!directoryExists(path)) {
// 96:         MKDIR(path);
// 97:     }
// 98: }
// 99:
// 100: int CVI_HW_SingleGMM(
// 101:     uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
// 102:     uint8_t *pstModel, uint8_t u8ModelNum,
// 103:     uint8_t *pstFg, uint16_t Fg_u16Stride, uint8_t *pstBg, uint16_t Bg_u16Stride,
// 104:     uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
// 105:     uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar, uint16_t u0q16InitWeight,
// 106:     uint8_t enDetectShadow, uint8_t u0q8ShadowThr, uint8_t u8SnsFactor)
// 107: {
// 108:     int result = 0;
// 109:     int min_k_Km1;
// 110:     uint8_t swapbuf[7];
// 111:     uint32_t One_div_wscale, sortKey_k1p1, sortKey_k1;
// 112:     int32_t diff;
// 113:     int k1, k, kForeground, kHit;
// 114:     uint8_t pix;
// 115:     uint32_t dw, wsum;
// 116:     uint64_t dist2, var;
// 117:     uint16_t x, y, w, mu;
// 118:     uint8_t *pstBg0;
// 119:     uint8_t *mptr;
// 120:     uint8_t *pstFg0;
// 121:     uint8_t *pstSrc0;
// 122:
// 123:     pstSrc0 = pstSrc;
// 124:     pstFg0 = pstFg;
// 125:     pstBg0 = pstBg;
// 126:     for ( y = 0; y < Src_u16Height ; ++y )
// 127:     {
// 128:         result = y;
// 129:         for ( x = 0; x < Src_u16Width ; ++x )
// 130:         {
// 131:             kHit = -1;
// 132:             kForeground = -1;
// 133:             pix = pstSrc0[x];
// 134:             wsum = 0;
// 135:             mptr = pstModel;
// 136:             for ( k = 0; k < u8ModelNum; ++k )
// 137:             {
// 138:                 mptr = &pstModel[7 * k];
// 139:                 w = mptr[0] + (mptr[1] << 8);
// 140:                 wsum += w;
// 141:                 if ( !w )
// 142:                     break;
// 143:                 mu = mptr[2] + (mptr[3] << 8);
// 144:                 var = mptr[4] + (mptr[5] << 8) + (mptr[6] << 16);
// 145:                 diff = (pix << 8) - mu;
// 146:                 dist2 = (diff * diff + 32) >> 6;
// 147:                 if ( dist2 < (u8q8VarThr * var + 128) >> 8 )
// 148:                 {
// 149:                     wsum -= w;
// 150:                     dw = ((0xFFFF - w) * (uint32_t)u0q16LearnRate + 0x8000) >> 16;
// 151:                     w += dw;
// 152:                     *(uint16_t *)mptr = w;
// 153:                     mu += ((u0q16LearnRate * diff) >> 16);
// 154:                     *((uint16_t *)mptr + 1) = mu;
// 155:                     var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
// 156:                     var = MIN2(var, u22q10MaxVar);
// 157:                     var = MAX2(var, u22q10MinVar);
// 158:                     mptr[4] = (uint8_t)(var & 0xFF);
// 159:                     mptr[5] = (uint8_t)((var >> 8) & 0xFF);
// 160:                     mptr[6] = (uint8_t)((var >> 16) & 0xFF);
// 161:                     for ( k1 = k - 1; k1 >= 0; --k1 )
// 162:                     {
// 163:                         sortKey_k1   = pstModel[7 * k1] + (pstModel[7 * k1 + 1] << 8);
// 164:                         sortKey_k1p1 = pstModel[7 * k1 + 7] + (pstModel[7 * k1 + 8] << 8);
// 165:                         if ( sortKey_k1 > sortKey_k1p1 )
// 166:                             break;
// 167:                         memcpy(swapbuf, &pstModel[7 * k1], 7);
// 168:                         memcpy(&pstModel[7 * k1], &pstModel[7 * k1 + 7], 7);
// 169:                         memcpy(&pstModel[7 * k1 + 7], swapbuf, 7);
// 170:                     }
// 171:                     kHit = k1 + 1;
// 172:                     break;
// 173:                 }
// 174:             }
// 175:             if ( kHit >= 0 )
// 176:             {
// 177:                 while ( k < u8ModelNum )
// 178:                 {
// 179:                     w = pstModel[7 * k] + (pstModel[7 * k + 1] << 8);
// 180:                     wsum += w;
// 181:                     ++k;
// 182:                 }
// 183:             }
// 184:             else
// 185:             {
// 186:                 if ( k <= u8ModelNum - 1 )
// 187:                     min_k_Km1 = k;
// 188:                 else
// 189:                     min_k_Km1 = u8ModelNum - 1;
// 190:                 k = min_k_Km1;
// 191:                 kHit = min_k_Km1;
// 192:                 w = pstModel[7 * min_k_Km1] + (pstModel[7 * min_k_Km1 + 1] << 8);
// 193:                 wsum += u0q16InitWeight - w;
// 194:                 pstModel[7 * k] = u0q16InitWeight;
// 195:                 pstModel[7 * k + 1] = (u0q16InitWeight >> 8);
// 196:                 pstModel[7 * k + 3] = pix;
// 197:                 pstModel[7 * k + 2] = 0;
// 198:                 var = 4 * u22q10NoiseVar;
// 199:                 pstModel[7 * k + 6] = (uint8_t)((var >> 16) & 0xFF);
// 200:                 pstModel[7 * k + 5]  = (uint8_t)((var >> 8) & 0xFF);
// 201:                 pstModel[7 * k + 4]  = (uint8_t)(var & 0xFF);
// 202:             }
// 203:             One_div_wscale = wsum;
// 204:             wsum = 0;
// 205:             for ( k = 0; k < u8ModelNum; ++k )
// 206:             {
// 207:                 w = pstModel[7 * k] + (pstModel[7 * k + 1] << 8);
// 208:                 w = 0xFFFF * (uint32_t)w / One_div_wscale;
// 209:                 *(uint16_t *)&pstModel[7 * k] = w;
// 210:                 wsum += w;
// 211:                 if ( wsum > u0q16BgRatio && kForeground < 0 )
// 212:                     kForeground = k + 1;
// 213:             }
// 214:             if ( enDetectShadow )
// 215:             {
// 216:             }
// 217:             else
// 218:             {
// 219:                 pstFg0[x] = (kHit >= kForeground)? 0xFF : 0;
// 220:             }
// 221:             pstBg0[x] = pstModel[3];
// 222:             pstModel += 7 * u8ModelNum;
// 223:         }
// 224:         pstSrc0 += Src_u16Stride;
// 225:         pstFg0 += Fg_u16Stride;
// 226:         pstBg0 += Bg_u16Stride;
// 227:     }
// 228:     return result;
// 229: }
// 230:
// 231: int32_t CVI_MPI_IVE_GMM(uint8_t *pstSrc, uint16_t Src_u16Stride, uint16_t Src_u16Width, uint16_t Src_u16Height,
// 232:     uint8_t *pstFg, uint16_t Fg_u16Stride,
// 233:     uint8_t *pstBg, uint16_t Bg_u16Stride,
// 234:     uint8_t *pstModel, uint8_t u8ModelNum,
// 235:     uint16_t u0q16LearnRate, uint16_t u0q16BgRatio, uint16_t u8q8VarThr,
// 236:     uint32_t u22q10NoiseVar, uint32_t u22q10MaxVar, uint32_t u22q10MinVar,
// 237:     uint16_t u0q16InitWeight)
// 238: {
// 239:     CVI_HW_SingleGMM(
// 240:             pstSrc, Src_u16Stride,
// 241:             Src_u16Width, Src_u16Height,
// 242:             pstModel, u8ModelNum,
// 243:             pstFg, Fg_u16Stride,
// 244:             pstBg, Bg_u16Stride,
// 245:             u0q16LearnRate, u0q16BgRatio, u8q8VarThr,
// 246:             u22q10NoiseVar, u22q10MaxVar, u22q10MinVar,
// 247:             u0q16InitWeight,
// 248:             0, 0, 8);
// 249:     return 0;
// 250: }
// 251:
// 252: static int32_t CreateImage(IMAGE_S* pstImg, uint32_t u32Width, uint32_t u32Height)
// 253: {
// 254: 	uint32_t u32Stride = u32Width;
// 255: 	pstImg->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height);
// 256: 	if (!pstImg->au64VirAddr[0]) return -1;
// 257: 	memset((void*)pstImg->au64VirAddr[0], 0, u32Stride * u32Height);
// 258: 	pstImg->au64PhyAddr[0] = 0;
// 259: 	pstImg->au32Stride[0] = u32Stride;
// 260: 	pstImg->u32Width = u32Width;
// 261: 	pstImg->u32Height = u32Height;
// 262: 	pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[2] = 0;
// 263: 	pstImg->au64VirAddr[1] = pstImg->au64VirAddr[2] = 0;
// 264: 	pstImg->au32Stride[1] = pstImg->au32Stride[2] = 0;
// 265: 	return 0;
// 266: }
// 267:
// 268: static void DestroyImage(IMAGE_S* pstImg)
// 269: {
// 270: 	if (pstImg->au64VirAddr[0]) {
// 271: 		free((void*)pstImg->au64VirAddr[0]);
// 272: 		pstImg->au64VirAddr[0] = 0;
// 273: 	}
// 274: }
// 275:
// 276: static int32_t CreateMem(MEM_INFO_S* pstMem, uint32_t u32Size)
// 277: {
// 278: 	pstMem->pData = (uint8_t*)malloc(u32Size);
// 279: 	if (!pstMem->pData) return -1;
// 280: 	memset(pstMem->pData, 0, u32Size);
// 281: 	pstMem->u32Size = u32Size;
// 282: 	return 0;
// 283: }
// 284:
// 285: static void DestroyMem(MEM_INFO_S* pstMem)
// 286: {
// 287: 	if (pstMem->pData) {
// 288: 		free(pstMem->pData);
// 289: 		pstMem->pData = NULL;
// 290: 	}
// 291: }
// 292:
// 293: static int32_t CVI_GMM(IMAGE_S *pstSrc, IMAGE_S *pstFg,
// 294: 	IMAGE_S *pstBg, MEM_INFO_S *pstModel, GMM_CTRL_S *pstCtrl)
// 295: {
// 296: 	return CVI_MPI_IVE_GMM(
// 297: 			(uint8_t*)pstSrc->au64VirAddr[0], (uint16_t)pstSrc->au32Stride[0],
// 298: 			(uint16_t)pstSrc->u32Width, (uint16_t)pstSrc->u32Height,
// 299: 			(uint8_t*)pstFg->au64VirAddr[0], (uint16_t)pstFg->au32Stride[0],
// 300: 			(uint8_t*)pstBg->au64VirAddr[0], (uint16_t)pstBg->au32Stride[0],
// 301: 			pstModel->pData, pstCtrl->u8ModelNum,
// 302: 			pstCtrl->u0q16LearnRate, pstCtrl->u0q16BgRatio, pstCtrl->u8q8VarThr,
// 303: 			pstCtrl->u22q10NoiseVar, pstCtrl->u22q10MaxVar, pstCtrl->u22q10MinVar,
// 304: 			pstCtrl->u0q16InitWeight);
// 305: }
// 306:
// 307: // Helper function: write raw RGB image data
// 308: static void writeRaw(const char* filename, uint8_t* data, int width, int height) {
// 309:     FILE* fp = fopen(filename, "wb");
// 310:     if (!fp) return;
// 311:     fwrite(data, 1, width * height * 3, fp);
// 312:     fclose(fp);
// 313: }
// 314:
// 315: // Helper function: convert BGR to grayscale
// 316: static void bgrToGray(uint8_t* bgr, uint8_t* gray, int width, int height) {
// 317:     for (int i = 0; i < width * height; i++) {
// 318:         uint8_t b = bgr[i * 3];
// 319:         uint8_t g = bgr[i * 3 + 1];
// 320:         uint8_t r = bgr[i * 3 + 2];
// 321:         // Standard grayscale conversion
// 322:         gray[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
// 323:     }
// 324: }
// 325:
// 326: // Helper function: convert RGB to grayscale
// 327: static void rgbToGray(uint8_t* rgb, uint8_t* gray, int width, int height) {
// 328:     for (int i = 0; i < width * height; i++) {
// 329:         uint8_t r = rgb[i * 3];
// 330:         uint8_t g = rgb[i * 3 + 1];
// 331:         uint8_t b = rgb[i * 3 + 2];
// 332:         // Standard grayscale conversion
// 333:         gray[i] = (uint8_t)(0.299f * r + 0.587f * g + 0.114f * b);
// 334:     }
// 335: }
// 336:
// 337: // Helper function: convert RGB to BGR
// 338: static void rgbToBgr(uint8_t* rgb, uint8_t* bgr, int width, int height) {
// 339:     for (int i = 0; i < width * height; i++) {
// 340:         bgr[i * 3] = rgb[i * 3 + 2];     // B
// 341:         bgr[i * 3 + 1] = rgb[i * 3 + 1]; // G
// 342:         bgr[i * 3 + 2] = rgb[i * 3];     // R
// 343:     }
// 344: }
// 345:
// 346: // Helper function: convert grayscale to RGB
// 347: static void grayToRgb(uint8_t* gray, uint8_t* rgb, int width, int height) {
// 348:     for (int i = 0; i < width * height; i++) {
// 349:         rgb[i * 3] = gray[i];
// 350:         rgb[i * 3 + 1] = gray[i];
// 351:         rgb[i * 3 + 2] = gray[i];
// 352:     }
// 353: }
// 354:
// 355: // Helper function: convert BGR to RGB
// 356: static void bgrToRgb(uint8_t* bgr, uint8_t* rgb, int width, int height) {
// 357:     for (int i = 0; i < width * height; i++) {
// 358:         rgb[i * 3] = bgr[i * 3 + 2];     // R
// 359:         rgb[i * 3 + 1] = bgr[i * 3 + 1]; // G
// 360:         rgb[i * 3 + 2] = bgr[i * 3];     // B
// 361:     }
// 362: }
// 363:
// 364: // Helper function: horizontal concatenate 3 images
// 365: static void hconcat3(uint8_t* src1, uint8_t* src2, uint8_t* src3, uint8_t* dst, int width, int height) {
// 366:     for (int y = 0; y < height; y++) {
// 367:         // Copy src1 row
// 368:         memcpy(dst + y * width * 3 * 3, src1 + y * width * 3, width * 3);
// 369:         // Copy src2 row
// 370:         memcpy(dst + y * width * 3 * 3 + width * 3, src2 + y * width * 3, width * 3);
// 371:         // Copy src3 row
// 372:         memcpy(dst + y * width * 3 * 3 + width * 6, src3 + y * width * 3, width * 3);
// 373:     }
// 374: }
// 375:
// 376: static bm_status_t check_gmm(
// 377:     bm_handle_t handle,
// 378:     int width,
// 379:     int height,
// 380:     int model_num) {
// 381:     if (handle == NULL) {
// 382:         bmlib_log("gmm", BMLIB_LOG_ERROR, "Can not get handle!\r\n");
// 383:         return BM_ERR_FAILURE;
// 384:     }
// 385:     if (width <= 0 || height <= 0) {
// 386:         bmlib_log("gmm", BMLIB_LOG_ERROR, "Invalid width or height!\r\n");
// 387:         return BM_ERR_FAILURE;
// 388:     }
// 389:     if (model_num <= 0 || model_num > 8) {
// 390:         bmlib_log("gmm", BMLIB_LOG_ERROR, "ModelNum must be between 1 and 8!\r\n");
// 391:         return BM_ERR_FAILURE;
// 392:     }
// 393:     return BM_SUCCESS;
// 394: }
// 395:
// 396: bm_status_t bmcv_image_gmm(
// 397:     bm_handle_t            handle,
// 398:     bm_image               input,
// 399:     bm_image               output_fg,
// 400:     bm_image               output_bg,
// 401:     bm_device_mem_t        output_model,
// 402:     GMM_CTRL_S             gmm_attr) {
// 403:     // bm_status_t ret = check_gmm(handle, width, height, model_num);
// 404:     // if (ret != BM_SUCCESS)
// 405:     //     return ret;
// 406:     bm_status_t ret = BM_SUCCESS;
// 407:
// 408:     tpu_kernel_module_t tpu_module = NULL;
// 409:     tpu_kernel_function_t func_id = 0;
// 410:     MEM_INFO_S* persistent_models = NULL;
// 411:     int width = output_fg.width;
// 412:     int height = output_fg.height;
// 413:
// 414:     const unsigned int *p = kernel_module_data;
// 415:     size_t length = sizeof(kernel_module_data);
// 416:     tpu_module = tpu_kernel_load_module(handle, (const char *)p, length);
// 417:     func_id = tpu_kernel_get_function(handle, tpu_module, "tpu_kernel_api_gmm");
// 418:     printf("gmm func_id = %d\n", func_id);
// 419:
// 420:     sg_api_cv_gmm_t api = {0};
// 421:
// 422:     int stride_src[3], stride_fg[3], stride_bg[3];
// 423:     bm_image_get_stride(input, stride_src);
// 424:     bm_image_get_stride(output_fg, stride_fg);
// 425:     bm_image_get_stride(output_bg, stride_bg);
// 426:     bm_device_mem_t input_mem[3], output_fg_mem[3], output_bg_mem[3];
// 427:     bm_image_get_device_mem(input, input_mem);
// 428:     bm_image_get_device_mem(output_fg, output_fg_mem);
// 429:     bm_image_get_device_mem(output_bg, output_bg_mem);
// 430:
// 431:     api.src_addr = bm_mem_get_device_addr(input_mem[0]);
// 432:     api.fg_addr = bm_mem_get_device_addr(output_fg_mem[0]);
// 433:     api.bg_addr = bm_mem_get_device_addr(output_bg_mem[0]);
// 434:     api.model_addr = bm_mem_get_device_addr(output_model);
// 435:
// 436:     api.width = width;
// 437:     api.height = height;
// 438:     api.u0q16LearnRate = gmm_attr.u0q16LearnRate;
// 439:     api.u0q16BgRatio = gmm_attr.u0q16BgRatio;
// 440:     api.u8q8VarThr = gmm_attr.u8q8VarThr;
// 441:     api.u22q10NoiseVar = gmm_attr.u22q10NoiseVar;
// 442:     api.u22q10MaxVar = gmm_attr.u22q10MaxVar;
// 443:     api.u22q10MinVar = gmm_attr.u22q10MinVar;
// 444:     api.u0q16InitWeight = gmm_attr.u0q16InitWeight;
// 445:     api.u8ModelNum = gmm_attr.u8ModelNum;
// 446:
// 447:     uint8_t* fg_out_tpu = NULL;
// 448:     uint8_t* bg_out_tpu = NULL;
// 449:
// 450:     printf("sizeof api = %ld\n", sizeof(api));
// 451:     ret = tpu_kernel_launch(handle, func_id, &api, sizeof(api));
// 452:     if (ret != BM_SUCCESS) {
// 453:         printf("tpu_kernel_launch failed\n");
// 454:         goto exit0;
// 455:     }
// 456:
// 457:     fg_out_tpu = (uint8_t*)malloc(height * width);
// 458:     bg_out_tpu = (uint8_t*)malloc(height * width);
// 459:     if (!fg_out_tpu || !bg_out_tpu) {
// 460:         ret = BM_ERR_NOMEM;
// 461:         goto exit4;
// 462:     }
// 463:
// 464:     ret = bm_memcpy_d2s(handle, fg_out_tpu, output_fg_mem[0]);
// 465:     if (ret != BM_SUCCESS) goto exit4;
// 466:
// 467:     ret = bm_memcpy_d2s(handle, bg_out_tpu, output_bg_mem[0]);
// 468:     if (ret != BM_SUCCESS) goto exit4;
// 469:
// 470:
// 471:
// 472: exit4:
// 473:     free(fg_out_tpu);
// 474:     free(bg_out_tpu);
// 475: // exit3:
// 476: //     bm_free_device(handle, model_mem);
// 477: // exit2:
// 478: //     bm_free_device(handle, bg_mem);
// 479: // exit1:
// 480: //     bm_free_device(handle, fg_mem);
// 481: exit0:
// 482: //     bm_image_destroy(stImg);
// 483: //     free(grayImg);
// 484:
// 485:     return ret;
// 486: }
// 487:
// 488: static int gmm_tpu(
// 489:     int show,
// 490:     int compare,
// 491:     char* raw_path,
// 492:     char* output_path
// 493: ) {
// 494:     int32_t s32Ret = 0;
// 495:     const char *pchRaw = raw_path ? raw_path : "/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/data/result/raw_frames.raw";
// 496:     const char *pchRes = output_path ? output_path : "/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/data/result/frame";
// 497:
// 498:     uint16_t u16Width, u16Height;
// 499:     int32_t s32FrmCnt = 0;
// 500:     const int32_t totalFrames = 35;
// 501:     GMM_CTRL_S gmmAttr;
// 502:     bm_image src;
// 503:     bm_image dst_fg, dst_bg;
// 504:     bm_device_mem_t dst_model;
// 505:     int stride[4];
// 506:     int dst_stride[4];
// 507:     unsigned long long time_single, time_total = 0, time_avg = 0;
// 508:     unsigned long long time_max = 0, time_min = 10000;
// 509:     struct timeval tv_start;
// 510:     struct timeval tv_end;
// 511:     struct timeval timediff;
// 512:
// 513:     // For raw video input, we need to know dimensions
// 514:     // Using fixed dimensions for raw_frames.raw (352x288)
// 515:     u16Width = 352;
// 516:     u16Height = 288;
// 517:
// 518:     gmmAttr.u0q16BgRatio = 45875;
// 519:     gmmAttr.u0q16InitWeight = 3277;
// 520:     gmmAttr.u22q10NoiseVar = 225 * 1024;
// 521:     gmmAttr.u22q10MaxVar = 2000 * 1024;
// 522:     gmmAttr.u22q10MinVar = 200 * 1024;
// 523:     gmmAttr.u8q8VarThr = (uint16_t)(256 * 6.25);
// 524:     gmmAttr.u8ModelNum = 3;
// 525:
// 526:     int data_size = 1; // gray
// 527:     int model_len = u16Width * u16Height * data_size * gmmAttr.u8ModelNum * 8;
// 528:
// 529:     uint8_t *cvImg = NULL;
// 530:     uint8_t *stImg = NULL;
// 531:     uint8_t *cvFg = NULL;
// 532:     uint8_t *cvBg = NULL;
// 533:     uint8_t *cvFgBGR = NULL;
// 534:     uint8_t *cvBgBGR = NULL;
// 535:     uint8_t *cvDispImg = NULL;
// 536:     uint8_t *model_data = NULL;
// 537:
// 538:     FILE* fpRaw = NULL;
// 539:
// 540:     // Allocate buffers
// 541:     int frameSize = u16Width * u16Height * 3; // RGB
// 542:     cvImg = (uint8_t*)malloc(frameSize);
// 543:     cvFg = (uint8_t*)malloc(u16Width * u16Height);
// 544:     cvBg = (uint8_t*)malloc(u16Width * u16Height);
// 545:     cvFgBGR = (uint8_t*)malloc(frameSize);
// 546:     cvBgBGR = (uint8_t*)malloc(frameSize);
// 547:     cvDispImg = (uint8_t*)malloc(frameSize * 3); // 3 images side by side
// 548:     model_data = (uint8_t*)malloc(model_len);
// 549:     memset(model_data, 0, model_len * sizeof(unsigned char));
// 550:
// 551:     if (!cvImg || !cvFg || !cvBg || !cvFgBGR || !cvBgBGR || !cvDispImg || !model_data) {
// 552:         printf("Failed to allocate memory\n");
// 553:         s32Ret = -1;
// 554:         goto FAIL_TPU_0;
// 555:     }
// 556:
// 557:     memset(model_data, 0, model_len * sizeof(unsigned char));
// 558:
// 559:     // calc ive image stride && create bm image struct
// 560:     s32Ret = bm_image_create(handle, u16Height, u16Width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &src, NULL);
// 561:     if (s32Ret != BM_SUCCESS) {
// 562:         printf("bm_image_create src failed. ret = %d\n", s32Ret);
// 563:         goto FAIL_TPU_0;
// 564:     }
// 565:     s32Ret = bm_image_create(handle, u16Height, u16Width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_fg, NULL);
// 566:     if (s32Ret != BM_SUCCESS) {
// 567:         printf("bm_image_create dst_fg failed. ret = %d\n", s32Ret);
// 568:         goto FAIL_TPU_1;
// 569:     }
// 570:     s32Ret = bm_image_create(handle, u16Height, u16Width, FORMAT_GRAY, DATA_TYPE_EXT_1N_BYTE, &dst_bg, NULL);
// 571:     if (s32Ret != BM_SUCCESS) {
// 572:         printf("bm_image_create dst_bg failed. ret = %d\n", s32Ret);
// 573:         goto FAIL_TPU_2;
// 574:     }
// 575:
// 576:     s32Ret = bm_image_alloc_dev_mem(src, BMCV_HEAP1_ID);
// 577:     if (s32Ret != BM_SUCCESS) {
// 578:         printf("bm_image_alloc_dev_mem src failed. ret = %d\n", s32Ret);
// 579:         goto FAIL_TPU_3;
// 580:     }
// 581:     s32Ret = bm_image_alloc_dev_mem(dst_fg, BMCV_HEAP1_ID);
// 582:     if (s32Ret != BM_SUCCESS) {
// 583:         printf("bm_image_alloc_dev_mem dst_fg failed. ret = %d\n", s32Ret);
// 584:         goto FAIL_TPU_4;
// 585:     }
// 586:     s32Ret = bm_image_alloc_dev_mem(dst_bg, BMCV_HEAP1_ID);
// 587:     if (s32Ret != BM_SUCCESS) {
// 588:         printf("bm_image_alloc_dev_mem dst_bg failed. ret = %d\n", s32Ret);
// 589:         goto FAIL_TPU_5;
// 590:     }
// 591:     s32Ret = bm_malloc_device_byte(handle, &dst_model, model_len);
// 592:     if (s32Ret != BM_SUCCESS) {
// 593:         printf("bm_malloc_device_byte failed. ret = %d\n", s32Ret);
// 594:         goto FAIL_TPU_6;
// 595:     }
// 596:
// 597:     s32Ret = bm_memcpy_s2d(handle, dst_model, model_data);
// 598:     if (s32Ret != BM_SUCCESS) {
// 599:         printf("bm_memcpy_s2d failed. ret = %d\n", s32Ret);
// 600:         goto FAIL_TPU_7;
// 601:     }
// 602:     // Open raw file for reading
// 603:     fpRaw = fopen(pchRaw, "rb");
// 604:     if (!fpRaw) {
// 605:         printf("Failed to open raw file: %s\n", pchRaw);
// 606:         s32Ret = -1;
// 607:         goto FAIL_TPU_7;
// 608:     }
// 609:
// 610:     for (s32FrmCnt = 0; s32FrmCnt < totalFrames; s32FrmCnt++) {
// 611:         size_t readBytes = fread(cvImg, 1, frameSize, fpRaw);
// 612:         if (readBytes != (size_t)frameSize) {
// 613:             printf("Warning: failed to read frame %d, got %zu bytes\n", s32FrmCnt, readBytes);
// 614:             break;
// 615:         }
// 616:
// 617:         // Copy input data to device src image
// 618:         s32Ret = bm_image_copy_host_to_device(src, (void**)&cvImg);
// 619:         if (s32Ret != BM_SUCCESS) {
// 620:             printf("bm_image_copy_host_to_device failed. ret = %d\n", s32Ret);
// 621:             goto FAIL_TPU_7;
// 622:         }
// 623:
// 624:         if (s32FrmCnt >= 500)
// 625:             gmmAttr.u0q16LearnRate = 131;  //0.02
// 626:         else
// 627:             gmmAttr.u0q16LearnRate = 65535 / (s32FrmCnt + 1);
// 628:
// 629:         gettimeofday(&tv_start, NULL);
// 630:         s32Ret = bmcv_image_gmm(handle, src, dst_fg, dst_bg, dst_model, gmmAttr);
// 631:         gettimeofday(&tv_end, NULL);
// 632:         timediff.tv_sec  = tv_end.tv_sec - tv_start.tv_sec;
// 633:         timediff.tv_usec = tv_end.tv_usec - tv_start.tv_usec;
// 634:         time_single = (unsigned int)(timediff.tv_sec * 1000000 + timediff.tv_usec);
// 635:
// 636:         if (time_single > time_max)
// 637:             time_max = time_single;
// 638:         if (time_single < time_min)
// 639:             time_min = time_single;
// 640:         time_total = time_total + time_single;
// 641:
// 642:         if (s32Ret != BM_SUCCESS) {
// 643:             printf("bmcv_ive_gmm execution failed\n");
// 644:             goto FAIL_TPU_7;
// 645:         }
// 646:
// 647:         // Copy results from device to host
// 648:         s32Ret = bm_image_copy_device_to_host(dst_fg, (void**)&cvFg);
// 649:         if (s32Ret != BM_SUCCESS) {
// 650:             printf("bm_image_copy_device_to_host dst_fg failed. ret = %d\n", s32Ret);
// 651:             goto FAIL_TPU_7;
// 652:         }
// 653:         s32Ret = bm_image_copy_device_to_host(dst_bg, (void**)&cvBg);
// 654:         if (s32Ret != BM_SUCCESS) {
// 655:             printf("bm_image_copy_device_to_host dst_bg failed. ret = %d\n", s32Ret);
// 656:             goto FAIL_TPU_7;
// 657:         }
// 658:
// 659:         if (show) {
// 660:             // Convert grayscale results to RGB
// 661:             grayToRgb(cvBg, cvBgBGR, u16Width, u16Height);
// 662:             grayToRgb(cvFg, cvFgBGR, u16Width, u16Height);
// 663:
// 664:             // Convert RGB to BGR for the source image in the concatenated display
// 665:             uint8_t* bgrBuf = (uint8_t*)malloc(frameSize);
// 666:             rgbToBgr(cvImg, bgrBuf, u16Width, u16Height);
// 667:
// 668:             // Create display image: src + bg + fg side by side
// 669:             hconcat3(bgrBuf, cvBgBGR, cvFgBGR, cvDispImg, u16Width, u16Height);
// 670:
// 671:             free(bgrBuf);
// 672:
// 673:             // Save as raw RGB data
// 674:             char filename[512];
// 675:             snprintf(filename, sizeof(filename), "%s/frameTPU_%06d.raw", pchRes, s32FrmCnt);
// 676:             createDirectory(pchRes);
// 677:             writeRaw(filename, cvDispImg, u16Width * 3, u16Height);
// 678:             printf("Saved frame %d to %s\n", s32FrmCnt, filename);
// 679:         }
// 680:     }
// 681:
// 682:     time_avg = time_total / totalFrames;
// 683:     printf("bmcv_ive_gmm: totalFrames is %d, size %d x %d \n",
// 684:            totalFrames, u16Width, u16Height);
// 685:     printf("bmcv_ive_gmm: time_max = %llu, time_avg = %llu\n",
// 686:            time_max, time_avg);
// 687:     printf("bmcv ive gmm test successful \n");
// 688:
// 689:     if (fpRaw) fclose(fpRaw);
// 690:
// 691: FAIL_TPU_7:
// 692:     bm_free_device(handle, dst_model);
// 693: FAIL_TPU_6:
// 694:     bm_image_destroy(dst_bg);
// 695: FAIL_TPU_5:
// 696:     bm_image_destroy(dst_fg);
// 697: FAIL_TPU_4:
// 698:     bm_image_destroy(src);
// 699: FAIL_TPU_3:
// 700: FAIL_TPU_2:
// 701: FAIL_TPU_1:
// 702: FAIL_TPU_0:
// 703:     if (model_data) free(model_data);
// 704:     if (cvDispImg) free(cvDispImg);
// 705:     if (cvBgBGR) free(cvBgBGR);
// 706:     if (cvFgBGR) free(cvFgBGR);
// 707:     if (cvBg) free(cvBg);
// 708:     if (cvFg) free(cvFg);
// 709:     if (cvImg) free(cvImg);
// 710:     return s32Ret;
// 711: }
// 712:
// 713: static int gmm_cpu(
// 714:     int show,
// 715:     int compare,
// 716:     char* raw_path,
// 717:     char* output_path
// 718: )
// 719: {
// 720: 	int32_t  s32Ret = 0;
// 721: 	int32_t s32CompareError = 0;
// 722: 	const char *pchRaw = raw_path ? raw_path : "/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/data/result/raw_frames.raw";
// 723: 	const char *pchRes = output_path ? output_path : "/home/zyz/workspace/TPU1686/tpu-kernel-bm1684x_v3.1.7-30101a21-260129/samples/data/result/frame";
// 724:
// 725: 	IMAGE_S stImg, stFg, stBg;
// 726: 	MEM_INFO_S stModel;
// 727: 	GMM_CTRL_S stCtrl;
// 728: 	uint16_t u16Width, u16Height;
// 729: 	int32_t s32FrmCnt = 0;
// 730: 	const int32_t totalFrames = 35;
// 731:
// 732:     uint8_t *cvImg = NULL;
// 733:     uint8_t *cvFg = NULL;
// 734:     uint8_t *cvBg = NULL;
// 735:     uint8_t *cvFgBGR = NULL;
// 736:     uint8_t *cvBgBGR = NULL;
// 737:     uint8_t *cvDispImg = NULL;
// 738:
// 739:     FILE* fpRaw = NULL;
// 740:
// 741:     // For raw video input, we need to know dimensions
// 742:     u16Width = 352;
// 743:     u16Height = 288;
// 744:
// 745: 	stCtrl.u0q16BgRatio		= 45875;
// 746: 	stCtrl.u0q16InitWeight	= 3277;
// 747: 	stCtrl.u22q10NoiseVar	= 225 * 1024;
// 748: 	stCtrl.u22q10MaxVar		= 2000 * 1024;
// 749: 	stCtrl.u22q10MinVar		= 200 * 1024;
// 750: 	stCtrl.u8q8VarThr		= (uint16_t)(256 * 6.25);
// 751: 	stCtrl.u8ModelNum		= 3;
// 752:
// 753: 	if (CreateImage(&stImg, u16Width, u16Height) != 0)goto FAIL_U8C1_0;
// 754: 	if (CreateImage(&stBg, u16Width, u16Height) != 0)goto FAIL_U8C1_1;
// 755: 	if (CreateImage(&stFg, u16Width, u16Height) != 0)goto FAIL_U8C1_2;
// 756: 	if (CreateMem(&stModel, stCtrl.u8ModelNum * 7 * u16Width * u16Height) != 0) goto FAIL_U8C1_3;
// 757:
// 758:     // Allocate buffers
// 759:     int frameSize;
// 760:     frameSize = u16Width * u16Height * 3; // RGB
// 761:     cvImg = (uint8_t*)malloc(frameSize);
// 762:     cvFg = (uint8_t*)malloc(u16Width * u16Height);
// 763:     cvBg = (uint8_t*)malloc(u16Width * u16Height);
// 764:     cvFgBGR = (uint8_t*)malloc(frameSize);
// 765:     cvBgBGR = (uint8_t*)malloc(frameSize);
// 766:     cvDispImg = (uint8_t*)malloc(frameSize * 3); // 3 images side by side
// 767:
// 768:     if (!cvImg || !cvFg || !cvBg || !cvFgBGR || !cvBgBGR || !cvDispImg) {
// 769:         printf("Failed to allocate memory\n");
// 770:         s32Ret = -1;
// 771:         goto FAIL_U8C1_4;
// 772:     }
// 773:
// 774:     // Open raw file for reading
// 775:     fpRaw = fopen(pchRaw, "rb");
// 776:     if (!fpRaw) {
// 777:         printf("Failed to open raw file: %s\n", pchRaw);
// 778:         s32Ret = -1;
// 779:         goto FAIL_U8C1_4;
// 780:     }
// 781:
// 782:     for (s32FrmCnt = 0; s32FrmCnt < totalFrames; s32FrmCnt++) {
// 783:         size_t readBytes = fread(cvImg, 1, frameSize, fpRaw);
// 784:         if (readBytes != (size_t)frameSize) {
// 785:             printf("Warning: failed to read frame %d, got %zu bytes\n", s32FrmCnt, readBytes);
// 786:             break;
// 787:         }
// 788:
// 789:         // Convert RGB to grayscale for GMM input
// 790:         rgbToGray(cvImg, (uint8_t*)stImg.au64VirAddr[0], u16Width, u16Height);
// 791:
// 792: 		stCtrl.u0q16LearnRate = (s32FrmCnt >= 500) ? 131 : (65535/(s32FrmCnt+1));
// 793:
// 794: 		s32Ret = CVI_GMM(&stImg, &stFg, &stBg, &stModel, &stCtrl);
// 795: 		if (s32Ret != 0) goto FAIL_U8C1_5;
// 796:
// 797: 		if (show) {
// 798:             // Convert grayscale results to RGB
// 799:             grayToRgb((uint8_t*)stBg.au64VirAddr[0], cvBgBGR, u16Width, u16Height);
// 800:             grayToRgb((uint8_t*)stFg.au64VirAddr[0], cvFgBGR, u16Width, u16Height);
// 801:
// 802:             // Convert RGB to BGR for the source image in the concatenated display
// 803:             uint8_t* bgrBuf = (uint8_t*)malloc(frameSize);
// 804:             rgbToBgr(cvImg, bgrBuf, u16Width, u16Height);
// 805:
// 806:             // Create display image: src + bg + fg side by side
// 807:             hconcat3(bgrBuf, cvBgBGR, cvFgBGR, cvDispImg, u16Width, u16Height);
// 808:
// 809:             free(bgrBuf);
// 810:
// 811:             // Save as raw RGB data
// 812:             char filename[512];
// 813:             snprintf(filename, sizeof(filename), "%s/frameCPU_%06d.raw", pchRes, s32FrmCnt);
// 814:             createDirectory(pchRes);
// 815:             writeRaw(filename, cvDispImg, u16Width * 3, u16Height);
// 816:             printf("Saved frame %d to %s\n", s32FrmCnt, filename);
// 817:         }
// 818: 	}
// 819:
// 820: FAIL_U8C1_5:
// 821: FAIL_U8C1_4:
// 822:     if (cvDispImg) free(cvDispImg);
// 823:     if (cvBgBGR) free(cvBgBGR);
// 824:     if (cvFgBGR) free(cvFgBGR);
// 825:     if (cvBg) free(cvBg);
// 826:     if (cvFg) free(cvFg);
// 827:     if (cvImg) free(cvImg);
// 828:     if (fpRaw) fclose(fpRaw);
// 829:     DestroyMem(&stModel);
// 830: FAIL_U8C1_3:
// 831:     DestroyImage(&stFg);
// 832: FAIL_U8C1_2:
// 833:     DestroyImage(&stBg);
// 834: FAIL_U8C1_1:
// 835:     DestroyImage(&stImg);
// 836: FAIL_U8C1_0:
// 837:     return (s32CompareError == 0) ? s32Ret : -1;
// 838: }
// 839:
// 840: int main(int argc, char* argv[])
// 841: {
// 842:     int loop = 1;
// 843:     int use_tpu = 0;
// 844:     int show = 1;
// 845:     int compare = 1;
// 846:     char *raw_path = NULL;
// 847:     char *output_path = NULL;
// 848:
// 849:     if (argc > 1 && atoi(argv[1]) == -1) {
// 850:         printf("usage: \n");
// 851:         printf("%s loop use_tpu show compare raw_path output_path\n", argv[0]);
// 852:         printf("example: \n");
// 853:         printf("%s \n", argv[0]);
// 854:         printf("%s 2 1 1 1 \n", argv[0]);
// 855:         return 0;
// 856:     }
// 857:
// 858:     if(argc > 1) loop = atoi(argv[1]);
// 859:     if(argc > 2) use_tpu = atoi(argv[2]);
// 860:     if(argc > 3) show = atoi(argv[3]);
// 861:     if(argc > 4) compare = atoi(argv[4]);
// 862:     if(argc > 5) raw_path = argv[5];
// 863:     if(argc > 6) output_path = argv[6];
// 864:
// 865:     int dev_id = 0;
// 866:     bm_status_t status = bm_dev_request(&handle, dev_id);
// 867:     if (status != BM_SUCCESS) {
// 868:         printf("Create bm handle failed. ret = %d\n", status);
// 869:         exit(-1);
// 870:     }
// 871:
// 872:     printf("Gray gmm sample start...\n");
// 873:
// 874:     for (int i = 0; i < loop; i++) {
// 875:         printf("use_tpu: %d, show: %d, compare: %d\n", use_tpu, show, compare);
// 876:
// 877:         int ret = 0;
// 878:         if (use_tpu) {
// 879:             ret = gmm_tpu(show, compare, raw_path, output_path);
// 880:         } else {
// 881:             ret = gmm_cpu(show, compare, raw_path, output_path);
// 882:         }
// 883:
// 884:         if (ret != 0) {
// 885:             printf("GMM_Sample_U8C1 failed, ret = %d\n", ret);
// 886:             printf("------TEST CV_GMM FAILED------\n");
// 887:             bm_dev_free(handle);
// 888:             return ret;
// 889:         }
// 890:         printf("------TEST CV_GMM PASSED!------\n");
// 891:     }
// 892:
// 893:     printf("-->end!\n");
// 894:     printf("gmm sample done\n");
// 895:
// 896:     bm_dev_free(handle);
// 897:     return status;
// 898: }

// (End of file - total 898 lines)
// </content>
// ```

// ---

// ## Assistant (Build · Kimi-K2.6 · 79.1s)

// 我来详细解释 `CVI_HW_SingleGMM` 函数中的定点数计算。这是一个基于高斯混合模型（GMM）的背景减除算法，所有浮点运算都通过定点数实现。

// ## 一、参数与数据结构的定点格式

// | 参数 | 类型 | 格式 | 含义 |
// |------|------|------|------|
// | `u0q16LearnRate` | `uint16_t` | **U0.16** | 学习率，范围 [0, 1) |
// | `u0q16BgRatio` | `uint16_t` | **U0.16** | 背景比例阈值，范围 [0, 1) |
// | `u8q8VarThr` | `uint16_t` | **U8.8** | 方差阈值，范围 [0, 256) |
// | `u22q10NoiseVar` | `uint32_t` | **U22.10** | 噪声方差 |
// | `u22q10MaxVar` | `uint32_t` | **U22.10** | 最大方差 |
// | `u22q10MinVar` | `uint32_t` | **U22.10** | 最小方差 |
// | `u0q16InitWeight` | `uint16_t` | **U0.16** | 初始权重 |

// ## 二、模型数据结构（每个高斯分量 7 字节）

// ```
// mptr[0:1] = w   (16位权重, U0.16)
// mptr[2:3] = mu  (16位均值, 像素值左移8位，即 U8.8)
// mptr[4:6] = var (24位方差, U22.10)
// ```

// ---

// ## 三、逐行分析定点数计算

// ### 1. 读取模型参数（第 139-144 行）

// ```c
// w = mptr[0] + (mptr[1] << 8);                          // 读取权重 w (U0.16)
// mu = mptr[2] + (mptr[3] << 8);                         // 读取均值 mu (U8.8)
// var = mptr[4] + (mptr[5] << 8) + (mptr[6] << 16);      // 读取方差 var (U22.10)
// ```

// ### 2. 计算像素与均值的差值（第 145-146 行）

// ```c
// diff = (pix << 8) - mu;           // diff = pix * 256 - mu, 单位: U8.8
// dist2 = (diff * diff + 32) >> 6;  // dist2 = (diff² + 32) / 64
// ```

// **定点分析：**
// - `pix` 是 uint8_t 像素值 (0~255)
// - `pix << 8` 将像素转为 **U8.8** 格式
// - `diff` 也是 **U8.8** 格式
// - `diff * diff` 结果是 **U16.16** 格式（因为 U8.8 × U8.8 = U16.16）
// - `+ 32` 是为了四舍五入的偏移量（对应 >> 6 的一半）
// - `>> 6` 后，`dist2` 变为 **U16.10** 格式（即 U22.10 的低精度表示）

// > 实际等价：`dist2 = diff² / 64 = (Δpix)²`，用于后续与阈值比较

// ### 3. 匹配判断（第 147 行）

// ```c
// if ( dist2 < (u8q8VarThr * var + 128) >> 8 )
// ```

// **定点分析：**
// - `u8q8VarThr` 是 **U8.8** 格式
// - `var` 是 **U22.10** 格式
// - `u8q8VarThr * var` 结果是 **U30.18** 格式（因为 U8.8 × U22.10 = U30.18）
// - `+ 128` 是四舍五入偏移（对应 >> 8 的一半）
// - `>> 8` 后，结果变为 **U22.10** 格式

// **比较：** `dist2` (U16.10) < `threshold` (U22.10)

// > 物理意义：如果像素到某个高斯分量的马氏距离小于方差阈值 × 该分量的方差，则认为匹配

// ### 4. 匹配成功时的权重更新（第 150-151 行）

// ```c
// dw = ((0xFFFF - w) * (uint32_t)u0q16LearnRate + 0x8000) >> 16;
// w += dw;
// ```

// **定点分析：**
// - `0xFFFF` = 65535 ≈ 1.0 (U0.16 的最大值)
// - `0xFFFF - w` 是剩余权重空间
// - `u0q16LearnRate` 是 **U0.16** 格式
// - 乘法 `(0xFFFF - w) * u0q16LearnRate` 结果是 **U0.32** 格式
// - `+ 0x8000` 是四舍五入偏移（对应 >> 16 的一半）
// - `>> 16` 后，`dw` 变为 **U0.16** 格式

// > 物理意义：`w_new = w_old + learn_rate * (1 - w_old)`，即权重向 1.0 靠近

// ### 5. 均值更新（第 153 行）

// ```c
// mu += ((u0q16LearnRate * diff) >> 16);
// ```

// **定点分析：**
// - `u0q16LearnRate` 是 **U0.16** 格式
// - `diff` 是 **U8.8** 格式（有符号）
// - `u0q16LearnRate * diff` 结果是 **U8.24** 格式
// - `>> 16` 后，结果是 **U8.8** 格式
// - `mu` 原本就是 **U8.8** 格式，直接相加

// > 物理意义：`mu_new = mu_old + learn_rate * (pix - mu_old)`，即均值向当前像素靠近

// ### 6. 方差更新（第 155 行）

// ```c
// var = ((var << 16) + (u0q16LearnRate * (dist2 - var))) >> 16;
// ```

// **定点分析：**
// - `var << 16`：将 **U22.10** 的 var 转为 **U22.26**（为了和后面的乘法对齐）
// - `dist2` 是 **U16.10** 格式，`var` 是 **U22.10** 格式
// - `dist2 - var` 需要对齐：实际代码中 `dist2` 和 `var` 都是 10 位小数，可以直接减
// - `u0q16LearnRate * (dist2 - var)`：`U0.16 × U22.10 = U22.26`
// - `(var << 16)` 是 **U22.26** 格式（因为 var 是 U22.10，左移16位后小数部分变为26位）
// - 相加后 `>> 16`，结果回到 **U22.10** 格式

// > 物理意义：`var_new = (1-learn_rate)*var_old + learn_rate*dist2`，即方差向当前距离靠近

// ### 7. 方差裁剪（第 156-157 行）

// ```c
// var = MIN2(var, u22q10MaxVar);  // 上限为 u22q10MaxVar
// var = MAX2(var, u22q10MinVar);  // 下限为 u22q10MinVar
// ```

// ### 8. 未匹配时的模型替换（第 192-201 行）

// ```c
// wsum += u0q16InitWeight - w;           // 调整总权重
// pstModel[7 * k] = u0q16InitWeight;     // 新权重 (U0.16)
// pstModel[7 * k + 1] = (u0q16InitWeight >> 8);  // 权重高8位
// pstModel[7 * k + 3] = pix;             // 新均值 = 当前像素值 (U8.0)
// pstModel[7 * k + 2] = 0;             // 均值低8位 = 0
// var = 4 * u22q10NoiseVar;              // 新方差 = 4 * 噪声方差 (U22.10)
// ```

// **定点分析：**
// - 新均值 `mu = pix << 8`（即 **U8.8**），但这里直接存 `pix` 到 `mptr[3]`，`0` 到 `mptr[2]`，所以实际 `mu = pix << 8 + 0 = pix << 8`
// - 新方差 `var = 4 * u22q10NoiseVar`，保持 **U22.10** 格式

// ### 9. 权重归一化（第 207-213 行）

// ```c
// w = 0xFFFF * (uint32_t)w / One_div_wscale;
// ```

// **定点分析：**
// - `0xFFFF` ≈ 1.0 (U0.16)
// - `w` 是当前权重（未归一化）
// - `One_div_wscale` 是所有权重之和
// - `0xFFFF * w` 是 **U0.32** 格式
// - 除以 `One_div_wscale` 后，结果重新缩放到 **U0.16** 格式

// > 物理意义：`w_norm = w / sum(w_i)`，使所有权重之和为 1.0

// ### 10. 前景/背景判断（第 211-219 行）

// ```c
// if ( wsum > u0q16BgRatio && kForeground < 0 )
//     kForeground = k + 1;
// // ...
// pstFg0[x] = (kHit >= kForeground)? 0xFF : 0;
// ```

// **定点分析：**
// - 按权重从大到小排序后，累加权重
// - 当累加权重 `wsum` 超过背景阈值 `u0q16BgRatio` (U0.16) 时，后面的分量视为前景
// - `kForeground` 是背景分量的数量
// - 如果匹配的分量索引 `kHit >= kForeground`，则判定为前景（0xFF），否则为背景（0）

// ---

// ## 四、总结：定点数设计的关键点

// | 运算 | 输入格式 | 输出格式 | 关键操作 |
// |------|---------|---------|---------|
// | 像素转定点 | U8.0 | U8.8 | `pix << 8` |
// | 差值计算 | U8.8 - U8.8 | U8.8 | 直接减法 |
// | 距离平方 | U8.8 × U8.8 | U16.16 → U16.10 | `*diff` 后 `>> 6` |
// | 阈值比较 | U16.10 vs U22.10 | bool | 统一为 U22.10 后比较 |
// | 权重更新 | U0.16 × U0.16 | U0.16 | `*`, `+0x8000`, `>> 16` |
// | 均值更新 | U0.16 × U8.8 | U8.8 | `*`, `>> 16` |
// | 方差更新 | U22.10, U0.16 | U22.10 | 左移对齐，乘加，右移归位 |
// | 权重归一化 | U0.16 / sum(U0.16) | U0.16 | `0xFFFF * w / wsum` |

// 这种设计充分利用了 BM1684X TPU 的整数运算能力，避免了浮点运算，同时通过合理的定点格式选择，在精度和范围之间取得了平衡。

// ---

// ## User

// mu = mptr[2] + (mptr[3] << 8);这个可不可以理解为mptr[3]与mptr[2]拼接为8bit

// ---

// ## Assistant (Build · Kimi-K2.6 · 32.6s)

// 是的，你的理解完全正确！

// ## 详细解释

// ```c
// mu = mptr[2] + (mptr[3] << 8);
// ```

// 这确实就是将 `mptr[3]` 和 `mptr[2]` **拼接成一个 16 位的值**。

// ### 内存布局

// ```
// 地址:    mptr+3      mptr+2
//         ┌─────┐    ┌─────┐
//         │  高8位 │    │ 低8位 │
//         │ mptr[3]│    │mptr[2]│
//         └─────┘    └─────┘
//            ↓          ↓
//         mu[15:8]   mu[7:0]
// ```

// ### 等价写法

// ```c
// // 原始写法
// mu = mptr[2] + (mptr[3] << 8);

// // 等价的位拼接写法
// mu = ((uint16_t)mptr[3] << 8) | mptr[2];

// // 或者用 memcpy（更清晰的语义）
// memcpy(&mu, &mptr[2], 2);  // 小端模式
// ```

// ### 为什么是这样存储的？

// 回顾模型数据的 7 字节结构：

// ```c
// // 每个高斯分量 7 字节
// mptr[0] = w[7:0]     // 权重低8位  (U0.16)
// mptr[1] = w[15:8]    // 权重高8位
// mptr[2] = mu[7:0]    // 均值低8位  (U8.8)
// mptr[3] = mu[15:8]   // 均值高8位
// mptr[4] = var[7:0]   // 方差低8位  (U22.10)
// mptr[5] = var[15:8]  // 方差中8位
// mptr[6] = var[23:16] // 方差高8位
// ```

// 这是**小端模式（Little Endian）**存储：
// - 低位字节存放在低地址
// - 高位字节存放在高地址

// ### 对比权重和方差的读取

// ```c
// // 权重 w (16位, U0.16)
// w = mptr[0] + (mptr[1] << 8);        // 同样是小端拼接

// // 均值 mu (16位, U8.8)
// mu = mptr[2] + (mptr[3] << 8);       // 你的问题

// // 方差 var (24位, U22.10)
// var = mptr[4] + (mptr[5] << 8) + (mptr[6] << 16);  // 三个字节拼接
// ```

// ### 存储时的反向操作

// ```c
// // 存储权重 (16位)
// mptr[0] = (uint8_t)(w & 0xFF);       // 低8位
// mptr[1] = (uint8_t)(w >> 8);         // 高8位

// // 存储均值 (16位)
// mptr[2] = (uint8_t)(mu & 0xFF);      // 低8位
// mptr[3] = (uint8_t)(mu >> 8);        // 高8位

// // 存储方差 (24位)
// mptr[4] = (uint8_t)(var & 0xFF);           // 低8位
// mptr[5] = (uint8_t)((var >> 8) & 0xFF);    // 中8位
// mptr[6] = (uint8_t)((var >> 16) & 0xFF);   // 高8位
// ```

// ---

// ## 总结

// | 问题 | 答案 |
// |------|------|
// | 是不是拼接？ | ✅ 是的，就是 8bit + 8bit 拼成 16bit |
// | 什么顺序？ | 小端模式：低地址是低字节，高地址是高字节 |
// | 为什么不用 `uint16_t` 直接存？ | 因为模型数据是按字节数组 `uint8_t mptr[]` 存储的，需要手动拼接 |

// 这种手动拼接的方式在嵌入式和 TPU 编程中很常见，因为硬件往往以字节为单位访问内存，而算法需要多字节的数据类型。

// ---

// ## User

// 举例说明memcpy(&mu, &mptr[2], 2)是怎么copy的

// ---

// ## Assistant (Build · Kimi-K2.6 · 46.4s)

// 我来详细解释 `memcpy(&mu, &mptr[2], 2)` 的复制过程。

// ## 前提条件

// 假设内存中的数据如下（小端模式系统，如 x86/ARM）：

// ```
// 地址:     0x1000   0x1001   0x1002   0x1003   0x1004
//          ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐  ┌─────┐
//          │ 0xAA │  │ 0xBB │  │ 0x34 │  │ 0x12 │  │ 0xCC │
//          └─────┘  └─────┘  └─────┘  └─────┘  └─────┘
//            ↑
//          mptr[0]  mptr[1]  mptr[2]  mptr[3]  mptr[4]
//                            ↑
//                          &mptr[2]
// ```

// ## 执行 memcpy

// ```c
// uint16_t mu;
// memcpy(&mu, &mptr[2], 2);  // 从 &mptr[2] 复制 2 个字节到 &mu
// ```

// ### 复制过程

// **Step 1: 确定源地址和目标地址**

// ```
// 源地址 (&mptr[2]):   0x1002  (值为 0x34)
// 目标地址 (&mu):      假设 mu 在栈上，地址为 0x2000
// ```

// **Step 2: 逐字节复制**

// ```
// 复制前:
//     源内存 (0x1002开始):  [0x34] [0x12]
//                          低字节  高字节

//     mu 内存 (0x2000):    [??] [??]  (未初始化)

// 复制中:
//     第1个字节: mu[0] = mptr[2] = 0x34
//     第2个字节: mu[1] = mptr[3] = 0x12

// 复制后:
//     mu 内存 (0x2000):    [0x34] [0x12]
//                           低字节  高字节
// ```

// ### 结果

// ```c
// mu = 0x1234;   // 小端模式：低字节在前
// ```

// ---

// ## 与手动拼接的等价性

// ```c
// // 方法1: 手动拼接（显式）
// mu = mptr[2] + (mptr[3] << 8);
// //     0x34  +  (0x12 << 8)
// //   = 0x34  +   0x1200
// //   = 0x1234

// // 方法2: memcpy（隐式，依赖系统字节序）
// memcpy(&mu, &mptr[2], 2);
// // 结果同样是 0x1234（在小端系统上）
// ```

// ---

// ## 关键：字节序的影响

// ### 小端模式（Little Endian，如 x86、ARM）

// ```
// 内存布局:
//     地址 0x1002:  0x34  (低字节)
//     地址 0x1003:  0x12  (高字节)

// 读取 uint16_t:
//     低地址作为低字节 -> mu = 0x1234

// 图示:
//     内存:   [0x34] [0x12]
//               ↓      ↓
//     mu:     0x1234  (高字节=0x12, 低字节=0x34)
// ```

// ### 大端模式（Big Endian，如某些嵌入式处理器）

// ```
// 内存布局相同:
//     地址 0x1002:  0x34
//     地址 0x1003:  0x12

// 读取 uint16_t:
//     低地址作为高字节 -> mu = 0x3412

// 图示:
//     内存:   [0x34] [0x12]
//               ↓      ↓
//     mu:     0x3412  (高字节=0x34, 低字节=0x12)
// ```

// ---

// ## 完整代码示例

// ```c
#include <stdio.h>
#include <stdint.h>
#include <string.h>

int main() {
    uint8_t mptr[4] = {0xAA, 0xBB, 0x34, 0x12};  // 模拟内存数据

    // 方法1: 手动拼接
    uint16_t mu1 = mptr[2] + (mptr[3] << 8);

    // uint32_t weight;
    // memcpy(&weight, mptr, 4);
    // printf("weight = %x\n", weight);

    uint32_t *weight = (uint32_t*)mptr;
    printf("weight = %x\n", *weight);
    printf("mptr[0] = %p, mptr[1] = %p, mptr[2] = %p, mptr[3] = %p\n", &mptr[0], &mptr[1], &mptr[2], &mptr[3]);
    // 方法2: memcpy
    uint16_t mu2;
    memcpy(&mu2, &mptr[2], 2);

    printf("mptr[2] = 0x%02X\n", mptr[2]);  // 0x34
    printf("mptr[3] = 0x%02X\n", mptr[3]);  // 0x12

    printf("mu1 (手动拼接) = 0x%04X (%u)\n", mu1, mu1);  // 0x1234 (4660)
    printf("mu2 (memcpy)   = 0x%04X (%u)\n", mu2, mu2);  // 0x1234 (4660)

    // 查看 mu2 在内存中的字节
    uint8_t *p = (uint8_t*)&mu2;
    printf("mu2 内存: [%02X] [%02X]\n", p[0], p[1]);  // [34] [12]

    return 0;
}
// ```

// **输出：**
// ```
// mptr[2] = 0x34
// mptr[3] = 0x12
// mu1 (手动拼接) = 0x1234 (4660)
// mu2 (memcpy)   = 0x1234 (4660)
// mu2 内存: [34] [12]
// ```

// ---

// ## 在 GMM 代码中的实际应用

// ```c
// // 原始代码中的三种读取方式（都是小端拼接）

// // 1. 权重 (16位)
// w = mptr[0] + (mptr[1] << 8);
// // 等价于: memcpy(&w, mptr, 2);

// // 2. 均值 (16位)
// mu = mptr[2] + (mptr[3] << 8);
// // 等价于: memcpy(&mu, &mptr[2], 2);

// // 3. 方差 (24位) - 注意这个不能直接 memcpy，因为 uint32_t 是 4 字节
// var = mptr[4] + (mptr[5] << 8) + (mptr[6] << 16);
// // 如果要用 memcpy，需要额外处理:
// uint32_t var_temp;
// memcpy(&var_temp, &mptr[4], 3);  // 复制3字节，但第4字节是垃圾值
// var = var_temp & 0xFFFFFF;       // 掩码掉高字节
// ```

// ---

// ## 总结

// | 方式 | 优点 | 缺点 |
// |------|------|------|
// | `mptr[2] + (mptr[3] << 8)` | 明确、可移植、不依赖字节序 | 代码稍长 |
// | `memcpy(&mu, &mptr[2], 2)` | 简洁、可能更高效（编译器优化） | 依赖系统字节序 |

// **在 BM1684X TPU 代码中**，使用手动拼接而不是 `memcpy` 的原因是：
// 1. **确定性**：不依赖运行平台的字节序
// 2. **精确控制**：可以处理非标准位宽（如 24 位方差）
// 3. **硬件兼容性**：TPU 核内可能有自己的字节序规则

// ---

