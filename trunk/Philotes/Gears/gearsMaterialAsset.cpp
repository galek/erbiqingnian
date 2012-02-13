
#include "gearsMaterialAsset.h"

#include "render.h"
#include "renderMaterial.h"
#include "renderMaterialDesc.h"
#include "renderMaterialInstance.h"

#include "gearsTextureAsset.h"
#include "gearsAssetManager.h"

#define RAPIDXML_NO_EXCEPTIONS
#include <rapidxml.hpp>
#include <ctype.h>

_NAMESPACE_BEGIN

static const char *getXMLAttribute(rapidxml::xml_node<char> &node, const char *attr)
{
	const char *value = 0;
	rapidxml::xml_attribute<char> *nameattr = node.first_attribute(attr);
	if(nameattr)
	{
		value = nameattr->value();
	}
	return value;
}

static void readFloats(const char *str, float *floats, unsigned int numFloats)
{
	uint32 fcount = 0;
	while(*str && !((*str>='0'&&*str<='9') || *str=='.')) str++;
	for(uint32 i=0; i<numFloats; i++)
	{
		if(*str)
		{
			floats[i] = (float)atof(str);
			while(*str &&  ((*str>='0'&&*str<='9') || *str=='.')) str++;
			while(*str && !((*str>='0'&&*str<='9') || *str=='.')) str++;
			fcount++;
		}
	}
	ph_assert(fcount==numFloats);
}

//////////////////////////////////////////////////////////////////////////

GearMaterialAsset::GearMaterialAsset(GearAssetManager &assetManager, rapidxml::xml_node<char> &xmlroot, const char *path) :
	GearAsset(ASSET_MATERIAL, path),
m_assetManager(assetManager)
{

	std::vector<const char*> mVertexShaderPaths;

	Render &renderer = assetManager.getRender();

	RenderMaterialDesc matdesc;
	const char *materialTypeName = getXMLAttribute(xmlroot, "type");
	if(materialTypeName && !strcmp(materialTypeName, "lit"))
	{
		matdesc.type = RenderMaterial::TYPE_LIT;
	}
	else if(materialTypeName && !strcmp(materialTypeName, "unlit"))
	{
		matdesc.type = RenderMaterial::TYPE_UNLIT;
	}
	for(rapidxml::xml_node<char> *child=xmlroot.first_node(); child; child=child->next_sibling())
	{
		const char *nodeName  = child->name();
		const char *nodeValue = child->value();
		const char *name      = getXMLAttribute(*child, "name");
		if(nodeName && nodeValue)
		{
			while(isspace(*nodeValue)) nodeValue++; // skip leading spaces.
			if(!strcmp(nodeName, "shader"))
			{
				if(name && !strcmp(name, "vertex"))
				{
					//matdesc.vertexShaderPath = nodeValue;
					mVertexShaderPaths.push_back(nodeValue);
				}
				else if(name && !strcmp(name, "fragment"))
				{
					matdesc.fragmentShaderPath = nodeValue;
				}
			}
			else if(!strcmp(nodeName, "alphaTestFunc"))
			{
				if(     !strcmp(nodeValue, "EQUAL"))         matdesc.alphaTestFunc = RenderMaterial::ALPHA_TEST_EQUAL;
				else if(!strcmp(nodeValue, "NOT_EQUAL"))     matdesc.alphaTestFunc = RenderMaterial::ALPHA_TEST_NOT_EQUAL;
				else if(!strcmp(nodeValue, "LESS"))          matdesc.alphaTestFunc = RenderMaterial::ALPHA_TEST_LESS;
				else if(!strcmp(nodeValue, "LESS_EQUAL"))    matdesc.alphaTestFunc = RenderMaterial::ALPHA_TEST_LESS_EQUAL;
				else if(!strcmp(nodeValue, "GREATER"))       matdesc.alphaTestFunc = RenderMaterial::ALPHA_TEST_GREATER;
				else if(!strcmp(nodeValue, "GREATER_EQUAL")) matdesc.alphaTestFunc = RenderMaterial::ALPHA_TEST_GREATER_EQUAL;
				else ph_assert(0); // Unknown alpha test func!
			}
			else if(!strcmp(nodeName, "alphaTestRef"))
			{
				matdesc.alphaTestRef = Math::Clamp((float)atof(nodeValue), 0.0f, 1.0f);
			}
			else if(!strcmp(nodeName, "blending") && strstr(nodeValue, "true"))
			{
				matdesc.blending = true;
				// HACK/TODO: read these values from disk!
				matdesc.srcBlendFunc = RenderMaterial::BLEND_SRC_ALPHA;
				matdesc.dstBlendFunc = RenderMaterial::BLEND_ONE_MINUS_SRC_ALPHA;
			}
		}
	}

	for (size_t materialIndex = 0; materialIndex < mVertexShaderPaths.size(); materialIndex++)
	{
		MaterialStruct materialStruct;

		matdesc.vertexShaderPath = mVertexShaderPaths[materialIndex];
		materialStruct.m_material = NULL;
		materialStruct.m_materialInstance = NULL;
		materialStruct.m_maxBones = 0;
		if (strstr(mVertexShaderPaths[materialIndex], "skeletalmesh") != NULL)
			materialStruct.m_maxBones = RENDERER_MAX_BONES;

		materialStruct.m_material = renderer.createMaterial(matdesc);
		ph_assert(materialStruct.m_material);
		if(materialStruct.m_material)
		{
			rapidxml::xml_node<char> *varsnode = xmlroot.first_node("variables");
			if(varsnode)
			{
				materialStruct.m_materialInstance = new RenderMaterialInstance(*materialStruct.m_material);
				for(rapidxml::xml_node<char> *child=varsnode->first_node(); child; child=child->next_sibling())
				{
					const char *nodename = child->name();
					const char *varname  = getXMLAttribute(*child, "name");
					const char *value    = child->value();

					if(!strcmp(nodename, "float"))
					{
						float f = (float)atof(value);
						const RenderMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, RenderMaterial::VARIABLE_FLOAT);
						ph_assert(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, &f);
					}
					else if(!strcmp(nodename, "float2"))
					{
						float f[2];
						readFloats(value, f, 2);
						const RenderMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, RenderMaterial::VARIABLE_FLOAT2);
						ph_assert(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, f);
					}
					else if(!strcmp(nodename, "float3"))
					{
						float f[3];
						readFloats(value, f, 3);
						const RenderMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, RenderMaterial::VARIABLE_FLOAT3);
						ph_assert(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, f);
					}
					else if(!strcmp(nodename, "float4"))
					{
						float f[4];
						readFloats(value, f, 4);
						const RenderMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, RenderMaterial::VARIABLE_FLOAT4);
						ph_assert(var);
						if(var) materialStruct.m_materialInstance->writeData(*var, f);
					}
					else if(!strcmp(nodename, "sampler2D"))
					{
						GearTextureAsset *textureAsset = static_cast<GearTextureAsset*>(assetManager.getAsset(value, ASSET_TEXTURE));
						ph_assert(textureAsset);
						if(textureAsset)
						{
							m_assets.Append(textureAsset);
							const RenderMaterial::Variable *var = materialStruct.m_materialInstance->findVariable(varname, RenderMaterial::VARIABLE_SAMPLER2D);
							ph_assert(var);
							if(var)
							{
								RenderTexture2D *texture = textureAsset->getTexture();
								materialStruct.m_materialInstance->writeData(*var, &texture);
							}
						}
					}

				}
			}

			m_vertexShaders.Append(materialStruct);
		}
	}
}

GearMaterialAsset::~GearMaterialAsset(void)
{
	uint32 numAssets = (uint32)m_assets.Size();
	for(uint32 i=0; i<numAssets; i++)
	{
		m_assetManager.returnAsset(*m_assets[i]);
	}
	for (SizeT index = 0; index < m_vertexShaders.Size(); index++)
	{
		if(m_vertexShaders[index].m_materialInstance) delete m_vertexShaders[index].m_materialInstance;
		if(m_vertexShaders[index].m_material)         m_vertexShaders[index].m_material->release();
	}
}

size_t GearMaterialAsset::getNumVertexShaders() const
{
	return m_vertexShaders.Size();
}

RenderMaterial *GearMaterialAsset::getMaterial(size_t vertexShaderIndex)
{
	return m_vertexShaders[vertexShaderIndex].m_material;
}

RenderMaterialInstance *GearMaterialAsset::getMaterialInstance(size_t vertexShaderIndex)
{
	return m_vertexShaders[vertexShaderIndex].m_materialInstance;
}

bool GearMaterialAsset::isOk(void) const
{
	return !m_vertexShaders.IsEmpty();
}

unsigned int GearMaterialAsset::getMaxBones(size_t vertexShaderIndex) const
{
	return m_vertexShaders[vertexShaderIndex].m_maxBones;
}

GearMaterialAsset* GearMaterialAsset::getPrefabAsset( PrefabMaterial type )
{
	switch (type)
	{
	case PM_LIGHT:
		{
			return static_cast<GearMaterialAsset*>(GearAssetManager::getSingleton(
				)->getAsset("materials/simple_lit.xml", GearAsset::ASSET_MATERIAL));
			break;
		}

	case PM_LIGHT_COLOR:
		{
			return static_cast<GearMaterialAsset*>(GearAssetManager::getSingleton(
				)->getAsset("materials/simple_lit_color.xml", GearAsset::ASSET_MATERIAL));
			break;
		}

	case PM_UNLIGHT:
		{
			return static_cast<GearMaterialAsset*>(GearAssetManager::getSingleton(
				)->getAsset("materials/simple_unlit.xml", GearAsset::ASSET_MATERIAL));
			break;
		}
	default:
		return NULL;
	}
}

_NAMESPACE_END