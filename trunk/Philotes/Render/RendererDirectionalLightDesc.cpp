
#include <RendererDirectionalLightDesc.h>

RendererDirectionalLightDesc::RendererDirectionalLightDesc(void) :
	RendererLightDesc(RendererLight::TYPE_DIRECTIONAL)
{
	direction = Vector3(0,0,0);
}

bool RendererDirectionalLightDesc::isValid(void) const
{
	bool ok = RendererLightDesc::isValid();
	if(direction.squaredLength() < 0.1f) ok = false;
	return ok;
}
