//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------


#include <windows.h>
#include <psapi.h>
#include <Shlwapi.h>	// DLLGetVersion
#include <string>
#include "safe_vector.h"
#include <algorithm>
#include <ws2spi.h>

//#define _SETUPAPI_VER 0x0501
#include <setupapi.h>	//

#include "sysinfo.h"
#include "syscaps.h"

//-----------------------------------------------------------------------------

namespace
{

//-----------------------------------------------------------------------------

#ifdef SYSINFO_USE_UNICODE_ENCODING
// Unicode encoding.
typedef std::wstring string_type;
#define _F(s) s##W
#define _SUFFIX_STR "W"
#define local_sprintf swprintf_s
#define local_vsprintf vswprintf_s
#else
// ASCII encoding.
typedef std::string string_type;
#define _F(s) s##A
#define _SUFFIX_STR "A"
#define local_sprintf sprintf_s
#define local_vsprintf vsprintf_s
#endif
#define _P(s) SYSINFO_TEXT(s)
typedef string_type::value_type char_type;
typedef safe_vector<string_type> string_vector;

const unsigned ONEMB = 1024 * 1024;

const unsigned SYSINFO_DATETIME_STRING_SIZE = 32;


//-----------------------------------------------------------------------------
struct SYSINFO_CALLBACK
{
	SYSINFO_REPORTFUNC * pCallback;
	void * pContext;
};

typedef safe_vector<SYSINFO_CALLBACK> SYSINFO_CALLBACK_VECTOR;
SYSINFO_CALLBACK_VECTOR * s_pSysInfo_Callbacks = 0;

//-----------------------------------------------------------------------------

const char_type * _GetOSVersionString(const _F(OSVERSIONINFO) & osv)
{
	if (osv.dwMajorVersion > 6) {
		return _P("Vista or later");
	}
	else if (osv.dwMajorVersion >= 6) {
		return _P("Vista");
	}
	else if (osv.dwMajorVersion >= 5) {
		if (osv.dwMinorVersion > 2) {
			// unknown
		}
		else if (osv.dwMinorVersion >= 2) {
			return _P("Server 2003");
		}
		else if (osv.dwMinorVersion >= 1) {
			return _P("XP");
		}
		else {
			return _P("2000");
		}
	}
	else if (osv.dwMajorVersion >= 4) {
		if (osv.dwMinorVersion >= 90) {
			return _P("ME");
		}
		else if (osv.dwMinorVersion >= 10) {
			return _P("98");
		}
		else {
			if (osv.dwPlatformId == VER_PLATFORM_WIN32_NT)
			{
				// NT
				return _P("NT 4");
			}
			return _P("95");
		}
	}
	return _P("<unknown>");
}

void SysInfo_ReportOperatingSystem(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Operating System Info"));
	_F(OSVERSIONINFO) osv;
	osv.dwOSVersionInfoSize = sizeof(osv);
	if (_F(::GetVersionEx)(&osv))
	{
		pLog->WriteLn(_P("Operating system = %s"), _GetOSVersionString(osv));
		pLog->WriteLn(_P("Operating system version = %u.%u build %u"), osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber);
		pLog->WriteLn(_P("Operating system detail = %s"), osv.szCSDVersion);
		pLog->WriteLn(_P("Operating system is NT = %s"), (osv.dwPlatformId == VER_PLATFORM_WIN32_NT) ? _P("yes") : _P("no"));
	}
	else
	{
		pLog->WriteLn(_P("[Error 0x%08x calling GetVersionEx()]"), ::GetLastError());
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void _GetMemoryStatus(MEMORYSTATUSEX & ms)
{
	ms.dwLength = sizeof(ms);

	HMODULE hKernel = ::GetModuleHandleA("kernel32.dll");
	if (hKernel != 0)
	{
		FARPROC pProc = ::GetProcAddress(hKernel, "GlobalMemoryStatusEx");
		if (pProc != 0)
		{
			if ((*reinterpret_cast<BOOL (WINAPI *)(MEMORYSTATUSEX *)>(pProc))(&ms))
			{
				return;
			}
		}
	}

	MEMORYSTATUS t;
	t.dwLength = sizeof(t);
	::GlobalMemoryStatus(&t);
	ms.dwMemoryLoad = t.dwMemoryLoad;
	ms.ullTotalPhys = t.dwTotalPhys;
	ms.ullAvailPhys = t.dwAvailPhys;
	ms.ullTotalPageFile = t.dwTotalPageFile;
	ms.ullAvailPageFile = t.dwAvailPageFile;
	ms.ullTotalVirtual = t.dwTotalVirtual;
	ms.ullAvailVirtual = t.dwAvailVirtual;
	ms.ullAvailExtendedVirtual = 0;
}

void SysInfo_ReportMemoryExtents(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Memory Info"));
	MEMORYSTATUSEX ms;
	_GetMemoryStatus(ms);
	pLog->WriteLn(_P("Total physical memory = %llu MB"), ms.ullTotalPhys / ONEMB);
	pLog->WriteLn(_P("Available physical memory = %llu MB"), ms.ullAvailPhys / ONEMB);
	pLog->WriteLn(_P("Total virtual memory = %llu MB"), ms.ullTotalVirtual / ONEMB);
	pLog->WriteLn(_P("Available virtual memory = %llu MB"), ms.ullAvailVirtual / ONEMB);
}

//-----------------------------------------------------------------------------

ULONGLONG _GetVolumeBytes(const char_type * pPath, unsigned nIndex)
{
	ULARGE_INTEGER nResult[3];
	HMODULE hKernel = ::GetModuleHandleA("kernel32.dll");
	if (hKernel != 0)
	{
		FARPROC pProc = ::GetProcAddress(hKernel, "GetDiskFreeSpaceEx" _SUFFIX_STR);
		if (pProc != 0)
		{
			if ((*reinterpret_cast<BOOL (WINAPI *)(const char_type *, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER)>(pProc))(pPath, &nResult[0], &nResult[1], &nResult[2]))
			{
				return nResult[nIndex].QuadPart;
			}
		}
	}

	DWORD nSectorsPerCluster, nBytesPerSector, nNumberOfFreeClusters, nTotalNumberOfClusters;
	bool bResult = !!::_F(GetDiskFreeSpace)(
		pPath,					// root path
		&nSectorsPerCluster,	// sectors per cluster
		&nBytesPerSector,		// bytes per sector
		&nNumberOfFreeClusters,	// free clusters
		&nTotalNumberOfClusters	// total clusters
	);
	if (!bResult)
	{
		return 0;
	}
	nResult[0].QuadPart = static_cast<ULONGLONG>(nNumberOfFreeClusters) * nSectorsPerCluster * nBytesPerSector;
	nResult[1].QuadPart = nResult[0].QuadPart;
	nResult[2].QuadPart = static_cast<ULONGLONG>(nTotalNumberOfClusters) * nSectorsPerCluster * nBytesPerSector;
	return nResult[nIndex].QuadPart;
}

void SysInfo_ReportHardDisks(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Drive Free Space"));
	DWORD nDrives = ::GetLogicalDrives();
	char_type buf[4];
	buf[0] = _P('A');
	buf[1] = _P(':');
	buf[2] = _P('\\');
	buf[3] = _P('\0');
	for (unsigned nDrive = 2; nDrive < 26; ++nDrive) {
		if ((nDrives & (1 << nDrive)) != 0) {
			buf[0] = static_cast<char_type>(_P('A') + nDrive);
			switch (_F(::GetDriveType)(buf)) {
				case DRIVE_NO_ROOT_DIR:
				case DRIVE_CDROM:
					break;
				default: {
					ULONGLONG nFree = _GetVolumeBytes(buf, 0);
					ULONGLONG nTotal = _GetVolumeBytes(buf, 1);
					pLog->WriteLn(_P("%c: has %llu MB free out of %llu MB"), buf[0], nFree / ONEMB, nTotal / ONEMB);
					break;
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------

struct ModuleInfo
{
	HMODULE hMod;
	MODULEINFO mi;
	bool operator<(const ModuleInfo & rhs) const
	{
		return mi.lpBaseOfDll < rhs.mi.lpBaseOfDll;
	}
};

string_type _GetModuleFileName(HMODULE hModule)
{
	DWORD nSize = MAX_PATH + 1;
	char_type * pBuf = 0;
	for (;;)
	{
		pBuf = new char_type[nSize];
		DWORD nResult = ::_F(GetModuleFileName)(hModule, pBuf, nSize);
		if (nResult == 0)
		{
			delete[] pBuf;
			return string_type();
		}
		if (nResult < nSize)
		{
			string_type s(pBuf);
			delete[] pBuf;
			return s;
		}
		delete[] pBuf;
		nSize *= 2;
	}
}

BOOL _VerQueryValue(const LPVOID pBlock, LPSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen)
{
	bool bResult = false;

	HMODULE hMod = ::LoadLibraryA("version.dll");
	if (hMod != 0)
	{
		FARPROC pProc = ::GetProcAddress(hMod, "VerQueryValueA");
		if (pProc != 0)
		{
			bResult = !!(*reinterpret_cast<BOOL (WINAPI *)(const LPVOID, LPSTR, LPVOID *, PUINT)>(pProc))(pBlock, lpSubBlock, lplpBuffer, puLen);
		}

		::FreeLibrary(hMod);
	}

	return bResult;
}

BOOL CALLBACK _GetDLLVersion_EnumProc(HMODULE /*hModule*/, LPCSTR /*lpszType*/, LPSTR lpszName, LONG_PTR lParam)
{
	*reinterpret_cast<LPSTR *>(lParam) = lpszName;
	return false;
}

ULONGLONG _GetDLLVersion(HMODULE hMod)
{
	if (hMod == 0)
	{
		return 0;
	}

	// Get version from DllGetVersion() function.
	FARPROC p = ::GetProcAddress(hMod, "DllGetVersion");
	if (p != 0)
	{
		DLLGETVERSIONPROC pDGV = reinterpret_cast<DLLGETVERSIONPROC>(p);
		HRESULT hRes;

		DLLVERSIONINFO2 dvi;
		dvi.info1.cbSize = sizeof(dvi);
		hRes = (*pDGV)(&dvi.info1);
		if (hRes == NOERROR)
		{
			return dvi.ullVersion;
		}

		dvi.info1.cbSize = sizeof(dvi.info1);
		hRes = (*pDGV)(&dvi.info1);
		if (hRes == NOERROR)
		{
			dvi.ullVersion = MAKEDLLVERULL(dvi.info1.dwMajorVersion, dvi.info1.dwMinorVersion, dvi.info1.dwBuildNumber, 0);
			return dvi.ullVersion;
		}
	}

	// Get version from resource.
	unsigned char * pBuf = 0;
	ULONGLONG nResult = 0;
	for (;;)
	{
		LPCSTR pName = 0;
		::EnumResourceNamesA(hMod, RT_VERSION, &_GetDLLVersion_EnumProc, reinterpret_cast<LONG_PTR>(&pName));
		HRSRC hRsrc = ::FindResourceA(hMod, pName, RT_VERSION);
		if (hRsrc == 0)
		{
			break;
		}
		DWORD nSize = ::SizeofResource(hMod, hRsrc);
		if (nSize == 0)
		{
			break;
		}
		pBuf = new unsigned char[nSize];
		{
			HGLOBAL hGlob = ::LoadResource(hMod, hRsrc);
			if (hGlob == 0)
			{
				break;
			}
			void * pRes = ::LockResource(hGlob);
			if (pRes == 0)
			{
				break;
			}
			memcpy(pBuf, pRes, nSize);
		}
		void * pSubBlock = 0;
		UINT nLen = 0;
		if (!_VerQueryValue(pBuf, "\\", &pSubBlock, &nLen))
		{
			break;
		}
		if ((pSubBlock == 0) || (nLen == 0))
		{
			break;
		}
		const VS_FIXEDFILEINFO * pInfo = static_cast<const VS_FIXEDFILEINFO *>(pSubBlock);
		LARGE_INTEGER li;
		li.LowPart = pInfo->dwFileVersionLS;
		li.HighPart = pInfo->dwFileVersionMS;
		nResult = li.QuadPart;
		break;
	}

	delete[] pBuf;
	return nResult;
}

bool SysInfo_GetFileWriteTime(FILETIME & TimeOut, const string_type & FileName)
{
	_F(WIN32_FIND_DATA) FindData;
	FindFileHandle handle = _F(::FindFirstFile)(FileName.c_str(), &FindData);
	if ((handle == 0) || (handle == INVALID_HANDLE_VALUE))
	{
		return false;
	}
	TimeOut = FindData.ftLastWriteTime;
	return true;
}

void SysInfo_GetFileTimeString(const FILETIME & Time, char_type * pStrOut, unsigned nStrBufferSizeInChars)
{
	SYSTEMTIME st;
	if (FileTimeToSystemTime(&Time, &st))
	{
		PStrPrintf(pStrOut, nStrBufferSizeInChars, _P("%u.%02u.%02u %02u:%02u:%02u"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
	}
}

void SysInfo_ReportModules(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Loaded Module Info"));
	bool bSuccess = false;
	HMODULE hMod = 0;
	for (;;)
	{
		hMod = ::LoadLibraryA("psapi.dll");
		if (hMod == 0)
		{
			break;
		}

		typedef BOOL (WINAPI EnumProcessModules_Type)(HANDLE, HMODULE *, DWORD, LPDWORD);
		EnumProcessModules_Type * pEnumProcessModules = reinterpret_cast<EnumProcessModules_Type *>(::GetProcAddress(hMod, "EnumProcessModules"));
		if (pEnumProcessModules == 0)
		{
			break;
		}

		typedef BOOL (WINAPI GetModuleInformation_Type)(HANDLE, HMODULE, LPMODULEINFO, DWORD cb);
		GetModuleInformation_Type * pGetModuleInformation = reinterpret_cast<GetModuleInformation_Type *>(::GetProcAddress(hMod, "GetModuleInformation"));
		if (pGetModuleInformation == 0)
		{
			break;
		}

		// Get list of modules.
		HANDLE hProcess = ::GetCurrentProcess();
		DWORD nBytesNeeded = 0;
		if (!(*pEnumProcessModules)(hProcess, 0, 0, &nBytesNeeded))
		{
			break;
		}
		if (nBytesNeeded < sizeof(HMODULE))
		{
			break;
		}
		HMODULE * pList = 0;
		unsigned nItems = 0;
		bool bFail = false;
		do
		{
			if (nBytesNeeded > nItems * sizeof(*pList))
			{
				delete[] pList;
				// CHB 2007.01.12 - Fix build server warning (code below is correct).
#pragma warning(push)
#pragma warning(disable:6384)	// warning C6384: Dividing sizeof a pointer by another value
				nItems = (nBytesNeeded + sizeof(*pList) - 1) / sizeof(*pList);
#pragma warning(pop)
				pList = new HMODULE[nItems];
			}
			if (!(*pEnumProcessModules)(hProcess, pList, nItems * sizeof(*pList), &nBytesNeeded))
			{
				bFail = true;
				break;
			}
		}
		while (nBytesNeeded > nItems * sizeof(*pList));
		if (bFail)
		{
			delete[] pList;
			break;
		}
		nItems = nBytesNeeded / sizeof(*pList);

		// Get info for each module.
		typedef safe_vector<ModuleInfo> ModuleInfoVector;
		ModuleInfoVector MIs;
		MIs.reserve(nItems);
		for (unsigned i = 0; i < nItems; ++i)
		{
			ModuleInfo t;
			if ((*pGetModuleInformation)(hProcess, pList[i], &t.mi, sizeof(t.mi)))
			{
				t.hMod = pList[i];
				MIs.push_back(t);
			}
		}
		delete[] pList;
		std::sort(MIs.begin(), MIs.end());
		pLog->WriteLn(_P("Start    End      Size     Version                  Date/Time            Path"));
		pLog->WriteLn(_P("-------- -------- -------- -----------------------  -------------------  ----------"));
		HMODULE hVersion = ::LoadLibraryA("version.dll");	// cache it
		for (ModuleInfoVector::const_iterator i = MIs.begin(); i != MIs.end(); ++i)
		{
			string_type ModFN = _GetModuleFileName(i->hMod);
			const void * pEnd = static_cast<const char *>(i->mi.lpBaseOfDll) + i->mi.SizeOfImage - 1;
			ULONGLONG nVersion = _GetDLLVersion(i->hMod);
			char_type VerStr[SYSINFO_VERSION_STRING_SIZE];
			SysInfo_VersionToString(VerStr, _countof(VerStr), nVersion);
			
			char_type TimeStr[SYSINFO_DATETIME_STRING_SIZE];
			TimeStr[0] = _P('\0');
			FILETIME WriteTime;
			if (SysInfo_GetFileWriteTime(WriteTime, ModFN))
			{
				SysInfo_GetFileTimeString(WriteTime, TimeStr, _countof(TimeStr));
			}

			pLog->WriteLn(_P("%p %p %8x %23s  %19s  %s"), i->mi.lpBaseOfDll, pEnd, static_cast<unsigned>(i->mi.SizeOfImage), VerStr, TimeStr, ModFN.c_str());
		}
		::FreeLibrary(hVersion);

		bSuccess = true;
		break;
	}

	if (!bSuccess)
	{
		pLog->WriteLn(_P("[Error 0x%08x using psapi.dll]"), ::GetLastError());
	}
	if (hMod != 0)
	{
		::FreeLibrary(hMod);
	}
}

//-----------------------------------------------------------------------------

class RegKey
{
	public:
		//bool Open(HKEY hKeyParent, const char_type * pSub);
		bool Open(const RegKey & KeyParent, const string_type & Sub);
		bool Open(HKEY hKeyParent, const string_type & Sub);
		void Reset(void);

		bool GetStringValue(string_type & StrOut, const char_type * pValueName);
		bool GetStringVectorValue(string_vector & StrVecOut, const char_type * pValueName);

		RegKey(void);
		~RegKey(void);

	private:
		typedef unsigned char data_type;
		bool GetValue(data_type * & pDataOut, DWORD & nTypeOut, const char_type * pValueName);	// caller must delete data
		HKEY hKey;
};

bool RegKey::GetValue(data_type * & pDataOut, DWORD & nTypeOut, const char_type * pValueName)
{
	data_type * pData = 0;
	unsigned nDataSize = 0;
	for (;;)
	{
		DWORD nRequiredSize = nDataSize;
		LONG nResult = _F(::RegQueryValueEx)(
			hKey,			// HKEY hKey
			pValueName,		// LPCTSTR lpValueName
			0,				// LPDWORD lpReserved
			&nTypeOut,		// LPDWORD lpType
			pData,			// LPBYTE lpData
			&nRequiredSize	// LPDWORD lpcbData
		);

		if ((nResult == ERROR_MORE_DATA) || (pData == 0))
		{
			// Add 4 to make sure there's always room for 2 wchar_t null terminators.
			nDataSize = max(nDataSize, nRequiredSize + 4);
			delete[] pData;
			pData = new data_type[nDataSize];
		}
		else if (nResult == ERROR_SUCCESS)
		{
			pDataOut = pData;
			return true;
		}
		else
		{
			delete[] pData;
			return false;
		}
	}
}

bool RegKey::GetStringVectorValue(string_vector & StrVecOut, const char_type * pValueName)
{
	data_type * pData = 0;
	DWORD nType = 0;
	if (!GetValue(pData, nType, pValueName))
	{
		return false;
	}
	if ((nType == REG_SZ) || (nType == REG_EXPAND_SZ))
	{
		string_type s(static_cast<const char_type *>(static_cast<const void *>(pData)));
		delete[] pData;
		string_vector v;
		v.push_back(s);
		StrVecOut.swap(v);
		return true;
	}
	if (nType == REG_MULTI_SZ)
	{
		const char_type * pStr = static_cast<const char_type *>(static_cast<const void *>(pData));
		string_vector v;
		while (*pStr != _P('\0'))
		{
			string_type s(pStr);
			v.push_back(s);
			pStr += s.length() + 1;
		}
		delete[] pData;
		StrVecOut.swap(v);
		return true;
	}
	delete[] pData;
	return false;
}

bool RegKey::GetStringValue(string_type & StrOut, const char_type * pValueName)
{
	data_type * pData = 0;
	DWORD nType = 0;
	if (!GetValue(pData, nType, pValueName))
	{
		return false;
	}
	if ((nType != REG_SZ) && (nType != REG_EXPAND_SZ))
	{
		return false;
	}
	StrOut = string_type(static_cast<char_type *>(static_cast<void *>(pData)));
	delete[] pData;
	return true;
}

bool RegKey::Open(HKEY hKeyParent, const string_type & Sub)
{
	Reset();
	string_type::size_type nPos = Sub.rfind(_P('\\'));
	if (nPos == string_type::npos)
	{
		LONG nResult = _F(::RegOpenKeyEx)(hKeyParent, Sub.c_str(), 0, KEY_READ, &hKey);
		return nResult == ERROR_SUCCESS;
	}
	else if (nPos == 0)
	{
		return false;
	}
	else if (nPos + 1 == Sub.length())
	{
		return false;
	}
	else
	{
		RegKey t;
		string_type Parent = Sub.substr(0, nPos);
		if (!t.Open(hKeyParent, Parent))
		{
			return false;
		}
		return Open(t.hKey, Sub.substr(nPos + 1));
	}
}

bool RegKey::Open(const RegKey & KeyParent, const string_type & Sub)
{
	return Open(KeyParent.hKey, Sub);
}

void RegKey::Reset(void)
{
	if (hKey != 0)
	{
		::RegCloseKey(hKey);
		hKey = 0;
	}
}

RegKey::RegKey(void)
{
	hKey = 0;
}

RegKey::~RegKey(void)
{
	Reset();
}

//-----------------------------------------------------------------------------

typedef BOOL (WINAPI SetupDiGetDeviceRegistryProperty_Proc)(HDEVINFO DeviceInfoSet, const SP_DEVINFO_DATA * DeviceInfoData, DWORD Property, PDWORD PropertyRegDataType, PBYTE PropertyBuffer, DWORD PropertyBufferSize, PDWORD RequiredSize);
typedef HDEVINFO (WINAPI SetupDiGetClassDevs_Proc)(const GUID * ClassGuid, const char_type * Enumerator, HWND hwndParent, DWORD Flags);
typedef BOOL (WINAPI SetupDiEnumDeviceInfo_Proc)(HDEVINFO DeviceInfoSet, DWORD MemberIndex, PSP_DEVINFO_DATA DeviceInfoData);
typedef BOOL (WINAPI SetupDiDestroyDeviceInfoList_Proc)(HDEVINFO DeviceInfoSet);

// Not very useful right now.
//#define SYSINFO_USE_DRIVER_INFO 1

#ifdef SYSINFO_USE_DRIVER_INFO
typedef _F(SP_DRVINFO_DATA_V2_) LOCAL_DRVINFO_DATA;
typedef BOOL (WINAPI SetupDiEnumDriverInfo_Proc)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD DriverType, DWORD MemberIndex, LOCAL_DRVINFO_DATA * DriverInfoData);
typedef BOOL (WINAPI SetupDiBuildDriverInfoList_Proc)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD DriverType);
typedef BOOL (WINAPI SetupDiDestroyDriverInfoList_Proc)(HDEVINFO DeviceInfoSet, PSP_DEVINFO_DATA DeviceInfoData, DWORD DriverType);
#endif

const string_vector _GetDeviceRegistryPropertyVector(SetupDiGetDeviceRegistryProperty_Proc * pSetupDiGetDeviceRegistryProperty, HDEVINFO hDevInfo, const SP_DEVINFO_DATA & DeviceInfoData, DWORD nProperty)
{
	char_type * pBuffer = 0;
	DWORD nBufferSize = 0;
	for (;;)
	{
		DWORD nRegDataType = 0;
		DWORD nRequiredSize = 0;
		bool bResult = !!(*pSetupDiGetDeviceRegistryProperty)(
			hDevInfo,
			&DeviceInfoData,
			nProperty,
			&nRegDataType,
			reinterpret_cast<BYTE *>(pBuffer),
			nBufferSize,
			&nRequiredSize
		);
		if (bResult && pBuffer)
		{
			if (nRegDataType == REG_MULTI_SZ)
			{
				const char_type * p = pBuffer;
				string_vector v;
				while (*p != _P('\0'))
				{
					string_type s(p);
					v.push_back(s);
					p += s.length() + 1;
				}
				delete[] pBuffer;
				return v;
			}
			else if ((nRegDataType == REG_SZ) || (nRegDataType == REG_EXPAND_SZ))
			{
				string_type s(pBuffer);
				delete[] pBuffer;
				string_vector v;
				v.push_back(s);
				return v;
			}
			else
			{
				return string_vector();
			}
		}
		delete[] pBuffer;
		if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			return string_vector();
		}
		// "Double the size to avoid problems on W2k MBCS systems per KB 888609."
		nBufferSize = nRequiredSize * 2;
		pBuffer = new char_type[nBufferSize / sizeof(char_type)];
	}
}

const string_type _GetDeviceRegistryProperty(SetupDiGetDeviceRegistryProperty_Proc * pSetupDiGetDeviceRegistryProperty, HDEVINFO hDevInfo, const SP_DEVINFO_DATA & DeviceInfoData, DWORD nProperty)
{
	const string_vector v = _GetDeviceRegistryPropertyVector(pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, nProperty);
	if (v.empty())
	{
		return string_type();
	}
	return v.front();
}

string_type _GetServiceImagePath(const string_type & ServiceName)
{
	RegKey k;
	if (!k.Open(HKEY_LOCAL_MACHINE, _P("SYSTEM\\CurrentControlSet\\Services\\") + ServiceName))
	{
		return string_type();
	}
	string_type v;
	if (!k.GetStringValue(v, _P("ImagePath")))
	{
		return string_type();
	}
	return v;
}

string_type _ExpandEnvironmentStrings(const string_type & Str)
{
	char_type * pBuf = 0;
	unsigned nBufChars = 0;
	for (;;)
	{
		DWORD nResult = _F(::ExpandEnvironmentStrings)(Str.c_str(), pBuf, nBufChars);
		if (nResult <= nBufChars)
		{
			string_type s(pBuf);
			delete[] pBuf;
			return s;
		}
		delete[] pBuf;
		nBufChars = max(nBufChars, nResult);
		pBuf = new char_type[nBufChars];
	}
}

bool _DoesFileExist(const string_type & FileName)
{
	return _F(::GetFileAttributes)(FileName.c_str()) != INVALID_FILE_ATTRIBUTES;
}

string_type _FindServiceImage(const string_type & ServiceNameIn)
{
	string_type ServiceName = _ExpandEnvironmentStrings(ServiceNameIn);
	if (_DoesFileExist(ServiceName))
	{
		return ServiceName;
	}

	ServiceName = _ExpandEnvironmentStrings(_P("%SystemRoot%\\") + ServiceNameIn);
	if (_DoesFileExist(ServiceName))
	{
		return ServiceName;
	}

	ServiceName = _ExpandEnvironmentStrings(_P("%SystemRoot%\\system32\\drivers\\") + ServiceNameIn);
	if (_DoesFileExist(ServiceName))
	{
		return ServiceName;
	}

	return string_type();
}

void _DumpDriverInfo(SysInfoLog * pLog, const char_type * pType, const string_vector & Strs)
{
	for (string_vector::const_iterator i = Strs.begin(); i != Strs.end(); ++i)
	{
		const string_type & ServiceName = *i;

		pLog->Write(_P(",%s,\"%s\""), pType, ServiceName.c_str());

		// Find the service's image path.
		string_type ImagePath = _GetServiceImagePath(ServiceName);
		if (ImagePath.empty())
		{
			// No image path (okay for drivers).
			// Make it.
			ImagePath = ServiceName + _P(".sys");
		}
		string_type ImageFile = _FindServiceImage(ImagePath);
		if (!ImageFile.empty())
		{
			pLog->Write(_P(",\"%s\""), ImageFile.c_str());

			HMODULE hMod = _F(::LoadLibraryEx)(ImageFile.c_str(), 0, LOAD_LIBRARY_AS_DATAFILE);
			if (hMod != 0)
			{
				ULONGLONG nVersion = _GetDLLVersion(hMod);
				::FreeLibrary(hMod);
				char_type VerStr[SYSINFO_VERSION_STRING_SIZE];
				SysInfo_VersionToString(VerStr, _countof(VerStr), nVersion);
				pLog->Write(_P(",%s"), VerStr);
			}
		}

		pLog->NewLine();
	}
}

void _DumpDriverInfo(SysInfoLog * pLog, SetupDiGetDeviceRegistryProperty_Proc * pSetupDiGetDeviceRegistryProperty, HDEVINFO hDevInfo, const SP_DEVINFO_DATA & DeviceInfoData, const char_type * pType, DWORD nProperty)
{
	const string_vector v = _GetDeviceRegistryPropertyVector(pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, nProperty);
	_DumpDriverInfo(pLog, pType, v);
}

void SysInfo_ReportHardware(SysInfoLog * pLog, bool bIncludeDrivers)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Hardware Information"));

	HMODULE hModule = 0;
	HDEVINFO hDevInfo = 0;
	SetupDiDestroyDeviceInfoList_Proc * pSetupDiDestroyDeviceInfoList = 0;

	HMODULE hVersion = ::LoadLibraryA("version.dll");	// cache it

	bool bSuccess = false;
	for (;;)
	{
		// Get library.
		hModule = ::LoadLibraryA("setupapi.dll");
		if ((hModule == 0) || (hModule == INVALID_HANDLE_VALUE))
		{
			hModule = 0;
			break;
		}

		// Get functions.
		SetupDiGetClassDevs_Proc * pSetupDiGetClassDevs = reinterpret_cast<SetupDiGetClassDevs_Proc *>(::GetProcAddress(hModule, "SetupDiGetClassDevs" _SUFFIX_STR));
		if (pSetupDiGetClassDevs == 0)
		{
			break;
		}
		SetupDiEnumDeviceInfo_Proc * pSetupDiEnumDeviceInfo = reinterpret_cast<SetupDiEnumDeviceInfo_Proc *>(::GetProcAddress(hModule, "SetupDiEnumDeviceInfo"));
		if (pSetupDiEnumDeviceInfo == 0)
		{
			break;
		}
		SetupDiGetDeviceRegistryProperty_Proc * pSetupDiGetDeviceRegistryProperty = reinterpret_cast<SetupDiGetDeviceRegistryProperty_Proc *>(::GetProcAddress(hModule, "SetupDiGetDeviceRegistryProperty" _SUFFIX_STR));
		if (pSetupDiGetDeviceRegistryProperty == 0)
		{
			break;
		}
		pSetupDiDestroyDeviceInfoList = reinterpret_cast<SetupDiDestroyDeviceInfoList_Proc *>(::GetProcAddress(hModule, "SetupDiDestroyDeviceInfoList"));
		if (pSetupDiDestroyDeviceInfoList == 0)
		{
			break;
		}
#ifdef SYSINFO_USE_DRIVER_INFO
		SetupDiEnumDriverInfo_Proc * pSetupDiEnumDriverInfo = 0;
		if (bIncludeDrivers)
		{
			pSetupDiEnumDriverInfo = reinterpret_cast<SetupDiEnumDriverInfo_Proc *>(::GetProcAddress(hModule, "SetupDiEnumDriverInfo" _SUFFIX_STR));
			if (pSetupDiEnumDriverInfo == 0)
			{
				break;
			}
		}
#endif

		// Create a HDEVINFO with all present devices.
		hDevInfo = (*pSetupDiGetClassDevs)(0, 0, 0, DIGCF_PRESENT | DIGCF_ALLCLASSES);
		if ((hDevInfo == 0) || (hDevInfo == INVALID_HANDLE_VALUE))
		{
			hDevInfo = 0;
			break;
		}

		//
		static const DWORD nProps[] = { SPDRP_DEVICEDESC, SPDRP_FRIENDLYNAME, SPDRP_CLASS, SPDRP_MFG, SPDRP_HARDWAREID, SPDRP_LOCATION_INFORMATION, };
		pLog->WriteLn(_P("DeviceDesc,FriendlyName,Class,Mfg,HardwareID,LocationInfo"));
		pLog->WriteLn(_P(",DriverType,Service,DriverPath,DriverVersion"));

		// Enumerate through all devices in Set.
		SP_DEVINFO_DATA DeviceInfoData;
		DeviceInfoData.cbSize = sizeof(DeviceInfoData);
		for (unsigned i = 0; (*pSetupDiEnumDeviceInfo)(hDevInfo, i, &DeviceInfoData); ++i)
		{
			for (unsigned j = 0; j < _countof(nProps); ++j)
			{
				if (j > 0)
				{
					pLog->Write(_P(","));
				}
				string_type Str = _GetDeviceRegistryProperty(pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, nProps[j]);
				if (!Str.empty())
				{
					pLog->Write(_P("\"%s\""), Str.c_str());
				}
			}
			pLog->NewLine();

			// Device driver.
			_DumpDriverInfo(pLog, pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, _P("ddr"), SPDRP_SERVICE);

			// Device lower filter drivers.
			_DumpDriverInfo(pLog, pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, _P("dlf"), SPDRP_LOWERFILTERS);

			// Device upper filter drivers.
			_DumpDriverInfo(pLog, pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, _P("duf"), SPDRP_UPPERFILTERS);

			// Dump class filter drivers.
			string_type ClassGUIDStr = _GetDeviceRegistryProperty(pSetupDiGetDeviceRegistryProperty, hDevInfo, DeviceInfoData, SPDRP_CLASSGUID);
			if (!ClassGUIDStr.empty())
			{
				// We could use SetupDiOpenClassRegKey() here but
				// that would necessitate converting the GUID from
				// string form to integer form.
				RegKey k;
				if (k.Open(HKEY_LOCAL_MACHINE, _P("SYSTEM\\CurrentControlSet\\Control\\Class\\") + ClassGUIDStr))
				{
					string_vector v;

					// Class lower filter drivers.
					if (k.GetStringVectorValue(v, _P("LowerFilters")))
					{
						_DumpDriverInfo(pLog, _P("clf"), v);
					}

					// Class upper filter drivers.
					if (k.GetStringVectorValue(v, _P("UpperFilters")))
					{
						_DumpDriverInfo(pLog, _P("cuf"), v);
					}
				}
			}

#ifdef SYSINFO_USE_DRIVER_INFO
			// CHB 2007.01.19 - The OS's driver details property
			// sheet parses the INF file to obtain all the related
			// drivers. I haven't figured out how to do this yet.
			if (bIncludeDrivers)
			{
				_F(SP_DEVINSTALL_PARAMS_) DeviceInstallParams;
				DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
				if (_F(SetupDiGetDeviceInstallParams)(hDevInfo, &DeviceInfoData, &DeviceInstallParams))
				{
					DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS | DI_FLAGSEX_INSTALLEDDRIVER;
					_F(SetupDiSetDeviceInstallParams)(hDevInfo, &DeviceInfoData, &DeviceInstallParams);
				}

				const DWORD nDriverType = SPDIT_COMPATDRIVER;
				if (SetupDiBuildDriverInfoList(hDevInfo, &DeviceInfoData, nDriverType))
				{
					LOCAL_DRVINFO_DATA DriverInfoData;
					DriverInfoData.cbSize = sizeof(DriverInfoData);
					for (unsigned j = 0; (*pSetupDiEnumDriverInfo)(hDevInfo, &DeviceInfoData, nDriverType, j, &DriverInfoData); ++j)
					{
						pLog->WriteLn(
							_P(",\"%s\",\"%s\",\"%s\""),
							DriverInfoData.Description,
							DriverInfoData.MfgName,
							DriverInfoData.ProviderName
						);

						unsigned char * pBuffer = 0;
						unsigned nBufferSize = 0;
						for (;;)
						{
							DWORD nRequiredSize = 0;
							_F(SP_DRVINFO_DETAIL_DATA_) * pDriverInfoDetailData = static_cast<_F(SP_DRVINFO_DETAIL_DATA_) *>(static_cast<void *>(pBuffer));
							if ((pDriverInfoDetailData != 0) && (nBufferSize >= sizeof(DWORD)))
							{
								pDriverInfoDetailData->cbSize = sizeof(*pDriverInfoDetailData);
							}
							bool bResult = !!_F(SetupDiGetDriverInfoDetail)(hDevInfo, &DeviceInfoData, reinterpret_cast<_F(SP_DRVINFO_DATA_) *>(&DriverInfoData), pDriverInfoDetailData, nBufferSize, &nRequiredSize);
							if (bResult)
							{
								// This function returns true even
								// when the buffer and buffer size
								// are 0. How lame.
								if ((pBuffer != 0) || (nBufferSize > 0))
								{
									break;
								}
							}
							delete[] pBuffer;
							if ((!bResult) && (::GetLastError() != ERROR_INSUFFICIENT_BUFFER))
							{
								pBuffer = 0;
								break;
							}
							pBuffer = new unsigned char[nRequiredSize];
							nBufferSize = nRequiredSize;
						}
						if (pBuffer != 0)
						{
							//
							_F(SP_DRVINFO_DETAIL_DATA_) * pDriverInfoDetailData = static_cast<_F(SP_DRVINFO_DETAIL_DATA_) *>(static_cast<void *>(pBuffer));
							pDriverInfoDetailData->DrvDescription;
							delete[] pBuffer;
						}
					}

					SetupDiDestroyDriverInfoList(hDevInfo, &DeviceInfoData, nDriverType);
				}
			}
#endif
		}

		if ((::GetLastError() != NO_ERROR) && (::GetLastError() != ERROR_NO_MORE_ITEMS))
		{
			break;
		}

		bSuccess = true;
		break;
	}

	if (hDevInfo != 0)
	{
		(*pSetupDiDestroyDeviceInfoList)(hDevInfo);
	}

	if (hModule != 0)
	{
		::FreeLibrary(hModule);
	}

	if (hVersion != 0)
	{
		::FreeLibrary(hVersion);
	}

	if (!bSuccess)
	{
		pLog->WriteLn(_P("[Error 0x%08x enumerating hardware]"), ::GetLastError());
	}

#ifndef SYSINFO_USE_DRIVER_INFO
	(void)bIncludeDrivers;
#endif
}

//-----------------------------------------------------------------------------

void SysInfo_ReportVirtualMemory(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Available Memory (1 MB or larger)"));
	pLog->WriteLn(_P("Start    End      Size (MB)"));
	pLog->WriteLn(_P("-------- -------- ---------"));
	const char * pBase = 0;
	for (;;)
	{
		MEMORY_BASIC_INFORMATION mbi;
		SIZE_T nResult = ::VirtualQuery(pBase, &mbi, sizeof(mbi));
		if (nResult < sizeof(mbi))
		{
			break;
		}
		pBase = static_cast<const char *>(mbi.BaseAddress) + mbi.RegionSize;
		if ((mbi.State == MEM_FREE) && (mbi.RegionSize >= ONEMB))
		{
			pLog->WriteLn(_P("%p %p %9Iu"), mbi.BaseAddress, pBase - 1, mbi.RegionSize / ONEMB);
		}
		if (pBase < mbi.BaseAddress)
		{
			break;
		}
	}
}

//-----------------------------------------------------------------------------

unsigned SysInfo_GetCPUClockInMHz(void)
{
	return SysCaps_Get().dwCPUSpeedMHz;
}

void SysInfo_ReportCPU(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* CPU Info"));
	SYSTEM_INFO sysinf;
	::GetSystemInfo(&sysinf);
	pLog->WriteLn(_P("CPU architecture = %u"), sysinf.wProcessorArchitecture);
	pLog->WriteLn(_P("CPU architecture level = 0x%04x"), sysinf.wProcessorLevel);
	pLog->WriteLn(_P("CPU architecture revision = 0x%04x"), sysinf.wProcessorRevision);
	pLog->WriteLn(_P("CPU clock = %u MHz"), SysInfo_GetCPUClockInMHz());
	pLog->WriteLn(_P("CPU count = %u"), sysinf.dwNumberOfProcessors);
}

//-----------------------------------------------------------------------------

void SysInfo_ReportTemporaryFilesPath(SysInfoLog * pLog)
{
	pLog->NewLine();
	pLog->WriteLn(_P("* Temporary Directory Info"));
	char_type * pPath = 0;
	DWORD nSize = 0;
	for (;;) {
#pragma warning(suppress : 6387) // pPath shouldn't be NULL according to SAL for GetTempPath. But that's wrong.
		DWORD nResult = _F(::GetTempPath)(nSize, pPath);
		if (nResult == 0) {
			pLog->WriteLn(_P("[Error 0x%08x calling GetTempPath()]"), ::GetLastError());
			break;
		}
		if (nResult < nSize) {
			pLog->WriteLn(_P("System temporary directory = \"%s\""), pPath);
			break;
		}
		nSize = nResult + 1;
		delete[] pPath;
		pPath = new char_type[nSize];
	}
	delete[] pPath;
}

//-----------------------------------------------------------------------------
void SysInfo_ReportWinsock(SysInfoLog *pLog)
{
    DWORD buflen = 64*1024;
    BYTE *buffer = new BYTE[buflen];
    INT error = NO_ERROR;
    if(buffer)
    {
        int numProto = WSCEnumProtocols(NULL, (LPWSAPROTOCOL_INFOW)buffer, &buflen, &error );
        if(numProto  != SOCKET_ERROR) 
        {
            WSAPROTOCOL_INFOW *protoInfo = (WSAPROTOCOL_INFOW*)buffer;
            pLog->WriteLn(_P("Enumerating %d winsock catalog entries"),numProto);
            pLog->WriteLn(_P("---------------------------------------"));
            for(int i = 0; i < numProto; i++)
            {
                pLog->WriteLn(_P("Entry #[%d] AddressFamily %d Protocol %d SocketType %d  (Provider type %s). Description %s"),
                                i,
                                protoInfo->iAddressFamily,
                                protoInfo->iProtocol,
                                protoInfo->iSocketType,
                                ( protoInfo->ProtocolChain.ChainLen != 1) ? "Layered" : "Base",
                                protoInfo->szProtocol);
                protoInfo++;
            }
        }

    }
    delete [] buffer;
}
//-----------------------------------------------------------------------------

class SysInfoLog_DebugOut : public SysInfoLog
{
	protected:
		virtual void _Write(const char_type * pFmt, va_list Args);
};

void SysInfoLog_DebugOut::_Write(const char_type * pFmt, va_list Args)
{
	char_type z[256];
	vswprintf_s(z, _countof(z), pFmt, Args);
	::OutputDebugStringW(z);
}

//-----------------------------------------------------------------------------

};

//-----------------------------------------------------------------------------

void SysInfo_Init(void)
{
	SysInfo_Deinit();
	s_pSysInfo_Callbacks = new SYSINFO_CALLBACK_VECTOR;
}

void SysInfo_Deinit(void)
{
	delete s_pSysInfo_Callbacks;
	s_pSysInfo_Callbacks = 0;
}

void SysInfo_ReportAll(SysInfoLog * pLog)
{
	// OS version.
	SysInfo_ReportOperatingSystem(pLog);

	// CPU info.
	SysInfo_ReportCPU(pLog);

	// Memory info.
	SysInfo_ReportMemoryExtents(pLog);

	// Hard disk drive capacity.
	SysInfo_ReportHardDisks(pLog);

	// Temporary file path.
	SysInfo_ReportTemporaryFilesPath(pLog);

	// Loaded module info.
	SysInfo_ReportModules(pLog);

	// Virtual memory.
	SysInfo_ReportVirtualMemory(pLog);

	// Hardware devices.
	SysInfo_ReportHardware(pLog, true);

    // Winsock catalog
    SysInfo_ReportWinsock(pLog);

	// User-defined reports.
	for (SYSINFO_CALLBACK_VECTOR::const_iterator i = s_pSysInfo_Callbacks->begin(); i != s_pSysInfo_Callbacks->end(); ++i)
	{
		(*i->pCallback)(pLog, i->pContext);
	}
}

void SysInfo_RegisterReportCallback(SYSINFO_REPORTFUNC * pCallback, void * pContext)
{
	ASSERT_RETURN(s_pSysInfo_Callbacks != 0);
	SYSINFO_CALLBACK cb;
	cb.pCallback = pCallback;
	cb.pContext = pContext;
	s_pSysInfo_Callbacks->push_back(cb);
}


void SysInfo_VersionToString(char_type * pStr, unsigned nBufSize, ULONGLONG nVersion)
{
	local_sprintf(pStr, nBufSize, _P("%u.%u.%u.%u"),
		static_cast<unsigned>(nVersion >> 48) & 0xffff,
		static_cast<unsigned>(nVersion >> 32) & 0xffff,
		static_cast<unsigned>(nVersion >> 16) & 0xffff,
		static_cast<unsigned>(nVersion >>  0) & 0xffff
	);
}

void SysInfo_ReportAllToDebugOutput(void)
{
	SysInfoLog_DebugOut sil;
	SysInfo_ReportAll(&sil);
}
