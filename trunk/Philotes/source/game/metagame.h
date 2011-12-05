//----------------------------------------------------------------------------
// FILE: metagame.h
//
//	Funcitonality shared by quests and PvP games.
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __METAGAME_H_
#define __METAGAME_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct GAME_SETUP;
struct LEVEL;
struct ROOM;
struct VECTOR;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum QUEST_MESSAGE_TYPE
{
	QUEST_MESSAGE_INVALID = -1,

	// *SERVER* quest messages	
	QM_SERVER_INTERACT,				// a player is talking to an npc
	QM_SERVER_INTERACT_GENERIC,			// a player started (any may have immediately finished) interacting with another unit
	QM_SERVER_TALK_STOPPED,			// a player finished reading/hearing dialog from an npc
	QM_SERVER_TALK_CANCELLED,		// a player cancelled reading/hearing dialog from an npc
	QM_SERVER_TIP_STOPPED,			// a player finished a tutorial tip
	QM_SERVER_INTERACT_INFO,		// what kind (if any) interact info does this unit has for a player
	QM_SERVER_MONSTER_KILL,			// a monster has died
	QM_SERVER_MONSTER_DYING,		// a monster has BEGUN to die
	QM_SERVER_CREATE_LEVEL,			// the game populated a level with rooms and such
	QM_SERVER_ENTER_LEVEL,			// a player has entered into a level
	QM_SERVER_ABOUT_TO_LEAVE_LEVEL,	// a player is going to leave a level
	QM_SERVER_LEAVE_LEVEL,			// a player has left a level
	QM_SERVER_SUB_LEVEL_TRANSITION,	// a player has entered/left a sub level
	QM_SERVER_ENTER_ROOM,			// a player has entered into a room
	QM_SERVER_ROOMS_ACTIVATED,		// a player has activated one more rooms since the last quest update  (only sent for rooms in the default sublevel for now, could easily be changed in s_QuestEventRoomActivated)
	QM_SERVER_EXIT_LIMBO,			// a player has exited limbo
	QM_SERVER_OBJECT_OPERATED,		// object has been operated
	QM_SERVER_LOOT,					// unit spawned loot, here it is
	QM_SERVER_PICKUP,				// item was picked up
	QM_SERVER_ATTEMPTING_PICKUP,	// trying to pickup an item, this happens after the item is pulled to the player
	QM_SERVER_DROPPED_ITEM,			// player dropped, or destroyed, an item
	QM_SERVER_PLAYER_APPROACH,		// player has approached
	QM_SERVER_QUEST_STATUS,			// quest status has changed
	QM_SERVER_QUEST_STATE_CHANGE_ON_CLIENT,		// the *client* has changed a state in a local quest
	QM_SERVER_ABORT,				// client has aborted the current quest cinematic/story mode
	QM_SERVER_CAN_RUN_DRLG_RULE,	// can the level creation place this rule?
	QM_SERVER_USE_ITEM,				// use this quest item
	QM_SERVER_KILL_DROP_GIVER_ITEM,	// player killed a monster, check if giver item should drop
	QM_SERVER_PARTY_KILL,			// player, or a party member, killed a monster
	QM_SERVER_UNIT_STATE_CHANGE,	// a "state" on a unit (ie STATE_QUEST_RTS) has been set or cleared
	QM_SERVER_QUEST_ACTIVATED,		// quest status has changed
	QM_SERVER_GAME_MESSAGE,			// quest game messages
	QM_SERVER_PLAYER_DIE,			// the player has died. only sent to that player's quest system for now...
	QM_SERVER_PLAYER_RESPAWN,		// the player has respawned. only sent to that player's quest system for now...
	QM_SERVER_RELOAD_UI,			// the client has completely reset the UI for whatever reason.  If the quest needs to have a UI element in a particular state it needs to set it in response to this
	QM_SERVER_ITEM_CRAFTED,			// item was crafted
	QM_SERVER_AI_UPDATE,			// an AI is asking a quest for an update
	QM_SERVER_TOGGLE_PORTAL,		// a portal has been forced open or closed
	QM_SERVER_BACK_FROM_MOVIE,		// a movie has finished on the client, release the hounds!!
	QM_SERVER_ABANDON,				// this quest was abandoned
	QM_SERVER_SKILL_NOTIFY,			// a notification is being sent by a skill
	QM_SERVER_RESPAWN_WARP_REQUEST,	// this event requests a metagame to warp the player to a respawn point
	QM_SERVER_WARP_TO_START_LOCATION,	// the start location for the level is requested
	QM_SERVER_LEVEL_GET_START_LOCATION,	// the start location for the level is requested
	QM_SERVER_FELL_OUT_OF_WORLD,	// player is forced to restart the level by being warped to start, they made some kind of fatal movement error (currently this just mea
	QM_SERVER_REMOVED_FROM_TEAM,	// player is forced to restart the level by being warped to start, they made some kind of fatal movement error (currently this just mea

