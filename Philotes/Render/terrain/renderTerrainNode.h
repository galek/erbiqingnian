
#pragma once

#include "math/axisAlignedBox.h"
#include "renderElement.h"

_NAMESPACE_BEGIN

//class 

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

	void				assignVertexData(uint16 treeDepthStart, uint16 treeDepthEnd, uint16 resolution, uint sz);

protected:

	void				createCpuVertexData();

	void				destroyCpuVertexData();

	void				createGpuVertexData();

	void				destroyGpuVertexData();

protected:

	RenderTerrain*		m_terrain;

	RenderTerrainNode*	m_parent;

	RenderTerrainNode*	m_children[4];

	uint16				m_offsetX, m_offsetY;

	uint16				m_boundaryX, m_boundaryY;

	uint16				m_size;

	uint16				m_baseLod;

	uint16				m_depth;

	uint16				m_quadRant;

	Vector3				m_localCentre;

	AxisAlignedBox		m_AABB;

	scalar				m_boundingRadius;

};

_NAMESPACE_END