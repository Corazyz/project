
#include "sample_assist.h"
#include "sample_define.h"
#include <stdlib.h>
#include <string.h>
#if defined WIN32 || defined _WIN32 || defined WINCE
#include <Windows.h>
#else
#include <time.h>
#endif

uint32_t CVI_CalcStride(uint32_t u32Width, uint32_t u32Align)
{
	uint32_t u16stride = u32Width + (u32Align - u32Width%u32Align)%u32Align;
	return u16stride;
}
#ifndef align_up
#define align_up(num, align) \
    (((num) + ((align) - 1)) & ~((align) - 1))
#endif
typedef uint16_t offset_t;
#define PTR_OFFSET_SZ sizeof(offset_t)

void * aligned_malloc(size_t align, size_t size)
{
    void * ptr = NULL;

    // We want it to be a power of two since
    // align_up operates on powers of two
    assert((align & (align - 1)) == 0);

    if(align && size)
    {
        /*
         * We know we have to fit an offset value
         * We also allocate extra bytes to ensure we
         * can meet the alignment
         */
        uint32_t hdr_size = PTR_OFFSET_SZ + (align - 1);
        void * p = malloc(size + hdr_size);

        if(p)
        {
            /*
             * Add the offset size to malloc's pointer
             * (we will always store that)
             * Then align the resulting value to the
             * target alignment
             */
            ptr = (void *) align_up(((uintptr_t)p + PTR_OFFSET_SZ), align);

            // Calculate the offset and store it
            // behind our aligned pointer
            *((offset_t *)ptr - 1) =
                (offset_t)((uintptr_t)ptr - (uintptr_t)p);

        } // else NULL, could not malloc
    } //else NULL, invalid arguments

    return ptr;
}
int32_t CVI_CreateIveImage(IVE_IMAGE_S *pstImage,IVE_IMAGE_TYPE_E enType, uint32_t u32Width, uint32_t u32Height)
{
	uint32_t u32Stride;
	int32_t s32Succ;

	CVI_CHECK_ET_NULL_RET(pstImage,CVI_FAILURE);

	pstImage->enType = enType;
	pstImage->u32Width = u32Width;
	pstImage->u32Height = u32Height;

	// u32Stride = CVI_CalcStride(u32Width, CVI_IVE2_STRIDE_ALIGN);
    u32Stride = CVI_CalcStride(u32Width, 1);//84x2 support byte align
	s32Succ = CVI_SUCCESS;
    printf("u32Stride = %d\n", u32Stride);
	switch(enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height);
			//pstImage->au64VirAddr[0] = (uint64_t)aligned_malloc(16, u32Stride * u32Height);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			assert(pstImage->au64VirAddr[0] % 16 == 0);
			pstImage->au32Stride[0]  = u32Stride;
		}
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 3/2);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + u32Stride * u32Height;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au32Stride[0]  = u32Stride;
			pstImage->au32Stride[1]  = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 2);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + u32Stride * u32Height;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au32Stride[0] = u32Stride;
			pstImage->au32Stride[1]  = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_YUV420P:
		{
			uint32_t u32Stride2;

			u32Stride2= CVI_CalcStride(u32Width/2, CVI_IVE2_STRIDE_ALIGN);
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height + u32Stride2 * u32Height);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);
			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + u32Stride * u32Height;
			pstImage->au64VirAddr[2] = pstImage->au64VirAddr[1] + u32Stride2 * u32Height/2;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au64PhyAddr[2] = (uint64_t)pstImage->au64VirAddr[2];
			pstImage->au32Stride[0] = u32Stride;
			pstImage->au32Stride[1]  = u32Stride2;
			pstImage->au32Stride[2]  = pstImage->au32Stride[1];
		}
		break;
	case IVE_IMAGE_TYPE_YUV422P:
		{
			uint32_t u32Stride2;

			u32Stride2 = CVI_CalcStride(u32Width/2, CVI_IVE2_STRIDE_ALIGN);
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height + u32Stride2 * u32Height*2);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);
			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + u32Stride * u32Height;
			pstImage->au64VirAddr[2] = pstImage->au64VirAddr[1] + u32Stride2 * u32Height;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au64PhyAddr[2] = (uint64_t)pstImage->au64VirAddr[2];
			pstImage->au32Stride[0]  = u32Stride;
			pstImage->au32Stride[1]  = u32Stride2;
			pstImage->au32Stride[2]  = pstImage->au32Stride[1];
		}
		break;
	case IVE_IMAGE_TYPE_YUV444P:
		{
			printf(" %d %d %d fff\n", u32Stride, u32Height, (u32Stride * u32Height * 3));
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 3);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);
			printf(" %d %d %d eee\n", u32Stride, u32Height, (u32Stride * u32Height * 3));
			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + 1;
			pstImage->au64VirAddr[2] = pstImage->au64VirAddr[1] + 1;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au64PhyAddr[2] = (uint64_t)pstImage->au64VirAddr[2];

			pstImage->au32Stride[0] = u32Stride;
			pstImage->au32Stride[1] = pstImage->au32Stride[0];
			pstImage->au32Stride[2] = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 2);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + 1;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au32Stride[0] = u32Stride;
			pstImage->au32Stride[1]  = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_S8C2_PLANAR:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 2);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + u32Stride * u32Height;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au32Stride[0]  = u32Stride;
			pstImage->au32Stride[1]  = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 2);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au32Stride[0] = u32Stride;
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 3);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + 1;
			pstImage->au64VirAddr[2] = pstImage->au64VirAddr[1] + 1;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au64PhyAddr[2] = (uint64_t)pstImage->au64VirAddr[2];
			pstImage->au32Stride[0] = u32Stride;
			pstImage->au32Stride[1] = pstImage->au32Stride[0];
			pstImage->au32Stride[2] = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		{
			//pstImage->au64VirAddr[0] = (uint64_t)aligned_malloc(16, u32Stride * u32Height * 3);
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 3);
			assert(pstImage->au64VirAddr[0] % 16 == 0);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64VirAddr[1] = pstImage->au64VirAddr[0] + u32Stride * u32Height;
			pstImage->au64VirAddr[2] = pstImage->au64VirAddr[1] + u32Stride * u32Height;
			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au64PhyAddr[1] = (uint64_t)pstImage->au64VirAddr[1];
			pstImage->au64PhyAddr[2] = (uint64_t)pstImage->au64VirAddr[2];
			pstImage->au32Stride[0] = u32Stride;
			pstImage->au32Stride[1] = pstImage->au32Stride[0];
			pstImage->au32Stride[2] = pstImage->au32Stride[0];
		}
		break;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 4);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au32Stride[0] = u32Stride;
		}
		break;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		{
			pstImage->au64VirAddr[0] = (uint64_t)malloc(u32Stride * u32Height * 8);
			CVI_CHECK_ET_RET(pstImage->au64VirAddr[0],0,CVI_FAILURE);

			pstImage->au64PhyAddr[0] = (uint64_t)pstImage->au64VirAddr[0];
			pstImage->au32Stride[0] = u32Stride;
		}
		break;
	default:
		{
			s32Succ = CVI_FAILURE;
		}
		break;
	}

	return s32Succ;
}

