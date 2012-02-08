
#ifndef D3D9_RENDERER_SPOT_LIGHT_H
#define D3D9_RENDERER_SPOT_LIGHT_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderSpotLight.h>
#include "D3D9Renderer.h"

_NAMESPACE_BEGIN

class D3D9RendererSpotLight : public RendererSpotLight
{
	public:
		D3D9RendererSpotLight(D3D9Renderer &renderer, const RendererSpotLightDesc &desc);
		virtual ~D3D9RendererSpotLight(void);
		
		virtual void bind(void) const;
		
	private:
		D3D9Renderer &m_renderer;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
