//----------------------------------------------------------------------------
// FILE: c_townportal.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_input.h"
#include "c_message.h"
#include "c_townportal.h"
#include "console.h"
#include "globalindex.h"
#include "objects.h"
#include "player.h" // also includes units.h; units.h includes game.h
#include "s_message.h"
#include "s_townportal.h"
#include "states.h"
#include "uix.h"
#include "uix_components.h"
#include "uix_priv.h"
#include "c_LevelAreasClient.h"

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum CTOWN_PORTAL_CONSTANTS
{
	MAX_TOWN_PORTALS = 64,	// arbitrary
};

//----------------------------------------------------------------------------
struct C_TOWN_PORTAL_INFO
{
	PGUID				Owner;
	WCHAR				OwnerName[MAX_CHARACTER_NAME];
	TOWN_PORTAL_SPEC	PortalSpec;
};

//----------------------------------------------------------------------------
struct CTOWN_PORTAL_GLOBALS
{
	UNITID idActiveWarp;
	
	// these are for the portals available to you to go through in town
	C_TOWN_PORTAL_INFO tPortal[ MAX_TOWN_PORTALS ];
	int nNumPortals;

	C_TOWN_PORTAL_INFO tAvailablePortal[ MAX_TOWN_PORTALS ];
	int nNumAvailablePortals;

