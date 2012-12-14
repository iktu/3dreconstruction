#include "PointCloud.h"
#include "d3dUtility.h"
#include "KinectDefine.h"

struct Vertex_PointCloud
{
	D3DXVECTOR3 _position;
	D3DCOLOR _color;
	static const DWORD FVF;
};
const DWORD Vertex_PointCloud::FVF = D3DFVF_XYZ | D3DFVF_DIFFUSE;

enum KINECT_COLOR
{
	BLUE=0,
	GREEN,
	RED,
	ALPHA,
	MAX_COUNT,
};

CPointCloud::CPointCloud(IDirect3DDevice9* device, int DepthWidth, int DepthHeight,  int ColorWidth, int ColorHeight)
: m_vb(NULL)
{
	m_Device = device;
	m_DepthWidth = DepthWidth;
	m_DepthHeight = DepthHeight;
	m_ColorWidth = ColorWidth;
	m_ColorHeight = ColorHeight;

	m_Device->CreateVertexBuffer(
		m_DepthWidth * m_DepthHeight * sizeof(Vertex_PointCloud),
		D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC | D3DUSAGE_POINTS,
		Vertex_PointCloud::FVF,
		D3DPOOL_DEFAULT,
		&m_vb,
		0);
}

CPointCloud::~CPointCloud()
{
	if(m_vb){m_vb->Release(); m_vb = 0;}
}

bool CPointCloud::Draw(Vector4* Points, BYTE* ColorData)
{
	//画出场景mesh
	Vertex_PointCloud* vertex;

	m_vb->Lock(0, 0, (void**)&vertex, 0);
	LONG* ColorCoordinates = Nui_MapColorToDepth();//调用api 对齐深度到彩色
	int ErrorPoint = 0;

	for (int i = 0; i < m_DepthWidth * m_DepthHeight; i++)
	{
		vertex[i]._position = D3DXVECTOR3(Points[i].x /** SKELETON_SCALE*/, Points[i].y/** SKELETON_SCALE*/, Points[i].z /** SKELETON_SCALE*/);
		//vertex[i]._position = D3DXVECTOR3(Points[i].x * SKELETON_SCALE, Points[i].y* SKELETON_SCALE, Points[i].z * SKELETON_SCALE);

		int width = i % m_DepthWidth;
		int height= i / m_DepthWidth;

		// 		vertex[i]._u = (float)width / float(m_DepthWidth);
		// 		vertex[i]._v = (float)height / float(m_DepthHeight);

		//计算出顶点颜色		
		//调用api 对齐深度到彩色
		//int DepthIndex = i+j*m_DepthWidth;
		LONG ColorInDepthX = ColorCoordinates[i*2];
		LONG ColorInDepthY = ColorCoordinates[i*2+1];
		if (ColorInDepthX >= 0 && ColorInDepthX < m_ColorWidth && ColorInDepthY < m_ColorHeight)
		{
			LONG colorIndex = ColorInDepthX + ColorInDepthY * m_ColorWidth;
			vertex[i]._color = D3DCOLOR_ARGB(
				ColorData[colorIndex*MAX_COUNT+ALPHA],
				ColorData[colorIndex*MAX_COUNT+RED],
				ColorData[colorIndex*MAX_COUNT+GREEN],
				ColorData[colorIndex*MAX_COUNT+BLUE]);
		}
		else
		{
			vertex[i]._color = D3DCOLOR_ARGB(0,255,255,255);
			ErrorPoint++;
		}
	}

	m_vb->Unlock();
	m_Device->SetMaterial(&d3d::WHITE_MTRL);
	m_Device->SetStreamSource(0, m_vb, 0, sizeof(Vertex_PointCloud));
	m_Device->SetFVF(Vertex_PointCloud::FVF);
	m_Device->DrawPrimitive(D3DPT_POINTLIST, 0, m_DepthWidth * m_DepthHeight);

	return true;
}