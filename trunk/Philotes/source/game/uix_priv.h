//----------------------------------------------------------------------------
// uix_priv.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_PRIV_H_
#define _UIX_PRIV_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _PTIME_H_
#include "ptime.h"
#endif

#ifndef __E_UI__
#include "e_ui.h"
#endif

#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef	_LIST_H_
#include "list.h"
#endif

#include "../data_common/excel/ui_component_hdr.h"

//----------------------------------------------------------------------------
// MACROS

#define UI_MAKE_COLOR(alpha, color)			((((DWORD)(alpha) & 0xff) << 24) + (((DWORD)(color)) & 0xffffff))
#define UI_GET_ALPHA(color)					(BYTE)((color) >> 24)
#define UI_GET_COLOR(color)					((color) & 0xffffff)
#define UI_COMBINE_ALPHA(alpha, color)		(((((alpha) & 0xff) * UI_GET_ALPHA(color) / 255) << 24) + ((color) & 0xffffff))

#define XML_LOADFLOAT(key, var, def)		xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (float)PStrToFloat(xml.GetChildData()); } else if (!component->m_bReference) { var = def; }
#define XML_LOADFLOAT_NC(key, var, def)		xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (float)PStrToFloat(xml.GetChildData()); } else { var = def; }

#define XML_LOADINT(key, var, def)			xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (int)PStrToInt(xml.GetChildData()); } else if (!component->m_bReference) { var = (int)(def); }
#define XML_LOADBYTE(key, var, def)			xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (BYTE)PStrToInt(xml.GetChildData()); } else if (!component->m_bReference) { var = (BYTE)(def); }
#define XML_LOAD(key, var, type, def)		xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (type)PStrToInt(xml.GetChildData()); } else if (!component->m_bReference) { var = (type)(def); }
#define XML_LOADSTRING(key, var, def, size)	xml.ResetChildPos(); if (xml.FindChildElem(key)) { PStrCopy(var, xml.GetChildData(), size); var[size-1] = 0; } else if (!component->m_bReference) { if (def) { PStrCopy(var, def, size); var[size-1] = 0;} else { var[0] = 0; } }
#define XML_LOADSTRING_NC(key, var, def, size)	xml.ResetChildPos(); if (xml.FindChildElem(key)) { PStrCopy(var, xml.GetChildData(), size); var[size-1] = 0; } else { if (def) { PStrCopy(var, def, size); var[size-1] = 0;} else { var[0] = 0; } }
#define XML_LOADBOOL(key, var, def)			xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (BOOL)PStrToInt(xml.GetChildData()); } else if (!component->m_bReference) { var = def; }
#define XML_LOADEXCELILINK(key, var, idx)	xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = ExcelGetLineByStringIndex(NULL, idx, xml.GetChildData()); } else if (!component->m_bReference) { var = INVALID_LINK; }
#define XML_LOADFRAME(key, tex, var, def)   { char frm[256]=""; XML_LOADSTRING(key, frm, def, 256); ASSERT(tex); if (tex && frm[0] != 0) { var = (UI_TEXTURE_FRAME*)StrDictionaryFind(tex->m_pFrames, frm); } else if (!component->m_bReference) { var = NULL; }}
#define XML_LOADENUM(key, var, tbl, def)	UIXMLLoadEnum(xml, key, (int&)var, tbl, (component->m_bReference ? var : def))

//REGION CHECK VERSIONS - check for sku, region, or language
// NOTE: BSP - the base load macros could be replaced with these to region check everything
//    but it's a little to far-reaching for right now
#define XML_LOADSTRING_RC(key, var, def, size)	xml.ResetChildPos(); if (xml.FindChildElem(key) && UIXmlCheckTagForLocation(xml)) { PStrCopy(var, xml.GetChildData(), size); var[size-1] = 0; } else if (!component->m_bReference) { if (def) { PStrCopy(var, def, size); var[size-1] = 0;} else { var[0] = 0; } }
#define XML_LOADINT_RC(key, var, def)			xml.ResetChildPos(); if (xml.FindChildElem(key) && UIXmlCheckTagForLocation(xml)) { var = (int)PStrToInt(xml.GetChildData()); } else if (!component->m_bReference) { var = (int)(def); }
#define XML_LOADFLOAT_RC(key, var, def)			xml.ResetChildPos(); if (xml.FindChildElem(key) && UIXmlCheckTagForLocation(xml)) { var = (float)PStrToFloat(xml.GetChildData()); } else if (!component->m_bReference) { var = def; }
#define XML_LOADBOOL_RC(key, var, def)			xml.ResetChildPos(); if (xml.FindChildElem(key) && UIXmlCheckTagForLocation(xml)) { var = (BOOL)PStrToInt(xml.GetChildData()); } else if (!component->m_bReference) { var = def; }

#define XML_GETBOOLATTRIB(key, att, var, def)	xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (xml.HasChildAttrib(att) ? (BOOL)PStrToInt(xml.GetChildAttrib(att)) : def); }
#define XML_GETINTATTRIB(key, att, var, def)	xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (xml.HasChildAttrib(att) ? PStrToInt(xml.GetChildAttrib(att)) : def); }
#define XML_GETFLOATATTRIB(key, att, var, def)	xml.ResetChildPos(); if (xml.FindChildElem(key)) { var = (xml.HasChildAttrib(att) ? (float)PStrToFloat(xml.GetChildAttrib(att)) : def); }

#define XML_COMP_LOADFLOAT_REL(key, cvar, ratio) if (component->m_pPrevSibling) { xml.ResetChildPos(); if (xml.FindChildElem(key)) { float fRelVal = (float)PStrToFloat(xml.GetChildData());component->cvar = (component->m_pPrevSibling->cvar / tUIXml.ratio) + fRelVal; } }

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------
#define UI_DEFAULT_WIDTH				1600
#define UI_DEFAULT_HEIGHT				1200

#define MAX_UI_TEXTURES					800
#define ICON_BASE_WIDTH					32
#define ICON_BASE_TEXTURE_WIDTH			32

#define UI_HILITE_GREEN					0xff00ff00
#define UI_HILITE_GRAY					0xff1f1f1f
#define UI_ITEM_GRAY					0xff9e9e9e

//----------------------------------------------------------------------------
// Forwards
//----------------------------------------------------------------------------
struct UI_COMPONENT;
struct UIX_TEXTURE;
class CMarkup;
struct STR_DICT;
struct STAT_MSG_HANDLER;
struct UI_CURSOR;
struct CEVENT_DATA;
enum GENDER;
struct NPC_DIALOG_SPEC;

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern BOOL		bDropShadowsEnabled;
extern STR_DICT pOrientationEnumTbl[];
extern STR_DICT pAlignEnumTbl[12];
extern STR_DICT pRenderSectionEnumTbl[];

#define UIX_GRAPHIC_ENUM
enum 
{
	UIMSG_NONE = -1,
	#include "uix_messages.h"
	NUM_UI_MESSAGES
};

enum
{
	UISTATE_INACTIVE,
	UISTATE_ACTIVATING,
	UISTATE_ACTIVE,
	UISTATE_INACTIVATING,
};

#undef  UIX_GRAPHIC_ENUM

enum
{
	RENDERSECTION_WORLD,
	RENDERSECTION_BOTTOM,
	RENDERSECTION_DYNAMIC_BOTTOM,
	RENDERSECTION_PREPMODELS,
	RENDERSECTION_MASKS,
	RENDERSECTION_MASKEDMODELS,
	RENDERSECTION_MODELS,
	RENDERSECTION_ITEMTEXT,
	RENDERSECTION_TOOLTIPS,
	RENDERSECTION_MENUS,
	RENDERSECTION_MENU_DIALOG,
	REDNERSECTION_PREP_DIALOG_MODELS,
	RENDERSECTION_DIALOG_MASKS,
	RENDERSECTION_DIALOG_MASKEDMODELS,
	RENDERSECTION_DIALOG_MODELS_OVERLAY,
	RENDERSECTION_LOADING_SCREEN,
	RENDERSECTION_DIALOG_TOP,
	RENDERSECTION_CURSOR_CONTENTS,
	RENDERSECTION_CURSOR,
	RENDERSECTION_DEBUG,
	NUM_RENDER_SECTIONS
};

