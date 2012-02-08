
#include <renderLight.h>
#include <renderLightDesc.h>

_NAMESPACE_BEGIN

RendererLight::RendererLight(const RendererLightDesc &desc) :
	m_type(desc.type)
{
	RendererProjection::makeProjectionMatrix(Radian(Degree(45)),1,0.1f,100.0f,m_shadowProjection);

	m_renderer = 0;
	setColor(desc.color);
	setIntensity(desc.intensity);
	setShadowMap(desc.shadowMap);
	setShadowTransform(desc.shadowTransform);
	setShadowProjection(desc.shadowProjection);
}

RendererLight::~RendererLight(void)
{
	ph_assert2(!isLocked(), "Light is locked by a Renderer during release.");
}

RendererLight::Type RendererLight::getType(void) const
{
	return m_type;
}

RendererMaterial::Pass RendererLight::getPass(void) const
{
	RendererMaterial::Pass pass = RendererMaterial::NUM_PASSES;
	switch(m_type)
	{
		case TYPE_POINT:       pass = RendererMaterial::PASS_POINT_LIGHT;       break;
		case TYPE_DIRECTIONAL: pass = RendererMaterial::PASS_DIRECTIONAL_LIGHT; break;
		case TYPE_SPOT:        pass = RendererMaterial::PASS_SPOT_LIGHT;        break;
	}
	ph_assert2(pass < RendererMaterial::NUM_PASSES, "Unable to compute the Pass for the Light.");
	return pass;
}

const RendererColor &RendererLight::getColor(void) const
{
	return m_color;
}

void RendererLight::setColor(const RendererColor &color)
{
	m_color = color;
}

float RendererLight::getIntensity(void) const
{
	return m_intensity;
}

void RendererLight::setIntensity(float intensity)
{
	ph_assert2(intensity >= 0, "Light intensity is negative.");
	if(intensity >= 0)
	{
		m_intensity = intensity;
	}
}

bool RendererLight::isLocked(void) const
{
	return m_renderer ? true : false;
}

RendererTexture2D *RendererLight::getShadowMap(void) const
{
	return m_shadowMap;
}

void RendererLight::setShadowMap(RendererTexture2D *shadowMap)
{
	m_shadowMap = shadowMap;
}

const Matrix4 &RendererLight::getShadowTransform(void) const
{
	return m_shadowTransform;
}

void RendererLight::setShadowTransform(const Matrix4 &shadowTransform)
{
	m_shadowTransform = shadowTransform;
}

const Matrix4 &RendererLight::getShadowProjection(void) const
{
	return m_shadowProjection;
}

void RendererLight::setShadowProjection(const Matrix4 &shadowProjection)
{
	m_shadowProjection = shadowProjection;
}

_NAMESPACE_END