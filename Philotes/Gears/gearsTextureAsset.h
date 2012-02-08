
#pragma once

#include "gearsAsset.h"

_NAMESPACE_BEGIN

class GearTextureAsset : public GearAsset
{
	friend class GearAssetManager;

public:
	enum Type
	{
		DDS,
		TGA,
		TEXTURE_FILE_TYPE_COUNT,
	};
protected:
	GearTextureAsset(Renderer &renderer, FILE &file, const char *path, Type texType);
	virtual ~GearTextureAsset(void);

public:
	RendererTexture2D *getTexture(void);

public:
	virtual bool isOk(void) const;

private:
	void loadDDS(Renderer &renderer, FILE &file);
	void loadTGA(Renderer &renderer, FILE &file);

	RendererTexture2D *m_texture;
};


_NAMESPACE_END