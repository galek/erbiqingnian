
#ifndef D3D9_RENDERER_DIRECTIONAL_LIGHT_H
#define D3D9_RENDERER_DIRECTIONAL_LIGHT_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderDirectionalLight.h>

#include "D3D9Render.h"

_NAMESPACE_BEGIN

class D3D9RenderDirectionalLight : public RenderDirectionalLight
{
	public:
		D3D9RenderDirectionalLight(D3D9Render &renderer, const RenderDirectionalLightDesc &desc);
		virtual ~D3D9RenderDirectionalLight(void);
		
		virtual void bind(void) const;
		
	private:
		D3D9Render &m_renderer;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
