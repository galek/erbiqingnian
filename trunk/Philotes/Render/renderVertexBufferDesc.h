
#ifndef RENDERER_VERTEXBUFFER_DESC_H
#define RENDERER_VERTEXBUFFER_DESC_H

#include <renderVertexBuffer.h>

_NAMESPACE_BEGIN

class RenderVertexBufferDesc
{
	public:
		RenderVertexBuffer::Hint	hint;

		RenderVertexBuffer::Format	semanticFormats[RenderVertexBuffer::NUM_SEMANTICS];
		
		uint32						maxVertices;
		
	public:
		RenderVertexBufferDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
