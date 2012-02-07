
#ifndef RENDERER_MATERIAL_DESC_H
#define RENDERER_MATERIAL_DESC_H

#include <RendererMaterial.h>

class RendererMaterialDesc
{
	public:
		RendererMaterial::Type          type;
		
		RendererMaterial::AlphaTestFunc alphaTestFunc;
		float                           alphaTestRef;
		
		bool                            blending;
		RendererMaterial::BlendFunc     srcBlendFunc;
		RendererMaterial::BlendFunc     dstBlendFunc;
		
		const char                     *geometryShaderPath;
		const char                     *vertexShaderPath;
		const char                     *fragmentShaderPath;
		
	public:
		RendererMaterialDesc(void);
		
		bool isValid(void) const;
};

#endif
