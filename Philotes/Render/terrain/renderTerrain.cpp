
#include "renderTerrain.h"
#include "renderTerrainNode.h"
#include "renderSceneManager.h"
#include "renderCellNode.h"
#include "renderTransform.h"
#include "renderCamera.h"
#include "renderMaterialInstance.h"
#include "renderIndexBuffer.h"
#include "renderIndexBufferDesc.h"
#include "render.h"
#include "gearsMaterialAsset.h"
#include "gearsAssetManager.h"
#include "gearsApplication.h"

#include "util/bitwise.h"

_NAMESPACE_BEGIN

TerrainDesc::TerrainDesc()
{
	terrainSize		= 1025;
	batchSize		= 65;
	pos				= Vector3::ZERO;
	worldSize		= 1000.0f;
	heightScale		= 1;
	heightBias		= 0;
}

bool TerrainDesc::validate() const
{
	// 数值必须为2次幂+1
	if (!Bitwise::isPO2(terrainSize - 1)||
		!Bitwise::isPO2(batchSize - 1))
	{
		return false;
	}

	// 至少有一层
	if(layers.IsEmpty())
	{
		return false;
	}

	if (layers.Size() > (SizeT)TerrainLayer::MAX_LAYER)
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

RenderTerrain::RenderTerrain(RenderSceneManager* smg)
	:m_quadTree(NULL),
	m_batchSize(0),
	m_heightData(NULL),
	m_size(0),
	m_treeDepth(0),
	m_align(ALIGN_X_Z),
	m_cellNode(NULL),
	m_sceneMgr(smg),
	m_materialAsset(NULL),
	m_materialInstance(NULL),
	m_indexBuffer(NULL)
{

}

RenderTerrain::~RenderTerrain()
{
	freeCpuResource();
	freeGpuResource();
}

uint16 RenderTerrain::getBatchSize() const
{
	return m_batchSize;
}

void RenderTerrain::prepareData( const TerrainDesc& desc )
{
	if (!desc.validate())
	{
		PH_EXCEPT(ERR_RENDER,"Invalid terrain description.");
	}

	m_size			= desc.terrainSize;
	m_batchSize		= desc.batchSize;
	m_worldSize		= desc.worldSize;

	for (SizeT i=0; i<desc.layers.Size();i++)
	{
		addLayer(desc.layers[i].worldSize,
			desc.layers[i].diffuseTexture,desc.layers[i].normalTexture);
	}

	setPosition(desc.pos);
	updateBaseScale();

	size_t numVertices = m_size * m_size;
	m_heightData = new float[numVertices];

	// 初始化地形高度数据
	//memset(m_heightData, 0, sizeof(float) * m_size * m_size);
	// 测试数据
	FILE* fp = fopen("../media/terrainData/heightData.bin","rb");
	fread(m_heightData,sizeof(float),numVertices,fp);
	fclose(fp);

	if (!Math::RealEqual(desc.heightBias, 0.0) || !Math::RealEqual(desc.heightScale, 1.0))
	{
		float* src = m_heightData;
		float* dst = m_heightData;
		for (size_t i = 0; i < numVertices; ++i)
		{
			*dst++ = (*src++ * desc.heightScale) + desc.heightBias;
		}
	}

	m_treeDepth = (uint16)(Math::Log2(scalar(m_size - 1)) - Math::Log2(scalar(m_batchSize - 1)) );

	m_quadTree	= ph_new(RenderTerrainNode)(this, 0, 0, 0, m_size, 0, 0);
	m_quadTree->prepareData();

	// 分配顶点数据
	distributeVertexData();
}

void RenderTerrain::setPosition( const Vector3& pos )
{
	m_position = pos;
}

void RenderTerrain::freeCpuResource()
{
	if (m_heightData)
	{
		delete[] m_heightData;
		m_heightData = NULL;
	}

	if (m_quadTree)
	{
		ph_delete(m_quadTree);
		m_quadTree = NULL;
	}
}

void RenderTerrain::freeGpuResource()
{

}

void RenderTerrain::loadData()
{
	m_materialAsset = static_cast<GearMaterialAsset*>(GearAssetManager::getSingleton(
		)->getAsset("materials/sampleterrain.xml", GearAsset::ASSET_MATERIAL));
	m_materialInstance = m_materialAsset->getMaterialInstance(0);
	m_materialInstance->getMaterial().setCullMode(RenderMaterial::COUNTER_CLOCKWISE);

	const RenderMaterial::Variable* var = m_materialInstance->findVariable("g_uvMul",RenderMaterial::VARIABLE_FLOAT4);
	if (var)
	{
		Vector4 uvm = getLayersUvMuler();
		m_materialInstance->writeData(*var,(void*)(&uvm));
	}

	createIndexBuffer();

	if (m_quadTree)
	{
		m_quadTree->loadData();
	}

	m_cellNode = m_sceneMgr->createCellNode("_terrain");
	m_sceneMgr->getRootCellNode()->addChild(m_cellNode);
	RenderTransform* tn = m_cellNode->createChildTransformNode();
	tn->attachObject(this);
	tn->setPosition(m_position);
}

void RenderTerrain::unloadData()
{
	if (m_quadTree)
	{
		m_quadTree->unloadData();
	}

	destroyIndexBuffer();

	RenderNode* ch = m_cellNode->getChild(0);
	RenderTransform* tn = static_cast<RenderTransform*>(m_cellNode->removeChild(ch));
	tn->detachAllObjects();
	ph_delete(tn);
	m_sceneMgr->destroyCellNode(m_cellNode);
	m_cellNode = NULL;

// 	if (m_materialInstance)
// 	{
// 		ph_delete(m_materialInstance);
// 	}
	if(m_materialAsset)
	{
		GearAssetManager::getSingleton()->returnAsset(*m_materialAsset);
	}
}

void RenderTerrain::distributeVertexData()
{
}

bool RenderTerrain::getUseVertexCompression()
{
	return false;
}

float* RenderTerrain::getHeightData() const
{
	return m_heightData;
}

float* RenderTerrain::getHeightData( long x, long y ) const
{
	ph_assert (x >= 0 && x < m_size && y >= 0 && y < m_size);
	return &m_heightData[y * m_size + x];
}

float RenderTerrain::getHeightAtPoint( long x, long y ) const
{
	x = Math::Min(x, (long)m_size - 1L);
	x = Math::Max(x, 0L);
	y = Math::Min(y, (long)m_size - 1L);
	y = Math::Max(y, 0L);

	return *getHeightData(x, y);
}

uint16 RenderTerrain::getSize() const
{
	return m_size;
}

void RenderTerrain::getPoint( long x, long y, Vector3* outpos )
{
	getPointAlign(x, y, m_align, outpos);
}

void RenderTerrain::getPoint( long x, long y, float height, Vector3* outpos )
{
	getPointAlign(x, y, height, m_align, outpos);
}

void RenderTerrain::getPointAlign( long x, long y, float height, Alignment align, Vector3* outpos )
{
	switch(align)
	{
	case ALIGN_X_Z:
		outpos->y = height;
		outpos->x = x * m_scale + m_base;
		outpos->z = y * -m_scale - m_base;
		break;
	case ALIGN_Y_Z:
		outpos->x = height;
		outpos->z = x * -m_scale - m_base;
		outpos->y = y * m_scale + m_base;
		break;
	case ALIGN_X_Y:
		outpos->z = height;
		outpos->x = x * m_scale + m_base;
		outpos->y = y * m_scale + m_base;
		break;
	}
}

void RenderTerrain::getPointAlign( long x, long y, Alignment align, Vector3* outpos )
{
	getPointAlign(x, y, *getHeightData(x, y), align, outpos);
}

void RenderTerrain::updateBaseScale()
{
	m_base = -m_worldSize * 0.5f; 
	m_scale =  m_worldSize / (scalar)(m_size - 1);
}

size_t RenderTerrain::_getNumIndexesForBatchSize( uint16 batchSize )
{
	size_t mainIndexesPerRow = batchSize * 2 + 1;
	size_t numRows = batchSize - 1;
	return mainIndexesPerRow * numRows;
}

Philo::scalar RenderTerrain::getBoundingRadius( void ) const
{
	return Math::POS_INFINITY;
}

const AxisAlignedBox& RenderTerrain::getBoundingBox( void ) const
{
	static AxisAlignedBox aabb;
	aabb.setInfinite();
	return aabb;
}

void RenderTerrain::visitRenderElement( RenderVisitor* visitor )
{
	Array<RenderElement*>::Iterator it;
	Array<RenderElement*>::Iterator itend = m_nodesToRender.End();
	for (it = m_nodesToRender.Begin(); it!=itend; ++it)
	{
		visitor->visit(*it);
	}
}

void RenderTerrain::cull( RenderCamera* camera )
{
	m_nodesToRender.Reset();

	if (m_quadTree)
	{
		m_quadTree->walkQuadTree(camera,m_nodesToRender);
	}
}

void RenderTerrain::createIndexBuffer()
{
	/* 地形的三角带示意图如下:
	6---7---8
	| \ | \ |
	3---4---5
	| / | / |
	0---1---2
	索引：(2,5,1,4,0,3,3,6,4,7,5,8)
	*/
	uint16 batchSize = getBatchSize();

	RenderIndexBufferDesc ibdesc;
	ibdesc.maxIndices = RenderTerrain::_getNumIndexesForBatchSize(batchSize);

	m_indexBuffer = GearApplication::getApp()->getRender()->createIndexBuffer(ibdesc);

	uint16* pI = static_cast<uint16*>(m_indexBuffer->lock());
	size_t rowSize = batchSize;
	size_t numRows = batchSize - 1;

	uint16 currentVertex = batchSize - 1;

	bool rightToLeft = true; //左右交替
	for (uint16 r = 0; r < numRows; ++r)
	{
		for (uint16 c = 0; c < batchSize; ++c)
		{
			*pI++ = currentVertex;
			*pI++ = currentVertex + rowSize;

			if (c+1 < batchSize)
			{
				currentVertex = rightToLeft ? currentVertex - 1 : currentVertex + 1;
			}				
		}
		rightToLeft = !rightToLeft;
		currentVertex += rowSize;
		*pI++ = currentVertex;
	}
	m_indexBuffer->unlock();
}

void RenderTerrain::destroyIndexBuffer()
{
	if (m_indexBuffer)
	{
		m_indexBuffer->release();
		m_indexBuffer = NULL;
	}
}

RenderIndexBuffer* RenderTerrain::getIndexBuffer()
{
	return m_indexBuffer;
}

void RenderTerrain::setLayerWorldSize( uint8 index, scalar size )
{
	if (index < m_layers.Size())
	{
		m_layers[index].worldSize = size;
	}
}

const TerrainLayer& RenderTerrain::getLayer( uint8 index ) const
{
	ph_assert(index<m_layers.Size());
	return m_layers[index];
}

void RenderTerrain::setLayerDiffuseTexture( uint8 index, const String& diffuses )
{
	if (index < m_layers.Size())
	{
		m_layers[index].diffuseTexture = diffuses;
	}
}

void RenderTerrain::setLayerNormalTexture( uint8 index, const String& normal )
{
	if (index < m_layers.Size())
	{
		m_layers[index].normalTexture = normal;
	}
}

bool RenderTerrain::addLayer( scalar worldSize, const String& diffuse, const String& normal )
{
	if (m_layers.Size() == TerrainLayer::MAX_LAYER)
	{
		return false;
	}
	TerrainLayer layer;
	layer.worldSize			= worldSize;
	layer.diffuseTexture	= diffuse;
	layer.normalTexture		= normal;
	m_layers.Append(layer);

	return true;
}

Vector4 RenderTerrain::getLayersUvMuler()
{
	Vector4 uvm(1,1,1,1);
	if (m_layers.Size() >= 1)
	{
		uvm.x = m_worldSize / m_layers[0].worldSize;
	}
	if (m_layers.Size() >= 2)
	{
		uvm.y = m_worldSize / m_layers[1].worldSize;
	}
	if (m_layers.Size() >= 3)
	{
		uvm.z = m_worldSize / m_layers[2].worldSize;
	}
	if (m_layers.Size() >= 4)
	{
		uvm.w = m_worldSize / m_layers[3].worldSize;
	}
	return uvm;
}

Philo::scalar RenderTerrain::getWorldSize() const
{
	return m_worldSize;
}

const uint16	RenderTerrain::TERRAIN_MAX_BATCH_SIZE = 129;
const uint8		TerrainLayer::MAX_LAYER = 4;

_NAMESPACE_END

