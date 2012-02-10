
#include <renderMaterialDesc.h>

_NAMESPACE_BEGIN

RenderMaterialDesc::RenderMaterialDesc(void)
{
	type               = RenderMaterial::TYPE_UNLIT;
	alphaTestFunc      = RenderMaterial::ALPHA_TEST_ALWAYS;
	alphaTestRef       = 0;
	blending           = false;
	srcBlendFunc       = RenderMaterial::BLEND_ZERO;
	dstBlendFunc       = RenderMaterial::BLEND_ZERO;
	geometryShaderPath = 0;
	vertexShaderPath   = 0;
	fragmentShaderPath = 0;
}
		
bool RenderMaterialDesc::isValid(void) const
{
	bool ok = true;
	if(type >= RenderMaterial::NUM_TYPES) ok = false;
	if(alphaTestRef < 0 || alphaTestRef > 1) ok = false;
	if(blending && type != RenderMaterial::TYPE_UNLIT) ok = false;
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