static const BOOL scbPrepareModelRenderSection[] = 
{
	FALSE, //	RENDERSECTION_WORLD
	FALSE, //	RENDERSECTION_BOTTOM
	FALSE, //	RENDERSECTION_DYNAMIC_BOTTOM
	TRUE,  //	RENDERSECTION_PREPMODELS
	FALSE, //	RENDERSECTION_MASKS
	FALSE, //	RENDERSECTION_MASKEDMODELS
	TRUE,  //	RENDERSECTION_MODELS
	FALSE, //	RENDERSECTION_ITEMTEXT
	FALSE, //	RENDERSECTION_TOOLTIPS
	FALSE, //	RENDERSECTION_MENUS
	FALSE, //	RENDERSECTION_MENU_DIALOG
	TRUE,  //	REDNERSECTION_PREP_DIALOG_MODELS
	FALSE, //	RENDERSECTION_DIALOG_MASKS
	FALSE, //	RENDERSECTION_DIALOG_MASKEDMODELS
	FALSE, //	RENDERSECTION_DIALOG_MODELS_OVERLAY
	FALSE, //	RENDERSECTION_LOADING_SCREEN
	FALSE, //	RENDERSECTION_DIALOG_TOP
	FALSE, //	RENDERSECTION_CURSOR_CONTENTS
	TRUE,  //	RENDERSECTION_CURSOR
	FALSE, //	RENDERSECTION_DEBUG

};

enum
{
	UIORIENT_LEFT,
	UIORIENT_RIGHT,
	UIORIENT_BOTTOM,
	UIORIENT_TOP,
};

enum LOAD_SCREEN_STATE
{
	LOAD_SCREEN_DOWN = 0,
	LOAD_SCREEN_UP,
	LOAD_SCREEN_FORCE_UP,
	LOAD_SCREEN_CAN_HIDE,
};

enum
{
	UIHOVERSTATE_NONE,
	UIHOVERSTATE_SHORT,
	UIHOVERSTATE_LONG,
};

enum CURSOR_STATE
{
	UICURSOR_STATE_INVALID = -1,
	UICURSOR_STATE_POINTER,
	UICURSOR_STATE_ITEM,
	UICURSOR_STATE_OVERITEM,
	UICURSOR_STATE_SKILLDRAG,
	UICURSOR_STATE_WAIT,
	UICURSOR_STATE_IDENTIFY,
	UICURSOR_STATE_SKILL_PICK_ITEM,
	UICURSOR_STATE_DISMANTLE,
	UICURSOR_STATE_ITEM_PICK_ITEM,
	UICURSOR_STATE_INITIATE_ATTACK,
	NUM_UICURSOR_STATES
};

enum 
{
	UI_ANIM_CATEGORY_INVALID = -1,
	UI_ANIM_CATEGORY_MAIN_PANELS,
	UI_ANIM_CATEGORY_MAIN_PANELS_UA,
	UI_ANIM_CATEGORY_LEFT_PANEL,
	UI_ANIM_CATEGORY_MIDDLE_PANEL,
	UI_ANIM_CATEGORY_RIGHT_PANEL,
	UI_ANIM_CATEGORY_QUICK_CLOSE,		// closes with the ESC key
	UI_ANIM_CATEGORY_CLOSE_ALL,			// close when the player dies (among other things)
	UI_ANIM_CATEGORY_TOOLTIPS,
	UI_ANIM_CATEGORY_CHAT_FADE,
	UI_ANIM_CATEGORY_MAP_FADE,

	NUM_UI_ANIM_CATEGORIES
};

//----------------------------------------------------------------------------
// EXPORTED STRUCTURES
//----------------------------------------------------------------------------
enum BLINK_STATE
{
	UIBLINKSTATE_NOT_BLINKING = 0,
	UIBLINKSTATE_ON,
	UIBLINKSTATE_OFF,
};

struct UI_BLINKDATA
{
	BLINK_STATE				m_eBlinkState;
	TIME					m_timeLastBlinkChange;
	TIME					m_timeBlinkStarted;
	int						m_nBlinkDuration;
	int						m_nBlinkPeriod;
	DWORD					m_dwBlinkColor;
	DWORD					m_dwBlinkAnimTicket;
};

enum UI_MSG_RETVAL
{
	UIMSG_RET_NOT_HANDLED,
	UIMSG_RET_HANDLED,
	UIMSG_RET_HANDLED_END_PROCESS,
	UIMSG_RET_END_PROCESS,
};

inline BOOL ResultStopsProcessing(UI_MSG_RETVAL eResult)
{
	return eResult == UIMSG_RET_END_PROCESS || eResult == UIMSG_RET_HANDLED_END_PROCESS;
}

inline BOOL ResultIsHandled(UI_MSG_RETVAL eResult)
{
	return eResult == UIMSG_RET_HANDLED || eResult == UIMSG_RET_HANDLED_END_PROCESS;
}

typedef UI_MSG_RETVAL (*PFN_UIMSG_HANDLER)(struct UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);

struct UI_SOUND_HANDLER
{
	UI_COMPONENT	*m_pComponent;
	int				m_nSoundId;
	int				m_nPlayingSoundId;
	BOOL			m_bStopSound;
};

struct UI_MSGHANDLER
{
	UI_COMPONENT			*m_pComponent;
	int						m_nMsg;
	PFN_UIMSG_HANDLER		m_fpMsgHandler;
	int						m_nStatID;
};

struct UI_ANIM_INFO
{
	TIME					m_tiAnimStart;
	DWORD					m_dwAnimDuration;
	DWORD					m_dwAnimTicket;

	UI_ANIM_INFO(void)		{ m_tiAnimStart = 0; m_dwAnimDuration = 0; m_dwAnimTicket = 0; }
};

struct UI_FRAME_ANIM_INFO
{
	int					m_nNumAnimFrames;
	UI_TEXTURE_FRAME**	m_pAnimFrames;
	DWORD				m_dwFrameAnimMSDelay;
	int					m_nCurrentFrame;
	DWORD				m_dwAnimTicket;
};

struct UI_DELAYED_ACTIVATE_INFO
{
	UI_COMPONENT*	pComponent;
	DWORD			dwTicket;
};

//----------------------------------------------------------------------------
typedef void (* PFN_DIALOG_CALLBACK)( void *pCallbackData, DWORD dwCallbackData );

void DialogCallbackInit( 
	struct DIALOG_CALLBACK &tDialogCallback);

//----------------------------------------------------------------------------
struct DIALOG_CALLBACK
{
	PFN_DIALOG_CALLBACK pfnCallback;
	void *pCallbackData;
	DWORD dwCallbackData;
	
	DIALOG_CALLBACK::DIALOG_CALLBACK( void )
	{ 
		DialogCallbackInit( *this );
	}
	
};

#define MAX_UI_COMPONENT_NAME_LENGTH (128)
#define UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH (1024)

struct UI_COMP_LATE_POINTER;
struct ANIM_RELATIONSHIP_NODE;

// basic ui component, organized into a tree
//----------------------------------------------------------------------------
struct UI_COMPONENT
{
	char					m_szName[MAX_UI_COMPONENT_NAME_LENGTH];
	int						m_idComponent;
	UI_COMPONENT*			m_pNextSibling;
	UI_COMPONENT*			m_pPrevSibling;
	UI_COMPONENT*			m_pParent;
	UI_COMPONENT*			m_pFirstChild;
	UI_COMPONENT*			m_pLastChild;

	int						m_eComponentType;
	int						m_eState;
	BOOL					m_bVisible;
	BOOL					m_bIndependentActivate;
	BOOL					m_bUserActive;

	UI_POSITION				m_Position;
	float					m_fZDelta;
	float					m_fWidth;
	float					m_fHeight;
	float					m_fWidthParentRel;
	float					m_fHeightParentRel;
	UI_POSITION				m_ActivePos;
	UI_POSITION				m_InactivePos;
	UI_POSITION				m_SizeMin;
	UI_POSITION				m_SizeMax;

	UI_POSITION				m_ScrollPos;
	UI_POSITION				m_ScrollMin;
	UI_POSITION				m_ScrollMax;
	float					m_fWheelScrollIncrement;
	UI_POSITION				m_posFrameScroll;
	float					m_fFrameScrollSpeed;
	BOOL					m_bScrollFrameReverse;
	float					m_fScrollHorizMax;
	float					m_fScrollVertMax;
	BOOL					m_bScrollsWithParent;

	float					m_fXmlWidthRatio;
	float					m_fXmlHeightRatio;
	BOOL					m_bNeedsRepaintOnVisible;		// if this is true a paint message will be sent to the component when it becomes visible
	BOOL					m_bNoScale;
	BOOL					m_bNoScaleFont;
	BOOL					m_bStretch;
	BOOL					m_bDynamic;
	BOOL					m_bIgnoresMouse;
	int						m_nAngle;
	UI_RECT					m_rectClip;
	BOOL					m_bGrayInGhost;

