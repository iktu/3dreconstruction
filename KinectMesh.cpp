#include "KinectMesh.h"
#include "d3dUtility.h"
#include "KinectDefine.h"
#include <fstream>
using namespace std;

struct Vertex_Mesh
{
	D3DXVECTOR3 _position;
	//D3DCOLOR _color;
	static const DWORD FVF;
	float _u, _v;
};
/*const DWORD Vertex_Mesh::FVF = D3DFVF_XYZRHW | / *D3DFVF_XYZ |* / / *D3DFVF_DIFFUSE |* / D3DFVF_TEX1;*/
const DWORD Vertex_Mesh::FVF = D3DFVF_XYZ | D3DFVF_TEX1;

enum KINECT_COLOR
{
	BLUE=0,
	GREEN,
	RED,
	ALPHA,
	MAX_COUNT,
};

#define SHOW_LINE_COUNT 100

CKinectMesh::CKinectMesh(IDirect3DDevice9* device, int DepthWidth, int DepthHeight,  int ColorWidth, int ColorHeight)
: m_vb(NULL)
, m_ib(NULL)
, m_Texure(NULL)
{
	m_Device = device;
	m_DepthWidth = DepthWidth;
	m_DepthHeight = DepthHeight;
	m_ColorWidth = ColorWidth;
	m_ColorHeight = ColorHeight;

	//m_Device->CreateVertexBuffer(
	//	6 * (m_DepthWidth-1) * (m_DepthHeight-1) * sizeof(Vertex_Mesh),
	//	D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
	//	Vertex_Mesh::FVF,
	//	D3DPOOL_DEFAULT,
	//	&m_vb,
	//	0);

	m_Device->CreateVertexBuffer(
		m_DepthWidth * m_DepthHeight * sizeof(Vertex_Mesh),
		D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
		Vertex_Mesh::FVF,
		D3DPOOL_DEFAULT,
		&m_vb,
		0);

	m_Device->CreateIndexBuffer(
		6 * (m_DepthWidth-1) * (m_DepthHeight-1) * sizeof(DWORD),
		D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
		D3DFMT_INDEX32,//这里一定要用32位缓存，16位的最大表示65535不够用，超出后会指向(0,0,0)点
		D3DPOOL_DEFAULT,
		&m_ib,
		0);

	//WORD* indexData = 0;
	//m_ib->Lock(0, 0, (void**)&indexData, 0);
	//int VertexIndex = 0;
	//const int POINT_INDEX_COUNT = 6;
	//int PointIndex[POINT_INDEX_COUNT];
	//for (int j=0; j < m_DepthHeight-1; j++)
	//{
	//	for (int i=0; i < m_DepthWidth-1; i++)
	//	{
	//		//if ( j != 0 || i > 50)
	//		//{
	//		//	break;
	//		//}
	//		//
	//		//计算出三角形顶点的位置
	//		//
	//		PointIndex[0] = j * m_DepthWidth + i;
	//		PointIndex[1] = PointIndex[0] +1;
	//		PointIndex[2] = PointIndex[1] + m_DepthWidth;

	//		PointIndex[3] = PointIndex[0];
	//		PointIndex[4] = PointIndex[3] + m_DepthWidth + 1;
	//		PointIndex[5] = PointIndex[3] + m_DepthWidth;
	//		//PointIndex[5] = PointIndex[3] + m_DepthWidth + 1;
	//		//PointIndex[4] = PointIndex[3] + m_DepthWidth;
	//		

	//		for (int k = 0; k < POINT_INDEX_COUNT; k++)
	//		{
	//			int index = PointIndex[k];
	//			indexData[VertexIndex++] = PointIndex[k];
	//		}
	//	}
	//}
	//m_ib->Unlock();

	//HRESULT hr = D3DXCreateTextureFromFile(
	//	m_Device,
	//	"mesh.jpg",
	//	&m_Texure);

	//D3DXIMAGE_INFO info;
	//D3DXGetImageInfoFromFile("mesh.jpg",&info);

	D3DXCreateSphere(
		m_Device,
		0.1f, // width
		30,
		30,
		&m_Spheres,
		0);
}

