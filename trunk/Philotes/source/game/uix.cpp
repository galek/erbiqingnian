//----------------------------------------------------------------------------
// uix.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "c_townportal.h"
#include "colors.h"
#include "fontcolor.h"
#include "globalIndex.h"
#include "uix.h"
#include "uix_priv.h"
#include "uix_money.h"
#include "uix_debug.h"
#include "uix_graphic.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "uix_components_hellgate.h"
#include "uix_quests.h"
#include "uix_menus.h"
#include "uix_options.h"
#include "uix_social.h"
#include "uix_loglcd.h"
#include "uix_quests.h"
#include "uix_achievements.h"
#include "uix_crafting.h"
#include "uix_chat.h"
#include "uix_email.h"
#include "uix_auction.h"
#include "markup.h"
#include "dictionary.h"
#include "uix_scheduler.h"
#include "excel.h"
#include "units.h"
#include "unit_priv.h"
#include "inventory.h"
#include "items.h"
#include "clients.h"
#include "c_message.h"
#include "c_font.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "player.h"
#include "c_camera.h"
#include "c_input.h"
// debug display
#include "quests.h"
#include "monsters.h"
#include "combat.h"
#include "skills.h"
#include "unittag.h"
#include "stringtable.h"
#include "objects.h"
#include "filepaths.h"
#include "perfhier.h"
#include "script.h"
#include "c_appearance.h"
#include "windowsmessages.h"
#include "quests.h"
#include "c_tasks.h"
#include "s_questgames.h"
#include "c_ui.h"
#include "npc.h"
#include "dialog.h"
#include "Console.h"
#include "quests.h"
#include "states.h"
#include "c_itemupgrade.h"
#include "c_trade.h"
#include "c_recipe.h"
#include "cube.h"
#include "c_reward.h"
#include "performance.h"
#include "pets.h"
#include "wardrobe.h"
#include "unitmodes.h"
#include "gameunits.h"
#include "c_appearance.h"
#include "console_priv.h"
#include "e_ui.h"
#include "e_primitive.h"
#include "e_ui.h"
#include "e_main.h"
#include "e_settings.h"
#include "e_texture.h"
#include "e_definition.h"
#include "e_environment.h"
#include "e_common.h"
#include "c_animation.h"
#include "treasure.h"
#include "camera_priv.h"
#include "language.h"
#include "skill_strings.h"
#include "c_quests.h"
#include "ChatServer.h"
#include "chat.h"
#include "offer.h"
#include "rectorganizer.h"
#include "s_reward.h"
#include "LevelAreas.h"
#include "patchclient.h"
#include "Quest.h"
#include "e_settings.h"
#include "c_voicechat.h"
#include "stash.h"
#include "gameoptions.h"
#include "c_LevelAreasClient.h"
#include "picker.h"
#include "weapons.h"
#include "c_questclient.h"
#include "Currency.h"
#include "c_network.h"
#include "sku.h"
#include "QuestProperties.h"
#include "stringreplacement.h"
#include "warp.h"
#include "quest_tutorial.h"
#include "e_caps.h"
#include "region.h"
#include "version.h"
#include "serverlist.h"
#include "demolevel.h"

#if ISVERSION(DEVELOPMENT)
	#define ACTIVATE_TRACE(fmt, ...)	{UIActivateTrace(fmt, __VA_ARGS__);}
#else
	#define ACTIVATE_TRACE
#endif

CMarkup _gtMarkup;

//----------------------------------------------------------------------------
// EXTERN
//----------------------------------------------------------------------------

extern UI_MSG_RETVAL UISetChatActive( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );

//----------------------------------------------------------------------------
// DEBUG PERFORMANCE
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#ifdef FLOOR
#undef FLOOR
#endif
#define FLOOR(x) ((float)((int)(x)))

#define UI_INIT_PROC_DISP		(sInitProcImmDisp())

BOOL XmlLoadString(
	CMarkup & xml,
	const char * szKey,
	char * szVar,
	int nVarLength,
	const char *szDef /*= NULL*/)
{
	xml.ResetChildPos();
	if (xml.FindChildElem(szKey))
	{
		xml.GetChildData(szVar, nVarLength);
		return TRUE;
	}
	else
	{
		if (szDef)
		{
			PStrCopy(szVar, szDef, nVarLength);
		}
		else
		{
			szVar[0] = 0;
		}
		return FALSE;
	}
}

void UIActivateTrace(
	const char * format,
	...)
{
#if ISVERSION(DEVELOPMENT)
	if (!g_UI.m_bActivateTrace)
		return;

	va_list args;
	va_start(args, format);

	vtrace(format, args);
#endif
}

static void sUIFillLinearMessageHandlerLists(
	UI_COMPONENT *pComponent);

static BOOL sUIRemoveMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg);

static BOOL sUIAddSoundMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg,
	int nSoundID,
	BOOL bStop);

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

#define CODE_PAGE_SIZE					16384

/*
#define UI_SKILL_NORMAL					0xffcfcfcf
#define UI_SKILL_COLOR1					0xffe7e7e7
#define UI_SKILL_BRIGHT					0xffffffff
#define UI_SKILL_COLOR2					0xffd4d4d4
#define UI_SKILL_COLOR3					0xffa9a9a9
#define UI_SKILL_DARK					0xff7f7f7f
*/
#define UI_SKILL_NORMAL					0xffffffff
#define UI_SKILL_COLOR1					0xffe7e7e7
#define UI_SKILL_COLOR2					0xffd4d4d4
#define UI_SKILL_COLOR3					0xffa9a9a9
#define UI_SKILL_DARK					0xff7f7f7f

#define UI_POSTLOAD_FADEOUT_MS			500
#define UI_POSTLOAD_FADEIN_MS			2000

#define	UIACTIVATEFLAG_WINDOWSHADE		0x0001
#define	UIACTIVATEFLAG_IMMEDIATE		0x0002

BOOL bDropShadowsEnabled = TRUE;
const float STD_ANIM_DURATION_MULT = 0.5f;
const DWORD STD_ANIM_TIME = 300;

static const int DEATH_UI_DELAY_IN_MS = GAME_TICKS_PER_SECOND * 4 * MSECS_PER_GAME_TICK;
static const int TUGBOAT_DEATH_UI_DELAY_IN_MS = GAME_TICKS_PER_SECOND * 2 * MSECS_PER_GAME_TICK;

// KCK: Unfortunate globals to store last angle and pitch
float g_fCameraAngle = 0.0f;
float g_fCameraPitch = 0.0f;

//----------------------------------------------------------------------------
static void sDisplayRestartScreen(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD dwData);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentOnFreeUnit(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component->m_idFocusUnit == (UNITID) wParam)
	{
		component->m_idFocusUnit = INVALID_ID;
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConvDlgFreeClientOnlyUnits(
	UI_CONVERSATION_DLG *pDialog)
{
	ASSERT_RETURN(pDialog);

	for (int i=0; i < MAX_CONVERSATION_SPEAKERS; i++)
	{
		if ( pDialog->m_idTalkingTo[i] != INVALID_ID )
		{
			UNIT *pUnit = UnitGetById(AppGetCltGame(), pDialog->m_idTalkingTo[i]);
			if (pUnit &&
				UnitTestFlag(pUnit, UNITFLAG_CLIENT_ONLY))
			{
				UnitFree( pUnit );
				pDialog->m_idTalkingTo[i] = INVALID_ID;
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UNITID UICreateClientOnlyUnit(
	GAME * game,
	int nClass )
{
	ASSERT_RETINVALID( game );

	ASSERT( IS_CLIENT( game ) );

	MONSTER_SPEC spec;
	spec.nClass = nClass;
	spec.dwFlags = MSF_FORCE_CLIENT_ONLY;

	// debug stuffs
	//UNIT * player = GameGetControlUnit( game );
	//spec.pRoom = UnitGetRoom( player );
	//spec.vDirection = player->vFaceDirection;
	//spec.vPosition = player->vPosition + VECTOR( 2.0f, 0.0f, 0.0f );

	UNIT *pUnit = MonsterInit( game, spec );
	if ( !pUnit )
	{
		return INVALID_ID;
	}

	WORD wStateIndex = 0;
	c_StateSet( pUnit, pUnit, STATE_NPC_TALK_TO, wStateIndex, -1, -1 );

	int idModel = c_UnitGetModelIdThirdPerson( pUnit );
	e_ModelSetFlagbit(idModel, MODEL_FLAGBIT_NODRAW, TRUE);

	return UnitGetId(pUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct UIMESSAGE_DATA
{
	int					msg;
	const char*			szMessage;
	BOOL				bPassToChildren;
	BOOL				bProcessWhenInactive;
	PFN_UIMSG_HANDLER	m_fpDefaultFunctionPtr;
};


UIMESSAGE_DATA gUIMESSAGE_DATA[] =
{
	#include "uix_messages.h"
};

static BOOL IsStatMessage(int uimsg)
{
	return (uimsg == UIMSG_SETCONTROLSTAT ||
			uimsg == UIMSG_SETTARGETSTAT ||
			uimsg == UIMSG_SETFOCUSSTAT);
}

STR_DICT pOrientationEnumTbl[] =
{
	{ "",				UIORIENT_LEFT},
	{ "left",			UIORIENT_LEFT},
	{ "right",			UIORIENT_RIGHT},
	{ "bottom",			UIORIENT_BOTTOM},
	{ "top",			UIORIENT_TOP},
	{ NULL,				UIORIENT_LEFT}
};

#endif	//	!ISVERSION(SERVER_VERSION)

//	needed for excel.cpp
STR_DICT pAlignEnumTbl[12] =
{
	{ "",				UIALIGN_TOPLEFT},
	{ "topleft",		UIALIGN_TOPLEFT},
	{ "top",			UIALIGN_TOP},
	{ "topright",		UIALIGN_TOPRIGHT},
	{ "right",			UIALIGN_RIGHT},
	{ "bottomright",	UIALIGN_BOTTOMRIGHT},
	{ "bottom",			UIALIGN_BOTTOM},
	{ "bottomleft",		UIALIGN_BOTTOMLEFT},
	{ "left",			UIALIGN_LEFT},
	{ "center",			UIALIGN_CENTER},

	{ NULL,				UIALIGN_TOPLEFT		},
};

#if !ISVERSION(SERVER_VERSION)

STR_DICT pRenderSectionEnumTbl[] =
{
	{ "World",				RENDERSECTION_WORLD},
	{ "",					RENDERSECTION_BOTTOM},
	{ "Bottom",				RENDERSECTION_BOTTOM},
	{ "DynamicBottom",		RENDERSECTION_DYNAMIC_BOTTOM},
	{ "Masks",				RENDERSECTION_MASKS},
	{ "MaskedModels",		RENDERSECTION_MASKEDMODELS},
	{ "Models",				RENDERSECTION_MODELS},
	{ "ItemText",			RENDERSECTION_ITEMTEXT},
	{ "Tooltips",			RENDERSECTION_TOOLTIPS},
	{ "Menus",				RENDERSECTION_MENUS},
	{ "Dialogs",			RENDERSECTION_MENU_DIALOG},
	{ "DialogMasks",		RENDERSECTION_DIALOG_MASKS},
	{ "DialogMaskedModels",	RENDERSECTION_DIALOG_MASKEDMODELS},
	{ "DialogModelOverlay",	RENDERSECTION_DIALOG_MODELS_OVERLAY},
	{ "LoadingScreen",		RENDERSECTION_LOADING_SCREEN},
	{ "DialogTop",			RENDERSECTION_DIALOG_TOP},
	{ "CursorContents",		RENDERSECTION_CURSOR_CONTENTS},
	{ "Cursor",				RENDERSECTION_CURSOR},
	{ "Debug",				RENDERSECTION_DEBUG},

	{ NULL,					RENDERSECTION_BOTTOM		},
};


STR_DICT pAnimRelationsEnumTbl[] =
{
	{ "opens",			ANIM_RELATIONSHIP_NODE::ANIM_REL_OPENS},
	{ "closes",			ANIM_RELATIONSHIP_NODE::ANIM_REL_CLOSES},
	{ "linked",			ANIM_RELATIONSHIP_NODE::ANIM_REL_LINKED},
	{ "blocked_by",		ANIM_RELATIONSHIP_NODE::ANIM_REL_BLOCKED_BY},
	{ "replaces",		ANIM_RELATIONSHIP_NODE::ANIM_REL_REPLACES},

	{ NULL,					-1	},
};

STR_DICT pAnimCategoriesEnumTbl[] =
{
	{ "main panels",		UI_ANIM_CATEGORY_MAIN_PANELS},
	{ "main panels ua",		UI_ANIM_CATEGORY_MAIN_PANELS_UA},
	{ "left panel",			UI_ANIM_CATEGORY_LEFT_PANEL},
	{ "middle panel",		UI_ANIM_CATEGORY_MIDDLE_PANEL},
	{ "right panel",		UI_ANIM_CATEGORY_RIGHT_PANEL},
	{ "quick close",		UI_ANIM_CATEGORY_QUICK_CLOSE},
	{ "close all",			UI_ANIM_CATEGORY_CLOSE_ALL},
	{ "tooltips",			UI_ANIM_CATEGORY_TOOLTIPS},
	{ "chat fade",			UI_ANIM_CATEGORY_CHAT_FADE},
	{ "map fade",			UI_ANIM_CATEGORY_MAP_FADE},

	{ NULL,					-1	},
};

typedef enum SLOT_SIZE
{
	SMALL_SLOT,
	LARGE_SLOT,
	NUM_SLOT_SIZES
};

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct STAT_MSG_HANDLER
{
	int					m_nMsg;
	int					m_nStat;
	UI_COMPONENT*		m_pComponent;
	PFN_UIMSG_HANDLER	m_pfHandler;
	int					m_nSoundId;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
UI				 g_UI;
UI_SCREEN_RATIOS g_UI_Ratios;

// this should be the number of the last set of UI Types included
#define NUM_UI_TYPES	NUM_UITYPES_MENUS

UI_XML_COMPONENT gUIComponentTypes[NUM_UI_TYPES];
UI_XML_ONFUNC*	gpUIXmlFunctions = NULL;
int				gnNumUIXmlFunctions = 0;

//----------------------------------------------------------------------------
// FORWARD DECLRATIONS
//----------------------------------------------------------------------------
void UIXmlInit(
	void);

char* UIXmlOpenFile(
	const char* filename,
	CMarkup& xml,
	const char* tag,
	BOOL bLoadFromLocalizedPak);

UI_COMPONENT* UIXmlComponentLoad(
	CMarkup& xml,
	CMarkup * includexml,
	UI_COMPONENT* parent,
	const UI_XML& tUIXml,
	BOOL bReferenceBase = FALSE,
	LPCSTR szAltComponentName = NULL);

UI_TEXTURE_FRAME* UITextureNewTextureFrame(
	UIX_TEXTURE* texture,
	const char* name,
	UI_TEXTURE_FRAME* pTemplate = NULL);

void UILabelAutosizeBasedOnText(
	UI_COMPONENT* component,
	const WCHAR* str);

void UIStopSkills(
	void);

void UIAutoMapBeginUpdates(
	void);

void UISetTimedHoverText(
	const WCHAR *szString,
	UI_COMPONENT *pComponent = NULL,
	DWORD dwDelayTime = (DWORD)-1);

UIX_TEXTURE* UITextureLoadDefinition(
	const char* filename,
	BOOL bLoadFromLocalizedPak);

BOOL UIPanelSetTab(
	UI_PANELEX *pPanel,
	int nTab);

UI_MSG_RETVAL UIProcessStatMessage(
	UI_COMPONENT *component,
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

UI_MSG_RETVAL UIProcessStatMessage(
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

PCODE UILoadCode(
	const char* szCode);

UI_XML_COMPONENT* UIXmlComponentFind(
	int nComponentType );

static void sPostInitAnimCategories(
	void);

void sUISpin(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD);

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
inline float UIComponentGetLogWidth(
	UI_COMPONENT * component)
{
	ASSERT(component);
	return component->m_fWidth * UIGetScreenToLogRatioX(component->m_bNoScale);

};

inline float UIComponentGetLogHeight(
	UI_COMPONENT * component)
{
	ASSERT(component);
	return component->m_fHeight * UIGetScreenToLogRatioY(component->m_bNoScale);

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISchedulerMsgCallback(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	int msg = (int)data.m_Data2;
	DWORD wParam = (DWORD)data.m_Data3;
	DWORD lParam = (DWORD)data.m_Data4;
	if (!component)
	{
		return;
	}

	UIComponentHandleUIMessage(component, msg, wParam, lParam);
}

#ifdef _DEBUG
DWORD CSchedulerRegisterMessageDbg(
	TIME time,
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam,
	const char* file,
	int line)
{
	return CSchedulerRegisterEventDbg(time, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR) component, msg, wParam, lParam), file, line);
}
#else
DWORD CSchedulerRegisterMessage(
	TIME time,
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	return CSchedulerRegisterEvent(time, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR) component, msg, wParam, lParam));
}
#endif

void UISetNeedToRender(
	UI_COMPONENT *component)
{
	UISetNeedToRender(component->m_nRenderSection);

	UI_COMPONENT* child = component->m_pFirstChild;
	while (child)
	{
		UISetNeedToRender(child);
		child = child->m_pNextSibling;
	}
}

void UISetNeedToRenderAll(
	void)
{
	for (int i=0; i < NUM_RENDER_SECTIONS; i++)
	{
		UISetNeedToRender(i);
	}
}

void UIRepaint(
	void)
{
	g_UI.m_bFullRepaintRequested = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	DO NOT CALL THIS AS A HANDLER TO A PAINT MESSAGE
UI_MSG_RETVAL UIRepaintComponent(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//	DO NOT CALL THIS AS A HANDLER TO A PAINT MESSAGE
UI_MSG_RETVAL UIRepaintParent(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component->m_pParent)
		return UIComponentHandleUIMessage( component->m_pParent, UIMSG_PAINT, 0, 0);
	else
		return UIMSG_RET_NOT_HANDLED;
}

void UIRefreshText(
	void)
{
	UIComponentHandleUIMessage( g_UI.m_Components, UIMSG_REFRESH_TEXT_KEY, 0, 0 );
	UIRepaint();
}

BOOL UIDebugTestingToggle()
{
	return g_UI.m_bDebugTestingToggle;
}

UI_COMPONENT* UIComponentGetByEnum(
	UI_COMPONENT_ENUM eComp)
{
	ASSERTX_RETNULL( eComp > UICOMP_INVALID && eComp < NUM_UICOMP, "Invalid UI component type" );
	return g_UI.m_pComponentsMap[eComp];
}

static UI_COMPONENT* sUIGetComponent(
					UI_COMPONENT* component,
					int id )
{
	if (!component || id == INVALID_ID)
	{
		return NULL;
	}
	if (component->m_idComponent == id)
	{
		return component;
	}
	UI_COMPONENT* child = component->m_pFirstChild;
	while (child)
	{
		UI_COMPONENT* found = sUIGetComponent(child, id);
		if (found)
		{
			return found;
		}
		child = child->m_pNextSibling;
	}
	return NULL;
}

// CHB 2006.08.22 - This function has been optimized for performance
// by caching the last component id and its respective pointer.
//#define UICOMPONENTGETBYID_USE_CACHE 1
UI_COMPONENT* UIComponentGetById(
	int id)
{
	if (id == INVALID_ID)
	{
		return NULL;
	}

#ifdef UICOMPONENTGETBYID_USE_CACHE
	// The cache.
	static UI_COMPONENT * pCache = 0;
	static int nCacheId = INVALID_ID;
#ifndef _DEBUG
	// In a release build, use the cached value.
	if (id == nCacheId) {
		return pCache;
	}
#endif
#endif

	UI_COMPONENT * found = 0;
	if (g_UI.m_Components != 0) {
		// CHB 2006.08.22 - The vast majority of desired components
		// are descendants of g_UI.m_Components, so try this one first.
		found = sUIGetComponent(g_UI.m_Components, id);
	}
	if ((found == 0) && (g_UI.m_Cursor != 0)) {
		found = sUIGetComponent(g_UI.m_Cursor, id);
	}

#ifdef UICOMPONENTGETBYID_USE_CACHE
	// This would prevent null values from being cached.
//	if (found == 0) {
//		return 0;
//	}

#ifdef _DEBUG
	// In a debug build, verify the cached value.
	// If this assert trips, it means the cached value is incorrect
	// (stale) for the id. It would then become necessary to flush
	// the cached value upon insertion or removal of a comonent.
	ASSERT((id != nCacheId) || (found == pCache));
#endif

	// Update the cache.
	nCacheId = id;
	pCache = found;
#endif

	return found;
}

void UIComponentSetActive(
	UI_COMPONENT* component,
	BOOL bActive)
{
	ASSERT_RETURN(component);

	int bState = bActive ? UISTATE_ACTIVE : UISTATE_INACTIVE;
	if (component->m_eState == bState)
	{
		return;
	}

	component->m_eState = (bState ? UISTATE_ACTIVE : UISTATE_INACTIVE);
	UI_COMPONENT *pChilde = component->m_pFirstChild;
	while(pChilde)
	{
		if (!pChilde->m_bIndependentActivate)
		{
			UIComponentSetActive(pChilde, bActive);
		}
		else
		{
			// check to see if this child is a sub-tab of a panel and if it's the active tab
			if (UIComponentIsPanel(component) && UIComponentIsPanel(pChilde))
			{
				UI_PANELEX *pParentPanel = UICastToPanel(component);
				UI_PANELEX *pChildPanel = UICastToPanel(pChilde);

				if (pParentPanel->m_nCurrentTab == pChildPanel->m_nTabNum)
				{
					UIComponentSetActive(pChilde, bActive);
				}
			}
		}
		pChilde = pChilde->m_pNextSibling;
	}

	if (component->m_eState == UISTATE_ACTIVE || component->m_eState == UISTATE_ACTIVATING)
	{
		UIComponentSetVisible(component, TRUE);
		UIComponentHandleUIMessage(component, UIMSG_POSTACTIVATE, 0, 0);
	}
	else
	{
		UIComponentHandleUIMessage(component, UIMSG_POSTINACTIVATE, 0, 0);
	}

}

UI_COMPONENT* UIComponentFindChildByName(
	UI_COMPONENT* component,
	const char* name,
	BOOL bPartial /*= FALSE*/)
{
	if (!component)
	{
		return NULL;
	}
	int nLen = (bPartial ? PStrLen(name) : MAX_UI_COMPONENT_NAME_LENGTH);
	if (PStrICmp(name, component->m_szName, nLen) == 0)
	{
		return component;
	}
	UI_COMPONENT* child = component->m_pFirstChild;
	while (child)
	{
		UI_COMPONENT* found = UIComponentFindChildByName(child, name, bPartial);
		if (found)
		{
			return found;
		}
		child = child->m_pNextSibling;
	}
	return NULL;
}

UI_COMPONENT* UIComponentFindParentByName(
	UI_COMPONENT* component,
	const char* name,
	BOOL bPartial /*= FALSE*/)
{
	if (!component)
	{
		return NULL;
	}
	int nLen = (bPartial ? PStrLen(name) : MAX_UI_COMPONENT_NAME_LENGTH);

	UI_COMPONENT* parent = component->m_pParent;
	while ( parent )
	{
		if (PStrICmp(name, parent->m_szName, nLen) == 0)
		{
			return parent;
		}
		parent = parent->m_pParent;
	}
	return NULL;
}

UI_COMPONENT* UIComponentIterateChildren(
	UI_COMPONENT* pComponent,
	UI_COMPONENT* pCurChild,
	int nUIType,
	BOOL bDeepSearch )
{
	if (pComponent == pCurChild || !pComponent)
	{
		return NULL;
	}

	UI_COMPONENT *pChild = NULL;
	if (!pCurChild)
	{
		pChild = pComponent->m_pFirstChild;
	}
	else
	{
		pChild = pCurChild->m_pNextSibling;
		//if (pCurChild->m_pNextSibling)
		//{
		//	pChild = pCurChild->m_pNextSibling;
		//}
		//else
		//{
		//	pChild = pCurChild;
		//	while (pChild && pChild->m_pNextSibling == NULL)
		//	{
		//		pChild = pChild->m_pParent;
		//		if (pChild == pComponent)
		//		{
		//			return NULL;
		//		}
		//	}

		//	if (pChild)
		//	{
		//		pChild = pChild->m_pNextSibling;
		//	}
		//	else
		//	{
		//		return NULL;
		//	}
		//}
	}

	while (pChild)
	{
		if (bDeepSearch)
		{
			UI_COMPONENT *pRetComp = UIComponentIterateChildren(pChild, NULL, nUIType, TRUE);
			if (pRetComp)
			{
				return pRetComp;
			}
		}

		if (nUIType == -1 || pChild->m_eComponentType == nUIType)
		{
			return pChild;
		}
		pChild = pChild->m_pNextSibling;
	}

	if (pCurChild && pCurChild->m_pParent && bDeepSearch)
	{
		if (nUIType == -1 || pCurChild->m_pParent->m_eComponentType == nUIType)
		{
			return pCurChild->m_pParent;
		}
		return UIComponentIterateChildren(pComponent, pCurChild->m_pParent, nUIType, TRUE);
	}

	return NULL;
}

inline int UIComponentGetAngle(
	UI_COMPONENT* component)
{
	int angle = component->m_nAngle;
	while (component->m_pParent)
	{
		component = component->m_pParent;
		angle += component->m_nAngle;
	}
	return NORMALIZE(angle, 360);
}

float UIGetDistance(
	const UI_POSITION& StartPos,
	const UI_POSITION& EndPos)
{
	float distsq = (EndPos.m_fX - StartPos.m_fX) * (EndPos.m_fX - StartPos.m_fX) +
		(EndPos.m_fY - StartPos.m_fY) * (EndPos.m_fY - StartPos.m_fY);
	return sqrt(distsq);
}

float UIGetDistanceSquared(
	const UI_POSITION& StartPos,
	const UI_POSITION& EndPos)
{
	float distsq = (EndPos.m_fX - StartPos.m_fX) * (EndPos.m_fX - StartPos.m_fX) +
		(EndPos.m_fY - StartPos.m_fY) * (EndPos.m_fY - StartPos.m_fY);
	return distsq;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* UIGetControlUnit(
	void)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return NULL;
	}
	return GameGetControlUnit(game);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentSetFocusUnit(
	UI_COMPONENT* component,
	UNITID idFocusUnit,
	BOOL bForce /*= FALSE*/)
{
	ASSERT_RETURN(component);
	if (component->m_idFocusUnit == idFocusUnit && bForce == FALSE)
	{
		return;
	}

	component->m_idFocusUnit = idFocusUnit;

	UIComponentHandleUIMessage(component, UIMSG_SETFOCUSUNIT, 0, 0);
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentSetFocusUnitByEnum(
	UI_COMPONENT_ENUM eComponent,
	UNITID idFocus,
	BOOL bForce /*= FALSE*/)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum( eComponent );
	if (pComponent)
	{
		UIComponentSetFocusUnit( pComponent, idFocus, bForce );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentSetFocusUnit(
	UI_COMPONENT* component,
	int ,
	DWORD wParam,
	DWORD )
{
	UIComponentSetFocusUnit(component, UNITID(wParam));
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_POSITION UIComponentGetAbsoluteLogPosition(
	UI_COMPONENT *component)
{
	UI_POSITION posScaled(0.0f, 0.0f);
	UI_POSITION posUnScaled(0.0f, 0.0f);

	UI_POSITION *pos = NULL;

	// find the total position of the element's parent and all of its parents
	while (component)
	{
		if (component->m_pParent && component->m_pParent->m_bNoScale)
			pos = &posUnScaled;
		else
			pos = &posScaled;

		*pos += component->m_Position;
		if (component->m_pParent)
		{
			// big 'ol exception first
			if (UIComponentIsScrollBar(component))
			{
				UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);
				if (pScrollBar->m_pScrollControl == component->m_pParent)
				{
					*pos += UIComponentGetScrollPos(component->m_pParent);		// don't move the scrollbar itself it it's scrolling its parent.
				}
			}
			*pos -= UIComponentGetScrollPos(component->m_pParent);
		}

		component = component->m_pParent;
	}

	posUnScaled.m_fX = posUnScaled.m_fX * UIGetScreenToLogRatioX();
	posUnScaled.m_fY = posUnScaled.m_fY * UIGetScreenToLogRatioY();

	UI_POSITION posReturn(posScaled.m_fX + posUnScaled.m_fX, posScaled.m_fY + posUnScaled.m_fY);
	return posReturn;
}

UI_RECT UIComponentGetAbsoluteRect(
	UI_COMPONENT* component)
{
	UI_RECT componentrect( 0.0f,
						   0.0f,
						   component->m_fWidth,
						   component->m_fHeight );

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(component);
	componentrect.m_fX1 += pos.m_fX;
	componentrect.m_fX2 += pos.m_fX;
	componentrect.m_fY1 += pos.m_fY;
	componentrect.m_fY2 += pos.m_fY;

	return componentrect;
}


void UIComponentSetAnimTime(
	UI_COMPONENT* component,
	const UI_POSITION& StartPos,
	const UI_POSITION& EndPos,
	DWORD dwAnimDuration,
	BOOL bFading)
{
	ASSERT_RETURN(component);

	component->m_tMainAnimInfo.m_dwAnimDuration = dwAnimDuration;

	component->m_tMainAnimInfo.m_tiAnimStart = AppCommonGetCurTime();
	if (component->m_bAnimatesWhenPaused)
		component->m_tMainAnimInfo.m_tiAnimStart = AppCommonGetAbsTime();

	if (!bFading)
	{
		float disttotal = UIGetDistance(StartPos, EndPos);
		float disttoend = UIGetDistance(component->m_Position, EndPos);

		component->m_AnimStartPos = component->m_Position;
		component->m_AnimEndPos = EndPos;
		if (disttotal > 0)
			component->m_tMainAnimInfo.m_dwAnimDuration = (DWORD)((float)dwAnimDuration * disttoend / disttotal);
	}
	else
	{
		component->m_AnimStartPos = component->m_Position;
		component->m_AnimEndPos = component->m_Position;
	}

	component->m_tMainAnimInfo.m_dwAnimDuration = MAX((DWORD)1, component->m_tMainAnimInfo.m_dwAnimDuration);
}

//void UIComponentSetFadeDuration(
//	UI_COMPONENT* component,
//	int uistate,
//	DWORD time)
//{
//	ASSERT_RETURN(component);
//	switch (uistate)
//	{
//	case UISTATE_ACTIVATING:
//		component->m_fFading = 1.0f;
//		break;
//	case UISTATE_INACTIVATING:
//		component->m_fFading = 0.0001f;
//		break;
//	}
//	component->m_tMainAnimInfo.m_tiAnimStart = (component->m_bAnimatesWhenPaused ?  AppCommonGetAbsTime() : AppCommonGetCurTime());
//	component->m_tMainAnimInfo.m_dwAnimDuration = time;
//	component->m_eState = uistate;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UIX_TEXTURE* UIGetFontTexture(
	LANGUAGE eLanguage,
	DWORD dwGetFontTextureFlags /*= 0*/)
{

	// if we have a texture already (and we're not ignoring it), don't search again, just return what we got
	if (TESTBIT( dwGetFontTextureFlags, GFTF_IGNORE_EXISTING_GLOBAL_ENTRY ) == FALSE)
	{
		if (g_UI.m_pFontTexture)
		{
			return g_UI.m_pFontTexture;
		}
	}

	// if no language passed in, use the current language
	LANGUAGE eLanguageToUse = eLanguage;
	if (eLanguageToUse == LANGUAGE_INVALID)
	{
		eLanguageToUse = LanguageGetCurrent();
	}
	ASSERTX_RETNULL( eLanguageToUse != LANGUAGE_INVALID, "No language specified" );

	// get language data
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( AppGameGet(), eLanguageToUse );
	ASSERT_RETNULL(pLanguageData);

	if (!pLanguageData->szFontAtlas || !pLanguageData->szFontAtlas[0])
	{
		return NULL;
	}

	UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, pLanguageData->szFontAtlas);
	if (!texture)
	{
		texture = UITextureLoadDefinition(pLanguageData->szFontAtlas, FALSE);
		if (texture)
		{
			texture->m_nDrawPriority = 100;		// this should be the last one
			StrDictionaryAdd(g_UI.m_pTextures, pLanguageData->szFontAtlas, texture);
		}
		else
		{
			ASSERTV( "Language '%s' font atlas '%s' not found", pLanguageData->szName, pLanguageData->szFontAtlas );
		}
	}

	// set in globals if requested
	if (TESTBIT( dwGetFontTextureFlags, GFTF_DO_NOT_SET_IN_GLOBALS ) == FALSE)
	{
		g_UI.m_pFontTexture = texture;
	}

	// return the texture
	return texture;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentRemove(
	UI_COMPONENT* component)
{
	if (!component->m_pParent)
	{
		ASSERT(!component->m_pPrevSibling && !component->m_pNextSibling);
		return;
	}
	UI_COMPONENT* parent = component->m_pParent;
	if (parent->m_pFirstChild == component)
	{
		ASSERT(!component->m_pPrevSibling);
		parent->m_pFirstChild = component->m_pNextSibling;
	}
	if (parent->m_pLastChild == component)
	{
		parent->m_pLastChild = component->m_pPrevSibling;
	}
	if (component->m_pPrevSibling)
	{
		component->m_pPrevSibling->m_pNextSibling = component->m_pNextSibling;
	}
	if (component->m_pNextSibling)
	{
		component->m_pNextSibling->m_pPrevSibling = component->m_pPrevSibling;
	}

	UIComponentRemoveAllElements(component);

	component->m_pParent = NULL;
	component->m_pPrevSibling = NULL;
	component->m_pNextSibling = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentAddChild(
	UI_COMPONENT* parent,
	UI_COMPONENT* child)
{
	if (!parent || !child)
	{
		return;
	}
	UIComponentRemove(child);
	if (parent->m_pLastChild)
	{
		ASSERT(parent->m_pLastChild->m_pNextSibling == NULL);
		parent->m_pLastChild->m_pNextSibling = child;
		child->m_pPrevSibling = parent->m_pLastChild;
		parent->m_pLastChild = child;
	}
	else
	{
		ASSERT(parent->m_pFirstChild == NULL);
		parent->m_pLastChild = parent->m_pFirstChild = child;
	}

	child->m_pParent = parent;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUIComponentFreeData(
	UI_COMPONENT * component)
{
	UIGfxElementFree(component);

	if (component->m_pBlinkData)
	{
		FREE(NULL, component->m_pBlinkData);
		component->m_pBlinkData = NULL;
	}

	if (component->m_pFrameAnimInfo)
	{
		if (component->m_pFrameAnimInfo->m_pAnimFrames)
		{
			FREE(NULL, component->m_pFrameAnimInfo->m_pAnimFrames);
		}
		FREE(NULL, component->m_pFrameAnimInfo);
		component->m_pFrameAnimInfo = NULL;
	}

	if (component->m_pAnimRelationships)
	{
		//Free each relationship
		ANIM_RELATIONSHIP_NODE * pRelationship = component->m_pAnimRelationships;
		while (pRelationship &&
			pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
		{
			pRelationship->Free();
			pRelationship++;
		}

		FREE(NULL, component->m_pAnimRelationships);
		component->m_pAnimRelationships = NULL;
	}

	// free specific data
	UI_XML_COMPONENT *pDefinition = UIXmlComponentFind(component->m_eComponentType);
	if (pDefinition && pDefinition->m_fpComponentFree)
	{
		pDefinition->m_fpComponentFree(component);
	}

	if( component->m_szTooltipText )
	{
		FREE(NULL, component->m_szTooltipText);
	}

	component->m_listMsgHandlers.Destroy(NULL);

}

void sUIComponentFree(
	UI_COMPONENT* component)
{
	ASSERT_RETURN(component);

	// free children first
	if (component->m_pFirstChild)
	{
		sUIComponentFree(component->m_pFirstChild);
		component->m_pFirstChild = NULL;
	}
	// then free next sibling
	if (component->m_pNextSibling)
	{
		sUIComponentFree(component->m_pNextSibling);
		component->m_pNextSibling = NULL;
	}
	// free data
	sUIComponentFreeData(component);

	// free self
	UI_XML_COMPONENT *pDefinition = UIXmlComponentFind(component->m_eComponentType);
	if (pDefinition && pDefinition->m_fpComponentFreeData)
	{
		pDefinition->m_fpComponentFreeData(component);
	}
}

//----------------------------------------------------------------------------
// free a UI_COMPONENT
//----------------------------------------------------------------------------
void UIComponentFree(
	UI_COMPONENT* component)
{
	ASSERT_RETURN(component);

	// unlink
	if (component->m_pParent)
	{
		if (component->m_pParent->m_pFirstChild == component)
		{
			component->m_pParent->m_pFirstChild = component->m_pNextSibling;
		}
		if (component->m_pParent->m_pLastChild == component)
		{
			component->m_pParent->m_pLastChild = component->m_pPrevSibling;
		}
	}

	if (!component->m_pPrevSibling)
	{
		if (g_UI.m_Components == component)
		{
			g_UI.m_Components = component;
		}
	}
	else
	{
		component->m_pPrevSibling->m_pNextSibling = component->m_pNextSibling;
	}
	if (component->m_pNextSibling)
	{
		component->m_pNextSibling->m_pPrevSibling = component->m_pPrevSibling;
	}

	// free children first
	if (component->m_pFirstChild)
	{
		sUIComponentFree(component->m_pFirstChild);
		component->m_pFirstChild = NULL;
	}
	// free data
	sUIComponentFreeData(component);

	// free self
	UI_XML_COMPONENT *pDefinition = UIXmlComponentFind(component->m_eComponentType);
	if (pDefinition && pDefinition->m_fpComponentFreeData)
	{
		pDefinition->m_fpComponentFreeData(component);
	}
}


//----------------------------------------------------------------------------
// callback for STR_DICTIONARY of texture frames UIX_TEXTURE::m_pFrames
//----------------------------------------------------------------------------
static void UITextureFramesDelete(
	char* key,
	void* data)
{
	ASSERT_RETURN(data);
	UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)data;
	if (frame->m_pbMaskBits)
	{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		FREE(g_StaticAllocator, frame->m_pbMaskBits);
#else
		FREE(NULL, frame->m_pbMaskBits);
#endif
	}
	if (frame->m_pChunks)
	{
		FREE(NULL, frame->m_pChunks);
	}
	FREE(NULL, frame);
}


//----------------------------------------------------------------------------
// callback for STR_DICTIONARY of texture fonts UIX_TEXTURE::m_pFonts
//----------------------------------------------------------------------------
static void UITextureFontsDelete(
	char* key,
	void* data)
{
	ASSERT_RETURN(data);
	UIX_TEXTURE_FONT* font = (UIX_TEXTURE_FONT*)data;

	font->Destroy();

	FREE(NULL, font);
}

static BOOL procinited= FALSE;
void sInitProcImmDisp()
{
	//char name[512];
	//DWORD size = 512;
	//if (GetUserName(name, &size))
	//{
	//	if (PStrICmp(name, "clambert")==0 ||
	//		PStrICmp(name, "etlich")==0 ||
	//		PStrICmp(name, "eschaefer")==0 ||
	//		PStrICmp(name, "dbrevik")==0 ||
	//		PStrICmp(name, "pharris")==0)
	//	{
	//		procinited=TRUE;
	//	}
	//}
}

//----------------------------------------------------------------------------
// GLOBAL UI FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIGlobalsSetDefaults(
	UI &tUI)
{

	// set to zero
	structclear( tUI );

	// init any default non-zero values
	tUI.m_nCurrentComponentId = INVALID_ID;
	tUI.m_idHoverUnit = INVALID_ID;
	tUI.m_idHoverUnitSetBy = INVALID_ID;
	tUI.m_idGunMod = INVALID_ID;
	tUI.m_idTargetUnit = INVALID_ID;
	tUI.m_idGunPanel = INVALID_ID;
	tUI.m_idLastMenu = INVALID_ID;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIFree(
	void)
{
	g_UI.m_pTBAltComponent = NULL;	// Tugboat

	UILogLCDFree();
	UIMoneyFree();
	UIXGraphicFree();
	UIChatFree();
	
	g_UI.m_listDelayedActivateTickets.Destroy(NULL);
	g_UI.m_listUseCursorComponents.Destroy(NULL);
	g_UI.m_listUseCursorComponentsWithAlt.Destroy(NULL);
	for (int i=0; i < NUM_UI_MESSAGES; i++)
	{
		g_UI.m_listMessageHandlers[i].Destroy(NULL);
		g_UI.m_listSoundMessageHandlers[i].Destroy(NULL);
	}
	for (int i=0; i < NUM_UI_ANIM_CATEGORIES; i++)
	{
		g_UI.m_listAnimCategories[i].Destroy(NULL);
		g_UI.m_listCompsThatNeedAnimCategories[i].Destroy(NULL);
	}
	g_UI.m_listOverrideMouseMsgComponents.Destroy(NULL);
	g_UI.m_listOverrideKeyboardMsgComponents.Destroy(NULL);
	g_UI.m_listChatBubbles.Destroy(NULL);

	g_UI.m_nHoverDelay = 0;
	g_UI.m_nHoverDelayLong = 0;
	g_UI.m_fUIScaler = 1.0f;

	if (g_UI.m_Components)
	{
		UIComponentFree(g_UI.m_Components);
		g_UI.m_Components = NULL;
	}
	if (g_UI.m_Cursor)
	{
		UIComponentFree(g_UI.m_Cursor);
		g_UI.m_Cursor = NULL;
	}

	if (g_UI.m_pCodeBuffer)
	{
		FREE(NULL, g_UI.m_pCodeBuffer);
		g_UI.m_pCodeBuffer = NULL;
		g_UI.m_nCodeSize = 0;
		g_UI.m_nCodeCurr = 0;
	}

	if (g_UI.m_pTextures)
	{
		StrDictionaryFree(g_UI.m_pTextures);
		g_UI.m_pTextures = NULL;
	}

	if (g_UI.m_pRectOrganizer)
	{
		RectOrganizeDestroy(NULL, g_UI.m_pRectOrganizer);
	}

	PickerFree(NULL, g_UI.m_pOutOfGamePickers);

	if (g_UI.m_pIncomingStore)
	{
		FREE(NULL, g_UI.m_pIncomingStore);
		g_UI.m_pIncomingStore = NULL;
		g_UI.m_bIncoming = FALSE;
	}

	if ( g_UI.m_idCubeDisplayUnit != INVALID_ID )
	{
		c_CubeDisplayUnitFree();
		ASSERT( g_UI.m_idCubeDisplayUnit == INVALID_ID );
	}

	// **************************** g_UI will be cleared after this point *********************************

	structclear(g_UI.m_pComponentsMap);
	structclear(g_UI);
	sUIGlobalsSetDefaults(g_UI);
	g_UI.m_idHoverUnit = INVALID_ID;
	g_UI.m_idGunMod = INVALID_ID;
	g_UI.m_idCubeDisplayUnit = INVALID_ID;

	if (gpUIXmlFunctions)
	{
		FREE(NULL, gpUIXmlFunctions);
		gpUIXmlFunctions = NULL;
	}
	gnNumUIXmlFunctions = 0;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIResizeWindow(
	void)
{
	g_UI_Ratios.m_fLogToScreenRatioX = (float)AppCommonGetWindowWidth() / UIDefaultWidth();
	g_UI_Ratios.m_fLogToScreenRatioY = (float)AppCommonGetWindowHeight() / UIDefaultHeight();
	ASSERT(g_UI_Ratios.m_fLogToScreenRatioX && g_UI_Ratios.m_fLogToScreenRatioY);
	if (!g_UI_Ratios.m_fLogToScreenRatioX || !g_UI_Ratios.m_fLogToScreenRatioY)
	{
		g_UI_Ratios.m_fLogToScreenRatioX = 1.0f;
		g_UI_Ratios.m_fLogToScreenRatioY = 1.0f;
	}
	g_UI_Ratios.m_fScreenToLogRatioX = 1.0f / g_UI_Ratios.m_fLogToScreenRatioX;
	g_UI_Ratios.m_fScreenToLogRatioY = 1.0f / g_UI_Ratios.m_fLogToScreenRatioY;

	UISetNeedToRenderAll();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetReloadCallback(UI_RELOAD_CALLBACK * pCallback)
{
#if ISVERSION(DEVELOPMENT)
	ASSERT((g_UI.m_pfnReloadCallback == 0) || (pCallback == 0));
#endif
	g_UI.m_pfnReloadCallback = pCallback;	// CHB 2007.04.03
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIRequestReload(
	void)
{
	g_UI.m_bRequestingReload = TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void UIDisplaySizeChanged(
	void)
{
	g_UI.m_nDisplayWidth = AppCommonGetWindowWidth();
	g_UI.m_nDisplayHeight = AppCommonGetWindowHeight();
	UIResizeWindow();
	UIRequestReload();
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDisplayChange(	// CHB 2007.08.23
	bool bPost)
{
	bool bSizeChanged = (AppCommonGetWindowWidth() != g_UI.m_nDisplayWidth) || (AppCommonGetWindowHeight() != g_UI.m_nDisplayHeight);

	if (bPost)
	{
		if (bSizeChanged)
		{
			// No need to acquire the mouse here...
			// Reloading the UI will do that.
			UIDisplaySizeChanged();
		}
		else
		{
			c_InputMouseAcquire(AppGetCltGame(), TRUE);
		}
	}
	else
	{
		c_InputMouseAcquire(AppGetCltGame(), FALSE);
		if (bSizeChanged)
		{
			UIXGraphicFree();
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT_ENUM UIComponentGetEnum(UI_COMPONENT * pComponent)
{
	for (int i = 0; i < NUM_UICOMP; ++i)
	{
		if (g_UI.m_pComponentsMap[i] == pComponent)
		{
			return static_cast<UI_COMPONENT_ENUM>(i);
		}
	}
	return UICOMP_INVALID;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIReload(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	// CHB 2007.03.06 - This causes the UI texture to become blurry on Mythos.
	// TWB - I'm not seeing this? I need some of the stuff that happens in here ( notably, changing aspect ratio );
	//if (AppIsTugboat())
	//{
	//	return;
	//}
	// CHB 2007.03.06 - AppReloadUI() doesn't appear to work too well.

	if( AppIsTugboat() )
	{

		// need to hide tug menus to close out any
		// merchant/recipe interactions before
		// the menus get blown away
		UI_TB_HideAllMenus();
	}

	// CHB 2007.08.14 - Preserve last menu across reload
	UI_COMPONENT_ENUM eLastMenu = UICOMP_INVALID;
	{
		UI_COMPONENT * pMenu = UIComponentGetById(g_UI.m_idLastMenu);
		if (pMenu != 0)
		{
			eLastMenu = UIComponentGetEnum(pMenu);
		}
	}

	#if 0
	extern void AppReloadUI(void);
	AppReloadUI();
	#else
	extern void c_sUIReload(BOOL);
	c_sUIReload(FALSE);
	#endif

	// CHB 2007.08.14 - Preserve last menu across reload
	if (eLastMenu != UICOMP_INVALID)
	{
		UI_COMPONENT * pMenu = UIComponentGetByEnum(eLastMenu);
		if (pMenu != 0)
		{
			g_UI.m_idLastMenu = UIComponentGetId(pMenu);
		}
	}

	if ( AppIsHellgate() )
	{
		UNIT *pControlUnit = UIGetControlUnit();
		if (pControlUnit)
			c_QuestEventReloadUI(pControlUnit);
	}
#endif
}

//----------------------------------------------------------------------------
// GENERIC XML File Open
//----------------------------------------------------------------------------
char * UIXmlLoadFile(
	const char * filename,
	CMarkup & xml )
{
	char fullfilename[MAX_XML_STRING_LENGTH];
	PStrPrintf(fullfilename, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_UI_XML, filename);
	PStrReplaceExtension(fullfilename, MAX_XML_STRING_LENGTH, fullfilename, DEFINITION_FILE_EXTENSION);
	DECLARE_LOAD_SPEC(spec, fullfilename);

#if ISVERSION(DEVELOPMENT)
	if (g_UI.m_bReloadDirect && AppCommonGetDirectLoad())		// so this is here for rapidly editing the xml file and reloading the UI while the game is running for quicker development
	{
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	}
#endif


	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	char* buf = (char*)PakFileLoadNow(spec);
	ASSERTV_RETNULL( buf, "Error loading file: '%s'", fullfilename );

	if (!xml.SetDoc(buf, fullfilename))
	{
		ASSERTV(0, "Error parsing file: '%s'", fullfilename);
		FREE(NULL, buf);
		return NULL;
	}

	return buf;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct INIT_LOAD_CALLBACKDATA
{
	int nParentComponentID;
	UI_XML			 tUIXml;
	//UI_COMPONENT **ppReturnComponent;			no longer used
};

static PRESULT sUIInitLoadCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	if (data.m_IOSize == 0)
	{
		return S_FALSE;
	}

	ASSERT_RETFAIL(spec->callbackData);
	INIT_LOAD_CALLBACKDATA *pInitLoadData = (INIT_LOAD_CALLBACKDATA *)spec->callbackData;

	CMarkup xml;
	CMarkup includexml;

	ASSERTV_RETVAL( spec->buffer, S_FALSE, "Error loading file: '%s'", spec->fixedfilename);

	if (!xml.SetDoc((TCHAR *)spec->buffer, (TCHAR *)spec->fixedfilename))
	{
		ASSERTV_RETVAL(0, S_FALSE, "Error parsing file: '%s'", spec->fixedfilename);
	}
	if (!xml.FindElem("ui"))
	{
		ASSERTV_RETVAL(0, S_FALSE, "Error parsing file: '%s', expected tag: '%s'", spec->fixedfilename, "ui");
	}

	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
	UI_COMPONENT *component = &tmp;
	// !!!!

	XML_LOADINT("widthbasis", pInitLoadData->tUIXml.m_nXmlWidthBasis, UIDefaultWidth());
	XML_LOADINT("heightbasis", pInitLoadData->tUIXml.m_nXmlHeightBasis, UIDefaultHeight());
	if (g_UI.m_bWidescreen)
	{
		XML_LOADINT("ws_widthbasis", pInitLoadData->tUIXml.m_nXmlWidthBasis, pInitLoadData->tUIXml.m_nXmlWidthBasis);
		XML_LOADINT("ws_heightbasis", pInitLoadData->tUIXml.m_nXmlHeightBasis, pInitLoadData->tUIXml.m_nXmlHeightBasis);
	}
	pInitLoadData->tUIXml.m_fXmlWidthRatio = UIDefaultWidth() / (float)pInitLoadData->tUIXml.m_nXmlWidthBasis;
	pInitLoadData->tUIXml.m_fXmlHeightRatio = UIDefaultHeight() / (float)pInitLoadData->tUIXml.m_nXmlHeightBasis;

	xml.ResetChildPos(); if (xml.FindChildElem("hoverdelay")) { g_UI.m_nHoverDelay = (int)PStrToInt(xml.GetChildData()); }
	xml.ResetChildPos(); if (xml.FindChildElem("hoverdelaylong")) { g_UI.m_nHoverDelayLong = (int)PStrToInt(xml.GetChildData()); }

	xml.ResetChildPos();
	BOOL bHasInclude = FALSE;
	char* buf = NULL;
	if( xml.FindChildElem("include") )
	{
		const char * sFile = xml.GetChildData();
		bHasInclude = TRUE;
		buf = UIXmlLoadFile( sFile,
					   includexml );
	}

	xml.ResetChildPos();

	xml.FindChildElem();

	UI_COMPONENT *pParent = UIComponentGetById(pInitLoadData->nParentComponentID);
	UI_COMPONENT *pComponent = NULL;
	while (TRUE)
	{
		if( bHasInclude )
		{
			pComponent = UIXmlComponentLoad(xml, &includexml, pParent, pInitLoadData->tUIXml);
		}
		else
		{
			pComponent = UIXmlComponentLoad(xml, NULL, pParent, pInitLoadData->tUIXml);
		}
		if (!pComponent)
		{
			if( !xml.FindChildElem() )
			{
				break;
			}
		}
		else
		{
			UIComponentHandleUIMessage(pComponent, UIMSG_POSTCREATE, 0, 0, TRUE);
			break;
		}
	}

	//little bitty hack here...
	// detect when we've loaded the main ui component
	if (pComponent && PStrICmp(pComponent->m_szName, "ui")==0)
	{
		ASSERTX_RETFAIL(g_UI.m_Components == NULL, "UI Loaded twice");
		g_UI.m_Components = pComponent;

		// CHB 2006.08.24 - Would need to flush component id cache here.
	}
	// detect when we've loaded the cursor and store it.
	if (pComponent && PStrICmp(pComponent->m_szName, "main cursor")==0)
	{
		g_UI.m_Cursor = UICastToCursor(pComponent);
		// CHB 2006.08.24 - Would need to flush component id cache here.
	}

	// Here's a little thing - if this is async, and we've already established a control unit
	//  some controls may need to know about it.  So send the message again so they can set it.
	UNIT *pControlUnit = UIGetControlUnit();
	if (pControlUnit)
	{
		UISendMessage(WM_SETCONTROLUNIT, (WPARAM)UnitGetId(pControlUnit), 0);
	}
	if( buf )
	{
		FREE(NULL, buf);
	}

	g_UI.m_nFilesToLoad--; // keeps track of how many are yet to load

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIXmlLoadBranch(
	LPCWSTR wszFileName,
	LPCSTR szLoadComponent,
	LPCSTR szComponentName,
	UI_COMPONENT *pParent)
{
	CMarkup xml;
	CMarkup includexml;

	ASSERT_RETNULL(wszFileName);
	ASSERT_RETNULL(szLoadComponent);
	ASSERT_RETNULL(pParent);

	if (!xml.Load(wszFileName))
	{
		ASSERTV_RETNULL(0, "Error loading branch: '%s'", wszFileName);
	}
	if (!xml.FindChildElem(szLoadComponent, TRUE, TRUE))
	{
		ASSERTV_RETNULL(0, "Error loading branch: '%s', expected component: '%s'", wszFileName, szLoadComponent);
	}

	UI_XML tUIXml;
	tUIXml.m_nXmlWidthBasis = (int)pParent->m_fWidth;
	tUIXml.m_nXmlHeightBasis = (int)pParent->m_fHeight;
	tUIXml.m_fXmlWidthRatio = 1.0f;
	tUIXml.m_fXmlHeightRatio = 1.0f;

	UI_COMPONENT *pComponent = NULL;
	while (TRUE)
	{
		pComponent = UIXmlComponentLoad(xml, NULL, pParent, tUIXml, NULL, szComponentName);

		if (!pComponent)
		{
			if( !xml.FindChildElem() )
			{
				break;
			}
		}
		else
		{
			UIComponentHandleUIMessage(pComponent, UIMSG_POSTCREATE, 0, 0, TRUE);
			break;
		}
	}

	return pComponent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIInitLoad(
	char * filename,
	UI_COMPONENT * parent_ptr,
	BOOL bForceImmediate = FALSE)
{
#if ISVERSION(DEVELOPMENT)
	char timer_dbgstr[MAX_PATH];
	PStrPrintf(timer_dbgstr, MAX_PATH, "UIInit() - UIInitLoad(%s)", filename);
	TIMER_STARTEX(timer_dbgstr, 50);
#endif

	char fullfilename[MAX_XML_STRING_LENGTH];
	PStrPrintf(fullfilename, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_UI_XML, filename);
	PStrReplaceExtension(fullfilename, MAX_XML_STRING_LENGTH, fullfilename, DEFINITION_FILE_EXTENSION);
	INIT_LOAD_CALLBACKDATA * pInitLoadData = (INIT_LOAD_CALLBACKDATA *)MALLOCZ(NULL, sizeof(INIT_LOAD_CALLBACKDATA));
	pInitLoadData->nParentComponentID = (parent_ptr ? parent_ptr->m_idComponent : INVALID_ID);

	DECLARE_LOAD_SPEC(spec, fullfilename);
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_FREE_BUFFER;

#if ISVERSION(DEVELOPMENT)
	if (g_UI.m_bReloadDirect && AppCommonGetDirectLoad())		// so this is here for rapidly editing the xml file and reloading the UI while the game is running for quicker development
	{
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	}
#endif

	g_UI.m_nFilesToLoad++; // keeps track of how many are yet to load

	spec.fpLoadingThreadCallback = sUIInitLoadCallback;
	spec.callbackData = (void *)pInitLoadData;
	spec.priority = ASYNC_PRIORITY_0;
	if (!bForceImmediate)
	{
		PakFileLoad(spec);
	}
	else
	{
		PakFileLoadNow(spec);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//void UIAsyncLoad(int idComponentNeeded, PFN_yadda pfnCallback)
//{
//	GameFileLoadAsyc(NULL, szFilename, 0, 0, )
//}

//----------------------------------------------------------------------------
// callback for STR_DICTIONARY of UIX_TEXTURES
//----------------------------------------------------------------------------
static void UITextureDelete(
	char* key,
	void* data)
{
	ASSERT_RETURN(data);

	UIX_TEXTURE* tex = (UIX_TEXTURE*)data;

	if (tex->m_pFrames)
	{
		StrDictionaryFree(tex->m_pFrames);
	}
	if (tex->m_pFonts)
	{
		StrDictionaryFree(tex->m_pFonts);
	}

	if ( tex->m_arrFontRows.Initialized() )
	{
		int iRow = tex->m_arrFontRows.GetFirst();
		while (iRow != INVALID_ID)
		{
			UIX_TEXTURE_FONT_ROW *pRow = tex->m_arrFontRows.Get(iRow);
			int iNextRow = tex->m_arrFontRows.GetNextId(iRow);
			ASSERT(pRow);
			pRow->m_arrFontCharacters.Destroy();
			iRow = iNextRow;
		}
		tex->m_arrFontRows.Destroy();
	}

	// Be sure to remove the actual texture resource, and actually remove it, not just the ref
	e_TextureReleaseRef(tex->m_nTextureId, TRUE);
	tex->m_nTextureId = INVALID_ID;

	// clean up references to the global font texture
	if (g_UI.m_pFontTexture == tex)
	{
		g_UI.m_pFontTexture = NULL;
	}

	FREE(NULL, tex);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIFontsResetTextureData()
{
	UIX_TEXTURE *pTexture = UIGetFontTexture(LanguageGetCurrent());

	if (!pTexture)
	{
		return;
	}

	//if (!pTexture->m_pFonts)
	//{
	//	return;
	//}

	//int nFonts = StrDictionaryGetCount(pTexture->m_pFonts);
	//for (int i=0; i < nFonts; i++)
	//{
	//	UIX_TEXTURE_FONT *pFont = (UIX_TEXTURE_FONT *)StrDictionaryGet(pTexture->m_pFonts, i);
	//	ASSERT_RETURN(pFont);
	//	pFont->Reset();
	//}

	if ( pTexture->m_arrFontRows.Initialized() )
	{
		int iRow = pTexture->m_arrFontRows.GetFirst();
		while (iRow != INVALID_ID)
		{
			UIX_TEXTURE_FONT_ROW *pRow = pTexture->m_arrFontRows.Get(iRow);
			ASSERT(pRow);

			UINT iChar = pRow->m_arrFontCharacters.GetFirst();
			while (iChar != INVALID_ID)
			{
				// the UI_TEXTURE_FONT_FRAMEs pointed to in the FontCharacters array can remain
				//   because they reside in the UIX_TEXTURE_FONT structure in a series of hashes.
				//   But they need to have their row IDs cleared so their positions on the texture can be replaced.

				UI_TEXTURE_FONT_FRAME **ppChar = pRow->m_arrFontCharacters.Get(iChar);
				ASSERT(ppChar && *ppChar);
				(*ppChar)->m_nTextureFontRowId = INVALID_ID;
				iChar = pRow->m_arrFontCharacters.GetNextId(iChar);
			}

			pRow->m_arrFontCharacters.Destroy();
			iRow = pTexture->m_arrFontRows.GetNextId(iRow);
		}
		pTexture->m_arrFontRows.Destroy();
	}

	int nWidth ,nHeight;
	V( e_TextureGetOriginalResolution( pTexture->m_nTextureId, nWidth, nHeight ) );

	if ( nHeight != 0 )
	{
		ArrayInit(pTexture->m_arrFontRows, NULL, nHeight / 16);
	}

	e_UIRequestFontTextureReset(FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentSetDataToInvalid(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	pComponent->m_dwData = (DWORD)INVALID_LINK;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_MSG_RETVAL UIRepaintComponent( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIDropCursorItem	( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIEquipCursorItem	( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICursorOnInactivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStashOnInactivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISkillIconDraw( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISkillCooldownDraw( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISkillSphereOnMouseHover( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISkillSphereOnMouseLeave( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIClearHoverText( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITargetedUnitPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISelectionPanelHitIndicatorPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISelectionPanelSetFocusVisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIMinimalSelectionPanelSetFocusVisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPlayerDeath( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIRestart( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIRestartPanelOnSetControlState( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnButtonDownRestartTown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnButtonDownRestartGhost( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnButtonDownRestartResurrect( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnButtonDownRestartArena( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnButtonDownGhostRestartTown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITooltipRestartTown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPaperDollPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointAddButtonSetVisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointAddButtonSetFocusUnit ( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointAddUp( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointAddDown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIActivateAttributeControls( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointSubButtonSetVisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointSubUp( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIStatPointSubDown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIHealthBarOnStatChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelOnMouseMove( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelRotate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelOnPostVisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelWorldCameraOnMouseMove( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelOnLButtonUp( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelOnLButtonDown( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIModelPanelOnPostInactivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIRecipeListSelectRecipe( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnPostActivatePaperdoll( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnPostInactivatePaperdoll( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIEatMsgHandler( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIEatMsgHandlerCheckBounds( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIHandleMsgHandler( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIIgnoreMsgHandler( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIMerchantShowPaperdoll( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIMerchantClose( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIMerchantOnInactivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIItemBoxOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIItemBoxOnMouseHover( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIItemBoxOnMouseLeave( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIOnPostActivateSetWelcomeMsg( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIOnPostActivateSetStatTraderWelcomeMsg( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPetDisplayOnInventoryChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPetCooldownOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPartyDisplayOnLevelRankChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPartyDisplayOnPartyChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIFocusUnitBarOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISelectionHealthBarOnSetTarget( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIEditCtrlOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITasksAccept( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITasksReject( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITasksAbandon( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITasksAcceptReward( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIComponentOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIQuestLogOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIQuestLogOnPostVisible(UI_COMPONENT* component,int msg,DWORD wParam,DWORD lParam);
UI_MSG_RETVAL UIQuestLogOnInventoryChange(UI_COMPONENT* component,int msg,DWORD wParam,DWORD lParam);
UI_MSG_RETVAL UIQuestCancelOnClick(UI_COMPONENT* component,int msg,DWORD wParam,DWORD lParam);
UI_MSG_RETVAL UIContextHelpLabelOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITradeOnAccept( UI_COMPONENT *pComponent,int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITradeMoneyOnChar( UI_COMPONENT *pComponent,int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITradeOnMoneyIncrease( UI_COMPONENT *pComponent,int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITradeOnMoneyDecrease( UI_COMPONENT *pComponent,int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIRewardOnTakeAll( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIRewardsCancel( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIComponentThrobStart( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIComponentThrobEnd( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIAltMenuOnPostVisible( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UISkillTooltipOnSetControlStat( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIComponentSetVisibleFalse( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIFactionStandingsOnPaint( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIFactionStadingDispOnMouseOver( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIFactionStadingDispOnMouseLeave( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIIncomingMessageOnAccept( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIMinigameSlotOnPaint( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIMinigameOnSetTicksLeft( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIUseProgressBarOnSetControlState( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIVisibleOnPlayerFocusOnly( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIComponentVisibleOnStat( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIPingMeterOnMouseHover( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIPingMeterOnPostCreate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIRefreshHoverItem( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIInventoryOnPostActivate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIInventoryOnCloseButtonClicked( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIQuickClosePanels( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIDrawPauseSymbol( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIClosePlayerInspectionPanel( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPlayerInspectionPanelPostInvisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharacterOptionsPanelOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIAutoPartyButtonOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIAutoPartyButtonClicked( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIInspectableButtonOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIInspectableButtonClicked( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIHideHelmetButtonOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIHideHelmetButtonClicked( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharacterSheetOnPostVisible( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharSelectRatingLogoOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIBackpackCancel( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITitleComboBoxOnPostCreate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITitleComboBoxOnSelect( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITitleComboBoxSetToCurrent( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharLevelOnSetFocusStat( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIDashboardSetFocusUnit( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIDashboardOnLevelChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIDashboardOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharacterSheetSetFocusUnit( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharacterSheetOnLevelChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UICharacterSheetOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);

// Tugboat-specific

UI_MSG_RETVAL UIMerchantOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIToggleMagnify( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIToggleWeaponSet( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIWeaponSetOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIPetDisplayOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UIWorldMapOnPaint( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITasksSelectQuest( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UITasksQuestMark( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);

UI_MSG_RETVAL UIShowGameScreen( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIShowAutoMap( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIRepaintAutoMap( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIShowCharacterAndSkillsScreen( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIHideLevelUpMessage( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIShowWorldMapScreen( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIShowTravelMapScreen( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UITasksOnPostActivate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIHideWorldMapScreen( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIToggleVoiceChat( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIVoiceChatOnPostCreate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIStashOnPostInvisible( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIRatingLogoOnPostCreate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UITogglePaneMenu( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UISelectReward( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UISelectDialogOption( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UISelectDialogLocationOption( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIVersionLabelOnPostCreate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
UI_MSG_RETVAL UIMoneyOnChar( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISharedStashGameModeComboOnPostCreate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISharedStashGameModeComboOnPostActivate( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL UISharedStashGameModeComboOnChange( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam);

UI_XML_ONFUNC gUIXmlFunctions[] =
{	// function name						function pointer
	{ "UIComponentSetFocusUnit", UIComponentSetFocusUnit },
	{ "UIRepaintComponent", UIRepaintComponent },
	{ "UIRepaintParent", UIRepaintParent },

	{ "UIDropCursorItem", UIDropCursorItem },
	{ "UICursorOnInactivate", UICursorOnInactivate },
	{ "UIStashOnInactivate", UIStashOnInactivate },
	{ "UIEatMsgHandler", UIEatMsgHandler },
	{ "UIEatMsgHandlerCheckBounds", UIEatMsgHandlerCheckBounds },
	{ "UIHandleMsgHandler", UIHandleMsgHandler },
	{ "UIIgnoreMsgHandler", UIIgnoreMsgHandler },
	{ "UIEquipCursorItem", UIEquipCursorItem },
	{ "UIFocusUnitBarOnPaint", UIFocusUnitBarOnPaint },
	{ "UIClearHoverText", UIClearHoverText },

	{ "UIHealthBarOnStatChange", UIHealthBarOnStatChange },
	{ "UIMerchantClose", UIMerchantClose },
	{ "UIMerchantOnInactivate", UIMerchantOnInactivate },
	{ "UIMerchantShowPaperdoll", UIMerchantShowPaperdoll },
	{ "UIModelPanelOnLButtonDown", UIModelPanelOnLButtonDown },
	{ "UIModelPanelOnLButtonUp", UIModelPanelOnLButtonUp },
	{ "UIModelPanelOnMouseMove", UIModelPanelOnMouseMove },
	{ "UIModelPanelRotate", UIModelPanelRotate },
	{ "UIModelPanelOnPostVisible", UIModelPanelOnPostVisible },
	{ "UIModelPanelWorldCameraOnMouseMove", UIModelPanelWorldCameraOnMouseMove },
	{ "UIModelPanelOnPostInactivate", UIModelPanelOnPostInactivate },
	{ "UIOnPostActivateSetWelcomeMsg", UIOnPostActivateSetWelcomeMsg },
	{ "UIOnPostActivateSetStatTraderWelcomeMsg", UIOnPostActivateSetStatTraderWelcomeMsg },

	{ "UIRecipeListSelectRecipe", UIRecipeListSelectRecipe },
	{ "UIRecipeOnChangeIngredients", UIRecipeOnChangeIngredients },
	{ "UIRecipeCreate", UIRecipeCreate },
	{ "UIRecipeCancel", UIRecipeCancel },
	{ "UIRecipeOnPostInactivate", UIRecipeOnPostInactivate },

	{ "UICubeOnChangeIngredients", UICubeOnChangeIngredients },
	{ "UICubeCreate", UICubeCreate },
	{ "UICubeCancel", UICubeCancel },
	{ "UICubeOnPostInactivate", UICubeOnPostInactivate },
	{ "UICubeRecipeOpen", UICubeRecipeOpen },
	{ "UICubeRecipeClose", UICubeRecipeClose },
	{ "UICubeScrollBarOnScroll", UICubeScrollBarOnScroll },
	{ "UICubeScrollBarOnMouseWheel", UICubeScrollBarOnMouseWheel },
	{ "UICubeRecipeOnPaint", UICubeRecipeOnPaint },
	{ "UICubeColumnOnClick", UICubeColumnOnClick },

	{ "UIItemUpgradeOnChangeItem", UIItemUpgradeOnChangeItem },
	{ "UIItemUpgradeUpgrade", UIItemUpgradeUpgrade },
	{ "UIItemUpgradeCancel", UIItemUpgradeCancel },
	{ "UIItemAugmentOnChangeItem", UIItemAugmentOnChangeItem },
	{ "UIItemAugmentCommon", UIItemAugmentCommon },
	{ "UIItemAugmentRare", UIItemAugmentRare },
	{ "UIItemAugmentLegendary", UIItemAugmentLegendary },
	{ "UIItemAugmentRandom", UIItemAugmentRandom },
	{ "UIItemAugmentCancel", UIItemAugmentCancel },
	{ "UIItemUnModButtonOnClick", UIItemUnModButtonOnClick },
	{ "UIItemUnModOnChangeItem", UIItemUnModOnChangeItem },
	{ "UIItemUnModCancel", UIItemUnModCancel },
	{ "UIItemUnModOnPostInactivate", UIItemUnModOnPostInactivate },
	{ "UIItemAugmentOnPostInactivate", UIItemAugmentOnPostInactivate },
	{ "UIItemUpgradeOnPostInactivate", UIItemUpgradeOnPostInactivate },
	{ "UIItemUpgradePostActivate", UIItemUpgradePostActivate },

	{ "UIItemBoxOnPaint", UIItemBoxOnPaint },
	{ "UIItemBoxOnMouseHover", UIItemBoxOnMouseHover },
	{ "UIItemBoxOnMouseLeave", UIItemBoxOnMouseLeave },

	{ "UIOnPostActivatePaperdoll",		UIOnPostActivatePaperdoll },
	{ "UIOnPostInactivatePaperdoll",		UIOnPostInactivatePaperdoll },

	{ "UIPaperDollPaint", UIPaperDollPaint },
	{ "UIPetDisplayOnInventoryChange", UIPetDisplayOnInventoryChange },
	{ "UIPetCooldownOnPaint", UIPetCooldownOnPaint },
	{ "UIPartyDisplayOnLevelRankChange", UIPartyDisplayOnLevelRankChange },
	{ "UIPartyDisplayOnPartyChange", UIPartyDisplayOnPartyChange },
	{ "UIPlayerDeath", UIPlayerDeath },
	{ "UIRestart", UIRestart },
	{ "UIRestartPanelOnSetControlState", UIRestartPanelOnSetControlState },
	{ "UIOnButtonDownRestartTown", UIOnButtonDownRestartTown },
	{ "UIOnButtonDownRestartGhost", UIOnButtonDownRestartGhost },
	{ "UIOnButtonDownRestartResurrect", UIOnButtonDownRestartResurrect },
	{ "UIOnButtonDownRestartArena", UIOnButtonDownRestartArena },
	{ "UIOnButtonDownGhostRestartTown",	UIOnButtonDownGhostRestartTown },
	{ "UITooltipRestartTown", UITooltipRestartTown },

	{ "UISelectionHealthBarOnSetTarget", UISelectionHealthBarOnSetTarget },
	{ "UISelectionPanelSetFocusVisible", UISelectionPanelSetFocusVisible },
	{ "UIMinimalSelectionPanelSetFocusVisible", UIMinimalSelectionPanelSetFocusVisible },
	{ "UISkillCooldownDraw", UISkillCooldownDraw },
	{ "UISkillIconDraw", UISkillIconDraw },
	{ "UISkillSphereOnMouseHover", UISkillSphereOnMouseHover },
	{ "UISkillSphereOnMouseLeave", UISkillSphereOnMouseLeave },

	{ "UIStatPointAddButtonSetFocusUnit", UIStatPointAddButtonSetFocusUnit },
	{ "UIStatPointAddButtonSetVisible", UIStatPointAddButtonSetVisible },
	{ "UIStatPointAddDown", UIStatPointAddDown },
	{ "UIStatPointAddUp", UIStatPointAddUp },
	{ "UIStatPointSubDown", UIStatPointSubDown },
	{ "UIActivateAttributeControls", UIActivateAttributeControls },
	{ "UIStatPointSubButtonSetVisible", UIStatPointSubButtonSetVisible },
	{ "UIStatPointSubUp", UIStatPointSubUp },
	{ "UITargetedUnitPaint", UITargetedUnitPaint },
	{ "UISelectionPanelHitIndicatorPaint", UISelectionPanelHitIndicatorPaint },
	{ "UITasksAbandon", UITasksAbandon },
	{ "UITasksAccept", UITasksAccept },
	{ "UITasksAcceptReward", UITasksAcceptReward },
	{ "UITasksReject", UITasksReject },
	{ "UIToggleDebugDisplay", UIToggleDebugDisplay },
	{ "UITownPortalButtonDown", UITownPortalButtonDown },
	{ "UITownPortalCancelButtonDown", UITownPortalCancelButtonDown },
	{ "UIComponentOnActivate", UIComponentOnActivate },
	{ "UIComponentOnInactivate", UIComponentOnInactivate },
	{ "UIQuestLogOnPaint", UIQuestLogOnPaint },
	{ "UIContextHelpLabelOnPaint", UIContextHelpLabelOnPaint },
	{ "UITradeOnAccept", UITradeOnAccept },
	{ "UITradeMoneyOnChar", UITradeMoneyOnChar },
	{ "UITradeOnMoneyIncrease",	UITradeOnMoneyIncrease },
	{ "UITradeOnMoneyDecrease",	UITradeOnMoneyDecrease },
	{ "UIRewardOnTakeAll",	UIRewardOnTakeAll },
	{ "UIRewardsCancel",	UIRewardsCancel },
	{ "UIComponentThrobStart",	UIComponentThrobStart },
	{ "UIComponentThrobEnd",	UIComponentThrobEnd },
	{ "UIAltMenuOnPostVisible",	UIAltMenuOnPostVisible },
	{ "UIQuestLogOnPostVisible", UIQuestLogOnPostVisible },
	{ "UIQuestCancelOnClick", UIQuestCancelOnClick },
	{ "UISkillTooltipOnSetControlStat", UISkillTooltipOnSetControlStat },
	{ "UIComponentSetVisibleFalse", UIComponentSetVisibleFalse },
	{ "UIFactionStandingsOnPaint", UIFactionStandingsOnPaint },
	{ "UIFactionStadingDispOnMouseLeave", UIFactionStadingDispOnMouseLeave },
	{ "UIFactionStadingDispOnMouseOver", UIFactionStadingDispOnMouseOver },
	{ "UIIncomingMessageOnAccept", UIIncomingMessageOnAccept },
	{ "UIMinigameSlotOnPaint", UIMinigameSlotOnPaint },
	{ "UIMinigameOnSetTicksLeft", UIMinigameOnSetTicksLeft },
	{ "UIUseProgressBarOnSetControlState", UIUseProgressBarOnSetControlState },
	{ "UIVisibleOnPlayerFocusOnly", UIVisibleOnPlayerFocusOnly },
	{ "UIComponentVisibleOnStat", UIComponentVisibleOnStat },
	{ "UIRatingLogoOnPostCreate", UIRatingLogoOnPostCreate },
	{ "UICharSelectRatingLogoOnPostActivate", UICharSelectRatingLogoOnPostActivate },
	{ "UIPingMeterOnMouseHover", UIPingMeterOnMouseHover },
	{ "UIPingMeterOnPostCreate", UIPingMeterOnPostCreate },
	{ "UIRefreshHoverItem", UIRefreshHoverItem },
	{ "UIInventoryOnPostActivate", UIInventoryOnPostActivate },
	{ "UIInventoryOnCloseButtonClicked", UIInventoryOnCloseButtonClicked },
	{ "UIQuickClose", UIQuickClosePanels },
	{ "UIComponentSetDataToInvalid", UIComponentSetDataToInvalid },
	{ "UIDrawPauseSymbol", UIDrawPauseSymbol },
	{ "UIClosePlayerInspectionPanel", UIClosePlayerInspectionPanel },
	{ "UIPlayerInspectionPanelPostInvisible", UIPlayerInspectionPanelPostInvisible },
	{ "UICharacterOptionsPanelOnPostActivate", UICharacterOptionsPanelOnPostActivate },
	{ "UIAutoPartyButtonOnPostActivate", UIAutoPartyButtonOnPostActivate },
	{ "UIAutoPartyButtonClicked", UIAutoPartyButtonClicked },
	{ "UIInspectableButtonOnPostActivate", UIInspectableButtonOnPostActivate },
	{ "UIInspectableButtonClicked", UIInspectableButtonClicked },
	{ "UIHideHelmetButtonOnPostActivate", UIHideHelmetButtonOnPostActivate },
	{ "UIHideHelmetButtonClicked", UIHideHelmetButtonClicked },
	{ "UIVersionLabelOnPostCreate", UIVersionLabelOnPostCreate },
	{ "UICharacterSheetOnPostVisible", UICharacterSheetOnPostVisible },
	{ "UIBackpackCancel", UIBackpackCancel },
	{ "UITitleComboBoxOnPostCreate", UITitleComboBoxOnPostCreate },
	{ "UITitleComboBoxOnSelect", UITitleComboBoxOnSelect },
	{ "UITitleComboBoxSetToCurrent", UITitleComboBoxSetToCurrent },
	{ "UICharLevelOnSetFocusStat", UICharLevelOnSetFocusStat },
	{ "UIDashboardSetFocusUnit", UIDashboardSetFocusUnit },
	{ "UIDashboardOnLevelChange", UIDashboardOnLevelChange },
	{ "UIDashboardOnPostActivate", UIDashboardOnPostActivate },
	{ "UICharacterSheetSetFocusUnit", UICharacterSheetSetFocusUnit },
	{ "UICharacterSheetOnLevelChange", UICharacterSheetOnLevelChange },
	{ "UICharacterSheetOnPostActivate", UICharacterSheetOnPostActivate },
	{ "UIMoneyOnChar",	UIMoneyOnChar},
	{ "UISharedStashGameModeComboOnPostCreate", UISharedStashGameModeComboOnPostCreate },
	{ "UISharedStashGameModeComboOnPostActivate", UISharedStashGameModeComboOnPostActivate },
	{ "UISharedStashGameModeComboOnChange", UISharedStashGameModeComboOnChange },

// Tugboat-specific
	{ "UIMerchantOnActivate",			UIMerchantOnActivate },
	{ "UIToggleMagnify",				UIToggleMagnify },
	{ "UIToggleWeaponSet",				UIToggleWeaponSet },
	{ "UIWeaponSetOnPaint",				UIWeaponSetOnPaint },
	{ "UITasksOnPostActivate",			UITasksOnPostActivate },
	{ "UIPetDisplayOnPaint",			UIPetDisplayOnPaint },
	{ "UITasksSelectQuest",				UITasksSelectQuest },
	{ "UIWorldMapOnPaint",				UIWorldMapOnPaint },
	{ "UIShowCharacterAndSkillsScreen",	UIShowCharacterAndSkillsScreen },
	{ "UIHideLevelUpMessage",			UIHideLevelUpMessage },
	{ "UIShowWorldMapScreen",			UIShowWorldMapScreen },
	{ "UIShowTravelMapScreen",			UIShowTravelMapScreen },
	{ "UIShowGameScreen",				UIShowGameScreen },
	{ "UIShowAutoMap",					UIShowAutoMap },
	{ "UIRepaintAutoMap",				UIRepaintAutoMap },
	{ "UIToggleVoiceChat",				UIToggleVoiceChat },
	{ "UIVoiceChatOnPostCreate",		UIVoiceChatOnPostCreate },
	{ "UIStashOnPostInvisible",			UIStashOnPostInvisible },
	{ "UITogglePaneMenu",				UITogglePaneMenu },
	{ "UISelectReward",					UISelectReward },
	{ "UISelectDialogOption",			UISelectDialogOption },
	{ "UISelectDialogLocationOption",	UISelectDialogLocationOption },
	{ "UITasksQuestMark",				UITasksQuestMark },
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddAllFontsToPak(
	void)
{
	BOOL bErrors = FALSE;

	// setup flags for searching for the texture
	DWORD dwGetFontTextureFlags = 0;
	SETBIT( dwGetFontTextureFlags, GFTF_DO_NOT_SET_IN_GLOBALS );
	SETBIT( dwGetFontTextureFlags, GFTF_IGNORE_EXISTING_GLOBAL_ENTRY );

	// scan the language datatable
	APP_GAME eAppGame = AppGameGet();
	int nNumLanguage = LanguageGetNumLanguages( eAppGame );
	for (int nLanguage = 0; nLanguage < nNumLanguage; ++nLanguage)
	{
		LANGUAGE eLanguage = (LANGUAGE)nLanguage;
		const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
		ASSERTX_CONTINUE( pLanguageData, "Expected language data" );

		// get the texture
		UIX_TEXTURE *pTexture = UIGetFontTexture( eLanguage, dwGetFontTextureFlags );
		if (pTexture == NULL)
		{
			bErrors = TRUE;
			ASSERTV_CONTINUE( 0, "Unable to load font texture for language '%s'", pLanguageData->szName );
		}

		// NOTE: We do not want to delete the texture here because it will be automatically
		// cleaned up when the ui texture dictionary is freed.  Also, we certainly don't
		// want to delete the texture that the UI is currently using

	}

	return bErrors;

}

static PRESULT sDumpIcon( UIX_TEXTURE * pTexture, const SKILL_DATA * pSkillData, const OS_PATH_CHAR * poszIconPath )
{
	ASSERT_RETINVALIDARG( pSkillData );

	ASSERT_RETFAIL( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_USES_WEAPON_ICON ) );

	UI_TEXTURE_FRAME * pFrame = NULL;

	if ( !AppIsTugboat() )
	{
		// Hellgate
		pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind( pTexture->m_pFrames, pSkillData->szLargeIcon );
	}
	else
	{
		// Mythos
		pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind( pTexture->m_pFrames, pSkillData->szLargeIcon );
	}

	if ( ! pFrame )
		return S_FALSE;

	ASSERT_RETFAIL( pFrame->m_nNumChunks >= 1 );
	ASSERT_RETFAIL( pFrame->m_pChunks );

	// bake it out
	trace( "Icon: [%s] - %dx%d, uv %3.2f,%3.2f - %3.2f,%3.2f\n",
		pSkillData->szName,
		(DWORD)( pFrame->m_pChunks[0].m_fWidth ),
		(DWORD)( pFrame->m_pChunks[0].m_fHeight ),
		pFrame->m_pChunks[0].m_fU1,
		pFrame->m_pChunks[0].m_fV1,
		pFrame->m_pChunks[0].m_fU2,
		pFrame->m_pChunks[0].m_fV2 );

	OS_PATH_CHAR oszIconFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( oszIconFile, DEFAULT_FILE_WITH_PATH_SIZE, L"%s%S%s", poszIconPath, pSkillData->szName, L".png" );

	int nW, nH;
	e_TextureGetLoadedResolution( pTexture->m_nTextureId, nW, nH );
	E_RECT tRect;
	tRect.Set(
		(LONG)(nW * pFrame->m_pChunks[0].m_fU1),
		(LONG)(nW * pFrame->m_pChunks[0].m_fV1),
		(LONG)(nW * pFrame->m_pChunks[0].m_fU1 + pFrame->m_fWidth),
		(LONG)(nW * pFrame->m_pChunks[0].m_fV1 + pFrame->m_fHeight) );
	V_RETURN( e_TextureSaveRect( pTexture->m_nTextureId, oszIconFile, &tRect, TEXTURE_SAVE_PNG, FALSE ) );

	return S_OK;
}


static int sDumpSkillIcons()
{
	OS_PATH_CHAR oszIconPath[ MAX_PATH ];
	PStrPrintf( oszIconPath, MAX_PATH, L"%S%S", FILE_PATH_SKILLS, "icons\\" );
	FileCreateDirectory( oszIconPath );



	int nSkillCount = ExcelGetNumRows( NULL, DATATABLE_SKILLS );
	for (int i = 0; i < nSkillCount; i++)
	{
		const SKILL_DATA * pSkillData = SkillGetData( NULL, i );
		if ( ! pSkillData )
			continue;

		if ( ! SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE ) )
			continue;


		const SKILLTAB_DATA *pSkillTabData = pSkillData->nSkillTab != INVALID_ID ? (const SKILLTAB_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_SKILLTABS, pSkillData->nSkillTab) : NULL;
		ASSERT_CONTINUE(pSkillTabData);
		UIX_TEXTURE* pSkillTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, pSkillTabData->szSkillIconTexture);
		ASSERT_CONTINUE(pSkillTexture);


		if (    pSkillData->szLargeIcon[ 0 ] == 0
			 || StrDictionaryFindIndex( pSkillTexture->m_pFrames, pSkillData->szLargeIcon ) == INVALID_ID
			 || PStrCmp( pSkillData->szLargeIcon, "placeholder_icon_1" ) == 0 )
			continue;

		sDumpIcon( pSkillTexture, pSkillData, oszIconPath );
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GLOBAL_INDEX sgeGlobalIndexSounds[] =
{
	GI_SOUND_ITEM_PURCHASE,
	GI_SOUND_SELL,
	GI_SOUND_POINTSPEND,
	GI_SOUND_MINIGAME_FANFARE,
	GI_SOUND_SKILL_CANNOT_SELECT,
	GI_SOUND_SKILL_SELECT,
	GI_SOUND_QUEST,
	GI_SOUND_REWARD,
	GI_SOUND_ERROR,
	GI_SOUND_DISMANTLE,
	GI_SOUND_UI_SKILLUP,
	GI_SOUND_UI_SKILL_PICKUP,
	GI_SOUND_UI_SKILL_PUTDOWN,
	GI_SOUND_UI_MONEY,
	GI_SOUND_UI_CHAR_SELECT_INVALID,
	GI_SOUND_ANALYZE_ITEM,
	GI_SOUND_DESTROY_ITEM,
	GI_SOUND_WEAPONCONFIG_1,
	GI_SOUND_WEAPONCONFIG_2,
	GI_SOUND_WEAPONCONFIG_3,
	GI_SOUND_EMAIL_SEND,
	GI_SOUND_EMAIL_RECEIVE,
};

static int sgnGlobalIndexSoundCount = arrsize(sgeGlobalIndexSounds);

static void sUIPreloadGlobalIndexSounds()
{
	for(int i=0; i<sgnGlobalIndexSoundCount; i++)
	{
		int nSoundGroup = GlobalIndexGet(sgeGlobalIndexSounds[i]);
		if(nSoundGroup >= 0)
		{
			c_SoundLoad(nSoundGroup);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIInit(
	BOOL bReloading /*= FALSE*/,
	BOOL bFillingPak /*= FALSE*/,
	int nFillPakSKU /*= INVALID_LINK*/,	// if we're filling a localized Pak, this will be the SKU we're using
	BOOL bReloadDirect /*= FALSE*/)
{
	TIMER_STARTEX("PrimeInit() - UIInit()", 100);


	g_UI.m_idCubeDisplayUnit = INVALID_ID;

	UIFree();
	UISetFlag(UI_FLAG_LOADING_UI, TRUE);

	g_UI.m_idAltTargetUnit = INVALID_ID;

	g_UI.m_nFillPakSKU = nFillPakSKU;


	// Add on the message handler functions for the local components
	int nOldSize = gpUIXmlFunctions ? sizeof(gpUIXmlFunctions) : 0;
	gnNumUIXmlFunctions = nOldSize + sizeof(gUIXmlFunctions);
	if (nOldSize > 0)
		gpUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, gpUIXmlFunctions, gnNumUIXmlFunctions);
	else
		gpUIXmlFunctions = (UI_XML_ONFUNC *)MALLOCZ(NULL, gnNumUIXmlFunctions);
	memcpy(&(gpUIXmlFunctions[nOldSize]), &(gUIXmlFunctions[0]), sizeof(gUIXmlFunctions));

	// init component types
	InitComponentTypes			(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesComplex	(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesHellgate	(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesQuest		(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesMenus		(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesSocial	(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesOptions	(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesAchievements(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesChat		(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	InitComponentTypesEmail	(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	if( AppIsTugboat() )
	{
		InitComponentTypesCrafting(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	}
	if( AppIsHellgate() )
	{
		InitComponentTypesAuction(gUIComponentTypes, gpUIXmlFunctions, gnNumUIXmlFunctions);
	}

	//read settings
	GameOptions_Load();

	UIXmlInit();
	UIXGraphicInit();
	if ( !AppIsTugboat() )
	{
		UICharacterCreateInit();
	}

	g_UI.m_dwStdAnimTime = (DWORD)(STD_ANIM_TIME * STD_ANIM_DURATION_MULT);
	structclear(g_UI_Ratios);
	g_UI.m_bRequestingReload = FALSE;

	g_UI.m_listDelayedActivateTickets.Init();
	g_UI.m_listUseCursorComponents.Init();
	g_UI.m_listUseCursorComponentsWithAlt.Init();
	for (int i=0; i < NUM_UI_MESSAGES; i++)
	{
		g_UI.m_listMessageHandlers[i].Init();
		g_UI.m_listSoundMessageHandlers[i].Init();
	}
	for (int i=0; i < NUM_UI_ANIM_CATEGORIES; i++)
	{
		g_UI.m_listAnimCategories[i].Init();
		g_UI.m_listCompsThatNeedAnimCategories[i].Init();
	}
	g_UI.m_listOverrideMouseMsgComponents.Init();
	g_UI.m_listOverrideKeyboardMsgComponents.Init();
	g_UI.m_listChatBubbles.Init();

	g_UI.m_pTextures = StrDictionaryInit(MAX_UI_TEXTURES, TRUE, UITextureDelete);

	UISetFlag(UI_FLAG_SHOW_ACC_BRACKETS, TRUE);

// Tugboat -----------------------
	g_UI.m_nDialogOpenCount = 0;
	g_UI.m_idClickedTargetUnit = INVALID_ID;
	g_UI.m_bAlwaysShowItemNames = TRUE;
// /Tugboat ----------------------
	g_UI.m_bShowItemNames = FALSE;
	g_UI.m_bGrayout = FALSE;

	g_UI.m_nDisplayWidth = AppCommonGetWindowWidth();
	g_UI.m_nDisplayHeight = AppCommonGetWindowHeight();
	float fAspectRatio = g_UI.m_nDisplayWidth / (float)g_UI.m_nDisplayHeight;
	g_UI.m_bWidescreen =  (fAspectRatio > 1.5f);

	REF(bReloading);		// might still want this later
//	g_UI.m_bReloading = bReloading;
	g_UI.m_bReloadDirect = bReloadDirect;

	// Don't bother loading UI if we're just converting files.
	if ( AppCommonIsFillpakInConvertMode() )
		return TRUE;

	UIInitLoad("ui", NULL, TRUE);
	if (g_UI.m_Components)
	{
		// temp
// #ifndef HAMMER
		if (!AppIsHammer())
		{
			UIInitLoad("cursor",			NULL,			   TRUE);
			UIInitLoad("loading_screen",	g_UI.m_Components, TRUE);
			UIInitLoad("gamestart",			g_UI.m_Components, TRUE);
			UIInitLoad("dialog",			g_UI.m_Components, TRUE);
			if( AppIsTugboat() )
			{
				UIInitLoad("email",				g_UI.m_Components);
			}
			UIInitLoad("character_select",	g_UI.m_Components, TRUE);
			if (AppIsTugboat())
			{
				UIInitLoad("anchorstone",		g_UI.m_Components, TRUE);
				UIInitLoad("runegate",			g_UI.m_Components, TRUE);
				UIInitLoad("worldmap",			g_UI.m_Components, TRUE );
			}
			UIInitLoad("townportal",		g_UI.m_Components, TRUE);
			if( AppIsHellgate() )
			{
				UIInitLoad("main_screen",		g_UI.m_Components, TRUE);
				UIInitLoad("player_matching",		g_UI.m_Components, TRUE);
			}
//			UIInitLoad("Popups",				g_UI.m_Components, TRUE);
			UIInitLoad("options",			g_UI.m_Components, TRUE);	// CHB 2007.05.24
			UIInitLoad("eula",				g_UI.m_Components, TRUE);	// CHB 2007.05.24

			if( AppIsHellgate() )
			{
				UIInitLoad("console",			g_UI.m_Components, TRUE );
				UIInitLoad("inventory_screen",	g_UI.m_Components );
				UIInitLoad("player_inspect",	g_UI.m_Components, TRUE );		// KCK: Should be able to not load immediately, but I'm not sure how this works.
				UIInitLoad("social",			g_UI.m_Components, TRUE );
				UIInitLoad("recipe",			g_UI.m_Components, TRUE );
				UIInitLoad("cube",				g_UI.m_Components, TRUE );
				UIInitLoad("itemupgrade",		g_UI.m_Components, TRUE );
				UIInitLoad("quest_panel",		g_UI.m_Components);
				UIInitLoad("movieplayer",		g_UI.m_Components, TRUE);
				UIInitLoad("email",				g_UI.m_Components);
				UIInitLoad("auction",		g_UI.m_Components);
			}

			BOOL bForceSyncLoadSkills = AppCommonGetDumpSkillIcons();
			REF( bForceSyncLoadSkills );
			UIInitLoad("skilltree",			g_UI.m_Components, AppIsTugboat() || bForceSyncLoadSkills );


			UIInitLoad("trade",				g_UI.m_Components, AppIsTugboat() );
			UIInitLoad("reward",			g_UI.m_Components, AppIsTugboat() );

			if (AppIsTugboat())
			{
				UIInitLoad("itemupgrade",		g_UI.m_Components, TRUE );
				UIInitLoad("character_screen",	g_UI.m_Components, TRUE );
				UIInitLoad("task_screen",		g_UI.m_Components, TRUE );
				UIInitLoad("achievements",		g_UI.m_Components, TRUE );
				UIInitLoad("crafting",			g_UI.m_Components, TRUE );
				//UIInitLoad("tradesman",			g_UI.m_Components );
				UIInitLoad("recipe",			g_UI.m_Components, TRUE );
				// Tug has the inventory all the way at the bottom, because it drops stuff
				// and needs to give everything else every available chance to use the item first.
				UIInitLoad("inventory_screen",	g_UI.m_Components, TRUE );
				UIInitLoad("zones",				g_UI.m_Components, TRUE );

			}
			if( AppIsTugboat() )
				UIInitLoad("main_screen",		g_UI.m_Components, TRUE);
			UIInitLoad("debug",				g_UI.m_Components, TRUE);
		}
// #else
		else
		{
			UIInitLoad("hammer",			g_UI.m_Components, TRUE);
			UIInitLoad("debug_hammer",		g_UI.m_Components );
		}
// #endif
		// end temp
	}

	UI_INIT_PROC_DISP;

	g_UI.m_bShowTargetIndicator = TRUE;
	g_UI.m_dwAutomapUpdateTicket = INVALID_ID;

	g_UI.m_pRectOrganizer = RectOrganizeInit(NULL);

	PickerInit(NULL, g_UI.m_pOutOfGamePickers);

	UIResizeWindow();
	UIMoneyInit();
	UILogLCDInit();

	if (AppIsHellgate() && !AppIsHammer())
	{
		UIChatInit();
	}

	if (AppIsTugboat())
	{
		UIPanesInit();
	}

	g_UI.m_pIncomingStore = (NPC_DIALOG_SPEC *)MALLOCZ(NULL, sizeof( NPC_DIALOG_SPEC ));
	g_UI.m_bIncoming = FALSE;

	// when filling the pak file, we load *all* the language font data
	if (bFillingPak || AppCommonGetForceLoadAllLanguages())
	{
		// This checks ERRORS for some reason. bizarre.
		if (sAddAllFontsToPak() == TRUE)
		{
			return FALSE;
		}
	}


	if( bFillingPak )
	{
		GAME* pGame = AppGetCltGame();
		// need to load and package up any loadingscreen assets
		int nRows = ExcelGetNumRows(pGame, DATATABLE_LOADING_SCREEN);

		for( int t = 0; t < nRows; t++ )
		{
			const LOADING_SCREEN_DATA* pData = (LOADING_SCREEN_DATA*)ExcelGetData(pGame, DATATABLE_LOADING_SCREEN, t);
			if( pData && PStrLen( pData->szPath )  != 0 )
			{

				char szFullName[MAX_PATH];
				PStrPrintf( szFullName, MAX_PATH, "%s%s%s", 
					FILE_PATH_DATA,
					pData->szPath,
					".png" );

				UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind( g_UI.m_pTextures, szFullName);
				if (!texture) {

					texture = UITextureLoadRaw( szFullName );
					//texture = NULL;
					if (texture) {
						StrDictionaryAdd( g_UI.m_pTextures, szFullName, texture);
					}
				}
			}
		}
	}

	UIResizeAll(g_UI.m_Components, TRUE);

	//Send first paint message
	UIHandleUIMessage(UIMSG_PAINT, 0, 0);

	UIAutoMapBeginUpdates();

	g_UI.m_bReloadDirect = FALSE;

	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT *pPlayer = GameGetControlUnit(pGame);
		if (pPlayer &&
			IsUnitDeadOrDying(pPlayer))
		{
			// re-show the restart UI

			// Close everything
			if (!AppIsTugboat())
			{
				UICloseAll();
				UIChatClose(FALSE);
			}
			else
			{
				UIHideGameMenu();
				UI_TB_HideModalDialogs();
				ConsoleSetEditActive(FALSE);
			}

			// display restart screen after a small delay
			CEVENT_DATA tEventData;
			sDisplayRestartScreen(pGame, tEventData, 0);
		}
	}


#if ISVERSION(DEVELOPMENT)
	// when dumping skill icons, just do it here
	if ( AppCommonGetDumpSkillIcons() )
	{
		int nExit = sDumpSkillIcons();

		_exit(nExit);
	}
#endif // DEVELOPMENT


	sUIPreloadGlobalIndexSounds();

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// called once after character select
void UINewSessionInit(
	void )
{
	if (AppIsHellgate())
	{
		UIChatSessionStart();

		UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_ACCOUNT_TIME_REMAIN_TEXT);
		if (pComponent)
		{
			UIComponentSetVisible(pComponent, FALSE);
		}
	}
	UIEmailInit();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// re-initialize in-game UI globals
void UIGameStartInit(
	void )
{
	//g_UI.m_bIncoming = FALSE;		// moved this to when the UI is restarting the game
	if (AppIsHellgate())
	{
		//UNIT * pPlayer = UIGetControlUnit();
		//ASSERT_RETURN(pPlayer);
		//if (UnitGetStat(pPlayer, STATS_HIDE_CHAT))
		//{
		//	UIChatClose(FALSE);
		//}
		//else
		//{
		//	UIChatOpen(FALSE);
		//}

		UIAuctionInit();
	}
	else
	{
		UIChatOpen(FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// re-initialize in-game UI globals
void UIGameRestart(
	void )
{
	g_UI.m_bIncoming = FALSE;
	UICloseAll(TRUE, TRUE);
	UIChatClose(FALSE);
	UIChatClear();
	UIComponentActivate(UICOMP_RESTART, FALSE);		// hide the restart after death component just in case.
	UIComponentActivate(UICOMP_DASHBOARD, TRUE);	// show the dashboard just in case

	UI_COMPONENT * pMain = UIComponentGetById(UIComponentGetIdByName("main"));
	if ( pMain )
	{
		UIComponentSetVisible(pMain, FALSE);
	}
	// TRAVIS : Hide our little levelup throbber
	if( AppIsTugboat() )
	{
		UI_COMPONENT * component = UIComponentGetByEnum(UICOMP_LEVELUPPANEL);
		if ( component )
		{
			UIComponentSetActive( component, FALSE );
			UIComponentSetVisible( component, FALSE );
		}
		component = UIComponentGetByEnum(UICOMP_ACHIEVEMENTUPPANEL);
		if ( component )
		{
			UIComponentSetActive( component, FALSE );
			UIComponentSetVisible( component, FALSE );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILoadComplete(
	void )
{
	sPostInitAnimCategories();

	// pull all of the components' custom message handlers into the UIs linear message handler lists in tree order
	sUIFillLinearMessageHandlerLists(g_UI.m_Cursor);
	sUIFillLinearMessageHandlerLists(g_UI.m_Components);

	g_UI.m_bPostLoadExecuted = TRUE;
	UISetFlag(UI_FLAG_LOADING_UI, FALSE);
	if ( AppGetCltGame() )
	{
		UnitRecreateIcons( AppGetCltGame() );
		UNITID idControlUnit = UnitGetId(UIGetControlUnit());
		UISendMessage(WM_SETCONTROLUNIT, idControlUnit, 0);
		UISendMessage(WM_INVENTORYCHANGE, idControlUnit, -1);	//force redraw of the inventory
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIShowingLoadingScreen()
{
	return g_UI.m_eLoadScreenState != LOAD_SCREEN_DOWN;
}
static BOOL sbUpdatingLoadingScreen = FALSE;
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIUpdatingLoadingScreen()
{
	return sbUpdatingLoadingScreen;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowLoadingScreen(
	int main_as_well,
	const WCHAR * name, /*= L"" */
	const WCHAR * subtext,
	BOOL bShowLoadingTip /*= TRUE */ )
{
	BOOL bForceRender = FALSE;
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_LOADINGSCREEN);
	if (component)
	{
		if (!UIShowingLoadingScreen())
		{
			// close everything else that might be open
			if( AppIsHellgate() )
			{
				//UICloseAll();	//This function closes all sorts of UI elements that Tugboat doesn't want invisible ( money for instance ).
				UISetCursorState(UICURSOR_STATE_WAIT);
				UIClearHoverText(UICOMP_INVALID, TRUE);
			}
			else
			{
				UISetCursorState(UICURSOR_STATE_WAIT);
				UI_TB_HideAllMenus();
				bForceRender = TRUE;
			}
			UIComponentSetActive(component, TRUE);

			g_UI.m_eLoadScreenState = LOAD_SCREEN_UP;
		}

		UI_COMPONENT *pChild = UIComponentFindChildByName(component, "level change text");
		if ( pChild )
		{
			if( AppIsTugboat())
			{
				if( PStrLen( name ) )
				{
					UILabelSetText( pChild, name );
				}
				else
				{
					UILabelSetText( pChild, GlobalStringGet( GS_LOADING ) );
				}
			}
			else
			{
				UILabelSetText( pChild, name );

			}
		}

		pChild = UIComponentFindChildByName(component, "level change subtext");
		if ( pChild )
			UILabelSetText( pChild, subtext );

		// code for flipping between individual loading screen textures, and throwing
		// old ones away, so that we can actually afford to do it.
		if( AppIsTugboat() )
		{
			pChild = UIComponentFindChildByName(component, "loading_image");
			if ( pChild )
			{
				GAME *pGame = AppGetCltGame();
				int nRows = ExcelGetNumRows(pGame, DATATABLE_LOADING_SCREEN);
				if (nRows > 0)
				{
					UNIT * pPlayer = (pGame ? GameGetControlUnit( pGame ) : NULL);
					static int snLastScreen = INVALID_ID;
					int nLoadingScreenToUse = INVALID_LINK;
					static TIME snLoadingScreenTimer = AppCommonGetAbsTime();

					if( AppCommonGetAbsTime() >= snLoadingScreenTimer)
					{
						sbUpdatingLoadingScreen = TRUE;
						PickerStartCommon(g_UI.m_pOutOfGamePickers, picker);
						for( int t = 0; t < nRows; t++ )
						{
							const LOADING_SCREEN_DATA* pData = (LOADING_SCREEN_DATA*)ExcelGetData(pGame, DATATABLE_LOADING_SCREEN, t);
							int code_len = 0;
							if (t != snLastScreen)
							{
								BYTE * code_ptr = ExcelGetScriptCode( NULL, DATATABLE_LOADING_SCREEN, pData->codeCondition, &code_len );
								if (!code_ptr || !pGame ||
									(pGame && pPlayer &&
									VMExecI( pGame, pPlayer, NULL, 0, 0, code_ptr, code_len )))
								{
									PickerAdd(MEMORY_FUNC_FILELINE(g_UI.m_pOutOfGamePickers, NULL, t, pData->nWeight));
								}
							}
						}

						if (PickerGetCount(g_UI.m_pOutOfGamePickers) > 0)
						{
							RAND tRand;
							RandInit( tRand, GetTickCount() );
							nLoadingScreenToUse = PickerChoose(g_UI.m_pOutOfGamePickers, tRand);

							const LOADING_SCREEN_DATA* pLastData = NULL;
							if( snLastScreen != INVALID_ID )
							{
								pLastData = (LOADING_SCREEN_DATA*)ExcelGetData(pGame, DATATABLE_LOADING_SCREEN, snLastScreen);
							}
							const LOADING_SCREEN_DATA* pData = (LOADING_SCREEN_DATA*)ExcelGetData(pGame, DATATABLE_LOADING_SCREEN, nLoadingScreenToUse);
							snLastScreen = nLoadingScreenToUse;

							// Throw away the old texture, if there was one
							if( pLastData && PStrLen( pLastData->szPath )  != 0 )
							{
								char szFullName[MAX_PATH];
								PStrPrintf( szFullName, MAX_PATH, "%s%s%s", 
									FILE_PATH_DATA,
									pData->szPath,
									".png" );
								UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind( g_UI.m_pTextures, szFullName);
								if( texture )
								{
									// Be sure to remove the actual texture resource, and actually remove it, not just the ref
									e_TextureReleaseRef(texture->m_nTextureId, TRUE);
									texture->m_nTextureId = INVALID_ID;

								}
							}

							
							// load up our new texture - synchronously! We're loading anyway. We want to show it NOW.
							if( pData && PStrLen( pData->szPath )  != 0 )
							{


								char szFullName[MAX_PATH];
								PStrPrintf( szFullName, MAX_PATH, "%s%s%s", 
									FILE_PATH_DATA,
									pData->szPath,
									".png" );
								UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind( g_UI.m_pTextures, szFullName);

								// old texture we dumped - let's fill it in again
								if( texture && texture->m_nTextureId == INVALID_ID )
								{
									UIX_TEXTURE* pTempTexture = UITextureLoadRaw( szFullName, 1600, 1200, FALSE );
									texture->m_nTextureId = pTempTexture->m_nTextureId;
									FREE( NULL, pTempTexture );
								}
								else if (!texture)
								{
									texture = UITextureLoadRaw( szFullName, 1600, 1200, FALSE );
									if( texture )
									{
										StrDictionaryAdd( g_UI.m_pTextures, szFullName, texture);
									}
								}
								if( texture )
								{
									UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "icon");
									if (frame)
									{
										// always 800x600 for Mythos
										frame->m_fWidth = 800;
										frame->m_fHeight = 600;
										UI_SIZE size;
										size.m_fWidth = component->m_fWidth / component->m_fXmlWidthRatio;
										size.m_fHeight = component->m_fHeight / component->m_fXmlHeightRatio / ( g_UI.m_bWidescreen ? 1.2f : 1.0f );
										UIComponentRemoveAllElements( pChild );
										UIComponentAddElement(pChild, texture, frame, UI_POSITION(0,0) , GFXCOLOR_WHITE, NULL, 0, 1.0f, 1.0f, &size  );
										snLoadingScreenTimer = ( AppCommonGetAbsTime() + 10000 );
										bForceRender = TRUE;
										UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
									}
								}
								
							}
						}
					}
				}
			}
		}


		pChild = UIComponentFindChildByName(component, "loading tip");
		if ( pChild )
		{
			UILabelSetText( pChild, L"" );
			if (bShowLoadingTip)
			{
				GAME *pGame = AppGetCltGame();
				int nRows = ExcelGetNumRows(pGame, DATATABLE_LOADING_TIPS);
				if (nRows > 0)
				{
					UNIT * pPlayer = (pGame ? GameGetControlUnit( pGame ) : NULL);
					static int snLastTip = INVALID_ID;
					int nLoadingTipToUse = INVALID_LINK;

					PickerStartCommon(g_UI.m_pOutOfGamePickers, picker);
					for( int t = 0; t < nRows; t++ )
					{
						const LOADING_TIP_DATA* pData = (LOADING_TIP_DATA*)ExcelGetData(pGame, DATATABLE_LOADING_TIPS, t);
						int code_len = 0;
						if (pData->nStringKey != INVALID_LINK &&
							t != snLastTip)
						{
							BYTE * code_ptr = ExcelGetScriptCode( NULL, DATATABLE_LOADING_TIPS, pData->codeCondition, &code_len );
							if (!code_ptr || !pData->bDontUseWithoutAGame ||
								(pGame && pPlayer &&
								VMExecI( pGame, pPlayer, NULL, 0, 0, code_ptr, code_len )))
							{
								PickerAdd(MEMORY_FUNC_FILELINE(g_UI.m_pOutOfGamePickers, NULL, t, pData->nWeight));
							}
						}
					}

					if (PickerGetCount(g_UI.m_pOutOfGamePickers) > 0)
					{
						RAND tRand;
						RandInit( tRand, GetTickCount() );
						nLoadingTipToUse = PickerChoose(g_UI.m_pOutOfGamePickers, tRand);

						const LOADING_TIP_DATA* pData = (LOADING_TIP_DATA*)ExcelGetData(pGame, DATATABLE_LOADING_TIPS, nLoadingTipToUse);
						snLastTip = nLoadingTipToUse;
						ASSERT_RETURN(pData);
						UILabelSetText( pChild, StringTableGetStringByIndex(pData->nStringKey) );
					}
				}
			}
		}

		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}
	if ( main_as_well )
	{
		UI_COMPONENT * main = UIComponentGetById(UIComponentGetIdByName("main"));
		if ( main )
		{
			UIComponentSetActive( main, TRUE );
//			UISetNeedToRender();
		}
	}

	if( name && PStrLen( name ) )
	{
		component = UIComponentGetByEnum(UICOMP_LEVELNAME);
		if (!component)
		{
			return;
		}

		UI_COMPONENT *pChild  = component->m_pFirstChild;
		while ( pChild )
		{
			if ( ( pChild->m_eComponentType == UITYPE_LABEL ) && ( PStrCmp( pChild->m_szName, "level name text" ) == 0 ) )
			{
				UILabelSetText( pChild->m_idComponent, name );
			}
			pChild = pChild->m_pNextSibling;
		}
	}
	sbUpdatingLoadingScreen = FALSE;
	if( bForceRender )
	{
		c_CameraUpdate(NULL, 0);

		CameraSetUpdated(TRUE);

		V( e_Update() );
		V( e_RenderUIOnly( TRUE ) );
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateLoadingScreen(
	void)
{
	// no loading screen update in Hammer (for now)
	if ( ! c_GetToolMode() )
	{
		AppLoadingScreenUpdate();
		UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_LOADINGSCREEN);
		if (component)
		{
			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
		}
	}
}

static void sLoadScreenFadeCallback()
{
	V( e_BeginFade( FADE_IN, UI_POSTLOAD_FADEIN_MS, 0x00000000 ) );
	g_UI.m_eLoadScreenState = LOAD_SCREEN_CAN_HIDE;
	UIHideLoadingScreen( FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideLoadingScreen(
	BOOL bFadeOut /*= TRUE*/)
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_LOADINGSCREEN);
	if (bFadeOut)
	{
		if ( g_UI.m_eLoadScreenState == LOAD_SCREEN_UP )
		{
			// begin fade out
			g_UI.m_eLoadScreenState = LOAD_SCREEN_FORCE_UP;

			void (*pfnFadeCallback)() = sLoadScreenFadeCallback;
			DWORD dwFadeoutMS = UI_POSTLOAD_FADEOUT_MS;
			if ( AppCommonGetDemoMode() )
			{
				pfnFadeCallback = DemoLevelLoadScreenFadeCallback;
				dwFadeoutMS = 2000;
			}

			V( e_BeginFade( FADE_OUT, dwFadeoutMS, 0x00000000, pfnFadeCallback ) );
			return;
		}

		if(g_UI.m_eLoadScreenState == LOAD_SCREEN_FORCE_UP)
		{
			// We're already fading out...don't interrupt that process
			return;
		}

		// check state, and only hide if it says we can
		if ( g_UI.m_eLoadScreenState != LOAD_SCREEN_CAN_HIDE )
		{
			return;
		}
	}
	else
	{
		e_SetUIOnlyRender( FALSE );
	}

	if (component)
	{
		UIComponentSetVisible(component, FALSE);
	}

	g_UI.m_eLoadScreenState = LOAD_SCREEN_DOWN;
	UISetCursorState(UICURSOR_STATE_POINTER);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowProgressChange(LPCWSTR strProgress) {
	UI_COMPONENT* component = NULL;

	component = UIComponentGetByEnum(UICOMP_LOADINGSCREEN);
	if (!component) {
		return;
	}
	UI_COMPONENT* pChild = UIComponentFindChildByName(component, "level progress text");
	if ( pChild && pChild->m_eComponentType == UITYPE_LABEL )
	{
		UILabelSetText(pChild->m_idComponent, strProgress);
		UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
	}

	pChild = UIComponentFindChildByName(component, "level progress bar");
	if ( pChild && pChild->m_eComponentType == UITYPE_BAR )
	{
		UI_BAR* pBar = UICastToBar( pChild );

		 if( PatchClientExePatchTotalFileCount() < 2 )
		 {
			 pBar->m_nValue = PatchClientGetProgress();
			 pBar->m_nMaxValue = 100;
		 }
		 else
		 {
			 pBar->m_nValue = PatchClientExePatchCurrentFileCount();
			 pBar->m_nMaxValue = PatchClientExePatchTotalFileCount();
		 }



		 UIComponentHandleUIMessage(pBar, UIMSG_PAINT, 0, 0);
	}
//	UIComponentSetActive( component, TRUE );
//	AppLoadingScreenUpdate();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowProgressChange(LPCSTR strProgress) {
	WCHAR strProgressW[DEFAULT_FILE_WITH_PATH_SIZE * 2];
	PStrCvt(strProgressW, strProgress, DEFAULT_FILE_WITH_PATH_SIZE * 2);
	UIShowProgressChange(strProgressW);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowPatchClientProgressChange()
{
	WCHAR buffer[2*DEFAULT_FILE_WITH_PATH_SIZE];

	PatchClientGetUIProgressMessage(buffer, 2*DEFAULT_FILE_WITH_PATH_SIZE);
	UIShowProgressChange(buffer);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowSubtitle(
	const WCHAR * szText)
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_SUBTITLE);
	if (!component)
	{
		return;
	}

	UILabelSetText( component, szText );

	if (!szText || !szText[0])
	{
		UIComponentSetVisible( component, FALSE );
	}
	else
	{
		UIComponentSetActive( component, TRUE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowPausedMessage(
	BOOL bShow)
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_MOVIE_PAUSED);
	if (!component)
	{
		return;
	}

	UIComponentActivate(component, bShow);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDrawPauseSymbol(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UIComponentRemoveAllElements(component);

	if (component->m_fWidth > 0.0f && component->m_fHeight > 0.0f)
	{
		BYTE byAlpha = UIComponentGetAlpha(component);
		DWORD dwColor2 = UI_MAKE_COLOR(byAlpha, 0x00);
		UI_RECT rect;
		rect.m_fX2 = component->m_fWidth / 3.0f;
		rect.m_fY2 = component->m_fHeight;
		UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, rect, NULL, NULL, dwColor2);

		UI_RECT rect2 = rect;
		rect2.Translate((component->m_fWidth * 2.0f)/ 3.0f, 0.0f);
		UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, rect2, NULL, NULL, dwColor2);

		rect.Translate(component->m_fWidth / -40.0f, component->m_fWidth / -40.0f);
		rect2.Translate(component->m_fWidth / -40.0f, component->m_fWidth / -40.0f);
		UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, rect, NULL, NULL, component->m_dwColor);
		UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, rect2, NULL, NULL, component->m_dwColor);
	}
	return UIMSG_RET_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowQuickMessage(
	const WCHAR * szText,
	float fFadeOutMultiplier /*= 0.0f */,
	const WCHAR * szSubtext /*= L""*/ )
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_FLOATING_MESSAGE);
	if (!component)
	{
		return;
	}

	if ( component->m_eState == UISTATE_ACTIVE )
	{
		OutputDebugString( "\n********** Floating message already active **************\n\n" );
	}

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while ( pChild )
	{
		if ( ( pChild->m_eComponentType == UITYPE_LABEL ) && ( PStrCmp( pChild->m_szName, "message text" ) == 0 ) )
		{
			UILabelSetText( pChild->m_idComponent, szText );
		}
		if ( ( pChild->m_eComponentType == UITYPE_LABEL ) && ( PStrCmp( pChild->m_szName, "message subtext" ) == 0 ) )
		{
			UILabelSetText( pChild->m_idComponent, szSubtext );
		}
		pChild = pChild->m_pNextSibling;
	}

	if ((!szText || !szText[0]) &&
		(!szSubtext || !szSubtext[0]))
	{
		UIComponentSetVisible(component, FALSE);
	}
	else
	{
		UIComponentActivateQuickMessage(component, fFadeOutMultiplier);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowQuickMessage(
	const char * szText,
	float fFadeOutMultiplier /*= 0.0f*/,
	const char * szSubtext /*= L""*/ )
{
	WCHAR szTemp[256];
	PStrCvt( szTemp, szText, 256 );
	WCHAR szTemp2[256];
	PStrCvt( szTemp2, szSubtext, 256 );
	UIShowQuickMessage( szTemp, fFadeOutMultiplier, szTemp2 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIComponentActivateQuickMessageEvent(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT * component = (UI_COMPONENT*)data.m_Data1;
	float fFadeOutMultiplier = (float)data.m_Data2;
	UIComponentActivateQuickMessage(component, fFadeOutMultiplier);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentActivateQuickMessage(
	UI_COMPONENT* component,
	float fFadeOutMultiplier)
{
	if (g_UI.m_eLoadScreenState == LOAD_SCREEN_DOWN)
	{
		// remove any old events
		if ( component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID )
		{
			CSchedulerCancelEvent( component->m_tMainAnimInfo.m_dwAnimTicket );
		}

		UIComponentSetActive( component, TRUE );

		if ( fFadeOutMultiplier != 0.0f )
		{
			DWORD dwOldAnimDuration = component->m_dwAnimDuration;
			component->m_dwAnimDuration = (DWORD)((float)component->m_dwAnimDuration * fFadeOutMultiplier);
			UIComponentActivate(component, FALSE);
			component->m_dwAnimDuration = dwOldAnimDuration;
		}
	}
	else
	{
		// schedule another check
		CEVENT_DATA data;
		data.m_Data1 = (DWORD_PTR)component;
		data.m_Data2 = (DWORD_PTR)fFadeOutMultiplier;

		CSchedulerRegisterEvent(
			AppCommonGetCurTime() + MSECS_PER_SEC/2,
			sUIComponentActivateQuickMessageEvent,
			data);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowScore( const WCHAR * text, BOOL bVisible )
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_QUESTGAMESCORE);
	if (!component)
	{
		return;
	}

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while ( pChild )
	{
		if ( ( pChild->m_eComponentType == UITYPE_LABEL ) && ( PStrCmp( pChild->m_szName, "score text" ) == 0 ) )
		{
			UILabelSetText( pChild->m_idComponent, text );
		}
		pChild = pChild->m_pNextSibling;
	}

	if ( bVisible )
		UIComponentSetActive( component, TRUE );
	else
		UIComponentSetVisible( component, FALSE );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define TUTORIALTIP_FADEOUTMULT_DEFAULT			2.5f

BOOL sTutorialFadeClear( GAME * pGame, UNIT * pUnit, const struct EVENT_DATA & event_data )
{
	MSG_CCMD_TUTORIAL_UPDATE message;
	message.nType = TUTORIAL_MSG_TIP_DONE;
	message.nData = (int)event_data.m_Data1;
	c_SendMessage( CCMD_TUTORIAL_UPDATE, &message );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowMessage(
	int nType,
	int nDialog,
	int nParam1,
	int nParam2,
	DWORD dwFlags )
{
	float fFade = 0.0f;
	if ( dwFlags == UISMS_FADEOUT )
		fFade = LEVELCHANGE_FADEOUTMULT_DEFAULT;

	WCHAR tempstr[256] = L"\0";

	switch ( nType )
	{
		//----------------------------------------------------------------------------
		case QUIM_DIALOG:
		{
			const WCHAR * msg = c_DialogTranslate( nDialog );
			if ( dwFlags == UISMS_DIALOG_QUEST )
			{
				// get format string
				WCHAR uszTextBuffer[ MAX_QUESTLOG_STRLEN ];
				uszTextBuffer[ 0 ] = 0;
				GAME * pGame = AppGetCltGame();
				ASSERT_RETURN( pGame );
				UNIT * pPlayer = GameGetControlUnit( pGame );
				ASSERT_RETURN( pPlayer );
				QUEST * pQuest = QuestGetQuest( pPlayer, nParam1 );
				c_QuestReplaceStringTokens( pPlayer, pQuest, msg, uszTextBuffer, arrsize( uszTextBuffer ) );
				if ( nParam2 != -1 )
				{
					fFade = (float)nParam2 / GAME_TICKS_PER_SECOND_FLOAT;
				}
				UIShowQuickMessage( &uszTextBuffer[0], fFade );
			}
			else
				UIShowQuickMessage( msg, fFade );
			break;
		}

		//----------------------------------------------------------------------------
		case QUIM_STRING:
		{
			const WCHAR * msg = StringTableGetStringByIndex( nDialog );
			if ( nParam1 != -1 )
			{
				fFade = (float)nParam1 / GAME_TICKS_PER_SECOND_FLOAT;
			}
			UIShowQuickMessage( msg, fFade );
			break;
		}

		//----------------------------------------------------------------------------
		case QUIM_GLOBAL_STRING:
		{
			PStrCopy( tempstr, GlobalStringGet( (GLOBAL_STRING)nDialog ), 256 );
			if ( nParam1 != -1 )
			{
				fFade = (float)nParam1 / GAME_TICKS_PER_SECOND_FLOAT;
			}
			UIShowQuickMessage( tempstr, fFade );
			break;
		}

		//----------------------------------------------------------------------------
		case QUIM_GLOBAL_STRING_WITH_INT:
		{
			const WCHAR *puszFormat = GlobalStringGet( (GLOBAL_STRING)nDialog );
			PStrPrintf( tempstr, 256, puszFormat, nParam1 );
			UIShowQuickMessage( tempstr, fFade );
			break;
		}

		//----------------------------------------------------------------------------
		case QUIM_GLOBAL_STRING_SCORE:
		{
			BOOL bVisible = TRUE;
			if ( dwFlags == UISMS_FORCEOFF )
			{
				bVisible = FALSE;
			}
			else
			{
				const WCHAR *puszFormat = GlobalStringGet( (GLOBAL_STRING)nDialog );
				PStrPrintf( tempstr, 256, puszFormat, nParam1, nParam2 );
			}
			UIShowScore( tempstr, bVisible );
			break;
		}

		//----------------------------------------------------------------------------
		case QUIM_TUTORIAL_TIP:
		{
			PStrCopy( tempstr, GlobalStringGet( (GLOBAL_STRING)nDialog ), 256 );
			// if it fades, set callback to send clear
			GAME * game = AppGetCltGame();
			ASSERT_RETURN( game );
			UNIT * player = GameGetControlUnit( game );
			ASSERT_RETURN( player );
			// get rid of old stuff
			if ( UnitHasTimedEvent( player, sTutorialFadeClear ) )
			{
				UnitUnregisterTimedEvent( player, sTutorialFadeClear );
			}
			if ( nParam1 != -1 )
			{
				UnitRegisterEventTimed( player, sTutorialFadeClear, EVENT_DATA( nParam2 ), nParam1 );
				fFade = (float)nParam1 / GAME_TICKS_PER_SECOND_FLOAT;
			}
			c_TutorialSetClientMessage( player, nParam2 );
			UIShowQuickMessage( tempstr, fFade );
			break;
		}

	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIShowLevelUpMessage( int nNewLevel, GLOBAL_STRING eGlobalStringIndex )
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_LEVELUPTEXT);
	if (!component)
	{
		return;
	}
	UI_LABELEX *pLabel = UICastToLabel(component);

	{
		WCHAR szTemp[ 256 ];
		PStrPrintf( szTemp, 256, GlobalStringGet( eGlobalStringIndex ), nNewLevel );
		UILabelSetText( pLabel, szTemp );
	}

	UIComponentActivate(component, TRUE);		//pop it up (m_bFadesIn == FALSE)
	UIComponentActivate(component, FALSE);		//fade it out (m_bFadesOut == TRUE)
	//DWORD dwMaxAnimDuration = 0;
	//UIComponentFadeOut(UIComponentGetByEnum(UICOMP_LEVELUPTEXT), 1.0f, dwMaxAnimDuration);

	// put the message over the portrait
	if( AppIsHellgate() )
	{
		component = UIComponentGetByEnum(UICOMP_PORTRAIT);
		UI_COMPONENT* pChild = UIComponentFindChildByName(component, "level up panel");
		if (pChild)
		{

			UIComponentSetVisible(pChild, TRUE);
		}
	}
	else if( AppIsTugboat() )
	{
		component = UIComponentGetByEnum(UICOMP_LEVELUPPANEL);
		if ( component )
		{
			UIComponentSetActive( component, TRUE );
			UIComponentSetVisible( component, TRUE );
			//UIComponentThrobStart( component, 0xffffffff, 2000 );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowLevelUpMessage( int nNewLevel )
{
	if (nNewLevel < 2)
	{
		return;
	}

	sUIShowLevelUpMessage( nNewLevel, GS_LEVELUP );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowRankUpMessage( int nNewRank )
{
	if (nNewRank < 1)
	{
		return;
	}

	sUIShowLevelUpMessage( nNewRank, GS_RANKUP );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowAchievementUpMessage( )
{
	if( AppIsTugboat() )
	{
		UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_ACHIEVEMENTUPPANEL);
		if ( component )
		{
			UIComponentSetActive( component, TRUE );
			UIComponentSetVisible( component, TRUE );
			UIComponentThrobStart( component, 0xffffffff, 2000 );
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIHideQuickMessage()
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_FLOATING_MESSAGE);
	if (!component)
	{
		return FALSE;
	}
	if ( !component->m_bVisible )
	{
		return FALSE;
	}

	UIComponentSetVisible( component, FALSE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIFadeOutQuickMessage( float fFadeOutTime )
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_FLOATING_MESSAGE);
	if (!component)
	{
		return FALSE;
	}
	if ( !component->m_bVisible )
	{
		return FALSE;
	}

	DWORD dwOldAnimDuration = component->m_dwAnimDuration;
	component->m_dwAnimDuration = (DWORD)((float)component->m_dwAnimDuration * fFadeOutTime);
	UIComponentActivate(component, FALSE);
	component->m_dwAnimDuration = dwOldAnimDuration;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIGetHoverUnit(
	int* setby)
{
	if (setby)
	{
		*setby = g_UI.m_idHoverUnitSetBy;
	}
	return g_UI.m_idHoverUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sClearHoverText(UI_COMPONENT_ENUM eComponent)
{
	UI_COMPONENT* pComponent = UIComponentGetByEnum(eComponent);
	if( AppIsTugboat() && !pComponent )
	{
		return;
	}
	ASSERT_RETURN(pComponent);
	UI_TOOLTIP *pItemTooltip = UICastToTooltip(pComponent);

	//UIComponentSetFocusUnit(pItemTooltip, INVALID_ID);

	pItemTooltip->m_idFocusUnit = INVALID_ID;	// It's set explicitly here, with no function call, so the children aren't told to repaint (which will blank them).
	UIComponentActivate(pItemTooltip, FALSE);
}

void UIClearHoverText(
	UI_COMPONENT_ENUM eExcept /*= UICOMP_INVALID*/,
	BOOL bImmediate /*= FALSE*/)
{
	UI_COMPONENT *pExcept = (eExcept != UICOMP_INVALID ? UIComponentGetByEnum(eExcept) : NULL);
	PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_TOOLTIPS].GetNext(pNode)) != NULL)
	{
		if (pNode->Value != pExcept)
		{
			// taking this out because it was leading to tooltips being repainted with no focus unit
			//pNode->Value->m_idFocusUnit = INVALID_ID;	// It's set explicitly here, with no function call, so the children aren't told to repaint (which will blank them).
			UIComponentActivate(pNode->Value, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
		}
	}
}

// I think only Mythos uses this now
void UIClosePopupMenus(
	void)
{
	UI_COMPONENT_ENUM eComponent[] = {UICOMP_PARTYMEMBER_POPUP_MENU, UICOMP_PET_POPUP_MENU, UICOMP_CHATCOMMAND_POPUP_MENU, UICOMP_EMOTECOMMAND_POPUP_MENU, UICOMP_PLAYER_INTERACT_POPUP_MENU, UICOMP_TRAVEL_POPUP_MENU };

	for (int i=0; i < arrsize(eComponent); i++)
	{
		UIComponentHandleUIMessage(UIComponentGetByEnum(eComponent[i]), UIMSG_INACTIVATE, 0, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIClearHoverText(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIClearHoverText();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetHoverUnit(
	UNITID idHover,
	int idComponent)
{

	if (g_UI.m_idHoverUnit == idHover)
	{
		return;
	}

	if (idHover == INVALID_ID)
	{
		UIClearHoverText();
	}
	else
	{
		// Clear the other tooltip
		UI_COMPONENT* pStatsTooltip = UIComponentGetByEnum(UICOMP_STATSTOOLTIP);
		if (pStatsTooltip &&
			UIComponentGetVisible(pStatsTooltip))
		{
			UIComponentActivate(pStatsTooltip, FALSE);
		}
	}

	if (!AppIsTugboat())
	{
		if (UIGetCursorState() == UICURSOR_STATE_POINTER &&
			idHover != INVALID_ID)
		{
			UNIT *pHoverUnit = UnitGetById(AppGetCltGame(), idHover);
			ASSERT_RETURN(pHoverUnit);
			if (!UnitGetStat(pHoverUnit, STATS_ITEM_LOCKED_IN_INVENTORY) &&
				UIComponentGetActive(UIComponentGetById(idComponent)))
			{
				UISetCursorState(UICURSOR_STATE_OVERITEM);
			}
		}
		else if (UIGetCursorState() == UICURSOR_STATE_OVERITEM &&
			idHover == INVALID_ID)
		{
			UISetCursorState(UICURSOR_STATE_POINTER);
		}
	}

	g_UI.m_idHoverUnit = idHover;
	g_UI.m_idHoverUnitSetBy = idComponent;
	UISendMessage(WM_UISETHOVERUNIT, idHover, idComponent);
}

void UISetHoverUnit(
	UNITID idHover)
{
	UISetHoverUnit(idHover, INVALID_ID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void UIComponentUpdatePosition(
	UI_COMPONENT* component,
	const UI_POSITION& StartPos,
	const UI_POSITION& EndPos,
	float fTimeRatio)
{
	if (fTimeRatio >= 1.0f)
	{
		component->m_Position = EndPos;
	}
	else
	{
		component->m_Position.m_fX = StartPos.m_fX + (EndPos.m_fX - StartPos.m_fX) * fTimeRatio;
		component->m_Position.m_fY = StartPos.m_fY + (EndPos.m_fY - StartPos.m_fY) * fTimeRatio;
	}
	UISetNeedToRender(component);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentSetVisible(
	UI_COMPONENT* component,
	BOOL bVisible,
	BOOL bEVERYONE /*= FALSE*/)
{
	if (!component)
	{
		return;
	}

	// if this component has a script to evaluate to determine if it can be visible or not
	//if (component->m_codeVisibleWhen)
	//{
	//	UNIT *pUnit = UIComponentGetFocusUnit(component);
	//	int nResult = VMExecI(AppGetCltGame(), pUnit, g_UI.m_pCodeBuffer + component->m_codeVisibleWhen, g_UI.m_nCodeCurr - component->m_codeVisibleWhen);
	//	if ((nResult != 0) != bVisible)
	//	{
	//		return;
	//	}
	//}

	if (bVisible == component->m_bVisible)
	{
		return;
	}
	component->m_bVisible = bVisible;

	if (component->m_bVisible)
	{
		UIComponentHandleUIMessage(component, UIMSG_POSTVISIBLE, 0, 0);
	}
	else
	{
		UIComponentHandleUIMessage(component, UIMSG_POSTINVISIBLE, 0, 0);
	}

	// this may be unnecessary now that we have the post visible messages
	if (component->m_bNeedsRepaintOnVisible)
	{
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}

	//omg this is sooo hacky!  I would do this with polymorphism, but we could do it with messages .... sloow
	//if (component->m_eComponentType == UITYPE_BAR)
	//{
	//	UI_BAR *bar = UICastToBar(component);
	//	bar->m_fOldRatio = -1.0f;			// gotta reset this so the bar will redraw
	//}

	UI_COMPONENT *pChilde = component->m_pFirstChild;
	while(pChilde)
	{
		if (!pChilde->m_bIndependentActivate || bEVERYONE)
		{
			UIComponentSetVisible(pChilde, bVisible, bEVERYONE);
		}
		pChilde = pChilde->m_pNextSibling;
	}

	if (!bVisible)
	{
		UIComponentSetActive(component, FALSE);

		// if there's an auto-tooltip up for this component, hide it
		UI_COMPONENT *pSimpleTooltip = UIComponentGetByEnum(UICOMP_SIMPLETOOLTIP);
		if (pSimpleTooltip &&
			(int)pSimpleTooltip->m_dwData == component->m_idComponent &&
			UIComponentGetVisible(pSimpleTooltip))
		{
			UIComponentActivate(pSimpleTooltip, FALSE);
		}
	}

	UISetNeedToRender(component);

	// the loading screen doesn't properly remove the texture when we take it down
	// when coming into a game, so this is a stop-gap hack to make it happen
	// Talk to Brennan/Chris/Colin for more info
	if (g_UI.m_eLoadScreenState != LOAD_SCREEN_DOWN)
	{
		UIClearNeedToRender(component->m_nRenderSection);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentSetVisibleByEnum(
	UI_COMPONENT_ENUM eComp,
	BOOL bVisible,
	BOOL bEVERYONE /*= FALSE*/)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(eComp);
	if (pComponent)
		UIComponentSetVisible(pComponent, bVisible, bEVERYONE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentSetActiveByEnum(
	UI_COMPONENT_ENUM eComp,
	BOOL bActive)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(eComp);
	if (pComponent)
		UIComponentSetActive(pComponent, bActive);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIComponentGetVisibleByEnum(
	UI_COMPONENT_ENUM eComp)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(eComp);
	if (pComponent)
		return UIComponentGetVisible(pComponent);
	else
		return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIComponentGetActiveByEnum(
	UI_COMPONENT_ENUM eComp)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(eComp);
	if (pComponent)
		return UIComponentGetActive(pComponent);
	else
		return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentThrob(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}

	const TIME tiStart = component->m_tThrobAnimInfo.m_tiAnimStart;
	DWORD dwDuration = (DWORD)data.m_Data2;
	DWORD dwPeriodMS = (DWORD)data.m_Data3;
	BOOL bReverse = (BOOL)data.m_Data4;

	TIME tiElapsed = AppCommonGetCurTime() - tiStart;
	float fTimeRatio = FMODF((float)tiElapsed, (float)dwPeriodMS) / (float)dwPeriodMS;
	if (bReverse)
		fTimeRatio += 0.5f;
	float fFading = fabs(1.0f - (fTimeRatio * 2.0f));

	//trace("fading	%0.4f\n", fFading);
	if (dwDuration != 0xFFFFFFFF && tiElapsed >= (int)dwDuration)
	{
		fFading = bReverse ? 1.0f : 0.0f;
	}

	UIComponentSetFade(component, fFading);
	UISetNeedToRender(component);
	if (component->m_tThrobAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tThrobAnimInfo.m_dwAnimTicket);
	}

	if (dwDuration == 0xFFFFFFFF || AppCommonGetCurTime() - tiStart < (int)dwDuration)
	{
		component->m_tThrobAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentThrob, data);
		return;
	}


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentThrobStart(
	UI_COMPONENT * pComponent,
	DWORD dwDurationMS,
	DWORD dwPeriodMS,
	BOOL bReverse /*= FALSE*/)
{
	pComponent->m_tThrobAnimInfo.m_tiAnimStart = AppCommonGetCurTime();
	UIComponentThrob(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pComponent, dwDurationMS, dwPeriodMS, bReverse), 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentThrobStart(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentThrobStart(component, 0xFFFFFFFF, AppIsHellgate() ? 1000 : 2000 );
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentThrobEnd(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentSetFade(component, 0.0f);

	if (component->m_tThrobAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tThrobAnimInfo.m_dwAnimTicket);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void UIComponentFade(
//	GAME* game,
//	const CEVENT_DATA& data,
//	DWORD)
//{
//	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
//	if (!component)
//	{
//		return;
//	}
//
//	TIME tiCurrent = (component->m_bAnimatesWhenPaused ?  AppCommonGetAbsTime() : AppCommonGetCurTime());
//	float fTimeRatio = ((float)(tiCurrent - component->m_tMainAnimInfo.m_tiAnimStart))/(float)component->m_tMainAnimInfo.m_dwAnimDuration;
//	float fFading = 0.0f;
//
//	BOOL bForce = (BOOL)data.m_Data2;
//	int nDesiredState = (int)data.m_Data3;
//
//	if (bForce)
//	{
//		component->m_eState = nDesiredState;
//	}
//
//	if (fTimeRatio >= 1.0f)
//	{
//		fFading = 0.0f;
//	}
//	else
//	{
//		switch (component->m_eState)
//		{
//		case UISTATE_ACTIVATING:
//			fFading = 1.0f - fTimeRatio;
//			break;
//		case UISTATE_INACTIVATING:
//			fFading = fTimeRatio;
//			break;
//		default:
////			fFading = 1.0f;		// if the component's state was changed in the middle of fading in or out, set the fading back to default
//			break;
//		}
//	}
//	UIComponentSetFade(component, fFading);
//
//	if (fTimeRatio < 1.0f &&
//		(component->m_eState == UISTATE_INACTIVATING || component->m_eState == UISTATE_ACTIVATING))	// if the component's state was changed in the middle of fading in or out,
//																									// don't schedule another fade event.
//	{
//		component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentFade, CEVENT_DATA((DWORD_PTR)component, bForce, nDesiredState));
//		return;
//	}
//
//	component->m_tMainAnimInfo.m_tiAnimStart = 0;
//	switch (component->m_eState)
//	{
//	case UISTATE_INACTIVATING:
//		//UIComponentSetActive(component, FALSE);
//		UIComponentSetVisible(component, FALSE);
//		break;
//	case UISTATE_ACTIVATING:
//		UIComponentSetActive(component, TRUE);
//		break;
//	}
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void UIComponentFadeIn(
//	UI_COMPONENT *pComponent,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bForce /*= FALSE*/,
//	BOOL bOnlyIfUserActive /*= FALSE*/)
//{
//	if (pComponent &&
//		(!bOnlyIfUserActive || UIComponentIsUserActive(pComponent)))
//	{
//		// if we're already fading in or already active, no need to fade in
//		if (!bForce && (pComponent->m_eState == UISTATE_ACTIVATING || pComponent->m_eState == UISTATE_ACTIVE))
//		{
//			return;
//		}
//
//		if (pComponent->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
//		{
//			CSchedulerCancelEvent(pComponent->m_tMainAnimInfo.m_dwAnimTicket);
//		}
//
//		DWORD dwComponentAnimTime = (DWORD)((float)pComponent->m_dwAnimDuration * fDurationMultiplier);
//		dwAnimTime = MAX(dwAnimTime, dwComponentAnimTime);
//
//		UIComponentSetVisible(pComponent, TRUE);
//		UIComponentSetFadeDuration(pComponent, UISTATE_ACTIVATING, dwComponentAnimTime);
//		//pComponent->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentFade, CEVENT_DATA((DWORD_PTR)pComponent, UISTATE_ACTIVATING));
//		UIComponentFade(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pComponent, bForce, UISTATE_ACTIVATING), 0);
//	}
//}
//
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void UIComponentFadeOut(
//	UI_COMPONENT *pComponent,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bForce /*= FALSE*/)
//{
//	if (pComponent)
//	{
//		// if we're already fading or aren't active, no need to fade out
//		if (!bForce && (pComponent->m_eState == UISTATE_INACTIVATING || pComponent->m_eState == UISTATE_INACTIVE))
//		{
//			return;
//		}
//
//		if (pComponent->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
//		{
//			CSchedulerCancelEvent(pComponent->m_tMainAnimInfo.m_dwAnimTicket);
//		}
//
//		DWORD dwComponentAnimTime = (DWORD)((float)pComponent->m_dwAnimDuration * fDurationMultiplier);
//		dwAnimTime = MAX(dwAnimTime, dwComponentAnimTime);
//
//		//component is set to inactive after it has finished fading out
//		//		UIComponentSetActive(component, FALSE);
//		UIComponentSetFadeDuration(pComponent, UISTATE_INACTIVATING, dwComponentAnimTime);
//		UIComponentFade( AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pComponent, bForce, UISTATE_INACTIVATING), 0);
//	}
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//void UISendComponentMsgWithAnimTime(
//	UI_COMPONENT* pComponent,
//	UINT msg,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bOnlyIfUserActive /*= FALSE*/)
//{
//	if (pComponent &&
//		(!bOnlyIfUserActive || UIComponentIsUserActive(pComponent)))
//	{
//		if (pComponent->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
//		{
//			CSchedulerCancelEvent(pComponent->m_tMainAnimInfo.m_dwAnimTicket);
//		}
//
//		DWORD dwComponentAnimTime = (DWORD)((float)pComponent->m_dwAnimDuration * fDurationMultiplier);
//		dwAnimTime = MAX(dwAnimTime, dwComponentAnimTime);
//		if (msg != UIMSG_NONE)
//		{
//			UIComponentHandleUIMessage(pComponent, msg, dwComponentAnimTime, 0);
//		}
//	}
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//void UISendComponentMsgWithAnimTime(
//	UI_COMPONENT_ENUM eComponent,
//	UINT msg,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bOnlyIfUserActive /*= FALSE*/)
//{
//	UI_COMPONENT* pComponent = UIComponentGetByEnum(eComponent);
//	UISendComponentMsgWithAnimTime(pComponent, msg, fDurationMultiplier, dwAnimTime, bOnlyIfUserActive);
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGetStdItemCursorSize(
	UNIT *pItem,
	float & fWidth,
	float & fHeight)
{
	// get the size of the blown-up item from the inventory grid
	UI_COMPONENT *cmp = UIComponentGetByEnum(UICOMP_PLAYERPACK);
	if (cmp)
	{
		UI_INVGRID *pPack = UICastToInvGrid(cmp);
		UI_SIZE size;
		if (UIInvGridGetItemSize(pPack, pItem, size))
		{
			fWidth = size.m_fWidth;
			fHeight = size.m_fHeight;
			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPlayItemPickupSound(
	UNIT* unit)
{
	ASSERT_RETURN(unit);

	if (UnitGetGenus(unit) != GENUS_ITEM)
	{
		return;
	}
	UNIT* container = UnitGetContainerInRoom(unit);
	if (!container)
	{
		return;
	}

	// don't play pickup sound for money because the money UI will do it when
	// the values change up and down
	if (UnitIsA( unit, UNITTYPE_MONEY ))
	{
		return;
	}

	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
	if (item_data && item_data->m_nInvPickupSound != INVALID_ID)
	{
		c_SoundPlay(item_data->m_nInvPickupSound, &c_UnitGetPosition(container), NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPlayItemPickupSound(
	UNITID idUnit )
{
	if (idUnit == INVALID_ID)
		return;

	UNIT *pUnit = UnitGetById(AppGetCltGame(), idUnit);
	if (!pUnit)
		return;

	UIPlayItemPickupSound(pUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPlayCantUseSound(
	GAME* game,
	UNIT* container,
	UNIT* item)
{
	ASSERT_RETURN(game && container && item);
	ASSERT_RETURN(IS_CLIENT(game));
	if (UnitGetGenus(item) != GENUS_ITEM)
	{
		return;
	}

	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(item));


	if ( item_data && item_data->m_nInvCantUseSound != INVALID_ID)
	{
		c_SoundPlay(item_data->m_nInvCantUseSound, &c_UnitGetPosition(container), NULL);
	}

	const UNIT_DATA* unit_data = UnitGetData( container );
	if (!unit_data)
	{
		return;
	}

	if (unit_data->m_nCantUseSound != INVALID_ID)
	{
		c_SoundPlay(unit_data->m_nCantUseSound, &c_UnitGetPosition(container), NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPlayItemPutdownSound(
	UNIT* unit,
	UNIT* container,
	int nLocation)
{
	ASSERT_RETURN(unit);

	if (UnitGetGenus(unit) != GENUS_ITEM)
	{
		return;
	}

	const UNIT_DATA* item_data = ItemGetData(UnitGetClass(unit));
	if (item_data)
	{
		if (nLocation != INVALID_LINK &&
			container &&
			InvLocIsEquipLocation(nLocation, container) &&
			item_data->m_nInvEquipSound != INVALID_ID)
		{
			c_SoundPlay(item_data->m_nInvEquipSound, &c_SoundGetListenerPosition(), NULL);
		}
		else if (item_data->m_nInvPutdownSound != INVALID_ID)
		{
			c_SoundPlay(item_data->m_nInvPutdownSound, &c_SoundGetListenerPosition(), NULL);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sComponentClipChildren(
	UI_COMPONENT *component)
{
	return;

#if 0

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while (pChild)
	{
		if (component->m_rectClip.m_fX1 != 0.0f ||
			component->m_rectClip.m_fX2 != 0.0f ||
			component->m_rectClip.m_fY1 != 0.0f ||
			component->m_rectClip.m_fY2 != 0.0f)
		{
			pChild->m_rectClip.m_fX1 = component->m_rectClip.m_fX1 - pChild->m_Position.m_fX;
			pChild->m_rectClip.m_fY1 = component->m_rectClip.m_fY1 - pChild->m_Position.m_fY;
			pChild->m_rectClip.m_fX2 = pChild->m_fWidth - ((pChild->m_Position.m_fX + pChild->m_fWidth) - component->m_rectClip.m_fX2);
			pChild->m_rectClip.m_fY2 = pChild->m_fHeight - ((pChild->m_Position.m_fY + pChild->m_fHeight) - component->m_rectClip.m_fY2);

			//pChild->m_rectClip.m_fX1 = MAX(component->m_rectClip.m_fX1 - pChild->m_Position.m_fX, pChild->m_rectClip.m_fX1);
			//pChild->m_rectClip.m_fY1 = MAX(component->m_rectClip.m_fY1 - pChild->m_Position.m_fY, pChild->m_rectClip.m_fY1);
			//pChild->m_rectClip.m_fX2 = MIN(pChild->m_fWidth - ((pChild->m_Position.m_fX + pChild->m_fWidth) - component->m_rectClip.m_fX2), pChild->m_rectClip.m_fX2);
			//pChild->m_rectClip.m_fY2 = MIN(pChild->m_fHeight - ((pChild->m_Position.m_fY + pChild->m_fHeight) - component->m_rectClip.m_fY2), pChild->m_rectClip.m_fY2);
		}
		else
		{
			structclear(pChild->m_rectClip);
		}
		sComponentClipChildren(pChild);
		pChild = pChild->m_pNextSibling;
	}
#endif 0
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (component->m_rectClip.m_fX1 != 0.0f ||
		component->m_rectClip.m_fX2 != 0.0f ||
		component->m_rectClip.m_fY1 != 0.0f ||
		component->m_rectClip.m_fY2 != 0.0f)
	{
		sComponentClipChildren(component);
	}

	UIComponentRemoveAllElements(component);
	if (!component->m_pFrame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentAddFrame(component);
//	UIComponentAddElement(component, UIComponentGetTexture(component), component->m_pFrame, UI_POSITION(0.0f, 0.0f), component->m_dwColor);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointAddButtonSetVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT* pUnit = UIComponentGetFocusUnit(component);

	BOOL bVisible = FALSE;
	if (pUnit)
	{
		GAME *pGame = UnitGetGame(pUnit);
		REF( pGame );
		if ((UnitIsPlayer(pUnit) ||
			(AppIsHellgate() && pGame && PetIsPlayerPet(pGame, pUnit))) &&
			UnitGetStat(pUnit, STATS_STAT_POINTS) > 0)
		{
			bVisible = TRUE;
		}
	}
	// TRAVIS: THis is for the levelup icon down at the bottom -
	// we hide it if the character screen is visible -
	// but we re-show it if you have stat points left to spend
	if( AppIsTugboat() &&
		component->m_dwParam == 1 )
	{
		if( UIPaneVisible( KPaneMenuCharacterSheet ) )
		{
			bVisible = FALSE;
		}
	}

	UIComponentSetActive(component, bVisible);
	UIComponentSetVisible(component, bVisible, TRUE);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointAddButtonSetFocusUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentSetFocusUnit(component, UNITID(wParam));
	UIStatPointAddButtonSetVisible(component, UIMSG_SETCONTROLSTAT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointAddUp(
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

	UI_BUTTONEX *pButton = UICastToButton(component);
	ASSERT_RETVAL(pButton, UIMSG_RET_NOT_HANDLED);

	if (pButton->m_nAssocStat == INVALID_ID)
	{
		return UIMSG_RET_END_PROCESS;
	}

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	ASSERTX_RETVAL(pFocusUnit, UIMSG_RET_END_PROCESS, "UIStatPointAddUp: Expected focus unit to add stat point to");

	MSG_CCMD_ALLOCSTAT allocstatmsg;
	allocstatmsg.wStat = (WORD)pButton->m_nAssocStat;
	allocstatmsg.dwParam = 0;
	allocstatmsg.nPoints = 1;
	allocstatmsg.idUnit = UnitGetId(pFocusUnit);

	c_SendMessage(CCMD_ALLOCSTAT, &allocstatmsg);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointAddDown(
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

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIActivateAttributeControls(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX * pButton;
	pButton = UICastToButton(UIComponentFindChildByName(component, "accu_clicker_sub"));
	UIStatPointSubButtonSetVisible(pButton, msg, wParam, lParam );
	pButton = UICastToButton(UIComponentFindChildByName(component, "str_clicker_sub"));
	UIStatPointSubButtonSetVisible(pButton, msg, wParam, lParam );
	pButton = UICastToButton(UIComponentFindChildByName(component, "stam_clicker_sub"));
	UIStatPointSubButtonSetVisible(pButton, msg, wParam, lParam );
	pButton = UICastToButton(UIComponentFindChildByName(component, "will_clicker_sub"));
	UIStatPointSubButtonSetVisible(pButton, msg, wParam, lParam );

	return UISetChatActive(component, msg, wParam, lParam );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointSubButtonSetVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component->m_pParent))
		return UIMSG_RET_NOT_HANDLED;

	UI_BUTTONEX *pButton = UICastToButton(component);
	ASSERT_RETVAL(pButton, UIMSG_RET_NOT_HANDLED);
	UNIT* pUnit = UIComponentGetFocusUnit(component);

	BOOL bVisible = FALSE;
	if (pUnit)
	{
		GAME *pGame = UnitGetGame(pUnit);
		const UNIT_DATA * player_data = PlayerGetData(pGame, UnitGetClass(pUnit));
		ASSERTV_RETVAL(player_data, UIMSG_RET_NOT_HANDLED, "UIStatPointSubButtonSetVisible: player_data is NULL");

		int nStat = pButton->m_nAssocStat;
		int nMin = player_data->nStartingAccuracy;
		if (nStat == STATS_STRENGTH)
			nMin = player_data->nStartingStrength;
		else if (nStat == STATS_STAMINA)
			nMin = player_data->nStartingStamina;
		else if (nStat == STATS_WILLPOWER)
			nMin = player_data->nStartingWillpower;

		if (UnitGetBaseStat(pUnit, nStat) > nMin)
			bVisible = TRUE;
	}

	UIComponentSetActive(component, bVisible);
	UIComponentSetVisible(component, bVisible, TRUE);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointSubUp(
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

	UI_BUTTONEX *pButton = UICastToButton(component);
	ASSERT_RETVAL(pButton, UIMSG_RET_NOT_HANDLED);

	if (pButton->m_nAssocStat == INVALID_ID)
	{
		return UIMSG_RET_END_PROCESS;
	}

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	ASSERTX_RETVAL(pFocusUnit, UIMSG_RET_END_PROCESS, "UIStatPointSubUp: Expected focus unit to add stat point to");
	// KCK: Server enforces that we have enough money in case of hacking, but we might as well just tell the player here.
	GAME *pGame = UnitGetGame(pFocusUnit);
	const UNIT_DATA * player_data = PlayerGetData(pGame, UnitGetClass(pFocusUnit));
	ASSERTV_RETVAL(player_data, UIMSG_RET_END_PROCESS, "UIStatPointSubUp: player_data is NULL");
	const PLAYERLEVEL_DATA * level_data = PlayerLevelGetData(pGame, player_data->nUnitTypeForLeveling, UnitGetExperienceLevel(pFocusUnit));
	cCurrency buyPrice = level_data->nStatRespecCost;
	cCurrency playerCurrency = UnitGetCurrency( pFocusUnit );
	if (playerCurrency < buyPrice)
	{
		const WCHAR *szMsg = GlobalStringGet(GS_UI_NOT_ENOUGH_MONEY);
		if (szMsg && szMsg[0])
		{
			UIShowQuickMessage(szMsg, 3.0f);
		}
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	MSG_CCMD_ALLOCSTAT allocstatmsg;
	allocstatmsg.wStat = (WORD)pButton->m_nAssocStat;
	allocstatmsg.dwParam = 0;
	allocstatmsg.nPoints = -1;
	allocstatmsg.idUnit = UnitGetId(pFocusUnit);

	c_SendMessage(CCMD_ALLOCSTAT, &allocstatmsg);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatPointSubDown(
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

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIDropItem(
	UNITID idUnit)
{
	if (idUnit == INVALID_ID)
	{
		return 0;
	}
	UNIT* unit = UnitGetById(AppGetCltGame(), idUnit);
	if (!unit)
	{
		return 0;
	}

	// in tug, if we 'drop' an item reference, we just clear the cursor.
	if( AppIsTugboat() )
	{
		InputSetInteracted( );

		if( g_UI.m_Cursor->m_idUnit != INVALID_ID )
		{
			UIClearCursor();
			return 1;
		}

	}

	if (UnitIsNoDrop(unit))
	{
		UIShowMessage( QUIM_DIALOG, DIALOG_NO_DROP );
		return 0;
	}

	if (UnitGetUltimateContainer( unit ) != UIGetControlUnit())
	{
		return 0;
	}

	if (ItemBelongsToAnyMerchant(unit))
	{
		return 0;
	}

	if( AppIsTugboat() &&
		QuestUnitIsUnitType_QuestItem( unit ) &&
		QuestUnitCanDropQuestItem( unit ) )
	{
		c_PlayerInteract( UnitGetGame( UIGetControlUnit() ), unit, UNIT_INTERACT_ITEMDROP );
		return 0;
	}

	// try to drop
	c_ItemTryDrop( unit );

	// Travis : this will happen after a successful or unsuccessful move, right?
	// seems to work fine without doing it ahead of time,
	// and avoids cases of invisible items in cursor with failed drops.
	//UIClearCursor();
	// I seem to need to do this for dropping by reference?

	if (UIGetModUnit() != INVALID_ID &&
		UIGetModUnit() == idUnit &&
		UIComponentGetActive(UIComponentGetByEnum(UICOMP_GUNMODSCREEN)))
	{
		UIComponentActivate(UICOMP_GUNMODSCREEN, FALSE);
		//UISwitchComponents(UICOMP_GUNMODSCREEN, UICOMP_PAPERDOLL);
	}

	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDropCursorItem(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) || !UIComponentGetVisible(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if( UIGetCursorUnit() == INVALID_ID )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UIDropItem(UIGetCursorUnit()))
	{
		if (component->m_eComponentType == UITYPE_BUTTON)
		{
			UI_BUTTONEX* button = UICastToButton(component);
			if (button)
			{
				button->m_dwPushstate = 0;
				UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
			}
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICursorOnInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eRetval = UIComponentOnInactivate(component, msg, wParam, lParam);
	if (ResultIsHandled(eRetval))
	{
		UIClearCursor();
	}

	return eRetval;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStashOnInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// guess what?  we have to do this *before* we close the stash.
	UIClearCursor();

	return UIComponentOnInactivate(component, msg, wParam, lParam);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIAutoEquipItem(
	UNIT *item,
	UNIT *container,
	int nFromWeaponConfig /*= -1*/)
{
	UIStopSkills();

	// Is the item already in an equip location?
	if (ItemIsInEquipLocation(container, item))
	{
		return TRUE;
	}

	int location, invx, invy;
	BOOL fOpenSpace = ItemGetEquipLocation(container, item, &location, &invx, &invy);
	if (fOpenSpace)
	{
		UNIT* pItemOwner = UnitGetUltimateOwner(item);

		if (ItemBelongsToAnyMerchant(item))
		{
			if (UIDoMerchantAction(item, location, invx, invy))
			{
				return TRUE;
			}
			return FALSE;
		}
		else if (pItemOwner != container)
		{
			// what would this be now that we handle store above?
			ASSERTX_RETFALSE( 0, "maybe code should not be here?" );
		}
		else
		{
			c_InventoryTryPut(container, item, location, invx, invy);
			UnitWaitingForInventoryMove(item, TRUE);
			UIPlayItemPutdownSound(item, container, location);
			if (nFromWeaponConfig != -1)
			{
				UIClearCursorUnitReferenceOnly();
			}
		}
		return TRUE;
	}
	if (location == INVLOC_NONE)
	{
		return FALSE;
	}

	UNIT* item_list[6];
	int overlap = UnitInventoryGetOverlap(container, item, location, invx, invy, item_list, 6);
	if (overlap > 1)
	{
		return FALSE;
	}
	UNIT* olditem = item_list[0];
	if (olditem && olditem == item)
	{
		return FALSE;
	}

	if (olditem)
	{
		// see if the blocking item can fit in another equip location
		int nAlternateLocation = INVLOC_NONE;
		int nExceptLocation = location;
		BOOL fOpenAlternateSpace = ItemGetEquipLocation(container, olditem, &nAlternateLocation, &invx, &invy, nExceptLocation);
		if (fOpenAlternateSpace && nAlternateLocation != INVLOC_NONE)
		{
			c_InventoryTrySwap(item, olditem, nFromWeaponConfig);
			UIPlayItemPutdownSound(item, container, location);
			if (nFromWeaponConfig != -1)
			{
				UIClearCursorUnitReferenceOnly();
			}

			// put it in the alternate location
			c_InventoryTryPut(container, olditem, nAlternateLocation, invx, invy);
			UIPlayItemPutdownSound(olditem, container, nAlternateLocation);

			return TRUE;
		}
	}

	if (olditem && UnitInventoryTestSwap(item, olditem))
	{
		c_InventoryTrySwap(item, olditem, nFromWeaponConfig);
		UIPlayItemPutdownSound(item, container, location);
		if (nFromWeaponConfig != -1)
		{
			UIClearCursorUnitReferenceOnly();
		}

		return TRUE;
	}
	if (olditem)
	{
		int nPreventngLoc;
		if (IsUnitPreventedFromInvLoc(container, item, location, &nPreventngLoc))
		{
			UNIT* pPreventItem = UnitInventoryGetByLocation(container, nPreventngLoc);
			if (!pPreventItem)
			{
				return FALSE;
			}
			int new_loc, new_x, new_y;
			if (!ItemGetOpenLocation(container, pPreventItem, TRUE, &new_loc, &new_x, &new_y))
			{
				return FALSE;
			}

			c_InventoryTryPut(container, pPreventItem, new_loc, new_x, new_y);
			c_InventoryTrySwap(item, olditem, nFromWeaponConfig);
			UIPlayItemPutdownSound(item, container, new_loc);
			if (nFromWeaponConfig != -1)
			{
				UIClearCursorUnitReferenceOnly();
			}
			return TRUE;
		}

		 //not prevented but won't swap with the item we want to eqiup.
		 //  see if it will fit anywhere in the swap item's location
		//int nSwapLocation = INVLOC_NONE;
		//int x = 0;
		//int y = 0;
		//if (!UnitGetInventoryLocation(item, &nSwapLocation, &x, &y))
		//{
		//	return FALSE;
		//}
		//if (UnitInventoryTestIgnoreItem(container, olditem, item, nSwapLocation, x, y))
		{
			// ask the server to try an "off-swap"
			c_InventoryTrySwap(item, olditem, nFromWeaponConfig);
			UIPlayItemPutdownSound(item, container, location);
			if (nFromWeaponConfig != -1)
			{
				UIClearCursorUnitReferenceOnly();
			}
			return TRUE;
		}
	}

	// If we're blocked, but there's no item in the slot, check the preventing slot
	int prev_loc;
	if (IsUnitPreventedFromInvLoc(container, item, location, &prev_loc))
	{
		UNIT* prev_item = UnitInventoryGetByLocation(container, prev_loc);
		if (prev_item)
		{
			c_InventoryTrySwap(item, prev_item, nFromWeaponConfig);
			UIPlayItemPutdownSound(item, container, location);
			if (nFromWeaponConfig != -1)
			{
				UIClearCursorUnitReferenceOnly();
			}
			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEquipCursorItem(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) || !UIComponentGetActive(component) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNITID idCursorUnit = UIGetCursorUnit();
	if (idCursorUnit == INVALID_ID)
	{
		return UIModelPanelOnLButtonDown(component, msg, wParam, lParam);
	}
	UNIT* item = UnitGetById(AppGetCltGame(), idCursorUnit);
	if (!item)
	{
		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_END_PROCESS;
	}
	UNIT* container = UIComponentGetFocusUnit(component);
	if (!container)
	{
		return UIMSG_RET_END_PROCESS;
	}

	if (ItemBelongsToAnyMerchant(item))
	{
		UIDoMerchantAction(item);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	UNIT* pItemOwner = UnitGetUltimateOwner(item);
	if (pItemOwner != container)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (UIAutoEquipItem(item, container, g_UI.m_Cursor->m_nFromWeaponConfig))
	{
//		UISetCursorUnit(INVALID_ID, TRUE);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIItemInitInventoryGfx(
	UNIT* pUnit,
	BOOL bEvenIfModelExists /*= FALSE*/,
	BOOL bCreateWardrobe /*= TRUE*/)
{
	if (!pUnit)
	{
		return 0;
	}
	UNIT *pOwner = UnitGetUltimateOwner( pUnit );
	BOOL bDoInventory = FALSE;
	int nModelId = bCreateWardrobe ? c_UnitGetModelIdInventoryInspect( pUnit ) : c_UnitGetModelIdInventory( pUnit );
	if (UnitGetGenus(pUnit) == GENUS_ITEM)
	{

		if ( !bEvenIfModelExists &&
			 nModelId != INVALID_ID)
		{
			return 1;
		}

		// these inventory gfx should actually be in the UI_TEXTURE_FRAME for that object -- then there's only one per unit model type
		// -- maybe model def instead?
		if (bEvenIfModelExists ||
			nModelId == INVALID_ID)
		{
			c_ItemInitInventoryGfx(AppGetCltGame(), UnitGetId(pUnit), bCreateWardrobe);
		}

		// we should create graphics for all the inventory of this unit too
		bDoInventory = TRUE;

	}
	else if (PetIsPet( pUnit ))
	{
		GAME *pGame = UnitGetGame( pUnit );
		UNITID idPetOwner = PetGetOwner( pUnit );
		UNIT *pPetOwner = UnitGetById( pGame, idPetOwner );

		if (pPetOwner && pPetOwner == pOwner)
		{
			bDoInventory = TRUE;
		}

	}

	if (bDoInventory)
	{
		// Now, and here's the crazy bit, the really good bit, are you ready for it?  We init the item's... INVENTORY ITEMS!
		UNIT* pItem = NULL;
		while ((pItem = UnitInventoryIterate(pUnit, pItem)) != NULL)
		{
			UIItemInitInventoryGfx(pItem, bEvenIfModelExists, bCreateWardrobe);
		}
	}

	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPetDisplayOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	//if( !UIComponentGetActive(component) )
	//{
	//	return UIMSG_RET_NOT_HANDLED;
	//}

	UNIT* container = UnitGetById(AppGetCltGame(), (UNITID)wParam);
	if (!container)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (container != UIGetControlUnit())
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nLocation = (msg == UIMSG_INVENTORYCHANGE ? (int)lParam : INVALID_LINK);
	{
		UNIT *pPlayer = UIGetControlUnit();
		if (!pPlayer)
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		if (nLocation != INVALID_LINK)
		{
			INVLOC_HEADER tInvLocData;
			if (!UnitInventoryGetLocInfo( pPlayer, nLocation, &tInvLocData ))
			{
				return UIMSG_RET_NOT_HANDLED;
			}
			if (!InvLocTestFlag( &tInvLocData, INVLOCFLAG_PETSLOT ))
			{
				return UIMSG_RET_NOT_HANDLED;
			}
		}


	}


	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	//UIPetDisplayOnPaint( component, msg, wParam, lParam );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPetCooldownOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);
	UNIT *pPet = UIComponentGetFocusUnit(component);
	if (pPet)
	{
		DWORD dwNowTick = GameGetTick( AppGetCltGame() );
		DWORD dwStartTick = UnitGetStatShift(pPet, STATS_FUSE_START_TICK);
		DWORD dwEndTick = UnitGetStatShift(pPet, STATS_FUSE_END_TICK);

		if (dwEndTick > dwNowTick &&
			dwEndTick > dwStartTick)
		{
			if (dwNowTick >= dwStartTick)
			{
				float fPct = PIN((float)(dwNowTick - dwStartTick) / (float)(dwEndTick - dwStartTick), 0.0f, 1.0f);
				UI_RECT rect(0.0f, component->m_fHeight - (component->m_fHeight * fPct), component->m_fWidth, component->m_fHeight);
				UIComponentAddPrimitiveElement(component,
					UIPRIM_BOX, 0, rect, NULL, NULL, component->m_dwColor);
			}

			// register another

			CSchedulerRegisterMessage(AppCommonGetCurTime() + 250, component, UIMSG_PAINT, 0, 0);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL	UIPetDisplayOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* container = GameGetControlUnit(pGame);
	//UI_COMPONENT* component = UIComponentGetByEnum( UICOMP_PET_PANEL  );

	if (AppIsTugboat())
	{
		// TRAVIS: TODO - make this work well with new pane menus

		if( e_GetUICovered() )
		{
			UIComponentSetVisible( component, FALSE );
			return UIMSG_RET_NOT_HANDLED;
		}
		else
		{
			UIComponentSetActive( component, TRUE );
		}
	}

	if( !container )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// ok, first we're going to gather up all the pets.  Right now we're just going to
	//  examine everything in the player's inventory and see if it's a pet.
static const int MAX_PETS = 6;
	int nNumMajorPets = 0;
	UNIT* pMajorPets[MAX_PETS];
	memclear(pMajorPets, sizeof(UNIT *) * MAX_PETS);
	int nNumPets = 0;
	UNIT* pPets[MAX_PETS];
	memclear(pPets, sizeof(UNIT *) * MAX_PETS);
	int nPetCount[MAX_PETS];
	memclear(nPetCount, sizeof(int) * MAX_PETS);

	UNIT * pItem = NULL;
	while ((pItem = UnitInventoryIterate(container, pItem)) != NULL)		// might want to narrow this iterate down if we can
	{
		if (PetIsPet(pItem) &&  UnitDataTestFlag(UnitGetData(pItem), UNIT_DATA_FLAG_SHOWS_PORTRAIT))
		{

			// don't consider dead pets or pets that aren't owned by us (it just so
			// happens that currently this function is called when a pet is being
			// removed from the inventory and is in an invalid inventory state cause
			// it has a container, but no location (however, owner is properly nulled out)
			// this is the minimal change possible for the E3'06 build -Colin
			if (IsUnitDeadOrDying( pItem ) || UnitGetOwnerUnit( pItem ) != container)
			{
				continue;
			}

			if (UnitHasState( pGame, container, STATE_QUEST_RTS ))
			{
				int nLocation = INVLOC_NONE;
				if (!UnitGetInventoryLocation(pItem, &nLocation))
				{
					continue;
				}

				if (nLocation != INVLOC_QUEST_RTS)
				{
					continue;
				}
			}


			BOOL bAdd = TRUE;
			if (UnitIsA(pItem, UNITTYPE_MINOR_PET) && nNumPets > 0)
			{
				// for minor pets we need to just increment the number of pets
				//   so see if we've already found another pet of this type
				int i=0;
				for (; i < nNumPets; i++)
				{
					if (UnitGetData(pPets[i]) == UnitGetData(pItem))
					{
						//same type of pet, increment the count
						nPetCount[i]++;
						bAdd = FALSE;
						break;
					}
				}
			}

			if (bAdd)
			{
				if (UnitIsA(pItem, UNITTYPE_MINOR_PET) || UnitIsA(pItem, UNITTYPE_PET_FOLLOWER))
				{
					ASSERT_RETVAL(nNumPets < MAX_PETS, UIMSG_RET_NOT_HANDLED);
					pPets[nNumPets] = pItem;
					nPetCount[nNumPets]++;
					nNumPets++;
				}
				else
				{
					ASSERT_RETVAL(nNumMajorPets < MAX_PETS, UIMSG_RET_NOT_HANDLED);
					pMajorPets[nNumMajorPets] = pItem;
					nNumMajorPets++;
				}
			}
		}
	}

	int nCurMinorPet = 0;
	int nCurMajorPet = 0;
	UI_COMPONENT *pChild = NULL;
	while ((pChild = UIComponentIterateChildren(component, pChild, UITYPE_PANEL, FALSE)) != NULL)
	{
		if (UIComponentFindChildByName(pChild, "pet health bar current"))
		{
			if (nCurMajorPet < nNumMajorPets)
			{
				UIComponentSetFocusUnit(pChild, UnitGetId(pMajorPets[nCurMajorPet]));
				UIComponentActivate(pChild, TRUE);
				nCurMajorPet++;
				UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
			}

			else
			{
				UIComponentSetFocusUnit(pChild, INVALID_ID);
				UIComponentSetVisible(pChild, FALSE);
			}
		}
		else
		{
			if (nCurMinorPet < nNumPets)
			{
				UIComponentSetFocusUnit(pChild, UnitGetId(pPets[nCurMinorPet]));
				UIComponentActivate(pChild, TRUE);

			    UI_COMPONENT *pIcon = UIComponentFindChildByName(pChild, "pet icon");

			    if( pIcon )
			    {
				    UIComponentRemoveAllElements(pIcon);
					UIX_TEXTURE *pIconTexture = (AppIsHellgate() ? UIComponentGetTexture(pIcon) : pPets[nCurMinorPet]->pIconTexture);
				    if( pIconTexture )
				    {
						BOOL bLoaded = TRUE;
						if (AppIsTugboat())
						{
							bLoaded = UITextureLoadFinalize( pPets[nCurMinorPet]->pIconTexture );
						}

						UI_TEXTURE_FRAME* pFrame = NULL;

						if (AppIsTugboat())
						{
							pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pIconTexture->m_pFrames, "icon");
						}
						else
						{
							pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pIconTexture->m_pFrames, pPets[nCurMinorPet]->pUnitData->szIcon);
						}

					    if( pFrame && bLoaded)
						{
							UI_SIZE sizeIcon;
							sizeIcon.m_fWidth = pIcon->m_fWidth;
							sizeIcon.m_fHeight = pIcon->m_fHeight;

							UIComponentAddElement(pIcon, pIconTexture, pFrame, UI_POSITION( 0, 0 ), GFXCOLOR_WHITE, NULL, FALSE, 1.0f, 1.0f, &sizeIcon );


						}
						else if (AppIsTugboat())
						{
							// it's not asynch loaded yet - schedule a repaint so we can take care of it
							if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
							{
								CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
							}
							component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR)component, UIMSG_PAINT, wParam, lParam));
						}
				    }
				}

				UI_COMPONENT *pCountLabel = UIComponentFindChildByName(pChild, "pet count");
				if (pCountLabel)
				{

					WCHAR buf[32];
					int nLocation = INVLOC_NONE;
					if (AppIsHellgate() && UnitGetInventoryLocation(pPets[nCurMinorPet], &nLocation))
					{
						int nMaxCount = UnitInventoryGetArea(container, nLocation);
						PStrPrintf(buf, 32, L"%d/%d", nPetCount[nCurMinorPet], nMaxCount);
					}
					else
					{
						PStrPrintf(buf, 32, L"%d", nPetCount[nCurMinorPet]);
					}
					UILabelSetText(pCountLabel, buf);
					UIComponentSetActive(pCountLabel, TRUE);
				}
				nCurMinorPet++;
			}
			else
			{
				UIComponentSetFocusUnit(pChild, INVALID_ID);
				UIComponentActivate(pChild, FALSE);
			}
		}

		// doing this will wipe out the icons we just added in Mythos!
		if( AppIsHellgate() )
		{
			UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
		}
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyDisplayOnLevelRankChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pLevel = UIComponentFindChildByName(component, "party member level");
	UI_COMPONENT *pLevelRank = UIComponentFindChildByName(component, "party member level rank");

	UNIT *pPartyMember = component->m_idFocusUnit != INVALID_ID ? UnitGetById(AppGetCltGame(), component->m_idFocusUnit) : NULL;
	const int iRank = pPartyMember ? UnitGetStat(pPartyMember, STATS_PLAYER_RANK) : -1;

	if (pLevel)
	{
		UIComponentSetVisible(pLevel, iRank == 0);
		UIComponentHandleUIMessage(pLevel, UIMSG_PAINT, 0, 0);
	}
	if (pLevelRank)
	{
		UIComponentSetVisible(pLevelRank, iRank > 0);
		UIComponentHandleUIMessage(pLevelRank, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyDisplayOnPartyChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (AppIsTugboat())	// -----------------------------------------------------------
	{

		UI_COMPONENT* component = 	UIComponentGetByEnum(UICOMP_PARTY_MEMBER_PANEL);
		UI_COMPONENT* petcomponent = 	UIComponentGetByEnum(UICOMP_PET_PANEL);
		UIPetDisplayOnPaint(petcomponent, UIMSG_PAINT, 0, 0);

		if( !AppGetCltGame() )
		{
			return UIMSG_RET_NOT_HANDLED;
		}


		if( e_GetUICovered() )
		{
			UIComponentSetVisible( component, FALSE );
			return UIMSG_RET_HANDLED;
		}
		else
		{
			UIComponentSetActive( component, TRUE );
		}
		UI_COMPONENT *pMember = component->m_pFirstChild;
		int nMember = 0;
		while (pMember && nMember < MAX_CHAT_PARTY_MEMBERS)
		{
			if (UIComponentIsPanel(pMember))
			{

				pMember->m_idFocusUnit = c_PlayerGetPartyMemberUnitId(nMember);
				pMember->m_dwData = nMember;  // save member index in dwData

				//pMember->m_idFocusUnit = UnitGetId( UIGetControlUnit() );
				UI_COMPONENT *pNameLabel = UIComponentFindChildByName(pMember, "party member name");
				if (pNameLabel)
				{
					// might want to call UnitGetName if we have a valid unit... right now we're just going to take what the server gives us
					//if (pMember->m_idFocusUnit != INVALID_ID)
					{
						int nArea = c_PlayerGetPartyMemberLevelArea(nMember, FALSE );
						int nDepth = c_PlayerGetPartyMemberLevelDepth(nMember, FALSE );

						BOOL bHasSubName = FALSE;
						
						WCHAR uszSubName[512 ] = { 0 };
						WCHAR szBuf[512];
						if( nArea != INVALID_ID )
						{
							bHasSubName = LevelDepthGetDisplayName( 
								AppGetCltGame(),
								nArea,
								INVALID_ID,
								nDepth, 
								uszSubName, 
								512,
								FALSE );
	
							WCHAR insertString[ 512 ];
							MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nArea, nDepth, &insertString[0], 512, FALSE );

							if( insertString[0] )
							{
								PStrPrintf(szBuf, 512, L"%s", &insertString[0] );
								if (bHasSubName)
								{
									PStrCat( szBuf, L" - ", 512 );
									PStrCat( szBuf, uszSubName, 512 );
								}
							}



							if( !insertString[0] )
							{
								PStrPrintf(szBuf, 512, L"%s\nTraveling", c_PlayerGetPartyMemberName(nMember) );
							}
							else
							{
								PStrPrintf(szBuf, 512, L"%s\n%s", c_PlayerGetPartyMemberName(nMember), &insertString[0] );
								if (bHasSubName)
								{
									PStrCat( szBuf, L" - ", 512 );
									PStrCat( szBuf, uszSubName, 512 );
								}
							}

						}
						else
						{
								PStrPrintf(szBuf, 512, L"%s\nTraveling", c_PlayerGetPartyMemberName(nMember) );
						}

						UILabelSetText(pNameLabel, szBuf );
					}
				}

				if (c_PlayerGetPartyMemberGUID(nMember) != INVALID_GUID)
				{
					UIComponentSetActive(pMember, TRUE);
				}
				else
				{
					UIComponentSetVisible(pMember, FALSE);
					//UIComponentSetVisible(pMember, TRUE);
				}
				UI_COMPONENT *pPortrait = UIComponentFindChildByName(pMember, "party member portrait model");
				UI_COMPONENT *pPortraitMissing = UIComponentFindChildByName(pMember, "party member portrait missing");

				if (pPortrait)
				{
					UIComponentSetVisible(pPortrait, pMember->m_idFocusUnit != INVALID_ID);
				}
				if (pPortraitMissing)
				{
					UIComponentSetVisible(pPortraitMissing, pMember->m_idFocusUnit == INVALID_ID);
				}

				nMember++;
			}

			pMember = pMember->m_pNextSibling;
		}

		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
		return UIMSG_RET_HANDLED;

	}	// -----------------------------------------------------------

	UI_COMPONENT *pMember = component->m_pFirstChild;
	int nMemberIndex = 0;
	while (pMember && nMemberIndex < MAX_CHAT_PARTY_MEMBERS)
	{
		if (UIComponentIsPanel(pMember))
		{
			pMember->m_idFocusUnit = c_PlayerGetPartyMemberUnitId(nMemberIndex);
			pMember->m_dwData = nMemberIndex;

			UI_COMPONENT *pNameLabel = UIComponentFindChildByName(pMember, "party member name");
			if (pNameLabel)
			{
				// might want to call UnitGetName if we have a valid unit... right now we're just going to take what the server gives us
				//if (pMember->m_idFocusUnit != INVALID_ID)
				{
					UILabelSetText(pNameLabel, c_PlayerGetPartyMemberName(nMemberIndex));
				}
			}

			if (c_PlayerGetPartyMemberGUID(nMemberIndex) != INVALID_GUID)
			{
				UIComponentSetActive(pMember, TRUE);
			}
			else
			{
				UIComponentSetVisible(pMember, FALSE);
			}
			UI_COMPONENT *pPortrait = UIComponentFindChildByName(pMember, "party member portrait model");
			UI_COMPONENT *pPortraitMissing = UIComponentFindChildByName(pMember, "party member portrait missing");

			if (pPortrait)
				UIComponentSetVisible(pPortrait, pMember->m_idFocusUnit != INVALID_ID);
			if (pPortraitMissing)
				UIComponentSetVisible(pPortraitMissing, pMember->m_idFocusUnit == INVALID_ID);

			UIPartyDisplayOnLevelRankChange(pMember, msg, wParam, lParam);

			nMemberIndex++;
		}

		pMember = pMember->m_pNextSibling;
	}

	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		UNIT * pPlayer = GameGetControlUnit( pGame );
		if (pPlayer)
		{
			if (c_PlayerGetPartyMemberCount() > 1)
			{
				if (!UnitHasExactState( pPlayer, STATE_IN_PARTY ))
				{
					c_StateSet(pPlayer, pPlayer, STATE_IN_PARTY, 0, 0, INVALID_ID);
				}
			}
			else
			{
				if (UnitHasExactState( pPlayer, STATE_IN_PARTY ))
				{
					c_StateClear(pPlayer, UnitGetId(pPlayer), STATE_IN_PARTY, 0);
				}
			}
		}
	}

	if (UIComponentGetActiveByEnum(UICOMP_PARTYPANEL))
	{
		if (wParam == 2)
		{
			// we just left our party, remove it from the list
			UIPartyPanelRemoveMyPartyDesc();
		}
		UIPartyPanelContextChange(UIComponentGetByEnum(UICOMP_PARTYPANEL));
	}
	
	if (UIComponentGetActiveByEnum(UICOMP_PLAYERMATCH_PANEL))
	{
		if (wParam == 2)
		{
			// we just left our party, remove it from the list
			UIPartyPanelRemoveMyPartyDesc();
		}
		UIPlayerMatchDialogContextChange();
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFactionStandingsOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pChild = NULL;
	while ((pChild = UIComponentIterateChildren(component, pChild, UITYPE_PANEL, FALSE)) != NULL)
	{
		UIComponentSetVisible(pChild, FALSE);
	}

	UNIT *pUnit = UIComponentGetFocusUnit(component);

	if (pUnit)
	{
		pChild = UIComponentIterateChildren(component, NULL, UITYPE_TOOLTIP, FALSE);
		UNIT_ITERATE_STATS_RANGE(pUnit, STATS_FACTION_SCORE, statsentry, ii, 64)
		{
			if (pChild)
			{
				int nFaction = statsentry[ii].GetParam();
				int nValue = statsentry[ii].value;

				const FACTION_DEFINITION *pFacDef = FactionGetDefinition(nFaction);

				if (pFacDef)
				{
					UIComponentSetActive(pChild, TRUE);

					UI_COMPONENT *pLabel = UIComponentFindChildByName(pChild, "faction label");
					if (pLabel)
					{
						UILabelSetText(pLabel, StringTableGetStringByIndex(pFacDef->nDisplayString));
					}

					int nStandingDef = FactionGetStanding(pUnit, nFaction);
					if (nStandingDef != INVALID_LINK)
					{
						pChild->m_dwData = (DWORD)nStandingDef;
						const FACTION_STANDING_DEFINITION *pStanding = FactionStandingGetDefinition(nStandingDef);
						if (pStanding)
						{
							pLabel = UIComponentFindChildByName(pChild, "faction standing desc");
							if (pLabel)
							{
								pLabel->m_dwData = (DWORD)nValue;
								UILabelSetText(pLabel, StringTableGetStringByIndex(pStanding->nDisplayString));

							}

							UI_COMPONENT *pBarChild = UIComponentIterateChildren(pChild, NULL, UITYPE_BAR, FALSE);
							if (pBarChild)
							{
								UI_BAR *pBar = UICastToBar(pBarChild);
								pBar->m_nMaxValue = pStanding->nMaxScore - pStanding->nMinScore;
								pBar->m_nValue = nValue - pStanding->nMinScore;
								UIComponentHandleUIMessage(pBar, UIMSG_PAINT, 0, 0);
							}
						}
					}
				}

				pChild = UIComponentIterateChildren(component, pChild, UITYPE_TOOLTIP, FALSE);
			}
		}
		UNIT_ITERATE_STATS_END;
	}


	UI_TOOLTIP *pTooltip = UICastToTooltip(component);
	if (pTooltip->m_bAutosize)
	{
		UITooltipDetermineSize(pTooltip);
	}

	pTooltip->m_InactivePos.m_fY = pTooltip->m_ActivePos.m_fY - pTooltip->m_fHeight;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFactionStadingDispOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT(component && component->m_pParent);

	int nStandingDef = (int)component->m_pParent->m_dwData;
	if (nStandingDef != INVALID_LINK)
	{
		int nValue = (int)component->m_dwData;
		const FACTION_STANDING_DEFINITION *pStanding = FactionStandingGetDefinition(nStandingDef);
		if (pStanding->nDisplayStringNumbers != INVALID_LINK)
		{
			const WCHAR *puszToken1 = L"[number1]";
			const WCHAR *puszToken2 = L"[number2]";
			WCHAR szBuf[256];
			WCHAR szNumberOut[256];
			WCHAR szNumberIn[256];
			PStrCopy(szBuf, StringTableGetStringByIndex(pStanding->nDisplayStringNumbers), arrsize(szBuf));
//			PStrPrintf(szBuf, arrsize(szBuf), szFormat, nValue - pStanding->nMinScore, (pStanding->nMaxScore - pStanding->nMinScore) +1);

			PStrPrintf(szNumberIn, arrsize(szNumberIn), L"%d", nValue - pStanding->nMinScore);
			LanguageFormatNumberString(szNumberOut, arrsize(szNumberOut), szNumberIn);
			PStrReplaceToken( szBuf, arrsize(szBuf), puszToken1, szNumberOut );

			PStrPrintf(szNumberIn, arrsize(szNumberIn), L"%d", (pStanding->nMaxScore - pStanding->nMinScore) +1);
			LanguageFormatNumberString(szNumberOut, arrsize(szNumberOut), szNumberIn);
			PStrReplaceToken( szBuf, arrsize(szBuf), puszToken2, szNumberOut );

			UILabelSetText(component, szBuf);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFactionStadingDispOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT(component && component->m_pParent);

	int nStandingDef = (int)component->m_pParent->m_dwData;
	const FACTION_STANDING_DEFINITION *pStanding = FactionStandingGetDefinition(nStandingDef);
	UILabelSetText(component, StringTableGetStringByIndex(pStanding->nDisplayString));

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct BULLSHIT_CALLBACK_DATA
{
	UI_RECT *pRects;
	BOOL bDoRows;
};

static int sCompareRects(
	void * pContext,
	const void * pLeft,
	const void * pRight)
{
	int nLeftIdx = *(int *)pLeft;
	int nRightIdx = *(int *)pRight;

	BULLSHIT_CALLBACK_DATA *pCallbackData = (BULLSHIT_CALLBACK_DATA *)pContext;

	if (pCallbackData->bDoRows)
	{
		return pCallbackData->pRects[nLeftIdx].Height() < pCallbackData->pRects[nRightIdx].Height();
	}
	else
	{
		return pCallbackData->pRects[nLeftIdx].Width() < pCallbackData->pRects[nRightIdx].Width();
	}
}

static BOOL sFitRectsOnscreen(
	UI_RECT *pRects,
	int nNumRects)
{
	float fScreenWidth = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX();
	float fScreenHeight = AppCommonGetWindowHeight() * UIGetScreenToLogRatioY();

	float fScreenCenterX = fScreenWidth / 2.0f;
	float fScreenCenterY = fScreenHeight / 2.0f;

	int nWidest = -1;
	int nTallest = -1;
	float fWidest = -1.0f;
	float fTallest = -1.0f;
	for (int i = 0; i < nNumRects; i++)
	{
		if (pRects[i].Width() > fWidest)
		{
			fWidest = pRects[i].Width();
			nWidest = i;
		}
		if (pRects[i].Height() > fTallest)
		{
			fTallest = pRects[i].Height();
			nTallest = i;
		}
	}

	// we may want to add more options later but we're going to start with a sort of spiral out
	if (nNumRects < 5 && fWidest < fScreenWidth / 2.0f && fTallest < fScreenHeight / 2.0f)
	{
		for (int i = 0; i < nNumRects; i++)
		{
			switch (i)
			{
			case 0: pRects[i].SetPos(fScreenCenterX,					fScreenCenterY); break;
			case 1: pRects[i].SetPos(fScreenCenterX-pRects[i].Width(),	fScreenCenterY); break;
			case 2: pRects[i].SetPos(fScreenCenterX-pRects[i].Width(),	fScreenCenterY-pRects[i].Height()); break;
			case 3: pRects[i].SetPos(fScreenCenterX,					fScreenCenterY-pRects[i].Height()); break;
			}

		}
		return TRUE;
	}
	else
	{
		float fAverageWidth = 0.0;
		float fTotalWidth = 0.0;
		float fAverageHeight = 0.0;
		float fTotalHeight = 0.0;
		for (int i = 0; i < nNumRects; i++)
		{
			fTotalWidth += pRects[i].Width();
			fTotalHeight += pRects[i].Height();
		}
		fAverageWidth = fTotalWidth / (float)nNumRects;
		fAverageHeight = fTotalHeight / (float)nNumRects;

		BULLSHIT_CALLBACK_DATA tCallbackData;
		tCallbackData.bDoRows = fAverageWidth / fScreenWidth < fAverageHeight / fScreenHeight;
		tCallbackData.pRects = pRects;

		// we can't resort the actual rects because the order matters to the caller, so we need an array of indicies
		int *pIndicies = (int *)MALLOC(NULL, sizeof(int)*nNumRects);
		for (int i = 0; i < nNumRects; i++)
			pIndicies[i] = i;

		qsort_s(pIndicies, nNumRects, sizeof(int), sCompareRects, &tCallbackData);

		float fCurX = 0.0f;
		float fCurY = 0.0f;
		float fMaxHeight = 0.0f;
		float fMaxWidth = 0.0f;
		for (int i = 0; i < nNumRects; i++)
		{
			pRects[pIndicies[i]].SetPos(fCurX, fCurY);
			if (tCallbackData.bDoRows)
			{
				fCurX += pRects[pIndicies[i]].Width();
				fMaxHeight = MAX(fMaxHeight, pRects[pIndicies[i]].Height());
				if (fCurX > fScreenWidth)
				{
					fCurX = 0.0f;
					fCurY += fMaxHeight;
					fMaxHeight = 0.0f;
				}
			}
			else
			{
				fCurY += pRects[pIndicies[i]].Height();
				fMaxWidth = MAX(fMaxHeight, pRects[pIndicies[i]].Width());
				if (fCurY > fScreenHeight)
				{
					fCurY = 0.0f;
					fCurX += fMaxWidth;
					fMaxWidth = 0.0f;
				}
			}
		}

		FREE(NULL, pIndicies);

		return fCurX < fScreenWidth && fCurY < fScreenHeight;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetHoverTextItem(
	UI_RECT *pRect,
	UNIT* item,
	float fRectScale /*= 1.0f*/)
{
	if (!item)
	{
		UIClearHoverText();
		return;
	}

	if (AppIsHellgate() && !UIComponentGetActive(g_UI.m_Cursor))
	{
		return;
	}

	if (UIShowingLoadingScreen())
	{
		return;
	}

	// If a mouse button is down, don't show the tooltip
	if (InputGetMouseButtonDown())
	{
		return;
	}

	UI_COMPONENT_ENUM eComp = UICOMP_ITEMTOOLTIP;
	if (!AppIsTugboat())
	{
		if (UnitIsA(item, UNITTYPE_MOD))
		{
			eComp = UICOMP_MODITEMTOOLTIP;
		}
		else
		if (UnitIsA(item, UNITTYPE_CONSUMABLE))
		{
			eComp = UICOMP_CONSUMABLETOOLTIP;
		}
		if (ItemIsRecipeIngredient( item ))
		{
			eComp = UICOMP_INGREDIENTTOOLTIP;
		}

	}

	UI_COMPONENT* pComponent = UIComponentGetByEnum(eComp);
	ASSERT_RETURN(pComponent);

	UI_TOOLTIP *pItemTooltip[3];
	memclear(pItemTooltip, sizeof(UI_TOOLTIP *) * 3);

	pItemTooltip[0] = UICastToTooltip(pComponent);
	int nNumTips = 1;

	if (UIGetHoverUnit() == UnitGetId(item) &&
		(UIComponentGetActive(pComponent) ||
		 UIComponentGetActive(pComponent)))
	{
		// if we're already open with the same tooltip for the same item
		return;
	}

	if (UIGetModUnit() == UnitGetId(item))
	{
		// if we're already showing all the item info in the gun mod screen, don't show this
		return;
	}

	// Hide all the other tooltips
	UIClearHoverText(eComp);

	pItemTooltip[0]->m_idFocusUnit = UnitGetId(item);
	UIComponentHandleUIMessage(pItemTooltip[0], UIMSG_PAINT, 0, 0);

	//UITooltipDetermineContents(pItemTooltip);
	//UITooltipDetermineSize(pItemTooltip);

	int nSuggestRelativePos = TTPOS_LEFT_TOP;
	if (fabs(pRect->m_fX2 - (UIDefaultWidth() / 2.0f)) < fabs(pRect->m_fX1 - (UIDefaultWidth() / 2.0f)))   // if the rect is on the left side of the screen
	{
		nSuggestRelativePos = TTPOS_RIGHT_TOP;
	}

	if (fRectScale != 1.0f && pRect)
	{
		float w = pRect->m_fX2 - pRect->m_fX1;
		float h = pRect->m_fY2 - pRect->m_fY1;
		pRect->m_fX1 -= (w / 2.0f) * (fRectScale - 1.0f);
		pRect->m_fX2 += (w / 2.0f) * (fRectScale - 1.0f);
		pRect->m_fY1 -= (h / 2.0f) * (fRectScale - 1.0f);
		pRect->m_fY2 += (h / 2.0f) * (fRectScale - 1.0f);
	}
	UITooltipPosition(pItemTooltip[0], pRect, nSuggestRelativePos);

	//DWORD dwAnimTime = 0;
	// SHOW THE SECONDARY ITEM TOOLTIP
	UNIT *pPlayerUnit = UIGetControlUnit();

	BOOL bPositioningOk = TRUE;

	// If this is a spell scroll, use the second item tooltip for a spell description
	if( UnitIsA( item, UNITTYPE_SPELLSCROLL ) )
	{

		UISetHoverTextSkill(pComponent, UIGetControlUnit(), UnitGetSkillID( item ), pRect, TRUE );
		pItemTooltip[1] = UICastToTooltip( UIComponentGetByEnum(UICOMP_SKILLTOOLTIP) );
		bPositioningOk &= UITooltipPosition(
					pItemTooltip[1],
					&UI_RECT(pItemTooltip[0]->m_Position.m_fX,
							pItemTooltip[0]->m_Position.m_fY,
							pItemTooltip[0]->m_Position.m_fX + (pItemTooltip[0]->m_fWidth * UIGetScreenToLogRatioX(pItemTooltip[0]->m_bNoScale)),
							pItemTooltip[0]->m_Position.m_fY + (pItemTooltip[0]->m_fHeight * UIGetScreenToLogRatioY(pItemTooltip[0]->m_bNoScale))),
					nSuggestRelativePos);
		nNumTips = 2;
	}
	if (!ItemIsInEquipLocation(pPlayerUnit, item) && InventoryItemMeetsClassReqs(item, pPlayerUnit))		// if this item is not equipped
	{
		int nLocation = INVLOC_NONE, x=0, y=0;
		if (!ItemGetEquipLocation(pPlayerUnit, item, &nLocation, &x, &y, INVLOC_NONE, TRUE) && nLocation != INVLOC_NONE)		// if a space is found and it's not empty
		{
			UNIT *pSecondItem = UnitInventoryGetByLocationAndXY(pPlayerUnit, nLocation, x, y);
			if (pSecondItem)
			{
				pItemTooltip[1] = UICastToTooltip(UIComponentGetByEnum(UICOMP_SECONDITEMTOOLTIP));

				pItemTooltip[1]->m_idFocusUnit = UnitGetId(pSecondItem);
				UIComponentHandleUIMessage(pItemTooltip[1], UIMSG_PAINT, 0, 0);

				nNumTips = 2;
				bPositioningOk &= UITooltipPosition(
					pItemTooltip[1],
					&UI_RECT(pItemTooltip[0]->m_Position.m_fX,
							pItemTooltip[0]->m_Position.m_fY,
							pItemTooltip[0]->m_Position.m_fX + (pItemTooltip[0]->m_fWidth * UIGetScreenToLogRatioX(pItemTooltip[0]->m_bNoScale)),
							pItemTooltip[0]->m_Position.m_fY + (pItemTooltip[0]->m_fHeight * UIGetScreenToLogRatioY(pItemTooltip[0]->m_bNoScale))),
					nSuggestRelativePos);
			}
		}

		if (UnitIsA(item, UNITTYPE_WEAPON))
		{
			UNIT *pThirdItem = UnitInventoryGetByLocationAndXY(pPlayerUnit, (nLocation == INVLOC_RHAND ? INVLOC_LHAND : INVLOC_RHAND), x, y);	//check the left hand too
			if (pThirdItem && InventoryItemMeetsClassReqs(item, pPlayerUnit))
			{
				pItemTooltip[2] = UICastToTooltip(UIComponentGetByEnum(UICOMP_THIRDITEMTOOLTIP));

				pItemTooltip[2]->m_idFocusUnit = UnitGetId(pThirdItem);
				UIComponentHandleUIMessage(pItemTooltip[2], UIMSG_PAINT, 0, 0);

				nNumTips = 3;
				if (pItemTooltip[1])
				{
					bPositioningOk &= UITooltipPosition(
						pItemTooltip[2],
						&UI_RECT(pItemTooltip[1]->m_Position.m_fX,
							pItemTooltip[1]->m_Position.m_fY,
							pItemTooltip[1]->m_Position.m_fX + (pItemTooltip[1]->m_fWidth * UIGetScreenToLogRatioX(pItemTooltip[1]->m_bNoScale)),
							pItemTooltip[1]->m_Position.m_fY + (pItemTooltip[1]->m_fHeight * UIGetScreenToLogRatioY(pItemTooltip[1]->m_bNoScale))),
						nSuggestRelativePos,
						&UI_RECT(pItemTooltip[0]->m_Position.m_fX,
							pItemTooltip[0]->m_Position.m_fY,
							pItemTooltip[0]->m_Position.m_fX + (pItemTooltip[0]->m_fWidth * UIGetScreenToLogRatioX(pItemTooltip[0]->m_bNoScale)),
							pItemTooltip[0]->m_Position.m_fY + (pItemTooltip[0]->m_fHeight * UIGetScreenToLogRatioY(pItemTooltip[0]->m_bNoScale))));
				}
				else
				{
					bPositioningOk &= UITooltipPosition(
						pItemTooltip[2],
						&UI_RECT(pItemTooltip[0]->m_Position.m_fX,
							pItemTooltip[0]->m_Position.m_fY,
							pItemTooltip[0]->m_Position.m_fX + (pItemTooltip[0]->m_fWidth * UIGetScreenToLogRatioX(pItemTooltip[0]->m_bNoScale)),
							pItemTooltip[0]->m_Position.m_fY + (pItemTooltip[0]->m_fHeight * UIGetScreenToLogRatioY(pItemTooltip[0]->m_bNoScale))),
						nSuggestRelativePos);
				}
			}
		}
	}

	if (!bPositioningOk)
	{
		// well, we couldn't fit them with our desired relative positionings so just try to make sure they're all on the screen and don't overlap
		UI_RECT rect[3];

		for (int i=0; i < nNumTips; i++)
		{
			rect[i] = UIComponentGetAbsoluteRect(pItemTooltip[i]);
		}
		sFitRectsOnscreen(rect, nNumTips);
		for (int i=0; i < nNumTips; i++)
		{
			pItemTooltip[i]->m_Position.m_fX = rect[i].m_fX1;
			pItemTooltip[i]->m_Position.m_fY = rect[i].m_fY1;
			pItemTooltip[i]->m_ActivePos = pItemTooltip[i]->m_Position;
		}
	}

	UIComponentActivate(pItemTooltip[0], TRUE);
	if (pItemTooltip[1])
		UIComponentActivate(pItemTooltip[1], TRUE);
	if (pItemTooltip[2])
		UIComponentActivate(pItemTooltip[2], TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetHoverTextItem(
	UI_COMPONENT *pComponent,
	UNIT* item,
	float fScale /*= 1.0f*/)
{
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pComponent);

	UISetHoverTextItem(
		&UI_RECT(pos.m_fX,
		pos.m_fY,
		pos.m_fX + pComponent->m_fWidth,
		pos.m_fY + pComponent->m_fHeight),
		item,
		fScale);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetRequirementFailString(
	const STATS_DATA *pStatsData)
{
	if (AppIsHellgate())
	{
		return pStatsData->m_nRequirementFailString[ RFA_HELLGATE ];
	}
	else if (AppIsTugboat())
	{
		return pStatsData->m_nRequirementFailString[ RFA_MYTHOS ];
	}
	else
	{
		return pStatsData->m_nRequirementFailString[ RFA_ALL ];
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UICheckRequirementsWithFailMessage(
	UNIT *pContainer,
	UNIT *pUnitCursor,
	UNIT *pIgnoreItem,
	int nLocation,
	UI_COMPONENT * pComponent)
{
#define NUM_FAILED	10
	DWORD pFailedRequirementStats[NUM_FAILED];
	memset(pFailedRequirementStats, INVALID_LINK, sizeof(DWORD) * NUM_FAILED);
	if (!InventoryCheckStatRequirements(pContainer, pUnitCursor, pIgnoreItem, NULL, nLocation, pFailedRequirementStats, NUM_FAILED))
	{
		for (int i=0; i < NUM_FAILED; i++)
		{
			WCHAR szFailedMsg[2048] = L"";
			if (pFailedRequirementStats[i] > INVALID_LINK)
			{
				const STATS_DATA *pStatsData = (const STATS_DATA*)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(pFailedRequirementStats[i]));
				int nString = sGetRequirementFailString( pStatsData );
				if (nString > INVALID_LINK)
				{
					if (szFailedMsg[0])
					{
						PStrCat(szFailedMsg, L"\n", 2048);
					}
					PStrCat(szFailedMsg, StringTableGetStringByIndex(nString), 2048);
				}
			}
			if (szFailedMsg[0])
			{
				UISetTimedHoverText(szFailedMsg, pComponent, (DWORD)-1);
			}
		}
		return FALSE;
	}

	return TRUE;
}


inline void GetAccumulationText( UNIT *pUnit,
								 const SKILL_DATA *pSkillData,
								 DWORD dwSkillLearnFlags,
								 WCHAR *puszBuffer,
								 int nBufferSize )
{
//print effect accumulation
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT_ACCUMULATION ) )
	{
		// get info text
		WCHAR uszEffect[ MAX_SKILL_TOOLTIP ];
		int nFlags( 0 );
		if( AppIsTugboat() )
		{
			for( int a = 0; a < MAX_SKILL_ACCULATE; a++ )
			{
				if( pSkillData->nSkillsToAccumulate[ a ] != INVALID_ID )
				{
					int nSkillToAccumulate = pSkillData->nSkillsToAccumulate[ a ];
					if( SkillGetLevel( pUnit, nSkillToAccumulate ) > 0 )
					{
						if (SkillGetString( nSkillToAccumulate, pUnit, SKILL_STRING_EFFECT_ACCUMULATION, STT_CURRENT_LEVEL, uszEffect, MAX_SKILL_TOOLTIP, nFlags, FALSE ))
						{
							if (PStrLen( uszEffect ))
							{
								PStrCat( puszBuffer, L"\n", nBufferSize );
								PStrCat( puszBuffer, uszEffect, nBufferSize );
							}
						}
					}
				}
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGetHoverTextSkill(
	int nSkill,
	int nSkillLevel,
	UNIT *pUnit,
	DWORD dwSkillLearnFlags,
	WCHAR *puszBuffer,
	int nBufferSize,
	BOOL bFormatting /* = TRUE */)
{
	ASSERTX_RETFALSE( nSkill != INVALID_LINK, "Expected skill index" );
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( puszBuffer, "Expected string buffer" );
	ASSERTX_RETFALSE( nBufferSize > 0, "Invalid buffer size" );
	WCHAR uszTemp[ MAX_SKILL_TOOLTIP ];

	// init the name
	puszBuffer[ 0 ] = 0;

	// get skill name
	const SKILL_DATA *pSkillData = SkillGetData( NULL, nSkill );
	ASSERT_RETFALSE(pSkillData);
	const WCHAR *puszSkillName = StringTableGetStringByIndex( pSkillData->nDisplayName );

	// add name
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NAME ))
	{
		PStrCat( puszBuffer, puszSkillName, nBufferSize );
	}

	int nSkillGroupCount = 0;
	for ( int i = 0; i < MAX_SKILL_GROUPS_PER_SKILL; i++ )
	{
		if ( pSkillData->pnSkillGroup[ i ] != INVALID_ID )
		{
			const SKILLGROUP_DATA * pSkillGroupData = (SKILLGROUP_DATA *) ExcelGetData( NULL, DATATABLE_SKILLGROUPS, pSkillData->pnSkillGroup[ i ] );
			if ( ! pSkillGroupData || ! pSkillGroupData->bDisplayInSkillString )
				continue;
			const WCHAR * pswString = pSkillGroupData->nDisplayString != INVALID_ID ? StringTableGetStringByIndex( pSkillGroupData->nDisplayString ) : NULL;
			if ( pswString )
			{
				if (AppIsHellgate())
				{
					if (!nSkillGroupCount)
					{
						PStrCat( puszBuffer, L"\n", nBufferSize );
						PStrCat( puszBuffer, GlobalStringGet( GS_SKILL_SKILLGROUP_PREFIX ), nBufferSize );
					}

					PStrPrintf(
						uszTemp,
						MAX_SKILL_TOOLTIP,
						nSkillGroupCount ? L", %s" : L"%s",
						pswString);
					PStrCat( puszBuffer, uszTemp, nBufferSize );
					nSkillGroupCount++;
				}
			}
		}
	}

	// add usage instructions
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_USE_INSTRUCTIONS ))
	{
		// append item type requirement (if any)
		if (pSkillData->nRequiredWeaponUnittype != INVALID_LINK &&
			( AppIsHellgate() || pSkillData->nRequiredWeaponUnittype != UNITTYPE_ANY ) )
		{
			const UNITTYPE_DATA *pUnitTypeData = UnitTypeGetData( pSkillData->nRequiredWeaponUnittype );
			int nString = pUnitTypeData->nDisplayName;
			if (nString != INVALID_LINK)
			{
				PStrPrintf(
					uszTemp,
					MAX_SKILL_TOOLTIP,
					GlobalStringGet( GS_SKILL_REQUIRES_WEAPON_UNIT_TYPE ),
					StringTableGetStringByIndex( nString ));
				PStrCat( puszBuffer, L"\n", MAX_SKILL_TOOLTIP );
				PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
			}
		}

		if ( pSkillData->pnStrings[ SKILL_STRING_AFTER_REQUIRED_WEAPON ] != INVALID_ID )
		{
			const WCHAR * pszAfterRequiredWeapon = StringTableGetStringByIndex( pSkillData->pnStrings[ SKILL_STRING_AFTER_REQUIRED_WEAPON ] );
			if ( pszAfterRequiredWeapon )
			{
				PStrCat( puszBuffer, L"\n", MAX_SKILL_TOOLTIP );
				PStrCat( puszBuffer, pszAfterRequiredWeapon, MAX_SKILL_TOOLTIP );
			}
		}
	}

	// show any missing skill prerequisites
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_MISSING_PREREQUISITE_SKILL ))
	{
		PStrCat( puszBuffer, L"\n", nBufferSize );
		for( int i = 0; i < MAX_PREREQUISITE_SKILLS; i++ )
		{
			if( pSkillData->pnRequiredSkills[i] != INVALID_ID &&
				pSkillData->pnRequiredSkillsLevels[i] > 0 )
			{
				const SKILL_DATA *pReqSkillData = SkillGetData( NULL, pSkillData->pnRequiredSkills[i] );
				if( pSkillData->bOnlyRequireOneSkill && i > 0 )
				{
					PStrPrintf( uszTemp,
						MAX_SKILL_TOOLTIP,
						GlobalStringGet( GS_SKILL_CANNOT_BUY_MISSING_PREREQ_SKILL_OR ),
						StringTableGetStringByIndex( pReqSkillData->nDisplayName ),
						pSkillData->pnRequiredSkillsLevels[i] );

					PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
				}
				else
				{
					PStrPrintf( uszTemp,
						MAX_SKILL_TOOLTIP,
						GlobalStringGet( GS_SKILL_CANNOT_BUY_MISSING_PREREQ_SKILL ),
						StringTableGetStringByIndex( pReqSkillData->nDisplayName ),
						pSkillData->pnRequiredSkillsLevels[i] );

					PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
				}

			}
			else
			{
				break;
			}
		}
	}

	// add can or cannot buy
	if( AppIsHellgate() )
	{
		const WCHAR * uszBuyString = NULL;
		if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_IS_DISABLED))
		{
			uszBuyString = GlobalStringGet( GS_SKILL_CANNOT_BUY_DISABLED );
		}
		else if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SUBSCRIPTION_REQUIRED ))
		{
			uszBuyString = GlobalStringGet( GS_SUBSCRIBER_ONLY_CHARACTER_HEADER ); //placeholder until I can get a hold of the strings - Tyler
		}
		else if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL ))
		{
			uszBuyString = GlobalStringGet( GS_SKILL_CAN_BUY );
		}
		else if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_WRONG_STYLE))
		{
			uszBuyString = GlobalStringGet( GS_SKILL_CANNOT_BUY_WRONG_STYLE );
		}
		else if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_LEVEL_IS_TOO_LOW ))
		{
			uszBuyString = GlobalStringGet( GS_SKILL_CANNOT_BUY_LEVEL_TOO_LOW );
		}

		if ( uszBuyString )
		{
			PStrPrintf(
				uszTemp,
				MAX_SKILL_TOOLTIP,
				L"\n%s",
				uszBuyString);
			PStrCat( puszBuffer, uszTemp, nBufferSize );
		}

		const WCHAR * uszLockedString = NULL;
		WCHAR uszLockedTempString[ MAX_SKILL_TOOLTIP ];
		WCHAR uszLockedFullString[ MAX_SKILL_TOOLTIP ];
		memclear(uszLockedTempString, MAX_SKILL_TOOLTIP * sizeof(WCHAR));
		memclear(uszLockedFullString, MAX_SKILL_TOOLTIP * sizeof(WCHAR));
		if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_IS_LOCKED ))
		{
			const UNIT_DATA * pItemData = pSkillData->nUnlockPurchaseItem >= 0 ? ItemGetData(pSkillData->nUnlockPurchaseItem) : NULL;
			if(pItemData && pItemData->nString >= 0)
			{
				uszLockedString = GlobalStringGet( GS_SKILL_CANNOT_BUY_LOCKED );

				const WCHAR * uszItemName = StringTableGetStringByIndex(pItemData->nString);
				if(uszItemName && uszLockedString)
				{
					PStrCopy(uszLockedTempString, uszLockedString, MAX_SKILL_TOOLTIP);

					const WCHAR *puszToken = L"[string1]";
					PStrReplaceToken( uszLockedTempString, MAX_SKILL_TOOLTIP, puszToken, uszItemName );
				}
				else
				{
					uszLockedString = GlobalStringGet( GS_SKILL_CANNOT_BUY_LOCKED_NO_ITEM );
					if(uszLockedString)
					{
						PStrCopy(uszLockedTempString, uszLockedString, MAX_SKILL_TOOLTIP);
					}
				}
			}
			else
			{
				uszLockedString = GlobalStringGet( GS_SKILL_CANNOT_BUY_LOCKED_NO_ITEM );
				if(uszLockedString)
				{
					PStrCopy(uszLockedTempString, uszLockedString, MAX_SKILL_TOOLTIP);
				}
			}
		}

		if ( uszLockedString )
		{
			PStrPrintf(
				uszLockedFullString,
				MAX_SKILL_TOOLTIP,
				L"\n%s",
				uszLockedTempString);
			PStrCat( puszBuffer, uszLockedFullString, nBufferSize );
		}
	}


	// add description
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_DESCRIPTION ) && pSkillData->pnStrings[ SKILL_STRING_DESCRIPTION ] != INVALID_ID )
	{
		DWORD dwFlags = SKILL_STRING_MASK_DESCRIPTION;
		if ( SkillGetString( nSkill, pUnit, SKILL_STRING_DESCRIPTION, STT_CURRENT_LEVEL, uszTemp, MAX_SKILL_TOOLTIP, dwFlags ) )
		{
			PStrCat( puszBuffer, L"\n\n", nBufferSize );
			PStrCat( puszBuffer, uszTemp, nBufferSize );
		}
	}

	int nMaxLevelForUI = SkillGetMaxLevelForUI( pUnit, nSkill );
	// add current level effect
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT ) )
	{
		if( AppIsHellgate() && pSkillData->pnSkillLevelToMinLevel[ 1 ] != INVALID_ID )
		{
			// create level label
			const WCHAR * pszCurrentLevelString = TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_LEVEL_ENHANCED_BY_ITEMS ) ? GlobalStringGet( GS_SKILL_CURRENT_LEVEL_ENHANCED ) : GlobalStringGet( GS_SKILL_CURRENT_LEVEL );
			if ( pszCurrentLevelString )
			{
				PStrPrintf(
					uszTemp,
					MAX_SKILL_TOOLTIP,
					pszCurrentLevelString,
					nSkillLevel,
					nMaxLevelForUI );

				// show it's at max if it is
				if ( TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_IS_MAX_LEVEL ))
				{
					PStrCat( uszTemp, L" (", MAX_SKILL_TOOLTIP );
					PStrCat( uszTemp, GlobalStringGet( GS_SKILL_LEVEL_MAX ), MAX_SKILL_TOOLTIP );
					PStrCat( uszTemp, L")", MAX_SKILL_TOOLTIP );
				}
				PStrCat( puszBuffer, L"\n\n", nBufferSize );
				PStrCat( puszBuffer, uszTemp, nBufferSize );
			}
		}
		// get info text
		WCHAR uszEffect[ MAX_SKILL_TOOLTIP ];
		DWORD dwSkillStringFlags = AppIsHellgate() ? SKILL_STRING_MASK_PER_LEVEL : 0;

		if (SkillGetString( nSkill, pUnit, SKILL_STRING_EFFECT, STT_CURRENT_LEVEL, uszEffect, MAX_SKILL_TOOLTIP, dwSkillStringFlags, bFormatting, nSkillLevel ))
		{
			if (PStrLen( uszEffect ))
			{
				PStrCat( puszBuffer, L"\n", nBufferSize );
				PStrCat( puszBuffer, uszEffect, nBufferSize );
			}
			//print effect accumulation
			GetAccumulationText( pUnit,
								 pSkillData,
								 dwSkillLearnFlags,
								 puszBuffer,
								 nBufferSize );
		}

	}

	// add next level info
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NEXT_LEVEL_EFFECT ))
	{
		// create level label
		int nSkillNextLevelAvailableAt = SkillNextLevelAvailableAt( pUnit, nSkill );
		if ( nSkillNextLevelAvailableAt != 0 && nSkillNextLevelAvailableAt != INVALID_ID )
		{
			const WCHAR * pszNextLevelString = TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_LEVEL_ENHANCED_BY_ITEMS ) ? GlobalStringGet( GS_SKILL_NEXT_LEVEL_ENHANCED ) : GlobalStringGet( GS_SKILL_NEXT_LEVEL );
			if ( pszNextLevelString )
			{
				PStrCat( puszBuffer, L"\n\n", nBufferSize );
				PStrPrintf(
					uszTemp,
					MAX_SKILL_TOOLTIP,
					pszNextLevelString,
					nSkillLevel + 1,
					nMaxLevelForUI );
				PStrCat( puszBuffer, uszTemp, nBufferSize );
			}

			if ( TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_LEVEL_IS_TOO_LOW ) )
			{
				if( AppIsHellgate() )
				{
					PStrPrintf(
						uszTemp,
						MAX_SKILL_TOOLTIP,
						GlobalStringGet( GS_SKILL_REQUIRES_LEVEL ),
						SkillNextLevelAvailableAt( pUnit, nSkill ));
					PStrCat( puszBuffer, uszTemp, nBufferSize );
				}
				else if( AppIsTugboat() )
				{
					/*
					SKILLTAB_DATA * pSkilltabData = (SKILLTAB_DATA *)ExcelGetData( NULL, DATATABLE_SKILLTABS, pSkillData->nSkillTab );
					ASSERT_RETFALSE( pSkilltabData );
					PStrPrintf(
						uszTemp,
						MAX_SKILL_TOOLTIP,
						GlobalStringGet( GS_SKILL_REQUIRES_LEVEL ),
						(((SkillNextLevelAvailableAt( pUnit, nSkill ) - 1 )/SKILL_POINTS_PER_TIER) * SKILL_POINTS_PER_TIER), //this needs
						StringTableGetStringByIndex( pSkilltabData->nDisplayString ));
					PStrCat( puszBuffer, uszTemp, nBufferSize );
					*/
				}
			}
		}

		// get info text
		WCHAR uszEffect[ MAX_SKILL_TOOLTIP ];
		DWORD dwSkillStringFlags = AppIsHellgate() ? SKILL_STRING_MASK_PER_LEVEL : 0;

		if (SkillGetString( nSkill, pUnit, SKILL_STRING_EFFECT, STT_NEXT_LEVEL, uszEffect, MAX_SKILL_TOOLTIP, dwSkillStringFlags ))
		{
			if (PStrLen( uszEffect ))
			{
				PStrCat( puszBuffer, L"\n", nBufferSize );
				PStrCat( puszBuffer, uszEffect, nBufferSize );
			}
		}

	}

	// add skill bonuses


	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_BONUSES ) && pSkillData->pnStrings[ SKILL_STRING_SKILL_BONUSES ] != INVALID_ID )
	{
		WCHAR uszEffect[ MAX_SKILL_TOOLTIP ];
		if ( SkillGetString( nSkill, pUnit, SKILL_STRING_SKILL_BONUSES, STT_CURRENT_LEVEL, uszEffect, MAX_SKILL_TOOLTIP, 0 ) )
		{
			PStrCat( puszBuffer, L"\n\n", nBufferSize );
			PStrCat( puszBuffer, uszEffect, nBufferSize );
		}
	}

	//print effect accumulation
	/*
	GetAccumulationText( pUnit,
						 pSkillData,
						 dwSkillLearnFlags,
						 puszBuffer,
						 nBufferSize );
	*/




	// add usage instructions
	if (TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_USE_INSTRUCTIONS ))
	{
		// append usage instructions
		if (pSkillData->eUsage == USAGE_ACTIVE)
		{
			PStrPrintf(
				uszTemp,
				MAX_SKILL_TOOLTIP,
				L"\n\n%s",
				GlobalStringGet( GS_SKILL_USAGE_ACTIVE ));
			PStrCat( puszBuffer, uszTemp, nBufferSize );
		}
		// append usage instructions
		else if (pSkillData->eUsage == USAGE_TOGGLE)
		{
			PStrPrintf(
				uszTemp,
				MAX_SKILL_TOOLTIP,
				L"\n\n%s",
				GlobalStringGet( GS_SKILL_USAGE_TOGGLE ));
			PStrCat( puszBuffer, uszTemp, nBufferSize );
		}

		// append shift activator (if any)
		if (pSkillData->szActivator[ 0 ] != 0 && UnitGetStat( pUnit, STATS_SKILL_SHIFT_ENABLED, nSkill ) )
		{
			// TODO: Make this give better shift skill info particular for each condition
			PStrPrintf(
				uszTemp,
				MAX_SKILL_TOOLTIP,
				L"\n%s",
				GlobalStringGet( GS_SKILL_SHIFT_ACTIVATOR ));
			PStrCat( puszBuffer, uszTemp, MAX_SKILL_TOOLTIP );
		}
	}

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetHoverTextSkill(
	UI_COMPONENT *pComponent,
	UNIT *pUnit,
	int nSkill,
	UI_RECT *pPositionRect /*= NULL*/,
	BOOL bSecondary /* = FALSE*/,
	BOOL bNoReposition /* = FALSE*/,
	BOOL bFormatting /* = TRUE*/)
{
	UI_COMPONENT* pTemp = UIComponentGetByEnum(UICOMP_SKILLTOOLTIP);
	ASSERT_RETURN(pTemp);
	UI_TOOLTIP *pTooltip = UICastToTooltip(pTemp);

	BOOL bNewSkill = (pTooltip->m_dwData != (DWORD) nSkill);

	// store the skill ID so we can refresh later if it goes up
	pTooltip->m_dwData = (DWORD) nSkill;

	// If a mouse button is down, don't show the tooltip
	//if (InputGetMouseButtonDown())
	//{
	//	return;
	//}

	if (AppIsHellgate() && !UIComponentGetActive(g_UI.m_Cursor))
	{
		return;
	}

	if (UIShowingLoadingScreen())
	{
		return;
	}

	// must have a valid skill
	if (nSkill != INVALID_ID)
	{

		if( AppIsTugboat() && !bSecondary )
		{
			UIClearHoverText(UICOMP_SKILLTOOLTIP);
		}

		// get the tooltip
		WCHAR uszTooltip[ MAX_SKILL_TOOLTIP ] = L"";
		if (bNewSkill)
		{
			// by default we just want the skill info with the skill rank like when
			// we're mousing over a skill unit set into the hotkey bar
			DWORD dwSkillLearnFlags = 0;
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NAME );
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_DESCRIPTION );
			if( AppIsTugboat() &&
				UIPaneVisible( KPaneMenuSkills ) )
			{
				SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_BONUSES );
			}
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
			SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_USE_INSTRUCTIONS );

			// for players, we show more info
			int nSkillLevel = 0;
			if (UnitIsA( pUnit, UNITTYPE_PLAYER ))
			{
				// get skill level
				nSkillLevel = SkillGetLevel( pUnit, nSkill );

				// if we have a level, show the effect
				if (nSkillLevel < 1)
				{
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
				}
				if( AppIsTugboat() )
				{
					if( nSkillLevel >= 1 )
					{
						SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT_ACCUMULATION );
					}
					if( !UIPaneVisible( KPaneMenuSkills ) && !UIPaneVisible( KPaneMenuCrafting ) )
					{
						CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
					}
				}
				const SKILL_DATA * pSkillData = SkillGetData( NULL, nSkill );
				if( !pSkillData ||
					SkillDataTestFlag( pSkillData, SKILL_FLAG_FORCE_UI_SHOW_EFFECT) )
				{
					SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
				}

				dwSkillLearnFlags |= SkillGetNextLevelFlags( pUnit, nSkill );

				if ( ! TESTBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_IS_MAX_LEVEL ) &&
					(AppIsHellgate() || (UIPaneVisible( KPaneMenuSkills ) || UIPaneVisible( KPaneMenuCrafting )) ) )
				{
					// show the next level effect
					SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NEXT_LEVEL_EFFECT );
				}

				if(AppIsHellgate() && TESTBIT(dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_IS_DISABLED))
				{
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_DESCRIPTION );
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NEXT_LEVEL_EFFECT );
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_USE_INSTRUCTIONS );
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_BONUSES );
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT_ACCUMULATION );
					CLEARBIT( dwSkillLearnFlags, SKILLLEARN_BIT_SKILL_LEVEL_ENHANCED_BY_ITEMS );
				}
			}

			if (!UIGetHoverTextSkill( nSkill, nSkillLevel, pUnit, dwSkillLearnFlags, uszTooltip, MAX_SKILL_TOOLTIP, bFormatting ))
			{
				UIComponentActivate(pTooltip, FALSE);
				return;
			}
		}

		int nSuggestRelativePos = TTPOS_RIGHT_TOP;
		if( pPositionRect )
		{
			nSuggestRelativePos = TTPOS_LEFT_TOP;
			if (fabs(pPositionRect->m_fX2 - (UIDefaultWidth() / 2.0f)) < fabs(pPositionRect->m_fX1 - (UIDefaultWidth() / 2.0f)))   // if the rect is on the left side of the screen
			{
				nSuggestRelativePos = TTPOS_RIGHT_TOP;
			}
		}

		// set on all labels of tooltip
		if (bNewSkill)
		{
			UI_COMPONENT *pChild = pTooltip->m_pFirstChild;
			while (pChild)
			{
				if (pChild->m_eComponentType == UITYPE_LABEL)
				{
					UILabelSetText(pChild, uszTooltip);
				}
				pChild = pChild->m_pNextSibling;
			}
		}

		UITooltipDetermineContents(pTooltip);
		UITooltipDetermineSize(pTooltip);
		if (!bNoReposition)
		{
			ASSERT_RETURN(pComponent);
			UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pComponent);
			UITooltipPosition(
				pTooltip,
				(pPositionRect ? pPositionRect :
				&UI_RECT(pos.m_fX,
				pos.m_fY,
				pos.m_fX + pComponent->m_fWidth,
				pos.m_fY + pComponent->m_fHeight)),
				nSuggestRelativePos);
		}

		UIComponentActivate(pTooltip, TRUE);

	}
	else
	{
		UIComponentActivate(pTooltip, FALSE);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillTooltipOnSetControlStat(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (STATS_SKILL_LEVEL == STAT_GET_STAT(lParam) &&
		(int)component->m_dwData == STAT_GET_PARAM(lParam))
	{
		UISetHoverTextSkill(NULL, UIGetControlUnit(), (int)component->m_dwData, NULL, FALSE, TRUE);
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetSimpleHoverText(
	float x1,
	float y1,
	float x2,
	float y2,
	const WCHAR *szString,
	BOOL bEvenIfMouseDown /*= FALSE*/,
	int nAssociateComponentID /*= INVALID_ID*/)
{
	UI_COMPONENT* pTemp = UIComponentGetByEnum(UICOMP_SIMPLETOOLTIP);
	ASSERT_RETURN(pTemp);
	UI_TOOLTIP *pTooltip = UICastToTooltip(pTemp);

	// If a mouse button is down, don't show the tooltip
	if (InputGetMouseButtonDown() && !bEvenIfMouseDown)
	{
		return;
	}

	if (UIShowingLoadingScreen())
	{
		return;
	}

	UIClearHoverText(UICOMP_SIMPLETOOLTIP);

	if (szString && szString[0])
	{
		pTooltip->m_dwData = (DWORD)nAssociateComponentID;

		UI_COMPONENT *pChild = pTooltip->m_pFirstChild;
		while (pChild)
		{
			if (pChild->m_eComponentType == UITYPE_LABEL)
			{
				UILabelSetText(pChild, szString);
			}
			pChild = pChild->m_pNextSibling;
		}
		UITooltipDetermineContents(pTooltip);
		UITooltipDetermineSize(pTooltip);

		UITooltipPosition( pTooltip, &UI_RECT(x1, y1, x2, y2) );

		if (pTooltip->m_eState != UISTATE_ACTIVE &&
			pTooltip->m_eState != UISTATE_ACTIVATING)
		{
			UIComponentActivate(pTooltip, TRUE);
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetSimpleHoverText(
	UI_COMPONENT *pComponentForPosition,
	const WCHAR *szString,
	BOOL bEvenIfMouseDown /*= FALSE*/,
	int nAssociateComponentID /*= INVALID_ID*/)
{
	ASSERT_RETURN(pComponentForPosition);

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pComponentForPosition);

	UISetSimpleHoverText(
		pos.m_fX,
		pos.m_fY,
		pos.m_fX + pComponentForPosition->m_fWidth,
		pos.m_fY + pComponentForPosition->m_fHeight,
		szString,
		bEvenIfMouseDown,
		nAssociateComponentID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetTimedHoverText(
	const WCHAR *szString,
	UI_RECT *pRect,
	DWORD dwDelayTime = (DWORD)-1)
{
	if (UIShowingLoadingScreen())
	{
		return;
	}

	UI_COMPONENT* pTemp = UIComponentGetByEnum(UICOMP_TIMEDTOOLTIP);
	ASSERT_RETURN(pTemp);
	UI_TOOLTIP *pTooltip = UICastToTooltip(pTemp);

	if (dwDelayTime == (DWORD)-1)
	{
		dwDelayTime = (DWORD)pTooltip->m_dwParam;
	}

	UI_COMPONENT *pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_LABEL)
		{
			UILabelSetText(pChild, szString);
		}
		pChild = pChild->m_pNextSibling;
	}
	UITooltipDetermineSize(pTooltip);
	UITooltipPosition( pTooltip, pRect );

	if (pTooltip->m_eState != UISTATE_ACTIVE &&
		pTooltip->m_eState != UISTATE_ACTIVATING)
	{
		UIComponentActivate(pTooltip, TRUE);
	}

	UIComponentActivate(pTooltip, FALSE, dwDelayTime);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetTimedHoverText(
	const WCHAR *szString,
	UI_COMPONENT *pComponent /*= NULL*/,
	DWORD dwDelayTime /* = (DWORD)-1*/)
{
	if (pComponent)
	{
		UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pComponent);

		UISetTimedHoverText(
			szString,
			&UI_RECT(pos.m_fX,
 					 pos.m_fY,
 					 pos.m_fX + pComponent->m_fWidth,
 					 pos.m_fY + pComponent->m_fHeight),
			dwDelayTime );
	}
	else
	{
		UISetTimedHoverText( szString, (UI_RECT *)NULL, dwDelayTime );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentMouseHoverLong(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (component->m_szTooltipText)
	{											 // vv this was changed from GetActive so we could have disabled components that would still show their tooltips
		if (UIComponentCheckBounds(component) && UIComponentGetVisible(component))
		{
			if( AppIsHellgate() )
			{
				UISetSimpleHoverText(component, component->m_szTooltipText, component->m_bHoverTextWithMouseDown, component->m_idComponent);
			}
/*			else
			{
				UISetSimpleHoverText(g_UI.m_Cursor, component->m_szTooltipText, component->m_bHoverTextWithMouseDown);
			}*/
// Tug does everything on the short hover
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentMouseHover(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (component->m_szTooltipText)
	{											 // vv this was changed from GetActive so we could have disabled components that would still show their tooltips
		if (UIComponentCheckBounds(component) && UIComponentGetVisible(component))
		{
			if( AppIsTugboat() )
			{
				UISetSimpleHoverText(g_UI.m_Cursor, component->m_szTooltipText, component->m_bHoverTextWithMouseDown, component->m_idComponent);
			}
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentRefreshTextKey(
	struct UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentIsLabel( pComponent ))
	{
#if ISVERSION(DEVELOPMENT)
		UI_LABELEX *pLabel = UICastToLabel( pComponent );
		if (pLabel->m_szTextKey[ 0 ] != 0)
		{
			UILabelSetTextByStringKey( pComponent, pLabel->m_szTextKey );
		}
#endif
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentLanguageChanged(
	struct UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent != NULL, UIMSG_RET_NOT_HANDLED);

	if (UIComponentIsLabel( pComponent ))
	{
		UI_LABELEX *pLabel = UICastToLabel( pComponent );
		if (pLabel->m_bTextHasKey && pLabel->m_nStringIndex != -1)
		{
			UILabelSetTextByStringIndex( pComponent, pLabel->m_nStringIndex );
			return UIMSG_RET_HANDLED;
		}
	}

	if (UIComponentIsUnitNameDisplay( pComponent ))
	{
		UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay( pComponent );

		if (!pNameDisp->m_pPickupLine)
			pNameDisp->m_pPickupLine = MALLOC_NEW(NULL, UI_LINE);

		pNameDisp->m_pPickupLine->SetText(GlobalStringGet( GS_UI_PICK_UP_TEXT ));
		UIReplaceStringTags(pNameDisp, pNameDisp->m_pPickupLine);

		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentOnMouseLeave(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component->m_szTooltipText)
	{
		UIClearHoverText();
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISkillShowCooldown(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill)
{
	UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)GMALLOCZ(pGame, sizeof(UI_STATE_MSG_DATA));
	pData->m_pSource		= NULL;
	pData->m_nState			= INVALID_ID;
	pData->m_wStateIndex	= 0;
	pData->m_nDuration		= 0;
	pData->m_nSkill			= nSkill;
	pData->m_bClearing		= FALSE;

	if (pUnit == GameGetControlUnit(pGame))
	{
		UISendMessage(WM_SETCONTROLSTATE, UnitGetId(pUnit), (LPARAM)pData);
	}
	else
	{
		UISendMessage(WM_SETTARGETSTATE, UnitGetId(pUnit),	(LPARAM)pData);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentBlinkUpdate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}

	if (!component->m_pBlinkData || component->m_pBlinkData->m_eBlinkState == UIBLINKSTATE_NOT_BLINKING)
	{
		return;
	}

	component->m_pBlinkData->m_dwBlinkAnimTicket = INVALID_ID;

	if (component->m_pBlinkData->m_nBlinkDuration > 0)
	{
		if (AppCommonGetCurTime() - component->m_pBlinkData->m_timeBlinkStarted >= component->m_pBlinkData->m_nBlinkDuration)
		{
			BOOL bTurnInvisibleAfter = (BOOL)data.m_Data2;
			component->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_NOT_BLINKING;
			UISetNeedToRender(component);

			if (bTurnInvisibleAfter)
			{
				UIComponentSetVisible(component, FALSE);
			}
			return;
		}
	}

	// Schedule the next update
	if (component->m_pBlinkData->m_dwBlinkAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_pBlinkData->m_dwBlinkAnimTicket);
	}
	component->m_pBlinkData->m_dwBlinkAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + component->m_pBlinkData->m_nBlinkPeriod, UIComponentBlinkUpdate, data);

	component->m_pBlinkData->m_timeLastBlinkChange = AppCommonGetCurTime();
	if (component->m_pBlinkData->m_eBlinkState == UIBLINKSTATE_ON)
	{
		component->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_OFF;
		UISetNeedToRender(component);
	}
	else if (component->m_pBlinkData->m_eBlinkState == UIBLINKSTATE_OFF)
	{
		component->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_ON;
		UISetNeedToRender(component);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentBlink(
	UI_COMPONENT *pComponent,
	int *pnDuration /*= NULL*/,
	int *pnPeriod /*= NULL*/,
	BOOL bTurnInvisibleAfter /*= FALSE*/)
{
	ASSERT_RETURN(pComponent);
	if (pComponent)
	{
		if (!pComponent->m_pBlinkData)
		{
			pComponent->m_pBlinkData = (UI_BLINKDATA *)MALLOC(NULL, sizeof(UI_BLINKDATA));
			pComponent->m_pBlinkData->m_nBlinkDuration = -1;
			pComponent->m_pBlinkData->m_nBlinkPeriod = 500;
			pComponent->m_pBlinkData->m_dwBlinkAnimTicket = INVALID_ID;
		}

		pComponent->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_ON;
		pComponent->m_pBlinkData->m_timeBlinkStarted = AppCommonGetCurTime();
		pComponent->m_pBlinkData->m_timeLastBlinkChange = AppCommonGetCurTime();
		if (pnDuration)
			pComponent->m_pBlinkData->m_nBlinkDuration = *pnDuration;
		if (pnPeriod)
			pComponent->m_pBlinkData->m_nBlinkPeriod = *pnPeriod;

		if (pComponent->m_pBlinkData->m_dwBlinkAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(pComponent->m_pBlinkData->m_dwBlinkAnimTicket);
		}
		pComponent->m_pBlinkData->m_dwBlinkAnimTicket = CSchedulerRegisterEventImm(UIComponentBlinkUpdate, CEVENT_DATA((DWORD_PTR) pComponent, (DWORD_PTR)bTurnInvisibleAfter));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentBlinkStop(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);
	if (pComponent && pComponent->m_pBlinkData)
	{
		pComponent->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_NOT_BLINKING;
		UISetNeedToRender(pComponent);
		if (pComponent->m_pBlinkData->m_dwBlinkAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(pComponent->m_pBlinkData->m_dwBlinkAnimTicket);
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIElementBlinkUpdate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_GFXELEMENT* pElement = (UI_GFXELEMENT*)data.m_Data1;
	if (!pElement)
	{
		return;
	}

	if (!pElement->m_pBlinkdata || pElement->m_pBlinkdata->m_eBlinkState == UIBLINKSTATE_NOT_BLINKING)
	{
		return;
	}

	if (pElement->m_pBlinkdata->m_nBlinkDuration > 0)
	{
		if (AppCommonGetCurTime() - pElement->m_pBlinkdata->m_timeBlinkStarted >= pElement->m_pBlinkdata->m_nBlinkDuration)
		{
			if (pElement->m_pBlinkdata->m_eBlinkState != UIBLINKSTATE_OFF)
			{
				pElement->m_pBlinkdata->m_eBlinkState = UIBLINKSTATE_OFF; //UIBLINKSTATE_NOT_BLINKING;
				UISetNeedToRender(pElement->m_pComponent);
			}
			return;
		}
	}

	if (pElement->m_pBlinkdata->m_dwBlinkAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(pElement->m_pBlinkdata->m_dwBlinkAnimTicket);
	}
	// Schedule the next update
	pElement->m_pBlinkdata->m_dwBlinkAnimTicket = CSchedulerRegisterEventImm(UIElementBlinkUpdate, CEVENT_DATA((DWORD_PTR) pElement));

	if (AppCommonGetCurTime() - pElement->m_pBlinkdata->m_timeLastBlinkChange < pElement->m_pBlinkdata->m_nBlinkPeriod)
	{
		return;
	}

	pElement->m_pBlinkdata->m_timeLastBlinkChange = AppCommonGetCurTime();
	if (pElement->m_pBlinkdata->m_eBlinkState == UIBLINKSTATE_ON)
	{
		pElement->m_pBlinkdata->m_eBlinkState = UIBLINKSTATE_OFF;
		UISetNeedToRender(pElement->m_pComponent);
	}
	else if (pElement->m_pBlinkdata->m_eBlinkState == UIBLINKSTATE_OFF)
	{
		pElement->m_pBlinkdata->m_eBlinkState = UIBLINKSTATE_ON;
		UISetNeedToRender(pElement->m_pComponent);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIElementStartBlink(
	UI_GFXELEMENT *pElement,
	int nBlinkDuration,
	int nBlinkPeriod,
	DWORD dwBlinkColor)
{
	ASSERT_RETURN(pElement);
	if (!pElement->m_pBlinkdata)
		pElement->m_pBlinkdata = (UI_BLINKDATA *) MALLOC(NULL, sizeof(UI_BLINKDATA));

	pElement->m_pBlinkdata->m_eBlinkState = UIBLINKSTATE_ON;
	pElement->m_pBlinkdata->m_timeBlinkStarted = AppCommonGetCurTime();
	pElement->m_pBlinkdata->m_timeLastBlinkChange = AppCommonGetCurTime();
	pElement->m_pBlinkdata->m_nBlinkDuration = nBlinkDuration;
	pElement->m_pBlinkdata->m_nBlinkPeriod = nBlinkPeriod;
	pElement->m_pBlinkdata->m_dwBlinkColor = dwBlinkColor;
	if (pElement->m_pBlinkdata->m_dwBlinkAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(pElement->m_pBlinkdata->m_dwBlinkAnimTicket);
	}

	pElement->m_pBlinkdata->m_dwBlinkAnimTicket = CSchedulerRegisterEventImm(UIElementBlinkUpdate, CEVENT_DATA((DWORD_PTR) pElement));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentAnimate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	BOOL bWindowshade = (BOOL)(data.m_Data3 & UIACTIVATEFLAG_WINDOWSHADE);
	BOOL bImmediate = (BOOL)(data.m_Data3 & UIACTIVATEFLAG_IMMEDIATE);

	int nState = (int)data.m_Data2;
	if (nState > -1)
	{
		component->m_eState = nState;
	}

	float fTimeRatio = 1.0f;
	if (!bImmediate)
	{
		TIME tiCurrent = (component->m_bAnimatesWhenPaused ?  AppCommonGetAbsTime() : AppCommonGetCurTime());
		fTimeRatio = ((float)(tiCurrent - component->m_tMainAnimInfo.m_tiAnimStart))/(float)component->m_tMainAnimInfo.m_dwAnimDuration;
	}

	if ((component->m_bFadesIn && component->m_eState == UISTATE_ACTIVATING) ||
		(component->m_bFadesOut && component->m_eState == UISTATE_INACTIVATING))
	{
		float fFading = 0.0f;

		//BOOL bForce = (BOOL)data.m_Data2;
		//int nDesiredState = (int)data.m_Data3;

		//if (bForce)
		//{
		//	component->m_eState = nDesiredState;
		//}

		if (fTimeRatio >= 1.0f)
		{
			fFading = 0.0f;
		}
		else
		{
			switch (component->m_eState)
			{
			case UISTATE_ACTIVATING:
				fFading = 1.0f - fTimeRatio;
				break;
			case UISTATE_INACTIVATING:
				fFading = fTimeRatio;
				break;
			default:
				//			fFading = 1.0f;		// if the component's state was changed in the middle of fading in or out, set the fading back to default
				break;
			}
		}
		UIComponentSetFade(component, fFading);
	}

	if (component->m_AnimStartPos != component->m_AnimEndPos)
	{
		UIComponentUpdatePosition(component, component->m_AnimStartPos, component->m_AnimEndPos, fTimeRatio);

		if (bWindowshade)
		{
			if (fTimeRatio < 1.0f)
			{
				if (component->m_ActivePos.m_fX < component->m_InactivePos.m_fX)
				{
					component->m_rectClip.m_fX1 = 0.0f;
					component->m_rectClip.m_fX2 = component->m_fWidth - (component->m_Position.m_fX - component->m_ActivePos.m_fX);
				}
				else
				{
					component->m_rectClip.m_fX1 = component->m_ActivePos.m_fX - component->m_Position.m_fX;
					component->m_rectClip.m_fX2 = component->m_fWidth;
				}

				if (component->m_ActivePos.m_fY < component->m_InactivePos.m_fY)
				{
					component->m_rectClip.m_fY1 = 0.0f;
					component->m_rectClip.m_fY2 = component->m_fHeight - (component->m_Position.m_fY - component->m_ActivePos.m_fY);
				}
				else
				{
					component->m_rectClip.m_fY1 = component->m_ActivePos.m_fY - component->m_Position.m_fY;
					component->m_rectClip.m_fY2 = component->m_fHeight;
				}
			}

			sComponentClipChildren(component);

			UISetNeedToRender(component);
		}

		if (data.m_Data2 == UISTATE_ACTIVATING)
		{
			UIComponentSetVisible(component, TRUE);
		}

		int idComponent = INVALID_ID;
		if (data.m_Data2 == UISTATE_ACTIVATING &&
			bWindowshade)
		{
			idComponent = component->m_idComponent;
		}
		UIComponentHandleUIMessage(component, UIMSG_ANIMATE, idComponent, 0);
												// ^^^ this is mainly for the windowshading panels in a scrollable panel
												// if we need the param for something else later we can change it
	}

	if (fTimeRatio < 1.0f)
	{
		component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentAnimate, data);
		return;
	}

	component->m_tMainAnimInfo.m_tiAnimStart = 0;
	component->m_tMainAnimInfo.m_dwAnimTicket = INVALID_ID;
	switch (data.m_Data2)
	{
	case UISTATE_INACTIVATING:
		UIComponentSetActive(component, FALSE);
		UIComponentSetVisible(component, FALSE);

		ACTIVATE_TRACE(">>> %22s %20s\n", "FINISHED INACTIVATING", component->m_szName);
		break;
	case UISTATE_ACTIVATING:
		UIComponentSetActive(component, TRUE);
		ACTIVATE_TRACE(">>> %22s %20s\n", "FINISHED ACTIVATING", component->m_szName);
		break;
	}

	if (bWindowshade)
	{
		structclear(component->m_rectClip);
		sComponentClipChildren(component);
		UISetNeedToRender(component);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICancelDelayedActivates(
	UI_COMPONENT *pRelatedToComponent)
{
	// cancel any scheduled activates for components that are related to this one
	ANIM_RELATIONSHIP_NODE * pRelationship = pRelatedToComponent->m_pAnimRelationships;
	while (pRelationship &&
		pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
	{
		UI_COMPONENT *pComp = pRelationship->Get();
		if (pComp)
		{
			PList<UI_DELAYED_ACTIVATE_INFO>::USER_NODE *pNode = g_UI.m_listDelayedActivateTickets.GetNext(NULL);
			while (pNode != NULL)
			{
				if (pNode->Value.pComponent == pComp)
				{
					ACTIVATE_TRACE(">>> %22s %20s %8s Ticket = %u\n", "CANCEL SCHEDULED EVENT", "", "", pNode->Value.dwTicket);
					CSchedulerCancelEvent(pNode->Value.dwTicket);
					pNode = g_UI.m_listDelayedActivateTickets.RemoveCurrent(pNode);
				}
				else
				{
					pNode = g_UI.m_listDelayedActivateTickets.GetNext(pNode);
				}
			}
		}

		pRelationship++;
	}

	PList<UI_DELAYED_ACTIVATE_INFO>::USER_NODE *pNode = g_UI.m_listDelayedActivateTickets.GetNext(NULL);
	while (pNode != NULL)
	{
		if (pNode->Value.pComponent == pRelatedToComponent)
		{
			ACTIVATE_TRACE(">>> %22s %20s %8s Ticket = %u\n", "CANCEL SCHEDULED EVENT", "", "", pNode->Value.dwTicket);
			CSchedulerCancelEvent(pNode->Value.dwTicket);
			pNode = g_UI.m_listDelayedActivateTickets.RemoveCurrent(pNode);
		}
		else
		{
			pNode = g_UI.m_listDelayedActivateTickets.GetNext(pNode);
		}
	}
	//DWORD dwTicket = INVALID_ID;
	//while (g_UI.m_listDelayedActivateTickets.PopHead(dwTicket))
	//{
	//	trace(">>> CANCELLING SCHEDULE ACTIVATE - Ticket = %u\n", dwTicket);
	//	CSchedulerCancelEvent(dwTicket);
	//}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void	sCancelAllDelayedActivates(
	void)
{
	// cancel any scheduled activates
	PList<UI_DELAYED_ACTIVATE_INFO>::USER_NODE *pNode = g_UI.m_listDelayedActivateTickets.GetNext(NULL);
	while (pNode != NULL)
	{
		ACTIVATE_TRACE(">>> %22s %20s %8s Ticket = %u\n", "CANCEL SCHEDULED EVENT", "", "", pNode->Value.dwTicket);
		CSchedulerCancelEvent(pNode->Value.dwTicket);
		pNode = g_UI.m_listDelayedActivateTickets.RemoveCurrent(pNode);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddDelayedActivateTicket(
	UI_COMPONENT *pComponent,
	DWORD dwAnimTicket)
{
	UI_DELAYED_ACTIVATE_INFO tNode;
	tNode.dwTicket = dwAnimTicket;
	tNode.pComponent = pComponent;
	g_UI.m_listDelayedActivateTickets.PListPushTail(tNode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRemoveDelayedActivateTicket(
	DWORD dwAnimTicket)
{
	ACTIVATE_TRACE(">>> %22s %20s %8s Ticket = %u\n", "ENDED SCHEDULED EVENT", "", "", dwAnimTicket);

	PList<UI_DELAYED_ACTIVATE_INFO>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listDelayedActivateTickets.GetNext(pNode)) != NULL)
	{
		if (pNode->Value.dwTicket == dwAnimTicket)
		{
			g_UI.m_listDelayedActivateTickets.RemoveCurrent(pNode);
			return;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIComponentActivate(
	UI_COMPONENT* component,
	BOOL bActivate,
	DWORD dwDelay /*= 0*/,
	DWORD * pdwTotalAnimTime /*= NULL*/,
	BOOL bFromAnimRelationship /*= FALSE*/,
	BOOL bOnlyIfUserActive /*= FALSE*/,
	BOOL bSetUserActive /*= TRUE*/,
	BOOL bImmediate /*= FALSE*/)
{
	ASSERT_RETFALSE(component);

	if (bActivate && UIComponentGetActive(component))
	{
		if (bSetUserActive && !component->m_bUserActive)
		{
			component->m_bUserActive = TRUE;
			UIComponentHandleUIMessage(component, UIMSG_POSTACTIVATE, 0, 0);	// send a message to the component it's really closed now
																		// this could be formalized in a SetUserActive function
																		// currently only the item upgrade components differentiate useractive on postinactivate
		}
		return FALSE;
	}

	if (!bActivate && !UIComponentGetVisible(component))
	{
		if (bSetUserActive && component->m_bUserActive)
		{
			component->m_bUserActive = FALSE;
			UIComponentHandleUIMessage(component, UIMSG_POSTINACTIVATE, 0, 0);	// send a message to the component it's really closed now
																		// this could be formalized in a SetUserActive function
																		// currently only the item upgrade components differentiate useractive on postinactivate
		}

		if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
		}

		if (component->m_pFrameAnimInfo && component->m_pFrameAnimInfo->m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(component->m_pFrameAnimInfo->m_dwAnimTicket);
		}

		return FALSE;
	}

	if (bOnlyIfUserActive && !component->m_bUserActive)
	{
		return FALSE;
	}

	ANIM_RELATIONSHIP_NODE * pRelationship = NULL;

	// we don't want overlaps.  If anyone was scheduled to come sliding in, cancel it
	if (!bFromAnimRelationship)
	{
		ACTIVATE_TRACE("\n>>>>>>>>>>>>>>>>>>>>>>> %s (%d) <<<<<<<<<<<<<<<<<<<<<<<<<<\n", component->m_szName, component->m_idComponent);
		UICancelDelayedActivates(component);
	}

	////we absolutely don't want overlaps.  If anybody's moving, cancel this
	//pRelationship = component->m_pAnimRelationships;
	//while (pRelationship &&
	//	pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
	//{
	//	UI_COMPONENT *pComp = pRelationship->Get();
	//	if (pComp)
	//	{
	//		if (pComp->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
	//			CSchedulerIsValidTicket(pComp->m_tMainAnimInfo.m_dwAnimTicket))
	//		{
	//			return FALSE;
	//		}
	//	}

	//	pRelationship++;
	//}

	if (bActivate)
	{
		// first we have to check for blockers
		pRelationship = component->m_pAnimRelationships;
		while (pRelationship &&
			pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
		{
			if (pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_BLOCKED_BY)
			{
				UI_COMPONENT *pComp = pRelationship->Get();
				if (pComp)
				{
					if (UIComponentGetVisible(pComp))
						return FALSE;
				}
			}

			pRelationship++;
		}
	}

	// Look for things to close
	DWORD dwCloseAnimTime = 0;
	pRelationship = component->m_pAnimRelationships;
	while (pRelationship &&
		pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
	{
		if (!pRelationship->m_bDirectOnly || !bFromAnimRelationship)
		{
			UI_COMPONENT *pComp = pRelationship->Get();
			if (pComp)
			{
				if ((!bActivate && pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_LINKED) ||
					( bActivate && pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_REPLACES) ||
					( bActivate == pRelationship->m_bOnActivate && pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_CLOSES) )
				{
					DWORD dwOtherAnimTime = 0;
					BOOL bOtherSetUserActive = bSetUserActive && !pRelationship->m_bPreserverUserActive;
					if (UIComponentActivate(pComp, FALSE, dwDelay, &dwOtherAnimTime, TRUE, FALSE, bOtherSetUserActive, bImmediate))
					{
						if (pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_REPLACES)
						{
							component->m_pReplacedComponent = pComp;
						}
						if (pRelationship->m_bNoDelay || bImmediate)
							dwOtherAnimTime = 0;
						dwCloseAnimTime = MAX(dwOtherAnimTime, dwCloseAnimTime);
					}
				}
			}
		}

		pRelationship++;
	}

	if (!bActivate)
	{
		if (!bImmediate && dwDelay > 0)
		{
			DWORD dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwDelay, UIDelayedActivate, CEVENT_DATA((DWORD_PTR)component, UIMSG_INACTIVATE, bFromAnimRelationship, bSetUserActive));
			ACTIVATE_TRACE(">>> %22s %20s in %5u Ticket = %8u Visible=%d  Active=%d  ActiveFromAnimRel=%d  bSetUserActive=%d  bOnlyIfUserActive=%d\n",
				"SCHEDULE INACTIVATE", component->m_szName, dwDelay, dwAnimTicket, component->m_bVisible, component->m_eState, bFromAnimRelationship, bSetUserActive, bOnlyIfUserActive);
			sAddDelayedActivateTicket(component, dwAnimTicket);
		}
		else
		{
			ACTIVATE_TRACE(">>> %22s %20s %26s Visible=%d  Active=%d  ActiveFromAnimRel=%d  bSetUserActive=%d  bOnlyIfUserActive=%d\n",
				"DEACTIVATING", component->m_szName, "", component->m_bVisible, component->m_eState, bFromAnimRelationship, bSetUserActive, bOnlyIfUserActive);
			LPARAM lParam = (bImmediate ? UIACTIVATEFLAG_IMMEDIATE : 0);
			UI_MSG_RETVAL eResult = UIComponentHandleUIMessage(component, UIMSG_INACTIVATE, 0, lParam, FALSE);
			if (!ResultIsHandled(eResult))
			{
				return FALSE;
			}
		}
		dwCloseAnimTime = MAX(component->m_dwAnimDuration, dwCloseAnimTime);
	}

	// now look for things to open
	DWORD dwOpenAnimTime = 0;
	pRelationship = component->m_pAnimRelationships;
	while (pRelationship &&
		pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
	{
		if (!pRelationship->m_bDirectOnly || !bFromAnimRelationship)
		{
			UI_COMPONENT *pComp = pRelationship->Get();
			if (pComp)
			{
				if (( bActivate && pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_LINKED) ||
					(!bActivate && pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_REPLACES) ||
					( bActivate == pRelationship->m_bOnActivate && pRelationship->m_eRelType == ANIM_RELATIONSHIP_NODE::ANIM_REL_OPENS) )
				{
					if (pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_REPLACES ||
						pComp == component->m_pReplacedComponent)
					{
						DWORD dwOtherAnimTime = 0;
						BOOL bOtherSetUserActive = bSetUserActive && !pRelationship->m_bPreserverUserActive;
						if (UIComponentActivate(pComp, TRUE, dwCloseAnimTime, &dwOtherAnimTime, TRUE, pRelationship->m_bOnlyIfUserActive, bOtherSetUserActive, bImmediate))
						{
							dwOpenAnimTime = MAX(dwOtherAnimTime, dwOpenAnimTime);
						}
					}
				}
			}
		}
		pRelationship++;
	}

	if (bActivate)
	{
		if (!bImmediate)
			dwDelay += dwCloseAnimTime + dwOpenAnimTime;

		// CHB 2007.08.17 - Do not delay activation if the timer is paused,
		// otherwise the activation event will never occur. (Tooltips require
		// a close anim time and this was affecting the options dialog.)
		BOOL bPaused = AppCommonIsPaused();
		//if (dwDelay > 0)
		if (!bImmediate && (dwDelay > 0) && (!bPaused))
		{
			DWORD dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwDelay, UIDelayedActivate, CEVENT_DATA((DWORD_PTR)component, UIMSG_ACTIVATE, bFromAnimRelationship, bSetUserActive));
			ACTIVATE_TRACE(">>> %22s %20s in %5u Ticket = %8u Visible=%d  Active=%d  ActiveFromAnimRel=%d  bSetUserActive=%d  bOnlyIfUserActive=%d\n",
				"SCHEDULE ACTIVATE", component->m_szName, dwDelay, dwAnimTicket, component->m_bVisible, component->m_eState, bFromAnimRelationship, bSetUserActive, bOnlyIfUserActive);
			sAddDelayedActivateTicket(component, dwAnimTicket);
		}
		else
		{
			ACTIVATE_TRACE(">>> %22s %20s %26s Visible=%d  Active=%d  ActiveFromAnimRel=%d  bSetUserActive=%d  bOnlyIfUserActive=%d\n",
			   "ACTIVATING", component->m_szName, "", component->m_bVisible, component->m_eState, bFromAnimRelationship, bSetUserActive, bOnlyIfUserActive);
			LPARAM lParam = (bImmediate ? UIACTIVATEFLAG_IMMEDIATE : 0);
			UI_MSG_RETVAL eResult = UIComponentHandleUIMessage(component, UIMSG_ACTIVATE, 0, lParam, FALSE);
			if (!ResultIsHandled(eResult))
			{
				return FALSE;
			}
		}

		//dwOpenAnimTime = MAX(component->m_dwAnimDuration, dwOpenAnimTime);
	}

	if (bSetUserActive)
		component->m_bUserActive = bActivate;

	if (pdwTotalAnimTime)
	{
		*pdwTotalAnimTime += dwOpenAnimTime + dwCloseAnimTime;
	}

	return TRUE;
}

BOOL UIComponentActivate(
	UI_COMPONENT_ENUM eComponent,
	BOOL bActivate,
	DWORD dwDelay /*= 0*/,
	DWORD * pdwTotalAnimTime /*= NULL*/,
	BOOL bFromAnimRelationship /*= FALSE*/,
	BOOL bOnlyIfUserActive /*= FALSE*/,
	BOOL bSetUserActive /*= TRUE*/,
	BOOL bImmediate /*= FALSE*/)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(eComponent);
	if (!pComp)
		return FALSE;

	return UIComponentActivate(pComp, bActivate, dwDelay, pdwTotalAnimTime, bFromAnimRelationship, bOnlyIfUserActive, bSetUserActive, bImmediate);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentOnActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (UIComponentGetActive(component))
	{
		return UIMSG_RET_HANDLED;
	}

	//if (!bWindowshade)
	{
		UIComponentSetVisible(component, TRUE);
	}

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	if (component->m_bFadesIn || component->m_Position != component->m_ActivePos)
	{
		if (component->m_bFadesIn)
		{
			component->m_fFading = 1.0f;
		}
		UIComponentSetAnimTime(component, component->m_InactivePos, component->m_ActivePos, wParam ? wParam : component->m_dwAnimDuration, component->m_bFadesIn);
		//component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentAnimate, CEVENT_DATA((DWORD_PTR)component, UISTATE_ACTIVATING, bWindowshade));
		UIComponentAnimate(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)component, UISTATE_ACTIVATING, lParam), 0);
	}
	else
	{
		UIComponentSetActive(component, TRUE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentOnInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (component->m_eState == UISTATE_INACTIVE ||
		(component->m_eState == UISTATE_INACTIVATING &&
		 ((lParam & UIACTIVATEFLAG_IMMEDIATE) == 0)))
	{
		return UIMSG_RET_HANDLED;
	}

	//UIComponentSetActive(component, FALSE);	//BSP 3/19/07 - I took this out because the state is immediately changed again
												//  on both the animate or the set visible and could lead to double post-inactivate calls.
												//  If this was propping up some other functionality, we may have to take another look at
												//  the active states of components.

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}
	if (component->m_bFadesOut || component->m_InactivePos != component->m_Position)
	{
		if (component->m_bFadesOut)
		{
			component->m_fFading = 0.001f;
		}
		UIComponentSetAnimTime(component, component->m_ActivePos, component->m_InactivePos, wParam ? wParam : component->m_dwAnimDuration, component->m_bFadesOut);
		//component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentAnimate, CEVENT_DATA((DWORD_PTR)component, UISTATE_INACTIVATING, bWindowshade ));
		UIComponentAnimate(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)component, UISTATE_INACTIVATING, lParam), 0);
	}
	else
	{
		UIComponentSetVisible(component, FALSE);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentWindowShadeOpen(
	UI_COMPONENT *component)
{
	//UIComponentOnActivate(component, UIMSG_ACTIVATE, 0, 1);
	UIComponentHandleUIMessage(component, UIMSG_ACTIVATE, 0, UIACTIVATEFLAG_WINDOWSHADE, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentWindowShadeClosed(
	UI_COMPONENT *component)
{
//	UIComponentOnInactivate(component, UIMSG_INACTIVATE, 0, 1);
	UIComponentHandleUIMessage(component, UIMSG_INACTIVATE, 0, UIACTIVATEFLAG_WINDOWSHADE, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ROTATION_DISTANCE	400.0f
UI_MSG_RETVAL UIModelPanelOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PANELEX *panel = UICastToPanel(component);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNITID idCursorUnit = UIGetCursorUnit();
	if (idCursorUnit != INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (pFocusUnit)
	{
		ASSERT(pFocusUnit->pUnitData);
		if (pFocusUnit->pUnitData &&
			UnitDataTestFlag(pFocusUnit->pUnitData, UNIT_DATA_FLAG_NOSPIN))
		{
			return UIMSG_RET_HANDLED;
		}
	}

	float x, y;
	UIGetCursorPosition(&x, &y);

	panel->m_bMouseDown = TRUE;

	float fDelta = ROTATION_DISTANCE * (panel->m_fModelRotation / TWOxPI);
	panel->m_posMouseDown.m_fX = x + fDelta;

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModelPanelOnLButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PANELEX *panel = UICastToPanel(component);

	panel->m_bMouseDown = FALSE;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModelPanelOnMouseMove(
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

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	ASSERT(pFocusUnit);
	if(pFocusUnit)
	{
		ASSERT(pFocusUnit->pUnitData);
		if(pFocusUnit->pUnitData && UnitDataTestFlag(pFocusUnit->pUnitData,UNIT_DATA_FLAG_NOSPIN))
		{
			return UIMSG_RET_HANDLED;
		}
	}
	float x, y;
	UIGetCursorPosition(&x, &y);

	float fDelta = panel->m_posMouseDown.m_fX - x;
	panel->m_fModelRotation = (fDelta / ROTATION_DISTANCE) * TWOxPI;

	if (fDelta != 0.0f)
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sModelPanelRotate(
							  GAME* game,
							  const CEVENT_DATA& data,
							  DWORD)
{


	UI_COMPONENT* component = UIComponentGetById( (int)data.m_Data4 );
	int nDelta = data.m_Data3 ? -15 : 15;
	UI_BUTTONEX* pButton = UICastToButton(component);
	ASSERT_RETURN( pButton );
	BOOL bIsDown = UIButtonGetDown(pButton);
	if( bIsDown )
	{
		UI_PANELEX *panel = UICastToPanel(component->m_pParent );

		panel->m_fModelRotation += (nDelta / ROTATION_DISTANCE) * TWOxPI;

		if (nDelta != 0.0f)
			UIComponentHandleUIMessage(panel, UIMSG_PAINT, 0, 0);

		// schedule another
		CSchedulerRegisterEvent(
			AppCommonGetCurTime() + MSECS_PER_SEC/30,
			sModelPanelRotate,
			data);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModelPanelRotate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}



	int nDelta = component->m_dwParam ? -30 : 30 ;

	UI_BUTTONEX* pButton = UICastToButton(component);
	if( pButton )
	{
		UIButtonSetDown(pButton, TRUE );
	}

	// schedule another
	CSchedulerRegisterEvent(
		AppCommonGetCurTime() + MSECS_PER_SEC/30,
		sModelPanelRotate,
		CEVENT_DATA((DWORD_PTR) component, msg, component->m_dwParam, component->m_idComponent));

	UI_PANELEX *panel = UICastToPanel(component->m_pParent );
	panel->m_fModelRotation += (nDelta / ROTATION_DISTANCE) * TWOxPI;

	if (nDelta != 0.0f)
		UIComponentHandleUIMessage(panel, UIMSG_PAINT, 0, 0);


	return UIMSG_RET_NOT_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUISpin(
			 GAME* game,
			 const CEVENT_DATA& data,
			 DWORD)
{
	UI_COMPONENT * component = (UI_COMPONENT*)data.m_Data1;

	if (!UIComponentGetVisible(component))
	{
		return;
	}

	BOOL spin = TRUE;
	/*
	ASSERT(component->m_pParent)
	if(component->m_pParent)
	{

		UNIT *pFocusUnit = UIComponentGetFocusUnit(component->m_pParent);
		ASSERT(pFocusUnit);
		if(pFocusUnit)
		{
			ASSERT(pFocusUnit->pUnitData);
			if(pFocusUnit->pUnitData && UnitDataTestFlag(pFocusUnit->pUnitData,UNIT_DATA_FLAG_NOSPIN))
			{
				spin = FALSE;
			}
		}
	}
	*/
	if (UIGetModUnit() != INVALID_ID)
	{

		UNIT* item = UnitGetById(AppGetCltGame(), UIGetModUnit());
		if (item)
		{
			if(item->pUnitData && UnitDataTestFlag(item->pUnitData, UNIT_DATA_FLAG_NOSPIN))
			{
				spin = FALSE;
			}
		}
	}


	UI_PANELEX *panel = UICastToPanel(component);
	if (!panel->m_bMouseDown && spin)
	{
		panel->m_fModelRotation += 0.015f;
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}

	// schedule another
	CSchedulerRegisterEvent(
		AppCommonGetCurTime() + MSECS_PER_SEC/30,
		sUISpin,
		data);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModelPanelOnPostVisible(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{

	CSchedulerRegisterEvent(
		AppCommonGetCurTime() + MSECS_PER_SEC/30,
		sUISpin,
		CEVENT_DATA((DWORD_PTR) component, msg, wParam, lParam));

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModelPanelWorldCameraOnMouseMove(
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

	float x, y;
	UIGetCursorPosition(&x, &y);

	float fDelta = panel->m_posMouseDown.m_fX - x;
	fDelta *= -2.0f;

	if (fDelta != 0.0f)
		PlayerMouseRotate(AppGetCltGame(), fDelta);

	panel->m_posMouseDown.m_fX = x;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIModelPanelOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_PANELEX *panel = UICastToPanel(component);
	ASSERT_RETVAL(panel, UIMSG_RET_NOT_HANDLED);

	panel->m_fModelRotation = 0.0f;
	panel->m_posMouseDown.m_fX = 0.0f;
	panel->m_bMouseDown = FALSE;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPaperDollPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (pFocusUnit)
	{
		UI_RECT tScreenRect;
		//UI_POSITION tScreenPos;
		//UIComponentGetAbsoluteLogPosition(component, &tScreenPos);
		tScreenRect.m_fX1 = 0;
		tScreenRect.m_fY1 = 0;
		tScreenRect.m_fX2 = component->m_fWidth * UIGetScreenToLogRatioX( component->m_bNoScale );
		tScreenRect.m_fY2 = component->m_fHeight * UIGetScreenToLogRatioY( component->m_bNoScale );;
		UI_PANELEX *panel = UICastToPanel(component);

		// make sure the unit's paperdoll model has been initialized
		c_UnitCreatePaperdollModel(pFocusUnit, APPEARANCE_NEW_DONT_DO_INIT_ANIMATION | APPEARANCE_NEW_FORCE_ANIMATE);

		int nModelID = c_UnitGetPaperdollModelId( pFocusUnit );
		int nAppearanceDefID = e_ModelGetAppearanceDefinition( nModelID );
		if (nModelID != INVALID_ID && nAppearanceDefID != INVALID_ID)
		{
			UIComponentAddModelElement(component, nAppearanceDefID, nModelID, tScreenRect, panel->m_fModelRotation, panel->m_nModelCam);
		}
		else
		{
			UIComponentAddTextElement(component, NULL, UIComponentGetFont(component), 0, L"NO\nMODEL", UI_POSITION(), GFXCOLOR_WHITE, &tScreenRect, UIALIGN_CENTER );
		}
		//UIComponentAddBoxElement(component, 0, 0, component->m_fWidth, component->m_fHeight, NULL, UI_HILITE_GREEN, 254);

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUICheckTradeDistance(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{

	UI_COMPONENT_ENUM eComp = (UI_COMPONENT_ENUM)data.m_Data1;
	float flMaxDistanceSq = (float)data.m_Data2;
	UI_COMPONENT *pComp = UIComponentGetByEnum( eComp );
	ASSERTX_RETURN( pComp, "Unable to find trade component when checking distance" );

	UNIT *pPlayer = UIGetControlUnit();
	UNIT *pTradingWith = UIComponentGetFocusUnit( pComp );

	if (eComp == UICOMP_STASH_PANEL)
	{
		UNITID idStash = UnitGetStat(pPlayer, STATS_LAST_STASH_UNIT_ID);
		pTradingWith = UnitGetById(game, idStash);
	}

	if (pTradingWith == NULL ||
		UIComponentGetActive( pComp ) == FALSE ||
		UnitsGetDistanceSquared( pPlayer, pTradingWith ) > flMaxDistanceSq )
	{
		UIComponentActivate(pComp, FALSE);
//		InputExecuteKeyCommand(CMD_UI_QUICK_EXIT);
		return;
	}

	// schedule another
	CSchedulerRegisterEvent(
		AppCommonGetCurTime() + MSECS_PER_SEC,
		sUICheckTradeDistance,
		data);

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUICheckTradeDistanceMythos(
						   GAME* game,
						   const CEVENT_DATA& data,
						   DWORD)
{

	EPaneMenu Pane = (EPaneMenu)data.m_Data1;
	float flMaxDistanceSq = (float)data.m_Data2;

	UNIT *pPlayer = UIGetControlUnit();

	UNIT* pTradingWith = pPlayer ? TradeGetTradingWith( pPlayer ) : NULL;

	if (pTradingWith == NULL ||
		UIPaneVisible( Pane ) == FALSE ||
		UnitsGetDistanceSquared( pPlayer, pTradingWith ) > flMaxDistanceSq )
	{
		UIHidePaneMenu( Pane );
		return;
	}

	// schedule another
	CSchedulerRegisterEvent(
		AppCommonGetCurTime() + MSECS_PER_SEC,
		sUICheckTradeDistanceMythos,
		data);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateRewardMessage(
	int nNumRewardTakes,
	int nOfferDefinition)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_REWARD_PANEL);
    UI_COMPONENT *pLabel = UIComponentFindChildByName(pPanel, "reward message");
    if (pLabel)
    {
		const int MAX_MESSAGE = 512;
		WCHAR uszMessage[ MAX_MESSAGE ];
		BOOL bFoundMessage = FALSE;

		// if we are only allowed to have a limited number of the items being
		// offered to us ... those instructions take priority over anything else
		if (nNumRewardTakes != REWARD_TAKE_ALL)
		{
			if (nNumRewardTakes == 1)
			{
				PStrCopy( uszMessage, GlobalStringGet( GS_REWARD_INSTRUCTIONS_TAKE_ONE ), MAX_MESSAGE );
			}
			else
			{
				PStrPrintf( uszMessage, MAX_MESSAGE, GlobalStringGet( GS_REWARD_INSTRUCTIONS_TAKE_SOME ), nNumRewardTakes );
			}
			bFoundMessage = TRUE;
		}

	    // use string in offer (if any)
	    if (bFoundMessage == FALSE && nOfferDefinition != INVALID_LINK)
	    {
		    const OFFER_DEFINITION *pOfferDefinition = OfferGetDefinition( nOfferDefinition );
		    if (pOfferDefinition)
		    {
			    if (pOfferDefinition->nOfferStringKey != INVALID_LINK)
			    {
					PStrCopy( uszMessage, StringTableGetStringByIndex( pOfferDefinition->nOfferStringKey ), MAX_MESSAGE );
					bFoundMessage = TRUE;
			    }
		    }
	    }

	    // if no string found, use the generic take items one
	    if (bFoundMessage == FALSE)
	    {
			PStrCopy( uszMessage, GlobalStringGet( GS_REWARD_INSTRUCTIONS ), MAX_MESSAGE );
			bFoundMessage = TRUE;
	    }

	    // set into label
	    ASSERTX_RETURN( bFoundMessage, "Expected offer message string" );
	    UILabelSetText( pLabel, uszMessage );

    }

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetRewardInfo(
	GAME *game,
	UNITID idFocus,
	int nOfferDefinition,
	int nNumRewardTakes)
{
    UI_COMPONENT *pPanel = UIComponentGetByEnum(UICOMP_REWARD_PANEL);
    if (pPanel)
    {

		// update the message
		UIUpdateRewardMessage( nNumRewardTakes, nOfferDefinition );

		// set portrait
		UI_COMPONENT *pPortrait = UIComponentFindChildByName(pPanel, "reward dialog portrait");
		if (pPortrait)
		{
			UNIT *pPlayer = GameGetControlUnit(AppGetCltGame());
			if (pPlayer && UnitGetId(pPlayer) == idFocus)
			{
				idFocus = INVALID_ID;
			}
			UNIT *pFocus = UnitGetById(AppGetCltGame(), idFocus);
			if (!pFocus || UnitDataTestFlag(pFocus->pUnitData, UNIT_DATA_FLAG_HIDE_DIALOG_HEAD))
			{
				idFocus = INVALID_ID;
			}
			UIComponentSetFocusUnit(pPortrait, idFocus);
			UIComponentSetVisible(pPortrait, idFocus != INVALID_ID);
		}
    }
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//void UIInventorySlideIn(
//	GAME* game,
//	const CEVENT_DATA& data,
//	DWORD dwData)
//{

	//// get data from globals
	//INVENTORY_STYLE eStyle = g_UI.m_tInventoryGlobals.eStyle;
	//int nOfferDefinition = g_UI.m_tInventoryGlobals.nOfferDefinition;
	//int nNumRewardTakes = g_UI.m_tInventoryGlobals.nNumRewardTakes;

	//UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	//UNITID idStyleFocus = (UNITID)data.m_Data2;
	//UNITID idPortraitFocus = (UNITID)data.m_Data3;
	//if (!component)
	//{
	//	return;
	//}

	//UNIT *pPlayer = UIGetControlUnit();
	//UNITID idPlayer = UnitGetId( pPlayer );
	//UNITID idFocusSecondary = idStyleFocus;
	//UNITID idFocusCenter = idStyleFocus;
	//BOOL bSecondaryForceFocus = FALSE;
	//BOOL bCenterForceFocus = FALSE;
	//
	//DWORD dwMaxAnimDuration = 0;

	//// which component pieces will come up with the inventory panel
	//UI_COMPONENT_ENUM eCompSecondary = UICOMP_INVALID;
	//UI_COMPONENT_ENUM eCompCenter = UICOMP_INVALID;
	//switch (eStyle)
	//{
	//	//----------------------------------------------------------------------------
	//	case STYLE_TRAINER:
	//	{
	//		eCompSecondary = UICOMP_TRAINERINVENTORY;
	//		eCompCenter = UICOMP_MERCHANTWELCOME;
	//		break;
	//	}

	//	//----------------------------------------------------------------------------
	//	case STYLE_MERCHANT:
	//	{
	//		eCompSecondary = UICOMP_MERCHANTINVENTORY;
	//		eCompCenter = UICOMP_MERCHANTWELCOME;
	//		break;
	//	}

	//	//----------------------------------------------------------------------------
	//	case STYLE_TRADE:
	//	{
	//		eCompSecondary = UICOMP_TRADE_PANEL;
	//		eCompCenter = UICOMP_PAPERDOLL;
	//		UNIT *pUnitSecondary = UnitGetById( game, idFocusSecondary );
	//		if (pUnitSecondary)
	//		{
	//			if (UnitIsPlayer( pUnitSecondary ) && pUnitSecondary != pPlayer)
	//			{
	//				// trading with another player, make us the center focus
	//				idFocusCenter = idPlayer;
	//			}
	//		}
	//
	//		break;
	//	}

	//	//----------------------------------------------------------------------------
	//	case STYLE_REWARD:
	//	case STYLE_OFFER:
	//	{
	//		//eCompSecondary = UICOMP_REWARD_PANEL;
	//		idFocusSecondary = idPlayer;  // player is actually the focus for the reward/offer
	//		if (AppIsHellgate())
	//		{
	//			sSetRewardInfo( game, idPortraitFocus, nOfferDefinition, nNumRewardTakes );
	//		}
	//		break;
	//	}

	//	//----------------------------------------------------------------------------
	//	case STYLE_RECIPE:
	//	{
	//		eCompSecondary = UICOMP_RECIPE_LIST_PANEL;
	//		eCompCenter = UICOMP_RECIPE_CURRENT_PANEL;
	//		idFocusCenter = idPlayer;  // player is actually the focus for ingredient panel
	//		break;
	//	}

	//	//----------------------------------------------------------------------------
	//	case STYLE_RECIPE_SINGLE:
	//	{
	//		eCompCenter = UICOMP_RECIPE_CURRENT_PANEL;
	//		idFocusCenter = idPlayer;  // player is actually the focus for ingredient panel
	//		break;
	//	}
	//
	//	//----------------------------------------------------------------------------
	//	case STYLE_STASH:
	//	{
	//		eCompSecondary = UICOMP_STASH_PANEL;
	//		eCompCenter = UICOMP_PAPERDOLL;
	//		break;
	//	}

	//	//----------------------------------------------------------------------------
	//	case STYLE_PET:
	//	{
	//		eCompSecondary = UICOMP_CHARSHEET;
	//		eCompCenter = UICOMP_PAPERDOLL;
	//		bCenterForceFocus = TRUE;
	//		break;
	//	}
	//
	//	//----------------------------------------------------------------------------
	//	case STYLE_ITEM_UPGRADE:
	//	{
	//		eCompCenter = UICOMP_ITEM_UPGRADE_PANEL;
	//		idFocusCenter = idPlayer;  // player is actually the focus for this panel
	//		break;
	//	}
	//
	//	//----------------------------------------------------------------------------
	//	case STYLE_ITEM_AUGMENT:
	//	{
	//		eCompCenter = UICOMP_ITEM_AUGMENT_PANEL;
	//		idFocusCenter = idPlayer;  // player is actually the focus for this panel
	//		break;
	//	}
	//
	//	//----------------------------------------------------------------------------
	//	case STYLE_ITEM_UNMOD:
	//	{
	//		eCompCenter = UICOMP_ITEM_UNMOD_PANEL;
	//		idFocusCenter = idPlayer;  // player is actually the focus for this panel
	//		break;
	//	}
	//
	//	//----------------------------------------------------------------------------
	//	default:
	//	{
	//		if (!AppIsTugboat())
	//		{
	//			eCompSecondary = UICOMP_CHARSHEET;
	//			eCompCenter = UICOMP_PAPERDOLL;
	//			if (idFocusSecondary == INVALID_ID)
	//			{
	//				idFocusSecondary = idPlayer;
	//			}
	//			if (idFocusCenter == INVALID_ID)
	//			{
	//				idFocusCenter = idPlayer;
	//			}
	//
	//		}
	//		break;
	//	}
	//
	//}

	//// bring up the secondary and center components
	//if( eCompSecondary != UICOMP_INVALID )
	//{
	//	UIComponentSetFocusUnitByEnum( eCompSecondary, idFocusSecondary, bSecondaryForceFocus );
	//	UISendComponentMsgWithAnimTime(eCompSecondary, UIMSG_ACTIVATE, STD_ANIM_DURATION_MULT, dwMaxAnimDuration);
	//}
	//if (eCompCenter != UICOMP_INVALID)
	//{
	//	UIComponentSetFocusUnitByEnum( eCompCenter, idFocusCenter, bCenterForceFocus );
	//	if (!AppIsTugboat())
	//	{
	//		UISendComponentMsgWithAnimTime(eCompCenter, UIMSG_ACTIVATE, STD_ANIM_DURATION_MULT, dwMaxAnimDuration);
	//	}
	//}
	//
	//UISendComponentMsgWithAnimTime(UICOMP_INVENTORY,	UIMSG_ACTIVATE, STD_ANIM_DURATION_MULT, dwMaxAnimDuration);

	//UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);		//explicitly re-paint the page (this will refresh statdispl's for example)

	////UI_COMPONENT* pMainScreen = UIComponentGetByEnum(UICOMP_MAINSCREEN);
	////if (!pMainScreen)
	////	return;
	////UIComponentHandleUIMessage(pMainScreen, UIMSG_PAINT, 0, 0);

	//// for trade situations, register a monitor to cancel trading if people wander away somehow
	//switch (eStyle)
	//{
	//
	//	//----------------------------------------------------------------------------
	//	case STYLE_MERCHANT:
	//	case STYLE_TRADE:
	//	case STYLE_STASH:
	//	case STYLE_TRAINER:
	//	{
	//
	//		// what is the max distance
	//		float flMaxDistanceSq = UNIT_INTERACT_DISTANCE_SQUARED;
	//		if (eStyle == STYLE_TRADE)
	//		{
	//			flMaxDistanceSq = TRADE_DISTANCE_SQ;
	//		}
	//
	//		CEVENT_DATA tCEventData;
	//		tCEventData.m_Data1 = eCompSecondary;
	//		tCEventData.m_Data2 = (DWORD_PTR)flMaxDistanceSq;
	//
	//		CSchedulerRegisterEvent(
	//			AppCommonGetCurTime() + MSECS_PER_SEC,
	//			sUICheckTradeDistance,
	//			tCEventData );
	//
	//		break;
	//
	//	}
	//
	//}
	//
//}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDelayedActivate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD dwTicket)
{
	UI_COMPONENT* pComponent = (UI_COMPONENT*)data.m_Data1;
	int nMsg = (int)data.m_Data2;
	BOOL bFromAnimRelationship = (BOOL)data.m_Data3;
	BOOL bSetUserActive = (BOOL)data.m_Data4;
	if (nMsg < 0)
	{
		nMsg = UIMSG_ACTIVATE;
	}
	if (pComponent)
	{
		UIComponentActivate(pComponent, nMsg == UIMSG_ACTIVATE, 0, NULL, bFromAnimRelationship, FALSE, bSetUserActive);
	}

	sRemoveDelayedActivateTicket(dwTicket);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDelayedVisible(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* pComponent = (UI_COMPONENT*)data.m_Data1;
	BOOL bVisible = (BOOL)data.m_Data2;
	UIComponentSetVisible(pComponent, bVisible);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDelayedImmediateActivate(		// what a confusing name... but yet that's what it does
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* pComponent = (UI_COMPONENT*)data.m_Data1;
	BOOL bActivate = (BOOL)data.m_Data2;
	UIComponentActivate(pComponent, bActivate, 0, NULL, FALSE, FALSE, TRUE, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//BOOL UISwitchComponents(
//	UI_COMPONENT_ENUM peComponentFrom[ NUM_COMPONENTS_TO_SLIDE ],
//	UI_COMPONENT_ENUM peComponentTo[ NUM_COMPONENTS_TO_SLIDE ])
//{
//	//clear tooltips
//	UISetHoverUnit(INVALID_ID, INVALID_ID);
//
//	// Another frickin' special case
//	UI_COMPONENT *pMainScreen = UIComponentGetByEnum(UICOMP_MAINSCREEN);
//	ASSERT_RETFALSE(pMainScreen)
//	if (pMainScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//		CSchedulerIsValidTicket(pMainScreen->m_tMainAnimInfo.m_dwAnimTicket))
//	{
//		// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//		return FALSE;
//	}
//
//	UI_COMPONENT *pInventoryScreen = UIComponentGetByEnum(UICOMP_INVENTORYSCREEN);
//	ASSERT_RETFALSE(pInventoryScreen)
//	if (pInventoryScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//		CSchedulerIsValidTicket(pInventoryScreen->m_tMainAnimInfo.m_dwAnimTicket))
//	{
//		// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//		return FALSE;
//	}
//
//	if (AppIsTugboat())
//	{
//		UI_COMPONENT *pCharacterScreen = UIComponentGetByEnum(UICOMP_CHARACTERSCREEN);
//		ASSERT_RETFALSE(pCharacterScreen)
//		if (pCharacterScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//			CSchedulerIsValidTicket(pCharacterScreen->m_tMainAnimInfo.m_dwAnimTicket))
//		{
//			// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//			return FALSE;
//		}
//
//		UI_COMPONENT *pSkillsScreen = UIComponentGetByEnum(UICOMP_SKILLSSCREEN);
//		ASSERT_RETFALSE(pSkillsScreen)
//		if (pSkillsScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//			CSchedulerIsValidTicket(pSkillsScreen->m_tMainAnimInfo.m_dwAnimTicket))
//		{
//			// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//			return FALSE;
//		}
//
//
//
//		UI_COMPONENT *pWorldMapScreen = UIComponentGetByEnum(UICOMP_WORLDMAPSCREEN);
//		ASSERT_RETFALSE(pWorldMapScreen)
//		if (pWorldMapScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//			CSchedulerIsValidTicket(pWorldMapScreen->m_tMainAnimInfo.m_dwAnimTicket))
//		{
//			// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//			return FALSE;
//		}
//
//		UI_COMPONENT *pTaskScreen = UIComponentGetByEnum(UICOMP_TASKSCREEN);
//		ASSERT_RETFALSE(pTaskScreen)
//		if (pCharacterScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//			CSchedulerIsValidTicket(pTaskScreen->m_tMainAnimInfo.m_dwAnimTicket))
//		{
//			// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//			return FALSE;
//		}
//
//		UI_COMPONENT *pRecipeScreen = UIComponentGetByEnum(UICOMP_RECIPE_CURRENT_PANEL);
//		ASSERT_RETFALSE(pRecipeScreen)
//		if (pRecipeScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//			CSchedulerIsValidTicket(pRecipeScreen->m_tMainAnimInfo.m_dwAnimTicket))
//		{
//			// If the main screen already has something scheduled (it's sliding in or out), don't allow a switch
//			return FALSE;
//		}
//	}
//	UI_COMPONENT* skill_map = UIComponentGetByEnum(UICOMP_SKILLMAP);
//	ASSERT_RETFALSE(skill_map)
//	if (skill_map->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID &&
//		CSchedulerIsValidTicket(skill_map->m_tMainAnimInfo.m_dwAnimTicket))
//	{
//		// If the skill page already has something scheduled (it's sliding in or out), don't allow a switch
//		return FALSE;
//	}
//
//	DWORD dwAnimTime = 0;
//
//	// Don't do this if the gun mod screen is open or
//	//  if the stash screen is open or
//	//  if the trade panel is open
//	if ((UIIsGunModScreenUp() && peComponentFrom[ 0 ] != UICOMP_GUNMODSCREEN) ||
//		UIComponentGetVisibleByEnum(UICOMP_STASH_PANEL) ||
//		UIComponentGetVisibleByEnum(UICOMP_TRADE_PANEL))
//		return FALSE;
//
//	trace("UISwitchComponents\n");
//
//	for ( int i = 0; i < NUM_COMPONENTS_TO_SLIDE; i++ )
//	{
//		if ( peComponentFrom[ i ] != INVALID_ID )
//			UISendComponentMsgWithAnimTime(peComponentFrom[ i ], UIMSG_INACTIVATE, STD_ANIM_DURATION_MULT, dwAnimTime);
//	}
//
//	for ( int i = 0; i < NUM_COMPONENTS_TO_SLIDE; i++ )
//	{
//		if ( peComponentTo[ i ] == INVALID_ID )
//			continue;
//
//		UI_COMPONENT* pComponentTo = UIComponentGetByEnum(peComponentTo[ i ]);
//		if (pComponentTo)
//		{
//			UIComponentHandleUIMessage(pComponentTo, UIMSG_PAINT, 0, 0);
//
//			if (pComponentTo->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
//			{
//				CSchedulerCancelEvent(pComponentTo->m_tMainAnimInfo.m_dwAnimTicket);
//			}
//			pComponentTo->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwAnimTime, UIDelayedActivate, CEVENT_DATA((DWORD_PTR)pComponentTo, UIMSG_ACTIVATE));
//		}
//	}
//
//	return TRUE;
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//BOOL UISwitchComponents(
//	UI_COMPONENT_ENUM eComponentFrom,
//	UI_COMPONENT_ENUM eComponentTo)
//{
//	UI_COMPONENT_ENUM peComponentFrom[ NUM_COMPONENTS_TO_SLIDE ];
//	UI_COMPONENT_ENUM peComponentTo[ NUM_COMPONENTS_TO_SLIDE ];
//	for ( int i = 1; i < NUM_COMPONENTS_TO_SLIDE; i++ )
//	{
//		peComponentFrom[ i ] = (UI_COMPONENT_ENUM) INVALID_ID;
//		peComponentTo  [ i ] = (UI_COMPONENT_ENUM) INVALID_ID;
//	}
//	peComponentFrom[ 0 ] = eComponentFrom;
//	peComponentTo  [ 0 ] = eComponentTo;
//	return UISwitchComponents( peComponentFrom, peComponentTo );
//}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHealthBarOnStatChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UNIT* pUnit = UIComponentGetFocusUnit(component);

	if (pUnit)
	{
		int nShieldBufferMax = UnitGetStat(pUnit, STATS_SHIELD_BUFFER_MAX);
		int nArmor = UnitGetStat(pUnit, STATS_ARMOR);


		UI_COMPONENT *pChilde = component->m_pFirstChild;
		while (pChilde)
		{
			if (AppIsTugboat())
			{
				UIComponentSetVisible(pChilde, PStrICmp(pChilde->m_szName, "health bar single")==0);
			}
			else
			{
				if (PStrICmp(pChilde->m_szName, "health bar small")==0)
				{
					UIComponentSetVisible(pChilde, nShieldBufferMax > 0 || nArmor > 0);
				}
				if (PStrICmp(pChilde->m_szName, "health bar big")==0)
				{
					UIComponentSetVisible(pChilde, nShieldBufferMax == 0 && nArmor == 0);
				}

				if (PStrICmp(pChilde->m_szName, "shield bar long")==0)
				{
					UIComponentSetVisible(pChilde, nShieldBufferMax > 0 && nArmor == 0);
				}
				if (PStrICmp(pChilde->m_szName, "shield bar short")==0)
				{
					UIComponentSetVisible(pChilde, nShieldBufferMax > 0 && nArmor > 0);
				}
				if (PStrICmp(pChilde->m_szName, "armor bar long")==0)
				{
					UIComponentSetVisible(pChilde, nShieldBufferMax == 0 && nArmor > 0);
				}
				if (PStrICmp(pChilde->m_szName, "armor bar short")==0)
				{
					UIComponentSetVisible(pChilde, nShieldBufferMax > 0 && nArmor > 0);
				}
			}

			pChilde = pChilde->m_pNextSibling;
		}
	}

	if (msg != UIMSG_PAINT)
	{
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillIconDraw(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UISkillIconAnimate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}
	UISkillIconDraw(component, UIMSG_SETCONTROLSTAT, 0, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillIconDraw(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);

	UNIT* unit = UIComponentGetFocusUnit(component);
	if (!unit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int skill = INVALID_ID;
	if (component->m_dwParam == 0)
	{
		skill = UnitGetStat(unit, STATS_SKILL_LEFT);
	}
	else
	{
		skill = UnitGetStat(unit, STATS_SKILL_RIGHT);
	}

	if (skill == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	const SKILL_DATA* skill_data = SkillGetData(UnitGetGame(unit), skill);
	if (!skill_data)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	WORD wStat = (WORD)STAT_GET_STAT(lParam);

    const SKILL_DATA * pSkillDataForLargeIcon = skill_data;
	if (!AppIsTugboat())
	{
	    if ((msg == UIMSG_SETCONTROLSTAT && (wStat == STATS_SKILL_LEFT || wStat == STATS_SKILL_RIGHT || wStat == STATS_SKILL_COOLING || wStat == STATS_SKILL_GROUP_COOLING)) ||
		    msg == UIMSG_INVENTORYCHANGE ||
		    !component->m_pFrame)
	    {
		    if ( SkillDataTestFlag( pSkillDataForLargeIcon, SKILL_FLAG_USES_WEAPON_ICON ) )
		    {
			    UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
			    UnitGetWeapons( unit, skill, pWeapons, FALSE );
			    int nWeaponSkill = ItemGetPrimarySkill( pWeapons[ 0 ] );
			    if ( nWeaponSkill != INVALID_ID && !pWeapons[ 1 ] )
			    {
				    const SKILL_DATA * pSkillDataForWeapon = SkillGetData( NULL, nWeaponSkill );
				    if ( pSkillDataForWeapon && pSkillDataForWeapon->szLargeIcon[ 0 ] != 0 )
					    pSkillDataForLargeIcon = pSkillDataForWeapon;
			    }
		    }
		    UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, pSkillDataForLargeIcon->szLargeIcon);
		    if (frame && component->m_pFrame != frame)
		    {
			    component->m_dwColor = GetFontColor(pSkillDataForLargeIcon->nIconColor);
			    component->m_pFrame = frame;
		    }
	    }
	}
	else
    {
	    if ((msg == UIMSG_SETCONTROLSTAT && (wStat == STATS_SKILL_LEFT || wStat == STATS_SKILL_RIGHT || wStat == STATS_SKILL_COOLING || wStat == STATS_SKILL_GROUP_COOLING)) ||
		    msg == UIMSG_INVENTORYCHANGE ||
		    !component->m_pFrame)
	    {
		    UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, skill_data->szLargeIcon);
		    if (frame && component->m_pFrame != frame)
		    {
			    component->m_dwColor = GetFontColor(skill_data->nIconColor);
			    component->m_pFrame = frame;
		    }
	    }
		else
		{
			 // if this skill has no icon, clear the panel!
			 UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, skill_data->szLargeIcon);
			 if( !frame )
			 {
				if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
				{
					CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
				}

				component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UISkillIconAnimate, CEVENT_DATA((DWORD_PTR)component));
				return UIMSG_RET_HANDLED;
			 }
		}

    }

	if (!component->m_pFrame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float fScale = 1.0f;
	BOOL bIconIsRed = FALSE;
	int nTicksRemaining = 0;
	c_SkillGetIconInfo( unit, skill, bIconIsRed, fScale, nTicksRemaining, TRUE );

	if (AppIsTugboat())
	{
		const SKILL_DATA * pSkillData = SkillGetData( NULL, skill );
		component->m_dwColor = bIconIsRed ? GFXCOLOR_RED : GetFontColor(skill_data->nIconColor);

		BOOL bDrawGrey = SkillGetLevel( unit, skill ) == 0 && SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE );
		if ( bDrawGrey )
			component->m_dwColor = GFXCOLOR_DKGRAY;

		DWORD dwColor = UI_MAKE_COLOR( 255, component->m_dwColor );
		// TRAVIS: HACK!
		// for now, no cooldown scale.
		fScale = 1.0f;
		// center the icon
		float fScaleDiff = component->m_fWidth / component->m_pFrame->m_fWidth;
		float fScaleDiffY = component->m_fHeight / component->m_pFrame->m_fHeight;
		UI_POSITION pos(0,0);


		UIComponentAddElement(component, texture, component->m_pFrame, pos, dwColor, NULL, FALSE, fScale * fScaleDiff, fScale * fScaleDiffY);
	}
	else
	{
		const SKILL_DATA * pSkillData = SkillGetData( NULL, skill );
		BOOL bDrawGrey = SkillGetLevel( unit, skill ) == 0 && SkillDataTestFlag( pSkillData, SKILL_FLAG_LEARNABLE );
		if ( bDrawGrey )
			component->m_dwColor = GFXCOLOR_DKGRAY;
		else if ( bIconIsRed )
			component->m_dwColor = GFXCOLOR_RED;
		else
			component->m_dwColor = GetFontColor(pSkillDataForLargeIcon->nIconColor);

		DWORD dwColor = component->m_dwColor;

		// center the icon
		UI_POSITION pos(FLOOR(component->m_fWidth / 2.0f) + 0.5f, FLOOR(component->m_fHeight / 2.0f) + 0.5f);
		UIComponentAddElement(component, texture, component->m_pFrame, pos, dwColor, NULL, FALSE, fScale, fScale, &UI_SIZE(component->m_fWidth, component->m_fHeight));
	}

	if (nTicksRemaining > 0)
	{
		WCHAR buf[32];
		PIntToStr(buf, 32, (nTicksRemaining + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND);
		UIComponentAddTextElement(component, NULL, UIComponentGetFont(component), UIComponentGetFontSize(component), buf, UI_POSITION(), GFXCOLOR_WHITE, NULL, UIALIGN_BOTTOM);
	}

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	if (fScale < 1.0f ||
		nTicksRemaining > 0 )
	{
		component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UISkillIconAnimate, CEVENT_DATA((DWORD_PTR)component));
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillSphereOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	float x = (float)wParam;
	float y = (float)lParam;

	if (!UIComponentGetVisible(component) || !UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pPlayer = UIComponentGetFocusUnit(component);

	if (!pPlayer)
		return UIMSG_RET_NOT_HANDLED;

	int nSkillID = INVALID_ID;
	if (component->m_dwParam == 0)
	{
		nSkillID = UnitGetStat(pPlayer, STATS_SKILL_LEFT);
	}
	else
	{
		nSkillID = UnitGetStat(pPlayer, STATS_SKILL_RIGHT);
	}

	if (nSkillID == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UISetHoverTextSkill(component, UIGetControlUnit(), nSkillID);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillSphereOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UISetHoverUnit(INVALID_ID, INVALID_ID);
	UIClearHoverText();

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillCooldownDraw(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UISkillCooldownAnimate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}
	UISkillCooldownDraw(component, UIMSG_SETCONTROLSTAT, 0, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISkillCooldownDraw(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT* unit = UIComponentGetFocusUnit(component);
	if (!unit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UI_TEXTURE_FRAME* frame = component->m_pFrame;
	if (!frame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nSkill = INVALID_ID;
	if (component->m_dwParam == 0)
	{
		nSkill = UnitGetStat(unit, STATS_SKILL_LEFT);
	}
	else
	{
		nSkill = UnitGetStat(unit, STATS_SKILL_RIGHT);
	}

	UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
	UnitGetWeapons( unit, nSkill, pWeapons, FALSE );
	UNIT * pWeapon = pWeapons[ 0 ];

	if (!pWeapon)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	BOOL bIconIsRed = FALSE;
	float fCooldownScale = 1.0f;
	int nTicksRemaining = 0;
	c_SkillGetIconInfo( unit, nSkill, bIconIsRed, fCooldownScale, nTicksRemaining, FALSE );

	if ( fCooldownScale == 1.0f )
	{
		if (component->m_dwData == 0)
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		component->m_dwData = 0;
		UIComponentRemoveAllElements(component);
		UIComponentAddElement(component, texture, component->m_pFrame, UI_POSITION());
		return UIMSG_RET_HANDLED;
	}

	component->m_dwData = 1;
	UIComponentRemoveAllElements(component);

	// float pct = (float)(nCurTime-nStartTime)/(float)(nEndTime-nStartTime);
	// float width = component->m_fWidth;
	// float height = component->m_fHeight;

	//struct WEDGE_DEF
	//{
	//	float pct;		// pct of circle complete
	//	float fU1, fV1;
	//	float fU2, fV2;
	//	float fU3, fV3;
	//	BOOL bHoldX;	// which axis to hold constant for partial wedge
	//	float fOffset;	// offset to compute other axis
	//	float fMult;	// multiplier for lookup
	//};
	//static const WEDGE_DEF tCCWWedges[8] =
	//{
	//	{ 0.125, 0.525f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0, 0.5f, -1.0f },
	//	{ 0.250, 0.0f, 0.0f, 0.5f, 0.5f, 0.0f, 0.5f, 1, 0.0f,  1.0f },
	//	{ 0.375, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1, 0.5f,  1.0f },
	//	{ 0.500, 0.0f, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0, 0.0f,  1.0f },
	//	{ 0.625, 0.5f, 1.0f, 0.5f, 0.5f, 1.0f, 1.0f, 0, 0.5f,  1.0f },
	//	{ 0.750, 1.0f, 1.0f, 0.5f, 0.5f, 1.0f, 0.5f, 1, 1.0f, -1.0f },
	//	{ 0.875, 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1, 0.5f, -1.0f },
	//	{ 1.000, 1.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0, 1.0f, -1.0f },
	//};
	//static const WEDGE_DEF tCWWedges[8] =
	//{
	//	{ 0.125, 0.5f, 0.5f, 0.475f, 0.0f, 1.0f, 0.0f, 0, 0.5f, 1.0f },
	//	{ 0.250, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0.5f, 1, 0.0f,  1.0f },
	//	{ 0.375, 0.5f, 0.5f, 1.0f, 0.5f, 1.0f, 1.0f, 1, 0.5f,  1.0f },
	//	{ 0.500, 0.5f, 0.5f, 1.0f, 1.0f, 0.5f, 1.0f, 0, 1.0f, -1.0f },
	//	{ 0.625, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 0, 0.5f, -1.0f },
	//	{ 0.750, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.5f, 1, 1.0f, -1.0f },
	//	{ 0.875, 0.5f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 1, 0.5f, -1.0f },
	//	{ 1.000, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0, 0.0f,  1.0f },
	//};


	// !!!! TODO: FIX ME!

	//const WEDGE_DEF* tWedges = component->m_dwParam == 0 ? tCCWWedges : tCWWedges;
	//float fU = frame->m_fU2 - frame->m_fU1;
	//float fV = frame->m_fV2 - frame->m_fV1;

	//for (int ii = 0; ii < 8; ii++)
	//{
	//	if (pct >= tWedges[ii].pct)
	//	{
	//		UI_TEXTURETRI tri;
	//		tri.x1 = tWedges[ii].fU1 * width;
	//		tri.y1 = tWedges[ii].fV1 * height;
	//		tri.x2 = tWedges[ii].fU2 * width;
	//		tri.y2 = tWedges[ii].fV2 * height;
	//		tri.x3 = tWedges[ii].fU3 * width;
	//		tri.y3 = tWedges[ii].fV3 * height;
	//		tri.u1 = frame->m_fU1 + tWedges[ii].fU1 * fU;
	//		tri.v1 = frame->m_fV1 + tWedges[ii].fV1 * fV;
	//		tri.u2 = frame->m_fU1 + tWedges[ii].fU2 * fU;
	//		tri.v2 = frame->m_fV1 + tWedges[ii].fV2 * fV;
	//		tri.u3 = frame->m_fU1 + tWedges[ii].fU3 * fU;
	//		tri.v3 = frame->m_fV1 + tWedges[ii].fV3 * fV;
	//		UIComponentAddTriElement(component, texture, component->m_pFrame, &tri);
	//	}
	//	else
	//	{
	//		// partial wedges aren't smooth yet, need to replace the 0.1f constant with a lookup using partial
	//		// LU[partial]
	//		int partial = (int)((pct - (tWedges[ii].pct - 0.125f))/0.025f);

	//		float fX3, fY3, fU3, fV3;
	//		if (tWedges[ii].bHoldX == 1)
	//		{
	//			fU3 = tWedges[ii].fU3;
	//			fX3 = fU3 * width;
	//			fV3 = tWedges[ii].fOffset + tWedges[ii].fMult * (partial * 0.1f);
	//			fY3 = fV3 * height;
	//		}
	//		else
	//		{
	//			fU3 = tWedges[ii].fOffset + tWedges[ii].fMult * (partial * 0.1f);
	//			fX3 = fU3 * width;
	//			fV3 = tWedges[ii].fV3;
	//			fY3 = fV3 * height;
	//		}
	//		UI_TEXTURETRI tri;
	//		tri.x1 = tWedges[ii].fU1 * width;
	//		tri.y1 = tWedges[ii].fV1 * height;
	//		tri.x2 = tWedges[ii].fU2 * width;
	//		tri.y2 = tWedges[ii].fV2 * height;
	//		tri.x3 = fX3;
	//		tri.y3 = fY3;
	//		tri.u1 = frame->m_fU1 + tWedges[ii].fU1 * fU;
	//		tri.v1 = frame->m_fV1 + tWedges[ii].fV1 * fV;
	//		tri.u2 = frame->m_fU1 + tWedges[ii].fU2 * fU;
	//		tri.v2 = frame->m_fV1 + tWedges[ii].fV2 * fV;
	//		tri.u3 = frame->m_fU1 + fU3 * fU;
	//		tri.v3 = frame->m_fV1 + fV3 * fV;
	//		UIComponentAddTriElement(component, texture, component->m_pFrame, &tri);
	//		break;
	//	}
	//}
	//if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	//{
	//	CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	//}
	//component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UISkillCooldownAnimate, CEVENT_DATA((DWORD_PTR)component));

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGetUnitScreenBox(
	UNIT* unit,
	UI_RECT &rectout)
{
	if (unit)
	{
		VECTOR * pvFullbox = e_GetModelRenderOBBInWorld(c_UnitGetModelId(unit));
		if (!pvFullbox)
			return FALSE;
		MATRIX mProjMatrix;
		V( e_GetWorldViewProjMatrix( &mProjMatrix ) );

		VECTOR4 vScreenVec;
		for (int i=0; i < 8; i++)
		{
			MatrixMultiply( &vScreenVec, &pvFullbox[i], &mProjMatrix );
			float fScreenX = ((vScreenVec.fX / vScreenVec.fW + 1.0f) / 2.0f) * UIDefaultWidth();
			float fScreenY = ((-vScreenVec.fY / vScreenVec.fW + 1.0f) / 2.0f) * UIDefaultHeight();
			fScreenX = FLOOR(fScreenX) + 0.5f;
			fScreenY = FLOOR(fScreenY) + 0.5f;

			rectout.m_fX1 = (i==0 ? fScreenX : MIN(fScreenX, rectout.m_fX1));
			rectout.m_fY1 = (i==0 ? fScreenY : MIN(fScreenY, rectout.m_fY1));
			rectout.m_fX2 = (i==0 ? fScreenX : MAX(fScreenX, rectout.m_fX2));
			rectout.m_fY2 = (i==0 ? fScreenY : MAX(fScreenY, rectout.m_fY2));
		}

		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITargetedUnitPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT* selection = UIComponentGetFocusUnit(component);
	if (!selection)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* unit = UIGetControlUnit();
	if (!unit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentRemoveAllElements(component);

	#define BUFFER_SIZE (1024)
	WCHAR szSelection[BUFFER_SIZE];
	DWORD dwColor = GFXCOLOR_WHITE;
	DWORD dwFlags = 0;
	SETBIT( dwFlags, UNF_EMBED_COLOR_BIT );
	UnitGetName(selection, szSelection, BUFFER_SIZE, dwFlags, &dwColor);
	UI_COMPONENT *pComp = UIComponentFindChildByName(component, "selection");
	if (pComp)
	{
		UI_LABELEX* label = UICastToLabel(pComp);
		UILabelSetText(label, szSelection);
	}

	component->m_dwColor = dwColor;
	UIComponentAddFrame(component);

	if (!AppIsTugboat())
	{
		// get desc and set
		WCHAR szDescription[BUFFER_SIZE];
		UnitGetAffixDescription(selection, szDescription, BUFFER_SIZE);
		pComp = UIComponentFindChildByName(component, "selectiondesc");
		if (pComp)
		{
			UI_LABELEX* labelDescription = UICastToLabel(pComp);
			UILabelSetText(labelDescription, szDescription);
		}

		// display the monster type icon
		//   (this is going to change to a nice data-driven method with statdispl icons and an isa script function, but there's no time for now
		UI_COMPONENT* pTypeIcon = UIComponentFindChildByName(component, "target type icon");
		UI_TEXTURE_FRAME *pFrame = NULL;
		if (pTypeIcon)
		{
			UIX_TEXTURE * pTexture = UIComponentGetTexture(pTypeIcon);
			UIComponentRemoveAllElements(pTypeIcon);
			if (UnitIsA(selection, UNITTYPE_BEAST))
			{
				pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, "minigameicon beast");
			}
			else if (UnitIsA(selection, UNITTYPE_UNDEAD))
			{
				pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, "minigameicon necro");
			}
			else if (UnitIsA(selection, UNITTYPE_DEMON))
			{
				pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, "minigameicon demon");
			}
			else if (UnitIsA(selection, UNITTYPE_SPECTRAL))
			{
				pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, "minigameicon specter");
			}
			pTypeIcon->m_pFrame = pFrame;
			if (pFrame)
				UIComponentAddFrame(pTypeIcon);
		}
	}
	else
	{
		UIComponentSetFocusUnit(component, UnitGetId( selection ) );
		WCHAR szDescription[BUFFER_SIZE];
		WCHAR szResistances[BUFFER_SIZE];
		pComp = UIComponentFindChildByName(component, "selectiondesc");
		if (pComp)
		{
			UnitGetAffixDescription(selection, szDescription, BUFFER_SIZE);
			UI_LABELEX* labelDescription = UICastToLabel(pComp);
			UnitGetResistanceDescription( selection, szResistances, BUFFER_SIZE );;
			PStrCat( szDescription, szResistances, BUFFER_SIZE );
			UILabelSetText(labelDescription, szDescription);
		}
	}

	if (UIComponentIsTooltip(component))
	{
		UITooltipOnPaint(component, msg, wParam, lParam);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISelectionPanelHitIndicatorPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (msg == UIMSG_SETFOCUSUNIT || !pFocusUnit)
	{
		UIComponentSetFade(component, 1.0f);		// totally faded out

		//UIComponentSetVisible(component, FALSE);
		return UIMSG_RET_HANDLED;
	}

	if (msg == UIMSG_SETFOCUSSTAT &&
		(UNITID)wParam == UnitGetId(pFocusUnit))
	{
		UIComponentThrobStart(component, component->m_dwAnimDuration, component->m_dwAnimDuration * 2, TRUE);
		//UIComponentSetActive(component, TRUE);
		//UIComponentActivate(component, FALSE);  //fades it out
	}

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISelectionHealthBarOnSetTarget(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	BOOL bFocusInRange = (BOOL)lParam;
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_BAR *bar = UICastToBar(component);

	bar->m_bUseAltColor = !bFocusInRange;

	// force repaint
	bar->m_nOldValue = -1;
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISelectionPanelSetFocusVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNITID idUnit = (UNITID)wParam;
	//BOOL bFocusInRange = (BOOL)lParam;

	if (AppIsTugboat())
    {
	    int nScreenWidth, nScreenHeight;
	    e_GetWindowSize( nScreenWidth, nScreenHeight );
		float OpenWidth = (float)nScreenWidth - e_GetUICoverageWidth();
		float CenterPoint = ( OpenWidth * .5f + ( e_GetUICoveredLeft() ? e_GetUICoverageWidth() : 0 ) ) * UIGetScreenToLogRatioX();
		component->m_pParent->m_Position.m_fX = CenterPoint;

    }

	//DWORD dwAnimTime = 0;
	if (idUnit != INVALID_ID)
	{
		GAME *pGame = AppGetCltGame();
		UNIT *pUnit = UnitGetById(pGame, idUnit);
		UNIT *pPlayer = GameGetControlUnit(pGame);
		BOOL bShow = !UnitCanInteractWith( pUnit, pPlayer );
        if ( AppIsTugboat() )
        {
	        bShow = !( ( UnitIsA(pUnit, UNITTYPE_OBJECT) ||
			           UnitIsA(pUnit, UNITTYPE_DESTRUCTIBLE) ||
			           (UnitTestFlag( pUnit, UNITFLAG_CANBEPICKEDUP )  && ItemCanPickup(pPlayer, pUnit) == PR_OK)) );
        }

		if ( bShow )
		{
			UIComponentSetFocusUnit(component, idUnit);

			UIComponentActivate(component, TRUE);

			if (AppIsTugboat())
		    {
			    BOOL bLevelVisible = FALSE;
			    if( UnitGetGenus(pUnit) == GENUS_MONSTER  ||
					UnitGetGenus(pUnit) == GENUS_PLAYER )
			    {
				    bLevelVisible = TRUE;
			    }
			    UI_COMPONENT *pChild = NULL;
				    pChild = UIComponentFindChildByName(component, "selection level");
			    if( pChild )
			    {
				    UIComponentSetVisible(pChild, bLevelVisible );
			    }
			    pChild = UIComponentFindChildByName(component, "selection level label");
			    if( pChild )
			    {
				    UIComponentSetVisible(pChild, bLevelVisible );
			    }
				BOOL bKarmaVisible = FALSE;
				if( bLevelVisible && UnitIsA( pUnit, UNITTYPE_PLAYER ) &&
					PlayerIsInPVPWorld( UIGetControlUnit() ) )
				{
					bKarmaVisible = TRUE;
				}
				pChild = UIComponentFindChildByName(component, "selection karma");
				if( pChild )
				{
					UIComponentSetVisible(pChild, bKarmaVisible );
				}
				pChild = UIComponentFindChildByName(component, "selection karma label");
				if( pChild )
				{
					UIComponentSetVisible(pChild, bKarmaVisible );
				}

		    }

			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
			return UIMSG_RET_HANDLED;
		}
	}

	if (!AppIsTugboat())
    {
		UIComponentActivate(component, FALSE);
	}
    else
    {
	    UIComponentSetVisible( component, FALSE );
    }
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMinimalSelectionPanelSetFocusVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UITestFlag(UI_FLAG_ALT_TARGET_INFO))
	{
		UIComponentSetVisible( component, FALSE );
		return UIMSG_RET_HANDLED;
	}

	UNITID idUnit = (UNITID)wParam;
	BOOL bFocusInRange = (BOOL)lParam;
	GAME *pGame = AppGetCltGame();
	UNIT *pControl = GameGetControlUnit(pGame);

	if (idUnit != INVALID_ID &&
		pControl &&
		!UnitHasState( pGame, pControl, STATE_QUEST_RTS ) )
	{
		UIComponentSetFocusUnit(component, idUnit);

		UNIT *pUnit = UnitGetById(pGame, idUnit);
		BOOL bShow = !UnitCanInteractWith( pUnit, pControl );

		if ( bShow )
		{
			UIComponentSetActive(component, TRUE);
			BYTE byAlpha = 192;
			if (!bFocusInRange)
				byAlpha = 128;

			UIComponentSetAlpha(component, byAlpha);
			UI_COMPONENT *pChild = NULL;
			while ((pChild = UIComponentIterateChildren(component, pChild, -1, TRUE)) != NULL)
			{
				if (UIComponentIsBar(pChild) &&
					UIComponentGetAlpha(pChild) != byAlpha)
				{
					UIComponentRemoveAllElements(pChild);	// force repaint
				}
				UIComponentSetAlpha(component, byAlpha);
			}

			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
			return UIMSG_RET_HANDLED;
		}
	}

    UIComponentSetVisible( component, FALSE );

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentVisibleOnStat(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (msg == UIMSG_SETTARGETUNIT)
	{
		UIComponentSetFocusUnit(component, UNITID(wParam));
	}

	UNIT* pUnit = UIComponentGetFocusUnit(component);
	if (component->m_codeStatToVisibleOn == NULL_CODE ||
		!pUnit)
	{
		UIComponentSetVisible(component, FALSE);
		return UIMSG_RET_HANDLED;
	}

	int curvalue = VMExecI(UnitGetGame(pUnit), pUnit, g_UI.m_pCodeBuffer + component->m_codeStatToVisibleOn,
		g_UI.m_nCodeCurr - component->m_codeStatToVisibleOn);

	if (curvalue)
	{
		UIComponentSetVisible(component, TRUE);
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}
	else
	{
		UIComponentSetVisible(component, FALSE);
	}
	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIQuickClose(
	void)
{
	BOOL bAnyClosed = FALSE;
	PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_QUICK_CLOSE].GetNext(pNode)) != NULL)
	{
		// First make sure everying's useractive is false so nothing comes popping back up.  Just to be sure.
		if (pNode->Value->m_bUserActive)
		{
			pNode->Value->m_bUserActive = FALSE;
			UIComponentHandleUIMessage(pNode->Value, UIMSG_POSTINACTIVATE, 0, 0, FALSE);	// send a message to the component it's really closed now
																		// this could be formalized in a SetUserActive function since UIComponentActivate does this too
																		// currently only the item upgrade components differentiate useractive on postinactivate
		}
	}

	while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_QUICK_CLOSE].GetNext(pNode)) != NULL)
	{
		bAnyClosed |= UIComponentActivate(pNode->Value, FALSE);
	}

	return bAnyClosed;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMuteSounds(
	BOOL bMute)
{
	if (bMute)
	{
		g_UI.m_nMuteUISounds++;
	}
	else
	{
		g_UI.m_nMuteUISounds--;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIFlagSoundsForLoad()
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICloseAll(
	BOOL bSuppressUISounds /*= TRUE*/,
	BOOL bImmediate /*= FALSE*/)
{
	if (bSuppressUISounds)
	{
		UIMuteSounds(TRUE);
	}

	UI_COMPONENT *pAutomap = UIComponentGetByEnum(UICOMP_AUTOMAP);
	const BOOL bAutomapUA = UIComponentGetActive(UICOMP_AUTOMAP) || (pAutomap && pAutomap->m_bUserActive);

	PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listAnimCategories[UI_ANIM_CATEGORY_CLOSE_ALL].GetNext(pNode)) != NULL)
	{
		UIComponentActivate(pNode->Value, FALSE, 0, NULL, FALSE, FALSE, TRUE, bImmediate);
	}

	if (pAutomap && bAutomapUA)
	{
		pAutomap->m_bUserActive = TRUE;
	}

	ConsoleSetEditActive(FALSE);

	if (bSuppressUISounds)
	{
		UIMuteSounds(FALSE);
	}

	UIHideQuickMessage();

	sCancelAllDelayedActivates();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetRestartInTownString(
	UNIT *pPlayer,
	WCHAR *puszBuffer,
	int nMaxBuffer)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nMaxBuffer > 0, "Invalid buffer size" );

	// get replacement token
	const WCHAR *puszLevelToken = StringReplacementTokensGet( SR_LEVEL );

	int nLevelDef = PlayerGetRestartInTownLevelDef( pPlayer );
	ASSERTX_RETURN( nLevelDef != INVALID_LINK, "Expected level def" );
	const WCHAR *puszLevel = LevelDefinitionGetDisplayName( nLevelDef );

	// construct string
	PStrCopy( puszBuffer, GlobalStringGet( GS_RESTART_TOWN ), nMaxBuffer );
	PStrReplaceToken( puszBuffer, nMaxBuffer, puszLevelToken, puszLevel );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDisplayRestartScreen(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD dwData)
{
	APP_STATE eAppState = AppGetState();
	if (eAppState == APP_STATE_IN_GAME)
	{
		UNIT *pPlayer = UIGetControlUnit();
		ASSERTX_RETURN( pPlayer, "Expected player" );
		ASSERTX_RETURN( UIGetControlUnit() == pPlayer, "Expected control unit" );
		ASSERTX( IsUnitDeadOrDying( pPlayer ), "Displaying restart screen, but player isn't dead" );

		if( AppIsTugboat() )
		{
			UI_TB_HideAllMenus();
			// show the restart screen
			UIComponentActivate( UICOMP_RESTART, TRUE );
			ASSERTV( UIComponentGetActive( UICOMP_RESTART ), "Restart UI is not active" );

			UI_COMPONENT *pButtonGhost = UIComponentGetByEnum( UICOMP_RESTART_BUTTON_GHOST );
			UI_COMPONENT *pButtonResurrect = UIComponentGetByEnum( UICOMP_RESTART_BUTTON_RESURRECT );
			if( PlayerCanResurrect( pPlayer ) )
			{
				UIComponentSetActive( pButtonGhost, FALSE);
				UIComponentSetVisible( pButtonGhost, FALSE);
				UIComponentSetActive( pButtonResurrect, TRUE);
				UIComponentSetVisible( pButtonResurrect, TRUE);
			}
			else
			{
				UIComponentSetActive( pButtonGhost, TRUE);
				UIComponentSetVisible( pButtonGhost, TRUE);
				UIComponentSetActive( pButtonResurrect, FALSE);
				UIComponentSetVisible( pButtonResurrect, FALSE);

			}
		}
		else
		{

			const int MAX_STRING = 256;
			WCHAR uszString[ MAX_STRING ];
			const WCHAR *puszLevelToken = StringReplacementTokensGet( SR_LEVEL );

			// show the restart screen
			UIComponentActivate( UICOMP_RESTART, TRUE );
			ASSERTV( UIComponentGetActive( UICOMP_RESTART ), "Restart UI is not active" );

			// restarting in town
			UI_COMPONENT *pLabelTown = UIComponentGetByEnum( UICOMP_RESTART_LABEL_TOWN );
			if (pLabelTown)
			{
				UI_LABELEX *pLabel = UICastToLabel( pLabelTown );

				// what town level will the player start in
				sGetRestartInTownString( pPlayer, uszString, MAX_STRING );

				// set label
				UILabelSetText( pLabel, uszString );

			}

			// restarting as ghost
			BOOL bCanRespawnAsGhost = TRUE;
			UI_COMPONENT *pLabelGhost = UIComponentGetByEnum( UICOMP_RESTART_LABEL_GHOST );
			if (pLabelGhost)
			{
				UI_LABELEX *pLabel = UICastToLabel( pLabelGhost );

				// what level will the player start in
				LEVEL *pLevel = UnitGetLevel( pPlayer );
				int nLevelDef = LevelGetDefinitionIndex( pLevel );
				ASSERTX_RETURN( nLevelDef != INVALID_LINK, "Expected level def" );
				const WCHAR *puszLevel = LevelDefinitionGetDisplayName( nLevelDef );

				// construct string
				PStrCopy( uszString, GlobalStringGet( GS_RESTART_GHOST ), MAX_STRING );
				PStrReplaceToken( uszString, MAX_STRING, puszLevelToken, puszLevel );

				// set label
				UILabelSetText( pLabel, uszString );

				// set the button active or inactive
				UI_COMPONENT *pButtonGhost = UIComponentGetByEnum( UICOMP_RESTART_BUTTON_GHOST );
				bCanRespawnAsGhost = PlayerCanRespawnAsGhost ( pPlayer );
				UIComponentSetActive( pButtonGhost, bCanRespawnAsGhost);

			}

			// set string for resurrect cost
			BOOL bCanResurrect = TRUE;
			UI_COMPONENT *pButtonResurrect = UIComponentGetByEnum( UICOMP_RESTART_BUTTON_RESURRECT );
			if (pButtonResurrect)
			{

				// get cost for resurrection
				cCurrency nResurrectCost = PlayerGetResurrectCost( pPlayer );
				const int MAX_MONEY_STRING = 128;
				WCHAR uszMoneyString[ MAX_MONEY_STRING ];
				UIMoneyGetString( uszMoneyString, MAX_MONEY_STRING, nResurrectCost.GetValue( KCURRENCY_VALUE_INGAME ) );

				// construct string
				PStrPrintf( uszString, MAX_STRING, GlobalStringGet( GS_RESTART_RESURRECT ), uszMoneyString );

				// set the text in the label
				UI_COMPONENT *pLabelResurrect = UIComponentGetByEnum( UICOMP_RESTART_LABEL_RESURRECT );
				ASSERTX_RETURN( pLabelResurrect, "Expected label for resurrect componentisbutton" );
				UILabelSetText( pLabelResurrect, uszString );

				// set the button active or inactive
				bCanResurrect = PlayerCanResurrect( pPlayer );
				UIComponentSetActive( pButtonResurrect, bCanResurrect );

			}

			// alternate button setup for hellgate
			if (AppIsHellgate())
			{

				if (c_QuestGamePlayerArenaRespawn( pPlayer ))
				{
					UIComponentActivate( UICOMP_RESTART_BUTTON_RESURRECT, FALSE );
					UIComponentActivate( UICOMP_RESTART_BUTTON_TOWN, FALSE );
					UIComponentActivate( UICOMP_RESTART_BUTTON_GHOST, FALSE );
					UIComponentActivate( UICOMP_RESTART_BUTTON_ARENA, TRUE );
				}
				else
				{
					UIComponentActivate( UICOMP_RESTART_BUTTON_RESURRECT, bCanResurrect );
					UIComponentActivate( UICOMP_RESTART_BUTTON_TOWN, TRUE );
					UIComponentActivate( UICOMP_RESTART_BUTTON_GHOST, bCanRespawnAsGhost );
					UIComponentActivate( UICOMP_RESTART_BUTTON_ARENA, FALSE );
				}
			}
		}
	}
	else if (eAppState == APP_STATE_LOADING ||
			 eAppState == APP_STATE_PLAYMOVIELIST)
	{
		// something is happening
		// try again in a little while
		CEVENT_DATA tEventData;
		if ( AppIsTugboat() )
		{
			CSchedulerRegisterEvent(
				AppCommonGetCurTime() + TUGBOAT_DEATH_UI_DELAY_IN_MS,
				sDisplayRestartScreen,
				tEventData);
		}
		else
		{
			CSchedulerRegisterEvent(
				AppCommonGetCurTime() + DEATH_UI_DELAY_IN_MS,
				sDisplayRestartScreen,
				tEventData);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerDeath(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME* game = AppGetCltGame();
	ASSERT_RETVAL(game, UIMSG_RET_NOT_HANDLED);
	UNIT* unit = GameGetControlUnit(game);
	ASSERT_RETVAL(unit, UIMSG_RET_NOT_HANDLED);

	if (wParam == UnitGetId(unit))
	{
		// Close everything
		if (!AppIsTugboat())
		{
			UICloseAll();
		}
		else
	    {
			UIHideGameMenu();
			UI_TB_HideModalDialogs();
			ConsoleSetEditActive(FALSE);
	    }

		if (!UnitHasState(game, unit, STATE_NO_RESTART_MENU))
		{
			// display restart screen after a small delay
			CEVENT_DATA tEventData;
			if ( AppIsTugboat() )
			{
				CSchedulerRegisterEvent(
					AppCommonGetCurTime() + TUGBOAT_DEATH_UI_DELAY_IN_MS,
					sDisplayRestartScreen,
					tEventData);
			}
			else
			{
				CSchedulerRegisterEvent(
					AppCommonGetCurTime() + DEATH_UI_DELAY_IN_MS,
					sDisplayRestartScreen,
					tEventData);
			}
		}

		return UIMSG_RET_HANDLED;

	}

	return UIMSG_RET_NOT_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRestart(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME* game = AppGetCltGame();
	ASSERT_RETVAL(game, UIMSG_RET_NOT_HANDLED);
	UNIT* unit = GameGetControlUnit(game);
	ASSERT_RETVAL(unit, UIMSG_RET_NOT_HANDLED);
	if (wParam == UnitGetId(unit))
	{
		//UIComponentSetVisible(component, FALSE);
		UIComponentActivate(component, FALSE);
	}

	// slide main panel back in
	UIComponentActivate(UICOMP_DASHBOARD, TRUE);

	// restart the tracker if needed
	UIComponentActivate(UICOMP_MONSTER_TRACKER, TRUE, 0, NULL, FALSE, TRUE, FALSE);

	// re-activate the automap
	UI_COMPONENT *pAutomap = UIComponentGetByEnum(UICOMP_AUTOMAP);
	if (pAutomap && pAutomap->m_bUserActive)
	{
		UIComponentActivate(pAutomap, TRUE);
	}

	UIRepaint();
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRestartPanelOnSetControlState(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	if (nMessage == UIMSG_SETCONTROLSTATE)
	{
		ASSERT_RETVAL(lParam, UIMSG_RET_NOT_HANDLED);
		UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)CAST_TO_VOIDPTR(lParam);

		if (pData->m_nState == STATE_GHOST)
		{
			g_UI.m_bGrayout = !pData->m_bClearing;
			UIRepaint();

			return UIMSG_RET_HANDLED;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITooltipRestartTown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentCheckBounds( pComponent ) && UIComponentGetVisible( pComponent ))
	{
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
		{

			// get tooltip
			const int MAX_STRING = 512;
			WCHAR uszString[ MAX_STRING ];
			sGetRestartInTownString( pPlayer, uszString, MAX_STRING );

			// display tooltip
			UISetSimpleHoverText(
				pComponent,
				uszString,
				pComponent->m_bHoverTextWithMouseDown,
				pComponent->m_idComponent);

			return UIMSG_RET_HANDLED_END_PROCESS;

		}

	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnButtonDownGhostRestartTown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ) )
	{
		c_PlayerSendRespawn( PLAYER_REVIVE_GHOST );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnButtonDownRestartTown(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ) )
	{
		c_PlayerSendRespawn( PLAYER_RESPAWN_TOWN );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnButtonDownRestartGhost(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ) )
	{
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
		{
			if (PlayerCanRespawnAsGhost( pPlayer ))
			{
				c_PlayerSendRespawn( PLAYER_RESPAWN_GHOST );
			}
		}
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnButtonDownRestartResurrect(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ) )
	{
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
		{
			if (PlayerCanResurrect( pPlayer ))
			{
				c_PlayerSendRespawn( PLAYER_RESPAWN_RESURRECT );
			}
		}
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnButtonDownRestartArena(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ) )
	{
		c_PlayerSendRespawn( PLAYER_RESPAWN_ARENA );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUICinematicModeOn(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* top = UIComponentGetById(UIComponentGetIdByName("cinematic top"));
	if (top)
	{
		UIComponentOnActivate(top, UIMSG_ACTIVATE, top->m_dwAnimDuration, 0);
	}
	UI_COMPONENT* bottom = UIComponentGetById(UIComponentGetIdByName("cinematic bottom"));
	if (bottom)
	{
		UIComponentOnActivate(bottom, UIMSG_ACTIVATE, bottom->m_dwAnimDuration, 0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD sUICinematicModeOff(
	void)
{
	DWORD dwAnimDuration = 0;
	UI_COMPONENT* top = UIComponentGetById(UIComponentGetIdByName("cinematic top"));
	if (top)
	{
		dwAnimDuration = MAX(dwAnimDuration, top->m_dwAnimDuration);
		UIComponentOnInactivate(top, UIMSG_INACTIVATE, top->m_dwAnimDuration, 0);
	}
	UI_COMPONENT* bottom = UIComponentGetById(UIComponentGetIdByName("cinematic bottom"));
	if (bottom)
	{
		dwAnimDuration = MAX(dwAnimDuration, bottom->m_dwAnimDuration);
		UIComponentOnInactivate(bottom, UIMSG_INACTIVATE, bottom->m_dwAnimDuration, 0);
	}

	return dwAnimDuration;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//int UITargetSetFocus(
//	UI_COMPONENT* component,
//	int msg,
//	DWORD wParam,
//	DWORD lParam)
//{
//	if (!UIComponentGetVisible(component))
//	{
//		return 0;
//	}
//	UIX_TEXTURE* texture = UIComponentGetTexture(component);
//	if (!texture)
//	{
//		return 0;
//	}
//
//	UNITID idUnit = (UNITID)wParam;
//	GAME* game = AppGetCltGame();
//	ASSERT_RETZERO(game);
//
//	UNIT* target = UnitGetById(game, idUnit);
//	BOOL bSelectionInRange = (BOOL)lParam;
//
//	UIComponentSetFocusUnit(component, idUnit);
//
//	UIComponentRemoveAllElements(component);
//
//	if (!target)
//	{
//		UIComponentAddElement(component, texture, component->m_pFrame, UI_POSITION());
//	}
//	else if (UnitGetTargetType(target) == TARGET_BAD)
//	{
//		UIComponentAddElement(component, texture, component->m_pFrame, UI_POSITION());
//
//		UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "lock on");
//		if (frame)
//		{
//			DWORD dwColor = bSelectionInRange ? GFXCOLOR_GREEN : GFXCOLOR_RED;
//			UIComponentAddElement(component, texture, frame, UI_POSITION(), dwColor);
//		}
//	}
//	else if (UnitCanInteractWith( target, GameGetControlUnit(game) ))
//	{
//		UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "talk cursor");
//		if (frame)
//		{
//			UIComponentAddElement(component, texture, frame, UI_POSITION());
//		}
//	}
//	return 0;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UICanSelectTarget(
	UNITID idTarget)
{
	if ( AppCommonGetDemoMode() )
		return FALSE;

	UI_COMPONENT* component = NULL;

	component = UIComponentGetByEnum(UICOMP_PAPERDOLL);
	if (component && UIComponentGetVisible(component) )
	{
		return FALSE;
	}

	component = UIComponentGetByEnum(UICOMP_MERCHANTWELCOME);
	if (component && UIComponentGetVisible(component) )
	{
		return FALSE;
	}

	component = UIComponentGetByEnum(UICOMP_SKILLMAP);
	if (component && UIComponentGetVisible(component) )
	{
		return FALSE;
	}

	component = UIComponentGetByEnum(UICOMP_QUEST_PANEL);
	if (component && UIComponentGetVisible(component) )
	{
		return FALSE;
	}

	UNIT *player = GameGetControlUnit(AppGetCltGame());
	if (IsUnitDeadOrDying(player))
	{
		return FALSE;
	}

	if (AppIsHellgate())
	{
		if (UnitIsGhost(player))
		{
			BOOL bRestricted = TRUE;
			UNIT *pTarget = UnitGetById( AppGetCltGame(), idTarget );
			if (pTarget)
			{
				if (UnitIsA( pTarget, UNITTYPE_OBJECT ))
				{
					if (ObjectCanTrigger( player, pTarget ))
					{
						bRestricted = FALSE;
					}
				}
			}
			if (bRestricted)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT))
STR_DICT pstrDictMsgRetval[] =
{
	{ "UIMSG_RET_NOT_HANDLED",			UIMSG_RET_NOT_HANDLED},
	{ "UIMSG_RET_HANDLED",				UIMSG_RET_HANDLED},
	{ "UIMSG_RET_HANDLED_END_PROCESS",	UIMSG_RET_HANDLED_END_PROCESS},
	{ "UIMSG_RET_END_PROCESS",			UIMSG_RET_END_PROCESS},
	{ NULL,								UIMSG_RET_NOT_HANDLED}
};

void UITraceMsg(
	UI_COMPONENT *component,
	int msg,
	UI_MSG_RETVAL result)
{
	if (g_UI.m_bMsgTraceOn)
	{
		if (g_UI.m_szMsgTraceCompNameFilter[0])
		{
			if (PStrStrI(component->m_szName, g_UI.m_szMsgTraceCompNameFilter) == NULL)
			{
				return;
			}
		}

		if (g_UI.m_szMsgTraceMessageNameFilter[0])
		{
			if (PStrStrI(gUIMESSAGE_DATA[msg].szMessage, g_UI.m_szMsgTraceMessageNameFilter) == NULL)
			{
				return;
			}
		}

		if (g_UI.m_szMsgTraceResultsFilter[0])
		{
			if (PStrStrI(StrDictGet(pstrDictMsgRetval, result), g_UI.m_szMsgTraceResultsFilter) == NULL)
			{
				return;
			}
		}

		trace(" msg: %s\t\tcomp: %s\t\tresult: %s\n", gUIMESSAGE_DATA[msg].szMessage, component->m_szName, StrDictGet(pstrDictMsgRetval, result));
	}
}

void UISetMsgTrace(
	BOOL bOn,
	const char * szComponentName /*= ""*/,
	const char * szMsgName /*= ""*/,
	const char * szResult /*= ""*/)
{
	g_UI.m_bMsgTraceOn = bOn;
	if (bOn)
	{
		PStrCopy(g_UI.m_szMsgTraceCompNameFilter, szComponentName, 256);
		PStrCopy(g_UI.m_szMsgTraceMessageNameFilter, szMsgName, 256);
		PStrCopy(g_UI.m_szMsgTraceResultsFilter, szResult, 256);
	}
	else
	{
		g_UI.m_szMsgTraceCompNameFilter[0] = 0;
		g_UI.m_szMsgTraceMessageNameFilter[0] = 0;
		g_UI.m_szMsgTraceResultsFilter[0] = 0;
	}
}

#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIFindAndStopSound(
	UI_COMPONENT* pComponent, 
	int nSoundId)
{
	// note: this is kind of slow, because it has to iterate sounds for all messages
	//		 it's currently only used in one place, but if it becomes more common it should be optimized
	for (int j=0; j < NUM_UI_MESSAGES; j++)
	{
		PList<UI_SOUND_HANDLER>::USER_NODE *pStopNode = NULL;
		while ((pStopNode = g_UI.m_listSoundMessageHandlers[j].GetNext(pStopNode)) != NULL)
		{
			if (pStopNode->Value.m_pComponent == pComponent &&
				pStopNode->Value.m_nSoundId == nSoundId &&
				pStopNode->Value.m_bStopSound == FALSE &&
				pStopNode->Value.m_nPlayingSoundId != INVALID_ID)
			{
				c_SoundStop(pStopNode->Value.m_nPlayingSoundId);
				pStopNode->Value.m_nPlayingSoundId = INVALID_ID;
				return;
			}
		}
	}
}

static UI_MSG_RETVAL sUIComponentHandleUIMessage(
	UI_COMPONENT *pComponent,
	UI_MSGHANDLER *pHandler,
	int msg,
	WPARAM wParam,
	LPARAM lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	PFN_UIMSG_HANDLER pfnHandler = NULL;
	if (!pHandler)
	{
		pfnHandler = gUIMESSAGE_DATA[msg].m_fpDefaultFunctionPtr;
	}
	else
	{
		pfnHandler = pHandler->m_fpMsgHandler;
	}

	UI_MSG_RETVAL result = UIMSG_RET_NOT_HANDLED;

	if (IsStatMessage(msg))
	{
		if (g_UI.m_bIgnoreStatMessages)
			return UIMSG_RET_NOT_HANDLED;

		int nStatID = (int)(STAT_GET_STAT(lParam));
		UNITID idUnit = (UNITID)wParam;

		if (pHandler->m_nStatID != nStatID)
			return result;

		if (msg == UIMSG_SETFOCUSSTAT)
		{
			UNIT *pUnit = UIComponentGetFocusUnit(pComponent);
			if (!pUnit ||
				UnitGetId(pUnit) != idUnit)
			{
				return result;
			}
		}
	}

	BOOL bHandler = (pfnHandler != NULL);
	if (bHandler)
	{
		result = pfnHandler(pComponent, msg, (DWORD)SIZE_TO_INT(wParam), (DWORD)SIZE_TO_INT(lParam));
#if (ISVERSION(DEVELOPMENT))
		UITraceMsg(pComponent, msg, result);
#endif
	}

	if ((!bHandler || ResultIsHandled(result)) &&
		g_UI.m_nMuteUISounds <= 0)
	{
		PList<UI_SOUND_HANDLER>::USER_NODE *pNode = NULL;
		while ((pNode = g_UI.m_listSoundMessageHandlers[msg].GetNext(pNode)) != NULL)
		{
			if (pNode->Value.m_pComponent == pComponent &&
				pNode->Value.m_nSoundId != INVALID_LINK)
			{
				UI_SOUND_HANDLER &tSndHandler = pNode->Value;
				
				if (tSndHandler.m_bStopSound)
				{
					sUIFindAndStopSound(pComponent, pNode->Value.m_nSoundId);
				}
				else
				{
					tSndHandler.m_nPlayingSoundId = c_SoundPlay(tSndHandler.m_nSoundId, &c_SoundGetListenerPosition(), NULL);
				}
			}
		}
	}
	return result;
}


UI_MSG_RETVAL UIComponentHandleUIMessage(
	UI_COMPONENT* pComponent,
	int nMsg,
	WPARAM wParam,
	LPARAM lParam,
	BOOL bPassToChildren /*= FALSE*/)
{
	if (!pComponent)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (nMsg < 0)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	ASSERT_RETVAL(nMsg >= 0 && nMsg < NUM_UI_MESSAGES, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pMouseOverride = NULL;
	UI_COMPONENT *pKeyboardOverride = NULL;

	// If this is a mouse or keyboard message, and there's an override component, make sure this is a child of said component
	if (nMsg == UIMSG_MOUSEMOVE		||
		nMsg == UIMSG_LBUTTONDOWN	||
		nMsg == UIMSG_LBUTTONUP		||
		nMsg == UIMSG_LBUTTONDBLCLK ||
		nMsg == UIMSG_RBUTTONDOWN	||
		nMsg == UIMSG_RBUTTONUP		||
		nMsg == UIMSG_RBUTTONDBLCLK ||
		nMsg == UIMSG_MBUTTONDOWN	||
		nMsg == UIMSG_MBUTTONUP		||
		nMsg == UIMSG_MOUSEWHEEL)
	{
		pMouseOverride = UIGetMouseOverrideComponent();
		if (pMouseOverride && 
			!UIComponentIsParentOf(pMouseOverride, pComponent) &&
			!UIComponentIsParentOf(pComponent, pMouseOverride))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

	if (nMsg == UIMSG_KEYDOWN	||
		 nMsg == UIMSG_KEYUP	||
		 nMsg == UIMSG_KEYCHAR)
	{
		pKeyboardOverride = UIGetKeyboardOverrideComponent();
		if (pKeyboardOverride && 
			!UIComponentIsParentOf(pKeyboardOverride, pComponent) && 
			!UIComponentIsParentOf(pComponent, pKeyboardOverride))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}


	// This is a message that is sent directly to a component and not broadcast.
	// it needs to iterate the children in tree order even if this component does not
	// have a handler for this message.
	if (bPassToChildren || gUIMESSAGE_DATA[nMsg].bPassToChildren)
	{
		UI_COMPONENT* child = pComponent->m_pFirstChild;
		while (child)
		{
			UI_COMPONENT* next = child->m_pNextSibling;
			UI_MSG_RETVAL result = UIComponentHandleUIMessage(child, nMsg, wParam, lParam, bPassToChildren);
			if (ResultStopsProcessing(result))
			{
				return result;
			}
			child = next;
		}
	}

	if (pMouseOverride && 
		!UIComponentIsParentOf(pMouseOverride, pComponent))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if (pKeyboardOverride && 
		!UIComponentIsParentOf(pKeyboardOverride, pComponent))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// find the handler
	BOOL bFoundHandler = FALSE;
	UI_MSG_RETVAL eResult = UIMSG_RET_NOT_HANDLED;
	PList<UI_MSGHANDLER>::USER_NODE *pNode = NULL;
	while ((pNode = pComponent->m_listMsgHandlers.GetNext(pNode)) != NULL)
	{
		if (pNode->Value.m_nMsg == nMsg)
		{
			bFoundHandler = TRUE;
			eResult = sUIComponentHandleUIMessage(pComponent, &pNode->Value, nMsg, wParam, lParam);
			if (ResultStopsProcessing(eResult))
			{
				return eResult;
			}
		}
	}

	if (!bFoundHandler &&
		gUIMESSAGE_DATA[nMsg].m_fpDefaultFunctionPtr)
	{
		return sUIComponentHandleUIMessage(pComponent, NULL, nMsg, wParam, lParam);
	}

	return eResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UITranslateMessage(
	UINT msg,
	BOOL bReverse = FALSE)
{
	struct MSG_MAP
	{
		UINT nWinMsg;
		UINT nUIMsg;
	};

	static const MSG_MAP map[] = 
	{
		{ WM_PAINT,					UIMSG_PAINT },
		{ WM_MOUSEMOVE,				UIMSG_MOUSEMOVE },
		{ WM_LBUTTONDOWN, 			UIMSG_LBUTTONDOWN },
		{ WM_MBUTTONDOWN, 			UIMSG_MBUTTONDOWN },
		{ WM_RBUTTONDOWN, 			UIMSG_RBUTTONDOWN },
		{ WM_LBUTTONUP,				UIMSG_LBUTTONUP },
		{ WM_MBUTTONUP,		 		UIMSG_MBUTTONUP},
		{ WM_RBUTTONUP,		 		UIMSG_RBUTTONUP},
		{ WM_LBUTTONDBLCLK,		 	UIMSG_LBUTTONDBLCLK},
		{ WM_RBUTTONDBLCLK,		 	UIMSG_RBUTTONDBLCLK},
		{ WM_MOUSEWHEEL,		 	UIMSG_MOUSEWHEEL},
		{ WM_SYSKEYDOWN,			UIMSG_KEYDOWN},	
		{ WM_KEYDOWN,				UIMSG_KEYDOWN},	
		{ WM_SYSKEYUP,				UIMSG_KEYUP},
		{ WM_KEYUP,					UIMSG_KEYUP},
		{ WM_SYSCHAR,				UIMSG_KEYCHAR},
		{ WM_CHAR,					UIMSG_KEYCHAR},
		{ WM_INVENTORYCHANGE,		UIMSG_INVENTORYCHANGE},
		{ WM_UISETHOVERUNIT,		UIMSG_SETHOVERUNIT},
		{ WM_ACTIVATE,				UIMSG_ACTIVATE},
		{ WM_SETCONTROLUNIT,	 	UIMSG_SETCONTROLUNIT},
		{ WM_SETTARGETUNIT,		 	UIMSG_SETTARGETUNIT},
		{ WM_CONTROLUNITGETHIT,	 	UIMSG_CONTROLUNITGETHIT},
		{ WM_SETCONTROLSTAT,	 	UIMSG_SETCONTROLSTAT},
		{ WM_SETTARGETSTAT,		 	UIMSG_SETTARGETSTAT},
		{ WM_TOGGLEDEBUGDISPLAY, 	UIMSG_TOGGLEDEBUGDISPLAY},
		{ WM_PLAYERDEATH,		 	UIMSG_PLAYERDEATH},
		{ WM_RESTART,				UIMSG_RESTART},
		{ WM_POSTACTIVATE,			UIMSG_POSTACTIVATE},
		{ WM_POSTINACTIVATE,		UIMSG_POSTINACTIVATE},
		{ WM_MOUSEHOVER,			UIMSG_MOUSEHOVER},
		{ WM_MOUSEHOVERLONG,		UIMSG_MOUSEHOVERLONG},
		{ WM_SETCONTROLSTATE,		UIMSG_SETCONTROLSTATE},
		{ WM_SETTARGETSTATE,		UIMSG_SETTARGETSTATE},
		{ WM_SETFOCUSSTAT,			UIMSG_SETFOCUSSTAT},
		{ WM_FREEUNIT,				UIMSG_FREEUNIT},
		{ WM_PARTYCHANGE,			UIMSG_PARTYCHANGE},
		{ WM_WARDROBECHANGE,		UIMSG_WARDROBECHANGE},
		{ WM_SETFOCUS,				UIMSG_SETFOCUS},
		{ WM_INPUTLANGCHANGE,		UIMSG_INPUTLANGCHANGE}
	};

	for (int i=0; i < arrsize(map); i++)
	{
		if (!bReverse && map[i].nWinMsg == msg)
			return map[i].nUIMsg;
		if (bReverse && map[i].nUIMsg == msg)
			return map[i].nWinMsg;
	}

	ASSERT_RETVAL(FALSE, -1);
	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL sUI_ProcessMessage(
	GAME* game,
	const CEVENT_DATA& data)
{
	UI_COMPONENT* component = UIComponentGetById((int)data.m_Data1);
	if (!component)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	return UIComponentHandleUIMessage(component, (int)data.m_Data2, (WPARAM)data.m_Data3, (LPARAM)data.m_Data4, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISendHoverMessage(
	BOOL bLongDelay)
{
	if (!UICursorGetActive())
		return;

	float x, y;
	UIGetCursorPosition(&x, &y);

	if (UIGetMouseOverrideComponent())
	{
		UIComponentHandleUIMessage( UIGetMouseOverrideComponent(), bLongDelay ? UIMSG_MOUSEHOVERLONG : UIMSG_MOUSEHOVER, (WPARAM)x, (LPARAM)y, TRUE);
	}
	else
	{
		UIHandleUIMessage( bLongDelay ? UIMSG_MOUSEHOVERLONG : UIMSG_MOUSEHOVER, (WPARAM)x, (LPARAM)y );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIComponentWantsMouseLeave(UI_COMPONENT* component)
{
	UI_MSGHANDLER *pHandler = UIGetMsgHandler(component, UIMSG_MOUSELEAVE);
	if (!pHandler ||
		!pHandler->m_fpMsgHandler)
	{
		return FALSE;
	}

	if (pHandler->m_fpMsgHandler == UIComponentOnMouseLeave &&
		!component->m_szTooltipText)
	{
		return FALSE;
	}

	if ( pHandler->m_fpMsgHandler == UIIgnoreMsgHandler )
	{
		return FALSE;
	}
	return TRUE;
}
static BOOL sgbMouseOverPanel = FALSE;
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUIGenerateMouseMoveMessages(
	UI_COMPONENT* component,
	BOOL &bMouseOverSet,
	BOOL &bMouseOverMessagesStopped)
{
	if (bMouseOverMessagesStopped && bMouseOverSet)
		return;

	UI_COMPONENT *pOverrideMouseMsgComponent = UIGetMouseOverrideComponent();
	if (pOverrideMouseMsgComponent &&
		!(UIComponentIsParentOf(pOverrideMouseMsgComponent, component) ||
		UIComponentIsParentOf(component, pOverrideMouseMsgComponent)))
	{
		return;
	}

	if (!component ||
//		!UIComponentGetActive(component) ||
		!UIComponentGetVisible(component))
	{
		return;
	}

	UI_COMPONENT* cur = component->m_pFirstChild;
	while (cur)
	{
		sUIGenerateMouseMoveMessages(cur, bMouseOverSet, bMouseOverMessagesStopped);
		cur = cur->m_pNextSibling;
	}

	if (component->m_bIgnoresMouse ||
		!UIComponentCheckBounds(component))
	{
		// Special case for textboxes with attached scrollbars
		if (UIComponentIsTextBox(component))
		{
			UI_TEXTBOX *pTextBox = UICastToTextBox(component);
			if (!pTextBox->m_pScrollbar || !UIScrollBarCheckBounds(pTextBox->m_pScrollbar))
			{
				return;
			}
		}
		else
		{
			if (g_UI.m_pCurrentMouseOverComponent == component)
			{
				UIComponentHandleUIMessage(g_UI.m_pCurrentMouseOverComponent, UIMSG_MOUSELEAVE, 0, 0, FALSE);
				g_UI.m_pCurrentMouseOverComponent = NULL;
			}
			return;
		}
	}

	if (!bMouseOverSet &&
//		g_UI.m_pCurrentMouseOverComponent != component &&
		!component->m_bIgnoresMouse)
//		!UIComponentIsParentOf(component, g_UI.m_pCurrentMouseOverComponent))
	{
		// Only set this for components that care when they're being left
		if (UIComponentWantsMouseLeave(component))
		{
			if (g_UI.m_pCurrentMouseOverComponent &&
				g_UI.m_pCurrentMouseOverComponent != component)
			{
				// this will catch cases where two components that want mouseleave are overlapping
				UIComponentHandleUIMessage(g_UI.m_pCurrentMouseOverComponent, UIMSG_MOUSELEAVE, 0, 0, FALSE);
				//trace("sending MouseLeave to %s (%d)\n", g_UI.m_pCurrentMouseOverComponent->m_szName, g_UI.m_pCurrentMouseOverComponent->m_idComponent);
			}
			g_UI.m_pCurrentMouseOverComponent = component;
			bMouseOverSet = TRUE;
			//trace( "Current mouse over - %s (%d)\n", component->m_szName, component->m_idComponent);
		}
	}

	if (!bMouseOverMessagesStopped)
	{
		UI_MSG_RETVAL eResult = UIComponentHandleUIMessage(component, UIMSG_MOUSEOVER, 0, 0, FALSE);
		if (ResultStopsProcessing(eResult))
		{
			//trace( "!! Sending mouse over - %s\n", component->m_szName);
			//DWORD dwTime = (DWORD)AppCommonGetRealTime();
			//trace("MO (%d) %d %s\n", component->m_idComponent, dwTime, component->m_szName);
			bMouseOverMessagesStopped = TRUE;
			sgbMouseOverPanel = TRUE;
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGenerateMouseMoveMessages(
	void)
{
	sgbMouseOverPanel = FALSE;
	g_UI.m_eHoverState = UIHOVERSTATE_NONE;
	g_UI.m_timeNextHoverCheck = AppCommonGetAbsTime() + g_UI.m_nHoverDelay;

	if (!UIComponentGetActive(g_UI.m_Cursor))
	{
		UIHandleUIMessage(UIMSG_MOUSELEAVE, 0, 0);
		g_UI.m_pCurrentMouseOverComponent = NULL;
		return;
	}

	UI_COMPONENT *pMouseOverride = UIGetMouseOverrideComponent();
	BOOL bMouseOverSet = FALSE;
	BOOL bMouseOverMessagesStopped = FALSE;
	if (pMouseOverride)
	{
		sUIGenerateMouseMoveMessages(pMouseOverride, bMouseOverSet, bMouseOverMessagesStopped);
		bMouseOverMessagesStopped = TRUE;
		sgbMouseOverPanel = TRUE;
	}

	sUIGenerateMouseMoveMessages(g_UI.m_Components, bMouseOverSet, bMouseOverMessagesStopped);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)	// CHB 2006.12.22
void UIDebugEditSetComponent(
	const char *szCompName)
{
	UI_COMPONENT *pComp = UIComponentFindChildByName(g_UI.m_Components, szCompName, TRUE);
	if (pComp)
	{
		g_UI.m_idDebugEditComponent = pComp->m_idComponent;
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sGenerateMouseClickMessages(
	UI_COMPONENT* component,
	UINT msg,
	float x,
	float y)
{
	UI_COMPONENT *pOverrideMouseMsgComponent = UIGetMouseOverrideComponent();
	if (pOverrideMouseMsgComponent &&
		!(UIComponentIsParentOf(pOverrideMouseMsgComponent, component) ||
		  UIComponentIsParentOf(component, pOverrideMouseMsgComponent)))
	{
		return FALSE;
	}

	if (!component ||
		!UIComponentGetVisible(component))
	{
		return FALSE;
	}

	UI_COMPONENT* cur = component->m_pFirstChild;
	while (cur)
	{
		if (sGenerateMouseClickMessages(cur, msg, x, y))
		{
			return TRUE;
		}
		cur = cur->m_pNextSibling;
	}

	if (component->m_bIgnoresMouse)
	{
		return FALSE;
	}

	if (!UIComponentGetActive(component))
	{
		return FALSE;
	}

#if ISVERSION(DEVELOPMENT)
	if (msg == UIMSG_LBUTTONDOWN && GameGetDebugFlag(AppGetCltGame(), DEBUGFLAG_UI_EDIT_MODE) && !(GetKeyState(VK_CONTROL) & 0x8000))
	{
		if (UIComponentCheckBounds(component, (float)x, (float)y))
		{
			if (component != UIComponentGetByEnum(UICOMP_CONSOLE))
			{
				g_UI.m_idDebugEditComponent = component->m_idComponent;
				return TRUE;
			}
		}
	}
#endif

	if (msg == UIMSG_LBUTTONDOWN ||
		msg == UIMSG_RBUTTONDOWN ||
		msg == UIMSG_MBUTTONDOWN)
	{
		if (!UIComponentCheckBounds(component, (float)x, (float)y))
		{
			return FALSE;
		}

		DWORD dwRealTime = AppCommonGetRealTime();
		DWORD dwDoubleClickTime = GetDoubleClickTime();
		if (g_UI.m_pLastMouseDownComponent == component &&
			g_UI.m_nLastMouseDownMsg == msg &&
			dwRealTime - g_UI.m_dwLastMouseDownMsgTime <= dwDoubleClickTime)
		{
			switch (msg)
			{
			case UIMSG_LBUTTONDOWN:	UIComponentHandleUIMessage(component, UIMSG_LBUTTONDBLCLK, 0, 0, FALSE); break;
			case UIMSG_RBUTTONDOWN:	UIComponentHandleUIMessage(component, UIMSG_RBUTTONDBLCLK, 0, 0, FALSE); break;
			}
			return TRUE;
		}

		if ((msg == UIMSG_LBUTTONDOWN && (UIGetMsgHandler(component, UIMSG_LBUTTONCLICK) || UIGetMsgHandler(component, UIMSG_LBUTTONDBLCLK))) ||
			(msg == UIMSG_RBUTTONDOWN && (UIGetMsgHandler(component, UIMSG_RBUTTONCLICK) || UIGetMsgHandler(component, UIMSG_RBUTTONDBLCLK))) ||
			(msg == UIMSG_MBUTTONDOWN && (UIGetMsgHandler(component, UIMSG_MBUTTONCLICK)) ))
		{
			//trace ("btn down component = %d last = %d msg = %d last = %d time = %u <= %u\n",
			//	component->m_idComponent,
			//	(g_UI.m_pLastMouseDownComponent ? g_UI.m_pLastMouseDownComponent->m_idComponent : INVALID_ID),
			//	g_UI.m_nLastMouseDownMsg,
			//	msg,
			//	dwRealTime - g_UI.m_dwLastMouseDownMsgTime,
			//	dwDoubleClickTime);

			g_UI.m_dwLastMouseDownMsgTime = dwRealTime;
			g_UI.m_pLastMouseDownComponent = component;
			g_UI.m_nLastMouseDownMsg = msg;
			return TRUE;
		}
	}

	if (msg == UIMSG_LBUTTONUP ||
		msg == UIMSG_RBUTTONUP ||
		msg == UIMSG_MBUTTONUP)
	{
		if (g_UI.m_pLastMouseDownComponent == component)
		{
			if (!UIComponentCheckBounds(component, (float)x, (float)y))
			{
				g_UI.m_pLastMouseDownComponent = NULL;
				return FALSE;
			}

			switch (msg)
			{
			case UIMSG_LBUTTONUP:	UIComponentHandleUIMessage(component, UIMSG_LBUTTONCLICK, 0, 0, FALSE); break;
			case UIMSG_RBUTTONUP:	UIComponentHandleUIMessage(component, UIMSG_RBUTTONCLICK, 0, 0, FALSE); break;
			case UIMSG_MBUTTONUP:	UIComponentHandleUIMessage(component, UIMSG_MBUTTONCLICK, 0, 0, FALSE); break;
			}

			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGenerateMouseClickMessages(
	UI_COMPONENT* component,
	UINT msg,
	int x,
	int y)
{
	//if (msg == UIMSG_LBUTTONDOWN ||
	//	msg == UIMSG_RBUTTONDOWN ||
	//	msg == UIMSG_MBUTTONDOWN)
	//{
	//	g_UI.m_pLastMouseDownComponent = NULL;
	//	g_UI.m_nLastMouseDownMsg = UIMSG_NONE;
	//}

	if ((msg == UIMSG_LBUTTONUP && g_UI.m_nLastMouseDownMsg != UIMSG_LBUTTONDOWN) ||
		(msg == UIMSG_RBUTTONUP && g_UI.m_nLastMouseDownMsg != UIMSG_RBUTTONDOWN) ||
		(msg == UIMSG_MBUTTONUP && g_UI.m_nLastMouseDownMsg != UIMSG_MBUTTONDOWN))
	{
		return FALSE;
	}

	float fx = x * UIGetScreenToLogRatioX();
	float fy = y * UIGetScreenToLogRatioY();

	return sGenerateMouseClickMessages(component, msg, fx, fy);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIStopSkills(
	void)
{
	GAME* game = AppGetCltGame();
	if (game)
	{
		UNIT* unit = GameGetControlUnit(game);
		if (unit)
		{
			SkillStopAll(game, unit);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIStopAllSkillRequests(
	void)
{
	GAME* game = AppGetCltGame();
	if (game)
	{
		UNIT* unit = GameGetControlUnit(game);
		if (unit)
		{
			SkillStopAllRequests( unit );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUIShowInventoryScreen(
	INVENTORY_STYLE eStyle,
	UNITID idStyleFocus,
	UNITID idPortraitFocus,
	int nOfferDefinition,
	int nNumRewardTakes)
{
	// set focus unit for some styles
	UI_COMPONENT_ENUM eComponent = UICOMP_INVALID;

	// Tugboat now uses a pane system for all menus
	if (AppIsTugboat())
    {
		// what is the max distance
		float flMaxDistanceSq = UNIT_INTERACT_DISTANCE_SQUARED;
		if (eStyle == STYLE_TRADE)
		{
			flMaxDistanceSq = TRADE_DISTANCE_SQ;
		}
		switch (eStyle)
		{

			//----------------------------------------------------------------------------
		case STYLE_MERCHANT:
			{
				UIHideAllPaneMenus();
				// set the merchant focus
				UIComponentSetFocusUnitByEnum( UICOMP_PANEMENU_MERCHANT, idStyleFocus, TRUE );

				UITogglePaneMenu( KPaneMenuMerchant );
				UITogglePaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );

				CEVENT_DATA tCEventData;
				tCEventData.m_Data1 = KPaneMenuMerchant;
				tCEventData.m_Data2 = (DWORD_PTR)flMaxDistanceSq;

				CSchedulerRegisterEvent(
					AppCommonGetCurTime() + MSECS_PER_SEC,
					sUICheckTradeDistanceMythos,
					tCEventData );
				break;
			}

			//----------------------------------------------------------------------------
		case STYLE_TRAINER:
			{
				UIHideAllPaneMenus();
				// set the merchant focus
				UIComponentSetFocusUnitByEnum( UICOMP_PANEMENU_CRAFTINGTRAINER, idPortraitFocus, TRUE );

				UITogglePaneMenu( KPaneMenuCraftingTrainer );
				UITogglePaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );

				CEVENT_DATA tCEventData;
				tCEventData.m_Data1 = KPaneMenuCraftingTrainer;
				tCEventData.m_Data2 = (DWORD_PTR)flMaxDistanceSq;

				CSchedulerRegisterEvent(
					AppCommonGetCurTime() + MSECS_PER_SEC,
					sUICheckTradeDistanceMythos,
					tCEventData );
				break;
			}
			//----------------------------------------------------------------------------
		case STYLE_TRADE:
			{
				GAME *pGame = AppGetCltGame();
				UNIT *pPlayer = GameGetControlUnit( pGame );
				ASSERTX_RETNULL( pPlayer, "Expected player" );

				// set the focus unit of "our offer"
				UI_COMPONENT *pCompMyOffer = UIComponentGetByEnum( UICOMP_TRADE_MY_OFFER_GRID );
				UIComponentSetFocusUnit( pCompMyOffer, UnitGetId( pPlayer ) );

				// set the focus unit of "their offer"
				UI_COMPONENT *pCompTheirOffer = UIComponentGetByEnum( UICOMP_TRADE_THEIR_OFFER_GRID );
				UIComponentSetFocusUnit( pCompTheirOffer, idStyleFocus );
				UIShowPaneMenu( KPaneMenuTrade );
				UIShowPaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );

				CEVENT_DATA tCEventData;
				tCEventData.m_Data1 = KPaneMenuTrade;
				tCEventData.m_Data2 = (DWORD_PTR)flMaxDistanceSq;

				CSchedulerRegisterEvent(
					AppCommonGetCurTime() + MSECS_PER_SEC,
					sUICheckTradeDistanceMythos,
					tCEventData );
				break;

			}

			//----------------------------------------------------------------------------
		case STYLE_REWARD:
		case STYLE_OFFER:
			{
				break;

			}

			//----------------------------------------------------------------------------
		case STYLE_STASH:
			{
				UIHideAllPaneMenus();
				// set the merchant focus
				UIComponentSetFocusUnitByEnum( UICOMP_PANEMENU_STASH, idStyleFocus, TRUE );

				UITogglePaneMenu( KPaneMenuStash );
				UITogglePaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );
				CEVENT_DATA tCEventData;
				tCEventData.m_Data1 = KPaneMenuStash;
				tCEventData.m_Data2 = (DWORD_PTR)flMaxDistanceSq;

				CSchedulerRegisterEvent(
					AppCommonGetCurTime() + MSECS_PER_SEC,
					sUICheckTradeDistanceMythos,
					tCEventData );



				break;
			}

			//----------------------------------------------------------------------------
		case STYLE_PET:
			{
				// set the merchant focus
				UIComponentSetFocusUnitByEnum( UICOMP_PANEMENU_HIRELINGEQUIPMENT, idStyleFocus, TRUE );
				UI_COMPONENT *pPaperdoll = UIComponentGetByEnum( UICOMP_PANEMENU_HIRELINGEQUIPMENT );
				ASSERTX_RETNULL( pPaperdoll, "Expected component" );
				pPaperdoll = UIComponentFindChildByName( pPaperdoll, "hireling model");
				UIComponentSetFocusUnit( pPaperdoll, idStyleFocus, TRUE );

				UIShowPaneMenu( KPaneMenuHirelingEquipment );

				break;
			}
			//----------------------------------------------------------------------------
		case STYLE_CUBE:
			{
				// set the merchant focus
				GAME *pGame = AppGetCltGame();
				UNIT *pPlayer = GameGetControlUnit( pGame );
				ASSERTX_RETNULL( pPlayer, "Expected player" );
				UIComponentSetFocusUnitByEnum( UICOMP_PANEMENU_CUBE, UnitGetId( pPlayer ), TRUE );

				UIShowPaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );

				UIShowPaneMenu( KPaneMenuCube );

				break;
			}
		}

		return TRUE;
    }

	switch (eStyle)
	{

		//----------------------------------------------------------------------------
		case STYLE_MERCHANT:
		{
			// merchant ui will come up
			eComponent = UICOMP_MERCHANTINVENTORY;
			UIComponentSetFocusUnitByEnum(UICOMP_MERCHANTINVENTORY, idStyleFocus, TRUE);
			break;
		}

		//----------------------------------------------------------------------------
		case STYLE_TRAINER:
		{
			// merchant ui will come up
			eComponent = UICOMP_TRAINERINVENTORY;
			UIComponentSetFocusUnitByEnum(UICOMP_TRAINERINVENTORY, idStyleFocus, TRUE);
			break;
		}
		//----------------------------------------------------------------------------
		case STYLE_TRADE:
		{
			GAME *pGame = AppGetCltGame();
			UNIT *pPlayer = GameGetControlUnit( pGame );
			ASSERTX_RETNULL( pPlayer, "Expected player" );

			// trade ui will come up
			eComponent = UICOMP_TRADE_PANEL;

			// set the focus unit of "our offer"
			UI_COMPONENT *pCompMyOffer = UIComponentGetByEnum( UICOMP_TRADE_MY_OFFER_GRID );
			UIComponentSetFocusUnit( pCompMyOffer, UnitGetId( pPlayer ) );

			// set the focus unit of "their offer"
			UI_COMPONENT *pCompTheirOffer = UIComponentGetByEnum( UICOMP_TRADE_THEIR_OFFER_GRID );
			UIComponentSetFocusUnit( pCompTheirOffer, idStyleFocus );

			break;

		}

		//----------------------------------------------------------------------------
		case STYLE_REWARD:
		case STYLE_OFFER:
		{

			// reward ui will come up
			//eCompSecondary = UICOMP_REWARD_PANEL;
			//idFocusSecondary = idPlayer;  // player is actually the focus for the reward/offer
			eComponent = UICOMP_INVENTORY;
			sSetRewardInfo( AppGetCltGame(), idPortraitFocus, nOfferDefinition, nNumRewardTakes );
			break;

		}

		//----------------------------------------------------------------------------
		case STYLE_STASH:
		{
			// Stash ui will come up
			eComponent = UICOMP_STASH_PANEL;

			break;
		}

		//----------------------------------------------------------------------------
		case STYLE_PET:
		{
			eComponent = UICOMP_INVENTORY;
			UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idStyleFocus, TRUE);
			UIComponentSetFocusUnitByEnum(UICOMP_CHARSHEET, idStyleFocus, TRUE);
			break;
		}

		//----------------------------------------------------------------------------
		case STYLE_RECIPE:
		{
			eComponent = UICOMP_RECIPE_LIST_PANEL;
			break;
		}
		case STYLE_CUBE:
		{
			eComponent = UICOMP_CUBE_PANEL;
			break;
		}
		case STYLE_BACKPACK:
		{
			eComponent = UICOMP_BACKPACK_PANEL;
			break;
		}
		case STYLE_RECIPE_SINGLE:
		{
			eComponent = UICOMP_RECIPE_CURRENT_PANEL;
			break;
		}
		case STYLE_STATTRADER:
		{
			eComponent = UICOMP_STATTRADER_PANEL;
			break;
		}
		default:
		{
			eComponent = UICOMP_INVENTORY;
			break;
		}

	}
	if (!AppIsTugboat())
    {
	    UIStopSkills();
	    c_PlayerClearAllMovement(AppGetCltGame());
    }

	UIComponentActivate(eComponent, TRUE);
	//UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);

	// for trade situations, register a monitor to cancel trading if people wander away somehow
	switch (eStyle)
	{

		//----------------------------------------------------------------------------
		case STYLE_MERCHANT:
		case STYLE_TRADE:
		case STYLE_STASH:
		case STYLE_TRAINER:
		{

			// what is the max distance
			float flMaxDistanceSq = UNIT_INTERACT_DISTANCE_SQUARED;
			if (eStyle == STYLE_TRADE)
			{
				flMaxDistanceSq = TRADE_DISTANCE_SQ;
			}

			CEVENT_DATA tCEventData;
			tCEventData.m_Data1 = eComponent;
			tCEventData.m_Data2 = (DWORD_PTR)flMaxDistanceSq;

			CSchedulerRegisterEvent(
				AppCommonGetCurTime() + MSECS_PER_SEC,
				sUICheckTradeDistance,
				tCEventData );

			break;

		}

	}

	return TRUE;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowInventoryScreen(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		sUIShowInventoryScreen( STYLE_DEFAULT, INVALID_ID, INVALID_ID, INVALID_LINK, 0 );
		return UIMSG_RET_HANDLED;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIShowInventoryScreen(
	void)
{
	return sUIShowInventoryScreen(STYLE_DEFAULT, INVALID_ID, INVALID_ID, INVALID_LINK, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIShowPetInventoryScreen(
	UNIT *pPet)
{
	ASSERTX_RETFALSE( pPet, "Expected pet unit" );
	ASSERTX_RETFALSE( PetIsPet( pPet ), "Expected pet" );
	UNITID idPet = UnitGetId( pPet );
	return sUIShowInventoryScreen(STYLE_PET, idPet, idPet, INVALID_LINK, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHideWorldMapScreen(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		UI_TB_HideWorldMapScreen( TRUE );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowWorldMapScreen(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		UI_TB_ShowWorldMapScreen(  );
		return UIMSG_RET_HANDLED;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowTravelMapScreen(
								   UI_COMPONENT *component,
								   int nMessage,
								   DWORD wParam,
								   DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		UI_TB_ShowTravelMapScreen(  );
		return UIMSG_RET_HANDLED;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_TB_ToggleAutomap()
{
	UI_COMPONENT * pAutomap = UIComponentGetByEnum( UICOMP_AUTOMAP );
	if (pAutomap)
	{
		if (UIComponentGetVisible(pAutomap))
		{
			UIComponentSetVisible(pAutomap, FALSE);
		}
		else
		{
			UIComponentSetActive(pAutomap, TRUE);
		}
	}
}

//----------------------------------------------------------------------- -----
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowAutoMap(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		UI_TB_ToggleAutomap(  );
		return UIMSG_RET_HANDLED;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

void UI_TB_RepaintAutoMap()
{
	UI_COMPONENT * pAutomap = UIComponentGetByEnum( UICOMP_AUTOMAP );
	UIComponentHandleUIMessage(pAutomap, UIMSG_PAINT, 0, 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRepaintAutoMap(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	UI_TB_RepaintAutoMap(  );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIShowGameScreen(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
//		GAME *game = AppGetCltGame();
		UI_TB_HideAllMenus();

		if( UI_TB_ModalDialogsOpen() )
		{
			UI_TB_HideModalDialogs();
		}

		UIStopAllSkillRequests();

		UIShowGameMenu();
		return UIMSG_RET_HANDLED;
	}
	return UIMSG_RET_NOT_HANDLED;
}

// ^^^ Tugboat-specific
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemBoxOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);
	UNIT *pItem = UIComponentGetFocusUnit(component, FALSE);
	if (!pItem)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_ITEMBOX *pItemBox = UICastToItemBox(component);

	UIComponentAddElement(component, UIComponentGetTexture(component), component->m_pFrame, UI_POSITION(0.0f, 0.0f));

	if ( pItem->pIconTexture )
	{
		BOOL bLoaded = UITextureLoadFinalize( pItem->pIconTexture );
		REF( bLoaded );
		UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pItem->pIconTexture->m_pFrames, "icon");
		if (frame && bLoaded)
		{

			UI_SIZE sizeIcon;
			sizeIcon.m_fWidth = (float)pItem->pUnitData->nInvWidth * ICON_BASE_TEXTURE_WIDTH;
			sizeIcon.m_fHeight = (float)pItem->pUnitData->nInvHeight * ICON_BASE_TEXTURE_WIDTH;


			if( sizeIcon.m_fHeight > component->m_fHeight )
			{
				float Ratio = component->m_fHeight / sizeIcon.m_fHeight;
				sizeIcon.m_fHeight *= Ratio;
				sizeIcon.m_fWidth *= Ratio;
			}
			if( sizeIcon.m_fWidth > component->m_fWidth )
			{
				float Ratio = component->m_fWidth / sizeIcon.m_fWidth;
				sizeIcon.m_fHeight *= Ratio;
				sizeIcon.m_fWidth *= Ratio;
			}
			float OffsetX =  component->m_fWidth / 2 - sizeIcon.m_fWidth / 2;
			float OffsetY =  component->m_fHeight / 2 - sizeIcon.m_fHeight / 2;

			UIComponentAddElement(component, pItem->pIconTexture, frame, UI_POSITION(OffsetX, OffsetY) , GFXCOLOR_WHITE, NULL, 0, 1.0f, 1.0f, &sizeIcon  );//, GetFontColor(pSkillData->nIconColor), NULL, FALSE, sSmallSkillIconScale());
		}
		else
		{
			// it's not asynch loaded yet - schedule a repaint so we can take care of it
			if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
			{
				CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
			}
			component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC / 10, UISchedulerMsgCallback, CEVENT_DATA((DWORD_PTR)component, UIMSG_PAINT, wParam, lParam));
		}
	}
	else
	{
		UIComponentAddItemGFXElement(
			component,
			pItem,
			UI_RECT( 0.0f, 0.0f, component->m_fWidth, component->m_fHeight ));
	}

	// add quantity required
	UI_COMPONENT *pCompQuantity = UIComponentFindChildByName( component, "label quantity required");
	if (pCompQuantity)
	{
		UI_LABELEX *pTextQuantity = pCompQuantity ? UICastToLabel( pCompQuantity ) : NULL;
		if (pTextQuantity)
		{
			if( AppIsHellgate() )
			{
				UNIT *pPlayer = UIGetControlUnit();
				ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

				DWORD dwInvIterateFlags = 0;
				SETBIT(dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT);
				int nPlayerHas = UnitInventoryCountItemsOfType(
					pPlayer,
					UnitGetClass(pItem),
					dwInvIterateFlags,
					ItemGetQuality(pItem),
					INVLOC_NONE);

				int nNeeded = ItemGetQuantity(pItem);

				WCHAR szPlayerHas[ 120 ];
				LanguageFormatIntString( szPlayerHas, arrsize(szPlayerHas), nPlayerHas);
				WCHAR szNeeded[ 120 ];
				LanguageFormatIntString( szNeeded, arrsize(szNeeded), nNeeded);


				WCHAR szText[256];
				PStrPrintf(szText, arrsize(szText), L"%s (%s)", szNeeded, szPlayerHas);

				UI_RECT rectComponent = UIComponentGetRect( pTextQuantity );

				pTextQuantity->m_dwColor = GetFontColor( nPlayerHas >= nNeeded ? FONTCOLOR_RECIPE_COMPLETE : FONTCOLOR_RECIPE_NOT_COMPLETE );

				UILabelSetText(pTextQuantity, szText);
			}
			else
			{
				UNIT *pPlayer = UIGetControlUnit();
				ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

				DWORD dwInvIterateFlags = 0;
				SETBIT(dwInvIterateFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT);
				int nPlayerHas = UnitInventoryCountItemsOfType(
					pPlayer,
					UnitGetClass(pItem),
					dwInvIterateFlags,
					ItemGetQuality(pItem),
					INVLOC_NONE);

				int nNeeded = ItemGetQuantity(pItem);

				WCHAR szPlayerHas[ 120 ];
				LanguageFormatIntString( szPlayerHas, arrsize(szPlayerHas), nPlayerHas);
				WCHAR szNeeded[ 120 ];
				LanguageFormatIntString( szNeeded, arrsize(szNeeded), nNeeded);


				WCHAR szText[256];
				WCHAR szHas[256];
				PStrPrintf(szHas, arrsize(szNeeded), L"%s/%s", szPlayerHas, szNeeded );
				UIColorCatString(szText, arrsize(szText), (FONTCOLOR)( nPlayerHas >= nNeeded ? FONTCOLOR_GREEN : FONTCOLOR_LIGHT_RED ), szHas );

				//PStrPrintf(szHas, arrsize(szHas), L"/%s", szNeeded );
				//UIColorCatString(szText, arrsize(szHas), FONTCOLOR_WHITE, szHas );
				//PStrCat( szText, szHas, 256 );

				UI_RECT rectComponent = UIComponentGetRect( pTextQuantity );

				//pTextQuantity->m_dwColor = GetFontColor( nPlayerHas >= nNeeded ? FONTCOLOR_RECIPE_COMPLETE : FONTCOLOR_RECIPE_NOT_COMPLETE );

				UILabelSetText(pTextQuantity, szText);
			}
			
		}

	}
	pCompQuantity = UIComponentFindChildByName( component, "label quantity");
	if (pCompQuantity)
	{
		UI_LABELEX *pTextQuantity = pCompQuantity ? UICastToLabel( pCompQuantity ) : NULL;
		if (pTextQuantity)
		{

			int nQuantity = ItemGetQuantity( pItem );

			if ( nQuantity > 1 )
			{
				UI_SIZE uiSize(component->m_fWidth, component->m_fHeight);
				const int MAX_STRING = 8;
				WCHAR uszText[ MAX_STRING ];
				LanguageFormatIntString( uszText, MAX_STRING, nQuantity );
				UI_RECT rectComponent = UIComponentGetRect( pTextQuantity );

				UIComponentAddTextElement(
					pTextQuantity,
					UIComponentGetTexture(pTextQuantity),
					UIComponentGetFont(pTextQuantity),
					UIComponentGetFontSize(pTextQuantity),
					uszText,
					UI_POSITION(0, 0),
					pItemBox->m_dwColorQuantity,
					NULL,
					pTextQuantity->m_nAlignment,
					&uiSize);
			}

		}
	}

	UI_SIZE uiSize(component->m_fWidth, component->m_fHeight);

	// draw background for slot
	int nBkgdColor = UIInvGetItemGridBackgroundColor(pItem);
	DWORD dwColor = GetFontColor(nBkgdColor);
	if( AppIsTugboat() && dwColor == GFXCOLOR_WHITE)
	{
		nBkgdColor = INVALID_ID;
	}	
	if (nBkgdColor != INVALID_ID && pItemBox->m_pFrameBackground)
	{
		UIComponentAddElement(
			component,
			UIComponentGetTexture(component),
			pItemBox->m_pFrameBackground,
			UI_POSITION(0, 0),
			dwColor,
			NULL,
			TRUE,
			1.0f,
			1.0f,
			&uiSize);
	}

	return UIMSG_RET_HANDLED;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemBoxOnMouseHover(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{

	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pUnit = UIComponentGetFocusUnit( pComponent, FALSE );
	if (pUnit)
	{
		UISetHoverTextItem( pComponent, pUnit );
		UISetHoverUnit( UnitGetId( pUnit ), UIComponentGetId( pComponent ) );
	}
	else
	{
		UISetHoverUnit( INVALID_ID, UIComponentGetId( pComponent ) );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemBoxOnMouseLeave(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	UISetHoverUnit( INVALID_ID, UIComponentGetId( pComponent ) );
	return UIMSG_RET_HANDLED;//_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnPostActivateSetWelcomeMsg(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component &&
		UIComponentGetActive(component))
	{
		static const int scnNumberOfWelcomes = 7;
		int nNumWelcome = RandGetNum(AppGetCltGame(), 1, scnNumberOfWelcomes);
		static const char *szWelcomeString = "Bodger Welcome %d";
		char szBuf[256];
		PStrPrintf(szBuf, 256, szWelcomeString, nNumWelcome);

		UILabelSetTextByStringKey(UIComponentGetById(component->m_idComponent), szBuf);
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnPostActivateSetStatTraderWelcomeMsg(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component &&
		UIComponentGetActive(component))
	{
		const WCHAR * pFormat;
		WCHAR	szBuf[1024];
		pFormat = StringTableGetStringByKey("stattrader welcome");
		GAME* game = AppGetCltGame();
		if (!game)
			return UIMSG_RET_NOT_HANDLED;
		UNIT* unit = GameGetControlUnit(game);
		if (!unit)
			return UIMSG_RET_NOT_HANDLED;
		const UNIT_DATA * player_data = PlayerGetData(game, UnitGetClass(unit));
		if (!player_data)
			return UIMSG_RET_NOT_HANDLED;
		const PLAYERLEVEL_DATA * level_data = PlayerLevelGetData(game, player_data->nUnitTypeForLeveling, UnitGetExperienceLevel(unit));
		if (!level_data)
			return UIMSG_RET_NOT_HANDLED;
		cCurrency buyPrice = level_data->nStatRespecCost;
		PStrPrintf(szBuf, 1024, pFormat, buyPrice);
		UILabelSetText(component, szBuf);
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnPostActivatePaperdoll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component && UIComponentGetActive(component))
	{
		// KCK: Store old pitch, so we can put it back when we're done
		GAME *pGame = AppGetCltGame();
		c_PlayerGetAngles( pGame, &g_fCameraAngle, &g_fCameraPitch );

		// Get the focus unit for the paperdoll and set the camera to them
		UNIT *pUnitFocus = UIComponentGetFocusUnit( component );
		UNIT *pControl = GameGetControlUnit( AppGetCltGame() );
		UNIT *pCameraUnit = pUnitFocus ? pUnitFocus : pControl;
		if (pCameraUnit)
		{
			GameSetCameraUnit( pGame, UnitGetId( pCameraUnit ) );
		}

		c_QuestEventOpenUI( UIE_PAPER_DOLL );

		if (c_CameraGetViewType() != VIEW_VANITY)
		{
			if (GameGetControlUnit(pGame) == UIComponentGetFocusUnit(component))
			{
				// flip the player around so that the model faces the current camera.
				c_PlayerSetAngles( pGame, g_fCameraAngle + PI );
			}

			c_CameraSetViewType(VIEW_VANITY, TRUE);
			c_WeaponSetLockOnVanity( FALSE );
			c_CameraSetVanityPitch(0.0f);

			VECTOR vDirection = UnitGetFaceDirection( pCameraUnit, FALSE );
			UnitSetFaceDirection( pCameraUnit, vDirection, TRUE, FALSE, FALSE );
			c_UnitUpdateFaceTarget( pCameraUnit, TRUE );
		}

		// we only want to show the weaponconfigs for the player
		UI_COMPONENT *pWeaponconfig = NULL;
		while ((pWeaponconfig = UIComponentIterateChildren(component, pWeaponconfig, UITYPE_WEAPON_CONFIG, TRUE)) != NULL)
		{
			UIComponentActivate(pWeaponconfig, pControl == pUnitFocus);
		}

		if (AppIsHellgate())
		{
			UI_COMPONENT *pBackpack = UIComponentGetByEnum(UICOMP_INVENTORY);
			if (pBackpack)
			{
				UIComponentHandleUIMessage(pBackpack, UIMSG_PAINT, 0, 0);
			}
		}
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOnPostInactivatePaperdoll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (component)
	{
		if (AppIsHellgate())
		{
			UNIT *pPlayer = UIComponentGetFocusUnit(component);
			if (pPlayer) c_TutorialInputOk( pPlayer, TUTORIAL_INPUT_INVENTORY_CLOSE );
		}

		GAME *pGame = AppGetCltGame();
		c_PlayerGetAngles( pGame, &g_fCameraAngle, NULL );

		c_WeaponSetLockOnVanity( TRUE );
		// restore camera unit
		UNIT *pControl = UIGetControlUnit();
		if (pControl)
		{
			GameSetCameraUnit( pGame, UnitGetId( pControl ) );
		}

		if (c_CameraGetViewType() == VIEW_VANITY)
		{
			if (GameGetControlUnit(pGame) == UIComponentGetFocusUnit(component))
			{
				// flip the player (and the camera) back around
				g_fCameraAngle += PI;
				// KCK: Normally this would be handled in c_PlayerSetAngles, but as the player is already
				// back to being the camera object, this step gets skipped. Setting as Camera unit after
				// this point will clobber the pitch back to zero, which I can't stand. So just do it here.
				VECTOR	vDirection;
				UNIT	*pUnit = GameGetControlUnit( pGame );
				VectorDirectionFromAnglePitchRadians( vDirection, g_fCameraAngle,  0.0f );
				UnitSetFaceDirection( pUnit, vDirection, TRUE, FALSE, FALSE );
			}

			// Reset old pitch
			c_PlayerSetAngles( pGame, g_fCameraAngle, g_fCameraPitch );
			c_CameraRestoreViewType();
		}

		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetCinematicMode(
	BOOL bOn)
{
	UIStopSkills();
	UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);
	c_PlayerClearAllMovement(AppGetCltGame());

	// drb - i'm not sure what this is supposed to be, but it isn't UICOMP_LEFTSKILL...
	// the code is supposed to check if the bars are already in the state they need to be
	// in and bails if they are, but it uses the wrong component...
/*	UI_COMPONENT* leftskill = UIComponentGetByEnum(UICOMP_LEFTSKILL);
	if (!leftskill)
	{
		return;
	}
	BOOL bActive = UIComponentGetActive(leftskill);

	if (bActive != bOn)
	{
		return;		// if we're already in/not in the mode we want
	}*/

	if (bOn)
	{
		// Hide the main controls
		UICrossHairsShow(FALSE);

// BSP - Broken all to hell now.  I will fix

		//DWORD dwAnimationTime = UIMainPanelSlideOut();
		//// Show the black bars
		//CSchedulerRegisterEvent(AppCommonGetCurTime() + dwAnimationTime, sUICinematicModeOn, CEVENT_DATA());
	}
	else if (!AppIsTugboat())
	{
		/*DWORD dwAnimationTime = */sUICinematicModeOff();

// BSP - Broken all to hell now.  I will fix

		//UI_COMPONENT* pMainScreen = UIComponentGetByEnum(UICOMP_MAINSCREEN);
		//if (!pMainScreen)
		//	return;

		//// It might have previously been sliding in. If so, cancel that.
		//if (pMainScreen->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
		//{
		//	CSchedulerCancelEvent(pMainScreen->m_tMainAnimInfo.m_dwAnimTicket);
		//}
		//pMainScreen->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwAnimationTime, UIMainPanelSlideIn, CEVENT_DATA());

		UICrossHairsShow(TRUE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITasksAccept(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{
		c_TasksTryAccept();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}



inline UI_MSG_RETVAL UITasksQuestUpdateCheckMarks( UI_COMPONENT *component, BOOL bSelectQuest )
{
	if( component->m_pParent )
	{
		int nQuestIndex = (int)component->m_pParent->m_dwParam;
		int nQuestID = MYTHOS_QUESTS::QuestGetQuestInQueueByOrder( UIGetControlUnit(), nQuestIndex );
		if( nQuestID != INVALID_ID )
		{
			c_QuestSetQuestShowInGame( nQuestID, !c_QuestShowsInGame( nQuestID ) );			
			UIButtonSetDown( UICastToButton( component ), c_QuestShowsInGame( nQuestID ) );
			if( AppIsHellgate() )
			{
				c_TasksSetActiveTaskByIndex( nQuestIndex );
				if( bSelectQuest )
				{
					UITasksSelectQuest( nQuestIndex );
				}
			}
		}
		else
		{
			UIButtonSetDown( UICastToButton( component ), FALSE );
		}
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetQuestButton( int Button )
{

	UI_PANELEX* panel = UICastToPanel(UIComponentGetByEnum(UICOMP_QUESTBUTTONS));
	UIPanelSetTab( panel, Button );
	UI_COMPONENT *pChild = panel->m_pFirstChild;
	if( AppIsTugboat() )
	{
		while( pChild )
		{			
			UIButtonOnButtonDown( pChild, 0, 0, 0 );			
			pChild = pChild->m_pNextSibling;
		}	
	}
	else
	{	
		while( pChild )
		{
			
			if( (int)pChild->m_dwParam == Button )
			{

				UIButtonOnButtonDown( pChild, 0, 0, 0 );
			}
			else
			{
				UIButtonSetDown( UICastToButton( pChild ), FALSE );
			}
			
			if( (int)pChild->m_dwParam == Button )
			{

				UIButtonOnButtonDown( pChild, 0, 0, 0 );
			}
			else
			{
				UIButtonSetDown( UICastToButton( pChild ), FALSE );
			}

			UI_COMPONENT* childCheckMark = pChild->m_pFirstChild;
			while( childCheckMark )
			{
				if( UIComponentIsButton( childCheckMark ) )
				{
					UITasksQuestUpdateCheckMarks( childCheckMark, FALSE );
				}
				childCheckMark = childCheckMark->m_pNextSibling;
			}
			
			pChild = pChild->m_pNextSibling;
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITasksSelectQuest( int nIndex )
{
	UISetQuestButton( nIndex );

	c_TasksSetActiveTaskByIndex( nIndex );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITasksQuestMark(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentGetVisible(component) && UIComponentCheckBounds(component) )
	{
		UI_MSG_RETVAL msg = UITasksQuestUpdateCheckMarks( component, TRUE );
		return msg;
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITasksSelectQuest(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component) && UIComponentGetVisible(component) )
	{
		/*
		UISetQuestButton( (int)component->m_dwParam );


		c_TasksSetActiveTaskByIndex( (int)component->m_dwParam );*/
		UIButtonOnButtonDown( component, nMessage, wParam, lParam );
		UITasksSelectQuest( (int)component->m_dwParam );

		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleMagnify(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component) && UIComponentGetVisible(component) )
	{
		if( UIGetAlwaysShowItemNamesState() )
		{
			UISetAlwaysShowItemNames( FALSE );
			UIButtonSetDown( UICastToButton( component ), FALSE );
		}
		else
		{
			UISetAlwaysShowItemNames( TRUE );
			UIButtonSetDown( UICastToButton( component ), TRUE );
		}
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleWeaponSet(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component) && UIComponentGetVisible(component) )
	{
		MSG_CCMD_REQ_WEAPONSET_SWAP msg;
		msg.wSet = (short)component->m_dwParam;
		c_SendMessage(CCMD_REQ_WEAPONSET_SWAP, &msg);

		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWeaponSetOnPaint(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentGetVisible(component) )
	{
		UNIT* pPlayer = UIGetControlUnit();

		if( UnitGetStat(pPlayer, STATS_ACTIVE_WEAPON_SET) == (int)component->m_dwParam)
		{
			UIButtonSetDown( UICastToButton( component ), TRUE );
		}
		else
		{
			UIButtonSetDown( UICastToButton( component ), FALSE );
		}
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

		return UIMSG_RET_HANDLED;
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIToggleVoiceChat(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component) && UIComponentGetVisible(component) )
	{
		if( !UIButtonGetDown( UICastToButton( component ) ) )
		{
			c_VoiceChatStop();
		}
		else
		{
			c_VoiceChatStart();
		}

		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIVoiceChatOnPostCreate(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if( c_VoiceChatIsEnabled() )
	{
		UIButtonSetDown( UICastToButton( component ), TRUE );
	}
	else
	{
		UIButtonSetDown( UICastToButton( component ), FALSE );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStashOnPostInvisible(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (c_StashClose() == TRUE)
	{
		UI_MSG_RETVAL UISetChatActive( UI_COMPONENT *pComponent, int nMessage, DWORD wParam, DWORD lParam );
		UISetChatActive(pComponent, nMessage, wParam, lParam);
		return UIMSG_RET_HANDLED;
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITasksReject(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{
		c_TasksTryReject();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITasksAbandon(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component) && UIComponentGetVisible(component))
	{
		c_TasksTryAbandon();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITasksAcceptReward(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{
		c_TasksTryAcceptReward();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleWorldMap()
{
	if( AppIsHellgate() )
	{
		UI_COMPONENT * pWorldMap = UIComponentGetByEnum( UICOMP_WORLD_MAP_HOLDER );

		if (pWorldMap)
		{
			UIComponentActivate(pWorldMap, !UIComponentGetActive(pWorldMap));

		//	if (UIComponentGetVisible(pWorldMap))
		//	{
		//		UIComponentSetVisible(pWorldMap, FALSE);
		//	}
		//	else
		//	{
		//		UIComponentSetActive(pWorldMap, TRUE);
		//		UIComponentSetVisibleByEnum(UICOMP_AUTOMAP, FALSE);
		//		UIComponentSetVisibleByEnum(UICOMP_AUTOMAP_BIG, FALSE);
		//		UIComponentHandleUIMessage(pWorldMap, UIMSG_PAINT, 0, 0);
		//	}
		}
	}
	else
	{
		if( UI_TB_IsWorldMapScreenUp() )
		{
			UI_TB_HideWorldMapScreen( TRUE );
		}
		else
		{
			UI_TB_ShowWorldMapScreen();
		}

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleShowItemNames(
	BOOL bState )
{
	BOOL bCurrent = UIGetShowItemNamesState( TRUE );
	if ( bState == bCurrent )
	{
		return;
	}

	//if ( bState )
	//{
	//	UIStopSkills();
	//}
	g_UI.m_bShowItemNames = bState;

	if ( bState )
	{
		// turn off closest item stuff
		UI_COMPONENT * closest = UIComponentGetByEnum( UICOMP_CLOSESTITEM );
		UIComponentSetVisible( closest, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGetShowItemNamesState( BOOL bIgnoreAlwaysState /* = FALSE */)
{
	if (AppIsTugboat())
	{
		if( bIgnoreAlwaysState )
		{
			return g_UI.m_bShowItemNames;
		}
		else
		{
			return UIGetAlwaysShowItemNamesState() ? ( !g_UI.m_bShowItemNames ) : g_UI.m_bShowItemNames;
		}
	}
    else
	{
		return (!UIComponentGetVisibleByEnum(UICOMP_PAPERDOLL) &&
				!UIComponentGetVisibleByEnum(UICOMP_INVENTORY));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetAlwaysShowItemNames(
	BOOL bState )
{
	BOOL bCurrent = UIGetAlwaysShowItemNamesState();
	if ( bState == bCurrent )
	{
		return;
	}

	g_UI.m_bAlwaysShowItemNames = bState;


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGetAlwaysShowItemNamesState()
{
	if (AppIsTugboat())
		return g_UI.m_bAlwaysShowItemNames;
    else
	{
		return (!UIComponentGetVisibleByEnum(UICOMP_PAPERDOLL) &&
				!UIComponentGetVisibleByEnum(UICOMP_INVENTORY));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UIGetClosestLabeledItem(
	BOOL *pbInRange)
{
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_UNIT_NAMES);
	if (pComponent)
	{
		UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay(pComponent);
		ASSERT_RETNULL(pNameDisp);

		UNIT *pItem = (pNameDisp->m_idUnitNameUnderCursor != INVALID_ID ? UnitGetById(AppGetCltGame(), pNameDisp->m_idUnitNameUnderCursor) : NULL);
		if (pItem)
		{
			if (pbInRange)
			{
				*pbInRange = pNameDisp->m_fUnitNameUnderCursorDistSq <= PICKUP_RADIUS_SQ;
			}
			return pItem;
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateClosestItem()
{
	UI_COMPONENT * pClosestTooltip = UIComponentGetByEnum(UICOMP_CLOSESTITEM);
	if ( !pClosestTooltip )
	{
		return;
	}

	BOOL bInRange = FALSE;
	UNIT *pItem = NULL;
	if (!AppIsTugboat())
    {
		UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_UNIT_NAMES);
		if (!pComponent)
			return;

	    UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay(pComponent);
	    ASSERT_RETURN(pNameDisp);

		pItem = UIGetClosestLabeledItem(&bInRange);
    }
	else
    {
	    ALTITEMLIST * pClosest = c_FindClosestItem( UIGetControlUnit() );
	    pItem = (pClosest && pClosest->idItem != INVALID_ID ? UnitGetById(AppGetCltGame(), pClosest->idItem) : NULL);
		bInRange = (pClosest ? pClosest->bInPickupRange : FALSE);
    }
	if ( pItem && bInRange)
	{
		UIComponentSetFocusUnit( pClosestTooltip, pItem->unitid );
		UITooltipDetermineContents(pClosestTooltip);
		UITooltipDetermineSize(pClosestTooltip);
		// Put it at the right side of the screen
		float fAppWidth = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX();
		pClosestTooltip->m_Position.m_fX = FLOOR((fAppWidth / 2.0f) - (pClosestTooltip->m_fWidth / 2.0f));
		UIComponentSetVisible( pClosestTooltip, TRUE );
		UI_COMPONENT * label = UIComponentFindChildByName(pClosestTooltip, "closest item msg");
		UILabelSetTextByStringKey(label, GlobalStringGetKey( GS_UI_PICK_UP_TEXT ) );
	}
	else
	{
		UIComponentSetVisible( pClosestTooltip, FALSE );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIShowMerchantScreen(
	UNITID idMerchant)
{
	UI_COMPONENT* merchant_screen = UIComponentGetByEnum(UICOMP_MERCHANTINVENTORY);
	if (!merchant_screen)
	{
		return FALSE;
	}
	UIComponentSetFocusUnit(merchant_screen, idMerchant);
	//return UIShowInventoryScreen(STYLE_MERCHANT, idMerchant);
	return sUIShowInventoryScreen( STYLE_MERCHANT, idMerchant, idMerchant, INVALID_LINK, 0 );
}

// ^^^ Tugboat-specific
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIHideMerchantScreen()
{
	if( AppIsTugboat() )
	{
		if( UIPaneVisible( KPaneMenuMerchant) )
		{
			UIHidePaneMenu( KPaneMenuMerchant );
			return TRUE;
		}
		return FALSE;
	}
	else
	{
		return UIComponentActivate(UICOMP_MERCHANTINVENTORY, FALSE);
	}
}

#define UI_PROC_DISP	(procinited && game && (RandGetFloat(game) < 0.00025f))
void sUIProcImmDisp(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	//UI_COMPONENT *cmp = UIComponentGetByEnum(UICOMP_PLAYERPACK);
	//if (cmp && UIComponentGetActive(cmp))
	//{
	//	int times = data.m_Data1;
	//	UI_INVGRID *pPack = UICastToInvGrid(cmp);

	//	for (int i=0; i < pPack->m_nGridWidth * pPack->m_nGridHeight; i++)
	//	{
	//		if (times == 0)
	//		{
	//			pPack->m_pItemAdj[i].fXOffset = 0.0f;
	//			pPack->m_pItemAdj[i].fYOffset = 0.0f;
	//		}
	//		else
	//		{
	//			pPack->m_pItemAdj[i].fXOffset = RandGetFloat( game, -pPack->m_fCellWidth / 2.0f, pPack->m_fCellWidth / 2.0f );
	//			pPack->m_pItemAdj[i].fYOffset = RandGetFloat( game, -pPack->m_fCellHeight / 2.0f, pPack->m_fCellHeight / 2.0f );
	//		}
	//	}

	//	UIComponentHandleUIMessage(pPack, UIMSG_PAINT, 0, 0);

	//	if (times > 0)
	//		CSchedulerRegisterEventImm(sUIProcImmDisp, CEVENT_DATA(times-1));

	//}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsMerchantScreenUp()
{
	UI_COMPONENT* merchant_screen = UIComponentGetByEnum(UICOMP_MERCHANTINVENTORY);
	if (merchant_screen && UIComponentGetVisible(merchant_screen) && UIComponentGetActive(merchant_screen))
	{
		return TRUE;
	}
	merchant_screen = UIComponentGetByEnum(UICOMP_PANEMENU_MERCHANT);
	if (merchant_screen && UIComponentGetVisible(merchant_screen) && UIComponentGetActive(merchant_screen))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsRecipeScreenUp()
{
	UI_COMPONENT* pComp = UIComponentGetByEnum(UICOMP_RECIPE_CURRENT_PANEL);
	if (pComp && UIComponentGetVisible(pComp) && UIComponentGetActive(pComp))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsStashScreenUp()
{
	UI_COMPONENT* stash_screen = UIComponentGetByEnum(UICOMP_STASH_PANEL);
	if (stash_screen && UIComponentGetVisible(stash_screen) && UIComponentGetActive(stash_screen))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsInventoryPackPanelUp()
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_INVENTORY);
	if (component && UIComponentGetVisible(component))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL UITradeScreenCallbackTimed(
	GAME* game,
	UNIT* unit,
	const struct EVENT_DATA& event_data)
{
	// make sure the reward callback cleanup is done if the panel gets
	// canceled in some way that doesn't trigger the callback
	// (inventory screen may be canceled before the trade
	// screen is up) - cmarch
	if ( AppIsHellgate() &&
		 !UIComponentGetActiveByEnum( UICOMP_NPC_CONVERSATION_DIALOG ) &&
		 TradeIsTrading( unit ) )
		c_RewardCallbackCancel(NULL, 0);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIShowTradeScreen(
	UNITID idTradingWith,
	BOOL bCancelled,
	const TRADE_SPEC *pTradeSpec)
{
	if (idTradingWith != INVALID_ID)
	{
		ASSERTX_RETFALSE( pTradeSpec, "Expected trade spec" );
		GAME *pGame = AppGetCltGame();
		UNIT *pTradingWith = UnitGetById( pGame, idTradingWith );

		UNIT * pPlayer = UIGetControlUnit();
		UNITID idPlayer = UnitGetId(pPlayer);
		UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
		UIComponentSetFocusUnitByEnum(UICOMP_CHARSHEET, idPlayer, TRUE);

		// hide character sheet immediately if it's up
		if (AppIsTugboat())
		{
			UIComponentActivate(UICOMP_SKILLSSHEET, FALSE);
        }

		//UIToggleShowItemNames(FALSE);

		// set inventory style if not explicitly given
		INVENTORY_STYLE eStyleToUse = (INVENTORY_STYLE)pTradeSpec->eStyle;
		if (eStyleToUse == STYLE_INVALID)
		{
			if (UnitIsMerchant( pTradingWith ))
			{
				eStyleToUse = STYLE_MERCHANT;
			}
			else
			{
				eStyleToUse = STYLE_TRADE;
			}
		}

		// when trading with a personal merchant, we are actually just looking
		// at hidden inventory slots on our player so the player themselves is
		// going to be the focus of the merchant store panels
		UNITID idTradeFocus = idTradingWith;
		UNITID idPortraitFocus = idTradingWith;
		if (UnitIsMerchant( pTradingWith ))
		{
			if (UnitMerchantSharesInventory( pTradingWith ) == FALSE)
			{
				UNIT *pPlayer = UIGetControlUnit();
				ASSERTX_RETFALSE( pPlayer, "Expected player" );
				idTradeFocus = UnitGetId( pPlayer );
			}
		}

		if (AppIsHellgate() &&
			eStyleToUse == STYLE_OFFER)
		{
			const WCHAR *puszText = NULL;

			// Use the NPC dialog + reward panel
			NPC_DIALOG_SPEC tSpec;
			tSpec.nOfferDefId = pTradeSpec->nOfferDefinition;

			// get offer string (if any)
			const OFFER_DEFINITION *pOfferDef = OfferGetDefinition( pTradeSpec->nOfferDefinition );
			if (pOfferDef && pOfferDef->nOfferStringKey != INVALID_LINK)
			{
				puszText = StringTableGetStringByIndex( pOfferDef->nOfferStringKey );
			}

			// if no string, use generic reward instructions
			if (puszText == NULL)
			{
				puszText = GlobalStringGet( GS_REWARD_INSTRUCTIONS );
			}

			// set the talking spec for this offer
			tSpec.puszText = puszText;
			tSpec.idTalkingTo = idTradingWith;
			DIALOG_CALLBACK *pCallbackCancel = &tSpec.tCallbackCancel;
			pCallbackCancel->pfnCallback = c_RewardCallbackCancel;

			// make sure the reward callback cleanup is done if the panel gets
			// canceled in some way that doesn't trigger the callback
			// (inventory screen may be canceled before the trade
			// screen is up) - cmarch
			UnitRegisterEventTimed( pTradingWith, UITradeScreenCallbackTimed, NULL, GAME_TICKS_PER_SECOND * 1 );

			// open the NPC dialog
			UIDisplayNPCDialog(AppGetCltGame(), &tSpec);
			return TRUE;

		}

//		 show the inventory screen
		if (sUIShowInventoryScreen( eStyleToUse, idTradeFocus, idPortraitFocus, pTradeSpec->nOfferDefinition, pTradeSpec->nNumRewardTakes))
		{
			//add the player's name we are trading with
			if( AppIsTugboat() )
			{
				UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_TRADE_WINDOW_TITLE);
				if( component )
				{
					const WCHAR *titleIs = StringTableGetStringByKey( "TradeWindowTitle" );
					WCHAR nNewTitle[2048];
					WCHAR nPlayerName[MAX_CHARACTER_NAME];
					PStrCvt(nNewTitle, titleIs, arrsize(nNewTitle));															
					UnitGetName(pTradingWith, nPlayerName, MAX_CHARACTER_NAME, 0);
					PStrReplaceToken( nNewTitle, arrsize(nNewTitle), StringReplacementTokensGet( SR_PLAYER ), nPlayerName );					
					UILabelSetText( component->m_idComponent, nNewTitle );
				}
			}
			return TRUE;
		}
	}
	else
	{
		if (UIComponentActivate(UICOMP_INVENTORY, FALSE))
		{
			return TRUE;

		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsGunModScreenUp()
{
	UI_COMPONENT *pGunMod = UIComponentGetByEnum(UICOMP_GUNMODSCREEN);
	if (pGunMod && UIComponentGetVisible(pGunMod))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMerchantShowPaperdoll( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{

		UI_COMPONENT *pPaperdoll = UIComponentGetByEnum(UICOMP_PAPERDOLL);
		if (!pPaperdoll)
			return UIMSG_RET_NOT_HANDLED;
		UNIT *pPlayer = UIGetControlUnit();
		if (!pPlayer)
			return UIMSG_RET_NOT_HANDLED;
		UIComponentActivate(pPaperdoll, TRUE);
		UIComponentSetFocusUnit(pPaperdoll, UnitGetId(pPlayer));
		//UISwitchComponents(UICOMP_MERCHANTWELCOME, UICOMP_PAPERDOLL);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMerchantClose( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{
		UIHideMerchantScreen();

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMerchantOnInactivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	if (component->m_eState == UISTATE_INACTIVE)
		return UIMSG_RET_NOT_HANDLED;

	c_TradeCancel();
	UIPanelOnInactivate( component, msg, wParam, lParam );
	return UIMSG_RET_HANDLED;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMerchantOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT* merchant_screen = component;
	if (!merchant_screen)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT *pUnit = UIComponentGetFocusUnit(merchant_screen);

	// if the merchant screen is the player, then we're
	// using a personal store - but we want to use
	// the tab preferences from the actual trader
	if( pUnit == UIGetControlUnit() )
	{
		pUnit = TradeGetTradingWith( pUnit );
	}
	if( pUnit && pUnit->pUnitData->nMerchantStartingPane != INVALID_ID )
	{
		UI_PANELEX* panel = UICastToPanel(merchant_screen);
		UIPanelSetTab( panel, pUnit->pUnitData->nMerchantStartingPane );
	}
	UIPartyDisplayOnPartyChange(component, msg, wParam, lParam);
	return UIMSG_RET_HANDLED;
}

// ^^^ Tugboat-specific
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEatMsgHandler(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEatMsgHandlerCheckBounds(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHandleMsgHandler(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIIgnoreMsgHandler(
								 UI_COMPONENT* component,
								 int msg,
								 DWORD wParam,
								 DWORD lParam)
{

	return UIMSG_RET_NOT_HANDLED;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCleanUpMessage(
	UINT uimsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uimsg)
	{
	case UIMSG_SETTARGETSTATE:
	case UIMSG_SETCONTROLSTATE:
		{
			if (lParam)
			{
				UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)lParam;
				GFREE(AppGetCltGame(), pData);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISendMessage(
	UINT nWinMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	int nUIMessage = UITranslateMessage(nWinMsg);

	return UIHandleUIMessage(nUIMessage, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHandleUIMessage(
	UINT nMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	if (nMsg == UIMSG_MOUSEMOVE		||
		nMsg == UIMSG_MOUSEWHEEL)
	{
		UIGenerateMouseMoveMessages();
	}

	if (nMsg == UIMSG_LBUTTONDOWN	||
		nMsg == UIMSG_LBUTTONUP		||
		nMsg == UIMSG_RBUTTONDOWN	||
		nMsg == UIMSG_RBUTTONUP		||
		nMsg == UIMSG_MBUTTONDOWN	||
		nMsg == UIMSG_MBUTTONUP		)
	{
		int x = LOWORD(lParam);
		int y = HIWORD(lParam);
		UIGenerateMouseClickMessages(g_UI.m_Components, nMsg, x, y);
	}

	UI_MSG_RETVAL eResult = UIComponentHandleUIMessage(g_UI.m_Cursor, nMsg, wParam, lParam, UIGetMouseOverrideComponent() != NULL);
	if (ResultStopsProcessing(eResult))
	{
		goto done;
	}

	if (UIGetMouseOverrideComponent() &&
		(nMsg == UIMSG_MOUSEMOVE		||
		nMsg == UIMSG_LBUTTONDOWN	||
		nMsg == UIMSG_LBUTTONUP		||
		nMsg == UIMSG_LBUTTONDBLCLK ||
		nMsg == UIMSG_RBUTTONDOWN	||
		nMsg == UIMSG_RBUTTONUP		||
		nMsg == UIMSG_RBUTTONDBLCLK ||
		nMsg == UIMSG_MBUTTONDOWN	||
		nMsg == UIMSG_MBUTTONUP		||
		nMsg == UIMSG_MOUSEWHEEL))
	{
		eResult = UIComponentHandleUIMessage(UIGetMouseOverrideComponent(), nMsg, wParam, lParam, TRUE);
		goto done;
	}

	if (UIGetKeyboardOverrideComponent() &&
		(nMsg == UIMSG_KEYDOWN	||
		 nMsg == UIMSG_KEYUP	||
		 nMsg == UIMSG_KEYCHAR))
	{
		eResult = UIComponentHandleUIMessage(UIGetKeyboardOverrideComponent(), nMsg, wParam, lParam, TRUE);
		goto done;
	}

	if (IsStatMessage(nMsg) && g_UI.m_bIgnoreStatMessages)
	{
		eResult = UIMSG_RET_END_PROCESS;
		goto done;
	}

	if (gUIMESSAGE_DATA[nMsg].m_fpDefaultFunctionPtr)
	{
		// if this is a message that every component is going to have a handler for (since there is a default)
		// then we should just send it to each in tree order.  This saves us some handler space.
		UIComponentHandleUIMessage(g_UI.m_Cursor, nMsg, wParam, lParam, TRUE);
		UIComponentHandleUIMessage(g_UI.m_Components, nMsg, wParam, lParam, TRUE);
	}
	else
	{
		// this method is for messages that only some components are going to have handlers for.
		//   those handlers are collected in these sparse, ordered lists that are iterated sequentially
		PList<UI_MSGHANDLER *>::USER_NODE *pNode = NULL;
		while ((pNode = g_UI.m_listMessageHandlers[nMsg].GetNext(pNode)) != NULL)
		{
			eResult = sUIComponentHandleUIMessage(pNode->Value->m_pComponent, pNode->Value, nMsg, wParam, lParam);
			if (ResultStopsProcessing(eResult))
			{
				goto done;
			}
		}
	}

done:
	sCleanUpMessage(nMsg, wParam, lParam);
	return eResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIProcess(
	void)
{
	GAME *game = AppGetCltGame();
	if (UI_PROC_DISP)
	{
		sUIProcImmDisp(game, CEVENT_DATA(20), 0);
	}

	UIRenderToLocal(g_UI.m_Components, g_UI.m_Cursor);

	if (UICursorGetActive())
	{
		if (g_UI.m_eHoverState == UIHOVERSTATE_NONE && AppCommonGetAbsTime() >= g_UI.m_timeNextHoverCheck)
		{
			g_UI.m_eHoverState = UIHOVERSTATE_SHORT;
			UISendHoverMessage(FALSE);
		}
		else if (g_UI.m_eHoverState == UIHOVERSTATE_SHORT && AppCommonGetAbsTime() - g_UI.m_timeNextHoverCheck > g_UI.m_nHoverDelayLong)
		{
			g_UI.m_eHoverState = UIHOVERSTATE_LONG;
			UISendHoverMessage(TRUE);
		}
	}

	UILogLCDUpdate();

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoItemWindowRepaints(
	void)
{

	// for now just paint the active bar ... my grand theory though would be
	// to repaint the inventory squares, or trade squares, any of that stuff because
	// I would like to show directly on an item some status information ... for
	// instance have it grey the ui square if the item is not currently usable (which
	// for now we only show in the active bar) -Colin
	UIComponentHandleUIMessage( UIComponentGetByEnum( UICOMP_ACTIVEBAR ), UIMSG_PAINT, 0, 0 );

	// we don't need any more repaints
	g_UI.m_bNeedItemWindowRepaint = FALSE;

}

static BOOL sUINeedsCursor()
{
	if (AppIsTugboat())
		return TRUE;

	PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
	while ((pNode = g_UI.m_listUseCursorComponents.GetNext(pNode)) != NULL)
	{
		if (UIComponentGetVisible(pNode->Value))
		{
			return TRUE;
		}
	}

	if (InputIsCommandKeyDown(CMD_SHOW_ITEMS))
	{
		while ((pNode = g_UI.m_listUseCursorComponentsWithAlt.GetNext(pNode)) != NULL)
		{
			if (UIComponentGetVisible(pNode->Value))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIProcess(
	void)
{
	PERF(UI);

	if (g_UI.m_bRequestingReload)
	{
		g_UI.m_bRequestingReload = false;
		UI_RELOAD_CALLBACK * pCallback = g_UI.m_pfnReloadCallback;
		UIReload();
		g_UI.m_pfnReloadCallback = pCallback;
		return;
	}

	// CHB 2007.04.03 - We always call this callback if it's there,
	// even if there was no reload, because the callers has a
	// difficult time knowing in advance whether there will be a
	// reload or not.  Easier to just make it appear to the caller
	// that the reload happens every time.
	if (g_UI.m_pfnReloadCallback != 0)
	{
		UI_RELOAD_CALLBACK * pCallback = g_UI.m_pfnReloadCallback;
		g_UI.m_pfnReloadCallback = 0;
		(*pCallback)();
		return;
	}

	if (!g_UI.m_bPostLoadExecuted &&
		g_UI.m_nFilesToLoad == 0)
	{
		UILoadComplete();
	}

	if (e_UIGetFontTextureResetRequest())
	{
		UIFontsResetTextureData();
		UISetNeedToRenderAll();
	}

	if (g_UI.m_bFullRepaintRequested)
	{
		UIComponentHandleUIMessage( g_UI.m_Components, UIMSG_PAINT, 0, 0 );
		UIComponentHandleUIMessage( g_UI.m_Cursor, UIMSG_PAINT, 0, 0 );
		g_UI.m_bFullRepaintRequested = FALSE;
		g_UI.m_bNeedItemWindowRepaint = FALSE;  // we've painted everything, can't possibly need the smaller subset of item ui pieces
	}

	// do any queued item window paint
	if (g_UI.m_bNeedItemWindowRepaint == TRUE)
	{
		sDoItemWindowRepaints();
	}

	// paint the activebar every (other) frame
	static BOOL bAlternateFrames = FALSE;
	if (!g_UI.m_bFullRepaintRequested)
	{
		if (bAlternateFrames)
		{
			UI_COMPONENT *pActiveBar = UIComponentGetByEnum(UICOMP_ACTIVEBAR);
			if (pActiveBar && UIComponentGetVisible(pActiveBar))
			{
				UIComponentHandleUIMessage(pActiveBar, UIMSG_PAINT, 0, 0);
			}
			pActiveBar = UIComponentGetByEnum(UICOMP_RTS_ACTIVEBAR);
			if (pActiveBar && UIComponentGetVisible(pActiveBar))
			{
				UIComponentHandleUIMessage(pActiveBar, UIMSG_PAINT, 0, 0);
			}
		}
		bAlternateFrames = !bAlternateFrames;
	}

	// KCK: For PvP games, we want to update the scoreboard. Do this every other frame.
	static BOOL bScoreboardUp = FALSE;
	GAME *pGame = AppGetCltGame();
	if (GameIsPVP(pGame))
	{
		if (bAlternateFrames)
		{
			UI_COMPONENT *pScoreboard = UIComponentGetByEnum(UICOMP_SCOREBOARD);
			UI_COMPONENT *pScoreboardShort = UIComponentGetByEnum(UICOMP_SCOREBOARD_SHORT);
			if (pScoreboard && pScoreboardShort)
			{
				// KCK: If neither are active, activate the short scoreboard now.
				if (!UIComponentGetVisible(pScoreboard) && !UIComponentGetVisible(pScoreboardShort))
				{
					UIComponentSetActive(pScoreboardShort, TRUE);
					UIComponentSetVisible(pScoreboardShort, TRUE);
				}
				// Paint whichever one is visible
				if (UIComponentGetVisible(pScoreboard))
					UIComponentHandleUIMessage(pScoreboard, UIMSG_PAINT, 0, 0);
				else
					UIComponentHandleUIMessage(pScoreboardShort, UIMSG_PAINT, 0, 0);
			}
		}
		bScoreboardUp = TRUE;
	}
	else if (bScoreboardUp)
	{
		UI_COMPONENT *pScoreboard = UIComponentGetByEnum(UICOMP_SCOREBOARD);
		if (pScoreboard)
		{
			UIComponentSetActive(pScoreboard, FALSE);
			UIComponentSetVisible(pScoreboard, FALSE);
		}
		UI_COMPONENT *pScoreboardShort = UIComponentGetByEnum(UICOMP_SCOREBOARD_SHORT);
		if (pScoreboardShort)
		{
			UIComponentSetActive(pScoreboardShort, FALSE);
			UIComponentSetVisible(pScoreboardShort, FALSE);
		}
		bScoreboardUp = FALSE;
	}

//	DWORD dwAnimTime = 0;
	if (g_UI.m_Cursor)
	{
		if (sUINeedsCursor())
		{
			UIComponentActivate(g_UI.m_Cursor, TRUE);
		}
		else
		{
			BOOL bWasActive = UIComponentGetActive(g_UI.m_Cursor);
			UIComponentActivate(g_UI.m_Cursor, FALSE);
			if (bWasActive)		// if we're de-activating the cursor
			{
				g_UI.m_pLastMouseDownComponent = NULL;
				UIGenerateMouseMoveMessages();
				UIChatFadeOut(FALSE);
			}
		}
	}

	{
		PERF(UI_PROCESS);
		sUIProcess();
	}

	{
		PERF(UI_SCHED);
		CSchedulerProcess(AppGetCltGame());
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIComponentGetIdByName(
	const char* name)
{
	UI_COMPONENT* component = UIComponentFindChildByName(g_UI.m_Components, name);
	if (!component)
	{
		return INVALID_ID;
	}
	return component->m_idComponent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetTargetUnit(
	UNITID idUnit,
	BOOL bSelectionInRange)
{
	if (UICanSelectTarget( idUnit ))
	{
		// if there's no target
		if (!AppIsTugboat())
		{
		    if (idUnit == INVALID_ID &&
				!UITestFlag(UI_FLAG_ALT_TARGET_INFO))
		    {
			    UNIT *pCurrentTarget = UnitGetById(AppGetCltGame(), g_UI.m_idTargetUnit);
			    if (pCurrentTarget && !IsUnitDeadOrDying(pCurrentTarget))
			    {
				    // keep the last target around for half a second
				    DWORD dwTimeDiff = TimeGetElapsed(g_UI.m_timeLastValidTarget, AppCommonGetCurTime());
				    // time diff is in milliseconds
				    if ( dwTimeDiff < 500)
				    {
					    return;
				    }
			    }
	        }
		}
        else
        {
		    if(  UIGetTargetUnit() && UIGetTargetUnitId() != idUnit )
		    {
			    // turn off fullbright if we already had a unit selected
			    c_AppearanceSetFullbright( c_UnitGetModelIdThirdPerson( UIGetTargetUnit() ),FALSE );
		    }

		}

		g_UI.m_timeLastValidTarget = AppCommonGetCurTime();
	}
	else
	{
		idUnit = INVALID_ID;
		bSelectionInRange = FALSE;
	}
	if( AppIsTugboat() )
	{
		UIUpdatePlayerInteractPanel( UIGetTargetUnit() );
	}

	if (idUnit != INVALID_ID &&
		UITestFlag(UI_FLAG_ALT_TARGET_INFO))
	{
		GAME *pGame = AppGetCltGame();
		UI_COMPONENT *pMinTarget = UIComponentGetByEnum(UICOMP_MINIMAL_TARGET);
		if (pMinTarget)
		{
			UNIT *pUnit = UnitGetById(pGame, idUnit);
			const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
			if (pCameraInfo)
			{
				VECTOR vEye = CameraInfoGetPosition( pCameraInfo );
				VECTOR vScreenPos;
				float fControlDistSq, fCameraDistSq, fScale;
				UnitDisplayGetOverHeadPositions(
					pUnit,
					GameGetControlUnit(AppGetCltGame()),
					vEye,
					1.0f,
					vScreenPos,
					fScale,
					fControlDistSq,
					fCameraDistSq);

				float fRightNudge = (float)pMinTarget->m_dwParam;
				pMinTarget->m_Position.m_fX = (vScreenPos.fX + 1.0f) * 0.5f * UIDefaultWidth();
				pMinTarget->m_Position.m_fY = (-vScreenPos.fY + 1.0f) * 0.5f * UIDefaultHeight();
				pMinTarget->m_Position.m_fY -= pMinTarget->m_fHeight;
				pMinTarget->m_Position.m_fX += fRightNudge * fScale;

				// don't let it go off the screen (for when you're real close
				if (pMinTarget->m_Position.m_fX < UIDefaultWidth() * 0.1f)
					pMinTarget->m_Position.m_fX = (UIDefaultWidth() * 0.1f);
				if (pMinTarget->m_Position.m_fX + pMinTarget->m_fWidth > UIDefaultWidth() * 0.9f)
					pMinTarget->m_Position.m_fX = (UIDefaultWidth() * 0.9f) - pMinTarget->m_fWidth;
				if (pMinTarget->m_Position.m_fY < UIDefaultHeight() * 0.1f)
					pMinTarget->m_Position.m_fY = (UIDefaultHeight() * 0.1f);
				if (pMinTarget->m_Position.m_fY + pMinTarget->m_fHeight > UIDefaultHeight() * 0.9f)
					pMinTarget->m_Position.m_fY = (UIDefaultHeight() * 0.9f) - pMinTarget->m_fHeight;

				if (pMinTarget->m_pFirstChild)
				{
					// change the distance meter
					UIComponentHandleUIMessage(pMinTarget->m_pFirstChild, UIMSG_PAINT, 0, 0);

					//mmmm.... hard-codey....
					pMinTarget->m_pFirstChild->m_dwColor = (bSelectionInRange ? GFXCOLOR_GREEN : GFXCOLOR_RED );
				}
				UISetNeedToRender(pMinTarget);
			}
			//UI_RECT rect;
			//UIGetUnitScreenBox(UnitGetById(AppGetCltGame(), idUnit), rect);

			//pMinTarget->m_Position.m_fX = rect.m_fX2;
			//pMinTarget->m_Position.m_fY = rect.m_fY1 - pMinTarget->m_fHeight;
		}
	}

	if (idUnit == g_UI.m_idTargetUnit &&
		bSelectionInRange == g_UI.m_bTargetInRange)
	{
		return;
	}

	g_UI.m_idTargetUnit = idUnit;
	g_UI.m_bTargetInRange = bSelectionInRange;
	UIHandleUIMessage(UIMSG_SETTARGETUNIT, idUnit, bSelectionInRange);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIGetTargetUnitId(
	void)
{
	return g_UI.m_idTargetUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* UIGetTargetUnit(
	void)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return NULL;
	}
	if (g_UI.m_idTargetUnit == INVALID_ID)
	{
		return NULL;
	}
	return UnitGetById(game, g_UI.m_idTargetUnit);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetAltTargetUnit(
	UNITID idUnit )
{
	if (!UICanSelectTarget( idUnit ))
	{
		idUnit = INVALID_ID;
	}

	g_UI.m_idAltTargetUnit = idUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIGetAltTargetUnitId(
	void)
{
	return g_UI.m_idAltTargetUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* UIGetAltTargetUnit(
	void)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return NULL;
	}
	if (g_UI.m_idAltTargetUnit == INVALID_ID)
	{
		return NULL;
	}
	return UnitGetById(game, g_UI.m_idAltTargetUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// vvv Tugboat-specific
void UISetClickedTargetUnit(
					 UNITID idUnit,
					 BOOL bSelectionInRange,
					 BOOL bLeft,
					 int nSkill )
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return;
	}
	if ( idUnit != g_UI.m_idClickedTargetUnit && nSkill != INVALID_ID )
	{
		c_SkillControlUnitChangeTarget( UnitGetById(game, idUnit) );
	}
	g_UI.m_idClickedTargetUnit = idUnit;
	g_UI.m_nClickedTargetSkill = nSkill;
	g_UI.m_nClickedTargetLeft = bLeft;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIGetClickedTargetUnitId(
						 void)
{
	return g_UI.m_idClickedTargetUnit;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* UIGetClickedTargetUnit( void  )
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return NULL;
	}
	if (g_UI.m_idClickedTargetUnit == INVALID_ID)
	{
		return NULL;
	}

	return UnitGetById(game, g_UI.m_idClickedTargetUnit);
}

int UIGetClickedTargetSkill( void )
{
	return g_UI.m_nClickedTargetSkill;
}

int UIGetClickedTargetLeft( void )
{
	return g_UI.m_nClickedTargetLeft;
}

// ^^^ Tugboat-specific
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static struct MINIGAME_FRAME {
	BOOL	m_bAchieved;
	int		m_nGoalCategory;
	int		m_nGoalType;
	CHAR	m_szFrameName[64];
} sgMinigameFrame[] =
{
	{ FALSE, MINIGAME_DMG_TYPE,			DAMAGE_TYPE_PHYSICAL,	    "minigameslot physical"},
	{ FALSE, MINIGAME_DMG_TYPE,			DAMAGE_TYPE_FIRE,		    "minigameslot fire"},
	{ FALSE, MINIGAME_DMG_TYPE,			DAMAGE_TYPE_ELECTRICITY,	"minigameslot electric"},
	{ FALSE, MINIGAME_DMG_TYPE,			DAMAGE_TYPE_SPECTRAL,	    "minigameslot spectral"},
	{ FALSE, MINIGAME_DMG_TYPE,			DAMAGE_TYPE_TOXIC,		    "minigameslot toxic"},

	{ FALSE, MINIGAME_KILL_TYPE,		UNITTYPE_BEAST,				"minigameslot beast"},
	{ FALSE, MINIGAME_KILL_TYPE, 		UNITTYPE_UNDEAD,			"minigameslot necro"},
	{ FALSE, MINIGAME_KILL_TYPE, 		UNITTYPE_DEMON,				"minigameslot demon"},
	{ FALSE, MINIGAME_KILL_TYPE, 		UNITTYPE_SPECTRAL,			"minigameslot specter"},

	{ FALSE, MINIGAME_CRITICAL, 		0,							"minigameslot crithit"},

	{ TRUE,  MINIGAME_DMG_TYPE,			DAMAGE_TYPE_PHYSICAL,	    "minigameicon physical"},
	{ TRUE,  MINIGAME_DMG_TYPE,			DAMAGE_TYPE_FIRE,		    "minigameicon fire"},
	{ TRUE,  MINIGAME_DMG_TYPE,			DAMAGE_TYPE_ELECTRICITY,	"minigameicon electric"},
	{ TRUE,  MINIGAME_DMG_TYPE,			DAMAGE_TYPE_SPECTRAL,	    "minigameicon spectral"},
	{ TRUE,  MINIGAME_DMG_TYPE,			DAMAGE_TYPE_TOXIC,		    "minigameicon toxic"},

	{ TRUE,  MINIGAME_KILL_TYPE, 		UNITTYPE_BEAST,				"minigameicon beast"},
	{ TRUE,  MINIGAME_KILL_TYPE, 		UNITTYPE_UNDEAD,			"minigameicon necro"},
	{ TRUE,  MINIGAME_KILL_TYPE, 		UNITTYPE_DEMON,				"minigameicon demon"},
	{ TRUE,  MINIGAME_KILL_TYPE, 		UNITTYPE_SPECTRAL,			"minigameicon specter"},

	{ TRUE,  MINIGAME_CRITICAL, 		0,							"minigameicon crithit"},

	{ FALSE, MINIGAME_QUEST_FINISH, 	0,							 "minigameslot quest"},
	{ FALSE, MINIGAME_ITEM_TYPE, 		UNITTYPE_ARMOR,				 "minigameslot armor"},
	{ FALSE, MINIGAME_ITEM_TYPE, 		UNITTYPE_MELEE,				 "minigameslot melee"},
	{ FALSE, MINIGAME_ITEM_TYPE, 		UNITTYPE_GUN,				 "minigameslot gun"},
	{ FALSE, MINIGAME_ITEM_TYPE, 		UNITTYPE_MOD,				 "minigameslot mod"},
	{ FALSE, MINIGAME_ITEM_MAGIC_TYPE,	0,							 "minigameslot magicitem"},

	{ TRUE, MINIGAME_QUEST_FINISH, 		0,							 "minigameicon quest"},
	{ TRUE, MINIGAME_ITEM_TYPE, 		UNITTYPE_ARMOR,				 "minigameicon armor"},
	{ TRUE, MINIGAME_ITEM_TYPE, 		UNITTYPE_MELEE,				 "minigameicon melee"},
	{ TRUE, MINIGAME_ITEM_TYPE, 		UNITTYPE_GUN,				 "minigameicon gun"},
	{ TRUE, MINIGAME_ITEM_TYPE, 		UNITTYPE_MOD,				 "minigameicon mod"},
	{ TRUE, MINIGAME_ITEM_MAGIC_TYPE,	0,							 "minigameicon magicitem"},

};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMinigameSlotOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	int nSlot = (int)component->m_dwParam;
	ASSERT_RETVAL(nSlot >= 0 && nSlot < NUM_MINIGAME_SLOTS, UIMSG_RET_NOT_HANDLED);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETVAL(pTexture, UIMSG_RET_NOT_HANDLED);

	BOOL bAchieved = FALSE;
	int nGoalCategory = INVALID_LINK;
	int nGoalType = INVALID_LINK;
	int nNeeded = 0;

	if (AppGetState() == APP_STATE_IN_GAME)
	{

		GAME *pGame = AppGetCltGame();
		if (pGame)
		{
			UNIT *pPlayer = GameGetControlUnit(pGame);
			ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

			UNIT_ITERATE_STATS_RANGE(pPlayer, STATS_MINIGAME_CATEGORY_NEEDED, statsentry, ii, NUM_MINIGAME_SLOTS)
			{
				if (ii==nSlot)
					//&&
					//(msg != UIMSG_SETCONTROLSTAT ||
					// lParam == statsentry[ii].stat))
				{
					PARAM param = statsentry[ii].GetParam();
					nGoalCategory = StatGetParam(STATS_MINIGAME_CATEGORY_NEEDED, param, 0);
					nGoalType = StatGetParam(STATS_MINIGAME_CATEGORY_NEEDED, param, 1);
					nNeeded = statsentry[ii].value;
					bAchieved = (nNeeded == 0);

					UIComponentRemoveAllElements(component);

					for (int i=0; i < arrsize(sgMinigameFrame); i++)
					{
						if (sgMinigameFrame[i].m_bAchieved == bAchieved &&
							sgMinigameFrame[i].m_nGoalCategory == nGoalCategory &&
							sgMinigameFrame[i].m_nGoalType == nGoalType)
						{
							UI_TEXTURE_FRAME* pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, sgMinigameFrame[i].m_szFrameName);
							UIComponentSetAlpha(component, (bAchieved ? 255 : 85));
							UIComponentAddElement(component, UIComponentGetTexture(component), pFrame);

							if (nNeeded > 1)
							{
								WCHAR str[32];
								PStrPrintf(str, arrsize(str), L"%d", nNeeded);
								UIComponentAddTextElement(component, NULL, UIComponentGetFont(component), UIComponentGetFontSize(component), str, UI_POSITION(), UI_MAKE_COLOR(192, GFXCOLOR_WHITE), NULL, UIALIGN_CENTER, &UI_SIZE(component->m_fWidth, component->m_fHeight), TRUE);
							}

							if (msg == UIMSG_SETCONTROLSTAT &&
								lParam == statsentry[ii].stat)
							{
								UIComponentBlink(component);
							}
						}
					}
					//return UIMSG_RET_HANDLED;
				}
			}
			UNIT_ITERATE_STATS_END;
		}

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMinigameUpdateTimeLeft(
	GAME* game, 
	const CEVENT_DATA& data,
	DWORD)
{
	int nComponentID = (int)data.m_Data1;
	if (nComponentID < 0)
		return;

	UI_COMPONENT* component = UIComponentGetById(nComponentID);
	if (!component)
		return;

	int nTicksLeft = (int)component->m_dwData;

	// display time left
	WCHAR szBuff[32] = { L'\0' };

	UI_LABELEX *pLabel = UICastToLabel(component);
	if (nTicksLeft < 5 * GAME_TICKS_PER_SECOND * SECONDS_PER_MINUTE)
	{
		int nDurationSeconds = (nTicksLeft + GAME_TICKS_PER_SECOND - 1) / GAME_TICKS_PER_SECOND;
		LanguageGetTimeString(szBuff, arrsize(szBuff), nDurationSeconds);

		// this will change the color based on the time left
		pLabel->m_bSelected = (nTicksLeft < GAME_TICKS_PER_SECOND * SECONDS_PER_MINUTE);

	}

	UILabelSetText(pLabel, szBuff);

	// decrement the counter
	nTicksLeft -= GAME_TICKS_PER_SECOND;
	component->m_dwData = (DWORD)MAX(nTicksLeft, 0);

	if (CSchedulerIsValidTicket(component->m_tMainAnimInfo.m_dwAnimTicket))
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}

	component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 1000, sMinigameUpdateTimeLeft, CEVENT_DATA((DWORD_PTR)component->m_idComponent));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMinigameOnSetTicksLeft(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (AppGetState() == APP_STATE_IN_GAME)
	{

		GAME *pGame = AppGetCltGame();
		if (pGame)
		{
			UNIT *pPlayer = GameGetControlUnit(pGame);
			ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

			int nTicks = UnitGetStat(pPlayer, STATS_MINIGAME_TICKS_LEFT);
			component->m_dwData = (DWORD)nTicks;

			if (CSchedulerIsValidTicket(component->m_tMainAnimInfo.m_dwAnimTicket))
			{
				CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
			}

			component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(sMinigameUpdateTimeLeft, CEVENT_DATA((DWORD_PTR)component->m_idComponent));
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIUpdateAnimFrame(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	int nComponentID = (int)data.m_Data1;
	if (nComponentID < 0)
		return;

	UI_COMPONENT* component = UIComponentGetById(nComponentID);
	if (!component)
		return;

	if (!component->m_pFrameAnimInfo)
		return;

	component->m_pFrameAnimInfo->m_nCurrentFrame++;
	if (component->m_pFrameAnimInfo->m_nCurrentFrame > component->m_pFrameAnimInfo->m_nNumAnimFrames-1)
		component->m_pFrameAnimInfo->m_nCurrentFrame = 0;

	component->m_pFrame = component->m_pFrameAnimInfo->m_pAnimFrames[component->m_pFrameAnimInfo->m_nCurrentFrame];

	// special case
	if (UIComponentIsBar(component))
	{
		// so it repaints
		UIComponentRemoveAllElements(component);
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	if (component->m_pFrameAnimInfo->m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_pFrameAnimInfo->m_dwAnimTicket);
	}

	component->m_pFrameAnimInfo->m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + component->m_pFrameAnimInfo->m_dwFrameAnimMSDelay, sUIUpdateAnimFrame, data);
}

//----------------------------------------------------------------------------
// XML FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// GENERIC XML File Open
//----------------------------------------------------------------------------
char * UIXmlOpenFile(
	const char * filename,
	CMarkup & xml,
	const char* tag,
	BOOL bLoadFromLocalizedPak)
{
	char fullfilename[MAX_XML_STRING_LENGTH];
	PStrPrintf(fullfilename, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_UI_XML, filename);
	PStrReplaceExtension(fullfilename, MAX_XML_STRING_LENGTH, fullfilename, DEFINITION_FILE_EXTENSION);

	DECLARE_LOAD_SPEC(spec, fullfilename);

#if ISVERSION(DEVELOPMENT)
	if (g_UI.m_bReloadDirect && AppCommonGetDirectLoad())		// so this is here for rapidly editing the xml file and reloading the UI while the game is running for quicker development
	{
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	}
#endif

	if (AppIsFillingPak())
	{
		spec.flags |= PAKFILE_LOAD_FORCE_FROM_DISK;
	}

	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	spec.pakEnum = (bLoadFromLocalizedPak ? PAK_LOCALIZED : PAK_DEFAULT);
	char* buf = (char*)PakFileLoadNow(spec);
	ASSERTV_RETNULL( buf, "Error loading file: '%s'", fullfilename );

	if (!xml.SetDoc(buf, fullfilename))
	{
		ASSERTV(0, "Error parsing file: '%s'", fullfilename);
		FREE(NULL, buf);
		return NULL;
	}
	if (!xml.FindElem(tag))
	{
		ASSERTV(0, "Error parsing file: '%s', expected tag: '%s'", fullfilename, tag);
		FREE(NULL, buf);
		return NULL;
	}

	return buf;
}

//----------------------------------------------------------------------------
// ATLAS loading: load texture file
//----------------------------------------------------------------------------
UIX_TEXTURE* UITextureLoadTextureFile(
	CMarkup& xml,
	BOOL bLoadFromLocalizedPak)
{
	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
	UI_COMPONENT *component = &tmp;
	// !!!!

	int nEmptyTexWidth = 0;
	int nEmptyTexHeight = 0;
	BOOL bMask = FALSE;

	char filename[DEFAULT_FILE_WITH_PATH_SIZE] = "";
	xml.ResetChildPos();
	if (!xml.FindChildElem("file"))
	{
		XML_LOADINT("emptytexwidth", nEmptyTexWidth, 0);
		XML_LOADINT("emptytexheight", nEmptyTexHeight, 0);
		XML_LOADINT("mask", bMask, FALSE);

		nEmptyTexWidth  = e_GetPow2Resolution( nEmptyTexWidth );
		nEmptyTexHeight = e_GetPow2Resolution( nEmptyTexHeight );

		int nMaxWidth, nMaxHeight;
		e_GetMaxTextureSizes(nMaxWidth, nMaxHeight);
		float fRatioX = (float)AppCommonGetWindowWidth() / UIDefaultWidth();
		float fRatioY = (float)AppCommonGetWindowHeight() / UIDefaultHeight();
		if (fRatioX > 1.0f || fRatioY > 1.0f)
		{
			if (nEmptyTexHeight < nMaxHeight)
				nEmptyTexHeight *= 2;
			else
				nEmptyTexWidth *= 2;
		}

		nEmptyTexWidth = MIN(nMaxWidth, nEmptyTexWidth);
		nEmptyTexHeight = MIN(nMaxHeight, nEmptyTexHeight);

	}
	else
	{
		xml.GetChildData(filename, DEFAULT_FILE_WITH_PATH_SIZE);
	}

	ASSERTX_RETNULL(filename[0] ||
		(nEmptyTexWidth != 0 && nEmptyTexHeight != 0),
		"No filename found for UI texture and no empty dimensions given.");

	int nChunkSize = 0;
	XML_LOADINT("chunksize", nChunkSize, 16);

	//Disable chunking for now
	nChunkSize = 0;

	//int nTextureFormat = 0;
	//XML_LOADENUM("texformat", nTextureFormat, , "ARGB");


	int nTextureId;

	if ( filename[0] )
	{
		nTextureId = DefinitionGetIdByFilename(DEFINITION_GROUP_TEXTURE, filename, -1, TRUE, TRUE, (bLoadFromLocalizedPak ? PAK_LOCALIZED : PAK_DEFAULT));
		if (!e_TextureIsValidID( nTextureId ))
		{
			V( e_UITextureLoadTextureFile( nTextureId, filename, nChunkSize, nTextureId, FALSE, bLoadFromLocalizedPak ) );
		}
	}
	else
	{
		V( e_UITextureCreateEmpty( nEmptyTexWidth, nEmptyTexHeight, nTextureId, bMask ) );
	}

	ASSERTV_RETNULL( nTextureId != INVALID_ID, "Error creating texture '%s' from file", filename);

	if (!e_TextureIsValidID(nTextureId))
	{
		return NULL;
	}

	UIX_TEXTURE* texture = (UIX_TEXTURE*)MALLOCZ(NULL, sizeof(UIX_TEXTURE));
	if (!texture)
	{
		return NULL;
	}

	PStrCopy(texture->m_pszFileName, filename, DEFAULT_FILE_WITH_PATH_SIZE);
	texture->m_nTextureId = nTextureId;

	int nWidth, nHeight;
	V( e_TextureGetOriginalResolution( nTextureId, nWidth, nHeight ) );
	texture->m_fTextureHeight = (float)nHeight;
	texture->m_fTextureWidth  = (float)nWidth;

	XML_LOADFLOAT("fontareax1", texture->m_rectFontArea.m_fX1, 0.0f);
	XML_LOADFLOAT("fontareay1", texture->m_rectFontArea.m_fY1, 0.0f);
	XML_LOADFLOAT("fontareax2", texture->m_rectFontArea.m_fX2, texture->m_fTextureWidth);
	XML_LOADFLOAT("fontareay2", texture->m_rectFontArea.m_fY2, texture->m_fTextureHeight);

	XML_LOADINT("renderpriority", texture->m_nDrawPriority, 0);

	return texture;
}


//----------------------------------------------------------------------------
// ATLAS loading: load texture file
//----------------------------------------------------------------------------
// vvv Tugboat-specific
UIX_TEXTURE* UITextureLoadTextureFileRaw(
	const char* filename,
	BOOL bAllowAsync,
	BOOL bLoadFromLocalizedPak)
{

	int nTextureId = DefinitionGetIdByFilename(DEFINITION_GROUP_TEXTURE, filename, -1, FALSE, TRUE);


	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;


	if (!e_TextureIsValidID( nTextureId ))
	{
		int nReturnedID;
		V( e_UITextureLoadTextureFile( nTextureId, filename, 0, nReturnedID, bAllowAsync, bLoadFromLocalizedPak ) );//nChunkSize);
		ASSERT( nReturnedID == nTextureId );
		nReturnedID = nTextureId;	// just in case
	}
	else // using one that's already out there - make sure to refcount it!
	{
		e_TextureAddRef( nTextureId );
	}

	ASSERTV_RETNULL( nTextureId != INVALID_ID, "Error creating texture '%s' from file", filename);

	if (!e_TextureIsValidID(nTextureId))
	{
		return NULL;
	}

	UIX_TEXTURE* texture = (UIX_TEXTURE*)MALLOCZ(NULL, sizeof(UIX_TEXTURE));
	if (!texture)
	{
		return NULL;
	}

	PStrCopy(texture->m_pszFileName, filename, DEFAULT_FILE_WITH_PATH_SIZE);
	texture->m_nTextureId = nTextureId;

	return texture;
}
// ^^^ Tugboat-specific

//----------------------------------------------------------------------------
// ATLAS loading: load texture frames
//----------------------------------------------------------------------------
UI_TEXTURE_FRAME* UITextureLoadTextureFrame(
	UIX_TEXTURE* texture,
	CMarkup& xml)
{
	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
	UI_COMPONENT *component = &tmp;
	// !!!!

	char name[256] = "";
	XML_LOADSTRING("name", name, "", 256);
	//if (name[0] == 0)
	//{
	//	XML_LOADSTRING("linkto", name, "", 256);
		ASSERTV_RETNULL( name[0] != 0, "Invalid frame definition for texture atlas");
	//	bLink = TRUE;
	//}

	UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)MALLOCZ(NULL, sizeof(UI_TEXTURE_FRAME));

	float fX1 = 0.0f, fY1 = 0.0f;
	XML_LOADFLOAT("x", fX1, 0.0f);
	XML_LOADFLOAT("y", fY1, 0.0f);
	XML_LOADFLOAT("w", frame->m_fWidth, 0.0f);
	XML_LOADFLOAT("h", frame->m_fHeight, 0.0f);

	if (frame->m_fHeight == 0 || frame->m_fWidth == 0)
	{
		FREE(NULL, frame);
		return NULL;
	}

	float fX2 = fX1 + frame->m_fWidth;
	float fY2 = fY1 + frame->m_fHeight;

	XML_LOADFLOAT("xoffs", frame->m_fXOffset, 0.0f);
	XML_LOADFLOAT("yoffs", frame->m_fYOffset, 0.0f);
	XML_LOAD("defaultcolor", frame->m_dwDefaultColor, DWORD, GFXCOLOR_WHITE);
	int r = -1, g = -1, b = -1, a = -1;
	XML_LOADINT("red", r, -1);
	XML_LOADINT("green", g, -1);
	XML_LOADINT("blue", b, -1);
	XML_LOADINT("alpha", a, 255);
	if (a != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0x00ffffff) + (a << 24);
	}
	if (r != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0xff00ffff) + (r << 16);
	}
	if (g != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0xffff00ff) + (g << 8);
	}
	if (b != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0xffffff00) + b;
	}

	XML_LOADBOOL("hasmask", frame->m_bHasMask, FALSE);

	V_DO_FAILED( e_UITextureAddFrame(frame, texture->m_nTextureId, UI_RECT(fX1, fY1, fX2, fY2) ) )
	{
		return NULL;
	}

	if (texture->m_XMLWidthRatio != 1.0f ||
		texture->m_XMLHeightRatio != 1.0f )
	{
		frame->m_fWidth *= texture->m_XMLWidthRatio;
		frame->m_fHeight *= texture->m_XMLHeightRatio;
		frame->m_fXOffset *= texture->m_XMLWidthRatio;
		frame->m_fYOffset *= texture->m_XMLHeightRatio;

		for (int i=0; i < frame->m_nNumChunks; i++)
		{
			frame->m_pChunks[i].m_fWidth *= texture->m_XMLWidthRatio;
			frame->m_pChunks[i].m_fHeight *= texture->m_XMLHeightRatio;
			frame->m_pChunks[i].m_fXOffset *= texture->m_XMLWidthRatio;
			frame->m_pChunks[i].m_fYOffset *= texture->m_XMLHeightRatio;
		}
	}

	if (!texture->m_pFrames)
	{
		texture->m_pFrames = StrDictionaryInit(2, TRUE, UITextureFramesDelete);
	}

	StrDictionaryAdd(texture->m_pFrames, name, frame);

	return frame;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UITextureLoadTextureFrameLinks(
	UIX_TEXTURE* texture,
	CMarkup& xml)
{
	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
	UI_COMPONENT *component = &tmp;
	// !!!!

	// First see if there's a nextframe, 'coz most frames won't have one
	char szNextName[256] = "";
	XML_LOADSTRING("nextframe", szNextName, "", 256);
	if (szNextName[0] == 0)
	{
		return FALSE;
	}

	DWORD dwDelay = 1;
	XML_GETINTATTRIB("nextframe", "delay", dwDelay, 1);

	char name[256] = "";
	XML_LOADSTRING("name", name, "", 256);
	ASSERTV_RETFALSE( name[0] != 0, "Invalid frame definition for texture atlas");

	UI_TEXTURE_FRAME* pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, name);
	ASSERT_RETFALSE(pFrame);

	pFrame->m_pNextFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, szNextName);
	ASSERT_RETFALSE(pFrame->m_pNextFrame);
	pFrame->m_dwNextFrameDelay = dwDelay;

	return TRUE;
}

//----------------------------------------------------------------------------
// ATLAS loading: load texture frames
//----------------------------------------------------------------------------
// vvv Tugboat-specific
UI_TEXTURE_FRAME* UITextureCreateTextureFrame(
	UIX_TEXTURE* texture,
	const UI_XML & tUIXml)
{

	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;

	char name[256] = "icon";

	UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)MALLOCZ(NULL, sizeof(UI_TEXTURE_FRAME));

	float fX1, fY1;
	fX1 = 0;
	fY1 = 0;
	frame->m_fWidth = texture->m_fTextureWidth;
	frame->m_fHeight = texture->m_fTextureHeight;

	float fX2 = fX1 + frame->m_fWidth;
	float fY2 = fY1 + frame->m_fHeight;

	frame->m_fXOffset = 0;
	frame->m_fYOffset = 0;
	frame->m_dwDefaultColor = GFXCOLOR_WHITE;
	int r, g, b;
	r = -1;
	g = -1;
	b = -1;

	if (r != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0xff00ffff) + (r << 16);
	}
	if (g != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0xffff00ff) + (g << 8);
	}
	if (b != -1)
	{
		frame->m_dwDefaultColor = (frame->m_dwDefaultColor & 0xffffff00) + b;
	}

	V_DO_FAILED( e_UITextureAddFrame(frame, texture->m_nTextureId, UI_RECT(fX1, fY1, fX2, fY2) ) )
	{
		return NULL;
	}

	if (tUIXml.m_fXmlWidthRatio != 1.0f ||
		tUIXml.m_fXmlHeightRatio != 1.0f )
	{
		frame->m_fWidth *= tUIXml.m_fXmlWidthRatio;
		frame->m_fHeight *= tUIXml.m_fXmlHeightRatio;
		frame->m_fXOffset *= tUIXml.m_fXmlWidthRatio;
		frame->m_fYOffset *= tUIXml.m_fXmlHeightRatio;

		for (int i=0; i < frame->m_nNumChunks; i++)
		{
			frame->m_pChunks[i].m_fWidth *= tUIXml.m_fXmlWidthRatio;
			frame->m_pChunks[i].m_fHeight *= tUIXml.m_fXmlHeightRatio;
			frame->m_pChunks[i].m_fXOffset *= tUIXml.m_fXmlWidthRatio;
			frame->m_pChunks[i].m_fYOffset *= tUIXml.m_fXmlHeightRatio;
		}
	}

	if (!texture->m_pFrames)
	{
		texture->m_pFrames = StrDictionaryInit(2, TRUE, UITextureFramesDelete);
	}

	StrDictionaryAdd(texture->m_pFrames, name, frame);

	return frame;
}
// ^^^ Tugboat-specific

//----------------------------------------------------------------------------
// ATLAS loading: new texture frame
//----------------------------------------------------------------------------
UI_TEXTURE_FRAME* UITextureNewTextureFrame(
	UIX_TEXTURE* texture,
	const char* name,
	UI_TEXTURE_FRAME* pTemplate )
{
	UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)MALLOCZ(NULL, sizeof(UI_TEXTURE_FRAME));

	if ( pTemplate )
		*frame = *pTemplate;

	if (!texture->m_pFrames)
	{
		texture->m_pFrames = StrDictionaryInit(2, TRUE, UITextureFramesDelete);
	}

	StrDictionaryAdd(texture->m_pFrames, name, frame);

	return frame;
}


//----------------------------------------------------------------------------
// ATLAS loading: base load
//----------------------------------------------------------------------------
// vvv Tugboat-specific

UIX_TEXTURE* UITextureLoadRaw(
	const char* filename,
	int nWidthBasis /*= 1600*/,
	int nHeightBasis /*= 1200*/,
	BOOL bAllowAsync /*= FALSE*/ )
{
	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
//	UI_COMPONENT *component = &tmp;
	// !!!!
	// FIXME!
	if (g_UI.m_bWidescreen)
	{
		if( (float)nHeightBasis / (float)nWidthBasis == .75 )
		{
			nHeightBasis = (int)( (float)nHeightBasis / 1.2f );
		}
	}
	ASSERT_RETNULL(filename);

//	tUIXml.m_fLastFontY = 0.0f;
/*
	XML_LOADINT("widthbasis", tUIXml.m_nXmlWidthBasis, UI_DEFAULT_WIDTH);
	XML_LOADINT("heightbasis", tUIXml.m_nXmlHeightBasis, UI_DEFAULT_HEIGHT);
	if (g_UI.m_bWidescreen)
	{
		XML_LOADINT("ws_widthbasis", tUIXml.m_nXmlWidthBasis, tUIXml.m_nXmlWidthBasis);
		XML_LOADINT("ws_heightbasis", tUIXml.m_nXmlHeightBasis, tUIXml.m_nXmlHeightBasis);
	}

	tUIXml.m_fXmlWidthRatio = round(UI_DEFAULT_WIDTH / (float)tUIXml.m_nXmlWidthBasis, 3);
	tUIXml.m_fXmlHeightRatio = round(UI_DEFAULT_HEIGHT / (float)tUIXml.m_nXmlHeightBasis, 3);

	tUIXml.m_fLastFontY = 0.0f;

	xml.ResetChildPos();*/

	UIX_TEXTURE* texture = NULL;

	ONCE
	{
		texture = UITextureLoadTextureFileRaw(filename, bAllowAsync, FALSE);
		if (!texture)
		{
			break;
		}

		UITextureLoadFinalize( texture );
	}

//	FREE(NULL, buf);
	return texture;
}
// ^^^ Tugboat-specific

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UITextureLoadFinalize( UIX_TEXTURE * pUITexture )
{
	if ( ! pUITexture )
		return FALSE;
	if ( ! e_TextureIsLoaded( pUITexture->m_nTextureId, TRUE ) )
		return FALSE;


	int nWidth, nHeight;
	V( e_TextureGetOriginalResolution( pUITexture->m_nTextureId, nWidth, nHeight ) );
	pUITexture->m_fTextureHeight = (float)nHeight;
	pUITexture->m_fTextureWidth  = (float)nWidth;


	if( !pUITexture->m_pFrames )
	{
		// CML 2007.1.20: HACKISH!  These numbers are as hacky here as in the arguments to UITextureLoadRaw, from
		//   whence they were derived.
		int nWidthBasis = 1600;
		int nHeightBasis = 1200;

		if (g_UI.m_bWidescreen)
		{
			if( (float)nHeightBasis / (float)nWidthBasis == .75 )
			{
				nHeightBasis = (int)( (float)nHeightBasis / 1.2f );
			}
		}

		UI_XML tUIXml;
		tUIXml.m_nXmlWidthBasis = nWidthBasis;
		tUIXml.m_nXmlHeightBasis = nHeightBasis;
		tUIXml.m_fXmlWidthRatio = round(UIDefaultWidth() / (float)tUIXml.m_nXmlWidthBasis, 3);
		tUIXml.m_fXmlHeightRatio = round(UIDefaultHeight() / (float)tUIXml.m_nXmlHeightBasis, 3);

		UITextureCreateTextureFrame(pUITexture, tUIXml);

		if (pUITexture->m_pFrames)
		{
			StrDictionarySort(pUITexture->m_pFrames);
		}
	}

	/*xml.ResetChildPos();
	while (xml.FindChildElem("font"))
	{
	xml.IntoElem();

	UITextureLoadTextureFont(texture, xml);

	xml.OutOfElem();
	}*/
	if (pUITexture->m_pFonts)
	{
		StrDictionarySort(pUITexture->m_pFonts);
	}

	// if applicable, update the texture
	V( e_UIRestoreTextureFile(pUITexture->m_nTextureId) );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CALLBACK EnumFontFamExProc(
	CONST LOGFONTA* lpelfe,
	CONST TEXTMETRICA* lpntme,
	DWORD FontType,
	LPARAM lParam)
{
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EnumFonts(
	HDC hdc)
{
	LOGFONT logfont;
	structclear(logfont);
	logfont.lfCharSet = ANSI_CHARSET;
	EnumFontFamiliesEx(hdc, &logfont, EnumFontFamExProc, 0, 0);
}


//----------------------------------------------------------------------------
// ATLAS loading: load texture font
//----------------------------------------------------------------------------
UIX_TEXTURE_FONT* UITextureLoadTextureFont(
	UIX_TEXTURE* texture,
	CMarkup& xml)
{
	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
	UI_COMPONENT *component = &tmp;
	// !!!!



	char name[256] = "";
	XML_LOADSTRING("name", name, "", 256);

	ASSERTV_RETNULL( name[0] != 0, "Invalid font definition for texture atlas");

	UIX_TEXTURE_FONT* font = (UIX_TEXTURE_FONT*)MALLOCZ(NULL, sizeof(UIX_TEXTURE_FONT));

	int nNumCharacters = UI_CHARACTERS_IN_FONT - ' ';
	REF(nNumCharacters);

	XML_LOADSTRING("systemname", font->m_szSystemName, "", DEFAULT_FILE_WITH_PATH_SIZE);
	XML_LOADSTRING("asciisystemname", font->m_szAsciiFontName, "", DEFAULT_FILE_WITH_PATH_SIZE);
	XML_LOADSTRING("localpath", font->m_szLocalPath, "", DEFAULT_FILE_WITH_PATH_SIZE);
	XML_LOADSTRING("asciilocalpath", font->m_szAsciiLocalPath, "", DEFAULT_FILE_WITH_PATH_SIZE);
	XML_LOADBOOL("italic", font->m_bItalic, FALSE);
	XML_LOADBOOL("bold", font->m_bBold, FALSE);

	font->m_nWeight = 0;//XML_LOADINT("weight", font->m_nWeight, FW_NORMAL);

	XML_LOADFLOAT("widthratio", font->m_fWidthRatio, 1.0f);
	XML_LOADINT("extraspacing", font->m_nExtraSpacing, 0);

	V( font->Init(texture) );

	if (!texture->m_pFonts)
	{
		texture->m_pFonts = StrDictionaryInit(1, TRUE, UITextureFontsDelete);
	}
	StrDictionaryAdd(texture->m_pFonts, name, font);

#if (ISVERSION(DEBUG_VERSION))
	char szBuf[256];
	PStrRemoveEntirePath(szBuf, arrsize(szBuf), font->m_szLocalPath);
	PStrPrintf(font->m_szDebugName, DEFAULT_FILE_WITH_PATH_SIZE, "%s:%s", name, szBuf );
#endif

	return font;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//const char* UIGetInvAtlasName(
//	void)
//{
//	return sgszInvAtlasFileName;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIXMLLoadEnum(
	CMarkup& xml,
	LPCSTR szKey,
	int& nVar,
	STR_DICT *pStrTable,
	int nDefault)
{
	xml.ResetChildPos();
	if (xml.FindChildElem(szKey))
	{
		const char * sValue = xml.GetChildData();
		if (PStrIsNumeric(sValue))
		{
			nVar = (int)PStrToInt(sValue);
			return;
		}
		else
		{
			while (pStrTable && pStrTable->str)
			{
				if (PStrICmp(sValue, pStrTable->str)==0)
				{
					nVar = pStrTable->value;
					return;
				}
				pStrTable++;
			}
		}
	}

	nVar = nDefault;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIXMLLoadColor(
	CMarkup& xml,
	UI_COMPONENT *component,
	LPCSTR szBaseKey,
	DWORD& dwVar,
	DWORD dwDefault)
{
	char szKey[256];

	if (!component->m_bReference)
	{
		dwVar = dwDefault;
	}

	PStrPrintf(szKey, 256, "%scolor", szBaseKey);

	xml.ResetChildPos();
	if (xml.FindChildElem(szKey))
	{
		char szValue[256] = "";
		PStrCopy(szValue, xml.GetChildData(), arrsize(szValue));
		int nColor = ExcelGetLineByStrKey(DATATABLE_FONTCOLORS, szValue);
		if (nColor != INVALID_LINK)
		{
			dwVar = GetFontColor(nColor);
		}
		else
		{
			// just load it as a hex string
			XML_LOAD(szKey, dwVar, DWORD, dwDefault);
		}
	}
	else
	{

		int r = -1, g = -1, b = -1, a = -1;
		PStrPrintf(szKey, 256, "%sred", szBaseKey);
		XML_LOADINT(szKey, r, -1);
		PStrPrintf(szKey, 256, "%sgreen", szBaseKey);
		XML_LOADINT(szKey, g, -1);
		PStrPrintf(szKey, 256, "%sblue", szBaseKey);
		XML_LOADINT(szKey, b, -1);
		PStrPrintf(szKey, 256, "%salpha", szBaseKey);
		XML_LOADINT(szKey, a, -1);
		if (a != -1)
		{
			dwVar = (dwVar & 0x00ffffff) + (a << 24);
		}
		if (r != -1)
		{
			dwVar = (dwVar & 0xff00ffff) + (r << 16);
		}
		if (g != -1)
		{
			dwVar = (dwVar & 0xffff00ff) + (g << 8);
		}
		if (b != -1)
		{
			dwVar = (dwVar & 0xffffff00) + b;
		}
	}
}

//----------------------------------------------------------------------------
// ATLAS loading: base load
//----------------------------------------------------------------------------
UIX_TEXTURE* UITextureLoadDefinition(
	const char* filename,
	BOOL bLoadFromLocalizedPak)
{
	// !!! TEMP TEMP TEMP
	UI_COMPONENT tmp;
	tmp.m_bReference = FALSE;
	UI_COMPONENT *component = &tmp;
	// !!!!

	ASSERT_RETNULL(filename);

	CMarkup xml;
	char* buf = UIXmlOpenFile(filename, xml, "atlas", bLoadFromLocalizedPak);
	if (!buf)
	{
		return NULL;
	}

	int nTexWidthBasis = UI_DEFAULT_WIDTH;
	int nTexHeightBasis = UI_DEFAULT_HEIGHT;
	XML_LOADINT("widthbasis", nTexWidthBasis, UI_DEFAULT_WIDTH);
	XML_LOADINT("heightbasis", nTexHeightBasis, UI_DEFAULT_HEIGHT);
	if (g_UI.m_bWidescreen)
	{
		XML_LOADINT("ws_widthbasis", nTexWidthBasis, nTexWidthBasis);
		XML_LOADINT("ws_heightbasis", nTexHeightBasis, nTexHeightBasis);
	}

	xml.ResetChildPos();

	UIX_TEXTURE* texture = NULL;

	ONCE
	{
		texture = UITextureLoadTextureFile(xml, bLoadFromLocalizedPak);
		if (!texture)
		{
			break;
		}

		texture->m_XMLWidthRatio = round(UIDefaultWidth() / (float)nTexWidthBasis, 3);
		texture->m_XMLHeightRatio = round(UIDefaultHeight() / (float)nTexHeightBasis, 3);

		xml.ResetChildPos();
		while (xml.FindChildElem("frame"))
		{
			xml.IntoElem();

			UITextureLoadTextureFrame(texture, xml);

			xml.OutOfElem();
		}
		if (texture->m_pFrames)
		{
			StrDictionarySort(texture->m_pFrames);
		}

		xml.ResetChildPos();
		while (xml.FindChildElem("frame"))
		{
			xml.IntoElem();

			UITextureLoadTextureFrameLinks(texture, xml);

			xml.OutOfElem();
		}

		xml.ResetChildPos();
		while (xml.FindChildElem("font"))
		{
			xml.IntoElem();

			UITextureLoadTextureFont(texture, xml);

			xml.OutOfElem();
		}
		if (texture->m_pFonts)
		{
			StrDictionarySort(texture->m_pFonts);
		}

		// if applicable, update the texture
		V( e_UIRestoreTextureFile(texture->m_nTextureId) );
	}

	FREE(NULL, buf);
	return texture;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PCODE UILoadCode(
	const char* szCode)
{
	if (szCode && szCode[0])
	{
		if (g_UI.m_nCodeCurr + CODE_PAGE_SIZE >= g_UI.m_nCodeSize)
		{
			g_UI.m_nCodeSize += CODE_PAGE_SIZE;
			g_UI.m_pCodeBuffer = (BYTE*)REALLOC(NULL, g_UI.m_pCodeBuffer, g_UI.m_nCodeSize);
			if (g_UI.m_nCodeCurr == 0)	// offset 0 is NULL_CODE
			{
				g_UI.m_nCodeCurr = 1;
			}
		}

		BYTE * end = ScriptParseEx(NULL, szCode, g_UI.m_pCodeBuffer + g_UI.m_nCodeCurr, g_UI.m_nCodeSize - g_UI.m_nCodeCurr, ERROR_LOG, "error parsing script in ui xml");
		if (end == NULL)
		{
			return NULL_CODE;
		}
		else if (end == g_UI.m_pCodeBuffer + g_UI.m_nCodeCurr)
		{
			return NULL_CODE;
		}
		else
		{
			PCODE retval = g_UI.m_nCodeCurr;
			g_UI.m_nCodeCurr = SIZE_TO_INT(end - g_UI.m_pCodeBuffer);
			return retval;
		}
	}
	return NULL_CODE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PCODE UIXmlLoadCode(
	CMarkup& xml,
	const char* key,
	PCODE codeDefault)
{
	xml.ResetChildPos();
	if (xml.FindChildElem(key))
	{
		return UILoadCode(xml.GetChildData());
	}
	return codeDefault;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// on function tags
struct UI_XML_ONFUNCTAG
{
	const char *		m_szOnTagName;
	int					m_eUIMessage;
	int					m_eType;				// 1 = sound event
												// 2 = stat event
};

UI_XML_ONFUNCTAG gUIXmlFunctionTags[] =
{
	{ "OnPaint",				UIMSG_PAINT,				0	},
	{ "OnMouseMove",			UIMSG_MOUSEMOVE,			0	},
	{ "OnMouseOver",			UIMSG_MOUSEOVER,			0	},
	{ "OnMouseOverSnd",			UIMSG_MOUSEOVER,			1	},
	{ "OnMouseLeave",			UIMSG_MOUSELEAVE,			0	},
	{ "OnMouseLeaveSnd",		UIMSG_MOUSELEAVE,			1	},
	{ "OnLButtonDown",			UIMSG_LBUTTONDOWN,			0	},
	{ "OnLButtonDownSnd",		UIMSG_LBUTTONDOWN,			1	},
	{ "OnMButtonDown",			UIMSG_MBUTTONDOWN,			0	},
	{ "OnMButtonDownSnd",		UIMSG_MBUTTONDOWN,			1	},
	{ "OnRButtonDown",			UIMSG_RBUTTONDOWN,			0	},
	{ "OnRButtonDownSnd",		UIMSG_RBUTTONDOWN,			1	},
	{ "OnLButtonUp",			UIMSG_LBUTTONUP,			0	},
	{ "OnLButtonUpSnd",			UIMSG_LBUTTONUP,			1	},
	{ "OnMButtonUp",			UIMSG_MBUTTONUP,			0	},
	{ "OnMButtonUpSnd",			UIMSG_MBUTTONUP,			1	},
	{ "OnRButtonUp",			UIMSG_RBUTTONUP,			0	},
	{ "OnRButtonUpSnd",			UIMSG_RBUTTONUP,			1	},
	{ "OnLClick",				UIMSG_LBUTTONCLICK,			0	},
	{ "OnLClickSnd",			UIMSG_LBUTTONCLICK,			1	},
	{ "OnRClick",				UIMSG_RBUTTONCLICK,			0	},
	{ "OnRClickSnd",			UIMSG_RBUTTONCLICK,			1	},
	{ "OnLDoubleClick",			UIMSG_LBUTTONDBLCLK,		0	},
	{ "OnRDoubleClick",			UIMSG_RBUTTONDBLCLK,		0	},
	{ "OnMouseWheel",			UIMSG_MOUSEWHEEL,			0	},
	{ "OnKeyDown",				UIMSG_KEYDOWN,				0	},
	{ "OnKeyDownSnd",			UIMSG_KEYDOWN,				1	},
	{ "OnKeyUp",				UIMSG_KEYUP,				0	},
	{ "OnKeyUpSnd",				UIMSG_KEYUP,				1	},
	{ "OnKeyChar",				UIMSG_KEYCHAR,				0	},
	{ "OnInventoryChange",		UIMSG_INVENTORYCHANGE,		0	},
	{ "OnSetHoverUnit",			UIMSG_SETHOVERUNIT,			0	},
	{ "OnActivate",				UIMSG_ACTIVATE,				0	},
	{ "OnActivateSnd",			UIMSG_ACTIVATE,				1	},
	{ "OnInactivate",			UIMSG_INACTIVATE,			0	},
	{ "OnInactivateSnd",		UIMSG_INACTIVATE,			1	},
	{ "OnSetControlUnit",		UIMSG_SETCONTROLUNIT,		0	},
	{ "OnSetTargetUnit",		UIMSG_SETTARGETUNIT,		0	},
	{ "OnControlUnitGetHit",	UIMSG_CONTROLUNITGETHIT,	0	},
	{ "OnSetControlStat",		UIMSG_SETCONTROLSTAT,		2	},
	{ "OnSetTargetStat",		UIMSG_SETTARGETSTAT,		2	},
	{ "OnSetFocusStat",			UIMSG_SETFOCUSSTAT,			2	},
	{ "OnToggleDebugDisplay",	UIMSG_TOGGLEDEBUGDISPLAY,	0	},
	{ "OnPlayerDeath",			UIMSG_PLAYERDEATH,			0	},
	{ "OnRestart",				UIMSG_RESTART,				0	},
	{ "OnPostActivate",			UIMSG_POSTACTIVATE,			0	},
	{ "OnPostActivateSnd",		UIMSG_POSTACTIVATE,			1	},
	{ "OnPostInactivate",		UIMSG_POSTINACTIVATE,		0	},
	{ "OnPostInactivateSnd",	UIMSG_POSTINACTIVATE,		1	},
	{ "OnMouseHover",			UIMSG_MOUSEHOVER,			0	},
	{ "OnMouseHoverLong",		UIMSG_MOUSEHOVERLONG,		0	},
	{ "OnScroll",				UIMSG_SCROLL,				0	},
	{ "OnSetControlState",		UIMSG_SETCONTROLSTATE,		0	},
	{ "OnSetTargetState",		UIMSG_SETTARGETSTATE,		0	},
	{ "OnSetFocus",				UIMSG_SETFOCUS,				0	},
	{ "OnPostVisible",			UIMSG_POSTVISIBLE,			0	},
	{ "OnPostInvisible",		UIMSG_POSTINVISIBLE,		0	},
	{ "OnPartyChange",			UIMSG_PARTYCHANGE,			0	},
	{ "OnAnimate",				UIMSG_ANIMATE,				0	},
	{ "OnLBSelect",				UIMSG_LB_ITEMSEL,			0	},
	{ "OnLBHover",				UIMSG_LB_ITEMHOVER,			0	},
	{ "OnPostCreate",			UIMSG_POSTCREATE,			0	},
	{ "OnSetFocusUnit",			UIMSG_SETFOCUSUNIT,			0	},
	{ "OnCursorActivate",		UIMSG_CURSORACTIVATE,		0	},
	{ "OnCursorInactivate",		UIMSG_CURSORINACTIVATE,		0	},
	{ "OnWardrobeChange",		UIMSG_WARDROBECHANGE,		0	},
	{ "OnFullTextReveal",		UIMSG_FULLTEXTREVEAL,		0	},
	{ "OnResize",				UIMSG_RESIZE,				0	},
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int UIXmlComponentCompare(
	const void * a,
	const void * b)
{
	UI_XML_COMPONENT * A = (UI_XML_COMPONENT *)a;
	UI_XML_COMPONENT * B = (UI_XML_COMPONENT *)b;
	ASSERT_RETVAL(A && A->m_szComponentName, -1);
	ASSERT_RETVAL(B && B->m_szComponentName, 1);
	return PStrCmp(A->m_szComponentName, B->m_szComponentName);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int UIXmlFunctionTagCompare(
	const void * a,
	const void * b)
{
	UI_XML_ONFUNCTAG * A = (UI_XML_ONFUNCTAG *)a;
	UI_XML_ONFUNCTAG * B = (UI_XML_ONFUNCTAG *)b;
	ASSERT_RETVAL(A && A->m_szOnTagName, -1);
	ASSERT_RETVAL(B && B->m_szOnTagName, 1);
	return PStrCmp(A->m_szOnTagName, B->m_szOnTagName);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int UIXmlFunctionCompare(
	const void * a,
	const void * b)
{
	UI_XML_ONFUNC * A = (UI_XML_ONFUNC *)a;
	UI_XML_ONFUNC * B = (UI_XML_ONFUNC *)b;
	ASSERT_RETVAL(A && A->m_szFunctionName, -1);
	ASSERT_RETVAL(B && B->m_szFunctionName, 1);
	return PStrCmp(A->m_szFunctionName, B->m_szFunctionName);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIXmlInit(
	void)
{
	{
		qsort(gUIComponentTypes, arrsize(gUIComponentTypes), sizeof(UI_XML_COMPONENT), UIXmlComponentCompare);
		qsort(gUIXmlFunctionTags, arrsize(gUIXmlFunctionTags), sizeof(UI_XML_ONFUNCTAG), UIXmlFunctionTagCompare);
		qsort(gpUIXmlFunctions, gnNumUIXmlFunctions / sizeof(UI_XML_ONFUNC), sizeof(UI_XML_ONFUNC), UIXmlFunctionCompare);
	}
	for (int ii = 0; ii < arrsize(gUIComponentTypes); ii++)
	{
		gUIComponentTypes[ii].m_nCount = 0;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_XML_COMPONENT * UIXmlComponentFind(
	const char * str)
{
	ASSERT_RETNULL(str);
	UI_XML_COMPONENT key;
	key.m_szComponentName = str;

	return (UI_XML_COMPONENT *)bsearch(&key, gUIComponentTypes, arrsize(gUIComponentTypes), sizeof(UI_XML_COMPONENT), UIXmlComponentCompare);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_XML_COMPONENT* UIXmlComponentFind(
	int nComponentType )
{
	for (int i=0; i < arrsize(gUIComponentTypes); i++)
	{
		if (gUIComponentTypes[i].m_eComponentType == nComponentType)
		{
			return &(gUIComponentTypes[i]);
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_XML_ONFUNCTAG * UIXmlFunctionTagFind(
	const char * str)
{
	ASSERT_RETNULL(str);

	UI_XML_ONFUNCTAG key;
	key.m_szOnTagName = str;

	return (UI_XML_ONFUNCTAG *)bsearch(&key, gUIXmlFunctionTags, arrsize(gUIXmlFunctionTags), sizeof(UI_XML_ONFUNCTAG), UIXmlFunctionTagCompare);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_XML_ONFUNC* UIXmlFunctionFind(
	const char* str)
{
	UI_XML_ONFUNC key;
	key.m_szFunctionName = str;

	return (UI_XML_ONFUNC*)bsearch(&key, gpUIXmlFunctions, gnNumUIXmlFunctions / sizeof(UI_XML_ONFUNC), sizeof(UI_XML_ONFUNC), UIXmlFunctionCompare);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentLoadExtras(
	CMarkup& xml,
	CMarkup* includexml,
	UI_COMPONENT* component,
	const UI_XML& tUIXml)
{
	char tag_name[256];

	// load default handlers for this component type
	UI_XML_COMPONENT *pDefinition = UIXmlComponentFind(component->m_eComponentType);
	ASSERT_RETURN(pDefinition);
	for (int i=0; i<NUM_UI_MESSAGES; i++)
	{
		if (pDefinition->m_pDefaultUiMsgHandlerTbl[i])
		{
			UIAddMsgHandler(component, i, pDefinition->m_pDefaultUiMsgHandlerTbl[i], INVALID_LINK, TRUE);
		}
	}

	xml.ResetChildPos();

	// now load custom message handlers
	while (xml.FindChildElem())
	{
		// see if this was a child component
		UI_COMPONENT *pChild = UIXmlComponentLoad(xml, includexml, component, tUIXml);

		PStrCopy(tag_name, xml.GetChildTagName(), 256);
		if (tag_name[0] == 0)
		{
			continue;
		}

		if (!pChild)
		{
			// see if it's a message handler
			UI_XML_ONFUNCTAG* tagdef = UIXmlFunctionTagFind(tag_name);
			if (tagdef)
			{
				int msg = tagdef->m_eUIMessage;
				switch (tagdef->m_eType)
				{
				case 0:		// standard message
					{
						if (PStrICmp("none", xml.GetChildData()) == 0)
						{
							sUIRemoveMsgHandler(component, msg);
						}
						else
						{
							UI_XML_ONFUNC* funcdef = UIXmlFunctionFind(xml.GetChildData());
							if (!funcdef)
							{
								trace("Error: unrecognized ui function: %s()\n", xml.GetChildData());
								//ASSERTN("BPlunkett", FALSE);
							}
							else
							{
								UIAddMsgHandler(component, msg, funcdef->m_fpFunctionPtr, INVALID_LINK);
							}
						}
					}
					break;
				case 1:			// sound event
					{
						int idSound = ExcelGetLineByStringIndex(NULL, DATATABLE_SOUNDS, xml.GetChildData());
						if (idSound != INVALID_ID)
						{
							c_SoundLoad(idSound);	// be sure to pre-load the sound so it plays the first time

							BOOL bStop = FALSE;
							if (xml.HasChildAttrib("stop"))
							{
								bStop = (BOOL)PStrToInt(xml.GetChildAttrib("stop"));
							}
							sUIAddSoundMsgHandler(component, msg, idSound, bStop);
						}
					}
					break;
				case 2:			// stats msg
					{
						UI_XML_ONFUNC* funcdef = UIXmlFunctionFind(xml.GetChildData());
						if (!funcdef)
						{
							trace("Error: unrecognized ui function: %s()\n", xml.GetChildData());
						}
						else
						{
							char szStat[256] = "";
							int nStat = INVALID_LINK;
							if (xml.HasChildAttrib("stat"))
							{
								PStrCopy(szStat, xml.GetChildAttrib("stat"), 256);
								nStat = ExcelGetLineByStringIndex(AppGetCltGame(), DATATABLE_STATS, szStat);
								ASSERT_BREAK(nStat != INVALID_LINK);
							}
							UIAddMsgHandler(component, msg, funcdef->m_fpFunctionPtr, nStat);
						}
					}
				}
			}
		}
	}
}

static BOOL sCheckTagForLocationCommon(
	CMarkup& xml,
	const char *szAttribName,
	int nType)
{
	if (xml.HasChildAttrib(szAttribName))
	{
		if (AppCommonGetForceLoadAllLanguages())
			return TRUE;

		char szVal[256] = "";
		PStrCopy(szVal, xml.GetChildAttrib(szAttribName), arrsize(szVal));

		int nVal = INVALID_LINK;
		const char * buf = szVal;
		char szSubtoken[256];
		int buflen = (int)PStrLen(szVal) + 1;
		int len;

		buf = PStrTok(szSubtoken, buf, ", ", (int)arrsize(szSubtoken), buflen, &len);
		while (buf)
		{
			BOOL bMatch = TRUE;
			const char *szKey = szSubtoken;
			if (szSubtoken[0] == '!')
			{
				bMatch = FALSE;
				szKey = &(szSubtoken[1]);
			}

			if (nType == 0)
				nVal = LanguageGetByName(AppGameGet(), szKey);
			else if (nType == 1)
				nVal = RegionGetByName(AppGameGet(), szKey);
			else if (nType == 2)
				nVal = SKUGetByName(AppGetCltGame(), szKey);

			if (AppTestDebugFlag(ADF_FILL_LOCALIZED_PAK_BIT) && g_UI.m_nFillPakSKU != INVALID_LINK)
			{
				// load language-specific components into the localized pak
				if (nType == 0 && SKUContainsLanguage(g_UI.m_nFillPakSKU, (LANGUAGE)nVal, FALSE) )
				{
					return bMatch;
				}
				else if (nType == 1 && SKUContainsRegion(g_UI.m_nFillPakSKU, (WORLD_REGION)nVal) )
				{
					return bMatch;
				}
				else if (nType == 2 && nVal == g_UI.m_nFillPakSKU )
				{
					return bMatch;
				}
			}
			else
			{
				if (nType == 0 && nVal == LanguageGetCurrent())
				{
					return bMatch;
				}
				else if (nType == 1 && nVal == RegionGetCurrent())
				{
					return bMatch;
				}
				else if (nType == 2 && nVal == SKUGetCurrent())
				{
					return bMatch;
				}
			}

			buf = PStrTok(szSubtoken, buf, ", ", arrsize(szSubtoken), buflen, &len);
		}

		return FALSE;
	}

	return TRUE;
}


BOOL UIXmlCheckTagForLocation(
	CMarkup& xml)
{
	if (!sCheckTagForLocationCommon(xml, "lang", 0))
	{
		return FALSE;
	}
	if (!sCheckTagForLocationCommon(xml, "region", 1))
	{
		return FALSE;
	}
	if (!sCheckTagForLocationCommon(xml, "sku", 2))
	{
		return FALSE;
	}

	return TRUE;
}


BOOL XMLFindBestLangChildElem(
	CMarkup& xml,
	const char *szTag,
	LANGUAGE eLanguage)
{
	int nBestPos = -1;
	xml.ResetChildPos();
	while (xml.FindChildElem(szTag))
	{
		if (xml.HasChildAttrib("lang"))
		{
			char szLanguage[256] = "";
			PStrCopy(szLanguage, xml.GetChildAttrib("lang"), arrsize(szLanguage));

			LANGUAGE eChildLang = LanguageGetByName(AppGameGet(), szLanguage);

			if (eChildLang == eLanguage)
			{
				nBestPos = xml.SavePos();
			}

		}
		else if (nBestPos == -1)
		{
			nBestPos = xml.SavePos();
		}
	}

	if (nBestPos != -1)
	{
		return xml.RestorePos(nBestPos, TRUE);
	}

	return FALSE;
}

void UIXMLLoadFont(
	CMarkup& xml,
	UI_COMPONENT *component,
	UIX_TEXTURE* pFontTexture,
	char * szFontTag,
	UIX_TEXTURE_FONT *&pFont,
	char * szFontSizeTag,
	int & nFontSize)
{
	char szBuf[256];
	if (XMLFindBestLangChildElem(xml, szFontSizeTag, LanguageGetCurrent()))
	{
		PStrCopy(szBuf, xml.GetChildData(), arrsize(szBuf));

		if (szBuf[0] == '-')
		{
			int nParentSize = (component->m_pParent ? UIComponentGetFontSize(component->m_pParent) : 0);
			nFontSize = MAX(0, nParentSize - PStrToInt(&(szBuf[1])));
		}
		else if (szBuf[0] == '+')
		{
			int nParentSize = (component->m_pParent ? UIComponentGetFontSize(component->m_pParent) : 0);
			nFontSize = nParentSize + PStrToInt(&(szBuf[1]));
		}
		else
		{
			nFontSize = PStrToInt(szBuf);
		}
	}
	else if (!component->m_bReference)
	{
		nFontSize = 0;		// default
	}

	if (pFontTexture && pFontTexture->m_pFonts)
	{
		char szFont[256] = "";
		if (XMLFindBestLangChildElem(xml, szFontTag, LanguageGetCurrent()))
		{
			PStrCopy(szFont, xml.GetChildData(), arrsize(szFont));
			pFont = (UIX_TEXTURE_FONT*)StrDictionaryFind(pFontTexture->m_pFonts, szFont);
			ASSERTV_RETURN(pFont, " !!! Font not found: %s\n", szFont );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UIX_TEXTURE * UILoadComponentTexture(
	UI_COMPONENT *component,
	const char * szTex,
	BOOL bLoadFromLocalizedPak)
{
	if (szTex && szTex[0] != 0)
	{
		UIX_TEXTURE* texture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, szTex);
		if (!texture)
		{
#if ISVERSION(DEVELOPMENT)
			char timer_dbgstr[MAX_PATH];
			PStrPrintf(timer_dbgstr, MAX_PATH, "UITextureLoadDefinition(%s)", component->m_szName);
			TIMER_STARTEX(timer_dbgstr, DEFAULT_PERF_THRESHOLD_LARGE);
#endif

			texture = UITextureLoadDefinition(szTex, bLoadFromLocalizedPak);
			if (texture)
			{
				if ( component->m_bDynamic )
					texture->m_dwFlags |= UI_TEXTURE_DISCARD;
				StrDictionaryAdd(g_UI.m_pTextures, szTex, texture);
			}
			else
			{
				ASSERTV(0, " !!! Texture not found: %s for component %s\n", szTex, component->m_szName);
			}
		}
		return texture;
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sPostInitAnimCategories(
	void)
{
	// this isn't the most efficient because of the recounting, but it only happens
	//   once on load so no biggie.

	ANIM_RELATIONSHIP_NODE tNode;
	for (int i=0; i < NUM_UI_ANIM_CATEGORIES; i++)
	{
		while (g_UI.m_listCompsThatNeedAnimCategories[i].PopTail(tNode))
		{
			UI_COMPONENT *pComponent = tNode.Get();
			ASSERT_CONTINUE(pComponent);

			int nCategoryCount = 0;
			PList<UI_COMPONENT *>::USER_NODE *pNode = NULL;
			while ((pNode = g_UI.m_listAnimCategories[i].GetNext(pNode)) != NULL)
			{
				if (pNode->Value != pComponent)
				{
					nCategoryCount++;
				}
			}
			if (nCategoryCount == 0)
			{
				continue;
			}

			//count current relationships
			int nRelCount = 0;
			ANIM_RELATIONSHIP_NODE * pRelationship = pComponent->m_pAnimRelationships;
			while (pRelationship &&
				pRelationship->m_eRelType != ANIM_RELATIONSHIP_NODE::ANIM_REL_INVALID)
			{
				pRelationship++;
				nRelCount++;
			}

			// make room for the new ones in the categories
			if (pComponent->m_pAnimRelationships)
			{
				pComponent->m_pAnimRelationships = (ANIM_RELATIONSHIP_NODE *)REALLOCZ(NULL, pComponent->m_pAnimRelationships, sizeof(ANIM_RELATIONSHIP_NODE) * (nRelCount + nCategoryCount + 1));	//don't forget the null terminator
			}
			else
			{
				pComponent->m_pAnimRelationships = (ANIM_RELATIONSHIP_NODE *)MALLOCZ(NULL, sizeof(ANIM_RELATIONSHIP_NODE) * (nRelCount + nCategoryCount + 1));	//don't forget the null terminator
			}

			pNode = NULL;
			int j = nRelCount;
			while ((pNode = g_UI.m_listAnimCategories[i].GetNext(pNode)) != NULL)
			{
				if (pNode->Value != pComponent)		// don't reference yourself... you'll go blind
				{
					ASSERT_CONTINUE(j < nRelCount + nCategoryCount);
					pComponent->m_pAnimRelationships[j] = tNode;	// copy most of the information
					pComponent->m_pAnimRelationships[j].SetDirect(pNode->Value);

					j++;
				}
			}
		}
	}

	// we don't need this anymore
	for (int i=0; i < NUM_UI_ANIM_CATEGORIES; i++)
	{
		g_UI.m_listCompsThatNeedAnimCategories[i].Clear();
	}
}


static void sLoadAnimRelationship(
	CMarkup& xml,
	ANIM_RELATIONSHIP_NODE& tRelationship)
{
	const char *szReltype = xml.GetChildAttrib("type");
	if (szReltype)
	{
		int nType = StrDictFind(pAnimRelationsEnumTbl, szReltype);
		if (nType != -1)
			tRelationship.m_eRelType = (ANIM_RELATIONSHIP_NODE::REL_TYPE)nType;
	}

	tRelationship.m_bDirectOnly = TRUE;
	if (xml.HasChildAttrib("ondirectonly"))
	{
		tRelationship.m_bDirectOnly = (PStrToInt(xml.GetChildAttrib("ondirectonly")) != 0);
	}

	tRelationship.m_bOnlyIfUserActive = FALSE;
	if (xml.HasChildAttrib("ifuseractive"))
	{
		tRelationship.m_bOnlyIfUserActive = (PStrToInt(xml.GetChildAttrib("ifuseractive")) != 0);
	}

	tRelationship.m_bPreserverUserActive = FALSE;
	if (xml.HasChildAttrib("preserveuseractive"))
	{
		tRelationship.m_bPreserverUserActive = (PStrToInt(xml.GetChildAttrib("preserveuseractive")) != 0);
	}

	tRelationship.m_bOnActivate = TRUE;
	if (xml.HasChildAttrib("onactivate"))
	{
		tRelationship.m_bOnActivate = (PStrToInt(xml.GetChildAttrib("onactivate")) != 0);
	}

	tRelationship.m_bNoDelay = FALSE;
	if (xml.HasChildAttrib("nodelay"))
	{
		tRelationship.m_bNoDelay = (PStrToInt(xml.GetChildAttrib("nodelay")) != 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentLoadAnimRelationships(
	CMarkup& xml,
	UI_COMPONENT *component)
{

	// !!! NOTE: this doesn't currently support references.  That will need to be added.

	if (component->m_bReference && component->m_pAnimRelationships != NULL)
		return;

	ASSERT_RETURN(component->m_pAnimRelationships == NULL);

	// first see if this component goes in any anim categories
	xml.ResetChildPos();
	while (xml.FindChildElem("anim_category"))
	{
		const char *szCategory = xml.GetChildData();
		int nCategory = StrDictFind(pAnimCategoriesEnumTbl, szCategory);
		ASSERTV_CONTINUE(nCategory > -1 && nCategory < NUM_UI_ANIM_CATEGORIES, "UI Anim category ""%s"" not found.", szCategory);
		if (!g_UI.m_listAnimCategories[nCategory].Find(component))
			g_UI.m_listAnimCategories[nCategory].PListPushHead(component);
	}

	int nCount = 0;
	int nCategoryCount = 0;
	xml.ResetChildPos();
	while (xml.FindChildElem("anim_relationship"))
	{
		if (xml.HasChildAttrib("category"))
		{
			if (PStrToInt(xml.GetChildAttrib("category")) != 0)
			{
				nCategoryCount++;
				continue;
			}
		}
		nCount++;
	}

	if (nCount + nCategoryCount == 0)
	{
		component->m_pAnimRelationships = NULL;
		return;
	}

	nCount++;		// add an empty one to mark the end of the list

	// NOTE: the default constructor for this class only zeros the members so a MALLOCZ here
	//			should be OK.  If the constructor does something else this will have to change
	component->m_pAnimRelationships = (ANIM_RELATIONSHIP_NODE *)MALLOCZ(NULL, sizeof(ANIM_RELATIONSHIP_NODE) * nCount);

	int i = 0;
	xml.ResetChildPos();
	while (xml.FindChildElem("anim_relationship"))
	{
		const char *szComponent = xml.GetChildData();

		if (xml.HasChildAttrib("category") && PStrToInt(xml.GetChildAttrib("category")) != 0)
		{
			int nCategory = StrDictFind(pAnimCategoriesEnumTbl, szComponent);
			ASSERTV_CONTINUE(nCategory > -1 && nCategory < NUM_UI_ANIM_CATEGORIES, "UI Anim category ""%s"" not found.", szComponent);

			ANIM_RELATIONSHIP_NODE tTempNode;
			tTempNode.m_pComponent = component;
			sLoadAnimRelationship(xml, tTempNode);
			g_UI.m_listCompsThatNeedAnimCategories[nCategory].PListPushHead(tTempNode);
		}
		else
		{
			component->m_pAnimRelationships[i].Set(szComponent);

			sLoadAnimRelationship(xml, component->m_pAnimRelationships[i]);
			i++;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Apply render section offsets to all children of a component -
// specifically used for referenced components with many children
void sUIComponentReapplyRenderSections( UI_COMPONENT* pComponent )
{
	ASSERT_RETURN( pComponent )

		UI_COMPONENT *pChild = pComponent->m_pFirstChild;
	while( pChild )
	{
		if ( pChild->m_nRenderSectionOffset == 1 )
		{
			pChild->m_nRenderSection = pComponent->m_nRenderSection + 1;
		}
		else if ( pChild->m_nRenderSectionOffset == -1 &&
			pComponent->m_nRenderSection != RENDERSECTION_BOTTOM )
		{
			pChild->m_nRenderSection = pComponent->m_nRenderSection - 1;
		}
		else if( pChild->m_nRenderSectionOffset == 100 ) // no specific rendersection was assigned - so make sure to use the parent's
		{
			pChild->m_nRenderSection = pComponent->m_nRenderSection;
		}

		if( pChild->m_pFirstChild )
		{
			sUIComponentReapplyRenderSections( pChild );
		}
		pChild = pChild->m_pNextSibling;
	}

} // void sUIComponentReapplyRenderSections()


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentLoadBaseData(
	UI_XML_COMPONENT* def,
	CMarkup& xml,
	UI_COMPONENT* component,
	const UI_XML & tUIXml,
	BOOL bReferenceBase)
{
	def->m_nCount++;

	XML_LOADSTRING("name", component->m_szName, component->m_szName, MAX_UI_COMPONENT_NAME_LENGTH);

	if (!component->m_szName[0])
	{
		PStrPrintf(component->m_szName, DEFAULT_INDEX_SIZE, "%s%d", (def->m_szComponentName ? def->m_szComponentName : ""), def->m_nCount);
	}

#if ISVERSION(DEVELOPMENT)
	char timer_dbgstr[MAX_PATH];
	PStrPrintf(timer_dbgstr, MAX_PATH, "UIComponentLoadBaseData(%s)", component->m_szName);
	TIMER_STARTEX(timer_dbgstr, DEFAULT_PERF_THRESHOLD_LARGE);
#endif

	XML_LOADFLOAT("x", component->m_Position.m_fX, 0.0f);
	XML_LOADFLOAT("y", component->m_Position.m_fY, 0.0f);
	XML_LOADFLOAT("width", component->m_fWidth, 0.0f);
	XML_LOADFLOAT("height", component->m_fHeight, 0.0f);
	XML_LOADFLOAT("zdelta", component->m_fZDelta, 0.0f);

	XML_LOADFLOAT("width_min", component->m_SizeMin.m_fX, NaN);
	XML_LOADFLOAT("width_max", component->m_SizeMax.m_fX, NaN);
	XML_LOADFLOAT("height_min", component->m_SizeMin.m_fY, NaN);
	XML_LOADFLOAT("height_max", component->m_SizeMax.m_fY, NaN);

	XML_LOADFLOAT("width_parent_rel", component->m_fWidthParentRel, NaN);
	XML_LOADFLOAT("height_parent_rel", component->m_fHeightParentRel, NaN);	

	UI_POSITION wsPos;
	// widescreen
	if (g_UI.m_bWidescreen)
	{
		if (component->m_bReference)
		{
			wsPos = component->m_Position;
		}
		XML_LOADFLOAT("ws_x",		wsPos.m_fX, component->m_Position.m_fX);
		XML_LOADFLOAT("ws_y",		wsPos.m_fY, component->m_Position.m_fY);
		XML_LOADFLOAT("ws_width",	component->m_fWidth, component->m_fWidth);
		XML_LOADFLOAT("ws_height",	component->m_fHeight, component->m_fHeight);

		component->m_Position = wsPos;
	}

	XML_COMP_LOADFLOAT_REL("x_rel", m_Position.m_fX, m_fXmlWidthRatio);
	XML_COMP_LOADFLOAT_REL("y_rel", m_Position.m_fY, m_fXmlHeightRatio);
	XML_COMP_LOADFLOAT_REL("width_rel", m_fWidth, m_fXmlWidthRatio);
	XML_COMP_LOADFLOAT_REL("height_rel", m_fHeight, m_fXmlHeightRatio);

	// set the reference component's inactive position equal to its position so that the
	// inactive position isn't set to its reference component
	// this will only be a problem if we want a reference with the exact same position
	if (component->m_bReference)
	{
		component->m_InactivePos = component->m_Position;
	}

	XML_LOADFLOAT("tgtx", component->m_InactivePos.m_fX, component->m_Position.m_fX);
	XML_LOADFLOAT("tgty", component->m_InactivePos.m_fY, component->m_Position.m_fY);
	if (g_UI.m_bWidescreen)
	{
		// if there's not a WS target, and there's a non-ws target, use it.  Otherwise use the ws position.
		XML_LOADFLOAT("ws_tgtx", component->m_InactivePos.m_fX, component->m_InactivePos.m_fX != component->m_Position.m_fX ? component->m_InactivePos.m_fX : component->m_Position.m_fX);
		XML_LOADFLOAT("ws_tgty", component->m_InactivePos.m_fY, component->m_InactivePos.m_fY != component->m_Position.m_fY ? component->m_InactivePos.m_fY : component->m_Position.m_fY);
	}
	XML_COMP_LOADFLOAT_REL("tgtx_rel", m_InactivePos.m_fX, m_fXmlWidthRatio);
	XML_COMP_LOADFLOAT_REL("tgty_rel", m_InactivePos.m_fY, m_fXmlHeightRatio);

	BOOL bClipChildren = FALSE;
	XML_LOADBOOL("clipchildren", bClipChildren, FALSE);
	if (bClipChildren)
	{
		component->m_rectClip.m_fX2 = component->m_fWidth;
		component->m_rectClip.m_fY2 = component->m_fHeight;
	}

	XML_LOADFLOAT("ScrollHorizMax", component->m_fScrollHorizMax, component->m_bReference ? component->m_fScrollHorizMax : 0.0f);
	XML_LOADFLOAT("ScrollVertMax", component->m_fScrollVertMax, component->m_bReference ? component->m_fScrollVertMax : 0.0f);
	component->m_fScrollVertMax *= g_UI.m_fUIScaler;
	XML_LOADFLOAT("wheelscrollincrement", component->m_fWheelScrollIncrement, component->m_bReference ? component->m_fWheelScrollIncrement : (component->m_pParent ? component->m_pParent->m_fWheelScrollIncrement: 1.0f));
	XML_LOADFLOAT("framescrollspeed", component->m_fFrameScrollSpeed, component->m_bReference ? component->m_fFrameScrollSpeed : 0.0f);
	XML_LOADFLOAT("framescrollstart", component->m_posFrameScroll.m_fY, component->m_bReference ? component->m_posFrameScroll.m_fY : 0.0f);
	XML_LOADBOOL("framescrollreverse", component->m_bScrollFrameReverse, component->m_bReference ? component->m_bScrollFrameReverse : FALSE);

	XML_LOADBOOL("scrolls_with_parent", component->m_bScrollsWithParent, TRUE);

	XML_LOADBOOL("hasownfocusunit", component->m_bHasOwnFocusUnit, component->m_bReference ? component->m_bHasOwnFocusUnit : FALSE);
	XML_LOADBOOL("gray_in_ghost", component->m_bGrayInGhost, component->m_bReference ? component->m_bGrayInGhost : (component->m_pParent ? component->m_pParent->m_bGrayInGhost : TRUE));

	XML_LOADBOOL("visible", component->m_bVisible, component->m_bReference ? component->m_bVisible : (component->m_pParent ? component->m_pParent->m_bVisible : TRUE) );
	BOOL bActive = TRUE;
	XML_LOADBOOL("active", bActive, bActive);
	if (component->m_bVisible && bActive)
	{
		component->m_eState = UISTATE_ACTIVE;
	}
	else
	{
		component->m_eState = UISTATE_INACTIVE;
	}

	XML_LOADBOOL("stretch", component->m_bStretch, component->m_bReference ? component->m_bStretch : FALSE);
	XML_LOADBOOL("dynamic", component->m_bDynamic, component->m_bReference ? component->m_bDynamic : FALSE);
	XML_LOADBOOL("ignoresmouse", component->m_bIgnoresMouse, component->m_bReference ? component->m_bIgnoresMouse : (component->m_pParent && component->m_pParent->m_eComponentType != UITYPE_SCREEN ? component->m_pParent->m_bIgnoresMouse : FALSE));

	XML_LOADBOOL("independentactivate", component->m_bIndependentActivate, component->m_bReference ? component->m_bIndependentActivate : FALSE);		//If this is true, the component won't activate/deactivate when its parent does

	XML_LOADBOOL("noscale", component->m_bNoScale, (component->m_pParent ? component->m_pParent->m_bNoScale : FALSE));
	XML_LOADBOOL("noscalefont", component->m_bNoScaleFont, (component->m_pParent && component->m_pParent->m_bNoScaleFont ? TRUE : component->m_bNoScale));

	XML_LOADINT("angle", component->m_nAngle, component->m_bReference ? component->m_nAngle : 0);

	char szBuf[256] = "";
	// Default to the parent's render section
	XML_LOADSTRING("rendersection", szBuf, "", 256);
	component->m_nRenderSectionOffset = 0;
	if (PStrICmp(szBuf, "UpOne") == 0)
	{
		component->m_nRenderSectionOffset = 1;
		component->m_nRenderSection = ( ( component->m_pParent && component->m_pParent->m_nRenderSection != RENDERSECTION_DEBUG ) ? component->m_pParent->m_nRenderSection : RENDERSECTION_BOTTOM) + 1;
	}
	else if (PStrICmp(szBuf, "DownOne") == 0)
	{
		component->m_nRenderSectionOffset = -1;
		component->m_nRenderSection = ( ( component->m_pParent && component->m_pParent->m_nRenderSection != RENDERSECTION_BOTTOM ) ? ( component->m_pParent->m_nRenderSection - 1 ) : RENDERSECTION_BOTTOM);
	}
	else if (!szBuf[0] && component->m_bReference)
	{
		// don't override the value set from the reference component if no tag was given
	}
	else
	{
		if( !szBuf[0] )	// no specific rendersection was assigned - remember this for referenced items
		{
			component->m_nRenderSectionOffset = 100;
		}
		UIXMLLoadEnum(xml, "rendersection", component->m_nRenderSection, pRenderSectionEnumTbl, (component->m_pParent ? component->m_pParent->m_nRenderSection : RENDERSECTION_BOTTOM));
		// if this is a reference, we need to go through
		// children and reapply any up or down rendersection offsets, as they will not happen otherwise
		if( component->m_bReference )
		{
			sUIComponentReapplyRenderSections( component );
		}
	}

	XML_LOADSTRING("ttline", szBuf, "", 256);
	if (PStrICmp(szBuf, "next") == 0)
	{
		component->m_nTooltipLine = component->m_pPrevSibling ? component->m_pPrevSibling->m_nTooltipLine + 1 : 1;
	}
	else if (PStrICmp(szBuf, "same") == 0)
	{
		component->m_nTooltipLine = component->m_pPrevSibling ? component->m_pPrevSibling->m_nTooltipLine : 1;
	}
	else
	{
		if( !szBuf[0] )
		{
			if( !component->m_bReference )
			{
				component->m_nTooltipLine = 0;
			}
		}
		else
		{
			component->m_nTooltipLine = PStrToInt(szBuf);
		}
	}
	XML_LOADSTRING("ttlineorder", szBuf, "", 256);
	if (PStrICmp(szBuf, "next") == 0 && component->m_pPrevSibling)
	{
		component->m_nTooltipOrder = component->m_pPrevSibling->m_nTooltipOrder + 1;
	}
	else if (PStrICmp(szBuf, "same") == 0 && component->m_pPrevSibling)
	{
		component->m_nTooltipOrder = component->m_pPrevSibling->m_nTooltipOrder;
	}
	else
	{
		if( !szBuf[0] )
		{
			if( !component->m_bReference )
			{
				component->m_nTooltipOrder = 1;
			}
		}
		else
		{
			component->m_nTooltipOrder = PStrToInt(szBuf);
		}
	}
	XML_LOADENUM("ttjustify", component->m_nTooltipJustify, pAlignEnumTbl, component->m_bReference ? component->m_nTooltipJustify : UIALIGN_TOPLEFT);
	XML_LOADFLOAT("ttpaddingx", component->m_fToolTipPaddingX, component->m_bReference ? component->m_fToolTipPaddingX : 0.0f);
	XML_LOADFLOAT("ttpaddingy", component->m_fToolTipPaddingY, component->m_bReference ? component->m_fToolTipPaddingY : 0.0f);
	XML_LOADINT("ttcrosseslines", component->m_nTooltipCrossesLines, component->m_bReference ? component->m_nTooltipCrossesLines : 0);
	XML_LOADBOOL("ttnospace", component->m_bTooltipNoSpace, component->m_bReference ? component->m_bTooltipNoSpace : FALSE);
	XML_LOADFLOAT("ttxoffs", component->m_fToolTipXOffset, component->m_bReference ? component->m_fToolTipXOffset : 0.0f);
	XML_LOADFLOAT("ttyoffs", component->m_fToolTipYOffset, component->m_bReference ? component->m_fToolTipYOffset : 0.0f);

	XML_LOADINT("param", component->m_dwParam, component->m_bReference ? component->m_dwParam : 0);
	XML_LOADINT("param2", component->m_dwParam2, component->m_bReference ? component->m_dwParam2 : 0);
	XML_LOADINT("param3", component->m_dwParam3, component->m_bReference ? component->m_dwParam3 : 0);

	XML_LOADBOOL("fadesin", component->m_bFadesIn, FALSE);
	XML_LOADBOOL("fadesout", component->m_bFadesOut, FALSE);
	BOOL bFades = FALSE;
	XML_LOADBOOL("fades", bFades, FALSE);
	if (bFades)
	{
		component->m_bFadesIn = TRUE;
		component->m_bFadesOut = TRUE;
	}

	XML_LOADBOOL("animateswhenpaused", component->m_bAnimatesWhenPaused, component->m_bReference ? component->m_bAnimatesWhenPaused : (component->m_pParent ? component->m_pParent->m_bAnimatesWhenPaused : FALSE));
	XML_LOADINT("animtime", component->m_dwAnimDuration, component->m_bReference ? component->m_dwAnimDuration : STD_ANIM_TIME);
	if (!component->m_bReference)
		component->m_dwAnimDuration = (DWORD)((float)component->m_dwAnimDuration * STD_ANIM_DURATION_MULT);

	component->m_tMainAnimInfo.m_dwAnimTicket = INVALID_ID;
	component->m_tThrobAnimInfo.m_dwAnimTicket = INVALID_ID;

	//component->m_codeVisibleWhen = UIXmlLoadCode(xml, "visiblewhen", (component->m_bReference ? component->m_codeVisibleWhen : NULL_CODE) );
	component->m_codeStatToVisibleOn = UIXmlLoadCode(xml, "visibleoncontrolstat", (component->m_bReference ? component->m_codeStatToVisibleOn : NULL_CODE) );

	if (component->m_codeStatToVisibleOn != NULL_CODE)
	{
		int nStat = -1;
		if (xml.HasChildAttrib("stat"))
		{
			nStat = ExcelGetLineByStringIndex(AppGetCltGame(), DATATABLE_STATS, xml.GetChildAttrib("stat"));
		}
		UIAddMsgHandler(component, UIMSG_SETCONTROLSTAT, UIComponentVisibleOnStat, nStat);
		UIAddMsgHandler(component, UIMSG_SETCONTROLUNIT, UIComponentVisibleOnStat, INVALID_LINK);
		component->m_bIndependentActivate = TRUE;
	}
	else
	{
		component->m_codeStatToVisibleOn = UIXmlLoadCode(xml, "visibleontargetstat", (component->m_bReference ? component->m_codeStatToVisibleOn : NULL_CODE));
		if (component->m_codeStatToVisibleOn != NULL_CODE)
		{
			int nStat = -1;
			if (xml.HasChildAttrib("stat"))
			{
				nStat = ExcelGetLineByStringIndex(AppGetCltGame(), DATATABLE_STATS, xml.GetChildAttrib("stat"));
			}
			UIAddMsgHandler(component, UIMSG_SETTARGETSTAT, UIComponentVisibleOnStat, nStat);
			UIAddMsgHandler(component, UIMSG_SETTARGETUNIT, UIComponentVisibleOnStat, INVALID_LINK);
			component->m_bIndependentActivate = TRUE;
		}
		else
		{
			component->m_codeStatToVisibleOn = UIXmlLoadCode(xml, "visibleonfocusstat", (component->m_bReference ? component->m_codeStatToVisibleOn : NULL_CODE));
			if (component->m_codeStatToVisibleOn != NULL_CODE)
			{
				int nStat = -1;
				if (xml.HasChildAttrib("stat"))
				{
					nStat = ExcelGetLineByStringIndex(AppGetCltGame(), DATATABLE_STATS, xml.GetChildAttrib("stat"));
				}
				UIAddMsgHandler(component, UIMSG_SETFOCUSSTAT, UIComponentVisibleOnStat, nStat);
				component->m_bIndependentActivate = TRUE;
			}
		}
	}

	const char *szBestTex = NULL;
	BOOL bLocalizedTex = FALSE;
	xml.ResetChildPos();
	while (xml.FindChildElem("texture"))
	{
		const char *szTex = xml.GetChildData();
		if (xml.HasChildAttrib("lang"))
		{
			const char *szLanguage = xml.GetChildAttrib("lang");
			LANGUAGE eLang = LanguageGetByName(AppGameGet(), szLanguage);

			if (AppCommonGetForceLoadAllLanguages() || (AppTestDebugFlag(ADF_FILL_LOCALIZED_PAK_BIT) && g_UI.m_nFillPakSKU != INVALID_LINK))
			{
				if (SKUContainsLanguage(g_UI.m_nFillPakSKU, eLang, FALSE))
				{
					// go ahead and load it anyway so it goes in the PAK.
					UILoadComponentTexture(component, szTex, TRUE);
					szBestTex = szTex;
					bLocalizedTex = TRUE;
				}
			}

			if (eLang != LanguageGetCurrent())
			{
				continue;
			}
			else
			{
				szBestTex = szTex;
				bLocalizedTex = TRUE;
			}
		}
		else
		{
			if (AppCommonGetForceLoadAllLanguages()) {
				UILoadComponentTexture(component, szTex, TRUE);
			}
			if (!szBestTex)
				szBestTex = szTex;
		}
	}

	const char *szBestWSTex = NULL;
	BOOL bLocalizedWSTex = FALSE;
	if (g_UI.m_bWidescreen || AppTestDebugFlag( ADF_FILL_PAK_BIT ) ||AppTestDebugFlag( ADF_FILL_PAK_MIN_BIT ) ||
		AppTestDebugFlag(ADF_FILL_LOCALIZED_PAK_BIT) || AppCommonGetForceLoadAllLanguages())
	{
		xml.ResetChildPos();
		while (xml.FindChildElem("ws_texture"))
		{
			const char *szTex = xml.GetChildData();
			if (xml.HasChildAttrib("lang"))
			{
				const char *szLanguage = xml.GetChildAttrib("lang");
				LANGUAGE eLang = LanguageGetByName(AppGameGet(), szLanguage);

				if (AppCommonGetForceLoadAllLanguages() || (AppTestDebugFlag(ADF_FILL_LOCALIZED_PAK_BIT) && g_UI.m_nFillPakSKU != INVALID_LINK))
				{
					if (SKUContainsLanguage(g_UI.m_nFillPakSKU, eLang, FALSE))
					{
						// go ahead and load it anyway so it goes in the PAK.
						UILoadComponentTexture(component, szTex, TRUE);
						szBestWSTex = szTex;
						bLocalizedWSTex = TRUE;
					}
				}

				if (eLang != LanguageGetCurrent())
				{
					continue;
				}
				else
				{
					szBestWSTex = szTex;
					bLocalizedWSTex = TRUE;
				}
			}
			else
			{
				if (AppCommonGetForceLoadAllLanguages()) {
					UILoadComponentTexture(component, szTex, TRUE);
				}
				if (!szBestWSTex)
					szBestWSTex = szTex;
			}
		}
	}

	// if we're doing a fillpak, go ahead and load both normal and widescreen so they both go in the pak
	if (AppTestDebugFlag( ADF_FILL_PAK_BIT ) || AppTestDebugFlag( ADF_FILL_PAK_MIN_BIT ) || AppCommonGetForceLoadAllLanguages())
	{
		// don't worry that we're calling it twice for the same file.  It checks for that.
		if (!bLocalizedTex)
			UILoadComponentTexture(component, szBestTex, FALSE);
		if (!bLocalizedWSTex)
			UILoadComponentTexture(component, szBestWSTex, FALSE);
	}

	if (szBestWSTex && g_UI.m_bWidescreen)
	{
		szBestTex = szBestWSTex;
		bLocalizedTex = bLocalizedWSTex	;
	}

	if (szBestTex)
	{
		component->m_pTexture = UILoadComponentTexture(component, szBestTex, bLocalizedTex);
	}

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	{
		UIX_TEXTURE* fonttexture = UIGetFontTexture( LANGUAGE_CURRENT );
		if (texture)
		{
			char frm[256] = "";
			//XML_LOADFRAME("frame", texture, component->m_pFrame);
			XML_LOADSTRING( (g_UI.m_bWidescreen ? "ws_frame" : "frame"), frm, "", 256);
			if (g_UI.m_bWidescreen && !frm[0])		// if it's widescreen, and we didn't find a specific widescreen frame, get the regular one
			{
				XML_LOADSTRING("frame", frm, "", 256);
			}

			if (frm[0] != 0)
			{
				component->m_pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, frm);
				ASSERTV( component->m_pFrame, " !!! Frame not found: [%s] for component [%s]\n", frm, component->m_szName );
			}

			XML_GETBOOLATTRIB(g_UI.m_bWidescreen ? "ws_frame" : "frame", "tile", component->m_bTileFrame, FALSE);
			XML_GETBOOLATTRIB(g_UI.m_bWidescreen ? "ws_frame" : "frame", "tilepaddingx", component->m_fTilePaddingX, 0.0f);
			XML_GETBOOLATTRIB(g_UI.m_bWidescreen ? "ws_frame" : "frame", "tilepaddingy", component->m_fTilePaddingY, 0.0f);

			UIXMLLoadFont(xml, component, fonttexture, "font", component->m_pFont, "fontsize", component->m_nFontSize);
		}
	}

	char str[256] = "";
	XML_LOADSTRING("tooltipstring", str, "", 256);
	if (str[0] != 0)
	{
		const WCHAR * pStringFromTable = StringTableGetStringByKey(str);
		if ( pStringFromTable )
		{
			if( !component->m_szTooltipText )
			{
				component->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
			}

			PStrCopy(component->m_szTooltipText, pStringFromTable, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
		}
	}
	XML_LOADBOOL("hovertextwithmousedown", component->m_bHoverTextWithMouseDown, FALSE);

	DWORD dwDefaultColor = GFXCOLOR_WHITE;
	if (component->m_pFrame)
	{
		dwDefaultColor = component->m_pFrame->m_dwDefaultColor;
	}
	if (component->m_pParent)
	{
		// default to the parent's alpha
		dwDefaultColor = UI_MAKE_COLOR(UI_GET_ALPHA(component->m_pParent->m_dwColor), dwDefaultColor);
	}
	UIXMLLoadColor(xml, component, "", component->m_dwColor, dwDefaultColor);

	int nBlinkDuration = -1, nBlinkPeriod = -1;
	XML_LOADINT("blinkduration", nBlinkDuration, -1);
	XML_LOADINT("blinkperiod", nBlinkPeriod, -1);

	int r=0, g=0, b=0, a=0;
	XML_LOADINT("blinkred", r, 0);
	XML_LOADINT("blinkgreen", g, 0);
	XML_LOADINT("blinkblue", b, 0);
	XML_LOADINT("blinkalpha", a, 0);

	if (a || r || g || b || nBlinkDuration > -1 || nBlinkPeriod > -1)
	{
		component->m_pBlinkData = (UI_BLINKDATA *)MALLOC(NULL, sizeof(UI_BLINKDATA));
		component->m_pBlinkData->m_dwBlinkAnimTicket = INVALID_ID;

		if (nBlinkPeriod >=0)
			component->m_pBlinkData->m_nBlinkPeriod = nBlinkPeriod;
		else
			component->m_pBlinkData->m_nBlinkPeriod = 500;

		component->m_pBlinkData->m_nBlinkDuration = nBlinkDuration;
		if (a || r || g || b)
			component->m_pBlinkData->m_dwBlinkColor = (a << 24) | (r << 16) | (g << 8) | b;
		else
			component->m_pBlinkData->m_dwBlinkColor = GFXCOLOR_RED;

		component->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_NOT_BLINKING;
	}

	// must go last
	if (texture)
	{
		DWORD dwAnimDelay = 0;
		XML_LOADINT("frame_anim_delay_MS", dwAnimDelay, 50);

		xml.ResetChildPos();
		while (xml.FindChildElem("animframe"))
		{
			const char *szFrame = xml.GetChildData();
			UI_TEXTURE_FRAME* pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, szFrame);
			ASSERTV( pFrame, " !!! Frame not found: %s for component %s\n", szFrame, component->m_szName );
			if (!component->m_pFrameAnimInfo)
			{
				component->m_pFrameAnimInfo = (UI_FRAME_ANIM_INFO *)MALLOCZ(NULL, sizeof(UI_FRAME_ANIM_INFO));
				ASSERT_RETURN(component->m_pFrameAnimInfo);
				component->m_pFrameAnimInfo->m_dwFrameAnimMSDelay = dwAnimDelay;
			}
			component->m_pFrameAnimInfo->m_nNumAnimFrames++;
			if (!component->m_pFrameAnimInfo->m_pAnimFrames)
			{
				component->m_pFrameAnimInfo->m_pAnimFrames = (UI_TEXTURE_FRAME **)MALLOCZ(NULL, sizeof(UI_TEXTURE_FRAME *));
			}
			else
			{
				component->m_pFrameAnimInfo->m_pAnimFrames = (UI_TEXTURE_FRAME **)REALLOC(NULL, component->m_pFrameAnimInfo->m_pAnimFrames, sizeof(UI_TEXTURE_FRAME *) * component->m_pFrameAnimInfo->m_nNumAnimFrames);
			}
			component->m_pFrameAnimInfo->m_pAnimFrames[component->m_pFrameAnimInfo->m_nNumAnimFrames-1] = pFrame;

		}

		if (component->m_pFrameAnimInfo && component->m_pFrameAnimInfo->m_pAnimFrames)
		{
			component->m_pFrame = component->m_pFrameAnimInfo->m_pAnimFrames[0];
			component->m_pFrameAnimInfo->m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + component->m_pFrameAnimInfo->m_dwFrameAnimMSDelay, sUIUpdateAnimFrame, CEVENT_DATA((DWORD_PTR)component->m_idComponent));
		}
	}

	component->m_dwFramesAnimTicket = INVALID_ID;

	BOOL bUsesCursor = FALSE;
	XML_LOADBOOL("usescursor",bUsesCursor, FALSE);
	if (bUsesCursor && !bReferenceBase)
	{
		if (xml.HasChildAttrib("onalt"))
		{
			g_UI.m_listUseCursorComponentsWithAlt.PListPushHead(component);
		}
		else
		{
			g_UI.m_listUseCursorComponents.PListPushHead(component);
		}
	}

	XML_LOADENUM("resize_anchor", component->m_nResizeAnchor, pAlignEnumTbl, component->m_pParent ? component->m_pParent->m_nResizeAnchor : UIALIGN_TOPLEFT);

	XML_LOADENUM("anchor_parent", component->m_nAnchorParent, pAlignEnumTbl, UIALIGN_CENTER);
	XML_LOADFLOAT("anchor_x_offs", component->m_fAnchorXoffs, 0.0f);
	XML_LOADFLOAT("anchor_y_offs", component->m_fAnchorYoffs, 0.0f);

	UIComponentLoadAnimRelationships(xml, component);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentAdjustBaseData(
	UI_XML_COMPONENT* def,
	CMarkup& xml,
	UI_COMPONENT* component,
	const UI_XML & tUIXml )
{
	ASSERT(component);

	component->m_Position.m_fX *= tUIXml.m_fXmlWidthRatio;
	component->m_Position.m_fY *= tUIXml.m_fXmlHeightRatio;
	component->m_InactivePos.m_fX *= tUIXml.m_fXmlWidthRatio;
	component->m_InactivePos.m_fY *= tUIXml.m_fXmlHeightRatio;

	component->m_fAnchorXoffs *= tUIXml.m_fXmlWidthRatio;
	component->m_fAnchorYoffs *= tUIXml.m_fXmlHeightRatio;

	// save for later
	component->m_fXmlWidthRatio  = tUIXml.m_fXmlWidthRatio;
	component->m_fXmlHeightRatio = tUIXml.m_fXmlHeightRatio;

	component->m_fWidth  *= tUIXml.m_fXmlWidthRatio;
	component->m_fHeight *= tUIXml.m_fXmlHeightRatio;
	component->m_ActivePos = component->m_Position;

	component->m_rectClip.m_fX2 *= tUIXml.m_fXmlWidthRatio;
	component->m_rectClip.m_fY2 *= tUIXml.m_fXmlHeightRatio;

	component->m_fTilePaddingX *= tUIXml.m_fXmlWidthRatio;
	component->m_fTilePaddingY *= tUIXml.m_fXmlHeightRatio;

	component->m_fToolTipPaddingX *= tUIXml.m_fXmlWidthRatio;
	component->m_fToolTipPaddingY *= tUIXml.m_fXmlHeightRatio;
	component->m_fToolTipXOffset *= tUIXml.m_fXmlWidthRatio;
	component->m_fToolTipYOffset *= tUIXml.m_fXmlHeightRatio;

	component->m_idFocusUnit = INVALID_ID;
	component->m_nModelID = INVALID_ID;
	component->m_nAppearanceDefID = INVALID_ID;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlFindComponent(
	CMarkup& xml,
	const char *szName)
{
	if (xml.HasChildAttrib("name"))
	{
		if (PStrICmp(xml.GetChildAttrib("name"), szName) == 0)
		{
			// We found it!  Get out.
			return TRUE;
		}
	}

	if (xml.IntoElem())
	{
		if (xml.FindChildElem("name"))
		{
			if (PStrICmp(xml.GetChildData(), szName) == 0)
			{
				// We found it!  Get out.
				xml.OutOfElem();
				return TRUE;
			}
		}
		if (UIXmlFindComponent(xml, szName))
		{
			return TRUE;
		}
		xml.OutOfElem();
	}

	while (xml.FindChildElem())
	{
		if (UIXmlFindComponent(xml, szName))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIFillLinearMessageHandlerLists(
	UI_COMPONENT *pComponent)
{
	if (!pComponent)
		return;

	sUIFillLinearMessageHandlerLists(pComponent->m_pFirstChild);

	PList<UI_MSGHANDLER>::USER_NODE *pNode = NULL;
	while ((pNode = pComponent->m_listMsgHandlers.GetNext(pNode)) != NULL)
	{
		g_UI.m_listMessageHandlers[pNode->Value.m_nMsg].PListPushTailPool(NULL, &pNode->Value);
	}

	sUIFillLinearMessageHandlerLists(pComponent->m_pNextSibling);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSGHANDLER * UIGetMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg,
	int nStatID /*= INVALID_LINK*/)
{
	ASSERT_RETNULL(pComponent);
	ASSERT_RETNULL(nMsg >= 0 && nMsg < NUM_UI_MESSAGES);

	PList<UI_MSGHANDLER>::USER_NODE *pNode = NULL;
	while ((pNode = pComponent->m_listMsgHandlers.GetNext(pNode)) != NULL)
	{
		if (pNode->Value.m_nMsg == nMsg &&
			pNode->Value.m_nStatID == nStatID)
		{
			return &(pNode->Value);
		}
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIAddMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg,
	PFN_UIMSG_HANDLER pfnHandler,
	int nStatID,
	BOOL bAddingDefaults /*= FALSE*/)
{
	ASSERT_RETNULL(pComponent);
	ASSERT_RETNULL(nMsg >= 0 && nMsg < NUM_UI_MESSAGES);
	ASSERTX_RETNULL(UITestFlag(UI_FLAG_LOADING_UI), "You should only call this function while loading the UI.");

	if (IsStatMessage(nMsg))
	{
		ASSERT_RETNULL(nStatID != INVALID_ID);
	}

	// replace if there's already one there
	//   (unless it's a reference and this is a default handler)
	UI_MSGHANDLER *pHandler = UIGetMsgHandler(pComponent, nMsg, nStatID);
	if (pHandler)
	{
		// don't replace one for a reference with a default
		if (!bAddingDefaults ||
			!pHandler->m_fpMsgHandler ||
			!pComponent->m_bReference)
		{
			pHandler->m_fpMsgHandler = pfnHandler;
		}
		return TRUE;
	}

	// otherwise add it
	UI_MSGHANDLER tHandler;
	tHandler.m_fpMsgHandler = pfnHandler;
	tHandler.m_pComponent = pComponent;
	tHandler.m_nMsg = nMsg;
	tHandler.m_nStatID = nStatID;
	BOOL bAdded = pComponent->m_listMsgHandlers.PListPushTailPool(NULL, tHandler);

	ASSERT(bAdded);

	return bAdded;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUIRemoveMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg)
{
	ASSERT_RETNULL(pComponent);
	ASSERT_RETNULL(nMsg >= 0 && nMsg < NUM_UI_MESSAGES);

	PList<UI_MSGHANDLER>::USER_NODE *pNode = NULL;
	while ((pNode = pComponent->m_listMsgHandlers.GetNext(pNode)) != NULL)
	{
		if (pNode->Value.m_nMsg == nMsg)
		{
			pComponent->m_listMsgHandlers.RemoveCurrent(pNode);
			return TRUE;
		}
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUIAddSoundMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg,
	int nSoundID,
	BOOL bStop)
{
	ASSERT_RETNULL(pComponent);
	ASSERT_RETNULL(nMsg >= 0 && nMsg < NUM_UI_MESSAGES);

	UI_SOUND_HANDLER tHandler;
	tHandler.m_nSoundId	 = nSoundID;
	tHandler.m_nPlayingSoundId = INVALID_ID;
	tHandler.m_bStopSound = bStop;
	tHandler.m_pComponent = pComponent;
	return g_UI.m_listSoundMessageHandlers[nMsg].PListPushTailPool(NULL, tHandler);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIComponentCreateNew(
	UI_COMPONENT * parent,
	UI_XML_COMPONENT * def,
	BOOL bLoadDefaults = FALSE)
{
	UI_COMPONENT *component = def->m_fpComponentMalloc(def);
	ASSERT_RETNULL(component);

	component->m_idComponent = ++g_UI.m_nCurrentComponentId;
	component->m_eComponentType = def->m_eComponentType;

	component->m_listMsgHandlers.Init();


	UIComponentAddChild(parent, component);

	if (bLoadDefaults)
	{
		CMarkup xml;
		UI_XML tUIXml;
		tUIXml.m_fXmlHeightRatio = parent->m_fXmlHeightRatio;
		tUIXml.m_fXmlWidthRatio = parent->m_fXmlWidthRatio;
		UIComponentLoadBaseData(def, xml, component, tUIXml, FALSE );
		UIComponentAdjustBaseData(def, xml, component, tUIXml);
		UIComponentLoadExtras(xml, NULL, component, tUIXml);
	}

	return component;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIComponentCreateNew(
	UI_COMPONENT * parent,
	int nUIType,
	BOOL bLoadDefaults /*= FALSE*/)
{
	UI_XML_COMPONENT *pDef = UIXmlComponentFind(nUIType);
	ASSERT_RETNULL(pDef);

	return UIComponentCreateNew(parent, pDef, bLoadDefaults);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIXmlComponentLoad(
	CMarkup & xml,
	CMarkup * includexml,
	UI_COMPONENT * parent,
	const UI_XML& tUIXml,
	BOOL bReferenceBase /*= FALSE */,
	LPCSTR szAltComponentName /*= NULL */)
{
	UI_XML_COMPONENT* def = NULL;
	UI_COMPONENT *component = NULL;

	char child_name[256];
	char szComponentName[256];

	PStrCopy(child_name, xml.GetChildTagName(), 256);
	if (child_name[0] == 0)
	{
		return NULL;
	}

	if (!UIXmlCheckTagForLocation(xml))
	{
		return NULL;
	}

	// If this is a reference tag
	if (PStrICmp(child_name, "reference") == 0)
	{
		char szReferenceName[256];
		xml.GetChildData(szReferenceName, 256);

		if (!szReferenceName[0])
			PStrCopy(szReferenceName, xml.GetChildAttrib("refname"), 256);

		// Make a copy of this XML reference, so we don't lose our original position
		unsigned int cursor = xml.SavePos();
		xml.ResetPos();

		// Now find the XML node that is referenced
		if (UIXmlFindComponent(xml, szReferenceName))
		{
			// Create a new component based on the found node
			component = UIXmlComponentLoad(xml, includexml, parent, tUIXml, TRUE);
			if (!component)
				return NULL;

			def = UIXmlComponentFind(component->m_eComponentType);
			component->m_bReference = TRUE;
			ASSERT(def);
		}

		ASSERT(xml.RestorePos(cursor, TRUE));

		if( includexml &&
			!component )
		{
			// Make a copy of this XML reference, so we don't lose our original position
			unsigned int cursor = includexml->SavePos();
			includexml->ResetPos();

			// Now find the XML node that is referenced
			if (UIXmlFindComponent(*includexml, szReferenceName))
			{
				// Create a new component based on the found node
				component = UIXmlComponentLoad(*includexml, includexml, parent, tUIXml, TRUE);
				if (!component)
					return NULL;

				def = UIXmlComponentFind(component->m_eComponentType);
				component->m_bReference = TRUE;
				ASSERT(def);
			}

			ASSERT(includexml->RestorePos(cursor, TRUE));
		}
	}

	if (szAltComponentName)
	{
		PStrCopy(szComponentName, szAltComponentName, 256);
	}
	else
	{
		PStrCopy(szComponentName, xml.GetChildAttrib("name"), 256);
	}

#if ISVERSION(DEVELOPMENT)
	char timer_dbgstr[MAX_PATH];
	PStrPrintf(timer_dbgstr, MAX_PATH, "UIXmlComponentLoad(%s)", szComponentName);
	TIMER_STARTEX(timer_dbgstr, DEFAULT_PERF_THRESHOLD_LARGE);
#endif

	if (!component)		// If we've already gotten a reference, this stuff is already done
	{
		def = UIXmlComponentFind(child_name);
		if (!def)
		{
			return NULL;
		}
	}
	if (!xml.IntoElem()) //-----------------------------------------------------
	{
		return NULL;
	}

	if (!component)		// If we've already gotten a reference, this stuff is already done
	{
		component = UIComponentCreateNew(parent, def);
	}

	ASSERT_RETNULL(component);

	if (szComponentName[0])
	{
		PStrCopy(component->m_szName, szComponentName, 128);
	}
	UIComponentLoadBaseData(def, xml, component, tUIXml, bReferenceBase);

	if (!bReferenceBase)
	{
		UIComponentAdjustBaseData(def, xml, component, tUIXml);
	}

	UIComponentLoadExtras(xml, includexml, component, tUIXml);

	if (component->m_eState == UISTATE_INACTIVE)
	{
		component->m_Position = component->m_InactivePos;
	}

	if (def->m_fpXmlLoadFunc)
	{
		def->m_fpXmlLoadFunc(xml, component, tUIXml);
	}

	xml.OutOfElem(); //----------------------------------------------------------

	if (!bReferenceBase)
	{
		int nNumUIComps = ExcelGetNumRows( NULL, DATATABLE_UI_COMPONENT );
		for (int i = 0; i < nNumUIComps; ++i)
		{
			const UI_COMPONENT_DATA *pUICompData = UIComponentGetData( i );
			ASSERTX_CONTINUE( pUICompData, "Expected ui component data" );
			if (PStrCmp(component->m_szName, pUICompData->szComponentString) == 0)
			{
				ASSERTV_RETNULL(g_UI.m_pComponentsMap[i] == NULL,
					"A duplicate has been found for component [%s].  This component is referenced by name and cannot have another with the same name.",
					component->m_szName);

				g_UI.m_pComponentsMap[i] = component;
				break;
			}
		}
	}

	return component;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_TEXTURE_FRAME* UIGetSkillTextureFrame(
	int nSkillID,
	UIX_TEXTURE **ppTexture)
{
	UI_TEXTURE_FRAME* frame = NULL;
	ASSERT_RETZERO(ppTexture != NULL);
	*ppTexture = NULL;

	if (nSkillID != INVALID_ID)
	{
		const SKILL_DATA* skill_data = SkillGetData(AppGetCltGame(), nSkillID);
		if (skill_data)
		{
			// ok, here's the deal.  Right now all the skill icons are in a different texture from the
			// other inventory items' icons.  So we're just gonna look at that one texture right here.
			*ppTexture = UIComponentGetTexture(UIComponentGetByEnum(UICOMP_SKILLMAP));
			if (*ppTexture)
			{
				frame = (UI_TEXTURE_FRAME*)StrDictionaryFind((*ppTexture)->m_pFrames, skill_data->szSmallIcon);
			}
		}
	}

	return frame;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sJoinRefuse()
{
	trace ("JOIN REFUSED\n");
}

void sJoinAccept()
{
	trace ("JOIN ACCEPTED\n");
}

void UITestingToggle()
{
	g_UI.m_bDebugTestingToggle = !g_UI.m_bDebugTestingToggle;
	UIRepaint();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleTargetIndicator()
{
	g_UI.m_bShowTargetIndicator = !g_UI.m_bShowTargetIndicator;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowTargetIndicator(BOOL bShow)
{
	g_UI.m_bShowTargetIndicator = bShow;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIToggleTargetBrackets()
{
	UIToggleFlag(UI_FLAG_SHOW_TARGET_BRACKETS);
	if (!UITestFlag(UI_FLAG_SHOW_TARGET_BRACKETS))
	{
		UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_TARGETCOMPLEX);
		if (component)
		{
			UI_COMPONENT *ppCorner[4];
			ppCorner[0] = UIComponentFindChildByName(component, "selection TL");
			ppCorner[1] = UIComponentFindChildByName(component, "selection TR");
			ppCorner[2] = UIComponentFindChildByName(component, "selection BL");
			ppCorner[3] = UIComponentFindChildByName(component, "selection BR");
			if (ppCorner[0] && ppCorner[1] && ppCorner[2] && ppCorner[3])
				for (int i=0; i<4; i++)
					UIComponentSetVisible(ppCorner[i], FALSE);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUnitLabelsAdd(
	UI_UNIT_DISPLAY * pUnitDisplay,
	int nUnits,
	float flMaxRange)
{
	//if (AppIsTugboat())
   // {
	//    return;
   // }

	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_UNIT_NAMES);
	if (!component)
		return;

	UI_UNIT_NAME_DISPLAY *pNameDisp = UICastToUnitNameDisplay(component);
	ASSERT_RETURN(pNameDisp);

	// clear display
	UIComponentRemoveAllElements(pNameDisp);
	pNameDisp->m_idUnitNameUnderCursor = INVALID_ID;

	// get player
	UNIT *pPlayer = UIGetControlUnit();
	if (pPlayer == NULL)
	{
		return;
	}

    UNIT *pCurrentTarget = UnitGetById(AppGetCltGame(), g_UI.m_idTargetUnit);

	// CHB 2006.09.01 - warning C4189: 'flFadeRingWidthDistSq' : local variable is initialized but not referenced
#if 0
	// what are the begin fade distances and total faded out distance
	//float flTotalFadeOutDistance = flMaxRange * 0.95f;
	//float flBeginFadeOutDistance = flTotalFadeOutDistance * 0.6f;
	//float flTotalFadeOutDistanceSq = flTotalFadeOutDistance * flTotalFadeOutDistance;
	//float flBeginFadeOutDistanceSq = flBeginFadeOutDistance * flBeginFadeOutDistance;
	//float flFadeRingWidthDistSq = flTotalFadeOutDistanceSq - flBeginFadeOutDistanceSq;
#endif

	// fill display
	if (pNameDisp && pUnitDisplay)
	{
		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pNameDisp);
		int nBaseFontSize = UIComponentGetFontSize(pNameDisp);
		UIX_TEXTURE *pTexture = UIComponentGetTexture(pNameDisp);
		GAME *pGame = AppGetCltGame();
		UNIT *pInteractTarget = UIGetTargetUnit();

		BOOL bCursorActive = UICursorGetActive();
		UI_POSITION posCursor;
		if (bCursorActive)
		{
			float x=0.0f, y=0.0f;
			UIGetCursorPosition(&x, &y, FALSE);
			posCursor.m_fX = x;
			posCursor.m_fY = y;
		}
		else
		{
			posCursor.m_fX = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX() / 2.0f;
			posCursor.m_fY = AppCommonGetWindowHeight() * UIGetScreenToLogRatioY() / 2.0f;
		}

		UI_RECT	rectElement;
		UI_RECT rectScreen(0.0f, 0.0f, UIDefaultWidth(), UIDefaultHeight());
		float fClosestZ = 1000.0f;
		int iClosest = -1;
		int iActualSelection = -1;
		UNIT* pActualSelection = NULL;
		UISetAltTargetUnit( INVALID_ID );

		float fClosestPos = FLT_MAX;
		float fMaxY = UIDefaultHeight();
		float fMinY = 0.0f;
		component = UIComponentGetByEnum(UICOMP_LEFTSKILL);
		if (component)
			fMaxY = component->m_Position.m_fY;
		component = UIComponentGetByEnum(UICOMP_PORTRAIT);
		if (component)
			fMinY = component->m_Position.m_fY + component->m_fHeight;

		for (int i=0; i<nUnits; i++)
		{
			UNIT *pUnit = UnitGetById(pGame, pUnitDisplay[i].nUnitID);

			pUnitDisplay[i].bIsItem = UnitIsA( pUnit, UNITTYPE_ITEM );

			// for items we will not show ones too far away
			if (UnitDebugTestLabelFlags() == FALSE &&
				pUnitDisplay[i].bIsItem &&
//				pUnitDisplay[i].fPlayerDistSq > PICKUP_RADIUS_SQ * 20.0f)
				pUnitDisplay[i].fPlayerDistSq > NAMELABEL_VIEW_DISTANCE_SQ)
			{
				continue;
			}

			// if it's behind you, don't show it
			if (pUnitDisplay[ i ].vScreenPos.fZ >= 1.0f)
			{
				continue;
			}
			if(   UnitDataTestFlag(UnitGetData( pUnit ), UNIT_DATA_FLAG_HIDE_NAME) )
			{
				continue;
			}

			if (pUnit)
			{
				BOOL bUseExistingPointer = FALSE;
				WCHAR *puszName = pUnitDisplay[i].uszDisplay;

				if( AppIsHellgate() )
				{
					pUnitDisplay[i].fLabelScale = PIN(pUnitDisplay[i].fLabelScale * 3.0f, 0.2f, 1.5f);
				}

				//if (g_UI.m_bDebugTestingToggle)
				//	fScale = pUnitDisplay[i].fLabelScale * 3.0f;

				float flScale = UnitGetLabelScale( pUnit );
				if (flScale == 0.0f)
				{
					continue;  // don't bother for labels we cant see
				}
				pUnitDisplay[i].fLabelScale *= flScale;

				float fWidthRatio = AppIsHellgate() ? pUnitDisplay[i].fLabelScale : 1.0f;
				UIElementGetTextLogSize(pFont,
					nBaseFontSize,
					fWidthRatio,
					pNameDisp->m_bNoScaleFont,
					puszName,
					&(pUnitDisplay[i].fTextWidth),
					&(pUnitDisplay[i].fTextHeight));

				if (UnitIsA(pUnit, UNITTYPE_ITEM ) &&
					pNameDisp->m_fMaxItemNameWidth > 0.0f &&
					pUnitDisplay[i].fTextWidth > pNameDisp->m_fMaxItemNameWidth * fWidthRatio)
				{
					int nLen = PStrLen(pUnitDisplay[i].uszDisplay);
					puszName = (WCHAR*)MALLOC(NULL, nLen * sizeof(WCHAR));
					PStrCopy(puszName, pUnitDisplay[i].uszDisplay, nLen);
					bUseExistingPointer = TRUE;

					UI_SIZE size = DoWordWrapEx(puszName, NULL, pNameDisp->m_fMaxItemNameWidth, pFont, pNameDisp->m_bNoScaleFont, nBaseFontSize, 0, fWidthRatio);

					pUnitDisplay[i].fTextWidth = size.m_fWidth;
					pUnitDisplay[i].fTextHeight = size.m_fHeight;
				}

				BYTE bAlpha = 255;

				// We only want to show the NPC you're pointing at at full alpha
				if (UnitIsNPC(pUnit))
				{
					if (pUnit != pInteractTarget)
					{
						bAlpha = 128;
					}
				}

				// add text
				DWORD dwTextColor = GFXCOLOR_WHITE;
				//if (!InventoryItemMeetsClassReqs(pUnit, pPlayer))
				//{
				//	dwTextColor = GFXCOLOR_RED;
				//}

				float fZDelta = e_UIConvertAbsoluteZToDeltaZ( pUnitDisplay[ i ].vScreenPos.fZ );
				UI_SIZE tSize (pUnitDisplay[i].fTextWidth, pUnitDisplay[i].fTextHeight);

				UI_POSITION posBase;
				if( AppIsHellgate() )
				{
					posBase.m_fX = (pUnitDisplay[i].vScreenPos.fX + 1.0f) * 0.5f * UIDefaultWidth();
					posBase.m_fY = (-pUnitDisplay[i].vScreenPos.fY + 1.0f) * 0.5f * UIDefaultHeight();
				}
				else
				{
					if( UnitIsA( pUnit, UNITTYPE_PLAYER ) )
					{
						int x, y;
						VECTOR vLoc;
						vLoc = UnitGetPosition( pUnit );
						vLoc.fZ += UnitGetCollisionHeight( pUnit ) * 1.6f;
						TransformWorldSpaceToScreenSpace( &vLoc, &x, &y );

						posBase.m_fX = (float)x ;
						posBase.m_fY = (float)y;

					}
					else
					{
						posBase.m_fX = pUnitDisplay[i].vScreenPos.fX;
						posBase.m_fY = pUnitDisplay[i].vScreenPos.fY - 100;

					}

				}

				UI_POSITION pos(posBase.m_fX - tSize.m_fWidth / 2.0f, posBase.m_fY - tSize.m_fHeight);
				WCHAR szPlayerInfo[256] = L"";

				// add some extra stuff for the players
				//   this may change the size
				if (UnitIsPlayer(pUnit))
				{
					if (pNameDisp->m_nDisplayArea != INVALID_LINK)
					{
						if ( AppIsHellgate() )
						{
							float fWidthRatio = pUnitDisplay[i].fLabelScale;
							float fExtraWidth = 0.0f;
							float fExtraHeight = 0.0f;

							PrintStats(pGame, pNameDisp->m_eTable, pNameDisp->m_nDisplayArea, pUnit, szPlayerInfo, arrsize(szPlayerInfo));

							if (szPlayerInfo[0])
							{
								UIElementGetTextLogSize(pFont,
									pNameDisp->m_nPlayerInfoFontsize,
									fWidthRatio,
									pNameDisp->m_bNoScaleFont,
									szPlayerInfo,
									&fExtraWidth,
									&fExtraHeight);

								tSize.m_fHeight += fExtraHeight + (pNameDisp->m_fPlayerInfoSpacing * pUnitDisplay[i].fLabelScale);
								tSize.m_fWidth = MAX(tSize.m_fWidth, fExtraWidth);

								pos.m_fY -= fExtraHeight;

								// Prepend the color code to the string, and return color at the end
								WCHAR szTempLine[256] = L"";
								UIColorCatString(szTempLine, arrsize(szTempLine), c_UIGetOverheadNameColor(pGame, pUnit, pPlayer), szPlayerInfo);
								PStrCopy(szPlayerInfo, szTempLine, arrsize(szPlayerInfo));
							}
						}
						else
						{
							WCHAR szGuildInfo[256] = L"";
							GUILD_RANK eGuildRank = GUILD_RANK_INVALID;
							WCHAR szGuildRankName[256] = L"";
							UnitGetPlayerGuildAssociationTag(pUnit, szGuildInfo, arrsize(szGuildInfo), eGuildRank, szGuildRankName, arrsize(szGuildRankName));

							if (szGuildInfo[0])
							{
								FONTCOLOR NameColor = c_UIGetOverheadNameColor(pGame, pUnit, pPlayer);
								UIColorCatString(puszName, MAX_UNIT_DISPLAY_STRING, NameColor, L"\n<");
								UIColorCatString(puszName, MAX_UNIT_DISPLAY_STRING, NameColor,  szGuildInfo);
								UIColorCatString(puszName, MAX_UNIT_DISPLAY_STRING, NameColor, L">");
							}
						}
					}

					if (pNameDisp->m_pHardcoreFrame &&
						PlayerIsHardcore(pUnit))
					{
						if( AppIsHellgate() || UnitIsInTown( pUnit ) )
						{
							// make it about the same size as the letters of the name
							float fIconHeight = pUnitDisplay[i].fTextHeight;
							float fRatio = AppIsHellgate() ? ( pUnitDisplay[i].fTextHeight / pNameDisp->m_pHardcoreFrame->m_fHeight ) : 1.5f;
	//						float fIconWidth = pNameDisp->m_pHardcoreFrame->m_fWidth * fRatio;

							UI_POSITION posIcon;
							if( AppIsTugboat() )
							{
								posIcon = UI_POSITION(tSize.m_fWidth + (pNameDisp->m_fPlayerBadgeSpacing * pUnitDisplay[i].fLabelScale), (tSize.m_fHeight - fIconHeight) / 2.0f);
							}
							else
							{
								posIcon = UI_POSITION(/*tSize.m_fWidth*/-pNameDisp->m_pHardcoreFrame->m_fWidth - (pNameDisp->m_fPlayerBadgeSpacing * pUnitDisplay[i].fLabelScale), (tSize.m_fHeight - fIconHeight) / 2.0f);

							}
							posIcon += pos;
							UI_GFXELEMENT *pIconElement = UIComponentAddElement(pNameDisp, pTexture,
								pNameDisp->m_pHardcoreFrame, posIcon, UI_MAKE_COLOR(bAlpha, pNameDisp->m_dwHardcoreFrameColor),
								NULL, FALSE, fRatio, fRatio );
							if( AppIsHellgate() )
							{
								pIconElement->m_fZDelta = fZDelta;
							}
						}
					}
					if (pNameDisp->m_pLeagueFrame &&
						PlayerIsLeague(pUnit))
					{
						if( AppIsHellgate() || UnitIsInTown( pUnit ) )
						{
							float fIconHeight = pUnitDisplay[i].fTextHeight;
							float fRatio = AppIsHellgate() ? ( pUnitDisplay[i].fTextHeight / pNameDisp->m_pLeagueFrame->m_fHeight ) : 1.5f;

							UI_POSITION posIcon(tSize.m_fWidth + (pNameDisp->m_fPlayerBadgeSpacing * pUnitDisplay[i].fLabelScale), (tSize.m_fHeight - fIconHeight) / 2.0f);
							posIcon += pos;
							UI_GFXELEMENT *pIconElement = UIComponentAddElement(pNameDisp, pTexture,
								pNameDisp->m_pLeagueFrame, posIcon, UI_MAKE_COLOR(bAlpha, pNameDisp->m_dwLeagueFrameColor),
								NULL, FALSE, fRatio, fRatio );
							if( AppIsHellgate() )
							{
								pIconElement->m_fZDelta = fZDelta;
							}
						}
					}
					if (pNameDisp->m_pEliteFrame &&
						PlayerIsElite(pUnit))
					{
						if( AppIsHellgate() || UnitIsInTown( pUnit ) )
						{
							float fIconHeight = pUnitDisplay[i].fTextHeight;
							float fRatio = AppIsHellgate() ? ( pUnitDisplay[i].fTextHeight / pNameDisp->m_pEliteFrame->m_fHeight ) : 1.5f;

							UI_POSITION posIcon;
							if( AppIsHellgate() )
							{
								posIcon.m_fX = tSize.m_fWidth + (pNameDisp->m_fPlayerBadgeSpacing * pUnitDisplay[i].fLabelScale);
								posIcon.m_fY = (tSize.m_fHeight - fIconHeight) / 2.0f;
							}
							else
							{
								posIcon.m_fX =  -1.0f * (pNameDisp->m_fPlayerBadgeSpacing * pUnitDisplay[i].fLabelScale) - pNameDisp->m_pEliteFrame->m_fWidth * fRatio;
								posIcon.m_fY = (tSize.m_fHeight - fIconHeight) / 2.0f;
							}
							posIcon += pos;
							UI_GFXELEMENT *pIconElement = UIComponentAddElement(pNameDisp, pTexture,
								pNameDisp->m_pEliteFrame, posIcon, UI_MAKE_COLOR(bAlpha, pNameDisp->m_dwEliteFrameColor),
								NULL, FALSE, fRatio, fRatio );
							if( AppIsHellgate() )
							{
								pIconElement->m_fZDelta = fZDelta;
							}
						}
					}
					if (pNameDisp->m_pPVPOnlyFrame &&
						PlayerIsPVPOnly(pUnit))
					{
						if( UnitIsInTown( pUnit ) )
						{
							//float fIconHeight = pUnitDisplay[i].fTextHeight;
							float fRatio = AppIsHellgate() ? ( pUnitDisplay[i].fTextHeight / pNameDisp->m_pEliteFrame->m_fHeight ) : 1.5f;

							UI_POSITION posIcon;
							
							posIcon.m_fX =  pNameDisp->m_pPVPOnlyFrame->m_fWidth * .5f;
							posIcon.m_fY = (tSize.m_fHeight);
		
							posIcon += pos;
							UIComponentAddElement(pNameDisp, pTexture,
								pNameDisp->m_pPVPOnlyFrame, posIcon, UI_MAKE_COLOR(bAlpha, pNameDisp->m_dwHardcoreFrameColor),
								NULL, FALSE, fRatio, fRatio );
							
						}
					}
				}
				else
				{
					if (pCurrentTarget == pUnit &&
						UnitIsA( pUnit, UNITTYPE_MONSTER ))
					{
						UI_COMPONENT *pMinTarget = UIComponentGetByEnum(UICOMP_MINIMAL_TARGET);
						if (pMinTarget)
						{
							pMinTarget->m_Position = pos;
							pMinTarget->m_Position.m_fX += tSize.m_fWidth + (20.f * pUnitDisplay[i].fLabelScale);
						}
					}
				}

				if (bUseExistingPointer)
				{
					pUnitDisplay[ i ].pTextElement = UIComponentAddTextElementNoCopy(
						pNameDisp,
						pTexture,
						pFont,
						nBaseFontSize,
						puszName,
						pos,
						UI_MAKE_COLOR( bAlpha, dwTextColor ),
						NULL,
						UIALIGN_TOP,
						&tSize,
						TRUE,
						0,
						fWidthRatio);
				}
				else
				{
					pUnitDisplay[ i ].pTextElement = UIComponentAddTextElement(
						pNameDisp,
						pTexture,
						pFont,
						nBaseFontSize,
						puszName,
						pos,
						UI_MAKE_COLOR( bAlpha, dwTextColor ),
						NULL,
						UIALIGN_TOP,
						&tSize,
						TRUE,
						0,
						fWidthRatio);
				}

				if( AppIsHellgate() )
				{
					if (ObjectIsWarp(pUnit))
						pUnitDisplay[ i ].pTextElement->m_fZDelta = 0.0f;
					else
						pUnitDisplay[ i ].pTextElement->m_fZDelta = fZDelta;

					if (szPlayerInfo[0])
					{
						float fNameHeight = 0.0f;
						UIElementGetTextLogSize(
							pFont,
							nBaseFontSize,
							fWidthRatio,
							pNameDisp->m_bNoScaleFont,
							puszName,
							NULL,
							&fNameHeight);

						UI_POSITION textPos = pos;
						textPos.m_fY = pos.m_fY + fNameHeight;

						UI_GFXELEMENT *pTextElement = UIComponentAddTextElement(
							pNameDisp,
							pTexture,
							pFont,
							pNameDisp->m_nPlayerInfoFontsize,
							szPlayerInfo,
							textPos,
							UI_MAKE_COLOR( bAlpha, GFXCOLOR_WHITE ),
							NULL,
							UIALIGN_TOP,
							&tSize,
							TRUE,
							0,
							fWidthRatio);

						pTextElement->m_fZDelta = fZDelta;
					}
				}

				//WCHAR szTemp2[2048];
				//PStrPrintf(szTemp2, 2045, L"v = (%0.2f, %0.2f, %0.2f) p = (%0.2f, %0.2f, %0.2f)",
				//	pUnitDisplay[i].vScreenPos.fX, pUnitDisplay[i].vScreenPos.fY, pUnitDisplay[i].vScreenPos.fZ, pos.m_fX, pos.m_fY, pUnitDisplay[ i ].pTextElement->m_fZDelta);
				//UIComponentAddTextElement(pNameDisp, NULL, UIComponentGetFont(pNameDisp), nBaseFontSize, szTemp2, UI_POSITION(pos.m_fX, pos.m_fY - (float)nBaseFontSize - 3.0f));

//				if (UIGetShowItemNamesState())
				if( AppIsTugboat() && ( UnitIsA( pUnit, UNITTYPE_OBJECT) ||
										  UnitIsA( pUnit, UNITTYPE_DESTRUCTIBLE) ||
										  UnitTestFlag( pUnit, UNITFLAG_CANBEPICKEDUP ) ) &&
					pUnit == UIGetTargetUnit() )
				{
					iActualSelection = i;
					pActualSelection = pUnit;
				}

				if (UnitIsA( pUnit, UNITTYPE_ITEM ) &&
					pUnitDisplay[i].fPlayerDistSq <= PICKUP_RADIUS_SQ)
				{
					UIElementGetScreenRect(pUnitDisplay[ i ].pTextElement, &rectElement);
					if (UIInRect(posCursor, rectElement) &&
						pUnitDisplay[ i ].pTextElement->m_fZDelta < fClosestZ)
					{
						fClosestZ = pUnitDisplay[ i ].pTextElement->m_fZDelta;
						iClosest = i;
						UISetAltTargetUnit( UnitGetId( pUnit ) );
					}
					else if (!bCursorActive && UIInRect(rectElement, rectScreen))
					{
						float fDist = UIGetDistanceSquared(posCursor, UI_POSITION(pos.m_fX + (pUnitDisplay[i].fTextWidth / 2.0f), pos.m_fY + (pUnitDisplay[i].fTextHeight / 2.0f)));
						if (fClosestZ == 1000.0f && fDist < fClosestPos)
						{
							fClosestPos = fDist;
							iClosest = i;
							UISetAltTargetUnit( UnitGetId( pUnit ) );
						}
					}

				}

			}

		}
		if( iClosest == -1 && iActualSelection > -1 )
		{
			iClosest = iActualSelection;
			//UISetAltTargetUnit( UnitGetId( pActualSelection ) );
		}

		if (UIGetShowItemNamesState() ||
			(AppIsTugboat() && iClosest != -1 ) )
		{
			if (iClosest != -1)
			{
				pNameDisp->m_idUnitNameUnderCursor = pUnitDisplay[iClosest].nUnitID;
				pNameDisp->m_fUnitNameUnderCursorDistSq = pUnitDisplay[iClosest].fPlayerDistSq;
			}

			for (int i=0; i< nUnits; i++)
			{
				if (pUnitDisplay[ i ].pTextElement && pUnitDisplay[i].bIsItem)
				{
					BYTE bAlpha = 92;

					if ( pUnitDisplay[i].fPlayerDistSq <= PICKUP_RADIUS_SQ )
					{
						bAlpha = (i == iClosest ? 255 : 192);
					}
					pUnitDisplay[ i ].pTextElement->m_dwColor = UI_MAKE_COLOR(bAlpha, pUnitDisplay[ i ].pTextElement->m_dwColor);

					UNIT *pUnit = UnitGetById(pGame, pUnitDisplay[i].nUnitID);
					UI_TEXTURE_FRAME *pIconFrame = NULL;
					int nColorIcon;
					int nBorderColor = UIInvGetItemGridBorderColor(pUnit, pNameDisp, &pIconFrame, &nColorIcon);

					UI_POSITION pos = pUnitDisplay[ i ].pTextElement->m_Position;
					UI_SIZE tSize (pUnitDisplay[i].fTextWidth, pUnitDisplay[i].fTextHeight);

					float fIconWidth = (pIconFrame ? (pIconFrame->m_fWidth * pUnitDisplay[i].fLabelScale) + 5.0f : 0.0f);
					tSize.m_fWidth += fIconWidth;

					if (i == iClosest)
					{
						pUnitDisplay[ iClosest ].pTextElement->m_fZDelta = 0.0f;

						if ( AppIsHellgate() && pNameDisp->m_pPickupLine)
						{
							float fWidthRatio = AppIsHellgate() ? pUnitDisplay[i].fLabelScale : 1.0f;

							float fPickupTextWidth = 0.0f;
							float fPickupTextHeight = 0.0f;
							UIElementGetTextLogSize(pFont,
								nBaseFontSize,
								fWidthRatio / 1.4f,
								pNameDisp->m_bNoScaleFont,
								pNameDisp->m_pPickupLine->GetText(),
								&fPickupTextWidth,
								&fPickupTextHeight);

							if (tSize.m_fWidth < fPickupTextWidth + fIconWidth)
							{
								pos.m_fX -= ((fPickupTextWidth + fIconWidth) - tSize.m_fWidth) / 2.0f;
							}
							tSize.m_fHeight += fPickupTextHeight;
							tSize.m_fWidth = MAX(tSize.m_fWidth, fPickupTextWidth + fIconWidth);

							UI_GFXELEMENT *pTextElement = UIComponentAddTextElement(
								pNameDisp,
								pTexture,
								pFont,
								(int)(nBaseFontSize / 1.5f),
								pNameDisp->m_pPickupLine->GetText(),
								pos,
								UI_MAKE_COLOR( bAlpha, GFXCOLOR_STD_UI_OUTLINE ),
								NULL,
								UIALIGN_BOTTOM,
								&tSize,
								TRUE,
								0,
								fWidthRatio);
							if( AppIsHellgate() )
							{
								pTextElement->m_fZDelta = pUnitDisplay[ i ].pTextElement->m_fZDelta;
							}
						}
					}

					static const float BORDER_SIZE = 10.0f;

					float fBorderSize = AppIsHellgate() ? ( BORDER_SIZE * pUnitDisplay[i].fLabelScale ) : pUnitDisplay[i].fLabelScale;
					tSize.m_fHeight += fBorderSize * 2.0f;
					tSize.m_fWidth += fBorderSize * 2.0f;
					UI_POSITION rectpos(pos.m_fX - fBorderSize, pos.m_fY - fBorderSize);

					DWORD dwBorderColor = (nBorderColor != INVALID_LINK ? GetFontColor(nBorderColor) : GFXCOLOR_WHITE);
					if( AppIsHellgate() )
					{
						dwBorderColor = UI_MAKE_COLOR( bAlpha, dwBorderColor);
					}
					else
					{
						if( i == iClosest )
						{
							dwBorderColor = UI_MAKE_COLOR( bAlpha, GFXCOLOR_WHITE);
						}
						else
						{
							dwBorderColor = UI_MAKE_COLOR( bAlpha, GFXCOLOR_BLACK);
						}
					}

					if( AppIsHellgate() || !UnitIsA( pUnit, UNITTYPE_PLAYER ) )
					{
						UI_GFXELEMENT *pRectElement = UIComponentAddElement(pNameDisp, pTexture, pNameDisp->m_pItemBorderFrame, rectpos, dwBorderColor, NULL, FALSE, 1.0f, 1.0f, &tSize);
						if( AppIsHellgate() )
						{
							pRectElement->m_fZDelta = pUnitDisplay[ i ].pTextElement->m_fZDelta;
						}
						if (pIconFrame)
						{
							UI_POSITION posIcon(tSize.m_fWidth - fIconWidth, (tSize.m_fHeight - (pIconFrame->m_fHeight * pUnitDisplay[i].fLabelScale)) / 2.0f);
							posIcon += rectpos;
							UI_GFXELEMENT *pIconElement = UIComponentAddElement(pNameDisp, pTexture, pIconFrame, posIcon, UI_MAKE_COLOR(bAlpha, GetFontColor(nColorIcon)), NULL, FALSE, pUnitDisplay[i].fLabelScale, pUnitDisplay[i].fLabelScale );
							pIconElement->m_fZDelta = pUnitDisplay[ i ].pTextElement->m_fZDelta;
						}
					}


					//WCHAR szBuf[256];
					//PStrPrintf(szBuf, arrsize(szBuf), L"scl=%0.2f tw=%0.2f iw=%0.2f bw=%0.2f",
					//	pUnitDisplay[i].fLabelScale,
					//	pUnitDisplay[i].fTextWidth,
					//	fIconWidth,
					//	tSize.m_fWidth);
					//UIComponentAddTextElement(pNameDisp, NULL, UIComponentGetFont(pNameDisp), nBaseFontSize, szBuf, UI_POSITION(rectpos.m_fX, rectpos.m_fY - (float)nBaseFontSize));
				}
				else if ( AppIsTugboat() &&
						  pUnitDisplay[ i ].pTextElement )
				{
					UNIT *pUnit = UnitGetById(pGame, pUnitDisplay[i].nUnitID);
					if( !UnitIsA( pUnit, UNITTYPE_PLAYER ) )
					{
						BYTE bAlpha = 192;

						if ( pUnitDisplay[i].fPlayerDistSq <= PICKUP_RADIUS_SQ )
						{
							bAlpha = (i == iClosest ? 255 : 192);
						}
						pUnitDisplay[ i ].pTextElement->m_dwColor = UI_MAKE_COLOR(bAlpha, pUnitDisplay[ i ].pTextElement->m_dwColor);

						UI_POSITION pos = pUnitDisplay[ i ].pTextElement->m_Position;
						UI_SIZE tSize (pUnitDisplay[i].fTextWidth, pUnitDisplay[i].fTextHeight);

						static const float BORDER_SIZE = 10.0f;

						float fBorderSize = pUnitDisplay[i].fLabelScale;
						tSize.m_fHeight += fBorderSize * 2.0f;
						tSize.m_fWidth += fBorderSize * 2.0f * pUnitDisplay[i].fLabelScale;
						UI_POSITION rectpos(pos.m_fX - fBorderSize, pos.m_fY - fBorderSize);

						DWORD dwBorderColor = GFXCOLOR_WHITE;

						if( i == iClosest )
						{
							dwBorderColor = UI_MAKE_COLOR( bAlpha, GFXCOLOR_WHITE);
						}
						else
						{
							dwBorderColor = UI_MAKE_COLOR( bAlpha, GFXCOLOR_BLACK);
						}

						UIComponentAddElement(pNameDisp, pTexture, pNameDisp->m_pItemBorderFrame, rectpos, dwBorderColor, NULL, FALSE, 1.0f, 1.0f, &tSize);

					}


				}

			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDebugLabelsClear()
{
	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_DEBUG_LABELS);
	if ( ! component )
		return;

	// clear display
	UIComponentRemoveAllElements(component);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UIDebugLabelAddScreen(
	const WCHAR * puszText,
	const VECTOR * pvScreenPos,
	float fScale,
	DWORD dwColor,
	UI_ALIGN eAnchor,
	UI_ALIGN eAlign,
	BOOL bPinToEdge )
{
	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_DEBUG_LABELS);
	if ( ! component )
		return;
	ASSERT_RETURN(puszText);
	ASSERT_RETURN(pvScreenPos);

	VECTOR vScreenPos = *pvScreenPos;

	//UIComponentSetVisible( component->m_pParent, TRUE );
	//UIComponentSetVisible( component, TRUE );

	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
	int nBaseFontSize = UIComponentGetFontSize(component);
	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);

	float fMaxX = UIDefaultWidth();
	float fMaxY = UIDefaultHeight();
	float fMinX = 0.0f;
	float fMinY = 0.0f;


//!!! BSP this scale stuff may need to be looked at again

	//static const float scfLongRange = 50.0f;
	//float fScale = MAX(((scfLongRange - (pUnitDisplay[i].fDistance * 2.0f)) / (scfLongRange - 10.0f)), 0.4f);
	//fScale *= UnitGetLabelScale( pUnit );
	float fFontSize = (float)nBaseFontSize;
	UI_POSITION pos;
	pos.m_fX = ( vScreenPos.fX + 1.0f) * 0.5f * UIDefaultWidth();
	pos.m_fY = (-vScreenPos.fY + 1.0f) * 0.5f * UIDefaultHeight();

	float fTextWidth = 0.0f;
	float fTextHeight = 0.0f;
	UIElementGetTextLogSize(pFont,
		nBaseFontSize,
		1.0f,
		component->m_bNoScaleFont,
		puszText,
		&fTextWidth,
		&fTextHeight);

	fTextWidth  *= UIGetScreenToLogRatioX( component->m_bNoScaleFont );
	fTextHeight *= UIGetScreenToLogRatioY( component->m_bNoScaleFont );

	// anchor x
	switch (eAnchor)
	{
	case UIALIGN_TOPLEFT:
	case UIALIGN_LEFT:
	case UIALIGN_BOTTOMLEFT:
		//pos.m_fX -= fTextWidth;
		break;
	case UIALIGN_TOPRIGHT:
	case UIALIGN_RIGHT:
	case UIALIGN_BOTTOMRIGHT:
		pos.m_fX -= fTextWidth;
		break;
	case UIALIGN_TOP:
	case UIALIGN_BOTTOM:
	case UIALIGN_CENTER:
	default:
		pos.m_fX -= fTextWidth * 0.5f;
	}

	// anchor y
	switch (eAnchor)
	{
	case UIALIGN_TOPLEFT:
	case UIALIGN_TOP:
	case UIALIGN_TOPRIGHT:
		//pos.m_fY -= fTextHeight;
		break;
	case UIALIGN_BOTTOMLEFT:
	case UIALIGN_BOTTOM:
	case UIALIGN_BOTTOMRIGHT:
		pos.m_fY -= fTextHeight;
		break;
	case UIALIGN_LEFT:
	case UIALIGN_CENTER:
	case UIALIGN_RIGHT:
	default:
		pos.m_fY -= fTextHeight * 0.5f;
	}


	// if label would be off the screen, pin it to the edges.
	if ( bPinToEdge )
	{
		pos.m_fX = PIN(pos.m_fX, fMinX, fMaxX - fTextWidth);
		pos.m_fY = PIN(pos.m_fY, fMinY, fMaxY - fTextHeight);

		// if it's behind you, pin to the edge of the screen
		if (vScreenPos.fZ >= 1.0f)
		{
			vScreenPos.fZ = 0.01f;
			if (vScreenPos.fX < 0.0f)
				pos.m_fX = fMaxX - fTextWidth;
			else
				pos.m_fX = fMinX;
		}
	}
	else if (vScreenPos.fZ >= 1.0f)
	{
		return;
	}


	// add text
	UI_GFXELEMENT * pElement = UIComponentAddTextElement(
		component,
		pTexture,
		pFont,
		(int)fFontSize,
		puszText,
		pos,
		dwColor,
		NULL,
		eAlign,
		&UI_SIZE(fTextWidth, fTextHeight));
	//pElement->m_fZDelta = e_UIConvertAbsoluteZToDeltaZ( vScreenPos.fZ );
	pElement->m_fZDelta = 0.f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDebugLabelAddWorld(
	const WCHAR * puszText,
	const VECTOR * pvWorldPos,
	float fScale,
	DWORD dwColor,
	UI_ALIGN eAnchor,
	UI_ALIGN eAlign,
	BOOL bPinToEdge )
{
	VECTOR vScreenPos;
	MATRIX mProj, mView;
	V( e_GetWorldViewProjMatricies( NULL, &mView, &mProj ) );

	V( VectorProject( &vScreenPos, pvWorldPos, &mProj, &mView ) );

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	vScreenPos.x /= nWidth;
	vScreenPos.y /= nHeight;

	vScreenPos.x -= 0.5f;
	vScreenPos.x *= 2.f;

	vScreenPos.y -= 0.5f;
	vScreenPos.y *= -2.f;

	UIDebugLabelAddScreen(
		puszText,
		&vScreenPos,
		fScale,
		dwColor,
		eAnchor,
		eAlign,
		bPinToEdge );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIUnitNameDisplayClick(
	int x,
	int y)
{
	// might use the x & y later, but not now

	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_UNIT_NAMES);
	if (component)
	{
		return ResultStopsProcessing( UIUnitNameDisplayOnClick(component, UIMSG_LBUTTONCLICK, 0, 0) );
	}

	return 0;
}

//----------------------------------------------------------------------------
// vvv Tugboat-specific
#define MAX_NAME_LEN (128)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDisplayNPCDialogFromText(
	GAME *pGame,
	UNITID idTalkingTo,
	const WCHAR *puszText,
	const WCHAR *puszReward)
{
	UNIT *pUnitTalkingTo = UnitGetById( pGame, idTalkingTo );

	if (pUnitTalkingTo)
	{

		UIShowGenericDialog(GlobalStringGet( GS_MENU_HEADER_ALERT ), puszText);

	}

}

// ^^^ Tugboat-specific
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIDoNPCDialogReplacements(
	UNIT *pPlayer,
	WCHAR *uszDest,
	int nStrLen,
	const WCHAR *puszInputText)
{
	ASSERT_RETFALSE(pPlayer);
	ASSERT_RETFALSE(uszDest);

    // copy input text, otherwise we will just do the replacement on uszDest as is
    if (puszInputText)
    {
		PStrCopy( uszDest, puszInputText, nStrLen );
	}

    // get player name
    WCHAR uszPlayerName[ MAX_CHARACTER_NAME ];
    PStrCopy( uszPlayerName, L"", arrsize(uszPlayerName) );
    UnitGetName( pPlayer, uszPlayerName, arrsize(uszPlayerName), 0 );

    // replace character name
    PStrReplaceToken( uszDest, nStrLen, StringReplacementTokensGet( SR_PLAYER_NAME ), uszPlayerName );

    const WCHAR * pszClassName = NULL;
    int monster_class = INVALID_LINK;
    if (UnitIsA( pPlayer, UNITTYPE_TEMPLAR ))
    {
		pszClassName = GlobalStringGet( GS_TEMPLAR_CLASS );
		monster_class = GlobalIndexGet( GI_MONSTER_BRANDON_LANN );
	}
	else if (UnitIsA( pPlayer, UNITTYPE_CABALIST ))
	{
		pszClassName = GlobalStringGet( GS_CABALIST_CLASS );
		monster_class = GlobalIndexGet( GI_MONSTER_ALAY_PENN );
	}
	else if (UnitIsA( pPlayer, UNITTYPE_HUNTER ))
	{
		pszClassName = GlobalStringGet( GS_HUNTER_CLASS );
		monster_class = GlobalIndexGet( GI_MONSTER_NASIM );
	}

	if (pszClassName)
	{
		PStrReplaceToken( uszDest, nStrLen, StringReplacementTokensGet( SR_PLAYER_CLASS ), pszClassName );
	}

	if (monster_class != INVALID_LINK)
	{
		const UNIT_DATA * pData = MonsterGetData( NULL, monster_class );
		if ( pData )
		{
			const WCHAR * pszNPCName = StringTableGetStringByIndex( pData->nString );
			PStrReplaceToken( uszDest, nStrLen, StringReplacementTokensGet( SR_PLAYER_CLASS_NPC ), pszNPCName );
		}
	}

    // replace area name
    LEVEL * level = UnitGetLevel( pPlayer );
    if ( level )
    {
	    const LEVEL_DEFINITION * leveldef_current = LevelDefinitionGet( level->m_nLevelDef );
	    ASSERT_RETFALSE( leveldef_current );
	    const LEVEL_DEFINITION * leveldef_next = LevelDefinitionGet( leveldef_current->nNextLevel);
	    if ( leveldef_next )
	    {
		    const WCHAR * puszNextName = StringTableGetStringByIndex( leveldef_next->nLevelDisplayNameStringIndex );
		    PStrReplaceToken( uszDest, nStrLen, StringReplacementTokensGet( SR_NEXT_AREA ), puszNextName );
	    }
    }

	return TRUE;
}
#define AREA_INVALID 10000
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDisplayNPCDialog(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pSpec, "Expected NPC dialog spec" );

	UIHideNPCVideo();

	UNITID idTalkingTo = pSpec->idTalkingTo;
	UNIT *pUnitTalkingTo = UnitGetById( pGame, idTalkingTo );
	UNIT *pPlayer = GameGetControlUnit( pGame );

	if (pUnitTalkingTo && pPlayer)
	{

		//const WCHAR *puszText = L"This is page 1.[pagebreak]\
		//						 [um m=npc_generic_talk_cycle]This should be generic talk.[pagebreak][endum m=npc_generic_talk_cycle]\
		//						 [um m=npc_angry_talk_cycle]This should be angry talk.[pagebreak][endum m=npc_angry_talk_cycle]\
		//						 [um m=npc_sad_talk_cycle]This should be sad talk.[pagebreak][endum m=npc_sad_talk_cycle]\
		//						 [um m=npc_scared_talk_cycle]This should be scared talk.[pagebreak][endum m=npc_scared_talk_cycle]\
		//						 [um m=npc_excited_talk_cycle]This should be excited talk.[pagebreak][endum m=npc_excited_talk_cycle]\
		//						 This is the last page.";

		// get text, this must take precedence
		const WCHAR *puszText = NULL;
		if (pSpec->puszText != NULL)
		{
			puszText = pSpec->puszText;
		}
		else if (pSpec->nDialog != INVALID_LINK)
		{
			puszText = c_DialogTranslate( pSpec->nDialog );
		}
		else if (pSpec->eType == NPC_DIALOG_ANALYZE)
		{
			int nExamineString = pUnitTalkingTo->pUnitData->nAnalyze;
			ASSERTX_RETURN( nExamineString != INVALID_LINK, "No examine string provided for NPC" );
			puszText = StringTableGetStringByIndex( nExamineString );
		}

		if (pSpec->eType != NPC_DIALOG_LIST )
		{
			ASSERTX_RETURN( puszText, "No conversation text for NPC dialog specified" );
		}

		// get reward text
		const WCHAR *puszReward = NULL;
		if (pSpec->puszRewardText != NULL)
		{
			puszReward = pSpec->puszRewardText;
		}
		else if (pSpec->nDialogReward != INVALID_LINK)
		{
			puszReward = c_DialogTranslate( pSpec->nDialogReward );
		}

	    if (AppIsTugboat() )
	    {
			UI_TB_HideTravelMapScreen( TRUE );
			UI_TB_HideWorldMapScreen( TRUE );
			UIHideGenericDialog();
			UI_COMPONENT * pDlgBackground = UIComponentGetByEnum(UICOMP_NPCDIALOGBOX);
			ASSERT_RETURN(pDlgBackground);

			UI_COMPONENT *pDialogBox = UIComponentFindChildByName(pDlgBackground, "Dialog Box NPC");
			if( pSpec->nQuestDefId != INVALID_ID )
			{
				WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
				int nStringLength = PStrLen( puszText );
				ASSERT_RETURN( MAX_STRING_ENTRY_LENGTH >= nStringLength );
				PStrCopy( newText, MAX_STRING_ENTRY_LENGTH, puszText, nStringLength + 1 );
				c_QuestFillFlavoredText( QuestTaskDefinitionGet( pSpec->nQuestDefId ), newText, MAX_STRING_ENTRY_LENGTH, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES );	
				UILabelSetText( pDialogBox, newText );
			}
			else if( puszText )
			{
				UILabelSetText( pDialogBox, puszText );
			}
			else
			{
				UILabelSetText( pDialogBox, L"" );
			}

			UIComponentSetActive(pDlgBackground, TRUE);

			/*
			if( pSpec->nQuestDefId != INVALID_ID )
			{
				c_QuestFillRandomQuestTagsUIDialogText( pDialogBox, QuestTaskDefinitionGet( pSpec->nQuestDefId ) );
			}
			*/
			c_QuestRewardUpdateUI( pDlgBackground,
								   pSpec->eType == NPC_DIALOG_OK_REWARD,
								   FALSE,
								   pSpec->nQuestDefId );


			//UISetMouseOverrideComponent(pDlgBackground);



			UI_COMPONENT *pAcceptButton = UIComponentFindChildByName(pDlgBackground, "dialog accept");
			UI_COMPONENT *pAcceptButtonLabel = UIComponentFindChildByName(pAcceptButton, "button dialog ok label");
			ASSERT_RETURN(pAcceptButton);
			ASSERT_RETURN(pAcceptButtonLabel);
		    pAcceptButton->m_tCallbackOK = pSpec->tCallbackOK;
			UILabelSetText(pAcceptButtonLabel, L"Accept");

			UI_COMPONENT *pCancelButton = UIComponentFindChildByName(pDlgBackground, "dialog cancel");
			ASSERT_RETURN(pCancelButton);
			UI_COMPONENT *pCancelButtonLabel = UIComponentFindChildByName(pCancelButton, "button dialog cancel label");
			ASSERT_RETURN(pCancelButtonLabel);
			pCancelButton->m_tCallbackCancel = pSpec->tCallbackCancel;
			UILabelSetText(pCancelButtonLabel, L"Decline");


			UIComponentSetActive( pAcceptButton, TRUE );


			UI_COMPONENT* pDialogOptions = UIComponentFindChildByName( pDlgBackground, "dialog topics" );
			if( pDialogOptions )
			{
				UIComponentSetVisible( pDialogOptions, FALSE );
			}
			pDialogOptions = UIComponentFindChildByName( pDlgBackground, "dialog locations" );
			if( pDialogOptions )
			{
				UIComponentSetVisible( pDialogOptions, FALSE );
			}

			// TRAVIS : UGggh, this is such a mess. Must clean up.
			if( pSpec->eType == NPC_DIALOG_LIST )
			{
				UNIT* pNPC = UnitGetById( pGame, idTalkingTo );
				if( pNPC )
				{
					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskArray[MAX_NPC_QUEST_TASK_AVAILABLE];
					int nQuestTaskCount = QuestGetAvailableNPCQuestTasks( UnitGetGame( pPlayer ),
						pPlayer,
						pNPC,
						pQuestTaskArray,
						FALSE,
						0 );

					int nQuests = QuestGetAvailableNPCSecondaryDialogQuestTasks( UnitGetGame( pPlayer ),
						pPlayer,
						pNPC,
						pQuestTaskArray,
						nQuestTaskCount );

					UI_COMPONENT* pDialogOptions = UIComponentFindChildByName( pDlgBackground, "dialog topics" );
					if( pDialogOptions )
					{
						UILabelSetText(pAcceptButtonLabel, L"Choose");
						UILabelSetText(pCancelButtonLabel, L"Cancel");
						UIComponentSetActive( pDialogOptions, TRUE );
						int Index = 0;
						UI_COMPONENT* pChild = pDialogOptions->m_pFirstChild;
						while (pChild)
						{
							if( Index < nQuests )
							{
								UI_LABELEX *pLabel = UICastToLabel(pChild);

								WCHAR uszChoice[ MAX_STRING_ENTRY_LENGTH ];
								int QuestState = c_GetQuestNPCState( pGame,
																		pPlayer,
																		pNPC,
																		pQuestTaskArray[Index] );
								PStrPrintf(uszChoice, MAX_STRING_ENTRY_LENGTH, L"");
								switch( QuestState )
								{
								case STATE_NPC_INFO_WAITING :
									UIColorCatString(uszChoice, MAX_STRING_ENTRY_LENGTH, FONTCOLOR_LIGHT_GRAY, L"? - ");
									break;
								case STATE_NPC_INFO_NEW :
									UIColorCatString(uszChoice, MAX_STRING_ENTRY_LENGTH, FONTCOLOR_LIGHT_YELLOW, L"! - ");
									break;
								case STATE_NPC_INFO_RETURN :
									UIColorCatString(uszChoice, MAX_STRING_ENTRY_LENGTH, FONTCOLOR_LIGHT_YELLOW, L"? - ");
									break;
								case STATE_NPC_INFO_LEVEL_TO_LOW :
									UIColorCatString(uszChoice, MAX_STRING_ENTRY_LENGTH, FONTCOLOR_LIGHT_GRAY, L"! - ");
									break;
								}
								//PStrPrintf( uszChoice, MAX_STRING_ENTRY_LENGTH, L"Topic - " );
								int nStringId = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTaskArray[Index], KQUEST_STRING_TITLE );
								PStrCat(uszChoice, StringTableGetStringByIndex( nStringId ), MAX_STRING_ENTRY_LENGTH );
								c_QuestFillFlavoredText( pQuestTaskArray[Index], INVALID_ID, uszChoice, MAX_STRING_ENTRY_LENGTH, 0 );
								UILabelSetText( pLabel, uszChoice );


								pLabel->m_dwParam = pQuestTaskArray[Index]->nParentQuest;
								Index++;
								UIComponentSetActive( pChild, TRUE );
								UIComponentSetFocusUnit( pChild, UnitGetId( pNPC ) );
							}
							else
							{
								UIComponentSetVisible( pChild, FALSE );
							}
							pChild = pChild->m_pNextSibling;
						}
					}
					UIComponentSetActive( pAcceptButton, FALSE );
				}

			}
			else if( pSpec->eType == NPC_LOCATION_LIST )
			{
				UNIT* pNPC = UnitGetById( pGame, idTalkingTo );
				if( pNPC )
				{
					UI_COMPONENT* pDialogOptions = UIComponentFindChildByName( pDlgBackground, "dialog locations" );
					if( pDialogOptions )
					{
						const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( LevelGetLevelAreaID( UnitGetLevel( UIGetControlUnit() ) ) );

						UILabelSetText(pAcceptButtonLabel, L"Choose");
						UILabelSetText(pCancelButtonLabel, L"Cancel");
						UIComponentSetActive( pDialogOptions, TRUE );
						int Index = 0;
						UI_COMPONENT* pChild = pDialogOptions->m_pFirstChild;
						while (pChild)
						{
							if( Index < pLevelArea->nNumTransporterAreas )
							{
								UI_LABELEX *pLabel = UICastToLabel(pChild);

								WCHAR uszChoice[ MAX_STRING_ENTRY_LENGTH ];
								const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelWarpArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( pLevelArea->nTransporterAreas[Index] );

								MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( pLevelArea->nTransporterAreas[Index], -1, uszChoice, MAX_STRING_ENTRY_LENGTH );
								pLabel->m_dwParam = pLevelArea->nTransporterAreas[Index];
								if( pLevelWarpArea->nAreaWarpLevelRequirement > UnitGetExperienceLevel( UIGetControlUnit() ) )
								{
									WCHAR szBuf[256];
									PStrPrintf(szBuf, arrsize(szBuf), StringTableGetStringByKey("warp_level_requirement"), pLevelWarpArea->nAreaWarpLevelRequirement);
									PStrCat( uszChoice, szBuf, MAX_STRING_ENTRY_LENGTH );
									pLabel->m_dwParam = AREA_INVALID;
								}

								UILabelSetText( pLabel, uszChoice );
								Index++;
								UIComponentSetActive( pChild, TRUE );
								UIComponentSetFocusUnit( pChild, UnitGetId( pNPC ) );
							}
							else
							{
								UIComponentSetVisible( pChild, FALSE );
							}
							pChild = pChild->m_pNextSibling;
						}
					}
					UIComponentSetActive( pAcceptButton, FALSE );
				}

			}

			// the Accept button is disabled by default if you must select from multiple rewards
			if( pSpec->eType == NPC_DIALOG_OK_REWARD )
			{
				// make sure no tabs are selected to start
				UI_PANELEX* pRewardPane = UICastToPanel( UIComponentFindChildByName( pDlgBackground, "reward panel main element" ) );
				UIPanelSetTab( pRewardPane, -1 );

				// unpush all buttons
				UI_COMPONENT *pOtherButton = NULL;
				int VisibleRewardButtons = 0;
				while ((pOtherButton = UIComponentIterateChildren(pRewardPane, pOtherButton, UITYPE_BUTTON, TRUE )) != NULL)
				{
					UI_BUTTONEX *pBtn = UICastToButton(pOtherButton);
					if( UIComponentGetVisible( pOtherButton) )
					{
						VisibleRewardButtons++;
					}
					if (pBtn->m_eButtonStyle == UIBUTTONSTYLE_RADIOBUTTON)
					{
						UIButtonSetDown( pBtn, FALSE );
						UIComponentHandleUIMessage( pOtherButton, UIMSG_PAINT, 0, 0);	//painting at the end now
					}
				}
				if( VisibleRewardButtons > 0 )
				{
					UIComponentSetActive( pAcceptButton, FALSE );
					UILabelSetText(pAcceptButtonLabel, L"Complete");
				}
				UILabelSetText(pCancelButtonLabel, L"Cancel");
			}
			UIComponentHandleUIMessage( pAcceptButton, UIMSG_PAINT, 0, 0);		//painting at the end now


			if ( pCancelButton )
			{
				if ( ( pSpec->eType == NPC_DIALOG_OK ) || ( pSpec->eType == NPC_DIALOG_ANALYZE ) )
				{
					UILabelSetText(pAcceptButtonLabel, L"Continue");
					UIComponentSetVisible( pCancelButton, FALSE );
				}
				else
				{
					UIComponentSetActive( pCancelButton, TRUE );
				}
			}
			   // UIDialogBox( puszText, pSpec->eType != NPC_DIALOG_OK, pSpec->tCallbackOK.pfnCallback );
			    //UIDisplayNPCDialogFromText( pGame, idTalkingTo, puszText, L"" );
		}
		else if (AppIsHellgate())
		{

		    UI_COMPONENT *pComp =  UIComponentGetByEnum( UICOMP_NPC_CONVERSATION_DIALOG );
		    if (!pComp)
		    {
			    return;
		    }

		    // display the conversation UI
		    UI_CONVERSATION_DLG *pDialog = UICastToConversationDlg(pComp);

		    // save dialog being spoken (if any)
		    pDialog->m_nDialog = pSpec->nDialog;
			pDialog->m_bLeaveUIOpenOnOk = pSpec->bLeaveUIOpenOnOk;

			sConvDlgFreeClientOnlyUnits(pDialog);			// free any previous unit
			pDialog->m_idTalkingTo[0] = idTalkingTo;
			pDialog->m_idTalkingTo[1] = INVALID_ID;

			if (pSpec->eGITalkingTo != GI_INVALID)
			{
				pDialog->m_idTalkingTo[1] = UICreateClientOnlyUnit(AppGetCltGame(), GlobalIndexGet(pSpec->eGITalkingTo));
			}

		    // set focus of dialog
		    UIComponentSetFocusUnit(pDialog, idTalkingTo);

		    // turn portrait on/off
		    UI_COMPONENT * pPortrait = UIComponentFindChildByName(pDialog, "dialog portrait");
			if ( !pPortrait )
			{
				return;
			}
			if ( !UnitHasState( pGame, pUnitTalkingTo, STATE_NPC_TALK_TO ) )
			{
				c_StateSet( pUnitTalkingTo, pPlayer, STATE_NPC_TALK_TO, 0, 0, INVALID_ID );
			}
			int nModelID = c_UnitGetPaperdollModelId( pUnitTalkingTo );
			REF(nModelID);
			int nAppearanceDefID = UnitGetAppearanceDefId( pUnitTalkingTo, TRUE );
			REF(nAppearanceDefID);
			BOOL bShowPortrait = !UnitDataTestFlag(pUnitTalkingTo->pUnitData, UNIT_DATA_FLAG_HIDE_DIALOG_HEAD);
			if ( UnitGetGenus( pUnitTalkingTo ) != GENUS_MONSTER )
				bShowPortrait = FALSE;
			//if (bShowPortrait)
			//{
			//	UIComponentHandleUIMessage(pPortrait, UIMSG_PAINT, 0, 0);		//painting at the end now
			//}
			UIComponentSetVisible( pPortrait, bShowPortrait );

		    WCHAR uszDialog[ MAX_STRING_ENTRY_LENGTH ];

			// rewards text
		    if ( puszReward && PStrLen( puszReward ) )
		    {
			    PStrCat( uszDialog, L"\n\n", MAX_STRING_ENTRY_LENGTH );
				UIColorCatString(uszDialog, MAX_STRING_ENTRY_LENGTH, FONTCOLOR_LIGHT_YELLOW, puszReward);
		    }

			UIDoNPCDialogReplacements(pPlayer, uszDialog, MAX_STRING_ENTRY_LENGTH, puszText);

		    // set text
			UI_LABELEX *pLabel = UICastToLabel(UIComponentFindChildByName(pDialog, "label dialog text"));
			if (pLabel)
				UILabelSetText( pLabel, uszDialog );

			// go ahead and set the video label too, 'cause it might be switching between them.  This lets them each do their own wordwrapping on the text block
			UI_COMPONENT *pVideoDialog = UIComponentGetByEnum(UICOMP_VIDEO_DIALOG);
			if (pVideoDialog)
			{
				UI_LABELEX *pVideoLabel = UICastToLabel(UIComponentFindChildByName(pVideoDialog, "label dialog text"));
				if (pVideoLabel)
				{
					UILabelSetText(pVideoLabel, uszDialog);
				}
			}


		    UI_COMPONENT *pCancelButton = UIComponentFindChildByName( pDialog, "button dialog cancel" );
		    if ( pCancelButton )
		    {
			    if ( ( pSpec->eType == NPC_DIALOG_OK ) || ( pSpec->eType == NPC_DIALOG_ANALYZE ) )
			    {
				    UIComponentSetVisible( pCancelButton, FALSE );
			    }
			    else
			    {
				    UIComponentSetActive( pCancelButton, TRUE );
			    }
		    }

			QUEST *pQuest = (pSpec->nQuestDefId != INVALID_ID ? QuestGetQuest( pPlayer, pSpec->nQuestDefId ) : NULL);
			pDialog->m_nQuest = (pQuest ? pQuest->nQuest : INVALID_LINK);		// save a pointer to the quest

			int nOfferingID = pSpec->nOfferDefId;
			if (nOfferingID == INVALID_ID && pQuest)
			{
				const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
				ASSERT_RETURN( pQuestDef );

				nOfferingID = pQuestDef->nOfferReward;
			}
			const OFFER_DEFINITION *pOfferDef = (nOfferingID != INVALID_ID ? OfferGetDefinition( nOfferingID ) : NULL);

			UI_COMPONENT *pAcceptButtonLabel = UIComponentFindChildByName(pDialog, "button dialog ok label");
			UI_COMPONENT *pAcceptButton = pAcceptButtonLabel->m_pParent;
			if (pAcceptButtonLabel && pAcceptButton)
			{
				UILabelSetText(pAcceptButtonLabel, GlobalStringGet( GS_UI_NPC_NONQUEST_ACCEPT ));
				pAcceptButton->m_dwData = 0;		// don't try to take items

				if (pQuest && QuestIsInactive(pQuest))
				{
					if (QuestPlayCanAcceptQuest(pPlayer, pQuest))
					{
						const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
						ASSERT_RETURN( pQuestDef );

						UILabelSetText(pAcceptButtonLabel, StringTableGetStringByIndex( pQuestDef->nAcceptButtonText ) /*GlobalStringGet( GS_UI_ACCEPT_QUEST )*/);
					}
					else
					{
						UILabelSetText(pAcceptButtonLabel, GlobalStringGet( GS_TOO_MANY_QUESTS ));
					}
				}
				else if (pQuest && QuestIsActive(pQuest))
				{
				}
				else if (pOfferDef)
				{
					// add line that says how many they can pick from if necessary
					if (pOfferDef->nNumAllowedTakes != REWARD_TAKE_ALL)
					{
						int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
						int nNumTaken =
							UnitGetStat(
								pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, nOfferingID, nDifficulty );

						int nNumChoicesLeft =
							( pOfferDef->nNumAllowedTakes - nNumTaken);
						if (nNumChoicesLeft == 1)
						{
							UILabelSetText(pAcceptButtonLabel, GlobalStringGet(GS_REWARD_INSTRUCTIONS_TAKE_ONE));
							pAcceptButton->m_dwData = 1;		// try to take items
						}
						else
						{
							const WCHAR *szFormat = GlobalStringGet(GS_REWARD_INSTRUCTIONS_TAKE_SOME);
							WCHAR szBuf[256];
							PStrPrintf(szBuf, arrsize(szBuf), szFormat, nNumChoicesLeft);
							UILabelSetText(pAcceptButtonLabel, szBuf);
						}
					}
					else
					{
						UILabelSetText(pAcceptButtonLabel, GlobalStringGet(GS_REWARD_INSTRUCTIONS));
						pAcceptButton->m_dwData = 1;		// try to take items
					}
					if (AppIsHellgate())
					{
						// HACK	skip the slow reveal text thing to avoid a bug where it is canceled
						//		prematurely, and the player has to click the accept button 3 times - cmarch
						UILabelDoRevealAllText(pLabel);
					}
				}
			}

			UI_COMPONENT *pQuestTitle = UIComponentFindChildByName(pDialog, "label quest title");
			if (pQuestTitle)
			{
				if (pQuest)
				{
					UILabelSetText(pQuestTitle, StringTableGetStringByIndex( pQuest->nStringKeyName ));
				}
				else
				{
					UILabelSetText(pQuestTitle, L"");
				}
			}

			UI_COMPONENT *pRewardPanel = UIComponentFindChildByName(pDialog, "npc dialog rewards");
			if (pRewardPanel)
			{
				BOOL bHasRewards = FALSE;
				if (pQuest || nOfferingID != INVALID_ID)
				{
					WCHAR szTemp[256];
					UI_COMPONENT *pChild = UIComponentFindChildByName(pRewardPanel, "money reward");
					if (pChild)
					{
						if (pQuest)
						{
							int nMoney = QuestGetMoneyReward(pQuest);
							if (nMoney > 0)
								bHasRewards = TRUE;
							PStrPrintf(szTemp, arrsize(szTemp), L"%d", nMoney);
						}
						else
						{
							szTemp[0] = L'\0';
						}
						UILabelSetText(pChild, szTemp);
					}
					pChild = UIComponentFindChildByName(pRewardPanel, "exp reward");
					if (pChild)
					{
						if (pQuest)
						{
							int nExp = QuestGetExperienceReward(pQuest);
							if (nExp > 0)
								bHasRewards = TRUE;
							LanguageFormatIntString(szTemp, arrsize(szTemp), nExp);
							//PStrPrintf(szTemp, arrsize(szTemp), L"%d", nExp);
						}
						else
						{
							szTemp[0] = L'\0';
						}
						UILabelSetText(pChild, szTemp);
					}
					pChild = UIComponentFindChildByName(pRewardPanel, "other reward");
					if (pChild)
					{
						if (pQuest)
						{
							szTemp[0] = L'\0';
							int nFactionScoreDelta = pQuest->pQuestDef->nFactionAmountReward;
							int nFaction = pQuest->pQuestDef->nFactionTypeReward;
							if (nFactionScoreDelta != 0 && nFaction != INVALID_LINK)
							{
								bHasRewards = TRUE;
								PStrPrintf(
									szTemp,
									arrsize(szTemp),
									GlobalStringGet( GS_FACTION_SCORE_REWARD ),
									nFactionScoreDelta,
									FactionGetDisplayName( nFaction ));
							}
						}
						else
						{
							szTemp[0] = L'\0';
						}
						UILabelSetText(pChild, szTemp);
					}

					pChild = UIComponentIterateChildren(pRewardPanel, NULL, UITYPE_INVGRID, TRUE);
					UI_INVGRID *pInvGrid = (pChild ? UICastToInvGrid(pChild) : NULL);
					pInvGrid->m_nInvLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD);

					UNIT *pReward = NULL;
					int nRewardItems = 0;
					while ((pReward = UnitInventoryLocationIterate(pPlayer, pInvGrid->m_nInvLocation, pReward, 0, FALSE)) != NULL)
						nRewardItems++;

					if (nRewardItems > 0)
						bHasRewards = TRUE;

					UI_COMPONENT *pRewardHeader = UIComponentFindChildByName(pRewardPanel, "reward header");
					if (!pQuest || (pQuest && QuestIsOffering(pQuest) && nOfferingID != INVALID_ID))
					{
						bHasRewards = TRUE;
						UIComponentSetAlpha(pRewardPanel, 255, TRUE);
						pInvGrid->m_nInvLocation = InventoryGetRewardLocation(pPlayer);
						UIComponentActivate(pRewardPanel, TRUE);
						//UIShowInventoryScreen();
						//sUIShowInventoryScreen(STYLE_REWARD, INVALID_ID, INVALID_ID, INVALID_LINK, 0);

						if (pRewardHeader)
						{
							// this is probably badly named, since you are being offered the rewards now
							//   it just says "Rewards"
							UILabelSetText(pRewardHeader, GlobalStringGet(GS_REWARD_INSTRUCTIONS_PROSPECTIVE));
						}

					}
					else
					{
						if (bHasRewards)
						{
							UIComponentSetAlpha(pRewardPanel, 128, TRUE);
							UIComponentSetVisible(pRewardPanel, TRUE);
							UIComponentSetActive(pRewardPanel, FALSE);
							if (!pOfferDef || pOfferDef->nNumAllowedTakes == nRewardItems)
							{
								UILabelSetText(pRewardHeader, GlobalStringGet(GS_REWARD_PROSPECTIVE_TAKEALL));
							}
							else if (pOfferDef)
							{
								UILabelSetText(pRewardHeader, GlobalStringGet(GS_REWARD_PROSPECTIVE_TAKEONE));
							}
						}
						else
						{
							UIComponentSetVisible(pRewardPanel, FALSE);
						}
					}

					if(UnitIsA(pUnitTalkingTo, UNITTYPE_DONATOR))
					{
						UI_COMPONENT *pDonate = UIComponentFindChildByName(pDialog, "donation dialog");
						UIComponentActivate(pDonate, TRUE);
						UI_COMPONENT *pOK = UIComponentFindChildByName(pDialog, "button dialog ok");
						UIComponentSetVisible(pOK,FALSE);
						UI_COMPONENT *pRewardPanel = UIComponentFindChildByName(pDialog, "npc dialog rewards");
						UIComponentSetVisible(pRewardPanel, FALSE);

					}
					else
					{
						UI_COMPONENT *pOK = UIComponentFindChildByName(pDialog, "button dialog ok");
						UIComponentActivate(pOK,TRUE);
						UI_COMPONENT *pDonate = UIComponentFindChildByName(pDialog, "donation dialog");
						UIComponentSetVisible(pDonate, FALSE);
					}

				}

				else
				{
					UIComponentSetVisible(pRewardPanel, FALSE);
					UI_COMPONENT *pOK = UIComponentFindChildByName(pDialog, "button dialog ok");
					UIComponentActivate(pOK,TRUE);
					UI_COMPONENT *pDonate = UIComponentFindChildByName(pDialog, "donation dialog");
					UIComponentSetVisible(pDonate, FALSE);
					UI_COMPONENT *pRewardPanel = UIComponentFindChildByName(pDialog, "npc dialog rewards");
					UIComponentSetVisible(pRewardPanel, FALSE);
				}
			}

			UIConversationOnPaint(pDialog, UIMSG_PAINT, 0, 0);		// Yeah, this'll happen twice, but it needs to be called first for the children.  Won't someone think of the children!

		    // save callbacks
		    pDialog->m_tCallbackOK = pSpec->tCallbackOK;
		    pDialog->m_tCallbackCancel = pSpec->tCallbackCancel;

			//DWORD dwAnimTime = 0;
			//dwAnimTime = UIMainPanelSlideOut();

			//if (pDialog->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
			//{
			//	CSchedulerCancelEvent(pDialog->m_tMainAnimInfo.m_dwAnimTicket);
			//}

		    // turn on cursor, etc
			UIStopSkills();
//			UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);
			c_PlayerClearAllMovement(AppGetCltGame());
//			pDialog->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwAnimTime, UIDelayedActivate, CEVENT_DATA((DWORD_PTR)pDialog, UIMSG_ACTIVATE));
			UIComponentActivate(pDialog, TRUE);

	    }
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideNPCDialog(
	CONVERSATION_COMPLETE_TYPE eType)
{
	if( AppIsHellgate() )
	{
		UI_COMPONENT *pDialog =  UIComponentGetByEnum( UICOMP_NPC_CONVERSATION_DIALOG );
		if ( !pDialog )
		{
			return;
		}
		if ( !UIComponentGetActive( pDialog ) )
		{
			return;
		}
		UI_CONVERSATION_DLG *pConvDlg = UICastToConversationDlg(pDialog);

		if (!pConvDlg->m_bLeaveUIOpenOnOk || eType != CCT_OK)
		{
			UIComponentActivate(UICOMP_INVENTORY, FALSE);

			// as well as the conversation dialog
			//UIComponentSetActiveByEnum( UICOMP_NPC_CONVERSATION_DIALOG, FALSE );
			//UIComponentSetVisibleByEnum( UICOMP_NPC_CONVERSATION_DIALOG, FALSE );

			UIComponentActivate(UICOMP_NPC_CONVERSATION_DIALOG, FALSE);

			UNIT *pFocusUnit = UIComponentGetFocusUnit(pDialog);
			if (pFocusUnit)
			{
				BOUNDED_WHILE(UnitTestModeGroup(pFocusUnit, MODEGROUP_NPC_DIALOG), 3)
				{
					UnitEndModeGroup(pFocusUnit, MODEGROUP_NPC_DIALOG);
				}
			}

			sConvDlgFreeClientOnlyUnits(pConvDlg);
		}

	}
	else if( AppIsTugboat() )
	{
		UI_COMPONENT *pDialog =  UIComponentGetByEnum( UICOMP_NPCDIALOGBOX );
		if ( !pDialog )
		{
			return;
		}
		UIComponentSetVisibleByEnum( UICOMP_REWARD_PANEL, FALSE );

		if ( !UIComponentGetActive( pDialog ) )
		{
			return;
		}

		// hide the conversation UI
		UIComponentActivate( UICOMP_NPCDIALOGBOX, FALSE );
		//UIComponentSetVisibleByEnum( UICOMP_NPCDIALOGBOX, FALSE );
		//UIRemoveMouseOverrideComponent(pDialog);
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDisplayNPCStory(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec)
{
	if ( !AppIsHellgate() )
		return;

	UIHideNPCVideo();
	UISetCinematicMode( TRUE );

	UNITID idTalkingTo = pSpec->idTalkingTo;
	UNIT *pUnitTalkingTo = UnitGetById( pGame, idTalkingTo );
	UNIT *pPlayer = GameGetControlUnit( pGame );

	if (pUnitTalkingTo && pPlayer)
	{

		const WCHAR *puszText = L"This is page 1.[pagebreak]\
								 [talk]This should be generic talk.[pagebreak]\
								 [angry]This should be angry talk.[pagebreak]\
								 [sad]This should be sad talk.[pagebreak]\
								 [scared]This should be scared talk.[pagebreak]\
								 [excited]This should be excited talk.[pagebreak]\
								 [wave]This is the last page.";

		// get text, this must take precedence
/*		const WCHAR *puszText = NULL;
		if (pSpec->puszText != NULL)
		{
			puszText = pSpec->puszText;
		}
		else if (pSpec->nDialog != INVALID_LINK)
		{
			puszText = c_DialogTranslate( pSpec->nDialog );
		}*/

		ASSERTX_RETURN( puszText, "No conversation text for NPC dialog specified" );

	    // get dialog
		UI_COMPONENT *pDialog =  UIComponentGetByEnum( UICOMP_CINEMATIC_TEXT );
		if (!pDialog)
		{
			return;
		}

		// set focus of dialog
		UIComponentSetFocusUnit(pDialog, idTalkingTo);

		// set text
	    WCHAR uszDialog[ MAX_STRING_ENTRY_LENGTH ];
		UIDoNPCDialogReplacements(pPlayer, uszDialog, MAX_STRING_ENTRY_LENGTH, puszText);
		UI_LABELEX *pLabel = UICastToLabel( UIComponentFindChildByName( pDialog, "label dialog text" ) );
		if (pLabel)
			UILabelSetText( pLabel, uszDialog );

		//DWORD dwMaxAnimDuration = 0;
		//float duration = 1.0f;
		//UIComponentFadeOut(UIComponentGetByEnum(UICOMP_CINEMATIC_TEXT), duration, dwMaxAnimDuration);

		UIConversationOnPaint(pDialog, UIMSG_PAINT, 0, 0);		// Yeah, this'll happen twice, but it needs to be called first for the children.  Won't someone think of the children!
		UIComponentHandleUIMessage(pDialog, UIMSG_PAINT, 0, 0);

	    // save callbacks
	    pDialog->m_tCallbackOK = pSpec->tCallbackOK;
	    pDialog->m_tCallbackCancel = pSpec->tCallbackCancel;

		//DWORD dwAnimTime = 0;
		//dwAnimTime = UIMainPanelSlideOut();

		//if (pDialog->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
		//{
		//	CSchedulerCancelEvent(pDialog->m_tMainAnimInfo.m_dwAnimTicket);
		//}

		//pDialog->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwAnimTime, UIDelayedActivate, CEVENT_DATA((DWORD_PTR)pDialog, UIMSG_ACTIVATE));
		UIComponentActivate(pDialog, TRUE);

		// turn on cursor, etc
		UIStopSkills();
		//UIComponentSetVisibleByEnum(UICOMP_RADIAL_MENU, FALSE);
		c_PlayerClearAllMovement(AppGetCltGame());
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define VIDEO_DELAY_MIN			5
#define VIDEO_DELAY_MAX			10

//----------------------------------------------------------------------------

static BOOL sIncomingMessageCanShow(GAME * game)
{
	ASSERT_RETFALSE( game );
	ASSERT_RETFALSE( IS_CLIENT( game ) );

	static const UI_COMPONENT_ENUM peComponents[] =
	{
		UICOMP_RESTART,
		UICOMP_PAPERDOLL,
		UICOMP_SKILLMAP,
		UICOMP_PLAYERPACK,
		UICOMP_RTS_MINIONS,
		UICOMP_GAMEMENU,
		UICOMP_KIOSK,
		UICOMP_QUEST_PANEL,
	};

	APP_STATE eState = AppGetState();
	if (eState != APP_STATE_IN_GAME)
	{
		return FALSE;
	}

	for (int i=0; i< arrsize(peComponents); i++)
	{
		if (UIComponentGetVisibleByEnum(peComponents[i]))
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------

static BOOL sIncomingMessage( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );
	ASSERT_RETFALSE( IS_CLIENT( game ) );
	ASSERT_RETFALSE( !g_UI.m_bIncoming );

	if (!sIncomingMessageCanShow(game))
	{
		// check again in a bit
		UNIT * player = GameGetControlUnit( game );
		ASSERT_RETFALSE( player );
		UnitRegisterEventTimed( player, sIncomingMessage, &EVENT_DATA(), VIDEO_DELAY_MIN * GAME_TICKS_PER_SECOND );
		return TRUE;
	}

	const WCHAR * msg = c_DialogTranslate( DIALOG_INCOMING_VIDEO );

	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_INCOMING_MESSAGE_PANEL);
	if (pComponent)
	{
		UIComponentActivate(pComponent, TRUE);
		UI_COMPONENT *pLabel = UIComponentFindChildByName(pComponent, "message label");
		if (pLabel)
		{
			UILabelSetText(pLabel, msg);
		}
		g_UI.m_bIncoming = TRUE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------

BOOL IsIncomingMessage(int)
{
	return g_UI.m_bIncoming;
}

//----------------------------------------------------------------------------

static void sIncomingVideo( GAME * pGame, NPC_DIALOG_SPEC * pSpec )
{
	*g_UI.m_pIncomingStore = *pSpec;
	g_UI.m_pIncomingStore->eType = NPC_VIDEO_OK;
	UNIT * player = GameGetControlUnit( pGame );
	ASSERT_RETURN( player );
	int random_delay = RandGetNum( pGame, ( VIDEO_DELAY_MAX - VIDEO_DELAY_MIN ) ) + VIDEO_DELAY_MIN;
	UnitRegisterEventTimed( player, sIncomingMessage, &EVENT_DATA(), random_delay * GAME_TICKS_PER_SECOND );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDisplayNPCVideoAndText(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec)
{
	if ( pSpec->eType == NPC_VIDEO_INCOMING )
	{
		if ( g_UI.m_bIncoming )
			return;
		sIncomingVideo( pGame, pSpec );
		return;
	}

	if ( pSpec->eType == NPC_VIDEO_INSTANT_INCOMING )
	{
		if ( g_UI.m_bIncoming )
			return;
		*g_UI.m_pIncomingStore = *pSpec;
		g_UI.m_pIncomingStore->eType = NPC_VIDEO_OK;
		sIncomingMessage( pGame, GameGetControlUnit(pGame), NULL );
		return;
	}

	ASSERT( pSpec->eType == NPC_VIDEO_OK );

	UIHideNPCDialog(CCT_OK);

	UI_COMPONENT *pDialog =  UIComponentGetByEnum( UICOMP_VIDEO_DIALOG );
	if (!pDialog)
	{
		return;
	}
	if ( UIComponentGetVisible( pDialog ) )
	{
		return;
	}

	// display the conversation UI
	UI_CONVERSATION_DLG *pConvDlg = UICastToConversationDlg(pDialog);

	// create the focus unit of dialog and set it
	sConvDlgFreeClientOnlyUnits(pConvDlg);			// free any previous unit
	pConvDlg->m_idTalkingTo[0] = UICreateClientOnlyUnit( pGame, GlobalIndexGet(pSpec->eGITalkingTo ));
	if ( pConvDlg->m_idTalkingTo[0] == INVALID_ID )
	{
		return;
	}
	pConvDlg->m_idTalkingTo[1] = INVALID_ID;
	if (pSpec->eGITalkingTo2 != GI_INVALID)
	{
		pConvDlg->m_idTalkingTo[1] = UICreateClientOnlyUnit(pGame, GlobalIndexGet(pSpec->eGITalkingTo2));
	}


	UIComponentSetFocusUnit(pDialog, pConvDlg->m_idTalkingTo[0] );

	// get name of who were talking to
	WCHAR uszSpeaker[ MAX_NAME_LEN ];
	UnitGetName( UnitGetById(AppGetCltGame(), pConvDlg->m_idTalkingTo[0]), uszSpeaker, arrsize(uszSpeaker), 0 );
	UILabelSetTextByEnum( UICOMP_LABEL_VIDEO_DIALOG_SPEAKER, uszSpeaker );

	// get player name
	WCHAR uszPlayerName[ MAX_CHARACTER_NAME ];
	PStrCopy( uszPlayerName, L"", arrsize(uszPlayerName) );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer)
	{
		UnitGetName( pPlayer, uszPlayerName, arrsize(uszPlayerName), 0 );
	}

	// what is being said
	const WCHAR * puszText = c_DialogTranslate( pSpec->nDialog );
	ASSERT(puszText);
	WCHAR uszDialog[ MAX_STRING_ENTRY_LENGTH ];
	// Add the video tag, so the label knows to be in video mode
	PStrPrintf(
		uszDialog,
		arrsize(uszDialog),
		L"[%s]",
		UIGetTextTag(TEXT_TAG_VIDEO));

	if(puszText) PStrCat( uszDialog, puszText, arrsize(uszDialog) );

	// replace character name
	PStrReplaceToken( uszDialog, arrsize(uszDialog), StringReplacementTokensGet( SR_PLAYER_NAME ), uszPlayerName );

	// set text
	UI_LABELEX *pLabel = UICastToLabel(UIComponentFindChildByName(pConvDlg, "label dialog text"));
	if (pLabel)
		UILabelSetText( pLabel, uszDialog );

	// autosize the dialog interface
	//UI_TOOLTIP *pTooltip = UICastToTooltip( pDialog );
	//UITooltipDetermineContents( pDialog );
	UI_COMPONENT *pOkButton = UIComponentFindChildByName(pDialog, "button video dialog ok");
	if (pOkButton)
	{
		UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pOkButton);
		float x = ( pos.m_fX + (pOkButton->m_fWidth / 2.0f) )  * UIGetLogToScreenRatioX();
		float y = ( pos.m_fY + (pOkButton->m_fHeight / 2.0f) ) * UIGetLogToScreenRatioY();
		InputSetMousePosition( x, y );
	}

	UIRepaint();		// repaint all - bsp - not positive why

	// save callbacks
	pDialog->m_tCallbackOK = pSpec->tCallbackOK;
	pDialog->m_tCallbackCancel = pSpec->tCallbackCancel;

	UIStopSkills();
	UIComponentActivate(UICOMP_RADIAL_MENU, FALSE);
	c_PlayerClearAllMovement(AppGetCltGame());

	UIComponentActivate(pDialog, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UIRetrieveNPCVideo( GAME * game )
{
	ASSERT_RETURN( g_UI.m_bIncoming );

	UNIT * unit = GameGetControlUnit( game );
	ASSERT_RETURN( unit );
	c_TutorialInputOk( unit, TUTORIAL_INPUT_VIDEO_ENTER );

	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_INCOMING_MESSAGE_PANEL);
	if (pComponent)
	{
		UIComponentActivate(pComponent, FALSE);
	}
	UIDisplayNPCVideoAndText( game, g_UI.m_pIncomingStore );
	g_UI.m_bIncoming = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIIncomingMessageOnAccept(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (g_UI.m_bIncoming)
	{
		UIRetrieveNPCVideo(AppGetCltGame());
	}
	else
	{
		UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_INCOMING_MESSAGE_PANEL);
		if (pComponent)
		{
			UIComponentActivate(pComponent, FALSE);
		}
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISelectDialogOption(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	ASSERT(component);

	// test cursor position
	float flX;
	float flY;
	UIGetCursorPosition( &flX, &flY );
	if (UIComponentGetActive( component ) == FALSE ||
		UIComponentCheckBounds( component, flX, flY ) == FALSE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UI_LABELEX *pLabel = UICastToLabel( component );

	UNIT* pTargetUnit = UIComponentGetFocusUnit(component);

	MSG_CCMD_QUEST_INTERACT_DIALOG msginteract;
	msginteract.idTarget = UnitGetId( pTargetUnit );
	msginteract.nQuestID = (int)pLabel->m_dwParam;

	// send to server
	c_SendMessage( CCMD_QUEST_INTERACT_DIALOG, &msginteract );

	return UIMSG_RET_HANDLED_END_PROCESS;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISelectDialogLocationOption(
								   UI_COMPONENT* component,
								   int msg,
								   DWORD wParam,
								   DWORD lParam)
{
	ASSERT(component);

	// test cursor position
	float flX;
	float flY;
	UIGetCursorPosition( &flX, &flY );
	if (UIComponentGetActive( component ) == FALSE ||
		UIComponentCheckBounds( component, flX, flY ) == FALSE)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UI_LABELEX *pLabel = UICastToLabel( component );

	if( pLabel->m_dwParam != AREA_INVALID )
	{
		MSG_CCMD_REQUEST_AREA_WARP message;
		message.nLevelArea = (int)pLabel->m_dwParam;
		message.nLevelDepth = 0;
		message.bSkipRoad = TRUE;
		message.guidPartyMember = INVALID_GUID;
		SETBIT( message.dwWarpFlags, WF_ARRIVE_AT_TRANSPORTER_BIT );
		c_SendMessage( CCMD_REQUEST_AREA_WARP, &message );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UIHideNPCVideo(
	void )
{
	UI_COMPONENT *pDialog =  UIComponentGetByEnum( UICOMP_VIDEO_DIALOG );
	if ( !pDialog )
	{
		return;
	}

	UI_CONVERSATION_DLG *pConvDlg = UICastToConversationDlg(pDialog);
	sConvDlgFreeClientOnlyUnits(pConvDlg);

	if ( !UIComponentGetActive( pDialog ) )
	{
		return;
	}

	// hide the conversation UI
	UIComponentActivate(pConvDlg, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetQuestText(
	UI_COMPONENT *pComponent,
	const WCHAR *szQuestText,
	BOOL bLastStep)
{
	// get ui component
	UI_COMPONENT *pComp2 = UIComponentFindChildByName(pComponent, "quest description");
	if (pComp2)
	{
		UI_LABELEX *pLabel = UICastToLabel(pComp2);

		// kind of re-purposing the selected and disabled colors here
		pLabel->m_dwColor = (bLastStep ? pLabel->m_dwSelectedColor : pLabel->m_dwDisabledColor);
		// set the text
		UILabelSetText( pLabel, szQuestText );

		// the label should've been auto-sized at this point.  Set the parent panel to fit around it
		if (pLabel->m_pParent)
		{
			float fSibWidth = 0.0f;
			if (pLabel->m_pPrevSibling)		// the header label
			{
				fSibWidth = pLabel->m_pPrevSibling->m_fWidth + pLabel->m_pPrevSibling->m_Position.m_fX;
				pLabel->m_Position.m_fY = pLabel->m_pPrevSibling->m_fHeight + 1.0f;
			}

			pLabel->m_pParent->m_fHeight = pLabel->m_fHeight + pLabel->m_Position.m_fY;
			pLabel->m_pParent->m_fWidth = MAX(fSibWidth, pLabel->m_fWidth + pLabel->m_Position.m_fX);

			//ok, now this is getting voncoluted
			if (pLabel->m_pParent->m_pNextSibling)
			{
				pLabel->m_pParent->m_fHeight += pLabel->m_pParent->m_pNextSibling->m_fToolTipYOffset;
			}
		}
	}

	UIComponentSetActive(pComponent, TRUE);
}

void UIUpdateQuestLog(
	QUEST_LOG_UI_STATE eState,
	int nChangedQuest /*= INVALID_ID*/)
{
	// do nothing if we havne't received all the setup from the server yet
	if (c_QuestIsAllInitialQuestStateReceived() == FALSE)
	{
		return;
	}

	UIUpdateQuestPanel();

	UI_COMPONENT *pQuestLog = UIComponentGetByEnum( UICOMP_QUEST_TRACKING );
	if ( !pQuestLog )
	{
		return;
	}

	// go through logs
	GAME *pGame = AppGetCltGame();
	UNIT * pPlayer = GameGetControlUnit( pGame );

	WCHAR szLogBuffer[ MAX_QUESTLOG_STRLEN ];
	WCHAR szTemp[ MAX_QUESTLOG_STRLEN ];
	UI_COMPONENT *pHolderPanel = UIComponentFindChildByName(pQuestLog, "quest panels holder");
	if (!pHolderPanel)
	{
		return;
	}

	UI_COMPONENT *pChangedPanel = NULL;
	UI_COMPONENT *pPanel = UIComponentIterateChildren(pHolderPanel, NULL, UITYPE_PANEL, FALSE);
	int nQuest = 0;
	while (pPanel && nQuest < NUM_QUESTS)
	{
		// int log buffer
		szLogBuffer[ 0 ] = 0;

		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
		int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
		BOOL bTracking = UnitGetStat(pPlayer, STATS_QUEST_PLAYER_TRACKING, nQuest, nDifficulty) != 0;
		// do nothing if not visible in log
		if (pQuest && pPanel && c_QuestIsVisibleInLog( pQuest ) &&
			(bTracking || nQuest == nChangedQuest))
		{
			BOOL bLastStep = FALSE;

			// go through each of the states
			for (int i = 0; i < pQuest->nNumStates; ++i)
			{
				QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );

				// we display only active or completed entries
				QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
				if (eStateValue == QUEST_STATE_ACTIVE)
				{
					bLastStep = (i == pQuest->nNumStates-1);
					//int nColor;
					WCHAR szQuestStateLogBuffer[ MAX_QUESTLOG_STRLEN ];
					const WCHAR *puszLogText = c_QuestStateGetLogText( pPlayer, pQuest, pState, NULL /*&nColor*/ );
					// skip states that don't have any log strings
					if (puszLogText == NULL)
					{
						continue;
					}

					c_QuestReplaceStringTokens(
						pPlayer,
						pQuest,
						puszLogText,
						szQuestStateLogBuffer,
						MAX_QUESTLOG_STRLEN);

					// set the text
					szTemp[0] = L'\0';
					//UIColorCatString(szTemp, MAX_QUESTLOG_STRLEN, (FONTCOLOR)nColor, szQuestStateLogBuffer);
					PStrCopy(szTemp, szQuestStateLogBuffer, arrsize(szTemp));

					int nQuestState = pState->nQuestState;
					const QUEST_STATE_DEFINITION *pStateDef = QuestStateGetDefinition( nQuestState );
					if (pQuest->nQuestStateActiveLog == nQuestState)
					{
						if ( eStateValue == QUEST_STATE_ACTIVE )
						{
							ASSERT_RETURN( pStateDef->nDialogFullDesc != INVALID_LINK );
							const WCHAR *puszFullLog = c_DialogTranslate( pStateDef->nDialogFullDesc );
							if ( puszFullLog )
							{
								c_QuestReplaceStringTokens(
									pPlayer,
									pQuest,
									puszFullLog,
									szQuestStateLogBuffer,
									MAX_QUESTLOG_STRLEN);

								UIAddChatLine( CHAT_TYPE_QUEST, ChatGetTypeColor(CHAT_TYPE_QUEST), szQuestStateLogBuffer );
							}
							pQuest->nQuestStateActiveLog = INVALID_ID;
						}
					}

					if (bTracking)
					{
						PStrCat( szLogBuffer, szTemp, MAX_QUESTLOG_STRLEN );
					}

					if ( ( nQuest == nChangedQuest ) && !UnitIsInTutorial( pPlayer ) && !pStateDef->bNoQuickMessageOnUpdate )
					{
						UIShowQuickMessage( szTemp, LEVELCHANGE_FADEOUTMULT_DEFAULT );
					}

					// Only do the first active one for now
					break;
				}
			}

			// set quest name
			UI_COMPONENT *pHeader = UIComponentFindChildByName(pPanel, "quest label");
			if (pHeader)
			{
				if (szLogBuffer[0])		// if we have something to show
				{
					UI_LABELEX *pLabel = UICastToLabel(pHeader);
					// kind of re-purposing the selected and disabled colors here
					pLabel->m_dwColor = (bLastStep ? pLabel->m_dwSelectedColor : pLabel->m_dwDisabledColor);
					// set the text
					UILabelSetText( pLabel, StringTableGetStringByIndex( pQuest->nStringKeyName ) );
				}
				else
				{
					UILabelSetText( pHeader, L"" );
				}
			}

			// set step descriptions
			sSetQuestText(pPanel, &szLogBuffer[0], bLastStep);

			pPanel = UIComponentIterateChildren(pHolderPanel, pPanel, UITYPE_PANEL, FALSE);
		}

		nQuest++;
	}

	// Set the rest of the panels invisible
	while (pPanel)
	{
		UIComponentSetVisible(pPanel, FALSE);
		pPanel = UIComponentIterateChildren(pHolderPanel, pPanel, UITYPE_PANEL, FALSE);
	}

	// now we need to reposition all the individual panels as the heights have changed
	pPanel = UIComponentIterateChildren(pHolderPanel, NULL, UITYPE_PANEL, FALSE);
	float fYPos = 0.0f;
	float fChangedPanelLowestPoint = 0.0f;
	while (pPanel)
	{
		fYPos += pPanel->m_fToolTipYOffset;
		pPanel->m_Position.m_fY = fYPos + 10.0f;
		float fLowestPoint = 0.0f;
		UIComponentGetLowestPoint(pPanel, fLowestPoint, 0.0f, 0, 10, FALSE);
		if (pPanel == pChangedPanel)
		{
			fChangedPanelLowestPoint = fLowestPoint;
		}
		fYPos += fLowestPoint;
		pPanel = UIComponentIterateChildren(pHolderPanel, pPanel, UITYPE_PANEL, FALSE);
	}

	UIComponentSetVisible(pHolderPanel, TRUE);	// set this visible so the scrollbar will update right
	UIQuestsPanelResizeScrollbar(pHolderPanel, 0, 0, 0);

	// Make sure if there's a quest that was updated that its panel is visible in the scrollable area
	if (pChangedPanel)
	{
		fYPos = pChangedPanel->m_Position.m_fY - pHolderPanel->m_ScrollPos.m_fY;
		if (fYPos < 0.0f)	// if it's above the top
		{
			UIScrollBarSetValue(pHolderPanel->m_pParent, "quest log scrollbar", pHolderPanel->m_ScrollPos.m_fY + fYPos);		//scroll up to it
//				pHolderPanel->m_ScrollPos.m_fY += fYPos;	//scroll up to it
		}
		else
		{
			if (fYPos + fChangedPanelLowestPoint > pHolderPanel->m_fHeight)	// if it's off the bottom
			{
				UIScrollBarSetValue(pHolderPanel->m_pParent, "quest log scrollbar", pHolderPanel->m_ScrollPos.m_fY - ((fYPos + fChangedPanelLowestPoint) - pHolderPanel->m_fHeight));		//scroll down to it
//				pHolderPanel->m_ScrollPos.m_fY -= (fYPos + fChangedPanelLowestPoint) - pHolderPanel->m_fHeight;	//scroll down to it
			}
		}
	}

	// repaint children
	UI_COMPONENT *pChild = NULL;
	while ((pChild = UIComponentIterateChildren(pQuestLog, pChild, -1, NULL)) != NULL)
	{
		UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestLogOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!GameGetControlUnit(pGame))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestLogOnPostVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!GameGetControlUnit(pGame))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestCancelOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// TODO: do the quest cancel here

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIWantsUnitStatChange(UNITID idUnit)
{
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowBreathMeter()
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_BREATH_METER);
	if (!component)
	{
		return;
	}

	UIComponentSetActive( component, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIHideBreathMeter()
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_BREATH_METER);
	if (!component)
	{
		return;
	}
	UIComponentSetVisible( component, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUITraceIDs(const char *szName, UI_COMPONENT *pComponent)
{
	if (!pComponent)
		return;

	if (!szName || !szName[0] || PStrStr(pComponent->m_szName, szName) != NULL)
	{
		trace("%s - %d\n", pComponent->m_szName, pComponent->m_idComponent);
	}

	sUITraceIDs(szName, pComponent->m_pFirstChild);

	sUITraceIDs(szName, pComponent->m_pNextSibling);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITraceIDs(const char *szName)
{
	sUITraceIDs(szName, g_UI.m_Components);
	sUITraceIDs(szName, g_UI.m_Cursor);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsLoaded()
{
	return g_UI.m_Components != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIOnUnitFree(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( UnitTestFlag( pUnit, UNITFLAG_JUSTFREED ) == TRUE, "Unit has not be marked as just freed" );

	UISendMessage(WM_FREEUNIT, (WPARAM)UnitGetId(pUnit), 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowAltMenu(
	BOOL bShow)
{
	if (!UIComponentGetVisibleByEnum(UICOMP_INCOMING_MESSAGE_PANEL))
	{
		if (!bShow)
		{
			UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_ALT_MENU), UIMSG_MOUSELEAVE, 0, 0, TRUE);
		}
		UIComponentActivate(UICOMP_ALT_MENU, bShow);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckAltMenuKey(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT *pAltMenu = UIComponentGetByEnum(UICOMP_ALT_MENU);
	if (pAltMenu)
	{
		if (!InputIsCommandKeyDown(CMD_SHOW_ITEMS))
		{
			UIComponentHandleUIMessage(pAltMenu, UIMSG_MOUSELEAVE, 0, 0, TRUE);
			UIComponentActivate(pAltMenu, FALSE);
		}

		//Schedule another
		CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC, sCheckAltMenuKey, CEVENT_DATA());
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAltMenuOnPostVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentHandleUIMessage(UIComponentGetByEnum(UICOMP_CONTEXT_HELP), UIMSG_PAINT, 0, 0);
	UI_COMPONENT *pChild = UIComponentFindChildByName(component, "skills btn");
	if (pChild)
	{
		if (g_UI.m_bVisitSkillPageReminder)
		{
			UIComponentThrobStart(pChild, 0xffffffff, 1000);
		}
		else
		{
			UIComponentThrobEnd(pChild, 0, 0, 0);
		}
	}

	UI_COMPONENT *pBuddyButton = UIComponentFindChildByName(component, "buddy list btn");
	if (pBuddyButton)
	{
		UIComponentSetActive(pBuddyButton, AppGetType() != APP_TYPE_SINGLE_PLAYER);
		UIComponentSetVisible(pBuddyButton, TRUE);
	}

	UI_COMPONENT *pGuildButton = UIComponentFindChildByName(component, "guild panel btn");
	if (pGuildButton)
	{
		UIComponentSetActive(pGuildButton, c_PlayerGetGuildChannelId() != INVALID_CHANNELID);
		UIComponentSetVisible(pGuildButton, TRUE);
	}

	UI_COMPONENT *pPartyButton = UIComponentFindChildByName(component, "partylist panel btn");
	if (pPartyButton)
	{
		UIComponentSetActive(pPartyButton, AppGetType() != APP_TYPE_SINGLE_PLAYER);
		UIComponentSetVisible(pPartyButton, TRUE);
	}

	CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC, sCheckAltMenuKey, CEVENT_DATA());
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIContextHelpLabelOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	component = UIComponentGetByEnum(UICOMP_CONTEXT_HELP);
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_LABELEX *pLabel = UICastToLabel(component);

	UIComponentThrobEnd(component, 0, 0, 0);

	BOOL bAltVisible = (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_ALT_MENU)));
	UI_COMPONENT *pXPBar = UIComponentGetByEnum(UICOMP_EXPERIENCEBAR);
	if (pXPBar && UIComponentGetActive(pXPBar))
	{
		UI_COMPONENT *pChild = NULL;
		while ((pChild = UIComponentIterateChildren(pXPBar, pChild, UITYPE_STATDISPL, FALSE)) != NULL)
		{
			UIComponentSetVisible(pChild, bAltVisible);
		}
	}
	else
	{
		pXPBar = UIComponentGetByEnum(UICOMP_RANKBAR);
		if (pXPBar && UIComponentGetActive(pXPBar))
		{
			UI_COMPONENT *pChild = NULL;
			while ((pChild = UIComponentIterateChildren(pXPBar, pChild, UITYPE_STATDISPL, FALSE)) != NULL)
			{
				UIComponentSetVisible(pChild, bAltVisible);
			}
		}
	}

	if (!UIGetControlUnit() || IsUnitDeadOrDying(UIGetControlUnit()))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}

	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_INVENTORY)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_SKILLMAP)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_ALT_MENU)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_INCOMING_MESSAGE_PANEL)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_VIDEO_DIALOG)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_QUEST_PANEL)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetActive(UIComponentGetByEnum(UICOMP_RTS_ACTIVEBAR)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}
	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_ACHIEVEMENT_COMPLETE_NOTIFY)))
	{
		UILabelSetText(pLabel, L"");
		return UIMSG_RET_HANDLED;
	}

	if (c_CameraGetViewType() == VIEW_VANITY)
	{
		UILabelSetTextByGlobalString(pLabel, GS_UI_VANITY_CLOSE );
		return UIMSG_RET_HANDLED;
	}

	if (UIComponentGetVisible(UIComponentGetByEnum(UICOMP_WORLD_MAP)))
	{
		UILabelSetTextByGlobalString(pLabel, GS_UI_WORLDMAP_CLOSE );
		return UIMSG_RET_HANDLED;
	}

	if (g_UI.m_bVisitSkillPageReminder)
	{
		UILabelSetTextByGlobalString(pLabel, GS_UI_CHECK_SKILL_PAGE );
		UIComponentThrobStart(component, 0xffffffff, 1000 );
		return UIMSG_RET_HANDLED;
	}

	UILabelSetTextByGlobalString(pLabel, GS_UI_ALT_MENU_HELP );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradeOnAccept(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		c_TradeToggleAccept();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradeMoneyOnChar(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ))
	{
		WCHAR ucCharacter = (WCHAR)wParam;
		if (UIEditCtrlOnKeyChar( pComponent, nMessage, wParam, lParam ) != UIMSG_RET_NOT_HANDLED)
		{
			c_TradeMoneyOnChar( pComponent, ucCharacter );
			return UIMSG_RET_HANDLED_END_PROCESS;  // input used
		}
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradeOnMoneyIncrease(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		UIButtonOnButtonDown( pComponent, nMessage, wParam, lParam );
		c_TradeMoneyOnIncrease( pComponent );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradeOnMoneyDecrease(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		UIButtonOnButtonDown( pComponent, nMessage, wParam, lParam );
		c_TradeMoneyOnDecrease( pComponent );
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRewardOnTakeAll(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		c_RewardOnTakeAll();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRewardsCancel(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		c_RewardClose();
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIQueueItemWindowsRepaint(
	void)
{
	g_UI.m_bNeedItemWindowRepaint = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIAppCursorAlwaysOn(
	void)
{
	if (AppIsHellgate())
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DialogCallbackInit(
	DIALOG_CALLBACK &tDialogCallback)
{
	tDialogCallback.pfnCallback = NULL;
	tDialogCallback.pCallbackData = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if (ISVERSION(DEVELOPMENT))
static WPARAM wLastKey = 0;

BOOL UIDebugEditKeyDown(
	WPARAM wKey)
{
	ASSERT_RETFALSE(GameGetDebugFlag(AppGetCltGame(), DEBUGFLAG_UI_EDIT_MODE));

	static DWORD dwFirstPress = 0;
	static float fIncrement = 1.0f;

	if (wLastKey == wKey)
	{
		DWORD dwNow = AppCommonGetRealTime();
		if ( dwNow - dwFirstPress > 4000)
		{
			fIncrement = 10.0f;
		}
		else if ( dwNow - dwFirstPress > 2000)
		{
			fIncrement = 5.0f;
		}
	}
	else
	{
		dwFirstPress = AppCommonGetRealTime();
		fIncrement = 1.0f;
	}
	wLastKey = wKey;

	if (g_UI.m_idDebugEditComponent != INVALID_ID)
	{
		UI_COMPONENT *pComponent = UIComponentGetById(g_UI.m_idDebugEditComponent);
		ASSERT_RETFALSE(pComponent);
		BOOL bRepaint = FALSE;

		if (GetKeyState(VK_SHIFT) & 0x8000)
		{
			switch (wKey)
			{
			case VK_UP:			pComponent->m_fHeight -= fIncrement;	bRepaint = TRUE; break;
			case VK_DOWN:		pComponent->m_fHeight += fIncrement;	bRepaint = TRUE; break;
			case VK_LEFT:		pComponent->m_fWidth  -= fIncrement;	bRepaint = TRUE; break;
			case VK_RIGHT:		pComponent->m_fWidth  += fIncrement;	bRepaint = TRUE; break;
			case 'X':			UIComponentActivate(pComponent, !UIComponentGetActive(pComponent));		bRepaint = TRUE; break;
			default:
				return FALSE;
			}
		}
		else
		{
			switch (wKey)
			{
			case VK_UP:			pComponent->m_Position.m_fY -= fIncrement;	break;
			case VK_DOWN:		pComponent->m_Position.m_fY += fIncrement;	break;
			case VK_LEFT:		pComponent->m_Position.m_fX -= fIncrement;	break;
			case VK_RIGHT:		pComponent->m_Position.m_fX += fIncrement;	break;
			case VK_PRIOR:		if (pComponent->m_pParent)		g_UI.m_idDebugEditComponent = pComponent->m_pParent->m_idComponent;			break;
			case VK_NEXT:		if (pComponent->m_pFirstChild)	g_UI.m_idDebugEditComponent = pComponent->m_pFirstChild->m_idComponent;		break;
			case VK_DELETE:		if (pComponent->m_pPrevSibling) g_UI.m_idDebugEditComponent = pComponent->m_pPrevSibling->m_idComponent;	break;
			case VK_END:		if (pComponent->m_pNextSibling) g_UI.m_idDebugEditComponent = pComponent->m_pNextSibling->m_idComponent;	break;
			//case VK_ESCAPE:		GameSetDebugFlag(AppGetCltGame(), DEBUGFLAG_UI_EDIT_MODE, FALSE);	break;
			default:
				return FALSE;
			}
		}

		if (bRepaint)
		{
			UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
		}
		else
		{
			UISetNeedToRender(pComponent);
		}

		return TRUE;
	}

	return FALSE;
}

BOOL UIDebugEditKeyUp(
	WPARAM wKey)
{
	switch (wKey)
	{
	case VK_UP:
	case VK_DOWN:
	case VK_LEFT:
	case VK_RIGHT:
	case VK_PRIOR:
	case VK_NEXT:
	case VK_DELETE:
	case VK_END:
		wLastKey = 0;
		return TRUE;
	}

	return FALSE;
}

#endif //(ISVERSION(DEVELOPMENT))

// Tugboat-specific
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGetSelectedUnitPosition(
	UNIT* unit,
	VECTOR* pVector)
{
	ASSERT_RETURN(unit);
	*pVector = UnitGetAimPoint( unit );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void TransformUnitSpaceToScreenSpace(
	UNIT* unit,
	int* x,
	int* y)
{
	ASSERT_RETURN(unit);
	VECTOR vLoc;
	UIGetSelectedUnitPosition(unit, (VECTOR*)&vLoc);
	TransformWorldSpaceToScreenSpace(  &vLoc, x, y );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void TransformWorldSpaceToScreenSpace(
	VECTOR* vLoc,
	int* x,
	int* y)
{


	// transform unit position to screen space
	MATRIX mOut;
	MATRIX mWorld;
	MATRIX mView;
	MATRIX mProj;
	e_InitMatrices( & mWorld, & mView, & mProj, NULL, NULL, NULL, NULL, TRUE );
	float Width = UIDefaultWidth();
	float Height = UIDefaultHeight();

	int nScreenWidth, nScreenHeight;
	e_GetWindowSize( nScreenWidth, nScreenHeight );
	float fCoverageWidth = e_GetUICoverageWidth();
	BOUNDING_BOX tViewport;
	tViewport.vMin = VECTOR(0, 0, 0.0f );
	tViewport.vMax = VECTOR( ( (float)nScreenWidth - fCoverageWidth ), (float)nScreenHeight, 1.0f );

	V( e_GetWorldViewProjMatrix( &mOut, TRUE ) );
	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mOut, TRUE, &tViewport );
	Width /= ( (float)nScreenWidth / ( (float)nScreenWidth - fCoverageWidth ) );

	/*
	if( e_GetUILeftCovered() )
	{
		int nScreenWidth, nScreenHeight;
		e_GetWindowSize( nScreenWidth, nScreenHeight );

		BOUNDING_BOX tViewport;
		tViewport.vMin = VECTOR(0, 0, 0.0f );
		tViewport.vMax = VECTOR( (float)nScreenWidth / 2, (float)nScreenHeight, 1.0f );

		V( e_GetWorldViewProjMatrix( &mOut, TRUE ) );
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mOut, TRUE, &tViewport );
		Width /= 2;
	}
	else if( e_GetUITopLeftCovered() && e_GetUIRightCovered() )
	{
		int nScreenWidth, nScreenHeight;
		e_GetWindowSize( nScreenWidth, nScreenHeight );

		BOUNDING_BOX tViewport;
		tViewport.vMin = VECTOR(0, 0, 0.0f );
		tViewport.vMax = VECTOR( (float)nScreenWidth / 2, (float)nScreenHeight * .44f, 1.0f );

		V( e_GetWorldViewProjMatrix( &mOut, TRUE ) );
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mOut, TRUE, &tViewport );
		Height *= .44f;
		Width /= 2;
	}
	else if( e_GetUIRightCovered() )
	{
		int nScreenWidth, nScreenHeight;
		e_GetWindowSize( nScreenWidth, nScreenHeight );

		BOUNDING_BOX tViewport;
		tViewport.vMin = VECTOR(0, 0, 0.0f );
		tViewport.vMax = VECTOR( (float)nScreenWidth / 2, (float)nScreenHeight, 1.0f );

		V( e_GetWorldViewProjMatrix( &mOut, TRUE ) );
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mOut, TRUE, &tViewport );
		Width /= 2;
	}*/

	MatrixMultiply ( &mOut, &mWorld, & mView );
	MatrixMultiply ( &mOut, &mOut, & mProj );





	VECTOR4 vNewLoc;
	MatrixMultiply( &vNewLoc, vLoc, &mOut );
	vNewLoc.fX = ((vNewLoc.fX / vNewLoc.fW + 1.0f) / 2.0f) * Width;
	vNewLoc.fY = ((-vNewLoc.fY / vNewLoc.fW + 1.0f) / 2.0f) * Height;
/*	if( e_GetUILeftCovered() )
	{
		vNewLoc.fX += UI_DEFAULT_WIDTH / 2;
	}
	else if( e_GetUITopLeftCovered() && e_GetUIRightCovered() )
	{
		//vNewLoc.fX -= Width / 8;
		vNewLoc.fY += UI_DEFAULT_HEIGHT *.56f;
	}
	else if( e_GetUIRightCovered() )
	{
		//vNewLoc.fX -= UI_DEFAULT_WIDTH / 2;
	}*/
	if( e_GetUICoveredLeft() )
	{
		vNewLoc.fX += UIDefaultWidth() / ( (float)nScreenWidth / fCoverageWidth  );
	}
	*x = (int)(FLOOR(vNewLoc.fX + 0.5f));
	*y = (int)(FLOOR(vNewLoc.fY + 0.5f));
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_TB_ShowRoomEnteredMessage( const WCHAR *questname )
{
	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_ROOM_ENTERED_MESSAGE);
	if (!component)
	{
		return;
	}
	UI_LABELEX *pLabel = UICastToLabel(component);


	WCHAR szTemp[ 256 ];
	PStrPrintf( szTemp, 256, L"%s", questname );
	UILabelSetText( pLabel, szTemp );
	UIAddChatLine( CHAT_TYPE_QUEST, ChatGetTypeColor(CHAT_TYPE_QUEST), szTemp );


	UIComponentActivate(component, TRUE);		//pop it up (m_bFadesIn == FALSE)
	UIComponentActivate(component, FALSE);		//fade it out (m_bFadesOut == TRUE)

	component = UIComponentGetByEnum(UICOMP_LEVELNAME);
	if (!component)
	{
		return;
	}

	UI_COMPONENT *pChild  = component->m_pFirstChild;
	while ( pChild )
	{
		if ( ( pChild->m_eComponentType == UITYPE_LABEL ) && ( PStrCmp( pChild->m_szName, "level name text" ) == 0 ) )
		{
			UILabelSetText( pChild->m_idComponent, szTemp );
		}
		pChild = pChild->m_pNextSibling;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIShowTaskCompleteMessage( const WCHAR *questname )
{

	UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_TASKCOMPLETETEXT);
	if (!component)
	{
		return;
	}
	UI_LABELEX *pLabel = UICastToLabel(component);

	{
		WCHAR szTemp[ 256 ];
		PStrPrintf( szTemp, 256, L"%s", questname );
		UILabelSetText( pLabel, szTemp );
		if( AppIsTugboat() )
		{
			UIAddChatLine( CHAT_TYPE_QUEST, ChatGetTypeColor(CHAT_TYPE_QUEST), szTemp );
		}
	}

	UIComponentActivate(component, TRUE);		//pop it up (m_bFadesIn == FALSE)
	UIComponentActivate(component, FALSE);		//fade it out (m_bFadesOut == TRUE)
	//UIComponentSetActive(component, TRUE);
	//DWORD dwMaxAnimDuration = 0;
	//float duration = ((AppIsHellgate())?1.0f:2.0f);
	//UIComponentFadeOut(UIComponentGetByEnum(UICOMP_TASKCOMPLETETEXT), duration, dwMaxAnimDuration);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentSetVisibleFalse(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UIComponentSetVisible(pComponent, FALSE);
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEnterRTSMode(
	void)
{
	//// Hide the main controls
	UIComponentActivate(UICOMP_RTS_MINIONS, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIExitRTSMode(
	void)
{
	UIComponentActivate(UICOMP_RTS_MINIONS, FALSE);
}

static float fDPSList[2] = {0, 0};
static TIME  tiReceived[2] = {TIME_ZERO, TIME_ZERO};
static const int DPS_MESSAGE_INTERVAL_SECS	= 2;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUIInterpolateDPS(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	float fDPS = 0.0f;

	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_DPS_BAR);
	if (pComp)
	{
		UI_BAR *pBar = UICastToBar(pComp);
		TIME tiElapsed = AppCommonGetCurTime() - tiReceived[1];
		{
			TIME tiBetweenMsg = tiReceived[1] - tiReceived[0];

			float fRatio = tiBetweenMsg ? (float)tiElapsed / (float)tiBetweenMsg : 0.0f;
			if (fRatio > 1.0f)
			{
				UIUpdateDPS(0.0f);
				return;
			}
			fDPS = fDPSList[0] + ((fDPSList[1] - fDPSList[0]) * fRatio);
			if (fDPS < 0.0f)
			{
				fDPS = 0.0f;
			}
		}

		pComp = UIComponentGetByEnum(UICOMP_DPS_LABEL);
		if (pComp)
		{
			if (fDPS > 0.0f)
			{
				WCHAR szDPS[256];
				PStrPrintf(szDPS, arrsize(szDPS), L"DPS:  %0.2f", fDPS);

				UILabelSetText(pComp, szDPS);
				UIComponentSetActive(pComp, TRUE);
			}
			else
			{
				UIComponentSetVisible(pComp, FALSE);
			}
		}

		fDPS *= 100.0f;
		if (fDPS > pBar->m_nMaxValue)
		{
			pBar->m_nMaxValue = (int)fDPS;
		}
		pBar->m_nValue = (int)fDPS;
		UIBarOnPaint(pBar, UIMSG_PAINT, 0, 0);

		if (CSchedulerIsValidTicket(pBar->m_tMainAnimInfo.m_dwAnimTicket))
		{
			CSchedulerCancelEvent(pBar->m_tMainAnimInfo.m_dwAnimTicket);
		}
		if (fDPSList[0] != 0.0f || fDPSList[1] != 0.0f)
		{
			pBar->m_tMainAnimInfo.m_dwAnimTicket =	CSchedulerRegisterEvent(AppCommonGetCurTime() + (GAME_TICKS_PER_SECOND / 4), sUIInterpolateDPS, CEVENT_DATA());
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateDPS(
	float fDPS)
{
	fDPSList[0] = fDPSList[1];
	tiReceived[0] = tiReceived[1];

	fDPSList[1] = fDPS;
	tiReceived[1] = AppCommonGetCurTime();

	sUIInterpolateDPS(AppGetCltGame(), CEVENT_DATA(), 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetMouseOverrideComponent(UI_COMPONENT *pComponent,
	BOOL bAddToTail /*= FALSE*/)
{
	ASSERT_RETURN(pComponent);

	if (g_UI.m_listOverrideMouseMsgComponents.Find(pComponent))
		return;

	if (bAddToTail)
		g_UI.m_listOverrideMouseMsgComponents.PListPushTail(pComponent);
	else
		g_UI.m_listOverrideMouseMsgComponents.PListPushHead(pComponent);

	if (g_UI.m_pCurrentMouseOverComponent)
	{
		UI_COMPONENT *pOverrideMouseMsgComponent = UIGetMouseOverrideComponent();
		if (pOverrideMouseMsgComponent &&
			!(UIComponentIsParentOf(pOverrideMouseMsgComponent, g_UI.m_pCurrentMouseOverComponent) ||
			UIComponentIsParentOf(g_UI.m_pCurrentMouseOverComponent, pOverrideMouseMsgComponent)))
		{
			UIComponentHandleUIMessage(g_UI.m_pCurrentMouseOverComponent, UIMSG_MOUSELEAVE, 0, 0, FALSE);
			g_UI.m_pCurrentMouseOverComponent = NULL;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetKeyboardOverrideComponent(UI_COMPONENT *pComponent,
	BOOL bAddToTail /*= FALSE*/)
{
	ASSERT_RETURN(pComponent);

	if (g_UI.m_listOverrideKeyboardMsgComponents.Find(pComponent))
		return;

	if (bAddToTail)
		g_UI.m_listOverrideKeyboardMsgComponents.PListPushTail(pComponent);
	else
		g_UI.m_listOverrideKeyboardMsgComponents.PListPushHead(pComponent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIGetMouseOverrideComponent()
{
	PList<UI_COMPONENT *>::USER_NODE *pNode = g_UI.m_listOverrideMouseMsgComponents.GetNext(NULL);
	return (pNode ? pNode->Value : NULL);

//	return g_UI.m_pOverrideMouseMsgComponent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_COMPONENT * UIGetKeyboardOverrideComponent()
{
	PList<UI_COMPONENT *>::USER_NODE *pNode = g_UI.m_listOverrideKeyboardMsgComponents.GetNext(NULL);
	return (pNode ? pNode->Value : NULL);

	//return g_UI.m_pOverrideKeyboardMsgComponent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIRemoveMouseOverrideComponent(UI_COMPONENT *pComponent)
{
	g_UI.m_listOverrideMouseMsgComponents.FindAndDelete(pComponent);
	//if (g_UI.m_pOverrideMouseMsgComponent == pComponent)
	//	g_UI.m_pOverrideMouseMsgComponent = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIRemoveKeyboardOverrideComponent(UI_COMPONENT *pComponent)
{
	g_UI.m_listOverrideKeyboardMsgComponents.FindAndDelete(pComponent);
	//if (g_UI.m_pOverrideKeyboardMsgComponent == pComponent)
	//	g_UI.m_pOverrideKeyboardMsgComponent = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoUIComponentArrayAction(
	UI_COMPONENT *pComp,
	UI_ACTION eAction,
	UNITID idParam)
{
	if (pComp)
	{

		switch (eAction)
		{

			case UIA_HIDE:
			{
				UIComponentSetVisible(pComp, FALSE);
				break;
			}

			case UIA_SHOW:
			{
				UIComponentSetVisible(pComp, TRUE);
				break;
			}

			case UIA_ACTIVATE:
			{
				UIComponentSetActive( pComp, TRUE );
				break;
			}

			case UIA_DEACTIVATE:
			{
				UIComponentSetActive( pComp, FALSE );
				break;
			}

			case UIA_SETFOCUSUNIT:
			{
				UIComponentSetFocusUnit( pComp, idParam );
				break;
			}

		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentArrayDoAction(
	const enum UI_COMPONENT_ENUM *pComps,
	UI_ACTION eAction,
	UNITID idParam)
{
	ASSERTX_RETURN( pComps, "Invalid comp array" );
	int nIndex = 0;
	while (pComps[ nIndex ] != UICOMP_INVALID)
	{
		UI_COMPONENT_ENUM eComp = pComps[ nIndex++ ];
		UI_COMPONENT *pComponent = UIComponentGetByEnum( eComp );
		sDoUIComponentArrayAction( pComponent, eAction, idParam );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentArrayDoAction(
	struct UI_COMPONENT **pComps,
	UI_ACTION eAction,
	UNITID idParam)
{
	ASSERTX_RETURN( pComps, "Invalid comp array" );
	int nIndex = 0;
	while (pComps[ nIndex ] != NULL)
	{
		UI_COMPONENT *pComponent = pComps[ nIndex++ ];
		sDoUIComponentArrayAction( pComponent, eAction, idParam );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUpdateProgressBar(
	GAME* pGame,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_BAR *pBar = (UI_BAR *)data.m_Data1;
	ASSERT_RETURN(pBar);

	DWORD dwCurrentTick = GameGetTick(pGame);

	pBar->m_nValue = (int)(dwCurrentTick - data.m_Data2);
	UIComponentHandleUIMessage(pBar, UIMSG_PAINT, 0, 0);

	if (pBar->m_nValue <= pBar->m_nMaxValue)
	{
		pBar->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC / 100, sUpdateProgressBar, data);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIUseProgressBarOnSetControlState(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	GAME *pGame = AppGetCltGame();
	DWORD dwCurrentTick = GameGetTick(pGame);
	if (nMessage == UIMSG_SETCONTROLSTATE)
	{
		ASSERT_RETVAL(lParam, UIMSG_RET_NOT_HANDLED);
		UI_STATE_MSG_DATA *pData = (UI_STATE_MSG_DATA *)CAST_TO_VOIDPTR(lParam);

		if (pData->m_nState == STATE_OPERATE_OBJECT_DELAY)
		{
			UI_COMPONENT *pChild = UIComponentIterateChildren(pComponent, NULL, UITYPE_BAR, FALSE);
			if (pChild)
			{
				if (pComponent->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
				{
					CSchedulerCancelEvent(pComponent->m_tMainAnimInfo.m_dwAnimTicket);
				}

				UI_BAR *pBar = UICastToBar(pChild);
				if (pData->m_bClearing)
				{
					if (pComponent->m_dwData > dwCurrentTick)	// if we're interrupted
					{
						pBar->m_dwColor = GFXCOLOR_RED;
						if (pBar->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
						{
							CSchedulerCancelEvent(pBar->m_tMainAnimInfo.m_dwAnimTicket);
						}
						UIComponentRemoveAllElements(pBar);		//force repaint
						UIComponentHandleUIMessage(pBar, UIMSG_PAINT, 0, 0);
						pComponent->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC, UIDelayedImmediateActivate, CEVENT_DATA((DWORD_PTR)pComponent, (DWORD_PTR)FALSE));
						pComponent->m_bUserActive = FALSE;
					}
					else
					{
						UIComponentActivate(pComponent, FALSE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
						pComponent->m_bUserActive = FALSE;
					}
				}
				else
				{
					UNIT *pPlayer = GameGetControlUnit(pGame);
					if (pPlayer)
					{
						pBar->m_dwColor = GFXCOLOR_WHITE;
						pBar->m_nMaxValue = UnitGetStat(pPlayer, STATS_QUEST_OPERATE_DELAY_IN_TICKS);
						pBar->m_nValue = 0;

						pComponent->m_dwData = dwCurrentTick + pBar->m_nMaxValue;

						UIComponentHandleUIMessage(pBar, UIMSG_PAINT, 0, 0);
						if (pBar->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
						{
							CSchedulerCancelEvent(pBar->m_tMainAnimInfo.m_dwAnimTicket);
						}
						pBar->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC / 100, sUpdateProgressBar, CEVENT_DATA((DWORD_PTR)pBar, dwCurrentTick, dwCurrentTick + pBar->m_nMaxValue));
					}

					pComponent->m_Position = pComponent->m_ActivePos;
					UIComponentActivate(pComponent, TRUE, 0, NULL, FALSE, FALSE, TRUE, TRUE);
					pComponent->m_bUserActive = TRUE;
				}
			}

			return UIMSG_RET_HANDLED;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIVisibleOnPlayerFocusOnly(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UNIT *pUnit = UIComponentGetFocusUnit(pComponent);
	if (!pUnit || !UnitIsPlayer(pUnit))
	{
		UIComponentSetVisible(pComponent, FALSE);
	}
	else
	{
		UIComponentSetActive(pComponent, TRUE);
	}

	return UIMSG_RET_NOT_HANDLED;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRatingLogoEvent(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT *pComponent = (UI_COMPONENT *)data.m_Data1;

	ASSERT_RETURN(pComponent);

	DWORD dwDuration = (DWORD)pComponent->m_dwParam2;	// in milliseconds

	UIComponentActivate(pComponent, TRUE);	// fades in
	UIComponentActivate(pComponent, FALSE, dwDuration);	// register a future fade out

	//DWORD dwMaxAnimDuration = 0;
	//UIComponentFadeIn(pComponent, STD_ANIM_DURATION_MULT, dwMaxAnimDuration, FALSE);

	//pComponent->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwMaxAnimDuration + dwDuration, UIDelayedFadeout, CEVENT_DATA((DWORD)pComponent->m_idComponent, (DWORD)TRUE));

	if ((DWORD)pComponent->m_dwParam > 0)
		UIRatingLogoOnPostCreate(pComponent, 0, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRatingLogoOnPostCreate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{

	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	// show again in a period of time

	DWORD dwPeriod = (DWORD)pComponent->m_dwParam;		// in seconds

	if (CSchedulerIsValidTicket(pComponent->m_tThrobAnimInfo.m_dwAnimTicket))
	{
		CSchedulerCancelEvent(pComponent->m_tThrobAnimInfo.m_dwAnimTicket);
	}
	pComponent->m_tThrobAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + (dwPeriod * MSECS_PER_SEC), sRatingLogoEvent, CEVENT_DATA((DWORD_PTR)pComponent) );

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharSelectRatingLogoEvent(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT *pComponent = (UI_COMPONENT *)data.m_Data1;

	ASSERT_RETURN(pComponent);

	if (UIShowingLoadingScreen())		// loading screen is up, try again later
	{
		pComponent->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + MSECS_PER_SEC, sCharSelectRatingLogoEvent, CEVENT_DATA(data.m_Data1));
		return;
	}

	DWORD dwDuration = (DWORD)pComponent->m_dwParam2;	// in milliseconds

	UIComponentActivate(pComponent, TRUE);	// fades in
	UIComponentActivate(pComponent, FALSE, dwDuration);	// register a future fade out

	//DWORD dwMaxAnimDuration = 0;
	//UIComponentFadeIn(pComponent, STD_ANIM_DURATION_MULT, dwMaxAnimDuration, FALSE);

	//pComponent->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwMaxAnimDuration + dwDuration, UIDelayedFadeout, CEVENT_DATA((DWORD)pComponent->m_idComponent, (DWORD)TRUE));

	if ((DWORD)pComponent->m_dwParam > 0)
		UIRatingLogoOnPostCreate(pComponent, 0, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharSelectRatingLogoOnPostActivate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{

	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	// show again in a period of time

	DWORD dwPeriod = (DWORD)pComponent->m_dwParam;		// in seconds

	if (CSchedulerIsValidTicket(pComponent->m_tMainAnimInfo.m_dwAnimTicket))
	{
		CSchedulerCancelEvent(pComponent->m_tMainAnimInfo.m_dwAnimTicket);
	}
	pComponent->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + (dwPeriod * MSECS_PER_SEC), sCharSelectRatingLogoEvent, CEVENT_DATA((DWORD_PTR)pComponent) );

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
UI_MSG_RETVAL UIVersionLabelOnPostCreate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{

	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	WCHAR szBuildVersion[MAX_XML_STRING_LENGTH];
	PStrPrintf( szBuildVersion, MAX_XML_STRING_LENGTH, L"%s%S%s%s%d.%d.%d.%d",
#if ISVERSION(DEVELOPMENT)
#	ifdef _WIN64
		L"x64 ",
#	else
		L"x86 ",
#	endif
		e_GetTargetName(), L"\nDevelopment\n",
#	if ISVERSION(OPTIMIZED_VERSION)
		L"Optimized\n",
#	else
		L"",
#	endif // OPTIMIZED_VERSION
#else
		L"", "", L"", L"",
#endif // DEVELOPMENT
		TITLE_VERSION,PRODUCTION_BRANCH_VERSION,RC_BRANCH_VERSION,MAIN_LINE_VERSION );

	UILabelSetText( pComponent, szBuildVersion );

	return UIMSG_RET_HANDLED;
}

//////////////////////////////////////////////////////////////////////////////
// TUGBOAT PANE MENUS
//////////////////////////////////////////////////////////////////////////////

#define BOTTOM_HUD_HEIGHT		48 // No pane menu may go lower than the top of the bottom hud
#define MINIMUM_PLAYABLE_WIDTH	300 // the minimum playable width allowed as viable after the panes have been arranged.
#define PANE_WIDTH				400	// standard pane width
#define PANE_HEIGHT				276	// standard pane height



#define PANE_FILLER_COUNT		8

#define PANE_ALIGN_LEFT			FALSE


// ALL THE PANE MENUS AVAILABLE TO SCRIPT
const static UI_COMPONENT_ENUM CompPaneMenus[KPaneMenus] =
{
	UICOMP_INVALID,
	UICOMP_PANEMENU_BACKPACK,
	UICOMP_PANEMENU_EQUIPMENT,
	UICOMP_PANEMENU_CHARACTERSHEET,
	UICOMP_PANEMENU_SKILLS,
	UICOMP_PANEMENU_CRAFTING,
	UICOMP_PANEMENU_QUESTS,
	UICOMP_PANEMENU_MERCHANT,
	UICOMP_PANEMENU_STASH,
	UICOMP_PANEMENU_TRADE,
	UICOMP_PANEMENU_RECIPE,
	UICOMP_PANEMENU_ITEMAUGMENT,
	UICOMP_ACHIEVEMENTS,
	UICOMP_PANEMENU_HIRELINGEQUIPMENT,
	UICOMP_PANEMENU_CUBE,
	UICOMP_PANEMENU_CRAFTINGRECIPE,
	UICOMP_PANEMENU_MAIL,
	UICOMP_PANEMENU_CRAFTINGTRAINER,
};

// FILLER PANES
const static UI_COMPONENT_ENUM CompPaneFillers[PANE_FILLER_COUNT] =
{
	UICOMP_PANEMENU_FILLER1,
	UICOMP_PANEMENU_FILLER2,
	UICOMP_PANEMENU_FILLER3,
	UICOMP_PANEMENU_FILLER4,
	UICOMP_PANEMENU_FILLER5,
	UICOMP_PANEMENU_FILLER6,
	UICOMP_PANEMENU_FILLER7,
	UICOMP_PANEMENU_FILLER8,
};

// Panes closed as a secondary action if the original pane is closed
const static UI_COMPONENT_ENUM CompPaneSecondaryClose[KPaneMenus] =
{
	UICOMP_INVALID,
	UICOMP_PANEMENU_MERCHANT,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_BACKPACK,
	UICOMP_PANEMENU_BACKPACK,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_BACKPACK,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_BACKPACK,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_BACKPACK
};

// Panes closed as a tertiary action if the original pane is closed
const static UI_COMPONENT_ENUM CompPaneTertiaryClose[KPaneMenus] =
{
	UICOMP_INVALID,
	UICOMP_PANEMENU_STASH,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_EQUIPMENT,
	UICOMP_PANEMENU_EQUIPMENT,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_EQUIPMENT,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_EQUIPMENT,
	UICOMP_INVALID,
	UICOMP_INVALID,
	UICOMP_PANEMENU_EQUIPMENT
};


const static int kPanePriority[KPaneMenus] =
{
	-1,
	4,
	2,
	6,
	7,
	1,
	9,
	5,
	10,
	11,
	12,
	13,
	14,
	15,
	16,
	3,
	0,
	17
};

const static float kPaneHeight[KPaneMenus] =
{
	0,
	1,  //1 UICOMP_PANEMENU_BACKPACK
	1,  
	1,  //3 UICOMP_PANEMENU_CHARACTERSHEET
	2,
	2,  //5 UICOMP_PANEMENU_CRAFTING
	2,
	2,
	2,
	1,
	1,
	1,
	2,
	1,
	1,
	1,
	2,
	1
};

static float gfCoverageWidth = 0;
static BOOL  bCovered = FALSE;
// the order in which Panes were opened - for determining oldest to close
static int gCompPaneHistory[KPaneMenus];
// current order of opened panes
static int gCurrentPaneOrder = 0;

//////////////////////////////////////////////////////////////////////////////
// Hide everything, and as a result reset the history
//////////////////////////////////////////////////////////////////////////////
void UIPanesInit( void )
{
	UIHideAllPaneMenus();
} // UIPanesInit()

//////////////////////////////////////////////////////////////////////////////
//retrieve pane index by enum
//////////////////////////////////////////////////////////////////////////////
static int sPaneIndex( UI_COMPONENT_ENUM Pane )
{
	for( int i = 0; i < KPaneMenus; i++ )
	{
		if( Pane == CompPaneMenus[i] )
		{
			return i;
		}
	}
	return -1;
} // sPaneIndex()

//----------------------------------------------------------------------------
//sort for active pane order
//----------------------------------------------------------------------------
static int sPaneMenuCompare(
	const void * item1,
	const void * item2)
{
	const UI_COMPONENT_ENUM * a = (const UI_COMPONENT_ENUM *)item1;
	const UI_COMPONENT_ENUM * b = (const UI_COMPONENT_ENUM *)item2;
	int P1 = kPanePriority[sPaneIndex(*a)];
	int P2 = kPanePriority[sPaneIndex(*b)];
	if ( P1 < P2 )
	{
		return -1;
	}
	else if ( P2 < P1 )
	{
		return 1;
	}
	return 0;
} // sPaneMenuCompare()

//////////////////////////////////////////////////////////////////////////////
//sort for active pane order
//////////////////////////////////////////////////////////////////////////////
static int sPaneMenuHistoryCompare(
							const void * item1,
							const void * item2)
{
	const UI_COMPONENT_ENUM * a = (const UI_COMPONENT_ENUM *)item1;
	const UI_COMPONENT_ENUM * b = (const UI_COMPONENT_ENUM *)item2;
	int P1 = gCompPaneHistory[sPaneIndex(*a)];
	int P2 = gCompPaneHistory[sPaneIndex(*b)];
	if ( P1 < P2 )
	{
		return -1;
	}
	else if ( P2 < P1 )
	{
		return 1;
	}
	return 0;
} // sPaneMenuHistoryCompare()

//////////////////////////////////////////////////////////////////////////////
// Close oldest open pane
//////////////////////////////////////////////////////////////////////////////

void UICloseOldestPaneMenu( BOOL bRearrangePanels )
{

	UI_COMPONENT_ENUM ActivePanes[KPaneMenus];
	int nActivePaneCount = 0;
	for( int i = 1; i < KPaneMenus; i++ )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneMenus[i] );
		if( UIComponentGetVisible( pPaneComponent ) )
		{
			ActivePanes[nActivePaneCount] = CompPaneMenus[i];
			nActivePaneCount++;
		}
	}

	qsort(ActivePanes, nActivePaneCount, sizeof(UI_COMPONENT_ENUM), sPaneMenuHistoryCompare);

	// deactivate the oldest pane
	if( nActivePaneCount > 0 )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( ActivePanes[0] );

		//UIComponentActivate(pPaneComponent, FALSE);
		UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
		UIComponentSetActive( pPaneComponent, FALSE );
		UIComponentSetVisible( pPaneComponent, FALSE );
	}

	if( bRearrangePanels )
	{
		UIUpdatePaneMenuPositions();
	}
} // UICloseOldestPaneMenu( )

//////////////////////////////////////////////////////////////////////////////
// Shuffle all visible panes by priority and place them onscreen
//////////////////////////////////////////////////////////////////////////////

void UIUpdatePaneMenuPositions( void )
{

	UI_COMPONENT_ENUM ActivePanes[KPaneMenus];
 	int nActivePaneCount = 0;
	for( int i = 1; i < KPaneMenus; i++ )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneMenus[i] );
		if( pPaneComponent && UIComponentGetVisible( pPaneComponent ) )
		{
			ActivePanes[nActivePaneCount] = CompPaneMenus[i];
			nActivePaneCount++;
		}
	}

	qsort(ActivePanes, nActivePaneCount, sizeof(UI_COMPONENT_ENUM), sPaneMenuCompare);

	// hide all our filler panes ahead of time
	for( int i = 0; i < PANE_FILLER_COUNT; i++ )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneFillers[i] );
		if( UIComponentGetVisible( pPaneComponent ) )
		{
			UIComponentSetVisible( pPaneComponent, FALSE );
		}

	}
	// now that we have a list, sort from the left, going downward until we wrap up again.
	// we can pack more panes in on higher res, because we don't scale - so first we need
	// to find the max width and height onscreen
	int nScreenWidth, nScreenHeight;
	e_GetWindowSize( nScreenWidth, nScreenHeight );


	int nCurrentFillerPane = 0;
	float fX = 0;
	float fY = 0;
	bool bOutOfRoom = false;
	gfCoverageWidth = 0;
	for( int i = 0; i < nActivePaneCount; i++ )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( ActivePanes[i] );
		if( bOutOfRoom ) // no more room - hide remaining menus.
		{
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
			continue;
		}
		if( PANE_ALIGN_LEFT )
		{
			pPaneComponent->m_Position.m_fX = fX;
		}
		else
		{
			pPaneComponent->m_Position.m_fX = ( nScreenWidth - fX - ( PANE_WIDTH * g_UI.m_fUIScaler) );
		}
		pPaneComponent->m_Position.m_fY = fY;
		pPaneComponent->m_InactivePos = pPaneComponent->m_ActivePos = pPaneComponent->m_Position;
		UIComponentHandleUIMessage( pPaneComponent, UIMSG_PAINT, 0, 0, TRUE);
		float fPaneHeight = ( PANE_HEIGHT * g_UI.m_fUIScaler) * kPaneHeight[sPaneIndex( ActivePanes[i] )];
		fY += fPaneHeight;
		gfCoverageWidth = fX + ( PANE_WIDTH * g_UI.m_fUIScaler);
		float fNextPaneHeight = ( PANE_HEIGHT * g_UI.m_fUIScaler);
		if( i < nActivePaneCount - 1 )
		{

			fNextPaneHeight = ( PANE_HEIGHT * g_UI.m_fUIScaler) * kPaneHeight[sPaneIndex( ActivePanes[i+1] )];
		}

		// we wrapped around the bottom
		if( i == nActivePaneCount - 1 ||
			fY + fNextPaneHeight > nScreenHeight - BOTTOM_HUD_HEIGHT  )
		{
			// add a filler pane
			if( fY < nScreenHeight - BOTTOM_HUD_HEIGHT )
			{
				UI_COMPONENT* pFillerComponent = UIComponentGetByEnum( CompPaneFillers[nCurrentFillerPane] );
				UIComponentSetActive( pFillerComponent, TRUE );
				nCurrentFillerPane++;
				pFillerComponent->m_fHeight = ( nScreenHeight ) - fY;
				pFillerComponent->m_fWidth = ( PANE_WIDTH * g_UI.m_fUIScaler);
				if( PANE_ALIGN_LEFT )
				{
					pFillerComponent->m_Position.m_fX = fX;
				}
				else
				{
					pFillerComponent->m_Position.m_fX = ( nScreenWidth - fX - ( PANE_WIDTH * g_UI.m_fUIScaler) );
				}
				pFillerComponent->m_Position.m_fY = fY;
				pFillerComponent->m_InactivePos = pFillerComponent->m_ActivePos = pFillerComponent->m_Position;

				UIComponentHandleUIMessage(pFillerComponent, UIMSG_PAINT, 0, 0, TRUE);
			}
			fY = 0;
			fX += ( PANE_WIDTH * g_UI.m_fUIScaler);
			// if we've pushed beyond the right of the screen, it's all over - it's covered
			if( fX + ( PANE_WIDTH * g_UI.m_fUIScaler) > nScreenWidth )
			{
				bOutOfRoom = true;
				if( i < nActivePaneCount - 1 )
				{
					// close the oldest pane - then reflow again, and this can recurse until it is happy.
					UICloseOldestPaneMenu( TRUE );
					return;
				}
			}
		}

	}
	// OK, if we don't have enough room left to play in, center all the elements, and throw gutter fills on. It looks
	// way nicer that way.
	if( !( gfCoverageWidth <= nScreenWidth - MINIMUM_PLAYABLE_WIDTH ) && gfCoverageWidth < nScreenWidth - 5 )
	{
		float Delta = ( nScreenWidth - gfCoverageWidth ) * .5f;
		if( !PANE_ALIGN_LEFT )
		{
			Delta *= -1;
		}
		gfCoverageWidth = (float)nScreenWidth;
		for( int i = 0; i < nActivePaneCount; i++ )
		{
			UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( ActivePanes[i] );
			if( UIComponentGetVisible( pPaneComponent ) )
			{
				pPaneComponent->m_Position.m_fX += Delta;
				pPaneComponent->m_InactivePos = pPaneComponent->m_ActivePos = pPaneComponent->m_Position;
				UIComponentHandleUIMessage( pPaneComponent, UIMSG_PAINT, 0, 0, TRUE);
			}
		}

		// offset any current filler panes too
		for( int i = 0; i < nCurrentFillerPane; i++ )
		{
			UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneFillers[i] );
			if( UIComponentGetVisible( pPaneComponent ) )
			{
				pPaneComponent->m_Position.m_fX += Delta;
				pPaneComponent->m_InactivePos = pPaneComponent->m_ActivePos = pPaneComponent->m_Position;
				UIComponentHandleUIMessage( pPaneComponent, UIMSG_PAINT, 0, 0, TRUE);
			}
		}

		// add 2 filler panes in the gutters
		if( fY < nScreenHeight - BOTTOM_HUD_HEIGHT )
		{
			UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneFillers[nCurrentFillerPane] );
			UIComponentSetActive( pPaneComponent, TRUE );
			nCurrentFillerPane++;
			pPaneComponent->m_fHeight = (float)nScreenHeight;
			pPaneComponent->m_fWidth = fabs(Delta);
			pPaneComponent->m_Position.m_fX = 0;
			pPaneComponent->m_Position.m_fY = 0;
			pPaneComponent->m_InactivePos = pPaneComponent->m_ActivePos = pPaneComponent->m_Position;
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_PAINT, 0, 0, TRUE);

			pPaneComponent = UIComponentGetByEnum( CompPaneFillers[nCurrentFillerPane] );
			UIComponentSetActive( pPaneComponent, TRUE );
			nCurrentFillerPane++;
			pPaneComponent->m_fHeight = (float)nScreenHeight;
			pPaneComponent->m_fWidth = fabs(Delta);
			pPaneComponent->m_Position.m_fX = nScreenWidth - fabs(Delta);
			pPaneComponent->m_Position.m_fY = 0;
			pPaneComponent->m_InactivePos = pPaneComponent->m_ActivePos = pPaneComponent->m_Position;
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_PAINT, 0, 0, TRUE);
		}
	}



	UI_COMPONENT * pAutomap = UIComponentGetByEnum( UICOMP_AUTOMAP );
	// if right- hand aligned, we'll scoot over the automap into normal space instead of over menus. Nicer that way.
	if( pAutomap && !PANE_ALIGN_LEFT )
	{
		pAutomap->m_Position.m_fX = ( nScreenWidth - gfCoverageWidth ) * UIGetScreenToLogRatioX();
		pAutomap->m_ActivePos = pAutomap->m_InactivePos = pAutomap->m_Position;
 		if( UIComponentGetVisible( pAutomap ) )
		{
			UIComponentHandleUIMessage( pAutomap, UIMSG_PAINT, 0, 0, TRUE);
		}
	}

	UI_COMPONENT * pDropdown = UIComponentGetByEnum( UICOMP_TOWN_INSTANCE_COMBO );
	// if right- hand aligned, we'll scoot over the instance dropdown into normal space instead of over menus. Nicer that way.
	if( pDropdown && !PANE_ALIGN_LEFT )
	{
		pDropdown = pDropdown->m_pParent;
		pDropdown->m_Position.m_fX = ( nScreenWidth - gfCoverageWidth ) * UIGetScreenToLogRatioX();
		pDropdown->m_ActivePos = pDropdown->m_InactivePos = pDropdown->m_Position;
		if( UIComponentGetVisible( pDropdown->m_pFirstChild ) )
		{
			UIComponentHandleUIMessage( pDropdown->m_pFirstChild, UIMSG_PAINT, 0, 0, TRUE);
		}
	}

	UI_COMPONENT * pLevelName = UIComponentGetByEnum( UICOMP_LEVELNAME );
	// if right- hand aligned, we'll scoot over the level name into normal space instead of over menus. Nicer that way.
	if( pLevelName && !PANE_ALIGN_LEFT )
	{
		pLevelName->m_Position.m_fX = ( nScreenWidth - gfCoverageWidth ) * UIGetScreenToLogRatioX();
		pLevelName->m_ActivePos = pLevelName->m_InactivePos = pLevelName->m_Position;
		if( UIComponentGetVisible( pLevelName ) )
		{
			UIComponentHandleUIMessage( pLevelName, UIMSG_PAINT, 0, 0, TRUE);
		}
	}

	UI_COMPONENT * pTaskLog = UIComponentGetByEnum( UICOMP_QUEST_TRACKING );
	// if right- hand aligned, we'll scoot over the level name into normal space instead of over menus. Nicer that way.
	if( pTaskLog && !PANE_ALIGN_LEFT )
	{
		pTaskLog->m_Position.m_fX = ( nScreenWidth - gfCoverageWidth ) * UIGetScreenToLogRatioX();
		pTaskLog->m_ActivePos = pTaskLog->m_InactivePos = pTaskLog->m_Position;
		if( UIComponentGetVisible( pTaskLog ) )
		{
			UIComponentHandleUIMessage( pTaskLog, UIMSG_PAINT, 0, 0, TRUE);
		}
	}


	UI_COMPONENT * pStateDisplay = UIComponentGetByEnum( UICOMP_STATES_DISPLAY );
	// if right- hand aligned, we'll scoot over the level name into normal space instead of over menus. Nicer that way.
	if( pStateDisplay && !PANE_ALIGN_LEFT )
	{
		pStateDisplay->m_Position.m_fX = ( nScreenWidth - gfCoverageWidth ) * UIGetScreenToLogRatioX();
		pStateDisplay->m_ActivePos = pStateDisplay->m_InactivePos = pStateDisplay->m_Position;
		if( UIComponentGetVisible( pStateDisplay ) )
		{
			UIComponentHandleUIMessage( pStateDisplay, UIMSG_PAINT, 0, 0, TRUE);
		}
	}

	e_SetUICoverage( gfCoverageWidth,
					 !( gfCoverageWidth <= nScreenWidth - MINIMUM_PLAYABLE_WIDTH ),
					 PANE_ALIGN_LEFT );

	// Now update party and pet menus to hide them if they are occluded

	UI_COMPONENT* pPartyComponent = UIComponentGetByEnum(UICOMP_PARTY_MEMBER_PANEL);
	if( pPartyComponent )
	{
		UIPartyDisplayOnPartyChange( pPartyComponent, UIMSG_PAINT, 0, 0 );
	}
} // UIUpdatePaneMenuPositions()

//////////////////////////////////////////////////////////////////////////////
// Hide all pane menus
//////////////////////////////////////////////////////////////////////////////
BOOL UIHideAllPaneMenus( void )
{
	gCurrentPaneOrder = 0;
	BOOL bClosedComponent( FALSE );
	for( int i = 1; i < KPaneMenus; i++ )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneMenus[i] );
		if( pPaneComponent && UIComponentGetVisible( pPaneComponent ) )
		{
			bClosedComponent = TRUE;
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
		}
		gCompPaneHistory[i] = -1;
	}

	// hide all our filler panes
	for( int i = 0; i < PANE_FILLER_COUNT; i++ )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneFillers[i] );
		if( pPaneComponent )
		{
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0 );
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
		}
	}

	UIUpdatePaneMenuPositions();
	return bClosedComponent;
} // // UIUpdatePaneMenuPositions())

//////////////////////////////////////////////////////////////////////////////
// hide any linked secondary components of a pane
//////////////////////////////////////////////////////////////////////////////
void sUIHideSecondaryPane( EPaneMenu Pane )
{
	if( CompPaneSecondaryClose[Pane] != UICOMP_INVALID )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneSecondaryClose[Pane] );
		// visible, we hide it
		if( UIComponentGetVisible( pPaneComponent ) )
		{
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
		}
	}

	if( CompPaneTertiaryClose[Pane] != UICOMP_INVALID )
	{
		UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneTertiaryClose[Pane] );
		// visible, we hide it
		if( UIComponentGetVisible( pPaneComponent ) )
		{
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
		}
	}
} // void sUIHideSecondaryPane()

//////////////////////////////////////////////////////////////////////////////
// Is the pane onscreen?
//////////////////////////////////////////////////////////////////////////////
BOOL UIPaneVisible( EPaneMenu Pane )
{
	UI_COMPONENT* pPaneComponent = UIComponentGetByEnum( CompPaneMenus[Pane] );
	// visible, we hide it
	if( UIComponentGetVisible( pPaneComponent ) )
	{
		return TRUE;
	}
	return FALSE;

} // BOOL UIPaneVisible()

//////////////////////////////////////////////////////////////////////////////
// Show panes regardless of their current visible state
//////////////////////////////////////////////////////////////////////////////

void UIShowPaneMenu( EPaneMenu Pane1,
					 EPaneMenu Pane2 )
{
	UI_COMPONENT* pPaneComponent = NULL;
	UI_COMPONENT* pPaneComponent2 = NULL;
	if( Pane1 > KPaneMenuNone && Pane1 < KPaneMenus )
	{
		pPaneComponent = UIComponentGetByEnum( CompPaneMenus[Pane1] );
	}
	if( Pane2 > KPaneMenuNone && Pane2 < KPaneMenus )
	{
		pPaneComponent2 = UIComponentGetByEnum( CompPaneMenus[Pane2] );
	}
	if( pPaneComponent )
	{
		if( !UIComponentGetVisible( pPaneComponent ) )
		{
			gCompPaneHistory[Pane1] = gCurrentPaneOrder;
			gCurrentPaneOrder++;
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_ACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, TRUE );
		}
	}


	if( pPaneComponent2 )
	{
		if( !UIComponentGetVisible( pPaneComponent2 ) )
		{
			gCompPaneHistory[Pane2] = gCurrentPaneOrder;
			gCurrentPaneOrder++;
			UIComponentHandleUIMessage(pPaneComponent2, UIMSG_ACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent2, TRUE );
		}
	}

	UIUpdatePaneMenuPositions();
} // void UIShowPaneMenu()


//////////////////////////////////////////////////////////////////////////////
// Hide panes regardless of their current visible state
//////////////////////////////////////////////////////////////////////////////

void UIHidePaneMenu( EPaneMenu Pane1,
					 EPaneMenu Pane2 )
{
	UI_COMPONENT* pPaneComponent = NULL;
	UI_COMPONENT* pPaneComponent2 = NULL;
	if( Pane1 > KPaneMenuNone && Pane1 < KPaneMenus )
	{
		pPaneComponent = UIComponentGetByEnum( CompPaneMenus[Pane1] );
	}
	if( Pane2 > KPaneMenuNone && Pane2 < KPaneMenus )
	{
		pPaneComponent2 = UIComponentGetByEnum( CompPaneMenus[Pane2] );
	}
	if( pPaneComponent )
	{
		if( UIComponentGetVisible( pPaneComponent ) )
		{
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
			sUIHideSecondaryPane( Pane1 );
		}
	}


	if( pPaneComponent2 )
	{
		if( UIComponentGetVisible( pPaneComponent2 ) )
		{
			UIComponentHandleUIMessage(pPaneComponent2, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent2, FALSE );
			UIComponentSetVisible( pPaneComponent2, FALSE );
			sUIHideSecondaryPane( Pane2 );
		}
	}

	UIUpdatePaneMenuPositions();
} // void UIHiePaneMenu()

//////////////////////////////////////////////////////////////////////////////
// Toggle pane menus by their index - rather than do a bunch of custom show/hides per menu, which is a pain.
// two panes can be shown at once by using both params - but if two are requested, and one of them is off
// and the other is on, it will hide instead
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UITogglePaneMenu( EPaneMenu Pane1,
							    EPaneMenu Pane2,
							    EPaneMenu Pane3 )
{
	UI_MSG_RETVAL RetVal = UIMSG_RET_NOT_HANDLED;

	UI_COMPONENT* pPaneComponent = NULL;
	UI_COMPONENT* pPaneComponent2 = NULL;
	UI_COMPONENT* pPaneComponent3 = NULL;
	if( Pane1 > KPaneMenuNone && Pane1 < KPaneMenus )
	{
		pPaneComponent = UIComponentGetByEnum( CompPaneMenus[Pane1] );
	}
	if( Pane2 > KPaneMenuNone && Pane2 < KPaneMenus )
	{
		pPaneComponent2 = UIComponentGetByEnum( CompPaneMenus[Pane2] );
	}
	if( Pane3 > KPaneMenuNone && Pane3 < KPaneMenus )
	{
		pPaneComponent3 = UIComponentGetByEnum( CompPaneMenus[Pane3] );
	}
	if( pPaneComponent && pPaneComponent2 )
	{
		if( ( UIComponentGetVisible( pPaneComponent )  &&
			  !UIComponentGetVisible( pPaneComponent2 ) ) ||
			  ( UIComponentGetVisible( pPaneComponent2 )  &&
			  !UIComponentGetVisible( pPaneComponent ) ) )
		{
			sUIHideSecondaryPane( Pane1 );
			sUIHideSecondaryPane( Pane2 );
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentHandleUIMessage(pPaneComponent2, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetActive( pPaneComponent2, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent2, FALSE );

			if( pPaneComponent3 )
			{				
				sUIHideSecondaryPane( Pane3 );
				UIComponentHandleUIMessage(pPaneComponent3, UIMSG_INACTIVATE, 0, 0);
				UIComponentSetActive( pPaneComponent3, FALSE );
				UIComponentSetVisible( pPaneComponent3, FALSE );
			}

			UIUpdatePaneMenuPositions();
			return UIMSG_RET_HANDLED; // input used
		}
	}
	int nVisible = 0;
	if( pPaneComponent )
	{
		// visible, we hide it
		if( UIComponentGetVisible( pPaneComponent ) )
		{
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, FALSE );
			UIComponentSetVisible( pPaneComponent, FALSE );
			sUIHideSecondaryPane( Pane1 );
		}
		else // otherwise, activate it
		{
			gCompPaneHistory[Pane1] = gCurrentPaneOrder;
			UIComponentHandleUIMessage(pPaneComponent, UIMSG_ACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent, TRUE );
			nVisible++;
		}
		RetVal = UIMSG_RET_HANDLED;  // input used
	}


	if( pPaneComponent2 )
	{
		// visible, we hide it
		if( UIComponentGetVisible( pPaneComponent2 ) )
		{
			UIComponentHandleUIMessage(pPaneComponent2, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent2, FALSE );
			UIComponentSetVisible( pPaneComponent2, FALSE );
			sUIHideSecondaryPane( Pane2 );
		}
		else // otherwise, activate it
		{
			gCompPaneHistory[Pane2] = gCurrentPaneOrder;
			gCurrentPaneOrder++;
			UIComponentHandleUIMessage(pPaneComponent2, UIMSG_ACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent2, TRUE );
			nVisible++;
		}
		RetVal = UIMSG_RET_HANDLED;  // input used
	}

	if( pPaneComponent3 )
	{
		// visible, we hide it
		if( UIComponentGetVisible( pPaneComponent3 ) && nVisible < 2 )
		{
			UIComponentHandleUIMessage(pPaneComponent3, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent3, FALSE );
			UIComponentSetVisible( pPaneComponent3, FALSE );
			sUIHideSecondaryPane( Pane3 );
		}
		else // otherwise, activate it
		{
			gCompPaneHistory[Pane3] = gCurrentPaneOrder;
			gCurrentPaneOrder++;
			UIComponentHandleUIMessage(pPaneComponent3, UIMSG_ACTIVATE, 0, 0);
			UIComponentSetActive( pPaneComponent3, TRUE );
		}
		RetVal = UIMSG_RET_HANDLED;  // input used
	}
	UIUpdatePaneMenuPositions();
	return RetVal;
} // UI_MSG_RETVAL UITogglePaneMenu()

//////////////////////////////////////////////////////////////////////////////
// Toggle pane menus by their index - rather than do a bunch of custom show/hides per menu, which is a pain.
// two panes can be shown at once by using both params - but if two are requested, and one of them is off
// and the other is on, it will hide instead
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UITogglePaneMenu(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	return UITogglePaneMenu( (EPaneMenu)pComponent->m_dwParam, (EPaneMenu)pComponent->m_dwParam2, (EPaneMenu)pComponent->m_dwParam3 );

} // UI_MSG_RETVAL UITogglePaneMenu()




//----------------------------------------------------------------------------
// Tugboat-specific functionality that doesn't have to do with the Pane menu system
//----------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// Hide all modal dialogs
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_ModalDialogsOpen( void )
{
	if( UI_TB_IsWorldMapScreenUp() ||
		UI_TB_IsTravelMapScreenUp() ||
		UI_TB_IsNPCDialogUp() ||
		UI_TB_IsGenericDialogUp() )
	{
		return TRUE;
	}
	return FALSE;
} // UI_TB_ModalDialogsOpen()

//////////////////////////////////////////////////////////////////////////////
// Hide all modal dialogs
//////////////////////////////////////////////////////////////////////////////
void UI_TB_HideModalDialogs( void )
{
	if (AppGetCltGame() == NULL) {
		return;
	}


	UI_TB_HideTravelMapScreen( TRUE );
	UI_TB_HideWorldMapScreen( TRUE );
	UI_TB_HideTownPortalScreen(  );
	UI_TB_HideAnchorstoneScreen(  );
	UI_TB_HideRunegateScreen(  );
	if( UI_TB_IsNPCDialogUp() )
	{
		UIDoConversationComplete(
			UIComponentGetByEnum(UICOMP_NPCDIALOGBOX),
			CCT_CANCEL );
	}
	UIHideGenericDialog();

} // UI_TB_HideModalDialogs( void ))

//////////////////////////////////////////////////////////////////////////////
// Hide all menus, including modals
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_HideAllMenus( void )
{
	if (AppGetCltGame() == NULL) {
		return FALSE;
	}
	BOOL bClosedComponent( FALSE );

	bClosedComponent = UIHideAllPaneMenus();

	bClosedComponent = bClosedComponent | UI_TB_HideWorldMapScreen( TRUE );
	UIHideAllPaneMenus();

	UI_TB_HideTravelMapScreen( TRUE );

	UIHideBuddylist();
	UIHideGuildPanel();
	UIHidePartyPanel();
	UI_TB_HideTownPortalScreen();
	UI_TB_HideAnchorstoneScreen();
	UI_TB_HideRunegateScreen();
	if( UI_TB_IsNPCDialogUp() )
	{
		bClosedComponent = TRUE;
		UI_COMPONENT * pDlgBackground = UIComponentGetByEnum(UICOMP_NPCDIALOGBOX);
		if( pDlgBackground )
		{
			UI_COMPONENT *pCancelButton = UIComponentFindChildByName(pDlgBackground, "dialog cancel");
			if( pCancelButton->m_bVisible )
			{
				UIDoConversationComplete(
					UIComponentGetByEnum(UICOMP_NPCDIALOGBOX),
					CCT_CANCEL );
			}
		}
	}
	bClosedComponent = bClosedComponent | UIHideGenericDialog();

	// make sure to shut down any recipes that got stopped midway.
	bClosedComponent = bClosedComponent | c_RecipeClose( TRUE );
	return bClosedComponent;
} // UI_TB_HideAllMenus( )

//////////////////////////////////////////////////////////////////////////////
// NPC interaction dialog open?
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_IsNPCDialogUp( void )
{

	UI_COMPONENT* dialog_screen = UIComponentGetByEnum(UICOMP_NPCDIALOGBOX);
	if ( dialog_screen && UIComponentGetVisible( dialog_screen ) )
	{
		return TRUE;
	}
	if( UI_TB_IsPortalScreenUp() )
	{
		return TRUE;
	}
	return FALSE;
} // UI_TB_IsNPCDialogUp()


//////////////////////////////////////////////////////////////////////////////
// Generic nonmodal dialog up?
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_IsGenericDialogUp( void )
{

	UI_COMPONENT* dialog_screen = UIComponentGetByEnum(UICOMP_GENERIC_DIALOG);
	if (dialog_screen && UIComponentGetVisible(dialog_screen))
	{
		return TRUE;
	}

	return FALSE;
} // UI_TB_IsGenericDialogUp()

//////////////////////////////////////////////////////////////////////////////
// Town portal screen open?
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_IsPortalScreenUp( void )
{
	UI_COMPONENT* portal_screen = UIComponentGetByEnum(UICOMP_TOWN_PORTAL_DIALOG);
	if (portal_screen && UIComponentGetActive(portal_screen))
	{
		return TRUE;
	}

	return FALSE;
} // UI_TB_IsPortalScreenUp())

//////////////////////////////////////////////////////////////////////////////
// Is the local zone map screen visible?
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_IsWorldMapScreenUp( void )
{
	UI_COMPONENT* worldmap_screen = UIComponentGetByEnum(UICOMP_WORLDMAPSHEET);
	if (worldmap_screen && UIComponentGetActive(worldmap_screen))
	{
		return TRUE;
	}
	return FALSE;
} // UI_TB_IsWorldMapScreenUp()

//////////////////////////////////////////////////////////////////////////////
// Is the local zone travel screen visible?
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_IsTravelMapScreenUp( void )
{
	UI_COMPONENT* worldmap_screen = UIComponentGetByEnum(UICOMP_TRAVELMAPSHEET);
	if (worldmap_screen && UIComponentGetActive(worldmap_screen))
	{
		return TRUE;
	}
	return FALSE;
} // UI_TB_IsTravelMapScreenUp()

//////////////////////////////////////////////////////////////////////////////
// Show the local zone map
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_ShowWorldMapScreen( int nRevealArea /* = INVALID_ID */)
{

	UI_COMPONENT* worldmap_sheet = UIComponentGetByEnum(UICOMP_WORLDMAPSHEET);
	if (!worldmap_sheet)
	{
		return FALSE;
	}
	UI_TB_HideTravelMapScreen( TRUE );

	if( nRevealArea == INVALID_ID )
	{
		if( UIComponentGetActive( worldmap_sheet ) )
		{
			UI_TB_HideWorldMapScreen( TRUE );
			return TRUE;
		}
		else
		{
			UI_TB_HideModalDialogs();
		}
	}
	else
	{
		UI_TB_HideModalDialogs();
		static int nShowAreaSound = INVALID_LINK;
		if (nShowAreaSound == INVALID_LINK)
		{
			nShowAreaSound = GlobalIndexGet(  GI_SOUND_SKILL_SELECT );
		}
		if (nShowAreaSound != INVALID_LINK)
		{
			c_SoundPlay( nShowAreaSound );
		}
	}


	UIComponentSetActive(worldmap_sheet, TRUE);
	UIComponentHandleUIMessage(worldmap_sheet, UIMSG_PAINT, 0, 0);

	if( nRevealArea != INVALID_ID )
	{
		UI_TB_WorldMapSetZoneByArea( nRevealArea );
		UI_TB_WorldMapRevealArea( nRevealArea );
	}
	else
	{
		UI_COMPONENT* component = UIComponentGetByEnum(UICOMP_WORLD_MAP);
		UI_COMPONENT * pZoneCombo = UIComponentFindChildByName( component->m_pParent, "Reveal" );

		UIComponentSetActive( pZoneCombo, FALSE );
		UIComponentSetVisible( pZoneCombo, FALSE );
	}


	return TRUE;
} // UI_TB_ShowWorldMapScreen()


//////////////////////////////////////////////////////////////////////////////
// Show the local zone travel  map
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_ShowTravelMapScreen( void )
{

	UI_COMPONENT* worldmap_sheet = UIComponentGetByEnum(UICOMP_TRAVELMAPSHEET);
	if (!worldmap_sheet)
	{
		return FALSE;
	}
	UI_TB_HideWorldMapScreen( TRUE );


	if( UIComponentGetActive( worldmap_sheet ) )
	{
		UI_TB_HideTravelMapScreen( TRUE );
		return TRUE;
	}


	UIComponentSetActive(worldmap_sheet, TRUE);
	UIComponentHandleUIMessage(worldmap_sheet, UIMSG_PAINT, 0, 0);


	return TRUE;
} // UI_TB_ShowTravelMapScreen()

//////////////////////////////////////////////////////////////////////////////
// Hide the local zone map
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_HideWorldMapScreen( BOOL bEscapeOut )
{
	UI_COMPONENT* worldmap_sheet = UIComponentGetByEnum(UICOMP_WORLDMAPSHEET);
	if (!worldmap_sheet)
	{
		return FALSE;
	}

	if( !UIComponentGetVisible( worldmap_sheet ) )
	{
		return 0;
	}

	UIClosePopupMenus();

	UIComponentSetVisible( worldmap_sheet, FALSE );
	UIClearHoverText();
	return TRUE;
} // UI_TB_HideWorldMapScreen()



//////////////////////////////////////////////////////////////////////////////
// Hide the town portal screen
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_HideTownPortalScreen(  )
{
	UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_TOWN_PORTAL_DIALOG);
	if (!pComponent)
	{
		return FALSE;
	}

	if( !UIComponentGetVisible( pComponent ) )
	{
		return 0;
	}

	UIComponentActivate( pComponent, FALSE );
	UIClearHoverText();
	return TRUE;
} // UI_TB_HideTownPortalScreen()


//////////////////////////////////////////////////////////////////////////////
// Hide the anchorstone screen
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_HideAnchorstoneScreen(  )
{
	UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_ANCHORSTONE_PANEL);
	if (!pComponent)
	{
		return FALSE;
	}

	if( !UIComponentGetVisible( pComponent ) )
	{
		return 0;
	}

	UIComponentActivate( pComponent, FALSE );
	UIClearHoverText();
	return TRUE;
} // UI_TB_HideAnchorstoneScreen()


//////////////////////////////////////////////////////////////////////////////
// Hide the runegate screen
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_HideRunegateScreen(  )
{
	UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_RUNEGATE_PANEL);
	if (!pComponent)
	{
		return FALSE;
	}

	if( !UIComponentGetVisible( pComponent ) )
	{
		return 0;
	}

	UIComponentActivate( pComponent, FALSE );
	UIClearHoverText();
	return TRUE;
} // UI_TB_HideRunegateScreen()


//////////////////////////////////////////////////////////////////////////////
// Hide the local travel  map
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_HideTravelMapScreen( BOOL bEscapeOut )
{
	UI_COMPONENT* worldmap_sheet = UIComponentGetByEnum(UICOMP_TRAVELMAPSHEET);
	if (!worldmap_sheet)
	{
		return FALSE;
	}

	if( !UIComponentGetVisible( worldmap_sheet ) )
	{
		return 0;
	}
	UIClosePopupMenus();
	UIComponentSetVisible( worldmap_sheet, FALSE );
	UIClearHoverText();
	return TRUE;
} // UI_TB_HideTravelMapScreen()

//////////////////////////////////////////////////////////////////////////////
// Is the cursor currently over any panels that eat the messages and prevent
// them from reaching the screen?
//////////////////////////////////////////////////////////////////////////////
BOOL UI_TB_MouseOverPanel( void )
{
/*	float x, y;
	UIGetCursorPosition(&x, &y, FALSE);

	//int uimsg = UITranslateMessage(WM_MOUSEMOVE);
	int result = UIProcessMessage(WM_MOUSEMOVE, 0, MAKE_PARAM((int)x, (int)y) );
	if (result)
	{
		return TRUE;
	}
	return FALSE;*/
	return sgbMouseOverPanel;
} // UI_TB_MouseOverPanel()


//////////////////////////////////////////////////////////////////////////////
// Find any items lurking in the cursor slot and purge them
//////////////////////////////////////////////////////////////////////////////
void UI_TB_LookForBadItems( void )
{
	// Clear the cursor
	// make sure any active stuff doesn't stay in the hand slot
	UNITID idCursorUnit = UIGetCursorUnit();
	UNIT* unitCursor = UnitGetById(AppGetCltGame(), idCursorUnit);

	{
		UNIT* pItemOwner = UIGetControlUnit();



		int location = GlobalIndexGet( GI_INVENTORY_LOCATION_CURSOR );
		INVLOC_HEADER info;
		if (!UnitInventoryGetLocInfo(pItemOwner, location, &info))
		{
			return;
		}



		UNIT* item = UnitInventoryGetByLocationAndXY(pItemOwner, location, 0, 0 );
		// uh oh, there's something in the hand slot that shouldn't be there! why?
		if (item && !unitCursor &&
			!UnitIsWaitingForInventoryMove( item ) )
		{
			UIDropItem(UnitGetId(item));
		}
	}
} // UI_TB_LookForBadItems()

//////////////////////////////////////////////////////////////////////////////
// show character and skill sheets after a levelup
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UIShowCharacterAndSkillsScreen(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{

		UIShowPaneMenu( KPaneMenuCharacterSheet, KPaneMenuSkills );

		UI_COMPONENT *pLevelUp = UIComponentGetByEnum(UICOMP_LEVELUPPANEL);
		if ( pLevelUp )
		{
			UIComponentThrobEnd( pLevelUp, 0, 0, 0 );
			UIComponentSetVisible( pLevelUp, FALSE );
		}
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}	// UIShowCharacterAndSkillsScreen()

//////////////////////////////////////////////////////////////////////////////
// hide the levelup message
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UIHideLevelUpMessage(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) )
	{



		UI_COMPONENT *pLevelUp = UIComponentGetByEnum(UICOMP_LEVELUPPANEL);
		if ( pLevelUp )
		{
			UIComponentThrobEnd( pLevelUp, 0, 0, 0 );
			UIComponentSetVisible( pLevelUp, FALSE );
		}
		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}	// UIShowCharacterAndSkillsScreen()

//////////////////////////////////////////////////////////////////////////////
// select one of the reward offerings, and enable the accept reward button
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UISelectReward(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component)  )
	{
		UI_COMPONENT * pDlgBackground = UIComponentGetByEnum(UICOMP_NPCDIALOGBOX);


		UI_COMPONENT *pAcceptButton = UIComponentFindChildByName(pDlgBackground, "dialog accept");


		UIComponentSetActive( pAcceptButton, TRUE );

		return UIMSG_RET_HANDLED_END_PROCESS;  // input used
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}	// UISelectReward()

//////////////////////////////////////////////////////////////////////////////
// Perform the actual refresh
//////////////////////////////////////////////////////////////////////////////
void UITasksRefresh( void )
{
	
	if( UIPaneVisible( KPaneMenuQuests ) )
	{

		UI_PANELEX* panel = UICastToPanel(UIComponentGetByEnum(UICOMP_QUESTBUTTONS));
		int nTab = panel->m_nCurrentTab;
		if( nTab < 0 )
		{
			nTab = 0;
		}
		int nLastTab = nTab;
		c_TBRefreshTasks( nTab );
		nTab = nLastTab;
		if( nTab < 0 )
		{
			nTab = 0;
		}
		//UISetQuestButton( nTab );
		/*
		CHAR questName[] = { "buton quest999" };
		int nCount( 1 );
		sprintf_s( questName, "%s%d", "button quest", nCount++ );
		UI_COMPONENT *pQuestButton = UIComponentFindChildByName(pDlgBackground, questName);
		while( pQuestButton != NULL )
		{
			UI_COMPONENT* child = pQuestButton->m_pFirstChild;
			if( UIComponentIsButton( child ) )
			{
				UITasksQuestUpdateCheckMarks( child );
			}
			sprintf_s( questName, "%s%d", "button quest", nCount++ );
			pQuestButton = UIComponentFindChildByName(pDlgBackground, questName);
		}
		*/
	
	}

	c_QuestsUpdate( FALSE ,FALSE );

} // UITasksRefresh()

//////////////////////////////////////////////////////////////////////////////
// Refresh tasks pane
//////////////////////////////////////////////////////////////////////////////
UI_MSG_RETVAL UITasksOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UITasksRefresh();
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUpdatePing(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD )
{
	UI_COMPONENT *pMeter = UIComponentGetByEnum(UICOMP_PING_METER);
	if (pMeter)
	{
		if (CSchedulerIsValidTicket(pMeter->m_tMainAnimInfo.m_dwAnimTicket))
		{
			CSchedulerCancelEvent(pMeter->m_tMainAnimInfo.m_dwAnimTicket);
		}

		if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
		{
			UIComponentActivate(pMeter->m_pParent, FALSE);
		}
		else
		{
			UIComponentActivate(pMeter->m_pParent, TRUE);

			int nPingTime = c_GetPing();

			if (nPingTime < 0)
				return;

			UI_BAR *pBar = UICastToBar(pMeter);
			pBar->m_nMaxValue = (int)pBar->m_fWidth;

			if (nPingTime <= 16)	// all bars
			{
				pBar->m_nValue = pBar->m_nMaxValue;
			}
			else if (nPingTime <= 45) // 6 bars
			{
				pBar->m_nValue = pBar->m_nMaxValue - 5;
			}
			else if (nPingTime <= 100) // 5 bars
			{
				pBar->m_nValue = pBar->m_nMaxValue - 11;
			}
			else if (nPingTime <= 250) // 4 bars
			{
				pBar->m_nValue = pBar->m_nMaxValue - 17;
			}
			else if (nPingTime <= 500) // 3 bars
			{
				pBar->m_nValue = pBar->m_nMaxValue - 23;
			}
			else if (nPingTime <= 1000) // 2 bars
			{
				pBar->m_nValue = pBar->m_nMaxValue - 29;
			}
			else						// 1 bar
			{
				pBar->m_nValue = pBar->m_nMaxValue - 35;
			}

			UIComponentHandleUIMessage(pBar, UIMSG_PAINT, 0, 0);
			if (UICursorGetActive())
			{
				UIComponentHandleUIMessage(pBar, UIMSG_MOUSEHOVER, 0, 0, FALSE);
			}
		}

		DWORD dwDelay = (DWORD)pMeter->m_dwParam;
		pMeter->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + dwDelay, sUpdatePing, CEVENT_DATA());
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPingMeterOnPostCreate(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	sUpdatePing(AppGetCltGame(), CEVENT_DATA(), 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPingMeterOnMouseHover(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{
		if (component->m_szTooltipText)
		{
			UI_BAR *pBar = UICastToBar(component);
			WCHAR szTemp[256];
			PStrPrintf(szTemp, arrsize(szTemp), component->m_szTooltipText, c_GetPing());
			UISetSimpleHoverText(pBar, szTemp, TRUE, pBar->m_idComponent);

			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRefreshHoverItem(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive(component))
	{
		UNITID idHover = UIGetHoverUnit();
		if (idHover != INVALID_ID)
		{
			UISetHoverTextItem(component, NULL);
			UISendHoverMessage(TRUE);
			return UIMSG_RET_HANDLED;
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInventoryOnPostActivate(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if( AppIsHellgate() )
	{
		UIStopAllSkillRequests();
		c_PlayerClearAllMovement(AppGetCltGame());

		UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
	}
	else
	{
		if( UIPaneVisible( KPaneMenuCraftingRecipes ) ||
			UIPaneVisible( KPaneMenuCraftingTrainer ) )
		{
			UI_PANELEX* pPane = UICastToPanel( component );
			UIPanelSetTab( pPane, 1 );
		}
		else
		{
			UI_PANELEX* pPane = UICastToPanel( component );
			UIPanelSetTab( pPane, 0 );
		}
		UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInventoryOnCloseButtonClicked(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	// make sure the conversation gets canceled if the inventory is up for a quest reward -cmarch
	UIConversationOnCancel( component, nMessage, wParam, lParam );
	UICloseParent( component, nMessage, wParam, lParam );

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuickClosePanels(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	UIQuickClose();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Event handler to trap events where we need to close the player inspection
//----------------------------------------------------------------------------
static void sStopInspect(
	GAME* game,
	UNIT* unit,
	UNIT* unitNotUsed,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId)
{
	UIClosePlayerInspectionPanel(NULL, 0, 0, 0);
}

//----------------------------------------------------------------------------
// Initiate a player inspection (called from c_network.cpp after server cmd)
//----------------------------------------------------------------------------
void UIOpenPlayerInspectionPanel(
	GAME *pGame,
	UNITID idUnit )
{
	UIComponentActivate(UICOMP_PLAYER_INSPECTION_SCREEN, TRUE);	
	UIComponentActivate(UICOMP_PLAYER_OVERVIEW_PANEL, TRUE);	
	UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idUnit, TRUE);
	UIComponentSetFocusUnitByEnum(UICOMP_PLAYER_OVERVIEW_PANEL, idUnit, TRUE);
	UIComponentSetFocusUnitByEnum(UICOMP_CHARSHEET, idUnit, TRUE);

	// KCK: Register event handlers so we can stop inspecting this unit when bad things happen
	UNIT * pUnit = UnitGetById(pGame, idUnit);
	if (pUnit)
	{
		UnitRegisterEventHandler( pGame, pUnit, EVENT_WARPED_LEVELS, sStopInspect, &EVENT_DATA() );
		UnitRegisterEventHandler( pGame, pUnit, EVENT_UNITDIE_BEGIN, sStopInspect, &EVENT_DATA() );
		UnitRegisterEventHandler( pGame, pUnit, EVENT_ON_FREED, sStopInspect, &EVENT_DATA() );
		UnitRegisterEventHandler( pGame, pUnit, EVENT_ENTER_LIMBO, sStopInspect, &EVENT_DATA() );
	}
}

//----------------------------------------------------------------------------
// Finish a player inspection (called on closing panel or by unit events)
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIClosePlayerInspectionPanel(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	UIQuickClose();

	GAME * pGame = AppGetCltGame();
	UI_COMPONENT *pComponent = UIComponentGetByEnum( UICOMP_PAPERDOLL );
	UNIT * pUnit = UIComponentGetFocusUnit(pComponent);
	if (pGame && pUnit)
	{
		UnitUnregisterEventHandler( pGame, pUnit, EVENT_WARPED_LEVELS, sStopInspect );
		UnitUnregisterEventHandler( pGame, pUnit, EVENT_UNITDIE_BEGIN, sStopInspect );
		UnitUnregisterEventHandler( pGame, pUnit, EVENT_ON_FREED, sStopInspect );
		UnitUnregisterEventHandler( pGame, pUnit, EVENT_ENTER_LIMBO, sStopInspect );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Finish a player inspection (called on closing panel or by unit events)
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInspectionPanelPostInvisible(
	UI_COMPONENT *component,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	// KCK: Make sure we set focus back to player's control unit
	UNIT * pPlayer = UIGetControlUnit();
	UNITID idPlayer = UnitGetId(pPlayer);
	UIComponentSetFocusUnitByEnum(UICOMP_PAPERDOLL, idPlayer, TRUE);
	UIComponentSetFocusUnitByEnum(UICOMP_CHARSHEET, idPlayer, TRUE);

	return UISetChatActive(component, nMessage, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterOptionsPanelOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutoPartyButtonOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	GAME *game = AppGetCltGame();
	ASSERT_RETVAL(game && IS_CLIENT(game), UIMSG_RET_NOT_HANDLED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, s_PlayerIsAutoPartyEnabled(pPlayer));

	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutoPartyButtonClicked(
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

	GAME *game = AppGetCltGame();
	ASSERT_RETVAL(game && IS_CLIENT(game), UIMSG_RET_NOT_HANDLED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, UIMSG_RET_NOT_HANDLED);
	BOOL bWasEnabled = s_PlayerIsAutoPartyEnabled(pPlayer);
	c_PlayerToggleAutoParty( pPlayer );

	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, !bWasEnabled);
	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInspectableButtonOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	GAME *game = AppGetCltGame();
	ASSERT_RETVAL(game && IS_CLIENT(game), UIMSG_RET_NOT_HANDLED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, UnitGetStat( pPlayer, STATS_CHAR_OPTION_ALLOW_INSPECTION ));

	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIInspectableButtonClicked(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *game = AppGetCltGame();
	ASSERT_RETVAL(game && IS_CLIENT(game), UIMSG_RET_NOT_HANDLED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, UIMSG_RET_NOT_HANDLED);

	bool	bWasEnabled = (UnitGetStat( pPlayer, STATS_CHAR_OPTION_ALLOW_INSPECTION ))? TRUE : FALSE;

	// Set stat locally and send to server
	UnitSetStat( pPlayer, STATS_CHAR_OPTION_ALLOW_INSPECTION, !bWasEnabled );
	MSG_CCMD_SET_PLAYER_OPTIONS  tMessage;
	tMessage.nAllowInspection = !bWasEnabled;
	tMessage.nHideHelmet = UnitGetStat( pPlayer, STATS_CHAR_OPTION_HIDE_HELMET );
	c_SendMessage(CCMD_SET_PLAYER_OPTIONS, &tMessage);


	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, !bWasEnabled);
	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHideHelmetButtonOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	GAME *game = AppGetCltGame();
	ASSERT_RETVAL(game && IS_CLIENT(game), UIMSG_RET_NOT_HANDLED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, UnitGetStat( pPlayer, STATS_CHAR_OPTION_HIDE_HELMET ));

	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIHideHelmetButtonClicked(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *game = AppGetCltGame();
	ASSERT_RETVAL(game && IS_CLIENT(game), UIMSG_RET_NOT_HANDLED);
	UNIT *pPlayer = GameGetControlUnit(game);
	ASSERT_RETVAL( pPlayer, UIMSG_RET_NOT_HANDLED);

	bool	bWasEnabled = (UnitGetStat( pPlayer, STATS_CHAR_OPTION_HIDE_HELMET ))? TRUE : FALSE;

	// Set stat locally and send to server
	UnitSetStat( pPlayer, STATS_CHAR_OPTION_HIDE_HELMET, !bWasEnabled );
	MSG_CCMD_SET_PLAYER_OPTIONS  tMessage;
	tMessage.nAllowInspection = UnitGetStat( pPlayer, STATS_CHAR_OPTION_ALLOW_INSPECTION );
	tMessage.nHideHelmet = !bWasEnabled;
	c_SendMessage(CCMD_SET_PLAYER_OPTIONS, &tMessage);

	UI_BUTTONEX *pButton = UICastToButton(component);
	UIButtonSetDown(pButton, !bWasEnabled);
	return UIComponentHandleUIMessage( component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSheetOnPostVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UNIT* pUnit = UIGetControlUnit();
	UI_COMPONENT* pHCIcon = UIComponentFindChildByName(component, "hardcore char icon");
	if(pHCIcon && pUnit)
		UIComponentSetVisible(pHCIcon, PlayerIsHardcore(pUnit));

	UI_COMPONENT* pRPIcon = UIComponentFindChildByName(component, "roleplay char icon");
	if(pRPIcon && pUnit)
		UIComponentSetVisible(pRPIcon, PlayerIsLeague(pUnit));

	UI_COMPONENT* pEIcon = UIComponentFindChildByName(component, "elite char icon");
	if(pEIcon && pUnit)
		UIComponentSetVisible(pEIcon, PlayerIsElite(pUnit));

	UI_COMPONENT *pInspectableButton = UIComponentGetById(UIComponentGetIdByName("inspectable btn"));
	if (pInspectableButton)
		UIInspectableButtonOnPostActivate(pInspectableButton, msg, wParam, lParam);

	UI_COMPONENT *pHideHelmetButton = UIComponentGetById(UIComponentGetIdByName("hide helmet btn"));
	if (pHideHelmetButton)
		UIHideHelmetButtonOnPostActivate(pHideHelmetButton, msg, wParam, lParam);

	UI_MSG_RETVAL UISetChatActive(UI_COMPONENT *, int, DWORD, DWORD);
	return UISetChatActive(component, msg, wParam, lParam);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBackpackCancel(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	c_BackpackClose();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITitleComboBoxOnDropDown(
	UI_COMPONENT* pComponent,
	int nMsg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	UI_BUTTONEX* pButton = UICastToButton(pComponent);
	if (UIButtonGetDown(pButton))
	{
		UI_COMBOBOX *pCombo = UICastToComboBox(pComponent->m_pParent);

		UITextBoxClear(pCombo->m_pListBox);

		UIListBoxAddString(pCombo->m_pListBox, L"", (QWORD)INVALID_LINK);
		int nTitleStringIDs[255];
		WCHAR uszTitle[255];

		int nCurrentTitleID = x_PlayerGetTitleStringID(UIGetControlUnit());
		int nNumTitles = c_PlayerListAvailableTitles(
			UIGetControlUnit(),
			nTitleStringIDs,
			arrsize(nTitleStringIDs));

		for (int i = 0; i < nNumTitles; i++)
		{
			PStrCopy(uszTitle, StringTableGetStringByIndex(nTitleStringIDs[i]), arrsize(uszTitle));
			PStrReplaceToken(uszTitle, arrsize(uszTitle), L"[PLAYERNAME]", L"");
			UIListBoxAddString(pCombo->m_pListBox, uszTitle, (QWORD)nTitleStringIDs[i]);
			if (nTitleStringIDs[i] == nCurrentTitleID)
				UIListBoxSetSelectedIndex(pCombo->m_pListBox, i+1, FALSE);
		}
	}


	return UIWindowshadeButtonOnClick(pComponent, nMsg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITitleComboBoxOnPostCreate(
	UI_COMPONENT* pComponent,
	int nMsg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_COMBOBOX *pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pButton, UIMSG_RET_NOT_HANDLED);

	UIAddMsgHandler(pCombo->m_pButton, UIMSG_LBUTTONCLICK, UITitleComboBoxOnDropDown, INVALID_LINK);
	UIAddMsgHandler(pCombo->m_pButton->m_pNextSibling, UIMSG_ANIMATE, UIMsgPassToParent, INVALID_LINK);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITitleComboBoxOnSelect(
	UI_COMPONENT* pComponent,
	int nMsg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_COMBOBOX *pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pButton, UIMSG_RET_NOT_HANDLED);

	int nTitleStringID = (int)UIListBoxGetSelectedData(pCombo->m_pListBox);

	c_PlayerSetTitle(UIGetControlUnit(), nTitleStringID);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITitleComboBoxSetToCurrent(
	UI_COMPONENT* pComponent,
	int nMsg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_COMBOBOX *pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pLabel, UIMSG_RET_NOT_HANDLED);

	WCHAR uszTitle[255] = L"";

	int nCurrentTitleID = x_PlayerGetTitleStringID(UIComponentGetFocusUnit(pComponent));
	if (nCurrentTitleID != INVALID_LINK)
	{
		PStrCopy(uszTitle, StringTableGetStringByIndex(nCurrentTitleID), arrsize(uszTitle));
		PStrReplaceToken(uszTitle, arrsize(uszTitle), L"[PLAYERNAME]", L"");
	}
	UILabelSetText(pCombo->m_pLabel, uszTitle);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharLevelOnSetFocusStat(
	UI_COMPONENT* pComponent,
	int nMsg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UNIT *pPlayer = UIComponentGetFocusUnit(pComponent);
	ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

	int nRank = UnitGetStat(pPlayer, STATS_PLAYER_RANK);
	if (!PStrICmp(pComponent->m_szName, "char level rank"))
	{
		UIComponentSetVisible(pComponent, nRank > 0);
	}
	else
	{
		UIComponentSetVisible(pComponent, nRank == 0);
	}

	UI_MSG_RETVAL UIStatDisplOnPaint(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
	return UIStatDisplOnPaint(pComponent, nMsg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIToggleLevelAndRankBars(
	UNIT * pControlUnit)
{
	if(!pControlUnit)
		return;

	int nLevel = UnitGetExperienceLevel(pControlUnit);
	int nMaxLevel = UnitGetMaxLevel(pControlUnit);
	if(nLevel >= nMaxLevel)
	{
		UIComponentActivate(UICOMP_EXPERIENCEBAR, FALSE);
		UIComponentActivate(UICOMP_RANKBAR, TRUE);
	}
	else
	{
		UIComponentActivate(UICOMP_EXPERIENCEBAR, TRUE);
		UIComponentActivate(UICOMP_RANKBAR, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDashboardSetFocusUnit( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam)
{
	UIComponentSetFocusUnit(component, UNITID(wParam));
	sUIToggleLevelAndRankBars(UIComponentGetFocusUnit(component));
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDashboardOnLevelChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam)
{
	sUIToggleLevelAndRankBars(UIComponentGetFocusUnit(component));
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIDashboardOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam)
{
	sUIToggleLevelAndRankBars(UIComponentGetFocusUnit(component));
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIToggleCharacterSheetRankDisplay(
	UNIT * pControlUnit)
{
	if(!pControlUnit)
		return;

	int nRankExperience = UnitGetRankExperienceValue(pControlUnit);

	int nLevel = UnitGetExperienceLevel(pControlUnit);
	int nMaxLevel = UnitGetMaxLevel(pControlUnit);
	if(nLevel >= nMaxLevel || nRankExperience > 0)
	{
		UIComponentActivate(UICOMP_RANK_XP_DISPLAY, TRUE);
	}
	else
	{
		UIComponentActivate(UICOMP_RANK_XP_DISPLAY, FALSE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSheetSetFocusUnit( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam)
{
	UIComponentSetFocusUnit(component, UNITID(wParam));
	sUIToggleCharacterSheetRankDisplay(UIComponentGetFocusUnit(component));
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSheetOnLevelChange( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam)
{
	sUIToggleCharacterSheetRankDisplay(UIComponentGetFocusUnit(component));
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICharacterSheetOnPostActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam)
{
	sUIToggleCharacterSheetRankDisplay(UIComponentGetFocusUnit(component));
	return UIMSG_RET_HANDLED;
}


static void sUICapMoney(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);

	// clean up money string (leading zeros for instance)
	int nMoney = c_GetMoneyValueInComponent( pComponent );

	// cap the money value 
	cCurrency playerCurrency = UnitGetCurrency( UIGetControlUnit() );
	int nMoneyMax = playerCurrency.GetValue( KCURRENCY_VALUE_INGAME );
	nMoney = PIN( nMoney, 0, nMoneyMax );


	if (nMoney > 0)
	{
		c_SetMoneyValueInComponent( pComponent, nMoney );
	}
	else
	{
		UILabelSetText(pComponent, L"");
	}
}

UI_MSG_RETVAL UIMoneyOnChar(
							UI_COMPONENT *pComponent,
							int nMessage,
							DWORD wParam,
							DWORD lParam)
{
	if (UIComponentGetActive( pComponent ))
	{
		WCHAR ucCharacter = (WCHAR)wParam;
		if (UIEditCtrlOnKeyChar( pComponent, nMessage, wParam, lParam ) != UIMSG_RET_NOT_HANDLED)
		{
			if (ucCharacter != VK_RETURN)
			{
				sUICapMoney(pComponent);
			}

			return UIMSG_RET_HANDLED_END_PROCESS;  // input used
		}
	}
	return UIMSG_RET_NOT_HANDLED;  // input not used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISharedStashGameModeComboOnPostCreate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	UITextBoxClear(pCombo->m_pListBox);
	UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("Normal"));
	UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("state_icon_tooltip_hardcore"));
	UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("state_icon_tooltip_elite"));
	UIListBoxAddString(pCombo->m_pListBox, StringTableGetStringByKey("Hardcore_Elite"));

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISharedStashGameModeComboOnPostActivate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	UNIT * pPlayer = UIComponentGetFocusUnit(pComponent);
	ASSERT_RETVAL(pPlayer, UIMSG_RET_NOT_HANDLED);

	DWORD nGameVariant =GameVariantFlagsGetStaticUnit(pPlayer);
	ASSERT_RETVAL(nGameVariant >= 0, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(nGameVariant < pCombo->m_pListBox->m_LinesList.Count(), UIMSG_RET_NOT_HANDLED);

	UIListBoxSetSelectedIndex(pCombo->m_pListBox, nGameVariant);
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);

	UISharedStashGameModeComboOnChange(pComponent, nMessage, wParam, lParam);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISharedStashGameModeComboOnChange(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_COMBOBOX * pCombo = UICastToComboBox(pComponent);
	ASSERT_RETVAL(pCombo->m_pListBox, UIMSG_RET_NOT_HANDLED);

	DWORD nGameVariant = UIListBoxGetSelectedIndex(pCombo->m_pListBox);

	UI_COMPONENT * pStashPanel = UIComponentGetByEnum(UICOMP_STASH_PANEL);
	ASSERT_RETVAL(pStashPanel, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT * pSharedStashGrid = UIComponentFindChildByName(pStashPanel, "shared stash grid");
	ASSERT_RETVAL(pSharedStashGrid, UIMSG_RET_NOT_HANDLED);

	UI_INVGRID * pSharedStashInvGrid = UICastToInvGrid(pSharedStashGrid);
	pSharedStashInvGrid->m_ScrollPos.m_fY = pSharedStashInvGrid->m_fCellHeight * (float)SHARED_STASH_SECTION_HEIGHT * (float)nGameVariant;
	UIComponentHandleUIMessage(pSharedStashInvGrid, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

#endif //!SERVER_VERSION
