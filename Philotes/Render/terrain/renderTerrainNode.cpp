
#include "renderTerrainNode.h"
#include "renderTerrain.h"
#include "render.h"
#include "renderBase.h"
#include "renderVertexBuffer.h"
#include "renderVertexBufferDesc.h"
#include "renderIndexBuffer.h"
#include "renderIndexBufferDesc.h"
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
											m_renderNode(NULL)
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
	createGpuVertexData();
	createGpuIndexData();

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
		m_renderNode = this;
		m_renderDataRecord = ph_new(RenderDataRecord)(resolution, sz, treeDepthEnd - treeDepthStart);

		createCpuVertexData();

		if (!isLeaf() && treeDepthEnd > (m_depth + 1))
		{
			for (int i = 0; i < 4; ++i)
			{
				m_children[i]->useAncestorVertexData(this, treeDepthEnd, resolution);
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

void RenderTerrainNode::useAncestorVertexData( RenderTerrainNode* owner, uint16 treeDepthEnd, uint16 resolution )
{
	m_renderNode = owner;
	m_renderDataRecord = 0;

	if (!isLeaf() && treeDepthEnd > (m_depth + 1)) 
	{
		for (int i = 0; i < 4; ++i)
		{
			m_children[i]->useAncestorVertexData(owner, treeDepthEnd, resolution);
		}
	}
}

void RenderTerrainNode::createCpuVertexData()
{
	if (m_renderDataRecord)
	{
		destroyCpuVertexData();

		m_renderDataRecord->cpuVertex = GearApplication::getApp()->getRender()->createRenderBase();

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

		size_t baseNumVerts		= (size_t)Math::Sqr(m_renderDataRecord->size);
		size_t numVerts			= baseNumVerts;
		uint16 levels			= m_renderDataRecord->treeLevels;

		vbdesc.hint				= RenderVertexBuffer::HINT_STATIC;
		vbdesc.maxVertices		= numVerts;

		RenderVertexBuffer* vb	= GearApplication::getApp()->getRender()->createVertexBuffer(vbdesc);

		m_renderDataRecord->cpuVertex->appendVertexBuffer(vb);
		m_renderDataRecord->cpuVertex->setVertexBufferRange(0,numVerts);

		Rect updateRect(m_offsetX, m_offsetY, m_boundaryX, m_boundaryY);
		updateVertexBuffer(vb, updateRect);
	}
}

void RenderTerrainNode::createGpuVertexData()
{
	if (m_renderDataRecord && m_renderDataRecord->cpuVertex && !m_renderDataRecord->gpuVertex)
	{
		m_renderDataRecord->gpuVertex = GearApplication::getApp()->getRender()->createRenderBase();

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

		size_t baseNumVerts		= (size_t)Math::Sqr(m_renderDataRecord->size);
		size_t numVerts			= baseNumVerts;
		uint16 levels			= m_renderDataRecord->treeLevels;

		vbdesc.hint				= RenderVertexBuffer::HINT_STATIC;
		vbdesc.maxVertices		= numVerts;

		RenderVertexBuffer* vb	= GearApplication::getApp()->getRender()->createVertexBuffer(vbdesc);

		m_renderDataRecord->gpuVertex->appendVertexBuffer(vb);
		m_renderDataRecord->gpuVertex->setVertexBufferRange(0,numVerts);

		vb->copyData(m_renderDataRecord->cpuVertex->getVertexBuffer(0));
		m_renderDataRecord->gpuVertexDataDirty = false;

		destroyCpuVertexData();
	}
}

void RenderTerrainNode::destroyCpuVertexData()
{
	if (m_renderDataRecord && m_renderDataRecord->cpuVertex)
	{
		m_renderDataRecord->cpuVertex->release();
		m_renderDataRecord->cpuVertex = NULL;
	}
}

void RenderTerrainNode::destroyGpuVertexData()
{
	if (m_renderDataRecord && m_renderDataRecord->gpuVertex)
	{
		m_renderDataRecord->gpuVertex->release();
		m_renderDataRecord->gpuVertex = NULL;
	}
}

void RenderTerrainNode::updateVertexBuffer( RenderVertexBuffer* posbuf,const Rect& rect )
{
	ph_assert (rect.left >= m_offsetX && rect.right <= m_boundaryX && 
		rect.top >= m_offsetY && rect.bottom <= m_boundaryY);

	resetBounds(rect);

	uint16 inc = (m_terrain->getSize()-1) / (m_renderDataRecord->resolution-1);
	long destOffsetX = rect.left <= m_offsetX ? 0 : (rect.left - m_offsetX) / inc;
	long destOffsetY = rect.top <= m_offsetY ? 0 : (rect.top - m_offsetY) / inc;

	scalar uvScale = 1.0f / (m_terrain->getSize() - 1);
	const float* pBaseHeight = m_terrain->getHeightData(rect.left, rect.top);
	uint16 rowskip = m_terrain->getSize() * inc;
	uint16 destPosRowSkip = 0, destDeltaRowSkip = 0;
	unsigned char* pRootPosBuf = 0;
	unsigned char* pRowPosBuf = 0;

	if (posbuf)
	{
		destPosRowSkip = m_renderDataRecord->size * posbuf->getVertexSize();
		pRootPosBuf = static_cast<unsigned char*>(posbuf->lock());
		pRowPosBuf = pRootPosBuf;
		pRowPosBuf += destOffsetY * destPosRowSkip + destOffsetX * posbuf->getVertexSize();
	}

	Vector3 pos;
	bool vcompress = m_terrain->_getUseVertexCompression();

	for (long y = rect.top; y < rect.bottom; y += inc)
	{
		const float* pHeight = pBaseHeight;
		float* pPosBuf = static_cast<float*>(static_cast<void*>(pRowPosBuf));
		for (long x = rect.left; x < rect.right; x += inc)
		{
			if (pPosBuf)
			{
				m_terrain->getPoint(x, y, *pHeight, &pos);

				mergeIntoBounds(x, y, pos);
				
				pos -= m_localCentre;

				writePosVertex(vcompress, (uint16)x, (uint16)y, *pHeight, pos, uvScale, &pPosBuf);
				pHeight += inc;
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

void RenderTerrainNode::createGpuIndexData()
{
	if (m_renderDataRecord)
	{
		RenderIndexBufferDesc ibdesc;
		ibdesc.maxIndices = ;
		RenderIndexBuffer* ib = GearApplication::getApp()->getRender()->createIndexBuffer(ibdesc);
		m_renderDataRecord->gpuVertex->getIndexBuffer();
	}
}

void RenderTerrainNode::destroyGpuIndexData()
{
	if (m_renderDataRecord)
	{
		m_renderDataRecord->gpuVertex->release();
		m_renderDataRecord->gpuVertex = NULL;
	}
}

_NAMESPACE_END