#include "MainApp.h"
#include <assert.h>
#include <zmouse.h>
#include "NuiImpl.h"
#include "KinectMesh.h"
#include <fstream>
using namespace std;
//
// Framework Functions
//
bool Setup()
{
	g_pKinectMesh = NULL;
	g_pSkeletonPoint = NULL;
	g_pPointCloud = NULL;

	ifstream file;
	string strInfo;
	file.open("data/config.txt");
	int Value1;
	int Value2;
	int Value3;
	file >> strInfo >> Value1;
	file >> strInfo >> Value2;
	file >> strInfo >> Value3;
	g_eShowType = (ESHOW_TYPE)Value1;
	g_bElimateBackground = (bool)Value2;
	g_bGenerateXFile = (bool)Value3;

	switch (g_eShowType)
	{
	case Type_SkeletonPoint:
		g_pSkeletonPoint = new CSkeletonPoint(Device);
		break;
	case Type_PointCloud:
		g_pPointCloud = new CPointCloud(Device, g_DepthWidth, g_DepthHeight, g_ColorWidth, g_ColorHeight);
		break;
	case Type_Mesh:
		g_pKinectMesh = new CKinectMesh(Device, g_DepthWidth, g_DepthHeight, g_ColorWidth, g_ColorHeight);
		break;
	default:
		assert(0);
		break;
	}	

	// 设置
	D3DXMATRIX proj;
	D3DXMatrixPerspectiveFovLH(
			&proj,
			NUI_CAMERA_DEPTH_NOMINAL_VERTICAL_FOV * D3DX_PI / 180.f, 
			//D3DX_PI * 0.5f, // 90 - degree
			(float)640/*Width*/ / (float)480/*Height*/,
			1.0f,
			1000.0f
			//1.0f * SKELETON_SCALE,
			//4.0f * SKELETON_SCALE
			);
	Device->SetTransform(D3DTS_PROJECTION, &proj);

	//
	// Switch to wireframe mode.
	//
	//Device->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
	//Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	//Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	//Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

	Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	return true;
}

void Cleanup()
{
	d3d::Delete<Terrain*>(g_Terrain);
	d3d::Delete<CKinectMesh*>(g_pKinectMesh);
	d3d::Delete<CSkeletonPoint*>(g_pSkeletonPoint);
	d3d::Delete<CPointCloud*>(g_pPointCloud);
}

void OnKeyboardEvent(Camera& camera, float timeDelta)
{
	//
	// Update: Update the camera.
	//
	if( ::GetAsyncKeyState('W') & 0x8000f )
		camera.walk(4.0f * timeDelta);

	if( ::GetAsyncKeyState('S') & 0x8000f )
		camera.walk(-4.0f * timeDelta);

	if( ::GetAsyncKeyState('A') & 0x8000f )
		camera.strafe(-4.0f * timeDelta);

	if( ::GetAsyncKeyState('D') & 0x8000f )
		camera.strafe(4.0f * timeDelta);

	if( ::GetAsyncKeyState('R') & 0x8000f )
		camera.fly(4.0f * timeDelta);

	if( ::GetAsyncKeyState('F') & 0x8000f )
		camera.fly(-4.0f * timeDelta);

	if( ::GetAsyncKeyState(VK_UP) & 0x8000f )
		camera.pitch(4.0f * timeDelta);

	if( ::GetAsyncKeyState(VK_DOWN) & 0x8000f )
		camera.pitch(-4.0f * timeDelta);

	if( ::GetAsyncKeyState(VK_LEFT) & 0x8000f )
		camera.yaw(-4.0f * timeDelta);

	if( ::GetAsyncKeyState(VK_RIGHT) & 0x8000f )
		camera.yaw(4.0f * timeDelta);

	if( ::GetAsyncKeyState('N') & 0x8000f )
		camera.roll(4.0f * timeDelta);

	if( ::GetAsyncKeyState('M') & 0x8000f )
		camera.roll(-4.0f * timeDelta);

	if (::GetAsyncKeyState('X') & 0x8000f)
	{
		camera.InitCamera();
	}

	D3DXMATRIX V;
	camera.getViewMatrix(&V);
	Device->SetTransform(D3DTS_VIEW, &V);	
}

bool Display(float timeDelta)
{
	if( Device )
	{
		OnKeyboardEvent(g_Camera, timeDelta);

		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);//清成黑屏
		Device->BeginScene();

		//渲染地平面
		d3d::DrawBasicScene(Device, 1.0f, g_eShowType);


		D3DXMATRIX I;
		D3DXMatrixIdentity(&I);
		if( g_Terrain )
			g_Terrain ->draw(&I, false);

		if (g_SkeletonAvaiable)
		{
			switch (g_eShowType)
			{
			case Type_SkeletonPoint:
				g_pSkeletonPoint->Draw(g_SkeletonPos);
				break;
			case Type_PointCloud:
				g_pPointCloud->Draw(g_PointsData, g_ColorsData);
				break;
			case Type_Mesh:
				g_pKinectMesh->Draw(g_PointsData, g_iPointsCount, g_ColorsData, g_bGenerateXFile);
				break;
			default:
				assert(0);
				break;
			}
		}
		Device->EndScene();
		Device->Present(0, 0, 0, 0);
	}
	return true;
}


//
// WndProc
//
LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static short xOldPos = 0;
	static short yOldPos = 0;

	static bool bLMouseDown = false;

	switch( msg )
	{
	case WM_DESTROY:
		::PostQuitMessage(0);
		break;
		
	case WM_KEYDOWN:
		if( wParam == VK_ESCAPE )
			::DestroyWindow(hwnd);
		break;
	
	//添加鼠标事件响应
	//鼠标中键控制摄像机的远近位置
	//鼠标左键旋转摄像机
	case WM_MOUSEWHEEL:
		{
			short zDelta = (short) HIWORD(wParam);    // wheel rotation
			if(zDelta   <   0) 
				g_Camera.walk(-1.0f);
			else 
				g_Camera.walk(1.0f);			
		}break;
	case WM_LBUTTONDOWN:
		{
			bLMouseDown = true;
		}break;
	case WM_LBUTTONUP:
		{
			bLMouseDown = false;
		}break;
	case WM_MOUSEMOVE:
		{
			if (bLMouseDown)
			{
				short xNewPos,yNewPos;
				if (xOldPos == 0 || yOldPos == 0)
				{
					xOldPos = (short)LOWORD(lParam);
					yOldPos = (short)HIWORD(lParam);
				}
				else
				{
					xNewPos = (short)LOWORD(lParam);
					yNewPos = (short)HIWORD(lParam);

					float dx = (xNewPos - xOldPos) * 0.005f;
					float dy = (yNewPos - yOldPos) * 0.005f;
					if (dx < 0.5 && dx > -0.5 && dy < 0.5 && dy > -0.5)// 限制摄像机镜头不要一次移动过快
					{
						g_Camera.yaw(dx);
						g_Camera.pitch(dy);

						char szInfo[MAX_PATH];
						sprintf_s(szInfo,"镜头移动 dx[%f], dy[%f]\n",dx,dy);
						//OutputDebugString(szInfo);
					}

					xOldPos = xNewPos;
					yOldPos = yNewPos;
				}
			}
		}break;
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

//
// WinMain
//
int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}

	Nui_Init();
		
	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}

	d3d::EnterMsgLoop( Display );

	Cleanup();

	Nui_UnInit();

	Device->Release();	

	return 0;
}