CKinectMesh::~CKinectMesh()
{
	if(m_vb){m_vb->Release(); m_vb = 0;}
	if(m_ib){m_ib->Release(); m_ib = 0;}

	if (m_Spheres)
	{
		m_Spheres->Release();
		m_Spheres = NULL;
	}

	d3d::Release<IDirect3DTexture9*>(m_Texure);
}

bool CKinectMesh::Draw(Vector4* Points, int PointsCount, BYTE* ColorData, bool bGenerateXFile)
{
	//画出场景mesh
	Vertex_Mesh* vertex;
	m_vb->Lock(0, 0, (void**)&vertex, 0);
	//LONG* ColorCoordinates = Nui_MapColorToDepth();//调用api 对齐深度到彩色

	int vertexIndex = 0;
	for (int i = 0; i < m_DepthWidth * m_DepthHeight; i++)
	{
		//vertex[i]._position = D3DXVECTOR3(Points[i].x , Points[i].y, Points[i].z);
		vertex[i]._position = D3DXVECTOR3(Points[i].x * SKELETON_SCALE, Points[i].y* SKELETON_SCALE, Points[i].z * SKELETON_SCALE);	
		int width = i % m_DepthWidth;
		int height= i / m_DepthWidth;
		vertex[i]._u = (float)width / float(m_DepthWidth);
		vertex[i]._v = (float)height / float(m_DepthHeight);
	}

	DWORD* indexData = 0;
	m_ib->Lock(0, 0, (void**)&indexData, 0);
	int VertexIndex = 0;
	int IndexCount = 0;
	int IndexOldCount = 0;
	const int POINT_INDEX_COUNT = 6;
	int PointIndex[POINT_INDEX_COUNT];

	int ZeroPoint =0;

	ofstream ofs("vertexdata.txt");

	for (int j=0; j < m_DepthHeight-1; j++)
	{
		for (int i=0; i < m_DepthWidth-1; i++)
		{
			//if (j > 120 && j < m_DepthHeight-1)
			{
				//continue;
			}

			//
			//计算出三角形顶点的位置
			//
			PointIndex[0] = j * m_DepthWidth + i;
			PointIndex[1] = PointIndex[0] +1;
			PointIndex[2] = PointIndex[1] + m_DepthWidth;

			PointIndex[3] = PointIndex[0];
			PointIndex[4] = PointIndex[3] + m_DepthWidth + 1;
			PointIndex[5] = PointIndex[3] + m_DepthWidth;

			if (vertex[PointIndex[0]]._position.z != 0 && vertex[PointIndex[1]]._position.z != 0 && vertex[PointIndex[2]]._position.z != 0)
			{
				//if ((vertex[PointIndex[0]]._position.y > 0.01 || vertex[PointIndex[0]]._position.y < -0.01) &&
				//	(vertex[PointIndex[1]]._position.y > 0.01 || vertex[PointIndex[1]]._position.y < -0.01) &&
				//	(vertex[PointIndex[2]]._position.y > 0.01 || vertex[PointIndex[2]]._position.y < -0.01))
				{
					float AvgZ = ( vertex[PointIndex[0]]._position.z + vertex[PointIndex[1]]._position.z + vertex[PointIndex[2]]._position.z ) / 3.0;
					vertex[PointIndex[0]]._position.z = AvgZ;
					vertex[PointIndex[1]]._position.z = AvgZ;
					vertex[PointIndex[2]]._position.z = AvgZ;

					indexData[VertexIndex++] = PointIndex[0];
					indexData[VertexIndex++] = PointIndex[1];
					indexData[VertexIndex++] = PointIndex[2];

					//ofs<<"vertex["<<PointIndex[0]<<"].x="<<vertex[PointIndex[0]]._position.x<<",y="<<vertex[PointIndex[0]]._position.y<<",z="<<vertex[PointIndex[0]]._position.z<<endl;
					//ofs<<"vertex["<<PointIndex[1]<<"].x="<<vertex[PointIndex[1]]._position.x<<",y="<<vertex[PointIndex[1]]._position.y<<",z="<<vertex[PointIndex[1]]._position.z<<endl;
					//ofs<<"vertex["<<PointIndex[2]<<"].x="<<vertex[PointIndex[2]]._position.x<<",y="<<vertex[PointIndex[2]]._position.y<<",z="<<vertex[PointIndex[2]]._position.z<<endl;

					IndexCount++;
				}
			}

			if (vertex[PointIndex[3]]._position.z != 0 && vertex[PointIndex[4]]._position.z != 0 && vertex[PointIndex[5]]._position.z != 0)
			{
				//if ((vertex[PointIndex[0]]._position.y > 0.01 || vertex[PointIndex[0]]._position.y < -0.01) &&
				//	(vertex[PointIndex[1]]._position.y > 0.01 || vertex[PointIndex[1]]._position.y < -0.01) &&
				//	(vertex[PointIndex[2]]._position.y > 0.01 || vertex[PointIndex[2]]._position.y < -0.01))
				{
					float AvgZ = ( vertex[PointIndex[3]]._position.z + vertex[PointIndex[4]]._position.z + vertex[PointIndex[5]]._position.z ) / 3.0;
					vertex[PointIndex[3]]._position.z = AvgZ;
					vertex[PointIndex[4]]._position.z = AvgZ;
					vertex[PointIndex[5]]._position.z = AvgZ;					
					
					indexData[VertexIndex++] = PointIndex[3];
					indexData[VertexIndex++] = PointIndex[4];
					indexData[VertexIndex++] = PointIndex[5];

					//ofs<<"vertex["<<PointIndex[4]<<"].x="<<vertex[PointIndex[4]]._position.x<<",y="<<vertex[PointIndex[4]]._position.y<<",z="<<vertex[PointIndex[4]]._position.z<<endl;

					IndexCount++;
				}
			}
		}
	}
	m_ib->Unlock();

	D3DXMATRIX ObjWorldMatrices;
	D3DXMatrixTranslation(&ObjWorldMatrices,  0,0,0);	
	m_Device->SetTransform(D3DTS_WORLD, &ObjWorldMatrices);
	m_Spheres->DrawSubset(0);	

	//DWORD* indexData = 0;
	//m_ib->Lock(0, 0, (void**)&indexData, 0);
	//int VertexIndex = 0;
	//const int POINT_INDEX_COUNT = 6;
	//int PointIndex[POINT_INDEX_COUNT];
	//for (int j=0; j < m_DepthHeight-1; j++)
	//{
	//	for (int i=0; i < m_DepthWidth-1; i++)
	//	{
	//		//
	//		//计算出三角形顶点的位置
	//		//
	//		PointIndex[0] = j * m_DepthWidth + i;
	//		PointIndex[1] = PointIndex[0] +1;
	//		PointIndex[2] = PointIndex[1] + m_DepthWidth;

	//		PointIndex[3] = PointIndex[0];
	//		PointIndex[4] = PointIndex[3] + m_DepthWidth + 1;
	//		PointIndex[5] = PointIndex[3] + m_DepthWidth;
			//PointIndex[5] = PointIndex[3] + m_DepthWidth + 1;
			//PointIndex[4] = PointIndex[3] + m_DepthWidth;			

			//for (int k = 0; k < POINT_INDEX_COUNT; k++)
			//{
			//	int index = PointIndex[k];
			//	indexData[VertexIndex++] = PointIndex[k];
			//}

			//if (vertex[VertexIndex++])
			//{
			//}
			//indexData[VertexIndex++] = PointIndex[0];
			//indexData[VertexIndex++] = PointIndex[1];
			//indexData[VertexIndex++] = PointIndex[2];

			//indexData[VertexIndex++] = PointIndex[3];
			//indexData[VertexIndex++] = PointIndex[4];
			//indexData[VertexIndex++] = PointIndex[5];
	//	}
	//}
	//m_ib->Unlock();

	//int VertexIndex = 0;
	//const int POINT_INDEX_COUNT = 6;
	//int PointIndex[POINT_INDEX_COUNT];
	//for (int j=0; j < m_DepthHeight-1; j++)
	//{
	//	for (int i=0; i < m_DepthWidth-1; i++)
	//	{
	//		if ( j != 0 || i > 50)
	//		{
	//			break;
	//		}
	//		//
	//		//计算出三角形顶点的位置
	//		//
	//		PointIndex[0] = j * m_DepthWidth + i;
	//		PointIndex[1] = PointIndex[0] +1;
	//		PointIndex[2] = PointIndex[1] + m_DepthWidth;

	//		PointIndex[3] = PointIndex[0];
	//		PointIndex[4] = PointIndex[3] + m_DepthWidth;
	//		PointIndex[5] = PointIndex[4] + 1;

	//		for (int k = 0; k < POINT_INDEX_COUNT; k++)
	//		{
	//			int index = PointIndex[k];
	//			vertex[VertexIndex++]._position = D3DXVECTOR3(Points[index].x, Points[index].y, Points[index].z);
	//		}
	//		//
	//		//计算出顶点颜色
	//		//
	//		//调用api 对齐深度到彩色
	//		//int DepthIndex = i+j*m_DepthWidth;
	//		//LONG ColorInDepthX = ColorCoordinates[DepthIndex*2];
	//		//LONG ColorInDepthY = ColorCoordinates[DepthIndex*2+1];
	//		//if (ColorInDepthX >= 0 && ColorInDepthX < m_ColorWidth && ColorInDepthY < m_ColorHeight)
	//		//{
	//		//	LONG colorIndex = ColorInDepthX + ColorInDepthY * m_ColorWidth;
	//		//	vertex[i]._color = D3DCOLOR_ARGB(
	//		//		ColorData[colorIndex*MAX_COUNT+ALPHA],
	//		//		ColorData[colorIndex*MAX_COUNT+RED],
	//		//		ColorData[colorIndex*MAX_COUNT+GREEN],
	//		//		ColorData[colorIndex*MAX_COUNT+BLUE]);
	//		//}
	//		//else
	//		//{
	//		//	vertex[i]._color = D3DCOLOR_ARGB(0,255,255,255);
	//		//	ErrorPoint++;
	//		//}
	//	}
	//}

	//for (int i = 0; i < m_DepthWidth * m_DepthHeight; i++)
	//{
	//	vertex[i]._position = D3DXVECTOR3(Points[i].x /** SKELETON_SCALE*/, Points[i].y/** SKELETON_SCALE*/, Points[i].z /** SKELETON_SCALE*/);

	//	//调用api 对齐深度到彩色
	//	LONG ColorInDepthX = ColorCoordinates[i*2];
	//	LONG ColorInDepthY = ColorCoordinates[i*2+1];

	//	//手动对齐
	//	// 				int x = i % g_DepthWidth;
	//	// 				int y= i / g_DepthWidth;
	//	// 				LONG ColorInDepthX = int((x*604.0) / 640.0 + 0.5) + 22;
	//	// 				LONG ColorInDepthY = int((y*468.0) / 480.0 + 0.5) + 30;

	//	if (ColorInDepthX >= 0 && ColorInDepthX < m_ColorWidth && ColorInDepthY < m_ColorHeight)
	//	{
	//		LONG colorIndex = ColorInDepthX + ColorInDepthY * m_ColorWidth;
	//		vertex[i]._color = D3DCOLOR_ARGB(
	//			ColorData[colorIndex*MAX_COUNT+ALPHA],
	//			ColorData[colorIndex*MAX_COUNT+RED],
	//			ColorData[colorIndex*MAX_COUNT+GREEN],
	//			ColorData[colorIndex*MAX_COUNT+BLUE]);
	//	}
	//	else
	//	{
	//		vertex[i]._color = D3DCOLOR_ARGB(0,255,255,255);
	//		ErrorPoint++;
	//	}
	//}

	m_vb->Unlock();

	//HRESULT hr = D3DXCreateTextureFromFileInMemory(
	//	m_Device,
	//	ColorData,
	//	m_ColorWidth*m_ColorHeight*4,
	//	&m_Texure);

	//D3DXIMAGE_INFO ImageInfo;
	//PALETTEENTRY entry;
	//D3DXCreateTextureFromFileInMemoryEx(
	//	m_Device,
	//	ColorData,
	//	m_ColorWidth*m_ColorWidth*4,
	//	m_ColorWidth,
	//	m_ColorHeight,
	//	1,
	//	D3DUSAGE_DYNAMIC,
	//	D3DFMT_A8B8G8R8,
	//	D3DPOOL_DEFAULT,
	//	D3DX_FILTER_POINT,
	//	D3DX_DEFAULT,
	//	0xFF000000,
	//	&ImageInfo,
	//	&entry,
	//	&m_Texure);
		

	//D3DXIMAGE_INFO ImageInfo;
	//D3DXGetImageInfoFromFileInMemory(ColorData, m_ColorWidth*m_ColorHeight*4, &ImageInfo);

	m_Device->SetTexture(0,m_Texure);

	m_Device->SetMaterial(&d3d::WHITE_MTRL);
	m_Device->SetFVF(Vertex_Mesh::FVF);
	m_Device->SetStreamSource(0, m_vb, 0, sizeof(Vertex_Mesh));
	m_Device->SetIndices(m_ib);
	m_Device->DrawIndexedPrimitive(/*D3DPT_TRIANGLESTRIP*/D3DPT_TRIANGLELIST, 0, 0,  m_DepthWidth * m_DepthHeight, 0, IndexCount);

#pragma region 保存导出成.x

	if (bGenerateXFile)
	{
		static int frame = 0;
		if (frame%200 != 199)
		{
			frame++;
			return true;
		}

		frame++;
		//
		//用mesh生成.x模型文件
		//
		ID3DXMesh*  Mesh = 0;
		HRESULT hr = D3DXCreateMeshFVF(
			6 * (m_DepthWidth-1) * (m_DepthHeight-1) * sizeof(DWORD)/*IndexCount*/,
			m_DepthWidth * m_DepthHeight,
			D3DXMESH_32BIT | D3DXMESH_MANAGED,//D3DXMESH_DYNAMIC,
			Vertex_Mesh::FVF,
			m_Device,
			&Mesh);

		//拷贝顶点缓存
		Vertex_Mesh* vertex_Dest;
		Mesh->LockVertexBuffer(0, (void**)&vertex_Dest);
		Vertex_Mesh* vertex_src;
		m_vb->Lock(0, 0, (void**)&vertex_src, 0);
		memcpy(vertex_Dest, vertex_src, sizeof(Vertex_Mesh)*m_DepthWidth * m_DepthHeight);
		m_vb->Unlock();	
		Mesh->UnlockVertexBuffer();

		//拷贝索引缓存
		DWORD* indexData_Dest = 0;
		Mesh->LockIndexBuffer(0, (void**)&indexData_Dest);
		DWORD* indexData_Src;
		m_ib->Lock(0, 0, (void**)&indexData_Src, 0);
		memcpy(indexData_Dest, indexData_Src, 6 * (m_DepthWidth-1) * (m_DepthHeight-1) * sizeof(DWORD));
		m_ib->Unlock();
		Mesh->UnlockIndexBuffer();

		hr = D3DXSaveMeshToX(
			"mj.x",
			Mesh,
			NULL,//(DWORD*)adjBuffer,
			NULL,//mtrls,
			NULL,
			0,//numMtrls,
			D3DXF_FILEFORMAT_TEXT/*保存成Text格式以便用3DMax打开*/);
	}

#pragma endregion 保存导出成.x

	return true;
}