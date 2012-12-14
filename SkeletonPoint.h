#pragma once

#include <d3dx9.h>
#include "NuiImpl.h"

#define SPHERE_COUNT 20

class CSkeletonPoint
{
public:
	CSkeletonPoint(IDirect3DDevice9* device);
	~CSkeletonPoint();

	bool Draw(IN Vector4 SkeletonPoints[NUI_SKELETON_POSITION_COUNT]);

private:
	IDirect3DDevice9*       m_Device;
	IDirect3DVertexBuffer9* m_vb;
	IDirect3DIndexBuffer9*  m_ib;
	IDirect3DTexture9* m_Texure;
	ID3DXMesh* m_Spheres[SPHERE_COUNT];
};