
#pragma once

#include "gearsTextureAsset.h"
#include "core/singleton.h"

_NAMESPACE_BEGIN

class GearAssetManager : public Singleton<GearAssetManager>
{
	public:
		
		GearAssetManager();

		virtual ~GearAssetManager(void);

		_DECLARE_SINGLETON(GearAssetManager);
		
		GearAsset*		getAsset(const String& path, GearAsset::Type type);

		void			returnAsset(GearAsset &asset);
		
		void         	addSearchPath(const String& path);

		void         	clearSearchPaths(void);

		FILE*		 	findFile(const String& path);

		const String&	findPath(const String& path);

		/**
		Search for the speficied path in the current directory. If not found, move up in the folder hierarchy
		until the path can be found or until the specified maximum number of steps is reached.

		\note On consoles no recursive search will be done

		\param	[in] path The path to look for
		\param	[out] buffer Buffer to store the (potentially) adjusted path
		\param	[in] bufferSize Size of buffer
		\param	[in] maxRecursion Maximum number steps to move up in the folder hierarchy
		return	true if path was found
		*/
		static bool 	searchForPath(const char* path, char* buffer, int bufferSize, int maxRecursion);
	
	protected:
		GearAsset*		findAsset(const String& path);

		GearAsset*		loadAsset(const String& path);

		void			releaseAsset(GearAsset &asset);
		
		GearAsset*		loadXMLAsset(FILE &file, const String& path);

		GearAsset*		loadTextureAsset(FILE &file, const String& path, GearTextureAsset::Type texType);
	
	private:
		GearAssetManager &operator=(const GearAssetManager&) { return *this; }
		
	protected:

		StringArray		m_searchPaths;

		AssetArray		m_assets;
};

_NAMESPACE_END