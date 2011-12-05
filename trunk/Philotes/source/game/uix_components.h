//----------------------------------------------------------------------------
// uix_components.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_COMPONENTS_H_
#define _UIX_COMPONENTS_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef _PTIME_H_
#include "ptime.h"
#endif

#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef __E_UI__
#include "e_ui.h"
#endif

#ifndef _COLORS_H_
#include "colors.h"
#endif

#ifndef _UIX_PRIV_H_
#include "uix_priv.h"
#endif

#ifndef _UIX_GRAPHIC_H_
#include "uix_graphic.h"
#endif

#ifndef _STRINGTABLE_H_
#include "stringtable.h"
#endif

#ifndef _APPCOMMONTIMER_H_
#include "appcommontimer.h"
#endif

#ifndef _UIX_HYPERTEXT_H_
#include "uix_hypertext.h"
#endif

#ifndef _LIST_H_
#include "list.h"
#endif

#ifndef __E_AUTOMAP__
#include "e_automap.h"
#endif

//----------------------------------------------------------------------------
// Forwards
//----------------------------------------------------------------------------
class CMarkup;
enum EXCELTABLE;
enum UI_COMPONENT_ENUM;
enum GLOBAL_STRING;
enum CHAT_TYPE;

struct UI_SCROLLBAR;

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------
#define UI_LINE_DEFAULT_MAX_LEN	1024

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UITYPE_SCREEN,
	UITYPE_CURSOR,
	UITYPE_LABEL,
	UITYPE_PANEL,
	UITYPE_BUTTON,
	UITYPE_BAR,
	UITYPE_SCROLLBAR,
	UITYPE_TOOLTIP,
	UITYPE_CONVERSATION_DLG,
	UITYPE_TEXTBOX,
	UITYPE_FLEXBORDER,
	UITYPE_FLEXLINE,
	UITYPE_MENU,
	UITYPE_LISTBOX,
	UITYPE_COMBOBOX,

	// Need to find a way to move these to uix_components_complex without breaking the cast functions
	UITYPE_STATDISPL,
	UITYPE_AUTOMAP,

	UITYPE_EDITCTRL,
	UITYPE_ITEMBOX,
	UITYPE_COLORPICKER,

	NUM_UITYPES_BASIC,
};

//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// UI_LINE class, used by textboxes, listboxes, editboxes, labels and most
// other things that needs to display text.
// KCK: If this UI code was written in C++ from the start, this would be so
// much easier and cleaner to do - But alas, this isn't the case so this
// class should be pointed to, not contained, by any struct/class using 
// MALLOC for memory allocation rather than new. It also means this must
// be allocated using the MALLOC_NEW macro (why don't we override new?!).
//----------------------------------------------------------------------------
class UI_LINE
{
private:
	WCHAR*					m_szText;
	int						m_nSize;
public:
	DWORD					m_dwColor;
	TIME					m_tiTimeAdded;
	CItemPtrList<UI_HYPERTEXT>	m_HypertextList;

public:
	UI_LINE(int nSize = 0);
	~UI_LINE();

	//----------------------------------------------------------------------------
	// Public Methods
	//----------------------------------------------------------------------------
	// int GetSize(void)				// Get the current buffer size
	// BOOL Resize(int)					// Resize the text buffer.
	// void SetText(WCHAR*)				// Set the text buffer (copies string)
	// void SetTextPtr(WCHAR*)			// Set the text buffer (assigns ptr)
	// const WCHAR* GetText(void)		// Returns the text buffer (non modifiable)
	// WCHAR* EditTexT(void)			// Returns a modifiable pointer to the buf
	// BOOL HasText(void)				// Returns true if buffer is not empty
	//----------------------------------------------------------------------------
	int					GetSize(void)				{ return m_nSize; }
	BOOL				Resize(int nSize);
	void				SetText(const WCHAR *szText);
	inline void			SetTextPtr(WCHAR *szText)	{ m_szText = szText; }
	inline const WCHAR* GetText(void)				{ return m_szText;	}
	inline WCHAR*		EditText(void)				{ return m_szText;	}
	inline BOOL			HasText(void)				{ return (m_szText && m_szText[0] != 0); }
};

struct UI_COMPONENT_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	char szComponentString[ MAX_UI_COMPONENT_NAME_LENGTH ];
};

struct UI_SCREENEX : UI_COMPONENT
{
};

struct UI_PANELEX : UI_COMPONENT
{
	int						m_nCurrentTab;		// which of this panel's child panels is the active tab
	int						m_nTabNum;			// the tab number assigned to this panel

	// extra Model-drawing info
	float					m_fModelRotation;
	int						m_nModelCam;
	BOOL					m_bMouseDown;
	UI_POSITION				m_posMouseDown;

	UI_TEXTURE_FRAME*		m_pHighlightFrame;

	UI_COMPONENT*			m_pResizeComponent;

	// Public methods
	UI_PANELEX(void);
};

struct UI_CURSOR : UI_COMPONENT
{
	CURSOR_STATE			m_eCursorState;
	UI_TEXTURE_FRAME*		m_pCursorFrame[NUM_UICURSOR_STATES];
	UIX_TEXTURE*			m_pCursorFrameTexture[NUM_UICURSOR_STATES];

	UNITID					m_idUnit;
	int						m_nFromWeaponConfig;		// This is used to track which weaponconfig the item was picked up from (if any)
	
	int						m_nSkillID;

	UNITID					m_idUseItem;

	int						m_nLastInvLocation;
	int						m_nLastInvX;
	int						m_nLastInvY;
	UNITID					m_idLastInvContainer;
	int						m_nItemPickActivateSkillID;

	UI_INV_ITEM_GFX_ADJUSTMENT m_tItemAdjust;				// some extra info for drawing an item
	float					m_fItemDrawWidth;
	float					m_fItemDrawHeight;

	// public methods
	UI_CURSOR::UI_CURSOR(void);
};

