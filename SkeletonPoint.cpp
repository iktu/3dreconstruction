#include "SkeletonPoint.h"
#include "d3dUtility.h"
#include <assert.h>
#include "KinectDefine.h"

#define SKELETON_LINE_INDEX_COUNT 50

struct Vertex_SkeletonPoint
{
	Vertex_SkeletonPoint(){}
	Vertex_SkeletonPoint(float x, float y, float z)
	{
		_x = x;  _y = y;  _z = z;
	}
	float _x, _y, _z;
	static const DWORD FVF;
};
const DWORD Vertex_SkeletonPoint::FVF = D3DFVF_XYZ;

CSkeletonPoint::CSkeletonPoint(IDirect3DDevice9* device)
: m_vb(NULL)
, m_ib(NULL)
, m_Texure(NULL)
{
	m_Device = device;

	HRESULT hr;
	hr = m_Device->CreateVertexBuffer(
		20 * sizeof(Vertex_SkeletonPoint), 
		D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
		Vertex_SkeletonPoint::FVF,
		D3DPOOL_DEFAULT,
		&m_vb,
		0);
	if (hr != S_OK)
	{
		assert(0);
	}

	//静态索引，因为顶点组织方式不变
	hr = m_Device->CreateIndexBuffer(
		SKELETON_LINE_INDEX_COUNT * sizeof(WORD),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&m_ib,
		0);
	if (hr != S_OK)
	{
		assert(0);
	}

	// 建立骨骼节点之间的连边关系
	WORD* indices = 0;
	m_ib->Lock(0, 0, (void**)&indices, 0);
	indices[0] = NUI_SKELETON_POSITION_HAND_RIGHT; indices[1] = NUI_SKELETON_POSITION_WRIST_RIGHT;
	indices[2] = NUI_SKELETON_POSITION_WRIST_RIGHT; indices[3] = NUI_SKELETON_POSITION_ELBOW_RIGHT;
	indices[4] = NUI_SKELETON_POSITION_ELBOW_RIGHT; indices[5] = NUI_SKELETON_POSITION_SHOULDER_RIGHT;
	indices[6] = NUI_SKELETON_POSITION_SHOULDER_RIGHT; indices[7] = NUI_SKELETON_POSITION_SHOULDER_CENTER;
	indices[8] = NUI_SKELETON_POSITION_SHOULDER_CENTER; indices[9] = NUI_SKELETON_POSITION_SHOULDER_LEFT;
	indices[10] = NUI_SKELETON_POSITION_SHOULDER_CENTER; indices[11] = NUI_SKELETON_POSITION_HEAD;
	indices[12] = NUI_SKELETON_POSITION_SHOULDER_LEFT; indices[13] = NUI_SKELETON_POSITION_ELBOW_LEFT;
	indices[14] = NUI_SKELETON_POSITION_ELBOW_LEFT; indices[15] = NUI_SKELETON_POSITION_WRIST_LEFT;
	indices[16] = NUI_SKELETON_POSITION_WRIST_LEFT; indices[17] = NUI_SKELETON_POSITION_HAND_LEFT;
	indices[18] = NUI_SKELETON_POSITION_SHOULDER_CENTER; indices[19] = NUI_SKELETON_POSITION_SPINE;
	indices[20] = NUI_SKELETON_POSITION_SPINE; indices[21] = NUI_SKELETON_POSITION_HIP_CENTER;
	indices[22] = NUI_SKELETON_POSITION_HIP_RIGHT; indices[23] = NUI_SKELETON_POSITION_HIP_CENTER;
	indices[24] = NUI_SKELETON_POSITION_HIP_LEFT; indices[25] = NUI_SKELETON_POSITION_HIP_CENTER;
	indices[26] = NUI_SKELETON_POSITION_HIP_RIGHT; indices[27] = NUI_SKELETON_POSITION_KNEE_RIGHT;
	indices[28] = NUI_SKELETON_POSITION_ANKLE_RIGHT; indices[29] = NUI_SKELETON_POSITION_FOOT_RIGHT;
	indices[30] = NUI_SKELETON_POSITION_HIP_LEFT; indices[31] = NUI_SKELETON_POSITION_KNEE_LEFT;
	indices[32] = NUI_SKELETON_POSITION_KNEE_LEFT; indices[33] = NUI_SKELETON_POSITION_ANKLE_LEFT;
	indices[34] = NUI_SKELETON_POSITION_ANKLE_LEFT; indices[35] = NUI_SKELETON_POSITION_FOOT_LEFT;
	indices[36] = NUI_SKELETON_POSITION_KNEE_RIGHT; indices[37] = NUI_SKELETON_POSITION_ANKLE_RIGHT;
	indices[38] = NUI_SKELETON_POSITION_SHOULDER_RIGHT; indices[39] = NUI_SKELETON_POSITION_SPINE;
	indices[40] = NUI_SKELETON_POSITION_SHOULDER_LEFT; indices[41] = NUI_SKELETON_POSITION_SPINE;
	indices[42] = NUI_SKELETON_POSITION_HIP_RIGHT; indices[43] = NUI_SKELETON_POSITION_SPINE;
	indices[44] = NUI_SKELETON_POSITION_HIP_LEFT; indices[45] = NUI_SKELETON_POSITION_SPINE;
	indices[46] = NUI_SKELETON_POSITION_SHOULDER_LEFT; indices[47] = NUI_SKELETON_POSITION_SHOULDER_RIGHT;
	indices[48] = NUI_SKELETON_POSITION_HIP_LEFT; indices[49] = NUI_SKELETON_POSITION_HIP_RIGHT;
	m_ib->Unlock();

	//m_Spheres[SPHERE_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	for (int i = 0; i < SPHERE_COUNT; i++)
	{
		D3DXCreateSphere(
			m_Device,
			0.1f, // width
			30,
			30,
			&m_Spheres[i],
			0);
	}
}

