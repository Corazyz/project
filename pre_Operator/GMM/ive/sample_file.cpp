#include "sample_file.h"
#include "sample_define.h"

CVI_S32 CVI_ReadFile(IVE_SRC_IMAGE_S *pstImage,FILE *fp)
{
	CVI_U16 y;
	CVI_S32 u32Succ;
	CVI_U8 *pData;


	CVI_CHECK_ET_NULL_RET(pstImage,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	if (feof(fp))
	{
		fseek(fp, 0 , SEEK_SET);
	}

	u32Succ = CVI_SUCCESS;

	switch(pstImage->enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fread(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}

		}
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fread(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}

			pData = (CVI_U8*)pstImage->au64VirAddr[1];
			for (y = 0; y < pstImage->u32Height/2; y++,pData += pstImage->au32Stride[1])
			{
				if (1 != fread(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fread(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}

			pData = (CVI_U8*)pstImage->au64VirAddr[1];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[1])
			{
				if (1 != fread(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 3)
			{
				if (1 != fread(pData,pstImage->u32Width * 3,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		{
			for (int i = 0; i < 3 ;i++)
			{
				pData = (CVI_U8*)pstImage->au64VirAddr[i];
				for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[i] )
				{
					if (1 != fread(pData,pstImage->u32Width,1,fp))
					{
						u32Succ = CVI_FAILURE;
						break;
					}
				}
			}

		}
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 2 )
			{
				if (2 != fread(pData,pstImage->u32Width,2,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 4 )
			{
				if (4 != fread(pData,pstImage->u32Width,4,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 8 )
			{
				if (8 != fread(pData,pstImage->u32Width,8,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	default:
		u32Succ = CVI_FAILURE;
		break;
	}

	return u32Succ;
}

CVI_S32 CVI_WriteFile(IVE_SRC_IMAGE_S *pstImage,FILE *fp)
{
	CVI_U16 y;
	CVI_S32 u32Succ;
	CVI_U8 *pData;


	CVI_CHECK_ET_NULL_RET(pstImage,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	u32Succ = CVI_SUCCESS;

	switch(pstImage->enType)
	{
	case IVE_IMAGE_TYPE_U8C1:
	case IVE_IMAGE_TYPE_S8C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fwrite(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}

		}
		break;
	case IVE_IMAGE_TYPE_YUV420SP:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fwrite(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}

			pData = (CVI_U8*)pstImage->au64VirAddr[1];
			for (y = 0; y < pstImage->u32Height/2; y++,pData += pstImage->au32Stride[1])
			{
				if (1 != fwrite(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_YUV422SP:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0])
			{
				if (1 != fwrite(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}

			pData = (CVI_U8*)pstImage->au64VirAddr[1];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[1])
			{
				if (1 != fwrite(pData,pstImage->u32Width,1,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PACKAGE:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++, pData += pstImage->au32Stride[0] * 3)
			{
				if (1 != fwrite(pData, pstImage->u32Width * 3, 1, fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_U8C3_PLANAR:
		{
			for (int i = 0; i < 3; i++)
			{
				pData = (CVI_U8*)pstImage->au64VirAddr[i];
				for (y = 0; y < pstImage->u32Height; y++, pData += pstImage->au32Stride[i])
				{
					if (1 != fwrite(pData, pstImage->u32Width, 1, fp))
					{
						u32Succ = CVI_FAILURE;
						break;
					}
				}
			}

		}
		break;
	case IVE_IMAGE_TYPE_S16C1:
	case IVE_IMAGE_TYPE_U16C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 2 )
			{
				if (2 != fwrite(pData,pstImage->u32Width,2,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S32C1:
	case IVE_IMAGE_TYPE_U32C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 4 )
			{
				if (4 != fwrite(pData,pstImage->u32Width,4,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	case IVE_IMAGE_TYPE_S64C1:
	case IVE_IMAGE_TYPE_U64C1:
		{
			pData = (CVI_U8*)pstImage->au64VirAddr[0];
			for (y = 0; y < pstImage->u32Height; y++,pData += pstImage->au32Stride[0] * 8 )
			{
				if (8 != fwrite(pData,pstImage->u32Width,8,fp))
				{
					u32Succ = CVI_FAILURE;
					break;
				}
			}
		}
		break;
	default:
		printf("Unknown IMAGE_TYPE %d\n", pstImage->enType);
		u32Succ = CVI_FAILURE;
		break;
	}

	return u32Succ;
}

CVI_S32 CVI_MemReadFile(IVE_MEM_INFO_S *pstMem,FILE *fp)
{
	CVI_S32 u32Succ;
	CVI_U8 *pData;

	CVI_CHECK_ET_NULL_RET(pstMem,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	u32Succ = CVI_SUCCESS;

	pData = (CVI_U8*)pstMem->u64VirAddr;
	if (1 != fread(pData,pstMem->u32Size,1,fp))
	{
		u32Succ = CVI_FAILURE;
	}

	return u32Succ;
}

CVI_S32 CVI_MemWriteFile(IVE_MEM_INFO_S *pstMem,FILE *fp)
{
	CVI_S32 u32Succ;
	CVI_U8 *pData;

	CVI_CHECK_ET_NULL_RET(pstMem,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	u32Succ = CVI_SUCCESS;

	pData = (CVI_U8*)pstMem->u64VirAddr;
	if (1 != fwrite(pData,pstMem->u32Size,1,fp))
	{
		u32Succ = CVI_FAILURE;
	}

	return u32Succ;
}

CVI_S32 CVI_DataReadFile(IVE_DATA_S *pstData,FILE *fp)
{
	CVI_S32 u32Succ;
	CVI_U8 *pData;
	CVI_U16 y;

	CVI_CHECK_ET_NULL_RET(pstData,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	u32Succ = CVI_SUCCESS;

	pData = (CVI_U8*)pstData->u64VirAddr;
	for (y = 0; y < pstData->u32Height; y++,pData += pstData->u32Stride)
	{
		if (1 != fread(pData,pstData->u32Width,1,fp))
		{
			u32Succ = CVI_FAILURE;
			break;
		}
	}

	return u32Succ;
}

CVI_S32 CVI_DataWriteFile(IVE_DATA_S *pstData,FILE *fp)
{
	CVI_S32 u32Succ;
	CVI_U8 *pData;
	CVI_U16 y;

	CVI_CHECK_ET_NULL_RET(pstData,CVI_FAILURE);
	CVI_CHECK_ET_NULL_RET(fp,CVI_FAILURE);

	u32Succ = CVI_SUCCESS;

	pData = (CVI_U8*)pstData->u64VirAddr;
	for (y = 0; y < pstData->u32Height; y++,pData += pstData->u32Stride)
	{
		if (1 != fwrite(pData,pstData->u32Width,1,fp))
		{
			u32Succ = CVI_FAILURE;
			break;
		}
	}

	return u32Succ;
}