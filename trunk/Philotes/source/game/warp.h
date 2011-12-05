//----------------------------------------------------------------------------
// warp.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_WARP_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _WARP_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _C_MESSAGE_H_
#include "c_message.h"
#endif


//----------------------------------------------------------------------------
// Forward references
//----------------------------------------------------------------------------
enum OPERATE_RESULT;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
struct WARP_DESTINATION
{
	int nLevelDefinition;
	ROOM *pRoom;
	VECTOR vPositionWorld;
	VECTOR vFacing;
};

//----------------------------------------------------------------------------
enum WARP_FLAGS
{
	WF_ARRIVE_AT_WARP_RETURN_BIT,		// at dest, place player at return warp
	WF_ARRIVE_AT_HEADSTONE_BIT,			// at dest, place player at their most recent headstone
	WF_ARRIVE_AT_GRAVEKEEPER_BIT,		// at dest, place player a gravekeeper (somebody who can send them back to their headstone)
	WF_ARRIVE_AT_START_LOCATION_BIT,	// arrive at start "home" location in level
	WF_ARRIVE_FROM_LEVEL_PORTAL,		// arrive at correct portal for level they just came from
	WF_RESTARTING_BIT,					// player has died and they are restarting - tugboat
	WF_ARRIVE_AT_GUID_BIT,				// arrive at the unit with the specified guid (warping to party members for instance)
	WF_ARRIVE_AT_ARENA_BIT,				// start at special arena marker (for quest minigame)
	WF_ARRIVE_VIA_PARTY_PORTAL_BIT,		// when arriving at dest party member GUID, create a portal at the arrival location
	WF_ARRIVE_AT_TRANSPORTER_BIT,		// arrive at the transporter unit in the new instance
	WF_BEING_BOOTED_BIT,				// this unit is being booted out of the instance
	WF_ARRIVE_AT_RANDOM_SPAWN_BIT,		// start at a random spawn location
	WF_ARRIVE_AT_FARTHEST_PVP_SPAWN_BIT,// start at the farthest spawn location marker from the other player in the level, or if no other players, choose one at random
	WF_ARRIVE_AT_OBJECT_BY_CODE,		// we arrive at an object that matches the class ID
	WF_USED_RECALL,						// for recalling in the world
};

//----------------------------------------------------------------------------
enum WARP_RESOLVE_PASS
{
	WRP_INVALID = -1,
	
	WRP_SUBLEVEL_POPULATE,				// resolve this warp link on sublevel populate
	WRP_LEVEL_FIRST_ACTIVATE,			// resolve this warp link on first level activation
	WRP_ON_USE,							// resolve this warp link when it is used by
	
	WRP_NUM_PASSES
};

//----------------------------------------------------------------------------
enum SWITCH_INSTANCE_FLAGS
{
	SIF_JOIN_OPEN_LEVEL_GAME_BIT,			// if possible, join game with an appropriate open level
	SIF_DISALLOW_DIFFERENT_GAMESERVER		// don't get idGames that do not reside on this server.  Currently only implemented for getting the partygame.
};

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------

GAME_SUBTYPE GetGameSubTypeByLevel(
	unsigned int level);

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL WarpToInstance(
	UNIT *pPlayer, 
	const WARP_SPEC &tSpec);
#endif

#if ISVERSION(SERVER_VERSION)
BOOL WarpGameIdRequiresServerSwitch(
	GAMEID idWarpGame);

BOOL ClientWarpPostServerSwitch(
	NETCLIENTID64 idNetClient,
	WARP_SPEC *pWarpSpec,
	GAMEID idGameToSwitchTo,
	PGUID guidPlayer,
	WCHAR *szPlayerName);
#endif

BOOL s_WarpToLevelOrInstance(
	UNIT *pPlayer,
	WARP_SPEC &tSpec);

GAMEID WarpGetGameIdToJoin(
	APPCLIENTID idAppClient,
	BOOL bInitialJoin,				// true if it's on initial join, false if switching levels/instances	
	int nLevelDef,
	int nLevelArea,					// Tugboat
	DWORD dwSeed,					// Tugboat
	DWORD *pdwSwitchInstanceFlags,
	int nDifficulty,
	const GAME_VARIANT &tGameVariant);

GAMEID WarpGetGameIdToJoinFromSaveData(
	APPCLIENTID idAppClient,
	BOOL bInitialJoin,				// true if it's on initial join, false if switching levels/instances
	CLIENT_SAVE_FILE *pClientSaveFile,
	int nDifficulty);

GAMEID WarpGetGameIdToJoinFromSinglePlayerSaveFile(
	APPCLIENTID idAppClient,
	BOOL bInitialJoin,				// true if it's on initial join, false if switching levels/instances
	const WCHAR * szPlayerName,
	int nDifficulty,
	DWORD dwNewPlayerFlags);		// see NEW_PLAYER_FLAGS
	
int WarpGetDestinationLevelID(
	UNIT *pWarp);
	
LEVELID s_WarpInitLink(
	UNIT *pWarp,
	UNITID idActivator,
	WARP_RESOLVE_PASS eWarpResolvePass);

BOOL PlayerHasWaypointTo(
	UNIT *pPlayer,
	int nLevelDefDest);

BOOL s_WarpValidatePartyMemberWarpTo(
	UNIT *pPlayer,
	PGUID guidPartyMemberDest,
	BOOL bInformClientOnRestricted);