enum LABEL_MARQUEE_MODE
{
	MARQUEE_NONE = 0,
	MARQUEE_AUTO = 1,
	MARQUEE_HORIZ = 2,
	MARQUEE_VERT = 3,
};

enum LABEL_MARQUEE_STATE
{
	MARQUEE_STATE_NONE = 0,
	MARQUEE_STATE_SCROLLING,
	MARQUEE_STATE_START_WAIT,
	MARQUEE_STATE_END_WAIT,
};

struct UI_LABELEX : UI_COMPONENT
{
	UI_LINE					m_Line;
	int						m_nStringIndex;
	BOOL					m_bTextHasKey;
	UI_ELEMENT_TEXT*		m_pTextElement;
#if ISVERSION(DEVELOPMENT)
	char					m_szTextKey[ MAX_STRING_KEY_LENGTH ];
#endif
	int						m_nAlignment;
	BOOL					m_bAutosize;
	BOOL					m_bAutosizeHorizontalOnly;
	BOOL					m_bAutosizeVerticalOnly;
	BOOL					m_bWordWrap;
	BOOL					m_bForceWrap;
	BOOL					m_bSlowReveal;
	float					m_fMaxWidth;
	float					m_fMaxHeight;
	BOOL					m_bUseParentMaxWidth;
	BOOL					m_bSelected;
	DWORD					m_dwSelectedColor;
	BOOL					m_bDisabled;
	DWORD					m_dwDisabledColor;
	int						m_nMenuOrder;

	LABEL_MARQUEE_MODE		m_eMarqueeMode;
	LABEL_MARQUEE_STATE		m_eMarqueeState;
	DWORD					m_dwMarqueeAnimTicket;
	float					m_fMarqueeIncrement;
	int						m_nMarqueeEndDelay;
	int						m_nMarqueeStartDelay;
	UI_POSITION				m_posMarquee;

	float					m_fTextWidth;
	float					m_fTextHeight;
	int						m_nPage;
	int						m_nNumPages;
	BOOL					m_bDrawDropshadow;
	DWORD					m_dwDropshadowColor;

	float					m_fFrameOffsetX;
	float					m_fFrameOffsetY;
	
	// slow reveal stuff
	int 					m_nTextToRevealLength;
	int 					m_nMaxChars;
	int						m_nNumCharsToAddEachUpdate;
	DWORD					m_dwUpdateEvent;

	UI_SCROLLBAR*			m_pScrollbar;
	BOOL					m_bHideInactiveScrollbar;
	float					m_fScrollbarExtraSpacing;

	// unitmode tags
	struct LABEL_UNITMODE*	m_pUnitmodes;
	int						m_nUnitmodesAlloc;
	
	BOOL					m_bAutosizeFont;
	BOOL					m_bPlayUnitModes;
	BOOL					m_bBkgdRect;
	DWORD					m_dwBkgdRectColor;

	// public methods
	UI_LABELEX(void);
};

struct UI_STATDISPL : UI_LABELEX
{
	int						m_nDisplayLine;
	int						m_nDisplayArea;
	float					m_fSpecialSpacing;
	EXCELTABLE				m_eTable;
	BOOL					m_bShowIcons;
	BOOL					m_bVertIcons;
	UIX_TEXTURE_FONT*		m_pFont2;
	int						m_nFontSize2;

	int						m_nUnitType;		// restrict to only this unit type

	// public methods
	UI_STATDISPL(void);
};

struct UI_MENU : UI_COMPONENT
{
	UI_COMPONENT *			m_pSelectedChild;
	BOOL					m_bAlwaysOneSelected;

	// public methods
	UI_MENU(void);
};

struct UI_EDITCTRL : UI_LABELEX
{
	BOOL					m_bHasFocus;
	BOOL					m_bStartsWithFocus;
	int						m_nCaretPos;
	UI_POSITION				m_CaretPos;
	DWORD					m_dwPaintTicket;
	WCHAR					m_szOnlyAllowChars[256];
	WCHAR					m_szDisallowChars[256];
	int						m_nMaxLen;
	BOOL					m_bPassword;
	BOOL					m_bLimitTextToSpace;
	BOOL					m_bMultiline;
	BOOL					m_bEatsAllKeys;
	BOOL					m_bAllowIME;

	UI_COMPONENT*			m_pNextTabCtrl;
	UI_COMPONENT*			m_pPrevTabCtrl;
	char					m_szNextTabCtrlName[MAX_UI_COMPONENT_NAME_LENGTH];

	// public methods
	UI_EDITCTRL(void);
};

enum UIBUTTONSTYLE
{
	UIBUTTONSTYLE_PUSHBUTTON,
	UIBUTTONSTYLE_CHECKBOX,
	UIBUTTONSTYLE_RADIOBUTTON,
};

struct UI_BUTTONEX : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pLitFrame;
	UI_TEXTURE_FRAME*		m_pDownFrame;
	UI_TEXTURE_FRAME*		m_pInactiveFrame;
	UI_TEXTURE_FRAME*		m_pInactiveDownFrame;
	UI_TEXTURE_FRAME*		m_pDownLitFrame;
	
	BOOL					m_bOverlayLitFrame;
	UIBUTTONSTYLE			m_eButtonStyle;
	DWORD					m_dwPushstate;
	int						m_nRadioParentLevel;

	int						m_nAssocStat;		//for the stat points button
	int						m_nAssocTab;		//if this is set, this tab will activate when the button is pressed, and all others will deactivate

	float					m_fScrollIncrement;	// for scroll buttons
	UI_COMPONENT*			m_pScrollControl;	// for scroll buttons
	BOOL					m_bHideOnScrollExtent;
	DWORD					m_dwRepeatMS;

	int						m_nCheckOnSound;
	int						m_nCheckOffSound;

	UI_TEXTURE_FRAME*		m_pFrameMid;
	UI_TEXTURE_FRAME*		m_pFrameLeft;
	UI_TEXTURE_FRAME*		m_pFrameRight;
	UI_TEXTURE_FRAME*		m_pLitFrameMid;
	UI_TEXTURE_FRAME*		m_pLitFrameLeft;
	UI_TEXTURE_FRAME*		m_pLitFrameRight;
	UI_TEXTURE_FRAME*		m_pDownFrameMid;
	UI_TEXTURE_FRAME*		m_pDownFrameLeft;
	UI_TEXTURE_FRAME*		m_pDownFrameRight;
	UI_TEXTURE_FRAME*		m_pInactiveFrameMid;
	UI_TEXTURE_FRAME*		m_pInactiveFrameLeft;
	UI_TEXTURE_FRAME*		m_pInactiveFrameRight;
	UI_TEXTURE_FRAME*		m_pDownLitFrameMid;
	UI_TEXTURE_FRAME*		m_pDownLitFrameLeft;
	UI_TEXTURE_FRAME*		m_pDownLitFrameRight;

	// public methods
	UI_BUTTONEX(void);
};