	// this is for the current active portal that you are touching in the world
	C_TOWN_PORTAL_INFO tPortalWorld;
		
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static CTOWN_PORTAL_GLOBALS sgCTownPortalGlobals;

//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CTOWN_PORTAL_GLOBALS *sCTownPortalGlobalsGet(
	void)
{
	return &sgCTownPortalGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTownPortalDestinationInit(
	C_TOWN_PORTAL_INFO * pTownPortal)
{
	pTownPortal->Owner = INVALID_GUID;
	pTownPortal->OwnerName[0] = 0;
	pTownPortal->PortalSpec.Init();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sActiveWarpIsReturnWarp(
	void)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	GAME *pGame = AppGetCltGame();

	// get warp
	UNITID idPortal = pGlobals->idActiveWarp;	
	ASSERTX_RETFALSE( idPortal != INVALID_ID, "There is no active warp to query" );
	UNIT *pPortal = UnitGetById( pGame, idPortal );
	ASSERTX_RETFALSE( pPortal, "Expected portal" );
	
	if (ObjectIsPortalFromTown( pPortal ))
	{
		return TRUE;
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalInitForApp(
	void)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	
	pGlobals->idActiveWarp = INVALID_ID;
	pGlobals->nNumPortals = 0;
	pGlobals->nNumAvailablePortals = 0;
	for (int i = 0; i < MAX_TOWN_PORTALS; ++i)
	{
		C_TOWN_PORTAL_INFO * pTownPortal = &pGlobals->tPortal[ i ];
		sTownPortalDestinationInit( pTownPortal );
		pTownPortal = &pGlobals->tAvailablePortal[ i ];
		sTownPortalDestinationInit( pTownPortal );
	}
	sTownPortalDestinationInit( &pGlobals->tPortalWorld );	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalFreeForApp(
	void)
{
	c_TownPortalInitForApp();
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPortalFindIndex( 
	PGUID guidOwner)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	
	// go through all portals
	for (int i = 0; i < pGlobals->nNumPortals; ++i)
	{
		const C_TOWN_PORTAL_INFO *pTownPortal = &pGlobals->tPortal[ i ];
		if (pTownPortal->Owner == guidOwner)
		{
			return i;
		}
	}
	return INVALID_INDEX;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateAvailablePortals()
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	int nLevelAreaPlayer = INVALID_ID;

	UNIT *pPlayer = UIGetControlUnit();
	if( !pPlayer )
	{
		return;
	}
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if( pLevel )
	{
		nLevelAreaPlayer = LevelGetLevelAreaID( pLevel );
	}
	if( nLevelAreaPlayer == INVALID_ID )
	{
		nLevelAreaPlayer = UnitGetStat( pPlayer, STATS_LAST_LEVEL_AREA );
	}
	pGlobals->nNumAvailablePortals = 0;
	for( int i = 0; i < pGlobals->nNumPortals; i++ )
	{
		if( pGlobals->tPortal[i].PortalSpec.nPVPWorld == ( PlayerIsInPVPWorld( pPlayer) ? 1 : 0 ) &&
			TownPortalCanUseByGUID( pPlayer, pGlobals->tPortal[i].Owner))
		{
			pGlobals->tAvailablePortal[ pGlobals->nNumAvailablePortals++ ] = pGlobals->tPortal[ i ];
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePortal(
	int nIndex)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
		
	// clear the data here
	C_TOWN_PORTAL_INFO *pTownPortal = &pGlobals->tPortal[ nIndex ];
	sTownPortalDestinationInit( pTownPortal );
	
	// if there are more portals, put the last one in this slot
	if (pGlobals->nNumPortals > 1)
	{
		int nLastIndex = pGlobals->nNumPortals - 1;
		
		// put here
		pGlobals->tPortal[ nIndex ] = pGlobals->tPortal[ nLastIndex ];
		
		// clear the old entry just to be clean
		sTownPortalDestinationInit( &pGlobals->tPortal[ nLastIndex ] );
		
	}
	
	// we have one less portal now
	pGlobals->nNumPortals--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTownPortalOpenedOrClosed(
	void)
{
	GAME *pGame = AppGetCltGame();
	if( AppIsTugboat() )
	{
		sUpdateAvailablePortals();
	}
	// update any return portals in the world 
	UnitIterateUnits( pGame, c_TownPortalSetReturnPortalState, NULL );



}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalOpened(
	PGUID guidOwner,
	const WCHAR *puszOwnerName,
	struct TOWN_PORTAL_SPEC *pPortalSpec)
{
	ASSERTX_RETURN( guidOwner != INVALID_GUID, "Expected guid of owner of town portal" );
	ASSERTX_RETURN( puszOwnerName, "Expected owner name" );
	ASSERTX_RETURN( pPortalSpec, "Expected portal spec" );
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected player unit" );

	// ignore town portals that we can't use
	if (TownPortalCanUseByGUID( pPlayer, guidOwner ) == FALSE)
	{
		return;
	}
	
	// you can only have one portal for a person at a time
	int nIndex = sPortalFindIndex( guidOwner );
	BOOL bError = FALSE;
	while (nIndex != INVALID_INDEX)
	{
		
		// flag an error
		bError = TRUE;
		
		// remove and try to go on
		sRemovePortal( nIndex );
		
		// look for more
		nIndex = sPortalFindIndex( guidOwner );
		
	}
	ASSERTX( bError == FALSE, "Opening a town portal before the old one has gone away" );
			
	// add portal
	ASSERTX_RETURN( pGlobals->nNumPortals < MAX_TOWN_PORTALS - 1, "Too many town portals available on client" );
	C_TOWN_PORTAL_INFO * pNewTownPortal = &pGlobals->tPortal[ pGlobals->nNumPortals++ ];
	pNewTownPortal->Owner = guidOwner;
	PStrCopy(pNewTownPortal->OwnerName, puszOwnerName,MAX_CHARACTER_NAME);
	pNewTownPortal->PortalSpec.Set(pPortalSpec);

	// do common open/close stuff
	sTownPortalOpenedOrClosed();		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalClosed(
	PGUID owner,
	const WCHAR * wszOwnerName,
	TOWN_PORTAL_SPEC * portalSpec )
{
	ASSERT_RETURN(owner!=INVALID_GUID);
	ASSERT_RETURN(wszOwnerName);
	ASSERT_RETURN(portalSpec);

	// find portal entry
	int nIndex = sPortalFindIndex( owner );
	if (nIndex != INVALID_INDEX)
	{
	
		// remove portal
		sRemovePortal( nIndex );

		// do common open/close stuff
		sTownPortalOpenedOrClosed();

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSnapToFrontOfPortal( 
	UNITID idWarp)
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPortal = UnitGetById( pGame, idWarp );
	ASSERTX_RETURN( pPortal, "Unable to find portal on client" );
	
	// get control unit
	UNIT *pUnit = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pUnit, "Expected unit" );
		
	// get position to warp to
	VECTOR vFacing;
	VECTOR vPosition;
	TownPortalGetReturnWarpArrivePosition( pPortal, &vFacing, &vPosition);
		
	// face toward portal
	VectorSubtract( vFacing, UnitGetPosition( pPortal ), vPosition );
	VectorNormalize( vFacing );
	
	// pop back
	UnitWarp( 
		pUnit, 
		UnitGetRoom( pPortal ),
		vPosition,
		vFacing,
		UnitGetUpDirection( pUnit ),
		0,
		FALSE);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSelectDestination(
	const C_TOWN_PORTAL_INFO * pTownPortal)
{
	
	// setup message
	MSG_CCMD_SELECT_RETURN_DEST tMessage;
	tMessage.guidOwner = INVALID_GUID;
	
	if (pTownPortal)
	{
		tMessage.guidOwner = pTownPortal->Owner;
		tMessage.tTownPortalSpec.Set(&pTownPortal->PortalSpec);		
	}
			
	// send message
	c_SendMessage( CCMD_SELECT_RETURN_DEST, &tMessage );

	// when selecting no dest (cancel) warp back in front of the warp
	//if (pTownPortal == NULL)
	//{
	//	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	//	sSnapToFrontOfPortal( pGlobals->idActiveWarp );
	//}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEnterTownPortal(
	UNITID idPortal)
{
	MSG_CCMD_ENTER_TOWN_PORTAL tMessage;
	tMessage.idPortal = idPortal;
			
	// send message
	c_SendMessage( CCMD_ENTER_TOWN_PORTAL, &tMessage );
	
	if( AppIsHellgate() )
	{
		// display message about why they can't enter any portals for a while
		if (idPortal == INVALID_ID)
		{
		
			// display message
			const WCHAR *puszFormat = GlobalStringGet( GS_TOWN_PORTALS_RESTRICTED );
			ConsoleString( CONSOLE_SYSTEM_COLOR, puszFormat, TOWN_PORTAL_RESTRICTED_TIME_IN_SECONDS );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTownPortalGetOppositeLevelName(
	C_TOWN_PORTAL_INFO * pTownPortal,
	WCHAR *puszBuffer,
	int nBufferSize,
	BOOL bDisplayOwner)
{
	ASSERTX_RETURN( pTownPortal, "Expected town portal" );
	ASSERTX_RETURN( puszBuffer && nBufferSize > 0, "Expected buffer" );
	GAME *pGame = AppGetCltGame();
		
	// init name
	puszBuffer[ 0 ] = 0;

	// get player
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer)
	{
		const LEVEL *pLevel = UnitGetLevel( pPlayer );
		const LEVEL_DEFINITION *pLevelDefPortal = NULL;
					
		if (AppIsHellgate())
		{
			int nLevelDefPlayer = LevelGetDefinitionIndex( pLevel );
			
			// if the player is on the same level as the portal get the dest level, otherwise
			// the player is on the dest level so get the level the portal is on
			if (nLevelDefPlayer == pTownPortal->PortalSpec.nLevelDefPortal)
			{
				pLevelDefPortal = LevelDefinitionGet( pTownPortal->PortalSpec.nLevelDefDest );
			}
			else
			{
				ASSERTX( nLevelDefPlayer == pTownPortal->PortalSpec.nLevelDefDest, "Player expected to be on destination level of town portal" );
				pLevelDefPortal = LevelDefinitionGet( pTownPortal->PortalSpec.nLevelDefPortal );
			}
		}
		else if (AppIsTugboat())
		{		
			int nLevelDepthPlayer = LevelGetDepth( pLevel );
			int nLevelAreaPlayer = LevelGetLevelAreaID( pLevel );
			
			// if the player is on the same level as the portal get the dest level, otherwise
			// the player is on the dest level so get the level the portal is on
			if (nLevelDepthPlayer == pTownPortal->PortalSpec.nLevelDepthPortal &&
				nLevelAreaPlayer == pTownPortal->PortalSpec.nLevelAreaPortal &&
				LevelGetPVPWorld( pLevel ) == ( pTownPortal->PortalSpec.nPVPWorld != 0 ) )
			{
				pLevelDefPortal = LevelDefinitionGet( LevelDefinitionGetRandom( NULL, pTownPortal->PortalSpec.nLevelAreaDest, pTownPortal->PortalSpec.nLevelDepthDest ) );
			}
			else
			{
				ASSERTX( nLevelDepthPlayer == pTownPortal->PortalSpec.nLevelDepthDest, "Player expected to be on destination level of town portal" );
				pLevelDefPortal = LevelDefinitionGet( LevelDefinitionGetRandom( NULL, pTownPortal->PortalSpec.nLevelAreaPortal, pTownPortal->PortalSpec.nLevelDepthPortal ) );
			}
			
		}
		
		// must have a level definition at this point
		ASSERTX_RETURN( pLevelDefPortal, "Expected destination level definition" );
				
		// first copy portal location
		if( AppIsHellgate() )
		{
			PStrCopy( puszBuffer, LevelDefinitionGetDisplayName( pLevelDefPortal ), nBufferSize );
		}
		else
		{

			//WCHAR puszAreaName[ 1024 ];
			//MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( msg->nLevelArea, &puszAreaName[0], 1024 );

			int nAreaID = LevelGetLevelAreaID( pLevel );
			if( nAreaID != INVALID_ID )
			{		
				MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nAreaID, LevelGetDepth( pLevel ), &puszBuffer[0], nBufferSize );

				//PStrCopy( puszBuffer, StringTableGetStringByIndex( nStringID ), nBufferSize );
			}
			else
			{
				PStrCopy( puszBuffer, LevelDefinitionGetDisplayName( pLevelDefPortal ), nBufferSize );
			}
		}
		
		// append who's portal it is after the name
		if (bDisplayOwner && pTownPortal->OwnerName[ 0 ] != 0)
		{
			PStrCat( puszBuffer, L" (", nBufferSize );
			PStrCat( puszBuffer, pTownPortal->OwnerName, nBufferSize );
			PStrCat( puszBuffer, L")", nBufferSize );
		}
		
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOpenSelectDestUI(
	C_TOWN_PORTAL_INFO * pTownPortal,
	int nNumPortals)
{
	ASSERTX_RETURN( pTownPortal, "Expected portals" );
	GAME *pGame = AppGetCltGame();
	
	// stop all movement
	c_PlayerClearAllMovement( pGame );
	
	// we will display the owner of the portal when there is more than one
	BOOL bDisplayOwner = FALSE;
//	if (nNumPortals > 1)
	{
		bDisplayOwner = TRUE;
	}
		
	// bring up the window
	UI_COMPONENT_ENUM eComp = UICOMP_TOWN_PORTAL_DIALOG;
	UI_COMPONENT *pComponent = UIComponentGetByEnum(eComp);
	ASSERTX_RETURN( pComponent, "Unable to find town portal dialog" );
	UIComponentSetActive( pComponent, TRUE );
	if( AppIsTugboat() )
	{
		pComponent = UIComponentGetByEnum(UICOMP_TOWN_PORTAL_PANEL);
	}
	// populate the window with the choices
	UI_COMPONENT *pChild = pComponent->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_dwParam != 0)
		{
			if ( (int)pChild->m_dwParam <= nNumPortals)
			{
				if (pChild->m_eComponentType == UITYPE_LABEL)
				{
					const int MAX_BUFFER = 1024;
					WCHAR uszBuffer[ MAX_BUFFER ];
					sTownPortalGetOppositeLevelName( &pTownPortal[ pChild->m_dwParam - 1 ], uszBuffer, MAX_BUFFER, bDisplayOwner );
					UILabelSetText( pChild, uszBuffer );
				}
				UIComponentSetActive( pChild, TRUE );
			}
			else
			{
				UIComponentSetVisible( pChild, FALSE );
			}

		}
		pChild = pChild->m_pNextSibling;
	}

	// turn on the mouse
#ifdef DRB_3RD_PERSON
	UISetInterfaceActive( TRUE );
#endif

	// if no destinations, just select nothing
	if (nNumPortals == 0)
	{
		c_TownPortalUIClose( TRUE );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalUIClose(
	BOOL bCancelled)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_TOWN_PORTAL_DIALOG);
	ASSERTX_RETURN( pComponent, "Unable to find town portal dialog" );

	UIComponentSetVisible(pComponent, FALSE);
	
#ifdef DRB_3RD_PERSON
	UISetInterfaceActive( FALSE );
#endif

	// if cancelled, tell the sever
	if (bCancelled == TRUE)
	{
		if (sActiveWarpIsReturnWarp())
		{
			sSelectDestination( NULL );
		}
		else
		{
			sEnterTownPortal( INVALID_ID );
		}
	}

	// clear our active warp
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	pGlobals->idActiveWarp = INVALID_ID;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalEnterUIOpen(
	UNITID idPortal)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();

	// save the active warp
	pGlobals->idActiveWarp = idPortal;

	// save portal spec of this portal
	GAME *pGame = AppGetCltGame();
	UNIT *pPortal = UnitGetById( pGame, idPortal );
	if( pPortal )
	{
		pGlobals->tPortalWorld.Owner = UnitGetGUIDOwner( pPortal );
		TownPortalSpecInit( &pGlobals->tPortalWorld.PortalSpec, pPortal, NULL );
	}

	if (AppIsTugboat())
	{
		UI_TB_HideModalDialogs();
	}
	
	// bring up the UI
	sOpenSelectDestUI( &pGlobals->tPortalWorld, 1 );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalUIOpen(
	UNITID idReturnWarp)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();

	// save the active warp
	pGlobals->idActiveWarp = idReturnWarp;

	// hide all ui
	if (AppIsTugboat())
	{
		
		UI_TB_HideModalDialogs();
	}
	if( AppIsTugboat() )
	{
		sUpdateAvailablePortals();
	}
		
	// if there is only one destination available just pick it
	//if (pGlobals->nNumPortals == 1)
	//{
	//	sSelectDestination( &pGlobals->tPortal[ 0 ] );
	//}
	//else
	{
		if( AppIsHellgate() )
		{
			// bring up the selection ui
			sOpenSelectDestUI( pGlobals->tPortal, pGlobals->nNumPortals );
		}
		else
		{
			sOpenSelectDestUI( pGlobals->tAvailablePortal, pGlobals->nNumAvailablePortals );
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_TownPortalGetReturnWarpName(
	GAME *pGame,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
	
	if( AppIsTugboat() )
	{
		sUpdateAvailablePortals();
	}
	int nNumPortals = AppIsHellgate() ? pGlobals->nNumPortals : pGlobals->nNumAvailablePortals;
	if (nNumPortals > 0)
	{

		// if there are more options available, we show it with a symbol
		if (nNumPortals > 1)
		{
			// we could maybe localize this text display if we wanted to
			PStrCat( puszBuffer, L"[ ... ]", nBufferSize );		
		}
		else
		{
			const C_TOWN_PORTAL_INFO * pTownPortal = AppIsHellgate() ? &pGlobals->tPortal[ 0 ] : &pGlobals->tAvailablePortal[ 0 ];
			const LEVEL_DEFINITION *pLevelDef = NULL;
			if (AppIsHellgate())
			{
				pLevelDef = LevelDefinitionGet( pTownPortal->PortalSpec.nLevelDefPortal );
				ASSERTX_RETURN( pLevelDef, "Expected level definition" );
				PStrCat( puszBuffer, LevelDefinitionGetDisplayName( pLevelDef ), nBufferSize );		
			}
			else
			{
				
				
				if( pTownPortal->PortalSpec.nLevelAreaPortal != INVALID_ID )
				{
					MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( pTownPortal->PortalSpec.nLevelAreaPortal,
												 pTownPortal->PortalSpec.nLevelDepthPortal,
												 &puszBuffer[0],
												 nBufferSize );
				}
				else
				{
					pLevelDef = LevelDefinitionGet( LevelDefinitionGetRandom( NULL, pTownPortal->PortalSpec.nLevelAreaPortal, pTownPortal->PortalSpec.nLevelDepthPortal ) );
					ASSERTX_RETURN( pLevelDef, "Expected level definition" );
					PStrCat( puszBuffer, LevelDefinitionGetDisplayName( pLevelDef ), nBufferSize );		

				}
			}
			
		}
						
	}
	else if (AppIsTugboat())
	{
		PStrPrintf( puszBuffer, nBufferSize, GlobalStringGet( GS_PORTAL ) );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT_ITERATE_RESULT c_TownPortalSetReturnPortalState(
	UNIT *pPortal,
	void *pCallbackData)
{
	ASSERTX_RETVAL( pPortal, UIR_CONTINUE, "Expected unit" );
	ASSERTX_RETVAL( pCallbackData == NULL, UIR_CONTINUE, "Param is only here so it can be used to iterate units" );

	// only care about return warp portals	
	if (ObjectIsPortalFromTown( pPortal ))
	{

		// set state based on if we have any return portal destinations available 
		CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();

		if( AppIsTugboat() )
		{
			sUpdateAvailablePortals();
		}

		int nNumPortals = AppIsHellgate() ? pGlobals->nNumPortals : pGlobals->nNumAvailablePortals;
		BOOL bVisible = nNumPortals != 0;

		/*if( AppIsTugboat() )
		{
			UNIT *pPlayer = UIGetControlUnit();

			BOOL bThisArea = FALSE;
			for( int i = 0; i < nNumPortals; i++ )
			{
				if( pGlobals->tPortal[i].PortalSpec.nLevelAreaPortal == LevelGetLevelAreaID( UnitGetLevel( pPortal ) ) &&
					( pGlobals->tPortal[i].PortalSpec.nPVPWorld != 0 ) == PlayerIsInPVPWorld( pPlayer ) )
				{
					bThisArea = TRUE;
				}
			}
			if( !bThisArea )
			{
				bVisible = FALSE;
			}
			//c_UnitSetNoDraw( pPortal, !bVisible );

		}*/
		if (bVisible == FALSE)
		{
			c_StateSet( pPortal, pPortal, STATE_HIDDEN, 0, 0, INVALID_LINK );
		}
		else
		{
			c_StateClear( pPortal, INVALID_ID, STATE_HIDDEN, 0, TRUE );
		}			
	}
	
	// continue iterating (if we are anyway)
	return UIR_CONTINUE;
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITownPortalButtonDown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (ResultIsHandled(UIButtonOnButtonDown( pComponent, nMessage, wParam, lParam )))
	{
		int nIndex = (int)pComponent->m_dwParam - 1;	
		CTOWN_PORTAL_GLOBALS *pGlobals = sCTownPortalGlobalsGet();
					
		if (sActiveWarpIsReturnWarp())
		{
			// tell server to go here				
			const C_TOWN_PORTAL_INFO *pTownPortal = AppIsHellgate() ? &pGlobals->tPortal[ nIndex ] : &pGlobals->tAvailablePortal[ nIndex ];
			sSelectDestination( pTownPortal );
		}
		else
		{
			// tell server to go here
			sEnterTownPortal( pGlobals->idActiveWarp );
		}

		
		// close UI
		c_TownPortalUIClose( FALSE );
		return UIMSG_RET_HANDLED_END_PROCESS;
		
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITownPortalCancelButtonDown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (ResultIsHandled(UIButtonOnButtonDown( pComponent, nMessage, wParam, lParam )))
	{
		c_TownPortalUIClose( TRUE );
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

#endif //!SERVER_VERSION

