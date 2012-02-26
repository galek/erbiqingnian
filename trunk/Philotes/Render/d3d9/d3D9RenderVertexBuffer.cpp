
#include "D3D9RenderVertexBuffer.h"

#if defined(RENDERER_ENABLE_DIRECT3D9)

#include <renderVertexBufferDesc.h>

_NAMESPACE_BEGIN

static D3DVERTEXELEMENT9 buildVertexElement(WORD stream, WORD offset, D3DDECLTYPE type, BYTE method, BYTE usage, BYTE usageIndex)
{
	D3DVERTEXELEMENT9 element;
	element.Stream     = stream;
	element.Offset     = offset;
	element.Type	   = (BYTE)type;
	element.Method     = method;
	element.Usage      = usage;
	element.UsageIndex = usageIndex;
	return element;
}

static D3DDECLTYPE getD3DType(RenderVertexBuffer::Format format)
{
	D3DDECLTYPE d3dType = D3DDECLTYPE_UNUSED;
	switch(format)
	{
		case RenderVertexBuffer::FORMAT_FLOAT1:  d3dType = D3DDECLTYPE_FLOAT1;   break;
		case RenderVertexBuffer::FORMAT_FLOAT2:  d3dType = D3DDECLTYPE_FLOAT2;   break;
		case RenderVertexBuffer::FORMAT_FLOAT3:  d3dType = D3DDECLTYPE_FLOAT3;   break;
		case RenderVertexBuffer::FORMAT_FLOAT4:  d3dType = D3DDECLTYPE_FLOAT4;   break;
		case RenderVertexBuffer::FORMAT_UBYTE4:  d3dType = D3DDECLTYPE_UBYTE4;   break;
		case RenderVertexBuffer::FORMAT_USHORT4: d3dType = D3DDECLTYPE_SHORT4;   break;
		case RenderVertexBuffer::FORMAT_USHORT2: d3dType = D3DDECLTYPE_SHORT2;   break;
		case RenderVertexBuffer::FORMAT_COLOR:   d3dType = D3DDECLTYPE_D3DCOLOR; break;
	}
	ph_assert2(d3dType != D3DDECLTYPE_UNUSED, "Invalid Direct3D9 vertex type.");
	return d3dType;
}

static D3DDECLUSAGE getD3DUsage(RenderVertexBuffer::Semantic semantic, uint8 &usageIndex)
{
	D3DDECLUSAGE d3dUsage = D3DDECLUSAGE_FOG;
	usageIndex = 0;
	if(semantic >= RenderVertexBuffer::SEMANTIC_TEXCOORD0 && semantic <= RenderVertexBuffer::SEMANTIC_TEXCOORDMAX)
	{
		d3dUsage   = D3DDECLUSAGE_TEXCOORD;
		usageIndex = (uint8)(semantic - RenderVertexBuffer::SEMANTIC_TEXCOORD0);
	}
	else
	{
		switch(semantic)
		{
			case RenderVertexBuffer::SEMANTIC_POSITION:  d3dUsage = D3DDECLUSAGE_POSITION; break;
			case RenderVertexBuffer::SEMANTIC_NORMAL:    d3dUsage = D3DDECLUSAGE_NORMAL;   break;
			case RenderVertexBuffer::SEMANTIC_TANGENT:   d3dUsage = D3DDECLUSAGE_TANGENT;  break;
			case RenderVertexBuffer::SEMANTIC_COLOR:     d3dUsage = D3DDECLUSAGE_COLOR;    break;
			case RenderVertexBuffer::SEMANTIC_BONEINDEX:
				d3dUsage   = D3DDECLUSAGE_TEXCOORD;
				usageIndex = RENDERER_BONEINDEX_CHANNEL;
				break;
			case RenderVertexBuffer::SEMANTIC_BONEWEIGHT:
				d3dUsage   = D3DDECLUSAGE_TEXCOORD;
				usageIndex = RENDERER_BONEWEIGHT_CHANNEL;
				break;
		}
	}
	ph_assert2(d3dUsage != D3DDECLUSAGE_FOG, "Invalid Direct3D9 vertex usage.");
	return d3dUsage;
}

D3D9RenderVertexBuffer::D3D9RenderVertexBuffer(IDirect3DDevice9 &d3dDevice, const RenderVertexBufferDesc &desc, bool deferredUnlock) :
	RenderVertexBuffer(desc),
	m_d3dDevice(d3dDevice)
{
	m_d3dVertexBuffer = 0;
	m_deferredUnlock = deferredUnlock;
	
	m_usage      = 0;
	m_pool       = D3DPOOL_MANAGED;
	m_bufferSize = (UINT)(desc.maxVertices * m_stride);

	m_bufferWritten = false;
	
#if RENDERER_ENABLE_DYNAMIC_VB_POOLS
	if(desc.hint==RenderVertexBuffer::HINT_DYNAMIC)
	{
		m_usage = D3DUSAGE_DYNAMIC;
		m_pool  = D3DPOOL_DEFAULT;
	}
#endif
	
	onDeviceReset();
	
	if(m_d3dVertexBuffer)
	{
		m_maxVertices = desc.maxVertices;
	}
}

D3D9RenderVertexBuffer::~D3D9RenderVertexBuffer(void)
{
	if(m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Release();
	}
}

void D3D9RenderVertexBuffer::addVertexElements(uint32 streamIndex, std::vector<D3DVERTEXELEMENT9> &vertexElements) const
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

void *D3D9RenderVertexBuffer::lockImpl(void)
{
	RENDERER_PERFZONE(D3D9RenderVBlock);
	void *lockedBuffer = 0;
	if(m_d3dVertexBuffer)
	{
		const uint32 bufferSize = m_maxVertices * m_stride;
		m_d3dVertexBuffer->Lock(0, (UINT)bufferSize, &lockedBuffer, 0);
		ph_assert2(lockedBuffer, "Failed to lock Direct3D9 Vertex Buffer.");
	}
	m_bufferWritten = true;
	return lockedBuffer;
}

void D3D9RenderVertexBuffer::unlockImpl(void)
{
	RENDERER_PERFZONE(D3D9RenderVBunlock);
	if(m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Unlock();
	}
}

void D3D9RenderVertexBuffer::bind(uint32 streamID, uint32 firstVertex)
{
	prepareForRender();
	if(m_d3dVertexBuffer)
	{
		m_d3dDevice.SetStreamSource((UINT)streamID, m_d3dVertexBuffer, firstVertex*m_stride, m_stride);
	}
}

void D3D9RenderVertexBuffer::unbind(uint32 streamID)
{
	m_d3dDevice.SetStreamSource((UINT)streamID, 0, 0, 0);
}

void D3D9RenderVertexBuffer::onDeviceLost(void)
{
	if(m_pool != D3DPOOL_MANAGED && m_d3dVertexBuffer)
	{
		m_d3dVertexBuffer->Release();
		m_d3dVertexBuffer = 0;
	}
}

void D3D9RenderVertexBuffer::onDeviceReset(void)
{
	if(!m_d3dVertexBuffer)
	{
		m_d3dDevice.CreateVertexBuffer(m_bufferSize, m_usage, 0, m_pool, &m_d3dVertexBuffer, 0);
		ph_assert2(m_d3dVertexBuffer, "Failed to create Direct3D9 Vertex Buffer.");
		m_bufferWritten = false;
	}
}

bool D3D9RenderVertexBuffer::checkBufferWritten(void)
{
	return m_bufferWritten;
}

_NAMESPACE_END

#endif // #if defined(RENDERER_ENABLE_DIRECT3D9)
