
#ifndef RENDERER_TARGET_DESC_H
#define RENDERER_TARGET_DESC_H

#include <renderConfig.h>

_NAMESPACE_BEGIN

class RenderTexture2D;

class RenderTargetDesc
{
	public:
		RenderTexture2D **textures;
		uint32               numTextures;
		
		RenderTexture2D  *depthStencilSurface;
		
	public:
		RenderTargetDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
