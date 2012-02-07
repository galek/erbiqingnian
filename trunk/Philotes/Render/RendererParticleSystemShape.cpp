#include <RendererParticleSystemShape.h>

#include <Renderer.h>
#include <RendererVertexBuffer.h>
#include <RendererVertexBufferDesc.h>
#include <RendererMesh.h>
#include <RendererMeshDesc.h>

RendererParticleSystemShape::RendererParticleSystemShape(Renderer &renderer, 
														uint32 num_vertices, 
														scalar* vertices) : RendererShape(renderer) {
	RendererVertexBufferDesc vbdesc;
	vbdesc.hint = RendererVertexBuffer::HINT_DYNAMIC;
	for(int i = 0; i < RendererVertexBuffer::NUM_SEMANTICS; ++i)
		vbdesc.semanticFormats[i] = RendererVertexBuffer::NUM_FORMATS;
	vbdesc.semanticFormats[RendererVertexBuffer::SEMANTIC_POSITION] = RendererVertexBuffer::FORMAT_FLOAT3;

	vbdesc.maxVertices = num_vertices;
	m_vertexBuffer = m_renderer.createVertexBuffer(vbdesc);
	ph_assert2(m_vertexBuffer, "Failed to create Vertex Buffer.");
	if(m_vertexBuffer)
	{
		uint32 positionStride = 0;
		void* positions = m_vertexBuffer->lockSemantic(RendererVertexBuffer::SEMANTIC_POSITION, positionStride);
		memcpy(positions, vertices, num_vertices * sizeof(scalar) * 3);
		m_vertexBuffer->unlockSemantic(RendererVertexBuffer::SEMANTIC_POSITION);
	}
	
	if(m_vertexBuffer)
	{
		RendererMeshDesc meshdesc;
		meshdesc.primitives       = RendererMesh::PRIMITIVE_POINT_SPRITES;
		meshdesc.vertexBuffers    = &m_vertexBuffer;
		meshdesc.numVertexBuffers = 1;
		meshdesc.firstVertex      = 0;
		meshdesc.numVertices      = num_vertices;
		meshdesc.indexBuffer      = NULL;
		meshdesc.firstIndex       = 0;
		meshdesc.numIndices       = 0;
		m_mesh = m_renderer.createMesh(meshdesc);
		ph_assert2(m_mesh, "Failed to create Mesh.");
	}
}

RendererParticleSystemShape::~RendererParticleSystemShape(void)
{
	if(m_vertexBuffer) m_vertexBuffer->release();
	if(m_mesh)
	{
		m_mesh->release();
		m_mesh = 0;
	}
}