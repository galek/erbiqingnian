
#include <renderVertexBufferDesc.h>

RenderVertexBufferDesc::RenderVertexBufferDesc(void)
{
	hint = RenderVertexBuffer::HINT_STATIC;
	for(uint32 i=0; i<RenderVertexBuffer::NUM_SEMANTICS; i++)
	{
		semanticFormats[i] = RenderVertexBuffer::NUM_FORMATS;
	}
	maxVertices = 0;
}

bool RenderVertexBufferDesc::isValid(void) const
{
	bool ok = true;
	// TODO: make sure at least "some" of the semanticFormats are set.
	if(!maxVertices) ok = false;
	return ok;
}
