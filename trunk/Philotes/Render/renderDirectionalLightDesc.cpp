
#include <renderDirectionalLightDesc.h>

_NAMESPACE_BEGIN

RenderDirectionalLightDesc::RenderDirectionalLightDesc(void) :
	RenderLightDesc(RenderLight::TYPE_DIRECTIONAL)
{
	direction = Vector3(0,0,0);
}

bool RenderDirectionalLightDesc::isValid(void) const
{
	bool ok = RenderLightDesc::isValid();
	if(direction.squaredLength() < 0.1f) ok = false;
	return ok;
}

_NAMESPACE_END