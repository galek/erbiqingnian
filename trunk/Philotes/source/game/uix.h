//----------------------------------------------------------------------------
// uix.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_H_
#define _UIX_H_

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum QUEST_LOG_UI_STATE
{
	QUEST_LOG_UI_HIDE = 0,		// force off
	QUEST_LOG_UI_VISIBLE,		// force on
	QUEST_LOG_UI_TOGGLE,		// toggle off if on, on if off
	QUEST_LOG_UI_UPDATE,		// just update text if on
	QUEST_LOG_UI_UPDATE_TEXT,	// update text regardless if on or not
};

enum UI_SHOW_MESSAGE_STATE
{
	UISMS_FADEOUT = 0,
	UISMS_NOFADE,
	UISMS_FORCEOFF,
	UISMS_DIALOG_QUEST,
};

enum UI_SOCIAL_PANELS
{
	UI_SOCIALPANEL_CHAT = 0,

	UI_NUM_SOCIAL_PANELS
};

#define MAX_QUESTLOG_STRLEN	(MAX_STRING_ENTRY_LENGTH)

#define LEVELCHANGE_FADEOUTMULT_DEFAULT			4.5f

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
enum QUEST_TYPE;
struct GAME;
enum UI_MSG_RETVAL;
enum NPC_DIALOG_TYPE;
enum GLOBAL_INDEX;
struct UIX_TEXTURE;
struct UI_COMPONENT;
struct TRADE_SPEC;
enum UI_COMPONENT_ENUM;

BOOL UIInit(
	BOOL bReloading = FALSE,
	BOOL bFillingPak = FALSE,
	int nFillPakSKU = INVALID_LINK,	// if we're filling a localized Pak, this will be the SKU we're using
	BOOL bReloadDirect = FALSE);

void UINewSessionInit(
	void );

void UIGameStartInit(
	void);

void UIProcess(
	void);

void UIFree(
	void);

UI_MSG_RETVAL UISendMessage(
	UINT nWinMsg,
	WPARAM wParam,
	LPARAM lParam);

void UISetCinematicMode(
	BOOL bOn);

int UIItemInitInventoryGfx(
	struct UNIT* pUnit,
	BOOL bEvenIfModelExists = FALSE,
	BOOL bCreateWardrobe = TRUE);

BOOL UIIsMerchantScreenUp();

BOOL UIIsRecipeScreenUp();

BOOL UIIsStashScreenUp();

BOOL UIIsInventoryPackPanelUp();

BOOL UIIsGunModScreenUp();

BOOL UIShowingLoadingScreen(
	void);

BOOL UIUpdatingLoadingScreen(
	void);

void UIShowLoadingScreen(
	int main_as_well,
	const WCHAR * name = L"",
	const WCHAR * subtext = L"",
	BOOL bShowLoadingTip = TRUE);

void UIHideLoadingScreen(
	BOOL bFadeOut = TRUE);

void UIShowQuickMessage(
	const WCHAR * szText,
	float fFadeOutMultiplier = 0.0f,
	const WCHAR * szSubtext = L"");

void UIShowQuickMessage(
	const char * szText,
	float fFadeOutMultiplier = 0.0f,
	const char * szSubtext = "");

void UIComponentActivateQuickMessage(
	UI_COMPONENT* component,
	float fFadeOutMultiplier);

void UIShowProgressChange(
	LPCSTR strProgress);

void UIShowProgressChange(
	LPCWSTR strProgress);

void UIShowPatchClientProgressChange();

void UIShowMessage(
	int nType,
	int nDialog,
	int nParam1 = 0,
	int nParam2 = 0,
	DWORD dwFlags = UISMS_FADEOUT);

BOOL UIHideQuickMessage(
	void);

BOOL UIFadeOutQuickMessage(
	float fFadeOutTime );

void UIDisplayNPCDialog(
	GAME *pGame,
	struct NPC_DIALOG_SPEC *pSpec);

void UIHideNPCDialog(
	enum CONVERSATION_COMPLETE_TYPE eType);		

void UIDisplayNPCStory(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec);

void UIDisplayNPCVideoAndText(
	GAME *pGame,
	NPC_DIALOG_SPEC * pSpec);

void UIRetrieveNPCVideo(
	GAME * game );

void UIHideNPCVideo(
	void );