	struct UIX_TEXTURE*			m_pTexture;
	struct UI_TEXTURE_FRAME*	m_pFrame;
	UI_FRAME_ANIM_INFO*			m_pFrameAnimInfo;			// one of these has to go
	DWORD						m_dwFramesAnimTicket;		// one of these has to go

	struct UIX_TEXTURE_FONT*	m_pFont;

	int						m_nFontSize;
	DWORD					m_dwColor;
	BOOL					m_bTileFrame;
	float					m_fTilePaddingX;
	float					m_fTilePaddingY;

	UNITID					m_idFocusUnit;
	BOOL					m_bHasOwnFocusUnit;	// don't search the component's parents for a focus unit
	DWORD_PTR				m_dwParam;
	DWORD_PTR				m_dwParam2;
	DWORD_PTR				m_dwParam3;
	DWORD_PTR				m_dwData;			// custom user data field
	QWORD					m_qwData;

	void *					m_pData;

	UI_BLINKDATA			*m_pBlinkData;

	PCODE					m_codeStatToVisibleOn;
	//PCODE					m_codeVisibleWhen;

	WCHAR*					m_szTooltipText;
	BOOL					m_bHoverTextWithMouseDown;

	// Tooltip info
	int						m_nTooltipLine;
	int						m_nTooltipOrder;
	int						m_nTooltipJustify;
	float					m_fToolTipPaddingX;
	float					m_fToolTipPaddingY;
	int						m_nTooltipCrossesLines;
	BOOL					m_bTooltipNoSpace;
	float					m_fToolTipXOffset;
	float					m_fToolTipYOffset;

	// animation info
	BOOL					m_bFadesIn;
	BOOL					m_bFadesOut;
	float					m_fFading;
	UI_POSITION				m_AnimStartPos;
	UI_POSITION				m_AnimEndPos;
	DWORD					m_dwAnimDuration;

	BOOL					m_bAnimatesWhenPaused;
	UI_ANIM_INFO			m_tMainAnimInfo;
	UI_ANIM_INFO			m_tThrobAnimInfo;

	int						m_nRenderSection;		// A render order for the purposes of UI model rendering.  0 will be before the models are rendered, 1 is the models, and 2 will be afterward.
	int						m_nRenderSectionOffset; // keep any up or down offsets to give to reference children if required
	int						m_nModelID;
	int						m_nAppearanceDefID;

	struct UI_GFXELEMENT*			m_pGfxElementFirst;
	struct UI_GFXELEMENT*			m_pGfxElementLast;

	BOOL					m_bReference;						// If this is true the component was based on another component.  Usually used by the XML loading.
	PList<UI_MSGHANDLER>	m_listMsgHandlers;

	DIALOG_CALLBACK			m_tCallbackOK;
	DIALOG_CALLBACK			m_tCallbackCancel;	

	UI_ALIGN				m_nResizeAnchor;

	UI_ALIGN				m_nAnchorParent;
	float					m_fAnchorXoffs;
	float					m_fAnchorYoffs;

	ANIM_RELATIONSHIP_NODE	*m_pAnimRelationships;
	UI_COMPONENT			*m_pReplacedComponent;			// the one component replaced as a result of an ANIM_REL_REPLACES
			
	// Public methods:
	UI_COMPONENT(void);
};

struct STARTGAME_PARAMS
{
	BOOL m_bSinglePlayer;
	BOOL m_bNewPlayer;
	GENDER m_eGender;
	int	m_nCharacterClass;
	int	m_nCharacterRace;
	WCHAR m_szPlayerName[256];
	DWORD m_dwGemActivation;
	int m_nDifficulty;
	int m_nPlayerSlot;
	int m_nCharacterButton;
	DWORD dwNewPlayerFlags;		// see NEW_PLAYER_FLAGS

	UNITID		m_idCharCreateUnit;
};
extern STARTGAME_PARAMS	g_UI_StartgameParams;
typedef void (*STARTGAME_CALLBACK)(BOOL bStartGame, const STARTGAME_PARAMS& tStartgameData);

struct UNIT;
struct UI_STATE_MSG_DATA 
{
	UNIT*	m_pSource;
	int		m_nState; 
	WORD 	m_wStateIndex;
	int 	m_nDuration;
	int 	m_nSkill;
	BOOL	m_bClearing;
};

struct UI_INV_ITEM_GFX_ADJUSTMENT
{
	float fScale;
	float fXOffset;
	float fYOffset;

	UI_INV_ITEM_GFX_ADJUSTMENT(void)	{ fScale = 0.0f; fXOffset = 0.0f; fYOffset = 0.0f; }
};

struct UI_CHAT_BUBBLE
{
	UNITID	idUnit;
	TIME	tiStart;
	WCHAR	szText[1024];
	DWORD	dwColor;
};

struct UI_SCREEN_RATIOS
{
	float	m_fLogToScreenRatioX;
	float	m_fLogToScreenRatioY;
	float	m_fScreenToLogRatioX;
	float	m_fScreenToLogRatioY;
};

// temporary, used for xml loading
struct UI_XML
{
	int		m_nXmlWidthBasis;
	int		m_nXmlHeightBasis;
	float	m_fXmlWidthRatio;
	float	m_fXmlHeightRatio;
};

struct CINVENTORY_GLOBALS
{
	enum INVENTORY_STYLE eStyle;
	int nOfferDefinition;
	int nNumRewardTakes;
};

// TODO: move more flags into here
enum UI_FLAG 
{
	UI_FLAG_SHOW_ACC_BRACKETS,
	UI_FLAG_ALT_TARGET_INFO,
	UI_FLAG_SHOW_TARGET_BRACKETS,
	UI_FLAG_LOADING_UI,
};

//----------------------------------------------------------------------------
// cmd_menus.txt table entries
//----------------------------------------------------------------------------
struct CMDMENU_DATA
{
	char	szCmdMenu[DEFAULT_INDEX_SIZE];
	char	szChatCommand[DEFAULT_INDEX_SIZE];
	BOOL	bImmediate;
};

typedef void (UI_RELOAD_CALLBACK)(void);	// CHB 2007.04.03

// global ui storage
struct UI
{
	BOOL					m_bWidescreen;
	BOOL					m_bRequestingReload;
	UI_RELOAD_CALLBACK *	m_pfnReloadCallback;	// CHB 2007.04.03
	int						m_nCurrentComponentId;
	BOOL					m_bShowItemNames;
	BOOL					m_bAlwaysShowItemNames;
	DWORD					m_dwFlags;  // see UI_FLAG
	UI_COMPONENT*			m_pCurrentMouseOverComponent;
	UI_COMPONENT*			m_pLastMouseDownComponent;
	UINT					m_nLastMouseDownMsg;
	DWORD					m_dwLastMouseDownMsgTime;

	UI_COMPONENT*			m_Components;
	UI_CURSOR*				m_Cursor;

	float					m_fUIScaler;
	int						m_nHoverDelay;
	int						m_nHoverDelayLong;
	UNITID					m_idHoverUnit;				// hover unit in inventory
	int						m_idHoverUnitSetBy;
	TIME					m_timeNextHoverCheck;		
	int						m_eHoverState;

	int						m_nInterface;				// > 0 if an "interface" screen is active (skills,inv,buy/sell,menu)

	UNITID					m_idGunMod;					// unit to mod
	UNITID					m_idTargetUnit;				// target unit in world
	UNITID					m_idAltTargetUnit;			// target unit in alt selection
	BOOL					m_bTargetInRange;
	TIME					m_timeLastValidTarget;

	struct STR_DICTIONARY*  m_pTextures;

	UI_COMPONENT*			m_pComponentsMap[NUM_UICOMP];
	int						m_idGunPanel;

	BYTE*					m_pCodeBuffer;
	int						m_nCodeSize;
	int						m_nCodeCurr;

	LOAD_SCREEN_STATE		m_eLoadScreenState;
	UI_COMPONENT*			m_pCurrentMenu;		
	int						m_idLastMenu;		
	DWORD					m_dwAutomapUpdateTicket;
	BOOL					m_bVisitSkillPageReminder;
	DWORD					m_dwStdAnimTime;

	BOOL					m_bShowTargetIndicator;
	BOOL					m_bDebugTestingToggle;

	UIX_TEXTURE*			m_pFontTexture;

	BOOL					m_bGrayout;

	BOOL					m_bIncoming;
	NPC_DIALOG_SPEC	*		m_pIncomingStore;

	BOOL					m_bNeedItemWindowRepaint;
	BOOL					m_bFullRepaintRequested;
	BOOL					m_bFontTextureResetNeeded;

	int						m_nMuteUISounds;
	BOOL					m_bIgnoreStatMessages;
	BOOL					m_bReloadDirect;

	UNITID					m_idCubeDisplayUnit;

