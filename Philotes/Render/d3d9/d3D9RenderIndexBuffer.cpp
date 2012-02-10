
#include "D3D9RenderIndexBuffer.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderIndexBufferDesc.h>

_NAMESPACE_BEGIN

static D3DFORMAT getD3D9Format(RenderIndexBuffer::Format format)
{
	D3DFORMAT d3dFormat = D3DFMT_UNKNOWN;
	switch(format)
	{
		case RenderIndexBuffer::FORMAT_UINT16: d3dFormat = D3DFMT_INDEX16; break;
		case RenderIndexBuffer::FORMAT_UINT32: d3dFormat = D3DFMT_INDEX32; break;
	}
	ph_assert2(d3dFormat != D3DFMT_UNKNOWN, "Unable to convert to D3DFORMAT.");
	return d3dFormat;
}

D3D9RenderIndexBuffer::D3D9RenderIndexBuffer(IDirect3DDevice9 &d3dDevice, const RenderIndexBufferDesc &desc) :
	RenderIndexBuffer(desc),
	m_d3dDevice(d3dDevice)
{
	m_d3dIndexBuffer = 0;

	m_usage      = 0;
	m_pool       = D3DPOOL_MANAGED;
	uint32	indexSize  = getFormatByteSize(desc.format);
	m_format     = getD3D9Format(desc.format);
    m_bufferSize = indexSize * desc.maxIndices;

#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint == RenderIndexBuffer::HINT_DYNAMIC)
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

D3D9RenderIndexBuffer::~D3D9RenderIndexBuffer(void)
{
	if(m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Release();
	}
}

void D3D9RenderIndexBuffer::onDeviceLost(void)
{
	if(m_pool != D3DPOOL_MANAGED && m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Release();
		m_d3dIndexBuffer = 0;
	}
}

void D3D9RenderIndexBuffer::onDeviceReset(void)
{
	if(!m_d3dIndexBuffer)
	{
		m_d3dDevice.CreateIndexBuffer(m_bufferSize, m_usage, m_format, m_pool, &m_d3dIndexBuffer, 0);
		ph_assert2(m_d3dIndexBuffer, "Failed to create Direct3D9 Index Buffer.");
	}
}

void *D3D9RenderIndexBuffer::lock(void)
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

void D3D9RenderIndexBuffer::unlock(void)
{
	if(m_d3dIndexBuffer)
	{
		m_d3dIndexBuffer->Unlock();
	}
}

void D3D9RenderIndexBuffer::bind(void) const
{
	m_d3dDevice.SetIndices(m_d3dIndexBuffer);
}

void D3D9RenderIndexBuffer::unbind(void) const
{
	m_d3dDevice.SetIndices(0);
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
