//----------------------------------------------------------------------------
// warp.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "definition.h"
#include "config.h"
#include "gameconfig.h"
#include "minitown.h"
#include "clients.h"
#include "unit_priv.h"
#include "player.h"
#include "events.h"
#include "level.h"
#include "objects.h"
#include "openlevel.h"
#include "s_message.h"
#include "s_network.h"
#include "c_message.h"
#include "filepaths.h"
#include "s_townportal.h"
#include "globalindex.h"
#include "s_adventure.h"
#include "s_partyCache.h"
#include "GameChatCommunication.h"
#include "warp.h"
#include "Act.h"
#include "room_path.h"
#include "svrstd.h"
#include "GameServer.h"
#include "dungeon.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "LevelAreas.h"
#include "guidcounters.h"
#include "quests.h"
#include "s_quests.h"
#include "faction.h"
#include "gamelist.h"
#include "LevelAreaLinker.h"
#include "states.h"
#include "waypoint.h"
#include "uix_priv.h"
#include "gamevariant.h"
#include "units.h"
#include "pets.h"
#include "combat.h"
#include "gameglobals.h"
#include "party.h"
#include "levelareas.h"

#if ISVERSION(SERVER_VERSION)
#include "warp.cpp.tmh"
#include <winperf.h>
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "dbunit.h"
#endif

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
const float OUT_OF_WARP_DISTANCE = 3.5f;
const float OUT_OF_WARP_DISTANCE_TUGBOAT = 1.0f;

static const float MIN_CREATE_PORTAL_DISTANCE = 5.0f;
static const float MAX_CREATE_PORTAL_DISTANCE = 40.0f;

static const int PORTAL_CREATE_NUM_RINGS = 4;
static const float PORTAL_CREATE_RING_INCREMENT = 2.0f;

static const float MIN_CREATE_PORTAL_DISTANCE_MYTHOS = 2.0f;
static const float MAX_CREATE_PORTAL_DISTANCE_MYTHOS = 20.0f;

static const int PORTAL_CREATE_NUM_RINGS_MYTHOS = 8;
static const float PORTAL_CREATE_RING_INCREMENT_MYTHOS = 1.0f;

