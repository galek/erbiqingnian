
#include "D3D9RendererInstanceBuffer.h"
#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderInstanceBufferDesc.h>

_NAMESPACE_BEGIN

static D3DVERTEXELEMENT9 buildVertexElement(WORD stream, WORD offset, D3DDECLTYPE type, BYTE method, BYTE usage, BYTE usageIndex)
{
	D3DVERTEXELEMENT9 element;
	element.Stream     = stream;
	element.Offset     = offset;
#if defined(RENDERER_WINDOWS)
	element.Type       = (BYTE)type;
#else
	element.Type       = type;
#endif
	element.Method     = method;
	element.Usage      = usage;
	element.UsageIndex = usageIndex;
	return element;
}

static D3DDECLTYPE getD3DType(RendererInstanceBuffer::Format format)
{
	D3DDECLTYPE d3dType = D3DDECLTYPE_UNUSED;
	switch(format)
	{
		case RendererInstanceBuffer::FORMAT_FLOAT1:  d3dType = D3DDECLTYPE_FLOAT1;   break;
		case RendererInstanceBuffer::FORMAT_FLOAT2:  d3dType = D3DDECLTYPE_FLOAT2;   break;
		case RendererInstanceBuffer::FORMAT_FLOAT3:  d3dType = D3DDECLTYPE_FLOAT3;   break;
		case RendererInstanceBuffer::FORMAT_FLOAT4:  d3dType = D3DDECLTYPE_FLOAT4;   break;
	}
	ph_assert2(d3dType != D3DDECLTYPE_UNUSED, "Invalid Direct3D9 vertex type.");
	return d3dType;
}

static D3DDECLUSAGE getD3DUsage(RendererInstanceBuffer::Semantic semantic, uint8 &usageIndex)
{
	D3DDECLUSAGE d3dUsage = D3DDECLUSAGE_FOG;
	usageIndex = 0;
	switch(semantic)
	{
		case RendererInstanceBuffer::SEMANTIC_POSITION:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_INSTANCE_POSITION_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_NORMALX:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_INSTANCE_NORMALX_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_NORMALY:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_INSTANCE_NORMALY_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_NORMALZ:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_INSTANCE_NORMALZ_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_VELOCITY_LIFE:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_INSTANCE_VEL_LIFE_CHANNEL;
			break;
		case RendererInstanceBuffer::SEMANTIC_DENSITY:
			d3dUsage   = D3DDECLUSAGE_TEXCOORD;
			usageIndex = RENDERER_INSTANCE_DENSITY_CHANNEL;
			break;
	}
	ph_assert2(d3dUsage != D3DDECLUSAGE_FOG, "Invalid Direct3D9 vertex usage.");
	return d3dUsage;
}

D3D9RendererInstanceBuffer::D3D9RendererInstanceBuffer(IDirect3DDevice9 &d3dDevice, const RendererInstanceBufferDesc &desc) :
	RendererInstanceBuffer(desc)
#if RENDERER_INSTANCING
	,m_d3dDevice(d3dDevice)
#endif
{
#if RENDERER_INSTANCING
	m_d3dVertexBuffer = 0;
#endif
	
	m_usage      = 0;
	m_pool       = D3DPOOL_MANAGED;
	m_bufferSize = (UINT)(desc.maxInstances * m_stride);
	
#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint==RendererInstanceBuffer::HINT_DYNAMIC)
	{
		m_usage = D3DUSAGE_DYNAMIC;
		m_pool  = D3DPOOL_DEFAULT;
	}
#endif
	
	onDeviceReset();

#if RENDERER_INSTANCING	
	if(m_d3dVertexBuffer)
	{
		m_maxInstances = desc.maxInstances;
	}
#else
	m_maxInstances = desc.maxInstances;
	mInstanceBuffer = PX_ALLOC(m_maxInstances*m_stride);
#endif
}

D3D9RendererInstanceBuffer::~D3D9RendererInstanceBuffer(void)
{
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer) m_d3dVertexBuffer->Release();
#else
	free(mInstanceBuffer);
#endif
}

void D3D9RendererInstanceBuffer::addVertexElements(uint32 streamIndex, std::vector<D3DVERTEXELEMENT9> &vertexElements) const
{
	for(uint32 i=0; i<NUM_SEMANTICS; i++)
	{
		Semantic semantic = (Semantic)i;
		const SemanticDesc &sm = m_semanticDescs[semantic];
		if(sm.format < NUM_FORMATS)
		{
			uint8 d3dUsageIndex  = 0;
			D3DDECLUSAGE d3dUsage = getD3DUsage(semantic, d3dUsageIndex);
			vertexElements.push_back(buildVertexElement((WORD)streamIndex, (WORD)sm.offset, getD3DType(sm.format), D3DDECLMETHOD_DEFAULT, (BYTE)d3dUsage, d3dUsageIndex));
		}
	}
}

void *D3D9RendererInstanceBuffer::lock(void)
{
	RENDERER_PERFZONE(D3D9RenderIBlock);

	void *lockedBuffer = 0;
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer)
	{
		const uint32 bufferSize = m_maxInstances * m_stride;
		m_d3dVertexBuffer->Lock(0, (UINT)bufferSize, &lockedBuffer, 0);
		ph_assert2(lockedBuffer, "Failed to lock Direct3D9 Vertex Buffer.");
	}
#else
	lockedBuffer = mInstanceBuffer;
#endif
	return lockedBuffer;
}

void D3D9RendererInstanceBuffer::unlock(void)
{
	RENDERER_PERFZONE(D3D9RenderIBunlock);
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Unlock();
	}
#endif
}

void D3D9RendererInstanceBuffer::bind(uint32 streamID, uint32 firstInstance) const
{
#if RENDERER_INSTANCING
	if(m_d3dVertexBuffer)
	{
		m_d3dDevice.SetStreamSourceFreq((UINT)streamID, (UINT)(D3DSTREAMSOURCE_INSTANCEDATA | 1));
		m_d3dDevice.SetStreamSource(    (UINT)streamID, m_d3dVertexBuffer, firstInstance*m_stride, m_stride);
	}
#endif
}

void D3D9RendererInstanceBuffer::unbind(uint32 streamID) const
{
#if RENDERER_INSTANCING
	m_d3dDevice.SetStreamSource((UINT)streamID, 0, 0, 0);
#endif
}

void D3D9RendererInstanceBuffer::onDeviceLost(void)
{
#if RENDERER_INSTANCING


	if(m_pool != D3DPOOL_MANAGED && m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = 0;
	}
#endif
}

void D3D9RendererInstanceBuffer::onDeviceReset(void)
{
#if RENDERER_INSTANCING
	if(!m_d3dVertexBuffer)
	{
		m_d3dDevice.CreateVertexBuffer(m_bufferSize, m_usage, 0, m_pool, &m_d3dVertexBuffer, 0);
		ph_assert2(m_d3dVertexBuffer, "Failed to create Direct3D9 Vertex Buffer.");
	}
#endif
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
