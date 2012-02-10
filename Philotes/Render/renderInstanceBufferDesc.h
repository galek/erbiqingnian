
#ifndef RENDERER_INSTANCEBUFFER_DESC_H
#define RENDERER_INSTANCEBUFFER_DESC_H

#include <renderInstanceBuffer.h>

_NAMESPACE_BEGIN

class RenderInstanceBufferDesc
{
	public:
		RenderInstanceBuffer::Hint   hint;
		RenderInstanceBuffer::Format semanticFormats[RenderInstanceBuffer::NUM_SEMANTICS];
		
		uint32                          maxInstances;
		
	public:
		RenderInstanceBufferDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
