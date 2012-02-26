
#include "renderBase.h"
#include "renderVertexBuffer.h"
#include "renderIndexBuffer.h"
#include "renderInstanceBuffer.h"

_NAMESPACE_BEGIN

RenderBase::RenderBase()
{
	m_firstVertex    = 0;
	m_numVertices    = 0;

	m_indexBuffer    = NULL;
	m_firstIndex     = 0;
	m_numIndices     = 0;

	m_instanceBuffer = NULL;
	m_firstInstance  = 0;
	m_numInstances   = 0;

	m_primitiveType  = PRIMITIVE_TRIANGLES;
}

RenderBase::~RenderBase(void)
{
	m_vertexBuffers.Reset();
}

RenderBase::Primitive RenderBase::getPrimitives(void) const
{
	return m_primitiveType;
}

uint32 RenderBase::getNumVertices(void) const
{
	return m_numVertices;
}

uint32 RenderBase::getNumIndices(void) const
{
	return m_numIndices;
}

uint32 RenderBase::getNumInstances(void) const
{
	return m_numInstances;
}

void RenderBase::setVertexBufferRange(uint32 firstVertex, uint32 numVertices)
{
	// TODO: Check for valid range...
	m_firstVertex = firstVertex;
	m_numVertices = numVertices;
}

void RenderBase::setIndexBufferRange(uint32 firstIndex, uint32 numIndices)
{
	// TODO: Check for valid range...
	m_firstIndex = firstIndex;
	m_numIndices = numIndices;
}

void RenderBase::setInstanceBufferRange(uint32 firstInstance, uint32 numInstances)
{
	// TODO: Check for valid range...
	m_firstInstance = firstInstance;
	m_numInstances  = numInstances;
}

SizeT RenderBase::getNumVertexBuffers(void) const
{
	return m_vertexBuffers.Size();
}

const RenderIndexBuffer *RenderBase::getIndexBuffer(void) const
{
	return m_indexBuffer;
}

RenderIndexBuffer* RenderBase::getIndexBuffer( void )
{
	return m_indexBuffer;
}

const RenderInstanceBuffer *RenderBase::getInstanceBuffer(void) const
{
	return m_instanceBuffer;
}

RenderInstanceBuffer* RenderBase::getInstanceBuffer( void )
{
	return m_instanceBuffer;
}

void RenderBase::bind(void) const
{
	SizeT vbNum = m_vertexBuffers.Size();
	for(SizeT i=0; i<vbNum; i++)
	{
		ph_assert2(m_vertexBuffers[i]->checkBufferWritten(), "Vertex buffer is empty!");
		m_vertexBuffers[i]->bind(i, m_firstVertex);
	}

	if(m_instanceBuffer)
	{
		m_instanceBuffer->bind(vbNum, m_firstInstance);
	}
	
	if(m_indexBuffer)
	{
		m_indexBuffer->bind();
	}
}

void RenderBase::render(RenderMaterial *material) const
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

void RenderBase::unbind(void) const
{
	SizeT vbNum = m_vertexBuffers.Size();

	if(m_indexBuffer)
	{
		m_indexBuffer->unbind();
	}
	if(m_instanceBuffer)
	{
		m_instanceBuffer->unbind(vbNum);
	}
	for(SizeT i=0; i<vbNum; i++)
	{
		m_vertexBuffers[i]->unbind(i);
	}
}

const RenderVertexBuffer* RenderBase::getVertexBuffer( SizeT index ) const
{
	if (index >= m_vertexBuffers.Size())
	{
		PH_EXCEPT(ERR_RENDER,"invalid vertex buffer index : RenderBase::getVertexBuffer");
	}

	return m_vertexBuffers[index];
}

RenderVertexBuffer* RenderBase::getVertexBuffer( SizeT index )
{
	if (index >= m_vertexBuffers.Size())
	{
		PH_EXCEPT(ERR_RENDER,"invalid vertex buffer index : RenderBase::getVertexBuffer");
	}

	return m_vertexBuffers[index];
}

void RenderBase::setPrimitives( Primitive pt )
{
	m_primitiveType = pt;
}

void RenderBase::appendVertexBuffer( RenderVertexBuffer* vb )
{
	m_vertexBuffers.Append(vb);
}

void RenderBase::setIndexBuffer( RenderIndexBuffer* id )
{
	m_indexBuffer = id;
}

_NAMESPACE_END