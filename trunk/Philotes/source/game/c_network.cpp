//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "units.h"
#include "gameunits.h"
#include "c_network.h"
#include "c_ui.h"
#include "c_unitnew.h"
#include "console.h"
#include "dbunit.h"
#include "effect.h"
#include "unit_priv.h"
#include "unitmodes.h"
#include "missiles.h"
#include "objects.h"
#include "s_message.h"
#include "combat.h"
#include "items.h"
#include "player.h"
#include "inventory.h"
#include "weapons.h"
#include "movement.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_backgroundsounds.h"
#include "skills.h"
#include "ctf.h"
#include "duel.h"
#include "s_network.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "c_camera.h"
#include "states.h"
#include "uix.h"
#include "uix_components_complex.h"
#include "uix_components_hellgate.h"
#include "uix_menus.h"
#include "uix_chat.h"
#include "uix_email.h"
#include "uix_auction.h"
#include "unittag.h"
#include "clients.h"
#include "c_message.h"
#include "pets.h"
#include "performance.h"
#include "npc.h"
#include "windowsmessages.h"
#include "c_tasks.h"
#include "stringtable.h"
#include "quests.h"
#include "monsters.h"
#include "c_trade.h"
#include "uix_money.h"
#include "filepaths.h"
#include "waypoint.h"
#include "e_main.h"
#include "e_settings.h"
#include "e_environment.h"
#include "e_irradiance.h"
#include "teams.h"
#include "c_townportal.h"
#include "perfhier.h"
#include "c_reward.h"
#include "jobs.h"
#include "c_quests.h"
#include "path.h"
#include "c_QuestClient.h"
#include "c_GameMapsClient.h"
#include "c_recipe.h"
#include "cube.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "c_LevelAreasClient.h"
#include "chat.h"
#include "Currency.h"
#include "room_path.h"
#if !ISVERSION(SERVER_VERSION)
#include "mailslot.h"
#include "svrstd.h"
#include "EmbeddedServerRunner.h"
#include "UserChatCommunication.h"
#include "c_chatNetwork.h"
#include "interact.h"
#include "quest_wormhunt.h"
#include "c_itemupgrade.h"
#include "statssave.h"
#include "hireling.h"
#include "ServerSuite/BillingProxy/c_BillingClient.h" 
#include "warp.h"
#include "c_credits.h"
#include "settings.h"
#include "settingsexchange.h"
#include "ServerSuite/Chat/ChatServer.h"
#include "GameChatCommunication.h"
#include "wordfilter.h"
#include "global_themes.h"
#include "serverlist.h"
#include "uix_crafting.h"
#include "c_playerEmail.h"
#include "stringreplacement.h"


//----------------------------------------------------------------------------
// DEBUGGING CONSTANTS
//----------------------------------------------------------------------------
#define DEBUG_MESSAGE_TRACE				0


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct SCMD_ERROR_MSG
{
	BYTE		bErrorCode;
	GLOBAL_STRING eTitle;
	GLOBAL_STRING eMessage;
};


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
static const SCMD_ERROR_MSG gNetErrors[] =
{	// UNKNOWN must be first error in this list!!!

	{	LOGINERROR_UNKNOWN,				GS_NET_ERROR_TITLE,			GS_NET_ERROR_UNKNOWN					},		
	{	LOGINERROR_INVALIDNAME,			GS_NET_ERROR_LOGIN_TITLE,	GS_NET_ERROR_INVALID_CHARACTER_NAME		},
	{	LOGINERROR_NAMEEXISTS,			GS_NET_ERROR_LOGIN_TITLE,	GS_NET_ERROR_DUPLICATE_NAME				},
	{	LOGINERROR_ALREADYONLINE,		GS_NET_ERROR_LOGIN_TITLE,	GS_NET_ERROR_CHARACTER_ALREADY_IN_GAME	},
	{	LOGINERROR_PLAYERNOTFOUND,		GS_NET_ERROR_LOGIN_TITLE,	GS_NET_ERROR_CHARACTER_NOT_FOUND		},
};

