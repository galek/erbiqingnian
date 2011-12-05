//----------------------------------------------------------------------------
// FILE: townportal.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "dungeon.h"
#include "game.h"
#include "globalindex.h"
#include "level.h"
#include "objects.h"
#include "room.h"
#include "room_path.h"
#include "s_message.h"
#include "c_message.h"
#include "s_townportal.h"
#include "units.h"
#include "unittag.h"
#include "warp.h"
#include "svrstd.h"
#include "s_partyCache.h"
#include "states.h"
#include "player.h"
#include "GameServer.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
static const int PORTAL_LIFETIME = GAME_TICKS_PER_SECOND * 60;
const BOOL gbTownportalAlwaysHasMenu = FALSE;

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TownPortalSpecInit(
	TOWN_PORTAL_SPEC *pTownPortal,
	UNIT *pPortal /*= NULL*/,
	UNIT *pOwner /*= NULL*/)
{	

	// set defaults
	pTownPortal->nLevelDefDest   = INVALID_LINK;
	pTownPortal->nLevelDefPortal = INVALID_LINK;
	pTownPortal->nLevelDepthDest = INVALID_LINK;
	pTownPortal->nLevelDepthPortal = INVALID_LINK;
	pTownPortal->nLevelAreaDest = INVALID_LINK;
	pTownPortal->nLevelAreaPortal = INVALID_LINK;
	pTownPortal->nPVPWorld = pOwner ? ( PlayerIsInPVPWorld( pOwner ) ? 1 : 0 ) : 0;

	pTownPortal->idGame = INVALID_ID;
	
	// if we have a portal, use it
	if (pPortal)
	{
		pTownPortal->idGame = UnitGetGameId( pPortal );
		pTownPortal->idPortal = UnitGetId( pPortal );
	
		LEVEL_TYPE tLevelTypeDest = TownPortalGetDestination( pPortal );
		if (AppIsHellgate())
		{
			pTownPortal->nLevelDefDest = tLevelTypeDest.nLevelDef;
			pTownPortal->nLevelDefPortal = UnitGetLevelDefinitionIndex( pPortal );
		}
		else if (AppIsTugboat())
		{		
			pTownPortal->nLevelDepthDest = tLevelTypeDest.nLevelDepth;
			pTownPortal->nLevelAreaDest = tLevelTypeDest.nLevelArea;
			pTownPortal->nPVPWorld = ( tLevelTypeDest.bPVPWorld != 0 );
			pTownPortal->nLevelDepthPortal = UnitGetLevelDepth( pPortal );
			pTownPortal->nLevelAreaPortal = UnitGetLevelAreaID( pPortal );		
		}
	}		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TownPortalGetReturnWarpArrivePosition( 
	UNIT *pReturnWarp, 
	VECTOR *pvFaceDirection, 
	VECTOR *pvPosition)
{
	ASSERTX_RETURN( pReturnWarp, "Expected unit" );
	ASSERTX_RETURN( ObjectIsPortalFromTown( pReturnWarp ), "Expected return warp" );
	ASSERTX_RETURN( pvFaceDirection, "Expected vector" );
	ASSERTX_RETURN( pvPosition, "Expected vector" );
	
	// get position
	*pvPosition = UnitGetPosition( pReturnWarp );

	// get face direction
	*pvFaceDirection = UnitGetFaceDirection( pReturnWarp, FALSE );		
		
	// offset position so it's in front of the warp a bit
	float flRadius = UnitGetCollisionRadius( pReturnWarp );
	VECTOR vOffset;
	VectorScale( vOffset, *pvFaceDirection, flRadius * 3.0f );
	VectorAdd( *pvPosition, *pvPosition, vOffset );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTownPortalGetDestinationLevelDefinition(	
	UNIT *pPortal)
{
	ASSERTX_RETINVALID( pPortal, "Expected portal" );
	return UnitGetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEF );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTownPortalGetDestinationLevelDepth(	
	UNIT *pPortal)
{
	ASSERTX_RETINVALID( pPortal, "Expected portal" );
	return UnitGetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEPTH );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTownPortalGetDestinationLevelArea(	
	UNIT *pPortal)
{
	ASSERTX_RETINVALID( pPortal, "Expected portal" );
	return UnitGetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_AREA );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sTownPortalGetDestinationPVPWorld(	
	UNIT *pPortal)
{
	ASSERTX_RETINVALID( pPortal, "Expected portal" );
	return UnitGetStat( pPortal, STATS_TOWN_PORTAL_DEST_PVPWORLD ) != 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL_TYPE TownPortalGetDestination(
	UNIT *pPortal)
{
	LEVEL_TYPE tLevelType;
	tLevelType.nLevelDef = sTownPortalGetDestinationLevelDefinition( pPortal );
	tLevelType.nLevelArea = sTownPortalGetDestinationLevelArea( pPortal );
	tLevelType.nLevelDepth = sTownPortalGetDestinationLevelDepth( pPortal ); 	
	tLevelType.bPVPWorld = sTownPortalGetDestinationPVPWorld( pPortal ); 	
	return tLevelType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownPortalCanUse(
	UNIT *pUnit,
	PGUID guidPortalOwner)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( guidPortalOwner != INVALID_GUID, "Expected guid" );
	
	if (AppIsHellgate())
	{
		// hellgate currently only allows players to use their own portals
		if (UnitGetGuid( pUnit ) == guidPortalOwner)
		{
			return TRUE;
		}
		return FALSE;
	}
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownPortalCanUseByGUID(
	UNIT *pUnit,
	PGUID guidPortalOwner)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( guidPortalOwner != INVALID_GUID, "Expected GUID" );
	BOOL bCanUse = TRUE;
		
	// hellgate units can only use their own portals
	if (AppIsHellgate())
	{			
		PGUID guid = UnitGetGuid( pUnit );	
		if (guid != guidPortalOwner)
		{
			bCanUse = FALSE;
		}
	}
	else
	{
		GAME* pGame = UnitGetGame( pUnit );
		PGUID guid = UnitGetGuid( pUnit );	
		if (guid != guidPortalOwner)
		{
			if( IS_SERVER( pGame ) )
			{

#if !ISVERSION(CLIENT_ONLY_VERSION)
				GAMECLIENT *pClient = UnitGetClient( pUnit );
				TOWN_PORTAL_SPEC tServerTownPortalSpec;

				GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
				ASSERT_RETFALSE(pServerContext);

				CHANNELID partyChannel = 
					ClientSystemGetParty( pServerContext->m_ClientSystem, pClient->m_idAppClient );
				if(partyChannel != INVALID_ID &&
					guidPortalOwner != INVALID_GUID &&
					!PartyCacheGetMemberTownPortal(
					pServerContext->m_PartyCache,
					partyChannel,
					guidPortalOwner,
					tServerTownPortalSpec)) 
				{
					bCanUse = FALSE;
				}
#endif
			}
			else
			{
#if !ISVERSION(SERVER_VERSION)

				bCanUse = c_PlayerFindPartyMember( guidPortalOwner ) != INVALID_INDEX;
#endif
			}
		}
	}
	
	return bCanUse;  
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownPortalCanUse(
	UNIT *pUnit,
	UNIT *pPortal)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pPortal, "Expected portal" );
	PGUID guidPortalOwner = UnitGetGUIDOwner( pPortal );
	return TownPortalCanUseByGUID( pUnit, guidPortalOwner );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TownPortalIsAllowed(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// see if the players current sublevel allows townportals
	LEVEL *pLevel = UnitGetLevel( pUnit );
	if (!pLevel || SubLevelAllowsTownPortals( pLevel, UnitGetSubLevelID( pUnit )) == FALSE)
	{
		return FALSE;
	}

	// check level
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( pLevel );
	if (pLevelDef->bDisableTownPortals == TRUE)
	{
		return FALSE;
	}

	// check for state explicitly forbidding creation of a town portal
	GAME *pGame = UnitGetGame( pUnit );
	if (UnitHasState( pGame, pUnit, STATE_TOWN_PORTAL_RESTRICTED ) == TRUE)
	{
		return FALSE;
	}

	return TRUE; // ok to create town portal

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsTownPortal(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsA( pUnit, UNITTYPE_PORTAL_TO_TOWN );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLinkOwnerToPortal(
	UNIT *pOwner,
	UNIT *pPortal)
{		
	ASSERTX_RETURN( pOwner, "Expected owner" );
	ASSERTX_RETURN( pPortal, "Expected portal" );

	// clear any old stats in favor of the new link (or lack of link)
	s_TownPortalClearSpecStats( pOwner );
		
	UNITID idPortal = UnitGetId( pPortal );
	UnitSetStat( pOwner, STATS_TOWN_PORTAL, idPortal );		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnlinkOwnerFromPortals(
	UNIT *pOwner)
{	
	
	if (pOwner)
	{
		// clear any old stats in favor of the new link (or lack of link)
		s_TownPortalClearSpecStats( pOwner );

		// clear stat
		UnitClearStat( pOwner, STATS_TOWN_PORTAL, 0 );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTownPortalDestroy( 
	UNIT *pPortal, 
	UNIT *pOwner)
{
	ASSERTX_RETURN( pPortal, "Expected portal" );
	
	// unlink owner from portal
	if (pOwner)
	{
		sUnlinkOwnerFromPortals( pOwner );
	}
	
	// destroy portal
	UnitFree( pPortal, UFF_SEND_TO_CLIENTS );
	
}

//----------------------------------------------------------------------------
enum TOWN_PORTAL_ACTION
{
	TPA_OPENING,
	TPA_CLOSING,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sSendTownPortalChanged( 
	UNIT *pOwner, 
	UNIT *pPortal, 
	TOWN_PORTAL_ACTION eAction)
{
	ASSERTX_RETURN( pOwner, "Expected owner" );
	ASSERTX_RETURN( UnitIsPlayer( pOwner ), "Expected player" );
	ASSERTX_RETURN( pPortal || eAction == TPA_CLOSING, "Expected portal" );
	GAME *pGame = UnitGetGame( pOwner );
	ASSERTX_RETURN( pGame, "Expected game" );
	
	// setup message
	MSG_SCMD_TOWN_PORTAL_CHANGED tMessage;
	tMessage.bIsOpening = (eAction == TPA_OPENING);
	TownPortalSpecInit( &tMessage.tTownPortal, pPortal, pOwner );

	// send the message	
	if( AppIsHellgate() )
	{
		GAMECLIENTID idClient = UnitGetClientId( pOwner );
		s_SendMessage( pGame, idClient, SCMD_TOWN_PORTAL_CHANGED, &tMessage );
	}
	else
	{
		tMessage.idPlayer = UnitGetGuid( pOwner );
		UnitGetName(pOwner, tMessage.szName, MAX_CHARACTER_NAME, 0);
		s_SendMessageToAll( pGame, SCMD_TOWN_PORTAL_CHANGED, &tMessage );
	}
	
	// update the chat server with this player information
	UpdateChatServerPlayerInfo( pOwner );

}
#endif
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sTownPortalsClose(
	UNIT *pOwner)
{
	ASSERTX_RETURN( pOwner, "Expected owner" );
	GAME *pGame = UnitGetGame( pOwner );

	// destroy their portal somewhere in the world (if any)
	UNITID idPortal = UnitGetStat( pOwner, STATS_TOWN_PORTAL );
	UNIT *pPortal = UnitGetById( pGame, idPortal );
	// tell clients the portal is gone (may be NULL if it is a return portal to another game)	
	sSendTownPortalChanged( pOwner, pPortal, TPA_CLOSING );
	if (pPortal)
	{
						
		// destroy it
		sTownPortalDestroy( pPortal, pOwner );		
	
	}
	else
	{
		
		// unlink from portals in other games, can't get a pointer here
		sUnlinkOwnerFromPortals( pOwner );

	}
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sFindTownPortalInLevel(
	PGUID guidOwner,
	LEVEL *pLevel)
{		
	// go through all rooms on this level
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
	
		// search objects here
		for (UNIT *pObject = pRoom->tUnitList.ppFirst[ TARGET_OBJECT ]; pObject; pObject = pObject->tRoomNode.pNext)
		{
		
			// compare owners
			if (sIsTownPortal( pObject ) == TRUE && UnitGetGUIDOwner( pObject ) == guidOwner)
			{
				return pObject;
			}
			
		}
		
	}
	
	return NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sFindTownPortal( 
	GAME *pGame,
	PGUID guidOwner, 
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( guidOwner != INVALID_GUID, "Expected guid" );

	if (pLevel)
	{
		return sFindTownPortalInLevel( guidOwner, pLevel );
	}
	else
	{
		for (LEVEL *pLevel = DungeonGetFirstPopulatedLevel( pGame ); 
			 pLevel; 
			 pLevel = DungeonGetNextPopulatedLevel( pLevel ))
		{
			UNIT *pPortal = sFindTownPortalInLevel( guidOwner, pLevel );
			if (pPortal)
			{
				return pPortal;
			}
		}
		
	}
	
	return NULL;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sIsGUIDConnected(
	GAME *pGame,
	PGUID guid)
{

	// check all connected clients (not just ingame clients)
	GAMECLIENT *pClient = NULL;
	while ((pClient = GameGetNextClient( pGame, pClient )) != NULL)	
	{
		UNIT *pPlayer = ClientGetPlayerUnit( pClient, TRUE );
		if (pPlayer)
		{
			if (UnitGetGuid( pPlayer ) == guid)
			{
				return pPlayer;
			}
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TownPortalsCloseByGUID(
	GAME *pGame,
	PGUID guidOwner,
	DWORD dwTownPortalCloseFlags)
{
	UNIT *pOwner = sIsGUIDConnected( pGame, guidOwner );

	// if we only close portals for players who are not in the game, check it now
	if (pOwner != NULL && 
		TESTBIT( dwTownPortalCloseFlags, TPCF_ONLY_WHEN_OWNER_NOT_CONNECTED_BIT ) == TRUE)
	{
		return;
	}
	
	UNIT *pPortal = sFindTownPortal( pGame, guidOwner, NULL );
	if (pPortal)
	{
		sTownPortalDestroy( pPortal, pOwner );	
	}	
	else
	{
		// unlink from portals in other games, can't get a pointer here
		sUnlinkOwnerFromPortals( pOwner );
	}
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPortalOwnerExitLimbo(
	GAME *pGame,
	UNIT *pOwner,
	UNIT *pUnitOther,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{

	// get their portal
	UNIT *pPortal = s_TownPortalGet( pOwner );
	ASSERTX_RETURN( pPortal, "Player coming out of limbo is supposed to have town portal but doesn't" );

	if (pPortal)
	{
		// the portal should be disabled
		LEVEL_TYPE tLevelTypeDest = TownPortalGetDestination( pPortal );
		if (AppIsHellgate())
		{
			ASSERTX( tLevelTypeDest.nLevelDef == INVALID_LINK, "Portal should not have destination, but does" );
		}
		else if (AppIsTugboat())
		{
			ASSERTX( tLevelTypeDest.nLevelArea == INVALID_LINK, "Portal should not have destination, but does" );
		}

		// destroy the portal
		sTownPortalDestroy( pPortal, pOwner );
	}
	else
	{
		// unlink from portals in other games, can't get a pointer here
		sUnlinkOwnerFromPortals( pOwner );
	}

	// remove hander
	UnitUnregisterEventHandler( pGame, pOwner, EVENT_EXIT_LIMBO, sPortalOwnerExitLimbo );
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
UNIT * s_TownPortalOpen(
	UNIT *pOwner)
{
	ASSERTX_RETNULL( pOwner, "Expected owner" );

	// if not allowed for this unit, forget it
	if (TownPortalIsAllowed( pOwner ) == FALSE)
	{
		return FALSE;
	}
				
	// close any existing town portals
	sTownPortalsClose( pOwner );

	// town portal is a party op, you need to be able to do it
	//	TODO: party dynamics have changed... this needs to as well...
	//if (UnitCanDoPartyOperations( pOwner ) == FALSE)
	//{
	//	return NULL;
	//}

	// create the portal object
	DWORD dwWarpCreateFlags = 0;
	SETBIT( dwWarpCreateFlags, WCF_CLEAR_FROM_WARPS);
	SETBIT( dwWarpCreateFlags, WCF_CLEAR_FROM_START_LOCATIONS );
	UNIT *pPortal = s_WarpCreatePortalAround( pOwner, GI_OBJECT_TOWN_PORTAL, dwWarpCreateFlags, pOwner );

	// if successfully opened, portal, tie up the other end
	if (pPortal)
	{
				
		// link our player to this portal
		sLinkOwnerToPortal( pOwner, pPortal );

		// what is the destination of this portal
		if (AppIsHellgate())
		{
			int nDestinationLevelDef = UnitGetReturnLevelDefinition( pOwner );
			UnitSetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEF, nDestinationLevelDef );
		}
		else if (AppIsTugboat())
		{
			//UNIT *pUltimateOwner = UnitGetUltimateOwner( pOwner );
			int nLevelAreaDestinationID( 0 );
			int nLevelAreaDepth( 0 );
			
			
			VECTOR vRespawnLocation;
			if( PlayerGetRespawnLocation( pOwner, KRESPAWN_TYPE_PRIMARY, nLevelAreaDestinationID, nLevelAreaDepth, vRespawnLocation, NULL ) )
			{
				UnitSetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_AREA, nLevelAreaDestinationID );	
 				UnitSetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEPTH, nLevelAreaDepth ); // depth of 0 as destination			
			}
			else
			{
				UnitSetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_AREA, LevelGetLevelAreaID( UnitGetLevel( pOwner ) ) );	
 				UnitSetStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEPTH, 0 ); // depth of 0 as destination			
			}

			UnitSetStat( pPortal, STATS_TOWN_PORTAL_DEST_PVPWORLD, PlayerIsInPVPWorld( pOwner ) );	
		}
		else
		{
			ASSERT_RETNULL( 0 );
		}
				
		// tell everyone about the portal
		sSendTownPortalChanged( pOwner, pPortal, TPA_OPENING );
												
	}
	
	// return the portal in the world
	return pPortal;
					
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_TownPortalGetSpec(
	UNIT *pUnit,
	TOWN_PORTAL_SPEC *pTownPortal)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pTownPortal, "Invalid params" );
	GAME *pGame = UnitGetGame( pUnit );
	PGUID guidOwner = UnitGetGuid( pUnit );

	// init spec
	TownPortalSpecInit( pTownPortal, NULL, NULL );
		
	// see if we have a town portal in this game and update spec
	UNIT *pPortal = sFindTownPortal( pGame, guidOwner, NULL );
	if (pPortal)
	{
		TownPortalSpecInit( pTownPortal, pPortal, pUnit );
		return TRUE;
	}

	// not in the world of this game, check our stats for a spec of one
	// in our reserve game when we switched instances to this game
	if (AppIsHellgate())
	{
		int nLevelDefDest = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION );
		if (nLevelDefDest != INVALID_LINK)
		{
			TownPortalSpecInit( pTownPortal, NULL, pUnit );
			pTownPortal->nLevelDefDest = nLevelDefDest;
			pTownPortal->nLevelDefPortal = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION );
			ASSERTX( pTownPortal->nLevelDefPortal != INVALID_LINK, "Town portal in reserve game is in an unknown level" );

			return TRUE;		
		}
	}
	else if (AppIsTugboat())
	{
		int nLevelDepthDest = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION );
		int nLevelAreaDest = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION_AREA );
		if (nLevelDepthDest != INVALID_LINK)
		{
			TownPortalSpecInit( pTownPortal, NULL, pUnit );

			pTownPortal->nLevelDepthDest = nLevelDepthDest;
			pTownPortal->nLevelAreaDest = nLevelAreaDest;
			pTownPortal->nPVPWorld = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_PVPWORLD );
			pTownPortal->nLevelDepthPortal = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION );
			pTownPortal->nLevelAreaPortal = UnitGetStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION_AREA );
			ASSERTX( pTownPortal->nLevelDepthPortal != INVALID_LINK, "Town portal in reserve game is in an unknown level" );

			return TRUE;		
		}	
	}
				 
	return FALSE;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TownPortalClearSpecStats(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UnitClearStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION, 0 );
	UnitClearStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION, 0 );	
	UnitClearStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION_AREA, 0 );
	UnitClearStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION_AREA, 0 );	
	UnitClearStat( pUnit, STATS_RESERVE_GAME_TOWN_PORTAL_PVPWORLD, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TownPortalFindAndLinkPlayerPortal(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	PGUID guidOwner = UnitGetGuid( pPlayer );
	
	UNIT *pPortal = sFindTownPortal( pGame, guidOwner, NULL );
	if (pPortal)
	{
		sLinkOwnerToPortal( pPlayer, pPortal );
	}
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_TownPortalGet(
	UNIT *pOwner)
{
	ASSERTX_RETNULL( pOwner, "Expected unit" );
	GAME *pGame = UnitGetGame( pOwner );
	
	// get the town portal linked to this unit (if any)
	UNITID idPortal = UnitGetStat( pOwner, STATS_TOWN_PORTAL );
	return UnitGetById( pGame, idPortal );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_TownPortalsClose(
	UNIT *pOwner)
{
	ASSERTX_RETURN( pOwner, "Expected owner" );
	GAME *pGame = UnitGetGame( pOwner );
		
	// get their portal
	UNIT *pPortal = s_TownPortalGet( pOwner );

	// close any portals on the client, pPortal may be NULL if there is a return portal to another game
	sSendTownPortalChanged( pOwner, pPortal, TPA_CLOSING );

	if (pPortal)
	{
		if (UnitIsInLimbo( pOwner ))
		{
			// disable the portal by removing its destination
			UnitClearStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEF, 0 );
			UnitClearStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_AREA, 0 );
			UnitClearStat( pPortal, STATS_TOWN_PORTAL_DEST_LEVEL_DEPTH, 0 );		
		}
	}

	// if the owner is using the portal (is in limbo) we will disable the portal
	// and destroy it when they finish arriving, otherwise we delete it
	if (UnitIsInLimbo( pOwner ))
	{
		// register event on owner to remove the portal, or unlink portals in other games, when they come out of limbo
		UnitRegisterEventHandler( pGame, pOwner, EVENT_EXIT_LIMBO, sPortalOwnerExitLimbo, NULL );			
	}
	else if (pPortal)
	{
		sTownPortalDestroy( pPortal, pOwner );
	}
	else
	{
		// unlink from portals in other games, can't get a pointer here
		sUnlinkOwnerFromPortals( pOwner );
	}
	if( AppIsTugboat() )
	{
		// update the chat server with this player information
		UpdateChatServerPlayerInfo( pOwner );

	}

}	
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveTownPortalRestriction(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	UnitClearFlag( pUnit, UNITFLAG_DONT_COLLIDE_TOWN_PORTALS );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTownPortalUnitCannotActivateForAWhile( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// set flag
	UnitSetFlag( pUnit, UNITFLAG_DONT_COLLIDE_TOWN_PORTALS );
	
	// register event to remove this flag in a little while
	DWORD dwDelay = GAME_TICKS_PER_SECOND * TOWN_PORTAL_RESTRICTED_TIME_IN_SECONDS;
	UnitRegisterEventTimed( pUnit, sRemoveTownPortalRestriction, NULL, dwDelay );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TownPortalEnter(
	UNIT *pUnit,
	UNITID idPortal)
{
	GAME *pGame = UnitGetGame( pUnit );
	
	// get the active warp
	UNITID idActiveWarp = UnitGetStat( pUnit, STATS_ACTIVE_WARP );

	// clear the active warp
	UnitClearStat( pUnit, STATS_ACTIVE_WARP, 0 );

	// clear colliding with town portals now
	UnitClearFlag( pUnit, UNITFLAG_DONT_COLLIDE_TOWN_PORTALS );

	// if we're warping somewhere do it
	if (idPortal != INVALID_ID && idPortal == idActiveWarp)
	{
		UNIT *pPortal = UnitGetById( pGame, idPortal );
		if (pPortal)
		{	
			PGUID guidPortalOwner = UnitGetGUIDOwner( pPortal );
			if (TownPortalCanUseByGUID( pUnit, guidPortalOwner ))
			{
				s_WarpEnterWarp( pUnit, pPortal );
			}
			
		}
			
	}
	else	
	{
	
		// entering an invalid portal means they cancelled, set this unit as can't
		// activate town portals for a short amount of time
		if (AppIsHellgate())
		{
			sTownPortalUnitCannotActivateForAWhile( pUnit );
		}
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCancelPortalReturn(
	UNIT *pUnit,
	BOOL bPopBack)
{		
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// put the player back from the warp
	if (bPopBack)
	{
		UNITID idReturnWarp = UnitGetStat( pUnit, STATS_ACTIVE_WARP );
		ASSERTX_RETURN( idReturnWarp != INVALID_ID, "No active return warp for player" );
		UNIT *pReturnWarp = UnitGetById( pGame, idReturnWarp );
		ASSERTX_RETURN( pReturnWarp, "Expected return warp" );
		
		// get where to pop back to
		VECTOR vFacing;
		VECTOR vPosition;
		TownPortalGetReturnWarpArrivePosition( 
			pReturnWarp, 
			&vFacing, 
			&vPosition);
		
		// face toward portal
		VectorSubtract( vFacing, UnitGetPosition( pReturnWarp ), vPosition );
				
		// pop back
		UnitWarp( 
			pUnit, 
			UnitGetRoom( pReturnWarp ),
			vPosition,
			vFacing,
			UnitGetUpDirection( pUnit ),
			0,
			TRUE);

	}
		
	// clear the active return warp
	UnitClearStat( pUnit, STATS_ACTIVE_WARP, 0 );

	// don't check for trigger for a little while so the player can walk away from the warp
	UnitAllowTriggers( pUnit, FALSE, 4.0f );

}

//----------------------------------------------------------------------------
// TODO: make sure this works for cancelling destination selection!
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_TownPortalValidateReturnDest(
	UNIT *pUnit,
	APPCLIENTID idAppClient,
	PGUID guidOwner, 
	const TOWN_PORTAL_SPEC & tTownPortalSpec)
{
	ASSERT_RETFALSE(pUnit);
	TOWN_PORTAL_SPEC tServerTownPortalSpec;

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETFALSE(pServerContext);

	//Verify I am partied with the portal owner.
	if(guidOwner != UnitGetGuid(pUnit))
	{
		CHANNELID partyChannel = 
			ClientSystemGetParty( pServerContext->m_ClientSystem, idAppClient );
		if(partyChannel != INVALID_ID &&
			guidOwner != INVALID_GUID &&
			!PartyCacheGetMemberTownPortal(
			pServerContext->m_PartyCache,
			partyChannel,
			guidOwner,
			tServerTownPortalSpec)) return FALSE;
	}
	else
	{//Get our own town portal spec, make sure it matches. 
//NOTE: We should probably do it by the chat server, as above.
//However, if we're partyless the partyCache does not have our data.
//So we'll do it directly, even though this means using two different systems
//Twice the breakage!
		s_TownPortalGetSpec( pUnit, &tServerTownPortalSpec );
	}
	if(!tServerTownPortalSpec.Equals(&tTownPortalSpec)) return FALSE;

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TownPortalReturnToPortalSpec(
	UNIT *pUnit,
	PGUID guidArrivePortalOwner,
	const TOWN_PORTAL_SPEC &tTownPortalSpec)
{

	// allow unit to trigger stuff again
	UnitAllowTriggers( pUnit, TRUE, 0.0f );

	// if there is a level selected warp to it (otherwise it was a cancel dest selection)
	if( (AppIsHellgate() && tTownPortalSpec.nLevelDefPortal == INVALID_LINK)
		||(AppIsTugboat()&& tTownPortalSpec.nLevelDepthPortal==INVALID_LINK))
	{
		sCancelPortalReturn(pUnit, FALSE);
	}
	else
	{//Set up a warp spec, then WarpToLevelOrInstance( pUnit, tSpec );
		// setup warp spec
		WARP_SPEC tSpec;
		if(AppIsHellgate())
		{
			tSpec.nLevelDefDest = tTownPortalSpec.nLevelDefPortal;
		}
		if(AppIsTugboat())
		{
			tSpec.nLevelAreaDest = tTownPortalSpec.nLevelAreaPortal;
			tSpec.nLevelDepthDest = tTownPortalSpec.nLevelDepthPortal;
			tSpec.nPVPWorld = tTownPortalSpec.nPVPWorld;
		}
		tSpec.guidArrivePortalOwner = guidArrivePortalOwner;
		tSpec.idGameOverride = tTownPortalSpec.idGame;

		// warp
		s_WarpToLevelOrInstance( pUnit, tSpec );
	}

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TownPortalReturnThrough(
	UNIT *pUnit,
	PGUID guidArrivePortalOwner,
	const TOWN_PORTAL_SPEC &tPortalSpec)
{

	// allow unit to trigger stuff again
	UnitAllowTriggers( pUnit, TRUE, 0.0f );

	// setup warp spec
	WARP_SPEC tWarpSpec;
	if (AppIsHellgate())
	{
	
		// if there is a level selected warp to it (otherwise it was a cancel dest selection)
		if (tPortalSpec.nLevelDefPortal != INVALID_LINK)
		{	
		
			// setup warp spec
			tWarpSpec.nLevelDefDest = tPortalSpec.nLevelDefPortal;
			tWarpSpec.guidArrivePortalOwner = guidArrivePortalOwner;
			tWarpSpec.idGameOverride = tPortalSpec.idGame;

		}

	}
	else
	{
	
		// if there is a level selected warp to it (otherwise it was a cancel dest selection)
		if (tPortalSpec.nLevelAreaPortal != INVALID_LINK)
		{	
		
			// setup warp spec
			tWarpSpec.guidArrivePortalOwner = guidArrivePortalOwner;
			tWarpSpec.nLevelAreaDest = tPortalSpec.nLevelAreaPortal;
			tWarpSpec.nLevelDepthDest = tPortalSpec.nLevelDepthPortal;
			tWarpSpec.nPVPWorld = tPortalSpec.nPVPWorld;
			tWarpSpec.idGameOverride = tPortalSpec.idGame;
						
		}
	
	}

	// we didn't warp
	if (tWarpSpec.nLevelDefDest != INVALID_LINK || tWarpSpec.nLevelAreaDest != INVALID_LINK)
	{
		if( AppIsTugboat()  )
		{
			UnitSetFlag( pUnit, UNITFLAG_RETURN_FROM_PORTAL );
		}
		s_WarpToLevelOrInstance( pUnit, tWarpSpec );	
	}
	else
	{
		sCancelPortalReturn( pUnit, FALSE );
	}
					
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_TownPortalGetArrivePosition(	
	GAME *pGame,
	PGUID guidOwner,
	LEVELID idLevel,
	ROOM **pRoomDest,
	VECTOR *pvPosition,
	VECTOR *pvDirection)
{
	ASSERTX_RETFALSE( guidOwner != INVALID_GUID, "Expected guid" );
	ASSERTX_RETFALSE( pRoomDest, "Expected room param" );
	ASSERTX_RETFALSE( pvPosition, "Expected position dest param" );
	ASSERTX_RETFALSE( pvDirection, "Expected facing dest param" );
		
	// get level
	LEVEL *pLevel = LevelGetByID( pGame, idLevel );
	ASSERTX_RETFALSE( pLevel, "Can't find level" );
	
	// find the town portal of owner
	UNIT *pPortal = sFindTownPortal( pGame, guidOwner, pLevel );
	if (pPortal)
	{
		*pRoomDest = WarpGetArrivePosition( pPortal, pvPosition, pvDirection, WD_FRONT );
		ASSERTX_RETFALSE( *pRoomDest, "Expected room" );
		return TRUE;			
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_TownPortalDisplaysMenu( 
	UNIT *pPlayer, 
	UNIT *pPortal)
{	

	// always display menu flag
	if (gbTownportalAlwaysHasMenu == TRUE)
	{
		return TRUE;
	}

	// hellgate never displays a menu
	if (AppIsHellgate())
	{
		return FALSE;
	}
		
	// multiplayer checks
	if (AppIsMultiplayer() &&
		!sIsTownPortal( pPortal ) )
	{
		return TRUE;		
	}
	
	return FALSE;  // don't display menu
		 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TownPortalsRestrictForUnit(
	UNIT *pUnit,
	BOOL bRestrict)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	if (bRestrict)
	{
		s_StateSet( pUnit, pUnit, STATE_TOWN_PORTAL_RESTRICTED, 0 );	
	}
	else
	{
		s_StateClear( pUnit, UnitGetId( pUnit), STATE_TOWN_PORTAL_RESTRICTED, 0 );	
	}
	
}
