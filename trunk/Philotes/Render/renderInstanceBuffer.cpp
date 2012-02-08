
#include <renderInstanceBuffer.h>
#include <renderInstanceBufferDesc.h>

_NAMESPACE_BEGIN

uint32 RendererInstanceBuffer::getFormatByteSize(Format format)
{
	uint32 size = 0;
	switch(format)
	{
		case FORMAT_FLOAT1: size = sizeof(float)*1; break;
		case FORMAT_FLOAT2: size = sizeof(float)*2; break;
		case FORMAT_FLOAT3: size = sizeof(float)*3; break;
		case FORMAT_FLOAT4: size = sizeof(float)*4; break;
	}
	ph_assert2(size, "Unable to determine size of Format.");
	return size;
}

RendererInstanceBuffer::RendererInstanceBuffer(const RendererInstanceBufferDesc &desc) :
	m_hint(desc.hint)
{
	m_maxInstances     = 0;
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

RendererInstanceBuffer::~RendererInstanceBuffer(void)
{
	ph_assert2(m_lockedBuffer==0 && m_numSemanticLocks==0, "InstanceBuffer had outstanding locks during destruction!");
}

RendererInstanceBuffer::Hint RendererInstanceBuffer::getHint(void) const
{
	return m_hint;
}

RendererInstanceBuffer::Format RendererInstanceBuffer::getFormatForSemantic(Semantic semantic) const
{
	return m_semanticDescs[semantic].format;
}

void *RendererInstanceBuffer::lockSemantic(Semantic semantic, uint32 &stride)
{
	void *semanticBuffer = 0;
	ph_assert2(semantic < NUM_SEMANTICS, "Invalid InstanceBuffer Semantic!");
	if(semantic < NUM_SEMANTICS)
	{
		SemanticDesc &sm = m_semanticDescs[semantic];
		ph_assert2(!sm.locked,              "InstanceBuffer Semantic already locked.");
		ph_assert2(sm.format < NUM_FORMATS, "InstanceBuffer does not use this semantic.");
		if(!sm.locked && sm.format < NUM_FORMATS)
		{
			if(!m_lockedBuffer && !m_numSemanticLocks)
			{
				m_lockedBuffer = lock();
			}
			ph_assert2(m_lockedBuffer, "Unable to lock InstanceBuffer!");
			if(m_lockedBuffer)
			{
				m_numSemanticLocks++;
				sm.locked        = true;
				semanticBuffer   = ((uint8*)m_lockedBuffer) + sm.offset;
				stride           = m_stride;
			}
		}
	}
	return semanticBuffer;
}

void RendererInstanceBuffer::unlockSemantic(Semantic semantic)
{
	ph_assert2(semantic < NUM_SEMANTICS, "Invalid InstanceBuffer Semantic!");
	if(semantic < NUM_SEMANTICS)
	{
		SemanticDesc &sm = m_semanticDescs[semantic];
		ph_assert2(m_lockedBuffer && m_numSemanticLocks && sm.locked, "Trying to unlock a semantic that was not locked.");
		if(m_lockedBuffer && m_numSemanticLocks && sm.locked)
		{
			sm.locked = false;
			m_numSemanticLocks--;
		}
		if(m_lockedBuffer && m_numSemanticLocks == 0)
		{
			unlock();
			m_lockedBuffer = 0;
		}
	}
}

_NAMESPACE_END