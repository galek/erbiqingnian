
#include <renderSpotLight.h>
#include <renderSpotLightDesc.h>

_NAMESPACE_BEGIN

RenderSpotLight::RenderSpotLight(const RenderSpotLightDesc &desc) :
	RenderLight(desc)
{
	setPosition(desc.position);
	setDirection(desc.direction);
	setRadius(desc.innerRadius, desc.outerRadius);
	setCone(desc.innerCone, desc.outerCone);
}

RenderSpotLight::~RenderSpotLight(void)
{

}

const Vector3 &RenderSpotLight::getPosition(void) const
{
	return m_position;
}

void RenderSpotLight::setPosition(const Vector3 &pos)
{
	m_position = pos;
}

const Vector3 &RenderSpotLight::getDirection(void) const
{
	return m_direction;
}

void RenderSpotLight::setDirection(const Vector3 &dir)
{
	ph_assert2(dir.squaredLength() >= 0.1f, "Trying to give Direction Light invalid Direction value.");
	if(dir.squaredLength() >= 0.1f)
	{
		m_direction = dir;
		m_direction.normalise();
	}
}

scalar RenderSpotLight::getInnerRadius(void) const
{
	return m_innerRadius;
}

scalar RenderSpotLight::getOuterRadius(void) const
{
	return m_outerRadius;
}

void RenderSpotLight::setRadius(scalar innerRadius, scalar outerRadius)
{
	ph_assert2(innerRadius>=0 && innerRadius<=outerRadius, "Invalid Spot Light radius values.");
	if(innerRadius>=0 && innerRadius<=outerRadius)
	{
		m_innerRadius = innerRadius;
		m_outerRadius = outerRadius;
	}
}

scalar RenderSpotLight::getInnerCone(void) const
{
	return m_innerCone;
}

scalar RenderSpotLight::getOuterCone(void) const
{
	return m_outerCone;
}

void RenderSpotLight::setCone(scalar innerCone, scalar outerCone)
{
	ph_assert2(innerCone<=1 && innerCone>=outerCone, "Invalid Spot Light cone values.");
	if(innerCone<=1 && innerCone>=outerCone)
	{
		m_innerCone = innerCone;
		m_outerCone = outerCone;
	}
}

_NAMESPACE_END