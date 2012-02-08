
#include "D3D9RendererSpotLight.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include "D3D9RendererTexture2D.h"

_NAMESPACE_BEGIN

D3D9RendererSpotLight::D3D9RendererSpotLight(D3D9Renderer &renderer, const RendererSpotLightDesc &desc) :
	RendererSpotLight(desc),
	m_renderer(renderer)
{
	
}

D3D9RendererSpotLight::~D3D9RendererSpotLight(void)
{
	
}

void D3D9RendererSpotLight::bind(void) const
{
	D3D9Renderer::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
	convertToD3D9(shaderEnv.lightColor, m_color);
	shaderEnv.lightIntensity = m_intensity;
	convertToD3D9(shaderEnv.lightPosition,  m_position);
	convertToD3D9(shaderEnv.lightDirection, m_direction);
	shaderEnv.lightInnerRadius = m_innerRadius;
	shaderEnv.lightOuterRadius = m_outerRadius;
	shaderEnv.lightInnerCone   = m_innerCone;
	shaderEnv.lightOuterCone   = m_outerCone;
	shaderEnv.lightShadowMap   = m_shadowMap ? static_cast<D3D9RendererTexture2D*>(m_shadowMap)->m_d3dTexture : 0;
	RendererProjection::buildProjectMatrix(shaderEnv.lightShadowMatrix, m_shadowProjection, m_shadowTransform);
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
