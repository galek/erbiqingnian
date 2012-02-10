
#include <renderDirectionalLight.h>
#include <renderDirectionalLightDesc.h>

_NAMESPACE_BEGIN

RenderDirectionalLight::RenderDirectionalLight(const RenderDirectionalLightDesc &desc) :
	RenderLight(desc)
{
	setDirection(desc.direction);
}

RenderDirectionalLight::~RenderDirectionalLight(void)
{

}

const Vector3 &RenderDirectionalLight::getDirection(void) const
{
	return m_direction;
}

void RenderDirectionalLight::setDirection(const Vector3 &dir)
{
	ph_assert2(dir.squaredLength() >= 0.1f, "Trying to give Direction Light invalid Direction value.");
	if(dir.squaredLength() >= 0.1f)
	{
		m_direction = dir;
		m_direction.normalise();
	}
}

_NAMESPACE_END