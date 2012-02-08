
#ifndef D3D9_RENDERER_DIRECTIONAL_LIGHT_H
#define D3D9_RENDERER_DIRECTIONAL_LIGHT_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderDirectionalLight.h>

#include "D3D9Renderer.h"

_NAMESPACE_BEGIN

class D3D9RendererDirectionalLight : public RendererDirectionalLight
{
	public:
		D3D9RendererDirectionalLight(D3D9Renderer &renderer, const RendererDirectionalLightDesc &desc);
		virtual ~D3D9RendererDirectionalLight(void);
		
		virtual void bind(void) const;
		
	private:
		D3D9Renderer &m_renderer;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
