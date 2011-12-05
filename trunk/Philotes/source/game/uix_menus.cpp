//----------------------------------------------------------------------------
// uix_menus.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix.h"
#include "uix_graphic.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_graphic.h"
#include "uix_scheduler.h"
#include "uix_menus.h"
#include "uix_options.h"	// CHB 2007.02.01
#include "uix_chat.h"
#include "dialog.h"
#include "fileversion.h"
#include "player.h"
#include "level.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "c_camera.h"
#include "e_environment.h"
#include "e_settings.h"
#include "unit_priv.h" // also includes units.h, which includes game.h
#include "gameunits.h"
#include "c_appearance.h"
#include "c_animation.h"
#include "unitmodes.h"
#include "stringtable.h"
#include "c_message.h"
#include "globalindex.h"
#include "c_input.h"
#include "npc.h"
#include "quests.h"
#include "console_priv.h"
#include "filepaths.h"
#include "teams.h"
#include "c_townportal.h"
#include "lightmaps.h"
#include "items.h"
#include "dictionary.h"
#include "e_budget.h"
#include "c_quests.h"
#include "definition_priv.h"
#include "warp.h"
#include "characterclass.h"
#include "console.h"
#include "namefilter.h"
#include "characterlimit.h"
#include "c_reward.h"
#include "c_trade.h"
#include "sku.h"
#include "duel.h"
#if !ISVERSION(SERVER_VERSION)
#include "svrstd.h"
#include "c_chatNetwork.h"
#include "c_chatBuddy.h"
#include "c_chatIgnore.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "UserChatCommunication.h"
#include "c_movieplayer.h"
#include "c_credits.h"
#include "c_characterselect.h"
#include "c_network.h"
#include "c_QuestClient.h"
#endif
#include "bookmarks.h"
#include "unitfile.h"
#include "skills.h"
#include "skills.h"
#include "states.h"
#include "CrashHandler.h"
#include "chat.h"
#include "ServerSuite\BillingProxy\c_BillingClient.h"
#include "serverlist.h"
#include "region.h"
#include "stringreplacement.h"
#include "language.h"
#include "region.h"

#if !ISVERSION(SERVER_VERSION)

#define USE_DELAYED_CHARACTER_CREATION

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------
#define UI_MENU_SELECTEDCOLOR			0xfff78e14
#define UI_MENU_NOTSELECTEDCOLOR		0xffffffff
#define UI_MENU_DISABLEDCOLOR			0x77777777
#define UI_MAX_CHARACTER_CLASSES		16
#define UI_MAX_CHARACTER_RACES			8
#define UI_MAX_CHARACTER_BUTTONS		12
#define MAX_NAME_LIST					MAX_CHARACTERS_PER_SINGLE_PLAYER

#define CHARACTER_DEFAULT_COLOR		((DWORD) GFXCOLOR_MAKE( 106, 182, 255, 255 ))
#define CHARACTER_HARDCORE_COLOR	((DWORD) GFXCOLOR_MAKE( 255, 125, 125, 255 ))
#define CHARACTER_DISABLED_COLOR	((DWORD) GFXCOLOR_MAKE( 175, 175, 175, 255 ))

#define UI_REALM_COMBO		"realm combo"

static const QWORD SERVERLIST_OVERRIDE_FLAG = (QWORD)1 << 63;

//If we want more characters in single player,
// we should use MAX(MAX_CHARACTERS_PER_ACCOUNT,32)
//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
STARTGAME_PARAMS	g_UI_StartgameParams;
STARTGAME_CALLBACK	g_UI_StartgameCallback = NULL;

static WCHAR gpszCharSelectNames[ MAX_NAME_LIST ][ MAX_CHARACTER_NAME ];
static WCHAR gpszCharSelectGuildNames[ MAX_NAME_LIST ][ MAX_GUILD_NAME ];
static UNIT * gppCharSelectUnits[ MAX_NAME_LIST ];
static UNIT * gpppCharCreateUnits[ UI_MAX_CHARACTER_CLASSES ][ UI_MAX_CHARACTER_RACES ][ GENDER_NUM_CHARACTER_GENDERS ];
static BOOL gbCharCreateUnitsCreated = FALSE;
static int gnStartgameLevel = INVALID_ID;
static int gnCharSelectScrollPos = 0;
static int gnNumCharButtons = 0;
static int gnCharButtonToPlayerSlot[ UI_MAX_CHARACTER_BUTTONS ];

enum CSLL_STATE
{
	CSLL_STATE_INIT = 0,
	CSLL_STATE_WAITING,
	CSLL_STATE_COMPLETE,
};

struct CHAR_SELECT_LEVEL_LOAD
{
	int			nLevelID;
	CSLL_STATE	eLoadState;
};
#define NUM_CHAR_SELECT_LEVELS		1
static CHAR_SELECT_LEVEL_LOAD sgtCharSelectLevels[ NUM_CHAR_SELECT_LEVELS ] = {0};