void UISkillShowCooldown(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill );

BOOL UIWantsUnitStatChange(
	UNITID idUnit);

BOOL UIShowInventoryScreen(
	void);

BOOL UIShowPetInventoryScreen(
	UNIT *pPet);
	
BOOL UIHideMerchantScreen(
	void);

void UICharacterCreateInit();

void UICharacterCreateReset();

BOOL UIShowTradeScreen(
	UNITID idTradingWith,
	BOOL bCancelled,
	const TRADE_SPEC *pTradeSpec);
	
void UIUpdateQuestLog(
	QUEST_LOG_UI_STATE eState,
	int nChangedQuest = INVALID_ID);

UNITID UIGetTargetUnitId(
	void);

struct UNIT* UIGetTargetUnit(
	void);

UNITID UIGetAltTargetUnitId(
	void);

struct UNIT* UIGetAltTargetUnit(
	void);

void UIToggleShowItemNames(
	BOOL bState);

void UISetAlwaysShowItemNames(
	BOOL bState);

void UIUpdateLoadingScreen(
	void);

//BOOL UIShowSocialScreen(
//	int nPanel);
//
//BOOL UIHideSocialScreen(
//	BOOL bEscapeOut);
//
void UIShowBreathMeter(
	void);

void UIHideBreathMeter(
	void);

void UIOnUnitFree( 
	UNIT *pUnit);

void UIDebugLabelsClear();

void UIShowAltMenu(
	BOOL bShow);

void UIQueueItemWindowsRepaint(
	void);

BOOL UIAppCursorAlwaysOn(
	void);
	
int UIDropItem(
	UNITID idUnit);


UIX_TEXTURE* UITextureLoadRaw(
	const char* filename,
	int nWidthBasis = 1600,
	int nHeightBasis = 1200,
	BOOL bAllowAsync = FALSE );

BOOL UITextureLoadFinalize(
	struct UIX_TEXTURE * pUITexture );

//////////////////////////////////////////////////////////////////////////////
// Tugboat Specific Pane menu implementation
//////////////////////////////////////////////////////////////////////////////
enum EPaneMenu
{
	KPaneMenuNone,			//0
	KPaneMenuBackpack,
	KPaneMenuEquipment,		//2
	KPaneMenuCharacterSheet,
	KPaneMenuSkills,		//4
	KPaneMenuCrafting,
	KPaneMenuQuests,		//6
	KPaneMenuMerchant,
	KPaneMenuStash,			//8
	KPaneMenuTrade,
	KPaneMenuRecipe,		//10
	KPaneMenuItemAugment,
	KPaneMenuAchievements,	//12
	KPaneMenuHirelingEquipment,
	KPaneMenuCube,			//14
	KPaneMenuCraftingRecipes,	
	KPaneMenuMail,
	KPaneMenuCraftingTrainer,	
	KPaneMenus
};

BOOL UIPaneVisible( EPaneMenu Pane );

UI_MSG_RETVAL UITogglePaneMenu( EPaneMenu Pane1,
		EPaneMenu Pane2 = KPaneMenuNone,
		EPaneMenu Pane3 = KPaneMenuNone );

void UIShowPaneMenu( EPaneMenu Pane1,
		EPaneMenu Pane2 = KPaneMenuNone );

void UIHidePaneMenu( EPaneMenu Pane1,
		EPaneMenu Pane2 = KPaneMenuNone );

BOOL UIHideAllPaneMenus( void );

void UIPanesInit( void );

void UIUpdatePaneMenuPositions( void );

void UISetQuestButton( int Button );

//////////////////////////////////////////////////////////////////////////////
// End of TUG Pane menu codes
//////////////////////////////////////////////////////////////////////////////


void UI_TB_LookForBadItems( void );

void UI_TB_HideModalDialogs( void );

BOOL UI_TB_HideAllMenus( void );

BOOL UI_TB_IsNPCDialogUp();

BOOL UI_TB_IsGenericDialogUp();

BOOL UI_TB_IsPortalScreenUp();

BOOL UI_TB_IsWorldMapScreenUp();

BOOL UI_TB_IsTravelMapScreenUp();

BOOL UI_TB_ModalDialogsOpen();

BOOL UI_TB_HideTownPortalScreen();

BOOL UI_TB_HideAnchorstoneScreen();

