
#pragma once

#include "renderConfig.h"
#include "renderIndexBuffer.h"

_NAMESPACE_BEGIN

class RenderBase
{
	friend class Render;

public:

	typedef enum Primitive
	{
		PRIMITIVE_POINTS = 0,
		PRIMITIVE_LINES,
		PRIMITIVE_LINE_STRIP,
		PRIMITIVE_TRIANGLES,
		PRIMITIVE_TRIANGLE_STRIP,
		PRIMITIVE_POINT_SPRITES,

		NUM_PRIMITIVES
	};

	RenderBase();

	virtual								~RenderBase(void);

public:

	void								release(void) { delete this; }

	Primitive							getPrimitives(void) const;

	void								setPrimitives(Primitive pt);

	uint32								getNumVertices(void) const;

	uint32								getNumIndices(void) const;

	uint32								getNumInstances(void) const;

	void								setVertexBufferRange(uint32 firstVertex, uint32 numVertices);

	void								setIndexBufferRange(uint32 firstIndex, uint32 numIndices);

	void								setInstanceBufferRange(uint32 firstInstance, uint32 numInstances);

	SizeT								getNumVertexBuffers(void) const;

	virtual void						appendVertexBuffer(RenderVertexBuffer* vb);

	const RenderVertexBuffer*			getVertexBuffer(SizeT index) const;

	RenderVertexBuffer*					getVertexBuffer(SizeT index);

	const RenderIndexBuffer*			getIndexBuffer(void) const;

	RenderIndexBuffer*					getIndexBuffer(void);

	void								setIndexBuffer(RenderIndexBuffer* id);

	const RenderInstanceBuffer*			getInstanceBuffer(void) const;

	RenderInstanceBuffer*				getInstanceBuffer(void);

private:

	void         						bind(void) const;

	void         						render(RenderMaterial *material) const;

	void         						unbind(void) const;

	virtual void 						renderIndices(uint32 numVertices, uint32 firstIndex, uint32 numIndices,
											RenderIndexBuffer::Format indexFormat) const = 0;

	virtual void 						renderVertices(uint32 numVertices) const = 0;

	virtual void 						renderIndicesInstanced(uint32 numVertices, uint32 firstIndex, uint32 numIndices, 
											RenderIndexBuffer::Format indexFormat,RenderMaterial *material) const = 0;

	virtual void 						renderVerticesInstanced(uint32 numVertices,RenderMaterial *material) const = 0;

protected:

	Primitive               			m_primitiveType;

	Array<RenderVertexBuffer*>			m_vertexBuffers;

	uint32                  			m_firstVertex;

	uint32                  			m_numVertices;

	RenderIndexBuffer*					m_indexBuffer;

	uint32                  			m_firstIndex;

	uint32                  			m_numIndices;

	RenderInstanceBuffer*				m_instanceBuffer;

	uint32                  			m_firstInstance;

	uint32                  			m_numInstances;
};

_NAMESPACE_END
