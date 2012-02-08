
#include <renderVertexBuffer.h>
#include <renderVertexBufferDesc.h>

_NAMESPACE_BEGIN

uint32 RendererVertexBuffer::getFormatByteSize(Format format)
{
	uint32 size = 0;
	switch(format)
	{
		case FORMAT_FLOAT1:   size = sizeof(float) * 1; break;
		case FORMAT_FLOAT2:   size = sizeof(float) * 2; break;
		case FORMAT_FLOAT3:   size = sizeof(float) * 3; break;
		case FORMAT_FLOAT4:   size = sizeof(float) * 4; break;
		case FORMAT_UBYTE4:   size = sizeof(uint8)  * 4; break;
		case FORMAT_USHORT4:  size = sizeof(uint16) * 4; break;
		case FORMAT_COLOR:    size = sizeof(uint8)  * 4; break;
	}
	ph_assert2(size, "Unable to determine size of Format.");
	return size;
}

RendererVertexBuffer::RendererVertexBuffer(const RendererVertexBufferDesc &desc) :
	m_hint(desc.hint),
	m_deferredUnlock(true)
{
	m_maxVertices      = 0;
	m_stride           = 0;
	m_lockedBuffer     = 0;
	m_numSemanticLocks = 0;
	
	for(uint32 i=0; i<NUM_SEMANTICS; i++)
	{
		Format format = desc.semanticFormats[i];
		if(format < NUM_FORMATS)
		{
			SemanticDesc &sm  = m_semanticDescs[i];
			sm.format         = format;
			sm.offset         = m_stride;
			m_stride         += getFormatByteSize(format);
		}
	}
}

RendererVertexBuffer::~RendererVertexBuffer(void)
{
	ph_assert2(m_numSemanticLocks==0, "VertexBuffer had outstanding locks during destruction!");
}

uint32 RendererVertexBuffer::getMaxVertices(void) const
{
	return m_maxVertices;
}

RendererVertexBuffer::Hint RendererVertexBuffer::getHint(void) const
{
	return m_hint;
}

RendererVertexBuffer::Format RendererVertexBuffer::getFormatForSemantic(Semantic semantic) const
{
	ph_assert2(semantic < NUM_SEMANTICS, "Invalid VertexBuffer Semantic!");
	return m_semanticDescs[semantic].format;
}

void *RendererVertexBuffer::lockSemantic(Semantic semantic, uint32 &stride)
{
	void *semanticBuffer = 0;
	ph_assert2(semantic < NUM_SEMANTICS, "Invalid VertexBuffer Semantic!");
	if(semantic < NUM_SEMANTICS)
	{
		SemanticDesc &sm = m_semanticDescs[semantic];
		ph_assert2(!sm.locked,              "VertexBuffer Semantic already locked.");
		ph_assert2(sm.format < NUM_FORMATS, "VertexBuffer does not use this semantic.");
		if(!sm.locked && sm.format < NUM_FORMATS)
		{
			if(!m_lockedBuffer && !m_numSemanticLocks)
			{
				m_lockedBuffer = lock();
			}
			ph_assert2(m_lockedBuffer, "Unable to lock VertexBuffer!");
			if(m_lockedBuffer)
			{
				m_numSemanticLocks++;
				sm.locked        = true;
				semanticBuffer   = ((uint8*)m_lockedBuffer) + sm.offset;
				stride           = m_stride;
			}
		}
	}
	if(semanticBuffer && semantic == SEMANTIC_COLOR)
	{
		swizzleColor(semanticBuffer, stride, m_maxVertices);
	}
	return semanticBuffer;
}

void RendererVertexBuffer::unlockSemantic(Semantic semantic)
{
	ph_assert2(semantic < NUM_SEMANTICS, "Invalid VertexBuffer Semantic!");
	if(semantic < NUM_SEMANTICS)
	{
		SemanticDesc &sm = m_semanticDescs[semantic];
		ph_assert2(m_lockedBuffer && m_numSemanticLocks && sm.locked, "Trying to unlock a semantic that was not locked.");
		if(m_lockedBuffer && m_numSemanticLocks && sm.locked)
		{
			if(m_lockedBuffer && semantic == SEMANTIC_COLOR)
			{
				swizzleColor(((uint8*)m_lockedBuffer)+sm.offset, m_stride, m_maxVertices);
			}
			sm.locked = false;
			m_numSemanticLocks--;
		}
		if(m_lockedBuffer && m_numSemanticLocks == 0 && m_deferredUnlock == false)
		{
			unlock();
			m_lockedBuffer = 0;
		}
	}
}

void RendererVertexBuffer::prepareForRender(void)
{
	if(m_lockedBuffer && m_numSemanticLocks == 0)
	{
		unlock();
		m_lockedBuffer = 0;
	}
	ph_assert2(m_lockedBuffer==0, "Vertex Buffer locked during usage!");
}

_NAMESPACE_END