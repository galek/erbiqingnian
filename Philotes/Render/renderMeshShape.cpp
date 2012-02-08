
#include <renderMeshShape.h>

#include <render.h>

#include <renderVertexBuffer.h>
#include <renderVertexBufferDesc.h>

#include <renderIndexBuffer.h>
#include <renderIndexBufferDesc.h>

#include <renderMesh.h>
#include <renderMeshDesc.h>

_NAMESPACE_BEGIN

RendererMeshShape::RendererMeshShape(	Renderer& renderer, 
										const Vector3* verts, uint32 numVerts, 
										const Vector3* normals,
										const scalar* uvs,
										const uint16* faces, uint32 numFaces, bool flipWinding) :
	RendererShape(renderer)
{
	Philo::RendererVertexBufferDesc vbdesc;
	vbdesc.hint = RendererVertexBuffer::HINT_STATIC;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION]  = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_NORMAL]    = RendererVertexBuffer::FORMAT_FLOAT3;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_TEXCOORD0] = RendererVertexBuffer::FORMAT_FLOAT2;
	vbdesc.maxVertices = numVerts;
	m_vertexBuffer = m_renderer.createVertexBuffer(vbdesc);
	ph_assert2(m_vertexBuffer, "Failed to create Vertex Buffer.");
	if(m_vertexBuffer)
	{
		uint32 positionStride = 0;
		void* vertPositions = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride);

		uint32 normalStride = 0;
		void* vertNormals = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL, normalStride);

		uint32 uvStride = 0;
		void* vertUVs = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0, uvStride);

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
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_NORMAL);
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_TEXCOORD0);
	}

	const uint32 numIndices = numFaces*3;
	
	RendererIndexBufferDesc ibdesc;
	ibdesc.hint       = RendererIndexBuffer::HINT_STATIC;
	ibdesc.format     = RendererIndexBuffer::FORMAT_UINT16;
	ibdesc.maxIndices = numIndices;
	m_indexBuffer = m_renderer.createIndexBuffer(ibdesc);
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
		RendererMeshDesc meshdesc;
		meshdesc.primitives       = RendererMesh::PRIMITIVE_TRIANGLES;
		meshdesc.vertexBuffers    = &m_vertexBuffer;
		meshdesc.numVertexBuffers = 1;
		meshdesc.firstVertex      = 0;
		meshdesc.numVertices      = numVerts;
		meshdesc.indexBuffer      = m_indexBuffer;
		meshdesc.firstIndex       = 0;
		meshdesc.numIndices       = numIndices;
		m_mesh = m_renderer.createMesh(meshdesc);
		ph_assert2(m_mesh, "Failed to create Mesh.");
	}
}

RendererMeshShape::~RendererMeshShape(void)
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