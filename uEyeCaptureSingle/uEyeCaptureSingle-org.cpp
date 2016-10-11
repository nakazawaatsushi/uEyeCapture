// uEyeCaptureSingle.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//

#include "stdafx.h"
#include "uEye.h"
#include "ueye_deprecated.h"

#define MAXFRAMES	1000


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


int _tmain(int argc, _TCHAR* argv[])
{

	INT			nCaptured;
	INT			nRet, nSizeX, nSizeY, nBitsPerPixel, nColorMode;
	INT			lMemoryId;	// grabber memory - buffer ID
	INT			lMemoryId2[MAXFRAMES];
	char*		pcImageMemory;// grabber memory - pointer to buffer
	char*		pcImageMemoryBuf[MAXFRAMES];
	HIDS		hCam;

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
		is_AllocImageMem (	hCam,
							nSizeX,
							nSizeY,
							nBitsPerPixel,
							&pcImageMemoryBuf[i],
							&lMemoryId2[i]);
	}
	printf("Completed preparation. Bits PerPixel = %d\n", nBitsPerPixel);


	nCaptured = 0;

	for(int i = 0; i < 100; i++){
		is_SetImageMem (hCam, pcImageMemoryBuf[nCaptured], lMemoryId2[nCaptured]);	// set memory active
		if( is_FreezeVideo(hCam, IS_WAIT) == IS_SUCCESS ){
			printf("capture frame %d\n", i);
		//	is_CopyImageMem(hCam, pcImageMemory, lMemoryId, pcImageMemoryBuf[i]);
			nCaptured++;
		}
	}

	printf("nCaptured = %d\n", nCaptured);

	for(int i = 0; i < nCaptured; i++){
		wchar_t wfname[256];
		IMAGE_FILE_PARAMS	ImageFileParams;

		wsprintfW(wfname, L"%05d.bmp", i);
		ImageFileParams.pwchFileName = wfname;
		ImageFileParams.pnImageID    = NULL;
		ImageFileParams.ppcImageMem  = &pcImageMemoryBuf[i];
		ImageFileParams.nFileType    = IS_IMG_BMP;
		ImageFileParams.nQuality = 100;

		if( is_ImageFile(hCam, IS_IMAGE_FILE_CMD_SAVE, (void*)&ImageFileParams, sizeof(ImageFileParams)) == IS_SUCCESS ){
			wprintf(L"saved: %s\n", wfname);
		} else {
			wprintf(L"failed to save %s\n", wfname);
		}
	}

	return 0;
}

