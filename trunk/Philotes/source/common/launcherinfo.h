#pragma once

/* ***************
* I think this should be singleton...but not sure...
* And singleton is messy in C++ (esp. with our custom constructors...)
* Does this need singleton? Well, we should only have one open at a time...but its scope
* should be low...so....
* ...also, this is VERY sensitive to changes in the output file; not using full parser/lexer cause that seems overkill
*************** */
class LauncherInfo
{
public:
	LauncherInfo(/*BOOL bCreateOnNoExist = FALSE*/);
	~LauncherInfo(void);
	LauncherInfo* Init();

	int nDX10;
	BOOL success;
	BOOL bNoSave;
};

BOOL LauncherSetDX10(BOOL dx);
BOOL LauncherGetDX10();

BOOL SupportsDX10();
BOOL SupportsDX9();
HRESULT CleanupD3D();
