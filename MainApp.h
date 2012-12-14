#pragma once
#include "d3dUtility.h"
#include "NuiApi.h"
#include "terrain.h"
#include "camera.h"
#include "KinectMesh.h"
#include "SkeletonPoint.h"
#include "floor.h"
#include "PointCloud.h"
#include "KinectDefine.h"
//
// Globals
//
IDirect3DDevice9* Device = 0; 
const int Width  = 1024;
const int Height = 768;

//kinect相关的外部全局变量
/////////////////////////////////
extern Vector4 g_SkeletonPos[NUI_SKELETON_POSITION_COUNT];
extern bool g_SkeletonAvaiable;
extern NUI_IMAGE_RESOLUTION g_DepthImgResolution;
extern Vector4* g_PointsData;
extern BYTE* g_ColorsData;
extern int g_iPointsCount;
extern LONG* g_ColorCoordinates;
extern DWORD g_DepthWidth;
extern DWORD g_DepthHeight;
extern DWORD g_ColorWidth;
extern DWORD g_ColorHeight;
extern bool g_bElimateBackground;
/////////////////////////////////

Terrain* g_Terrain = 0;
Camera g_Camera(Camera::LANDOBJECT);

//3D
CKinectMesh* g_pKinectMesh;
CSkeletonPoint* g_pSkeletonPoint;
CPointCloud* g_pPointCloud;
ESHOW_TYPE g_eShowType;
bool g_bGenerateXFile;