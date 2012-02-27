
#pragma once

#include "renderTransformElement.h"

_NAMESPACE_BEGIN

struct TerrainDesc
{
	TerrainDesc()
	{
		terrainSize		= 1025;
		batchSize		= 65;
		pos				= Vector3::ZERO;
		worldSize		= 1000.0f;
	}
	uint16	terrainSize;
	uint16	batchSize;
	Vector3 pos;
	scalar	worldSize;
};

//////////////////////////////////////////////////////////////////////////

class RenderTerrain : public RenderTransformElement
{
public:

	RenderTerrain(RenderSceneManager* smg);

	virtual					~RenderTerrain();

public:

	static const uint16 TERRAIN_MAX_BATCH_SIZE;

	enum Alignment
	{
		ALIGN_X_Z = 0, 
		ALIGN_X_Y = 1, 
		ALIGN_Y_Z = 2
	};

	uint16					getBatchSize() const;

	uint16					getSize() const;

	void					prepareData(const TerrainDesc& desc);

	void					setPosition(const Vector3& pos);

	void					freeCpuResource();

	void					freeGpuResource();

	void					loadData();

	void					unloadData();

	// 是否使用顶点压缩
	bool					_getUseVertexCompression();

	float*					getHeightData() const;

	float*					getHeightData(long x, long y) const;

	float					getHeightAtPoint(long x, long y) const;

	void					getPointAlign(long x, long y, float height, Alignment align, Vector3* outpos);

	void					getPointAlign(long x, long y, Alignment align, Vector3* outpos);

	void					getPoint(long x, long y, Vector3* outpos);

	void					getPoint(long x, long y, float height, Vector3* outpos);

	static size_t			_getNumIndexesForBatchSize(uint16 batchSize);

	const AxisAlignedBox&	getBoundingBox(void) const;

	scalar					getBoundingRadius(void) const;

	void					visitRenderElement(RenderVisitor* visitor);

	RenderMaterialInstance* getMaterialInstance() const { return m_materialInstance; }

	RenderIndexBuffer*		getIndexBuffer();

	// 裁剪
	void					cull(RenderCamera* camera);


protected:

	void					distributeVertexData();

	void					updateBaseScale();

	void					createIndexBuffer();

	void					destroyIndexBuffer();

protected:

	Alignment				m_align;

	uint16					m_size;

	scalar					m_worldSize;

	scalar					m_base;

	scalar					m_scale;

	uint16					m_batchSize;

	RenderTerrainNode*		m_quadTree;

	float*					m_heightData;

	uint16					m_treeDepth;

	RenderSceneManager* 	m_sceneMgr;

	RenderCellNode*			m_cellNode;

	Array<RenderElement*>	m_nodesToRender;

	GearMaterialAsset*		m_materialAsset;

	RenderMaterialInstance*	m_materialInstance;

	RenderIndexBuffer*		m_indexBuffer;

	Vector3					m_position;
};

_NAMESPACE_END