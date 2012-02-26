
#pragma once

#include "math/axisAlignedBox.h"
#include "renderElement.h"

_NAMESPACE_BEGIN

//////////////////////////////////////////////////////////////////////////

class RenderTerrainNode : public RenderElement
{
public:

	RenderTerrainNode(RenderTerrain* terrain, RenderTerrainNode* parent,
		uint16 xoff, uint16 yoff, uint16 size,uint16 depth, uint16 quadrant);

	virtual				~RenderTerrainNode();

public:

	void				getWorldTransforms(Matrix4* xform) const;

	bool				isLeaf();

	void				prepareData();

	void				loadData();

	void				unloadData();

	struct RenderDataRecord
	{
		RenderDataRecord(uint16 res, uint16 sz, uint16 lvls)
		{
			cpuVertex 			= NULL;
			gpuVertex 			= NULL;
			resolution			= res;
			size				= sz;
			treeLevels			= lvls;
			gpuVertexDataDirty	= false;
		}

		RenderBase*		cpuVertex;
		RenderBase*		gpuVertex;
		uint16			resolution;
		uint16			size;
		uint16			treeLevels;
		bool			gpuVertexDataDirty;

	};

	void				assignVertexData(uint16 treeDepthStart, uint16 treeDepthEnd, uint16 resolution, uint sz);

	void				useAncestorVertexData(RenderTerrainNode* owner, uint16 treeDepthEnd, uint16 resolution);

	void				resetBounds(const Rect& rect);

	void				mergeIntoBounds(long x, long y, const Vector3& pos);

	bool				rectContainsNode(const Rect& rect);

	bool				rectIntersectsNode(const Rect& rect);

	bool				pointIntersectsNode(long x, long y);

protected:

	void				createCpuVertexData();

	void				destroyCpuVertexData();

	void				createGpuVertexData();

	void				destroyGpuVertexData();

	void				createGpuIndexData();

	void				destroyGpuIndexData();

	void				updateVertexBuffer(RenderVertexBuffer* posbuf, const Rect& rect);

	void				writePosVertex(bool compress, uint16 x, uint16 y, float height, const Vector3& pos, float uvScale, float** ppPos);

protected:

	RenderTerrain*		m_terrain;

	RenderTerrainNode*	m_parent;

	RenderTerrainNode*	m_children[4];

	RenderTerrainNode*	m_renderNode;

	uint16				m_offsetX, m_offsetY;

	uint16				m_boundaryX, m_boundaryY;

	uint16				m_size;

	uint16				m_baseLod;

	uint16				m_depth;

	uint16				m_quadRant;

	Vector3				m_localCentre;

	AxisAlignedBox		m_AABB;

	scalar				m_boundingRadius;

	RenderDataRecord*	m_renderDataRecord;

};

_NAMESPACE_END