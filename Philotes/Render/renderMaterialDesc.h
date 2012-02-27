
#ifndef RENDERER_MATERIAL_DESC_H
#define RENDERER_MATERIAL_DESC_H

#include <renderMaterial.h>

_NAMESPACE_BEGIN

struct ShaderDefines
{
	ShaderDefines()
	{
	}
	ShaderDefines(const char* d, const char* v)
	{
		define = d;
		value = v;
	}
	String define;
	String value;
};

class RenderMaterialDesc
{
public:
	RenderMaterial::Type			type;

	RenderMaterial::AlphaTestFunc	alphaTestFunc;
	float                           alphaTestRef;

	bool                            blending;
	RenderMaterial::BlendFunc		srcBlendFunc;
	RenderMaterial::BlendFunc		dstBlendFunc;

	const char*						geometryShaderPath;
	const char*						vertexShaderPath;
	const char*						fragmentShaderPath;

	Array<ShaderDefines>			extraDefines;

public:
	RenderMaterialDesc(void);
	bool isValid(void) const;
};

_NAMESPACE_END

#endif
