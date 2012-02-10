
#include <renderTexture2DDesc.h>

_NAMESPACE_BEGIN

RenderTexture2DDesc::RenderTexture2DDesc(void)
{
	format       = RenderTexture2D::NUM_FORMATS;
	filter       = RenderTexture2D::FILTER_LINEAR;
	addressingU  = RenderTexture2D::ADDRESSING_WRAP;
	addressingV  = RenderTexture2D::ADDRESSING_WRAP;
	width        = 0;
	height       = 0;
	numLevels    = 0;
	renderTarget = false;
}


bool RenderTexture2DDesc::isValid(void) const
{
	bool ok = true;
	if(format      >= RenderTexture2D::NUM_FORMATS)    ok = false;
	if(filter      >= RenderTexture2D::NUM_FILTERS)    ok = false;
	if(addressingU >= RenderTexture2D::NUM_ADDRESSING) ok = false;
	if(addressingV >= RenderTexture2D::NUM_ADDRESSING) ok = false;
	if(width <= 0 || height <= 0) ok = false; // TODO: check for power of two.
	if(numLevels <= 0) ok = false;
	if(renderTarget)
	{
		if(format == RenderTexture2D::FORMAT_DXT1) ok = false;
		if(format == RenderTexture2D::FORMAT_DXT3) ok = false;
		if(format == RenderTexture2D::FORMAT_DXT5) ok = false;
	}
	return ok;
}

_NAMESPACE_END