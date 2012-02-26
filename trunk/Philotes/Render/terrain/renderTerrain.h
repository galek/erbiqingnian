
#pragma once

_NAMESPACE_BEGIN

struct TerrainDesc
{
	TerrainDesc()
	{
		terrainSize		= 1025;
		maxBatchSize	= 65;
		minBatchSize	= 17;
		pos				= Vector3::ZERO;
		worldSize		= 1000.0f;
	}
	uint16	terrainSize;
	uint16	maxBatchSize;
	uint16	minBatchSize;
	Vector3 pos;
	scalar	worldSize;
};

//////////////////////////////////////////////////////////////////////////

class RenderTerrain
{
public:

	RenderTerrain();

	virtual				~RenderTerrain();

public:

	static const uint16 TERRAIN_MAX_BATCH_SIZE;

	enum Alignment
	{
		ALIGN_X_Z = 0, 
		ALIGN_X_Y = 1, 
		ALIGN_Y_Z = 2
	};

	uint16				getMaxBatchSize() const;

	uint16				getMinBatchSize() const;

	uint16				getSize() const;

	void				prepareData(const TerrainDesc& desc);

	void				setPosition(const Vector3& pos);

	void				freeCpuResource();

	void				freeGpuResource();

	void				loadData();

	void				unloadData();

	// 是否使用顶点压缩
	bool				_getUseVertexCompression();

	float*				getHeightData() const;

	float*				getHeightData(long x, long y) const;

	float				getHeightAtPoint(long x, long y) const;

	void				getPointAlign(long x, long y, float height, Alignment align, Vector3* outpos);

	void				getPointAlign(long x, long y, Alignment align, Vector3* outpos);

	void				getPoint(long x, long y, Vector3* outpos);

	void				getPoint(long x, long y, float height, Vector3* outpos);


protected:

	void				distributeVertexData();

	void				updateBaseScale();

protected:

	Alignment			m_align;

	uint16				m_size;

	scalar				m_worldSize;

	scalar				m_base;

	scalar				m_scale;

	uint16				m_maxBatchSize;

	uint16				m_minBatchSize;

	Vector3				m_position;

	RenderTerrainNode*	m_quadTree;

	float*				m_heightData;

	uint16				m_treeDepth;
};

_NAMESPACE_END