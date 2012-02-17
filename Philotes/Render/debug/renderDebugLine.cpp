
#include "renderDebugLine.h"
#include "gearsAsset.h"
#include "gearsMaterialAsset.h"
#include "gearsAssetManager.h"
#include "gearsApplication.h"

#include "render.h"
#include "renderMesh.h"
#include "renderMeshDesc.h"
#include "renderVertexBuffer.h"
#include "renderVertexBufferDesc.h"
#include "renderIndexBuffer.h"
#include "renderMaterialInstance.h"
#include "renderTransform.h"

_NAMESPACE_BEGIN

RenderLineElement::RenderLineElement() : 
	RenderElement(), RenderTransformElement()
{
	init();
}

RenderLineElement::RenderLineElement( const String& name ) : 
	RenderElement(), RenderTransformElement(name)
{
	init();
}

RenderLineElement::~RenderLineElement()
{
	checkUnlock();

	if(m_vertexbuffer) 
	{
		m_vertexbuffer->release();
	}

	if(m_mesh)        
	{
		m_mesh->release();
	}

	if(m_materialAsset)
	{
		GearAssetManager::getSingleton()->returnAsset(*m_materialAsset);
	}
}

void RenderLineElement::init()
{
	m_materialAsset = GearMaterialAsset::getPrefabAsset(PM_UNLIGHT);

	ph_assert(m_materialAsset);
	ph_assert(m_materialAsset->getNumVertexShaders() == 1);

	setMaterialInstance(m_materialAsset ? new RenderMaterialInstance(*m_materialAsset->getMaterial(0)) : 0);

	m_maxVerts        = 0;
	m_numVerts        = 0;
	m_vertexbuffer    = 0;
	m_mesh            = 0;
	m_lockedPositions = 0;
	m_positionStride  = 0;
	m_lockedColors    = 0;
	m_colorStride     = 0;
}

void RenderLineElement::checkResizeLine( uint32 maxVerts )
{
	if(maxVerts > m_maxVerts)
	{
		m_maxVerts = maxVerts + (uint32)(maxVerts*0.2f);

		RenderVertexBufferDesc vbdesc;
		vbdesc.hint = RenderVertexBuffer::HINT_DYNAMIC;
		vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_POSITION] = RenderVertexBuffer::FORMAT_FLOAT3;
		vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_COLOR]    = RenderVertexBuffer::FORMAT_COLOR;
		vbdesc.maxVertices = m_maxVerts;
		RenderVertexBuffer *vertexbuffer = GearApplication::getApp()->getRender()->createVertexBuffer(vbdesc);
		ph_assert(vertexbuffer);

		if(vertexbuffer)
		{
			uint32 positionStride = 0;
			void *positions = vertexbuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_POSITION, positionStride);

			uint32 colorStride = 0;
			void *colors = vertexbuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_COLOR, colorStride);

			ph_assert(positions && colors);
			if(positions && colors)
			{
				if(m_numVerts > 0)
				{
					// if we have existing verts, copy them to the new VB...
					ph_assert(m_lockedPositions);
					ph_assert(m_lockedColors);
					if(m_lockedPositions && m_lockedColors)
					{
						for(uint32 i=0; i<m_numVerts; i++)
						{
							memcpy(((uint8*)positions) + (positionStride*i), ((uint8*)m_lockedPositions) + (m_positionStride*i), sizeof(Vector3));
							memcpy(((uint8*)colors)    + (colorStride*i),    ((uint8*)m_lockedColors)    + (m_colorStride*i),    sizeof(uint32));
						}
					}
					m_vertexbuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_COLOR);
					m_vertexbuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_POSITION);
				}
			}
			vertexbuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_COLOR);
			vertexbuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_POSITION);
		}
		if(m_vertexbuffer)
		{
			m_vertexbuffer->release();
			m_vertexbuffer    = 0;
			m_lockedPositions = 0;
			m_positionStride  = 0;
			m_lockedColors    = 0;
			m_colorStride     = 0;
		}
		if(m_mesh)
		{
			m_mesh->release();
			m_mesh = 0;
		}
		if(vertexbuffer)
		{
			m_vertexbuffer = vertexbuffer;
			RenderMeshDesc meshdesc;
			meshdesc.primitives       = RenderMesh::PRIMITIVE_LINES;
			meshdesc.vertexBuffers    = &m_vertexbuffer;
			meshdesc.numVertexBuffers = 1;
			meshdesc.firstVertex      = 0;
			meshdesc.numVertices      = m_numVerts;
			m_mesh = GearApplication::getApp()->getRender()->createMesh(meshdesc);
			ph_assert(m_mesh);
		}
		setMesh(m_mesh);
	}
}

void RenderLineElement::clearLine( void )
{
	m_numVerts = 0;
}

void RenderLineElement::checkLock( void )
{
	if(m_vertexbuffer)
	{
		if(!m_lockedPositions)
		{
			m_lockedPositions = m_vertexbuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_POSITION, m_positionStride);
			ph_assert(m_lockedPositions);
		}
		if(!m_lockedColors)
		{
			m_lockedColors = m_vertexbuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_COLOR, m_colorStride);
			ph_assert(m_lockedColors);
		}
	}
}

void RenderLineElement::checkUnlock( void )
{
	if(m_vertexbuffer)
	{
		if(m_lockedPositions)
		{
			m_vertexbuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_POSITION);
			m_lockedPositions = 0;
		}
		if(m_lockedColors)
		{
			m_vertexbuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_COLOR);
			m_lockedColors = 0;
		}
	}
}

void RenderLineElement::addPoint( const Vector3 &p, const Colour &color )
{
	ph_assert(m_maxVerts > m_numVerts);
	{
		checkLock();
		if(m_lockedPositions && m_lockedColors)
		{
			memcpy(((uint8*)m_lockedPositions) + (m_positionStride*m_numVerts), &p,     sizeof(Vector3));

			uint32 c = color.getAsARGB();
			memcpy(((uint8*)m_lockedColors)    + (m_colorStride*m_numVerts),    &c,		sizeof(uint32));
			m_numVerts++;
		}
	}
}

bool RenderLineElement::preQueuedToRender()
{
	if(getMesh())
	{
		checkUnlock();
		m_mesh->setVertexBufferRange(0, m_numVerts);
		return true;
	}
	return false;
}

void RenderLineElement::addLine( const Vector3 &p0,const Vector3 &p1,const Colour& color )
{
	checkResizeLine(m_numVerts+2);

	addPoint(p0,color);
	addPoint(p1,color);
}

_NAMESPACE_END
