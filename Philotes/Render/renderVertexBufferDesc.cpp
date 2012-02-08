
#include <renderVertexBufferDesc.h>

RendererVertexBufferDesc::RendererVertexBufferDesc(void)
{
	hint = RendererVertexBuffer::HINT_STATIC;
	for(uint32 i=0; i<RendererVertexBuffer::NUM_SEMANTICS; i++)
	{
		semanticFormats[i] = RendererVertexBuffer::NUM_FORMATS;
	}
	maxVertices = 0;
}

bool RendererVertexBufferDesc::isValid(void) const
{
	bool ok = true;
	// TODO: make sure at least "some" of the semanticFormats are set.
	if(!maxVertices) ok = false;
	return ok;
}
