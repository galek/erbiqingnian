
#ifndef D3D9_RENDERER_TARGET_H
#define D3D9_RENDERER_TARGET_H

#include <renderConfig.h>

// TODO: 360 can't render directly into texture memory, so we need to create a pool of
//       surfaces to share internally and then "resolve" them into the textures inside of
//       the 'unbind' call.
#if defined(RENDERER_WINDOWS)
	#define RENDERER_ENABLE_DIRECT3D9_TARGET
#endif

#if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)

#include <renderTarget.h>
#include "renderTargetDesc.h"
#include "D3D9Renderer.h"

_NAMESPACE_BEGIN

class D3D9RendererTexture2D;

class D3D9RendererTarget : public RendererTarget, public D3D9RendererResource
{
	public:
		D3D9RendererTarget(IDirect3DDevice9 &d3dDevice, const Philo::RendererTargetDesc &desc);
		virtual ~D3D9RendererTarget(void);
		
	private:
		D3D9RendererTarget& operator=( const D3D9RendererTarget& ) {}
		virtual void bind(void);
		virtual void unbind(void);
	
	private:
		virtual void onDeviceLost(void);
		virtual void onDeviceReset(void);
	
	private:
		IDirect3DDevice9               &m_d3dDevice;
		IDirect3DSurface9              *m_d3dLastSurface;
		IDirect3DSurface9              *m_d3dLastDepthStencilSurface;
		IDirect3DSurface9              *m_d3dDepthStencilSurface;
		std::vector<D3D9RendererTexture2D*> m_textures;
		D3D9RendererTexture2D          *m_depthStencilSurface;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
#endif
