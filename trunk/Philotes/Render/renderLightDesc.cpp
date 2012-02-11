
#include <renderLightDesc.h>

RenderLightDesc::RenderLightDesc(RenderLight::Type _type) :
	type(_type)
{
	color			= Colour(0,0,0,0);
	intensity		= 0;
	shadowMap		= 0;
	shadowTransform = Matrix4::IDENTITY;

	Math::makeProjectionMatrix(Radian(Degree(45)),1,0.1f,100.0f,shadowProjection);
}

bool RenderLightDesc::isValid(void) const
{
	bool ok = true;
	if(intensity < 0) ok = false;
	return ok;
}