struct UI_BAR : UI_COMPONENT
{
	PCODE					m_codeCurValue;
	PCODE					m_codeMaxValue;
	PCODE					m_codeRegenValue;
	int						m_nRegenStat;
	int						m_nIncreaseLeftStat;
	int						m_nBlinkThreshold;
	BOOL					m_bEndCap;
	float					m_fSlantWidth;
	int						m_nOrientation;
	int						m_nOldValue;
	int						m_nOldMax;
	int						m_nShowIncreaseDuration;
	BOOL					m_bShowTempRegen;
	int						m_nMaxValue;				// A max that can be specifically set without using a script
	int						m_nValue;					// ditto
	int						m_nMinValue;				// ditto

	BOOL					m_bRadial;
	float					m_fMaxAngle;
	BOOL					m_bCounterClockwise;
	float					m_fMaskAngleStart;
	float					m_fMaskBaseAngle;

	DWORD					m_dwAltColor;
	DWORD					m_dwAltColor2;
	DWORD					m_dwAltColor3;
	BOOL					m_bUseAltColor;

	int						m_nStat;					// just for feed bars right now (special case)

	// public methods
	UI_BAR(void);
};

struct UI_SCROLLBAR : UI_COMPONENT
{
	BOOL				m_bOrientation;
	BOOL				m_bSendPaintToParent;

	// these could be replaced with m_ScrollMin and m_ScrollMax but those are really
	//   for the subject of the scrolling... plus it'd be a pain right now.
	float				m_fMin;
	float				m_fMax;

	float				m_fValue;
	int					m_nOrientation;

	UI_BAR*				m_pBar;
	UI_TEXTURE_FRAME*	m_pThumbpadFrame;

	BOOL				m_bMouseDownOnThumbpad;
	UI_POSITION			m_posMouseDownOffset;
	UI_GFXELEMENT*		m_pThumbpadElement;
	UI_COMPONENT*		m_pScrollControl;

	// public methods
	UI_SCROLLBAR(void);
};

struct UI_FLEXBORDER : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pFrameTL;
	UI_TEXTURE_FRAME*		m_pFrameTM;
	UI_TEXTURE_FRAME*		m_pFrameTR;
	UI_TEXTURE_FRAME*		m_pFrameML;
	UI_TEXTURE_FRAME*		m_pFrameMM;
	UI_TEXTURE_FRAME*		m_pFrameMR;
	UI_TEXTURE_FRAME*		m_pFrameBL;
	UI_TEXTURE_FRAME*		m_pFrameBM;
	UI_TEXTURE_FRAME*		m_pFrameBR;

	BOOL					m_bAutosize;
	float					m_fBorderSpace;
	float					m_fBorderScale;
	BOOL					m_bStretchBorders;

	// public methods
	UI_FLEXBORDER(void);
};

struct UI_FLEXLINE : UI_COMPONENT
{
	BOOL					m_bHorizontal;
	UI_TEXTURE_FRAME*		m_pFrameStartCap;
	UI_TEXTURE_FRAME*		m_pFrameEndCap;
	UI_TEXTURE_FRAME*		m_pFrameLine;
	int						m_nVisibleOnSibling;
	BOOL					m_bAutosize;

	// public methods
	UI_FLEXLINE(void);
};

struct UI_TOOLTIP : UI_COMPONENT
{
	float					m_fMinWidth;
	float					m_fMaxWidth;
	BOOL					m_bAutosize;
	float					m_fXOffset;
	float					m_fYOffset;
	BOOL					m_bCentered;

	// public methods
	UI_TOOLTIP(void);
};

#define MAX_CONVERSATION_SPEAKERS	(2)
struct UI_CONVERSATION_DLG : UI_COMPONENT
{
	UNITID					m_idTalkingTo[MAX_CONVERSATION_SPEAKERS];				
	int						m_nDialog;				// the dialog being spoken (if provided)
	BOOL					m_bLeaveUIOpenOnOk;		// don't close the dialog when ok is hit, something else is coming.
	int						m_nQuest;

	// public methods
	UI_CONVERSATION_DLG(void);
};

struct UI_TEXTBOX : UI_COMPONENT
{
	int						m_nMaxLines;
	BOOL					m_bBottomUp;
	BOOL					m_bWordWrap;
	BOOL					m_bHideInactiveScrollbar;
	BOOL					m_bScrollOnLeft;
	float					m_fScrollbarExtraSpacing;
	int						m_nFadeoutTicks;
	int						m_nFadeoutDelay;
	BOOL					m_bDrawDropshadow;

	CItemPtrList<UI_LINE>	m_LinesList;			
	CItemPtrList<UI_LINE>	m_AddedLinesList;
	PList<QWORD>			m_DataList;
	PList<QWORD>			m_AddedDataList;
	DWORD					m_dwBkgColor;
	int						m_nAlignment;
	float					m_fBordersize;
	
	UI_SCROLLBAR*			m_pScrollbar;
	UI_TEXTURE_FRAME*		m_pScrollbarFrame;
	UI_TEXTURE_FRAME*		m_pThumbpadFrame;

