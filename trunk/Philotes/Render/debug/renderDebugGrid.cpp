
#include "renderDebugGrid.h"
#include "gearsApplication.h"
#include "gearsAsset.h"
#include "gearsMaterialAsset.h"
#include "renderMaterialInstance.h"
#include "gearsAssetManager.h"
#include "render.h"
#include "renderMesh.h"
#include "renderMeshDesc.h"
#include "renderVertexBuffer.h"
#include "renderVertexBufferDesc.h"
#include "renderIndexBuffer.h"

_NAMESPACE_BEGIN

RenderGridElement::RenderGridElement( uint32 size, float cellSize ):
	RenderElement()
{
	m_vertexBuffer = 0;

	const uint32 numColLines = size*2 + 1;
	const uint32 numLines    = numColLines*2;
	const uint32 numVerts    = numLines*2;

	RenderVertexBufferDesc vbdesc;
	vbdesc.hint                                                     = RenderVertexBuffer::HINT_STATIC;
	vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_POSITION]	= RenderVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_COLOR]		= RenderVertexBuffer::FORMAT_COLOR;
	vbdesc.maxVertices                                              = numVerts;
	m_vertexBuffer = GearApplication::getApp()->getRender()->createVertexBuffer(vbdesc);
	if(m_vertexBuffer)
	{
		RenderMeshDesc meshdesc;
		meshdesc.primitives			= RenderMesh::PRIMITIVE_LINES;
		meshdesc.vertexBuffers		= &m_vertexBuffer;
		meshdesc.numVertexBuffers	= 1;
		meshdesc.firstVertex		= 0;
		meshdesc.numVertices		= numVerts;
		m_mesh						= GearApplication::getApp()->getRender()->createMesh(meshdesc);
	}
	if(m_vertexBuffer && m_mesh)
	{
		uint32 color2 = 0xFF788AA3;
		uint32 color1 = 0xFF5E6C7F;

		uint32 positionStride	= 0;
		void *positions			= m_vertexBuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_POSITION, positionStride);
		uint32 colorStride		= 0;
		void *colors			= m_vertexBuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_COLOR, colorStride);

		if(positions && colors)
		{
			const float radius   = size*cellSize;
			const float diameter = radius*2;
			for(uint32 c = 0; c<numColLines; c++)
			{
				float *p0	= (float*)positions;
				uint8  *c0	= (uint8*)colors;
				positions	= ((uint8*)positions)+positionStride;
				colors		= ((uint8*)colors)+colorStride;
				float *p1	= (float*)positions;
				uint8  *c1	= (uint8*)colors;
				positions	= ((uint8*)positions)+positionStride;
				colors		= ((uint8*)colors)+colorStride;

				const float t = c / (float)(numColLines-1);
				const float d = diameter*t - radius;

				uint32 *col1 = (uint32 *)c0;
				uint32 *col2 = (uint32 *)c1;
				if(d==0.0f) 
				{
					*col1 = color1;
					*col2 = color1;
				}
				else        
				{
					*col1 = color2;
					*col2 = color2;
				}
				p0[0] = -radius; p0[1] = 0; p0[2] = d;
				p1[0] =  radius; p1[1] = 0; p1[2] = d;
			}
			for(uint32 r=0; r<numColLines; r++)
			{
				float *p0 = (float*)positions;
				uint8  *c0 = (uint8*)colors;
				positions = ((uint8*)positions)+positionStride;
				colors    = ((uint8*)colors)+colorStride;
				float *p1 = (float*)positions;
				uint8  *c1 = (uint8*)colors;
				positions = ((uint8*)positions)+positionStride;
				colors    = ((uint8*)colors)+colorStride;

				const float t = r / (float)(numColLines-1);
				const float d = diameter*t - radius;
				uint32 *col1 = (uint32 *)c0;
				uint32 *col2 = (uint32 *)c1;
				if(d==0.0f) 
				{
					*col1 = color1;
					*col2 = color1;
				}
				else        
				{
					*col1 = color2;
					*col2 = color2;
				}

				p0[0] = d; p0[1] = 0; p0[2] = -radius;
				p1[0] = d; p1[1] = 0; p1[2] =  radius;
			}
		}
		m_vertexBuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_COLOR);
		m_vertexBuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_POSITION);
	}

	m_materialAsset = GearMaterialAsset::getPrefabAsset(PM_UNLIGHT);

	setMaterialInstance(m_materialAsset ? new RenderMaterialInstance(*m_materialAsset->getMaterial(0)) : 0);
}

RenderGridElement::~RenderGridElement( void )
{
	if(m_vertexBuffer) 
	{
		m_vertexBuffer->release();
	}

	if(m_mesh)
	{
		m_mesh->release();
		m_mesh = 0;
	}

	if(m_materialAsset)
	{
		GearAssetManager::getSingleton()->returnAsset(*m_materialAsset);
	}
}

void RenderGridElement::getWorldTransforms( Matrix4* xform ) const
{
	xform[0] = Matrix4::IDENTITY;
}


_NAMESPACE_END

