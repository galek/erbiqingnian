
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
	GearTextureAsset(FILE &file, const String& path, Type texType);

	virtual ~GearTextureAsset(void);

public:
	RenderTexture2D *getTexture(void);

public:
	virtual bool isOk(void) const;

private:

	void loadDDS(FILE &file);

	void loadTGA(FILE &file);

	RenderTexture2D *m_texture;
};


_NAMESPACE_END