	BOOL					m_bBkgdRect;
	DWORD					m_dwBkgdRectColor;

	int						m_nNumChatChannels;
	CHAT_TYPE *				m_peChatChannels;

	// public methods
	UI_TEXTBOX(void);
	~UI_TEXTBOX(void);
};

struct UI_LISTBOX : UI_TEXTBOX
{
	int						m_nSelection;
	int						m_nHoverItem;
	DWORD					m_dwItemHighlightColor;
	DWORD					m_dwItemColor;
	DWORD					m_dwItemHighlightBKColor;
	float					m_fLineHeight;
	BOOL					m_bAutosize;

	UI_TEXTURE_FRAME*		m_pHighlightBarFrame;

	// public methods
	UI_LISTBOX(void);
};							

struct UI_COMBOBOX : UI_COMPONENT
{
	UI_LISTBOX*				m_pListBox;
	UI_LABELEX*				m_pLabel;
	UI_BUTTONEX*			m_pButton;

	// public methods
	UI_COMBOBOX(void);
};

struct AUTOMAP_ITEM
{
	int						m_nType;
	DWORD					m_idUnit;
	LEVELID					wCode;
	BOOL					m_bInHellrift; //Hellrift and non-hellrift areas have same levels, but diff coord space, so need to diff.
	VECTOR					m_vPosition;
	AUTOMAP_ITEM*			m_pNext;
	WCHAR					m_szName[128];
	BOOL					m_bBlocked;
	UI_TEXTURE_FRAME*		m_pIcon;
	DWORD					m_dwColor;
};

#define MAX_AUTOMAP_STRING (256)

struct UI_AUTOMAP : UI_PANELEX
{
	DWORD					m_dwCycleLastChange;

	float					m_fScale;
	float					m_fOverworldScale;
	int						m_nZoomLevel;
	AUTOMAP_ITEM*			m_pAutoMapItems;
	long					m_nAutoMapLen;
	VECTOR					m_vLastPosition;
	VECTOR					m_vLastFacing;
	float					m_fLastFacingAngle;
	float					m_fMonsterDistance;

	UI_TEXTURE_FRAME*		m_pFrameWarpIn;
	UI_TEXTURE_FRAME*		m_pFrameWarpOut;
	UI_TEXTURE_FRAME*		m_pFrameHero;
	UI_TEXTURE_FRAME*		m_pFrameTown;
	UI_TEXTURE_FRAME*		m_pFrameMonster;
	UI_TEXTURE_FRAME*		m_pFrameHellrift;
	UI_TEXTURE_FRAME*		m_pFrameQuestNew;
	UI_TEXTURE_FRAME*		m_pFrameQuestWaiting;
	UI_TEXTURE_FRAME*		m_pFrameQuestReturn;
	UI_TEXTURE_FRAME*		m_pFrameVendorNPC;
	UI_TEXTURE_FRAME*		m_pFrameHealerNPC;
	UI_TEXTURE_FRAME*		m_pFrameTrainerNPC;
	UI_TEXTURE_FRAME*		m_pFrameGuideNPC;
	UI_TEXTURE_FRAME*		m_pFrameCrafterNPC;
	UI_TEXTURE_FRAME*		m_pFrameMapVendorNPC;
	UI_TEXTURE_FRAME*		m_pFrameGrave;
	UI_TEXTURE_FRAME*		m_pGamblerNPC;
	UI_TEXTURE_FRAME*		m_pGuildmasterNPC;
	UI_TEXTURE_FRAME*		m_pFrameMapExit;


	BYTE					m_bIconAlpha;			
	BYTE					m_bWallsAlpha;
	BYTE					m_bTextAlpha;

	int						m_nLastLevel;
	int						m_nHellriftClass;
	WCHAR					m_szHellgateString[ MAX_AUTOMAP_STRING ];

	class RECT_ORGANIZE_SYSTEM*	m_pRectOrganize;

	// public methods
	UI_AUTOMAP(void);
};

struct UI_ITEMBOX : UI_PANELEX
{
	UI_TEXTURE_FRAME*		m_pFrameBackground;
	DWORD					m_dwColorQuantity;

	// public methods
	UI_ITEMBOX(void)		{ m_pFrameBackground = 0; m_dwColorQuantity = 0; }
};

																						// 0 - canceled, 1 - accepted, 2 - selected
typedef void (*PFN_COLORPICKER_CALLBACK_FUNC)(DWORD dwColorChosen, int nIndexChosen, BOOL bResult, DWORD dwData);	
struct UI_COLORPICKER : UI_COMPONENT
{
	UI_TEXTURE_FRAME*		m_pCellFrame;
	UI_TEXTURE_FRAME*		m_pCellUpFrame;
	UI_TEXTURE_FRAME*		m_pCellDownFrame;
	int						m_nCurrentColor;

	float					m_fGridOffsetX;
	float					m_fGridOffsetY;
	float					m_fCellSize;
	float					m_fCellSpacing;
	int						m_nGridRows;
	int						m_nGridCols;
	DWORD*					m_pdwColors;
	int						m_nSelectedIndex;
	int						m_nOriginalIndex;

	PFN_COLORPICKER_CALLBACK_FUNC m_pfnCallback;
	DWORD					m_dwCallbackData;

	// public methods
	UI_COLORPICKER(void);
};

