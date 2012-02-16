
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
	void parse_error_handler(const char * what, void * /*where*/)
	{
		ph_error("xml error : %s",what);
	}
}

_NAMESPACE_BEGIN

_IMPLEMENT_SINGLETON(GearAssetManager);

GearAssetManager::GearAssetManager()
{
}

GearAssetManager::~GearAssetManager(void)
{
	ph_assert(m_assets.Size() == 0);
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

void GearAssetManager::addSearchPath(const String& path)
{
	const uint32 len = path.Length();
	if(len)
	{
		const uint32 len2 = len+2;
		char *searchPath = new char[len2];
		strncpy_s(searchPath, len2, path.c_str(), len2);
		if(path[len-1] != '/' && path[len-1] != '\\')
		{
			strncat_s(searchPath, len2, "/", len2);
		}
		m_searchPaths.Append(searchPath);
	}
}

void GearAssetManager::clearSearchPaths()
{
	const uint32 numSearchPaths = (uint32)m_searchPaths.Size();
	m_searchPaths.Reset();
}

GearAsset *GearAssetManager::findAsset(const String& path)
{
	GearAsset *asset = 0;
	uint32 numAssets = (uint32)m_assets.Size();
	for(uint32 i=0; i<numAssets; i++)
	{
		if(m_assets[i]->getPath() != path)
		{
			asset = m_assets[i];
			break;
		}
	}
	return asset;
}

GearAsset *GearAssetManager::loadAsset(const String& path)
{
	GearAsset *asset = 0;
	const String extension = path.GetFileExtension();

	if(!extension.IsEmpty())
	{
		FILE *file = 0;
		const uint32 numSearchPaths = (uint32)m_searchPaths.Size();
		for(uint32 i=0; i<numSearchPaths; i++)
		{
			const char *prefix = m_searchPaths[i].c_str();
			char fullPath[512];
			strncpy_s(fullPath, 512, prefix, 512);
			strncat_s(fullPath, 512, path.c_str(),   512);
			fopen_s(&file, fullPath, "rb");
			if(file) break;
		}

		if(!file)
			fopen_s(&file, path.c_str(), "rb");

		ph_assert(file);

		if(file)
		{
			if(extension == "xml")      asset = loadXMLAsset(*file, path);
			else if(extension == "dds") asset = loadTextureAsset(*file, path, GearTextureAsset::DDS);
			else if(extension == "tga") asset = loadTextureAsset(*file, path, GearTextureAsset::TGA);

			fclose(file);
		}
		else
		{

#define SAM_DEFAULT_MATERIAL "materials/simple_lit.xml"
#define SAM_DEFAULT_TEXTURE "textures/test.dds"

			char msg[1024];

			if( !strcmp(extension.c_str(), "xml") && strcmp(path.c_str(), SAM_DEFAULT_MATERIAL) )  // Avoid infinite recursion
			{
				sprintf_s(msg, sizeof(msg), "Could not find material: %s, loading default material: %s", 
					path, SAM_DEFAULT_MATERIAL);
				RENDERER_OUTPUT_MESSAGE(&m_renderer, msg);

				return loadAsset(SAM_DEFAULT_MATERIAL);  // Try to use the default asset
			}
			else if(!strcmp(extension.c_str(), "dds"))
			{
				sprintf_s(msg, sizeof(msg), "Could not find texture: %s, loading default texture: %s", 
					path, SAM_DEFAULT_TEXTURE);
				RENDERER_OUTPUT_MESSAGE(&m_renderer, msg);

				return loadAsset(SAM_DEFAULT_TEXTURE);  // Try to use the default asset
			}
		}
	}
	
	if(asset && !asset->isOk())
	{
		delete asset;
		asset = 0;
	}
	if(asset)
	{
		m_assets.Append(asset);
	}
	return asset;
}

void GearAssetManager::releaseAsset(GearAsset &asset)
{
	uint32 numAssets = (uint32)m_assets.Size();
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
		m_assets[found] = m_assets.Back();
		m_assets.PopBack();
		asset.release();
	}
}

GearAsset *GearAssetManager::loadXMLAsset(FILE &file, const String& path)
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

GearAsset *GearAssetManager::loadTextureAsset(FILE &file, const String& path, GearTextureAsset::Type texType)
{
	GearTextureAsset *asset = 0;
	asset = new GearTextureAsset(file, path, texType);
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