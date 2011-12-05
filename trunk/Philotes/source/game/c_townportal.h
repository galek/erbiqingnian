//----------------------------------------------------------------------------
// FILE: c_townportal.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __FILE_C_TOWN_PORTAL_H_
#define __FILE_C_TOWN_PORTAL_H_

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UI_COMPONENT;
enum UI_MSG_RETVAL;
enum UNIT_ITERATE_RESULT;
struct UNIT;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void c_TownPortalInitForApp(
	void);

void c_TownPortalFreeForApp(
	void);
		
void c_TownPortalOpened(
	PGUID guidOwner,
	const WCHAR *puszOwnerName,
	struct TOWN_PORTAL_SPEC *pPortalSpec );

void c_TownPortalClosed(
	PGUID owner,
	const WCHAR * wszOwnerName,
	struct TOWN_PORTAL_SPEC * portalSpec );

void c_TownPortalGetReturnWarpName(
	GAME *pGame,
	WCHAR *puszBuffer,
	int nBufferSize);

void c_TownPortalEnterUIOpen(
	UNITID idPortal);
	
void c_TownPortalUIOpen(
	UNITID idReturnWarp);

void c_TownPortalUIClose(
	BOOL bCancelled);

UNIT_ITERATE_RESULT c_TownPortalSetReturnPortalState(
	UNIT *pUnit,
	void *pCallbackData);

UI_MSG_RETVAL UITownPortalCancelButtonDown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UITownPortalButtonDown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam);
			
#endif