struct UI_XML_COMPONENT; 
typedef BOOL (*PFN_XML_LOAD_FUNC)(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
typedef void (*PF_COMPONENTFREE)(UI_COMPONENT* component);
typedef UI_COMPONENT* (*PF_COMPONENTMALLOC)(UI_XML_COMPONENT* pComponentType);

// component types
struct UI_XML_COMPONENT
{
	const char *		m_szComponentName;
	int					m_nAllocSize;
	int					m_eComponentType;
	PFN_XML_LOAD_FUNC	m_fpXmlLoadFunc;
	PF_COMPONENTMALLOC	m_fpComponentMalloc;
	PF_COMPONENTFREE	m_fpComponentFreeData;
	PF_COMPONENTFREE	m_fpComponentFree;
	PFN_UIMSG_HANDLER	m_pDefaultUiMsgHandlerTbl[NUM_UI_MESSAGES];
	int					m_nCount;
};

// Default UI_COMPONENT allocator
template <typename T>
UI_COMPONENT* UIAllocComponent(UI_XML_COMPONENT * def)
{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	return (UI_COMPONENT*) MALLOC_NEW(g_StaticAllocator, T);
#else
	return (UI_COMPONENT*) MALLOC_NEW(NULL, T);
#endif
}

// Default UI_COMPONENT free/delete
template <typename T>
void UIFreeComponent(UI_COMPONENT * component)
{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	FREE_DELETE(g_StaticAllocator, component, T);
#else
	FREE_DELETE(NULL, component, T);
#endif
}


#define UI_REGISTER_COMPONENT( _ct, _nt, _compname, _lf, _ff )	{ pComponentTypes[_nt].m_szComponentName = _compname; \
													  pComponentTypes[_nt].m_nAllocSize = sizeof(_ct); \
													  pComponentTypes[_nt].m_eComponentType = _nt; \
													  pComponentTypes[_nt].m_fpComponentMalloc = UIAllocComponent<_ct>; \
													  pComponentTypes[_nt].m_fpXmlLoadFunc = _lf; \
													  pComponentTypes[_nt].m_fpComponentFreeData = _ff; \
													  pComponentTypes[_nt].m_fpComponentFree = UIFreeComponent<_ct>; \
													  memclear(pComponentTypes[_nt].m_pDefaultUiMsgHandlerTbl, sizeof(pComponentTypes[_nt].m_pDefaultUiMsgHandlerTbl)); }

#define UIMAP(type, msg, func)			{ pComponentTypes[type].m_pDefaultUiMsgHandlerTbl[msg] = func; }

struct UI_XML_ONFUNC
{
	const char *		m_szFunctionName;
	PFN_UIMSG_HANDLER	m_fpFunctionPtr;
};

//----------------------------------------------------------------------------
// CASTING FUNCTIONS
//----------------------------------------------------------------------------
#define CAST_FUNC(type, strct, name)		inline strct* UICastTo##name(UI_COMPONENT* component) \
												{ ASSERT_RETNULL(component); ASSERT_RETNULL(component->m_eComponentType == type); \
												return (strct*)component; } \
											inline BOOL UIComponentIs##name(UI_COMPONENT* component) \
											{ ASSERT_RETNULL(component); return (component->m_eComponentType == type); }

#define CAST_FUNC2(type1, type2, strct, name)	inline strct* UICastTo##name(UI_COMPONENT* component) \
												{ ASSERT_RETNULL(component); ASSERT_RETNULL(component->m_eComponentType == type1 || component->m_eComponentType == type2); \
												return (strct*)component; }	\
												inline BOOL UIComponentIs##name(UI_COMPONENT* component) \
												{ ASSERT_RETNULL(component); return (component->m_eComponentType == type1 || component->m_eComponentType == type2); }

#define CAST_FUNC3(type1, type2, type3, strct, name)	inline strct* UICastTo##name(UI_COMPONENT* component) \
												{ ASSERT_RETNULL(component); ASSERT_RETNULL(component->m_eComponentType == type1 || component->m_eComponentType == type2 || component->m_eComponentType == type3); \
												return (strct*)component; }	\
												inline BOOL UIComponentIs##name(UI_COMPONENT* component) \
												{ ASSERT_RETNULL(component); return (component->m_eComponentType == type1 || component->m_eComponentType == type2 || component->m_eComponentType == type3); }

CAST_FUNC(UITYPE_SCREEN, UI_SCREENEX, Screen);
CAST_FUNC(UITYPE_CURSOR, UI_CURSOR, Cursor);
CAST_FUNC3(UITYPE_LABEL, UITYPE_STATDISPL, UITYPE_EDITCTRL, UI_LABELEX, Label);
CAST_FUNC3(UITYPE_PANEL, UITYPE_AUTOMAP, UITYPE_ITEMBOX, UI_PANELEX, Panel);
CAST_FUNC(UITYPE_BUTTON, UI_BUTTONEX, Button);
CAST_FUNC(UITYPE_BAR, UI_BAR, Bar);
CAST_FUNC(UITYPE_SCROLLBAR, UI_SCROLLBAR, ScrollBar);
CAST_FUNC(UITYPE_TOOLTIP, UI_TOOLTIP, Tooltip);
CAST_FUNC(UITYPE_CONVERSATION_DLG, UI_CONVERSATION_DLG, ConversationDlg);
CAST_FUNC2(UITYPE_TEXTBOX, UITYPE_LISTBOX, UI_TEXTBOX, TextBox);
CAST_FUNC(UITYPE_LISTBOX, UI_LISTBOX, ListBox);
CAST_FUNC(UITYPE_FLEXBORDER, UI_FLEXBORDER, FlexBorder);
CAST_FUNC(UITYPE_FLEXLINE, UI_FLEXLINE, FlexLine);
CAST_FUNC(UITYPE_MENU, UI_MENU, Menu);
CAST_FUNC(UITYPE_COMBOBOX, UI_COMBOBOX, ComboBox);

CAST_FUNC(UITYPE_STATDISPL, UI_STATDISPL, StatDispL);
CAST_FUNC(UITYPE_AUTOMAP, UI_AUTOMAP, AutoMap);
CAST_FUNC(UITYPE_ITEMBOX, UI_ITEMBOX, ItemBox);

CAST_FUNC(UITYPE_EDITCTRL, UI_EDITCTRL, EditCtrl);
CAST_FUNC(UITYPE_COLORPICKER, UI_COLORPICKER, ColorPicker);
//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

const UI_COMPONENT_DATA *UIComponentGetData(
	int nUICompIndex);
	
