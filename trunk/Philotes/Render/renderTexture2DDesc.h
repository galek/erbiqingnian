
#ifndef RENDERER_TEXTURE_2D_DESC_H
#define RENDERER_TEXTURE_2D_DESC_H

#include <renderTexture2D.h>

_NAMESPACE_BEGIN

class RenderTexture2DDesc
{
	public:
		RenderTexture2D::Format     format;
		RenderTexture2D::Filter     filter;
		RenderTexture2D::Addressing addressingU;
		RenderTexture2D::Addressing addressingV;
		
		uint32                     width;
		uint32                     height;
		uint32                     numLevels;
		
		bool                      renderTarget;
		
	public:
		RenderTexture2DDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