CSkeletonPoint::~CSkeletonPoint()
{
	if(m_vb){m_vb->Release(); m_vb = 0;}
	if(m_ib){m_ib->Release(); m_ib = 0;}

	for (int i =0; i < SPHERE_COUNT; i++)
	{
		m_Spheres[i]->Release();
		m_Spheres[i] = NULL;
	}
}

bool CSkeletonPoint::Draw(IN Vector4 SkeletonPoints[NUI_SKELETON_POSITION_COUNT])
{
	//画出骨骼球
	Vertex_SkeletonPoint* vertices;
	m_vb->Lock(0, 0, (void**)&vertices, 0);
	for(int i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
	{
		//确定骨骼顶点位置
		vertices[i] = Vertex_SkeletonPoint(SkeletonPoints[i].x * SKELETON_SCALE, SkeletonPoints[i].y * SKELETON_SCALE,  SkeletonPoints[i].z * SKELETON_SCALE);
	}
	D3DXMATRIX ObjWorldMatrices[NUI_SKELETON_POSITION_COUNT];
	for(int i = 0; i < NUI_SKELETON_POSITION_COUNT; i++)
	{
		D3DXMatrixTranslation(&ObjWorldMatrices[i],  SkeletonPoints[i].x * SKELETON_SCALE, SkeletonPoints[i].y * SKELETON_SCALE,  SkeletonPoints[i].z * SKELETON_SCALE);	
		m_Device->SetMaterial(&d3d::BLUE_MTRL);
		m_Device->SetTexture(0, 0);
		m_Device->SetTransform(D3DTS_WORLD, &ObjWorldMatrices[i]);
		m_Spheres[i]->DrawSubset(0);	
	}
	D3DXMATRIX I;
	D3DXMatrixIdentity(&I);
	m_Device->SetTransform(D3DTS_WORLD, &I);
	m_vb->Unlock();

	//画球之间的连线
	m_Device->SetStreamSource(0, m_vb, 0, sizeof(Vertex_SkeletonPoint));
	m_Device->SetIndices(m_ib);
	m_Device->SetFVF(Vertex_SkeletonPoint::FVF);
	m_Device->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, 20,0,SKELETON_LINE_INDEX_COUNT / 2);

	return true;
}