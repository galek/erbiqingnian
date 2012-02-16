
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
		ASSET_MESH,
		ASSET_SKELETON,

		NUM_TYPES
	};

protected:

	GearAsset(Type type, const String& path);

	virtual			~GearAsset(void);

	virtual void	release(void) { delete this; }

public:

	virtual bool	isOk(void) const = 0;

	Type			getType(void) const { return m_type; }

	const String&	getPath(void) const { return m_path; }

private:

	GearAsset &operator=(const GearAsset&) { return *this; }

private:

	const Type		m_type;

	String			m_path;

	uint32      	m_numUsers;
};

typedef Array<GearAsset*>	AssetArray;

_NAMESPACE_END