
#ifndef RENDERER_TARGET_DESC_H
#define RENDERER_TARGET_DESC_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class RendererTexture2D;

class RendererTargetDesc
{
	public:
		RendererTexture2D **textures;
		uint32               numTextures;
		
		RendererTexture2D  *depthStencilSurface;
		
	public:
		RendererTargetDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
