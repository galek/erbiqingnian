
#include "D3D9RendererIndexBuffer.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderIndexBufferDesc.h>

_NAMESPACE_BEGIN

static D3DFORMAT getD3D9Format(RendererIndexBuffer::Format format)
{
	D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
	switch(format)
	{
		case RendererIndexBuffer::FORMAT_UINT16: d3dFormat = D3DFMT_INDEX16; break;
		case RendererIndexBuffer::FORMAT_UINT32: d3dFormat = D3DFMT_INDEX32; break;
	}
	ph_assert2(d3dFormat != D3DFMT_UNKNOWN, "Unable to convert to D3DFORMAT.");
	return d3dFormat;
}

D3D9RendererIndexBuffer::D3D9RendererIndexBuffer(IDirect3DDevice9 &d3dDevice, const RendererIndexBufferDesc &desc) :
	RendererIndexBuffer(desc),
	m_d3dDevice(d3dDevice)
{
	m_d3dIndexBuffer = 0;

	m_usage      = 0;
	m_pool       = D3DPOOL_MANAGED;
	uint32	indexSize  = getFormatByteSize(desc.format);
	m_format     = getD3D9Format(desc.format);
    m_bufferSize = indexSize * desc.maxIndices;

#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint == RendererIndexBuffer::HINT_DYNAMIC)
	{
		m_usage = D3DUSAGE_DYNAMIC;
		m_pool  = D3DPOOL_DEFAULT;
	}
#endif

	onDeviceReset();

	if(m_d3dIndexBuffer)
	{
		m_maxIndices = desc.maxIndices;
	}
}

D3D9RendererIndexBuffer::~D3D9RendererIndexBuffer(void)
{
	if(m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Release();
	}
}

void D3D9RendererIndexBuffer::onDeviceLost(void)
{
	if(m_pool != D3DPOOL_MANAGED && m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Release();
		m_d3dIndexBuffer = 0;
	}
}

void D3D9RendererIndexBuffer::onDeviceReset(void)
{
	if(!m_d3dIndexBuffer)
	{
		m_d3dDevice.CreateIndexBuffer(m_bufferSize, m_usage, m_format, m_pool, &m_d3dIndexBuffer, 0);
		ph_assert2(m_d3dIndexBuffer, "Failed to create Direct3D9 Index Buffer.");
	}
}

void *D3D9RendererIndexBuffer::lock(void)
{
	void *buffer = 0;
	if(m_d3dIndexBuffer)
	{
		const Format format     = getFormat();
		const uint32  maxIndices = getMaxIndices();
		const uint32  bufferSize = maxIndices * getFormatByteSize(format);
		if(bufferSize > 0)
		{
			m_d3dIndexBuffer->Lock(0, (UINT)bufferSize, &buffer, 0);
		}
	}
	return buffer;
}

void D3D9RendererIndexBuffer::unlock(void)
{
	if(m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Unlock();
	}
}

void D3D9RendererIndexBuffer::bind(void) const
{
	m_d3dDevice.SetIndices(m_d3dIndexBuffer);
}

void D3D9RendererIndexBuffer::unbind(void) const
{
	m_d3dDevice.SetIndices(0);
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
