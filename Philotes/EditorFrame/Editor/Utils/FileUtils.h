
/********************************************************************
	����:		2012��2��23��
	�ļ���: 	FileUtils.h
	������:		������
	����:		�ļ���������	
*********************************************************************/

#pragma once


enum ECustomFileType
{
	EFILE_TYPE_ANY,
	EFILE_TYPE_GEOMETRY,
	EFILE_TYPE_TEXTURE,
	EFILE_TYPE_SOUND,
	EFILE_TYPE_DIALOG,
	EFILE_TYPE_LAST,
};

//////////////////////////////////////////////////////////////////////////
class CFileUtil
{
public:
	struct FileDesc
	{
		CString			filename;
		unsigned int	attrib;
		time_t			time_create;    //! -1 for FAT file systems
		time_t			time_access;    //! -1 for FAT file systems
		time_t			time_write;
		INT64			size;
	};

	enum ETextFileType
	{
		FILE_TYPE_SCRIPT,
		FILE_TYPE_SHADER
	};

	enum ECopyTreeResult
	{
		ETREECOPYOK,
		ETREECOPYFAIL,
		ETREECOPYUSERCANCELED,
		ETREECOPYUSERDIDNTCOPYSOMEITEMS,
	};

	typedef std::vector<FileDesc> FileArray;
	typedef bool (*ScanDirectoryUpdateCallBack)(const CString& msg);

	//////////////////////////////////////////////////////////////////////////

	// ����Ŀ¼����Ŀ¼��ֻ�����ļ��У�
	static bool					ScanDirectory( const CString &path,const CString &fileSpec,CStringArray &files, bool recursive=true,
											ScanDirectoryUpdateCallBack updateCB = NULL );

	// ����Ŀ¼����Ŀ¼��ֻ�����ļ���
	static bool					ScanFiles( const CString &path,const CString &fileSpec,CStringArray &files, bool recursive=true,
											ScanDirectoryUpdateCallBack updateCB = NULL );

	// ���ļ��Ի���
	static bool					SelectFile( const CString &fileSpec,const CString &searchFolder,CString &fullFileName );
	
	static bool					SelectFiles( const CString &fileSpec,const CString &searchFolder,std::vector<CString> &files );

	static bool					SelectSaveFile( const CString &fileFilter,const CString &defaulExtension,const CString &startFolder,CString &fileName );

	// ��ʱ���ʽ�����ļ�
	static void					BackupFileDated( const char *filename, bool bUseBackupSubDirectory=false );

	static bool					Deltree(const char *szFolder, bool bRecurse);

	static bool   				IsFileExclusivelyAccessable(const CString& strFilePath);

	static bool   				CreatePath(const CString& strPath);

	static void					CreateDirectory( const char *dir );

	static bool   				DeleteFile(const CString& strPath);

	static bool					RemoveDirectory(const CString& strPath);

	static ECopyTreeResult   	CopyFile(const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite=false);

	static ECopyTreeResult   	MoveTree(const CString& strSourceDirectory,const CString& strTargetDirectory,bool boRecurse=true,bool boConfirmOverwrite=false);

	static ECopyTreeResult   	MoveFile(const CString& strSourceFile,const CString& strTargetFile,bool boConfirmOverwrite=false);

	static bool					SmartSelectSingleFile(ECustomFileType fileType, CString &outputFile, 
									const CString& initialSearchTerm, const CString& initialDir="", const CString& filter="");

	struct ExtraMenuItems
	{
		std::vector<CString> names;
		int selectedIndexIfAny;

		ExtraMenuItems() : selectedIndexIfAny(-1) {}

		int AddItem(const CString& name)
		{
			names.push_back(name);
			return names.size() - 1;
		}
	};

private:
	static bool s_singleFileDlgPref[EFILE_TYPE_LAST];

};

//////////////////////////////////////////////////////////////////////////

class CAutoRestoreMasterCDRoot
{
public:
	~CAutoRestoreMasterCDRoot();
};

class CAutoDirectoryRestoreFileDialog : public CAutoRestoreMasterCDRoot, public CFileDialog
{
public:
	explicit CAutoDirectoryRestoreFileDialog(
		BOOL bOpenFileDialog,
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0
		) : CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize) {}
};
