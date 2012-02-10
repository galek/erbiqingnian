
#include <renderSpotLightDesc.h>

_NAMESPACE_BEGIN

RenderSpotLightDesc::RenderSpotLightDesc(void) :
	RenderLightDesc(RenderLight::TYPE_SPOT)
{
	position = Vector3(0,0,0);
	direction = Vector3(0,0,0);
	innerRadius = 0;
	outerRadius = 0;
	innerCone   = 0;
	outerCone   = 0;
}

bool RenderSpotLightDesc::isValid(void) const
{
	bool ok = RenderLightDesc::isValid();
	if(direction.squaredLength() < 0.1f) ok = false;
	if(innerRadius < 0)                     ok = false;
	if(outerRadius < innerRadius)           ok = false;
	if(innerCone > 1)                       ok = false;
	if(outerCone > innerCone)               ok = false;
	return ok;
}

_NAMESPACE_END