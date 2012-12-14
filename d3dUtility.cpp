//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: d3dUtility.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Provides utility functions for simplifying common tasks.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"


extern float g_DistanceFromFloor;

// vertex formats
const DWORD d3d::Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

bool d3d::InitD3D(
	HINSTANCE hInstance,
	int width, int height,
	bool windowed,
	D3DDEVTYPE deviceType,
	IDirect3DDevice9** device)
{
	//
	// Create the main application window.
	//

	WNDCLASS wc;

	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = (WNDPROC)d3d::WndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = hInstance;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = "DepthWithColor-DX9";

	if( !RegisterClass(&wc) ) 
	{
		::MessageBox(0, "RegisterClass() - FAILED", 0, 0);
		return false;
	}
		
	HWND hwnd = 0;
	hwnd = ::CreateWindow("DepthWithColor-DX9", "DepthWithColor-DX9", 
		WS_EX_TOPMOST,
		0, 0, width, height,
		0 /*parent hwnd*/, 0 /* menu */, hInstance, 0 /*extra*/); 

	if( !hwnd )
	{
		::MessageBox(0, "CreateWindow() - FAILED", 0, 0);
		return false;
	}

	::ShowWindow(hwnd, SW_SHOW);
	::UpdateWindow(hwnd);	

	//
	// Init D3D: 
	//

	HRESULT hr = 0;

	// Step 1: Create the IDirect3D9 object.

	IDirect3D9* d3d9 = 0;
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    if( !d3d9 )
	{
		::MessageBox(0, "Direct3DCreate9() - FAILED", 0, 0);
		return false;
	}

	// Step 2: Check for hardware vp.

	D3DCAPS9 caps;
	d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, deviceType, &caps);

	int vp = 0;
	if( caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT )
		vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
	else
		vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	// Step 3: Fill out the D3DPRESENT_PARAMETERS structure.
 
	D3DPRESENT_PARAMETERS d3dpp;
	d3dpp.BackBufferWidth            = width;
	d3dpp.BackBufferHeight           = height;
	d3dpp.BackBufferFormat           = D3DFMT_A8R8G8B8;
	d3dpp.BackBufferCount            = 1;
	d3dpp.MultiSampleType            = D3DMULTISAMPLE_NONE;
	d3dpp.MultiSampleQuality         = 0;
	d3dpp.SwapEffect                 = D3DSWAPEFFECT_DISCARD; 
	d3dpp.hDeviceWindow              = hwnd;
	d3dpp.Windowed                   = windowed;
	d3dpp.EnableAutoDepthStencil     = true; 
	d3dpp.AutoDepthStencilFormat     = D3DFMT_D24S8;
	d3dpp.Flags                      = 0;
	d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
	d3dpp.PresentationInterval       = D3DPRESENT_INTERVAL_IMMEDIATE;

	// Step 4: Create the device.

	hr = d3d9->CreateDevice(
		D3DADAPTER_DEFAULT, // primary adapter
		deviceType,         // device type
		hwnd,               // window associated with device
		vp,                 // vertex processing
	    &d3dpp,             // present parameters
	    device);            // return created device

	if( FAILED(hr) )
	{
		// try again using a 16-bit depth buffer
		d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
		
		hr = d3d9->CreateDevice(
			D3DADAPTER_DEFAULT,
			deviceType,
			hwnd,
			vp,
			&d3dpp,
			device);

		if( FAILED(hr) )
		{
			d3d9->Release(); // done with d3d9 object
			::MessageBox(0, "CreateDevice() - FAILED", 0, 0);
			return false;
		}
	}

	d3d9->Release(); // done with d3d9 object
	
	return true;
}

