
#include "D3D9RenderSpotLight.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "D3D9RenderTexture2D.h"

_NAMESPACE_BEGIN

D3D9RenderSpotLight::D3D9RenderSpotLight(D3D9Render &renderer, const RenderSpotLightDesc &desc) :
	RenderSpotLight(desc),
	m_renderer(renderer)
{
	
}

D3D9RenderSpotLight::~D3D9RenderSpotLight(void)
{
	
}

void D3D9RenderSpotLight::bind(void) const
{
	D3D9Render::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
	convertToD3D9(shaderEnv.lightColor, m_color);
	shaderEnv.lightIntensity = m_intensity;
	convertToD3D9(shaderEnv.lightPosition,  m_position);
	convertToD3D9(shaderEnv.lightDirection, m_direction);
	shaderEnv.lightInnerRadius = m_innerRadius;
	shaderEnv.lightOuterRadius = m_outerRadius;
	shaderEnv.lightInnerCone   = m_innerCone;
	shaderEnv.lightOuterCone   = m_outerCone;
	shaderEnv.lightShadowMap   = m_shadowMap ? static_cast<D3D9RenderTexture2D*>(m_shadowMap)->m_d3dTexture : 0;
	Math::buildProjectMatrix(shaderEnv.lightShadowMatrix, m_shadowProjection, m_shadowTransform);
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
