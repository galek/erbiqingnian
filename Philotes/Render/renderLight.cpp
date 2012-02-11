
#include <renderLight.h>
#include <renderLightDesc.h>

_NAMESPACE_BEGIN

RenderLight::RenderLight(const RenderLightDesc &desc) :
	m_type(desc.type)
{
	Math::makeProjectionMatrix(Radian(Degree(45)),1,0.1f,100.0f,m_shadowProjection);

	m_renderer = 0;
	setColor(desc.color);
	setIntensity(desc.intensity);
	setShadowMap(desc.shadowMap);
	setShadowTransform(desc.shadowTransform);
	setShadowProjection(desc.shadowProjection);
}

RenderLight::~RenderLight(void)
{
	ph_assert2(!isLocked(), "Light is locked by a Render during release.");
}

RenderLight::Type RenderLight::getType(void) const
{
	return m_type;
}

RenderMaterial::Pass RenderLight::getPass(void) const
{
	RenderMaterial::Pass pass = RenderMaterial::NUM_PASSES;
	switch(m_type)
	{
		case TYPE_POINT:       pass = RenderMaterial::PASS_POINT_LIGHT;       break;
		case TYPE_DIRECTIONAL: pass = RenderMaterial::PASS_DIRECTIONAL_LIGHT; break;
		case TYPE_SPOT:        pass = RenderMaterial::PASS_SPOT_LIGHT;        break;
	}
	ph_assert2(pass < RenderMaterial::NUM_PASSES, "Unable to compute the Pass for the Light.");
	return pass;
}

const Colour &RenderLight::getColor(void) const
{
	return m_color;
}

void RenderLight::setColor(const Colour &color)
{
	m_color = color;
}

float RenderLight::getIntensity(void) const
{
	return m_intensity;
}

void RenderLight::setIntensity(float intensity)
{
	ph_assert2(intensity >= 0, "Light intensity is negative.");
	if(intensity >= 0)
	{
		m_intensity = intensity;
	}
}

bool RenderLight::isLocked(void) const
{
	return m_renderer ? true : false;
}

RenderTexture2D *RenderLight::getShadowMap(void) const
{
	return m_shadowMap;
}

void RenderLight::setShadowMap(RenderTexture2D *shadowMap)
{
	m_shadowMap = shadowMap;
}

const Matrix4 &RenderLight::getShadowTransform(void) const
{
	return m_shadowTransform;
}

void RenderLight::setShadowTransform(const Matrix4 &shadowTransform)
{
	m_shadowTransform = shadowTransform;
}

const Matrix4 &RenderLight::getShadowProjection(void) const
{
	return m_shadowProjection;
}

void RenderLight::setShadowProjection(const Matrix4 &shadowProjection)
{
	m_shadowProjection = shadowProjection;
}

_NAMESPACE_END