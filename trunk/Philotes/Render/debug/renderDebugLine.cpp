
#include "renderDebugLine.h"
#include "gearsAsset.h"
#include "gearsMaterialAsset.h"
#include "gearsAssetManager.h"
#include "render.h"
#include "renderMesh.h"
#include "renderMeshDesc.h"
#include "renderVertexBuffer.h"
#include "renderVertexBufferDesc.h"
#include "renderIndexBuffer.h"

_NAMESPACE_BEGIN

RendererDebugLine::RendererDebugLine( Renderer &renderer, GearAssetManager &assetmanager )
	:m_renderer(renderer),
	m_assetmanager(assetmanager)
{
	m_material = static_cast<GearMaterialAsset*>(m_assetmanager.getAsset("materials/simple_unlit.xml", GearAsset::ASSET_MATERIAL));
	ph_assert(m_material);
	ph_assert(m_material->getNumVertexShaders() == 1);
	m_meshContext.material = m_material ? m_material->getMaterial(0) : 0;

	m_maxVerts        = 0;
	m_numVerts        = 0;
	m_vertexbuffer    = 0;
	m_mesh            = 0;
	m_lockedPositions = 0;
	m_positionStride  = 0;
	m_lockedColors    = 0;
	m_colorStride     = 0;
}

RendererDebugLine::~RendererDebugLine( void )
{
	checkUnlock();
	if(m_vertexbuffer) m_vertexbuffer->release();
	if(m_mesh)         m_mesh->release();
	if(m_material)
	{
		m_assetmanager.returnAsset(*m_material);
	}
}

void RendererDebugLine::addLine( const Vector3 &p0, const Vector3 &p1, const RendererColor &color )
{
	checkResizeLine(m_numVerts+2);
	addVert(p0, color);
	addVert(p1, color);
}

void RendererDebugLine::checkResizeLine( uint32 maxVerts )
{
	if(maxVerts > m_maxVerts)
	{
		m_maxVerts = maxVerts + (uint32)(maxVerts*0.2f);

		RendererVertexBufferDesc vbdesc;
		vbdesc.hint = RendererVertexBuffer::HINT_DYNAMIC;
		vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT3;
		vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_COLOR]    = RendererVertexBuffer::FORMAT_COLOR;
		vbdesc.maxVertices = m_maxVerts;
		RendererVertexBuffer *vertexbuffer = m_renderer.createVertexBuffer(vbdesc);
		ph_assert(vertexbuffer);

		if(vertexbuffer)
		{
			uint32 positionStride = 0;
			void *positions = vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride);

			uint32 colorStride = 0;
			void *colors = vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, colorStride);

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
							memcpy(((uint8*)colors)    + (colorStride*i),    ((uint8*)m_lockedColors)    + (m_colorStride*i),    sizeof(RendererColor));
						}
					}
					m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
					m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
				}
			}
			vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
			vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
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
			RendererMeshDesc meshdesc;
			meshdesc.primitives       = RendererMesh::PRIMITIVE_LINES;
			meshdesc.vertexBuffers    = &m_vertexbuffer;
			meshdesc.numVertexBuffers = 1;
			meshdesc.firstVertex      = 0;
			meshdesc.numVertices      = m_numVerts;
			m_mesh = m_renderer.createMesh(meshdesc);
			ph_assert(m_mesh);
		}
		m_meshContext.mesh = m_mesh;
	}
}

void RendererDebugLine::queueForRenderLine( void )
{
	if(m_meshContext.mesh)
	{
		checkUnlock();
		m_mesh->setVertexBufferRange(0, m_numVerts);
		m_renderer.queueMeshForRender(m_meshContext);
	}
}

void RendererDebugLine::clearLine( void )
{
	m_numVerts = 0;
}

void RendererDebugLine::checkLock( void )
{
	if(m_vertexbuffer)
	{
		if(!m_lockedPositions)
		{
			m_lockedPositions = m_vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, m_positionStride);
			ph_assert(m_lockedPositions);
		}
		if(!m_lockedColors)
		{
			m_lockedColors = m_vertexbuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_COLOR, m_colorStride);
			ph_assert(m_lockedColors);
		}
	}
}

void RendererDebugLine::checkUnlock( void )
{
	if(m_vertexbuffer)
	{
		if(m_lockedPositions)
		{
			m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
			m_lockedPositions = 0;
		}
		if(m_lockedColors)
		{
			m_vertexbuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_COLOR);
			m_lockedColors = 0;
		}
	}
}

void RendererDebugLine::addVert( const Vector3 &p, const RendererColor &color )
{
	ph_assert(m_maxVerts > m_numVerts);
	{
		checkLock();
		if(m_lockedPositions && m_lockedColors)
		{
			memcpy(((uint8*)m_lockedPositions) + (m_positionStride*m_numVerts), &p,     sizeof(Vector3));
			memcpy(((uint8*)m_lockedColors)    + (m_colorStride*m_numVerts),    &color, sizeof(RendererColor));
			m_numVerts++;
		}
	}
}


_NAMESPACE_END

