// FILE: s_townportal.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __FILE_S_TOWN_PORTAL_H_
#define __FILE_S_TOWN_PORTAL_H_

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct UNIT;
struct GAME;
struct ROOM;
struct LEVEL_TYPE;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
#define TOWN_PORTAL_RESTRICTED_TIME_IN_SECONDS (15)

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void TownPortalSpecInit(
	TOWN_PORTAL_SPEC *pTownPortal,
	UNIT *pPortal = NULL,
	UNIT *pOwner = NULL);

void TownPortalGetReturnWarpArrivePosition( 
	UNIT *pReturnWarp, 
	VECTOR *pvFaceDirection, 
	VECTOR *pvPosition);

LEVEL_TYPE TownPortalGetDestination(
	UNIT *pPortal);

BOOL TownPortalCanUseByGUID(
	UNIT *pUnit,
	PGUID guidPortalOwner);

BOOL TownPortalCanUse(
	UNIT *pUnit,
	UNIT *pPortal);
				
BOOL TownPortalIsAllowed(
	UNIT *pUnit);

//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
UNIT *s_TownPortalOpen(
	UNIT *pUnit);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TownPortalsClose(
	UNIT *pOwner);
#endif


//----------------------------------------------------------------------------
enum TOWN_PORTAL_CLOSE_FLAGS
{
	TPCF_ONLY_WHEN_OWNER_NOT_CONNECTED_BIT,		// only close when the owner of this portal isn't currently in the game
};

void s_TownPortalsCloseByGUID(
	GAME *pGame,
	PGUID guidOwner,
	DWORD dwTownPortalCloseFlags);		// see TOWN_PORTAL_CLOSE_FLAGS

BOOL s_TownPortalGetSpec(
	UNIT *pUnit,
	TOWN_PORTAL_SPEC *pTownPortal);

void s_TownPortalClearSpecStats(
	UNIT *pUnit);

void s_TownPortalFindAndLinkPlayerPortal(
	UNIT *pPlayer);
				
UNIT *s_TownPortalGet(
	UNIT *pUnit);
	
void s_TownPortalEnter(
	UNIT *pUnit,
	UNITID idPortal);

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_TownPortalValidateReturnDest(
	UNIT *pUnit,
	APPCLIENTID idAppClient,
	PGUID guidOwner, 
	const struct TOWN_PORTAL_SPEC & tTownPortalSpec);
#endif

void s_TownPortalReturnToPortalSpec(
	UNIT *pUnit,
	PGUID guidArrivePortalOwner,
	const struct TOWN_PORTAL_SPEC &tTownPortalSpec);
	
void s_TownPortalReturnThrough(
	UNIT *pUnit,
	PGUID guidArrivePortalOwner,
	const TOWN_PORTAL_SPEC &tPortalSpec);
	
BOOL s_TownPortalGetArrivePosition(
	GAME *pGame,
	PGUID guidOwner,
	LEVELID idLevel,
	ROOM **pRoomDest,
	VECTOR *pvPosition,
	VECTOR *pvDirection);

BOOL s_TownPortalDisplaysMenu( 
	UNIT *pPlayer, 
	UNIT *pPortal);
	
void s_TownPortalsRestrictForUnit(
	UNIT *pUnit,
	BOOL bRestrict);
	
//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern const BOOL gbTownportalAlwaysHasMenu;

#endif
