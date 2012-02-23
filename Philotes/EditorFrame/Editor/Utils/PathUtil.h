
#pragma once

#include <shlwapi.h>

namespace Path
{
	//! Convert a path to the uniform form.
	inline CString ToUnixPath( const CString& strPath )
	{
		CString str = strPath;
		str.Replace( '\\','/' );
		return str;
	}

	//! Split full file name to path and filename
	//! @param filepath [IN] Full file name inclusing path.
	//! @param path [OUT] Extracted file path.
	//! @param file [OUT] Extracted file (with extension).
	inline void Split( const CString &filepath,CString &path,CString &file )
	{
		char path_buffer[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath( filepath,drive,dir,fname,ext );
		_makepath( path_buffer,drive,dir,0,0 );
		path = path_buffer;
		_makepath( path_buffer,0,0,fname,ext );
		file = path_buffer;
	}

	//! Split full file name to path and filename
	//! @param filepath [IN] Full file name inclusing path.
	//! @param path [OUT] Extracted file path.
	//! @param filename [OUT] Extracted file (without extension).
	//! @param ext [OUT] Extracted files extension.
	inline void Split( const CString &filepath,CString &path,CString &filename,CString &fext )
	{
		char path_buffer[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath( filepath,drive,dir,fname,ext );
		_makepath( path_buffer,drive,dir,0,0 );
		path = path_buffer;
		filename = fname;
		fext = ext;
	}

	//! Extract extension from full specified file path.
	inline CString GetExt( const CString &filepath )
	{
		char ext[_MAX_EXT];
		_splitpath( filepath,0,0,0,ext );
		if (ext[0] == '.')
			return ext+1;
		
		return ext;
	}

	//! Extract path from full specified file path.
	inline CString GetPath( const CString &filepath )
	{
		char path_buffer[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		_splitpath( filepath,drive,dir,0,0 );
		_makepath( path_buffer,drive,dir,0,0 );
		return path_buffer;
	}

	//! Extract file name with extension from full specified file path.
	inline CString GetFile( const CString &filepath )
	{
		char path_buffer[_MAX_PATH];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		_splitpath( filepath,0,0,fname,ext );
		_makepath( path_buffer,0,0,fname,ext );
		return path_buffer;
	}

	//! Extract file name without extension from full specified file path.
	inline CString GetFileName( const CString &filepath )
	{
		char fname[_MAX_FNAME];
		_splitpath( filepath,0,0,fname,0 );
		return fname;
	}

	//! Removes the trailing backslash from a given path.
	inline CString RemoveBackslash( const CString &path )
	{
		if (path.IsEmpty())
			return path;

		int iLenMinus1 = path.GetLength()-1;
		char cLastChar = path[iLenMinus1];
		
		if(cLastChar=='\\' || cLastChar == '/')
			return path.Mid( 0,iLenMinus1 );

		return path;
	}

	//! add a backslash if needed
	inline CString AddBackslash( const CString &path )
	{
		if(path.IsEmpty() || path[path.GetLength()-1] == '\\')
			return path;

		return path + "\\";
	}
	//! add a slash if needed
	inline CString AddSlash( const CString &path )
	{
		if(path.IsEmpty() || path[path.GetLength()-1] == '/')
			return path;

		return path + "/";
	}

	//! add a slash if needed(no slash or backslash at the end of the path)
	inline CString AddPathSlash( const CString &path )
	{
		if((path.IsEmpty()) || (path[path.GetLength()-1] == '/') || (path[path.GetLength()-1] == '\\'))
			return path;

		return path + "/";
	}

	//! Replace extension for given file.
	inline CString ReplaceExtension( const CString &filepath,const CString &ext )
	{
		char path_buffer[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		_splitpath( filepath,drive,dir,fname,0 );
		_makepath( path_buffer,drive,dir,fname,ext );
		return path_buffer;
	}

	//! Replace extension for given file.
	inline CString RemoveExtension( const CString &filepath )
	{
		char path_buffer[_MAX_PATH];
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		_splitpath( filepath,drive,dir,fname,0 );
		_makepath( path_buffer,drive,dir,fname,0 );
		return path_buffer;
	}

	//! Makes a fully specified file path from path and file name.
	inline CString Make( const CString &path,const CString &file )
	{
		//if (gEnv->pCryPak->IsAbsPath(file))
		//	return file;
		return AddBackslash(path) + file;
	}

	//! Makes a fully specified file path from path and file name.
	inline CString Make( const CString &dir,const CString &filename,const CString &ext )
	{
		char path_buffer[_MAX_PATH];
		_makepath( path_buffer,NULL,dir,filename,ext );
		return path_buffer;
	}

	//! Makes a fully specified file path from path and file name - this doesn't do what you think it does .
	inline CString MakeFullPath( const CString &relativePath )
	{
		return relativePath;
		//TCHAR path_buffer[_MAX_PATH];
		//return _tfullpath(path_buffer, relativePath, _MAX_PATH) ? path_buffer : relativePath;
	}

	//! Makes a fully specified file path from path and file name - this does what you think it does.
	inline CString MakeFullPathFromRelative( const CString &inRelativePath, CString &outFullPath)
	{
		TCHAR path_buffer[_MAX_PATH];
		outFullPath = _tfullpath(path_buffer, inRelativePath, _MAX_PATH) ? path_buffer : inRelativePath;
		return outFullPath;
	}

	CString GamePathToFullPath( const CString &path, bool bForWriting = false );
	CString FullPathToGamePath( const CString &path );

	void SplitPath(const CString& rstrFullPathFilename,CString& rstrDriveLetter,CString& rstrDirectory,CString& rstrFilename,CString& rstrExtension);

	// Requires a path from Splithpath: no drive letter and backslash at the end.
	void GetDirectoryQueue(const CString& rstrSourceDirectory,std::vector<CString>& rcstrDirectoryTree);

	// Converts all slashes to backslashes so MS things won't complain.
	void ConvertSlashToBackSlash(CString& rstrStringToConvert);

	// Converts backslashes into forward slashes.
	void ConvertBackSlashToSlash(CString& rstrStringToConvert);

	// Surrounds a string with quotes if necessary. This is useful for calling other programs.
	void SurroundWithQuotes(CString& rstrSurroundString);

	// This function takes a directory string input and returns the parent director string.
	void GetParentDirectoryString(CString& strInputParentDirectory);

  // Gets the temporary directory path (which may not exist).
	CString GetWindowsTempDirectory();

	// This function returns the full path used to run the editor.
	CString GetExecutableFullPath();

	// This function returns the directory above Bin32 or Bin64 from which the editor was run (does not work with Unicode paths).
	CString GetExecutableParentDirectory();

	// This function returns the directory above Bin32 or Bin64 from which the editor was run (also works with Unicode paths).
	CString GetExecutableParentDirectoryUnicode();

	// This function replaces the filename from a path, keeping extension and directory/drive path.
	// WARNING: do not use the same variable in the last parameter and in any of the others.
	CString& ReplaceFilename( const CString &strFilepath,const CString &strFilename,CString& strOutputFilename);
};

inline CString operator /( const CString &first, const CString &second )
{
	return Path::Make(first, second);
}

