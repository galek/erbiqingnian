
#include <renderTargetDesc.h>
#include <renderTexture2D.h>

_NAMESPACE_BEGIN

RendererTargetDesc::RendererTargetDesc(void)
{
	textures            = 0;
	numTextures         = 0;
	depthStencilSurface = 0;
}

bool RendererTargetDesc::isValid(void) const
{
	bool ok = true;
	uint32 width  = 0;
	uint32 height = 0;
	if(numTextures != 1) ok = false; // for now we only support single render targets at the moment.
	if(textures)
	{
		if(textures[0])
		{
			width  = textures[0]->getWidth();
			height = textures[0]->getHeight();
		}
		for(uint32 i=0; i<numTextures; i++)
		{
			if(!textures[i]) ok = false;
			else
			{
				if(width  != textures[i]->getWidth())  ok = false;
				if(height != textures[i]->getHeight()) ok = false;
				const RendererTexture2D::Format format = textures[i]->getFormat();
				if(     format == RendererTexture2D::FORMAT_DXT1) ok = false;
				else if(format == RendererTexture2D::FORMAT_DXT3) ok = false;
				else if(format == RendererTexture2D::FORMAT_DXT5) ok = false;
			}
		}
	}
	else
	{
		ok = false;
	}
	if(depthStencilSurface)
	{
		if(width  != depthStencilSurface->getWidth())  ok = false;
		if(height != depthStencilSurface->getHeight()) ok = false;
		if(!RendererTexture2D::isDepthStencilFormat(depthStencilSurface->getFormat())) ok = false;
	}
	else
	{
		ok = false;
	}
	return ok;
}

_NAMESPACE_END