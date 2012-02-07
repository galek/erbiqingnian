
#include <RendererIndexBuffer.h>
#include <RendererIndexBufferDesc.h>

uint32 RendererIndexBuffer::getFormatByteSize(Format format)
{
	uint32 size = 0;
	switch(format)
	{
		case FORMAT_UINT16: size = sizeof(uint16); break;
		case FORMAT_UINT32: size = sizeof(uint32); break;
	}
	ph_assert2(size, "Unable to determine size of format.");
	return size;
}
	
RendererIndexBuffer::RendererIndexBuffer(const RendererIndexBufferDesc &desc) :
	m_hint(desc.hint),
	m_format(desc.format)
{
	m_maxIndices = 0;
}

RendererIndexBuffer::~RendererIndexBuffer(void)
{
	
}
		
RendererIndexBuffer::Hint RendererIndexBuffer::getHint(void) const
{
	return m_hint;
}

RendererIndexBuffer::Format RendererIndexBuffer::getFormat(void) const
{
	return m_format;
}

uint32 RendererIndexBuffer::getMaxIndices(void) const
{
	return m_maxIndices;
}
