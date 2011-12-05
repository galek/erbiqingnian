//----------------------------------------------------------------------------
// FILE: npc.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __NPC_H_
#define __NPC_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _UIX_PRIV_H_
#include "uix_priv.h"
#endif

#ifndef __GLOBALINDEX_H_
#include "globalindex.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
class cCurrency;
enum GLOBAL_INDEX;
enum UNIT_INTERACT;
struct HIRELING_CHOICE;
struct QUEST;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum INTERACT_EVENT
{
	IE_INVALID = -1,
	
	IE_HELLO_GENERIC,
	IE_HELLO_TEMPLAR,
	IE_HELLO_CABALIST,
	IE_HELLO_HUNTER,
	IE_HELLO_MALE,
	IE_HELLO_FEMALE,
	IE_HELLO_FACTION_BAD,
	IE_HELLO_FACTION_NEUTRAL,
	IE_HELLO_FACTION_GOOD,

	IE_GOODBYE_GENERIC,
	IE_GOODBYE_TEMPLAR,
	IE_GOODBYE_CABALIST,
	IE_GOODBYE_HUNTER,
	IE_GOODBYE_MALE,
	IE_GOODBYE_FEMALE,
	IE_GOODBYE_FACTION_BAD,
	IE_GOODBYE_FACTION_NEUTRAL,
	IE_GOODBYE_FACTION_GOOD,
	
	IE_MAX_CHOICES		// keep this last
};

//----------------------------------------------------------------------------
struct NPC_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	int nSound[ IE_MAX_CHOICES ];
};

//----------------------------------------------------------------------------
enum NPC_INTERACT_TYPE
{
	NPC_INVALID_INTERACTION = -1,
	NPC_DEFAULT_INTERACTION = 0,
	NPC_ANALYZE_INTERACTION,
	NPC_GOSSIP_INTERACTION,
};

//----------------------------------------------------------------------------
enum CONVERSATION_COMPLETE_TYPE
{
	CCT_INVALID = -1,
	
	CCT_OK,
	CCT_CANCEL,
	CCT_FORCE_CANCEL,
	
};

//----------------------------------------------------------------------------
enum NPC_DIALOG_TYPE
{
	NPC_DIALOG_NO_BUTTONS = 0,	// npc dialog window with no buttons (tutorial non-modal)
	NPC_DIALOG_OK,				// npc dialog box with OK button only
	NPC_DIALOG_OK_CANCEL,		// npc dialog box with OK and CANCEL buttons
	NPC_VIDEO_INCOMING,			// npc video dialog box with OK button pending... with random time delay
	NPC_VIDEO_INSTANT_INCOMING,	// npc video dialog box with OK button pending, but no delay before msg
	NPC_VIDEO_OK,				// npc video dialog box with OK button only
	NPC_VIDEO_OK_CANCEL,		// npc video dialog box with OK and CANCEL buttons
	NPC_DIALOG_ANALYZE,			// npc dialog window with OK button and NPC analyze description
	NPC_DIALOG_TWO_OK,			// npc dialog with two NPCs and an OK button
	NPC_DIALOG_OK_REWARD,		// npc dialog box with OK and active reward selection
	NPC_DIALOG_LIST,			// npc dialog box listing all available questlines for selection
	NPC_LOCATION_LIST,			// npc dialog box listing all available warp destinations
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

const NPC_DATA *NPCGetData(
	GAME *pGame,
	UNIT *pUnit);
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
	
void s_NPCDoGreeting( 
	GAME *pGame, 
	UNIT *pNPC, 
	UNIT *pPlayer);

void s_NPCDoGoodbye( 
	GAME *pGame, 
	UNIT *pNPC, 
	UNIT *pPlayer);

void s_UnitInteract(
	UNIT *pPlayer, 
	UNIT *pTarget,
	UNIT_INTERACT eInteract);
	
void s_NPCTalkStart( 
	UNIT *pPlayer, 
	UNIT *pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward = INVALID_LINK,
	GLOBAL_INDEX eSecondNPC = GI_INVALID,
	int nQuestId = INVALID_LINK  );

void s_NPCQuestTalkStart( 
	QUEST * pQuest, 
	UNIT * pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward = INVALID_LINK,
	GLOBAL_INDEX eSecondNPC = GI_INVALID, 
	BOOL bLeaveUIOpenOnOk = FALSE);

void s_NPCTalkStartToTeam( 
	UNIT *pPlayer, 
	UNIT *pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward = INVALID_LINK,
	GLOBAL_INDEX eSecondNPC = GI_INVALID );

void s_NPCVideoStart( 
	GAME *pGame, 
	UNIT *pPlayer, 
	GLOBAL_INDEX eNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward = INVALID_LINK,
	GLOBAL_INDEX eInterruptNPC = GI_INVALID);

void s_NPCTalkStop(
	GAME *pGame,
	UNIT *pPlayer,
	CONVERSATION_COMPLETE_TYPE eCompleteType = CCT_OK,
	int nRewardSelection = -1,
	BOOL bCancelOffers = TRUE);
	
void s_NPCEmoteStart(
	GAME *pGame, 
	UNIT *pPlayer, 
	int nDialogEmote);

void s_InteractInfoUpdate(
	UNIT *pPlayer,
	UNIT *pTarget,
	BOOL bUpdatingAllQuests = FALSE );

int s_NPCInfoPriority( 
	enum INTERACT_INFO eInfo);

void s_NPCBuyHireling(
	UNIT *pPlayer,
	UNIT *pForeman,
	int nBuyIndex);
		
//----------------------------------------------------------------------------
// TALKING
//----------------------------------------------------------------------------

void s_TalkSetTalkingTo(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog);

UNIT *s_TalkGetTalkingTo( 
	UNIT *pPlayer);

BOOL s_TalkIsBeingTalkedTo(
	UNIT *pUnit);

BOOL NPCCanBeExamined(
	UNIT *pPlayer,
	UNIT *pNPC);

int x_NPCInteractGetAvailable(
	UNIT *pPlayer,
	UNIT *pNPC,
	struct INTERACT_MENU_CHOICE *ptBuffer,
	int nBufferSize);

//----------------------------------------------------------------------------
// CLIENT FUNCTIONS
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
struct NPC_DIALOG_SPEC
{
	UNITID			idTalkingTo;		// instance of the unit the player is talking with
	GLOBAL_INDEX	eGITalkingTo;		// used for the video transmission "talking" or a 2nd npc in a conversation
	GLOBAL_INDEX	eGITalkingTo2;		// used for the video transmission for a 2nd interrupt character
	
