
#ifndef D3D9_RENDERER_SPOT_LIGHT_H
#define D3D9_RENDERER_SPOT_LIGHT_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderSpotLight.h>
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9RenderSpotLight : public RenderSpotLight
{
	public:
		D3D9RenderSpotLight(D3D9Render &renderer, const RenderSpotLightDesc &desc);
		virtual ~D3D9RenderSpotLight(void);
		
		virtual void bind(void) const;
		
	private:
		D3D9Render &m_renderer;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
