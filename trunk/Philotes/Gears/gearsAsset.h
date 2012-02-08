
#pragma once

_NAMESPACE_BEGIN

class GearAsset
{
	friend class GearAssetManager;
public:
	typedef enum Type
	{
		ASSET_MATERIAL = 0,
		ASSET_TEXTURE,

		NUM_TYPES
	};

protected:
	GearAsset(Type type, const char *path);
	virtual ~GearAsset(void);

	virtual void release(void) { delete this; }

public:
	virtual bool isOk(void) const = 0;

public:
	Type        getType(void) const { return m_type; }
	const char *getPath(void) const { return m_path; }

private:
	GearAsset &operator=(const GearAsset&) { return *this; }

private:
	const Type	m_type;
	char*		m_path;
	uint32      m_numUsers;
};

_NAMESPACE_END