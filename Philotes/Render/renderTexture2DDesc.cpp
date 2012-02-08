
#include <renderTexture2DDesc.h>

_NAMESPACE_BEGIN

RendererTexture2DDesc::RendererTexture2DDesc(void)
{
	format       = RendererTexture2D::NUM_FORMATS;
	filter       = RendererTexture2D::FILTER_LINEAR;
	addressingU  = RendererTexture2D::ADDRESSING_WRAP;
	addressingV  = RendererTexture2D::ADDRESSING_WRAP;
	width        = 0;
	height       = 0;
	numLevels    = 0;
	renderTarget = false;
}


bool RendererTexture2DDesc::isValid(void) const
{
	bool ok = true;
	if(format      >= RendererTexture2D::NUM_FORMATS)    ok = false;
	if(filter      >= RendererTexture2D::NUM_FILTERS)    ok = false;
	if(addressingU >= RendererTexture2D::NUM_ADDRESSING) ok = false;
	if(addressingV >= RendererTexture2D::NUM_ADDRESSING) ok = false;
	if(width <= 0 || height <= 0) ok = false; // TODO: check for power of two.
	if(numLevels <= 0) ok = false;
	if(renderTarget)
	{
		if(format == RendererTexture2D::FORMAT_DXT1) ok = false;
		if(format == RendererTexture2D::FORMAT_DXT3) ok = false;
		if(format == RendererTexture2D::FORMAT_DXT5) ok = false;
	}
	return ok;
}

_NAMESPACE_END