//----------------------------------------------------------------------------
// STATICS
//----------------------------------------------------------------------------
static int snPingTime;
static MSG_SCMD_PING sMsgPing;
static int snServerTimeDiffEstimate;
static int snMinRoundtripEstimator = 100000;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCltPostMessageToMailbox(
	BYTE command,
	MSG_STRUCT * msg)
{
	MAILSLOT * mailslot = AppGetGameMailSlot();
	ASSERT_RETFALSE(mailslot);

	BYTE buf_data[MAX_SMALL_MSG_STRUCT_SIZE];
	BYTE_BUF_NET bbuf(buf_data, MAX_SMALL_MSG_STRUCT_SIZE);
	unsigned int size;

	ASSERT_RETFALSE(NetTranslateMessageForSend(gApp.m_GameServerCommandTable, command, msg, size, bbuf));

	MSG_BUF * msgbuf = MailSlotGetBuf(mailslot);
	ASSERT_RETFALSE(msgbuf);
	msgbuf->size = size;
	memcpy(msgbuf->msg, buf_data, size);
	MailSlotAddMsg(mailslot, msgbuf);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetClientId(
	GAMECLIENTID idClient)
{
	gApp.m_idRemoteClient = idClient;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SetPing(int nPingTime)
{
	snPingTime = nPingTime;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_GetPing()
{
	return snPingTime;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MSG_SCMD_PING c_GetPingMsg()
{
	return sMsgPing;
}
//----------------------------------------------------------------------------
// Looking at the latest ping, we update our estimate of the server's time
// desync with us.  A zeroish ping will give us a fairly exact number.
//
// This allows us to determine whether the lag is client-server or
// server-client.
//
// Note: clock granularity epsilons, since we're all using the cheap clock()
//----------------------------------------------------------------------------
#define CLOCK_EPSILON 15
static void sEstimateServerClock(int timeSend, int timeReceivedByServer,
								 int timeDiff)
{
	//If we are the lowest ping, or the server time estimate is out
	//of bounds, just use this ping.
	if(timeDiff < snMinRoundtripEstimator ||
		(timeReceivedByServer - snServerTimeDiffEstimate < timeSend - CLOCK_EPSILON
		|| timeReceivedByServer - snServerTimeDiffEstimate > timeSend + timeDiff + CLOCK_EPSILON) )
	{//Assume the server response is midway in the middle.
		snServerTimeDiffEstimate = timeReceivedByServer - timeSend - (timeDiff/2);
		snMinRoundtripEstimator = timeDiff;
		return;
	}
	if(timeDiff == snMinRoundtripEstimator)
	{//Take a running average.
		int nDiffEstimate = timeReceivedByServer - timeSend - (timeDiff/2);
		snServerTimeDiffEstimate = 
			int((3.0*snServerTimeDiffEstimate + 1.0*nDiffEstimate)/4.0);
	}

}
//----------------------------------------------------------------------------
// If it's an immediate (nothing is stored in field 1) put the time in field
// one, trace the immediate ping, store it in the static for the UI to read 
// off, and post it to the mailbox.  If the ping time is low, use it to make
// a guess as to the server time difference.  If it's a mailbox (stuff in
// field 1 but not 2) trace the mailbox ping and store the message in the
// the static for the UI to read off.
//----------------------------------------------------------------------------
void c_HandlePing(MSG_SCMD_PING *msg)
{
	ASSERT_RETURN(msg);
	if(msg->timeStamp1 == INVALID_ID)
	{
		msg->timeStamp1 = (int)GetRealClockTime() - msg->timeOfSend;
		//trace("Round trip ping-immediate of %d milliseconds from server.\n", msg->timeStamp1);
		sEstimateServerClock(msg->timeOfSend, msg->timeOfReceive, msg->timeStamp1);
		msg->timeOfReceive -= snServerTimeDiffEstimate + msg->timeOfSend;
		//trace("Estimated clockdiff: %d ms.  Server process estimate: %d ms\n", snServerTimeDiffEstimate, msg->timeOfReceive);
		c_SetPing(msg->timeStamp1);
		sCltPostMessageToMailbox(SCMD_PING, msg);
	}
	else
	{
		msg->timeStamp2 = (int)GetRealClockTime() - msg->timeOfSend;
		//trace("Round trip ping-processed of %d milliseconds from server.\n", msg->timeStamp2);
		sMsgPing = *msg;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENTID AppGetClientId(
	void)
{
	return gApp.m_idRemoteClient;
}

//----------------------------------------------------------------------------
// Ping the server to track latency.
// The server will boot us if we do this too often :)
//----------------------------------------------------------------------------
static BOOL sPingServer(
	GAME * game,
	UNIT *,
	const EVENT_DATA &)
{
	ASSERT_RETFALSE(IS_CLIENT(game));
	MSG_CCMD_PING msg;
	msg.timeOfSend = GetRealClockTime();
	msg.bIsReply = FALSE;
	c_SendMessage(CCMD_PING, &msg);

	GameEventRegister(game, sPingServer, NULL, NULL, NULL, CLIENT_PING_INTERVAL);
	return TRUE;
}
//----------------------------------------------------------------------------
// All commands from server to client get processed here
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdYourId(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_YOURID* msg = (MSG_SCMD_YOURID*)data;
	
	if (AppGetState() == APP_STATE_CONNECTING) 
	{
		const WCHAR *puszString = GlobalStringGet( GS_LOADING );
		AppSwitchState(APP_STATE_LOADING, puszString, L"");
	}
	AppSetClientId((GAMECLIENTID)msg->id);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdError(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_ERROR * msg = (MSG_SCMD_ERROR *)data;

	const SCMD_ERROR_MSG *pErrorMsg = NULL;
	for (unsigned int ii = 0; ii < arrsize(gNetErrors); ++ii)
	{
		if (msg->bError == gNetErrors[ii].bErrorCode)
		{
			pErrorMsg = &gNetErrors[ ii ];
			break;
		}
	}
    if(!pErrorMsg) {
        ASSERTX( pErrorMsg, "Error message not found" );
        pErrorMsg = &gNetErrors[LOGINERROR_UNKNOWN];
    }
	
	AppSetErrorState(pErrorMsg->eTitle, pErrorMsg->eMessage);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdGameNew(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_GAME_NEW * msg = (MSG_SCMD_GAME_NEW *)data;
	ASSERTX_DO(AppGetCltGame() == NULL, "Client already has a game")
	{
		extern BOOL AppRemoveCltGame();
		AppRemoveCltGame();
	}

	AppInitCltGame(
		msg->idGame, 
		msg->dwCurrentTick, 
		(GAME_SUBTYPE)msg->bGameSubType, 
		msg->dwGameFlags,
		&msg->tGameVariant); 
		
	GameEventRegister(AppGetCltGame(), sPingServer, NULL, NULL, NULL, CLIENT_PING_INTERVAL);
	UIShowLoadingScreen(1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSetLevelCommChannel(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL * msg = (MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL*)data;
	gApp.m_idLevelChatChannel = msg->idChannel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTeamNew(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TEAM_NEW * msg = (MSG_SCMD_TEAM_NEW *)data;

	TeamReceiveFromServer(game, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTeamDeactivate(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TEAM_DEACTIVATE * msg = (MSG_SCMD_TEAM_DEACTIVATE *)data;

	TeamDeactivateReceiveFromServer(game, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTeamJoin(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TEAM_JOIN * msg = (MSG_SCMD_TEAM_JOIN *)data;

	TeamJoinReceiveFromServer(game, msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdGameClose(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_GAME_CLOSE *pMessage = (MSG_SCMD_GAME_CLOSE *)pData;

	BOOL bShowCredits = pMessage->bShowCredits;
	BOOL bRestartingGame = pMessage->bRestartingGame;
	GAMEID idGame = pMessage->idGame;
	
	if (pGame == NULL)
	{
		return;
	}
	ASSERTX_RETURN(idGame == GameGetId(pGame), "Received game close for game we are not in!");

	if ( bShowCredits )
	{
		c_CreditsSetAutoEnter( TRUE );
		UIGameRestart();
	}
	c_CameraRestoreViewType();
	e_SetUIOnlyRender( TRUE );
	
	// we won't show the loading screen inbetween the level and the auto entering of
	// the credits cause it shows for a split second and looks silly
	if (bShowCredits == FALSE)
	{
		UIShowLoadingScreen(0);
		UIUpdateLoadingScreen();
	}
	
	GameSetState(AppGetCltGame(), GAMESTATE_REQUEST_SHUTDOWN);
	GameSetGameFlag(pGame, GAMEFLAG_RESTARTING, bRestartingGame);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdOpenPlayerInitSend(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_OPEN_PLAYER_INITSEND * msg = (MSG_SCMD_OPEN_PLAYER_INITSEND *)data;

	if (gApp.m_SaveFile)
	{
		FREE(NULL, gApp.m_SaveFile);
		gApp.m_SaveFile = 0;
	}
	if (msg->dwSize == 0 || msg->dwSize > UNITFILE_MAX_SIZE_ALL_UNITS)
	{
		return;
	}
	gApp.m_SaveCur = 0;
	gApp.m_SaveCRC = msg->dwCRC;
	gApp.m_SaveSize = msg->dwSize;
	gApp.m_SaveFile = (BYTE *)MALLOC(NULL, gApp.m_SaveSize);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void JobPlayerSaveByteBufferToFile(JOB_DATA & data)
{
	PlayerSaveByteBufferToFile((WCHAR *)data.data1, (BYTE *)data.data2, (DWORD)data.data3);
}

void sSCmdOpenPlayerSendFile(
	 GAME * game,
	 BYTE * data)
{
	MSG_SCMD_OPEN_PLAYER_SENDFILE * msg = (MSG_SCMD_OPEN_PLAYER_SENDFILE *)data;
	
	ASSERT_RETURN(gApp.m_SaveFile);
	ASSERT_RETURN(gApp.m_SaveSize > 0);
	unsigned int buflen = MSG_GET_BLOB_LEN(msg, buf);
	ASSERT_RETURN(buflen <= MAX_PLAYERSAVE_BUFFER);
	ASSERT_RETURN(gApp.m_SaveCur + buflen <= gApp.m_SaveSize);
	memcpy(gApp.m_SaveFile + gApp.m_SaveCur, msg->buf, buflen);
	gApp.m_SaveCur += buflen;

	if (gApp.m_SaveCur < gApp.m_SaveSize)
	{
		return;
	}
	
	DWORD crc = CRC(0, gApp.m_SaveFile, gApp.m_SaveSize);
	ASSERT_RETURN(crc == gApp.m_SaveCRC);
	
	UNIT * unit = GameGetControlUnit(game);
	ASSERT_RETURN(unit);
	
	// save the file
	WCHAR * szUnitName = (WCHAR *)MALLOC(NULL, MAX_CHARACTER_NAME*sizeof(WCHAR));
	ASSERT_DO(szUnitName != NULL)
	{
		gApp.m_SaveCur = 0;
		gApp.m_SaveCRC = 0;
		gApp.m_SaveSize = 0;
		FREE(NULL, gApp.m_SaveFile);
		gApp.m_SaveFile = 0;

		FREE(NULL, szUnitName);
	}

	PStrCopy(szUnitName, unit->szName, MAX_CHARACTER_NAME);

	JOB_DATA saveData( (DWORD_PTR)szUnitName, (DWORD_PTR)gApp.m_SaveFile, gApp.m_SaveSize);
	c_JobSubmit(JOB_TYPE_BIG, JobPlayerSaveByteBufferToFile, saveData); //even small writes take a lot of time, n'est-ce pas?

	gApp.m_SaveCur = 0;
	gApp.m_SaveCRC = 0;
	gApp.m_SaveSize = 0;
	gApp.m_SaveFile = 0;

	return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdStartLevelLoad(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_START_LEVEL_LOAD* msg = (MSG_SCMD_START_LEVEL_LOAD*)data;
	ASSERT(AppGetCltGame() != NULL);

/*	
	UNIT * player = GameGetControlUnit( game );
	if ( player )
	{
		UnitSetFlag( player, UNITFLAG_JUSTWARPEDIN );
	}
*/

	const WCHAR * puszLevelName = NULL;
	const WCHAR * puszDRLGName = NULL;

	// rjd - level definition may not be known to client switching gameservers
	if(AppIsHellgate())
	{
		if (msg->nLevelDefinition != INVALID_LINK)
		{
			const LEVEL_DEFINITION * level_definition = LevelDefinitionGet(msg->nLevelDefinition);
			ASSERT_RETURN(level_definition);
			puszLevelName = StringTableGetStringByIndex(level_definition->nLevelDisplayNameStringIndex);
		}
	}
	
	
	// phu - for now, when switching instances we don't know what drlg we're gonna get in the next instance
	if( AppIsHellgate() )
	{
		if (msg->nDRLGDefinition != INVALID_LINK)
		{
			const DRLG_DEFINITION * pDRLGDefinition = DRLGDefinitionGet(msg->nDRLGDefinition);
			ASSERT_RETURN(pDRLGDefinition);
			if (pDRLGDefinition->nDRLGDisplayNameStringIndex != INVALID_LINK)
			{
				puszDRLGName = StringTableGetStringByIndex(pDRLGDefinition->nDRLGDisplayNameStringIndex);
			}
		}
	}
	
	WCHAR szBuf[1024];
	if(AppIsTugboat())
	{
		UIShowQuickMessage( "" );
		if( GameGetControlUnit( game ) )
		{

											  
			UnitTriggerEvent( game, EVENT_WARPED_LEVELS, GameGetControlUnit( game ), NULL, NULL );
		}

		WCHAR uszSubName[ 1024 ] = { 0 };
	
		BOOL bHasSubName = FALSE;

		bHasSubName = LevelDepthGetDisplayName( 
							game,
							msg->nLevelArea,
							msg->nLevelDefinition,
							msg->nLevelDepth, 
							uszSubName, 
							1024,
							FALSE );

		if( msg->nLevelArea == -1 )
		{

			PStrPrintf(szBuf, 1024, GlobalStringGet( GS_LOADING ) );
		}
		else
		{			
			WCHAR insertString[ 1024 ];
			MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( msg->nLevelArea, msg->nLevelDepth, &insertString[0], 1024, FALSE );

			if( !insertString[0] )
			{
				PStrPrintf(szBuf, 1024, GlobalStringGet( GS_LOADING ) );
			}
			else
			{
				PStrPrintf(szBuf, 1024, L"%s", &insertString[0] );
				if (bHasSubName)
				{
					PStrCat( szBuf, L" - ", 1024 );
					PStrCat( szBuf, uszSubName, 1024 );
				}
			}
			
		}
		
		puszLevelName = szBuf;
	}

	//e_SetRenderEnabled(FALSE);

	//e_DisableVisibleRender();
	//// burn a few frames to cover all restores and loads, then reset device again
	//EVENT_DATA tData;
	//GameEventRegister(game, e_EnableVisibleRender, NULL, NULL, &tData, 5);
	
	// stop moving
	c_PlayerClearAllMovement(game);

	// depends on game time... might be switching games
	c_CameraShakeOFF();

	// Make sure any popups are closed
	UICloseAll(TRUE, TRUE);

	AppSwitchState( APP_STATE_LOADING, puszLevelName, puszDRLGName);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSetLevel(
	GAME* game,
	BYTE* data)
{
	ASSERT_RETURN(game);
	MSG_SCMD_SET_LEVEL* msg = (MSG_SCMD_SET_LEVEL*)data;

	c_LevelSetAndLoad(game, msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSetControlUnit(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_SET_CONTROL_UNIT * msg = (MSG_SCMD_SET_CONTROL_UNIT *)data;

	UNIT * unit = UnitGetById(game, msg->id);
	if (!unit)
		AppSetErrorState(GS_ERROR, GS_NET_ERROR_CHARACTER_NOT_FOUND);
	// UnitSetClientID(pUnit, AppGetClientId());	// client doesn't have a GAMECLIENT, so why set the id?
	GameSetControlUnit(game, unit);
	ASSERT(UnitSetGuid(unit, msg->guid));
	ROOM * pUnitRoom = UnitGetRoom(unit);
	if (pUnitRoom)
	{
		const ROOM_INDEX * pRoomIndex = RoomGetRoomIndex(game, pUnitRoom);
		c_SoundSetEnvironment(pRoomIndex->pReverbDefinition);
		c_BGSoundsPlay(pRoomIndex->nBackgroundSound);
	}
	else
	{
		c_SoundSetEnvironment(NULL);
		c_BGSoundsPlay(INVALID_ID);
	}

	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		c_StateSet(unit, unit, STATE_SINGLE_PLAYER, 0, 0, INVALID_ID);
	}
	else
	{
		c_StateSet(unit, unit, STATE_MULTI_PLAYER, 0, 0, INVALID_ID);
	}

	UISendMessage(WM_SETCONTROLUNIT, UnitGetId(unit), 0);

	if( AppIsTugboat() )
	{
		c_CameraSetViewType( c_CameraGetViewType() );
	}

	if (!AsyncFileIsAsyncRead()) 
	{
		// tell the server that we're totally loaded now
		MSG_CCMD_LEVELLOADED msg;
		c_SendMessage(CCMD_LEVELLOADED, &msg);
		
		// triger preloaded event
		UnitTriggerEvent(game, EVENT_PRELOADED, unit, NULL, NULL);
		
		AppSwitchState(APP_STATE_IN_GAME);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdControlUnitFinished(
	GAME * game,
	BYTE * data)
{
	ASSERT_RETURN(IS_CLIENT(game));
	MSG_SCMD_SET_CONTROL_UNIT * msg = (MSG_SCMD_SET_CONTROL_UNIT *)data;

	UNIT * pControlUnit = GameGetControlUnit(game);
	ASSERT_RETURN(UnitGetId(pControlUnit) == msg->id);

	ControlUnitLoadInventoryUnitData(pControlUnit);
	c_UnitFlagSoundsForLoad(pControlUnit);
	c_SoundUnloadAtLevelChange(TRUE);
	c_UnitDataAllUnflagSoundsForLoad(game);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPlayerNew(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_PLAYERNEW * msg = (MSG_SCMD_PLAYERNEW *)data;
	REF(msg);

	ASSERT(0);		// need to link other player's clients with their unit???
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPlayerRemove(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_PLAYERREMOVE* msg = (MSG_SCMD_PLAYERREMOVE*)data;

	if (AppGetClientId() == (GAMECLIENTID)msg->id)
	{
		if(AppGetType() == APP_TYPE_CLOSED_CLIENT)
			AppSwitchState(APP_STATE_RESTART);
		else
			AppSwitchState(APP_STATE_EXIT);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdMissileNewXYZ(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_MISSILENEWXYZ* msg = (MSG_SCMD_MISSILENEWXYZ *)data;

	ROOM * room = RoomGetByID(game, msg->idRoom);
	ASSERT_RETURN(room);

	UNIT * pSource = UnitGetById(game, msg->idSource);
	ASSERT_RETURN( MAX_WEAPONS_PER_UNIT == 2 );
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	pWeapons[ 0 ] = UnitGetById(game, msg->idWeapon0);
	pWeapons[ 1 ] = UnitGetById(game, msg->idWeapon1);

	ASSERT(sizeof(msg->nSkill) == sizeof(WORD));
	int nSkill = (msg->nSkill == 0xffff ? INVALID_ID : (int)msg->nSkill);
	UNIT * pMissile = 
		MissileInit(game, msg->nClass, msg->id, pSource, pWeapons, nSkill, msg->nTeam, room, 
					msg->position, msg->MoveDirection, msg->MoveDirection );
	if(pMissile)
	{
		const UNIT_DATA * pUnitData = UnitGetData(pMissile);
		ASSERT_RETURN(pUnitData);
		if(UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_XFER_MISSILE_STATS))
		{
			BIT_BUF tBuffer(msg->tBuffer, MAX_MISSILE_STATS_SIZE);
			XFER<XFER_LOAD> tXfer(&tBuffer);
			StatsXfer(game, UnitGetStats(pMissile), tXfer, STATS_WRITE_METHOD_NETWORK);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdMissileNewOnUnit(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_MISSILENEWONUNIT* msg = (MSG_SCMD_MISSILENEWONUNIT*)data;

	ROOM* room = RoomGetByID(game, msg->idRoom);
	ASSERT_RETURN(room);

	UNIT * pSource = UnitGetById( game, msg->idSource );
	UNIT * pTarget = UnitGetById( game, msg->idTarget );

	ASSERT_RETURN( MAX_WEAPONS_PER_UNIT == 2 );
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	pWeapons[ 0 ] = UnitGetById(game, msg->idWeapon0);
	pWeapons[ 1 ] = UnitGetById(game, msg->idWeapon1);

	MissileInitOnUnit(game, msg->nClass, msg->id, 
					  pSource, pWeapons, msg->nSkill, pTarget, 
					  msg->nTeam, room );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUpdate(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UPDATE* msg = (MSG_SCMD_UPDATE*)data;
	REF(msg);

	// phu- pause if the game tick is too far off?
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitTurnId(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITTURNID* msg = (MSG_SCMD_UNITTURNID*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}
	UNIT* target = UnitGetById(game, msg->targetid);
	if (!target)
	{
		return;
	}

	BOOL bImmediate = TESTBIT(&msg->bFlags, UT_IMMEDIATE);
	BOOL bFaceAway = TESTBIT(&msg->bFlags, UT_FACEAWAY);

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		UnitUpdateWeaponPosAndDirecton( game, unit, i, target, TRUE ); 

	VECTOR vTarget = c_UnitGetPosition(target);
	if(AppIsHellgate())
	{
		// facing you?
		if (target == GameGetControlUnit(game) && !AppGetDetachedCamera())
		{
			const CAMERA_INFO* camera = c_CameraGetInfo(TRUE);
			if (camera)
			{
				VECTOR vCameraPosition = CameraInfoGetPosition(camera);
				vTarget.fX = vCameraPosition.fX;
				vTarget.fY = vCameraPosition.fY;
			}
		}
	}

	VECTOR vDelta = vTarget - UnitGetPosition(unit);
	if(bFaceAway)
	{
		vDelta = -vDelta;
	}

	vDelta.fZ = 0.0f;
	VECTOR vDirection;
	if (VectorIsZero(vDelta))
	{
		vDirection = VECTOR(0, 1, 0);
	}
	else 
	{
		VectorNormalize(vDirection, vDelta);
	}
	if ( AppIsTugboat() )
	{
		UnitSetQueuedFaceDirection( unit, vDirection );
	}
	UnitSetFaceDirection(unit, vDirection, bImmediate);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitTurnXYZ(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_UNITTURNXYZ * msg = (MSG_SCMD_UNITTURNXYZ *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	if ( AppIsTugboat() )
	{
		UnitSetQueuedFaceDirection( unit, msg->vFaceDirection );
	}
	UnitSetUpDirection( unit, msg->vUpDirection );
	UnitSetFaceDirection( unit, msg->vFaceDirection, msg->bImmediate );
	unit->pvWeaponDir[ 0 ] = msg->vFaceDirection;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitDie(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UNITDIE * msg = (MSG_SCMD_UNITDIE *)data;

	UNIT * unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	UNIT * killer = UnitGetById(game, msg->idKiller);

	BOOL bDefenderIsADestructible = UnitIsA(unit, UNITTYPE_DESTRUCTIBLE);
	UNIT * pControlUnit = GameGetControlUnit(game);

	UnitTriggerEvent(game, EVENT_GOTKILLED, unit, killer, NULL);
	if (killer)
	{
		do
		{
			if (!UnitIsA(killer, UNITTYPE_MISSILE))
			{
				UnitTriggerEvent(game, EVENT_DIDKILL, killer, unit, NULL);
			}
			if (!bDefenderIsADestructible)
			{
				if(pControlUnit == killer)
				{
					UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(killer, TAG_SELECTOR_MONSTERS_KILLED);
					if (pTag)
					{
						pTag->m_nCounts[pTag->m_nCurrent]++;
					}
				}
				if(UnitIsA(killer, UNITTYPE_PLAYER))
				{
					UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(killer, TAG_SELECTOR_MONSTERS_KILLED_TEAM);
					if (pTag)
					{
						pTag->m_nCounts[pTag->m_nCurrent]++;
					}
				}
			}
			
			UNIT * killer_owner = UnitGetOwnerUnit(killer);
			if (!killer_owner || killer_owner == killer)
			{
				break;
			}
			killer = killer_owner;
		} while (TRUE);
	}

	UnitDie(unit, killer);

	if (UnitIsA(unit, UNITTYPE_PLAYER))
	{
		UISendMessage(WM_PLAYERDEATH, UnitGetId(unit), 0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRoomAdd(
	GAME* game,
	BYTE* data)
{
	TIMER_START("sSCmdRoomAdd");
	MSG_SCMD_ROOM_ADD* msg = (MSG_SCMD_ROOM_ADD*)data;

	ASSERTX(msg->idRoomDef != INVALID_LINK, "Room is not in room_index.txt");

	// for now, do nothing if we know about this room already ... TODO: change this, but note that
	// the gfx needs to be able to quickly build light maps and stuff for rooms as they are
	// streamed to you ... at present, we sent all rooms at the beginning of the level
	ROOMID idRoom = msg->idRoom;
	ROOM *pRoom = RoomGetByID( game, idRoom );
	if (pRoom != NULL)
	{
		if (gbClientsHaveVisualRoomsForEntireLevel == FALSE)
		{
			const int MAX_MESSAGE = 1024;
			char szMessage[ MAX_MESSAGE ];
			PStrPrintf( 
				szMessage,
				MAX_MESSAGE,
				"CLIENT: Received room add for room we already have '%s' [%d]\n",
				RoomGetDevName( pRoom ),
				RoomGetId( pRoom ));
			ASSERTX( 0, szMessage );
		}
	}
	else
	{
		LEVEL * level = LevelGetByID(game, AppGetCurrentLevel());
		ASSERT_RETURN(level);
		pRoom = RoomAdd(game, level, msg->idRoomDef, &msg->mTranslation, TRUE, msg->dwRoomSeed, idRoom, msg->idSubLevel, msg->szLayoutOverride);
		//marsh is testing streaming..
		if( AppIsTugboat() &&
			level->m_pLevelDefinition->bClientDiscardRooms)
		{
			RoomDoSetup(game, level, pRoom );
			RoomCreatePathEdgeNodeConnections(game, pRoom, FALSE);
		}
	}

	// debugging
	if (pRoom)
	{
		if (msg->bKnownOnServer)
		{
			RoomSetFlag( pRoom, ROOM_KNOWN_BIT );
		}
	}
	
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRoomAddCompressed(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_ROOM_ADD_COMPRESSED * compressed = (MSG_SCMD_ROOM_ADD_COMPRESSED *)data;
	MSG_SCMD_ROOM_ADD original;
	UncompressRoomMessage(original, *compressed);
	sSCmdRoomAdd(game, (BYTE *)(&original));
}

//---------------------------------------------------------------f-------------
//----------------------------------------------------------------------------
void sSCmdRoomUnitsAdded(
	GAME* game,
	BYTE* data)
{
	TIMER_START("sSCmdRoomUnitsAdded");
	MSG_SCMD_ROOM_UNITS_ADDED* msg = (MSG_SCMD_ROOM_UNITS_ADDED*)data;

	// for now, do nothing if we know about this room already ... TODO: change this, but note that
	// the gfx needs to be able to quickly build light maps and stuff for rooms as they are
	// streamed to you ... at present, we sent all rooms at the beginning of the level
	ROOMID idRoom = msg->idRoom;
	ROOM *pRoom = RoomGetByID( game, idRoom );

	if (pRoom != NULL)
	{
		c_TaskUpdateNPCs( game, pRoom );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRoomRemove(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_ROOM_REMOVE *pMessage = (MSG_SCMD_ROOM_REMOVE *)pData;
	ROOMID idRoom = pMessage->idRoom;	
	ASSERTX_RETURN( idRoom != INVALID_ID, "The server told us about an invalid room" );

	// get the room
	ROOM *pRoom = RoomGetByID( pGame, idRoom );
	if (pRoom == NULL)
	{
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE, 
			"CLIENT: Can't remove unknown room '%d'\n",
			idRoom );
		ASSERTX_RETURN( 0, szMessage );
	}
	// get the level the room is on
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETURN( pLevel, "Can't remove room that isn't on a level" );
	// we only remove if we really allow you to forget about a room in a level
	if (gbClientsHaveVisualRoomsForEntireLevel == FALSE ||
		pLevel->m_pLevelDefinition->bClientDiscardRooms )
	{
				
		// remove room from the level
		LevelRemoveRoom( pGame, pLevel, pRoom );
		
		// free the room
		RoomFree(pGame, pRoom, FALSE);
	}
	else
	{
	
		// OK, we have visual rooms for the whole level, but this room 
		// is no longer officially known to us as far as the server knows
		RoomSetFlag( pRoom, ROOM_KNOWN_BIT, FALSE );

	}
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdLoadComplete(
	GAME *pGame, 
	BYTE *pData)
{
	MSG_SCMD_LOADCOMPLETE *pMessage = (MSG_SCMD_LOADCOMPLETE *)pData;
	ASSERT( IS_CLIENT( pGame ) );
	c_LevelLoadComplete( pMessage->nLevelDefinition, pMessage->nDRLGDefinition, pMessage->nLevelId, pMessage->nLevelDepth );	
	c_PlayerRefreshPartyUI();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sHotkeyUpdatePickupItem(
	GAME* game,
	UNIT* item)
{
	if (!item)
	{
		return;
	}
	UNIT* unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}

	for (int ii = 0; ii <= TAG_SELECTOR_HOTKEY12; ii++)
	{
		UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag(unit, ii);
		if (!tag || tag->m_nLastUnittype <= INVALID_LINK)
		{
			continue;
		}
		if (tag->m_idItem[0] != INVALID_ID &&	UnitGetById(game, tag->m_idItem[0]) != NULL)
		{
			continue;
		}
		if (tag->m_idItem[1] != INVALID_ID &&	UnitGetById(game, tag->m_idItem[1]) != NULL)
		{
			continue;
		}
		if (tag->m_nSkillID[0] != INVALID_ID || tag->m_nSkillID[1] != INVALID_ID)
		{
			continue;
		}

		if (UnitIsA(item, tag->m_nLastUnittype))
		{
			MSG_CCMD_HOTKEY msg;
			msg.idItem[0] = UnitGetId(item);
			msg.idItem[1] = INVALID_ID;
			msg.bHotkey = (BYTE)ii;
			msg.nSkillID[0] = tag->m_nSkillID[0];
			msg.nSkillID[1] = tag->m_nSkillID[1];
			c_SendMessage(CCMD_HOTKEY, &msg);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitDamaged(
	 GAME* game,
	 BYTE* data)
{
	MSG_SCMD_UNIT_DAMAGED* msg = (MSG_SCMD_UNIT_DAMAGED*)data;

	UNIT* pDefender = UnitGetById( game, msg->idDefender );
	if (!pDefender)
	{
		return;
	}
	UNIT* pAttacker = UnitGetById( game, msg->idAttacker );


	if(!IsUnitDeadOrDying(pDefender))
	{
		UnitSetStat( pDefender, STATS_HP_CUR, msg->nCurHp );
	}

	if( AppIsTugboat() && msg->bIsMelee && pAttacker && UnitGetUltimateOwner( pAttacker ) == GameGetControlUnit( game ) )
	{
		if( msg->wHitState != COMBAT_RESULT_FUMBLE &&
			msg->wHitState != COMBAT_RESULT_BLOCK )
		{
			if( UnitHasState( game, pDefender, STATE_HAS_GETHIT_PARTICLES ) )
			{
				return;
			}
		}

	}
	c_UnitHitUnit( game, msg->idAttacker, msg->idDefender, msg->bResult, msg->wHitState, (BOOL)msg->bIsMelee );

	// Unit took damage... check if the unit is the player and needs get hit UI
	if (IS_CLIENT(game) && GameGetControlUnit(game) == pDefender )
	{
		UISendMessage(WM_CONTROLUNITGETHIT, msg->nSourceX, msg->nSourceY);
		if( AppIsHellgate() )
		{
			// shake the camera too
			VECTOR vDirection;
			vDirection.fX = pDefender->vPosition.fX - ( (float)msg->nSourceX / 1000.0f );
			vDirection.fY = pDefender->vPosition.fY - ( (float)msg->nSourceY / 1000.0f );
			vDirection.fZ = 0.0f;
			c_CameraShake(game,vDirection);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsChangeMessageTrace(
	GAME * game,
	UNIT * unit,
	WORD wStat,
	DWORD dwParam,
	int nNewValue,
	const char * pszMessageName)
{
#if DEBUG_MESSAGE_TRACE
	const STATS_DATA * stats_data = StatsGetData(game, wStat);
	if (StatsDataTestFlag(stats_data, STATS_DATA_FLOAT))
	{
		trace("[%5d]  %s:  unit [%d: %s]   stat [%s]   param [%d]   value [%.2f]\n", 
			GameGetTick(game), pszMessageName, UnitGetId(unit), UnitGetDevName(unit), stats_data->m_szName, dwParam, *(float *)((int *)&nNewValue));
	}
	else
	{
		trace("[%5d]  %s:  unit [%d: %s]   stat [%s]   param [%d]   value [%d]\n", 
			GameGetTick(game), pszMessageName, UnitGetId(unit), UnitGetDevName(unit), stats_data->m_szName, dwParam, nNewValue);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStatsChangeMessageDoControlUnitEvents(
	GAME * game,
	UNIT * unit,
	WORD wStat,
	DWORD dwParam,
	int nMsgValue,
	int nOldValue,
	int nNewValue)
{
	if (unit == GameGetControlUnit(game))
	{
		switch (wStat)
		{
			//----------------------------------------------------------------------------
		case STATS_HP_CUR:
			{
#if ISVERSION(DEVELOPMENT)
				LogMessage(COMBAT_LOG, "unit[%d] hp=%d.",
					DWORD(UnitGetId(unit)), UnitGetStatShift(unit, STATS_HP_CUR));
#endif
				break;
			}

			//----------------------------------------------------------------------------		
		case STATS_TALKING_TO:
			{
				c_NPCTalkingTo( (UNITID)nMsgValue );
				break;
			}

			//----------------------------------------------------------------------------
		case STATS_SKILL_POINTS:
			{
				c_QuestEventSkillPoint();
				break;
			}

			//----------------------------------------------------------------------------
		case STATS_FACTION_SCORE:
			{
				int nFaction = dwParam;
				c_FactionScoreChanged( unit, nFaction, nOldValue, nNewValue );
				break;
			}

			//----------------------------------------------------------------------------
		case STATS_BREATH:
			{
				if(AppIsHellgate())
				{
					c_UnitBreathChanged( unit );
				}
			}
		}		
	}
	else
	{

	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitSetStat(
	 GAME * game,
	 BYTE * data)
{
	MSG_SCMD_UNITSETSTAT * msg = (MSG_SCMD_UNITSETSTAT *)data;

	UNIT * unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	WORD wStat = STAT_GET_STAT(msg->dwStat);
	DWORD dwParam = STAT_GET_PARAM(msg->dwStat);
	int nOldValue = UnitGetAccruedStat(unit, wStat, dwParam);
	int delta = msg->nValue - nOldValue;
	UnitChangeStat(unit, wStat, dwParam, delta);
	int nNewValue = UnitGetStat(unit, wStat, dwParam);
	
	sStatsChangeMessageTrace(game, unit, wStat, dwParam, nNewValue, "MSG_SCMD_UNITSETSTAT");
	sStatsChangeMessageDoControlUnitEvents(game, unit, wStat, dwParam, msg->nValue, nOldValue, nNewValue);

	UnitSetStatDelayedTimer(unit, wStat, dwParam, nNewValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitSetStatInList(
	 GAME * game,
	 BYTE * data)
{
	MSG_SCMD_UNITSETSTATINLIST * msg = (MSG_SCMD_UNITSETSTATINLIST *)data;

	UNIT * unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	WORD wStat = STAT_GET_STAT(msg->dwStat);
	DWORD dwParam = STAT_GET_PARAM(msg->dwStat);
	STATS * stats = StatsGetModList(unit);
	while(stats)
	{
		if(StatsListGetId(stats) == msg->dwStatsListId)
		{
			break;
		}
		stats = StatsGetModNext(stats);
	}
	if(!stats)
	{
		return;
	}

	StatsSetStat(game, stats, wStat, dwParam, msg->nValue);
	
	sStatsChangeMessageTrace(game, unit, wStat, dwParam, msg->nValue, "MSG_SCMD_UNITSETSTATINLIST");
	sStatsChangeMessageDoControlUnitEvents(game, unit, wStat, dwParam, msg->nValue, msg->nValue, msg->nValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitSetState(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UNITSETSTATE * msg = (MSG_SCMD_UNITSETSTATE *)data;

	UNIT * target = UnitGetById(game, msg->idTarget);
	if (!target)
	{
		return;
	}
	UNIT * source = UnitGetById(game, msg->idSource);
	if (!source)
	{
		return;
	}

#if DEBUG_MESSAGE_TRACE
	const STATE_DATA * state_data = StateGetData(game, msg->wState);
	ASSERT_RETURN(state_data);
	trace("[CLT]  MSG_SCMD_UNITSETSTATE:  target [%d: %s]   source: [%d: %s]  state [%s]  index [%d]  tick [%d]\n", 
		UnitGetId(target), UnitGetDevName(target), UnitGetId(source), UnitGetDevName(source), state_data->szName, msg->wStateIndex, GameGetTick(game));
#endif

	STATS * stats = c_StateSet(target, source, msg->wState, msg->wStateIndex, msg->nDuration, msg->wSkill, FALSE, msg->bDontExecuteSkillStats);

	if( stats && msg->dwStatsListId != INVALID_ID )
	{
		StatsListSetId(stats, msg->dwStatsListId);
	}

	// See if some stats were sent along with the state
	if (stats && MSG_GET_BLOB_LEN(msg, tBuffer) > 0)
	{
		BIT_BUF tBuffer(msg->tBuffer, MAX_STATE_STATS_BUFFER);
		XFER<XFER_LOAD> tXfer(&tBuffer);
		StatsXfer(game, stats, tXfer, STATS_WRITE_METHOD_NETWORK);
	}
	if (stats)
	{
		c_StateSetPostProcessStats(game, target, source, stats, msg->wState, msg->wStateIndex, msg->wSkill);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitClearState(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UNITCLEARSTATE * msg = (MSG_SCMD_UNITCLEARSTATE *)data;

	UNIT * target = UnitGetById(game, msg->idTarget);
	if (!target)
	{
		return;
	}

#if DEBUG_MESSAGE_TRACE
	const STATE_DATA * state_data = StateGetData(game, msg->wState);
	ASSERT_RETURN(state_data);
	trace("[CLT]  MSG_SCMD_UNITCLEARSTATE:  target [%d: %s]   state [%s]  index [%d]  tick [%d]\n", 
		UnitGetId(target), UnitGetDevName(target), state_data->szName, msg->wStateIndex, GameGetTick(game));
#endif

	c_StateClear(target, msg->idSource, msg->wState, msg->wStateIndex);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitSetFlag(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNIT_SET_FLAG * msg = ( MSG_SCMD_UNIT_SET_FLAG * )data;

	UNIT * target = UnitGetById( game, msg->idUnit );
	ASSERT_RETURN( target );

	ASSERT( msg->nFlag < NUM_UNITFLAGS );
	ASSERT( ( msg->bValue == 0 ) || ( msg->bValue == 1 ) );

	UnitSetFlag( target, msg->nFlag, msg->bValue );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitWarp(
	 GAME* game,
	 BYTE* data)
{
	MSG_SCMD_UNITWARP* msg = (MSG_SCMD_UNITWARP*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

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

	BOOL bRepaint = FALSE;
	if (PetIsPet(unit) && UnitGetContainer(unit) == GameGetControlUnit(game) && room != NULL)
	{
		bRepaint = TRUE;
	}

	// do the warp	
	UnitWarp(
		unit, 
		room, 
		msg->vPosition, 
		msg->vFaceDirection, 
		msg->vUpDirection, 
		msg->dwUnitWarpFlags);

	if (bRepaint)
	{
		UIRepaint();
	}
	if( AppIsHellgate() )
	{
		c_UnitSetLastKnownGoodPosition(unit, msg->vPosition);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitPathPosUpdate(
	 GAME* game,
	 BYTE* data)
{
	MSG_SCMD_UNIT_PATH_POSITION_UPDATE * msg = ( MSG_SCMD_UNIT_PATH_POSITION_UPDATE * )data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	ROOM * room = RoomGetByID( game, msg->idPathRoom );
	if ( !room )
	{
		return;
	}

	// update me!
	c_UnitSetPathServerLoc( unit, room, msg->nPathIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChangeInvLocation(
	GAME * game,
	UNIT * container,
	UNIT * item,
	int location,
	int x,
	int y)
{
	// TODO: Make this an error
	if (item == NULL)
	{
		return;
	}
	if (container == NULL)
	{
		return;
	}

	// this is a message from the server, who is the authority on everything, so 
	// setup some flags that will bypass the client side sanity checks
	UNIT * oldContainer = UnitGetContainer(item);

	if (x < 0)
	{
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, container, item, location, CLF_IGNORE_NO_DROP_GRID_BIT);
	}
	else if (y < 0)
	{
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, container, item, location, x, CLF_IGNORE_NO_DROP_GRID_BIT);
	}
	else
	{
		UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, container, item, location, x, y, CLF_IGNORE_NO_DROP_GRID_BIT);
	}

	if (GameGetControlUnit(game) == container && container != oldContainer)
	{
		sHotkeyUpdatePickupItem(game, item);
	}
	if( AppIsTugboat() &&
		game->eGameType == GAME_TYPE_CLIENT )
	{
		UnitCalculateVisibleDamage( GameGetControlUnit( game ) );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInventoryLinkTo(
	 GAME *pGame,
	 BYTE *pData)
{
	const MSG_SCMD_INVENTORY_LINK_TO *pMessage = (const MSG_SCMD_INVENTORY_LINK_TO *)pData;
	UNIT *pUnit = UnitGetById( pGame, pMessage->idUnit );
	UNIT *pContainer = UnitGetById( pGame, pMessage->idContainer );

	// it is possible that the unit is not known by this client ... it's unfortunate
	// that the server has no way of knowing exactly which units the clients knows about, 
	// client units are not even necessarily in the same room on each machine due to 
	// network delays and the fact that both the client and server are pathfinding and
	// trying to hide the network lag
	if (pUnit == NULL)
	{
		return;
	}
	
	// debug validation, we should always know about the container
	if (pContainer == NULL)
	{
#if ISVERSION(RETAIL_VERSION)
		ASSERT_RETURN(0);
#else
		const int MAX_MESSAGE = 1024;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE, 
			"Client Inventory Link To Error: Unit '%s' [%d] wants to link into '%s' [%d]",
			pUnit ? UnitGetDevName( pUnit ) :  "UNKNOWN",
			pUnit ? UnitGetId( pUnit ) : INVALID_ID,
			pContainer ? UnitGetDevName( pContainer ) : "UNKNOWN",
			pContainer ? UnitGetId( pContainer ) : INVALID_ID);
		ASSERTX_RETURN( 0, szMessage );
#endif
	}

	// add to inventory of container
	const INVENTORY_LOCATION &tInvLoc = pMessage->tLocation;	
	InventoryLinkUnitToInventory(pUnit, pContainer, tInvLoc, TIME_ZERO, FALSE, 0);
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRemoveItemFromInv(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_REMOVE_ITEM_FROM_INV * msg = (MSG_SCMD_REMOVE_ITEM_FROM_INV *)data;
	{
		UNIT * item = UnitGetById(game, msg->idItem);	
		ASSERT_RETURN(item);
		UnitInventoryRemove(item, ITEM_ALL, 0, TRUE);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdChangeInvLocation(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_CHANGE_INV_LOCATION * msg = (MSG_SCMD_CHANGE_INV_LOCATION *)data;
	UNIT * container = UnitGetById(game, msg->idContainer);
	ASSERT_RETURN(container);
	UNIT * item = UnitGetById(game, msg->idItem);	
	ASSERT_RETURN(item);
	const INVENTORY_LOCATION * location = &msg->tLocation;	
	
	sChangeInvLocation(game, container, item, location->nInvLocation, location->nX, location->nY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSwapInvLocation(
	 GAME *pGame,
	 BYTE *pData)
{
	const MSG_SCMD_SWAP_INV_LOCATION *pMessage = (const MSG_SCMD_SWAP_INV_LOCATION *)pData;
	
	// get items
	UNIT *pItemSource = UnitGetById( pGame, pMessage->idItemSource );
	ASSERTX_RETURN( pItemSource, "No source item for inventory swap" );
	UNIT *pItemDest = UnitGetById( pGame, pMessage->idItemDest );
	ASSERTX_RETURN( pItemDest, "No dest item for inventory swap" );	

	// get containers
	UNIT *pContainerSource = UnitGetContainer( pItemSource );
	ASSERTX_RETURN( pContainerSource, "No source container" );
	UNIT *pContainerDest = UnitGetContainer( pItemDest );
	ASSERTX_RETURN( pContainerDest, "No dest container" );

	// get locations
	const INVENTORY_LOCATION *pNewSourceLoc = &pMessage->tNewSourceLocation;
	const INVENTORY_LOCATION *pNewDestLoc = &pMessage->tNewDestLocation;

	// remove source item
	ASSERT_RETURN(UnitInventoryRemove(pItemSource, ITEM_ALL) == pItemSource);
	
	// remove dest item
	ASSERT_RETURN(UnitInventoryRemove(pItemDest, ITEM_ALL) == pItemDest);
	
	// put source in new location
	ASSERT(UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainerDest, pItemSource, pNewSourceLoc->nInvLocation, pNewSourceLoc->nX, pNewSourceLoc->nY));
	
	// put dest in new location
	ASSERT(UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, pContainerSource, pItemDest, pNewDestLoc->nInvLocation, pNewDestLoc->nX, pNewDestLoc->nY));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInvItemUnusable(
	 GAME * game,
	 BYTE * data)
{
	const MSG_SCMD_INV_ITEM_UNUSABLE * msg = (const MSG_SCMD_INV_ITEM_UNUSABLE *)data;

	UNIT * item = UnitGetById(game, msg->idItem);
	ASSERT_RETURN(item);

	UNIT * container = UnitGetContainer(item);
	ASSERT_RETURN(container);

	SetEquippedItemUnusable(game, container, item);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInvItemUsable(
	 GAME * game,
	 BYTE * data)
{
	const MSG_SCMD_INV_ITEM_USABLE * msg = (const MSG_SCMD_INV_ITEM_USABLE *)data;

	UNIT * item = UnitGetById(game, msg->idItem);
	ASSERT_RETURN(item);

	UNIT * container = UnitGetContainer(item);
	ASSERT_RETURN(container);

	SetEquippedItemUsable(game, container, item);
}
	

//----------------------------------------------------------------------------
// called to drop an item into the world
//----------------------------------------------------------------------------
void sSCmdUnitDrop(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UNITDROP * msg = (MSG_SCMD_UNITDROP *)data;

	UNIT * unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	// remove item from the inventory
	if (!UnitInventoryRemove( unit, ITEM_ALL))
	{
		return;
	}

	VECTOR vPosition = msg->vPosition;
	VECTOR vTarget = msg->vNewPosition;
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, UW_FORCE_VISIBLE_BIT );
	UnitWarp( 
		unit, 
		RoomGetByID( game, msg->idRoom ), 
		vPosition, 
		VECTOR(0, 1, 0), 
		VECTOR(0, 0, 1), 
		dwWarpFlags,
		FALSE,
		FALSE);

	VECTOR vStart = vTarget + VECTOR( 0, 0, 50 );
	float fDist = 100;
	VECTOR vCollisionNormal;
	float fCollideDistance = LevelLineCollideLen( game, UnitGetLevel( unit ), vStart, VECTOR( 0, 0, -1 ), fDist, &vCollisionNormal, 0, (int)PROP_MATERIAL);
	if (fCollideDistance < fDist )
	{
		vTarget = vStart;
		vTarget.fZ -= fCollideDistance;
	}

	unit->bDropping = TRUE;
	unit->fDropPosition = 0;
	unit->m_pActivePath->Clear();
	unit->m_pActivePath->CPathAddPoint(vPosition, VECTOR(0, 0, 1));
	VECTOR vApex = ( vPosition + vTarget ) / 2.0f;
	vApex.fZ += RandGetFloat( game, 2.5f, 3.2f );
	unit->m_pActivePath->CPathAddPoint(vApex, VECTOR(0, 0, 1));
	unit->m_pActivePath->CPathAddPoint(vTarget, VECTOR(0, 0, 1));
	UnitSetUpDirection( unit, msg->vUpDirection );
	UnitSetFaceDirection( unit, msg->vFaceDirection, TRUE );
	UnitUpdateRoom( unit, RoomGetByID( game, msg->idRoom ), TRUE );
	//unit->pRoom = RoomGetByID( game, msg->idRoom );
	ASSERT( unit->pRoom );
	// gold doesn't flip - just shows up in a pile
	if ( UnitGetClass( unit ) == 0 )
	{
		unit->fDropPosition = 1;

	}

	if (UnitGetGenus(unit) == GENUS_ITEM &&
		UnitGetClass( unit ) != 0 )
	{
		const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
		if (item_data && item_data->m_nFlippySound != INVALID_ID)
		{
			c_SoundPlay(item_data->m_nFlippySound, &c_UnitGetPosition(unit), NULL);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPetDeathRepaintUIHandler(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherunit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	UIRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInventoryAddPet(
	 GAME * pGame,
	 BYTE * pData)
{
	const MSG_SCMD_INVENTORY_ADD_PET *pMessage = (const MSG_SCMD_INVENTORY_ADD_PET*)pData;
	const INVENTORY_LOCATION *pLocation = &pMessage->tLocation;

	// get owner and pet	
	UNIT *pOwner = UnitGetById( pGame, pMessage->idOwner );
	ASSERTX_RETURN( pOwner, "Client got add pet for unknown owner" );
	UNIT *pPet = UnitGetById( pGame, pMessage->idPet );
	ASSERTX_RETURN( pPet, "Client got add pet for unknown pet" );
	
	// add pet to inventory
	sChangeInvLocation(pGame, pOwner, pPet, pLocation->nInvLocation, pLocation->nX, pLocation->nY);
	
	const INVLOC_DATA * pInvLoc = UnitGetInvLocData(pOwner, pLocation->nInvLocation);
	ASSERTX_RETURN(pInvLoc, "Invalid inventory location for unit");

	if ( TESTBIT( pInvLoc->dwFlags, INVLOCFLAG_RESURRECTABLE ) )
	{
		// so set an event to notify the UI to redraw when the pet dies
		UnitRegisterEventHandler( pGame, pPet, EVENT_UNITDIE_BEGIN, sPetDeathRepaintUIHandler, NULL );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInventoryRemovePet(
	 GAME* game,
	 BYTE* data)
{
	const MSG_SCMD_INVENTORY_REMOVE_PET* msg = (const MSG_SCMD_INVENTORY_REMOVE_PET*)data;

	UNIT *pOwner = UnitGetById(game, msg->idOwner);
	ASSERTX_RETURN( pOwner, "Client got pet removal for unknown owner" );
	UNIT *pPet = UnitGetById(game, msg->idPet);
	ASSERTX_RETURN( pPet, "Client got pet removal for unknown pet" );
	ASSERTX_RETURN(	UnitGetContainer( pPet ) == pOwner, "Client got remove pet message, but pet is not in container" );

	// remove	
	UnitInventoryRemove( pPet, ITEM_ALL );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdHotkey(
	 GAME* game,
	 BYTE* data)
{
	ASSERT_RETURN(game);
	MSG_SCMD_HOTKEY* msg = (MSG_SCMD_HOTKEY*)data;

	UNIT* unit = GameGetControlUnit(game);
	ASSERTX_RETURN(unit, "Hotkey message can't find control unit on client" );

	int slot = msg->bHotkey;
	UNIT_TAG_HOTKEY* tag = UnitGetHotkeyTag(unit, slot);
	if (!tag)
	{
		tag = UnitAddHotkeyTag(unit, slot);
		ASSERT_RETURN(tag);
	}

	tag->m_nSkillID[0] = msg->nSkillID[0];
	tag->m_nSkillID[1] = msg->nSkillID[1];

	UNIT* item = UnitGetById(game, msg->idItem[0]);
	if ((msg->idItem[0] == INVALID_ID || (item &&	UnitGetOwnerId(item) == UnitGetId(unit))) &&
		(tag && (tag->m_idItem[0] != msg->idItem[0])))
	{
		tag->m_idItem[0] = msg->idItem[0];
	}
	if (item)
		UnitWaitingForInventoryMove(item, FALSE);
	item = UnitGetById(game, msg->idItem[1]);
	if ((msg->idItem[1] == INVALID_ID || (item &&	UnitGetOwnerId(item) == UnitGetId(unit))) &&
		(tag && (tag->m_idItem[1] != msg->idItem[1])))
	{
		tag->m_idItem[1] = msg->idItem[1];
	}
	if (item)
		UnitWaitingForInventoryMove(item, FALSE);

	tag->m_nLastUnittype = msg->nLastUnittype;		// the server tells the client what the last unittype is

	// we might want to target this at just the activebar, but this happens fairly infrequently
	//   and the paint isn't that expensive, so I'm going to leave it for now.
	UISendMessage(WM_PAINT, 0, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdClearUnitWaitingForInvMove(
	 GAME* game,
	 BYTE* data)
{
	ASSERT_RETURN(game);

	MSG_SCMD_CLEAR_INV_WAITING* msg = (MSG_SCMD_CLEAR_INV_WAITING*)data;

	UNIT* item = UnitGetById(game, msg->idItem);
	ASSERT_RETURN(item);
	
	UnitWaitingForInventoryMove(item, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdCursorItemReference(
	 GAME* game,
	 BYTE* data)
{
	ASSERT_RETURN(game);

	MSG_SCMD_CURSOR_ITEM_REFERENCE* msg = (MSG_SCMD_CURSOR_ITEM_REFERENCE*)data;

	UNIT* item = UnitGetById(game, msg->idItem);
	ASSERT_RETURN(item);
	
	UISetCursorUnit(msg->idItem, FALSE, msg->nFromWeaponConfig, 0.0f, 0.0f, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitNew(
	GAME* game,
	BYTE* data)
{
	TIMER_START("sSCmdNewUnit");
	MSG_SCMD_UNIT_NEW * msg = (MSG_SCMD_UNIT_NEW *)data;

	// receive the new unit message
	c_UnitNewMessageReceived( game, msg );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitRemove(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_UNIT_REMOVE *pMessage = (const MSG_SCMD_UNIT_REMOVE *)pData;
	UNITID idUnit = pMessage->id;
	DWORD dwUnitFreeFlags = pMessage->flags;

	// remove unit
	c_UnitRemove( pGame, idUnit, dwUnitFreeFlags );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void sSCmdUnitMode(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_UNITMODE * msg = (MSG_SCMD_UNITMODE *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	c_UnitSetLastKnownGoodPosition(unit, msg->position);
	UNITMODE eMode = (UNITMODE)msg->mode;
	if(c_UnitModeOverrideClientPathing(eMode) || !UnitPathIsActiveClient(unit))
	{
		c_UnitSetMode(unit, eMode, msg->wParam, msg->duration);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void sSCmdUnitModeEnd(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_UNITMODEEND * msg = (MSG_SCMD_UNITMODEEND *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	if ( UnitGetRoom( unit ))
		c_UnitSetLastKnownGoodPosition(unit, msg->position);
	UNITMODE eMode = (UNITMODE)msg->mode;
	if(c_UnitModeOverrideClientPathing(eMode) || !UnitPathIsActiveClient(unit))
	{
		UnitEndMode( unit, eMode, 0, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitPosition(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITPOSITION* msg = (MSG_SCMD_UNITPOSITION*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	UnitChangePosition(unit, msg->TargetPosition);
	c_UnitSetLastKnownGoodPosition(unit, msg->TargetPosition);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitMoveXYZ(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITMOVEXYZ* msg = (MSG_SCMD_UNITMOVEXYZ *)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	UnitSetStatFloat(unit, STATS_STOP_DISTANCE, 0, msg->fStopDistance);
	UnitSetVelocity(unit, msg->fVelocity);
	float fModeDuration = 0.0f;
	if(msg->mode == MODE_KNOCKBACK)
	{
		UnitSetFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
		fModeDuration = VectorLength(UnitGetPosition(unit) - msg->TargetPosition) / msg->fVelocity;
	}
	UnitSetFlag(unit, UNITFLAG_NO_CLIENT_FIXUP);
	c_UnitSetMode(unit, (UNITMODE)msg->mode, 0, fModeDuration);
	UnitClearFlag(unit, UNITFLAG_NO_CLIENT_FIXUP);

	if(msg->idFaceTarget >= 0)
	{
		UNIT * pFaceTarget = UnitGetById(game, msg->idFaceTarget);
		if(pFaceTarget)
		{
			VECTOR vFaceDirection = UnitGetPosition(pFaceTarget) - UnitGetPosition(unit);
			VectorNormalize(vFaceDirection);
			UnitSetFaceDirection(unit, vFaceDirection);
		}
	}

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
		if(AppIsHellgate())
		{
			BYTE bPathFlags = 0;
			if(msg->bFlags & MOVE_FLAG_CLIENT_STRAIGHT)
			{
				SETBIT(&bPathFlags, PSS_CLIENT_STRAIGHT_BIT);
			}
			SETBIT(&bPathFlags, PSS_APPEND_DESTINATION_BIT);
			PathStructDeserialize(unit, msg->buf, bPathFlags);
		}

		if(AppIsTugboat())
		{
			unit->vQueuedFaceDirection = VECTOR( 0, 0, 0 );
			UnitClearBothPathNodes( unit );
			BYTE bPathFlags = 0;
			SETBIT(&bPathFlags, PSS_APPEND_DESTINATION_BIT);
			PathStructDeserialize(unit, msg->buf, bPathFlags);
			UnitReservePathOccupied( unit );
		}
	}
	else
	{
		UnitStepListAdd( game, unit );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitMoveID(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITMOVEID* msg = (MSG_SCMD_UNITMOVEID*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	UnitSetStatFloat(unit, STATS_STOP_DISTANCE, 0, msg->fStopDistance);
	UnitSetVelocity(unit, msg->fVelocity);
	UnitSetFlag(unit, UNITFLAG_NO_CLIENT_FIXUP);
	if( AppIsHellgate() || !UnitTestMode( unit, (UNITMODE)msg->mode ) )
	{
		c_UnitSetMode(unit, (UNITMODE)msg->mode, 0, 0.0f);
	}
	UnitClearFlag(unit, UNITFLAG_NO_CLIENT_FIXUP);

	UNIT* target = UnitGetById(game, msg->idTarget);
	if (!target)
	{
		return;
	}

	if(TESTBIT(msg->bFlags, UMID_NO_PATH))
	{
		UnitSetFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
	}
	UnitSetMoveTarget(unit, msg->idTarget);
	if (unit->m_pPathing && !UnitTestFlag(unit, UNITFLAG_TEMPORARILY_DISABLE_PATHING))
	{
		if(!TESTBIT(msg->bFlags, UMID_USE_EXISTING_PATH))
		{
			if(AppIsHellgate())
			{
				BYTE bPathFlags = 0;
				if(TESTBIT(msg->bFlags, UMID_CLIENT_STRAIGHT))
				{
					SETBIT(&bPathFlags, PSS_CLIENT_STRAIGHT_BIT);
				}
				SETBIT(&bPathFlags, PSS_APPEND_DESTINATION_BIT);
				PathStructDeserialize(unit, msg->buf, bPathFlags);
			}
			if(AppIsTugboat())
			{
				unit->vQueuedFaceDirection = VECTOR( 0, 0, 0 );
				UnitClearBothPathNodes( unit );
				BYTE bPathFlags = 0;
				SETBIT(&bPathFlags, PSS_APPEND_DESTINATION_BIT);
				PathStructDeserialize(unit, msg->buf, bPathFlags);
				UnitReservePathOccupied( unit );
			}
		}
	}
	else
	{
		UnitStepListAdd( game, unit );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitMoveDirection(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITMOVEDIRECTION* msg = (MSG_SCMD_UNITMOVEDIRECTION*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	UnitSetStatFloat(unit, STATS_STOP_DISTANCE, 0, msg->fStopDistance);
	UnitSetVelocity(unit, msg->fVelocity);
	UnitSetFlag(unit, UNITFLAG_NO_CLIENT_FIXUP);
	c_UnitSetMode(unit, (UNITMODE)msg->mode, 0, 0.0f);
	UnitClearFlag(unit, UNITFLAG_NO_CLIENT_FIXUP);

	VECTOR vDirection = msg->MoveDirection;
	VectorNormalize(vDirection, vDirection);

	UnitSetMoveDirection(unit, vDirection);
	if (unit->m_pPathing)
	{
		if(AppIsHellgate())
		{
			PathStructDeserialize(unit, msg->buf);
		}
		if(AppIsTugboat())
		{
			unit->vQueuedFaceDirection = VECTOR( 0, 0, 0 );
			UnitClearBothPathNodes( unit );
			PathStructDeserialize(unit, msg->buf);
			UnitReservePathOccupied( unit );
		}
	}
	else
	{
		UnitStepListAdd( game, unit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitPathChange(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITPATHCHANGE * msg = ( MSG_SCMD_UNITPATHCHANGE * )data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	if (unit->m_pPathing)
	{
		if(AppIsHellgate())
		{
			PathStructDeserialize(unit, msg->buf);
		}
		if(AppIsTugboat())
		{
			unit->vQueuedFaceDirection = VECTOR( 0, 0, 0 );
			UnitClearBothPathNodes( unit );
			PathStructDeserialize(unit, msg->buf);
			UnitReservePathOccupied( unit );
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitJump(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITJUMP* msg = (MSG_SCMD_UNITJUMP*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	UnitStartJump(game, unit, msg->fZVelocity, FALSE, msg->bRemoveFromStepList);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitWardrobe(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITWARDROBE* msg = (MSG_SCMD_UNITWARDROBE*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}

	c_WardrobeUpdateLayersFromMsg( unit, msg);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitAddImpact(
	GAME* game,
	BYTE* data)
{
	MSG_SCMD_UNITADDIMPACT* msg = (MSG_SCMD_UNITADDIMPACT*)data;

	UNIT* unit = UnitGetById(game, msg->id);
	if (!unit)
	{
		return;
	}
#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		HAVOK_IMPACT tImpact;
		tImpact.fForce = msg->fForce;
		tImpact.dwFlags = msg->wFlags;
		tImpact.vSource = msg->vSource;
		tImpact.vDirection = msg->vDirection;
		tImpact.dwTick = GameGetTick( game );

		UnitAddImpact( game, unit, &tImpact );
		if (unit->pHavokRigidBody)
		{
			UnitApplyImpacts(game, unit);
		}
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillStart(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLSTART * msg = (MSG_SCMD_SKILLSTART *)data;

#ifdef TRACE_SYNCH
	trace("RECV: SCMD_SKILLSTART[C]: UNIT:%d  SKILL:%d  TIME:%d\n",
		msg->id,
		msg->wSkillId,
		-1/*(DWORD)AppGetAbsTime()*/);
#endif

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	c_UnitSetLastKnownGoodPosition(unit, msg->vUnitPosition);

	SkillStartRequest(	
		pGame, 
		unit, 
		msg->wSkillId, 
		msg->idTarget, 
		msg->vTarget,
		msg->dwStartFlags, 
		msg->dwSkillSeed);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillStartXYZ(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLSTARTXYZ * msg = (MSG_SCMD_SKILLSTARTXYZ *)data;

#ifdef TRACE_SYNCH
	trace("RECV: SCMD_SKILLSTARTXYZ[C]: UNIT:%d  SKILL:%d  TIME:%d\n",
		msg->id,
		msg->wSkillId,
		-1/*(DWORD)AppGetAbsTime()*/);
#endif

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	c_UnitSetLastKnownGoodPosition(unit, msg->vUnitPosition);

	SkillStartRequest( 
		pGame, 
		unit, 
		msg->wSkillId, 
		INVALID_ID, 
		msg->vTarget,
		msg->dwStartFlags, 
		msg->dwSkillSeed);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillStartID(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLSTARTID * msg = (MSG_SCMD_SKILLSTARTID *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	c_UnitSetLastKnownGoodPosition(unit, msg->vUnitPosition);

	SkillStartRequest( 
		pGame, 
		unit, 
		msg->wSkillId, 
		msg->idTarget, 
		VECTOR(0), 
		msg->dwStartFlags,
		msg->dwSkillSeed);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillChangeTargetID(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLCHANGETARGETID * msg = (MSG_SCMD_SKILLCHANGETARGETID *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}

	SkillSetTarget( unit, msg->wSkill, msg->idWeapon, msg->idTarget, msg->bIndex ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillChangeTargetLocation(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLCHANGETARGETLOC * msg = (MSG_SCMD_SKILLCHANGETARGETLOC *)data;
	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}
	UnitSetStatVector(unit, STATS_SKILL_TARGET_X, msg->wSkillId, msg->nTargetIndex, msg->vTarget);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillChangeMultipleTargetLocations(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLCHANGETARGETLOCMULTI * msg = (MSG_SCMD_SKILLCHANGETARGETLOCMULTI *)data;
	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}
	for (int i = 0; i < msg->nLocationCount; i++)
	{
		UnitSetStatVector(unit, STATS_SKILL_TARGET_X, msg->wSkillId, i, msg->vTarget);
	}	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillStop(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLSTOP * msg = (MSG_SCMD_SKILLSTOP *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}
 
	if ( msg->bRequest )
		SkillStopRequest( unit, msg->wSkillId, FALSE, TRUE );
	else
	{
		SkillStop( pGame, unit, msg->wSkillId, INVALID_ID );
		SkillUpdateActiveSkills( unit );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSkillCooldown(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_SKILLCOOLDOWN * msg = (MSG_SCMD_SKILLCOOLDOWN *)data;

	UNIT * unit = UnitGetById(pGame, msg->id);
	if (!unit)
	{
		return;
	}
	UNIT * pWeapon = UnitGetById(pGame, msg->idWeapon);

	const SKILL_DATA * pSkillData = SkillGetData( pGame, msg->wSkillId );
	ASSERT_RETURN( pSkillData );
	SkillStartCooldown( pGame, unit, pWeapon, msg->wSkillId, 0, pSkillData, msg->wTicks, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSysText(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_SYS_TEXT * msg = (MSG_SCMD_SYS_TEXT *)data;

	ChatFilterStringInPlace(msg->str);
	if (msg->bConsoleOnly) {
		ConsoleString1(msg->dwColor, msg->str);
	} else {
		// do this in local AND global chat channels 
		UIAddChatLine(CHAT_TYPE_GAME, msg->dwColor, msg->str);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUIShowMessage(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UISHOWMESSAGE * msg = (MSG_SCMD_UISHOWMESSAGE *)data;

	UIShowMessage(msg->bType, msg->nDialog, msg->nParam1, msg->nParam2, (DWORD)msg->bFlags);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdStateAddTarget(
	GAME* pGame,
	BYTE* data)
{
	MSG_SCMD_STATEADDTARGET * msg = (MSG_SCMD_STATEADDTARGET*)data;

	c_StateAddTarget( pGame, msg );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdStateRemoveTarget(
	GAME* pGame,
	BYTE* data)
{
	MSG_SCMD_STATEREMOVETARGET * msg = (MSG_SCMD_STATEREMOVETARGET*)data;

	c_StateRemoveTarget( pGame, msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdStateAddTargetLocation(
	GAME* pGame,
	BYTE* data)
{
	MSG_SCMD_STATEADDTARGETLOCATION * msg = (MSG_SCMD_STATEADDTARGETLOCATION*)data;

	c_StateAddTargetLocation( pGame, msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitCraftingBreakdown( GAME * game,
								 BYTE * data)
{	
	cUICraftingShowItemsBrokenDownMsg( data );
	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void sSCmdQuestEvent(
	GAME * pGame,
	BYTE * data)
{
	MSG_SCMD_QUESTEVENT * msg = (MSG_SCMD_QUESTEVENT *)data;

	UNIT * unit = NULL;
	if ( msg->idSource != INVALID_ID )
		unit = UnitGetById(pGame, msg->idSource);

	c_QuestEvent( pGame, unit, &msg->vPosition, msg->byQuestEventId );
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitPlaySound(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UNITPLAYSOUND * msg = ( MSG_SCMD_UNITPLAYSOUND * )data;
	UNIT* unit = UnitGetById(game, msg->id);
	ASSERT_RETURN(unit);
	
	SOUND_PLAY_EXINFO tExInfo;
	tExInfo.bForceBlockingLoad = TRUE;  // we say that server sounds are important to know now

	VECTOR vPosition = UnitGetAimPoint(unit);
	c_SoundPlay(msg->idSound, &vPosition, &tExInfo);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdAudioDialogPlay(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_AUDIO_DIALOG_PLAY *pMessage = (MSG_SCMD_AUDIO_DIALOG_PLAY *)pData;
	UNITID idSource = pMessage->idSource;
	UNIT *pUnit = UnitGetById( pGame, idSource );
	if( AppIsTugboat() && !pUnit )
	{
		// we have cases where you teleport during interaction with an NPC - and they won't exist
		// anymore when this occurs.
		return;
	}
	ASSERTX_RETURN( pUnit, "Unable to find source unit for audio dialog" );

	// stop any audio dialog that is currently playing at this unit cause overlapping
	// audio dialog is silly
	SOUND_ID idSoundCurrent = UnitGetStat( pUnit, STATS_AUDIO_DIALOG_SOUND_ID );
	if (idSoundCurrent != INVALID_ID)
	{
		c_SoundStop( idSoundCurrent );
	}

	// play new audio	
	int nSound = pMessage->nSound;
	VECTOR vAudioPosition = UnitGetAimPoint( pUnit );
	SOUND_PLAY_EXINFO tExInfo;
	tExInfo.bForceBlockingLoad = TRUE;  // we say that server sounds are important to know now	
	SOUND_ID idSound = c_SoundPlay( nSound, &vAudioPosition, &tExInfo );
	if (idSound != INVALID_ID)
	{
	
		// save sound id of dialog in stat
		UnitSetStat( pUnit, STATS_AUDIO_DIALOG_SOUND_ID, idSound );
	
	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Note: This gets sent to us several times a second from all the players
// so if we miss one, it's no big deal...
void sSCmdPlayerMove(
	GAME * game,
	BYTE * data)
{
	c_PlayerMoveMsg( game, (MSG_SCMD_PLAYERMOVE *) data );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitsetOwner(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_UNITSETOWNER * msg = ( MSG_SCMD_UNITSETOWNER * )data;

	UNIT * unit = UnitGetById( game, msg->idSource );
	ASSERT_RETURN( unit );
	UNIT * owner = UnitGetById( game, msg->idOwner );

	UnitSetOwner(unit, owner);

	if (msg->bChangeTargetType)
	{
		if( UnitGetTargetType( owner ) != TARGET_DEAD )
		{
			UnitChangeTargetType(unit, UnitGetTargetType(owner));
		}		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdMissilesChangeOwner(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_MISSILES_CHANGEOWNER * msg = (MSG_SCMD_MISSILES_CHANGEOWNER*)data;

	UNIT * unit = UnitGetById( game, msg->idUnit );
	ASSERT_RETURN( unit );
	UNIT * weapon = UnitGetById( game, msg->idWeapon );
	for(int i=0; i<msg->nMissileCount; i++)
	{
		ASSERT_BREAK(i < ARRAYSIZE(msg->nMissiles));
		if(msg->nMissiles[i] < 0)
		{
			continue;
		}
		UNIT * missile = UnitGetById(game, msg->nMissiles[i]);
		ASSERT_CONTINUE(missile);
		MissileChangeOwner(game, missile, unit, weapon, msg->bFlags);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdAvailableTasks(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_AVAILABLE_TASKS *msg = (MSG_SCMD_AVAILABLE_TASKS *)data;
	c_TaskAvailableTasks( msg );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdAvailableQuests(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_AVAILABLE_QUESTS *msg = (MSG_SCMD_AVAILABLE_QUESTS *)data;
	CQuestAvailableQuests( game, msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRecipeListOpen(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_RECIPE_LIST_OPEN *msg = (MSG_SCMD_RECIPE_LIST_OPEN *)data;
	int nRecipeList = msg->nRecipeList;
	UNIT *pRecipeGiver = UnitGetById( game, msg->idRecipeGiver );
	c_RecipeListOpen( nRecipeList, pRecipeGiver );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRecipeOpen(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_RECIPE_OPEN *pMessage = (MSG_SCMD_RECIPE_OPEN *)pData;
	int nRecipe = pMessage->nRecipe;
	UNIT *pRecipeGiver = UnitGetById( pGame, pMessage->idRecipeGiver );
	c_RecipeOpen( nRecipe, pRecipeGiver );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRecipeForceClosed(
	GAME *pGame,
	BYTE *pData)
{
	//MSG_SCMD_RECIPE_FORCE_CLOSED *pMessage = (MSG_SCMD_RECIPE_FORCE_CLOSED *)pData;
	c_RecipeClose( FALSE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRecipeCreated(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_RECIPE_CREATED *pMessage = (MSG_SCMD_RECIPE_CREATED *)pData;
	if( AppIsHellgate() ||
		pMessage->bSuccess )
	{
		c_RecipeCreated( pMessage->nRecipe );
	}
	else
	{
		c_RecipeFailed( pMessage->nRecipe );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskStatus(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TASK_STATUS *msg = (MSG_SCMD_TASK_STATUS *)data;
	c_TasksSetStatus( msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPointOfInterestUpdate( GAME * game,
								 BYTE * data )
{
	MSG_SCMD_POINT_OF_INTEREST *msg = (MSG_SCMD_POINT_OF_INTEREST *)data;		
	UNIT *pPlayer = GameGetControlUnit( game );
	MYTHOS_POINTSOFINTEREST::cPointsOfInterest *cPointsOfInterest = PlayerGetPointsOfInterest( pPlayer );
	ASSERT_RETURN( cPointsOfInterest );
	switch( (int)msg->nMessageType )
	{
		case MYTHOS_POINTSOFINTEREST::KPofI_MessageType_Add:
			{
				GENUS nGenus = TEST_MASK( msg->nFlags, MYTHOS_POINTSOFINTEREST::KPofI_Flag_IsObject )?GENUS_OBJECT:GENUS_MONSTER;
				const UNIT_DATA *pUnitData = UnitGetData( game, nGenus, msg->nClass );		
				cPointsOfInterest->AddPointOfInterest( pUnitData,
					msg->nUnitId,
					msg->nClass,
					msg->nX, 
					msg->nY,
					INVALID_ID,
					msg->nFlags );

			}
			break;
		case MYTHOS_POINTSOFINTEREST::KPofI_MessageType_Remove:
			cPointsOfInterest->RemovePointOfInterest( msg->nUnitId );
			break;
		case MYTHOS_POINTSOFINTEREST::KPofI_MessageType_RemoveAll:
			cPointsOfInterest->ClearAllPointsOfInterest();
			break;
		case MYTHOS_POINTSOFINTEREST::KPofI_MessageType_Refresh:
			{
				c_QuestCaculatePointsOfInterest( pPlayer );
				//tell the player they are entering a new level and or floor				
				LEVEL *pLevel = UnitGetLevel( pPlayer );
				if( pLevel &&
					pLevel->m_pLevelDefinition )
				{
					c_PlayerEnteringNewLevelAndFloor( game, 
						pPlayer, 
						pLevel->m_pLevelDefinition->nIndex,
						pLevel->m_nDRLGDef,
						LevelGetLevelAreaID( pLevel ),
						LevelGetDepth( pLevel ) );
				}
			}
			break;
	}
	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdAnchorPointAddedToLevel(
						  GAME * game,
						  BYTE * data )
{
	MSG_SCMD_ANCHOR_MARKER *msg = (MSG_SCMD_ANCHOR_MARKER *)data;
	if( msg->nUnitId == INVALID_ID )
	{

		LEVEL *pLevel = LevelGetByID( game, msg->nLevelId );	
		ASSERT_RETURN( pLevel );
		MYTHOS_ANCHORMARKERS::cAnchorMarkers *cAnchorMarkers = LevelGetAnchorMarkers( pLevel );
		ASSERT_RETURN( cAnchorMarkers );
		cAnchorMarkers->Clear();	

	}
	else
	{
		LEVEL *pLevel = LevelGetByID( game, msg->nLevelId );	
		ASSERT_RETURN( pLevel );
		MYTHOS_ANCHORMARKERS::cAnchorMarkers *cAnchorMarkers = LevelGetAnchorMarkers( pLevel );
		ASSERT_RETURN( cAnchorMarkers );
		cAnchorMarkers->AddAnchor( game, msg->nBaseRow, msg->fX, msg->fY );	
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestTaskStatus(
	GAME * game,
	BYTE * data )
{
	MSG_SCMD_QUEST_TASK_STATUS *msg = (MSG_SCMD_QUEST_TASK_STATUS *)data;
	c_QuestHandleQuestTaskMessage( game, msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdLevelAreaMapMessage(
						  GAME * game,
						  BYTE * data )
{
	MSG_SCMD_LEVELAREA_MAP_MESSAGE *msg = (MSG_SCMD_LEVELAREA_MAP_MESSAGE *)data;
	MYTHOS_MAPS::c_MapMessage( game, msg );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskOfferReward(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TASK_OFFER_REWARD *msg = (MSG_SCMD_TASK_OFFER_REWARD *)data;
	c_TasksOfferReward( msg->idTaskGiver, msg->idTask );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskClose(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TASK_CLOSE *msg = (MSG_SCMD_TASK_CLOSE *)data;
	c_TasksCloseTask( msg->idTask );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskRestore(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TASK_RESTORE* msg = (MSG_SCMD_TASK_RESTORE*)data;
	c_TasksRestoreTask( &msg->tTaskTemplate, (TASK_STATUS)msg->eStatus );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskRestrictedByFaction(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_TASK_RESTRICTED_BY_FACTION *pMessage = (MSG_SCMD_TASK_RESTRICTED_BY_FACTION *)pData;
	c_TasksRestrictedByFaction( pMessage->idGiver, pMessage->nFactionBad );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdEnterIdentify(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_ENTER_IDENTIFY *pMessage = (const MSG_SCMD_ENTER_IDENTIFY *)pData;
	UNIT *pAnalyzer = UnitGetById( pGame, pMessage->idAnalyzer );
	c_ItemEnterMode( ITEM_MODE_IDENTIFY, pAnalyzer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskIncomplete(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TASK_INCOMPLETE* msg = (MSG_SCMD_TASK_INCOMPLETE*)data;
	c_TasksIncomplete( msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdCannotUse(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_CANNOT_USE *pMessage = (const MSG_SCMD_CANNOT_USE *)pData;
	USE_RESULT eUseResult = (USE_RESULT)pMessage->eReason;
	UNITID idItem = pMessage->idUnit;
	
	UNIT *pItem = UnitGetById( pGame, idItem );
	if (pItem)
	{
		c_ItemCannotUse( pItem, eUseResult );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdCannotPickup(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_CANNOT_PICKUP *pMessage = (const MSG_SCMD_CANNOT_PICKUP *)pData;
	PICKUP_RESULT eReason = (PICKUP_RESULT)pMessage->eReason;
	UNITID idItem = pMessage->idUnit;	
	c_ItemCannotPickup( idItem, eReason );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdCannotDrop(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_CANNOT_DROP *pMessage = (const MSG_SCMD_CANNOT_DROP *)pData;
	DROP_RESULT eReason = (DROP_RESULT)pMessage->eReason;
	UNITID idItem = pMessage->idUnit;	
	c_ItemCannotDrop( idItem, eReason );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdHeadstoneInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_HEADSTONE_INFO *pMessage = (const MSG_SCMD_HEADSTONE_INFO *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pAsker = UnitGetById( pGame, pMessage->idAsker );
	
	// format the text for the query
	c_NPCHeadstoneInfo( 
		pUnit, 
		pAsker, 
		pMessage->bCanReturnNow, 
		pMessage->nCost, 
		pMessage->nLevelDefDest);
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdDungeonWarpInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_DUNGEON_WARP_INFO *pMessage = (const MSG_SCMD_DUNGEON_WARP_INFO *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pWarper = UnitGetById( pGame, pMessage->idWarper );
	
	// format the text for the query
	c_NPCDungeonWarpInfo( 
		pUnit, 
		pWarper, 
		pMessage->nDepth,
		cCurrency( pMessage->nCost, pMessage->nCostRealWorld) );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdCreateGuildInfo(
						  GAME *pGame,
						  BYTE *pData)
{
	const MSG_SCMD_CREATEGUILD_INFO *pMessage = (const MSG_SCMD_CREATEGUILD_INFO *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pGuildmaster = UnitGetById( pGame, pMessage->idGuildmaster );

	// format the text for the query
	c_NPCCreateGuildInfo( 
		pUnit, 
		pGuildmaster, 
		cCurrency( pMessage->nCost, pMessage->nCostRealWorld) );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRespecInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_RESPEC_INFO *pMessage = (const MSG_SCMD_RESPEC_INFO *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pGuildmaster = UnitGetById( pGame, pMessage->idRespeccer );

	// format the text for the query
	c_NPCRespecInfo( 
		pUnit, 
		pGuildmaster, 
		cCurrency( pMessage->nCost, pMessage->nCostRealWorld),
		pMessage->nCrafting != 0 );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTransporterWarpInfo(
						  GAME *pGame,
						  BYTE *pData)
{
	const MSG_SCMD_TRANSPORTER_WARP_INFO *pMessage = (const MSG_SCMD_TRANSPORTER_WARP_INFO *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pWarper = UnitGetById( pGame, pMessage->idWarper );

	// format the text for the query
	c_NPCTransporterWarpInfo( 
		pUnit, 
		pWarper );

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPvPSignerUpperInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_PVP_SIGNER_UPPER_INFO *pMessage = (const MSG_SCMD_PVP_SIGNER_UPPER_INFO *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pPvPSignerUpper = UnitGetById( pGame, pMessage->idPvPSignerUpper );

	// format the text for the query
	c_NPCPvPSignerUpperInfo( 
		pUnit, 
		pPvPSignerUpper);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdHirelingSelection(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_HIRELING_SELECTION *pMessage = (const MSG_SCMD_HIRELING_SELECTION *)pData;
	UNIT *pUnit = GameGetControlUnit( pGame );
	UNIT *pForeman = UnitGetById( pGame, pMessage->idForeman );
	const HIRELING_CHOICE *pHirelings = pMessage->tHirelings;
	int nNumHirelings = pMessage->nNumHirelings;

	// open up UI		
	c_HirelingsOffer( pUnit, pForeman, pHirelings, nNumHirelings, pMessage->bSufficientFunds );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTalkStart(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_TALK_START *msg = (MSG_SCMD_TALK_START *)data;
	
	// init spec
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = msg->idTalkingTo;
	tSpec.eType = (NPC_DIALOG_TYPE)msg->nDialogType;
	tSpec.nDialog = msg->nDialog;
	tSpec.nDialogReward = msg->nDialogReward;
	tSpec.nQuestDefId = msg->nQuestId;
	tSpec.eGITalkingTo = (GLOBAL_INDEX)msg->nGISecondNPC;
	//tSpec.tCallbackOK = UIGenericDialogOnOK;	//to tell the NPC dialog to go away

	
	// start talk	
	c_NPCTalkStart( game, &tSpec );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTalkForceCancel(
	GAME *pGame,
	BYTE *pData)
{
	//const MSG_CMD_TALK_FORCE_CANCEL *pMessage = (const MSG_CMD_TALK_FORCE_CANCEL *)pData;
	c_NPCTalkForceCancel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdExamine(
	GAME *pGame,
	BYTE *pData)
{	
	MSG_SCMD_EXAMINE *pMessage = (MSG_SCMD_EXAMINE *)pData;
	
	// init spec
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = pMessage->idUnit;
	tSpec.eType = NPC_DIALOG_ANALYZE;
	
	// start talk	
	c_NPCTalkStart( pGame, &tSpec );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
extern void UIOpenPlayerInspectionPanel(GAME *pGame, UNITID idUnit );
//----------------------------------------------------------------------------
void sSCmdInspect(
	GAME *pGame,
	BYTE *pData)
{	
	MSG_SCMD_INSPECT *pMessage = (MSG_SCMD_INSPECT *)pData;
	UNITID idPlayer = pMessage->idUnit;
	UIOpenPlayerInspectionPanel(pGame, idPlayer);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdVideoStart(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_VIDEO_START *msg = (MSG_SCMD_VIDEO_START *)data;
	
	// init spec
	NPC_DIALOG_SPEC tSpec;
	tSpec.eGITalkingTo = (GLOBAL_INDEX)msg->nGITalkingTo;
	tSpec.eGITalkingTo2 = (GLOBAL_INDEX)msg->nGITalkingTo2;
	tSpec.eType = (NPC_DIALOG_TYPE)msg->nDialogType;
	tSpec.nDialog = msg->nDialog;
	tSpec.nDialogReward = msg->nDialogReward;
	
	// start talk	
	c_NPCVideoStart( game, &tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdEmoteStart(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_EMOTE_START *msg = (MSG_SCMD_EMOTE_START *)data;
	
	// start emote
	c_NPCEmoteStart( game, msg->nDialog );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInteractInfo(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_INTERACT_INFO *msg = (MSG_SCMD_INTERACT_INFO *)data;
	c_InteractInfoSet( game, msg->idSubject, (INTERACT_INFO)msg->bInfo );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdLevelMarker(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_LEVELMARKER * msg = ( MSG_SCMD_LEVELMARKER * )data;

	const UNIT_DATA * objectdata = ObjectGetData( game, msg->nObjectClass );
	if (objectdata && objectdata->nString != INVALID_LINK)
	{
		const WCHAR * str = StringTableGetStringByIndex( objectdata->nString );
		ASSERT(str);
		UIShowQuickMessage( str, LEVELCHANGE_FADEOUTMULT_DEFAULT );
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestNPCUpdate(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_QUEST_NPC_UPDATE * msg = ( MSG_SCMD_QUEST_NPC_UPDATE * )data;

	UNIT* pNPC = UnitGetById( game, msg->idNPC );
	if( pNPC )
	{
		c_QuestUpdateQuestNPCs( game,
						   	  GameGetControlUnit( game ),
							  pNPC );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestState(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_QUEST_STATE * msg = ( MSG_SCMD_QUEST_STATE * )data;
	int nQuest = msg->nQuest;
	int nQuestState = msg->nQuestState;
	QUEST_STATE_VALUE eValue = (QUEST_STATE_VALUE)msg->nValue;
	BOOL bRestore = (BOOL)msg->bRestore;

	// set new state value
	c_QuestSetState( nQuest, nQuestState, eValue, bRestore );	 

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestStatus(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_QUEST_STATUS * msg = ( MSG_SCMD_QUEST_STATUS * )data;
	int nQuest = msg->nQuest;
	int nDifficulty = msg->nDifficulty;
	QUEST_STATUS eStatus = (QUEST_STATUS)msg->nStatus;

	c_QuestEventQuestStatus( nQuest, nDifficulty, eStatus, msg->bRestore );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdEndQuestSetup(
	GAME * game,
	BYTE * data)
{	
	c_QuestEventEndQuestSetup();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPlayMovielist(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_PLAY_MOVIELIST * msg = ( MSG_SCMD_PLAY_MOVIELIST * )data;
	AppSwitchState( APP_STATE_PLAYMOVIELIST, msg->nMovieListIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestDisplayDialog(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_QUEST_DISPLAY_DIALOG *msg = (MSG_SCMD_QUEST_DISPLAY_DIALOG*)data;
	c_QuestDisplayDialog( game, msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestTemplateRestore(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_QUEST_TEMPLATE_RESTORE *msg = (MSG_SCMD_QUEST_TEMPLATE_RESTORE*)data;
	c_QuestTemplateRestore( game, msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestClearStates(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_QUEST_CLEAR_STATES *msg = (MSG_SCMD_QUEST_CLEAR_STATES*)data;
	c_QuestClearAllStates( game, msg->nQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSubLevelStatus(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_SUBLEVEL_STATUS *pMessage = (const MSG_SCMD_SUBLEVEL_STATUS *)pData;
	LEVEL *pLevel = LevelGetByID( pGame, pMessage->idLevel );
	if (pLevel)
	{
		SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, pMessage->idSubLevel );
		ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
		
		// one day we might want to make client sublevel status bits, in which case
		// we would need to now blow them away here -C.Day
		pSubLevel->dwSubLevelStatus = pMessage->dwSubLevelStatus;		
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdDBUnitLog(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_DBUNIT_LOG *pMessage = (const MSG_SCMD_DBUNIT_LOG *)pData;
	c_DBUnitLog( (DATABASE_OPERATION)pMessage->nOperation, pMessage->szMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdDBUnitLogStatus(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_DBUNIT_LOG_STATUS *pMessage = (const MSG_SCMD_DBUNIT_LOG_STATUS *)pData;
	c_DBUnitLogStatus( pMessage->nEnabled );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdOperateResult(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_OPERATE_RESULT * msg = (MSG_SCMD_OPERATE_RESULT * )data;
	OPERATE_RESULT eResult = (OPERATE_RESULT)msg->nResult;
	int nStringIndex = INVALID_LINK;
	
	switch (eResult)
	{
	
		//----------------------------------------------------------------------------
		case OPERATE_RESULT_FORBIDDEN_BY_QUEST:
		{
			nStringIndex = StringTableGetStringIndexByKey( "CannotOperateQuest" );
			break;
		}

		//----------------------------------------------------------------------------	
		case OPERATE_RESULT_FORBIDDEN:
		{
			nStringIndex = StringTableGetStringIndexByKey( "CannotOperate" );
			break;
		}
		
		//----------------------------------------------------------------------------
		case OPERATE_RESULT_OK:
		{
			ASSERTX_RETURN( 0, "OPERATE_RESULT_OK sent to client, no reason to send this if we operated it ok" );
			break;
		}
		
	}
	
	// display console message for now
	if (nStringIndex != INVALID_LINK)
	{
		const WCHAR *puszString = StringTableGetStringByIndex( nStringIndex );
		ConsoleString( CONSOLE_SYSTEM_COLOR, puszString );	
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdBlockingState(
	GAME * game,
	BYTE * data)
{	
	MSG_SCMD_BLOCKING_STATE * msg = ( MSG_SCMD_BLOCKING_STATE * )data;
	BLOCKING_STAT_VALUE eState = (BLOCKING_STAT_VALUE)msg->nState;
	UNIT * unit = UnitGetById( game, msg->idUnit );

	// if it wasn't found, it will resync once this client knows about it, so no worries here
	if ( !unit )
	{
		return;
	}

	switch( eState )
	{
		//----------------------------------------------------------------------------	
		case BSV_NOT_SET:
		{
			ASSERTX_RETURN( 0, "BSV_NOT_SET sent to client, no reason to send this" );
			break;
		}

		//----------------------------------------------------------------------------	
		case BSV_BLOCKING:
		{
			ObjectSetBlocking( unit );
			break;
		}

		//----------------------------------------------------------------------------	
		case BSV_OPEN:
		{
			ObjectClearBlocking( unit );
			break;
		}
	}
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTutorial(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TUTORIAL * msg = ( MSG_SCMD_TUTORIAL * )data;
	REF(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskExterminatedCount(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_TASK_EXTERMINATED_COUNT* pMessage = (MSG_SCMD_TASK_EXTERMINATED_COUNT*)pData;
	c_TasksUpdateExterminatedCount( pMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTaskRewardTaken(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_TASK_REWARD_TAKEN *pMessage = (const MSG_SCMD_TASK_REWARD_TAKEN *)pData;
	c_TaskRewardTaken( pMessage->idTask, pMessage->idReward );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUpdateNumRewardItemsTaken(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN *pMessage = (const MSG_SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN *)pData;
	UIUpdateRewardMessage( pMessage->nMaxTakes - pMessage->nNumTaken, pMessage->nOfferDefinition );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeStart(
	GAME* pGame,
	BYTE* pData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	MSG_SCMD_TRADE_START* pMessage = (MSG_SCMD_TRADE_START*)pData;
	UNITID idTradingWith = pMessage->idTradingWith;
	UNIT* pTradingWith = UnitGetById( pGame, idTradingWith );
	ASSERTX_RETURN( pTradingWith, "Expected trading with" );
	
	const TRADE_SPEC &tTradeSpec = pMessage->tTradeSpec;

	c_TradeStart( pTradingWith, tTradeSpec );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeForceCancel(
	GAME* pGame,
	BYTE* pData)
{
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	c_TradeForceCancel();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeStatus(
	GAME* pGame,
	BYTE* pData)
{
	const MSG_SCMD_TRADE_STATUS *pMessage = (const MSG_SCMD_TRADE_STATUS *)pData;
	c_TradeSetStatus( pMessage->idTrader, 
		(TRADE_STATUS)pMessage->nStatus, pMessage->nTradeOfferVersion );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeCannotAccept(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_TRADE_CANNOT_ACCEPT *pMessage = (const MSG_SCMD_TRADE_CANNOT_ACCEPT *)pData;
	REF(pMessage);
	c_TradeCannotAccept();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeRequestNew(
	GAME* pGame,
	BYTE* pData)
{
	const MSG_SCMD_TRADE_REQUEST_NEW *pMessage = (const MSG_SCMD_TRADE_REQUEST_NEW *)pData;
	c_TradeBeingAskedToTrade( pMessage->idToTradeWith );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeRequestNewCancel(
	GAME* pGame,
	BYTE* pData)
{
	const MSG_SCMD_TRADE_REQUEST_NEW_CANCEL *pMessage = (const MSG_SCMD_TRADE_REQUEST_NEW_CANCEL *)pData;
	c_TradeRequestCancelledByAskingPlayer(pMessage->idInitialRequester);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeMoney(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_TRADE_MONEY *pMessage = (const MSG_SCMD_TRADE_MONEY *)pData;
	c_TradeMoneyModified( pMessage->nMoneyYours, pMessage->nMoneyTheirs, pMessage->nMoneyRealWorldYours, pMessage->nMoneyRealWorldTheirs );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTradeError(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_TRADE_ERROR *pMessage = (const MSG_SCMD_TRADE_ERROR *)pData;
	c_TradeError( pMessage->idTradingWith, (TRADE_ERROR)pMessage->nTradeError );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdWallWalkOrient(
	GAME* pGame,
	BYTE* pData)
{
	MSG_SCMD_WALLWALK_ORIENT* pMessage = (MSG_SCMD_WALLWALK_ORIENT*)pData;
	UNIT* unit = UnitGetById(pGame, pMessage->id);
	if (!unit)
	{
		return;
	}

	UnitSetFaceDirection(unit, pMessage->vFaceDirection);
	UnitSetUpDirection(unit, pMessage->vUpDirection);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdToggleUIElement(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_TOGGLE_UI_ELEMENT *pMessage = (MSG_SCMD_TOGGLE_UI_ELEMENT *)data;
	UI_ELEMENT eElement = (UI_ELEMENT)pMessage->nElement;
	UI_ELEMENT_ACTION eAction = (UI_ELEMENT_ACTION)pMessage->nAction;
	
	switch (eElement)
	{
	
		//----------------------------------------------------------------------------	
		case UIE_PAPER_DOLL:
		{
			UIComponentActivate(UICOMP_INVENTORY, eAction == UIEA_OPEN);
			break;
		}
		
		//----------------------------------------------------------------------------	
		case UIE_SKILL_MAP:
		{
			UIComponentActivate(UICOMP_SKILLMAP, eAction == UIEA_OPEN);
			break;
		}

		//----------------------------------------------------------------------------	
		case UIE_WORLDMAP:
		{
			if (AppIsTugboat())
			{
				eAction == UIEA_OPEN ? UI_TB_ShowTravelMapScreen() : UI_TB_HideTravelMapScreen( FALSE );
			}
			else
			{
				UIComponentActivate(UICOMP_WORLD_MAP_HOLDER, eAction == UIEA_OPEN);
			}
			break;
		}

		//----------------------------------------------------------------------------	
		case UIE_STASH:
		{
			UNIT * pPlayer = GameGetControlUnit( game );
			UNITID idPlayer = UnitGetId(pPlayer);
			UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
			if( AppIsTugboat() )
			{
				if( eAction == UIEA_OPEN )
				{
					UIShowPaneMenu( KPaneMenuStash );
					UIShowPaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );
				}
				else
				{
					UIHidePaneMenu( KPaneMenuStash );
					UIHidePaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );
				}
			}
			else
			{
				UIComponentActivate(UICOMP_STASH_PANEL, eAction == UIEA_OPEN);
			}
			break;
		}

		//----------------------------------------------------------------------------	
		case UIE_CONVERSATION:
		{
			if (eAction == UIEA_OPEN)
			{
				ASSERTX_BREAK( 0, "Can't open ui element like this" );
			}
			else
			{
				UIHideNPCDialog(CCT_CANCEL);
			}
			break;
		}

		//----------------------------------------------------------------------------
		case UIE_ITEM_UPGRADE:
		{
			UNIT * pPlayer = GameGetControlUnit( game );
			UNITID idPlayer = UnitGetId(pPlayer);
			UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
			if (eAction == UIEA_OPEN)
			{
				c_ItemUpgradeOpen(IUT_UPGRADE);
			}
			else
			{
				c_ItemUpgradeClose(IUT_UPGRADE);
			}
			break;
		}

		//----------------------------------------------------------------------------
		case UIE_ITEM_AUGMENT:
		{
			UNIT * pPlayer = GameGetControlUnit( game );
			UNITID idPlayer = UnitGetId(pPlayer);
			UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
			if (eAction == UIEA_OPEN)
			{
				c_ItemUpgradeOpen(IUT_AUGMENT);
			}
			else
			{
				c_ItemUpgradeClose(IUT_AUGMENT);
			}
			break;
		}
		
		//----------------------------------------------------------------------------
		case UIE_ITEM_UNMOD:
		{
			UNIT * pPlayer = GameGetControlUnit( game );
			UNITID idPlayer = UnitGetId(pPlayer);
			UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
			if (eAction == UIEA_OPEN)
			{
				c_ItemUpgradeOpen(IUT_UNMOD);
			}
			else
			{
				c_ItemUpgradeClose(IUT_UNMOD);
			}
			break;
		}

		//----------------------------------------------------------------------------		
		case UIE_EMAIL_PANEL:
		{
			UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_EMAIL_PANEL);
			if (pComponent)
			{
				if (eAction == UIEA_OPEN)
				{
					if (!UIComponentGetVisible(pComponent))
					{
						UIToggleEmail();
					}
				}
				else
				{
					if (UIComponentGetVisible(pComponent))
					{
						UIToggleEmail();
					}
				}
			}
		}

		//----------------------------------------------------------------------------		
		case UIE_AUCTION_PANEL:
		{
			UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_AUCTION_PANEL);
			if (pComponent)
			{
				if (eAction == UIEA_OPEN)
				{
					if (!UIComponentGetVisible(pComponent))
					{
						UIToggleAuctionHouse();
					}
				}
				else
				{
					if (UIComponentGetVisible(pComponent))
					{
						UIToggleAuctionHouse();
					}
				}
			}
		}

		//----------------------------------------------------------------------------		
		default:
		{
			break;
		}
		
	}
	
}

//----------------------------------------------------------------------------
// displays the recipe pane
//----------------------------------------------------------------------------
void sSCmdShowRecipePane(
	 GAME * game,
	 BYTE * data )
{
	MSG_SCMD_SHOW_RECIPE_PANE *pMessage = (MSG_SCMD_SHOW_RECIPE_PANE *)data;
	int nRecipeID = pMessage->nRecipeID;
	//UI_TB_ShowWorldMapScreen( nRecipeID );
	REF( nRecipeID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdShowMapAtlas(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_SHOW_MAP_ATLAS *pMessage = (MSG_SCMD_SHOW_MAP_ATLAS *)data;
	int nArea = pMessage->nArea;
	switch( (EMAP_ATLAS_MESSAGE)pMessage->nResult )
	{
	case MAP_ATLAS_MSG_NOROOM :
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_ALERT ),  GlobalStringGet( GS_MAP_NO_ROOM ), FALSE );
		break;
	case MAP_ATLAS_MSG_ALREADYKNOWN :
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_ALERT ),  GlobalStringGet( GS_MAP_ALREADY_KNOWN ), FALSE );
		break;
	case MAP_ATLAS_MSG_MAPREMOVED :
		UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_ALERT ),  GlobalStringGet( GS_MAP_REMOVED ), FALSE );
		break;
	default :
		 UI_TB_ShowWorldMapScreen( nArea );
		break;
	}
	


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdEffectAtUnit(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_EFFECT_AT_UNIT *pMessage = (const MSG_SCMD_EFFECT_AT_UNIT *)pData;
	UNIT *pUnit = UnitGetById( pGame, pMessage->idUnit );
	if (pUnit)
	{
		c_EffectAtUnit( pUnit, (EFFECT_TYPE)pMessage->eEffectType );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdEffectAtLocation(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_EFFECT_AT_LOCATION *pMessage = (const MSG_SCMD_EFFECT_AT_LOCATION *)pData;
	c_EffectAtLocation( &pMessage->vPosition, (EFFECT_TYPE)pMessage->eEffectType );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdOperateWaypointMachine(
	GAME *pGame,
	BYTE *pData)
{
	if( AppIsHellgate() )
	{
		c_LevelOpenWaypointUI( pGame );
	}
	else
	{
		UIShowAnchorstoneScreen();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdOperateRunegate(
	GAME *pGame,
	BYTE *pData)
{

	UIShowRunegateScreen();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdJumpBegin(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_JUMP_BEGIN *pMessage = (const MSG_SCMD_JUMP_BEGIN *)pData;
	
	// get unit that's jumping
	UNIT *pUnit = UnitGetById( pGame, pMessage->idUnit );
	ASSERTX_RETURN( pUnit, "Client find unit to make it jump!" );

	// make it jump
	UnitModeStartJumping( pUnit );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdJumpEnd(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_JUMP_END *pMessage = (const MSG_SCMD_JUMP_END *)pData;
	const VECTOR *pvPosition = &pMessage->vPosition;
	
	// get unit that's jumping
	UNIT *pUnit = UnitGetById( pGame, pMessage->idUnit );
	ASSERTX_RETURN( pUnit, "Client find unit to make jump stop!" );

	// allow c_PlayerDoStep to simulate other players to the ground, to avoid the position popping from latency
	if ( !UnitIsPlayer(pUnit) || pUnit == GameGetControlUnit( pGame ) )
	{
		// warp to the position on the server maybe?
		LEVEL *pLevel = UnitGetLevel( pUnit );
		ROOM *pRoom = RoomGetFromPosition( pGame, pLevel, pvPosition );
		UnitWarp(
			pUnit,
			pRoom,
			*pvPosition,
			UnitGetFaceDirection( pUnit, TRUE ),
			UnitGetUpDirection( pUnit ),
			0, 
			FALSE);	

		// stop the jumping
		UnitModeEndJumping( pUnit, FALSE );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdTownPortal(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_TOWN_PORTAL_CHANGED * pMessage = (MSG_SCMD_TOWN_PORTAL_CHANGED*)pData;
	if( AppIsHellgate() )
	{
		if( pMessage->bIsOpening )
		{
			c_TownPortalOpened(
				gApp.m_characterGuid,
				gApp.m_wszPlayerName,
				&pMessage->tTownPortal );
		}
		else
		{
			c_TownPortalClosed(
				gApp.m_characterGuid,
				gApp.m_wszPlayerName,
				&pMessage->tTownPortal );
		}
	}
	else
	{
		if( pMessage->bIsOpening )
		{
			c_TownPortalOpened(
				pMessage->idPlayer,
				pMessage->szName,
				&pMessage->tTownPortal );
		}
		else
		{
			c_TownPortalClosed(
				pMessage->idPlayer,
				pMessage->szName,
				&pMessage->tTownPortal );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdEnterTownPortal(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_ENTER_TOWN_PORTAL *pMessage = (const MSG_SCMD_ENTER_TOWN_PORTAL *)pData;
	c_TownPortalEnterUIOpen( pMessage->idPortal );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdSelectReturnDestination(
	GAME *pGame,
	BYTE *pData)
{
//	ConsoleString( CONSOLE_SYSTEM_COLOR, "Select warp destination" );
	const MSG_SCMD_SELECT_RETURN_DESTINATION *pMessage = (const MSG_SCMD_SELECT_RETURN_DESTINATION *)pData;
	c_TownPortalUIOpen( pMessage->idReturnWarp );
}


static SIMPLE_DYNAMIC_ARRAY<MSG_SCMD_GAME_INSTANCE_UPDATE> sGameInstanceList;
static BOOL sbInstanceListInitialized = FALSE;
static GAME_INSTANCE_TYPE seInstanceListType = GAME_INSTANCE_NONE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdStartGameInstanceUpdate(
	GAME * game,
	BYTE * data )
{
	MSG_SCMD_GAME_INSTANCE_UPDATE_BEGIN * msg = (MSG_SCMD_GAME_INSTANCE_UPDATE_BEGIN*) data;
	ASSERT_RETURN(msg);

	UNREFERENCED_PARAMETER(game);
	sGameInstanceList.Init(MEMORY_FUNC_FILELINE(NULL, 0));
	sbInstanceListInitialized = TRUE;
	seInstanceListType = (GAME_INSTANCE_TYPE) msg->nInstanceListType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdGameInstanceUpdate(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);

	MSG_SCMD_GAME_INSTANCE_UPDATE * townInstance = NULL;
	townInstance = (MSG_SCMD_GAME_INSTANCE_UPDATE*)data;

	ASSERT_RETURN(sbInstanceListInitialized);

	sGameInstanceList.Add(MEMORY_FUNC_FILELINE(townInstance[0]));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdEndGameInstanceUpdate(
	GAME * game,
	BYTE * data )
{
	UNREFERENCED_PARAMETER(game);

	if (sGameInstanceList.Count() > 0)
		UIInstanceListReceiveInstances(seInstanceListType, &sGameInstanceList[0], sGameInstanceList.Count());
	else
		UIInstanceListReceiveInstances(seInstanceListType, NULL, 0);

	sbInstanceListInitialized = FALSE;
	sGameInstanceList.Destroy();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRemovingFromReservedGame(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_REMOVING_FROM_RESERVED_GAME *pMessage = (const MSG_SCMD_REMOVING_FROM_RESERVED_GAME *)pData;
	
	// display message
	DWORD dwDelayInSeconds = pMessage->dwDelayInTicks / GAME_TICKS_PER_SECOND;
	const WCHAR *puszFormat = GlobalStringGet( GS_REMOVE_FROM_RESERVED_GAME );	
	ConsoleString( CONSOLE_SYSTEM_COLOR, puszFormat, dwDelayInSeconds );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPartyMemberInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_PARTY_MEMBER_INFO *pMessage = (const MSG_SCMD_PARTY_MEMBER_INFO *)pData;

	UIPartyListAddPartyMember(
		pMessage->idParty,
		pMessage->szName,
		pMessage->nLevel,
		pMessage->nRank,
		pMessage->nPlayerUnitClass,
		pMessage->idPlayer);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPartyAdResult(
	GAME *pGame,
	BYTE *pData )
{
	const MSG_SCMD_PARTY_ADVERTISE_RESULT *pMessage = (const MSG_SCMD_PARTY_ADVERTISE_RESULT *)pData;
	UIPartyCreateOnServerResult( (UI_PARTY_CREATE_RESULT)c_ChatNetGetPartyAdResult(pMessage->eResult) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPartyAcceptInviteConfirm(
	GAME *pGame,
	BYTE *pData )
{
	const MSG_SCMD_PARTY_ACCEPT_INVITE_CONFIRM *pMessage = (const MSG_SCMD_PARTY_ACCEPT_INVITE_CONFIRM *)pData;
	c_PlayerInviteAccept( pMessage->guidInviter, pMessage->bAbandonGame );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCCmdPartyJoinConfirm(
	GAME *pGame,
	BYTE *pData )
{
	const MSG_SCMD_PARTY_JOIN_CONFIRM *pMessage = (const MSG_SCMD_PARTY_JOIN_CONFIRM *)pData;
	c_PlayerPartyJoinConfirm( pMessage->bAbandonGame );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPing(
	GAME * game,
	BYTE * data)
{
	MSG_SCMD_PING* msg = (MSG_SCMD_PING *)data;

	if(msg->bIsReply)
	{ //Trace the round trip time.  Print to console?
		c_HandlePing(msg);
		/*int nRoundTripTime = (int)GetRealClockTime() - msg->timeOfSend;
		trace("Round trip ping of %d milliseconds from server.\n", nRoundTripTime);
		//ConsoleString("Round trip ping of %d milliseconds from server.\n", nRoundTripTime);
		c_SetPing(nRoundTripTime); //we grab this in the UI debug ping display*/
	}
	else
	{ //Send it back to them as a reply
		MSG_CCMD_PING cMsg;
		cMsg.timeOfSend = msg->timeOfSend;
		cMsg.timeOfReceive = (int)GetRealClockTime();
		cMsg.bIsReply = TRUE;
		c_SendMessage(CCMD_PING, &cMsg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdInteractChoices(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_INTERACT_CHOICES *pMessage = (const MSG_SCMD_INTERACT_CHOICES *)pData;
	UNIT *pPlayer = GameGetControlUnit( pGame );
	UNIT *pUnit = UnitGetById( pGame, pMessage->idUnit );
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pUnit, "Expected unit for interaction" );

	// display radial menu (or whatever logic we decide to end up using)
	c_InteractDisplayChoices( pPlayer, pUnit, pMessage->ptInteract, pMessage->nCount );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdConsoleMessage(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_CONSOLE_MESSAGE *pMessage = (const MSG_SCMD_CONSOLE_MESSAGE *)pData;

	GLOBAL_STRING eGlobalString = (GLOBAL_STRING)pMessage->nGlobalString;
	c_ConsoleMessage( eGlobalString );

	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdConsoleDialogMessage(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_CONSOLE_DIALOG_MESSAGE *pMessage = (const MSG_SCMD_CONSOLE_DIALOG_MESSAGE *)pData;	
	const WCHAR *szTemp = c_DialogTranslate( pMessage->nDialogID );		
	if( szTemp )
	{
		ConsoleString( szTemp );
	}	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdDPSInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_DPS_INFO *pMessage = (const MSG_SCMD_DPS_INFO *)pData;

	UIUpdateDPS(pMessage->fDPS);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdQuestRadar(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_QUEST_RADAR * pMessage = (const MSG_SCMD_QUEST_RADAR *)pData;

	switch ( pMessage->bCommand )
	{
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_RESET:
		{
			UIQuestTrackerClearAll();
		}
		break;
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_ADD_UNIT:
		{
			UIQuestTrackerUpdatePos(pMessage->idUnit, pMessage->vPosition, pMessage->nType, pMessage->dwColor);
		}
		break;
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_REMOVE_UNIT:
		{
			UIQuestTrackerRemove(pMessage->idUnit);
		}
		break;
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_UPDATE_UNIT:
		{
			UIQuestTrackerUpdatePos(pMessage->idUnit, pMessage->vPosition);
		}
		break;
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_TURN_OFF:
		{
			UIQuestTrackerEnable(FALSE);
		}
		break;
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_TURN_ON:
		{
			UIQuestTrackerEnable(TRUE);
		}
		break;
		//----------------------------------------------------------------------------
		case QUEST_UI_RADAR_TOGGLE:
		{
			UIQuestTrackerToggle();
		}
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRideStart(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_RIDE_START *pMessage = (const MSG_SCMD_RIDE_START *)pData;
	UNIT *pPlayer = GameGetControlUnit( pGame );
	UNIT *pRide = UnitGetById( pGame, pMessage->idUnit );
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pRide, "Expected unit for ride" );

	// start a train ride
	c_ObjectStartRide( pRide, pPlayer, pMessage->bCommand );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdRideEnd(
	GAME * pGame,
	BYTE * pData)
{
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "Expected player" );

	// end train ride
	c_ObjectEndRide( pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdMusicInfo(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_MUSIC_INFO *pMessage = (const MSG_SCMD_MUSIC_INFO *)pData;
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "Expected player" );

	c_PlayerUpdateMusicInfo(pPlayer, pMessage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdItemUpgraded(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_ITEM_UPGRADED *pMessage = (const MSG_SCMD_ITEM_UPGRADED *)pData;
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		c_ItemUpgradeItemUpgraded( pItem );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdItemAugmented(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_ITEM_AUGMENTED *pMessage = (const MSG_SCMD_ITEM_AUGMENTED *)pData;
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		c_ItemAugmentItemAugmented( pItem, pMessage->nAffixAugmented );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdItemsRestored(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_ITEMS_RESTORED *pMessage = (const MSG_SCMD_ITEMS_RESTORED *)pData;
	c_ItemsRestored( pMessage->nNumItemsRestored, pMessage->nNumItemsDropped );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdWarpRestricted(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_WARP_RESTRICTED *pMessage = (const MSG_SCMD_WARP_RESTRICTED *)pData;
	WARP_RESTRICTED_REASON eReason = (WARP_RESTRICTED_REASON)pMessage->nReason;
	c_WarpRestricted( eReason );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdGuildActionResult(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_GUILD_ACTION_RESULT *pMessage = (const MSG_SCMD_GUILD_ACTION_RESULT*)pData;

	const WCHAR * wszHeader = NULL;
	const WCHAR * wszText   = NULL;

	switch (pMessage->wActionType)
	{
	//------------------------------------------------------------------------
	//-- GUILD CREATION RESULTS ----------------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_CREATE_GUILD:
		{
			wszHeader = GlobalStringGet(GS_GUILD_CREATION_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_CREATION_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_INVALID_GUILD_NAME:			wszText = GlobalStringGet(GS_GUILD_CREATION_FAILURE_INVALID_NAME);			break;	//	illegal name
			case CHAT_ERROR_GUILD_NAME_TAKEN:			wszText = GlobalStringGet(GS_GUILD_CREATION_FAILURE_NAME_TAKEN);			break;	//	name already taken
			case CHAT_ERROR_MEMBER_ALREADY_IN_A_GUILD:	wszText = GlobalStringGet(GS_GUILD_CREATION_FAILURE_ALREADY_IN_A_GUILD);	break;	//	can't create a guild while in a guild
			case CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER:	wszText = GlobalStringGet(GS_GUILD_CREATION_FAILURE_NOT_A_SUBSCRIBER);		break;	//	not a subscriber
			case CHAT_ERROR_INTERNAL_ERROR:				//	unknown error
			default:									wszText = GlobalStringGet(GS_GUILD_CREATION_FAILURE_UNKNOWN);				break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- SEND GUILD INVITE RESULTS -------------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_INVITE_TO_GUILD:
		{
			wszHeader = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_SEND_INVITE_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_MEMBER_NOT_IN_A_GUILD:				wszText = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_NOT_IN_A_GUILD);		break;	//	inviter not in a guild to invite to
			case CHAT_ERROR_MEMBER_ALREADY_IN_A_GUILD:			wszText = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_TARGET_IN_A_GUILD);	break;	//	target player already in a guild, cannot receive invites while in a guild
			case CHAT_ERROR_MEMBER_ALREADY_INVITED_TO_A_GUILD:	wszText = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_TARGET_HAS_INVITE);	break;	//	target player already invited to a guild, cannot receive more than one invite
			case CHAT_ERROR_MEMBER_NOT_FOUND:					wszText = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_TARGET_NOT_FOUND);	break;	//	target player does not exist
			case CHAT_ERROR_GUILD_ACTION_PERMISSION_DENIED:		wszText = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_PERMISSION_DENIED);	break;	//	not high enough rank to invite
			case CHAT_ERROR_MEMBER_IGNORED:						//	they are ignoring the inviter
			case CHAT_ERROR_INTERNAL_ERROR:						//	unknown error
			default:											wszText = GlobalStringGet(GS_GUILD_SEND_INVITE_FAILURE_UNKNOWN);			break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- DECLINE GUILD INVITE RESULTS ----------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_DECLINE_GUILD_INVITE:
		{
			wszHeader = GlobalStringGet(GS_GUILD_DECLINE_INVITE_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_DECLINE_INVITE_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_NO_MEMBER_GUILD_INVITES:	wszText = GlobalStringGet(GS_GUILD_DECLINE_INVITE_FAILURE_NO_INVITES);	break;	//	no invite to decline
			case CHAT_ERROR_INTERNAL_ERROR:				//	unknown error
			default:									wszText = GlobalStringGet(GS_GUILD_DECLINE_INVITE_FAILURE_UNKNOWN);		break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- ACCEPT GUILD INVITE RESULTS -----------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_ACCEPT_GUILD_INVITE:
		{
			wszHeader = GlobalStringGet(GS_GUILD_ACCEPT_INVITE_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_ACCEPT_INVITE_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_NO_MEMBER_GUILD_INVITES:	wszText = GlobalStringGet(GS_GUILD_ACCEPT_INVITE_FAILURE_NO_INVITES);			break;	//	no invite to accept
			case CHAT_ERROR_MEMBER_ALREADY_IN_A_GUILD:	wszText = GlobalStringGet(GS_GUILD_ACCEPT_INVITE_FAILURE_ALREADY_IN_A_GUILD);	break;	//	must first leave a guild to join another
			case CHAT_ERROR_INTERNAL_ERROR:				//	unknown error
			default:									wszText = GlobalStringGet(GS_GUILD_ACCEPT_INVITE_FAILURE_UNKNOWN);				break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- LEAVE GUILD RESULTS -------------------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_LEAVE_GUILD:
		{
			wszHeader = GlobalStringGet(GS_GUILD_LEAVE_GUILD_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_LEAVE_GUILD_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_MEMBER_NOT_IN_A_GUILD:	wszText = GlobalStringGet(GS_GUILD_LEAVE_GUILD_FAILURE_NOT_IN_A_GUILD);	break;	//	no guild to leave
			case CHAT_ERROR_INTERNAL_ERROR:			//	unknown error
			default:								wszText = GlobalStringGet(GS_GUILD_LEAVE_GUILD_FAILURE_UNKNOWN);		break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- REMOVE GUILD MEMBER RESULTS -----------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_REMOVE_GUILD_MEMBER:
		{
			wszHeader = GlobalStringGet(GS_GUILD_REMOVE_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_REMOVE_SUCCESS));	wszHeader = NULL;
				break;
			case CHAT_ERROR_MEMBER_NOT_IN_A_GUILD:			wszText = GlobalStringGet(GS_GUILD_REMOVE_FAILURE_NOT_IN_A_GUILD);		break;	//	player not in a guild
			case CHAT_ERROR_MEMBER_NOT_IN_YOUR_GUILD:		wszText = GlobalStringGet(GS_GUILD_REMOVE_FAILURE_NOT_IN_YOUR_GUILD);	break;	//	not in your guild
			case CHAT_ERROR_GUILD_ACTION_PERMISSION_DENIED: wszText = GlobalStringGet(GS_GUILD_REMOVE_FAILURE_PERMISSION_DENIED);	break;	//	not a high enough rank to boot
			case CHAT_ERROR_INTERNAL_ERROR:	//	unknown error
			default:										wszText = GlobalStringGet(GS_GUILD_REMOVE_FAILURE_UNKNOWN);				break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- CHANGE MEMBER RANK RESULTS ------------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_CHANGE_GUILD_MEMBER_RANK:
		{
			wszHeader = GlobalStringGet(GS_GUILD_CHANGE_RANK_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_CHANGE_RANK_SUCCESS));	wszHeader = NULL;
				break;
			case CHAT_ERROR_MEMBER_NOT_IN_A_GUILD:			wszText = GlobalStringGet(GS_GUILD_PROMOTE_FAILURE_NOT_IN_A_GUILD);		break;	//	player not in a guild
			case CHAT_ERROR_MEMBER_NOT_IN_YOUR_GUILD:		wszText = GlobalStringGet(GS_GUILD_PROMOTE_FAILURE_NOT_IN_YOUR_GUILD);	break;	//	not in your guild
			case CHAT_ERROR_GUILD_ACTION_PERMISSION_DENIED: wszText = GlobalStringGet(GS_GUILD_PROMOTE_FAILURE_PERMISSION_DENIED);	break;	//	not a high enough rank to promote
			case CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER:		wszText = GlobalStringGet(GS_GUILD_PROMOTE_FAILURE_NOT_A_SUBSCRIBER);	break;	//	not a subscriber
			case CHAT_ERROR_INTERNAL_ERROR:	//	unknown error
			default:										wszText = GlobalStringGet(GS_GUILD_CHANGE_RANK_FAILURE_UNKNOWN);				break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- CHANGE RANK NAME ----------------------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_CHANGE_RANK_NAME:
		{
			wszHeader = GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_MEMBER_NOT_IN_A_GUILD:			wszText = GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_FAILURE_NOT_IN_A_GUILD);	break;
			case CHAT_ERROR_GUILD_ACTION_PERMISSION_DENIED:	wszText = GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_FAILURE_PERMISSION_DENIED);	break;
			case CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER:		wszText = GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_FAILURE_NOT_A_SUBSCRIBER);	break;
			case CHAT_ERROR_INVALID_GUILD_RANK_NAME:		wszText = GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_FAILURE_INVALID_NAME);		break;
			case CHAT_ERROR_INTERNAL_ERROR:	//	unknown error
			default:										wszText = GlobalStringGet(GS_GUILD_CHANGE_RANK_NAME_FAILURE_UNKNOWN);			break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- CHANGE GUILD ACTION PERMISSIONS -------------------------------------
	//------------------------------------------------------------------------
	case GAME_REQUEST_CHANGE_ACTION_PERMISSIONS:
		{
			wszHeader = GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_FAILURE_HEADER);
			switch (pMessage->wChatErrorResultCode)
			{
			case CHAT_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_SUCCESS));	wszHeader = NULL;
				break;

			case CHAT_ERROR_MEMBER_NOT_A_SUBSCRIBER:		wszText = GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_FAILURE_NOT_A_SUBSCRIBER);	break;
			case CHAT_ERROR_INVALID_PERMISSION_LEVEL:		wszText = GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_FAILURE_INVALID_RANK);		break;
			case CHAT_ERROR_MEMBER_NOT_IN_A_GUILD:			wszText = GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_FAILURE_NOT_IN_GUILD);		break;
			case CHAT_ERROR_GUILD_ACTION_PERMISSION_DENIED:	wszText = GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_FAILURE_PERMISSION_DENIED);break;
			case CHAT_ERROR_INTERNAL_ERROR:	//	unknown error
			default:										wszText = GlobalStringGet(GS_GUILD_CHANGE_ACTION_PERMISSIONS_FAILURE_UNKNOWN);	break;
			}
		}
		break;

	default:
		TraceError("Received unknown guild request type from guild action result message. Action Code: %hx", pMessage->wActionType);
		break;
	};

	if (wszHeader && wszText)
		UIShowGenericDialog(wszHeader, wszText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdUnitGuildAssociation(
	GAME * game,
	BYTE * data )
{
	MSG_SCMD_UNIT_GUILD_ASSOCIATION * msg = (MSG_SCMD_UNIT_GUILD_ASSOCIATION*)data;

	UNIT * unit = UnitGetById( game, msg->id );
	if (!unit)
		return;

	UnitAddOrUpdatePlayerGuildAssociationTag( game, unit, msg->wszGuildName, (GUILD_RANK)msg->eGuildRank, msg->wszRankName );

#if ISVERSION(DEBUG_VERSION)
	//	TEMP: temp unit association display for debugging purposes
	if (msg->wszGuildName[0])
	{
		WCHAR gname[MAX_GUILD_NAME];
		GUILD_RANK grank;
		WCHAR grankname[MAX_CHARACTER_NAME];
		UnitGetPlayerGuildAssociationTag(unit, gname, MAX_GUILD_NAME, grank, grankname, MAX_CHARACTER_NAME);
		WCHAR unitName[MAX_CHARACTER_NAME];
		UnitGetName( unit, unitName, MAX_CHARACTER_NAME, 0 );

		UIAddChatLineArgs(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_GUILD), L"Player %s is a %s of guild %s.", unitName, grankname, gname);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdPvPActionResult(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_PVP_ACTION_RESULT *pMessage = (const MSG_SCMD_PVP_ACTION_RESULT*)pData;

	const WCHAR * wszHeader = NULL;
	const WCHAR * wszText   = NULL;

	switch (pMessage->wActionType)
	{
	//------------------------------------------------------------------------
	//-- PvP ENABLE RESULTS ----------------------------------------------
	//------------------------------------------------------------------------
	case PVP_ACTION_ENABLE:
		{
 			wszHeader = GlobalStringGet(GS_PVP_ENABLE_FAILURE_HEADER);
			switch (pMessage->wActionErrorType)
			{
			case PVP_ACTION_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_PVP_ENABLE_SUCCESS));	wszHeader = NULL;
				break;

			case PVP_ACTION_ERROR_ALREADY_ENABLED:		wszText = GlobalStringGet(GS_PVP_ENABLE_FAILURE_ALREADY_ENABLED);		break;
			default:									wszText = GlobalStringGet(GS_PVP_ENABLE_FAILURE_UNKNOWN);				break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- PvP DISABLE RESULTS ----------------------------------------------
	//------------------------------------------------------------------------
	case PVP_ACTION_DISABLE:
		{
 			wszHeader = GlobalStringGet(GS_PVP_DISABLE_FAILURE_HEADER);
			switch (pMessage->wActionErrorType)
			{
			case PVP_ACTION_ERROR_NONE:	//	success
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_PVP_DISABLE_SUCCESS));	wszHeader = NULL;
				break;
			//	

			case PVP_ACTION_ERROR_IN_A_MATCH:			wszText = GlobalStringGet(GS_PVP_DISABLE_FAILURE_IN_A_MATCH);			break;
 			default:									wszText = GlobalStringGet(GS_PVP_DISABLE_FAILURE_UNKNOWN);				break;
			}
		}
		break;

	//------------------------------------------------------------------------
	//-- PvP DISABLE DELAY RESULTS ----------------------------------------------
	//------------------------------------------------------------------------
	case PVP_ACTION_DISABLE_DELAY:
		{
			switch (pMessage->wActionErrorType)
			{
			case PVP_ACTION_ERROR_NONE:	//	success
				if (PVP_DISABLE_DELAY_IN_SECONDS == 1)
				{
					UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), GlobalStringGet(GS_PVP_DISABLE_SUCCESS_DELAY_1_SECOND));	
				}
				else
				{
					const WCHAR *wszGlobalString = GlobalStringGet(GS_PVP_DISABLE_SUCCESS_DELAY);	
					if (wszGlobalString)
					{
						WCHAR wszTextDigitsTemp[32]; // I put %s in the global by accident instead of %d
						WCHAR wszTextTemp[1024];
						PStrPrintf(wszTextDigitsTemp, arrsize(wszTextDigitsTemp), L"%d", PVP_DISABLE_DELAY_IN_SECONDS);
						PStrPrintf(wszTextTemp, arrsize(wszTextTemp), wszGlobalString, wszTextDigitsTemp);
						UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), wszTextTemp);
					}
				}
				wszHeader = NULL;
				break;
			}
		}
		break;

	default:
		TraceError("Received unknown PvP request type from PvP action result message. Action Code: %hx", pMessage->wActionType);
		break;
	};

	if (wszHeader && wszText)
		UIShowGenericDialog(wszHeader, wszText);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdDuelInvite(
	GAME *pGame,
	BYTE *pData)
{
	ASSERT_RETURN(pGame);
	const MSG_SCMD_DUEL_INVITE *pMessage = (const MSG_SCMD_DUEL_INVITE*)pData;
	ASSERT_RETURN(pMessage);
	UNIT *playerInviting = UnitGetById(pGame, pMessage->idOpponent);
	if (playerInviting)
	{
		UIShowDuelInviteDialog(playerInviting);
	}
	else
	{
		MSG_CCMD_DUEL_INVITE_FAILED msg;
		msg.idOpponent = pMessage->idOpponent;
		msg.wFailReasonInviter = DUEL_ILLEGAL_REASON_UNKNOWN; // opponent can't find inviter, not a very interesting error message for the inviter
		msg.wFailReasonInvitee = DUEL_ILLEGAL_REASON_UNKNOWN;
		c_SendMessage(CCMD_DUEL_INVITE_FAILED, &msg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdDuelInviteDecline(
	GAME *pGame,
	BYTE *pData)
{
	ASSERT_RETURN(pGame);
	const MSG_SCMD_DUEL_INVITE_DECLINE *pMessage = (const MSG_SCMD_DUEL_INVITE_DECLINE*)pData;
	ASSERT_RETURN(pMessage);
	UNIT *playerDeclining = UnitGetById(pGame, pMessage->idOpponent);
	if (playerDeclining)
	{
		const WCHAR *wszGlobalString = GlobalStringGet(GS_PVP_DUEL_INVITE_DECLINED);	
		if (wszGlobalString)
		{
			WCHAR wszDeclinerName[MAX_CHARACTER_NAME];
			WCHAR wszText[2048];
			wszDeclinerName[0] = L'\0';
			if (UnitGetName(playerDeclining, wszDeclinerName, arrsize(wszDeclinerName), 0))
			{
				PStrPrintf(wszText, arrsize(wszText), wszGlobalString, wszDeclinerName);
				UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), wszText);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdDuelInviteFailed(
	GAME *pGame,
	BYTE *pData)
{
	ASSERT_RETURN(pGame);
	const MSG_SCMD_DUEL_INVITE_FAILED *pMessage = (const MSG_SCMD_DUEL_INVITE_FAILED*)pData;
	ASSERT_RETURN(pMessage);
	c_DuelDisplayInviteError((DUEL_ILLEGAL_REASON)pMessage->wFailReasonInviter, (DUEL_ILLEGAL_REASON)pMessage->wFailReasonInvitee);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdDuelInviteAcceptFailed(
	GAME *pGame,
	BYTE *pData)
{
	ASSERT_RETURN(pGame);
	const MSG_SCMD_DUEL_INVITE_ACCEPT_FAILED *pMessage = (const MSG_SCMD_DUEL_INVITE_ACCEPT_FAILED*)pData;
	ASSERT_RETURN(pMessage);
	c_DuelDisplayAcceptError((DUEL_ILLEGAL_REASON)pMessage->wFailReasonInviter, (DUEL_ILLEGAL_REASON)pMessage->wFailReasonInvitee);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdDuelStart(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_DUEL_START *pMessage = (const MSG_SCMD_DUEL_START*)pData;
	c_DuelStart(pMessage->wCountdownSeconds);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdDuelMessage(
					GAME *pGame,
					BYTE *pData)
{
	const MSG_SCMD_DUEL_MESSAGE *pMessage = (const MSG_SCMD_DUEL_MESSAGE*)pData;
	UNIT* pOpponent = UnitGetById( pGame, pMessage->idOpponent );
	c_DuelMessage(GameGetControlUnit( pGame ),
				  pOpponent,
				  (DUEL_MESSAGE)pMessage->nMessage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdPvPTeamStartCountdown(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_PVP_TEAM_START_COUNTDOWN *pMessage = (const MSG_SCMD_PVP_TEAM_START_COUNTDOWN*)pData;
	c_CTFStartCountdown(pMessage->wCountdownSeconds);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdPvPTeamStartMatch(
	GAME *pGame,
	BYTE *pData)
{
	//const MSG_SCMD_PVP_TEAM_START_MATCH *pMessage = (const MSG_SCMD_PVP_TEAM_START_MATCH*)pData;
	// 	c_DuelStart(pMessage->wCountdownSeconds);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdPvPTeamEndMatch(
	GAME *pGame,
	BYTE *pData)
{
	const MSG_SCMD_PVP_TEAM_END_MATCH *pMessage = (const MSG_SCMD_PVP_TEAM_END_MATCH*)pData;
	c_CTFEndMatch(pMessage->wScoreTeam0, pMessage->wScoreTeam0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sScmdGlobalThemeChange(
	GAME *pGame,
	BYTE *pData)
{
	ASSERT_RETURN(pGame);
	const MSG_SCMD_GLOBAL_THEME_CHANGE *pMessage = (const MSG_SCMD_GLOBAL_THEME_CHANGE*)pData;
	ASSERT_RETURN(pMessage);
	if ( pMessage->bEnable )
	{
		GlobalThemeEnable( pGame, pMessage->wTheme );
	} else {
		GlobalThemeDisable( pGame, pMessage->wTheme );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdCubeOpen(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_CUBE_OPEN *pMessage = (MSG_SCMD_CUBE_OPEN *)pData;
	UNIT * pCube = UnitGetById( pGame, pMessage->idCube );
	c_CubeOpen( pCube );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdBackpackOpen(
	GAME *pGame,
	BYTE *pData)
{
	MSG_SCMD_BACKPACK_OPEN *pMessage = (MSG_SCMD_BACKPACK_OPEN *)pData;
	UNIT * pBackpack = UnitGetById( pGame, pMessage->idBackpack );
	c_BackpackOpen( pBackpack );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPlaceField(
	GAME * pGame,
	BYTE * pData)
{
	MSG_SCMD_PLACE_FIELD * pMessage = (MSG_SCMD_PLACE_FIELD *)pData;
	UNIT * pOwner = UnitGetById(pGame, pMessage->idOwner);
	// For FIELD MISSILES ONLY we are allowed to use the control unit as the owner, because there's nothing special 
	// or weird that we're allowed to do with the missiles themselves
	if(!pOwner)
		pOwner = GameGetControlUnit(pGame);
	ASSERT_RETURN(pOwner);

	float fRadius = (float)pMessage->nRadius / RADIUS_DIVISOR;
	c_MissileCreateField(pGame, pOwner, pMessage->nFieldMissile, pMessage->vPosition, fRadius, pMessage->nDuration);
}

void sSCmdPlaceFieldPathNode(
	GAME * pGame,
	BYTE * pData)
{
	MSG_SCMD_PLACE_FIELD_PATHNODE * pMessage = (MSG_SCMD_PLACE_FIELD_PATHNODE *)pData;
	ROOM * pRoom = RoomGetByID(pGame, pMessage->idRoom);
	ASSERT_RETURN(pRoom);
	ROOM_PATH_NODE * pPathNode = RoomGetRoomPathNode(pRoom, pMessage->nPathNode);
	ASSERT_RETURN(pPathNode);
	UNIT * pOwner = UnitGetById(pGame, pMessage->idOwner);
	// For FIELD MISSILES ONLY we are allowed to use the control unit as the owner, because there's nothing special 
	// or weird that we're allowed to do with the missiles themselves
	if(!pOwner)
		pOwner = GameGetControlUnit(pGame);
	ASSERT_RETURN(pOwner);

	VECTOR vPosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);
	float fRadius = (float)pMessage->nRadius / RADIUS_DIVISOR;
	c_MissileCreateField(pGame, pOwner, pMessage->nFieldMissile, vPosition, fRadius, pMessage->nDuration);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sSCmdPlayed(
	GAME *pGame,
	BYTE *pData)
{
	ASSERT_RETURN(pData);
	MSG_SCMD_PLAYED * pMessage = (MSG_SCMD_PLAYED *)pData;

	DWORD dwPlayedTime = pMessage->dwPlayedTime;
	if ( dwPlayedTime )
	{
		int nDisplaySeconds = dwPlayedTime % 60;
		int nDisplayMinutes = dwPlayedTime / 60;
		int nDisplayHours = nDisplayMinutes / 60;
		nDisplayMinutes -= nDisplayHours * 60;
		int nDisplayDays = nDisplayHours / 24;
		nDisplayHours -= nDisplayDays * 24;
		int nDisplayWeeks = nDisplayDays / 7;
		nDisplayDays -= nDisplayWeeks * 7;

		//L"Played Time: %d weeks, %d days, %d hours, %d minutes, %d seconds"
		UIAddChatLineArgs (CHAT_TYPE_LOCALIZED_CMD, ChatGetTypeColor(CHAT_TYPE_SERVER),
			GlobalStringGet(GS_PLAYED), nDisplayWeeks, 
			nDisplayDays, nDisplayHours, nDisplayMinutes, nDisplaySeconds );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdEmailResult(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_EMAIL_RESULT * msg = (MSG_SCMD_EMAIL_RESULT*)pData;
	c_PlayerEmailHandleResult(msg->actionType, msg->actionResult, (EMAIL_SPEC_SOURCE)msg->actionSenderContext);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdEmailMetadata(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_EMAIL_METADATA * msg = (MSG_SCMD_EMAIL_METADATA*)pData;
	c_PlayerEmailHandleMetadata(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdEmailData(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_EMAIL_DATA * msg = (MSG_SCMD_EMAIL_DATA*)pData;
	c_PlayerEmailHandleData(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdEmailUpdate(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_EMAIL_UPDATE * msg = (MSG_SCMD_EMAIL_UPDATE*)pData;
	c_PlayerEmailHandleDataUpdate(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionError(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_ERROR * msg = (MSG_SCMD_AH_ERROR*)pData;
	UIAuctionError(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionInfo(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_INFO * msg = (MSG_SCMD_AH_INFO*)pData;
	UIAuctionInfo(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionSearchResult(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_SEARCH_RESULT * msg = (MSG_SCMD_AH_SEARCH_RESULT*)pData;
	UIAuctionSearchResult(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionSearchResultNext(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_SEARCH_RESULT_NEXT * msg = (MSG_SCMD_AH_SEARCH_RESULT_NEXT*)pData;
	UIAuctionSearchResultNext(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionPlayerItemSaleList(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_PLAYER_ITEM_SALE_LIST * msg = (MSG_SCMD_AH_PLAYER_ITEM_SALE_LIST*)pData;
	UIAuctionPlayerItemSaleList(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionSearchResultItemInfo(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_SEARCH_RESULT_ITEM_INFO * msg = (MSG_SCMD_AH_SEARCH_RESULT_ITEM_INFO*)pData;
	UIAuctionSearchResultItemInfo(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdAuctionSearchResultItemBlob(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_AH_SEARCH_RESULT_ITEM_BLOB * msg = (MSG_SCMD_AH_SEARCH_RESULT_ITEM_BLOB*)pData;
	UIAuctionSearchResultItemBlob(msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSCmdGlobalLocalizedAnnouncement(
	GAME * pGame,
	BYTE * pData )
{
	MSG_SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT * msg = (MSG_SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT*)pData;
	WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
	PStrCopy( newText, 
		GlobalStringGet( (GLOBAL_STRING)msg->tLocalizedParamString.eGlobalString),
		MAX_STRING_ENTRY_LENGTH );
	if(msg->tLocalizedParamString.param1.eStringReplacement != -1)
	{
		PStrReplaceToken(newText, MAX_STRING_ENTRY_LENGTH, 
			StringReplacementTokensGet((STRING_REPLACEMENT)
			msg->tLocalizedParamString.param1.eStringReplacement),
			(WCHAR *)msg->tLocalizedParamString.param1.GetParam());
	}
	if(msg->tLocalizedParamString.param2.eStringReplacement != -1)
	{
		PStrReplaceToken(newText, MAX_STRING_ENTRY_LENGTH, 
			StringReplacementTokensGet((STRING_REPLACEMENT)
			msg->tLocalizedParamString.param2.eStringReplacement),
			(WCHAR *)msg->tLocalizedParamString.param2.GetParam());
	}

	UIAddChatLine(CHAT_TYPE_QUEST, ChatGetTypeColor(CHAT_TYPE_QUEST),
		newText );

	if (msg->tLocalizedParamString.bPopup)
	{
		UIShowQuickMessage(newText, 5.0f);
	}
}

//----------------------------------------------------------------------------
typedef void FP_PROCESS_MESSAGE(GAME * game, BYTE * data);

//----------------------------------------------------------------------------
struct PROCESS_MESSAGE_STRUCT
{
	NET_CMD					cmd;
	FP_PROCESS_MESSAGE *	fp;
	BOOL					bNeedGame;
	BOOL					bImmediateCallback;
	const char *			szFuncName;	
};

#if ISVERSION(DEVELOPMENT)
#define SRV_MESSAGE_PROC(c, f, g, imm)		{ c, f, g, imm, #f },
#else
#define SRV_MESSAGE_PROC(c, f, g, imm)		{ c, f, g, imm, "" },
#endif

PROCESS_MESSAGE_STRUCT gfpServerMessageHandler[] = 
{	//																				need game	immediate callback																						/
	SRV_MESSAGE_PROC(SCMD_YOURID,						sSCmdYourId,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ERROR,						sSCmdError,						FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GAME_NEW,						sSCmdGameNew,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SET_LEVEL_COMMUNICATION_CHANNEL,sSCmdSetLevelCommChannel,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TEAM_NEW,						sSCmdTeamNew,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TEAM_DEACTIVATE,				sSCmdTeamDeactivate,			FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TEAM_JOIN,					sSCmdTeamJoin,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GAME_CLOSE,					sSCmdGameClose,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_OPEN_PLAYER_INITSEND,			sSCmdOpenPlayerInitSend,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_OPEN_PLAYER_SENDFILE,			sSCmdOpenPlayerSendFile,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_START_LEVEL_LOAD,				sSCmdStartLevelLoad,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SET_LEVEL,					sSCmdSetLevel,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SET_CONTROL_UNIT,				sSCmdSetControlUnit,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PLAYERNEW,					sSCmdPlayerNew,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PLAYERREMOVE,					sSCmdPlayerRemove,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_MISSILENEWXYZ,				sSCmdMissileNewXYZ,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_MISSILENEWONUNIT,				sSCmdMissileNewOnUnit,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UPDATE,						sSCmdUpdate,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITTURNID,					sSCmdUnitTurnId,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITTURNXYZ,					sSCmdUnitTurnXYZ,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITDIE,						sSCmdUnitDie,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ROOM_ADD,						sSCmdRoomAdd,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ROOM_ADD_COMPRESSED,			sSCmdRoomAddCompressed,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ROOM_UNITS_ADDED,				sSCmdRoomUnitsAdded,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ROOM_REMOVE,					sSCmdRoomRemove,				TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_LOADCOMPLETE,					sSCmdLoadComplete,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNIT_NEW,						sSCmdUnitNew,					TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_UNIT_REMOVE,					sSCmdUnitRemove,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNIT_DAMAGED,					sSCmdUnitDamaged,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITSETSTAT,					sSCmdUnitSetStat,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITSETSTATE,					sSCmdUnitSetState,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITCLEARSTATE,				sSCmdUnitClearState,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNIT_SET_FLAG,				sSCmdUnitSetFlag,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITWARP,						sSCmdUnitWarp,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITDROP,						sSCmdUnitDrop,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNIT_PATH_POSITION_UPDATE,	sSCmdUnitPathPosUpdate,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_INVENTORY_LINK_TO,			sSCmdInventoryLinkTo,			TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_REMOVE_ITEM_FROM_INV,			sSCmdRemoveItemFromInv,			TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_CHANGE_INV_LOCATION,			sSCmdChangeInvLocation,			TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_SWAP_INV_LOCATION,			sSCmdSwapInvLocation,			TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_INV_ITEM_UNUSABLE,			sSCmdInvItemUnusable,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_INV_ITEM_USABLE,				sSCmdInvItemUsable,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_INVENTORY_ADD_PET,			sSCmdInventoryAddPet,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_INVENTORY_REMOVE_PET,			sSCmdInventoryRemovePet,		TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_HOTKEY,						sSCmdHotkey,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_CLEAR_INV_WAITING,			sSCmdClearUnitWaitingForInvMove,TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITMODE,						sSCmdUnitMode,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITMODEEND,					sSCmdUnitModeEnd,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITPOSITION,					sSCmdUnitPosition,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITMOVEXYZ,					sSCmdUnitMoveXYZ,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITMOVEID,					sSCmdUnitMoveID,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITMOVEDIRECTION,			sSCmdUnitMoveDirection,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITPATHCHANGE,				sSCmdUnitPathChange,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITJUMP,						sSCmdUnitJump,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITWARDROBE,					sSCmdUnitWardrobe,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITADDIMPACT,				sSCmdUnitAddImpact,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SKILLSTART,					sSCmdSkillStart,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SKILLSTARTXYZ,				sSCmdSkillStartXYZ,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SKILLSTARTID,					sSCmdSkillStartID,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SKILLCHANGETARGETID,			sSCmdSkillChangeTargetID,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SKILLCHANGETARGETLOC,			sSCmdSkillChangeTargetLocation,	TRUE,	TRUE)
	SRV_MESSAGE_PROC(SCMD_SKILLCHANGETARGETLOCMULTI,	sSCmdSkillChangeMultipleTargetLocations,	TRUE,	TRUE)
	SRV_MESSAGE_PROC(SCMD_SKILLSTOP,					sSCmdSkillStop,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SKILLCOOLDOWN,				sSCmdSkillCooldown,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SYS_TEXT,						sSCmdSysText,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UISHOWMESSAGE,				sSCmdUIShowMessage,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_STATEADDTARGET,				sSCmdStateAddTarget,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_STATEREMOVETARGET,			sSCmdStateRemoveTarget,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_STATEADDTARGETLOCATION,		sSCmdStateAddTargetLocation,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_CRAFT_BREAKDOWN,				sSCmdUnitCraftingBreakdown,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITPLAYSOUND,				sSCmdUnitPlaySound,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AUDIO_DIALOG_PLAY,			sSCmdAudioDialogPlay,			TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_PLAYERMOVE,					sSCmdPlayerMove,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNITSETOWNER,					sSCmdUnitsetOwner,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AVAILABLE_QUESTS,				sSCmdAvailableQuests,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_RECIPE_LIST_OPEN,				sSCmdRecipeListOpen,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_RECIPE_OPEN,					sSCmdRecipeOpen,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_RECIPE_FORCE_CLOSED,			sSCmdRecipeForceClosed,			TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_RECIPE_CREATED,				sSCmdRecipeCreated,				TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_AVAILABLE_TASKS,				sSCmdAvailableTasks,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_STATUS,					sSCmdTaskStatus,				TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_POINT_OF_INTEREST,			sSCmdPointOfInterestUpdate,		TRUE,	FALSE) //tugboat
	SRV_MESSAGE_PROC(SCMD_ANCHOR_MARKER,				sSCmdAnchorPointAddedToLevel,	TRUE,	FALSE) //tugboat
	SRV_MESSAGE_PROC(SCMD_QUEST_TASK_STATUS,			sSCmdQuestTaskStatus,			TRUE,	FALSE) //tugboat
	SRV_MESSAGE_PROC(SCMD_LEVELAREA_MAP_MESSAGE,		sSCmdLevelAreaMapMessage,		TRUE,	FALSE) //tugboat
	SRV_MESSAGE_PROC(SCMD_TASK_OFFER_REWARD,			sSCmdTaskOfferReward,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_CLOSE,					sSCmdTaskClose,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_RESTORE,					sSCmdTaskRestore,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_RESTRICTED_BY_FACTION,	sSCmdTaskRestrictedByFaction,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_INCOMPLETE,				sSCmdTaskIncomplete,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ENTER_IDENTIFY,				sSCmdEnterIdentify,				TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_CANNOT_USE,					sSCmdCannotUse,					TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_CANNOT_PICKUP,				sSCmdCannotPickup,				TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_CANNOT_DROP,					sSCmdCannotDrop,				TRUE,	FALSE)			
	SRV_MESSAGE_PROC(SCMD_HEADSTONE_INFO,				sSCmdHeadstoneInfo,				TRUE,	FALSE)			
	SRV_MESSAGE_PROC(SCMD_DUNGEON_WARP_INFO,			sSCmdDungeonWarpInfo,			TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_CREATEGUILD_INFO,				sSCmdCreateGuildInfo,			TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_RESPEC_INFO,					sSCmdRespecInfo,				TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_TRANSPORTER_WARP_INFO,		sSCmdTransporterWarpInfo,		TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_PVP_SIGNER_UPPER_INFO,		sSCmdPvPSignerUpperInfo,		TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_HIRELING_SELECTION,			sSCmdHirelingSelection,			TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_TALK_START,					sSCmdTalkStart,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EXAMINE,						sSCmdExamine,					TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_INSPECT,						sSCmdInspect,					TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_VIDEO_START,					sSCmdVideoStart,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EMOTE_START,					sSCmdEmoteStart,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_INTERACT_INFO,				sSCmdInteractInfo,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_LEVELMARKER,					sSCmdLevelMarker,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_NPC_UPDATE,				sSCmdQuestNPCUpdate,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_STATE,					sSCmdQuestState,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_STATUS,					sSCmdQuestStatus,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_END_QUEST_SETUP,				sSCmdEndQuestSetup,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PLAY_MOVIELIST,				sSCmdPlayMovielist,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_DISPLAY_DIALOG,			sSCmdQuestDisplayDialog,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_TEMPLATE_RESTORE,		sSCmdQuestTemplateRestore,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_OPERATE_RESULT,				sSCmdOperateResult,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_BLOCKING_STATE,				sSCmdBlockingState,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TOGGLE_UI_ELEMENT,			sSCmdToggleUIElement,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SHOW_MAP_ATLAS,				sSCmdShowMapAtlas,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SHOW_RECIPE_PANE,				sSCmdShowRecipePane,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TUTORIAL,						sSCmdTutorial,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_REWARD_TAKEN,			sSCmdTaskRewardTaken,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TASK_EXTERMINATED_COUNT,		sSCmdTaskExterminatedCount,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UPDATE_NUM_REWARD_ITEMS_TAKEN,sSCmdUpdateNumRewardItemsTaken,	TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_TRADE_START,					sSCmdTradeStart,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TRADE_FORCE_CANCEL,			sSCmdTradeForceCancel,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TRADE_STATUS,					sSCmdTradeStatus,				TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_TRADE_CANNOT_ACCEPT,			sSCmdTradeCannotAccept,			TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_TRADE_REQUEST_NEW,			sSCmdTradeRequestNew,			TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_TRADE_REQUEST_NEW_CANCEL,		sSCmdTradeRequestNewCancel,		TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_TRADE_MONEY,					sSCmdTradeMoney,				TRUE,	FALSE)			
	SRV_MESSAGE_PROC(SCMD_TRADE_ERROR,					sSCmdTradeError,				TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_WALLWALK_ORIENT,				sSCmdWallWalkOrient,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EFFECT_AT_UNIT,				sSCmdEffectAtUnit,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EFFECT_AT_LOCATION,			sSCmdEffectAtLocation,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_OPERATE_WAYPOINT_MACHINE,		sSCmdOperateWaypointMachine,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_OPERATE_RUNEGATE,				sSCmdOperateRunegate,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_JUMP_BEGIN,					sSCmdJumpBegin,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_JUMP_END,						sSCmdJumpEnd,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GAME_INSTANCE_UPDATE_BEGIN,	sSCmdStartGameInstanceUpdate,	FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GAME_INSTANCE_UPDATE_END,		sSCmdEndGameInstanceUpdate,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GAME_INSTANCE_UPDATE,			sSCmdGameInstanceUpdate,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_TOWN_PORTAL_CHANGED,			sSCmdTownPortal,				FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ENTER_TOWN_PORTAL,			sSCmdEnterTownPortal,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SELECT_RETURN_DESTINATION,	sSCmdSelectReturnDestination,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_REMOVING_FROM_RESERVED_GAME,	sSCmdRemovingFromReservedGame,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PARTY_MEMBER_INFO,			sSCmdPartyMemberInfo,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PARTY_ADVERTISE_RESULT,		sSCmdPartyAdResult,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PARTY_ACCEPT_INVITE_CONFIRM,	sCCmdPartyAcceptInviteConfirm,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PARTY_JOIN_CONFIRM,			sCCmdPartyJoinConfirm,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_MISSILES_CHANGEOWNER,			sSCmdMissilesChangeOwner,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_CONTROLUNIT_FINISHED,			sSCmdControlUnitFinished,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PING,							sSCmdPing,						FALSE,	TRUE)
	SRV_MESSAGE_PROC(SCMD_INTERACT_CHOICES,				sSCmdInteractChoices,			FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_CONSOLE_MESSAGE,				sSCmdConsoleMessage,			FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_CONSOLE_DIALOG_MESSAGE,		sSCmdConsoleDialogMessage,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DPS_INFO,						sSCmdDPSInfo,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_RADAR,					sSCmdQuestRadar,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_RIDE_START,					sSCmdRideStart,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_RIDE_END,						sSCmdRideEnd,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_MUSIC_INFO,					sSCmdMusicInfo,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ITEM_UPGRADED,				sSCmdItemUpgraded,				TRUE,	FALSE)			
	SRV_MESSAGE_PROC(SCMD_ITEM_AUGMENTED,				sSCmdItemAugmented,				TRUE,	FALSE)				
	SRV_MESSAGE_PROC(SCMD_REAL_MONEY_TXN_RESULT,		sSCmdRealMoneyTxnResult,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_ITEMS_RESTORED,				sSCmdItemsRestored,				FALSE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_WARP_RESTRICTED,				sSCmdWarpRestricted,			FALSE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_CURSOR_ITEM_REFERENCE,		sSCmdCursorItemReference,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_QUEST_CLEAR_STATES,			sSCmdQuestClearStates,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_SUBLEVEL_STATUS,				sSCmdSubLevelStatus,			TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_DBUNIT_LOG,					sSCmdDBUnitLog,					TRUE,	FALSE)	
	SRV_MESSAGE_PROC(SCMD_DBUNIT_LOG_STATUS,			sSCmdDBUnitLogStatus,			TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_UNITSETSTATINLIST,			sSCmdUnitSetStatInList,			TRUE,	FALSE)		
	SRV_MESSAGE_PROC(SCMD_GUILD_ACTION_RESULT,			sScmdGuildActionResult,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_UNIT_GUILD_ASSOCIATION,		sSCmdUnitGuildAssociation,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PVP_ACTION_RESULT,			sScmdPvPActionResult,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DUEL_INVITE,					sScmdDuelInvite,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DUEL_INVITE_DECLINE,			sScmdDuelInviteDecline,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DUEL_INVITE_FAILED,			sScmdDuelInviteFailed,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DUEL_INVITE_ACCEPT_FAILED,	sScmdDuelInviteAcceptFailed,	TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DUEL_START,					sScmdDuelStart,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_DUEL_MESSAGE,					sScmdDuelMessage,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PVP_TEAM_START_COUNTDOWN,		sScmdPvPTeamStartCountdown,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PVP_TEAM_START_MATCH,			sScmdPvPTeamStartMatch,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PVP_TEAM_END_MATCH,			sScmdPvPTeamEndMatch,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GLOBAL_THEME_CHANGE,			sScmdGlobalThemeChange,			TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_CUBE_OPEN,					sSCmdCubeOpen,					TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_BACKPACK_OPEN,				sSCmdBackpackOpen,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PLACE_FIELD,					sSCmdPlaceField,				TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PLACE_FIELD_PATHNODE,			sSCmdPlaceFieldPathNode,		TRUE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_PLAYED,						sSCmdPlayed,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EMAIL_RESULT,					sSCmdEmailResult,				FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EMAIL_METADATA,				sSCmdEmailMetadata,				FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EMAIL_DATA,					sSCmdEmailData,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_EMAIL_UPDATE,					sSCmdEmailUpdate,				FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT,sSCmdGlobalLocalizedAnnouncement,	FALSE,	FALSE)

	SRV_MESSAGE_PROC(SCMD_AH_INFO,						sSCmdAuctionInfo,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AH_ERROR,						sSCmdAuctionError,					FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AH_PLAYER_ITEM_SALE_LIST,		sSCmdAuctionPlayerItemSaleList,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AH_SEARCH_RESULT,				sSCmdAuctionSearchResult,			FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AH_SEARCH_RESULT_NEXT,		sSCmdAuctionSearchResultNext,		FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AH_SEARCH_RESULT_ITEM_INFO,	sSCmdAuctionSearchResultItemInfo,	FALSE,	FALSE)
	SRV_MESSAGE_PROC(SCMD_AH_SEARCH_RESULT_ITEM_BLOB,	sSCmdAuctionSearchResultItemBlob,	FALSE,	FALSE)
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CltValidateMessageTable(
	void)
{
	ASSERT_RETFALSE(arrsize(gfpServerMessageHandler) == SCMD_LAST);
	for (unsigned int ii = 0; ii < arrsize(gfpServerMessageHandler); ii++)
	{
		ASSERT_RETFALSE(gfpServerMessageHandler[ii].cmd == ii);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CltProcessMessage(
	MSG_STRUCT * msg)
{
	NET_CMD command = msg->hdr.cmd;
	ASSERT_RETURN(command >= 0 && command < SCMD_LAST);
	ASSERT_RETURN(gfpServerMessageHandler[command].fp);

	NetTraceRecvMessage(gApp.m_GameServerCommandTable, msg, "CLTRCV: ");

	GAME * game = AppGetCltGame();

	ASSERT_RETURN(game || !gfpServerMessageHandler[command].bNeedGame);

	gfpServerMessageHandler[command].fp(game, (BYTE*)msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CltProcessMessages(
	LONG max_time) // in milliseconds
{
	PERF(CLT_MSG);
	LONG cur_time, new_time;
	cur_time = GetRealClockTime();

	// remove all the messages and process them
	MSG_BUF * head, * tail;
	unsigned int count = MailSlotGetMessages(AppGetGameMailSlot(), head, tail);
	REF(count);

	MSG_BUF * msg = NULL;
	MSG_BUF * next = head;
	while ((msg = next) != NULL)
	{
		next = msg->next;

		BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];
		if (NetTranslateMessageForRecv(gApp.m_GameServerCommandTable, msg->msg, msg->size, (MSG_STRUCT *)message))
		{
			CltProcessMessage((MSG_STRUCT *)message);
		}

		new_time = GetRealClockTime();

		GAME * game = AppGetCltGame();
		if ((game && GameGetState(game) == GAMESTATE_REQUEST_SHUTDOWN) ||
			((max_time != 0) && (new_time - cur_time > max_time)))
		{
			// put rest of messages back on front of mailbox & set up head, tail for recycling (tail is current message)
			MSG_BUF * unprocessed_head = msg->next;
			MSG_BUF * unprocessed_tail = tail;
			tail = msg;
			msg->next = NULL;
			if (unprocessed_head)
			{
				MailSlotPushbackMessages(AppGetGameMailSlot(), unprocessed_head, unprocessed_tail);
			}
			break;
		}
	}

	MailSlotRecycleMessages(AppGetGameMailSlot(), head, tail);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientProcessImmediateMessage(
	struct NETCOM *,
	NETCLIENTID, 
	BYTE * data,
	unsigned int size)
{
	BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];

	if(NetTranslateMessageForRecv(
		gApp.m_GameServerCommandTable, data, size, (MSG_STRUCT *)message))
	{
		CltProcessMessage((MSG_STRUCT *)message);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientRegisterImmediateMessages()
{
	for(NET_CMD command = 0; command < SCMD_LAST; command++)
	{
		if(gfpServerMessageHandler[command].bImmediateCallback)
			NetCommandTableRegisterImmediateMessageCallback(
			gApp.m_GameServerCommandTable,
			command,
			ClientProcessImmediateMessage);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
void sServerSelecitonExchangeFunc(SettingsExchange & se, void * pContext)
{
	SettingsExchange_Do(se, (char*)pContext, MAX_XML_STRING_LENGTH, "DefaultServer");
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AppLoadServerSelection(char * szSeverName)
{
	Settings_Load("Network", &sServerSelecitonExchangeFunc, szSeverName);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void AppSaveServerSelection(T_SERVER * pServerDef)
{
	Settings_Save("Network", &sServerSelecitonExchangeFunc, &pServerDef->szOperationalName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetServerSelection(T_SERVER* pServerDef)
{
	if (gApp.pServerSelection != pServerDef)
	{
		gApp.pServerSelection = pServerDef;
		AppSaveServerSelection(pServerDef);
	}
}

#endif //!SERVER_VERSION
