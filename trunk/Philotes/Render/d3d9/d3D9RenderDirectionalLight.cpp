
#include "D3D9RenderDirectionalLight.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

_NAMESPACE_BEGIN

D3D9RenderDirectionalLight::D3D9RenderDirectionalLight(D3D9Render &renderer, const RenderDirectionalLightDesc &desc) :
	RenderDirectionalLight(desc),
	m_renderer(renderer)
{
	
}

D3D9RenderDirectionalLight::~D3D9RenderDirectionalLight(void)
{
	
}

void D3D9RenderDirectionalLight::bind(void) const
{
	D3D9Render::ShaderEnvironment &shaderEnv = m_renderer.getShaderEnvironment();
	convertToD3D9(shaderEnv.lightColor, m_color);
	shaderEnv.lightIntensity = m_intensity;
	convertToD3D9(shaderEnv.lightDirection, m_direction);
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
