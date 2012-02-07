
#ifndef RENDERER_TEXTURE_2D_DESC_H
#define RENDERER_TEXTURE_2D_DESC_H

#include <RendererTexture2D.h>

class RendererTexture2DDesc
{
	public:
		RendererTexture2D::Format     format;
		RendererTexture2D::Filter     filter;
		RendererTexture2D::Addressing addressingU;
		RendererTexture2D::Addressing addressingV;
		
		uint32                     width;
		uint32                     height;
		uint32                     numLevels;
		
		bool                      renderTarget;
		
	public:
		RendererTexture2DDesc(void);
		
		bool isValid(void) const;
};

#endif
