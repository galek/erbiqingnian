//----------------------------------------------------------------------------
// drlg.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _DRLG_H_
#define _DRLG_H_

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

#define MAX_NEARBY_ROOMS		100

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct GAME;

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct DRLG_PASS
{
	char		pszDrlgFileName[DEFAULT_INDEX_SIZE];
	int			nPathIndex;
	char		pszDRLGName[DEFAULT_INDEX_SIZE];
	char		pszLabel[DEFAULT_INDEX_SIZE];
	int			nMinSubs[2];
	int			nMaxSubs[2];
	int			nAttempts;
	BOOL		bReplaceAll;
	BOOL		bReplaceWithCollisionCheck;
	BOOL		bReplaceAndCheckOnce;
	BOOL		bMustPlace;
	BOOL		bWeighted;
	BOOL		bExitRule;
	BOOL		bDeadEndRule;
	int			nLoopAmount;
	char		pszLoopLabel[DEFAULT_INDEX_SIZE];
	BOOL		bAskQuests;
	int			nQuestPercent;
	int			nQuestTaskComplete;
	int			nThemeRunRule;
	int			nThemeSkipRule;
	SAFE_PTR(struct DRLG_LEVEL_DEFINITION*, pDrlgDef);
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL DRLGCreateLevel(
	GAME* game,
	struct LEVEL* level,
	DWORD dwSeed,
	UNITID idActivator);

void DRLGCopyConnectionsFromDefinition(
	GAME* game,
	struct ROOM* room);

void DRLGCreateRoomConnections(
	GAME* game,
	struct LEVEL* level,
	struct ROOM* room);

void DRLGCalculateVisibleRooms(
	GAME* game,
	struct LEVEL* level,
	ROOM* center_room);

void DRLGCreateAutoMap(
	GAME* game,
	ROOM* room);

void DRLGFreePathingMesh(
	GAME * game,
	ROOM * room);

void DRLGRoomGetGetLayoutOverride(
	const char * pszFileName,
	char * szName,
	char * szLayout);

const char * DRLGRoomGetGetPath(
	char * szName,
	char * szFilePath,
	int nPathIndex);

void DRLGRoomGetPath(
	const struct DRLG_ROOM *pDRLGRoom,
	const struct DRLG_PASS *def,
	char *szFilePath,
	char *szLayout,
	int nStringLength);

typedef void (* PFN_ROOM_LAYOUT_GROUP_ITERATOR)( 
	const struct ROOM_LAYOUT_GROUP *ptLayoutDef,
	void *pCallbackData);

void DRLGPassIterateLayouts( 
	const DRLG_PASS *ptDRLGPass, 
	PFN_ROOM_LAYOUT_GROUP_ITERATOR pfnCallback,
	void *pCallbackData);

void DRLGRoomIterateLayouts(
	const DRLG_ROOM *ptDRLGRoom,
	const DRLG_PASS *ptDRLGPass, 
	PFN_ROOM_LAYOUT_GROUP_ITERATOR pfnCallback,
	void *pCallbackData);
	
void DRLGGetPossibleMonsters(
	GAME *pGame,
	int nDRLG,
	int *pnMonsterClassBuffer,
	int nBufferSize,
	int *pnCurrentBufferCount);

#endif // _DRLG_H_