void UI_TB_MouseToScene( 
	GAME * pGame, 
	VECTOR *pvOrig, 
	VECTOR *pvDir,
	BOOL bCenter = FALSE );

void UITextBoxClear(
	int idComponent);

void UITextBoxClear(
	UI_COMPONENT* component);

void UITextBoxResize(
	UI_COMPONENT* component);

void UITextBoxAddLine(
	int idComponent,
	const WCHAR* str,
	DWORD color = GFXCOLOR_WHITE,
	QWORD nData = 0);

void UITextBoxAddLine(
	UI_TEXTBOX* textbox,
	const WCHAR* str,
	DWORD color = GFXCOLOR_WHITE,
	QWORD nData = 0,
	UI_LINE* pOriginLine = NULL,
	BOOL bIgnoreHypertext = FALSE );

void UITextBoxResetFadeout(
	UI_TEXTBOX *pTextBox);

void UILabelSetTextByStringKey(
	UI_COMPONENT *pComponent,
	const char *pszStringKey);

void UILabelSetTextByStringIndex(
	UI_COMPONENT *pComponent,
	int nStringIndex);

void UILabelSetTextByGlobalString(
	UI_COMPONENT *pComponent,
	GLOBAL_STRING eGlobalString);

void UILabelSetText(
	UI_COMPONENT* component,
	const WCHAR* str,
	BOOL bCheckDifferent = FALSE);

void UILabelSetText(
	int idComponent,
	const WCHAR* str,
	BOOL bCheckDifferent = FALSE);

const WCHAR* UILabelGetText(
	UI_COMPONENT* component);

void UILabelSetTextByEnum( 
	UI_COMPONENT_ENUM eType, 
	const WCHAR *puszName);

const WCHAR* UILabelGetTextByEnum( 
	UI_COMPONENT_ENUM eType);

void UILabelSetTextKeyByEnum( 
	UI_COMPONENT_ENUM eType, 
	const char *pszKey);

void UILabelDoWordWrap(
	UI_LABELEX* label);

UNITID UIGetCursorUnit(
	void);

BOOL UICursorUnitIsReference(
	void);

int UIGetCursorSkill(
	void);

BOOL UICursorIsEmpty(
	void);

void UIClearCursor(
	void);

void UIClearCursorUnitReferenceOnly(
	void);

BOOL UISetCursorUnit(
	UNITID idCursor,
	BOOL bSendMouseMove,
	int nFromWeaponConfig = -1,
	float fOffsetPctX = 0.0f,
	float fOffsetPctY = 0.0f,
	BOOL bForceReference = FALSE);

void UISetCursorSkill(
	int nSkillID,
	BOOL bSendMouseMove);

void UISetCursorUseUnit(
	UNITID idUnit);

UNITID UIGetCursorUseUnit(
	void);
	
int UIListBoxGetSelectedIndex(
	UI_LISTBOX *pListBox);

int UIListBoxGetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szListBoxName);

int UIComboBoxGetSelectedIndex(
	UI_COMBOBOX *pComboBox);

int UIComboBoxGetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szComboBoxName);

QWORD UIListBoxGetSelectedData(
	UI_LISTBOX *pListBox);

QWORD UIComboBoxGetSelectedData(
	UI_COMBOBOX *pComboBox);

void UIListBoxSetSelectedIndex(
	UI_LISTBOX *pListBox,
	int nItem,
	BOOL bClose = TRUE,
	BOOL bSendSelectMessage = TRUE);

void UIListBoxSetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szListBoxName,
	int nItem,
	BOOL bClose = TRUE,
	BOOL bSendSelectMessage = TRUE);

void UIComboBoxSetSelectedIndex(
	UI_COMBOBOX *pComboBox,
	int nItem,
	BOOL bClose = TRUE,
	BOOL bSendSelectMessage = TRUE );

void UIComboBoxSetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szComboBoxName,
	int nItem,
	BOOL bClose = TRUE,
	BOOL bSendSelectMessage = TRUE );

float UIScrollBarGetValue(
	UI_SCROLLBAR *pScrollBar,
	BOOL bMappedToRange = FALSE );

float UIScrollBarGetValue(
	UI_COMPONENT *pParent,
	const char *szScrollbarName,
	BOOL bMappedToRange = FALSE );

void UIScrollBarSetValue(
	UI_SCROLLBAR *pScrollBar,
	float fValue,
	BOOL bMappedToRange = FALSE );

void UIScrollBarSetValue(
	UI_COMPONENT *pParent,
	const char *szScrollbarName,
	float fValue,
	BOOL bMappedToRange = FALSE );

void UIColorPickerShow(
	UI_COMPONENT *pNextToComponent,
	DWORD *pdwColors,
	PFN_COLORPICKER_CALLBACK_FUNC pfnCallback,
	int nSelected);

// grrr - from uix.h
UI_MSG_RETVAL UIHandleUIMessage(
	UINT msg,
	WPARAM wParam,
	LPARAM lParam);

void UIResizeAll(
	UI_COMPONENT* component,
	BOOL bWordWrap = FALSE);

void UIResizeComponentByDelta(
	UI_COMPONENT* pResizeComp,
	UI_POSITION& posDelta);

BOOL UIComponentIsResizing(
	void);

