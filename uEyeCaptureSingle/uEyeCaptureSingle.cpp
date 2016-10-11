// uEyeCaptureSingle.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
#pragma comment (lib, "winmm.lib")

#include "stdafx.h"

#include <stdio.h>
#include <time.h>
#include <windows.h>
#include <mmsystem.h>
#include <direct.h>

#include "uEye.h"
#include "ueye_deprecated.h"
#include "opencv2\opencv.hpp"
#include "opencv2\highgui\highgui.hpp"
#include <sstream>
#include <cstdlib>
#include <crtdbg.h>
#include <chrono>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#define MAXFRAMES	5
#define D_FRAMES	200 //保存するフレーム数
#define B_FRAMES	100 //imgBufの数

#define CAP_WAIT	10

INT InitCamera (HIDS *hCam, HWND hWnd)
{
	INT nRet = is_InitCamera (hCam, hWnd);	
	/************************************************************************************************/
	/*                                                                                              */
	/*  If the camera returns with "IS_STARTER_FW_UPLOAD_NEEDED", an upload of a new firmware       */
	/*  is necessary. This upload can take several seconds. We recommend to check the required      */
	/*  time with the function is_GetDuration().                                                    */
	/*                                                                                              */
	/*  In this case, the camera can only be opened if the flag "IS_ALLOW_STARTER_FW_UPLOAD"        */ 
	/*  is "OR"-ed to m_hCam. This flag allows an automatic upload of the firmware.                 */
	/*                                                                                              */                        
	/************************************************************************************************/
	if (nRet == IS_STARTER_FW_UPLOAD_NEEDED)
	{
		// Time for the firmware upload = 25 seconds by default
		INT nUploadTime = 25000;
		is_GetDuration (*hCam, IS_STARTER_FW_UPLOAD, &nUploadTime);

		//        CString Str1, Str2, Str3;
		//        Str1 = "This camera requires a new firmware. The upload will take about";
		//        Str2 = "seconds. Please wait ...";
		//        Str3.Format ("%s %d %s", Str1, nUploadTime / 1000, Str2);
		//        AfxMessageBox (Str3, MB_ICONWARNING);

		// Set mouse to hourglass
		//	    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));

		// Try again to open the camera. This time we allow the automatic upload of the firmware by
		// specifying "IS_ALLOW_STARTER_FIRMWARE_UPLOAD"
		*hCam = (HIDS) (((INT)*hCam) | IS_ALLOW_STARTER_FW_UPLOAD); 
		nRet = is_InitCamera (hCam, hWnd);   
	}

	return nRet;
}


void GetMaxImageSize(int m_hCam, INT *pnSizeX, INT *pnSizeY)
{
	// Check if the camera supports an arbitrary AOI
	// Only the ueye xs does not support an arbitrary AOI
	INT nAOISupported = 0;
	BOOL bAOISupported = TRUE;
	if (is_ImageFormat(m_hCam,
		IMGFRMT_CMD_GET_ARBITRARY_AOI_SUPPORTED, 
		(void*)&nAOISupported, 
		sizeof(nAOISupported)) == IS_SUCCESS)
	{
		bAOISupported = (nAOISupported != 0);
	}

	if (bAOISupported)
	{  
		// All other sensors
		// Get maximum image size
		SENSORINFO sInfo;
		is_GetSensorInfo (m_hCam, &sInfo);
		*pnSizeX = sInfo.nMaxWidth;
		*pnSizeY = sInfo.nMaxHeight;
	}
	else
	{
		// Only ueye xs
		// Get image size of the current format
		IS_SIZE_2D imageSize;
		is_AOI(m_hCam, IS_AOI_IMAGE_GET_SIZE, (void*)&imageSize, sizeof(imageSize));

		*pnSizeX = imageSize.s32Width;
		*pnSizeY = imageSize.s32Height;
	}
}

