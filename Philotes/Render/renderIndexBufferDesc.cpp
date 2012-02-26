
#include <renderIndexBufferDesc.h>

_NAMESPACE_BEGIN

RenderIndexBufferDesc::RenderIndexBufferDesc(void)
{
	hint       = RenderIndexBuffer::HINT_STATIC;
	format     = RenderIndexBuffer::FORMAT_UINT16;
	maxIndices = 0;
}
		
bool RenderIndexBufferDesc::isValid(void) const
{
	bool ok = true;
	if(format >= RenderIndexBuffer::NUM_FORMATS) 
	{
		ok = false;
	}
	if(maxIndices == 0)
	{
		ok = false;
	}
	return ok;
}

_NAMESPACE_END