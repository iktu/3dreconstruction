#include <windows.h>
#include "NuiImpl.h"
#include <stdio.h>
#include <assert.h>
#include "KinectDefine.h"
#include <float.h>

//全局外部变量
///////////////////////
bool g_SkeletonAvaiable = false;
Vector4 g_SkeletonPos[NUI_SKELETON_POSITION_COUNT];
float g_DistanceFromFloor = 0;
NUI_IMAGE_RESOLUTION g_DepthImgResolution;
NUI_IMAGE_RESOLUTION g_ColorImgResolution;
Vector4* g_PointsData;
int g_iPointsCount;
BYTE* g_ColorsData;
LONG* g_ColorCoordinates;
LONG  g_colorToDepthDivisor;
DWORD g_DepthWidth;
DWORD g_DepthHeight;
DWORD g_ColorWidth;
DWORD g_ColorHeight;
DWORD g_TrackingUserID;
bool		 g_bElimateBackground;
///////////////////////

USHORT* m_DepthData;

INuiSensor *         m_pNuiSensor;
BSTR                    m_instanceId;

HANDLE        m_hNextDepthFrameEvent;
HANDLE        m_hNextColorFrameEvent;
HANDLE        m_hNextSkeletonEvent;

HANDLE        m_pDepthStreamHandle;
HANDLE        m_pVideoStreamHandle;

DWORD         m_SkeletonTrackingFlags;
DWORD         m_DepthStreamFlags;

// thread handling
HANDLE        m_hThNuiProcess;
HANDLE        m_hEvNuiProcessStop;



template<class Interface>
inline void SafeRelease( Interface *& pInterfaceToRelease )
{
	if ( pInterfaceToRelease != NULL )
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}

HRESULT Nui_Init( )
{
	HRESULT  hr;
	bool     result;

	if ( !m_pNuiSensor )
	{
		hr = NuiCreateSensorByIndex(0, &m_pNuiSensor);

		if ( FAILED(hr) )
		{
			return hr;
		}

		SysFreeString(m_instanceId);

		m_instanceId = m_pNuiSensor->NuiDeviceConnectionId();
	}

	m_hNextDepthFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	m_hNextColorFrameEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	m_hNextSkeletonEvent = CreateEvent( NULL, TRUE, FALSE, NULL );	

	DWORD nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH_AND_PLAYER_INDEX | NUI_INITIALIZE_FLAG_USES_SKELETON |  NUI_INITIALIZE_FLAG_USES_COLOR;

	hr = m_pNuiSensor->NuiInitialize(nuiFlags);
	if ( E_NUI_SKELETAL_ENGINE_BUSY == hr )
	{
		nuiFlags = NUI_INITIALIZE_FLAG_USES_DEPTH |  NUI_INITIALIZE_FLAG_USES_COLOR;
		hr = m_pNuiSensor->NuiInitialize( nuiFlags) ;
	}

	if ( HasSkeletalEngine( m_pNuiSensor ) )
	{
		//m_SkeletonTrackingFlags = NUI_SKELETON_TRACKING_FLAG_TITLE_SETS_TRACKED_SKELETONS;
		hr = m_pNuiSensor->NuiSkeletonTrackingEnable( m_hNextSkeletonEvent, 0/*m_SkeletonTrackingFlags*/ );
		if( FAILED( hr ) )
		{
			return hr;
		}
	}

	g_ColorImgResolution = NUI_IMAGE_RESOLUTION_640x480;
	hr = m_pNuiSensor->NuiImageStreamOpen(
		NUI_IMAGE_TYPE_COLOR,
		g_ColorImgResolution,
		0,
		2,
		m_hNextColorFrameEvent,
		&m_pVideoStreamHandle );

	if ( FAILED( hr ) )
	{
		return hr;
	}

	//g_DepthImgResolution = NUI_IMAGE_RESOLUTION_320x240;
	g_DepthImgResolution = NUI_IMAGE_RESOLUTION_640x480;
	hr = m_pNuiSensor->NuiImageStreamOpen(
		HasSkeletalEngine(m_pNuiSensor) ? NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX : NUI_IMAGE_TYPE_DEPTH,
		g_DepthImgResolution,
		m_DepthStreamFlags,
		2,
		m_hNextDepthFrameEvent,
		&m_pDepthStreamHandle );

	if ( FAILED( hr ) )
	{
		return hr;
	}

	//new点云数据
	NuiImageResolutionToSize(g_DepthImgResolution, g_DepthWidth, g_DepthHeight );
	g_PointsData = new Vector4[g_DepthWidth*g_DepthHeight];
	m_DepthData = new USHORT[g_DepthWidth*g_DepthHeight];

	//new图像数据
	NuiImageResolutionToSize(g_ColorImgResolution, g_ColorWidth, g_ColorHeight);
	g_ColorsData = new BYTE[g_ColorWidth*g_ColorHeight*4];

	g_ColorCoordinates = new LONG[g_DepthWidth*g_DepthHeight*2];

	g_colorToDepthDivisor = g_ColorWidth/g_DepthWidth;

	// Start the Nui processing thread
	m_hEvNuiProcessStop = CreateEvent( NULL, FALSE, FALSE, NULL );
	m_hThNuiProcess = CreateThread( NULL, 0, Nui_ProcessThread, NULL, 0, NULL );

	g_TrackingUserID= 0;

	return hr;
}