UI_MSG_RETVAL UIComponentResize(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UISlashCommand(
	GLOBAL_STRING eGlobalString,
	WCHAR *puszBuffer,
	int nBufferSize);

void UISlashCommandSetInput(
	GLOBAL_STRING eGlobalString);

//----------------------------------------------------------------------------
// Inlines
//----------------------------------------------------------------------------
inline UI_POSITION UIComponentGetScrollPos(UI_COMPONENT *component)
{
	if (component->m_eComponentType == UITYPE_SCROLLBAR)		// TODO: need to fix this and move it back to uix_priv.h
	{
		return UI_POSITION();
	}
	return component->m_ScrollPos;
}

//----------------------------------------------------------------------------
inline BOOL UICursorGetVisible(
	void)
{
	return UIComponentGetVisible(g_UI.m_Cursor);
}


//----------------------------------------------------------------------------
inline BOOL UICursorGetActive(
	void)
{
	return UIComponentGetActive(g_UI.m_Cursor);
}

//----------------------------------------------------------------------------

inline void UICursorSetPosition(
	float fX,
	float fY)
{
	if (!g_UI.m_Cursor)
	{
		return;
	}
	g_UI.m_Cursor->m_Position.m_fX = fX * UIGetScreenToLogRatioX();
	g_UI.m_Cursor->m_Position.m_fY = fY * UIGetScreenToLogRatioY();
	UIRound(g_UI.m_Cursor->m_Position);

	static TIME tiLastUpdate;
	TIME tiCurrent = AppCommonGetAbsTime();
	if (tiCurrent > tiLastUpdate)
	{
		tiLastUpdate = tiCurrent;
		UIHandleUIMessage(UIMSG_MOUSEMOVE, 0, MAKE_PARAM((int)fX, (int)fY));
	}
	UISetNeedToRender(g_UI.m_Cursor->m_nRenderSection);
	if (g_UI.m_Cursor->m_pFirstChild)
		UISetNeedToRender(g_UI.m_Cursor->m_pFirstChild->m_nRenderSection);
}

//----------------------------------------------------------------------------
inline BOOL UIGetCursorPosition(
	int* pnX,
	int* pnY, 
	BOOL bLogical = TRUE)
{
	*pnX = 0;
	*pnY = 0;
	if (!g_UI.m_Cursor)
	{
		return FALSE;
	}
	if (!UICursorGetVisible())
	{
		return FALSE;
	}
	*pnX = (int)(g_UI.m_Cursor->m_Position.m_fX * (bLogical ? 1.0f : UIGetLogToScreenRatioX()));
	*pnY = (int)(g_UI.m_Cursor->m_Position.m_fY * (bLogical ? 1.0f : UIGetLogToScreenRatioY()));
	return TRUE;
}

//----------------------------------------------------------------------------
inline BOOL UIGetCursorPosition(
	float* fX,
	float* fY,
	BOOL bLogical = TRUE)
{
	if (!g_UI.m_Cursor)
	{
		return FALSE;
	}
	if (!UICursorGetVisible())
	{
		return FALSE;
	}
	*fX = g_UI.m_Cursor->m_Position.m_fX * (bLogical ? 1.0f : UIGetLogToScreenRatioX());
	*fY = g_UI.m_Cursor->m_Position.m_fY * (bLogical ? 1.0f : UIGetLogToScreenRatioY());
	return TRUE;
}

//----------------------------------------------------------------------------
inline BOOL UIElementCheckBounds(
	UI_GFXELEMENT *pElement,
	float x,
	float y)
{
	if (!pElement)
		return FALSE;

	x += pElement->m_fXOffset;
	y += pElement->m_fYOffset;
	return (x >= pElement->m_Position.m_fX &&
			y >= pElement->m_Position.m_fY &&
			x < pElement->m_Position.m_fX + pElement->m_fWidth &&
			y < pElement->m_Position.m_fY + pElement->m_fHeight);
}

inline BOOL UIComponentCheckBounds(
	UI_COMPONENT* component,
	float x,
	float y,
	UI_POSITION* posReturn = NULL)
{
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(component);

	UI_RECT componentrect = UIComponentGetRect(component);

	if (posReturn)
	{
		*posReturn = pos;
	}

	if (x <	 pos.m_fX + componentrect.m_fX1 || 
		x >= pos.m_fX + componentrect.m_fX2 ||
		y <  pos.m_fY + componentrect.m_fY1 ||
		y >= pos.m_fY + componentrect.m_fY2 )
	{
		return FALSE;
	}

	if (!component->m_pFrame || !component->m_pFrame->m_bHasMask)
	{
		return TRUE;
	}

	if (component->m_pFrame->m_pbMaskBits == NULL)
	{
		return FALSE;
	}

	int ii = (int)((x - (pos.m_fX + componentrect.m_fX1)) / component->m_fXmlWidthRatio);
	int jj = (int)((y - (pos.m_fY + componentrect.m_fY1)) / component->m_fXmlHeightRatio);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	int nPixelWidth = (int)(component->m_pFrame->m_fWidth / pTexture->m_XMLWidthRatio);
	int nPixelHeight= (int)(component->m_pFrame->m_fHeight / pTexture->m_XMLHeightRatio);

	if (ii > nPixelWidth ||
		jj > nPixelHeight)
	{
		return FALSE;
	}

	int nByte = (ii + (jj * nPixelWidth)) / 8;
	int nBit  = (ii + (jj * nPixelHeight)) % 8;

	//BYTE *pbPos = component->m_pFrame->m_pbMaskBits;
	//pbPos += nByte;
	//BOOL bVal = ((*pbPos) & (1 << nBit)) != 0;
	//return bVal;
	return *(component->m_pFrame->m_pbMaskBits + nByte) & (1 << nBit);
}

inline BOOL UIComponentCheckBounds(
	UI_COMPONENT* component,
	UI_POSITION* posReturn = NULL)
{
	float x = 0.0f, y = 0.0f;
	return UIGetCursorPosition(&x, &y) && UIComponentCheckBounds(component, x, y, posReturn);
}

inline BOOL UIComponentCheckMask(
	UI_COMPONENT* component,
	float x,
	float y)
{
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(component);
	UI_RECT componentrect = UIComponentGetRect(component);

	if (x <	 pos.m_fX + componentrect.m_fX1 || 
		x >= pos.m_fX + componentrect.m_fX2 ||
		y <  pos.m_fY + componentrect.m_fY1 ||
		y >= pos.m_fY + componentrect.m_fY2 )
	{
		return FALSE;
	}

	if (!component->m_pFrame || !component->m_pFrame->m_bHasMask || !component->m_pFrame->m_pChunks)
	{
		return TRUE;
	}

	if (component->m_pFrame->m_pbMaskBits == NULL)
	{
		return FALSE;
	}

	int ii = (int)((x - (pos.m_fX + componentrect.m_fX1)) / component->m_fXmlWidthRatio);
	int jj = (int)((y - (pos.m_fY + componentrect.m_fY1)) / component->m_fXmlHeightRatio);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	int nPixelWidth = (int)(component->m_pFrame->m_fWidth / pTexture->m_XMLWidthRatio);
	int nPixelHeight= (int)(component->m_pFrame->m_fHeight / pTexture->m_XMLHeightRatio);

	if (ii > nPixelWidth ||
		jj > nPixelHeight)
	{
		return FALSE;
	}

	int nByte = (ii + (jj * nPixelWidth)) / 8;
	int nBit  = (ii + (jj * nPixelHeight)) % 8;

	BYTE *pbPos = component->m_pFrame->m_pbMaskBits;
	pbPos += nByte;
	BOOL bVal = ((*pbPos) & (1 << nBit)) != 0;
	return bVal;
//	return *(component->m_pFrame->m_pChunks[0].m_pdwMaskBits + nByte) & (1 << nBit);
}

inline BOOL UIComponentCheckMask(
	UI_COMPONENT* component)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	return UIComponentCheckMask(component, x, y);
}

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------
void UITooltipDetermineContents(
	UI_COMPONENT *pComponent);