int d3d::EnterMsgLoop( bool (*ptr_display)(float timeDelta) )
{
	MSG msg;
	::ZeroMemory(&msg, sizeof(MSG));

	static DWORD dwLastFrameCount=0;
	static DWORD dwFrameCount=0;
	static float lastTime_forfps = (float)timeGetTime(); 
	static float lastTime = (float)timeGetTime(); 

	while(msg.message != WM_QUIT)
	{
		if(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
		else
        {	
			float currTime  = (float)timeGetTime();
			float timeDelta = (currTime - lastTime)*0.001f;
			float timeDelta_forfps = (currTime - lastTime_forfps)*0.001f;
			if (timeDelta_forfps > 1)
			{
				DWORD fps;
				fps = (dwFrameCount - dwLastFrameCount) / timeDelta_forfps;
				dwLastFrameCount = dwFrameCount;

				char szfps[MAX_PATH];
				sprintf_s(szfps, "fps=[%d] \n",fps);
				//OutputDebugString(szfps);

				lastTime_forfps = currTime;
			}

			ptr_display(timeDelta);
			dwFrameCount++;

			lastTime = currTime;
        }
    }
    return msg.wParam;
}

float d3d::Lerp(float a, float b, float t)
{
	return a - (a*t) + (b*t);
}

bool d3d::DrawBasicScene(IDirect3DDevice9* device, float scale, ESHOW_TYPE ShowType)
{
	static IDirect3DVertexBuffer9* floor  = 0;
	static IDirect3DTexture9*      tex    = 0;
	static ID3DXMesh*              pillar = 0;

	HRESULT hr = 0;

	if( device == 0 )
	{
		if( floor && tex && pillar )
		{
			// they already exist, destroy them
			d3d::Release<IDirect3DVertexBuffer9*>(floor);
			d3d::Release<IDirect3DTexture9*>(tex);
			d3d::Release<ID3DXMesh*>(pillar);
		}
	}
	else if( !floor && !tex && !pillar )
	{
		// they don't exist, create them
		device->CreateVertexBuffer(
			6 * sizeof(d3d::Vertex),
			0, 
			d3d::Vertex::FVF,
			D3DPOOL_MANAGED,
			&floor,
			0);

		Vertex* v = 0;
		floor->Lock(0, 0, (void**)&v, 0);

		const int Height = -4.0f;//-2.5f

		v[0] = Vertex(-20.0f, Height, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
		v[1] = Vertex(-20.0f, Height,  20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
		v[2] = Vertex( 20.0f, Height,  20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

		v[3] = Vertex(-20.0f, Height, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
		v[4] = Vertex( 20.0f, Height,  20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
		v[5] = Vertex( 20.0f, Height, -20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);

		floor->Unlock();

		D3DXCreateCylinder(device, 0.5f, 0.5f, 5.0f, 20, 20, &pillar, 0);

		D3DXCreateTextureFromFile(
			device,
			"desert.bmp",
			&tex);
	}
	else
	{
		//
		// Pre-Render Setup
		//
		//device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		//device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		//device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);

		D3DXVECTOR3 dir(0.707f, -0.707f, 0.707f);
		D3DXCOLOR col(1.0f, 1.0f, 1.0f, 1.0f);
		D3DLIGHT9 light = d3d::InitDirectionalLight(&dir, &col);

		device->SetLight(0, &light);
		device->LightEnable(0, true);
		device->SetRenderState(D3DRS_NORMALIZENORMALS, true);
		device->SetRenderState(D3DRS_SPECULARENABLE, true);		

		//
		// Render
		//
		D3DXMATRIX T, R, P, S;
		D3DXMatrixScaling(&S, scale, scale, scale);
		// used to rotate cylinders to be parallel with world's y-axis
		D3DXMatrixRotationX(&R, -D3DX_PI * 0.5f);

		if (g_DistanceFromFloor != 0)
		{
			Vertex* v = 0;
			floor->Lock(0, 0, (void**)&v, 0);
			//const int Height = -4.0f;//-2.5f
			int Height = g_DistanceFromFloor * -1 * SKELETON_SCALE;

			if (ShowType == Type_PointCloud)
			{
				float height = -0.9f;
				v[0] = Vertex(-20.0f, height, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
				v[1] = Vertex(-20.0f, height,  20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
				v[2] = Vertex( 20.0f, height,  20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

				v[3] = Vertex(-20.0f, height, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
				v[4] = Vertex( 20.0f, height,  20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
				v[5] = Vertex( 20.0f, height, -20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
			}
			else
			{
				v[0] = Vertex(-20.0f, Height, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
				v[1] = Vertex(-20.0f, Height,  20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
				v[2] = Vertex( 20.0f, Height,  20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);

				v[3] = Vertex(-20.0f, Height, -20.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
				v[4] = Vertex( 20.0f, Height,  20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
				v[5] = Vertex( 20.0f, Height, -20.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);
			}


			floor->Unlock();

			// 画 floor
			D3DXMatrixIdentity(&T);
			T = T * S;
			device->SetTransform(D3DTS_WORLD, &T);
			//const D3DMATERIAL9 BEACH_SAND_MTRL = InitMtrl(BEACH_SAND,BEACH_SAND,BEACH_SAND,BLACK,2.f);
			//const D3DMATERIAL9 DESERT_SAND_MTRL = InitMtrl(DESERT_SAND,DESERT_SAND,DESERT_SAND,BLACK,2.f);
			//const D3DMATERIAL9 LIGHTBROWN_MTRL = InitMtrl(LIGHTBROWN,LIGHTBROWN,LIGHTBROWN,BLACK,2.f);
			//const D3DMATERIAL9 DARKBROWN_MTRL = InitMtrl(DARKBROWN,DARKBROWN,DARKBROWN,BLACK,2.f);
			device->SetMaterial(&d3d::DARKBROWN_MTRL);
			device->SetTexture(0, tex);
			device->SetStreamSource(0, floor, 0, sizeof(Vertex));
			device->SetFVF(Vertex::FVF);
			device->SetRenderState(D3DRS_LIGHTING, true);//开启光照
			device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2);
			
			if (ShowType == Type_PointCloud)
			{
				//在渲染地平面后关闭光照，关掉纹理
				//若开启纹理，人物点云会显示单色(绿色，跟纹理颜色有关)
				//若开启光照,人物点云也会显示单色(灰色，跟光照颜色有关)
				device->SetRenderState(D3DRS_LIGHTING, false);
				device->SetTexture(0, NULL);
			}			
		}


		//// 画 圆柱体
		//device->SetMaterial(&d3d::BLUE_MTRL);
		//device->SetTexture(0, 0);
		//for(int i = 0; i < 5; i++)
		//{
		//	D3DXMatrixTranslation(&T, -5.0f, 0.0f, -15.0f + (i * 7.5f));
		//	P = R * T * S;
		//	device->SetTransform(D3DTS_WORLD, &P);
		//	pillar->DrawSubset(0);

		//	D3DXMatrixTranslation(&T, 5.0f, 0.0f, -15.0f + (i * 7.5f));
		//	P = R * T * S;
		//	device->SetTransform(D3DTS_WORLD, &P);
		//	pillar->DrawSubset(0);
		//}
	}
	return true;
}

D3DLIGHT9 d3d::InitDirectionalLight(D3DXVECTOR3* direction, D3DXCOLOR* color)
{
	D3DLIGHT9 light;
	::ZeroMemory(&light, sizeof(light));

	light.Type      = D3DLIGHT_DIRECTIONAL;
	light.Ambient   = *color * 0.4f;
	light.Diffuse   = *color;
	light.Specular  = *color * 0.6f;
	light.Direction = *direction;

	return light;
}

D3DLIGHT9 d3d::InitPointLight(D3DXVECTOR3* position, D3DXCOLOR* color)
{
	D3DLIGHT9 light;
	::ZeroMemory(&light, sizeof(light));

	light.Type      = D3DLIGHT_POINT;
	light.Ambient   = *color * 0.6f;
	light.Diffuse   = *color;
	light.Specular  = *color * 0.6f;
	light.Position  = *position;
	light.Range        = 1000.0f;
	light.Falloff      = 1.0f;
	light.Attenuation0 = 1.0f;
	light.Attenuation1 = 0.0f;
	light.Attenuation2 = 0.0f;

	return light;
}

D3DLIGHT9 d3d::InitSpotLight(D3DXVECTOR3* position, D3DXVECTOR3* direction, D3DXCOLOR* color)
{
	D3DLIGHT9 light;
	::ZeroMemory(&light, sizeof(light));

	light.Type      = D3DLIGHT_SPOT;
	light.Ambient   = *color * 0.0f;
	light.Diffuse   = *color;
	light.Specular  = *color * 0.6f;
	light.Position  = *position;
	light.Direction = *direction;
	light.Range        = 1000.0f;
	light.Falloff      = 1.0f;
	light.Attenuation0 = 1.0f;
	light.Attenuation1 = 0.0f;
	light.Attenuation2 = 0.0f;
	light.Theta        = 0.4f;
	light.Phi          = 0.9f;

	return light;
}

D3DMATERIAL9 d3d::InitMtrl(D3DXCOLOR a, D3DXCOLOR d, D3DXCOLOR s, D3DXCOLOR e, float p)
{
	D3DMATERIAL9 mtrl;
	mtrl.Ambient  = a;
	mtrl.Diffuse  = d;
	mtrl.Specular = s;
	mtrl.Emissive = e;
	mtrl.Power    = p;
	return mtrl;
}