static const float WARP_MIN_DISTANCE_FROM_WARPS = 8.0f;
static const float WARP_MIN_DISTANCE_FROM_START_LOCS = 5.0f;

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsInstanceChangeRequired(	
	const LEVEL_DEFINITION *ptLevelDefDest,
	const LEVEL_DEFINITION *ptLevelDefLeaving)
{
	// check debug flag
	if (AppCommonGetOneInstance())
	{
		return FALSE;
	}

	// single player never requires instance change
	if (AppIsHellgate() &&
		( !AppTestDebugFlag(ADF_SP_USE_INSTANCE) && AppGetType() == APP_TYPE_SINGLE_PLAYER ) )
	{
		return FALSE;
	}

	ASSERT_RETFALSE(ptLevelDefDest);
	ASSERT_RETTRUE(ptLevelDefLeaving);
	// are the level types different
	// if we are switching level types, and the destination is a standard instanced level,
	// we require an instance change if the leveltypes are different ( from town to instance )
	// but instance to instance remains the same
	switch( ptLevelDefDest->eSrvLevelType )
	{
		case SRVLEVELTYPE_DEFAULT :								// regular instanced adventuring level
		default :
			return ptLevelDefDest->eSrvLevelType != ptLevelDefLeaving->eSrvLevelType;
			
		// if the destination is ever a shared space though, or a tutorial, which is special case,
		// we ALWAYS switch instances.
		case SRVLEVELTYPE_MINITOWN :							// minitown shared instance for server
		case SRVLEVELTYPE_OPENWORLD :							// open world shared instance for server
		case SRVLEVELTYPE_BIGTOWN :								// bigtown
		case SRVLEVELTYPE_CUSTOM :
		case SRVLEVELTYPE_PVP_PRIVATE :
		case SRVLEVELTYPE_PVP_CTF :
		case SRVLEVELTYPE_TUTORIAL :							// tutorial forces separate instance per player
			{
				if( AppIsTugboat() )
				{
					return( ptLevelDefDest != ptLevelDefLeaving );
				}
				else
				{
					return TRUE;
				}

			}
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sClientWarpSendLoadingScreen(
	GAME * game,
	GAMECLIENTID idClient,
	const WARP_SPEC & tWarpSpec)
{
	s_LevelSendStartLoad(
		game,
		idClient,
		tWarpSpec.nLevelDefDest,
		tWarpSpec.nLevelDepthDest,
		tWarpSpec.nLevelAreaDest,
		INVALID_LINK);		
	return TRUE;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientWarpReserveOrDestroyGame(
	GAME * game,
	APPCLIENTID idAppClient,
	GAMECLIENT *pGameClient,
	int nAreaID,
	BOOL bCanReserveGame)
{

	// if not using instances, just the game
	if (AppIsHellgate() && !AppTestDebugFlag(ADF_SP_USE_INSTANCE) && AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);
	}
	else
	{
	
		// when you're leaving a dungeon game, we always want to reserve the game that we're
		// leaving so that we can get back to it.  Imagine the situation of two players in
		// a party, both in a dungeon, when the first player returns to town, we need
		// to reserve this game for him, because while in town, the other player might quit
		// the game, and we don't want it to go away

		// destroy or reserve game (or just leave it sitting around)
		switch (GameGetSubType(game))
		{

			//----------------------------------------------------------------------------	
			case GAME_SUBTYPE_DUNGEON:
			{
				// reserve this game for us so we can get back to it
				if (bCanReserveGame)
				{
					ClientSystemSetReservedGame( 
						AppGetClientSystem(), 
						idAppClient,
						pGameClient, 
						game, 
						game->m_dwReserveKey,
						nAreaID );
				}						
				break;
			}
			
			//----------------------------------------------------------------------------
			case GAME_SUBTYPE_TUTORIAL:
			{
				ASSERTX_BREAK( GameGetClientCount( game ) == 0, "Client is leaving tutorial, but other clients are still in it!!" );
				GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);	
				break;
			}
			
			//----------------------------------------------------------------------------
			case GAME_SUBTYPE_MINITOWN:
			case GAME_SUBTYPE_BIGTOWN:
			case GAME_SUBTYPE_OPENWORLD:
			{
				// just leave towns hanging around
				break;
			
			}
			
			//----------------------------------------------------------------------------
			case GAME_SUBTYPE_PVP_PRIVATE:
			case GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED:
				{
					if ( GameGetClientCount( game ) <= 0 )
					{
						GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);	
					}
					break;
				}

			//----------------------------------------------------------------------------
			case GAME_SUBTYPE_CUSTOM:
			case GAME_SUBTYPE_PVP_CTF:
			default:
			{
				// temporary
				break;
			}
			
		}

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sDoPetKill(
	UNIT * unit,
	void * data)
{
	UnitDie(unit, NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sClientWarpRemovePlayerFromInstance(
	GAME * game,
	GAMECLIENT * client)
{
	// remove this player
	UNIT * player = ClientGetPlayerUnitForced(client);
	int nAreaID = INVALID_ID;
	if (player)
	{
		if(game && IS_SERVER(game))
		{
			LEVEL * pLevel = UnitGetLevel(player);
			if( pLevel )
			{
				if( AppIsTugboat() )
				{
					nAreaID = LevelGetLevelAreaID(pLevel);
				}
				ClientClearAllRoomsKnown(client, pLevel);
				s_LevelRemovePlayer(pLevel,player);
			}
		}
		//Free his pets too, since otherwise this can cause...problems..in multiplayer -bmanegold
		// from what I can tell, this makes absolutely no sense - and it's causing
		// issues for Mythos with Hireling equipment on 'death' - a more robust explanation
		// of how on earth this helps anything would be great! ...problems is pretty vague.
		if( AppIsHellgate() )
		{
			PetIteratePets(player, sDoPetKill, NULL);
		}
		UnitFree( player, UFF_SEND_TO_CLIENTS | UFF_SWITCHING_INSTANCE );
	}

	// send message to client that game's closed
	MSG_SCMD_GAME_CLOSE msg;
	msg.idGame = GameGetId(game);
	msg.bRestartingGame = FALSE;
	msg.bShowCredits = FALSE;
	s_SendMessage(game, ClientGetId(client), SCMD_GAME_CLOSE, &msg);

	APPCLIENTID idAppClient = ClientGetAppId(client);

	// before we destroy this client, we need to see if they can reserve a game
	BOOL bCanReserveGame = !GameClientTestFlag( client, GCF_CANNOT_RESERVE_GAME );
	
	// remove the client from the game
	GameRemoveClient(game, client);
	client = NULL;
	// can't use client after this point
	
	sClientWarpReserveOrDestroyGame(game, idAppClient, NULL, nAreaID, bCanReserveGame);

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL WarpGameIdRequiresServerSwitch(
	GAMEID idWarpGame)
{
	SVRINSTANCE nServerInstance = CurrentSvrGetInstance();
	SVRID idServerGame = GameIdGetSvrId(idWarpGame);
	return (nServerInstance != ServerIdGetInstance(idServerGame));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sClientWarpRequiresServerSwitch(
	GAME * game,
	const WARP_SPEC &tWarpSpec,
	GAMEID &idPortalGame)
{
	if(AppGetType() != APP_TYPE_CLOSED_SERVER) return FALSE;

	idPortalGame = tWarpSpec.idGameOverride;
	if(idPortalGame == INVALID_ID) return FALSE;
	return GameIdGetSvrId(GameGetId(game)) != GameIdGetSvrId(idPortalGame);
}

//----------------------------------------------------------------------------
// Struct
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
struct GAME_SERVER_SWITCH_WARP_SPEC
{
	WCHAR			szCharName[MAX_CHARACTER_NAME];
	MEMORYPOOL *	pPool;
	NETCLIENTID64	idNetClient;
	int				nGameServer;	// game server to switch to (redundant)
	GAMEID			idGame;			// nGameServer = GameIdGetSvrId(idGame)
	WARP_SPEC		tWarpSpec;
	struct SERVER_MAILBOX *pServerMailbox;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME_SERVER_SWITCH_WARP_SPEC *ClientWarpSetupServerSwitchWarpSpec(
	NETCLIENTID64 idNetClient,
	const WARP_SPEC *pWarpSpec,
	GAMEID idGameToSwitchTo,
	const WCHAR *szPlayerName)
{
	ONCE
	{
		GameServerContext *pServerContext =
			(GameServerContext *)CurrentSvrGetContext();
		ASSERT_BREAK(pServerContext);
		ASSERT_BREAK(pServerContext->m_DatabaseCounter);

		//Set up data to pass through our dbSave callback.
		GAME_SERVER_SWITCH_WARP_SPEC * pServerWarpSpec =(GAME_SERVER_SWITCH_WARP_SPEC *)
			MALLOC(NULL, sizeof(GAME_SERVER_SWITCH_WARP_SPEC));
		ASSERT_BREAK(pServerWarpSpec);

		pServerWarpSpec->pPool = NULL;
		pServerWarpSpec->idNetClient = idNetClient;
		pServerWarpSpec->nGameServer = (int)(GameIdGetSvrId(idGameToSwitchTo));
		pServerWarpSpec->idGame = idGameToSwitchTo;
		pServerWarpSpec->tWarpSpec = *pWarpSpec;
		pServerWarpSpec->pServerMailbox = pServerContext->m_pPlayerToGameServerMailBox;

		PStrCopy(pServerWarpSpec->szCharName, MAX_CHARACTER_NAME,
			szPlayerName, MAX_CHARACTER_NAME);

		return pServerWarpSpec;
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientWarpPostServerSwitchMessage(
	void * context)
{
	ASSERT_RETURN(context);
	GAME_SERVER_SWITCH_WARP_SPEC * pSwitchSpec =
		(GAME_SERVER_SWITCH_WARP_SPEC *) context;

	ASSERT_RETURN(pSwitchSpec->pServerMailbox);

	//Set up a message with their warp data and target.
	MSG_APPCMD_SWITCHSERVER msg;
	msg.nGameServer = pSwitchSpec->nGameServer;
	msg.tWarpSpec = pSwitchSpec->tWarpSpec;
	msg.idGameToJoin = pSwitchSpec->idGame;
	PStrCopy(msg.szCharName, sizeof(msg.szCharName),
		pSwitchSpec->szCharName, sizeof(pSwitchSpec->szCharName));
	
	//AppPostMessageToMailbox doesn't work because we are in an arbitrary thread
	SvrNetPostFakeClientRequestToMailbox(
		pSwitchSpec->pServerMailbox,
		pSwitchSpec->idNetClient,
		&msg,
		APPCMD_SWITCHSERVER );

	FREE(pSwitchSpec->pPool, pSwitchSpec);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientWarpSetServerSwitchCallback(
	GAME_SERVER_SWITCH_WARP_SPEC *pServerWarpSpec,
	PGUID guidPlayer)
{
	ASSERT_RETFALSE(pServerWarpSpec);
	GameServerContext *pServerContext =
		(GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETFALSE(pServerContext);
	ASSERT_RETFALSE(pServerContext->m_DatabaseCounter);

	pServerContext->m_DatabaseCounter->
		SetCallbackOnZero(guidPlayer, pServerWarpSpec, sClientWarpPostServerSwitchMessage);
	pServerContext->m_DatabaseCounter->SetRemoveOnZero(guidPlayer);
	return TRUE;  //We'll post the message in the database callback
}

//----------------------------------------------------------------------------
// Setup the switch message and post it.  Called from s_network if we
// end up resolving the warp game at SWITCH_INSTANCE message processing time 
// instead of at warp time.
//----------------------------------------------------------------------------
BOOL ClientWarpPostServerSwitch(
	NETCLIENTID64 idNetClient,
	WARP_SPEC *pWarpSpec,
	GAMEID idGameToSwitchTo,
	PGUID guidPlayer,
	WCHAR *szPlayerName)
{
	GAME_SERVER_SWITCH_WARP_SPEC *pServerWarpSpec =
		ClientWarpSetupServerSwitchWarpSpec(
		idNetClient,
		pWarpSpec,
		idGameToSwitchTo,
		szPlayerName);
	ASSERT_RETFALSE(pServerWarpSpec);

	return ClientWarpSetServerSwitchCallback(pServerWarpSpec, guidPlayer);
}
#endif //SERVER_VERSION
//----------------------------------------------------------------------------
struct DO_WARP_DATA
{	
	GAMECLIENTID idClient;
	DWORD dwKey;
	WARP_SPEC tWarpSpec;	
	int nLevelDefLeaving;
	int nLevelDefLastHeadstone;
	TOWN_PORTAL_SPEC tTownPortal;
	BOOL bJoinOpenGame;	
	WCHAR szPlayerName[32];
	PGUID guidPlayer;
	int nDifficulty;	
	GAME_VARIANT tGameVariant;		// type of game this character has chosen to play
	LEVELID idLevelLeaving;
	
	// init new fields in sDoWarpDataInit() below
	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoWarpDataInit(
	DO_WARP_DATA &tDoWarpData)
{
	tDoWarpData.idClient = INVALID_CLIENTID;
	tDoWarpData.dwKey = 0;
	tDoWarpData.nLevelDefLeaving = INVALID_LINK;
	tDoWarpData.nLevelDefLastHeadstone = INVALID_LINK;
	tDoWarpData.bJoinOpenGame = FALSE;
	tDoWarpData.guidPlayer = INVALID_GUID;
	tDoWarpData.nDifficulty = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
	tDoWarpData.idLevelLeaving = INVALID_ID;
}

//----------------------------------------------------------------------------
// TODO: factor pPlayer out of server warp info
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sClientWarpToInstanceDo(
	GAME * game,
	UNIT * unit,
	const EVENT_DATA & event_data)
{
	ASSERTX_RETFALSE( game, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( game ), "Expected server game" );
	ASSERTX_RETFALSE( unit == NULL, "Expected NULL unit" );
	
	const DO_WARP_DATA *pDoWarpData = (const DO_WARP_DATA *)event_data.m_Data1;
	ASSERTX_RETFALSE (pDoWarpData, "Expected warp data" );
	GAMECLIENTID idClient = pDoWarpData->idClient;
	GAMECLIENT * client = ClientGetById(game, idClient);
	ASSERTX_RETFALSE( client, "Expected client" );
	
	DWORD key = pDoWarpData->dwKey;
	const WARP_SPEC *pWarpSpec = &pDoWarpData->tWarpSpec;
	
	// send msg to show loading screen
	sClientWarpSendLoadingScreen(
		game, 
		idClient, 
		*pWarpSpec);

	NETCLIENTID64 idNetClient = ClientGetNetId(client);
	ASSERT(idNetClient != INVALID_NETCLIENTID64);

	GAMEID idGameToSwitchTo;
	BOOL bServerSwitch = sClientWarpRequiresServerSwitch(game, *pWarpSpec, idGameToSwitchTo);

	// setup message for switching instances
	MSG_APPCMD_SWITCHINSTANCE tMessage;
#if ISVERSION(SERVER_VERSION)
	GAME_SERVER_SWITCH_WARP_SPEC *pServerWarpSpec = NULL;
#endif
	PGUID guidPlayer = pDoWarpData->guidPlayer;
	

	if(bServerSwitch)
	{		
#if ISVERSION(SERVER_VERSION)
		//attach the warp spec to the database record
		pServerWarpSpec = ClientWarpSetupServerSwitchWarpSpec(
			idNetClient,  pWarpSpec, idGameToSwitchTo, pDoWarpData->szPlayerName);
		ASSERT_DO(pServerWarpSpec != NULL)
		{
			bServerSwitch = FALSE;
		}
#else
		ASSERT_MSG("We should never get here in the client version!");
		bServerSwitch = FALSE;
#endif
	}
	if(!bServerSwitch)
	{
		tMessage.dwKey = key;
		tMessage.nLevelDefLeaving = pDoWarpData->nLevelDefLeaving;
		tMessage.dwSeed = DungeonGetSeed( game, pWarpSpec->nLevelAreaDest );
		tMessage.tTownPortal = pDoWarpData->tTownPortal;
		tMessage.nLevelDefLastHeadstone = pDoWarpData->nLevelDefLastHeadstone;
		tMessage.tWarpSpec = *pWarpSpec;
		tMessage.dwSwitchInstanceFlags = 0;
		tMessage.guidPlayer = guidPlayer;
		tMessage.nDifficulty = pDoWarpData->nDifficulty;
		tMessage.tGameVariant = pDoWarpData->tGameVariant;
		if (pDoWarpData->bJoinOpenGame)
		{
			SETBIT(tMessage.dwSwitchInstanceFlags, SIF_JOIN_OPEN_LEVEL_GAME_BIT);
		}
	}

	// remove the client from the game
	sClientWarpRemovePlayerFromInstance(game, client);

	// done with this data now
	GFREE( game, pDoWarpData );
	pDoWarpData = NULL;

	// post a message to the application to switch 1instances
	if(!bServerSwitch) 
	{
		return AppPostMessageToMailbox(idNetClient, APPCMD_SWITCHINSTANCE, &tMessage);	
	}
	else 
	{
#if ISVERSION(SERVER_VERSION)
		return ClientWarpSetServerSwitchCallback(pServerWarpSpec, guidPlayer);
#else
		ASSERT_MSG("Server only code!?");
		return FALSE;
#endif
	}
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GAMEID sCreateNewGame(
	int nDifficulty,
	GAME_SUBTYPE eGameSubType,
	GAME_SETUP &tSetup)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	return SrvNewGame(eGameSubType, &tSetup, FALSE, nDifficulty);
#else
	return INVALID_ID;
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
}


//----------------------------------------------------------------------------
// GAME_SUBTYPE must be default to use this function
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAMEID sGetReservedOrNewGame(
	APPCLIENTID idAppClient,
	int nDifficulty,
	GAME_SETUP &tSetup,
	const LEVEL_TYPE *pOpenLevelTypeDest)
{

	// first get reserved game	
	DWORD dwReserveKey = 0;
	GAMEID idGame = ClientSystemGetReservedGame(AppGetClientSystem(), idAppClient, dwReserveKey);

	if (idGame == INVALID_ID)
	{	
		GAME_SUBTYPE eSubType = GAME_SUBTYPE_DEFAULT;
		
		// create new game
		idGame = sCreateNewGame(nDifficulty, eSubType, tSetup);
		
		// if we want to post this game as an open level, do so now
		if (pOpenLevelTypeDest)
		{
		
			// fill out open game info
			OPEN_GAME_INFO tOpenGameInfo;
			tOpenGameInfo.idGame = idGame;
			tOpenGameInfo.eSubType = eSubType;
			tOpenGameInfo.tGameVariant = tSetup.tGameVariant;
			tOpenGameInfo.tLevelType = *pOpenLevelTypeDest;
			tOpenGameInfo.nNumPlayersInOrEnteringGame = 1;
			
			// post it
			OpenLevelPost( tOpenGameInfo, OLS_OPEN );
			
		}
		
	}
	else if( AppIsTugboat() && pOpenLevelTypeDest )
	{
		GAME_SUBTYPE eSubType = GAME_SUBTYPE_DEFAULT;

		// if we want to post this game as an open level, do so now
		if (pOpenLevelTypeDest)
		{

			// fill out open game info
			OPEN_GAME_INFO tOpenGameInfo;
			tOpenGameInfo.idGame = idGame;
			tOpenGameInfo.eSubType = eSubType;
			tOpenGameInfo.tGameVariant = tSetup.tGameVariant;
			tOpenGameInfo.tLevelType = *pOpenLevelTypeDest;
			tOpenGameInfo.nNumPlayersInOrEnteringGame = 1;

			// post it
			OpenLevelPost( tOpenGameInfo, OLS_OPEN );

		}
	}
	// todo: is there the possibility that the reserved game is closed at this point?
	// it shouldn't happen, but there could be a bug, and we should handle it if that's the case?
	return idGame;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GAMEID sCreateOrFindPartyGame(
	PARTY_GAME_TYPE ePartyGameType,
	APPCLIENTID idAppClient,
	const LEVEL_TYPE &tLevelTypeDest,
	int nDifficulty,
	GAME_SETUP &tSetup)
{
	GAMEID idGame = INVALID_ID;
	
#if !ISVERSION(CLIENT_ONLY_VERSION)
	switch (ePartyGameType)
	{
	
		//----------------------------------------------------------------------------
		case PGT_PARTY_GAME:
		{
					
			// a new game that is just for this party
			idGame = sCreateNewGame(nDifficulty, GAME_SUBTYPE_DEFAULT, tSetup);
			break;
			
		}
		
		//----------------------------------------------------------------------------
		case PGT_TOWN_GAME:
		{
			const GAME_VARIANT &tGameVariant = tSetup.tGameVariant;
			ASSERTV( AppIsTugboat() || tGameVariant.dwGameVariantFlags == 0, "Town game should have no special game flags" );
			ASSERTV( tGameVariant.nDifficultyCurrent == GlobalIndexGet( GI_DIFFICULTY_DEFAULT ), "Town game should have default difficutly" );
			
			// get an available town game
			idGame = MiniTownGetAvailableGameId( idAppClient, tLevelTypeDest.nLevelDef, tSetup );
			break;
			
		}
		
	}
#endif
	
	return idGame;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GAMEID sGetExistingPartyGame(
	PARTYID idParty,
	APPCLIENTID idAppClient,
	const LEVEL_TYPE &tLevelTypeDest)
{
	ASSERTX_RETINVALID( idParty != INVALID_LINK, "Expected link" );
	GAMEID idGameDest = INVALID_ID;

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMEID idGameReserved = INVALID_ID;
	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
		
	CachedPartyPtr party(svr->m_PartyCache);
	party = PartyCacheLockParty( svr->m_PartyCache, idParty );
	if( party )
	{
			
		// go through all party members -
		// TRAVIS: check their current level defs, and if they match OUR destination,
		// pick their gameid. Otherwise, we need to grab one from the minitown
		// so that hopefully, we end up with other people, and not in
		// our own lonely game.
		CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
		const LEVEL_DEFINITION *pLevelDefDest = LevelDefinitionGet(tLevelTypeDest.nLevelDef);
		ASSERTX_RETINVALID( pLevelDefDest, "Expected level dest defintion" );
		int Members = party->Members.Count();
		for( int i = 0; i < Members; i++ )
		{
			CachedPartyMember *pPartyMember = party->Members[ i ];
			if( pPartyMember->IdAppClient == idAppClient )
			{
				continue;
			}
			
			// forget members that are not in game (does that happen???)
			ASSERT_CONTINUE( pPartyMember->MemberData.IdGame != INVALID_ID );
			
			if( TESTBIT( pPartyMember->MemberData.tGameVariant.dwGameVariantFlags, GV_PVPWORLD ) != tLevelTypeDest.bPVPWorld)
			{
				continue;
			}
			// where is this member right now?
			const LEVEL_DEFINITION *pLevelDefMember = LevelDefinitionGet(pPartyMember->MemberData.nLevelDef);
			
			// get where this party member is going to (if anywhere)
			LEVEL_GOING_TO tLevelGoingTo;
			ClientSystemGetLevelGoingTo( pClientSystem, pPartyMember->IdAppClient, &tLevelGoingTo );
			
			// are we going to an instance level
			if (pLevelDefDest->eSrvLevelType == SRVLEVELTYPE_DEFAULT)
			{
			
				// if any party member is in an instance level, we will go to their game
				// even if they are on a different level
				if (pLevelDefMember && pLevelDefMember->eSrvLevelType == SRVLEVELTYPE_DEFAULT) 
				{
					idGameDest = pPartyMember->MemberData.IdGame;
				}
				
				// OK, this member is not in an instance level ... if they are currently going to one
				// we can use that game id.  This instance game is recorded when a player goes
				// through a warp and will allow us to move a group of players who zone at 
				// exactly the same time to the same game
				if (idGameDest == INVALID_ID)
				{
					if (tLevelGoingTo.idGame != INVALID_ID && 
						LevelDefinitionGetSrvLevelType( tLevelGoingTo.tLevelType.nLevelDef ) == SRVLEVELTYPE_DEFAULT)
					{
						idGameDest = tLevelGoingTo.idGame;
					}
				}
			}
			
			// ok, if no game, if we find any party member on the level we're going to, we want
			// to go to their game (includes towns and whatever else we can think of)
			if (idGameDest == INVALID_ID)
			{
				if( AppIsHellgate())
				{
					// Hellgate can just check the leveldef				
					if (pPartyMember->MemberData.nLevelDef == tLevelTypeDest.nLevelDef)
					{
						idGameDest = pPartyMember->MemberData.IdGame;
					}
					else if (tLevelGoingTo.idGame != INVALID_ID)
					{
						// this party member is not on the level type we're looking for, but
						// if they are in the process of going there, we can use that
						// "going to" game as well
						if (tLevelGoingTo.tLevelType.nLevelDef == tLevelTypeDest.nLevelDef)
						{
							idGameDest = tLevelGoingTo.idGame;
						}
					}
					
				}
				else
				{
					// Mythos checks areaid - that's the most telling thing due to our randomized map system,
					// and the fact that one outdoor/dungeon instance may comprise several DIFFERENT leveldefs.
					// if one partymember is in the caves, you might be checking against the clearing definition,
					// and you wouldn't find a match.				
					if (pPartyMember->MemberData.nArea == tLevelTypeDest.nLevelArea &&
						 TESTBIT( pPartyMember->MemberData.tGameVariant.dwGameVariantFlags, GV_PVPWORLD ) == tLevelTypeDest.bPVPWorld ) 
					{
						idGameDest = pPartyMember->MemberData.IdGame;
					}
					else if (tLevelGoingTo.idGame != INVALID_ID)
					{
						// this party member is not on the level type we're looking for, but
						// if they are in the process of going there, we can use that
						// "going to" game as well
						if (tLevelGoingTo.tLevelType.nLevelArea == tLevelTypeDest.nLevelArea &&
							tLevelGoingTo.tLevelType.bPVPWorld == tLevelTypeDest.bPVPWorld )
						{
							idGameDest = tLevelGoingTo.idGame;
						}
					}
					
				}
				
			}

			// ok, if a player in our party has a reserved game (which are only set when a
			// player leaves an instance level) we want to be able to go to that game
			// if we don't find any active players in games to go to.
			// This will catch the case of Player A enters level, 
			// player B joins party, player A returns to town, and player B enters level		
			// TRAVIS: We should ONLY DO THIS IF THIS IS AN INSTANCED GAME WE'RE GOING TO! NEVER for town
			if (idGameDest == INVALID_ID &&
				pLevelDefDest->eSrvLevelType == SRVLEVELTYPE_DEFAULT )
			{
				DWORD dwReserveKey = 0;
				idGameReserved = ClientSystemGetReservedGame( pClientSystem, pPartyMember->IdAppClient, dwReserveKey );
			}
			
			// if we got a game, stop searching
			if (idGameDest != INVALID_ID)
			{
				break;
			}
			
		}

	}

	// if we still have not found a game, ie, none of our party members are in appropriate
	// places for us to join their game, try to use a reserved game (if any) from any party member
	if (idGameDest == INVALID_ID)
	{
		idGameDest = idGameReserved;
	}

#endif //  !ISVERSION(CLIENT_ONLY_VERSION)
		
	return idGameDest;		
	
}

//----------------------------------------------------------------------------
// GAME_SUBTYPE must be default to use this function
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAMEID sWarpGetGroupGame(
	APPCLIENTID idAppClient,
	PARTYID idParty,
	const LEVEL_TYPE &tLevelTypeDest,
	PARTY_GAME_TYPE ePartyGameType,
	int nDifficulty,
	GAME_SETUP &tSetup)
{
	ASSERTX_RETINVALID( nDifficulty != INVALID_LINK, "Expected difficulty" );
	REF(idAppClient);
	CHAT_REQUEST_MSG_UPDATE_PARTY_COMMON_DATA partyUpdateMsg;
	BOOL shouldSend = FALSE;

	ASSERT_RETINVALID(idParty != INVALID_ID);

	GAMEID idGameDest = INVALID_ID;
	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if( !svr )
	{
		return INVALID_ID;	//	shutting down...
	}

	// find a game that any of your party members
	idGameDest = sGetExistingPartyGame( idParty, idAppClient, tLevelTypeDest );

	// if we didn't find one ...
	if (idGameDest == INVALID_ID)
	{	
	
		// get pointer to party (will auto free the lock when this goes out of scope)
		CachedPartyPtr party(svr->m_PartyCache);		
		party = PartyCacheLockParty( svr->m_PartyCache, idParty );		
		if( party )
		{
								
			// OK, none of our party members are in a level that we want to go to, if we're
			// going to a default/dungeon level and we have a reserve game 
			// set for our client, we will return to it
			const LEVEL_DEFINITION *pLevelDefDest = LevelDefinitionGet(tLevelTypeDest.nLevelDef);			
			if (pLevelDefDest && pLevelDefDest->eSrvLevelType == SRVLEVELTYPE_DEFAULT)
			{
				DWORD dwReserveKey = 0;
				GAMEID idGameReserved = ClientSystemGetReservedGame( AppGetClientSystem(), idAppClient, dwReserveKey );
				if (idGameReserved != INVALID_ID &&
					WarpGameIdRequiresServerSwitch( idGameReserved ) == FALSE)
				{
					// only allow returning to the reserved game if its on this server simply
					// because we don't want players hoping server lots of times
					idGameDest = idGameReserved;
				}			
			}
									
			// if no game, create it
			if( idGameDest == INVALID_ID  )
			{
				// create game
				idGameDest = sCreateOrFindPartyGame( 
					ePartyGameType, 
					idAppClient, 
					tLevelTypeDest, 
					nDifficulty,
					tSetup);
				ASSERTX_RETINVALID( idGameDest != INVALID_LINK, "Unable to create or find game for party" );
				
				// setup message to update chat server
				partyUpdateMsg.TagParty = idParty;
				partyUpdateMsg.PartyData.Set( &party->PartyData );

				shouldSend = TRUE;

				//	have to do this within the lock in closed server, but can't
				//	in open server because sending to other servers isn't actually async...
				if(AppGetType() == APP_TYPE_CLOSED_SERVER)
				{
					GameServerToChatServerSendMessage( &partyUpdateMsg, GAME_REQUEST_UPDATE_PARTY_COMMON_DATA );
				}
				
			}
			
		}
	}
	
	if(AppGetType() == APP_TYPE_OPEN_SERVER && shouldSend)
	{
		GameServerToChatServerSendMessage( &partyUpdateMsg, GAME_REQUEST_UPDATE_PARTY_COMMON_DATA );
	}
	
	return idGameDest;	
	
}
#endif


//----------------------------------------------------------------------------
// GAME_SUBTYPE must be default to use this function
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAMEID sGetOnePartyGame(
	GAME_SETUP &tSetup)
{
	static GAMEID idOnePartyGame = INVALID_ID;
	if (idOnePartyGame == INVALID_ID)
	{
		idOnePartyGame = sCreateNewGame(0, GAME_SUBTYPE_DEFAULT, tSetup);
	}
	return idOnePartyGame;
}
#endif


//----------------------------------------------------------------------------
// GAME_SUBTYPE must be default to use this function
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAMEID sGetDungeonGameToJoin(
	APPCLIENTID idAppClient,
	const LEVEL_TYPE & tLevelTypeDest,
	DWORD *pdwSwitchInstanceFlags,
	SRVLEVELTYPE eServerLevelType,
	int nDifficulty,
	GAME_SETUP &tSetup)
{

	// one party game debug
	if (AppCommonGetOneParty() || AppCommonGetOneInstance())
	{
		return sGetOnePartyGame( tSetup );
	}
	
	PARTYID idParty = ClientSystemGetParty(AppGetClientSystem(), idAppClient);
	if (idParty != INVALID_ID)	// if i'm in a group
	{
	
		// which group game do we want to get
		PARTY_GAME_TYPE ePartyGameType = PGT_PARTY_GAME;
		if (eServerLevelType == SRVLEVELTYPE_DEFAULT)
		{
			ePartyGameType = PGT_PARTY_GAME;
		}
		else
		{
			ePartyGameType = PGT_TOWN_GAME;
		}

		// get the game id to join for the party		
		GAMEID idGameDest = sWarpGetGroupGame(
			idAppClient, 
			idParty, 
			tLevelTypeDest, 
			ePartyGameType, 
			nDifficulty, 
			tSetup);
			
		TraceGameInfo(
			"Got dest '%s' game ID %I64x for appclient %x", 
			ePartyGameType == PGT_PARTY_GAME ? "party" : "town",
			idGameDest, 
			idAppClient);

		ONCE
		{	//Complicated code... if it's on a different server, check if that's allowed.  
			//If it's on the same server, check if the game is still running.
			if(GAMEID_IS_INVALID(idGameDest)) 
			{
				break;
			}
			BOOL bRequiresSwitch = WarpGameIdRequiresServerSwitch(idGameDest);
			if(bRequiresSwitch)
			{
				TraceGameInfo("Got party game ID %I64x that requires server switch", idGameDest);
			}
			if( (pdwSwitchInstanceFlags && 
				TESTBIT(*pdwSwitchInstanceFlags, SIF_DISALLOW_DIFFERENT_GAMESERVER) )
				&& bRequiresSwitch)
			{
				TraceError("Server switch disallowed, rejecting partygame");
				break;
			}

			GameServerContext * context = (GameServerContext*)CurrentSvrGetContext();

			if(!bRequiresSwitch && context && !GameListQueryGameIsRunning(context->m_GameList, idGameDest) )
			{
				TraceError("Rejecting non-running party game ID %I64x", idGameDest);
				break;			
			}
			return idGameDest;
		}
	}

	if (eServerLevelType == SRVLEVELTYPE_DEFAULT)
	{
			
		// if requested, see if there is a game out there with an 
		// open level of our destination that we can join
		BOOL bJoinOpenLevelGame = FALSE;
		if (pdwSwitchInstanceFlags != NULL && TESTBIT(*pdwSwitchInstanceFlags, SIF_JOIN_OPEN_LEVEL_GAME_BIT))
		{
			bJoinOpenLevelGame = TRUE;
			GAMEID idGame = OpenLevelGetGame(tLevelTypeDest, tSetup.tGameVariant);
			if (idGame != INVALID_ID)
			{
				return idGame;
			}					
		}
		
		// use the reserved or a brand new game
		return sGetReservedOrNewGame(
			idAppClient, 
			nDifficulty, 
			tSetup,
			bJoinOpenLevelGame ? &tLevelTypeDest : NULL);
		
	}
	else
	{
		ASSERTX( eServerLevelType == SRVLEVELTYPE_MINITOWN ||
				 eServerLevelType == SRVLEVELTYPE_OPENWORLD , "Expected minitown server level type" );
		
		// what difficulty to use, for multiplayer we will always go to a town game 
		// setup as default difficulty, but for single player, since we have just one
		// game instance, when we're going into town, we should setup the game 
		// options correctly
		int nDifficultyToUse = AppIsSinglePlayer() ? nDifficulty : GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
		tSetup.tGameVariant.nDifficultyCurrent = AppIsSinglePlayer() ? nDifficulty : GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
		
		// get minitown game
		return MiniTownGetAvailableGameId( 
			idAppClient, 
			tLevelTypeDest.nLevelDef, 
			tSetup, 
			nDifficultyToUse);
			
	}
	
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
GAME_SUBTYPE GetGameSubTypeByLevel(
	unsigned int level)
{
	const LEVEL_DEFINITION * level_data = LevelDefinitionGet(level);
	ASSERT_RETVAL(level_data, GAME_SUBTYPE_DEFAULT);
	switch (level_data->eSrvLevelType)
	{
	case SRVLEVELTYPE_DEFAULT:		
		return GAME_SUBTYPE_DEFAULT;
	case SRVLEVELTYPE_TUTORIAL:		
		return GAME_SUBTYPE_TUTORIAL;
	case SRVLEVELTYPE_MINITOWN:		
		return GAME_SUBTYPE_MINITOWN;
	case SRVLEVELTYPE_OPENWORLD:		
		return GAME_SUBTYPE_OPENWORLD;
	case SRVLEVELTYPE_BIGTOWN:		
		return GAME_SUBTYPE_BIGTOWN;
	case SRVLEVELTYPE_CUSTOM:
		return GAME_SUBTYPE_CUSTOM;
	case SRVLEVELTYPE_PVP_PRIVATE:
		return GAME_SUBTYPE_PVP_PRIVATE;	//This is somewhat broken, as we use different gametypes for automated and non-automated pvp, but the same levels.  Fortunately, nothing currently depends on this.
	case SRVLEVELTYPE_PVP_CTF:
		return GAME_SUBTYPE_PVP_CTF;
	default:
		ASSERT_RETVAL(0, GAME_SUBTYPE_DEFAULT);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LEVELID sLevelWarpGetOrCreateDestinationLevel(
	UNIT *pPlayer,
	const WARP_SPEC &tWarpSpec,
	UNIT *pWarpCreator,		// if present
	int nDifficultyOffset,
	BOOL bSearchForExistingByType)
{
	ASSERTX_RETINVALID( pPlayer, "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	LEVEL_TYPE tLevelTypeDest = WarpSpecGetLevelType( tWarpSpec );
	
	// trivial case, the caller has already provided one somehow
	if (tWarpSpec.idLevelDest != INVALID_LINK)
	{
		return tWarpSpec.idLevelDest;
	}

	// do searching by type if requested
	if (bSearchForExistingByType == TRUE)
	{
	
		// this should change to use level instance ids instead of finding the first by type -Colin
		LEVEL *pLevelExisting = LevelFindFirstByType( pGame, tLevelTypeDest );
		
		// if not found, try creating levels at root (which will create connected chains and
		// possibly the level we're looking for)
		if (pLevelExisting == NULL)
		{
			s_LevelCreateRoot( pGame, tLevelTypeDest.nLevelDef, UnitGetId( pPlayer ) );
			pLevelExisting = LevelFindFirstByType( pGame, tLevelTypeDest );
		}

		// if we have an existing level, use it		
		if (pLevelExisting)
		{
			return LevelGetID( pLevelExisting );
		}

	}
		
	// get the current level we're linking from ... we use the warp if it's present
	// because we are sure it's in the level ... the player, in some instances
	// may not actually be in the level yet (like when we're populating a level ...
	// we have a player that is responsible for the populate, but not in a level yet)
	// maybe we should always use the warp's level instance here? -Colin
	LEVEL *pLevelCurrent = pWarpCreator ? UnitGetLevel( pWarpCreator ) : UnitGetLevel( pPlayer );
	LEVELID idLevelCurrent = LevelGetID( pLevelCurrent );

	// check the warps that have been recorded in the dest level as linking to
	// the current level ... if any of them are the same type of level as the
	// dest we're looking for, we'll return one of those already created level instances
	LEVELID idLevelExisting = s_LevelFindLevelOfTypeLinkingTo( 
		pGame, 
		tLevelTypeDest,
		idLevelCurrent );
	if (idLevelExisting != INVALID_ID)
	{
		return idLevelExisting;
	}
				
	// no existing level found, fill out spec for new level
	LEVEL_SPEC tSpec;
	tSpec.tLevelType = tLevelTypeDest;
	tSpec.dwSeed = DefinitionGetGlobalSeed();
	tSpec.pLevelCreator = pLevelCurrent;
	tSpec.idWarpCreator = UnitGetId( pWarpCreator );
	tSpec.idActivator = UnitGetId( pPlayer );	
	tSpec.nTreasureClassInLevel = s_AdventurePassageGetTreasureClass( pWarpCreator );
	tSpec.nDifficultyOffset = nDifficultyOffset;
	tSpec.bPopulateLevel = FALSE;  // we must not populate it now
	
	// create level	
	LEVEL *pLevelNew = LevelAdd( pGame, tSpec );
	ASSERTX_RETINVALID( pLevelNew , "Unable to create new level" );
	
	// return level id of newly created level	
	return LevelGetID( pLevelNew );	
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL WarpToInstance(
	UNIT *pPlayer, 
	const WARP_SPEC &tSpec)
{
	ASSERT_RETFALSE(pPlayer);
	GAME *pGame = UnitGetGame( pPlayer );

	// we always close portals when we switch instances in mythos
	/*if( AppIsTugboat() )
	{
		// if we are warping to a new area, always close all old warps.
		s_TownPortalsClose( pPlayer );
	}*/
	// do not allow warping to levels that are not available
	if (tSpec.nLevelDefDest != INVALID_LINK)
	{
		if (LevelDefIsAvailableToUnit( pPlayer, tSpec.nLevelDefDest ) != WRR_OK)
		{
			return FALSE;
		}
	}
	
	GAMECLIENT *client = UnitGetClient( pPlayer );
	ASSERT_RETFALSE( client );
	
	DWORD key = RandGetNum(pGame);
	key = MAX((DWORD)1, key);

	// band-aid, cancel all interactions ... specifically we mostly care about trade
	// bugs that the Koreans are finding by leaving the server instance during a trade
	// in various ways, but in general canceling all interactions before saving
	// and leaving the server will also keep always the player in a valid state
	s_InteractCancelAll( pPlayer );
		
	// save off the player to the appclient
	if (!PlayerSaveToAppClientBuffer(pPlayer, key))
	{
		return FALSE;
	}

	// In server, always save on instance switch for now,
	// so save cannot be avoided by disconnecting during switch.
#if ISVERSION(SERVER_VERSION)
	// save character digest/wardrobe data on warp 
	CharacterDigestSaveToDatabase(pPlayer);

	// Set any outstanding DB_OPs to commit immediately
	if (s_DBUnitOperationsImmediateCommit(pPlayer) == FALSE)
	{
		TraceError("Error committing pending DB Unit Operations in WarpToInstance.  GUID: [%I64d]", pPlayer->guid);
	}

	s_DatabaseUnitEnable( pPlayer, FALSE );
#endif

	// setup to remove the player from the game
	// set client as disconnecting, further requests to client get control unit will fail
	ClientSetState(client, GAME_CLIENT_STATE_SWITCHINGINSTANCE);

	// register event to remove client & swap instance
	DO_WARP_DATA *pDoWarpData = (DO_WARP_DATA *)GMALLOCZ( pGame, sizeof( DO_WARP_DATA ) );
	sDoWarpDataInit( *pDoWarpData );
	pDoWarpData->idClient = ClientGetId( client );
	pDoWarpData->dwKey = key;
	pDoWarpData->tWarpSpec = tSpec;
	pDoWarpData->nLevelDefLeaving = UnitGetLevelDefinitionIndex( pPlayer );
	s_TownPortalGetSpec( pPlayer, &pDoWarpData->tTownPortal );
	pDoWarpData->nLevelDefLastHeadstone = PlayerGetLastHeadstoneLevelDef( pPlayer );	
	pDoWarpData->idLevelLeaving = UnitGetLevelID( pPlayer );
	pDoWarpData->bJoinOpenGame = FALSE;
	
	// join open level
	if (OpenLevelSystemInUse())
	{
		if( AppIsTugboat() )
		{
			pDoWarpData->bJoinOpenGame = TRUE;
		}
		else
		{
			if (s_PlayerIsAutoPartyEnabled( pPlayer ) == TRUE)
			{
				if (UnitGetPartyId( pPlayer ) == INVALID_ID)
				{
					pDoWarpData->bJoinOpenGame = TRUE;
				}
			}
		}
	}
	
	pDoWarpData->guidPlayer = UnitGetGuid(pPlayer);
	if(pPlayer->szName) 
	{
		PStrCopy(pDoWarpData->szPlayerName, MAX_CHARACTER_NAME, pPlayer->szName, MAX_CHARACTER_NAME);
	}
	pDoWarpData->nDifficulty = DifficultyGetCurrent( pPlayer );
	
	// setup game variant information for the type of game we want to go into
	GameVariantInit( pDoWarpData->tGameVariant, pPlayer );
		
	// setup event data
	EVENT_DATA event_data( (DWORD_PTR)pDoWarpData );
		
	// remove this player right now, we no longer need to delay a frame and
	// care about dangling unit pointers on the stack because all unit destructions
	// are deferred until the end of the frame anyway
	sClientWarpToInstanceDo( pGame, NULL, event_data );
	//GameEventRegister(pGame, sClientWarpToInstanceDo, NULL, NULL, &event_data, 1);

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WarpToLevelOrInstance(
	UNIT *pPlayer,
	WARP_SPEC &tSpec)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Expected server" );	

	// can't warp if state doens't allow it unless we're being booted
	if (UnitHasState( pGame, pPlayer, STATE_NO_TRIGGER_WARPS ) &&
		TESTBIT( tSpec.dwWarpFlags, WF_BEING_BOOTED_BIT ) == FALSE)
	{
		return FALSE;
	}
	
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETFALSE( pClient, "Expected client" );

	// init level instance
	ASSERTX_RETFALSE( tSpec.idLevelDest == INVALID_ID, "Level instance is specified ... this is ok? Seems strnage, we should allow the caller to specify this if they want -Colin" );		
	tSpec.idLevelDest = INVALID_ID;
	
	// get current level
	LEVEL *pLevelCurrent = UnitGetLevel( pPlayer );	
	const LEVEL_DEFINITION *ptLevelDefCurrent = LevelDefinitionGet( pLevelCurrent );
	if( AppIsTugboat() && tSpec.nLevelAreaPrev == INVALID_ID )
	{
		tSpec.nLevelAreaPrev = pLevelCurrent ? LevelGetLevelAreaID( pLevelCurrent ) : INVALID_ID;
	}
	// get dest level
	int nLevelDefDest = INVALID_LINK;
	const LEVEL_DEFINITION *ptLevelDefDest = NULL;
	if (AppIsHellgate())
	{
		// KCK: For warp to instance
		if (tSpec.nLevelDefDest == INVALID_ID && tSpec.nLevelAreaDest != INVALID_ID)
			nLevelDefDest = tSpec.nLevelAreaDest;
		else
			nLevelDefDest = tSpec.nLevelDefDest;
		ptLevelDefDest = LevelDefinitionGet( nLevelDefDest );
		tSpec.nLevelDefDest = nLevelDefDest;
	}
	else
	{
		// get a level def dest (maybe this should have been filled out in warp spec?) -Colin
		nLevelDefDest = LevelDefinitionGetRandom( pLevelCurrent, tSpec.nLevelAreaDest, tSpec.nLevelDepthDest );
		ASSERTX_RETFALSE( nLevelDefDest != INVALID_LINK, "Invalid level definition destination" );		
		ptLevelDefDest = LevelDefinitionGet( nLevelDefDest );			
	}
	ASSERTX_RETFALSE( ptLevelDefDest, "Expected level definition for destination" );
	
	// level must be available
	if (LevelDefIsAvailableToUnit( pPlayer, nLevelDefDest ) != WRR_OK)
	{
		return FALSE;
	}
	
	s_QuestEventAboutToLeaveLevel( pPlayer );

	tSpec.nDifficulty = pGame->nDifficulty;
	if (sIsInstanceChangeRequired( ptLevelDefDest, ptLevelDefCurrent ) ||
			(tSpec.idGameOverride != INVALID_ID && 
			 tSpec.idGameOverride != UnitGetGameId(pPlayer)))
	{
		// quit the game and post message to app to switch instances
		return WarpToInstance( pPlayer, tSpec );
	}
	else
	{
			
		int nDifficultyOffset = 0;
		if (AppIsTugboat())
		{		
			nDifficultyOffset = DungeonGetDifficulty( pGame, tSpec.nLevelAreaDest, pPlayer );
		}
					
		// do regular warp level logic
		tSpec.idLevelDest = sLevelWarpGetOrCreateDestinationLevel(
			pPlayer,
			tSpec,
			NULL,
			nDifficultyOffset,
			TRUE);
										
	}
	
	
	// do the warp	
	ASSERTX_RETFALSE( tSpec.idLevelDest != INVALID_ID, "Expected level id" );
	return s_LevelWarp( pPlayer, tSpec );
#else
	REF( pPlayer );
	REF( tSpec );
	return FALSE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
		
}


//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// return the gameid to join, creating the game if necessary
// based on level
//----------------------------------------------------------------------------

GAMEID WarpGetGameIdToJoin(
	APPCLIENTID idAppClient,
	BOOL bInitialJoin,			// true if it's on initial join, false if switching levels/instances	
	int nLevelDef,				// hellgate
	int nLevelArea,				// tugboat
	DWORD dwSeed,
	DWORD *pdwSwitchInstanceFlags, // see SWITCH_INSTANCE_FLAGS
	int nDifficulty /*= 0*/,	// remove this
	const GAME_VARIANT &tGameVariant)
{
	if (bInitialJoin)
	{
		GAME_GLOBAL_DEFINITION * game_globals = DefinitionGetGameGlobal();
		ASSERT_RETINVALID(game_globals);
		if (game_globals->nLevelDefinition != INVALID_LINK &&
			LevelDefinitionGet( game_globals->nLevelDefinition ) != NULL)	
		{
			nLevelDef = game_globals->nLevelDefinition;
		}	
	}
	
	if (AppIsHellgate() && nLevelDef == INVALID_LINK)
	{
		nLevelDef = LevelGetStartNewGameLevelDefinition();
	}

	if (AppIsTugboat())
	{
		int nArea = MYTHOS_LEVELAREAS::LevelAreaGetStartNewGameLevelDefinition();
		int nDepth(0);
		if (nLevelArea == INVALID_LINK)
		{
			nLevelDef = LevelDefinitionGetRandom( NULL, nArea, nDepth ); 
		}
		else
		{
			nLevelDef = LevelDefinitionGetRandom( NULL, nLevelArea, nDepth );  
		}
	}

	if (AppTestDebugFlag(ADF_START_SOUNDFIELD_BIT))
	{
		nLevelDef = GlobalIndexGet(GI_LEVEL_SOUND_FIELD);
	}
			
	const LEVEL_DEFINITION * level_data = LevelDefinitionGet(nLevelDef);
	ASSERT_RETINVALID(level_data);
	SRVLEVELTYPE eSrvLevelType = level_data->eSrvLevelType;

	GAMEID idGame = INVALID_ID;

	// setup game options
	GAME_SETUP tSetup;
	tSetup.tGameVariant = tGameVariant;
	tSetup.tGameVariant.nDifficultyCurrent = nDifficulty;	
	
	switch (eSrvLevelType)
	{
	
		//----------------------------------------------------------------------------
		case SRVLEVELTYPE_MINITOWN:	
		case SRVLEVELTYPE_OPENWORLD:	
		{
		
			if (AppIsMultiplayer())
			{
			
				// multiplayer towns are forced to "default" difficulty
				nDifficulty = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
				tSetup.tGameVariant.nDifficultyCurrent = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
				
				// multiplayer towns will clear out any game flags we're looking for
				// for elite and role play servers
				CLEARBIT( tSetup.tGameVariant.dwGameVariantFlags, GV_ELITE );
				CLEARBIT( tSetup.tGameVariant.dwGameVariantFlags, GV_HARDCORE );
				CLEARBIT( tSetup.tGameVariant.dwGameVariantFlags, GV_LEAGUE );
				
			}
		
			// NOT a break here on purpose!!!!
					
		}
		
		//----------------------------------------------------------------------------
		case SRVLEVELTYPE_DEFAULT:
		case SRVLEVELTYPE_PVP_PRIVATE:
		{
		
			if (eSrvLevelType == SRVLEVELTYPE_PVP_PRIVATE) 
		{
		
				tSetup.nNumTeams = 2;
			}

			// setup level def for the dest
			LEVEL_TYPE tLevelTypeDest;
			tLevelTypeDest.nLevelDef = nLevelDef;
			tLevelTypeDest.nLevelArea = nLevelArea;
			tLevelTypeDest.nLevelDepth = 0;
			tLevelTypeDest.bPVPWorld = TESTBIT( tGameVariant.dwGameVariantFlags, GV_PVPWORLD );
						
			// get game id
			idGame = sGetDungeonGameToJoin(
				idAppClient, 
				tLevelTypeDest, 
				pdwSwitchInstanceFlags, 
				level_data->eSrvLevelType,
				nDifficulty,	// Only "action" level type games should have a difficulty set to the game
				tSetup); 
				
			// when going to instance games, record where we're going ... we do this because
			// we need to be able to handle two players zoning at the same time to either
			// an action level or town.  So, once player A gets a game to go to, we save that
			// in player A, so that later *this* frame for player B on the game server 
			// we can look to see if there are any party members going to the same place
			// and pick the same game instance
			ClientSystemSetLevelGoingTo( AppGetClientSystem(), idAppClient, idGame, &tLevelTypeDest );
			
			break;
			
		}
		
		//----------------------------------------------------------------------------
		case SRVLEVELTYPE_TUTORIAL:
		{
			idGame = sCreateNewGame(nDifficulty, GAME_SUBTYPE_TUTORIAL, tSetup);
			break;
		}

		//----------------------------------------------------------------------------				
		case SRVLEVELTYPE_BIGTOWN:
		{
			ASSERT(0);
			break;
		}

		//----------------------------------------------------------------------------		
		case SRVLEVELTYPE_CUSTOM:
		case SRVLEVELTYPE_PVP_CTF:
		{
					
			// for the time being, treat custom games as mini towns		
			tSetup.nNumTeams = 2;
			
			idGame = MiniTownGetAvailableGameId(idAppClient, nLevelDef, tSetup);
			break;
		}
		
	}
	TraceGameInfo("Got game id %I64x for appclient id %x with target level def %d",
		idGame, idAppClient, nLevelDef);
	
	return idGame;
}

//----------------------------------------------------------------------------
// return the gameid to join, creating the game if necessary
// based on character save data
//----------------------------------------------------------------------------
GAMEID WarpGetGameIdToJoinFromSaveData(
	APPCLIENTID idAppClient,
	BOOL bInitialJoin,
	CLIENT_SAVE_FILE *pClientSaveFile,
	int nDifficulty)
{
	ASSERT_RETINVALID(pClientSaveFile);

	UNIT_FILE tUnitFile;
	if (ClientSaveDataGetRootUnitBuffer( pClientSaveFile, &tUnitFile ) == FALSE)
	{
		return INVALID_ID;
	}

	// read header
	BOOL bInvalidFile = FALSE;
	UNIT_FILE_HEADER tHeader;
	if (UnitFileReadHeader( tUnitFile.pBuffer, tUnitFile.dwBufferSize, tHeader ) == FALSE)
	{
		bInvalidFile = TRUE;
	}
	
	// check player save version
	if (UnitFileHeaderCanBeLoaded( tHeader, PLAYERSAVE_MAGIC_NUMBER ) == FALSE)
	{
		bInvalidFile = TRUE;
	}

	// find the starting location we want to use
	const START_LOCATION *pStartLocation = UnitFileHeaderGetStartLocation( tHeader, nDifficulty );
	if (pStartLocation == NULL)
	{
		bInvalidFile = TRUE;
	}
	else
	{
		// check for level
		if ( AppIsHellgate() && pStartLocation->nLevelDef == INVALID_LINK)
		{
			bInvalidFile = TRUE;
		}
		
		// check for area
		if ( AppIsTugboat() && pStartLocation->nLevelAreaDef == INVALID_LINK)
		{
			bInvalidFile = TRUE;	
		}	
	}
	
	// setup game variant of what type of game we want to join
	GAME_VARIANT tGameVariant;
	tGameVariant.nDifficultyCurrent = nDifficulty;
	tGameVariant.dwGameVariantFlags = 0;
	StateArrayGetGameVariantFlags( 
		tHeader.nSavedStates, 
		tHeader.dwNumSavedStates, 
		&tGameVariant.dwGameVariantFlags );
	
	// do the warp
	if (bInvalidFile == TRUE)
	{
		return WarpGetGameIdToJoin(
			idAppClient, 
			bInitialJoin, 
			INVALID_LINK,
			INVALID_LINK,
			0,
			NULL,
			nDifficulty, 
			tGameVariant);
	}	
	else 
	{		
		return WarpGetGameIdToJoin( 
			idAppClient, 
			bInitialJoin, 								
			pStartLocation->nLevelDef,
			pStartLocation->nLevelAreaDef,
			tHeader.dwMagicNumber,
			NULL,
			nDifficulty,
			tGameVariant);
	}
			
}

//----------------------------------------------------------------------------
// return the gameid to join, creating the game if necessary
// based on a player name of an existing character file
//----------------------------------------------------------------------------
GAMEID WarpGetGameIdToJoinFromSinglePlayerSaveFile(
	APPCLIENTID idAppClient,
	BOOL bInitialJoin,
	const WCHAR * szPlayerName,
	int nDifficulty,
	DWORD dwNewPlayerFlags)	// see NEW_PLAYER_FLAGS
{	

	// construct path to file to load
	OS_PATH_CHAR szName[MAX_PATH];
	PStrCvt(szName, szPlayerName, MAX_PATH);
	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, MAX_PATH, OS_PATH_TEXT("%s%s.hg1"), FilePath_GetSystemPath(FILE_PATH_SAVE), szName);

	// setup client save file structure for loading
	CLIENT_SAVE_FILE tClientSaveFile;
	PStrCopy( tClientSaveFile.uszCharName, szPlayerName, MAX_CHARACTER_NAME );
	tClientSaveFile.eFormat = PSF_HIERARCHY;
	BYTE *pFileData = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, filename, &tClientSaveFile.dwBufferSize));
	tClientSaveFile.pBuffer = RichSaveFileRemoveHeader(pFileData, &tClientSaveFile.dwBufferSize);
	
	// if no data, join using default settings
	if (!tClientSaveFile.pBuffer)
	{
	
		// no player file, get game flags based off new player flags
		GAME_VARIANT tGameVariant;
		tGameVariant.nDifficultyCurrent = nDifficulty;
		tGameVariant.dwGameVariantFlags = s_GameVariantFlagsGetFromNewPlayerFlags( idAppClient, dwNewPlayerFlags );
		
		return WarpGetGameIdToJoin(
			idAppClient, 
			bInitialJoin, 
			INVALID_LINK,
			INVALID_LINK,
			0,
			NULL,
			nDifficulty, 
			tGameVariant);
	}

	// get the game to join from the save data loaded (not that we're ignoring the new
	// player flags and will get all the gameflag into to join out of the player save file)
	GAMEID idGame = WarpGetGameIdToJoinFromSaveData(
		idAppClient, 
		bInitialJoin, 
		&tClientSaveFile, 
		nDifficulty);

	// free file data
	FREE(NULL, pFileData);
	pFileData = NULL;
	
	// return the game we found
	return idGame;
	
}

//----------------------------------------------------------------------------
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int WarpGetDestinationLevelID(
	UNIT *pWarp)
{
	ASSERTX_RETINVALID( pWarp, "Expected unit" );
	return (LEVELID)UnitGetStat( pWarp, STATS_WARP_DEST_LEVEL_ID );
}	

//----------------------------------------------------------------------------
 
LEVELID s_WarpInitLink(
	UNIT *pWarp,
	UNITID idActivator,
	WARP_RESOLVE_PASS eWarpResolvePass)
{
	ASSERTX_RETINVALID( pWarp, "Expected unit" );
	ASSERTX_RETINVALID( UnitGetGenus( pWarp ) == GENUS_OBJECT, "Expected object" );
	ASSERTX_RETINVALID( ObjectIsWarp( pWarp ), "Expected warp" );
	GAME *pGame = UnitGetGame( pWarp );
	ASSERTX_RETINVALID( IS_SERVER( pGame ), "Server only" );	
	const UNIT_DATA *pObjectData = ObjectGetData( NULL, pWarp );
	LEVEL *pLevelCurrent = UnitGetLevel( pWarp );
	LEVELID idLevelCurrent = LevelGetID( pLevelCurrent );
	const LEVEL_DEFINITION *ptLevelDefCurrent = LevelDefinitionGet( pLevelCurrent );

	// do not do this if we've been here before	
	LEVELID idLevelDest = WarpGetDestinationLevelID( pWarp );
	if (idLevelDest != INVALID_ID)
	{
		return idLevelDest;
	}

	// we have a few different kind of warp types
	LEVEL_TYPE tLevelTypeDest;
	if (UnitIsA( pWarp, UNITTYPE_WARP_LEVEL ))
	{
		const UNIT_DATA *pTriggerData = ObjectGetData( NULL, pWarp );
		ASSERTX_RETINVALID( pTriggerData, "Expected object data" );

		// get opposite level name
		const char *pszLevelDestName = NULL;
		if (PStrICmp(ptLevelDefCurrent->pszName, pTriggerData->szTriggerString1, DEFAULT_INDEX_SIZE) != 0)
		{
			pszLevelDestName = pTriggerData->szTriggerString1;
		}
		else
		{
			pszLevelDestName = pTriggerData->szTriggerString2;
		}

		// check for invalid link
		ASSERTV_RETINVALID( 
			pszLevelDestName && PStrLen( pszLevelDestName, DEFAULT_INDEX_SIZE ) > 0, 
			"Warp '%s' on level '%s' has no destination level",
			UnitGetDevName( pWarp ),
			LevelGetDevName( pLevelCurrent ));

		// get the destination level definition
		tLevelTypeDest.nLevelDef = LevelGetDefinitionIndex( pszLevelDestName );
		ASSERTV_RETINVALID( 
			tLevelTypeDest.nLevelDef != INVALID_LINK, 
			"Unable to resolve warp '%s' on level '%s' dest level name '%s' to a level definition",
			UnitGetDevName( pWarp ),
			LevelGetDevName( pLevelCurrent ),
			pszLevelDestName ? pszLevelDestName : "UNKNOWN");
	
	}
	else if (UnitIsA( pWarp, UNITTYPE_WARP_NEXT_LEVEL ))
	{
		idLevelDest = pLevelCurrent->m_idLevelNext;
		ASSERTV_RETINVALID( 
			idLevelDest != INVALID_ID, 
			"Next level in '%s' should already exist",
			LevelGetDevName( pLevelCurrent ) );
	}
	else if (UnitIsA( pWarp, UNITTYPE_WARP_PREV_LEVEL ))
	{
		idLevelDest = pLevelCurrent->m_idLevelPrev;
		ASSERTV_RETINVALID( 
			idLevelDest != INVALID_ID, 
			"Prev level in '%s' should already exist",
			LevelGetDevName( pLevelCurrent ));
	}
	else if (UnitIsA( pWarp, UNITTYPE_WARP_ONE ))
	{
		
		// get level name
		ASSERTX_RETINVALID( pObjectData, "Expected object data" );
		const char *pszLevelName = pObjectData->szTriggerString1;
		ASSERTV_RETINVALID( 
			pszLevelName && PStrLen( pszLevelName ) > 0, 
			"Invalid warp one link for unit '%s' on level '%s'",
			UnitGetDevName( pWarp ),
			LevelGetDevName( pLevelCurrent ));
		
		// get level index
		tLevelTypeDest.nLevelDef = LevelGetDefinitionIndex( pszLevelName );
								
	}
	else if (UnitIsA( pWarp, UNITTYPE_WARP_RETURN_ONE ) ||
			 UnitIsA( pWarp, UNITTYPE_PASSAGE_EXIT ))
	{
		LEVELID idLevelOpposite = INVALID_LINK;
		
		// get the warp that created this level
		UNIT *pWarpCreator = UnitGetById( pGame, pLevelCurrent->m_idWarpCreator );
		if (pWarpCreator)
		{
			idLevelOpposite = UnitGetLevelID( pWarpCreator );
		}
		else
		{
		
			// we dont' want this to happen anymore cause we have the warp creators in the level now
			ASSERTV( 
				0, 
				"Level (%s) has no warp creator, using backup method to find level linking to this one",
				LevelGetDevName( pLevelCurrent ));

			// but as a backup error case, we'll keep it here in the hops nobody is ever stuck				
			// Note that we're passing in a type of "any" to the level find function, but
			// since we're looking for anything liked to this specific level instance I think
			// it's ok ... at least for now -Colin
			idLevelOpposite = s_LevelFindLevelOfTypeLinkingTo( pGame, gtLevelTypeAny, idLevelCurrent );				
			
		}
		
		ASSERTV_RETINVALID( 
			idLevelOpposite != INVALID_ID, 
			"Can't find spawning level for warp return one '%s' on level '%s'",
			UnitGetDevName( pWarp ),
			LevelGetDevName( pLevelCurrent ));		
		idLevelDest = idLevelOpposite;
		
	}
	else if (UnitIsA( pWarp, UNITTYPE_PASSAGE_ENTER ))
	{
		tLevelTypeDest.nLevelDef = s_AdventureGetPassageDest( pWarp );
	}
	else if (UnitIsA( pWarp, UNITTYPE_STAIRS_DOWN ))
	{
		//purely Tugboat
		UNIT *pPlayer = UnitGetById( pGame, idActivator );
		LEVEL *pLevel = UnitGetLevel( pWarp );
		MYTHOS_LEVELAREAS::FillOutLevelTypeForWarpObject( pLevel, pPlayer, &tLevelTypeDest, MYTHOS_LEVELAREAS::KLEVELAREA_LINK_NEXT );
	}
	else if (UnitIsA( pWarp, UNITTYPE_STAIRS_UP ))
	{
		//purely Tugboat
		UNIT *pPlayer = UnitGetById( pGame, idActivator );		
		LEVEL *pLevel = UnitGetLevel( pWarp );
		MYTHOS_LEVELAREAS::FillOutLevelTypeForWarpObject( pLevel, pPlayer, &tLevelTypeDest, MYTHOS_LEVELAREAS::KLEVELAREA_LINK_PREVIOUS );
		//tLevelTypeDest.nLevelArea = LevelGetLevelAreaID( pLevel );
		//tLevelTypeDest.nLevelDepth = LevelGetDepth( pLevel ) - 1;		
		//ASSERTX_RETINVALID( tLevelTypeDest.nLevelDepth >= 0, "Level depth must be 0 or greater" );		
	}
	else
	{
		// this is a warp, but it's not a warp that is tied to a level instance like a
		// town portal or a town portal return warp or a hellrift
		return INVALID_ID;
	}

	// create level if necessary
	if (idLevelDest == INVALID_ID)
	{
		ASSERTX_RETINVALID( LevelTypeIsValid( tLevelTypeDest ), "Expected level type destination for warp" );
		UNIT *pPlayer = UnitGetById( pGame, idActivator );
		
		// setup warp spec
		WARP_SPEC tSpec;
		tSpec.nLevelDefDest = tLevelTypeDest.nLevelDef;
		tSpec.nLevelAreaDest = tLevelTypeDest.nLevelArea;
		tSpec.nLevelDepthDest = tLevelTypeDest.nLevelDepth;	
		tSpec.nPVPWorld = tLevelTypeDest.bPVPWorld ? 1 : 0;
		
		int nDifficultyOffset = 0;
		if (AppIsTugboat())
		{		
			nDifficultyOffset = DungeonGetDifficulty( pGame, tSpec.nLevelAreaDest, pPlayer );
		}

		// what is the search method for this warp ... most warps use the level
		// links that are created on sublevel population, but some warps have created
		// funky one way link situations and need to search for a level that is assumed to
		// be there
		const UNIT_DATA *pWarpData = UnitGetData( pWarp );
		BOOL bSearchForExistingByType = UnitDataTestFlag( pWarpData, UNIT_DATA_FLAG_LINK_WARP_DEST_BY_LEVEL_TYPE );
		
		// create the destination level, do not populate it now
		idLevelDest = sLevelWarpGetOrCreateDestinationLevel( 
			pPlayer,
			tSpec,
			pWarp,
			nDifficultyOffset,
			bSearchForExistingByType);
		ASSERTV_RETINVALID( 
			idLevelDest != INVALID_ID, 
			"Unable to create new level for warp '%s' on level '%s'",
			UnitGetDevName( pWarp ),
			LevelGetDevName( pLevelCurrent ));
		
	}
		
	// tie the warp to this level instance
	UnitSetStat( pWarp, STATS_WARP_DEST_LEVEL_ID, idLevelDest );
	LEVEL *pLevelDest = LevelGetByID( pGame, idLevelDest );	
	ASSERTX_RETVAL( pLevelDest, idLevelDest, "Expected destination level" );
	UnitSetStat( pWarp, STATS_WARP_DEST_LEVEL_DEF, LevelGetDefinitionIndex( pLevelDest ) );
	UnitSetStat( pWarp, STATS_WARP_DEST_LEVEL_DEPTH, LevelGetDepth( pLevelDest ) );	
	//UnitSetStat( pWarp, STATS_WARP_DEST_LEVEL_AREA, tLevelTypeDest.nLevelArea );
	//UnitSetStat( pWarp, STATS_WARP_DEST_ITEM_KNOWING, tLevelTypeDest.nItemKnowingArea );

	// return the level link that was used
	return idLevelDest;
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WarpPartyMemberEnter(
	UNIT *pPlayer,
	UNIT *pWarp)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( pWarp, "Expected warp" );
	
#if !ISVERSION( CLIENT_ONLY_VERSION )  // doing this is such a rediculous mess!

	// who owns this portal
	PGUID guidOwner = UnitGetGUIDOwner( pWarp );
	
	// only the owner can use this portal
	PGUID guidPlayer = UnitGetGuid( pPlayer );
	if (guidOwner == guidPlayer)
	{
		GAME *pGame = UnitGetGame( pPlayer );
		
		// which party member does this warp go to
		PGUID guidDest = UnitGetGUIDStat( pWarp, STATS_WARP_PARTY_MEMBER_PORTAL_DEST_GUID );
		ASSERTX_RETFALSE( guidDest != INVALID_GUID, "Expected guid" );

		// can't warp to yourself
		ASSERTX_RETFALSE( guidDest != guidPlayer, "You can't warp to yourself" );

		// validate can still warp to them
		if (s_WarpValidatePartyMemberWarpTo( pPlayer, guidDest, TRUE ) == FALSE)
		{
			s_WarpCloseAll( pPlayer, WARP_TYPE_PARTY_MEMBER );
			return FALSE;
		}
		
		// if they're in this game, create the exit portal at the party member now
		GAMEID idGamePartyMember = PartyCacheGetPartyMemberGameId( pPlayer, guidDest );
		if (GameGetId( pGame ) == idGamePartyMember)
		{
			UNIT *pDestUnit = UnitGetByGuid( pGame, guidDest );
			if (pDestUnit)
			{
			
				// create portal at the destination unit
				s_WarpPartyMemberExitCreate( pPlayer, pDestUnit );
				
			}
		
		}
				
		// warp to that GUID
		return s_WarpToPartyMember( pPlayer, guidDest );
		
	}
#endif
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemovePartyMemberPortals(
	GAME *pGame,
	UNIT *pOwner,
	UNIT *pUnitOther,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId)
{

	// remove all party portals
	s_WarpCloseAll( pOwner, WARP_TYPE_PARTY_MEMBER );

	// remove hander
	UnitUnregisterEventHandler( pGame, pOwner, EVENT_EXIT_LIMBO, sRemovePartyMemberPortals );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRegisterPartyPortalRemoval(
	UNIT *pUnit)
{		
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// if we don't have an event already, register one
	if (UnitHasEventHandler( pGame, pUnit, EVENT_EXIT_LIMBO, sRemovePartyMemberPortals ) == FALSE)
	{
		UnitRegisterEventHandler( pGame, pUnit, EVENT_EXIT_LIMBO, sRemovePartyMemberPortals, NULL );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendPartyMemberWarpToRestricted( 
	UNIT *pPlayer,
	WARP_RESTRICTED_REASON eReason)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// setup message
	MSG_SCMD_WARP_RESTRICTED tMessage;
	tMessage.nReason = eReason;
	
	// send
	GAMECLIENT *pClient = UnitGetClient(pPlayer);
	if(pClient)
	{
		s_SendMessage( UnitGetGame(pPlayer), ClientGetId(pClient),
			SCMD_WARP_RESTRICTED, &tMessage );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerHasWaypointTo(
	UNIT *pPlayer,
	int nLevelDefDest)
{
	BOOL bValid = TRUE;

	// what level is the party member we want to warp to on
	if (nLevelDefDest == INVALID_LINK)
	{
		return FALSE;  // just quit out, no error etc.
	}
	else
	{
		// if they have the waypoint of this actual level, then it's cool
		GAME *pGame = UnitGetGame(pPlayer);
		int nDifficulty = DifficultyGetCurrent(pPlayer);

		if (WaypointIsMarked( pGame, pPlayer, nLevelDefDest, nDifficulty ) == FALSE)
		{
			// ok, find the "root" level along the previous level chain from this level
			int nLevelDefRootPrev = LevelFindConnectedRootLevelDefinition( nLevelDefDest, LCD_PREV );
			ASSERTX_RETFALSE( nLevelDefRootPrev != INVALID_LINK, "Expected prev root level definition" );

			// must have a waypoint to the root level that is "previous" of the level def destination
			bValid = WaypointIsMarked( pGame, pPlayer, nLevelDefRootPrev, nDifficulty );
		}
	}

	return bValid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WarpValidatePartyMemberWarpTo(
	UNIT *pPlayer,
	PGUID guidPartyMemberDest,
	BOOL bInformClientOnRestricted)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETFALSE( guidPartyMemberDest != INVALID_GUID, "Expected destination GUID" );
	WARP_RESTRICTED_REASON eRestrictedReason = WRR_OK;

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (AppIsHellgate())
	{
		// what level is the party member we want to warp to on
		int nLevelDefDest = PartyCacheGetPartyMemberLevelDefinition( pPlayer, guidPartyMemberDest );
		if (nLevelDefDest == INVALID_LINK)
		{
			return FALSE;  // just quit out, no error etc.
		}
		int nAct = LevelDefinitionGetAct( nLevelDefDest );
		eRestrictedReason = ActIsAvailableToUnit( pPlayer, nAct );
		if (eRestrictedReason == WRR_ACT_TRIAL_ACCOUNT)
		{
			// KCK: Kind of a weird syntax, but as we only have strings for this error code from that function,
			// we want to try to get a more specific error for everything but this case.
			eRestrictedReason = WRR_ACT_TRIAL_ACCOUNT;
		}
		else if (UnitPvPIsEnabled(pPlayer))
		{
			eRestrictedReason = WRR_PROHIBITED_IN_PVP;
		}
		else if (!PlayerHasWaypointTo(pPlayer, nLevelDefDest))
		{
			eRestrictedReason = WRR_NO_WAYPOINT;
		}
		else
		{		
			// check restrictions of this player and the specific level they want to go to
			eRestrictedReason = LevelDefIsAvailableToUnit( pPlayer, nLevelDefDest );
		}
	}
	else
	{
		// Mythos

		// Party warps are only valid if we're in the same pvp world
		if (PartyCacheGetPartyMemberPVPWorld( pPlayer, guidPartyMemberDest ) != PlayerIsInPVPWorld( pPlayer ))
		{
			eRestrictedReason = WRR_NO_WAYPOINT;
		}
	}

	// if restricted, we tell the client if instructed to
	if (eRestrictedReason != WRR_OK && bInformClientOnRestricted == TRUE)
	{
		sSendPartyMemberWarpToRestricted( pPlayer, eRestrictedReason );
	}
#endif
	return eRestrictedReason == WRR_OK; // no valid reason for restriction, so... not restricted, in other words OK TO WARP
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_WarpOpen(
	UNIT *pPlayer,
	WARP_TYPE eWarpType)
{
	ASSERTX_RETNULL( pPlayer, "Expected unit" );
	ASSERTV_RETNULL( UnitIsPlayer( pPlayer ), "Expected player (%s)", UnitGetDevName( pPlayer ) );
	ASSERTV_RETNULL( eWarpType != WARP_TYPE_INVALID, "Invalid warp type (%d)", eWarpType );

	// close any previous warp that is open
	s_WarpCloseAll( pPlayer, eWarpType );

	// create the portal	
	UNIT *pPortal = NULL;
	switch (eWarpType)
	{
	
		//----------------------------------------------------------------------------
		case WARP_TYPE_PARTY_MEMBER:
		{
			pPortal = s_WarpCreatePortalAround( 
				pPlayer, 
				GI_OBJECT_PORTAL_PARTY_MEMBER_ENTER, 
				0,
				pPlayer,
				STATS_WARP_PARTY_MEMBER_ENTER_UNITID);
			break;		
		}
		
		//----------------------------------------------------------------------------
		case WARP_TYPE_RECALL:
		{
			// create portal
			pPortal = s_WarpCreatePortalAround( 
				pPlayer, 
				GI_OBJECT_PORTAL_RECALL_ENTER, 
				0,
				pPlayer,
				STATS_WARP_RECALL_ENTER_UNITID);
				
			// set destination on warp
			if (pPortal)
			{
				int nLevelDefDest = UnitGetReturnLevelDefinition( pPlayer );
				UnitSetStat( pPortal, STATS_WARP_RECALL_ENTER_DEST_LEVEL_DEF, nLevelDefDest );
			}
						
			break;
		}
		
	}

	if (pPortal)
	{
	
		// record who created this portal
		UnitSetGUIDOwner( pPortal, pPlayer );

	}

	return pPortal;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreePortalFromStat(
	UNIT *pPlayer,
	int nStat)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( nStat != INVALID_LINK, "Expected stat" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	
	// get unitid of portal we have open
	UNITID idPortal = UnitGetStat( pPlayer, nStat );
	if (idPortal != INVALID_ID)
	{
		UNIT *pPortal = UnitGetById( pGame, idPortal );
		ASSERTX( pPortal, "Expected portal unit" );
		
		// destroy the portal (we delay the free because typically we close them while triggering them)
		if (pPortal)
		{
			UnitFree( pPortal, UFF_SEND_TO_CLIENTS | UFF_DELAYED );
		}
				
		// clear stat
		UnitClearStat( pPlayer, nStat, 0 );
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WarpCloseAll(
	UNIT *pPlayer, 
	WARP_TYPE eWarpType)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player (%s)", UnitGetDevName( pPlayer ) );
	ASSERTV_RETURN( eWarpType != WARP_TYPE_INVALID, "Invalid warp type (%d)", eWarpType );

	switch (eWarpType)
	{
	
		//----------------------------------------------------------------------------	
		case WARP_TYPE_RECALL:
		{
			sFreePortalFromStat( pPlayer, STATS_WARP_RECALL_ENTER_UNITID );
			sFreePortalFromStat( pPlayer, STATS_WARP_RECALL_EXIT_UNITID );
			break;
		}
		
		//----------------------------------------------------------------------------
		case WARP_TYPE_PARTY_MEMBER:
		{

			sFreePortalFromStat( pPlayer, STATS_WARP_PARTY_MEMBER_ENTER_UNITID );
			sFreePortalFromStat( pPlayer, STATS_WARP_PARTY_MEMBER_EXIT_UNITID );
			break;
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WarpPartyMemberOpen(
	UNIT *pUnit,
	PGUID guidPartyMember)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( guidPartyMember != INVALID_GUID, "Expected party member guid" );

	// validate we can in fact warp to the party member
	if (s_WarpValidatePartyMemberWarpTo( pUnit, guidPartyMember, TRUE ) == FALSE)
	{
		return;
	}

	//Validate that target is a party member and has a game to warp to.
	GAMEID idGamePartyMember = PartyCacheGetPartyMemberGameId( pUnit, guidPartyMember );
	ASSERTX_RETURN(
		GAMEID_IS_INVALID(idGamePartyMember) == FALSE,
		"Party member nonexistent or gameless.");

	// close any previous warp that is already open
	s_WarpCloseAll( pUnit, WARP_TYPE_PARTY_MEMBER );

	// we should not have a portal open to any party member at this time (we can only have one)
	ASSERTX( UnitGetStat( pUnit, STATS_WARP_PARTY_MEMBER_ENTER_UNITID ) == INVALID_ID, "Already has a portal to a party member" );

	// must be in a party
	if (UnitGetPartyId( pUnit ) == INVALID_ID)
	{
		return;
	}
					
	// create a portal
	UNIT *pPortalEnter = s_WarpOpen( pUnit, WARP_TYPE_PARTY_MEMBER );
	if (pPortalEnter)
	{
		UNITID idPortalEnter = UnitGetId( pPortalEnter);
		
		// record that we have a party member portal open now
		ASSERTX( (UNITID)UnitGetStat( pUnit, STATS_WARP_PARTY_MEMBER_ENTER_UNITID ) == idPortalEnter, "Party portal not recorded properly" );
		
		// save the guid that this portal will lead to when used
		UnitSetGUIDStat( pPortalEnter, STATS_WARP_PARTY_MEMBER_PORTAL_DEST_GUID, guidPartyMember );
	
		// we'll remove this portal if we go into limbo again to make sure it goes away
		sRegisterPartyPortalRemoval( pUnit );
									
	}
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClosePartyPortalsToGUID(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	PGUID guidPartyMember = *(PGUID *)pCallbackData;
	
	// if we have any party portals open to the party member in question, close them
	UNITID idPortal = UnitGetStat( pPlayer, STATS_WARP_PARTY_MEMBER_ENTER_UNITID );
	if (idPortal != INVALID_ID)
	{
		GAME *pGame = UnitGetGame( pPlayer );
		UNIT *pPortal = UnitGetById( pGame, idPortal );
		if (pPortal)
		{
			PGUID guidPartyMemberDest = UnitGetGUIDStat( pPortal, STATS_WARP_PARTY_MEMBER_PORTAL_DEST_GUID );
			if (guidPartyMemberDest == guidPartyMember)
			{
				// we can close all the party portals because you can only have one open at
				// a time (for now) -cday
				s_WarpCloseAll( pPlayer, WARP_TYPE_PARTY_MEMBER );
			}
			
		}
		
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WarpPartyMemberCloseLeadingTo(
	GAME *pGame,
	PGUID guidDest)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( guidDest != INVALID_GUID, "Expected guid" );
	
	// close any party portals that are pointing to this unit
	PlayersIterate( pGame, sClosePartyPortalsToGUID, &guidDest );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_WarpPartyMemberExitCreate( 
	UNIT *pUnit, 
	UNIT *pDest)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	ASSERTX_RETNULL( pDest, "Expected dest unit" );
	GAME *pGame = UnitGetGame( pUnit );
	UNIT *pUnitToCreatePortalAt = pDest;
	DWORD dwWarpCreateFlags = 0;
	
	// see if we already have one
	UNITID idPortal = UnitGetStat( pUnit, STATS_WARP_PARTY_MEMBER_EXIT_UNITID );
	if (idPortal != INVALID_ID)
	{
		UNIT *pPortal = UnitGetById( pGame, idPortal );
		if (pPortal)
		{
			return pPortal;
		}
	}
	
	// if the dest unit is in a sublevel that does not allow portals to be opened to, we will
	// create it at the sublevel entrance
	SUBLEVEL *pSubLevelDestUnit = UnitGetSubLevel( pDest );
	ASSERTX_RETNULL( pSubLevelDestUnit, "Expected sublevel" );
	const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevelDestUnit );
	if (pSubLevelDef->bPartyPortalsAtEntrance == TRUE)
	{
		const SUBLEVEL_DOORWAY *pDoorway = s_SubLevelGetDoorway( pSubLevelDestUnit, SD_ENTRANCE );
		ASSERTX_RETNULL( pDoorway, "Expected doorway for sublevel entrance" );
		UNIT *pEntrance = UnitGetById( pGame, pDoorway->idDoorway );
		if (pEntrance == NULL)
		{
			return NULL;  // can't do it
		}
		
		// create portal at entrance instead of party member
		pUnitToCreatePortalAt = pEntrance;

	}
	else
	{
		// create portal at the exact location of the dest unit
		SETBIT( dwWarpCreateFlags, WCF_EXACT_DESTINATION_UNIT_LOCATION );		
	}

	// if the dest player is dead we will create a portal to their headstone
	// not their player/ghost location
	if (UnitIsPlayer( pUnitToCreatePortalAt ) && UnitIsGhost( pUnitToCreatePortalAt ))
	{
		UNIT *pHeadstone = PlayerGetMostRecentHeadstone( pUnitToCreatePortalAt );
		if (pHeadstone)
		{
			pUnitToCreatePortalAt = pHeadstone;
		}
	}

	// create the portal
	UNIT *pPortal = s_WarpCreatePortalAround( 
							pUnitToCreatePortalAt, 
							GI_OBJECT_PORTAL_PARTY_MEMBER_EXIT, 
							dwWarpCreateFlags, 
							pUnit, 
							STATS_WARP_PARTY_MEMBER_EXIT_UNITID);
	if (pPortal)
	{
	
		// when this player comes out of limbo we want to remove this portal
		sRegisterPartyPortalRemoval( pUnit );

		// the "destination" of this portal is the unit that created it
		UnitSetGUIDStat( pPortal, STATS_WARP_PARTY_MEMBER_PORTAL_DEST_GUID, UnitGetGuid( pUnit ));
		
	}
		
	return pPortal;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WarpPartyMemberGetExitPortalLocation(
	UNIT *pOwner,
	ROOM **pRoom,
	VECTOR *pvPosition,
	VECTOR *pvDirection,
	VECTOR *pvUp)
{
	ASSERTX_RETFALSE( pOwner, "Expected owner" );
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( pvPosition, "Expected pos vector" );
	ASSERTX_RETFALSE( pvDirection, "Expected dir vector" );
	ASSERTX_RETFALSE( pvUp, "Expected up vector" );
	GAME *pGame = UnitGetGame( pOwner );

	// get the portal
	UNITID idPortal = UnitGetStat( pOwner, STATS_WARP_PARTY_MEMBER_EXIT_UNITID );
	UNIT *pPortal = UnitGetById( pGame, idPortal );
	if (pPortal)
	{
		*pRoom = UnitGetRoom( pPortal );
		*pvPosition = UnitGetPosition( pPortal );
		*pvDirection = UnitGetFaceDirection( pPortal, FALSE );
		*pvUp = UnitGetUpDirection( pPortal );
		
		return TRUE;
		
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WarpToPartyMember( 
	UNIT *pUnit, 
	PGUID guidPartyMember)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( guidPartyMember != INVALID_GUID, "Expected party member guid" );

	// get client info
	GAMECLIENT *pClient = UnitGetClient( pUnit );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	
	// get the party game of the unit that wants to warp
	PARTYID idParty = UnitGetPartyId( pUnit );
	if( idParty != INVALID_ID)
	{
		GAMEID idGameDest = PartyCacheGetPartyMemberGameId( pUnit, guidPartyMember );
		if (idGameDest == INVALID_ID)
		{
			return FALSE;  // can't go to that game (member is gone, not in party, etc)
		}	

		// fill out warp spec
		WARP_SPEC tWarpSpec;
		tWarpSpec.nLevelDefDest = UnitGetReturnLevelDefinition( pUnit );  // not really where we're going, but we gotta have somewhere to go incase of a failure
		tWarpSpec.idGameOverride = idGameDest;  // go to the destination game of the player
		tWarpSpec.guidArriveAt = guidPartyMember;
		SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_GUID_BIT );
		SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_VIA_PARTY_PORTAL_BIT );

		// validate we can still warp to them
		if (s_WarpValidatePartyMemberWarpTo( pUnit, guidPartyMember, TRUE ) == FALSE)
		{
			return FALSE;
		}
				
		// see if party member is in this game, we just need to warp to them
		GAME *pGame = UnitGetGame( pUnit );
		if (idGameDest == pGame->m_idGame)
		{
		
			// find the unit that we're warping to
			UNIT *pPartyMember = UnitGetByGuid( pGame, guidPartyMember );
			
			// if no party member left in this game, forget it
			if (pPartyMember == NULL)
			{
				return FALSE;
			}

			// see if it's just a warp inside the same level
			if (UnitsAreInSameLevel( pUnit, pPartyMember ))
			{			

				// get where we're going 
				ROOM *pRoom = NULL;
				VECTOR vPosition;
				VECTOR vDirection;
				VECTOR vUp;
				if (!s_WarpPartyMemberGetExitPortalLocation( pUnit, &pRoom, &vPosition, &vDirection, &vUp ))
				{
					pRoom = UnitGetRoom( pPartyMember );
					vPosition = UnitGetPosition( pPartyMember );
					vDirection = UnitGetFaceDirection( pPartyMember, FALSE );
					vUp = UnitGetUpDirection( pPartyMember );					
				}
									
				// just warp there ... note we are not making the player safe for this
				// warp because they're not using the warp feature as a means of joining up
				// with their party, they are using it as a kind of travel "cheat"
				DWORD dwWarpFlags = 0;					
				UnitWarp( 
					pUnit, 
					pRoom,
					vPosition,
					vDirection,
					vUp,
					dwWarpFlags);
											
				// remove all warping portals
				s_WarpCloseAll( pUnit, WARP_TYPE_PARTY_MEMBER );
				
				// don't continue warp logic
				return FALSE;
					
			}
			else
			{
			
				// setup warp spec to go to the level the party member is on ... could make this
				// the actual level instance id when we have the ability to do more instanced level warping
				LEVEL *pLevelDest = UnitGetLevel( pPartyMember );
				tWarpSpec.nLevelDefDest = LevelGetDefinitionIndex( pLevelDest );

			}
						
		}
		
		// do the warp
		return s_WarpToLevelOrInstance( pUnit, tWarpSpec );
			
	}

#endif		
	
	return FALSE;  // didn't warp
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_WarpEnterWarp(
	UNIT *pPlayer,
	UNIT *pWarp)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player" );
	ASSERTX_RETURN( pWarp, "Expected unit" );
	ASSERTX_RETURN( ObjectIsWarp( pWarp ), "Expected warp" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// ghosts cannot enter warps (don't think we need this anymore, but can't hurt to leave in)
	if (UnitIsGhost( pPlayer ) == TRUE)
	{
		return;
	}

	if (UnitHasExactState( pPlayer, STATE_NO_TRIGGER_WARPS ))
	{
		return;
	}
	
	// fill out warp spec info
	WARP_SPEC tSpec;
		
	// town portal logic (Colin - unify this with the else case below)
	if (ObjectIsPortalToTown( pWarp ))
	{
	
		// try level def method (Hellgate)
		LEVEL_TYPE tLevelTypeDest = TownPortalGetDestination( pWarp );
		if (tLevelTypeDest.nLevelDef != INVALID_LINK)
		{
					
			// setup arrival destination
			tSpec.nLevelDefDest = tLevelTypeDest.nLevelDef;
			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_WARP_RETURN_BIT );
			
			// warp
			s_WarpToLevelOrInstance( pPlayer, tSpec );
				
		}
		else
		{		
		
			// try level area and depth method (Tugboat)
			if (tLevelTypeDest.nLevelArea != INVALID_LINK)
			{
						
				// setup arrival destination
				tSpec.nLevelAreaDest = tLevelTypeDest.nLevelArea;
				tSpec.nLevelDepthDest = tLevelTypeDest.nLevelDepth;	
				tSpec.nPVPWorld =tLevelTypeDest.bPVPWorld ? 1 : 0;
				SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_WARP_RETURN_BIT );
				
				// warp
				s_WarpToLevelOrInstance( pPlayer, tSpec );
										
			}
			
		}
				
	}
	else
	{
		
		// player should arrive in the new level at the portal that leads there
		SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_FROM_LEVEL_PORTAL );	
		
		// get the destination of the warp
		LEVELID idLevelDest = WarpGetDestinationLevelID( pWarp );
		if (idLevelDest == INVALID_ID )			  
		{			
			if( AppIsHellgate() ||
				( AppIsTugboat() && UnitGetWarpToLevelAreaID( pWarp ) == INVALID_ID ) )
			{
				// don't check for trigger for a little while so the player can walk away from the warp
				UnitAllowTriggers( pPlayer, FALSE, 3.0f );
				
				LEVEL *pLevel = UnitGetLevel( pWarp );
				const int MAX_MESSAGE = 1024;
				char szMessage[ MAX_MESSAGE ];
				PStrPrintf( 
					szMessage, 
					MAX_MESSAGE, 
					"Warp (%s) [id=%d] on level '%s' has no destination level id stat",
					UnitGetDevName( pWarp ),
					UnitGetId( pWarp ),
					LevelGetDevName( pLevel ));
				ASSERTX_RETURN( 0, szMessage );
			}
		}
		// get our current level
		LEVEL *pLevelCurrent = UnitGetLevel( pPlayer );
		const LEVEL_DEFINITION *ptLevelDefCurrent = LevelDefinitionGet( pLevelCurrent );
		LEVEL *pLevelDest = LevelGetByID( pGame, idLevelDest );

		if( AppIsTugboat() )
		{
			if( UnitGetLevel( pPlayer ) )
			{
				tSpec.nLevelAreaPrev = UnitGetLevelAreaID( pPlayer );
			}
		}
		if( AppIsTugboat() &&
			pLevelCurrent &&
			MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( LevelGetLevelAreaID( pLevelCurrent )) )
		{
			//entering a warp from a static world, we reset the return point
			PlayerSetRespawnLocation( KRESPAWN_TYPE_RETURNPOINT, pPlayer, NULL, pLevelCurrent );
		}
		if( AppIsTugboat() &&
			pLevelDest == NULL ) //YES, tugboat can have levels here that aren't created..
		{

			//reason being that it's a warp to a new area. To do that we need to act like a town portal...
			// setup arrival destination			
			if( pWarp &&
				UnitGetWarpToLevelAreaID( pWarp ) != INVALID_ID )
			{
				tSpec.nLevelAreaDest = UnitGetWarpToLevelAreaID( pWarp );
				tSpec.nLevelDepthDest = UnitGetWarpToDepth( pWarp );
				if( tSpec.nLevelDepthDest < 0 )
				{
					tSpec.nLevelDepthDest = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( tSpec.nLevelAreaDest ) - 1;
				}
			}
			else
			{
				tSpec.nLevelAreaDest = MYTHOS_LEVELAREAS::GetLevelAreaIDByStairs( pWarp );
				tSpec.nLevelDepthDest = 0;

			}						
			tSpec.nPVPWorld  = ( PlayerIsInPVPWorld( pPlayer ) ? 1 : 0 );


			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );				
			// warp
			s_WarpToLevelOrInstance( pPlayer, tSpec );
			return;
		}
		else if( AppIsTugboat() )
		{
			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
		}

		const LEVEL_DEFINITION *ptLevelDefDest = LevelDefinitionGet( pLevelDest );
		tSpec.nLevelDefDest = LevelGetDefinitionIndex( pLevelDest );

		// if this level is not available in this version of the game, do not warp
		if (LevelDefIsAvailableToUnit( pPlayer, tSpec.nLevelDefDest ) != WRR_OK)
		{
			return;
		}

		// validate that you can use this warp now
		//if (sCheckInstanceRestrictions( pPlayer, pWarp, pLevelDest ) == INSTANCE_RESTRICTED)
		//{
		//	return;
		//}
						

		
		if( AppIsTugboat() )
		{
			tSpec.nLevelAreaDest = LevelGetLevelAreaID( pLevelDest );
			tSpec.nLevelDepthDest = LevelGetDepth( pLevelDest );
			tSpec.nPVPWorld = LevelGetPVPWorld( pLevelDest ) ? 1 : 0;
		}
		tSpec.nDifficulty = pGame->nDifficulty;
		


		// does the warp require an instance change
		if (sIsInstanceChangeRequired( ptLevelDefDest, ptLevelDefCurrent ))
		{	

			WarpToInstance( pPlayer, tSpec );							
		}
		else
		{	
		
			// we want to go to an existing level in this game instance
			tSpec.idLevelDest = idLevelDest;

			// warp to level in same game instance
			s_LevelWarp( pPlayer, tSpec );
			
		}
	
	}
#else
	REF( pPlayer );
	REF( pWarp );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void WarpGetArrivePosition(	
	LEVEL *pLevel,
	const VECTOR &vWarpPosition,
	const VECTOR &vWarpDirection,
	float flWarpOutDistance,
	WARP_DIRECTION eDirection,
	BOOL bFaceAfterWarp,
	WARP_ARRIVE_POSITION *pArrivePosition)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pArrivePosition, "Expected warp arrive position struct" );
	GAME *pGame = LevelGetGame( pLevel );

	// get warp out distance from warp (use default if not provided)
	if (flWarpOutDistance == -1.0f)
	{
		flWarpOutDistance = AppIsHellgate() ? OUT_OF_WARP_DISTANCE : OUT_OF_WARP_DISTANCE_TUGBOAT;
	}
	
	// get direction of portal	
	pArrivePosition->vDirection = vWarpDirection;
	VectorNormalize( pArrivePosition->vDirection );

	// reverse direction if requested
	if (eDirection == WD_BEHIND)
	{
		pArrivePosition->vDirection = -pArrivePosition->vDirection;
	}

	// get position
	pArrivePosition->vPosition = vWarpPosition + (pArrivePosition->vDirection * flWarpOutDistance);
	float OriginalZ = pArrivePosition->vPosition.fZ;

	// use path nodes to find nearest valid location
	ROOM *pRoomDest = NULL;
	DWORD dwFreePathNodeFlags = 0;
	SETBIT( dwFreePathNodeFlags, NPN_ALLOW_OCCUPIED_NODES );
	ROOM_PATH_NODE *pDestinationNode = RoomGetNearestFreePathNode(
		pGame, 
		pLevel,
		pArrivePosition->vPosition,
		&pRoomDest, 
		NULL, 
		0.0f, 
		0.0f, 
		NULL,
		dwFreePathNodeFlags);	
	pArrivePosition->vPosition = RoomPathNodeGetExactWorldPosition( pGame, pRoomDest, pDestinationNode );
	ASSERTX_RETURN( pRoomDest, "No room found for warp arrive position" );
	if( AppIsTugboat() )
	{
		pArrivePosition->vPosition.fZ = OriginalZ;
	}
	// our final direction is from the portal to the position we found
	pArrivePosition->vDirection = pArrivePosition->vPosition - vWarpPosition;
	pArrivePosition->vDirection.fZ = 0.0f;  // remove z component
	VectorNormalize( pArrivePosition->vDirection );
	
	// reverse the direction if instructed (it makes sense to arrive facing ladders for instance)
	if (bFaceAfterWarp)
	{
		pArrivePosition->vDirection.fX = -pArrivePosition->vDirection.fX;
		pArrivePosition->vDirection.fY = -pArrivePosition->vDirection.fY;
		pArrivePosition->vDirection.fZ = -pArrivePosition->vDirection.fZ;
	}
	
	pArrivePosition->nRotation = VectorDirectionToAngleInt( pArrivePosition->vDirection );
	pArrivePosition->pRoom = pRoomDest;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM *WarpGetArrivePosition( 
	UNIT *pWarp,
	VECTOR *pvPosition, 
	VECTOR *pvDirection,
	WARP_DIRECTION eDirection,
	int * pnRotation)
{
	ASSERTX_RETNULL( pWarp, "Expected unit" );
	ASSERTX_RETNULL( pvPosition, "Expected position" );
	ASSERTX_RETNULL( pvDirection, "Expected direction" );
	const UNIT_DATA *pUnitData = UnitGetData( pWarp );
	const VECTOR &vWarpPosition = UnitGetPosition( pWarp );
	VECTOR vWarpDirection = UnitGetFaceDirection( pWarp, FALSE );
	LEVEL *pLevel = UnitGetLevel( pWarp );

	// get params for warp
	BOOL bFaceAfterWarp = UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_FACE_AFTER_WARP);
	
	WARP_ARRIVE_POSITION tArrivePosition;
	WarpGetArrivePosition( 
		pLevel,
		vWarpPosition, 
		vWarpDirection, 
		pUnitData->flWarpOutDistance, 
		eDirection, 
		bFaceAfterWarp,
		&tArrivePosition);

	// copy results to params
	*pvPosition = tArrivePosition.vPosition;
	*pvDirection = tArrivePosition.vDirection;
	if (pnRotation)
	{
		*pnRotation = tArrivePosition.nRotation;
	}

	return tArrivePosition.pRoom;
				
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL_TYPE WarpSpecGetLevelType(
	const WARP_SPEC &tSpec)
{
	LEVEL_TYPE tLevelType;
	
	if (AppIsHellgate())
	{
		tLevelType.nLevelDef = tSpec.nLevelDefDest;
	}
	else if (AppIsTugboat())
	{
		tLevelType.nLevelArea = tSpec.nLevelAreaDest;
		tLevelType.nLevelDepth = tSpec.nLevelDepthDest;	
		tLevelType.bPVPWorld = ( tSpec.nPVPWorld != 0 );
	}
	
	return tLevelType;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
OPERATE_RESULT WarpCanOperate( 
	UNIT *pPlayer, 
	UNIT *pWarp)
{
	ASSERTX_RETVAL( pPlayer, OPERATE_RESULT_FORBIDDEN, "Expected unit" );
	ASSERTX_RETVAL( UnitIsPlayer( pPlayer ), OPERATE_RESULT_FORBIDDEN, "Expected player" );
	ASSERTX_RETVAL( pWarp, OPERATE_RESULT_FORBIDDEN, "Expected warp unit" );
	ASSERTX_RETVAL( ObjectTriggerIsWarp( pWarp ), OPERATE_RESULT_FORBIDDEN, "Expected warp" );		
	return ObjectCanTrigger( pPlayer, pWarp );
}

//----------------------------------------------------------------------------
struct VALIDATE_WARP_LOC_DATA
{
	const VECTOR *pvPosition;
	DWORD dwWarpCreateFlags;
	BOOL bRestricted;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sUnitValidatePortalLocation(
	UNIT *pUnit,
	void *pCallbackData)
{
	VALIDATE_WARP_LOC_DATA *pValidateData = (VALIDATE_WARP_LOC_DATA *)pCallbackData;
	const VECTOR &vUnitPos = UnitGetPosition( pUnit );
	
	// get distance to this unit from the position in question
	float flDistSq = VectorDistanceSquaredXY( vUnitPos, *pValidateData->pvPosition );
	
	// this location can not be near any level warp portals
	if (TESTBIT( pValidateData->dwWarpCreateFlags, WCF_CLEAR_FROM_WARPS ))
	{
		if (ObjectIsWarp( pUnit ))
		{
			if (flDistSq <= WARP_MIN_DISTANCE_FROM_WARPS)
			{
				pValidateData->bRestricted = TRUE;
			}
		}
		
	}
	
	// this location can not be near any start locations
	if (TESTBIT( pValidateData->dwWarpCreateFlags, WCF_CLEAR_FROM_START_LOCATIONS ))
	{
		if (UnitIsA( pUnit, UNITTYPE_START_LOCATION ))
		{
			if (flDistSq <= WARP_MIN_DISTANCE_FROM_START_LOCS)
			{
				pValidateData->bRestricted = TRUE;
			}
		}
	}

	// stop iterating if this is restricted
	if (pValidateData->bRestricted == TRUE)
	{	
		return RIU_STOP;
	}
	
	return RIU_CONTINUE;  // keep checking units in this room
					
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sValidatePortalLocation( 
	UNIT *pCreator,
	const VECTOR *pvPosition,
	const ROOM *pRoom,
	DWORD dwWarpCreateFlags)
{
	ASSERTX_RETFALSE( pCreator, "Expected unit" );
	ASSERTX_RETFALSE( pvPosition, "Expected position" );
	ASSERTX_RETFALSE( pRoom, "Expected room" );

	// if no options to check, it's OK with us!
	if (dwWarpCreateFlags == 0)
	{
		return TRUE;
	}

	// if the creator is not a player, we say the game knows what it's doing, lol
	if (UnitIsPlayer( pCreator ) == FALSE)
	{
		return TRUE;
	}
	UNIT *pPlayer = pCreator;
			
	// get the rooms that this client knows about because it's a good list of the rooms that
	// are close by to this player
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETFALSE( pClient, "Expected client" );

	// setup callback for the room iterate units
	VALIDATE_WARP_LOC_DATA tValidateData;
	tValidateData.pvPosition = pvPosition;
	tValidateData.dwWarpCreateFlags = dwWarpCreateFlags;
	tValidateData.bRestricted = FALSE;

	// setup target types we'll search (expand this as needed)
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
			
	// iterate the rooms they know about
	ROOM * room = ClientGetFirstRoomKnown(pClient, pPlayer);
	while (room)
	{
		if (RoomIterateUnits(room, eTargetTypes, sUnitValidatePortalLocation, &tValidateData) == RIU_STOP)
		{
			break;
		}
		room = ClientGetNextRoomKnown(pClient, pPlayer, room);
	}

	// return the result
	return !tValidateData.bRestricted;
					
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sFindPortalLocationAroundAnchor(
	UNIT *pAnchor,
	UNIT *pCreator,
	VECTOR *pvWorldPosition,
	ROOM **pRoom,
	DWORD dwWarpCreateFlags)
{
	ASSERTX_RETFALSE( pAnchor, "Expected unit" );
	ASSERTX_RETFALSE( pvWorldPosition, "Expected vector" );
	ASSERTX_RETFALSE( pRoom, "Expected room pointer" );
	GAME *pGame = UnitGetGame( pAnchor );
	
	// initialize warp position to the player anchor themselves
	*pvWorldPosition = UnitGetPosition( pAnchor );
	*pRoom = UnitGetRoom( pAnchor );
	
	// if instructed to only use the position of the anchor, just bail out
	if (TESTBIT( dwWarpCreateFlags, WCF_EXACT_DESTINATION_UNIT_LOCATION ))
	{
		return TRUE;
	}
	
	// get direction for query
	VECTOR vDirection = UnitGetDirectionXY( pAnchor );
	ROOM *pRoomAnchor = UnitGetRoom( pAnchor );
	ASSERTX_RETFALSE( pRoomAnchor, "Expected room" );
	const VECTOR &vPosAnchor = UnitGetAimPoint( pAnchor );

	// setup search spec	
	NEAREST_NODE_SPEC tSpec;
	SETBIT(tSpec.dwFlags, NPN_SORT_OUTPUT);
	if( AppIsHellgate() )
	{
		SETBIT(tSpec.dwFlags, NPN_CHECK_LOS_AGAINST_OBJECTS);
		SETBIT(tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY);
		SETBIT(tSpec.dwFlags, NPN_IN_FRONT_ONLY);
		SETBIT(tSpec.dwFlags, NPN_CHECK_LOS);
	}	
	else
	{
		SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
	}
	tSpec.vFaceDirection = vDirection;
	tSpec.fMinDistance = AppIsHellgate() ? MIN_CREATE_PORTAL_DISTANCE : MIN_CREATE_PORTAL_DISTANCE_MYTHOS;
	tSpec.fMaxDistance = AppIsHellgate() ? MAX_CREATE_PORTAL_DISTANCE : MAX_CREATE_PORTAL_DISTANCE_MYTHOS;

	// check expanding circles around the anchor object
	FREE_PATH_NODE tFreePathNode;	
	int nRings = AppIsHellgate() ? PORTAL_CREATE_NUM_RINGS : PORTAL_CREATE_NUM_RINGS_MYTHOS;
	for (int i = 0; i < nRings; ++i)
	{
	
		// find node around where we want
		int nNumNodes = RoomGetNearestPathNodes(
			pGame, 
			pRoomAnchor,
			vPosAnchor,
			1, 
			&tFreePathNode, 
			&tSpec);

		// if we found a node
		if (nNumNodes == 1)
		{
		
			// get world location
			VECTOR vWorldPosition;
			RoomTransformFreePathNode( &tFreePathNode, &vWorldPosition );
			
			// validate that a the creator can really create a portal here
			if (sValidatePortalLocation( pCreator, &vWorldPosition, tFreePathNode.pRoom, dwWarpCreateFlags ) == TRUE)
			{
				*pvWorldPosition = vWorldPosition;
				*pRoom = tFreePathNode.pRoom;
				return TRUE;
			}
			
		}
	
		// didn't find a spot we could use, expand our search
		if( AppIsHellgate() )
		{
			tSpec.fMinDistance += PORTAL_CREATE_RING_INCREMENT;
			tSpec.fMaxDistance += PORTAL_CREATE_RING_INCREMENT;
		}
		else
		{
			tSpec.fMinDistance += PORTAL_CREATE_RING_INCREMENT_MYTHOS;
			tSpec.fMaxDistance += PORTAL_CREATE_RING_INCREMENT_MYTHOS;
		}
	
	}

	// unable to find a location
	return FALSE;		
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_WarpCreatePortalAround(
	UNIT *pAnchor,
	GLOBAL_INDEX eGIWarp,
	DWORD dwWarpCreateFlags,
	UNIT *pCreator,
	int nStatRecordPortalIDOnCreator /*= INVALID_LINK*/)
{
	ASSERTX_RETNULL( pAnchor, "Expected unit" );
	ASSERTX_RETNULL( eGIWarp != GI_INVALID, "Invalid global index" );
	GAME *pGame = UnitGetGame( pAnchor );
	ASSERTX_RETNULL( pGame, "Expected game" );

	// if we have a stat to record, we must have a unit to record the stat on too
	if (nStatRecordPortalIDOnCreator != INVALID_LINK)
	{
		ASSERTX_RETNULL( pCreator, "Expected unit" );
	}
		
	// where will we create the town portal
	VECTOR vWorldPosition;
	ROOM *pRoom = NULL;
	if (sFindPortalLocationAroundAnchor( pAnchor, pCreator, &vWorldPosition, &pRoom, dwWarpCreateFlags ) == FALSE)
	{
		return NULL;  // unable to find a suitable location for the portal
	}
	
	// face the portal toward the player
	VECTOR vPositionOwner = UnitGetPosition( pAnchor );
	VECTOR vFacing;  
	VectorSubtract( vFacing, vPositionOwner, vWorldPosition );
	VectorNormalize( vFacing );
	vFacing.fZ = 0.0f;  // face only in X/Y
	

	if( AppIsTugboat() )
	{
		LEVEL* pLevel = UnitGetLevel( pAnchor );
		VECTOR Normal( 0, 0, -1 );
		VECTOR Start = vWorldPosition;

		if( pLevel )
		{
			Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
			float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
			VECTOR vCollisionNormal;
			int Material;
			float fCollideDistance = LevelLineCollideLen( pGame, pLevel, Start, Normal, fDist, Material, &vCollisionNormal);
			if (fCollideDistance < fDist &&
				Material != PROP_MATERIAL &&
				Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
				Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
			{
				vWorldPosition.fZ = Start.fZ - fCollideDistance;
			}
		}

	}

	// create the portal
	OBJECT_SPEC tSpec;
	tSpec.nClass = GlobalIndexGet( eGIWarp );
	tSpec.pRoom = pRoom;
	tSpec.vPosition = vWorldPosition;
	tSpec.pvFaceDirection = &vFacing;
	tSpec.guidOwner = UnitIsPlayer( pCreator ) ? UnitGetGuid( pCreator ) : INVALID_GUID;
	UNIT *pPortal = s_ObjectSpawn( pGame, tSpec );
	
	// record the unitid of the portal on the player at the record stat (if present)
	if (nStatRecordPortalIDOnCreator != INVALID_LINK)
	{
		if (pPortal)
		{
			UNITID idPortal = UnitGetId( pPortal );
			UnitSetStat( pCreator, nStatRecordPortalIDOnCreator, idPortal );
		}
		else
		{
			UnitClearStat( pCreator, nStatRecordPortalIDOnCreator, 0 );
		}
	}
	
	return pPortal;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WARP_RESTRICTED_REASON WarpDestinationIsAvailable(
	UNIT *pOperator,
	UNIT *pWarp)
{		
	ASSERTX_RETVAL( pOperator, WRR_UNKNOWN, "Expected operator unit" );
	ASSERTX_RETVAL( pWarp, WRR_UNKNOWN, "Expected unit" );
	ASSERTX_RETVAL( ObjectIsWarp( pWarp ), WRR_UNKNOWN, "Expected warp" );
	
	LEVELID idLevelDest = WarpGetDestinationLevelID( pWarp );
	if (idLevelDest != INVALID_ID)
	{
		GAME *pGame= UnitGetGame( pWarp );
		LEVEL *pLevel = LevelGetByID( pGame, idLevelDest );
		if (pLevel)
		{
			return LevelIsAvailableToUnit( pOperator, pLevel );
		}
	}
	
	// no dest ID, that's OK I guess for now?
	return WRR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WarpSetBlockingReason( 
	UNIT *pWarp, 
	WARP_RESTRICTED_REASON eReason)
{
	ASSERTX_RETURN( pWarp, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pWarp, UNITTYPE_OBJECT ), "Expected object" );
	ASSERTX_RETURN( ObjectIsWarp( pWarp ), "Expected warp" );
	
	// save reason in stat
	UnitSetStat( pWarp, STATS_WARP_BLOCKING_REASON, eReason );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WARP_RESTRICTED_REASON c_WarpGetBlockingReason( 
	UNIT *pWarp)
{
	ASSERTX_RETVAL( pWarp, WRR_INVALID, "Expected unit" );
	ASSERTX_RETVAL( UnitIsA( pWarp, UNITTYPE_OBJECT ), WRR_INVALID, "Expected object" );
	ASSERTX_RETVAL( ObjectIsWarp( pWarp ), WRR_INVALID, "Expected warp" );
	
	// save reason in stat
	WARP_RESTRICTED_REASON eReason = (WARP_RESTRICTED_REASON)UnitGetStat( pWarp, STATS_WARP_BLOCKING_REASON );
	if (eReason > WRR_INVALID && eReason < WRR_NUM_REASONS)
	{
		return eReason;
	}
	
	return WRR_INVALID;
	
}

//----------------------------------------------------------------------------
struct WARP_RESTRICTED_LOOKUP
{
	WARP_RESTRICTED_REASON eReason;
	GLOBAL_STRING eGlobalString;
};
static const WARP_RESTRICTED_LOOKUP sgtWarpRestrictedLookup[] = 
{
	{ WRR_UNKNOWN,							GS_WARP_RESTRICTED_REASON_UNKNOWN },
	{ WRR_NO_WAYPOINT,						GS_WARP_RESTRICTED_REASON_NO_WAYPOINT },
	{ WRR_PROHIBITED_IN_PVP,				GS_WARP_RESTRICTED_REASON_PVP },
	{ WRR_TARGET_PLAYER_NOT_FOUND,			GS_WARP_RESTRICTED_REASON_PLAYER_NOT_FOUND },
	{ WRR_TARGET_PLAYER_NOT_ALLOWED,		GS_WARP_RESTRICTED_REASON_PLAYER_NOT_ALLOWED },
	{ WRR_LEVEL_NOT_AVAILABLE_TO_GAME,		GS_WARP_RESTRICTED_REASON_UNKNOWN },  // todo, make specific error message
	{ WRR_ACT_BETA_GRACE_PERIOD,			GS_WARP_RESTRICTED_REASON_UNKNOWN },  // todo, make specific error message
	{ WRR_ACT_SUBSCRIBER_ONLY,				GS_WARP_RESTRICTED_REASON_UNKNOWN },  // todo, make specific error message
	{ WRR_ACT_EXPERIENCE_TOO_LOW,			GS_WARP_RESTRICTED_REASON_UNKNOWN },  // todo, make specific error message
	{ WRR_ACT_TRIAL_ACCOUNT,				GS_TRIAL_PLAYERS_CANNOT_ACCESS },		
};
static const int sgnNumWarpRestrictedLookup = arrsize( sgtWarpRestrictedLookup );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WarpRestricted( 
	WARP_RESTRICTED_REASON eReason)
{
#if !ISVERSION(SERVER_VERSION)
	
	ASSERTX_RETURN( eReason != WRR_INVALID, "Invalid warp restricted reason" );
	ASSERTX_RETURN( eReason != WRR_OK, "OK warp restricted reason" );
	ASSERTX_RETURN( eReason < WRR_NUM_REASONS, "Out of range value for warp restricted reason" );
		
	// get string
	const WCHAR *puszTitle = GlobalStringGet( GS_WARP_RESTRICTED_TITLE );
	const WCHAR *puszReason = NULL;
	
	// get reason string
	GLOBAL_STRING eGlobalString = GS_WARP_RESTRICTED_REASON_UNKNOWN;
	for (int i = 0; i < sgnNumWarpRestrictedLookup; ++i)
	{
		const WARP_RESTRICTED_LOOKUP *pLookup = &sgtWarpRestrictedLookup[ i ];
		if (pLookup->eReason == eReason)
		{
			eGlobalString = pLookup->eGlobalString;
		}
	}
	puszReason = GlobalStringGet( eGlobalString );	
	ASSERTX_RETURN( puszReason, "Expected reason string" );
		
	// display generic error
	UIShowGenericDialog( puszTitle, puszReason );
		
#endif
	
}
