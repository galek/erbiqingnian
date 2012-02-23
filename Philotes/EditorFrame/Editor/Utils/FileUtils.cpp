
#include "FileUtils.h"
#include "EditorRoot.h"
#include <io.h>

bool CFileUtil::s_singleFileDlgPref[EFILE_TYPE_LAST] = { true, true, true, true, true };

//////////////////////////////////////////////////////////////////////////

static bool CheckAndCreateDirectory( const char *path )
{
	WIN32_FIND_DATA FindFileData;

	HANDLE hFind = FindFirstFile( path,&FindFileData );
	if (hFind == INVALID_HANDLE_VALUE) 
	{
		return ::CreateDirectory( path,NULL ) == TRUE;
	}
	else
	{
		DWORD attr = FindFileData.dwFileAttributes;
		FindClose(hFind);
		if (attr & FILE_ATTRIBUTE_DIRECTORY) 
		{
			return true;
		}
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

bool CFileUtil::SelectFile( const CString &fileSpec,const CString &searchFolder,CString &fullFileName )
{
	CString filter;
	filter.Format( "%s|%s||",(const char*)fileSpec,(const char*)fileSpec );

	CAutoDirectoryRestoreFileDialog dlg(TRUE, NULL,NULL, OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, filter );
	dlg.m_ofn.lpstrInitialDir = searchFolder;

	if (dlg.DoModal() == IDOK)
	{
		fullFileName = dlg.GetPathName();
		return true;
	}
	return false;
}

bool CFileUtil::SelectFiles( const CString &fileSpec,const CString &searchFolder,std::vector<CString> &files )
{
	CString filter;
	filter.Format( "%s|%s||",(const char*)fileSpec,(const char*)fileSpec );

	char filesStr[16768];
	memset( filesStr,0,sizeof(filesStr) );

	CAutoDirectoryRestoreFileDialog dlg(TRUE, NULL,NULL, OFN_ALLOWMULTISELECT|OFN_ENABLESIZING|OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR, filter );
	dlg.m_ofn.lpstrInitialDir = searchFolder;
	dlg.m_ofn.lpstrFile = filesStr;
	dlg.m_ofn.nMaxFile = sizeof(filesStr);

	files.clear();
	if (dlg.DoModal())
	{
		POSITION pos = dlg.GetStartPosition();
		while (pos != NULL)
		{
			CString fileName = dlg.GetNextPathName(pos);
			if (fileName.IsEmpty())
				continue;
			files.push_back( fileName );
		}
	}

	if (!files.empty())
		return true;

	return false;
}

bool CFileUtil::SelectSaveFile( const CString &fileFilter,const CString &defaulExtension,const CString &startFolder,CString &fileName )
{
	CAutoDirectoryRestoreFileDialog dlg(FALSE, defaulExtension, fileName, OFN_ENABLESIZING|OFN_EXPLORER|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT, fileFilter );
	CString initialPath = Path::MakeFullPath(startFolder);
	dlg.m_ofn.lpstrInitialDir = initialPath;

	if (dlg.DoModal() == IDOK)
	{
		fileName = dlg.GetPathName();
		if (!fileName.IsEmpty())
		{
			return true;
		}
	}

	return false;
}

void CFileUtil::BackupFileDated( const char *filename, bool bUseBackupSubDirectory/*=false */ )
{
	bool makeBackup = true;
	{
		CFile bak;
		if (bak.Open( filename,CFile::modeRead ))
		{
			if (bak.GetLength() <= 0)
				makeBackup = false;
		}
		else
			makeBackup = false;
	}

	if ( makeBackup )
	{
		time_t ltime;
		time( &ltime );
		tm *today = localtime( &ltime );

		char sTemp[128];
		strftime( sTemp, sizeof(sTemp), ".%Y%m%d.%H%M%S.", today);		
		CString bakFilename = Path::RemoveExtension(filename) + sTemp + Path::GetExt(filename);

		if ( bUseBackupSubDirectory )
		{
			CString sBackupPath = Path::ToUnixPath( Path::GetPath( filename ) ) + CString( "/backups" );
			CFileUtil::CreateDirectory( sBackupPath );
			bakFilename = sBackupPath + CString("/") + Path::GetFile( bakFilename );
		}

		SetFileAttributes( filename,FILE_ATTRIBUTE_NORMAL );
		MoveFileEx( filename,bakFilename,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH );	
	}
}

bool CFileUtil::Deltree( const char *szFolder, bool bRecurse )
{
	bool boRemoveResult(false);

	__finddata64_t fd;
	std::string filespec = szFolder;
	filespec += "*.*";
	intptr_t hfil = 0;
	if ((hfil = _findfirst64(filespec.c_str(), &fd)) == -1)
	{
		return false;
	}
	do
	{
		if (fd.attrib & _A_SUBDIR)
		{
			CString name = fd.name;
			if ((name != ".") && (name != ".."))
			{
				if (bRecurse)
				{
					name = szFolder;
					name += fd.name;
					name += "/";
					Deltree(name.GetBuffer(), bRecurse);
				}
			}
		}
		else
		{
			CString name = szFolder;
			name += fd.name;
			CFileUtil::DeleteFile(name);
		}
	} while(!_findnext64(hfil, &fd));

	_findclose(hfil);

	boRemoveResult=CFileUtil::RemoveDirectory(szFolder);

	return boRemoveResult;
}

bool CFileUtil::IsFileExclusivelyAccessable( const CString& strFilePath )
{
	HANDLE hTesteFileHandle(NULL);

	hTesteFileHandle=CreateFile(
		strFilePath,
		0,	
		0,	
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL 
		);

	if (hTesteFileHandle==INVALID_HANDLE_VALUE)
	{
		return false;
	}

	CloseHandle(hTesteFileHandle);

	return true;
}

bool CFileUtil::CreatePath( const CString& strPath )
{
	CString									strDriveLetter;
	CString									strDirectory;
	CString									strFilename;
	CString									strExtension;
	CString									strCurrentDirectoryPath;
	std::vector<CString>					cstrDirectoryQueue;
	size_t									nCurrentPathQueue(0);
	size_t									nTotalPathQueueElements(0);
	BOOL									bnLastDirectoryWasCreated(FALSE);

	if (::PathFileExists(strPath.GetString()))
	{
		return true;
	}

	Path::SplitPath(strPath,strDriveLetter,strDirectory,strFilename,strExtension);
	Path::GetDirectoryQueue(strDirectory,cstrDirectoryQueue);

	if (strDriveLetter.GetLength()>0)
	{
		strCurrentDirectoryPath=strDriveLetter;
		strCurrentDirectoryPath+="\\";
	}


	nTotalPathQueueElements = cstrDirectoryQueue.size();
	for (nCurrentPathQueue = 0; nCurrentPathQueue<nTotalPathQueueElements;++nCurrentPathQueue)
	{
		strCurrentDirectoryPath+=cstrDirectoryQueue[nCurrentPathQueue];
		strCurrentDirectoryPath+="\\";
		bnLastDirectoryWasCreated = ::CreateDirectory(strCurrentDirectoryPath,NULL);
	}

	if (!bnLastDirectoryWasCreated)
	{
		if (ERROR_ALREADY_EXISTS!=GetLastError())
		{
			return false;
		}
	}

	return true;
}

void CFileUtil::CreateDirectory( const char *dir )
{
	if (!PathFileExists(dir))
	{
		::CreateDirectory(dir,NULL);
	}
}

bool CFileUtil::DeleteFile( const CString& strPath )
{
	BOOL bnAttributeChange(FALSE);
	BOOL bnFileDeletion(FALSE);

	bnAttributeChange=SetFileAttributes(strPath,FILE_ATTRIBUTE_NORMAL);
	bnFileDeletion=::DeleteFile(strPath);

	return (bnFileDeletion!=FALSE);
}

bool CFileUtil::RemoveDirectory( const CString& strPath )
{
	BOOL bnRemoveResult(FALSE);
	BOOL bnAttributeChange(FALSE);

	bnAttributeChange=SetFileAttributes(strPath,FILE_ATTRIBUTE_NORMAL);
	bnRemoveResult=::RemoveDirectory(strPath);

	return (bnRemoveResult!=FALSE);
}

CFileUtil::ECopyTreeResult CFileUtil::CopyFile( const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite/*=false*/ )
{
	return ::CopyFile(strSourceFile,strTargetFile,boConfirmOverwrite) ? ETREECOPYOK : ETREECOPYFAIL;
}

CFileUtil::ECopyTreeResult CFileUtil::MoveFile( const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite/*=false*/ )
{
	return ::MoveFile(strSourceFile,strTargetFile) ? ETREECOPYOK : ETREECOPYFAIL;
}

bool CFileUtil::ScanFiles( const CString &path,const CString &fileSpec,CStringArray &files,
						  bool recursive/*=true*/, ScanDirectoryUpdateCallBack updateCB /*= NULL */ )
{
	CStringArray dirs;
	CString searchname;
	CFileFind find;

	files.RemoveAll();

	dirs.Add(path);
	BOOL bRet;
	while(dirs.GetSize()>0)
	{
		if (dirs[0][dirs[0].GetLength()-1] == '\\')
		{
			searchname = dirs[0] + fileSpec;
		}
		else
		{
			searchname = dirs[0] + "\\" + fileSpec;
		}
		dirs.RemoveAt(0);
		bRet = find.FindFile (searchname,0);
		if(!bRet)
		{
			continue;
		}
		do
		{
			bRet = find.FindNextFile ();
			if(find.IsDots())
			{
				//忽略.和..文件
				continue;
			}
			if(find.IsDirectory ())
			{
				//目录
				//在此不做处理
				continue;
			}
			else
			{
				files.Add(find.GetFilePath());
				if (updateCB)
				{
					updateCB(find.GetFilePath());
				}
			}
		}while(bRet) ;
	}
	return true;
}

bool CFileUtil::ScanDirectory( const CString &path,const CString &fileSpec,
							  CStringArray &files, bool recursive/*=true*/, ScanDirectoryUpdateCallBack updateCB /*= NULL */ )
{
	CStringArray dirs;
	CString searchname;
	CFileFind find;

	files.RemoveAll();

	dirs.Add(path);
	BOOL bRet;
	while(dirs.GetSize()>0)
	{
		if (dirs[0][dirs[0].GetLength()-1] == '\\')
		{
			searchname = dirs[0] + fileSpec;
		}
		else
		{
			searchname = dirs[0] + "\\" + fileSpec;
		}
		dirs.RemoveAt(0);
		bRet = find.FindFile (searchname,0);
		if(!bRet)
		{
			continue;
		}
		do
		{
			bRet = find.FindNextFile ();
			if(find.IsDots())
			{
				//忽略.和..文件
				continue;
			}
			if(find.IsDirectory ())
			{
				//目录
				files.Add(find.GetFilePath());
				if (updateCB)
				{
					updateCB(find.GetFilePath());
				}
				continue;
			}
			else
			{
				//文件，此处不处理
			}
		}while(bRet) ;
	}
	return true;
}

CAutoRestoreMasterCDRoot::~CAutoRestoreMasterCDRoot()
{
	SetCurrentDirectory(EditorRoot::Get().GetMasterFolder());
}
