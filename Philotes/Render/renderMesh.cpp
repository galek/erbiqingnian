
#include <renderMesh.h>
#include <renderMeshDesc.h>

#include <renderVertexBuffer.h>
#include <renderIndexBuffer.h>
#include <renderInstanceBuffer.h>

_NAMESPACE_BEGIN

RendererMesh::RendererMesh(const RendererMeshDesc &desc)
{
	m_primitives = desc.primitives;
	
	m_numVertexBuffers = desc.numVertexBuffers;
	m_vertexBuffers = new RendererVertexBuffer*[m_numVertexBuffers];
	for(uint32 i=0; i<m_numVertexBuffers; i++)
	{
		m_vertexBuffers[i] = desc.vertexBuffers[i];
	}
	m_firstVertex    = desc.firstVertex;
	m_numVertices    = desc.numVertices;
	
	m_indexBuffer    = desc.indexBuffer;
	m_firstIndex     = desc.firstIndex;
	m_numIndices     = desc.numIndices;
	
	m_instanceBuffer = desc.instanceBuffer;
	m_firstInstance  = desc.firstInstance;
	m_numInstances   = desc.numInstances;
}

RendererMesh::~RendererMesh(void)
{
	delete [] m_vertexBuffers;
}

RendererMesh::Primitive RendererMesh::getPrimitives(void) const
{
	return m_primitives;
}

uint32 RendererMesh::getNumVertices(void) const
{
	return m_numVertices;
}

uint32 RendererMesh::getNumIndices(void) const
{
	return m_numIndices;
}

uint32 RendererMesh::getNumInstances(void) const
{
	return m_numInstances;
}

void RendererMesh::setVertexBufferRange(uint32 firstVertex, uint32 numVertices)
{
	// TODO: Check for valid range...
	m_firstVertex = firstVertex;
	m_numVertices = numVertices;
}

void RendererMesh::setIndexBufferRange(uint32 firstIndex, uint32 numIndices)
{
	// TODO: Check for valid range...
	m_firstIndex = firstIndex;
	m_numIndices = numIndices;
}

void RendererMesh::setInstanceBufferRange(uint32 firstInstance, uint32 numInstances)
{
	// TODO: Check for valid range...
	m_firstInstance = firstInstance;
	m_numInstances  = numInstances;
}

uint32 RendererMesh::getNumVertexBuffers(void) const
{
	return m_numVertexBuffers;
}

const RendererVertexBuffer *const*RendererMesh::getVertexBuffers(void) const
{
	return m_vertexBuffers;
}

const RendererIndexBuffer *RendererMesh::getIndexBuffer(void) const
{
	return m_indexBuffer;
}

const RendererInstanceBuffer *RendererMesh::getInstanceBuffer(void) const
{
	return m_instanceBuffer;
}

void RendererMesh::bind(void) const
{
	for(uint32 i=0; i<m_numVertexBuffers; i++)
	{
		ph_assert2(m_vertexBuffers[i]->checkBufferWritten(), "Vertex buffer is empty!");
		m_vertexBuffers[i]->bind(i, m_firstVertex);
	}
	if(m_instanceBuffer)
	{
		m_instanceBuffer->bind(m_numVertexBuffers, m_firstInstance);
	}
	if(m_indexBuffer)
	{
		m_indexBuffer->bind();
	}
}

void RendererMesh::render(RendererMaterial *material) const
{
	if(m_instanceBuffer)
	{
		if(m_indexBuffer)
		{
			renderIndicesInstanced(m_numVertices, m_firstIndex, m_numIndices, m_indexBuffer->getFormat(),material);
		}
		else if(m_numVertices)
		{
			renderVerticesInstanced(m_numVertices,material);
		}
	}
	else
	{
		if(m_indexBuffer)
		{
			renderIndices(m_numVertices, m_firstIndex, m_numIndices, m_indexBuffer->getFormat());
		}
		else if(m_numVertices)
		{
			renderVertices(m_numVertices);
		}
	}
}

void RendererMesh::unbind(void) const
{
	if(m_indexBuffer)
	{
		m_indexBuffer->unbind();
	}
	if(m_instanceBuffer)
	{
		m_instanceBuffer->unbind(m_numVertexBuffers);
	}
	for(uint32 i=0; i<m_numVertexBuffers; i++)
	{
		m_vertexBuffers[i]->unbind(i);
	}
}

_NAMESPACE_END