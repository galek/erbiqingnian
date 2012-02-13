
#include "gearsAsset.h"
#include "gearsAssetManager.h"
#include "gearsTextureAsset.h"
#include "gearsMaterialAsset.h"

#include "render.h"
#include "renderConfig.h"

#define RAPIDXML_NO_EXCEPTIONS
#include <rapidxml.hpp>

namespace rapidxml
{
	void parse_error_handler(const char * /*what*/, void * /*where*/)
	{
		ph_assert(0);
	}
}

_NAMESPACE_BEGIN

_IMPLEMENT_SINGLETON(GearAssetManager);

GearAssetManager::GearAssetManager(Render &renderer) :
m_renderer(renderer)
{
}

GearAssetManager::~GearAssetManager(void)
{
	ph_assert(m_assets.size() == 0);
	clearSearchPaths();
}

GearAsset *GearAssetManager::getAsset(const char *path, GearAsset::Type type)
{	
	GearAsset *asset = findAsset(path);
	if(!asset)
	{
		asset = loadAsset(path);
	}
	if(asset && asset->getType() != type)
	{
		releaseAsset(*asset);
		asset = 0;
	}
	if(asset)
	{
		asset->m_numUsers++;
	}
	return asset;
}

void GearAssetManager::returnAsset(GearAsset &asset)
{
	ph_assert(asset.m_numUsers > 0);
	if(asset.m_numUsers > 0)
	{
		asset.m_numUsers--;
	}
	if(asset.m_numUsers == 0)
	{
		releaseAsset(asset);
	}
}

void GearAssetManager::addSearchPath(const char *path)
{
	const uint32 len = path && *path ? (uint32)strlen(path) : 0;
	if(len)
	{
		const uint32 len2 = len+2;
		char *searchPath = new char[len2];
		strncpy_s(searchPath, len2, path, len2);
		if(path[len-1] != '/' && path[len-1] != '\\')
		{
			strncat_s(searchPath, len2, "/", len2);
		}
		m_searchPaths.push_back(searchPath);
	}
}

void GearAssetManager::clearSearchPaths()
{
	const uint32 numSearchPaths = (uint32)m_searchPaths.size();
	for(uint32 i=0; i<numSearchPaths; i++)
	{
		delete [] m_searchPaths[i];
	}
	m_searchPaths.clear();
}

GearAsset *GearAssetManager::findAsset(const char *path)
{
	GearAsset *asset = 0;
	uint32 numAssets = (uint32)m_assets.size();
	for(uint32 i=0; i<numAssets; i++)
	{
		if(!strcmp(m_assets[i]->getPath(), path))
		{
			asset = m_assets[i];
			break;
		}
	}
	return asset;
}

static const char *strext(const char *str)
{
	const char *ext = str;
	while(str)
	{
		str = strchr(str, '.');
		if(str)
		{
			str++;
			ext = str;
		}
	}
	return ext;
}

GearAsset *GearAssetManager::loadAsset(const char *path)
{
	GearAsset *asset = 0;
	const char *extension = strext(path);
	if(extension && *extension)
	{
		FILE *file = 0;
		const uint32 numSearchPaths = (uint32)m_searchPaths.size();
		for(uint32 i=0; i<numSearchPaths; i++)
		{
			const char *prefix = m_searchPaths[i];
			char fullPath[512];
			strncpy_s(fullPath, 512, prefix, 512);
			strncat_s(fullPath, 512, path,   512);
			fopen_s(&file, fullPath, "rb");
			if(file) break;
		}

		if(!file)
			fopen_s(&file, path, "rb");

		ph_assert(file);

		// printf("Path: %s   (%s)\n", path, extension);

		if(file)
		{
			if(!strcmp(extension, "xml"))      asset = loadXMLAsset(*file, path);
			else if(!strcmp(extension, "dds")) asset = loadTextureAsset(*file, path, GearTextureAsset::DDS);
			else if(!strcmp(extension, "tga")) asset = loadTextureAsset(*file, path, GearTextureAsset::TGA);

			fclose(file);
		}
		else
		{
#define SAM_DEFAULT_MATERIAL "materials/simple_lit.xml"
#define SAM_DEFAULT_TEXTURE "textures/test.dds"

			// report the missing asset
			char msg[1024];

			if( !strcmp(extension, "xml") && strcmp(path, SAM_DEFAULT_MATERIAL) )  // Avoid infinite recursion
			{
				sprintf_s(msg, sizeof(msg), "Could not find material: %s, loading default material: %s", 
					path, SAM_DEFAULT_MATERIAL);
				RENDERER_OUTPUT_MESSAGE(&m_renderer, msg);

				return loadAsset(SAM_DEFAULT_MATERIAL);  // Try to use the default asset
			}
			else if(!strcmp(extension, "dds"))
			{
				sprintf_s(msg, sizeof(msg), "Could not find texture: %s, loading default texture: %s", 
					path, SAM_DEFAULT_TEXTURE);
				RENDERER_OUTPUT_MESSAGE(&m_renderer, msg);

				return loadAsset(SAM_DEFAULT_TEXTURE);  // Try to use the default asset
			}
		}
	}
	//ph_assert(asset && asset->isOk());
	if(asset && !asset->isOk())
	{
		delete asset;
		asset = 0;
	}
	if(asset)
	{
		m_assets.push_back(asset);
	}
	return asset;
}

void GearAssetManager::releaseAsset(GearAsset &asset)
{
	uint32 numAssets = (uint32)m_assets.size();
	uint32 found     = numAssets;
	for(uint32 i=0; i<numAssets; i++)
	{
		if(&asset == m_assets[i])
		{
			found = i;
			break;
		}
	}
	ph_assert(found < numAssets);
	if(found < numAssets)
	{
		m_assets[found] = m_assets.back();
		m_assets.pop_back();
		asset.release();
	}
}

GearAsset *GearAssetManager::loadXMLAsset(FILE &file, const char *path)
{
	GearAsset *asset = 0;
	fseek(&file, 0, SEEK_END);
	size_t filelen = ftell(&file);
	fseek(&file, 0, SEEK_SET);
	char *filedata = new char[filelen+1];
	filelen = fread(filedata, 1, filelen, &file);
	filedata[filelen] = 0;
	if(filedata && *filedata)
	{
		rapidxml::xml_document<char>* xmldoc = new rapidxml::xml_document<char>;
		xmldoc->parse<0>(filedata);
		rapidxml::xml_node<char> *rootnode = xmldoc->first_node();
		ph_assert(rootnode);
		if(rootnode)
		{
			const char *name = rootnode->name();
			if(!strcmp(name, "material"))
			{
				asset = new GearMaterialAsset(*this, *rootnode, path);
			}
		}
		delete xmldoc;
	}
	delete [] filedata;
	return asset;
}

GearAsset *GearAssetManager::loadTextureAsset(FILE &file, const char *path, GearTextureAsset::Type texType)
{
	GearTextureAsset *asset = 0;
	asset = new GearTextureAsset(getRender(), file, path, texType);
	return asset;
}

bool GearAssetManager::searchForPath( const char* path, char* buffer, int bufferSize, int maxRecursion )
{
	char* tmpBuffer = (char*)alloca(bufferSize);
	strcpy_s(buffer, bufferSize, path);
	for(int i = 0; i < maxRecursion; i++)
	{
		if(GetFileAttributes(buffer) == INVALID_FILE_ATTRIBUTES)
		{
			sprintf_s(tmpBuffer, bufferSize, "../%s", buffer);
			strcpy_s(buffer, bufferSize, tmpBuffer);
		}
		else 
			return true;
	}
	return false;
}

_NAMESPACE_END