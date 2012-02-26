
#ifndef RENDERER_INDEXBUFFER_DESC_H
#define RENDERER_INDEXBUFFER_DESC_H

#include <renderIndexBuffer.h>

_NAMESPACE_BEGIN

class RenderIndexBufferDesc
{
	public:
		RenderIndexBuffer::Hint		hint;
		RenderIndexBuffer::Format	format;
		
		uint32						maxIndices;

	public:
		RenderIndexBufferDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif

