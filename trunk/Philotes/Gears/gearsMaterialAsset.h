#pragma once

#include "gearsAsset.h"

namespace rapidxml
{
	template<typename Ch> class xml_node;
}

_NAMESPACE_BEGIN

class GearMaterialAsset : public GearAsset
{
	friend class GearAssetManager;

protected:

	GearMaterialAsset(GearAssetManager &assetManager, rapidxml::xml_node<char> &xmlroot, const char *path);

	virtual ~GearMaterialAsset(void);

public:
	size_t						getNumVertexShaders() const;
	RenderMaterial*			getMaterial(size_t vertexShaderIndex = 0);
	RenderMaterialInstance*	getMaterialInstance(size_t vertexShaderIndex = 0);
	unsigned int				getMaxBones(size_t vertexShaderIndex) const;

public:
	virtual bool				isOk(void) const;

private:
	GearAssetManager			&m_assetManager;

	struct MaterialStruct
	{
		RenderMaterial         *m_material;
		RenderMaterialInstance *m_materialInstance;
		unsigned int              m_maxBones;
	};

	std::vector<MaterialStruct> m_vertexShaders;

	std::vector<GearAsset*> m_assets;
};

_NAMESPACE_END