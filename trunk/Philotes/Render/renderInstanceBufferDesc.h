
#ifndef RENDERER_INSTANCEBUFFER_DESC_H
#define RENDERER_INSTANCEBUFFER_DESC_H

#include <renderInstanceBuffer.h>

_NAMESPACE_BEGIN

class RendererInstanceBufferDesc
{
	public:
		RendererInstanceBuffer::Hint   hint;
		RendererInstanceBuffer::Format semanticFormats[RendererInstanceBuffer::NUM_SEMANTICS];
		
		uint32                          maxInstances;
		
	public:
		RendererInstanceBufferDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
