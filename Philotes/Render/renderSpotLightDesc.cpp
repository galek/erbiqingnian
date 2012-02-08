
#include <renderSpotLightDesc.h>

_NAMESPACE_BEGIN

RendererSpotLightDesc::RendererSpotLightDesc(void) :
	RendererLightDesc(RendererLight::TYPE_SPOT)
{
	position = Vector3(0,0,0);
	direction = Vector3(0,0,0);
	innerRadius = 0;
	outerRadius = 0;
	innerCone   = 0;
	outerCone   = 0;
}

bool RendererSpotLightDesc::isValid(void) const
{
	bool ok = RendererLightDesc::isValid();
	if(direction.squaredLength() < 0.1f) ok = false;
	if(innerRadius < 0)                     ok = false;
	if(outerRadius < innerRadius)           ok = false;
	if(innerCone > 1)                       ok = false;
	if(outerCone > innerCone)               ok = false;
	return ok;
}

_NAMESPACE_END