void UITooltipDetermineSize(
	UI_COMPONENT *pComponent);

enum {
	TTPOS_RIGHT_TOP,
	TTPOS_RIGHT_BOTTOM,
	TTPOS_LEFT_TOP,
	TTPOS_LEFT_BOTTOM,
	TTPOS_BOTTOM_LEFT,
	TTPOS_BOTTOM_RIGHT,
	TTPOS_TOP_LEFT,
	TTPOS_TOP_RIGHT,
	TTPOS_CENTER,

	NUM_TTPOS,
};
BOOL UITooltipPosition(
	UI_TOOLTIP *pTooltip,
	UI_RECT *pRect = NULL,
	int nSuggestRelativePos = TTPOS_RIGHT_TOP,
	UI_RECT *pAvoidRect = NULL);
	
void InitComponentTypes(UI_XML_COMPONENT *pComponentTypes, UI_XML_ONFUNC*& pUIXmlFunctions, int& nXmlFunctionsSize);

BOOL UIPanelSetTab(
	UI_PANELEX *pPanel,
	int nTab);

UI_PANELEX * UIPanelGetTab(
	UI_PANELEX *pPanel,
	int nTab);

BOOL UIButtonGetDown(
	UI_BUTTONEX * pButton);

BOOL UIButtonGetDown(
	UI_COMPONENT *pParent,
	const char *szButtonName);

void UIButtonSetDown(
	UI_BUTTONEX * pButton,
	BOOL bDown);

void UIButtonSetDown(
	UI_COMPONENT *pParent,
	const char *szButtonName,
	BOOL bDown);

void UIButtonBlinkHighlight(
	UI_BUTTONEX *pButton, 
	int *pnDuration = NULL, 
	int *pnPeriod = NULL);

void UIComponentDrawFlexFrames(
	UI_COMPONENT* pComponent,
	UI_TEXTURE_FRAME *pStartFrame,
	UI_TEXTURE_FRAME *pMidFrame,
	UI_TEXTURE_FRAME *pEndFrame,
	BOOL bHorizontal = TRUE,
	UI_SIZE *pSize = NULL);

void UIListBoxAddString(
	UI_LISTBOX *pListBox,
	const WCHAR *szString,
	const QWORD nData = 0);

UI_MSG_RETVAL UIQuestsPanelResizeScrollbar( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

void UIEditCtrlSetFocus(
	UI_EDITCTRL *pEditCtrl);

BOOL UIScrollBarCheckBounds(
	UI_SCROLLBAR *pScrollBar);

void UILabelStartSlowReveal(
	UI_COMPONENT *pComponent);

BOOL UILabelDoRevealAllText(
	UI_COMPONENT *pComponent);

void UIEditPasteString(
	   const WCHAR *string );

BOOL UIIsImeWindowsActive();

BOOL UILabelOnAdvancePage(
	UI_COMPONENT* component,
	int nDelta);

void UIComponentRemoveAutoMapItems(
	UI_AUTOMAP* automap);

BOOL UILabelIsSelected(			
	UI_COMPONENT* pComponent);

BOOL UILabelIsSelected(			
	UI_COMPONENT* pComponent,
	const char *szChildName);

// -- Load functions ----------------------------------------------------------
BOOL UIXmlLoadScreen	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadCursor	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadLabel		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadPanel		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadButton	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadBar		(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadScrollBar	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadTooltip	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadTextBox	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadFlexBorder(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadFlexLine	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadStatDispL	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadAutoMap	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadItemBox	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadListBox	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
BOOL UIXmlLoadComboBox	(CMarkup& xml, UI_COMPONENT* component, const UI_XML & tUIXml);
	
// -- Free functions ----------------------------------------------------------
void UIComponentFreeLabel	(UI_COMPONENT* component);
void UIComponentFreeTextBox	(UI_COMPONENT* component);
void UIComponentFreeAutoMap	(UI_COMPONENT* component);

// -- Exported message handler functions --------------------------------------
UI_MSG_RETVAL  UIButtonOnMouseOver	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIButtonOnButtonDown	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIButtonOnButtonUp	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UILabelOnPaint		(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIAutoMapOnPaint		(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIAutoMapOnActivate	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIPanelOnInactivate	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIPanelOnActivate	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIBarOnPaint			(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIScrollBarOnScroll	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UITooltipOnPaint		(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIEditCtrlOnKeyChar	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);
UI_MSG_RETVAL  UIWindowshadeButtonOnClick(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);	// CHB 2007.02.02
UI_MSG_RETVAL  UILabelSelect		(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);	
UI_MSG_RETVAL  UIMsgPassToParent	(UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam);	

#endif
