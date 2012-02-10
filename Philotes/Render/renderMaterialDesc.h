
#ifndef RENDERER_MATERIAL_DESC_H
#define RENDERER_MATERIAL_DESC_H

#include <renderMaterial.h>

_NAMESPACE_BEGIN

class RenderMaterialDesc
{
	public:
		RenderMaterial::Type          type;
		
		RenderMaterial::AlphaTestFunc alphaTestFunc;
		float                           alphaTestRef;
		
		bool                            blending;
		RenderMaterial::BlendFunc     srcBlendFunc;
		RenderMaterial::BlendFunc     dstBlendFunc;
		
		const char                     *geometryShaderPath;
		const char                     *vertexShaderPath;
		const char                     *fragmentShaderPath;
		
	public:
		RenderMaterialDesc(void);
		
		bool isValid(void) const;
};

_NAMESPACE_END

#endif
