//----------------------------------------------------------------------------
// s_network.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "mailslot.h"
#include "clients.h"
#include "console.h"
#include "consolecmd.h"
#include "s_network.h"
#include "player.h"
#include "items.h"
#include "objects.h"
#include "monsters.h"
#include "inventory.h"
#include "c_message.h"
#include "s_message.h"
#include "skills.h"
#include "npc.h"
#include "movement.h"
#include "quests.h"
#include "unitmodes.h"
#include "unittag.h"
#include "performance.h"
#include "perfhier.h"
#include "config.h"
#include "gameconfig.h"
#include "c_network.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "c_appearance.h"
#include "filepaths.h"
#include "minitown.h"
#include "waypoint.h"
#include "warp.h"
#include "chat.h"
#include "customgame.h"
#include "s_townportal.h"
#include "s_trade.h"
#include "s_reward.h"
#include "netclient.h"
#include "weaponconfig.h"
#include "svrstd.h"
#include "GameServer.h"
#include "s_chatNetwork.h"
#include "s_quests.h"
#include "s_questgames.h"
#include "quest_tutorial.h"
#include "quest.h"
#include "pets.h"				//tugboat
#include "GameChatCommunication.h"
#include "s_partyCache.h"
#include "tasks.h"
#include "skills_shift.h"
#include "openlevel.h"
#include "states.h"
#include "s_store.h"
#include "gameunits.h"
#include "recipe.h"
#include "s_recipe.h"
#include "cube.h"
#include "rjdmovementtracker.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "level.h"
#include "s_QuestServer.h"
#include "namefilter.h"
#include "..\BotClient\BotCheat.h"
#include "ai_priv.h"
#include "version.h"
#include "stash.h"
#include "hotkeys.h"
#include "itemupgrade.h"
#include "s_itemupgrade.h"
#include "combat.h"
#include "LevelAreaLinker.h"
#include "Currency.h"
#include "dungeon.h"
#include "party.h"
#include "duel.h"
#include "player_move_msg.h"
#include "gamevariant.h"
#include "achievements.h"
#include "wordfilter.h"
#include "partymatchmaker.h"
#include "teams.h"
#include "ehm.h"
#include "gag.h"
#include "crafting_properties.h"
#include "s_crafting.h"
#include "CommonChatMessages.h"
#include "UserChatCommunication.h"
#include "s_playerEmail.h"
#include "s_globalGameEventNetwork.h"
#include "treasure.h"
#include "utilitygame.h"
#include "itemrestore.h"
#include "uix_hypertext.h"

#if ISVERSION(SERVER_VERSION)
#include "s_townInstanceList.h"
#include "s_battleNetwork.h"
#include "s_network.cpp.tmh"
#include "winperf.h"
#include "ServerSuite/BillingProxy/BillingMsgs.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "GameAuctionCommunication.h"
#include "gamevariant.h"
#include "Economy.h"
#include "s_auctionNetwork.h"
#include "utilitygame.h"
#else
#include "EmbeddedServerRunner.h"
#endif

#define PLAYER_SAVE_INTERVAL				(60 * GAME_TICKS_PER_SECOND)
//#define SERVER_PING_INTERVAL				(53 * GAME_TICKS_PER_SECOND) //defined in message_tracker.h
#define MISSED_PINGS_TO_DISCONNECT			10 //Note: a client would have to miss all 10, in a row.  Ping count resets upon one success.


#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void sCCmdAddToWeaponConfig(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data);

BOOL SrvNetworkRemove(
	NETCLIENTID64 idNetClient,
	void * data);


//----------------------------------------------------------------------------
// save the player every so often
//----------------------------------------------------------------------------
static BOOL SavePlayersOnTick(
	GAME * game,
	UNIT *,
	const EVENT_DATA &)
{
	// save all players
	for (GAMECLIENT *pClient = ClientGetFirstInGame( game ); 
		 pClient; 
		 pClient = ClientGetNextInGame( pClient ))	
	{
		if(!ClientValidateIdConsistency(game, ClientGetId(pClient)) )
		{//Race condition: Netclient disconnected, but didn't have a game yet so couldn't disconnect the gameclient
			ASSERT_MSG("Game client still around; netclient gone.");
#ifdef _DEBUG
			ClientValidateIdConsistency(game, ClientGetId(pClient)); //Just in case we need to step through their client mapping.
#endif
#if ISVERSION(SERVER_VERSION)
			GameServerContext * pServerContext = game->pServerContext;
			if(pServerContext) 
				PERF_ADD64(pServerContext->m_pPerfInstance,
				GameServer,GameServerIdValidateBootCount,1);			
#endif
			ClientRemoveFromGame(game, pClient, 1);
		}

		UNIT * player = ClientGetPlayerUnit( pClient );
		if (!player)
		{
			continue;
		}

#if ISVERSION(SERVER_VERSION)
		// Probably should just remove this altogether as it won't be hit -- BAN 9/24/2007
		if (s_IncrementalDBUnitsAreEnabled() == FALSE) {
			AppPlayerSave(player);
	    }
#else
		AppPlayerSave(player);
#endif
		
	}

	GameEventRegister(game, SavePlayersOnTick, NULL, NULL, NULL, PLAYER_SAVE_INTERVAL);
	return TRUE;
}


//----------------------------------------------------------------------------
// ping the player every so often
//----------------------------------------------------------------------------
static BOOL PingPlayersOnTick(
	GAME * game,
	UNIT *,
	const EVENT_DATA &)
{
	// ping all players
	for (GAMECLIENT *pClient = ClientGetFirstInGame( game ); 
		pClient; 
		pClient = ClientGetNextInGame( pClient ))	
	{
		if(pClient->m_nMissedPingCount < MISSED_PINGS_TO_DISCONNECT &&
			ClientGetNetId(pClient) != INVALID_NETCLIENTID64)
		{
			MSG_SCMD_PING msg;
			msg.timeOfSend = GetRealClockTime();
			msg.bIsReply = FALSE;
			s_SendMessage(game, ClientGetId(pClient), SCMD_PING, &msg);
			pClient->m_nMissedPingCount++;
		}
		else
		{
			TraceGameInfo("Removed gameclient %I64x with netclient id %I64x due to missed pings.\n", 
				ClientGetId(pClient), ClientGetNetId(pClient));
#if ISVERSION(SERVER_VERSION)
			GameServerContext * pServerContext = game->pServerContext;
			if(pServerContext) 
				PERF_ADD64(pServerContext->m_pPerfInstance,
				GameServer,GameServerPingBootCount,1);			
#endif
			ClientRemoveFromGame(game, pClient, 1);
		}
	}

	GameEventRegister(game, PingPlayersOnTick, NULL, NULL, NULL, SERVER_PING_INTERVAL);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID SrvNewGame(
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * game_setup,
	BOOL bReserve,
	int nDifficulty)
{
	GameServerContext * gameContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETINVALID(gameContext);
	ASSERT_RETINVALID(gameContext->m_GameList);
	GAME * game = GameListAddGame(gameContext->m_GameList, eGameSubType, game_setup, nDifficulty);
	ASSERT_RETINVALID(game);
	GAMEID idGame = GameGetId(game);

	if (bReserve)
	{
		game->m_nReserveCount = 1;		// all mini towns don't go away
	}

	game->eGameState = GAMESTATE_RUNNING;
	GameEventRegister(game, SavePlayersOnTick, NULL, NULL, NULL, PLAYER_SAVE_INTERVAL);
	GameEventRegister(game, PingPlayersOnTick, NULL, NULL, NULL, SERVER_PING_INTERVAL);

	if (AppGetType() == APP_TYPE_OPEN_SERVER)
	{
		AppSetSrvGameId(idGame);
	}
	
	GameListReleaseGameData(gameContext->m_GameList, game);
	MemorySetThreadPool(NULL);
	return idGame;
}

//----------------------------------------------------------------------------
// SERVER MESSAGES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// validate and copy name from message to buffer
//----------------------------------------------------------------------------
static BOOL GetPlayerNewName(
	WCHAR * wszDest,
	unsigned int nDestLen,
	const WCHAR * wszSource,
	unsigned int nSourceLen)
{
	ASSERT_RETFALSE(wszSource);

	if (nSourceLen == 0 || wszSource[0] == 0)
	{
		ASSERT_RETFALSE(AppGetType() == APP_TYPE_SINGLE_PLAYER);

		GLOBAL_DEFINITION * global = DefinitionGetGlobal();
		ASSERT_RETFALSE(global);

		const char * cur = global->szPlayerName;
		cur = PStrSkipWhitespace(cur, MAX_XML_STRING_LENGTH);
		PStrCvt(wszDest, cur, nDestLen);

		if (!wszDest[0])
		{
			PStrCopy(wszDest, DEFAULT_PLAYER_NAME, nDestLen);
		}
	}
	else
	{
		unsigned int ii;
		for (ii = 0; ii < nSourceLen; ii++)
		{
			if (wszSource[ii] == 0)
			{
				break;
			}
		}
		ASSERT_RETFALSE(ii < nSourceLen);
		PStrCopy(wszDest, wszSource, nDestLen);
	}
	if (PStrICmp(wszDest, AUCTION_OWNER_NAME) == 0) {
		return TRUE;
	}

	return ((AppGetType()!=APP_TYPE_CLOSED_SERVER)?TRUE:ValidateName(wszDest, nDestLen, NAME_TYPE_PLAYER, TRUE) );
}


//----------------------------------------------------------------------------
// inform client of error joining game
//----------------------------------------------------------------------------
void SendError(
	NETCLIENTID64 idNetClient,
	int nError)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pServerContext = (GameServerContext*)CurrentSvrGetContext();
	if(pServerContext) 
		PERF_ADD64(pServerContext->m_pPerfInstance,GameServer,GameServerSendErrorCount,1);
#endif

	if(idNetClient == INVALID_NETCLIENTID64) return;

	MSG_SCMD_ERROR msg;
	msg.bError = (BYTE)nError;

	TraceError("Client %llu had error %d joining game", idNetClient, nError);

	s_SendMessageNetClient(idNetClient, SCMD_ERROR, &msg);
	GameServerNetClientRemove( idNetClient );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SendError(
	APPCLIENTID idClient,
	int nError)
{
	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idClient);

	SendError(idNetClient, nError);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_NetworkJoinGameError(
	GAME * game,
	APPCLIENTID idAppClient,
	GAMECLIENT * client,
	UNIT * unit,
	int errorCode,
	const char * fmt,
	...)
{
	va_list args;
	va_start(args, fmt);

	char str[4096];
	PStrPrintf(str, arrsize(str), "[%s]  ", (client && client->m_szName[0]) ? client->m_szName : "???");

	char str2[4096];
	PStrVprintf(str2, arrsize(str2), fmt, args);

	PStrCat(str, str2, arrsize(str));

	s_PlayerJoinLog(game, unit, idAppClient, str);

	ASSERTX(0, str);

	SendError(idAppClient, errorCode);
	if (game && client)
	{
		ClientRemoveFromGameNow(game, client, FALSE);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
// Handle any login messages we need to make to other servers.
//----------------------------------------------------------------------------
static BOOL s_sNetLoginMember(
	UNIT * unit, 
	UNIQUE_ACCOUNT_ID idAccount)
{
	BOOL toRet = s_ChatNetLoginMember(unit, idAccount);
#if ISVERSION(SERVER_VERSION)
	//Send a message to the billing proxy to update our login.
	BILLING_REQUEST_GAME_LOGIN msg;
	msg.AccountId = idAccount;
	NETCLIENTID64 idNetClient = ClientGetNetId(UnitGetClient(unit));

	ASSERTX(toRet, "Chat net login failed");

	toRet = GameServerToBillingProxySendMessage(
		&msg, EBillingServerRequest_GameLogin, 
		NetClientGetBillingInstance(idNetClient)) && toRet;

	TraceInfo(TRACE_FLAG_GAME_NET,
		"Sending login message to billing proxy for account %I64u, billing instance %d with result %d",
		idAccount, NetClientGetBillingInstance(idNetClient), toRet);

	if (toRet) {
		//Also send a message to auction server
		GAME_AUCTION_REQUEST_LOGIN_MEMBER_MSG msgAuction;
		msgAuction.MemberGuid = UnitGetGuid(unit);
//		GameVariantInit(msgAuction.tGameVariant, unit);
		UnitGetName(unit, msgAuction.wszMemberName, MAX_CHARACTER_NAME, 0);
		toRet = GameServerToAuctionServerSendMessage(&msgAuction, GAME_AUCTION_REQUEST_LOGIN_MEMBER);

		TraceInfo(TRACE_FLAG_GAME_NET,
			"Sending login message to auction server for character unit %I64u with result %d",
			UnitGetGuid(unit), toRet);
	}
#endif
	return toRet;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GAMECLIENT * sClientAddToGame(
	APPCLIENTID idAppClient,
	GAME * game,
	WCHAR * wszPlayerName,
	DWORD dwSwitchInstanceFlags)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT * client = NULL;
	
	ASSERT_RETNULL(idAppClient != INVALID_CLIENTID);
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(wszPlayerName && wszPlayerName[0]);

	// when joining open level games, make certain that the game can really accept
	// a new player before adding them ... this is a sanity check to make sure we
	// didn't screw up the open level logic and let too many people into a game
	if (AppIsHellgate() && TESTBIT(dwSwitchInstanceFlags, SIF_JOIN_OPEN_LEVEL_GAME_BIT) && GameCanHaveOpenLevels(game))
	{
		if (OpenLevelGameCanAcceptNewPlayers(game) == FALSE)
		{
			s_NetworkJoinGameError(game, idAppClient, NULL, NULL, LOGINERROR_UNKNOWN, "sClientAddToGame() -- Open Level can't accept any more players.");
			return NULL;
		}
	}
		
	if (!ClientSystemSetClientGame(AppGetClientSystem(), idAppClient, GameGetId(game), wszPlayerName))
	{
		s_NetworkJoinGameError(game, idAppClient, NULL, NULL, LOGINERROR_UNKNOWN, "sClientAddToGame() -- Client System failed to set client game.");
		return NULL;
	}

	client = ClientAddToGame(game, idAppClient);
	if (!client)
	{
		s_NetworkJoinGameError(game, idAppClient, NULL, NULL, LOGINERROR_UNKNOWN, "sClientAddToGame() -- ClientAddToGame() failed.");
		return NULL;
	}

	// todo: after this point, need to remove client from the game if we get an error
	if (!ClientSystemSetGameClient(AppGetClientSystem(), idAppClient, ClientGetId(client)))
	{
		s_NetworkJoinGameError(game, idAppClient, client, NULL, LOGINERROR_ALREADYONLINE, "sClientAddToGame() -- Client already online.");
		return NULL;
	}

	GameSendToClient(game, client);
	
	return client;
#else
	REF(idAppClient);
	REF(game);
	REF(wszPlayerName);
	REF(dwSwitchInstanceFlags);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerAddToGame(
	GAME * game,
	UNIT * unit,
	WARP_SPEC &tWarpSpec,
	int nLevelDefLeaving,
	DWORD dwSwitchInstanceFlags,
	BOOL bFirstGameJoin,
	BOOL bLoadedFromDisk)
{
	BOOL bAdded = FALSE;
	
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(unit);
	
	GAMECLIENT * client = UnitGetClient(unit);
	ASSERT_RETFALSE(client);

	s_PlayerJoinLog(game, unit, client->m_idAppClient, "sPlayerAddToGame()");		
	
	// players should be not be on a level when they are joining the game
	ASSERTV( 
		UnitGetLevel( unit ) == NULL, 
		"Adding player '%s' to game that is already on a level '%s'",
		UnitGetDevName( unit ),
		LevelGetDevName( UnitGetLevel( unit ) ) );

	// setup the players quests on this server
	if(AppIsHellgate())
	{
	
		// restore state of quest system	
		s_QuestDoSetup( unit, bFirstGameJoin );
		
		// init quest games
		s_QuestGamesInit( unit );
	}
	
	// set the player unit for this client, it is important to do this before we do the
	// level warp which will start sending things to the client
	ClientSetPlayerUnit(game, client, unit);
	
	if (IS_SERVER(game))
	{
		ClientSystemSetGuid(AppGetClientSystem(), ClientGetAppId(client), UnitGetGuid(unit));
	}

	// revive player
	UnitEndMode(unit, MODE_DYING, 0, TRUE);
	UnitEndMode(unit, MODE_DEAD, 0, TRUE);
	s_UnitSetMode(unit, MODE_IDLE);

	s_PlayerJoinLog( game, unit, client->m_idAppClient, "level warping player into game" );

	// put player in level (will put them into limbo!!!!), has to occur before player add
	if (s_LevelWarpIntoGame( unit, tWarpSpec, nLevelDefLeaving ) ==	TRUE)
	{
		// add player
		s_PlayerJoinLog( game, unit, client->m_idAppClient, "Adding player to game" );
		if( s_PlayerAdd(game, client, unit ))
		{
		
			// now that the quests are initialized and the player is now successfully brought
			// entirely into the game, bring the player up to the current version
			if (bLoadedFromDisk == TRUE)
			{
				ASSERTX( bFirstGameJoin == TRUE, "Expected first game join" );
				ASSERTX( IS_SERVER( game ), "Expected server game" );
				s_UnitVersion( unit );		
			}
		
			s_StateSet(unit, unit, STATE_TEST_CENTER_SUBSCRIBER, 0);
			// restore vitals
			s_UnitRestoreVitals(unit, FALSE);

#if ISVERSION(SERVER_VERSION)
			// give the player his exp boost
			UnitSetStat(unit, STATS_EXP_BONUS,UnitGetBaseStat(unit, STATS_EXP_BONUS)+gGameSettings.m_nExpBonus);
#endif
			
			// sync important information with the chat server about this player
			UpdateChatServerPlayerInfo(unit);			
			SVR_VERSION_ONLY(s_ChatNetRetrieveGuildAssociation(unit));
			
			// player has now been added!  yay for players!
			s_PlayerJoinLog( game, unit, client->m_idAppClient, "player added" );
			bAdded = TRUE;
						
		}
		else
		{
			ASSERTV( "Failed to s_PlayerAdd() '%s'", UnitGetDevName( unit ) );
		}

		if(PlayerIsSubscriber(unit, TRUE))
		{ //Subscriber needs to be a state, so others can see it,
			//but can't save on unit...so let's set it here!
			s_StateSet( unit, unit, STATE_SUBSCRIBER, 0 );
		}

	}

	// if joining an open level game, we will auto party with the party in this game
	LEVEL_TYPE levelType;
	levelType.nLevelDef = tWarpSpec.nLevelDefDest;
	levelType.nLevelArea = tWarpSpec.nLevelAreaDest;
	levelType.nLevelDepth = tWarpSpec.nLevelDepthDest;
	levelType.bPVPWorld = tWarpSpec.nPVPWorld != 0;

	if( AppIsHellgate() )
	{
 		if (TESTBIT(dwSwitchInstanceFlags, SIF_JOIN_OPEN_LEVEL_GAME_BIT) && 
 			GameCanHaveOpenLevels(game) && 
 			LevelCanBeOpenLevel(levelType))
		{
			
			// find party to join or create new party
			PARTYID idParty = GameGetAutoPartyId(game);
			if (idParty == INVALID_ID)
			{
				PartyCacheCreateParty(GameGetId(game),0/*ullRequestCode*/);
			}
			else
			{
				OpenLevelAutoFormParty(game, idParty);
			}	
		}
	}
	if( AppIsTugboat() && bFirstGameJoin )
	{
		PlayerSetRespawnLocation( KRESPAWN_TYPE_PRIMARY, unit, NULL, UnitGetLevel( unit ) );
	}

	s_PlayerJoinLog( game, unit, client->m_idAppClient, "sPlayerAddToGame() complete with bAdded=%d", bAdded );		
	
	// if we couldn't add--critical error--we free the unit and stuff in the calling function.
	
#else
	REF( game );
	REF( unit );
	REF( tWarpSpec );
	REF( nLevelDefLeaving );
	REF( dwSwitchInstanceFlags );
	return FALSE;
#endif
	
	return bAdded;
}

//Error handling macro for join failures
#define JOIN_ERROR(x) {eLoginError = (x); goto Error;} 

//----------------------------------------------------------------------------
// attempt to join an existing game
//----------------------------------------------------------------------------
static BOOL sPlayerJoinGame(
	GAME * game,
	APPCLIENTID idAppClient,
	MSG_CCMD_PLAYERNEW * msg)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(msg);
	LOGINERROR eLoginError = LOGINERROR_UNKNOWN;

	UNIT * unit = NULL;
	GAMECLIENT * client = NULL;

	WCHAR wszPlayerName[MAX_CHARACTER_NAME];
	BOOL bSentFile = FALSE;
	CLIENT_SAVE_FILE tClientSaveFile;
	BOOL bFirstJoin = TRUE;
	unsigned int nClass = msg->bClass;
	WARP_SPEC tWarpSpec;
	GAMECLIENTID idClient = INVALID_ID;

	s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame()");
	
	// get difficulty of player (attempt to recover when no difficulty is set)
	int nDifficulty = msg->nDifficulty;
	ASSERT(DifficultyValidateValue(nDifficulty) );
	
	//game->nDifficulty = nDifficulty;
	//DungeonPickDRLGs(game);
	// read data from the message
	if (!GetPlayerNewName(wszPlayerName, MAX_CHARACTER_NAME, msg->szCharName, MAX_CHARACTER_NAME))
	{
		s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Invalid player name" );
		JOIN_ERROR(LOGINERROR_INVALIDNAME);
	}

	PStrCopy( tClientSaveFile.uszCharName, wszPlayerName, MAX_CHARACTER_NAME );

	if (nClass >= ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_PLAYERS))
	{
		if (nClass != (BYTE)-1)		// this indicates sent character for open games?
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Invalid class" );		
			JOIN_ERROR(LOGINERROR_UNKNOWN);
		}
		// if nClass == 255, check if we have save data
		if (!ClientSystemGetClientSaveFile(AppGetClientSystem(), idAppClient, UCT_CHARACTER, INVALID_ID, &tClientSaveFile))
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Unable to get client save file" );		
			JOIN_ERROR(LOGINERROR_UNKNOWN);
		}
		ASSERT(msg->bNewPlayer == FALSE);
		if (msg->bNewPlayer != FALSE)
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Expected existing player" );		
			JOIN_ERROR(LOGINERROR_UNKNOWN);
		}
		ASSERT(tClientSaveFile.pBuffer && tClientSaveFile.dwBufferSize);
		if (!tClientSaveFile.pBuffer)
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Invalid save buffer" );
			JOIN_ERROR(LOGINERROR_UNKNOWN);		
		}
		if (!tClientSaveFile.dwBufferSize)
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Invalid save buffer size" );
			JOIN_ERROR(LOGINERROR_UNKNOWN);		
		}		
		bSentFile = TRUE;
	}

	// take care of new players, dupicate names, chars already loaded, etc.
	if (msg->bNewPlayer)
	{
		if (PlayerNameExists(wszPlayerName))
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), New player name already exists Name:%S", wszPlayerName );		
			JOIN_ERROR(LOGINERROR_NAMEEXISTS);
		}
	}
	else
	{
		if (!bSentFile && !AppCommonGetDemoMode() && !PlayerNameExists(wszPlayerName))
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), File not sent and name doesn't exist" );		
			JOIN_ERROR(LOGINERROR_PLAYERNOTFOUND);
		}
	}

	client = sClientAddToGame(idAppClient, game, wszPlayerName, 0);
	if (!client)
	{
		s_NetworkJoinGameError(game, idAppClient, NULL, NULL, LOGINERROR_UNKNOWN, "sPlayerJoinGame() -- Failed to add client to game.");
		JOIN_ERROR(LOGINERROR_UNKNOWN);
	}
	idClient = ClientGetId( client );
	
	// add player
	
	BOOL bLoadedFromDisk = FALSE;
	if (!msg->bNewPlayer)
	{
		unit = PlayerLoad(game, &tClientSaveFile, 0, idClient);
		ClientSystemFreeCharacterSaveFile(AppGetClientSystem(), idAppClient, FALSE);
		if(unit)
		{
			//Validate that the player is NOT in any room or level yet.
			LEVEL * pPlayerLevel = UnitGetLevel(unit);
			ASSERTV( 
				pPlayerLevel == NULL,
				"sPlayerJoinGame() - Unit '%s' just loaded but is already on level '%s' in room '%s' with id %d",
				UnitGetDevName( unit ),
				LevelGetDevName( pPlayerLevel ),
				RoomGetDevName( UnitGetRoom( unit ) ),
				RoomGetId( UnitGetRoom (unit) ) );
			bLoadedFromDisk = TRUE;
		}
	}
	if (!unit)
	{
		if (bSentFile)
		{
			s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), No unit and client sent save file" );
			JOIN_ERROR(LOGINERROR_UNKNOWN);
		}
		WARDROBE_INIT tWardrobeInit;
		if (!WardrobeInitStructRead(tWardrobeInit, msg->bufWardrobeInit, MAX_WARDROBE_INIT_BUFFER, nClass) )
		{
			const UNIT_DATA * pUnitData = PlayerGetData( game, nClass );
			if (!pUnitData)
			{
				s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), No unit data for player class %d", nClass );			
				JOIN_ERROR(LOGINERROR_UNKNOWN);
			}
			int nAppearanceGroup = pUnitData->pnWardrobeAppearanceGroup[UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON];

			tWardrobeInit.nAppearanceGroup = nAppearanceGroup;
			WardrobeRandomizeInitStruct( game, tWardrobeInit, nAppearanceGroup, TRUE );
		}

		DWORD dwNewPlayerFlags= 0;
		if ( msg->dwNewPlayerFlags & NPF_HARDCORE )
		{
#if ISVERSION( SERVER_VERSION ) || ISVERSION( DEVELOPMENT )
			BADGE_COLLECTION badges;
			ClientGetBadges(client, badges);
			if ( ! ClientHasBadge( client, ACCT_ACCOMPLISHMENT_CAN_PLAY_HARDCORE_MODE ) 
                 || !BadgeCollectionHasSubscriberBadge(badges)
                 || ClientHasBadge(client, ACCT_TITLE_TRIAL_USER))
			{
				s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Client can't use hardcore character" );
				JOIN_ERROR(LOGINERROR_UNKNOWN);
			}
			dwNewPlayerFlags |= NPF_HARDCORE;
#endif
		}

		if ( msg->dwNewPlayerFlags & NPF_PVPONLY )
		{
#if ISVERSION( SERVER_VERSION ) || ISVERSION( DEVELOPMENT )
			dwNewPlayerFlags |= NPF_PVPONLY;
#endif
		}
		
		if ( msg->dwNewPlayerFlags & NPF_ELITE)
		{
#if ISVERSION( SERVER_VERSION ) //|| ISVERSION( DEVELOPMENT )
			if ( ! ClientHasBadge( client, ACCT_ACCOMPLISHMENT_CAN_PLAY_ELITE_MODE)
				 || ClientHasBadge(client, ACCT_TITLE_TRIAL_USER))
			{
				s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), Client can't use elite character" );
				JOIN_ERROR(LOGINERROR_UNKNOWN);
			}
#endif
			dwNewPlayerFlags |= NPF_ELITE;
		}

		APPEARANCE_SHAPE tAppearanceShape;
		tAppearanceShape.bHeight = msg->bHeight;
		tAppearanceShape.bWeight = msg->bWeight;
		if(msg->dwNewPlayerFlags & NPF_DISABLE_TUTORIAL)
		{
			dwNewPlayerFlags |= NPF_DISABLE_TUTORIAL;
		}
		unit = s_PlayerNew(
			game, 
			idClient, 
			wszPlayerName, 
			nClass, 
			&tWardrobeInit, 
			&tAppearanceShape, 
			dwNewPlayerFlags);	
			
	}
	if (!unit)
	{
		ClientSetPlayerUnit(game, client, NULL);
		s_PlayerJoinLog( game, NULL, idAppClient, "sPlayerJoinGame(), No unit, clearing player unit for client" );		
		ASSERT_DO(unit != NULL)
		{
			JOIN_ERROR(LOGINERROR_UNKNOWN);
		}
	}

	// Make sure they haven't cheated their way to a higher difficulty
	// Now that we have a unit, we can validate it for the unit's max.
	UNITLOG_ASSERT(DifficultyValidateValue(nDifficulty, unit, client), unit); 

	// set the difficulty this player wants to use
	UnitSetStat(unit, STATS_DIFFICULTY_CURRENT, nDifficulty);
	
	// fill out warp spec for where they will start, we will either generate this from data
	// saved on the player, or use the warp spec that's being passed along as players switch instances

	{// see if player is coming in from another game (switching game instances and possibly servers)
		GAMEID idStoredGame = ClientSystemGetServerWarpGame( AppGetClientSystem(), idAppClient, tWarpSpec);
		if (idStoredGame != INVALID_ID)
		{
			ASSERTX( idStoredGame == GameGetId( game ), "Player is switching servers and joining the wrong game" );
			
			// clear out the game we were keeping for purposes of switching servers
			ClientSystemSetServerWarpGame( AppGetClientSystem(), idAppClient, INVALID_ID, tWarpSpec);
			
			// this player is coming from another game, this is not their first join
			bFirstJoin = FALSE;
			
		}
		else
		{	
			// nope, first time in any game, this is a brand new player
			tWarpSpec.nLevelDefDest = UnitGetStartLevelDefinition( unit );
			SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );			
			if (AppIsTugboat())
			{
				VECTOR vPosition;
				PlayerGetRespawnLocation( unit, KRESPAWN_TYPE_PRIMARY, tWarpSpec.nLevelAreaDest, tWarpSpec.nLevelDepthDest, vPosition );
				//UnitGetStartLevelAreaDefinition( unit, tWarpSpec.nLevelAreaDest, tWarpSpec.nLevelDepthDest );
				tWarpSpec.nPVPWorld = PlayerIsInPVPWorld( unit );
				
			}
		
		}
	}

	if( tWarpSpec.nLevelAreaDest == INVALID_ID &&
		AppIsTugboat() )
	{
		//must be a new character or something broke!
		tWarpSpec.nLevelAreaDest = unit->pUnitData->nStartingLevelArea;
		tWarpSpec.nLevelDepthDest = 0;
	}
	
	// always restore vitals when joining a new game
	s_UnitRestoreVitals( unit, FALSE );

	// add them to the game			
	BOOL bAdded = sPlayerAddToGame( game, unit, tWarpSpec, INVALID_LINK, 0, bFirstJoin, bLoadedFromDisk );

	//Clear out states that give a player an advantage in pvp...
	if(PlayerPvPIsEnabled(unit))
		StateClearAllOfType(unit, STATE_REMOVE_ON_PVP);

	//	log them into the chat server
	s_sNetLoginMember(unit, ClientGetUniqueAccountId(AppGetClientSystem(), idAppClient));
	
	s_PlayerJoinLog( game, unit, idAppClient, "sPlayerJoinGame() completed for with return code '%d'", bAdded);
	if(!bAdded) JOIN_ERROR(LOGINERROR_UNKNOWN);

	return TRUE;
Error:
	SendError(idAppClient, eLoginError);
	s_PlayerJoinLog( game, unit, idAppClient,
		"sPlayerJoinGame() failed with error %d",
		eLoginError);		

	if(client) GameRemoveClient(game, client);
	if(unit) UnitFree(unit);


	return FALSE;		
}


//----------------------------------------------------------------------------
// process APPCMD_SWITCHINSTANCE at GAME level
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sPlayerSwitchInstance(
	GAME * game,
	NETCLIENTID64 idNetClient,
	MSG_APPCMD_SWITCHINSTANCE * msg)
{
	ASSERT_RETFALSE(game);
	LOGINERROR eLoginError = LOGINERROR_UNKNOWN;

	GAMECLIENT * client = NULL;
	UNIT * unit = NULL;

	APPCLIENTID idAppClient = NetClientGetAppClientId(idNetClient);
	if (idAppClient == INVALID_CLIENTID)
	{
		// client is now gone, forget it
		return TRUE;
	}

	CLIENT_SAVE_FILE tClientSaveFile;
	DWORD key = 0;
	if (!ClientSystemGetClientInstanceSaveBuffer(AppGetClientSystem(), idAppClient, &tClientSaveFile, &key))
	{
		ASSERTX_DO(0, "Failed to get save buffer")
		{
			JOIN_ERROR(LOGINERROR_UNKNOWN);
		}
	}
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(key == msg->dwKey);	
#endif// !ISVERSION(SERVER_VERSION)

	client = sClientAddToGame(idAppClient, game, tClientSaveFile.uszCharName, msg->dwSwitchInstanceFlags);
	
	// if we couldn't join, if we were trying to join an open level game that we found, forget
	// it and just get a new game
	if (client == NULL)
	{
		if (TESTBIT(msg->dwSwitchInstanceFlags, SIF_JOIN_OPEN_LEVEL_GAME_BIT) && GameCanHaveOpenLevels(game))
		{
			// remove the request to join an open level game, we could change this to try 
			// again up to a max if we wanted one day
			CLEARBIT(msg->dwSwitchInstanceFlags, SIF_JOIN_OPEN_LEVEL_GAME_BIT);

			// tell app to keep on switching			
			return AppPostMessageToMailbox(idNetClient, APPCMD_SWITCHINSTANCE, msg);
		}
	}
	
	// must have client here	
	ASSERTX_DO(client != NULL, "Cannot get client for switchinstance")
	{
		JOIN_ERROR(LOGINERROR_UNKNOWN);
	}

	unit = PlayerLoad(game, &tClientSaveFile, 0, ClientGetId(client));
	ASSERTX_DO(unit != NULL, "Player failed to load" )
	{
		JOIN_ERROR(LOGINERROR_UNKNOWN);
	}

	// save our town portal from the game we came from (if any)
	const TOWN_PORTAL_SPEC *pTownPortal = &msg->tTownPortal;
	if ( ( AppIsHellgate() && pTownPortal->nLevelDefDest != INVALID_LINK ) || 
		 ( AppIsTugboat() && pTownPortal->nLevelDepthDest != INVALID_LINK ) )
	{
		ASSERTX( ( ( AppIsHellgate() && pTownPortal->nLevelDefPortal != INVALID_LINK ) ||
				 ( AppIsTugboat() && pTownPortal->nLevelDepthPortal != INVALID_LINK ) ),				 
				 "Don't know where portal in reserve game is" );
		if( AppIsHellgate() )
		{
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION, pTownPortal->nLevelDefDest );
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION, pTownPortal->nLevelDefPortal );
		}
		else
		{
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION, pTownPortal->nLevelDepthDest );
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION, pTownPortal->nLevelDepthPortal );
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_DESTINATION_AREA, pTownPortal->nLevelAreaDest );
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_LOCATION_AREA, pTownPortal->nLevelAreaPortal );
			UnitSetStat( unit, STATS_RESERVE_GAME_TOWN_PORTAL_PVPWORLD, pTownPortal->nPVPWorld );
		}		
	}

	// save where our headstone is (if anywhere)
	if( AppIsHellgate() )
	{
		PlayerSetHeadstoneLevelDef( unit, msg->nLevelDefLastHeadstone );
	}
	
#if !ISVERSION(SERVER_VERSION)
	ClientSystemFreeCharacterSaveFile(AppGetClientSystem(), idAppClient, FALSE);
#endif

	BOOL success = sPlayerAddToGame( 
		game, unit, 
		msg->tWarpSpec, 
		msg->nLevelDefLeaving, 
		msg->dwSwitchInstanceFlags,
		FALSE,
		FALSE);

	if(!success) JOIN_ERROR(LOGINERROR_UNKNOWN);
		
	return success;

