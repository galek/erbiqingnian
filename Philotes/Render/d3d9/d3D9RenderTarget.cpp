
#include "D3D9RenderTarget.h"

#if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)

#include <renderTargetDesc.h>
#include "D3D9RenderTexture2D.h"

namespace Philo
{
	D3D9RenderTarget::D3D9RenderTarget(IDirect3DDevice9 &d3dDevice, const Philo::RenderTargetDesc &desc) :
		m_d3dDevice(d3dDevice)
	{
		m_d3dLastSurface = 0;
		m_d3dLastDepthStencilSurface = 0;
		m_d3dDepthStencilSurface = 0;
		for(uint32 i=0; i<desc.numTextures; i++)
		{
			D3D9RenderTexture2D &texture = *static_cast<D3D9RenderTexture2D*>(desc.textures[i]);
			m_textures.push_back(&texture);
		}
		m_depthStencilSurface = static_cast<D3D9RenderTexture2D*>(desc.depthStencilSurface);
		ph_assert2(m_depthStencilSurface && m_depthStencilSurface->m_d3dTexture, "Invalid Target Depth Stencil Surface!");
		onDeviceReset();
	}

	D3D9RenderTarget::~D3D9RenderTarget(void)
	{
		if(m_d3dDepthStencilSurface) m_d3dDepthStencilSurface->Release();
	}

	void D3D9RenderTarget::bind(void)
	{
		ph_assert2(m_d3dLastSurface==0 && m_d3dLastDepthStencilSurface==0, "Render Target in bad state!");
		if(m_d3dDepthStencilSurface && !m_d3dLastSurface && !m_d3dLastDepthStencilSurface)
		{
			m_d3dDevice.GetRenderTarget(0, &m_d3dLastSurface);
			m_d3dDevice.GetDepthStencilSurface(&m_d3dLastDepthStencilSurface);
			const uint32 numTextures = (uint32)m_textures.size();
			for(uint32 i=0; i<numTextures; i++)
			{
				IDirect3DSurface9 *d3dSurcace = 0;
				D3D9RenderTexture2D &texture = *m_textures[i];
				/* HRESULT result = */ texture.m_d3dTexture->GetSurfaceLevel(0, &d3dSurcace);
				ph_assert2(d3dSurcace, "Cannot get Texture Surface!");
				if(d3dSurcace)
				{
					m_d3dDevice.SetRenderTarget(i, d3dSurcace);
					d3dSurcace->Release();
				}
			}
			m_d3dDevice.SetDepthStencilSurface(m_d3dDepthStencilSurface);
			const DWORD flags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
			m_d3dDevice.Clear(0, 0, flags, 0x00000000, 1.0f, 0);
		}
		float depthBias = 0.0001f;
		float biasSlope = 1.58f;
	#if RENDERER_ENABLE_DRESSCODE
		depthBias = dcParam("depthBias", depthBias, 0.0f, 0.01f);
		biasSlope = dcParam("biasSlope", biasSlope, 0.0f, 5.0f);
	#endif
		m_d3dDevice.SetRenderState(D3DRS_DEPTHBIAS,           *(DWORD*)&depthBias);
		m_d3dDevice.SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&biasSlope);
	}

	void D3D9RenderTarget::unbind(void)
	{
		ph_assert2(m_d3dLastSurface && m_d3dLastDepthStencilSurface, "Render Target in bad state!");
		if(m_d3dDepthStencilSurface && m_d3dLastSurface && m_d3dLastDepthStencilSurface)
		{
			m_d3dDevice.SetDepthStencilSurface(m_d3dLastDepthStencilSurface);
			m_d3dDevice.SetRenderTarget(0, m_d3dLastSurface);
			const uint32 numTextures = (uint32)m_textures.size();
			for(uint32 i=1; i<numTextures; i++)
			{
				m_d3dDevice.SetRenderTarget(i, 0);
			}
			m_d3dLastSurface->Release();
			m_d3dLastSurface = 0;
			m_d3dLastDepthStencilSurface->Release();
			m_d3dLastDepthStencilSurface = 0;
		}
		float depthBias = 0;
		float biasSlope = 0;
		m_d3dDevice.SetRenderState(D3DRS_DEPTHBIAS,           *(DWORD*)&depthBias);
		m_d3dDevice.SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&biasSlope);
	}

	void D3D9RenderTarget::onDeviceLost(void)
	{
		ph_assert2(m_d3dLastDepthStencilSurface==0, "Render Target in bad state!");
		ph_assert2(m_d3dDepthStencilSurface,        "Render Target in bad state!");
		if(m_d3dDepthStencilSurface)
		{
			m_d3dDepthStencilSurface->Release();
			m_d3dDepthStencilSurface = 0;
		}
	}

	void D3D9RenderTarget::onDeviceReset(void)
	{
		ph_assert2(m_d3dDepthStencilSurface==0, "Render Target in bad state!");
		if(!m_d3dDepthStencilSurface && m_depthStencilSurface && m_depthStencilSurface->m_d3dTexture)
		{
			bool ok = m_depthStencilSurface->m_d3dTexture->GetSurfaceLevel(0, &m_d3dDepthStencilSurface) == D3D_OK;
			if(!ok)
			{
				ph_assert2(ok, "Failed to create Render Target Depth Stencil Surface.");
			}
		}
	}

}

#endif //#if defined(RENDERER_ENABLE_DIRECT3D9) && defined(RENDERER_ENABLE_DIRECT3D9_TARGET)
