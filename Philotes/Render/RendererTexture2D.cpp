
#include <RendererTexture2D.h>
#include <RendererTexture2DDesc.h>

static uint32 computeCompressedDimension(uint32 dimension)
{
	if(dimension < 4)
	{
		dimension = 4;
	}
	else
	{
		uint32 mod = dimension % 4;
		if(mod) dimension += 4 - mod;
	}
	return dimension;
}

uint32 RendererTexture2D::computeImageByteSize(uint32 width, uint32 height, RendererTexture2D::Format format)
{
	uint32 size = 0;
	uint32 numPixels = width * height;
	switch(format)
	{
		case RendererTexture2D::FORMAT_B8G8R8A8:
			size = numPixels * sizeof(uint8) * 4;
			break;
		case RendererTexture2D::FORMAT_A8:
			size = numPixels * sizeof(uint8) * 1;
			break;
		case RendererTexture2D::FORMAT_R32F:
			size = numPixels * sizeof(float);
			break;
		case RendererTexture2D::FORMAT_DXT1:
			width  = computeCompressedDimension(width);
			height = computeCompressedDimension(height);
			size   = computeImageByteSize(width, height, RendererTexture2D::FORMAT_B8G8R8A8) / 8;
			break;
		case RendererTexture2D::FORMAT_DXT3:
		case RendererTexture2D::FORMAT_DXT5:
			width  = computeCompressedDimension(width);
			height = computeCompressedDimension(height);
			size   = computeImageByteSize(width, height, RendererTexture2D::FORMAT_B8G8R8A8) / 4;
			break;
	}
	ph_assert2(size, "Unable to compute Image Size.");
	return size;
}

uint32 RendererTexture2D::getLevelDimension(uint32 dimension, uint32 level)
{
	dimension >>=level;
	if(!dimension) dimension=1;
	return dimension;
}

bool RendererTexture2D::isCompressedFormat(Format format)
{
	return (format >= FORMAT_DXT1 && format <= FORMAT_DXT5);
}

bool RendererTexture2D::isDepthStencilFormat(Format format)
{
	return (format >= FORMAT_D16 && format <= FORMAT_D24S8);
}

uint32 RendererTexture2D::getFormatNumBlocks(uint32 dimension, Format format)
{
	uint32 blockDimension = 0;
	if(isCompressedFormat(format))
	{
		blockDimension = dimension / 4;
		if(dimension % 4) blockDimension++;
	}
	else
	{
		blockDimension = dimension;
	}
	return blockDimension;
}

uint32 RendererTexture2D::getFormatBlockSize(Format format)
{
	return computeImageByteSize(1, 1, format);
}

RendererTexture2D::RendererTexture2D(const RendererTexture2DDesc &desc)
{
	m_format      = desc.format;
	m_filter      = desc.filter;
	m_addressingU = desc.addressingU;
	m_addressingV = desc.addressingV;
	m_width       = desc.width;
	m_height      = desc.height;
	m_numLevels   = desc.numLevels;
}

RendererTexture2D::~RendererTexture2D(void)
{
	
}

uint32 RendererTexture2D::getWidthInBlocks(void) const
{
	return getFormatNumBlocks(getWidth(), getFormat());
}

uint32 RendererTexture2D::getHeightInBlocks(void) const
{
	return getFormatNumBlocks(getHeight(), getFormat());
}

uint32 RendererTexture2D::getBlockSize(void) const
{
	return getFormatBlockSize(getFormat());
}
