
#pragma once

#include "renderTransformElement.h"

_NAMESPACE_BEGIN

// ���������
struct TerrainLayer
{
	TerrainLayer(){}

	TerrainLayer(scalar sz, const String& diff, const String& normal)
	:worldSize(sz),diffuseTexture(diff),normalTexture(normal)
	{}

	scalar	worldSize;
	String	diffuseTexture;
	String	normalTexture;

	// ������������4
	static const uint8 MAX_LAYER;
};
typedef Array<TerrainLayer> LayerList;

// �������������ڴ�������
struct TerrainDesc
{
	TerrainDesc();

	bool		validate() const;

	// ��������ֵ����Ϊ2����+1
	uint16		terrainSize;
	uint16		batchSize;

	Vector3		pos;
	scalar		worldSize;

	scalar		heightScale;
	scalar		heightBias;

	// ������һ�������
	LayerList	layers;
};

//////////////////////////////////////////////////////////////////////////

class RenderTerrain : public RenderTransformElement
{
public:

	RenderTerrain(RenderSceneManager* smg);

	virtual					~RenderTerrain();

public:

	static const uint16 TERRAIN_MAX_BATCH_SIZE;

	uint16					getBatchSize() const;

	uint16					getSize() const;

	scalar					getWorldSize() const;

	void					prepareData(const TerrainDesc& desc);

	void					setPosition(const Vector3& pos);

	void					freeCpuResource();

	void					freeGpuResource();

	void					loadData();

	void					unloadData();

	// �Ƿ�ʹ�ö���ѹ��
	bool					getUseVertexCompression();

	float*					getHeightData() const;

	float*					getHeightData(long x, long y) const;

	float					getHeightAtPoint(long x, long y) const;

	void					getPoint(long x, long y, Vector3* outpos);

	void					getPoint(long x, long y, float height, Vector3* outpos);

	static size_t			_getNumIndexesForBatchSize(uint16 batchSize);

	const AxisAlignedBox&	getBoundingBox(void) const;

	scalar					getBoundingRadius(void) const;

	void					visitRenderElement(RenderVisitor* visitor);

	RenderMaterialInstance* getMaterialInstance() const { return m_materialInstance; }

	RenderIndexBuffer*		getIndexBuffer();

	void					setLayerWorldSize(uint8 index, scalar size);

	void					setLayerDiffuseTexture(uint8 index, const String& diffuses);

	void					setLayerNormalTexture(uint8 index, const String& normal);

	const TerrainLayer&		getLayer(uint8 index) const;

	bool					addLayer(scalar worldSize, const String& diffuse, const String& normal);

	// �ü�
	void					cull(RenderCamera* camera);


protected:

	void					distributeVertexData();

	void					updateBaseScale();

	void					createIndexBuffer();

	void					destroyIndexBuffer();

	Vector4					getLayersUvMuler();

protected:

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

	LayerList				m_layers;
};

_NAMESPACE_END