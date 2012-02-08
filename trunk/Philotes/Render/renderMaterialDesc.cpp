
#include <renderMaterialDesc.h>

_NAMESPACE_BEGIN

RendererMaterialDesc::RendererMaterialDesc(void)
{
	type               = RendererMaterial::TYPE_UNLIT;
	alphaTestFunc      = RendererMaterial::ALPHA_TEST_ALWAYS;
	alphaTestRef       = 0;
	blending           = false;
	srcBlendFunc       = RendererMaterial::BLEND_ZERO;
	dstBlendFunc       = RendererMaterial::BLEND_ZERO;
	geometryShaderPath = 0;
	vertexShaderPath   = 0;
	fragmentShaderPath = 0;
}
		
bool RendererMaterialDesc::isValid(void) const
{
	bool ok = true;
	if(type >= RendererMaterial::NUM_TYPES) ok = false;
	if(alphaTestRef < 0 || alphaTestRef > 1) ok = false;
	if(blending && type != RendererMaterial::TYPE_UNLIT) ok = false;
	if(geometryShaderPath)
	{
		ph_assert2(0, "Not implemented!");
		ok = false;
	}
	if(!vertexShaderPath)   ok = false;
	if(!fragmentShaderPath) ok = false;
	return ok;
}

_NAMESPACE_END