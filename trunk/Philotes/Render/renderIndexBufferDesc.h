
#ifndef RENDERER_INDEXBUFFER_DESC_H
#define RENDERER_INDEXBUFFER_DESC_H

#include <renderIndexBuffer.h>

_NAMESPACE_BEGIN

class RendererIndexBufferDesc
{
	public:
		RendererIndexBuffer::Hint   hint;
		RendererIndexBuffer::Format format;
		
		uint32                       maxIndices;

	public:
		RendererIndexBufferDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif

