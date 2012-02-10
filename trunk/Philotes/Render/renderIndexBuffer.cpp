
#include <renderIndexBuffer.h>
#include <renderIndexBufferDesc.h>

_NAMESPACE_BEGIN

uint32 RenderIndexBuffer::getFormatByteSize(Format format)
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
	
RenderIndexBuffer::RenderIndexBuffer(const RenderIndexBufferDesc &desc) :
	m_hint(desc.hint),
	m_format(desc.format)
{
	m_maxIndices = 0;
}

RenderIndexBuffer::~RenderIndexBuffer(void)
{
	
}
		
RenderIndexBuffer::Hint RenderIndexBuffer::getHint(void) const
{
	return m_hint;
}

RenderIndexBuffer::Format RenderIndexBuffer::getFormat(void) const
{
	return m_format;
}

uint32 RenderIndexBuffer::getMaxIndices(void) const
{
	return m_maxIndices;
}

_NAMESPACE_END