int32_t CVI_DestroyIveImage(IVE_IMAGE_S *pstImage)
{

#if defined BOARD_FPGA || defined BOARD_ASIC
    UNUSED(pstImage);
#else
	CVI_CHECK_ET_NULL_RET(pstImage,CVI_FAILURE);
	if (0 != pstImage->au64VirAddr[0])
	{
		free((void*)pstImage->au64VirAddr[0]);
		pstImage->au64VirAddr[0] = 0;
	}
#endif

	return CVI_SUCCESS;
}

int32_t comp(const void *a,const void *b)
{
	return  *(uint8_t *)(*(uint32_t *)b) - *(uint8_t *)(*(uint32_t *)a);
}

static int32_t find_first_diff_u8(const uint8_t *A, const uint8_t *B, int32_t count)
{
	int32_t i;
	int32_t result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

void dump_data_u8(const uint8_t *data, int32_t count, const char *desc)
{
	int32_t i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%02X ", data[i]);
		if ((i+1) % 8 == 0 || i+1 == count) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("\n");
			} else if (i+1 == count) {
				printf("\n");
			}
		}
	}
}

static int32_t find_first_diff_u16(const uint16_t *A, const uint16_t *B, int32_t count)
{
	int32_t i;
	int32_t result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

void dump_data_u16(const uint16_t *data, int32_t count, const char *desc)
{
	int32_t i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%04X ", data[i]);
		if ((i+1) % 8 == 0 || i+1 == count) {
			printf(" ");
			if ((i+1) % 16 == 0) {
				printf("\n");
			} else if (i+1 == count) {
				printf("\n");
			}
		}
	}
}

