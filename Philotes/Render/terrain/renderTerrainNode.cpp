
#include "renderTerrainNode.h"
#include "renderTerrain.h"

_NAMESPACE_BEGIN

RenderTerrainNode::RenderTerrainNode( RenderTerrain* terrain, RenderTerrainNode* parent, 
											uint16 xoff, uint16 yoff,
											uint16 size,uint16 depth, uint16 quadrant )
											:m_terrain(terrain),
											m_parent(parent),
											m_offsetX(xoff),m_offsetY(yoff),
											m_size(size),m_depth(depth),
											m_quadRant(quadrant),
											m_boundingRadius(0)
{
	if (terrain->getMaxBatchSize() < size)
	{
		uint16 childSize	= (uint16)(((size - 1) * 0.5f) + 1);
		uint16 childOff		= childSize - 1;
		uint16 childDepth	= depth + 1;

		m_children[0] = ph_new(RenderTerrainNode)(terrain, this, xoff, yoff, childSize, childDepth, 0);
		m_children[1] = ph_new(RenderTerrainNode)(terrain, this, xoff + childOff, yoff, childSize, childDepth, 1);
		m_children[2] = ph_new(RenderTerrainNode)(terrain, this, xoff, yoff + childOff, childSize, childDepth, 2);
		m_children[3] = ph_new(RenderTerrainNode)(terrain, this, xoff + childOff, yoff + childOff, childSize, childDepth, 3);
	}
	else
	{
		// р╤вс╫з╣Ц
		memset(m_children, 0, sizeof(RenderTerrainNode*) * 4);

	}
}

RenderTerrainNode::~RenderTerrainNode()
{
	for (int i = 0; i < 4; ++i)
	{
		ph_delete(m_children[i]);
	}
}

void RenderTerrainNode::getWorldTransforms( Matrix4* xform ) const
{
	*xform = Matrix4::IDENTITY;
}

bool RenderTerrainNode::isLeaf()
{
	return m_children[0] == NULL;
}

void RenderTerrainNode::prepareData()
{

}

void RenderTerrainNode::loadData()
{
	if (!isLeaf())
	{
		for (int i = 0; i < 4; ++i)
		{
			m_children[i]->loadData();
		}
	}

}

void RenderTerrainNode::unloadData()
{
	if (!isLeaf())
	{
		for (int i = 0; i < 4; ++i)
		{
			m_children[i]->unloadData();
		}
	}
}

void RenderTerrainNode::assignVertexData( uint16 treeDepthStart, uint16 treeDepthEnd, 
										 uint16 resolution, uint sz )
{
	ph_assert2( treeDepthStart >= m_depth,"Should not be calling this" );

	if (m_depth == treeDepthStart)
	{
		//mNodeWithVertexData = this;
		//mVertexDataRecord = OGRE_NEW VertexDataRecord(resolution, sz, treeDepthEnd - treeDepthStart);

		//createCpuVertexData();

		// pass on to children
		if (!isLeaf() && treeDepthEnd > (m_depth + 1)) // treeDepthEnd is exclusive, and this is children
		{
			for (int i = 0; i < 4; ++i)
			{
				//mChildren[i]->useAncestorVertexData(this, treeDepthEnd, resolution);
			}
		}
	}
	else
	{
		ph_assert2(!isLeaf(),"No more levels below this!");

		for (int i = 0; i < 4; ++i)
		{
			m_children[i]->assignVertexData(treeDepthStart, treeDepthEnd, resolution, sz);
		}
	}
}

void RenderTerrainNode::createCpuVertexData()
{

}

void RenderTerrainNode::createGpuVertexData()
{

}

void RenderTerrainNode::destroyCpuVertexData()
{

}

void RenderTerrainNode::destroyGpuVertexData()
{

}


_NAMESPACE_END