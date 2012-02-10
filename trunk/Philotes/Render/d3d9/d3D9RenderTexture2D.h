
#ifndef D3D9_RENDERER_TEXTURE_2D_H
#define D3D9_RENDERER_TEXTURE_2D_H

#include <renderConfig.h>

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderTexture2D.h>
#include "D3D9Render.h"

_NAMESPACE_BEGIN

class RenderTexture2DDesc;

class D3D9RenderTexture2D : public RenderTexture2D, public D3D9RenderResource
{
	friend class D3D9RenderTarget;
	friend class D3D9RenderSpotLight;
	public:
		D3D9RenderTexture2D(IDirect3DDevice9 &d3dDevice, const RenderTexture2DDesc &desc);
		virtual ~D3D9RenderTexture2D(void);
	
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
