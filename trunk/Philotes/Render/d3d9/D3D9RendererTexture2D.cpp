
#include "D3D9RendererTexture2D.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <RendererTexture2DDesc.h>

static D3DFORMAT getD3D9TextureFormat(RendererTexture2D::Format format)
{
	D3DFORMAT d3dFormat = static_cast<D3DFORMAT>(SamplePlatform::platform()->getD3D9TextureFormat(format));
	ph_assert2(d3dFormat != D3DFMT_UNKNOWN, "Unable to convert to D3D9 Texture Format.");
	return d3dFormat;
}

static D3DTEXTUREFILTERTYPE getD3D9TextureFilter(RendererTexture2D::Filter filter)
{
	D3DTEXTUREFILTERTYPE d3dFilter = D3DTEXF_FORCE_DWORD;
	switch(filter)
	{
		case RendererTexture2D::FILTER_NEAREST:     d3dFilter = D3DTEXF_POINT;       break;
		case RendererTexture2D::FILTER_LINEAR:      d3dFilter = D3DTEXF_LINEAR;      break;
		case RendererTexture2D::FILTER_ANISOTROPIC: d3dFilter = D3DTEXF_ANISOTROPIC; break;
	}
	ph_assert2(d3dFilter != D3DTEXF_FORCE_DWORD, "Unable to convert to D3D9 Filter mode.");
	return d3dFilter;
}

static D3DTEXTUREADDRESS getD3D9TextureAddressing(RendererTexture2D::Addressing addressing)
{
	D3DTEXTUREADDRESS d3dAddressing = D3DTADDRESS_FORCE_DWORD;
	switch(addressing)
	{
		case RendererTexture2D::ADDRESSING_WRAP:   d3dAddressing = D3DTADDRESS_WRAP;   break;
		case RendererTexture2D::ADDRESSING_CLAMP:  d3dAddressing = D3DTADDRESS_CLAMP;  break;
		case RendererTexture2D::ADDRESSING_MIRROR: d3dAddressing = D3DTADDRESS_MIRROR; break;
	}
	ph_assert2(d3dAddressing != D3DTADDRESS_FORCE_DWORD, "Unable to convert to D3D9 Addressing mode.");
	return d3dAddressing;
}

D3D9RendererTexture2D::D3D9RendererTexture2D(IDirect3DDevice9 &d3dDevice, const RendererTexture2DDesc &desc) :
	RendererTexture2D(desc),
	m_d3dDevice(d3dDevice)
{
	m_d3dTexture     = 0;
	m_usage          = 0;
	m_pool           = D3DPOOL_MANAGED;
	m_format         = getD3D9TextureFormat(desc.format);
	m_d3dMinFilter   = getD3D9TextureFilter(desc.filter);
	m_d3dMagFilter   = getD3D9TextureFilter(desc.filter);
	m_d3dMipFilter   = getD3D9TextureFilter(desc.filter);
	m_d3dAddressingU = getD3D9TextureAddressing(desc.addressingU);
	m_d3dAddressingV = getD3D9TextureAddressing(desc.addressingV);
	if(desc.renderTarget)
	{
		m_usage = D3DUSAGE_RENDERTARGET; 
		m_pool  = D3DPOOL_DEFAULT;
	}
	if(isDepthStencilFormat(desc.format))
	{
		m_usage = D3DUSAGE_DEPTHSTENCIL; 
		m_pool  = D3DPOOL_DEFAULT;
	}
	onDeviceReset();
}

D3D9RendererTexture2D::~D3D9RendererTexture2D(void)
{
	if(m_d3dTexture)
	{
		m_d3dDevice.SetTexture(0, NULL);
		SamplePlatform::platform()->D3D9BlockUntilNotBusy(m_d3dTexture);
		m_d3dTexture->Release();
	}
}

void *D3D9RendererTexture2D::lockLevel(uint32 level, uint32 &pitch)
{
	void *buffer = 0;
	if(m_d3dTexture)
	{
		D3DLOCKED_RECT lockedRect;
		HRESULT result = m_d3dTexture->LockRect((DWORD)level, &lockedRect, 0, D3DLOCK_NOSYSLOCK);
		ph_assert2(result == D3D_OK, "Unable to lock Texture 2D.");
		if(result == D3D_OK)
		{
			buffer = lockedRect.pBits;
			pitch  = (uint32)lockedRect.Pitch;
		}
	}
	return buffer;
}

void D3D9RendererTexture2D::unlockLevel(uint32 level)
{
	if(m_d3dTexture)
	{
		m_d3dTexture->UnlockRect(level);
	}
}

void D3D9RendererTexture2D::bind(uint32 samplerIndex)
{
	m_d3dDevice.SetTexture(     (DWORD)samplerIndex, m_d3dTexture);
	m_d3dDevice.SetSamplerState((DWORD)samplerIndex, D3DSAMP_MINFILTER, m_d3dMinFilter);
	m_d3dDevice.SetSamplerState((DWORD)samplerIndex, D3DSAMP_MAGFILTER, m_d3dMagFilter);
	m_d3dDevice.SetSamplerState((DWORD)samplerIndex, D3DSAMP_MIPFILTER, m_d3dMipFilter);
	m_d3dDevice.SetSamplerState((DWORD)samplerIndex, D3DSAMP_ADDRESSU,  m_d3dAddressingU);
	m_d3dDevice.SetSamplerState((DWORD)samplerIndex, D3DSAMP_ADDRESSV,  m_d3dAddressingV);
}

void D3D9RendererTexture2D::onDeviceLost(void)
{
	if(m_pool != D3DPOOL_MANAGED)
	{
		if(m_d3dTexture)
		{
			m_d3dTexture->Release();
			m_d3dTexture = 0;
		}
	}
}

void D3D9RendererTexture2D::onDeviceReset(void)
{
	if(!m_d3dTexture)
	{
		HRESULT result   = m_d3dDevice.CreateTexture((UINT)getWidth(), (UINT)getHeight(), (UINT)getNumLevels(), m_usage, m_format, m_pool, &m_d3dTexture, 0);
		ph_assert2(result == D3D_OK, "Unable to create D3D9 Texture.");
		if(result == D3D_OK)
		{
			
		}
	}
}

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
