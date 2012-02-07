
#ifndef RENDERER_INDEXBUFFER_DESC_H
#define RENDERER_INDEXBUFFER_DESC_H

#include <RendererIndexBuffer.h>

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

#endif

