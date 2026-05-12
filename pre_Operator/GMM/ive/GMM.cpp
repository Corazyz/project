#include "GMM.h"
#include "sample_assist.h"
#include "sample_file.h"
#include "sample_define.h"
// #include "mpi_ive.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

bool directoryExists(const char* path) {
    #ifdef _WIN32
        struct _stat info;
        return (_stat(path, &info) == 0 && (info.st_mode & _S_IFDIR));
    #else
        struct stat info;
        return (stat(path, &info) == 0 && (info.st_mode & S_IFDIR));
    #endif
}

void createDirectory(const char* path) {
    if (!directoryExists(path)) {
        MKDIR(path);
    }
}

using namespace std;
using namespace cv;

enum {
	CVGRAY_COMMON_U8C1 = 0,
	CVBGR_COMMON_U8C3PACKAGE = 1,
	COMMON_U8C1_CVGRAY = 3,
	COMMON_U8C3PACKAGE_CVBGR = 4
};

typedef struct _COMMON_MEM_INFO_S {
	uint8_t* pData;
	uint32_t u32Size;
} COMMON_MEM_INFO_S;

typedef struct _COMMON_GMM_CTRL_S {
	uint16_t u0q16LearnRate;
	uint16_t u0q16BgRatio;
	uint16_t u8q8VarThr;
	uint32_t u22q10NoiseVar;
	uint32_t u22q10MaxVar;
	uint32_t u22q10MinVar;
	uint16_t u0q16InitWeight;
	uint8_t  u8ModelNum;
} COMMON_GMM_CTRL_S;

