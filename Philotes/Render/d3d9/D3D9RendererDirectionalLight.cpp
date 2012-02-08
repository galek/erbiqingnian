
#include "D3D9RendererDirectionalLight.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

_NAMESPACE_BEGIN

D3D9RendererDirectionalLight::D3D9RendererDirectionalLight(D3D9Renderer &renderer, const RendererDirectionalLightDesc &desc) :
	RendererDirectionalLight(desc),
	m_renderer(renderer)
{
	
}

D3D9RendererDirectionalLight::~D3D9RendererDirectionalLight(void)
{
	
}

void D3D9RendererDirectionalLight::bind(void) const
{
	D3D9Renderer::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
	convertToD3D9(shaderEnv.lightColor, m_color);
	shaderEnv.lightIntensity = m_intensity;
	convertToD3D9(shaderEnv.lightDirection, m_direction);
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