	// these are arrays of lists that contain the message handlers that components have registered for
	PList<UI_MSGHANDLER *>	m_listMessageHandlers[NUM_UI_MESSAGES];
	PList<UI_SOUND_HANDLER> m_listSoundMessageHandlers[NUM_UI_MESSAGES];

	// Tugboat-specific
	int						m_nDialogOpenCount;
	UNITID					m_idClickedTargetUnit;		// target unit in world
	int						m_nClickedTargetSkill;
	int						m_nClickedTargetLeft;	
	UI_COMPONENT *			m_pTBAltComponent;

	class RECT_ORGANIZE_SYSTEM * m_pRectOrganizer;

	struct PICKERS *		m_pOutOfGamePickers;
	UINT					m_nFilesToLoad;				// keeps track of how many are yet to load
	BOOL					m_bPostLoadExecuted;
	int						m_nFillPakSKU;

	int						m_nDisplayWidth;			// CHB 2007.08.23
	int						m_nDisplayHeight;			// CHB 2007.08.23

	// for debug trace of UI messages
#if (ISVERSION(DEVELOPMENT))		
	BOOL					m_bMsgTraceOn;
	char					m_szMsgTraceCompNameFilter[256];
	char					m_szMsgTraceMessageNameFilter[256];
	char					m_szMsgTraceResultsFilter[256];
	int						m_idDebugEditComponent;
	BOOL					m_bActivateTrace;
#endif

	PList<struct UI_DELAYED_ACTIVATE_INFO>	m_listDelayedActivateTickets;
	PList<UI_COMPONENT *>	m_listUseCursorComponents;
	PList<UI_COMPONENT *>	m_listUseCursorComponentsWithAlt;

	PList<UI_COMPONENT *>	m_listAnimCategories[NUM_UI_ANIM_CATEGORIES];
	// this list is only available during load - it is temporary storage
	PList<ANIM_RELATIONSHIP_NODE>	m_listCompsThatNeedAnimCategories[NUM_UI_ANIM_CATEGORIES];

	PList<UI_CHAT_BUBBLE>	m_listChatBubbles;

protected:
	PList<UI_COMPONENT *>	m_listOverrideMouseMsgComponents;
	PList<UI_COMPONENT *>	m_listOverrideKeyboardMsgComponents;
	UI_COMPONENT*			m_pOverrideMouseMsgComponent;
	UI_COMPONENT*			m_pOverrideKeyboardMsgComponent;

	friend BOOL UIInit(BOOL bReloading, BOOL bFillingPak, int nFillPakSKU, BOOL bReloadDirect);
	friend void UIFree();
	friend void UISetMouseOverrideComponent(UI_COMPONENT *pComponent, BOOL bAddToTail = FALSE);
	friend void UISetKeyboardOverrideComponent(UI_COMPONENT *pComponent, BOOL bAddToTail = FALSE);
	friend UI_COMPONENT *UIGetMouseOverrideComponent();
	friend UI_COMPONENT *UIGetKeyboardOverrideComponent();
	friend void UIRemoveMouseOverrideComponent(UI_COMPONENT *pComponent);
	friend void UIRemoveKeyboardOverrideComponent(UI_COMPONENT *pComponent);
};

//----------------------------------------------------------------------------
struct LOADING_TIP_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];		// name
	int nStringKey;							// index in string table of string for name
	int nWeight;							// weight in being selected for drawing
	PCODE codeCondition;					// condition for displaying the tooltip
	BOOL bDontUseWithoutAGame;				// we can't do conditions without games
};

//----------------------------------------------------------------------------
struct LOADING_SCREEN_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];		// name
	char szPath[ MAX_PATH ];				// path to loading screen
	int nWeight;							// weight in being selected for drawing
	PCODE codeCondition;					// condition for displaying the tooltip
};

//----------------------------------------------------------------------------
// Externals II
//----------------------------------------------------------------------------
extern UI_SCREEN_RATIOS g_UI_Ratios;
extern UI				g_UI;

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
int UIComponentGetIdByName(
	const char* name);

BOOL UIIsLoaded(
	void);

void UIRender(
	void);

void UIResizeWindow(
	void);

void UIReload(
	void);

void UIRequestReload(
	void);

void UIDisplayChange(	// CHB 2007.08.23
	bool bPost);

void UISetReloadCallback(
	UI_RELOAD_CALLBACK * pCallback);