static int Common_GetChannels(IVE_IMAGE_TYPE_E enType)
{
	return (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? 3 : 1;
}

static int32_t Common_CreateImage(COMMON_IMAGE_S* pstImg, IVE_IMAGE_TYPE_E enType, uint32_t u32Width, uint32_t u32Height)
{
	int ch = (enType == IVE_IMAGE_TYPE_U8C3_PACKAGE) ? 3 : 1;
	uint32_t u32Stride = u32Width;
	pstImg->au64VirAddr[0] = (uint64_t)malloc(u32Stride * ch * u32Height);
	if (!pstImg->au64VirAddr[0]) return CVI_FAILURE;
	memset((void*)pstImg->au64VirAddr[0], 0, u32Stride * ch * u32Height);
	pstImg->au64PhyAddr[0] = 0;
	pstImg->au32Stride[0] = u32Stride;
	pstImg->u32Width = u32Width;
	pstImg->u32Height = u32Height;
	pstImg->enType = enType;
	pstImg->au64PhyAddr[1] = pstImg->au64PhyAddr[2] = 0;
	pstImg->au64VirAddr[1] = pstImg->au64VirAddr[2] = 0;
	pstImg->au32Stride[1] = pstImg->au32Stride[2] = 0;
	return CVI_SUCCESS;
}

static void Common_DestroyImage(COMMON_IMAGE_S* pstImg)
{
	if (pstImg->au64VirAddr[0]) {
		free((void*)pstImg->au64VirAddr[0]);
		pstImg->au64VirAddr[0] = 0;
	}
}

static int32_t Common_CreateMem(COMMON_MEM_INFO_S* pstMem, uint32_t u32Size)
{
	pstMem->pData = (uint8_t*)malloc(u32Size);
	if (!pstMem->pData) return CVI_FAILURE;
	memset(pstMem->pData, 0, u32Size);
	pstMem->u32Size = u32Size;
	return CVI_SUCCESS;
}

static void Common_DestroyMem(COMMON_MEM_INFO_S* pstMem)
{
	if (pstMem->pData) {
		free(pstMem->pData);
		pstMem->pData = NULL;
	}
}

static int32_t CVI_Common_GMM(COMMON_IMAGE_S *pstSrc, COMMON_IMAGE_S *pstFg,
	COMMON_IMAGE_S *pstBg, COMMON_MEM_INFO_S *pstModel, COMMON_GMM_CTRL_S *pstCtrl)
{
	return CVI_MPI_IVE_GMM(
			(uint8_t*)pstSrc->au64VirAddr[0], (uint16_t)pstSrc->au32Stride[0],
			(uint16_t)pstSrc->u32Width, (uint16_t)pstSrc->u32Height,
			(uint8_t*)pstFg->au64VirAddr[0], (uint16_t)pstFg->au32Stride[0],
			(uint8_t*)pstBg->au64VirAddr[0], (uint16_t)pstBg->au32Stride[0],
			pstModel->pData, pstCtrl->u8ModelNum,
			pstCtrl->u0q16LearnRate, pstCtrl->u0q16BgRatio, pstCtrl->u8q8VarThr,
			pstCtrl->u22q10NoiseVar, pstCtrl->u22q10MaxVar, pstCtrl->u22q10MinVar,
			pstCtrl->u0q16InitWeight,
			(uint8_t)(Common_GetChannels(pstSrc->enType) == 3 ? 1 : 0));
}

static bool Common_CompareImage(COMMON_IMAGE_S *pstImg, const char* fileName)
{
	FILE *fp;
	int32_t open = fopen_s(&fp, fileName, "rb");
	if (open != CVI_SUCCESS) {
		printf("%s not exist\n", fileName);
		return false;
	}

	COMMON_IMAGE_S stRef;
	Common_CreateImage(&stRef, pstImg->enType, pstImg->u32Width, pstImg->u32Height);
	CVI_ReadFile(&stRef, fp);
	CVI_FCLOSE(fp);

	int ch = Common_GetChannels(pstImg->enType);
	uint32_t strideBytesA = pstImg->au32Stride[0] * ch;
	uint32_t strideBytesB = stRef.au32Stride[0] * ch;
	bool match = true;

	for (uint32_t y = 0; y < pstImg->u32Height; y++) {
		uint8_t *pA = (uint8_t*)(pstImg->au64VirAddr[0] + y * strideBytesA);
		uint8_t *pB = (uint8_t*)(stRef.au64VirAddr[0] + y * strideBytesB);
		if (memcmp(pA, pB, pstImg->u32Width * ch) != 0) {
			match = false;
			for (uint32_t x = 0; x < pstImg->u32Width * ch; x++) {
				if (pA[x] != pB[x]) {
					printf("  first diff at y=%u x=%u A=0x%02x B=0x%02x\n", y, x, pA[x], pB[x]);
					break;
				}
			}
			break;
		}
	}
	Common_DestroyImage(&stRef);

	if (match) {
		printf("Compare against %s passed\n", fileName);
		return false;
	} else {
		printf("Compare against %s failed\n", fileName);
		return true;
	}
}

static void mat2CommonImg(Mat *src, COMMON_IMAGE_S *pstDst, int type)
{
	if(src->cols != (int)pstDst->u32Width || src->rows != (int)pstDst->u32Height) {
		printf("width or height didn't equal, line:%d\n",__LINE__);
	}

	int ch = Common_GetChannels(pstDst->enType);
	uint8_t *pBase = (uint8_t*)pstDst->au64VirAddr[0];
	uint32_t strideBytes = pstDst->au32Stride[0] * ch;

	switch(type) {
	case CVGRAY_COMMON_U8C1:
		for(int r=0; r<src->rows; r++) {
			uint8_t *ptm = src->ptr(r);
			uint8_t *pti = pBase + r * strideBytes;
			memcpy(pti, ptm, pstDst->u32Width);
		}
		break;
	case CVBGR_COMMON_U8C3PACKAGE:
		for(int r=0; r<src->rows; r++) {
			uint8_t *ptm = src->ptr(r);
			uint8_t *pti = pBase + r * strideBytes;
			memcpy(pti, ptm, pstDst->u32Width * 3);
		}
		break;
	default:
		printf("not support type for mat2CommonImg!\n");
	}
}

static void commonImg2Mat(COMMON_IMAGE_S *pstSrc, Mat *dst, int type)
{
	if(dst->cols != (int)pstSrc->u32Width || dst->rows != (int)pstSrc->u32Height) {
		printf("width or height didn't equal,line:%d\n",__LINE__);
		return;
	}

	int ch = Common_GetChannels(pstSrc->enType);
	uint8_t *pBase = (uint8_t*)pstSrc->au64VirAddr[0];
	uint32_t strideBytes = pstSrc->au32Stride[0] * ch;

	switch(type) {
	case COMMON_U8C1_CVGRAY:
		for(int r=0; r<dst->rows; r++) {
			uint8_t *ptm = dst->ptr(r);
			uint8_t *pti = pBase + r * strideBytes;
			memcpy(ptm, pti, dst->cols);
		}
		break;
	case COMMON_U8C3PACKAGE_CVBGR:
		for(int r=0; r<dst->rows; r++) {
			uint8_t *ptm = dst->ptr(r);
			uint8_t *pti = pBase + r * strideBytes;
			memcpy(ptm, pti, dst->cols * 3);
		}
		break;
	default:
		printf("not support type for commonImg2Mat!\n");
	}
}

int32_t GMM_Sample_U8C3_PACKAGE(int show, int compare)
{
	int32_t  s32Ret = CVI_SUCCESS;
	int32_t  s32CompareError = 0;
	const char *pchAvi = "./data/avi/campus.avi";
	const char *pchRes = "./data/avi/GMM_Sample_U8C3_PACKAGE.avi";

	COMMON_IMAGE_S stCommImg, stCommFg, stCommBg;
	COMMON_MEM_INFO_S stModel;
	COMMON_GMM_CTRL_S stCtrl;
	uint16_t u16Width, u16Height;
	int32_t s32FrmCnt = 0;

	Mat cvImg, cvFg, cvBg, cvFgBGR, cvDispImg;
	VideoCapture cvCap;
	VideoWriter  cvWte;
	cv::VideoWriter videoWriter;
	bool isVideoInitialized = false;
	int cvWidth, cvHeight, cvGap;
	Size cvSz;
    int fourcc;

	cvCap.open(pchAvi);
	cvWidth  = (int32_t)cvCap.get(cv::CAP_PROP_FRAME_WIDTH);
	cvHeight = (int32_t)cvCap.get(cv::CAP_PROP_FRAME_HEIGHT);

	u16Width  = cvWidth & (~1);
	u16Height = cvHeight & (~1);

	stCtrl.u0q16BgRatio		= 45875;
	stCtrl.u0q16InitWeight	= 3277;
	stCtrl.u22q10NoiseVar	= 225 * 3 * 1024;
	stCtrl.u22q10MaxVar		= 3 * 4000 * 1024;
	stCtrl.u22q10MinVar		= 600 * 1024;
	stCtrl.u8q8VarThr		= (uint16_t)(256 * 6.25);
	stCtrl.u8ModelNum		= 3;

	if (Common_CreateImage(&stCommImg, IVE_IMAGE_TYPE_U8C3_PACKAGE, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_0;
	if (Common_CreateImage(&stCommBg, IVE_IMAGE_TYPE_U8C3_PACKAGE, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_1;
	if (Common_CreateImage(&stCommFg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_2;

	cvBg = Mat::zeros(u16Height, u16Width, CV_8UC3);
	cvFg    = Mat::zeros(u16Height, u16Width, CV_8UC1);
	cvFgBGR = Mat::zeros(u16Height, u16Width, CV_8UC3);

	if (Common_CreateMem(&stModel, stCtrl.u8ModelNum * 11 * u16Width * u16Height) != CVI_SUCCESS)goto FAIL_3;

	cvGap = 10;
	cvSz.width  = stCommImg.u32Width  + stCommBg.u32Width  + stCommFg.u32Width  + 4 * cvGap;
	cvSz.height = max(max(stCommImg.u32Height, stCommBg.u32Height), stCommFg.u32Height) + 2 * cvGap;
    fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
    cvWte.open(pchRes, fourcc, cvCap.get(cv::CAP_PROP_FPS), cvSz);

	for(;;) {
        s32FrmCnt = (int32_t)cvCap.get(cv::CAP_PROP_POS_FRAMES);
        cvCap >> cvImg;
        if(cvImg.empty()) break;

        if(cvImg.rows != (int)stCommImg.u32Height || cvImg.cols != (int)stCommImg.u32Width)
            resize(cvImg, cvImg, Size(stCommImg.u32Width, stCommImg.u32Height));

        mat2CommonImg(&cvImg, &stCommImg, CVBGR_COMMON_U8C3PACKAGE);

        stCtrl.u0q16LearnRate = (s32FrmCnt >= 500) ? 131 : (65535/(s32FrmCnt+1));

        s32Ret = CVI_Common_GMM(&stCommImg, &stCommFg, &stCommBg, &stModel, &stCtrl);
        if (s32Ret != CVI_SUCCESS) goto FAIL_4;

        if (compare && (s32FrmCnt == 0 || s32FrmCnt == 1 || s32FrmCnt == 512)) {
            char fileName[_MAX_FNAME];

            snprintf(fileName, _MAX_FNAME, "./data/result/sample_GMM_U8C3_PACKAGE_bg_%d.rgb", s32FrmCnt);
            if (Common_CompareImage(&stCommBg, fileName))
                s32CompareError++;

            snprintf(fileName, _MAX_FNAME, "./data/result/sample_GMM_U8C3_PACKAGE_fg_%d.yuv", s32FrmCnt);
            if (Common_CompareImage(&stCommFg, fileName))
                s32CompareError++;
        }

        if (show) {
            commonImg2Mat(&stCommBg, &cvBg, COMMON_U8C3PACKAGE_CVBGR);
            commonImg2Mat(&stCommFg, &cvFg, COMMON_U8C1_CVGRAY);
            cvtColor(cvFg, cvFgBGR, cv::COLOR_GRAY2BGR);

            hconcat(cvImg, cvBg, cvDispImg);
            hconcat(cvDispImg, cvFgBGR, cvDispImg);

            putText(cvDispImg, "srcImg", Point(5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));
            putText(cvDispImg, "bgImg", Point(cvImg.cols + 5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));
            putText(cvDispImg, "fgImg", Point(cvImg.cols + cvBg.cols + 5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));

            if (!isVideoInitialized && cvDispImg.rows > 0 && cvDispImg.cols > 0) {
                const char* videoPath = "./data/result/output_video.mp4";
                int vfourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
                double fps = 25.0;
                cv::Size frameSize(cvDispImg.cols, cvDispImg.rows);
                videoWriter.open(videoPath, vfourcc, fps, frameSize);
                if (videoWriter.isOpened()) isVideoInitialized = true;
            }

            if (isVideoInitialized) videoWriter.write(cvDispImg);

            char filename[256];
            snprintf(filename, sizeof(filename), "./data/result/frame/frame_%06d.png", s32FrmCnt);
            createDirectory("./data/result/frame/");
            imwrite(filename, cvDispImg);
        }
	}

FAIL_4:
    cvWte.release();
    if (isVideoInitialized) { videoWriter.release(); }
FAIL_3:
    Common_DestroyMem(&stModel);
FAIL_2:
    Common_DestroyImage(&stCommFg);
FAIL_1:
    Common_DestroyImage(&stCommBg);
FAIL_0:
    Common_DestroyImage(&stCommImg);
    destroyAllWindows();
    return (s32CompareError == 0) ? s32Ret : CVI_FAILURE;
}

int32_t GMM_Sample_U8C1(int show, int compare)
{
	int32_t  s32Ret = CVI_SUCCESS;
	int32_t s32CompareError = 0;
	const char *pchAvi = "./data/avi/campus.avi";
	const char *pchRes = "./data/avi/GMM_Sample_U8C1.avi";

	COMMON_IMAGE_S stCommImg, stCommFg, stCommBg;
	COMMON_MEM_INFO_S stModel;
	COMMON_GMM_CTRL_S stCtrl;
	uint16_t u16Width, u16Height;
	int32_t s32FrmCnt = 0;

	Mat cvImg, cvFg, cvBg, cvImgGray, cvFgBGR, cvBgBGR, cvDispImg;
	VideoCapture cvCap;
	VideoWriter  cvWte;
	cv::VideoWriter videoWriter;
	bool isVideoInitialized = false;
	int cvWidth, cvHeight, cvGap;
	Size cvSz;
    int fourcc;

	cvCap.open(pchAvi);
	cvWidth  = (int32_t)cvCap.get(cv::CAP_PROP_FRAME_WIDTH);
	cvHeight = (int32_t)cvCap.get(cv::CAP_PROP_FRAME_HEIGHT);

	u16Width  = cvWidth & (~1);
	u16Height = cvHeight & (~1);

	stCtrl.u0q16BgRatio		= 45875;
	stCtrl.u0q16InitWeight	= 3277;
	stCtrl.u22q10NoiseVar	= 225 * 1024;
	stCtrl.u22q10MaxVar		= 2000 * 1024;
	stCtrl.u22q10MinVar		= 200 * 1024;
	stCtrl.u8q8VarThr		= (uint16_t)(256 * 6.25);
	stCtrl.u8ModelNum		= 3;

	if (Common_CreateImage(&stCommImg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_U8C1_0;
	if (Common_CreateImage(&stCommBg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_U8C1_1;
	if (Common_CreateImage(&stCommFg, IVE_IMAGE_TYPE_U8C1, u16Width, u16Height) != CVI_SUCCESS)goto FAIL_U8C1_2;

	cvBg	= Mat::zeros(u16Height, u16Width, CV_8UC1);
	cvBgBGR = Mat::zeros(u16Height, u16Width, CV_8UC3);
	cvFg    = Mat::zeros(u16Height, u16Width, CV_8UC1);
	cvFgBGR = Mat::zeros(u16Height, u16Width, CV_8UC3);

	if (Common_CreateMem(&stModel, stCtrl.u8ModelNum * 7 * u16Width * u16Height) != CVI_SUCCESS)goto FAIL_U8C1_3;

	cvGap = 10;
	cvSz.width  = stCommImg.u32Width  + stCommBg.u32Width  + stCommFg.u32Width  + 4 * cvGap;
	cvSz.height = max(max(stCommImg.u32Height, stCommBg.u32Height), stCommFg.u32Height) + 2 * cvGap;
    fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
    cvWte.open(pchRes, fourcc, cvCap.get(cv::CAP_PROP_FPS), cvSz);

	for(;;) {
		s32FrmCnt = (int32_t)cvCap.get(cv::CAP_PROP_POS_FRAMES);
		cvCap >> cvImg;
		if(cvImg.empty()) break;

		if(cvImg.rows != (int)stCommImg.u32Height || cvImg.cols != (int)stCommImg.u32Width)
			resize(cvImg, cvImg, Size(stCommImg.u32Width, stCommImg.u32Height));

		cvtColor(cvImg, cvImgGray, cv::COLOR_BGR2GRAY);
		mat2CommonImg(&cvImgGray, &stCommImg, CVGRAY_COMMON_U8C1);

		stCtrl.u0q16LearnRate = (s32FrmCnt >= 500) ? 131 : (65535/(s32FrmCnt+1));

		s32Ret = CVI_Common_GMM(&stCommImg, &stCommFg, &stCommBg, &stModel, &stCtrl);
		if (s32Ret != CVI_SUCCESS) goto FAIL_U8C1_4;

		if (compare && (s32FrmCnt == 0 || s32FrmCnt == 1 || s32FrmCnt == 512)) {
			char fileName[_MAX_FNAME];

			snprintf(fileName, _MAX_FNAME, "./data/result/sample_GMM_U8C1_bg_%d.yuv", s32FrmCnt);
			if (Common_CompareImage(&stCommBg, fileName))
				s32CompareError++;

			snprintf(fileName, _MAX_FNAME, "./data/result/sample_GMM_U8C1_fg_%d.yuv", s32FrmCnt);
			if (Common_CompareImage(&stCommFg, fileName))
				s32CompareError++;
		}

		if (show) {
            commonImg2Mat(&stCommBg, &cvBg, COMMON_U8C1_CVGRAY);
            commonImg2Mat(&stCommFg, &cvFg, COMMON_U8C1_CVGRAY);
            cvtColor(cvBg, cvBgBGR, cv::COLOR_GRAY2BGR);
            cvtColor(cvFg, cvFgBGR, cv::COLOR_GRAY2BGR);

            hconcat(cvImg, cvBgBGR, cvDispImg);
            hconcat(cvDispImg, cvFgBGR, cvDispImg);

            putText(cvDispImg, "srcImg", Point(5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));
            putText(cvDispImg, "bgImg", Point(cvImg.cols + 5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));
            putText(cvDispImg, "fgImg", Point(cvImg.cols + cvBgBGR.cols + 5,15), FONT_HERSHEY_COMPLEX_SMALL, 1, CV_RGB(255,0,0));

            if (!isVideoInitialized && cvDispImg.rows > 0 && cvDispImg.cols > 0) {
                const char* videoPath = "./data/result/output_video_u8c1.mp4";
                int vfourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
                double fps = 25.0;
                cv::Size frameSize(cvDispImg.cols, cvDispImg.rows);
                videoWriter.open(videoPath, vfourcc, fps, frameSize);
                if (videoWriter.isOpened()) isVideoInitialized = true;
            }

            if (isVideoInitialized) videoWriter.write(cvDispImg);

            char filename[256];
            snprintf(filename, sizeof(filename), "./data/result/frame/frame_%06d.png", s32FrmCnt);
            createDirectory("./data/result/frame/");
            imwrite(filename, cvDispImg);
        }
	}

FAIL_U8C1_4:
    cvWte.release();
    if (isVideoInitialized) { videoWriter.release(); std::cout << "U8C1 Video file saved successfully." << std::endl; }
FAIL_U8C1_3:
    Common_DestroyMem(&stModel);
FAIL_U8C1_2:
    Common_DestroyImage(&stCommFg);
FAIL_U8C1_1:
    Common_DestroyImage(&stCommBg);
FAIL_U8C1_0:
    Common_DestroyImage(&stCommImg);
    destroyAllWindows();
    return (s32CompareError == 0) ? s32Ret : CVI_FAILURE;
}
