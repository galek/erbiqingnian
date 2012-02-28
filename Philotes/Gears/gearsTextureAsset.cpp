
#include "gearsTextureAsset.h"
#include "gearsApplication.h"
#include "render.h"
#include "renderTexture2D.h"
#include "renderTexture2DDesc.h"

// 分别支持DDS和TGA格式
#include "nv_dds.h"
#include "targa.h"

_NAMESPACE_BEGIN


GearTextureAsset::GearTextureAsset(FILE &file, const String& path, Type texType) :
GearAsset(ASSET_TEXTURE, path)
{
	m_texture = 0;

	switch(texType)
	{
	case DDS: loadDDS(file); break;
	case TGA: loadTGA(file); break;
	default: ph_assert(0 && "Invalid texture type requested"); break;
	}
}

GearTextureAsset::~GearTextureAsset(void)
{
	if(m_texture) m_texture->release();
}

void GearTextureAsset::loadDDS(FILE &file) 
{
	nv_dds::CDDSImage ddsimage;
	bool ok = ddsimage.load(&file, false);
	ph_assert(ok);
	if(ok)
	{
		RenderTexture2DDesc tdesc;
		nv_dds::TextureFormat ddsformat = ddsimage.get_format();
		switch(ddsformat)
		{
		case nv_dds::TextureBGRA: tdesc.format = RenderTexture2D::FORMAT_B8G8R8A8;	break;
		case nv_dds::TextureDXT1: tdesc.format = RenderTexture2D::FORMAT_DXT1;		break;
		case nv_dds::TextureDXT3: tdesc.format = RenderTexture2D::FORMAT_DXT3;		break;
		case nv_dds::TextureDXT5: tdesc.format = RenderTexture2D::FORMAT_DXT5;		break;
		case nv_dds::TextureLuminance: tdesc.format = RenderTexture2D::FORMAT_L8;	break;
		}
		tdesc.width     = ddsimage.get_width();
		tdesc.height    = ddsimage.get_height();
		tdesc.numLevels = ddsimage.get_num_mipmaps()+1;

		ph_assert(tdesc.isValid());
		m_texture = GearApplication::getApp()->getRender()->createTexture2D(tdesc);
		ph_assert(m_texture);
		if(m_texture)
		{
			uint32 pitch  = 0;
			void *buffer = m_texture->lockLevel(0, pitch);
			ph_assert(buffer);
			if(buffer)
			{
				uint8       *levelDst    = (uint8*)buffer;
				const uint8 *levelSrc    = (uint8*)(unsigned char*)ddsimage;
				const uint32 levelWidth  = m_texture->getWidthInBlocks();
				const uint32 levelHeight = m_texture->getHeightInBlocks();
				const uint32 rowSrcSize  = levelWidth * m_texture->getBlockSize();
				ph_assert(rowSrcSize <= pitch);
				for(uint32 row=0; row<levelHeight; row++)
				{
					memcpy(levelDst, levelSrc, rowSrcSize);
					levelDst += pitch;
					levelSrc += rowSrcSize;
				}
			}
			m_texture->unlockLevel(0);

			for(uint32 i=1; i<tdesc.numLevels; i++)
			{
				void *buffer = m_texture->lockLevel(i, pitch);
				ph_assert(buffer);
				if(buffer && pitch )
				{
					const nv_dds::CSurface &surface = ddsimage.get_mipmap(i-1);
					uint8       *levelDst    = (uint8*)buffer;
					const uint8 *levelSrc    = (uint8*)(unsigned char*)surface;
					const uint32 levelWidth  = RenderTexture2D::getFormatNumBlocks(surface.get_width(),  m_texture->getFormat());
					const uint32 levelHeight = RenderTexture2D::getFormatNumBlocks(surface.get_height(), m_texture->getFormat());
					const uint32 rowSrcSize  = levelWidth * m_texture->getBlockSize();
					ph_assert(rowSrcSize <= pitch);
					for(uint32 row=0; row<levelHeight; row++)
					{
						memcpy(levelDst, levelSrc, rowSrcSize);
						levelDst += pitch;
						levelSrc += rowSrcSize;
					}
				}
				m_texture->unlockLevel(i);
			}
		}
	}
}

void GearTextureAsset::loadTGA(FILE &file)
{
#ifdef RENDERER_ENABLE_TGA_SUPPORT

	tga_image* image = new tga_image();
	bool ok = (TGA_NOERR == tga_read_from_FILE( image, &file ));

	// flip it to make it look correct in the SampleFramework's renderer
	tga_flip_vert( image );

	ph_assert(ok);
	if( ok )
	{
		RenderTexture2DDesc tdesc;
		int componentCount = image->pixel_depth/8;
		if( componentCount == 3 || componentCount == 4 )
		{
			tdesc.format = RenderTexture2D::FORMAT_B8G8R8A8;

			tdesc.width     = image->width;
			tdesc.height    = image->height;

			tdesc.numLevels = 1;
			ph_assert(tdesc.isValid());
			m_texture = GearApplication::getApp()->getRender()->createTexture2D(tdesc);
			ph_assert(m_texture);

			if(m_texture)
			{
				uint32 pitch  = 0;
				void *buffer = m_texture->lockLevel(0, pitch);
				ph_assert(buffer);
				if(buffer)
				{
					uint8       *levelDst    = (uint8*)buffer;
					const uint8 *levelSrc    = (uint8*)image->image_data;
					const uint32 levelWidth  = m_texture->getWidthInBlocks();
					const uint32 levelHeight = m_texture->getHeightInBlocks();
					const uint32 rowSrcSize  = levelWidth * m_texture->getBlockSize();
					ph_assert(rowSrcSize <= pitch); // the pitch can't be less than the source row size.
					for(uint32 row=0; row<levelHeight; row++)
					{ 
						if( componentCount == 3 )
						{
							// copy per pixel to handle RBG case, based on component count
							for(uint32 col=0; col<levelWidth; col++)
							{
								*levelDst++ = levelSrc[0];
								*levelDst++ = levelSrc[1];
								*levelDst++ = levelSrc[2];
								*levelDst++ = 0xFF; //alpha
								levelSrc += componentCount;
							}
						}
						else
						{
							memcpy(levelDst, levelSrc, rowSrcSize);
							levelDst += pitch;
							levelSrc += rowSrcSize;
						}
					}
				}
				m_texture->unlockLevel(0);
			}
		}
	}
	delete image;

#endif /* RENDERER_ENABLE_TGA_SUPPORT */
}

RenderTexture2D *GearTextureAsset::getTexture(void)
{
	return m_texture;
}

bool GearTextureAsset::isOk(void) const
{
	return m_texture ? true : false;
}


_NAMESPACE_END