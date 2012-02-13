
#include <renderMeshShape.h>

#include <render.h>

#include <renderVertexBuffer.h>
#include <renderVertexBufferDesc.h>

#include <renderIndexBuffer.h>
#include <renderIndexBufferDesc.h>

#include <renderMesh.h>
#include <renderMeshDesc.h>

#include "gearsApplication.h"

_NAMESPACE_BEGIN

RenderMeshShape::RenderMeshShape(	const String& name,
									const Vector3* verts, uint32 numVerts, 
									const Vector3* normals,
									const scalar* uvs,
									const uint16* faces, uint32 numFaces, bool flipWinding) :
	RenderElement(name)
{
	Philo::RenderVertexBufferDesc vbdesc;
	vbdesc.hint = RenderVertexBuffer::HINT_STATIC;
	vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_POSITION]  = RenderVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_NORMAL]    = RenderVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RenderVertexBuffer::SEMANTIC_TEXCOORD0] = RenderVertexBuffer::FORMAT_FLOAT2;
	vbdesc.maxVertices = numVerts;
	m_vertexBuffer = GearApplication::getApp()->getRender()->createVertexBuffer(vbdesc);
	ph_assert2(m_vertexBuffer, "Failed to create Vertex Buffer.");
	if(m_vertexBuffer)
	{
		uint32 positionStride = 0;
		void* vertPositions = m_vertexBuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_POSITION, positionStride);

		uint32 normalStride = 0;
		void* vertNormals = m_vertexBuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_NORMAL, normalStride);

		uint32 uvStride = 0;
		void* vertUVs = m_vertexBuffer->lockSemantic(RenderVertexBuffer::SEMANTIC_TEXCOORD0, uvStride);

		if(vertPositions && vertNormals && vertUVs)
		{
			for(uint32 i=0; i<numVerts; i++)
			{
				memcpy(vertPositions, verts+i, sizeof(Vector3));
				if(normals)
					memcpy(vertNormals, normals+i, sizeof(Vector3));
				else
					memset(vertNormals, 0, sizeof(Vector3));

				if(uvs)
					memcpy(vertUVs, uvs+i*2, sizeof(scalar)*2);
				else
					memset(vertUVs, 0, sizeof(scalar)*2);

				vertPositions = (void*)(((uint8*)vertPositions) + positionStride);
				vertNormals   = (void*)(((uint8*)vertNormals)   + normalStride);
				vertUVs       = (void*)(((uint8*)vertUVs)       + uvStride);
			}
		}
		m_vertexBuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_NORMAL);
		m_vertexBuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_POSITION);
		m_vertexBuffer->unlockSemantic(RenderVertexBuffer::SEMANTIC_TEXCOORD0);
	}

	const uint32 numIndices = numFaces*3;
	
	RenderIndexBufferDesc ibdesc;
	ibdesc.hint       = RenderIndexBuffer::HINT_STATIC;
	ibdesc.format     = RenderIndexBuffer::FORMAT_UINT16;
	ibdesc.maxIndices = numIndices;
	m_indexBuffer = GearApplication::getApp()->getRender()->createIndexBuffer(ibdesc);
	ph_assert2(m_indexBuffer, "Failed to create Index Buffer.");
	if(m_indexBuffer)
	{
		uint16* indices = (uint16*)m_indexBuffer->lock();
		if(indices)
		{
			if(flipWinding)
			{
				for(uint32 i=0;i<numFaces;i++)
				{
					indices[i*3+0] = faces[i*3+0];
					indices[i*3+1] = faces[i*3+2];
					indices[i*3+2] = faces[i*3+1];
				}
			}
			else memcpy(indices, faces, sizeof(*faces)*numFaces*3);
		}
		m_indexBuffer->unlock();
	}
	
	if(m_vertexBuffer && m_indexBuffer)
	{
		RenderMeshDesc meshdesc;
		meshdesc.primitives       = RenderMesh::PRIMITIVE_TRIANGLES;
		meshdesc.vertexBuffers    = &m_vertexBuffer;
		meshdesc.numVertexBuffers = 1;
		meshdesc.firstVertex      = 0;
		meshdesc.numVertices      = numVerts;
		meshdesc.indexBuffer      = m_indexBuffer;
		meshdesc.firstIndex       = 0;
		meshdesc.numIndices       = numIndices;
		m_mesh = GearApplication::getApp()->getRender()->createMesh(meshdesc);
		ph_assert2(m_mesh, "Failed to create Mesh.");
	}
}

RenderMeshShape::~RenderMeshShape(void)
{
	if(m_vertexBuffer) m_vertexBuffer->release();
	if(m_indexBuffer)  m_indexBuffer->release();
	if(m_mesh)
	{
		m_mesh->release();
		m_mesh = 0;
	}
}

_NAMESPACE_END