Error:
	SendError(idAppClient, eLoginError);
	s_PlayerJoinLog( game, unit, idAppClient,
		"sPlayerSwitchInstance() failed with error %d",
		eLoginError);		

	if(client) GameRemoveClient(game, client);
	if(unit) UnitFree(unit);

	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPlayerFileInitSend(
	APPCLIENTID idAppClient,
	PGUID guidDestPlayer,
	PLAYER_SAVE_FORMAT eFormat,
	PGUID guidUnitTreeRoot,
	const WCHAR *puszCharacterName,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection,
	PLAYER_LOAD_TYPE eLoadType,
	int nDifficultySelected,
	DWORD dwCRC,
	DWORD dwSize)
{
	ASSERT_RETURN(idAppClient != INVALID_CLIENTID);
	
	// read data from the message
	WCHAR wszPlayerName[MAX_CHARACTER_NAME];
	if (!GetPlayerNewName(wszPlayerName, MAX_CHARACTER_NAME, puszCharacterName, MAX_CHARACTER_NAME))
	{
		if (eCollectionType == UCT_CHARACTER)
		{
			// send login error if loading all units (entering the game)
			SendError(idAppClient, LOGINERROR_INVALIDNAME);
		}
		return;
	}

	if (dwSize == 0)
	{
		if (eCollectionType == UCT_CHARACTER)
		{
			// send login error if loading all units (entering the game)
			SendError(idAppClient, LOGINERROR_UNKNOWN);
		}
		ASSERT_RETURN(0);
		return;	
	}

	if (!ClientSystemSetClientSaveFile(
			AppGetClientSystem(), 
			idAppClient, 
			guidDestPlayer,
			eFormat, 
			guidUnitTreeRoot,
			wszPlayerName, 
			eCollectionType,
			idCollection,
			eLoadType,
			nDifficultySelected,
			dwCRC, 
			dwSize))
	{
		return;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPlayerSaveFileChunk(
	APPCLIENTID idAppClient,
	const BYTE *pBuffer,
	unsigned int nBufferSize,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection)
{
	ASSERT_RETURN(idAppClient != INVALID_CLIENTID);
	ASSERT_RETURN(eCollectionType > UCT_INVALID && eCollectionType < UCT_NUM_TYPES);
	CLIENTSYSTEM *pClientSystem = AppGetClientSystem();

	BOOL bDone = FALSE;
	PLAYER_SAVE_FORMAT eFormat = PSF_INVALID;
	if (!ClientSystemAddClientSaveFileChunk(
				pClientSystem, 
				idAppClient, 
				eCollectionType,
				idCollection,
				pBuffer, 
				nBufferSize, 
				bDone, 
				eFormat))
	{
		goto _ERROR;
	}
	if (!bDone)
	{
		return;
	}

	// do post process on this file
	if (ClientSaveFilePostProcess( pClientSystem, idAppClient, eCollectionType, idCollection ) == FALSE)
	{
		goto _ERROR;	
	}
	
	{
		CLIENT_SAVE_FILE tClientSaveFile;
		ClientSystemGetClientSaveFile( pClientSystem, idAppClient, eCollectionType, idCollection, &tClientSaveFile );

		if (eCollectionType == UCT_CHARACTER)
		{
			NETCLIENTID64 idNetClient = ClientGetNetId(pClientSystem, idAppClient);
			if (idNetClient == INVALID_CLIENTID)
			{
				goto _ERROR;
			}

			WARP_SPEC tWarpSpec;
			GAMEID idGame = ClientSystemGetServerWarpGame(pClientSystem, idAppClient, tWarpSpec);
			//TODO: verify this GAMEID is STILL valid.  If not, set it back to INVALID_ID
			if(idGame != INVALID_ID)
			{
				GameServerContext * context = (GameServerContext*)CurrentSvrGetContext();

				if(!context || GameListQueryGameIsRunning(context->m_GameList, idGame) == FALSE)
				{
					idGame = INVALID_ID;
					ClientSystemSetServerWarpGame(pClientSystem, idAppClient, idGame, tWarpSpec);
				}
			}

			// get character info from client
			CHARACTER_INFO tCharacterInfo;
			ClientSystemGetCharacterInfo(pClientSystem, idAppClient, tCharacterInfo);

			// if not game to join, get a game ID to join based on our start level in the character save data
			if(idGame == INVALID_ID)
			{
				idGame = WarpGetGameIdToJoinFromSaveData(
					idAppClient, 
					TRUE, 
					&tClientSaveFile,
					tCharacterInfo.nDifficulty);
			}
			if (idGame == INVALID_ID)
			{
				goto _ERROR;		
			}

			MSG_CCMD_PLAYERNEW msg;
			PStrCopy( msg.szCharName, tClientSaveFile.uszCharName, MAX_CHARACTER_NAME );
			msg.nDifficulty = tCharacterInfo.nDifficulty;
			ASSERT(PStrCmp(tClientSaveFile.uszCharName, tCharacterInfo.szCharName, MAX_CHARACTER_NAME) == 0);

			if (!SrvPostMessageToMailbox(idGame, idNetClient, CCMD_PLAYERNEW, &msg))
			{
				goto _ERROR;		
			}

		}
		else if (eCollectionType == UCT_ITEM_COLLECTION)
		{
			CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
			
			// get the game this player is in
			GAMEID idGame = INVALID_ID;
			
			if (idAppClient == APPCLIENTID_AUCTION_SYSTEM ||
				idAppClient == APPCLIENTID_ITEM_RESTORE_SYSTEM)  
			{
				idGame = UtilityGameGetGameId((GameServerContext*)CurrentSvrGetContext());
			} 
			else 
			{
				idGame = ClientSystemGetClientGame( pClientSystem, idAppClient );
			}

			if (idGame == INVALID_ID)
			{
				// no longer in a game, recycle all of this data back to the empty client
				ClientSystemRecycleClientSaveFile( pClientSystem, idAppClient, eCollectionType, idCollection );
			}
			else
			{
				// send this item tree to the game					
				MSG_APPCMD_NEW_ITEM_COLLECTION tMessage;
				tMessage.dwAppClientID = (DWORD)idAppClient;
				tMessage.dwUntiCollectionID = (DWORD)idCollection;
				tMessage.guidDestPlayer = tClientSaveFile.guidDestPlayer;

				if (!SrvPostMessageToMailbox(
						idGame, 
						ServiceUserId(USER_SERVER, 0, 0),
						APPCMD_NEW_ITEM_COLLECTION, 
						&tMessage))
				{
					goto _ERROR;		
				}
				
			}
			
		}
		
	}
	
	return;
	
_ERROR:

	if (eCollectionType == UCT_CHARACTER)
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);	
	}
	ASSERT_RETURN(0);
}
	
	
//----------------------------------------------------------------------------
// process APPCMD_SWITCHINSTANCE at APPLICATION level
//----------------------------------------------------------------------------
static void sCCmdSwitchInstance(
	APPCLIENTID idAppClient,
	MSG_STRUCT * data)
{
	ASSERT_RETURN(AppIsTugboat() || AppTestDebugFlag(ADF_SP_USE_INSTANCE) || AppGetType() != APP_TYPE_SINGLE_PLAYER);
	
	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
	if (idNetClient == INVALID_NETCLIENTID64)
	{
		ASSERT_RETURN(0);
	}

	MSG_APPCMD_SWITCHINSTANCE * msg = (MSG_APPCMD_SWITCHINSTANCE *)data;

	//Validate difficulty setting.
	ASSERT(DifficultyValidateValue(msg->nDifficulty));
	
	CLIENT_SAVE_FILE tClientSaveFile;
	DWORD key = 0;
	if (!ClientSystemGetClientInstanceSaveBuffer(AppGetClientSystem(), idAppClient, &tClientSaveFile, &key))
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);
		ASSERT_RETURN(0);
	}
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(key == msg->dwKey);
#endif// !ISVERSION(SERVER_VERSION)

	GAMEID idGame = msg->tWarpSpec.idGameOverride;
	if(idGame != INVALID_ID)
	{
		GameServerContext * context = (GameServerContext*)CurrentSvrGetContext();

		if(!context || GameListQueryGameIsRunning(context->m_GameList, idGame) == FALSE)
			idGame = INVALID_ID;
	}
	if (idGame == INVALID_ID) 
	{
		idGame = WarpGetGameIdToJoin( 
			idAppClient, 
			FALSE, 
			AppIsHellgate() ? msg->tWarpSpec.nLevelDefDest : msg->tWarpSpec.nLevelDepthDest, 
			msg->tWarpSpec.nLevelAreaDest, 
			msg->dwSeed,
			&msg->dwSwitchInstanceFlags,
			msg->nDifficulty,
			msg->tGameVariant);
	}
#if ISVERSION(SERVER_VERSION)
	if(idGame != INVALID_ID && WarpGameIdRequiresServerSwitch(idGame))
	{
		WCHAR szPlayerName[MAX_CHARACTER_NAME];
		ONCE
		{
			ASSERT_BREAK(NetClientGetName(idNetClient, szPlayerName, MAX_CHARACTER_NAME) );
			ASSERT_BREAK(ClientWarpPostServerSwitch(idNetClient, &msg->tWarpSpec, idGame, msg->guidPlayer, szPlayerName) );
			
			TraceGameInfo(
				"Resolved server switch for appclient %x to game ID %I64x in sCCmdSwitchInstance()",
				idAppClient, idGame);
			return;
		}
		//else
		{
			ASSERT_MSG("Could not post server switch!");
			//Set the flag to get a local game and get it, so he's not stuck in limbo.
			SETBIT(msg->dwSwitchInstanceFlags, SIF_DISALLOW_DIFFERENT_GAMESERVER);
			idGame = WarpGetGameIdToJoin( 
				idAppClient, 
				FALSE, 
				AppIsHellgate() ? msg->tWarpSpec.nLevelDefDest : msg->tWarpSpec.nLevelDepthDest, 
				msg->tWarpSpec.nLevelAreaDest, 
				msg->dwSeed,
				&msg->dwSwitchInstanceFlags,
				msg->nDifficulty,
				msg->tGameVariant);
		}
	}
#endif

	if(idGame != INVALID_ID)
	{
		GameServerContext * context = (GameServerContext*)CurrentSvrGetContext();

		if(!context || GameListQueryGameIsRunning(context->m_GameList, idGame) == FALSE)
		{
			TraceError("Got non-running game %I64x in sCCmdSwitchInstance", idGame);
			ASSERT(FALSE);
			idGame = INVALID_ID;
		}
	}
		
	if (idGame == INVALID_ID)
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);
		ASSERT_RETURN(0);
	}

	if (!SrvPostMessageToMailbox(idGame, idNetClient, APPCMD_SWITCHINSTANCE, msg))
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);
		ASSERT_RETURN(0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdUnreserveGame(
	GAME * game,
	MSG_CCMD_UNRESERVEGAME * msg)
{
	ASSERT_RETURN(game->m_dwReserveKey == msg->dwReserveKey);
	PGUID guidReservedFor = msg->guid;

	// remove any town portals from the reserver
	DWORD dwTownPortalCloseFlags = 0;
	SETBIT( dwTownPortalCloseFlags, TPCF_ONLY_WHEN_OWNER_NOT_CONNECTED_BIT );
	s_TownPortalsCloseByGUID( game, guidReservedFor, dwTownPortalCloseFlags );

	ASSERT_RETURN(game->m_nReserveCount > 0);
	--game->m_nReserveCount;
	if (game->m_nReserveCount > 0)
	{
		return;
	}
	if (GameGetClientCount(game) == 0)
	{
		GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);
	}
}

//----------------------------------------------------------------------------
// process APPCMD_SWITCHSERVER at APPLICATION level only
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
static void sCCmdSwitchServer(
	NETCLIENTID64 idNetClient,
	MSG_STRUCT * data)
{
	ASSERT_RETURN(AppGetType() == APP_TYPE_CLOSED_SERVER);
	ASSERT_RETURN(data);

	MSG_APPCMD_SWITCHSERVER * msg = (MSG_APPCMD_SWITCHSERVER *)data;

	GameServerSwitchInstance(
		idNetClient, 
		msg->nGameServer,
		msg->szCharName,
		&msg->tWarpSpec,
		msg->idGameToJoin);
}
#endif

//----------------------------------------------------------------------------
// NOTE: This assumes players are not hacking... adding 2 players from same 
// client, things like that are all not checked yet
//----------------------------------------------------------------------------
static void sCCmdPlayerNew(
	APPCLIENTID idAppClient,
	MSG_STRUCT * data)
{
	TIMER_START("sCCmdPlayerNew()");

	MSG_CCMD_PLAYERNEW * msg = (MSG_CCMD_PLAYERNEW *)data;

	if(msg->qwBuildVersion != VERSION_AS_ULONGLONG)
	{
		MSG_SCMD_SYS_TEXT msgBuild;
		PStrPrintf(msgBuild.str, MAX_CHEAT_LEN,
			L"Different build version than server!\nServer Build Version: %I64x\nClient Build Version: %I64x\n",
			VERSION_AS_ULONGLONG, msg->qwBuildVersion);
		s_SendMessageAppClient(idAppClient, SCMD_SYS_TEXT, &msgBuild);
	}

	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
	if (idNetClient == INVALID_NETCLIENTID64)
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);
		ASSERT_RETURN(0);
	}
	//Validate that we have the same name as stated to the login

	WCHAR wszPlayerName[MAX_CHARACTER_NAME];
	NetClientGetName(idNetClient, wszPlayerName, MAX_CHARACTER_NAME);

#if ISVERSION(SERVER_VERSION)
	//Only in server version: we know their player name from login data from loadbalancer.
	ASSERT_DO(PStrCmp(msg->szCharName, wszPlayerName, MAX_CHARACTER_NAME) == 0)
	{
		SendError(idAppClient, LOGINERROR_INVALIDNAME);
		return;
	}
#endif

	// read data from the message
	
	if (!GetPlayerNewName(wszPlayerName, MAX_CHARACTER_NAME, msg->szCharName, MAX_CHARACTER_NAME))
	{
		SendError(idAppClient, LOGINERROR_INVALIDNAME);
		return;
	}

	if (msg->bNewPlayer)
	{
		if (PlayerNameExists(wszPlayerName))
		{
			SendError(idAppClient, LOGINERROR_NAMEEXISTS);
			return;
		}
	}
	else if( AppGetType() == APP_TYPE_CLOSED_SERVER )
	{
		PlayerLoadFromDatabase(idAppClient,msg->szCharName);
		return;
	}
		
	// setup difficulty this player wants
	//Validate message difficulty setting
	ASSERT(DifficultyValidateValue(msg->nDifficulty));
	int nDifficulty = msg->nDifficulty;
#if ISVERSION(DEVELOPMENT)
	if(AppTestDebugFlag(ADF_DIFFICULTY_OVERRIDE))
	{
		nDifficulty = msg->nDifficulty;
	}
#endif

	// get game ID to join
	GAMEID idGame = INVALID_ID;	
	BOOL bInitialJoin = TRUE;  // this is an initial join
	if (msg->bNewPlayer)
	{
	
		// setup game variant from message information
		GAME_VARIANT tGameVariant;
		tGameVariant.nDifficultyCurrent = nDifficulty;
		tGameVariant.dwGameVariantFlags = s_GameVariantFlagsGetFromNewPlayerFlags( idAppClient, msg->dwNewPlayerFlags );
			
		idGame = WarpGetGameIdToJoin(
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
		
		idGame = WarpGetGameIdToJoinFromSinglePlayerSaveFile(
			idAppClient, 
			bInitialJoin, 
			wszPlayerName, 
			nDifficulty,
			msg->dwNewPlayerFlags);
			
	}
	if (idGame == INVALID_ID)
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);
		ASSERT_RETURN(0);
	}

	if (!SrvPostMessageToMailbox(idGame, idNetClient, CCMD_PLAYERNEW, msg))
	{
		SendError(idAppClient, LOGINERROR_UNKNOWN);
		ASSERT_RETURN(0);
	}

	return;
}