static int32_t find_first_diff_u32(const uint32_t *A, const uint32_t *B, int32_t count)
{
	int32_t i;
	int32_t result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

void dump_data_u32(const uint32_t *data, int32_t count, const char *desc)
{
	int32_t i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%08X ", data[i]);
		if ((i+1) % 4 == 0 || i+1 == count) {
			printf(" ");
			if ((i+1) % 8 == 0) {
				printf("\n");
			} else if (i+1 == count) {
				printf("\n");
			}
		}
	}
}

static int32_t find_first_diff_u64(const uint64_t *A, const uint64_t *B, int32_t count)
{
	int32_t i;
	int32_t result = -1;
	for (i = 0; i < count; ++i) {
		if (A[i] != B[i]) {
			result = i;
			break;
		}
	}
	return result;
}

void dump_data_u64(const uint64_t *data, int32_t count, const char *desc)
{
	int32_t i;
	printf("%s\n", desc);
	for (i = 0; i < count; ++i) {
		printf("%016jX ", data[i]);
		if ((i+1) % 2 == 0 || i+1 == count) {
			printf(" ");
			if ((i+1) % 4 == 0) {
				printf("\n");
			} else if (i+1 == count) {
				printf("\n");
			}
		}
	}
}

int32_t CVI_SaveIveImageToBin(IVE_IMAGE_S *pstImage, const char *filepath)
{
    if (pstImage == NULL || filepath == NULL) {
        printf("Invalid parameters\n");
        return CVI_FAILURE;
    }

    FILE *fp = fopen(filepath, "wb");
    if (fp == NULL) {
        perror("File open failed");
        return CVI_FAILURE;
    }

    IVE_IMAGE_TYPE_E enType = pstImage->enType;
    uint32_t u32Width = pstImage->u32Width;
    uint32_t u32Height = pstImage->u32Height;

    size_t bytes_written = 0;

    switch (enType) {
        case IVE_IMAGE_TYPE_U8C1:
        case IVE_IMAGE_TYPE_S8C1:
        case IVE_IMAGE_TYPE_S16C1:
        case IVE_IMAGE_TYPE_U16C1:
        case IVE_IMAGE_TYPE_S32C1:
        case IVE_IMAGE_TYPE_U32C1:
        case IVE_IMAGE_TYPE_S64C1:
        case IVE_IMAGE_TYPE_U64C1:
        {
            uint8_t *pData = (uint8_t*)pstImage->au64VirAddr[0];
            uint32_t stride = pstImage->au32Stride[0];
            uint32_t height = u32Height;

            for (uint32_t y = 0; y < height; y++) {
                bytes_written = fwrite(pData, 1, stride, fp);
                if (bytes_written != stride) {
                    perror("File write error");
                    fclose(fp);
                    return CVI_FAILURE;
                }
                pData += stride;
            }
            break;
        }

        case IVE_IMAGE_TYPE_YUV420SP:
        {
            // Y 平面: w × h
            uint8_t *pDataY = (uint8_t*)pstImage->au64VirAddr[0];
            uint32_t strideY = pstImage->au32Stride[0];
            for (uint32_t y = 0; y < u32Height; y++) {
                bytes_written = fwrite(pDataY, 1, strideY, fp);
                if (bytes_written != strideY) {
                    perror("File write error");
                    fclose(fp);
                    return CVI_FAILURE;
                }
                pDataY += strideY;
            }

            // UV 平面: w × h/2 (SP格式，UV交错，共用一个plane)
            uint8_t *pDataUV = (uint8_t*)pstImage->au64VirAddr[1];
            uint32_t strideUV = pstImage->au32Stride[1];
            uint32_t heightUV = u32Height / 2;

            for (uint32_t y = 0; y < heightUV; y++) {
                bytes_written = fwrite(pDataUV, 1, strideUV, fp);
                if (bytes_written != strideUV) {
                    perror("File write error");
                    fclose(fp);
                    return CVI_FAILURE;
                }
                pDataUV += strideUV;
            }
            break;
        }

        case IVE_IMAGE_TYPE_YUV422SP:
        {
            // Y 平面: w × h
            uint8_t *pDataY = (uint8_t*)pstImage->au64VirAddr[0];
            uint32_t strideY = pstImage->au32Stride[0];
            for (uint32_t y = 0; y < u32Height; y++) {
                bytes_written = fwrite(pDataY, 1, strideY, fp);
                if (bytes_written != strideY) {
                    perror("File write error");
                    fclose(fp);
                    return CVI_FAILURE;
                }
                pDataY += strideY;
            }

            // UV 平面: w × h (SP格式，UV交错，共用一个plane)
            uint8_t *pDataUV = (uint8_t*)pstImage->au64VirAddr[1];
            uint32_t strideUV = pstImage->au32Stride[1];
            for (uint32_t y = 0; y < u32Height; y++) {
                bytes_written = fwrite(pDataUV, 1, strideUV, fp);
                if (bytes_written != strideUV) {
                    perror("File write error");
                    fclose(fp);
                    return CVI_FAILURE;
                }
                pDataUV += strideUV;
            }
            break;
        }

        case IVE_IMAGE_TYPE_YUV444P:
        {
            for (int plane = 0; plane < 3; plane++) {
                uint8_t *pData = (uint8_t*)pstImage->au64VirAddr[plane];
                uint32_t stride = pstImage->au32Stride[plane];
                for (uint32_t y = 0; y < u32Height; y++) {
                    bytes_written = fwrite(pData, 1, stride, fp);
                    if (bytes_written != stride) {
                        perror("File write error");
                        fclose(fp);
                        return CVI_FAILURE;
                    }
                    pData += stride;
                }
            }
            break;
        }

        case IVE_IMAGE_TYPE_U8C3_PACKAGE:
        {
            uint8_t *pData = (uint8_t*)pstImage->au64VirAddr[0];
            uint32_t stride = pstImage->au32Stride[0];
            for (uint32_t y = 0; y < u32Height; y++) {
                bytes_written = fwrite(pData, 1, stride, fp);
                if (bytes_written != stride) {
                    perror("File write error");
                    fclose(fp);
                    return CVI_FAILURE;
                }
                pData += stride;
            }
            break;
        }

        case IVE_IMAGE_TYPE_U8C3_PLANAR:
        {
            for (int plane = 0; plane < 3; plane++) {
                uint8_t *pData = (uint8_t*)pstImage->au64VirAddr[plane];
                uint32_t stride = pstImage->au32Stride[plane];
                for (uint32_t y = 0; y < u32Height; y++) {
                    bytes_written = fwrite(pData, 1, stride, fp);
                    if (bytes_written != stride) {
                        perror("File write error");
                        fclose(fp);
                        return CVI_FAILURE;
                    }
                    pData += stride;
                }
            }
            break;
        }

        default:
            printf("Unsupported image type: %d\n", enType);
            fclose(fp);
            return CVI_FAILURE;
    }

    fclose(fp);
    printf("Image saved to: %s\n", filepath);
    return CVI_SUCCESS;
}


