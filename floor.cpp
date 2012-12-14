#include "floor.h"
#include "d3dUtility.h"
#include <assert.h>
#include "KinectDefine.h"

#define SKELETON_LINE_INDEX_COUNT 50

struct Vertex_Floor
{
	Vertex_Floor(){}
	Vertex_Floor(float x, float y, float z)
	{
		_x = x;  _y = y;  _z = z;
	}
	float _x, _y, _z;
	static const DWORD FVF;
};
const DWORD Vertex_Floor::FVF = D3DFVF_XYZ;

CFloor::CFloor(IDirect3DDevice9* device, string strTextureFileName)
: m_vb(NULL)
, m_ib(NULL)
, m_Texure(NULL)
{
	m_Device = device;

	HRESULT hr;
	hr = m_Device->CreateVertexBuffer(
		8 * sizeof(Vertex_Floor), 
		D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
		Vertex_Floor::FVF,
		D3DPOOL_DEFAULT,
		&m_vb,
		0);

	//静态索引，因为顶点组织方式不变
	hr = m_Device->CreateIndexBuffer(
		11 * sizeof(WORD),
		D3DUSAGE_WRITEONLY,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&m_ib,
		0);

	Vertex_Floor* vertices;
	m_vb->Lock(0, 0, (void**)&vertices, 0);
	vertices[0] = Vertex_Floor(-5.0f, 0.0f, 0.0f);//左下
	vertices[1] = Vertex_Floor(5.0f,  0.0f, 0.0f);//右下
	vertices[2] = Vertex_Floor(-5.0f, 0.0f, 2.0f);
	vertices[3] = Vertex_Floor(5.0f,  0.0f, 2.0f);
	vertices[4] = Vertex_Floor(-5.0f, 0.0f, 4.0f);//左上
	vertices[5] = Vertex_Floor(5.0f,  0.0f, 4.0f);//右上
	vertices[6] = Vertex_Floor(0.0f, 0.0f, 0.0f);//下中
	vertices[7] = Vertex_Floor(0.0f, 0.0f, 4.0f);//上中
	m_vb->Unlock();

	WORD* indices_grid = 0;
	m_ib->Lock(0, 0, (void**)&indices_grid, 0);
	indices_grid[0] = 0;indices_grid[1] = 1;
	indices_grid[2] = 2; indices_grid[3] = 3;  
	indices_grid[4] = 4; indices_grid[5] = 5;
	indices_grid[6] = 0; indices_grid[7] = 4;  
	indices_grid[8] = 1; indices_grid[9] = 5;
	indices_grid[9] = 6; indices_grid[10] = 7;
	m_ib->Unlock();
}

CFloor::~CFloor()
{
	if(m_vb){m_vb->Release(); m_vb = 0;}
	if(m_ib){m_ib->Release(); m_ib = 0;}
}

bool CFloor::Draw()
{
	return true;
}