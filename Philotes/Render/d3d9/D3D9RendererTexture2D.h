
#ifndef D3D9_RENDERER_TEXTURE_2D_H
#define D3D9_RENDERER_TEXTURE_2D_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderTexture2D.h>
#include "D3D9Renderer.h"

_NAMESPACE_BEGIN

class RendererTexture2DDesc;

class D3D9RendererTexture2D : public RendererTexture2D, public D3D9RendererResource
{
	friend class D3D9RendererTarget;
	friend class D3D9RendererSpotLight;
	public:
		D3D9RendererTexture2D(IDirect3DDevice9 &d3dDevice, const RendererTexture2DDesc &desc);
		virtual ~D3D9RendererTexture2D(void);
	
	public:
		virtual void *lockLevel(uint32 level, uint32 &pitch);
		virtual void  unlockLevel(uint32 level);
		
		void bind(uint32 samplerIndex);

		virtual	void	select(uint32 stageIndex)
		{
			bind(stageIndex);
		}

	private:
		virtual void onDeviceLost(void);
		virtual void onDeviceReset(void);
		
	private:
		IDirect3DDevice9          &m_d3dDevice;
		IDirect3DTexture9         *m_d3dTexture;
		
		DWORD                      m_usage;
		D3DPOOL                    m_pool;
		D3DFORMAT                  m_format;
		
		D3DTEXTUREFILTERTYPE       m_d3dMinFilter;
		D3DTEXTUREFILTERTYPE       m_d3dMagFilter;
		D3DTEXTUREFILTERTYPE       m_d3dMipFilter;
		D3DTEXTUREADDRESS          m_d3dAddressingU;
		D3DTEXTUREADDRESS          m_d3dAddressingV;
};

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
#endif
