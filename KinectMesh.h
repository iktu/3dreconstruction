#pragma once

#include <d3dx9.h>
#include "NuiImpl.h"

class CKinectMesh
{
public:
	CKinectMesh(IDirect3DDevice9* device, int DepthWidth, int DepthHeight,  int ColorWidth, int ColorHeight);
	~CKinectMesh();

	bool Draw(Vector4* Points, int PointsCount, BYTE* ColorData, bool bGenerateXFile);

private:
	IDirect3DDevice9*       m_Device;
	IDirect3DVertexBuffer9* m_vb;
	IDirect3DIndexBuffer9*  m_ib;
	IDirect3DTexture9* m_Texure;
	ID3DXMesh* m_Spheres;

	int m_DepthWidth;
	int m_DepthHeight;
	int m_ColorWidth;
	int m_ColorHeight;
};