BOOL UI_TB_HideRunegateScreen();

BOOL UI_TB_HideWorldMapScreen( BOOL bEscapeOut);

BOOL UI_TB_ShowWorldMapScreen( int nRevealArea = INVALID_ID );	

BOOL UI_TB_HideTravelMapScreen( BOOL bEscapeOut);

BOOL UI_TB_ShowTravelMapScreen( void );	

void UIShowTaskCompleteMessage( const WCHAR *questname );

void UI_TB_ShowRoomEnteredMessage( const WCHAR *questname );

void UITasksSelectQuest( int nIndex );

void UITasksRefresh( void );

//////////////////////////////////////////////////////////////////////////////
// End of Tug specific code
//////////////////////////////////////////////////////////////////////////////


void UISetSimpleHoverText(
	float x1,
	float y1,
	float x2,
	float y2,
	const WCHAR *szString,
	BOOL bEvenIfMouseDown = FALSE,
	int nAssociateComponentID = INVALID_ID);

void UISetSimpleHoverText(
	UI_COMPONENT *pComponent,
	const WCHAR *szString,
	BOOL bEvenIfMouseDown = FALSE,
	int nAssociateComponentID = INVALID_ID);

BOOL UIGetHoverTextSkill(
	int nSkill,
	int nSkillLevel,
	UNIT *pUnit,
	DWORD dwSkillLearnFlags,
	WCHAR *puszBuffer,
	int nBufferSize,
	BOOL bFormatting = TRUE);

	
void UISetClickedTargetUnit(
					 UNITID idUnit,
					 BOOL bSelectionInRange,
					 BOOL bLeft,
					 int nSkill );
UNITID UIGetClickedTargetUnitId(
						 void);
struct UNIT* UIGetClickedTargetUnit(
	void);

int UIGetClickedTargetSkill( void );

int UIGetClickedTargetLeft( void );

void TransformUnitSpaceToScreenSpace(
	UNIT* unit,
	int* x,
	int* y);

void TransformWorldSpaceToScreenSpace(
	VECTOR* vLoc,
	int* x,
	int* y);

void UIUpdateRewardMessage(
	int nNumRewardTakes,
	int nOfferDefinition);

void UIEnterRTSMode(
	void);

void UIExitRTSMode(
	void);

void UIUpdateDPS(
	float fDPS);

void UIShowSubtitle( 
	const WCHAR * szText);

void UIShowPausedMessage( 
	BOOL bShow);

BOOL UIComponentActivate(
	UI_COMPONENT* component,
	BOOL bActivate,
	DWORD dwDelay = 0,
	DWORD * pdwTotalAnimTime = NULL,
	BOOL bFromAnimRelationship = FALSE,
	BOOL bOnlyIfUserActive = FALSE,
	BOOL bSetUserActive = TRUE,
	BOOL bImmediate = FALSE);

BOOL UIComponentActivate(
	UI_COMPONENT_ENUM eComponent,
	BOOL bActivate,
	DWORD dwDelay = 0,
	DWORD * pdwTotalAnimTime = NULL,
	BOOL bFromAnimRelationship = FALSE,
	BOOL bOnlyIfUserActive = FALSE,
	BOOL bSetUserActive = TRUE,
	BOOL bImmediate = FALSE);

void UIComponentAddChild(
	UI_COMPONENT* parent,
	UI_COMPONENT* child);

void UIComponentFree(
	UI_COMPONENT* component);

void UIMuteSounds(
	BOOL bMute);

void UICancelDelayedActivates(
	UI_COMPONENT *pRelatedToComponent);

UI_COMPONENT * UIXmlLoadBranch(
	LPCWSTR wszFileName,
	LPCSTR szLoadComponent,
	LPCSTR szComponentName,
	UI_COMPONENT *pParent);

//----------------------------------------------------------------------------
enum UI_ACTION
{
	UIA_HIDE,
	UIA_SHOW,
	UIA_ACTIVATE,
	UIA_DEACTIVATE,
	UIA_SETFOCUSUNIT,
};

void UIComponentArrayDoAction( 
	const enum UI_COMPONENT_ENUM *pComps,
	UI_ACTION eAction,
	UNITID idParam);

void UIComponentArrayDoAction( 
	struct UI_COMPONENT **pComps,
	UI_ACTION eAction,
	UNITID idParam);

#endif
