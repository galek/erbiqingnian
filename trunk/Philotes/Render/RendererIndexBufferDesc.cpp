
#include <RendererIndexBufferDesc.h>

RendererIndexBufferDesc::RendererIndexBufferDesc(void)
{
	hint       = RendererIndexBuffer::HINT_STATIC;
	format     = RendererIndexBuffer::NUM_FORMATS;
	maxIndices = 0;

}
		
bool RendererIndexBufferDesc::isValid(void) const
{
	bool ok = true;
	if(format >= RendererIndexBuffer::NUM_FORMATS) ok = false;
	if(maxIndices == 0) ok = false;
	return ok;
}