void cvtImage2opencv(char *camImage, IplImage *img, int nBitsPerPixel)
{
	int nSizeX = img->width;
	int nSizeY = img->height;

	if( img->nChannels == 3 ){
		// Color mode
		unsigned char *src, *dst;
		src = (unsigned char*)camImage;
		dst = (unsigned char*)img->imageData;

		// if color 24 bit
		if( nBitsPerPixel == 24 ){
			for( int i = 0; i < nSizeY; i++ ){
				for( int j=0; j < nSizeX; j++ ){
					*dst     = *(src);
					*(dst+1) = *(src+1);
					*(dst+2) = *(src+2);
					dst += 3;
					src += 3;
				}
			}
		}

		// if color 32 bit
		if( nBitsPerPixel == 32 ){
			for( int i = 0; i < nSizeY; i++ ){
				for( int j=0; j < nSizeX; j++ ){
					*dst     = *(src);
					*(dst+1) = *(src+1);
					*(dst+2) = *(src+2);
					dst += 3;
					src += 4;
				}
			}
		}
	} else {
		// Monocrome mode
		if( img->depth == IPL_DEPTH_16U )
			memcpy((unsigned char*)img->imageData, (unsigned char*)camImage, nSizeX*nSizeY*2);
		else if( img->depth == IPL_DEPTH_8U )
			memcpy((unsigned char*)img->imageData, (unsigned char*)camImage, nSizeX*nSizeY);
	}
}

void imEnhance(IplImage *dst, IplImage *src, int level)
{
	if( dst->depth != IPL_DEPTH_16U )
		return;

	// Color mode
	unsigned short *psrc, *pdst;

	psrc = (unsigned short*)src->imageData;
	pdst = (unsigned short*)dst->imageData;

	// if color
	for( int i = 0; i < dst->height; i++ ){
		for( int j=0; j < dst->width; j++ ){
			*pdst     = (*psrc << level);
			pdst ++;
			psrc ++;
		}
	}
}

int getCameraInfo(UEYE_CAMERA_LIST* pucl, std::string* c_type, int i)
{
	INT nNumCam;
	std::string str;
	if( is_GetNumberOfCameras( &nNumCam ) == IS_SUCCESS) {
		if( nNumCam >= 1 ) {
			// 適合するサイズで新規リストを作成

			pucl->dwCount = nNumCam;

			//カメラ情報の取得
			if (is_GetCameraList(pucl) == IS_SUCCESS) {

				//画面にカメラ情報をテスト出力

				if(strcmp(pucl->uci[i].SerNo, "4102622403") == 0){

					*c_type = "Eye";

				} else if(strcmp(pucl->uci[i].SerNo , "4102615093") == 0){
					*c_type = "Scene";
				} else{
					printf("Unknown Camera\n");
				}

				printf("Camera %i Id:%d, Type:%s\n", i,
					pucl->uci[i].dwCameraID, (*c_type).c_str());

			}
		}
	}
	return nNumCam;

}

void cvSaveImageSub(const char path[256], const IplImage *ImageBuf, const int nCaptured)
{
	char fnamebuff[256];

	sprintf(fnamebuff,"%s\\%04d.png",path, nCaptured);
	cvSaveImage(fnamebuff,ImageBuf);
}

