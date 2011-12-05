//----------------------------------------------------------------------------
// definition_common.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _DEFINITION_COMMON_H_
#define _DEFINITION_COMMON_H_

#include "pakfile.h"

#define DEFINITION_FILE_EXTENSION "xml"
#define BLANK_STRING "None"

typedef void (PFN_DEFINITION_POSTASYNCHLOAD)(void *, struct EVENT_DATA *);

//-------------------------------------------------------------------------------------------------
#define DEFINITION_GROUPS_PER_LIB	32
enum {
	DEFINITION_GROUP_START_COMMON = 0,
	DEFINITION_GROUP_START_ENGINE = DEFINITION_GROUP_START_COMMON + DEFINITION_GROUPS_PER_LIB,
	DEFINITION_GROUP_START_SOUND  = DEFINITION_GROUP_START_ENGINE + DEFINITION_GROUPS_PER_LIB,
	DEFINITION_GROUP_START_GAME   = DEFINITION_GROUP_START_SOUND  + DEFINITION_GROUPS_PER_LIB,
	NUM_DEFINITION_GROUPS         = DEFINITION_GROUP_START_GAME   + DEFINITION_GROUPS_PER_LIB,
};

typedef int DEFINITION_GROUP_TYPE;

enum {
	DEFINITION_GROUP_GLOBAL = DEFINITION_GROUP_START_COMMON,
	DEFINITION_GROUP_CONFIG,
//	DEFINITION_GROUP_SERVERLIST,
//	DEFINITION_GROUP_SERVERLISTOVERRIDE,
	DEFINITION_GROUP_TEST,
};

enum DEFINITION_HEADER_FLAGS
{
	DHF_LOADED_BIT,
	DHF_EXISTS_BIT,
};

typedef struct DEFINITION_HEADER {
	char		pszName[ MAX_XML_STRING_LENGTH ];
	int			nId;
	DWORD		dwFlags;	// This is the same size as a BOOL
}DEFINITION_HEADER;

//-------------------------------------------------------------------------------------------------
void * DefinitionGetById  ( DEFINITION_GROUP_TYPE eType, int nId );
void * DefinitionGetByName( DEFINITION_GROUP_TYPE eType, const char * pszName, int nPriorityOverride = -1, BOOL bForceSyncLoad = FALSE, BOOL bForceIgnoreWarnIfMissing = FALSE );
int DefinitionGetIdByName( DEFINITION_GROUP_TYPE eType, const char * pszName, int nPriorityOverride = -1, BOOL bForceSyncLoad = FALSE, BOOL bForceIgnoreWarnIfMissing = FALSE );
void * DefinitionGetByFilename( DEFINITION_GROUP_TYPE eType, const char * pszName, int nPriorityOverride = -1, BOOL bForceSyncLoad = FALSE, BOOL bForceIgnoreWarnIfMissing = FALSE );
int DefinitionGetIdByFilename( DEFINITION_GROUP_TYPE eType, const char * pszName, int nPriorityOverride = -1, BOOL bForceSyncLoad = FALSE, BOOL bForceIgnoreWarnIfMissing = FALSE, PAK_ENUM ePakfile = PAK_DEFAULT );
const char * DefinitionGetNameById  ( DEFINITION_GROUP_TYPE eType, int nId );
void DefinitionAddProcessCallback(DEFINITION_GROUP_TYPE eType, int nId, PFN_DEFINITION_POSTASYNCHLOAD * fnPostLoad, struct EVENT_DATA * eventData);
int DefinitionGetFirstId( DEFINITION_GROUP_TYPE eType, BOOL bEvenIfNotLoaded = FALSE );
int DefinitionGetNextId( DEFINITION_GROUP_TYPE eType, int nId, BOOL bEvenIfNotLoaded = FALSE );

inline struct GLOBAL_DEFINITION * DefinitionGetGlobal(
	BOOL bDoNotAssert = FALSE)
{
	extern int gnGlobalDefinitionID;
	if (gnGlobalDefinitionID == INVALID_ID)
	{
		gnGlobalDefinitionID = DefinitionGetIdByName(DEFINITION_GROUP_GLOBAL, "default", -1, FALSE, !bDoNotAssert);
		if (gnGlobalDefinitionID == INVALID_ID)
		{
			return NULL;
		}
	}
	return (struct GLOBAL_DEFINITION *)DefinitionGetById(DEFINITION_GROUP_GLOBAL, gnGlobalDefinitionID);
}


inline struct CONFIG_DEFINITION* DefinitionGetConfig()
{
	extern int gnConfigDefinitionID;
	if ( gnConfigDefinitionID == INVALID_ID )
		gnConfigDefinitionID = DefinitionGetIdByName( DEFINITION_GROUP_CONFIG, "config", -1, FALSE, TRUE );
	return (struct CONFIG_DEFINITION*)DefinitionGetById(DEFINITION_GROUP_CONFIG, gnConfigDefinitionID );
}
/*
inline struct SERVERLIST_DEFINITION* DefinitionGetServerlist()
{
	extern int gnServerlistDefinitionID;
	if ( gnServerlistDefinitionID == INVALID_ID )
		gnServerlistDefinitionID = DefinitionGetIdByName( DEFINITION_GROUP_SERVERLIST, "serverlist" );
	return (struct SERVERLIST_DEFINITION*)DefinitionGetById(DEFINITION_GROUP_SERVERLIST, gnServerlistDefinitionID );
}

inline struct SERVERLISTOVERRIDE_DEFINITION* DefinitionGetServerlistOverride()
{
	extern int gnServerlistOverrideDefinitionID;
	if ( gnServerlistOverrideDefinitionID == INVALID_ID )
		gnServerlistOverrideDefinitionID = DefinitionGetIdByName( DEFINITION_GROUP_SERVERLISTOVERRIDE, "serverlistoverride", -1, FALSE, TRUE );
	return (struct SERVERLISTOVERRIDE_DEFINITION*)DefinitionGetById(DEFINITION_GROUP_SERVERLISTOVERRIDE, gnServerlistOverrideDefinitionID );
}
*/
#endif
