
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
	GearTextureAsset(Render &renderer, FILE &file, const char *path, Type texType);
	virtual ~GearTextureAsset(void);

public:
	RenderTexture2D *getTexture(void);

public:
	virtual bool isOk(void) const;

private:
	void loadDDS(Render &renderer, FILE &file);
	void loadTGA(Render &renderer, FILE &file);

	RenderTexture2D *m_texture;
};


_NAMESPACE_END