void Nui_UnInit( )
{
	// Stop the Nui processing thread
	if ( NULL != m_hEvNuiProcessStop )
	{
		// Signal the thread
		SetEvent(m_hEvNuiProcessStop);

		// Wait for thread to stop
		if ( NULL != m_hThNuiProcess )
		{
			WaitForSingleObject( m_hThNuiProcess, INFINITE );
			CloseHandle( m_hThNuiProcess );
		}
		CloseHandle( m_hEvNuiProcessStop );
	}

	if ( m_pNuiSensor )
	{
		m_pNuiSensor->NuiShutdown( );
	}
	if ( m_hNextSkeletonEvent && ( m_hNextSkeletonEvent != INVALID_HANDLE_VALUE ) )
	{
		CloseHandle( m_hNextSkeletonEvent );
		m_hNextSkeletonEvent = NULL;
	}
	if ( m_hNextDepthFrameEvent && ( m_hNextDepthFrameEvent != INVALID_HANDLE_VALUE ) )
	{
		CloseHandle( m_hNextDepthFrameEvent );
		m_hNextDepthFrameEvent = NULL;
	}
	if ( m_hNextColorFrameEvent && ( m_hNextColorFrameEvent != INVALID_HANDLE_VALUE ) )
	{
		CloseHandle( m_hNextColorFrameEvent );
		m_hNextColorFrameEvent = NULL;
	}

	SafeRelease( m_pNuiSensor );
	
	if (g_PointsData != NULL)
	{
		delete[] g_PointsData;
		g_PointsData = NULL;
	}	

	if (g_ColorsData != NULL)
	{
		delete[] g_ColorsData;
		g_ColorsData = NULL;
	}

	if (m_DepthData != NULL)
	{
		delete[] m_DepthData;
		m_DepthData = NULL;
	}

	if (g_ColorCoordinates != NULL)
	{
		delete[] g_ColorCoordinates;
		g_ColorCoordinates = NULL;
	}
}

static DWORD WINAPI Nui_ProcessThread( LPVOID pParam )
{
	//CSkeletalViewerApp *pthis = (CSkeletalViewerApp *)pParam;
	return Nui_ProcessThread();
}

DWORD WINAPI Nui_ProcessThread( )
{
	const int numEvents = 4;
	HANDLE hEvents[numEvents] = { m_hEvNuiProcessStop, m_hNextDepthFrameEvent, m_hNextColorFrameEvent, m_hNextSkeletonEvent };
	int    nEventIdx;
	DWORD  t;

	// Main thread loop
	bool continueProcessing = true;
	while ( continueProcessing )
	{
		// Wait for any of the events to be signalled
		nEventIdx = WaitForMultipleObjects( numEvents, hEvents, FALSE, 100 );

		// Timed out, continue
		if ( nEventIdx == WAIT_TIMEOUT )
		{
			continue;
		}

		// stop event was signalled 
		if ( WAIT_OBJECT_0 == nEventIdx )
		{
			continueProcessing = false;
			break;
		}
		if ( WAIT_OBJECT_0 == WaitForSingleObject( m_hNextDepthFrameEvent, 0 ) )
		{
			//only increment frame count if a frame was successfully drawn
			Nui_GotDepthAlert();
		}

		if ( WAIT_OBJECT_0 == WaitForSingleObject( m_hNextColorFrameEvent, 0 ) )
		{
			Nui_GotColorAlert();
		}

		if (  WAIT_OBJECT_0 == WaitForSingleObject( m_hNextSkeletonEvent, 0 ) )
		{
			Nui_GotSkeletonAlert( );
		}
	}

	return 0;
}


