//----------------------------------------------------------------------------
// FILE: 
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __FILLPAK_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __FILLPAK_H_

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum FILLPAK_TYPE
{
	FILLPAK_DEFAULT,
	FILLPAK_SOUND,
	FILLPAK_LOCALIZED,
	FILLPAK_MIN,
	FILLPAK_ADVERT_DUMP,
};

//----------------------------------------------------------------------------
enum FILLPAK_SECTION_ENUM
{

};

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define FILLPAK_LOAD_WARDROBES 1
#define FILLPAK_LOAD_UNITDATA 1
#define FILLPAK_LOAD_ROOMS 1
#define FILLPAK_LOAD_DRLGS 1
#define FILLPAK_LOAD_DRLG_LAYOUTS 1
#define FILLPAK_LOAD_ENVIRONMENTS 1
#define FILLPAK_LOAD_STATES 1
#define FILLPAK_LOAD_SKILLS 1
#define FILLPAK_LOAD_AIS 1
#define FILLPAK_LOAD_EFFECTS 1
#define FILLPAK_LOAD_DEMOLEVELS 1
#define FILLPAK_LOAD_MELEEWEAPONS 1
#define FILLPAK_LOAD_SOUNDS 1


//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void FillPakInitForApp(
	void);

void FillPakFreeForApp(
	void);
	
BOOL FillPak( 
	FILLPAK_TYPE eType);

void FillPakLocalizedSetup(
	const char *pszSKU);

void FillPak_LoadStatesAll();
void FillPak_LoadGlobalMaterials();
void FillPak_LoadEffects();
void FillPak_LoadWardrobesAll();

void FillPakAddFilter(
	const char* filterString );
BOOL FillPakShouldLoad( 
	const char* key );

#if !ISVERSION(SERVER_VERSION)

BOOL FillPak_Execute(
	MSG_STRUCT* msg,
	BOOL& bExit);

NET_COMMAND_TABLE* FillPakClientGetRequestTable();

NET_COMMAND_TABLE* FillPakClientGetResponseTable();

#if ISVERSION(DEVELOPMENT)
BOOL FillPakSendFileToServer(
	LPCTSTR filepath,
	LPCTSTR filename,
	UINT32 filesize,
	UINT32 filesize_compressed,
	PAK_ENUM paktype,
	void* pBuffer,
	UINT8* hash,
	FILE_HEADER* pHeader);
#endif

BOOL FillPakClt_SendFinishedMsg();

#endif

#endif