	// *CLIENT* quest messages
	QM_CLIENT_OPEN_UI,				// player has opened a ui element
	QM_CLIENT_SKILL_POINT,			// player has received skill point
	QM_CLIENT_QUEST_STATE_CHANGED,	// state has changed
	QM_CLIENT_QUEST_STATUS,			// quest status has changed
	QM_CLIENT_MONSTER_INIT,			// monster being initialized on client
	QM_CLIENT_ATTEMPTING_PICKUP,	// trying to pickup an item, this happens right when the player presses the 'f' button

	// either client or server quest messages
	QM_CAN_OPERATE_OBJECT,			// can a player operate a thing (like a chest, door etc)
	QM_CAN_USE_ITEM,				// can the player use this quest item?
	QM_IS_UNIT_VISIBLE,				// ask quest system if player can see this unit
		
	NUM_QUEST_MESSAGES
};

//----------------------------------------------------------------------------
enum QUEST_MESSAGE_RESULT
{
	QMR_INVALID = -1,			// invalid quest message result
	QMR_OK,						// ok
	QMR_FINISHED,				// this message has been taken care of, don't continue asking other quests
	QMR_NPC_TALK,				// message resulted in talking with an mpc	
	QMR_NPC_INFO_UNAVAILABLE,	// npc has new info, but the player does not meet some requirement (can't accept quest)
	QMR_NPC_INFO_NEW,			// npc has new info to give (like giving a quest)
	QMR_NPC_INFO_WAITING,		// npc is waiting for the player to do something
	QMR_NPC_INFO_RETURN,		// npc is waiting for player to return (they have done the "thing" they were waiting for)	
	QMR_NPC_INFO_MORE,			// npc has told info, but has more optional things to say
	QMR_NPC_STORY_NEW,			// npc has a new storyline quest
	QMR_NPC_STORY_RETURN,		// npc has more info or is finishing a storyline quest
	QMR_NPC_STORY_GOSSIP,		// npc has gossip about a storyline quest
	QMR_NPC_OPERATED,			// clicking on the npc did something in the quests. all finished, so move on
	QMR_OPERATE_OK,				// can operate object
	QMR_OPERATE_FORBIDDEN,		// cannot operate object
	QMR_DRLG_RULE_FORBIDDEN,	// can not run drlg rule
	QMR_USE_OK,					// can use/operate this item
	QMR_USE_FAIL,				// use of this quest item failed for some reason
	QMR_OFFERING,				// player has been given an offering
	QMR_KNOWN_RESTRICTED,		// client cannot know about this unit
	QMR_UNIT_VISIBLE,			// unit is visible to quest player
	QMR_UNIT_HIDDEN,			// unit is hidden to quest player
	QMR_PICKUP_FAIL,			// pickup of this quest item failed for some reason
};

//----------------------------------------------------------------------------
#define	QSMF_ALL				0	// default

enum QUEST_SEND_MESSAGE_FLAGS
{
	QSMF_STORY_ONLY,				// send this message only to story quests
	QSMF_FACTION_ONLY,				// send this message only to faction quests
	QSMF_APPROACH_RADIUS,			// send this message only to quests that care about monitoring approach radius
	QSMF_BACK_FROM_MOVIE,			// send this message only to quests that care about coming back from movies
	QSMF_IS_UNIT_VISIBLE,			// send this message only to quests that care about unit visibility
};

//----------------------------------------------------------------------------
typedef QUEST_MESSAGE_RESULT (* PFN_METAGAME_MESSAGE_HANDLER)( UNIT *pPlayer, QUEST_MESSAGE_TYPE eMessageType, const void *pMessage );

//----------------------------------------------------------------------------
struct METAGAME
{
	PFN_METAGAME_MESSAGE_HANDLER pfnMessageHandler;	// message handler	
	void * pData;
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void MetagameInit(GAME *pGame, const GAME_SETUP * pGameSetup);
void MetagameFree(GAME *pGame);

QUEST_MESSAGE_RESULT MetagameSendMessage(
	UNIT * pPlayer,
	QUEST_MESSAGE_TYPE eMessageType,
	void *pMessage,
	DWORD dwFlags = QSMF_ALL );

METAGAME * GameGetMetagame(GAME *pGame);

void s_MetagameEventRespawnWarpRequest(
	UNIT* pPlayer);

void s_MetagameEventExitLimbo(
	UNIT* pPlayer);

ROOM * s_MetagameLevelGetStartPosition(
	UNIT* pPlayer,
	LEVELID idLevel,
	ROOM** pRoom, 
	VECTOR * pPosition, 
	VECTOR * pFaceDirection);

void s_MetagameEventWatpToStartLocation(
	UNIT* pPlayer);

void s_MetagameEventFellOutOfWorld(
	UNIT* pPlayer);

void s_MetagameEventRemoveFromTeam(
	UNIT* pPlayer,
	int nTeam);

void s_MetagameEventPlayerRespawn(
	UNIT * pPlayer);

#endif