BOOL s_WarpPartyMemberEnter(
	UNIT *pPlayer,
	UNIT *pWarp);
	
void s_WarpPartyMemberOpen(
	UNIT *pUnit,
	PGUID guidPartyMember);

void s_WarpPartyMemberCloseLeadingTo(
	GAME *pGame,
	PGUID guidDest);
	
UNIT *s_WarpPartyMemberExitCreate( 
	UNIT *pUnit, 
	UNIT *pDest);

BOOL s_WarpPartyMemberGetExitPortalLocation(
	UNIT *pOwner,
	ROOM **pRoom,
	VECTOR *pvPosition,
	VECTOR *pvDirection,
	VECTOR *pvUp);
		
BOOL s_WarpToPartyMember( 
	UNIT *pUnit, 
	PGUID guidPartyMember);

//----------------------------------------------------------------------------
enum WARP_TYPE
{
	WARP_TYPE_INVALID = -1,
	
	WARP_TYPE_RECALL,			// one way portal to town
	WARP_TYPE_PARTY_MEMBER,		// one way portal to party member
	
	WARP_TYPE_NUM_TYPES
};

UNIT *s_WarpOpen(
	UNIT *pPlayer,
	WARP_TYPE eWarpType);

void s_WarpCloseAll(
	UNIT *pPlayer,
	WARP_TYPE eWarpType);
	
void s_WarpEnterWarp(
	UNIT *pPlayer,
	UNIT *pWarp);

//----------------------------------------------------------------------------
enum WARP_DIRECTION
{
	WD_FRONT,		// position is in front of warp
	WD_BEHIND,		// position is behind warp
};

//----------------------------------------------------------------------------
struct WARP_ARRIVE_POSITION
{
	VECTOR vPosition;
	VECTOR vDirection;
	int nRotation;
	ROOM *pRoom;
	
	WARP_ARRIVE_POSITION::WARP_ARRIVE_POSITION( void )
		:	vPosition( cgvNone ),
			vDirection( cgvNone ),
			nRotation( 0 ),
			pRoom( NULL )
	{ }
	
};

void WarpGetArrivePosition(	
	LEVEL *pLevel,
	const VECTOR &vWarpPosition,
	const VECTOR &vWarpDirection,
	float flWarpOutDistance,
	WARP_DIRECTION eDirection,
	BOOL bFaceAfterWarp,
	WARP_ARRIVE_POSITION *pArrivePosition);

ROOM *WarpGetArrivePosition( 
	UNIT *pWarp,
	VECTOR *pvPosition, 
	VECTOR *pvDirection,
	WARP_DIRECTION eDirection,
	int * pnRotation = NULL);

LEVEL_TYPE WarpSpecGetLevelType(
	const WARP_SPEC &tSpec);

OPERATE_RESULT WarpCanOperate( 
	UNIT *pPlayer, 
	UNIT *pWarp);

//----------------------------------------------------------------------------
enum WARP_CREATE_FLAGS
{
	WCF_CLEAR_FROM_WARPS,				// do not create warp if it's near a level warp
	WCF_CLEAR_FROM_START_LOCATIONS,		// do not create warp if it's near a start location
	WCF_EXACT_DESTINATION_UNIT_LOCATION,// don't do fancy searching, just create the warp at the destination unit
};

UNIT *s_WarpCreatePortalAround(
	UNIT *pPlayer,
	GLOBAL_INDEX eGIWarp,
	DWORD dwWarpCreateFlags,		// see WARP_CREATE_FLAGS
	UNIT *pCreator = NULL,
	int nStatRecordPortalIDOnCreator = INVALID_LINK);

//----------------------------------------------------------------------------
enum WARP_RESTRICTED_REASON
{
	WRR_INVALID = -1,
	
	WRR_OK,							// no restriction
	
	WRR_UNKNOWN,					// unknown reason
	WRR_NO_WAYPOINT,				// player doesn't have the waypoint for the level they want to warp to (or the most recent waypoint area before the destination level)
	WRR_PROHIBITED_IN_PVP,			// player is involved in PvP, and party portals are prohibited
	WRR_TARGET_PLAYER_NOT_FOUND,	// target player was not found
	WRR_TARGET_PLAYER_NOT_ALLOWED,	// target player is not in a valid destination
	
	WRR_LEVEL_NOT_AVAILABLE_TO_GAME,	// level is not available in this game settings (beta period for instance)
	WRR_ACT_BETA_GRACE_PERIOD,			// act cannot be played by beta accounts and player is a beta grace player
	WRR_ACT_SUBSCRIBER_ONLY,			// act that is for subscribers only
	WRR_ACT_EXPERIENCE_TOO_LOW,			// act that has a minimum experience level for a player to enter that they do not meet
	WRR_ACT_TRIAL_ACCOUNT,				// act is restricted for trial account players
	
	WRR_NUM_REASONS					// keep this last please
};

WARP_RESTRICTED_REASON WarpDestinationIsAvailable(
	UNIT *pOperator,
	UNIT *pWarp);

void c_WarpSetBlockingReason( 
	UNIT *pWarp, 
	WARP_RESTRICTED_REASON eReason);

WARP_RESTRICTED_REASON c_WarpGetBlockingReason( 
	UNIT *pWarp);

void c_WarpRestricted( 
	WARP_RESTRICTED_REASON eReason);

#endif // _WARP_H_
