#pragma once

#include <d3dx9.h>
#include "NuiImpl.h"

class CPointCloud
{
public:
	CPointCloud(IDirect3DDevice9* device, int DepthWidth, int DepthHeight,  int ColorWidth, int ColorHeight);
	~CPointCloud();

	bool Draw(Vector4* Points, BYTE* ColorData);

private:
	IDirect3DDevice9*       m_Device;
	IDirect3DVertexBuffer9* m_vb;

	int m_DepthWidth;
	int m_DepthHeight;
	int m_ColorWidth;
	int m_ColorHeight;
};