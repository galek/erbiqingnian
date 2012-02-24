
#include "renderTerrain.h"
#include "renderTerrainNode.h"

_NAMESPACE_BEGIN

RenderTerrain::RenderTerrain()
	:m_quadTree(NULL),
	m_position(Vector3::ZERO),
	m_maxBatchSize(0),
	m_minBatchSize(0),
	m_heightData(NULL),
	m_size(0),
	m_treeDepth(0)
{

}

RenderTerrain::~RenderTerrain()
{
	freeCpuResource();
	freeGpuResource();
}

uint16 RenderTerrain::getMaxBatchSize() const
{
	return m_maxBatchSize;
}

uint16 RenderTerrain::getMinBatchSize() const
{
	return m_minBatchSize;
}

void RenderTerrain::prepareData( const TerrainDesc& desc )
{
	m_size			= desc.terrainSize;
	m_minBatchSize	= desc.minBatchSize;
	m_maxBatchSize	= desc.maxBatchSize;
	m_worldSize		= desc.worldSize;

	setPosition(desc.pos);

	size_t numVertices = m_size * m_size;

	m_heightData = new float[numVertices];

	// 初始化地形高度数据
	memset(m_heightData, 0, sizeof(float) * m_size * m_size);

	m_treeDepth = (uint16)(Math::Log2(scalar(m_maxBatchSize - 1)) - Math::Log2(scalar(m_minBatchSize - 1)) + 2);

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
	if (m_quadTree)
	{
		m_quadTree->loadData();
	}
}

void RenderTerrain::unloadData()
{
	if (m_quadTree)
	{
		m_quadTree->unloadData();
	}
}

void RenderTerrain::distributeVertexData()
{
	uint16 depth = m_treeDepth;
	uint16 prevdepth = depth;
	uint16 currresolution = m_size;
	uint16 bakedresolution = m_size;
	uint16 targetSplits = (bakedresolution - 1) / (TERRAIN_MAX_BATCH_SIZE - 1);
	while(depth-- && targetSplits)
	{
		uint splits = 1 << depth;
		if (splits == targetSplits)
		{
			size_t sz = ((bakedresolution-1) / splits) + 1;
			m_quadTree->assignVertexData(depth, prevdepth, bakedresolution, sz);

			bakedresolution =  ((currresolution - 1) >> 1) + 1;
			targetSplits = (bakedresolution - 1) / (TERRAIN_MAX_BATCH_SIZE - 1);
			prevdepth = depth;

		}

		currresolution = ((currresolution - 1) >> 1) + 1;
	}

	if (prevdepth > 0)
	{
		m_quadTree->assignVertexData(0, 1, bakedresolution, bakedresolution);

	}

}

bool RenderTerrain::_getUseVertexCompression()
{
	return true;
}

const uint16 RenderTerrain::TERRAIN_MAX_BATCH_SIZE = 129;

_NAMESPACE_END