//----------------------------------------------------------------------------
// Forwards
//----------------------------------------------------------------------------
void UISetCurrentMenu(UI_COMPONENT * pMenu);
void UICharacterCreateInit(void);
void UICharacterCreateReset(void);
UI_MSG_RETVAL UIStartGameMenuDoSingleplayer( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIStartGameMenuDoMultiplayer( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIInputOverrideRemove(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
static void sSwitchToCreateUnit( BOOL bCreateAllPaperdolls = FALSE, BOOL bForceImmediatePanelActivate = FALSE );

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#include "uix_debug.h"
#include "windowsmessages.h"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CharCreateResetUnitLists(void)
{
	structclear(gppCharSelectUnits);
	structclear(gpppCharCreateUnits);
}

static void sUpdateCharSelectButtons();

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sGetCharCreateUnit()
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETNULL( pGame );
	return UnitGetById( pGame, g_UI_StartgameParams.m_idCharCreateUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateOnKeyDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

#if ISVERSION(DEVELOPMENT)
	//if (wParam == VK_OEM_3)	// tilde
	//{	
	//	UNITID idUnit = g_UI_StartgameParams.m_idCharCreateUnit;
	//	CSchedulerFree();
	//	UIFree();
	//	CSchedulerInit();
	//	UIInit(TRUE);

	//	// preserve the old unit id
	//	g_UI_StartgameParams.m_idCharCreateUnit = idUnit;

	//	if (!AppIsTugboat())
	//	{
	//	    UICharacterCreateReset();
	//       }
	//       else
	//       {
	//		UICharacterCreateInit();
	//	}

	//	UIComponentSetActive(UIComponentGetByEnum(UICOMP_CHARACTER_SELECT), TRUE);
	//}

	if (wParam == 'F' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		UISendMessage(WM_TOGGLEDEBUGDISPLAY, UIDEBUG_FPS, -1);
	}

	if (wParam == 191 && GameGetDebugFlag(AppGetCltGame(), DEBUGFLAG_ALLOW_CHEATS))
	{
		ConsoleActivateEdit(FALSE,FALSE);
	}
#endif

	return UIMSG_RET_HANDLED_END_PROCESS;
}

////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//UI_MSG_RETVAL UIMenuPanelOnKeyDown(
//	UI_COMPONENT* component,
//	int msg,
//	DWORD wParam,
//	DWORD lParam)
//{
//	if (UIComponentIsPanel(component))
//	{
//		
//	}
//
//	return UIMSG_RET_NOT_HANDLED;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
static const char * MAIN_MENU_CHEAT_CODE = "YSHIPPYQUIT";
#endif

UI_MSG_RETVAL UIMainMenuOnKeyDown(
								  UI_COMPONENT* component,
								  int msg,
								  DWORD wParam,
								  DWORD lParam)
{
	static int nCheatCodeMatch = 0;

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetActive(component))
	{
		nCheatCodeMatch = 0;
		return UIMSG_RET_NOT_HANDLED;
	}

	if (AppGetState() != APP_STATE_MAINMENU)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

#if ISVERSION(DEVELOPMENT)
	static int nCheatCodeLen = PStrLen(MAIN_MENU_CHEAT_CODE);

	if (nCheatCodeMatch <= nCheatCodeLen)
	{
		if (wParam == (DWORD)MAIN_MENU_CHEAT_CODE[nCheatCodeMatch])
		{
			//if (msg == )
			{
				nCheatCodeMatch++;
				if (nCheatCodeMatch >= nCheatCodeLen)
				{
					LogMessage("Quitting via main menu cheat.\n");
					PostQuitMessage(0);
				}
			}
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	nCheatCodeMatch = 0;
#endif

	return UIMSG_RET_NOT_HANDLED; //UIMenuPanelOnKeyDown(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMainMenuPaintIfNoMovieIsPlaying(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if(c_MoviePlayerGetPlaying() != INVALID_LINK)
	{
		UIComponentRemoveAllElements(component);
		return UIMSG_RET_HANDLED;
	}

	return UIComponentOnPaint(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharacterRandomizeHeight(
									  UNIT * pUnit )
{
	RAND tRand;
	RandInit( tRand );

	float fWeightPercent = RandGetFloat( tRand, 1.0f );
	float fHeightPercent = RandGetFloat( tRand, 1.0f );
	UnitSetShape( pUnit, fHeightPercent, fWeightPercent );

	// update the sliders from these values
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE_HEIGHT_BAR);
	if (pComponent)
	{
		pComponent->m_ScrollPos.m_fY = 255.0f * fHeightPercent;
		UIScrollBarOnScroll(pComponent, UIMSG_SCROLL, 0, 0);
	}
	pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE_WEIGHT_BAR);
	if (pComponent)
	{
		pComponent->m_ScrollPos.m_fY = 255.0f * fWeightPercent;
		UIScrollBarOnScroll(pComponent, UIMSG_SCROLL, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCharacterSelectCreateLevels()
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETFALSE( pGame );

	// CML 2006.8.24
	// NOTE:  Hard-coding player class excel line here, since all player classes/genders
	// currently use the same character create level.  Eventually, we should create all the
	// permutations and have them ready.
	ASSERT( NUM_CHAR_SELECT_LEVELS == 1 );  // TJT - some players are disabled in some versions and we need to search for the level
	int nNumPlayerClasses = ExcelGetNumRows( pGame, DATATABLE_PLAYERS );
	for ( int nPlayerClass = 0; nPlayerClass < nNumPlayerClasses; nPlayerClass++ )
	{
		//int nPlayerClass = UnitGetClass(pUnit);  // how we mean to get the player class
		sgtCharSelectLevels[ 0 ].eLoadState		= CSLL_STATE_WAITING;
		sgtCharSelectLevels[ 0 ].nLevelID		= c_LevelCreatePaperdollLevel( nPlayerClass );
		if ( sgtCharSelectLevels[ 0 ].nLevelID != INVALID_ID )
			break;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCharacterSelectStopSkillRequest(
	GAME * pGame, 
	UNIT * pUnit, 
	const EVENT_DATA & event_data)
{
	int nPaperdollSkill = int(event_data.m_Data1);
	SkillStopRequest(pUnit, nPaperdollSkill);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCharacterSelectStartSkill(
	GAME * pGame, 
	UNIT * pUnit, 
	const EVENT_DATA & event_data)
{
	GAME_STATE ePreviousGameState = (GAME_STATE)event_data.m_Data2;
	if(GameGetState(pGame) == GAMESTATE_LOADING || ePreviousGameState == GAMESTATE_LOADING)
	{
		EVENT_DATA tEventData(event_data.m_Data1, GameGetState(pGame));
		UnitRegisterEventTimed(pUnit, sCharacterSelectStartSkill, &tEventData, 40);
		return TRUE;
	}

	int nPaperdollSkill = int(event_data.m_Data1);
	if(nPaperdollSkill >= 0)
	{
		DWORD dwSkillStartFlags = SKILL_START_FLAG_DONT_SEND_TO_SERVER | SKILL_START_FLAG_IGNORE_POWER_COST | SKILL_START_FLAG_IGNORE_COOLDOWN;
		SkillStartRequest( pGame, pUnit, nPaperdollSkill, INVALID_ID, VECTOR(0), dwSkillStartFlags, 0 );

		EVENT_DATA tEventData(nPaperdollSkill);
		UnitRegisterEventTimed(pUnit, sCharacterSelectStopSkillRequest, &tEventData, 1);
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharacterCreateEndBadassModes(
	GAME * pGame,
	UNIT * pUnit)
{
	UnitEndMode( pUnit, MODE_IDLE_BADASS, 0, FALSE );
	UnitEndMode( pUnit, MODE_INTRO_BADASS, 0, FALSE );
	UnitEndMode( pUnit, MODE_SHOWOFF_BADASS, 0, FALSE );
	UnitEndMode( pUnit, MODE_IDLE_PAPERDOLL, 0, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharacterCreateSelectUnitResetUnit(
	GAME * pGame,
	UNIT * pUnit)
{
	ASSERT_RETURN( pUnit );

	c_UnitSetNoDraw(pUnit, TRUE, FALSE);
	StateClearAllOfType(pUnit, STATE_CHARACTERCREATE_SKILL);
	UnitUnregisterTimedEvent(pUnit, sCharacterSelectStartSkill);
	SkillStopAll(pGame, pUnit);
	c_ModelAttachmentRemoveAllByOwner(c_UnitGetModelId(pUnit), ATTACHMENT_OWNER_SKILL, INVALID_ID);
	sCharacterCreateEndBadassModes( pGame, pUnit );
	if( AppIsHellgate() )
	{
		c_UnitSetMode( pUnit, MODE_INTRO_BADASS );
	}
	else
	{
		c_UnitSetMode( pUnit, MODE_IDLE_CHARSELECT );
	}

	StateClearAllOfType( pUnit, STATE_GEM );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#include "dungeon.h"
#include "drlg.h"
static BOOL sCharacterSelectUnit(
								 UNIT *pUnit,
								 BOOL bStartSkill = FALSE)
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETFALSE( pGame );

	UNIT *pLastUnit = GameGetControlUnit(pGame);
	if (pLastUnit == pUnit)
	{
		return TRUE;
	}
	if (pLastUnit)
	{
		sCharacterCreateSelectUnitResetUnit(pGame, pLastUnit);
	}
	UNIT *pCreateUnit = sGetCharCreateUnit();
	if( pCreateUnit )
	{
		sCharacterCreateSelectUnitResetUnit(pGame, pCreateUnit);
	}
	for ( int iChar = 0; iChar < MAX_NAME_LIST; iChar++ )
	{
		UNIT *pOldUnit = gppCharSelectUnits[iChar];
		if (pOldUnit)
		{
			sCharacterCreateSelectUnitResetUnit(pGame, pOldUnit);
		}
	}

	// eventually, we'll select the proper level based on the player class
#if !ISVERSION(SERVER_VERSION)
	if (!AppIsTugboat())
	{
		for ( int i = 0; i < NUM_CHAR_SELECT_LEVELS; i++ )
		{
			if ( sgtCharSelectLevels[ i ].nLevelID != INVALID_ID )
			{
				AppSetCurrentLevel( sgtCharSelectLevels[ i ].nLevelID );
				break;
			}
		}
	}
#endif

	if (pUnit)
	{
		int nLevelId = INVALID_ID;
		if (!AppIsTugboat())
		{
			nLevelId = AppGetCurrentLevel();
			ASSERT( nLevelId != INVALID_ID );
		}
		else
		{
			int nPlayerClass = UnitGetClass(pUnit);
			nLevelId = c_LevelCreatePaperdollLevel( nPlayerClass );

#if !ISVERSION(SERVER_VERSION)
			AppSetCurrentLevel( nLevelId );							
#endif

		}

		DWORD dwWarpFlags = 0;
		SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );

		VECTOR vPosition;
		ROOM* pRoom = LevelGetStartPosition( 
			pGame, 
			pUnit,
			nLevelId, 
			vPosition, 
			pUnit->vFaceDirection, 
			dwWarpFlags);
		if( AppIsTugboat() )
		{
			vPosition = VECTOR( -116.9987, -221.0f, 7.167f );
		}
		UnitSetPosition( pUnit, vPosition );

		ASSERT_RETFALSE( pRoom );

		UnitUpdateRoom( pUnit, pRoom, FALSE );
		VECTOR vDirection( 0, -1, 0 );
		if( AppIsTugboat() )
		{
			VectorZRotation( vDirection, DegreesToRadians( 60.0f ) );
		}
		UnitSetFaceDirection( pUnit, vDirection, TRUE );

		c_UnitUpdateGfx( pUnit );
		if( AppIsHellgate() )
		{
			if(bStartSkill)
			{
				sCharacterCreateEndBadassModes( pGame, pUnit );
				c_UnitSetMode( pUnit, MODE_INTRO_BADASS );
			}
			else
			{
				sCharacterCreateEndBadassModes( pGame, pUnit );
				c_UnitSetMode( pUnit, MODE_IDLE_PAPERDOLL );
			}
		}
		else
		{
			c_UnitSetMode( pUnit, MODE_IDLE_CHARSELECT );

		}
		c_UnitSetNoDraw(pUnit, ( AppGetState() == APP_STATE_CHARACTER_CREATE ) ? FALSE : TRUE , FALSE);
	}
	else
	{
		ASSERTX(AppIsTugboat(), "Brennan says: \"We should never not be selecting a character.\"  However, pUnit in sCharacterSelectUnit is NULL.\n" );
		// temp level load for no character
		int nLevelDef = GlobalIndexGet( GI_LEVEL_CHARACTER_CREATE );
		int nDRLG = DungeonGetLevelDRLG( pGame, nLevelDef, TRUE );

		LEVEL_TYPE tLevelType;
		tLevelType.nLevelDef = nLevelDef;
		LEVEL * pLevel = LevelFindFirstByType( pGame, tLevelType );
		if (pLevel == NULL)
		{
			V( e_TextureBudgetUpdate() );

			int nEnvironment = DungeonGetDRLGBaseEnvironment( nDRLG );

			// FIX THIS
			c_SetEnvironmentDef( pGame, 0, nEnvironment, "", TRUE );

			// level spec
			LEVEL_SPEC tSpec;
			tSpec.idLevel = INVALID_ID;
			tSpec.tLevelType.nLevelDef = nLevelDef;
			tSpec.nDRLGDef = nDRLG;
			tSpec.bClientOnlyLevel = TRUE;

			pLevel = LevelAdd( pGame, tSpec );

			if ( pLevel )
				DRLGCreateLevel( pLevel->m_pGame, pLevel, pLevel->m_dwSeed, INVALID_ID );
		}

#if !ISVERSION(SERVER_VERSION)
		if ( pLevel )
			AppSetCurrentLevel( pLevel->m_idLevel );
#endif
	}

	// Let's remove all of the summoned monsters (elementals aren't monsters, so we have to do another search)
	UNITTYPE eTypes[2] = {UNITTYPE_MONSTER, UNITTYPE_PET};
	for(int i=0; i<2; i++)
	{
		UNITID idMonsters[MAX_TARGETS_PER_QUERY];
		SKILL_TARGETING_QUERY tTargetingQuery;
		tTargetingQuery.pSeekerUnit = pUnit;
		tTargetingQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
		tTargetingQuery.pnUnitIds = idMonsters;
		tTargetingQuery.fMaxRange = 50.0f;
		tTargetingQuery.tFilter.nUnitType = eTypes[i];
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
		SkillTargetQuery(pGame, tTargetingQuery);

		for(int j=0; j<tTargetingQuery.nUnitIdCount; j++)
		{
			UNIT * pMonster = UnitGetById(pGame, idMonsters[j]);
			if(pMonster)
			{
				c_UnitSetNoDraw(pMonster, TRUE, TRUE);
				UnitFree(pMonster);
			}
		}
	}

	GameSetControlUnit( pGame, pUnit );
	if( pUnit )
	{
		// we need to flip the nodraws on and off again, so it catches
		// any new paperdolls that might have been created
		c_UnitSetNoDraw(pUnit, TRUE, FALSE);
		c_UnitSetNoDraw(pUnit, ( AppGetState() == APP_STATE_CHARACTER_CREATE ) ? FALSE : TRUE, FALSE);
	}
	c_CameraSetViewType( VIEW_CHARACTER_SCREEN, AppIsTugboat() );

	const UNIT_DATA * pPlayerData = UnitGetData(pUnit);
	UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_STARTGAME_CLASS_LABEL);;
	if (pComponent)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComponent);
#if ISVERSION(DEVELOPMENT)
		const char *pszKey = StringTableGetKeyByStringIndex( pPlayerData->nString );
		UILabelSetTextByStringKey( pLabel, pszKey );
#else
		UILabelSetText(pLabel, StringTableGetStringByIndex( pPlayerData->nString ));
#endif
	}

	g_UI_StartgameParams.m_dwGemActivation = 0;
	pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE);
	if ( pComponent )
		UIButtonSetDown( pComponent, "gem btn", FALSE );

	pComponent = UIComponentGetByEnum(UICOMP_STARTGAME_SEX_LABEL);
	if (pComponent)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComponent);
		GLOBAL_STRING eString = g_UI_StartgameParams.m_eGender == GENDER_MALE ? GS_SEX_MALE : GS_SEX_FEMALE;
		UILabelSetTextByGlobalString(pLabel, eString);
	}

	pComponent = UIComponentGetByEnum(UICOMP_STARTGAME_ACCESSORIES_LABEL);
	if (pComponent)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComponent);
		GLOBAL_STRING eString = g_UI_StartgameParams.m_eGender == GENDER_MALE ? GS_UI_CHAR_CREATE_FACIAL_HAIR : GS_UI_CHAR_CREATE_ACCESSORIES;
		UILabelSetTextByGlobalString(pLabel, eString);
	}

	pComponent = UIComponentGetByEnum(UICOMP_STARTGAME_RACE_LABEL);
	if (pComponent)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComponent);
		const PLAYER_RACE_DATA* pPlayerRaceData = (PLAYER_RACE_DATA *)ExcelGetData(NULL, DATATABLE_PLAYER_RACE, g_UI_StartgameParams.m_nCharacterRace );
		UILabelSetText( pLabel, StringTableGetStringByIndex( pPlayerRaceData->nDisplayString ) );
	}

	float fHeightPercent = 0.0f;
	float fWeightPercent = 0.0f;
	UnitGetShape(pUnit, fHeightPercent, fWeightPercent);
	// update the sliders from these values
	pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE_HEIGHT_BAR);
	if (pComponent)
	{
		pComponent->m_ScrollPos.m_fY = 255.0f * fHeightPercent;
		UIScrollBarOnScroll(pComponent, UIMSG_SCROLL, 0, 0);
	}
	pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE_WEIGHT_BAR);
	if (pComponent)
	{
		pComponent->m_ScrollPos.m_fY = 255.0f * fWeightPercent;
		UIScrollBarOnScroll(pComponent, UIMSG_SCROLL, 0, 0);
	}

	UI_COMPONENT * pDescription = UIComponentGetByEnum( UICOMP_CHARACTER_CREATE_DESCRIPTION );
	if ( pDescription )
	{
		UIComponentSetVisible( pDescription, TRUE );

		const UNIT_DATA *pUnitData = UnitGetData(pUnit);

		if (pUnitData->szCharSelectFont[0])
		{
			UIX_TEXTURE *pFontTexture = UIGetFontTexture(LANGUAGE_CURRENT);
			UIX_TEXTURE_FONT *pFont = (UIX_TEXTURE_FONT*)StrDictionaryFind(pFontTexture->m_pFonts, pUnitData->szCharSelectFont);
			if (pFont)
			{
				pDescription->m_pFont = pFont;
			}
		}
		UILabelSetText( pDescription, StringTableGetStringByIndex( pUnitData->nAddtlDescripString ) );
	}
	pDescription = UIComponentGetByEnum( UICOMP_CHARACTER_RACE_DESCRIPTION );
	if ( pDescription )
	{
		UIComponentSetVisible( pDescription, TRUE );

		const UNIT_DATA *pUnitData = UnitGetData(pUnit);

		UILabelSetText( pDescription, StringTableGetStringByIndex( pUnitData->nAddtlRaceDescripString ) );
	}
	pDescription = UIComponentGetByEnum( UICOMP_CHARACTER_RACE_BONUS_DESCRIPTION );
	if ( pDescription )
	{
		UIComponentSetVisible( pDescription, TRUE );

		const UNIT_DATA *pUnitData = UnitGetData(pUnit);

		UILabelSetText( pDescription, StringTableGetStringByIndex( pUnitData->nAddtlRaceBonusDescripString ) );
	}
	if ( bStartSkill && pPlayerData->nPaperdollSkill != INVALID_ID )
	{
		UnitRegisterEventTimed(pUnit, sCharacterSelectStartSkill, &EVENT_DATA(pPlayerData->nPaperdollSkill, GameGetState(pGame)), 20);
	}

	UIRepaint();
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sCanDoDifficulty(
					  int nDifficulty )
{
	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return FALSE;
	UNIT *pUnit = GameGetControlUnit(pGame);
	if ( ! pUnit )
		return FALSE;
	return UnitGetStat( pUnit, STATS_DIFFICULTY_MAX ) >= nDifficulty;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSelectCharacterButton(
	int nButton,
	BOOL bCanSwitchToCreate,
	BOOL bForceImmediatePanelActivate = FALSE )
{
	ASSERT_RETURN( nButton >= 0 && nButton < UI_MAX_CHARACTER_BUTTONS );
	g_UI_StartgameParams.m_nCharacterButton = nButton;
	int nPlayerSlot = gnCharButtonToPlayerSlot[ nButton ];
	nPlayerSlot = MAX( nPlayerSlot, 0 );

	if (g_UI_StartgameParams.m_nPlayerSlot == nPlayerSlot)
	{
		return;
	}

	g_UI_StartgameParams.m_nPlayerSlot = nPlayerSlot;

	if ( gppCharSelectUnits[nPlayerSlot] )
	{
		g_UI_StartgameParams.m_bNewPlayer = FALSE;
		sCharacterSelectUnit( gppCharSelectUnits[nPlayerSlot] );
		UIComponentActivate(UICOMP_CHARACTER_CREATE, FALSE, 0, NULL, FALSE, FALSE, TRUE, bForceImmediatePanelActivate ); 
		UIComponentActivate(UICOMP_CHARACTER_SELECT_PANEL, TRUE, 0, NULL, FALSE, FALSE, TRUE, bForceImmediatePanelActivate );	
		UI_COMPONENT * pDescription = UIComponentGetByEnum( UICOMP_CHARACTER_CREATE_DESCRIPTION );
		if ( pDescription )
			UIComponentSetVisible( pDescription, FALSE );

		g_UI_StartgameParams.m_nDifficulty = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );

		if( AppIsTugboat() )
		{
			// go to the character select panel for the rest
			UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_PANEL);
			UIComponentSetVisible(pComponent, TRUE);
			pComponent->m_Position = pComponent->m_ActivePos;

			if (pComponent->m_pParent)
			{
				UI_COMPONENT *pStartButton = UIComponentFindChildByName(pComponent->m_pParent, "character select accept");
				if (pStartButton)
				{
					UIComponentSetActive( pStartButton, 
										 ( gppCharSelectUnits[nPlayerSlot] && PlayerIsHardcoreDead(gppCharSelectUnits[nPlayerSlot]) ) ?
										 FALSE : TRUE);
					UIComponentSetVisible( pStartButton, 
						( gppCharSelectUnits[nPlayerSlot] && PlayerIsHardcoreDead(gppCharSelectUnits[nPlayerSlot]) ) ?
										 FALSE : TRUE);
				}
			}
		} else {
			UI_COMPONENT * pNightmareComponent = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_NIGHTMARE_DIFFICULTY);
			ASSERT_RETURN( pNightmareComponent );
			
			int nDifficultyDefault = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
			int nDifficultyNightmare = GlobalIndexGet( GI_DIFFICULTY_NIGHTMARE );
			
			const WCHAR * szTooltipText = NULL;
			BOOL bCanDoDifficulty = sCanDoDifficulty( nDifficultyNightmare );
			// KCK: If we're in trial mode but could otherwise do this difficulty, then we want to display the button but
			// set it inactive.
			const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
			if ( bCanDoDifficulty && pBadges && pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
			{
				UIComponentActivate( pNightmareComponent, FALSE );
				szTooltipText = GlobalStringGet(GS_DIFFICULTY_NOT_AVAILABLE_WITH_TRIAL);
			}
			else
			{
				UIComponentActivate( pNightmareComponent, bCanDoDifficulty );
				szTooltipText = GlobalStringGet(GS_UI_CHAR_SELECT_NIGHTMARE);
			}
			UIComponentSetVisible( pNightmareComponent, bCanDoDifficulty );

			if (!pNightmareComponent->m_szTooltipText)
				pNightmareComponent->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
			PStrCopy(pNightmareComponent->m_szTooltipText, szTooltipText, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);

			UI_BUTTONEX * pButton = UICastToButton( pNightmareComponent );
			ASSERT_RETURN( pButton );
			UIButtonSetDown( pButton, bCanDoDifficulty );
			//UIButtonSetDown( pButton, bSelectNightmare );

			// KCK: Disable "Play" button if character mode is not allowed
			UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_PANEL);
			if (pComponent && pComponent->m_pParent)
			{
				UI_COMPONENT *pStartButton = UIComponentFindChildByName(pComponent->m_pParent, "character select accept");
				if (pStartButton)
				{
					BOOL bActive = TRUE;
					if (gppCharSelectUnits[nPlayerSlot] &&
						(PlayerIsElite(gppCharSelectUnits[nPlayerSlot]) ||
						 PlayerIsHardcore(gppCharSelectUnits[nPlayerSlot]) ||
						 PlayerIsLeague(gppCharSelectUnits[nPlayerSlot])) && 
						pBadges && pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
					{
						bActive = FALSE;
						szTooltipText = GlobalStringGet(GS_CHARACTER_NOT_AVAILABLE_WITH_TRIAL);
					}
					// KCK: Enable this if we want to prevent dead hardcore players from entering the game
//					else if (gppCharSelectUnits[nPlayerSlot] && PlayerIsHardcoreDead(gppCharSelectUnits[nPlayerSlot]))
//					{
//						bActive = FALSE;
//						szTooltipText = GlobalStringGet(GS_CHARACTER_PLAY);	 // KCK: Add new string if this is activated
//					}
					else
					{
						szTooltipText = GlobalStringGet(GS_CHARACTER_PLAY);
					}
					UIComponentSetActive( pStartButton, bActive);
					if (!pStartButton->m_szTooltipText)
						pStartButton->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
					PStrCopy(pStartButton->m_szTooltipText, szTooltipText, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
				}
			}


			g_UI_StartgameParams.m_nDifficulty = bCanDoDifficulty ? nDifficultyNightmare : nDifficultyDefault; // default to doing the hardest difficulty that they can do
		}
		sUpdateCharSelectButtons();
	} 
	else if ( bCanSwitchToCreate )
	{
		sSwitchToCreateUnit();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sSelectMinUsedPlayerSlot( BOOL bForceImmediatePanelActivate = FALSE )
{
	for ( int i = 0; i < gnNumCharButtons; i++ )
	{
		int nSlot = gnCharButtonToPlayerSlot[ i ];
		if ( nSlot != INVALID_ID && gppCharSelectUnits[ nSlot ] )
		{
			sSelectCharacterButton( i, FALSE, bForceImmediatePanelActivate );
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCharacterSelectMakePaperdollModel(
	UNIT *pUnit)
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETFALSE( pGame );
	ASSERT_RETFALSE( pUnit );

	VECTOR vDirection( 0, -1, 0 );
	if( AppIsTugboat() )
	{
		VectorZRotation( vDirection, DegreesToRadians( 60.0f ) );
	}
	UnitSetFaceDirection( pUnit, vDirection, TRUE );

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe != INVALID_ID )
	{
		c_WardrobeUpgradeLOD( nWardrobe );
	}
	int nModelId = c_UnitGetModelIdThirdPerson( pUnit );
	if ( nModelId != INVALID_ID )
		c_AppearanceUpdateAnimationGroup( pUnit, pGame, nModelId, UnitGetAnimationGroup( pUnit ) );
	c_UnitUpdateGfx( pUnit );

	c_CameraSetViewType( VIEW_CHARACTER_SCREEN, TRUE );
	if(AppIsHellgate())
	{
		c_UnitSetMode( pUnit, MODE_IDLE_PAPERDOLL );
	}
	else
	{
		c_UnitSetMode( pUnit, MODE_IDLE_CHARSELECT );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRandomizePaperDoll(
	UNIT *pUnit)
{	
	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETFALSE( nWardrobe != INVALID_ID );
	
	c_WardrobeRandomize( nWardrobe );

	ASSERT_RETFALSE( pUnit->pAppearanceShape );

	sCharacterRandomizeHeight( pUnit );
	//c_WardrobeUpdate(); // we need to have it update so that we can add attachments
	//c_WardrobeUpdateAttachments ( UnitGetGame( pUnit ), pWardrobe, c_UnitGetModelId(pUnit), TRUE, FALSE );

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sFreeCharacterCreateOrSelectUnit(
									  GAME * pGame,
									  UNIT * pUnit )
{
	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe != INVALID_ID )
	{
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			UNIT * pWeapon = WardrobeGetWeapon( pGame, nWardrobe, i );
			if ( pWeapon )
				UnitFree( pWeapon );
		}
	}
	UnitFree( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCharacterCreateMakePaperdollModel(
	int nCharacterClass,
	int nRace,
	GENDER eGender)
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETFALSE( pGame );

	{
		UNIT * pUnit = gpppCharCreateUnits[ nCharacterClass ][ nRace ][ eGender ];
		if ( pUnit )
		{
			if (!AppIsTugboat())
			{
				if ( UnitGetById( pGame, pUnit->unitid ) )
				{
					return TRUE;
				} else
				{
					DEBUG_TRACE("Char Create Paperdoll Unit freed but still in array!\n");
					gpppCharCreateUnits[ nCharacterClass ][nRace ][ eGender ] = NULL;
				}
			}
			else
			{
				sFreeCharacterCreateOrSelectUnit( pGame, pUnit );
				gpppCharCreateUnits[ nCharacterClass ][nRace ][ eGender ] = NULL;
			}
		}
	}

	int nPlayerClass = CharacterClassGetPlayerClass( nCharacterClass, nRace, eGender );
	ASSERT_RETFALSE( nPlayerClass != INVALID_ID );

	const UNIT_DATA * pUnitData = PlayerGetData( pGame, nPlayerClass );
	if ( ! pUnitData )
	{
		gpppCharCreateUnits[ nCharacterClass ][ nRace ][ eGender ] = NULL;
		return FALSE;
	}

	VECTOR vDirection( 0, -1, 0 );
	PLAYER_SPEC tSpec;
	tSpec.nPlayerClass = nPlayerClass;
	tSpec.vDirection = vDirection;
	if( AppIsTugboat() )
	{
		tSpec.vPosition = VECTOR( -126.75f, -221.0f, 7.7623f );
	}
	UNIT * pUnit = PlayerInit( pGame, tSpec );
	
	ASSERT_RETFALSE( pUnit );
	sRandomizePaperDoll( pUnit );	// this is needed because the unit creation will ignore our wardrobe init

	gpppCharCreateUnits[ nCharacterClass ][ nRace ][ eGender ] = pUnit;

	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETFALSE( nWardrobe != INVALID_ID );

	c_WardrobeSetColorSetIndex( nWardrobe, pUnitData->nPaperdollColorset, UnitGetType( pUnit ), TRUE );

	WardrobeSetMask( AppGetCltGame(), nWardrobe, WARDROBE_FLAG_FORCE_USE_BODY_ONLY );
	c_WardrobeForceLayerOntoUnit( pUnit, pUnitData->nCharacterScreenWardrobeLayer );
	c_WardrobeMakeClientOnly( pUnit);
	c_WardrobeUpgradeLOD( nWardrobe );

	WARDROBE_WEAPON* pWeapons = c_WardrobeGetWeapons( nWardrobe );
	ASSERT_RETFALSE( pWeapons );
	for ( int i = 0; i < MAX_PAPERDOLL_WEAPONS; i++ )
	{
		if ( pUnitData->pnPaperdollWeapons[ i ] == INVALID_ID )
			continue;

		ITEM_SPEC tItemSpec;
		tItemSpec.nItemClass = pUnitData->pnPaperdollWeapons[ i ];
		tItemSpec.vDirection = cgvYAxis;
		tItemSpec.eLoadType = UDLT_TOWN_OTHER;
		UNIT * pWeaponNew = ItemInit( pGame, tItemSpec );

		if ( pWeaponNew )
		{
			WARDROBE_WEAPON* pWardrobeWeapon = &pWeapons[ i ];
			pWardrobeWeapon->idWeaponReal = UnitGetId( pWeaponNew );
			c_WardrobeUpdateAttachments( pUnit, nWardrobe, c_UnitGetModelIdThirdPerson( pUnit ), TRUE, FALSE );
		}
	}

	int nModelId = c_UnitGetModelIdThirdPerson( pUnit );
	if ( nModelId != INVALID_ID )
		c_AppearanceUpdateAnimationGroup( pUnit, pGame, nModelId, UnitGetAnimationGroup( pUnit ) );

	sCharacterRandomizeHeight( pUnit );

	if ( pUnitData->nPaperdollSkill != INVALID_ID )
		c_SkillLoad( pGame, pUnitData->nPaperdollSkill );

	if ( pUnitData->nCharacterScreenState != INVALID_ID )
	{
		c_StateLoad( pGame, pUnitData->nCharacterScreenState );
		c_StateSet( pUnit, pUnit, pUnitData->nCharacterScreenState, 0, 0 );
	}

	gbCharCreateUnitsCreated = FALSE;

	c_UnitSetNoDraw(pUnit, TRUE, FALSE);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreateCharCreateUnitsIfNecessary()
{
	int nNumCharacterClasses = CharacterClassNumClasses();
	int nNumRaces = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYER_RACE);
	for (int nCharacterClass = 0; nCharacterClass < nNumCharacterClasses; ++nCharacterClass)
	{
		for (int i = 0; i < GENDER_NUM_CHARACTER_GENDERS; ++i)
		{
			GENDER eGender = (GENDER)i;
			for( int nRace = 0; nRace < nNumRaces; nRace++ )
			{
				if (   CharacterClassIsEnabled( nCharacterClass, nRace, eGender )
					&& ! gpppCharCreateUnits[ nCharacterClass ][nRace ][ eGender ] )
				{
					sCharacterCreateMakePaperdollModel( nCharacterClass, nRace, eGender );
				}
			}
		}
	}
}

static void sSwitchToCreateUnit( BOOL bCreateAllPaperdolls /*= FALSE*/, BOOL bForceImmediatePanelActivate /*= FALSE*/  )
{
	g_UI_StartgameParams.m_bNewPlayer = TRUE;

	UNIT *pCreateUnit = sGetCharCreateUnit();

#ifdef USE_DELAYED_CHARACTER_CREATION
	if ( !pCreateUnit || bCreateAllPaperdolls )
	{
		sCreateCharCreateUnitsIfNecessary();
		pCreateUnit = sGetCharCreateUnit();
	}
#endif

	ASSERT_RETURN( pCreateUnit );
	sCharacterSelectUnit(pCreateUnit, TRUE);

	// AE 2008.01.23
	// For bug 51702 (http://fsshome.com/show_bug.cgi?id=51702) there is a timing
	// issue where delayed component activation causes the character create panel 
	// to activate improperly after the select panel has been activated. See the 
	// bug for further details on how to reproduce and comment out the line below.
	// bForceImmediatePanelActivate = FALSE;

	UIComponentActivate(UICOMP_CHARACTER_SELECT_PANEL, FALSE, 0, NULL, FALSE, FALSE, TRUE, bForceImmediatePanelActivate );	
	UIComponentActivate(UICOMP_CHARACTER_CREATE, TRUE, 0, NULL, FALSE, FALSE, TRUE, bForceImmediatePanelActivate );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateRotatePanelOnMouseMove(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PANELEX *panel = UICastToPanel(component);

	if (!panel->m_bMouseDown)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x, y;
	UIGetCursorPosition(&x, &y);

	float fDelta = panel->m_posMouseDown.m_fX - x;

	fDelta = (fDelta / component->m_fWidth) * 2880.0f;

	panel->m_posMouseDown.m_fX = x;		// reset, since we're doing a running change not maintaining a rotation position

	if (fDelta != 0.0f)
	{
		PlayerMouseRotate(AppGetCltGame(), fDelta);
	}

	return UIMSG_RET_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCharacterSelectRotate(
									   GAME * pGame, 
									   UNIT * pUnit, 
									   const EVENT_DATA & event_data)
{

	if ( !pUnit )
		return FALSE;

	UI_COMPONENT* component = UIComponentGetById( (int)event_data.m_Data2 );
	int nDelta = event_data.m_Data1 ? -30 : 30;
	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETFALSE( pButton );
	BOOL bIsDown = UIButtonGetDown(pButton);
	if( bIsDown )
	{
		PlayerMouseRotate(AppGetCltGame(), (float)nDelta);
		EVENT_DATA tEventData(event_data.m_Data1, component->m_idComponent );
		UnitRegisterEventTimed(pUnit, sCharacterSelectRotate, &tEventData, 1);

	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateRotatePanelRotate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	

	int nDelta = component->m_dwParam ? -60 : 60 ;

	UNIT * pUnit = GameGetControlUnit(AppGetCltGame());
	if ( !pUnit )
		return UIMSG_RET_NOT_HANDLED;
	UI_BUTTONEX* pButton = UICastToButton(component);
	if( pButton )
	{
		UIButtonSetDown(pButton, TRUE );
	}
	
	EVENT_DATA tEventData(component->m_dwParam, component->m_idComponent );
	UnitRegisterEventTimed(pUnit, sCharacterSelectRotate, &tEventData, 1);

	PlayerMouseRotate(AppGetCltGame(), (float)nDelta);


	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------

static void sClearCharSelectUnits()
{
	for ( int i = 0; i < MAX_NAME_LIST; i++ )
	{
		if ( gppCharSelectUnits[ i ] )
		{
			UnitFree( gppCharSelectUnits[ i ] );
			gppCharSelectUnits[ i ] = NULL;
		}
	}
}

//----------------------------------------------------------------------------
static void sClearNameList()
{
	gnCharSelectScrollPos = 0;
	sClearCharSelectUnits();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICharacterCreateInit(void)
{
	if (!AppIsTugboat())
	{
		sClearNameList();
		structclear(gpppCharCreateUnits);
	}
	else
	{
		int nLevelId = c_LevelCreatePaperdollLevel( 0 );

#if !ISVERSION(SERVER_VERSION)
		AppSetCurrentLevel( nLevelId );
		c_QuestsFreeClientLocalValues( NULL ); //this is probably the wrong place for this
#endif
		UNIT* pUnit = gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender];
		if (pUnit)
		{
			GAME * pGame = AppGetCltGame();

			sFreeCharacterCreateOrSelectUnit( pGame, pUnit );
		}

		structclear(gpppCharCreateUnits);
		g_UI_StartgameParams.m_bNewPlayer = TRUE;

		UICharacterCreateReset();
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUpdateTutorialButton()
{
	if ( ! AppIsHellgate() )
		return;

	UI_COMPONENT * pComponentParent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE);
	if ( pComponentParent )
	{
		UI_COMPONENT * pTutorialButton = UIComponentFindChildByName( pComponentParent, "character create disable tutorial tips");
		ASSERT_RETURN( pTutorialButton );

		UI_BUTTONEX * pButton = UICastToButton( pTutorialButton );
		ASSERT_RETURN( pButton );

		UIButtonSetDown( pButton, g_UI_StartgameParams.dwNewPlayerFlags & NPF_DISABLE_TUTORIAL);
	}
}


int SinglePlayerGetNumSaves()
{
	/*
	int ii;
	for(ii = 0; ii < MAX_NAME_LIST; ii++)
		if(!gppCharSelectUnits[ii])
			break;
	return ii;

	*/
	ULARGE_INTEGER puNameTime[ MAX_NAME_LIST ];

	const OS_PATH_CHAR * szWidePath = FilePath_GetSystemPath(FILE_PATH_SAVE);

	int nPlayerSlotCurr = 0;
	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, MAX_PATH, OS_PATH_TEXT("%s*.hg1"), szWidePath);
	OS_PATH_FUNC(WIN32_FIND_DATA) data;
	FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)( filename, &data );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		// grab em all
		do
		{
			WCHAR cFileName[_countof(gpszCharSelectNames[0])];
			PStrCvt(cFileName, data.cFileName, _countof(cFileName));
			PStrRemoveExtension(gpszCharSelectNames[ nPlayerSlotCurr ], _countof(gpszCharSelectNames[ nPlayerSlotCurr ]), cFileName);
			puNameTime[ nPlayerSlotCurr ].HighPart = data.ftLastWriteTime.dwHighDateTime;
			puNameTime[ nPlayerSlotCurr ].LowPart = data.ftLastWriteTime.dwLowDateTime;
			nPlayerSlotCurr++;
		} while ( ( nPlayerSlotCurr < MAX_NAME_LIST ) && ( OS_PATH_FUNC(FindNextFile)( handle, &data ) ) );
	}
	return nPlayerSlotCurr;


}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanDoHardcoreMode()
{
#ifndef DEBUG
	if(AppIsSinglePlayer() && AppIsHellgate())
		return FALSE;
#endif
	const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
	if ( ! pBadges || ! pBadges->HasBadge(ACCT_ACCOMPLISHMENT_CAN_PLAY_HARDCORE_MODE) || !BadgeCollectionHasSubscriberBadge(*pBadges) || pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanDoEliteMode()
{
//#ifndef DEBUG
	if(AppIsSinglePlayer() && AppIsHellgate())
		//return TRUE;
		return (BOOL)SinglePlayerGetNumSaves();
//#endif
	const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
	if ( ! pBadges || ! pBadges->HasBadge(ACCT_ACCOMPLISHMENT_CAN_PLAY_ELITE_MODE) || pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanDoPVPOnlyMode()
{
	//#ifndef DEBUG
	if(AppIsHellgate())
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanDoLeagueMode()
{
#ifndef DEBUG
	if(AppIsSinglePlayer() && AppIsHellgate())
		return FALSE;
#endif
	const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
	if ( ! pBadges || ! pBadges->HasBadge(ACCT_ACCOMPLISHMENT_CAN_PLAY_ELITE_MODE) || pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
		return FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICharacterCreateReset(void)
{
	int nSanityCount = 0;
	while (!CharacterClassIsEnabled(g_UI_StartgameParams.m_nCharacterClass, g_UI_StartgameParams.m_nCharacterRace, g_UI_StartgameParams.m_eGender))
	{
		// Check the other gender
		GENDER eGenderOther = g_UI_StartgameParams.m_eGender == GENDER_MALE ? GENDER_FEMALE : GENDER_MALE;

		if (CharacterClassIsEnabled(g_UI_StartgameParams.m_nCharacterClass, g_UI_StartgameParams.m_nCharacterRace, eGenderOther ))
		{
			g_UI_StartgameParams.m_eGender = eGenderOther;
			break;
		}

		g_UI_StartgameParams.m_nCharacterClass++;
		if (g_UI_StartgameParams.m_nCharacterClass >= CharacterClassNumClasses())
		{
			g_UI_StartgameParams.m_nCharacterClass = 0;
		}
		nSanityCount++;

		ASSERTX(nSanityCount < CharacterClassNumClasses(), "No enabled character classes found");
	}

	// enable rendering of the background for the character creation screen
	e_SetUIOnlyRender( FALSE );

#ifndef USE_DELAYED_CHARACTER_CREATION
	int nNumCharacterClasses = CharacterClassNumClasses();
	int nNumRaces = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYER_RACE);
	for (int nCharacterClass = 0; nCharacterClass < nNumCharacterClasses; ++nCharacterClass)
	{
		for (int i = 0; i < GENDER_NUM_CHARACTER_GENDERS; ++i)
		{
			GENDER eGender = (GENDER)i;
			for( int nRace = 0; nRace < nNumRaces; nRace++ )
			{
				if (CharacterClassIsEnabled( nCharacterClass, nRace, eGender ))
				{
					sCharacterCreateMakePaperdollModel( nCharacterClass, nRace, eGender );
				}
			}
		}
	}
#else
	if ( ! gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender] )
		sCharacterCreateMakePaperdollModel( g_UI_StartgameParams.m_nCharacterClass, g_UI_StartgameParams.m_nCharacterRace, g_UI_StartgameParams.m_eGender );
#endif

	UNIT *pUnit = gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender];
	if (pUnit)
	{

		g_UI_StartgameParams.m_idCharCreateUnit = UnitGetId(pUnit);
	}
	else
	{
		g_UI_StartgameParams.m_idCharCreateUnit = INVALID_ID;
	}

	UI_COMPONENT * pComponentParent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE);
	if ( pComponentParent )
	{
		char* btns[4] = { "hardcore btn", "league btn", "elite btn", "pvponly btn"};
		BOOL (*testFunc[4])(void) = {sCanDoHardcoreMode, sCanDoEliteMode, sCanDoEliteMode, sCanDoPVPOnlyMode};
		for(int ii = 0; ii < 4; ii++)
		{
			UI_COMPONENT * pComponent = UIComponentFindChildByName(pComponentParent, btns[ii]);
			if(AppIsSinglePlayer() && AppIsHellgate() && testFunc[ii] != sCanDoEliteMode)
			{
				UIComponentSetVisible(pComponent, FALSE);
			}
			{
				if (pComponent)
				{
					UI_BUTTONEX* pButton = UICastToButton(pComponent);
					if ( pButton )
						UIButtonSetDown( pButton, FALSE );
					BOOL bShow = testFunc[ii]();
					UIComponentSetActive( pComponent, bShow);
				}
			}
		}
	}

	sUpdateTutorialButton();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreatePaperDollOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	{
		UI_RECT tScreenRect;
		tScreenRect.m_fX1 = 0; 
		tScreenRect.m_fY1 = 0; 
		tScreenRect.m_fX2 = component->m_fWidth;
		tScreenRect.m_fY2 = component->m_fHeight;
		UI_PANELEX *panel = UICastToPanel(component);
		REF(panel);

		UIComponentRemoveAllElements(component);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateAccountStatus()
{

	if( AppIsTugboat() )
		return;	//we don't have a UI for this yet

	UI_COMPONENT * pComponent = UIComponentGetByEnum( UICOMP_CHARACTER_ACCOUNT_STATUS );
	ASSERT_RETURN( pComponent );

	UI_LABELEX * pLabel = UICastToLabel( pComponent );
	ASSERT_RETURN(pLabel);

	UIUpdateAccountStatus(pLabel);
}

void UIUpdateAccountStatus(UI_LABELEX * pLabel)
{
	ASSERT_RETURN( pLabel );

	ESubscriptionType eSubscriptionType = c_BillingGetSubscriptionType();
	GLOBAL_STRING eString = (GLOBAL_STRING)INVALID_ID;
	switch ( eSubscriptionType )
	{
	case ESubscriptionType_Expired:
		eString = GS_ACCOUNT_SUBSCRIPTION_EXPIRED;
		break;
	case ESubscriptionType_Standard:
		eString = GS_ACCOUNT_SUBSCRIBER;
		break;
	case ESubscriptionType_Lifetime:
		eString = GS_ACCOUNT_LIFETIME;
		break;
	}

	const WCHAR * pszString = eString == INVALID_ID ? NULL : GlobalStringGet( eString );

	UILabelSetText( pLabel, pszString );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectCancelOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg != UIMSG_KEYDOWN && !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYDOWN && wParam != VK_ESCAPE)
		return UIMSG_RET_NOT_HANDLED;

	UIComponentActivate(UICOMP_CHARACTER_SELECT, FALSE);

	AppSetType(gApp.eStartupAppType);

	if (!AppIsTugboat())
	{
		if (AppIsDemo())
		{
			UISetCurrentMenu(UIComponentGetByEnum(UICOMP_STARTGAME_DEMO_MENU));
		}
		//else if (AppCommonMultiplayerOnly())
		//{
		//	UISetCurrentMenu(UIComponentGetByEnum(UICOMP_LOGIN_SCREEN));
		//}
		else 
		{
			UISetCurrentMenu(UIGetMainMenu());
		}
	}
	if (AppGetState() == APP_STATE_CONNECTING)
	{
		AppSwitchState(APP_STATE_RESTART);
	} else {
		AppSwitchState(APP_STATE_RESTART);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static 
int sGetMaxPossiblePlayerSlots()
{
	int nMax = MAX_CHARACTERS_PER_ACCOUNT;
	if ( AppIsHellgate() )
	{
		if ( AppGetType() == APP_TYPE_CLOSED_CLIENT )
		{
			// we're no longer restricting this based on subscription status

			//const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
			//if ( ! pBadges || ! pBadges->HasBadge( ACCT_TITLE_SUBSCRIBER ) )
			//	nMax = MAX_CHARACTERS_PER_NON_SUBSCRIBER;
		} else {
			nMax = MAX_CHARACTERS_PER_SINGLE_PLAYER;
		}
	} 

	return nMax;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UICharacterSelectLoadExistingCharacter()
{
	int MaxPossiblePlayerSlots = sGetMaxPossiblePlayerSlots();
	if ( g_UI_StartgameParams.m_nPlayerSlot >= MaxPossiblePlayerSlots )
	{
	
		// setup max slots as string
		const int MAX_NUMBER = 32;
		WCHAR uszNumber[ MAX_NUMBER ];
		PStrPrintf( uszNumber, MAX_NUMBER, L"%d", MaxPossiblePlayerSlots );

		// setup message with max player slots available		
		const int MAX_STRING = 1024;
		WCHAR uszMessage[ MAX_STRING ];		
		PStrCopy( uszMessage, GlobalStringGet( GS_SUBSCRIBER_ONLY_CHARACTER ), MAX_STRING );
		const WCHAR *puszToken = StringReplacementTokensGet( SR_NUMBER );
		PStrReplaceToken( uszMessage, MAX_STRING, puszToken, uszNumber );
		
		// display dialog
		UIShowGenericDialog( GlobalStringGet( GS_SUBSCRIBER_ONLY_CHARACTER_HEADER ), uszMessage );
						
		return TRUE;
		
	}

	UNIT *pSelectedUnit = GameGetControlUnit(AppGetCltGame());
	ASSERTX_RETVAL(pSelectedUnit, FALSE, "No character unit selected");
	if ( pSelectedUnit && PlayerIsHardcore( pSelectedUnit ) && ! sCanDoHardcoreMode() )
	{
		UIShowGenericDialog( GlobalStringGet( GS_HARDCORE_REQUIRES_SUBSCRIPTION_HEADER ), GlobalStringGet( GS_HARDCORE_REQUIRES_SUBSCRIPTION ) );
		return TRUE;
	}

	if ( pSelectedUnit && PlayerIsElite( pSelectedUnit ) && ! sCanDoEliteMode() )
	{
		UIShowGenericDialog( GlobalStringGet( GS_ELITE_REQUIRES_SUBSCRIPTION_HEADER ), GlobalStringGet( GS_ELITE_REQUIRES_SUBSCRIPTION ) );
		return TRUE;
	}

	if( AppIsTugboat() && pSelectedUnit && PlayerIsHardcoreDead( pSelectedUnit ) )
	{
		// TO DO - add a dialog or something?
		return TRUE;
	}
	UIComponentActivate(UICOMP_CHARACTER_SELECT, FALSE);

	g_UI_StartgameParams.m_bNewPlayer = FALSE;
	ASSERTX_RETVAL(pSelectedUnit->szName && pSelectedUnit->szName[0], FALSE, "Selected unit has no name");

	AppSetTypeByStartData(g_UI_StartgameParams);

	PStrCopy(g_UI_StartgameParams.m_szPlayerName, pSelectedUnit->szName, 256);

	CharacterSelectSetCharName( g_UI_StartgameParams.m_szPlayerName, 
		g_UI_StartgameParams.m_nPlayerSlot,
		g_UI_StartgameParams.m_nDifficulty,
		g_UI_StartgameParams.m_bNewPlayer );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectAcceptOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg != UIMSG_KEYDOWN && !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYDOWN && wParam != VK_RETURN)
		return UIMSG_RET_NOT_HANDLED;

	UI_COMPONENT *pScreen = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT);
	ASSERT_RETVAL(pScreen, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pMainPanel = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_PANEL);
	ASSERT_RETVAL(pMainPanel, UIMSG_RET_NOT_HANDLED);

	if (g_UI_StartgameParams.m_bNewPlayer)
	{
		UI_COMPONENT *pNameEditComp = UIComponentGetByEnum(UICOMP_EDITCTRL_CREATE_CHARNAME);
		ASSERT_RETVAL(pNameEditComp, UIMSG_RET_NOT_HANDLED);
		UI_LABELEX *pNameEdit = UICastToLabel(pNameEditComp);
		ASSERT_RETVAL(pNameEdit, UIMSG_RET_NOT_HANDLED);
		if (!pNameEdit->m_Line.HasText())
		{
			const WCHAR *puszTitle = GlobalStringGet( GS_ENTER_CHARACTER_NAME_TITLE );
			const WCHAR *puszText = GlobalStringGet( GS_ENTER_CHARACTER_NAME );

			UIShowGenericDialog(puszTitle, puszText,0,0,0,GDF_OVERRIDE_EVENTS);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		if (PlayerNameExists(pNameEdit->m_Line.GetText()))
		{
			const WCHAR *puszTitle = GlobalStringGet( GS_NAME_TAKEN_TITLE );
			const WCHAR *puszText = GlobalStringGet( GS_NAME_TAKEN );
			UIShowGenericDialog( puszTitle, puszText,0,0,0,GDF_OVERRIDE_EVENTS );
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		PStrCopy(g_UI_StartgameParams.m_szPlayerName, pNameEdit->m_Line.GetText(), 256);		

		//UIComponentSetVisible(pScreen, FALSE, TRUE);
		UIComponentActivate(pScreen, FALSE);

		CharCreateResetUnitLists();
		if (g_UI_StartgameCallback)
			g_UI_StartgameCallback(TRUE, g_UI_StartgameParams);		
	}
	else
	{
		ASSERT_RETVAL(UICharacterSelectLoadExistingCharacter(),
			UIMSG_RET_HANDLED_END_PROCESS);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static 
int sGetLastUsedPlayerSlot()
{
	int nMax = -1;
	for ( int i = 0; i < MAX_NAME_LIST; i++ )
	{
		if ( gppCharSelectUnits[ i ] )
		{
			nMax = i;
		}
	}
	return nMax;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectCreateOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	UI_COMPONENT * pCharacterPanel = UIComponentFindParentByName( component, "character panel", TRUE ); // if we can't find it, then we probably have no character right now
	int nCharacterButton = (int)pCharacterPanel->m_dwParam;
	if ( nCharacterButton >= 0 && nCharacterButton <= gnNumCharButtons )
	{// figure out which available player slot to use for this button
		ASSERT_RETVAL( gnCharButtonToPlayerSlot[ nCharacterButton ] == INVALID_ID, UIMSG_RET_NOT_HANDLED);
		//int nNumSlotsToSkip = 0;
		//for ( int i = 0; i < nCharacterButton; i++ )
		//{
		//	if ( gnCharButtonToPlayerSlot[ i ] == INVALID_ID ) // look at previous buttons that are for creation and skip that many slots
		//		nNumSlotsToSkip++;
		//}
		int nPlayerSlot = INVALID_ID;
		int nMaxSlot = sGetLastUsedPlayerSlot() + 1;
		for ( int i = 0; i <= nMaxSlot; i++ ) 
		{
			if ( !gppCharSelectUnits[ i ] )
			{
				//if ( nNumSlotsToSkip )
				//	nNumSlotsToSkip--;
				//else
				{
					nPlayerSlot = i;
					break;
				}
			}
		}
		ASSERT_RETVAL( nPlayerSlot != INVALID_ID, UIMSG_RET_NOT_HANDLED);
		g_UI_StartgameParams.m_nPlayerSlot = nPlayerSlot;
		g_UI_StartgameParams.m_nCharacterButton = nCharacterButton;

	} else {
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	sSwitchToCreateUnit( TRUE );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateCancelOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg != UIMSG_KEYDOWN && !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYDOWN && wParam != VK_ESCAPE)
		return UIMSG_RET_NOT_HANDLED;

	GAME * pGame = AppGetCltGame();
	if(pGame)
	{
		UNIT* pControlUnit = GameGetCameraUnit( pGame );
		sCharacterCreateSelectUnitResetUnit(pGame, pControlUnit);

		GameSetControlUnit(pGame, NULL);
	}

	if ( sGetLastUsedPlayerSlot() < 0 )
	{
		return UICharacterSelectCancelOnClick(component, msg, wParam, lParam);
	}

	sSelectMinUsedPlayerSlot();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateAcceptOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg != UIMSG_KEYDOWN && !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYDOWN && wParam != VK_RETURN)
		return UIMSG_RET_NOT_HANDLED;

	UI_COMPONENT *pNameEditComp = UIComponentGetByEnum(UICOMP_EDITCTRL_CREATE_CHARNAME);
	ASSERT_RETVAL(pNameEditComp, UIMSG_RET_NOT_HANDLED);
	UI_LABELEX *pNameEdit = UICastToLabel(pNameEditComp);
	ASSERT_RETVAL(pNameEdit, UIMSG_RET_NOT_HANDLED);
	if (!pNameEdit->m_Line.HasText())
	{
		const WCHAR *puszTitle = GlobalStringGet( GS_ENTER_CHARACTER_NAME_TITLE );
		const WCHAR *puszText = GlobalStringGet( GS_ENTER_CHARACTER_NAME );

		InputKeyIgnore(1000);
		UIShowGenericDialog(puszTitle, puszText,0,0,0,GDF_OVERRIDE_EVENTS);
		goto fail;
	}

	// trim trailing spaces
	PStrTrimTrailingSpaces(pNameEdit->m_Line.EditText(), 0);

	if (!g_UI_StartgameParams.m_bSinglePlayer && !ValidateName(pNameEdit->m_Line.EditText(), MAX_CHARACTER_NAME, NAME_TYPE_PLAYER))	
	{
		const WCHAR *puszTitle = GlobalStringGet( GS_CHARACTER_NAME_NOT_ALLOWED_TITLE );
		const WCHAR *puszText = GlobalStringGet( GS_CHARACTER_NAME_NOT_ALLOWED );
		UIShowGenericDialog(puszTitle, puszText,0,0,0,GDF_OVERRIDE_EVENTS);

		goto fail;
	}

	if (PlayerNameExists(pNameEdit->m_Line.GetText()))
	{
		const WCHAR *puszTitle = GlobalStringGet( GS_NAME_TAKEN_TITLE );
		const WCHAR *puszText = GlobalStringGet( GS_NAME_TAKEN );
		UIShowGenericDialog( puszTitle, puszText,0,0,0,GDF_OVERRIDE_EVENTS );	

		goto fail;
	}

	AppSetTypeByStartData(g_UI_StartgameParams);

	if (g_UI_StartgameParams.m_bNewPlayer)
	{
		GAME * game = AppGetCltGame();
		ASSERT_RETVAL(game, UIMSG_RET_HANDLED_END_PROCESS);
		UNIT * unit = GameGetControlUnit(game);
		ASSERT_RETVAL(unit, UIMSG_RET_HANDLED_END_PROCESS);
		int nWardrobe = UnitGetWardrobe(unit);
		ASSERT_RETVAL(nWardrobe != INVALID_ID, UIMSG_RET_HANDLED_END_PROCESS);
		extern WARDROBE_BODY gWardrobeBody;
		extern APPEARANCE_SHAPE gAppearanceShape;
		WARDROBE_BODY* pBody = c_WardrobeGetBody( nWardrobe );
		if ( pBody )
			MemoryCopy(&gWardrobeBody, sizeof(WARDROBE_BODY), pBody, sizeof(WARDROBE_BODY));
		MemoryCopy(&gAppearanceShape, sizeof(APPEARANCE_SHAPE), unit->pAppearanceShape, sizeof(APPEARANCE_SHAPE));
	}

	if( AppIsHellgate() )
	{
		UIShowGenericDialog(StringTableGetStringByKey("ui creating character"), StringTableGetStringByKey("ui please wait"), FALSE, NULL, NULL, GDF_NO_OK_BUTTON);
	}

	if ( pNameEdit->m_Line.HasText() )
		PStrCopy(g_UI_StartgameParams.m_szPlayerName, pNameEdit->m_Line.GetText(), 256);
	else
		g_UI_StartgameParams.m_szPlayerName[0] = 0;

	CharacterSelectSetCharName( g_UI_StartgameParams.m_szPlayerName, 
		g_UI_StartgameParams.m_nPlayerSlot,
		g_UI_StartgameParams.m_nDifficulty,
		g_UI_StartgameParams.m_bNewPlayer );

	return UIMSG_RET_HANDLED_END_PROCESS;

fail:
	int nSoundId = GlobalIndexGet(GI_SOUND_UI_CHAR_SELECT_INVALID);
	if (nSoundId != INVALID_LINK)
	{
		c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
	}

	return UIMSG_RET_NOT_HANDLED;	// technically it was, but this is an easy way to make the accept sound not play

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateRandomizeOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
		return UIMSG_RET_NOT_HANDLED;

	GAME * pGame = AppGetCltGame();
	UNIT * pUnit = pGame ? UnitGetById( pGame, g_UI_StartgameParams.m_idCharCreateUnit ) : NULL;
	if ( pUnit )
	{
		int nWardrobe = UnitGetWardrobe( pUnit );
		ASSERT_RETVAL( nWardrobe != INVALID_ID, UIMSG_RET_NOT_HANDLED );

		if ( c_WardrobeIsUpdated( nWardrobe ) )	
		{
			if (sRandomizePaperDoll( pUnit ) == FALSE)
			{
				ASSERT_RETVAL( 0, UIMSG_RET_NOT_HANDLED );
			}
		}
		else
			return UIMSG_RET_NOT_HANDLED;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterAdjustOnClick(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (g_UI_StartgameParams.m_idCharCreateUnit == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT * pUnit = sGetCharCreateUnit();
	ASSERT_RETVAL( pUnit, UIMSG_RET_NOT_HANDLED );
	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETVAL( nWardrobe != INVALID_ID, UIMSG_RET_NOT_HANDLED );

	BOOL bNext = (BOOL) component->m_dwParam2;
	if ( component->m_dwParam >= NUM_WARDROBE_BODY_SECTIONS )
	{
		int nColor = ((int)component->m_dwParam - NUM_WARDROBE_BODY_SECTIONS);
		c_WardrobeIncrementBodyColor( nWardrobe, nColor, bNext );
	} else {
		WARDROBE_BODY_SECTION eBodySection = (WARDROBE_BODY_SECTION) component->m_dwParam;
		if ( eBodySection > WARDROBE_BODY_SECTION_BASE && eBodySection < NUM_WARDROBE_BODY_SECTIONS )
		{
			BODY_SECTION_INCREMENT_RETURN eRetVal = c_WardrobeIncrementBodySection( nWardrobe, eBodySection, bNext );

			switch (eRetVal)
			{
			case BODY_SECTION_INCREMENT_OK:			trace("BODY_SECTION_INCREMENT_OK\n");			break;
			case BODY_SECTION_INCREMENT_LOOPED:		trace("BODY_SECTION_INCREMENT_LOOPED\n");		break;
			case BODY_SECTION_INCREMENT_NO_OPTIONS:	trace("BODY_SECTION_INCREMENT_NO_OPTIONS\n");	break;
			case BODY_SECTION_INCREMENT_ERROR:		trace("BODY_SECTION_INCREMENT_ERROR\n");		break;
			}
		}
	}
	c_WardrobeUpdate(); // we need to have it update so that we can add attachments
	c_WardrobeUpdateAttachments( pUnit, nWardrobe, c_UnitGetModelId(pUnit), TRUE, FALSE );


	return UIButtonOnButtonUp(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterAdjustSize(
									UI_COMPONENT* component,
									int msg,
									DWORD wParam,
									DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UNIT * pUnit = sGetCharCreateUnit();
	ASSERT_RETVAL( pUnit, UIMSG_RET_NOT_HANDLED );
	ASSERT_RETVAL( pUnit->pAppearanceShape, UIMSG_RET_NOT_HANDLED );

	UIScrollBarOnScroll(component, msg, wParam, lParam);

	float fHeightPercent = 0.5f;
	float fWeightPercent = 0.5f;
	{
		UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE_HEIGHT_BAR);
		if (pComponent)
		{
			fHeightPercent = pComponent->m_ScrollPos.m_fY / 255.0f;
		}
		pComponent = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE_WEIGHT_BAR);
		if (pComponent)
		{
			fWeightPercent = pComponent->m_ScrollPos.m_fY / 255.0f;
		}
	}
	UnitSetShape( pUnit, fHeightPercent, fWeightPercent );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterAdjustClassOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nNumClasses = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYERS);
	REF(nNumClasses);

	g_UI_StartgameParams.m_nCharacterClass += (int)component->m_dwParam;
	if (g_UI_StartgameParams.m_nCharacterClass < 0)
	{
		g_UI_StartgameParams.m_nCharacterClass = CharacterClassNumClasses() - 1;	
	}
	if (g_UI_StartgameParams.m_nCharacterClass >= CharacterClassNumClasses())
	{
		g_UI_StartgameParams.m_nCharacterClass = 0;
	}

	int nSanityCount = 0;
	while (!CharacterClassIsEnabled(g_UI_StartgameParams.m_nCharacterClass, g_UI_StartgameParams.m_nCharacterRace, g_UI_StartgameParams.m_eGender))
	{
		// Check the other gender
		GENDER eGenderOther = g_UI_StartgameParams.m_eGender == GENDER_MALE ? GENDER_FEMALE : GENDER_MALE;

		if (CharacterClassIsEnabled(g_UI_StartgameParams.m_nCharacterClass, g_UI_StartgameParams.m_nCharacterRace, eGenderOther))
		{
			g_UI_StartgameParams.m_eGender = eGenderOther;
			break;
		}

		g_UI_StartgameParams.m_nCharacterClass += (int)component->m_dwParam;
		if (g_UI_StartgameParams.m_nCharacterClass < 0)
		{
			g_UI_StartgameParams.m_nCharacterClass =  CharacterClassNumClasses() - 1;
		}
		if (g_UI_StartgameParams.m_nCharacterClass >= CharacterClassNumClasses())
		{
			g_UI_StartgameParams.m_nCharacterClass = 0;
		}

		nSanityCount++;

		ASSERTX(nSanityCount < CharacterClassNumClasses(), "No enabled character classes found");
	}

	if ( ! gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender] )
		sCreateCharCreateUnitsIfNecessary();

	sCharacterSelectUnit(gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender], TRUE);
	g_UI_StartgameParams.m_idCharCreateUnit = UnitGetId(gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender]);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterGemToggle(
								   UI_COMPONENT* component,
								   int msg,
								   DWORD wParam,
								   DWORD lParam)
{
	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	if ( !bIsDown || g_UI_StartgameParams.m_dwGemActivation )
		g_UI_StartgameParams.m_dwGemActivation = 0;
	else
	{
		RAND tRand;
		RandInit( tRand );
		g_UI_StartgameParams.m_dwGemActivation = RandGetNum( tRand, 0, 255 );
		if ( g_UI_StartgameParams.m_dwGemActivation > 254 ) // decrease the odds of getting 255
			g_UI_StartgameParams.m_dwGemActivation = RandGetNum( tRand, 0, 255 );
		if ( ! g_UI_StartgameParams.m_dwGemActivation )
			g_UI_StartgameParams.m_dwGemActivation = 1;
	}

	UNIT * pUnit = sGetCharCreateUnit();
	if ( pUnit )
	{
		StateClearAllOfType( pUnit, STATE_GEM );
		if ( g_UI_StartgameParams.m_dwGemActivation > 254 )
		{
			c_StateSet( pUnit, pUnit, STATE_GEM_ACTIVATED_HIGH, 0, 0, INVALID_ID );
		} 
		else if ( g_UI_StartgameParams.m_dwGemActivation == 0 ) 
		{
		} 
		else if ( g_UI_StartgameParams.m_dwGemActivation < 50 ) 
		{
			c_StateSet( pUnit, pUnit, STATE_GEM_ACTIVATED_LOW, 0, 0, INVALID_ID );
		}
		else 
		{
			c_StateSet( pUnit, pUnit, STATE_GEM_ACTIVATED, 0, 0, INVALID_ID );
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterTutorialToggle(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	//g_UI_StartgameParams.m_bSkipTutorialTips = bIsDown;
	if(bIsDown)
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_DISABLE_TUTORIAL;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_DISABLE_TUTORIAL;

	UI_COMPONENT * pChild = component->m_pFirstChild;
	if ( pChild && UIComponentIsLabel( pChild ) )
	{
		pChild->m_dwColor = (g_UI_StartgameParams.dwNewPlayerFlags & NPF_DISABLE_TUTORIAL) ? GFXCOLOR_WHITE : GFXCOLOR_DKGRAY;
		UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterHardcoreHover(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	float x;
	float y;

	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	const WCHAR * szTooltipText = NULL;
	if (UIComponentCheckBounds(component, x, y, &pos))
	{
		x -= pos.m_fX;
		y -= pos.m_fY;

		if ( sCanDoHardcoreMode() )
		{
			if ( g_UI_StartgameParams.dwNewPlayerFlags & NPF_HARDCORE)
				szTooltipText = GlobalStringGet(GS_HARDCORE_ENABLED);
			else
				szTooltipText = GlobalStringGet(GS_HARDCORE_DISABLED);
		} else {
			const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
			if ( pBadges && pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
				szTooltipText = GlobalStringGet(GS_MODE_NOT_AVAILABLE_WITH_TRIAL);
			else
				szTooltipText = GlobalStringGet(GS_HARDCORE_REQUIREMENTS);
		}

		if (!component->m_szTooltipText)
		{
			component->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
		}
		PStrCopy(component->m_szTooltipText, szTooltipText, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
	}

/*
	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	if ( pElement && szTooltipText )
	{
		UISetSimpleHoverText(
			pos.m_fX + pElement->m_Position.m_fX,
			pos.m_fY + pElement->m_Position.m_fY,
			pos.m_fX + pElement->m_Position.m_fX + pElement->m_fWidth,
			pos.m_fY + pElement->m_Position.m_fY + pElement->m_fHeight,
			szTooltipText);
		return UIMSG_RET_HANDLED_END_PROCESS;
	} 
*/
	//else 
	//{
	//UIClearHoverText(); /// is this going to kill hover text for everyone?  I don't know...

	// yes it is. You need to  clear your tooltip on mouseleave.

	//}
	return UIMSG_RET_NOT_HANDLED;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterLeagueHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	float x;
	float y;

	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	const WCHAR * szTooltipText = NULL;
	if (UIComponentCheckBounds(component, x, y, &pos))
	{
		x -= pos.m_fX;
		y -= pos.m_fY;

		if ( sCanDoLeagueMode() )
		{
			if ( g_UI_StartgameParams.dwNewPlayerFlags & NPF_LEAGUE )
				szTooltipText = GlobalStringGet(GS_LEAGUE_ENABLED);
			else
				szTooltipText = GlobalStringGet(GS_LEAGUE_DISABLED);
		} else {
			const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
			if ( pBadges && pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
				szTooltipText = GlobalStringGet(GS_MODE_NOT_AVAILABLE_WITH_TRIAL);
			else
				szTooltipText = GlobalStringGet(GS_LEAGUE_REQUIREMENTS);
		}

		if (!component->m_szTooltipText)
		{
			component->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
		}
		PStrCopy(component->m_szTooltipText, szTooltipText, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
	}

/*	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	if ( pElement && szTooltipText )
	{
		UISetSimpleHoverText(
			pos.m_fX + pElement->m_Position.m_fX,
			pos.m_fY + pElement->m_Position.m_fY,
			pos.m_fX + pElement->m_Position.m_fX + pElement->m_fWidth,
			pos.m_fY + pElement->m_Position.m_fY + pElement->m_fHeight,
			szTooltipText);
		return UIMSG_RET_HANDLED_END_PROCESS;
	} 
*/
	//else 
	//{
	//UIClearHoverText(); /// is this going to kill hover text for everyone?  I don't know...

	// yes it is. You need to  clear your tooltip on mouseleave.

	//}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterEliteHover(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	float x;
	float y;

	UIGetCursorPosition(&x, &y);

	UI_POSITION pos;
	const WCHAR * szTooltipText = NULL;
	if (UIComponentCheckBounds(component, x, y, &pos))
	{
		x -= pos.m_fX;
		y -= pos.m_fY;

		if ( sCanDoEliteMode() )
		{
			if ( g_UI_StartgameParams.dwNewPlayerFlags & NPF_ELITE)
				szTooltipText = GlobalStringGet(GS_ELITE_ENABLED);
			else
				szTooltipText = GlobalStringGet(GS_ELITE_DISABLED);
		} else {
			const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();  
			if ( pBadges && pBadges->HasBadge(ACCT_TITLE_TRIAL_USER) )
				szTooltipText = GlobalStringGet(GS_MODE_NOT_AVAILABLE_WITH_TRIAL);
			else
				szTooltipText = GlobalStringGet(GS_ELITE_REQUIREMENTS);
		}

		if (!component->m_szTooltipText)
		{
			component->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
		}
		PStrCopy(component->m_szTooltipText, szTooltipText, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
	}

/*	UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
	if ( pElement && szTooltipText )
	{
		UISetSimpleHoverText(
			pos.m_fX + pElement->m_Position.m_fX,
			pos.m_fY + pElement->m_Position.m_fY,
			pos.m_fX + pElement->m_Position.m_fX + pElement->m_fWidth,
			pos.m_fY + pElement->m_Position.m_fY + pElement->m_fHeight,
			szTooltipText);
		return UIMSG_RET_HANDLED_END_PROCESS;
	} 
*/
	//else 
	//{
	//UIClearHoverText(); /// is this going to kill hover text for everyone?  I don't know...

	// yes it is. You need to  clear your tooltip on mouseleave.

	//}
	return UIMSG_RET_NOT_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterHardcoreToggle(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	//g_UI_StartgameParams.m_bHardcore = sCanDoHardcoreMode() && bIsDown;
	//Want to use xor, but that might cause subtle bugs...right? -bmanegold
	if(bIsDown && sCanDoHardcoreMode())
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_HARDCORE;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_HARDCORE;

	if ( !sCanDoHardcoreMode() )
		UIComponentSetActive( component, FALSE );

	if( AppIsHellgate() )
	{
		UIClearHoverText();
//		UICharacterHardcoreHover( component, msg, wParam, lParam );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterPVPOnlyToggle(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	if(bIsDown )
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_PVPONLY;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_PVPONLY;

	return UIMSG_RET_HANDLED_END_PROCESS;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterLeagueToggle(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	if(bIsDown )
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_LEAGUE;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_LEAGUE;

	if ( !sCanDoLeagueMode() )
		UIComponentSetActive( component, FALSE );

	if( AppIsHellgate() )
	{
		UIClearHoverText();
//		UICharacterLeagueHover( component, msg, wParam, lParam );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterEliteToggle(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	//g_UI_StartgameParams.m_bElite = sCanDoEliteMode() && bIsDown;
	if(bIsDown && sCanDoEliteMode())
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_ELITE;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_ELITE;
	if ( !sCanDoEliteMode() )
		UIComponentSetActive( component, FALSE );

	if( AppIsHellgate() )
	{
		UIClearHoverText();
//		UICharacterEliteHover( component, msg, wParam, lParam );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterNightmareToggle(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETVAL( pButton, UIMSG_RET_HANDLED_END_PROCESS );
	BOOL bIsDown = UIButtonGetDown(pButton);

	int nDesiredDifficulty = 1;
	g_UI_StartgameParams.m_nDifficulty = (bIsDown && sCanDoDifficulty( nDesiredDifficulty )) ? nDesiredDifficulty : 0;
	if ( !sCanDoDifficulty( nDesiredDifficulty ) )
		UIComponentSetActive( component, FALSE );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterAdjustSexOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{


	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	GENDER eGenderOther = g_UI_StartgameParams.m_eGender == GENDER_MALE ? GENDER_FEMALE : GENDER_MALE;
	if (CharacterClassIsEnabled(g_UI_StartgameParams.m_nCharacterClass, g_UI_StartgameParams.m_nCharacterRace, eGenderOther))
	{
		g_UI_StartgameParams.m_eGender = eGenderOther;

		if ( ! gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender] )
			sCreateCharCreateUnitsIfNecessary();

		sCharacterSelectUnit(gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender], TRUE);
		g_UI_StartgameParams.m_idCharCreateUnit = UnitGetId(gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender]);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterAdjustRaceOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nNumRaces = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYER_RACE);

	g_UI_StartgameParams.m_nCharacterRace += ( component->m_dwParam ? 1 : -1 );
	if ( g_UI_StartgameParams.m_nCharacterRace < 0 )
	{
		g_UI_StartgameParams.m_nCharacterRace = nNumRaces - 1;	
	}
	if ( g_UI_StartgameParams.m_nCharacterRace >= nNumRaces )
	{
		g_UI_StartgameParams.m_nCharacterRace = 0;
	}

	if ( ! gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender] )
		sCreateCharCreateUnitsIfNecessary();

	sCharacterSelectUnit(gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender], TRUE);
	g_UI_StartgameParams.m_idCharCreateUnit = UnitGetId(gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender]);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnPaint(
									UI_COMPONENT* component,
									int msg,
									DWORD wParam,
									DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel(UIComponentFindChildByName(component, "label dialog text"));
	if (pLabel && pLabel->m_nNumPages > 0)
	{
		UI_COMPONENT *pChild = UIComponentFindChildByName(component, "dialog prev btn");
		if (pChild)
		{
			UIComponentSetActive(pChild, (pLabel->m_nPage > 0));
		}
		pChild = UIComponentFindChildByName(component, "dialog next btn");
		if (pChild)
		{
			UIComponentSetActive(pChild, (pLabel->m_nPage < pLabel->m_nNumPages-1));
		}
		UI_COMPONENT *pOkBtn = UIComponentFindChildByName(component, "button dialog ok");
		if (pOkBtn)
		{
			UNIT *pPlayer = UIGetControlUnit();
			ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

			BOOL bActivate = (pLabel->m_nPage == pLabel->m_nNumPages-1);
			bActivate |= UnitGetStat(pPlayer, STATS_LEVEL) > 2;
			//UIComponentSetActive(pOkBtn, bActivate);
		}
	}

	return UIComponentOnPaint(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIStopAllSkillRequests();
	c_PlayerClearAllMovement(AppGetCltGame());
	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// returns whether it's on the last page
static BOOL sConversationChangePage(
									UI_COMPONENT* component,
									int nDelta)
{
	UI_LABELEX *pLabel = UICastToLabel(UIComponentFindChildByName(component, "label dialog text"));
	if (pLabel && 
		pLabel->m_nPage + nDelta >= 0 && 
		pLabel->m_nPage + nDelta <= pLabel->m_nNumPages /*- 1*/)
	{

		// a click on the button will try to reveal all the text
		if (UILabelDoRevealAllText( pLabel ) == TRUE)
		{
			return FALSE;
		}

		UILabelOnAdvancePage(pLabel, nDelta);

		BOOL bVideo = FALSE;
		int nSpeaker = UIGetSpeakerNumFromText(UILabelGetText(pLabel), pLabel->m_nPage, bVideo);

		ASSERTX_RETFALSE(nSpeaker >= 0 && nSpeaker < 2, "Unexpected speaker found" );

		BOOL bOnVideo = (component == UIComponentGetByEnum(UICOMP_VIDEO_DIALOG));

		UI_CONVERSATION_DLG *pConvDlg = UICastToConversationDlg(component);
		// copy the speakers in case the components switch
		UNITID	idTalkingTo[MAX_CONVERSATION_SPEAKERS];				
		memcpy(idTalkingTo, pConvDlg->m_idTalkingTo, sizeof(UNITID) * MAX_CONVERSATION_SPEAKERS);

		if (bVideo != bOnVideo)
		{
			UIComponentHandleUIMessage(component, UIMSG_INACTIVATE, 0, 0);
			component = bVideo ? UIComponentGetByEnum(UICOMP_VIDEO_DIALOG) : UIComponentGetByEnum(UICOMP_NPC_CONVERSATION_DIALOG);
			UI_LABELEX *pOtherLabel = UICastToLabel(UIComponentFindChildByName(component, "label dialog text"));
			ASSERTX_RETFALSE(pOtherLabel, "Could not find video dialog text label.");
			pOtherLabel->m_nPage = pLabel->m_nPage;
			UIComponentHandleUIMessage(pOtherLabel, UIMSG_PAINT, 0, 0);		// need to re-paint the label with the new page number
			if (pOtherLabel->m_bSlowReveal)	// needs to go after the paint message so the text element has been updated
			{
				UILabelStartSlowReveal( pOtherLabel );
			}
			UIComponentHandleUIMessage(component, UIMSG_ACTIVATE, 0, 0);

			pConvDlg = UICastToConversationDlg(component);
			memcpy(pConvDlg->m_idTalkingTo, idTalkingTo, sizeof(UNITID) * MAX_CONVERSATION_SPEAKERS);
		}

		UNIT *pFocus = UIComponentGetFocusUnit(component);
		if (!pFocus	|| 
			UnitGetId(pFocus) != pConvDlg->m_idTalkingTo[nSpeaker])
		{
			UIComponentSetFocusUnit(component, pConvDlg->m_idTalkingTo[nSpeaker]);
		}

		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

		return FALSE;	// we just turned the page.  Not done yet
	}

	return pLabel && pLabel->m_nPage >= pLabel->m_nNumPages-1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnPageNext(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		if (msg == UIMSG_KEYUP && wParam != L' ' && wParam != VK_RETURN)
		{
			return UIMSG_RET_NOT_HANDLED;  
		}

		if (msg == UIMSG_KEYDOWN)
		{
			return UIMSG_RET_NOT_HANDLED;  
		}

		UI_COMPONENT * parent = component->m_pParent;
		sConversationChangePage(parent, 1);

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}
	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnPagePrev(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		UI_COMPONENT * parent = component->m_pParent;
		sConversationChangePage(parent, -1);

		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DIALOG_CALLBACK *sGetConversationCallback(
	UI_COMPONENT *pComponent,
	CONVERSATION_COMPLETE_TYPE eType)
{
	ASSERTX_RETNULL( pComponent, "Expected component" );
	if (eType == CCT_OK)
	{
		return &pComponent->m_tCallbackOK;
	}
	else if (eType == CCT_CANCEL)
	{
		return &pComponent->m_tCallbackCancel;
	}
	ASSERTX_RETNULL( 0, "Unknown conversatoin callback type" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDoConversationComplete(
							  UI_COMPONENT *pComponent,
							  CONVERSATION_COMPLETE_TYPE eType, 
							  BOOL bSendTalkDone /*= TRUE*/,
							  BOOL bCloseConversationUI /*= TRUE*/)
{
	ASSERTX_RETURN( pComponent, "Expected component" );

	// reveal all text if not all revealed when proceeding
	if (eType == CCT_OK )
	{
		UI_COMPONENT *pCompText = UIComponentFindChildByName(pComponent, "label dialog text");
		if (pCompText)
		{		
			if (UILabelDoRevealAllText( pCompText ) == TRUE)
			{
				return;
			}		
		}
	}

	// close the UI if desired
	if (bCloseConversationUI)
	{
		UIHideNPCDialog(eType);	
		if (AppIsHellgate())
		{
			UIHideNPCVideo();
		}
	}

	// search up our parent chain looking for a callback ... this seems problematic, what
	// if we had multiple frames setup with callbacks, we wouldn't know when to take
	// one and when to skip one
	UI_COMPONENT *pCompCurrent = pComponent;
	const DIALOG_CALLBACK *pCallback = sGetConversationCallback( pCompCurrent, eType );
	while (pCallback->pfnCallback == NULL)
	{
		pCompCurrent = pCompCurrent->m_pParent;
		if (pCompCurrent)
		{
			pCallback = sGetConversationCallback( pCompCurrent, eType );
		}
		else
		{
			break;
		}
	}

	// call the callback
	if (pCallback && pCallback->pfnCallback)
	{
		pCallback->pfnCallback( pCallback->pCallbackData, pCallback->dwCallbackData );
	}

	if (bSendTalkDone)
	{
		// tell server we're done talking with them
		MSG_CCMD_TALK_DONE message;
		message.nConversationCompleteType = (int)eType;
		if( AppIsTugboat() )
		{

			// send off our reward selection, if applicable.
			UI_COMPONENT* pRewardPanel = UIComponentGetByEnum(UICOMP_NPCDIALOGBOX);
			if( pComponent )
			{
				UI_PANELEX* pRewardPane = UICastToPanel( UIComponentFindChildByName( pRewardPanel, "reward panel main element" ) );
				message.nRewardSelection = pRewardPane->m_nCurrentTab;

			}
		}
		c_SendMessage( CCMD_TALK_DONE, &message );
	}

	// do "on finished" dialog behavior
	UI_COMPONENT *pCompConversationDialog = pComponent;
	while( pCompConversationDialog && !UIComponentIsConversationDlg( pCompConversationDialog ) ) 
	{
		pCompConversationDialog = pCompConversationDialog->m_pParent;
	}
	if (pCompConversationDialog)
	{
		UI_CONVERSATION_DLG *pConversationDialog = UICastToConversationDlg( pCompConversationDialog );

		// play movie on finished talking if present
		int nDialog = pConversationDialog->m_nDialog;
		if (nDialog != INVALID_LINK)
		{
			const DIALOG_DATA *pDialogData = DialogGetData( nDialog );
			if (pDialogData)
			{
				int nMovieList = pDialogData->nMovieListOnFinished;
				if (nMovieList != INVALID_LINK)
				{
					AppSwitchState( APP_STATE_PLAYMOVIELIST, nMovieList );
				}

			}

		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnClick(
									UI_COMPONENT* component,
									int msg,
									DWORD wParam,
									DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		int nDelta = (msg == UIMSG_RBUTTONCLICK ? -1 : 1);
		sConversationChangePage(UIComponentGetByEnum(UICOMP_NPC_CONVERSATION_DIALOG), nDelta);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnFullTextReveal(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	(void)component;
	(void)msg;
	(void)wParam;
	(void)lParam;
	return UIMSG_RET_HANDLED_END_PROCESS;
	/*
	if (UIComponentGetActive(component))
	{
	// enable ok button
	UI_LABELEX *pLabel = UICastToLabel(UIComponentFindChildByName(component, "label dialog text"));

	if (pLabel && pLabel->m_nPage == pLabel->m_nNumPages-1 && component->m_pParent)
	{
	UI_COMPONENT *pOkBtn = UIComponentFindChildByName(component->m_pParent, "button dialog ok");
	if (!pOkBtn) pOkBtn = UIComponentFindChildByName(component->m_pParent, "video dialog ok button");
	if (pOkBtn)
	{
	UIComponentSetActive(pOkBtn, TRUE);
	}

	UI_COMPONENT *pChild = UIComponentFindChildByName(component->m_pParent, "dialog next btn");
	if (pChild)
	{
	UIComponentSetActive(pChild, FALSE);
	}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;  
	*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL NPCConversationOnInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eResult = UIComponentOnInactivate(component, msg, wParam, lParam);
	if (ResultIsHandled(eResult))
	{
		UIComponentActivate( UICOMP_REWARD_PANEL, FALSE );

		c_TradeCancel(FALSE);
	}

	return eResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnOK(
								 UI_COMPONENT* component,
								 int msg,
								 DWORD wParam,
								 DWORD lParam)
{
	if (UIComponentGetActive(component)
		&& (/*msg == UIMSG_KEYDOWN || */msg == UIMSG_KEYUP || UIComponentCheckBounds(component)))		// This bounds check is unnecessary if this is the result of an LClick message, but I'm leaving it here for Tugboat
	{
		UI_COMPONENT *pParent = component;
		while( pParent && !UIComponentIsConversationDlg(pParent) ) 
		{
			pParent = pParent->m_pParent;
		}

		if (msg == UIMSG_KEYUP)
		{
			if (wParam != L' ' && wParam != VK_RETURN)
			{
				if (wParam == VK_ESCAPE)
				{
					// if the accept button is the only one shown, handle ESC as accept
					UI_COMPONENT *pCancelButton = UIComponentFindChildByName( component->m_pParent, "button dialog cancel" );
					if ( pCancelButton && !UIComponentGetVisible(pCancelButton))
					{
						UIDoConversationComplete( component, CCT_OK );
						return UIMSG_RET_HANDLED_END_PROCESS;  
					}
				}

				return UIMSG_RET_NOT_HANDLED;  
			}

			if (pParent)
			{
				UI_LABELEX *pLabel = UICastToLabel(UIComponentFindChildByName(pParent, "label dialog text"));
				if (pLabel && pLabel->m_nNumPages > 0 && pLabel->m_nPage < pLabel->m_nNumPages - 1)
				{
					sConversationChangePage( pParent, 1 );
					return UIMSG_RET_HANDLED_END_PROCESS;  
				}
				else if (UILabelDoRevealAllText( pLabel ) == TRUE)
				{
					return UIMSG_RET_HANDLED_END_PROCESS;  
				}
			}
		}

		// do complete logic
		if (component->m_dwData == 1)
		{
			// Note that we don't close the conversation ui when we do this because we want
			// the server to validate if the player can in fact take all the items
			// in the dialog ... if they can, the server will then tell the client to
			// close the UI
			UIDoConversationComplete( component, CCT_OK, FALSE, FALSE );
			c_RewardOnTakeAll();
		}
		else
		{
			// hey, Mythos doesn't do this-
			if( AppIsHellgate() )
			{
				ASSERTX_RETVAL(pParent, UIMSG_RET_NOT_HANDLED, "Expected parent conversation dialog");
				UI_CONVERSATION_DLG *pDialog = UICastToConversationDlg(pParent);
				UI_COMPONENT *pDonate = UIComponentFindChildByName(pParent, "donation dialog");
				if (pDialog->m_nQuest != INVALID_LINK)
				{
					UNIT *pPlayer = UIGetControlUnit();
					ASSERTX_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED, "Expected player");

					QUEST *pQuest = QuestGetQuest( pPlayer, pDialog->m_nQuest );
					if (pQuest &&
						QuestIsInactive(pQuest) && 
						!QuestPlayCanAcceptQuest(pPlayer, pQuest) &&
						!pDonate->m_bVisible)
					{
						UIShowGenericDialog(GlobalStringGet( GS_TOO_MANY_QUESTS ), GlobalStringGet( GS_TOO_MANY_QUESTS_DIALOG ));

						UIDoConversationComplete( component, CCT_CANCEL );
					}
					if(pDonate && pDonate->m_bVisible)
					{
						MSG_CCMD_DONATE_MONEY msg;
						UI_COMPONENT* pEditCtrl = UIComponentFindChildByName(pDonate, "donate money editctrl");
						msg.nMoney = PStrToInt(UILabelGetText(pEditCtrl));
						UILabelSetText(pEditCtrl, L"");

						if(msg.nMoney)
							c_SendMessage(CCMD_DONATE_MONEY, &msg);

					}
				}

			}

			UIDoConversationComplete( component, CCT_OK );
		}
		return UIMSG_RET_HANDLED_END_PROCESS;  

	}

	return UIMSG_RET_NOT_HANDLED;  // input not used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnCancel(
									 UI_COMPONENT* component,
									 int msg,
									 DWORD wParam,
									 DWORD lParam)
{
	if (UIComponentGetActive(component)
		&& (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP || UIComponentCheckBounds(component)))		// This bounds check is unnecessary if this is the result of an LClick message, but I'm leaving it here for Tugboat
	{
		if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP)
		{
			if (wParam != VK_ESCAPE)
			{
				return UIMSG_RET_NOT_HANDLED;  // input not used
			}
		}		

		// do complete logic
		UIDoConversationComplete( component, CCT_CANCEL );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}

	return UIMSG_RET_NOT_HANDLED;  // input not used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradeCancel(
							UI_COMPONENT* component,
							int msg,
							DWORD wParam,
							DWORD lParam)
{
	if (c_TradeCancel(FALSE))
		return UIMSG_RET_HANDLED;

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowDuelInviteDialog(
	UNIT *pInviter)
{
	ASSERT_RETURN(pInviter);
	WCHAR szName[MAX_CHARACTER_NAME];
	UnitGetName(pInviter, szName, MAX_CHARACTER_NAME, 0);

	WCHAR szText[512] = L"";
	const WCHAR *szFormat = GlobalStringGet( GS_PVP_DUEL_INVITE_MESSAGE_IN_DUEL_ARENA );
	if (szFormat)
	{
		PStrPrintf(szText, arrsize(szText), szFormat, szName);
	}

	UIShowPlayerRequestDialog((PGUID)pInviter->unitid, szText, c_DuelInviteOnAccept, c_DuelInviteOnDecline, c_DuelInviteOnIgnore);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_sAcceptPartyInvite(PGUID guidOtherPlayer)
{
	CHAT_REQUEST_MSG_ACCEPT_PARTY_INVITE acceptMsg;
	acceptMsg.guidInvitingMember = guidOtherPlayer;
	c_ChatNetSendMessage(&acceptMsg, USER_REQUEST_ACCEPT_PARTY_INVITE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_sDeclinePartyInvite(PGUID guidOtherPlayer)
{
	CHAT_REQUEST_MSG_DECLINE_PARTY_INVITE declineMsg;
	declineMsg.guidInvitingMember = guidOtherPlayer;
	c_ChatNetSendMessage(&declineMsg, USER_REQUEST_DECLINE_PARTY_INVITE);
	c_PlayerClearPartyInvites();}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPartyInviteOnIgnore(
	PGUID guidOtherPlayer, 
	void * )
{
	if (guidOtherPlayer != INVALID_CLUSTERGUID)
	{
		c_sDeclinePartyInvite(guidOtherPlayer);
		c_ChatNetIgnoreMember(guidOtherPlayer);
	}
	else
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding party invitations.");
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPartyInviteOnDecline(
	PGUID guidOtherPlayer, 
	void * )
{
	if (guidOtherPlayer != INVALID_CLUSTERGUID)
	{
		c_sDeclinePartyInvite(guidOtherPlayer);
	}
	else
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding party invitations.");
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sExitInstanceForPartyDialogOKCallback(
	void *pCallbackData, 
	DWORD dwCallbackData)
{
	PGUID *pGuidInviter = (PGUID *)pCallbackData;
	ASSERT_RETURN(pGuidInviter);
	c_sAcceptPartyInvite(*pGuidInviter);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sExitInstanceForPartyDialogCancelCallback(
	void *pCallbackData, 
	DWORD dwCallbackData)
{
	PGUID *pGuidInviter = (PGUID *)pCallbackData;
	ASSERT_RETURN(pGuidInviter);
	c_sDeclinePartyInvite(*pGuidInviter);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowExitInstanceForPartyJoinDialog(
	PGUID guidInviter)
{
	// static variable used for callback data
	static PGUID guidInviterStatic = INVALID_CLUSTERGUID;
	guidInviterStatic = guidInviter;

	DIALOG_CALLBACK tOkCallback;

	DialogCallbackInit( tOkCallback );
	tOkCallback.pfnCallback = sExitInstanceForPartyDialogOKCallback;
	tOkCallback.pCallbackData = &guidInviterStatic;

	DIALOG_CALLBACK tCancelCallback;

	DialogCallbackInit( tCancelCallback );
	tCancelCallback.pfnCallback = sExitInstanceForPartyDialogCancelCallback;
	tCancelCallback.pCallbackData = &guidInviterStatic;

	UIShowGenericDialog(GlobalStringGet(GS_UI_PARTY_JOIN_EXIT_INSTANCE_CONFIRM_HEADER), GlobalStringGet(GS_UI_PARTY_JOIN_EXIT_INSTANCE_CONFIRM), TRUE, &tOkCallback, &tCancelCallback );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sExitInstanceForPartyDialogOKCallback2(
	void *pCallbackData, 
	DWORD dwCallbackData)
{
	UIDoJoinParty();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowExitInstanceForPartyJoinDialog()
{
	// static variable used for callback data
	DIALOG_CALLBACK tOkCallback;

	DialogCallbackInit( tOkCallback );
	tOkCallback.pfnCallback = sExitInstanceForPartyDialogOKCallback2;

	UIShowGenericDialog(GlobalStringGet(GS_UI_PARTY_JOIN_EXIT_INSTANCE_CONFIRM_HEADER), GlobalStringGet(GS_UI_PARTY_JOIN_EXIT_INSTANCE_CONFIRM), TRUE, &tOkCallback );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPartyInviteOnAccept(
	PGUID guidOtherPlayer, 
	void * )
{
	if (guidOtherPlayer != INVALID_CLUSTERGUID)
	{
		if (AppIsTugboat())
		{
			c_sAcceptPartyInvite(guidOtherPlayer);
		}
		else
		{
			// send a message to the game server, before accepting the party 
			// invite- this allows the game server to tell the client
			// if it needs to show a "are you sure you want to abandon your
			// game instance?" dialog
			MSG_CCMD_PARTY_ACCEPT_INVITE tMessage;
			tMessage.guidInviter = guidOtherPlayer; 
			c_SendMessage(CCMD_PARTY_ACCEPT_INVITE, &tMessage);
		}
	}
	else
	{
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding party invitations.");
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowPartyInviteDialog(
	PGUID guidInviter,
	const WCHAR *szInviter)
{
	WCHAR szText[512] = L"";
	const WCHAR *szFormat = GlobalStringGet( GS_UI_INVITE_MESSAGE );
	if (szFormat)
	{
		PStrPrintf(szText, arrsize(szText), szFormat, szInviter);
	}

	UIShowPlayerRequestDialog(guidInviter, szText, sPartyInviteOnAccept, sPartyInviteOnDecline, sPartyInviteOnIgnore);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowGuildInviteDialog(
	PGUID guidInviter,
	const WCHAR *szInviter, 
	const WCHAR *szGuildName)
{
	WCHAR szText[512] = L"";
	const WCHAR *szFormat = GlobalStringGet( GS_UI_GUILD_INVITE_MESSAGE );
	if (szFormat)
	{
		PStrPrintf(szText, arrsize(szText), szFormat, szInviter, szGuildName);
	}
																								// vvv TODO: make this work
	UIShowPlayerRequestDialog(guidInviter, szText, c_GuildInviteAccept, c_GuildInviteDecline, /*c_GuildInviteIgnore*/ NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowBuddyInviteDialog(
	const WCHAR *szInviter)
{
	//UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_BUDDY_INVITE_DIALOG);
	//ASSERT_RETURN(pDialog);

	//UIComponentSetFocusUnit(pDialog, NULL);
	//UI_COMPONENT *pPortrait = UIComponentFindChildByName(pDialog, "dialog portrait");
	//UIComponentSetVisible(pPortrait, FALSE);

	//// even if it can't be shown set the label for a delayed show
	//UI_COMPONENT *pLabel = UIComponentFindChildByName(pDialog, "invite dialog text");
	//if (pLabel)
	//{
	//	const WCHAR *szFormat = GlobalStringGet( GS_BUDDY_REQUEST1 );
	//	if (szFormat)
	//	{
	//		WCHAR szText[512];
	//		PStrPrintf(szText, arrsize(szText), szFormat, szInviter);
	//		UILabelSetText(pLabel, szText);
	//	}
	//}

	//if (sPartyInviteCanBeShown())
	//{
	//	UI_COMPONENT *pOK = UIComponentFindChildByName(pDialog, "invite dialog accept");
	//	//UI_COMPONENT *pCancel = UIComponentFindChildByName(pDialog, "button dialog cancel");

	//	// center the mouse over the OK button
	//	UI_COMPONENT *pCompToCenterMouseOver = pOK;
	//	if (pCompToCenterMouseOver)
	//	{
	//		UI_POSITION tScreenPos = UIComponentGetAbsoluteLogPosition( pCompToCenterMouseOver );
	//		float flX = (tScreenPos.m_fX + (pCompToCenterMouseOver->m_fWidth / 2.0f)) * UIGetLogToScreenRatioX();
	//		float flY = (tScreenPos.m_fY + (pCompToCenterMouseOver->m_fHeight / 2.0f)) * UIGetLogToScreenRatioY();
	//		InputSetMousePosition( flX, flY );
	//	}

	//	UIStopSkills();
	//	UIComponentSetActive(pDialog, TRUE);
	//	UIComponentHandleUIMessage(pDialog, UIMSG_PAINT, 0, 0);
	//}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowBuddyInviteDialog(
	UNIT *pInviter)
{
	ASSERT_RETURN(pInviter);
	WCHAR szName[MAX_CHARACTER_NAME];
	UnitGetName(pInviter, szName, MAX_CHARACTER_NAME, 0);
	UIShowBuddyInviteDialog(szName);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDelayedShowBuddyInviteDialog()
{
	WCHAR * szInviter = c_PlayerGetBuddyInviter();
	if (szInviter && szInviter[0])
	{
		UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_BUDDY_INVITE_DIALOG);
		ASSERT_RETURN(pDialog);

		UIStopSkills();
		UIComponentSetActive(pDialog, TRUE);
		UIComponentHandleUIMessage(pDialog, UIMSG_PAINT, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyInviteOnIgnore(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;  

	if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP || UIComponentCheckBounds(component))		// This bounds check is unnecessary if this is the result of an LClick message, but I'm leaving it here for Tugboat
	{
		if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP)
		{
			//if (wParam != L' ' && wParam != VK_ESCAPE)
			{
				return UIMSG_RET_NOT_HANDLED;  
			}
		}

		WCHAR * szInviter = c_PlayerGetBuddyInviter();
		if (szInviter && szInviter[0])
		{
//			c_BuddyDecline(szInviter);
			c_ChatNetIgnoreMemberByName(szInviter);

			c_PlayerClearBuddyInvites();
		}
		else
		{
			ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding buddy invitations.");
		}

		UIComponentActivate(UICOMP_BUDDY_INVITE_DIALOG, FALSE);
		return UIMSG_RET_HANDLED_END_PROCESS;  

	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyInviteOnDecline(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	/*	buddy list is no longer consensual, so player may not decline
	 *
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;  

	if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP || UIComponentCheckBounds(component))		// This bounds check is unnecessary if this is the result of an LClick message, but I'm leaving it here for Tugboat
	{
		if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP)
		{
			if (wParam != L' ' && wParam != VK_ESCAPE)
			{
				return UIMSG_RET_NOT_HANDLED;  
			}
		}

		WCHAR * szInviter = c_PlayerGetBuddyInviter();
		if (szInviter && szInviter[0])
		{
			c_BuddyDecline(szInviter);
			c_PlayerClearBuddyInvites();
		}
		else
		{
			ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding buddy invitations.");
		}

		UIComponentActivate(UICOMP_BUDDY_INVITE_DIALOG, FALSE);
		return UIMSG_RET_HANDLED_END_PROCESS;  

	}
*/
	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBuddyInviteOnAccept(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;  

	if (msg == UIMSG_KEYDOWN  || msg == UIMSG_KEYUP
		|| UIComponentCheckBounds(component))		// This bounds check is unnecessary if this is the result of an LClick message, but I'm leaving it here for Tugboat
	{
		if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP)
		{
			if (wParam != L' ' && wParam != VK_RETURN)
			{
				return UIMSG_RET_NOT_HANDLED;  
			}
		}

		WCHAR * szInviter = c_PlayerGetBuddyInviter();
		if (szInviter && szInviter[0])
		{
			//	the buddy system is no longer consentual, players will never have to
			//		"accept" a buddy request
			//c_BuddyAccept(szInviter);
		}
		else
		{
			ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding buddy invitations.");
		}

		UIComponentActivate(UICOMP_BUDDY_INVITE_DIALOG, FALSE);
		return UIMSG_RET_HANDLED_END_PROCESS;  

	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIGetCurrentMenu(void)
{
	return g_UI.m_pCurrentMenu;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMenuSetSelect(int nSelected)
{
	UI_COMPONENT *pMenu = UIGetCurrentMenu(); 
	if (!pMenu || !UIComponentGetActive(pMenu))
	{
		return;
	}

	if (nSelected < 0)
		nSelected = 0;

	UI_COMPONENT *pChild = pMenu->m_pFirstChild;
	while (pChild)
	{
		if (UIComponentIsLabel(pChild))
		{
			UI_LABELEX *pLabel = UICastToLabel(pChild);
			if (pLabel->m_nMenuOrder != -1 && !pLabel->m_bDisabled)
			{
				pLabel->m_bSelected = (pLabel->m_nMenuOrder == nSelected);
				if (pLabel->m_nMenuOrder == nSelected)
					pMenu->m_dwData = (DWORD)nSelected;
			}
		}
		pChild = pChild->m_pNextSibling;
	}

	UIComponentHandleUIMessage(pMenu, UIMSG_PAINT, 0, 0); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveFromNameList(
								int nIndex)
{
	ASSERT_RETURN(nIndex >= 0 && nIndex < MAX_NAME_LIST);

	if ( gppCharSelectUnits[nIndex] )
		UnitFree( gppCharSelectUnits[nIndex] );

	for (int i = nIndex; i < MAX_NAME_LIST-1; i++)
	{
		PStrCopy(gpszCharSelectNames[i], gpszCharSelectNames[i+1], MAX_CHARACTER_NAME);
		PStrCopy(gpszCharSelectGuildNames[i], gpszCharSelectGuildNames[i+1], MAX_GUILD_NAME);
		gppCharSelectUnits[i] = gppCharSelectUnits[i+1];
	}

	gpszCharSelectNames[MAX_NAME_LIST-1][0] = L'\0';
	gpszCharSelectGuildNames[MAX_NAME_LIST-1][0] = L'\0';
	gppCharSelectUnits[MAX_NAME_LIST-1] = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sClearFromNameList(
							   int nIndex)
{
	ASSERT_RETURN(nIndex >= 0 && nIndex < MAX_NAME_LIST);

	if ( gppCharSelectUnits[nIndex] )
		UnitFree( gppCharSelectUnits[nIndex] );

	gpszCharSelectNames[nIndex][0] = L'\0';
	gpszCharSelectGuildNames[nIndex][0] = L'\0';
	gppCharSelectUnits[nIndex] = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//void PlayerIterateSinglePlayerSaves();
/*
PLAYER_ITERATE_SP_SAVES PlayerIterateSinglePlayerSaves(
	const TARGET_TYPE *eTargetTypes,
	PFN_ROOM_ITERATE_UNIT_CALLBACK pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETVAL( pRoom, RIU_CONTINUE, "Expected room" );
	ASSERTX_RETVAL( pfnCallback, RIU_CONTINUE, "Expected callback" );
}
*/


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFillNameList()
{
	sClearNameList();

	if (!g_UI_StartgameParams.m_bSinglePlayer &&			// we may not have "officially" set the app type yet.  So check for m_bSinglePlayer
		(AppGetType() == APP_TYPE_CLOSED_CLIENT ||
		AppGetType() == APP_TYPE_CLOSED_SERVER))
	{
		return;
	}

	// fill in array with file info
	ULARGE_INTEGER puNameTime[ MAX_NAME_LIST ];

	const OS_PATH_CHAR * szWidePath = FilePath_GetSystemPath(FILE_PATH_SAVE);

	int nPlayerSlotCurr = 0;
	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, MAX_PATH, OS_PATH_TEXT("%s*.hg1"), szWidePath);
	OS_PATH_FUNC(WIN32_FIND_DATA) data;
	FindFileHandle handle = OS_PATH_FUNC(FindFirstFile)( filename, &data );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		// grab em all
		do
		{
			WCHAR cFileName[_countof(gpszCharSelectNames[0])];
			PStrCvt(cFileName, data.cFileName, _countof(cFileName));
			PStrRemoveExtension(gpszCharSelectNames[ nPlayerSlotCurr ], _countof(gpszCharSelectNames[ nPlayerSlotCurr ]), cFileName);
			puNameTime[ nPlayerSlotCurr ].HighPart = data.ftLastWriteTime.dwHighDateTime;
			puNameTime[ nPlayerSlotCurr ].LowPart = data.ftLastWriteTime.dwLowDateTime;
			nPlayerSlotCurr++;
		} while ( ( nPlayerSlotCurr < MAX_NAME_LIST ) && ( OS_PATH_FUNC(FindNextFile)( handle, &data ) ) );

		// sort
		for ( int i = 0; i < nPlayerSlotCurr-1; i++ )
		{
			for ( int j = i; j < nPlayerSlotCurr; j++ )
			{
				BOOL bSwap = FALSE;
				if ( puNameTime[j].HighPart > puNameTime[i].HighPart )
				{
					bSwap = TRUE;
				} else if ( puNameTime[j].HighPart == puNameTime[i].HighPart )
				{
					if ( puNameTime[j].LowPart > puNameTime[i].LowPart )
						bSwap = TRUE;
				}
				if ( bSwap )
				{
					// swap
					WCHAR name_temp[ MAX_CHARACTER_NAME ];
					PStrCopy( name_temp, gpszCharSelectNames[i], _countof(name_temp) );
					PStrCopy( gpszCharSelectNames[i], gpszCharSelectNames[j], _countof(gpszCharSelectNames[i]) );
					PStrCopy( gpszCharSelectNames[j], name_temp, _countof(gpszCharSelectNames[j]) );
					ULARGE_INTEGER time_temp = puNameTime[i];
					puNameTime[i] = puNameTime[j];
					puNameTime[j] = time_temp;
				}
			}
		}
	}

	if(nPlayerSlotCurr!= 0)
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_DISABLE_TUTORIAL;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_DISABLE_TUTORIAL;
	//g_UI_StartgameParams.m_bSkipTutorialTips = nPlayerSlotCurr != 0;
	sUpdateTutorialButton();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UISetCurrentMenu(UI_COMPONENT * pMenu)
{
	if (g_UI.m_pCurrentMenu == pMenu)
		return;

	if (g_UI.m_pCurrentMenu)
	{
		//UIComponentSetActive(g_UI.m_pCurrentMenu, FALSE);
		//UIComponentSetVisible(g_UI.m_pCurrentMenu, FALSE);
		UIComponentActivate(g_UI.m_pCurrentMenu, FALSE);

		UIRemoveMouseOverrideComponent(g_UI.m_pCurrentMenu);
		UIRemoveKeyboardOverrideComponent(g_UI.m_pCurrentMenu);
	}
	g_UI.m_pCurrentMenu = pMenu;
	if (pMenu)
	{
		UIComponentActivate(pMenu, TRUE);
		UIMenuSetSelect(0);
		// hack to enable changing between "back" and "quit"
		if (pMenu == UIComponentGetByEnum(UICOMP_STARTGAME_CHARACTER))
		{
			UI_COMPONENT *pBack = UIComponentFindChildByName(pMenu, "ui menu character back");
			UI_COMPONENT *pQuit = UIComponentFindChildByName(pMenu, "ui menu character quit");
			if (!AppIsDemo() && 
				(AppCommonMultiplayerOnly() || AppCommonSingleplayerOnly()))
			{
				if (pQuit)
				{
					UIComponentActivate(pQuit, TRUE);
				}
				if (pBack)
				{
					UIComponentActivate(pBack, FALSE);
				}
			}
			else 
			{
				if (pQuit)
				{
					UIComponentActivate(pQuit, FALSE);
				}
				if (pBack)
				{
					UIComponentActivate(pBack, TRUE);
				}
			}
		}

		UISetMouseOverrideComponent(pMenu);
		UISetKeyboardOverrideComponent(pMenu);
	}

	UIComponentHandleUIMessage(pMenu, UIMSG_PAINT, 0, 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideCurrentMenu(void)
{
	if (g_UI.m_pCurrentMenu)
	{
		g_UI.m_idLastMenu = UIComponentGetId(g_UI.m_pCurrentMenu);
	}

	UISetCurrentMenu(NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIRestoreLastMenu(void)
{
	if (g_UI.m_idLastMenu != INVALID_ID)
	{
		UI_COMPONENT *pMenu = UIComponentGetById(g_UI.m_idLastMenu);
		if (pMenu)
			UISetCurrentMenu(pMenu);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMenuDoSelected(void)
{
	UI_COMPONENT *pMenu = UIGetCurrentMenu();
	if (!pMenu || !UIComponentGetActive(pMenu))
	{
		return;
	}

	if (pMenu->m_dwData < 0)
		return;

	UI_COMPONENT *pChild = pMenu->m_pFirstChild;
	while (pChild)
	{
		if (UIComponentIsLabel(pChild))
		{
			UI_LABELEX *pLabel = UICastToLabel(pChild);
			if (pLabel->m_nMenuOrder == (int)pMenu->m_dwData && !pLabel->m_bDisabled)
			{
				UIComponentHandleUIMessage(pChild, UIMSG_LBUTTONCLICK, 0, 0);
				return;
			}
		}
		pChild = pChild->m_pNextSibling;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMenuChangeSelect(int nDelta)
{
	UI_COMPONENT *pMenu = UIGetCurrentMenu();
	if (!pMenu || !UIComponentGetActive(pMenu))
	{
		return;
	}

	int nCurrentSelect = (int)pMenu->m_dwData;

	if (nDelta == 0)
	{
		return;
	}

	// we need to find the child that's either the closest higher or the closest lower (depending on delta)
	//   we can't depend on the numbers being consecutive.  

	int nClosest = -1;
	int nDistance = 1000;

	UI_COMPONENT *pChild = pMenu->m_pFirstChild;
	while (pChild)
	{
		if (UIComponentIsLabel(pChild))
		{
			UI_LABELEX *pLabel = UICastToLabel(pChild);
			if (pLabel->m_nMenuOrder != -1 && !pLabel->m_bDisabled)
			{
				pLabel->m_bSelected = UIComponentCheckBounds(pLabel);

				if ((nDelta < 0 && pLabel->m_nMenuOrder < nCurrentSelect) ||
					(nDelta > 0 && pLabel->m_nMenuOrder > nCurrentSelect))
				{
					int nThisDistance = abs(pLabel->m_nMenuOrder - nCurrentSelect);
					if (nThisDistance < nDistance)
					{
						nDistance = nThisDistance;
						nClosest = pLabel->m_nMenuOrder;
					}
				}
			}
		}
		pChild = pChild->m_pNextSibling;
	}

	if (nClosest != -1)
	{
		UIMenuSetSelect(nClosest);
	}
	else
	{
		UIMenuSetSelect(nCurrentSelect);
	}
}

UI_COMPONENT * UIGetMainMenu( void )
{
	if (AppIsDemo())
	{
		return UIComponentGetByEnum(UICOMP_STARTGAME_DEMO_MENU);
	}
	else
	{
		if( AppIsTugboat() )
		{
			return UIComponentGetByEnum(UICOMP_STARTGAME_GAMETYPE);
		}
		else
		{

#if ISVERSION(CLIENT_ONLY_VERSION)
			return UIComponentGetByEnum(UICOMP_STARTGAME_GAMETYPE_MP_ONLY);
#elif ISVERSION(RELEASE_VERSION)
			return UIComponentGetByEnum(UICOMP_STARTGAME_GAMETYPE_SP_ONLY);
#else
			return UIComponentGetByEnum(UICOMP_STARTGAME_GAMETYPE);
#endif
		}
	}
}

void UIShowMainMenu( STARTGAME_CALLBACK pfnStartgameCallback )
{
	if (AppIsTugboat())
	{
		c_ClearVisibleRooms();
		//structclear(gppCharSelectUnits);
		//structclear(gpppCharCreateUnits);
	}

	structclear( g_UI_StartgameParams );

	g_UI_StartgameParams.m_idCharCreateUnit = INVALID_ID;
	g_UI_StartgameParams.m_nCharacterClass = CharacterClassGetDefault();
	//g_UI_StartgameParams.m_bSkipTutorialTips = FALSE;
	g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_DISABLE_TUTORIAL;

	g_UI_StartgameCallback = pfnStartgameCallback;
#ifdef DRB_3RD_PERSON
	UISetInterfaceActive( TRUE );
#endif

	// TODO: TURN MULTI-SINGLE PLAYER OPTIONS ON AND OFF
	//#if !ISVERSION(CLIENT_ONLY_VERSION) && !ISVERSION(DEVELOPMENT)

	UISetCurrentMenu(UIGetMainMenu());

	if (AppGetState() == APP_STATE_CONNECTING)
	{
		AppSwitchState(APP_STATE_MAINMENU);
	}

	if (!AppIsDemo())
	{
		//if (AppCommonMultiplayerOnly())
		//{
		//	g_UI_StartgameParams.m_bSinglePlayer = FALSE;
		//	UIStartGameMenuDoMultiplayer(NULL, 0, 0, 0);
		//}
		//else if (AppCommonSingleplayerOnly())
		//{
		//	g_UI_StartgameParams.m_bSinglePlayer = TRUE;
		//	UIStartGameMenuDoSingleplayer(NULL, 0, 0, 0);
		//}
	}

	// clear all town portal data we have
	c_TownPortalInitForApp();
}

BOOL UISkipMainMenu( STARTGAME_CALLBACK pfnStartgameCallback )
{
	g_UI_StartgameParams.m_bSinglePlayer = TRUE;
	// Created 10/11/06 by Matthew Sellers
	if(gApp.m_wszSavedFile[0])
	{
		PStrCopy(g_UI_StartgameParams.m_szPlayerName, gApp.m_wszSavedFile, 256);
		goto UILoadChar;
	}

	UIComponentActivate(UICOMP_DASHBOARD, TRUE);

	// End of Added material
	sFillNameList();
	if (gpszCharSelectNames[0][0])
	{
		PStrCopy(g_UI_StartgameParams.m_szPlayerName, gpszCharSelectNames[0], 256);
UILoadChar:					// Created 10/11/06 by Matthew Sellers
		if (AppCommonMultiplayerOnly())
		{
			g_UI_StartgameParams.m_bSinglePlayer = FALSE;
		}
		else if (AppCommonSingleplayerOnly())
		{
			g_UI_StartgameParams.m_bSinglePlayer = TRUE;
		}

		// clear all town portal data we have
		c_TownPortalInitForApp();

		pfnStartgameCallback(TRUE, g_UI_StartgameParams);

		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideMainMenu(void)
{
	UISetCurrentMenu(NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowGameMenu(void)
{
	//UIStopSkills();
	//c_SoundPauseAll(TRUE);

	// close any possibly interfering UI that might still be open
	UIQuickClose();

	c_PlayerClearAllMovement(AppGetCltGame());
	if (!AppIsTugboat())
	{
		UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);
	}
	UISetCurrentMenu(UIComponentGetByEnum(UICOMP_GAMEMENU));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideGameMenu(void)
{
	//c_SoundPauseAll(FALSE);
	UISetCurrentMenu(NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMenuItemOnMouseOver( 
									UI_COMPONENT* component, 
									int msg, 
									DWORD wParam, 
									DWORD lParam )
{
	if (!AppIsTugboat() ||
		(!UIComponentCheckBounds(component) || !UIComponentGetVisible(component)))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIMenuSetSelect((int)component->m_dwParam);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGameMenuDoReturn( 
								 UI_COMPONENT* component, 
								 int msg, 
								 DWORD wParam, 
								 DWORD lParam )
{
	AppPause(FALSE);
	AppUserPause(FALSE);
	UIHideGameMenu();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sRestartGameCallbackOk(
							void *, 
							DWORD )
{
	AppPause(FALSE);
	AppUserPause(FALSE);

	UIHideGameMenu();
	UIGameRestart();

	// tell the server we want to exit
	MSG_CCMD_EXIT_GAME msgExit;
	c_SendMessage(CCMD_EXIT_GAME, &msgExit);
	if(!AppIsSinglePlayer())
		AppSwitchState(APP_STATE_RESTART);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGameMenuDoRestart( 
								  UI_COMPONENT* component, 
								  int msg, 
								  DWORD wParam, 
								  DWORD lParam )
{
	if( AppIsHellgate() )
	{
		DIALOG_CALLBACK tOkCallback;

		DialogCallbackInit( tOkCallback );
		tOkCallback.pfnCallback = sRestartGameCallbackOk;
		UIShowGenericDialog(GlobalStringGet(GS_MENU_CONFIRM_EXIT_HEADER), GlobalStringGet(GS_MENU_CONFIRM_EXIT), TRUE, &tOkCallback );

	}
	else
	{
		sRestartGameCallbackOk( NULL, 0 );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGameMenuOnPostActivate( 
									   UI_COMPONENT* component, 
									   int msg, 
									   DWORD wParam, 
									   DWORD lParam )
{
	AppPause(TRUE);
	AppUserPause(TRUE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.05.10
void UIExitApplication(void)
{
	AppPause(FALSE);
	AppUserPause(FALSE);
	c_SendRemovePlayer();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGameMenuDoExit( 
							   UI_COMPONENT* component, 
							   int msg, 
							   DWORD wParam, 
							   DWORD lParam )
{
	GAME *game = AppGetCltGame();
	if( game )
	{
		UNIT * unit = GameGetControlUnit( game );
		if ( unit )
		{
			if ( UnitTestFlag( unit, UNITFLAG_QUESTSTORY ) )
			{
				c_QuestAbortCurrent( game, unit );
				return UIMSG_RET_HANDLED;
			}
		}
	}

	UIExitApplication();	// CHB 2007.05.10

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGameMenuDoOptions( 
								  UI_COMPONENT* component, 
								  int msg, 
								  DWORD wParam, 
								  DWORD lParam )
{
	if (AppIsTugboat())
	{
		if (!UIComponentGetActive(component))
		{
			return UIMSG_RET_HANDLED;
		}
		if (!UIComponentCheckBounds(component) && !wParam)
		{
			return UIMSG_RET_HANDLED;
		}
	}

	GAME *game = AppGetCltGame();
	if (game)
	{
		UNIT * unit = GameGetControlUnit( game );
		if ( unit )
		{
			if ( UnitTestFlag( unit, UNITFLAG_QUESTSTORY ) )
			{
				c_QuestAbortCurrent( game, unit );
				return UIMSG_RET_HANDLED;
			}
		}
	}

	UIShowOptionsDialog();
	return UIMSG_RET_HANDLED_END_PROCESS;
}

// !!! menu functions VVVVV ///////////////////////////////////////////////////

UI_MSG_RETVAL UIStartGameMenuDoSingleplayer( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	g_UI_StartgameParams.m_bSinglePlayer = TRUE;
	AppSetType(APP_TYPE_SINGLE_PLAYER);

	sFillNameList();

	AppSwitchState(APP_STATE_CHARACTER_CREATE);
	UISetCurrentMenu(NULL);

	UIComponentActivate(UICOMP_CHARACTER_SELECT, TRUE);	

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStartGameMenuDoMultiplayer( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{

	g_UI_StartgameParams.m_bSinglePlayer = FALSE;
	if (AppGetType() == APP_TYPE_CLOSED_CLIENT)
	{
		sClearNameList();

		// Skip the login if one has been passed in the cmdline
		if (gApp.userName[0] && gApp.passWord[0])
		{
			AppSwitchState(APP_STATE_CONNECTING_START);	
		}
		else
		{
			UISetCurrentMenu(UIComponentGetByEnum(UICOMP_LOGIN_SCREEN));
		}
	}
	else
	{
		sFillNameList();

		AppSwitchState(APP_STATE_CHARACTER_CREATE);
		UISetCurrentMenu(NULL);

		UIComponentActivate(UICOMP_CHARACTER_SELECT, TRUE);	
	}
	
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStartGameMenuDoQuit( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (!UIComponentGetActive(component) || !UIComponentGetActive(component->m_pParent))
		return UIMSG_RET_NOT_HANDLED;

	UISetCurrentMenu(NULL);

	if (g_UI_StartgameCallback)
		g_UI_StartgameCallback(FALSE, g_UI_StartgameParams);

	return UIMSG_RET_HANDLED_END_PROCESS;
}
UI_MSG_RETVAL UIStartGameMenuDoGetGameType( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (!UIComponentGetActive(component) || !UIComponentGetActive(component->m_pParent))
		return UIMSG_RET_NOT_HANDLED;

	if (msg == UIMSG_KEYUP && wParam != VK_ESCAPE)
		return UIMSG_RET_NOT_HANDLED;

	UISetCurrentMenu(UIGetMainMenu());
	if (AppGetState() == APP_STATE_CONNECTING)
	{
		AppSwitchState(APP_STATE_MAINMENU);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

UI_MSG_RETVAL UIDemoMenuDoOrder( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	WCHAR uszURL[ REGION_MAX_URL_LENGTH ];
	RegionCurrentGetURL( RU_ORDER_GAME, uszURL, REGION_MAX_URL_LENGTH );
	//const WCHAR *szURL = GlobalStringGet(GS_URL_ORDER_GAME);
	if (uszURL[ 0 ])
	{
		ShowWindow(AppCommonGetHWnd(), SW_MINIMIZE);
		ShellExecuteW(NULL, NULL, uszURL, NULL, NULL, SW_SHOW );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

// !!! menu functions ^^^^^ ///////////////////////////////////////////////////

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILoginScreenOnBack( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if ((msg == UIMSG_KEYUP || msg == UIMSG_KEYDOWN) && wParam != VK_ESCAPE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (AppCommonSingleplayerOnly())
	{
		UISetCurrentMenu(NULL);

		if (g_UI_StartgameCallback)
			g_UI_StartgameCallback(FALSE, g_UI_StartgameParams);
	}
	//else if (AppCommonMultiplayerOnly())
	//{
	//	UIStartGameMenuDoQuit(component, msg, wParam, lParam);
	//}
	else
	{
		UISetCurrentMenu(UIGetMainMenu());
		if (AppGetState() == APP_STATE_CONNECTING)
		{
			AppSwitchState(APP_STATE_MAINMENU);
		}

	}

	UILabelSetTextByEnum(UICOMP_LOGIN_PASSWORD, L"");

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILoginScreenOnEnter( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if ((msg == UIMSG_KEYUP || msg == UIMSG_KEYDOWN) && wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	g_UI_StartgameParams.m_bSinglePlayer = FALSE;

	const WCHAR * szLoginName = UILabelGetTextByEnum(UICOMP_LOGIN_NAME);
	const WCHAR * szPassword = UILabelGetTextByEnum(UICOMP_LOGIN_PASSWORD);

	if (!szLoginName || szLoginName[0] == L'\0')
	{
		UIShowGenericDialog(GlobalStringGet(GS_UI_LOGIN_MISSING_INFO_TITLE), GlobalStringGet(GS_UI_LOGIN_MISSING_NAME), FALSE,0,0,GDF_OVERRIDE_EVENTS);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	if (!szPassword || szPassword[0] == L'\0')
	{
		UIShowGenericDialog(GlobalStringGet(GS_UI_LOGIN_MISSING_INFO_TITLE), GlobalStringGet(GS_UI_LOGIN_MISSING_PASSWORD), FALSE,0,0,GDF_OVERRIDE_EVENTS);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	PStrCvt(gApp.userName, szLoginName, MAX_USERNAME_LEN);
	PStrCvt(gApp.passWord, szPassword, MAX_USERNAME_LEN);

	if (AppIsTugboat())
	{
		UISetCurrentMenu(UIComponentGetByEnum(UICOMP_STARTGAME_CHARACTER));
	}

	AppSwitchState(APP_STATE_CONNECTING_START);	

	UILabelSetTextByEnum(UICOMP_LOGIN_PASSWORD, L"");

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILoginScreenOnKeyUp( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (wParam == VK_ESCAPE)
	{
		UILoginScreenOnBack(component, msg, wParam, lParam);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (wParam == VK_RETURN)
	{
		UILoginScreenOnEnter(component, msg, wParam, lParam);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICreateAccountOnClick( 
	UI_COMPONENT* component, 
	int msg, 
	DWORD wParam, 
	DWORD lParam )
{
	UI_COMPONENT * pRealmCombo = UIComponentFindChildByName( component->m_pParent, UI_REALM_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pRealmCombo);
	int nIndex = UIComboBoxGetSelectedIndex(pCombo);

	WCHAR uszURL[ REGION_MAX_URL_LENGTH ] = L"";

	if (nIndex != INVALID_ID)
	{
		QWORD nServerIndexData = UIComboBoxGetSelectedData(pCombo);
		BOOL bServerListOverride = !!(nServerIndexData & SERVERLIST_OVERRIDE_FLAG);
		int nServerIndex = (int)(nServerIndexData & 0xffffffff);

//		const SERVERLIST_DEFINITION * pServerlist = DefinitionGetServerlist();
//		const SERVERLISTOVERRIDE_DEFINITION * pServerlistOverride = DefinitionGetServerlistOverride();

		const T_SERVERLIST * pServerlist = ServerListGet();
		const T_SERVERLIST * pServerlistOverride = ServerListOverrideGet();

		WORLD_REGION eRegion = WORLD_REGION_INVALID;
		if ( pServerlist && !bServerListOverride )
		{
			ASSERT_RETVAL(nServerIndex >= 0 && nServerIndex < pServerlist->nServerDefinitionCount, UIMSG_RET_HANDLED);
			PStrCvt(uszURL,pServerlist->pServerDefinitions[nServerIndex].szURLs[RU_ACCOUNT_CREATE],REGION_MAX_URL_LENGTH);
			eRegion = pServerlist->pServerDefinitions[nServerIndex].eRegion;
		}
		else if( pServerlistOverride && bServerListOverride )
		{
			ASSERT_RETVAL(nServerIndex >= 0 && nServerIndex < pServerlistOverride->nServerDefinitionCount, UIMSG_RET_HANDLED);
			PStrCvt(uszURL,pServerlistOverride->pServerDefinitions[nServerIndex].szURLs[RU_ACCOUNT_CREATE],REGION_MAX_URL_LENGTH);
			eRegion = pServerlistOverride->pServerDefinitions[nServerIndex].eRegion;
		}

		URL_REPLACEMENT_INFO tReplaceInfo;
		tReplaceInfo.eRegion = eRegion;
		tReplaceInfo.eLanguage = LanguageGetCurrent();
		RegionURLDoReplacements( AppGameGet(), uszURL, REGION_MAX_URL_LENGTH, &tReplaceInfo );
		
	}

	if (!uszURL[0])
	{
		RegionCurrentGetURL(RU_ACCOUNT_CREATE, uszURL, REGION_MAX_URL_LENGTH);
	}

	if (uszURL[ 0 ])
	{
		ShowWindow(AppCommonGetHWnd(), SW_MINIMIZE);
		ShellExecuteW(NULL, NULL, uszURL, NULL, NULL, SW_SHOW );
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIManageAccountOnClick( 
									 UI_COMPONENT* component, 
									 int msg, 
									 DWORD wParam, 
									 DWORD lParam )
{
	WCHAR uszURL[ REGION_MAX_URL_LENGTH ];
	RegionCurrentGetURL( RU_ACCOUNT_MANAGE_NAME, uszURL, REGION_MAX_URL_LENGTH );
	//const WCHAR *szURL = GlobalStringGet(GS_URL_ACCOUNT_MANAGE_NAME);
	if (uszURL[ 0 ])
	{
		ShowWindow(AppCommonGetHWnd(), SW_MINIMIZE);
		ShellExecuteW(NULL, NULL, uszURL, NULL, NULL, SW_SHOW );

	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMenuDoSelectedOnEnter( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (wParam == VK_RETURN &&
		UIComponentGetActive(component))
	{
		UIMenuDoSelected();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIConversationOnKeyDown(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		if (wParam == VK_ESCAPE || wParam == VK_SPACE)
		{
			UIComponentSetVisible(component, FALSE);
			UIRemoveMouseOverrideComponent(component);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		// eat all other keys
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDeleteCharDialogOnKeyDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		if (wParam == VK_ESCAPE)
		{
			UIComponentSetVisible(component, FALSE);
			UIRemoveMouseOverrideComponent(component);
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		// eat all other keys
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowGenericDialog(
						 const char *szHeader,
						 const char *szText,
						 BOOL bShowCancel /*= FALSE*/,
						 const DIALOG_CALLBACK *pCallbackOK /*= NULL*/,
						 const DIALOG_CALLBACK *pCallbackCancel /*= NULL*/,
						 DWORD dwGenericDialogFlags /*= 0*/)
{
	WCHAR wszHeader[2048];
	WCHAR wszText[2048];

	PStrCvt(wszHeader, szHeader, 2048);
	PStrCvt(wszText, szText, 2048);

	UIShowGenericDialog(wszHeader, wszText, bShowCancel, pCallbackOK, pCallbackCancel, dwGenericDialogFlags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowGenericDialog(
						 const WCHAR *szHeader,
						 const WCHAR *szText,
						 BOOL bShowCancel /*= FALSE*/,
						 const DIALOG_CALLBACK *pCallbackOK /*= NULL*/,
						 const DIALOG_CALLBACK *pCallbackCancel /*= NULL*/,
						 DWORD dwGenericDialogFlags /*= 0*/)
{
	if (AppIsTugboat())
	{
		if( UI_TB_IsNPCDialogUp() )
		{
			UIDoConversationComplete(
				UIComponentGetByEnum(UICOMP_NPCDIALOGBOX),
				CCT_CANCEL );
		}
	}

	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
	UI_COMPONENT * pParentDialog = pDialog;

	// Mythos has to do this to grab the sub element, because of the
	// noscaling nature of the UI that requires a parent element to position
	if( AppIsTugboat() )
	{

		pDialog = UIComponentFindChildByName( pDialog, "generic dialog main element" );
	}
	if (pDialog)
	{
		UI_COMPONENT *pChild = UIComponentFindChildByName(pDialog, "generic dialog header");
		if (pChild && pChild->m_eComponentType == UITYPE_LABEL)
		{
			UILabelSetText(UICastToLabel(pChild), szHeader);
			if( pChild->m_pParent->m_eComponentType == UITYPE_TOOLTIP )
			{
				UITooltipDetermineContents(pChild->m_pParent);
				UITooltipDetermineSize(pChild->m_pParent);
			}
		}
		pChild = UIComponentFindChildByName(pDialog, "generic dialog text");
		if (pChild && pChild->m_eComponentType == UITYPE_LABEL)
		{
			UILabelSetText(UICastToLabel(pChild), szText);
		}

		UI_COMPONENT *pOK = UIComponentFindChildByName(pDialog, "button dialog ok");
		UI_COMPONENT *pCancel = UIComponentFindChildByName(pDialog, "button dialog cancel");

		const BOOL bShowOK = !(dwGenericDialogFlags & GDF_NO_OK_BUTTON);
		const float fOffset = MAX((pOK ? fabs(pOK->m_fToolTipXOffset) : 0.0f), (pCancel ? fabs(pCancel->m_fToolTipXOffset) : 0.0f));

		if (AppIsHellgate())
		{
			if (pOK)
			{
				if (bShowOK && (!pCancel || !bShowCancel))
				{
					pOK->m_nTooltipJustify = UIALIGN_CENTER;
					pOK->m_fToolTipXOffset = 0.0f;
				}
				else
				{
					pOK->m_nTooltipJustify = UIALIGN_LEFT;
					pOK->m_fToolTipXOffset = +fOffset;
				}
			}
			if (pCancel)
			{
				if (bShowCancel && (!pOK || !bShowOK))
				{
					pCancel->m_nTooltipJustify = UIALIGN_CENTER;
					pCancel->m_fToolTipXOffset = 0.0f;
				}
				else
				{
					pCancel->m_nTooltipJustify = UIALIGN_RIGHT;
					pCancel->m_fToolTipXOffset = -fOffset;
				}
			}
		}

		if (pCancel)
		{
			if (bShowCancel)
			{
				UIComponentSetActive(pCancel, TRUE);
			}
			else
			{
				UIComponentSetVisible(pCancel, FALSE);
			}

			UIComponentHandleUIMessage(pCancel, UIMSG_PAINT, 0, 0);
		}

		if (pOK)
		{
			if (bShowOK)
			{
				UIComponentSetActive(pOK, TRUE);
			}
			else
			{
				UIComponentSetVisible(pOK, FALSE);
			}

			UIComponentHandleUIMessage(pOK, UIMSG_PAINT, 0, 0);
		}

		// save callbacks
		DialogCallbackInit( pDialog->m_tCallbackOK );
		DialogCallbackInit( pDialog->m_tCallbackCancel );
		if (pCallbackOK)
		{
			pDialog->m_tCallbackOK = *pCallbackOK;
		}
		if (pCallbackCancel)
		{
			pDialog->m_tCallbackCancel = *pCallbackCancel;
		}		

		if (UIComponentIsTooltip(pDialog))
		{
			UI_TOOLTIP * pTooltipDialog = UICastToTooltip(pDialog);
			UITooltipDetermineContents(pTooltipDialog);
			UITooltipDetermineSize(pTooltipDialog);
		}

		if (AppIsHellgate())
		{
			// Center the dialog
			float fAppWidth = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX();
			float fAppHeight = AppCommonGetWindowHeight() * UIGetScreenToLogRatioY();
			pDialog->m_Position.m_fX = (fAppWidth - (pDialog->m_fWidth * UIGetScreenToLogRatioX(pDialog->m_bNoScale))) / 2.0f;
			pDialog->m_Position.m_fY = (fAppHeight - (pDialog->m_fHeight * UIGetScreenToLogRatioY(pDialog->m_bNoScale))) / 2.0f;
			pDialog->m_ActivePos = pDialog->m_Position;
			pDialog->m_InactivePos = pDialog->m_Position;
		}
		if( AppIsHellgate() )
		{
			if (dwGenericDialogFlags & GDF_OVERRIDE_EVENTS)
			{
				pDialog->m_dwParam = 1;
			}
			else
			{
				pDialog->m_dwParam = 1;
			}
		}
		else
		{
			if (dwGenericDialogFlags & GDF_OVERRIDE_EVENTS)
			{
				pParentDialog->m_dwParam = 1;
			}
			else
			{
				pParentDialog->m_dwParam = 0;
			}
		}
		//DWORD dwAnimTime = 0;
		//UIComponentFadeIn(pDialog, 1.0f, dwAnimTime);
		if( AppIsHellgate() )
		{
			UIComponentActivate(pDialog, TRUE);
		}
		else
		{
			UIComponentActivate(pParentDialog, TRUE);			
		}

		// center the mouse over the OK button
		UI_COMPONENT *pCompToCenterMouseOver = NULL;
		if (dwGenericDialogFlags & GDF_CENTER_MOUSE_ON_OK)
		{
			pCompToCenterMouseOver = pOK;
		}
		else if (dwGenericDialogFlags & GDF_CENTER_MOUSE_ON_CANCEL)
		{
			pCompToCenterMouseOver = pCancel;
		}
		if (pCompToCenterMouseOver)
		{
			UI_POSITION tScreenPos = UIComponentGetAbsoluteLogPosition( pCompToCenterMouseOver );
			float flX = (tScreenPos.m_fX + (pCompToCenterMouseOver->m_fWidth / 2.0f)) * UIGetLogToScreenRatioX();
			float flY = (tScreenPos.m_fY + (pCompToCenterMouseOver->m_fHeight / 2.0f)) * UIGetLogToScreenRatioY();
			InputSetMousePosition( flX, flY );
		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowConfirmDialog(const char * szStringKey1, const char * szStringKey2, PFN_DIALOG_CALLBACK pfnCallback)
{
	UIShowConfirmDialog(StringTableGetStringByKey(szStringKey1), StringTableGetStringByKey(szStringKey2), pfnCallback);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowConfirmDialog(LPCWSTR wszString1, LPCWSTR wszString2, PFN_DIALOG_CALLBACK pfnCallback)
{
	// setup callback
	DIALOG_CALLBACK tCallback;
	DialogCallbackInit( tCallback );
	tCallback.pfnCallback = pfnCallback;

	UIShowGenericDialog(wszString1, wszString2, TRUE, &tCallback, 0, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIHideGenericDialog( void )
{
	BOOL bWasActive( FALSE );
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
	if (UIComponentGetActive(pDialog))
	{
		if( AppIsTugboat() )
		{
			UIRemoveMouseOverrideComponent(pDialog);
			UIRemoveKeyboardOverrideComponent(pDialog);
		}
		bWasActive = TRUE;
		const DIALOG_CALLBACK &tCancelCallback = pDialog->m_tCallbackCancel;
		if (tCancelCallback.pfnCallback)
		{
			tCancelCallback.pfnCallback(tCancelCallback.pCallbackData, tCancelCallback.dwCallbackData);
		}
	}

	UIComponentSetVisible(pDialog, FALSE);
	return bWasActive;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGenericDialogOnOK(
								  UI_COMPONENT* component,
								  int msg,
								  DWORD wParam,
								  DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		BOOL bOk = TRUE;
		if (msg == UIMSG_KEYCHAR)
		{
			bOk = FALSE;
			if (wParam == VK_RETURN)
			{
				bOk = TRUE;
			}
			else if (wParam == L' ' || wParam == VK_ESCAPE)
			{
				// if the accept button is the only one shown, handle ESC as accept
				UI_COMPONENT *pCancelButton = UIComponentFindChildByName( component->m_pParent, "button dialog cancel" );
				if ( pCancelButton && !UIComponentGetVisible(pCancelButton))
				{
					bOk = TRUE;
				}
			}
			if( AppIsHellgate() )
			{
				InputKeyIgnore(1000);	// block key input for 1 second
			}
		}

		if (bOk)
		{
			UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
			UIComponentActivate(pDialog, FALSE);	
			if( AppIsTugboat() )
			{
				UIRemoveMouseOverrideComponent(pDialog);
				UIRemoveKeyboardOverrideComponent(pDialog);
			}
			const DIALOG_CALLBACK &tCallback = component->m_pParent->m_tCallbackOK;
			if (tCallback.pfnCallback)
			{
				tCallback.pfnCallback(tCallback.pCallbackData, tCallback.dwCallbackData);
			}
			return UIMSG_RET_HANDLED_END_PROCESS;  
		}

	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIGenericDialogOnCancel(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		if ((msg == UIMSG_KEYCHAR) &&
			(wParam != L' ' && wParam != VK_ESCAPE))
		{
			return UIMSG_RET_NOT_HANDLED;  
		}

		if( AppIsHellgate() )
		{
			InputKeyIgnore(1000);	// block key input for 1 second
		}
		UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
		UIComponentActivate(pDialog, FALSE);	
		if( AppIsTugboat() )
		{
			UIRemoveMouseOverrideComponent(pDialog);
			UIRemoveKeyboardOverrideComponent(pDialog);
		}
		const DIALOG_CALLBACK &tCallback = component->m_pParent->m_tCallbackCancel;
		if (tCallback.pfnCallback)
		{
			tCallback.pfnCallback(tCallback.pCallbackData, tCallback.dwCallbackData);
		}
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInputOverridePushHead(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	if( AppIsHellgate() || component->m_dwParam == 1 || component->m_dwParam2 == 1 )
	{
		UISetMouseOverrideComponent(component);
		UISetKeyboardOverrideComponent(component);
	}

	return UIMSG_RET_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInputOverrideRemove(
									UI_COMPONENT* component,
									int msg,
									DWORD wParam,
									DWORD lParam)
{
	UIRemoveMouseOverrideComponent(component);
	UIRemoveKeyboardOverrideComponent(component);

	return UIMSG_RET_HANDLED;  
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInputOverridePushTail(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	if( AppIsHellgate() || component->m_dwParam == 1 || component->m_dwParam2 == 1 )
	{
		UISetMouseOverrideComponent(component, TRUE);
		UISetKeyboardOverrideComponent(component, TRUE);
	}

	return UIMSG_RET_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDeathDialogOnOK(
								UI_COMPONENT* component,
								int msg,
								DWORD wParam,
								DWORD lParam)
{
	if (UIComponentGetActive(component))
	{

		UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_RESTART);
		UIComponentSetVisible(pDialog, FALSE);

		GAME* game = AppGetCltGame();
		ASSERT_RETVAL(game, UIMSG_RET_NOT_HANDLED);
		UNIT* unit = GameGetControlUnit(game);
		ASSERT_RETVAL(unit, UIMSG_RET_NOT_HANDLED);

		if (!IsUnitDeadOrDying(unit))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		c_PlayerSendRespawn( PLAYER_RESPAWN_INVALID );	

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static 
DWORD sGetCharacterSelectColorForUnit(
									  UNIT * pUnit,
									  int nPlayerSlot,
									  int nMaxActivePlayerSlot )
{
	DWORD dwColor;
	if ( nPlayerSlot > nMaxActivePlayerSlot )
		dwColor = CHARACTER_DISABLED_COLOR;
	else if ( PlayerIsHardcore( pUnit ) )
		dwColor = CHARACTER_HARDCORE_COLOR;
	else
		dwColor = CHARACTER_DEFAULT_COLOR;
	return dwColor;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdateCharSelectButtons()
{
	UI_COMPONENT *pScreen = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT);
	ASSERT_RETURN(pScreen);
	UI_COMPONENT *pMainPanel = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_PANEL);
	ASSERT_RETURN(pMainPanel);


	UI_COMPONENT *pDeleteButton = UIComponentFindChildByName(pMainPanel, "character delete button");
	if (pDeleteButton)
	{
		UIComponentSetActive(pDeleteButton, FALSE);
	}

	int nMaxPossiblePlayerSlots = sGetMaxPossiblePlayerSlots();

	// if we're scrolled down too far, like because a character was deleted, we need to scroll up
	// count the characters
	int nTotalChars = 0;
	for ( int i = 0; i < MAX_NAME_LIST; i++ )
	{
		if ( gppCharSelectUnits[ i ] )
		{
			nTotalChars++;
		}
	}

	BOOL bNeedCreateButtonAtEnd = ( nMaxPossiblePlayerSlots != nTotalChars );

	BOOL bScrolledToBottom;
	{ // compute both bScrolledToBottom and limit gnCharSelectScrollPos
		int nScrollLimit = 0;

		nScrollLimit = nTotalChars - gnNumCharButtons + (bNeedCreateButtonAtEnd ? 1 : 0);
		nScrollLimit = MAX( nScrollLimit, 0 );
		gnCharSelectScrollPos = MIN( gnCharSelectScrollPos, nScrollLimit );
		gnCharSelectScrollPos = MAX( gnCharSelectScrollPos, 0 );
		bScrolledToBottom = (gnCharSelectScrollPos == nScrollLimit);
	}

	int nCharacterButtonCurr = 0;
	int nLastCreateButton = 0;
	int nSkippedCount = 0;

	for ( int i = 0; i < MAX_NAME_LIST && nCharacterButtonCurr < gnNumCharButtons; i++ )
	{
		if ( gppCharSelectUnits[ i ] )
		{
			if ( nSkippedCount < gnCharSelectScrollPos )
			{
				nSkippedCount++;
			} else {
				gnCharButtonToPlayerSlot[ nCharacterButtonCurr ] = i;
				nCharacterButtonCurr++;
				nLastCreateButton++;
			}
		}

	}

	for ( ; nCharacterButtonCurr < gnNumCharButtons; nCharacterButtonCurr++ )
	{
		gnCharButtonToPlayerSlot[ nCharacterButtonCurr ] = -1;
	}

	if( bNeedCreateButtonAtEnd )
	{
		gnCharButtonToPlayerSlot[ nCharacterButtonCurr - 1 ] = -1; // always have a create character button
	}

	pMainPanel = UIComponentFindChildByName(pMainPanel, "character panel 1");
	ASSERTX( pMainPanel, "Character panel not found in character select\n" );
	pMainPanel = pMainPanel->m_pParent;

	BOOL bCreateButtonVisibleAtEnd = FALSE;

	UI_COMPONENT *pChild = pMainPanel->m_pFirstChild;
	char pszCharacterPanelName[] = "character panel";
	int nCharacterPanelNameLen = PStrLen(pszCharacterPanelName);
	while (pChild)
	{
		if (UIComponentIsPanel(pChild) && 
			PStrICmp(pszCharacterPanelName, pChild->m_szName, nCharacterPanelNameLen) == 0)
		{
			int nCharacterButton = (int)pChild->m_dwParam;
			int nPlayerSlot = gnCharButtonToPlayerSlot[ nCharacterButton ];
			UNIT *pUnit = NULL;
			if ( nPlayerSlot < MAX_NAME_LIST && nPlayerSlot >= 0 )
			{
				pUnit = gppCharSelectUnits[nPlayerSlot];
			}

			UIComponentSetVisible(pChild, TRUE );
			UIComponentSetActive( pChild, TRUE );

			UI_COMPONENT * pSelectPanel = UIComponentFindChildByName(pChild, "select panel");
			ASSERT_RETURN( pSelectPanel );
			UI_COMPONENT * pCreatePanel = UIComponentFindChildByName(pChild, "create panel");
			ASSERT_RETURN( pCreatePanel );

			UIComponentSetVisible(pSelectPanel, pUnit != NULL);
			UIComponentSetActive( pSelectPanel, pUnit != NULL);

			if ( ! pUnit )
			{
				if ( (bCreateButtonVisibleAtEnd && AppGetType() == APP_TYPE_SINGLE_PLAYER) ||
					nCharacterButton > nLastCreateButton ) 
				{
					UIComponentSetVisible(pCreatePanel, FALSE );
					UIComponentSetActive( pCreatePanel, FALSE );
				} else {
					UIComponentSetVisible(pCreatePanel, TRUE );
					UIComponentSetActive( pCreatePanel, TRUE );
					if ( nCharacterButton == nLastCreateButton )
						bCreateButtonVisibleAtEnd = TRUE;
				}
			}
			else
			{
				/*if((AppIsSinglePlayer() )
					&& (PlayerIsHardcore(pUnit) || PlayerIsElite(pUnit)))
					UIComponentSetActive(pSelectPanel, FALSE);*/
				UIComponentSetVisible(pCreatePanel, FALSE );
				UIComponentSetActive( pCreatePanel, FALSE );

				UIComponentSetFocusUnit(pChild, UnitGetId(pUnit) );

				UI_COMPONENT *pNameLabel = UIComponentFindChildByName(pChild, "character name label");
				if (pNameLabel)
				{
					const UNIT_DATA *pUnitData = UnitGetData(pUnit);

					if (pUnitData->szCharSelectFont[0])
					{
						UIX_TEXTURE *pFontTexture = UIGetFontTexture(LANGUAGE_CURRENT);
						UIX_TEXTURE_FONT *pFont = (UIX_TEXTURE_FONT*)StrDictionaryFind(pFontTexture->m_pFonts, pUnitData->szCharSelectFont);
						if (pFont)
						{
							pNameLabel->m_pFont = pFont;
						}
					}

					UILabelSetText(pNameLabel, gpszCharSelectNames[nPlayerSlot]);

					if (nPlayerSlot == g_UI_StartgameParams.m_nPlayerSlot)
					{
						if (pDeleteButton)
						{
							UIComponentSetActive(pDeleteButton, TRUE);
						}
						// start with the first one selected by default
						PStrCopy(g_UI_StartgameParams.m_szPlayerName, gpszCharSelectNames[nPlayerSlot], 256);
						trace("Setting name to %ls\n", g_UI_StartgameParams.m_szPlayerName);
					}
				}

				UI_COMPONENT * pButtonComponent = UIComponentFindChildByName(pChild, "character select button");
				UI_BUTTONEX *pButton = pButtonComponent ? UICastToButton(pButtonComponent) : NULL;
				if (pButton)
				{
					UIButtonSetDown(pButton, nPlayerSlot == g_UI_StartgameParams.m_nPlayerSlot);
					UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);

					if( AppIsHellgate() )
					{
						pButtonComponent->m_dwColor = sGetCharacterSelectColorForUnit( pUnit, nPlayerSlot, nMaxPossiblePlayerSlots );
					}

					UI_COMPONENT *pDelBtn = UIComponentFindChildByName(pButton, "character delete btn");
					if (pDelBtn)
					{
						UIComponentSetActive(pDelBtn, TRUE);
					}
				}

				UI_COMPONENT * pBackPanel = UIComponentFindChildByName(pChild, "character select portrait backing");
				if ( pBackPanel )
				{
					pBackPanel->m_dwColor = sGetCharacterSelectColorForUnit( pUnit, nPlayerSlot, nMaxPossiblePlayerSlots );
				}

				UI_COMPONENT *pClassLabel = UIComponentFindChildByName(pChild, "character class label");
				if (pClassLabel)
				{
					WCHAR szClass[256], szLevel[256], szBuf[256];

					UnitGetTypeString(pUnit, szClass, 256, TRUE, NULL);

					int nLevel = UnitGetExperienceLevel(pUnit);
					LanguageFormatIntString(szLevel, arrsize(szLevel), nLevel);

					// Hey! Remember that Mythos is here! We use this stuff too!
					if( AppIsTugboat() )
					{

						PStrPrintf(szBuf, 256, GlobalStringGet(GS_CHAR_SELECT_CLASS_FMT), nLevel, szClass);
					}
					else
					{
						int nRank = UnitGetExperienceRank(pUnit);
						if (nRank)
						{
							WCHAR szRank[256];
							LanguageFormatIntString(szRank, arrsize(szRank), nRank);
							PStrCopy(szBuf, GlobalStringGet(GS_CHAR_SELECT_CLASS_FMT_WITH_RANK), arrsize(szBuf));
							PStrReplaceToken( szBuf, arrsize(szBuf), StringReplacementTokensGet( SR_LEVEL ), szLevel );
							PStrReplaceToken( szBuf, arrsize(szBuf), StringReplacementTokensGet( SR_RANK ), szRank );
							PStrReplaceToken( szBuf, arrsize(szBuf), StringReplacementTokensGet( SR_PLAYER_CLASS ), szClass );
						}
						else
						{
							PStrCopy(szBuf, GlobalStringGet(GS_CHAR_SELECT_CLASS_FMT), arrsize(szBuf));
							PStrReplaceToken( szBuf, arrsize(szBuf), StringReplacementTokensGet( SR_LEVEL ), szLevel );
							PStrReplaceToken( szBuf, arrsize(szBuf), StringReplacementTokensGet( SR_PLAYER_CLASS ), szClass );
						}
					}


					UILabelSetText(pClassLabel, szBuf);
				}

				UI_COMPONENT * pPortraitModel	= UIComponentFindChildByName(pChild, "character select portrait model");
				UI_COMPONENT * pHardcore		= UIComponentFindChildByName(pChild, "character select portrait hardcore icon");
				UI_COMPONENT * pHardcoreDead	= UIComponentFindChildByName(pChild, "character select portrait dead hardcore");
				UI_COMPONENT * pEliteMode		= UIComponentFindChildByName(pChild, "character select portrait elite icon");
				UI_COMPONENT * pPVPOnly			= UIComponentFindChildByName(pChild, "character select portrait pvponly icon");

				BOOL bIsHardcoreDead = PlayerIsHardcoreDead( pUnit );
				if ( pPortraitModel )
				{
					UIComponentSetActive( pPortraitModel, !bIsHardcoreDead );
					UIComponentSetVisible( pPortraitModel, !bIsHardcoreDead );
				}
				if ( pHardcoreDead )
				{
					UIComponentSetActive( pHardcoreDead, bIsHardcoreDead );
					UIComponentSetVisible( pHardcoreDead, bIsHardcoreDead );
				}
				BOOL bIsHardcore = PlayerIsHardcore( pUnit );
				if ( pHardcore )
				{
					UIComponentSetActive( pHardcore, bIsHardcore );
					UIComponentSetVisible( pHardcore, bIsHardcore );
				}

				BOOL bIsPVPOnly = PlayerIsPVPOnly( pUnit );
				if ( pPVPOnly )
				{
					UIComponentSetActive( pPVPOnly, bIsPVPOnly );
					UIComponentSetVisible( pPVPOnly, bIsPVPOnly );
				}

				BOOL bIsElite = PlayerIsElite(pUnit);
				if( pEliteMode )
				{
					UIComponentSetActive( pEliteMode, bIsElite );
					UIComponentSetVisible( pEliteMode, bIsElite );
				}

			} 
		}
		pChild = pChild->m_pNextSibling;
	}

	UI_COMPONENT *pUpButton = UIComponentFindChildByName(pMainPanel, "character select up btn");
	if (pUpButton)
	{
		if (gnCharSelectScrollPos > 0)
		{
			UIComponentSetActive(pUpButton, TRUE);
		}
		else
		{
			UIComponentSetVisible(pUpButton, FALSE);
		}
	}

	UI_COMPONENT *pDownButton = UIComponentFindChildByName(pMainPanel, "character select down btn");
	if (pDownButton)
	{
		if ( !bScrolledToBottom )
		{
			UIComponentSetActive(pDownButton, TRUE);
		}
		else
		{
			UIComponentSetVisible(pDownButton, FALSE);
		}
	}

	UIComponentHandleUIMessage(pScreen, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectButtonOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT * pCharacterPanel = UIComponentFindParentByName( component, "character panel", TRUE );
	ASSERT_RETVAL(pCharacterPanel, UIMSG_RET_NOT_HANDLED);

	int nButton = (int)pCharacterPanel->m_dwParam;

	sSelectCharacterButton( nButton, TRUE );

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectOnKeyDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (wParam == VK_DOWN || wParam == VK_UP)
	{
		int nDelta = (wParam == VK_DOWN ? 1 : -1);

		for ( int nCharacterButton = g_UI_StartgameParams.m_nCharacterButton + nDelta; 
			nCharacterButton >= 0 && nCharacterButton < gnNumCharButtons;
			nCharacterButton += nDelta )
		{
			int nPlayerSlot = gnCharButtonToPlayerSlot[ nCharacterButton ];
			if ( nPlayerSlot == INVALID_ID || ! gppCharSelectUnits[ nPlayerSlot ] )
				continue;

			sSelectCharacterButton( nCharacterButton, FALSE );
			break;
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UICharacterCreateOnKeyDown(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sCharSelectLevelsAllLoaded()
{
	for ( int i = 0; i < NUM_CHAR_SELECT_LEVELS; i++ )
	{
		ASSERT( sgtCharSelectLevels[ i ].eLoadState != CSLL_STATE_INIT );
		if ( sgtCharSelectLevels[ i ].eLoadState != CSLL_STATE_COMPLETE )
			return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------

BOOL sCharSelectUnitsReady()
{
	for (int i=0; i < MAX_NAME_LIST; i++)
	{
		UNIT * pUnit = gppCharSelectUnits[ i ];
		if ( pUnit )
		{
			int nWardrobe = UnitGetWardrobe( pUnit );
			ASSERT_CONTINUE( nWardrobe != INVALID_ID );
			if ( !c_WardrobeIsUpdated( nWardrobe ) )
				return FALSE;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------

BOOL sCharCreateUnitsReady()
{
	int nNumRaces = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_PLAYER_RACE);
	int nNumCharClasses = CharacterClassNumClasses();
	for (int nCharClass = 0; nCharClass < nNumCharClasses; ++nCharClass)
	{
		for (int nGender = 0; nGender < GENDER_NUM_CHARACTER_GENDERS; nGender++)
		{
			for( int nRace = 0; nRace < nNumRaces; nRace++ )
			{
				if ( CharacterClassIsEnabled( nCharClass, nRace, (GENDER)nGender ) )
				{
					UNIT *pUnit = gpppCharCreateUnits[ nCharClass ][nRace][ nGender ];
					if (pUnit)
					{
						int nWardrobe = UnitGetWardrobe( pUnit );
						ASSERT_CONTINUE( nWardrobe != INVALID_ID );
						if ( !c_WardrobeIsUpdated( nWardrobe ) )
						{
							return FALSE;
						}
					}
				}
			}
		}
	}

	return TRUE;
}
//----------------------------------------------------------------------------

void UICharacterSelectionScreenUpdate()
{
	if ( AppIsHellgate() && UIComponentGetActive( UICOMP_CHARACTER_CREATE ) 
		&& ! gbCharCreateUnitsCreated )
	{
		if ( sCharCreateUnitsReady() )
		{
			gbCharCreateUnitsCreated = TRUE;
			UIHideGenericDialog();
		}
		else
			UIShowGenericDialog(GlobalStringGet( GS_LOADING ), StringTableGetStringByKey("ui please wait"), FALSE, NULL, NULL, GDF_NO_OK_BUTTON);
	}
}

//----------------------------------------------------------------------------
// Brennan call this to see whether we are ready to draw the character creation/selection screen
//----------------------------------------------------------------------------

BOOL UICharacterSelectionScreenIsReadyToDraw()
{
	// If the loading screen has already gone down, then we no longer need to
	// hide the character selection/creation screen.
	if ( g_UI.m_eLoadScreenState == LOAD_SCREEN_DOWN )
		return TRUE;

	if ( ! sCharSelectUnitsReady() )
		return FALSE;

	if ( ! sCharCreateUnitsReady() )
		return FALSE;

	if (!AppIsTugboat())
	{
		if ( ! e_GetCurrentEnvironmentDef() )
			return FALSE;

		// check that all levels have finished loading
		if ( ! sCharSelectLevelsAllLoaded() )
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


static PRESULT sCharSelectLevelLoadedCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	WAIT_FOR_LOAD_CALLBACK_DATA *pCallbackData = (WAIT_FOR_LOAD_CALLBACK_DATA *)spec->callbackData;
	ASSERT_RETFAIL(pCallbackData);

	sgtCharSelectLevels[ pCallbackData->nLevelDefinition ].eLoadState = CSLL_STATE_COMPLETE;

	// if all levels are done loading
	if ( ! sCharSelectLevelsAllLoaded() )
		return S_OK;

	// select the first unit
	int nMaxPlayerSlot = sGetLastUsedPlayerSlot();
	if (nMaxPlayerSlot >= 0)
	{
		sSelectMinUsedPlayerSlot();
	}
	else
	{
		sSwitchToCreateUnit( FALSE, TRUE );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pRatingsLogo = UIComponentFindChildByName(component, "rating logo");
	if (pRatingsLogo)
	{
		UIComponentActivate(pRatingsLogo, TRUE);
	}

	if (!AppIsTugboat())
	{
		UICharacterCreateReset();

		// start loading level data
		sCharacterSelectCreateLevels();

		// go to the character select panel for the rest
		component = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_PANEL);
		UIComponentSetVisible(component, TRUE);
		component->m_Position = component->m_ActivePos;

		UI_COMPONENT *pCharCreate = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE);
		if (pCharCreate)
		{
			UIComponentSetVisible(pCharCreate, FALSE);
			pCharCreate->m_Position = pCharCreate->m_InactivePos;
		}

	}
	else
	{
		if (component->m_pParent)
		{
			UI_COMPONENT *pStartButton = UIComponentFindChildByName(component->m_pParent, "character select accept");
			if (pStartButton)
			{
				UIComponentSetActive(pStartButton, TRUE);
			}
		}
		GAME* pGame = AppGetCltGame();
		UNIT * pUnit = sGetCharCreateUnit();
		if ( pUnit )
		{
			sFreeCharacterCreateOrSelectUnit( pGame, pUnit );
		}
		g_UI_StartgameParams.m_idCharCreateUnit = INVALID_ID;
		structclear(gpppCharCreateUnits);
		UICharacterCreateInit();
	}

	if (AppGetType() != APP_TYPE_CLOSED_CLIENT &&
		AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		//sFillNameList();

		for (int i=0; i < MAX_NAME_LIST; i++)
		{
			if ( ! gppCharSelectUnits[i] && gpszCharSelectNames[ i ][ 0 ] != 0 )
			{
				CLIENT_SAVE_FILE tClientSaveFile;
				PStrCopy( tClientSaveFile.uszCharName, gpszCharSelectNames[ i ], MAX_CHARACTER_NAME );
				tClientSaveFile.eFormat = PSF_HIERARCHY;
				tClientSaveFile.pBuffer = NULL;
				tClientSaveFile.dwBufferSize = 0;
				DWORD dwPlayerLoadFlags = 0;
				SETBIT( dwPlayerLoadFlags, PLF_FOR_CHARACTER_SELECTION );
				gppCharSelectUnits[i] = PlayerLoad(AppGetCltGame(), &tClientSaveFile, dwPlayerLoadFlags, INVALID_GAMECLIENTID);
				if ( ! gppCharSelectUnits[i] )
					continue;
				sCharacterSelectMakePaperdollModel(gppCharSelectUnits[i]);
			}
		}
	}

	UI_COMPONENT *pBtnPanel = UIComponentFindChildByName(component->m_pParent, "extra buttons panel singleplayer");
	if (pBtnPanel)
	{
		UIComponentActivate(pBtnPanel, AppGetType() != APP_TYPE_CLOSED_CLIENT);
	}

	pBtnPanel = UIComponentFindChildByName(component->m_pParent, "extra buttons panel multiplayer");
	if (pBtnPanel)
	{
		UIComponentActivate(pBtnPanel, AppGetType() == APP_TYPE_CLOSED_CLIENT);
	}

	gnNumCharButtons = 0;
	gnCharSelectScrollPos = 0;
	UI_COMPONENT *pChild = 	 UIComponentFindChildByName(component, "character panel 1");
	ASSERTX( pChild, "Character panel not found in character select\n" );



	while (pChild)
	{
		if (UIComponentIsPanel(pChild))
		{
			//int i = pChild->m_dwParam;
			//if ((int)pChild->m_dwParam < gnNumCharSelectNames)
			{
				gnNumCharButtons++;
			}
		}
		pChild = pChild->m_pNextSibling;
	}

	g_UI_StartgameParams.m_nPlayerSlot = -1;
	g_UI_StartgameParams.m_nCharacterButton = -1;
	sUpdateCharSelectButtons();

	// enable rendering of the background for the character selection screen
	e_SetUIOnlyRender( FALSE );
	c_CameraSetViewType( VIEW_CHARACTER_SCREEN, AppIsTugboat() );

	if (!AppIsTugboat())
	{
		for ( int i = 0; i < NUM_CHAR_SELECT_LEVELS; i++ )
		{
			g_UI_StartgameParams.m_bNewPlayer = FALSE;
			c_LevelWaitForLoaded( AppGetCltGame(), i, -1, sCharSelectLevelLoadedCallback );
		}

		// if we're single or multi-player only, there's no need for the back button
		UI_COMPONENT *pBackButton = UIComponentFindChildByName(component, "character select cancel");
		if (pBackButton)
		{
			if (!AppIsDemo() &&
				((AppCommonMultiplayerOnly() && AppGetType() != APP_TYPE_CLOSED_CLIENT) 
				|| AppCommonSingleplayerOnly()))
				UIComponentSetVisible(pBackButton, FALSE);
			else
				UIComponentSetActive(pBackButton, TRUE);
		}
	}

	// clear the create new character name edit
	UI_COMPONENT *pCharCreate = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE);
	if (pCharCreate)
	{
		pChild = UIComponentFindChildByName(pCharCreate, "character create name");
		if (pChild)
		{
			UILabelSetText(pChild, L"");
		}
	}

	if ( ! sSelectMinUsedPlayerSlot() )
	{
		sSwitchToCreateUnit( FALSE, TRUE );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	{
		// go to the character select panel for the rest
		component = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT_PANEL);
	}

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_bIndependentActivate)
		{
			UIComponentSetVisible(pChild, FALSE);
		}
		pChild = pChild->m_pNextSibling;
	}

	// Bad news for Mythos! This will cause a crash when switching races/classes in the charcreate
	// don't know if it happens for Hellgate?(no room on this laptop to synch and find out )
	if( AppIsHellgate() )
	{
		CharCreateResetUnitLists();
	}
	
	UIComponentActivate(UICOMP_CHARACTER_CREATE, FALSE);
	UIHideGenericDialog();

	UI_COMPONENT *pCharSelect = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT);
	ASSERT_RETVAL(pCharSelect, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pTimeRemainPanel = UIComponentFindChildByName(pCharSelect, "account time remaining panel");
	if (pTimeRemainPanel)
	{
		UIComponentSetVisible(pTimeRemainPanel, FALSE);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFrontEndOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (!AppIsTugboat())
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	/*	GAME * pGame = AppGetCltGame();
	UNIT *pUnit = sGetCharCreateUnit();
	if ( pUnit )
	{
	sFreeCharacterCreateOrSelectUnit( pUnit );
	}
	UICharacterCreateInit();


	// enable rendering of the background for the character selection screen
	e_SetUIOnlyRender( FALSE );
	c_CameraSetViewType( VIEW_CHARACTER_SCREEN, TRUE );

	UNIT *pCreateUnit = sGetCharCreateUnit();
	ASSERT_RETVAL(pCreateUnit, UIMSG_RET_NOT_HANDLED);
	sCharacterSelectUnit(pCreateUnit);
	*/
	// only show singleplayer in non-clientonly test builds.
	UI_COMPONENT* pSPButton = UIComponentFindChildByName(component, "button singleplayer");
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UIComponentSetActive(pSPButton, TRUE);
#else
	UIComponentSetActive(pSPButton, FALSE);
	UIComponentSetVisible(pSPButton, FALSE);
#endif
	UNIT* pCreateUnit = gpppCharCreateUnits[g_UI_StartgameParams.m_nCharacterClass][g_UI_StartgameParams.m_nCharacterRace][g_UI_StartgameParams.m_eGender];
	if (pCreateUnit)
	{

		sCharacterSelectUnit(pCreateUnit, TRUE);
		// enable rendering of the background for the character selection screen
		e_SetUIOnlyRender( FALSE );
		c_CameraSetViewType( VIEW_CHARACTER_SCREEN, TRUE );
	}
	else
	{
		int nLevelId = c_LevelCreatePaperdollLevel( 0 );

#if !ISVERSION(SERVER_VERSION)
		AppSetCurrentLevel( nLevelId );
#endif
		e_SetUIOnlyRender( FALSE );
		c_CameraSetViewType( VIEW_CHARACTER_SCREEN, TRUE );

		/*
		UICharacterCreateInit();
		// enable rendering of the background for the character selection screen
		e_SetUIOnlyRender( FALSE );
		c_CameraSetViewType( VIEW_CHARACTER_SCREEN, TRUE );

		UNIT *pCreateUnit = sGetCharCreateUnit();
		ASSERT_RETVAL(pCreateUnit, UIMSG_RET_NOT_HANDLED);
		sCharacterSelectUnit(pCreateUnit);*/
	}
	return UIMSG_RET_HANDLED;
	//	return( UICharacterSelectOnPostActivate( component, msg, wParam, lParam ) );
	//sCharacterCreateMakePaperdollModel( CharacterClassGetDefault(), GENDER_MALE );
	//return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterCreateOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);


	if (component->m_pParent)
	{
		UI_COMPONENT *pStartButton = UIComponentFindChildByName(component->m_pParent, "character select accept");
		if (pStartButton)
		{
			if (msg == UIMSG_POSTINACTIVATE)
			{
				UIComponentSetActive(pStartButton, TRUE);
			}
			else
			{
				UIComponentSetVisible(pStartButton, FALSE);
			}
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSelectScrollOnClick(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	int nScroll = 1;
	if (component->m_dwParam == 0)
	{
		nScroll = -1;
	}

	gnCharSelectScrollPos += nScroll; // we limit gnCharSelectScrollPos to the proper range within sUpdateCharSelectButtons

	sUpdateCharSelectButtons();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL OnCharacterDeleteClick(	
									 UI_COMPONENT* component,
									 int msg,
									 DWORD wParam,
									 DWORD lParam)
{
	UI_COMPONENT *pConfirmDlg = UIComponentGetByEnum(UICOMP_CHAR_DELETE_DIALOG);
	if (pConfirmDlg)
	{
		ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(component->m_pParent->m_pParent, UIMSG_RET_NOT_HANDLED);

		int nIndex = g_UI_StartgameParams.m_nCharacterButton;

		UI_COMPONENT * pCharacterComponent = UIComponentFindParentByName( component, "character panel ", TRUE );
		if ( pCharacterComponent )
		{
			nIndex = (int)pCharacterComponent->m_dwParam;
			sSelectCharacterButton( nIndex, FALSE );
		}

		if (nIndex >= 0 && nIndex < gnNumCharButtons)
		{
			pConfirmDlg->m_dwParam = (DWORD)nIndex;
			UI_COMPONENT *pNameEdit = UIComponentFindChildByName(pConfirmDlg, "character delete name");
			UILabelSetText(pNameEdit, L"");

			UITooltipDetermineContents(pConfirmDlg);
			UITooltipDetermineSize(pConfirmDlg);

			// Center the dialog
			if( AppIsHellgate( ) )
			{
				float fAppWidth = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX();
				float fAppHeight = AppCommonGetWindowHeight() * UIGetScreenToLogRatioY();
				pConfirmDlg->m_Position.m_fX = (fAppWidth - pConfirmDlg->m_fWidth) / 2.0f;
				pConfirmDlg->m_Position.m_fY = (fAppHeight - pConfirmDlg->m_fHeight) / 2.0f;
			}


			UIComponentSetActive(pConfirmDlg, TRUE);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDeleteLocalCharacter(
								  const WCHAR * strPlayerName)
{
	ASSERT_RETFALSE(strPlayerName && strPlayerName[0]);

	OS_PATH_CHAR playername[MAX_PATH];
	PStrCvt(playername, strPlayerName, arrsize(playername));

	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, arrsize(filename), OS_PATH_TEXT("%s%s"), FilePath_GetSystemPath(FILE_PATH_SAVE), playername);
	FileDeleteDirectory(filename, TRUE);

	//	PStrPrintf(filename, arrsize(filename), OS_PATH_TEXT("%s%s.hg1"), FilePath_GetSystemPath(FILE_PATH_SAVE), playername);
	PStrCat(filename, OS_PATH_TEXT(".hg1"), arrsize(filename));
	FileDelete(filename);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDeleteServerCharacter(
   const WCHAR * strPlayerName)
{
	ASSERT_RETFALSE(strPlayerName && strPlayerName[0]);

	extern void CharacterSelectDeleteCharName(const WCHAR *);
	CharacterSelectDeleteCharName(strPlayerName);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL OnCharacterDeleteOnOk(	
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!component || !UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (msg == UIMSG_LBUTTONDOWN && !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (msg == UIMSG_KEYDOWN && wParam != VK_RETURN)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pConfirmDlg = UIComponentGetByEnum(UICOMP_CHAR_DELETE_DIALOG);
	if (pConfirmDlg)
	{
		UI_COMPONENT *pComponent = UIComponentFindChildByName(pConfirmDlg, "character delete name");
		if (pComponent)
		{
			UI_EDITCTRL *pNameEdit = UICastToEditCtrl(pComponent);
			int nIndex = (int)pConfirmDlg->m_dwParam;

			if (nIndex >= 0 && nIndex < gnNumCharButtons && pNameEdit->m_Line.HasText())
			{
				int nPlayerSlot = gnCharButtonToPlayerSlot[ nIndex ];
				ASSERT_RETVAL( nPlayerSlot >= 0, UIMSG_RET_HANDLED_END_PROCESS );
				ASSERT_RETVAL( nPlayerSlot < MAX_NAME_LIST, UIMSG_RET_HANDLED_END_PROCESS );
				if (PStrICmp(pNameEdit->m_Line.GetText(), gpszCharSelectNames[nPlayerSlot]) == 0)
				{
					if(AppGetType() != APP_TYPE_CLOSED_CLIENT)
					{
						ASSERT_RETVAL(sDeleteLocalCharacter(gpszCharSelectNames[nPlayerSlot]), UIMSG_RET_NOT_HANDLED);
					}
					else
					{
						ASSERT_RETVAL(sDeleteServerCharacter(gpszCharSelectNames[nPlayerSlot]), UIMSG_RET_NOT_HANDLED);
					}

					ASSERT_RETVAL(component->m_pParent, UIMSG_RET_NOT_HANDLED);
					ASSERT_RETVAL(component->m_pParent->m_pParent, UIMSG_RET_NOT_HANDLED);

					UIComponentSetVisible(UIComponentGetByEnum(UICOMP_CHAR_DELETE_DIALOG), FALSE);

					//sFillNameList();
					if(AppGetType() != APP_TYPE_CLOSED_CLIENT)
					{
						sRemoveFromNameList(nPlayerSlot);
					}
					else
					{
						sClearFromNameList(nPlayerSlot);
					}

					sUpdateCharSelectButtons();

					g_UI_StartgameParams.m_nPlayerSlot = -1;		// the old one should be gone now, so re-initialize it.
					if ( ! sSelectMinUsedPlayerSlot() )
					{
						sSwitchToCreateUnit();
					}

					UIComponentSetVisible(pConfirmDlg, FALSE);

					return UIMSG_RET_HANDLED_END_PROCESS;
				}
				else
				{
					UIShowGenericDialog(StringTableGetStringByKey("ui delete character"), StringTableGetStringByKey("ui name does not match"));
				}
			}

		}
	}

	return UIMSG_RET_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL OnCharacterDeleteOnCancel(	
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	if (!component || !UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (msg == UIMSG_LBUTTONDOWN && !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (msg == UIMSG_KEYDOWN && wParam != VK_ESCAPE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pConfirmDlg = UIComponentGetByEnum(UICOMP_CHAR_DELETE_DIALOG);
	if (pConfirmDlg)
	{
		UIComponentSetVisible(pConfirmDlg, FALSE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILoadingBarAnimate(	
								  UI_COMPONENT* component,
								  int msg,
								  DWORD wParam,
								  DWORD lParam)
{
	UIComponentRemoveAllElements(component);
	if (!component || !UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_RECT cliprect(0.0f, 0.0f, component->m_fWidth, component->m_fHeight);
	UI_POSITION pos((float) component->m_dwData, 0.0f);
	UIComponentAddElement(component, UIComponentGetTexture(component), component->m_pFrame, pos, component->m_dwColor, &cliprect);
	pos.m_fX -= component->m_pFrame->m_fWidth;
	UIComponentAddElement(component, UIComponentGetTexture(component), component->m_pFrame, pos, component->m_dwColor, &cliprect);


	component->m_dwData += 10;
	if (component->m_dwData >= component->m_pFrame->m_fWidth)
	{
		component->m_dwData = 0;
	}

	//Schedule another
	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	// schedule another one for one second later
	component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + component->m_dwAnimDuration, component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILoadingCircleAnimate(	
									 UI_COMPONENT* component,
									 int msg,
									 DWORD wParam,
									 DWORD lParam)
{
	UIComponentRemoveAllElements(component);
	if (!component || !UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_RECT cliprect(0.0f, 0.0f, component->m_fWidth, component->m_fHeight);

	float fAngle = (float) component->m_dwData / 100.0f;
	DWORD dwRotationFlags = 0;
	SETBIT(dwRotationFlags, ROTATION_FLAG_ABOUT_SELF_BIT);
	UIComponentAddRotatedElement(component, UIComponentGetTexture(component), component->m_pFrame, UI_POSITION(), component->m_dwColor, fAngle, UI_POSITION(), &cliprect, FALSE, 1.0f, dwRotationFlags);

	fAngle += 0.1f;
	if (fAngle > TWOxPI)
		fAngle -= TWOxPI;

	component->m_dwData = (DWORD)(fAngle * 100.0f);

	//Schedule another
	if (component->m_tThrobAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tThrobAnimInfo.m_dwAnimTicket);
	}

	// schedule another one for one second later
	component->m_tThrobAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + component->m_dwAnimDuration, component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_MultiplayerCharCountHandler(
									short nCount)
{
	if (g_UI_StartgameParams.m_bSinglePlayer)
	{
		return;
	}

	//ASSERT_RETURN( nCount < MAX_NAME_LIST );

	if(nCount != 0)
		g_UI_StartgameParams.dwNewPlayerFlags |= NPF_DISABLE_TUTORIAL;
	else
		g_UI_StartgameParams.dwNewPlayerFlags &= ~NPF_DISABLE_TUTORIAL;
	//g_UI_StartgameParams.m_bSkipTutorialTips = nCount != 0;
	sUpdateTutorialButton();

	if (nCount == 0)
	{
		sSwitchToCreateUnit();
	}

	TraceDebugOnly("Expecting %d characters in list\n", nCount);
}

//----------------------------------------------------------------------------
// For error handling among other things, find the lowest numbered empty slot
// of a given slot type.
//----------------------------------------------------------------------------
int UI_GetFirstEmptySlotForSlotType(
									CHARACTER_SLOT_TYPE eCharacterSlotType)
{
	for(int i = 0; i < MAX_NAME_LIST; i++)
	{
		if(gppCharSelectUnits[i] == NULL && 
			CharacterSelectGetSlotType(i) == eCharacterSlotType)
			return i;
	}
	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UI_MultiplayerCharListHandler(
								   WCHAR *szCharName,
								   int nPlayerSlot,
								   BYTE *data,
								   DWORD dataLen,
								   WCHAR *szCharGuildName)
{
	if (AppGetState() != APP_STATE_CHARACTER_CREATE)
		return FALSE;

	if (g_UI_StartgameParams.m_bSinglePlayer)
	{
		return FALSE;
	}

	ASSERT_RETFALSE( nPlayerSlot < MAX_NAME_LIST );
	ASSERT_RETFALSE( nPlayerSlot >= 0 );

	ASSERTX_DO( ! gppCharSelectUnits[ nPlayerSlot ],
		"Found existing character in slot.  Moving character to another slot if possible.")
	{//Error handling for redundant slot usage.
		int nPlayerSlotForMove = 
			UI_GetFirstEmptySlotForSlotType(CharacterSelectGetSlotType(nPlayerSlot));
		if(nPlayerSlotForMove != -1)	//-1 means no slots available.  ack!
		{
			gppCharSelectUnits[nPlayerSlotForMove] = gppCharSelectUnits[nPlayerSlot];
			gppCharSelectUnits[nPlayerSlot] = NULL;
		}
	}

	if ( ! gppCharSelectUnits[ nPlayerSlot ] )
	{
		PStrCopy( gpszCharSelectNames[ nPlayerSlot ], szCharName, MAX_CHARACTER_NAME );
		PStrCopy( gpszCharSelectGuildNames[ nPlayerSlot ], szCharGuildName, MAX_GUILD_NAME );

		GAME *pGame = AppGetCltGame();

		// read the unit
		BIT_BUF buf(data, dataLen);
		XFER<XFER_LOAD> tXfer(&buf );
		UNIT_FILE_XFER_SPEC<XFER_LOAD> tSpec(tXfer);
		SETBIT( tSpec.dwReadUnitFlags, RUF_IGNORE_INVENTORY_BIT, TRUE );		
		tSpec.pRoom = NULL;  // nowhere
		gppCharSelectUnits[nPlayerSlot] = UnitFileXfer( pGame, tSpec );

		if (gppCharSelectUnits[nPlayerSlot])
			c_WardrobeMakeClientOnly( gppCharSelectUnits[nPlayerSlot] );

		if (gppCharSelectUnits[nPlayerSlot] == NULL) {
			UIShowGenericDialog(szCharName, L"Can't retrieve character data",0,0,0,GDF_OVERRIDE_EVENTS);
		}
		else
		{
			UnitSetName(gppCharSelectUnits[nPlayerSlot], szCharName);
			sCharacterSelectMakePaperdollModel(gppCharSelectUnits[nPlayerSlot]);
			c_UnitSetNoDraw(gppCharSelectUnits[nPlayerSlot], TRUE, FALSE);
		}
	}

	if( AppIsTugboat() )
	{
		// clear the create new character name edit
		UI_COMPONENT *pCharCreate = UIComponentGetByEnum(UICOMP_CHARACTER_CREATE);
		if (pCharCreate)
		{
			UI_COMPONENT *pChild = UIComponentFindChildByName(pCharCreate, "character create name");
			if (pChild)
			{
				UILabelSetText(pChild, L"");
			}
		}
	}

	sUpdateCharSelectButtons();

	sSelectMinUsedPlayerSlot( TRUE );

	TraceDebugOnly("Received data for characters %ls\n", szCharName);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMultiplayerStartCharacterSelect()
{
	AppSwitchState(APP_STATE_CHARACTER_CREATE);
	UISetCurrentMenu(NULL);

	UIComponentActivate(UICOMP_CHARACTER_SELECT, TRUE);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICharCreateColorPickerShow(
								 UI_COMPONENT *pNextToComponent,
								 int nBodyColorIndex,
								 PFN_COLORPICKER_CALLBACK_FUNC pfnCallback)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_COLOR_PICKER);
	ASSERT_RETURN(pComp);

	UI_COLORPICKER *pPicker = UICastToColorPicker(pComp);

	UNIT * pUnit = sGetCharCreateUnit();
	ASSERT_RETURN( pUnit );
	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETURN( nWardrobe != INVALID_ID );

	if (pPicker->m_pfnCallback)
	{
		DWORD dwColorChosen = 0;
		if (pPicker->m_nSelectedIndex >=0 && pPicker->m_nSelectedIndex < pPicker->m_nGridCols * pPicker->m_nGridRows)
		{
			dwColorChosen = pPicker->m_pdwColors[pPicker->m_nSelectedIndex];
		}
		pPicker->m_pfnCallback( dwColorChosen, pPicker->m_nSelectedIndex, 0, pPicker->m_dwCallbackData);
		pPicker->m_pfnCallback = NULL;
		pPicker->m_dwCallbackData = 0;
	}

	pPicker->m_pfnCallback = pfnCallback;
	pPicker->m_dwCallbackData = (DWORD)nBodyColorIndex;

	if (pNextToComponent)
	{
		pPicker->m_Position = UIComponentGetAbsoluteLogPosition(pNextToComponent);
		pPicker->m_Position.m_fX += pNextToComponent->m_fWidth + pPicker->m_fToolTipXOffset;
		pPicker->m_Position.m_fY += pPicker->m_fToolTipYOffset;
		pPicker->m_ActivePos = pPicker->m_Position;
		pPicker->m_InactivePos = pPicker->m_Position;
	}

	// initialize the colors
	DWORD *pdwColor = pPicker->m_pdwColors;
	for (int i=0; i<pPicker->m_nGridCols; i++)
	{
		for (int j=0; j<pPicker->m_nGridRows; j++)
		{
			DWORD dwColor = 0;
			c_WardrobeGetBodyColor(nBodyColorIndex, UnitGetType(pUnit), (BYTE)((j * pPicker->m_nGridCols) + i), dwColor);

			pdwColor[(j * pPicker->m_nGridCols) + i] = dwColor;
		}
	}

	pPicker->m_nSelectedIndex = pPicker->m_nOriginalIndex = c_WardrobeGetBodyColorIndex(nWardrobe, nBodyColorIndex);

	UIComponentHandleUIMessage(pPicker, UIMSG_PAINT, 0, 0);
	UIComponentSetActive(pPicker, TRUE);
}

static void sUICharCreateColorPickerCallback(
	DWORD dwColorChosen, 
	int nIndexChosen, 
	BOOL bResult,	// 0 - canceled, 1 - accepted, 2 - selected
	DWORD dwCallbackData)
{
	UNIT * pUnit = sGetCharCreateUnit();
	ASSERT_RETURN( pUnit );
	int nWardrobe = UnitGetWardrobe( pUnit );
	ASSERT_RETURN( nWardrobe != INVALID_ID );

	c_WardrobeSetBodyColor( nWardrobe, (int)dwCallbackData, (BYTE)nIndexChosen);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChooseColorOnClick(	
								   UI_COMPONENT* component,
								   int msg,
								   DWORD wParam,
								   DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UICharCreateColorPickerShow(component, (int)component->m_dwParam, sUICharCreateColorPickerCallback);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUIRefreshRealmComboSetSelection(UI_COMPONENT * pComboComp )
{
	ASSERT_RETURN(pComboComp);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	UITextBoxClear(pCombo->m_pListBox);

	int nSelectionIndex= -1;
	int nIndexAll = -1;
	int nIndices = 0;
	int nIndicesDefault = 0;

	WORLD_REGION eRegion = RegionGetCurrent();
	T_SERVER *pSelectedServerDef = AppGetServerSelection();

//	const SERVERLIST_DEFINITION * pServerlist = DefinitionGetServerlist();
//	const SERVERLISTOVERRIDE_DEFINITION * pServerlistOverride = DefinitionGetServerlistOverride();

	const T_SERVERLIST * pServerlist = ServerListGet();
	const T_SERVERLIST * pServerlistOverride = ServerListOverrideGet();

	BOOL bShowAllRegions = !!UIButtonGetDown(pComboComp->m_pParent, "show all servers btn");

	// build list of viewable regions
	static const int MAX_VIEWABLE_REGIONS = 50;
	WORLD_REGION eViewableRegions[MAX_VIEWABLE_REGIONS + 1];		// always leave an WORLD_REGION_INVALID at the end
	int nNumViewableRegions = 0;
	memset(eViewableRegions, WORLD_REGION_INVALID, sizeof(WORLD_REGION) * MAX_VIEWABLE_REGIONS);

	for (int i=0; i < pServerlist->nRegionViewCount && nNumViewableRegions < MAX_VIEWABLE_REGIONS; i++)
	{
//		REGION_VIEW_DEFINITION &tRegionView = pServerlist->pRegionViews[i];
		T_REGION_VIEW &tRegionView = pServerlist->pRegionViews[i];
		if (tRegionView.eRegion == eRegion)
		{
			for (int j=0; j< tRegionView.nViewRegionCount && nNumViewableRegions < MAX_VIEWABLE_REGIONS; j++)
			{
				if (tRegionView.peViewRegions[j] == eRegion ||
					bShowAllRegions)
				{
					eViewableRegions[nNumViewableRegions++] = tRegionView.peViewRegions[j];
				}
			}
		}
	}
	for (int i=0; i < pServerlistOverride->nRegionViewCount && nNumViewableRegions < MAX_VIEWABLE_REGIONS; i++)
	{
//		REGION_VIEW_DEFINITION &tRegionView = pServerlistOverride->pRegionViews[i];
		T_REGION_VIEW &tRegionView = pServerlistOverride->pRegionViews[i];
		if (tRegionView.eRegion == eRegion)
		{
			for (int j=0; j<tRegionView.nViewRegionCount && nNumViewableRegions < MAX_VIEWABLE_REGIONS; j++)
			{
				if (tRegionView.peViewRegions[j] == eRegion ||
					bShowAllRegions)
				{
					eViewableRegions[nNumViewableRegions++] = tRegionView.peViewRegions[j];
				}
			}
		}
	}

	char szDefaultServer[MAX_XML_STRING_LENGTH] = {'\0'};
	AppLoadServerSelection(szDefaultServer);

	if ( pServerlist )
	{
		for( int i = 0; i < pServerlist->nServerDefinitionCount; i++ )
		{
			for (int j=0; j<MAX_VIEWABLE_REGIONS; j++)
			{
				if (pServerlist->pServerDefinitions[i].eRegion == eViewableRegions[j])
				{
					if (szDefaultServer && szDefaultServer[0] && !PStrICmp(szDefaultServer, pServerlist->pServerDefinitions[i].szOperationalName, MAX_XML_STRING_LENGTH))
					{
						pSelectedServerDef = &(pServerlist->pServerDefinitions[i]);
					}
					if (pSelectedServerDef == &(pServerlist->pServerDefinitions[i]))
					{
						nSelectionIndex = nIndices;
					}
					WCHAR Str[MAX_XML_STRING_LENGTH];
					PStrCvt(Str, pServerlist->pServerDefinitions[i].wszCommonName, MAX_XML_STRING_LENGTH );
					nIndices++;
					if (pServerlist->pServerDefinitions[i].bAllowRandomSelect)
					{
						nIndicesDefault++;
					}
					UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)i);
					break;
				}
			}
		}
	}
	if ( pServerlistOverride )
	{
		for( int i = 0; i < pServerlistOverride->nServerDefinitionCount; i++ )
		{
			for (int j=0; j<MAX_VIEWABLE_REGIONS; j++)
			{
				if (pServerlistOverride->pServerDefinitions[i].eRegion == eViewableRegions[j])
				{
					if (szDefaultServer && szDefaultServer[0] && !PStrICmp(szDefaultServer, pServerlistOverride->pServerDefinitions[i].szOperationalName, MAX_XML_STRING_LENGTH))
					{
						pSelectedServerDef = &(pServerlistOverride->pServerDefinitions[i]);
					}
					if (pSelectedServerDef == &(pServerlistOverride->pServerDefinitions[i]))
					{
						nSelectionIndex = nIndices;
					}
					WCHAR Str[MAX_XML_STRING_LENGTH];
					PStrCvt(Str, pServerlistOverride->pServerDefinitions[i].wszCommonName, MAX_XML_STRING_LENGTH );
					nIndices++;
					UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)i | SERVERLIST_OVERRIDE_FLAG);
					break;
				}
			}
		}
	}

	if (pServerlist)
	{
		if (pSelectedServerDef == NULL && nIndicesDefault > 0)
		{
			// choose a default server at random
			//
			ASSERT_GOTO(nIndicesDefault <= pServerlist->nServerDefinitionCount, nevermind);

			RAND tRand;
			RandInit( tRand );
			const int nSelIdx = (int)RandGetNum( tRand, nIndicesDefault );

			PList<QWORD>::USER_NODE *pItr = NULL;
			int i=-1, j=-1;
			do
			{
				do
				{
					pItr = pCombo->m_pListBox->m_DataList.GetNext(pItr);
					ASSERT_GOTO(pItr, nevermind);
					ASSERT_GOTO(pItr->Value < pServerlist->nServerDefinitionCount, nevermind);
					++j;
				}
				while ( !pServerlist->pServerDefinitions[pItr->Value].bAllowRandomSelect );
				++i;
			}
			while (i < nSelIdx);

			nSelectionIndex = j;
			pSelectedServerDef = &(pServerlist->pServerDefinitions[pItr->Value]);

			nevermind: {}
		}

		if (pSelectedServerDef == NULL && pCombo->m_pListBox->m_DataList.Count() > 0)
		{
			// still don't have a selection... just choose the first one in the list
			//
			PList<QWORD>::USER_NODE *pItr = pCombo->m_pListBox->m_DataList.GetNext(NULL);
			ASSERT_GOTO(pItr, nevermind2);
			ASSERT_GOTO(pItr->Value < pServerlist->nServerDefinitionCount, nevermind2);

			nSelectionIndex = 0;
			pSelectedServerDef = &(pServerlist->pServerDefinitions[pItr->Value]);

			nevermind2: {}
		}
	}

	if( nSelectionIndex >= nIndices )
	{
		nSelectionIndex = 0;
	}
	nSelectionIndex = (nSelectionIndex < 0) ? nIndexAll : nSelectionIndex;
	UIComboBoxSetSelectedIndex(pCombo, nSelectionIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
void sUIRealmComboSetSelection(UI_COMPONENT * pComboComp, int nSelection)
{
	ASSERT_RETURN(pComboComp);


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowAllServersOnButtonDown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_MSG_RETVAL eRetval = UIButtonOnButtonDown(component, msg, wParam, lParam);

	if (ResultIsHandled(eRetval))
	{
		UI_COMPONENT * pRealmCombo = UIComponentFindChildByName(component->m_pParent, UI_REALM_COMBO);
		if (pRealmCombo != 0)
		{
			sUIRefreshRealmComboSetSelection(pRealmCombo);
		}

	}
	return eRetval;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILoginOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pRealmCombo = UIComponentFindChildByName(component, UI_REALM_COMBO);
	if (pRealmCombo != 0)
	{
		sUIRefreshRealmComboSetSelection(pRealmCombo);
	}

	//if (AppCommonMultiplayerOnly())
	//{
	//	UI_COMPONENT * pReturnBtnLabel = UIComponentFindChildByName(component, "login cancel label");
	//	if (pReturnBtnLabel)
	//	{
	//		UILabelSetTextByGlobalString(pReturnBtnLabel, GS_UI_MENU_QUIT);
	//	}
	//}

	return UIMSG_RET_HANDLED;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRealmComboOnSelect( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pRealmCombo = UIComponentFindChildByName( component, UI_REALM_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pRealmCombo);
	int nIndex = UIComboBoxGetSelectedIndex(pCombo);

	if (nIndex == INVALID_ID)
	{
		return UIMSG_RET_HANDLED;
	}

	QWORD nServerIndexData = UIComboBoxGetSelectedData(pCombo);
	BOOL bServerListOverride = !!(nServerIndexData & SERVERLIST_OVERRIDE_FLAG);
	int nServerIndex = (int)(nServerIndexData & 0xffffffff);

//	const SERVERLIST_DEFINITION * pServerlist = DefinitionGetServerlist();
//	const SERVERLISTOVERRIDE_DEFINITION * pServerlistOverride = DefinitionGetServerlistOverride();

	const T_SERVERLIST * pServerlist = ServerListGet();
	const T_SERVERLIST * pServerlistOverride = ServerListOverrideGet();

	if ( pServerlist && !bServerListOverride )
	{
		ASSERT_RETVAL(nServerIndex < pServerlist->nServerDefinitionCount, UIMSG_RET_HANDLED);
		PStrCopy(gApp.szAuthenticationIP, pServerlist->pServerDefinitions[nServerIndex].szIP, MAX_NET_ADDRESS_LEN);
		AppSetServerSelection( &(pServerlist->pServerDefinitions[nServerIndex]) );
	}
	else if( pServerlistOverride && bServerListOverride )
	{
		ASSERT_RETVAL(nServerIndex < pServerlistOverride->nServerDefinitionCount, UIMSG_RET_HANDLED);
		PStrCopy(gApp.szAuthenticationIP, pServerlistOverride->pServerDefinitions[nServerIndex].szIP, MAX_NET_ADDRESS_LEN);
		AppSetServerSelection( &(pServerlistOverride->pServerDefinitions[nServerIndex]) );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowBugReportDlg(
						char * szShortText,
						const OS_PATH_CHAR * szScreenshot)
{
	UI_COMPONENT *pDlg = UIComponentGetByEnum(UICOMP_BUG_REPORT);
	if (!pDlg)
		return;

	//UIStopSkills();
	c_PlayerClearAllMovement(AppGetCltGame());
	if (!AppIsTugboat())
	{
		UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);
	}

	if (AppIsHellgate())
	{
		UNITID idMurmur = INVALID_ID;
		int nMurmur = GlobalIndexGet(GI_MONSTER_MURMUR);
		if (nMurmur != INVALID_LINK)
		{
			idMurmur = UICreateClientOnlyUnit(AppGetCltGame(), nMurmur);
		}
		pDlg->m_dwParam = (DWORD)idMurmur;				//hacky - running out of generic variables - maybe it needs to be a new type

		UIComponentSetFocusUnit(pDlg, idMurmur);
		UIComponentHandleUIMessage(pDlg, UIMSG_PAINT, 0, 0);
	}

	// this is probably bad.
	if (pDlg->m_pData)
	{
		FREE(NULL, pDlg->m_pData);
		pDlg->m_pData = NULL;
	}
	if (szScreenshot)
	{
		int nLen = PStrLen(szScreenshot)+1;
		pDlg->m_pData = MALLOCZ(NULL, sizeof(OS_PATH_CHAR) * (nLen+1));
		PStrCopy((OS_PATH_CHAR *)pDlg->m_pData, szScreenshot, nLen);
	}

	UI_COMPONENT *pShortEdit = UIComponentFindChildByName(pDlg, "bug report short desc");
	if (pShortEdit)
	{
		WCHAR szBuf[2048] = L"";
		if (szShortText)
			PStrCvt(szBuf, szShortText, arrsize(szBuf));
		UILabelSetText(pShortEdit, szBuf);
	}

	UI_COMPONENT *pLongEdit = UIComponentFindChildByName(pDlg, "bug report long desc");
	if (pLongEdit)
	{
		UILabelSetText(pLongEdit, L"");
	}

	UI_COMPONENT *pEmailEdit = UIComponentFindChildByName(pDlg, "bug report email");
	UI_COMPONENT *pEmailLabel = UIComponentFindChildByName(pDlg, "email label");
	if (pEmailEdit && pEmailLabel)
	{
#if !ISVERSION(RETAIL_VERSION)
		char szEmail[256];
		WCHAR uszEmail[256] = L"";
		::GetPrivateProfileString("Flagship Error Handler", "Bugzilla Login", "", szEmail, 256, "CrashHandler.ini");
		if (szEmail && uszEmail)
		{
			PStrCvt(uszEmail, szEmail, arrsize(uszEmail));
		}
		UILabelSetText(pEmailEdit, uszEmail);
		UIComponentSetVisible(pEmailEdit, TRUE);
		UIComponentSetVisible(pEmailLabel, TRUE);
#else
		UIComponentSetVisible(pEmailEdit, FALSE);
		UIComponentSetVisible(pEmailLabel, FALSE);
#endif
	}

	UIComponentSetActive(pDlg, TRUE);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBugReportOnPostActivate(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	//	AppUserPause(TRUE);
	//	AppPause(TRUE);

	UISetMouseOverrideComponent(component);
	UISetKeyboardOverrideComponent(component);

	return UIMSG_RET_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBugReportOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIRemoveMouseOverrideComponent(component);
	UIRemoveKeyboardOverrideComponent(component);

	if( AppIsHellgate() )
	{
		UNITID idMurmur = (UNITID)component->m_dwParam;		//hacky - running out of generic variables - maybe it needs to be a new type
		if (idMurmur != INVALID_ID)
		{
			UNIT *pUnit = UnitGetById(AppGetCltGame(), idMurmur);
			ASSERT_RETVAL(pUnit && UnitTestFlag(pUnit, UNITFLAG_CLIENT_ONLY), UIMSG_RET_HANDLED);

			if (pUnit &&
				UnitTestFlag(pUnit, UNITFLAG_CLIENT_ONLY))
			{
				UnitFree( pUnit );
				component->m_dwParam = (DWORD)INVALID_ID;
			}
		}
	}

	if (component->m_pData)
	{
		FREE(NULL, component->m_pData);
		component->m_pData = NULL;
	}

	//	AppPause(FALSE);
	//	AppUserPause(FALSE);

	return UIMSG_RET_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBugReportCancel(
								UI_COMPONENT* component,
								int msg,
								DWORD wParam,
								DWORD lParam)
{
	if (msg == UIMSG_KEYUP || msg == UIMSG_KEYDOWN)
	{
		if (wParam != VK_ESCAPE)
			return UIMSG_RET_NOT_HANDLED;
	}
	UI_COMPONENT *pDlg = UIComponentGetByEnum(UICOMP_BUG_REPORT);

	UIComponentHandleUIMessage( pDlg, UIMSG_INACTIVATE, 0, 0 );

	return UIMSG_RET_HANDLED_END_PROCESS;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBugReportSend(
							  UI_COMPONENT* component,
							  int msg,
							  DWORD wParam,
							  DWORD lParam)
{
	if (msg == UIMSG_KEYUP || msg == UIMSG_KEYDOWN || msg == UIMSG_KEYCHAR)
	{
		if (wParam != VK_RETURN)
			return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_BUG_REPORT);
	UI_COMPONENT *pEmailEdit = UIComponentFindChildByName(pDialog, "bug report email");
	ASSERT_RETVAL(pEmailEdit, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pShortDesc = UIComponentFindChildByName(pDialog, "bug report short desc");
	ASSERT_RETVAL(pShortDesc, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pLongDesc = UIComponentFindChildByName(pDialog, "bug report long desc");
	ASSERT_RETVAL(pLongDesc, UIMSG_RET_NOT_HANDLED);

#if !ISVERSION(RETAIL_VERSION)
	const WCHAR *uszEmail = UILabelGetText(pEmailEdit);
	if (!uszEmail || !(uszEmail[0]))
	{
		InputKeyIgnore(1000);	// block key input for 1 second
		UIShowGenericDialog("Enter email", "Please enter a valid email address");
	}
	else
#endif
	{
		char szShortTitle[2048];
		PStrCvt(szShortTitle, UILabelGetText(pShortDesc), arrsize(szShortTitle));
		char szDescription[2048];
		PStrCvt(szDescription, UILabelGetText(pLongDesc), arrsize(szDescription));
		char szEmail[256];

#if !ISVERSION(RETAIL_VERSION)
		PStrCvt(szEmail, uszEmail, arrsize(szEmail));
#else
		PStrCopy(szEmail, gApp.userName, arrsize(szEmail));
#endif

		OS_PATH_CHAR *szScreenshotFile = (OS_PATH_CHAR *)pDialog->m_pData;
		ULONG nBugID = AddBugReport(szShortTitle, szDescription, szEmail, szScreenshotFile);

		InputKeyIgnore(1000);	// block key input for 1 second
		if (nBugID == 0)
		{
			UIShowGenericDialog("Failure", "Sorry, the bug report was not sent.  See the error log for more info.");
		}
		else if (nBugID == (ULONG)-1)
		{
			UIShowGenericDialog("Failure", "The email address was not recognized as a valid Bugzilla login\n(or could not connect to the Bugzilla server).");
		}
		else
		{
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), L"Bug report sent.  Thank you for your input.");
			::WritePrivateProfileString("Flagship Error Handler", "Bugzilla Login", szEmail, "CrashHandler.ini");
			UIComponentHandleUIMessage( UIComponentGetByEnum(UICOMP_BUG_REPORT), UIMSG_INACTIVATE, 0, 0 );
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;  
}


//----------------------------------------------------------------------------
// Handlers defined in c_eula.cpp
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEulaCancelOnClick(
								  UI_COMPONENT* component,
								  int msg,
								  DWORD wParam,
								  DWORD lParam);

UI_MSG_RETVAL UIEulaOkOnClick(
							  UI_COMPONENT* component,
							  int msg,
							  DWORD wParam,
							  DWORD lParam);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEulaScreenOnKeyUp( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (wParam == VK_ESCAPE) {
		UIEulaCancelOnClick(component, msg, wParam, lParam);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (wParam == VK_RETURN) {
		UIEulaOkOnClick(component, msg, wParam, lParam);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

// With out these overrides, they can't accept any inputs for some reason -rli

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEulaScreenOnActivate(
									 UI_COMPONENT* component,
									 int msg,
									 DWORD wParam,
									 DWORD lParam)
{
	component = UIComponentGetByEnum(UICOMP_EULA_SCREEN);
	if (component) {
		UISetMouseOverrideComponent(component);
		UISetKeyboardOverrideComponent(component);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEulaScreenOnInactivate(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	component = UIComponentGetByEnum(UICOMP_EULA_SCREEN);
	if (component) {
		UIRemoveMouseOverrideComponent(component);
		UIRemoveKeyboardOverrideComponent(component);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICreateGuildDialogOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(component, "guild create name");
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);
	UI_EDITCTRL *pEditCtrlX = UICastToEditCtrl(pEditCtrl);
	pEditCtrlX->m_nMaxLen = MAX_CHAT_CNL_NAME;
	UILabelSetText(pEditCtrl, L"");

	UIInputOverridePushHead(component, msg, wParam, lParam);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICreateGuildDialogOnCancel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if( AppIsHellgate() )
	{
		UI_COMPONENT *pDialog = component->m_pParent;
		ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);

		UIInputOverrideRemove(pDialog, msg, wParam, lParam);
		UIComponentActivate(pDialog, FALSE);
	}
	else
	{
		UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GUILD_CREATE_DIALOG);
		UIInputOverrideRemove(pDialog, msg, wParam, lParam);
		UIComponentActivate(pDialog, FALSE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICreateGuildDialogOnOK(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pDialog = component->m_pParent;
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(pDialog, "guild create name");
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);

	const WCHAR *szGuildName = UILabelGetText(pEditCtrl);
	if (!szGuildName || !szGuildName[0])
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// everything looks ok - send guild create message to the server
	//
	MSG_CCMD_GUILD_CREATE msgGuildCreate;
	PStrCopy(msgGuildCreate.wszGuildName, MAX_CHAT_CNL_NAME, szGuildName, MAX_CHAT_CNL_NAME);
	msgGuildCreate.idGuildHerald = UIGetCursorUseUnit();
	c_SendMessage(CCMD_GUILD_CREATE, &msgGuildCreate);

	return UICreateGuildDialogOnCancel(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICreateGuildDialogOnKeyDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component))
	{
		if (wParam == VK_RETURN)
		{
			UI_COMPONENT *pOKButton = UIComponentFindChildByName(component, "button dialog ok");
			ASSERT_RETVAL(pOKButton, UIMSG_RET_NOT_HANDLED);
			return UICreateGuildDialogOnOK(pOKButton, msg, wParam, lParam);
		}

		if (wParam == VK_ESCAPE)
		{
			UI_COMPONENT *pCancelButton = UIComponentFindChildByName(component, "button dialog cancel");
			ASSERT_RETVAL(pCancelButton, UIMSG_RET_NOT_HANDLED);
			return UICreateGuildDialogOnCancel(pCancelButton, msg, wParam, lParam);
		}

		// eat all other keys
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowStackSplitDialog(
	UNIT * pUnit)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_STACK_SPLIT_DIALOG);
	ASSERT_RETURN(pDialog);
	UIComponentSetFocusUnit(pDialog, UnitGetId(pUnit));
	UIComponentActivate(pDialog, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitDialogOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(component, "stack size");
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);
	UI_EDITCTRL *pEditCtrlX = UICastToEditCtrl(pEditCtrl);
	pEditCtrlX->m_nMaxLen = 5;
	UILabelSetText(pEditCtrl, L"");

	UIInputOverridePushHead(component, msg, wParam, lParam);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitDialogOnCancel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_STACK_SPLIT_DIALOG);
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);

	UIInputOverrideRemove(pDialog, msg, wParam, lParam);
	UIComponentActivate(pDialog, FALSE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitDialogOnOK(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pDialog = component->m_pParent;
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(pDialog, "stack size");
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);

	const WCHAR *szSizeText = UILabelGetText(pEditCtrl);
	if (!szSizeText || !szSizeText[0])
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT *pItem = UIComponentGetFocusUnit(pDialog);
	int nSize = PStrToInt(szSizeText);
	if (nSize > ItemGetQuantity(pItem) - 1)
		return UIMSG_RET_NOT_HANDLED;

	// everything looks ok - send stack split message to the server
	MSG_CCMD_ITEM_SPLIT_STACK msgSplit;
	msgSplit.idItem = UnitGetId(pItem);
	msgSplit.nStackPrevious = ItemGetQuantity(pItem);
	msgSplit.nNewStackSize = nSize;
	c_SendMessage(CCMD_ITEM_SPLIT_STACK, &msgSplit);

	return UIStackSplitDialogOnCancel(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitDialogOnKeyDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component))
	{
		if (wParam == VK_RETURN)
		{
			UI_COMPONENT *pOKButton = UIComponentFindChildByName(component, "button dialog ok");
			ASSERT_RETVAL(pOKButton, UIMSG_RET_NOT_HANDLED);
			return UIStackSplitDialogOnOK(pOKButton, msg, wParam, lParam);
		}

		if (wParam == VK_ESCAPE)
		{
			UI_COMPONENT *pCancelButton = UIComponentFindChildByName(component, "button dialog cancel");
			ASSERT_RETVAL(pCancelButton, UIMSG_RET_NOT_HANDLED);
			return UIStackSplitDialogOnCancel(pCancelButton, msg, wParam, lParam);
		}

		// eat all other keys
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitEditOnKeyChar(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIEditCtrlOnKeyChar(component, msg, wParam, lParam);

	if (UIComponentGetActive(component))
	{
		// Cap value
		UI_COMPONENT *pDialog = component->m_pParent;
		ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
		UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(pDialog, "stack size");
		ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);
		const WCHAR *szSizeText = UILabelGetText(pEditCtrl);
		UNIT *pItem = UIComponentGetFocusUnit(pDialog);
		int nSize = PStrToInt(szSizeText);
		if (nSize > ItemGetQuantity(pItem) - 1)
		{
			nSize = ItemGetQuantity(pItem) - 1;
			WCHAR szNewText[256];
			PStrPrintf(szNewText, 256, L"%d", nSize);
			UILabelSetText(pEditCtrl, szNewText);
		}

		// eat all other keys
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
// Timer for click and hold functionality of +/- buttons
//----------------------------------------------------------------------------
static DWORD sgdwUIStackSplitMouseRepeat = INVALID_ID;
static int sgnUIStackSplitMouseRepeatValue = 0;
static int sgnUIStackSplitMouseRepeatDelta = 1;
static int sgnUIStackSplitMouseRepeatCap = 0;
UI_COMPONENT* sgpUIStackSplitEditControl = NULL;
#define MOUSE_REPEAT_RATE 100

static void sMouseRepeat(
	GAME*,
	const CEVENT_DATA&,
	DWORD nFirstCall)
{
	if (sgdwUIStackSplitMouseRepeat != INVALID_ID)
		CSchedulerCancelEvent(sgdwUIStackSplitMouseRepeat);

	if (sgnUIStackSplitMouseRepeatValue != sgnUIStackSplitMouseRepeatCap)
	{
		sgnUIStackSplitMouseRepeatValue += sgnUIStackSplitMouseRepeatDelta;
		WCHAR szNewText[256];
		PStrPrintf(szNewText, 256, L"%d", sgnUIStackSplitMouseRepeatValue);
		UILabelSetText(sgpUIStackSplitEditControl, szNewText);
	}
	DWORD mult = (nFirstCall == INVALID_ID)? 2 : 1; // KCK: Delay longer after the first click
	TIME time = CSchedulerGetTime() + MOUSE_REPEAT_RATE * mult;
	CEVENT_DATA nodata;
	sgdwUIStackSplitMouseRepeat = CSchedulerRegisterEvent(time, sMouseRepeat, nodata);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitPlusUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonUp(component, msg, wParam, lParam);

	if (sgdwUIStackSplitMouseRepeat != INVALID_ID)
		CSchedulerCancelEvent(sgdwUIStackSplitMouseRepeat);


	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitPlusDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonDown(component, msg, wParam, lParam);

	UI_COMPONENT *pDialog = component->m_pParent;
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(pDialog, "stack size");
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);

	const WCHAR *szSizeText = UILabelGetText(pEditCtrl);
	sgnUIStackSplitMouseRepeatValue = PStrToInt(szSizeText);
	sgnUIStackSplitMouseRepeatDelta = 1;
	UNIT *pItem = UIComponentGetFocusUnit(pDialog);
	sgnUIStackSplitMouseRepeatCap = ItemGetQuantity(pItem)-1;
	sgpUIStackSplitEditControl = pEditCtrl;

	CEVENT_DATA nodata;
	sMouseRepeat(NULL, nodata, INVALID_ID);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitMinusUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonUp(component, msg, wParam, lParam);

	if (sgdwUIStackSplitMouseRepeat != INVALID_ID)
		CSchedulerCancelEvent(sgdwUIStackSplitMouseRepeat);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStackSplitMinusDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) ||
		!UIComponentGetVisible(component) ||
		!UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIButtonOnButtonDown(component, msg, wParam, lParam);

	UI_COMPONENT *pDialog = component->m_pParent;
	ASSERT_RETVAL(pDialog, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pEditCtrl = UIComponentFindChildByName(pDialog, "stack size");
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);

	const WCHAR *szSizeText = UILabelGetText(pEditCtrl);
	sgnUIStackSplitMouseRepeatValue = PStrToInt(szSizeText);
	sgnUIStackSplitMouseRepeatDelta = -1;
	sgnUIStackSplitMouseRepeatCap = 0;
	sgpUIStackSplitEditControl = pEditCtrl;

	CEVENT_DATA nodata;
	sMouseRepeat(NULL, nodata, INVALID_ID);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerRequestCanBeShown()
{
	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit(pGame);
		if (pPlayer)
		{
			return !IsUnitDeadOrDying(pPlayer);
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTryPlayerRequestDialog(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD dwTicket)
{
	if (!AppGetCltGame() || AppGetState() != APP_STATE_IN_GAME)
		return;

	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
	ASSERT_RETURN(pComponent);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(pComponent);

	if (!sPlayerRequestCanBeShown() || UIComponentGetVisible(pComponent))
	{
		// schedule another
		pDialog->m_dwRetryEventTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 3000, sTryPlayerRequestDialog, CEVENT_DATA());
		return;
	}

	// Show the request now
	//   set the message and show the dialog
	UI_COMPONENT *pComp = UIComponentFindChildByName(pDialog, "invite dialog text");
	ASSERT_RETURN(pComp);

	PList<UI_PLAYER_REQUEST_DATA>::USER_NODE *pItr = pDialog->m_listPlayerRequests.GetNext(NULL);
	if (!pItr)
		return;		//hm... nothing to show.  May have been cancelled

	UILabelSetText(pComp, pItr->Value.szMessage);

	UI_COMPONENT *pIgnoreButton = UIComponentFindChildByName(pDialog, "invite dialog ignore");
	if (pIgnoreButton)
	{
		UIComponentActivate(pIgnoreButton, pItr->Value.pfnOnIgnore != NULL);
	}
	
	UIComponentActivate(pDialog, TRUE);
	UIComponentHandleUIMessage(pDialog, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowPlayerRequestDialog(
	PGUID guidOtherPlayer,
	const WCHAR *szMessage,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnAccept,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnDecline,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnIgnore)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
	ASSERT_RETURN(pComponent);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(pComponent);

	UI_PLAYER_REQUEST_DATA tRequest;
	tRequest.guidOtherPlayer = guidOtherPlayer;
	PStrCopy(tRequest.szMessage, szMessage, arrsize(tRequest.szMessage));
	tRequest.pfnOnAccept = pfnOnAccept;
	tRequest.pfnOnDecline = pfnOnDecline;
	tRequest.pfnOnIgnore = pfnOnIgnore;
	pDialog->m_listPlayerRequests.PListPushTail(tRequest);

	if (!CSchedulerIsValidTicket(pDialog->m_dwRetryEventTicket))
	{
		pDialog->m_dwRetryEventTicket = CSchedulerRegisterEventImm( sTryPlayerRequestDialog, CEVENT_DATA() );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICancelPlayerRequestDialog(
	PGUID guidOtherPlayer,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnAccept,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnDecline,
	PFN_PLAYER_REQUEST_CALLBACK pfnOnIgnore)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
	ASSERT_RETURN(pComponent);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(pComponent);

	PList<UI_PLAYER_REQUEST_DATA>::USER_NODE *pHead = pDialog->m_listPlayerRequests.GetNext(NULL);
	PList<UI_PLAYER_REQUEST_DATA>::USER_NODE *pItr = pHead;
	while (pItr)
	{
		if (pItr->Value.guidOtherPlayer == guidOtherPlayer &&
			pItr->Value.pfnOnAccept == pfnOnAccept &&
			pItr->Value.pfnOnDecline == pfnOnDecline &&
			pItr->Value.pfnOnIgnore == pfnOnIgnore)
		{
			if (pItr == pHead)	// if this is the request currently being shown
			{
				UI_COMPONENT *pDialog = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
				if (pDialog && UIComponentGetVisible(pDialog))
				{
					UIComponentActivate(pDialog, FALSE);
				}
			}
			pItr = pDialog->m_listPlayerRequests.RemoveCurrent(pItr);
		}
		else
		{
			pItr = pDialog->m_listPlayerRequests.GetNext(pItr);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerRequestOnButtonCommon(
	UI_COMPONENT* component,
	int msg,
	UI_PLAYER_REQUEST_DIALOG *pDialog,
	PFN_PLAYER_REQUEST_CALLBACK pfnCallback,
	UI_PLAYER_REQUEST_DATA *pReqData)
{
	ASSERT_RETFALSE(component);
	if (!UIComponentGetActive(component))
		return FALSE;  

	if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP || UIComponentCheckBounds(component))		// This bounds check is unnecessary if this is the result of an LClick message, but I'm leaving it here for Tugboat
	{
		if (msg == UIMSG_KEYDOWN || msg == UIMSG_KEYUP)
		{
			//if (wParam != VK_ESCAPE)
			{
				return FALSE;  
			}
		}
		ASSERT_RETFALSE(pDialog);
		ASSERTX_RETFALSE(pReqData, "Expected to have player request data!");

		if (pfnCallback)
		{
			pfnCallback(pReqData->guidOtherPlayer, pReqData->pData);
		}

		UIComponentActivate(pDialog, FALSE);
		PList<UI_PLAYER_REQUEST_DATA>::USER_NODE *pItr = pDialog->m_listPlayerRequests.GetNext(NULL);
		if (pItr)
		{
			if (!CSchedulerIsValidTicket(pDialog->m_dwRetryEventTicket))
			{
				pDialog->m_dwRetryEventTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 1500, sTryPlayerRequestDialog, CEVENT_DATA());
			}
		}

		return TRUE;  

	}

	return FALSE;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerRequestOnIgnore(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(pComponent);

	UI_PLAYER_REQUEST_DATA tReqData = pDialog->m_listPlayerRequests.GetNext( NULL )->Value;

	BOOL bRet = pDialog->m_listPlayerRequests.Count() > 0;
	// if for SOME reason we really have no requests, then nix this dialog - it shouldn't be up!
	if( !bRet )
	{
		UIComponentActivate(pDialog, FALSE);
	}
	ASSERTX_RETVAL(bRet, UIMSG_RET_NOT_HANDLED, "Expected to have player request data!");

	if (sPlayerRequestOnButtonCommon(component, msg, pDialog, tReqData.pfnOnIgnore, &tReqData))
	{
		pDialog->m_listPlayerRequests.PopHead(tReqData);
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerRequestOnDecline(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(pComponent);

	UI_PLAYER_REQUEST_DATA tReqData = pDialog->m_listPlayerRequests.GetNext( NULL )->Value;

	BOOL bRet = pDialog->m_listPlayerRequests.Count() > 0;
	// if for SOME reason we really have no requests, then nix this dialog - it shouldn't be up!
	if( !bRet )
	{
		UIComponentActivate(pDialog, FALSE);
	}
	ASSERTX_RETVAL(bRet, UIMSG_RET_NOT_HANDLED, "Expected to have player request data!");

	if (sPlayerRequestOnButtonCommon(component, msg, pDialog, tReqData.pfnOnDecline, &tReqData))
	{
		pDialog->m_listPlayerRequests.PopHead(tReqData);
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerRequestOnAccept(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_PLAYER_REQUEST_DIALOG);
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(pComponent);

	UI_PLAYER_REQUEST_DATA tReqData = pDialog->m_listPlayerRequests.GetNext( NULL )->Value;

	BOOL bRet = pDialog->m_listPlayerRequests.Count() > 0;
	// if for SOME reason we really have no requests, then nix this dialog - it shouldn't be up!
	if( !bRet )
	{
		UIComponentActivate(pDialog, FALSE);
	}
	ASSERTX_RETVAL(bRet, UIMSG_RET_NOT_HANDLED, "Expected to have player request data!");

	if (sPlayerRequestOnButtonCommon(component, msg, pDialog, tReqData.pfnOnAccept, &tReqData))
	{
		pDialog->m_listPlayerRequests.PopHead(tReqData);
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadPlayerRequestDialog(
	CMarkup& xml, 
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	ASSERT_RETFALSE(component);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(component);

	pDialog->m_dwRetryEventTicket = INVALID_ID;
	pDialog->m_listPlayerRequests.Init();

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentFreePlayerRequestDialog(
	UI_COMPONENT* component)
{
	ASSERT_RETURN(component);
	UI_PLAYER_REQUEST_DIALOG *pDialog = UICastToPlayerRequestDialog(component);

	pDialog->m_listPlayerRequests.Destroy(NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesMenus(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	
	UI_REGISTER_COMPONENT( UI_PLAYER_REQUEST_DIALOG, UITYPE_PLAYER_REQUEST_DIALOG, "player_request_dialog", UIXmlLoadPlayerRequestDialog, UIComponentFreePlayerRequestDialog );

	UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name							function pointer
		{ "UICharacterAdjustClassOnClick",			UICharacterAdjustClassOnClick },
		{ "UICharacterAdjustOnClick",				UICharacterAdjustOnClick },
		{ "UICharacterGemToggle",					UICharacterGemToggle },
		{ "UICharacterHardcoreToggle",				UICharacterHardcoreToggle },
		{ "UICharacterLeagueToggle",				UICharacterLeagueToggle },
		{ "UICharacterEliteToggle",					UICharacterEliteToggle },
		{ "UICharacterNightmareToggle",				UICharacterNightmareToggle },
		{ "UICharacterPVPOnlyToggle",				UICharacterPVPOnlyToggle },
		{ "UICharacterTutorialToggle",				UICharacterTutorialToggle },
		{ "UICharacterHardcoreHover",				UICharacterHardcoreHover },
		{ "UICharacterLeagueHover",				UICharacterLeagueHover },
		{ "UICharacterEliteHover",					UICharacterEliteHover },
		{ "UICharacterAdjustSexOnClick",			UICharacterAdjustSexOnClick },
		{ "UICharacterAdjustRaceOnClick",			UICharacterAdjustRaceOnClick },
		{ "UICharacterAdjustSize",					UICharacterAdjustSize },
		{ "UICharacterCreateAcceptOnClick",			UICharacterCreateAcceptOnClick },
		{ "UICharacterCreateCancelOnClick",			UICharacterCreateCancelOnClick },
		{ "UICharacterCreateOnKeyDown",				UICharacterCreateOnKeyDown },
		{ "UICharacterCreateRandomizeOnClick",		UICharacterCreateRandomizeOnClick },
		{ "UICharacterCreateRotatePanelOnMouseMove",UICharacterCreateRotatePanelOnMouseMove },
		{ "UICharacterCreateRotatePanelRotate",		UICharacterCreateRotatePanelRotate },
		{ "UICharacterCreateOnPostActivate",		UICharacterCreateOnPostActivate },

		{ "UICharacterSelectOnPostActivate",		UICharacterSelectOnPostActivate },
		{ "UICharacterSelectOnPostInactivate",		UICharacterSelectOnPostInactivate },
		{ "UICharacterSelectAcceptOnClick",			UICharacterSelectAcceptOnClick },
		{ "UICharacterSelectCancelOnClick",			UICharacterSelectCancelOnClick },
		{ "UICharacterSelectButtonOnClick",			UICharacterSelectButtonOnClick },
		{ "UICharacterSelectScrollOnClick",			UICharacterSelectScrollOnClick },
		{ "UICharacterSelectCreateOnClick",			UICharacterSelectCreateOnClick },
		{ "OnCharacterDeleteClick",					OnCharacterDeleteClick },
		{ "OnCharacterDeleteOnOk",					OnCharacterDeleteOnOk },
		{ "OnCharacterDeleteOnCancel",				OnCharacterDeleteOnCancel },
		{ "UICharacterSelectOnKeyDown",				UICharacterSelectOnKeyDown },
		{ "UIDeleteCharDialogOnKeyDown",			UIDeleteCharDialogOnKeyDown },
		{ "UIButtonCreditsShow",					UIButtonCreditsShow },
		{ "UIButtonCreditsExit",					UIButtonCreditsExit },

		{ "UIConversationOnKeyDown",				UIConversationOnKeyDown },
		{ "UIConversationOnOK",						UIConversationOnOK },
		{ "UIConversationOnCancel",					UIConversationOnCancel },
		{ "UIConversationOnPageNext",				UIConversationOnPageNext },
		{ "UIConversationOnPagePrev",				UIConversationOnPagePrev },
		{ "UIConversationOnPaint",					UIConversationOnPaint },
		{ "UIConversationOnPostActivate",			UIConversationOnPostActivate },
		{ "UIConversationOnClick",					UIConversationOnClick },
		{ "UIConversationOnFullTextReveal",			UIConversationOnFullTextReveal },
		{ "NPCConversationOnInactivate",			NPCConversationOnInactivate },

		{ "UIGenericDialogOnOK",					UIGenericDialogOnOK },
		{ "UIGenericDialogOnCancel",				UIGenericDialogOnCancel },
		{ "UIInputOverridePushHead",				UIInputOverridePushHead },
		{ "UIInputOverridePushTail",				UIInputOverridePushTail },
		{ "UIInputOverrideRemove",					UIInputOverrideRemove },

		{ "UIGameMenuDoExit",						UIGameMenuDoExit },
		{ "UIGameMenuDoOptions",					UIGameMenuDoOptions },
		{ "UIGameMenuDoReturn",						UIGameMenuDoReturn },
		{ "UIGameMenuDoRestart",					UIGameMenuDoRestart },
		{ "UIGameMenuOnPostActivate",				UIGameMenuOnPostActivate },

		{ "UIStartGameMenuDoMultiplayer",			UIStartGameMenuDoMultiplayer },
		{ "UIStartGameMenuDoQuit",					UIStartGameMenuDoQuit },
		{ "UIStartGameMenuDoGetGameType",			UIStartGameMenuDoGetGameType },
		{ "UIStartGameMenuDoSingleplayer",			UIStartGameMenuDoSingleplayer },
		{ "UIMainMenuOnKeyDown",					UIMainMenuOnKeyDown },
		{ "UIDemoMenuDoOrder",						UIDemoMenuDoOrder },
		{ "UIPaintIfNoMovieIsPlaying",				UIMainMenuPaintIfNoMovieIsPlaying },

		{ "UILoginScreenOnBack",					UILoginScreenOnBack },
		{ "UILoginScreenOnEnter",					UILoginScreenOnEnter },
		{ "UILoginScreenOnKeyUp",					UILoginScreenOnKeyUp },
		{ "UICreateAccountOnClick",					UICreateAccountOnClick },
		{ "UIManageAccountOnClick",					UIManageAccountOnClick },
		{ "UIShowAllServersOnButtonDown",			UIShowAllServersOnButtonDown },

		{ "UIEulaOkOnClick",						UIEulaOkOnClick },
		{ "UIEulaCancelOnClick",					UIEulaCancelOnClick },
		{ "UIEulaScreenOnKeyUp",					UIEulaScreenOnKeyUp },
		{ "UIEulaScreenOnActivate",					UIEulaScreenOnActivate },
		{ "UIEulaScreenOnInactivate",				UIEulaScreenOnInactivate },

		{ "UIMenuDoSelectedOnEnter",				UIMenuDoSelectedOnEnter },

		{ "UIBuddyInviteOnIgnore",					UIBuddyInviteOnIgnore },
		{ "UIBuddyInviteOnDecline",					UIBuddyInviteOnDecline },
		{ "UIBuddyInviteOnAccept",					UIBuddyInviteOnAccept },

		{ "UIPlayerRequestOnIgnore",				UIPlayerRequestOnIgnore },
		{ "UIPlayerRequestOnDecline",				UIPlayerRequestOnDecline },
		{ "UIPlayerRequestOnAccept",				UIPlayerRequestOnAccept },

		{ "UILoadingBarAnimate",					UILoadingBarAnimate },
		{ "UILoadingCircleAnimate",					UILoadingCircleAnimate },
		{ "UIChooseColorOnClick",					UIChooseColorOnClick },
		{ "UITradeCancel",							UITradeCancel },

		{ "UIBugReportOnPostActivate",				UIBugReportOnPostActivate },
		{ "UIBugReportOnPostInactivate",			UIBugReportOnPostInactivate },
		{ "UIBugReportCancel",						UIBugReportCancel },
		{ "UIBugReportSend",						UIBugReportSend },

		{ "UIFrontEndOnActivate",					UIFrontEndOnActivate },
		{ "UILoginOnActivate",						UILoginOnActivate },
		{ "UIRealmComboOnSelect",					UIRealmComboOnSelect },
		{ "UIDeathDialogOnOK",						UIDeathDialogOnOK },

		{ "UICreateGuildDialogOnOK",				UICreateGuildDialogOnOK },
		{ "UICreateGuildDialogOnCancel",			UICreateGuildDialogOnCancel },
		{ "UICreateGuildDialogOnPostActivate",		UICreateGuildDialogOnPostActivate },
		{ "UICreateGuildDialogOnKeyDown",			UICreateGuildDialogOnKeyDown },

		{ "UIStackSplitDialogOnOK",					UIStackSplitDialogOnOK },
		{ "UIStackSplitDialogOnCancel",				UIStackSplitDialogOnCancel },
		{ "UIStackSplitDialogOnPostActivate",		UIStackSplitDialogOnPostActivate },
		{ "UIStackSplitDialogOnKeyDown",			UIStackSplitDialogOnKeyDown },
		{ "UIStackSplitEditOnKeyChar",				UIStackSplitEditOnKeyChar },
		{ "UIStackSplitPlusUp",						UIStackSplitPlusUp },
		{ "UIStackSplitPlusDown",					UIStackSplitPlusDown },
		{ "UIStackSplitMinusUp",					UIStackSplitMinusUp },
		{ "UIStackSplitMinusDown",					UIStackSplitMinusDown },

	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}

#endif //!ISVERSION(SERVER_VERSION)