bool Nui_GotSkeletonAlert( )
{
	NUI_SKELETON_FRAME SkeletonFrame = {0};

	bool foundSkeleton = false;
	int iCount=0;

	if ( SUCCEEDED(m_pNuiSensor->NuiSkeletonGetNextFrame( 0, &SkeletonFrame )) )
	{
		//for ( int i = 0 ; i < NUI_SKELETON_COUNT ; i++ )
		//{
		//	NUI_SKELETON_TRACKING_STATE trackingState = SkeletonFrame.SkeletonData[i].eTrackingState;

		//	if ( trackingState == NUI_SKELETON_TRACKED || trackingState == NUI_SKELETON_POSITION_ONLY )
		//	{
		//		foundSkeleton = true;
		//		iCount++;
		//	}
		//}
	}

	// no skeletons!
	//if( !foundSkeleton )
	//{
	//	return true;
	//}

	////DWORD TrackingIDs[NUI_SKELETON_MAX_TRACKED_COUNT];
	//DWORD TrackingIDs[NUI_SKELETON_MAX_TRACKED_COUNT];
	//int TrackingIDsIndex=0;
	//for (int i=0; i < NUI_SKELETON_COUNT; i++)
	//{
	//	if (SkeletonFrame.SkeletonData[i].eTrackingState)
	//	{
	//		TrackingIDs[TrackingIDsIndex++] = SkeletonFrame.SkeletonData[i].dwTrackingID;
	//	}		
	//}

	//if (iCount >= 2)
	//{
	//	DWORD dwTime = GetTickCount();
	//	char szInfo[MAX_PATH];
	//	sprintf_s(szInfo,"ID1=%d, ID2=%d, time=[%d]\n", TrackingIDs[0], TrackingIDs[1],dwTime);
	//	OutputDebugString(szInfo);
	//	//assert(0);
	//}

	#define INVALID_TRACKING_ID 0

	float NearestDistance  = FLT_MAX;
	DWORD NewNearestID = INVALID_TRACKING_ID;

	g_SkeletonAvaiable = false;
	for ( int i = 0 ; i < NUI_SKELETON_COUNT; i++ )
	{
		NUI_SKELETON_TRACKING_STATE trackingState = SkeletonFrame.SkeletonData[i].eTrackingState;
		NUI_SKELETON_DATA& data = SkeletonFrame.SkeletonData[i];

		if ( trackingState == NUI_SKELETON_TRACKED )
		{
			float Distance = (data.Position.x * data.Position.x) + /*(data.Position.y * data.Position.y)*/ + (data.Position.z * data.Position.z);
			//根据距离来决定追踪谁
			if (Distance < NearestDistance)
			{
				NewNearestID = data.dwTrackingID;
				NearestDistance = Distance;
			}			
		}
	}

	if (NewNearestID == INVALID_TRACKING_ID)
	{
		//没追踪到任何人
		return true;
	}
	else
	{
		if (g_TrackingUserID != NewNearestID)
		{
			//追踪的人变了
			g_TrackingUserID = NewNearestID;
			//MyDebugOutput("useIDChanged, g_FarestUserID[%d]\n",g_TrackingUserID);
			//DWORD TrackingIDs[NUI_SKELETON_MAX_TRACKED_COUNT];
			//TrackingIDs[0] = g_TrackingUserID;
			//TrackingIDs[1] = 0;
			//NuiSkeletonSetTrackedSkeletons(TrackingIDs);
		}
	}

	HRESULT hr = m_pNuiSensor->NuiTransformSmooth(&SkeletonFrame,NULL);
	if ( FAILED(hr) )
	{
		return false;
	}

	for ( int i = 0 ; i < NUI_SKELETON_COUNT; i++ )
	{
		NUI_SKELETON_DATA& data = SkeletonFrame.SkeletonData[i];
		if (g_TrackingUserID == data.dwTrackingID)
		{
			for (int j=0; j < NUI_SKELETON_POSITION_COUNT; j++)
			{
				g_SkeletonPos[j] = SkeletonFrame.SkeletonData[i].SkeletonPositions[j];
					g_SkeletonAvaiable = true;
			}			
		}
	}

	if (SkeletonFrame.vFloorClipPlane.w != 0)
	{
		g_DistanceFromFloor = SkeletonFrame.vFloorClipPlane.w;
	}

	return true;
}