UI_MSG_RETVAL UIHandleUIMessage(
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

UI_MSG_RETVAL UIComponentHandleUIMessage(
	UI_COMPONENT* pComponent,
	int msg,
	WPARAM wParam,
	LPARAM lParam,
	BOOL bPassToChildren = FALSE);

void UIComponentSetVisible(
	UI_COMPONENT* component,
	BOOL bVisible,
	BOOL bEVERYONE = FALSE);

void UIComponentSetActive(
	UI_COMPONENT* component,
	BOOL bActive);

UI_COMPONENT* UIComponentFindChildByName(
	UI_COMPONENT* component,
	const char* name,
	BOOL bPartial = FALSE);

UI_COMPONENT* UIComponentFindParentByName(
	UI_COMPONENT* component,
	const char* name,
	BOOL bPartial = FALSE);

UI_COMPONENT* UIComponentIterateChildren(
	UI_COMPONENT* pComponent, 
	UI_COMPONENT* pCurChild, 
	int nUIType, 
	BOOL bDeepSearch );

void UIXMLLoadFont(	
	CMarkup& xml,
	UI_COMPONENT *component,
	UIX_TEXTURE* pFontTexture,
	char * szFontTag,
	UIX_TEXTURE_FONT *&pFont, 
	char * szFontSizeTag,
	int & nFontSize);

// DRB_3RD_PERSON
BOOL UIGetInterfaceActive(
	void);

void UIRestoreVBuffers(
	void);

void UIComponentSetVisibleByEnum(
	UI_COMPONENT_ENUM eComp,
	BOOL bVisible,
	BOOL bEVERYONE = FALSE);

BOOL UIComponentGetVisibleByEnum(
	UI_COMPONENT_ENUM eComp);

void UIComponentSetActiveByEnum(
	UI_COMPONENT_ENUM eComp,
	BOOL bActive);

BOOL UIComponentGetActiveByEnum(
	UI_COMPONENT_ENUM eComp);

UI_COMPONENT * UIComponentCreateNew(
	UI_COMPONENT * parent, 
	int nUIType,
	BOOL bLoadDefaults = FALSE);

void UISetTargetUnit(
	UNITID idUnit,
	BOOL bSelectionInRange);

void UISetAltTargetUnit(
	UNITID idUnit );

void UIWeaponHotkey(
	struct GAME* game,
	struct UNIT* unit,
	int n);

void PerfZeroAll(
	void);

void HitCountZeroAll(
	void);

void UIAutoMapZoom(
	int zoom,
	BOOL bForce = FALSE );

//#define NUM_COMPONENTS_TO_SLIDE 2
//BOOL UISwitchComponents(
//	UI_COMPONENT_ENUM peComponentFrom[ NUM_COMPONENTS_TO_SLIDE ],
//	UI_COMPONENT_ENUM peComponentTo[ NUM_COMPONENTS_TO_SLIDE ]);
//
//BOOL UISwitchComponents(
//	UI_COMPONENT_ENUM eComponentFrom,
//	UI_COMPONENT_ENUM eComponentTo);


void UIActiveBarHotkeyUp(
	struct GAME* game,
	struct UNIT* unit,
	int n);

void UIActiveBarHotkeyDown(
	struct GAME* game,
	struct UNIT* unit,
	int n,
	VECTOR* vClickLocation = NULL);

void UIPlayItemPickupSound(
	struct UNIT* unit );

void UIPlayItemPickupSound(
	UNITID idUnit );

BOOL UIPlayerInteractMenuOpen( void );

void UIUpdatePlayerInteractPanel( UNIT* pUnit );

void UIUpdateHolyRadiusMonsterCount();

void UIUpdateClosestItem();

UNITID UIGetHoverUnit(
	int* setby = NULL);

void UIToggleDrawDebugRects();

UI_COMPONENT * UIGetMainMenu( void );

void UIShowMainMenu(
	STARTGAME_CALLBACK pfnStartgameCallback );

BOOL UISkipMainMenu( 
	STARTGAME_CALLBACK pfnStartgameCallback );

void UIHideMainMenu(
	void);

void UIShowGameMenu(
	void);

void UIHideGameMenu(
	void);

void UIStopSkills(
	void);

void UIStopAllSkillRequests(
	void);

void UIShowLevelUpMessage( 
	int nNewLevel );

void UIShowRankUpMessage( 
	int nNewRank );

void UIShowAchievementUpMessage(void);

void UIMenuChangeSelect(
	int nDelta);

void UIMenuDoSelected(void);

void UIOptionsDialogCancel(void);

void UIRenderAll();

void UIToggleTargetIndicator();

void UIShowTargetIndicator(
	BOOL bShow);

void UIToggleTargetBrackets();

#if ISVERSION(DEVELOPMENT)
void UITraceIDs(const char *szName);
#endif

void UIUnitLabelsAdd( 
	struct UI_UNIT_DISPLAY *pUnitDisplay, 
	int nUnits,
	float flMaxRange);

void UIDebugLabelAddScreen(
	const WCHAR * puszText,
	const VECTOR * pvScreenPos,
	float fScale,
	DWORD dwColor = 0xffffffff,
	UI_ALIGN eAnchor = UIALIGN_TOP,
	UI_ALIGN eAlign = UIALIGN_CENTER,
	BOOL bPinToEdge = FALSE );

void UIDebugLabelAddWorld(
	const WCHAR * puszText,
	const VECTOR * pvWorldPos,
	float fScale,
	DWORD dwColor = 0xffffffff,
	UI_ALIGN eAnchor = UIALIGN_TOP,
	UI_ALIGN eAlign = UIALIGN_CENTER,
	BOOL bPinToEdge = FALSE );

BOOL UIGetShowItemNamesState( BOOL bIgnoreAlwaysState = FALSE );

BOOL UIGetAlwaysShowItemNamesState();


BOOL UnitDebugTestLabelFlags(
	void);
	
int UIUnitNameDisplayClick(
	int x,
	int y);

BOOL UIDebugTestingToggle();

void UICycleAutomap();

void UIComponentDrawLine(
						 UI_COMPONENT* component,
						 UIX_TEXTURE* texture,
						 UI_TEXTURE_FRAME* frame,
						 float x1,
						 float y1,
						 float x2,
						 float y2,
						 UI_RECT* cliprect,
						 DWORD color,
						 float LineWidth = 2.0f );

//void UIComponentFadeOut(
//	UI_COMPONENT *pComponent,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bForce = FALSE);
//
//void UIComponentFadeIn(
//	UI_COMPONENT *pComponent,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bForce = FALSE,
//	BOOL bOnlyIfUserActive = FALSE);
//
void UISetNeedToRenderAll(
	void);

void UISetNeedToRender(
	UI_COMPONENT *component);

void UISetNeedToRender(
	int nRenderSection);

void UIClearNeedToRender(
	int nRenderSection);

void UIRepaint(
	void);

void UIRefreshText(
	void);

struct UI_COMPONENT *UIComponentGetById(
	int id);

void UIToggleWorldMap();

BOOL UIToggleHideUI();

BOOL UIHideGenericDialog( void );

void UISchedulerMsgCallback(
							GAME* game, 
							const CEVENT_DATA& data,
							DWORD);

enum GENERIC_DIALOG_FLAG
{
	GDF_CENTER_MOUSE_ON_OK			= MAKE_MASK( 0 ),
	GDF_CENTER_MOUSE_ON_CANCEL		= MAKE_MASK( 1 ),
	GDF_OVERRIDE_EVENTS				= MAKE_MASK( 2 ),
	GDF_NO_OK_BUTTON				= MAKE_MASK( 3 ),
};

void UIShowGenericDialog(
	const WCHAR *szHeader,
	const WCHAR *szText,
	BOOL bShowCancel = FALSE,
	const DIALOG_CALLBACK *pCallbackOK = NULL,
	const DIALOG_CALLBACK *pCallbackCancel = NULL,
	DWORD dwGenericDialogFlags = 0);

void UIShowGenericDialog(
	const char *szHeader,
	const char *szText,
	BOOL bShowCancel = FALSE,
	const DIALOG_CALLBACK *pCallbackOK = NULL,
	const DIALOG_CALLBACK *pCallbackCancel = NULL,
	DWORD dwGenericDialogFlags = 0);

void UIShowConfirmDialog(
	const char * szStringKey1,
	const char * szStringKey2,
	PFN_DIALOG_CALLBACK pfnCallback);

void UIShowConfirmDialog(
	LPCWSTR wszString1,
	LPCWSTR wszString2,
	PFN_DIALOG_CALLBACK pfnCallback);

UI_POSITION UIComponentGetAbsoluteLogPosition(
	UI_COMPONENT *component);

UI_RECT UIComponentGetAbsoluteRect(
	UI_COMPONENT* component);

enum GET_FONT_TEXTURE_FLAGS
{
	GFTF_DO_NOT_SET_IN_GLOBALS,				// after loaded, do not set the texture in the UI globals
	GFTF_IGNORE_EXISTING_GLOBAL_ENTRY,		// ignore any existing texture loaded in globals
};

UIX_TEXTURE* UIGetFontTexture(
	enum LANGUAGE eLanguage,
	DWORD dwGetFontTextureFlags = 0);		// see GET_FONT_TEXTURE_FLAGS

void UIComponentRemoveAllElements(
	UI_COMPONENT* component);

BOOL XmlLoadString(
	CMarkup & xml,
	const char * szKey, 
	char * szVar, 
	int nVarLength,
	const char *szDef = NULL);

void UIXMLLoadEnum(
	CMarkup& xml,
	LPCSTR szKey,
	int& nVar,
	STR_DICT *pStrTable,
	int nDefault);

void UIXMLLoadColor(
	CMarkup& xml,
	UI_COMPONENT *pComponent,
	LPCSTR szBaseKey,
	DWORD& dwVar,
	DWORD dwDefault);

PCODE UIXmlLoadCode(
	CMarkup& xml,
	const char* key,
	PCODE codeDefault);

UI_COMPONENT* UIComponentGetByEnum(
	UI_COMPONENT_ENUM eComp);

#ifdef _DEBUG
#define CSchedulerRegisterMessage(t, cmp, msg, w, l)		CSchedulerRegisterMessageDbg(t, cmp, msg, w, l, "UISchedulerMsgCallback", __LINE__)
DWORD CSchedulerRegisterMessageDbg(
	TIME time,
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam,
	const char* file,
	int line);
#else
DWORD CSchedulerRegisterMessage(
	TIME time,
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);
#endif

void UIClosePopupMenus(
	void);

void UIClearHoverText(
	UI_COMPONENT_ENUM eExcept = UICOMP_INVALID,
	BOOL bImmediate = FALSE);

void UISetHoverUnit(
	UNITID idHover,
	int idComponent);

void UISetCursorState(
	CURSOR_STATE eState,
	DWORD dwParam = 0,
	BOOL bForce = FALSE);

void UIUpdateHardwareCursor( void );

void UIPlaceMod(
	UNIT* itemtomod,
	UNIT* itemtoplace,
	BOOL bShowScreen = TRUE);

BOOL UICheckRequirementsWithFailMessage(
	UNIT *pContainer, 
	UNIT *pUnitCursor, 
	UNIT *pIgnoreItem, 
	int nLocation, 
	UI_COMPONENT * pComponent);

void UIPlayItemPutdownSound(
	UNIT* unit,
	UNIT* container,
	int nLocation);

void UIPlayCantUseSound(
	GAME* game,
	UNIT* container,
	UNIT* item );

void UIDelayedActivate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD dwTicket);

//void UIDelayedFadeout(
//	GAME* game,
//	const CEVENT_DATA& data,
//	DWORD);

void UISetHoverTextItem(
	UI_RECT *pRect,
	UNIT* item,
	float fRectScale = 1.0f);

void UISetHoverTextItem(
	UI_COMPONENT *pComponent,
	UNIT* item,
	float fScale = 1.0f);

void UISetHoverTextSkill(
	UI_COMPONENT *pComponent,
	UNIT *pUnit,
	int nSkill,
	UI_RECT *pPositionRect = NULL,
	BOOL bSecondary = FALSE,
	BOOL bNoReposition = FALSE,
	BOOL bFormatting = TRUE);

//void UISendComponentMsgWithAnimTime( 
//	UI_COMPONENT_ENUM eComponent,
//	UINT msg,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bOnlyIfUserActive = FALSE);
//
//void UISendComponentMsgWithAnimTime( 
//	UI_COMPONENT *pComponent,
//	UINT msg,
//	float fDurationMultiplier,
//	DWORD &dwAnimTime,
//	BOOL bOnlyIfUserActive = FALSE);

void UIComponentThrobStart(
	UI_COMPONENT * pComponent,
	DWORD dwDurationMS,
	DWORD dwPeriodMS,
	BOOL bReverse = FALSE);

