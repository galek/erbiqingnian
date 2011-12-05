//----------------------------------------------------------------------------
// player.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "player.h"
#include "characterclass.h"
#include "prime.h"
#include "dungeon.h"
#include "globalindex.h"
#include "units.h"
#include "excel_private.h"
#include "s_gdi.h"
#include "clients.h"
#include "c_message.h"
#include "c_trade.h"
#include "console_priv.h"
#include "gameglobals.h"
#include "gameunits.h"
#include "unit_priv.h"
#include "inventory.h"
#include "c_animation.h"
#include "wardrobe.h"
#include "items.h"
#include "c_appearance.h" 
#include "s_message.h"
#include "uix_components.h"
#include "uix_menus.h"
#include "quests.h"
#include "skills.h"
#include "c_camera.h"
#include "unitmodes.h"
#include "teams.h"
#include "unitidmap.h"
#include "unittag.h"
#include "treasure.h"
#include "states.h"
#include "objects.h"
#include "console.h"
#include "c_sound.h"
#include "c_footsteps.h"
#include "filepaths.h"
#include "script.h"
#include "appcommontimer.h"
#include "ai.h"
#include "picker.h"
#include "e_model.h"
#include "e_main.h"
#include "e_settings.h"
#include "movement.h"
#include "quests.h"
#include "waypoint.h"
#include "customgame.h"
#include "ctf.h"
#include "warp.h"
#include "s_townportal.h"
#include "dbhellgate.h"
#include "c_ui.h"
#include "windowsmessages.h"
#include "weaponconfig.h"
#include "s_quests.h"
#include "svrstd.h"
#include "GameServer.h"
#include "UserChatCommunication.h"
#include "s_message.h"
#include "s_network.h"
#include "wardrobe_priv.h"
#include "room_path.h"
#include "s_partycache.h"
#include "party.h"
#include "pets.h"
#include "tasks.h"
#include "openlevel.h"
#include "s_store.h"
#include "c_tasks.h"
#include "weather.h"
#include "unitfilecommon.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "guidcounters.h"
#include "Quest.h"	//tugboat only
#include "bookmarks.h"
#include "unitfile.h"
#include "s_recipe.h"
#include "excel_private.h"
#include "hotkeys.h"
#include "s_questgames.h"
#include "GameChatCommunication.h"
#include "quest_tutorial.h"
#include "Currency.h"
#include "s_reward.h"
#include "s_itemupgrade.h"
#include "offer.h"
#include "items.h"
#include "gameglobals.h"
#include "player_move_msg.h"
#include "antifatigue.h"
#include "levelareas.h"
#include "stringhashkey.h"
#include "fasttraversehash.h"
#include "s_network.h"
#include "duel.h"
#include "achievements.h"
#include "unitfileversion.h"
#include "inventory.h"
#include "s_crafting.h"
#include "waypoint.h"
#include "partymatchmaker.h"
#include "demolevel.h"
#include "gamevariant.h"
#include "PointsOfInterest.h"
#include "c_QuestClient.h"
#include "unit_metrics.h"
#if !ISVERSION(SERVER_VERSION)
#include "c_recipe.h"
#include "c_chatNetwork.h"
#include "c_chatBuddy.h"
#include "c_chatIgnore.h"
#include "c_townportal.h"
#include "uix.h"
#include "uix_components_complex.h"
#include "uix_social.h"
#include "uix_chat.h"
#include "c_itemupgrade.h"
#include "chat.h"
#include "c_input.h"
#include "c_chatNetwork.h"
#endif
#if ISVERSION(SERVER_VERSION)
#include "player.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
BOOL gbSkipTutorial = FALSE;
static const float ROOM_UPDATE_DIST = 4.0f;
static const float ROOM_UPDATE_DIST_SQ = ROOM_UPDATE_DIST * ROOM_UPDATE_DIST;
static const BOOL sgbEnableExperienceDeathPenalty = TRUE;
static const int EXPERIENCE_PENALTY_UPDATE_RATE_IN_TICKS = GAME_TICKS_PER_SECOND;
const BOOL gbEnablePlayerJoinLog = TRUE;
const DWORD PARTY_VALIDATION_DELAY_IN_TICKS = GAME_TICKS_PER_SECOND * 30;

const DWORD MINIGAME_TIMEOUT = GAME_TICKS_PER_SECOND * SECONDS_PER_MINUTE * 20;
const DWORD MINIGAME_CHECK_TIMEOUT_FREQ	= GAME_TICKS_PER_SECOND * 10;

//----------------------------------------------------------------------------
struct CPLAYER
{
	UNITID idPlayer;
	CPLAYER *pNext;
	CPLAYER *pPrev;
};

struct MINIGAME_STATUS {
	BOOL	m_bAchieved;
	int		m_nGoalCategory;
	int		m_nGoalType;
	int		m_nNeeded;
};

struct GUILD_LIST_ENTRY : PLAYER_GUILD_MEMBER_DATA
{
	CStringHashKey<WCHAR, MAX_CHARACTER_NAME> id;
	GUILD_LIST_ENTRY * next;
	GUILD_LIST_ENTRY * traverse[2];
};

//----------------------------------------------------------------------------
struct CLIENT_PLAYER_GLOBALS
{
	float		fViewAngle;		// new client angle
	float		fViewPitch;		// new client z pitch angle

	UNITMODE	eForwardMode;
	UNITMODE	eSideMode;
	int			nMovementKeys;	// current input
	float		fBob;			// up and down bob value
	int			nBobCount;
	float		fBobTime;
	int			nModelId;

	float		fRotationalVelocity;	// used if rotation via keyboard

	BOOL		bDetachedCamera;
	int			nDetachedCameraViewAngle;
	int			nDetachedCameraViewPitch;
	float		fDetachedCameraViewAngle;
	float		fDetachedCameraViewPitch;
	VECTOR		vDetachedCameraPosition;
	int			nDetachedCameraMovement;
	TIME		tiDetachedCameraLastMovement;

	UNITID		idLastInteractedUnit;
	BOOL		bInCSRChat;

	CPLAYER *pCPlayerList;

	PGUID					 idPartyInviter;
	WCHAR					 szBuddyInviter[MAX_CHARACTER_NAME];
	CHAT_PARTY_INFO			 tPartyInfo;
	CHAT_MEMBER_INFO		 tPartyMembers[MAX_CHAT_PARTY_MEMBERS];
	COMMON_PARTY_CLIENT_DATA tPartyClientData;

	CHANNELID				idGuildChatChannel;
	WCHAR					wszGuildName[MAX_CHAT_CNL_NAME];
	GUILD_RANK				eGuildRank;
	PGUID					idGuildLeaderCharacterGuid;
	WCHAR					wszGuildLeaderName[MAX_CHARACTER_NAME];

	WCHAR					wszGuildLeaderRankName[MAX_CHARACTER_NAME];
	WCHAR					wszGuildOfficerRankName[MAX_CHARACTER_NAME];
	WCHAR					wszGuildMemberRankName[MAX_CHARACTER_NAME];
	WCHAR					wszGuildRecruitRankName[MAX_CHARACTER_NAME];

	GUILD_RANK				eMinPromoteRank;
	GUILD_RANK				eMinDemoteRank;
	GUILD_RANK				eMinEmailRank;
	GUILD_RANK				eMinInviteRank;
	GUILD_RANK				eMinBootRank;

	CFastTraverseHash<
		GUILD_LIST_ENTRY,
		CStringHashKey<WCHAR, MAX_CHARACTER_NAME>,
		2048>				tGuildListHash;
	DWORD					dwGuildListHashCount;
};


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define INIT_PLAYER_ANGLE						270
#define PLAYER_BACK_VELOCITY_MULTIPLIER			0.8f
#define PLAYER_STRAFE_VELOCITY_MULTIPLIER		0.9f

#define PLAYER_MOVEMENT_MASK					(PLAYER_MOVE_FORWARD | PLAYER_MOVE_BACK | PLAYER_ROTATE_LEFT | PLAYER_ROTATE_RIGHT | PLAYER_STRAFE_LEFT | PLAYER_STRAFE_RIGHT)

#define GUILD_COST								50000
#define RESPEC_COST								10000
#define MAX_RESPEC_COST							9999999

//----------------------------------------------------------------------------
// CLIENT ONLY CONSTANTS
//----------------------------------------------------------------------------
#define ROTATION_PITCH_ANGLE_DELTA				( PI / ( 180.f * 6.0f ) )
#define PLAYER_KEYBOARD_ROTATE_SPEED			( PI_OVER_FOUR )


#define MAX_PLAYER_PITCH_SPEED					1
#define	MAX_PLAYER_PITCH						85
#define	MAX_PLAYER_PITCH_WITH_CANNON			10

#define BOB_RATE_PER_SECOND						450.0f
#define BOB_RATE_PER_GAME_TICK					(BOB_RATE_PER_SECOND * GAME_TICK_TIME)
#define BOB_AMPLITUDE							0.1f


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
CLIENT_PLAYER_GLOBALS sgClientPlayerGlobals;
static float sgfPlayerVelocity = PLAYER_SPEED;


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CLIENT_PLAYER_GLOBALS *sClientPlayerGlobalsGet(
	void)
{
	return &sgClientPlayerGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClientPlayerGlobalsInit(
	CLIENT_PLAYER_GLOBALS &tGlobals)
{

	tGlobals.fViewAngle = 0.0f;
	tGlobals.fViewPitch = 0.0f;
	tGlobals.eForwardMode = MODE_IDLE;
	tGlobals.eSideMode = MODE_IDLE;
	tGlobals.nMovementKeys = 0;
	tGlobals.fBob = 0.0f;
	tGlobals.nBobCount = 0;
	tGlobals.fBobTime = 0.0f;
	tGlobals.nModelId = INVALID_ID;
	tGlobals.idLastInteractedUnit = INVALID_ID;
	tGlobals.bInCSRChat = FALSE;

	tGlobals.bDetachedCamera = FALSE;
	tGlobals.nDetachedCameraViewAngle = 0;
	tGlobals.nDetachedCameraViewPitch = 0;
	tGlobals.fDetachedCameraViewAngle = 0.0f;
	tGlobals.fDetachedCameraViewPitch = 0.0f;
	tGlobals.vDetachedCameraPosition = cgvNone;
	tGlobals.nDetachedCameraMovement = 0;
	tGlobals.tiDetachedCameraLastMovement = 0;
	
	tGlobals.pCPlayerList = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerInitForGame(
	GAME *pGame)
{

	// do nothing if server
	if (IS_SERVER( pGame ))
	{
		return;
	}

#if !ISVERSION(SERVER_VERSION)
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	
	// set defaults
	sClientPlayerGlobalsInit( *pGlobals );
#endif //!ISVERSION(SERVER_VERSION)
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDestroyPointsOfInterest( 
	GAME *pGame,
	UNIT *pPlayer )
{
	if( pPlayer == NULL )
		return;
	if( pPlayer->m_pPointsOfInterest )
	{		
		MEMORYPOOL_DELETE( GameGetMemPool( pGame ), pPlayer->m_pPointsOfInterest );
		pPlayer->m_pPointsOfInterest = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerRemove( 
	GAME *pGame,
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	//destroy those points of interest
	sDestroyPointsOfInterest( pGame, pPlayer );
}	


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCPlayerListFree(
	GAME *pGame,
	CPLAYER *pCPlayer)
{
	
	CPLAYER *pNext;
	while (pCPlayer)
	{
		pNext = pCPlayer->pNext;
		GFREE( pGame, pCPlayer );
		pCPlayer = pNext;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerCloseForGame(
	GAME *pGame)
{

	// do nothing if server
	if (IS_SERVER( pGame ))
	{
		return;
	}

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	
	// free player list
	sCPlayerListFree( pGame, pGlobals->pCPlayerList );
	pGlobals->pCPlayerList = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CPLAYER *sCPlayerFind(
	UNITID idPlayer)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();

	CPLAYER *pCPlayer = pGlobals->pCPlayerList;	
	while (pCPlayer)
	{
		if (pCPlayer->idPlayer == idPlayer)
		{
			return pCPlayer;
		}
		pCPlayer = pCPlayer->pNext;
	}
	
	return NULL;  // not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerAdd(
	GAME * game,
	UNIT * unit)
{	
	ASSERT_RETURN(game);
	ASSERT_RETURN(unit);	
	CLIENT_PLAYER_GLOBALS * pGlobals = sClientPlayerGlobalsGet();	

	// get id	
	UNITID idPlayer = UnitGetId(unit);
	ASSERTX_RETURN(sCPlayerFind(idPlayer) == NULL, "Client already has player");
	
	// allocate player
	CPLAYER * pCPlayer = (CPLAYER *)GMALLOCZ(game, sizeof(CPLAYER));

	// assign data
	pCPlayer->idPlayer = idPlayer;
	
	// link to list
	if (pGlobals->pCPlayerList)
	{
		pGlobals->pCPlayerList->pPrev = pCPlayer;
	}
	pCPlayer->pNext = pGlobals->pCPlayerList;
	pGlobals->pCPlayerList = pCPlayer;
}	
//----------------------------------------------------------------------------
//Client side only
//----------------------------------------------------------------------------
static int nRoomMessage = INVALID_ID;
static BOOL bRoomMessageOkToShow = TRUE;
static int nRoomMessageNeedsToShow = INVALID_ID;
static int nRoomMessageRoomID = INVALID_ID;
static BOOL sHandleRoomMessage( GAME* pGame,
								UNIT* pUnit,
								const EVENT_DATA& event_data)
{	
	bRoomMessageOkToShow = TRUE;
	ASSERT_RETFALSE( pUnit );
	ASSERT_RETFALSE( UnitGetLevel( pUnit ) );
	if( nRoomMessageNeedsToShow != INVALID_ID )
	{		
		ROOM *pRoom = UnitGetRoom( pUnit );
		if( pRoom &&
			(int)pRoom->idRoom == nRoomMessageRoomID )
		{
			c_PlayerDisplayRoomMessage( nRoomMessageRoomID, nRoomMessageNeedsToShow );
		}				
	}
	nRoomMessageNeedsToShow = INVALID_ID;
	nRoomMessageRoomID = INVALID_ID;
	return TRUE;
}
//----------------------------------------------------------------------------
//Client side only
//----------------------------------------------------------------------------

int c_PlayerGetLastRoomMessage()
{
	return nRoomMessage;
}
//----------------------------------------------------------------------------
//Client side only
//----------------------------------------------------------------------------
void c_PlayerDisplayRoomMessage( int nRoomID,
								 int nMessage )
{
	if( nMessage == INVALID_ID ||
		nMessage == nRoomMessage ) //we already showed this one
		return;

	if( bRoomMessageOkToShow )
	{
		const WCHAR *nActualText = StringTableGetStringByIndex( nMessage );
		if( nActualText == NULL )
			return;	
		UI_TB_ShowRoomEnteredMessage( nActualText );
		nRoomMessage = nMessage;
		bRoomMessageOkToShow = FALSE;
		UnitRegisterEventTimed( UIGetControlUnit(), sHandleRoomMessage, NULL, GAME_TICKS_PER_SECOND * 5 );
	}
	else
	{
		nRoomMessageNeedsToShow = nMessage;
		nRoomMessageRoomID = nRoomID;
	}

	
}
//----------------------------------------------------------------------------
//warping into a new level or levelarea on the client. This 
//gets called right before the load screen goes away
//----------------------------------------------------------------------------
void c_PlayerEnteringNewLevelAndFloor( GAME *pGame,
									  UNIT *pPlayer,
									  int LevelDef,
									  int DRLGDef,
									  int LevelAreaID,
									  int LevelAreaDepth )
{
#if !ISVERSION(SERVER_VERSION)
	nRoomMessage = INVALID_ID;
#endif
	if( LevelAreaDepth == 0 )
	{
		c_RecipeClearLastRecipeCreated(); //tells the client to resend any crafting data 	
	}
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if( pLevel )
	{
		c_LevelFilloutLevelWarps( pLevel );
		cLevelUpdateUnitsVisByThemes( pLevel );
		c_LevelResetDungeonEntranceVisibility( pLevel,
											   pPlayer );
	}
	c_QuestCaculatePointsOfInterest( pPlayer );	//caculates the new points of interest for the player
}

void c_PlayerRemove(
	GAME *pGame,
	UNIT *pUnit)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	

	//destroy those points of interest
	sDestroyPointsOfInterest( pGame, pUnit );
	// get cplayer
	UNITID idPlayer = UnitGetId( pUnit );
	CPLAYER *pCPlayer = sCPlayerFind( idPlayer );
	ASSERTX_RETURN( pCPlayer, "Unable to find cplayer" );
	
	// remove from list
	if (pCPlayer->pNext)
	{
		pCPlayer->pNext->pPrev = pCPlayer->pPrev;
	}
	if (pCPlayer->pPrev)
	{
		pCPlayer->pPrev->pNext = pCPlayer->pNext;
	}
	else
	{
		pGlobals->pCPlayerList = pCPlayer->pNext;
	}

	// free memory
	GFREE( pGame, pCPlayer );
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_PlayerGetMyGuid(
	void)
{
	UNIT *pMyUnit = UIGetControlUnit();
	return pMyUnit ? UnitGetGuid(pMyUnit) : INVALID_GUID;
}

//----------------------------------------------------------------------------
// PARTY FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerRefreshPartyUI(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETURN(pGlobals);

	for( UINT ii = 0; ii < MAX_CHAT_PARTY_MEMBERS; ++ii )
	{
		UNITID id = pGlobals->tPartyMembers[ii].PlayerData.idUnit;
		if( id != INVALID_ID )
		{
			UISendMessage(
				WM_PARTYCHANGE,
				0,
				id );		// 0 - added, 1 - updated, 2 - removed
		}
		else
		{
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetParty(
	CHAT_PARTY_INFO * partyInfo )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	ASSERT_RETURN(partyInfo);
	c_PlayerClearParty();
	pGlobals->tPartyInfo.Set(partyInfo);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerUpdatePartyInfo(
	CHAT_PARTY_INFO * partyInfo )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	ASSERT_RETURN(partyInfo);
	pGlobals->tPartyInfo.Set(partyInfo);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerGetPartyInfo(
	CHAT_PARTY_INFO & partyInfo )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETFALSE(pGlobals);
	partyInfo.Set(&pGlobals->tPartyInfo);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerUpdatePartyLeader(
	PGUID leaderGuid,
	const WCHAR * leaderName )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	ASSERT_RETURN(leaderName);

	pGlobals->tPartyInfo.LeaderGuid = leaderGuid;
	PStrCopy( pGlobals->tPartyInfo.wszLeaderName, leaderName, MAX_CHARACTER_NAME );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetPartyClientData(
	COMMON_PARTY_CLIENT_DATA * data )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	ASSERT_RETURN(data);

	pGlobals->tPartyClientData.Set( data );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct COMMON_PARTY_CLIENT_DATA * c_PlayerGetPartyClientData(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETNULL(pGlobals);

	if(pGlobals->tPartyInfo.IdPartyChannel == INVALID_CHANNELID)
		return NULL;

	return &pGlobals->tPartyClientData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerClearParty(
	void )
{
	BOOL bRet = FALSE;
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	if(!pGlobals)
		return bRet;
	bRet = pGlobals->tPartyInfo.bForPlayerAdvertisementOnly;
	const BOOL bChanged = (pGlobals->tPartyInfo.IdPartyChannel != INVALID_CHANNELID);
	pGlobals->tPartyInfo.Init();
	pGlobals->tPartyInfo.IdPartyChannel = INVALID_CHANNELID;
	pGlobals->tPartyClientData.Init();
	for (int i=0; i<MAX_CHAT_PARTY_MEMBERS; i++)
	{
		TOWN_PORTAL_SPEC * pTownPortal = &pGlobals->tPartyMembers[i].PlayerData.tTownPortal;
		if( pTownPortal->IsValid() )
		{
			c_TownPortalClosed(
				pGlobals->tPartyMembers[i].MemberGuid,
				pGlobals->tPartyMembers[i].wszMemberName,
				pTownPortal );
		}
		pGlobals->tPartyMembers[i].Init();
	}
	if (bChanged)
	{
		UISendMessage(WM_PARTYCHANGE, 2, INVALID_ID);		// 0 - added, 1 - updated, 2 - removed
	}
	return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHANNELID c_PlayerGetPartyId(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETX(pGlobals,INVALID_CHANNELID);
	return pGlobals->tPartyInfo.IdPartyChannel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_PlayerGetPartyLeader(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETX(pGlobals,INVALID_GUID);
	return pGlobals->tPartyInfo.LeaderGuid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerAddPartyInvite(
	PGUID idInviter )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	if(pGlobals->idPartyInviter != INVALID_CLUSTERGUID)
	{
		//	some day there will probably be a list of outstanding invites
		//		that the client can choose from, for now we store only one.
		CHAT_REQUEST_MSG_DECLINE_PARTY_INVITE msg;
		msg.guidInvitingMember = pGlobals->idPartyInviter;
		c_ChatNetSendMessage(
			&msg,
			USER_REQUEST_DECLINE_PARTY_INVITE );
	}
	pGlobals->idPartyInviter = idInviter;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_PlayerGetPartyInviter(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETX(pGlobals,INVALID_CLUSTERGUID);
	return pGlobals->idPartyInviter;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearPartyInvites(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETURN(pGlobals);
	pGlobals->idPartyInviter = INVALID_CLUSTERGUID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerAddBuddyInvite(
	LPCWSTR szInviter )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	/*
	if(pGlobals->szBuddyInviter)
	{
	//	some day there will probably be a list of outstanding invites
	//		that the client can choose from, for now we store only one.
	//CHAT_REQUEST_MSG_DECLINE_BUDDY_INVITE msg;
	//msg.idDecliningPlayerGuid = pGlobals->szBuddyInviter;
	//c_ChatNetSendMessage(
	//	&msg,
	//	USER_REQUEST_DECLINE_BUDDY_INVITE );

	c_BuddyInviteDecline();
	}
	*/
	PStrCopy(pGlobals->szBuddyInviter, szInviter, MAX_CHARACTER_NAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WCHAR * c_PlayerGetBuddyInviter(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETNULL(pGlobals);
	return pGlobals->szBuddyInviter;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearBuddyInvites(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETURN(pGlobals);
	PStrCopy(pGlobals->szBuddyInviter, L"", MAX_CHARACTER_NAME);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerAddPartyMember(
	CHAT_MEMBER_INFO * newMemberInfo )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	ASSERT_RETURN(newMemberInfo);
	if( newMemberInfo->MemberGuid == gApp.m_characterGuid )
	{
		return;	//	don't add ourself to the list
	}

	int nFirstEmpty = -1;
	for (int i=0; i<MAX_CHAT_PARTY_MEMBERS; i++)
	{
		if (pGlobals->tPartyMembers[i].MemberGuid == newMemberInfo->MemberGuid)
		{
			trace("Asked to add duplicate party member by server.");
			return;
		}

		if (pGlobals->tPartyMembers[i].MemberGuid == INVALID_GUID)
		{
			nFirstEmpty = i;
			break;
		}
	}

	ASSERTX_RETURN(nFirstEmpty != -1 && nFirstEmpty < MAX_CHAT_PARTY_MEMBERS, "Party list is full but server is asking to add another member.");

	pGlobals->tPartyMembers[nFirstEmpty].Set(newMemberInfo);

	TOWN_PORTAL_SPEC * tpSpec = &pGlobals->tPartyMembers[nFirstEmpty].PlayerData.tTownPortal;
	if( tpSpec->IsValid() )
	{
		c_TownPortalOpened(
			newMemberInfo->MemberGuid,
			newMemberInfo->wszMemberName,
			tpSpec );
	}

	UISendMessage(WM_PARTYCHANGE, 0, newMemberInfo->PlayerData.idUnit );		// 0 - added, 1 - updated, 2 - removed
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerUpdatePartyMember(
	CHAT_MEMBER_INFO * memberInfo )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);
	ASSERT_RETURN(memberInfo);

	int i=0;
	for (; i<MAX_CHAT_PARTY_MEMBERS; i++)
	{
		if (pGlobals->tPartyMembers[i].MemberGuid == memberInfo->MemberGuid)
			break;
	}

	if(i == MAX_CHAT_PARTY_MEMBERS)
		return;	//	member not found.

	TOWN_PORTAL_SPEC * oldPortal = &pGlobals->tPartyMembers[i].PlayerData.tTownPortal;
	TOWN_PORTAL_SPEC * newPortal = &memberInfo->PlayerData.tTownPortal;
	if( !oldPortal->Equals( newPortal ) )
	{
		if( oldPortal->IsValid() )
		{
			c_TownPortalClosed(
				memberInfo->MemberGuid,
				memberInfo->wszMemberName,
				oldPortal );
		}
		if( newPortal->IsValid() )
		{
			c_TownPortalOpened(
				memberInfo->MemberGuid,
				memberInfo->wszMemberName,
				newPortal );
		}
	}

	pGlobals->tPartyMembers[i].Set( memberInfo );

	UISendMessage(WM_PARTYCHANGE, 1, memberInfo->PlayerData.idUnit);		// 0 - added, 1 - updated, 2 - removed
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerRemovePartyMember(
	PGUID guidPlayer )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	BOOL bDeadmanFound = FALSE;
	for (int i=0; i<MAX_CHAT_PARTY_MEMBERS; i++)
	{
		if (pGlobals->tPartyMembers[i].MemberGuid == guidPlayer)
			bDeadmanFound = TRUE;

		if (bDeadmanFound)
		{
			TOWN_PORTAL_SPEC * tpSpec = &pGlobals->tPartyMembers[i].PlayerData.tTownPortal;
			if( tpSpec->IsValid() )
			{
				c_TownPortalClosed(
					pGlobals->tPartyMembers[i].MemberGuid,
					pGlobals->tPartyMembers[i].wszMemberName,
					tpSpec );
			}
			if ( i == MAX_CHAT_PARTY_MEMBERS - 1 )	// if we're on the last one
			{
				// clear it
				pGlobals->tPartyMembers[i].Init();
			}
			else
			{
				pGlobals->tPartyMembers[i].Set( &pGlobals->tPartyMembers[i+1] );
				pGlobals->tPartyMembers[i+1].Init();
			}
		}
	}

	UISendMessage(WM_PARTYCHANGE, 2, INVALID_ID);		// 0 - added, 1 - updated, 2 - removed

	ASSERTX_RETURN(bDeadmanFound, "Cannot find party member entry remove.");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_PlayerGetPartyMemberCount(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETX(pGlobals, -1);

	if (pGlobals->tPartyInfo.IdPartyChannel == INVALID_CHANNELID)
		return -1;

	int memberCount = 1;	//	count us as well
	for (int i=0; i<MAX_CHAT_PARTY_MEMBERS; i++)
	{
		if (pGlobals->tPartyMembers[i].MemberGuid != INVALID_CLUSTERGUID)
			++memberCount;
	}

	return memberCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_PlayerFindPartyMember(
	PGUID guidMember )
{
	ASSERT_RETX(guidMember != INVALID_GUID, INVALID_INDEX);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETX(pGlobals, INVALID_INDEX);

	for( UINT ii = 0; ii < MAX_CHAT_PARTY_MEMBERS; ++ii )
	{
		if( pGlobals->tPartyMembers[ii].MemberGuid == guidMember )
			return ii;
	}
	return INVALID_INDEX;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_PlayerGetPartyMemberGUID(
	UINT nMember )
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_GUID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_GUID);

	return pGlobals->tPartyMembers[nMember].MemberGuid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID c_PlayerGetPartyMemberGameId(
	UINT nMember )
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_ID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_ID);

	return pGlobals->tPartyMembers[nMember].PlayerData.idGame;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID c_PlayerGetPartyMemberUnitId(
	UINT nMember,
	BOOL bSameGameOnly /* = TRUE */)
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_ID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_ID);

	if( !bSameGameOnly )
	{
		return pGlobals->tPartyMembers[nMember].PlayerData.idUnit;
	}
	else
	{
		GAME * myGame = AppGetCltGame();
		if( myGame &&
			GameGetId(myGame) == pGlobals->tPartyMembers[nMember].PlayerData.idGame )
		{
			return pGlobals->tPartyMembers[nMember].PlayerData.idUnit;
		}
		else
		{
			return INVALID_ID;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_PlayerGetPartyMemberUnitGuid(
	UINT nMember )
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_GUID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_GUID);

	return pGlobals->tPartyMembers[nMember].MemberGuid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID c_PlayerGetPartyMemberLevelDepth(
	UINT nMember,
	BOOL bSameGameOnly /* = TRUE */)
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_ID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_ID);

	if( !bSameGameOnly )
	{
		return pGlobals->tPartyMembers[nMember].PlayerData.nDepth;
	}
	else
	{
		GAME * myGame = AppGetCltGame();
		if( myGame &&
			GameGetId(myGame) == pGlobals->tPartyMembers[nMember].PlayerData.idGame )
		{
			return pGlobals->tPartyMembers[nMember].PlayerData.nDepth;
		}
		else
		{
			return INVALID_ID;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID c_PlayerGetPartyMemberLevelArea(
	UINT nMember,
	BOOL bSameGameOnly /* = TRUE */)
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_ID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_ID);

	if( !bSameGameOnly )
	{
		return pGlobals->tPartyMembers[nMember].PlayerData.nArea;
	}
	else
	{
		GAME * myGame = AppGetCltGame();
		if( myGame &&
			GameGetId(myGame) == pGlobals->tPartyMembers[nMember].PlayerData.idGame )
		{
			return pGlobals->tPartyMembers[nMember].PlayerData.nArea;
		}
		else
		{
			return INVALID_ID;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID c_PlayerGetPartyMemberPVPWorld(
									   UINT nMember,
									   BOOL bSameGameOnly /* = TRUE */)
{
	ASSERT_RETVAL(nMember < MAX_CHAT_PARTY_MEMBERS, INVALID_ID);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETVAL(pGlobals, INVALID_ID);

	if( !bSameGameOnly )
	{
		return TESTBIT( pGlobals->tPartyMembers[nMember].PlayerData.tGameVariant.dwGameVariantFlags, GV_PVPWORLD );
	}
	else
	{
		GAME * myGame = AppGetCltGame();
		if( myGame &&
			GameGetId(myGame) == pGlobals->tPartyMembers[nMember].PlayerData.idGame )
		{
			return TESTBIT( pGlobals->tPartyMembers[nMember].PlayerData.tGameVariant.dwGameVariantFlags, GV_PVPWORLD );
		}
		else
		{
			return FALSE;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * c_PlayerGetPartyMemberName(
	UINT nMember)
{
	ASSERT_RETNULL(nMember < MAX_CHAT_PARTY_MEMBERS);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETNULL(pGlobals);

	return pGlobals->tPartyMembers[nMember].wszMemberName;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD c_PlayerGetPartyMemberClientData(
	UINT nMember)
{
	ASSERT_RETNULL(nMember < MAX_CHAT_PARTY_MEMBERS);

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETNULL(pGlobals);

	return pGlobals->tPartyMembers[nMember].ClientSetPlayerData.XfireId;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
BOOL c_PlayerIsInMyParty(
	PGUID nTargetPlayer)
{
#if !ISVERSION(SERVER_VERSION)

	const int nCount = c_PlayerGetPartyMemberCount();

	for (int i=0; i<nCount ++i)
	{
		if (c_PlayerGetPartyMemberGUID(i) == nTargetPlayer)
		{
			return TRUE;
		}
	}

	//	UNITID * pMe = GameGetControlUnit(AppGetCltGame());
	//	ASSERT_RETFALSE(pMe);
	//	PARTYID pMyPartyID = UnitGetPartyId(pMe);
	//	if (pMyPartyID == INVALID_GUID)
	//	{
	//		return FALSE;
	//	}
	//
	//	for ( GAMECLIENT *pClient = ClientGetFirstInGame( AppGetCltGame() );
	//		 pClient;
	//		 pClient = ClientGetNextInGame( pClient ))
	//	{
	//		UNIT *pUnit = ClientGetControlUnit( pClient );
	//		ASSERTX_RETURN( pUnit != NULL, "Unexpected NULL Control Unit from Client" );
	//		// must be in the same level, and flagged to receive a party kill message
	//		if ( UnitGetPartyId( pUnit ) == pMyPartyID && pUnit != pMe )
	//		{
	//			return TRUE;
	//		}
	//	}
#endif

	return FALSE;
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetGuildInfo(
	CHANNELID guildChannelId,
	const WCHAR * guildName,
	GUILD_RANK eGuildRank )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETURN(pGlobals);

	pGlobals->idGuildChatChannel = guildChannelId;
	PStrCopy(pGlobals->wszGuildName, MAX_CHAT_CNL_NAME, WSTR_ARG(guildName), MAX_CHAT_CNL_NAME);
	pGlobals->eGuildRank = eGuildRank;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearGuildInfo(
	void )
{
	c_PlayerSetGuildInfo(INVALID_CHANNELID, L"", GUILD_RANK_INVALID);
	c_PlayerSetGuildSettingsInfo(L"", L"", L"", L"");
	c_PlayerSetGuildPermissionsInfo(GUILD_RANK_INVALID,GUILD_RANK_INVALID,GUILD_RANK_INVALID,GUILD_RANK_INVALID,GUILD_RANK_INVALID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CHANNELID c_PlayerGetGuildChannelId(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETNULL(pGlobals);

	return pGlobals->idGuildChatChannel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * c_PlayerGetGuildName(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETNULL(pGlobals);

	return pGlobals->wszGuildName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerGetIsGuildLeader(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETFALSE(pGlobals);

	return (pGlobals->eGuildRank == GUILD_RANK_LEADER);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GUILD_RANK c_PlayerGetCurrentGuildRank(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETX(pGlobals, GUILD_RANK_INVALID);

	return pGlobals->eGuildRank;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetGuildSettingsInfo(
	const WCHAR * leaderRankName,
	const WCHAR * officerRankName,
	const WCHAR * memberRankName,
	const WCHAR * recruitRankName )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	PStrCopy(pGlobals->wszGuildLeaderRankName,  WSTR_ARG(leaderRankName),  MAX_CHARACTER_NAME);
	PStrCopy(pGlobals->wszGuildOfficerRankName, WSTR_ARG(officerRankName), MAX_CHARACTER_NAME);
	PStrCopy(pGlobals->wszGuildMemberRankName,  WSTR_ARG(memberRankName),  MAX_CHARACTER_NAME);
	PStrCopy(pGlobals->wszGuildRecruitRankName, WSTR_ARG(recruitRankName), MAX_CHARACTER_NAME);

	UIGuildPanelUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * c_PlayerGetRankStringForCurrentGuild(
	GUILD_RANK eGuildRank )
{
	ASSERT_RETX(eGuildRank >= GUILD_RANK_RECRUIT && eGuildRank <= GUILD_RANK_LEADER, L"");

	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETX(pGlobals, L"");

	switch (eGuildRank)
	{
	case GUILD_RANK_LEADER:
		return pGlobals->wszGuildLeaderRankName[0] ? pGlobals->wszGuildLeaderRankName : GlobalStringGet(GS_GUILD_RANK_LEADER);
	case GUILD_RANK_OFFICER:
		return pGlobals->wszGuildOfficerRankName[0] ? pGlobals->wszGuildOfficerRankName : GlobalStringGet(GS_GUILD_RANK_OFFICER);
	case GUILD_RANK_MEMBER:
		return pGlobals->wszGuildMemberRankName[0] ? pGlobals->wszGuildMemberRankName : GlobalStringGet(GS_GUILD_RANK_MEMBER);
	case GUILD_RANK_RECRUIT:
		return pGlobals->wszGuildRecruitRankName[0] ? pGlobals->wszGuildRecruitRankName : GlobalStringGet(GS_GUILD_RANK_RECRUIT);
	default:
		return L"";
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetGuildPermissionsInfo(
	GUILD_RANK promoteMinRank,
	GUILD_RANK demoteMinRank,
	GUILD_RANK emailMinRank,
	GUILD_RANK inviteMinRank,
	GUILD_RANK bootMinRank )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	pGlobals->eMinPromoteRank = promoteMinRank;
	pGlobals->eMinDemoteRank = demoteMinRank;
	pGlobals->eMinEmailRank = emailMinRank;
	pGlobals->eMinInviteRank = inviteMinRank;
	pGlobals->eMinBootRank = bootMinRank;

	UIGuildPanelUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerGetGuildPermissionsInfo(
	GUILD_RANK &promoteMinRank,
	GUILD_RANK &demoteMinRank,
	GUILD_RANK &emailMinRank,
	GUILD_RANK &inviteMinRank,
	GUILD_RANK &bootMinRank )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETFALSE(pGlobals);

	promoteMinRank = pGlobals->eMinPromoteRank;
	demoteMinRank = pGlobals->eMinDemoteRank;
	emailMinRank = pGlobals->eMinEmailRank;
	inviteMinRank = pGlobals->eMinInviteRank;
	bootMinRank = pGlobals->eMinBootRank;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerCanDoGuildAction(
	GUILD_ACTION action,
	GUILD_RANK targetPlayerRank /*= GUILD_RANK_INVALID*/)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETFALSE(pGlobals);

	GUILD_RANK playerRank = c_PlayerGetCurrentGuildRank();
	if (playerRank == GUILD_RANK_INVALID)
		return FALSE;

	UNIT * pPlayer = GameGetControlUnit(AppGetCltGame());
	ASSERT_RETFALSE(pPlayer);

	if (!PlayerIsSubscriber(pPlayer))
	{
		if (action != GUILD_ACTION_PROMOTE)
			return FALSE;
		
		if (action == GUILD_ACTION_PROMOTE && playerRank != GS_GUILD_RANK_LEADER)
			return FALSE;
	}

	switch (action)
	{
	case GUILD_ACTION_PROMOTE:
		return (playerRank >= pGlobals->eMinPromoteRank && (targetPlayerRank == GUILD_RANK_INVALID || targetPlayerRank < playerRank) && targetPlayerRank < GUILD_RANK_LEADER);

	case GUILD_ACTION_DEMOTE:
		return (playerRank >= pGlobals->eMinDemoteRank && (targetPlayerRank == GUILD_RANK_INVALID || targetPlayerRank < playerRank) && targetPlayerRank > GUILD_RANK_RECRUIT);

	case GUILD_ACTION_EMAIL_GUILD:
		return (playerRank >= pGlobals->eMinEmailRank);

	case GUILD_ACTION_INVITE:
		return (playerRank >= pGlobals->eMinInviteRank);

	case GUILD_ACTION_BOOT:
		return (playerRank >= pGlobals->eMinBootRank && (targetPlayerRank == GUILD_RANK_INVALID || targetPlayerRank < playerRank));

	default:
		ASSERT_MSG("Unknown guild action.");
		break;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetGuildLeaderInfo(
	PGUID idLeaderCharacterId,
	const WCHAR * wszLeaderName )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	pGlobals->idGuildLeaderCharacterGuid = idLeaderCharacterId;
	PStrCopy(pGlobals->wszGuildLeaderName, wszLeaderName, MAX_CHARACTER_NAME);

	UIGuildPanelUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID c_PlayerGetGuildLeaderCharacterId(
	void )
{
	CLIENT_PLAYER_GLOBALS * pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETX(pGlobals, INVALID_GUID);

	return pGlobals->idGuildLeaderCharacterGuid;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * c_PlayerGetGuildMemberNameByID(
	PGUID guid )
{
	const PLAYER_GUILD_MEMBER_DATA *pGuildMember = c_PlayerGuildMemberListGetNextMember(NULL, FALSE);
	while (pGuildMember)
	{
		if (pGuildMember->idMemberCharacterId == guid)
		{
			return pGuildMember->wszMemberName;
		}
		pGuildMember = c_PlayerGuildMemberListGetNextMember(pGuildMember, FALSE);
	}
	return NULL;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GUILD_RANK c_PlayerGetGuildMemberRankByID(
	PGUID guid )
{
	const PLAYER_GUILD_MEMBER_DATA *pGuildMember = c_PlayerGuildMemberListGetNextMember(NULL, FALSE);
	while (pGuildMember)
	{
		if (pGuildMember->idMemberCharacterId == guid)
		{
			return pGuildMember->eGuildRank;
		}
		pGuildMember = c_PlayerGuildMemberListGetNextMember(pGuildMember, FALSE);
	}
	return GUILD_RANK_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerIsGuildMemberOnline(
	PGUID guid )
{
	const PLAYER_GUILD_MEMBER_DATA *pGuildMember = c_PlayerGuildMemberListGetNextMember(NULL, TRUE);
	while (pGuildMember)
	{
		if (pGuildMember->idMemberCharacterId == guid)
		{
			return pGuildMember->bIsOnline;
		}
		pGuildMember = c_PlayerGuildMemberListGetNextMember(pGuildMember, TRUE);
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * c_PlayerGetGuildLeaderName(
	void )
{
	CLIENT_PLAYER_GLOBALS * pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETNULL(pGlobals);

	return pGlobals->wszGuildLeaderName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearGuildLeaderInfo(
	void )
{
	c_PlayerSetGuildLeaderInfo(INVALID_GUID, L"");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sGuildListInitiated(FALSE);
void c_PlayerInitGuildMemberList(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	if (sGuildListInitiated)
		pGlobals->tGuildListHash.Destroy(NULL);

	pGlobals->tGuildListHash.Init();
	pGlobals->dwGuildListHashCount = 0;
	sGuildListInitiated = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearGuildMemberList(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	pGlobals->dwGuildListHashCount = 0;
	pGlobals->tGuildListHash.Destroy(NULL);
	pGlobals->tGuildListHash.Init();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerGuildMemberListAddOrUpdateMember(
	PGUID idMemberCharacterId,
	const WCHAR * wszMemberName,
	BOOL bIsOnline,
	GUILD_RANK rank)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	WCHAR mbrNameCI[MAX_CHARACTER_NAME];
	PStrCopy(mbrNameCI, wszMemberName, MAX_CHARACTER_NAME);
	PStrUpper(mbrNameCI, MAX_CHARACTER_NAME);
	GUILD_LIST_ENTRY * entry = pGlobals->tGuildListHash.Get(mbrNameCI);
	if (!entry)
	{
		entry = pGlobals->tGuildListHash.Add(NULL, mbrNameCI);
		ASSERT_RETURN(entry);
		entry->tMemberClientData.Init();
		++pGlobals->dwGuildListHashCount;
	}

	entry->idMemberCharacterId = idMemberCharacterId;
	PStrCopy(entry->wszMemberName, wszMemberName, MAX_CHARACTER_NAME);
	entry->bIsOnline = bIsOnline;
	entry->eGuildRank = rank;

	UIGuildPanelUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerGuildMemberListAddOrUpdateMemberEx(
	PGUID idMemberCharacterId,
	const WCHAR * wszMemberName,
	BOOL bIsOnline,
	GUILD_RANK rank,
	COMMON_CLIENT_DATA & tMemberData )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	WCHAR mbrNameCI[MAX_CHARACTER_NAME];
	PStrCopy(mbrNameCI, wszMemberName, MAX_CHARACTER_NAME);
	PStrUpper(mbrNameCI, MAX_CHARACTER_NAME);
	GUILD_LIST_ENTRY * entry = pGlobals->tGuildListHash.Get(mbrNameCI);
	if (!entry)
	{
		entry = pGlobals->tGuildListHash.Add(NULL, mbrNameCI);
		ASSERT_RETURN(entry);
		entry->tMemberClientData.Init();
		++pGlobals->dwGuildListHashCount;
	}

	entry->idMemberCharacterId = idMemberCharacterId;
	PStrCopy(entry->wszMemberName, wszMemberName, MAX_CHARACTER_NAME);
	entry->bIsOnline = bIsOnline;
	entry->eGuildRank = rank;
	entry->tMemberClientData.Set( &tMemberData );

	UIGuildPanelUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerGuildMemberListRemoveMember(
	PGUID idMemberCharacterId,
	const WCHAR * wszMemberName )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	WCHAR mbrNameCI[MAX_CHARACTER_NAME];
	PStrCopy(mbrNameCI, wszMemberName, MAX_CHARACTER_NAME);
	PStrUpper(mbrNameCI, MAX_CHARACTER_NAME);
	if (pGlobals->tGuildListHash.Remove(mbrNameCI))
	{
		--pGlobals->dwGuildListHashCount;
	}

	UIGuildPanelUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD c_PlayerGuildMemberListGetMemberCount(
	void )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETZERO(pGlobals);

	return pGlobals->dwGuildListHashCount;
}

//----------------------------------------------------------------------------
//	pass NULL to get 1st member in list
//----------------------------------------------------------------------------
const PLAYER_GUILD_MEMBER_DATA * c_PlayerGuildMemberListGetNextMember(
	const PLAYER_GUILD_MEMBER_DATA * previous,
	BOOL bHideOfflinePlayers /*= FALSE*/ )
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETNULL(pGlobals);

	GUILD_LIST_ENTRY * entry = (GUILD_LIST_ENTRY*)previous;

	do 
	{
		if (entry)
			entry = pGlobals->tGuildListHash.GetNext(entry);
		else
			entry = pGlobals->tGuildListHash.GetFirst();
	} while(entry && bHideOfflinePlayers && !entry->bIsOnline);

	return entry;
}

#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerBroadcastGuildAssociation(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( UnitCanSendMessages( unit ), "Can't send unit who isn't ready to send messages yet" );

	if (!UnitIsA(unit, UNITTYPE_PLAYER))
		return;

	MSG_SCMD_UNIT_GUILD_ASSOCIATION msg;
	msg.id = UnitGetId( unit );
	s_ClientGetGuildAssociation( client, msg.wszGuildName, MAX_GUILD_NAME, msg.eGuildRank, msg.wszRankName, MAX_CHARACTER_NAME );

	ROOM * room = UnitGetRoom( unit );
	s_SendMessageToAllWithRoom( game, room, SCMD_UNIT_GUILD_ASSOCIATION, &msg );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_GuildInvite(
	LPCWSTR szName)
{
#if !ISVERSION(SERVER_VERSION)
	MSG_CCMD_GUILD_INVITE msg;
	PStrCopy(msg.wszPlayerToInviteName, szName, MAX_CHARACTER_NAME);
	return c_SendMessage(CCMD_GUILD_INVITE, &msg);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_GuildInviteAccept(
	PGUID guidInviter,
	void *)
{
#if !ISVERSION(SERVER_VERSION)
	REF(guidInviter);

	MSG_CCMD_GUILD_ACCEPT_INVITE msg;
	c_SendMessage(CCMD_GUILD_ACCEPT_INVITE, &msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_GuildInviteDecline(
	PGUID guidInviter,
	void *)
{
#if !ISVERSION(SERVER_VERSION)
	REF(guidInviter);

	MSG_CCMD_GUILD_DECLINE_INVITE msg;
	c_SendMessage(CCMD_GUILD_DECLINE_INVITE, &msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_GuildInviteIgnore(
	PGUID guidInviter,
	void *pData)
{
#if !ISVERSION(SERVER_VERSION)
	// TODO: add the inviter to the ignore list, once you have a valid guid for them

	c_GuildInviteDecline(guidInviter, pData);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PGUID sGuildInviteGuid = INVALID_GUID;

static void sGuildInviteCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
	ASSERT_RETURN(sGuildInviteGuid != INVALID_GUID);
	WCHAR *szName = c_PlayerGetNameByGuid(sGuildInviteGuid);
	ASSERT_RETURN(szName != NULL);
	c_GuildInvite( szName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_GuildLeave(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	MSG_CCMD_GUILD_LEAVE msg;
	return c_SendMessage(CCMD_GUILD_LEAVE, &msg);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_GuildKick(
	PGUID guid)
{
#if !ISVERSION(SERVER_VERSION)
	MSG_CCMD_GUILD_REMOVE msg;
	PStrCopy(msg.wszPlayerToRemoveName, c_PlayerGetGuildMemberNameByID(guid), arrsize(msg.wszPlayerToRemoveName));
	return c_SendMessage(CCMD_GUILD_REMOVE, &msg);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PGUID sGuildKickGuid = INVALID_GUID;

static void sGuildKickCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
	ASSERT_RETURN(sGuildKickGuid != INVALID_GUID);
	c_GuildKick( sGuildKickGuid );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_GuildChangeMemberRank(
	PGUID guid,
	GUILD_RANK newRank)
{
#if !ISVERSION(SERVER_VERSION)
	MSG_CCMD_GUILD_CHANGE_RANK msg;
	PStrCopy(msg.wszTargetName, c_PlayerGetGuildMemberNameByID(guid), arrsize(msg.wszTargetName));
	msg.eTargetRank = (BYTE)newRank;
	return c_SendMessage(CCMD_GUILD_CHANGE_RANK, &msg);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_GuildChangeRankName(
	GUILD_RANK rank,
	const WCHAR * rankName )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(rankName);
	ASSERT_RETURN(rank >= GUILD_RANK_RECRUIT && rank <= GUILD_RANK_LEADER);

	MSG_CCMD_GUILD_CHANGE_RANK_NAME msg;
	msg.eRank = (BYTE)rank;
	PStrCopy(msg.wszRankName, rankName, MAX_CHARACTER_NAME);

	c_SendMessage(CCMD_GUILD_CHANGE_RANK_NAME, &msg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_GuildChangeActionPermissions(
	GUILD_ACTION guildAction,
	GUILD_RANK minimumRequiredRank )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(guildAction >= GUILD_ACTION_PROMOTE && guildAction <= GUILD_ACTION_BOOT);
	ASSERT_RETURN(minimumRequiredRank >= GUILD_RANK_MEMBER && minimumRequiredRank <= GUILD_RANK_LEADER);

	MSG_CCMD_GUILD_CHANGE_ACTION_PERMISSIONS permMsg;
	permMsg.eGuildActionType = (BYTE)guildAction;
	permMsg.eMinimumGuildRank = (BYTE)minimumRequiredRank;

	c_SendMessage(CCMD_GUILD_CHANGE_ACTION_PERMISSIONS, &permMsg);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void c_GuildPrintSettings(void)
{
#if !ISVERSION(SERVER_VERSION)
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();	
	ASSERT_RETURN(pGlobals);

	UIAddChatLineArgs(
		CHAT_TYPE_SERVER,
		ChatGetTypeNameColor(CHAT_TYPE_SERVER),
		L"Guild Name: %s, "
		L"Leader Rank Name:  %s, "
		L"Officer Rank Name: %s, "
		L"Member Rank Name:  %s, "
		L"Recruit Rank Name: %s, "
		L"Permission Levels: %u, %u, %u, %u, %u",
		pGlobals->wszGuildName,
		c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER),
		c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER),
		c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER),
		c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT),
		pGlobals->eMinPromoteRank,
		pGlobals->eMinDemoteRank,
		pGlobals->eMinEmailRank,
		pGlobals->eMinInviteRank,
		pGlobals->eMinBootRank);
#endif	//	!ISVERSION(SERVER_VERSION)
}
#endif	//	ISVERSION(DEVELOPMENT)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GUILD_RANK c_PlayerGetGuildRankFromRankString(
	const WCHAR * rankString )
{
#if !ISVERSION(SERVER_VERSION)
	if (!rankString || !rankString[0])
		return GUILD_RANK_INVALID;

	if (PStrICmp(rankString, c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_LEADER)) == 0)
	{
		return GUILD_RANK_LEADER;
	}
	else if (PStrICmp(rankString, c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_OFFICER)) == 0)
	{
		return GUILD_RANK_OFFICER;
	}
	else if (PStrICmp(rankString, c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_MEMBER)) == 0)
	{
		return GUILD_RANK_MEMBER;
	}
	else if (PStrICmp(rankString, c_PlayerGetRankStringForCurrentGuild(GUILD_RANK_RECRUIT)) == 0)
	{
		return GUILD_RANK_RECRUIT;
	}
#endif
	return GUILD_RANK_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PGUID sGuildPromoteGuid = INVALID_GUID;
static PGUID sGuildDemoteGuid = INVALID_GUID;
static GUILD_RANK sGuildPromoteRank = GUILD_RANK_INVALID;
static GUILD_RANK sGuildDemoteRank = GUILD_RANK_INVALID;

static void sGuildPromoteCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
	ASSERT_RETURN(sGuildPromoteGuid != INVALID_GUID && sGuildPromoteRank != GUILD_RANK_INVALID);
	c_GuildChangeMemberRank( sGuildPromoteGuid, sGuildPromoteRank );
}

static void sGuildDemoteCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
	ASSERT_RETURN(sGuildDemoteGuid != INVALID_GUID && sGuildDemoteRank != GUILD_RANK_INVALID);
	c_GuildChangeMemberRank( sGuildPromoteGuid, sGuildDemoteRank );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelClassLevelsPostProcessSetMaxLevel(
	int nUnitType,
	int nLevel)
{
	unsigned int nPlayerClassMax = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYERS);

	if ( AppIsDemo() )
		nLevel = 5;

	for (unsigned int jj = 0; jj < nPlayerClassMax; jj++)
	{
		UNIT_DATA * player_data = (UNIT_DATA *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_PLAYERS, jj);
		if (player_data == NULL)
		{
			continue;
		}

		if (player_data->nMaxLevel > nLevel && UnitIsA( player_data->nUnitType, nUnitType) )
		{
			player_data->nMaxLevel = nLevel;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelClassLevelsPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	// validate that levels appear in sequence
	PLAYERLEVEL_DATA * prev = (PLAYERLEVEL_DATA *)ExcelGetDataPrivate(table, 0);
	ASSERT_RETFALSE(prev);
	ASSERT_RETFALSE(prev->nLevel == 1);

	for (unsigned int ii = 1; ii < ExcelGetCountPrivate(table); ++ii)
	{
		PLAYERLEVEL_DATA * curr = (PLAYERLEVEL_DATA *)ExcelGetDataPrivate(table, ii);
		
		if (curr->nUnitType == prev->nUnitType)
		{
			ASSERT(curr->nLevel == prev->nLevel + 1);
		}
		else
		{
			sExcelClassLevelsPostProcessSetMaxLevel(prev->nUnitType, prev->nLevel);
		}
		prev = curr;
	}
	
	sExcelClassLevelsPostProcessSetMaxLevel(prev->nUnitType, prev->nLevel);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelClassLevelsPostProcessSetMaxRank(
	int nUnitType,
	int nRank)
{
	unsigned int nPlayerClassMax = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYERS);

	if ( AppIsDemo() )
		nRank = 5;

	for (unsigned int jj = 0; jj < nPlayerClassMax; jj++)
	{
		UNIT_DATA * player_data = (UNIT_DATA *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_PLAYERS, jj);
		if (player_data == NULL)
		{
			continue;
		}

		if (player_data->nMaxRank > nRank && UnitIsA( player_data->nUnitType, nUnitType) )
		{
			player_data->nMaxRank = nRank;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelClassRanksPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	// validate that levels appear in sequence
	PLAYERRANK_DATA * prev = (PLAYERRANK_DATA *)ExcelGetDataPrivate(table, 0);
	ASSERT_RETFALSE(prev);
	ASSERT_RETFALSE(prev->nLevel == 1);

	for (unsigned int ii = 1; ii < ExcelGetCountPrivate(table); ++ii)
	{
		PLAYERRANK_DATA * curr = (PLAYERRANK_DATA *)ExcelGetDataPrivate(table, ii);
		
		if (curr->nUnitType == prev->nUnitType)
		{
			ASSERT(curr->nLevel == prev->nLevel + 1);
		}
		else
		{
			sExcelClassLevelsPostProcessSetMaxRank(prev->nUnitType, prev->nLevel);
		}
		prev = curr;
	}
	
	sExcelClassLevelsPostProcessSetMaxRank(prev->nUnitType, prev->nLevel);

	return TRUE;
}

//----------------------------------------------------------------------------
// Note: this function doesn't currently return a valid answer for server.
// We'd probably need a map<string, ...> of names to do it.
//----------------------------------------------------------------------------
BOOL PlayerNameExists(
	const WCHAR * szPlayerNameIn )
{
	if (AppGetType() == APP_TYPE_CLOSED_CLIENT || 
		AppGetType() == APP_TYPE_CLOSED_SERVER)
		return FALSE;

	OS_PATH_CHAR szPlayerName[MAX_PATH];
	PStrCvt(szPlayerName, szPlayerNameIn, _countof(szPlayerName));

	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, _countof(filename), OS_PATH_TEXT("%s%s.hg1"), FilePath_GetSystemPath(FILE_PATH_SAVE), szPlayerName);

	return FileExists( filename );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * PlayerLoad(
	GAME * game,
	CLIENT_SAVE_FILE *pClientSaveFile,
	DWORD dwPlayerLoadFlags,
	GAMECLIENTID idClient)
{
	if ( AppCommonGetDemoMode() )
	{
		// In the benchmark mode, we simply create a new player and exit.
		return DemoLevelInitializePlayer();
	}

	ASSERTX_RETNULL( pClientSaveFile, "Expected client save file struct" );
	BOOL bForCharacterSelection = TESTBIT( dwPlayerLoadFlags, PLF_FOR_CHARACTER_SELECTION );
	
	OS_PATH_CHAR szPlayerName[MAX_PATH];
	PStrCvt(szPlayerName, pClientSaveFile->uszCharName, _countof(szPlayerName));

	BYTE * pRichSaveHeader = NULL;
	BOOL bLoadedFile = FALSE;
	if (!pClientSaveFile->pBuffer)
	{
		OS_PATH_CHAR filename[MAX_PATH];
		PStrPrintf(filename, _countof(filename), OS_PATH_TEXT("%s%s.hg1"), FilePath_GetSystemPath(FILE_PATH_SAVE), szPlayerName);

		pRichSaveHeader = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, filename, &pClientSaveFile->dwBufferSize));
		pClientSaveFile->pBuffer = RichSaveFileRemoveHeader(pRichSaveHeader, &pClientSaveFile->dwBufferSize);
		bLoadedFile = TRUE;
	}
	
	// setup autofree of the rich save header
	AUTOFREE autofree(NULL, pRichSaveHeader);			
	
	// don't proceed if we have no data
	if (!pClientSaveFile->pBuffer)
	{
		return NULL;
	}

	UNIT * unit = NULL;
	ONCE
	{	
	
		// reset id aliasing table for this player
		UnitIdMapReset(game->m_pUnitIdMap, FALSE);

		// load player (do *not* give them a ROOM, we will handle that later)
		BIT_BUF tBBClientSaveData(pClientSaveFile->pBuffer, pClientSaveFile->dwBufferSize);		
		XFER<XFER_LOAD> tXfer( &tBBClientSaveData );
		UNIT_FILE_XFER_SPEC<XFER_LOAD> tReadSpec( tXfer );
		tReadSpec.pRoom = NULL;		// NO ROOM, we will warp them to a room soon! -Colin
		SETBIT( tReadSpec.dwReadUnitFlags, RUF_IGNORE_INVENTORY_BIT, bForCharacterSelection );
		SETBIT( tReadSpec.dwReadUnitFlags, RUF_IGNORE_HOTKEYS_BIT );  // we can't read hotkeys until all units are loaded
		tReadSpec.idClient = idClient;
		if (pClientSaveFile->eFormat == PSF_DATABASE)
		{
#if !ISVERSION(CLIENT_ONLY_VERSION)
			DATABASE_UNIT_COLLECTION *pDBUnitCollection = (DATABASE_UNIT_COLLECTION *)pClientSaveFile->pBuffer;
			DATABASE_UNIT_COLLECTION_RESULTS tResults;
			s_DBUnitCollectionLoad( game, NULL, pDBUnitCollection, tReadSpec, tResults );
			
			// we should have loaded exactly one unit
			if (tResults.nNumUnits == 1)
			{
				unit = tResults.pUnits[ 0 ];
			}
			
			if (unit == NULL)
			{
				const int MAX_MESSAGE = 256;
				char szMessage[ MAX_MESSAGE ];
				PStrPrintf( 
					szMessage, 
					MAX_MESSAGE, 
					"Unable to load player from database unit collection" );
				ASSERTX( 0, szMessage );										
			}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
		}
		else
		{
			unit = UnitFileXfer(game, tReadSpec);		
			if (unit == NULL)
			{
				const int MAX_MESSAGE = 256;
				char szMessage[ MAX_MESSAGE ];
				PStrPrintf( 
					szMessage, 
					MAX_MESSAGE, 
					"Error with save/load, please attach character files to bug.  Unable to xfer unit for player load");
				ASSERTX( 0, szMessage );										
			}
		}
		
		if (!unit)
		{
			break;
		}
								
		if ( ! bForCharacterSelection )
		{
			GAMECLIENT *pClient = ClientGetById( game, idClient );
			ASSERTX_RETNULL( pClient, "Expected client" );

			// check for loading hardcore characters when they aren't allowed for this client
			if (PlayerIsHardcore( unit ) == TRUE &&
				(ClientHasBadge( pClient, ACCT_ACCOMPLISHMENT_CAN_PLAY_HARDCORE_MODE) == FALSE || !PlayerIsSubscriber(unit) ))
			{
				UNITLOG_WARNX( 0, "Account trying to load hardcore character, but doesn't have permission", unit );
				UnitFree( unit );
				unit = NULL;
				break;
			}

			if (PlayerIsElite( unit ) == TRUE &&
				(ClientHasBadge( pClient, ACCT_ACCOMPLISHMENT_CAN_PLAY_ELITE_MODE) == FALSE) && !AppIsSinglePlayer())
			{
				UNITLOG_WARNX( 0, "Account trying to load elite character, but doesn't have permission", unit );
				UnitFree( unit );
				unit = NULL;
				break;
			}
	
			// now load hotkeys only
			ASSERTX( tReadSpec.tXfer.GetBuffer() == &tBBClientSaveData, "Xfer buffer has changed!" );
			tReadSpec.tXfer.SetCursor( 0 );  // go back to beginning
			tReadSpec.eOnlyBookmark = UB_HOTKEYS;
			tReadSpec.pUnitExisting = unit;  // do not create a new unit, just load onto this existing one we already have now
			CLEARBIT( tReadSpec.dwReadUnitFlags, RUF_IGNORE_HOTKEYS_BIT );
			if (pClientSaveFile->eFormat == PSF_DATABASE)
			{
#if !ISVERSION(CLIENT_ONLY_VERSION)
				DATABASE_UNIT_COLLECTION *pDBUnitCollection = (DATABASE_UNIT_COLLECTION *)pClientSaveFile->pBuffer;
				s_DatabaseUnitsLoadOntoExistingRootOnly( unit, pDBUnitCollection, tReadSpec );	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
			}
			else
			{
				UnitFileXfer( game, tReadSpec );
			}
			
			if ( IS_SERVER( unit ) )
			{
				s_WeaponConfigInitialize( game, unit );
				s_WeaponconfigUpdateSkills( unit, TAG_SELECTOR_WEAPCONFIG_HOTKEY1 + UnitGetStat( unit, STATS_CURRENT_WEAPONCONFIG ) );
			}
		}

		UnitSetName(unit, pClientSaveFile->uszCharName);
		UnitClearFlag(unit, UNITFLAG_JUSTCREATED);

		if (IS_SERVER(game))
		{
			s_WardrobeUpdateClient(game, unit, NULL);
		} 
		else if ( bForCharacterSelection )
		{
			c_WardrobeMakeClientOnly( unit);
		}

	}
	
	if (bLoadedFile)
	{
		pClientSaveFile->pBuffer = NULL;
		pClientSaveFile->dwBufferSize = 0;
	}

	// bring this player up to current balanced game data
			
	return unit;
	
}

//----------------------------------------------------------------------------
// This function is capable of loading the following types of collections
// - Item tree: A single item tree, like a weapon with many mods
// - Item collection: An arbitrary collection of units (such as a shared stash slot)
//----------------------------------------------------------------------------
BOOL PlayerLoadUnitCollection(
	UNIT *pPlayer,
	CLIENT_SAVE_FILE *pClientSaveFile)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player unit" );
	ASSERTX_RETFALSE( pClientSaveFile, "Expected client save file" );
	ASSERTX_RETFALSE( pClientSaveFile->pBuffer, "Expected client save file data" );
	ASSERTX_RETFALSE( pClientSaveFile->dwBufferSize > 0, "Invaid client save file data size" );
	ASSERTX_RETFALSE( pClientSaveFile->eFormat == PSF_DATABASE, "Expected database format for client save file" );
	ASSERTX_RETFALSE( pClientSaveFile->ePlayerLoadType != PLT_INVALID, "Expected player load type in client save file" );

#if !ISVERSION(CLIENT_ONLY_VERSION)

	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETFALSE( pGame, "Expected game" );

	// for item trees we must have a root unit
	if (pClientSaveFile->ePlayerLoadType == PLT_ITEM_TREE ||
		pClientSaveFile->ePlayerLoadType == PLT_AH_ITEMSALE_SEND_TO_AH)
	{
	
		// This type is used to load a mini client save file with 1 unit hierarchy
		// into an existing player inventory.  
		// We use this currently in response to loading the items that
		// are attached to an email from the database and into a game where the
		// recipient is playing	
		ASSERTX_RETFALSE( pClientSaveFile->guidUnitRoot != INVALID_GUID, "Expected GUID for unit root" );
		
	}

	// do we have an override unit that will server as the collection root
	UNIT *pCollectionRoot = NULL;
	if (pClientSaveFile->ePlayerLoadType == PLT_ACCOUNT_UNITS)
	{
		
		// instead of getting the root item from the collection, we will use the player
		// as the root of the collection.  This is necessary for loading an item
		// collection because we will have an arbitrary number of items in that
		// collection that should be loaded into the players inventory
		pCollectionRoot = pPlayer;
		
	}

	// load the units
	DATABASE_UNIT_COLLECTION_RESULTS tResults;
	if (!s_DBUnitsLoadFromClientSaveFile(
			pGame,
			pPlayer,
			pCollectionRoot,
			pClientSaveFile,
			&tResults))
	{
		return FALSE;
	}
	
	// when loading an item tree, our results should have exactly one unit
	if (pClientSaveFile->ePlayerLoadType == PLT_ITEM_TREE ||
		pClientSaveFile->ePlayerLoadType == PLT_AH_ITEMSALE_SEND_TO_AH)
	{
		UNIT *pNewUnit = NULL;
		if( tResults.nNumUnits == 1)
		{
			pNewUnit = tResults.pUnits[ 0 ];
		}

		// validate			
		if (pNewUnit == NULL)
		{
			const int MAX_MESSAGE = 256;
			char szMessage[ MAX_MESSAGE ];
			WCHAR uszCharacterName[ MAX_CHARACTER_NAME ];
			UnitGetName( pPlayer, uszCharacterName, MAX_CHARACTER_NAME, 0 );
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"Unable to load unit tree (GUID=%I64d) from unit collection onto player '%S' (GUID=%I64d)",
				pClientSaveFile->guidUnitRoot,
				uszCharacterName,
				UnitGetGuid( pPlayer ));
			ASSERTX_RETFALSE( 0, szMessage );										
		}
		
	}
	
	// process the results of the the load
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	for (int i = 0; i < tResults.nNumUnits; ++i)
	{
		UNIT *pNewUnit = tResults.pUnits[ i ];
		ASSERTX_CONTINUE( pNewUnit, "Expected new unit" );

		// if the player is ready for database operations, this new unit is
		// now also ready for them
		UNIT *pUltimateOwner = UnitGetUltimateOwner( pPlayer );
		if (s_DBUnitCanDoDatabaseOperation( pUltimateOwner, pPlayer ))
		{
			s_DatabaseUnitEnable( pNewUnit, TRUE );
		}
			
		// if the player is already setup and receiving messages, we need to send
		// this new unit to them
		if (UnitCanSendMessages( pPlayer ))
		{
			UnitSetCanSendMessages( pNewUnit, TRUE );
			s_SendUnitToClient( pNewUnit, pClient );
		}
				
	}
	
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
	
	return TRUE;
	
}

/*
//----------------------------------------------------------------------------
// drb - tutorial skip code
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sSkipTutorial( GAME * game, UNIT * unit )
{
	// set tutorial to complete
	UnitSetStat( unit, STATS_TUTORIAL, UnitGetStat( unit, STATS_DIFFICULTY_CURRENT ), 1 );

	if(AppIsHellgate())
	{
		// Start with quest inventory

		// foot-locker
		int nTreasureClass = GlobalIndexGet( GI_TREASURE_TUTORIAL_CRATE );
		ASSERT( nTreasureClass != INVALID_LINK );

		ITEM_SPEC tSpec;
		SETBIT( tSpec.dwFlags, ISF_STARTING_BIT );
		SETBIT( tSpec.dwFlags, ISF_PICKUP_BIT);
		tSpec.unitOperator = unit;
		s_TreasureSpawnClass( unit, nTreasureClass, &tSpec, NULL );

		// 1st drop
		nTreasureClass = GlobalIndexGet( GI_TREASURE_TUTORIAL_FIRSTDROP );
		ASSERT( nTreasureClass != INVALID_LINK );
		ItemSpawnSpecInit( tSpec );	
		SETBIT( tSpec.dwFlags, ISF_STARTING_BIT );
		SETBIT( tSpec.dwFlags, ISF_PICKUP_BIT );
		tSpec.unitOperator = unit;
		s_TreasureSpawnClass( unit, nTreasureClass, &tSpec, NULL );

		// reward
		nTreasureClass = GlobalIndexGet( GI_TREASURE_TUTORIAL_REWARD );
		ASSERT( nTreasureClass != INVALID_LINK );
		ItemSpawnSpecInit( tSpec );	
		SETBIT( tSpec.dwFlags, ISF_STARTING_BIT );
		SETBIT( tSpec.dwFlags, ISF_PICKUP_BIT );
		tSpec.unitOperator = unit;
		s_TreasureSpawnClass( unit, nTreasureClass, &tSpec, NULL );
	}
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void s_sPlayerSetup( 
	UNIT *pUnit)
{
	GAME *pGame = UnitGetGame( pUnit );
	
	// initialize the tutorial to "not complete"
	UnitSetStat( pUnit, STATS_TUTORIAL, UnitGetStat( pUnit, STATS_DIFFICULTY_CURRENT ), 0 );
	
	// set the starting level
	int nLevelDefStart = INVALID_LINK;
	if(AppIsHellgate())
	{
		nLevelDefStart = LevelGetStartNewGameLevelDefinition();
		UnitSetStat( pUnit, STATS_LEVEL_DEF_START, UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT), nLevelDefStart );
		UnitSetStat( pUnit, STATS_LEVEL_DEF_RETURN, UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT), nLevelDefStart );
	}
	else
	{
		int nStartLevelArea( INVALID_ID );
		int nStartLevelDepth( 0 );
		VECTOR vPosition;
		PlayerGetRespawnLocation( pUnit, KRESPAWN_TYPE_PRIMARY, nStartLevelArea, nStartLevelDepth, vPosition );
		nStartLevelArea = (nStartLevelArea != INVALID_ID )?nStartLevelArea:pUnit->pUnitData->nStartingLevelArea;
		//UnitGetStartLevelAreaDefinition( pUnit, nStartLevelArea, nStartLevelDepth );
		nLevelDefStart = LevelDefinitionGetRandom( NULL, nStartLevelArea, nStartLevelDepth );
		UnitSetStat( pUnit, STATS_LEVEL_DEF_START, UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT), 0 );
		UnitSetStat( pUnit, STATS_LEVEL_DEF_RETURN, UnitGetStat(pUnit, STATS_DIFFICULTY_CURRENT),0 );
	}
			
	// skip the tutorial if we're supposed to
	//if (gbSkipTutorial == TRUE)
	//{
	//	sSkipTutorial( pGame, pUnit );
	//}

	// if this level def has an automatic player level associated with it, boost them
	// CHB 2006.12.22 - Is this a development-only feature?
	const LEVEL_DEFINITION *pStartLevelDef = LevelDefinitionGet( nLevelDefStart );

	if (pStartLevelDef->nBonusStartingTreasure != INVALID_ID)
	{
		ITEM_SPEC spec;
		SETBIT(spec.dwFlags, ISF_STARTING_BIT);
		SETBIT(spec.dwFlags, ISF_PICKUP_BIT );
		spec.nLevel = UnitGetExperienceLevel( pUnit );
		s_TreasureSpawnClass(pUnit, pStartLevelDef->nBonusStartingTreasure, &spec, NULL);
	}

	if (pStartLevelDef->nPlayerExpLevel > 1)
	{
		// remove newbie weapons to make room for new ones
		int nWardrobe = UnitGetWardrobe( pUnit );
		if ( nWardrobe != INVALID_ID )
		{
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
				if ( pWeapon )
				{
					UnitFree( pWeapon, UFF_SEND_TO_CLIENTS );
				}
			}
		}

		PlayerDoLevelUp( pUnit, pStartLevelDef->nPlayerExpLevel );
		cCurrency playerCurrency = UnitGetCurrency( pUnit );
		playerCurrency += cCurrency( pStartLevelDef->nStartingGold, 0 );
		//UnitSetStat(pUnit, STATS_GOLD, pStartLevelDef->nStartingGold);
#if ISVERSION(DEVELOPMENT)	// CHB 2006.12.22
		s_ItemGenerateRandomArmor( pUnit, FALSE );
		s_ItemGenerateRandomWeapons( pUnit, FALSE, 5 );
#endif
	}

	if (pStartLevelDef->nBonusStartingTreasure != INVALID_ID)
	{
		// remove newbie weapons to make room for new ones
		int nWardrobe = UnitGetWardrobe( pUnit );
		if ( nWardrobe != INVALID_ID )
		{
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
				if ( pWeapon )
				{
					UnitFree( pWeapon, UFF_SEND_TO_CLIENTS );
				}
			}
		}
		ITEM_SPEC spec;
		SETBIT(spec.dwFlags, ISF_STARTING_BIT);
		SETBIT(spec.dwFlags, ISF_PICKUP_BIT );
		spec.nLevel = UnitGetExperienceLevel( pUnit );
		s_TreasureSpawnClass(pUnit, pStartLevelDef->nBonusStartingTreasure, &spec, NULL);
	}

	// give the player all the waypoints necessary up to the starting level they are in
	int nNumLevelDefs = ExcelGetNumRows( pGame, DATATABLE_LEVEL );
	for (int i = 0; i < nNumLevelDefs; ++i)
	{
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( i );
		if ( !pLevelDef )
			continue;
		
		if (pLevelDef->nSequenceNumber != INVALID_LINK && 
			pLevelDef->nSequenceNumber <= pStartLevelDef->nSequenceNumber)
		{
			if ( pLevelDef->nWaypointMarker != INVALID_LINK )
			{
				WaypointGive( pGame, pUnit, pLevelDef );
			}
			if(AppIsHellgate())
			{
				LevelMarkVisited( pUnit, i, WORLDMAP_VISITED);
			}
		}
		
	}

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerToggleHardcore(
	UNIT * unit,
	BOOL bForceOn)
{
#if ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )

	BOOL bTurnOn = !PlayerIsHardcore( unit );
	if (bForceOn == TRUE)
	{
		bTurnOn = TRUE;
	}

	if (bTurnOn)
	{
		s_StateSet( unit, unit, STATE_HARDCORE, 0 );
	}
	else
	{
		s_StateClear( unit, UnitGetId( unit ), STATE_HARDCORE, 0 );
		s_StateClear( unit, UnitGetId( unit ), STATE_HARDCORE_DEAD, 0 );
	}
		
#endif //ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerTogglePVPOnly(
							UNIT * unit,
							BOOL bForceOn)
{
#if ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )

	BOOL bTurnOn = !PlayerIsPVPOnly( unit );
	if (bForceOn == TRUE)
	{
		bTurnOn = TRUE;
	}

	if (bTurnOn)
	{
		s_StateSet( unit, unit, STATE_PVPONLY, 0 );
	}
	else
	{
		s_StateClear( unit, UnitGetId( unit ), STATE_PVPONLY, 0 );
	}

#endif //ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerToggleLeague(
	UNIT * unit,
	BOOL bForceOn)
{
#if ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )

	BOOL bTurnOn = !PlayerIsLeague( unit );
	if (bForceOn == TRUE)
	{
		bTurnOn = TRUE;
	}

	if (bTurnOn)
	{
		s_StateSet( unit, unit, STATE_LEAGUE, 0 );
	}
	else
	{
		s_StateClear( unit, UnitGetId( unit ), STATE_LEAGUE, 0 );
	}

#endif //ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerToggleElite(
							UNIT * unit,
							BOOL bForceOn)
{
#if ISVERSION( DEVELOPMENT ) || !ISVERSION(CLIENT_ONLY_VERSION)

	BOOL bTurnOn = !PlayerIsElite( unit );
	if (bForceOn == TRUE)
	{
		bTurnOn = TRUE;
	}

	if (bTurnOn)
	{
		s_StateSet( unit, unit, STATE_ELITE, 0 );
	}
	else
	{
		s_StateClear( unit, UnitGetId( unit ), STATE_ELITE, 0 );
	}

#endif //ISVERSION( DEVELOPMENT ) || ISVERSION( SERVER_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_PlayerRemoveFromParty(
	UNIT * unit)
{
	if (unit)
	{
		CHAT_REQUEST_MSG_REMOVE_MEMBER_FROM_PARTY removeMsg;
		removeMsg.TagMemberToRemove = UnitGetGuid(unit);
		GameServerToChatServerSendMessage(&removeMsg, GAME_REQUEST_REMOVE_MEMBER_FROM_PARTY);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_STARTING_SKILLS_PER_UNIT 10
#if !ISVERSION( CLIENT_ONLY_VERSION )
UNIT * s_PlayerNew(
	GAME * game,
	GAMECLIENTID idClient,
	const WCHAR * name,
	int nClass,
	WARDROBE_INIT * pWardrobeInit,
	APPEARANCE_SHAPE * pAppearanceShape,
	DWORD dwNewPlayerFlags)
{
	BOOL bHardcore = dwNewPlayerFlags & NPF_HARDCORE;
	BOOL bSkipTutorialTips = dwNewPlayerFlags & NPF_DISABLE_TUTORIAL;
	
	BOOL bLeague = dwNewPlayerFlags & NPF_LEAGUE;
	BOOL bElite = dwNewPlayerFlags & NPF_ELITE;
	BOOL bPVPOnly = dwNewPlayerFlags & NPF_PVPONLY;

	const UNIT_DATA * player_data = PlayerGetData(game, nClass);
	if (!player_data)
	{
		nClass = 0;
		player_data = PlayerGetData(game, nClass);
		if (!player_data)
		{
			return NULL;
		}
	}
	
	// load the player, note that we're not putting them in any room during their init process
	PLAYER_SPEC tSpec;
	tSpec.idClient = idClient;
	tSpec.nPlayerClass = nClass;
	tSpec.pWardrobeInit = pWardrobeInit;
	tSpec.pAppearanceShape = pAppearanceShape;
	
	// create new player
	UNIT * unit = PlayerInit( game, tSpec );

	ASSERT_RETNULL(UnitSetGuid(unit, GameServerGenerateGUID()));	
	UnitSetName(unit, name);

	UnitSetStatShift(unit, STATS_HP_MAX, player_data->nMaxHitPointPct);
	UnitSetStatShift(unit, STATS_POWER_MAX, player_data->nMaxPower);
	
	// give the player base stats
	UnitSetStat(unit, STATS_ACCURACY, player_data->nStartingAccuracy );
	UnitSetStat(unit, STATS_STRENGTH, player_data->nStartingStrength );
	UnitSetStat(unit, STATS_STAMINA, player_data->nStartingStamina );
	UnitSetStat(unit, STATS_WILLPOWER, player_data->nStartingWillpower );
		
	// give properties	
	for (int ii = 0; ii < NUM_UNIT_PROPERTIES; ii++)
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_PLAYERS, player_data->codeProperties[ii], &code_len);
		if (code_ptr)
		{
			VMExecI(game, unit, code_ptr, code_len);
		}
	}

	// level up to the first level
	PlayerDoLevelUp(unit, 1);

	if ( player_data->nFiringErrorIncrease )
		UnitSetStat( unit, STATS_FIRING_ERROR_INCREASE_SOURCE, player_data->nFiringErrorIncrease );
	if ( player_data->nFiringErrorDecrease )
		UnitSetStat( unit, STATS_FIRING_ERROR_DECREASE_SOURCE, player_data->nFiringErrorDecrease );
	if ( player_data->nFiringErrorMax )
		UnitSetStat( unit, STATS_FIRING_ERROR_MAX, player_data->nFiringErrorMax );

	if ( bHardcore ) // this should be verified by the calling function
	{
		s_PlayerToggleHardcore( unit, TRUE );
	}

	if ( bPVPOnly ) // this should be verified by the calling function
	{
		s_PlayerTogglePVPOnly( unit, TRUE );
	}

	if(bLeague)
	{
		s_PlayerToggleLeague(unit, TRUE);
	}
	if(bElite)
	{
		s_PlayerToggleElite(unit, TRUE);
	}

	ITEM_SPEC spec;
	SETBIT(spec.dwFlags, ISF_STARTING_BIT);
	SETBIT(spec.dwFlags, ISF_PICKUP_BIT );
	s_TreasureSpawnClass(unit, player_data->nStartingTreasure, &spec, NULL);

	//new players need all the weaponconfig tags
	//  and they need the starting skills to be in them
	for (int i=TAG_SELECTOR_WEAPCONFIG_HOTKEY1; i <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; i++)
	{
		UNIT_TAG_HOTKEY *pTag = UnitAddHotkeyTag(unit, i);
		ASSERT_RETNULL(pTag);
	}

	// set the first minigame
	if(AppIsHellgate())
	{
		PlayerSetStartingMinigameTags(game, unit);
	}

	// clear all waypoint flags
	if (IS_SERVER(game))
	{
		s_PlayerInitWaypointFlags(game, unit);
	}

	UnitClearFlag(unit, UNITFLAG_JUSTCREATED);

	// Give the player level 1 in their starting skill(s) - after clearing the just created flag
	for (int ii = 0; ii < NUM_UNIT_START_SKILLS; ii++)
	{
		if (player_data->nStartingSkills[ii] == INVALID_ID)
		{
			continue;
		}

		SkillPurchaseLevel(unit, player_data->nStartingSkills[ii], FALSE);
	}

	//SkillsSelectAll( game, unit );

	// give default hotkey (this needs to be made data-driven), select either a skill or an item
	int nHotkeyCurr = TAG_SELECTOR_HOTKEY1;
	UNIT_TAG_HOTKEY* tag = UnitAddHotkeyTag(unit, nHotkeyCurr);
	if (tag)
	{
		tag->m_nLastUnittype = AppIsHellgate() ? UNITTYPE_MEDPACK : UNITTYPE_HEALTHPOTION;
		UNIT* item = InventoryGetReplacementItemFromLocation(unit, UnitGetBigBagInvLoc(), tag->m_nLastUnittype);
		if (item)
		{
			tag->m_idItem[0] = UnitGetId(item);
		}
		nHotkeyCurr++;
	}

	if( AppIsTugboat() )
	{
		tag = UnitAddHotkeyTag(unit, nHotkeyCurr);
		if (tag)
		{
			tag->m_nLastUnittype = UNITTYPE_MANAPOTION;
			UNIT* item = InventoryGetReplacementItemFromLocation(unit, UnitGetBigBagInvLoc(), tag->m_nLastUnittype);
			if (item)
			{
				tag->m_idItem[0] = UnitGetId(item);
			}
			nHotkeyCurr++;
		}
	}

	tag = UnitAddHotkeyTag(unit, nHotkeyCurr);
	UNIT_ITERATE_STATS_RANGE( unit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_STARTING_SKILLS_PER_UNIT )
	{
		int nSkill = pStatsEntry[ jj ].GetParam();
		const SKILL_DATA * pSkillData = SkillGetData( game, nSkill );

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_HOTKEYABLE ) &&
			pSkillData->nRequiredUnittype != UNITTYPE_ANY &&
			pSkillData->nRequiredUnittype != UNITTYPE_CREATURE &&
			tag)
		{
			tag->m_nSkillID[ 0 ] = nSkill;
			nHotkeyCurr++;
			tag = UnitAddHotkeyTag(unit, nHotkeyCurr);
		}
	}
	UNIT_ITERATE_STATS_END;

	// if the player has something in their hands, these need to be in a weaponconfig
	UNIT_TAG_HOTKEY * pTag = UnitGetHotkeyTag(unit, TAG_SELECTOR_WEAPCONFIG_HOTKEY1);
	ASSERT_RETNULL(pTag);
	for (int i=0; i<MAX_HOTKEY_ITEMS; i++)
	{
		UNIT * pItem = UnitInventoryGetByLocation(unit, GetWeaponconfigCorrespondingInvLoc(i));
		if (pItem)
		{
			pTag->m_idItem[i] = UnitGetId(pItem);
		}
	}

	// set the starting location
	s_sPlayerSetup( unit );

	// don't display tips if set that way from client
#if HELLGATE_ONLY
	if ( bSkipTutorialTips )
	{
		s_TutorialTipsOff( unit );
	}
#else
	REF( bSkipTutorialTips );
#endif
	UnitTriggerEvent( game, EVENT_CREATED, unit, NULL, NULL );

	// figure out what skills should be hotkeyed where
	for (int i=TAG_SELECTOR_WEAPCONFIG_HOTKEY1; i <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; i++)
	{
		s_WeaponconfigUpdateSkills( unit, i );
	}

	PlayerInventorySetGameVariants(unit);

	if( AppIsTugboat() ) 
	{
		// TRAVIS: Tug has gone to a no-scroll skilltree system. Now we have a default skill

		if( player_data->nRightSkill != INVALID_ID )
		{
			HotkeySet( game, unit, 1, player_data->nRightSkill );
		}
		/*
		// find any spell scrolls in the starting player's inventory and learn them.
		UNIT* pItem = NULL;
		pItem = UnitInventoryIterate( unit, pItem );
		while ( pItem != NULL)
		{
			UNIT* pNext = UnitInventoryIterate( unit, pItem );
			if ( UnitIsA( pItem, UNITTYPE_SPELLSCROLL ) )
			{
				s_ItemUse (game, unit, pItem );				
			}	
			pItem = pNext;
		}*/
	}
	
	// mark player as new so we can put them in the database if they get into a game
	UnitSetFlag( unit, UNITFLAG_NEED_INITIAL_DATABASE_ADD );

	// mark player as needing to load the shared stash units for this account
	#if ISVERSION(SERVER_VERSION)
		UnitSetFlag( unit, UNITFLAG_LOAD_SHARED_STASH );
	#endif
	
	//s_GiveBadgeRewards(unit);

	// Default player options here
	UnitSetStat(unit, STATS_CHAR_OPTION_ALLOW_INSPECTION, 1 );

	return unit;	
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct STARTING_MINIGAME_TAG {
	int		m_nGoalCategory;
	int		m_nGoalType;
	int		m_nMinNeeded;
	int		m_nMaxNeeded;
} tStartingMinigameTags[] = 
{
	{ MINIGAME_DMG_TYPE,		DAMAGE_TYPE_PHYSICAL,		10, 20},
	{ MINIGAME_DMG_TYPE,		DAMAGE_TYPE_FIRE,			10, 20},		    
	{ MINIGAME_DMG_TYPE,		DAMAGE_TYPE_ELECTRICITY,	10, 20},	    
	{ MINIGAME_DMG_TYPE,		DAMAGE_TYPE_SPECTRAL,		10, 20},	    
	{ MINIGAME_DMG_TYPE,		DAMAGE_TYPE_TOXIC,			10, 20},		    

	{ MINIGAME_KILL_TYPE,		UNITTYPE_BEAST,				 5, 10},		
	{ MINIGAME_KILL_TYPE, 		UNITTYPE_UNDEAD,			 5, 10},	
	{ MINIGAME_KILL_TYPE, 		UNITTYPE_DEMON,				 5, 10},		
	{ MINIGAME_KILL_TYPE, 		UNITTYPE_SPECTRAL,			 5, 10},	

	{ MINIGAME_CRITICAL, 		0,							15, 30},	
//	{ MINIGAME_QUEST_FINISH, 	0,							 1,  1},	
	{ MINIGAME_ITEM_TYPE, 		UNITTYPE_ARMOR,				 3,  6},	
	{ MINIGAME_ITEM_TYPE, 		UNITTYPE_MELEE,				 1,  3},	
	{ MINIGAME_ITEM_TYPE, 		UNITTYPE_GUN,				 1,  3},	
	{ MINIGAME_ITEM_TYPE, 		UNITTYPE_MOD,				 2,  5},	
	{ MINIGAME_ITEM_MAGIC_TYPE, 0,							 1,  1},	

};

void PlayerSetStartingMinigameTags(
	GAME* game,
	UNIT* unit)
{
	ASSERT(unit);
	ASSERT(IS_SERVER(game));
	ASSERT(UnitGetClientId(unit) != INVALID_ID);

	if (unit && IS_SERVER(game) && UnitHasClient(unit))
	{
		PickerStart(game, picker);
		UnitClearStatsRange(unit, STATS_MINIGAME_CATEGORY_NEEDED);

		for (int i=0; i < arrsize(tStartingMinigameTags); i++)
		{
			PickerAdd(MEMORY_FUNC_FILELINE(game, i, 1));
		}

		for (int i=0; i < NUM_MINIGAME_SLOTS; i++)
		{
			int idx = PickerChooseRemove(game);
			ASSERT(idx >= 0 && idx < arrsize(tStartingMinigameTags));

			int nNeeded = RandGetNum(game, tStartingMinigameTags[idx].m_nMinNeeded, tStartingMinigameTags[idx].m_nMaxNeeded);
			UnitSetStat(unit, STATS_MINIGAME_CATEGORY_NEEDED, tStartingMinigameTags[idx].m_nGoalCategory, tStartingMinigameTags[idx].m_nGoalType, nNeeded);

		}

		//UnitSetStat(unit, STATS_MINIGAME_TICKS_LEFT, MINIGAME_TIMEOUT);
		//if (!UnitHasTimedEvent(unit, s_PlayerMinigameCheckTimeout))
		//{
		//	UnitRegisterEventTimed(unit, s_PlayerMinigameCheckTimeout, NULL, MINIGAME_CHECK_TIMEOUT_FREQ);
		//}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerDoMinigameReset(
	GAME* game,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	PlayerSetStartingMinigameTags(game, unit);

	// schedule another one
	//UnitRegisterEventTimed(unit, s_PlayerDoMinigameReset, NULL, MINIGAME_TIMEOUT);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerMinigameCheckTimeout(
	GAME* game,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	ASSERT_RETFALSE(unit);

	//int nTicksLeft = UnitGetStat(unit, STATS_MINIGAME_TICKS_LEFT);

	//nTicksLeft -= MINIGAME_CHECK_TIMEOUT_FREQ;

	//if (nTicksLeft <= 0)
	//{
	//	PlayerSetStartingMinigameTags(game, unit);
	//}
	//else
	//{
	//	UnitSetStat(unit, STATS_MINIGAME_TICKS_LEFT, nTicksLeft);

	//	// schedule another one
	//	UnitRegisterEventTimed(unit, s_PlayerMinigameCheckTimeout, NULL, MINIGAME_CHECK_TIMEOUT_FREQ);
	//}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_PlayerCheckMinigame(GAME *game, UNIT *unit, int nGoalCategory, int nGoalType)
{
	ASSERT(unit);
	ASSERT(IS_SERVER(game));
	ASSERT(UnitGetClientId(unit) != INVALID_ID);

	BOOL bAchievedOne = FALSE;
	if (unit && IS_SERVER(game) && UnitHasClient(unit))
	{
		UNIT_ITERATE_STATS_RANGE(unit, STATS_MINIGAME_CATEGORY_NEEDED, statsentry, ii, NUM_MINIGAME_SLOTS)
		{
			if (statsentry[ii].value > 0)
			{
				PARAM param = statsentry[ii].GetParam();
				int nCheckGoalCategory = StatGetParam(STATS_MINIGAME_CATEGORY_NEEDED, param, 0);
				int nCheckGoalType = StatGetParam(STATS_MINIGAME_CATEGORY_NEEDED, param, 1);

				if (nCheckGoalCategory != nGoalCategory)
				{
					continue;
				}

				if (nGoalCategory == MINIGAME_KILL_TYPE ||
					nGoalCategory == MINIGAME_ITEM_TYPE)
				{
					if (!UnitIsA(nGoalType, nCheckGoalType))
					{
						continue;
					}
				}

				if (nGoalCategory == MINIGAME_DMG_TYPE &&
					nCheckGoalType != nGoalType)
				{
					continue;
				}

				UnitChangeStat(unit, STATS_MINIGAME_CATEGORY_NEEDED, param, -1);

				bAchievedOne = TRUE;
			}
		}
		UNIT_ITERATE_STATS_END;

		if (bAchievedOne &&
			UnitGetStatsTotal(unit, STATS_MINIGAME_CATEGORY_NEEDED) == 0)
		{
			// Give a reward here
			int nTreasureClass = GlobalIndexGet(GI_TREASURE_MINIGAME);
			if (nTreasureClass > INVALID_LINK)
			{
				ITEM_SPEC spec;
				SETBIT(spec.dwFlags, ISF_PLACE_IN_WORLD_BIT);
				SETBIT(spec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT);
				spec.guidRestrictedTo = UnitGetGuid(unit);
				s_TreasureSpawnClass(unit, nTreasureClass, &spec, NULL);
			}

			// Tell the client to play a fanfare
			MSG_SCMD_UNITPLAYSOUND msg;
			msg.id = UnitGetId(unit);
			msg.idSound = GlobalIndexGet( GI_SOUND_MINIGAME_FANFARE );
			if (msg.idSound != INVALID_LINK)
			{
				s_SendMessage(game, UnitGetClientId(unit), SCMD_UNITPLAYSOUND, &msg);
			}
				
			// send to the achievements system
			s_AchievementsSendMinigameWin(unit);

			// reset the minigame
//			UnitUnregisterTimedEvent(unit, s_PlayerMinigameCheckTimeout);	// stop checking the timeout, when the minigame restarts after GAME_TICKS_PER_SECOND * 5 it will start the check up again
			UnitUnregisterTimedEvent(unit, s_PlayerDoMinigameReset);		// unregister an previous reset that was scheduled (there should have been one)
			UnitRegisterEventTimed(unit, s_PlayerDoMinigameReset, NULL, GAME_TICKS_PER_SECOND * 5);
		}
	}

	return bAchievedOne;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_PlayerRemoveInvalidMinigameEntries(GAME *game, UNIT *unit)
{
	ASSERT(unit);
	ASSERT(IS_SERVER(game));
	ASSERT(UnitGetClientId(unit) != INVALID_ID);

	if (unit && IS_SERVER(game) && UnitHasClient(unit))
	{
		UNIT_ITERATE_STATS_RANGE(unit, STATS_MINIGAME_CATEGORY_NEEDED, statsentry, ii, NUM_MINIGAME_SLOTS)
		{
			if (statsentry[ii].value > 0)
			{
				PARAM param = statsentry[ii].GetParam();
				int nCheckGoalCategory = StatGetParam(STATS_MINIGAME_CATEGORY_NEEDED, param, 0);
				int nCheckGoalType = StatGetParam(STATS_MINIGAME_CATEGORY_NEEDED, param, 1);

				BOOL bMatchFound = FALSE;
				for (int iTag=0; iTag < arrsize(tStartingMinigameTags); iTag++)
				{
					if (tStartingMinigameTags[iTag].m_nGoalCategory == nCheckGoalCategory &&
						tStartingMinigameTags[iTag].m_nGoalType == nCheckGoalType)
					{
						bMatchFound = TRUE;
						break;
					}
				}

				if (!bMatchFound)
				{
					// An invalid minigame tag was found.  Reset the whole minigame.
					PlayerSetStartingMinigameTags(game, unit);
					return;
				}

			}
		}
		UNIT_ITERATE_STATS_END;

	}

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDecrementExperiencePenaltyLeft(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pUnit ), "Expected player" );
		
	// take one second away
	int nSecondsToRemove = EXPERIENCE_PENALTY_UPDATE_RATE_IN_TICKS / GAME_TICKS_PER_SECOND;
	int nNewTimeInSeconds = UnitGetStat( pUnit, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS ) - nSecondsToRemove;
	
	// keep going if we have more time left
	if (nNewTimeInSeconds > 0)
	{
	
		// set the new duration
		UnitSetStat( pUnit, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS, nNewTimeInSeconds );
		
		// register event to remove more time 
		UnitRegisterEventTimed( 
			pUnit, 
			sDecrementExperiencePenaltyLeft, 
			NULL, 
			EXPERIENCE_PENALTY_UPDATE_RATE_IN_TICKS );
	}
	else
	{
	
		// clear stat
		UnitClearStat( pUnit, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS, 0 );

		// the state system will clear the state because we have given it a duration, but we'll
		// clear it here just to be extra certain it goes away
		s_StateClear( pUnit, INVALID_ID, STATE_EXPERIENCE_PENALTY, 0 );
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sPlayerGiveExperiencePenalty(
	UNIT *pPlayer,
	int nDurationInSeconds)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// tugboat doens't do this
	if (AppIsTugboat())
	{
		return;
	}
		
	// will we do a penalty at all
	BOOL bDoPenalty = sgbEnableExperienceDeathPenalty;

	// don't do penalty if one is not set
	int nPercent = s_UnitGetPossibleExperiencePenaltyPercent( pPlayer );
	if (nPercent >= 100)
	{
		bDoPenalty = FALSE;
	}
		
	if (bDoPenalty)
	{
		
		// if no duration specified, use the default
		if (nDurationInSeconds == 0)
		{
			const STATE_DATA *pStateData = StateGetData( pGame, STATE_EXPERIENCE_PENALTY );
			if(pStateData) nDurationInSeconds = pStateData->nDefaultDuration / GAME_TICKS_PER_SECOND;
		}

		// remove any previous record of this penalty in place already
		s_PlayerClearExperiencePenalty( pPlayer );
						
		// set state	
		int nDurationInTicks = nDurationInSeconds * GAME_TICKS_PER_SECOND;
		s_StateSet( pPlayer, NULL, STATE_EXPERIENCE_PENALTY, nDurationInTicks );
					
		// set the new duration into the stat
		UnitSetStat( pPlayer, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS, nDurationInSeconds );
			
		// set an event to remove the penalty stat once second at a time
		UnitRegisterEventTimed( 
			pPlayer, 
			sDecrementExperiencePenaltyLeft, 
			NULL, 
			EXPERIENCE_PENALTY_UPDATE_RATE_IN_TICKS);
							
	}
	else
	{
		// not enabled, clear any stat and state we might of had just to be clean
		s_PlayerClearExperiencePenalty( pPlayer );		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_PlayerAdd(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);

	ClientSetPlayerUnit(game, client, unit);
	
	if (IS_SERVER(game))
	{
		ClientSystemSetGuid(AppGetClientSystem(), ClientGetAppId(client), UnitGetGuid(unit));
	}
	
	ClientSetState(client, GAME_CLIENT_STATE_INGAME);
	trace("new player added (id:%d)\n", ClientGetId( client ));

	// player is now ready to be known about
	ASSERTX(UnitIsInWorld(unit), "Player must be in world now");
	UnitSetCanSendMessages(unit, TRUE);
	UnitSetWardrobeChanged(unit, TRUE);
	// inform clients about new unit
	ROOM * room = UnitGetRoom(unit);
	s_SendUnitToAllWithRoom(game, unit, room);

	// send hotkeys
	HotkeysSendAll(game, client, unit, TAG_SELECTOR_HOTKEY1, (TAG_SELECTOR)TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST);

	// skills
	s_SkillsSelectAll(game, unit);

	// trigger event on all units in the game that a new player is here
	UnitsAllTriggerEvent(game, EVENT_PLAYER_JOIN, unit, NULL);
	
	// tell task system player is here
	s_TaskPlayerJoinGame(unit);

	s_CustomGameEventAddPlayer(game, unit);

	s_CTFEventAddPlayer(game, unit);

	// tell quest system a new player is here (must happen at and of playeradd)
	s_QuestPlayerJoinGame(unit);

	// this updates all weapons to broadcast an equip event. Which allows skills to modify the weapons.	
	if( AppIsTugboat() )
	{
		DWORD flags( 0 );	
		SETBIT( flags, IIF_EQUIP_SLOTS_ONLY_BIT );
		InvBroadcastEventOnUnitsByFlags( unit, EVENT_EQUIPED, flags );
	}	
	
	// init store information for this player
	s_StoreInit( unit );

	// re-enable the visual state for an experience penalty if present
	int nExpPenaltyInSeconds = UnitGetStat( unit, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS );
	if (nExpPenaltyInSeconds > 0)
	{
		s_sPlayerGiveExperiencePenalty( unit, nExpPenaltyInSeconds );
	}
	
	// this unit is now ready for database operations
	s_DatabaseUnitEnable( unit, TRUE );
	
	// for new players, add them to the database now
	BOOL bCommitDBOperations = FALSE;
	if (UnitTestFlag( unit, UNITFLAG_NEED_INITIAL_DATABASE_ADD ))
	{
		// add new player
		s_DatabaseUnitAddAll( unit, unit );
		UnitClearFlag( unit, UNITFLAG_NEED_INITIAL_DATABASE_ADD  );
		
		// we want a commit right now
		bCommitDBOperations = TRUE;
		
	}

	// check for any items on the player that need database updates right now
	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryIterate( unit, pItem )) != NULL)
	{
		if (UnitTestFlag( pItem, UNITFLAG_NEEDS_DATABASE_UPDATE ))
		{
			
			// queue update
			s_DatabaseUnitOperation( UnitGetUltimateOwner( pItem ), pItem, DBOP_UPDATE );
			
			// clear the flag
			UnitClearFlag( pItem, UNITFLAG_NEEDS_DATABASE_UPDATE );
			
			// make the commits happen right now 
			bCommitDBOperations = TRUE;
			
		}
		
	}
	
	// commit current queue operations right now so new player is saved
	if (bCommitDBOperations)
	{
		s_DBUnitOperationsBatchedCommit( unit, DBOP_ADD, TRUE );	
	}
	
	// tell the DB unit system a player is now loaded
	s_DBUnitPlayerAddedToGame( unit );

	// give any special items to this player based on their account badges
	// like giving special items to buyers of the collecters edition
	s_GiveBadgeRewards( unit );
	
	// check for minigame entries (stats) a player might have that have been removed
	s_PlayerRemoveInvalidMinigameEntries(game, unit);

	// set up an event to check the timout on the minigame
	//if (!UnitHasTimedEvent(unit, s_PlayerMinigameCheckTimeout))
	//{
		// if they've got a completed game (can happen if you warp just after completing)
		//    do a reset on it
		if (UnitGetStatsTotal(unit, STATS_MINIGAME_CATEGORY_NEEDED) == 0)
		{
			s_PlayerDoMinigameReset(game, unit, EVENT_DATA());
		}
	//	else
	//	{
	//		UnitRegisterEventTimed(unit, s_PlayerMinigameCheckTimeout, NULL, MINIGAME_CHECK_TIMEOUT_FREQ);
	//	}
	//}

	// KCK: I think this is better implemented as a client pull rather than a server push,
	// But if tugboat wants to do it this way they can call sCCmdListGameInstances();
//	SendTownInstanceLists( game, unit );

	// load the shared stash for new players
	if (UnitTestFlag( unit, UNITFLAG_LOAD_SHARED_STASH ))
	{

		// queue the load (but only for subscribers)
		PlayerLoadAccountUnitsFromDatabase( unit );

		// clear the flag that we need to load the shared stash
		UnitClearFlag( unit, UNITFLAG_LOAD_SHARED_STASH );
		
	}

	return TRUE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
// save a player file to a data buffer
//----------------------------------------------------------------------------
unsigned int PlayerSaveToBuffer(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	UNITSAVEMODE savemode,
	BYTE * data,
	unsigned int size)
{
	ASSERT_RETZERO(unit);
	ASSERT_RETZERO(data);
	ASSERT_RETZERO(size > 0);

#if !ISVERSION( CLIENT_ONLY_VERSION )
	UnitIdMapReset(game->m_pUnitIdMap, FALSE);	// id map is used to store unit ids in a file
	
	// setup xfer
	BIT_BUF buf(data, size);
	XFER<XFER_SAVE> tXfer(&buf );
	UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec(tXfer);
	tSpec.pUnitExisting = unit;
	tSpec.eSaveMode = savemode;
	tSpec.idClient = (client != NULL ? ClientGetId( client ) : INVALID_GAMECLIENTID);
	if (UnitFileXfer( game, tSpec ) != unit)
	{
		const int MAX_MESSAGE = 256;
		char szMessage[ MAX_MESSAGE ];
		PStrPrintf( 
			szMessage, 
			MAX_MESSAGE, 
			"Error with save/load, please attach character files to bug.  Unable to xfer unit '%s' [%d] for saving to buffer",
			UnitGetDevName( unit ),
			UnitGetId( unit ));
		ASSERTX_RETZERO( 0, szMessage );							
	}
	
	ASSERT_RETZERO(buf.GetLen() > 0);
		
	int nSavedSize = buf.GetLen();	
	ASSERTX_RETZERO(nSavedSize > 0, "Player save buffer size is zero" );
	
	return nSavedSize;
	
#else
	REF( game );
	REF( client );
	REF( savemode );
	return 0;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

}
//----------------------------------------------------------------------------
// Convert string to guid
//----------------------------------------------------------------------------
BOOL ConvertStringToGUID( const WCHAR* strIn, GUID* pGuidOut )
{
    UINT aiTmp[10];

    if( swscanf_s( strIn, L"{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}",
                    &pGuidOut->Data1, 
                    &aiTmp[0], &aiTmp[1], 
                    &aiTmp[2], &aiTmp[3],
                    &aiTmp[4], &aiTmp[5],
                    &aiTmp[6], &aiTmp[7],
                    &aiTmp[8], &aiTmp[9] ) != 11 )
    {
        ZeroMemory( pGuidOut, sizeof(GUID) );
        return FALSE;
    }
    else
    {
        pGuidOut->Data2       = (USHORT) aiTmp[0];
        pGuidOut->Data3       = (USHORT) aiTmp[1];
        pGuidOut->Data4[0]    = (BYTE) aiTmp[2];
        pGuidOut->Data4[1]    = (BYTE) aiTmp[3];
        pGuidOut->Data4[2]    = (BYTE) aiTmp[4];
        pGuidOut->Data4[3]    = (BYTE) aiTmp[5];
        pGuidOut->Data4[4]    = (BYTE) aiTmp[6];
        pGuidOut->Data4[5]    = (BYTE) aiTmp[7];
        pGuidOut->Data4[6]    = (BYTE) aiTmp[8];
        pGuidOut->Data4[7]    = (BYTE) aiTmp[9];
        return TRUE;
    }
}
//------------------------------------------------------
// Remove Rich Save Header from point - return Pointer Past Headedr
//------------------------------------------------------
BYTE * RichSaveFileRemoveHeader(
	BYTE* data, 
	DWORD* size)
{
	ASSERT_RETNULL(data);
#if ISVERSION(DEVELOPMENT)
	RICH_GAME_MEDIA_HEADER header;// = (RICH_GAME_MEDIA_HEADER)data;
	MemoryCopy(&header, sizeof(RICH_GAME_MEDIA_HEADER), data, sizeof(RICH_GAME_MEDIA_HEADER));
#endif
	BYTE * Data_Point = &data[sizeof(RICH_GAME_MEDIA_HEADER)];
	(*size)-= (sizeof(RICH_GAME_MEDIA_HEADER));
	return Data_Point;
}
//------------------------------------------------------
// Add Rich Save Header to file - return pointer past Header
//------------------------------------------------------
static BYTE * sAddRichSaveHeader(
	BYTE* pBuffer,
	DWORD dwBufferSize,
	WCHAR * szName, 
	DWORD * size)
{
	RICH_GAME_MEDIA_HEADER RichSaveHeader;
	RichSaveHeader.dwMagicNumber = RM_MAGICNUMBER;
	RichSaveHeader.dwHeaderVersion = 1;
	RichSaveHeader.dwHeaderSize = sizeof(RICH_GAME_MEDIA_HEADER);
	RichSaveHeader.liThumbnailOffset.QuadPart = sizeof(RICH_GAME_MEDIA_HEADER);
	RichSaveHeader.dwThumbnailSize = 0;			// Get ScreenShot Size
	PStrCopy(RichSaveHeader.szGameName, L"Hellgate: London", RM_MAXLENGTH);
	ConvertStringToGUID(L"6D03749A-A92D-47DF-AD7B-BDDF24DA5B86", &RichSaveHeader.guidGameId);
	PStrCopy(RichSaveHeader.szSaveName, szName, RM_MAXLENGTH);
	PStrCopy(RichSaveHeader.szLevelName, L"No detail", RM_MAXLENGTH);
	PStrCopy(RichSaveHeader.szComments, L"Load this Character", RM_MAXLENGTH);

	UINT Offset = (UINT)sizeof(RICH_GAME_MEDIA_HEADER);
	MemoryCopy(pBuffer, dwBufferSize, &RichSaveHeader,  sizeof(RICH_GAME_MEDIA_HEADER));
	if(RichSaveHeader.dwThumbnailSize > 0)
	{
		BYTE * GameImage = 0;
		MemoryCopy(&pBuffer[Offset], UNITFILE_MAX_SIZE_ALL_UNITS, GameImage, RichSaveHeader.dwThumbnailSize);
		Offset += RichSaveHeader.dwThumbnailSize;
	}
	//////////////////////////////////////////////////////
	// Move Data Pointer past Header information
	//////////////////////////////////////////////////////
	BYTE * pDataPoint = &pBuffer[Offset];
	(*size) = Offset;
	return pDataPoint;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sSaveRichSaveFile(
	BYTE *pBuffer,
	DWORD dwBufferSize,
	UNIT *pUnit,
	UNITSAVEMODE eSaveMode)
{
	ASSERTX_RETZERO( pBuffer, "Expected buffer to store save data in" );
	ASSERTX_RETZERO( dwBufferSize <= UNITFILE_MAX_SIZE_ALL_UNITS, "Invalid buffer size for save file" );
	ASSERTX_RETZERO( pUnit, "Expected unit for save" );
	
	// add rich header as first thing
	DWORD dwRichHeaderSize = 0;	
	BYTE *pSaveDataBufferStart = sAddRichSaveHeader( pBuffer, dwBufferSize, pUnit->szName, &dwRichHeaderSize );
	ASSERTX_RETZERO( pSaveDataBufferStart, "Expected beginning of save data buffer" );
	ASSERTX_RETZERO( dwRichHeaderSize > 0, "Invalid rich header save file size" );
	
	// we need to know how much room is left in the buffer for the actual save now
	DWORD dwSizeRemaining = dwBufferSize - dwRichHeaderSize;
	ASSERTX_RETZERO( dwSizeRemaining > 0, "Buffer too small for saving player" );
	
	// save unit data
	GAME *pGame = UnitGetGame( pUnit );
	GAMECLIENT *pClient = UnitGetClient( pUnit );
	DWORD dwDataSize = PlayerSaveToBuffer( pGame, pClient, pUnit, eSaveMode, pSaveDataBufferStart, dwSizeRemaining );
	ASSERTX_RETZERO( dwDataSize > 0, "Error saving player to buffer" );

	// return total size
	DWORD dwTotalSize = dwRichHeaderSize + dwDataSize;
	ASSERTX_RETZERO( dwTotalSize <= dwBufferSize, "Exceeded save buffer size" );
	return dwTotalSize;
	
}

//----------------------------------------------------------------------------
// save a player to a file on disk
//----------------------------------------------------------------------------
BOOL PlayerSaveToFile(
	UNIT * unit)
{
	if ( AppCommonGetDemoMode() )
	{
		// Don't save the fake benchmark mode player!
		return FALSE;
	}

	ASSERT_RETFALSE(unit);
	
	if (!unit->szName)
	{
		return FALSE;
	}
	
	// storage for player file data
	BYTE data[ UNITFILE_MAX_SIZE_ALL_UNITS ];
	
	// save a rich save file
	DWORD dwSize = sSaveRichSaveFile( data, UNITFILE_MAX_SIZE_ALL_UNITS, unit, UNITSAVEMODE_FILE );
	ASSERTX_RETFALSE( dwSize > 0 && dwSize < UNITFILE_MAX_SIZE_ALL_UNITS, "Error saving player to buffer" );
		
	OS_PATH_CHAR szName[MAX_PATH];
	PStrCvt(szName, unit->szName, _countof(szName));

	const OS_PATH_CHAR * pszWidePath = FilePath_GetSystemPath(FILE_PATH_SAVE);
	if (!FileDirectoryExists(pszWidePath))
	{
		FileCreateDirectory(pszWidePath);
	}

	OS_PATH_CHAR szFileTemp[MAX_PATH];
	PStrPrintf(szFileTemp, _countof(szFileTemp), OS_PATH_TEXT("%s%s.tmp"), pszWidePath, szName);

	BOOL bSuccess = TRUE;
	if (FileSave(szFileTemp, data, dwSize))
	{
		OS_PATH_CHAR szNewFile[MAX_PATH];
		PStrPrintf(szNewFile, _countof(szNewFile), OS_PATH_TEXT("%s%s.hg1"), pszWidePath, szName);
		if (!FileRename(szFileTemp, szNewFile, TRUE))
		{
			char szSingleByteNameOld[ MAX_PATH ];
			PStrCvt( szSingleByteNameOld, szFileTemp, MAX_PATH );
			char szSingleByteNameNew[ MAX_PATH ];
			PStrCvt( szSingleByteNameNew, szNewFile, MAX_PATH );
			ASSERTV(
				0, 
				"Error with FileRename() when renaming player file '%s' to '%s'",
				szSingleByteNameOld,
				szSingleByteNameNew);
			bSuccess = FALSE;
		}
	
	}
	else
	{
		char szSingleByteName[ MAX_PATH ];
		PStrCvt( szSingleByteName, szFileTemp, MAX_PATH );
		ASSERTV(
			0, 
			"Error with FileSave() when saving player data (filename: %s, size: %d) to disk",
			szSingleByteName,
			dwSize);	
		bSuccess = FALSE;			
	}
		
	return bSuccess;

}

//----------------------------------------------------------------------------
// save player's byte buffer to a file.
//----------------------------------------------------------------------------
BOOL PlayerSaveByteBufferToFile(WCHAR * szUnitName, BYTE * SaveFile, DWORD SaveSize)
{
	ASSERT_RETFALSE(szUnitName);
	ASSERT_RETFALSE(SaveFile);

	OS_PATH_CHAR szName[MAX_PATH];
	PStrCvt(szName, szUnitName, _countof(szName));

	const OS_PATH_CHAR * pszWidePath = FilePath_GetSystemPath(FILE_PATH_SAVE);

	OS_PATH_CHAR szFile[MAX_PATH];
	PStrPrintf(szFile, _countof(szFile), OS_PATH_TEXT("%s%s.hg1"), pszWidePath, szName);

	if (!FileDirectoryExists(pszWidePath))
	{
		FileCreateDirectory(pszWidePath);
	}

	FileSave(szFile, SaveFile, SaveSize);
	
	FREE(NULL, SaveFile);
	FREE(NULL, szUnitName);

	return TRUE;
}

//----------------------------------------------------------------------------
// save a player & send to remote client
//----------------------------------------------------------------------------
BOOL PlayerSaveToSendBuffer(
	UNIT * unit)
{
	GAME * game = UnitGetGame(unit);

	ASSERT_RETFALSE(unit);
	if (!unit->szName)
	{
		return FALSE;
	}

	BYTE data[ UNITFILE_MAX_SIZE_ALL_UNITS ];
	DWORD dwSize = sSaveRichSaveFile( data, UNITFILE_MAX_SIZE_ALL_UNITS, unit, UNITSAVEMODE_FILE );
	ASSERT_RETFALSE(dwSize > 0 && dwSize <= UNITFILE_MAX_SIZE_ALL_UNITS);
	
	// send the file (todo: delay send)
	MSG_SCMD_OPEN_PLAYER_INITSEND msg;
	msg.dwCRC = CRC(0, data, dwSize);
	msg.dwSize = dwSize;

	// send
	GAMECLIENTID idClient = UnitGetClientId(unit);	
	s_SendMessage(game, idClient, SCMD_OPEN_PLAYER_INITSEND, &msg);

	BYTE *pDataToSend = data;
	DWORD dwLeftToSend = dwSize;
	while (dwLeftToSend != 0)
	{
		DWORD dwSizeToSend = MIN(dwLeftToSend, (DWORD)MAX_PLAYERSAVE_BUFFER);

		MSG_SCMD_OPEN_PLAYER_SENDFILE msg;	
		MemoryCopy(msg.buf, MAX_PLAYERSAVE_BUFFER, pDataToSend, dwSizeToSend );
		MSG_SET_BLOB_LEN( msg, buf, dwSizeToSend );
		s_SendMessage( game, idClient, SCMD_OPEN_PLAYER_SENDFILE, &msg);

		pDataToSend += dwSizeToSend;
		dwLeftToSend -= dwSizeToSend;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// save a player & store in appclient 
//----------------------------------------------------------------------------
BOOL PlayerSaveToAppClientBuffer(
	UNIT * unit,
	DWORD key)
{
	GAME * game = UnitGetGame(unit);
	GAMECLIENTID idClient = UnitGetClientId(unit);
	GAMECLIENT * client = ClientGetById(game, idClient);

	ASSERT_RETFALSE(unit);
	if (!unit->szName)
	{
		return FALSE;
	}

	BYTE data[ UNITFILE_MAX_SIZE_ALL_UNITS ];
	unsigned int size = PlayerSaveToBuffer(game, client, unit, UNITSAVEMODE_FILE, data, UNITFILE_MAX_SIZE_ALL_UNITS);
	ASSERT_RETFALSE(size > 0 && size <= UNITFILE_MAX_SIZE_ALL_UNITS);

	BOOL retval = ClientSystemSetClientInstanceSaveBuffer(
						AppGetClientSystem(), 
						ClientGetAppId(client), 
						unit->szName, 
						data, 
						size, 
						key);
	return retval;

}

#if ISVERSION(SERVER_VERSION)
#include "database.h"
#include "DatabaseManager.h"
#include <vector>
#include <string>

#define TEST_GARBAGE_SAVE_DATA 0

#if TEST_GARBAGE_SAVE_DATA
#define TEST_GARBAGE_UNITDATA
//#define TEST_GARBAGE_UNITPARAMS
// We don't bother testing garbaging the wardrobe data, as the server never looks at this
#endif

//----------------------------------------------------------------------------
// CHARACTER DIGEST DATABASE SAVE REQUEST OBJECT
//----------------------------------------------------------------------------
struct CharacterDigestSaveDBRequest
{
	typedef UNIT InitData;
	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		UINT				PlayerDataSize;
		CHARACTER_INFO		PlayerCharacterInfo;
		UNIQUE_ACCOUNT_ID	PlayerAccountId;
		CHAR				CharacterClassDescription[MAX_CHARACTER_NAME];
		BYTE				PlayerUnitForSelectionData[MAX_DB_FOR_SELECTION_BUFFER];
		BOOL				bValidUnitForSelectionData;		
		PGUID				PlayerGuid;
		SVRID				ServerId;
		WORD				wPlayerLevel;
		WORD				wPlayerClass;
		DWORD				dwPlayerGameVariantFlags;			// see GAME_VARIANT_TYPE
		DWORD				dwPlayerRank;						// STATS_PLAYER_RANK
		DWORD				dwPlayerRankExperience;				// STATS_RANK_EXPERIENCE
		BYTE				bIsDead;		// dead and cannot come back to life (like hardcore dead)
		BYTE				bIsLeagueCharacter;					// set the league_id to the newest league in the database
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * unit )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(unit);

		GAME * game = UnitGetGame(unit);
		ASSERT_RETFALSE(game);

		GAMECLIENTID idClient = UnitGetClientId(unit);
		ASSERT_RETFALSE(idClient != INVALID_ID);

		GAMECLIENT * client = ClientGetById(game, idClient);
		ASSERT_RETFALSE(client);

		//	get name
		if (!unit->szName)
			return FALSE;

		APPCLIENTID idAppClient = ClientGetAppId(client);
		ClientSystemGetCharacterInfo(AppGetClientSystem(), idAppClient, toFill->PlayerCharacterInfo);

		// Get Account ID
		NETCLIENTID64 idNetClient = ClientGetNetId(client);
		NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient); 
		ASSERT(idNetClient == idNetClientFromAppClient);
		if (idNetClient == INVALID_NETCLIENTID64)
			return FALSE;

		toFill->ServerId = ServerId(GAME_SERVER, CurrentSvrGetInstance() );

		toFill->PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), client->m_idAppClient);
		ASSERT(toFill->PlayerAccountId == NetClientGetUniqueAccountId(idNetClient) );
		if (toFill->PlayerAccountId == INVALID_UNIQUE_ACCOUNT_ID)
		{
			return FALSE;
		}

		// Player Level
		toFill->wPlayerLevel = (WORD)UnitGetExperienceLevel( unit );
		toFill->wPlayerClass = (WORD)UnitGetClass( unit );

		// Player Difficulty
		toFill->dwPlayerGameVariantFlags = GameVariantFlagsGetStaticUnit( unit );
		toFill->bIsDead = PlayerIsHardcoreDead(unit);

		// Player Rank and Rank Experience
		toFill->dwPlayerRank = UnitGetPlayerRank(unit);
		toFill->dwPlayerRankExperience = UnitGetRankExperienceValue(unit);

		// Player League
		// TODO: If the player is a league character, this should be set to 1.
		// When bIsLeagueCharacter = 1, sp_save_char_digest populates league_id with the highest value in tbl_leagues.
		// If set to 0, then league_id is set to NULL in tbl_characters.
		toFill->bIsLeagueCharacter = (BYTE)0;

		// Character Class Description
		strncpy_s(toFill->CharacterClassDescription,MAX_CHARACTER_NAME,unit->pUnitData->szName,_TRUNCATE);

		// initialize elements for saving the selection data
		toFill->bValidUnitForSelectionData = TRUE;		
		BIT_BUF buf(toFill->PlayerUnitForSelectionData,MAX_DB_FOR_SELECTION_BUFFER);
		XFER<XFER_SAVE> tXfer(&buf);
		UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec(tXfer);

		// save character selection information
		tSpec.pUnitExisting = unit;
		tSpec.idClient = ClientGetId( client );
		tSpec.eSaveMode = UNITSAVEMODE_DATABASE_FOR_SELECTION;
		GAME *pGame = UnitGetGame(unit);
		if(UnitFileXfer( pGame, tSpec ) != unit)
		{
			ASSERT_MSG("Player unit data for db selection write FAILED!");
			TraceError(
				"!!!!!WARNING!!!!! PlayerUnitForSelectionData write with length %d FAILED with %d size limit!!!!!",
				buf.GetLen(), 
				MAX_DB_FOR_SELECTION_BUFFER);

			toFill->bValidUnitForSelectionData = FALSE;
		}
		TraceGameInfo(
			"PlayerUnitForSelectionData size %d out of %d\n", 
			buf.GetLen(), 
			MAX_DB_FOR_SELECTION_BUFFER);

		//rjd added for tracking
		PGUID guidPlayer = UnitGetGuid(unit);
		toFill->PlayerGuid = guidPlayer;
		//Possibly increment the counter here, but I'll do it one level up

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = reqData->PlayerDataSize;
		UINT PlayerNameLen = PStrLen(reqData->PlayerCharacterInfo.szCharName);
		UINT CharacterClassLen = PStrLen(reqData->CharacterClassDescription);

		hStmt = DatabaseManagerGetStatement(db,"{?=CALL sp_save_char_digest(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}");
		ASSERT_GOTO(hStmt,_END);

		if(reqData->bValidUnitForSelectionData)
		{
			bool bValidCharacterId = false;
			if(SQL_OK(hRet))
			{
				bValidCharacterId = true;
			}
			int dbResult = 0;
			// INSERT or UPDATE character wardrobe digest via stored procedure
			len = MAX_DB_FOR_SELECTION_BUFFER;
			hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &dbResult, (SQLINTEGER)0, NULL);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerAccountId, (SQLINTEGER)0, (SQLLEN *)0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerGuid, (SQLINTEGER)0, (SQLLEN *)0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, PlayerNameLen, 0, reqData->PlayerCharacterInfo.szCharName, MAX_CHARACTER_NAME, NULL);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &reqData->ServerId, 0, 0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			len = MAX_DB_FOR_SELECTION_BUFFER;
			hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, MAX_DB_FOR_SELECTION_BUFFER, 0, reqData->PlayerUnitForSelectionData, MAX_DB_FOR_SELECTION_BUFFER, &len);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->PlayerCharacterInfo.nPlayerSlot), 0, 0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->PlayerCharacterInfo.eSlotType), 0, 0);	
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 9, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->wPlayerLevel), 0, 0);	
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 10, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->wPlayerClass), 0, 0);	
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 11, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, CharacterClassLen, 0, reqData->CharacterClassDescription, MAX_CHARACTER_NAME, NULL);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 12, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->dwPlayerGameVariantFlags, (SQLINTEGER)0, (SQLLEN *)0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 13, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->dwPlayerRank, (SQLINTEGER)0, (SQLLEN *)0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 14, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->dwPlayerRankExperience, (SQLINTEGER)0, (SQLLEN *)0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 15, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &reqData->bIsDead, 0, 0);
			ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLBindParameter(hStmt, 16, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &reqData->bIsLeagueCharacter, 0, 0);
			ASSERT_GOTO(SQL_OK(hRet),_END);

			hRet = SQLExecute(hStmt);
			ASSERT_GOTO(SQL_OK(hRet),_END);

			ASSERT_GOTO(dbResult != 0,_END);
		}
		else
		{
			TraceError("Not writing to tbl_characters.  Wardrobe data is invalid.\n");
		}
		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("Character Digest Save Failed, hRet: %d, Name: %ls, ErrorMsg: %s", hRet, reqData->PlayerCharacterInfo.szCharName, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("CharacterDigestSaveDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("CharacterDigestSaveDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<CharacterDigestSaveDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<CharacterDigestSaveDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharacterDigestSaveDBCallback(
	void * callbackContext,
	CharacterDigestSaveDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	trace("sCharacterDigestSaveDBCallback\n");
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}

//----------------------------------------------------------------------------
// TEMPORARY CLOSED SERVER DATABASE SAVE
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// SAVE DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct PlayerSaveDBRequest
{
	typedef UNIT InitData;
	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		UINT				PlayerDataSize;
		//WCHAR				PlayerName[MAX_CHARACTER_NAME+1];
		CHARACTER_INFO		PlayerCharacterInfo;
		UNIQUE_ACCOUNT_ID	PlayerAccountId;
		BYTE				PlayerUnitForSelectionData[MAX_DB_FOR_SELECTION_BUFFER];
		BOOL				bValidUnitForSelectionData;		
		PGUID				PlayerGuid;
		SVRID				ServerId;
		DATABASE_UNIT_COLLECTION *pDBUCollection;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * unit )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(unit);

		GAME * game = UnitGetGame(unit);
		ASSERT_RETFALSE(game);

		GAMECLIENTID idClient = UnitGetClientId(unit);
		ASSERT_RETFALSE(idClient != INVALID_ID);

		GAMECLIENT * client = ClientGetById(game, idClient);
		ASSERT_RETFALSE(client);

		//	get name
		if (!unit->szName)
			return FALSE;

		APPCLIENTID idAppClient = ClientGetAppId(client);
		//PStrCopy(toFill->PlayerName,MAX_CHARACTER_NAME+1,unit->szName,MAX_CHARACTER_NAME);
		ClientSystemGetCharacterInfo(AppGetClientSystem(), idAppClient, toFill->PlayerCharacterInfo);

		// Get Account ID
		NETCLIENTID64 idNetClient = ClientGetNetId(client);
		NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient); 
		ASSERT(idNetClient == idNetClientFromAppClient);
		if (idNetClient == INVALID_NETCLIENTID64)
			return FALSE;

		toFill->ServerId = ServerId(GAME_SERVER, CurrentSvrGetInstance() );

		toFill->PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), client->m_idAppClient);
		ASSERT(toFill->PlayerAccountId == NetClientGetUniqueAccountId(idNetClient) );
		if (toFill->PlayerAccountId == INVALID_UNIQUE_ACCOUNT_ID)
		{
			return FALSE;
		}

		// initialize elements for saving the selection data
		toFill->bValidUnitForSelectionData = TRUE;		
		BIT_BUF buf(toFill->PlayerUnitForSelectionData,MAX_DB_FOR_SELECTION_BUFFER);
		XFER<XFER_SAVE> tXfer(&buf);
		UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec(tXfer);
	
		// init the db unit collection that we will save units into
		toFill->MemPool = CurrentSvrGetMemPool();		
		toFill->pDBUCollection = (DATABASE_UNIT_COLLECTION *)MALLOCZ( toFill->MemPool, sizeof( DATABASE_UNIT_COLLECTION ) );		
		DATABASE_UNIT_COLLECTION *pDBUCollection = toFill->pDBUCollection;
		ASSERTX_RETFALSE( pDBUCollection, "Unable to allocate db unit collection for saving" );
		WCHAR uszCharacterName[ MAX_CHARACTER_NAME ];
		UnitGetName( unit, uszCharacterName, MAX_CHARACTER_NAME, 0 );
		s_DatabaseUnitCollectionInit( uszCharacterName, pDBUCollection );
	
		// setup storage for a DB unit
		BYTE bDataBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		DATABASE_UNIT tDBUnit;

		// save the entire player and all of their items, and all their items items to the db collection
		int nFatalErrors = 0;
		int nValidationErrors = 0;
		s_DBUCollectionCreateAll( 
			unit, 
			pDBUCollection, 
			&tDBUnit, 
			bDataBuffer, 
			arrsize( bDataBuffer ),
			nFatalErrors,
			nValidationErrors);
		
		// debug information
		for (int i = 0; i < pDBUCollection->nNumUnits; ++i)
		{
			const DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ i ];			
			TraceExtraVerbose( TRACE_FLAG_GAME_NET,
				"#: %d\tSize in Bytes: %d\tGUID: %I64d\tUltOwnerGUID: %I64d\tContUnitGUID: %I64d\tSpecies: %d\tILoc: %d [%d] (%d,%d)\tName: %s\n",
				i,
				pDBUnit->dwDataSize,
				pDBUnit->guidUnit,
				pDBUnit->guidUltimateOwner,
				pDBUnit->guidContainer,
				pDBUnit->spSpecies,
				pDBUnit->tDBUnitInvLoc.dwInvLocationCode,
				pDBUnit->tDBUnitInvLoc.tInvLoc.nInvLocation,
				pDBUnit->tDBUnitInvLoc.tInvLoc.nX,
				pDBUnit->tDBUnitInvLoc.tInvLoc.nY,
				pDBUnit->szName);
		}
		TraceExtraVerbose( TRACE_FLAG_GAME_NET,
			"There were '%d' fatal errors and '%d' validation errors saving the entire unit collection for '%s' [guid=%I64d] [id=%d]",
			nFatalErrors,
			nValidationErrors,
			UnitGetDevName( unit ),
			UnitGetGuid( unit ),
			UnitGetId( unit ));
			
		// save character selection information
		tSpec.pUnitExisting = unit;
		tSpec.idClient = ClientGetId( client );
		tSpec.eSaveMode = UNITSAVEMODE_DATABASE_FOR_SELECTION;
		GAME *pGame = UnitGetGame(unit);
		if(UnitFileXfer( pGame, tSpec ) != unit)
		{
			ASSERT_MSG("Player unit data for db selection write FAILED!");
			TraceError(
				"!!!!!WARNING!!!!! PlayerUnitForSelectionData write with length %d FAILED with %d size limit!!!!!",
				buf.GetLen(), 
				MAX_DB_FOR_SELECTION_BUFFER);

			toFill->bValidUnitForSelectionData = FALSE;
		}
		TraceGameInfo(
			"PlayerUnitForSelectionData size %d out of %d\n", 
			buf.GetLen(), 
			MAX_DB_FOR_SELECTION_BUFFER);

#if TEST_GARBAGE_SAVE_DATA
		RAND & rand = game->rand;
		REF(rand);

#ifdef TEST_GARBAGE_UNITDATA
		{//Arbitrarily overwrite the data buffer in the DATABASE_UNIT_COLLECTION
			float fAlterationProbability = RandGetFloat(rand, 0.0f, 0.1f)*RandGetFloat(rand, 0.0f, 1.0f);

			for(DWORD i = 0; i < pDBUCollection->dwUsedBlobDataSize; i++)
			{
				if(RandGetFloat(rand, 0.0f, 1.0f) < fAlterationProbability) 
					pDBUCollection->bUnitBlobData[i] = (BYTE)RandGetNum(rand, 0, 255);
			}
		}
#endif //TEST_GARBAGE_UNITDATA

#ifdef TEST_GARBAGE_UNITPARAMS
		{//Arbitrarily modify memory in arbitrary DATABASE_UNIT structs
			float fAlterationProbability = RandGetFloat(rand, 0.0f, 1.0f)*RandGetFloat(rand, 0.0f, 1.0f);
			for(int j = 0; j < pDBUCollection->nNumUnits; j++)
				for(DWORD i = 0; i < sizeof(DATABASE_UNIT); i++)
				{
					if(RandGetFloat(rand, 0.0f, 1.0f) < fAlterationProbability) 
						*((BYTE *)(&pDBUCollection->tUnits[j]) + i)= (BYTE)RandGetNum(rand, 0, 255);
				}

		}
#endif //TEST_GARBAGE_UNITPARAMS

#endif //TEST_GARBAGE_SAVE_DATA

		//rjd added for tracking
		PGUID guidPlayer = UnitGetGuid(unit);
		toFill->PlayerGuid = guidPlayer;
		//Possibly increment the counter here, but I'll do it one level up

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
		if(reqData->pDBUCollection)
		{
			FREE( reqData->MemPool, reqData->pDBUCollection );
			reqData->pDBUCollection = NULL;
		}
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData->pDBUCollection,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = reqData->PlayerDataSize;
		UINT PlayerNameLen = PStrLen(reqData->PlayerCharacterInfo.szCharName);

		// Initialize Stale Unit Query String
		std::string expireUnitsQuery = "DELETE FROM tbl_units WHERE account_id = ";

		// Query Database to see if character digest already exists
		hStmt = DatabaseManagerGetStatement(db, "SELECT unit_id FROM tbl_characters WHERE unit_id = ? AND account_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerAccountId, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
			PGUID unitId = 0;
			hRet = SQLBindCol(hStmt, 1, SQL_C_SBIGINT, &unitId, sizeof(unitId), &len);
		ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);
			hRet = SQLFetch(hStmt);
		DatabaseManagerReleaseStatement(hStmt);

		// get db unit collection
		DATABASE_UNIT_COLLECTION *pDBUCollection = reqData->pDBUCollection;

		if(reqData->bValidUnitForSelectionData)
		{
			if(!SQL_OK(hRet))
			{
				// Character Digest Not Found, Insert into tbl_chars
				hStmt = DatabaseManagerGetStatement(db,"{? = CALL sp_save_char_digest (?,?,?,?,?,?,?)}");
				ASSERT_GOTO(hStmt,_END);
				len = MAX_DB_FOR_SELECTION_BUFFER;
				int dbResult = 0;
				hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &dbResult, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerAccountId, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerGuid, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, PlayerNameLen, 0, reqData->PlayerCharacterInfo.szCharName, MAX_CHARACTER_NAME, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &reqData->ServerId, 0, 0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				len = MAX_DB_FOR_SELECTION_BUFFER;
				hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, MAX_DB_FOR_SELECTION_BUFFER, 0, reqData->PlayerUnitForSelectionData, MAX_DB_FOR_SELECTION_BUFFER, &len);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->PlayerCharacterInfo.nPlayerSlot), 0, 0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->PlayerCharacterInfo.eSlotType), 0, 0);	
				//Note: This may fail with different endians.  I assume the SQL looks at this as a byte pointer.
				ASSERT_GOTO(SQL_OK(hRet),_END);

				hRet = SQLExecute(hStmt);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				DatabaseManagerReleaseStatement(hStmt);
			}

			// Begin unit save
					
			// Build Query String for Unit Delete
			char accountIdString[32];
			char characterIdString[32];
			_snprintf_s(accountIdString,32,31,"%I64d", reqData->PlayerAccountId);
			// DISABLED
			// _snprintf_s(characterIdString,32,31,"%I64d", characterId);
			expireUnitsQuery += accountIdString;
			expireUnitsQuery += " and character_id = ";
			expireUnitsQuery += characterIdString;
			expireUnitsQuery += " and unit_id not in (";

			// Loop through DBUCollection and retrieve all unit ids
			for (int ii = 0; ii < pDBUCollection->nNumUnits; ii++) 
			{
				// Add current unit id to query string
				DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ ii ];
				char guidString[32];
				_snprintf_s(guidString,32,31,"%I64d", pDBUnit->guidUnit);
				expireUnitsQuery += guidString;
				if(ii + 1 < pDBUCollection->nNumUnits) {
					expireUnitsQuery += ", ";
				} else {
					expireUnitsQuery += ")";
				}
			}

			// Disable Auto-commit, start save transaction
			SQLSetConnectAttr(db,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);

			hStmt = DatabaseManagerGetStatement(db,"EXEC sp_save_unit ?,?,?,?,?,?,?,?,?,?,?,?,?,?");
			ASSERT_GOTO(hStmt,_END);

			int unitCount   = 0;
			int failedCount = 0;
			for (int ii = 0; ii < pDBUCollection->nNumUnits; ii++) 
			{
				DATABASE_UNIT *pDBUnit = &pDBUCollection->tUnits[ ii ];
					
				len = pDBUnit->dwDataSize;

				// Insert units into tbl_units
				hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerAccountId, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				// character_id is obsolete.  Enter a NULL value for now.
				SQLLEN nullValue = SQL_NULL_DATA;
				hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->guidUnit, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->guidUltimateOwner, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->guidContainer, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->spSpecies, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->tDBUnitInvLoc.dwInvLocationCode, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->tDBUnitInvLoc.tInvLoc.nX, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 9, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->tDBUnitInvLoc.tInvLoc.nY, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 10, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->dwExperience, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 11, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->dwMoney, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 12, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->dwQuantity, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 13, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &pDBUnit->dwSecondsPlayed, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);

				// blob data					
				BYTE *pDataBuffer = s_DBUnitGetDataBuffer( pDBUCollection, pDBUnit );
				hRet = SQLBindParameter(hStmt, 14, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, pDBUnit->dwDataSize, 0, pDataBuffer, pDBUnit->dwDataSize, &len);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLExecute(hStmt);

				if(!SQL_OK(hRet)) {
					// Don't fail on SQLExecute, log error and continue to save other units
					failedCount++;
					TraceError("Failed to Write Unit: GUID: %I64d, Size: %d bytes, Species: %d", pDBUnit->guidUnit, pDBUnit->dwDataSize, pDBUnit->spSpecies);
				}

				unitCount++;
			}

			DatabaseManagerReleaseStatement(hStmt);

			if(failedCount > 0) {
				TraceError("Failed Writing %d Units out of %d", failedCount, unitCount);
			}

			// Only delete units if incremental saving is disabled
			if(s_IncrementalDBUnitsAreEnabled() == FALSE) {
				// Execute query to delete stale units
				TraceExtraVerbose(TRACE_FLAG_GAME_NET, "Deleting all except %d units (%s)", unitCount + 1, expireUnitsQuery.c_str());
				hStmt = DatabaseManagerGetStatement(db,expireUnitsQuery.c_str());
				ASSERT_GOTO(hStmt,_END);
				// SQL_NO_DATA_FOUND is a valid result, so we don't assert here
				// DISABLED
				// hRet = SQLExecute(hStmt);
				DatabaseManagerReleaseStatement(hStmt);
			}

			// Commit Transaction
			hRet = SQLEndTran(SQL_HANDLE_DBC,db,SQL_COMMIT);
			ASSERT_GOTO(SQL_OK(hRet),_END);
		}
		else
		{
			TraceError("Not writing to tbl chars.  Wardrobe data is invalid.\n");
		}
		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("Character Save Failed, hRet: %d, Name: %ls, ErrorMsg: %s", hRet, reqData->PlayerCharacterInfo.szCharName, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
			// End transaction with rollback
			hRet = SQLEndTran(SQL_HANDLE_DBC,db,SQL_ROLLBACK);
		}
		// Enable auto-commit
		SQLSetConnectAttr(db,SQL_ATTR_AUTOCOMMIT,(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
		DatabaseManagerReleaseStatement(hStmt);

		// Clear query string
		expireUnitsQuery.clear();

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("PlayerSaveDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("PlayerSaveDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<PlayerSaveDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<PlayerSaveDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBSaveCallback(
	void * callbackContext,
	PlayerSaveDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}


//----------------------------------------------------------------------------
// UNIT BATCHED ADD TO DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitBatchedAddDBRequest
{
	struct InitData
	{
		UNIT	*					unit;
		DATABASE_UNIT_COLLECTION *	dbUnitCollection;
		BOOL						bNewPlayer;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		DATABASE_UNIT_COLLECTION *	pDBUCollection;
		CHARACTER_INFO		PlayerCharacterInfo;
		UNIQUE_ACCOUNT_ID	PlayerAccountId;
		PGUID				PlayerGuid;
		UNIT				PlayerUnit;
		BOOL				IsNewPlayer;
		// Character Digest Init Data
		UINT				PlayerDataSize;
		BYTE				PlayerUnitForSelectionData[MAX_DB_FOR_SELECTION_BUFFER];
		BOOL				bValidUnitForSelectionData;		
		SVRID				ServerId;
		WORD				wPlayerLevel;
		WORD				wPlayerClass;
		CHAR				CharacterClassDescription[MAX_CHARACTER_NAME];
		DWORD				dwPlayerGameVariantFlags;		// see GAME_VARIANT_TYPE
		DWORD				dwPlayerRank;						// STATS_PLAYER_RANK
		DWORD				dwPlayerRankExperience;				// STATS_RANK_EXPERIENCE
		BOOL				bIsDead;
		BYTE				bIsLeagueCharacter;					// set the league_id to the newest league in the database

	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		UNIT * pPlayer;
		if(!UnitIsPlayer(initData->unit))
		{
			pPlayer = UnitGetUltimateOwner(initData->unit);
		} else {
			pPlayer = initData->unit;
		}
		GAME * game = UnitGetGame(pPlayer);
		ASSERT_RETFALSE(game);

		GAMECLIENTID idClient = UnitGetClientId(pPlayer);
		ASSERT_RETFALSE(idClient != INVALID_ID);

		GAMECLIENT * client = ClientGetById(game, idClient);
		ASSERT_RETFALSE(client);

		APPCLIENTID idAppClient = ClientGetAppId(client);
		ClientSystemGetCharacterInfo(AppGetClientSystem(), idAppClient, toFill->PlayerCharacterInfo);

		// Get Account ID
		NETCLIENTID64 idNetClient = ClientGetNetId(client);
		NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient); 
		ASSERT(idNetClient == idNetClientFromAppClient);
		if (idNetClient == INVALID_NETCLIENTID64)
			return FALSE;

		toFill->PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), client->m_idAppClient);
		ASSERT(toFill->PlayerAccountId == NetClientGetUniqueAccountId(idNetClient) );
		if (toFill->PlayerAccountId == INVALID_UNIQUE_ACCOUNT_ID)
		{
			return FALSE;
		}

		// Copy DBUCollection
		toFill->MemPool = CurrentSvrGetMemPool();
		toFill->pDBUCollection = (DATABASE_UNIT_COLLECTION *)MALLOC( toFill->MemPool, sizeof(DATABASE_UNIT_COLLECTION) );
		ASSERTX_RETFALSE( toFill->pDBUCollection, "Failed to allocate db unit collection for batched unit add request" );		
		MemoryCopy(toFill->pDBUCollection,sizeof(DATABASE_UNIT_COLLECTION),initData->dbUnitCollection,sizeof(DATABASE_UNIT_COLLECTION));

		// Copy Player Unit
		MemoryCopy( &toFill->PlayerUnit, sizeof(UNIT), initData->unit, sizeof(UNIT) );

		// Copy IsNewPlayer
		toFill->IsNewPlayer = initData->bNewPlayer;

		// Copy Character Digest Data
		//----------------------------------------------------------------------------
		toFill->ServerId = ServerId(GAME_SERVER, CurrentSvrGetInstance());
		// Player Level
		toFill->wPlayerLevel = (WORD)UnitGetExperienceLevel( initData->unit );
		toFill->wPlayerClass = (WORD)UnitGetClass( initData->unit );

		// Player Difficulty
		toFill->dwPlayerGameVariantFlags = GameVariantFlagsGetStaticUnit( initData->unit );
		toFill->bIsDead = PlayerIsHardcoreDead(initData->unit);

		// Player Rank and Rank Experience
		toFill->dwPlayerRank = UnitGetPlayerRank(initData->unit);
		toFill->dwPlayerRankExperience = UnitGetRankExperienceValue(initData->unit);

		// Player League
		// TODO: If the player is a league character, this should be set to 1.
		// When bIsLeagueCharacter = 1, sp_save_char_digest populates league_id with the highest value in tbl_leagues.
		// If set to 0, then league_id is set to NULL in tbl_characters.
		toFill->bIsLeagueCharacter = (BYTE)0;

		// Character Class Description
		strncpy_s(toFill->CharacterClassDescription,MAX_CHARACTER_NAME,initData->unit->pUnitData->szName,_TRUNCATE);

		// initialize elements for saving the selection data
		toFill->bValidUnitForSelectionData = TRUE;		
		BIT_BUF buf(toFill->PlayerUnitForSelectionData,MAX_DB_FOR_SELECTION_BUFFER);
		XFER<XFER_SAVE> tXfer(&buf);
		UNIT_FILE_XFER_SPEC<XFER_SAVE> tSpec(tXfer);

		// save character selection information
		tSpec.pUnitExisting = initData->unit;
		tSpec.idClient = ClientGetId( client );
		tSpec.eSaveMode = UNITSAVEMODE_DATABASE_FOR_SELECTION;
		GAME *pGame = UnitGetGame(initData->unit);
		if(UnitFileXfer( pGame, tSpec ) != initData->unit)
		{
			ASSERT_MSG("Player unit data for db selection write FAILED!");
			TraceError(
				"!!!!!WARNING!!!!! PlayerUnitForSelectionData write with length %d FAILED with %d size limit!!!!!",
				buf.GetLen(), 
				MAX_DB_FOR_SELECTION_BUFFER);

			toFill->bValidUnitForSelectionData = FALSE;
		}
		TraceGameInfo(
			"PlayerUnitForSelectionData size %d out of %d\n", 
			buf.GetLen(), 
			MAX_DB_FOR_SELECTION_BUFFER);
		//----------------------------------------------------------------------------

		//rjd added for tracking
		PGUID guidPlayer = UnitGetGuid(pPlayer);
		
		if(game->pServerContext && game->pServerContext->m_DatabaseCounter) 
		{
			game->pServerContext->
				m_DatabaseCounter->Increment(guidPlayer);
			toFill->PlayerGuid = guidPlayer;
		}
		else
		{
			toFill->PlayerGuid = INVALID_GUID;
		}

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
		if(reqData->pDBUCollection)
		{
			FREE( reqData->MemPool,reqData->pDBUCollection );
		}
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len[MAX_BATCHED_OPERATIONS] = {(SQLLEN)reqData->pDBUCollection->tUnits[ 0 ].dwDataSize};
		UINT PlayerNameLen = PStrLen(reqData->PlayerCharacterInfo.szCharName);
		UINT CharacterClassLen = PStrLen(reqData->CharacterClassDescription);

		// Begin batched unit add
		hStmt = DatabaseManagerGetStatement(db,"EXEC sp_batch_unit_add_8 ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?");
		ASSERT_GOTO(hStmt,_END);

		int nBatchCount = 0;
		for(int nOperationCount = 0; nOperationCount < reqData->pDBUCollection->nNumUnits; nOperationCount += MAX_BATCHED_OPERATIONS)
		{
		for(int i = 0; i < MAX_BATCHED_OPERATIONS; i++)
		{
			if ((i + nOperationCount) < reqData->pDBUCollection->nNumUnits)
			{
				int nParamOffset = 13 * i;
				// Bind parameters for each unit in batch
				hRet = SQLBindParameter(hStmt, nParamOffset + 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].guidUnit, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].guidUltimateOwner, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].guidContainer, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 4, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].eGenus, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwClassCode, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].tDBUnitInvLoc.dwInvLocationCode, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 7, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].tDBUnitInvLoc.tInvLoc.nX, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 8, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].tDBUnitInvLoc.tInvLoc.nY, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 9, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwExperience, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 10, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwMoney, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 11, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwQuantity, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 12, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwSecondsPlayed, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				// Blob Data
				BYTE * pDataBuffer = s_DBUnitGetDataBuffer(reqData->pDBUCollection, &reqData->pDBUCollection->tUnits[ i + nOperationCount ]);
				len[ i ] = reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwDataSize;
				hRet = SQLBindParameter(hStmt, nParamOffset + 13, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwDataSize, 0, pDataBuffer, reqData->pDBUCollection->tUnits[ i + nOperationCount ].dwDataSize, &len[ i ]);
				ASSERT_GOTO(SQL_OK(hRet),_END);
			}
			else
			{
				// DBUCollection empty, specify NULL parameter values for rest of batch
				int nParamOffset = 13 * i;
				SQLLEN nullValue = SQL_NULL_DATA;
				// Bind parameters for each unit in batch
				hRet = SQLBindParameter(hStmt, nParamOffset + 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 4, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 7, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 8, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 9, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 10, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 11, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, nParamOffset + 12, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, NULL, (SQLINTEGER)0, &nullValue);
				ASSERT_GOTO(SQL_OK(hRet),_END);

				// Send random data buffer, we won't use it anyway
				BYTE * pDataBuffer;
				int nUnitIndex = 0;
				int nEmptyUnitSize = 1;
				pDataBuffer = s_DBUnitGetDataBuffer(reqData->pDBUCollection, &reqData->pDBUCollection->tUnits[ 0 ]);
				len[ i ] = 1;
				hRet = SQLBindParameter(hStmt, nParamOffset + 13, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, nEmptyUnitSize, 0, pDataBuffer, nEmptyUnitSize, &len[ i ]);
				ASSERT_GOTO(SQL_OK(hRet),_END);
			}
		}
			// Execute Batched Add
			hRet = SQLExecute(hStmt);

			if(!SQL_OK(hRet)) {
				TraceError("Failed to Batched Units for Player GUID: [%I64d], ErrorMsg: %s", reqData->PlayerGuid, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
			}

			nBatchCount++;
		}

		// Save Character Digest if this is a new player
		//----------------------------------------------------------------------------
		if(reqData->IsNewPlayer == TRUE)
		{
			SQLLEN digestLen = reqData->PlayerDataSize;
			UINT PlayerNameLen = PStrLen(reqData->PlayerCharacterInfo.szCharName);

			if(reqData->bValidUnitForSelectionData)
			{
				int dbResult = 0;
				// INSERT or UPDATE character wardrobe digest via stored procedure
				hStmt = DatabaseManagerGetStatement(db,"{?=CALL sp_save_char_digest(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)}");
				ASSERT_GOTO(hStmt,_END);
				digestLen = MAX_DB_FOR_SELECTION_BUFFER;
				hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_OUTPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &dbResult, (SQLINTEGER)0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerAccountId, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerGuid, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, PlayerNameLen, 0, reqData->PlayerCharacterInfo.szCharName, MAX_CHARACTER_NAME, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, 0, 0, &reqData->ServerId, 0, 0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				digestLen = MAX_DB_FOR_SELECTION_BUFFER;
				hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, MAX_DB_FOR_SELECTION_BUFFER, 0, reqData->PlayerUnitForSelectionData, MAX_DB_FOR_SELECTION_BUFFER, &digestLen);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->PlayerCharacterInfo.nPlayerSlot), 0, 0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->PlayerCharacterInfo.eSlotType), 0, 0);	
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 9, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->wPlayerLevel), 0, 0);	
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 10, SQL_PARAM_INPUT, SQL_TINYINT, SQL_TINYINT, 0, 0, &(reqData->wPlayerClass), 0, 0);	
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 11, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, CharacterClassLen, 0, reqData->CharacterClassDescription, MAX_CHARACTER_NAME, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 12, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->dwPlayerGameVariantFlags, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 13, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->dwPlayerRank, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 14, SQL_PARAM_INPUT, SQL_INTEGER, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->dwPlayerRankExperience, (SQLINTEGER)0, (SQLLEN *)0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 15, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &reqData->bIsDead, 0, 0);
				ASSERT_GOTO(SQL_OK(hRet),_END);
				hRet = SQLBindParameter(hStmt, 16, SQL_PARAM_INPUT, SQL_C_BIT, SQL_BIT, 0, 0, &reqData->bIsLeagueCharacter, 0, 0);
				ASSERT_GOTO(SQL_OK(hRet),_END);

				hRet = SQLExecute(hStmt);
				ASSERT_GOTO(SQL_OK(hRet),_END);

				ASSERT_GOTO(dbResult != 0,_END);
			}
			else
			{
				TraceError("Not writing to tbl_chars.  Wardrobe data is invalid.\n");
			}
		}
		//----------------------------------------------------------------------------

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED && hStmt)
		{
			TraceError("UnitBatchedAdd Failed, hRet: %d, Name: %ls, ErrorMsg: %s", hRet, reqData->PlayerCharacterInfo.szCharName, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitBatchedAddDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitBatchedAddDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitBatchedAddDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitBatchedAddDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitBatchedAddDBCallback(
	void * callbackContext,
	UnitBatchedAddDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
	if(!requestSuccess && userRequestType->IsNewPlayer)
	{//If they're a new player and their initial digest save fails, DAS BOOT!
		TraceError("Booting new player %ls due to initial save failure", 
			userRequestType->PlayerCharacterInfo.szCharName);
		GameServerBootAccountId(svr, userRequestType->PlayerAccountId);
	}
}

//----------------------------------------------------------------------------
// UNIT ADD TO DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitAddDBRequest
{
	struct InitData
	{
		UNIT	*			unit;
		DATABASE_UNIT *		dbUnit;
		BYTE *				bUnitData;
		UNIT_DATABASE_CONTEXT eContext;
		
		InitData::InitData( void )
			:	unit( NULL ),
				dbUnit( NULL ),
				bUnitData( NULL ),
				eContext( UDC_NORMAL )
		{ }
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		DATABASE_UNIT 		tDBUnit;
		BYTE 				bDBUnitBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		CHARACTER_INFO		PlayerCharacterInfo;
		UNIQUE_ACCOUNT_ID	PlayerAccountId;
		PGUID				PlayerGuid;
		UNIT_DATABASE_CONTEXT eContext;
		
		DataType::DataType( void )
			:	MemPool( NULL ),
				PlayerAccountId( INVALID_UNIQUE_ACCOUNT_ID ),
				eContext( UDC_NORMAL )
		{ 
			s_DBUnitInit( &tDBUnit );
			bDBUnitBuffer[ 0 ] = 0;
		}
		
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		// copy context
		toFill->eContext = initData->eContext;

		UNIT *pUnit = initData->unit;
		ASSERT_RETFALSE( pUnit );		
		
		// get player
		UNIT *pUltimateOwner = UnitGetUltimateOwner( pUnit );

		// get game		
		GAME * game = UnitGetGame( pUnit );
		ASSERT_RETFALSE(game);

		// player specific logic
		if (pUltimateOwner && UnitIsPlayer( pUltimateOwner ))
		{
			GAMECLIENTID idClient = UnitGetClientId(pUltimateOwner);
			ASSERT_RETFALSE(idClient != INVALID_CLIENTID);

			GAMECLIENT * client = ClientGetById(game, idClient);
			ASSERT_RETFALSE(client);

			APPCLIENTID idAppClient = ClientGetAppId(client);
			ClientSystemGetCharacterInfo(AppGetClientSystem(), idAppClient, toFill->PlayerCharacterInfo);

			// Get Account ID
			NETCLIENTID64 idNetClient = ClientGetNetId(client);
			NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient); 
			ASSERT(idNetClient == idNetClientFromAppClient);
			if (idNetClient == INVALID_NETCLIENTID64)
				return FALSE;

			toFill->PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), client->m_idAppClient);
			ASSERT(toFill->PlayerAccountId == NetClientGetUniqueAccountId(idNetClient) );
			if (toFill->PlayerAccountId == INVALID_UNIQUE_ACCOUNT_ID)
			{
				return FALSE;
			}

		}
		else if (toFill->eContext == UDC_ITEM_RESTORE)
		{		
			// set some defaults for an item restore add to database since 
			// items being resotred don't currently "belong" to anybody
			CHARACTER_INFO *pCharacterInfo = &toFill->PlayerCharacterInfo;
			PStrCopy( pCharacterInfo->szCharName, L"ITEM_RESTORE", MAX_CHARACTER_NAME );
		}
		
		// copy init data into DataType
		MemoryCopy(toFill->bDBUnitBuffer,UNITFILE_MAX_SIZE_ALL_UNITS,initData->bUnitData,initData->dbUnit->dwDataSize);
		MemoryCopy(&toFill->tDBUnit,sizeof(DATABASE_UNIT),initData->dbUnit,sizeof(DATABASE_UNIT));

		TraceExtraVerbose(TRACE_FLAG_GAME_NET, "Initializing Data For Unit: %I64d, Account: %I64d, Character: %ls\n", toFill->tDBUnit.guidUnit, toFill->PlayerAccountId, toFill->PlayerCharacterInfo.szCharName);

		// rjd added for tracking
		PGUID guidPlayer = pUltimateOwner ? UnitGetGuid(pUltimateOwner) : INVALID_GUID;
		if( guidPlayer != INVALID_GUID && 
			game->pServerContext && 
			game->pServerContext->m_DatabaseCounter) 
		{
			game->pServerContext->m_DatabaseCounter->Increment(guidPlayer);
			toFill->PlayerGuid = guidPlayer;
		}
		else
		{
			toFill->PlayerGuid = INVALID_GUID;
		}

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData->bDBUnitBuffer,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = reqData->tDBUnit.dwDataSize;
		UINT PlayerNameLen = PStrLen(reqData->PlayerCharacterInfo.szCharName);

		// Begin unit save

		hStmt = DatabaseManagerGetStatement(db,"INSERT INTO tbl_units (unit_id,ultimate_owner_id,container_id,genus,class_code,location,location_x,location_y,experience,money,quantity,seconds_played,data) VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)");
		ASSERT_GOTO(hStmt,_END);

		len = reqData->tDBUnit.dwDataSize;

		// Insert units into tbl_units
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.guidUnit, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.guidUltimateOwner, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.guidContainer, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.eGenus, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwClassCode, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.tDBUnitInvLoc.dwInvLocationCode, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.tDBUnitInvLoc.tInvLoc.nX, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.tDBUnitInvLoc.tInvLoc.nY, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 9, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwExperience, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 10, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwMoney, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 11, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwQuantity, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 12, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwSecondsPlayed, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		// blob data					
		hRet = SQLBindParameter(hStmt, 13, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, reqData->tDBUnit.dwDataSize, 0, reqData->bDBUnitBuffer, reqData->tDBUnit.dwDataSize, &len);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("UnitAdd Failed [%I64d], Size: %d bytes, Unit Name: %s, hRet: %d, Name: %ls, ErrorMsg: %s", reqData->tDBUnit.guidUnit, reqData->tDBUnit.dwDataSize, reqData->tDBUnit.szName, hRet, reqData->PlayerCharacterInfo.szCharName, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitAddDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitAddDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitAddDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitAddDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitAddDBCallback(
	void * callbackContext,
	UnitAddDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}

//----------------------------------------------------------------------------
// UNIT FULL UPDATE IN DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitFullUpdateDBRequest
{
	struct InitData
	{
		UNIT	*			unit;
		DATABASE_UNIT *		dbUnit;
		BYTE *				bUnitData;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		DATABASE_UNIT 		tDBUnit;
		BYTE 				bDBUnitBuffer[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		CHARACTER_INFO		PlayerCharacterInfo;
		UNIQUE_ACCOUNT_ID	PlayerAccountId;
		PGUID				PlayerGuid;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		UNIT * pPlayer;
		if(!UnitIsPlayer(initData->unit))
		{
			pPlayer = UnitGetUltimateOwner(initData->unit);
		} else {
			pPlayer = initData->unit;
		}
		GAME * game = UnitGetGame(pPlayer);
		ASSERT_RETFALSE(game);

		GAMECLIENTID idClient = UnitGetClientId(pPlayer);
		ASSERT_RETFALSE(idClient != INVALID_ID);

		GAMECLIENT * client = ClientGetById(game, idClient);
		ASSERT_RETFALSE(client);

		APPCLIENTID idAppClient = ClientGetAppId(client);
		ClientSystemGetCharacterInfo(AppGetClientSystem(), idAppClient, toFill->PlayerCharacterInfo);

		// Get Account ID
		NETCLIENTID64 idNetClient = ClientGetNetId(client);
		NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient); 
		ASSERT(idNetClient == idNetClientFromAppClient);
		if (idNetClient == INVALID_NETCLIENTID64)
			return FALSE;

		toFill->PlayerAccountId = ClientGetUniqueAccountId(AppGetClientSystem(), client->m_idAppClient);
		ASSERT(toFill->PlayerAccountId == NetClientGetUniqueAccountId(idNetClient) );
		if (toFill->PlayerAccountId == INVALID_UNIQUE_ACCOUNT_ID)
		{
			return FALSE;
		}

		// copy init data into DataType
		MemoryCopy(toFill->bDBUnitBuffer,UNITFILE_MAX_SIZE_ALL_UNITS,initData->bUnitData,initData->dbUnit->dwDataSize);
		MemoryCopy(&toFill->tDBUnit,sizeof(DATABASE_UNIT),initData->dbUnit,sizeof(DATABASE_UNIT));

		// Determine if item is going into shared stash location
		if (toFill->tDBUnit.tDBUnitInvLoc.tInvLoc.nInvLocation == (int)INVLOC_SHARED_STASH)
		{
			// Set Ultimate Owner ID to Account ID because this item is going into shared stash
			toFill->tDBUnit.guidUltimateOwner = (PGUID)toFill->PlayerAccountId;

			// Set Container Id to Account Id if this item is owned by the player (i.e. not contained by another item)
			if (toFill->tDBUnit.guidContainer == initData->dbUnit->guidUltimateOwner)
			{
				toFill->tDBUnit.guidContainer = (PGUID)toFill->PlayerAccountId;
			}
		}

		TraceExtraVerbose(TRACE_FLAG_GAME_NET, "Initializing Data For Unit: %I64d, Account: %I64d, Character: %ls\n", toFill->tDBUnit.guidUnit, toFill->PlayerAccountId, toFill->PlayerCharacterInfo.szCharName);

		//rjd added for tracking
		PGUID guidPlayer = UnitGetGuid(pPlayer);
		if(game->pServerContext && game->pServerContext->m_DatabaseCounter) 
		{
			game->pServerContext->
				m_DatabaseCounter->Increment(guidPlayer);
			toFill->PlayerGuid = guidPlayer;
		}
		else
		{
			toFill->PlayerGuid = INVALID_GUID;
		}

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData->bDBUnitBuffer,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = reqData->tDBUnit.dwDataSize;
		UINT PlayerNameLen = PStrLen(reqData->PlayerCharacterInfo.szCharName);

		// Begin unit update

		hStmt = DatabaseManagerGetStatement(db,"UPDATE tbl_units SET ultimate_owner_id = ?,container_id = ?,genus = ?,class_code = ?,location = ?,location_x = ?,location_y = ?,experience = ?,money = ?,quantity = ?,seconds_played = ?,data = ? WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);

		len = reqData->tDBUnit.dwDataSize;

		// Insert units into tbl_units
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.guidUltimateOwner, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.guidContainer, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.eGenus, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwClassCode, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.tDBUnitInvLoc.dwInvLocationCode, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.tDBUnitInvLoc.tInvLoc.nX, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_C_TINYINT, SQL_TINYINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.tDBUnitInvLoc.tInvLoc.nY, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwExperience, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 9, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwMoney, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 10, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwQuantity, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 11, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.dwSecondsPlayed, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		// blob data					
		hRet = SQLBindParameter(hStmt, 12, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_LONGVARBINARY, reqData->tDBUnit.dwDataSize, 0, reqData->bDBUnitBuffer, reqData->tDBUnit.dwDataSize, &len);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		// unit_id
		hRet = SQLBindParameter(hStmt, 13, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->tDBUnit.guidUnit, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("UnitFullUpdate Failed [%I64d], Size: %d bytes, Unit Name: %s, hRet: %d, Name: %ls, ErrorMsg: %s", reqData->tDBUnit.guidUnit, reqData->tDBUnit.dwDataSize, reqData->tDBUnit.szName, hRet, reqData->PlayerCharacterInfo.szCharName, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitFullUpdateDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitFullUpdateDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitFullUpdateDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitFullUpdateDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitFullUpdateDBCallback(
	void * callbackContext,
	UnitFullUpdateDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}

//----------------------------------------------------------------------------
// UNIT REMOVE FROM DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitDeleteDBRequest
{
	struct InitData
	{
		PGUID unitId;
		PGUID ownerId;
		const UNIT_DATA *pUnitData;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		PGUID				UnitGuid;
		const UNIT_DATA	  * UnitData;
		PGUID				PlayerGuid;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		// Copy unit guid for delete
		toFill->UnitGuid = initData->unitId;
		toFill->PlayerGuid = initData->ownerId;
		toFill->UnitData = initData->pUnitData;

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = 0;

		// Begin unit delete

		hStmt = DatabaseManagerGetStatement(db,"DELETE FROM tbl_units WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UnitGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);

		if(!SQL_OK(hRet)) 
		{
			toRet = DB_REQUEST_RESULT_FAILED;
		}
		else
		{
			toRet = DB_REQUEST_RESULT_SUCCESS;
		}

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			static int nErrorCount = 0;
					
			// we're only tracing this every so often because it's really spamming the logs		
			int nLogRate = 70;
			if (++nErrorCount % nLogRate == 0)
			{
				TraceError(
					"DB Delete Failed (Reported once every %d failures). GUID=%I64d, OwnerGUID=%I64d, Name=%s, hRet: %d, ErrorMsg: %s", 
					nLogRate,
					reqData->UnitGuid, 
					reqData->PlayerGuid,
					reqData->UnitData ? reqData->UnitData->szName : "UNKNOWN",
					hRet, 
					GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
					
			}
			
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitDeleteDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitDeleteDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitDeleteDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitDeleteDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitDeleteDBCallback(
	void * callbackContext,
	UnitDeleteDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}

//----------------------------------------------------------------------------
// UNIT CHANGE LOCATION DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitChangeLocationDBRequest
{
	struct InitData
	{
		UNIT * unit;
		PGUID unitId;
		PGUID ownerId;
		PGUID containerId;
		INVENTORY_LOCATION * invLoc;
		DWORD invLocationCode;
		BOOL bContainedItemGoingIntoSharedStash;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		PGUID				UltimateOwnerId;
		PGUID				ContainerId;
		PGUID				UnitGuid;
		PGUID				PlayerGuid;
		UNIQUE_ACCOUNT_ID	AccountId;
		INVENTORY_LOCATION  InventoryLocation;
		DWORD				InventoryLocationCode;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);
		ASSERT_RETFALSE(initData->unit);

		// Copy unit guid
		toFill->UnitGuid = initData->unitId;
		toFill->PlayerGuid = initData->ownerId;
		toFill->ContainerId = initData->containerId;
		// Copy invloc
		toFill->InventoryLocationCode = initData->invLocationCode;
		toFill->InventoryLocation.nX = initData->invLoc->nX;
		toFill->InventoryLocation.nY = initData->invLoc->nY;

		// Get game, clientid, and appclientid to retrieve Account ID
		GAME * game = UnitGetGame(initData->unit);
		ASSERT_RETFALSE(game);

		GAMECLIENTID idClient = UnitGetClientId(initData->unit);
		ASSERT_RETFALSE(idClient != INVALID_ID);

		GAMECLIENT * client = ClientGetById(game, idClient);
		ASSERT_RETFALSE(client);

		APPCLIENTID idAppClient = ClientGetAppId(client);

		NETCLIENTID64 idNetClient = ClientGetNetId(client);
		NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(AppGetClientSystem(), idAppClient); 
		ASSERT(idNetClient == idNetClientFromAppClient);
		if (idNetClient == INVALID_NETCLIENTID64)
		{
			return FALSE;
		}

		// Copy account id (for shared stash unit storage)
		toFill->AccountId = ClientGetUniqueAccountId(AppGetClientSystem(), client->m_idAppClient);
		ASSERT(toFill->AccountId == NetClientGetUniqueAccountId(idNetClient));
		if (toFill->AccountId == INVALID_UNIQUE_ACCOUNT_ID)
		{
			return FALSE;
		}

		// Determine if item is going into shared stash location
		// NOTE: This is for containers, not contained items
		if (initData->invLoc->nInvLocation == (int)INVLOC_SHARED_STASH)
		{

			// Set Ultimate Owner ID to Account ID because this item is going into shared stash
			toFill->UltimateOwnerId = (PGUID)toFill->AccountId;

			// Set Container Id to Account Id if this item is owned by the player (i.e. not contained by another item)
			if (initData->containerId == initData->ownerId)
			{
				toFill->ContainerId = toFill->UltimateOwnerId;
			}
		}
		else if (initData->bContainedItemGoingIntoSharedStash == TRUE)
		{
			// If this item is contained by another item in shared stash,
			// we also need to set the ultimate owner to Account Id
			toFill->UltimateOwnerId = (PGUID)toFill->AccountId;
		}
		else
		{
			// Item is not in shared stash, so use the normal ultimate owner
			toFill->UltimateOwnerId = UnitGetUltimateOwnerGuid(initData->unit);
			if (toFill->UltimateOwnerId == INVALID_GUID)
			{
				// Must have a valid owner
				return FALSE;
			}
		}

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = 0;

		// Begin unit change location 

		// Set inventory location of the unit being changed
		hStmt = DatabaseManagerGetStatement(db,"UPDATE tbl_units SET location = ?, location_x = ?, location_y = ?, ultimate_owner_id = ?, container_id = ? WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->InventoryLocationCode, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->InventoryLocation.nX, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->InventoryLocation.nY, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UltimateOwnerId, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->ContainerId, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UnitGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		if (hRet == SQL_SUCCESS || hRet == SQL_SUCCESS_WITH_INFO)
		{
			toRet = DB_REQUEST_RESULT_SUCCESS;
		}

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("Unit Change Location Failed [%I64d] InvLoc: %d (%d,%d), hRet: %d, ErrorMsg: %s", reqData->UnitGuid, reqData->InventoryLocation.nInvLocation, reqData->InventoryLocation.nX, reqData->InventoryLocation.nY, hRet, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitChangeLocationDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitChangeLocationDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitChangeLocationDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitChangeLocationDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitChangeLocationDBCallback(
	void * callbackContext,
	UnitChangeLocationDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}

//----------------------------------------------------------------------------
// UNIT UPDATE EXPERIENCE DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitUpdateExperienceDBRequest
{
	struct InitData
	{
		PGUID unitId;
		DWORD dwExperience;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		PGUID				UnitGuid;
		DWORD				Experience;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		// Copy unit guid
		toFill->UnitGuid = initData->unitId;
		// Copy dwExperience
		toFill->Experience = initData->dwExperience;

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = 0;

		// Begin update experience

		hStmt = DatabaseManagerGetStatement(db,"UPDATE tbl_units SET experience = ? WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->Experience, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UnitGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("UnitUpdateExperience Failed [%I64d], Exp: %d, hRet: %d, ErrorMsg: %s", reqData->UnitGuid, reqData->Experience, hRet, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitUpdateExperienceDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitUpdateExperienceDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitUpdateExperienceDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitUpdateExperienceDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitUpdateExperienceDBCallback(
	void * callbackContext,
	UnitUpdateExperienceDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->UnitGuid);
}

//----------------------------------------------------------------------------
// UNIT UPDATE MONEY DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitUpdateMoneyDBRequest
{
	struct InitData
	{
		PGUID unitId;
		DWORD dwMoney;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		PGUID				UnitGuid;
		DWORD				Money;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		// Copy unit guid
		toFill->UnitGuid = initData->unitId;
		// Copy dwMoney
		toFill->Money = initData->dwMoney;

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = 0;

		// Begin update money 

		hStmt = DatabaseManagerGetStatement(db,"UPDATE tbl_units SET money = ? WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->Money, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UnitGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("UnitUpdateMoney Failed [%I64d], Money: %d, hRet: %d, ErrorMsg: %s", reqData->UnitGuid, reqData->Money, hRet, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitUpdateMoneyDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitUpdateMoneyDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitUpdateMoneyDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitUpdateMoneyDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitUpdateMoneyDBCallback(
	void * callbackContext,
	UnitUpdateMoneyDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->UnitGuid);
}


//----------------------------------------------------------------------------
// UNIT UPDATE QUANTITY DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct UnitUpdateQuantityDBRequest
{
	struct InitData
	{
		PGUID unitId;
		PGUID ownerId;
		WORD  wQuantity;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		PGUID				UnitGuid;
		PGUID				PlayerGuid;
		WORD				Quantity;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		// Copy unit guid
		toFill->UnitGuid = initData->unitId;
		toFill->PlayerGuid = initData->ownerId;
		// Copy wQuantity
		toFill->Quantity = initData->wQuantity;

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = 0;

		// Begin update quantity

		hStmt = DatabaseManagerGetStatement(db,"UPDATE tbl_units SET quantity = ? WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_SHORT, SQL_SMALLINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->Quantity, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UnitGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("UnitUpdateQuantity Failed [%I64d], Quantity: %d, hRet: %d, ErrorMsg: %s", reqData->UnitGuid, reqData->Quantity, hRet, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitUpdateQuantityDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("UnitUpdateQuantityDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<UnitUpdateQuantityDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<UnitUpdateQuantityDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitUpdateQuantityDBCallback(
	void * callbackContext,
	UnitUpdateQuantityDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->PlayerGuid);
}

//----------------------------------------------------------------------------
// CHARACTER UPDATE RANK EXPERIENCE DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------
struct CharacterUpdateRankExperienceDBRequest
{
	struct InitData
	{
		PGUID unitId;
		DWORD dwRankExperience;
		DWORD dwRank;
	};

	struct DataType
	{
		struct MEMORYPOOL * MemPool;
		PGUID				UnitGuid;
		DWORD				RankExperience;
		DWORD				PlayerRank;
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);

		// Copy unit guid
		toFill->UnitGuid = initData->unitId;
		// Copy dwRankExperience
		toFill->RankExperience = initData->dwRankExperience;
		// Copy dwRank
		toFill->PlayerRank = initData->dwRank;

		return TRUE;

	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		ASSERT_RETX(db,DB_REQUEST_RESULT_FAILED);
		ASSERT_RETX(reqData,DB_REQUEST_RESULT_FAILED);

		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt;
		SQLRETURN hRet = SQL_SUCCESS;
		SQLLEN len = 0;

		// Begin update experience

		hStmt = DatabaseManagerGetStatement(db,"UPDATE tbl_characters SET rank_experience = ?, rank = ? WHERE unit_id = ?");
		ASSERT_GOTO(hStmt,_END);
		hRet = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->RankExperience, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->PlayerRank, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, (SQLUINTEGER)0, (SQLSMALLINT)0, &reqData->UnitGuid, (SQLINTEGER)0, (SQLLEN *)0);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);

		toRet = DB_REQUEST_RESULT_SUCCESS;

_END:
		if(toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError("CharacterUpdateRankExperience Failed [%I64d], Exp: %d, hRet: %d, ErrorMsg: %s", reqData->UnitGuid, reqData->RankExperience, hRet, GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}


	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("CharacterUpdateRankExperience does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("CharacterUpdateRankExperience does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<CharacterUpdateRankExperienceDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<CharacterUpdateRankExperienceDBRequest>::s_dat;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharacterUpdateRankExperienceDBCallback(
	void * callbackContext,
	CharacterUpdateRankExperienceDBRequest::DataType * userRequestType,
	BOOL requestSuccess )
{
	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	InterlockedDecrement(&svr->m_outstandingDbRequests);
	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Decrement(userRequestType->UnitGuid);
}

//----------------------------------------------------------------------------
// LOAD DATABASE REQUEST OBJECT
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct PlayerLoadDBRequest
{
	
	struct InitData
	{
		APPCLIENTID				IdAppClient;			// the app client
		const WCHAR *			wszCharName;			// the character name
		PGUID					guidUnitTreeRoot;		// for loading a unit tree
		PGUID					guidDestPlayer;			// for loading a unit tree/collection, the dest player
		PLAYER_LOAD_TYPE		eLoadType;				// the type of load 
		UNIQUE_ACCOUNT_ID		CharacterAccountId;
		ASYNC_REQUEST_ID		idAsyncRequest;			// async request associated with this load (if any)
		
		InitData::InitData( void )
			:	IdAppClient( INVALID_CLIENTID ),
				wszCharName( NULL ),
				guidUnitTreeRoot( INVALID_GUID ),
				guidDestPlayer( INVALID_GUID ),
				eLoadType( PLT_INVALID ),
				CharacterAccountId( INVALID_UNIQUE_ACCOUNT_ID ),
				idAsyncRequest( INVALID_ID )
		{
		}
		
	};
	
	struct DataType
	{
		struct MEMORYPOOL *			MemPool;
		WCHAR						CharacterName[MAX_CHARACTER_NAME+1];
		UNIQUE_ACCOUNT_ID			CharacterAccountId;
		APPCLIENTID					CharacterAppClientId;
		PGUID						PlayerUnitId;
		DATABASE_UNIT_COLLECTION *	pDBUCollection;
		PGUID						guidUnitTreeRoot;		// for loading a unit tree
		PGUID						guidDestPlayer;			// for loading a unit tree, the dest player		
		PLAYER_LOAD_TYPE			eLoadType;				// type of load we're doing		
	};

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static BOOL InitRequestData(
		DataType * toFill,
		InitData * initData )
	{
		ASSERT_RETFALSE(toFill);
		ASSERT_RETFALSE(initData);
		ASSERT_RETFALSE(initData->wszCharName && initData->wszCharName[0]);
		
		// copy the type of load
		toFill->eLoadType = initData->eLoadType;
			
		// copy the guid of the tree root
		toFill->guidUnitTreeRoot = initData->guidUnitTreeRoot;
		
		// record the dest player (for item collections/trees)
		toFill->guidDestPlayer = initData->guidDestPlayer;
				
		//	copy app client id
		toFill->CharacterAppClientId = initData->IdAppClient;

		//	get name
		PStrCopy(toFill->CharacterName,MAX_CHARACTER_NAME+1,initData->wszCharName,MAX_CHARACTER_NAME);

		//	get account id
		if (initData->CharacterAccountId != INVALID_UNIQUE_ACCOUNT_ID) 
		{
			toFill->CharacterAccountId = initData->CharacterAccountId;
		} 
		else if (toFill->eLoadType != PLT_ITEM_RESTORE)
		{
			NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), initData->IdAppClient);
			ASSERT_RETFALSE(idNetClient != INVALID_NETCLIENTID64);
			toFill->CharacterAccountId = NetClientGetUniqueAccountId(idNetClient);
			ASSERT_RETFALSE(toFill->CharacterAccountId != INVALID_UNIQUE_ACCOUNT_ID);
		}

		//	get a destination buffer
		toFill->MemPool = CurrentSvrGetMemPool();

		GameServerContext * pGameContext = (GameServerContext *)CurrentSvrGetContext();
		ASSERT_RETFALSE( pGameContext );

		// Allocate pDBUCollection from freelist
		toFill->pDBUCollection = FREELIST_GET( (pGameContext->m_DatabaseUnitCollectionList[0]), toFill->MemPool);
		ASSERTX_RETFALSE( toFill->pDBUCollection, "Failed to allocate db unit collection for load request" );		
		s_DatabaseUnitCollectionInit( 
			initData->wszCharName, 
			toFill->pDBUCollection, 
			initData->idAsyncRequest);		

		return TRUE;
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static void FreeRequestData(
		DataType * reqData )
	{
		ASSERT_RETURN(reqData);
		if(reqData->pDBUCollection)
		{
			GameServerContext * pGameContext = (GameServerContext *)CurrentSvrGetContext();
			pGameContext->m_DatabaseUnitCollectionList->Free( reqData->MemPool, reqData->pDBUCollection );
		}
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static DB_REQUEST_RESULT DoSingleRequest(
		HDBC db,
		DataType * reqData )
	{
		DB_REQUEST_RESULT toRet = DB_REQUEST_RESULT_FAILED;
		SQLHANDLE hStmt = NULL;
		SQLRETURN hRet = NULL;
		SQLLEN PlayerUnitIdLen = 0;
		SQLLEN bloblen = 0;
		reqData->PlayerUnitId = INVALID_GUID;
				
		// setup a DB unit to read into, the buffer we will use is the local one on the stack
		BYTE bBlobData[ UNITFILE_MAX_SIZE_SINGLE_UNIT ];
		DWORD dwBlobDataSize = 0;
		DATABASE_UNIT tDBUnit;
		const INVLOCIDX_DATA* pInvLoc = InvLocIndexGetData(GlobalIndexGet(GI_INVENTORY_EMAIL_ESCROW));
		SQLINTEGER ignoredInventoryLocation = -1;
		if (pInvLoc)
		{
			ignoredInventoryLocation = pInvLoc->wCode;
		}
		
		SQLUSMALLINT nParam = 1;
		switch (reqData->eLoadType)
		{
		
			//----------------------------------------------------------------------------
			case PLT_CHARACTER:
			{
				hStmt = DatabaseManagerGetStatement(db,"EXEC sp_load_character_ignored_inv_loc ?, ?");
				ASSERT_GOTO(hStmt, _END);				
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, PStrLen(reqData->CharacterName), 0, reqData->CharacterName, MAX_CHARACTER_NAME, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);				
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, sizeof(ignoredInventoryLocation), 0, &ignoredInventoryLocation, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet), _END);
				break;
			}
			
			//----------------------------------------------------------------------------
			case PLT_ITEM_TREE:
			case PLT_AH_ITEMSALE_SEND_TO_AH:
			{
				hStmt = DatabaseManagerGetStatement(db,"EXEC sp_load_character_item_tree ?, ?, ?");
				ASSERT_GOTO(hStmt, _END);				
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(reqData->guidUnitTreeRoot), (SQLSMALLINT)0, &reqData->guidDestPlayer, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);								
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(reqData->guidUnitTreeRoot), (SQLSMALLINT)0, &reqData->guidUnitTreeRoot, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);								
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(reqData->guidUnitTreeRoot), (SQLSMALLINT)0, &reqData->guidUnitTreeRoot, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);								

				// set the player ID to the id of the existing player we're loading for
				reqData->PlayerUnitId = reqData->guidDestPlayer;

				break;
			}
			
			//----------------------------------------------------------------------------		
			case PLT_ACCOUNT_UNITS:
			{
				hStmt = DatabaseManagerGetStatement(db,"EXEC sp_load_character_account_units ?");
				ASSERT_GOTO(hStmt, _END);				
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(reqData->CharacterAccountId), (SQLSMALLINT)0, &reqData->CharacterAccountId, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);		
				
				// set the player ID to the id of the existing player we're loading for
				reqData->PlayerUnitId = reqData->guidDestPlayer;
						
				break;
			}
			
			//----------------------------------------------------------------------------
			case PLT_ITEM_RESTORE:
			{
				hStmt = DatabaseManagerGetStatement(db,"EXEC sp_logs_item_load_item ?, ?");
				ASSERT_GOTO(hStmt, _END);				
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(reqData->guidDestPlayer), (SQLSMALLINT)0, &reqData->guidDestPlayer, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);		
				hRet = SQLBindParameter(hStmt, nParam++, SQL_PARAM_INPUT, SQL_C_SBIGINT, SQL_BIGINT, sizeof(reqData->guidUnitTreeRoot), (SQLSMALLINT)0, &reqData->guidUnitTreeRoot, 0, NULL);
				ASSERT_GOTO(SQL_OK(hRet),_END);		
														
				break;
			
			}
			
		}
		
		// bind output result columns
		ASSERT_GOTO( s_DBUnitBindSQLOutput( 
			reqData->eLoadType, 
			hStmt, 
			tDBUnit, 
			bBlobData, 
			bloblen, 
			&reqData->PlayerUnitId ), _END );
				
		hRet = SQLExecute(hStmt);
		ASSERT_GOTO(SQL_OK(hRet),_END);
		int nRowCount = 0;
		
		while(SQL_OK(SQLFetch(hStmt)))
		{
		
			// resolve all the db unit fields after select
			if (s_DBUnitSQLResultPostProcess( 
					nRowCount,
					reqData->eLoadType, 
					tDBUnit, 
					bloblen,
					(PGUID)reqData->CharacterAccountId,
					reqData->PlayerUnitId ))
			{
					
				// add to db collection
				if (s_DatabaseUnitCollectionAddDBUnit( 
						reqData->pDBUCollection, 
						&tDBUnit, 
						bBlobData, 
						(DWORD)bloblen))
				{
					// if we add one unit, we consider it success enough to continue going forward
					toRet = DB_REQUEST_RESULT_SUCCESS;
				}			

			}			

			// Clear Buffer and get ready for another row result
			bBlobData[ 0 ];
			dwBlobDataSize = 0;
			nRowCount++;
			s_DBUnitInit( &tDBUnit );
			
		}

_END:
		if (toRet == DB_REQUEST_RESULT_FAILED)
		{
			TraceError(
				"Complete Player Load Failed, hRet: %d, Name: %ls, ErrorMsg: %s", 
				hRet, 
				reqData->CharacterName, 
				GetSQLErrorStr(hStmt,SQL_HANDLE_STMT).c_str());
		}

		DatabaseManagerReleaseStatement(hStmt);

		return toRet;
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT PartialBatchSize = 0;
	static DB_REQUEST_RESULT DoPartialBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("PlayerLoadDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	static const UINT FullBatchSize = 0;
	static DB_REQUEST_RESULT DoFullBatchRequest( HDBC, DataType * * )
	{
		ASSERT_MSG("PlayerLoadDBRequest does not support batched requests.");
		return DB_REQUEST_RESULT_FAILED;
	}
	
	static const DWORD MaxBatchWaitTime = INFINITE;
	
};
DB_REQUEST_BATCHER<PlayerLoadDBRequest>::_BATCHER_DATA DB_REQUEST_BATCHER<PlayerLoadDBRequest>::s_dat;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBLoadCallbackCommon(
	PLAYER_LOAD_TYPE eLoadType,
	void * callbackContext,
	PlayerLoadDBRequest::DataType * reqData,
	BOOL requestSuccess )
{
	static UNIT_COLLECTION_ID UNIQUE_COLLECTION_ID = 0;
	MSG_APPCMD_DATABASE_PLAYER_FILE_START loadMsg;	
	MSG_APPCMD_DATABASE_PLAYER_FILE_CHUNK dataMsg;		

	ASSERT_RETURN(callbackContext);
	GameServerContext * svr = (GameServerContext*)callbackContext;
	ASSERT_GOTO(reqData,_LOAD_COMPLETE);
	ASSERT_GOTO(reqData->pDBUCollection,_LOAD_COMPLETE);

	if( !requestSuccess ) 
	{
		if (eLoadType == PLT_CHARACTER)
		{
			s_NetworkJoinGameError( 
				NULL, 
				reqData->CharacterAppClientId, 
				NULL, 
				NULL, 
				LOGINERROR_PLAYERNOTFOUND,
				"Player data not found in database for character '%S' on account '%I64d'",
				reqData->CharacterName,
				reqData->CharacterAccountId);
		}				
		goto _LOAD_COMPLETE;
	}

	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
	if (ClientSystemIsSystemAppClient( reqData->CharacterAppClientId ))
	{
		idNetClient = (NETCLIENTID64)ServiceUserId(USER_SERVER, 0, 0);
	} 
	else 
	{
		idNetClient = ClientGetNetId(AppGetClientSystem(), reqData->CharacterAppClientId);
	}
		
	if( idNetClient == INVALID_NETCLIENTID64 )
	{
		goto _LOAD_COMPLETE;	//	client has since disconnected
	}

	// setup what we'll send to the game
	// TODO: We could put the buffer of blob data at the end of the db unit collection,
	// and do some math to figure out how much of it we want to send so that we aren't
	// sending buffer space that wasn't actually used, but it's a little dangerous
	// on the other end, we'd have to make sure we allocate a whole collection on
	// the other end so we don't have a piece of memory cast to a dbu collection
	// that is really only partially allocated, yuck ... 
	// but it would save some cycles here -cday
	BYTE *pDataToSend = (BYTE *)reqData->pDBUCollection;
	DWORD dwDataSize = sizeof( *reqData->pDBUCollection );
	DWORD dwLeftToSend = dwDataSize;
	
	DWORD dwCRC = 0;  // replace this with a value from a field in the unit table

	// what type of collection do we use for the load requested
	UNIT_COLLECTION_TYPE eCollectionType = UCT_INVALID;
	switch (eLoadType)
	{
	
		//----------------------------------------------------------------------------
		case PLT_CHARACTER:
		{
			eCollectionType = UCT_CHARACTER;
			ASSERTX_GOTO( reqData->guidUnitTreeRoot == INVALID_GUID, "Root unit specified for a character load type that does not need it", _LOAD_COMPLETE );
			break;
		}
			
		//----------------------------------------------------------------------------			
		case PLT_ACCOUNT_UNITS:
		{
			eCollectionType = UCT_ITEM_COLLECTION;
			ASSERTX_GOTO( reqData->guidUnitTreeRoot == INVALID_GUID, "Root unit specified for a item collection type that does not need it", _LOAD_COMPLETE );
			break;
		}
			
		//----------------------------------------------------------------------------
		case PLT_ITEM_TREE:
		case PLT_ITEM_RESTORE:
		case PLT_AH_ITEMSALE_SEND_TO_AH:
		{
			eCollectionType = UCT_ITEM_COLLECTION;  // also an item collection
			ASSERTX_GOTO( reqData->guidUnitTreeRoot != INVALID_GUID, "A root unit is required for loading item trees", _LOAD_COMPLETE );
			break;
		}
		
	}
		
	// send message to app for start of character file
	PStrCopy(loadMsg.szCharName, reqData->CharacterName, MAX_CHARACTER_NAME);
	loadMsg.dwCRC = dwCRC;
	loadMsg.dwSize = dwDataSize;
	loadMsg.nCollectionType = eCollectionType;
	loadMsg.nLoadType = eLoadType;
	loadMsg.dwCollectionID = ++UNIQUE_COLLECTION_ID;	
	loadMsg.guidUnitTreeRoot = reqData->guidUnitTreeRoot;  // used for PLT_ITEM_TREE types
	loadMsg.idAppClient = reqData->CharacterAppClientId;

	// record the dest player for item trees
	loadMsg.guidDestPlayer = reqData->guidDestPlayer;
	if (reqData->eLoadType == PLT_ITEM_TREE ||
		reqData->eLoadType == PLT_AH_ITEMSALE_SEND_TO_AH)
	{
		ASSERTX_GOTO( 
			loadMsg.guidDestPlayer != INVALID_GUID, 
			"Dest player GUID required to load item tree collections",
			_LOAD_COMPLETE);
	}
		
	ASSERT_GOTO(
		//AppPostMessageToMailbox(idNetClient,APPCMD_DATABASE_PLAYER_FILE_START,&loadMsg),
		//We need to pass in the game server context, since we are not in the main thread
		SvrNetPostFakeClientRequestToMailbox(
			svr->m_pPlayerToGameServerMailBox,
			idNetClient,
			&loadMsg,
			APPCMD_DATABASE_PLAYER_FILE_START ),
		_LOAD_COMPLETE );

	// send all chunks of file 	
	while (dwLeftToSend != 0)
	{
		DWORD dwSizeToSend = MIN(dwLeftToSend, (DWORD)MAX_PLAYERSAVE_BUFFER);
		MemoryCopy(dataMsg.buf, MAX_PLAYERSAVE_BUFFER, pDataToSend, dwSizeToSend);
		MSG_SET_BLOB_LEN(dataMsg, buf, dwSizeToSend);
		dataMsg.nCollectionType = eCollectionType;			// pass thru the collection type
		dataMsg.dwCollectionID = loadMsg.dwCollectionID;	// pass thru the collection id
		dataMsg.idAppClient = reqData->CharacterAppClientId;
		ASSERT_GOTO(
			//AppPostMessageToMailbox(idNetClient,CCMD_OPEN_PLAYER_SENDFILE,&dataMsg),
			//We need to pass in the game server context, since we are not in the main thread
			SvrNetPostFakeClientRequestToMailbox(
			svr->m_pPlayerToGameServerMailBox,
			idNetClient,
			&dataMsg,
			APPCMD_DATABASE_PLAYER_FILE_CHUNK),
			_LOAD_COMPLETE );

		pDataToSend += dwSizeToSend;
		dwLeftToSend -= dwSizeToSend;
	}

_LOAD_COMPLETE:
	InterlockedDecrement(&svr->m_outstandingDbRequests);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBLoadItemTreeCallback(
	void * callbackContext,
	PlayerLoadDBRequest::DataType * reqData,
	BOOL requestSuccess )
{
	ASSERT_RETURN(	reqData->eLoadType == PLT_ITEM_TREE ||
					reqData->eLoadType == PLT_AH_ITEMSALE_SEND_TO_AH );

	sDBLoadCallbackCommon( reqData->eLoadType, callbackContext, reqData, requestSuccess );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBLoadItemRestoreCallback(
	void * callbackContext,
	PlayerLoadDBRequest::DataType * reqData,
	BOOL requestSuccess )
{
	sDBLoadCallbackCommon( PLT_ITEM_RESTORE, callbackContext, reqData, requestSuccess );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBLoadAccountUnitsCallback(
	void * callbackContext,
	PlayerLoadDBRequest::DataType * reqData,
	BOOL requestSuccess )
{
	sDBLoadCallbackCommon( PLT_ACCOUNT_UNITS, callbackContext, reqData, requestSuccess );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDBLoadAllCallback(
	void * callbackContext,
	PlayerLoadDBRequest::DataType * reqData,
	BOOL requestSuccess )
{
	sDBLoadCallbackCommon( PLT_CHARACTER, callbackContext, reqData, requestSuccess );
}

#endif	//	ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerLoadAccountUnitsFromDatabase(
	UNIT *pPlayer)
{
#if ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// only subscribers can do this right now
	if (PlayerIsSubscriber( pPlayer ) == FALSE)
	{
		return;
	}
	
	GameServerContext *pServerContext = (GameServerContext*)CurrentSvrGetContext();
	if( pServerContext )
	{

		// get character information
		WCHAR uszName[ MAX_CHARACTER_NAME ];
		UnitGetName( pPlayer, uszName, arrsize( uszName ), 0 );
		APPCLIENTID idAppClient = UnitGetAppClientID( pPlayer );
		PGUID guidPlayer = UnitGetGuid( pPlayer );
				
		// log
		TraceVerbose(
			TRACE_FLAG_GAME_NET,
			"Character '%S' is loading account items from database",
			uszName);

		PlayerLoadDBRequest::InitData reqData;
		reqData.IdAppClient = idAppClient;
		reqData.wszCharName = uszName;
		reqData.eLoadType = PLT_ACCOUNT_UNITS;
		reqData.guidUnitTreeRoot = INVALID_GUID;
		reqData.guidDestPlayer = guidPlayer;
		
		DB_REQUEST_TICKET ticket;
		ASSERT_RETURN(
			DatabaseManagerQueueRequest
				<PlayerLoadDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					pServerContext,
					sDBLoadAccountUnitsCallback,
					FALSE,
					&reqData ) );

		InterlockedIncrement(&pServerContext->m_outstandingDbRequests);

	}
	
#else
	ASSERT_MSG("No database loading allowed in client or open server!");
#endif	//	ISVERSION(SERVER_VERSION)
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
static void sDBLoadItemTreeForAHCallback(
	void * callbackContext,
	PlayerLoadDBRequest::DataType * pData,
	BOOL requestSuccess )
{
	PLAYER_LOAD_TYPE eLoadType = PLT_ITEM_TREE;

	static UNIT_COLLECTION_ID UNIQUE_COLLECTION_ID = 0;
	MSG_APPCMD_DATABASE_PLAYER_FILE_START loadMsg;	
	MSG_APPCMD_DATABASE_PLAYER_FILE_CHUNK dataMsg;		

	GameServerContext* pSvrCtx = (GameServerContext*)callbackContext;
	ASSERT_RETURN(pSvrCtx != NULL);
	ASSERT_GOTO(pData,_LOAD_COMPLETE);
	ASSERT_GOTO(pData->pDBUCollection,_LOAD_COMPLETE);

	if (!requestSuccess) {
		goto _LOAD_COMPLETE;
	}

//	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), pData->CharacterAppClientId);
//	if(idNetClient == INVALID_NETCLIENTID64) {
//		goto _LOAD_COMPLETE;	//	client has since disconnected
//	}

// setup what we'll send to the game
		// TODO: We could put the buffer of blob data at the end of the db unit collection,
		// and do some math to figure out how much of it we want to send so that we aren't
		// sending buffer space that wasn't actually used -cday
		BYTE *pDataToSend = (BYTE *)reqData->pDBUCollection;
		DWORD dwDataSize = sizeof( *reqData->pDBUCollection );
		DWORD dwLeftToSend = dwDataSize;

		DWORD dwCRC = 0;  // replace this with a value from a field in the unit table

		UNIT_COLLECTION_TYPE eCollectionType = UCT_ITEM_COLLECTION;
		ASSERTX_GOTO( reqData->guidUnitTreeRoot == INVALID_GUID, "Root unit specified for a item collection type that does not need it", _LOAD_COMPLETE );



		// send message to app for start of character file
		PStrCopy(loadMsg.szCharName, reqData->CharacterName, MAX_CHARACTER_NAME);
		loadMsg.dwCRC = dwCRC;
		loadMsg.dwSize = dwDataSize;
		loadMsg.nCollectionType = eCollectionType;
		loadMsg.nLoadType = eLoadType;
		loadMsg.dwCollectionID = ++UNIQUE_COLLECTION_ID;	
		loadMsg.guidUnitTreeRoot = reqData->guidUnitTreeRoot;  // used for PLT_ITEM_TREE types

		// record the dest player for item trees
		loadMsg.guidDestPlayer = reqData->guidDestPlayer;
		if (reqData->eLoadType == UCT_ITEM_COLLECTION)
		{
			ASSERTX_GOTO( 
				loadMsg.guidDestPlayer != INVALID_GUID, 
				"Dest player GUID required to load item tree collections",
				_LOAD_COMPLETE);
		}

		ASSERT_GOTO(
			//AppPostMessageToMailbox(idNetClient,APPCMD_DATABASE_PLAYER_FILE_START,&loadMsg),
			//We need to pass in the game server context, since we are not in the main thread
			SvrNetPostFakeClientRequestToMailbox(
			svr->m_pPlayerToGameServerMailBox,
			idNetClient,
			&loadMsg,
			APPCMD_DATABASE_PLAYER_FILE_START ),
			_LOAD_COMPLETE );

		// send all chunks of file 	
		while (dwLeftToSend != 0)
		{
			DWORD dwSizeToSend = MIN(dwLeftToSend, (DWORD)MAX_PLAYERSAVE_BUFFER);
			MemoryCopy(dataMsg.buf, MAX_PLAYERSAVE_BUFFER, pDataToSend, dwSizeToSend);
			MSG_SET_BLOB_LEN(dataMsg, buf, dwSizeToSend);
			dataMsg.nCollectionType = eCollectionType;			// pass thru the collection type
			dataMsg.dwCollectionID = loadMsg.dwCollectionID;	// pass thru the collection id

			ASSERT_GOTO(
				//AppPostMessageToMailbox(idNetClient,CCMD_OPEN_PLAYER_SENDFILE,&dataMsg),
				//We need to pass in the game server context, since we are not in the main thread
				SvrNetPostFakeClientRequestToMailbox(
				svr->m_pPlayerToGameServerMailBox,
				idNetClient,
				&dataMsg,
				APPCMD_DATABASE_PLAYER_FILE_CHUNK),
				_LOAD_COMPLETE );

			pDataToSend += dwSizeToSend;
			dwLeftToSend -= dwSizeToSend;
		}

_LOAD_COMPLETE:
		InterlockedDecrement(&svr->m_outstandingDbRequests);

}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerLoadItemTreeFromDatabaseForAuctionHouse(
	PGUID guidItemRoot)
{
	GameServerContext * pSvrCtx = (GameServerContext*)CurrentSvrGetContext();
	if(pSvrCtx == NULL)
		return;

	PlayerLoadDBRequest::InitData reqData;
	reqData.IdAppClient = GameServerGetAuctionHouseAppClientID(pSvrCtx);
	reqData.wszCharName = AUCTION_OWNER_NAME;
	reqData.eLoadType = PLT_ITEM_TREE;
	reqData.guidUnitTreeRoot = guidItemRoot;
	reqData.guidDestPlayer = guidItemRoot;
	reqData.bItemOnly = TRUE;

	DB_REQUEST_TICKET ticket;
	ASSERT_RETURN(DatabaseManagerQueueRequest<PlayerLoadDBRequest>(
		SvrGetGameDatabaseManager(),
		FALSE,
		ticket,
		pSvrCtx,
		sDBLoadItemTreeForAHCallback,
		FALSE,
		&reqData));

	InterlockedIncrement(&pSvrCtx->m_outstandingDbRequests);
}
*/
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerLoadUnitTreeFromDatabase(
	APPCLIENTID idAppClient,
	PGUID guidDestPlayer,
	const WCHAR *puszCharacter,
	PGUID guidUnitRoot,
	UNIQUE_ACCOUNT_ID idCharAccount,
	PLAYER_LOAD_TYPE eLoadType)
{
#if ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN(idAppClient != INVALID_ID, "Expected app client id" );
	ASSERTX_RETURN(puszCharacter != NULL, "Expected character" );
	ASSERTX_RETURN(guidUnitRoot != INVALID_GUID, "Expected guid for unit root" );
	ASSERTX_RETURN(guidDestPlayer != INVALID_GUID, "Expected guid for dest player" );
		
	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if( !svr )
		return ;	// shutting down...
	
	// log
	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Character '%S' is loading item tree with root '%iI64d' from the database.",
		puszCharacter,
		guidUnitRoot);

	PlayerLoadDBRequest::InitData reqData;
	reqData.IdAppClient = idAppClient;
	reqData.wszCharName = puszCharacter;
	reqData.eLoadType = eLoadType;
	reqData.guidUnitTreeRoot = guidUnitRoot;
	reqData.guidDestPlayer = guidDestPlayer;
	reqData.CharacterAccountId = idCharAccount;

	DB_REQUEST_TICKET ticket;
	ASSERT_RETURN(
		DatabaseManagerQueueRequest
			<PlayerLoadDBRequest>(
				SvrGetGameDatabaseManager(),
				FALSE,
				ticket,
				svr,
				sDBLoadItemTreeCallback,
				FALSE,
				&reqData ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

#else
	ASSERT_MSG("No database loading allowed in client or open server!");
#endif	//	ISVERSION(SERVER_VERSION)

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerRestoreItemFromWarehouseDatabase(
	PGUID guidPlayer,
	PGUID guidItem,
	ASYNC_REQUEST_ID idRequest)
{
	ASSERTX_RETFALSE( guidPlayer, "Expected guid for player" );
	ASSERTX_RETFALSE( guidItem, "Expected guid for item" );
	ASSERTX_RETFALSE( idRequest != INVALID_ID, "Invalid async request id" );

#if ISVERSION(SERVER_VERSION)

	GameServerContext *pServerContext = (GameServerContext*)CurrentSvrGetContext();
	if( !pServerContext )
	{
		return FALSE;	// shutting down...
	}
	
	// log
	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Restoring item '%iI64d' on character '%iI64d' from item warehouse",
		guidItem,
		guidPlayer);

	PlayerLoadDBRequest::InitData reqData;
	reqData.IdAppClient = APPCLIENTID_ITEM_RESTORE_SYSTEM;
	reqData.wszCharName = L"ITEM_RESTORE";
	reqData.eLoadType = PLT_ITEM_RESTORE;
	reqData.guidUnitTreeRoot = guidItem;
	reqData.guidDestPlayer = guidPlayer;
	reqData.CharacterAccountId = INVALID_UNIQUE_ACCOUNT_ID;
	reqData.idAsyncRequest = idRequest;

	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<PlayerLoadDBRequest>(
				SvrGetGameDatabaseManager( DS_LOG_WAREHOUSE ),
				FALSE,
				ticket,
				pServerContext,
				sDBLoadItemRestoreCallback,
				FALSE,
				&reqData ) );

	InterlockedIncrement( &pServerContext->m_outstandingDbRequests );

#endif // ISVERSION(SERVER_VERSION)

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerLoadFromDatabase(
	APPCLIENTID idAppClient,
	WCHAR * wszCharName,
	UNIQUE_ACCOUNT_ID idCharAccount)
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(idAppClient != INVALID_ID);
	ASSERT_RETURN(wszCharName && wszCharName[0]);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if( !svr )
		return ;	// shutting down...

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Loading player character \"%S\" from the database.",
		wszCharName );

	PlayerLoadDBRequest::InitData reqData;
	reqData.IdAppClient = idAppClient;
	reqData.wszCharName = wszCharName;
	reqData.eLoadType = PLT_CHARACTER;
	reqData.guidUnitTreeRoot = INVALID_GUID;
	reqData.CharacterAccountId = idCharAccount;

	DB_REQUEST_TICKET ticket;
	ASSERT_RETURN(
		DatabaseManagerQueueRequest
			<PlayerLoadDBRequest>(
				SvrGetGameDatabaseManager(),
				FALSE,
				ticket,
				svr,
				sDBLoadAllCallback,
				FALSE,
				&reqData ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

#else
	ASSERT_MSG("No database loading allowed in client or open server!");
#endif	//	ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerSaveToDatabase(
	UNIT * unit )
{
#if ISVERSION(SERVER_VERSION)
	// OBSOLETE - Return false - BAN 8/20/2007
	return FALSE;
	/*
	ASSERT_RETFALSE(unit);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Saving player character \"%S\" to the database.",
		unit->szName );

	if(svr->m_DatabaseCounter) 
	{
		svr->m_DatabaseCounter->Increment(UnitGetGuid(unit));
	}

	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<PlayerSaveDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sDBSaveCallback,
				FALSE,
				unit ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
	*/
#else
	ASSERT_MSG("No database saving allowed in client or open server!");
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CharacterDigestSaveToDatabase(
	UNIT * unit )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unit);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	if (AppIsSaving() == FALSE)
	{
		return FALSE;
	}
	
	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Saving character digest for \"%S\" to the database.",
		unit->szName );

	if(svr->m_DatabaseCounter) svr->m_DatabaseCounter->Increment(UnitGetGuid(unit));

	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<CharacterDigestSaveDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sCharacterDigestSaveDBCallback,
				FALSE,
				unit,
				UnitGetGuid(unit) ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	//ASSERT_MSG("No database saving allowed in client or open server!");
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitBatchedAddToDatabase(
	UNIT * unit,
	VOID * pDBUnitCollection,
	BOOL bNewPlayer /* = FALSE */ )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unit);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(TRACE_FLAG_GAME_NET, "Batched Add To Database For Unit: %I64d", UnitGetGuid(unit));

	UnitBatchedAddDBRequest::InitData reqData;
	reqData.dbUnitCollection = (DATABASE_UNIT_COLLECTION *)pDBUnitCollection;
	reqData.unit			 = unit;
	reqData.bNewPlayer       = bNewPlayer;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitBatchedAddDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitBatchedAddDBCallback,
				FALSE,
				&reqData,
				UnitGetGuid(unit) ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitAddToDatabase(
	UNIT * unit,
	VOID * pDBUnit,
	BYTE * pDataBuffer,
	UNIT_DATABASE_CONTEXT eContext /*= UDC_NORMAL*/,	
	DB_GAME_CALLBACK *pCallback /*= NULL*/)
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unit);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;


	TraceVerbose(TRACE_FLAG_GAME_NET, "Saving unit (%I64d) to the database.", UnitGetGuid(unit));

	UnitAddDBRequest::InitData reqData;
	reqData.bUnitData = pDataBuffer;
	reqData.unit	  = unit;
	reqData.dbUnit	  = (DATABASE_UNIT *)pDBUnit;
	reqData.eContext  = eContext;
	
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest<UnitAddDBRequest>(
				SvrGetGameDatabaseManager(),
				FALSE,
				ticket,
				svr,
				sUnitAddDBCallback,
				FALSE,
				&reqData,
				UnitGetUltimateOwnerGuid(unit),
				pCallback) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitRemoveFromDatabase(
	PGUID unitId,
	PGUID ownerId,
	const UNIT_DATA *pUnitData)
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unitId != INVALID_GUID);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceExtraVerbose(
		TRACE_FLAG_GAME_NET,
		"Removing unit (%I64d) from the database.",
		unitId);

	if(svr->m_DatabaseCounter && ownerId != INVALID_GUID) 
	{
		svr->m_DatabaseCounter->Increment(ownerId);
	}

	UnitDeleteDBRequest::InitData reqData;
	reqData.unitId = unitId;
	reqData.ownerId = ownerId;
	reqData.pUnitData = pUnitData;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitDeleteDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitDeleteDBCallback,
				FALSE,
				&reqData,
				ownerId ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitFullUpdateInDatabase(
	UNIT * unit,
	VOID * pDBUnit,
	BYTE * pDataBuffer )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unit);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(TRACE_FLAG_GAME_NET, "Fully Updating unit (%I64d) in the database.", UnitGetGuid(unit));

	UnitFullUpdateDBRequest::InitData reqData;
	reqData.bUnitData = pDataBuffer;
	reqData.unit	  = unit;
	reqData.dbUnit	  = (DATABASE_UNIT *)pDBUnit;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitFullUpdateDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitFullUpdateDBCallback,
				FALSE,
				&reqData,
				UnitGetUltimateOwnerGuid(unit) ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitChangeLocationInDatabase(
    UNIT  * unit,
	PGUID unitId,
	PGUID ownerId,
	PGUID containerId,
	INVENTORY_LOCATION * invLoc,
	DWORD dwInvLocationCode,
	BOOL bContainedItemGoingIntoSharedStash )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unitId != INVALID_GUID);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Changing Unit Location (%I64d) in database.",
		unitId);

	if(svr->m_DatabaseCounter) 
	{
		svr->m_DatabaseCounter->Increment(ownerId);
	}

	UnitChangeLocationDBRequest::InitData reqData;
	reqData.unit = unit;
	reqData.unitId = unitId;
	reqData.ownerId = ownerId;
	reqData.containerId = containerId;
	reqData.invLoc = (INVENTORY_LOCATION *)invLoc;
	reqData.invLocationCode = dwInvLocationCode;
	if (bContainedItemGoingIntoSharedStash == TRUE)
	{
		// The container of this item is in shared stash,
		// so the ultimate owner of this item should be
		// set to the account id
		reqData.bContainedItemGoingIntoSharedStash = true;
	}
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitChangeLocationDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitChangeLocationDBCallback,
				FALSE,
				&reqData,
				ownerId ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitUpdateExperienceInDatabase(
	PGUID unitId,
	PGUID ownerId,
	DWORD dwExperience )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unitId != INVALID_GUID);
	ASSERT_RETFALSE(dwExperience > 0);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Changing Unit Experience (%I64d) in database.",
		unitId);

	if(svr->m_DatabaseCounter) 
	{
		svr->m_DatabaseCounter->Increment(unitId);
	}

	UnitUpdateExperienceDBRequest::InitData reqData;
	reqData.unitId = unitId;
	reqData.dwExperience = dwExperience;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitUpdateExperienceDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitUpdateExperienceDBCallback,
				FALSE,
				&reqData,
				ownerId ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitUpdateMoneyInDatabase(
	PGUID unitId,
	PGUID ownerId,
	DWORD dwMoney )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unitId != INVALID_GUID);
	ASSERT_RETFALSE(dwMoney > 0);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Changing Unit Money (%I64d) in database.",
		unitId);

	if(svr->m_DatabaseCounter) 
	{
		svr->m_DatabaseCounter->Increment(unitId);
	}

	UnitUpdateMoneyDBRequest::InitData reqData;
	reqData.unitId = unitId;
	reqData.dwMoney = dwMoney;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitUpdateMoneyDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitUpdateMoneyDBCallback,
				FALSE,
				&reqData,
				ownerId ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitUpdateQuantityInDatabase(
	PGUID unitId,
	PGUID ownerId,
	WORD wQuantity)
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unitId != INVALID_GUID);
	ASSERT_RETFALSE(wQuantity >= 0);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Changing Unit Quantity (%I64d) in database.",
		unitId);

	if(svr->m_DatabaseCounter) 
	{
		svr->m_DatabaseCounter->Increment(ownerId);
	}

	UnitUpdateQuantityDBRequest::InitData reqData;
	reqData.unitId = unitId;
	reqData.ownerId = ownerId;
	reqData.wQuantity = wQuantity;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<UnitUpdateQuantityDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sUnitUpdateQuantityDBCallback,
				FALSE,
				&reqData,
				ownerId ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CharacterUpdateRankExperienceInDatabase(
	PGUID unitId,
	PGUID ownerId,
	DWORD dwRankExperience,
	DWORD dwRank )
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(unitId != INVALID_GUID);
	ASSERT_RETFALSE(dwRankExperience >= 0);
	ASSERT_RETFALSE(dwRank >= 0);

	GameServerContext * svr = (GameServerContext*)CurrentSvrGetContext();
	if(!svr)
		return FALSE;

	TraceVerbose(
		TRACE_FLAG_GAME_NET,
		"Changing Unit Rank Experience (%I64d) in database.",
		unitId);

	if(svr->m_DatabaseCounter) 
	{
		svr->m_DatabaseCounter->Increment(unitId);
	}

	CharacterUpdateRankExperienceDBRequest::InitData reqData;
	reqData.unitId = unitId;
	reqData.dwRankExperience = dwRankExperience;
	reqData.dwRank = dwRank;
	DB_REQUEST_TICKET ticket;
	ASSERT_RETFALSE(
		DatabaseManagerQueueRequest
			<CharacterUpdateRankExperienceDBRequest>(
					SvrGetGameDatabaseManager(),
					FALSE,
					ticket,
					svr,
					sCharacterUpdateRankExperienceDBCallback,
				FALSE,
				&reqData,
				ownerId ) );

	InterlockedIncrement(&svr->m_outstandingDbRequests);

	return TRUE;
#else
	return FALSE;
#endif // ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelPlayersPostProcess(
	EXCEL_TABLE * table)
{
	ExcelPostProcessUnitDataInit(table, TRUE);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerUpdateAggressiveAttacksTag(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit, 
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(unit, TAG_SELECTOR_AGGRESSIVE_SKILLS_STARTED);
	if (pTag)
	{
		pTag->m_nCounts[pTag->m_nCurrent]++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PLAYER_MUSIC_INFO
{
	int		nMonstersOriginallySpawnedInLevel;
	int		nMonstersLeft;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearMusicInfo(
	UNIT * pPlayer)
{
	ASSERT_RETURN(pPlayer);
	ASSERT_RETURN(IS_CLIENT(pPlayer));

	if(!pPlayer->pMusicInfo)
		return;

	pPlayer->pMusicInfo->nMonstersLeft = 0;
	pPlayer->pMusicInfo->nMonstersOriginallySpawnedInLevel = 0;

	ClearValueTimeTag(pPlayer, TAG_SELECTOR_MONSTERS_KILLED);
	ClearValueTimeTag(pPlayer, TAG_SELECTOR_MONSTERS_KILLED_TEAM);
	ClearValueTimeTag(pPlayer, TAG_SELECTOR_HP_LOST);
	ClearValueTimeTag(pPlayer, TAG_SELECTOR_DAMAGE_DONE);
	ClearValueTimeTag(pPlayer, TAG_SELECTOR_METERS_MOVED);
	ClearValueTimeTag(pPlayer, TAG_SELECTOR_AGGRESSIVE_SKILLS_STARTED);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerUpdateMusicInfo(
	UNIT * pPlayer,
	const MSG_SCMD_MUSIC_INFO * pMsg)
{
	ASSERT_RETURN(pPlayer);
	ASSERT_RETURN(IS_CLIENT(pPlayer));
	ASSERT_RETURN(pMsg);

	if(!pPlayer->pMusicInfo)
		return;

	pPlayer->pMusicInfo->nMonstersLeft = pMsg->nMonstersLeft;
	pPlayer->pMusicInfo->nMonstersOriginallySpawnedInLevel = pMsg->nMonstersSpawned;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_PlayerGetMusicInfoMonsterPercentRemaining(
	UNIT * pPlayer)
{
	ASSERT_RETZERO(pPlayer);
	ASSERT_RETZERO(IS_CLIENT(pPlayer));

	if(!pPlayer->pMusicInfo)
		return 0;

	if(pPlayer->pMusicInfo->nMonstersOriginallySpawnedInLevel <= 0)
		return 0;

	return pPlayer->pMusicInfo->nMonstersLeft * 100 / pPlayer->pMusicInfo->nMonstersOriginallySpawnedInLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerMusicMessageCountOriginallySpawnedMonsters(
	ROOM *pRoom,
	void *pCallbackData )
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pCallbackData);
	int * nMonstersOriginallySpawnedInLevel = (int*)pCallbackData;
	if(RoomTestFlag(pRoom, ROOM_POPULATED_BIT))
	{
		*nMonstersOriginallySpawnedInLevel += pRoom->nMonsterCountOnPopulate[ RMCT_DENSITY_VALUE_OVERRIDE ];
	}
	else
	{
		*nMonstersOriginallySpawnedInLevel += s_RoomGetEstimatedMonsterCount(pRoom);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define PLAYER_MUSIC_MESSAGE_UPDATE_SECONDS 5
static BOOL sPlayerSendMusicMessageToClient(
	GAME * pGame, 
	UNIT * pUnit,
	const EVENT_DATA & event_data)
{
	ASSERT_RETFALSE(IS_SERVER(pGame));

	PLAYER_MUSIC_INFO * pMusicInfo = pUnit->pMusicInfo;
	if(!pMusicInfo)
		return TRUE;

	UnitRegisterEventTimed(pUnit, sPlayerSendMusicMessageToClient, &EVENT_DATA(), PLAYER_MUSIC_MESSAGE_UPDATE_SECONDS * GAME_TICKS_PER_SECOND);

	LEVEL * pLevel = UnitGetLevel(pUnit);
	if(!pLevel)
		return TRUE;

	pMusicInfo->nMonstersOriginallySpawnedInLevel = 0;
	LevelIterateRooms(pLevel, sPlayerMusicMessageCountOriginallySpawnedMonsters, &pMusicInfo->nMonstersOriginallySpawnedInLevel);

	pMusicInfo->nMonstersLeft = s_LevelCountMonstersUseEstimate(pLevel);

	if(UnitHasClient(pUnit))
	{
		MSG_SCMD_MUSIC_INFO tMsg;
		tMsg.nMonstersLeft = pMusicInfo->nMonstersLeft;
		tMsg.nMonstersSpawned = pMusicInfo->nMonstersOriginallySpawnedInLevel;
		s_SendMessage(pGame, UnitGetClientId(pUnit), SCMD_MUSIC_INFO, &tMsg);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * PlayerInit(
	GAME* game,
	PLAYER_SPEC &tSpec)
{
	trace("PlayerInit (%c): (%8.2f, %8.2f, %8.2f)  (%d)\n",
		HOST_CHARACTER(game),
		tSpec.vPosition.fX, tSpec.vPosition.fY, tSpec.vPosition.fZ, tSpec.nAngle);

	const UNIT_DATA * player_data = (UNIT_DATA*)ExcelGetData(game, DATATABLE_PLAYERS, tSpec.nPlayerClass);
	ASSERT_RETNULL(player_data);

	UnitDataLoad( game, DATATABLE_PLAYERS, tSpec.nPlayerClass );

	// a player should *NEVER* have a room when loading them on the server, if we allow a player
	// to have a room before we are ready, the know rooms and network messages will not
	// work correctly.  This is in fact what we're doing, but there was a bug that caused
	// this to happen anyway, so we're putting this there to be extra safe in what we're 
	// doing with player units and their positions/rooms
	ROOM *pRoomToUse = tSpec.pRoom;
	VECTOR vPositionToUse = tSpec.vPosition;
	if (IS_SERVER( game ))
	{
		ASSERTV_DO( tSpec.pRoom == NULL, "SERVER: PlayerInit(), player should not be given a room on initialization" )
		{
			pRoomToUse = NULL;
			vPositionToUse = cgvNone;
		}
	}

	UNIT_CREATE newunit( player_data );
	newunit.species = MAKE_SPECIES(GENUS_PLAYER, tSpec.nPlayerClass);
	newunit.unitid = tSpec.idUnit;
	newunit.guidUnit = tSpec.guidUnit;
	newunit.idClient = tSpec.idClient;
	newunit.pRoom = pRoomToUse;
	newunit.vPosition = vPositionToUse;
	newunit.vFaceDirection = tSpec.vDirection;
	newunit.nAngle = tSpec.nAngle;
	newunit.pWardrobeInit = tSpec.pWardrobeInit;
	newunit.pAppearanceShape = tSpec.pAppearanceShape;

	newunit.fScale = RandGetFloat(game, player_data->fScale, player_data->fScale + player_data->fScaleDelta);

	SET_MASK(newunit.dwUnitCreateFlags, UCF_NO_DATABASE_OPERATIONS);

	if ( TEST_MASK( tSpec.dwFlags, PSF_IS_CONTROL_UNIT ) )
		SET_MASK( newunit.dwUnitCreateFlags, UCF_IS_CONTROL_UNIT );
	
#if ISVERSION(DEBUG_ID)
	PStrPrintf(newunit.szDbgName, MAX_ID_DEBUG_NAME, "%s", player_data->szName);
#endif

	UNIT * unit = UnitInit(game, &newunit);
	if (!unit)
	{
		return NULL;
	}
	UnitInventoryInit(unit);

	// Just make sure that the player has a wardrobe
	ASSERT( UnitGetWardrobe( unit ) != INVALID_ID );

	if (tSpec.nTeam == INVALID_TEAM)
	{
		tSpec.nTeam = UnitGetDefaultTeam(game, UnitGetGenus(unit), UnitGetClass(unit), player_data);
	}
	TeamAddUnit(unit, tSpec.nTeam);

	if(AppIsTugboat())
	{
		//init those points of interest!
		unit->m_pPointsOfInterest = (MYTHOS_POINTSOFINTEREST::cPointsOfInterest *)MEMORYPOOL_NEW( GameGetMemPool( game ) ) MYTHOS_POINTSOFINTEREST::cPointsOfInterest( unit );
		unit->m_pPathing = PathStructInit(game);
		UnitSetFlag(unit, UNITFLAG_PATHING);
		UnitSetFlag(unit, UNITFLAG_ALLOW_NON_NODE_PATHING);
		UnitSetFlag(unit, UNITFLAG_CAN_REQUEST_LEFT_HAND, TRUE);
		UnitSetStat(unit, STATS_LEVELWARP_LAST, INVALID_LINK);
		if( IS_CLIENT( game ) )
		{
			UnitSetFlag(unit, UNITFLAG_ROOMCOLLISION);
		}
	}
	else
	{
		UnitSetFlag(unit, UNITFLAG_ROOMCOLLISION);
		UnitSetStat(unit, STATS_LAST_LEVELID, INVALID_LINK);
	}

	UnitSetFlag(unit, UNITFLAG_COLLIDABLE);
	UnitSetFlag(unit, UNITFLAG_CANPICKUPSTUFF);
	UnitSetFlag(unit, UNITFLAG_CANGAINEXPERIENCE);
	UnitSetOnGround(unit, TRUE);
	UnitSetFlag(unit, UNITFLAG_DESTROY_DEAD_NEVER);
	UnitSetFlag(unit, UNITFLAG_CANAIM);
	UnitSetFlag(unit, UNITFLAG_DONT_PLAY_IDLE);
	UnitSetFlag(unit, UNITFLAG_BOUNCE_OFF_CEILING);

	UnitSetDefaultTargetType(game, unit);

	UnitSetStat(unit, STATS_QUEST_SOUND_ID, INVALID_ID);
	UnitSetStat(unit, STATS_QUEST_CURRENT, -1);

	UnitSetStat(unit, STATS_IS_PLAYER, 1);

	if (AppTestDebugFlag( ADF_COLIN_BIT ))
	{
		int nVelocity = 300;
		UnitSetStat( unit, STATS_VELOCITY, nVelocity );
	}

	if( AppIsHellgate() )
	{
		UnitSetStat(unit, STATS_LUCK_MAX, 10000);
	}
	else
	{
		//set how much damage the player does to the other player by %
		UnitSetStat( unit, STATS_DEFENDERS_VULNERABILITY_ATTACKER, UNITTYPE_PLAYER, PLAYER_VS_PLAYER_DMG_MOD );
		UnitSetStat(unit, STATS_LUCK_MAX, 20000); //mythos screwed up big with luck and now people just have to much. We had to change the max value.
	}

	// this is done on both client and server because the achievements need it at least
	UnitInitValueTimeTag( game, unit, TAG_SELECTOR_MONSTERS_KILLED );

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game))
	{
		if(AppIsTugboat())
		{
			// TRAVIS: Unlike hellgate, we need player pathing info on the client, so we need to init a node here
				// for our path to work with. 
			// This can be an invalid node though, because our char-create screen has no pathing nodes.
			LEVEL *pLevel = UnitGetLevel( unit );
			//		ROOM *pRoom = UnitGetRoom( unit );
			VECTOR vPosition = UnitGetPosition( unit );
	
			// which is our new path node
			if( pLevel )
			{
				ROOM *pRoomDestination = NULL;
				ROOM_PATH_NODE * pPathNode = RoomGetNearestFreePathNode(game, pLevel, vPosition, &pRoomDestination, unit );
	
				if( pRoomDestination && pPathNode && pPathNode->nIndex != INVALID_ID )
				{
					// reserve our new location
					UnitInitPathNodeOccupied( unit, pRoomDestination, pPathNode->nIndex );
					UnitReservePathOccupied( unit );
				}
			}
		}

		UnitInitValueTimeTag( game, unit, TAG_SELECTOR_MONSTERS_KILLED_TEAM );
		UnitInitValueTimeTag( game, unit, TAG_SELECTOR_HP_LOST );
		UnitInitValueTimeTag( game, unit, TAG_SELECTOR_METERS_MOVED );
		UnitInitValueTimeTag( game, unit, TAG_SELECTOR_AGGRESSIVE_SKILLS_STARTED );

		UnitRegisterEventHandler( game, unit, EVENT_AGGRESSIVE_SKILL_STARTED, sPlayerUpdateAggressiveAttacksTag, &EVENT_DATA( ) );

		// add to client
		c_PlayerAdd( game, unit );

	}
#endif //!SERVER_VERSION

	//if (IS_SERVER(game))
	//{
	//	UnitInitValueTimeTag(game, unit, TAG_SELECTOR_DAMAGE_DONE);
	//}
	if ( AppIsHellgate() )
	{
		PlayerMoveMsgInitPlayer( unit );
	}

	// We send a short music info message to the client every 5 seconds or so
	if(IS_SERVER(game))
	{
		UnitRegisterEventTimed(unit, sPlayerSendMusicMessageToClient, &EVENT_DATA(), PLAYER_MUSIC_MESSAGE_UPDATE_SECONDS * GAME_TICKS_PER_SECOND);

		if ( AppIsHellgate() )
		{
			// enable Anti-Fatigue system for Chinese minors
			s_AntiFatigueSystemPlayerInit( game, unit );
		}

		DWORD dwSecond = GameGetTick( game ) / GAME_TICKS_PER_SECOND;
		UnitSetStat( unit, STATS_PREVIOUS_SAVE_TIME_IN_SECONDS, dwSecond );
	}
	unit->pMusicInfo = (PLAYER_MUSIC_INFO*)GMALLOCZ(game, sizeof(PLAYER_MUSIC_INFO));

	UnitPostCreateDo(game, unit);

	return unit;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerGetView(
	GAME * pGame,
	int * pnX, 
	int * pnY)
{
	ASSERT(IS_SERVER(pGame));
	UNIT * unit = GameGetControlUnit(pGame);
	if (!unit)
	{
		*pnX = 0;
		*pnY = 0;
		return;
	}
	*pnX = PrimeFloat2Int(unit->vPosition.fX);
	*pnY = PrimeFloat2Int(unit->vPosition.fY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerSetVelocity(
	float fSpeed)
{
#ifdef _DEBUG
	sgfPlayerVelocity = fSpeed;
#endif
}


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_PlayerGetMovement(
	GAME* game )
{
	return sgClientPlayerGlobals.nMovementKeys;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetMovement(
	GAME* game,
	int movement_flag)
{
	ASSERT(IS_CLIENT(game));

	if (AppGetDetachedCamera())
	{
		AppDetachedCameraSetMovement(movement_flag, TRUE);
		return;
	}

	// For GStar we want players to revert to normal view if they move while
	//   in vanity-cam so no one gets stuck in vanity at the show.
	int nCameraViewType = c_CameraGetViewType();
	if (nCameraViewType == VIEW_VANITY)
	{
		if (!UIComponentGetVisibleByEnum(UICOMP_PAPERDOLL))
		{
			c_CameraRestoreViewType();
			UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);
		}
	}
	UNIT* unit = GameGetControlUnit(game);
	ASSERT_RETURN(unit);

	if(movement_flag == PLAYER_JUMP && UnitHasState(game, unit, STATE_DISABLE_JUMP))
		return;
	if(movement_flag == PLAYER_JUMP && nCameraViewType == VIEW_1STPERSON)
	{
		const UNIT_DATA * pUnitData = UnitGetData(unit);
		if(pUnitData && pUnitData->nFootstepJump1stPerson >= 0 && !(sgClientPlayerGlobals.nMovementKeys & PLAYER_JUMP))
		{
			c_FootstepPlay(pUnitData->nFootstepJump1stPerson, UnitGetPosition(unit));
		}
	}

	sgClientPlayerGlobals.nMovementKeys |= movement_flag;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerTestMovement(
	int movement_flag)
{

	UNIT* unit = GameGetControlUnit(AppGetCltGame());
	ASSERT_RETFALSE(unit);

	return ((sgClientPlayerGlobals.nMovementKeys & movement_flag) != 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearMovement(
	GAME* game,
	int movement_flag)
{
	ASSERT(IS_CLIENT(game));

	if (AppGetDetachedCamera())
	{
		AppDetachedCameraSetMovement(movement_flag, FALSE);
		return;
	}

	sgClientPlayerGlobals.nMovementKeys &= ~movement_flag;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_PlayerToggleMovement(
	GAME * game,
	int movement_flag)
{
	ASSERT(IS_CLIENT(game));

	if (AppGetDetachedCamera())
	{
		return;
	}

	sgClientPlayerGlobals.nMovementKeys ^= movement_flag;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerClearAllMovement(
	GAME* game)
{
	if (!game)
		return;

	ASSERT(IS_CLIENT(game));

	if (AppGetDetachedCamera())
	{
		AppDetachedCameraClearAllMovement();
		return;
	}

	sgClientPlayerGlobals.nMovementKeys = 0;
}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerGetXYZ(
	GAME * pGame,
	VECTOR * pvPosition)
{
	UNIT * unit = GameGetControlUnit(pGame);
	ASSERT(unit);
	if (!unit)
	{
		pvPosition->fX = 0.0f;
		pvPosition->fY = 0.0f;
		pvPosition->fZ = 0.0f;
		return;
	}

	*pvPosition = unit->vPosition;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * c_PlayerGetRoom( 
	GAME * pGame)
{
	ASSERT(IS_CLIENT(pGame));

	UNIT * unit = GameGetControlUnit(pGame);
	if (unit)
	{
		return unit->pRoom;
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sgbPathing = FALSE;
static VECTOR sgvPathDest, sgvPathDirection;
static float sgfPathLastDist = 0.0f;
static float sgfRePathTimer = 0.0f;
static float sgfTurnTimer = 0.0f;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetPlayerFaceDirection(
									GAME * pGame,
									VECTOR & vDirection )
{
	UNIT * pUnit = GameGetControlUnit( pGame );
	if ( pUnit )
	{
		if ( !IsUnitDeadOrDying( pUnit ) ||
			( AppIsTugboat() && UnitTestMode( pUnit, MODE_IDLE_CHARSELECT ) ) )
		{		
			UnitSetFaceDirection( pUnit, vDirection, FALSE, FALSE, FALSE );

#if !ISVERSION(SERVER_VERSION)
			if( AppIsTugboat() && sgfTurnTimer <= 0 )
			{
				c_SendUnitSetFaceDirection( pUnit, UnitGetFaceDirection( pUnit, FALSE ) );
				sgfTurnTimer = .1f;
			}			
#endif
		}
	}

	UNIT * pCameraUnit = GameGetCameraUnit( pGame );
	if ( AppIsHellgate() && pCameraUnit != pUnit && pCameraUnit )
	{
		UnitSetFaceDirection( pCameraUnit, vDirection, TRUE, FALSE, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetAngles(
					   GAME * pGame,
					   float fAngle,
					   float fPitch )  // =0.0f
{
	sgClientPlayerGlobals.fViewAngle = fAngle;

	while ( sgClientPlayerGlobals.fViewAngle < 0.0f )
		sgClientPlayerGlobals.fViewAngle += MAX_RADIAN;
	while ( sgClientPlayerGlobals.fViewAngle >= MAX_RADIAN )
		sgClientPlayerGlobals.fViewAngle -= MAX_RADIAN;

	sgClientPlayerGlobals.fViewAngle = fAngle;
	sgClientPlayerGlobals.fViewPitch = fPitch;

	VECTOR vFace;
	VectorDirectionFromAnglePitchRadians( vFace, sgClientPlayerGlobals.fViewAngle, sgClientPlayerGlobals.fViewPitch );

	sSetPlayerFaceDirection( pGame, vFace );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetAngles(
					   GAME * pGame,
					   int angle,
					   float fPitch )  // =0.0f
{
	c_PlayerSetAngles( pGame, (float) ( ( angle * PI ) / 180.0f ), fPitch );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerGetAngles(
					   GAME * pGame,
					   float * pfAngle,
					   float * pfPitch)
{
	ASSERT_GOTO( pGame && IS_CLIENT(pGame), errout );

	UNIT * unit = GameGetControlUnit(pGame);
	ASSERT_GOTO( unit, errout );

	if ( pfAngle )
		*pfAngle = sgClientPlayerGlobals.fViewAngle;
	if ( pfPitch )
		*pfPitch = sgClientPlayerGlobals.fViewPitch;
	return;

errout:
	if ( pfAngle )
		*pfAngle = 0.0f;
	if ( pfPitch )
		*pfPitch = 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerGetAngles(
	GAME * pGame,
	int * pnAngle,
	int * pnPitch)
{
	float	fAngle;
	float	fPitch;
	c_PlayerGetAngles(pGame, &fAngle, &fPitch);

	if ( pnAngle )
		*pnAngle = (INT) (fAngle * (180.0f / PI)); 
	if ( pnPitch )
		*pnPitch = (INT) (fPitch * (180.0f / PI)); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerSetMovementAndAngle(
	GAME* game,
	GAMECLIENTID idClient,
	const VECTOR & vFaceDirection,
	const VECTOR pvWeaponPos[ MAX_WEAPONS_PER_UNIT ],
	const VECTOR pvWeaponDir[ MAX_WEAPONS_PER_UNIT ],
	const VECTOR & vMoveDir)
{
	UNIT * pUnit = NULL;
	// temporary client-side input process
	if (IS_CLIENT(game))
	{
		pUnit = GameGetControlUnit(game);
	}
	else
	{
		// proper stuff
		GAMECLIENT * client = ClientGetById(game, idClient);
		if (client)
		{
			pUnit = ClientGetControlUnit(client);
		}
	}

	if (pUnit)
	{
		pUnit->vFaceDirection = vFaceDirection;

		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			pUnit->pvWeaponPos[ i ] = pvWeaponPos[ i ];
			pUnit->pvWeaponDir[ i ] = pvWeaponDir[ i ];
		}

		pUnit->vMoveDirection = vMoveDir;
		UnitCalcVelocity( pUnit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPlayerKeyboardRotation( UNIT * pUnit, float fDelta )
{
	if ( IS_CLIENT( pUnit ) )
	{
		c_PlayerSetAngles( UnitGetGame( pUnit ), sgClientPlayerGlobals.fViewAngle + fDelta,
							 sgClientPlayerGlobals.fViewPitch );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ss_DrawPlayerGFX(
	int x, 
	int y,
	int angle)
{
	float fPlayerCollideRadius = 0.6f;
	// draw base
	int radius = PrimeFloat2Int(fPlayerCollideRadius);
	gdi_drawcircle(x, y, radius-0, gdi_color_blue);
	gdi_drawcircle(x, y, radius-1, gdi_color_cyan);
	gdi_drawcircle(x, y, radius-2, gdi_color_blue);

	// draw facing
	int x2 = x + PrimeFloat2Int(gfCos360Tbl[angle] * (fPlayerCollideRadius + 2.0f));
	int y2 = y + PrimeFloat2Int(gfSin360Tbl[angle] * (fPlayerCollideRadius + 2.0f));
	gdi_drawline(x, y, x2, y2, gdi_color_cyan );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DrawPlayer(
	GAME * game,
	int nWorldX,
	int nWorldY)
{
	ASSERT(IS_SERVER(game));

	int nWidth, nHeight;
	gdi_getscreensize(&nWidth, &nHeight);
	int nScreenX = nWorldX - (nWidth / 2);
	int nScreenY = nWorldY - (nHeight / 2);

	for (GAMECLIENT *pClient = ClientGetFirstInGame( game ); 
		 pClient; 
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *unit = ClientGetControlUnit( pClient );
		ASSERT( unit );
		int x = PrimeFloat2Int(unit->vPosition.fX) - nScreenX;
		int y = PrimeFloat2Int(unit->vPosition.fY) - nScreenY;
		ss_DrawPlayerGFX(x, y, UnitGetPlayerAngle( unit ));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerResetCameraOrbit( 
	GAME * game )
{
	if ( AppIsTugboat() && c_CameraGetViewType() == VIEW_3RDPERSON )
	{
		c_CameraResetFollow( );
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerMouseRotate( 
	GAME * game,
	float delta)
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! game )
		return;
	ASSERT(IS_CLIENT(game));
	if (delta == 0)
	{
		return;
	}

#ifdef _DEBUG
	if (AppGetDetachedCamera())
	{
		AppDetachedCameraMouseRotate((int)delta);
		return;
	}
#endif

	if ( c_CameraGetViewType() == VIEW_VANITY )
	{
		c_CameraVanityRotate( delta * ROTATION_PITCH_ANGLE_DELTA );
		return;
	}
	else if ( AppIsTugboat() && AppGetState() == APP_STATE_IN_GAME && InputGetAdvancedCamera() )
	{
		c_CameraFollowRotate( delta * ROTATION_PITCH_ANGLE_DELTA );
		return;
	}
	
	if ( c_CameraGetViewType() == VIEW_DEMOLEVEL )
	{
		return;
	}

	if ( c_CameraGetViewType() == VIEW_FOLLOWPATH )
	{
		return;
	}

	if ( c_CameraGetViewType() == VIEW_RTS )
	{
		return;
	}

	UNIT * unit = GameGetControlUnit(game);
	if ( !unit )
		return;

	c_TutorialInputOk( unit, TUTORIAL_INPUT_MOUSE_MOVE );

	float fDelta = (float)delta * ROTATION_PITCH_ANGLE_DELTA;
	c_PlayerSetAngles(game, sgClientPlayerGlobals.fViewAngle + fDelta, sgClientPlayerGlobals.fViewPitch);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerMousePitch(
	GAME* game,
	float delta)
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! game )
		return;
	ASSERT(IS_CLIENT(game));
	if (delta == 0)
	{
		return;
	}

#ifdef _DEBUG
	if (AppGetDetachedCamera())
	{
		AppDetachedCameraMousePitch((int)delta);
		return;
	}
#endif


	if ( c_CameraGetViewType() == VIEW_VANITY )
	{
		c_CameraVanityPitch( (float)delta );
		return;
	}
	else if ( AppIsTugboat() && AppGetState() == APP_STATE_IN_GAME && InputGetAdvancedCamera() && !InputGetLockedPitch() )
	{
		c_CameraFollowPitch( (float)delta );
		return;
	}
	if ( c_CameraGetViewType() == VIEW_DEMOLEVEL )
	{
		return;
	}

	if ( c_CameraGetViewType() == VIEW_FOLLOWPATH )
	{
		return;
	}

	if ( c_CameraGetViewType() == VIEW_RTS )
	{
		return;
	}

	UNIT* unit = GameGetControlUnit(game);
	if ( ! unit )
		return;

	c_TutorialInputOk( unit, TUTORIAL_INPUT_MOUSE_MOVE );

	float fDelta = (float)delta * (PI / (180.f * 6.0f));
	sgClientPlayerGlobals.fViewPitch += fDelta;
	sgClientPlayerGlobals.fViewPitch = NORMALIZE_EX(sgClientPlayerGlobals.fViewPitch, MAX_RADIAN);

	int nPitchDownLimit = unit == GameGetCameraUnit( game ) ? MAX_PLAYER_PITCH : MAX_PLAYER_PITCH_WITH_CANNON;

	if (sgClientPlayerGlobals.fViewPitch > PI && sgClientPlayerGlobals.fViewPitch < (((360 - nPitchDownLimit) * PI) / 180.0f))
	{
		sgClientPlayerGlobals.fViewPitch = ((360 - nPitchDownLimit) * PI) / 180.0f;
	}
	else if (sgClientPlayerGlobals.fViewPitch < PI && sgClientPlayerGlobals.fViewPitch > ((MAX_PLAYER_PITCH * PI) / 180.0f))
	{
		sgClientPlayerGlobals.fViewPitch = (MAX_PLAYER_PITCH * PI) / 180.0f;
	}

	int pitch = PrimeFloat2Int((sgClientPlayerGlobals.fViewPitch * 180.0f) / PI);
	pitch = NORMALIZE_EX(pitch, 360);

	// pin pitch to an arc between 315-0-45
	if (pitch > 180 && pitch < 360 - nPitchDownLimit)	// "negative" pitch
	{
		pitch = 360 - nPitchDownLimit;
		sgClientPlayerGlobals.fViewPitch = (FLOAT) pitch * (PI / 180.0f);
	}
	else if (pitch < 180 && pitch > MAX_PLAYER_PITCH)	// positive pitch
	{
		pitch = MAX_PLAYER_PITCH;
		sgClientPlayerGlobals.fViewPitch = (FLOAT) pitch * (PI / 180.0f);
	}
#endif
}


//----------------------------------------------------------------------------
// all client-side only player effects, etc go here
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerUpdateClientFX( GAME *pGame )
{

	ASSERT(IS_CLIENT(pGame));
	UNIT * pUnit = GameGetControlUnit( pGame );

	if ( !pUnit || ( pUnit->fVelocity == 0.0f ) )
	{
		sgClientPlayerGlobals.nBobCount = 0;
		sgClientPlayerGlobals.fBob = 0.0f;
		sgClientPlayerGlobals.fBobTime = 0.0f;
		return;
	}

	if ( sgClientPlayerGlobals.fBobTime == 0.0f )
		sgClientPlayerGlobals.fBobTime = GameGetTimeFloat(pGame);

	float fTimeDelta = GameGetTimeFloat(pGame) - sgClientPlayerGlobals.fBobTime;
	if ( fTimeDelta >= GAME_TICK_TIME )
	{
		sgClientPlayerGlobals.fBobTime += GAME_TICK_TIME;
		sgClientPlayerGlobals.nBobCount += (int)FLOOR( BOB_RATE_PER_GAME_TICK );
		if ( sgClientPlayerGlobals.nBobCount >= 360 )
			sgClientPlayerGlobals.nBobCount -= 360;
		fTimeDelta -= GAME_TICK_TIME;
	}
	float fBobTime = (float)ROUND((BOB_RATE_PER_SECOND * fTimeDelta));
	int nBobAngle = sgClientPlayerGlobals.nBobCount + (int)fBobTime;
	if ( nBobAngle > 360 )
		nBobAngle = 360;
	sgClientPlayerGlobals.fBob = gfSin360Tbl[ nBobAngle ] * BOB_AMPLITUDE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameGetControlPosition(
	GAME * pGame,
	VECTOR * pvPosition,
	float * pfAngle,
	float * pfPitch)
{
	UNIT * unit = GameGetControlUnit(pGame);
	if (!unit)
	{
		return FALSE;
	}
	if (pvPosition)
	{
		*pvPosition = unit->vPosition;
	}
	if (pfAngle)
	{
		if( AppIsHellgate() )
		{
			*pfAngle = sgClientPlayerGlobals.fViewAngle;
		}
		else
		{
			*pfAngle = 0;
		}
	}
	if (pfPitch)
	{
		*pfPitch = sgClientPlayerGlobals.fViewPitch;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerSetExperienceMarkersByLevel(
	UNIT *pPlayer,
	int nExperienceLevel)
{
	ASSERTV_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ));
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pPlayerData = UnitGetData( pPlayer );
	ASSERT_RETURN(pPlayerData);
	const PLAYERLEVEL_DATA *pPlayerLevelData = PlayerLevelGetData( pGame, pPlayerData->nUnitTypeForLeveling, nExperienceLevel );
	if (!pPlayerLevelData)
	{
		UnitSetStat( pPlayer, STATS_EXPERIENCE_NEXT, 0);
	}
	else
	{
		// save the experience need to achieve the next level
		UnitSetStat( pPlayer, STATS_EXPERIENCE_NEXT, pPlayerLevelData->nExperience );
		
		// save the experience needed to active this level
		int nExperiencePrev = 0;
		int nExperienceLevelPrev = nExperienceLevel - 1;
		if (nExperienceLevel >= 1)
		{
			const PLAYERLEVEL_DATA *pPlayerLevelDataPrev = PlayerLevelGetData( pGame, pPlayerData->nUnitTypeForLeveling, nExperienceLevelPrev );
			if (pPlayerLevelDataPrev)
			{
				nExperiencePrev = pPlayerLevelDataPrev->nExperience;
			}
		}
		UnitSetStat( pPlayer, STATS_EXPERIENCE_PREV, nExperiencePrev );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerSetExperienceMarkersByRank(
	UNIT *pPlayer,
	int nRankExperienceLevel)
{
	ASSERTV_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ));
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pPlayerData = UnitGetData( pPlayer );
	ASSERT_RETURN(pPlayerData);
	const PLAYERRANK_DATA *pPlayerRankData = PlayerRankGetData( pGame, pPlayerData->nUnitTypeForLeveling, nRankExperienceLevel );
	if (!pPlayerRankData)
	{
		UnitSetStat( pPlayer, STATS_RANK_EXPERIENCE_NEXT, 0);
	}
	else
	{

		// save the experience need to achieve the next level
		UnitSetStat( pPlayer, STATS_RANK_EXPERIENCE_NEXT, pPlayerRankData->nRankExperience );

		// save the experience needed to active this level
		int nRankExperiencePrev = 0;
		int nRankExperienceLevelPrev = nRankExperienceLevel - 1;
		if (nRankExperienceLevel >= 1)
		{
			const PLAYERRANK_DATA *pPlayerRankDataPrev = PlayerRankGetData( pGame, pPlayerData->nUnitTypeForLeveling, nRankExperienceLevelPrev );
			if (pPlayerRankDataPrev)
			{
				nRankExperiencePrev = pPlayerRankDataPrev->nRankExperience;
			}
		}
		UnitSetStat( pPlayer, STATS_RANK_EXPERIENCE_PREV, nRankExperiencePrev );

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerDoLevelUp(
	UNIT * unit,
	int nNewLevel)
{
	ASSERT(unit);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitIsA(unit, UNITTYPE_PLAYER))
	{
		return FALSE;
	}
	GAME * game = UnitGetGame(unit);
	ASSERT(game && IS_SERVER(game));

	int nClass = UnitGetClass(unit);
	const UNIT_DATA * player_data = PlayerGetData(game, nClass);
	if (!player_data)
	{
		return FALSE;
	}

	int nCurLevel = UnitGetExperienceLevel(unit);
	if (nCurLevel >= nNewLevel)
	{
		return FALSE;
	}

	while (nCurLevel < nNewLevel)
	{
		const PLAYERLEVEL_DATA * nextlevel_data = PlayerLevelGetData(game, player_data->nUnitTypeForLeveling, nCurLevel + 1);
		if (!nextlevel_data)
		{
			return FALSE;
		}

		// add stats
		UnitChangeStatShift(unit, STATS_HP_MAX, nextlevel_data->nHp);
		UnitChangeStatShift(unit, STATS_ATTACK_RATING, nextlevel_data->nAttackRating);
		UnitSetStatShift(unit, STATS_SFX_DEFENSE_BONUS, nextlevel_data->nSfxDefenseRating);
		UnitChangeStatShift(unit, STATS_STAT_POINTS, nextlevel_data->nStatPoints);
		if(AppIsTugboat())
		{
			UnitChangeStatShift( unit, STATS_POWER_MAX, nextlevel_data->nMana );  //Marsh Added
		}

		if (nCurLevel > 0)
		{
			if(AppIsTugboat())
			{
				PlayerGiveSkillPoint(unit, nextlevel_data->nSkillPoints); 
				PlayerGiveCraftingPoint(unit, nextlevel_data->nCraftingPoints);
			}
			else
			{
				PlayerGiveSkillPoint(unit, 1); 
			}
			
		}
		for (int ii = 0; ii < CLASSLEVEL_PROPERTIES; ii++)
		{
			int code_len = 0;
			BYTE * code_ptr = ExcelGetScriptCode(game, DATATABLE_PLAYERLEVELS, nextlevel_data->codeProperty[ii], &code_len);
			if (code_ptr)
			{
				VMExecI(game, unit, code_ptr, code_len);
			}
		}
		
		// add experience & level
		if (UnitGetExperienceValue(unit) < nextlevel_data->nExperience)
		{
			UnitSetFlag(unit, UNITFLAG_DONT_CHECK_LEVELUP);
			UnitSetStat(unit, STATS_EXPERIENCE, nextlevel_data->nExperience);
			UnitClearFlag(unit, UNITFLAG_DONT_CHECK_LEVELUP);
		}
		nCurLevel++;
		UnitSetExperienceLevel(unit, nCurLevel);
		UnitTriggerEvent( game, EVENT_LEVEL_UP, unit, NULL, NULL );

		// full heal and mana =)
		if (IsUnitDeadOrDying( unit ) == FALSE)
		{
			s_UnitRestoreVitals( unit, FALSE );
		}
	}

	int nMaxLevel = UnitGetMaxLevel( unit );

	if ( nCurLevel >= nMaxLevel )
	{
		UnitSetStat(unit, STATS_EXPERIENCE_NEXT, 0);
		sPlayerSetExperienceMarkersByRank( unit, 1 );
	}
	else
	{
		sPlayerSetExperienceMarkersByLevel( unit, nCurLevel + 1 );
	}
	
	if (player_data->m_nLevelUpState > INVALID_LINK && nNewLevel > 1 )
	{
		const STATE_DATA* state_data = StateGetData(game, player_data->m_nLevelUpState);
		if (state_data)
		{
			s_StateSet(unit, unit, player_data->m_nLevelUpState, state_data->nDefaultDuration * GAME_TICKS_PER_SECOND / MSECS_PER_SEC);
		}
	}

	//This is added for when the player levels up quest NPC's will have the icons update.
	if( AppIsTugboat() )
	{
		QuestUpdateWorld( game, unit, UnitGetLevel( unit ), NULL, QUEST_WORLD_UPDATE_QSTATE_BEFORE, NULL ); //updates the state of the NPC ( to show the question mark above the head )
	}
	else if ( AppIsHellgate() && IS_SERVER( UnitGetGame( unit ) ) )
	{
		s_QuestsUpdateAvailability( unit );
	}

	// inform the achievements system
	s_AchievementsSendPlayerLevelUp(unit);

	// KCK: Check if they player has reached level 20 and set some nifty badges if so.
	// Don't really like the idea of hardcoding this, but I'm not sure this falls under the achievement system
	int nLevel = UnitGetStat(unit, STATS_LEVEL);
	if (nLevel >= 20)
	{
		GAMECLIENT *pClient = UnitGetClient( unit );
		GameClientAddAccomplishmentBadge( pClient, ACCT_ACCOMPLISHMENT_CAN_PLAY_ELITE_MODE );
		// KCK: Hardcore has the additional requirement of being a subscriber, but this is being checked
		// on the request side anyway currently. Otherwise we'd have to clear this if the player drops his
		// subscription.
		GameClientAddAccomplishmentBadge( pClient, ACCT_ACCOMPLISHMENT_CAN_PLAY_HARDCORE_MODE );
	}

	if( AppIsTugboat() )
	{
		UnitCalculatePVPGearDrop( unit );
	}

	SVR_VERSION_ONLY(s_UnitTrackLevelUp( unit ) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerDoRankUp(
	UNIT * unit,
	int nNewRank)
{
	ASSERT(unit);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitIsA(unit, UNITTYPE_PLAYER))
	{
		return FALSE;
	}
	GAME * game = UnitGetGame(unit);
	ASSERT(game && IS_SERVER(game));

	int nClass = UnitGetClass(unit);
	const UNIT_DATA * player_data = PlayerGetData(game, nClass);
	if (!player_data)
	{
		return FALSE;
	}

	int nCurRank = UnitGetPlayerRank(unit);
	if (nCurRank >= nNewRank)
	{
		return FALSE;
	}

	while (nCurRank < nNewRank)
	{
		const PLAYERRANK_DATA * nextlevel_data = PlayerRankGetData(game, player_data->nUnitTypeForLeveling, nCurRank + 1);
		if (!nextlevel_data)
		{
			return FALSE;
		}

		// add stats
		UnitChangeStatShift(unit, STATS_PERK_POINTS, nextlevel_data->nPerkPoints);

		// add experience & level
		if (UnitGetRankExperienceValue(unit) < nextlevel_data->nRankExperience)
		{
			UnitSetFlag(unit, UNITFLAG_DONT_CHECK_LEVELUP);
			UnitSetStat(unit, STATS_RANK_EXPERIENCE, nextlevel_data->nRankExperience);
			UnitClearFlag(unit, UNITFLAG_DONT_CHECK_LEVELUP);
		}
		nCurRank++;
		UnitSetPlayerRank(unit, nCurRank);
		UnitTriggerEvent( game, EVENT_RANK_UP, unit, NULL, NULL );

		// full heal and mana =)
		if (IsUnitDeadOrDying( unit ) == FALSE)
		{
			s_UnitRestoreVitals( unit, FALSE );
		}
	}

	int nMaxRank = UnitGetMaxRank( unit );

	if ( nCurRank >= nMaxRank )
	{
		UnitSetStat(unit, STATS_RANK_EXPERIENCE_NEXT, 0);
	}
	else
	{
		sPlayerSetExperienceMarkersByRank( unit, nCurRank + 1 );
	}

	if (player_data->m_nRankUpState > INVALID_LINK && nNewRank >= 1 )
	{
		const STATE_DATA* state_data = StateGetData(game, player_data->m_nRankUpState);
		if (state_data)
		{
			s_StateSet(unit, unit, player_data->m_nRankUpState, state_data->nDefaultDuration * GAME_TICKS_PER_SECOND / MSECS_PER_SEC);
		}
	}

	// inform the achievements system
	s_AchievementsSendPlayerRankUp(unit);

	s_UnitTrackLevelUp( unit );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerCheckLevelUp(
	UNIT* unit)
{
	ASSERT(unit);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitIsA(unit, UNITTYPE_PLAYER))
	{
		return FALSE;
	}
	GAME* game = UnitGetGame(unit);
	ASSERT(game && IS_SERVER(game));

	int nClass = UnitGetClass(unit);
	const UNIT_DATA * player_data = PlayerGetData(game, nClass);
	if (!player_data)
	{
		return FALSE;
	}

	int nExperience = UnitGetExperienceValue(unit);

	int nLevel = UnitGetExperienceLevel(unit);
	int nMaxLevel = UnitGetMaxLevel( unit );
	if (nLevel >= nMaxLevel)
	{
		return FALSE;
	}

	int nNextLevel = nLevel;

	const PLAYERLEVEL_DATA * nextlevel_data = PlayerLevelGetData(game, player_data->nUnitTypeForLeveling, nLevel + 1);
	if (!nextlevel_data)
	{
		return FALSE;
	}

	while (nNextLevel < nMaxLevel &&
		nExperience >= nextlevel_data->nExperience)
	{
		nNextLevel++;
		nextlevel_data = PlayerLevelGetData(game, player_data->nUnitTypeForLeveling, nNextLevel + 1);
		if (!nextlevel_data)
		{
			break;
		}
	}

	if (nNextLevel <= nLevel)
	{
		return FALSE;
	}

	return PlayerDoLevelUp(unit, nNextLevel);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerCheckRankUp(
	UNIT* unit)
{
	ASSERT(unit);
	if (!unit)
	{
		return FALSE;
	}
	if (!UnitIsA(unit, UNITTYPE_PLAYER))
	{
		return FALSE;
	}
	GAME* game = UnitGetGame(unit);
	ASSERT(game && IS_SERVER(game));

	int nClass = UnitGetClass(unit);
	const UNIT_DATA * player_data = PlayerGetData(game, nClass);
	if (!player_data)
	{
		return FALSE;
	}

	int nExperience = UnitGetRankExperienceValue(unit);

	int nRank = UnitGetPlayerRank(unit);
	int nMaxRank = UnitGetMaxRank(unit);
	if (nRank >= nMaxRank)
	{
		return FALSE;
	}

	int nNextRank = nRank;

	const PLAYERRANK_DATA * nextlevel_data = PlayerRankGetData(game, player_data->nUnitTypeForLeveling, nRank + 1);
	if (!nextlevel_data)
	{
		return FALSE;
	}

	while (nNextRank < nMaxRank &&
		nExperience >= nextlevel_data->nRankExperience)
	{
		nNextRank++;
		nextlevel_data = PlayerRankGetData(game, player_data->nUnitTypeForLeveling, nNextRank + 1);
		if (!nextlevel_data)
		{
			break;
		}
	}

	if (nNextRank <= nRank)
	{
		return FALSE;
	}

	return PlayerDoRankUp(unit, nNextRank);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerGiveSkillPoint(
	UNIT *pPlayer,
	int nNumSkillPoints)
{
	if (pPlayer && nNumSkillPoints > 0)
	{
		UnitChangeStatShift( pPlayer, STATS_SKILL_POINTS, nNumSkillPoints );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerGivePerkPoint(
	UNIT *pPlayer,
	int nNumPerkPoints)
{
	if (pPlayer && nNumPerkPoints > 0)
	{
		UnitChangeStatShift( pPlayer, STATS_PERK_POINTS, nNumPerkPoints );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerGiveCraftingPoint(
						  UNIT *pPlayer,
						  int nNumCraftingPoints)
{
	if (pPlayer && nNumCraftingPoints > 0)
	{
		UnitChangeStatShift( pPlayer, STATS_CRAFTING_POINTS, nNumCraftingPoints );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerGiveStatPoints(
	UNIT * pPlayer,
	int nNumStatPoints )
{
	if ( pPlayer && nNumStatPoints > 0 )
	{
		UnitChangeStatShift( pPlayer, STATS_STAT_POINTS, nNumStatPoints );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerToggleUIElement(
	UNIT *pPlayer,
	UI_ELEMENT eElement,
	UI_ELEMENT_ACTION eAction)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// setup message
	MSG_SCMD_TOGGLE_UI_ELEMENT tMessage;
	tMessage.nElement = eElement;
	tMessage.nAction = eAction;
	
	// send
	GAME *pGame = UnitGetGame( pPlayer );	
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TOGGLE_UI_ELEMENT, &tMessage);
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerShowWorldMap(
	UNIT *pPlayer,
	EMAP_ATLAS_MESSAGE Result,
	int nArea )
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	
	// setup message
	MSG_SCMD_SHOW_MAP_ATLAS tMessage;
	tMessage.nArea = nArea;
	tMessage.nResult = Result;
	
	// send
	GAME *pGame = UnitGetGame( pPlayer );	
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_SHOW_MAP_ATLAS, &tMessage);
	
}


void s_PlayerShowRecipePane( UNIT *pPlayer,						     
							int nRecipeID )
{
	ASSERTX_RETURN( pPlayer, "Expected player" );

	// setup message
	MSG_SCMD_SHOW_RECIPE_PANE tMessage;
	tMessage.nRecipeID = nRecipeID;	

	// send
	GAME *pGame = UnitGetGame( pPlayer );	
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_SHOW_RECIPE_PANE, &tMessage);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void PlayerRotationInterpolating( GAME *pGame, float fTimeDelta )
{
	ASSERT( IS_CLIENT( pGame ) );
	if ( fTimeDelta <= 0.0f )
	{
		return;
	}

#if HELLGATE_ONLY
	UNIT *pUnit = GameGetControlUnit( pGame );
	if ( pUnit->pHavokRigidBody )
	{
		return;
	}
#endif

	if ( !sgClientPlayerGlobals.fRotationalVelocity )
	{
		return;
	}

	float fTickDelta = fTimeDelta * sgClientPlayerGlobals.fRotationalVelocity;

	c_PlayerSetAngles( pGame, sgClientPlayerGlobals.fViewAngle + fTickDelta,
		sgClientPlayerGlobals.fViewPitch );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DRB_3RD_PERSON

void PlayerFaceTarget( GAME * game, UNIT * target )
{
	ASSERT( IS_CLIENT( game ) );
	if ( !target )
	{
		return;
	}

	UNIT * player = GameGetControlUnit( game );
	if ( !player )
	{
		return;
	}

	float fNewAngle = VectorToAngle( UnitGetPosition( player ), UnitGetPosition( target ) );
	c_PlayerSetAngles( game, fNewAngle, sgClientPlayerGlobals.fViewPitch );
}

#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sPlayerFirstTimeEnterWorld( 
	UNIT* pPlayer)
{	
	// mark stat that we've been in the world now
	UnitSetStat( pPlayer, STATS_ENTERED_WORLD, TRUE );

	UnitClearStatsRange( pPlayer, STATS_SKILL_COOLING );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_UnitPutInLimbo(
	UNIT* unit,
	BOOL bSafeOnExit)
{
	ASSERT_RETFALSE(unit);

	trace( "Unit '%s' [%d] entering limbo\n", UnitGetDevName( unit ), UnitGetId( unit ));

	if( UnitPathIsActive(unit) )
	{
		UnitEndPathReserve( unit );
	}
	if( AppIsTugboat() )
	{
		ClearAggressionList( unit );
	}
	GAME * pGame = UnitGetGame(unit);
	if( UnitGetRoom( unit ) )
	{

		// clear any movement modes, otherwise things walk in place and it looks fugly!
		if (UnitIsInLimbo( unit) == FALSE )
		{
			UnitClearStepModes( unit );
		}

		// take them off the step list
		UnitStepListRemove( pGame, unit, SLRF_FORCE );	

		// clear path nodes that we were occupying
		if(unit->m_pPathing)
		{
			ROOM * pTestRoom = NULL;
			UnitGetPathNodeOccupied(unit, &pTestRoom, NULL);
			if( pTestRoom )
			{
				UnitClearBothPathNodes( unit );
			}
		}

		// clear any path stuff they had before

		//PathingUnitWarped( unit );
	}

	if(unit->m_pPathing)
	{
		UnitErasePathNodeOccupied(unit);
	}

	UnitSetFlag(unit, UNITFLAG_IN_LIMBO);

	s_StateSet( unit, unit, STATE_PLAYER_LIMBO, 0 );

	UnitSetStat(unit, STATS_LAST_TRIGGER, INVALID_LINK );

	UnitTriggerEvent(UnitGetGame(unit), EVENT_ENTER_LIMBO, unit, NULL, NULL);

	SkillStopAllRequests(unit);

	// I sure hope that we really need to clear all of these... there could be some worth saving?
	UnitEventEnterLimbo(UnitGetGame(unit), unit);

	s_UnitBreathLevelChange(unit);

	// set safe on exit
	UnitSetFlag(unit, UNITFLAG_SAFE_ON_LIMBO_EXIT, bSafeOnExit);

	return TRUE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sStopPortalSafety(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	// clear flag
	UnitClearFlag( pUnit, UNITFLAG_RETURN_FROM_PORTAL );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_UnitTakeOutOfLimbo(
	UNIT* unit)
{
	if (UnitIsInLimbo( unit ) == FALSE)
	{
		return TRUE;
	}

	trace( "Unit '%s' [%d] exiting limbo\n", UnitGetDevName( unit ), UnitGetId( unit ));

	GAME * pGame = UnitGetGame( unit );

	UnitEventExitLimbo( pGame, unit );
	UnitClearFlag( unit, UNITFLAG_IN_LIMBO );
	s_StateClear( unit, UnitGetId(unit), STATE_PLAYER_LIMBO, 0 );

	UnitTriggerEvent( pGame, EVENT_EXIT_LIMBO, unit, NULL, NULL );
	StatsExitLimboCallback( pGame, unit );
	UnitSetStat(unit, STATS_MOVIE_TO_PLAY_AFTER_LEVEL_LOAD, INVALID_ID);


	// going to do some stuff here so that we can set it up to keep respawns from happening in our returning sphere of influence
	if (UnitTestFlag( unit, UNITFLAG_RETURN_FROM_PORTAL ))
	{
		UnitRegisterEventTimed( unit, sStopPortalSafety, NULL, 20 );						
	}

	// "safety" them if requested
	if (UnitTestFlag( unit, UNITFLAG_SAFE_ON_LIMBO_EXIT ))
	{

		// clear flag
		UnitClearFlag( unit, UNITFLAG_SAFE_ON_LIMBO_EXIT );
			
		// "safety" them		
		UnitSetSafe( unit );
		
	}

	// hellgate, you cannot enter warps for a short amount of time after you exit limbo,
	// this is a safeguard against infinite level warping loops due to data errors (yah, that sux)
	// Right now, this just looks horribly broken in Mythos - so we're skipping it for the moment.
	if( AppIsHellgate() )
	{
		s_StateSet( unit, unit, STATE_NO_TRIGGER_WARPS, 0 );
	}
		
	UNIT* pItem = NULL;
	while ((pItem = UnitInventoryIterate(unit, pItem)) != NULL)		// might want to narrow this iterate down if we can
	{

		if ( UnitIsA( pItem, UNITTYPE_HIRELING ) ||
			   UnitIsA( pItem, UNITTYPE_PET ) )
		{			
			if (IsUnitDeadOrDying( pItem ) || UnitGetOwnerUnit( pItem ) != unit)
			{
				continue;
			}	
			if( UnitGetStat( pItem, STATS_PET_OWNER_UNITID ) == (int)UnitGetId( unit ) )
			{
				continue;
			}
			UnitClearFlag(pItem, UNITFLAG_JUSTCREATED);			
			//UnitInventoryRemove( pItem, ITEM_ALL );

			ROOM *room = UnitGetRoom( unit );
			UnitSetPosition( pItem, UnitGetPosition( unit ) );
			UnitUpdateRoom( pItem, room, TRUE);			
			PathingUnitWarped( pItem );
			UnitSetStat( pItem, STATS_PET_OWNER_UNITID, INVALID_ID );
			int nItemLocation;
			int nItemLocationX;
			int nItemLocationY;
			if (UnitGetInventoryLocation(pItem, &nItemLocation, &nItemLocationX, &nItemLocationY))
			{
				PetAdd( pItem, unit, nItemLocation, INVALID_ID, 0, FALSE );
			}
			// Temporary hack to heal pets after level changes,
			// since they are starting at level 1 health each time! Yikes!
			if( AppIsTugboat() )
			{
				s_UnitRestoreVitals(
					pItem,
					FALSE );
				GAMECLIENT* pClient = UnitGetClient( unit );

				// send inventory we can know about
				s_SendInventory( pClient, pItem );
			}
		}
	}
	if( UnitIsA( unit, UNITTYPE_PLAYER ) )
	{
	
		// start the party validation (to catch stray people getting into game with no parties)
		// note that we have to delay this a small amount of time open entering a game because
		// we have to give the party server a chance to update this game with new party info
		// yeah, it's ugly and not perfect, but it's the best i can do with the time left -cday
		s_PlayerPartyValidation(unit, GAME_TICKS_PER_SECOND * 10);

		// start the auto party matchmaker
		s_PartyMatchmakerEnable( unit, AUTO_PARTY_UPDATE_DELAY_IN_TICKS );

		if (UnitHasState( pGame, unit, STATE_REMOVE_FROM_PARTY_ON_EXIT_LIMBO ))
		{
			s_PlayerRemoveFromParty( unit );
			s_StateClear( unit, UnitGetId( unit ), STATE_REMOVE_FROM_PARTY_ON_EXIT_LIMBO, 0 );
		}
		// update chat server with player info
		UpdateChatServerPlayerInfo(unit);
		
		// if not in an instance level, clear any reserved game
		if (UnitIsInInstanceLevel( unit ) == TRUE)
		{
			GAMECLIENT *pGameClient = UnitGetClient( unit );
			APPCLIENTID idAppClient = ClientGetAppId( pGameClient );
			ClientSystemClearReservedGame( AppGetClientSystem(), idAppClient, pGameClient );
			ClientSystemSetLevelGoingTo( AppGetClientSystem(), idAppClient, INVALID_ID, NULL );
		}

		// now that a player is in the world and ready to interact, we want all clients
		// to re-evaluate their known rooms because they might now get a chance to know
		// about this new player and the room they are in
		s_LevelQueuePlayerRoomUpdate( UnitGetLevel( unit ), unit );

#if !ISVERSION(SERVER_VERSION)
		//UIPartyDisplayOnPartyChange();
#endif

	}

	// TRAVIS: this is a temporary hack until Peter finishes with the stat overhaul.
	// this will make sure that any skills that start on select will be OK on load on the client
	// TYLER: it looks like this "hack" is old.  I've changed it to use SkillSelectAll, but I'm not sure why Tugboat needs this and Hellgate doesn't - Travis?  Marsh?
	if( AppIsTugboat() &&
		!UnitTestFlag(unit, UNITFLAG_IN_LIMBO) )
	{
		s_SkillsSelectAll( pGame, unit );
	}
	if (UnitIsPlayer(unit))
	{
		if( AppIsTugboat() )
		{
			CRAFTING::s_CraftingPlayerRegisterMods( unit );	
		}
		s_AchievementsInitialize( unit );  //this might not be the best place

		s_DuelEventPlayerExitLimbo( pGame, unit );
		s_MetagameEventExitLimbo( unit );

		if( AppIsTugboat() )
		{
			QuestPlayerComesOutOfLimbo( pGame, unit );
			ONCE
			{
				LEVEL* pLevel = UnitGetLevel( unit );
				ASSERTX_BREAK(pLevel, "Unit has no level on limbo exit!");
				if( pLevel->m_nDepth == 0 &&
					!pLevel->m_bVisited )
				{
					pLevel->m_bVisited = TRUE;
				}
				int nLevelAreaID = LevelGetLevelAreaID( pLevel );
				UNIT* pItem = NULL;	
				while ( ( pItem = UnitInventoryIterate( unit, pItem ) ) != NULL)
				{		
					// if we were the creator of this level, and we have arrived -
					// and this is a temporary map - remove it from our atlas!
					// this needs much better/more interesting rules, but for now...				
					if( UnitIsA( pItem,  UNITTYPE_MAP_KNOWN ) && 
						!UnitIsA( pItem, UNITTYPE_MAP_KNOWN_PERMANENT ) )
					{			
						int ItemLevelAreaID = MYTHOS_MAPS::GetLevelAreaID( pItem );
						if( nLevelAreaID == ItemLevelAreaID ||
							ItemLevelAreaID == INVALID_ID )
						{
							UnitFree( pItem, UFF_SEND_TO_CLIENTS );
							s_PlayerShowWorldMap( unit, MAP_ATLAS_MSG_MAPREMOVED, LevelGetLevelAreaID( pLevel ) );
							if( ItemLevelAreaID != INVALID_ID )
							{
								break;
							}
						}
					}
				}
			}
		}
	}

	// even though we hope this will never happen, if we got items stuck in our 
	// trade inventory we will attempt to put those items back in the our inventory,
	// in the super error condition that they won't fit we will drop them to the ground
	// and lock them to this player
	s_ItemsReturnToStandardLocations( unit, TRUE );

	// TRAVIS: We need to do this post limbo exit, because passive skills that give us stat bonuses
	// have just kicked in, and we need to reverify any equipment that may have been disabled.
	if( AppIsTugboat() )
	{
		s_LevelSendPointsOfInterest( UnitGetLevel( unit ), unit );
		s_InventoryChanged(unit);

		// because in playerwarplevels THIS DOESN'T ACTUALLY HAPPEN BECAUSE IT CAN'T SEND ANY INFO! Good grief.
		DWORD dwUnitWarpFlags = 0;	
		SETBIT( dwUnitWarpFlags, UW_FORCE_VISIBLE_BIT );
		SETBIT( dwUnitWarpFlags, UW_LEVEL_CHANGING_BIT );
		SETBIT( dwUnitWarpFlags, UW_TRIGGER_EVENT_BIT );
		UnitWarp( 
			unit, 
			UnitGetRoom( unit ), 
			UnitGetPosition( unit ), 
			UnitGetFaceDirection( unit, FALSE ), 
			VECTOR( 0, 0, 1 ), 
			dwUnitWarpFlags, 
			TRUE, 
			TRUE,
			UnitGetLevelID( unit ) );

		

	}
	return TRUE;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sPetClearBadStates(
	UNIT *pPet,
	void *pUserData)
{
	s_StateClearBadStates( pPet );
}
#endif

#if !ISVERSION( CLIENT_ONLY_VERSION )
inline void sRemoveBindedLevelItems( UNIT *pUnit,
									 LEVEL *pNewLevel )
{
	ASSERT_RETURN( pUnit );	
	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );		
	int nLevelAreaID = INVALID_ID;
	if( pNewLevel )
		nLevelAreaID = LevelGetLevelAreaID( pNewLevel );
	UNIT * pItem = NULL;		
	while ((pItem = UnitInventoryIterate( pUnit, pItem, dwInvIterateFlags )) != NULL)
	{
		int nBindedToLevelArea = ItemGetBindedLevelArea( pItem );
		if( nBindedToLevelArea != INVALID_ID &&
			nBindedToLevelArea != nLevelAreaID )
		{
			UnitFree( pItem, UFF_SEND_TO_CLIENTS );
			pItem = NULL;
		}
		
	}	
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_PlayerWarpLevels(
	UNIT * unit,
	LEVELID idLevelDest,
	LEVELID idLevelLeaving,
	WARP_SPEC &tWarpSpec)	
{
	ASSERTX_RETFALSE( unit, "Expected unit" );
	ASSERTV_RETFALSE( UnitIsPlayer( unit ), "Expected player, got '%s'", UnitGetDevName( unit ));	
	GAME *pGame = UnitGetGame(unit);

	// the player should always be alive to warp levels
	if (IsUnitDeadOrDying( unit ) == TRUE)
	{
	
		// log error
		ASSERTV( 
			IsUnitDeadOrDying( unit ) == FALSE, 
			"Unit '%s' trying to warp levels, but is dead or dying", 
			UnitGetDevName( unit ));
			
		// restore them to life, it's far better to open up a possible exploit to come back
		// to life by somehow warping on death than it is to allow a player to remain dead somewhere
		// with no means of coming back to life
		VITALS_SPEC tVitalsSpec;
		VitalsSpecZero( tVitalsSpec );
		tVitalsSpec.nHealthAbsolute = 1;
		s_UnitRestoreVitals( unit, FALSE, &tVitalsSpec );
		
	}
		
	LEVEL* pLevelOld = UnitGetLevel(unit);
	DWORD dwUnitWarpFlags = 0;	
	SETBIT( dwUnitWarpFlags, UW_FORCE_VISIBLE_BIT );
	SETBIT( dwUnitWarpFlags, UW_LEVEL_CHANGING_BIT );
	SETBIT( dwUnitWarpFlags, UW_TRIGGER_EVENT_BIT );
	
	VECTOR vPosition;
	VECTOR vFaceDirection = UnitGetFaceDirection( unit, TRUE );

	ROOM *room = NULL;
	
	// if there is a warp position we will use it, otherwise we will find the start pos in the level
	BOOL bArrivingThroughOwnTownPortal = FALSE;
	BOOL bFoundLocation = FALSE;
	if (tWarpSpec.guidArrivePortalOwner != INVALID_GUID)
	{
		bFoundLocation = s_TownPortalGetArrivePosition(
			pGame,
			tWarpSpec.guidArrivePortalOwner,
			idLevelDest,
			&room,
			&vPosition,
			&vFaceDirection);
			
		// if this is our portal, then close it
		if (tWarpSpec.guidArrivePortalOwner == UnitGetGuid( unit ))
		{
			bArrivingThroughOwnTownPortal = TRUE;
		}
							
	}
	else if (tWarpSpec.guidArriveAt != INVALID_GUID)
	{
		ASSERTX( TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_GUID_BIT ), "Expected warp flag to arrive at specified GUID to be set!" );
		UNIT *pDestUnit = UnitGetByGuid( pGame, tWarpSpec.guidArriveAt );
		if (pDestUnit)
		{
		
			room = UnitGetRoom( pDestUnit );
			vPosition = UnitGetPosition( pDestUnit );
			vFaceDirection = UnitGetFaceDirection( pDestUnit, FALSE );
			bFoundLocation = TRUE;
			
			// if we want, create a portal here too so it looks like we're coming out of it
			if (TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_VIA_PARTY_PORTAL_BIT))
			{
			
				// create portal
				UNIT *pPortal = s_WarpPartyMemberExitCreate( unit, pDestUnit );
				if (pPortal)
				{
				
					// arrive at the actual portal location instead
					room = UnitGetRoom( pPortal );
					vPosition = UnitGetPosition( pPortal );
					vFaceDirection = UnitGetFaceDirection( pPortal, FALSE );
					
				}
				
			}
			
		}
		
	}

	if (bFoundLocation == FALSE && 
		( AppIsHellgate() || TESTBIT( tWarpSpec.dwWarpFlags, WF_USED_RECALL ) == FALSE ) )
	{
		// if we are leaving a level and are arriving at a level warp portal,
		// we need to find the correct entrance portal
		if ( ( TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_FROM_LEVEL_PORTAL ) &&
			   idLevelLeaving != INVALID_ID ) || tWarpSpec.nLevelAreaPrev != INVALID_ID )
		{

			if( TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_OBJECT_BY_CODE ) )
			{
				LEVEL *pLevelDest = LevelGetByID( pGame, idLevelDest );
				MYTHOS_ANCHORMARKERS::cAnchorMarkers* pMarkers = LevelGetAnchorMarkers( pLevelDest );
				if( pMarkers )
				{
					const MYTHOS_ANCHORMARKERS::ANCHOR *pAnchor = pMarkers->GetAnchorByObjectCode( tWarpSpec.wObjectCode );
					ASSERT( pAnchor );	//should have anchor stone here.
					if( pAnchor )
					{
						UNIT *pUnit = UnitGetById( pGame, pAnchor->nUnitId );
						if( pUnit )
						{
							bFoundLocation = TRUE;
							vPosition = UnitGetPosition( pUnit );
							room = UnitGetRoom( pUnit );
							vFaceDirection = UnitGetFaceDirection( pUnit, TRUE );
							
							vPosition += vFaceDirection * 2.0f;

						}
					}
				}
			}

			if( !bFoundLocation )	//we always look for a return portal first!
			{
				bFoundLocation = s_LevelGetArrivalPortalLocation( 
					pGame,
					idLevelDest, 
					idLevelLeaving,
					tWarpSpec.nLevelAreaPrev,
					&room,
					&vPosition,
					&vFaceDirection);
			}			


			if( !bFoundLocation &&
				AppIsTugboat() &&		//has to be tugboat
				!MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( tWarpSpec.nLevelAreaPrev ) && //the level I'm leaving can't be a static world
				MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( tWarpSpec.nLevelAreaDest ) ) //the level I'm going to has to be a static world
			{
				int nStartingArea;
				int nStartingDepth;
				VECTOR vPositionStarting( 0, 0, 0 );
				if( PlayerGetRespawnLocation( unit,
										      KRESPAWN_TYPE_RETURNPOINT,
											  nStartingArea,
											  nStartingDepth,
											  vPositionStarting ) )
				{
					if( nStartingArea == tWarpSpec.nLevelAreaDest )
					{					
						vPosition = vPositionStarting;
						vFaceDirection = VECTOR( 1, 0, 0 );
						bFoundLocation = TRUE;
						room = RoomGetFromPosition( pGame, LevelGetByID( pGame, idLevelDest ), &vPosition );				
					}
				}
			}

		}
		//if we haven't found a place yet lets look for objects that link multiple levelareas
		if( bFoundLocation == FALSE &&
			AppIsTugboat() )
		{
			UNIT *pWarpConnection = LevelGetWarpConnectionUnit( pGame,
																LevelGetByID( pGame, idLevelDest ),
																idLevelLeaving );
			if( pWarpConnection )
			{
				bFoundLocation = TRUE;
				room = UnitGetRoom( pWarpConnection );
				vPosition = UnitGetPosition( pWarpConnection );
				vFaceDirection = UnitGetFaceDirection( pWarpConnection, FALSE );
			}
																
		}
		
		//we must have logged in now - we aren't using a recall charm, we haven't found a warp that connects to our last location... so we'll use our last saved location
		if( !bFoundLocation &&
			TESTBIT( tWarpSpec.dwWarpFlags, WF_USED_RECALL ) == FALSE &&
			AppIsTugboat() )
		{
			int nStartingArea;
			int nStartingDepth;
			VECTOR vPositionStarting( 0, 0, 0 );
			if( PlayerGetRespawnLocation( unit,
				KRESPAWN_TYPE_PRIMARY,
				nStartingArea,
				nStartingDepth,
				vPositionStarting ) )
			{
				if( nStartingArea == tWarpSpec.nLevelAreaDest )
				{					
					vPosition = vPositionStarting;
					vFaceDirection = VECTOR( 1, 0, 0 );
					bFoundLocation = TRUE;
					room = RoomGetFromPosition( pGame, LevelGetByID( pGame, idLevelDest ), &vPosition );				
				}
			}

		}

		// last ditch option here...
		if (bFoundLocation == FALSE)
		{

			room = LevelGetStartPosition(
				pGame, 
				unit,
				idLevelDest, 
				vPosition, 
				vFaceDirection, 
				tWarpSpec.dwWarpFlags);
			//lets force them to actually set the location
			//why do we do this? Basically if a player just starts the game we still need a starting location.
			if( AppIsTugboat() && TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT ) )
			{
				SETBIT( tWarpSpec.dwWarpFlags, WF_USED_RECALL );	//we need to test to make sure that the anchorstone location isn't set

			}
			ASSERTX_RETFALSE( room, "Expected room" );
			bFoundLocation = TRUE;
		}
	}

	// Used recall charm. See if we need to set the position manually
	if( TESTBIT( tWarpSpec.dwWarpFlags, WF_USED_RECALL ) )
	{
		int nStartingArea;
		int nStartingDepth;
		VECTOR vPositionStarting( 0, 0, 0 );
		LEVEL *pLevelDest = LevelGetByID( pGame, idLevelDest );
		if( PlayerGetRespawnLocation( unit, KRESPAWN_TYPE_ANCHORSTONE, nStartingArea, nStartingDepth, vPositionStarting, pLevelDest ) )
		{
			if( nStartingArea == tWarpSpec.nLevelAreaDest &&
				!( vPositionStarting.fX == -1.0f &&
				vPositionStarting.fY == -1.0f ))
			{							
				ROOM *pRoom = RoomGetFromPosition( pGame, LevelGetByID( pGame, idLevelDest ), &vPositionStarting );
				//ASSERTX( pRoom, "Unable to find room for starting position - could the room be randomly rotating?");
				if( pRoom )
				{
					vPosition = vPositionStarting;
					vPosition.fZ = .1f;
				}
			}
		}
		else if( PlayerGetRespawnLocation( unit, KRESPAWN_TYPE_PRIMARY, nStartingArea, nStartingDepth, vPositionStarting, pLevelDest ) )
		{
			if( nStartingArea == tWarpSpec.nLevelAreaDest &&
				!( vPositionStarting.fX == -1.0f &&
				vPositionStarting.fY == -1.0f ))
			{							
				ROOM *pRoom = RoomGetFromPosition( pGame, LevelGetByID( pGame, idLevelDest ), &vPositionStarting );
				//ASSERTX( pRoom, "Unable to find room for starting position - could the room be randomly rotating?");
				if( pRoom )
				{
					vPosition = vPositionStarting;
					vPosition.fZ = .1f;
				}
			}
		}
	}
	else if (TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_WARP_RETURN_BIT ))
	{

		room = LevelGetStartPosition( pGame, unit, idLevelDest, vPosition, vFaceDirection, tWarpSpec.dwWarpFlags );

	}
		
	if( AppIsTugboat() )
	{
		FREE_PATH_NODE tFreePathNodes[2];
		NEAREST_NODE_SPEC tSpec;
		tSpec.fMaxDistance = 5;
		SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
		SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
		SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
		SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
		SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
		ROOM *pRoom = RoomGetFromPosition( pGame, LevelGetByID( pGame, idLevelDest ), &vPosition );
		if( pRoom )
		{
			room = pRoom;
			int nCount = RoomGetNearestPathNodes(pGame, pRoom, vPosition, 1, tFreePathNodes, &tSpec);
			if(nCount == 1 && pRoom)
			{
				const FREE_PATH_NODE * freePathNode = &tFreePathNodes[0];
				if( freePathNode )
				{
					vPosition = RoomPathNodeGetWorldPosition(pGame, freePathNode->pRoom, freePathNode->pNode);
				}
			}
		}
	}



	// entering a level by means of a normal level transition, we will save as a stat
	// the id of the last level the player was on ... when they die, we will 
	// return them to that level entrance
	if (TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_FROM_LEVEL_PORTAL ) &&
		idLevelLeaving != INVALID_ID)
	{
		UnitSetStat( unit, STATS_LAST_LEVELID, idLevelLeaving );
	}

	// we have to have a location now
	ASSERTX_RETFALSE( room, "No location found in level to warp to" )

	// cancel all trade
	s_TradeCancel( unit, TRUE );

	// null out our previous room, in case we are warping to the same level ( respawning )
	// otherwise the room won't be 'known' on the client, as it won't be sent during the room switch
	if( AppIsTugboat() )
	{
 		UnitRemoveFromRoomList( unit, true );
	}
	
	// warp unit, this will switch the unit room and therefore level
	UnitWarp( 
		unit, 
		room, 
		vPosition, 
		vFaceDirection, 
		VECTOR( 0, 0, 1 ), 
		dwUnitWarpFlags, 
		TRUE, 
		TRUE,
		idLevelDest );

	// get new level
	LEVEL* pLevelNew = UnitGetLevel(unit);
	ASSERTX_RETFALSE( pLevelNew, "Peter says this is still a problem.  Colin says look at the previous assert for clues" );

	// some levels will remove all bad states from the player
	const LEVEL_DEFINITION *pLevelDefNew = LevelDefinitionGet( pLevelNew );
	ASSERTX_RETFALSE(pLevelDefNew, "Peter says we still have a problem here");
	if (pLevelDefNew->bOnEnterClearBadStates)
	{
		
		// clear bad states on us
		s_StateClearBadStates( unit );
		
		// on our pets too
		PetIteratePets( unit, sPetClearBadStates, NULL );
		
	}

	//Remove any items that are binded to the level area
	sRemoveBindedLevelItems( unit, pLevelNew );

	//reset the spawn location and execute player enters level script
	if( AppIsTugboat() )
	{		
		//execute script, the script should set the respawn location if need be
		LevelExecutePlayerEnterLvlScript( pLevelNew, unit );
		//Save this as a primary location if it's a static world...
		PlayerSetRespawnLocation( KRESPAWN_TYPE_PRIMARY, unit, NULL, pLevelNew );
		//when a player first starts the game they don't have an achorstone set yet. So we need to set it!
		//If the player already has had this set it just skips it.
		PlayerSetRespawnLocation( KRESPAWN_TYPE_PLAYERFIRSTLOAD, unit, NULL, pLevelNew );


	}

	if (pLevelOld != pLevelNew)
	{
		if(AppIsHellgate())
		{
			s_QuestEventTransitionLevel(unit, pLevelOld, pLevelNew);
		}
		s_LevelRemovePlayer(pLevelOld,unit);
		s_LevelAddPlayer(pLevelNew,unit);
	}
	if( LevelIsActionLevel( pLevelNew ) &&
		AppIsTugboat() &&
		!MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( LevelGetLevelAreaID( pLevelNew ) ) )
	{
		int DeepestDepth = UnitGetStat( unit, STATS_LEVEL_DEPTH );
		if( pLevelNew->m_nDepth > DeepestDepth )
		{
			UnitSetStat( unit, STATS_LEVEL_DEPTH, pLevelNew->m_nDepth );
		}
		if( UnitGetStat( unit, STATS_LAST_LEVEL_AREA ) != LevelGetLevelAreaID( pLevelNew ) )
		{
			UnitSetStat( unit, STATS_LEVEL_DEPTH, pLevelNew->m_nDepth );
			UnitSetStat( unit, STATS_LAST_LEVEL_AREA, LevelGetLevelAreaID( pLevelNew ) );
		}
	}


	if(AppIsHellgate())
	{
		// is there a possibility of breathing bar?
		s_UnitBreathInit( unit );
	}

	// setup tasks the first time we enter into the world
	if (UnitGetStat( unit, STATS_ENTERED_WORLD ) == FALSE)
	{
		s_sPlayerFirstTimeEnterWorld( unit );
	}

	// mark this level as visited by player
	if( AppIsHellgate() )
	{
		LevelMarkVisited( unit, pLevelNew->m_nLevelDef, WORLDMAP_VISITED );
	}

	// now that we're in the level, if we arrived from our own town portal, we close it
	if (bArrivingThroughOwnTownPortal)
	{
		s_TownPortalsClose( unit );
	}
			
	// trigger event for entering level
	UnitTriggerEvent( pGame, EVENT_WARPED_LEVELS, unit, NULL, NULL );

	WeatherApplyStateToUnit(unit, pLevelNew->m_pWeather);

	// send the status of the sublevels on the new level
	s_LevelSendAllSubLevelStatus( unit );

	// clear all states that should be cleared on level warp
	StateClearAllOfType( unit, STATE_REMOVE_ON_LEVEL_WARP );
	
	// post this level for others to automatically join if they come to this level and need a party
	if (s_PlayerPostsOpenLevels( unit ) == TRUE)	
	{	
	
		// if a player is leaving a level that no longer has any players on it, we will
		// remove that old level from any posting (if it was even posted)
		OpenLevelPlayerLeftLevel( pGame, idLevelLeaving );
				
	}

	// KCK: I think this is better implemented as a client pull rather than a server push,
	// But if tugboat wants to do it this way they can call sCCmdListGameInstances();
//	SendTownInstanceLists( pGame, unit );
				
	return TRUE;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerCountInLimbo(
	GAME *pGame)
{
	int nCount = 0;

	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))	
	{
		UNIT *pPlayer = ClientGetPlayerUnit( pClient );
		if (pPlayer && UnitIsInLimbo( pPlayer ))
		{
			nCount++;
		}
	}
	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAGIC_FOOTSTEP_NUMBER_DOWN		0.16f
#define MAGIC_FOOTSTEP_1STPERSON_TICKS	7
#define MAGIC_FOOTSTEP_3RDPERSON_TICKS	4

#if !ISVERSION(SERVER_VERSION)
static void c_sPlayerUpdateFootsteps(
	GAME * pGame,
	UNIT * pUnit)
{
	ASSERTX_RETURN(pUnit, "Expected Player");
	ASSERTX_RETURN(UnitIsA(pUnit, UNITTYPE_PLAYER), "Attempting to use player footsteps on a non-player unit");

	VECTOR vPlayerPos = UnitGetPosition(pUnit);
	vPlayerPos.fZ += 0.01f;
	if(!c_FootstepShouldPlay(vPlayerPos))
	{
		return;
	}

	int nModelIdThird = c_UnitGetModelIdThirdPerson(pUnit);
	LEVEL * pPlayerLevel = UnitGetLevel(pUnit);
	if(nModelIdThird == INVALID_ID ||
		!pPlayerLevel)
	{
		return;
	}

	const UNIT_DATA * pUnitData = UnitGetData(pUnit);
	int nFootstep = UnitTestFlag(pUnit, UNITFLAG_FOOTSTEP_LEG) ? pUnitData->nFootstepForwardLeft : pUnitData->nFootstepForwardRight;
	if(UnitTestMode(pUnit, MODE_BACKUP))
	{
		nFootstep = UnitTestFlag(pUnit, UNITFLAG_FOOTSTEP_LEG) ? pUnitData->nFootstepBackwardLeft : pUnitData->nFootstepBackwardRight;
	}
	if ( nFootstep == INVALID_ID )
		return;

	GAME_TICK nCurrentGameTick = GameGetTick(pGame);
	GAME_TICK nLastFootstepTick = UnitGetStat(pUnit, STATS_FOOTSTEP_TICK);
	if(GameGetControlUnit(pGame) == pUnit)
	{
		if(c_CameraGetViewType() == VIEW_1STPERSON)
		{
			if(nCurrentGameTick - nLastFootstepTick >= MAGIC_FOOTSTEP_1STPERSON_TICKS)
			{
				float fDistance = LevelLineCollideLen(pGame, pPlayerLevel, vPlayerPos, VECTOR(0, 0, -1), 0.25f);
				if(fDistance < MAGIC_FOOTSTEP_NUMBER_DOWN)
				{
					int nModelIdFirst = c_UnitGetModelId(pUnit);
					MATRIX tWorldScaled = *e_ModelGetWorldScaled( nModelIdFirst );
					VECTOR vModelPosition(0);
					MatrixMultiply(&vModelPosition, &vModelPosition, &tWorldScaled);
					VECTOR vUnitPosition = UnitGetPosition(pUnit);
					VECTOR vUltimatePosition = vUnitPosition - vModelPosition;

					vUltimatePosition /= e_ModelGetScale(nModelIdFirst).fX;

					// Play the footstep
					ATTACHMENT_DEFINITION tDef;
					structclear(tDef);
					tDef.vPosition = vUltimatePosition;
					tDef.nAttachedDefId = nFootstep;
					tDef.eType = ATTACHMENT_FOOTSTEP;
					tDef.dwFlags |= ATTACHMENT_FLAGS_FORCE_2D;
					c_ModelAttachmentAdd(nModelIdFirst, tDef, "", TRUE, ATTACHMENT_OWNER_NONE, INVALID_ID, INVALID_ID);
					UnitSetStat(pUnit, STATS_FOOTSTEP_TICK, nCurrentGameTick);
					UnitSetFlag(pUnit, UNITFLAG_FOOTSTEP_LEG, !UnitTestFlag(pUnit, UNITFLAG_FOOTSTEP_LEG));
				}
			}
			return;
		}
	}
}

#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsPathing(
	UNIT * pUnit)
{
	return UnitTestFlag(pUnit, UNITFLAG_PATHING) && !UnitTestFlag(pUnit, UNITFLAG_TEMPORARILY_DISABLE_PATHING) && pUnit->m_pPathing;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerWarpToStartLocation(
	GAME *pGame,
	UNIT * unit)
{
	ASSERT_RETURN(pGame);
	// this function will only work on the server since the clients don't know about the whole level or where warps are located
	ASSERT_RETURN( IS_SERVER( pGame ) );
	ASSERT_RETURN(unit);
	LEVEL * level = UnitGetLevel(unit);
	ASSERT_RETURN(level);

	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );

	VECTOR vPosition;
	VECTOR vFaceDirection;

	ROOM* homeroom = LevelGetStartPosition( 
		pGame, 
		unit,
		LevelGetID(level), 
		vPosition, 
		vFaceDirection, 
		dwWarpFlags);
	ASSERT_RETURN(homeroom);

	UnitWarp(unit, UnitGetRoom(unit), vPosition, vFaceDirection, VECTOR(0, 0, 1), 0);
	s_UnitSetMode(unit, MODE_IDLE);
	UnitSetZVelocity( unit, 0.0f );	

	s_MetagameEventWatpToStartLocation( unit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerMovementCollideWithMonsters(
	UNIT * pUnit)
{
	if(UnitIsGhost( pUnit ) || UnitHasState(UnitGetGame(pUnit), pUnit, STATE_DONT_COLLIDE_WITH_MONSTERS))
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

VECTOR c_PlayerDoStep(
	GAME * pGame,
	UNIT * unit,
	float fTimeDelta,
	BOOL bRealStep )
{
	ASSERT_RETVAL( AppIsTugboat() || IS_CLIENT( pGame ), UnitGetPosition(unit) );

	// check for dead
	if ( IsUnitDeadOrDying( unit ) )
	{
		UnitSetOnGround(unit, TRUE);
		c_UnitUpdateGfx(unit);
		return UnitGetPosition(unit);
	}

	// get movement params				
	float fVelocityCurr = UnitGetVelocity( unit );

	// tugboat has to calculate movement direction from the path!
	if( AppIsTugboat()  )
	{
		if( sUnitIsPathing(unit) && UnitPathIsActive(unit) )
		{
			UNITMODE eForwardMode = MODE_IDLE;

			//int nStaminaCur = UnitGetStatShift( unit, STATS_STAMINA_CUR );
			//int nStaminaGate = UnitGetStatShift( unit, STATS_STAMINA_GATE );
			BOOL bStopMoving = FALSE;
			if ( UnitHasState( pGame, unit, STATE_STOP_MOVING ) )
			{
				fVelocityCurr = 0;
				bStopMoving = TRUE;
#if !ISVERSION(SERVER_VERSION)
				c_PlayerStopPath( pGame );
#endif
			}
			if( bStopMoving )
			{
				eForwardMode = MODE_IDLE;
				if( UnitTestMode( unit, MODE_WALK ) )
				{
					UnitEndMode( unit, MODE_WALK, 0, FALSE );
				}
				if( UnitTestMode( unit, MODE_RUN ) )
				{
					UnitEndMode( unit, MODE_RUN, 0, FALSE );
				}
				if ( ! UnitTestMode( unit, eForwardMode ) )
				{
					if( IS_SERVER(pGame) )
					{
						s_UnitSetMode( unit, eForwardMode, 0, 0, 0, FALSE );
					}
					else
					{
						c_UnitSetMode( unit, eForwardMode );
					}
				}
			}
			else if ( !UnitHasState( pGame, unit, STATE_DAZED ) )//if( nStaminaCur >= nStaminaGate )
			{
				eForwardMode = MODE_RUN;
				if( UnitTestMode( unit, MODE_WALK ) )
				{
					UnitEndMode( unit, MODE_WALK, 1, FALSE );
				}
				if ( ! UnitTestMode( unit, eForwardMode ) )
				{
					if( IS_SERVER(pGame) )
					{
						s_UnitSetMode( unit, eForwardMode, 0, 0, 0, FALSE );
					}
					else
					{
						c_UnitSetMode( unit, eForwardMode );
					}
				}

			}
			/*else
			{
				if( UnitTestMode( unit, MODE_RUN ) )
				{
					UnitEndMode( unit, MODE_RUN, 1, FALSE );
				}
				eForwardMode = MODE_WALK;
				if ( ! UnitTestMode( unit, eForwardMode ) )
				{
					if( IS_SERVER(pGame) )
					{
						s_UnitSetMode( unit, eForwardMode, 0, 0, FALSE );
					}
					else
					{
						c_UnitSetMode( unit, eForwardMode );
					}
				}
			}*/
		}

		if( IS_CLIENT( pGame ) && unit == GameGetControlUnit( pGame ) )
		{
			CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();

			if( pGlobals->idLastInteractedUnit != INVALID_ID )
			{
				UNIT* pLastInteracted = UnitGetById( pGame, pGlobals->idLastInteractedUnit );
				if( pLastInteracted && UnitsGetDistanceSquaredMinusRadiusXY( unit, pLastInteracted )  >= UNIT_INTERACT_INITIATE_DISTANCE_SQUARED )
				{
					CLT_VERSION_ONLY(UIHideGenericDialog());
					pGlobals->idLastInteractedUnit = INVALID_ID;
				}
			}
		}
		VECTOR vPathDirection = UnitCalculateMoveDirection(pGame, unit, bRealStep);
		UnitSetMoveDirection( unit, vPathDirection );
	}
	VECTOR vDirection = UnitGetMoveDirection( unit );

	// save prev position and start calculation of new one
	VECTOR vPositionPrev = UnitGetPosition(unit);
	VECTOR vNewPosition = vPositionPrev; 

	COLLIDE_RESULT move_result;
	move_result.flags = 0;
	move_result.unit = NULL;
	BOOL bOldOnGround = UnitIsOnGround( unit );

	ROOM* room = UnitGetRoom(unit);
	ASSERT_RETVAL( room, UnitGetPosition(unit) );

	if ( ( !UnitIsOnGround(unit) || ( fVelocityCurr != 0.0f ) || sUnitIsPathing(unit) || UnitIsOnTrain(unit)) &&
		( fTimeDelta > 0.0f ) )
	{
		UnitSetFlag( unit, UNITFLAG_CLIENT_PLAYER_MOVING, TRUE );

		VECTOR vPositionPrev = unit->vPosition;
#ifdef _DEBUG
		fVelocityCurr *= sgfPlayerVelocity;
#endif
		vDirection *= ( fVelocityCurr * fTimeDelta );

		vNewPosition = unit->vPosition + vDirection;

		DWORD dwRoomCollideFlags = 0;

		/*if ( UnitIsOnTrain( unit ) )
		{
			vNewPosition = ObjectTrainMoveRider( unit, vNewPosition, fTimeDelta );
			dwRoomCollideFlags |= MOVEMASK_ON_TRAIN;
		}*/

		if (sPlayerMovementCollideWithMonsters(unit))
		{
			dwRoomCollideFlags |= MOVEMASK_SOLID_MONSTERS;
		}
		
		if ( !UnitIsOnGround( unit ) )
		{
			//trace( "Z Velocity = %4.2f, TimeDelta = %4.2f\n", UnitGetZVelocity(unit), fTimeDelta );
			vNewPosition.fZ += UnitGetZVelocity(unit) * fTimeDelta;
			if ( bRealStep )
			{
				UnitChangeZVelocity( unit, -GRAVITY_ACCELERATION * fTimeDelta );
			}
			if( AppIsHellgate() )
			{
				dwRoomCollideFlags |= MOVEMASK_ONLY_ONGROUND_SOLID;
			}
		}
#if !ISVERSION(SERVER_VERSION)
		if (bRealStep && UnitTestFlag(unit, UNITFLAG_ROOMCOLLISION) && (AppIsHellgate() || IS_CLIENT(pGame)))
		{
			if(AppIsTugboat())
			{
				dwRoomCollideFlags |= MOVEMASK_TEST_ONLY | MOVEMASK_NOT_WALLS | MOVEMASK_NO_GEOMETRY;
			}
			else 
			{
				dwRoomCollideFlags |= MOVEMASK_USE_DESIRED_MELEE_RANGE;
			}
			//RoomCollideNew(unit, vPositionPrev, vDirection, &unit->vPosition, 0 );
			//trace( "---------- Move ----------\n" );
			RoomCollide( unit, vPositionPrev, vDirection, &vNewPosition, UnitGetCollisionRadius( unit ), UnitGetCollisionHeight( unit ), dwRoomCollideFlags, &move_result );

			if (TESTBIT(&move_result.flags, MOVERESULT_SETONGROUND))
			{
				if (!UnitIsOnGround( unit ))
				{
					UnitSetOnGround( unit, TRUE );
				}
			}
			else if (TESTBIT(&move_result.flags, MOVERESULT_TAKEOFFGROUND))
			{
				if (UnitIsOnGround( unit ))
				{
					UnitSetOnGround( unit, FALSE );
				}
			}

			if(AppIsTugboat() )
			{
				if( TESTBIT(&move_result.flags, MOVERESULT_COLLIDE_UNIT) )
				{
					if(	fVelocityCurr > 0 &&
						UnitPathIsActive(unit) )
					{
						VECTOR vForward = vDirection;
						VectorNormalize( vForward );
						VECTOR vDelta = vNewPosition - vPositionPrev;
						float Dot = VectorDot( vDelta, vForward );

						vDelta -= vForward * Dot;
						vDelta.fZ = 0;
							

						vNewPosition = vPositionPrev + vDirection + vDelta;
						vDelta = vNewPosition - vPositionPrev;

						VectorNormalize( vDelta );
						vDelta *= VectorLength( vDirection );
						vNewPosition = vPositionPrev + vDelta;


						vDelta = vNewPosition - vPositionPrev;
						VectorNormalize( vDelta );
						vDelta += UnitGetFaceDirection( unit, TRUE );
						vDelta *= .5f;
						if( Dot >= 0 )
						{
							UnitSetFaceDirection(unit, vDelta);
						}
						else
						{
							UnitSetMoveDirection( unit, vDelta );
						}

					}
					else
					{
						vNewPosition = vPositionPrev + vDirection;
					}
				}
			}
		}
#endif

		if ( bRealStep )
		{
			if(AppIsTugboat() && fVelocityCurr != 0)
			{
				LEVEL* pLevel = UnitGetLevel( unit );

				// Town has contours. This is kind of Ghetto.
				if( pLevel->m_pLevelDefinition->bContoured ||
					IS_CLIENT( pGame ) )
				{
					VECTOR Normal( 0, 0, -1 );
					VECTOR Start = vNewPosition;
					Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
					float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
					VECTOR vCollisionNormal;
					int Material = 0;
					float fCollideDistance = 0;

#if HELLGATE_ONLY
#else
					if( unit->m_pPathing )
					{
						PATH_STATE * path = unit->m_pPathing;
						VECTOR vStart = vNewPosition + VECTOR( -2.0f, -2.0f, 0 );
						VECTOR vEnd = vNewPosition + VECTOR( 2.0f, 2.0f, 0 );
						vEnd -= vStart;
						float fLen = VectorLength( vEnd );
						vEnd /= fLen;
						if( path->m_LastSortPosition.fX == -1.0f ||
							VectorDistanceXY( path->m_LastSortPosition, vNewPosition ) >= fLen * .5f )
						{
							ArrayClear(path->m_SortedFaces);
							LevelSortFaces( pGame, pLevel, path->m_SortedFaces, vStart, vEnd, fLen );
							path->m_LastSortPosition = vNewPosition;
						}
						fCollideDistance = LevelLineCollideLen( pGame, pLevel, path->m_SortedFaces, Start, Normal, fDist, Material, &vCollisionNormal);
					}
#endif

					if (fCollideDistance < fDist &&
						Material != PROP_MATERIAL &&
						Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
						Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
					{
						vNewPosition.fZ = Start.fZ - fCollideDistance;
					}
				}
				else
				{
					vNewPosition.fZ = 0;
				}
				if( IS_SERVER( pGame ) )
				{					
					ObjectsCheckForTriggers(pGame, UnitGetClient( unit ), unit);
					if (unit == NULL || UnitTestFlag( unit, UNITFLAG_FREED ))
					{
						return cgvNull;
					}
				}
			}

			// set the new position for real steps
			UnitSetPosition(unit, vNewPosition);

			// drb.path
			if ( sUnitIsPathing(unit) )
			{
				if ( IS_CLIENT( pGame ) )
				{
					c_UpdatePathing( pGame, unit, vPositionPrev, vNewPosition, &move_result );

				}
				else
				{
					s_UpdatePathing( pGame, unit, vPositionPrev, vNewPosition, TRUE );

					if ( UnitPathIsActive(unit) )
					{
						s_UnitAutoPickupObjectsAround( unit );
					}
				}
			}

			// did I fall?
  			if ( bOldOnGround && !UnitIsOnGround( unit ) && !UnitModeIsJumping( unit ) )
  			{
 				UnitSetZVelocity( unit, 0.0f );
  			}

#if !ISVERSION(SERVER_VERSION)
			if ( !bOldOnGround && UnitIsOnGround( unit ) && UnitModeIsJumping( unit ) )
			{
				UnitModeEndJumping( unit, FALSE );
				UnitSetZVelocity( unit, 0.0f );
			}
#endif

			if(AppIsHellgate())
			{
				ObjectsCheckBlockingCollision( unit );
			}

			//trace( "Player Move: Vel(%8.4f) Time(%8.4f) - Move(%8.4f, %8.4f, %8.4f) Dir(%8.4f, %8.4f, %8.4f)\n",
			//	fVelocityCurr,
			//	fTimeDelta,
			//	vDirection.fX, vDirection.fY, vDirection.fZ,
			//	vTemp.fX, vTemp.fY, vTemp.fZ );

			//trace("Player Move: Vel(%8.2f) Time(%8.2f) - Move(%8.2f, %8.2f, %8.2f) Old(%8.2f, %8.2f, %8.2f) New(%8.2f, %8.2f, %8.2f)\n",
			//	fVelocityCurr,
			//	fTimeDelta,
			//	vDirection.fX, vDirection.fY, vDirection.fZ,
			//	vPositionPrev.fX, vPositionPrev.fY, vPositionPrev.fZ,
			//	unit->vPosition.fX, unit->vPosition.fY, unit->vPosition.fZ);

			ROOM * newroom = UnitUpdateRoomFromPosition(pGame, unit, &vPositionPrev, false);
			if (!newroom && !IS_SERVER(pGame) ) //Tugboat client wall walking should only warp on client side, since weirdness may happen with small epsilons.
			{
				if( AppIsTugboat() )
				{
					vNewPosition = vPositionPrev;
					UnitSetPosition(unit, vNewPosition);
					newroom = UnitUpdateRoomFromPosition(pGame, unit, &vPositionPrev, false);
#if !ISVERSION(SERVER_VERSION)
					c_PlayerStopPath(pGame);
#endif
				}
				else
				{
					//Trace room and location where we jumped out of the world.
					//It may be better to move back to the same room, not the start,
					//though this could cause an infinite loop if the start location is invalid.
					// -RJD
#ifdef _DEBUG
					VECTOR vPosition, vFaceDirection;
					LEVEL * level = RoomGetLevel(room);
					ASSERTV(0, "Moved out of the world in room %s, level %s, location %f, %f, %f, room tWorldMatrix_41:%f _42:%f _43:%f ",
						RoomGetDevName(room), LevelGetDevName(level), vPositionPrev.fX, vPositionPrev.fY, vPositionPrev.fZ,
						room->tWorldMatrix._41, room->tWorldMatrix._42, room->tWorldMatrix._43);
					//May need to trace more worldmatrix info to get the room's orientation.
#endif
#if !ISVERSION(SERVER_VERSION)
					if(unit == GameGetControlUnit(pGame))
					{	//If I am the player, restart.  Otherwise, don't.
						MSG_CCMD_PLAYER_FELL_OUT_OF_WORLD msg;
						c_SendMessage( CCMD_PLAYER_FELL_OUT_OF_WORLD, &msg );
						//PlayerWarpToStartLocation(pGame, unit);
					}
#endif // !ISVERSION(SERVER_VERSION)
				}
				
			}
		}

		if ( AppIsHellgate() )
		{
			SkillsUpdateMovingFiringError( unit, fVelocityCurr );
		}

#if !ISVERSION(SERVER_VERSION)
		if( AppIsHellgate() )
		{
			c_UnitUpdateGfx(unit);
			c_sPlayerUpdateFootsteps(pGame, unit);
		}
#endif
	} else {

		if ( AppIsHellgate() )
		{
			SkillsUpdateMovingFiringError( unit, 0.0f );
		}

	}
	
	if ( bRealStep )
	{
		UnitCalculateTargetReached(
			pGame, 
			unit, 
			vPositionPrev, 
			vNewPosition, 
			TESTBIT( &move_result.flags, MOVERESULT_COLLIDE ), 
			bOldOnGround, 
			UnitIsOnGround( unit ));

		UNIT_TAG_VALUE_TIME * pTag = UnitGetValueTimeTag(unit, TAG_SELECTOR_METERS_MOVED);
		if (pTag)
		{
			pTag->m_nCounts[pTag->m_nCurrent] += (unsigned int)(100*VectorLength(vPositionPrev - vNewPosition));
		}

		//QuestUpdateStone( pGame, unit );
	}
	
	return vNewPosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_PlayerUpdateOthers(
	GAME *pGame)
{	
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();

	// go through the players
	for (CPLAYER *pCPlayer = pGlobals->pCPlayerList; pCPlayer; pCPlayer = pCPlayer = pCPlayer->pNext)
	{
		UNIT *pUnit = UnitGetById( pGame, pCPlayer->idPlayer );
		ASSERTX_CONTINUE( pUnit, "Unable to find player unit on client" );

		// ignore ourselves
		if (GameGetControlUnit( pGame ) == pUnit)
		{
			continue;
		}

		// do just like we received a new position update ... this will determine if we
		// should stop moving for instance
		c_PlayerMoveMsgUpdateOtherPlayer( pUnit );
	}
}
#endif // !ISVERSION(SERVER_VERSION)


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR c_PlayerGetDirectionFromInput(
	int nInput )
{
	int dir = 4;
	static const float dirtoangle[] =
	{
		-PI/4.0f, 0.0f, PI/4.0f, -PI/2.0f, 0, PI/2.0f, -((PI/4.0f)+(PI/2.0f)), PI, (PI/4.0f)+(PI/2.0f)
	};

	if (nInput & PLAYER_MOVE_FORWARD)
	{
		dir -= 3;
	} 
	else if (nInput & PLAYER_MOVE_BACK)
	{
		dir += 3;
	} 

	if (nInput & PLAYER_STRAFE_LEFT)
	{
		dir -= 1;
	} 
	else if (nInput & PLAYER_STRAFE_RIGHT)
	{
		dir += 1;
	}

	VECTOR vDirection;
	VectorDirectionFromAngleRadians(vDirection, sgClientPlayerGlobals.fViewAngle + dirtoangle[dir]);
	return vDirection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sPlayerProcessInput(
	GAME * pGame,
	UNIT * unit,
	float fTimeDelta )
{
	int nInput = sgClientPlayerGlobals.nMovementKeys;

	BOOL bStopMoving = FALSE;
	if ( SkillsIgnoreUnitInput( pGame, unit, bStopMoving ) )
		nInput = 0;

	if ( UnitHasState( pGame, unit, STATE_STOP_MOVING ) )
		bStopMoving = TRUE;

	if(AppIsHellgate())
	{
		if ( nInput & PLAYER_AUTORUN )
		{
			nInput &= ~( PLAYER_AUTORUN | PLAYER_MOVE_FORWD_AND_BACK );
			nInput |= PLAYER_MOVE_FORWARD;
		}
	}
	

	// check invalid pressing forward and back at the same time
	if ((nInput & PLAYER_MOVE_FORWD_AND_BACK) == PLAYER_MOVE_FORWD_AND_BACK)
	{
		nInput &= ~PLAYER_MOVE_FORWD_AND_BACK;
	}

	// check invalid pressing left and right at the same time
	if ((nInput & PLAYER_MOVE_LEFT_AND_RIGHT) == PLAYER_MOVE_LEFT_AND_RIGHT)
	{
		nInput &= ~PLAYER_MOVE_LEFT_AND_RIGHT;
	}

	// check invalid pressing of rotate left and right at the same time
	if ((nInput & PLAYER_ROTATE_LEFT_AND_RIGHT) == PLAYER_ROTATE_LEFT_AND_RIGHT)
	{
		nInput &= ~PLAYER_ROTATE_LEFT_AND_RIGHT;
	}

	BOOL bUnitIsInAttack = FALSE;
	if( AppIsTugboat() )
	{
		bUnitIsInAttack = UnitIsInAttackLoose( unit );
		const CAMERA_INFO * camera_info = c_CameraGetInfo(FALSE);
		if (camera_info && InputGetAdvancedCamera() )
		{
			float fVelocityBonus = ((float)UnitGetStat( unit, STATS_VELOCITY_BONUS ) ) / 100.0f + sgfPlayerVelocity;
			BOOL bStopMoving = (nInput & PLAYER_HOLD ) || bUnitIsInAttack;


			VECTOR vDirection = CameraInfoGetDirection(camera_info);
			VECTOR vTarget;
			vDirection.fZ = 0;
			VectorNormalize( vDirection, vDirection );
			VectorZRotation( vDirection, DegreesToRadians( 90.0f ) );
			float fNewAngle = (float)atan2f( -vDirection.fX, vDirection.fY);
			const CAMERA_INFO * camera_info = c_CameraGetInfo(FALSE);

			if (( nInput & PLAYER_MOVE_FORWARD || nInput & PLAYER_AUTORUN ) && nInput & PLAYER_STRAFE_LEFT  && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( -45.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}	
			else if (( nInput & PLAYER_MOVE_FORWARD || nInput & PLAYER_AUTORUN ) && nInput & PLAYER_STRAFE_RIGHT && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( 45.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}	
			else if (nInput & PLAYER_MOVE_BACK && nInput & PLAYER_STRAFE_LEFT && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( -135.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}	
			else if (nInput & PLAYER_MOVE_BACK && nInput & PLAYER_STRAFE_RIGHT && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( 135.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}	
			else if (( nInput & PLAYER_MOVE_FORWARD || nInput & PLAYER_AUTORUN ) && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}	
			else if (nInput & PLAYER_STRAFE_LEFT && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( -90.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}
			if (nInput & PLAYER_STRAFE_LEFT && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( -90.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}
			else if (nInput & PLAYER_STRAFE_RIGHT && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( 90.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}
			else if (nInput & PLAYER_MOVE_BACK && !bStopMoving)
			{
				if( c_PlayerCanPath() )
				{
					VECTOR vDirection = CameraInfoGetDirection(camera_info);
					vDirection.fZ = 0;

					VectorNormalize( vDirection, vDirection );
					vDirection *= ( 1.5f * fVelocityBonus );
					VectorZRotation( vDirection, DegreesToRadians( 180.0f ) );
					vTarget = UnitGetPosition( unit ) + vDirection;
					c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					if( !UnitPathIsActive(unit) )
					{
						vTarget = UnitGetPosition( unit ) + vDirection * ( 2.0f * fVelocityBonus );
						c_PlayerMoveToLocation( pGame, &vTarget, TRUE );
					}
				}
			}
			else if (nInput & PLAYER_ROTATE_LEFT)
			{
				PlayerMouseRotate(pGame, (-800.0f * fTimeDelta));
				if( sUnitIsPathing( unit )  && !UnitPathIsActive(unit) )
					c_PlayerSetAngles( pGame, fNewAngle );
			}
			else if (nInput & PLAYER_ROTATE_RIGHT)
			{
				PlayerMouseRotate(pGame, (800.0f * fTimeDelta));
				if( sUnitIsPathing( unit )  && !UnitPathIsActive(unit) )
					c_PlayerSetAngles( pGame, fNewAngle );
			}

			
		}
	}

	if(AppIsHellgate())
	{
		if ( ( nInput & PLAYER_CROUCH ) && ( UnitIsA( unit, UNITTYPE_HUNTER )))
		{
			// if you are crouching, you can *only* crouch... no jump, move, etc...
			nInput = PLAYER_CROUCH;
		}
	}

	BOOL bSetModesToIdle = FALSE;
	if(AppIsHellgate())
	{
		// Run at unit when in melee code...
		if ( !sgbPathing && !nInput)
		{
			VECTOR vForward = UnitGetFaceDirection( unit, FALSE );
			nInput = c_SkillGetPlayerInputOverride( pGame, unit, vForward, bSetModesToIdle );
		}

#ifdef DRB_3RD_PERSON
		VECTOR vCameraDir;
		if ( nInput && CameraRotateDetached( vCameraDir ) && ( c_CameraGetViewType() == VIEW_3RDPERSON ) )
		{
			// snap player back to old direction
			float angle = atan2( vCameraDir.fY, vCameraDir.fX );
			c_PlayerSetAngles( pGame, angle, sgClientPlayerGlobals.fViewPitch );
		}
#endif
	}

	int nNewMovement = PLAYER_MOVE_NONE;
	float fRotational = 0.0f;
	VECTOR vDirection;

	UNITMODE eForwardMode = MODE_IDLE;
	UNITMODE eSideMode = MODE_IDLE;

	if ( sgbPathing ||
		(AppIsTugboat() && UnitPathIsActive(unit) && !bUnitIsInAttack))
	{
		if(AppIsHellgate())
		{
			// face dest
			VECTOR vDelta = sgvPathDest - unit->vPosition;
			float fDistance = sqrtf( vDelta.fX * vDelta.fX + vDelta.fY * vDelta.fY );
			if ( ( fDistance <= sgfPathLastDist ) && ( fDistance >= 0.1f ) )
			{
				vDirection = sgvPathDirection;
				nNewMovement |= PLAYER_MOVE_FORWARD;
			}
			else
				sgbPathing = FALSE;
		}
		else
		{
			vDirection = UnitGetMoveDirection( unit );
			
			if ( !UnitHasState( pGame, unit, STATE_DAZED ) )
			{
				eForwardMode = MODE_RUN;
			}
			else
			{
				eForwardMode = MODE_WALK;
			}

			/*int nStaminaCur = UnitGetStatShift( unit, STATS_STAMINA_CUR );
			int nStaminaGate = UnitGetStatShift( unit, STATS_STAMINA_GATE );
			if( nStaminaCur >= nStaminaGate )
			{
				eForwardMode = MODE_RUN;
			}
			else
			{
				eForwardMode = MODE_WALK;
			}*/
			//eForwardMode = MODE_RUN;
		}
	}
	else if( AppIsHellgate() )
	{
		vDirection = c_PlayerGetDirectionFromInput( nInput );

		if (nInput & PLAYER_MOVE_FORWARD)
		{
			nNewMovement |= nInput & PLAYER_MOVE_FORWARD;
			eForwardMode = MODE_RUN;
		} 
		else if (nInput & PLAYER_MOVE_BACK)
		{
			nNewMovement |= nInput & PLAYER_MOVE_BACK;
			eForwardMode = MODE_BACKUP;
		} 

		if (nInput & PLAYER_STRAFE_LEFT)
		{
			nNewMovement |= nInput & PLAYER_STRAFE_LEFT;
			eSideMode = MODE_STRAFE_LEFT;
		} 
		else if (nInput & PLAYER_STRAFE_RIGHT)
		{
			nNewMovement |= nInput & PLAYER_STRAFE_RIGHT;
			eSideMode = MODE_STRAFE_RIGHT;
		}

		if (nInput & PLAYER_ROTATE_LEFT)
		{
			sPlayerKeyboardRotation( unit, -(PLAYER_KEYBOARD_ROTATE_SPEED * fTimeDelta) );
			fRotational = -PLAYER_KEYBOARD_ROTATE_SPEED;
			nNewMovement |= nInput & PLAYER_ROTATE_LEFT;
		}
		else if (nInput & PLAYER_ROTATE_RIGHT)
		{
			sPlayerKeyboardRotation( unit, (PLAYER_KEYBOARD_ROTATE_SPEED * fTimeDelta) );
			fRotational = PLAYER_KEYBOARD_ROTATE_SPEED;
			nNewMovement |= nInput & PLAYER_ROTATE_RIGHT;
		}
	}

	if ( bStopMoving || bSetModesToIdle )
	{
		eForwardMode = MODE_IDLE;
		eSideMode = MODE_IDLE;
	}

	sgClientPlayerGlobals.fRotationalVelocity = fRotational;

	if (IsUnitDeadOrDying(unit))
	{
		sgClientPlayerGlobals.eForwardMode = eForwardMode;
		sgClientPlayerGlobals.eSideMode = eSideMode;
		return;
	}

	if ( sgClientPlayerGlobals.eForwardMode != eForwardMode &&
		sgClientPlayerGlobals.eForwardMode != MODE_IDLE )
		UnitEndMode( unit, sgClientPlayerGlobals.eForwardMode );

	if ( sgClientPlayerGlobals.eSideMode != eSideMode &&
		sgClientPlayerGlobals.eSideMode != MODE_IDLE )
		UnitEndMode( unit, sgClientPlayerGlobals.eSideMode );

	if ( eForwardMode != MODE_IDLE && ! UnitTestMode( unit, eForwardMode ) )
		c_UnitSetMode( unit, eForwardMode );

	if ( eSideMode != MODE_IDLE && ! UnitTestMode( unit, eSideMode ) )
		c_UnitSetMode( unit, eSideMode );

	sgClientPlayerGlobals.eForwardMode = eForwardMode;
	sgClientPlayerGlobals.eSideMode    = eSideMode;

	UnitCalcVelocity(unit);

	if(AppIsHellgate())
	{
		if ( UnitIsOnGround( unit ) && !bStopMoving && ! SkillIsOnWithFlag( unit, SKILL_FLAG_ON_GROUND_ONLY ) )
		{
			if ( nInput & PLAYER_JUMP )
			{
				// jump man!
				c_PlayerClearMovement( pGame, PLAYER_JUMP );		
				UnitModeStartJumping( unit );
			}
		}

		UnitSetMoveDirection( unit, vDirection );
	}
}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void PlayerMovementUpdate( GAME *pGame, float fTimeDelta )
{

	ASSERT( IS_CLIENT( pGame ) );
	if ( fTimeDelta <= 0.0f )
	{
		return;
	}

	if( sgfRePathTimer > 0 )
	{
		sgfRePathTimer -= fTimeDelta;
	}
	if( sgfTurnTimer > 0 )
	{
		sgfTurnTimer -= fTimeDelta;
	}
	UNIT* unit = GameGetControlUnit( pGame );

	ROOM* room = UnitGetRoom(unit);
	if (!room)
	{
		return;
	}

#ifdef HAVOK_ENABLED
	if ( unit->pHavokRigidBody )
	{
		return;
	}
#endif

	if(AppIsHellgate())
	{
		UnitSetFlag( unit, UNITFLAG_CLIENT_PLAYER_MOVING, FALSE );

#if !ISVERSION(SERVER_VERSION)
		c_sPlayerProcessInput( pGame, unit, fTimeDelta );

		c_PlayerDoStep( pGame, unit, fTimeDelta, TRUE );

		// update other player movements for multiplayer
		c_PlayerUpdateOthers( pGame );
#endif //!ISVERSION(SERVER_VERSION)
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		// break this down into ticks, otherwise collisions can go bonkers.
		int	Ticks		= ( int )CEIL( fTimeDelta / .1f );
		if ( Ticks > 10 )
		{
			Ticks = 10;
		}
		if( Ticks < 1 )
		{
			Ticks = 1;
		}
		// calculate the sub-interval length for each tick
		float	TickInterval	= ( fTimeDelta / ( ( float ) Ticks ) );

		// perform ticks on the player.
		for ( int i = Ticks - 1; i >= 0; i-- )
		{
			UnitSetFlag( unit, UNITFLAG_CLIENT_PLAYER_MOVING, FALSE );

			c_sPlayerProcessInput( pGame, unit, TickInterval );
			c_PlayerDoStep( pGame, unit, TickInterval, TRUE );

		}
#endif //!ISVERSION(SERVER_VERSION)
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)
void MouseToScene( GAME * pGame, VECTOR *pvOrig, VECTOR *pvDir )
{
    MATRIX mWorld, mView, mProj;
	e_GetWorldViewProjMatricies( &mWorld, &mView, &mProj, NULL, TRUE );

    int nCursorX, nCursorY;
    UIGetCursorPosition( &nCursorX, &nCursorY, FALSE );

    // Compute the vector of the pick ray in screen space
	int nScreenWidth, nScreenHeight;
	e_GetWindowSize( nScreenWidth, nScreenHeight );

	VECTOR v;
    v.fX =  ( ( ( 2.0f * (float)nCursorX ) / (float)nScreenWidth  ) - 1 ) / mProj._11;
    v.fY = -( ( ( 2.0f * (float)nCursorY ) / (float)nScreenHeight ) - 1 ) / mProj._22;
    v.fZ =  1.0f;
    
    //MATRIX m = mWorld * mView;
	MATRIX m;
	MatrixMultiply( &m, &mWorld, &mView );
	MatrixInverse( &m, &m );

    // Transform the screen space pick ray into 3D space
    pvDir->fX  = v.fX*m._11 + v.fY*m._21 + v.fZ*m._31;
    pvDir->fY  = v.fX*m._12 + v.fY*m._22 + v.fZ*m._32;
    pvDir->fZ  = v.fX*m._13 + v.fY*m._23 + v.fZ*m._33;
    pvOrig->fX = m._41;
    pvOrig->fY = m._42;
    pvOrig->fZ = m._43;
}
#define MAX_PATH_DISTANCE 30
#define SHORT_PATH_DISTANCE 5
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerMoveToMouse(
	GAME *pGame,
	BOOL bForce)
{
	ASSERT(IS_CLIENT(pGame));

	UNIT *pUnit = GameGetControlUnit( pGame );
	if ( !pUnit )
		return;

	// find cursor in scene
	VECTOR vStart, vDirection;
	if(AppIsHellgate())
	{
		MouseToScene( pGame, &vStart, &vDirection );
	}
	else
	{
		if( c_PlayerTestMovement( PLAYER_STRAFE_LEFT) ||
			c_PlayerTestMovement( PLAYER_STRAFE_RIGHT ) ||
			c_PlayerTestMovement( PLAYER_MOVE_FORWARD ) || 
			c_PlayerTestMovement( PLAYER_MOVE_BACK ) )
		{
			return;
		}

		UI_TB_MouseToScene( pGame, &vStart, &vDirection );
		if( vDirection.fZ > -.02f )
		{
			vDirection.fZ = -.02f;
			VectorNormalize( vDirection );
		}
	}

	// this is just a trial for now.. will have to figure out a real algorithm later - drb

	// first calc an x,y location from the current mouse position projected into the scene at our Z + 1/2 our height
	sgvPathDest.fZ = pUnit->vPosition.fZ;

	float fLength = ( sgvPathDest.fZ - vStart.fZ  ) / vDirection.fZ;
	sgvPathDest.fX = vStart.fX + ( vDirection.fX * fLength );
	sgvPathDest.fY = vStart.fY + ( vDirection.fY * fLength );
	if(AppIsHellgate())
	{
		sgvPathDest.fZ += PLAYER_HEIGHT / 2.0f;
	}
	else
	{
		sgvPathDest.fZ =  pUnit->vPosition.fZ;
	}

	// calc how far away that is from us
	VECTOR vTest = pUnit->vPosition;
	if(AppIsHellgate())
	{
		vTest.fZ += ( PLAYER_HEIGHT / 2.0f );
	}

	VECTOR vTestDir = sgvPathDest - vTest;
	if(AppIsTugboat())
	{
		// this is pretty lame, just closing the augment panel on the client if we're moving.
		// but in combination with a distance check on the server, could be workable.
		// the server doesn't have knowledge of the state of the augment panel right now 
		// on the client,
#if !ISVERSION( SERVER_VERSION )

		if( UIPaneVisible( KPaneMenuItemAugment ) )
		{
			c_ItemUpgradeClose(IUT_AUGMENT);
		}
#endif
		vTestDir.fZ = 0;
		VectorNormalize( vTestDir, vTestDir );

		// project a ray into scene to see if there is something solid in between
		sgfRePathTimer = .1f;
		VECTOR vDelta = sgvPathDest - pUnit->vPosition;
		vDelta.fZ = 0.0f;
		sgfPathLastDist = sqrtf( vDelta.fX * vDelta.fX + vDelta.fY * vDelta.fY );
		if( sgfPathLastDist < .75f )
		{
			return;
		}
		sgfPathLastDist = MIN( sgfPathLastDist, (float)MAX_PATH_DISTANCE );

		if( !bForce )
		{
			sgfPathLastDist = MIN( sgfPathLastDist, (float)SHORT_PATH_DISTANCE );
		}
		VectorNormalize( vDelta );
		vDelta *= sgfPathLastDist;	
		sgvPathDest = pUnit->vPosition + vDelta;

		if( UnitIsInAttackLoose( pUnit ) )
			return;

		if( UnitIsInAttack( pUnit ) )
		{
			SkillStopAll(pGame, pUnit);
		}
		// move forward
		VECTOR vPathTarget = sgvPathDest;

		if( sgfPathLastDist < 1.5f )
		{
			VectorNormalize( vDelta );
			vDelta *= 1.5f;
			vPathTarget = pUnit->vPosition + vDelta;
		}
		DWORD dwFlags = 0;
		SETBIT(dwFlags, PCF_ACCEPT_PARTIAL_SUCCESS_BIT);
		UnitSetMoveTarget(pUnit, vPathTarget, vPathTarget );
		BOOL bSuccess = UnitCalculatePath( pGame, pUnit, PATH_FULL, dwFlags );
		if( !bSuccess )
		{
			if( UnitTestMode( pUnit, MODE_WALK ) )
			{
				UnitEndMode( pUnit, MODE_WALK, 0, FALSE );
			}
			if( UnitTestMode( pUnit, MODE_RUN ) )
			{
				UnitEndMode( pUnit, MODE_RUN, 0, FALSE );
			}
		}		
	}
	else
	{
		float fTotalDistance = VectorLength( vTestDir );
		VectorNormalize( vTestDir, vTestDir );

		LEVEL* pLevel = UnitGetLevel( pUnit );
		// project a ray into scene to see if there is something solid in between
		float fDistance = LevelLineCollideLen( pGame, pLevel, vTest, vTestDir, fTotalDistance );

		// if there is, return for the moment
		if ( fDistance < ( fTotalDistance - 0.01f ) )
			return;

		// move forward
		sgbPathing = TRUE;
		VECTOR vDelta = sgvPathDest - pUnit->vPosition;
		vDelta.fZ = 0.0f;
		sgfPathLastDist = sqrtf( vDelta.fX * vDelta.fX + vDelta.fY * vDelta.fY );

		// face dest
		if ( VectorIsZero( vDelta ) )
			sgvPathDirection = VECTOR ( 0, 1, 0 );
		else 
			VectorNormalize( sgvPathDirection, vDelta );

		float fNewAngle = acosf( PIN( -sgvPathDirection.fX, -1.0f, 1.0f ) );
		if ( sgvPathDirection.fY > 0.0f )
		{
			fNewAngle = -sgClientPlayerGlobals.fViewAngle;
		}
		fNewAngle += PI;

		c_PlayerSetAngles( pGame, fNewAngle, sgClientPlayerGlobals.fViewPitch );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerMoveToLocation(
	GAME *pGame,
	VECTOR* pvDestination, 
	BOOL bIgnoreKeys /*=FALSE*/ )
{
	ASSERT(IS_CLIENT(pGame));

	UNIT *pUnit = GameGetControlUnit( pGame );
	if ( !pUnit )
		return;

	if( AppIsTugboat() && !bIgnoreKeys )
	{
		if( c_PlayerTestMovement( PLAYER_STRAFE_LEFT) ||
			c_PlayerTestMovement( PLAYER_STRAFE_RIGHT ) ||
			c_PlayerTestMovement( PLAYER_MOVE_FORWARD ) || 
			c_PlayerTestMovement( PLAYER_MOVE_BACK ) )
		{
			return;
		}
	}

	sgvPathDest = *pvDestination;
	sgvPathDest.fZ = pUnit->vPosition.fZ;


	// calc how far away that is from us
	VECTOR vTest = pUnit->vPosition;

	VECTOR vTestDir = sgvPathDest - vTest;
	vTestDir.fZ = 0;
//	float fTotalDistance = VectorLength( vTestDir );
	VectorNormalize( vTestDir, vTestDir );

//	LEVEL * pLevel = UnitGetLevel( pUnit );

	sgfRePathTimer = .1f;
	VECTOR vDelta = sgvPathDest - pUnit->vPosition;
	vDelta.fZ = 0.0f;
	sgfPathLastDist = sqrtf( vDelta.fX * vDelta.fX + vDelta.fY * vDelta.fY );

	if( UnitIsInAttackLoose( pUnit ) )
		return;
	if( UnitIsInAttack( pUnit ) )
	{
		SkillStopAll(pGame, pUnit);
	}

	// move forward
	//sgbPathing = TRUE;

	VECTOR vPathTarget = sgvPathDest;
	if( AppIsHellgate() )
	{
		UnitSetMoveTarget(pUnit, vPathTarget, vPathTarget - UnitGetPosition(pUnit));
	}
	if( AppIsTugboat() )
	{
		// this is pretty lame, just closing the augment panel on the client if we're moving.
		// but in combination with a distance check on the server, could be workable.
		// the server doesn't have knowledge of the state of the augment panel right now 
		// on the client,
#if !ISVERSION( SERVER_VERSION )

		if( UIPaneVisible( KPaneMenuItemAugment ) )
		{
			c_ItemUpgradeClose(IUT_AUGMENT);
		}
#endif
		UnitSetMoveTarget(pUnit, vPathTarget, vPathTarget );
	}
	DWORD dwFlags = 0;
	SETBIT(dwFlags, PCF_ACCEPT_PARTIAL_SUCCESS_BIT);
	UnitCalculatePath( pGame, pUnit, PATH_FULL, dwFlags );
	
	if( AppIsTugboat() &&
		UnitGetPathNodeCount( pUnit ) < 2 )
	{
		c_PlayerClearMovement(pGame, PLAYER_AUTORUN);
	}
	// create a path for splines
	//if( !bSuccess )
	//{
		//sgbPathing = FALSE;
	//}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerCanPath()
{
	return sgfRePathTimer <= 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerStopPath(
	GAME *pGame )
{
	ASSERT(IS_CLIENT(pGame));

	UNIT *pUnit = GameGetControlUnit( pGame );
	if ( !pUnit )
		return;

	if(AppIsHellgate())
	{
		sgbPathing = FALSE;
	}
	else
	{

		c_UnitSetMode( pUnit, MODE_IDLE );
		UnitDoPathTermination( pGame, pUnit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerMouseAimLocation(
	GAME *pGame,
	VECTOR* pvDestination )
{
	//ConsoleString(CONSOLE_SYSTEM_COLOR,"aim location\n" );
	ASSERT(IS_CLIENT(pGame));

	UNIT *pUnit = GameGetControlUnit( pGame );
	if ( !pUnit )
		return;

	if( IsUnitDeadOrDying( pUnit ) )
	{
		return;
	}
	if( AppIsTugboat() )
	{
		if( c_PlayerTestMovement( PLAYER_ROTATE_LEFT) ||
			c_PlayerTestMovement( PLAYER_ROTATE_RIGHT ) )
		{
			return;
		}
	}
	// this is just a trial for now.. will have to figure out a real algorithm later - drb

	// first calc an x,y location from the current mouse position projected into the scene at our Z + 1/2 our height
	sgvPathDest = *pvDestination;
	sgvPathDest.fZ = pUnit->vPosition.fZ;

	// calc how far away that is from us
	VECTOR vTest = pUnit->vPosition;

	VECTOR vTestDir = sgvPathDest - vTest;
	vTestDir.fZ = 0;
	VectorNormalize( vTestDir, vTestDir );

	VECTOR vDelta = sgvPathDest - pUnit->vPosition;
	vDelta.fZ = 0.0f;

	// face dest
	if ( VectorIsZero( vDelta ) /*|| sgfPathLastDist < .1f*/  )
		sgvPathDirection = VECTOR ( 0, 1, 0 );
	else 
		VectorNormalize( sgvPathDirection, vDelta );


	VectorZRotation( sgvPathDirection, DegreesToRadians( 90.0f ) );
	float fNewAngle = (float)atan2f( -sgvPathDirection.fX, sgvPathDirection.fY);


	c_PlayerSetAngles( pGame, fNewAngle  );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_PlayerMouseAim( GAME *pGame,
					   BOOL bAimAtWeaponHeight /* = FALSE */)
{
	ASSERT(IS_CLIENT(pGame));

	UNIT *pUnit = GameGetControlUnit( pGame );
	if ( !pUnit )
		return;

	if( IsUnitDeadOrDying( pUnit ) )
	{
		return;
	}

	// find cursor in scene
	VECTOR vStart, vDirection;
	if(AppIsHellgate())
	{
		MouseToScene( pGame, &vStart, &vDirection );
	}
	else
	{
		if( c_PlayerTestMovement( PLAYER_ROTATE_LEFT) ||
			c_PlayerTestMovement( PLAYER_ROTATE_RIGHT ) )
		{
			return;
		}
		UI_TB_MouseToScene( pGame, &vStart, &vDirection );
		if( vDirection.fZ > -.02f )
		{
			vDirection.fZ = -.02f;
			VectorNormalize( vDirection );
		}
	}

	// this is just a trial for now.. will have to figure out a real algorithm later - drb

	if(AppIsHellgate())
	{
		// first calc an x,y location from the current mouse position projected into the scene at our Z + 1/2 our height
		VECTOR vDest;
		vDest.fZ = pUnit->vPosition.fZ;
		float fLength = ( vDest.fZ - vStart.fZ ) / vDirection.fZ;
		vDest.fX = vStart.fX + ( vDirection.fX * fLength );		
		vDest.fY = vStart.fY + ( vDirection.fY * fLength );

		VECTOR vDelta = vDest - pUnit->vPosition;

		// face dest
		if ( VectorIsZero( vDelta ) )
			vDirection = VECTOR ( 0, 1, 0 );
		else 
			VectorNormalize( vDirection, vDelta );

		float fNewAngle = acosf( PIN( -vDirection.fX, -1.0f, 1.0f ) );
		if ( vDirection.fY > 0.0f )
		{
			fNewAngle = -sgClientPlayerGlobals.fViewAngle;
		}
		fNewAngle += PI;
		c_PlayerSetAngles( pGame, fNewAngle, 0.0f );
	}
	else
	{

		// first calc an x,y location from the current mouse position projected into the scene at our Z + 1/2 our height
		//sgvPathDest.fZ = pUnit->vPosition.fZ;
		
		float fVanityHeight = ( bAimAtWeaponHeight ? UnitGetData( pUnit )->fVanityHeight : 0 ) * fabs( vDirection.fZ );
		
		float fLength = ( ( sgvPathDest.fZ + ( fVanityHeight ) ) - vStart.fZ ) / vDirection.fZ;

		sgvPathDest.fX = vStart.fX + ( vDirection.fX * fLength );
		sgvPathDest.fY = vStart.fY + ( vDirection.fY * fLength );
		sgvPathDest.fZ = pUnit->vPosition.fZ;

		// calc how far away that is from us
		VECTOR vTest = pUnit->vPosition;


		VECTOR vTestDir = sgvPathDest - vTest;
		vTestDir.fZ = 0;
		VectorNormalize( vTestDir, vTestDir );

		VECTOR vDelta = sgvPathDest - pUnit->vPosition;
		vDelta.fZ = 0.0f;

		// face dest
		if ( VectorIsZero( vDelta ) || sgfPathLastDist < .1f  )
			sgvPathDirection = VECTOR ( 0, 1, 0 );
		else 
			VectorNormalize( sgvPathDirection, vDelta );


		VectorZRotation( sgvPathDirection, DegreesToRadians( 90.0f ) );
		float fNewAngle = (float)atan2f( -sgvPathDirection.fX, sgvPathDirection.fY);

		c_PlayerSetAngles( pGame, fNewAngle );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCallbackPortalUseAccept( 
	void *pUserData, 
	DWORD dwCallbackData)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();

	// setup message
	MSG_CCMD_INTERACT msg;
	msg.idTarget = pGlobals->idLastInteractedUnit;
	msg.nInteract = (BYTE)UNIT_INTERACT_DEFAULT;

	// send to server
	c_SendMessage( CCMD_INTERACT, &msg );

	pGlobals->idLastInteractedUnit = INVALID_ID;
}
//----------------------------------------------------------------------------
//confirm you want to drop it.
//----------------------------------------------------------------------------
static void sCallbackDropQuestUnit( 
	void *pUserData, 
	DWORD dwCallbackData )
{
	
	UNIT *pTarget = (UNIT *)pUserData;
	ASSERTX_RETURN( pTarget, "Not target unit passed in." );
	c_ItemTryDrop( pTarget );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

WCHAR * c_PlayerGetNameByGuid(
	PGUID guid)
{
	ASSERT_RETNULL(guid != INVALID_GUID);

	UNIT *pUnit = UnitGetByGuid(AppGetCltGame(), guid);
	if (pUnit)
	{
		return (pUnit->szName);
	}

	WCHAR *szName = (WCHAR*)c_BuddyGetCharNameByGuid(guid);
	if (szName)
	{
		return szName;
	}

	szName = (WCHAR*)c_PlayerGetGuildMemberNameByID(guid);
	if (szName)
	{
		return szName;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PlayerGetByNameCBData
{
	const WCHAR *szName;
	UNIT *pPlayer;
};

static UNIT_ITERATE_RESULT sPlayerGetByNameCB( UNIT *pUnit, void *pUserData )
{
	PlayerGetByNameCBData *pData = (PlayerGetByNameCBData *)pUserData;
	WCHAR szName[MAX_CHARACTER_NAME];
	UnitGetName(pUnit, szName, MAX_CHARACTER_NAME, 0);
	if (UnitIsPlayer(pUnit) &&
		PStrICmp(szName, pData->szName, MAX_CHARACTER_NAME) == 0)
	{
		pData->pPlayer = pUnit;
		return UIR_STOP;
	}
	return UIR_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * PlayerGetByName( 
	GAME *pGame,
	const WCHAR *szName)
{
	ASSERT_RETNULL(pGame);
	ASSERT_RETNULL(szName);

	PlayerGetByNameCBData tData;
	tData.szName = szName;
	tData.pPlayer = NULL;

	// PlayersIterate only works on the server it seems
	UnitIterateUnits( 
		pGame, 
		sPlayerGetByNameCB,
		(void *)&tData);

	return tData.pPlayer;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sBuddyRemoveCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
	UIRemoveCurBuddy();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_PlayerWarpTo(
	PGUID guid)
{
	ASSERT_RETURN(guid != INVALID_GUID);

	if (c_PlayerCanWarpTo(guid))
	{
		// setup message
		MSG_CCMD_WARP_TO_PLAYER msg;
		msg.guidPlayerToWarpTo = guid;

		// send to server
		c_SendMessage( CCMD_WARP_TO_PLAYER, &msg );
	}
	else
	{
		c_WarpRestricted( WRR_TARGET_PLAYER_NOT_ALLOWED );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_PlayerInteract( 
	GAME *pGame, 
	UNIT *pTarget, 
	UNIT_INTERACT eInteract /*= UNIT_INTERACT_DEFAULT*/,
	BOOL bWarn /*= FALSE*/,
	PGUID guid /*=INVALID_GUID*/)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT( IS_CLIENT( pGame ) );
	ASSERTX_RETURN( eInteract > UNIT_INTERACT_INVALID && eInteract < NUM_UNIT_INTERACT, "Invalid interaction" );
	//	ASSERT( pTarget );
	InputSetInteracted();

	if (pTarget && UnitIsA(pTarget, UNITTYPE_ITEM) ) //make sure it's not a quest unit
	{
		switch (eInteract)
		{
		
			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMIDENTIFY:
			{
				c_ItemTryIdentify( pTarget, FALSE );
				return;
			}


			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMDISMANTLE:
			{
				c_ItemTryDismantle( pTarget, FALSE );
				return;
			}

			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMSPLIT:
			{
				UIShowStackSplitDialog( pTarget );
				return;
			}

			//----------------------------------------------------------------------------									
			case UNIT_INTERACT_ITEMEQUIP:
			{
				UIAutoEquipItem( pTarget, GameGetControlUnit(pGame) );
				return;
			}

			//----------------------------------------------------------------------------						
			case UNIT_INTERACT_ITEMDROP:
			{
				if( pTarget &&
					AppIsTugboat() &&
					 QuestUnitIsUnitType_QuestItem( pTarget ) &&
					 QuestUnitCanDropQuestItem( pTarget ) )
				{
					DIALOG_CALLBACK tCallbackOK;
					DialogCallbackInit( tCallbackOK );
					tCallbackOK.pfnCallback = sCallbackDropQuestUnit;
					tCallbackOK.pCallbackData = pTarget;
					UIShowGenericDialog( GlobalStringGet( GS_MENU_HEADER_WARNING ), StringTableGetStringByIndex( GlobalStringGetStringIndex( GS_DROP_AND_DESTROY_QUEST_ITEM ) ), TRUE, &tCallbackOK );
				}else{				
					UIDropItem(UnitGetId(pTarget));
				}
				return;
			}

			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMPICKCOLOR:
			{
				MSG_CCMD_PICK_COLORSET tMessage;
				tMessage.idItem = UnitGetId( pTarget );
				c_SendMessage( CCMD_PICK_COLORSET, &tMessage );
				break;
			}

			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMMOD:
			{
				UIPlaceMod(pTarget, NULL);
				break;
			}

			//----------------------------------------------------------------------------						
			case UNIT_INTERACT_ITEMUSE:
			{
				c_ItemUse(pTarget);
				break;
			}

			//----------------------------------------------------------------------------						
			case UNIT_INTERACT_ITEMSHOWINCHAT:
			{
				c_ItemShowInChat(pTarget);
				break;
			}

			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMBUY:
			{
				UIDoMerchantAction(pTarget);
				break;
			}

			//----------------------------------------------------------------------------		
			case UNIT_INTERACT_ITEMSELL:
			{
				UIDoMerchantAction(pTarget);
				break;
			}
					
			//----------------------------------------------------------------------------				
			default:
			{
				return;
			}
			
		}
	}
	else if (!pTarget && guid == INVALID_GUID)
	{
		switch (eInteract)
		{
			case UNIT_INTERACT_PLAYERBUDDYREMOVE:
			{
				UIShowConfirmDialog("InteractPlayerBuddyRemove", "ui buddylist remove instructions", sBuddyRemoveCallback);
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDKICK:
			{
				UIShowConfirmDialog("InteractPlayerGuildKick", "ui buddylist remove instructions", sGuildKickCallback);
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDPROMOTE:
			{
				ASSERT_RETURN(c_PlayerCanDoGuildAction(GUILD_ACTION_PROMOTE, c_PlayerGetGuildMemberRankByID(sGuildPromoteGuid)));
				if (sGuildPromoteRank == GUILD_RANK_LEADER)
				{
					UIShowConfirmDialog("InteractPlayerGuildPromote", "ui buddylist remove instructions", sGuildPromoteCallback);
				}
				else
				{
					sGuildPromoteCallback(NULL, NULL);
				}
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDDEMOTE:
			{
				ASSERT_RETURN(c_PlayerCanDoGuildAction(GUILD_ACTION_DEMOTE, c_PlayerGetGuildMemberRankByID(sGuildDemoteGuid)));
				UIShowConfirmDialog("InteractPlayerGuildDemote", "ui buddylist remove instructions", sGuildDemoteCallback);
				break;
			}
			default:
			{
				ASSERTX_RETURN(1, "c_PlayerInteract: No target and no GUID!  Nothing to do!");
				break;
			}
		};
	}
	else if (!pTarget)
	{
		// we have only a guid
		// this happens if the player right-clicked on a player in the buddy list or guild panel

		GAMECLIENT* pClient = UnitGetClient( GameGetControlUnit( pGame ) );
		switch (eInteract)
		{
			case UNIT_INTERACT_PLAYERWHISPER:
			{
				WCHAR *szName = c_PlayerGetNameByGuid(guid);
				ASSERT_RETURN(szName != NULL);
				c_PlayerSetupWhisperTo( szName );
				break;
			}
			case UNIT_INTERACT_PLAYERINVITE:
			{
				WCHAR *szName = c_PlayerGetNameByGuid(guid);
				ASSERT_RETURN(szName != NULL);
				c_PlayerInvite(szName);
				break;
			}
			case UNIT_INTERACT_PLAYERKICK:
			{
				if (c_PlayerGetPartyId() == INVALID_CHANNELID)
				{
					ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"You are not currently in a party.");
				}
				else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
				{
					ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"Only the leader of a party may remove party members.");
				}
				else
				{
					CHAT_REQUEST_MSG_REMOVE_PARTY_MEMBER removeMsg;
					WCHAR *szName = c_PlayerGetNameByGuid(guid);
					ASSERT_RETURN(szName != NULL);
					removeMsg.MemberToRemove = szName;

					c_ChatNetSendMessage(&removeMsg, USER_REQUEST_REMOVE_PARTY_MEMBER);
				}
				break;
			}
			case UNIT_INTERACT_PLAYERWARPTO:
			{
				if (c_PlayerFindPartyMember(guid) != INVALID_INDEX)
				{
					c_PartyMemberWarpTo(guid);
				}
				else
				{
					c_PlayerWarpTo(guid);
				}
				break;
			}
			case UNIT_INTERACT_PLAYERBUDDYADD:
			{
				WCHAR *szName = c_PlayerGetNameByGuid(guid);
				ASSERT_RETURN(szName != NULL);
				c_BuddyAdd( szName );
				break;
			}
			case UNIT_INTERACT_PLAYERBUDDYREMOVE:
			{
				UIShowConfirmDialog("InteractPlayerBuddyRemove", "ui buddylist remove instructions", sBuddyRemoveCallback);
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDINVITE:
			{
				sGuildInviteGuid = guid;
				UIShowConfirmDialog("InteractPlayerGuildInvite", "ui buddylist remove instructions", sGuildInviteCallback);
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDKICK:
			{
				sGuildKickGuid = guid;
				UIShowConfirmDialog("InteractPlayerGuildKick", "ui buddylist remove instructions", sGuildKickCallback);
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDPROMOTE:
			{
				ASSERT_RETURN(c_PlayerCanDoGuildAction(GUILD_ACTION_PROMOTE, c_PlayerGetGuildMemberRankByID(guid)));
				if (sGuildPromoteRank == GUILD_RANK_LEADER)
				{
					UIShowConfirmDialog("InteractPlayerGuildPromote", "ui buddylist remove instructions", sGuildPromoteCallback);
				}
				else
				{
					sGuildPromoteCallback(NULL, NULL);
				}
				break;
			}
			case UNIT_INTERACT_PLAYERGUILDDEMOTE:
			{
				ASSERT_RETURN(c_PlayerCanDoGuildAction(GUILD_ACTION_DEMOTE, c_PlayerGetGuildMemberRankByID(guid)));
				UIShowConfirmDialog("InteractPlayerGuildDemote", "ui buddylist remove instructions", sGuildDemoteCallback);
				break;
			}
		};
	}
	else
	{
		if (guid == INVALID_GUID)
		{
			float fDistance = ( AppIsHellgate() ) ? UnitsGetDistanceSquared( GameGetControlUnit( pGame ), pTarget ) : UnitsGetDistanceSquaredMinusRadiusXY( GameGetControlUnit( pGame ), pTarget );
			// Tug doesn't care how far away you are for invites - invite from anywhere in the level!
			if( AppIsTugboat() && ( 
				eInteract == UNIT_INTERACT_PLAYERINVITE ||
				eInteract == UNIT_INTERACT_PLAYERDISBAND ||
				eInteract == UNIT_INTERACT_PLAYERKICK ||
				eInteract == UNIT_INTERACT_PLAYERLEAVE ||
				eInteract == UNIT_INTERACT_ITEMDESTROY ))
			{
				fDistance = 0.0f;
			}
			const UNIT_DATA * pUnitData = UnitGetData(pTarget);
			if(pUnitData && UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IGNORE_INTERACT_DISTANCE))
			{
				fDistance = 0.0f;
			}
			if ( fDistance > ( UNIT_INTERACT_DISTANCE_SQUARED ) )
			{
				return;
			}
		}

		if (UnitIsPlayer(pTarget))
		{

			GAMECLIENT* pClient = UnitGetClient( GameGetControlUnit( pGame ) );
			switch (eInteract)
			{
			case UNIT_INTERACT_PLAYERTRADE:
				{
					c_TradeAskOtherToTrade( UnitGetId( pTarget ) );
				}
				break;
			case UNIT_INTERACT_PLAYERWHISPER:
				{
					c_PlayerSetupWhisperTo( pTarget );
				}
				break;
			case UNIT_INTERACT_PLAYERINVITE:
				{
					WCHAR szName[MAX_CHARACTER_NAME];
					if (UnitGetName(pTarget, szName, MAX_CHARACTER_NAME, 0))
					{
						c_PlayerInvite( szName );
					}
				}
				break;
			case UNIT_INTERACT_PLAYERKICK:
				{
					if (c_PlayerGetPartyId() == INVALID_CHANNELID)
					{
						ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"You are not currently in a party.");
					}
					else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
					{
						ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"Only the leader of a party may remove party members.");
					}
					else
					{
						CHAT_REQUEST_MSG_REMOVE_PARTY_MEMBER removeMsg;
						removeMsg.MemberToRemove = pTarget->szName;
						c_ChatNetSendMessage(&removeMsg, USER_REQUEST_REMOVE_PARTY_MEMBER);
					}
				}
				break;
			case UNIT_INTERACT_PLAYERDISBAND:
				{
					if (c_PlayerGetPartyId() == INVALID_CHANNELID)
					{
						ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"You are not currently in a party.");
					}
					else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
					{
						ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"Only the leader of a party may disband the party.");
					}
					else
					{
						CHAT_REQUEST_MSG_DISBAND_PARTY disbandMsg;
						c_ChatNetSendMessage(&disbandMsg, USER_REQUEST_DISBAND_PARTY);
					}
				}
				break;
			case UNIT_INTERACT_PLAYERLEAVE:
				{
					if (c_PlayerGetPartyId() == INVALID_CHANNELID)
					{
						ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"You are not currently in a party.");
					}
					else
					{
						CHAT_REQUEST_MSG_DISBAND_PARTY disbandMsg;
						c_ChatNetSendMessage(&disbandMsg, USER_REQUEST_LEAVE_PARTY);
					}
				}
				break;
			case UNIT_INTERACT_PLAYERWARPTO:
				{
					PGUID guid = UnitGetGuid(pTarget);
					if (c_PlayerFindPartyMember(guid) != INVALID_INDEX)
					{
						c_PartyMemberWarpTo(guid);
					}
					else
					{
						c_PlayerWarpTo(guid);
					}
					break;
				}
			case UNIT_INTERACT_PLAYERBUDDYADD:
				{
					WCHAR szName[MAX_CHARACTER_NAME];
					if (UnitGetName(pTarget, szName, MAX_CHARACTER_NAME, 0))
					{
						c_BuddyAdd( szName );
					}
					break;
				}
			case UNIT_INTERACT_PLAYERBUDDYREMOVE:
				{
					UIShowConfirmDialog("InteractPlayerBuddyRemove", "ui buddylist remove instructions", sBuddyRemoveCallback);
					break;
				}
			case UNIT_INTERACT_PLAYERGUILDINVITE:
				{
					sGuildInviteGuid = UnitGetGuid( pTarget );
					UIShowConfirmDialog("InteractPlayerGuildInvite", "ui buddylist remove instructions", sGuildInviteCallback);
					break;
				}
			case UNIT_INTERACT_PLAYERGUILDKICK:
				{
					sGuildKickGuid = UnitGetGuid( pTarget );
					UIShowConfirmDialog("InteractPlayerGuildKick", "ui buddylist remove instructions", sGuildKickCallback);
					break;
				}
			case UNIT_INTERACT_PLAYERGUILDPROMOTE:
				{
					ASSERT_RETURN(c_PlayerCanDoGuildAction(GUILD_ACTION_PROMOTE, c_PlayerGetGuildMemberRankByID(UnitGetGuid(pTarget))));
					if (sGuildPromoteRank == GUILD_RANK_LEADER)
					{
						UIShowConfirmDialog("InteractPlayerGuildPromote", "ui buddylist remove instructions", sGuildPromoteCallback);
					}
					else
					{
						sGuildPromoteCallback(NULL, NULL);
					}
					break;
				}
			case UNIT_INTERACT_PLAYERGUILDDEMOTE:
				{
					ASSERT_RETURN(c_PlayerCanDoGuildAction(GUILD_ACTION_DEMOTE, c_PlayerGetGuildMemberRankByID(UnitGetGuid(pTarget))));
					UIShowConfirmDialog("InteractPlayerGuildDemote", "ui buddylist remove instructions", sGuildDemoteCallback);
					break;
				}
			case UNIT_INTERACT_PLAYERINVITEDUEL:
				{
					c_DuelInviteAttempt( pTarget );
					break;
				}
			case UNIT_INTERACT_PLAYERINSPECT:
				{
					MSG_CCMD_INSPECT_PLAYER  tMessage;
					tMessage.idToInspect = UnitGetId(pTarget);
					c_SendMessage(CCMD_INSPECT_PLAYER, &tMessage);
					break;
				}
			}
		}
		else 
		{
			// in Tug, we let you visually open something before it happens
			// on the server, for a more immediate reaction, like barrels/etc.

			// send a message to the server to interact with this NPC
			ASSERT( pTarget && UnitCanInteractWith( pTarget, GameGetControlUnit( pGame ) ) );
			
			if( AppIsTugboat() && UnitIsA( pTarget, UNITTYPE_OPENABLE ) && ObjectRequiresItemToOperation( pTarget ) == FALSE )
			{

				if( UnitDataTestFlag(pTarget->pUnitData, UNIT_DATA_FLAG_DIE_ON_CLIENT_TRIGGER))
				{
					BOOL bHasMode;
					float fDyingDurationInTicks = UnitGetModeDuration( pGame, pTarget, MODE_DYING, bHasMode );
					if( bHasMode )
					{
						c_UnitSetMode( pTarget, MODE_DYING, 0, fDyingDurationInTicks );
						UnitSetFlag(pTarget, UNITFLAG_DEAD_OR_DYING);
						UnitChangeTargetType(pTarget, TARGET_DEAD);
					}
				}

			}
			/*
			if( AppIsTugboat() && bWarn && ( UnitIsA( pTarget, UNITTYPE_STAIRS_DOWN ) ||
											 UnitIsA( pTarget, UNITTYPE_STAIRS_UP ) ) )
			{
				CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
				pGlobals->idLastInteractedUnit = UnitGetId( pTarget );
				// setup callback
				DIALOG_CALLBACK tCallbackOK;
				DialogCallbackInit( tCallbackOK );
				tCallbackOK.pfnCallback = sCallbackPortalUseAccept;
				LEVEL *pLevel = UnitGetLevel( pTarget );
				if( pLevel &&
					pLevel->m_pLevelDefinition )
				{
					if( UnitIsA( pTarget, UNITTYPE_STAIRS_DOWN ) )
					{
						if( pLevel->m_pLevelDefinition->nStringEnter != INVALID_ID )
						{
							UIShowGenericDialog( GlobalStringGet(GS_MENU_HEADER_EXIT_MAP), StringTableGetStringByIndex( pLevel->m_pLevelDefinition->nStringEnter ), TRUE, &tCallbackOK );
						}
					}
					else
					{
						if( pLevel->m_pLevelDefinition->nStringLeave != INVALID_ID )
						{
							UIShowGenericDialog( GlobalStringGet(GS_MENU_HEADER_EXIT_MAP), StringTableGetStringByIndex( pLevel->m_pLevelDefinition->nStringLeave ), TRUE, &tCallbackOK );
						}
					}
				}
				return;
			}*/
			// setup message
			MSG_CCMD_INTERACT msg;
			msg.idTarget = UnitGetId( pTarget );
			msg.nInteract = (BYTE)eInteract;

			// send to server
			c_SendMessage( CCMD_INTERACT, &msg );
		}

		
	}
#endif	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL PlayerIsInAttackRange(
	GAME *pGame,
	UNIT *pTarget,
	int nSkill,
	BOOL bLineOfSight )
{
	UNIT * pUnit = GameGetControlUnit( pGame );
	return IsInAttackRange( pUnit, pGame, pTarget, nSkill, bLineOfSight );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_PlayerSetActiveSkill( GAME * game, int nSkillStat, char * pszName )
{
	ASSERT( IS_CLIENT( game ) );

	UNIT * unit = GameGetControlUnit( game );
	if ( ! unit )
		return;

	int nSkill = ExcelGetLineByStringIndex( game, DATATABLE_SKILLS, pszName );
	UnitSetStat( unit, nSkillStat, nSkill );
	ASSERT( UnitGetStat( unit, nSkillStat ) == nSkill );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerInitControlUnit(
	UNIT * unit)
{
	GAME * pGame = UnitGetGame(unit);
	ASSERT_RETURN(IS_CLIENT(pGame));


	// Fly-through camera demo mode
	if ( AppCommonGetDemoMode() )
	{
		V( DemoLevelGeneratePath() );
		c_CameraSetViewType( VIEW_DEMOLEVEL );
		UICloseAll();

		//int nModelID = c_UnitGetModelIdThirdPerson( unit );
		////e_ModelSetDrawAndShadow( nModelID, FALSE );
		//e_ModelSetFlagbit( nModelID, MODEL_FLAGBIT_NODRAW, TRUE );

		return;
	}

	ASSERT_RETURN(unit->pGfx);

	if (unit->pGfx->nModelIdThirdPerson != INVALID_ID)
	{
		e_ModelSetFlagbit( unit->pGfx->nModelIdThirdPerson, MODEL_FLAGBIT_ARTICULATED_SHADOW, TRUE );
	}


	if(AppIsHellgate())
	{
		if (unit->pGfx->nModelIdFirstPerson != INVALID_ID)
		{
			return; // we are just here to set up the first person model
		}
	}

	int nClass = UnitGetClass(unit);
	
	const UNIT_DATA * pUnitData = PlayerGetData(pGame, nClass);
	ASSERT_RETURN(pUnitData);

	if(AppIsHellgate())
	{
		unit->pGfx->nModelIdFirstPerson = c_AppearanceNew( pUnitData->nAppearanceDefId_FirstPerson, APPEARANCE_NEW_FLAG_FORCE_WARDROBE | APPEARANCE_NEW_FIRSTPERSON_WARDROBE_APPEARANCE_GROUP, unit, pUnitData );
		e_ModelSetScale( unit->pGfx->nModelIdFirstPerson, 0.5f );
		int nWardrobeFirst = c_AppearanceGetWardrobe( c_UnitGetModelIdFirstPerson( unit ) );
		ASSERT_RETURN( nWardrobeFirst != INVALID_ID );
		c_WardrobeUpgradeCostumeCount( nWardrobeFirst, TRUE );
		c_WardrobeUpgradeLOD( nWardrobeFirst ); // otherwise they will be low LOD
	}

	int nWardrobeThird = c_AppearanceGetWardrobe( c_UnitGetModelIdThirdPerson( unit ) );
	ASSERT_RETURN( nWardrobeThird != INVALID_ID );
	c_WardrobeUpgradeCostumeCount( nWardrobeThird, TRUE );
	c_WardrobeUpgradeLOD( nWardrobeThird ); // otherwise they will be low LOD

	c_UnitCreatePaperdollModel(unit, APPEARANCE_NEW_FORCE_ANIMATE);

	if(AppIsTugboat())
	{
		c_CameraInitViewType( VIEW_3RDPERSON );
	}
	else
	{
		int nViewType = c_CameraGetViewType();
		if ( nViewType != VIEW_1STPERSON && nViewType != VIEW_3RDPERSON )
		{
			if ( UnitIsA( unit, UNITTYPE_HUNTER ) )
			{
				c_CameraInitViewType( VIEW_1STPERSON );
			}
			else
			{
				c_CameraInitViewType( VIEW_3RDPERSON );
			}
		}
	}


	e_ModelSetDrawAndShadow(unit->pGfx->nModelIdFirstPerson, FALSE);
	if( AppIsTugboat() )
	{
		ROOM* pRoom = UnitGetRoom(unit);
		if(pRoom)
		{
			c_TaskUpdateNPCs( pGame, pRoom);
		}

		LEVEL * pLevel = UnitGetLevel( unit );
		if(pLevel)
		{
			cLevelUpdateUnitsVisByThemes( pLevel );
		}
	}

}
#endif //!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppGetDetachedCamera(
	void)
{
	return sgClientPlayerGlobals.bDetachedCamera;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppResetDetachedCamera(
	void)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return;
	}
	UNIT* unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}
	sgClientPlayerGlobals.vDetachedCameraPosition = UnitGetPosition(unit);
	sgClientPlayerGlobals.nDetachedCameraViewAngle = (INT) (sgClientPlayerGlobals.fViewAngle * (180.0f / PI));
	sgClientPlayerGlobals.nDetachedCameraViewPitch = (INT) (sgClientPlayerGlobals.fViewPitch * (180.0f / PI));
	sgClientPlayerGlobals.fDetachedCameraViewAngle = sgClientPlayerGlobals.fViewAngle;
	sgClientPlayerGlobals.fDetachedCameraViewPitch = sgClientPlayerGlobals.fViewPitch;
	sgClientPlayerGlobals.nDetachedCameraMovement = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetDetachedCamera(
	BOOL value)
{
	if (value == sgClientPlayerGlobals.bDetachedCamera)
	{
		return;
	}
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return;
	}
	UNIT* unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}
	sgClientPlayerGlobals.bDetachedCamera = value;
	if (value)
	{
		AppResetDetachedCamera();
	}
	else
	{
		sgClientPlayerGlobals.vDetachedCameraPosition = UnitGetPosition(unit);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppGetCameraPosition(
	VECTOR* pvPosition,
	float* pfAngle,
	float* pfPitch)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return FALSE;
	}
	UNIT * camera_unit = GameGetCameraUnit( game );
	if ( camera_unit && ( game->pControlUnit != camera_unit ) )
	{
		GameGetControlPosition( game, NULL, NULL, pfPitch );
		if ( pvPosition )
		{
			*pvPosition = camera_unit->vPosition;
		}
		if ( pfAngle )
		{
			*pfAngle = VectorDirectionToAngle( UnitGetFaceDirection( camera_unit, TRUE ) );
		}
		return TRUE;
	}
	else if ( AppGetDetachedCamera() )
	{
		*pvPosition = sgClientPlayerGlobals.vDetachedCameraPosition;
		*pfAngle = sgClientPlayerGlobals.fDetachedCameraViewAngle;
		*pfPitch = sgClientPlayerGlobals.fDetachedCameraViewPitch;
		return TRUE;
	}
	else
	{
		return GameGetControlPosition(game, pvPosition, pfAngle, pfPitch);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDetachedCameraMouseRotate( 
	int delta)
{
	if (!AppGetDetachedCamera())
	{
		return;
	}
	if (delta == 0)
	{
		return;
	}
	float fDelta = (float)delta * ROTATION_PITCH_ANGLE_DELTA;
	sgClientPlayerGlobals.fDetachedCameraViewAngle += fDelta;
	sgClientPlayerGlobals.fDetachedCameraViewAngle = NORMALIZE_EX(sgClientPlayerGlobals.fDetachedCameraViewAngle, MAX_RADIAN);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDetachedCameraMousePitch( 
	int delta)
{
	if (!AppGetDetachedCamera())
	{
		return;
	}
	if (delta == 0)
	{
		return;
	}
	float fDelta = (float)delta * (PI / (180.f * 6.0f));
	sgClientPlayerGlobals.fDetachedCameraViewPitch += fDelta;
	sgClientPlayerGlobals.fDetachedCameraViewPitch = NORMALIZE_EX(sgClientPlayerGlobals.fDetachedCameraViewPitch, MAX_RADIAN);
	if (sgClientPlayerGlobals.fDetachedCameraViewPitch > PI && sgClientPlayerGlobals.fDetachedCameraViewPitch < (((360 - MAX_PLAYER_PITCH) * PI) / 180.0f))
	{
		sgClientPlayerGlobals.fDetachedCameraViewPitch = ((360 - MAX_PLAYER_PITCH) * PI) / 180.0f;
	}
	else if (sgClientPlayerGlobals.fDetachedCameraViewPitch < PI && sgClientPlayerGlobals.fDetachedCameraViewPitch > ((MAX_PLAYER_PITCH * PI) / 180.0f))
	{
		sgClientPlayerGlobals.fDetachedCameraViewPitch = (MAX_PLAYER_PITCH * PI) / 180.0f;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDetachedCameraSetMovement(
	int movementflag,
	BOOL value)
{
	if (!AppGetDetachedCamera())
	{
		return;
	}
#if (ISVERSION(DEVELOPMENT))
	if (value)
	{
		sgClientPlayerGlobals.nDetachedCameraMovement |= movementflag;
		switch (movementflag)
		{
		case PLAYER_STRAFE_LEFT:
			sgClientPlayerGlobals.nDetachedCameraMovement &= ~PLAYER_STRAFE_RIGHT;
			break;
		case PLAYER_STRAFE_RIGHT:
			sgClientPlayerGlobals.nDetachedCameraMovement &= ~PLAYER_STRAFE_LEFT;
			break;
		case PLAYER_MOVE_FORWARD:
			sgClientPlayerGlobals.nDetachedCameraMovement &= ~PLAYER_MOVE_BACK;
			break;
		case PLAYER_MOVE_BACK:
			sgClientPlayerGlobals.nDetachedCameraMovement &= ~PLAYER_MOVE_FORWARD;
			break;
		case CAMERA_MOVE_UP:
			sgClientPlayerGlobals.nDetachedCameraMovement &= ~CAMERA_MOVE_DOWN;
			break;
		case CAMERA_MOVE_DOWN:
			sgClientPlayerGlobals.nDetachedCameraMovement &= ~CAMERA_MOVE_UP;
			break;
		}
	}
	else
	{
		sgClientPlayerGlobals.nDetachedCameraMovement &= ~movementflag;
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDetachedCameraClearAllMovement()
{
	sgClientPlayerGlobals.nDetachedCameraMovement = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDetachedCameraUpdate(
	void)
{
#if (ISVERSION(DEVELOPMENT))
	if (!AppGetDetachedCamera())
	{
		return;
	}
	static const float dirtoangle[] =
	{
		-PI/4.0f, 0.0f, PI/4.0f, -PI/2.0f, 0, PI/2.0f, -((PI/4.0f)+(PI/2.0f)), PI, (PI/4.0f)+(PI/2.0f)
	};

	BOOL bMoving = sgClientPlayerGlobals.nDetachedCameraMovement & (PLAYER_MOVE_FORWARD | PLAYER_STRAFE_LEFT | PLAYER_STRAFE_RIGHT | PLAYER_MOVE_BACK | CAMERA_MOVE_UP | CAMERA_MOVE_DOWN);
	if (!bMoving)
	{
		return;
	}

	VECTOR vDirection(0.f, 0.f, 0.f);

	if (sgClientPlayerGlobals.nDetachedCameraMovement & (PLAYER_MOVE_FORWARD | PLAYER_STRAFE_LEFT | PLAYER_STRAFE_RIGHT | PLAYER_MOVE_BACK))
	{
		int dir = 4;
		if (sgClientPlayerGlobals.nDetachedCameraMovement & PLAYER_MOVE_FORWARD)
		{
			dir -= 3;
		} 
		if (sgClientPlayerGlobals.nDetachedCameraMovement & PLAYER_STRAFE_LEFT)
		{
			dir -= 1;
		} 
		if (sgClientPlayerGlobals.nDetachedCameraMovement & PLAYER_STRAFE_RIGHT)
		{
			dir += 1;
		} 
		if (sgClientPlayerGlobals.nDetachedCameraMovement & PLAYER_MOVE_BACK)
		{
			dir += 3;
		} 
		VectorDirectionFromAngleRadians(vDirection, sgClientPlayerGlobals.fDetachedCameraViewAngle + dirtoangle[dir]);
	}

	if (sgClientPlayerGlobals.nDetachedCameraMovement & CAMERA_MOVE_UP)
	{
		vDirection.fZ = 1.0f;
	}
	else if (sgClientPlayerGlobals.nDetachedCameraMovement & CAMERA_MOVE_DOWN)
	{
		vDirection.fZ = -1.0f;
	}
	VectorNormalize(vDirection);

	int elapsed_delta = (int)(AppCommonGetAbsTime() - sgClientPlayerGlobals.tiDetachedCameraLastMovement);
	if (elapsed_delta > MSECS_PER_SEC / 20)
	{
		elapsed_delta = MSECS_PER_SEC / 20;
	}
	float elapsed_time = float(double(elapsed_delta) / double(MSECS_PER_SEC));
	sgClientPlayerGlobals.tiDetachedCameraLastMovement = AppCommonGetAbsTime();

	vDirection *= 5.0f * elapsed_time;
	VectorAdd(sgClientPlayerGlobals.vDetachedCameraPosition, sgClientPlayerGlobals.vDetachedCameraPosition, vDirection);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerRemoveItemFromWeaponConfigs(
	UNIT *pOwner,
	UNIT *pItem,
	int  nHotkey /*= -1*/)
{
	ASSERT_RETURN(IS_SERVER(UnitGetGame(pOwner)));
	ASSERT_RETURN(UnitIsA(pOwner, UNITTYPE_PLAYER));

	UNITID idItem = UnitGetId(pItem);
	for (int iSelector = TAG_SELECTOR_WEAPCONFIG_HOTKEY1; iSelector <= TAG_SELECTOR_WEAPCONFIG_HOTKEY_LAST; iSelector++)
	{
		if (nHotkey == -1 || iSelector == nHotkey)
		{
			UNIT_TAG_HOTKEY *pTag = UnitGetHotkeyTag(pOwner, iSelector);
			BOOL bFound = FALSE;
			if (pTag)
			{
				int i=0;
				for (; i<MAX_HOTKEY_ITEMS; i++)
				{
					if (pTag->m_idItem[i] == idItem)
					{
						pTag->m_idItem[i] = INVALID_ID;
						bFound = TRUE;
						break;
					}
				}

				if (bFound)
				{
					s_UnitSetWeaponConfigChanged( pOwner, TRUE );
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerDeleteOldVersions(
	void)
{
	const OS_PATH_CHAR * pSavePath = FilePath_GetSystemPath(FILE_PATH_SAVE);
	int nNumDeleted = 0;
	OS_PATH_CHAR szFileSearchName[MAX_PATH];
	PStrPrintf(szFileSearchName, _countof(szFileSearchName), OS_PATH_TEXT("%s*.hg1"), pSavePath);
	OS_PATH_FUNC(WIN32_FIND_DATA) finddata;
	FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)( szFileSearchName, &finddata);
	if ( handle != INVALID_HANDLE_VALUE )
	{
		do 
		{
			OS_PATH_CHAR szFilename[MAX_PATH];
			PStrPrintf(szFilename, _countof(szFilename), OS_PATH_TEXT("%s%s"), pSavePath, finddata.cFileName);
			CLIENT_SAVE_FILE tClientSaveFile;
			BYTE * pRichSaveHeaderPoint = (BYTE*) FileLoad(MEMORY_FUNC_FILELINE(NULL, szFilename, &tClientSaveFile.dwBufferSize));
			tClientSaveFile.pBuffer = RichSaveFileRemoveHeader( pRichSaveHeaderPoint, &tClientSaveFile.dwBufferSize);
			if (!tClientSaveFile.pBuffer)
			{
				FileDelete(finddata.cFileName);
			}
			else
			{
				if (!UnitFileVersionCanBeLoaded(&tClientSaveFile))
				{
					FileDelete(szFilename);
					nNumDeleted++;
				}
				FREE(NULL, pRichSaveHeaderPoint);
//				FREE(NULL, tClientSaveFile.pBuffer);
			}
		}
		while (OS_PATH_FUNC(FindNextFile)( handle, &finddata ));
	}

	return nNumDeleted;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID LocalPlayerGetID(
	void)
{	
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETNULL( pGame, "Expected game" );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETNULL( pPlayer, "Expected player" );
	return UnitGetId( pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *LocalPlayerGet(
	void)
{	
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETNULL( pGame, "Expected game" );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETNULL( pPlayer, "Expected player" );
	return pPlayer;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsCrouching(
	UNIT * player )
{
	if ( ! player )
		return FALSE;
	return UnitHasState( UnitGetGame( player ), player, STATE_CROUCH );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerSetHeadstoneLevelDef(
	UNIT *pUnit,
	int nLevelDef)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UnitSetStat( pUnit, STATS_HEADSTONE_LEVEL_DEF, nLevelDef );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerGetLastHeadstoneLevelDef(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	return UnitGetStat( pUnit, STATS_HEADSTONE_LEVEL_DEF );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerHasHeadstone( 
	UNIT *pUnit)
{
	int nLevelDef = PlayerGetLastHeadstoneLevelDef( pUnit );
	return nLevelDef != INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency PlayerGetResurrectCost(
	UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	
	// for now, we base this on the headstone cost
	int nExperienceLevel = UnitGetExperienceLevel( pUnit );
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	const PLAYERLEVEL_DATA *pLevelData = PlayerLevelGetData( NULL, pUnitData->nUnitTypeForLeveling, nExperienceLevel );
	return cCurrency( pLevelData->nHeadstoneReturnCost, 0 );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency PlayerGetReturnToHeadstoneCost( 
	UNIT *pUnit)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	ASSERTX_RETZERO( PlayerHasHeadstone( pUnit ), "Player has no headstone" );
	
	// for now, we base this on the players experience level
	int nExperienceLevel = UnitGetExperienceLevel( pUnit );
	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	const PLAYERLEVEL_DATA *pLevelData = PlayerLevelGetData( NULL, pUnitData->nUnitTypeForLeveling, nExperienceLevel );
	return cCurrency( pLevelData->nHeadstoneReturnCost, 0 );
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency PlayerGetReturnToDungeonCost( 
	UNIT *pUnit)
{
	int nDepthDeepest = UnitGetStat( pUnit, STATS_LEVEL_DEPTH ) - LevelGetDepth( UnitGetLevel( pUnit ) );

	return cCurrency( nDepthDeepest * (2 * MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( UnitGetLevelAreaID( pUnit ) ) ), 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency PlayerGetGuildCreationCost( 
	UNIT *pUnit)
{

	return cCurrency( GUILD_COST, 0 );
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
cCurrency PlayerGetRespecCost( 
	UNIT *pUnit,
	BOOL bCrafting )
{
	if( pUnit )
	{
		if( UnitGetStat( pUnit, STATS_LEVEL ) > 5 )
		{
			int nRespecs = bCrafting ? UnitGetStat( pUnit, STATS_RESPECSCRAFTING ) : UnitGetStat( pUnit, STATS_RESPECS );
			int nValue = 1;
			for( int i = 0; i < nRespecs; i++ )
			{
				nValue *= 2;
			}
			nValue *= RESPEC_COST;
			nValue = MIN( MAX_RESPEC_COST, nValue );
			return cCurrency( nValue, 0 );
		}
		else
		{
			return cCurrency( 0, 0 );
		}
	}
	return cCurrency( RESPEC_COST, 0 );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerSendToCorpse( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( PlayerHasHeadstone( pUnit ), "Player must have headstone" );
	
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = PlayerGetLastHeadstoneLevelDef( pUnit );
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_HEADSTONE_BIT );
	
	// do warp
	s_WarpToLevelOrInstance( pUnit, tSpec );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerTryReturnToHeadstone( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	BOOL bCanReturnToHeadstone = FALSE;
	
	// must have a headstone
	if (PlayerHasHeadstone( pUnit ))
	{
	
		// player can return to headstone
		bCanReturnToHeadstone = TRUE;
		
		// how much does it cost to go back there
		cCurrency Cost = PlayerGetReturnToHeadstoneCost( pUnit );

		// player must have enough money
		if ( UnitGetCurrency( pUnit ) >= Cost)
		{
		
			// take the money
			ASSERT_RETVAL(UnitRemoveCurrencyValidated( pUnit, Cost ), 
				bCanReturnToHeadstone );
			
			// transport them
			sPlayerSendToCorpse( pUnit );
						
		}
				
	}
	
	return bCanReturnToHeadstone;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *PlayerGetMostRecentHeadstone(
	UNIT *pPlayer)
{
	ASSERTX_RETNULL( pPlayer, "Expected unit" );
	ASSERTX_RETNULL( UnitIsPlayer( pPlayer ), "Expected player" );

	// get headstones
	UNIT *pHeadstones[ MAX_HEADSTONES ];
	int nNumHeadstones = PlayerGetHeadstones( pPlayer, pHeadstones, MAX_HEADSTONES );

	// find most recent one	
	UNIT *pHeadstoneLast = NULL;
	int nHeadstoneCreationTick = 0;
	for (int i = 0; i < nNumHeadstones; ++i)
	{
		UNIT *pHeadstone = pHeadstones[ i ];
		int nCreateTick = UnitGetStat( pHeadstone, STATS_SPAWN_TICK );
		if (pHeadstoneLast == NULL || nCreateTick > nHeadstoneCreationTick )
		{
			pHeadstoneLast = pHeadstone;
			nHeadstoneCreationTick = nCreateTick;
		}		
	}
	
	return pHeadstoneLast;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerGetHeadstones(	
	UNIT *pUnit, 
	UNIT **pHeadstoneBuffer, 
	int nMaxHeadstones)
{
	ASSERTX_RETZERO( pUnit, "Expected unit" );
	ASSERTX_RETZERO( pHeadstoneBuffer, "Expected headstone buffer" );
	ASSERTX_RETZERO( nMaxHeadstones > 0, "Invalid headstone buffer size" );
	GAME *pGame = UnitGetGame( pUnit );
	int nNumHeadstones = 0;
		
	// go through all levels
	for (LEVEL *pLevel = DungeonGetFirstPopulatedLevel( pGame ); 
		 pLevel;
		 pLevel = DungeonGetNextPopulatedLevel( pLevel ))
	{
	
		// check all rooms on this level
		for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{
			// search all objects
			for (UNIT *pObject = pRoom->tUnitList.ppFirst[ TARGET_OBJECT ]; pObject; pObject = pObject->tRoomNode.pNext)
			{
			
				// is this a headstone
				if (ObjectIsHeadstone( pObject, pUnit, NULL ))
				{
					ASSERTX_RETZERO( nNumHeadstones <= nMaxHeadstones - 1, "Too many headstones found for player" );
					pHeadstoneBuffer[ nNumHeadstones++ ] = pObject;
				}
										
			}	
				
		}	

	}
		
	return nNumHeadstones;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetHeadstoneLocation( 
	UNIT *pUnit, 
	ROOM *&pHeadstoneRoom, 
	VECTOR &vHeadstonePos, 
	VECTOR &vHeadstoneDir)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	SUBLEVEL *pSubLevel = UnitGetSubLevel( pUnit );
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );
	UNIT *pLocationUnit = pUnit;  // default to player unit
		
	// dying in some sublevels will create the headstone at the entrance to the sublevel
	const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
	ASSERTX_RETURN( pSubLevelDef, "Expected sublevel def" );
	if (pSubLevelDef->bHeadstoneAtEntranceObject == TRUE)
	{
		
		// find the sublevel entrance object
		SUBLEVELID idSubLevel = SubLevelGetId( pSubLevel );	
		SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
		const SUBLEVEL_DOORWAY *pSubLevelEntrance = s_SubLevelGetDoorway( pSubLevel, SD_ENTRANCE );
		ASSERTX( pSubLevelEntrance, "Expected sublevel exit" );		
		if (pSubLevelEntrance)
		{
			UNIT *pEntrance = UnitGetById( pGame, pSubLevelEntrance->idDoorway );
			if (pEntrance)
			{
				pLocationUnit = pEntrance;
			}
			else
			{
				if (pSubLevelEntrance->idRoom != INVALID_ID)
				{
					
					// don't use the default location unit set so far
					pLocationUnit = NULL;
					
					// use the last known good sublevel entrnace
					pHeadstoneRoom = RoomGetByID( pGame, pSubLevelEntrance->idRoom );
					vHeadstoneDir = pSubLevelEntrance->vDirection;
					vHeadstonePos = pSubLevelEntrance->vPosition;
					
				}
				
			}

		}
				
	}

	// given the location unit, use its position
	if (pLocationUnit)
	{
		ROOM *pRoom = UnitGetRoom( pLocationUnit );
		const VECTOR &vPosition = UnitGetPosition( pLocationUnit );
	
		// find nearest path node to this unit
		DWORD dwNearestPathNodeFlags = 0;
		SETBIT( dwNearestPathNodeFlags, NPN_ALLOW_OCCUPIED_NODES );
		SETBIT( dwNearestPathNodeFlags, NPN_USE_XY_DISTANCE );
		SETBIT( dwNearestPathNodeFlags, NPN_CHECK_LOS );		
		SETBIT( dwNearestPathNodeFlags, NPN_DONT_CHECK_HEIGHT );
		SETBIT( dwNearestPathNodeFlags, NPN_DONT_CHECK_RADIUS );
		SETBIT( dwNearestPathNodeFlags, NPN_EMPTY_OUTPUT_IS_OKAY );

		//SETBIT( dwNearestPathNodeFlags, NPN_CHECK_LOS_AGAINST_OBJECTS );	
		ROOM_PATH_NODE *pPathNode = RoomGetNearestPathNode( pGame, pRoom, vPosition, &pHeadstoneRoom, dwNearestPathNodeFlags );
		if (pPathNode)
		{
		
			vHeadstonePos = RoomPathNodeGetWorldPosition( pGame, pHeadstoneRoom, pPathNode );
			
		}
		else
		{
		
			// no path node found, use unit position
			pHeadstoneRoom = UnitGetRoom( pLocationUnit );
			vHeadstonePos = UnitGetPosition( pLocationUnit );

			// force the headstone to be on the floor	
			LevelProjectToFloor( pLevel, vHeadstonePos, vHeadstonePos );
		
		}

		// headstone direction
		vHeadstoneDir = UnitGetFaceDirection( pLocationUnit, FALSE );
					
	}
			
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreateHeadstone(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// don't drop headstones in arena games
	if ( s_QuestGamePlayerArenaRespawn( pUnit ) )
		return;

	// find all the headstones that this player has created
	UNIT *pHeadstones[ MAX_HEADSTONES ];
	int nNumHeadstones = PlayerGetHeadstones( pUnit, pHeadstones, MAX_HEADSTONES );

	// remove all the old headstones
	for (int i = 0; i < nNumHeadstones; ++i)
	{
		UNIT *pHeadstone = pHeadstones[ i ];
		ASSERTX_CONTINUE( pHeadstone, "Expected headstone" );
		UnitFree( pHeadstone, UFF_SEND_TO_CLIENTS );
		pHeadstones[ i ] = NULL;
	}
	nNumHeadstones = 0;
		
	// get headstone position
	ROOM *pHeadstoneRoom = NULL;
	VECTOR vHeadstonePos;
	VECTOR vHeadstoneDir;
	sGetHeadstoneLocation( pUnit, pHeadstoneRoom, vHeadstonePos, vHeadstoneDir );
					
	// spawn new headstone 
	VECTOR vDirection = UnitGetFaceDirection( pUnit, FALSE );
	OBJECT_SPEC tSpec;
	tSpec.nClass = GlobalIndexGet( GI_OBJECT_HEADSTONE );
	tSpec.pRoom = pHeadstoneRoom;
	tSpec.vPosition = vHeadstonePos;
	tSpec.pvFaceDirection = &vHeadstoneDir;
	
	// copy name of player to headstone
	WCHAR uszName[ MAX_CHARACTER_NAME ];
	DWORD dwUnitNameFlags = 0;
	SETBIT( dwUnitNameFlags, UNF_BASE_NAME_ONLY_BIT );
	SETBIT( dwUnitNameFlags, UNF_NAME_NO_MODIFICATIONS );
	UnitGetName( pUnit, uszName, MAX_CHARACTER_NAME, dwUnitNameFlags );	
	tSpec.puszName = uszName;

	// create the headstone object	
	UNIT *pHeadstone = s_ObjectSpawn( pGame, tSpec );
			
	// set the owner of this headstone
	UnitSetGUIDOwner( pHeadstone, pUnit );
	
	// mark this headstone as the newest one
	UnitSetStat( pHeadstone, STATS_NEWEST_HEADSTONE_FLAG, TRUE );
	
	// set the level the most recent headstone is on
	PlayerSetHeadstoneLevelDef( pUnit, UnitGetLevelDefinitionIndex( pHeadstone ) );
	
	// set unit id of most recent headstone
	UnitSetStat( pUnit, STATS_NEWEST_HEADSTONE_UNIT_ID, UnitGetId( pHeadstone ) );
	
	// set headstone as hidden
	//s_StateSet( pHeadstone, pHeadstone, STATE_DONT_DRAW_QUICK, 0 );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sShowHeadstone(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// get most recent headstone
	UNITID idLastHeadstone = UnitGetStat( pPlayer, STATS_NEWEST_HEADSTONE_UNIT_ID );
	UNIT *pLastHeadstone = UnitGetById( pGame, idLastHeadstone );
	ASSERTX_RETURN( pLastHeadstone, "Expected headstone" );

	// clear the hidden state
	s_StateClear( pLastHeadstone, UnitGetId( pLastHeadstone ), STATE_DONT_DRAW_QUICK, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerCanRespawnAsGhost(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

	// if can't restart at all, can't restart as a ghost!
	if (IsUnitDeadOrDying( pPlayer ) == FALSE)
	{
		return FALSE;
	}
			
	// hardcore players can't respawn as ghosts
	if (PlayerIsHardcore( pPlayer ) &&
		!UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_NO_MAJOR_DEATH_PENALTIES ) )
	{
		return FALSE;
	}

	if ( PlayerIsElite( pPlayer ) )
	{
		return FALSE;
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerCanResurrect(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

	// if can't restart, can't possibly resurrect
	if (IsUnitDeadOrDying( pPlayer ) == FALSE)
	{
		return FALSE;
	}
		
	// hardcore players cannot resurrect
	if (PlayerIsHardcore( pPlayer ) &&
		!UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_NO_MAJOR_DEATH_PENALTIES ) )
	{
		return FALSE;
	}

	if ( AppIsHellgate() &&
		PlayerIsElite( pPlayer ) )		//tugboat elite players can resurrect
	{
		return FALSE;
	}
		
	// must have enough money	
	// Hellgate only! Rrrrrr.
	if ( AppIsHellgate() &&
		UnitGetCurrency( pPlayer ) <= PlayerGetResurrectCost( pPlayer ))
	{
		return FALSE;
	}

	// cannot be in a room that does not allow resurrect
	ROOM *pRoom = UnitGetRoom( pPlayer );
	if (RoomAllowsResurrect( pRoom ) == FALSE)
	{
		return FALSE;
	}
		
	return TRUE;	

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTakeResurrectCost(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	
	// check if we can or not
	if (PlayerCanResurrect( pPlayer ) == FALSE )
	{
		return FALSE;
	}

	// take the money	
	cCurrency Cost = PlayerGetResurrectCost( pPlayer );
	cCurrency playersCurrency = UnitGetCurrency( pPlayer );
	
	if (playersCurrency >= Cost)
	{
		ASSERT_RETFALSE(UnitRemoveCurrencyValidated( pPlayer, Cost ));
		return TRUE;
	}
	
	return FALSE;  // not enough money
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerGetRestartInTownLevelDef(
	UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTV_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ));

	// get the return level def
	int nLevelDef = UnitGetReturnLevelDefinition( pPlayer );
	if (nLevelDef != INVALID_LINK)
	{
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
		ASSERTX_RETINVALID( pLevelDef, "Expected level definition" );
		
		// check for redirected levels
		if (pLevelDef->nLevelRestartRedirect != INVALID_LINK)
		{
			nLevelDef = pLevelDef->nLevelRestartRedirect;
		}
		
	}
	
	return nLevelDef;
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRespawnInTown(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	int nLevelDefDest = PlayerGetRestartInTownLevelDef( pUnit );
	if (nLevelDefDest != INVALID_LINK)
	{

		// setup warp spec
		WARP_SPEC tSpec;
		tSpec.nLevelDefDest = nLevelDefDest;
		SETBIT( tSpec. dwWarpFlags, WF_ARRIVE_AT_GRAVEKEEPER_BIT );
		
		// warp to the level
		s_WarpToLevelOrInstance( pUnit, tSpec );
		return TRUE;
			
	}

	return FALSE;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRespawn(
	UNIT *pUnit,
	BOOL bAsGhost )
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
			
	ROOM *pRoom = NULL;
	VECTOR vPosition;
	VECTOR vDirection;

	// look at the stat saved in the player for the last level they were in
	BOOL bFound = FALSE;
	LEVELID idLevelLast = UnitGetStat( pUnit, STATS_LAST_LEVELID );
	if (idLevelLast != INVALID_ID)
	{
		LEVEL *pLevel = UnitGetLevel( pUnit );
		
		// given the level that we were last at, find the the arrival
		// position into the current level
		bFound = s_LevelGetArrivalPortalLocation(
			pGame,
			LevelGetID( pLevel ),
			idLevelLast,
			INVALID_ID,
			&pRoom,
			&vPosition,
			&vDirection);

		// NOTE that is is totally possible that there is not a valid "arrival" portal in 
		// this level that matches the "last level id" the player was on because they player
		// could have warped to this level via a PRD, or party warp, etc.
		
		if (bFound)
		{
			// do not allow players to start inside sublevels that don't allow it, instead we will
			// put them at the sublevel entrance		
			SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
			if (pSubLevel)
			{
				const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
				if (pSubLevelDef->bRespawnAtEntrance == TRUE)
				{
					const SUBLEVEL_DOORWAY *pSubLevelEntrance = s_SubLevelGetDoorway( pSubLevel, SD_ENTRANCE );
					if (pSubLevelEntrance)
					{
						if (pSubLevelEntrance->idRoom != INVALID_ID)
						{
							WARP_ARRIVE_POSITION tArrivePosition;
							WarpGetArrivePosition(
								pLevel,
								pSubLevelEntrance->vPosition,
								pSubLevelEntrance->vDirection,
								-1.0f,
								WD_FRONT,
								FALSE,
								&tArrivePosition);
							if (tArrivePosition.pRoom)
							{
								pRoom = tArrivePosition.pRoom;
								vPosition = tArrivePosition.vPosition;
								vDirection = tArrivePosition.vDirection;
							}
							else
							{
								pRoom = RoomGetByID( pGame, pSubLevelEntrance->idRoom );
								vPosition = pSubLevelEntrance->vPosition;
								vDirection = pSubLevelEntrance->vDirection;							
							}
							
						}
						
					}
					
				}
				
			}
			
		}
					
	}

	// fallback case for could not find the "arrival" portal in this level
	if (bFound == FALSE)
	{
		
		// where is the player right now
		ROOM *pRoomCurrent = UnitGetRoom( pUnit );
		ASSERT_RETURN(pRoomCurrent);
		LEVEL *pLevel = RoomGetLevel( pRoomCurrent );
		ASSERT_RETURN( pLevel );

		// get the start location
		switch (GameGetSubType( pGame ))
		{
		case GAME_SUBTYPE_CUSTOM:
			pRoom = s_CustomGameGetStartPosition( 
				pGame, 
				pUnit, 
				LevelGetID( pLevel ), 
				vPosition, 
				vDirection);
			break;

		case GAME_SUBTYPE_PVP_CTF:
			pRoom = s_CTFGetStartPosition( 
				pGame, 
				pUnit, 
				LevelGetID( pLevel ), 
				vPosition, 
				vDirection);
			break;

		default:
			pRoom = LevelGetStartPosition(
				pGame, 
				pUnit,
				LevelGetID( pLevel ), 
				vPosition, 
				vDirection, 
				0);		
			break;
		}

	}

	if( bAsGhost )
	{
		// set player as ghost
		UnitGhost( pUnit, TRUE );
	}
	
	// set the player as a "safe" so that they have a chance to look around
	// before being killed again
	//UnitSetSafe( pUnit );
			
	// warp
	ASSERTX_RETURN( pRoom, "Unable to find room to respawn in" );
	UnitWarp( pUnit, pRoom, vPosition, vDirection, cgvZAxis, 0 );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sResurrect(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pUnit ), "Expected player" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// make them safe
	UnitSetSafe( pUnit );
	
	// warp them to their most recent headstone, it should be in the same sublevel as the
	// player right now, but we're checking just incase
	UNITID idLastHeadstone = UnitGetStat( pUnit, STATS_NEWEST_HEADSTONE_UNIT_ID );
	UNIT *pLastHeadstone = UnitGetById( pGame, idLastHeadstone );
	ASSERTX_RETURN( pLastHeadstone, "Expected headstone" );	
	if (pLastHeadstone && UnitsAreInSameSubLevel( pUnit, pLastHeadstone ))
	{
		UnitWarp(
			pUnit,
			UnitGetRoom( pLastHeadstone ),
			UnitGetPosition( pLastHeadstone ),
			UnitGetFaceDirection( pLastHeadstone, FALSE ),
			UnitGetUpDirection( pUnit ),
			0);
	}
	else
	{
	
		// couldn't find headstone, snap to floor incase they are in the air
		UnitForceToGround( pUnit );
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sArenaRespawn(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( s_QuestGamePlayerArenaRespawn( pUnit ), "Expected player in PvP state" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( pGame, "Expected game" );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	ASSERTX_RETURN( pLevel, "Expected level" );
	LEVELID idLevel = LevelGetID( pLevel );
	
	// find start location
	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_ARENA_BIT );
	VECTOR vPosition;
	VECTOR vDirection;
	ROOM *pRoom = LevelGetStartPosition( pGame, pUnit, idLevel, vPosition, vDirection, dwWarpFlags );
	ASSERTX_RETURN( pRoom, "Expected room for respawn arena" );

	// warp to location
	UnitWarp( pUnit, pRoom, vPosition, vDirection, cgvZAxis, 0 );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPvPRespawn(
	UNIT *pUnit,
	BOOL bFarthest)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( pGame, "Expected game" );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	ASSERTX_RETURN( pLevel, "Expected level" );
	LEVELID idLevel = LevelGetID( pLevel );

	// find start location
	DWORD dwWarpFlags = 0;
	if (bFarthest)
		SETBIT( dwWarpFlags, WF_ARRIVE_AT_FARTHEST_PVP_SPAWN_BIT );
	else
		SETBIT( dwWarpFlags, WF_ARRIVE_AT_RANDOM_SPAWN_BIT );

	VECTOR vPosition;
	VECTOR vDirection;
	ROOM *pRoom = LevelGetStartPosition( pGame, pUnit, idLevel, vPosition, vDirection, dwWarpFlags );
	ASSERTX_RETURN( pRoom, "Expected room for respawn arena" );

	// warp to location
	UnitWarp( pUnit, pRoom, vPosition, vDirection, cgvZAxis, 0 );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sPlayerDieValidateRecallLevel(
	UNIT *pPlayer,
	int nLevelDef)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTV_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETINVALID( IS_SERVER( pGame ), "Server only" );
		
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	ASSERTX( pLevelDef, "Expected level def return" );
	if (pLevelDef)
	{
		// if their return level def is not a valid portal and recall location, put
		// them in the first town
		BOOL bValid = TRUE;
		if (pLevelDef->bPortalAndRecallLoc == FALSE)
		{
			bValid = FALSE;
		}
		
		// check for hardcore dead players and levels that they're not
		// allowed to be on
		if (PlayerIsHardcore( pPlayer ) && pLevelDef->bHardcoreDeadCanVisit == FALSE)
		{
			bValid = FALSE;
		}

		if (bValid == FALSE)
		{
			nLevelDef = LevelGetFirstPortalAndRecallLocation( pGame );		
		}
				
	}

	ASSERTX( nLevelDef != INVALID_LINK, "Expected level def" )
	return nLevelDef;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sPlayerDieValidateStartAndRecallLevels(
	UNIT *pPlayer)
{		
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// validate the return/recall level definition and fix if necessary	
	int nLevelDefReturn = UnitGetReturnLevelDefinition( pPlayer );
	nLevelDefReturn = s_sPlayerDieValidateRecallLevel( pPlayer, nLevelDefReturn );
	UnitSetStat( pPlayer, STATS_LEVEL_DEF_RETURN, UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT), nLevelDefReturn );
	
	// do the same thing for the start level def
	int nLevelDefStart = UnitGetStartLevelDefinition( pPlayer );
	nLevelDefStart = s_sPlayerDieValidateRecallLevel( pPlayer, nLevelDefStart );
	UnitSetStat( pPlayer, STATS_LEVEL_DEF_START, UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT), nLevelDefStart );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_PlayerOnDie(
	UNIT *pPlayer,
	UNIT *pKiller)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// mark the player as waiting for a respawn now
	UnitSetStat( pPlayer, STATS_PLAYER_WAITING_TO_RESPAWN, TRUE );
	
	// cancel any interactions
	s_InteractCancelAll( pPlayer );

	// TODO: detach missiles stuck to the player
	BOOL bKilledByNonHardcorePlayer = pKiller && UnitIsPlayer( pKiller ) && !PlayerIsHardcore( pKiller );		

	if ( PlayerIsHardcore( pPlayer ) &&
		!(AppIsTugboat() && bKilledByNonHardcorePlayer) &&
		 !UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_NO_MAJOR_DEATH_PENALTIES ) )
	{
	
		// hardcore players that die before they get to a valid portal and recall location, put them
		// at the first portal and recall location available	
		if( AppIsHellgate() )
		{
			s_sPlayerDieValidateStartAndRecallLevels( pPlayer );		
		}
		

		// they are now dead forever		
		s_StateSet( pPlayer, pPlayer, STATE_HARDCORE_DEAD, 0 );

		//	update their dead hardcore status on the chat server
		UpdateChatServerPlayerInfo(pPlayer);
	
		//	tell the chat server to remove them from any parties they may be in
		s_PlayerRemoveFromParty(pPlayer);
		
		// save player in single player
		if (AppIsSinglePlayer())
		{
			AppPlayerSave( pPlayer );
		}
		
	}
		
	if( AppIsHellgate() )
	{
		// inform quests of player death
		s_QuestEventPlayerDie( pPlayer, pKiller );

	}

	if( AppIsHellgate() )
	{
		// create headstone
		sCreateHeadstone( pPlayer );
	
		// clear xp penalty system when player dies because we auto remove the state anyway
		s_PlayerClearExperiencePenalty( pPlayer );
		
	}
	else if( AppIsTugboat() && PlayerIsInPVPWorld( pPlayer ) )
	{
		// create headstone
		sCreateHeadstone( pPlayer );
	}
	
	s_UnitTrackPlayerDeath( pPlayer, pKiller );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetRestoreVitalsSpec(
	UNIT *pUnit,
	VITALS_SPEC *pVitals)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( pVitals, "Expected vitals" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// get player level data
	int nExperienceLevel = UnitGetExperienceLevel( pUnit );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	int nUnitType = pUnitData->nUnitTypeForLeveling;
	const PLAYERLEVEL_DATA *pPlayerLevelData = PlayerLevelGetData( pGame, nUnitType, nExperienceLevel );
	if (pPlayerLevelData)
	{
		pVitals->nHealthPercent = pPlayerLevelData->nRestartHealthPercent;
		pVitals->nPowerPercent = pPlayerLevelData->nRestartPowerPercent;
		pVitals->nShieldPercent = pPlayerLevelData->nRestartShieldPercent;		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerRestoreVitals(
	UNIT *pPlayer,
	BOOL bPlayEffects)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// get vital spec
	VITALS_SPEC tVitals;
	sGetRestoreVitalsSpec( pPlayer, &tVitals );

	// restore them
	s_UnitRestoreVitals( pPlayer, bPlayEffects, &tVitals );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAdjustRestoreVitalsForRespawnChoice(
	VITALS_SPEC *pVitals,
	PLAYER_RESPAWN eRespawnChoice)
{
	ASSERTX_RETURN( pVitals, "Expected vitals" );
		
	switch (eRespawnChoice)
	{
	case PLAYER_RESPAWN_RESURRECT:			// resurrecting we will fully heal
	case PLAYER_RESPAWN_RESURRECT_FREE:
	case PLAYER_RESPAWN_PVP_FARTHEST:
	case PLAYER_RESPAWN_PVP_RANDOM:
	case PLAYER_RESPAWN_METAGAME_CONTROLLED:
		pVitals->nHealthPercent = 100;
		pVitals->nPowerPercent = 100;
		pVitals->nShieldPercent = 100;
		break;
	
	// a ghost will make them gone
	case PLAYER_RESPAWN_GHOST:
		// however, until we fix the "regen while a ghost" problem will will put them at
		// full health and take it away when they actually resurrect
		pVitals->nHealthPercent = 100;
		pVitals->nPowerPercent = 100;
		pVitals->nShieldPercent = 100;	
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerClearExperiencePenalty(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	UnitUnregisterTimedEvent( pPlayer, sDecrementExperiencePenaltyLeft );
	s_StateClear( pPlayer, INVALID_ID, STATE_EXPERIENCE_PENALTY, 0 );
	UnitClearStat( pPlayer, STATS_EXPERIENCE_PENALTY_DURATION_LEFT_IN_SECONDS, 0 );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerRestart(
	UNIT *pUnit,
	PLAYER_RESPAWN eRespawnChoice)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )	
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pUnit ), "Expected player, got '%s'", UnitGetDevName( pUnit ) );
	GAME *pGame = UnitGetGame( pUnit );

	BOOL bRevivedGhost = FALSE;
	if (eRespawnChoice == PLAYER_REVIVE_GHOST)
	{
		if (UnitIsGhost( pUnit ))
		{
			
			// remove ghost state
			UnitGhost( pUnit, FALSE );
			
			// restart them in town
			eRespawnChoice = PLAYER_RESPAWN_TOWN;
			
			// the ghost has been revived
			bRevivedGhost = TRUE;
			
		}
		
	}

	// player must be dead, note do not log a hacking attempt here as we can receive
	// restart messages multiple times from multiple key presses on a client
	BOOL bAllowRespawnOfLivingForPvP = AppIsHellgate() && UnitPvPIsEnabled( pUnit ) && !UnitHasState( pGame, pUnit, STATE_PVP_FFA );
	if (bRevivedGhost == FALSE && !bAllowRespawnOfLivingForPvP)
	{
		if (IsUnitDeadOrDying( pUnit ) == FALSE)
		{
			return;
		}
	}
			
	// hardcore players are forced to respawn in town
	if (PlayerIsHardcore( pUnit ) &&
		!UnitHasState( UnitGetGame( pUnit ), pUnit, STATE_NO_MAJOR_DEATH_PENALTIES ) )
	{
		UNITLOG_WARNX( eRespawnChoice == PLAYER_RESPAWN_TOWN, "Hardcore player attempting to cheat on respawn", pUnit );
		eRespawnChoice = PLAYER_RESPAWN_TOWN;
	}
	
	BOOL bInADuel = UnitHasState( pGame, pUnit, STATE_PVP_DUEL );

	if( AppIsTugboat() )
	{

		if (bInADuel &&
			eRespawnChoice != PLAYER_RESPAWN_PVP_RANDOM &&
			eRespawnChoice != PLAYER_RESPAWN_PVP_FARTHEST)
		{
			eRespawnChoice = PLAYER_RESPAWN_PVP_RANDOM;
		}

		ROOM * cur_room = UnitGetRoom( pUnit );
		ASSERT_RETURN(cur_room);
		LEVEL * level = RoomGetLevel(cur_room);
		ASSERT_RETURN(level);
		
		s_UnitRestoreVitals( pUnit, FALSE);
		//UnitWarp(unit, room, vPosition, vFaceDirection, VECTOR(0, 0, 1), 0);
		UnitEndMode(pUnit, MODE_DYING, 0, TRUE);
		UnitEndMode(pUnit, MODE_DEAD, 0, TRUE);
		s_UnitSetMode(pUnit, MODE_IDLE);
		// find out if we're respawning in the same level - if so, don't go through all the recall junk

		int nLevelDefDest = LevelDefinitionGetRandom( level, LevelGetLevelAreaID( level ), 0 );
		ASSERTX_RETURN( nLevelDefDest != INVALID_LINK, "Invalid level definition destination" );		
		/*
		const LEVEL_DEFINITION *ptLevelDefDest = LevelDefinitionGet( nLevelDefDest );			
		
		if( ptLevelDefDest == LevelDefinitionGet( level ) )
		{
			sRespawn( pUnit, FALSE );
			// "safety" them		
			UnitSetSafe( pUnit );
		}
		else
		{
		*/

		// do restart choice
		switch (eRespawnChoice)
		{

			//----------------------------------------------------------------------------
		case PLAYER_RESPAWN_PVP_RANDOM:
			{
				sPvPRespawn( pUnit, FALSE );
				break;				
			}

			//----------------------------------------------------------------------------
		case PLAYER_RESPAWN_PVP_FARTHEST:
			{
				sPvPRespawn( pUnit, TRUE );
				break;				
			}
		default:
			{
				DWORD dwLevelRecallFlags = 0;
				SETBIT( dwLevelRecallFlags, LRF_FORCE );
				SETBIT( dwLevelRecallFlags, LRF_PRIMARY_ONLY );
				s_LevelRecall( pGame, pUnit, dwLevelRecallFlags );
				break;
			}

		}
		//}
	}
	else
	{
									
		// as a precaution against cheating, if a player is part of the arena quest minigame or duel, 
		// there can only be one respawn choice
		if (s_QuestGamePlayerArenaRespawn( pUnit ))
		{
			eRespawnChoice = PLAYER_RESPAWN_ARENA;
		}

		if (bInADuel &&
			eRespawnChoice != PLAYER_RESPAWN_PVP_RANDOM &&
			eRespawnChoice != PLAYER_RESPAWN_PVP_FARTHEST)
		{
			eRespawnChoice = PLAYER_RESPAWN_PVP_RANDOM;
		}

				
		// validate that they can resurrect if the are choosing to
		if (eRespawnChoice == PLAYER_RESPAWN_RESURRECT)
		{
			if (sTakeResurrectCost( pUnit ) == FALSE)
			{
				return;
			}		
		}
				
		// show headstone
 		if (!s_QuestGamePlayerArenaRespawn(pUnit) && !bAllowRespawnOfLivingForPvP)
		{
			sShowHeadstone( pUnit );
		}

		// setup vitals restoration spec
		VITALS_SPEC tVitalsSpec;
		sGetRestoreVitalsSpec( pUnit, &tVitalsSpec );

		// adjust the restore vitals based on respawn choice
		sAdjustRestoreVitalsForRespawnChoice( &tVitalsSpec, eRespawnChoice );
					
		// restore vitals to a percentage, unless you are resurrecting, then we give all of it
		s_UnitRestoreVitals( pUnit, FALSE, &tVitalsSpec );

		// explicitly end death
		UnitEndMode( pUnit, MODE_DYING, 0, TRUE );
		UnitEndMode( pUnit, MODE_DEAD, 0, TRUE );

		// go into idle
		s_UnitSetMode( pUnit, MODE_IDLE );

		// set resurrected state
		s_StateSet( pUnit, pUnit, STATE_RESURRECT, 0 );
		
		// set experience penalty state
		switch (eRespawnChoice)
		{
		case PLAYER_RESPAWN_ARENA:
		case PLAYER_RESPAWN_PVP_FARTHEST:
		case PLAYER_RESPAWN_PVP_RANDOM:
		case PLAYER_RESPAWN_METAGAME_CONTROLLED:
			break;

		default:
			s_sPlayerGiveExperiencePenalty( pUnit, 0 );
			break;
		}
		
		// do restart choice
		switch (eRespawnChoice)
		{
			
			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_TOWN:
			{
				if (sRespawnInTown( pUnit ) == FALSE)
				{
					sRespawn( pUnit, TRUE );
				}
				break;				
			}

			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_RESURRECT:
			case PLAYER_RESPAWN_RESURRECT_FREE:
			{
				sResurrect( pUnit );
				break;				
			}

			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_GHOST:
			default:
			{
				sRespawn( pUnit, TRUE );
				break;				
			}
			
			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_ARENA:
			{
				sArenaRespawn( pUnit );
				break;
			}

			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_PVP_RANDOM:
			{
				sPvPRespawn( pUnit, FALSE );
				break;				
			}

			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_PVP_FARTHEST:
			{
				sPvPRespawn( pUnit, TRUE );
				break;				
			}


			//----------------------------------------------------------------------------
			case PLAYER_RESPAWN_METAGAME_CONTROLLED:
			{
				s_MetagameEventRespawnWarpRequest( pUnit );
				break;				
			}
				
		}

		if (!UnitIsGhost( pUnit ))
			s_MetagameEventPlayerRespawn( pUnit );
		
	}
#endif		
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *PlayerFindNearest(
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	UNIT *pPlayerClosest = NULL;
	float flClosestDistSq = 0.0f;
	
	// scan all clients	
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))	
	{
	
		// get player
		UNIT *pPlayer = ClientGetPlayerUnit( pClient );
		
		// must be on same level
		if (UnitsAreInSameLevel( pPlayer, pUnit ))
		{
			
			// get distance
			float flDistSq = UnitsGetDistanceSquared( pUnit, pPlayer );
			
			// check for closest
			if (pPlayerClosest == NULL || flDistSq < flClosestDistSq)
			{
				pPlayerClosest = pPlayer;
				flClosestDistSq = flDistSq;
			}
			
		}
		
	}

	return pPlayerClosest;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayersIterate( 
	GAME *pGame, 
	PFN_PLAYER_ITERATE pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pfnCallback, "Expected callback function" );
		
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pUnit = ClientGetControlUnit( pClient );
		if (pUnit)
		{
			pfnCallback( pUnit, pCallbackData );
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerGetAvailableSkillsOfGroup(
	GAME * pGame,
	UNIT * pUnit,
	int nSkillGroup )
{
	ASSERT_RETINVALID( pUnit );
	if ( ! UnitIsPlayer( pUnit ) )
		return 0;
	int nSlots = 0;

	int nClass = UnitGetClass( pUnit );
	const UNIT_DATA * player_data = PlayerGetData( pGame, nClass );
	ASSERT_RETFALSE(player_data);
	int nPlayerLevel = UnitGetExperienceLevel( pUnit );
	int nMaxLevel = UnitGetMaxLevel( pUnit );

	for( int iLevel = 0; iLevel < nMaxLevel; iLevel++ )
	{
		if( iLevel <= nPlayerLevel )
		{
			const PLAYERLEVEL_DATA * level_data = PlayerLevelGetData( pGame, player_data->nUnitTypeForLeveling, iLevel );
			if( level_data )
			{
				for( int i = 0; i < MAX_SPELL_SLOTS_PER_TIER; i++ )
				{
					if( level_data->nSkillGroup[i] == nSkillGroup )
					{
						nSlots++;
					}
				}
			}
		}
	}
	return nSlots;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerGetKnownSkillsOfGroup(
	GAME * pGame,
	UNIT * pUnit,
	int nSkillGroup )
{
	ASSERT_RETINVALID( pUnit );
	if ( ! UnitIsPlayer( pUnit ) )
		return 0;

	int nKnown = 0;
	int nRowCount = ExcelGetNumRows(pGame, DATATABLE_SKILLS);

	int nClass = UnitGetClass( pUnit );
	const UNIT_DATA * player_data = PlayerGetData( pGame, nClass );
	ASSERT_RETFALSE(player_data);

	for (int iRow = 0; iRow < nRowCount; iRow++)
	{
		const SKILL_DATA* skill_data = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
		if (skill_data )
		{
			for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
			{
				if ( skill_data->pnSkillGroup[ i ] == nSkillGroup &&
					SkillGetLevel(pUnit, iRow) > 0 )
				{
					nKnown++;
					break;
				}
			}
		}
	}
	return nKnown;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PlayerGetNextFreeSpellSlot(
	GAME * pGame,
	UNIT * pUnit,
	int nSkillGroup,
	int nSkillID )
{
	ASSERT_RETINVALID( pUnit );
	if ( ! UnitIsPlayer( pUnit ) )
		return 0;

	int nSlots = 0;

	int nClass = UnitGetClass( pUnit );
	const UNIT_DATA * player_data = PlayerGetData( pGame, nClass );
	ASSERT_RETFALSE(player_data);
	SIMPLE_DYNAMIC_ARRAY<int> Slots;
	ArrayInit(Slots, GameGetMemPool(pGame), 1);
	int nMaxLevel = UnitGetMaxLevel( pUnit );
	for( int iLevel = 0; iLevel < nMaxLevel; iLevel++ )
	{
		const PLAYERLEVEL_DATA * level_data = PlayerLevelGetData( pGame, player_data->nUnitTypeForLeveling, iLevel );
		if( level_data )
		{
			for( int i = 0; i < MAX_SPELL_SLOTS_PER_TIER; i++ )
			{
				if( level_data->nSkillGroup[i] > 0 )
				{
					ArrayAddItem(Slots, level_data->nSkillGroup[i]);
					nSlots++;
				}
			}
		}
	}

	int nRowCount = ExcelGetNumRows(pGame, DATATABLE_SKILLS);



	for (int iRow = 0; iRow < nRowCount; iRow++)
	{
		const SKILL_DATA* skill_data = (SKILL_DATA *)ExcelGetData(pGame, DATATABLE_SKILLS, iRow);
		if (skill_data &&
			skill_data->pnSkillGroup[ 0 ] == nSkillGroup &&
			iRow != nSkillID )
		{

			if( SkillGetLevel(pUnit, iRow) > 0 )
			{
				int Slot = UnitGetStat( pUnit, STATS_SPELL_SLOT, iRow );
				if( Slot < nSlots )
				{
					Slots[Slot] = -1;
				}
			}
		}
	}
	
	for( int i = 0; i < nSlots; i++ )
	{
		if( Slots[i] == nSkillGroup )
		{
			return i;
		}
	}
	
	Slots.Destroy();

	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int x_PlayerInteractGetChoices(
	UNIT *pUnitInteracting,
	UNIT *pPlayerTarget,
	INTERACT_MENU_CHOICE *ptChoices,
	int nBufferSize,
	UI_COMPONENT *pComponent /*=NULL*/,
	PGUID guid /*=INVALID_GUID*/)
{
	int nNumChoices = 0;
	for (int i = 0; i < nBufferSize; ++i)
	{
		ptChoices[ i ].nInteraction = UNIT_INTERACT_INVALID;
		ptChoices[ i ].bDisabled = FALSE;
	}

	if (pPlayerTarget || guid != INVALID_GUID)
	{
		const PGUID Guid = (guid == INVALID_GUID) ? UnitGetGuid(pPlayerTarget) : guid;

#if !ISVERSION(SERVER_VERSION)
		if (c_PlayerFindPartyMember(Guid) == INVALID_INDEX)
		{
			// invite to join party
			InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERINVITE, TRUE, &nNumChoices, ptChoices, nBufferSize );
		}

		if (!c_BuddyGetCharNameByGuid(Guid))
		{
			// add unit to your buddy list
			InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERBUDDYADD, TRUE, &nNumChoices, ptChoices, nBufferSize );
		}

		// whisper to player
		InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERWHISPER, TRUE, &nNumChoices, ptChoices, nBufferSize );
#endif

		// ****************************************************************************************************************************
		//  we directly clicked on someone
		//

		if (pPlayerTarget && !pComponent)
		{
#if !ISVERSION(SERVER_VERSION)
			// get guild info and rank from the target unit
			WCHAR szGuildInfo[256] = L"";
			GUILD_RANK eGuildRank = GUILD_RANK_INVALID;
			WCHAR szGuildRankName[256] = L"";
			UnitGetPlayerGuildAssociationTag(pPlayerTarget, szGuildInfo, arrsize(szGuildInfo), eGuildRank, szGuildRankName, arrsize(szGuildRankName));

			// trade with player			
			int nOverrideTooltipId = -1;
			BOOL bCanTrade = TradeCanTradeWith( pUnitInteracting, pPlayerTarget, &nOverrideTooltipId );
			InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERTRADE, bCanTrade, &nNumChoices, ptChoices, nBufferSize, nOverrideTooltipId );

			if (szGuildInfo[0])
			{
				if (c_PlayerCanDoGuildAction(GUILD_ACTION_PROMOTE, eGuildRank))
				{
					// promote player by one rank
					sGuildPromoteGuid = UnitGetGuid( pPlayerTarget );
					sGuildPromoteRank = c_PlayerGetGuildMemberRankByID(sGuildPromoteGuid);
					sGuildPromoteRank = (GUILD_RANK)(((int)sGuildPromoteRank)+1);
					InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDPROMOTE, TRUE, &nNumChoices, ptChoices, nBufferSize );
				}
			}
			else
			{
				if (c_PlayerCanDoGuildAction(GUILD_ACTION_INVITE))
				{
					// invite player to guild
					InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDINVITE, TRUE, &nNumChoices, ptChoices, nBufferSize );
				}
			}
#endif

			if (DuelCanPlayersDuel( pUnitInteracting, pPlayerTarget ))
			{
				// invite player to duel
				InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERINVITEDUEL, TRUE, &nNumChoices, ptChoices, nBufferSize );
			}

			// KCK: Allow inspection of other players
			if (PlayerCanBeInspected( pUnitInteracting, pPlayerTarget ))
			{
				InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERINSPECT, TRUE, &nNumChoices, ptChoices, nBufferSize );	
			}
		}

		// ****************************************************************************************************************************
		//  we clicked on a player in a buddy or guild list
		//
		if (pComponent)
		{
#if !ISVERSION(SERVER_VERSION)
			// get guild rank from the guild list
			GUILD_RANK eGuildRank = c_PlayerGetGuildMemberRankByID(Guid);

			if (c_PlayerCanWarpTo(Guid))
			{
				// warp to player
				InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERWARPTO, TRUE, &nNumChoices, ptChoices, nBufferSize );
			}

			if (!PStrICmp(pComponent->m_szName, "guild list"))
			{
				if (eGuildRank != GUILD_RANK_INVALID)
				{
					if (c_PlayerCanDoGuildAction(GUILD_ACTION_BOOT, eGuildRank))
					{
						// kick player from guild
						sGuildKickGuid = Guid;
						InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDKICK, TRUE, &nNumChoices, ptChoices, nBufferSize );
					}
					if (c_PlayerCanDoGuildAction(GUILD_ACTION_PROMOTE, eGuildRank))
					{
						// promote player by one rank
						sGuildPromoteGuid = Guid;
						sGuildPromoteRank = c_PlayerGetGuildMemberRankByID(sGuildPromoteGuid);
						sGuildPromoteRank = (GUILD_RANK)(((int)sGuildPromoteRank)+1);
						InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDPROMOTE, TRUE, &nNumChoices, ptChoices, nBufferSize );
					}
					if (c_PlayerCanDoGuildAction(GUILD_ACTION_DEMOTE, eGuildRank))
					{
						// demote player by one rank
						sGuildDemoteGuid = UnitGetGuid( pPlayerTarget );
						sGuildDemoteRank = c_PlayerGetGuildMemberRankByID(sGuildDemoteGuid);
						sGuildDemoteRank = (GUILD_RANK)(((int)sGuildDemoteRank)-1);
						InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDDEMOTE, TRUE, &nNumChoices, ptChoices, nBufferSize );
					}
				}
			}
			else if (!PStrICmp(pComponent->m_szName, "buddy list"))
			{
				// remove the unit from your buddy list
				InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERBUDDYREMOVE, TRUE, &nNumChoices, ptChoices, nBufferSize );

				if (eGuildRank == GUILD_RANK_INVALID)
				{
					// invite player to guild
					InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDINVITE, TRUE, &nNumChoices, ptChoices, nBufferSize );
				}
			}
#endif
		}
	}
	else if(pComponent)
	{
#if !ISVERSION(SERVER_VERSION)
		// no unit or guid, there are only certain cases this should happen
		if (!PStrICmp(pComponent->m_szName, "buddy list"))
		{
			// remove the (offline) unit from your buddy list
			InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERBUDDYREMOVE, TRUE, &nNumChoices, ptChoices, nBufferSize );
		}
		else if (!PStrICmp(pComponent->m_szName, "guild list"))
		{
			extern PGUID curGuildMemberLinkId;
			GUILD_RANK eGuildRank = c_PlayerGetGuildMemberRankByID(curGuildMemberLinkId);

			if (eGuildRank != GUILD_RANK_INVALID)
			{
				if (c_PlayerCanDoGuildAction(GUILD_ACTION_BOOT, eGuildRank))
				{
					// kick (offline) player from guild
					sGuildKickGuid = curGuildMemberLinkId;
					InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDKICK, TRUE, &nNumChoices, ptChoices, nBufferSize );
				}
				if (c_PlayerCanDoGuildAction(GUILD_ACTION_PROMOTE, eGuildRank))
				{
					// promote (offline) player by one rank
					sGuildPromoteGuid = curGuildMemberLinkId;
					sGuildPromoteRank = c_PlayerGetGuildMemberRankByID(sGuildPromoteGuid);
					sGuildPromoteRank = (GUILD_RANK)(((int)sGuildPromoteRank)+1);
					InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDPROMOTE, TRUE, &nNumChoices, ptChoices, nBufferSize );
				}
				if (c_PlayerCanDoGuildAction(GUILD_ACTION_DEMOTE, eGuildRank))
				{
					// demote (offline) player by one rank
					sGuildDemoteGuid = curGuildMemberLinkId;
					sGuildDemoteRank = c_PlayerGetGuildMemberRankByID(sGuildDemoteGuid);
					sGuildDemoteRank = (GUILD_RANK)(((int)sGuildDemoteRank)-1);
					InteractAddChoice( pUnitInteracting, UNIT_INTERACT_PLAYERGUILDDEMOTE, TRUE, &nNumChoices, ptChoices, nBufferSize );
				}
			}
		}
#endif
	}

	return nNumChoices;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerPostsOpenLevels(
	UNIT *pPlayer)
{

	// tugboat doesn't want this functionality :(
	if (AppIsTugboat())
	{
		return FALSE;
	}

	// we only post levels if the player has chosen so for their character
	return s_PlayerIsAutoPartyEnabled( pPlayer );	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerIsAutoPartyEnabled(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// Mythos doesn't do these yet
	if (AppIsTugboat())
	{
		return FALSE;
	}

	// return the stat recorded
	return UnitGetStat( pPlayer, STATS_AUTO_PARTY_ENABLED );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerEnableAutoParty(
	UNIT *pPlayer, 
	BOOL bEnabled)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	UnitSetStat( pPlayer, STATS_AUTO_PARTY_ENABLED, bEnabled );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_PlayerToggleAutoParty(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );	
	
	// only do this in multiplayer and if the auto party system is enabled in this build
	GAME *pGame = UnitGetGame( pPlayer );
	if (AppIsMultiplayer() && PartyMatchmakerIsEnabledForGame( pGame ))
	{
		MSG_CCMD_TOGGLE_AUTO_PARTY tMessage;
		c_SendMessage( CCMD_TOGGLE_AUTO_PARTY, &tMessage );
	}
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerPvPIsEnabled(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	return UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_PVP_ENABLED );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_PlayerPvPToggle(UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );	

	// only do this in multiplayer
	if (AppIsMultiplayer() && !AppTestFlag(AF_CENSOR_NO_PVP))
	{
		MSG_CCMD_PVP_TOGGLE tMessage;
		c_SendMessage( CCMD_PVP_TOGGLE, &tMessage );
	}

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_PlayerSendRespawn(
	PLAYER_RESPAWN eChoice)
{

	// tell server we want to restart
	MSG_CCMD_PLAYER_RESPAWN tMessage;
	tMessage.nPlayerRespawn = eChoice;
	c_SendMessage( CCMD_PLAYER_RESPAWN, &tMessage );

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDoPlayerRoomUpdate(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	s_PlayerQueueRoomUpdate( pUnit, 0, TRUE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerQueueRoomUpdate(
	UNIT *pPlayer,
	DWORD dwDelayInTicks,
	BOOL bCalledFromEvent /*= FALSE*/)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );
	
	// remove any existing room update event we had one and it's necessary to even check
	if (bCalledFromEvent == FALSE)
	{
		UnitUnregisterTimedEvent( pPlayer, sDoPlayerRoomUpdate );
	}
	else
	{
		// was called from event, should be an immediate update
		ASSERTX( dwDelayInTicks == 0, "Expected 0 delay for player queue room update" );	
	}
			
	// do an update now or queue one
	if (dwDelayInTicks)
	{
		UnitRegisterEventTimed( pPlayer, sDoPlayerRoomUpdate, NULL, dwDelayInTicks );						
	}
	else
	{
	
		// do a room update
		RoomUpdateClient( pPlayer );
		
		// mark this position as the last position we did a room update at
		const VECTOR &vPosCurrent = UnitGetPosition( pPlayer );		
		UnitSetStatVector( pPlayer, STATS_LAST_ROOM_UPDATE_POS_X, 0, vPosCurrent );

		// when players move around, we need to update rooms (so we can activate and deactivate them)
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		LevelNeedRoomUpdate( pLevel, TRUE );
	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerMoved( 
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// auto pickup stuff
	if( AppIsHellgate() )
	{		
		s_UnitAutoPickupObjectsAround( pPlayer );
	}

	// have we moved far enough from our last location to do a room update
	const VECTOR &vPosCurrent = UnitGetPosition( pPlayer );
	VECTOR vPosLastUpdate = UnitGetStatVector( pPlayer, STATS_LAST_ROOM_UPDATE_POS_X, 0 );
	float flDistSq = VectorDistanceSquared( vPosCurrent, vPosLastUpdate );
	if (flDistSq >= ROOM_UPDATE_DIST_SQ)
	{
		// do an update now
		s_PlayerQueueRoomUpdate( pPlayer, 0 );		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerGetRespawnLocation( 
	UNIT *pPlayer,	
	ERESPAWN_TYPE type,
	int &nLevelAreaDestinationID,
	int &nLevelDepthDestination,
	VECTOR &vPosition,
	LEVEL *pLevel /* = NULL */  )
{
	nLevelAreaDestinationID = INVALID_ID;
	nLevelDepthDestination = INVALID_ID;
	vPosition = VECTOR( -1.0f, -1.0f, -1.0f );
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitGetGenus( pPlayer ) == GENUS_PLAYER );	

	switch( type )
	{
	case KRESPAWN_TYPE_ANCHORSTONE:
		{
			//not in all cases is the unit found here
			int nAnchorStoneClass = UnitGetStat( pPlayer, STATS_ANCHORMARKERS_SET );
			if( nAnchorStoneClass == INVALID_ID )
				return FALSE;
			const UNIT_DATA *pUnitData = ObjectGetData( UnitGetGame( pPlayer ), nAnchorStoneClass );
			if( pUnitData == NULL )
				return FALSE;
			nLevelAreaDestinationID = UnitGetStat( pPlayer, STATS_ANCHORMARKERS_LEVELAREA_SET );
			nLevelDepthDestination = 0;			
			MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfInterest = PlayerGetPointsOfInterest( pPlayer );
			if( pPointsOfInterest )
			{
				int nPointOfInterestIndex = pPointsOfInterest->GetPointOfInterestIndexByUnitData( pUnitData );
				if( nPointOfInterestIndex != INVALID_ID )
				{
					const MYTHOS_POINTSOFINTEREST::PointOfInterest *pPointOfInterest = pPointsOfInterest->GetPointOfInterestByIndex( nPointOfInterestIndex );
					if( pPointOfInterest )
					{
						vPosition.fX = (float)pPointOfInterest->nPosX;	//get the position
						vPosition.fY = (float)pPointOfInterest->nPosY;	//get the position
						vPosition.fZ = 0.0f;							//get the position
					}
				}
			}
			if( vPosition.fX == -1.0f && 
				vPosition.fY == -1.0f &&
				pLevel &&
				IS_SERVER( pPlayer ) )
			{
				int nPointOfInterest = s_LevelGetUnitOfInterestCount( pLevel );
				for( int t = 0; t < nPointOfInterest; t++ )
				{
					UNIT *pPofI = s_LevelGetUnitOfInterestByIndex( pLevel, t );
					if( pPofI->pUnitData == pUnitData )
					{
						VECTOR vPositionOfUnit = UnitGetPosition( pPofI	);
						vPosition.fX = (float)vPositionOfUnit.fX;	//get the position
						vPosition.fY = (float)vPositionOfUnit.fY;	//get the position
						vPosition.fZ = 0.0f;							//get the position
						break;
					}
				}
			}

		}
		break;
	case KRESPAWN_TYPE_PRIMARY:
		{
			vPosition.fX = (float)UnitGetStat( pPlayer, STATS_RESPAWN_LEVEL_X );
			vPosition.fY = (float)UnitGetStat( pPlayer, STATS_RESPAWN_LEVEL_Y );
			nLevelAreaDestinationID = UnitGetStat( pPlayer, STATS_RESPAWN_LEVELAREA );
			nLevelDepthDestination = UnitGetStat( pPlayer, STATS_RESPAWN_LEVELDEPTH );
		}
		break;
	case KRESPAWN_TYPE_RETURNPOINT:
		{
			vPosition.fX = (float)UnitGetStat( pPlayer, STATS_RESPAWN_SECONDARY_LEVEL_X );
			vPosition.fY = (float)UnitGetStat( pPlayer, STATS_RESPAWN_SECONDARY_LEVEL_Y );
			nLevelAreaDestinationID = UnitGetStat( pPlayer, STATS_RESPAWN_SECONDARY_LEVELAREA );
			nLevelDepthDestination = 0;
		}
		break;
	}

	return ( nLevelAreaDestinationID != INVALID_ID)?TRUE:FALSE; 
}

//----------------------------------------------------------------------------
//sets stats for the player to respawn in a specific area
//----------------------------------------------------------------------------
void PlayerSetRespawnLocation( 
							  ERESPAWN_TYPE type,
							  UNIT *pPlayer,
							  UNIT *pTrigger,
							  LEVEL *pLevel )
{
	ASSERT_RETURN( pPlayer );	
	ASSERT_RETURN( pPlayer );	
	ASSERT_RETURN( pLevel );
	
	if( type == KRESPAWN_TYPE_PLAYERFIRSTLOAD )
	{
		if( UnitGetStat( pPlayer, STATS_ANCHORMARKERS_SET ) == INVALID_ID )
		{
			int nLineNumber = GlobalIndexGet( GI_OBJECT_DEFAULT_ANCHORSTONE );
			UnitSetStat( pPlayer, STATS_ANCHORMARKERS_SET, nLineNumber );
			UnitSetStat( pPlayer, STATS_ANCHORMARKERS_LEVELAREA_SET, LevelGetLevelAreaID( pLevel ) );		
		}
		else
		{
			return;	//we don't want to set the primary if we have already set the starting location
		}

	}
	//Primary always saves now KRESPAWN_TYPE_PRIMARY
	if( MYTHOS_LEVELAREAS::LevelAreaAllowPrimarySave( LevelGetLevelAreaID( pLevel ) ) )
	{
		UnitSetStat( pPlayer, STATS_RESPAWN_LEVELAREA, LevelGetLevelAreaID( pLevel ) );
		UnitSetStat( pPlayer, STATS_RESPAWN_LEVELDEPTH, LevelGetDepth( pLevel ) );
		VECTOR playerPos = UnitGetPosition( pPlayer );
		UnitSetStat( pPlayer, STATS_RESPAWN_LEVEL_X, (int)playerPos.fX );
		UnitSetStat( pPlayer, STATS_RESPAWN_LEVEL_Y, (int)playerPos.fY );
	}
	if( type == KRESPAWN_TYPE_ANCHORSTONE )
	{		
		ASSERT_RETURN( pTrigger );	
		UINT nDataTable = ( UnitGetGenus( pTrigger ) == GENUS_OBJECT )?DATATABLE_OBJECTS:DATATABLE_ITEMS;
		nDataTable = ( UnitGetGenus( pTrigger ) == GENUS_MONSTER )?DATATABLE_MONSTERS:nDataTable;
		UINT nLineNumber = ExcelGetLineByCode( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), nDataTable, pTrigger->pUnitData->wCode );
		UnitSetStat( pPlayer, STATS_ANCHORMARKERS_SET, nLineNumber );
		UnitSetStat( pPlayer, STATS_ANCHORMARKERS_LEVELAREA_SET, LevelGetLevelAreaID( pLevel ) );
	}
	else if( type == KRESPAWN_TYPE_RETURNPOINT )
	{
		UnitSetStat( pPlayer, STATS_RESPAWN_SECONDARY_LEVELAREA, LevelGetLevelAreaID( pLevel ) );		
		VECTOR playerPos = UnitGetPosition( pPlayer );
		UnitSetStat( pPlayer, STATS_RESPAWN_SECONDARY_LEVEL_X, (int)playerPos.fX );
		UnitSetStat( pPlayer, STATS_RESPAWN_SECONDARY_LEVEL_Y, (int)playerPos.fY );
	}
	
}

//----------------------------------------------------------------------------
// This returns the object that is closest to the players starting respawn stat
//----------------------------------------------------------------------------
UNIT * PlayerGetRespawnMarker( UNIT *pPlayer,
							   LEVEL *pLevel,
							   VECTOR *pVector /*= NULL*/)
{
	ASSERT_RETNULL( pPlayer );	
	if( pLevel == NULL )
		return NULL;
	VECTOR vPosition = VECTOR( (float)UnitGetStat( pPlayer, STATS_RESPAWN_LEVEL_X ),
							   (float)UnitGetStat( pPlayer, STATS_RESPAWN_LEVEL_Y ),
							   0.0f );
	if( pVector )
	{
		vPosition = *pVector;
	}
	UNIT *pUnitToReturn( NULL );
	float fDistance( 999999.0f );
	for ( ROOM * pRoom = LevelGetFirstRoom(pLevel); pRoom; pRoom = LevelGetNextRoom(pRoom) )	
	{
		for ( UNIT * test = pRoom->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
		{
			if( UnitIsA( test, UNITTYPE_ANCHORSTONE_MARKER ) )			
			{
				VECTOR vObjectPos = UnitGetPosition( test );
				vObjectPos -= vPosition;
				float fLength = VectorLength( vObjectPos );
				if( fLength < fDistance )
				{
					fDistance = fLength;
					pUnitToReturn = test;
				}
			}
		}
	}
	return pUnitToReturn;
}



//----------------------------------------------------------------------------
// Fix so that players who've managed to set their current station, on nightmare,
// to something other than holborn w/o having holborn as a waypoint, start in Russell Square
//----------------------------------------------------------------------------
static BOOL sPlayerFixNightmareWaypoints(
	UNIT *pPlayer)
{
	if(AppIsTugboat()) // This is a hellgate only fix
		return FALSE;

	if(UnitGetStat(pPlayer, STATS_DIFFICULTY_MAX) == DIFFICULTY_NIGHTMARE)
	{
		if(!WaypointIsMarked(NULL, pPlayer, GlobalIndexGet( GI_LEVEL_HOLBORN_STATION ), DIFFICULTY_NIGHTMARE)) // If they don't have Holborn station, BLAST all their waypoints away!
		{
			s_WaypointsClearAll(pPlayer, DIFFICULTY_NIGHTMARE);
			return TRUE;
		}
	}

	return FALSE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCountEarnedQuestStatPoints(
	QUEST *pQuest,
	void *pCallbackData)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// is quest complete
	if (QuestIsDone( pQuest ))
	{
		const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
		if (pQuestDef && pQuestDef->nStatPoints > 0)
		{
			int *pnStatPoints = (int *)pCallbackData;
			*pnStatPoints = *pnStatPoints + pQuestDef->nStatPoints;
		}
		
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCountEarnedQuestSkillPoints(
	QUEST *pQuest,
	void *pCallbackData)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// is quest complete
	if (QuestIsDone( pQuest ))
	{
		const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
		if (pQuestDef && pQuestDef->nSkillPoints > 0)
		{
			int *pnSkillPoints = (int *)pCallbackData;
			*pnSkillPoints = *pnSkillPoints + pQuestDef->nSkillPoints;
		}
		
	}	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetExpectedSkillPoints( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pUnitData = UnitGetData( pPlayer );
	int nExperienceLevel = UnitGetExperienceLevel( pPlayer );
	
	int nSkillPointsExpected = 0;		
	if (AppIsTugboat())
	{
		for (int i = 1; i <= nExperienceLevel; ++i)
		{
			const PLAYERLEVEL_DATA *pLevelData = PlayerLevelGetData( pGame, pUnitData->nUnitTypeForLeveling, i );
			ASSERTX_CONTINUE( pLevelData, "Expected level data" );
			nSkillPointsExpected += pLevelData->nSkillPoints;
		}	
	}
	else
	{
		// hellgate has it hardcoded to one skill point per level starting at level 2
		nSkillPointsExpected = nExperienceLevel - 1;	
	}

	// now get all the skill points from quest rewards
	// how many stat points are given from completed quests
	DWORD dwQuestIterateFlags = 0;
	SETBIT( dwQuestIterateFlags, QIF_ALL_DIFFICULTIES );
	QuestIterateQuests( pPlayer, sCountEarnedQuestSkillPoints, &nSkillPointsExpected, dwQuestIterateFlags, FALSE );
			
	return nSkillPointsExpected;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetExpectedPerkPoints( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pUnitData = UnitGetData( pPlayer );
	int nRank = UnitGetPlayerRank( pPlayer );

	int nPerkPointsExpected = 0;		
	for (int i = 1; i <= nRank; ++i)
	{
		const PLAYERRANK_DATA *pRankData = PlayerRankGetData( pGame, pUnitData->nUnitTypeForLeveling, i );
		ASSERTX_CONTINUE( pRankData, "Expected rank data" );
		nPerkPointsExpected += pRankData->nPerkPoints;
	}	

	return nPerkPointsExpected;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetExpectedCraftingPoints( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pUnitData = UnitGetData( pPlayer );
	int nExperienceLevel = UnitGetExperienceLevel( pPlayer );

	int nCraftingPointsExpected = 0;		

	for (int i = 1; i <= nExperienceLevel; ++i)
	{
		const PLAYERLEVEL_DATA *pLevelData = PlayerLevelGetData( pGame, pUnitData->nUnitTypeForLeveling, i );
		ASSERTX_CONTINUE( pLevelData, "Expected level data" );
		nCraftingPointsExpected += pLevelData->nCraftingPoints;
	}	
	
	return nCraftingPointsExpected;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetActualSkillPoints( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );
	const UNIT_DATA *pUnitData = UnitGetData( pPlayer );

	// start with the skill points that are unassigned
	int nSkillPointsAvailable = UnitGetBaseStat( pPlayer, STATS_SKILL_POINTS );
	
	// check all of the skills that we actually have as they are "spent" skill points
	int nSkillPointsSpent = 0;
	UNIT_ITERATE_STATS_RANGE( pPlayer, STATS_SKILL_LEVEL, tStatsSkillLevels, i, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = tStatsSkillLevels[ i ].GetParam();
		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
		ASSERT_CONTINUE(pSkillData);
		if( !pSkillData->bUsesCraftingPoints && !SkillUsesPerkPoints(pSkillData) )
		{
			nSkillPointsSpent += UnitGetBaseStat( pPlayer, STATS_SKILL_LEVEL, nSkill );	
		}
	}
	UNIT_ITERATE_STATS_END;
			
	// the starting skills were given free of charge and should not be counted as actual
	// skill points because they were not counted as the expected skill points
	for (int i = 0; i < NUM_UNIT_START_SKILLS; ++i)
	{
		int nSkill = pUnitData->nStartingSkills[ i ];
		if (nSkill != INVALID_LINK)
		{
			nSkillPointsSpent--;
		}
	}

	return nSkillPointsAvailable + nSkillPointsSpent;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetActualPerkPoints( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );

	// start with the skill points that are unassigned
	int nPerkPointsAvailable = UnitGetBaseStat( pPlayer, STATS_PERK_POINTS );
	
	// check all of the skills that we actually have as they are "spent" skill points
	int nPerkPointsSpent = 0;
	UNIT_ITERATE_STATS_RANGE( pPlayer, STATS_SKILL_LEVEL, tStatsSkillLevels, i, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = tStatsSkillLevels[ i ].GetParam();
		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
		ASSERT_CONTINUE(pSkillData);
		if( SkillUsesPerkPoints(pSkillData) )
		{
			int nSkillLevel = UnitGetBaseStat( pPlayer, STATS_SKILL_LEVEL, nSkill );
			nPerkPointsSpent += SkillGetTotalPerkPointCostForLevel(pSkillData, nSkillLevel);
		}
	}
	UNIT_ITERATE_STATS_END;
			
	return nPerkPointsAvailable + nPerkPointsSpent;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetActualCraftingPoints( 
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );


	// start with the skill points that are unassigned
	int nSkillPointsAvailable = UnitGetBaseStat( pPlayer, STATS_CRAFTING_POINTS );

	// check all of the skills that we actually have as they are "spent" skill points
	int nSkillPointsSpent = 0;
	UNIT_ITERATE_STATS_RANGE( pPlayer, STATS_SKILL_LEVEL, tStatsSkillLevels, i, MAX_SKILLS_PER_UNIT )
	{
		int nSkill = tStatsSkillLevels[ i ].GetParam();
		const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
		if( pSkillData->bUsesCraftingPoints )
		{
			nSkillPointsSpent += UnitGetBaseStat( pPlayer, STATS_SKILL_LEVEL, nSkill );	

		}
	}
	UNIT_ITERATE_STATS_END;


	int nTierPointsInvested = 0;
	int nCraftingTierPaneIDs[ 5 ];
	int nCraftingTierPaneCount(0);
	int nRecipeCategoryCount = ExcelGetCount(EXCEL_CONTEXT(NULL), DATATABLE_RECIPECATEGORIES);
	for( int t = 0; t < nRecipeCategoryCount; t++ )
	{
		const RECIPE_CATEGORY *pRecipeCategory = RecipeGetCategoryDefinition( t );
		if( pRecipeCategory &&
			pRecipeCategory->nRecipePane > 0 )
		{

			BOOL bAdd( TRUE );
			for( int i = 0; i < nCraftingTierPaneCount; i++ )
			{
				if( nCraftingTierPaneIDs[ i ] == pRecipeCategory->nRecipePane )
				{
					bAdd = FALSE;
					break;
				}
			}		
			if( bAdd )
			{
				nCraftingTierPaneIDs[ nCraftingTierPaneCount++ ] = pRecipeCategory->nRecipePane;
				nTierPointsInvested += UnitGetStat( pPlayer, STATS_SKILL_POINTS_TIER_SPENT, pRecipeCategory->nRecipePane );
			}
		}
	}
	
	return nSkillPointsAvailable + nSkillPointsSpent + nTierPointsInvested;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerValidateExperienceLevel(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pUnitData = UnitGetData( pPlayer );

	// level the player up to the correct level if they have enough experience, this was a problem
	// in mis-configured asia servers that incorrectly tried to remove a level cap, causing
	// people to gather experience, but not properly level up.
	
	// do any level ups necessary
	PlayerCheckLevelUp( pPlayer );
	
	// how many stat points are given by level ups
	int nStatPointsExpected = 0;
	int nExperienceLevel = UnitGetExperienceLevel( pPlayer );
	for (int i = 1; i <= nExperienceLevel; ++i)
	{
		const PLAYERLEVEL_DATA *pLevelData = PlayerLevelGetData( pGame, pUnitData->nUnitTypeForLeveling, i );
		ASSERTX_CONTINUE( pLevelData, "Expected level data" );
		nStatPointsExpected += pLevelData->nStatPoints;
	}
	
	// how many stat points are given from completed quests
	DWORD dwQuestIterateFlags = 0;
	SETBIT( dwQuestIterateFlags, QIF_ALL_DIFFICULTIES );
	QuestIterateQuests( pPlayer, sCountEarnedQuestStatPoints, &nStatPointsExpected, dwQuestIterateFlags, FALSE );

	// how many stat points does this player have (allocated or not) excluding ones we gave them
	// when they started a new character
	int nStatPointsActual = UnitGetBaseStat( pPlayer, STATS_STAT_POINTS );
	nStatPointsActual += UnitGetBaseStat( pPlayer, STATS_ACCURACY ) - pUnitData->nStartingAccuracy;
	nStatPointsActual += UnitGetBaseStat( pPlayer, STATS_STRENGTH ) - pUnitData->nStartingStrength;
	nStatPointsActual += UnitGetBaseStat( pPlayer, STATS_STAMINA ) - pUnitData->nStartingStamina;
	nStatPointsActual += UnitGetBaseStat( pPlayer, STATS_WILLPOWER ) - pUnitData->nStartingWillpower;

	// if they don't have as many stat points as we think they should ... give them the rest
	int nStatPointsDifference = nStatPointsExpected - nStatPointsActual;
	if (nStatPointsDifference > 0)
	{
		UnitChangeStat( pPlayer, STATS_STAT_POINTS, nStatPointsDifference );
	}

	// now lets validate their skill points and give them any that they are "missing"
	int nSkillPointsExpected = sGetExpectedSkillPoints( pPlayer );
	int nSkillPointsActual = sGetActualSkillPoints( pPlayer );
	int nSkillPointDifference = nSkillPointsExpected - nSkillPointsActual;
	if (nSkillPointDifference > 0)
	{
		PlayerGiveSkillPoint( pPlayer, nSkillPointDifference );
	}

	// now lets validate their perk points and give them any that they are "missing"
	int nPerkPointsExpected = sGetExpectedPerkPoints( pPlayer );
	int nPerkPointsActual = sGetActualPerkPoints( pPlayer );
	int nPerkPointDifference = nPerkPointsExpected - nPerkPointsActual;
	if (nPerkPointDifference > 0)
	{
		PlayerGivePerkPoint( pPlayer, nPerkPointDifference );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerAttribsRespec(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	const UNIT_DATA *pUnitData = UnitGetData( pPlayer );

	// level the player up to the correct level if they have enough experience, this was a problem
	// in mis-configured asia servers that incorrectly tried to remove a level cap, causing
	// people to gather experience, but not properly level up.
	
	// do any level ups necessary
	PlayerCheckLevelUp( pPlayer );
	
	// how many stat points are given by level ups
	int nStatPointsExpected = 0;
	int nExperienceLevel = UnitGetExperienceLevel( pPlayer );
	for (int i = 1; i <= nExperienceLevel; ++i)
	{
		const PLAYERLEVEL_DATA *pLevelData = PlayerLevelGetData( pGame, pUnitData->nUnitTypeForLeveling, i );
		ASSERTX_CONTINUE( pLevelData, "Expected level data" );
		nStatPointsExpected += pLevelData->nStatPoints;
	}
	
	// how many stat points are given from completed quests
	DWORD dwQuestIterateFlags = 0;
	SETBIT( dwQuestIterateFlags, QIF_ALL_DIFFICULTIES );
	QuestIterateQuests( pPlayer, sCountEarnedQuestStatPoints, &nStatPointsExpected, dwQuestIterateFlags, FALSE );

	// set the player's stats to their starting points
	if (AppIsHellgate())
	{
		UnitSetStat( pPlayer, STATS_ACCURACY, pUnitData->nStartingAccuracy);
		UnitSetStat( pPlayer, STATS_STRENGTH, pUnitData->nStartingStrength);
		UnitSetStat( pPlayer, STATS_STAMINA, pUnitData->nStartingStamina);
		UnitSetStat( pPlayer, STATS_WILLPOWER, pUnitData->nStartingWillpower);

		// set the number of stat points they have to spend
		UnitSetStat( pPlayer, STATS_STAT_POINTS, nStatPointsExpected );

		// this will verify equip requirements and remove items if necessary
		s_InventoryChanged(pPlayer);
	
		return TRUE;
	}

	// Tugboat uses different stats.  They can be set here.

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerValidateStartingSkills(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	const UNIT_DATA* player_data = UnitGetData( pPlayer );

	// Give the player level 1 in their starting skill(s) - only if they don't already have them
	for (int ii = 0; ii < NUM_UNIT_START_SKILLS; ii++)
	{
		if (player_data->nStartingSkills[ii] == INVALID_ID)
		{
			continue;
		}
		int nSkillLevel = SkillGetLevel( pPlayer, player_data->nStartingSkills[ii], TRUE );
		if( nSkillLevel == 0 )
		{
			SkillPurchaseLevel(pPlayer, player_data->nStartingSkills[ii], FALSE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerValidateCraftingLevel(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// now lets validate their skill points and give them any that they are "missing"
	if( AppIsTugboat() )
	{
		int nCraftingPointsExpected = sGetExpectedCraftingPoints( pPlayer );
		int nCraftingPointsActual = sGetActualCraftingPoints( pPlayer );
		int nCraftingPointDifference = nCraftingPointsExpected - nCraftingPointsActual;
		if (nCraftingPointDifference > 0)
		{
			PlayerGiveCraftingPoint( pPlayer, nCraftingPointDifference );
		}
	}


}
//----------------------------------------------------------------------------
//in the next version of the player MODS are able to have different stats based off
//of what item they are socketed with. This meant that mods couldn't have their
//stats set until they were socketed. This was flaged by a new stat. But since
//all old mods had their stats already set on them, with the new system they would
//have the old ones and then get the new stats applied. Giving double bonuses.
//so we need to set the flag that says they already have their stats applied.
//----------------------------------------------------------------------------
static void sApplyPlayerFixForModStats(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );
	if (IS_CLIENT( pGame ))
		return;
	UNIT *pItemIter = NULL;
	pItemIter = UnitInventoryIterate( pPlayer, pItemIter );
	while ( pItemIter )
	{		
		UNIT *pItem = pItemIter;
		pItemIter = UnitInventoryIterate( pPlayer, pItemIter );
		if( pItem &&
			UnitIsA( pItem, UNITTYPE_MOD ) )
		{
			UnitSetStat( pItem, STATS_ITEM_PROPS_SET, 1 );  //props on the item have been set.
		}
	}	
}


//----------------------------------------------------------------------------
//this checks to see if we modified any skill levels. If we have and a player 
//has to many skill points invested it rewards them back to redistrubute. Might
//want to put this someplace else.
//----------------------------------------------------------------------------
static void sPlayerVersionSkills(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( pGame );
	if (IS_SERVER( pGame ))
	{
		int nSkillPointsToRewardBack = 0;
		int nCraftingPointsToRewardBack = 0;
		int nPerkPointsToRewardBack = 0;
		STATS_ENTRY tStatsUnitSkills[ 200 ]; //hard coded for now. 
		int nSkillCounts = UnitGetStatsRange( pUnit, STATS_SKILL_LEVEL, 0, tStatsUnitSkills, 200 );	
		for( int t = 0; t < nSkillCounts; t++ )
		{
			int nSkillID = STAT_GET_PARAM( tStatsUnitSkills[ t ].stat );			
			const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkillID );
			if ( ! pSkillData )
				continue;
			int nSkillLevel = SkillGetLevel( pUnit, nSkillID, TRUE );
			if( nSkillLevel > pSkillData->nMaxLevel ) //to many points invested
			{
				UnitSetStat( pUnit, STATS_SKILL_LEVEL, nSkillID, pSkillData->nMaxLevel ); //set the points invested to be max
				if( pSkillData->bUsesCraftingPoints )
				{
					nCraftingPointsToRewardBack += nSkillLevel - pSkillData->nMaxLevel; //points to reward back
				}
				else if( SkillUsesPerkPoints(pSkillData) )
				{
					// This assumes that the perk point costs remain in the spreadsheet
					nPerkPointsToRewardBack += SkillGetTotalPerkPointCostForLevel(pSkillData, nSkillLevel) - SkillGetTotalPerkPointCostForLevel(pSkillData, pSkillData->nMaxLevel);
				}
				else
				{
					nSkillPointsToRewardBack += nSkillLevel - pSkillData->nMaxLevel; //points to reward back
				}
			}
		}
		if( nSkillPointsToRewardBack > 0 )
		{
			nSkillPointsToRewardBack += UnitGetStat( pUnit, STATS_SKILL_POINTS ); //add the skill points back on
			UnitSetStat( pUnit, STATS_SKILL_POINTS, nSkillPointsToRewardBack );
		}
		if( nPerkPointsToRewardBack > 0 )
		{
			nPerkPointsToRewardBack += UnitGetStat( pUnit, STATS_PERK_POINTS ); //add the skill points back on
			UnitSetStat( pUnit, STATS_PERK_POINTS, nPerkPointsToRewardBack );
		}
		if( nCraftingPointsToRewardBack > 0 )
		{
			nCraftingPointsToRewardBack += UnitGetStat( pUnit, STATS_CRAFTING_POINTS ); //add the skill points back on
			UnitSetStat( pUnit, STATS_CRAFTING_POINTS, nCraftingPointsToRewardBack );
		}

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerFixExperienceMarkers(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	BOOL bSetMarkers = TRUE;
		
	// at one point, we allowed some accounts in the beta to progress through the
	// full game, but unfortunately, if any characters out there had reached the max level
	// before the patch, they were prevented from leveling up anymore because
	// the next experience level had already been cleared/capped, we want to try to 
	// recover those players now
	if (UnitGetVersion( pPlayer ) <= USV_MAX_LEVEL_BROKEN_FOR_BETA_FULL_GAME_ACCOUNTS)
	{
		int nExpNextLevel = UnitGetStat( pPlayer, STATS_EXPERIENCE_NEXT );
		if (nExpNextLevel == 0)
		{
			GAMECLIENT *pClient = UnitGetClient( pPlayer );
			ASSERTV_RETURN( pClient, "Expected client" );
			if (AppIsBeta() && ClientHasPermission( pClient, BADGE_PERMISSION_BETA_FULL_GAME ))
			{
				bSetMarkers = TRUE;
			}
			
		}
		
	}
	
	// a misconfigured asia server broke this for lots of players
	if (UnitGetVersion( pPlayer ) <= USV_BROKEN_EXPERIENCE_MARKERS)	
	{
		bSetMarkers = TRUE;
	}
	
	// set the markers
	if (bSetMarkers == TRUE)
	{
		int nExperienceLevel = UnitGetExperienceLevel( pPlayer );
		sPlayerSetExperienceMarkersByLevel( pPlayer, nExperienceLevel + 1 );
	}
	
}
//----------------------------------------------------------------------------
//Zeros out a stat value for all params of the stat
//----------------------------------------------------------------------------
inline void sPlayerZeroAllInstancesOfStat( UNIT *pPlayer, int nStat )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( nStat != INVALID_ID );
	STATS_ENTRY nStatEntries[ 500 ]; //hard coded for now. Testing..
	int nStatEntryCount = UnitGetStatsRange( pPlayer, nStat, 0, nStatEntries, 500 );	
	for( int t = 0; t < nStatEntryCount; t++ )
	{
		int nParamID = STAT_GET_PARAM( nStatEntries[ t ].stat );			
		if( nParamID != INVALID_ID )
		{		
			UnitSetStat( pPlayer, nStat, nParamID, 0 );
		}
	}	

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * s_sGivePlayerItemOnStartup( 
	UNIT *pPlayer,
	int nItemClass)
{
	ASSERT_RETNULL( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETNULL( pGame );

	ITEM_SPEC tItemSpec;
	tItemSpec.nItemClass = nItemClass;
	if ( tItemSpec.nItemClass == INVALID_ID )
		return NULL;
	tItemSpec.nLevel = 1;
	SETBIT( tItemSpec.dwFlags, ISF_IDENTIFY_BIT );			// auto identify affixes
	if( AppIsHellgate() )
	{
		SETBIT( tItemSpec.dwFlags, ISF_USE_SPEC_LEVEL_ONLY_BIT );  // use only the experience level we provide
	}
	SETBIT( tItemSpec.dwFlags, ISF_RESTRICT_AFFIX_LEVEL_CHANGES_BIT );	// do not allow affixes to change the level of this item

	UNIT* pItem = s_ItemSpawn(pGame, tItemSpec, NULL);

	int nDestinationLocation;
	int nInvX, nInvY;
	if(ItemGetOpenLocation( pPlayer, pItem, TRUE, &nDestinationLocation, &nInvX, &nInvY ))
	{
		// this item can now be sent to the client (must
		UnitSetCanSendMessages( pItem, TRUE );

		if (!UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pItem, nDestinationLocation))
		{
			return pItem;
		}
	}
	else
	{
		UnitFree(pItem);
		return NULL;
	}

	return pItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CHARACTER_CLASS_DATA * PlayerGetCharacterClass(
	UNIT * pPlayer )
{
	int nCharacterClass = CharacterClassGet( pPlayer );
	if ( nCharacterClass == INVALID_ID )
		return NULL;

	return CharacterClassGetData( nCharacterClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sGiveRespecPerksItem( 
	UNIT *pPlayer )
{
	BOOL bNeedsRespec = FALSE;
	int nUnitVersion = UnitGetVersion( pPlayer );
	const CHARACTER_CLASS_DATA * pCharacterClassData = PlayerGetCharacterClass( pPlayer );
	if ( pCharacterClassData->nUnitVersionToGetPerkRespec != INVALID_ID &&
		 nUnitVersion <= pCharacterClassData->nUnitVersionToGetPerkRespec )
	{
		bNeedsRespec = TRUE;
	}
 
	DWORD* flags = AppGetFlags();
	if(!bNeedsRespec && (TESTBIT(flags, AF_RESPEC)))
	{
		bNeedsRespec = TRUE;
	}

	if(bNeedsRespec) //If we're giving them one, wipe out all that they have
		s_PlayerLimitInventoryType(pPlayer, UNITTYPE_RESPEC_PERKS_UNIQUE, 0);
	else //Otherwise, let them keep 1
		s_PlayerLimitInventoryType(pPlayer, UNITTYPE_RESPEC_PERKS_UNIQUE, 1);

	if (bNeedsRespec)
		UnitSetStat( pPlayer, STATS_RESPEC_PERKS_ITEM_NEEDED, pCharacterClassData->nUnitVersionToGetPerkRespec, 1 );

	if ( ! UnitGetStat( pPlayer, STATS_RESPEC_PERKS_ITEM_NEEDED, pCharacterClassData->nUnitVersionToGetPerkRespec ))
		return;

	if( s_sGivePlayerItemOnStartup( pPlayer, GlobalIndexGet( GI_ITEM_RESPEC_PERKS ) ) )
	{
		UnitSetStat(pPlayer, STATS_RESPEC_PERKS_ITEM_NEEDED, pCharacterClassData->nUnitVersionToGetPerkRespec, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_sGiveRespecItem( 
	UNIT *pPlayer )
{
	BOOL bNeedsRespec = FALSE;
	int nUnitVersion = UnitGetVersion( pPlayer );
	if ( nUnitVersion <= USV_GRANT_RESPEC_ITEM && PlayerIsSubscriber( pPlayer ) )
		bNeedsRespec = TRUE;
	else 
	{
		const CHARACTER_CLASS_DATA * pCharacterClassData = PlayerGetCharacterClass( pPlayer );
		if ( pCharacterClassData->nUnitVersionToGetSkillRespec != INVALID_ID &&
			 nUnitVersion <= pCharacterClassData->nUnitVersionToGetSkillRespec )
			 bNeedsRespec = TRUE;
	}
 
	DWORD* flags = AppGetFlags();
	//ASSERT(flags);
	if(!bNeedsRespec && (TESTBIT(flags, AF_RESPEC)))
	{
		bNeedsRespec = TRUE;
	}

	if(bNeedsRespec) //If we're giving them one, wipe out all that they have
		s_PlayerLimitInventoryType(pPlayer, UNITTYPE_RESPEC_SKILLS_UNIQUE, 0);
	else //Otherwise, let them keep 1
		s_PlayerLimitInventoryType(pPlayer, UNITTYPE_RESPEC_SKILLS_UNIQUE, 1);

	//ASSERT(flags);

	if (bNeedsRespec)
		UnitSetStat( pPlayer, STATS_RESPEC_ITEM_NEEDED, USV_GRANT_RESPEC_ITEM, 1 );

	if ( ! UnitGetStat( pPlayer, STATS_RESPEC_ITEM_NEEDED, USV_GRANT_RESPEC_ITEM ))
		return;

	if ( s_sGivePlayerItemOnStartup( pPlayer, GlobalIndexGet( GI_ITEM_RESPEC ) ) )
	{
		UnitSetStat(pPlayer, STATS_RESPEC_ITEM_NEEDED, USV_GRANT_RESPEC_ITEM, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sRemapVisitedLevelToBitfield(
	UNIT* pPlayer)
{
	int nRows = ExcelGetNumRows(NULL, DATATABLE_LEVEL);
	for(int ii = 0; ii < nRows; ii++)
	{
		int stat = UnitGetStat(pPlayer, STATS_PLAYER_VISITED_LEVEL, ii);
	    const LEVEL_DEFINITION *pLevelDef;
		pLevelDef = LevelDefinitionGet(ii);
		
		if(stat)
			LevelMarkVisited(pPlayer, ii, (WORLD_MAP_STATE)stat, DIFFICULTY_NORMAL);
	}
	//Nightmare stats: 
	//For now...just give them every town...its nightmare; they already know everything!...but may not have waypoints
	// ...AHA! we can give them the waypoint for every town they have a waypoint for on nightmare!
	if(DifficultyGetMax(pPlayer) == DIFFICULTY_NIGHTMARE)
	{
		
		for(int ii = 0; ii < nRows; ii++)
		{
			const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet(ii);
			if(pLevelDef && pLevelDef->bTown)
			{
				LevelMarkVisited(pPlayer, ii, WORLDMAP_VISITED, DIFFICULTY_NIGHTMARE);
				//UnitSetStat(pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, ii, DIFFICULTY_NIGHTMARE, WORLDMAP_VISITED);
			}
		}

	}
}

/*This is an unfinished stub...don't use yet
Also, this will only work when the # of params doesn't change*/
static void s_sRemapBitfieldStat(
	UNIT* pPlayer,
	int inStat,
	int outStat)
{
	int MAX_PARAM1 = 0;
	int MAX_VALUE = 0;
	for(int jj = 0; jj < MAX_PARAM1; jj++)
	{

		for(int i = 0; i < MAX_VALUE; i++)
		{
			UnitSetStat(pPlayer, inStat,jj, UnitGetStat(pPlayer, outStat, jj));
		}
	}


}

//----------------------------------------------------------------------------
//fixes a bug with people dropping the totem and not being able to complete the quest
//----------------------------------------------------------------------------
void sPlayerFixTotemQuest(UNIT *pPlayer )
{
	BOOL bFirstQuestComplete = QuestIsQuestTaskComplete( pPlayer, QuestTaskDefinitionGet("BeastQuestTask_ConfrontTinus") );
	BOOL bSecondQuestNotComplete = QuestIsQuestTaskComplete( pPlayer, QuestTaskDefinitionGet("BeastQuestTask_11_SummonFerus") );
	const UNIT_DATA *pItemData = (const UNIT_DATA *)ExcelGetDataByStringIndex( EXCEL_CONTEXT(), DATATABLE_ITEMS, "Totem_of_Ferus" );
	ASSERT_RETURN( pItemData );
	int nClassID = pItemData->nUnitDataBaseRow;	
	if( bFirstQuestComplete &&
		!bSecondQuestNotComplete &&
		UnitInventoryCountItemsOfType( pPlayer, nClassID, 0 ) == 0)
	{
		//got to give the item to the player now
		s_sGivePlayerItemOnStartup( pPlayer, nClassID );
	}

}
//----------------------------------------------------------------------------
//Find the items and either deletes them and recreates them or just resets the props
//----------------------------------------------------------------------------
void sPlayerResetItem( UNIT *pPlayer,
					   const UNIT_DATA *pUnitData,
					   BOOL bRecreate )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( pUnitData );

	UNIT* pItem = NULL;
	pItem = UnitInventoryIterate( pPlayer, pItem, 0 );	
#define MAX_UNITS	500
	UNIT *pUnitsToRecreate[ MAX_UNITS ];	
	int nUnitCount( 0 );
	ZeroMemory( pUnitsToRecreate, sizeof( UNIT * ) * MAX_UNITS );		
	while ( pItem != NULL)
	{		
		if( pItem->pUnitData == pUnitData )
		{
			if( bRecreate )
			{
				//it can only be in the players inventory
				if( UnitGetUltimateOwner(pItem) == pPlayer )
				{
					pUnitsToRecreate[ nUnitCount++ ] = pItem;
				}
			}
			else
			{

				s_ItemPropsInit( UnitGetGame( pPlayer ), 
					             pItem,
								 pPlayer,
								 (( UnitGetUltimateOwner(pItem) == pPlayer )?NULL:UnitGetOwnerUnit( pItem ) ) );

			}
		}
		pItem = UnitInventoryIterate( pPlayer, pItem, 0 );
	}	
	if( bRecreate == FALSE )
	{
		return; //we're done
	}
	for( int t = 0; t < nUnitCount; t++ )
	{
		if( pUnitsToRecreate[ t ] != NULL )
		{
			int nCount = ItemGetQuantity( pUnitsToRecreate[ t ] );
			int nClass = UnitGetClass( pUnitsToRecreate[ t ] );
			UnitFree( pUnitsToRecreate[ t ] );
			UNIT *pNewItem = s_sGivePlayerItemOnStartup( pPlayer, nClass );
			ASSERT( pNewItem );		
			ItemSetQuantity( pNewItem, nCount );
		}
	}

}

//----------------------------------------------------------------------------
//Iterate through the player's inventory and set STATS_GAME_VARIANT for all items
//----------------------------------------------------------------------------
void PlayerInventorySetGameVariants( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitIsPlayer(pPlayer) );

	DWORD dwGameVariant = GameVariantFlagsGetStaticUnit(pPlayer);
	ASSERT_RETURN(dwGameVariant <= 0xff); // STATS_GAME_VARIANT is only 8 bits for now

	UNIT * pItem = NULL;
	while ((pItem = UnitInventoryIterate( pPlayer, pItem )) != NULL)
	{
		if (UnitGetStat(pItem, STATS_GAME_VARIANT) == -1)
		{
			UnitSetStat(pItem, STATS_GAME_VARIANT, (int)dwGameVariant);
		}
	}
}

//----------------------------------------------------------------------------
//Iterate through the player's shared stash and place items into appropriate
//section based on game variant
//----------------------------------------------------------------------------
void PlayerInventorySplitSharedStash( UNIT *pPlayer )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitIsPlayer(pPlayer) );

	DWORD dwGameVariant = GameVariantFlagsGetStaticUnit(pPlayer);
	ASSERT_RETURN(dwGameVariant <= 0xff); // STATS_GAME_VARIANT is only 8 bits for now

	UNIT * pItem = NULL;
	while ((pItem = UnitInventoryLocationIterate( pPlayer, INVLOC_SHARED_STASH, pItem )) != NULL)
	{
		DWORD dwGameVariant = UnitGetStat(pItem, STATS_GAME_VARIANT);
		ASSERT_CONTINUE(dwGameVariant >= 0);

		const int nMinY = dwGameVariant * SHARED_STASH_SECTION_HEIGHT;
		const int nMaxY = ((dwGameVariant + 1) * SHARED_STASH_SECTION_HEIGHT) - 1;

		int location, x, y;
		UnitGetInventoryLocation(pItem, &location, &x, &y);
		ASSERT_CONTINUE(location == INVLOC_SHARED_STASH);

		if (y < nMinY || y > nMaxY)
		{
			const int newY = (y % SHARED_STASH_SECTION_HEIGHT) + (SHARED_STASH_SECTION_HEIGHT * dwGameVariant);
			ASSERT_CONTINUE(newY >= nMinY && newY <= nMaxY);

			InventoryChangeLocation(
				pPlayer,
				pItem,
				INVLOC_SHARED_STASH,
				x,
				newY,
				0);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerVersion(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// version the player skills for tugboat
	if (AppIsTugboat())
	{				
		sPlayerVersionSkills( pPlayer );

		// always reset this, in case we twiddle it.
		UnitSetStat( pPlayer, STATS_DEFENDERS_VULNERABILITY_ATTACKER, UNITTYPE_PLAYER, PLAYER_VS_PLAYER_DMG_MOD );
		UnitSetStat(pPlayer, STATS_LUCK_MAX, 20000); //mythos screwed up big with luck and now people just have to much. We had to change the max value.

	}
	//fixes stat issue with potions.
	//Also fixed issue with quest
	if( AppIsTugboat() &&
		UnitGetVersion( pPlayer ) <= USV_POTION_REBALANCING )
	{		
		sPlayerResetItem( pPlayer, (const UNIT_DATA* )ExcelGetDataByStringIndex( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_ITEMS, "minor health potion" ), FALSE ); 
		sPlayerResetItem( pPlayer, (const UNIT_DATA* )ExcelGetDataByStringIndex( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_ITEMS, "health potion" ), FALSE ); 
		sPlayerResetItem( pPlayer, (const UNIT_DATA* )ExcelGetDataByStringIndex( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_ITEMS, "antidote" ), FALSE ); 
		sPlayerResetItem( pPlayer, (const UNIT_DATA* )ExcelGetDataByStringIndex( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_ITEMS, "minor mana potion" ), FALSE ); 
		sPlayerResetItem( pPlayer, (const UNIT_DATA* )ExcelGetDataByStringIndex( EXCEL_CONTEXT( UnitGetGame( pPlayer ) ), DATATABLE_ITEMS, "mana potion" ), FALSE ); 
	}

	// fixes issue with power and mana cost modification stat being saved on the player
	if( AppIsTugboat() &&
		UnitGetVersion( pPlayer) <= USV_SKILL_STAT_FIX_FOR_MANA_AND_POWER_MOD )
	{
		sPlayerZeroAllInstancesOfStat( pPlayer, STATS_POWER_COST_PCT_SKILL );
		sPlayerZeroAllInstancesOfStat( pPlayer, STATS_COOL_DOWN_PCT_SKILL );
	}

	if( AppIsTugboat() /*&&
		UnitGetVersion( pPlayer) <= USV_BITFIELD_CRAFTING_STATS*/ )
	{
		sPlayerValidateCraftingLevel( pPlayer );
	}

	// for asia ... because they turned off the level cap, some players got screwed and we 
	// want to try to fix them, but seems like a good thing we can leave in right now
	if (AppIsHellgate() &&
		UnitGetVersion( pPlayer ) <= USV_POSSIBLY_BROKEN_EXPERIENCE_LEVEL_BENEFITS)
	{
		sPlayerValidateExperienceLevel( pPlayer );
	}

	if( AppIsTugboat() )
	{
		sPlayerValidateStartingSkills( pPlayer );
	}

	// versions that incorrectly saved the hotkeys have caused some items to get stuck
	// in the hotkey inventory slots, return those to the players inventory
	if (UnitGetVersion( pPlayer ) <= UNITSAVE_VESRION_HOTKEYS_WRONG_SELECTOR_2007_07_02)
	{
		s_HotkeysReturnItemsToStandardInventory( pPlayer );
	}

	// we need to fix experience markers
	sPlayerFixExperienceMarkers( pPlayer );
	
	// fix some broken waypoints in nightmare mode
	if (UnitGetVersion( pPlayer ) <= USV_NIGHTMARE_WAYPOINTS_BROKEN)
	{
		sPlayerFixNightmareWaypoints(pPlayer);
	}


	// some characters need a respec item because they are older than the new skill changes
	if (AppIsHellgate() )
	{
		s_sGiveRespecItem(pPlayer);
		s_sGiveRespecPerksItem(pPlayer);
	}

	// fix quest data
	if( AppIsHellgate() )
	{
		s_QuestVersionPlayer( pPlayer );
	}
			
	// new newest version mods can have different values depending on the item they are socketed with. Tugboat only for now
	if( AppIsTugboat() &&
		UnitGetVersion( pPlayer ) <= USV_MULTIPLE_STAT_VALUES_FOR_MODS_ALLOWED )
	{
		sApplyPlayerFixForModStats( pPlayer );
	}

	// We used to store an individual 2 bit value for each level on the map; now its bitfields!
	if(AppIsHellgate() && UnitGetVersion(pPlayer) <= USV_BITFIELD_LEVEL_VISITED_STATS)
	{
		//...arg, we could just use a generalized "remap stats to bitfield"...but we're also adding a nightmare parameter
		s_sRemapVisitedLevelToBitfield(pPlayer);
	}

	if(UnitGetVersion(pPlayer) <= USV_NONTOWN_WAYPOINTS_BROKEN)
	{
		PlayerFixNontownWaypoints(pPlayer);
	}

	if(UnitGetVersion(pPlayer) <= USV_INVENTORY_NEEDS_GAME_VARIANTS)
	{
		PlayerInventorySetGameVariants(pPlayer);
	}

	if(UnitGetVersion(pPlayer) <= USV_SPLIT_SHARED_STASH)
	{
		PlayerInventorySplitSharedStash(pPlayer);
	}

	if(UnitGetVersion(pPlayer) <= USV_WARDROBE_AFFIX_COUNT_CHANGED_FROM_SIX)
	{
		UnitSetWardrobeChanged(pPlayer, TRUE);
		WardrobeUpdateFromInventory(pPlayer); 		
	}


	// The following isn't a player fix as much of an account fix...that needs a player in a certain state to activate.
	// Since the root cause of the problem hasn't been fixed yet, no version # yet - bmanegold
	if(AppIsHellgate())
	{
		GAMECLIENT * pClient = UnitGetClient( pPlayer );
		if(UnitGetStat(pPlayer, STATS_DIFFICULTY_MAX) >= DIFFICULTY_NIGHTMARE) 
			if ( !ClientHasBadge( pClient, ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN ) )
				GameClientAddAccomplishmentBadge( pClient, ACCT_ACCOMPLISHMENT_REGULAR_MODE_BEATEN );
	}

	// one day you might want to put things here like ... some absurd health value, or regeneration
	// rate, or whatever.  Now is you change to "fix" any old characters and bring them up to date
	// with the current set of game data

	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsHardcore(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );

	// check state	
	if (UnitHasState( pGame, pPlayer, STATE_HARDCORE ))
	{
		return TRUE;
	}

	return FALSE;
			   
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsPVPOnly(
					  UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );

	// check state	
	if (UnitHasState( pGame, pPlayer, STATE_PVPONLY ))
	{
		return TRUE;
	}

	return FALSE;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsHardcoreDead(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// check state
	if (UnitHasState( pGame, pPlayer, STATE_HARDCORE_DEAD ))
	{
		return TRUE;
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsInPVPWorld(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	if( !UnitIsPlayer( pPlayer ) )
	{
		return FALSE;
	}
	GAME *pGame = UnitGetGame( pPlayer );

	// check state
	if (UnitHasState( pGame, pPlayer, STATE_PVPWORLD ))
	{
		return TRUE;
	}

	return FALSE;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsLeague(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );

	// check state
	if (UnitHasState( pGame, pPlayer, STATE_LEAGUE ))
	{
		return TRUE;
	}

	return FALSE;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerIsElite(
						  UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );

	// check state
	if (UnitHasState( pGame, pPlayer, STATE_ELITE ))
	{
		return TRUE;
	}

	return FALSE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL PlayerIsSubscriber(
	UNIT *pPlayer,
	BOOL bForceBadgeCheck /* = FALSE */)
{
	if(!AppIsHellgate()) // We don't have subscribers in Mythos...so everyone is a subscriber! yay!
		return TRUE;

#if ISVERSION(CLIENT_ONLY_VERSION)
	GAME* pClGame = AppGetCltGame();
	if(pClGame)
	{
		UNIT* pUnit = GameGetControlUnit(pClGame);
		if(pUnit && UnitHasState(pClGame, pUnit, STATE_TEST_CENTER_SUBSCRIBER))
			return TRUE;
	}
#endif
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

	GAME* pGame = UnitGetGame(pPlayer);
	ASSERT_RETFALSE(pGame);

	if(bForceBadgeCheck || IS_SERVER(pGame) || UnitIsControlUnit(pPlayer))
	{
		return UnitHasBadge(pPlayer, ACCT_TITLE_SUBSCRIBER) || (AppTestFlag(AF_TEST_CENTER_SUBSCRIBER) && UnitHasBadge(pPlayer, ACCT_TITLE_TEST_CENTER_SUBSCRIBER));
	}
	else
	{
		GAME* pGame = UnitGetGame(pPlayer);
		ASSERTX_RETFALSE(pGame, "Expected player to have a game");
		return UnitHasState(pGame, pPlayer, STATE_SUBSCRIBER);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetupWhisperTo(
	UNIT *pPlayerOther)
{
	ASSERTX_RETURN( pPlayerOther, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayerOther ), "Expected player" );

#if !ISVERSION(SERVER_VERSION)
	const int MAX_STRING = 512;
	WCHAR uszUnitName[ MAX_STRING ];

	UnitGetName( pPlayerOther, uszUnitName, MAX_STRING, 0 );
	c_PlayerSetupWhisperTo(uszUnitName);
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetupWhisperTo(
	LPCWSTR uszUnitName)
{
	ASSERTX_RETURN( uszUnitName, "Expected unit name" );

#if !ISVERSION(SERVER_VERSION)
	const int MAX_STRING = 512;
	WCHAR strNewInput[ MAX_STRING ];
	WCHAR uszCommand[ MAX_STRING ];
	UISlashCommand( GS_CCMD_CHAT_WHISPER, uszCommand, MAX_STRING );
	PStrCopy(strNewInput, uszCommand, MAX_STRING );
	PStrCat(strNewInput, uszUnitName, MAX_STRING );
	//PStrCat(strNewInput, L" ", MAX_STRING );

	ConsoleSetChatContext(CHAT_TYPE_GAME);
	ConsoleActivateEdit(FALSE, TRUE);	
	ConsoleSetInputLine( strNewInput, ConsoleInputColor(), 0);
	ConsoleHandleInputChar(AppGetCltGame(), VK_SPACE, 0);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_GiveBadgeRewards(UNIT* pUnit)
{
	ASSERT_RETURN( UnitIsA( pUnit, UNITTYPE_PLAYER ) );
	ASSERT_RETURN(IS_SERVER(pUnit));

	GAME* pGame = UnitGetGame(pUnit);
	int nNumBadgeRewards = ExcelGetNumRows(pGame, DATATABLE_BADGE_REWARDS);
	for(int nBadgeReward = 0; nBadgeReward < nNumBadgeRewards; nBadgeReward++)
	{
		BADGE_REWARDS_DATA* pBadgeReward = (BADGE_REWARDS_DATA*)ExcelGetData(pGame, DATATABLE_BADGE_REWARDS, nBadgeReward);

		if(!pBadgeReward)
			continue;

		if(!UnitHasBadge(pUnit, pBadgeReward->m_nBadge))
			continue;

		if(pBadgeReward->m_nItem != INVALID_LINK && UnitGetStat(pUnit, STATS_BADGE_REWARD_RECEIVED, nBadgeReward) == 0)
		{
			BOOL bBadgeRewardReceived = FALSE;
			if(pBadgeReward->m_nDontApplyIfPlayerHasRewardItemFor != INVALID_LINK)
			{
				BADGE_REWARDS_DATA* pBadgeRewardOther = (BADGE_REWARDS_DATA*)ExcelGetData(pGame, DATATABLE_BADGE_REWARDS, pBadgeReward->m_nDontApplyIfPlayerHasRewardItemFor);
				if(pBadgeRewardOther && pBadgeRewardOther->m_nItem != INVALID_LINK)
				{
					DWORD dwIterateFlags = 0;
					//SETBIT(dwIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT);
					int nCount = UnitInventoryCountItemsOfType(pUnit, pBadgeRewardOther->m_nItem, dwIterateFlags);
					if(nCount > 0)
					{
						bBadgeRewardReceived = TRUE;
					}
				}
			}

			if ( !bBadgeRewardReceived && s_sGivePlayerItemOnStartup( pUnit, pBadgeReward->m_nItem ) )
			{
				bBadgeRewardReceived = TRUE;
			}

			if(bBadgeRewardReceived)
			{
				UnitSetStat(pUnit, STATS_BADGE_REWARD_RECEIVED, nBadgeReward, 1);
			}
		}
		if(pBadgeReward->m_nState != INVALID_LINK && !UnitHasState(pGame, pUnit, pBadgeReward->m_nState))
		{
			s_StateSet(pUnit, pUnit, pBadgeReward->m_nState, 0);
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerJoinLog(
	GAME *pGame,
	UNIT *pPlayer,
	APPCLIENTID idAppClient,
	const char *pszFormat,
	...)
{
#if ISVERSION(SERVER_VERSION)

	// bail out if we're not doing the log
	if (gbEnablePlayerJoinLog)
	{
		const int MAX_MESSAGE = 2048;
		char szMessage[ MAX_MESSAGE ];
		va_list args;
		va_start( args, pszFormat );
		PStrVprintf( szMessage, MAX_MESSAGE, pszFormat, args );
				
		// get unit name (if any)
		UNITID idUnit = INVALID_ID;
		char szUnitName[ MAX_CHARACTER_NAME ];
		PStrCopy( szUnitName, "NO UNIT", MAX_CHARACTER_NAME );
		if (pPlayer)
		{
			WCHAR uszUnitName[ MAX_CHARACTER_NAME ];
			UnitGetName( pPlayer, uszUnitName, MAX_CHARACTER_NAME, 0 );
			PStrCvt( szUnitName, uszUnitName, MAX_CHARACTER_NAME );
			idUnit = UnitGetId( pPlayer );
		}
		
		char szAccountCharacterName[ MAX_CHARACTER_NAME ];
		PStrCopy( szAccountCharacterName, "NO NAME", MAX_CHARACTER_NAME );
		CLIENTSYSTEM *pClientSystem = AppGetClientSystem();
		UNIQUE_ACCOUNT_ID idAccount = ClientGetUniqueAccountId( pClientSystem, idAppClient );
		const WCHAR *puszAccountCharacterName = ClientGetCharName( pClientSystem, idAppClient );
		if (puszAccountCharacterName)
		{
			PStrCvt( szAccountCharacterName, puszAccountCharacterName, MAX_CHARACTER_NAME );
		}
		
		// do the trace
		UINT64 i64Time = AppCommonGetCurTime();
		TraceVerbose( 
			TRACE_FLAG_GAME_MISC_LOG, 
			"JOIN_LOG: Time:%I64d AppCltID:%d AcctID:%I64d AcctChar:%s UnitName:%s UnitId:%d Msg:%s", 
			i64Time,
			idAppClient,
			idAccount,
			szAccountCharacterName,
			szUnitName,
			idUnit,
			szMessage);

	}
		
#endif
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerInvite(
	WCHAR *puszPlayerName)
{
#if !ISVERSION(SERVER_VERSION)

	ASSERTX_RETURN( puszPlayerName, "Expected player name" );
	UNIT *pPlayer = UIGetControlUnit();
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// cannot invite a player if our party has too many members already
	if (c_PlayerGetPartyMemberCount() >= MAX_PARTY_MEMBERS)
	{
		UIShowGenericDialog(
			GlobalStringGet( GS_PARTY_INVITE_FAILED_HEADER ),
			GlobalStringGet( GS_PARTY_INVITE_FAILED_PARTY_COUNT_MAX ) );
	}
	else
	{

		// setup message
		CHAT_REQUEST_MSG_INVITE_PARTY_MEMBER tInviteMessage;
		tInviteMessage.TagMemberToInvite = puszPlayerName;

		// send to chat server
		c_ChatNetSendMessage( &tInviteMessage, USER_REQUEST_INVITE_PARTY_MEMBER );

	}
	
#endif		
}

#if (0)
// close any town portals, which may now point to a new game instance (which can allow players to skip the levels leading up to a destination they would like to reset, like a boss) -cmarch

//player's reserved game is not equal to party's reserved game, and valid. that must abndon their game
BOOL bPlayerMustAbandonReservedGame = s_PlayerMustAbandonGameForParty( pPlayer, idParty );

if( AppIsHellgate() && bPlayerMustAbandonReservedGame ) 
{
	s_TownPortalsClose( pPlayer );
}

// where is this unit now
if (bPlayerMustAbandonReservedGame && UnitIsInInstanceLevel(pPlayer))
{
	// when joining a party, we will sometimes need to boot you
	// from the server because you're no longer allowed to play in the instance

	// if this unit is in an instance, warp them back to town
	if( AppIsHellgate() )
	{
		DWORD dwLevelRecallFlags = 0;
		SETBIT( dwLevelRecallFlags, LRF_FORCE );
		SETBIT( dwLevelRecallFlags, LRF_BEING_BOOTED );
		s_LevelRecall( pGame, pPlayer, dwLevelRecallFlags );
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerInviteAccept( 
	PGUID guidInviter,
	BOOL bAbandonGame)
{
#if !ISVERSION(SERVER_VERSION)
	if (bAbandonGame)
	{
		UIShowExitInstanceForPartyJoinDialog(guidInviter);
	}
	else
	{
		CHAT_REQUEST_MSG_ACCEPT_PARTY_INVITE acceptMsg;
		acceptMsg.guidInvitingMember = guidInviter;
		c_ChatNetSendMessage(&acceptMsg, USER_REQUEST_ACCEPT_PARTY_INVITE);
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerPartyJoinConfirm( 
	BOOL bAbandonGame )
{
#if !ISVERSION(SERVER_VERSION)
	if (bAbandonGame)
	{
		UIShowExitInstanceForPartyJoinDialog();
	}
	else
	{
		UIDoJoinParty();
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
// Determines if a player must abandon their game on party join
//----------------------------------------------------------------------------
BOOL s_PlayerMustAbandonGameForParty(
	UNIT *pPlayer)
{
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE(pGame && IS_SERVER(pGame));

	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTV_RETFALSE( pClient, "Expected client" );
	APPCLIENTID idAppClient = ClientGetAppId( pClient );
	DWORD dwReserveKey = 0;
	GAMEID idGameReservedForPlayer = 
		UnitIsInInstanceLevel(pPlayer) ? 
		GameGetId( pGame ) :
		ClientSystemGetReservedGame( AppGetClientSystem(), idAppClient, dwReserveKey );
		
	return idGameReservedForPlayer != INVALID_GAMEID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerJoinedParty(
	UNIT *pPlayer,
	PARTYID idParty)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(idParty != INVALID_ID);

	ASSERTV_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );

	GAME *pGame = UnitGetGame( pPlayer );

	// close any town portals, which may now point to a new game instance (which can allow players to skip the levels leading up to a destination they would like to reset, like a boss) -cmarch

	//player's reserved game is not equal to party's reserved game, and valid. that must abndon their game
	BOOL bPlayerMustAbandonReservedGame = FALSE;
	
	if( AppIsHellgate() )
	{
		bPlayerMustAbandonReservedGame = s_PlayerMustAbandonGameForParty( pPlayer );
	}

	if( bPlayerMustAbandonReservedGame ) 
	{
		s_TownPortalsClose( pPlayer );
	
		// where is this unit now
		if( UnitIsInInstanceLevel(pPlayer))
		{
			// when joining a party, we will sometimes need to boot you
			// from the server because you're no longer allowed to play in the instance

			// if this unit is in an instance, warp them back to town
			DWORD dwLevelRecallFlags = 0;
			SETBIT( dwLevelRecallFlags, LRF_FORCE );
			SETBIT( dwLevelRecallFlags, LRF_BEING_BOOTED );
			s_LevelRecall( pGame, pPlayer, dwLevelRecallFlags );
		}	
	}

#endif //!ISVERSION(CLIENT_ONLY_VERSION)		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerLeftParty(
	UNIT *pPlayer,
	PARTYID idParty,
	PARTY_LEAVE_REASON eLeaveReason)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

	ASSERTV_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );
	GAME *pGame = UnitGetGame( pPlayer );
	
	// if removing from party
	if (idParty == INVALID_ID)
	{
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		ASSERTV_RETURN( pLevel, "Expected level of player" );

		// this player left a party, if they are in a level that allows auto parties to form
		// we will start the auto party matchmaker
		s_PartyMatchmakerEnable( pPlayer, AUTO_PARTY_UPDATE_DELAY_IN_TICKS );
		
		// clear any reserved game
		GAMECLIENT *pClient = UnitGetClient( pPlayer );
		ASSERTV_RETURN( pClient, "Expected client" );
		ClientSystemClearReservedGame( AppGetClientSystem(), ClientGetAppId( pClient ), pClient );

		// close any party portals in this game instance that are leading to this player who
		// is no longer in a party
		s_WarpPartyMemberCloseLeadingTo( pGame, UnitGetGuid( pPlayer ) );

		// close any town portals, which may now point to a new game instance (which can allow players to skip the levels leading up to a destination they would like to reset, like a boss) -cmarch
		if( AppIsHellgate() )
		{
			s_TownPortalsClose( pPlayer );
		}

		// where is this unit now
		if (LevelAllowsUnpartiedPlayers( pLevel ) == FALSE)
		{
			
			// when being removed from a party, we will sometimes need to boot you
			// from the server because you're no longer allowed to play in the instance
			switch (eLeaveReason)
			{
			
				//----------------------------------------------------------------------------
				case PARTY_MEMBER_KICKED_OUT:
				case PARTY_MEMBER_LEFT_PARTY:
				case PARTY_MEMBER_PARTY_DISBANDED:
				{
				
					// band-aid ... don't let people use the party system to 
					// cheat experience penalties
					if (IsUnitDeadOrDying( pPlayer ))
					{
						s_sPlayerGiveExperiencePenalty( pPlayer, 0 );
					}
					
					// this game client can no longer reserve a game as they exit
					GameClientSetFlag( pClient, GCF_CANNOT_RESERVE_GAME, TRUE );

					// if this unit is in an action level ( and not in a duel), warp them back to town
					if( AppIsHellgate() &&
						UnitGetStat(pPlayer, STATS_PVP_DUEL_OPPONENT_ID) == INVALID_ID )
					{
						DWORD dwLevelRecallFlags = 0;
						SETBIT( dwLevelRecallFlags, LRF_FORCE );
						SETBIT( dwLevelRecallFlags, LRF_BEING_BOOTED );
						s_LevelRecall( pGame, pPlayer, dwLevelRecallFlags );
					}
					break;
				
				}

				//----------------------------------------------------------------------------
				default:
				{
					break;
				}
				
			}
			
		}
		
		// if there are no clients in game with no party anymore, we will clear the party 
		// id used for auto party
		if (s_PartyIsInGame( pGame ) == FALSE)
		{
			pGame->m_idPartyAuto = INVALID_ID;
		}
		
	}		

#endif //!ISVERSION(CLIENT_ONLY_VERSION)		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sDoPlayerPartyValidation(
	GAME* game,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	s_PlayerPartyValidation( unit, 0 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerPartyValidation(
	UNIT *pPlayer,
	DWORD dwDelayInTicks)
{
	ASSERTV_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ));

	// in single player we don't care
	if (AppIsSinglePlayer())
	{
		return;
	}

	// unregister existing one if they have one
	UnitUnregisterTimedEvent( pPlayer, sDoPlayerPartyValidation );

	// do the validation now or in a little while
	DWORD dwNextEventDelayInTicks = dwDelayInTicks;
	if (dwDelayInTicks == 0)
	{
					
		// for a short time, we will validate that this player is in a party if there is a party
		// in the game at all.  This is a failsafe of somehow allowing players into game instances
		// and not putting them in your party (party full, some cheat was found, etc)
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		if (pLevel && LevelAllowsUnpartiedPlayers( pLevel ) == FALSE)
		{
			GAME *pGame = UnitGetGame( pPlayer );
			
			if (s_PartyIsInGame( pGame ) && UnitGetPartyId( pPlayer ) == INVALID_ID)
			{

				// run the function like they were kicked out of the party
				s_PlayerLeftParty( pPlayer, INVALID_ID, PARTY_MEMBER_KICKED_OUT );
						
			}
					
		}

		// automatically setup the delay for the next validation event
		dwNextEventDelayInTicks = PARTY_VALIDATION_DELAY_IN_TICKS;
		
	}
	
	// register another event, we only do this when you're in an action level, and 
	// every time you switch into a level, we call this function, that way in towns
	// there is never any of the validation going on wasting event cycle time
	UnitRegisterEventTimed( pPlayer, sDoPlayerPartyValidation, NULL, dwNextEventDelayInTicks );
		
}

//----------------------------------------------------------------------------
// Limits to inventory type to limit
// If nLimit = -1, just returns the number of that unittype
// Probably should have a better name to reflect this behaviour, but can't think of one - bmanegold
//----------------------------------------------------------------------------
int s_PlayerLimitInventoryType(
	UNIT* pPlayer,
	int nType,
	int nLimit /* = -1*/)

{
	ASSERT(pPlayer);
	int nItem = 0;
	UNIT *pItem, *pNext;
	for(pItem = UnitInventoryIterate(pPlayer, NULL, 0); pItem; pItem = pNext)
	{
		pNext = UnitInventoryIterate(pPlayer, pItem, 0);
		if(UnitIsA(pItem, nType))
		{
			if(nItem >= nLimit && nLimit >= 0)
			{
				//delete it!
				UnitFree(pItem, UFF_SEND_TO_CLIENTS);
			}
			else
			{
				nItem++;
			}
		}
	}
	return nItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerCleanUpGuildHeralds(
	GAME * game,
	UNIT * player,
	BOOL bInGuild )
{
	int n = 50;
	UNIT * pGuildHerald = FindInUseGuildHerald(player);
	while (pGuildHerald && (n-- > 0))
	{
		if (bInGuild)
		{
			sItemDecrementOrUse(game, pGuildHerald, FALSE);
		}
		else
		{
			UnitClearStat(pGuildHerald, STATS_GUILD_HERALD_IN_USE, 0);
		}

		pGuildHerald = FindInUseGuildHerald(player);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerCSRChatEnter(
	void)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETURN(pGlobals);
	pGlobals->bInCSRChat = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PlayerCSRChatExit(
	void)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETURN(pGlobals);
	pGlobals->bInCSRChat = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerInCSRChat(
	void)
{
	CLIENT_PLAYER_GLOBALS *pGlobals = sClientPlayerGlobalsGet();
	ASSERT_RETFALSE(pGlobals);
	return pGlobals->bInCSRChat;
}

void PlayerLearnRecipe( UNIT *pPlayer,
					    int nRecipeID )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( nRecipeID != INVALID_ID );
	UnitSetStat( pPlayer, STATS_RECIPES_LEARNED, nRecipeID, 1 );
}

BOOL PlayerHasRecipe( UNIT *pPlayer,
					  int nRecipeID )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( nRecipeID != INVALID_ID );
	return ( UnitGetStat( pPlayer, STATS_RECIPES_LEARNED, nRecipeID ) == 1 )?TRUE:FALSE;
}

void PlayerRemoveRecipe( UNIT *pPlayer,
						 int nRecipeID )
{
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( nRecipeID != INVALID_ID );
	if( IS_SERVER( pPlayer ) )
	{
		UnitSetStat( pPlayer, STATS_RECIPES_LEARNED, nRecipeID, 0 );
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerCanBeInspected(
					  UNIT *pPlayer,
					  UNIT *pOtherPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( pOtherPlayer, "Expected unit" );

	if (UnitGetStat(pOtherPlayer, STATS_CHAR_OPTION_ALLOW_INSPECTION) == 0)
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PlayerCanWarpTo(
	UNIT * pPlayer,
	int nLevelDefDest)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(nLevelDefDest != INVALID_LINK);

	const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(nLevelDefDest);
	if (!pLevelDef || !pLevelDef->bTown)
	{
		return FALSE;
	}

	if (!PlayerHasWaypointTo(pPlayer, nLevelDefDest))
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_PlayerListAvailableTitles(
	UNIT *pPlayer,
	int *pnTitleArray,
	int nTitleArraySize)
{
	ASSERT_RETZERO(pPlayer);

	int nNumTitles = 0;
	GAME *pGame = UnitGetGame(pPlayer);

	int nFactionTitle = FactionGetTitle(pPlayer);
	if (nFactionTitle != INVALID_LINK && nNumTitles < nTitleArraySize)
	{
		pnTitleArray[nNumTitles++] = nFactionTitle;
	}

	int nAchievementCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);

	for (int iAch = 0; iAch	< nAchievementCount && nNumTitles < nTitleArraySize; iAch++)
	{
		const ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAch);
		ASSERT_RETZERO(pAchievement);

		if (pAchievement->nRewardTitleString != INVALID_LINK)
		{
			if (x_AchievementIsComplete(pPlayer, pAchievement))
			{
				pnTitleArray[nNumTitles++] = pAchievement->nRewardTitleString;
			}
		}
	}

	return nNumTitles;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerTitleIsAvailable(
	UNIT *pPlayer,
	int nTitleID)
{
	ASSERT_RETZERO(pPlayer);

	GAME *pGame = UnitGetGame(pPlayer);

	int nFactionTitle = FactionGetTitle(pPlayer);
	if (nTitleID == nFactionTitle)
		return TRUE;

	int nAchievementCount = ExcelGetCount(pGame, DATATABLE_ACHIEVEMENTS);
	for (int iAch = 0; iAch	< nAchievementCount; iAch++)
	{
		const ACHIEVEMENT_DATA *pAchievement = (ACHIEVEMENT_DATA *) ExcelGetData(pGame, DATATABLE_ACHIEVEMENTS, iAch);
		ASSERT_RETZERO(pAchievement);

		if (pAchievement->nRewardTitleString == nTitleID)
		{
			return x_AchievementIsComplete(pPlayer, pAchievement);
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PlayerSetTitle(
	UNIT *pPlayer,
	int nStringID)
{
	ASSERT_RETURN(pPlayer);
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETURN(IS_SERVER(pGame));
	ASSERT_RETURN(UnitIsPlayer(pPlayer));
	
	if (s_PlayerTitleIsAvailable(pPlayer, nStringID))
	{
		UnitSetStat(pPlayer, STATS_PLAYER_TITLE_STRING, nStringID);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int x_PlayerGetTitleStringID(
	UNIT *pPlayer)
{
	ASSERT_RETINVALID(pPlayer);
	
	return UnitGetStat(pPlayer, STATS_PLAYER_TITLE_STRING);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIQUE_ACCOUNT_ID PlayerGetAccountID(
	UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );

	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETINVALID( pClient, "Expected client" );
	
	NETCLIENTID64 idNetClient = ClientGetNetId( AppGetClientSystem(), pClient->m_idAppClient );
	ASSERT_RETFALSE(idNetClient != INVALID_NETCLIENTID64);
	return NetClientGetUniqueAccountId( idNetClient );
	
}

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_PlayerSetTitle(
	UNIT *pPlayer,
	int nStringID)
{
	ASSERT_RETURN(pPlayer);
	GAME *pGame = UnitGetGame(pPlayer);
	ASSERT_RETURN(IS_CLIENT(pGame));	
	ASSERT_RETURN(pPlayer == GameGetControlUnit(pGame));	

	MSG_CCMD_SELECT_PLAYER_TITLE msg;

	msg.nTitleString = nStringID;
	c_SendMessage(CCMD_SELECT_PLAYER_TITLE, &msg);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPlayerFindGuildMemberLevelId(
	PGUID guid)
{
	if (guid == INVALID_GUID)
	{
		return INVALID_ID;
	}

	const PLAYER_GUILD_MEMBER_DATA *pGuildMember = c_PlayerGuildMemberListGetNextMember(NULL, TRUE);
	while (pGuildMember)
	{
		if (pGuildMember->idMemberCharacterId == guid)
		{
			return pGuildMember->tMemberClientData.nLevelDefId;
		}
		pGuildMember = c_PlayerGuildMemberListGetNextMember(pGuildMember, TRUE);
	}

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPlayerFindBuddyOrGuildMemberLevelId(
	PGUID guid)
{
	int nLevel = c_BuddyGetLevelId(guid);
	if (nLevel != INVALID_ID)
	{
		return nLevel;
	}

	return sPlayerFindGuildMemberLevelId(guid);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_PlayerCanWarpTo(
	PGUID guid)
{
	if (guid == INVALID_GUID)
	{
		return FALSE;
	}

	int nLevelDefDest;

	UNIT * pUnit = UnitGetByGuid(AppGetCltGame(), guid);
	if (pUnit)
	{
		ROOM* pRoom = UnitGetRoom(pUnit);
		if (! pRoom )
			return FALSE;

		LEVEL* pLevel = RoomGetLevel(pRoom);
		if (! pLevel )
			return FALSE;

		nLevelDefDest = pLevel->m_nLevelDef;
	}
	else
	{
		nLevelDefDest = sPlayerFindBuddyOrGuildMemberLevelId(guid);
	}

	return PlayerCanWarpTo(UIGetControlUnit(), nLevelDefDest);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_GetPlayerUnitDataRowByUnitTypeAndGender(
	GENDER eGender,
	int nFactionUnitType )
{
	int nNumClasses = ExcelGetCount( NULL, DATATABLE_PLAYERS);
	int nClass;
	for ( nClass = 0; nClass < nNumClasses; ++nClass )
	{
		UNIT_DATA * pUnitData = (UNIT_DATA *) ExcelGetData( NULL, DATATABLE_PLAYERS, nClass );
		if ( ! pUnitData )
			continue;
		if ( eGender != pUnitData->eGender )
			continue;
		if ( ! UnitIsA( pUnitData->nUnitType, nFactionUnitType ) )
			continue;

		return nClass;
	}

	return NULL;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL s_MakeNewPlayer(
	GAME * game,
	GAMECLIENT * client,
	int playerClass,
	const WCHAR * strName)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERT_RETFALSE(client);

	UNIT * oldunit = ClientGetControlUnit(client);
	ASSERT_RETFALSE(oldunit);
	
	const UNIT_DATA * playerData = PlayerGetData(game, playerClass);
	ASSERT_RETFALSE(playerData);

	int appearanceGroup = playerData->pnWardrobeAppearanceGroup[UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON];
	WARDROBE_INIT wardrobeInit(appearanceGroup);
	WardrobeRandomizeInitStruct( game, wardrobeInit, appearanceGroup, TRUE);

	APPEARANCE_SHAPE appearanceShape;
	AppearanceShapeRandomize(playerData->tAppearanceShapeCreate, appearanceShape);

	GAMECLIENTID idClient = ClientGetId(client);
	UNIT * newunit = s_PlayerNew(game, idClient, strName, playerClass, &wardrobeInit, &appearanceShape, 0);
	ASSERT_RETFALSE(newunit);

	UnitSetFlag( newunit, UNITFLAG_ALLOW_PLAYER_ROOM_CHANGE );

	// put player at same location
	UnitWarp(newunit, UnitGetRoom(oldunit), UnitGetPosition(oldunit), UnitGetFaceDirection(oldunit, FALSE), cgvZAxis, 0, TRUE);
		
	if (!s_PlayerAdd(game, client, newunit))
	{
		UnitFree(newunit, 0);
		ClientSetControlUnit(game, client, oldunit);
		return FALSE;
	}

	// Now handled outside
	//s_sSaveDefaultName(strName, arrsize(strName));

	// setup the players quests on this server
#if HELLGATE_ONLY
	if(AppIsHellgate())
	{

		// restore state of quest system	
		s_QuestDoSetup( newunit, FALSE );

		// init quest games
		s_QuestGamesInit( newunit );

		s_QuestsUpdateAvailability( newunit );

		s_TutorialTipsOff( newunit );

	}
#endif
	UnitFree(oldunit, UFF_SEND_TO_CLIENTS);
#endif // CLIENT_ONLY_VERSION
	return TRUE;
}

#else // SERVER_VERSION begins here

BOOL s_MakeNewPlayer(GAME *, GAMECLIENT *, int, const WCHAR *)				{ return FALSE; }

#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
MYTHOS_POINTSOFINTEREST::cPointsOfInterest * PlayerGetPointsOfInterest( UNIT *pPlayer )
{
	ASSERT_RETNULL( pPlayer );
	ASSERT_RETNULL( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	return pPlayer->m_pPointsOfInterest;
}