//----------------------------------------------------------------------------
// GAME MESSAGES
//----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4100)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAssert(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPlayerJoin(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && client && data);
	ASSERT_RETURN(unit == NULL);

	MSG_CCMD_PLAYERNEW * msg = (MSG_CCMD_PLAYERNEW *)data;

	APPCLIENTID idAppClient = ClientGetAppId(client);
	if (idAppClient != INVALID_CLIENTID)
	{
		ASSERT_RETURN(sPlayerJoinGame(game, idAppClient, msg));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdPlayerInChatServer(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
	ASSERT_RETURN(game && client && unit);

	//	set their data on the chat server
	UpdateChatServerPlayerInfo(unit);

	//	add them to their level's chat channel
	LEVEL * pLevel = UnitGetLevel(unit);
	if( pLevel && pLevel->m_idChatChannel != INVALID_CHANNELID )
	{
		MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL setChannelMessage;
		setChannelMessage.idGame = GameGetId(game);
		setChannelMessage.idLevel = LevelGetID(pLevel);
		setChannelMessage.idChannel = pLevel->m_idChatChannel;

		s_SendMessage(
			game,
			ClientGetId(client),
			SCMD_SET_LEVEL_COMMUNICATION_CHANNEL,
			&setChannelMessage );

		s_ChatNetAddPlayerToChannel(
			UnitGetGuid(unit),
			pLevel->m_idChatChannel);
	}

#if ISVERSION(SERVER_VERSION)
	//	send a request for any guild invites they may have received while logged of
	s_ChatNetRetrieveGuildInvitesSentOffline(unit);

	//	get all emails
	s_PlayerEmailRetrieveMessages(game, client, unit);
#endif

	//	add them to instancing channels
	s_ChatNetAddPlayerToDefaultInstancingChannels(UnitGetGuid(unit), game, unit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdChatChannelCreated(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	if( !game )
		return;

	MSG_APPCMD_CHAT_CHANNEL_CREATED * channelMsg = NULL;
	channelMsg = (MSG_APPCMD_CHAT_CHANNEL_CREATED*)data;

	BOOL wasSet = FALSE;
	LEVEL * pLevel = LevelGetByID(game, (LEVELID)channelMsg->idLevel);
	if( pLevel )
	{
		wasSet = s_LevelSetCommunicationChannel(
					game,
					pLevel,
					channelMsg->idChatChannel );
	}
	if( !wasSet )
	{
		s_ChatNetReleaseChatChannel( channelMsg->idChatChannel );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAutoPartyCreated(
	GAME *pGame,
	GAMECLIENT * /*pClient*/,
	UNIT * /*pUnit*/,
	BYTE *pData)
{
	const MSG_APPCMD_AUTO_PARTY_CREATED *pMessage = (const MSG_APPCMD_AUTO_PARTY_CREATED *)pData;
	PARTYID idParty = (PARTYID)pMessage->idParty;
	
	// form open level auto parties
	OpenLevelAutoFormParty( pGame, idParty );

	// form matchmaker auto parties on the server
	s_PartyMatchmakerFormAutoParty( pGame, idParty, pMessage->ullRequestCode );
	
}

//----------------------------------------------------------------------------
// Switch over the gameclient and appclient to the new netclient.  
//----------------------------------------------------------------------------
static void sAppCmdTakeOverClient(
	GAME *pGame,
	GAMECLIENT * pClient,
	UNIT * pUnit,
	BYTE *pData)
{
	const MSG_APPCMD_TAKE_OVER_CLIENT *pMessage = (const MSG_APPCMD_TAKE_OVER_CLIENT *)pData;
	//Validity/obselescence checks.
	if(pClient == NULL || pUnit == NULL || UnitGetLevel(pUnit) == NULL)
	{	//We're in some screwy state we don't want to even try replacing.
		SendError(pMessage->idNetClientNew, LOGINERROR_ALREADYONLINE);
		SendError(pMessage->idNetClientOld, LOGINERROR_UNKNOWN);
	}
	//Do the account IDs match the message?
	if(NetClientGetUniqueAccountId(pMessage->idNetClientNew) != pMessage->idAccount)
	{
		SendError(pMessage->idNetClientNew, LOGINERROR_UNKNOWN);
		return;
	}
	if(NetClientGetUniqueAccountId(pMessage->idNetClientOld) != pMessage->idAccount)
	{
		//Did netclient old already get nuked?
		SendError(pMessage->idNetClientNew, LOGINERROR_UNKNOWN); 
		//Yes, the error is sent to New, not Old.
		return;
	}
	//Transfer over the appclient and gameclient
	if(!ClientSystemTransferClients(AppGetClientSystem(), pClient, pMessage->idNetClientNew, pMessage->idNetClientOld))
	{
		SendError(pMessage->idNetClientNew, LOGINERROR_ALREADYONLINE);
		SendError(pMessage->idNetClientOld, LOGINERROR_UNKNOWN);
		return;
	}

	GameServerNetClientRemove(pMessage->idNetClientOld);

	//Should we limbo them?  No for now, but it's a gameplay question.  Potentially:
	/*BOOL bSafeOnExitLimbo = LevelIsActionLevel( UnitGetLevel(pUnit) ); 
	s_UnitPutInLimbo( pPlayer, bSafeOnExitLimbo );*/

	//Danger: maintainability.  This sort of duplicates code from player.cpp and level.cpp
	GameSendToClient(pGame, pClient);
	s_LevelResendToClient(pGame, UnitGetLevel(pUnit), pClient, pUnit);
	HotkeysSendAll(pGame, pClient, pUnit, TAG_SELECTOR_HOTKEY1, (TAG_SELECTOR)TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);
#if HELLGATE_ONLY
	if(AppIsHellgate())
	{
		PlayerSetStartingMinigameTags(pGame, pUnit);
	}
#endif
	//If the player is dead, bring up the death UI by resending his death.
	if(IsUnitDeadOrDying(pUnit))
	{
		MSG_SCMD_UNITDIE msg;
		msg.id = UnitGetId(pUnit);
		msg.idKiller = INVALID_ID;
		s_SendMessage(pGame, ClientGetId(pClient), SCMD_UNITDIE, &msg);
	}

	//Log them into the chat server.
	s_sNetLoginMember(pUnit, pMessage->idAccount);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdPlayerGag(
	GAME *pGame,
	GAMECLIENT * pClient,
	UNIT * pUnit,
	BYTE *pData)
{
	const MSG_APPCMD_PLAYERGAG *pMessage = (const MSG_APPCMD_PLAYERGAG *)pData;
	GAG_ACTION eGagAction = (GAG_ACTION)pMessage->nGagAction;
	
	if (eGagAction == GAG_UNGAG)
	{
		s_GagDisable( pUnit, TRUE );
	}
	else
	{

		// make time 
		time_t timeGaggedUntil = GagGetTimeFromNow( eGagAction );
					
		// for now we'll just gag them forever in this game instance
		s_GagEnable( pUnit, timeGaggedUntil, TRUE );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdUtilityGameRequest(
	GAME *pGame,
	GAMECLIENT * pClient,
	UNIT * pUnit,
	BYTE *pData)
{
	const MSG_APPCMD_UTILITY_GAME_REQUEST *pMessage = (const MSG_APPCMD_UTILITY_GAME_REQUEST *)pData;
	ASSERTX_RETURN(GameIsUtilityGame(pGame), "Game is not a utility game" );

	// process the utility game request
	if ( UtilityGameProcessRequest( pGame, pMessage ) == FALSE)
	{
	
		// TODO: Send a failure message from this game instance to the game server
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdInGameDBCallback(
	GAME *pGame,
	GAMECLIENT * pClient,
	UNIT * pUnit,
	BYTE *pData)
{
	const MSG_APPCMD_IN_GAME_DB_CALLBACK *pMessage = (const MSG_APPCMD_IN_GAME_DB_CALLBACK *)pData;
	ASSERTX_RETURN( pMessage, "Expected message" );
	ASSERTX_RETURN( pMessage->idGame == GameGetId( pGame ), "In game db callback routed to wrong game!" );

	// do the callback
	FP_GAME_DATABASE_CALLBACK fpCallback = (FP_GAME_DATABASE_CALLBACK)pMessage->ullCallbackFunctionPointer;
	if (fpCallback)
	{
		fpCallback( pGame, (BOOL)pMessage->bSuccess, pMessage->bCallbackData );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPlayerRemove(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{

	UNIT * player = ClientGetPlayerUnit(client);
	if (player)
	{	//Note: this save code is also present in ClientRemoveFromGameDo.
		// Cancel any trades so we can save.
		if(TradeIsTrading(player))
			s_TradeCancel( player, FALSE );

#if ISVERSION(SERVER_VERSION)
		// save character digest/wardrobe data on exit
		CharacterDigestSaveToDatabase(player);

		// save the player before removing them
		if (s_IncrementalDBUnitsAreEnabled() == TRUE)
		{
			if (s_DBUnitOperationsImmediateCommit(player) == FALSE)
			{
				TraceError("Error committing pending DB Unit Operations in sCCmdPlayerRemove.  GUID: [%I64d]", player->guid);
			}

			// force a full update of the player unit
			DATABASE_UNIT tDBUnit;
			s_DBUnitInit(&tDBUnit);
			BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
			DWORD dwDataBufferSize = arrsize( bDataBuffer );
			int nValidationErrors = 0;
			if (s_DBUnitCreate( player, &tDBUnit, bDataBuffer, arrsize( bDataBuffer ), nValidationErrors, TRUE ) == FALSE)
			{
				TraceError("Unable to create db unit for same on sCCmdPlayerRemove GUID: [%I64d]", player->guid);
			}
			if (tDBUnit.guidUnit != INVALID_GUID)
			{
				// Send update request to database
				UnitFullUpdateInDatabase( player, &tDBUnit, bDataBuffer );
			}
			else
			{
				TraceError("Invalid DB Unit on sCCmdPlayerRemove: [%I64d]", player->guid);
			}

			// Clear buffers
			bDataBuffer[ 0 ];
			dwDataBufferSize = 0;
			s_DBUnitInit(&tDBUnit);
		}
#else
		AppPlayerSave(player);
#endif
	}

	ClientRemoveFromGame(game, client, 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdReqMove(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	s_PlayerMoveMsgHandleRequest( game, client, unit, data );
}

// TRAVIS - this is only sent by players
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ALLOWED_WARP_PATH_RESYNC_TIME 1*GAME_TICKS_PER_SECOND

void sCCmdUnitMoveXYZ(
	GAME* game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	UNITLOG_ASSERT_RETURN( AppIsTugboat(), unit );
	
	MSG_CCMD_UNITMOVEXYZ* msg = (MSG_CCMD_UNITMOVEXYZ *)data;

	// moving forces you to stop talking to anybody
	// since this is a player-only move request, we can do this.
	s_InteractCancelUnitsTooFarAway( unit );

	UnitSetVelocity(unit, msg->fVelocity);
	float fModeDuration = 0.0f;
	if(msg->mode == MODE_KNOCKBACK)
	{
		UnitSetFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
		fModeDuration = VectorLength(UnitGetPosition(unit) - msg->TargetPosition) / msg->fVelocity;
	}
	// need to send? I don't think so - the move request to everybody else will set the mdoe, 
	// and the original client already knows about it.
	s_UnitSetMode(unit, (UNITMODE)msg->mode, 0, fModeDuration, 0, FALSE);

	VECTOR vDirection = msg->MoveDirection;
	if ( !(msg->bFlags & MOVE_FLAG_USE_DIRECTION) && msg->TargetPosition.x != 0.0f)
	{
		VectorSubtract(vDirection, msg->TargetPosition, UnitGetPosition(unit));
		VectorNormalize(vDirection, vDirection);
	}

	if ( msg->bFlags & MOVE_FLAG_NOT_ONGROUND )
	{
		UnitSetOnGround( unit, FALSE );
	}

	if ( msg->bFlags & MOVE_FLAG_CLEAR_ZVELOCITY )
	{
		UnitSetZVelocity( unit, 0.0f );
	}

	UnitSetMoveTarget(unit, msg->TargetPosition, vDirection);
	if (unit->m_pPathing && !UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
	{
		// clear where I was  - on the server, gotta keep it neat with player-sent pathing!
		UnitClearBothPathNodes( unit );
		BYTE bPathFlags = 0;
		SETBIT(&bPathFlags, PSS_APPEND_DESTINATION_BIT);
		PathStructDeserialize(unit, msg->buf, bPathFlags);
		UnitReservePathOccupied( unit );
		UnitStepListAdd( game, unit );
	}
	else
	{
		UnitStepListAdd( game, unit );
	}

	// Validate path isn't too desynced
	if( AppIsHellgate() )
	{
		if(GameGetTick(game) - unit->tiLastGameTickWarped >= ALLOWED_WARP_PATH_RESYNC_TIME)
			s_UnitTestPathSync(game, unit);
	}

	// player has moved
	s_PlayerMoved( unit );

	// now send to everybody else
	GAMECLIENTID idClient = ClientGetId( client );
	s_SendUnitMoveXYZ( unit, 0, MODE_RUN,  unit->vMoveTarget, unit->vMoveDirection, 0, TRUE, idClient );	
	
}

// TRAVIS - this is only sent by players
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdUnitSetFaceDirection(
	GAME* game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_UNITSETFACEDIRECTION* msg = (MSG_CCMD_UNITSETFACEDIRECTION*)data;



	VECTOR vDirection = msg->FaceDirection;
	VectorNormalize(vDirection, vDirection);

	UnitSetFaceDirection(unit, vDirection);
	// TODO -  need to notify other clients of the update
	MSG_SCMD_UNITTURNXYZ turnMsg;
	turnMsg.id = UnitGetId(unit);
	turnMsg.vFaceDirection = msg->FaceDirection;
	turnMsg.vUpDirection = VECTOR(0, 0, 1);
	turnMsg.bImmediate = FALSE;
	GAMECLIENTID idClient = ClientGetId( client );

	s_SendUnitMessage( unit, SCMD_UNITTURNXYZ, &turnMsg, idClient );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Tugboat code
void sCCmdUnitPathPosUpdate(
	GAME* game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	UNITLOG_ASSERT_RETURN( 0, unit ); // I don't this this message is valid anymore - Tyler

	MSG_CCMD_UNIT_PATH_POSITION_UPDATE * msg = ( MSG_CCMD_UNIT_PATH_POSITION_UPDATE * )data;

	

	ROOM * room = RoomGetByID( game, msg->idPathRoom );
	if ( !room )
	{
		return;
	}

	// update me!
	c_UnitSetPathServerLoc( unit, room, msg->nPathIndex );
}

//end tugboat code

//----------------------------------------------------------------------------
// Previously tugboat-only, but now using this to communicate 
// "fell out of the world" teleports to the server.
//----------------------------------------------------------------------------
void sCCmdUnitWarp(
	GAME* game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_UNITWARP* msg = (MSG_CCMD_UNITWARP*)data;

	ROOM* room = RoomGetByID(game, msg->idRoom);
	if ( !room )
	{
		return;
	}
	ASSERT_RETURN(room);
	
	if (UnitGetContainer( unit ))
	{
		ASSERTX( UnitIsPhysicallyInContainer( unit ) == FALSE, "Unit is warping that is physically inside its container" );
		
		//if (UnitIsPhysicallyInContainer( unit ) == TRUE)
		//{
		//
		//	// update hotkeys
		//	c_ItemHotkeyUpdateDropItem(game, unit);

		//	// failsafe to make sure item is not in any inventory
		//	UnitInventoryRemove(unit, ITEM_ALL);
		//	ASSERT(UnitGetContainer(unit) == NULL);
		//
		//}
	}

	// do the warp	
	UnitWarp(
		unit, 
		room, 
		msg->vPosition, 
		msg->vFaceDirection, 
		msg->vUpDirection, 
		msg->dwUnitWarpFlags,
		TRUE,
		FALSE); // don't send to client that sent it.

	client->m_vPositionLastReported = msg->vPosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdUnitFellOutOfWorld(
	GAME* game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	PlayerWarpToStartLocation( game, unit );
	s_MetagameEventFellOutOfWorld( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillSaveAssign(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_SKILLSAVEASSIGN * pMsg = (MSG_CCMD_SKILLSAVEASSIGN *) data;
	HotkeySet( pGame, unit, pMsg->bySkillAssign, pMsg->wSkill );
	/*//Put in check for skill validity before assigning.
	const SKILL_DATA * pSkillData = SkillGetData( pGame, pMsg->wSkill );
	UNITLOG_ASSERT_RETURN(pSkillData && SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_MOUSE ), unit );
	//end check.

	UnitSetStat( unit, pMsg->bySkillAssign == 0 ? STATS_SKILL_LEFT : STATS_SKILL_RIGHT, pMsg->wSkill );

	int nCurrentConfig = UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG);
	UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(unit, nCurrentConfig + TAG_SELECTOR_WEAPCONFIG_HOTKEY1 );
	ASSERT_RETURN(pTag);
	if (pMsg->bySkillAssign == 0)
		pTag->m_nSkillID[0] = pMsg->wSkill;
	else
		pTag->m_nSkillID[1] = pMsg->wSkill;*/

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSetDebugUnit(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_SETDEBUGUNIT * pMsg = (MSG_CCMD_SETDEBUGUNIT *) data;
	GameSetDebugUnit(pGame, pMsg->idDebugUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdLevelLoaded(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	s_UnitTakeOutOfLimbo(unit);
	UnitTriggerEvent(pGame, EVENT_PRELOADED, unit, NULL, NULL);
	ObjectsCheckForTriggers( pGame, client, unit );
	
	//	TODO: update chat server as to location.
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdItemSell(
	GAME * pGame, 
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_ITEMSELL *pMessage = (const MSG_CCMD_ITEMSELL *)pData;
	
	// verify player
	UNIT *pPlayer = pUnit;
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( UnitGetClient( pPlayer ) == pClient, "Client mismatch" );

	// get merchant we're selling to	
	UNITID idMerchant = pMessage->idSellTo;
	UNIT *pMerchant = UnitGetById( pGame, idMerchant );
	ASSERTX_RETURN( pMerchant, "Expected unit" );
	ASSERTX_RETURN( UnitIsMerchant( pMerchant ), "Expected merchant" );
	ASSERTX_RETURN( TradeIsTradingWith( pPlayer, pMerchant ), "Not trading with merchant unit" );
	
	// get item we're selling
	UNITID idItem = pMessage->idItem;
	UNIT *pItem = UnitGetById( pGame, idItem );
	ASSERTX_RETURN( pItem, "Expected item unit" );
	
	// do the sell	
	// if we do this AFTER the store-sell, the item could be freed!
	s_InventoryClearWaitingForMove(pGame, pClient, pItem);
	s_StoreSellItem( pPlayer, pItem, pMerchant );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdItemBuy(
	GAME * pGame, 
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_ITEMBUY *pMessage = (const MSG_CCMD_ITEMBUY *)pData;
	
	// verify player
	UNIT *pPlayer = pUnit;
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( UnitGetClient( pPlayer ) == pClient, "Client mismatch" );

	// get merchant we're selling to	
	UNITID idMerchant = pMessage->idBuyFrom;
	UNIT *pMerchant = UnitGetById( pGame, idMerchant );
	UNITLOG_ASSERTX_RETURN( pMerchant, "Expected unit", pPlayer );
	UNITLOG_ASSERTX_RETURN( UnitIsMerchant( pMerchant ), "Expected merchant", pPlayer );
	UNITLOG_ASSERTX_RETURN( TradeIsTradingWith( pPlayer, pMerchant ), "Not trading with merchant unit", pPlayer );

	// get item we're buying
	UNITID idItem = pMessage->idItem;
	UNIT *pItem = UnitGetById( pGame, idItem );
	if (pItem == NULL)
	{
		return;  // this is a valid *non* cheating case for shared merchant stores
	}

	// the merchant does not technically own the item, the player does.
	// not true, we can have shared merchant stores, mythos uses them still i believe -cday
	//ASSERTX_RETURN( UnitGetUltimateOwner(pItem) == pMerchant, "Merchant does not own item");
	
	// setup inventory location
	INVENTORY_LOCATION tInvLoc;
	tInvLoc.nInvLocation = pMessage->nSuggestedLocation;
	tInvLoc.nX = pMessage->nSuggestedX;
	tInvLoc.nY = pMessage->nSuggestedY;

	// buy it
	UNITLOG_ASSERT(s_StoreBuyItem( pPlayer, pItem, pMerchant, tInvLoc ), pPlayer );

	if (pItem)		// it could've been freed if it was part of a stack or sump'n
		s_InventoryClearWaitingForMove(pGame, pClient, pItem);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdItemPickedItem(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_ITEM_PICK_ITEM *pMsg = (MSG_CCMD_ITEM_PICK_ITEM *)data;

	// get item
	UNIT *pItem = UnitGetById( pGame, pMsg->idItem );
	ASSERT_RETURN( pItem);
	
	// get item picked
	UNIT *pItemPicked = UnitGetById( pGame, pMsg->idItemPicked );	
	
	// must have an item at this point
	ASSERT_RETURN( pItemPicked);	
	s_ItemUseOnItem( pGame, UnitGetUltimateOwner( pItem ), pItem, pItemPicked );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTryIdentify(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TRY_IDENTIFY * pMsg = (MSG_CCMD_TRY_IDENTIFY *) data;
	
	// get item
	UNIT *pItem = UnitGetById( pGame, pMsg->idItem );
	ASSERTX_RETURN( pItem, "Expected item to identify" );
	
	// get analyzer
	UNIT *pAnalyzer = UnitGetById( pGame, pMsg->idAnalyzer );	
	
	// if no analyzer, we will look for one on the player
	if (pAnalyzer == NULL)
	{
		pAnalyzer = InventoryGetFirstAnalyzer( unit );	
	}
	
	// must have analyzer at this point
	ASSERTX_RETURN( pAnalyzer, "Expected item analyzer for identify" );
	if(ItemBelongsToAnyMerchant(pItem)) // If its a merchan'ts item, no identify allowed!
		return;
	
	// do the identify
	s_ItemIdentify( unit, pAnalyzer, pItem );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdCanceldentify(
	GAME *pGame, 
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	//const MSG_CCMD_CANCEL_IDENTIFY *pMessage = (const MSG_CCMD_CANCEL_IDENTIFY *)pData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTryDismantle(
	GAME *pGame, 
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	MSG_CCMD_TRY_DISMANTLE *pMessage = (MSG_CCMD_TRY_DISMANTLE *)pData;
	
	// get item
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	ASSERTX_RETURN( pItem, "Expected item to dismantle" );

	// get dismantler item (if any)
	UNIT *pDismantler = NULL;
	if (pMessage->idItemDismantler != INVALID_ID)
	{
		pDismantler = UnitGetById( pGame, pMessage->idItemDismantler );
		ASSERTX_RETURN( pDismantler, "Expected dismantler item to use to dismantle with" );
	}
	
	// do the dismantle
	s_ItemDismantle( pUnit, pItem, pDismantler );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPvPSignUpAccept(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	UNITLOG_ASSERTX_RETURN(AppIsHellgate(),
		"Client trying to use Hellgate-only pvp warp code",	unit);

	// fill out warp spec	
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = GlobalIndexGet( GI_LEVEL_PVPARENA );

	// do the warp
	s_WarpToLevelOrInstance( unit, tSpec );

}


//----------------------------------------------------------------------------
static CCriticalSection				m_csGameInstanceListLock(TRUE);
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdListGameInstances(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if ISVERSION(SERVER_VERSION)


	MSG_CCMD_LIST_GAME_INSTANCES *msg = (MSG_CCMD_LIST_GAME_INSTANCES *)data;

	// SendTownInstanceLists
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);

	GAMECLIENTID idClient = ClientGetId(client);
	ASSERTX_RETURN( idClient != INVALID_GAMECLIENTID, "Invalid client id" );

	// KCK: This needs a critical section to keep these messages from overlapping.
	m_csGameInstanceListLock.Enter();
	MSG_SCMD_GAME_INSTANCE_UPDATE_BEGIN msgBegin;
	msgBegin.nInstanceListType = msg->nTypeOfInstance;
	s_SendMessage( pGame, idClient, SCMD_GAME_INSTANCE_UPDATE_BEGIN, &msgBegin );

	GAME_INSTANCE_INFO_VECTOR		InstanceList;
	new (&InstanceList) GAME_INSTANCE_INFO_VECTOR(serverContext->m_pMemPool);
	TownInstanceListGetAllInstances( serverContext->m_pInstanceList, InstanceList );

	MSG_SCMD_GAME_INSTANCE_UPDATE msgGut;
	for( unsigned int i = 0; i < InstanceList.size(); i++ )
	{
		// KCK: Apply filter here by the type we requested
		if ( msg->nTypeOfInstance == InstanceList[i].nInstanceType )
		{
			msgGut.idGame = InstanceList[i].idGame;
			msgGut.nInstanceNumber = InstanceList[i].nInstanceNumber;
			msgGut.idLevelDef = InstanceList[i].nLevelDefinition;
			msgGut.nInstanceType = InstanceList[i].nInstanceType;
			msgGut.nPVPWorld = InstanceList[i].nPVPWorld;
			s_SendMessage( pGame, idClient, SCMD_GAME_INSTANCE_UPDATE, &msgGut);
		}
	}

	InstanceList.clear();
	InstanceList.~GAME_INSTANCE_INFO_VECTOR();

	MSG_SCMD_GAME_INSTANCE_UPDATE_END msgEnd;
	s_SendMessage( pGame, idClient, SCMD_GAME_INSTANCE_UPDATE_END, &msgEnd);
	m_csGameInstanceListLock.Leave();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdWarpToGameInstance(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if ISVERSION(SERVER_VERSION)
	MSG_CCMD_WARP_TO_GAME_INSTANCE *msg = (MSG_CCMD_WARP_TO_GAME_INSTANCE *)data;

	// KCK: Hellgate doesn't use Area ID, use  Definition Index instead
	// Find the area id of the area we'd like to warp to
	int nAreaID = INVALID_ID;
	if (msg->nTypeOfInstance != GAME_INSTANCE_PVP)
	{
		if ( AppIsTugboat() )
			nAreaID = UnitGetLevelAreaID( unit );
		if ( AppIsHellgate() )
			nAreaID = UnitGetLevelDefinitionIndex( unit );
	}
	else
	{
		nAreaID = msg->m_nLevelDef;
	}

	if( nAreaID != INVALID_ID &&
		!IsUnitDeadOrDying( unit ) &&
		GameGetId( pGame ) != msg->idGameToWarpTo )
	{

		GameServerContext * gameContext = (GameServerContext*)CurrentSvrGetContext();
		GAME_INSTANCE_INFO InstanceInfo;
		// For towns: make sure this REALLY IS a valid instance and not just some game somewhere, and that it matches the area we area 
		// actually in. 
		// no town warping with this!
		// KCK TODO: We need to do more checks vs PVP Instances before just allowing the player to warp there. Such as,
		// is the game full? Does it actually exist? Is it guild only? Errors should be returned to notify user of the problem
		if ( TownInstanceListGetInstanceInfo( gameContext->m_pInstanceList, nAreaID, msg->idGameToWarpTo, InstanceInfo ) &&
			 msg->nTypeOfInstance == InstanceInfo.nInstanceType )
		{

			WARP_SPEC tSpec;
			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
			tSpec.nLevelDepthDest = 0;	
			tSpec.nLevelAreaDest = nAreaID;
			tSpec.nLevelAreaPrev = nAreaID;
			tSpec.nPVPWorld = PlayerIsInPVPWorld( unit );
			tSpec.idGameOverride = msg->idGameToWarpTo;


			if( s_TownPortalGet( unit ) )
			{
				s_TownPortalsClose( unit );
			}

			s_WarpToLevelOrInstance(unit, tSpec);
		}
	}

#endif // ISVERSION(SERVER_VERSION)
}


// vvv Tugboat-specific
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdReturnToLowestDungeonLevel(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	UNITLOG_ASSERTX_RETURN(AppIsTugboat(), "Tugboat Only", unit);
	
	int DeepestDepth = UnitGetStat( unit, STATS_LEVEL_DEPTH );	
	if( DeepestDepth <= 0 )
	{
		return; //bogus value
	}
	
	//Take money
	cCurrency cost = PlayerGetReturnToDungeonCost(unit);		
	UNITLOG_ASSERT_RETURN(UnitRemoveCurrencyValidated( unit, cost ), unit);	

	// fill out warp spec	
	WARP_SPEC tSpec;
	tSpec.nLevelAreaDest = UnitGetStat( unit, STATS_LAST_LEVEL_AREA );
	tSpec.nLevelDepthDest = DeepestDepth;
	tSpec.nPVPWorld = ( PlayerIsInPVPWorld( unit ) ? 1 : 0 );

	// do the warp
	s_WarpToLevelOrInstance( unit, tSpec );

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdBuyHireling(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{	
	MSG_CCMD_BUY_HIRELING *pMessage = (MSG_CCMD_BUY_HIRELING *)data;

	UNITLOG_ASSERTX_RETURN(AppIsTugboat(), "Tugboat Only", unit);

	int nBuyIndex = pMessage->nIndex;

	// who is the player talking to
	UNIT *pForeman = s_TalkGetTalkingTo( unit );
	s_NPCBuyHireling( unit, pForeman, nBuyIndex );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAchievementSelected(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{	
	MSG_CCMD_ACHIEVEMENT_SELECTED *pMessage = (MSG_CCMD_ACHIEVEMENT_SELECTED *)data;
	ASSERT_RETURN( pMessage );
	x_AchievementPlayerSelect( unit, pMessage->nAchievementID, pMessage->nAchievementSlot );	
}



//----------------------------------------------------------------------------
// Validation strategy: get the LEVEL_AREA_DEFINITION
// check existence and map visibility.
//----------------------------------------------------------------------------
static void sCCmdRequestAreaWarp(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_REQUEST_AREA_WARP *pMsg = (MSG_CCMD_REQUEST_AREA_WARP*) data;

	UNITLOG_ASSERTX_RETURN(AppIsTugboat(), "Tugboat Only", unit);

	if( pMsg->nLevelArea == INVALID_ID ||
		pMsg->nLevelArea < 0  )
	{
		return;
	}


	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.dwWarpFlags = pMsg->dwWarpFlags;
	tSpec.nLevelDepthDest = pMsg->nLevelDepth;	
	tSpec.nLevelAreaDest = pMsg->nLevelArea;
	tSpec.nLevelAreaPrev = UnitGetLevelAreaID( unit );
	tSpec.nPVPWorld = PlayerIsInPVPWorld( unit );
	MYTHOS_LEVELAREAS::ConfigureLevelAreaByLinker( tSpec, unit, ((pMsg->bSkipRoad > 0 )?TRUE:FALSE) );
	
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = 
		MYTHOS_LEVELAREAS::LevelAreaDefinitionGet(pMsg->nLevelArea );

	UNITLOG_ASSERTX_RETURN(pLevelArea,
		"Client requesting warp to nonexistent area.",
		unit);

	UNITLOG_ASSERTX_RETURN( pMsg->nLevelDepth == 0 || ( pLevelArea->nCanWarpToSection != 0 && pMsg->nLevelDepth == pLevelArea->nCanWarpToSection - 1 ), 
		"Currently, unmarked depths are not allowed for AreaWarp",
		unit);	

	UNITLOG_ASSERT_RETURN(
		MYTHOS_LEVELAREAS::PlayerCanTravelToLevelArea( unit, pMsg->nLevelArea ),
		unit);

	if(pMsg->guidPartyMember == INVALID_GUID)
	{	//If you're not going to a party member, you need the map to it.
		UNITLOG_ASSERTX_RETURN( MYTHOS_LEVELAREAS::LevelAreaIsKnowByPlayer( unit, tSpec.nLevelAreaDest ),
			"Client requesting warp to area not visible to it.",
			unit);
	}
	else
	{	//Verify that their party member is in that game and area.
		GAMEID idPartyMemberGame = PartyCacheGetPartyMemberGameId(unit, pMsg->guidPartyMember);
		BOOL bInPVPWorld = PartyCacheGetPartyMemberPVPWorld( unit, pMsg->guidPartyMember );
		int nArea = PartyCacheGetPartyMemberLevelArea(unit, pMsg->guidPartyMember);
		if(idPartyMemberGame != pMsg->idGameOfPartyMember || nArea != tSpec.nLevelAreaDest ||
			bInPVPWorld != PlayerIsInPVPWorld( unit ) )
		{
			// even if the party member isn't in the requested area - if we know about it,
			// we can still go.
			if( !MYTHOS_LEVELAREAS::LevelAreaIsKnowByPlayer( unit, tSpec.nLevelAreaDest ) )
			{
				TraceGameInfo("Rejecting area warp from player %ls to client with guid %I64x in area %d.",
					unit->szName, pMsg->guidPartyMember, tSpec.nLevelAreaDest);

				return;
			}
			else
			{
				// but since we're not warping to the party member, don't try to warp to their game!
				pMsg->guidPartyMember = INVALID_GUID;
			}
		}
	}


	// the following block of code should be done in a common place where a player
	// is removed from the game, also perhaps before the player is saved -Colin
	{	
		// players can't maintain tasks across instanced dungeons - only quests.	
		s_TaskCloseAll( unit );
		if( s_TownPortalGet( unit ) )
		{
			s_TownPortalsClose( unit );
		}
	}

	

	//If we're warping to a party member, force a warp to his game
	if(pMsg->guidPartyMember != INVALID_GUID)
		tSpec.idGameOverride = pMsg->idGameOfPartyMember;
	
	// do the warp
	s_WarpToLevelOrInstance( unit, tSpec );
	
}
// ^^^ Tugboat-specific

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdQuestAccept(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_QUEST_TRY_ACCEPT *pMsg = (MSG_CCMD_QUEST_TRY_ACCEPT*) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
		
	s_QuestAccept( pGame, pPlayer, UnitGetById( pGame, pMsg->idQuestGiver ), pMsg->nQuestID );
}

//----------------------------------------------------------------------------
//Clears all player modifications to a recipe
//----------------------------------------------------------------------------
static void sCCmdRecipeClearPropertis(GAME * pGame, 
									  GAMECLIENT * client,
									  UNIT * unit,
									  BYTE * data)
{	
	UNIT *pPlayer = ClientGetControlUnit( client );
	ASSERT_RETURN( pPlayer );	
	s_RecipeClearPlayerModifiedRecipeProperties( pPlayer);
}

//----------------------------------------------------------------------------
//retrieves the message from the client saying what property on the recipe has been changed
//----------------------------------------------------------------------------
static void sCCmdRecipeSetProperty(
	  GAME * pGame, 
	  GAMECLIENT * client,
	  UNIT * unit,
	  BYTE * data)
{
	MSG_CCMD_RECIPE_SET_PLAYER_MODIFICATIONS *pMsg = (MSG_CCMD_RECIPE_SET_PLAYER_MODIFICATIONS *) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
	ASSERT_RETURN( pPlayer );	
	s_RecipeSetPlayerModifiedRecipeProperty( pPlayer, pMsg->nRecipeProperty, pMsg->nRecipePropertyValue  );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdRecipeCreate(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_RECIPE_TRY_CREATE *pMsg = (MSG_CCMD_RECIPE_TRY_CREATE *) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
	UNIT *pRecipeGiver = UnitGetById( pGame, pMsg->idRecipeGiver );
	s_RecipeCreate( pPlayer, pRecipeGiver, pMsg->nRecipe, pMsg->nIndex  );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdRecipeBuy(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_RECIPE_TRY_BUY *pMsg = (MSG_CCMD_RECIPE_TRY_BUY *) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
	UNIT *pRecipeGiver = UnitGetById( pGame, pMsg->idRecipeGiver );
	s_RecipeBuy( pPlayer, pRecipeGiver, pMsg->nRecipe );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdRecipeClose(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);
	s_RecipeClose( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdRecipeSelect(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);
	const MSG_CCMD_RECIPE_SELECT *pMsg = (const MSG_CCMD_RECIPE_SELECT *) data;	
	UNITID idRecipeGiver = pMsg->idRecipeGiver;
	UNIT *pRecipeGiver = UnitGetById( game, idRecipeGiver );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
	//TODO: Validate that we are talking to the recipe giver.
	
	// save this recipe is selected ...  aka "in focus" or something
	s_RecipeSelect( unit, pMsg->nRecipe, pMsg->nIndexInRecipeList, pRecipeGiver );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdCubeTryCreate(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
	ASSERT_RETURN(game && unit);

	s_CubeTryCreate( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdItemSplitStack(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_ITEM_SPLIT_STACK * pMsg = (MSG_CCMD_ITEM_SPLIT_STACK*) data;

	// get item
	UNIT *pItem = UnitGetById( game, pMsg->idItem );
	ASSERTX_RETURN( pItem, "Expected item to split" );

	// KCK: Security measure, check stack size prior to split in order to prevent duping.
	if (pMsg->nStackPrevious != UnitGetStat(pItem, STATS_ITEM_QUANTITY))
		return;
	if (pMsg->nStackPrevious - pMsg->nNewStackSize <= 0)
		return;

	s_ItemPickupFromStack(unit, pItem, pMsg->nNewStackSize);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTaskAccept(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TASK_TRY_ACCEPT *pMsg = (MSG_CCMD_TASK_TRY_ACCEPT*) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
		
	s_TaskAccept( pPlayer, pMsg->nTaskID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTaskAbandon(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TASK_TRY_ABANDON *pMsg = (MSG_CCMD_TASK_TRY_ABANDON *) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
		
	s_TaskAbandon( pPlayer, pMsg->nTaskID );	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdQuestTryAbandon(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_QUEST_TRY_ABANDON *pMsg = (MSG_CCMD_QUEST_TRY_ABANDON *) data;
	UNIT *pPlayer = ClientGetControlUnit( client );

	s_QuestClearFromUser( pPlayer, QuestGetActiveTask( pPlayer, pMsg->nQuestID ) );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTaskAcceptReward(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TASK_TRY_ACCEPT_REWARD *pMsg = (MSG_CCMD_TASK_TRY_ACCEPT_REWARD *) data;
	UNIT *pPlayer = ClientGetControlUnit( client );
		
	s_TaskAcceptReward( pPlayer, pMsg->nTaskID );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTalkDone(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TALK_DONE *pMessage = (MSG_CCMD_TALK_DONE *)data;	
	s_NPCTalkStop( pGame, unit, (CONVERSATION_COMPLETE_TYPE)pMessage->nConversationCompleteType, pMessage->nRewardSelection );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTutorialUpdate(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TUTORIAL_UPDATE * pMessage = (MSG_CCMD_TUTORIAL_UPDATE *)data;
	s_TutorialUpdate( unit, pMessage->nType, pMessage->nData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdTryReturnToHeadstone(
	GAME *pGame, 
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRY_RETURN_TO_HEADSTONE *pMessage = (const MSG_CCMD_TRY_RETURN_TO_HEADSTONE *)pData;	
	if (pMessage->bAccept == TRUE)
	{
		s_PlayerTryReturnToHeadstone( pUnit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define CLIENT_SKILL_START_ALLOWABLE_BUFFER 3.0f
#define CLIENT_SKILL_START_ALLOWABLE_BUFFER_SQ ((CLIENT_SKILL_START_ALLOWABLE_BUFFER)*(CLIENT_SKILL_START_ALLOWABLE_BUFFER))
#define CLIENT_SKILL_START_ALLOWABLE_SEQUENCE_OFFSET 10

static BOOL sSkillSeedValidateInputs(
	UNIT * pUnit,
	const VECTOR & vPosition,
	const VECTOR & vWeaponPosition,
	const VECTOR & vWeaponDirection)
{
	if(VectorDistanceSquared(UnitGetPosition(pUnit), vPosition) > CLIENT_SKILL_START_ALLOWABLE_BUFFER_SQ)
	{
		return FALSE;
	}

	GAME * pGame = UnitGetGame(pUnit);
	int nWeaponIndex = 0;
	UNIT * pWeapon = WardrobeGetWeapon(pGame, UnitGetWardrobe(pUnit), nWeaponIndex);
	if(!pWeapon)
	{
		nWeaponIndex++;
	}
	VECTOR vWeaponPositionServer, vWeaponDirectionServer;
	UnitGetWeaponPositionAndDirection(pGame, pUnit, nWeaponIndex, &vWeaponPositionServer, &vWeaponDirectionServer);

	if(VectorDistanceSquared(vWeaponPositionServer, vWeaponPosition) > CLIENT_SKILL_START_ALLOWABLE_BUFFER_SQ)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sGetSkillSeed(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	const VECTOR & vPosition,
	const VECTOR & vWeaponPosition,
	const VECTOR & vWeaponDirection)
{
	if(sSkillSeedValidateInputs(pUnit, vPosition, vWeaponPosition, vWeaponDirection))
	{
		return SkillGetCRC(vPosition, vWeaponPosition, vWeaponDirection);
	}
	else
	{
		return SkillGetNextSkillSeed( pGame );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillStart(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_SKILLSTART * pMsg = (MSG_CCMD_SKILLSTART *) data;
	DWORD dwSkillSeed = sGetSkillSeed(pGame, unit, pMsg->wSkill, pMsg->vPosition, pMsg->vWeaponPosition, pMsg->vWeaponDirection);
	SkillStartRequest( pGame, unit, pMsg->wSkill, INVALID_ID, VECTOR(0), 0, dwSkillSeed );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillStartXYZ(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit);

	MSG_CCMD_SKILLSTARTXYZ * pMsg = (MSG_CCMD_SKILLSTARTXYZ *) data;
	DWORD dwSkillSeed = sGetSkillSeed(pGame, unit, pMsg->wSkill, pMsg->vPosition, pMsg->vWeaponPosition, pMsg->vWeaponDirection);
	SkillStartRequest( pGame, unit, pMsg->wSkill, INVALID_ID, pMsg->vTarget, 0, dwSkillSeed );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillStartID(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit);

	MSG_CCMD_SKILLSTARTID * pMsg = (MSG_CCMD_SKILLSTARTID *) data;
	DWORD dwSkillSeed = sGetSkillSeed(pGame, unit, pMsg->wSkill, pMsg->vPosition, pMsg->vWeaponPosition, pMsg->vWeaponDirection);
	SkillStartRequest( pGame, unit, pMsg->wSkill, pMsg->idTarget, VECTOR(0), 0, dwSkillSeed );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillStartXYZID(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit);

	MSG_CCMD_SKILLSTARTXYZID * pMsg = (MSG_CCMD_SKILLSTARTXYZID *) data;
	DWORD dwSkillSeed = sGetSkillSeed(pGame, unit, pMsg->wSkill, pMsg->vPosition, pMsg->vWeaponPosition, pMsg->vWeaponDirection);
	SkillStartRequest( pGame, unit, pMsg->wSkill, pMsg->idTarget, pMsg->vTarget, 0, dwSkillSeed );
}

//----------------------------------------------------------------------------
// VERIFICATION: is new target allowed?
//----------------------------------------------------------------------------
static void sCCmdSkillChangeID(
	GAME * pGame,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit);

	MSG_CCMD_SKILLCHANGETARGETID * pMsg = (MSG_CCMD_SKILLCHANGETARGETID *) data;

	const SKILL_DATA *pSkillData = SkillGetData( pGame, pMsg->wSkill );
	UNITLOG_ASSERT_RETURN( pSkillData, unit );

	// Do we need to verify the targets?
	if(SkillDataTestFlag(pSkillData, SKILL_FLAG_VERIFY_TARGETS_ON_REQUEST))
	{
		UNIT * pTarget = UnitGetById( pGame, pMsg->idTarget );
		UNIT * pWeapon = NULL;

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON ) )
		{
			UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
			UnitGetWeapons( unit, pMsg->wSkill, pWeapons, FALSE );
			pWeapon = pWeapons[ 0 ];
		}

		UNITLOG_ASSERTX_RETURN(
			SkillIsValidTarget(pGame, unit, pTarget, pWeapon, pMsg->wSkill , TRUE, NULL),
			"Switching skill to invalid target",
			unit);
	}
	SkillSetTarget( unit, pMsg->wSkill, pMsg->idWeapon, pMsg->idTarget, pMsg->bIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillEnd(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_SKILLSTOP* msg = (MSG_CCMD_SKILLSTOP*)data;
	ASSERT_RETURN(unit)
	if (IsUnitDeadOrDying(unit))
	{
		return;
	}

	if (msg->wSkill == -1)
	{
		SkillStopAll(game, unit);
	}
	else
	{
		int skill = msg->wSkill;
		if ((unsigned int)skill < ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_SKILLS))
		{
			SkillStopRequest( unit, msg->wSkill, TRUE, TRUE );
		}
		else UNITLOG_ASSERT(FALSE, unit);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSkillShiftEnable(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_SKILLSHIFT_ENABLE * msg = (MSG_CCMD_SKILLSHIFT_ENABLE*)data;
	ASSERT_RETURN(unit);

	int nSkill = msg->wSkill;
	BOOL bEnable = msg->bEnabled != 0;

	UNITLOG_ASSERT_RETURN((unsigned int)nSkill < ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_SKILLS), unit);

	const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );
	UNITLOG_ASSERT_RETURN( pSkillData != NULL, unit);

	UNITLOG_ASSERT_RETURN( UnitIsA( unit, pSkillData->nRequiredUnittype ), unit );

	UnitSetStat( unit, STATS_SKILL_SHIFT_ENABLED, nSkill, bEnable ? 1 : 0 );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPlayerRespawn(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	const MSG_CCMD_PLAYER_RESPAWN *pRespawnMessage = (const MSG_CCMD_PLAYER_RESPAWN *)data;
	
	// do player restart
	PLAYER_RESPAWN eChoice = (PLAYER_RESPAWN)pRespawnMessage->nPlayerRespawn;

	if (eChoice != PLAYER_RESPAWN_INVALID)
	{
		UNITLOG_ASSERT_RETURN( eChoice >= (PLAYER_RESPAWN)0, unit );
		UNITLOG_ASSERT_RETURN( eChoice < NUM_PLAYER_RESPAWN_TYPES, unit );
		UNITLOG_ASSERT_RETURN( g_bClientCanRequestRespawn[eChoice], unit );
	}

	s_PlayerRestart( unit, eChoice );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPickup(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERTX_RETURN( unit, "Expected unit to pick something up" );
	
	// cannot pickup stuff when you're dead (legitimate case, not hacking!)
	if (IsUnitDeadOrDying( unit ))
	{
		return;
	}
	
	MSG_CCMD_PICKUP * msg = (MSG_CCMD_PICKUP *)data;
	UNIT * selection = UnitGetById(game, msg->id);
	
	// if item is already gone, forget it (legitimate case, not hacking!)
	if (selection == NULL)
	{
		return;
	}
	if (!UnitTestFlag(unit, UNITFLAG_CANPICKUPSTUFF))
	{
		return;
	}
	if (!UnitTestFlag(selection, UNITFLAG_CANBEPICKEDUP))
	{
		// can't pickup something that is flagged as not being able to, unless it is 
		// already in the process of being picked up by you anyway
		UNITID idBeingPickedUpBy = UnitGetStat(selection, STATS_BEING_PICKED_UP_BY_UNITID );
		if (idBeingPickedUpBy != UnitGetId( unit ))
		{
			return;
		}
	}
	if (!UnitGetRoom(selection))
	{
		return;
	}
	if (UnitGetLevel(unit) != UnitGetLevel(selection))
	{
		return;
	}
	if ( AppIsHellgate() && VectorDistanceSquared(unit->vPosition, selection->vPosition) > PICKUP_RADIUS_SQ)
	{
 		return;
	}
	else if ( AppIsTugboat() && VectorDistanceSquaredXY(unit->vPosition, selection->vPosition) > PICKUP_RADIUS_SQ)
	{
		return;
	}

	if ( ItemCanPickup(unit, selection) != PR_OK)
	{
		return;
	}

	// TRAVIS : In Tug we are having issues with small objects on the other side of
	// crypts and the like that can't be picked up due to LOS issues.
	if ( AppIsHellgate() && ClearLineOfSightBetweenUnits( unit, selection ) == FALSE)
	{
		return;
	}
		
	s_ItemPullToPickup(unit, selection);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdInvEquip(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	MSG_CCMD_INVEQUIP * msg = (MSG_CCMD_INVEQUIP *)data;
	
	UNIT * item = UnitGetById(game, msg->id);
	UNITLOG_ASSERT_RETURN(item, unit);

	if (UnitGetContainer(item) != unit)
	{
		return;
	}

	// equip the item
	// get the current location of the item
	int location;
	if (!UnitGetInventoryLocation(item, &location))
	{
		return;
	}
	
	INVLOC_HEADER info;
	if (!UnitInventoryGetLocInfo(unit, location, &info))
	{
		return;
	}

	if (info.nFilter > 0)
	{	// unequip the item
		int location, x, y;
		if (!ItemGetOpenLocation(unit, item, FALSE, &location, &x, &y))
		{
			return;
		}
		if (!InventoryItemCanBeEquipped(unit, item, location))
		{
			return;
		}
		UnitInventoryAdd(INV_CMD_PICKUP, unit, item, location, x, y);
	}
	else
	{	// equip the item
		int location, x, y;
		BOOL bEmptySlot = ItemGetEquipLocation(unit, item, &location, &x, &y);

		if (!bEmptySlot)
		{
			UNIT * curr = UnitInventoryGetByLocationAndXY(unit, location, x, y);
			ASSERT(curr);
			if (curr)
			{
				int currloc, currx, curry;
				if (!ItemGetOpenLocation(unit, curr, FALSE, &currloc, &currx, &curry))
				{
					return;
				}
				UNITLOG_ASSERT_RETURN(InventoryItemCanBeEquipped(unit, item, curr, NULL, location), unit);
				UnitInventoryAdd(INV_CMD_PICKUP, unit, curr, currloc);
			}
		}

		UNITLOG_ASSERT_RETURN(InventoryItemCanBeEquipped(unit, item, NULL, NULL, location), unit);
		UnitInventoryAdd(INV_CMD_PICKUP, unit, item, location, x, y);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdInvDrop(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	ASSERT_RETURN( pUnit );
	const MSG_CCMD_INVDROP *pDropMessage = (const MSG_CCMD_INVDROP *)pData;
	
	UNIT *pItem = UnitGetById( pGame, pDropMessage->id );
	UNITLOG_ASSERT_RETURN( pItem, pUnit );

	DROP_RESULT eResult = InventoryCanDropItem( pUnit, pItem );
	if ( eResult == DR_OK || eResult == DR_OK_DESTROY )
	{

		// give server an opportunity to respond to this removal of an item
		ItemRemovingFromCanTakeLocation( pUnit, pItem );
		
		// destroy item
		if (AppIsHellgate())
		{
			UnitFree( pItem, UFF_SEND_TO_CLIENTS, UFC_ITEM_DESTROY );
		}
		else
		{
			// drop the item
			DROP_RESULT eResult = s_ItemDrop( pUnit, pItem );
			if( eResult == DR_OK_DESTROYED )
			{
				return; //item has been destroyed
			}	
			// clear waiting to move logic
			s_InventoryClearWaitingForMove(pGame, pClient, pItem);
			
			// if we failed, tell the client why
			if (eResult != DR_OK)
			{
				s_ItemSendCannotDrop( pUnit, pItem, eResult );
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdInvPut(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));
	MSG_CCMD_INVPUT * msg = (MSG_CCMD_INVPUT *)data;

	// setup location
	INVENTORY_LOCATION invloc;
	invloc.nInvLocation = msg->bLocation;
	invloc.nX = msg->bX;
	invloc.nY = msg->bY;

	// get container for item
	UNIT * container = UnitGetById(game, msg->idContainer);
	if (container == NULL || IsUnitDeadOrDying(container))
	{
		return;
	}

	// note we need to make sure that the dest container and the item are valid for the player to manipulate
	
	// get item
	UNIT * item = UnitGetById(game, msg->idItem);
	if (item == NULL)
	{
		return;
	}
	
	// this should not be considered a cheating as there are valid cases of the server moving
	// and item at the same time that a client tries to move an item ... this typically happens when
	// entering a game and the server is trying to fix up errors and/or lost items and the newly
	// online client is also trying to fix up items in cursors etc
	if (UnitGetUltimateContainer(item) != unit)
	{
		return;
	}
	
	// don't allow clients to put into restricted slots
	if (InventoryLocPlayerCanPut(container, item, invloc.nInvLocation) == FALSE)	
	{
		s_InventoryClearWaitingForMove(game, client, item);
		return;
	}
	
	// Don't let the player take from an inventory they're not supposed to take from
	INVENTORY_LOCATION currInvLoc;
	UnitGetInventoryLocation(item, &currInvLoc);
	if (currInvLoc.nInvLocation != INVLOC_NONE &&
		InventoryLocPlayerCanTake(UnitGetContainer(item), item, currInvLoc.nInvLocation) == FALSE)
	{
		s_InventoryClearWaitingForMove(game, client, item);
		return;
	}

	// do the put
	s_InventoryPut(container, item, invloc);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdExitGame(
	GAME * game, 
	GAMECLIENT* client,
	UNIT * unit,
	BYTE * data)
{
	const MSG_CCMD_EXIT_GAME *pMessage = (const MSG_CCMD_EXIT_GAME *)data;
	REF( pMessage );

#if ISVERSION(SERVER_VERSION)
	if(game && game->pServerContext && game->pServerContext->m_pPerfInstance)
		PERF_ADD64(game->pServerContext->m_pPerfInstance,GameServer,GameServerCCMD_EXIT_GAMECount,1);	
#endif
	
	s_GameExit( game, client );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdInvSwap(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_INVSWAP* msg = (MSG_CCMD_INVSWAP*)data;
	
	UNIT * itemSrc = UnitGetById(game, msg->idSrc);
	UNIT * itemDest = UnitGetById(game, msg->idDest);
	UNITLOG_ASSERT_RETURN(itemSrc && itemDest, unit); //shouldn't ask to swap nonexistent items.
	//It's possible we should do an ASSERT_DO and call s_InventoryClearWaitingForMove, but screw it.
	//we'll do that if some untoward effect of not doing it comes up.  This assert should
	//never happen for non-hacked clients.

	// you cannot swap with items that are in locations that you cannot normally put them
	// into (server controlled slots like rewards, stores, offerings, etc)
	INVENTORY_LOCATION tInvLocSrc;
	INVENTORY_LOCATION tInvLocDest;
	UnitGetInventoryLocation( itemSrc, &tInvLocSrc );
	UNIT *pContainerSource = UnitGetContainer( itemSrc );
	UnitGetInventoryLocation( itemDest, &tInvLocDest );	
	UNIT *pContainerDest = UnitGetContainer( itemDest );
	if (InventoryLocPlayerCanPut( pContainerSource, itemDest, tInvLocSrc.nInvLocation ) == FALSE ||
		InventoryLocPlayerCanPut( pContainerDest, itemSrc, tInvLocDest.nInvLocation ) == FALSE ||
		InventoryLocPlayerCanTake( pContainerSource, itemSrc, tInvLocSrc.nInvLocation ) == FALSE ||
		InventoryLocPlayerCanTake( pContainerDest, itemDest, tInvLocDest.nInvLocation ) == FALSE)
	{
		if (itemDest)
			s_InventoryClearWaitingForMove(game, client, itemDest);		
		if (itemSrc)
			s_InventoryClearWaitingForMove(game, client, itemSrc);		
		return;
	}

	//check that the unit actually owns the items here.
	UNITLOG_ASSERT_RETURN(UnitGetUltimateContainer( itemSrc ) == unit, unit);
	UNITLOG_ASSERT_RETURN(UnitGetUltimateContainer( itemDest ) == unit, unit);

	
	// If we're swapping with one of the hands, we have to treat it just as if it were a weaponconfig
	int nDestLocation = INVLOC_NONE;
	if ( AppIsHellgate() &&
		 UnitGetInventoryLocation(itemDest, &nDestLocation) &&
		 UnitGetContainer(itemDest) == unit &&
		(GetWeaponconfigCorrespondingInvLoc(0) == nDestLocation ||
		 GetWeaponconfigCorrespondingInvLoc(1) == nDestLocation))
	{
		MSG_CCMD_ADD_WEAPONCONFIG HotkeyMsg;
		HotkeyMsg.bHotkey = (BYTE)(TAG_SELECTOR_WEAPCONFIG_HOTKEY1 + UnitGetStat(unit, STATS_CURRENT_WEAPONCONFIG));
		HotkeyMsg.bSuggestedPos = (BYTE)(GetWeaponconfigCorrespondingInvLoc(1) == nDestLocation);
		HotkeyMsg.idItem = msg->idSrc;
		HotkeyMsg.nFromWeaponConfig = msg->nFromWeaponConfig;
		sCCmdAddToWeaponConfig(game, client, unit, (BYTE *)&HotkeyMsg);
		s_InventoryClearWaitingForMove(game, client, itemDest);		// screw it.  clear the waiting flag for the dest item just in case
	}
	else
	{
		int nPos = -1;
		UNIT_TAG_HOTKEY * pSwapSourceTag = UnitGetHotkeyTag(unit, msg->nFromWeaponConfig);
		if (pSwapSourceTag)
		{
			for (int i=0; i < MAX_HOTKEY_ITEMS; i++)
			{
				if (pSwapSourceTag->m_idItem[i] == msg->idSrc)
				{
					nPos = i;
				}
			}
		}

		if (pSwapSourceTag && nPos != -1)
		{
			// we're swapping to a weapon config.  Do that.
			MSG_CCMD_ADD_WEAPONCONFIG HotkeyMsg;
			HotkeyMsg.bHotkey = (BYTE)msg->nFromWeaponConfig;
			HotkeyMsg.bSuggestedPos = (BYTE)nPos;
			HotkeyMsg.idItem = msg->idDest;
			HotkeyMsg.nFromWeaponConfig = -1;
			sCCmdAddToWeaponConfig(game, client, unit, (BYTE *)&HotkeyMsg);
			s_InventoryClearWaitingForMove(game, client, itemDest);		// screw it.  clear the waiting flag for the dest item just in case
		}
		else
		{
			// just do a normal swap
			// we need to save off the IDs in case the items get freed (as they will in the case of stacking items)
			int idDest = UnitGetId(itemDest);
			int idSrc = UnitGetId(itemSrc);
			InventorySwapLocation(itemSrc, itemDest, msg->nAltDestX, msg->nAltDestY);
			itemDest = UnitGetById(game, idDest);
			itemSrc = UnitGetById(game, idSrc);
			if (itemDest)
				s_InventoryClearWaitingForMove(game, client, itemDest);		
			if (itemSrc)
				s_InventoryClearWaitingForMove(game, client, itemSrc);		
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdDoubleEquip(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	MSG_CCMD_DOUBLEEQUIP* msg = (MSG_CCMD_DOUBLEEQUIP*)data;
	
	InventoryDoubleEquip(game, unit, msg->idItem1, msg->idItem2, msg->bLocation1, msg->bLocation2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdItemInvPut(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * pPlayer,
	BYTE * data)
{
	ASSERT_RETURN(pPlayer && !IsUnitDeadOrDying(pPlayer));

	MSG_CCMD_ITEMINVPUT* msg = (MSG_CCMD_ITEMINVPUT*)data;
	int nInvLocation = msg->bLocation;
	int nInvX = msg->bX;
	int nInvY = msg->bY;
	
	UNIT *pContainer = UnitGetById(game, msg->idContainer);
	if (!pContainer)
	{
		return;
	}

	INVENTORY_LOCATION tInvLocCurrent;
	UNIT *pItem = UnitGetById(game, msg->idItem);
	if (!pItem)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}
	if (UnitGetOwnerId(pItem) != UnitGetId(pPlayer) &&
		UnitGetUltimateOwner(pItem) != pPlayer)
	{
		UNITLOG_ASSERT(FALSE, unit);
		goto _fail;
	}
	if (ItemBelongsToAnyMerchant(pContainer) || 
		ItemBelongsToAnyMerchant(pItem))
	{
		goto _fail;
	}

	// check the special requirements for crafting socketables
	if( ItemIsACraftedItem( pContainer ) && UnitIsA( pItem, UNITTYPE_CRAFTING_MOD )  )
	{
		if( !CRAFTING::CraftingCanModItem( pContainer, pItem ) )
		{
			goto _fail;
		}
		else
		{
			// wipe out any previous heraldries
			CRAFTING::s_CraftingRemoveMods( pContainer );
		}
	}

	// the container item must be in an inventory location that the player can normally
	// place items into (like a backpack, or equip slot, *not* a offering or reward slot etc)
	if (InventoryLocPlayerCanPut( pContainer, pItem, nInvLocation ) == FALSE)
	{
		goto _fail;
	}

	// Don't let the player take from an inventory they're not supposed to take from
	UnitGetInventoryLocation( pItem, &tInvLocCurrent );
	if (tInvLocCurrent.nInvLocation != INVLOC_NONE &&
		InventoryLocPlayerCanTake( UnitGetContainer(pItem), pItem, tInvLocCurrent.nInvLocation ) == FALSE)
	{
		goto _fail;
	}

	if (!UnitInventoryTest( pContainer, pItem, nInvLocation, nInvX, nInvY ))
	{
		goto _fail;
	}
	if (!InventoryCheckStatRequirements( pContainer, pItem, NULL, NULL, nInvLocation ))
	{
		goto _fail;
	}

	// we need to add the properties to the mod.
	if( AppIsTugboat() ) //This could be removed, but first a few things would need to be done for hellgate. Check out function s_ItemPropsInit
	{
		if( UnitIsA( pItem, UNITTYPE_MOD ) )
		{
			s_ItemPropsInit( game, pItem, UnitGetUltimateOwner( pItem ), pContainer );
		}
	}
		
	// add to new container
	InventoryChangeLocation( pContainer, pItem, nInvLocation, nInvX, nInvY, 0 );
	
	// if this is a mod, and being placed in an item, then the parent item needs
	// to update saved states to take advantage of any visual states that the mod might have
	if( UnitIsA( pContainer, UNITTYPE_ITEM ) &&
		UnitIsA( pItem, UNITTYPE_MOD ) )
	{
		StatesShareModStates( pContainer, pItem );
	}

	return;

_fail:
	if (pItem)
		s_InventoryClearWaitingForMove(game, client, pItem);		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdInvUse(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	MSG_CCMD_INVUSE* msg = (MSG_CCMD_INVUSE*)data;

	UNIT * item = UnitGetById(game, msg->idItem);
	if (!item)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}
	if (UnitGetOwnerId(item) != UnitGetId(unit) &&
		UnitGetUltimateOwner(item) != unit)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}
	
	// you can only use items that are in a location that the player can normally interact with
	UNIT *pContainer = UnitGetContainer( item );
	UNITLOG_ASSERT( pContainer, "Trying to use item not contained by anything" );
	INVENTORY_LOCATION tInvLoc;
	UnitGetInventoryLocation( item, &tInvLoc );
	if (InventoryLocPlayerCanPut( pContainer, item, tInvLoc.nInvLocation ) == FALSE)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}
		
	if (ItemBelongsToAnyMerchant(item))
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;	
	}

	if( ItemHasSkillWhenUsed(item) &&
		UnitDataTestFlag(UnitGetData(item), UNIT_DATA_FLAG_ITEM_IS_TARGETED) )
	{
		UnitSetStatVector(unit, STATS_SKILL_TARGET_X, UnitGetData(item)->nSkillWhenUsed, 0, msg->TargetPosition );
	}

	s_ItemUse(game, unit, item);
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdInvUseOn(
				 GAME * game, 
				 GAMECLIENT * client,
				 UNIT * unit,
				 BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	MSG_CCMD_INVUSEON* msg = (MSG_CCMD_INVUSEON*)data;

	UNIT * item = UnitGetById(game, msg->idItem);
	UNIT * target = UnitGetById(game, msg->idTarget);
	if (!item || !target)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}
	if( !PetIsPet( target ) || PetGetOwner( target ) != UnitGetId( unit ) )
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}
	if (UnitGetOwnerId(item) != UnitGetId(unit) &&
		UnitGetUltimateOwner(item) != unit)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}

	// you can only use items that are in a location that the player can normally interact with
	UNIT *pContainer = UnitGetContainer( item );
	UNITLOG_ASSERT( pContainer, "Trying to use item not contained by anything" );
	INVENTORY_LOCATION tInvLoc;
	UnitGetInventoryLocation( item, &tInvLoc );
	if (InventoryLocPlayerCanPut( pContainer, item, tInvLoc.nInvLocation ) == FALSE)
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;
	}

	if (ItemBelongsToAnyMerchant(item))
	{
		UNITLOG_ASSERT(FALSE, unit);
		return;	
	}

	s_ItemUse(game, target, item);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdInvShowInChat(
				   GAME * game, 
				   GAMECLIENT * client,
				   UNIT * unit,
				   BYTE * data)
{
	ASSERT_RETURN(unit);

	MSG_CCMD_INVSHOWINCHAT* msg = (MSG_CCMD_INVSHOWINCHAT*)data;

	UNIT * item = UnitGetById(game, msg->idItem);
	ASSERT_RETURN(item);


}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdHotKey(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	s_WeaponConfigDoHotkey( game, client, unit, (MSG_CCMD_HOTKEY *)data, TRUE, FALSE );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdAddToWeaponConfig(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	s_WeaponConfigAdd( game, client, unit, (MSG_CCMD_ADD_WEAPONCONFIG*)data );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdSelectWeaponConfig(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	s_WeaponConfigSelect( game, unit, (MSG_CCMD_SELECT_WEAPONCONFIG*)data );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL ValidateCheatMsg(
	MSG_CCMD_CHEAT * msg)
{
	WCHAR * str = msg->str;
	WCHAR * end = str + MAX_CHEAT_LEN;

	while (str < end)
	{
		if (*str == 0)
		{
			return TRUE;
		}
		if (*str > 1 && *str < 32)
		{
			return FALSE;
		}
		str++;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdCheat(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_CHEAT * msg  = (MSG_CCMD_CHEAT *)data;

#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEVELOPMENT)
	if(!ClientHasBadge(client, ACCT_TITLE_CUSTOMER_SERVICE_REPRESENTATIVE) && !ClientHasBadge(client, ACCT_TITLE_TESTER))
	{
		TraceGameInfo("Rejected serverside cheat %ls from badgeless gameclient %I64x", msg->str, ClientGetId(client));
		return;
	}
	else
#endif
	{
		TraceGameInfo("Gameclient %I64x used serverside cheat \"%ls\"", ClientGetId(client), msg->str);
	}
	if (!ValidateCheatMsg(msg))
	{
		return;
	}
	ConsoleParseCommand(game, client, msg->str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdQuestInteractDialog(
				   GAME * game, 
				   GAMECLIENT * client,
				   UNIT * unit,
				   BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	MSG_CCMD_QUEST_INTERACT_DIALOG * msg = ( MSG_CCMD_QUEST_INTERACT_DIALOG * )data;

	UNIT * target = UnitGetById( game, msg->idTarget );

	// this is a valid case (not cheating!)
	if ( !target )
		return;

	if ( UnitCanInteractWith( target, unit ) == FALSE )
		return;

	float fDistance = AppIsHellgate() ? UnitsGetDistanceSquared( unit, target ) : UnitsGetDistanceSquaredMinusRadiusXY( unit, target );
	if (fDistance > UNIT_INTERACT_DISTANCE_SQUARED * 2.0f)
	{
		return;
	}
	s_QuestEventInteract( unit, target, msg->nQuestID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdInteract(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit && !IsUnitDeadOrDying(unit));

	MSG_CCMD_INTERACT * msg = ( MSG_CCMD_INTERACT * )data;

	UNIT * target = UnitGetById( game, msg->idTarget );

	// this is a valid case (not cheating!)
	if ( !target )
		return;

	if ( UnitCanInteractWith( target, unit ) == FALSE )
		return;

	float fDistance = AppIsHellgate() ? UnitsGetDistanceSquared( unit, target ) : UnitsGetDistanceSquaredMinusRadiusXY( unit, target );
	const UNIT_DATA * pUnitData = UnitGetData(target);
	if(pUnitData && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IGNORE_INTERACT_DISTANCE))
	{
		fDistance = 0.0f;
	}

	if (fDistance > UNIT_INTERACT_DISTANCE_SQUARED * 2.0f)
	{
		return;
	}

	// interact baby!
	switch(UnitGetGenus(target))
	{

		//----------------------------------------------------------------------------
		case GENUS_ITEM:
		{
			UNIT_INTERACT eInteract = (UNIT_INTERACT)msg->nInteract;			
			s_UnitInteract( unit, target, eInteract );
			break;
		}

		//----------------------------------------------------------------------------
		case GENUS_MONSTER:
		case GENUS_PLAYER:
		{
			UNIT_INTERACT eInteract = (UNIT_INTERACT)msg->nInteract;			
			s_UnitInteract( unit, target, eInteract );
			break;
		}
		
		//----------------------------------------------------------------------------
		case GENUS_OBJECT:
		{
			ObjectTrigger(game, client, unit, target);
			break;
		}
		
		//----------------------------------------------------------------------------
		default:
		{
			break;
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdRequestInteractChoices(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	MSG_CCMD_REQUEST_INTERACT_CHOICES *pMessage = (MSG_CCMD_REQUEST_INTERACT_CHOICES *)pData;
	UNITID idTarget = pMessage->idUnit;
	UNIT *pTarget = UnitGetById( pGame, idTarget );

	//setup return message
	MSG_SCMD_INTERACT_CHOICES tReturnMsg;
	tReturnMsg.idUnit = idTarget;
	tReturnMsg.nCount = 0;
	for (int i = 0; i < MAX_INTERACT_CHOICES; ++i)
	{
		tReturnMsg.ptInteract[ i ].nInteraction = UNIT_INTERACT_INVALID;
	}
	
	// get available interactions
	tReturnMsg.nCount = x_InteractGetChoices( pUnit, pTarget, tReturnMsg.ptInteract, MAX_INTERACT_CHOICES );
	
	// send response to the client
	s_SendMessage( pGame, pClient->m_idGameClient, SCMD_INTERACT_CHOICES, &tReturnMsg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdSkillSelect(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_SKILLSELECT * msg = ( MSG_CCMD_SKILLSELECT * )data;

	SkillSelect( game, unit, msg->wSkill, FALSE, TRUE, msg->dwSeed );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdSkillDeselect(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_SKILLDESELECT * msg = ( MSG_CCMD_SKILLDESELECT * )data;

	SkillDeselect( game, unit, msg->wSkill, FALSE );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdSkillPickMeleeSkill(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_SKILLPICKMELEESKILL * msg = ( MSG_CCMD_SKILLPICKMELEESKILL * )data;

	int nSkill = msg->wSkill;

	if ( nSkill != INVALID_ID )
	{
		const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );
		UNITLOG_ASSERT_RETURN( pSkillData, unit );
		UNITLOG_ASSERT_RETURN( pSkillData->eActivatorKey == SKILL_ACTIVATOR_KEY_MELEE, unit);
	}
	UnitSetStat( unit, STATS_SKILL_MELEE_SKILL, nSkill );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdReqSkillUp(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_REQ_SKILL_UP * msg = ( MSG_CCMD_REQ_SKILL_UP * )data;

	UNITLOG_ASSERT(SkillPurchaseLevel( unit, msg->wSkill ), unit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdReqTierUp(
					 GAME * game, 
					 GAMECLIENT * client,
					 UNIT * unit,
					 BYTE * data)
{
	ASSERT_RETURN(game && unit);

	//MSG_CCMD_REQ_TIER_UP * msg = ( MSG_CCMD_REQ_TIER_UP * )data;

	// OK, we're going to invest in all tiers equally
	UNITLOG_ASSERT(SkillPurchaseTier( unit, 0/*msg->wSkillTab*/ ), unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdReqCraftingTierUp(
					GAME * game, 
					GAMECLIENT * client,
					UNIT * unit,
					BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_REQ_CRAFTING_TIER_UP * msg = ( MSG_CCMD_REQ_CRAFTING_TIER_UP * )data;

	// OK, we're going to invest in all tiers equally
	UNITLOG_ASSERT(CraftingPurchaseTier( unit, msg->wSkillTab ), unit);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdReqWeaponSetSwap(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_REQ_WEAPONSET_SWAP * msg = ( MSG_CCMD_REQ_WEAPONSET_SWAP * )data;

	if( msg->wSet != UnitGetStat( unit, STATS_ACTIVE_WEAPON_SET ) )
	{
		int nMaxStat = StatsDataGetMaxValue(game, STATS_ACTIVE_WEAPON_SET);
		UNITLOG_ASSERT_RETURN(msg->wSet < nMaxStat && msg->wSet >= 0, unit);
		UnitSetStat( unit, STATS_ACTIVE_WEAPON_SET, msg->wSet );
		InventorySwapWeaponSet( game, unit );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdAggressionToggle(
						   GAME * game, 
						   GAMECLIENT * client,
						   UNIT * unit,
						   BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_AGGRESSION_TOGGLE * msg = ( MSG_CCMD_AGGRESSION_TOGGLE * )data;

	if( PlayerIsInPVPWorld( unit ) )
	{
		UNIT* target = UnitGetById( game, msg->idTarget );
		if( target )
		{
			AddToAggressionList( unit, target );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdReqSkillUnlearn(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

#if !ISVERSION(DEVELOPMENT)
	UNITLOG_ASSERT_RETURN(FALSE, unit);
	//This gives a free respec: under current code, the player is refunded
	//skill points next time he logs on.  Deprecated until we decide to allow individual skill resets.
#endif

	MSG_CCMD_REQ_SKILL_UNLEARN * msg = ( MSG_CCMD_REQ_SKILL_UNLEARN * )data;

	UNITLOG_ASSERT(SkillUnlearn( unit, msg->wSkill ), unit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdAllocStat(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * player,
	BYTE * data)
{
	ASSERT_RETURN(game);

	MSG_CCMD_ALLOCSTAT * msg = (MSG_CCMD_ALLOCSTAT *)data;

	UNIT *unit = UnitGetById(game, msg->idUnit);
	ASSERT_RETURN(unit);
	UNITLOG_ASSERT_RETURN(UnitGetUltimateOwner(unit) == player, player);
	const STATS_DATA * stats_data = StatsGetData(game, msg->wStat);
	UNITLOG_ASSERT_RETURN(stats_data, player);
	ASSERT_RETURN(stats_data->m_nStatsType == STATSTYPE_ALLOCABLE);
	ASSERT_RETURN(stats_data->m_nAssociatedStat[0] > INVALID_LINK);
	// KCK NOTE: this is the number of "unallocated" points we have
	int points = UnitGetStat(unit, stats_data->m_nAssociatedStat[0], 0);

	// KCK: It may be better to make another message for stats trading, but
	// for now this seems to work well with less code bloat. Basically, if 
	// we've got a negative delta we're "buying" a stat point back, so 
	// subtract the cost of doing so.
	// NOTE that there is a small security loophole here in that we don't
	// check the origin of this message or whether the player is trading with
	// a stats trader. I don't think this is a serious problem though.
	if (msg->nPoints < 0)
	{
		const UNIT_DATA * player_data = PlayerGetData(game, UnitGetClass(unit));
		const PLAYERLEVEL_DATA * level_data = PlayerLevelGetData(game, player_data->nUnitTypeForLeveling, UnitGetExperienceLevel(unit));
		cCurrency buyPrice = level_data->nStatRespecCost * msg->nPoints * -1;
		cCurrency playerCurrency = UnitGetCurrency( player );
		int nMin = 0;
		if (msg->wStat == STATS_ACCURACY)
			nMin = player_data->nStartingAccuracy;
		if (msg->wStat == STATS_STRENGTH)
			nMin = player_data->nStartingStrength;
		if (msg->wStat == STATS_STAMINA)
			nMin = player_data->nStartingStamina;
		if (msg->wStat == STATS_WILLPOWER)
			nMin = player_data->nStartingWillpower;
		if ( buyPrice.IsZero() || playerCurrency < buyPrice || !PlayerIsSubscriber(player) || UnitGetBaseStat(unit, msg->wStat) + msg->nPoints < nMin )
		{
			return;
		}
		// take the cost of the item from the player	
		UNITLOG_ASSERT(UnitRemoveCurrencyValidated( player, buyPrice ), player );
	}
	if (points >= msg->nPoints)
	{
		UnitChangeStat(unit, stats_data->m_nAssociatedStat[0], -msg->nPoints);
		UnitChangeStat(unit, msg->wStat, msg->nPoints);
		UnitTriggerEvent( game, EVENT_STAT_POINT_USED, unit, unit, &EVENT_DATA(msg->wStat) );

		// this will verify equip requirements and re-equip items if necessary
		s_InventoryChanged(unit);

	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdAbortQuest(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_ABORTQUEST * msg = ( MSG_CCMD_ABORTQUEST * )data;
	ASSERT_RETURN(msg->idPlayer == UnitGetId(unit));
	s_QuestAbortCurrent( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdClientQuestStateChange(
	GAME *pGame,
	GAMECLIENT *pGameClient,
	UNIT *pUnit,
	BYTE *pData)
{
	ASSERT_RETURN( pGame && pUnit );

	const MSG_CCMD_CLIENT_QUEST_STATE_CHANGE *pMessage = (MSG_CCMD_CLIENT_QUEST_STATE_CHANGE *)pData;
	int nQuest = pMessage->nQuest;
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = (QUEST_STATE_VALUE)pMessage->nValue;

	// send event to server side quest
	s_QuestEventClientQuestStateChange( nQuest, nQuestState, eValue, pUnit );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdOperateWaypoint(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_OPERATE_WAYPOINT * msg = ( MSG_CCMD_OPERATE_WAYPOINT * )data;
	s_LevelWaypointWarp( game, client, unit, msg->nLevelDefinition, msg->nLevelArea );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdOperateAnchorstone(
						  GAME * game,
						  GAMECLIENT * client,
						  UNIT * unit,
						  BYTE * data)
{
	ASSERT_RETURN(game && unit);		
	MSG_CCMD_OPERATE_ANCHORSTONE * msg = ( MSG_CCMD_OPERATE_ANCHORSTONE * )data;
	LEVEL *pLevel = UnitGetLevel( unit );
	ASSERT_RETURN(pLevel);
	
	MYTHOS_ANCHORMARKERS::cAnchorMarkers* pMarkers = LevelGetAnchorMarkers( pLevel );
	if( msg->nAction == 0 )
	{
		
		UNITID idTrigger = UnitGetStat( unit, STATS_TALKING_TO );
		UNIT* pAnchorObject = UnitGetById( game, idTrigger );
		if( pAnchorObject )
		{			
			LevelGetAnchorMarkers( pLevel )->SetAnchorAsRespawnLocation( unit, pAnchorObject );		
		}
	}
	else

	{
		const UNIT_DATA *pObjectAnchorshrine = (const UNIT_DATA *)ExcelGetDataByCode( EXCEL_CONTEXT( NULL ), DATATABLE_OBJECTS, (WORD)msg->wObjectCode ); 		
		ASSERT_RETURN( pObjectAnchorshrine );		
		if( pObjectAnchorshrine )
		{
			int nLineNumberForAnchor = (int)ExcelGetLineByCode( EXCEL_CONTEXT( NULL ), DATATABLE_OBJECTS, (WORD)msg->wObjectCode );
			if( pMarkers->HasAnchorBeenVisited( unit, nLineNumberForAnchor ) )
			{
				for( int t = 0; t < pMarkers->GetAnchorCount(); t++ )
				{
					const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor = pMarkers->GetAnchorByIndex( t );
					if( pAnchor )
					{
						UNIT *pAnchorUnit = UnitGetById( game, pAnchor->nUnitId );
						if( pAnchorUnit->pUnitData == pObjectAnchorshrine )
						{
							//we are on the same level, lets just warp
							VECTOR vPosition = UnitGetPosition( pAnchorUnit );
							vPosition += UnitGetFaceDirection( pAnchorUnit, TRUE ) * 2.0f;
							UnitWarp(unit, UnitGetRoom(unit), vPosition, UnitGetFaceDirection( unit, TRUE ), VECTOR(0, 0, 1), 0);
							UNIT* pItem = NULL;
							while ((pItem = UnitInventoryIterate(unit, pItem)) != NULL)		// might want to narrow this iterate down if we can
							{

								if ( UnitIsA( pItem, UNITTYPE_HIRELING ) ||
									UnitIsA( pItem, UNITTYPE_PET ) )
								{					
									UnitWarp(pItem, UnitGetRoom(pItem), vPosition, UnitGetFaceDirection( pItem, TRUE ), VECTOR(0, 0, 1), 0);

								}
							}
							return;
						}
					}
				}

				////////////////////////////////////////////////////////////////////////////////////////////////////////////
				//Ok we aren't on the same level so we need to switch instances 
				////////////////////////////////////////////////////////////////////////////////////////////////////////////
				ASSERT_RETURN( pObjectAnchorshrine->nLinkToLevelArea != INVALID_ID ); //the anchorshrine HAS to have a levelarea to link to
				WARP_SPEC tWarpSpec;
				tWarpSpec.dwWarpFlags = 0;
				tWarpSpec.nLevelAreaPrev = LevelGetLevelAreaID( pLevel );
				tWarpSpec.nLevelAreaDest = pObjectAnchorshrine->nLinkToLevelArea;
				tWarpSpec.nLevelDepthDest = pObjectAnchorshrine->nLinkToLevelAreaFloor;
				tWarpSpec.wObjectCode = msg->wObjectCode;				
				tWarpSpec.nPVPWorld = PlayerIsInPVPWorld( unit );
				SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_OBJECT_BY_CODE );				
				// do the warp
				s_WarpToLevelOrInstance( unit, tWarpSpec );

			}
		}
	}



}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdOperateRunegate(
							 GAME * game,
							 GAMECLIENT * client,
							 UNIT * unit,
							 BYTE * data)
{
	ASSERT_RETURN(game && unit);

	MSG_CCMD_OPERATE_RUNEGATE * msg = ( MSG_CCMD_OPERATE_RUNEGATE * )data;
	LEVEL *pLevel = UnitGetLevel( unit );
	ASSERT_RETURN(pLevel);




	if( msg->guidPartyMember != INVALID_GUID ||
		msg->nLevelArea != INVALID_LINK )
	{
		msg->nLevelArea = UnitGetStat( unit, STATS_LAST_LEVEL_AREA );

		// setup warp spec
		WARP_SPEC tSpec;
		tSpec.dwWarpFlags = 0;
		tSpec.nLevelDepthDest = 0;
		tSpec.nLevelAreaDest = msg->nLevelArea;
		tSpec.nLevelAreaPrev = UnitGetLevelAreaID( unit );
		tSpec.nPVPWorld = PlayerIsInPVPWorld( unit );
		MYTHOS_LEVELAREAS::ConfigureLevelAreaByLinker( tSpec, unit, FALSE );

		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = 
			MYTHOS_LEVELAREAS::LevelAreaDefinitionGet(msg->nLevelArea );

		UNITLOG_ASSERTX_RETURN(pLevelArea,
			"Client requesting warp to nonexistent area.",
			unit);

		if(msg->guidPartyMember == INVALID_GUID)
		{	//If you're not going to a party member, you need the map to it.
			UNITLOG_ASSERTX_RETURN( MYTHOS_LEVELAREAS::LevelAreaIsRuneStone( tSpec.nLevelAreaDest ),
				"Client requesting warp to area not visible to it.",
				unit);
		}
		else
		{	//Verify that their party member is in that game and area.
			GAMEID idPartyMemberGame = PartyCacheGetPartyMemberGameId(unit, msg->guidPartyMember);
			BOOL bInPVPWorld = PartyCacheGetPartyMemberPVPWorld( unit, msg->guidPartyMember );
			tSpec.nLevelAreaDest = PartyCacheGetPartyMemberLevelArea(unit, msg->guidPartyMember);
			if(idPartyMemberGame != msg->idGameOfPartyMember ||
				bInPVPWorld != PlayerIsInPVPWorld( unit ) )
			{

				if( !MYTHOS_LEVELAREAS::LevelAreaIsRuneStone( tSpec.nLevelAreaDest ) )
				{
					TraceGameInfo("Rejecting area warp from player %ls to client with guid %I64x in area %d.",
						unit->szName, msg->guidPartyMember, tSpec.nLevelAreaDest);

					return;
				}
				else
				{
					// but since we're not warping to the party member, don't try to warp to their game!
					msg->guidPartyMember = INVALID_GUID;
				}
			}
		}



		//If we're warping to a party member, force a warp to his game
		if(msg->guidPartyMember != INVALID_GUID)
			tSpec.idGameOverride =msg->idGameOfPartyMember;

		// do the warp
		s_WarpToLevelOrInstance( unit, tSpec );

		//entering a warp from a static world, we reset the return point
		PlayerSetRespawnLocation( KRESPAWN_TYPE_RETURNPOINT, unit, NULL, pLevel );
		return;

	}







	UNITID idRunestone = (UNITID)msg->idRunestone;
	UNIT* pRunestone = UnitGetById( game, idRunestone );
	if( !pRunestone )
	{
		return;
	}

	UNITID idTrigger = UnitGetStat(unit, STATS_TALKING_TO );
	UNIT *pTrigger = UnitGetById(game, idTrigger);
	if( !pTrigger )
	{
		return;
	}
	// ObjectRequiresItemToOperation( trigger )
	//////////////////////////////////////////////////////////////////////////
	//Iterate the entire players inventory looking for keys.
	//////////////////////////////////////////////////////////////////////////
	int nLevelAreaIDGoingTo = INVALID_ID;	
	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );			
	// only search on person ( need to make this search party members too!		
	UNIT * pItem = NULL;			
	while ((pItem = UnitInventoryIterate( unit, pItem, dwInvIterateFlags )) != NULL)
	{
		if ( pItem == pRunestone )
		{				
			nLevelAreaIDGoingTo = MYTHOS_MAPS::GetLevelAreaID( pItem );
			UnitFree( pItem, UFF_SEND_TO_CLIENTS );			
			break;
		}
	}		
	if( nLevelAreaIDGoingTo == INVALID_ID )
		return;	//no key found
	//////////////////////////////////////////////////////////////////////////
	//Ok found a key - now lets set a stat on the player so we know that this gate has a portal
	//////////////////////////////////////////////////////////////////////////	
	unsigned int nLineNumber = ExcelGetLineByCode( EXCEL_CONTEXT( NULL ), DATATABLE_OBJECTS, pTrigger->pUnitData->wCode );
	UnitSetStat( unit, 
		STATS_RANDOM_WARP_LEVELAREA_ID, 
		nLineNumber,
		nLevelAreaIDGoingTo );


	WARP_SPEC tSpec;	
	tSpec.nLevelAreaPrev = LevelGetLevelAreaID( pLevel );
	tSpec.nLevelAreaDest = nLevelAreaIDGoingTo;	
	tSpec.nLevelDepthDest = 0;
	tSpec.nPVPWorld = 0;	


	//entering a warp from a static world, we reset the return point
	PlayerSetRespawnLocation( KRESPAWN_TYPE_RETURNPOINT, unit, NULL, pLevel );
	// warp!
	s_WarpToLevelOrInstance( unit, tSpec );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeCancel(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(game && unit);
	s_TradeCancel( unit, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeStatus(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	ASSERT_RETURN( pGame && pUnit );
	const MSG_CCMD_TRADE_STATUS *pMessage = (const MSG_CCMD_TRADE_STATUS *)pData;
	s_TradeSetStatus( pUnit, (TRADE_STATUS)pMessage->nStatus, 
		pMessage->nTradeOfferVersion );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeRequestNew(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRADE_REQUEST_NEW *pMessage = (const MSG_CCMD_TRADE_REQUEST_NEW *)pData;
	UNIT *pUnitToTradeWith = UnitGetByGuid( pGame, pMessage->guidToTradeWith );
	if (pUnitToTradeWith != NULL)
	{
		s_TradeRequestNew( pUnit, pUnitToTradeWith );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeRequestNewCancel(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRADE_REQUEST_NEW_CANCEL *pMessage = (const MSG_CCMD_TRADE_REQUEST_NEW_CANCEL *)pData;
	REF(pMessage);
	s_TradeRequestNewCancel( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeRequestAccept(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRADE_REQUEST_ACCEPT	*pMessage = (const MSG_CCMD_TRADE_REQUEST_ACCEPT *)pData;
	UNIT *pAskedToTradeBy = UnitGetById( pGame, pMessage->idToTradeWith );
	if (pAskedToTradeBy)
	{
		s_TradeRequestAccept( pUnit, pAskedToTradeBy );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeRequestReject(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRADE_REQUEST_REJECT	*pMessage = (const MSG_CCMD_TRADE_REQUEST_REJECT *)pData;
	UNIT *pAskedToTradeBy = UnitGetById( pGame, pMessage->idToTradeWith );
	if (pAskedToTradeBy)
	{
		s_TradeRequestReject( pUnit, pAskedToTradeBy );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTradeModifyMoney(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRADE_MODIFY_MONEY *pMessage = (const MSG_CCMD_TRADE_MODIFY_MONEY *)pData;
	cCurrency currency( pMessage->nMoney, pMessage->nRealWorldMoney );
	s_TradeModifyMoney( pUnit, currency );
}

//----------------------------------------------------------------------------
// Client request to inspect another player's paperdoll
//----------------------------------------------------------------------------
void sCCmdInspectPlayer(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_INSPECT_PLAYER *pMessage = (const MSG_CCMD_INSPECT_PLAYER *)pData;
	UNIT	*pTarget = UnitGetById(pGame, pMessage->idToInspect);

	// KCK: This is checked on the client side, but better to check again here just to be safe
	if (!PlayerCanBeInspected( pUnit, pTarget ))
		return;

	// Send information on all equipped inventory items
	UNIT	*item = NULL;	
	DWORD	dwInvFlags = 0;
	dwInvFlags = SETBIT(dwInvFlags, IIF_EQUIP_SLOTS_ONLY_BIT);
	while ((item = UnitInventoryIterate(pTarget, item, dwInvFlags)) != NULL)
	{
		DWORD	dwKnownFlags = 0;
		dwKnownFlags = SETBIT(dwKnownFlags, UKF_ALLOW_INSPECTION);
		s_SendUnitToClient( item, pClient, dwKnownFlags );
	}

	// Send command to inspect
	MSG_SCMD_INSPECT	tMessage;
	tMessage.idUnit = UnitGetId( pTarget );
	GAMECLIENTID idClient = ClientGetId(pClient);
	s_SendMessage( pGame, idClient, SCMD_INSPECT, &tMessage );									
}

//----------------------------------------------------------------------------
// Client request to warp to another player
//----------------------------------------------------------------------------
void sCCmdWarpToPlayer(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
#if ISVERSION(SERVER_VERSION)
	const MSG_CCMD_WARP_TO_PLAYER *pMessage = (const MSG_CCMD_WARP_TO_PLAYER *)pData;
	s_ChatNetRequestPlayerDataForWarpToPlayer(pGame, pUnit, pMessage->guidPlayerToWarpTo);
#endif
}

//----------------------------------------------------------------------------
// Client request to set character options
//----------------------------------------------------------------------------
void sCCmdSetCharacterOptions(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_SET_PLAYER_OPTIONS *pMessage = (const MSG_CCMD_SET_PLAYER_OPTIONS *)pData;
	UNIT	*pTarget = pClient->m_pControlUnit;

	if (pTarget)
	{
		UnitSetStat( pTarget, STATS_CHAR_OPTION_ALLOW_INSPECTION, pMessage->nAllowInspection );
		UnitSetStat( pTarget, STATS_CHAR_OPTION_HIDE_HELMET, pMessage->nHideHelmet );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdRewardTakeAll(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	s_RewardTakeAll( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdRewardCancel(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	s_RewardCancel( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdJumpBegin(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_JUMP_BEGIN *pMessage = (const MSG_CCMD_JUMP_BEGIN *)pData;
	REF(pMessage);

	// setup jump message
	MSG_SCMD_JUMP_BEGIN tMessage;
	tMessage.idUnit = UnitGetId( pUnit );

	// tell any other clients that know about this player that they are jumping
	s_SendUnitMessage( pUnit, SCMD_JUMP_BEGIN, &tMessage, ClientGetId( pClient ) );

	s_PlayerMoved( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdJumpEnd(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_JUMP_END *pMessage = (const MSG_CCMD_JUMP_END *)pData;
	REF(pMessage);

	// setup jump message
	MSG_SCMD_JUMP_END tMessage;
	tMessage.idUnit = UnitGetId( pUnit );
	tMessage.vPosition = UnitGetPosition( pUnit );

	// tell any other clients that know about this player that they are jumping
	s_SendUnitMessage( pUnit, SCMD_JUMP_END, &tMessage, ClientGetId( pClient ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdEnterTownPortal(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_ENTER_TOWN_PORTAL *pMessage = (const MSG_CCMD_ENTER_TOWN_PORTAL *)pData;
	UNITID idPortal = pMessage->idPortal;

	// enter portal	
	s_TownPortalEnter( pUnit, idPortal );
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdSelectReturnDest(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_SELECT_RETURN_DEST *pMessage = (const MSG_CCMD_SELECT_RETURN_DEST *)pData;
	
	UNITLOG_ASSERT_RETURN(
		s_TownPortalValidateReturnDest(pUnit, ClientGetAppId(pClient), 
		pMessage->guidOwner, pMessage->tTownPortalSpec), 
		pUnit);

	s_TownPortalReturnToPortalSpec(pUnit, pMessage->guidOwner, pMessage->tTownPortalSpec);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPartyAdvertise(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_PARTY_ADVERTISE * msg = (MSG_CCMD_PARTY_ADVERTISE *)data;

	ChatFilterStringInPlace(msg->szPartyDesc);

	CHAT_REQUEST_MSG_ADVERTISE_PARTY adMsg;
	adMsg.TagLeader = UnitGetGuid(unit);
	PStrCopy(adMsg.PartyInfo.wszPartyDesc, msg->szPartyDesc, MAX_CHAT_PARTY_DESC);
	adMsg.PartyInfo.wMaxMembers = PIN((WORD)msg->nMaxPlayers, (WORD)2, (WORD)MAX_PARTY_MEMBERS);
	adMsg.PartyInfo.wMinLevel   = PIN((WORD)msg->nMinLevel, (WORD)1, (WORD)UnitGetMaxLevel(unit));
	adMsg.PartyInfo.wMaxLevel   = PIN((WORD)msg->nMaxLevel, (WORD)adMsg.PartyInfo.wMinLevel, (WORD)UnitGetMaxLevel(unit));
	adMsg.PartyInfo.bForPlayerAdvertisementOnly = msg->bForSinglePlayerOnly;
	adMsg.PartyInfo.nMatchType = msg->nMatchType;
	//	TODO: utilize party ad classifications some day...

	if (UnitGetExperienceLevel(unit) < adMsg.PartyInfo.wMinLevel ||
		UnitGetExperienceLevel(unit) > adMsg.PartyInfo.wMaxLevel)
	{
		MSG_SCMD_PARTY_ADVERTISE_RESULT createResult;
		createResult.eResult = CHAT_ERROR_INVALID_PARTY_LEVEL_RANGE;
		SendMessageToClient(game, ClientGetId(client), SCMD_PARTY_ADVERTISE_RESULT, &createResult);
		return;
	}

	GameServerToChatServerSendMessage(
		&adMsg,
		GAME_REQUEST_ADVERTISE_PARTY );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPartyUnadvertise(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_PARTY_UNADVERTISE * msg = (MSG_CCMD_PARTY_UNADVERTISE *)data;
	UNREFERENCED_PARAMETER(msg);

	CHAT_REQUEST_MSG_STOP_ADVERTISING_PARTY stopMsg;
	stopMsg.TagLeader = UnitGetGuid(unit);
	GameServerToChatServerSendMessage(
		&stopMsg,
		GAME_REQUEST_STOP_ADVERTISING_PARTY );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdToggleAutoParty(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_TOGGLE_AUTO_PARTY * msg = (MSG_CCMD_TOGGLE_AUTO_PARTY *)data;
	UNREFERENCED_PARAMETER(msg);
	BOOL bEnabled = s_PlayerIsAutoPartyEnabled( unit );
	s_PlayerEnableAutoParty( unit, !bEnabled );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sPvPToggleEvent( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( game && unit );
	ASSERT_RETFALSE( IS_SERVER( game ) );
	GAMECLIENTID idClient = UnitGetClientId(unit);
	GAMECLIENT * client = ClientGetById(game, idClient);
	ASSERT_RETFALSE( client );

	if ( UnitHasExactState( unit, STATE_PVP_FFA ) )
	{
		// go ahead and disable the world pvp flag
		s_StateClear( unit, UnitGetId( unit ), STATE_PVP_FFA, 0 );
		s_SendPvPActionResult(client->m_idNetClient, WORD(PVP_ACTION_DISABLE), WORD(PVP_ACTION_ERROR_NONE));
	}

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPvPToggle(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_PVP_TOGGLE * msg = (MSG_CCMD_PVP_TOGGLE *)data;
	UNREFERENCED_PARAMETER(msg);
	if ( UnitHasExactState( unit, STATE_PVP_FFA ) )
	{
		// set unit event to disable after a fixed time
		UnitUnregisterTimedEvent( unit, s_sPvPToggleEvent );
		UnitRegisterEventTimed( unit, s_sPvPToggleEvent, &EVENT_DATA(), GAME_TICKS_PER_SECOND * PVP_DISABLE_DELAY_IN_SECONDS);

		// go ahead and disable the world pvp flag
		s_SendPvPActionResult(client->m_idNetClient, WORD(PVP_ACTION_DISABLE_DELAY), WORD(PVP_ACTION_ERROR_NONE));
	}
	else if ( UnitHasState( game, unit, STATE_PVP_ENABLED ) )
	{
		// world pvp is not on, see if we can turn it on
		// in a match or duel, don't change state
		s_SendPvPActionResult(client->m_idNetClient, WORD(PVP_ACTION_ENABLE), WORD(PVP_ACTION_ERROR_ALREADY_ENABLED));

	}
	else if (!DuelUnitHasActiveOrPendingDuel(unit))
	{
		// go ahead and turn on world pvp
		s_StateSet( unit, unit, STATE_PVP_FFA, 0, 0 );
		s_SendPvPActionResult(client->m_idNetClient, WORD(PVP_ACTION_ENABLE), WORD(PVP_ACTION_ERROR_NONE));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPing(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_PING* msg = (MSG_CCMD_PING *)data;

	if(msg->bIsReply)
	{ //Trace the round trip time
		client->m_nMissedPingCount = 0; //ping0
		int nRoundTripTime = (int)GetRealClockTime() - msg->timeOfSend;
		if(!AppIsSinglePlayer())
		{
			TraceGameInfo("Round trip ping of %d milliseconds from client %016I64x.\n", nRoundTripTime, ClientGetId(client));
			REF(nRoundTripTime);	// CHB 2007.02.01
		}

#if ISVERSION(SERVER_VERSION)
		static CCritSectLite csPing;
		static float TotalPing = 0.0f;
		static const float PingMovingCount = 1000.0f;

		{
			CSLAutoLock autolock(&csPing);
			TotalPing = TotalPing - (TotalPing / PingMovingCount);
			TotalPing += (float)nRoundTripTime;
		}

		GameServerContext * pContext = game->pServerContext;
		if (pContext) 			
		{
			pContext->m_pPerfInstance->Ping = (int)(TotalPing / PingMovingCount);
		}
#endif
	}
	else
	{ //Send it back to them as a reply
		MSG_SCMD_PING sMsg;
		sMsg.timeOfSend = msg->timeOfSend;
		sMsg.timeOfReceive = (int)GetRealClockTime();
		sMsg.bIsReply = TRUE;
		s_SendMessage(game, ClientGetId(client), SCMD_PING, &sMsg);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPickColorset(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN( unit );

	MSG_CCMD_PICK_COLORSET * msg = (MSG_CCMD_PICK_COLORSET *)data;

	UNIT * pItem = UnitGetById( game, msg->idItem );
	if ( ! pItem )
		return;

	s_WardrobeSetColorsetItem( unit, pItem );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------	
void sCCmdBotCheat(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_BOT_CHEAT *pMessage = (const MSG_CCMD_BOT_CHEAT *)pData;

#if !ISVERSION(DEBUG_VERSION)
	//Only allow if we have a bot account badge.
	BOOL bBotCheatsOn = ClientHasBadge(pClient, ACCT_TITLE_BOT);
	if(!bBotCheatsOn) UNITLOG_ASSERTX_RETURN(FALSE, "Client requesting bot cheat", unit); 
#endif

	BOTCHEAT eCheat = (BOTCHEAT)pMessage->nCheatType;
	switch(eCheat)
	{
	case BOTCHEAT_IDENTIFY:
		{
			UNIT *pItem = UnitGetById( pGame, pMessage->dwParam1 );
			ASSERTX_RETURN( pItem, "Expected item" );
			s_ItemIdentify( pUnit, NULL, pItem );
			break;
		}
	case BOTCHEAT_MOVE_TUGBOAT:
		{
			//Use serverside pathing to create a CCmdUnitMoveXYZ to ourselves.
			ASSERTX_RETURN(AppIsTugboat(), "Tugboat specific cheat!");
			ASSERTX_RETURN(pUnit, "Requires unit.");
			
			pUnit->idMoveTarget = INVALID_ID;
			pUnit->vMoveTarget = pMessage->vPosition;
			if(UnitCalculatePath(pGame, pUnit))
			{
				MSG_CCMD_UNITMOVEXYZ msg;
				int nBufferLength = AICopyPathToMessageBuffer(pGame, pUnit, msg.buf);
				MSG_SET_BLOB_LEN(msg, buf, nBufferLength);
				msg.fVelocity = 5.0f;//UnitGetVelocity( pUnit );

				sCCmdUnitMoveXYZ(pGame, pClient, pUnit, (BYTE*)&msg);
			}
			else
			{
				ROOM * room = 
					RoomGetFromPosition(pGame, UnitGetLevel(pUnit), &pMessage->vPosition);
				if(room)
				{
					UnitWarp(pUnit, room, pMessage->vPosition, 
					UnitGetFaceDirection( pUnit, FALSE ), 
					UnitGetVUpDirection( pUnit ), 0,
					TRUE, FALSE);
				}
			}
			break;
		}
	case BOTCHEAT_WARPAREA_TUGBOAT:
		{
			// setup warp spec
			WARP_SPEC tSpec;
			tSpec.nLevelAreaDest = pMessage->dwParam1;
			tSpec.nLevelDepthDest = pMessage->dwParam2;
			tSpec.nPVPWorld = ( PlayerIsInPVPWorld( pUnit ) ? 1 : 0 );
			TraceGameInfo("Bot warping to area %d at depth %d", 
				pMessage->dwParam1, pMessage->dwParam2);

			// do the warp
			s_WarpToLevelOrInstance( pUnit, tSpec );

			break;
		}
	case BOTCHEAT_WARPLEVEL_HELLGATE:
		{
			// setup warp spec
			WARP_SPEC tSpec;
			tSpec.nLevelDefDest = pMessage->dwParam1;

			TraceGameInfo("Bot warping to level definition %d",
				pMessage->dwParam1);

			// do the warp
			s_WarpToLevelOrInstance( pUnit, tSpec );

			break;
		}
	case BOTCHEAT_MOVE_HELLGATE:
		{
			s_BotMoveMsgHandleRequest( pGame, pClient, pUnit, pData );
			break;
		}
	default:
		{
			TraceError("Client requested unimplemented botcheat %d", eCheat);
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPartyMemberWarpTo(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_PARTY_MEMBER_TRY_WARP_TO *pMessage = (const MSG_CCMD_PARTY_MEMBER_TRY_WARP_TO *)pData;
	PGUID guidPartyMember = pMessage->guidPartyMember;
	
	// open a portal to the party member
	s_WarpPartyMemberOpen( pUnit, guidPartyMember );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPartyAcceptInvite(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	const MSG_CCMD_PARTY_ACCEPT_INVITE *pMessage = (const MSG_CCMD_PARTY_ACCEPT_INVITE *)data;

	// check if the party member accepting needs to abandon their current game,
	MSG_SCMD_PARTY_ACCEPT_INVITE_CONFIRM msg_outgoing;
	msg_outgoing.guidInviter = pMessage->guidInviter;
	msg_outgoing.bAbandonGame = (BYTE) s_PlayerMustAbandonGameForParty(unit);
	SendMessageToClient(game, ClientGetId(client), SCMD_PARTY_ACCEPT_INVITE_CONFIRM, &msg_outgoing);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPartyJoinAttempt(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	// check if the party member accepting needs to abandon their current game,
	MSG_SCMD_PARTY_JOIN_CONFIRM msg_outgoing;
	msg_outgoing.bAbandonGame = (BYTE) s_PlayerMustAbandonGameForParty(unit);
	SendMessageToClient(game, ClientGetId(client), SCMD_PARTY_JOIN_CONFIRM, &msg_outgoing);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdStashClose(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_STASH_CLOSE *pMessage = (const MSG_CCMD_STASH_CLOSE *)pData;
	REF( pMessage );
	s_StashClose( pUnit );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTryItemUpgrade(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	s_ItemUpgrade( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTryItemAugment(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_TRY_ITEM_AUGMENT *pMessage = (const MSG_CCMD_TRY_ITEM_AUGMENT *)pData;
	ITEM_AUGMENT_TYPE eAugmentType = (ITEM_AUGMENT_TYPE)pMessage->bAugmentType;
	s_ItemAugment( pUnit, eAugmentType );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdTryItemUnMod(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	s_ItemUnMod( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdItemUpgradeClose(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	const MSG_CCMD_ITEM_UPGRADE_CLOSE *pMessage = (const MSG_CCMD_ITEM_UPGRADE_CLOSE *)pData;
	ITEM_AUGMENT_TYPE eType = (ITEM_AUGMENT_TYPE)pMessage->bItemUpgradeType;
	switch(eType)
	{
	case IUT_UPGRADE:
		s_ItemUpgradeClose( pUnit );
		break;
	case IUT_AUGMENT:
		s_ItemAugmentClose( pUnit );
		break;
	case IUT_UNMOD:
		s_ItemUnModClose( pUnit );
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdEmote(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pUnit,
	BYTE *pData)
{
	ASSERT_RETURN(pGame);
	const MSG_CCMD_EMOTE *pMessage = (const MSG_CCMD_EMOTE *)pData;

	// Some emotes are restricted.  See if this is one and if so, can it be performed.
	if( pMessage->mode != INVALID_ID)	// only going to care about ones that animate
	{
		unsigned int nCount = ExcelGetCount(pGame, DATATABLE_EMOTES);
		for (unsigned int ii = 0; ii < nCount; ++ii)
		{
			const EMOTE_DATA *pEmote = (EMOTE_DATA *) ExcelGetData(pGame, DATATABLE_EMOTES, ii);
			if (pEmote && pEmote->nUnitMode == pMessage->mode)
			{
				if (!x_EmoteCanPerform(pUnit, pEmote))
				{
					return;
				}
				break;
			}
		}
	}

	s_Emote( pGame, pUnit, pMessage->uszMessage );
	// if the emote included a requested mode set, then do it
	if( pMessage->mode != INVALID_ID)
	{
		BOOL bHasMode;
		float fDuration = UnitGetModeDuration(pGame, pUnit, (UNITMODE)pMessage->mode, bHasMode);
		if (bHasMode)
		{
			const UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, (UNITMODE)pMessage->mode );
			ASSERT_RETURN(pModeData);
			UNITLOG_ASSERT_RETURN(pModeData->nGroup == MODEGROUP_EMOTE, pUnit);

			if( UnitTestModeGroup( pUnit, MODEGROUP_EMOTE ) )
			{
				UnitEndModeGroup( pUnit, MODEGROUP_EMOTE );
			}
			s_UnitSetMode(pUnit, (UNITMODE)pMessage->mode, 0, fDuration);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdWho(
	GAME *game,
	GAMECLIENT *pPlayerClient,
	UNIT *pUnit,
	BYTE *pData)
{
	UNREFERENCED_PARAMETER(pData);
	UNREFERENCED_PARAMETER(pUnit);

	unsigned int total = 0;

	for (GAMECLIENT *pClient = ClientGetFirstInGame( game ); 
		pClient; 
		pClient = ClientGetNextInGame( pClient ))
	{
		WCHAR szCharacter[MAX_CHARACTER_NAME];

		if(ClientGetName(pClient, szCharacter, MAX_CHARACTER_NAME) )
		{
			s_SendSysText(CHAT_TYPE_WHISPER, FALSE, game, pPlayerClient, szCharacter);
			total++;			
		}
	}
	
	// do NOT put hard coded english here, english CANNOT show up in foreign languages!!!!
	s_SendSysTextFmt(CHAT_TYPE_WHISPER, FALSE, game, pPlayerClient, CONSOLE_SYSTEM_COLOR,
		//GlobalStringGet( GS_TOTAL_PLAYERS_CURRENT_GAME ),
		L"=== %d ===", //workaround for global strings not working properly on server at the moment.
		total);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdKillPet(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_KILLPET * msg = (MSG_CCMD_KILLPET *)data;
	ASSERT_RETURN(msg && game && client);
	UNIT * pet = UnitGetById(game, msg->id);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETURN(player);
	// the things that can be killed by this message should be limited as much as possible, to avoid exploits
	if (!pet || !PetIsPlayerPet(game, pet) || PetGetOwner(pet) != UnitGetId(player))
	{
		return;	
	}

	SETBIT(pet->pdwFlags, UNIT_DATA_FLAG_DISMISS_PET);
		
	UnitDie(pet, NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdJoinInstancingChannel(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_JOIN_INSTANCING_CHANNEL * msg = 
		(MSG_CCMD_JOIN_INSTANCING_CHANNEL *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	s_ChatNetAddPlayerToInstancingChannel(
		UnitGetGuid(unit), game, unit, msg->szInstancingChannelName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdQuestTrack(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_QUEST_TRACK * msg = 
		(MSG_CCMD_QUEST_TRACK *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	QUEST *pQuest = QuestGetQuest(unit, msg->nQuestID, FALSE);
	if (pQuest)
	{
		s_QuestTrack( pQuest, msg->bTracking );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAutoTrackQuests(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_AUTO_TRACK_QUESTS * msg = 
		(MSG_CCMD_AUTO_TRACK_QUESTS *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	UnitSetStat(unit, STATS_DONT_AUTO_TRACK_QUESTS, !msg->bTracking);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdUIReload(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	ASSERT_RETURN(unit);

	// just sent it to the quest system.  Then vigorously wash hands.
	MetagameSendMessage( unit, QM_SERVER_RELOAD_UI, NULL );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdRemoveFromWeaponConfig(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_REMOVE_FROM_WEAPONCONFIG * msg = 
		(MSG_CCMD_REMOVE_FROM_WEAPONCONFIG *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	WeaponConfigRemoveItemReference(unit, msg->idItem, msg->nWeaponConfig);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdRequestRealMoneyTxn(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(data && client);
	const MSG_CCMD_REQUEST_REAL_MONEY_TXN& msg = *reinterpret_cast<MSG_CCMD_REQUEST_REAL_MONEY_TXN*>(data);

	// FIXME:  we need to validate the purchase request before forwarding it onto the billing system.
	// we should then wait for a positive response before applying the purchase to the character's inventory
	BILLING_REQUEST_REAL_MONEY_TXN Request;
	Request.Context = client->m_idAppClient;
	DBG_ASSERT(static_cast<APPCLIENTID>(Request.Context) == client->m_idAppClient);
	Request.AccountId = NetClientGetUniqueAccountId(client->m_idNetClient);
	Request.iItemCode = msg.iItemCode;
	PStrCopy(Request.szItemDesc, msg.szItemDesc, arrsize(Request.szItemDesc));
	Request.iPrice = msg.iPrice;
	GameServerToBillingProxySendMessage
		(&Request, EBillingServerRequest_RealMoneyTxn, NetClientGetBillingInstance(client->m_idNetClient));
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAccountStatusUpdate(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_ACCOUNT_STATUS_UPDATE * msg = 
		(MSG_CCMD_ACCOUNT_STATUS_UPDATE *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	UnitSetStat( unit, STATS_MONEY_REALWORLD, msg->nCurrencyBalance );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAccountBalanceUpdate(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_ACCOUNT_BALANCE_UPDATE * msg = 
		(MSG_CCMD_ACCOUNT_BALANCE_UPDATE *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	UnitSetStat( unit, STATS_MONEY_REALWORLD, msg->nCurrencyBalance );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdQuestAbandon(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_QUEST_ABANDON * msg = (MSG_CCMD_QUEST_ABANDON *)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	QUEST *pQuest = QuestGetQuest(unit, msg->nQuestID, FALSE);
	if (pQuest)
	{
		s_QuestAbandon( pQuest );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdMovieFinished(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_CCMD_MOVIE_FINISHED * msg = (MSG_CCMD_MOVIE_FINISHED*)data;

	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);

	// Dave: Do you thing here...
	s_QuestBackFromMovie( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdDuelInvite(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_DUEL_INVITE *msg = (MSG_CCMD_DUEL_INVITE *)data;
	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);
	UNIT *playerToInvite = UnitGetByGuid(game, msg->guidOpponent);
	DUEL_ILLEGAL_REASON nIllegalReasonInviter = DUEL_ILLEGAL_REASON_UNKNOWN;
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee = DUEL_ILLEGAL_REASON_UNKNOWN;
	if (DuelCanPlayersDuel(unit, playerToInvite, &nIllegalReasonInviter, &nIllegalReasonInvitee))
	{
		ASSERTX_RETURN( UnitHasClient( playerToInvite ), "Player being invited to a duel no client" );
		GAMECLIENTID idClient = UnitGetClientId( playerToInvite );	
		MSG_SCMD_DUEL_INVITE msg_outgoing;
		msg_outgoing.idOpponent = UnitGetId(unit);
		// mark the inviter as having invited that unit, in case they accept
		UnitSetStat(unit, STATS_PVP_DUEL_OPPONENT_ID, UnitGetId(playerToInvite));
		UnitSetStat(playerToInvite, STATS_PVP_DUEL_INVITER_ID, msg_outgoing.idOpponent);

		s_SendMessage(game, idClient, SCMD_DUEL_INVITE, &msg_outgoing);
	}
	else
	{
		ASSERTX_RETURN( UnitHasClient( unit ), "Player inviting another to a duel has no client" );
		GAMECLIENTID idClient = UnitGetClientId( unit );	
		MSG_SCMD_DUEL_INVITE_FAILED msg_outgoing;
		msg_outgoing.wFailReasonInviter = WORD(nIllegalReasonInviter);
		msg_outgoing.wFailReasonInvitee = WORD(nIllegalReasonInvitee);
		s_SendMessage(game, idClient, SCMD_DUEL_INVITE_FAILED, &msg_outgoing);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdDuelInviteDecline(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_DUEL_INVITE_DECLINE *msg = (MSG_CCMD_DUEL_INVITE_DECLINE *)data;
	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);
	UNIT *playerWhoInvited = UnitGetById(game, msg->idOpponent);
	if (playerWhoInvited == NULL)
		return;

	// TODO verify pending opponent stats and send decline message to inviter?
	UnitSetStat(playerWhoInvited, STATS_PVP_DUEL_OPPONENT_ID, INVALID_ID);

	ASSERTX_RETURN( UnitHasClient( playerWhoInvited ), "Player being invited to a duel no client" );
	GAMECLIENTID idClient = UnitGetClientId( playerWhoInvited );	
	MSG_SCMD_DUEL_INVITE_DECLINE msg_outgoing;
	msg_outgoing.idOpponent = UnitGetId(unit);
	s_SendMessage(game, idClient, SCMD_DUEL_INVITE_DECLINE, &msg_outgoing);
	
	UnitSetStat(unit, STATS_PVP_DUEL_INVITER_ID, INVALID_ID);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdDuelInviteFailed(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_DUEL_INVITE_FAILED *msg = (MSG_CCMD_DUEL_INVITE_FAILED *)data;
	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);
	UNIT *playerWhoInvited = UnitGetById(game, msg->idOpponent);
	if (playerWhoInvited == NULL)
		return;

	// TODO verify pending opponent stats and send decline message to inviter?
	UnitSetStat(playerWhoInvited, STATS_PVP_DUEL_OPPONENT_ID, INVALID_ID);

	ASSERTX_RETURN( UnitHasClient( playerWhoInvited ), "Player being invited to a duel no client" );
	GAMECLIENTID idClient = UnitGetClientId( playerWhoInvited );	
	MSG_SCMD_DUEL_INVITE_FAILED msg_outgoing;
	msg_outgoing.wFailReasonInviter = msg->wFailReasonInviter;
	msg_outgoing.wFailReasonInvitee = msg->wFailReasonInvitee;
	s_SendMessage(game, idClient, SCMD_DUEL_INVITE_FAILED, &msg_outgoing);

	UnitSetStat(unit, STATS_PVP_DUEL_INVITER_ID, INVALID_ID);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdDuelInviteAccept(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	s_DuelAccept(game, unit, (MSG_CCMD_DUEL_INVITE_ACCEPT *)data);
}

//----------------------------------------------------------------------------
// Only valid for closed server.
// All automated duels are ranked, so trial users can't use this.
//----------------------------------------------------------------------------
static void sCCmdDuelAutomatedSeek(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if ISVERSION(SERVER_VERSION)
	UNITLOG_ASSERT_RETURN(!ClientHasBadge(client, ACCT_TITLE_TRIAL_USER), unit);
	SendSeekAutoMatch(unit);
#endif
}

//----------------------------------------------------------------------------
// Since this is an app message, we allow it for all sorts of situations,
// including ghost and no unit.  However, we don't want to actually
// host the duel in these situations, so we check for them and post a failure
// to the Battle Server.
//
// Once we have a UI and a state for seeking, we may want to check whether
// they're still seeking (they may have cancelled and race conditioned) and
// fail if they are not.
//----------------------------------------------------------------------------
static void sAppCmdDuelAutomatedHost(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if ISVERSION(SERVER_VERSION)
	BOOL bRet = TRUE;
	MSG_APPCMD_DUEL_AUTOMATED_HOST *msg = (MSG_APPCMD_DUEL_AUTOMATED_HOST *)data;
	CBRA(game && client && unit && !UnitIsGhost(unit));
	//Call a function in duel.cpp that creates a game and sends the game info back.
	CBRA( s_DuelAutomatedHost(game, unit, msg) );
Error:
	if(!bRet)
	{//Send a message to the battle server informing him of failure.
		SendBattleCancel(msg->idBattle);
	}
#endif
}

//----------------------------------------------------------------------------
// Similar reasoning and checks as above function.
//----------------------------------------------------------------------------
static void sAppCmdDuelAutomatedGuest(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if ISVERSION(SERVER_VERSION)
	BOOL bRet = TRUE;
	MSG_APPCMD_DUEL_AUTOMATED_GUEST *msg = (MSG_APPCMD_DUEL_AUTOMATED_GUEST *)data;
	CBRA(game && client && unit && !UnitIsGhost(unit));
	//func in duel.cpp that just warps to the specified game and warp spec.
	CBRA( s_DuelAutomatedGuest(game, unit, msg) );
Error:
	if(!bRet)
	{
		SendBattleCancel(msg->idBattle);
	}
#endif
}

//----------------------------------------------------------------------------
// Currently just reports the new rating.  In future, may modify a permanent
// stat so the player can look at himself and go gee whiz.
//----------------------------------------------------------------------------
static void sCCmdDuelNewRating(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
	MSG_APPCMD_DUEL_NEW_RATING *msg = (MSG_APPCMD_DUEL_NEW_RATING *)data;
	ASSERT_RETURN(UnitGetGuid(unit) == msg->guidCharacter);

	int nNewRatingIntegerized = 
		int(100*(UnitGetExperienceLevel(unit) + msg->fNewPvpLevelDelta));

	//This is TMP!  In future, if we want to display their new rating,
	//we will send a message to the client which plugs into a UI or the
	//global strings.
	{
		WCHAR szNewRating[64];
		PStrPrintf(szNewRating, 64, L"New rating: %d", nNewRatingIntegerized);
		s_SendSysText(CHAT_TYPE_SERVER, FALSE, game, client, szNewRating);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdRespec(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(AppIsTugboat() ) //Skill respec should currently only be allowed via token in Hellgate.
	{
		SkillRespec( unit, SRF_CHARGE_CURRENCY_MASK | SRF_USE_RESPEC_STAT_MASK );
	}
	else 
	{
		TraceWarn(TRACE_FLAG_GAME_NET, "Sorry, skill respec is only via token in Hellgate.");
	}
#endif
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdRespecCrafting(
						GAME * game, 
						GAMECLIENT * client,
						UNIT * unit,
						BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(AppIsTugboat() ) //Skill respec should currently only be allowed via token in Hellgate.
	{
		SkillRespec( unit, SRF_CHARGE_CURRENCY_MASK | SRF_USE_RESPEC_STAT_MASK | SRF_CRAFTING_MASK );
	}
	else 
	{
		TraceWarn(TRACE_FLAG_GAME_NET, "Sorry, skill respec is only via token in Hellgate.");
	}
#endif
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sCmdStuckStopSkillRequest(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	int nStuckSkill = (int)event_data.m_Data1;
	if(nStuckSkill < 0)
	{
		return FALSE;
	}

	SkillStopRequest(unit, nStuckSkill);
	return TRUE;
}
#endif

static void sCCmdStuck(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pControlUnit = ClientGetControlUnit(client);
	if(!pControlUnit)
		return;

	int nStuckSkill = GlobalIndexGet(GI_SKILLS_STUCK);
	if(nStuckSkill < 0)
		return;

	if(SkillStartRequest(game, pControlUnit, nStuckSkill, INVALID_ID, VECTOR(0), 0, 0))
	{
		UnitRegisterEventTimed(pControlUnit, s_sCmdStuckStopSkillRequest, EVENT_DATA(nStuckSkill), 1);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sGetPlayerInfoForGuildMessage(
	GAMECLIENT * pClient,
	WCHAR * nameDest,
	ULONGLONG & guid,
	ULONGLONG & accountId )
{
	if (pClient == NULL || pClient->m_pPlayerUnit == NULL)
		return FALSE;

	ASSERT_RETFALSE(UnitGetName(pClient->m_pPlayerUnit, nameDest, MAX_CHARACTER_NAME, 0));

	guid = UnitGetGuid(pClient->m_pPlayerUnit);
	ASSERT_RETFALSE(guid != INVALID_GUID);

	accountId = NetClientGetUniqueAccountId(pClient->m_idNetClient);
	ASSERT_RETFALSE(accountId != INVALID_UNIQUE_ACCOUNT_ID);

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildCreate(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_GUILD_CREATE * msg = (MSG_CCMD_GUILD_CREATE*)data;

	UNIT * pGuildHerald = NULL;
	if( AppIsHellgate() )
	{
		// make sure msg.idGuildHerald belongs to player (unit) and is actually a guild herald and is not "in use"
		pGuildHerald = UnitGetById(game, msg->idGuildHerald);
		if (!pGuildHerald)
		{
			return;
		}
		UNITLOG_ASSERT_RETURN(UnitIsA(pGuildHerald, UNITTYPE_GUILD_HERALD), unit);
		if (!UnitIsContainedBy(pGuildHerald, unit))
		{
			return;
		}
		if (UnitGetStat(pGuildHerald, STATS_GUILD_HERALD_IN_USE))
		{
			return;
		}
	}

	if (!ValidateName(msg->wszGuildName, MAX_CHAT_CNL_NAME, NAME_TYPE_GUILD, TRUE))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CREATE_GUILD, CHAT_ERROR_INVALID_GUILD_NAME);
		return;
	}

	//	must be a subscriber
	BADGE_COLLECTION badges;
	if(!PlayerIsSubscriber(unit))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CREATE_GUILD, CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER);
		return;
	}

	if( AppIsTugboat())
	{
		cCurrency playerCurrency = UnitGetCurrency( unit );
		if( playerCurrency < PlayerGetGuildCreationCost(unit) )
		{
			s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CREATE_GUILD, CHAT_ERROR_INVALID_GUILD_NAME);
			return;
		}
	}

	//	TODO: do any item and/or account status validation here, use s_SendGuildActionResult to report errors to client

	if( AppIsHellgate() )
	{
		//  set guild herald "in use"
		UnitSetStat(pGuildHerald, STATS_GUILD_HERALD_IN_USE, 1);
	}

	CHAT_REQUEST_MSG_CREATE_GUILD createMessage;
	PStrCopy(createMessage.wszGuildName, MAX_CHAT_CNL_NAME, msg->wszGuildName, MAX_CHAT_CNL_NAME);
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
		client,
		createMessage.wszPlayerName,
		createMessage.idPlayerGuid,
		createMessage.idPlayerAccountId));

	ASSERT(GameServerToChatServerSendMessage(&createMessage, GAME_REQUEST_CREATE_GUILD));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildInvite(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_GUILD_INVITE * msg = (MSG_CCMD_GUILD_INVITE*)data;

	//	TODO: do any item and/or account status validation here, guild membership permissions should be checked on the chat server
	//		  use s_SendGuildActionResult to report errors to client

	//	must be a subscriber, possible to have a leader/officer/member who is not a subscriber, just take away his power to do anything except promote to leader

	if(!PlayerIsSubscriber(unit))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_INVITE_TO_GUILD, CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER);
		return;
	}

	CHAT_REQUEST_MSG_INVITE_TO_GUILD inviteMessage;
	PStrCopy(inviteMessage.wszTargetPlayerName, MAX_CHARACTER_NAME, msg->wszPlayerToInviteName, MAX_CHARACTER_NAME);
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
		client,
		inviteMessage.wszInvitingPlayerName,
		inviteMessage.idInvitingPlayerGuid,
		inviteMessage.idInvitingPlayerAccountId));

	ASSERT(GameServerToChatServerSendMessage(&inviteMessage, GAME_REQUEST_INVITE_TO_GUILD));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildDeclineInvite(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	CHAT_REQUEST_MSG_DECLINE_GUILD_INVITE declineMessage;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
		client,
		declineMessage.wszDecliningPlayerName,
		declineMessage.idDecliningPlayerGuid,
		declineMessage.idDecliningPlayerAccountId));

	ASSERT(GameServerToChatServerSendMessage(&declineMessage, GAME_REQUEST_DECLINE_GUILD_INVITE));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildAcceptInvite(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	CHAT_REQUEST_MSG_ACCEPT_GUILD_INVITE acceptMessage;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
		client,
		acceptMessage.wszAcceptingPlayerName,
		acceptMessage.idAcceptingPlayerGuid,
		acceptMessage.idAcceptingPlayerAccountId));

	ASSERT(GameServerToChatServerSendMessage(&acceptMessage, GAME_REQUEST_ACCEPT_GUILD_INVITE));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildLeave(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	CHAT_REQUEST_MSG_LEAVE_GUILD leaveMessage;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
		client,
		leaveMessage.wszLeavingPlayerName,
		leaveMessage.idLeavingPlayerGuid,
		leaveMessage.idLeavingPlayerAccountId));

	ASSERT(GameServerToChatServerSendMessage(&leaveMessage, GAME_REQUEST_LEAVE_GUILD));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildRemove(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	//	must be a subscriber, possible to have a leader/officer/member who is not a subscriber, just take away his power to do anything except promote to leader
	if(!PlayerIsSubscriber(unit))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_REMOVE_GUILD_MEMBER, CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER);
		return;
	}

	MSG_CCMD_GUILD_REMOVE * msg = (MSG_CCMD_GUILD_REMOVE*)data;
	CHAT_REQUEST_MSG_REMOVE_GUILD_MEMBER removeMessage;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
		client,
		removeMessage.wszBootingPlayerName,
		removeMessage.idBootingPlayerGuid,
		removeMessage.idBootingPlayerAccountId));
	PStrCopy(removeMessage.wszTargetPlayerName, msg->wszPlayerToRemoveName, MAX_CHARACTER_NAME);

	ASSERT(GameServerToChatServerSendMessage(&removeMessage, GAME_REQUEST_REMOVE_GUILD_MEMBER));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildChangeRank(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_GUILD_CHANGE_RANK * msg = (MSG_CCMD_GUILD_CHANGE_RANK*)data;

	//	must be a subscriber, possible to have a leader/officer/member who is not a subscriber, just take away his power to do anything except promote to leader
	if(msg->eTargetRank != GUILD_RANK_LEADER && !PlayerIsSubscriber(unit))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CHANGE_GUILD_MEMBER_RANK, CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER);
		return;
	}

	CHAT_REQUEST_MSG_CHANGE_GUILD_MEMBER_RANK rankMsg;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
			client,
			rankMsg.wszPromotingPlayerName,
			rankMsg.idPromotingPlayerGuid,
			rankMsg.idPromotingPlayerAccountId));
	PStrCopy(rankMsg.wszTargetPlayerName, msg->wszTargetName, MAX_CHARACTER_NAME);
	rankMsg.eNewGuildRank = msg->eTargetRank;

	ASSERT(GameServerToChatServerSendMessage(&rankMsg, GAME_REQUEST_CHANGE_GUILD_MEMBER_RANK));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildChangeRankName(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_GUILD_CHANGE_RANK_NAME * msg = (MSG_CCMD_GUILD_CHANGE_RANK_NAME*)data;

	//	must be a subscriber, possible to have a leader/officer/member who is not a subscriber, just take away his power to do anything except promote to leader
	if(!PlayerIsSubscriber(unit))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CHANGE_RANK_NAME, CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER);
		return;
	}

	if (!ValidateName(msg->wszRankName, MAX_CHARACTER_NAME, NAME_TYPE_GUILD, FALSE))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CHANGE_RANK_NAME, CHAT_ERROR_INVALID_GUILD_RANK_NAME);
		return;
	}

	CHAT_REQUEST_MSG_CHANGE_RANK_NAME changeMsg;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
			client,
			changeMsg.wszLeaderPlayerName,
			changeMsg.idLeaderPlayerGuid,
			changeMsg.idLeaderPlayerAccountId));
	changeMsg.eRank = msg->eRank;
	PStrCopy(changeMsg.wszNewRankName, msg->wszRankName, MAX_CHARACTER_NAME);

	ASSERT(GameServerToChatServerSendMessage(&changeMsg, GAME_REQUEST_CHANGE_RANK_NAME));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdGuildChangeActionPerms(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_GUILD_CHANGE_ACTION_PERMISSIONS * msg = (MSG_CCMD_GUILD_CHANGE_ACTION_PERMISSIONS*)data;

	//	must be a subscriber, possible to have a leader/officer/member who is not a subscriber, just take away his power to do anything except promote to leader
	if(!PlayerIsSubscriber(unit))
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CHANGE_ACTION_PERMISSIONS, CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER);
		return;
	}

	if (msg->eMinimumGuildRank == GUILD_RANK_RECRUIT)
	{
		s_SendGuildActionResult(client->m_idNetClient, GAME_REQUEST_CHANGE_ACTION_PERMISSIONS, CHAT_ERROR_INVALID_PERMISSION_LEVEL);
		return;
	}

	CHAT_REQUEST_MSG_CHANGE_ACTION_PERMISSIONS changeMsg;
	ASSERT_RETURN(
		sGetPlayerInfoForGuildMessage(
			client,
			changeMsg.wszLeaderPlayerName,
			changeMsg.idLeaderPlayerGuid,
			changeMsg.idLeaderPlayerAccountId));
	changeMsg.eActionType = msg->eGuildActionType;
	changeMsg.eMinimumRank = msg->eMinimumGuildRank;

	ASSERT(GameServerToChatServerSendMessage(&changeMsg, GAME_REQUEST_CHANGE_ACTION_PERMISSIONS));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdPlayerGuildData(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_PLAYER_GUILD_DATA_GAME * infoMsg = (MSG_APPCMD_PLAYER_GUILD_DATA_GAME*)data;

	s_ClientSetGuildAssociation(client, infoMsg->wszGuildName, infoMsg->eGuildRank, infoMsg->wszRankName);
	s_PlayerBroadcastGuildAssociation( game, client, unit );

	UNIT * pPlayer = UnitGetByGuid(game, infoMsg->ullPlayerGuid);
	ASSERT_RETURN(pPlayer);

	WCHAR szGuildName[MAX_GUILD_NAME];
	BYTE eGuildRank = (BYTE)GUILD_RANK_INVALID;
	WCHAR szGuildRankName[MAX_CHARACTER_NAME];
	s_ClientGetGuildAssociation(UnitGetClient(pPlayer), szGuildName, MAX_GUILD_NAME, eGuildRank, szGuildRankName, MAX_CHARACTER_NAME);
	const BOOL bInGuild = (szGuildName && szGuildName[0]);

	PlayerCleanUpGuildHeralds(game, pPlayer, bInGuild);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdGuildActionResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_PLAYER_GUILD_ACTION_RESULT_GAME * resultMsg = (MSG_APPCMD_PLAYER_GUILD_ACTION_RESULT_GAME*)data;

	if (resultMsg->wPlayerRequest == GAME_REQUEST_CREATE_GUILD)
	{
		// make sure msg.idGuildHerald belongs to player (unit) and is actually a guild herald and is not "in use"
		if( AppIsHellgate() )
		{
			UNIT * pPlayer = UnitGetByGuid(game, resultMsg->ullPlayerGuid);
			ASSERT_RETURN(pPlayer);
			UNIT * pGuildHerald = FindInUseGuildHerald(pPlayer);
			ASSERT_RETURN(pGuildHerald);
			ASSERT_RETURN(UnitIsA(pGuildHerald, UNITTYPE_GUILD_HERALD));
			ASSERT_RETURN(UnitIsContainedBy(pGuildHerald, pPlayer));
			ASSERT_RETURN(UnitGetStat(pGuildHerald, STATS_GUILD_HERALD_IN_USE));

			UnitClearStat(pGuildHerald, STATS_GUILD_HERALD_IN_USE, 0);

			if (resultMsg->wActionResult == CHAT_ERROR_NONE)
			{
				//	TODO: handle successful guild creation here, player has already been notified
				sItemDecrementOrUse(game, pGuildHerald, FALSE);
			}
			else
			{
				//	TODO: handle failed guild creation here, player has already been notified
			}
		}
		else if (resultMsg->wActionResult == CHAT_ERROR_NONE)
		{
			//Take money
			cCurrency cost = PlayerGetGuildCreationCost(unit);		
			UnitRemoveCurrencyValidated( unit, cost );

		}
	}

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdPlayerPartyChanged(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_PARTY_MEMBER_INFO_GAME * msg = (MSG_APPCMD_PARTY_MEMBER_INFO_GAME*)data;

	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	if (!serverContext)
		return;

	PARTYID idParty = msg->IdNewPartyChannel;  // id will be INVALID_CHANNELID if leaving a party
	PGUID guidMember = msg->IdPlayerCharacterGuid;
	PARTY_LEAVE_REASON eLeaveReason = (PARTY_LEAVE_REASON)msg->LeaveReason;
	
	APPCLIENTID idAppClient = ClientSystemFindByPguid( serverContext->m_ClientSystem, guidMember );

	PARTYID idOldParty = 
		ClientSystemGetParty( serverContext->m_ClientSystem, idAppClient );

	ASSERT(
		ClientSystemSetParty(
			serverContext->m_ClientSystem,
			idAppClient,
			idParty) );
			
	// give game server opportunity to respond to the party add/remove
	UNIT *pUnitMember = UnitGetByGuid( game, guidMember );
	if (pUnitMember)
	{
		if (idParty == INVALID_ID)
		{
			s_PlayerLeftParty( pUnitMember, idParty, eLeaveReason );
		}
		else if (idParty != idOldParty)
		{
			s_PlayerJoinedParty( pUnitMember, idParty );
		}
	}

	// any time the party makeup changes, we have to validate our aggression lists across the board
	if( AppIsTugboat() )
	{
		// check all the clients in game
		for (GAMECLIENT *pClient = ClientGetFirstInGame( game );
			pClient;
			pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pPlayer = ClientGetControlUnit( pClient );
			if (pPlayer)
			{
				// check all the clients in game
				for (GAMECLIENT *pClient2 = ClientGetFirstInGame( game );
					pClient2;
					pClient2 = ClientGetNextInGame( pClient2 ))
				{
					UNIT *pPlayer2 = ClientGetControlUnit( pClient2 );
					if (pPlayer2 && pPlayer2 != pPlayer &&
						UnitGetPartyId( pPlayer ) != INVALID_ID &&
						UnitGetPartyId( pPlayer ) == UnitGetPartyId( pPlayer2 ) )
					{
						RemoveFromAggressionList( pPlayer, pPlayer2 );
					}

				}
			}

		}
	}

	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdDeleteItem(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pPlayer,
	BYTE *pData)
{
	MSG_APPCMD_DELETE_ITEM *pMessage = (MSG_APPCMD_DELETE_ITEM *)pData;
	UNIT *pItem = UnitGetByGuid( pGame, pMessage->guidItemToDelete );
	if (pItem && UnitGetUltimateContainer( pItem ) == pPlayer)
	{
	
		// disable database operatoins on this unit if requested
		if (pMessage->bInformDatabase == FALSE)
		{
			s_DatabaseUnitEnable( pItem, FALSE );
		}
		
		// free the item
		UnitFree( pItem, UFF_SEND_TO_CLIENTS );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdWarpToPlayerData(
	GAME* pGame,
	GAMECLIENT* pClient,
	UNIT* pUnit,
	BYTE* pData )
{
#if ISVERSION(SERVER_VERSION)

	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pClient);

	MSG_APPCMD_WARP_TO_PLAYER_RESULT * msg = (MSG_APPCMD_WARP_TO_PLAYER_RESULT*)pData;

	if (msg->bFoundTargetMember)
	{
		if (PlayerCanWarpTo(pUnit, msg->TargetMemberGameData.nLevelDef))
		{
			WARP_SPEC tSpec;
			tSpec.nLevelDefDest = msg->TargetMemberGameData.nLevelDef;
			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_GUID_BIT );
			tSpec.guidArrivePortalOwner = msg->TargetMemberGuid;
			tSpec.idGameOverride = msg->TargetMemberGameData.IdGame;

			s_WarpToLevelOrInstance(pUnit, tSpec);
		}
		else
		{
			// error: warp to player not allowed
			MSG_SCMD_WARP_RESTRICTED msg;
			msg.nReason = WRR_TARGET_PLAYER_NOT_ALLOWED;
			SendMessageToClient(pGame, pClient, SCMD_WARP_RESTRICTED, &msg);
		}
	}
	else
	{
		// error: warp to player not found
		MSG_SCMD_WARP_RESTRICTED msg;
		msg.nReason = WRR_TARGET_PLAYER_NOT_FOUND;
		SendMessageToClient(pGame, pClient, SCMD_WARP_RESTRICTED, &msg);
	}

#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdPlayed(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNREFERENCED_PARAMETER(data);
	MSG_SCMD_PLAYED msg;
	msg.dwPlayedTime = UnitGetPlayedTime( unit );
	SendMessageToClient(game, client, SCMD_PLAYED, &msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailSendStart(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_SEND_START * msg = (MSG_CCMD_EMAIL_SEND_START*)data;
	if (!s_ClientIsSendMailMessageAllowed(client))
		return;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailCreateNewEmail(game, client, unit, msg->wszEmailSubject, msg->wszEmailBody);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailAddByName(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_ADD_RECIPIENT_BY_CHARACTER_NAME * msg = (MSG_CCMD_EMAIL_ADD_RECIPIENT_BY_CHARACTER_NAME*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailAddEmailRecipientByCharacterName(unit, msg->wszTargetCharacterName);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailAddByGuild(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_ADD_GUILD_MEMBERS_AS_RECIPIENTS * msg = (MSG_CCMD_EMAIL_ADD_GUILD_MEMBERS_AS_RECIPIENTS*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailAddGuildMembersAsRecipients(unit);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailSetAttachments(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_SET_ATTACHMENTS * msg = (MSG_CCMD_EMAIL_SET_ATTACHMENTS*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailAddEmailAttachments(unit, msg->idAttachedItemId, msg->dwAttachedMoney);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailSendCommit(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailSend(game, client, unit);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailCreationResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_CREATION_RESULT * msg = (MSG_APPCMD_EMAIL_CREATION_RESULT*)data;
#if ISVERSION(SERVER_VERSION)
	vector_mp<wstring_mp>::type ignoringPlayers(NULL);
	if (msg->wszIgnoringTarget[0])
	{
		ignoringPlayers.push_back(msg->wszIgnoringTarget);
	}
	s_PlayerEmailHandleCreationResult(
		game, 
		client, 
		unit, 
		msg->idEmailMessageId,
		msg->dwfEmailCreationResult, 
		(EMAIL_SPEC_SOURCE)msg->eEmailSpecSource,
		msg->dwUserContext,
		ignoringPlayers);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailCreationResultUtilGame(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_CREATION_RESULT_UTIL_GAME * msg = (MSG_APPCMD_EMAIL_CREATION_RESULT_UTIL_GAME *)data;
#if ISVERSION(SERVER_VERSION)
	sAppCmdEmailCreationResult( game, client, unit, (BYTE *)&msg->tResult );
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailItemTransferResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_ITEM_TRANSFER_RESULT * msg = (MSG_APPCMD_EMAIL_ITEM_TRANSFER_RESULT*)data;
#if ISVERSION(SERVER_VERSION)
	s_SystemEmailHandleItemMoved(
		game, 
		(EMAIL_SPEC_SOURCE)msg->eEmailSpecSource, 
		client, 
		unit, 
		msg->ullRequestCode, 
		msg->idItemGuid, 
		msg->bTransferSuccess, 
		msg->idEmailMessageId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailItemTransferResultUtilGame(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_ITEM_TRANSFER_RESULT * msg = (MSG_APPCMD_EMAIL_ITEM_TRANSFER_RESULT*)data;
#if ISVERSION(SERVER_VERSION)

	EMAIL_SPEC_SOURCE eSource = (EMAIL_SPEC_SOURCE)msg->eEmailSpecSource;
	switch (eSource)
	{
	
		//----------------------------------------------------------------------------	
		case ESS_CSR:
			s_SystemEmailHandleItemMoved(
				game, 
				eSource, 
				NULL,
				NULL,
				msg->ullRequestCode, 
				msg->idItemGuid, 
				msg->bTransferSuccess, 
				msg->idEmailMessageId);
			break;
		
		//----------------------------------------------------------------------------
		default:
			s_AuctionOwnerEmailHandleItemMoved(
				game, 
				msg->ullRequestCode, 
				msg->idItemGuid, 
				msg->bTransferSuccess, 
				msg->idEmailMessageId);
			break;
			
	}
	
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailMoneyDeductionResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_MONEY_DEDUCTION_RESULT * msg = (MSG_APPCMD_EMAIL_MONEY_DEDUCTION_RESULT*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleMoneyDeducted(game, client, unit, msg->bTransferSuccess);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailDeliveryResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_DELIVERY_RESULT * msg = (MSG_APPCMD_EMAIL_DELIVERY_RESULT*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleDeliveryResult(
		game, 
		client, 
		unit, 
		msg->idEmailMessageId,
		(EMAIL_SPEC_SOURCE)msg->eEmailSpecSource,
		msg->bEmailDeliveryResult);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailDeliveryResultUtilGame(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	const MSG_APPCMD_EMAIL_DELIVERY_RESULT_UTIL_GAME * msg = (const MSG_APPCMD_EMAIL_DELIVERY_RESULT_UTIL_GAME *)data;
#if ISVERSION(SERVER_VERSION)
	sAppCmdEmailDeliveryResult(game, client, unit, (BYTE *)&msg->tResult);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailNotification(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_NOTIFICATION * msg = (MSG_APPCMD_EMAIL_NOTIFICATION*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailRetrieveMessages(game, client, unit, msg->idEmailMessageId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailMetadata(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_METADATA * msg = (MSG_APPCMD_EMAIL_METADATA*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleEmailMetadata(game, client, unit, msg);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailData(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_ACCEPTED_EMAIL_DATA * msg = (MSG_APPCMD_ACCEPTED_EMAIL_DATA*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleEmailData(game, client, unit, msg);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailDataUpdate(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_UPDATE * msg = (MSG_APPCMD_EMAIL_UPDATE*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleEmailDataUpdate(game, client, unit, msg);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailRestoreOutboxItemLoc(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNREFERENCED_PARAMETER(data);
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailRestoreOutboxItemLocation(game, client, unit);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailMarkMessageRead(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_MARK_MESSAGE_READ * msg = (MSG_CCMD_EMAIL_MARK_MESSAGE_READ*)data;
	if (!s_ClientIsGeneralEmailActionAllowed(client))
		return;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailMarkMessageAsRead(game, client, unit, msg->idEmailMessageId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailRemoveAttachedMoney(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_REMOVE_ATTACHED_MONEY * msg = (MSG_CCMD_EMAIL_REMOVE_ATTACHED_MONEY*)data;
	if (!s_ClientIsGeneralEmailActionAllowed(client))
		return;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailRemoveAttachedMoney(game, client, unit, msg->idEmailMessageId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailClearAttachedItem(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_CLEAR_ATTACHED_ITEM * msg = (MSG_CCMD_EMAIL_CLEAR_ATTACHED_ITEM*)data;
	if (!s_ClientIsGeneralEmailActionAllowed(client))
		return;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailClearAttachedItem(game, client, unit, msg->idEmailMessageId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdEmailDeleteMessage(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_EMAIL_DELETE_MESSAGE * msg = (MSG_CCMD_EMAIL_DELETE_MESSAGE*)data;
	if (!s_ClientIsGeneralEmailActionAllowed(client))
		return;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailDeleteMessage(game, client, unit, msg->idEmailMessageId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailAttachedMoneyInfo(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_ATTACHED_MONEY_INFO * msg = (MSG_APPCMD_EMAIL_ATTACHED_MONEY_INFO*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleAttachedMoneyInfoForRemoval(game, client, unit, msg->idEmailMessageId, msg->dwMoneyAmmount, msg->idMoneyUnitId);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdEmailAttachedMoneyResult(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_APPCMD_EMAIL_MONEY_REMOVAL_RESULT * msg = (MSG_APPCMD_EMAIL_MONEY_REMOVAL_RESULT*)data;
#if ISVERSION(SERVER_VERSION)
	s_PlayerEmailHandleAttachedMoneyRemovalResult(game, client, unit, msg->idEmailMessageId, msg->dwMoneyAmmount, msg->bSuccess);
#else
	UNREFERENCED_PARAMETER(msg);
#endif
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdSelectTitle(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_SELECT_PLAYER_TITLE * pMsg = (MSG_CCMD_SELECT_PLAYER_TITLE *)data;

	s_PlayerSetTitle(unit, pMsg->nTitleString);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdDonateMoney(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_DONATE_MONEY * msg = (MSG_CCMD_DONATE_MONEY *)data;

	cCurrency toDonate(msg->nMoney, msg->nRealWorldMoney);
	UNITLOG_ASSERT_RETURN( UnitRemoveCurrencyValidated(unit, toDonate), unit); //This should check negativity, etc.

	int nEventType = s_GlobalGameEventGetRandomEventType(game);
	ASSERT(s_GlobalGameEventSendDonate(unit, toDonate, nEventType) );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdItemChatMessage(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
#if ISVERSION(SERVER_VERSION)

	if (s_GagIsGagged(unit))
		return;

	MSG_CCMD_ITEM_MESSAGE * msg = (MSG_CCMD_ITEM_MESSAGE*)data;

	BYTE hypertextData[2048];	//	some size... stand-in for now
	DWORD hypertextDataLength = sizeof(hypertextData);

	// This checks to make sure the incoming data is valid and reprocesses it in order to pack it into the outgoing buffer.
	if (!s_ValididateReprocessAndForwardHypertext(game, client, msg->wszMessageText, msg->HypertextData, MSG_GET_BLOB_LEN(msg, HypertextData), hypertextData, hypertextDataLength))
		return;

	s_ChatNetSendHypertextChatMessage(
		client,
		unit,
		(ITEM_CHAT_TYPE)msg->eItemChatType,
		msg->wszTargetName,
		msg->targetChannelId,
		msg->wszMessageText,
		hypertextData,
		hypertextDataLength );

#endif
#endif
}

//----------------------------------------------------------------------------
// TODO:
// 1) Move this functionality, at least for CTF, into ctf.cpp
// 2) Set up tugboat version.
// 3) Generalize to other gametypes or more specific options we want.
//
// For KCK:
// Figure out how anyone else is getting to this game.
// Via the town instance list with a different frontend?
//----------------------------------------------------------------------------
static void sCCmdCreatePvpGame(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_CREATE_PVP_GAME * msg = (MSG_CCMD_CREATE_PVP_GAME*)data;
	GAME_SUBTYPE eGameSubType = (GAME_SUBTYPE)msg->ePvPGameType;
	ASSERT_RETURN(eGameSubType == GAME_SUBTYPE_PVP_CTF);

	//create the game
	GAME_SETUP tGameSetup;
	tGameSetup.nNumTeams = 2;
	GameVariantInit( tGameSetup.tGameVariant, unit );
	DifficultyValidateValue(tGameSetup.tGameVariant.nDifficultyCurrent, unit);
	GAMEID idGame = SrvNewGame(eGameSubType, &tGameSetup, TRUE, tGameSetup.tGameVariant.nDifficultyCurrent);

	ASSERTX_RETURN(idGame != INVALID_ID, "failed to create custom game for create pvp game request");

	//Note that we only have the GAMEID here.  If we want special things,
	// we need to post messages to its mailbox.  Alternately we can mess
	// with the unit or its warp spec, but this is somewhat hacky unless
	// the things are unit-specific, such as its team.
	
	//Set up a warp spec.  Warp them to the CTF level.
	WARP_SPEC tWarpSpec;

	if(AppIsHellgate() )
	{
		tWarpSpec.nLevelDefDest = GlobalIndexGet( GI_LEVEL_ABYSS_CTF );
	}
	else
	{
		ASSERT_MSG("CTF level area not defined for Mythos");
	}
	tWarpSpec.idGameOverride = idGame;

	SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_FARTHEST_PVP_SPAWN_BIT );
	ASSERT(s_WarpToLevelOrInstance( unit, tWarpSpec ) );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdJoinPvpGames(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	BYTE * data )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	MSG_CCMD_JOIN_PVP_GAME *msg = (MSG_CCMD_JOIN_PVP_GAME *)data;

	int nAreaID = UnitGetLevelDefinitionIndex( unit );
	if( nAreaID != INVALID_ID &&
		!IsUnitDeadOrDying( unit ) &&
		GameGetId( game ) != msg->idGameToWarpTo )
	{
		// KCK TODO: Check max number of players and any other factors which might limit our ability to join
		// Send error messages if join failed
		if (1)
		{
			WARP_SPEC tSpec;
			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
			tSpec.nLevelDepthDest = 0;	
			tSpec.nLevelAreaDest = nAreaID;
			tSpec.nLevelAreaPrev = nAreaID;
			tSpec.nPVPWorld = PlayerIsInPVPWorld( unit );
			tSpec.idGameOverride = msg->idGameToWarpTo;

			s_WarpToLevelOrInstance(unit, tSpec);
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void FP_REMOVE_FROM_PARTIES(
	 UNIT *pUnit,
	 void *pCallbackData )
{
	GAME * pGame = (GAME*)pCallbackData;
	ASSERT_RETURN(pUnit && pGame);

	if (UnitGetPartyId(pUnit) != INVALID_CHANNELID)
	{
		MSG_APPCMD_PARTY_MEMBER_INFO_GAME fakeMsg;
		fakeMsg.IdGame = GameGetId(pGame);
		fakeMsg.ullPlayerAccountId = UnitGetAccountId(pUnit);
		fakeMsg.IdPlayerCharacterGuid = UnitGetGuid(pUnit);
		fakeMsg.IdNewPartyChannel = INVALID_CHANNELID;
		fakeMsg.LeaveReason = PARTY_MEMBER_PARTY_DISBANDED;

		sAppCmdPlayerPartyChanged(pGame, ClientGetById(pGame, UnitGetClientId(pUnit)), pUnit, (BYTE*)&fakeMsg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdRecoverFromChatCrash(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* /*pData*/ )
{
	ASSERT_RETURN(pGame);

	LEVEL * lvl = DungeonGetFirstLevel(pGame);
	while (lvl)
	{
		s_LevelClearCommunicationChannel(pGame, lvl);
		if (s_LevelGetPlayerCount(lvl) > 0)
			s_ChatNetRequestChatChannel(lvl);

		LevelIteratePlayers(lvl, FP_REMOVE_FROM_PARTIES, pGame);

		GAME_INSTANCE_TYPE	eInstanceType = GAME_INSTANCE_NONE;
		if (LevelIsTown(lvl))
			eInstanceType = GAME_INSTANCE_TOWN;
		else if (GameIsPVP(pGame))
			eInstanceType = GAME_INSTANCE_PVP;

		if (eInstanceType != GAME_INSTANCE_NONE)
		{
			if( AppIsHellgate() )
			{
				s_ChatNetRegisterNewInstance(GameGetId(pGame), LevelGetDefinitionIndex(lvl), (int)eInstanceType, 0);
			}
			else
			{
				s_ChatNetRegisterNewInstance(GameGetId(pGame), LevelGetLevelAreaID(lvl), (int)eInstanceType, LevelGetPVPWorld(lvl) ? 1 : 0 );
			}
		}

		lvl = DungeonGetNextLevel(lvl);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdDonationEventDoState(
	UNIT* pUnit,
	void* pfnCallbackData
)
{
	int nRow = *((int*) pfnCallbackData);
	DONATION_REWARDS_DATA *pDonation = (DONATION_REWARDS_DATA*)ExcelGetData(NULL, DATATABLE_DONATION_REWARDS, nRow);
	ASSERT_RETURN(pDonation);

	s_StateSet(pUnit, NULL, pDonation->m_nState, (pDonation->m_nTicks != INVALID_ID ? pDonation->m_nTicks:36000));
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdDonationEvent(
	GAME* game,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* data )
{
	MSG_APPCMD_DONATION_EVENT * msg = (MSG_APPCMD_DONATION_EVENT *)data;

	REF(game);
	REF(msg);

	//We would put the picker chooser here if we wanted every existing game on the server
	//to have a different effect (for extra added "WTF" factor) - bmanegold

	//int nState = STATE_DONATION_EXPERIENCE_BONUS;
	PlayersIterate(game, sAppCmdDonationEventDoState, &(msg->nDonationEventType));
	//s_StartDonationEvent(game, msg->nDonationEventType);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdUtilGameAddAHClient(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* /*pData*/ )
{
	ASSERT_RETURN(pGame != NULL);

#if ISVERSION(SERVER_VERSION)
/*	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx != NULL);

	APPCLIENTID idAppClient = GameServerGetAuctionHouseAppClientID(pSvrCtx);
	ASSERT_RETURN(idAppClient != INVALID_CLIENTID);

	GAMECLIENT* pClient = ClientAddToGame(pGame, idAppClient);
	ASSERT_RETURN(pClient != NULL);

	PlayerLoadFromDatabase(idAppClient, AUCTION_OWNER_NAME, GameServerGetAuctionHouseAccountID(pSvrCtx));
/*
	CLIENT_SAVE_FILE tClientSaveFile;
	PStrCopy(tClientSaveFile.uszCharName, AUCTION_OWNER_NAME, MAX_CHARACTER_NAME);
	tClientSaveFile.pBuffer = NULL;
	tClientSaveFile.dwBufferSize = 0;
	tClientSaveFile.eFormat = PSF_HIERARCHY;

	UNIT* pUnit = PlayerLoad(pGame, &tClientSaveFile, 0, ClientGetId(pClient));
	ASSERT_RETURN(pUnit);

	if (!s_PlayerAdd(pGame, pClient, pUnit))
	{
		UnitFree(pUnit, 0);
		ClientSetControlUnit(pGame, pClient, NULL);
		return;
	}
	*/
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdGenerateRandomItem(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* /*pData*/ )
{
	UNREFERENCED_PARAMETER(pGame);
#if ISVERSION(SERVER_VERSION)
	UNIT* pItem = NULL;
	BOOL bResult = FALSE;

	ASSERT_GOTO(pGame != NULL, _err);
	ASSERT_GOTO(GameIsUtilityGame(pGame), _err);

	// Only  try once
//	while (TRUE) {
	ONCE {
		pItem = s_TreasureSpawnSingleItemSimple(pGame, GlobalIndexGet(GI_TREASURE_BOSS), RandGetNum(pGame, 50));
		if (pItem != NULL) {
			if (UnitIsA(pItem, UNITTYPE_MOD) ||
				UnitIsA(pItem, UNITTYPE_WEAPON) ||
				UnitIsA(pItem, UNITTYPE_ARMOR)) {
				break;
			} else {
				UnitFree(pItem);
				pItem = NULL;
			}
		}
	}

	if (pItem == NULL) {
		goto _err;
	}

	{
		int nQuality = ItemGetQuality(pItem);
		const ITEM_QUALITY_DATA* pQualityData = ItemQualityGetData(pGame, nQuality);
		ASSERT_GOTO(pQualityData != NULL, _err);

		SIMPLE_DYNAMIC_ARRAY<int> nEquivTypes;
		ArrayInit(nEquivTypes, GameGetMemPool(pGame), 8);
		ASSERT_GOTO(UnitTypeGetAllEquivTypes(UnitGetType(pItem), nEquivTypes), _err);

		UINT32 i = 0;
		GAME_AUCTION_RESPONSE_ITEM_INFO_MSG msg;
		msg.GameID = GameGetId(pGame);
		msg.itemInfoData.ItemGuid = UnitGetGuid(pItem);
		msg.itemInfoData.SellerGuid = INVALID_GUID;
		msg.itemInfoData.ItemQuality = pQualityData->nQualityLevel;
		msg.itemInfoData.ItemLevel = UnitGetExperienceLevel(pItem);
		msg.itemInfoData.ItemType = UnitGetType(pItem);
		msg.itemInfoData.ItemClass = UnitGetClass(pItem);
		msg.itemInfoData.ItemPrice = EconomyItemSellPrice(pItem).GetValue(KCURRENCY_VALUE_INGAME);
		msg.itemInfoData.ItemVariant = 0;

#if ISVERSION(DEBUG_VERSION)
		const UNITTYPE_DATA* pUnitTypeData = UnitTypeGetData(UnitGetType(pItem));
		if (pUnitTypeData != NULL) {
			PStrCopy(msg.itemInfoData.ItemName, pUnitTypeData->szName, MAX_CHARACTER_NAME);
		} else {
			msg.itemInfoData.ItemName[0] = '\0';
		}
#endif

		for (i = 0; i < nEquivTypes.Count() && i < MAX_ITEM_EQUIV_TYPES; i++) {
			msg.itemInfoData.ItemEquivTypes[i] = nEquivTypes[i];
		}
		for (; i < MAX_ITEM_EQUIV_TYPES; i++) {
			msg.itemInfoData.ItemEquivTypes[i] = INVALID_AUCTION_ITEM_TYPE;
		}

		ASSERT_GOTO(GameServerToAuctionServerSendMessage(&msg, GAME_AUCTION_RESPONSE_ITEM_INFO), _err);
	}

	bResult = TRUE;

_err:
	if (!bResult) {
		GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED_MSG msgFail;
		GameServerToAuctionServerSendMessage(&msgFail, GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED);

		if (pItem != NULL) {
			UnitFree(pItem);
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdGenerateRandomItemGetBlob(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_GENERATE_RANDOM_ITEM_GETBLOB* pMsg = (MSG_APPCMD_GENERATE_RANDOM_ITEM_GETBLOB*)pData;
	ASSERT_RETURN(pMsg != NULL);

	DATABASE_UNIT unitDB;
	BYTE pDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
	DWORD dwDataBufferSize = arrsize(pDataBuffer);
	int nValidationErrors = 0;
	BOOL bResult = FALSE;

	UNIT* pItem = UnitGetByGuid(pGame, pMsg->ItemGUID);
	ASSERT_GOTO(pItem != NULL, _err);

	dwDataBufferSize = PlayerSaveToBuffer(pGame, NULL, pItem, UNITSAVEMODE_CLIENT_ONLY, pDataBuffer, UNITFILE_MAX_SIZE_SINGLE_UNIT);
	ASSERT_GOTO(dwDataBufferSize > 0, _err);

	{
		GAME_AUCTION_RESPONSE_ITEM_BLOB_MSG msg;
		msg.ItemGuid = UnitGetGuid(pItem);
		MemoryCopy(msg.ItemBlob, DEFAULT_MAX_ITEM_BLOB_MSG_SIZE,
			pDataBuffer, dwDataBufferSize);
		MSG_SET_BLOB_LEN(msg, ItemBlob, dwDataBufferSize);

		ASSERT_GOTO(GameServerToAuctionServerSendMessage(&msg, GAME_AUCTION_RESPONSE_ITEM_BLOB), _err);
	}

	bResult = TRUE;
_err:
	if (!bResult) {
		GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED_MSG msgFail;
		GameServerToAuctionServerSendMessage(&msgFail, GAME_AUCTION_RESPONSE_ITEM_GEN_FAILED);
	}

	if (pItem != NULL) {
		UnitFree(pItem);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHError(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_AH_ERROR* pMsg = (MSG_APPCMD_AH_ERROR*)pData;
	ASSERT_RETURN(pMsg != NULL);

	UNIT* pUnit = UnitGetByGuid(pGame, pMsg->PlayerGUID);
	if (pUnit) {
		GAMECLIENTID idGameClient = UnitGetClientId(pUnit);
		if (idGameClient != INVALID_GAMECLIENTID) {
			MSG_SCMD_AH_ERROR msgError;
			msgError.ErrorCode = pMsg->ErrorCode;
			msgError.ItemGUID = pMsg->ItemGUID;
			s_SendMessage(pGame, idGameClient, SCMD_AH_ERROR, &msgError);
		}
/*
		if (pMsg->ErrorCode == AH_ERROR_SELL_ITEM_SUCCESS) {
			GAME_AUCTION_REQUEST_WITHDRAW_ITEM_MSG msgWithdraw;
			msgWithdraw.PlayerGUID = pMsg->PlayerGUID;
			msgWithdraw.GameID = GameGetId(pGame);
			msgWithdraw.ItemGUID = pMsg->ItemGUID;
			UnitGetName(pUnit, msgWithdraw.PlayerName, MAX_CHARACTER_NAME, 0);
		
			GameServerToAuctionServerSendMessage(&msgWithdraw, GAME_AUCTION_REQUEST_WITHDRAW_ITEM);
		}
*/
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHPlayerItemSaleList(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_AH_PLAYER_ITEM_SALE_LIST* pMsg = (MSG_APPCMD_AH_PLAYER_ITEM_SALE_LIST*)pData;
	ASSERT_RETURN(pMsg != NULL);

	UNIT* pUnit = UnitGetByGuid(pGame, pMsg->PlayerGUID);
	if (pUnit) {
		GAMECLIENTID idGameClient = UnitGetClientId(pUnit);
		if (idGameClient != INVALID_GAMECLIENTID) {
			MSG_SCMD_AH_PLAYER_ITEM_SALE_LIST msgSaleList;
			msgSaleList.ItemCount = pMsg->ItemCount;
			for (UINT32 i = 0; i < pMsg->ItemCount && i < AUCTION_MAX_ITEM_SALE_COUNT; i++) {
				msgSaleList.ItemGUIDs[i] = pMsg->ItemGUIDs[i];
			}

			s_SendMessage(pGame, idGameClient, SCMD_AH_PLAYER_ITEM_SALE_LIST, &msgSaleList);
		}
	}

#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHSearchResponse(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT* pMsg = (MSG_APPCMD_AH_SEARCH_RESULT*)pData;
	ASSERT_RETURN(pMsg != NULL);

	UNIT* pUnit = UnitGetByGuid(pGame, pMsg->PlayerGUID);
	if (pUnit) {
		GAMECLIENTID idGameClient = UnitGetClientId(pUnit);
		if (idGameClient != INVALID_GAMECLIENTID) {
			MSG_SCMD_AH_SEARCH_RESULT msgResult;
			msgResult.ResultSize = pMsg->ResultSize;
			s_SendMessage(pGame, idGameClient, SCMD_AH_SEARCH_RESULT, &msgResult);
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHSearchResponseNext(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT_NEXT* pMsg = (MSG_APPCMD_AH_SEARCH_RESULT_NEXT*)pData;
	ASSERT_RETURN(pMsg != NULL);

	UNIT* pUnit = UnitGetByGuid(pGame, pMsg->PlayerGUID);
	if (pUnit) {
		GAMECLIENTID idGameClient = UnitGetClientId(pUnit);
		if (idGameClient != INVALID_GAMECLIENTID) {
			MSG_SCMD_AH_SEARCH_RESULT_NEXT msgResult;
			msgResult.ResultSize = pMsg->ResultSize;
			msgResult.ResultCurIndex = pMsg->ResultCurIndex;
			msgResult.ResultCurCount = pMsg->ResultCurCount;
			msgResult.ResultOwnItem = pMsg->ResultOwnItem;

			for (UINT32 i = 0; i < msgResult.ResultCurCount; i++) {
				msgResult.ItemGUIDs[i] = pMsg->ItemGUIDs[i];
			}
			s_SendMessage(pGame, idGameClient, SCMD_AH_SEARCH_RESULT_NEXT, &msgResult);
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHSearchResponseItemInfo(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT_ITEM_INFO* pMsg = (MSG_APPCMD_AH_SEARCH_RESULT_ITEM_INFO*)pData;
	ASSERT_RETURN(pMsg != NULL);

	UNIT* pUnit = UnitGetByGuid(pGame, pMsg->PlayerGUID);
	if (pUnit) {
		GAMECLIENTID idGameClient = UnitGetClientId(pUnit);
		if (idGameClient != INVALID_GAMECLIENTID) {
			MSG_SCMD_AH_SEARCH_RESULT_ITEM_INFO msgResult;
			msgResult.ItemGUID = pMsg->ItemGUID;
			msgResult.ItemPrice = pMsg->ItemPrice;
			PStrCopy(msgResult.szSellerName, pMsg->szSellerName, MAX_CHARACTER_NAME);
			s_SendMessage(pGame, idGameClient, SCMD_AH_SEARCH_RESULT_ITEM_INFO, &msgResult);
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHSearchResponseItemBlob(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);

	MSG_APPCMD_AH_SEARCH_RESULT_ITEM_BLOB* pMsg = (MSG_APPCMD_AH_SEARCH_RESULT_ITEM_BLOB*)pData;
	ASSERT_RETURN(pMsg != NULL);

	UNIT* pUnit = UnitGetByGuid(pGame, pMsg->PlayerGUID);
	if (pUnit) {
		GAMECLIENTID idGameClient = UnitGetClientId(pUnit);
		if (idGameClient != INVALID_GAMECLIENTID) {
			MSG_SCMD_AH_SEARCH_RESULT_ITEM_BLOB msgResult;
			msgResult.ItemGUID = pMsg->ItemGUID;
			MemoryCopy(msgResult.ItemBlob, DEFAULT_MAX_ITEM_BLOB_MSG_SIZE, pMsg->ItemBlob, MSG_GET_BLOB_LEN(pMsg, ItemBlob));
			MSG_SET_BLOB_LEN(msgResult, ItemBlob, MSG_GET_BLOB_LEN(pMsg, ItemBlob));
			s_SendMessage(pGame, idGameClient, SCMD_AH_SEARCH_RESULT_ITEM_BLOB, &msgResult);
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdUtilGameCheckItemEmail(
	GAME* pGame,
	GAMECLIENT* /*pClient*/,
	UNIT* /*pUnit*/,
	BYTE* pData)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pData);
#if ISVERSION(SERVER_VERSION)
	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx);

	MSG_APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL* pMsg = (MSG_APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL*)pData;
	ASSERT_RETURN(pMsg != NULL);

	if (!GameIsUtilityGame(pGame)) {
		return;
	}

	CHAT_REQUEST_MSG_RETRIEVE_EMAIL_MESSAGES requestMsg;
	requestMsg.idAccountId = GameServerGetAuctionHouseAccountID(pSvrCtx);
	requestMsg.idCharacterId = GameServerGetAuctionHouseCharacterID(pSvrCtx);
	requestMsg.idTargetGameId = GameGetId(pGame);
	requestMsg.idOptionalEmailMessageId = pMsg->EmailGUID;

	ASSERT(
		GameServerToChatServerSendMessage(
		&requestMsg,
		GAME_REQUEST_RETRIEVE_EMAIL_MESSAGES));
#endif
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHGetInfo(
	GAME* pGame,
	GAMECLIENT* pClient,
	UNIT* pUnit,
	BYTE*)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pClient);
	UNREFERENCED_PARAMETER(pUnit);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pClient != NULL);
	ASSERT_RETURN(pUnit != NULL);

	if (!UnitIsInTown(pUnit)) {
		return;
	}

	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx != NULL);

	MSG_SCMD_AH_INFO msgInfo;
	{
		CSAutoLock autoLock(&pSvrCtx->m_csAuctionItemSales);
		msgInfo.dwFeeRate = pSvrCtx->m_dwAuctionHouseFeeRate;
		msgInfo.dwMaxItemSaleCount = pSvrCtx->m_dwAuctionHouseMaxItemSaleCount;
	}

	s_SendMessage(pGame, ClientGetId(pClient), SCMD_AH_INFO, &msgInfo);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHRetrieveSaleItems(
	GAME* pGame,
	GAMECLIENT*,
	UNIT* pUnit,
	BYTE*)
{
	UNREFERENCED_PARAMETER(pGame);
	UNREFERENCED_PARAMETER(pUnit);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pUnit != NULL);

	if (!UnitIsInTown(pUnit)) {
		return;
	}

	GAME_AUCTION_REQUEST_PLAYER_ITEM_SALE_LIST_MSG msgRequest;
	msgRequest.PlayerGUID = UnitGetGuid(pUnit);
	msgRequest.GameID = GameGetId(pGame);
	msgRequest.PlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	ASSERT(GameServerToAuctionServerSendMessage(&msgRequest, GAME_AUCTION_REQUEST_PLAYER_ITEM_SALE_LIST));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHSearchItems(
	GAME* pGame,
	GAMECLIENT*,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	MSG_CCMD_AH_SEARCH_ITEMS* pMsg = (MSG_CCMD_AH_SEARCH_ITEMS*)pData;
	GAME_AUCTION_REQUEST_SEARCH_ITEM_MSG msgRequest;

	msgRequest.GameID = GameGetId(pGame);
	msgRequest.PlayerGUID = UnitGetGuid(pUnit);
	msgRequest.PlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	msgRequest.ItemType = pMsg->ItemType;
	msgRequest.ItemClass = pMsg->ItemClass;
	msgRequest.ItemMinPrice = pMsg->ItemMinPrice;
	msgRequest.ItemMaxPrice = pMsg->ItemMaxPrice;
	msgRequest.ItemMinLevel = pMsg->ItemMinLevel;
	msgRequest.ItemMaxLevel = pMsg->ItemMaxLevel;
	msgRequest.ItemMinQuality = pMsg->ItemMinQuality;
	msgRequest.ItemSortMethod = pMsg->ItemSortMethod;

	ASSERT(GameServerToAuctionServerSendMessage(&msgRequest, GAME_AUCTION_REQUEST_SEARCH_ITEM));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHSearchItemNext(
	GAME* pGame,
	GAMECLIENT*,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	MSG_CCMD_AH_SEARCH_ITEMS_NEXT* pMsg = (MSG_CCMD_AH_SEARCH_ITEMS_NEXT*)pData;
	GAME_AUCTION_REQUEST_SEARCH_ITEM_NEXT_MSG msgRequest;
	msgRequest.PlayerGUID = UnitGetGuid(pUnit);
	msgRequest.GameID = GameGetId(pGame);
	msgRequest.SearchIndex = pMsg->SearchIndex;
	msgRequest.SearchSize = pMsg->SearchSize;
	msgRequest.SearchOwnItem = pMsg->SearchOwnItem;

	ASSERT(GameServerToAuctionServerSendMessage(&msgRequest, GAME_AUCTION_REQUEST_SEARCH_ITEM_NEXT));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHRequestItemInfo(
	GAME* pGame,
	GAMECLIENT*,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	MSG_CCMD_AH_REQUEST_ITEM_INFO* pMsg = (MSG_CCMD_AH_REQUEST_ITEM_INFO*)pData;
	GAME_AUCTION_REQUEST_ITEM_INFO_MSG msgRequest;
	msgRequest.PlayerGUID = UnitGetGuid(pUnit);
	// Change this back later
	msgRequest.PlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	msgRequest.GameID = GameGetId(pGame);
	msgRequest.ItemGUID = pMsg->ItemGUID;

	ASSERT(GameServerToAuctionServerSendMessage(&msgRequest, GAME_AUCTION_REQUEST_ITEM_INFO));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHRequestItemBlob(
	GAME* pGame,
	GAMECLIENT*,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	MSG_CCMD_AH_REQUEST_ITEM_BLOB* pMsg = (MSG_CCMD_AH_REQUEST_ITEM_BLOB*)pData;
	GAME_AUCTION_REQUEST_ITEM_BLOB_MSG msgRequest;
	msgRequest.PlayerGUID = UnitGetGuid(pUnit);
	// Change this back later
	msgRequest.PlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	msgRequest.GameID = GameGetId(pGame);
	msgRequest.ItemGUID = pMsg->ItemGUID;

	ASSERT(GameServerToAuctionServerSendMessage(&msgRequest, GAME_AUCTION_REQUEST_ITEM_BLOB));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ITEM_MESSAGE_LEN 32

static void sCCmdAHSellItem(
	GAME* pGame,
	GAMECLIENT* pGameClient,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pGameClient != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	GameServerContext * pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx != NULL);

	MSG_CCMD_AH_SELL_ITEM* pMsg = (MSG_CCMD_AH_SELL_ITEM*)pData;
	MSG_SCMD_AH_ERROR msgError;

	UNIT* pItem = UnitGetByGuid(pGame, pMsg->ItemGUID);
	if (pItem == NULL) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_DOES_NOT_EXIST;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	if (!TradeCanTradeItem(pUnit, pItem)) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_NOT_TRADABLE;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	if (!UnitIsA(pItem, UNITTYPE_MOD) &&
		!UnitIsA(pItem, UNITTYPE_WEAPON) &&
		!UnitIsA(pItem, UNITTYPE_ARMOR)) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_NOT_TRADABLE;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	DWORD dwMinPrice = x_AuctionGetMinSellPrice(pItem);
	if (pMsg->ItemPrice < dwMinPrice) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_PRICE_TOO_LOW;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	pSvrCtx->m_csAuctionItemSales.Enter();
	DWORD dwFeeRate = pSvrCtx->m_dwAuctionHouseFeeRate;
	pSvrCtx->m_csAuctionItemSales.Leave();

	DWORD dwFee = x_AuctionGetCostToSell(pMsg->ItemPrice, dwFeeRate);

	cCurrency money = UnitGetCurrency(pUnit);
	if ((DWORD)money.GetValue(KCURRENCY_VALUE_INGAME) < dwFee) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_NOT_ENOUGH_MONEY;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	WCHAR szMsg[ITEM_MESSAGE_LEN];
	DWORD dwPlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	PStrPrintf(szMsg, ITEM_MESSAGE_LEN, L"%x %x", pMsg->ItemPrice, dwPlayerVariant);

	PGUID idEmail = s_PlayerEmailCreateNewEmail(pGame, pGameClient, pUnit, L"", szMsg, ESS_PLAYER_ITEMSALE);
	if (idEmail == INVALID_GUID) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_INTERNAL_ERROR;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	s_PlayerEmailAddEmailRecipientByCharacterName(pUnit, AUCTION_OWNER_NAME);
	s_PlayerEmailAddEmailAttachments(pUnit, UnitGetGuid(pItem), dwFee);

	WCHAR szPlayerName[MAX_CHARACTER_NAME];
	UnitGetName(pUnit, szPlayerName, MAX_CHARACTER_NAME, 0);
	UtilGameAuctionItemSaleHandleEmailMetaData(pSvrCtx,
		GameGetId(pGame),
		idEmail,
		pMsg->ItemGUID,
		UnitGetGuid(pUnit),
		szPlayerName);

	if (!s_PlayerEmailSend(pGame, pGameClient, pUnit, 0)) {
		msgError.ErrorCode = AH_ERROR_SELL_ITEM_INTERNAL_ERROR;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);

		UtilGameAuctionItemSaleRemoveItemByItemID(pSvrCtx, pMsg->ItemGUID);
		return;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHSellRandomItem(
	GAME* pGame,
	GAMECLIENT* pGameClient,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pGameClient != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(DEBUG_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	int nInvLocAuctionOutbox = GlobalIndexGet(GI_INVENTORY_AUCTION_OUTBOX);

	UNIT* pItem = NULL;	
	while ((pItem = UnitInventoryIterate(pUnit, pItem, 0)) != NULL) {
		if (UnitIsA(pItem, UNITTYPE_MOD) ||
			UnitIsA(pItem, UNITTYPE_WEAPON) ||
			UnitIsA(pItem, UNITTYPE_ARMOR)) {

			if (InventoryMoveToAnywhereInLocation(pUnit,
				pItem, nInvLocAuctionOutbox,
				CLF_ALLOW_NEW_CONTAINER_BIT))
			{
				MSG_CCMD_AH_SELL_ITEM msgSell;
				msgSell.ItemGUID = UnitGetGuid(pItem);
				msgSell.ItemPrice = 100;
				sCCmdAHSellItem(pGame, pGameClient, pUnit, (BYTE*)&msgSell);
				break;
			}
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHWithdrawItem(
	GAME* pGame,
	GAMECLIENT* pGameClient,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pGameClient != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);

#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	MSG_CCMD_AH_WITHDRAW_ITEM* pMsg = (MSG_CCMD_AH_WITHDRAW_ITEM*)pData;
	ASSERT_RETURN(pMsg != NULL);

	GAME_AUCTION_REQUEST_WITHDRAW_ITEM_MSG msgWithdraw;
	msgWithdraw.PlayerGUID = UnitGetGuid(pUnit);
	msgWithdraw.GameID = GameGetId(pGame);
	msgWithdraw.ItemGUID = pMsg->ItemGUID;
	UnitGetName(pUnit, msgWithdraw.PlayerName, MAX_CHARACTER_NAME, 0);
	ASSERT(GameServerToAuctionServerSendMessage(&msgWithdraw, GAME_AUCTION_REQUEST_WITHDRAW_ITEM));
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCCmdAHBuyItem(
	GAME* pGame,
	GAMECLIENT* pGameClient,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pGameClient != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);
#if ISVERSION(SERVER_VERSION)
	if (!UnitIsInTown(pUnit)) {
		return;
	}

	MSG_CCMD_AH_BUY_ITEM * buyMsg = (MSG_CCMD_AH_BUY_ITEM*)pData;
	MSG_SCMD_AH_ERROR msgError;
	msgError.ItemGUID = buyMsg->ItemGUID;

	cCurrency money = UnitGetCurrency(pUnit);
	if ((DWORD)money.GetValue(KCURRENCY_VALUE_INGAME) < buyMsg->ItemPrice) {
		msgError.ErrorCode = AH_ERROR_BUY_ITEM_NOT_ENOUGH_MONEY;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	GAME_AUCTION_REQUEST_BUY_ITEM_CHECK_MSG buyCheckMsg;
	buyCheckMsg.PlayerGUID = UnitGetGuid(pUnit);
	buyCheckMsg.PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), ClientGetAppId(pGameClient));
	buyCheckMsg.PlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	buyCheckMsg.GameID = GameGetId(pGame);
	buyCheckMsg.ItemGUID = buyMsg->ItemGUID;
	buyCheckMsg.ItemPrice = buyMsg->ItemPrice;

	if (!GameServerToAuctionServerSendMessage(&buyCheckMsg, GAME_AUCTION_REQUEST_BUY_ITEM_CHECK))
	{
		msgError.ErrorCode = AH_ERROR_BUY_ITEM_INTERNAL_ERROR;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppCmdAHBuyItemValidated(
	GAME* pGame,
	GAMECLIENT* pGameClient,
	UNIT* pUnit,
	BYTE* pData)
{
	ASSERT_RETURN(pGame != NULL);
	ASSERT_RETURN(pGameClient != NULL);
	ASSERT_RETURN(pUnit != NULL);
	ASSERT_RETURN(pData != NULL);
#if ISVERSION(SERVER_VERSION)

	MSG_APPCMD_AH_OK_TO_BUY * buyMsg = (MSG_APPCMD_AH_OK_TO_BUY*)pData;
	MSG_SCMD_AH_ERROR msgError;
	msgError.ItemGUID = buyMsg->ItemGUID;

	if (pUnit->pEmailSpec != NULL)
	{
		msgError.ErrorCode = AH_ERROR_BUY_ITEM_INTERNAL_ERROR;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	WCHAR szMsg[MAX_EMAIL_BODY];
	DWORD dwPlayerVariant = GameVariantFlagsGetStaticUnit(pUnit);
	PGUID itemEmailId = GameServerGenerateGUID();
	PGUID moneyEmailId = GameServerGenerateGUID();
	PStrPrintf(szMsg, arrsize(szMsg), L"%I64x %x %I64x %I64x", buyMsg->ItemGUID, dwPlayerVariant, itemEmailId, moneyEmailId);

	PGUID idEmail = s_PlayerEmailCreateNewEmail(pGame, pGameClient, pUnit, L"", szMsg, ESS_PLAYER_ITEMBUY);
	if (idEmail == INVALID_GUID)
	{
		msgError.ErrorCode = AH_ERROR_BUY_ITEM_INTERNAL_ERROR;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}

	s_PlayerEmailAddEmailRecipientByCharacterName(pUnit, AUCTION_OWNER_NAME);
	s_PlayerEmailAddEmailAttachments(pUnit, INVALID_GUID, buyMsg->ItemPrice);

	if (!s_PlayerEmailSend(pGame, pGameClient, pUnit, 0))
	{
		msgError.ErrorCode = AH_ERROR_BUY_ITEM_INTERNAL_ERROR;
		s_SendMessage(pGame, ClientGetId(pGameClient), SCMD_AH_ERROR, &msgError);
		return;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#pragma warning(pop)

typedef void FP_PROCESS_GAME_MESSAGE(GAME * game, GAMECLIENT * client, UNIT * unit, BYTE * data);

//----------------------------------------------------------------------------
enum MESSAGE_CONDITION
{
	GAME_NO,		// ok out of game
	GAME_YES,		// must be in game
	GAME_ROUTE_TO,	// route to a game
	UTIL_GAME,		// route to the utility game
	UNIT_NO,		// ok without unit
	UNIT_YES,		// must have unit
	DEAD_NO,		// dead can't do
	DEAD_YES,		// dead can do
	LIMBO_NO,		// limbo can't do
	LIMBO_YES,		// limbo can do
	GHOST_NO,		// ghost can't do
	GHOST_YES,		// ghost can do
};

//----------------------------------------------------------------------------
enum MESSAGE_SOURCE
{
	CLIENT_MESSAGE,
	APP_MESSAGE,
};

//----------------------------------------------------------------------------
struct PROCESS_GAME_MESSAGE_STRUCT
{
	NET_CMD						cmd;				// command
	FP_PROCESS_GAME_MESSAGE *	func;				// handler
	MESSAGE_CONDITION			eMCInGame;			// in game message
	MESSAGE_CONDITION			eMCNeedUnit;		// can we do this without a control unit
	MESSAGE_CONDITION			eMCCanDoWhileDead;	// can we do this when dead	
	MESSAGE_CONDITION			eMCCanDoInLimbo;	// should add limbo switch here
	MESSAGE_CONDITION			eMCGhostAllowed;	// can be done as a ghost
	MESSAGE_SOURCE				eMessageSource;		// can this command come from the client, or is it purely for server to game messaging
	const char *				szFuncName;
};

#define CLT_MESSAGE_PROC(commmand, function, game, unit, dead, limbo, ghost, cltMsg) { commmand, function, game, unit, dead, limbo, ghost, cltMsg, #function },

PROCESS_GAME_MESSAGE_STRUCT gClientGameMessageHandlerTbl[] =
{	
	
	// this seems like a prime thing to be put into an excel table 
	// since messages have these column options now -Colin
	
	// messages processed on a game server app level (aka, not inside a game)
	CLT_MESSAGE_PROC(CCMD_PLAYERNEW,					sCCmdPlayerJoin,				GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_NEW_ITEM_COLLECTION,		sCCmdAssert,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	APP_MESSAGE)	
	CLT_MESSAGE_PROC(APPCMD_PLAYER_IN_CHAT_SERVER,		sAppCmdPlayerInChatServer,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_CHAT_CHANNEL_CREATED,		sAppCmdChatChannelCreated,		GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AUTO_PARTY_CREATED,			sAppCmdAutoPartyCreated,		GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_OPEN_PLAYER_FILE_START,		NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_OPEN_PLAYER_FILE_CHUNK,		NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DATABASE_PLAYER_FILE_START,	NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DATABASE_PLAYER_FILE_CHUNK,	NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_SWITCHINSTANCE,				sCCmdAssert,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_UNRESERVEGAME,				sCCmdAssert,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_SWITCHSERVER,				sCCmdAssert,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	APP_MESSAGE)
 	CLT_MESSAGE_PROC(APPCMD_PLAYER_GUILD_ACTION_RESULT,	NULL,							GAME_NO,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_PLAYER_GUILD_ACTION_RESULT_GAME,sAppCmdGuildActionResult,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
 	CLT_MESSAGE_PROC(APPCMD_RECOVER_LEVEL_CHAT_CHANNELS,NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
 	CLT_MESSAGE_PROC(APPCMD_PLAYER_GUILD_DATA_APP,		NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_PLAYER_GUILD_DATA_GAME,		sAppCmdPlayerGuildData,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_PARTY_MEMBER_INFO_APP,		NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_PARTY_MEMBER_INFO_GAME,		sAppCmdPlayerPartyChanged,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	
	// processed inside a game, but app to self only.
	CLT_MESSAGE_PROC(APPCMD_TAKE_OVER_CLIENT,			sAppCmdTakeOverClient,			GAME_NO,		UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_PLAYERGAG, 					sAppCmdPlayerGag,				GAME_YES,		UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)	
	CLT_MESSAGE_PROC(APPCMD_UTILITY_GAME_REQUEST, 		sAppCmdUtilityGameRequest,		UTIL_GAME,		UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)		
	CLT_MESSAGE_PROC(APPCMD_IN_GAME_DB_CALLBACK, 		sAppCmdInGameDBCallback,		GAME_ROUTE_TO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DELETE_ITEM,				sAppCmdDeleteItem,				GAME_ROUTE_TO,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
		
	// client messages	
	CLT_MESSAGE_PROC(CCMD_PLAYERREMOVE,					sCCmdPlayerRemove,				GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQMOVE,						sCCmdReqMove,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_UNITMOVEXYZ,					sCCmdUnitMoveXYZ,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_UNITSETFACEDIRECTION,			sCCmdUnitSetFaceDirection,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_UNIT_PATH_POSITION_UPDATE,	sCCmdUnitPathPosUpdate,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_UNITWARP,						sCCmdUnitWarp,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PLAYER_FELL_OUT_OF_WORLD,		sCCmdUnitFellOutOfWorld,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSTART,					sCCmdSkillStart,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSTARTXYZ,				sCCmdSkillStartXYZ,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSTARTID,					sCCmdSkillStartID,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSTARTXYZID,				sCCmdSkillStartXYZID,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLCHANGETARGETID,			sCCmdSkillChangeID,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSTOP,					sCCmdSkillEnd,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSHIFT_ENABLE,			sCCmdSkillShiftEnable,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)	
	CLT_MESSAGE_PROC(CCMD_PLAYER_RESPAWN,				sCCmdPlayerRespawn,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PICKUP,						sCCmdPickup,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)	
	CLT_MESSAGE_PROC(CCMD_INVEQUIP,						sCCmdInvEquip,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INVDROP,						sCCmdInvDrop,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INVPUT,						sCCmdInvPut,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INVSWAP,						sCCmdInvSwap,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEMINVPUT,					sCCmdItemInvPut,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INVUSE,						sCCmdInvUse,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INVUSEON,						sCCmdInvUseOn,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INVSHOWINCHAT,				sCCmdInvShowInChat,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_HOTKEY,						sCCmdHotKey,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ADD_WEAPONCONFIG,				sCCmdAddToWeaponConfig,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SELECT_WEAPONCONFIG,			sCCmdSelectWeaponConfig,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_CHEAT,						sCCmdCheat,						GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INTERACT,						sCCmdInteract,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_QUEST_INTERACT_DIALOG,		sCCmdQuestInteractDialog,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQUEST_INTERACT_CHOICES,		sCCmdRequestInteractChoices,	GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)	
	CLT_MESSAGE_PROC(CCMD_SKILLSELECT,					sCCmdSkillSelect,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLDESELECT,				sCCmdSkillDeselect,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLPICKMELEESKILL,			sCCmdSkillPickMeleeSkill,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ALLOCSTAT,					sCCmdAllocStat,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ABORTQUEST,					sCCmdAbortQuest,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EXIT_GAME,					sCCmdExitGame,					GAME_YES,	UNIT_NO,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DOUBLEEQUIP,					sCCmdDoubleEquip,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SKILLSAVEASSIGN,				sCCmdSkillSaveAssign,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SETDEBUGUNIT,					sCCmdSetDebugUnit,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_LEVELLOADED,					sCCmdLevelLoaded,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEMSELL,						sCCmdItemSell,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEMBUY,						sCCmdItemBuy,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRY_IDENTIFY,					sCCmdTryIdentify,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEM_PICK_ITEM,				sCCmdItemPickedItem,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_CANCEL_IDENTIFY,				sCCmdCanceldentify,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)		
	CLT_MESSAGE_PROC(CCMD_TRY_DISMANTLE,				sCCmdTryDismantle,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PVP_SIGN_UP_ACCEPT,			sCCmdPvPSignUpAccept,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RETURN_TO_LOWEST_LEVEL,		sCCmdReturnToLowestDungeonLevel,GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_BUY_HIRELING,					sCCmdBuyHireling,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ACHIEVEMENT_SELECTED,			sCCmdAchievementSelected,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)

	CLT_MESSAGE_PROC(CCMD_LIST_GAME_INSTANCES,			sCCmdListGameInstances,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_WARP_TO_GAME_INSTANCE,		sCCmdWarpToGameInstance,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQUEST_AREA_WARP,			sCCmdRequestAreaWarp,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_QUEST_TRY_ACCEPT,				sCCmdQuestAccept,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RECIPE_CLEAR_PLAYER_MODIFICATIONS, sCCmdRecipeClearPropertis,	GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RECIPE_SET_PLAYER_MODIFICATIONS, sCCmdRecipeSetProperty,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RECIPE_TRY_CREATE,			sCCmdRecipeCreate,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RECIPE_TRY_BUY,				sCCmdRecipeBuy,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RECIPE_CLOSE,					sCCmdRecipeClose,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RECIPE_SELECT,				sCCmdRecipeSelect,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)	
	CLT_MESSAGE_PROC(CCMD_TASK_TRY_ACCEPT,				sCCmdTaskAccept,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TASK_TRY_ABANDON,				sCCmdTaskAbandon,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TASK_TRY_ACCEPT_REWARD,		sCCmdTaskAcceptReward,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_QUEST_TRY_ABANDON,			sCCmdQuestTryAbandon,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TALK_DONE,					sCCmdTalkDone,					GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TUTORIAL_UPDATE,				sCCmdTutorialUpdate,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRY_RETURN_TO_HEADSTONE,		sCCmdTryReturnToHeadstone,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)	
	CLT_MESSAGE_PROC(CCMD_REQ_SKILL_UP,					sCCmdReqSkillUp,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQ_TIER_UP,					sCCmdReqTierUp,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQ_CRAFTING_TIER_UP,			sCCmdReqCraftingTierUp,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQ_SKILL_UNLEARN,			sCCmdReqSkillUnlearn,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQ_WEAPONSET_SWAP,			sCCmdReqWeaponSetSwap,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AGGRESSION_TOGGLE,			sCCmdAggressionToggle,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_CLIENT_QUEST_STATE_CHANGE,	sCCmdClientQuestStateChange,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_OPERATE_WAYPOINT,				sCCmdOperateWaypoint,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_OPERATE_ANCHORSTONE,			sCCmdOperateAnchorstone,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_OPERATE_RUNEGATE,				sCCmdOperateRunegate,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRADE_CANCEL,					sCCmdTradeCancel,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRADE_STATUS,					sCCmdTradeStatus,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)	
	CLT_MESSAGE_PROC(CCMD_TRADE_REQUEST_NEW,			sCCmdTradeRequestNew,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRADE_REQUEST_NEW_CANCEL,		sCCmdTradeRequestNewCancel,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRADE_REQUEST_ACCEPT,			sCCmdTradeRequestAccept,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRADE_REQUEST_REJECT,			sCCmdTradeRequestReject,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRADE_MODIFY_MONEY,			sCCmdTradeModifyMoney,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_INSPECT_PLAYER,				sCCmdInspectPlayer,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_WARP_TO_PLAYER,				sCCmdWarpToPlayer,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SET_PLAYER_OPTIONS,			sCCmdSetCharacterOptions,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REWARD_TAKE_ALL,				sCCmdRewardTakeAll,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REWARD_CANCEL,				sCCmdRewardCancel,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_JUMP_BEGIN,					sCCmdJumpBegin,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_JUMP_END,						sCCmdJumpEnd,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ENTER_TOWN_PORTAL,			sCCmdEnterTownPortal,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SELECT_RETURN_DEST,			sCCmdSelectReturnDest,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PARTY_ADVERTISE,				sCCmdPartyAdvertise,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PARTY_UNADVERTISE,			sCCmdPartyUnadvertise,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TOGGLE_AUTO_PARTY,			sCCmdToggleAutoParty,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PVP_TOGGLE,					sCCmdPvPToggle,					GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PING,							sCCmdPing,						GAME_YES,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PICK_COLORSET,				sCCmdPickColorset,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_BOT_CHEAT,					sCCmdBotCheat,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PARTY_MEMBER_TRY_WARP_TO,		sCCmdPartyMemberWarpTo,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PARTY_JOIN_ATTEMPT,			sCCmdPartyJoinAttempt,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PARTY_ACCEPT_INVITE,			sCCmdPartyAcceptInvite,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_STASH_CLOSE,					sCCmdStashClose,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRY_ITEM_UPGRADE,				sCCmdTryItemUpgrade,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRY_ITEM_AUGMENT,				sCCmdTryItemAugment,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_TRY_ITEM_UNMOD,				sCCmdTryItemUnMod,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEM_UPGRADE_CLOSE,			sCCmdItemUpgradeClose,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMOTE,						sCCmdEmote,						GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_WHO,							sCCmdWho,						GAME_YES,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_KILLPET,						sCCmdKillPet,					GAME_YES,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_JOIN_INSTANCING_CHANNEL,		sCCmdJoinInstancingChannel,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_QUEST_TRACK,					sCCmdQuestTrack,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AUTO_TRACK_QUESTS,			sCCmdAutoTrackQuests,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_UI_RELOAD,					sCCmdUIReload,					GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REMOVE_FROM_WEAPONCONFIG,		sCCmdRemoveFromWeaponConfig,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_REQUEST_REAL_MONEY_TXN,		sCCmdRequestRealMoneyTxn,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ACCOUNT_STATUS_UPDATE,		sCCmdAccountStatusUpdate,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ACCOUNT_BALANCE_UPDATE,		sCCmdAccountBalanceUpdate,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_QUEST_ABANDON,				sCCmdQuestAbandon,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_MOVIE_FINISHED,				sCCmdMovieFinished,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DUEL_INVITE,					sCCmdDuelInvite,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DUEL_INVITE_ACCEPT,			sCCmdDuelInviteAccept,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DUEL_INVITE_DECLINE,			sCCmdDuelInviteDecline,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DUEL_INVITE_FAILED,			sCCmdDuelInviteFailed,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_CREATE,					sCCmdGuildCreate,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_INVITE,					sCCmdGuildInvite,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_DECLINE_INVITE,			sCCmdGuildDeclineInvite,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_ACCEPT_INVITE,			sCCmdGuildAcceptInvite,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_LEAVE,					sCCmdGuildLeave,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_REMOVE,					sCCmdGuildRemove,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_CHANGE_RANK,			sCCmdGuildChangeRank,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_CHANGE_RANK_NAME,		sCCmdGuildChangeRankName,		GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_GUILD_CHANGE_ACTION_PERMISSIONS,sCCmdGuildChangeActionPerms,	GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RESPEC,						sCCmdRespec,					GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_RESPECCRAFTING,				sCCmdRespecCrafting,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_STUCK,						sCCmdStuck,						GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_CUBE_TRY_CREATE,				sCCmdCubeTryCreate,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEM_SPLIT_STACK,				sCCmdItemSplitStack,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DUEL_AUTOMATED_SEEK,			sCCmdDuelAutomatedSeek,			GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DUEL_AUTOMATED_HOST,		sAppCmdDuelAutomatedHost,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DUEL_AUTOMATED_GUEST,		sAppCmdDuelAutomatedGuest,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DUEL_NEW_RATING,			sCCmdDuelNewRating,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_WARP_TO_PLAYER_RESULT,		sAppCmdWarpToPlayerData,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_PLAYED,						sCCmdPlayed,					GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_SELECT_PLAYER_TITLE,			sCCmdSelectTitle,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_DONATE_MONEY,					sCCmdDonateMoney,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_DONATION_EVENT,				NULL,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_SEND_START,				sCCmdEmailSendStart,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_ADD_RECIPIENT_BY_CHARACTER_NAME,sCCmdEmailAddByName,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_ADD_GUILD_MEMBERS_AS_RECIPIENTS,sCCmdEmailAddByGuild,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_SET_ATTACHMENTS,		sCCmdEmailSetAttachments,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_SEND_COMMIT,			sCCmdEmailSendCommit,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_CREATION_RESULT,			sAppCmdEmailCreationResult,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_CREATION_RESULT_UTIL_GAME,sAppCmdEmailCreationResultUtilGame,	UTIL_GAME,	UNIT_NO,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)	
	CLT_MESSAGE_PROC(APPCMD_EMAIL_ITEM_TRANSFER_RESULT,	sAppCmdEmailItemTransferResult,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_MONEY_DEDUCTION_RESULT,sAppCmdEmailMoneyDeductionResult,GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_DELIVERY_RESULT,			sAppCmdEmailDeliveryResult,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_DELIVERY_RESULT_UTIL_GAME,sAppCmdEmailDeliveryResultUtilGame,	UTIL_GAME,	UNIT_NO,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)	
	CLT_MESSAGE_PROC(APPCMD_EMAIL_NOTIFICATION,			sAppCmdEmailNotification,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_METADATA,				sAppCmdEmailMetadata,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_ACCEPTED_EMAIL_DATA,		sAppCmdEmailData,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_UPDATE,				sAppCmdEmailDataUpdate,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_RESTORE_OUTBOX_ITEM_LOC,sCCmdEmailRestoreOutboxItemLoc,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_MARK_MESSAGE_READ,		sCCmdEmailMarkMessageRead,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_REMOVE_ATTACHED_MONEY,	sCCmdEmailRemoveAttachedMoney,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_CLEAR_ATTACHED_ITEM,	sCCmdEmailClearAttachedItem,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_EMAIL_DELETE_MESSAGE,			sCCmdEmailDeleteMessage,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_ATTACHED_MONEY_INFO,	sAppCmdEmailAttachedMoneyInfo,	GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_EMAIL_MONEY_REMOVAL_RESULT,	sAppCmdEmailAttachedMoneyResult,GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_ITEM_MESSAGE,					sCCmdItemChatMessage,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_NO,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_CREATE_PVP_GAME,				sCCmdCreatePvpGame,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_JOIN_PVP_GAME,				sCCmdJoinPvpGames,				GAME_YES,	UNIT_YES,	DEAD_NO,	LIMBO_NO,	GHOST_NO,	CLIENT_MESSAGE)


	CLT_MESSAGE_PROC(APPCMD_AH_ERROR,					sAppCmdAHError,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AH_PLAYER_ITEM_SALE_LIST,	sAppCmdAHPlayerItemSaleList,	GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AH_SEARCH_RESULT,			sAppCmdAHSearchResponse,		GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AH_SEARCH_RESULT_NEXT,		sAppCmdAHSearchResponseNext,	GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AH_SEARCH_RESULT_ITEM_INFO,	sAppCmdAHSearchResponseItemInfo,GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AH_SEARCH_RESULT_ITEM_BLOB,	sAppCmdAHSearchResponseItemBlob,GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_AH_OK_TO_BUY,				sAppCmdAHBuyItemValidated,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)

	CLT_MESSAGE_PROC(APPCMD_GENERATE_RANDOM_ITEM,					sAppCmdGenerateRandomItem,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_GENERATE_RANDOM_ITEM_GETBLOB,			sAppCmdGenerateRandomItemGetBlob,			GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_UTIL_GAME_ADD_AH_CLIENT,				sAppCmdUtilGameAddAHClient,					GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL,				sAppCmdUtilGameCheckItemEmail,				GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_UTIL_GAME_EMAIL_METADATA,				sAppCmdEmailMetadata,						GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_UTIL_GAME_EMAIL_DATA,					sAppCmdEmailData,							GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)
	CLT_MESSAGE_PROC(APPCMD_UTIL_GAME_EMAIL_ITEM_TRANSFER_RESULT,	sAppCmdEmailItemTransferResultUtilGame,		GAME_NO,	UNIT_NO,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	APP_MESSAGE)

	CLT_MESSAGE_PROC(CCMD_AH_GET_INFO,					sCCmdAHGetInfo,					GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_RETRIEVE_SALE_ITEMS,		sCCmdAHRetrieveSaleItems,		GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_SEARCH_ITEMS,				sCCmdAHSearchItems,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_SEARCH_ITEMS_NEXT,			sCCmdAHSearchItemNext,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_REQUEST_ITEM_INFO,			sCCmdAHRequestItemInfo,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_REQUEST_ITEM_BLOB,			sCCmdAHRequestItemBlob,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_SELL_ITEM,					sCCmdAHSellItem,				GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_SELL_RANDOM_ITEM,			sCCmdAHSellRandomItem,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_WITHDRAW_ITEM,				sCCmdAHWithdrawItem,			GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)
	CLT_MESSAGE_PROC(CCMD_AH_BUY_ITEM,					sCCmdAHBuyItem,					GAME_YES,	UNIT_YES,	DEAD_YES,	LIMBO_YES,	GHOST_YES,	CLIENT_MESSAGE)

	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void SrvProcessGameMessage(
	GAME * game,
	GAMECLIENT * client,
	NET_CMD command,
	MSG_STRUCT * msg)
{
	ASSERT_RETURN(game);
	const PROCESS_GAME_MESSAGE_STRUCT *pMessageInfo = &gClientGameMessageHandlerTbl[command];
	GAMECLIENTID idClient = client ? ClientGetId( client ) : INVALID_GAMECLIENTID;
	UNIT *unit = NULL;	
	
	if (client)
	{
	
		// validate this client
		DBG_ASSERT(ClientValidateIdConsistency(game, idClient));
		
		// get the unit for this client for validations
		unit = ClientGetControlUnit(client);
		
	}

	// check message game logic requirements (TODO: data drive this better)
		
	// requires unit
	if (pMessageInfo->eMCNeedUnit == UNIT_YES && !unit)
	{
		return;
	}

	// dead
	if (pMessageInfo->eMCCanDoWhileDead == DEAD_NO && unit && IsUnitDeadOrDying(unit))
	{
		trace( "Rejecting '%s' from client [%d] because they are dead\n", pMessageInfo->szFuncName, idClient );	
		return;
	}
	
	// limbo
	if (pMessageInfo->eMCCanDoInLimbo == LIMBO_NO && unit && UnitIsInLimbo(unit))
	{
		trace( "Rejecting '%s' from client [%d] because they are in limbo\n", pMessageInfo->szFuncName, idClient );		
		return;
	}

	// ghost
	if (unit && pMessageInfo->eMCGhostAllowed == GHOST_NO && UnitIsGhost( unit ))
	{
		trace( "Rejecting '%s' from client [%d] because they are a ghost ... spooky!\n", pMessageInfo->szFuncName, idClient );
		return;
	}
	
	// do function callback
  	pMessageInfo->func(game, client, unit, (BYTE *)msg);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sProcessNewSystemItemCollection( 
	GAME *pGame,
	const MSG_APPCMD_NEW_ITEM_COLLECTION *pNewItemCollectionMessage)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pNewItemCollectionMessage, "Expected message" );
	CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
	ASSERTX_RETURN( pClientSystem, "Expected client system" );
	UNIT_COLLECTION_ID idCollection = (UNIT_COLLECTION_ID)pNewItemCollectionMessage->dwUntiCollectionID;
	APPCLIENTID idAppClient = pNewItemCollectionMessage->dwAppClientID;
					
	// get the save file
	CLIENT_SAVE_FILE tClientSaveFile;
	if (ClientSystemGetClientSaveFile(
			pClientSystem,
			idAppClient,
			UCT_ITEM_COLLECTION,
			idCollection,
			&tClientSaveFile) == FALSE)
	{
		return;
	}

	switch (tClientSaveFile.ePlayerLoadType) {
#if ISVERSION(SERVER_VERSION)
		case PLT_ITEM_RESTORE:
		{
			ASSERTX_RETURN( GameIsUtilityGame( pGame ), "Utility game logic only" );
			ASSERT_RETURN(ClientSystemIsSystemAppClient(idAppClient));
			ASSERT_RETURN(idAppClient == APPCLIENTID_ITEM_RESTORE_SYSTEM);

			s_ItemRestoreDatabaseLoadComplete( pGame, tClientSaveFile );
			break;
		}

		case PLT_AH_ITEMSALE_SEND_TO_AH:
		{
			ASSERTX_RETURN( GameIsUtilityGame( pGame ), "Utility game logic only" );
			ASSERT_RETURN(ClientSystemIsSystemAppClient(idAppClient));
			ASSERT_RETURN(idAppClient == APPCLIENTID_AUCTION_SYSTEM);

			DATABASE_UNIT_COLLECTION_RESULTS tResults;
			if (s_DBUnitsLoadFromClientSaveFile( 
				pGame, 
				NULL,
				NULL, 
				&tClientSaveFile, 
				&tResults))
			{
				ASSERTX_RETURN( tResults.nNumUnits == 1, "Expected 1 unit only" );
				UNIT *pItem = tResults.pUnits[ 0 ];
				if (pItem != NULL) 
				{
					GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
					UtilGameAuctionItemSaleHandleItem( pServerContext, pGame, pItem );
					UnitFree( pItem );
				}
			}
			break;
		}
#endif
		// will character & account units ever get here?
		case PLT_CHARACTER:
		case PLT_ITEM_TREE:
		case PLT_ACCOUNT_UNITS:
		{
			ASSERT_RETURN(!ClientSystemIsSystemAppClient(idAppClient));

			PGUID guidPlayer = ClientSystemGetGuid( pClientSystem, idAppClient );
			if (guidPlayer != INVALID_GUID)
			{

				// must be intended for this player (this is to prevent any wacky edge cases
				// of items being slowly sent to a destination game, but the dest client has
				// exited the server and somebody else logged in and go the same app client
				// id ... doesn't seem likely, but seems like a good thing to check for 
				// just for sanity -cday
				if (guidPlayer == pNewItemCollectionMessage->guidDestPlayer)
				{
					UNIT *pPlayer = UnitGetByGuid( pGame, guidPlayer );
					if (pPlayer)
					{
						PlayerLoadUnitCollection( pPlayer, &tClientSaveFile );
					}
				}
			}
			break;
		}

		default:
			ASSERT(FALSE);
			break;
	}
}		 

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void SrvGameProcessMessage(
	GAME * game,
	NETCLIENTID64 idNetClient,
	MSG_STRUCT * msg,
	BOOL bFake = FALSE)
{
	NET_CMD command = msg->hdr.cmd;

	ASSERT_RETURN(command >= 0 && command < CCMD_LAST);
	ASSERT_RETURN(gClientGameMessageHandlerTbl[command].func ||
				  gClientGameMessageHandlerTbl[command].eMCInGame == GAME_NO);

	DBG_ASSERT(NetClientValidateIdConsistency(idNetClient));

	CLT_VERSION_ONLY(NetTraceRecvMessage(EmbeddedServerRunnerGetCommandTable(USER_SERVER,GAME_SERVER,TRUE), msg, "SRVRCV: "));

	//RJD client command logging code.
	//if(gClientGameMessageHandlerTbl[command].cmd != CCMD_REQMOVE && !bFake) //ignore ReqMove
	{
		//trace( "c_message %2d %d %25s\t%d \n", gClientGameMessageHandlerTbl[command].cmd, -1, gClientGameMessageHandlerTbl[command].szFuncName, msg->hdr.size);
		/*		TraceGameInfo( "\n0x");
		for(int i = 0; i <= msg->hdr.size; i+=4)
		{
		TraceGameInfo( "%08x ", *((int *)msg + i/4) );
		}
		TraceGameInfo( "\n");*/
		//log byte code to check matchup.
		/*for(int i = 0; i <= msg->hdr.size; i++)
		{
			trace( "%02x", (unsigned int)*((BYTE*)msg + i) );
			if(i%4 == 3) trace( " ");
		}
		trace( "\n");*/
	}

	GameServerContext* pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pSvrCtx != NULL);

	PROCESS_GAME_MESSAGE_STRUCT *pMessageStruct = &gClientGameMessageHandlerTbl[ command ];
	ASSERTX_RETURN( pMessageStruct, "Expected message struct" );

	// handle messages that should be auto routed to a game
	if (pMessageStruct->eMCInGame == GAME_ROUTE_TO)
	{
		ASSERT_RETURN(bFake);
		GAMECLIENTID idGameClient = NetClientGetGameClientId(idNetClient);
		//Process the message even if we have no gameclient.
		GAMECLIENT * client = ClientGetById(game, idGameClient);
		SrvProcessGameMessage(game, client, command, msg);
	}
	else if (pMessageStruct->eMCInGame == UTIL_GAME)
	{
		ASSERT_RETURN(bFake);
		SrvProcessGameMessage(game, NULL, command, msg);
	}
	else if (pMessageStruct->eMCInGame == GAME_NO)
	{
		switch (command)
		{
		
		//----------------------------------------------------------------------------	
		case CCMD_PLAYERNEW:
			{
				ASSERT_RETURN(bFake);

				GAMECLIENTID idGameClient = NetClientGetGameClientId(idNetClient);
				ASSERT_BREAK(idGameClient == INVALID_GAMECLIENTID);

				if(NetClientQueryDetached(idNetClient) == TRUE)
				{
					break;
				}
				ASSERT_BREAK(NetClientQueryRemoved(idNetClient) == FALSE);

				APPCLIENTID idAppClient = NetClientGetAppClientId(idNetClient);
				if (idAppClient != INVALID_CLIENTID)
				{

					ASSERT(sPlayerJoinGame(game, idAppClient, (MSG_CCMD_PLAYERNEW *)msg));
				}
			}
			break;
			
		//----------------------------------------------------------------------------			
		case APPCMD_NEW_ITEM_COLLECTION:
			{
				const MSG_APPCMD_NEW_ITEM_COLLECTION *pNewItemCollectionMessage = (const MSG_APPCMD_NEW_ITEM_COLLECTION *)msg;
				ASSERTX_BREAK( pNewItemCollectionMessage, "Expected new item collection message" );

				APPCLIENTID idAppClient = pNewItemCollectionMessage->dwAppClientID;
				if (idNetClient == INVALID_NETCLIENTID64) 
				{
					idAppClient = NetClientGetAppClientId(idNetClient);
				}
				ASSERTX_BREAK( idAppClient != INVALID_ID, "Expected app client id" );				

				CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
				ASSERTX_BREAK( pClientSystem, "Expected client system" );

				UNIT_COLLECTION_ID idCollection = (UNIT_COLLECTION_ID)pNewItemCollectionMessage->dwUntiCollectionID;
				
				// process this new item collection
				sProcessNewSystemItemCollection( game, pNewItemCollectionMessage );
				
				// release the collection back to this client to be reused if we receive
				// another item tree for this client				
				ClientSystemRecycleClientSaveFile( 
					pClientSystem, 
					idAppClient,
					UCT_ITEM_COLLECTION,
					idCollection);
				
			}
			break;

		//----------------------------------------------------------------------------
		case APPCMD_SWITCHINSTANCE:
			{
				ASSERT_RETURN(bFake);
				GAMECLIENTID idGameClient = NetClientGetGameClientId(idNetClient);
				ASSERT_BREAK(idGameClient == INVALID_GAMECLIENTID);

				ASSERT_BREAK(NetClientQueryDetached(idNetClient) == FALSE);
				ASSERT_BREAK(NetClientQueryRemoved(idNetClient) == FALSE);

				ASSERT(sPlayerSwitchInstance(game, idNetClient, (MSG_APPCMD_SWITCHINSTANCE *)msg));
				//If switch instance failed, they are booted in sPlayerSwitchInstance.
			}
			break;
			
		//----------------------------------------------------------------------------			
		case CCMD_UNRESERVEGAME:
			{
				ASSERT_RETURN(bFake);
				sCCmdUnreserveGame(game, (MSG_CCMD_UNRESERVEGAME *)msg);
			}
			break;		
			
		//----------------------------------------------------------------------------			
		case CCMD_PLAYERREMOVE:
			{
				GAMECLIENTID idGameClient = NetClientGetGameClientId(idNetClient);
				if (idGameClient != INVALID_GAMECLIENTID)
				{
					goto _process_regular_message;
				}
				else
				{
                    GameServerNetClientRemove( idNetClient );
				}
			}
			break;

		//----------------------------------------------------------------------------
		case APPCMD_CHAT_CHANNEL_CREATED:
			{
				sAppCmdChatChannelCreated(game,NULL,NULL,(BYTE*)msg);
			}
			break;
			
		//----------------------------------------------------------------------------
		case APPCMD_AUTO_PARTY_CREATED:
			{
				sAppCmdAutoPartyCreated(game,NULL,NULL,(BYTE*)msg);
			}
			break;

		//----------------------------------------------------------------------------			
		// these messages are just routed to the game
		case APPCMD_TAKE_OVER_CLIENT:
		//case APPCMD_DELETE_ITEM:
		//case APPCMD_MAIL_SEND_RESULT:
		//case APPCMD_MAIL_TAKE_ATTACHED_MONEY:
		case APPCMD_PLAYERGAG:
		case CCMD_ACCOUNT_STATUS_UPDATE:
		case CCMD_ACCOUNT_BALANCE_UPDATE:		
			{
				ASSERT_RETURN(bFake);
				GAMECLIENTID idGameClient = NetClientGetGameClientId(idNetClient);
				//Process the message even if we have no gameclient.
				GAMECLIENT * client = ClientGetById(game, idGameClient);
				SrvProcessGameMessage(game, client, command, msg);

				break;
			}
			
		//----------------------------------------------------------------------------			
		case APPCMD_RECOVER_LEVEL_CHAT_CHANNELS:
			{
				ASSERT_RETURN(bFake);
				sAppCmdRecoverFromChatCrash(game, NULL, NULL, (BYTE*)msg);
				break;
			}
			
		//----------------------------------------------------------------------------			
		case APPCMD_DONATION_EVENT:
			{
				ASSERT_RETURN(bFake);
				sAppCmdDonationEvent(game, NULL, NULL, (BYTE*)msg);
				break;
			}
			
		//----------------------------------------------------------------------------			
		// TODO: figure out how to use GAME_ROUTE_TO for these messages	
		case APPCMD_GENERATE_RANDOM_ITEM:
		case APPCMD_GENERATE_RANDOM_ITEM_GETBLOB:
		case APPCMD_AH_ERROR:
		case APPCMD_AH_SEARCH_RESULT:
		case APPCMD_AH_SEARCH_RESULT_NEXT:
		case APPCMD_AH_SEARCH_RESULT_ITEM_INFO:
		case APPCMD_AH_SEARCH_RESULT_ITEM_BLOB:
		case APPCMD_AH_PLAYER_ITEM_SALE_LIST:
		case APPCMD_UTIL_GAME_ADD_AH_CLIENT:
		case APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL:
		case APPCMD_UTIL_GAME_EMAIL_METADATA:
		case APPCMD_UTIL_GAME_EMAIL_DATA:
		case APPCMD_UTIL_GAME_EMAIL_ITEM_TRANSFER_RESULT:
			{
				ASSERT_RETURN(bFake);

				const PROCESS_GAME_MESSAGE_STRUCT *pMessageInfo = &gClientGameMessageHandlerTbl[command];
				if (pMessageInfo && pMessageInfo->func) {
					pMessageInfo->func(game, NULL, NULL, (BYTE*)msg);
				}

				break;
			}
		default:
			ASSERT(0);
			break;
		}
	}
	else
	{
_process_regular_message:	
		GAMECLIENTID idClient = NetClientGetGameClientId(idNetClient);
		if (idClient != INVALID_GAMECLIENTID)
		{
			GAMECLIENT * client = ClientGetById(game, idClient);
			if (client)
			{
				if(bFake || MessageTracker::AddClientMessageToTrackerForCCMD(client, GameGetTick(game), command, msg->hdr.size))
				{
					SrvProcessGameMessage(game, client, command, msg);
				}
				else
				{
					TraceWarn(TRACE_FLAG_GAME_NET, "Rejecting net_cmd %d from gameclient %I64x for exceeding permissible rate.  Booting.\n",
						command, ClientGetId(client));
#if !ISVERSION(DEBUG_VERSION)
#if ISVERSION(SERVER_VERSION)
					GameServerContext * pContext = game->pServerContext;
					//Note: if we're being DOSed, it's possible for one player to hit this perf counter many times.
					if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerMessageRateBootCount,1);
#endif
					ClientRemoveFromGame(game, client, 1);
#endif
				}
			}
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SrvGameProcessMessages( 
	GAME * game)
 {
	PERF(SRV_PROCESS_MESSAGES);
	ASSERT(CCMD_LAST == arrsize(gClientGameMessageHandlerTbl));

	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN( serverContext );

	struct SERVER_MAILBOX * pMailbox = 
		GameListGetPlayerToGameMailbox(
			serverContext->m_GameList,
			GameGetId(game) );
	ASSERT_RETURN(pMailbox);

	struct MAILBOX_MSG_LIST * pList = SvrMailboxGetAllMessages(pMailbox);
	ASSERT_RETURN(pList);

	MAILBOX_MSG Msg;

	while( SvrMailboxListHasMoreMessages( pList ) )
	{
		if ( SvrNetPopMailboxListMessage(pList, &Msg) )
		{
			MSG_STRUCT * msg = Msg.GetMsg();
			ASSERT_CONTINUE(msg);

			if(!Msg.IsFakeMessage)
			{
				ASSERT_CONTINUE(gClientGameMessageHandlerTbl[msg->hdr.cmd].eMessageSource == CLIENT_MESSAGE);
#if ISVERSION(SERVER_VERSION)
				PERF_ADD64(serverContext->m_pPerfInstance,GameServer,GameServerMessagesReceivedPerSecond,1);
#endif
			}

			NETCLIENTID64 idClient = Msg.RequestingClient;
	
			if (GameGetState(game) == GAMESTATE_REQUEST_SHUTDOWN)
			{
				break;
			}


			SrvGameProcessMessage(game, idClient, msg, Msg.IsFakeMessage);
		}
	}
	SvrMailboxReleaseMessageList(pMailbox, pList);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SrvProcessAppMessage(
	NETCLIENTID64 idNetClient,
	MSG_STRUCT * msg,
	BOOL bFake = FALSE)
{
	NET_CMD command = msg->hdr.cmd;

	ASSERT_RETURN(command >= 0 && command < CCMD_LAST);

	CLT_VERSION_ONLY(NetTraceRecvMessage(EmbeddedServerRunnerGetCommandTable(USER_SERVER, GAME_SERVER, TRUE), msg, "SRVRCV: "));

	DBG_ASSERT(NetClientValidateIdConsistency(idNetClient));
	const PROCESS_GAME_MESSAGE_STRUCT *pMessageStruct = &gClientGameMessageHandlerTbl[ command ];
	ASSERTX_RETURN( pMessageStruct, "Expected message struct" );

	// for messages that are marked for in game only, ignore them
	if(pMessageStruct->eMCInGame == GAME_YES)
    {
        TraceDebugOnly("SrvProcessAppMessage got ingame command %u from AppClient %d\n",command, NetClientGetAppClientId(idNetClient));
        return;
    }

	// some messages are marked as "route to game", lets auto handle those here
	if (pMessageStruct->eMCInGame == GAME_ROUTE_TO)
	{
		if (idNetClient != INVALID_NETCLIENTID64)
		{
			GAMEID idGame = NetClientGetGameId(idNetClient);
			if (idGame != INVALID_GAMEID)
			{
				SrvPostMessageToMailbox( idGame, idNetClient, command, msg );
			}
		}
		return;	
	}
	
	// some messages are marked as route to the well known utility game
	if (pMessageStruct->eMCInGame == UTIL_GAME)
	{
		ASSERTX( 0, "Should not get here, leaving this assert in to be sure -colin" );
		GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
		if (pServerContext)
		{
			GAMEID idGame = UtilityGameGetGameId( pServerContext );
			if (idGame != INVALID_GAMEID)
			{
				SrvPostMessageToMailbox( idGame, idNetClient, command, msg );
			}
		}
	}
	
	//RJD client command logging code.
//	if(gClientGameMessageHandlerTbl[command].cmd != CCMD_REQMOVE && !bFake) //ignore ReqMove
	{
		//trace( "c_message %2d %d %25s\t%d \n", gClientGameMessageHandlerTbl[command].cmd, -1, gClientGameMessageHandlerTbl[command].szFuncName, msg->hdr.size);
/*		TraceGameInfo( "\n0x");
		for(int i = 0; i <= msg->hdr.size; i+=4)
		{
			TraceGameInfo( "%08x ", *((int *)msg + i/4) );
		}
		TraceGameInfo( "\n");*/
		//log byte code to check matchup.
/*		for(int i = 0; i <= msg->hdr.size; i++)
		{
		trace( "%02x", (unsigned int)*((BYTE*)msg + i) );
		if(i%4 == 3) trace( " ");
		}
		trace( "\n");*/
	}

	switch (command)
	{
	
		//----------------------------------------------------------------------------	
		case CCMD_PLAYERNEW:
		{
			APPCLIENTID idAppClient = NetClientGetAppClientId(idNetClient);
			ASSERT_RETURN(idAppClient != INVALID_CLIENTID);
			sCCmdPlayerNew(idAppClient, msg);
		}
		break;

		//----------------------------------------------------------------------------			
		case CCMD_OPEN_PLAYER_FILE_START:
		{
			APPCLIENTID idAppClient = NetClientGetAppClientId(idNetClient);
			ASSERT_RETURN(idAppClient != INVALID_CLIENTID);
#if ISVERSION(SERVER_VERSION)
			ASSERT_RETURN(bFake);
#endif
			
			MSG_CCMD_OPEN_PLAYER_FILE_START *pStartMessage = (MSG_CCMD_OPEN_PLAYER_FILE_START*)msg;	
			WCHAR *puszCharacterName = pStartMessage->szCharName;
			DWORD dwCRC = pStartMessage->dwCRC;
			DWORD dwSize = pStartMessage->dwSize;
			
			sCCmdPlayerFileInitSend(
				idAppClient, 
				INVALID_GUID,
				PSF_HIERARCHY, 
				INVALID_GUID,
				puszCharacterName, 
				UCT_CHARACTER,
				INVALID_ID,
				PLT_CHARACTER,
				pStartMessage->nDifficultySelected,
				dwCRC, 
				dwSize);
			
		}		
		break;

		//----------------------------------------------------------------------------		
		case CCMD_OPEN_PLAYER_FILE_CHUNK:
		{
			APPCLIENTID idAppClient = NetClientGetAppClientId(idNetClient);
			ASSERT_RETURN(idAppClient != INVALID_CLIENTID);
#if ISVERSION(SERVER_VERSION)
			ASSERT_RETURN(bFake);
#endif

			const MSG_CCMD_OPEN_PLAYER_FILE_CHUNK *pFileChunk = (const MSG_CCMD_OPEN_PLAYER_FILE_CHUNK *)msg;
			const BYTE *pBuffer = pFileChunk->buf;
			int nBufferSize = MSG_GET_BLOB_LEN(pFileChunk, buf);
			sCCmdPlayerSaveFileChunk(idAppClient, pBuffer, nBufferSize, UCT_CHARACTER, INVALID_ID);
			
		}
		break;

		//----------------------------------------------------------------------------	
		case APPCMD_DATABASE_PLAYER_FILE_START:
		{
			ASSERT_RETURN(bFake);

			MSG_APPCMD_DATABASE_PLAYER_FILE_START *pStartMessage = (MSG_APPCMD_DATABASE_PLAYER_FILE_START *)msg;
			WCHAR *puszCharacterName = pStartMessage->szCharName;
			DWORD dwCRC = pStartMessage->dwCRC;
			DWORD dwSize = pStartMessage->dwSize;
			UNIT_COLLECTION_TYPE eCollectionType = (UNIT_COLLECTION_TYPE)pStartMessage->nCollectionType;
			UNIT_COLLECTION_ID idCollection = (UNIT_COLLECTION_ID)pStartMessage->dwCollectionID;
			PGUID guidUnitTreeRoot = pStartMessage->guidUnitTreeRoot;
			PGUID guidDestPlayer = pStartMessage->guidDestPlayer;
			PLAYER_LOAD_TYPE eLoadType = (PLAYER_LOAD_TYPE)pStartMessage->nLoadType;
			
			APPCLIENTID idAppClient = INVALID_CLIENTID;
			if (pStartMessage->idAppClient != INVALID_CLIENTID) {
				idAppClient = pStartMessage->idAppClient;
			} else {
				idAppClient = NetClientGetAppClientId(idNetClient);
			}
			ASSERT_RETURN(idAppClient != INVALID_CLIENTID);

			sCCmdPlayerFileInitSend(
				idAppClient, 
				guidDestPlayer,
				s_DatabaseUnitsAreEnabled() ? PSF_DATABASE : PSF_HIERARCHY, 
				guidUnitTreeRoot,
				puszCharacterName, 
				eCollectionType,
				idCollection,
				eLoadType,
				INVALID_LINK,  // it would be cleaner if this code path was identical to open server
				dwCRC, 
				dwSize);

		}		
		break;

		//----------------------------------------------------------------------------		
		case APPCMD_DATABASE_PLAYER_FILE_CHUNK:
		{
#if ISVERSION(SERVER_VERSION)
			ASSERT_RETURN(bFake);
#endif

			const MSG_APPCMD_DATABASE_PLAYER_FILE_CHUNK *pFileChunk = (const MSG_APPCMD_DATABASE_PLAYER_FILE_CHUNK *)msg;
			const BYTE *pBuffer = pFileChunk->buf;
			int nBufferSize = MSG_GET_BLOB_LEN(pFileChunk, buf);
			UNIT_COLLECTION_TYPE eCollectionType = (UNIT_COLLECTION_TYPE)(pFileChunk->nCollectionType);
			UNIT_COLLECTION_ID idCollection = (UNIT_COLLECTION_ID)(pFileChunk->dwCollectionID);

			APPCLIENTID idAppClient = INVALID_CLIENTID;
			if (pFileChunk->idAppClient != INVALID_CLIENTID) {
				idAppClient = pFileChunk->idAppClient;
			} else {
				idAppClient = NetClientGetAppClientId(idNetClient);
			}
			ASSERT_RETURN(idAppClient != INVALID_CLIENTID);


			sCCmdPlayerSaveFileChunk(
				idAppClient, 
				pBuffer, 
				nBufferSize, 
				eCollectionType,
				idCollection);

		}
		break;

		//----------------------------------------------------------------------------	
		case APPCMD_SWITCHINSTANCE:
		{
			APPCLIENTID idAppClient = NetClientGetAppClientId(idNetClient);
			ASSERT_RETURN(idAppClient != INVALID_CLIENTID);
			ASSERT_RETURN(bFake); //gameserver to self message only.
			sCCmdSwitchInstance(idAppClient, msg);
		}
		break;	

		//----------------------------------------------------------------------------	
		case APPCMD_SWITCHSERVER:
		{
#if ISVERSION(SERVER_VERSION)
			ASSERT_RETURN(bFake); //gameserver to self message only.
			sCCmdSwitchServer(idNetClient, msg);
#else
			ASSERT_RETURN(FALSE);
#endif
		}
		break;	

		//----------------------------------------------------------------------------	
		case CCMD_PLAYERREMOVE:			// need to handle this here too!
		{
			GameServerNetClientRemove( idNetClient );
		}
   		break;
   		
 		//----------------------------------------------------------------------------
 		case APPCMD_PLAYER_GUILD_ACTION_RESULT:
 		{
 			MSG_APPCMD_PLAYER_GUILD_ACTION_RESULT * myMsg = (MSG_APPCMD_PLAYER_GUILD_ACTION_RESULT*)msg;
 			NETCLIENTID64 clientConnectionId = GameServerGetNetClientId64FromAccountId(myMsg->ullPlayerAccountId);
			if (clientConnectionId != INVALID_NETCLIENTID64)
			{
				s_SendGuildActionResult(clientConnectionId, myMsg->wPlayerRequest, myMsg->wActionResult);
				GAMEID idGame = NetClientGetGameId(clientConnectionId);
				if (idGame != INVALID_GAMEID)
				{
					SrvPostMessageToMailbox(idGame, clientConnectionId, APPCMD_PLAYER_GUILD_ACTION_RESULT_GAME, myMsg);
				}
			}
 		}
 		break;
 
 		//----------------------------------------------------------------------------
 		case APPCMD_RECOVER_LEVEL_CHAT_CHANNELS:
 		{
 			GameServerContext * gameSvr = (GameServerContext*)CurrentSvrGetContext();
 			if (!gameSvr) return;
 
 			//	this only happens when a chat server comes back online
			GameListMessageAllGames(
				gameSvr->m_GameList,
				ServiceUserId(USER_SERVER,0,INVALID_SVRCONNECTIONID),
				msg,
				APPCMD_RECOVER_LEVEL_CHAT_CHANNELS);
 		}
 		break;
 
 		//----------------------------------------------------------------------------
 		case APPCMD_PLAYER_GUILD_DATA_APP:
 		{
 			MSG_APPCMD_PLAYER_GUILD_DATA_APP * myMsg = (MSG_APPCMD_PLAYER_GUILD_DATA_APP*)msg;
 			NETCLIENTID64 idNetClient = GameServerGetNetClientId64FromAccountId(myMsg->ullPlayerAccountId);
			if (idNetClient != INVALID_NETCLIENTID64)
			{
 				GAMEID idGame = NetClientGetGameId(idNetClient);
				if (idGame != INVALID_GAMEID)
				{
 					SrvPostMessageToMailbox(idGame, idNetClient, APPCMD_PLAYER_GUILD_DATA_GAME, myMsg);
				}
			}
 		}
 		break;

		//----------------------------------------------------------------------------
		case APPCMD_PARTY_MEMBER_INFO_APP:
		{
			MSG_APPCMD_PARTY_MEMBER_INFO_APP * myMsg = (MSG_APPCMD_PARTY_MEMBER_INFO_APP*)msg;
			NETCLIENTID64 idNetClient = GameServerGetNetClientId64FromAccountId(myMsg->ullPlayerAccountId);
			if (idNetClient != INVALID_NETCLIENTID64)
			{
				GAMEID idGame = NetClientGetGameId(idNetClient);
				if (idGame != INVALID_GAMEID)
				{
					SrvPostMessageToMailbox(idGame, idNetClient, APPCMD_PARTY_MEMBER_INFO_GAME, myMsg);
				}
			}
		}
		break;
			
		//----------------------------------------------------------------------------			
		default:
		{
			ASSERT_RETURN(0);
		}
		
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SrvProcessAppMessages(
	void)
{
	ASSERT(CCMD_LAST == arrsize(gClientGameMessageHandlerTbl));

	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN( serverContext );
	ASSERT_RETURN( serverContext->m_pPlayerToGameServerMailBox );

		struct MAILBOX_MSG_LIST * pList = SvrMailboxGetAllMessages(
							serverContext->m_pPlayerToGameServerMailBox );
		ASSERT_RETURN( pList );

		MAILBOX_MSG servermsg;

		while( SvrMailboxListHasMoreMessages( pList ) )
		{
			if ( SvrNetPopMailboxListMessage( pList, &servermsg ) )
			{
				MSG_STRUCT * msg = servermsg.GetMsg();
				if(!servermsg.IsFakeMessage)
				{
					ASSERT_CONTINUE(gClientGameMessageHandlerTbl[msg->hdr.cmd].eMessageSource == CLIENT_MESSAGE);
#if ISVERSION(SERVER_VERSION)
					PERF_ADD64(serverContext->m_pPerfInstance,GameServer,GameServerMessagesReceivedPerSecond,1);
#endif
				}
				SrvProcessAppMessage(
					servermsg.RequestingClient,
					msg, servermsg.IsFakeMessage );
			}
		}

		SvrMailboxReleaseMessageList(
			serverContext->m_pPlayerToGameServerMailBox,
			pList);
}


void SrvProcessDisconnectedNetclients()
{
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();

	ASSERT_RETURN(serverContext);

	BYTE listnum = serverContext->m_bListSwap ^ 1;

	void * data;

	while ((data = serverContext->m_clientFinishedList[listnum].Pop()) != NULL)
	{
		ASSERT_CONTINUE( ((NETCLT_USER *)data)->m_bRemoveCalled == TRUE);
		serverContext->m_clientFreeList.Free( serverContext->m_pMemPool, (NETCLT_USER *) data);
	}

	serverContext->m_bListSwap = listnum; //I hope this doesn't require a lock...
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void SrvProcessDirtyTownInstanceLists()
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();

	ASSERT_RETURN(serverContext);

	if (AppIsTugboat())
	{
		UINT64 nTownInstanceUpdateTime = TownInstanceListGetLastUpdateTime( serverContext->m_pInstanceList );
		if( serverContext->m_nLastTownInstanceUpdateTimer != 
			nTownInstanceUpdateTime )
		{
			// KCK: Need to critical section this with the other method of sending
			m_csGameInstanceListLock.Enter();

			MSG_SCMD_GAME_INSTANCE_UPDATE_BEGIN msgBegin;
			msgBegin.nInstanceListType = GAME_INSTANCE_TOWN; // KCK: icky, but since Tugboat only tracks towns this is safe.
			ClientSystemSendMessageToAll(AppGetClientSystem(), INVALID_NETCLIENTID64, SCMD_GAME_INSTANCE_UPDATE_BEGIN, &msgBegin);

			GAME_INSTANCE_INFO_VECTOR		InstanceList;
			new (&InstanceList) GAME_INSTANCE_INFO_VECTOR(serverContext->m_pMemPool);
			TownInstanceListGetAllInstances( serverContext->m_pInstanceList, InstanceList );

			MSG_SCMD_GAME_INSTANCE_UPDATE msg;
			for( unsigned int i = 0; i < InstanceList.size(); i++ )
			{
				msg.idGame = InstanceList[i].idGame;
				msg.nInstanceNumber = InstanceList[i].nInstanceNumber;
				msg.idLevelDef = InstanceList[i].nLevelDefinition;
				msg.nInstanceType = InstanceList[i].nInstanceType;
				msg.nPVPWorld = InstanceList[i].nPVPWorld;

				ClientSystemSendMessageToAll(AppGetClientSystem(), INVALID_NETCLIENTID64, SCMD_GAME_INSTANCE_UPDATE, &msg);
			}
			InstanceList.clear();
			InstanceList.~GAME_INSTANCE_INFO_VECTOR();

			MSG_SCMD_GAME_INSTANCE_UPDATE_END msgEnd;
			ClientSystemSendMessageToAll(AppGetClientSystem(), INVALID_NETCLIENTID64, SCMD_GAME_INSTANCE_UPDATE_END, &msgEnd);
			m_csGameInstanceListLock.Leave();

			serverContext->m_nLastTownInstanceUpdateTimer = nTownInstanceUpdateTime;
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SrvValidateMessageTable(
	void )
{
	ASSERT_RETFALSE(arrsize(gClientGameMessageHandlerTbl) == CCMD_LAST);
	for (unsigned int ii = 0; ii < arrsize(gClientGameMessageHandlerTbl); ii++)
	{
		ASSERT_RETFALSE(gClientGameMessageHandlerTbl[ii].cmd == ii);
	}
	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)


//----------------------------------------------------------------------------
//this function seems to be in the wrong file
//----------------------------------------------------------------------------
BOOL AppPostMessageToMailbox(
	NETCLIENTID64 idClient,
	BYTE command,
	MSG_STRUCT * msg)
{
	GameServerContext * gameContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE( gameContext );

	return SvrNetPostFakeClientRequestToMailbox(
				gameContext->m_pPlayerToGameServerMailBox,
				idClient,
				msg,
				command );
}


//----------------------------------------------------------------------------
//this function seems to be in the wrong file
//----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
BOOL SrvPostMessageToMailboxDbg(
	GAMEID idGame,
	NETCLIENTID64 idClient64,
	BYTE command,
	MSG_STRUCT * msg,
	const char * file,
	unsigned int line)
#else
BOOL SrvPostMessageToMailbox(
	GAMEID idGame,
	NETCLIENTID64 idClient64,
	BYTE command,
	MSG_STRUCT * msg)
#endif
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();

	ASSERT_RETFALSE( serverContext );
	struct SERVER_MAILBOX * mailbox = GameListGetPlayerToGameMailbox(
										serverContext->m_GameList,
										idGame );

#if ISVERSION(DEBUG_VERSION)
	if (!mailbox)
	{
		trace("SrvPostMessageToMailbox() invalid gameid called from file:%s  line:%d\n", file, line);
		ASSERT_RETFALSE(FALSE);
	}
#endif

	ASSERT_RETFALSE(mailbox);
	return SvrNetPostFakeClientRequestToMailbox(
				mailbox,
				idClient64,
				msg,
				command );
#else
	return FALSE;
#endif // !ISVERSION( CLIENT_ONLY_VERSION )
}
