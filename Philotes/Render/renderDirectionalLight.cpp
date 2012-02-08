
#include <renderDirectionalLight.h>
#include <renderDirectionalLightDesc.h>

_NAMESPACE_BEGIN

RendererDirectionalLight::RendererDirectionalLight(const RendererDirectionalLightDesc &desc) :
	RendererLight(desc)
{
	setDirection(desc.direction);
}

RendererDirectionalLight::~RendererDirectionalLight(void)
{

}

const Vector3 &RendererDirectionalLight::getDirection(void) const
{
	return m_direction;
}

void RendererDirectionalLight::setDirection(const Vector3 &dir)
{
	ph_assert2(dir.squaredLength() >= 0.1f, "Trying to give Direction Light invalid Direction value.");
	if(dir.squaredLength() >= 0.1f)
	{
		m_direction = dir;
		m_direction.normalise();
	}
}

_NAMESPACE_END