	// what to display in dialog (you can use either talkids or straight text)
	int nDialog;
	int nDialogReward;
	const WCHAR *puszText;
	const WCHAR *puszRewardText;
	int nQuestDefId;
	int nOfferDefId;

	NPC_DIALOG_TYPE eType;
	BOOL			bLeaveUIOpenOnOk;
	DIALOG_CALLBACK tCallbackOK;
	DIALOG_CALLBACK tCallbackCancel;

	NPC_DIALOG_SPEC::NPC_DIALOG_SPEC( void )
		:	idTalkingTo( INVALID_ID ),
			eGITalkingTo( GI_INVALID ),
			eGITalkingTo2( GI_INVALID ),
			nDialog( INVALID_LINK ),
			nDialogReward( INVALID_LINK ),
			puszText( NULL ),
			puszRewardText( NULL ),
			eType( NPC_DIALOG_OK ),
			nQuestDefId( INVALID_ID ),
			nOfferDefId( INVALID_ID ),
			bLeaveUIOpenOnOk( FALSE )
	{
		DialogCallbackInit( tCallbackOK );
		DialogCallbackInit( tCallbackCancel );
	}
		
};
	
void c_NPCTalkStart(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec);

void c_NPCTalkForceCancel(
	void);
	
void c_NPCHeadstoneInfo(
	UNIT *pUnit,
	UNIT *pAsker,
	BOOL bCanReturnNow,
	int nCost,
	int nLevelDefDest);

void c_NPCDungeonWarpInfo( 
	UNIT *pPlayer, 
	UNIT *pWarper, 
	int nDepth,
	cCurrency nCost);

void c_NPCCreateGuildInfo( 
	UNIT *pPlayer, 
	UNIT *pGuildmaster, 
	cCurrency nCost);

void c_NPCRespecInfo( 
	UNIT *pPlayer, 
	UNIT *pRespeccer, 
	cCurrency nCost,
	BOOL bCrafting);

void c_NPCTransporterWarpInfo( 
	UNIT *pPlayer, 
	UNIT *pWarper );

void c_NPCPvPSignerUpperInfo( 
	UNIT * pPlayer, 
	UNIT * pPvPSignerUpper);

void c_NPCVideoStart(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec);

void c_NPCEmoteStart(
	GAME * pGame,
	int nDialogEmote);

void c_NPCTalkingTo( 
	UNITID idTalkingTo);

void c_NPCFlagSoundsForLoad(
	GAME * pGame,
	int nNPC,
	BOOL bLoadNow);

#endif // !SERVER_VERSION
#endif  // end __NPC_H_

