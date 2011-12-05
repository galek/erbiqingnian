//----------------------------------------------------------------------------
// FILE: c_quests.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __CQUESTS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __CQUESTS_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward References
//----------------------------------------------------------------------------
enum QUEST_STATE_VALUE;
enum QUEST_STATUS;
enum UI_ELEMENT;
enum QUEST_STYLE;
struct ROOM_LAYOUT_GROUP;
struct QUEST;
struct ROOM;
struct UNIT;
struct GAME;
struct QUEST_STATE;
struct MSG_SCMD_QUEST_DISPLAY_DIALOG;
struct MSG_SCMD_QUEST_TEMPLATE_RESTORE;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)

void c_QuestSetState(
	int nQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	BOOL bRestore);
	
void c_QuestEventQuestStatus(
	int nQuest,
	int nDifficulty,
	QUEST_STATUS eStatus,
	BOOL bRestore );

void c_QuestTrack(
	QUEST * pQuest,
	BOOL bEnableTracking);

void c_AutoTrackQuests(
	BOOL bEnableTracking);
	
void c_QuestEventOpenUI(
	UI_ELEMENT eElement);
	
void c_QuestEventSkillPoint(
	void);
	
void c_QuestEventReloadUI(
	UNIT *pPlayer );

void c_QuestSetUnitVisibility(
	UNIT * unit,
	BOOL bVisible );

void c_QuestEventUnitVisibleTest(
	UNIT * unit );

void c_QuestEventMonsterInit(
	UNIT * monster );

BOOL c_QuestEventCanPickup(
	UNIT *pPlayer,
	UNIT *pAttemptingToPickedUp );

void c_QuestsUpdate(
	GAME *pGame);
	
BOOL c_QuestMapGetIconForLevel(
	int nLevelDefID,
	char * szFrameNameBuf,
	int nFrameNameBufSize,
	DWORD& dwIconColor);
	
void c_QuestEventEndQuestSetup(	
	void);
	
BOOL c_QuestIsAllInitialQuestStateReceived(
	void);
	
void c_QuestAbortCurrent( 
	GAME * game, 
	UNIT * player );
		
BOOL c_QuestIsVisibleInLog(
	QUEST *pQuest);

void c_QuestEventStateChanged(
	QUEST *pQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValueOld,
	QUEST_STATE_VALUE eValueNew);

const WCHAR *c_QuestStateGetLogText(
	UNIT *pPlayer,
	QUEST *pQuest,
	const QUEST_STATE *pState,
	int *pnColor);

void c_QuestDisplayDialog( 
	GAME *pGame,
	const MSG_SCMD_QUEST_DISPLAY_DIALOG *pMessage );

void c_QuestTemplateRestore( 
	GAME *pGame, 
	const MSG_SCMD_QUEST_TEMPLATE_RESTORE *pMessage );

void c_QuestReplaceStringTokens(
	UNIT *pPlayer,
	QUEST *pQuest,
	const WCHAR *puszFormat,
	WCHAR *puszBuffer,
	int nBufferSize );

QUEST_STYLE c_QuestGetStyle(
	GAME * pGame, 
	int nQuestId );

BOOL c_QuestPlayerHasObjectiveInLevel(
	UNIT *pPlayer,
	int nLevelDef);

void c_QuestClearAllStates(
	GAME * pGame,
	int nQuestId );

BOOL c_QuestIsThisLevelNextStoryLevel(
	GAME * pGame,
	int nLevelDefIndex );

#endif !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern BOOL gbFullQuestLog;

	
#endif