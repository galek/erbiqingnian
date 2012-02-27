
#include "renderTerrainNode.h"
#include "renderTerrain.h"
#include "render.h"
#include "renderBase.h"
#include "renderVertexBuffer.h"
#include "renderVertexBufferDesc.h"
#include "renderIndexBuffer.h"
#include "renderIndexBufferDesc.h"
#include "renderMaterialInstance.h"
#include "gearsApplication.h"

_NAMESPACE_BEGIN

RenderTerrainNode::RenderTerrainNode( RenderTerrain* terrain, RenderTerrainNode* parent, 
											uint16 xoff, uint16 yoff,
											uint16 size,uint16 depth, uint16 quadrant )
											:m_terrain(terrain),
											m_parent(parent),
											m_offsetX(xoff),m_offsetY(yoff),
											m_size(size),m_depth(depth),
											m_boundaryX(xoff + size),
											m_boundaryY(yoff + size),
											m_quadRant(quadrant),
											m_boundingRadius(0),
											m_localCentre(Vector3::ZERO),
											m_renderData(NULL)
{
	if (terrain->getBatchSize() < size)
	{
		uint16 childSize	= (uint16)(((size - 1) * 0.5f) + 1);
		uint16 childOff		= childSize - 1;
		uint16 childDepth	= depth + 1;

		m_children[0] = ph_new(RenderTerrainNode)(terrain, this, xoff,				yoff, 			 childSize, childDepth, 0);
		m_children[1] = ph_new(RenderTerrainNode)(terrain, this, xoff + childOff,	yoff, 			 childSize, childDepth, 1);
		m_children[2] = ph_new(RenderTerrainNode)(terrain, this, xoff,				yoff + childOff, childSize, childDepth, 2);
		m_children[3] = ph_new(RenderTerrainNode)(terrain, this, xoff + childOff,	yoff + childOff, childSize, childDepth, 3);
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
	if (!isLeaf())
	{
		for (int i = 0; i < 4; ++i)
		{
			m_children[i]->prepareData();
		}
	}
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
	else
	{
		createRenderData();
		setMaterialInstance(m_terrain->getMaterialInstance());
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
	else
	{
		destroyRenderData();
	}
}

void RenderTerrainNode::updateVertexBuffer( RenderVertexBuffer* posbuf,const Rect& rect )
{
	ph_assert (rect.left >= m_offsetX && rect.right <= m_boundaryX && 
		rect.top >= m_offsetY && rect.bottom <= m_boundaryY);

	resetBounds(rect);

	long destOffsetX = rect.left <= m_offsetX ? 0 : (rect.left - m_offsetX);
	long destOffsetY = rect.top	 <= m_offsetY ? 0 : (rect.top  - m_offsetY);

	scalar uvScale = 1.0f / (m_terrain->getSize() - 1);
	const float* pBaseHeight = m_terrain->getHeightData(rect.left, rect.top);
	uint16 rowskip = m_terrain->getSize();
	uint16 destPosRowSkip = 0, destDeltaRowSkip = 0;
	unsigned char* pRootPosBuf	= 0;
	unsigned char* pRowPosBuf	= 0;

	if (posbuf)
	{
		destPosRowSkip	= m_terrain->getBatchSize() * posbuf->getVertexSize();
		pRootPosBuf		= static_cast<unsigned char*>(posbuf->lock());
		pRowPosBuf		= pRootPosBuf;
		pRowPosBuf		+= destOffsetY * destPosRowSkip + destOffsetX * posbuf->getVertexSize();
	}

	Vector3 pos;
	bool vcompress = m_terrain->_getUseVertexCompression();

	for (long y = rect.top; y < rect.bottom; y ++)
	{
		const float* pHeight = pBaseHeight;
		float* pPosBuf = static_cast<float*>(static_cast<void*>(pRowPosBuf));
		for (long x = rect.left; x < rect.right; x ++)
		{
			if (pPosBuf)
			{
				m_terrain->getPoint(x, y, *pHeight, &pos);

				mergeIntoBounds(x, y, pos);
				
				pos -= m_localCentre;

				writePosVertex(vcompress, (uint16)x, (uint16)y, *pHeight, pos, uvScale, &pPosBuf);
				pHeight ++;
			}

		}
		pBaseHeight += rowskip;
		if (pRowPosBuf)
		{
			pRowPosBuf += destPosRowSkip;
		}
	}

	if (posbuf)
	{
		posbuf->unlock();
	}
}

void RenderTerrainNode::resetBounds( const Rect& rect )
{
	if (rectContainsNode(rect))
	{
		m_AABB.setNull();
		m_boundingRadius = 0;

		if (!isLeaf())
		{
			for (int i = 0; i < 4; ++i)
			{
				m_children[i]->resetBounds(rect);
			}
		}
	}
}

bool RenderTerrainNode::rectContainsNode( const Rect& rect )
{
	return (rect.left <= m_offsetX && rect.right > m_boundaryX &&
		rect.top <= m_offsetY && rect.bottom > m_boundaryY);
}

bool RenderTerrainNode::rectIntersectsNode( const Rect& rect )
{
	return (rect.right >= m_offsetX && rect.left <= m_boundaryX &&
		rect.bottom >= m_offsetY && rect.top <= m_boundaryY);
}

void RenderTerrainNode::mergeIntoBounds( long x, long y, const Vector3& pos )
{
	if (pointIntersectsNode(x, y))
	{
		Vector3 localPos = pos - m_localCentre;
		m_AABB.merge(localPos);
		m_boundingRadius = Math::Max(m_boundingRadius, localPos.length());

		if (!isLeaf())
		{
			for (int i = 0; i < 4; ++i)
			{
				m_children[i]->mergeIntoBounds(x, y, pos);
			}
		}
	}
}

bool RenderTerrainNode::pointIntersectsNode( long x, long y )
{
	return x >= m_offsetX && x < m_boundaryX && 
		y >= m_offsetY && y < m_boundaryY;
}

void RenderTerrainNode::writePosVertex( bool compress, uint16 x, uint16 y, 
									   float height, const Vector3& pos,
									   float uvScale, float** ppPos )
{
	float* pPosBuf = *ppPos;

	if (compress)
	{
		short* pPosShort = static_cast<short*>(static_cast<void*>(pPosBuf));
		*pPosShort++ = (short)x;
		*pPosShort++ = (short)y;
		pPosBuf = static_cast<float*>(static_cast<void*>(pPosShort));

		*pPosBuf++ = height;
	}
	else 
	{
		*pPosBuf++ = pos.x;
		*pPosBuf++ = pos.y;
		*pPosBuf++ = pos.z;

		*pPosBuf++ = x * uvScale;
		*pPosBuf++ = 1.0f - (y * uvScale);
	}

	*ppPos = pPosBuf;
}

void RenderTerrainNode::createRenderData()
{
	if (isLeaf())
	{
		m_renderData = GearApplication::getApp()->getRender()->createRenderBase();

		RenderVertexBufferDesc vbdesc;

		if (m_terrain->_getUseVertexCompression())
		{
			vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_POSITION]	= RenderVertexBuffer::FORMAT_USHORT4;
			vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_TEXCOORD0]	= RenderVertexBuffer::FORMAT_FLOAT1;
		}
		else
		{
			vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_POSITION]	= RenderVertexBuffer::FORMAT_FLOAT3;
			vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_TEXCOORD0]	= RenderVertexBuffer::FORMAT_FLOAT2;
		}

		size_t baseNumVerts		= (size_t)Math::Sqr(m_terrain->getBatchSize());
		size_t numVerts			= baseNumVerts;

		vbdesc.hint				= RenderVertexBuffer::HINT_STATIC;
		vbdesc.maxVertices		= numVerts;

		RenderVertexBuffer* vb	= GearApplication::getApp()->getRender()->createVertexBuffer(vbdesc);

		m_renderData->appendVertexBuffer(vb);
		m_renderData->setVertexBufferRange(0,numVerts);
		m_renderData->setIndexBuffer(m_terrain->getIndexBuffer());
		m_renderData->setIndexBufferRange(0,m_terrain->getIndexBuffer()->getMaxIndices());
		m_renderData->setPrimitives(RenderBase::PRIMITIVE_TRIANGLE_STRIP);

		Rect updateRect(m_offsetX, m_offsetY, m_boundaryX, m_boundaryY);
		updateVertexBuffer(vb, updateRect);

		setMesh(m_renderData);
	}
}

void RenderTerrainNode::destroyRenderData()
{
	if (m_renderData)
	{
		if (m_renderData->getVertexBuffer(0))
		{
			m_renderData->getVertexBuffer(0)->release();
		}
		m_renderData->release();
		m_renderData = NULL;
	}
}

void RenderTerrainNode::walkQuadTree( Array<RenderElement*>& visible )
{
	if (!isLeaf())
	{
		for (int i = 0; i < 4; ++i)
		{
			m_children[i]->walkQuadTree(visible);
		}
	}
	else
	{
		visible.Append(this);
	}
}

_NAMESPACE_END