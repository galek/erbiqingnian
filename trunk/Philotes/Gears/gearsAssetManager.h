
#pragma once

#include "gearsTextureAsset.h"

_NAMESPACE_BEGIN

class GearAssetManager
{
	public:
		GearAssetManager(Render &renderer);
		~GearAssetManager(void);
		
		Render    &getRender(void) { return m_renderer; }
		
		GearAsset *getAsset(const char *path, GearAsset::Type type);
		void         returnAsset(GearAsset &asset);
		
		void         addSearchPath(const char *path);
		void         clearSearchPaths(void);
		FILE*		 findFile(const char* path);
		const char*	 findPath(const char* path);

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
		static bool searchForPath(const char* path, char* buffer, int bufferSize, int maxRecursion);
	
	protected:
		GearAsset *findAsset(const char *path);
		GearAsset *loadAsset(const char *path);
		void         releaseAsset(GearAsset &asset);
		
		GearAsset *loadXMLAsset(FILE &file, const char *path);
		GearAsset *loadTextureAsset(FILE &file, const char *path, GearTextureAsset::Type texType);
	
	private:
		GearAssetManager &operator=(const GearAssetManager&) { return *this; }
		
	protected:
		Render&	m_renderer;
		std::vector<char *>    m_searchPaths;
		std::vector<GearAsset*>	m_assets;
};

_NAMESPACE_END