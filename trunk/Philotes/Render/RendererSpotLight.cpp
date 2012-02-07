
#include <RendererSpotLight.h>
#include <RendererSpotLightDesc.h>


RendererSpotLight::RendererSpotLight(const RendererSpotLightDesc &desc) :
	RendererLight(desc)
{
	setPosition(desc.position);
	setDirection(desc.direction);
	setRadius(desc.innerRadius, desc.outerRadius);
	setCone(desc.innerCone, desc.outerCone);
}

RendererSpotLight::~RendererSpotLight(void)
{

}

const Vector3 &RendererSpotLight::getPosition(void) const
{
	return m_position;
}

void RendererSpotLight::setPosition(const Vector3 &pos)
{
	m_position = pos;
}

const Vector3 &RendererSpotLight::getDirection(void) const
{
	return m_direction;
}

void RendererSpotLight::setDirection(const Vector3 &dir)
{
	ph_assert2(dir.squaredLength() >= 0.1f, "Trying to give Direction Light invalid Direction value.");
	if(dir.squaredLength() >= 0.1f)
	{
		m_direction = dir;
		m_direction.normalise();
	}
}

scalar RendererSpotLight::getInnerRadius(void) const
{
	return m_innerRadius;
}

scalar RendererSpotLight::getOuterRadius(void) const
{
	return m_outerRadius;
}

void RendererSpotLight::setRadius(scalar innerRadius, scalar outerRadius)
{
	ph_assert2(innerRadius>=0 && innerRadius<=outerRadius, "Invalid Spot Light radius values.");
	if(innerRadius>=0 && innerRadius<=outerRadius)
	{
		m_innerRadius = innerRadius;
		m_outerRadius = outerRadius;
	}
}

scalar RendererSpotLight::getInnerCone(void) const
{
	return m_innerCone;
}

scalar RendererSpotLight::getOuterCone(void) const
{
	return m_outerCone;
}

void RendererSpotLight::setCone(scalar innerCone, scalar outerCone)
{
	ph_assert2(innerCone<=1 && innerCone>=outerCone, "Invalid Spot Light cone values.");
	if(innerCone<=1 && innerCone>=outerCone)
	{
		m_innerCone = innerCone;
		m_outerCone = outerCone;
	}
}