bool Nui_GotDepthAlert( )
{
	NUI_IMAGE_FRAME imageFrame;
	bool processedFrame = true;

	HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame(
		m_pDepthStreamHandle,
		0,
		&imageFrame );

	if ( FAILED( hr ) )
	{
		return false;
	}

	INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect( 0, &LockedRect, NULL, 0 );
	if ( 0 != LockedRect.Pitch )
	{
		memcpy(m_DepthData, LockedRect.pBits, LockedRect.size);

		// draw the bits to the bitmap
		//BYTE * rgbrun = m_depthRGBX;
		const USHORT * pBufferRun = (const USHORT *)LockedRect.pBits;

		// end pixel is start + width*height - 1
		const USHORT * pBufferEnd = pBufferRun + (g_DepthWidth * g_DepthHeight);

		//assert( frameWidth * frameHeight * g_BytesPerPixel <= ARRAYSIZE(m_depthRGBX) );
		int DepthIndex = 0;
		g_iPointsCount = 0;
		while ( pBufferRun < pBufferEnd )
		{
			USHORT depth     = *pBufferRun;
			//USHORT realDepth = NuiDepthPixelToDepth(depth);
			USHORT player    = NuiDepthPixelToPlayerIndex(depth);

			// transform 13-bit depth information into an 8-bit intensity appropriate
			// for display (we disregard information in most significant bit)
			//BYTE intensity = static_cast<BYTE>(~(realDepth >> 4));

			LONG DepthX = DepthIndex % g_DepthWidth;
			LONG DepthY = DepthIndex / g_DepthWidth;
			Vector4 point;
			point = NuiTransformDepthImageToSkeleton(DepthX, DepthY, depth, g_DepthImgResolution);
			
			if (g_bElimateBackground)
			{
				// 只显示人
				if (player != 0)
				{				
					g_PointsData[DepthIndex] = point;
				}
				else
				{
					g_PointsData[DepthIndex].x = 0.0f;
					g_PointsData[DepthIndex].y = 0.0f;
					g_PointsData[DepthIndex].z = 0.0f;
					g_PointsData[DepthIndex].w = 0.0f;
				}
			}
			else
			{
				//同时显示背景和人
				g_PointsData[DepthIndex] = point;
			}

			DepthIndex++;

			++pBufferRun;
		}

		//m_pDrawDepth->Draw( m_depthRGBX, frameWidth * frameHeight * g_BytesPerPixel );
	}
	else
	{
		processedFrame = false;
		//OutputDebugString( L"Buffer length of received texture is bogus\r\n" );
	}

	pTexture->UnlockRect(0);

	m_pNuiSensor->NuiImageStreamReleaseFrame( m_pDepthStreamHandle, &imageFrame );

	return processedFrame;
}

bool Nui_GotColorAlert( )
{
	NUI_IMAGE_FRAME imageFrame;
	bool processedFrame = true;

	HRESULT hr = m_pNuiSensor->NuiImageStreamGetNextFrame( m_pVideoStreamHandle, 0, &imageFrame );

	if ( FAILED( hr ) )
	{
		return false;
	}

	INuiFrameTexture * pTexture = imageFrame.pFrameTexture;
	NUI_LOCKED_RECT LockedRect;
	pTexture->LockRect( 0, &LockedRect, NULL, 0 );
	if ( LockedRect.Pitch != 0 )
	{
		memcpy(g_ColorsData, LockedRect.pBits, LockedRect.size);
	}
	else
	{
		//OutputDebugString( L"Buffer length of received texture is bogus\r\n" );
		processedFrame = false;
	}

	pTexture->UnlockRect( 0 );

	m_pNuiSensor->NuiImageStreamReleaseFrame( m_pVideoStreamHandle, &imageFrame );

	return processedFrame;
}

LONG* Nui_MapColorToDepth()
{
	m_pNuiSensor->NuiImageGetColorPixelCoordinateFrameFromDepthPixelFrameAtResolution(
		g_ColorImgResolution,
		g_DepthImgResolution,
		g_DepthWidth*g_DepthHeight,
		m_DepthData,
		g_DepthWidth*g_DepthHeight*2,
		g_ColorCoordinates);

	return g_ColorCoordinates;
}