int32_t CVI_CompareIveImage(IVE_IMAGE_S *pstImage1,IVE_IMAGE_S *pstImage2)
{
	uint32_t u32Stride;
	int32_t s32Succ;
	IVE_IMAGE_TYPE_E enType;
	uint32_t u32Width;
	uint32_t u32Height;
	uint8_t *pData1;
	uint8_t *pData2;
	uint16_t *pData1_U16;
	uint16_t *pData2_U16;
	uint32_t *pData1_U32;
	uint32_t *pData2_U32;
	uint64_t *pData1_U64;
	uint64_t *pData2_U64;
	uint16_t y;
	int n;

	CVI_CHECK_ET_NULL_RET(pstImage1,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(pstImage2,CVI_FAILURE);

	if (pstImage1->enType != pstImage2->enType)
	{
		printf("Not same IMAGE_TYPE, %d vs %d\n", pstImage1->enType, pstImage2->enType);
		return CVI_FAILURE;
	}
	if (pstImage1->u32Width != pstImage2->u32Width)
	{
		printf("Not same u32Width, %d vs %d\n", pstImage1->u32Width, pstImage2->u32Width);
		return CVI_FAILURE;
	}
	if (pstImage1->u32Height != pstImage2->u32Height)
	{
		printf("Not same u32Height, %d vs %d\n", pstImage1->u32Height, pstImage2->u32Height);
		return CVI_FAILURE;
	}

	enType = pstImage1->enType;
	u32Width = pstImage1->u32Width;
	u32Height = pstImage1->u32Height;

	u32Stride = CVI_CalcStride(u32Width, CVI_IVE2_STRIDE_ALIGN);
	s32Succ = CVI_SUCCESS;

	switch(enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			printf("compare A at %p, B at %p\n", pData1, pData2);
			for (y = 0; y < u32Height; y++,pData1 += u32Stride,pData2 += u32Stride)
			{
				n = memcmp(pData1,pData2,u32Width);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData1 += u32Stride,pData2 += u32Stride)
			{
				n = memcmp(pData1,pData2,u32Width);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare Y failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}

			pData1 = (uint8_t*)pstImage1->au64VirAddr[1];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[1];
			for (y = 0; y < u32Height / 2; y++,pData1 += u32Stride,pData2 += u32Stride)
			{
				n = memcmp(pData1,pData2,u32Width);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare UV failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData1 += u32Stride,pData2 += u32Stride)
			{
				n = memcmp(pData1,pData2,u32Width);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare Y failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}

			pData1 = (uint8_t*)pstImage1->au64VirAddr[1];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[1];
			for (y = 0; y < u32Height; y++,pData1 += u32Stride,pData2 += u32Stride)
			{
				n = memcmp(pData1,pData2,u32Width);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare UV failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV444P:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++, pData1 += u32Stride*3,pData2 += u32Stride*3)
			{
				n = memcmp(pData1,pData2,u32Width * 3);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width * 3);
					CVI_ASSERT(idx >= 0);
					printf("Compare U8C3_PACKAGE failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf(" A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width * 3, "A");
					dump_data_u8(pData2, u32Width * 3, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV422P:
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
	case IVE_IMAGE_TYPE_S8C2_PLANAR:
		{
			printf("Unsupported IMAGE_TYPE %d\n", enType);
			s32Succ = CVI_FAILURE;
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++, pData1 += u32Stride*3,pData2 += u32Stride*3)
			{
				n = memcmp(pData1,pData2,u32Width * 3);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width * 3);
					CVI_ASSERT(idx >= 0);
					printf("Compare U8C3_PACKAGE failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width * 3, "A");
					dump_data_u8(pData2, u32Width * 3, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		{
			for (int i = 0; i < 3; i++)
			{
				pData1 = (uint8_t*)pstImage1->au64VirAddr[i];
				pData2 = (uint8_t*)pstImage2->au64VirAddr[i];
				for (y = 0; y < u32Height; y++, pData1 += u32Stride,pData2 += u32Stride)
				{
				n = memcmp(pData1,pData2,u32Width);
				if (0 != n)
				{
					int idx = find_first_diff_u8(pData1, pData2, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare UV failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
					dump_data_u8(pData1, u32Width, "A");
					dump_data_u8(pData2, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData1 += u32Stride*2,pData2 += u32Stride*2)
			{
				n = memcmp(pData1,pData2,u32Width*2);
				if (0 != n)
				{
					pData1_U16 = (uint16_t*)pData1;
					pData2_U16 = (uint16_t*)pData2;
					int idx = find_first_diff_u16(pData1_U16, pData2_U16, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%04X, B = 0x%04X\n", pData1_U16[idx], pData2_U16[idx]);
					dump_data_u16(pData1_U16, u32Width, "A");
					dump_data_u16(pData2_U16, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData1 += u32Stride*4,pData2 += u32Stride*4)
			{
				n = memcmp(pData1,pData2,u32Width*4);
				if (0 != n)
				{
					pData1_U32 = (uint32_t*)pData1;
					pData2_U32 = (uint32_t*)pData2;
					int idx = find_first_diff_u32(pData1_U32, pData2_U32, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%08X, B = 0x%08X\n", pData1_U32[idx], pData2_U32[idx]);
					dump_data_u32(pData1_U32, u32Width, "A");
					dump_data_u32(pData2_U32, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		{
			pData1 = (uint8_t*)pstImage1->au64VirAddr[0];
			pData2 = (uint8_t*)pstImage2->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData1 += u32Stride*8,pData2 += u32Stride*8)
			{
				n = memcmp(pData1,pData2,u32Width*8);
				if (0 != n)
				{
					pData1_U64 = (uint64_t*)pData1;
					pData2_U64 = (uint64_t*)pData2;
					int idx = find_first_diff_u64(pData1_U64, pData2_U64, u32Width);
					CVI_ASSERT(idx >= 0);
					printf("Compare failed, at y = %d, x = %d, n = %d\n", y, idx, n);
					printf("  A = 0x%016jX, B = 0x%016jX\n", pData1_U64[idx], pData2_U64[idx]);
					dump_data_u64(pData1_U64, u32Width, "A");
					dump_data_u64(pData2_U64, u32Width, "B");
					s32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	default:
		{
			printf("Unknown IMAGE_TYPE %d\n", enType);
			s32Succ = CVI_FAILURE;
		}
		break;
	}

	return s32Succ;
}

int32_t CVI_ResetIveImage(IVE_IMAGE_S *pstImage,uint8_t val)
{
	uint32_t u32Stride;
	int32_t s32Succ;
	IVE_IMAGE_TYPE_E enType;
	uint32_t u32Width;
	uint32_t u32Height;
	uint8_t *pData;
	uint16_t y;

	CVI_CHECK_ET_NULL_RET(pstImage,CVI_FAILURE);

	enType = pstImage->enType;
	u32Width = pstImage->u32Width;
	u32Height = pstImage->u32Height;

	u32Stride = CVI_CalcStride(u32Width, CVI_IVE2_STRIDE_ALIGN);
	s32Succ = CVI_SUCCESS;

	switch(enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		{
			pData = (uint8_t*)pstImage->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData += u32Stride)
			{
				memset(pData,val,u32Width);
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		{
			pData = (uint8_t*)pstImage->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData += u32Stride)
			{
				memset(pData,val,u32Width);
			}
			pData = (uint8_t*)pstImage->au64VirAddr[1];
			for (y = 0; y < u32Height / 2; y++,pData += u32Stride)
			{
				memset(pData,val,u32Width);
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		{
			pData = (uint8_t*)pstImage->au64VirAddr[0];
			for (y = 0; y < u32Height; y++,pData += u32Stride)
			{
				memset(pData,val,u32Width);
			}
			pData = (uint8_t*)pstImage->au64VirAddr[1];
			for (y = 0; y < u32Height; y++,pData += u32Stride)
			{
				memset(pData,val,u32Width);
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV420P:
	case IVE_IMAGE_TYPE_YUV422P:
	case IVE_IMAGE_TYPE_S8C2_PACKAGE:
	case IVE_IMAGE_TYPE_S8C2_PLANAR:
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		{
			printf("Unsupported IMAGE_TYPE %d\n", enType);
			s32Succ = CVI_FAILURE;
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		{
			pData = (uint8_t*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++, pData += pstImage->au32Stride[0] * 3)
			{
				memset(pData,val,pstImage->u32Width * 3);
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		{
			for (int i = 0; i < 3; i++)
			{
				pData = (uint8_t*)pstImage->au64VirAddr[i];
				for (y = 0; y < pstImage->u32Height; y++, pData += pstImage->au32Stride[i])
				{
					memset(pData,val,pstImage->u32Width);
				}
			}

		}
		break;	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		{
			printf("Unsupported IMAGE_TYPE %d\n", enType);
			s32Succ = CVI_FAILURE;
		}
		break;
	default:
		{
			printf("Unknown IMAGE_TYPE %d\n", enType);
			s32Succ = CVI_FAILURE;
		}
		break;
	}

	return s32Succ;
}

int32_t CVI_CompareIveMem(IVE_MEM_INFO_S *pstMem1,IVE_MEM_INFO_S *pstMem2)
{
	int32_t s32Succ;
	uint32_t u32Size;
	uint8_t *pData1;
	uint8_t *pData2;
	int n;

	CVI_CHECK_ET_NULL_RET(pstMem1,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(pstMem2,CVI_FAILURE);

	if (pstMem1->u32Size != pstMem2->u32Size)
	{
		printf("Not same size, %d vs %d\n", pstMem1->u32Size, pstMem2->u32Size);
		return CVI_FAILURE;
	}

	u32Size = pstMem1->u32Size;
	s32Succ = CVI_SUCCESS;

	pData1 = (uint8_t*)pstMem1->u64VirAddr;
	pData2 = (uint8_t*)pstMem2->u64VirAddr;
	n = memcmp(pData1,pData2,u32Size);
	if (0 != n)
	{
		int idx = find_first_diff_u8((uint8_t*)pData1, (uint8_t*)pData2, u32Size);
		CVI_ASSERT(idx >= 0);
		printf("Compare failed, at idx = %d, n = %d\n", idx, n);
		printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
		dump_data_u8((uint8_t*)pData1, u32Size, "A");
		dump_data_u8((uint8_t*)pData2, u32Size, "B");
		s32Succ = CVI_FAILURE;
	}

	return s32Succ;
}

int32_t CVI_ResetIveMem(IVE_MEM_INFO_S *pstMem,uint8_t val)
{
	int32_t s32Succ;
	uint32_t u32Size;
	uint8_t *pData;

	CVI_CHECK_ET_NULL_RET(pstMem,CVI_FAILURE);

	u32Size = pstMem->u32Size;
	s32Succ = CVI_SUCCESS;

	pData = (uint8_t*)pstMem->u64VirAddr;
	memset(pData,val,u32Size);

	return s32Succ;
}

int32_t CVI_CompareIveData(IVE_DATA_S *pstData1,IVE_DATA_S *pstData2)
{
	int32_t s32Succ;
	uint32_t u32Height;
	uint32_t u32Width;
	uint8_t *pData1;
	uint8_t *pData2;
	uint16_t y;
	int n;

	CVI_CHECK_ET_NULL_RET(pstData1,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(pstData2,CVI_FAILURE);

	if (pstData1->u32Height != pstData2->u32Height)
	{
		printf("Not same Height, %d vs %d\n", pstData1->u32Height, pstData2->u32Height);
		return CVI_FAILURE;
	}
	if (pstData1->u32Width != pstData2->u32Width)
	{
		printf("Not same Height, %d vs %d\n", pstData1->u32Height, pstData2->u32Height);
		return CVI_FAILURE;
	}

	u32Height = pstData1->u32Height;
	u32Width = pstData1->u32Width;
	s32Succ = CVI_SUCCESS;

	pData1 = (uint8_t*)pstData1->u64VirAddr;
	pData2 = (uint8_t*)pstData2->u64VirAddr;
	for (y = 0; y < u32Height; y++,pData1 += pstData1->u32Stride,pData2 += pstData2->u32Stride)
	{
		n = memcmp(pData1,pData2,u32Width);
		if (0 != n)
		{
			int idx = find_first_diff_u8((uint8_t*)pData1, (uint8_t*)pData2, u32Width);
			CVI_ASSERT(idx >= 0);
			printf("Compare failed, at idx = %d, n = %d\n", idx, n);
			printf("  A = 0x%02X, B = 0x%02X\n", pData1[idx], pData2[idx]);
			dump_data_u8((uint8_t*)pData1, u32Width, "A");
			dump_data_u8((uint8_t*)pData2, u32Width, "B");
			s32Succ = CVI_FAILURE;
			break;
		}
	}

	return s32Succ;
}

int64_t CVI_GetTickCount(void)
{
	int64_t s32Tick = 0;

#if defined WIN32 || defined _WIN32 || defined WINCE
	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );
	s32Tick = (int64_t)counter.QuadPart;
#elif defined BOARD_FPGA  || defined BOARD_ASIC
    #include "timer.h"
    //s32Tick = timer_meter_get_ms();
    s32Tick = 0;
#elif defined __linux || defined __linux__
	struct timespec tp;
	clock_gettime(CLOCK_MONOTONIC, &tp);
	s32Tick =  (int64_t)tp.tv_sec*1000000000 + tp.tv_nsec;
#else
	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz );
	s32Tick =  (int64_t)tv.tv_sec*1000000 + tv.tv_usec;
#endif

	return s32Tick;
}

CVI_DOUBLE CVI_GetTickFrequency(void)
{
	CVI_DOUBLE dFre = 1.0;
#if defined WIN32 || defined _WIN32 || defined WINCE
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	dFre =  (CVI_DOUBLE)freq.QuadPart;
#elif defined __linux || defined __linux__
	dFre =  1e9;
#else
	dFre =  1e6;
#endif

	return (dFre * 1e-6);

}

int32_t CVI_GenRand(int32_t s32Max,int32_t s32Min)
{
	int32_t s32Result = 0;

	if (s32Min >= 0)
	{
		s32Result = s32Min + rand()%(s32Max - s32Min + 1);
	}
	else
	{
		s32Result = rand() % (s32Max - s32Min + 1);
		s32Result = s32Result > s32Max ?   s32Max - s32Result: s32Result;
	}

	return s32Result;
}
