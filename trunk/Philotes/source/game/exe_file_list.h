#if !ISVERSION(SERVER_VERSION)

// HAS TO BE NULL-TERMINATED LIST

LPCTSTR listReleaseFiles_Hellgate_LauncherFiles[] = 
{
	_T("Language\\English\\Strings_Install.xls.uni"),
	_T("Language\\Japanese\\Strings_Install.xls.uni"),
	_T("Language\\Korean\\Strings_Install.xls.uni"),
	_T("Language\\Chinese_Simplified\\Strings_Install.xls.uni"),
	_T("Language\\Chinese_Traditional\\Strings_Install.xls.uni"),
	_T("Language\\French\\Strings_Install.xls.uni"),
	_T("Language\\Spanish\\Strings_Install.xls.uni"),
	_T("Language\\German\\Strings_Install.xls.uni"),
	_T("Language\\Italian\\Strings_Install.xls.uni"),
	_T("Language\\Polish\\Strings_Install.xls.uni"),
	_T("Language\\Czech\\Strings_Install.xls.uni"),
	_T("Language\\Hungarian\\Strings_Install.xls.uni"),
	_T("Language\\Russian\\Strings_Install.xls.uni"),
	_T("Language\\Thai\\Strings_Install.xls.uni"),	
	_T("Language\\Vietnamese\\Strings_Install.xls.uni"),		

	_T("Launcher.exe"),
	NULL
};

LPCTSTR listReleaseFiles_Hellgate[] = 
{
	_T("MP_x86\\hellgate_mp_dx9_x86.exe"),
	_T("MP_x86\\hellgate_mp_dx10_x86.exe"),
	_T("MP_x86\\dpvs.dll"),
	_T("MP_x86\\fmodex.dll"),
	_T("MP_x86\\granny2.dll"),
	_T("HellgateGDF.dll"),
	_T("MP_x86\\mss32.dll"),
	_T("MP_x86\\binkw32.dll"),

	_T("data\\ntlfl.cfg"),
	_T("data\\serverlist.xml"),
	NULL
};

LPCTSTR listReleaseFiles_Mythos[] = 
{
	_T("bin\\Mythos.exe"),
	_T("bin\\fmodex.dll"),
	_T("bin\\granny2.dll"),
	_T("data_mythos\\ntlfl.cfg"),
	_T("data_mythos\\serverlist.xml"),
	NULL
};

// Somebody add something for these
LPCTSTR listReleaseFiles64_Hellgate[] = 
{
	_T("MP_x64\\hellgate_mp_dx9_x64.exe"),
	_T("MP_x64\\hellgate_mp_dx10_x64.exe"),
	_T("MP_x64\\binkw64.dll"),
	_T("MP_x64\\dpvs.dll"),
	_T("MP_x64\\fmodex64.dll"),
	_T("MP_x64\\granny2_x64.dll"),
	_T("HellgateGDF.dll"),
	_T("MP_x64\\mss64.dll"),

	_T("data\\ntlfl.cfg"),
	_T("data\\serverlist.xml"),
	NULL
};

LPCTSTR listReleaseFiles64_Mythos[] = 
{
	_T("bin\\x64\\Mythos.exe"),
	_T("data_mythos\\ntlfl.cfg"),
	_T("data_mythos\\serverlist.xml"),
	NULL
};

#endif
