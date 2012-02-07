
#ifndef RENDERER_VERTEXBUFFER_DESC_H
#define RENDERER_VERTEXBUFFER_DESC_H

#include <RendererVertexBuffer.h>

namespace physx
{
namespace pxtask
{
	class CudaContextManager;
}
};

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

#endif