int main(int argc, char * argv[])
{
	INT			nCaptured = 0;
	INT			nRet, nSizeX, nSizeY, nBitsPerPixel, nColorMode,nSizeX2, nSizeY2, nBitsPerPixel2, nColorMode2;
	INT			lMemoryId[MAXFRAMES],lMemoryId2[MAXFRAMES];
	INT			nMemID, nMemID2;
	char*		pcImageMemory[MAXFRAMES];
	char*		pcImageMemory2[MAXFRAMES];
	char		*pBuffer, *pBuffer2;
	HIDS		hCam, hCam2;
	IplImage    *imgBuf[B_FRAMES],*imgBuf2[B_FRAMES];
	IplImage	*view,*view2;
	time_t		ti, ti2;
	DWORD		sTime,fTime;
	int			timelist[30*60*20];//max: 30*60*20frame 
	struct tm	*lt;
	UEYE_CAMERA_LIST *pucl;
	INT			f_disp = 1;
	SYSTEMTIME shoot_timelist[30*60*20];
	std::string			c_type, c_type2;
	boost::thread_group thr_grp;

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	int	nCam; 
	is_GetNumberOfCameras( &nCam );//接続されているカメラの数

	if( nCam == 0 ){
		printf("No camera found. exit.\n");
		exit(0);
	} else if(nCam == 1){
		printf("Single Camera Mode\n");
	} else if(nCam > 2){
		printf("Multi Camera Mode\n");
	}

	// Open First Camera
	pucl = (UEYE_CAMERA_LIST*) new BYTE [sizeof (DWORD) + nCam * sizeof (UEYE_CAMERA_INFO)];
	getCameraInfo(pucl, &c_type, 0);	
	
	hCam = 0;
	if( InitCamera(&hCam, NULL) != IS_SUCCESS ){
		printf("Cannot open camera %d\n", hCam);
	} else {
		printf("Camera Open %d success.\n", hCam);
	}

	GetMaxImageSize(hCam, &nSizeX, &nSizeY);
	printf("Xsize, Ysize = %d %d\n", nSizeX, nSizeY);

	// ファイルからパラメータをロードする (ファイルボックスを開く)
	printf("Read parameter for %s Camera\n",c_type.c_str());
	nRet = is_ParameterSet(hCam, IS_PARAMETERSET_CMD_LOAD_FILE, NULL, NULL);

	// カラーモードをセットする
	switch( is_SetColorMode( hCam, IS_GET_COLOR_MODE ) )
	{
        case IS_CM_RGBA12_UNPACKED:
        case IS_CM_BGRA12_UNPACKED:
            nBitsPerPixel = 64;
		break;

        case IS_CM_RGB12_UNPACKED:
        case IS_CM_BGR12_UNPACKED:
        case IS_CM_RGB10_UNPACKED:
        case IS_CM_BGR10_UNPACKED:
			nBitsPerPixel = 48;
		break;

        case IS_CM_RGBA8_PACKED:
		case IS_CM_BGRA8_PACKED:
        case IS_CM_RGB10_PACKED:
		case IS_CM_BGR10_PACKED:
        case IS_CM_RGBY8_PACKED:
        case IS_CM_BGRY8_PACKED:
			nBitsPerPixel = 32;
		break;
			
        case IS_CM_RGB8_PACKED:
        case IS_CM_BGR8_PACKED:
            nBitsPerPixel = 24;
		break;
			
        case IS_CM_BGR565_PACKED:
		case IS_CM_UYVY_PACKED:
        case IS_CM_CBYCRY_PACKED:
			nBitsPerPixel = 16;
		break;

		case IS_CM_BGR5_PACKED:
			nBitsPerPixel = 15;
		break;

        case IS_CM_MONO16:
		case IS_CM_SENSOR_RAW16:
		case IS_CM_MONO12:
		case IS_CM_SENSOR_RAW12:
		case IS_CM_MONO10:
		case IS_CM_SENSOR_RAW10:
			nBitsPerPixel = 16;
		break;

        case IS_CM_RGB8_PLANAR:
			nBitsPerPixel = 24;
		break;

        case IS_CM_MONO8:
		case IS_CM_SENSOR_RAW8:
		default:
			nBitsPerPixel = 8;
		break;
	}

	printf("Completed preparation of first camera. Bits PerPixel = %d\n", nBitsPerPixel);
	if( !(nBitsPerPixel == 8 || nBitsPerPixel == 16 || nBitsPerPixel == 24) ){
		printf("This program supports only 8bits mono, 16bits bayer raw or 24bit RGB images\n");
		exit(0);
	}

	// allocate image memory
	for( int i = 0; i < MAXFRAMES; i++ ){
		is_AllocImageMem (	hCam,
			nSizeX,
			nSizeY,
			nBitsPerPixel,
			&pcImageMemory[i],
			&lMemoryId[i]);
		is_AddToSequence( hCam, pcImageMemory[i], lMemoryId[i] );
	}

	is_InitImageQueue( hCam, 0 );

	// allocate image memory of opencv
	for(int i = 0; i < B_FRAMES; i++){
		if( nBitsPerPixel == 8 )
			// 8 bit mono mode
			imgBuf[i] = cvCreateImage(cvSize(nSizeX,nSizeY), IPL_DEPTH_8U, 1);
		else if( nBitsPerPixel == 16 ){
			// 16 bit mono mode
			imgBuf[i] = cvCreateImage(cvSize(nSizeX,nSizeY), IPL_DEPTH_16U, 1);
			if( i == 0 )
				view = cvCreateImage(cvSize(nSizeX,nSizeY), IPL_DEPTH_16U, 1);
		} else if( nBitsPerPixel == 24 )
			// 24 bit color mode
			imgBuf[i] = cvCreateImage(cvSize(nSizeX,nSizeY), IPL_DEPTH_8U, 3);
	}

	// open opencv window
	cv::namedWindow("Camera Input",1);
	is_CaptureVideo( hCam, IS_WAIT );

	printf("now showing the image, 's' to start to capture, [ESC] to exit.\n");

	while(1){
		if(is_WaitForNextImage(hCam, CAP_WAIT, &pBuffer, &nMemID) == IS_SUCCESS){
			is_UnlockSeqBuf( hCam, nMemID, pBuffer);
			cvtImage2opencv(pBuffer, imgBuf[0], nBitsPerPixel);

			if( nBitsPerPixel == 16 ){
				// Bayer Camera
				imEnhance(view, imgBuf[0], 4);
				cvShowImage("Camera Input", view);
			} else if( nBitsPerPixel == 24 ){
				cvShowImage("Camera Input", imgBuf[0]);	
			}
		}
		
		unsigned char k = cvWaitKey(1);
				
		if( k == 0x1b ){
			goto EXIT;
		} else if( k == 's' ){
			break;
		}
	}

	char path[256], fname[256];

	printf("now start to capture. [ESC] to quit. \n");

	time(&ti);
	lt = localtime(&ti);

	// create directory
	sprintf(path,"%02d%02d_%02d%02d%02d", lt->tm_mon+1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);
	mkdir(path);

	while(1){
		int key;

		for( int i = 0; i < B_FRAMES; i++ ){
			if(is_WaitForNextImage(hCam, CAP_WAIT, &pBuffer, &nMemID) == IS_SUCCESS){
				sTime = timeGetTime();
				GetLocalTime(&shoot_timelist[nCaptured]);
				cvtImage2opencv(pBuffer, imgBuf[i], nBitsPerPixel);
				is_UnlockSeqBuf( hCam, nMemID, pBuffer);
			} else {
				continue;
			}

			thr_grp.create_thread(boost::bind(&cvSaveImageSub, path, imgBuf[i], nCaptured));

			// @show Image
			if((nCaptured % 5) == 0){
				// in case 12 bit image, increase the intensity to 16 for visualization
				if( nBitsPerPixel == 16 ){
					imEnhance(view, imgBuf[i], 4);
					cvShowImage("Camera Input", view);
				} else {
					cvShowImage("Camera Input", imgBuf[i]);
				}
				time(&ti2);
				printf("%d frames takes %d seconds.\n", nCaptured, ti2-ti);
			}

			key = cvWaitKey(1);
			if( key == 0x1b ){
				thr_grp.join_all();
				break;
			}

			nCaptured++;
		}

		if( key == 0x1b )
			break;
	}

	printf("Finish\nCamera%d:%d Captured\n",hCam, nCaptured);

	//画像の取得時刻(ms)をファイルに出力
	char fname_tx[256];
	FILE *outputfilex;
	sprintf(fname_tx,"%s\\timex.csv",path);
	outputfilex = fopen(fname_tx,"w");
	if(outputfilex == NULL){
		printf("cannot make time.csv\n");
	}
		for(int i =0; i < nCaptured; i++){
			fprintf_s(outputfilex,"%d,%02d%02d%02d%02d%02d%04d\n",i,shoot_timelist[i].wMonth,shoot_timelist[i].wDay,shoot_timelist[i].wHour,shoot_timelist[i].wMinute,shoot_timelist[i].wSecond,shoot_timelist[i].wMilliseconds);//
	}
	fclose(outputfilex);

	//各フレームにかかった時間をファイルに出力
	char	fname_t[256];
	FILE	*outputfile;
	sprintf(fname_t,"%s\\time.csv",path);
	outputfile = fopen(fname_t,"w");
	if(outputfile == NULL){
		printf("cannot make time.csv\n");
	}
	for(int i =0; i < nCaptured; i++){
		fprintf(outputfile,"%d,",timelist[i]);
	}
	fclose(outputfile);

	delete pucl;
	for(int i = 0; i < B_FRAMES; i++){
		cvReleaseImage(&imgBuf[i]);
	}
	for( int i = 0; i < MAXFRAMES; i++ ){
		is_FreeImageMem (	hCam,
		pcImageMemory[i],
		lMemoryId[i]);
	}

	cvReleaseImage(&view);

EXIT:
	is_ExitImageQueue (hCam);
	is_ExitCamera(hCam);

	if(nCam == 2){

		for(int i = 0; i < B_FRAMES; i++){
			cvReleaseImage(&imgBuf2[i]);
		}
		
		for( int i = 0; i < MAXFRAMES; i++ ){
			is_FreeImageMem (	hCam2,
				pcImageMemory2[i],
				lMemoryId2[i]);
		}
		is_ExitImageQueue (hCam2);
		is_ExitCamera(hCam2);
		cvReleaseImage(&view2);
	}

	return 0;
}
