// uEyeCaptureSingle.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"

#include <stdio.h>
#include <time.h>

#include "uEye.h"
#include "ueye_deprecated.h"
#include "opencv2\opencv.hpp"
#include "opencv2\highgui\highgui.hpp"


#define MAXFRAMES	100


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

void cvtImage2opencv(char *camImage, IplImage *img)
{
	unsigned char *src, *dst;
	int nSizeX = img->width;
	int nSizeY = img->height;

	src = (unsigned char*)camImage;
	dst = (unsigned char*)img->imageData;

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


int main(int argc, char * argv[])
{
	INT			nCaptured;
	INT			nRet, nSizeX, nSizeY, nBitsPerPixel, nColorMode;
	INT			lMemoryId;	// grabber memory - buffer ID
	INT			lMemoryId2[MAXFRAMES];
	char*		pcImageMemory;// grabber memory - pointer to buffer
	char*		pcImageMemoryBuf[MAXFRAMES];
	HIDS		hCam;
	IplImage    *img;
	time_t		ti, ti2;
	int			f_disp = 1;

	// check arguments
	if( argc == 2 ){
		if( strcmp(argv[1],"-nodisp") == 0 )
		{
			// no-display mode
			printf("no display mode.\n");
			f_disp = 0;
		}
	}

	// Open First Camera
	hCam = 1;
	nRet = InitCamera(&hCam, NULL);

	if( nRet != IS_SUCCESS ){
		printf("Cannot open camera\n");
	}

	printf("Camera Open %d success.\n", hCam);

	GetMaxImageSize(hCam, &nSizeX, &nSizeY);
	printf("Xsize, Ysize = %d %d\n", nSizeX, nSizeY);

	// setup the color depth to the current windows setting
	is_GetColorDepth (hCam, &nBitsPerPixel, &nColorMode);
	is_SetColorMode (hCam, nColorMode);

    // memory initialization
	is_AllocImageMem (	hCam,
						nSizeX,
						nSizeY,
						nBitsPerPixel,
						&pcImageMemory,
						&lMemoryId);
	is_SetImageMem (hCam, pcImageMemory, lMemoryId);	// set memory active

	// allocate image memory
	for( int i = 0; i < MAXFRAMES; i++ ){
		pcImageMemoryBuf[i] = (char*)malloc(nSizeX*nSizeY*nBitsPerPixel/8 );
		is_SetAllocatedImageMem (	hCam,
							nSizeX,
							nSizeY,
							nBitsPerPixel,
							pcImageMemoryBuf[i],
							&lMemoryId2[i]);
	}
	printf("Completed preparation. Bits PerPixel = %d\n", nBitsPerPixel);

	// allocate opencv image
	img = cvCreateImage(cvSize(nSizeX,nSizeY), IPL_DEPTH_8U, 3);

	// open opencv window
	if( f_disp ){
		cv::namedWindow("Camera Input", CV_WINDOW_AUTOSIZE|CV_WINDOW_FREERATIO);
	}

	nCaptured = 0;

	printf("[ESC] to exit.\n");
	time(&ti);

	for(;;){
		is_FreezeVideo(hCam, IS_WAIT);

		if( f_disp ){
			cvtImage2opencv(pcImageMemory, img);
			cvShowImage("Camera Input", img);
		}

		if( cvWaitKey(10) == 0x1b ){
			break;
		}

		nCaptured++;

		if( (nCaptured % 100) == 0 ){
			time(&ti2);
			printf("%d frames takes %d seconds. %d fps\n", nCaptured, ti2-ti);
		}
	}

	printf("nCaptured = %d\n", nCaptured);

	return 0;
}

