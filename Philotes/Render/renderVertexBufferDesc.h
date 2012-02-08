
#ifndef RENDERER_VERTEXBUFFER_DESC_H
#define RENDERER_VERTEXBUFFER_DESC_H

#include <renderVertexBuffer.h>

_NAMESPACE_BEGIN

class RendererVertexBufferDesc
{
	public:
		RendererVertexBuffer::Hint   hint;
		RendererVertexBuffer::Format semanticFormats[RendererVertexBuffer::NUM_SEMANTICS];
		
		uint32                 maxVertices;
		
	public:
		RendererVertexBufferDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
