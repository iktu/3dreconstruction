#pragma once

#include <d3dx9.h>
#include "NuiImpl.h"
#include <string>
using namespace std;

class CFloor
{

public:
	CFloor(IDirect3DDevice9* device, string strTextureFileName);
	~CFloor();

	bool Draw();

private:
	IDirect3DDevice9*       m_Device;
	IDirect3DVertexBuffer9* m_vb;
	IDirect3DIndexBuffer9*  m_ib;
	IDirect3DTexture9* m_Texure;
};