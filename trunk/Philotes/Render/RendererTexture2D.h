
#ifndef RENDERER_TEXTURE_2D_H
#define RENDERER_TEXTURE_2D_H

#include <RendererConfig.h>

class RendererTexture2DDesc;

class RendererTexture2D
{
	public:
		typedef enum Format
		{
			FORMAT_B8G8R8A8 = 0,
			FORMAT_A8,
			FORMAT_R32F,
			
			FORMAT_DXT1,
			FORMAT_DXT3,
			FORMAT_DXT5,
			
			FORMAT_D16,
			FORMAT_D24S8,
			
			NUM_FORMATS
		};
		
		typedef enum Filter
		{
			FILTER_NEAREST = 0,
			FILTER_LINEAR,
			FILTER_ANISOTROPIC,
			
			NUM_FILTERS
		};
		
		typedef enum Addressing
		{
			ADDRESSING_WRAP = 0,
			ADDRESSING_CLAMP,
			ADDRESSING_MIRROR,
			
			NUM_ADDRESSING
		};
	
	public:
		static uint32 computeImageByteSize(uint32 width, uint32 height, Format format);
		static uint32 getLevelDimension(uint32 dimension, uint32 level);
		static bool  isCompressedFormat(Format format);
		static bool  isDepthStencilFormat(Format format);
		static uint32 getFormatNumBlocks(uint32 dimension, Format format);
		static uint32 getFormatBlockSize(Format format);
		
	protected:
		RendererTexture2D(const RendererTexture2DDesc &desc);
		virtual ~RendererTexture2D(void);
	
	public:
		void release(void) { delete this; }
		
		Format     getFormat(void)      const { return m_format; }
		Filter     getFilter(void)      const { return m_filter; }
		Addressing getAddressingU(void) const { return m_addressingU; }
		Addressing getAddressingV(void) const { return m_addressingV; }
		uint32      getWidth(void)       const { return m_width; }
		uint32      getHeight(void)      const { return m_height; }
		uint32      getNumLevels(void)   const { return m_numLevels; }
		
		uint32      getWidthInBlocks(void)  const;
		uint32      getHeightInBlocks(void) const;
		uint32      getBlockSize(void)      const;
		
	public:
		//! pitch is the number of bytes between the start of each row.
		virtual void *lockLevel(uint32 level, uint32 &pitch) = 0;
		virtual void  unlockLevel(uint32 level) = 0;
		virtual	void	select(uint32 stageIndex)	= 0;

	private:
		RendererTexture2D &operator=(const RendererTexture2D&) { return *this; }
		
	private:
		Format     m_format;
		Filter     m_filter;
		Addressing m_addressingU;
		Addressing m_addressingV;
		uint32      m_width;
		uint32      m_height;
		uint32      m_numLevels;
};

#endif
