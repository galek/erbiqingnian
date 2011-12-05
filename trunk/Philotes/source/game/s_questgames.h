//----------------------------------------------------------------------------
// FILE: s_questgames.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __SQUESTGAMES_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __SQUESTGAMES_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct QUEST;

//----------------------------------------------------------------------------
// Defines & Enums
//----------------------------------------------------------------------------

#define QUEST_OPERATE_OBJECT_DISTANCE			1.0f
#define QUEST_OPERATE_OBJECT_DISTANCE_SQUARED	( QUEST_OPERATE_OBJECT_DISTANCE * QUEST_OPERATE_OBJECT_DISTANCE )

//----------------------------------------------------------------------------

enum QUEST_GAME_MESSAGE_TYPE
{
	QUESTGAME_MESSAGE_INVALID = -1,

	// from quest game system
	QGM_GAME_NEW,
	QGM_GAME_COPY_CURRENT_STATE,
	QGM_GAME_JOIN,
	QGM_GAME_START,
	QGM_GAME_LEAVE,
	QGM_GAME_DESTROY,
	QGM_GAME_VICTORY,
	QGM_GAME_FAILED,
	QGM_GAME_AIUPDATE,
	QGM_GAME_ARENA_RESPAWN,

	//--------------------
	// custom messages
	//--------------------
	QGM_CUSTOM_LIST_BEGIN,

	// triage
	QGM_CUSTOM_TRIAGE_NEW_INJURED,
	QGM_CUSTOM_TRIAGE_INJURED_DEAD,
	QGM_CUSTOM_TRIAGE_INJURED_SAVED,
	QGM_CUSTOM_TRIAGE_NEW_HELLBALL,
	QGM_CUSTOM_TRIAGE_HELLBALL_KILL,

	// test of knowledge
	QGM_CUSTOM_TOK_CLEAR_BOOK,
	QGM_CUSTOM_TOK_SET_BOOK,

	// test of leadership
	QGM_CUSTOM_TOL_ADD_PROTECTOR,
	QGM_CUSTOM_TOL_REMOVE_PROTECTOR,
	QGM_CUSTOM_TOL_CHANGE_NODE,
	QGM_CUSTOM_TOL_PROTECTOR_FOLLOW,
	QGM_CUSTOM_TOL_PROTECTOR_DEFEND,

	// test of beauty
	QGM_CUSTOM_TOB_ADD_BOIL,
	QGM_CUSTOM_TOB_REMOVE_BOIL,
	QGM_CUSTOM_TOB_CHANGE_KNOT,

	// test of fellowship
	QGM_CUSTOM_TOF_CHANGE_NODE,
	QGM_CUSTOM_TOF_PLAYER_SCORE,
	QGM_CUSTOM_TOF_DEMON_SCORE,

	// test...
};

//----------------------------------------------------------------------------
// structures
//----------------------------------------------------------------------------

struct QUEST_GAME_LABEL_NODE_INFO
{
	ROOM *	pRoom;
	VECTOR	vPosition;
	VECTOR	vDirection;
};

//----------------------------------------------------------------------------

#define QUEST_GAME_MAX_NEARBY_UNITS			16

struct QUEST_GAME_NEARBY_INFO
{
	UNIT *	pNearbyPlayers[ QUEST_GAME_MAX_NEARBY_UNITS ];
	int		nNumNearbyPlayers;
	UNIT *	pNearbyObjects[ QUEST_GAME_MAX_NEARBY_UNITS ];
	int		nNumNearbyObjects;
	UNIT *	pNearbyEnemies[ QUEST_GAME_MAX_NEARBY_UNITS ];
	int		nNumNearbyEnemies;
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void s_QuestGamesInit(
	UNIT * pPlayer );

void s_QuestGameDataInit(
	QUEST * pQuest );

void s_QuestGameDataFree(
	QUEST * pQuest );

void s_QuestGameJoin(
	QUEST * pQuest );

void s_QuestGameLeave(
	QUEST * pQuest );

void s_QuestGameVictory(
	QUEST * pQuest );

void s_QuestGameFailed(
	QUEST * pQuest );

void s_QuestGameCustomMessage(
	QUEST * pQuest,
	QUEST_GAME_MESSAGE_TYPE eMessageType,
	UNIT * pUnit = NULL,
	DWORD dwData = 0 );

BOOL QuestGamePlayerInGame(
	UNIT * pPlayer );

void s_QuestGameUIMessage(
	QUEST * pQuest,
	QUEST_UI_MESSAGE_TYPE eUIMessageType,
	int nDialog,
	int nParam1 = 0,
	int nParam2 = 0,
	DWORD dwFlags = 0 );

BOOL s_QuestGameInOperateDistance(
	UNIT * pOperator,
	UNIT * pObject );

BOOL c_QuestGamePlayerArenaRespawn(
	UNIT * pPlayer );

BOOL s_QuestGamePlayerArenaRespawn(
	UNIT * pPlayer );

void s_QuestGameWarpPlayerIntoArena(
	QUEST * pQuest,
	UNIT * player );

void s_QuestGameRespawnDeadPlayer(
	UNIT * pPlayer );

void s_QuestGameWarpUnitToNode(
	UNIT * unit,
	char * label );

BOOL s_QuestGameRespawnMonster(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA& tEventData );

BOOL s_QuestGameFindLayoutLabel(
	LEVEL * pLevel,
	char * pszNodeName,
	QUEST_GAME_LABEL_NODE_INFO * pInfo );

UNITID s_QuestGameSpawnEnemy(
	QUEST * pQuest,
	char * pszLabelName,
	int nClass,
	int nLevel);

void s_QuestGameNearbyInfo(
	UNIT * monster,
	QUEST_GAME_NEARBY_INFO * pNearbyInfo,
	BOOL bIncludeEnemies = FALSE );

UNIT * s_QuestGameGetFirstPlayer(
	GAME * pGame,
	int nQuestId );

#endif