UI_MSG_RETVAL UIComponentThrobEnd(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UIComponentBlink(
	UI_COMPONENT *pComponent, 
	int *pnDuration = NULL, 
	int *pnPeriod = NULL,
	BOOL bTurnInvisibleAfter = FALSE);

void UIComponentBlinkStop(
	UI_COMPONENT *pComponent);

void UIElementStartBlink(
	UI_GFXELEMENT *pElement, 
	int nBlinkDuration, 
	int nBlinkPeriod, 
	DWORD dwBlinkColor);

CURSOR_STATE UIGetCursorState(
	void);

int UIGetCursorActivateSkill(
	void);

BOOL UIAutoEquipItem(
	UNIT *item,
	UNIT *container,
	int nFromWeaponConfig = -1);

UNIT* UIGetControlUnit(
	void);

BOOL UICanSelectTarget(
	UNITID idTarget);

void UICrossHairsShow(
	BOOL bShow);

UI_MSG_RETVAL UIComponentMouseHoverLong(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIComponentMouseHover(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIComponentRefreshTextKey(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIComponentLanguageChanged(
	struct UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

BOOL IsIncomingMessage(int);

BOOL UIGetStdItemCursorSize(
	UNIT *pItem,
	float & fWidth,
	float & fHeight);

void UIComponentWindowShadeOpen(
	UI_COMPONENT *component);

void UIComponentWindowShadeClosed(
	UI_COMPONENT *component);

void UIComponentSetAnimTime(
	UI_COMPONENT* component,								
	const UI_POSITION& StartPos,
	const UI_POSITION& EndPos,
	DWORD time,
	BOOL bFading);

void UIComponentAnimate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD);

UNIT * UIGetClosestLabeledItem(
	BOOL *pbInRange);

void UICloseAll(
	BOOL bSuppressUISounds = TRUE,
	BOOL bImmediate = FALSE);

#if (ISVERSION(DEVELOPMENT))		
void UISetMsgTrace(
	BOOL bOn,
	const char * szComponentName = "",
	const char * szMsgName = "",
	const char * szResult = "");

BOOL UIDebugEditKeyDown(
	WPARAM wKey);

BOOL UIDebugEditKeyUp(
	WPARAM wKey);

#endif

void UIGenerateMouseMoveMessages(
	void);

int UIGetSpeakerNumFromText(
	const WCHAR *szText,
	int nPageNum,
	BOOL &bVideo);

void UIDebugEditSetComponent(
	const char *szCompName);

void UISendHoverMessage(
	BOOL bLongDelay);

BOOL UIGetUnitScreenBox(
	UNIT* unit,
	UI_RECT &rectout);

UIX_TEXTURE * UILoadComponentTexture(
	UI_COMPONENT *component,
	const char * szTex,
	BOOL bLoadFromLocalizedPak);

UNITID UICreateClientOnlyUnit( 
	GAME * game, 
	int nClass );

BOOL UIDoNPCDialogReplacements(
	UNIT *pPlayer,
	WCHAR *uszDest, 
	int nStrLen, 
	const WCHAR *puszInputText);

BOOL UIQuickClose(
	void);

void UIGameRestart(
	void );

BOOL UIXmlCheckTagForLocation(
	CMarkup& xml);

UI_MSGHANDLER * UIGetMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg,
	int nStatID = INVALID_LINK);

BOOL UIAddMsgHandler(
	UI_COMPONENT *pComponent,
	int nMsg,
	PFN_UIMSG_HANDLER pfnHandler,
	int nStatID,
	BOOL bAddingDefaults = FALSE);

// -- Exported message handler functions --------------------------------------
UI_MSG_RETVAL  UIComponentOnPaint		(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIComponentOnActivate	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIComponentOnInactivate(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIComponentOnMouseLeave(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);

//----------------------------------------------------------------------------
// Inlines
//----------------------------------------------------------------------------
inline float UIDefaultWidth( void )
{
	return UI_DEFAULT_WIDTH * g_UI.m_fUIScaler;
}

inline float UIDefaultHeight( void )
{
	return UI_DEFAULT_HEIGHT * g_UI.m_fUIScaler;
}

inline float UIRound(
	float f)				
{
	return (float)ROUND(f) - 0.5f;
}

inline void UIRound(UI_POSITION & pos)
{
	pos.m_fX = UIRound(pos.m_fX);
	pos.m_fY = UIRound(pos.m_fY);
}

inline void UIRound(UI_RECT & rect)
{
	rect.m_fX1 = UIRound(rect.m_fX1);
	rect.m_fY1 = UIRound(rect.m_fY1);
	rect.m_fX2 = UIRound(rect.m_fX2);
	rect.m_fY2 = UIRound(rect.m_fY2);
}

//----------------------------------------------------------------------------
inline BOOL UIUnionRects(
	UI_RECT& rect1,
	UI_RECT& rect2,
	UI_RECT& unionrect)
{
	if (rect1.m_fX2 < rect2.m_fX1 ||
		rect1.m_fX1 > rect2.m_fX2 ||
		rect1.m_fY2 < rect2.m_fY1 ||
		rect1.m_fY1 > rect2.m_fY2 )
	{
		return FALSE;
	}

	unionrect.m_fX1 = MAX(rect1.m_fX1, rect2.m_fX1);
	unionrect.m_fX2 = MIN(rect1.m_fX2, rect2.m_fX2);
	unionrect.m_fY1 = MAX(rect1.m_fY1, rect2.m_fY1);
	unionrect.m_fY2 = MIN(rect1.m_fY2, rect2.m_fY2);

	return TRUE;
}

//----------------------------------------------------------------------------
inline BOOL UIInRect(
	const UI_POSITION& pos,
	const UI_RECT& rect,
	float fBuffer = 0.0f)
{
	if (pos.m_fX + fBuffer < rect.m_fX1 ||
		pos.m_fX - fBuffer > rect.m_fX2 ||
		pos.m_fY + fBuffer < rect.m_fY1 ||
		pos.m_fY - fBuffer > rect.m_fY2 )
	{
		return FALSE;
	}

	return TRUE;
}

inline BOOL UIInRect(
	const UI_RECT& rect1,
	const UI_RECT& rect2)
{
	if (rect1.m_fX2 < rect2.m_fX1 ||
		rect1.m_fX1 > rect2.m_fX2 || 
		rect1.m_fY2 < rect2.m_fY1 ||
		rect1.m_fY1 > rect2.m_fY2 )
	{
		return FALSE;
	}

	return TRUE;
}

inline BOOL UIEntirelyInRect(
	const UI_RECT& rect1,
	const UI_RECT& rect2)
{
	if (rect1.m_fX1 < rect2.m_fX1 ||
		rect1.m_fX2 > rect2.m_fX2 || 
		rect1.m_fY1 < rect2.m_fY1 ||
		rect1.m_fY2 > rect2.m_fY2 )
	{
		return FALSE;
	}

	return TRUE;
}

inline float UIGetLogToScreenRatioX(
	void)
{
	return g_UI_Ratios.m_fLogToScreenRatioX;
}

inline float UIGetLogToScreenRatioY(
	void)
{
	return g_UI_Ratios.m_fLogToScreenRatioY;
}

inline float UIGetScreenToLogRatioX(
	void)
{
	return g_UI_Ratios.m_fScreenToLogRatioX;
}

inline float UIGetScreenToLogRatioY(
	void)
{
	return g_UI_Ratios.m_fScreenToLogRatioY;
}

inline float UIGetAspectRatio(
	void)
{
	return g_UI_Ratios.m_fScreenToLogRatioX / g_UI_Ratios.m_fScreenToLogRatioY;
}

inline float UIGetLogToScreenRatioX(
	BOOL bNoScale)
{
	if (bNoScale)
	{
		return 1.0f;
	}
	else
	{
		return UIGetLogToScreenRatioX();
	}
}

inline float UIGetLogToScreenRatioY(
	BOOL bNoScale)
{
	if (bNoScale)
	{
		return 1.0f;
	}
	else
	{
		return UIGetLogToScreenRatioY();
	}
}

inline float UIGetScreenToLogRatioX(
	BOOL bNoScale)
{
	if (!bNoScale)
	{
		return 1.0f;
	}
	else
	{
		return UIGetScreenToLogRatioX();
	}
}

inline float UIGetScreenToLogRatioY(
	BOOL bNoScale)
{
	if (!bNoScale)
	{
		return 1.0f;
	}
	else
	{
		return UIGetScreenToLogRatioY();
	}
}

inline UNITID UIGetModUnit(
	void)
{
	return g_UI.m_idGunMod;		// TODO: need to change this to use a focus unit on the gunmod panel - BSP
}

inline void UISetModUnit(
	UNITID unitid)
{
	g_UI.m_idGunMod = unitid;		// TODO: need to change this to use a focus unit on the gunmod panel - BSP
}

inline int UIComponentGetId(
	UI_COMPONENT* component)
{
	if (!component)
		return INVALID_ID;

	return component->m_idComponent;
}

inline void UIComponentCheckClipping(
	UI_COMPONENT* component,
	UI_RECT& rect)
{
	// checking the clip rect at each level because I think it'll be too expensive not too.
	// if we need to check at each level we can.

	if (component->m_pParent)
	{
		if (component->m_pParent->m_rectClip.m_fX1 != 0.0f || 
			component->m_pParent->m_rectClip.m_fX2 != 0.0f || 
			component->m_pParent->m_rectClip.m_fY1 != 0.0f || 
			component->m_pParent->m_rectClip.m_fY2 != 0.0f)
		{
			rect.m_fX1 += component->m_Position.m_fX;
			rect.m_fX2 += component->m_Position.m_fX;
			rect.m_fY1 += component->m_Position.m_fY;
			rect.m_fY2 += component->m_Position.m_fY;

			if (component->m_bScrollsWithParent)
			{
				rect.m_fX1 -= component->m_pParent->m_ScrollPos.m_fX;
				rect.m_fX2 -= component->m_pParent->m_ScrollPos.m_fX;
				rect.m_fY1 -= component->m_pParent->m_ScrollPos.m_fY;
				rect.m_fY2 -= component->m_pParent->m_ScrollPos.m_fY;
			}

			rect.m_fX1 = MAX(rect.m_fX1, component->m_pParent->m_rectClip.m_fX1);
			rect.m_fX2 = MIN(rect.m_fX2, component->m_pParent->m_rectClip.m_fX2);
			rect.m_fY1 = MAX(rect.m_fY1, component->m_pParent->m_rectClip.m_fY1);
			rect.m_fY2 = MIN(rect.m_fY2, component->m_pParent->m_rectClip.m_fY2);

			UIComponentCheckClipping(component->m_pParent, rect);

			if (component->m_bScrollsWithParent)
			{
				rect.m_fX1 += component->m_pParent->m_ScrollPos.m_fX;
				rect.m_fX2 += component->m_pParent->m_ScrollPos.m_fX;
				rect.m_fY1 += component->m_pParent->m_ScrollPos.m_fY;
				rect.m_fY2 += component->m_pParent->m_ScrollPos.m_fY;
			}

			rect.m_fX1 -= component->m_Position.m_fX;
			rect.m_fX2 -= component->m_Position.m_fX;
			rect.m_fY1 -= component->m_Position.m_fY;
			rect.m_fY2 -= component->m_Position.m_fY;
		}

	}
}

inline UI_RECT UIComponentGetRect(
	UI_COMPONENT* component)
{
	UI_RECT componentrect( 0.0f, 
						   0.0f, 
						   component->m_fWidth,
						   component->m_fHeight );

	UIComponentCheckClipping(component, componentrect);
	componentrect.m_fX1 *= UIGetScreenToLogRatioX(component->m_bNoScale);
	componentrect.m_fX2 *= UIGetScreenToLogRatioX(component->m_bNoScale);
	componentrect.m_fY1 *= UIGetScreenToLogRatioY(component->m_bNoScale);
	componentrect.m_fY2 *= UIGetScreenToLogRatioY(component->m_bNoScale);

	//componentrect.m_fX1 -= UIComponentGetScrollPos(component).m_fX;
	//componentrect.m_fX2 -= UIComponentGetScrollPos(component).m_fX;
	//componentrect.m_fY1 -= UIComponentGetScrollPos(component).m_fY;
	//componentrect.m_fY2 -= UIComponentGetScrollPos(component).m_fY;

	return componentrect;
}

inline UI_RECT UIComponentGetRectBase(
								  UI_COMPONENT* component)
{
	UI_RECT componentrect( 0.0f, 
		0.0f, 
		component->m_fWidth, 
		component->m_fHeight );
	//componentrect.m_fX1 -= UIComponentGetScrollPos(component).m_fX;
	//componentrect.m_fX2 -= UIComponentGetScrollPos(component).m_fX;
	//componentrect.m_fY1 -= UIComponentGetScrollPos(component).m_fY;
	//componentrect.m_fY2 -= UIComponentGetScrollPos(component).m_fY;

	return componentrect;
}

inline UI_RECT UIComponentGetPositionRect(
	UI_COMPONENT* component)
{
	UI_RECT componentrect(component->m_Position.m_fX, component->m_Position.m_fY, component->m_Position.m_fX + component->m_fWidth, component->m_Position.m_fY + component->m_fHeight);

	return componentrect;
}

inline UIX_TEXTURE* UIComponentGetTexture(
	UI_COMPONENT* component)
{
	while (component)
	{
		if (component->m_pTexture)
		{
			return component->m_pTexture;
		}
		component = component->m_pParent;
	}
	return NULL;
}

inline BOOL UIComponentGetVisible(
	UI_COMPONENT* component)
{
	if (!component)
		return FALSE;

	while (component)
	{
		if (!component->m_bVisible)
		{
			return FALSE;
		}
		component = component->m_pParent;
	}
	return TRUE;
}

inline BOOL UIComponentGetVisible(
	UI_COMPONENT_ENUM eComponent)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(eComponent);
	ASSERT_RETFALSE(pComp);
	return UIComponentGetVisible(pComp);
}


inline BOOL UIComponentGetActive(
	UI_COMPONENT* component)
{
	if (!component)
		return FALSE;
	
	//return component->m_eState == UISTATE_ACTIVE || component->m_eState == UISTATE_ACTIVATING;

	if (component->m_eState == UISTATE_INACTIVE || component->m_eState == UISTATE_INACTIVATING)
	{
		return FALSE;
	}

	if (!UIComponentGetVisible(component))
	{
		return FALSE;
	}

	return TRUE;
}

inline BOOL UIComponentGetActive(
	UI_COMPONENT_ENUM eComponent)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(eComponent);
	ASSERT_RETFALSE(pComp);
	return UIComponentGetActive(pComp);
}

//----------------------------------------------------------------------------
inline void UIComponentSetFade(
	UI_COMPONENT *pComponent, 
	float fFade)
{
	pComponent->m_fFading = fFade;
	UISetNeedToRender(pComponent);
}

//----------------------------------------------------------------------------
inline float UIComponentGetFade(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETZERO(pComponent);
	float fFade = pComponent->m_fFading;

	UI_COMPONENT *pParent = pComponent->m_pParent;
	while (pParent)
	{
		if (pParent->m_fFading != 0.0f)
		{
			if (fFade == 0.0f)
			{
				fFade = pParent->m_fFading;
			}
			else
			{
				fFade = MAX(fFade, pParent->m_fFading);
			}
		}
		pParent = pParent->m_pParent;
	}

	return fFade;
}

//----------------------------------------------------------------------------
inline BOOL UIComponentIsParentOf(
	UI_COMPONENT *pParent,
	UI_COMPONENT *pChild)
{
	if (pChild == NULL)
		return FALSE;

	if (pParent == pChild)
		return TRUE;

	return UIComponentIsParentOf(pParent, pChild->m_pParent);
}

//----------------------------------------------------------------------------
inline UIX_TEXTURE_FONT* UIComponentGetFont(
	UI_COMPONENT* component)
{
	while (component)
	{
		if (component->m_pFont)
		{
			return component->m_pFont;
		}
		component = component->m_pParent;
	}
	return NULL;
}


//----------------------------------------------------------------------------
inline int UIComponentGetFontSize(
	UI_COMPONENT* component,
	UIX_TEXTURE_FONT* font = NULL)
{
	UI_COMPONENT *pOrigComp = component;

	if (!font)
	{
		font = UIComponentGetFont(component);
	}

	while (component)
	{
		if (component->m_nFontSize)
		{
			return component->m_nFontSize;
		}
		component = component->m_pParent;
	}

	ASSERTV_RETZERO(FALSE, "Component [%s] has no font size", pOrigComp->m_szName);
	REF(pOrigComp);	// CHB 2007.03.12 - silence release build warning
}

//----------------------------------------------------------------------------
UNIT* UnitGetById(GAME * game, UNITID unitid);
GAME* AppGetCltGame(void);

inline UNIT* UIComponentGetFocusUnit(
	UI_COMPONENT* component,
	BOOL bSearchParents = TRUE)
{
	GAME* game = AppGetCltGame();
	if (!game)
	{
		return NULL;
	}
	while (component)
	{
		if (component->m_idFocusUnit != INVALID_ID)
		{
			UNIT* unit = UnitGetById(game, component->m_idFocusUnit);
			if (unit)
			{
				return unit;
			}
			else
			{
				//component->m_idFocusUnit = INVALID_ID;		// some components need to save the id to track a unit even if the pointer is currently unavailable
			}
		}
		if (bSearchParents == FALSE ||
			component->m_bHasOwnFocusUnit)
		{
			break;
		}
		component = component->m_pParent;
	}
	return NULL;
}

//----------------------------------------------------------------------------
UNITID UnitGetId(const UNIT* unit);

inline UNITID UIComponentGetFocusUnitId(
	UI_COMPONENT* component)
{
	UNIT* unitFocus = UIComponentGetFocusUnit(component);
	if (unitFocus)
	{
		return UnitGetId(unitFocus);
	}
	return INVALID_ID;
}

void UIComponentSetFocusUnit(
	UI_COMPONENT* component,
	UNITID idFocusUnit,
	BOOL bForce = FALSE);

void UIComponentSetFocusUnitByEnum(
	UI_COMPONENT_ENUM eComponent,
	UNITID idFocus,
	BOOL bForce = FALSE);

UI_MSG_RETVAL UIComponentSetFocusUnit(
	UI_COMPONENT* component,
	int ,
	DWORD wParam,
	DWORD );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline float UIComponentGetLowestPoint(
	UI_COMPONENT *pComponent,
	float& fLowestPoint,
	float fPlusPoint,
	int nDepth,
	int nMaxDepth,
	BOOL bVisibleOnly = TRUE)
{
	if (nDepth <= nMaxDepth)
	{
		UI_COMPONENT *pChild = pComponent->m_pFirstChild;
		while (pChild)
		{
			if (bVisibleOnly == FALSE || UIComponentGetVisible(pChild))
			//if (pChild->m_eState == UISTATE_ACTIVATING)
			//{
			//	fLowestPoint = MAX(fLowestPoint, pChild->m_ActivePos.m_fY + pChild->m_fHeight + fPlusPoint);
			//	fLowestPoint = MAX(fLowestPoint, UIComponentGetLowestPoint(pChild, fLowestPoint, pChild->m_ActivePos.m_fY + fPlusPoint, nDepth+1, nMaxDepth, bVisibleOnly));
			//}
			//else if (pChild->m_eState == UISTATE_INACTIVATING)
			//{
			//	fLowestPoint = MAX(fLowestPoint, pChild->m_InactivePos.m_fY + pChild->m_fHeight + fPlusPoint);
			//	fLowestPoint = MAX(fLowestPoint, UIComponentGetLowestPoint(pChild, fLowestPoint, pChild->m_InactivePos.m_fY + fPlusPoint, nDepth+1, nMaxDepth, bVisibleOnly));
			//}
			//else
			{
				fLowestPoint = MAX(fLowestPoint, pChild->m_Position.m_fY + pChild->m_fHeight + fPlusPoint);
				fLowestPoint = MAX(fLowestPoint, UIComponentGetLowestPoint(pChild, fLowestPoint, pChild->m_Position.m_fY + fPlusPoint, nDepth+1, nMaxDepth, bVisibleOnly));
			}

			pChild = pChild->m_pNextSibling;
		}
	}

	return fLowestPoint;
}

inline void UISetIgnoreStatMsgs(
	BOOL bIgnore)
{
	g_UI.m_bIgnoreStatMessages = bIgnore;
}

inline float UIGetAngleRad(
	float x1,
	float y1,
	float x2,
	float y2)
{
	float dx = x2 - x1;
	float dy = y2 - y1;

	float fAngleRad = (dx ? atanf(dy / dx) : (dy > 0.0f ? 0.0f : PI_OVER_TWO));
	if (dx < 0.0f)
		fAngleRad += PI;

	fAngleRad += PI_OVER_TWO;
	return NORMALIZE(fAngleRad, TWOxPI);
}

inline float UIGetAngleDeg(
	float x1,
	float y1,
	float x2,
	float y2)
{
	return (UIGetAngleRad(x1, y1, x2, y2) * 180.0f ) / PI;
}

inline void UIComponentSetAlpha(
	UI_COMPONENT *pComponent,
	BYTE byAlpha,
	BOOL bSetChildren = FALSE)
{
	ASSERT_RETURN(pComponent);
	pComponent->m_dwColor = UI_MAKE_COLOR(byAlpha, pComponent->m_dwColor);

	if (bSetChildren)
	{
		UI_COMPONENT *pChild = pComponent->m_pFirstChild;
		while (pChild)
		{
			UIComponentSetAlpha(pChild, byAlpha, TRUE);
			pChild = pChild->m_pNextSibling;
		}
	}
}

inline BYTE UIComponentGetAlpha(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETZERO(pComponent);
	return UI_GET_ALPHA(pComponent->m_dwColor);
}

inline BOOL UIComponentIsUserActive(
	UI_COMPONENT* component)
{
	if (!component)
	{
		return FALSE;
	}
	return component->m_bUserActive;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void UISetUIScaler( int nUIScaler )
{
	float fUIScaler = nUIScaler * .05f + 1;
	if( fUIScaler * 800.0f > AppCommonGetWindowWidth() )
	{
		fUIScaler = AppCommonGetWindowWidth() / 800.0f;
	}
	if( fUIScaler * 600.0f > AppCommonGetWindowHeight() )
	{
		fUIScaler = AppCommonGetWindowHeight() / 600.0f;
	}
	if( g_UI.m_fUIScaler != fUIScaler )
	{
		g_UI.m_fUIScaler = fUIScaler;
		UIRequestReload();
	}
}

inline void UISetFlag(
	UI_FLAG eFlag,
	BOOL bValue)
{
	SETBIT( g_UI.m_dwFlags, eFlag, bValue );
}	

inline BOOL UITestFlag(
	UI_FLAG eFlag)
{
	return TESTBIT( g_UI.m_dwFlags, eFlag );
}	

inline BOOL UIToggleFlag(
	UI_FLAG eFlag)
{
	UISetFlag( eFlag, !UITestFlag( eFlag ) );
	return UITestFlag( eFlag );
}	


// vvv Tugboat-specific
void UI_TB_HotSpellHotkeyUp(
	struct GAME* game,
	struct UNIT* unit,
	int n);

BOOL UI_TB_HotSpellHotkeyDown(
	struct GAME* game,
	struct UNIT* unit,
	int n);

BOOL UI_TB_MouseOverPanel( 
	void );

void UI_TB_WorldMapRevealArea( int nRevealArea );

void UI_TB_WorldMapSetZoneByArea( int nArea );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct UI_COMP_LATE_POINTER
{
protected:
	UI_COMPONENT *	m_pComponent;
	char *			m_szName;

	UI_COMP_LATE_POINTER() : m_pComponent(NULL), m_szName(NULL) {};

friend void UIComponentLoadAnimRelationships(CMarkup& xml, UI_COMPONENT *component);

public:
	UI_COMPONENT *Get(void)
	{
		if (m_pComponent)
			return m_pComponent;

		if (m_szName)
		{
			m_pComponent = UIComponentFindChildByName(g_UI.m_Components, m_szName);
			return m_pComponent;
		}

		return NULL;
	}

	void Set(const char * szName)
	{
		Free();
		if (szName)
		{
			int nSize = PStrLen(szName);
			m_szName = (char *)MALLOCZ(NULL, sizeof(char) * (nSize + 1));
			PStrCopy(m_szName, szName, nSize+1);
		}
	}

	// please use this sparingly
	void SetDirect(UI_COMPONENT *pComponent)
	{
		Free();
		m_pComponent = pComponent;
	}

	void Free( void )
	{
		m_pComponent = NULL;
		if (m_szName)
		{
			FREE(NULL, m_szName);
			m_szName = NULL;
		}
	}
};

struct ANIM_RELATIONSHIP_NODE : public UI_COMP_LATE_POINTER
{
	enum REL_TYPE 
	{
		ANIM_REL_INVALID = 0,
		ANIM_REL_OPENS,
		ANIM_REL_CLOSES,
		ANIM_REL_LINKED,
		ANIM_REL_BLOCKED_BY,
		ANIM_REL_REPLACES,
	};

	REL_TYPE m_eRelType;
	BOOL m_bOnActivate;
	BOOL m_bDirectOnly;
	BOOL m_bOnlyIfUserActive;
	BOOL m_bPreserverUserActive;
	BOOL m_bNoDelay;

	BOOL operator==(
		const ANIM_RELATIONSHIP_NODE & rhs) const
	{
		return (m_pComponent == rhs.m_pComponent &&
			    m_eRelType == rhs.m_eRelType);
	}	

};

#endif
