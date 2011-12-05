//----------------------------------------------------------------------------
// uix_components.cpp
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
#include "uix_chat.h"
#include "dictionary.h"
#include "markup.h"
#include "stringtable.h"
#include "excel.h"
#include "prime.h"
#include "game.h"
#include "globalIndex.h"
#include "monsters.h"
#include "complexmaths.h"
#include "c_camera.h"
#include "config.h"
#include "skills.h"
#include "pets.h"
#include "states.h"
#include "perfhier.h"
#include "windowsmessages.h"
#include "script.h"
#include "items.h"
#include "objects.h"
#include "c_font.h"
#include "fontcolor.h"
#include "c_trade.h"
#include "c_message.h"
#include "RectOrganizer.h"
#include "c_imm.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "imports.h"
#include "uix_components_complex.h"
#include "uix_components_hellgate.h"
#include "c_camera.h"
#include "camera_priv.h"
#include "e_settings.h"
#include "e_main.h"
#include "console_priv.h"
#include "party.h"
#include "teams.h"
#include "chatserver.h"
#include "unitmodes.h"
#include "unittag.h"
#include "combat.h"
#include "console.h"
#include "weaponconfig.h"
#include "language.h"
#include "e_automap.h"
#include "gameglobals.h"
#include "duel.h"
#include "common/resource.h"
#include "gameoptions.h"
#include "uix_hypertext.h"
#include "LevelZones.h"
#include "pointsofinterest.h"
#include "c_questclient.h"
#if !ISVERSION(SERVER_VERSION)
#include "c_input.h"
#include "chat.h"
#include "svrstd.h"
#include "UserChatCommunication.h"
#include "c_chatNetwork.h"

#define AUTOMAP_RADIUS 120.0f
#define AUTOMAP_RADIUS_TUGBOAT 100.0f
using namespace FSSE;

//----------------------------------------------------------------------------
// Forwards
//----------------------------------------------------------------------------
void UIAutoMapDisplayUpdate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD);

static void sUIEditCtrlSetCaret(
	UI_EDITCTRL* pEditCtrl);

static float sUIEditCtrlGetLineSize(
	UI_EDITCTRL *pEditCtrl);

static void sUIEditCtrlPositionCaretAt(
	UI_EDITCTRL *pEditCtrl,
	float x,
	float y);

UI_MSG_RETVAL UIWindowshadeButtonOnClick( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum
{
	UI_BUTTONSTATE_HOVER =			MAKE_MASK(0),
	UI_BUTTONSTATE_DOWN =			MAKE_MASK(1),		
};

enum 
{
	FLEX_SIB_NONE = 0,
	FLEX_SIB_PREV,
	FLEX_SIB_NEXT,
	FLEX_SIB_NEXT_ANY,
	FLEX_SIB_PREV_ANY,
};

static const float MAP_ICON_SCALE_TUGBOAT = 1.0f;
static const float MAP_ICON_SCALE_HELLGATE = 1.0f;

//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
extern STR_DICT tooltipareas[];

STR_DICT pMarqueeEnumTbl[] =
{
	{ "",		MARQUEE_NONE },
	{ "none",	MARQUEE_NONE },
	{ "auto",	MARQUEE_AUTO },
	{ "horiz",	MARQUEE_HORIZ },
	{ "vert",	MARQUEE_VERT },

	{ NULL,		MARQUEE_NONE },
};

struct LABEL_UNITMODE
{
	UNITMODE	mode;
	WORD		wParam;
	float		fDurationInSeconds;
	BOOL		bEnd;
	int			nPage;
};

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

static UI_EDITCTRL*	gpsLastActiveEditCtrl = NULL;		// Used to track the which editctrl we're using right now.

//----------------------------------------------------------------------------
// UI_COMPONENT and child class constructors
//----------------------------------------------------------------------------

UI_COMPONENT::UI_COMPONENT(void)
: m_idComponent(0)
, m_pNextSibling(NULL)
, m_pPrevSibling(NULL)
, m_pParent(NULL)
, m_pFirstChild(NULL)
, m_pLastChild(NULL)
, m_eComponentType(0)
, m_eState(0)
, m_bVisible(FALSE)
, m_bIndependentActivate(FALSE)
, m_bUserActive(FALSE)
, m_fZDelta(0.0f)
, m_fWidth(0.0f)
, m_fHeight(0.0f)
, m_fWidthParentRel(0.0f)
, m_fHeightParentRel(0.0f)
, m_ActivePos(0.0f, 0.0f)
, m_InactivePos(0.0f, 0.0f)
, m_SizeMin(0.0f, 0.0f)
, m_SizeMax(0.0f, 0.0f)
, m_ScrollPos(0.0f, 0.0f)
, m_ScrollMin(0.0f, 0.0f)
, m_ScrollMax(0.0f, 0.0f)
, m_fWheelScrollIncrement(0.0f)
, m_posFrameScroll(0.0f, 0.0f)
, m_fFrameScrollSpeed(0.0f)
, m_bScrollFrameReverse(FALSE)
, m_fScrollHorizMax(0.0f)
, m_fScrollVertMax(0.0f)
, m_bScrollsWithParent(FALSE)
, m_fXmlWidthRatio(0.0f)
, m_fXmlHeightRatio(0.0f)
, m_bNeedsRepaintOnVisible(FALSE)
, m_bNoScale(FALSE)
, m_bNoScaleFont(FALSE)
, m_bStretch(FALSE)
, m_bDynamic(FALSE)
, m_bIgnoresMouse(FALSE)
, m_nAngle(0)
, m_rectClip(0.0f, 0.0f, 0.0f, 0.0f)
, m_bGrayInGhost(FALSE)
, m_pTexture(NULL)
, m_pFrame(NULL)
, m_pFrameAnimInfo(NULL)
, m_dwFramesAnimTicket(0)
, m_pFont(0)
, m_nFontSize(0)
, m_dwColor(0)
, m_bTileFrame(FALSE)
, m_fTilePaddingX(0.0f)
, m_fTilePaddingY(0.0f)
, m_idFocusUnit(0)
, m_bHasOwnFocusUnit(FALSE)
, m_dwParam(0)
, m_dwParam2(0)
, m_dwParam3(0)
, m_dwData(0)
, m_qwData(0)
, m_pBlinkData(NULL)
, m_codeStatToVisibleOn(0)
, m_pData(NULL)
, m_szTooltipText(NULL)
, m_bHoverTextWithMouseDown(FALSE)
, m_nTooltipLine(0)
, m_nTooltipOrder(0)
, m_nTooltipJustify(0)
, m_fToolTipPaddingX(0.0f)
, m_fToolTipPaddingY(0.0f)
, m_nTooltipCrossesLines(0)
, m_bTooltipNoSpace(FALSE)
, m_fToolTipXOffset(0.0f)
, m_fToolTipYOffset(0.0f)
, m_bFadesIn(FALSE)
, m_bFadesOut(FALSE)
, m_fFading(0.0f)
, m_AnimStartPos(0.0f, 0.0f)
, m_AnimEndPos(0.0f, 0.0f)
, m_dwAnimDuration(0)
, m_bAnimatesWhenPaused(FALSE)
, m_nRenderSection(0)
, m_nRenderSectionOffset(0)
, m_nModelID(0)
, m_nAppearanceDefID(0)
, m_pGfxElementFirst(NULL)
, m_pGfxElementLast(NULL)
, m_bReference(FALSE)
, m_nResizeAnchor(UIALIGN_TOPLEFT)
, m_nAnchorParent(UIALIGN_TOPLEFT)
, m_fAnchorXoffs(0.0f)
, m_fAnchorYoffs(0.0f)
, m_pAnimRelationships(NULL)
, m_pReplacedComponent(NULL)
{
	m_szName[0] = 0;

	m_listMsgHandlers.Init();
}

UI_PANELEX::UI_PANELEX(void)
: UI_COMPONENT()
, m_nCurrentTab(0)
, m_nTabNum(0)
, m_fModelRotation(0.0f)
, m_nModelCam(0)
, m_bMouseDown(FALSE)
, m_posMouseDown(0.0f, 0.0f)
, m_pHighlightFrame(NULL)
, m_pResizeComponent(NULL)
{
}

UI_CURSOR::UI_CURSOR(void)
: UI_COMPONENT()
, m_eCursorState(UICURSOR_STATE_POINTER)
, m_idUnit(0)
, m_nFromWeaponConfig(0)
, m_nSkillID(0)
, m_idUseItem(0)
, m_nLastInvLocation(0)
, m_nLastInvX(0)
, m_nLastInvY(0)
, m_idLastInvContainer(0)
, m_nItemPickActivateSkillID(0)
, m_fItemDrawWidth(0.0f)
, m_fItemDrawHeight(0.0f)
{
	memset(m_pCursorFrame, 0, NUM_UICURSOR_STATES * sizeof(UI_TEXTURE_FRAME*));
	memset(m_pCursorFrameTexture, 0, NUM_UICURSOR_STATES * sizeof(UIX_TEXTURE*));
}

UI_LABELEX::UI_LABELEX() 
: UI_COMPONENT()
, m_nStringIndex(0)
, m_bTextHasKey(FALSE)
, m_pTextElement(NULL)
, m_nAlignment(0)
, m_bAutosize(FALSE)
, m_bAutosizeHorizontalOnly(FALSE)
, m_bAutosizeVerticalOnly(FALSE)
, m_bWordWrap(FALSE)
, m_bForceWrap(FALSE)
, m_bSlowReveal(FALSE)
, m_fMaxWidth(0.0f)
, m_fMaxHeight(0.0f)
, m_bUseParentMaxWidth(FALSE)
, m_bSelected(FALSE)
, m_dwSelectedColor(0)
, m_bDisabled(FALSE)
, m_dwDisabledColor(0)
, m_nMenuOrder(0)
, m_eMarqueeMode(MARQUEE_NONE)
, m_eMarqueeState(MARQUEE_STATE_NONE)
, m_dwMarqueeAnimTicket(0)
, m_fMarqueeIncrement(0.0f)
, m_nMarqueeEndDelay(0)
, m_nMarqueeStartDelay(0)
, m_posMarquee(0.0f, 0.0f)
, m_fTextWidth(0.0f)
, m_fTextHeight(0.0f)
, m_nPage(0)
, m_nNumPages(0)
, m_bDrawDropshadow(FALSE)
, m_dwDropshadowColor(0)
, m_fFrameOffsetX(0.0f)
, m_fFrameOffsetY(0.0f)
, m_nTextToRevealLength(0)
, m_nMaxChars(0)
, m_nNumCharsToAddEachUpdate(0)
, m_dwUpdateEvent(0)
, m_pScrollbar(NULL)
, m_bHideInactiveScrollbar(FALSE)
, m_fScrollbarExtraSpacing(0.0f)
, m_pUnitmodes(NULL)
, m_nUnitmodesAlloc(0)
, m_bAutosizeFont(FALSE)
, m_bPlayUnitModes(FALSE)
, m_bBkgdRect(FALSE)
, m_dwBkgdRectColor(0)
{
#if ISVERSION(DEVELOPMENT)
	m_szTextKey[0] = 0;
#endif
};

UI_STATDISPL::UI_STATDISPL(void)
: UI_LABELEX()
, m_nDisplayLine(0)
, m_nDisplayArea(0)
, m_fSpecialSpacing(0.0f)
, m_bShowIcons(FALSE)
, m_bVertIcons(FALSE)
, m_pFont2(NULL)
, m_nFontSize2(0)
, m_nUnitType(0)
{
}

UI_MENU::UI_MENU(void)
: UI_COMPONENT()
, m_pSelectedChild(NULL)
, m_bAlwaysOneSelected(FALSE)
{
}

UI_EDITCTRL::UI_EDITCTRL(void)
: UI_LABELEX()
, m_bHasFocus(FALSE)
, m_bStartsWithFocus(FALSE)
, m_nCaretPos(0)
, m_CaretPos(0.0f, 0.0f)
, m_dwPaintTicket(0)
, m_nMaxLen(0)
, m_bPassword(FALSE)
, m_bLimitTextToSpace(FALSE)
, m_bMultiline(FALSE)
, m_bEatsAllKeys(FALSE)
, m_bAllowIME(FALSE)
, m_pNextTabCtrl(NULL)
, m_pPrevTabCtrl(NULL)
{
	m_szOnlyAllowChars[0] = 0;
	m_szDisallowChars[0] = 0;
	m_szNextTabCtrlName[0] = 0;
};


UI_BUTTONEX::UI_BUTTONEX(void)
: UI_COMPONENT()
, m_pLitFrame(NULL)
, m_pDownFrame(NULL)
, m_pInactiveFrame(NULL)
, m_pInactiveDownFrame(NULL)
, m_pDownLitFrame(NULL)
, m_bOverlayLitFrame(FALSE)
, m_eButtonStyle(UIBUTTONSTYLE_PUSHBUTTON)
, m_dwPushstate(0)
, m_nRadioParentLevel(0)
, m_nAssocStat(0)
, m_nAssocTab(0)
, m_fScrollIncrement(0.0f)
, m_pScrollControl(NULL)
, m_bHideOnScrollExtent(FALSE)
, m_dwRepeatMS(0)
, m_nCheckOnSound(0)
, m_nCheckOffSound(0)
, m_pFrameMid(NULL)
, m_pFrameLeft(NULL)
, m_pFrameRight(NULL)
, m_pLitFrameMid(NULL)
, m_pLitFrameLeft(NULL)
, m_pLitFrameRight(NULL)
, m_pDownFrameMid(NULL)
, m_pDownFrameLeft(NULL)
, m_pDownFrameRight(NULL)
, m_pInactiveFrameMid(NULL)
, m_pInactiveFrameLeft(NULL)
, m_pInactiveFrameRight(NULL)
, m_pDownLitFrameMid(NULL)
, m_pDownLitFrameLeft(NULL)
, m_pDownLitFrameRight(NULL)
{  
}

UI_BAR::UI_BAR(void)
: UI_COMPONENT()
, m_codeCurValue(0)
, m_codeMaxValue(0)
, m_codeRegenValue(0)
, m_nRegenStat(0)
, m_nIncreaseLeftStat(0)
, m_nBlinkThreshold(0)
, m_bEndCap(FALSE)
, m_fSlantWidth(0.0f)
, m_nOrientation(0)
, m_nOldValue(0)
, m_nOldMax(0)
, m_nShowIncreaseDuration(0)
, m_bShowTempRegen(FALSE)
, m_nMaxValue(0)
, m_nValue(0)
, m_nMinValue(0)
, m_bRadial(FALSE)
, m_fMaxAngle(0.0f)
, m_bCounterClockwise(FALSE)
, m_fMaskAngleStart(0.0f)
, m_fMaskBaseAngle(0.0f)
, m_dwAltColor(0)
, m_dwAltColor2(0)
, m_dwAltColor3(0)
, m_bUseAltColor(FALSE)
, m_nStat(0)
{
}

UI_SCROLLBAR::UI_SCROLLBAR(void)
: UI_COMPONENT()
, m_bOrientation(FALSE)
, m_bSendPaintToParent(FALSE)
, m_fMin(0.0f)
, m_fMax(0.0f)
, m_fValue(0.0f)
, m_nOrientation(0)
, m_pBar(NULL)
, m_pThumbpadFrame(NULL)
, m_bMouseDownOnThumbpad(FALSE)
, m_posMouseDownOffset(0.0f, 0.0f)
, m_pThumbpadElement(NULL)
, m_pScrollControl(NULL)
{
}

UI_FLEXBORDER::UI_FLEXBORDER(void)
: UI_COMPONENT()
, m_pFrameTL(NULL)
, m_pFrameTM(NULL)
, m_pFrameTR(NULL)
, m_pFrameML(NULL)
, m_pFrameMM(NULL)
, m_pFrameMR(NULL)
, m_pFrameBL(NULL)
, m_pFrameBM(NULL)
, m_pFrameBR(NULL)
, m_bAutosize(FALSE)
, m_fBorderSpace(0.0f)
, m_fBorderScale(0.0f)
, m_bStretchBorders(FALSE)
{
}

UI_FLEXLINE::UI_FLEXLINE(void)
: UI_COMPONENT()
, m_bHorizontal(FALSE)
, m_pFrameStartCap(NULL)
, m_pFrameEndCap(NULL)
, m_pFrameLine(NULL)
, m_nVisibleOnSibling(0)
, m_bAutosize(FALSE)
{
}

UI_TOOLTIP::UI_TOOLTIP(void)
: UI_COMPONENT()
, m_fMinWidth(0.0f)
, m_fMaxWidth(0.0f)
, m_bAutosize(FALSE)
, m_fXOffset(0.0f)
, m_fYOffset(0.0f)
, m_bCentered(FALSE)
{
}

UI_CONVERSATION_DLG::UI_CONVERSATION_DLG(void)
: UI_COMPONENT()
, m_nDialog(0)
, m_bLeaveUIOpenOnOk(FALSE)
, m_nQuest(0)
{
	memset(m_idTalkingTo, 0, sizeof(UNITID) * MAX_CONVERSATION_SPEAKERS);
}

UI_TEXTBOX::UI_TEXTBOX(void)
: UI_COMPONENT()
, m_nMaxLines(0)
, m_bBottomUp(FALSE)
, m_bWordWrap(FALSE)
, m_bHideInactiveScrollbar(FALSE)
, m_bScrollOnLeft(FALSE)
, m_fScrollbarExtraSpacing(0.0f)
, m_nFadeoutTicks(0)
, m_nFadeoutDelay(0)
, m_bDrawDropshadow(FALSE)
, m_dwBkgColor(0)
, m_nAlignment(0)
, m_fBordersize(0.0f)
, m_pScrollbar(NULL)
, m_pScrollbarFrame(NULL)
, m_pThumbpadFrame(NULL)
, m_bBkgdRect(FALSE)
, m_dwBkgdRectColor(0)
, m_nNumChatChannels(0)
, m_peChatChannels(NULL)
{
	m_DataList.Init();
	m_AddedDataList.Init();
}

UI_TEXTBOX::~UI_TEXTBOX(void)
{
	m_LinesList.Destroy();
	m_AddedLinesList.Destroy();
	m_DataList.Destroy();
	m_AddedDataList.Destroy();
}

UI_LISTBOX::UI_LISTBOX(void)
: UI_TEXTBOX()
, m_nSelection(0)
, m_nHoverItem(0)
, m_dwItemHighlightColor(0)
, m_dwItemColor(0)
, m_dwItemHighlightBKColor(0)
, m_fLineHeight(0.0f)
, m_bAutosize(FALSE)
, m_pHighlightBarFrame(NULL)
{
}

UI_COMBOBOX::UI_COMBOBOX(void)
: UI_COMPONENT()
, m_pListBox(NULL)
, m_pLabel(NULL)
, m_pButton(NULL)
{
}

UI_AUTOMAP::UI_AUTOMAP(void)
: UI_PANELEX()
, m_dwCycleLastChange()
, m_fScale(0.0f)
, m_nZoomLevel(0)
, m_pAutoMapItems(NULL)
, m_nAutoMapLen(0)
, m_vLastPosition(0.0f, 0.0f)
, m_vLastFacing(0.0f, 0.0f)
, m_fLastFacingAngle(0.0f)
, m_fMonsterDistance(0.0f)
, m_pFrameWarpIn(NULL)
, m_pFrameWarpOut(NULL)
, m_pFrameHero(NULL)
, m_pFrameTown(NULL)
, m_pFrameMonster(NULL)
, m_pFrameHellrift(NULL)
, m_pFrameQuestNew(NULL)
, m_pFrameQuestWaiting(NULL)
, m_pFrameQuestReturn(NULL)
, m_pFrameVendorNPC(NULL)
, m_pFrameHealerNPC(NULL)
, m_pFrameTrainerNPC(NULL)
, m_pFrameGuideNPC(NULL)
, m_pFrameCrafterNPC(NULL)
, m_pFrameMapVendorNPC(NULL)
, m_pFrameGrave(NULL)
, m_pGamblerNPC(NULL)
, m_pGuildmasterNPC(NULL)
, m_pFrameMapExit(NULL)
, m_bIconAlpha(FALSE)
, m_bWallsAlpha(FALSE)
, m_bTextAlpha(FALSE)
, m_nLastLevel(0)
, m_nHellriftClass(0)
, m_pRectOrganize(NULL)
{
	m_szHellgateString[0] = 0;
}

UI_COLORPICKER::UI_COLORPICKER(void)
: UI_COMPONENT()
, m_pCellFrame(NULL)
, m_pCellUpFrame(NULL)
, m_pCellDownFrame(NULL)
, m_nCurrentColor(0)
, m_fGridOffsetX(0.0f)
, m_fGridOffsetY(0.0f)
, m_fCellSize(0.0f)
, m_fCellSpacing(0.0f)
, m_nGridRows(0)
, m_nGridCols(0)
, m_pdwColors(NULL)
, m_nSelectedIndex(0)
, m_nOriginalIndex(0)
, m_pfnCallback(NULL)
, m_dwCallbackData(0)
{
}

//----------------------------------------------------------------------------
// UI_LINE body definitions
//----------------------------------------------------------------------------

UI_LINE::UI_LINE(int nSize)
{
	m_nSize = nSize;
	m_szText = (nSize)? MALLOC_NEWARRAY(NULL, WCHAR, nSize) : NULL;
	if (m_szText)
		m_szText[0] = 0;
	m_tiTimeAdded = AppCommonGetCurTime();
}

UI_LINE::~UI_LINE()
{
	if (m_szText)
		FREE_DELETE_ARRAY(NULL, m_szText, WCHAR);
	m_HypertextList.Destroy();
}

BOOL UI_LINE::Resize(int nSize)
{
	if (nSize < 8)
		nSize = 8;					// Set a minimum size so we don't try to resize all the time
	if (m_szText && nSize <= m_nSize && nSize * 2 > m_nSize)
		return FALSE;				// Already have enough space (but not to much), just use what we have
	if (m_szText)
		FREE_DELETE_ARRAY(NULL, m_szText, WCHAR);
	m_szText = MALLOC_NEWARRAY(NULL, WCHAR, nSize);
	m_nSize = nSize;
	if (m_szText)
		m_szText[0] = 0;
	return TRUE;
}

void UI_LINE::SetText(const WCHAR *szText)
{
	int	nLen = PStrLen(szText) + 1;
	if (nLen > m_nSize)
		Resize(nLen);
	if (m_szText)
	{
		if (nLen <= 1)
			m_szText[0] = 0;
		else
			PStrCopy(m_szText, szText, nLen);
	}
}

//----------------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------------

static void sAutomapGetPosition(
	int nEngineAutomap,
	float fXOffset,
	float fYOffset,
	float fAutomapScale,
	const VECTOR & vItemPos,
	const VECTOR & vControlUnitPos,
	const MATRIX & mTransformationMatrix,
	float & x1,
	float & y1,
	float & x2,
	float & y2 )
{

#ifdef ENABLE_NEW_AUTOMAP
	{
		VECTOR vec;
		E_RECT tRect;
		tRect.Set( 0, 0, (LONG)UIDefaultWidth(), (LONG)UIDefaultHeight() );
		V( e_AutomapGetItemPos( nEngineAutomap, vItemPos, vec, tRect ) );
		x2 = vec.fX + fXOffset;
		y2 = vec.fY + fYOffset;
	}
#endif // ENABLE_NEW_AUTOMAP

	x1 = fXOffset;
	y1 = fYOffset;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const UI_COMPONENT_DATA *UIComponentGetData(
	int nUICompIndex)
{
	return (const UI_COMPONENT_DATA *)ExcelGetData( NULL, DATATABLE_UI_COMPONENT, nUICompIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UI_TB_MouseToScene( GAME * pGame, VECTOR *pvOrig, VECTOR *pvDir, BOOL bCenter /* = FALSE */ )
{

	int nCursorX, nCursorY;
	UIGetCursorPosition( &nCursorX, &nCursorY, FALSE );

	// Compute the vector of the pick ray in screen space
	int nScreenWidth, nScreenHeight;
	e_GetWindowSize( nScreenWidth, nScreenHeight );
	if( bCenter )
	{
		nCursorX = nScreenWidth / 2;
		nCursorY = nScreenHeight / 2;
	}
	MATRIX mWorld, mView, mProj;
	e_GetWorldViewProjMatricies( &mWorld, &mView, &mProj, NULL, TRUE );
	float fCoverageWidth = e_GetUICoverageWidth();
	float fCursorX = (float)nCursorX;
	if( e_GetUICoveredLeft() )
	{
		fCursorX -= fCoverageWidth;
	}
	fCursorX *= ( (float)nScreenWidth / ( (float)nScreenWidth - fCoverageWidth ) );
	nCursorX = (int)fCursorX;

	BOUNDING_BOX tViewport;
	tViewport.vMin = VECTOR(0, 0, 0.0f );
	tViewport.vMax = VECTOR( ( (float)nScreenWidth - fCoverageWidth ), (float)nScreenHeight, 1.0f );
	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mProj, TRUE, &tViewport );
/*
	if( e_GetUILeftCovered() )
	{
		nCursorX -= nScreenWidth / 2;
		nCursorX *= 2;
		//nScreenWidth /= 2;
		
		BOUNDING_BOX tViewport;
		tViewport.vMin = VECTOR(0, 0, 0.0f );
		tViewport.vMax = VECTOR( (float)nScreenWidth / 2, (float)nScreenHeight, 1.0f );
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mProj, TRUE, &tViewport );
	}
	else if( e_GetUITopLeftCovered() && e_GetUIRightCovered() )
	{
		nCursorX *= 2;
		//nScreenWidth /= 2;
		nCursorY = (int)( ( (float)nCursorY  - (float)nScreenHeight * .56f )/ .44f);
		BOUNDING_BOX tViewport;
		tViewport.vMin = VECTOR(0, 0, 0.0f );
		tViewport.vMax = VECTOR( (float)nScreenWidth / 2, (float)nScreenHeight * .44f, 1.0f );
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mProj, TRUE, &tViewport );
	}
	else if( e_GetUIRightCovered() )
	{
		nCursorX *= 2;
		//nScreenWidth /= 2;
		
		BOUNDING_BOX tViewport;
		tViewport.vMin = VECTOR(0, 0, 0.0f );
		tViewport.vMax = VECTOR( (float)nScreenWidth / 2, (float)nScreenHeight, 1.0f );
		const CAMERA_INFO * pCameraInfo = CameraGetInfo();
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mProj, TRUE, &tViewport );
	}*/


	VECTOR v;
	v.fX =  ( ( ( 2.0f * (float)nCursorX ) / (float)nScreenWidth  ) - 1 ) / mProj._11;
	v.fY = -( ( ( 2.0f * (float)nCursorY ) / (float)nScreenHeight ) - 1 ) / mProj._22;
	v.fZ =  1.0f;

	//MATRIX m = mWorld * mView;
	MATRIX m;
	MatrixMultiply( &m, &mWorld, &mView );
	MatrixInverse( &m, &m );

	// Transform the screen space pick ray into 3D space
	pvDir->fX  = v.fX*m._11 + v.fY*m._21 + v.fZ*m._31;
	pvDir->fY  = v.fX*m._12 + v.fY*m._22 + v.fZ*m._32;
	pvDir->fZ  = v.fX*m._13 + v.fY*m._23 + v.fZ*m._33;
	pvOrig->fX = m._41;
	pvOrig->fY = m._42;
	pvOrig->fZ = m._43;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sUITextBoxGetLineHeight(
	UI_COMPONENT * pComponent)
{
	ASSERT_RETZERO(pComponent);

	UIX_TEXTURE_FONT* pFont = UIComponentGetFont(pComponent);
	if (!pFont)
	{
		return 0.0f;
	}

	int nFontSize = UIComponentGetFontSize(pComponent, pFont);
	float fSize = (float)(nFontSize + 1 ) * g_UI.m_fUIScaler;
	if (!pComponent->m_bNoScale)
	{
		fSize *= UIGetScreenToLogRatioY(pComponent->m_bNoScaleFont);
	}

	return fSize;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUITextBoxFadeoutLines(
	GAME *pGame,
	const CEVENT_DATA& tData,
	DWORD)
{
	UI_TEXTBOX* pTextBox = (UI_TEXTBOX *)(tData.m_Data1);
	ASSERT_RETURN(pTextBox);

	if (pTextBox->m_nFadeoutTicks > 0)
	{
		BOOL bResetLines = (BOOL)(tData.m_Data2);
		TIME tiCurTime = AppCommonGetCurTime();
		BOOL bOneVisible = FALSE;
		BYTE bAlpha = 0;

		UI_GFXELEMENT *pElement = pTextBox->m_pGfxElementFirst;
		while (pElement)
		{
			if (pElement->m_eGfxElement == GFXELEMENT_TEXT)
			{
				UI_LINE *pLine = (UI_LINE *)pElement->m_qwData;
				if (pLine)
				{
					if (bResetLines)
						pLine->m_tiTimeAdded = tiCurTime;

					DWORD dwAge = TimeGetElapsed(pLine->m_tiTimeAdded, tiCurTime);
					if (dwAge < (UINT)pTextBox->m_nFadeoutDelay)
					{
						bAlpha = 255;
						bOneVisible = TRUE;
					}
					else if ( dwAge - pTextBox->m_nFadeoutDelay > (UINT)pTextBox->m_nFadeoutTicks)
					{
						bAlpha = 0;
					}
					else
					{
						bAlpha = 255 - (BYTE)((255 * (dwAge - pTextBox->m_nFadeoutDelay)) / pTextBox->m_nFadeoutTicks);
						bOneVisible = TRUE;
					}
					pElement->m_dwColor = UI_MAKE_COLOR(bAlpha, pElement->m_dwColor);
				}
			}
			pElement = pElement->m_pNextElement;
		}

		if (pTextBox->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(pTextBox->m_tMainAnimInfo.m_dwAnimTicket);
		}

		if (bOneVisible)
		{
			pTextBox->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(sUITextBoxFadeoutLines, CEVENT_DATA((DWORD_PTR)pTextBox, (DWORD_PTR)FALSE));
		}
		else
		{
			if (pTextBox->m_pScrollbar)
			{
			}
		}

		UISetNeedToRender(pTextBox);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITextBoxOnPaint(
	UI_COMPONENT * pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_TEXTBOX * pTextBox = UICastToTextBox(pComponent);
	ASSERT_RETVAL(pTextBox, UIMSG_RET_NOT_HANDLED);

	UIX_TEXTURE_FONT* pFont = UIComponentGetFont(pComponent);
	if (!pFont)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nFontSize = UIComponentGetFontSize(pComponent, pFont);

	UIComponentRemoveAllElements(pTextBox);
	UIComponentAddFrame(pTextBox);

	UI_RECT cliprect(pTextBox->m_fBordersize, pTextBox->m_fBordersize, pTextBox->m_fWidth - pTextBox->m_fBordersize, pTextBox->m_fHeight - pTextBox->m_fBordersize);
	//if (pTextBox->m_dwBkgColor >> 24 > 0)	
	//{
	//	UIComponentAddPrimitiveElement(pTextBox, UIPRIM_BOX, 0, rectComponent, NULL, NULL, pTextBox->m_dwBkgColor);
	//}
	 
	float fScreenToLogRatioX = UIGetScreenToLogRatioX(pTextBox->m_bNoScaleFont);
	float fScreenToLogRatioY = UIGetScreenToLogRatioY(pTextBox->m_bNoScaleFont);

	BOOL bBottomAlign = (pTextBox->m_nAlignment == UIALIGN_BOTTOM || pTextBox->m_nAlignment == UIALIGN_BOTTOMLEFT || pTextBox->m_nAlignment == UIALIGN_BOTTOMRIGHT);
	float fLineHeight = sUITextBoxGetLineHeight(pTextBox);
	float fStartY = pTextBox->m_fBordersize;

	if (bBottomAlign)
	{
		fLineHeight = -fLineHeight;
		fStartY = pTextBox->m_fHeight;
	}

	CItemPtrList<UI_LINE>::USER_NODE *pItr = (bBottomAlign ? pTextBox->m_LinesList.GetPrev(NULL) : pTextBox->m_LinesList.GetNext(NULL));
	int nLine = bBottomAlign ? 1 : 0;
	while (pItr)
	{
		if (pItr->m_Value->HasText())
		{
			float fY = fStartY + ((float)(nLine++) * fLineHeight);
			if (pTextBox->m_bBkgdRect)
			{
				UI_RECT tRect(0.0f, 0.0f, 0.0f, 0.0f);
				UIElementGetTextLogSize(pFont, nFontSize, 1.0f, pTextBox->m_bNoScaleFont, pItr->m_Value->GetText(), &tRect.m_fX2, &tRect.m_fY2, TRUE, TRUE);
				tRect.m_fY2 += 1.0f;
				tRect.m_fX2 *= fScreenToLogRatioX;
				tRect.m_fY2 *= fScreenToLogRatioY;
				tRect.Translate(pTextBox->m_fBordersize, fY);
				UI_ELEMENT_PRIMITIVE *pPrim = UIComponentAddPrimitiveElement(pTextBox, 
					UIPRIM_BOX, 0, tRect, NULL, NULL, pTextBox->m_dwBkgdRectColor);
			
				if (pPrim)
					pPrim->m_qwData = (QWORD)(&pItr->m_Value);
			}

			// KCK: I hate to do yet another width processing step, but unfortunately without major changes this is the
			// only place I have the information necessary to determine the hypertext bounds. Future refactoring of this
			// code should fold this in with the initial text processing.
			UISetHypertextBounds(pItr->m_Value, pFont, nFontSize, pTextBox->m_bNoScale, abs(fLineHeight), 0.0f, fY);

			UI_GFXELEMENT *pElement = UIComponentAddTextElement(pTextBox, NULL, pFont, nFontSize, pItr->m_Value->GetText(), 
				UI_POSITION(pTextBox->m_fBordersize, fY), pItr->m_Value->m_dwColor,
				&cliprect, UIALIGN_TOPLEFT, NULL, pTextBox->m_bDrawDropshadow );

			// Ok, stay with me here.  I'm going to point the element back directly to the line so we can get the
			//   time and fade out without having to repaint everything.  The savings may be slight, but hey.
			//   The time is too big to save directly in the DWORD.
			pElement->m_qwData = (QWORD)(&pItr->m_Value);
		}
		else
		{
			nLine++;
		}
		pItr = (bBottomAlign ? pTextBox->m_LinesList.GetPrev(pItr) : pTextBox->m_LinesList.GetNext(pItr));
	}

	sUITextBoxFadeoutLines(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pTextBox, (DWORD_PTR)0), 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIListBoxOnPaint(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_LISTBOX * pListBox = UICastToListBox(component);
	ASSERT_RETVAL(pListBox, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(pListBox);
	UIComponentAddFrame(pListBox);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(pListBox);
	if (!font)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	 
	float x, y;
	int nHoverItem = -1;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (UIComponentCheckBounds(component, x, y, &pos))
	{
		// get mouse relative to the component
		x -= pos.m_fX + pListBox->m_fBordersize;
		y -= pos.m_fY + pListBox->m_fBordersize;
		if (pListBox->m_pScrollbar)
		{
			y += pListBox->m_pScrollbar->m_ScrollPos.m_fY;
		}
		x /= UIGetScreenToLogRatioX(component->m_bNoScale);
		y /= UIGetScreenToLogRatioY(component->m_bNoScale);

		nHoverItem = (int)(y / pListBox->m_fLineHeight);

		if (nHoverItem >= (int)pListBox->m_LinesList.Count())
		{
			nHoverItem = -1;
		}
	}

	if (pListBox->m_nHoverItem != nHoverItem)
	{
		pListBox->m_nHoverItem = nHoverItem;
		UIComponentHandleUIMessage(component->m_pParent, UIMSG_LB_ITEMHOVER, 0, 0, FALSE);
	}

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);

	int nFontSize = UIComponentGetFontSize(pListBox, font);
	float ystart = pListBox->m_fBordersize;

	UI_RECT cliprect(pListBox->m_fBordersize, pListBox->m_fBordersize, pListBox->m_fWidth - pListBox->m_fBordersize, pListBox->m_fHeight - pListBox->m_fBordersize);

	UI_COMPONENT *pDrawComponent = pListBox;
	if (pListBox->m_pFirstChild &&
		pListBox->m_pFirstChild->m_eComponentType == UITYPE_FLEXBORDER)
	{
		pDrawComponent = pListBox->m_pFirstChild;
		UIComponentRemoveAllElements(pDrawComponent);	// CHB 2007.01.23 - Fix element leak.
	}
	
	int ii = 0;
	CItemPtrList<UI_LINE>::USER_NODE *pItr = pListBox->m_LinesList.GetNext(NULL);
	while (pItr)
	{
		if (pItr->m_Value->HasText())
		{
			DWORD dwColor = pItr->m_Value->m_dwColor;
			UI_POSITION pos(pListBox->m_fBordersize, ystart + (float)ii * pListBox->m_fLineHeight);
			if (ii == pListBox->m_nSelection || ii == pListBox->m_nHoverItem)
			{
				dwColor = pListBox->m_dwItemHighlightColor;
			}
			if (ii == pListBox->m_nSelection && pListBox->m_pHighlightBarFrame)
			{
				UIComponentAddElement(pDrawComponent, pTexture, pListBox->m_pHighlightBarFrame, pos, pListBox->m_dwItemHighlightBKColor, &cliprect, FALSE, 1.0f, 1.0f, &UI_SIZE(pListBox->m_fWidth - pListBox->m_fBordersize * 2.0f, (float)nFontSize)); 
			}
			UIComponentAddTextElement(pDrawComponent, UIComponentGetTexture(pListBox), font, 
				nFontSize, pItr->m_Value->GetText(), pos, dwColor, &cliprect);
		}
		pItr = pListBox->m_LinesList.GetNext(pItr);
		ii++;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextBoxClear(
	UI_COMPONENT* component)
{
	UI_TEXTBOX* pTextBox = UICastToTextBox(component);
	ASSERT_RETURN(pTextBox);

	pTextBox->m_LinesList.Destroy();			// KCK Note: Destroy will clean up the memory and call destructors. Better in this case
	pTextBox->m_DataList.Clear();
	pTextBox->m_AddedLinesList.Destroy();		// KCK Note: Destroy will clean up the memory and call destructors. Better in this case
	pTextBox->m_AddedDataList.Clear();

	if (pTextBox->m_pScrollbar)
	{
		pTextBox->m_pScrollbar->m_ScrollPos.m_fX = 0.0f;
		pTextBox->m_pScrollbar->m_ScrollPos.m_fY = 0.0f;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextBoxClear(
	int idComponent)
{
	UI_COMPONENT* component = UIComponentGetById(idComponent);
	if (!component)
	{
		return;
	}
	UITextBoxClear(component);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITextBoxOnMouseLeave(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentOnMouseLeave(component, msg ,wParam, lParam);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_TEXTBOX *pTextBox = UICastToTextBox(component);
	if (!pTextBox->m_pScrollbar ||
		!pTextBox->m_bHideInactiveScrollbar)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentSetVisible(pTextBox->m_pScrollbar, FALSE);
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITextBoxOnMouseOver(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_TEXTBOX *pTextBox = UICastToTextBox(component);
	if (!pTextBox->m_pScrollbar ||
		!pTextBox->m_bHideInactiveScrollbar)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// scrollbar's not being used right now
	if (pTextBox->m_pScrollbar->m_fMin == 0 &&
		pTextBox->m_pScrollbar->m_fMax == 0)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UICursorGetActive())
	{
		return UITextBoxOnMouseLeave(component, msg, wParam, lParam);
	}

	UIComponentSetActive(pTextBox->m_pScrollbar, TRUE);
	sUITextBoxFadeoutLines(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pTextBox, (DWORD_PTR)0), 0);
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUILabelAdjustScrollbar( UI_LABELEX* pLabel)
{
	ASSERT_RETURN(pLabel);
	if (pLabel->m_pScrollbar)
	{
		float fTotalHeight = pLabel->m_fTextHeight;

		if (fTotalHeight > pLabel->m_fHeight)
		{
			if (!pLabel->m_bHideInactiveScrollbar ||
				UIComponentCheckBounds(pLabel))
			{
				UIComponentSetActive(pLabel->m_pScrollbar, TRUE);
			}
			if (pLabel->m_nAlignment == UIALIGN_BOTTOM || pLabel->m_nAlignment == UIALIGN_BOTTOMLEFT || pLabel->m_nAlignment == UIALIGN_BOTTOMRIGHT)
			{
				pLabel->m_pScrollbar->m_fMin = pLabel->m_fHeight - fTotalHeight;
				pLabel->m_pScrollbar->m_fMax = 0;
			}
			else
			{
				pLabel->m_pScrollbar->m_fMin = 0;
				pLabel->m_pScrollbar->m_fMax = fTotalHeight - pLabel->m_fHeight;
			}
		}
		else
		{
			pLabel->m_pScrollbar->m_fMin = 0;
			pLabel->m_pScrollbar->m_fMax = 0;
			UIComponentSetVisible(pLabel->m_pScrollbar, FALSE);
		}
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUITextBoxAdjustScrollbar(
	UI_TEXTBOX* pTextBox)
{
	ASSERT_RETURN(pTextBox);
	if (pTextBox->m_pScrollbar)
	{
		float fLineHeight = sUITextBoxGetLineHeight(pTextBox);
		float fTotalHeight = (float)pTextBox->m_LinesList.Count() * fLineHeight;
		fTotalHeight += pTextBox->m_fBordersize * 2;

		if (fTotalHeight > pTextBox->m_fHeight)
		{
			if (!pTextBox->m_bHideInactiveScrollbar ||
				UIComponentCheckBounds(pTextBox))
			{
				UIComponentSetActive(pTextBox->m_pScrollbar, TRUE);
			}
			if (pTextBox->m_nAlignment == UIALIGN_BOTTOM || pTextBox->m_nAlignment == UIALIGN_BOTTOMLEFT || pTextBox->m_nAlignment == UIALIGN_BOTTOMRIGHT)
			{
				pTextBox->m_pScrollbar->m_fMin = pTextBox->m_fHeight - fTotalHeight;
				pTextBox->m_pScrollbar->m_fMax = 0;
			}
			else
			{
				pTextBox->m_pScrollbar->m_fMin = 0;
				pTextBox->m_pScrollbar->m_fMax = fTotalHeight - pTextBox->m_fHeight;
			}
		}
		else
		{
			pTextBox->m_pScrollbar->m_fMin = 0;
			pTextBox->m_pScrollbar->m_fMax = 0;
			UIComponentSetVisible(pTextBox->m_pScrollbar, FALSE);
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUITextBoxAddLine(
	UI_TEXTBOX* pTextBox,
	const WCHAR* str,
	DWORD color = GFXCOLOR_WHITE,
	QWORD nData = 0)
{
	if (pTextBox->m_nMaxLines > 0 &&
		(int)pTextBox->m_LinesList.Count() >= pTextBox->m_nMaxLines)
	{
		if (pTextBox->m_bBottomUp)
		{
			pTextBox->m_LinesList.PopTail(TRUE);		// get rid of the oldest line
			pTextBox->m_DataList.PopTail();

		}
		else
		{
			pTextBox->m_LinesList.PopHead(TRUE);		// get rid of the oldest line
			pTextBox->m_DataList.PopHead();
		}
	}

	UI_LINE* pNewLine = MALLOC_NEW(NULL, UI_LINE);		// Grr. Stupid memory macros don't allow non-default constructor arguments
	pNewLine->Resize(UI_LINE_DEFAULT_MAX_LEN);
	pNewLine->m_dwColor = color;

//	PStrCopy(pNewLine->m_szText, str, arrsize(pNewLine->m_szText));
	pNewLine->SetText(str);

	// KCK: It would be nice to be able to fold this into the word wrap code, as it's 
	// basically just reprocessing the string after word wrapping has been done. However,
	// as this is only done on initialization and resize, I'm not to worried about it.
	UIReprocessHyperText(pNewLine);

	if (pTextBox->m_bBottomUp)
	{
		pTextBox->m_LinesList.CItemListPushHead(pNewLine);
		pTextBox->m_DataList.PListPushHead(nData);
	}
	else
	{
		pTextBox->m_LinesList.CItemListPushTail(pNewLine);
		pTextBox->m_DataList.PListPushTail(nData);
	}

	sUITextBoxAdjustScrollbar(pTextBox);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextBoxAddLine(
	UI_TEXTBOX* pTextBox,
	const WCHAR* str,
	DWORD color /*= GFXCOLOR_WHITE*/,
	QWORD nData /* = 0 */,
	UI_LINE* pOriginLine /* = NULL */,
	BOOL bIgnoreHypertext /* = FALSE */)
{
	ASSERT_RETURN(pTextBox);

	// KCK: If we have no origin line, then this is a new line and we need to create one
	if (!pOriginLine)
	{
		ASSERT_RETURN(str);

		UI_LINE*	pNewLine = MALLOC_NEW(NULL, UI_LINE);
//		pNewLine->Resize(UI_LINE_DEFAULT_MAX_LEN);
		pNewLine->m_dwColor = color;
		pNewLine->SetText(str);

		if (pTextBox->m_nMaxLines > 0 &&
			(int)pTextBox->m_AddedLinesList.Count() >= pTextBox->m_nMaxLines)
		{
			pTextBox->m_AddedLinesList.PopHead(TRUE);		// get rid of the oldest line
			pTextBox->m_AddedDataList.PopHead();
		}

		if (!bIgnoreHypertext)
		{
			UIReprocessHyperText(pNewLine);

			// KCK: Process the Original text string, replacing all HTML-like hypertext info with
			// control character triggered hypertext information and create hypertext objects.
			UIProcessHyperText(pNewLine);
		}

		pTextBox->m_AddedLinesList.CItemListPushTail(pNewLine);
		pTextBox->m_AddedDataList.PListPushTail(nData);

		pOriginLine = pNewLine;
	}

	// Copy the processed text to a temporary buffer while we do word wrapping
	WCHAR *szBuf = (WCHAR*) MALLOC(NULL, sizeof(WCHAR) * (PStrLen(str)+1));
	if (pOriginLine->HasText())
		PStrCopy(szBuf, pOriginLine->GetText(), PStrLen(str)+1);
	else
		szBuf[0] = 0;

	if (pTextBox->m_bWordWrap)
	{
		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pTextBox);
		DoWordWrapEx(szBuf, NULL, pTextBox->m_fWidth * UIGetLogToScreenRatioX(pTextBox->m_bNoScale || !pTextBox->m_bNoScaleFont), pFont, pTextBox->m_bNoScaleFont, UIComponentGetFontSize(pTextBox, pFont));
	}

	WCHAR* pCur = szBuf;
	do
	{
		WCHAR* pStart = pCur;
		BOOL bNotherLine = FALSE;

		while (*pCur && *pCur != L'\n' && *pCur != L'\r')
		{
			pCur++;
		}
		if (*pCur == L'\n' || *pCur == L'\r')
		{
			*pCur = 0;
			bNotherLine = TRUE;
		}
		sUITextBoxAddLine(pTextBox, pStart, color, nData);
		if (bNotherLine)
		{
			pCur++;
		}
	} while (*pCur);

	FREE(NULL, szBuf);

	// restore the textbox if it's been faded
	//if (pTextBox->m_nFadeoutTicks > 0)
	//	UITextBoxResetFadeout(pTextBox);

	UIComponentHandleUIMessage(pTextBox, UIMSG_PAINT, 0, 0);

	// schedule a fadeout if the control hasn't been accessed in a while
	sUITextBoxFadeoutLines(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pTextBox, (DWORD_PTR)0), 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextBoxAddLine(
	int idComponent,
	const WCHAR* str,
	DWORD color,
	QWORD nData /* = 0 */)
{
	UI_COMPONENT* component = UIComponentGetById(idComponent);
	if (!component)
	{
		return;
	}
	UI_TEXTBOX* pTextBox = UICastToTextBox(component);
	ASSERT_RETURN(pTextBox);

	UITextBoxAddLine(pTextBox, str, color, nData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextBoxResetFadeout(
	UI_TEXTBOX *pTextBox)
{
	sUITextBoxFadeoutLines(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pTextBox, (DWORD_PTR)TRUE), 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITextBoxOnResize(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentResize(component, msg, wParam, lParam);

	UI_TEXTBOX* pTextBox = UICastToTextBox(component);

	pTextBox->m_LinesList.Clear();
	pTextBox->m_DataList.Clear();

	CItemPtrList<UI_LINE>::USER_NODE *pItr = pTextBox->m_AddedLinesList.GetNext(NULL);
	PList<QWORD>::USER_NODE *pDataItr = pTextBox->m_AddedDataList.GetNext(NULL);

	while (pItr && pDataItr)
	{
		UITextBoxAddLine(pTextBox, pItr->m_Value->GetText(), pItr->m_Value->m_dwColor, pDataItr->Value, pItr->m_Value);

		pItr = pTextBox->m_AddedLinesList.GetNext(pItr);
		pDataItr = pTextBox->m_AddedDataList.GetNext(pDataItr);
	}

	if (pTextBox->m_pScrollbar)
	{
		if( AppIsHellgate() )
		{
			pTextBox->m_pScrollbar->m_fHeight = pTextBox->m_fHeight;
			UIComponentHandleUIMessage(pTextBox->m_pScrollbar, UIMSG_SCROLL, 0, 0, FALSE);
			UIComponentHandleUIMessage(pTextBox->m_pScrollbar, UIMSG_PAINT, 0, 0);
		}
		else // in Mythos, our scrollbar doesn't actually match the chatbox height - this doesn't work for us
		{
			pTextBox->m_pScrollbar->m_fHeight = pTextBox->m_fHeight - 50;
			UIComponentHandleUIMessage(pTextBox->m_pScrollbar, UIMSG_SCROLL, 0, 0, FALSE);
			UIComponentHandleUIMessage(pTextBox->m_pScrollbar, UIMSG_PAINT, 0, 0);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITextBoxOnScroll(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_TEXTBOX *pTextBox = UICastToTextBox(component);
	UITextBoxResetFadeout(pTextBox);
	return UIMSG_RET_HANDLED;
 }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatOnMouseWheel(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	// if the scroll bar is active it should handle the message first.
	//   this is for if it's not active
	if (UIComponentGetActive(component) && UIComponentCheckBounds(component))
	{
		UI_TEXTBOX *pTextBox = UICastToTextBox(component);
		UITextBoxResetFadeout(pTextBox);

		// special check for the scroll bar inactive but we need it to be active
		if (pTextBox->m_pScrollbar && !UIComponentGetActive(pTextBox->m_pScrollbar))
		{
			sUITextBoxAdjustScrollbar(pTextBox);
			UIComponentHandleUIMessage(pTextBox->m_pScrollbar, msg, wParam, lParam);
		}
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
 }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIListBoxOnMouseDown(
	UI_COMPONENT * component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_LISTBOX * pListBox = UICastToListBox(component);
	ASSERT_RETVAL(pListBox, UIMSG_RET_NOT_HANDLED);

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// get mouse relative to the component
	x -= pos.m_fX + pListBox->m_fBordersize;
	y -= pos.m_fY + pListBox->m_fBordersize;

	if (pListBox->m_pScrollbar)
	{
		y += pListBox->m_pScrollbar->m_ScrollPos.m_fY;
	}
	x /= UIGetScreenToLogRatioX(component->m_bNoScale);
	y /= UIGetScreenToLogRatioY(component->m_bNoScale);

	UIListBoxSetSelectedIndex(pListBox, (int)(y / pListBox->m_fLineHeight));

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIListBoxAddString(
	UI_LISTBOX *pListBox,
	const WCHAR *szString,
	const QWORD nData /* =0 */)
{
	ASSERT_RETURN(pListBox);
	UITextBoxAddLine(pListBox, szString, pListBox->m_dwItemColor, nData);

	if (pListBox->m_bAutosize)
	{
		pListBox->m_fHeight = (pListBox->m_LinesList.Count() * pListBox->m_fLineHeight) + (pListBox->m_fBordersize * 2.0f);
		pListBox->m_InactivePos.m_fY = pListBox->m_ActivePos.m_fY - pListBox->m_fHeight;

		// Ugly special case
		if (pListBox->m_pParent && pListBox->m_pParent->m_pParent && pListBox->m_pParent->m_pParent->m_eComponentType == UITYPE_COMBOBOX)
		{
			pListBox->m_pParent->m_fHeight = pListBox->m_fHeight;
		}

		if (pListBox->m_pScrollbar)
		{
			UIComponentSetVisible(pListBox->m_pScrollbar, FALSE);
		}
	}
	else if (pListBox->m_LinesList.Count() * pListBox->m_fLineHeight > 
	   		 pListBox->m_fHeight - pListBox->m_fBordersize * 2.0f)
	{
		if (!pListBox->m_pScrollbar)
		{
			pListBox->m_pScrollbar = (UI_SCROLLBAR *) UIComponentCreateNew(pListBox, UITYPE_SCROLLBAR, TRUE);
			ASSERT_RETURN(pListBox->m_pScrollbar);

			pListBox->m_pScrollbar->m_nOrientation = UIORIENT_TOP;
			pListBox->m_pScrollbar->m_eState = pListBox->m_eState;
			pListBox->m_pScrollbar->m_bVisible = pListBox->m_bVisible;
			pListBox->m_pScrollbar->m_pFrame = pListBox->m_pScrollbarFrame;
			pListBox->m_pScrollbar->m_pThumbpadFrame = pListBox->m_pThumbpadFrame;
			pListBox->m_pScrollbar->m_fWidth = pListBox->m_pScrollbarFrame ? pListBox->m_pScrollbarFrame->m_fWidth : 0.0f;
			pListBox->m_pScrollbar->m_bSendPaintToParent = TRUE;
//			pListBox->m_pScrollbar->m_pScrollControl = pListBox;
		}
		pListBox->m_pScrollbar->m_Position.m_fX = pListBox->m_fWidth - pListBox->m_pScrollbar->m_fWidth;
		pListBox->m_pScrollbar->m_fHeight = pListBox->m_fHeight;

		pListBox->m_pScrollbar->m_fMin = 0;
		pListBox->m_pScrollbar->m_fMax = (pListBox->m_LinesList.Count() * pListBox->m_fLineHeight) - (pListBox->m_fHeight - pListBox->m_fBordersize * 2.0f);
		UIComponentSetActive(pListBox->m_pScrollbar, TRUE);
	}

	UIComponentHandleUIMessage(pListBox, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIListBoxGetSelectedIndex(
	UI_LISTBOX *pListBox)
{
	ASSERT_RETVAL(pListBox, -1);

	return pListBox->m_nSelection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIListBoxGetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szListBoxName)
{
	ASSERT_RETVAL(pParent, -1);
	ASSERT_RETVAL(szListBoxName, -1);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pParent, szListBoxName);
	if (pComponent)
	{
		UI_LISTBOX *pListBox = UICastToListBox(pComponent);
		return UIListBoxGetSelectedIndex(pListBox);
	}

	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIComboBoxGetSelectedIndex(
	UI_COMBOBOX *pComboBox)
{
	ASSERT_RETVAL(pComboBox, -1);
	ASSERT_RETVAL(pComboBox->m_pListBox, -1);

	return pComboBox->m_pListBox->m_nSelection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIComboBoxGetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szComboBoxName)
{
	ASSERT_RETVAL(pParent, -1);
	ASSERT_RETVAL(szComboBoxName, -1);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pParent, szComboBoxName);
	if (pComponent)
	{
		UI_COMBOBOX *pComboBox = UICastToComboBox(pComponent);
		return UIComboBoxGetSelectedIndex(pComboBox);
	}

	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QWORD UIListBoxGetSelectedData(
	UI_LISTBOX *pListBox)
{
	ASSERT_RETVAL(pListBox, 0);

	ASSERT_RETVAL(pListBox->m_nSelection >= 0 && (unsigned int)pListBox->m_nSelection < pListBox->m_DataList.Count(), 0);

	PList<QWORD>::USER_NODE *pItr = pListBox->m_DataList.GetNext(NULL);
	int ii = 0;
	while (pItr)
	{
		if (ii == pListBox->m_nSelection)
		{
			return pItr->Value;
		}
		pItr = pListBox->m_DataList.GetNext(pItr);
		ii++;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QWORD UIComboBoxGetSelectedData(
	UI_COMBOBOX *pComboBox)
{
	ASSERT_RETVAL(pComboBox, (QWORD)-1);
	ASSERT_RETVAL(pComboBox->m_pListBox, (QWORD)-1);

	return UIListBoxGetSelectedData(pComboBox->m_pListBox);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *UIListGetSelectedString(
	UI_LISTBOX *pListBox)
{
	ASSERT_RETVAL(pListBox, L"");

	ASSERT_RETVAL(pListBox->m_nSelection >= 0 && (unsigned int)pListBox->m_nSelection < pListBox->m_LinesList.Count(), L"");

	CItemPtrList<UI_LINE>::USER_NODE *pItr = pListBox->m_LinesList.GetNext(NULL);
	int ii = 0;
	while (pItr)
	{
		if (ii == pListBox->m_nSelection)
		{
			return pItr->m_Value->GetText();
		}
		pItr = pListBox->m_LinesList.GetNext(pItr);
		ii++;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIListBoxSetSelectedIndex(
	UI_LISTBOX *pListBox,
	int nItem,
	BOOL bClose /*=TRUE*/,
	BOOL bSendSelectMessage /*=TRUE*/ )
{
	ASSERT_RETURN(pListBox);

	int nPreviousSelection = pListBox->m_nSelection;

	if (nItem >= 0 &&
		(unsigned int)nItem < pListBox->m_LinesList.Count())
	{
		pListBox->m_nSelection = nItem;
	}
	else
	{
		pListBox->m_nSelection = -1;
	}

	if (pListBox->m_pParent && pListBox->m_pParent->m_pParent && pListBox->m_pParent->m_pParent->m_eComponentType == UITYPE_COMBOBOX)
	{
		UI_COMBOBOX *pComboBox = UICastToComboBox(pListBox->m_pParent->m_pParent);

		if (pComboBox->m_pLabel && pListBox->m_nSelection >= 0)
		{
			UILabelSetText(pComboBox->m_pLabel, UIListGetSelectedString(pListBox));
		}

		if (pComboBox->m_pButton && bClose)
		{
			g_UI.m_pLastMouseDownComponent = pComboBox->m_pButton;
			pComboBox->m_pButton->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
			UIComponentHandleUIMessage(pComboBox->m_pButton, UIMSG_LBUTTONCLICK, 0, 0);
		}
		//UIComponentWindowShadeClosed(pListBox);

		UIComponentHandleUIMessage(pComboBox, UIMSG_PAINT, 0, 0);
		if( bSendSelectMessage )
		{
			UIComponentHandleUIMessage(pComboBox, UIMSG_LB_ITEMSEL, nPreviousSelection, 0);
		}
	}
	else
	{
		UIComponentHandleUIMessage(pListBox, UIMSG_PAINT, 0, 0);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIListBoxSetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szListBoxName,
	int nItem,
	BOOL bClose /*=TRUE*/,
	BOOL bSendSelectMessage /*=TRUE*/)
{
	ASSERT_RETURN(pParent);
	ASSERT_RETURN(szListBoxName);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pParent, szListBoxName);
	if (pComponent)
	{
		UI_LISTBOX *pListBox = UICastToListBox(pComponent);
		UIListBoxSetSelectedIndex(pListBox, nItem, bClose, bSendSelectMessage );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComboBoxSetSelectedIndex(
	UI_COMBOBOX *pComboBox,
	int nItem,
	BOOL bClose  /*=TRUE*/,
	BOOL bSendSelectMessage /*=TRUE*/)
{
	ASSERT_RETURN(pComboBox);
	ASSERT_RETURN(pComboBox->m_pListBox);

	UIListBoxSetSelectedIndex(pComboBox->m_pListBox, nItem, bClose, bSendSelectMessage);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComboBoxSetSelectedIndex(
	UI_COMPONENT *pParent,
	const char *szComboBoxName,
	int nItem,
	BOOL bClose /*=TRUE*/,
	BOOL bSendSelectMessage /*=TRUE*/)
{
	ASSERT_RETURN(pParent);
	ASSERT_RETURN(szComboBoxName);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pParent, szComboBoxName);
	if (pComponent)
	{
		UI_COMBOBOX *pComboBox = UICastToComboBox(pComponent);
		UIComboBoxSetSelectedIndex(pComboBox, nItem, bClose, bSendSelectMessage);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_GFXELEMENT* sUIBarDraw(
	UI_BAR *pBar,
	DWORD dwColor,
	float fMinValue,
	float fMaxValue,
	float fValue1,
	float fValue2)
{
	ASSERT_RETNULL(pBar);
//	ASSERT_RETNULL(fMaxValue - fMinValue > 0);

	float fRatio1 = (fValue1 - fMinValue) / (fMaxValue - fMinValue);
	float fRatio2 = (fValue2 - fMinValue) / (fMaxValue - fMinValue);

	if (!pBar->m_bRadial)
	{
		float x1 = 0.0f, y1 = 0.0f;
		float clipx1 = 0.0f, clipy1 = 0.0f;
		float clipx2 = pBar->m_fWidth, clipy2 = pBar->m_fHeight;
		float fWidth = pBar->m_fWidth - pBar->m_fSlantWidth;

		switch (pBar->m_nOrientation)
		{
		case UIORIENT_LEFT:
			clipx2 = (fWidth * fRatio2) + pBar->m_fSlantWidth;
			clipx1 = (fWidth * fRatio1) ;//+ pBar->m_fSlantWidth;
			if (pBar->m_bEndCap)
			{
				x1 = clipx2 - pBar->m_fWidth;
			}
			break;
		case UIORIENT_RIGHT:
			clipx1 = fWidth - ((pBar->m_fWidth * fRatio2) + pBar->m_fSlantWidth);
			clipx2 = fWidth - ((pBar->m_fWidth * fRatio1) /*+ pBar->m_fSlantWidth*/);
			if (pBar->m_bEndCap)
			{
				x1 = clipx1;
			}
			break;
		case UIORIENT_BOTTOM:
			clipy1 = pBar->m_fHeight - (pBar->m_fHeight * fRatio2);
			clipy2 = pBar->m_fHeight - (pBar->m_fHeight * fRatio1);
			if (pBar->m_bEndCap)
			{
				y1 = clipy1;
			}
			break;
		case UIORIENT_TOP:
			clipy2 = pBar->m_fHeight * fRatio2;
			clipy1 = pBar->m_fHeight * fRatio1;
			if (pBar->m_bEndCap)
			{
				y1 = clipy2 - pBar->m_fHeight;
			}
			break;
		}

		return UIComponentAddElement(pBar, UIComponentGetTexture(pBar), pBar->m_pFrame, UI_POSITION(x1, y1), dwColor, &UI_RECT(clipx1, clipy1, clipx2, clipy2));
	}
	else
	{
		UI_POSITION posRotate = UIComponentGetAbsoluteLogPosition(pBar);
		posRotate.m_fX += pBar->m_pFrame->m_fWidth / 2.0f;
		posRotate.m_fY += pBar->m_pFrame->m_fHeight / 2.0f;

		float fAngleDeg = pBar->m_fMaxAngle * (1.0f - fRatio2);
		if (pBar->m_bCounterClockwise)
		{
			fAngleDeg = -fAngleDeg;
		}

		UI_COMPONENT *pMaskComp = UIComponentFindChildByName(pBar->m_pParent, "radial mask");
		if (pMaskComp)
		{
			UI_ARCSECTOR_DATA tData;

			tData.fAngleWidthDeg = fabs(fAngleDeg) + pBar->m_fMaskBaseAngle;
			tData.fAngleStartDeg = pBar->m_fMaskAngleStart;
			if (pBar->m_bCounterClockwise)
			{
				tData.fAngleStartDeg -= tData.fAngleWidthDeg;
			}

			tData.fRingRadiusPct = 1.00f;
			tData.fSegmentWidthDeg = 2.0f;
			UI_RECT rectMask(0.0f, 0.0f, pMaskComp->m_fWidth, pMaskComp->m_fHeight);

			DWORD dwFlags = 0;
			SETBIT(dwFlags, UI_PRIM_ALPHABLEND);
			SETBIT(dwFlags, UI_PRIM_ZWRITE);

			UIComponentRemoveAllElements(pMaskComp);
			UIComponentAddPrimitiveElement(pMaskComp, UIPRIM_ARCSECTOR, dwFlags, rectMask, &tData, NULL,  pMaskComp->m_dwColor);

			//{
			//	WCHAR szBuf[256];
			//	PStrPrintf(szBuf, arrsize(szBuf), L"msk strt=%0.2f  msk wdth=%0.2f  angl=%0.2f", tData.fAngleStartDeg, tData.fAngleWidthDeg, fAngleDeg);
			//	UIComponentAddTextElement(pBar, NULL, UIComponentGetFont(pBar), UIComponentGetFontSize(pBar), szBuf, UI_POSITION());
			//}
		}


		float fAngle = fAngleDeg * (TWOxPI / 360.0f);

		UI_GFXELEMENT *pElement = NULL;

		if (pBar->m_bEndCap)
		{
			pElement = UIComponentAddRotatedElement(pBar, 
				UIComponentGetTexture(pBar), 
				pBar->m_pFrame, 
				UI_POSITION(), 
				dwColor, 
				fAngle, 
				posRotate);

			//, NULL, 
			//FALSE, 
			//1.0f, 
			//dwRotationFlags);
		}
		else
		{
			pElement = UIComponentAddElement(pBar, 
				UIComponentGetTexture(pBar), 
				pBar->m_pFrame, 
				UI_POSITION(), 
				dwColor);
		}

		return pElement;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIBarOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BAR* bar = UICastToBar(component);
	ASSERT_RETVAL(bar, UIMSG_RET_NOT_HANDLED);

	if (msg == UIMSG_SETFOCUSSTAT)
	{
		if (wParam != UIComponentGetFocusUnitId(component))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

	if (!bar->m_pFrame)
	{
		//UIComponentRemoveAllElements(bar);
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* unit = UIComponentGetFocusUnit(component);
	if (!unit && 
		(bar->m_codeCurValue != NULL_CODE ||
		 bar->m_codeMaxValue != NULL_CODE))
	{
		UIComponentRemoveAllElements(bar);
		return UIMSG_RET_HANDLED;
	}
	if (bar->m_codeMaxValue == NULL_CODE &&
		bar->m_nMaxValue == 0)
	{
		UIComponentRemoveAllElements(bar);
		return UIMSG_RET_HANDLED;
	}

	int curvalue = bar->m_nValue;
	int maxvalue = bar->m_nMaxValue;

	if (bar->m_codeCurValue != NULL_CODE)
	{
		bar->m_nValue = curvalue = VMExecI(UnitGetGame(unit), unit, g_UI.m_pCodeBuffer + bar->m_codeCurValue, g_UI.m_nCodeCurr - bar->m_codeCurValue);
	}
	if (bar->m_codeMaxValue != NULL_CODE)
	{
		bar->m_nMaxValue = maxvalue = VMExecI(UnitGetGame(unit), unit, g_UI.m_pCodeBuffer + bar->m_codeMaxValue, g_UI.m_nCodeCurr - bar->m_codeMaxValue);
	}

	curvalue -= bar->m_nMinValue;
	maxvalue -= bar->m_nMinValue;
	
	if (!maxvalue)
	{
		UIComponentRemoveAllElements(bar);
		return UIMSG_RET_HANDLED;
	}

	curvalue = PIN(curvalue, 0, maxvalue);

	DWORD dwBarColor = (bar->m_bUseAltColor ? bar->m_dwAltColor : bar->m_dwColor);
	if (bar->m_pBlinkData)
	{
		if ((((float)curvalue / (float)maxvalue) * 100.0f) <= bar->m_nBlinkThreshold)
		{
			dwBarColor = bar->m_pBlinkData->m_dwBlinkColor;
			if (bar->m_pBlinkData->m_eBlinkState == UIBLINKSTATE_NOT_BLINKING)
			{
				UIComponentBlink(bar);
			}
		}
		else
		{
			bar->m_pBlinkData->m_eBlinkState = UIBLINKSTATE_NOT_BLINKING;
		}
	}

	//UIComponentSetVisible(bar, TRUE);
	if (msg == UIMSG_PAINT ||	// I want to be able to assume that if I send a paint message to a bar it will be repainted.  Hopefully they don't get them too much (as opposed to stat change messages)
		curvalue != bar->m_nOldValue || 
		maxvalue != bar->m_nOldMax || 
		!bar->m_pGfxElementFirst)
	{
		UIComponentRemoveAllElements(bar);
		
		sUIBarDraw(bar, dwBarColor, 0.0f, (float)maxvalue, 0.0f, (float)curvalue);

		if (bar->m_nShowIncreaseDuration > 0)
		{
			ASSERT_RETVAL(bar->m_pBlinkData, UIMSG_RET_NOT_HANDLED);

			UI_GFXELEMENT* pElement = sUIBarDraw(bar, dwBarColor, 0.0f, (float)maxvalue, (float)bar->m_nOldValue, (float)curvalue);
			UIElementStartBlink(pElement, bar->m_nShowIncreaseDuration, bar->m_pBlinkData->m_nBlinkPeriod, bar->m_pBlinkData->m_dwBlinkColor);
		}

		if (bar->m_nIncreaseLeftStat != INVALID_LINK && 
			unit)
		{
			int nChangeAmount = UnitGetStatShift(unit, bar->m_nIncreaseLeftStat);
			if (nChangeAmount > 0)
				sUIBarDraw(bar, dwBarColor, 0.0f, (float)maxvalue, 0.0f, (float)(curvalue + nChangeAmount));
		}

		bar->m_nOldValue = curvalue;
		bar->m_nOldMax = maxvalue;
	}


	// see if we need to schedule another paint
	if (bar->m_codeRegenValue != NULL_CODE)
	{
		int regenvalue = VMExecI(UnitGetGame(unit), unit, g_UI.m_pCodeBuffer + bar->m_codeRegenValue, 
			g_UI.m_nCodeCurr - bar->m_codeRegenValue);

		if ((regenvalue > 0 && curvalue < maxvalue) || (regenvalue < 0 && curvalue > 0))
		{
			if (bar->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
			{
				CSchedulerCancelEvent(bar->m_tMainAnimInfo.m_dwAnimTicket);
			}

			// schedule another one for one quarter second later
			bar->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + UI_BAR_REGEN_INTERVAL, bar, UIMSG_PAINT, 0, 0);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFocusUnitBarOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(msg == UIMSG_SETFOCUSSTAT, UIMSG_RET_NOT_HANDLED);

	if (wParam == UIComponentGetFocusUnitId(component))
	{
		return UIBarOnPaint(component, msg, wParam, lParam);
	}

	return UIMSG_RET_NOT_HANDLED;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFeedBarOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (pFocusUnit)
	{
		UI_BAR* pBar = UICastToBar(component);
		if (msg == UIMSG_SETHOVERUNIT ||
			msg == UIMSG_INVENTORYCHANGE)
		{
			// force repaint
			pBar->m_nOldValue = -1;
		}

		int nFeedChange = VMFeedChangeForHoverItem(pFocusUnit, pBar->m_nStat, 1);	// for these purposes the feed change is the net change considering stat change as well
																					//  so if feed goes up by 10 but the stat also goes up by 12 then this will be -2
		if (nFeedChange)
		{
			int nBaseFeedChange = VMFeedChangeForHoverItem(pFocusUnit, pBar->m_nStat, 0);			

			const STATS_DATA *pStatsData = StatsGetData(AppGetCltGame(), pBar->m_nStat);
			int nMaxStat = StatsGetAssociatedStat(pStatsData, 0);

			int nCurrentFeed = UnitGetStat(pFocusUnit, pBar->m_nStat);
			int nStatChange = nBaseFeedChange - nFeedChange;

			int nNewFeedTotal = nCurrentFeed + nBaseFeedChange;

			int nCurrentFeedMax = UnitGetStat(pFocusUnit, nMaxStat);
			int nNewFeedMax = nCurrentFeedMax + nStatChange;

			if (nNewFeedTotal > nNewFeedMax)
			{
				UIComponentRemoveAllElements(pBar);
				sUIBarDraw(pBar, pBar->m_dwAltColor, 0.0f, (float)pBar->m_nMaxValue, 0.0f, (float)pBar->m_nMaxValue);
				return UIMSG_RET_HANDLED;
			}
			
			float fCurrentPct = (float)nCurrentFeed * 100.0f / (float)nCurrentFeedMax;
			float fNewPct = (float)nNewFeedTotal * 100.0f / (float)nNewFeedMax;
			
			if (fNewPct > fCurrentPct)
			{
				UIComponentRemoveAllElements(pBar);
				sUIBarDraw(pBar, pBar->m_dwColor, 0.0f, 100.0f, 0.0f, fCurrentPct);
				sUIBarDraw(pBar, pBar->m_dwAltColor2, 0.0f, 100.0f, fCurrentPct, fNewPct);
			}
			else
			{
				UIComponentRemoveAllElements(pBar);
				sUIBarDraw(pBar, pBar->m_dwColor, 0.0f, 100.0f, 0.0f, fNewPct);
				sUIBarDraw(pBar, pBar->m_dwAltColor3, 0.0f, 100.0f, fNewPct, fCurrentPct);
				return UIMSG_RET_HANDLED;
			}
		}

		//int nChange = VMFeedChangeForHoverItem(pFocusUnit, pBar->m_nStat, 1);
		//if (nChange)
		//{
		//	if (nChange + pBar->m_nValue > pBar->m_nMaxValue)
		//	{
		//		UIComponentRemoveAllElements(pBar);
		//		sUIBarDraw(pBar, pBar->m_dwAltColor, 0.0f, (float)pBar->m_nMaxValue, 0.0f, (float)pBar->m_nMaxValue);
		//		return UIMSG_RET_HANDLED;
		//	}
		//	else if (nChange > 0)
		//	{
		//		UIBarOnPaint(component, msg, wParam, lParam);
		//		sUIBarDraw(pBar, pBar->m_dwAltColor2, 0.0f, (float)pBar->m_nMaxValue, (float)pBar->m_nValue, (float)(pBar->m_nValue + nChange));
		//	}
		//	else
		//	{
		//		UIComponentRemoveAllElements(pBar);
		//		sUIBarDraw(pBar, pBar->m_dwColor, 0.0f, (float)pBar->m_nMaxValue, 0.0f, (float)(pBar->m_nValue + nChange));
		//		sUIBarDraw(pBar, pBar->m_dwAltColor3, 0.0f, (float)pBar->m_nMaxValue, (float)(pBar->m_nValue + nChange), (float)pBar->m_nValue);
		//		return UIMSG_RET_HANDLED;
		//	}
		//}

		else
		{
			return UIBarOnPaint(component, msg, wParam, lParam);
		}

	}
	return UIMSG_RET_NOT_HANDLED;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatDisplPaintIcon(
	UI_STATDISPL *displ, 
	UNIT *unit)
{
	UIComponentRemoveAllElements(displ);
	displ->m_fHeight = 0.0f;
	displ->m_fWidth = 0.0f;

	UIX_TEXTURE_FONT* font = UIComponentGetFont(displ);
	//ASSERT_RETURN(font);
	if (!font)
		return;

	UIX_TEXTURE* texture = UIComponentGetTexture(displ);
	if (!texture)
		return;

	int nFontSize = UIComponentGetFontSize(displ, font);
	float fWidthRatio = 1.0f;//fontsize / font->m_fHeight;

	if (!EvaluateDisplayCondition(AppGetCltGame(), displ->m_eTable, displ->m_nDisplayLine, NULL, 0, unit, NULL))
	{
		return;
	}

static const int	MAX_CHAR_BUF = 256;
	WCHAR szIconText[MAX_CHAR_BUF];
	WCHAR szDescripText[MAX_CHAR_BUF];
	CHAR  szIconFrame[MAX_CHAR_BUF];

	if (PrintStatsLineIcon( UnitGetGame(unit), 
		displ->m_eTable, displ->m_nDisplayLine, unit, NULL, szIconText, MAX_CHAR_BUF, szDescripText, MAX_CHAR_BUF, szIconFrame, MAX_CHAR_BUF))
	{
		UI_TEXTURE_FRAME *pIconFrame = NULL;
		if (PStrICmp(szIconFrame, "*dmg icon w/text") == 0)
		{
			const UNIT_DATA *pUnitData = UnitGetData(unit);
			ASSERT_RETURN(pUnitData);

			pIconFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, pUnitData->szDamageIcon);
			PStrCopy(szDescripText, StringTableGetStringByIndex(pUnitData->nDamageDescripString), MAX_CHAR_BUF);
		}
		else if (PStrICmp(szIconFrame, "*dmg icon") == 0)
		{
			const UNIT_DATA *pUnitData = UnitGetData(unit);
			ASSERT_RETURN(pUnitData);

			pIconFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, pUnitData->szDamageIcon);
			szDescripText[0] = L'\0';
		}
		else
		{
			pIconFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, szIconFrame);
		}

		float fIconTextWidth = 0.0f;
		float fIconTextHeight = 0.0f;
		float fDescripTextWidth = 0.0f;
		float fDescripTextHeight = 0.0f;
		UIElementGetTextLogSize(font, nFontSize, fWidthRatio, displ->m_bNoScaleFont, szIconText, &fIconTextWidth, &fIconTextHeight);
		UIElementGetTextLogSize(font, nFontSize, fWidthRatio, displ->m_bNoScaleFont, szDescripText, &fDescripTextWidth, &fDescripTextHeight);

		float fIconFrameWidth = 0.0f;
		float fIconFrameHeight = 0.0f;
		if (pIconFrame)
		{
			fIconFrameWidth =  pIconFrame->m_fWidth;// * UIGetLogToScreenRatioX(displ->m_bNoScale);
			fIconFrameHeight = pIconFrame->m_fHeight;// * UIGetLogToScreenRatioY(displ->m_bNoScale);
		}
		else
		{
			fIconFrameHeight = fIconTextHeight;
			fIconFrameWidth = fIconTextWidth;
		}

		float fExtraIconSpacing = MAX(0.0f, (fDescripTextWidth - fIconFrameWidth) / 2.0f);
		fIconFrameWidth = MAX(fIconFrameWidth, fDescripTextWidth);

		UI_POSITION	position;

		if (pIconFrame)
		{
			// Add the icon
			UI_POSITION posIcon = position;
			posIcon.m_fX += fExtraIconSpacing;
			UIComponentAddElement(displ, texture, pIconFrame, posIcon, GFXCOLOR_HOTPINK, NULL, TRUE);
			displ->m_fHeight = MAX(displ->m_fHeight, fIconFrameHeight);
			displ->m_fWidth = MAX(displ->m_fWidth, fIconFrameWidth);
		}

		if (szIconText && szIconText[0])
		{
			// add the damage text in the icon
			UIComponentAddTextElement(displ, texture, font, nFontSize, 
				szIconText, position, displ->m_dwColor, &UI_RECT(0.0f, 0.0f, displ->m_fWidth, displ->m_fHeight), 
				UIALIGN_CENTER, 
				&UI_SIZE(fIconFrameWidth, fIconFrameHeight));
			displ->m_fHeight = MAX(displ->m_fHeight, fIconTextHeight);
			displ->m_fWidth = MAX(displ->m_fWidth, fIconTextWidth);
		}

		if (szDescripText && szDescripText[0])
		{
			// Now add the description below the icon
			position.m_fY += fIconFrameHeight + displ->m_fSpecialSpacing;
			displ->m_fHeight += displ->m_fSpecialSpacing + fDescripTextHeight;
			displ->m_fWidth = MAX(displ->m_fWidth, fDescripTextWidth);
			UIComponentAddTextElement(displ, texture, font, nFontSize, 
				szDescripText, position, displ->m_dwColor, &UI_RECT(0.0f, 0.0f, displ->m_fWidth, displ->m_fHeight), 
				UIALIGN_CENTER, 
				&UI_SIZE(fIconFrameWidth, fDescripTextHeight));
		}
	}

	if (g_UI.m_bDebugTestingToggle)
		UIComponentAddPrimitiveElement(displ, UIPRIM_BOX, 0, UI_RECT(0, 0, displ->m_fWidth, displ->m_fHeight), NULL, NULL, GFXCOLOR_GREEN);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StatDisplPaintIcons(
	UI_STATDISPL *displ, 
	UNIT *unit,
	EXCELTABLE eDisplayTable,
	int nDisplayArea)
{
	// the pointers should already be valid from the calling function.  We won't check again right now.

static const int	MAX_LINES = 128;
static const int	MAX_CHAR_BUF = 256;
static const float	SPACING	= 1.0f;

	//clear
	UIComponentRemoveAllElements(displ);
	displ->m_fHeight = 0.0f;
	displ->m_fWidth = 0.0f;
	
	UIX_TEXTURE_FONT* font = UIComponentGetFont(displ);
	//ASSERT_RETURN(font);
	if (!font)
		return;

	UIX_TEXTURE* texture = UIComponentGetTexture(displ);
	if (!texture)
		return;

	int nFontSize = UIComponentGetFontSize(displ, font);
	float fWidthRatio = 1.0f; // fontsize / font->m_fHeight;

	int nValidLines[MAX_LINES];
	STATS *ppRiderStats[MAX_LINES];
	int nResultCount = EvaluateStatConditions( UnitGetGame(unit), eDisplayTable, nDisplayArea, unit, nValidLines, ppRiderStats, MAX_LINES, TRUE);

	for (int i=0; i < nResultCount; i++)
	{
		WCHAR szIconText[MAX_CHAR_BUF];
		WCHAR szDescripText[MAX_CHAR_BUF];
		CHAR  szIconFrame[MAX_CHAR_BUF];

		if (PrintStatsLineIcon( UnitGetGame(unit), 
			eDisplayTable, nValidLines[i], unit, ppRiderStats[i], szIconText, MAX_CHAR_BUF, szDescripText, MAX_CHAR_BUF, szIconFrame, MAX_CHAR_BUF))
		{
			UI_TEXTURE_FRAME *pIconFrame = NULL;
			if (PStrICmp(szIconFrame, "*dmg icon w/text") == 0)
			{
				const UNIT_DATA *pUnitData = UnitGetData(unit);
				ASSERT_RETURN(pUnitData);

				pIconFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, pUnitData->szDamageIcon);
				PStrCopy(szDescripText, StringTableGetStringByIndex(pUnitData->nDamageDescripString), MAX_CHAR_BUF);
			}
			else if (PStrICmp(szIconFrame, "*dmg icon") == 0)
			{
				const UNIT_DATA *pUnitData = UnitGetData(unit);
				ASSERT_RETURN(pUnitData);

				pIconFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, pUnitData->szDamageIcon);
				szDescripText[0] = L'\0';
			}
			else
			{
				pIconFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, szIconFrame);
			}

			float fIconTextWidth = 0.0f;
			float fIconTextHeight = 0.0f;
			float fDescripTextWidth = 0.0f;
			float fDescripTextHeight = 0.0f;
			UIElementGetTextLogSize((displ->m_pFont2 ? displ->m_pFont2 : font), (displ->m_pFont2 ? displ->m_nFontSize2 : nFontSize), (displ->m_pFont2 ? displ->m_pFont2->m_fWidthRatio : fWidthRatio), displ->m_bNoScaleFont, szIconText, &fIconTextWidth, &fIconTextHeight);
			UIElementGetTextLogSize(font, nFontSize, fWidthRatio, displ->m_bNoScaleFont, szDescripText, &fDescripTextWidth, &fDescripTextHeight);

			if (displ->m_bNoScaleFont && !displ->m_bNoScale)
			{
				// the text size is specified as screen coordinates (and logical coordinates)
				//   the component however will try to convert the sizes, so we need to pre-emptively scale them up
				fIconTextWidth /= UIGetLogToScreenRatioX();
				fIconTextHeight /= UIGetLogToScreenRatioY();
				fDescripTextWidth /= UIGetLogToScreenRatioX();
				fDescripTextHeight /= UIGetLogToScreenRatioY();
			}

			float fIconFrameWidth = 0.0f;
			float fIconFrameHeight = 0.0f;
			if (pIconFrame)
			{
				fIconFrameWidth =  pIconFrame->m_fWidth; //	* UIGetLogToScreenRatioX(displ->m_bNoScale);
				fIconFrameHeight = pIconFrame->m_fHeight; // * UIGetLogToScreenRatioY(displ->m_bNoScale);
			}
			else
			{
				continue;
				//fIconFrameHeight = fIconTextHeight;
				//fIconFrameWidth = fIconTextWidth;
			}

			float fTotalSegmentWidth = MAX(fIconFrameWidth, fDescripTextWidth);
			fTotalSegmentWidth = MAX(fTotalSegmentWidth, fIconTextWidth);
			float fExtraIconSpacing = MAX(0.0f, (fTotalSegmentWidth - fIconFrameWidth) / 2.0f);
			float fTotalSegmentHeight = MAX(fIconFrameHeight, fIconTextHeight);

			UI_POSITION	position;
			if (displ->m_bVertIcons)
			{
				position.m_fY = displ->m_fHeight;
				displ->m_fHeight += fTotalSegmentHeight + SPACING;
				displ->m_fWidth = MAX(displ->m_fWidth, fTotalSegmentWidth);
			}
			else
			{
				position.m_fX = displ->m_fWidth;
				displ->m_fWidth += fTotalSegmentWidth + SPACING;
				displ->m_fHeight = MAX(displ->m_fHeight, fTotalSegmentHeight);
			}

			DWORD dwTextColor = displ->m_dwColor;

			if (pIconFrame)
			{
				// Add the icon
				UI_POSITION posIcon = position;
				posIcon.m_fX += fExtraIconSpacing;			
				UIComponentAddElement(displ, texture, pIconFrame, posIcon, GFXCOLOR_HOTPINK, NULL, TRUE);
			}

			if (szIconText && szIconText[0])
			{
				// add the damage text in the icon
				UIComponentAddTextElement(displ, texture, 
					(displ->m_pFont2 ? displ->m_pFont2 : font), (displ->m_pFont2 ? displ->m_nFontSize2 : nFontSize), 
					szIconText, position, dwTextColor, &UI_RECT(0.0f, 0.0f, displ->m_fWidth, displ->m_fHeight), 
					UIALIGN_CENTER, 
					&UI_SIZE(fTotalSegmentWidth, fTotalSegmentHeight));
			}

			if (szDescripText && szDescripText[0])
			{
				// Now add the description below the icon
				position.m_fY += fTotalSegmentHeight + displ->m_fSpecialSpacing;
				if (displ->m_bVertIcons)
				{
					displ->m_fHeight += displ->m_fSpecialSpacing + fDescripTextHeight;
				}
				else
				{
					displ->m_fHeight = MAX(displ->m_fHeight, fIconFrameHeight + fDescripTextHeight + displ->m_fSpecialSpacing);
				}
				UIComponentAddTextElement(displ, texture, 
					font, nFontSize, 
					szDescripText, position, dwTextColor, &UI_RECT(0.0f, 0.0f, displ->m_fWidth, displ->m_fHeight), 
					UIALIGN_CENTER, 
					&UI_SIZE(fTotalSegmentWidth, fDescripTextHeight));
			}
		}
	}

	if (g_UI.m_bDebugTestingToggle)
		UIComponentAddPrimitiveElement(displ, UIPRIM_BOX, 0, UI_RECT(0, 0, displ->m_fWidth, displ->m_fHeight), NULL, NULL, GFXCOLOR_GREEN);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatDisplOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_STATDISPL* displ = UICastToStatDispL(component);
	ASSERT_RETVAL(displ, UIMSG_RET_NOT_HANDLED);

	if (msg == UIMSG_SETFOCUSSTAT)
	{
		if (wParam != UIComponentGetFocusUnitId(component))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	}

#define TEXT_BUFFER_SIZE 4096
	WCHAR szStatText[TEXT_BUFFER_SIZE];
	szStatText[0] = 0;

	UNIT* unit = UIComponentGetFocusUnit(displ);
	if (displ->m_eTable != DATATABLE_NONE &&
		unit != NULL &&
		(displ->m_nUnitType == INVALID_LINK || UnitIsA(unit, displ->m_nUnitType)))
	{
		UIX_TEXTURE_FONT* font = UIComponentGetFont(displ);
		//ASSERT_RETZERO(font);
		if (!font)
			return UIMSG_RET_NOT_HANDLED;

		if (displ->m_nDisplayLine != -1)
		{
			if (displ->m_bShowIcons)
			{
				StatDisplPaintIcon(displ, unit);
			}
			else
			{
				PrintStatsLine(UnitGetGame(unit), displ->m_eTable, displ->m_nDisplayLine, NULL, unit, szStatText, TEXT_BUFFER_SIZE, -1, FALSE);
				UILabelSetText(component, szStatText);
			}
		}
		else if (displ->m_nDisplayArea != -1)
		{

			//// even though we totally hide the important magic affixes for unidentified items
			//// from all clients, an item still has a set of "standard" things that they client
			//// knows about, which we want to hide here cause it makes it more mysterious, 
			//// people could totally hack this display, but the base damage and feed requirements
			//// aren't really interesting enough to also super hide all that too -Colin
			//if( /*displ->m_nDisplayArea == SDTTA_OTHER && */
			//	UnitGetGenus( unit ) == GENUS_ITEM &&
			//	ItemIsUnidentified( unit ))
			//{
			//	UILabelSetText(component, L"" );
			//	return UIMSG_RET_HANDLED;
			//}
			//if( ItemBelongsToAnyMerchant( unit ) &&
			//	UnitIsGambler( ItemGetMerchant(unit) ) &&
			//	!( displ->m_nDisplayArea == SDTTA_PRICE ||
			//	   displ->m_nDisplayArea == SDTTA_ITEMTYPES ||
			//	   displ->m_nDisplayArea == SDTTA_REQUIREMENTS ) )
			//{
			//	UILabelSetText(component, L"" );
			//	return UIMSG_RET_HANDLED;
			//}

			if (displ->m_bShowIcons)
			{
				StatDisplPaintIcons(displ, unit, displ->m_eTable, displ->m_nDisplayArea);
			}
			else
			{

				WCHAR szTemp[TEXT_BUFFER_SIZE];
				szStatText[0] = 0;
				BOOL bFound = PrintStats(UnitGetGame(unit), displ->m_eTable, displ->m_nDisplayArea, unit, szTemp, TEXT_BUFFER_SIZE);
				if( AppIsTugboat())
				{
					WCHAR szTemp2[TEXT_BUFFER_SIZE];
					bFound = bFound | PrintStatsForUnitTypes(UnitGetGame(unit), displ->m_eTable, displ->m_nDisplayArea, unit, szTemp, szTemp2, TEXT_BUFFER_SIZE);
				}
				if (bFound)
				{
					//PStrCat(szStatText, L"\n", TEXT_BUFFER_SIZE);
					PStrCat(szStatText, szTemp, TEXT_BUFFER_SIZE);
				}
				UILabelSetText(component, szStatText);
			}
		}
	}
	else
	{
		UILabelSetText(component, L"");
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatDisplOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentActivate(UICOMP_STATSTOOLTIP, FALSE);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStatDisplOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_STATDISPL* displ = UICastToStatDispL(component);
	ASSERT_RETVAL(displ, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (UIGetCursorUnit() != INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT* unit = UIComponentGetFocusUnit(displ);
	if (displ->m_nDisplayLine == -1 ||
		displ->m_eTable == DATATABLE_NONE ||
		unit == NULL)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	const STATS_DISPLAY* display = (const STATS_DISPLAY*)ExcelGetData(AppGetCltGame(), displ->m_eTable, displ->m_nDisplayLine);
	if (!display)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (displ->m_szTooltipText ||
		display->nToolTipText <= INVALID_LINK)
	{
		return UIComponentMouseHoverLong(component, msg, wParam, lParam);
	}

	UIClearHoverText(UICOMP_STATSTOOLTIP);

	UI_COMPONENT* pComponent = UIComponentGetByEnum(UICOMP_STATSTOOLTIP);
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);
	UI_TOOLTIP *pTooltip = UICastToTooltip(pComponent);

	UIComponentSetFocusUnit(pTooltip, UnitGetId(unit));

	UI_COMPONENT *pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_LABEL)
		{
			#if ISVERSION(DEVELOPMENT)
				const char *pszKey = StringTableGetKeyByStringIndex( display->nToolTipText );
				UILabelSetTextByStringKey( pChild, pszKey );
			#else
				UILabelSetText(pChild, StringTableGetStringByIndex(display->nToolTipText));
			#endif
//			UIComponentAddBoxElement(pChild, 0, 0, pChild->m_fWidth, pChild->m_fHeight, NULL, UI_HILITE_GREEN, 100);
		}
		pChild = pChild->m_pNextSibling;
	}

	UITooltipDetermineSize(pTooltip);
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(displ);
	UITooltipPosition(
		pTooltip,
		&UI_RECT(pos.m_fX, 
				 pos.m_fY, 
				 pos.m_fX + displ->m_fWidth, 
				 pos.m_fY + displ->m_fHeight));

	if (!UIComponentGetVisible(pTooltip))
	{
		UIComponentActivate(pTooltip, TRUE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICursorAnimate(
	GAME* game, 
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT* component = (UI_COMPONENT*)data.m_Data1;
	if (!component)
	{
		return;
	}
	float fTimeRatio = ((float)(AppCommonGetCurTime() - component->m_tMainAnimInfo.m_tiAnimStart))/(float)component->m_tMainAnimInfo.m_dwAnimDuration;

	switch (component->m_eState)
	{
	case UISTATE_ACTIVATING:
		//trace("fading in %0.2f, %0.2f\n", component->m_fFading, fTimeRatio);
		if (fTimeRatio >= 1.0f)
		{
			component->m_eState = UISTATE_ACTIVE;
			component->m_fFading = 0.0f;
		}
		else
		{
			component->m_fFading = 1.0f - fTimeRatio;
			CSchedulerRegisterEventImm(UICursorAnimate, CEVENT_DATA((DWORD_PTR)component));
		}
		break;
	case UISTATE_INACTIVATING:
		//trace("fading out %0.2f, %0.2f\n", component->m_fFading, fTimeRatio);
		if (fTimeRatio >= 1.0f)
		{
			component->m_eState = UISTATE_INACTIVE;
			component->m_fFading = 0.0f;
			UIComponentSetVisible(component, FALSE);
		}
		else
		{
			component->m_fFading = fTimeRatio;
			CSchedulerRegisterEventImm(UICursorAnimate, CEVENT_DATA((DWORD_PTR)component));
		}
		break;
	}

	UI_COMPONENT *pChild = component->m_pFirstChild;
	while (pChild)
	{
		pChild->m_fFading = component->m_fFading;
		UISetNeedToRender(pChild->m_nRenderSection);
		pChild = pChild->m_pNextSibling;
	}

	UISetNeedToRender(component->m_nRenderSection);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICursorOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_CURSOR *pCursor = UICastToCursor(component);	

	UI_COMPONENT *pChild = pCursor->m_pFirstChild;
	UIComponentRemoveAllElements(pCursor);
	if (pChild)
		UIComponentRemoveAllElements(pChild);
	if( InputIsCommandKeyDown( CMD_CAMERAORBIT ) && c_CameraGetViewType() == VIEW_VANITY  )
	{
		return UIMSG_RET_HANDLED;
	}


	if (pCursor->m_eCursorState == UICURSOR_STATE_ITEM)
	{
		UNITID idCursor = UIGetCursorUnit();
		ASSERT_RETVAL(idCursor != INVALID_ID, UIMSG_RET_NOT_HANDLED);
		UNIT *item = UnitGetById(AppGetCltGame(), idCursor);
		if (!item)		
		{
			// This item went away.  It could've been dropped.
			UIClearCursor();
			// fall through and draw the proper frame
		}
		else
		{
			if( AppIsTugboat() && item->pIconTexture )  // ------------------------------------------------------------
			{
				BOOL bLoaded = UITextureLoadFinalize( item->pIconTexture );
				REF( bLoaded );
				UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(item->pIconTexture->m_pFrames, "icon");
				if (frame)
				{
					UI_SIZE sizeIcon;
					sizeIcon.m_fWidth = (float)item->pUnitData->nInvWidth * ICON_BASE_TEXTURE_WIDTH;
					sizeIcon.m_fHeight = (float)+item->pUnitData->nInvHeight * ICON_BASE_TEXTURE_WIDTH;


					UIComponentAddElement(pChild, item->pIconTexture, frame, UI_POSITION(-sizeIcon.m_fWidth * .25f, -sizeIcon.m_fWidth * .25f), GFXCOLOR_WHITE, NULL, FALSE, 1.0f, 1.0f, &sizeIcon );
				}
			}				// ------------------------------------------------------------
			else
			{
				if (pCursor->m_fItemDrawWidth == 0.0f || pCursor->m_fItemDrawHeight == 0.0f)
				{
					UIGetStdItemCursorSize(item, pCursor->m_fItemDrawWidth, pCursor->m_fItemDrawHeight);
				}

				if (g_UI.m_bDebugTestingToggle)
				{
					UIComponentAddBoxElement(pCursor, pCursor->m_tItemAdjust.fXOffset, pCursor->m_tItemAdjust.fYOffset, 
						pCursor->m_fItemDrawWidth + pCursor->m_tItemAdjust.fXOffset, 
						pCursor->m_fItemDrawHeight + pCursor->m_tItemAdjust.fYOffset, 
						NULL, UI_HILITE_GREEN, 100);
				}

				UIComponentAddItemGFXElement(pCursor, item, 
					UI_RECT(0.0f, 0.0f, pCursor->m_fItemDrawWidth, pCursor->m_fItemDrawHeight), NULL, &pCursor->m_tItemAdjust);
			}
		}
	}
	else if (pCursor->m_eCursorState == UICURSOR_STATE_SKILLDRAG)
	{
		int nSkillID = pCursor->m_nSkillID;
		if (nSkillID != INVALID_ID)
		{
			UI_TEXTURE_FRAME *pBackground = NULL; //AppIsHellgate() ? UIGetStdSkillIconBackground() : NULL;
			UI_TEXTURE_FRAME *pBorder = NULL; //UIGetStdSkillIconBorder();
			float Width, Height;
			//if( AppIsHellgate() )
			//{
			//	Width = pBackground->m_fWidth;
			//	Height = pBackground->m_fHeight;
			//}
			//else
			{
				Width = pCursor->m_pCursorFrame[UICURSOR_STATE_POINTER]->m_fWidth;
				Height = pCursor->m_pCursorFrame[UICURSOR_STATE_POINTER]->m_fHeight;
			}
			UI_DRAWSKILLICON_PARAMS tParams(pChild, UI_RECT(), UI_POSITION(), nSkillID, pBackground, pBorder, NULL, NULL);
			tParams.pBackgroundComponent = pCursor;
			tParams.pTexture = UIGetStdSkillIconBackgroundTexture();
			if( AppIsHellgate() )
			{
				tParams.fSkillIconWidth = Width - 2.0f;
				tParams.fSkillIconHeight = Height - 2.0f;
			}
			else
			{
				tParams.bSmallIcon = FALSE;
				tParams.bShowHotkey = FALSE;
				tParams.fSkillIconWidth = Width;
				tParams.fSkillIconHeight = Height;
			}

			UI_RECT rect;
			if( AppIsHellgate() )
			{
				tParams.rect.m_fX1 = Width / -2.0f;
				tParams.rect.m_fY1 = Height / -2.0f;
				tParams.rect.m_fX2 = tParams.rect.m_fX1 + Width;
				tParams.rect.m_fY2 = tParams.rect.m_fY1 + Height;
			}
			else
			{
				tParams.rect.m_fX1 = Width / -2.0f;
				tParams.rect.m_fY1 = Height / -2.0f;
				tParams.rect.m_fX2 = tParams.rect.m_fX1 + Width;
				tParams.rect.m_fY2 = tParams.rect.m_fY1 + Height;
				tParams.pos.m_fX = tParams.rect.m_fX1;
				tParams.pos.m_fY = tParams.rect.m_fY1;

				
			}

			UIDrawSkillIcon(tParams);
		}
	}

	if (pChild && pCursor->m_pCursorFrame[pCursor->m_eCursorState] && 
		!AppIsTugboat() )
	{
		UIComponentAddElement(pChild, pCursor->m_pCursorFrameTexture[pCursor->m_eCursorState], pCursor->m_pCursorFrame[pCursor->m_eCursorState], UI_POSITION(0.0f, 0.0f));
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIGetCursorActivateSkill()
{
	if (!g_UI.m_Cursor)
		return INVALID_ID;

	return g_UI.m_Cursor->m_nItemPickActivateSkillID;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CURSOR_STATE UIGetCursorState()
{
	if (!g_UI.m_Cursor)
		return UICURSOR_STATE_INVALID;

	return g_UI.m_Cursor->m_eCursorState;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetCursorState(
	CURSOR_STATE eState,
	DWORD dwParam /*= 0*/,
	BOOL bForce /*= FALSE*/ )
{
	if (g_UI.m_Cursor)
	{
		if (!bForce && g_UI.m_Cursor->m_eCursorState == eState)
			return;

		if (g_UI.m_Cursor->m_eCursorState == UICURSOR_STATE_SKILL_PICK_ITEM &&
			AppIsHellgate() )
		{
			ASSERT_RETURN(g_UI.m_Cursor->m_nItemPickActivateSkillID != INVALID_ID);
			const SKILL_DATA * pSkillData = SkillGetData(AppGetCltGame(), g_UI.m_Cursor->m_nItemPickActivateSkillID);
			ASSERT_RETURN(pSkillData);
			c_SkillControlUnitRequestStartSkill(AppGetCltGame(), pSkillData->pnFallbackSkills[0], NULL);
		}

		ASSERT_RETURN(eState >= 0 && eState < NUM_UICURSOR_STATES);

		// if there's still an item in the location, we won't allow the cursor to switch states.
		GAME *pGame = AppGetCltGame();
		if (pGame)
		{
			int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
			UNIT * pPlayer = GameGetControlUnit(pGame);
			if (pPlayer)
			{
				UNIT *pCursorUnit = UnitInventoryGetByLocation(pPlayer, nCursorLocation);
				if (pCursorUnit)
				{
					eState = UICURSOR_STATE_ITEM;
				}
			}
		}
		if (!bForce && g_UI.m_Cursor->m_eCursorState == eState)
			return; //why reset everything again.

		g_UI.m_Cursor->m_eCursorState = eState;

		if (eState != UICURSOR_STATE_ITEM &&
			eState != UICURSOR_STATE_IDENTIFY &&
			eState != UICURSOR_STATE_DISMANTLE &&
			eState != UICURSOR_STATE_ITEM_PICK_ITEM )
		{
			g_UI.m_Cursor->m_idUnit = INVALID_ID;
		}
		
					
		if (eState == UICURSOR_STATE_SKILL_PICK_ITEM)
		{
			g_UI.m_Cursor->m_nItemPickActivateSkillID = (int)dwParam;
			UIRepaint();
		}
		else
		{
			g_UI.m_Cursor->m_nItemPickActivateSkillID = INVALID_ID;
		}
		
		if (AppIsTugboat())
		{
			
			UI_CURSOR *pCursor = g_UI.m_Cursor;	
			UI_COMPONENT *pChild = pCursor->m_pFirstChild;
			UIComponentRemoveAllElements(pCursor);
			if (pChild)
				UIComponentRemoveAllElements(pChild);
			// No more! Doing hardware cursor now!
			/*
			if (pChild && pCursor->m_pCursorFrame[pCursor->m_eCursorState])
			{
				UIComponentAddElement(pChild, pCursor->m_pCursorFrameTexture[pCursor->m_eCursorState], pCursor->m_pCursorFrame[pCursor->m_eCursorState], UI_POSITION(0.0f, 0.0f));
			}*/

			UIUpdateHardwareCursor();

			return;
		}

		UIComponentHandleUIMessage(g_UI.m_Cursor, UIMSG_PAINT, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdateHardwareCursor( void )
{
	if( g_UI.m_Cursor )
	{
		if( InputIsCommandKeyDown( CMD_CAMERAORBIT ) && ( InputGetAdvancedCamera() || c_CameraGetViewType() == VIEW_VANITY ) ) 
		{
			::ShowCursor( FALSE );
			SetCursor( NULL );
			UIComponentHandleUIMessage(g_UI.m_Cursor, UIMSG_PAINT, 0, 0);
		}
		else
		{
			if( !GetCursor() )
			{
				UIComponentHandleUIMessage(g_UI.m_Cursor, UIMSG_PAINT, 0, 0);
			}
			switch( g_UI.m_Cursor->m_eCursorState )
			{
			case UICURSOR_STATE_WAIT :
				::ShowCursor( TRUE );
				SetCursor( LoadCursor( AppCommonGetHInstance(), MAKEINTRESOURCE( IDC_MYT_HOURGLASS ) ) );
				break;
			case UICURSOR_STATE_IDENTIFY :
				::ShowCursor( TRUE );
				SetCursor( LoadCursor( AppCommonGetHInstance(), MAKEINTRESOURCE( IDC_MYT_POINTERIDENT ) ) );
				break;
			case UICURSOR_STATE_INITIATE_ATTACK :
				::ShowCursor( TRUE );
				SetCursor( LoadCursor( AppCommonGetHInstance(), MAKEINTRESOURCE( IDC_MYT_ATTACK ) ) );
				break;
			case UICURSOR_STATE_ITEM :
			case UICURSOR_STATE_SKILLDRAG :
				::ShowCursor( FALSE );
				SetCursor( NULL );
				break;
			default:
				::ShowCursor( TRUE );
				SetCursor( LoadCursor( AppCommonGetHInstance(), MAKEINTRESOURCE( IDC_MYT_POINTER ) ) );
				break;
			}
		}
		
	}
	else
	{
		::ShowCursor( TRUE );
		SetCursor( LoadCursor( AppCommonGetHInstance(), MAKEINTRESOURCE( IDC_MYT_HOURGLASS ) ) );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetCursorStateBasedOnItem(
	UI_CURSOR *cursor,
	UNITID idUnit)
{
	UIX_TEXTURE* texture = UIComponentGetTexture(cursor);
	ASSERT_RETURN(texture);

	if (idUnit == INVALID_ID)
	{
		if (cursor->m_nSkillID == INVALID_LINK)
		{
			UISetCursorState(UICURSOR_STATE_POINTER);
		}
		return;
	}

	if (idUnit == INVALID_ID)
	{
		UNITID idHoverUnit = UIGetHoverUnit(NULL);
		if (idHoverUnit != INVALID_ID)
			UISetCursorState(UICURSOR_STATE_OVERITEM);
		else
			UISetCursorState(UICURSOR_STATE_POINTER);
		return;
	}

	UISetCursorState(UICURSOR_STATE_ITEM);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICursorOnInventoryChange(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	GAME *pGame = AppGetCltGame();
	ASSERT_RETVAL(pGame, UIMSG_RET_NOT_HANDLED);

	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	
	UI_CURSOR* cursor = UICastToCursor(component);

	if ((UNITID)wParam == INVALID_ID)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pContainer = UnitGetById(pGame, (UNITID)wParam);
	ASSERT_RETVAL(pContainer, UIMSG_RET_NOT_HANDLED);
	
	UNIT *pControlUnit = UIGetControlUnit();
	if (pControlUnit != pContainer)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UNIT *pItem = UnitInventoryGetByLocation(pContainer, nCursorLocation);
	if (g_UI.m_Cursor->m_idUnit != INVALID_ID)
	{
		UNIT *pReferenceItem = UnitGetById(pGame, g_UI.m_Cursor->m_idUnit);
		if (!pReferenceItem ||
			ItemBelongsToAnyMerchant(pReferenceItem) ||
			UnitGetUltimateContainer(pReferenceItem) != pControlUnit)
		{
			UIClearCursorUnitReferenceOnly();
		}
	}
	UNITID idItem = pItem ? UnitGetId(pItem) : g_UI.m_Cursor->m_idUnit;

	sSetCursorStateBasedOnItem(cursor, idItem);

	if (idItem == INVALID_ID)
	{
		// if we happened to be putting an item down, this should pop the tooltip of that item up
		UISendHoverMessage(FALSE);
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	//UISendMessage(WM_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIClearCursor(
	void)
{
	ASSERT_RETURN(g_UI.m_Cursor);

	UNIT *pCursorUnit = (g_UI.m_Cursor->m_idUnit != INVALID_ID ? UnitGetById(AppGetCltGame(), g_UI.m_Cursor->m_idUnit) : NULL);
	if (pCursorUnit && ItemBelongsToAnyMerchant(pCursorUnit))
	{
		UIPlayItemPutdownSound(pCursorUnit, NULL, INVLOC_NONE );
	}

	g_UI.m_Cursor->m_idUnit = INVALID_ID;

	GAME *pGame = AppGetCltGame();
	if (pGame)
	{
		int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
		UNIT * pPlayer = GameGetControlUnit(pGame);

		if (pPlayer)
		{
			pCursorUnit = UnitInventoryGetByLocation(pPlayer, nCursorLocation);
			if (pCursorUnit)
			{
				UNIT *pContainer = UnitGetById(pGame, g_UI.m_Cursor->m_idLastInvContainer);
				if (pContainer && InventoryTestPut(pContainer, pCursorUnit, g_UI.m_Cursor->m_nLastInvLocation, g_UI.m_Cursor->m_nLastInvX, g_UI.m_Cursor->m_nLastInvY, 0))
				{
					// these (if successful) will result in an INVCHANGE message which will be handled above
					c_InventoryTryPut(
						pContainer,
						pCursorUnit,
						g_UI.m_Cursor->m_nLastInvLocation,
						g_UI.m_Cursor->m_nLastInvX,
						g_UI.m_Cursor->m_nLastInvY);
					UIPlayItemPutdownSound(pCursorUnit, pContainer, g_UI.m_Cursor->m_nLastInvLocation);
				}
				else
				{
					int nBigGridLoc = GlobalIndexGet( GI_INVENTORY_LOCATION_BIGPACK);
					int x, y;
					if (g_UI.m_Cursor->m_nFromWeaponConfig != -1)
					{
						// Try to put it back in the weapon config from whence it came
						MSG_CCMD_ADD_WEAPONCONFIG msg;
						msg.idItem = UnitGetId(pCursorUnit);
						msg.bHotkey = (BYTE)g_UI.m_Cursor->m_nFromWeaponConfig;
						msg.nFromWeaponConfig = -1;
						msg.bSuggestedPos = (BYTE)0;
						c_SendMessage(CCMD_ADD_WEAPONCONFIG, &msg);
						UIClearCursorUnitReferenceOnly();
						UIPlayItemPutdownSound(pCursorUnit, pContainer, g_UI.m_Cursor->m_nLastInvLocation);
					}
					else if (UnitInventoryFindSpace(pPlayer, pCursorUnit, nBigGridLoc, &x, &y))
					{
						// these (if successful) will result in an INVCHANGE message which will be handled above				
						c_InventoryTryPut( pPlayer, pCursorUnit, nBigGridLoc, x, y );
						UIPlayItemPutdownSound(pCursorUnit, pPlayer, nBigGridLoc);
					}
					else
					{
						// well, there's nowhere to go.  I guess it'll have to stick around
						return;
					}
				}
			}
		}
	}

	g_UI.m_Cursor->m_nFromWeaponConfig = -1;
	g_UI.m_Cursor->m_idLastInvContainer = INVALID_ID;
	g_UI.m_Cursor->m_nLastInvLocation = INVLOC_NONE;
	g_UI.m_Cursor->m_nLastInvX = -1;
	g_UI.m_Cursor->m_nLastInvY = -1;
	g_UI.m_Cursor->m_nSkillID = INVALID_ID;
	sSetCursorStateBasedOnItem(g_UI.m_Cursor, INVALID_ID);

	UISendHoverMessage(FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIClearCursorUnitReferenceOnly(
	void)
{
	ASSERT_RETURN(g_UI.m_Cursor);

	if (g_UI.m_Cursor->m_idUnit == INVALID_ID)
		return;

	sSetCursorStateBasedOnItem(g_UI.m_Cursor, INVALID_ID);
	g_UI.m_Cursor->m_idUnit = INVALID_ID;
	g_UI.m_Cursor->m_nFromWeaponConfig = -1;
	g_UI.m_Cursor->m_idLastInvContainer = INVALID_ID;
	g_UI.m_Cursor->m_nLastInvLocation = INVLOC_NONE;
	g_UI.m_Cursor->m_nLastInvX = -1;
	g_UI.m_Cursor->m_nLastInvY = -1;

	UISendHoverMessage(FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UISetCursorUnit(
	UNITID idUnit,
	BOOL bSendMouseMove,
	int nFromWeaponConfig /*= -1*/,
	float fOffsetPctX /*= 0.0f*/,
	float fOffsetPctY /*= 0.0f*/,
	BOOL bForceReference /*= FALSE*/)
{
	ASSERT_RETFALSE(g_UI.m_Cursor);

	if (!bForceReference &&
		UIGetCursorUnit() == idUnit)
	{
		return FALSE;
	}

	GAME *pGame = AppGetCltGame();
	ASSERT_RETFALSE(pGame);
	UNIT *pUnitToPickup = UnitGetById(pGame, idUnit);
	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);

	UNIT * pPlayer = GameGetControlUnit(pGame);
	ASSERT_RETFALSE(pPlayer);

	if (!bForceReference)
	// clearing cursor first
		UIClearCursor();

	if (pUnitToPickup)
	{
		UNIT* pUnitToPickupOwner = UnitGetUltimateOwner(pUnitToPickup);
		g_UI.m_Cursor->m_nFromWeaponConfig = nFromWeaponConfig;

		// set some item model info
		g_UI.m_Cursor->m_tItemAdjust.fScale = 1.5f;

		UIGetStdItemCursorSize(pUnitToPickup, g_UI.m_Cursor->m_fItemDrawWidth, g_UI.m_Cursor->m_fItemDrawHeight);

		g_UI.m_Cursor->m_tItemAdjust.fXOffset = fOffsetPctX * g_UI.m_Cursor->m_fItemDrawWidth ;//* g_UI.m_Cursor->m_tItemAdjust.fScale;
		g_UI.m_Cursor->m_tItemAdjust.fYOffset = fOffsetPctY * g_UI.m_Cursor->m_fItemDrawHeight ;//* g_UI.m_Cursor->m_tItemAdjust.fScale;

		int nCurWeaponConfig = UnitGetStat(pPlayer, STATS_CURRENT_WEAPONCONFIG) + TAG_SELECTOR_WEAPCONFIG_HOTKEY1;

		// if the unit is coming from the current weapon config and it's not in any other WC, just move it completely to the cursor
		// if it is in multiple WCs or it's not from the current, just pick up a reference 
		if (g_UI.m_Cursor->m_nFromWeaponConfig == nCurWeaponConfig &&
			!ItemInMultipleWeaponConfigs(pUnitToPickup))
		{
			g_UI.m_Cursor->m_nFromWeaponConfig = -1;
		}

		int nCurrentLocation = INVLOC_NONE;
		if (UnitGetInventoryLocation(pUnitToPickup, &nCurrentLocation))
		{
			// if you're picking something up that you won't be able to put right back down,
			//    check to see if you have room to put it somewhere else.
			if (!bForceReference &&
				g_UI.m_Cursor->m_nFromWeaponConfig == -1  &&
				pPlayer == pUnitToPickupOwner &&
				!InventoryLocPlayerCanPut(UnitGetContainer(pUnitToPickup), pUnitToPickup, nCurrentLocation))
			{
				if (!ItemGetOpenLocationForPlayerToPutDown(pPlayer, pUnitToPickup, TRUE, TRUE ))
				{
					UIShowGenericDialog(StringTableGetStringByKey("item_pickup_no_room_header"), StringTableGetStringByKey("item_pickup_no_room"));
					return FALSE;
				}
			}
		}

		if (!bForceReference &&
			(g_UI.m_Cursor->m_nFromWeaponConfig == -1) &&
			pPlayer == pUnitToPickupOwner &&
			ItemBelongsToAnyMerchant(pUnitToPickup) == FALSE &&
			ItemIsInRewardLocation(pUnitToPickup) == FALSE)
		{
			if (InventoryTestPut( pPlayer, pUnitToPickup, nCursorLocation, 0, 0, 0 ))
			{
				g_UI.m_Cursor->m_idUnit = INVALID_ID;
				c_InventoryTryPut(pPlayer, pUnitToPickup, nCursorLocation, 0, 0);  // these (if successful) will result in an INVCHANGE message which will be handled above

				// save the last location of the item
				UnitGetInventoryLocation(pUnitToPickup, &g_UI.m_Cursor->m_nLastInvLocation, &g_UI.m_Cursor->m_nLastInvX, &g_UI.m_Cursor->m_nLastInvY);
				UNIT *pContainer = UnitGetContainer(pUnitToPickup);
				g_UI.m_Cursor->m_idLastInvContainer = (pContainer ? UnitGetId(pContainer) : INVALID_ID);

			}
		}
		else
		{
			g_UI.m_Cursor->m_idUnit = idUnit;
			sSetCursorStateBasedOnItem(g_UI.m_Cursor, g_UI.m_Cursor->m_idUnit);
			UIRepaint();																//force redraw of the inventory
		}

	}

	// We were probably hovering over an item when we picked this one up, so clear the hover item (and any tooltip with it)
	UISetHoverUnit(INVALID_ID, INVALID_ID);

	if (idUnit != INVALID_ID)
	{
		UIClearHoverText();
	}
	else if (bSendMouseMove)
	{
		float x, y;
		UIGetCursorPosition(&x, &y);

		UISendMessage(WM_MOUSEMOVE, 0, MAKE_PARAM((int)x, (int)y));
		UISendHoverMessage(FALSE);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetCursorSkill(
	int nSkillID,
	BOOL bSendMouseMove)
{
	ASSERT_RETURN(g_UI.m_Cursor);

	if (g_UI.m_Cursor->m_nSkillID == nSkillID)
		return;

	// clearing cursor first
	UIClearCursor();

	if (nSkillID != INVALID_ID)
		g_UI.m_Cursor->m_eCursorState = UICURSOR_STATE_SKILLDRAG;
	else
		g_UI.m_Cursor->m_eCursorState = UICURSOR_STATE_POINTER;

	g_UI.m_Cursor->m_nSkillID = nSkillID;

	// We were probably hovering over an item when we picked this one up, so clear the hover item (and any tooltip with it)
	UISetHoverUnit(INVALID_ID, INVALID_ID);

	if (nSkillID != INVALID_ID)
	{
		UIClearHoverText();
		int nSoundId = GlobalIndexGet(GI_SOUND_UI_SKILL_PICKUP);
		if (nSoundId != INVALID_LINK)
		{
			c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
		}
	}
	else if (bSendMouseMove)
	{
		float x, y;
		UIGetCursorPosition(&x, &y);

		UISendMessage(WM_MOUSEMOVE, 0, MAKE_PARAM((int)x, (int)y));
	}

	UIRepaint();																//force redraw of the inventory
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISetCursorUseUnit(
	UNITID idUnit)
{
	UI_CURSOR *pCursor = g_UI.m_Cursor;
	ASSERTX_RETURN( pCursor, "Expected cursor" );
	pCursor->m_idUseItem = idUnit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIGetCursorUnit(
	void)
{
	ASSERT_RETINVALID(g_UI.m_Cursor);

	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	
	UNIT *pContainer = UIGetControlUnit();
	if ( ! pContainer )
		return INVALID_ID;

	UNIT *pItem = NULL;
	if (UnitInventoryHasLocation(pContainer, nCursorLocation))
	{
		pItem = UnitInventoryGetByLocation(pContainer, nCursorLocation);
	}
	UNITID idItem = pItem ? UnitGetId(pItem) : g_UI.m_Cursor->m_idUnit;

	return idItem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UICursorUnitIsReference(
	void)
{
	ASSERT_RETINVALID(g_UI.m_Cursor);

	if (g_UI.m_Cursor->m_idUnit == INVALID_ID)
	{
		return FALSE;
	}

	// this rest may not be completely necessary, but let's make sure
	int nCursorLocation = GlobalIndexGet(GI_INVENTORY_LOCATION_CURSOR);
	
	UNIT *pContainer = UIGetControlUnit();
	if ( ! pContainer )
		return FALSE;

	UNIT *pItem = NULL;
	if (UnitInventoryHasLocation(pContainer, nCursorLocation))
	{
		pItem = UnitInventoryGetByLocation(pContainer, nCursorLocation);
	}

	return pItem == NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIGetCursorSkill(
	void)
{
	ASSERT_RETINVALID(g_UI.m_Cursor);

	return g_UI.m_Cursor->m_nSkillID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UICursorIsEmpty(
	void)
{
	UNITID idUnit = UIGetCursorUnit();
	int		nSkill = UIGetCursorSkill();
	return (idUnit == INVALID_ID &&
			nSkill == INVALID_ID);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIGetCursorUseUnit(
	void)
{
	UI_CURSOR *pCursor = g_UI.m_Cursor;
	ASSERTX_RETINVALID( pCursor, "Expected cursor" );
	return pCursor->m_idUseItem;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScreenOnRButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// Clear the cursor
	if (!UICursorIsEmpty())
	{
		UISetCursorUnit(INVALID_ID, FALSE);
	    UISetCursorSkill(INVALID_ID, FALSE);
		UIRepaint();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if ( UIGetCursorState() == UICURSOR_STATE_IDENTIFY ||
		 UIGetCursorState() == UICURSOR_STATE_DISMANTLE ||
		 UIGetCursorState() == UICURSOR_STATE_ITEM_PICK_ITEM )
	{
		UISetCursorUnit( INVALID_ID, TRUE );
		UISetCursorUseUnit( INVALID_ID );
		UISetCursorState( UICURSOR_STATE_POINTER );
		UIRepaint();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (UIGetCursorState() == UICURSOR_STATE_SKILL_PICK_ITEM)
	{		
		UISetCursorState(UICURSOR_STATE_POINTER);
		UIRepaint();
	}
	return UIMSG_RET_NOT_HANDLED;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) || !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	component->m_ScrollPos.m_fY += (int)lParam > 0 ? -component->m_fWheelScrollIncrement : component->m_fWheelScrollIncrement;
	component->m_ScrollPos.m_fY = PIN(component->m_ScrollPos.m_fY, 0.0f, component->m_fScrollVertMax);
	UIComponentHandleUIMessage(component, UIMSG_SCROLL, 0, lParam);
	UISetNeedToRender(component);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollOnMouseWheelRepaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!ResultIsHandled(UIScrollOnMouseWheel(component, msg, wParam, lParam)))
		return UIMSG_RET_NOT_HANDLED;

	if (component->m_pParent)
		UIComponentHandleUIMessage(component->m_pParent, UIMSG_PAINT, 0, 0, TRUE);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollChildBarOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentCheckBounds(component) || !UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	UI_COMPONENT *pBar = UIComponentIterateChildren(component, NULL, UITYPE_SCROLLBAR, FALSE);
	if (pBar)
	{
		UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pBar);
		pScrollBar->m_ScrollPos.m_fY += (int)lParam > 0 ? -pScrollBar->m_fWheelScrollIncrement : pScrollBar->m_fWheelScrollIncrement;
		pScrollBar->m_ScrollPos.m_fY = PIN(pScrollBar->m_ScrollPos.m_fY, pScrollBar->m_fMin, pScrollBar->m_fMax);
		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, lParam);
		UISetNeedToRender(pScrollBar);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sButtonScrollCallback(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	ASSERT(data.m_Data1);
	UI_BUTTONEX* button = (UI_BUTTONEX *)data.m_Data1;

    if (button->m_dwPushstate & UI_BUTTONSTATE_DOWN)
	{
		// May want to change this to send a message to the component which then does its own scrolling
		button->m_pScrollControl->m_ScrollPos.m_fY += button->m_fScrollIncrement;
		if( UIComponentIsScrollBar( button->m_pScrollControl ) )
		{
			UI_SCROLLBAR* pScrollBar = UICastToScrollBar( button->m_pScrollControl );
			if( pScrollBar->m_pScrollControl )
			{
				button->m_pScrollControl->m_ScrollPos.m_fY = PIN(button->m_pScrollControl->m_ScrollPos.m_fY, pScrollBar->m_fMin, pScrollBar->m_fMax);
			}
			else
			{
				button->m_pScrollControl->m_ScrollPos.m_fY = PIN(button->m_pScrollControl->m_ScrollPos.m_fY, 0.0f, button->m_pScrollControl->m_fScrollVertMax);
			}
		}
		else
		{
			button->m_pScrollControl->m_ScrollPos.m_fY = PIN(button->m_pScrollControl->m_ScrollPos.m_fY, 0.0f, button->m_pScrollControl->m_fScrollVertMax);
		}

		if (button->m_pParent)
			UIComponentHandleUIMessage(button->m_pParent, UIMSG_PAINT, 0, 0, TRUE);
		UIComponentHandleUIMessage(button->m_pScrollControl, UIMSG_SCROLL, 0, (LPARAM)button->m_fScrollIncrement);
		//UIComponentHandleUIMessage(button				   , UIMSG_PAINT, 0, 0);	//we may need to hide or show this button

		// Check again in a few milliseconds

		// this doesn't currently work while it's paused
		TIME tiCurrent = (button->m_bAnimatesWhenPaused ?  AppCommonGetAbsTime() : AppCommonGetCurTime());
		CSchedulerRegisterEvent(tiCurrent + button->m_dwRepeatMS, sButtonScrollCallback, data);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIButtonGetDown(
	UI_BUTTONEX * pButton)
{
	ASSERT_RETFALSE(pButton);
	return (pButton->m_dwPushstate & UI_BUTTONSTATE_DOWN) != 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIButtonGetDown(
	UI_COMPONENT *pParent,
	const char *szButtonName)
{
	ASSERT_RETFALSE(pParent);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pParent, szButtonName);
	if (!pComponent)
	{
		return FALSE;
	}

	UI_BUTTONEX *pButton = UICastToButton(pComponent);

	return UIButtonGetDown(pButton);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIButtonSetDown(
	UI_BUTTONEX * pButton,
	BOOL bDown)
{
	ASSERT_RETURN(pButton);
	if (bDown)
	{
		pButton->m_dwPushstate |= UI_BUTTONSTATE_DOWN;
	}
	else
	{
		pButton->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIButtonSetDown(
	UI_COMPONENT *pParent,
	const char *szButtonName,
	BOOL bDown)
{
	ASSERT_RETURN(pParent);

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pParent, szButtonName);
	if (!pComponent)
		return;

	UI_BUTTONEX *pButton = UICastToButton(pComponent);

	UIButtonSetDown(pButton, bDown);
}


UI_MSG_RETVAL UIButtonOnButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_BUTTONEX* button = UICastToButton(component);
	if (wParam == 0 && !UIComponentCheckBounds(button))
	{
		//button->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
		return UIMSG_RET_NOT_HANDLED;
	}

	if (button->m_eButtonStyle == UIBUTTONSTYLE_CHECKBOX)
	{
		// CHB 2007.02.01
		button->m_dwPushstate ^= UI_BUTTONSTATE_DOWN;
		if (button->m_dwPushstate & UI_BUTTONSTATE_DOWN)
		{
			if (button->m_nCheckOnSound != INVALID_LINK)
				c_SoundPlay(button->m_nCheckOnSound, &c_SoundGetListenerPosition(), NULL);
		}
		else
		{
			if (button->m_nCheckOffSound != INVALID_LINK)
				c_SoundPlay(button->m_nCheckOffSound, &c_SoundGetListenerPosition(), NULL);
		}
		
	}
	else
	{
		button->m_dwPushstate |= UI_BUTTONSTATE_DOWN;
	}

	if (button->m_pScrollControl && button->m_fScrollIncrement)
	{
		CSchedulerRegisterEventImm(sButtonScrollCallback, CEVENT_DATA((DWORD_PTR)button));
	}

	if (button->m_nAssocTab != -1 && button->m_pParent && UIComponentIsPanel(button->m_pParent))
	{
		UIPanelSetTab(UICastToPanel(button->m_pParent), button->m_nAssocTab);
	}

	if (button->m_eButtonStyle == UIBUTTONSTYLE_RADIOBUTTON)
	{
		// if this is a radiobutton, un-push all the other radio buttons on this "level"
		UI_COMPONENT *pTopLevel = button->m_pParent;
		for (int i=0; i < button->m_nRadioParentLevel; i++)
		{
			if (pTopLevel->m_pParent)
			{
				pTopLevel = pTopLevel->m_pParent;
			}
		}

		UI_COMPONENT *pOtherButton = NULL;
		while ((pOtherButton = UIComponentIterateChildren(pTopLevel, pOtherButton, UITYPE_BUTTON, TRUE )) != NULL)
		{
			if (pOtherButton != button)
			{
				UI_BUTTONEX *pBtn = UICastToButton(pOtherButton);
				if (pBtn->m_eButtonStyle == UIBUTTONSTYLE_RADIOBUTTON)
				{
					pBtn->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
					pBtn->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;		
					UIComponentHandleUIMessage(pBtn, UIMSG_PAINT, 0, 0);  
				}
			}
		}
	}

	UIComponentHandleUIMessage(button, UIMSG_PAINT, 0, 0);  
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnButtonUp(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetVisible(component))
	{
		UI_BUTTONEX* button = UICastToButton(component);

		if (button->m_eButtonStyle == UIBUTTONSTYLE_PUSHBUTTON)
		{
			button->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;

			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);  
		}

		return UIMSG_RET_HANDLED;
	}
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX* button = UICastToButton(component);
	ASSERT_RETVAL(button, UIMSG_RET_NOT_HANDLED);

	UI_TEXTURE_FRAME* pFrame = NULL;
	UI_TEXTURE_FRAME* pMidFrame = NULL;
	UI_TEXTURE_FRAME* pLeftFrame = NULL;
	UI_TEXTURE_FRAME* pRightFrame = NULL;

	BOOL bHover = (button->m_dwPushstate & UI_BUTTONSTATE_HOVER) != 0;
	BOOL bDown = (button->m_dwPushstate & UI_BUTTONSTATE_DOWN) != 0;

	if (button->m_nAssocTab != -1)
	{
		// Show the button as down if its associated tab is the current tab
		ASSERT_RETVAL(button->m_pParent, UIMSG_RET_NOT_HANDLED);
		UI_PANELEX *pParentPanel = UICastToPanel(button->m_pParent);
		ASSERT_RETVAL(pParentPanel, UIMSG_RET_NOT_HANDLED);
		if (pParentPanel->m_nCurrentTab == button->m_nAssocTab)
		{
			bDown = TRUE;
		}
	}

	if (bDown)
	{
		pFrame = button->m_pDownFrame;
		pMidFrame = button->m_pDownFrameMid;
		pLeftFrame = button->m_pDownFrameLeft;
		pRightFrame = button->m_pDownFrameRight;
		if (bHover)
		{
			if (button->m_pDownLitFrame)
				pFrame = button->m_pDownLitFrame;
			if (button->m_pDownLitFrameMid)
				pMidFrame = button->m_pDownLitFrameMid;
			if (button->m_pDownLitFrameLeft)
				pLeftFrame = button->m_pDownLitFrameLeft;
			if (button->m_pDownLitFrameRight)
				pRightFrame = button->m_pDownLitFrameRight;
		}
	}
	else if (bHover && button->m_pLitFrame != NULL && !button->m_bOverlayLitFrame)
	{
		pFrame = button->m_pLitFrame;
		pMidFrame = button->m_pLitFrameMid;
		pLeftFrame = button->m_pLitFrameLeft;
		pRightFrame = button->m_pLitFrameRight;
	}
	else
	{
		pFrame = button->m_pFrame;
		pMidFrame = button->m_pFrameMid;
		pLeftFrame = button->m_pFrameLeft;
		pRightFrame = button->m_pFrameRight;
	}

	if ((component->m_eState == UISTATE_INACTIVE || component->m_eState == UISTATE_INACTIVATING)
		&& button->m_pInactiveFrame != NULL)
	{
		if (UIButtonGetDown(button) && button->m_pInactiveDownFrame)
		{
			pFrame = button->m_pInactiveDownFrame;
		}
		else
		{
			pFrame = button->m_pInactiveFrame;
		}
		pMidFrame = button->m_pInactiveFrameMid;
		pLeftFrame = button->m_pInactiveFrameLeft;
		pRightFrame = button->m_pInactiveFrameRight;	
	}
	
	UIComponentRemoveAllElements(button);

	if (button->m_pScrollControl)
	{
		if (button->m_fScrollIncrement < 0 &&
			button->m_pScrollControl->m_ScrollPos.m_fY <= 0.0f &&
			button->m_bHideOnScrollExtent)
		{
			// the control is at the bottom
			return UIMSG_RET_NOT_HANDLED;	// don't show this button
		}
		if (button->m_fScrollIncrement > 0 &&
			button->m_pScrollControl->m_ScrollPos.m_fY >= button->m_pScrollControl->m_fScrollVertMax &&
			button->m_bHideOnScrollExtent)
		{
			// the control is at the bottom
			return UIMSG_RET_NOT_HANDLED;	// don't show this button
		}
	}

	if (pMidFrame)
	{
		UIComponentDrawFlexFrames(button, pLeftFrame, pMidFrame, pRightFrame, TRUE);
		if (bHover && button->m_bOverlayLitFrame && button->m_pLitFrameMid)
		{
			UIComponentDrawFlexFrames(button, button->m_pLitFrameLeft, button->m_pLitFrameMid, button->m_pLitFrameRight, TRUE);
		}
	}

	else if (pFrame)
	{
		UIComponentAddElement(button, UIComponentGetTexture(button), pFrame, UI_POSITION(0.0f, 0.0f), button->m_dwColor );
		if (bHover && button->m_bOverlayLitFrame &&button->m_pLitFrame)
		{
			UIComponentAddElement(button, UIComponentGetTexture(button), button->m_pLitFrame, UI_POSITION(0.0f, 0.0f), button->m_dwColor);
		}
	}

  
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX *button = UICastToButton( component );
	if (button->m_pInactiveFrame)
	{
		UISetNeedToRender(component);	
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);  		
	}
	return UIMSG_RET_HANDLED;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX *button = UICastToButton( component );
	button->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;
	if (button->m_pInactiveFrame)
	{
		UISetNeedToRender(component);	
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);  		
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnPostInvisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX *button = UICastToButton( component );
	button->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX* button = UICastToButton(component);
	ASSERT_RETVAL(button, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component))
	{
		if (!(button->m_dwPushstate & UI_BUTTONSTATE_HOVER))
		{
			button->m_dwPushstate |= UI_BUTTONSTATE_HOVER;
			UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
			if( AppIsHellgate() )
				return UIMSG_RET_HANDLED_END_PROCESS;
		}
		if( AppIsTugboat() )
			return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{


	UI_BUTTONEX* button = UICastToButton(component);
	ASSERT_RETVAL(button, UIMSG_RET_NOT_HANDLED);

	//if (UIComponentCheckBounds(component))
	//	button->m_dwPushstate |= UI_BUTTONSTATE_HOVER;
	//else
		button->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;

	if (button->m_eButtonStyle == UIBUTTONSTYLE_PUSHBUTTON)
	{
		button->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
	}

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentOnMouseLeave(component, msg, wParam, lParam);

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);    
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnMouseLeavePassThrough(
								   UI_COMPONENT* component,
								   int msg,
								   DWORD wParam,
								   DWORD lParam)
{


	UI_BUTTONEX* button = UICastToButton(component);
	ASSERT_RETVAL(button, UIMSG_RET_NOT_HANDLED);

	//if (UIComponentCheckBounds(component))
	//	button->m_dwPushstate |= UI_BUTTONSTATE_HOVER;
	//else
	button->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;

	if (button->m_eButtonStyle == UIBUTTONSTYLE_PUSHBUTTON)
	{
		button->m_dwPushstate &= ~UI_BUTTONSTATE_DOWN;
	}

	if (!UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UIComponentOnMouseLeave(component, msg, wParam, lParam);

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);    
	return UIMSG_RET_HANDLED;
}

static UI_COMPONENT * s_pComponentResize = NULL;
static UI_POSITION m_ResizePosition;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIComponentIsResizing()
{
	return s_pComponentResize != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIResizeComponentStart(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	s_pComponentResize = component;
	UIGetCursorPosition(&m_ResizePosition.m_fX, &m_ResizePosition.m_fY);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIResizeComponentEnd(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (s_pComponentResize)
	{
		UI_PANELEX *pPanel = UICastToPanel(s_pComponentResize);
		UI_COMPONENT *pResizeComp = pPanel->m_pResizeComponent;
		if (pResizeComp)
		{
			UIResizeAll(pResizeComp, TRUE);
		}
		s_pComponentResize = NULL;
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIResizeAll(
	UI_COMPONENT* component,
	BOOL bWordWrap /*= FALSE*/)
{
	ASSERT_RETURN(component);

//	UIComponentResize(component, NULL, 0, 0);
	UIComponentHandleUIMessage(component, UIMSG_RESIZE, 0, 0);

	if (component->m_eComponentType == UITYPE_TEXTBOX)
	{
		if (bWordWrap)
		{
			UITextBoxOnResize(component, NULL, 0, 0);
		}
		else
		{
			UI_TEXTBOX* pTextBox = UICastToTextBox(component);
			if (pTextBox->m_pScrollbar)
			{
				UIComponentSetVisible((UI_COMPONENT*)pTextBox->m_pScrollbar, FALSE);
			}
		}
	}

	UI_COMPONENT* pChild = component->m_pFirstChild;
	while (pChild)
	{
		UIResizeAll(pChild, bWordWrap);
		pChild = pChild->m_pNextSibling;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIResizeComponentByDelta(
	UI_COMPONENT* pResizeComp,
	UI_POSITION& posDelta)
{
	ASSERT_RETURN(pResizeComp);

	if (!Math_IsNaN(pResizeComp->m_SizeMin.m_fX) && pResizeComp->m_fWidth + posDelta.m_fX < pResizeComp->m_SizeMin.m_fX)
	{
		posDelta.m_fX = (pResizeComp->m_SizeMin.m_fX - pResizeComp->m_fWidth);
	}
	if (!Math_IsNaN(pResizeComp->m_SizeMax.m_fX) && pResizeComp->m_fWidth + posDelta.m_fX > pResizeComp->m_SizeMax.m_fX)
	{
		posDelta.m_fX = (pResizeComp->m_SizeMax.m_fX - pResizeComp->m_fWidth);
	}
	if (!Math_IsNaN(pResizeComp->m_SizeMin.m_fY) && pResizeComp->m_fHeight + posDelta.m_fY < pResizeComp->m_SizeMin.m_fY)
	{
		posDelta.m_fY = (pResizeComp->m_SizeMin.m_fY - pResizeComp->m_fHeight);
	}
	if (!Math_IsNaN(pResizeComp->m_SizeMax.m_fY) && pResizeComp->m_fHeight + posDelta.m_fY > pResizeComp->m_SizeMax.m_fY)
	{
		posDelta.m_fY = (pResizeComp->m_SizeMax.m_fY - pResizeComp->m_fHeight);
	}

	if (pResizeComp->m_nResizeAnchor == UIALIGN_TOPRIGHT ||
		pResizeComp->m_nResizeAnchor == UIALIGN_RIGHT ||
		pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOMRIGHT)
	{
		pResizeComp->m_Position.m_fX -= posDelta.m_fX;
		pResizeComp->m_ActivePos.m_fX -= posDelta.m_fX;
		pResizeComp->m_InactivePos.m_fX -= posDelta.m_fX;
	}

	if (pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOMLEFT ||
		pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOM ||
		pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOMRIGHT)
	{
		pResizeComp->m_Position.m_fY -= posDelta.m_fY;
		pResizeComp->m_ActivePos.m_fY -= posDelta.m_fY;
		pResizeComp->m_InactivePos.m_fY -= posDelta.m_fY;
	}

	pResizeComp->m_fWidth += posDelta.m_fX;
	pResizeComp->m_fHeight += posDelta.m_fY;

	//LPARAM lParam = MAKE_PARAM((int)posDelta.m_fX, (int)posDelta.m_fY);
	UIResizeAll(pResizeComp);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIResizeComponent(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!s_pComponentResize)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UIComponentGetActive(component))
	{
		UIResizeComponentEnd(component, msg, wParam, lParam);
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_POSITION posNew;
	UIGetCursorPosition(&posNew.m_fX, &posNew.m_fY);

	UI_POSITION posDelta;
	posDelta.m_fX = posNew.m_fX -= m_ResizePosition.m_fX;
	posDelta.m_fY = posNew.m_fY -= m_ResizePosition.m_fY;

	UIGetCursorPosition(&m_ResizePosition.m_fX, &m_ResizePosition.m_fY);

	UI_PANELEX *pPanel = UICastToPanel(component);

	UI_COMPONENT *pResizeComp = pPanel->m_pResizeComponent;
	if (pResizeComp)
	{
		if (pResizeComp->m_nResizeAnchor == UIALIGN_TOPRIGHT ||
			pResizeComp->m_nResizeAnchor == UIALIGN_RIGHT ||
			pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOMRIGHT)
		{
			posDelta.m_fX = -posDelta.m_fX;
		}

		if (pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOMLEFT ||
			pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOM ||
			pResizeComp->m_nResizeAnchor == UIALIGN_BOTTOMRIGHT)
		{
			posDelta.m_fY = -posDelta.m_fY;	
		}

		UIResizeComponentByDelta(pResizeComp, posDelta);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIComponentResize(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	const UI_POSITION oldPosition = component->m_Position;

	UI_COMPONENT *pParent = component->m_pParent;
	if (pParent)
	{
		if (component->m_nAnchorParent != UIALIGN_CENTER)
		{
			switch (component->m_nAnchorParent)
			{
				case UIALIGN_TOPLEFT:
					component->m_Position.m_fX = 0.0f;
					component->m_Position.m_fY = 0.0f;
					break;
				case UIALIGN_TOP:
					component->m_Position.m_fX = 0.0f;
					component->m_Position.m_fY = 0.0f;
					component->m_fWidth = pParent->m_fWidth;
					break;
				case UIALIGN_TOPRIGHT:
					component->m_Position.m_fX = pParent->m_fWidth - component->m_fWidth;
					component->m_Position.m_fY = 0.0f;
					break;
				case UIALIGN_RIGHT:
					component->m_Position.m_fX = pParent->m_fWidth - component->m_fWidth;
					component->m_Position.m_fY = 0.0f;
					component->m_fHeight = pParent->m_fHeight;
					break;
				case UIALIGN_LEFT:
					component->m_Position.m_fX = 0.0f;
					component->m_Position.m_fY = 0.0f;
					component->m_fHeight = pParent->m_fHeight;
					break;
				case UIALIGN_BOTTOMLEFT:
					component->m_Position.m_fX = 0.0f;
					component->m_Position.m_fY = pParent->m_fHeight - component->m_fHeight;
					break;
				case UIALIGN_BOTTOM:
					component->m_Position.m_fX = 0.0f;
					component->m_Position.m_fY = pParent->m_fHeight - component->m_fHeight;
					component->m_fWidth = pParent->m_fWidth;
					break;
				case UIALIGN_BOTTOMRIGHT:
					component->m_Position.m_fX = pParent->m_fWidth - component->m_fWidth;
					component->m_Position.m_fY = pParent->m_fHeight - component->m_fHeight;
					break;
			};

			component->m_Position.m_fX += component->m_fAnchorXoffs;
			component->m_Position.m_fY += component->m_fAnchorYoffs;
		}

		if (!Math_IsNaN(component->m_fWidthParentRel))
		{
			component->m_fWidth = pParent->m_fWidth + component->m_fWidthParentRel;
		}

		if (!Math_IsNaN(component->m_fHeightParentRel))
		{
			component->m_fHeight = pParent->m_fHeight + component->m_fHeightParentRel;
		}
	}

	const UI_POSITION posDelta = component->m_Position - oldPosition;
	component->m_ActivePos += posDelta;
	component->m_InactivePos += posDelta;

	return UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnMouseOverItem(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX* button = UICastToButton(component);
	ASSERT_RETVAL(button, UIMSG_RET_NOT_HANDLED);

	if (!UICursorIsEmpty())
	{
		return UIMSG_RET_END_PROCESS;
	}

	button->m_dwPushstate |= UI_BUTTONSTATE_HOVER;
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);    
	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonOnMouseLeaveItem(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_BUTTONEX* button = UICastToButton(component);
	ASSERT_RETVAL(button, UIMSG_RET_NOT_HANDLED);

	button->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);    
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIButtonBlinkHighlightUpdate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_BUTTONEX *pButton = (UI_BUTTONEX*)data.m_Data1;
	if (!pButton)
	{
		return;
	}

	if (!pButton->m_pBlinkData)
	{
		return;
	}

	if (pButton->m_pBlinkData->m_nBlinkDuration > 0)
	{
		if (AppCommonGetCurTime() - pButton->m_pBlinkData->m_timeBlinkStarted >= pButton->m_pBlinkData->m_nBlinkDuration)
		{
			pButton->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;
			UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);    
			return;
		}
	}

	// Schedule the next update
	if (pButton->m_pBlinkData->m_dwBlinkAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(pButton->m_pBlinkData->m_dwBlinkAnimTicket);
	}
	pButton->m_pBlinkData->m_dwBlinkAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + pButton->m_pBlinkData->m_nBlinkPeriod, UIButtonBlinkHighlightUpdate, CEVENT_DATA((DWORD_PTR) pButton));

	pButton->m_pBlinkData->m_timeLastBlinkChange = AppCommonGetCurTime();
	if (pButton->m_dwPushstate & UI_BUTTONSTATE_HOVER)
	{
		pButton->m_dwPushstate &= ~UI_BUTTONSTATE_HOVER;
	}
	else 
	{
		pButton->m_dwPushstate |= UI_BUTTONSTATE_HOVER;
	}
	UIComponentHandleUIMessage(pButton, UIMSG_PAINT, 0, 0);    
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIButtonBlinkHighlight(
	UI_BUTTONEX *pButton, 
	int *pnDuration /*= NULL*/, 
	int *pnPeriod /*= NULL*/)
{
	ASSERT_RETURN(pButton);
	if (pButton)
	{
		if (!pButton->m_pBlinkData)
		{
			pButton->m_pBlinkData = (UI_BLINKDATA *)MALLOC(NULL, sizeof(UI_BLINKDATA));
			pButton->m_pBlinkData->m_nBlinkDuration = -1;
			pButton->m_pBlinkData->m_nBlinkPeriod = 500;
			pButton->m_pBlinkData->m_dwBlinkAnimTicket = INVALID_ID;
		}
		
		pButton->m_pBlinkData->m_timeBlinkStarted = AppCommonGetCurTime();
		pButton->m_pBlinkData->m_timeLastBlinkChange = AppCommonGetCurTime();
		if (pnDuration)
			pButton->m_pBlinkData->m_nBlinkDuration = *pnDuration;
		if (pnPeriod)
			pButton->m_pBlinkData->m_nBlinkPeriod = *pnPeriod;
		CSchedulerRegisterEventImm(UIButtonBlinkHighlightUpdate, CEVENT_DATA((DWORD_PTR) pButton));
	}
}

static int sUIComponentGetNumElements(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETFALSE(pComponent);

	int nCount = 0;
	UI_GFXELEMENT* pElement = pComponent->m_pGfxElementFirst;
	while (pElement)
	{
		nCount++;
		pElement = pElement->m_pNextElement;
	}

	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFlexBorderOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);
	UI_FLEXBORDER *pFlexBorder = UICastToFlexBorder(component);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(pFlexBorder);
	ASSERT_RETVAL(pTexture, UIMSG_RET_NOT_HANDLED);
	//if( AppIsHellgate() )
	{
		if (pFlexBorder->m_bAutosize && pFlexBorder->m_pParent)
		{
			pFlexBorder->m_fWidth = pFlexBorder->m_pParent->m_fWidth;
			pFlexBorder->m_fHeight = pFlexBorder->m_pParent->m_fHeight;
		}
		float fWidth  = pFlexBorder->m_fWidth + (pFlexBorder->m_fBorderSpace * 2.0f);
		float fHeight = pFlexBorder->m_fHeight + (pFlexBorder->m_fBorderSpace * 2.0f);

		ASSERT_RETVAL(fWidth <= UIDefaultWidth() * 2.0f, UIMSG_RET_NOT_HANDLED);
		ASSERT_RETVAL(fHeight <= UIDefaultHeight() * 2.0f, UIMSG_RET_NOT_HANDLED);

		float fLeftX = 0.0f - pFlexBorder->m_fBorderSpace;
		float fTopY = 0.0f - pFlexBorder->m_fBorderSpace;
		float fRightWidth = pFlexBorder->m_pFrameBR ? pFlexBorder->m_pFrameBR->m_fWidth : (pFlexBorder->m_pFrameMR ? pFlexBorder->m_pFrameMR->m_fWidth : 0.0f);
		float fRightX = fLeftX + (fWidth - fRightWidth);
		float fBottomHeight = pFlexBorder->m_pFrameBR ? pFlexBorder->m_pFrameBR->m_fHeight : (pFlexBorder->m_pFrameBM ? pFlexBorder->m_pFrameBM->m_fHeight : 0.0f);
		float fBottomY = fTopY + (fHeight - fBottomHeight);
	
		if (fRightX < 0.0f || fBottomY < 0.0f)
		{
			return UIMSG_RET_NOT_HANDLED;
		}
	
	
	
		// Corners -------------------------------------------------
		if (pFlexBorder->m_pFrameTL) UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameTL, UI_POSITION(fLeftX,  fTopY), pFlexBorder->m_dwColor	);
		if (pFlexBorder->m_pFrameTR) UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameTR, UI_POSITION(fRightX, fTopY), pFlexBorder->m_dwColor	);
		if (pFlexBorder->m_pFrameBL) UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameBL, UI_POSITION(fLeftX,  fBottomY), pFlexBorder->m_dwColor);
		if (pFlexBorder->m_pFrameBR) UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameBR, UI_POSITION(fRightX, fBottomY), pFlexBorder->m_dwColor);
	
		// -----------------------------------------------------------
	
		// Make a clip rect for the interior sections so they don't overlap the corners
		UI_RECT cliprect;
	
		float fX = 0.0f;
		float fY = 0.0f;
		float i = 0.0f;
	
		// Top line --------------------------------------------------
		if (pFlexBorder->m_pFrameTM)
		{
			float fTLWidth = (pFlexBorder->m_pFrameTL ? pFlexBorder->m_pFrameTL->m_fWidth : 0.0f);
			cliprect.m_fX1 = fLeftX + fTLWidth;
			cliprect.m_fY1 = fTopY;
			cliprect.m_fX2 = fRightX;
			cliprect.m_fY2 = fTopY + fHeight;
			ASSERT_RETVAL(pFlexBorder->m_pFrameTM->m_fWidth >= 1.0f, UIMSG_RET_NOT_HANDLED);
			if( pFlexBorder->m_bStretchBorders )
			{

				float fScaleX = ( cliprect.m_fX2 - cliprect.m_fX1 ) / pFlexBorder->m_pFrameTM->m_fWidth;
				UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameTM, UI_POSITION( fLeftX + fTLWidth,  fTopY), pFlexBorder->m_dwColor, &cliprect, FALSE, fScaleX, 1.0f	);
			}
			else
			{
				for (fX = fLeftX + fTLWidth, i = 0.0f; fX < fRightX; i += 1.0f, fX = fLeftX + fTLWidth + (i * pFlexBorder->m_pFrameTM->m_fWidth) )
				{
					UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameTM, UI_POSITION(fX, fTopY), pFlexBorder->m_dwColor, &cliprect );
				}
			}
		}
		// -----------------------------------------------------------
	
		// Bottom line -----------------------------------------------
		if (pFlexBorder->m_pFrameBM)
		{
			float fBLWidth = (pFlexBorder->m_pFrameBL ? pFlexBorder->m_pFrameBL->m_fWidth : 0.0f);
			cliprect.m_fX1 = fLeftX + fBLWidth;
			cliprect.m_fY1 = fBottomY;
			cliprect.m_fX2 = fRightX;
			cliprect.m_fY2 = fTopY + fHeight;
			ASSERT_RETVAL(pFlexBorder->m_pFrameBM->m_fWidth >= 1.0f, UIMSG_RET_NOT_HANDLED);
			if( pFlexBorder->m_bStretchBorders )
			{

				float fScaleX = ( cliprect.m_fX2 - cliprect.m_fX1 ) / pFlexBorder->m_pFrameBM->m_fWidth;
				UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameBM, UI_POSITION( fLeftX + fBLWidth,  fBottomY), pFlexBorder->m_dwColor, &cliprect, FALSE, fScaleX, 1.0f	);
			}
			else
			{
				for (fX = fLeftX + fBLWidth, i = 0.0f; fX < fRightX; i += 1.0f, fX = fLeftX + fBLWidth + (i * pFlexBorder->m_pFrameBM->m_fWidth) )
				{
					UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameBM, UI_POSITION(fX, fBottomY), pFlexBorder->m_dwColor, &cliprect );
				}
			}
		}
		// -----------------------------------------------------------
	
		// Left line -------------------------------------------------
		if (pFlexBorder->m_pFrameML)
		{
			float fTLHeight = (pFlexBorder->m_pFrameTL ? pFlexBorder->m_pFrameTL->m_fHeight : 0.0f);
			cliprect.m_fX1 = fLeftX;
			cliprect.m_fY1 = fTopY + fTLHeight;
			cliprect.m_fX2 = fLeftX + pFlexBorder->m_pFrameML->m_fWidth;
			cliprect.m_fY2 = fBottomY;
			ASSERT_RETVAL(pFlexBorder->m_pFrameML->m_fHeight >= 1.0f, UIMSG_RET_NOT_HANDLED);
			if( pFlexBorder->m_bStretchBorders )
			{

				float fScaleY = ( cliprect.m_fY2 - cliprect.m_fY1 ) / pFlexBorder->m_pFrameML->m_fHeight;
				UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameML, UI_POSITION( fLeftX,  fTopY + fTLHeight), pFlexBorder->m_dwColor, &cliprect, FALSE, 1.0f, fScaleY );
			}
			else
			{
				for (fY = fTopY + fTLHeight, i = 0.0f; fY < fBottomY; i += 1.0f, fY = fTopY + fTLHeight + (i * pFlexBorder->m_pFrameML->m_fHeight) )
				{
					UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameML, UI_POSITION(fLeftX, fY), pFlexBorder->m_dwColor, &cliprect );
				}
			}
		}
		// -----------------------------------------------------------
	
	
		// Right line ------------------------------------------------
		if (pFlexBorder->m_pFrameMR)
		{
			float fTRHeight = pFlexBorder->m_pFrameTR ? pFlexBorder->m_pFrameTR->m_fHeight : 0.0f;
			cliprect.m_fX1 = fRightX;
			cliprect.m_fY1 = fTopY + fTRHeight;
			cliprect.m_fX2 = fRightX + pFlexBorder->m_pFrameMR->m_fWidth;
			cliprect.m_fY2 = fBottomY;
			ASSERT_RETVAL(pFlexBorder->m_pFrameMR->m_fHeight >= 1.0f, UIMSG_RET_NOT_HANDLED);
			if( pFlexBorder->m_bStretchBorders )
			{

				float fScaleY = ( cliprect.m_fY2 - cliprect.m_fY1 ) / pFlexBorder->m_pFrameMR->m_fHeight;
				UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameMR, UI_POSITION( fRightX,  fTopY + fTRHeight), pFlexBorder->m_dwColor, &cliprect, FALSE, 1.0f, fScaleY );
			}
			else
			{
				for (fY = fTopY + fTRHeight, i = 0.0f; fY < fBottomY; i += 1.0f, fY = fTopY + fTRHeight + (i * pFlexBorder->m_pFrameMR->m_fHeight) )
				{
					UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameMR, UI_POSITION(fRightX, fY), pFlexBorder->m_dwColor, &cliprect );
				}
			}
		}
		// -----------------------------------------------------------
	
		// middle fill -----------------------------------------------
		if (pFlexBorder->m_pFrameMM)
		{
			float fMLWidth = pFlexBorder->m_pFrameML ? pFlexBorder->m_pFrameML->m_fWidth : 0.0f;
			float fTMHeight = pFlexBorder->m_pFrameTM ? pFlexBorder->m_pFrameTM->m_fHeight : 0.0f;
			cliprect.m_fX1 = fLeftX + fMLWidth;
			cliprect.m_fY1 = fTopY + fTMHeight;
			cliprect.m_fX2 = fRightX;
			cliprect.m_fY2 = fBottomY;
			ASSERT_RETVAL(pFlexBorder->m_pFrameMM->m_fWidth >= 1.0f, UIMSG_RET_NOT_HANDLED);
			ASSERT_RETVAL(pFlexBorder->m_pFrameMM->m_fHeight >= 1.0f, UIMSG_RET_NOT_HANDLED);
	
			float j = 0.0f;
			if( pFlexBorder->m_bStretchBorders )
			{

				float fScaleY = ( cliprect.m_fY2 - cliprect.m_fY1 ) / pFlexBorder->m_pFrameMM->m_fHeight;
				float fScaleX = ( cliprect.m_fX2 - cliprect.m_fX1 ) / pFlexBorder->m_pFrameMM->m_fWidth;
				UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameMM, UI_POSITION( fLeftX + fMLWidth,  fTopY + fTMHeight), pFlexBorder->m_dwColor, &cliprect, FALSE, fScaleX, fScaleY );
			}
			else
			{
				for (fX = fLeftX + fMLWidth, i = 0.0f; fX < fRightX; i += 1.0f, fX = fLeftX + fMLWidth + (i * pFlexBorder->m_pFrameMM->m_fWidth) )
				{
					for (fY = fTopY + fTMHeight, j = 0.0f; fY < fBottomY; j += 1.0f, fY = fTopY + fTMHeight + (j * pFlexBorder->m_pFrameMM->m_fHeight) )
					{
						UIComponentAddElement(pFlexBorder, pTexture, pFlexBorder->m_pFrameMM, UI_POSITION(fX, fY), pFlexBorder->m_dwColor, &cliprect );
					}
				}
			}
		}
	}

	int nNumElements = sUIComponentGetNumElements(pFlexBorder);
	if (nNumElements > 10000)
	{
		DebugBreak();
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIFlexLineSetSize(
	UI_COMPONENT* component,
	BOOL bSetBaseSize = FALSE)
{
	ASSERT_RETURN(component);

	UI_FLEXLINE *pFlexLine = UICastToFlexLine(component);
	if (!pFlexLine->m_bAutosize)
		return;

	ASSERT_RETURN(pFlexLine->m_pFrameLine);

	//if (component->m_codeVisibleWhen && AppGetCltGame())
	//{
	//	UNIT *pUnit = UIComponentGetFocusUnit(component);
	//	int nResult = VMExecI(AppGetCltGame(), pUnit, g_UI.m_pCodeBuffer + component->m_codeVisibleWhen, g_UI.m_nCodeCurr - component->m_codeVisibleWhen);		
	//	if (nResult == 0) 
	//	{
	//		pFlexLine->m_fWidth = 0;
	//		pFlexLine->m_fHeight = 0;
	//		return;
	//	}
	//}

	if (pFlexLine->m_nVisibleOnSibling != FLEX_SIB_NONE)
	{	
		UI_COMPONENT *pSibling = NULL;
		// TODO: implement nextany and prevany

		if (pFlexLine->m_nVisibleOnSibling == FLEX_SIB_NEXT ||
			pFlexLine->m_nVisibleOnSibling == FLEX_SIB_NEXT_ANY)
			pSibling = pFlexLine->m_pNextSibling;
		if (pFlexLine->m_nVisibleOnSibling == FLEX_SIB_PREV ||
			pFlexLine->m_nVisibleOnSibling == FLEX_SIB_PREV_ANY)
			pSibling = pFlexLine->m_pPrevSibling;

		BOOL bFoundOne = FALSE;
		while (pSibling && !bFoundOne && !UIComponentIsFlexLine(pSibling))
		{
			if (pSibling->m_fWidth != 0.0f && pSibling->m_fHeight != 0.0f)
			{
				bFoundOne = TRUE;
			}
			if (pFlexLine->m_nVisibleOnSibling == FLEX_SIB_NEXT ||
				pFlexLine->m_nVisibleOnSibling == FLEX_SIB_PREV)
			{
				pSibling = NULL;
			}
			if (pFlexLine->m_nVisibleOnSibling == FLEX_SIB_NEXT_ANY)
				pSibling = pSibling->m_pNextSibling;
			if (pFlexLine->m_nVisibleOnSibling == FLEX_SIB_PREV_ANY)
				pSibling = pSibling->m_pPrevSibling;
		}

		if (!bFoundOne)
		{
			pFlexLine->m_fWidth = 0;
			pFlexLine->m_fHeight = 0;
			return;
		}
	}

	if (bSetBaseSize)
	{
		pFlexLine->m_fWidth = pFlexLine->m_pFrameLine->m_fWidth;
		pFlexLine->m_fHeight = pFlexLine->m_pFrameLine->m_fHeight;
		return;
	}

	if (pFlexLine->m_bHorizontal)
	{
		pFlexLine->m_fHeight = pFlexLine->m_pFrameLine->m_fHeight;
		pFlexLine->m_fWidth  = pFlexLine->m_pParent->m_fWidth;
	}
	else
	{
		pFlexLine->m_fHeight = pFlexLine->m_pParent->m_fHeight;
		pFlexLine->m_fWidth = pFlexLine->m_pFrameLine->m_fWidth;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentDrawFlexFrames(
	UI_COMPONENT* pComponent,
	UI_TEXTURE_FRAME *pStartFrame,
	UI_TEXTURE_FRAME *pMidFrame,
	UI_TEXTURE_FRAME *pEndFrame,
	BOOL bHorizontal /*= TRUE*/,
	UI_SIZE *pSize /*= NULL*/)
{
	ASSERT_RETURN(pComponent);
	ASSERT_RETURN(pMidFrame);

	UI_SIZE sizeComponent(pComponent->m_fWidth, pComponent->m_fHeight);

	if (!pSize)
		pSize = &sizeComponent;

	float fMidFrameWidth  = pMidFrame->m_fWidth;
	float fMidFrameHeight = pMidFrame->m_fHeight;
	float fStartFrameWidth  = 0.0f;
	float fStartFrameHeight = 0.0f;
	float fEndFrameWidth  = 0.0f;
	float fEndFrameHeight = 0.0f;
	if (pStartFrame)
	{
		fStartFrameWidth  = pStartFrame->m_fWidth;
		fStartFrameHeight = pStartFrame->m_fHeight;
	}
	if (pEndFrame)
	{
		fEndFrameWidth  = pEndFrame->m_fWidth;
		fEndFrameHeight = pEndFrame->m_fHeight;
	}

	UI_POSITION posCurrent;
	posCurrent.m_fX = 0.0f;
	posCurrent.m_fY = 0.0f;

	UI_RECT cliprect(0.0f, 0.0f, pSize->m_fWidth, pSize->m_fHeight);

	float *curpos = bHorizontal ? &posCurrent.m_fX : &posCurrent.m_fY;
	float maxpos = bHorizontal ? pSize->m_fWidth : pSize->m_fHeight;

	float caplen = 0.0f;
	if (pStartFrame != NULL)
	{
		UIComponentAddElement(pComponent, UIComponentGetTexture(pComponent), pStartFrame, posCurrent, GFXCOLOR_HOTPINK, &cliprect );
		caplen = (bHorizontal ? fStartFrameWidth : fStartFrameHeight);
	}

	if (pEndFrame != NULL)
	{
		*curpos = MAX(0.0f, maxpos - (bHorizontal ? fEndFrameWidth : fEndFrameHeight));
		UIComponentAddElement(pComponent, UIComponentGetTexture(pComponent), pEndFrame, posCurrent, GFXCOLOR_HOTPINK, &cliprect );
		// make sure the line stops before it overruns the endcap
		if (bHorizontal)
		{
			cliprect.m_fX2 = *curpos;
		}
		else
		{
			cliprect.m_fY2 = *curpos;
		}
	}

	ASSERT_RETURN(fMidFrameWidth > 0.1f && fMidFrameHeight > 0.1f);

	float i = 0.0f;
	*curpos = caplen;
	while (*curpos < maxpos)
	{
		UIComponentAddElement(pComponent, UIComponentGetTexture(pComponent), pMidFrame, posCurrent, GFXCOLOR_HOTPINK, &cliprect );

		i += 1.0f;
		*curpos = caplen + (i * (bHorizontal ? fMidFrameWidth : fMidFrameHeight));
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIFlexLineOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);
	UI_FLEXLINE *pFlexLine = UICastToFlexLine(component);

	ASSERT_RETVAL(pFlexLine->m_pFrameLine, UIMSG_RET_NOT_HANDLED);

	// Tooltips set their size, then expand.  They handle the size of flexlines on their own, so don't reset the size
	//   in case the line is being repainted later.
	if (!UIComponentIsTooltip(pFlexLine->m_pParent))
	{
		UIFlexLineSetSize(component);
	}

	UIComponentDrawFlexFrames(component, pFlexLine->m_pFrameStartCap, pFlexLine->m_pFrameLine, pFlexLine->m_pFrameEndCap, pFlexLine->m_bHorizontal);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITooltipDetermineContents(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);

	UI_TOOLTIP *pTooltip = UICastToTooltip(pComponent);

	UI_COMPONENT *pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_STATDISPL ||
			pChild->m_eComponentType == UITYPE_LABEL)
		{
			UI_LABELEX *pLabel = UICastToLabel(pChild);
			if ((pLabel->m_bAutosize || pLabel->m_bAutosizeHorizontalOnly) && pLabel->m_bWordWrap)
			{
				// we gotta do these later
				pLabel->m_fWidth = 0.0f;
				if (!pLabel->m_bAutosizeHorizontalOnly)
					pLabel->m_fHeight = 0.0f;
			}
			else if (pChild->m_eComponentType == UITYPE_STATDISPL)
			{
				// reset the sizing
				UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
			}
		}
		else if (pChild->m_eComponentType == UITYPE_FLEXLINE)
		{
			UIFlexLineSetSize(pChild, TRUE);	// Set the base flexline size
		}

		pChild = pChild->m_pNextSibling;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITooltipDetermineSize(
	UI_COMPONENT *pComponent)
{
	ASSERT_RETURN(pComponent);

	float fBorderLeft = 0.0f;
	float fBorderTop = 0.0f;
	float fBorderRight = 0.0f;
	float fBorderBottom = 0.0f;
	static const int MAX_LINES = 20;
	static const int MAX_ITEMS_ON_A_LINE = 20;
	float fLineWidth[MAX_LINES][3];
	float fLineHeight[MAX_LINES];
	UI_COMPONENT *ppTooltipMembers[MAX_LINES][MAX_ITEMS_ON_A_LINE];
	int nNumArrangedChildren = 0;
	memclear(fLineHeight, sizeof(float) * MAX_LINES);
	memclear(fLineWidth, sizeof(float) * MAX_LINES * 3);
	memclear(ppTooltipMembers, sizeof(UI_COMPONENT *) * MAX_LINES * MAX_ITEMS_ON_A_LINE);
	int nNumLines = 0;

	UI_TOOLTIP *pTooltip = UICastToTooltip(pComponent);
	pTooltip->m_fHeight = 0.0f;

	pTooltip->m_fWidth = 0.0f;

	UI_COMPONENT *pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_FLEXLINE)
		{
			//UIFlexLineSetSize(pChild, TRUE);		 gonna do this later on
		}

		if (pChild->m_eComponentType == UITYPE_FLEXBORDER)
		{
			UI_FLEXBORDER *pBorder = UICastToFlexBorder(pChild);
			fBorderTop = MAX(fBorderLeft, pBorder->m_pFrameTM ? pBorder->m_pFrameTM->m_fHeight : 0.0f);
			fBorderLeft = MAX(fBorderLeft, pBorder->m_pFrameML ? pBorder->m_pFrameML->m_fWidth : 0.0f);
			fBorderRight = MAX(fBorderRight, pBorder->m_pFrameMR ? pBorder->m_pFrameMR->m_fWidth : 0.0f);
			fBorderBottom = MAX(fBorderRight, pBorder->m_pFrameBM ? pBorder->m_pFrameBM->m_fHeight : 0.0f);
			fBorderTop *= pBorder->m_fBorderScale;
			fBorderLeft *= pBorder->m_fBorderScale;
			fBorderRight *= pBorder->m_fBorderScale;
			fBorderBottom *= pBorder->m_fBorderScale;
		}

		if (pChild->m_nTooltipLine > 0 &&
			pChild->m_nTooltipLine <= MAX_LINES &&
			pChild->m_nTooltipOrder > 0 &&
			pChild->m_nTooltipOrder <= MAX_ITEMS_ON_A_LINE)
		{
			int nLine = pChild->m_nTooltipLine-1;
			nNumLines = MAX(nNumLines, pChild->m_nTooltipLine);

			if (!pChild->m_nTooltipCrossesLines && (pChild->m_bVisible || !pChild->m_bIndependentActivate))
			{
				fLineHeight[nLine] = MAX(fLineHeight[nLine], (pChild->m_fHeight > 0.0f ? pChild->m_fHeight + pChild->m_fToolTipPaddingY : 0.0f));
			}

			int nSection = 0;
			if (e_UIAlignIsCenterHoriz(pChild->m_nTooltipJustify))
			{
				nSection = 1;	// center
			}
			else if (e_UIAlignIsRight(pChild->m_nTooltipJustify))
			{
				nSection = 2;	// right
			}

			// if the child isn't visible, and it's not going to automatically be set visible later, don't include it in size calculations
			if (pChild->m_bVisible || !pChild->m_bIndependentActivate)
			{
				//for (int i=0; i<= pChild->m_nTooltipCrossesLines; i++)
				//{
				//	fLineWidth[nLine+i][nSection] += pChild->m_fWidth + (pChild->m_fWidth ? pChild->m_fToolTipPaddingX : 0.0f);
				//}

				if (!pChild->m_bTooltipNoSpace && !pChild->m_nTooltipCrossesLines)
					fLineWidth[nLine][nSection] += pChild->m_fWidth + (pChild->m_fWidth ? pChild->m_fToolTipPaddingX : 0.0f);

				ASSERTV(ppTooltipMembers[pChild->m_nTooltipLine-1][pChild->m_nTooltipOrder-1] == NULL, 
					"Tooltip [%s] child [%s] is being overwritten by [%s]", 
					pComponent->m_szName,
					ppTooltipMembers[pChild->m_nTooltipLine-1][pChild->m_nTooltipOrder-1]->m_szName,
					pChild->m_szName);

				ppTooltipMembers[pChild->m_nTooltipLine-1][pChild->m_nTooltipOrder-1] = pChild;
				nNumArrangedChildren++;
			}
		}

		pChild = pChild->m_pNextSibling;
	}

	// if there's a child that crosses lines, make sure there's room for it
	pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_nTooltipCrossesLines > 0)
		{
			int nSection = 0;
			if (e_UIAlignIsCenterHoriz(pChild->m_nTooltipJustify))
			{
				nSection = 1;	// center
			}
			else if (e_UIAlignIsRight(pChild->m_nTooltipJustify))
			{
				nSection = 2;	// right
			}

			int i=pChild->m_nTooltipLine-1;
			int nLinesUsed = 0;
			float fTotalheight = 0.0f;
			while (nLinesUsed <= pChild->m_nTooltipCrossesLines && i < MAX_LINES)
			{
				//if (fLineHeight[i] > 0.0f)
				{
					fTotalheight += fLineHeight[i];
					fLineWidth[i][nSection] += pChild->m_fWidth + (pChild->m_fWidth ? pChild->m_fToolTipPaddingX : 0.0f);
					nLinesUsed++;
				}
				i++;
			}

			if (fTotalheight < pChild->m_fHeight)
			{
				if (i >= MAX_LINES)
					i = MAX_LINES - 1;
				fLineHeight[i-1] += pChild->m_fHeight - fTotalheight;		// make up the difference
			}
		}

		pChild = pChild->m_pNextSibling;
	}

	// Size the whole tooltip to encompass all of the children
	for (int iLine = 0; iLine < nNumLines; iLine++)
	{
		// if there are center elements, the right side and left side need to be the same width
		if (fLineWidth[iLine][1] > 0)
		{
			fLineWidth[iLine][0] = fLineWidth[iLine][2] = MAX(fLineWidth[iLine][0], fLineWidth[iLine][2]);
		}

		pTooltip->m_fWidth = MAX(pTooltip->m_fWidth, fLineWidth[iLine][0] + fLineWidth[iLine][1] + fLineWidth[iLine][2]);
	}
	if (pTooltip->m_fMaxWidth > 0.0f)
	{
		pTooltip->m_fWidth = MIN(pTooltip->m_fWidth, pTooltip->m_fMaxWidth);
	}
	if (pTooltip->m_fMinWidth > 0.0f)
	{
		pTooltip->m_fWidth = MAX(pTooltip->m_fWidth, pTooltip->m_fMinWidth);
	}

	// Now size the autosize/wordwrapped items specially
	pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_STATDISPL ||
			pChild->m_eComponentType == UITYPE_LABEL)
		{
			UI_LABELEX *pLabel = UICastToLabel(pChild);
			if ((pLabel->m_bAutosize || pLabel->m_bAutosizeHorizontalOnly) && pLabel->m_bWordWrap)
			{
				int iLine = pChild->m_nTooltipLine-1;
				if ((nNumArrangedChildren == 1 || pLabel->m_bUseParentMaxWidth) && pTooltip->m_fMaxWidth > 0)
				{
					// if this is the only child, and it's word-wrapped then we're just going to wrap the tooltip around it
					pLabel->m_fMaxWidth = pTooltip->m_fMaxWidth;
				}
				else
				{
					pLabel->m_fMaxWidth = pTooltip->m_fWidth - fLineWidth[iLine][0] + fLineWidth[iLine][1] + fLineWidth[iLine][2];
				}
//				ASSERTN("brennan", pLabel->m_fMaxWidth > 0 || PStrLen(pLabel->m_Line.m_szText) == 0);
//				ASSERTN("brennan", pLabel->m_fMaxWidth <= pTooltip->m_fWidth);
				UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
//				ASSERTN("brennan", pLabel->m_fWidth <= pTooltip->m_fWidth);
				fLineHeight[iLine] = MAX(fLineHeight[iLine], (pChild->m_fHeight > 0.0f ? pChild->m_fHeight + pChild->m_fToolTipPaddingY : 0.0f));
				fLineWidth[iLine][1] += pChild->m_fWidth;

				if (nNumArrangedChildren == 1 || pLabel->m_bUseParentMaxWidth)
				{
					// if this is the only child, and it's word-wrapped then we're just going to wrap the tooltip around it
					pTooltip->m_fWidth = MAX(pLabel->m_fWidth, pTooltip->m_fWidth);
				}
			}
		}
		pChild = pChild->m_pNextSibling;
	}

	// Set the size of any flexlines (we just gotta do this here).
	pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_FLEXLINE)
		{
			UIFlexLineSetSize(pChild);

			// re-check the line height in case one of the lines was made visible or invisible based on the visibility of siblings
			int nLine = pChild->m_nTooltipLine-1;
			fLineHeight[nLine] = MAX(fLineHeight[nLine], (pChild->m_fHeight > 0.0f ? pChild->m_fHeight + pChild->m_fToolTipPaddingY : 0.0f));	
		}
		pChild = pChild->m_pNextSibling;
	}


	for (int iLine = 0; iLine < nNumLines; iLine++)
	{
		pTooltip->m_fHeight += fLineHeight[iLine];
	}

	// Now add on the size for the border
	pTooltip->m_fHeight += (fBorderTop + fBorderBottom);
	pTooltip->m_fWidth += (fBorderLeft + fBorderRight);

	ASSERT_RETURN(pTooltip->m_fWidth <= UIDefaultWidth() * 2);
	ASSERT_RETURN(pTooltip->m_fHeight <= UIDefaultHeight() * 2);

	if( pTooltip->m_bCentered )
	{
		pTooltip->m_Position.m_fX = pTooltip->m_ActivePos.m_fX - pTooltip->m_fWidth * .5f;
		pTooltip->m_Position.m_fY = pTooltip->m_ActivePos.m_fY - pTooltip->m_fHeight * .5f;
	}

	// Resize the border now that we have a final size
	pChild = pTooltip->m_pFirstChild;
	while (pChild)
	{
		if (pChild->m_eComponentType == UITYPE_FLEXBORDER ||
			pChild->m_eComponentType == UITYPE_FLEXLINE)
		{
			UIComponentHandleUIMessage(pChild, UIMSG_PAINT, 0, 0);
		}
		pChild = pChild->m_pNextSibling;
	}

	// Now we gotta go position all the children the way they want it.
	float fCurrentY = fBorderTop;
	for (int iLine = 0; iLine < nNumLines; iLine++)
	{
		float fCurrentX = fBorderLeft;
		float fThisLineHeight = fLineHeight[iLine];
		for (int iOrder = 0; iOrder < MAX_ITEMS_ON_A_LINE; iOrder++)
		{
			pChild = ppTooltipMembers[iLine][iOrder];
			if (!pChild)
			{
				continue;
			}
			float fChildHeight = pChild->m_fHeight;
			float fChildWidth = pChild->m_fWidth + pChild->m_fToolTipPaddingX;

			// Top justify
			if (e_UIAlignIsTop(pChild->m_nTooltipJustify))
			{
				pChild->m_Position.m_fY = fCurrentY;
			}

			// Bottom justify
			if (e_UIAlignIsBottom(pChild->m_nTooltipJustify))
			{
				pChild->m_Position.m_fY = fCurrentY + (fThisLineHeight - fChildHeight);
			}

			// Center justify (vertically)
			if (e_UIAlignIsCenterVert(pChild->m_nTooltipJustify))
			{
				pChild->m_Position.m_fY = fCurrentY + ((fThisLineHeight - fChildHeight) / 2.0f);
			}

			// Left justify
			if (e_UIAlignIsLeft(pChild->m_nTooltipJustify))
			{
				pChild->m_Position.m_fX = fCurrentX;
				if (!pChild->m_bTooltipNoSpace)
					fCurrentX += fChildWidth;
			}
		}

		// Center justify (horizontally)
		fCurrentX = (pTooltip->m_fWidth / 2.0f) - (fLineWidth[iLine][1] / 2);  
		for (int iOrder = 0; iOrder < MAX_ITEMS_ON_A_LINE; iOrder++)
		{
			pChild = ppTooltipMembers[iLine][iOrder];
			if (!pChild)
			{
				continue;
			}
			if (e_UIAlignIsCenterHoriz(pChild->m_nTooltipJustify))
			{
				pChild->m_Position.m_fX = fCurrentX;
				if (!pChild->m_bTooltipNoSpace)
					fCurrentX += pChild->m_fWidth + pChild->m_fToolTipPaddingX;
			}
		}

		fCurrentX = pTooltip->m_fWidth - fBorderRight;
		for (int iOrder = MAX_ITEMS_ON_A_LINE-1; iOrder >= 0; iOrder--)
		{
			pChild = ppTooltipMembers[iLine][iOrder];
			if (!pChild)
			{
				continue;
			}
			float fChildWidth = pChild->m_fWidth + pChild->m_fToolTipPaddingX;

			// Right justify
			if (e_UIAlignIsRight(pChild->m_nTooltipJustify))
			{
				if (!pChild->m_bTooltipNoSpace)
					fCurrentX -= fChildWidth;
				pChild->m_Position.m_fX = fCurrentX;
			}
			pChild->m_Position.m_fX += pChild->m_fToolTipXOffset;
			pChild->m_Position.m_fY += pChild->m_fToolTipYOffset;
		}


		fCurrentY += fThisLineHeight;
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UITooltipPosition(
	UI_TOOLTIP *pTooltip,
	UI_RECT *pRect /*= NULL*/,
	int nSuggestRelativePos /*= TTPOS_RIGHT_TOP*/,
	UI_RECT *pAvoidRect /*= NULL*/)
{
	float fAppWidth = AppCommonGetWindowWidth() * UIGetScreenToLogRatioX();
	float fAppHeight = AppCommonGetWindowHeight() * UIGetScreenToLogRatioY();

	float fTooltipWidth  = pTooltip->m_fWidth  * UIGetScreenToLogRatioX(pTooltip->m_bNoScale);
	float fTooltipHeight = pTooltip->m_fHeight * UIGetScreenToLogRatioY(pTooltip->m_bNoScale);

	if (!pRect)
	{
		// Center on the screen
		nSuggestRelativePos = TTPOS_CENTER;
	}


	int nNumTries = 0;
	BOOL bOk = FALSE;
	float fFudge = 0.0f;
	while (nNumTries < NUM_TTPOS * 2)
	{
		switch(nSuggestRelativePos)
		{
		case TTPOS_RIGHT_TOP:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX2 + pTooltip->m_fXOffset - fFudge;
				pTooltip->m_Position.m_fY = pRect->m_fY1 + pTooltip->m_fYOffset;
			}
			break;
		case TTPOS_RIGHT_BOTTOM:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX2 + pTooltip->m_fXOffset - fFudge;
				pTooltip->m_Position.m_fY = (pRect->m_fY2 - fTooltipHeight) + pTooltip->m_fYOffset;
			}
			break;
		case TTPOS_LEFT_TOP:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX1 - (fTooltipWidth + pTooltip->m_fXOffset + fFudge);
				pTooltip->m_Position.m_fY = pRect->m_fY1 + pTooltip->m_fYOffset;
			}
			break;
		case TTPOS_LEFT_BOTTOM:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX1 - (fTooltipWidth + pTooltip->m_fXOffset + fFudge);
				pTooltip->m_Position.m_fY = (pRect->m_fY2 - fTooltipHeight) + pTooltip->m_fYOffset;
			}
			break;
		case TTPOS_BOTTOM_LEFT:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX1;
				pTooltip->m_Position.m_fY = pRect->m_fY2 + pTooltip->m_fYOffset + fFudge;
			}
			break;
		case TTPOS_BOTTOM_RIGHT:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX2 - fTooltipWidth;
				pTooltip->m_Position.m_fY = pRect->m_fY2 + pTooltip->m_fYOffset + fFudge;
			}
			break;
		case TTPOS_TOP_LEFT:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX1;
				pTooltip->m_Position.m_fY = pRect->m_fY1 - (fTooltipHeight + pTooltip->m_fYOffset + fFudge);
			}
			break;
		case TTPOS_TOP_RIGHT:
			{
				pTooltip->m_Position.m_fX = pRect->m_fX2 - fTooltipWidth;
				pTooltip->m_Position.m_fY = pRect->m_fY1 - (fTooltipHeight + pTooltip->m_fYOffset + fFudge);
			}
			break;
		case TTPOS_CENTER:
			{
				if( AppIsHellgate() )
				{
					pTooltip->m_Position.m_fX = (fAppWidth - fTooltipWidth) / 2.0f;
					pTooltip->m_Position.m_fY = (fAppHeight - fTooltipHeight) / 2.0f;
				}
			}
			break;
		}

		bOk = TRUE;
		UI_RECT rectTooltip(pTooltip->m_Position.m_fX, pTooltip->m_Position.m_fY, pTooltip->m_Position.m_fX + fTooltipWidth, pTooltip->m_Position.m_fY + fTooltipHeight);
		UI_RECT rectApp(0.0f, 0.0f, fAppWidth, fAppHeight);
		if (pRect && !UIEntirelyInRect(rectTooltip, rectApp))
		{
			bOk = FALSE;
		}

		if (bOk && pAvoidRect && UIInRect(rectTooltip, *pAvoidRect))
		{
			bOk = FALSE;
		}

		if (!bOk)
		{
			// try again
			nSuggestRelativePos++;
			if (nSuggestRelativePos >= NUM_TTPOS)
			{
				// try all of them again with an offset
				fFudge += AppIsHellgate() ? 22.0f : 50.0f;
				nSuggestRelativePos = 0;
			}
			nNumTries++;

		}
		else
		{
			pTooltip->m_ActivePos = pTooltip->m_Position;
			pTooltip->m_InactivePos = pTooltip->m_Position;
			break;
		}
	}
	if( AppIsTugboat() && !bOk)
	{
		pTooltip->m_Position.m_fX = pRect->m_fX2 + pTooltip->m_fXOffset;
		pTooltip->m_Position.m_fY = pRect->m_fY1 + pTooltip->m_fYOffset;
		UI_RECT rect1(pTooltip->m_Position.m_fX, pTooltip->m_Position.m_fY, pTooltip->m_Position.m_fX + fTooltipWidth, pTooltip->m_Position.m_fY + fTooltipHeight);
		UI_RECT rect2(0.0f, 0.0f, fAppWidth, fAppHeight);
		if (rect1.m_fX1 < rect2.m_fX1)
		{
			pTooltip->m_Position.m_fX += rect2.m_fX1 - rect1.m_fX1;
		}
		if (rect1.m_fX2 > rect2.m_fX2)
		{
			pTooltip->m_Position.m_fX += rect2.m_fX2 - rect1.m_fX2;
		}
		if (rect1.m_fY1 < rect2.m_fY1)
		{
			pTooltip->m_Position.m_fY += rect2.m_fY1 - rect1.m_fY1;
		}
		if (rect1.m_fY2 > rect2.m_fY2)
		{
			pTooltip->m_Position.m_fY += rect2.m_fY2 - rect1.m_fY2;
		}		
		pTooltip->m_ActivePos = pTooltip->m_Position;
		pTooltip->m_InactivePos = pTooltip->m_Position;
	}

	UISetNeedToRender(pTooltip);
	return bOk;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITooltipOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_TOOLTIP *pTooltip = UICastToTooltip(component);

	if (pTooltip->m_bAutosize)
	{
		UITooltipDetermineContents(pTooltip);
		UITooltipDetermineSize(pTooltip);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIItemTooltipOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNIT *pFocus = UIComponentGetFocusUnit(component);
	
	if (!pFocus)
	{
		//UIComponentSetVisible(component, FALSE);
		return UIMSG_RET_HANDLED;
	}

	return UITooltipOnPaint(component, msg, wParam, lParam);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQualityBadgeOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements(component);
	component->m_pFrame = NULL;

	UNIT *pUnit = UIComponentGetFocusUnit(component);
	if (pUnit)
	{
		if (UnitIsA(pUnit, UNITTYPE_ITEM))
		{
			int nQuality = ItemGetQuality(pUnit);
			if (nQuality != INVALID_LINK)
			{
				const ITEM_QUALITY_DATA *pItemQualityData = ItemQualityGetData( nQuality );
				ASSERT_RETVAL(pItemQualityData, UIMSG_RET_HANDLED);

				UI_COMPONENT *pLabel = UIComponentIterateChildren(component, NULL, UITYPE_LABEL, FALSE);
				if (pLabel)
				{
					if (pItemQualityData->szTooltipBadgeFrame[0])
					{
						UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
						ASSERT_RETVAL(pTexture, UIMSG_RET_HANDLED);
						component->m_pFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pTexture->m_pFrames, pItemQualityData->szTooltipBadgeFrame);

						pLabel->m_dwColor = (pItemQualityData->nNameColor != INVALID_LINK ? GetFontColor(pItemQualityData->nNameColor) : GFXCOLOR_WHITE);
						UILabelSetTextByStringIndex(pLabel, pItemQualityData->nDisplayName);

						if (component->m_pFrame)
						{
							UI_RECT rect((float)component->m_dwParam2, pLabel->m_Position.m_fY + pLabel->m_fHeight, (float)component->m_dwParam, component->m_pFrame->m_fHeight - (float)component->m_dwParam2);
							rect.m_fX2 *= UIGetLogToScreenRatioY() / UIGetLogToScreenRatioX();
							UI_ELEMENT_PRIMITIVE *pElement = UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, rect, NULL, NULL, pLabel->m_dwColor);
							pElement->m_nRenderSection--;
						}
					}
					else
					{
						UILabelSetText(pLabel, L"");
					}
				}
			}
		}
	}

	if (component->m_pFrame)
	{
		UIComponentAddFrame(component);
	}
	
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollBarOnPaint(
	UI_COMPONENT *component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if ( e_IsNoRender() )
		return UIMSG_RET_HANDLED;

	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentOnPaint(component, msg, wParam, lParam);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	if (pScrollBar->m_pThumbpadFrame)
	{
		UI_POSITION pos;
		float fSlantWidth = (pScrollBar->m_pBar ? pScrollBar->m_pBar->m_fSlantWidth : 0.0f);
		float fWidth = (pScrollBar->m_pBar ? pScrollBar->m_pBar->m_fWidth : pScrollBar->m_fWidth);
		float fHeight = (pScrollBar->m_pBar ? pScrollBar->m_pBar->m_fHeight : pScrollBar->m_fHeight);

		if (!pScrollBar->m_pBar)
		{
			fHeight -= pScrollBar->m_pThumbpadFrame->m_fHeight * UIGetScreenToLogRatioY( pScrollBar->m_bNoScale );
			if( fHeight < 0 )
			{
				fHeight = 0;
			}

			fWidth -= pScrollBar->m_pThumbpadFrame->m_fWidth * UIGetScreenToLogRatioX( pScrollBar->m_bNoScale );
			if( fWidth < 0 )
			{
				fWidth = 0;
			}
		}

		float ratio = 1.0f;
		if( pScrollBar->m_fMax != pScrollBar->m_fMin ) 
		{
			float fValue = (pScrollBar->m_pBar ? (float)pScrollBar->m_pBar->m_nValue : pScrollBar->m_ScrollPos.m_fY);
			ratio = (fValue - pScrollBar->m_fMin) / fabs(pScrollBar->m_fMax - pScrollBar->m_fMin);
		}
		switch (pScrollBar->m_nOrientation)
		{
			case UIORIENT_LEFT:
				pos.m_fX = ((fWidth - fSlantWidth) * ratio ) + fSlantWidth;
				break;
			case UIORIENT_RIGHT:
				pos.m_fX = ((fWidth - fSlantWidth) * (1.0f - ratio)) + fSlantWidth;
				break;
			case UIORIENT_TOP:
				pos.m_fY = fHeight * ratio;
				break;
			case UIORIENT_BOTTOM:
				pos.m_fY = fHeight * (1.0f - ratio);
				break;
		}
		UI_COMPONENT *pComponent = pScrollBar;
		if (pScrollBar->m_pBar)
		{
			pComponent = pScrollBar->m_pBar;
			if( pComponent->m_pGfxElementFirst && pComponent->m_pGfxElementFirst->m_pComponent->m_pFrame == pScrollBar->m_pThumbpadFrame)
				UIComponentRemoveAllElements( pComponent );
		}

		pScrollBar->m_pThumbpadElement = UIComponentAddElement(pComponent, UIComponentGetTexture(component), pScrollBar->m_pThumbpadFrame, pos, GFXCOLOR_HOTPINK, NULL);

		// special case for bar masks (no, not *that* kind of bar mask)
		if (pScrollBar->m_pBar)
		{
			UI_COMPONENT *pChild = NULL;
			while ((pChild = UIComponentIterateChildren(pScrollBar, pChild, -1, FALSE)) != NULL)
			{
				if (pChild->m_fZDelta < pScrollBar->m_pThumbpadElement->m_fZDelta)
				{
					pScrollBar->m_pThumbpadElement->m_fZDelta = pChild->m_fZDelta - 0.001f; 
				}
			}
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollBarOnMouseWheel(
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

	if (!UICursorGetActive())
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	if (UIComponentCheckBounds(pScrollBar) ||
		(pScrollBar->m_pScrollControl && UIComponentCheckBounds(pScrollBar->m_pScrollControl)))
	{
		pScrollBar->m_ScrollPos.m_fY += (int)lParam > 0 ? -pScrollBar->m_fWheelScrollIncrement : pScrollBar->m_fWheelScrollIncrement;
		pScrollBar->m_ScrollPos.m_fY = PIN(pScrollBar->m_ScrollPos.m_fY, pScrollBar->m_fMin, pScrollBar->m_fMax);
		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, lParam);
		UISetNeedToRender(pScrollBar);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollBarOnMouseMove(
	UI_COMPONENT *component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	if (pScrollBar->m_bMouseDownOnThumbpad)
	{
		ASSERT_RETVAL(pScrollBar->m_pThumbpadElement, UIMSG_RET_NOT_HANDLED);

		UI_POSITION posComponent;
		UI_POSITION posMouse;
		UIGetCursorPosition(&posMouse.m_fX, &posMouse.m_fY);
		float fSlantWidth = 0.0f;
		float fWidth = pScrollBar->m_fWidth;
		float fHeight = pScrollBar->m_fHeight;
		if (pScrollBar->m_pBar)
		{
			fSlantWidth = pScrollBar->m_pBar->m_fSlantWidth;
			fWidth = pScrollBar->m_pBar->m_fWidth;
			fHeight = pScrollBar->m_pBar->m_fHeight;
			posComponent = UIComponentGetAbsoluteLogPosition(pScrollBar->m_pBar);
		}
		else
		{
			posComponent = UIComponentGetAbsoluteLogPosition(pScrollBar);
		}
		if( pScrollBar->m_pThumbpadFrame )
		{
//			float fOriginalHeight = fHeight;
			fHeight -= pScrollBar->m_pThumbpadFrame->m_fHeight;
			if( fHeight < 0 )
			{
				fHeight = 0;
			}
			//float Percentage = fHeight / fOriginalHeight;
			//fSlantWidth *= Percentage;
		}
		if (pScrollBar->m_nOrientation == UIORIENT_LEFT || 
			pScrollBar->m_nOrientation == UIORIENT_RIGHT)
		{
			float fX = posMouse.m_fX - (posComponent.m_fX + fSlantWidth);
			fX -= pScrollBar->m_posMouseDownOffset.m_fX;
			fX /= (fWidth - fSlantWidth) * UIGetScreenToLogRatioX( pScrollBar->m_bNoScale );
			pScrollBar->m_ScrollPos.m_fY = (fX * pScrollBar->m_fMax) + pScrollBar->m_fMin;
		}
		else
		{
			float fY = posMouse.m_fY - (posComponent.m_fY + pScrollBar->m_posMouseDownOffset.m_fY);
			fY /= fHeight * UIGetScreenToLogRatioY( pScrollBar->m_bNoScale );
			pScrollBar->m_ScrollPos.m_fY = (fY * fabs(pScrollBar->m_fMax - pScrollBar->m_fMin)) + pScrollBar->m_fMin;
		}

		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
	}

	return UIMSG_RET_NOT_HANDLED;
}

BOOL UIScrollBarCheckBounds(
	UI_SCROLLBAR *pScrollBar)
{
	UI_POSITION posMouse;
	UIGetCursorPosition(&posMouse.m_fX, &posMouse.m_fY);
	if (UIComponentCheckBounds(pScrollBar, posMouse.m_fX, posMouse.m_fY))
	{
		return TRUE;
	}

	if (pScrollBar->m_pThumbpadElement)
	{
		UI_POSITION posComponent;
		if (pScrollBar->m_pBar)
		{
			posComponent = UIComponentGetAbsoluteLogPosition(pScrollBar->m_pBar);
		}
		else
		{
			posComponent = UIComponentGetAbsoluteLogPosition(pScrollBar);
		}

		posMouse -= posComponent;
		UI_RECT rectThumbpad;
		rectThumbpad.m_fX1 = pScrollBar->m_pThumbpadElement->m_Position.m_fX - pScrollBar->m_pThumbpadElement->m_fXOffset;
		rectThumbpad.m_fY1 = pScrollBar->m_pThumbpadElement->m_Position.m_fY - pScrollBar->m_pThumbpadElement->m_fYOffset;
		rectThumbpad.m_fX2 = rectThumbpad.m_fX1 + pScrollBar->m_pThumbpadElement->m_fWidth;
		rectThumbpad.m_fY2 = rectThumbpad.m_fY1 + pScrollBar->m_pThumbpadElement->m_fHeight;

		if (UIInRect(posMouse, rectThumbpad))
		{
			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCheckThumbpadElement(UI_SCROLLBAR *pScrollBar, UI_POSITION posComponent, UI_POSITION posMouse)
{
	posMouse -= posComponent;
	UI_RECT rectThumbpad;
	if ( ! pScrollBar || ! pScrollBar->m_pThumbpadElement )
		return FALSE;
	rectThumbpad.m_fX1 = pScrollBar->m_pThumbpadElement->m_Position.m_fX - pScrollBar->m_pThumbpadElement->m_fXOffset;
	rectThumbpad.m_fY1 = pScrollBar->m_pThumbpadElement->m_Position.m_fY - pScrollBar->m_pThumbpadElement->m_fYOffset;
	rectThumbpad.m_fX2 = rectThumbpad.m_fX1 + pScrollBar->m_pThumbpadElement->m_fWidth;
	rectThumbpad.m_fY2 = rectThumbpad.m_fY1 + pScrollBar->m_pThumbpadElement->m_fHeight;

	posMouse.m_fX /= UIGetScreenToLogRatioX( pScrollBar->m_bNoScale );
	posMouse.m_fY /= UIGetScreenToLogRatioY( pScrollBar->m_bNoScale );
	if (UIInRect(posMouse, rectThumbpad))
	{
		pScrollBar->m_bMouseDownOnThumbpad = TRUE;
		pScrollBar->m_posMouseDownOffset.m_fX = posMouse.m_fX - rectThumbpad.m_fX1 - pScrollBar->m_pThumbpadElement->m_fXOffset;
		pScrollBar->m_posMouseDownOffset.m_fY = posMouse.m_fY - rectThumbpad.m_fY1 - pScrollBar->m_pThumbpadElement->m_fYOffset;

		//trace("m_posMouseDownOffset.m_fX = %0.2f\n", pScrollBar->m_posMouseDownOffset.m_fX );
		return TRUE;
	}
	return FALSE;
}

UI_MSG_RETVAL UIScrollBarOnLButtonDown(
	UI_COMPONENT *component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	UI_POSITION posComponent;
	UI_POSITION posMouse;
	UIGetCursorPosition(&posMouse.m_fX, &posMouse.m_fY);
	if (pScrollBar->m_pBar)
	{
		posComponent = UIComponentGetAbsoluteLogPosition(pScrollBar->m_pBar);
	}
	else
	{
		posComponent = UIComponentGetAbsoluteLogPosition(pScrollBar);
	}

	if (pScrollBar->m_pThumbpadElement)
	{
		if (sCheckThumbpadElement(pScrollBar, posComponent, posMouse))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	float fSlantWidth = 0.0f;
	if (pScrollBar->m_pBar)
	{
		fSlantWidth = pScrollBar->m_pBar->m_fSlantWidth;
		component = pScrollBar->m_pBar;
	}
	if (UIComponentCheckBounds(component))
	{
		if (pScrollBar->m_nOrientation == UIORIENT_LEFT || 
			pScrollBar->m_nOrientation == UIORIENT_RIGHT)
		{
			float fWidth = component->m_fWidth;
			if (!pScrollBar->m_pBar && pScrollBar->m_pThumbpadElement)
			{
				fWidth -= pScrollBar->m_pThumbpadElement->m_fWidth;
			}
			float fX = posMouse.m_fX - (posComponent.m_fX + fSlantWidth);
			fX -= pScrollBar->m_posMouseDownOffset.m_fX;
			fX /= fWidth - fSlantWidth;
			fX /= UIGetScreenToLogRatioX( pScrollBar->m_bNoScale );
			pScrollBar->m_ScrollPos.m_fY = (fX * pScrollBar->m_fMax) + pScrollBar->m_fMin;
		}
		else
		{
			float fHeight = component->m_fHeight;
			if (!pScrollBar->m_pBar && pScrollBar->m_pThumbpadElement)
			{
				fHeight -= pScrollBar->m_pThumbpadElement->m_fHeight;
			}
			float fY = posMouse.m_fY - (posComponent.m_fY + pScrollBar->m_posMouseDownOffset.m_fY);
			fY /= fHeight;
			fY /= UIGetScreenToLogRatioY( pScrollBar->m_bNoScale );
			pScrollBar->m_ScrollPos.m_fY = (fY * fabs(pScrollBar->m_fMax - pScrollBar->m_fMin)) + pScrollBar->m_fMin;
		}

		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
		sCheckThumbpadElement(pScrollBar, posComponent, posMouse);

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollBarOnLButtonUp(
	UI_COMPONENT *component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	pScrollBar->m_bMouseDownOnThumbpad = FALSE;
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// bMappedToRange : if TRUE, returns value mapped to 0..1
float UIScrollBarGetValue(
	UI_SCROLLBAR *pScrollBar,
	BOOL bMappedToRange /*= FALSE*/ )
{
	ASSERT_RETVAL(pScrollBar, 0.0f);

	if ( bMappedToRange )
		return MAP_VALUE_TO_RANGE( pScrollBar->m_ScrollPos.m_fY, pScrollBar->m_fMin, pScrollBar->m_fMax, 0.f, 1.f );

	return pScrollBar->m_ScrollPos.m_fY;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// bMappedToRange : if TRUE, returns value mapped to 0..1
float UIScrollBarGetValue(
	UI_COMPONENT *pParent,
	const char *szScrollbarName,
	BOOL bMappedToRange /*= FALSE*/ )
{
	ASSERT_RETVAL(pParent, 0.0f);

	UI_COMPONENT *pChild = UIComponentFindChildByName(pParent, szScrollbarName);
	if (pChild)
	{
		UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pChild);
		return UIScrollBarGetValue(pScrollBar, bMappedToRange);
	}

	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// bMappedToRange : if TRUE, assumes fValue is 0..1 and maps to fMin..fMax
void UIScrollBarSetValue(
	UI_SCROLLBAR *pScrollBar,
	float fValue,
	BOOL bMappedToRange /*= FALSE*/ )
{
	ASSERT_RETURN(pScrollBar);

	if ( bMappedToRange )
	{
		fValue = MAP_VALUE_TO_RANGE( fValue, 0.f, 1.f, pScrollBar->m_fMin, pScrollBar->m_fMax );
	}

	pScrollBar->m_ScrollPos.m_fY = PIN(fValue, pScrollBar->m_fMin, pScrollBar->m_fMax);

	if (pScrollBar->m_pScrollControl)
	{
		pScrollBar->m_pScrollControl->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
		UISetNeedToRender(pScrollBar->m_pScrollControl);
		UIComponentHandleUIMessage(pScrollBar->m_pScrollControl, UIMSG_SCROLL, 0, 0);
	}

	if (pScrollBar->m_pBar)
	{
		pScrollBar->m_pBar->m_nValue = (int)floor( pScrollBar->m_ScrollPos.m_fY + .5f );
	}

	if (pScrollBar->m_bSendPaintToParent)
		UIComponentHandleUIMessage(pScrollBar->m_pParent, UIMSG_PAINT, 0, 0);
	else
		UIComponentHandleUIMessage(pScrollBar, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// bMappedToRange : if TRUE, assumes fValue is 0..1 and maps to fMin..fMax
void UIScrollBarSetValue(
	UI_COMPONENT *pParent,
	const char *szScrollbarName,
	float fValue,
	BOOL bMappedToRange /*= FALSE*/ )
{
	ASSERT_RETURN(pParent);

	UI_COMPONENT *pChild = UIComponentFindChildByName(pParent, szScrollbarName);
	if (pChild)
	{
		UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pChild);
		UIScrollBarSetValue(pScrollBar, fValue, bMappedToRange);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollBarOnScroll(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(component);

	UIScrollBarSetValue(pScrollBar, pScrollBar->m_ScrollPos.m_fY);

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIScrollFramePaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIComponentRemoveAllElements(component);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETVAL(pTexture, UIMSG_RET_NOT_HANDLED);

	UI_RECT cliprect = UIComponentGetRect(component);

	UIComponentAddElement(component, pTexture, component->m_pFrame, component->m_posFrameScroll, GFXCOLOR_HOTPINK, &cliprect);

	if (component->m_posFrameScroll.m_fX != 0.0f ||
		component->m_posFrameScroll.m_fY != 0.0f)
	{
		// wrap the frame (only doing Y right now)
		UI_POSITION pos = component->m_posFrameScroll;
		pos.m_fY -= (component->m_bStretch ? component->m_fHeight : component->m_pFrame->m_fHeight);
		UIComponentAddElement(component, pTexture, component->m_pFrame, pos, GFXCOLOR_HOTPINK, &cliprect);
	}

	float fDelta = (component->m_fFrameScrollSpeed < 1.0f ? 1.0f / component->m_fFrameScrollSpeed : 1.0f);

	if (component->m_bScrollFrameReverse)
	{
		component->m_posFrameScroll.m_fY -= fDelta;

		if (component->m_posFrameScroll.m_fY < 0.0f)
			component->m_posFrameScroll.m_fY = component->m_fHeight;
	}
	else
	{
		component->m_posFrameScroll.m_fY += fDelta;

		if (component->m_posFrameScroll.m_fY >= component->m_fHeight)
			component->m_posFrameScroll.m_fY = 0.0f;
	}

	if (component->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(component->m_tMainAnimInfo.m_dwAnimTicket);
	}
	if (UIComponentGetVisible(component))
	{
		component->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterMessage(AppCommonGetCurTime() + (int)(component->m_fFrameScrollSpeed < 1.0f ? 1.0f : component->m_fFrameScrollSpeed), component, UIMSG_PAINT, 0, 0);
	}
	else
	{
		component->m_bNeedsRepaintOnVisible = TRUE;
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMenuOnMouseOver(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT(component);
	ASSERT(UIComponentIsMenu(component));

	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}

	BOOL bOneSelected = FALSE;
	UI_LABELEX *pPrevSelected = NULL;
	UI_COMPONENT *pChild = component->m_pFirstChild;
	while (pChild)
	{
		if (UIComponentIsLabel(pChild))
		{
			UI_LABELEX *pLabel = UICastToLabel(pChild);
			if (pLabel->m_nMenuOrder != -1)
			{
				if (pLabel->m_bSelected)
					pPrevSelected = pLabel;

				if (UIComponentCheckBounds(pLabel) &&
					!bOneSelected)  // only select one item
				{
					pLabel->m_bSelected = TRUE;
					bOneSelected = TRUE;
				}
				else
				{
					pLabel->m_bSelected = FALSE;
				}
			}
		}

		pChild = pChild->m_pNextSibling;
	}

	UI_MENU *pMenu = UICastToMenu(component);
	if (!bOneSelected && pMenu->m_bAlwaysOneSelected && pPrevSelected)
	{
		pPrevSelected->m_bSelected = TRUE;
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	return UIMSG_RET_HANDLED_END_PROCESS;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Forwards
void UISetCurrentMenu(UI_COMPONENT * pMenu);

UI_MSG_RETVAL UIMenuOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT(component);
	ASSERT(UIComponentIsMenu(component));

	// This is all in case an active menu is closed with a method other
	//  than SetCurrentMenu
	if (g_UI.m_pCurrentMenu == component)
	{
		UISetCurrentMenu(NULL);
	}

	// just in case
	UIRemoveMouseOverrideComponent(component);
	UIRemoveKeyboardOverrideComponent(component);


	return UIMSG_RET_HANDLED_END_PROCESS;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_MSG_RETVAL sToggleChatPopupMenu(
	UI_COMPONENT *pComponent,
	UI_COMPONENT_ENUM eMenu)
{
	ASSERT(pComponent);
	
	// test cursor position`
	float flX;
	float flY;
	UIGetCursorPosition( &flX, &flY );
	if (UIComponentGetActive( pComponent ) == FALSE || 
		UIComponentCheckBounds( pComponent, flX, flY ) == FALSE)
	{
		return UIMSG_RET_NOT_HANDLED;  
	}

	// get menu
	UI_COMPONENT *pMenu = UIComponentGetByEnum( eMenu );
	if (pMenu == NULL)
	{
		return UIMSG_RET_NOT_HANDLED;  
	}

	if( UIComponentGetActive( pMenu ) )
	{
		UIComponentHandleUIMessage(pMenu, UIMSG_INACTIVATE, 0, 0);
		UIComponentSetVisible(pMenu, FALSE);
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}
	
	// set menu position
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pComponent);


	pMenu->m_Position = pos;
	pMenu->m_Position.m_fY-= pMenu->m_fHeight * UIGetScreenToLogRatioY( pMenu->m_bNoScale );
	pMenu->m_ActivePos = pMenu->m_Position;
	pMenu->m_InactivePos = pMenu->m_Position;


	// activate menu
	UIComponentHandleUIMessage( pMenu, UIMSG_ACTIVATE, 0, 0 );	
	return UIMSG_RET_HANDLED_END_PROCESS;  

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenChatCommandPopupMenu(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{ 
	if( UIComponentGetActive( UICOMP_EMOTECOMMAND_POPUP_MENU ) )
	{
		sToggleChatPopupMenu( component, UICOMP_EMOTECOMMAND_POPUP_MENU );
	}

	return sToggleChatPopupMenu( component, UICOMP_CHATCOMMAND_POPUP_MENU );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenEmoteCommandPopupMenu(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{ 

	if( UIComponentGetActive( UICOMP_CHATCOMMAND_POPUP_MENU ) )
	{
		sToggleChatPopupMenu( component, UICOMP_CHATCOMMAND_POPUP_MENU );
	}
	return sToggleChatPopupMenu( component, UICOMP_EMOTECOMMAND_POPUP_MENU );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISlashCommand(
	GLOBAL_STRING eGlobalString,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	const WCHAR *puszCommand = GlobalStringGet( eGlobalString );
	ASSERTX_RETURN( puszCommand, "Expected command" );
	PStrPrintf( puszBuffer, nBufferSize, L"/%s ", puszCommand );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UISlashCommandSetInput(
	GLOBAL_STRING eGlobalString)
{
	const int MAX_COMMAND = 256;
	WCHAR uszCommand[ MAX_COMMAND ];
	
	// get the command
	UISlashCommand( eGlobalString, uszCommand, MAX_COMMAND );
	
	// set into console
	ConsoleSetInputLine(uszCommand, ConsoleInputColor(), 0);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIChatCommand(
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
	ConsoleSetEditActive(TRUE);
	UNIT* pTargetUnit = NULL;
	if( UIPlayerInteractMenuOpen() )
	{
		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
		pTargetUnit = UIComponentGetFocusUnit(pComp);
	}
		
	switch( pLabel->m_nMenuOrder )
	{
		case 0:
			UISlashCommandSetInput( GS_CCMD_CHAT_LOCAL );
			break;
		case 1:
			UISlashCommandSetInput( GS_CCMD_CHAT_WHISPER );
			break;
		case 2:
			if( pTargetUnit )
			{
				c_PlayerSetupWhisperTo( pTargetUnit );
			}
			else
			{
				UISlashCommandSetInput( GS_CCMD_CHAT_WHISPER );
			}
			break;
		case 3:
			UISlashCommandSetInput( GS_CCMD_CHAT_SHOUT );
			break;
		case 4:
			UISlashCommandSetInput( GS_CCMD_CHAT_YELL );
			break;
		case 5:
			UISlashCommandSetInput( GS_CCMD_CHAT_REPLY );		
			break;
		case 6:
			UISlashCommandSetInput( GS_CCMD_CHAT_PARTY );		
			break;
		case 7:
			UISlashCommandSetInput( GS_CCMD_IGNORE );		
			break;
		case 8:
			UISlashCommandSetInput( GS_CCMD_UNIGNORE );		
			break;
		case 9:
			UISlashCommandSetInput( GS_CCMD_WHO );
			ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
			break;
		case 10:
			UISlashCommandSetInput( GS_CCMD_INVITE );
			break;
		case 11:
			UISlashCommandSetInput( GS_CCMD_ACCEPT );
			ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
			break;
		case 12:
			UISlashCommandSetInput( GS_CCMD_DECLINE );
			ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
			break;
		case 13:
			UISlashCommandSetInput( GS_CCMD_DISBAND );		
			ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
			break;
		case 14:
			UISlashCommandSetInput( GS_CCMD_LEAVE );
			ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
			break;
		case 15:
			UISlashCommandSetInput( GS_CCMD_CHAT_CHANNEL_LIST );
			ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
			break;
		case 16:
			UISlashCommandSetInput( GS_CCMD_CHAT_CHANNELJOIN );
			break;
		case 17:
			UISlashCommandSetInput( GS_CCMD_CHAT_CHANNELLEAVE );
			break;
		case 18:
			UISlashCommandSetInput( GS_CCMD_GUILD_CHAT );
			break;
		case 19:
			UISlashCommandSetInput( GS_CCMD_EMOTE );
			break;
	}
	UIClosePopupMenus();
	return UIMSG_RET_HANDLED_END_PROCESS;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEmoteCommand(
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
	ConsoleSetEditActive(TRUE);
	UNIT* pTargetUnit = NULL;
	if( UIPlayerInteractMenuOpen() )
	{
		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
		pTargetUnit = UIComponentGetFocusUnit(pComp);
	}

	switch( pLabel->m_nMenuOrder )
	{
	case 0:
		UISlashCommandSetInput( GS_CCMD_EMOTE_CRY_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 1:
		UISlashCommandSetInput( GS_CCMD_EMOTE_CHEER_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 2:
		UISlashCommandSetInput( GS_CCMD_EMOTE_BOW_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 3:
		UISlashCommandSetInput( GS_CCMD_EMOTE_BEG_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 4:
		UISlashCommandSetInput( GS_CCMD_EMOTE_FLEX_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 5:
		UISlashCommandSetInput( GS_CCMD_EMOTE_KISS_ENG );		
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 6:
		UISlashCommandSetInput( GS_CCMD_EMOTE_LAUGH_ENG );		
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 7:
		UISlashCommandSetInput( GS_CCMD_EMOTE_POINT_ENG );		
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 8:
		UISlashCommandSetInput( GS_CCMD_EMOTE_STOP_ENG );		
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 9:
		UISlashCommandSetInput( GS_CCMD_EMOTE_TAUNT_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 10:
		UISlashCommandSetInput( GS_CCMD_EMOTE_CHICKEN_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 11:
		UISlashCommandSetInput( GS_CCMD_EMOTE_DOH_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 12:
		UISlashCommandSetInput( GS_CCMD_EMOTE_NO_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 13:
		UISlashCommandSetInput( GS_CCMD_EMOTE_SALUTE_ENG );		
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	case 14:
		UISlashCommandSetInput( GS_CCMD_EMOTE_WAVE_ENG );
		ConsoleHandleInputChar(AppGetCltGame(), VK_RETURN, 0);
		break;
	
	}
	UIClosePopupMenus();
	return UIMSG_RET_HANDLED_END_PROCESS;  
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPopupMenuClose(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT(component);

	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}

	UIComponentHandleUIMessage(component, UIMSG_INACTIVATE, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_MSG_RETVAL sOpenPopupMenu(
	UI_COMPONENT *pComponentPortrait,
	UI_COMPONENT_ENUM eMenu)
{
	ASSERT(pComponentPortrait);
	
	// test cursor position
	float flX;
	float flY;
	UIGetCursorPosition( &flX, &flY );
	if (UIComponentGetActive( pComponentPortrait ) == FALSE || 
		UIComponentCheckBounds( pComponentPortrait, flX, flY ) == FALSE)
	{
		return UIMSG_RET_NOT_HANDLED;  
	}

	// get menu
	UI_COMPONENT *pMenu = UIComponentGetByEnum( eMenu );
	if (pMenu == NULL)
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	
	if (AppIsHellgate())
	{
		UIGetCursorPosition(&pMenu->m_Position.m_fX, &pMenu->m_Position.m_fY);
		if (pMenu->m_pParent)
			pMenu->m_Position -= pMenu->m_pParent->m_Position;
	}
	else
	{
		pMenu->m_Position = pComponentPortrait->m_Position;
		pMenu->m_Position.m_fX += pComponentPortrait->m_fWidth;
	}

	// set menu position
	pMenu->m_InactivePos = pMenu->m_Position;
	pMenu->m_ActivePos = pMenu->m_Position;

	// set focus unit
	UNITID idFocus = UIComponentGetFocusUnitId( pComponentPortrait );
	UIComponentSetFocusUnit( pMenu, idFocus );

	// save index in party 
	pMenu->m_dwData = pComponentPortrait->m_dwData;
	
	// activate menu
	UIComponentHandleUIMessage( pMenu, UIMSG_ACTIVATE, 0, 0 );

	return UIMSG_RET_HANDLED_END_PROCESS;  

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenPartyMemberPopupMenu(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{ 
	UI_MSG_RETVAL retVal = sOpenPopupMenu( component, UICOMP_PARTYMEMBER_POPUP_MENU );

	if (AppTestFlag(AF_CENSOR_NO_PVP))
	{
		UI_COMPONENT *pPopup = UIComponentGetByEnum(UICOMP_PARTYMEMBER_POPUP_MENU);
		ASSERT_RETVAL(pPopup, UIMSG_RET_NOT_HANDLED);

		UI_COMPONENT *pDuelItem = UIComponentFindChildByName(pPopup, "partymember popup menu duel");
		UIComponentSetVisible(pDuelItem, FALSE);
	}

	return retVal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIOpenPetPopupMenu(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	return sOpenPopupMenu( component, UICOMP_PET_POPUP_MENU );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractPanelSetFocusVisible(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UNITID idUnit = (UNITID)wParam;


	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
	UIComponentSetFocusUnit( pComp, idUnit);
	UNIT *pUnit = UnitGetById(AppGetCltGame(), idUnit);
	UIUpdatePlayerInteractPanel( pUnit );


	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIPlayerInteractMenuOpen( void )
{
	UI_COMPONENT *pPopup = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_POPUP_MENU);
	return UIComponentGetVisible( pPopup );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIUpdatePlayerInteractPanel( UNIT* pUnit )
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
	GAME *pGame = pUnit ? UnitGetGame( pUnit ) : NULL;
	BOOL bIsOverworld = ( pUnit && UnitGetRoom( pUnit ) )? MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( UnitGetLevelAreaID( pUnit ) ) : FALSE;
	if ( pGame &&
		!GameGetGameFlag(pGame, GAMEFLAG_DISABLE_PLAYER_INTERACTION) && 
		UnitIsPlayer(pUnit) && 
		!InputIsCommandKeyDown( CMD_ENGAGE_PVP ) &&
		!TestHostile( pGame, UIGetControlUnit(), pUnit ) &&
		( bIsOverworld || LevelIsSafe( UnitGetLevel( pUnit ) ) || PlayerIsInPVPWorld( pUnit ) ) )	// have to be in town for this!
	{
	    UIComponentSetActive( pComp, TRUE );
	    UIComponentSetVisible( pComp, TRUE );
		UI_COMPONENT *pPopup = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_POPUP_MENU);
		if( !e_GetUICovered() && pUnit && !UIComponentGetVisible( pPopup ) )
		{
			int x, y;
			VECTOR vLoc;
			vLoc = UnitGetPosition( pUnit );
			vLoc.fZ += UnitGetCollisionHeight( pUnit ) * 1.4f;
			TransformWorldSpaceToScreenSpace( &vLoc, &x, &y );

			pComp->m_Position.m_fX = (float)x ;
			pComp->m_Position.m_fY = (float)y;

		}
	}
	else
	{
	    UIComponentSetActive( pComp, FALSE );
	    UIComponentSetVisible( pComp, FALSE );
		pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_POPUP_MENU);
	    UIComponentSetActive( pComp, FALSE );
	    UIComponentSetVisible( pComp, FALSE );

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
	UNIT* pUnit = UIComponentGetFocusUnit(pComp);
	UIComponentSetFocusUnit( pComp, UnitGetId( pUnit ));
	UIUpdatePlayerInteractPanel( pUnit );
	
	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractMouseOver(
									UI_COMPONENT* component,
									int msg,
									DWORD wParam,
									DWORD lParam)
{
	UIButtonOnMouseOver( component, msg, wParam, lParam );
	return UIMSG_RET_NOT_HANDLED; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractPopupOver(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	UIMenuOnMouseOver( component, msg, wParam, lParam );
	return UIMSG_RET_NOT_HANDLED; 
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	ASSERT(component);
	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_COMPONENT *pMenu = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_POPUP_MENU);
	if (!pMenu)
		return UIMSG_RET_NOT_HANDLED;  

	if (UIComponentGetActive(pMenu) && !UIComponentCheckBounds(pMenu, x, y))
	{

			UIPopupMenuClose( pMenu, msg, wParam, lParam );
		return UIMSG_RET_NOT_HANDLED;  
	}
	if (!UIComponentGetActive(component) || !UIComponentCheckBounds(component, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}

	InputSetInteracted( );

	//UIGetCursorPosition(&pMenu->m_Position.m_fX, &pMenu->m_Position.m_fY);
	//if (pMenu->m_pParent)
	//	pMenu->m_Position -= pMenu->m_pParent->m_Position;

	pMenu->m_InactivePos = pMenu->m_Position;
	pMenu->m_ActivePos = pMenu->m_Position;

	UIComponentSetFocusUnit(pMenu, UIComponentGetFocusUnitId(component));

	UIComponentHandleUIMessage(pMenu, UIMSG_ACTIVATE, 0, 0);

	return UIMSG_RET_HANDLED_END_PROCESS;  
	/*
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_PLAYER_INTERACT_PANEL);
	UNIT* pUnit = UIComponentGetFocusUnit(pComp);
	UIUpdatePlayerInteractPanel( pUnit );
	
	return UIMSG_RET_HANDLED;*/
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractTrade( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	InputSetInteracted( );

	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled)
	{
		UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
		if (pUnit)
		{
			// for now all we do is try to start trade
			c_TradeAskOtherToTrade( UnitGetId( pUnit ) );

			//UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
	}
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractInvite( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	InputSetInteracted( );

	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled)
	{
		UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
		if (pUnit)
		{
			GAME * pGame = AppGetCltGame();
			c_PlayerInteract( pGame, pUnit, UNIT_INTERACT_PLAYERINVITE );
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;  // input used
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractKick( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	InputSetInteracted( );


	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled)
	{
		UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
		if (pUnit)
		{
			GAME * pGame = AppGetCltGame();
			c_PlayerInteract( pGame, pUnit, UNIT_INTERACT_PLAYERKICK );
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;  // input used
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractDisband( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	InputSetInteracted( );

	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled)
	{
		UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
		//if (pUnit)
		{
			GAME * pGame = AppGetCltGame();
			c_PlayerInteract( pGame, pUnit, UNIT_INTERACT_PLAYERDISBAND );
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;  // input used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractLeave( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	InputSetInteracted( );

	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled)
	{
		UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
		//if (pUnit)
		{
			GAME * pGame = AppGetCltGame();
			c_PlayerInteract( pGame, pUnit, UNIT_INTERACT_PLAYERLEAVE );
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
	}
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPlayerInteractDuel( 
									UI_COMPONENT *pComponent,
									int nMessage, 
									DWORD wParam, 
									DWORD lParam)
{
	float x, y;
	UIGetCursorPosition(&x, &y);
	if (!UIComponentGetActive(pComponent) || !UIComponentCheckBounds(pComponent, x, y))
	{
		return UIMSG_RET_NOT_HANDLED;  
	}
	InputSetInteracted( );

	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled)
	{
		UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
		if (pUnit)
		{

			c_DuelInviteAttempt( pUnit );
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
	}
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickTrade( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled && 
		UIComponentGetActive(pLabel) && 
		UIComponentCheckBounds(pLabel))
	{
		InputSetInteracted( );
		UNIT *pPartyMember = UIComponentGetFocusUnit( pComponent );
		if (pPartyMember)
		{
			if (UnitIsPlayer( pPartyMember ))
			{
				c_TradeAskOtherToTrade( UnitGetId( pPartyMember ) );
			}
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnPostVisibleTrade( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );
	BOOL bAllowed = FALSE;
	UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
	if (pUnit)
	{
		UNIT *pLocalPlayer = UIGetControlUnit();
	
		// can only trade with players
		if (pLocalPlayer && UnitIsPlayer( pUnit ) && TradeCanTradeWith( pLocalPlayer, pUnit ))
		{
			bAllowed = TRUE;
		}
				
	}

	pLabel->m_bDisabled = !bAllowed;
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);	
	
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickWarpToBtn( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	int nMemberIndex = (int)pComponent->m_pParent->m_dwData;
	PGUID guidPartyMember = c_PlayerGetPartyMemberGUID( nMemberIndex );
	if (guidPartyMember != INVALID_GUID)
	{
		InputSetInteracted( );
		c_PartyMemberWarpTo( guidPartyMember );
	}
	
	return UIMSG_RET_HANDLED_END_PROCESS;  
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickWarpTo( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled && 
		UIComponentGetActive(pLabel) && 
		UIComponentCheckBounds(pLabel))
	{
		UI_COMPONENT *pMenu = pComponent->m_pParent;
		if (pMenu)
		{
			InputSetInteracted( );
			int nMemberIndex = (int)pMenu->m_dwData;
			PGUID guidPartyMember = c_PlayerGetPartyMemberGUID( nMemberIndex );
			if (guidPartyMember != INVALID_GUID)
			{
				c_PartyMemberWarpTo( guidPartyMember );
			}
		}		
		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);
		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnPostVisibleWarpTo( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );
	BOOL bAllowed = FALSE;
	
	int nMemberIndex = (int)pComponent->m_dwData;
	PGUID guidPartyMember = c_PlayerGetPartyMemberGUID( nMemberIndex );
	if (guidPartyMember != INVALID_GUID)
	{
		bAllowed = TRUE;
	}
	
	pLabel->m_bDisabled = !bAllowed;
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);	
	
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickKick( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled && 
		UIComponentGetActive(pLabel) && 
		UIComponentCheckBounds(pLabel))
	{
		UI_COMPONENT *pMenu = pComponent->m_pParent;
		if (pMenu)
		{
			InputSetInteracted( );
			int nMemberIndex = (int)pMenu->m_dwData;
			PGUID guidPartyMember = c_PlayerGetPartyMemberGUID( nMemberIndex );
			if (guidPartyMember != INVALID_GUID)
			{
				GAME *pGame = AppGetCltGame();
				GAMECLIENT* pClient = UnitGetClient( GameGetControlUnit( pGame ) );
				if (c_PlayerGetPartyId() == INVALID_CHANNELID)
				{
					ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"You are not currently in a party.");
				}
				else if (c_PlayerGetPartyLeader() != gApp.m_characterGuid)
				{
					ConsoleString(pGame, pClient, CONSOLE_SYSTEM_COLOR, L"Only the leader of a party may remove party members.");
				}
				else
				{
					CHAT_REQUEST_MSG_REMOVE_PARTY_MEMBER removeMsg;
					removeMsg.MemberToRemove = guidPartyMember;
					c_ChatNetSendMessage(&removeMsg, USER_REQUEST_REMOVE_PARTY_MEMBER);
				}
			}
		}		
		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnPostVisibleKick( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );
	BOOL bAllowed = FALSE;
	
	int nMemberIndex = (int)pComponent->m_dwData;
	PGUID guidPartyMember = c_PlayerGetPartyMemberGUID( nMemberIndex );
	if (c_PlayerGetPartyId() != INVALID_CHANNELID &&
		c_PlayerGetPartyLeader() == gApp.m_characterGuid &&
		guidPartyMember != INVALID_GUID)
	{
		bAllowed = TRUE;
	}
	
	pLabel->m_bDisabled = !bAllowed;
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);	
	
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickLeave( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled && 
		UIComponentGetActive(pLabel) && 
		UIComponentCheckBounds(pLabel))
	{
		UI_COMPONENT *pMenu = pComponent->m_pParent;
		if (pMenu)
		{
			InputSetInteracted( );
			if (c_PlayerGetPartyId() == INVALID_CHANNELID)
			{
				UIAddChatLine(
					CHAT_TYPE_SERVER,
					ChatGetTypeColor(CHAT_TYPE_SERVER),
					GlobalStringGet( GS_PARTY_NOT_IN_A_PARTY ) );
			}
			else
			{
				CHAT_REQUEST_MSG_LEAVE_PARTY leaveMsg;
				c_ChatNetSendMessage(&leaveMsg, USER_REQUEST_LEAVE_PARTY);
			}
		}		
		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickDuelInvite( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled && 
		UIComponentGetActive(pLabel) && 
		UIComponentCheckBounds(pLabel))
	{
		InputSetInteracted( );
		UNIT *pPartyMember = UIComponentGetFocusUnit( pComponent );
		c_DuelInviteAttempt(pPartyMember);

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPetPartyMemberOnPaint( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UIComponentRemoveAllElements(pComponent);

	UIComponentAddFrame(pComponent);

	if (UIComponentIsPanel(pComponent))
	{
		UI_PANELEX *pPanel = UICastToPanel(pComponent);

		if (pPanel->m_pHighlightFrame &&
			UIComponentCheckBounds(pComponent) && 
			nMessage != UIMSG_MOUSELEAVE &&
			UICursorGetActive())
		{
			// draw a highlight
			UIComponentAddElement(pComponent, UIComponentGetTexture(pComponent), pPanel->m_pHighlightFrame, UI_POSITION(), pComponent->m_dwColor);
		}

	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPetOnClickDismiss( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	if (!pLabel->m_bDisabled && 
		UIComponentGetActive(pLabel) && 
		UIComponentCheckBounds(pLabel))
	{
		UNIT *pPet = UIComponentGetFocusUnit( pComponent );
		if (pPet && PetIsPet( pPet) )
		{
			MSG_CCMD_KILLPET msg;  
			msg.id = UnitGetId(pPet); 

			c_SendMessage(CCMD_KILLPET, &msg);		
		}

		UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

		return UIMSG_RET_HANDLED_END_PROCESS;  
	}

	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnClickInventory( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	if( AppIsHellgate() )
	{
		UI_LABELEX *pLabel = UICastToLabel( pComponent );

		if (!pLabel->m_bDisabled && 
			UIComponentGetActive(pLabel) && 
			UIComponentCheckBounds(pLabel))
		{
			UNIT *pPartyMember = UIComponentGetFocusUnit( pComponent );
			if (pPartyMember)
			{
				if (PetIsPet( pPartyMember ))
				{
					UIShowPetInventoryScreen( pPartyMember );
				}
			}

			UIComponentHandleUIMessage(pComponent->m_pParent, UIMSG_INACTIVATE, 0, 0);

			return UIMSG_RET_HANDLED_END_PROCESS;  
		}
	}
	else
	{
		if( UIComponentGetActive(pComponent) && 
			UIComponentCheckBounds(pComponent))
		{
			InputSetInteracted( );
			UNIT *pPartyMember = UIComponentGetFocusUnit( pComponent );
			if (pPartyMember)
			{
				if (PetIsPet( pPartyMember ) )
				{

					UNITID idCursorUnit = UIGetCursorUnit();
					UNIT* unitCursor = ( idCursorUnit != INVALID_ID ) ? UnitGetById(AppGetCltGame(), idCursorUnit) : NULL;
					if (unitCursor &&
						UnitIsUsable( UIGetControlUnit(), unitCursor ) &&
						UnitIsA( unitCursor, UNITTYPE_POTION ) )
					{
						c_ItemUseOn(unitCursor, pPartyMember);
						UISetCursorUnit(INVALID_ID, FALSE);
					}
					else if( UnitIsA( pPartyMember, UNITTYPE_HIRELING ) )
					{
						UIShowPetInventoryScreen( pPartyMember );
					}


				}
			}
			return UIMSG_RET_HANDLED_END_PROCESS;  
		}
	}
	
	return UIMSG_RET_NOT_HANDLED;  
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPartyMemberOnPostVisibleInventory( 
	UI_COMPONENT *pComponent,
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );
	BOOL bAllowed = FALSE;
	UNIT *pUnit = UIComponentGetFocusUnit( pComponent );
	if (pUnit)
	{
		if (PetIsPet( pUnit ) &&
			UnitHasInventory(pUnit))
		{
			bAllowed = TRUE;
		}
	}

	pLabel->m_bDisabled = !bAllowed;
	UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);	
	
	return UIMSG_RET_HANDLED_END_PROCESS;  // input used

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_POSITION UIComponentGetPosRelativeTo(
	UI_COMPONENT *pChild,
	UI_COMPONENT *pGreaterRelative)
{
	if (!pChild)
		return UI_POSITION();

	if (!pGreaterRelative ||
		pGreaterRelative == pChild->m_pParent)
		return pChild->m_Position;

	ASSERT_RETVAL(UIComponentIsParentOf(pGreaterRelative, pChild), UI_POSITION());

	return pChild->m_Position + UIComponentGetPosRelativeTo(pChild->m_pParent, pGreaterRelative);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPanelResizeScrollbar( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// first find the scrollbar for this component (it should be a sibling).
	UI_COMPONENT *pSibling = component;
	while (pSibling->m_pPrevSibling)
	{
		pSibling = pSibling->m_pPrevSibling;
	}

	UI_SCROLLBAR *pScrollBar = NULL;
	while(pSibling && !pScrollBar)
	{
		if (pSibling->m_eComponentType == UITYPE_SCROLLBAR)
		{
			pScrollBar = UICastToScrollBar(pSibling);
		}
		pSibling = pSibling->m_pNextSibling;
	}

	if (pScrollBar)
	{
		float fLowestPoint = 0.0f;
		UIComponentGetLowestPoint(component, fLowestPoint, 0.0f, 0, 3, TRUE);
		float fExtraLength = fLowestPoint - component->m_fHeight;
		if (fExtraLength <= 0.0f)
		{
			pScrollBar->m_fMax = 0.0f;
			UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
			UIComponentSetVisible(pScrollBar, FALSE);
		}
		else
		{
			pScrollBar->m_fMax = fExtraLength;

			if (wParam != INVALID_ID)	// this should be the id of the component that passed the message along
			{
				UI_COMPONENT *pChild = UIComponentGetById((int)wParam);
				if (pChild && pChild->m_pParent)
				{
					//make sure the panel is fully visible
					//float fTop = pChild->m_Position.m_fY + pChild->m_pParent->m_Position.m_fY;
					float fTop = UIComponentGetPosRelativeTo(pChild, component).m_fY;
					if (fTop - pScrollBar->m_ScrollPos.m_fY < 0.0f)
					{
						pScrollBar->m_ScrollPos.m_fY = fTop;
					}
					else if ((fTop - pScrollBar->m_ScrollPos.m_fY) + pChild->m_fHeight > component->m_fHeight)
					{
						pScrollBar->m_ScrollPos.m_fY = fTop - (component->m_fHeight - pChild->m_fHeight);
					}
				}
			}

			UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
			UIComponentSetActive(pScrollBar, TRUE);
		}
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIQuestsPanelResizeScrollbar( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// first find the scrollbar for this component (it should be a sibling).
	UI_COMPONENT *pSibling = component;
	while (pSibling->m_pPrevSibling)
	{
		pSibling = pSibling->m_pPrevSibling;
	}

	float fLowestPoint = 0.0f;
	UIComponentGetLowestPoint(component, fLowestPoint, 0.0f, 0, 1, TRUE);

	UI_SCROLLBAR *pScrollBar = NULL;
	while(pSibling && !pScrollBar)
	{
		if (pSibling->m_eComponentType == UITYPE_SCROLLBAR)
		{
			pScrollBar = UICastToScrollBar(pSibling);
		}
		pSibling = pSibling->m_pNextSibling;
	}

	if (pScrollBar)
	{
		float fExtraLength = fLowestPoint - component->m_fHeight;
		if (fExtraLength <= 0.0f)
		{
			pScrollBar->m_fMax = 0.0f;
			UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
			UIComponentSetVisible(pScrollBar, FALSE);

			// here's the special part
			component->m_Position.m_fY = component->m_fHeight - fLowestPoint;
		}
		else
		{
			pScrollBar->m_fMax = fExtraLength;

			if (wParam != INVALID_ID)	// this should be the id of the component that passed the message along
			{
				UI_COMPONENT *pChild = UIComponentGetById((int)wParam);
				if (pChild && pChild->m_pParent)
				{
					//make sure the panel is fully visible
					float fTop = pChild->m_Position.m_fY + pChild->m_pParent->m_Position.m_fY;
					if (fTop - pScrollBar->m_ScrollPos.m_fY < 0.0f)
					{
						pScrollBar->m_ScrollPos.m_fY = fTop;
					}
					else if ((fTop - pScrollBar->m_ScrollPos.m_fY) + pChild->m_fHeight > component->m_fHeight)
					{
						pScrollBar->m_ScrollPos.m_fY = fTop - (component->m_fHeight - pChild->m_fHeight);
					}
				}
			}

			UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
			UIComponentSetActive(pScrollBar, TRUE);
			component->m_Position.m_fY = 0.0f;
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UI_SCROLLBAR * sGetSiblingScrollbar(
	UI_COMPONENT * component)
{
	UI_COMPONENT *pSibling = component;
	while (pSibling->m_pPrevSibling)
	{
		pSibling = pSibling->m_pPrevSibling;
	}

	UI_SCROLLBAR *pScrollBar = NULL;
	while(pSibling && !pScrollBar)
	{
		if (pSibling->m_eComponentType == UITYPE_SCROLLBAR)
		{
			pScrollBar = UICastToScrollBar(pSibling);
		}
		pSibling = pSibling->m_pNextSibling;
	}

	return pScrollBar;
}


UI_MSG_RETVAL UIPanelBeginMouseDragScroll( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component)
		&& UIComponentCheckBounds(component))
	{
		UI_PANELEX *pPanel = UICastToPanel(component);
		pPanel->m_bMouseDown = TRUE;
		UIGetCursorPosition(&pPanel->m_posMouseDown.m_fX, &pPanel->m_posMouseDown.m_fY);

		if (!sGetSiblingScrollbar(component))
		{
			pPanel->m_posMouseDown += pPanel->m_ScrollPos;
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPanelMouseDragScroll( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_PANELEX *pPanel = UICastToPanel(component);

	if (!UICursorGetActive() ||
		(UIGetMouseOverrideComponent() != NULL &&
		UIGetMouseOverrideComponent() != component) ||
		!UIComponentGetActive(component))
	{
		pPanel->m_bMouseDown = FALSE;		
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!pPanel->m_bMouseDown)
		return UIMSG_RET_NOT_HANDLED;

	// first find the scrollbar for this component (it should be a sibling).
	UI_SCROLLBAR *pScrollBar = sGetSiblingScrollbar(component);

	UI_POSITION posComponent;
	UI_POSITION posMouse;
	UIGetCursorPosition(&posMouse.m_fX, &posMouse.m_fY);
	float fDeltaX = pPanel->m_posMouseDown.m_fX - posMouse.m_fX;
	float fDeltaY = pPanel->m_posMouseDown.m_fY - posMouse.m_fY;

	if (pScrollBar)
	{
		posComponent = UIComponentGetAbsoluteLogPosition(component);

		if (pScrollBar->m_nOrientation == UIORIENT_LEFT || 
			pScrollBar->m_nOrientation == UIORIENT_RIGHT)
		{
			float fNewScrollVal = pScrollBar->m_ScrollPos.m_fY + fDeltaX;
			if (fNewScrollVal >= pScrollBar->m_fMin &&
				fNewScrollVal <= pScrollBar->m_fMax)
			{
				pScrollBar->m_ScrollPos.m_fY = fNewScrollVal;
				pPanel->m_posMouseDown.m_fX = posMouse.m_fX;
			}
		}
		else
		{
			float fNewScrollVal = pScrollBar->m_ScrollPos.m_fY + fDeltaY;
			if (fNewScrollVal >= pScrollBar->m_fMin &&
				fNewScrollVal <= pScrollBar->m_fMax)
			{
				pScrollBar->m_ScrollPos.m_fY = fNewScrollVal;
				pPanel->m_posMouseDown.m_fY = posMouse.m_fY;
			}
		}

		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
		return UIMSG_RET_HANDLED;
	}

	pPanel->m_ScrollPos.m_fX = PIN(fDeltaX, pPanel->m_ScrollMin.m_fX, pPanel->m_ScrollMax.m_fX);
	pPanel->m_ScrollPos.m_fY = PIN(fDeltaY, pPanel->m_ScrollMin.m_fY, pPanel->m_ScrollMax.m_fY);
	UISetNeedToRender(pPanel);
	return UIMSG_RET_HANDLED;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPanelEndMouseDragScroll( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_PANELEX *pPanel = UICastToPanel(component);
	pPanel->m_bMouseDown = FALSE;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UISetCharsheetScrollbar( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	// now we need to reset the scroll size
	UI_COMPONENT *pCharSheet = UIComponentGetByEnum(UICOMP_CHARSHEET);
	ASSERT_RETVAL(pCharSheet, UIMSG_RET_HANDLED_END_PROCESS);
	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(UIComponentFindChildByName(pCharSheet, "char sheet scrollbar"));
	ASSERT_RETVAL(pScrollBar, UIMSG_RET_HANDLED_END_PROCESS);
	UI_COMPONENT *pPanel = UIComponentFindChildByName(pCharSheet, "character stats");
	ASSERT_RETVAL(pPanel, UIMSG_RET_HANDLED_END_PROCESS);

	float fLowestPoint = 0.0f;
	UIComponentGetLowestPoint(pPanel, fLowestPoint, 0.0f, 0, 1);
	float fExtraLength = fLowestPoint - pPanel->m_fHeight;
	if (fExtraLength <= 0.0f)
	{
		pScrollBar->m_fMax = 0.0f;
		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
		UIComponentSetVisible(pScrollBar, FALSE);
	}
	else
	{
		pScrollBar->m_fMax = fExtraLength;
		UIComponentHandleUIMessage(pScrollBar, UIMSG_SCROLL, 0, 0);
		UIComponentSetActive(pScrollBar, TRUE);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWindowshadeButtonOnClick( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_HANDLED);

	if (ResultIsHandled(UIButtonOnButtonUp(component, msg, wParam, lParam)))
	{
		UI_BUTTONEX* button = UICastToButton(component);
		ASSERT_RETVAL(button->m_pParent, UIMSG_RET_HANDLED);
		if (button->m_pNextSibling)
		{
			if (button->m_dwPushstate & UI_BUTTONSTATE_DOWN)
			{
				UIComponentWindowShadeOpen(button->m_pNextSibling);
			}
			else
			{
				UIComponentWindowShadeClosed(button->m_pNextSibling);
			}
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIWindowshadeAndSlideButtonOnClick( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_HANDLED);

	UI_MSG_RETVAL eRetval = UIMSG_RET_NOT_HANDLED;
	if (msg == UIMSG_LBUTTONCLICK || msg == UIMSG_LBUTTONUP)
	{
		eRetval = UIButtonOnButtonUp(component, msg, wParam, lParam);
	}
	if (msg == UIMSG_LBUTTONDOWN)
	{
		eRetval = UIButtonOnButtonDown(component, msg, wParam, lParam);
	}

	if (ResultIsHandled(eRetval))
	{
		UI_BUTTONEX* button = UICastToButton(component);
		ASSERT_RETVAL(button->m_pParent, UIMSG_RET_HANDLED);
		if (button->m_pNextSibling)
		{
			float fStartY = button->m_pParent->m_Position.m_fY;
			if (button->m_dwPushstate & UI_BUTTONSTATE_DOWN)
			{
				fStartY += button->m_pNextSibling->m_ActivePos.m_fY + button->m_pNextSibling->m_fHeight;
				UIComponentWindowShadeOpen(button->m_pNextSibling);
			}
			else
			{
				fStartY += button->m_pNextSibling->m_InactivePos.m_fY + button->m_pNextSibling->m_fHeight;
				UIComponentWindowShadeClosed(button->m_pNextSibling);
			}

			UI_COMPONENT *pSlidingPanel = button->m_pParent->m_pNextSibling;
			while (pSlidingPanel)
			{
				fStartY += pSlidingPanel->m_fToolTipYOffset;
				UIComponentSetAnimTime(pSlidingPanel, pSlidingPanel->m_Position, UI_POSITION(pSlidingPanel->m_Position.m_fX, fStartY), button->m_pNextSibling->m_dwAnimDuration, FALSE);
				float fLowestPoint = 0.0f;
				fStartY += UIComponentGetLowestPoint(pSlidingPanel, fLowestPoint, 0.0f, 0, 10, TRUE);
				if (pSlidingPanel->m_tMainAnimInfo.m_dwAnimTicket != INVALID_ID)
				{
					CSchedulerCancelEvent(pSlidingPanel->m_tMainAnimInfo.m_dwAnimTicket);
				}
				//pSlidingPanel->m_tMainAnimInfo.m_dwAnimTicket = CSchedulerRegisterEventImm(UIComponentAnimate, CEVENT_DATA((DWORD_PTR)pSlidingPanel, (DWORD_PTR)-1, (DWORD_PTR)FALSE));
				UIComponentAnimate(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)pSlidingPanel, (DWORD_PTR)-1, (DWORD_PTR)FALSE), 0);
				pSlidingPanel = pSlidingPanel->m_pNextSibling;
			}
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMsgPassToParent( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (component->m_pParent)
	{
		return UIComponentHandleUIMessage(component->m_pParent, msg, wParam, lParam);
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIMsgPassToSibling( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (component->m_pNextSibling)
	{
		return UIComponentHandleUIMessage(component->m_pNextSibling, msg, wParam, lParam);
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIClickSiblingButton( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (component->m_pNextSibling &&
		component->m_pNextSibling->m_eComponentType == UITYPE_BUTTON)
	{
		if (UIComponentCheckBounds(component))
		{
			return UIComponentHandleUIMessage(component->m_pNextSibling, UIMSG_LBUTTONDOWN, 1, lParam);
		}
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILanguageIndicatorOnPaint( 
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	WCHAR szInd[16] = L"none";
	if (IMM_IsEnabled() &&
		(IMM_GetPrimaryLanguage() == LANG_CHINESE ||
		IMM_GetPrimaryLanguage() == LANG_JAPANESE ||
		IMM_GetPrimaryLanguage() == LANG_KOREAN)) {
		UIComponentSetVisible(component, TRUE);
		IMM_GetLangIndicator(szInd, arrsize(szInd));

		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
		int nFontSize = UIComponentGetFontSize(component, pFont);
		UIElementGetTextLogSize(pFont, nFontSize, 1.0f, component->m_bNoScaleFont, szInd, &component->m_fWidth, NULL, TRUE, TRUE);
	
	} else {
		UIComponentSetVisible(component, FALSE);
	}

	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETVAL(label, UIMSG_RET_NOT_HANDLED);
	UILabelSetText(label, szInd, TRUE);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelSetToFocusUnitName(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UNIT *pUnit = UIComponentGetFocusUnit(component);
	if (!pUnit)
	{
		UILabelSetText( component, L"" );
	}
	else
	{
		#define MAX_NAME_LEN (128)
		WCHAR uszUnitName[ MAX_NAME_LEN ];
		UnitGetName( pUnit, uszUnitName, arrsize(uszUnitName), 0 );
		UILabelSetText( component, uszUnitName );
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIColorPickerShow(
	UI_COMPONENT *pNextToComponent,
	DWORD *pdwColorsIn,
	PFN_COLORPICKER_CALLBACK_FUNC pfnCallback,
	int nSelected)
{
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_COLOR_PICKER);
	ASSERT_RETURN(pComp);

	UI_COLORPICKER *pPicker = UICastToColorPicker(pComp);

	if (pPicker->m_pfnCallback)
	{
		DWORD dwColorChosen = 0;
		if (pPicker->m_nSelectedIndex >=0 && pPicker->m_nSelectedIndex < pPicker->m_nGridCols * pPicker->m_nGridRows)
		{
			dwColorChosen = pPicker->m_pdwColors[pPicker->m_nSelectedIndex];
		}
		pPicker->m_pfnCallback( dwColorChosen, pPicker->m_nOriginalIndex, 0, pPicker->m_dwCallbackData);
		pPicker->m_pfnCallback = NULL;
		pPicker->m_dwCallbackData = 0;
	}

	pPicker->m_pfnCallback = pfnCallback;

	pPicker->m_nSelectedIndex = pPicker->m_nOriginalIndex = nSelected;

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
			*pdwColor++ = *pdwColorsIn;
		}
	}

	UIComponentHandleUIMessage(pPicker, UIMSG_PAINT, 0, 0);
	UIComponentActivate(pPicker, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIColorPickerOnBtn(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetActive(component))
		return UIMSG_RET_NOT_HANDLED;

	if (!UIComponentCheckBounds(component))
	{
		// close the dialog and accept the choice
		UI_COLORPICKER *pPicker = UICastToColorPicker(component);

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
		
			UIComponentActivate(pPicker, FALSE);
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIColorPickerOnLClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	UI_COLORPICKER *pPicker = UICastToColorPicker(component);

	float fCursX, fCursY;
	UIGetCursorPosition(&fCursX, &fCursY);

	UI_POSITION posComp = UIComponentGetAbsoluteLogPosition(component);
	fCursX -= posComp.m_fX;
	fCursY -= posComp.m_fY;
	fCursX -= pPicker->m_fGridOffsetX;
	fCursY -= pPicker->m_fGridOffsetY;

	int col = (int)fCursX / (int)((pPicker->m_fCellSize + pPicker->m_fCellSpacing) * pPicker->m_fXmlWidthRatio);
	int row = (int)fCursY / (int)((pPicker->m_fCellSize + pPicker->m_fCellSpacing) * pPicker->m_fXmlHeightRatio);

	if (col >= 0 && col < pPicker->m_nGridCols &&
		row >= 0 && row < pPicker->m_nGridRows)
	{
		pPicker->m_nSelectedIndex = (row * pPicker->m_nGridCols) + col;

		if (pPicker->m_pfnCallback)
		{
			DWORD dwColorChosen = 0;
			dwColorChosen = pPicker->m_pdwColors[pPicker->m_nSelectedIndex];
			pPicker->m_pfnCallback( dwColorChosen, pPicker->m_nSelectedIndex, 2, pPicker->m_dwCallbackData);
		}
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIColorPickerColorsOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentOnPaint(component, msg, wParam, lParam);

	UI_COLORPICKER *pPicker = UICastToColorPicker(component);
	UI_POSITION pos(pPicker->m_fGridOffsetX, pPicker->m_fGridOffsetY);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);

	for (int i=0; i<pPicker->m_nGridCols; i++)
	{
		for (int j=0; j<pPicker->m_nGridRows; j++)
		{
			int nIndex = (j * pPicker->m_nGridCols) + i;
			DWORD dwColor = pPicker->m_pdwColors[nIndex];

			UIComponentAddElement(pPicker, pTexture, pPicker->m_pCellFrame, pos, dwColor);

			if (nIndex == pPicker->m_nSelectedIndex)
				UIComponentAddElement(pPicker, pTexture, pPicker->m_pCellDownFrame, pos, GFXCOLOR_WHITE);
			else
				UIComponentAddElement(pPicker, pTexture, pPicker->m_pCellUpFrame, pos, GFXCOLOR_WHITE);
			//UIComponentAddPrimitiveElement(pPicker, UIPRIM_BOX, 0, UI_RECT(pos.m_fX, pos.m_fY, pos.m_fX + pPicker->m_fCellSize, pos.m_fY + pPicker->m_fCellSize), NULL, NULL, dwColor);

			pos.m_fY += (pPicker->m_fCellSize + pPicker->m_fCellSpacing) * pPicker->m_fXmlHeightRatio;
		}
		pos.m_fY = pPicker->m_fGridOffsetY;
		pos.m_fX += (pPicker->m_fCellSize + pPicker->m_fCellSpacing) * pPicker->m_fXmlWidthRatio;
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIColorPickerAcceptOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent && UIComponentIsColorPicker(component->m_pParent), UIMSG_RET_NOT_HANDLED);
	UI_COLORPICKER *pPicker = UICastToColorPicker(component->m_pParent);

	if (pPicker->m_pfnCallback)
	{
		DWORD dwColorChosen = 0;
		if (pPicker->m_nSelectedIndex >=0 && pPicker->m_nSelectedIndex < pPicker->m_nGridCols * pPicker->m_nGridRows)
		{
			dwColorChosen = pPicker->m_pdwColors[pPicker->m_nSelectedIndex];
		}
		pPicker->m_pfnCallback( dwColorChosen, pPicker->m_nSelectedIndex, 1, pPicker->m_dwCallbackData);
		pPicker->m_pfnCallback = NULL;
		pPicker->m_dwCallbackData = 0;
	}

	UIComponentActivate(pPicker, FALSE);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIColorPickerCancelOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(component->m_pParent && UIComponentIsColorPicker(component->m_pParent), UIMSG_RET_NOT_HANDLED);
	UI_COLORPICKER *pPicker = UICastToColorPicker(component->m_pParent);

	if (pPicker->m_pfnCallback)
	{
		DWORD dwColorChosen = 0;
		if (pPicker->m_nSelectedIndex >=0 && pPicker->m_nSelectedIndex < pPicker->m_nGridCols * pPicker->m_nGridRows)
		{
			dwColorChosen = pPicker->m_pdwColors[pPicker->m_nSelectedIndex];
		}
		pPicker->m_pfnCallback( dwColorChosen, pPicker->m_nOriginalIndex, 0, pPicker->m_dwCallbackData);
		pPicker->m_pfnCallback = NULL;
		pPicker->m_dwCallbackData = 0;
	}

	UIComponentActivate(pPicker, FALSE);

	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIStashHolderOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pExtra = UIComponentFindChildByName(component, "extra stash grid");
	if (pExtra)
	{
		UIComponentSetVisible(pExtra, FALSE);
		UI_INVGRID *pGrid = UICastToInvGrid(pExtra);
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
		{
			if (InventoryLocPlayerCanTake(pPlayer, NULL, pGrid->m_nInvLocation) &&
				InventoryLocPlayerCanPut(pPlayer, NULL, pGrid->m_nInvLocation) )
			{
				UIComponentSetActive(pGrid, TRUE);
			}
			else if (UnitInventoryLocationIterate(pPlayer, pGrid->m_nInvLocation, NULL) != NULL)
			{
				// there's an item here that the player can't access
				UIComponentSetVisible(pGrid, TRUE);		// make it visible but inactive
			}
		}
	}

	float fLowest = 0.0f;
	component->m_fScrollVertMax = UIComponentGetLowestPoint(component, fLowest, 0.0f, 0, 10, TRUE) - component->m_fHeight;

	UIComponentHandleUIMessage(component->m_pParent, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadLabel(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETFALSE(label);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(component);
	REF(font);
	//ASSERT_RETZERO(font);

	//XML_LOADINT("align", label->m_nAlignment, UIALIGN_TOPLEFT);
	XML_LOADENUM("align", label->m_nAlignment, pAlignEnumTbl, component->m_bReference ? label->m_nAlignment : UIALIGN_TOPLEFT);
	XML_LOADENUM("marquee", label->m_eMarqueeMode, pMarqueeEnumTbl, component->m_bReference ? label->m_eMarqueeMode : MARQUEE_NONE);
	if (label->m_eMarqueeMode != MARQUEE_NONE)
	{
		label->m_bNeedsRepaintOnVisible = TRUE;
	}
	XML_GETFLOATATTRIB("marquee", "increment", label->m_fMarqueeIncrement, component->m_bReference ? label->m_fMarqueeIncrement : 0.1f);
	XML_GETINTATTRIB("marquee", "enddelay", label->m_nMarqueeEndDelay, component->m_bReference ? label->m_nMarqueeEndDelay : 3000);
	XML_GETINTATTRIB("marquee", "startdelay", label->m_nMarqueeStartDelay, component->m_bReference ? label->m_nMarqueeStartDelay : 3000);
	label->m_dwMarqueeAnimTicket = INVALID_ID;

	XML_LOADFLOAT("frameoffsetx", label->m_fFrameOffsetX, component->m_bReference ? label->m_fFrameOffsetX : 0.0f);
	XML_LOADFLOAT("frameoffsety", label->m_fFrameOffsetY, component->m_bReference ? label->m_fFrameOffsetY : 0.0f);

	// These should probably be an enum
	XML_LOADBOOL("autosize", label->m_bAutosize, component->m_bReference ? label->m_bAutosize : FALSE);
	XML_LOADBOOL("autosizeverticalonly", label->m_bAutosizeVerticalOnly, component->m_bReference ? label->m_bAutosizeVerticalOnly : FALSE);
	XML_LOADBOOL("autosizehorizontalonly", label->m_bAutosizeHorizontalOnly, component->m_bReference ? label->m_bAutosizeHorizontalOnly : FALSE);
	ASSERT_RETFALSE(!(label->m_bAutosizeVerticalOnly && label->m_bAutosizeHorizontalOnly));
	XML_LOADBOOL("autosizefont", label->m_bAutosizeFont, component->m_bReference ? label->m_bAutosizeFont: FALSE);
	
	XML_LOADBOOL("dropshadow", label->m_bDrawDropshadow, component->m_bReference ? label->m_bDrawDropshadow : TRUE);
	UIXMLLoadColor(xml, label, "dropshadow", label->m_dwDropshadowColor, component->m_bReference ? label->m_dwDropshadowColor : GFXCOLOR_BLACK);
	
	XML_LOADBOOL("bkgdrect", label->m_bBkgdRect, component->m_bReference ? label->m_bBkgdRect : FALSE);
	UIXMLLoadColor(xml, label, "bkgdrect", label->m_dwBkgdRectColor, component->m_bReference ? label->m_dwBkgdRectColor : GFXCOLOR_GRAY);
	
	XML_LOADBOOL("slowreveal", label->m_bSlowReveal, component->m_bReference ? label->m_bSlowReveal : FALSE);	
	XML_LOADBOOL("wordwrap", label->m_bWordWrap, component->m_bReference ? label->m_bWordWrap : FALSE);
	XML_LOADBOOL("forcewrap", label->m_bForceWrap, component->m_bReference ? label->m_bForceWrap : FALSE);
	if (xml.HasChildAttrib("useparentmaxwidth"))
	{
		label->m_bUseParentMaxWidth = TRUE;
	}

	//XML_LOADFLOAT("maxwidth", label->m_fMaxWidth, -1.0f);
	label->m_fMaxWidth = label->m_fWidth;
	xml.ResetChildPos();
	if (xml.FindChildElem("maxwidth"))
	{
		label->m_fMaxWidth = (float)PStrToInt(xml.GetChildData());
		if (xml.HasChildAttrib("scale") && label->m_bNoScale)
		{
			label->m_fMaxWidth *= UIGetLogToScreenRatioX();
		}
		label->m_fMaxWidth *= tUIXml.m_fXmlWidthRatio;
	}

	label->m_fMaxHeight = -1.0f;
	xml.ResetChildPos();
	if (xml.FindChildElem("maxheight"))
	{
		label->m_fMaxHeight = (float)PStrToInt(xml.GetChildData());
		if (xml.HasChildAttrib("scale") && label->m_bNoScale)
		{
			label->m_fMaxHeight *= UIGetLogToScreenRatioX();
		}
		label->m_fMaxHeight *= tUIXml.m_fXmlHeightRatio;
	}


	char str[256] = "";
	XML_LOADSTRING("string", str, "", 256);
	if (str[0] != 0)
	{
		UILabelSetTextByStringKey(label, str);
		label->m_nStringIndex = StringTableCommonGetStringIndexByKey(LanguageGetCurrent(), str);
	}
	else
	{
		const int MAX_STRING = 4096;
		char strBig[MAX_STRING];
		memclear(strBig, MAX_STRING * sizeof(char));
		XML_LOADSTRING("text", strBig, "", MAX_STRING);
		if (strBig[0] != 0)
		{
			WCHAR wstr[MAX_STRING] = L"";
			PStrCvt(wstr, strBig, MAX_STRING);
#if ISVERSION(DEVELOPMENT)
			if (!AppIsTugboat())
            {
	            // TRAVIS: TODO LOCALIZATION CHECK FIX LATER
	            PStrCat(wstr, L"[NOT LOCALIZED]", MAX_STRING);
            }
#endif
			UILabelSetText(label, wstr);
		}
		label->m_nStringIndex = -1;
	}

	XML_LOADINT("menuorder", label->m_nMenuOrder, -1);
	label->m_bSelected = FALSE;
	XML_LOADBOOL("disabled", label->m_bDisabled, FALSE);

	UIXMLLoadColor(xml, label, "selected", label->m_dwSelectedColor, 0xfff78e14);
	UIXMLLoadColor(xml, label, "disabled", label->m_dwDisabledColor, 0x77777777);

	UI_TEXTURE_FRAME *pScrollbarFrame = NULL;
	XML_LOADFRAME("scrollbarframe", UIComponentGetTexture(component), pScrollbarFrame, "");
	if (pScrollbarFrame)
	{
		UI_TEXTURE_FRAME *pThumbpadFrame = NULL;
		XML_LOADFRAME("thumbpadframe",	UIComponentGetTexture(component), pThumbpadFrame, "");

		XML_LOADBOOL("hidescrollbar", label->m_bHideInactiveScrollbar, component->m_bReference ? label->m_bHideInactiveScrollbar : FALSE);


		UI_SCROLLBAR *pScrollbar = (UI_SCROLLBAR *) UIComponentCreateNew(component, UITYPE_SCROLLBAR, TRUE);
		ASSERT_RETFALSE(pScrollbar &&
			component->m_pFirstChild == pScrollbar);

		XML_GETBOOLATTRIB("scrollbarframe", "tile", pScrollbar->m_bTileFrame, FALSE);

		pScrollbar->m_nOrientation = UIORIENT_TOP;
		pScrollbar->m_bIndependentActivate = TRUE;
		pScrollbar->m_eState = UISTATE_INACTIVE;
		pScrollbar->m_bVisible = FALSE;
		pScrollbar->m_pFrame = pScrollbarFrame;
		pScrollbar->m_pThumbpadFrame = pThumbpadFrame;
		pScrollbar->m_fHeight = component->m_fHeight;
		pScrollbar->m_fWidth = pScrollbarFrame->m_fWidth;
		pScrollbar->m_bSendPaintToParent = TRUE;
		pScrollbar->m_pScrollControl = component;

		XML_LOADFLOAT("scrollextraspacing", label->m_fScrollbarExtraSpacing, 0.0f);
		pScrollbar->m_Position.m_fX = component->m_fWidth + label->m_fScrollbarExtraSpacing;
		pScrollbar->m_fWheelScrollIncrement = (float)UIComponentGetFontSize(component);

		label->m_pScrollbar = pScrollbar;
		sUILabelAdjustScrollbar( label );

	}

	XML_LOADBOOL("play_unit_modes", label->m_bPlayUnitModes, FALSE);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentFreeLabel(
	UI_COMPONENT* component)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETURN(label);

	if (label->m_pUnitmodes)
	{
		FREE(NULL, label->m_pUnitmodes);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadScreen(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_SCREENEX* screen = UICastToScreen(component);
	ASSERT_RETFALSE(screen);

	XML_LOADBOOL("ignoresmouse", component->m_bIgnoresMouse, TRUE);

	UIResizeWindow();

	if (component->m_pTexture)
	{
		UI_TEXTURE_FRAME* boxframe = (UI_TEXTURE_FRAME*)StrDictionaryFind(component->m_pTexture->m_pFrames, "textboxcolor");
		if (boxframe)
		{
			component->m_pTexture->m_pBoxFrame = boxframe;
		}
		else
		{
			UI_TEXTURE_FRAME* boxframe = (UI_TEXTURE_FRAME*)StrDictionaryFind(component->m_pTexture->m_pFrames, "BLANK BLACK SQUARE");
			if (boxframe)
			{
				component->m_pTexture->m_pBoxFrame = boxframe;
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * gszCursorStateFrameNames[NUM_UICURSOR_STATES] = 
{
	"arrowframe",				//	UICURSOR_STATE_POINTER
	"carryitemframe",			//	UICURSOR_STATE_ITEM
	"overitemframe",			//  UICURSOR_STATE_OVERITEM
	"skilldragframe",			//  UICURSOR_STATE_SKILLDRAG
	"waitframe",				//  UICURSOR_STATE_WAIT
	"identifyframe",			//	UICURSOR_STATE_IDENTIFY
	"skillpickitemframe",		//  UICURSOR_STATE_SKILL_PICK_ITEM
	"dismantleframe",			//  UICURSOR_STATE_DISMANTLE	
	"itempickitem",				//  UICURSOR_STATE_ITEM_PICK_ITEM
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadCursor(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	ASSERT_RETFALSE(component);
	UI_CURSOR* cursor = UICastToCursor(component);

	for (int i=0 ; i < NUM_UICURSOR_STATES; i++)
	{
		char szTex[256] = "";
		char szKeyname[256];
		PStrPrintf(szKeyname, arrsize(szKeyname), "%stexture", gszCursorStateFrameNames[i]);
		XML_LOADSTRING(szKeyname, szTex, "", 256);
		if (szTex[0])
			cursor->m_pCursorFrameTexture[i] = UILoadComponentTexture(component, szTex, FALSE);
		else
			cursor->m_pCursorFrameTexture[i] = cursor->m_pTexture;

		ASSERT_RETFALSE(cursor->m_pCursorFrameTexture[i]);
		if (gszCursorStateFrameNames[i])
		{
			XML_LOADFRAME(gszCursorStateFrameNames[i], cursor->m_pCursorFrameTexture[i], cursor->m_pCursorFrame[i], "");
		}
	}

	cursor->m_idUnit = INVALID_ID;
	cursor->m_idUseItem = INVALID_ID;
	cursor->m_nSkillID = INVALID_ID;
	cursor->m_nFromWeaponConfig = -1;
	cursor->m_nItemPickActivateSkillID = INVALID_ID;

	// initialize the item model info
	cursor->m_tItemAdjust.fScale = 1.5f;

	return TRUE;
}


STR_DICT pstrdictButtonStyles[] =
{
	{ "",				UIBUTTONSTYLE_PUSHBUTTON },
	{ "pushbutton",		UIBUTTONSTYLE_PUSHBUTTON },
	{ "checkbox",		UIBUTTONSTYLE_CHECKBOX },
	{ "radiobutton",	UIBUTTONSTYLE_RADIOBUTTON },

	{ NULL,				UIBUTTONSTYLE_PUSHBUTTON },
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadButton(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_BUTTONEX* button = UICastToButton(component);

	button->m_nAssocTab = -1;
	button->m_dwRepeatMS = 250;

	if (!component->m_bReference)
	{
		button->m_pLitFrame = NULL;
		button->m_pDownFrame = NULL;
	}

	UIX_TEXTURE* texture = UIComponentGetTexture(button);
	if (!texture)
	{
		return FALSE;
	}

	XML_LOADENUM("buttonstyle", button->m_eButtonStyle, pstrdictButtonStyles, component->m_bReference ? button->m_eButtonStyle : UIBUTTONSTYLE_PUSHBUTTON);
	XML_LOADINT("radioparentlevel", button->m_nRadioParentLevel, component->m_bReference ? button->m_nRadioParentLevel : 0);
	XML_LOADFRAME("litframe", texture, button->m_pLitFrame, "");
	XML_LOADBOOL("overlaylitframe", button->m_bOverlayLitFrame, component->m_bReference ? button->m_bOverlayLitFrame : FALSE);
	XML_LOADFRAME("downlitframe", texture, button->m_pDownLitFrame, "");
	XML_LOADFRAME("downframe", texture, button->m_pDownFrame, "");
	XML_LOADFRAME("inactiveframe", texture, button->m_pInactiveFrame, "");
	XML_LOADFRAME("inactivedownframe", texture, button->m_pInactiveDownFrame, "");

	XML_LOADFRAME("framemid",   	texture, button->m_pFrameMid, "");
	XML_LOADFRAME("frameleft",  	texture, button->m_pFrameLeft, "");
	XML_LOADFRAME("frameright", 	texture, button->m_pFrameRight, "");

	XML_LOADFRAME("litframemid",   	texture, button->m_pLitFrameMid, "");
	XML_LOADFRAME("litframeleft",  	texture, button->m_pLitFrameLeft, "");
	XML_LOADFRAME("litframeright", 	texture, button->m_pLitFrameRight, "");

	XML_LOADFRAME("downlitframemid",   	texture, button->m_pDownLitFrameMid, "");
	XML_LOADFRAME("downlitframeleft",  	texture, button->m_pDownLitFrameLeft, "");
	XML_LOADFRAME("downlitframeright", 	texture, button->m_pDownLitFrameRight, "");

	XML_LOADFRAME("downframemid",   texture, button->m_pDownFrameMid, "");
	XML_LOADFRAME("downframeleft",  texture, button->m_pDownFrameLeft, "");
	XML_LOADFRAME("downframeright", texture, button->m_pDownFrameRight, "");

	XML_LOADFRAME("inactiveframemid",   texture, button->m_pInactiveFrameMid, "");
	XML_LOADFRAME("inactiveframeleft",  texture, button->m_pInactiveFrameLeft, "");
	XML_LOADFRAME("inactiveframeright", texture, button->m_pInactiveFrameRight, "");

	button->m_dwPushstate = 0;
	BOOL bStartDown = FALSE;
	XML_LOADBOOL("startdown", bStartDown, FALSE);
	if (bStartDown)
	{
		button->m_dwPushstate |= UI_BUTTONSTATE_DOWN;
	}

	XML_LOADEXCELILINK("assocstat", button->m_nAssocStat, DATATABLE_STATS);
	XML_LOADINT("tab", button->m_nAssocTab, -1);
	XML_LOADINT("repeat", button->m_dwRepeatMS, 250);

	XML_LOADEXCELILINK("check_on_sound", button->m_nCheckOnSound, DATATABLE_SOUNDS);
	XML_LOADEXCELILINK("check_off_sound", button->m_nCheckOffSound, DATATABLE_SOUNDS);

	char szControl[256] = "";
	XML_LOADSTRING("scrollcontrol", szControl, "", 256);
	if (szControl[0])
	{
		if (PStrICmp(szControl, "parent") == 0)
		{
			button->m_pScrollControl = button->m_pParent;
		}	
		else
		{
			button->m_pScrollControl = UIComponentFindChildByName(button->m_pParent, szControl);
		}
		if (button->m_pScrollControl)
		{
			XML_GETFLOATATTRIB("scrollcontrol", "inc", button->m_fScrollIncrement, 0.0f);

			XML_LOADBOOL("hideonscrollextent", button->m_bHideOnScrollExtent, 0);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadBar(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_BAR* bar = UICastToBar(component);
	ASSERT_RETFALSE(bar);

	bar->m_codeCurValue = UIXmlLoadCode(xml, "cur", (bar->m_bReference ? bar->m_codeCurValue : NULL_CODE));
	bar->m_codeMaxValue = UIXmlLoadCode(xml, "max", (bar->m_bReference ? bar->m_codeMaxValue : NULL_CODE));
	bar->m_codeRegenValue = UIXmlLoadCode(xml, "regen", (bar->m_bReference ? bar->m_codeRegenValue : NULL_CODE));

	// Load the stat number for the regen as well
	//   (we *could* find this out on our own....)

	//XML_LOADEXCELILINK("regen", bar->m_nRegenStat, DATATABLE_STATS);
	XML_LOADEXCELILINK("increaseleft", bar->m_nIncreaseLeftStat, DATATABLE_STATS);

	//XML_LOADBOOL("showtempregenincrease", bar->m_bShowTempRegen, FALSE);

	//if (bar->m_nRegenStat == INVALID_ID)
	//{
	//	bar->m_bShowTempRegen = FALSE
	//}

	//if (bar->m_bShowTempRegen)
	//{
	//	GAME* game = AppGetCltGame();
	//	STATS_DATA* stats_data = (STATS_DATA*)ExcelGetData(game, DATATABLE_STATS, bar->m_nRegenStat);
	//	for (int i=0; i < stats_data->nFormulaAffectedByList; i++)
	//	{
	//		int nFormula = stats_data->pnFormulaAffectedByList[i];
	//		const STATS_FUNCTION* pStatsFunc = StatsFuncGetData(game, nFormula);

	//	}
	//}

	XML_LOADBOOL("endcap", bar->m_bEndCap, FALSE);
	XML_LOADFLOAT("slantwidth", bar->m_fSlantWidth, 0.0f);
	//XML_LOADINT("orientation", bar->m_nOrientation, UIORIENT_LEFT);
	XML_LOADENUM("orientation", bar->m_nOrientation, pOrientationEnumTbl, UIORIENT_LEFT);
	XML_LOADINT("blinkthreshold", bar->m_nBlinkThreshold, -1);
	bar->m_nOldValue = -1;
	bar->m_nOldMax = -1;

	XML_LOADINT("showincreaseduration", bar->m_nShowIncreaseDuration, 0);

	if (!bar->m_pBlinkData && (bar->m_nBlinkThreshold >= 0 || bar->m_nShowIncreaseDuration > 0))
	{
		bar->m_pBlinkData = (UI_BLINKDATA *)MALLOC(NULL, sizeof(UI_BLINKDATA));
		memclear(bar->m_pBlinkData, sizeof(UI_BLINKDATA));
		bar->m_pBlinkData->m_dwBlinkColor = GFXCOLOR_RED;
		bar->m_pBlinkData->m_dwBlinkAnimTicket = INVALID_ID;
	}

	UIXMLLoadColor(xml, bar, "alt", bar->m_dwAltColor, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, bar, "alt2", bar->m_dwAltColor2, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, bar, "alt3", bar->m_dwAltColor3, GFXCOLOR_WHITE);

	XML_LOADEXCELILINK("stat", bar->m_nStat, DATATABLE_STATS);

	XML_LOADBOOL("radial", bar->m_bRadial, FALSE);
	XML_LOADFLOAT("maxangle", bar->m_fMaxAngle, 0.0f);
	XML_LOADBOOL("ccw", bar->m_bCounterClockwise, FALSE);
	XML_LOADFLOAT("maskanglestart", bar->m_fMaskAngleStart, 0.0f);
	XML_LOADFLOAT("maskbaseangle", bar->m_fMaskBaseAngle, 0.0f);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadScrollBar(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_SCROLLBAR* pScrollBar = UICastToScrollBar(component);
	ASSERT_RETFALSE(pScrollBar);

	XML_LOADFLOAT("minvalue", pScrollBar->m_fMin, 0.0f);
	XML_LOADFLOAT("maxvalue", pScrollBar->m_fMax, 100.0f);
	pScrollBar->m_fMin *= g_UI.m_fUIScaler;
	pScrollBar->m_fMax *= g_UI.m_fUIScaler;
	XML_LOADENUM("orientation", pScrollBar->m_nOrientation, pOrientationEnumTbl, UIORIENT_LEFT);

	pScrollBar->m_fScrollVertMax = pScrollBar->m_fMax;
	
	UI_COMPONENT *pChild = pScrollBar->m_pFirstChild;
	while(pChild)
	{
		if (pChild->m_eComponentType == UITYPE_BAR)
		{
			pScrollBar->m_pBar = UICastToBar(pChild);
			pScrollBar->m_pBar->m_nValue = 0;
			pScrollBar->m_pBar->m_nMaxValue = (int)pScrollBar->m_fMax;
			pScrollBar->m_pBar->m_nOrientation = pScrollBar->m_nOrientation;
			break;
		}
		pChild = pChild->m_pNextSibling;
	}

	XML_LOADFRAME("thumbpadframe", UIComponentGetTexture(component), pScrollBar->m_pThumbpadFrame, "");

	char szControl[256] = "";
	XML_LOADSTRING("scrollcontrol", szControl, "", 256);
	if (szControl[0])
	{
		if (PStrICmp(szControl, "parent") == 0)
		{
			pScrollBar->m_pScrollControl = pScrollBar->m_pParent;
		}	
		else
		{
			pScrollBar->m_pScrollControl = UIComponentFindChildByName(pScrollBar->m_pParent, szControl);

		}
		if( pScrollBar->m_pScrollControl )
		{
			if( UIComponentIsTextBox( pScrollBar->m_pScrollControl ) )
			{
				UI_TEXTBOX* pTextBox  = UICastToTextBox( pScrollBar->m_pScrollControl );
				pTextBox->m_pScrollbar = pScrollBar;
				pScrollBar->m_bSendPaintToParent = TRUE;
			}
			else if( UIComponentIsLabel( pScrollBar->m_pScrollControl ) )
			{
				UI_LABELEX* pLabel  = UICastToLabel( pScrollBar->m_pScrollControl );
				pLabel->m_pScrollbar = pScrollBar;
				pScrollBar->m_bSendPaintToParent = TRUE;

			}
		}
	}

	UIScrollBarOnScroll(component, UIMSG_SCROLL, 0, 0 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadTooltip(
	CMarkup &xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_TOOLTIP* pTooltip = UICastToTooltip(component);
	ASSERT_RETFALSE(pTooltip);

	//XML_LOADFLOAT("minwidth", pTooltip->m_fMinWidth, -1.0f );
	//BOOL bScaleWidth = 0;
	//XML_GETBOOLATTRIB("minwidth", "scale", bScaleWidth, 0);
	//pTooltip->m_fMinWidth *= UIGetLogToScreenRatioX(!bScaleWidth);	
	xml.ResetChildPos();
	if (xml.FindChildElem("minwidth"))
	{
		pTooltip->m_fMinWidth = (float)PStrToInt(xml.GetChildData());
		if (xml.HasChildAttrib("scale") && pTooltip->m_bNoScale)
		{
			pTooltip->m_fMinWidth *= UIGetLogToScreenRatioX();
		}
		pTooltip->m_fMinWidth *= tUIXml.m_fXmlWidthRatio;
	}
	else if (!pTooltip->m_bReference)
	{
		pTooltip->m_fMinWidth = -1.0f;
	}

	//XML_LOADFLOAT("maxwidth", pTooltip->m_fMaxWidth, -1.0f );
	//bScaleWidth = 0;
	//XML_GETBOOLATTRIB("maxwidth", "scale", bScaleWidth, 0);
	//pTooltip->m_fMaxWidth *= UIGetLogToScreenRatioX(!bScaleWidth);	
	xml.ResetChildPos();
	if (xml.FindChildElem("maxwidth"))
	{
		pTooltip->m_fMaxWidth = (float)PStrToInt(xml.GetChildData());
		if (xml.HasChildAttrib("scale") && pTooltip->m_bNoScale)
		{
			pTooltip->m_fMaxWidth *= UIGetLogToScreenRatioX();
		}
		pTooltip->m_fMaxWidth *= tUIXml.m_fXmlWidthRatio;
	}
	else if (!pTooltip->m_bReference)
	{
		pTooltip->m_fMaxWidth = -1.0f;
	}

	XML_LOADFLOAT("xoffset", pTooltip->m_fXOffset, 0.0f );
	XML_LOADFLOAT("yoffset", pTooltip->m_fYOffset, 0.0f );
	pTooltip->m_fXOffset *= tUIXml.m_fXmlWidthRatio;
	pTooltip->m_fYOffset *= tUIXml.m_fXmlHeightRatio;

	XML_LOADBOOL("autosize", pTooltip->m_bAutosize, TRUE);
	XML_LOADBOOL("center", pTooltip->m_bCentered, FALSE);

	// Change the default rendersection for tooltips
	XML_LOADENUM("rendersection", pTooltip->m_nRenderSection, pRenderSectionEnumTbl, RENDERSECTION_TOOLTIPS);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadConversationDlg(
	CMarkup &xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_CONVERSATION_DLG *pDialog = UICastToConversationDlg(component);
	pDialog->m_nQuest = INVALID_LINK;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadFlexBorder(
	CMarkup &xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_FLEXBORDER *pFlexBorder = UICastToFlexBorder(component);
	ASSERT(pFlexBorder);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(pTexture);

	XML_LOADFRAME("frameTL", pTexture, pFlexBorder->m_pFrameTL, "");
	XML_LOADFRAME("frameTM", pTexture, pFlexBorder->m_pFrameTM, "");
	XML_LOADFRAME("frameTR", pTexture, pFlexBorder->m_pFrameTR, "");
	XML_LOADFRAME("frameML", pTexture, pFlexBorder->m_pFrameML, "");
	XML_LOADFRAME("frameMM", pTexture, pFlexBorder->m_pFrameMM, "");
	XML_LOADFRAME("frameMR", pTexture, pFlexBorder->m_pFrameMR, "");
	XML_LOADFRAME("frameBL", pTexture, pFlexBorder->m_pFrameBL, "");
	XML_LOADFRAME("frameBM", pTexture, pFlexBorder->m_pFrameBM, "");
	XML_LOADFRAME("frameBR", pTexture, pFlexBorder->m_pFrameBR, "");

	XML_LOADBOOL("autosize", pFlexBorder->m_bAutosize, component->m_bReference ? pFlexBorder->m_bAutosize : TRUE);
	
	XML_LOADFLOAT("borderspace", pFlexBorder->m_fBorderSpace, component->m_bReference ? pFlexBorder->m_fBorderSpace : 0.0f);

	XML_LOADBOOL("stretchborders", pFlexBorder->m_bStretchBorders, component->m_bReference ? pFlexBorder->m_bStretchBorders : 0);

	XML_LOADFLOAT("borderscale", pFlexBorder->m_fBorderScale, component->m_bReference ? pFlexBorder->m_fBorderScale : 1.0f);

	if (pFlexBorder->m_bAutosize && pFlexBorder->m_pParent)
	{
		pFlexBorder->m_fWidth	= pFlexBorder->m_pParent->m_fWidth + (pFlexBorder->m_fBorderSpace * 2.0f); 
		pFlexBorder->m_fHeight	= pFlexBorder->m_pParent->m_fHeight + (pFlexBorder->m_fBorderSpace * 2.0f); 
		pFlexBorder->m_fZDelta	= pFlexBorder->m_pParent->m_fZDelta; 
	}
	pFlexBorder->m_bStretch = 0; 


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STR_DICT pFlexSibEnumTbl[] =
{
	{ "",			FLEX_SIB_NONE },
	{ "none",		FLEX_SIB_NONE },
	{ "prev",		FLEX_SIB_PREV },
	{ "next",		FLEX_SIB_NEXT },
	{ "nextany",	FLEX_SIB_NEXT_ANY },
	{ "prevany",	FLEX_SIB_PREV_ANY },

	{ NULL,		MARQUEE_NONE },
};

BOOL UIXmlLoadFlexLine(
	CMarkup &xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_FLEXLINE *pFlexLine = UICastToFlexLine(component);
	ASSERT(pFlexLine);

	XML_LOADBOOL("horizontal", pFlexLine->m_bHorizontal, 1);

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(pTexture);

	XML_LOADFRAME("lineframe",		pTexture, pFlexLine->m_pFrameLine, "");
	XML_LOADFRAME("startcapframe",	pTexture, pFlexLine->m_pFrameStartCap, "");
	XML_LOADFRAME("endcapframe",	pTexture, pFlexLine->m_pFrameEndCap, "");

	ASSERT_RETFALSE(pFlexLine->m_pFrameLine);

	XML_LOADBOOL("autosize", pFlexLine->m_bAutosize, TRUE);
	if (pFlexLine->m_bAutosize && pFlexLine->m_pParent)
	{
		pFlexLine->m_fWidth		= pFlexLine->m_bHorizontal ? pFlexLine->m_pParent->m_fWidth : pFlexLine->m_pFrameLine->m_fWidth; 
		pFlexLine->m_fHeight	= pFlexLine->m_bHorizontal ? pFlexLine->m_pFrameLine->m_fHeight : pFlexLine->m_pParent->m_fHeight; 
		pFlexLine->m_fZDelta	= pFlexLine->m_pParent->m_fZDelta; 
	}
	pFlexLine->m_bStretch = 0; 

	XML_LOADENUM("visibleonsibling", pFlexLine->m_nVisibleOnSibling, pFlexSibEnumTbl, FLEX_SIB_NONE);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadMenu(
	CMarkup &xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_MENU *pMenu = UICastToMenu(component);
	ASSERT(pMenu);

	XML_LOADBOOL("keeponeselected", pMenu->m_bAlwaysOneSelected, TRUE);


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadPanel(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_PANELEX* panel = UICastToPanel(component);
	ASSERT_RETFALSE(panel);

	panel->m_bUserActive = UIComponentGetVisible(panel);
	panel->m_bMouseDown = FALSE;

	XML_LOADINT("starttab", panel->m_nCurrentTab, component->m_bReference ? panel->m_nCurrentTab : -2);
	XML_LOADINT("tab", panel->m_nTabNum, -1);
	XML_LOADINT("modelcam", panel->m_nModelCam, 0);
	if (panel->m_nTabNum != -1)
	{
		XML_LOADBOOL("independentactivate", component->m_bIndependentActivate, TRUE);	// load again and default to true

		BOOL bDefaultVisible = FALSE;
		if (panel->m_pParent && UIComponentIsPanel(panel->m_pParent))
		{
			UI_PANELEX *pParentPanel = UICastToPanel(panel->m_pParent);
			if (pParentPanel->m_nCurrentTab == panel->m_nTabNum)
			{
				bDefaultVisible = TRUE;
			}

		}
		XML_LOADBOOL("visible", component->m_bVisible, bDefaultVisible );			// load again and default true iff it's the current tab
	}

	XML_LOADFRAME("highlightframe", UIComponentGetTexture(component), panel->m_pHighlightFrame, "");

	char szResizeComponent[256] = "";
	XML_LOADSTRING("resizecomponent", szResizeComponent, "", 256);
	if (szResizeComponent[0])
	{
		if (PStrICmp(szResizeComponent, "parent") == 0)
		{
			panel->m_pResizeComponent = panel->m_pParent;
		}
		else
		{
			panel->m_pResizeComponent = UIComponentFindParentByName(panel, szResizeComponent);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadTextBox(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_TEXTBOX* pTextBox = UICastToTextBox(component);
	ASSERT_RETFALSE(pTextBox);

	pTextBox->m_DataList.Init();
	pTextBox->m_AddedDataList.Init();

	XML_LOADBOOL("bottomup", pTextBox->m_bBottomUp, FALSE);
	XML_LOADBOOL("wordwrap", pTextBox->m_bWordWrap, FALSE);
	XML_LOADBOOL("hidescrollbar", pTextBox->m_bHideInactiveScrollbar, FALSE);
	XML_LOADBOOL("scrollonleft", pTextBox->m_bScrollOnLeft, FALSE);
	XML_LOADFLOAT("scrollextraspacing", pTextBox->m_fScrollbarExtraSpacing, 0.0f);
	XML_LOADINT("fadeticks", pTextBox->m_nFadeoutTicks, 0);
	XML_LOADINT("fadedelay", pTextBox->m_nFadeoutDelay, 0);

	XML_LOADINT("maxlines", pTextBox->m_nMaxLines, -1);
	XML_LOADENUM("align", pTextBox->m_nAlignment, pAlignEnumTbl, UIALIGN_TOPLEFT);

	XML_LOADFLOAT("bordersize", pTextBox->m_fBordersize, 0.0f);
	XML_LOADBOOL("dropshadow", pTextBox->m_bDrawDropshadow, TRUE);

	XML_LOADBOOL("bkgdrect", pTextBox->m_bBkgdRect, component->m_bReference ? pTextBox->m_bBkgdRect : FALSE);
	UIXMLLoadColor(xml, pTextBox, "bkgdrect", pTextBox->m_dwBkgdRectColor, component->m_bReference ? pTextBox->m_dwBkgdRectColor : GFXCOLOR_GRAY);
	
	pTextBox->m_bUserActive = pTextBox->m_bVisible;		// mainly for the chat
	
	UIXMLLoadColor(xml, pTextBox, "bkgcolor", pTextBox->m_dwBkgColor, 0);

	XML_LOADFRAME("scrollbarframe", UIComponentGetTexture(component), pTextBox->m_pScrollbarFrame, "");
	if (pTextBox->m_pScrollbarFrame)
	{
		XML_LOADFRAME("thumbpadframe",	UIComponentGetTexture(component), pTextBox->m_pThumbpadFrame, "");

		pTextBox->m_pScrollbar = (UI_SCROLLBAR *) UIComponentCreateNew(pTextBox, UITYPE_SCROLLBAR, TRUE);
		ASSERT_RETFALSE(pTextBox->m_pScrollbar);

		XML_GETBOOLATTRIB("scrollbarframe", "tile", pTextBox->m_pScrollbar->m_bTileFrame, FALSE);

		pTextBox->m_pScrollbar->m_nOrientation = UIORIENT_TOP;
		pTextBox->m_pScrollbar->m_bIndependentActivate = TRUE;
		pTextBox->m_pScrollbar->m_eState = UISTATE_INACTIVE;
		pTextBox->m_pScrollbar->m_bVisible = pTextBox->m_bVisible;//!pTextBox->m_bHideInactiveScrollbar;
		pTextBox->m_pScrollbar->m_pFrame = pTextBox->m_pScrollbarFrame;
		pTextBox->m_pScrollbar->m_pThumbpadFrame = pTextBox->m_pThumbpadFrame;
		pTextBox->m_pScrollbar->m_fHeight = pTextBox->m_fHeight;
		pTextBox->m_pScrollbar->m_fWidth = pTextBox->m_pScrollbarFrame->m_fWidth;
		pTextBox->m_pScrollbar->m_bSendPaintToParent = TRUE;
		pTextBox->m_pScrollbar->m_pScrollControl = pTextBox;
		sprintf_s(pTextBox->m_pScrollbar->m_szName, MAX_UI_COMPONENT_NAME_LENGTH, "%s scrollbar", pTextBox->m_szName);

		if (pTextBox->m_bScrollOnLeft)
		{
			pTextBox->m_pScrollbar->m_Position.m_fX = 0 - (pTextBox->m_pScrollbar->m_fWidth + pTextBox->m_fScrollbarExtraSpacing);
		}
		else
		{
			pTextBox->m_pScrollbar->m_Position.m_fX = pTextBox->m_fWidth + pTextBox->m_fScrollbarExtraSpacing;
		}

		pTextBox->m_pScrollbar->m_fWheelScrollIncrement = sUITextBoxGetLineHeight(pTextBox);
	}

	while (xml.FindChildElem("line"))
	{
		char temp[1024];
		xml.GetChildData(temp, 1024);
		WCHAR wtmp[1024];
		PStrCvt(wtmp, temp, 1024);
		UITextBoxAddLine(pTextBox, wtmp);
	}
	UIComponentHandleUIMessage(pTextBox, UIMSG_PAINT, 0, 0);
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentFreeTextBox(
	UI_COMPONENT* component)
{
	UI_TEXTBOX* pTextBox = UICastToTextBox(component);
	ASSERT_RETURN(pTextBox);

	pTextBox->m_DataList.Destroy();
	pTextBox->m_AddedDataList.Destroy();

	if (pTextBox->m_nNumChatChannels > 0 &&
		pTextBox->m_peChatChannels)
	{
		FREE(NULL, pTextBox->m_peChatChannels);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadStatDispL(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	//Load extras for parent class
	if (!UIXmlLoadLabel(xml, component, tUIXml))
		return FALSE;

	UI_STATDISPL* displ = UICastToStatDispL(component);
	ASSERT_RETFALSE(displ);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(displ);
	REF(font);
	//ASSERT_RETZERO(font);

	// default autosize to TRUE for statdisplays
	XML_LOADBOOL("autosize", displ->m_bAutosize, component->m_bReference ? displ->m_bAutosize : TRUE);

	XML_LOADBOOL("showicons", displ->m_bShowIcons, component->m_bReference ? displ->m_bShowIcons : FALSE);
	XML_LOADBOOL("verticons", displ->m_bVertIcons, component->m_bReference ? displ->m_bVertIcons : FALSE);

	XML_LOADFLOAT("specialspacing", displ->m_fSpecialSpacing, component->m_bReference ? displ->m_fSpecialSpacing :  0.0f);

	if (!component->m_bReference)		// if this isn't a reference, set the defaults
	{
		displ->m_eTable = DATATABLE_NONE;
		displ->m_nDisplayLine = -1;
		displ->m_nDisplayArea = -1;
	}

	XML_LOADEXCELILINK("unittype", displ->m_nUnitType, DATATABLE_UNITTYPES);

	char key[256] = "";
	XML_LOADSTRING("itemdisplay", key, "", 256);
	if (key[0] != 0)
	{
		displ->m_eTable = DATATABLE_ITEMDISPLAY;
	}
	else
	{
		XML_LOADSTRING("chardisplay", key, "", 256);
		if (key[0] != 0)
		{
			displ->m_eTable = DATATABLE_CHARDISPLAY;
		}
	}

	// a second font, only used right now for the icon-having statdispls
	UIXMLLoadFont( xml, component, UIGetFontTexture(LANGUAGE_CURRENT), "font2", displ->m_pFont2, "fontsize2", displ->m_nFontSize2);

	// you can either have a display line (single line) or a display area (all stats for that area).
	if (displ->m_eTable != DATATABLE_NONE && key[0])
	{
		displ->m_nDisplayLine = ExcelGetLineByStringIndex(EXCEL_CONTEXT(AppGetCltGame()), displ->m_eTable, key);
		return TRUE;
	}
	else
	{
		//XML_LOADINT("itemdisplayarea", displ->m_nDisplayArea, -1);
		//XML_LOADENUM("itemdisplayarea", displ->m_nDisplayArea, (STR_DICT *) pTooltipAreas, -1);
		XML_LOADENUM("itemdisplayarea", displ->m_nDisplayArea, tooltipareas, -1);
		if (displ->m_nDisplayArea != -1)
		{
			displ->m_eTable = DATATABLE_ITEMDISPLAY;
			return TRUE;
		}
		else
		{
			// char display area would go here if we wanted it
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadAutoMap(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	//Load extras for parent class
	UIXmlLoadPanel(xml, component, tUIXml);

	UI_AUTOMAP* automap = UICastToAutoMap(component);
	ASSERT_RETFALSE(automap);

	XML_LOADFLOAT("monsterdistance",automap->m_fMonsterDistance, 10.0f);

	UIX_TEXTURE *texture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(texture);

	automap->m_pFrameWarpIn =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap warp in");
	automap->m_pFrameWarpOut =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap warp out");
	automap->m_pFrameHero =			(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap hero");
	automap->m_pFrameTown =			(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap town arrow");
	automap->m_pFrameMonster =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap monster");
	automap->m_pFrameHellrift = 	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap hellrift");
	automap->m_pFrameQuestNew =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap quest new");
	automap->m_pFrameQuestWaiting = (UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap quest waiting");
	automap->m_pFrameQuestReturn =	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap quest return");
	automap->m_pFrameVendorNPC =	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap vendorNPC");
	automap->m_pFrameHealerNPC =	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap healerNPC");
	automap->m_pFrameCrafterNPC =	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap crafterNPC");
	automap->m_pFrameTrainerNPC =	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap trainerNPC");
	automap->m_pFrameGuideNPC =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap guideNPC");
	automap->m_pFrameMapVendorNPC =	(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap mapvendorNPC");
	automap->m_pFrameGrave =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap grave");
	automap->m_pGamblerNPC =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap gamblerNPC");
	automap->m_pGuildmasterNPC =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap guildmasterNPC");
	automap->m_pFrameMapExit =		(UI_TEXTURE_FRAME*)StrDictionaryFind(texture->m_pFrames, "automap mapexit");

	XML_LOADBYTE("iconalpha",  automap->m_bIconAlpha, 255);
	XML_LOADBYTE("wallsalpha", automap->m_bWallsAlpha, 255);
	XML_LOADBYTE("textalpha",  automap->m_bTextAlpha, 255);

	automap->m_nHellriftClass = INVALID_ID;
	if( AppIsHellgate() )	// tug has no hellgates.
	{
		automap->m_nHellriftClass = GlobalIndexGet( GI_MONSTER_HELLGATE );
		if (automap->m_nHellriftClass != INVALID_ID)
		{
			const UNIT_DATA *pUnitDataHellgate = MonsterGetData( AppGetCltGame(), automap->m_nHellriftClass );
			if (pUnitDataHellgate)
			{
				const WCHAR *szString = StringTableGetStringByIndex( pUnitDataHellgate->nString );
				if (szString)
					PStrCopy(automap->m_szHellgateString, szString, MAX_AUTOMAP_STRING);
			}
		}
	}

	automap->m_nLastLevel = INVALID_LINK;

	automap->m_pRectOrganize = RectOrganizeInit(NULL);

	
	GAMEOPTIONS Options;
	structclear(Options);
	GameOptions_Get(Options);

	//if we've never stored the zoom level and scale in the game options..
	if(Options.nAutomapZoomLevel == 0 || Options.fAutomapScale == 0.0f)
	{
		//load from the xml and store the xml's values in the game options
		XML_LOADINT("zoomlevel",automap->m_nZoomLevel, 3);
		XML_LOADFLOAT("startingscale",automap->m_fScale, 3);
		Options.nAutomapZoomLevel = automap->m_nZoomLevel;
		Options.fAutomapScale = automap->m_fScale;
		GameOptions_Set(Options);
	}
	else
	{
		//otherwise, just use the gameoptions values.
		automap->m_nZoomLevel = Options.nAutomapZoomLevel;
		automap->m_fScale = Options.fAutomapScale;
	}


	CSchedulerRegisterEventImm(UIAutoMapDisplayUpdate, CEVENT_DATA((DWORD_PTR)automap));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadItemBox(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	//Load extras for parent class
	UIXmlLoadPanel(xml, component, tUIXml);

	UI_ITEMBOX *pItemBox = UICastToItemBox(component);
	XML_LOADFRAME("background", UIComponentGetTexture(component), pItemBox->m_pFrameBackground, "");
	
	int r = -1, g = -1, b = -1, a = -1;
	XML_LOADINT("quanitityred", r, -1);
	XML_LOADINT("quanititygreen", g, -1);
	XML_LOADINT("quanitityblue", b, -1);
	XML_LOADINT("quanitityalpha", a, 255);
	pItemBox->m_dwColorQuantity = GFXCOLOR_MAKE( r, g, b, a );

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentRemoveAutoMapItems(
	UI_AUTOMAP* automap)
{
	ASSERT_RETURN(automap);

	AUTOMAP_ITEM* item = automap->m_pAutoMapItems;
	while (item)
	{
		AUTOMAP_ITEM* next = item->m_pNext;
		FREE(NULL, item);
		item = next;
	}
	automap->m_pAutoMapItems = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentFreeAutoMap(
	UI_COMPONENT* component)
{
	UI_AUTOMAP* automap = UICastToAutoMap(component);
	ASSERT_RETURN(automap);

	UIComponentRemoveAutoMapItems(automap);

	if (automap->m_pRectOrganize)
	{
		RectOrganizeDestroy(NULL, automap->m_pRectOrganize);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void sUIUnicodeConvertSpecials(
	WCHAR * text)
{
	while (text && *text)
	{
		// Convert single-quotes into apostrophes
		if (*text == 0x201B || *text == 0x2018 || *text == 0x2019)
		{
			*text = 0x0027;
		}
		text++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetTextByStringIndex(
	UI_COMPONENT *pComponent,
	int nStringIndex)
{
	UILabelSetText( pComponent, StringTableGetStringByIndex( nStringIndex ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetTextByStringKey(
	UI_COMPONENT *pComponent,
	const char *pszStringKey)
{
	UI_LABELEX *pLabel = UICastToLabel( pComponent );	
	ASSERTX_RETURN( pLabel, "Expected label" );
	
	// save the string key
#if ISVERSION(DEVELOPMENT)
	PStrCopy( pLabel->m_szTextKey, pszStringKey, MAX_STRING_KEY_LENGTH );
#endif
	
	// set the text
	UILabelSetText( pComponent, StringTableGetStringByKey( pszStringKey ) );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetTextByGlobalString(
	UI_COMPONENT *pComponent,
	GLOBAL_STRING eGlobalString)
{
	UILabelSetTextByStringKey( pComponent, GlobalStringGetKey( eGlobalString ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sLabelScanTags(
	UI_LABELEX *pLabel)
{
	ASSERT_RETURN(pLabel);
	pLabel->m_nNumPages = 1;

	for (int i=0; i < pLabel->m_nUnitmodesAlloc; i++)
	{
		pLabel->m_pUnitmodes[i].mode = MODE_INVALID;
	}

	int iMode = 0;
	WCHAR uszContents[1024];
	if (pLabel->m_Line.HasText())
	{
		WCHAR *szTag = PStrStrI(pLabel->m_Line.GetText(), TEXT_TAG_BEGIN_STRING);
		while (szTag)
		{
			int nTextTag = UIDecodeTextTag(szTag, uszContents, arrsize(uszContents));
			if (nTextTag == TEXT_TAG_PGBRK ||
				nTextTag == TEXT_TAG_INTERRUPT ||
				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
			{
				pLabel->m_nNumPages++;
			}

			if ((nTextTag == TEXT_TAG_UNITMODE ||
				nTextTag == TEXT_TAG_UNITMODE_END)
				&& pLabel->m_bPlayUnitModes)
			{
				if (iMode >= pLabel->m_nUnitmodesAlloc)
				{
					pLabel->m_nUnitmodesAlloc++;
					if (!pLabel->m_pUnitmodes)
					{
						pLabel->m_pUnitmodes = (LABEL_UNITMODE *)MALLOC(NULL, sizeof(LABEL_UNITMODE));
						ASSERT_RETURN(pLabel->m_pUnitmodes);
					}
					else
					{
						pLabel->m_pUnitmodes = (LABEL_UNITMODE *)REALLOC(NULL, pLabel->m_pUnitmodes, sizeof(LABEL_UNITMODE) * pLabel->m_nUnitmodesAlloc);
						ASSERT_RETURN(pLabel->m_pUnitmodes);
					}
				}

				WCHAR uszBuf[256];
				BOOL bResult = UIStringTagGetStringParam(uszContents, L"m=", uszBuf, arrsize(uszBuf));
				if (!bResult || !uszBuf[0])
				{
					char szMsg[256];
					PStrPrintf(szMsg, arrsize(szMsg),"Expected mode in unitmode tag %s", szTag);
					ASSERTX_RETURN(FALSE, szMsg);
				}
				char szBuf[256];
				PStrCvt(szBuf, uszBuf, arrsize(szBuf));
				pLabel->m_pUnitmodes[iMode].mode = (UNITMODE)ExcelGetLineByStrKey(DATATABLE_UNITMODES, szBuf);

				pLabel->m_pUnitmodes[iMode].bEnd = (nTextTag==TEXT_TAG_UNITMODE_END);
				pLabel->m_pUnitmodes[iMode].nPage = pLabel->m_nNumPages-1;

				int nVal = 0;
				UIStringTagGetIntParam(uszContents, L"d=", nVal);
				pLabel->m_pUnitmodes[iMode].fDurationInSeconds = (float)nVal;
				nVal = 0;
				UIStringTagGetIntParam(uszContents, L"wp=", nVal);
				pLabel->m_pUnitmodes[iMode].wParam = (WORD)nVal;
				iMode++;
			}
			if ( pLabel->m_bPlayUnitModes && 
				nTextTag >= TEXT_TAG_EMOTE_ANIM_START && 
				nTextTag <= TEXT_TAG_EMOTE_ANIM_END )
			{
				UNITMODE eAnim = MODE_INVALID;
				switch ( nTextTag )
				{
				case TEXT_TAG_TALK:
					eAnim = MODE_NPC_GENERIC_TALK_IN;
					break;
				case TEXT_TAG_ANGRY:
					eAnim = MODE_NPC_ANGRY_TALK_IN;
					break;
				case TEXT_TAG_EXCITED:
					eAnim = MODE_NPC_EXCITED_TALK_IN;
					break;
				case TEXT_TAG_PROUD:
					eAnim = MODE_NPC_PROUD_IDLE;
					break;
				case TEXT_TAG_SAD:
					eAnim = MODE_NPC_SAD_TALK_IN;
					break;
				case TEXT_TAG_SCARED:
					eAnim = MODE_NPC_SCARED_TALK_IN;
					break;
				case TEXT_TAG_WAVE:
					eAnim = MODE_NPC_WAVE;
					break;
				case TEXT_TAG_BOW:
					eAnim = MODE_NPC_BOW;
					break;
				case TEXT_TAG_GIVE:
					eAnim = MODE_NPC_GIVE;
					break;
				case TEXT_TAG_GET:
					eAnim = MODE_NPC_GET;
					break;
				case TEXT_TAG_PICKUP:
					eAnim = MODE_NPC_PICKUP;
					break;
				case TEXT_TAG_SECRET:
					eAnim = MODE_NPC_SECRET_TALK_IN;
					break;
				case TEXT_TAG_KNEEL:
					eAnim = MODE_NPC_KNEEL_IN;
					break;
				case TEXT_TAG_TURN:
					eAnim = MODE_NPC_TURN;
					break;
				};

				if ( eAnim != MODE_INVALID )
				{
					if (iMode >= pLabel->m_nUnitmodesAlloc)
					{
						pLabel->m_nUnitmodesAlloc++;
						if (!pLabel->m_pUnitmodes)
						{
							pLabel->m_pUnitmodes = (LABEL_UNITMODE *)MALLOC(NULL, sizeof(LABEL_UNITMODE));
							ASSERT_RETURN(pLabel->m_pUnitmodes);
						}
						else
						{
							pLabel->m_pUnitmodes = (LABEL_UNITMODE *)REALLOC(NULL, pLabel->m_pUnitmodes, sizeof(LABEL_UNITMODE) * pLabel->m_nUnitmodesAlloc);
							ASSERT_RETURN(pLabel->m_pUnitmodes);
						}
					}

					pLabel->m_pUnitmodes[iMode].mode = eAnim;
					pLabel->m_pUnitmodes[iMode].bEnd = FALSE;
					pLabel->m_pUnitmodes[iMode].nPage = pLabel->m_nNumPages-1;
					pLabel->m_pUnitmodes[iMode].fDurationInSeconds = 0.0f;
					pLabel->m_pUnitmodes[iMode].wParam = 0;
					iMode++;
				}
			}
			szTag = PStrStrI(szTag+1, TEXT_TAG_BEGIN_STRING);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sLabelSetUnitmodeForPage(
	UI_LABELEX *pLabel)
{
	ASSERT_RETURN(pLabel);

	if (!pLabel->m_bPlayUnitModes)
		return;

	UNIT *pFocusUnit = UIComponentGetFocusUnit(pLabel);
	if (!pFocusUnit)
	{
		return;
	}

	if (pLabel->m_nUnitmodesAlloc == 0 ||
		pLabel->m_pUnitmodes == NULL)
	{
		return;
	}

	for (int i=0; i < pLabel->m_nUnitmodesAlloc; i++)
	{
		if (pLabel->m_pUnitmodes[i].mode == MODE_INVALID)
			break;

		if (pLabel->m_pUnitmodes[i].nPage == pLabel->m_nPage)
		{
			if (pLabel->m_pUnitmodes[i].bEnd)
			{
				UnitEndMode(pFocusUnit, pLabel->m_pUnitmodes[i].mode, pLabel->m_pUnitmodes[i].wParam);
			}
			else
			{
				c_UnitSetMode(pFocusUnit, pLabel->m_pUnitmodes[i].mode, pLabel->m_pUnitmodes[i].wParam, pLabel->m_pUnitmodes[i].fDurationInSeconds);
			}
		}
	}


}

//----------------------------------------------------------------------------
int CONVERSATION_UPDATE_DELAY_IN_MS = 20;
int CONVERSATION_CHAR_GROUP_SIZE = 3;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoSlowRevealUpdate(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD dwData)
{
	int idLabel = (int)tEventData.m_Data1;
	ASSERTX_RETURN( idLabel != INVALID_ID, "Expected label id" );	
	UI_COMPONENT *pComponent = UIComponentGetById( idLabel );	
	ASSERTX_RETURN( pComponent, "Expected component" );
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	// clear event record
	pLabel->m_dwUpdateEvent = 0;

	// what's the max characters to display
	int nNewMaxChars = pLabel->m_nMaxChars + pLabel->m_nNumCharsToAddEachUpdate;	
	nNewMaxChars = PIN( nNewMaxChars, 0, pLabel->m_nTextToRevealLength );
	pLabel->m_nMaxChars = nNewMaxChars;

	// set the new terminator index in the gfx element
	if (pLabel->m_pTextElement)
	{
		UITextElementSetEarlyTerminator( pLabel->m_pTextElement, pLabel->m_nMaxChars );
		
		// update the length of the actual string to reveal based on the text element
		// count (the text element is the thing that actually parses the pages etc)
		int nNumCharsOnPage = UITextElementGetNumCharsOnPage( pLabel->m_pTextElement );
		if (nNumCharsOnPage > 0)
		{
			pLabel->m_nTextToRevealLength = nNumCharsOnPage;
		}
	}

	// need to render this
	UISetNeedToRender( pLabel->m_nRenderSection );
	
	// repaint the label
//	UILabelOnPaint( pLabel, UIMSG_PAINT, 0, 0 );
	
	// if we still have more stuff to display, keep updating
	if (pLabel->m_nMaxChars < pLabel->m_nTextToRevealLength)
	{
		pLabel->m_dwUpdateEvent = CSchedulerRegisterEvent( 
			AppCommonGetCurTime() + CONVERSATION_UPDATE_DELAY_IN_MS, 
			sDoSlowRevealUpdate, 
			tEventData);	
	}
	else
	{
		UIComponentHandleUIMessage( pLabel, UIMSG_FULLTEXTREVEAL, 0, 0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStartSlowRevealUpdateEvent(
	int idLabel)
{	
	CEVENT_DATA tEventData;
	tEventData.m_Data1 = idLabel;	
	sDoSlowRevealUpdate( AppGetCltGame(), tEventData, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUILabelSlowRevealStop(
	UI_LABELEX *pLabel)
{					
	if (pLabel->m_dwUpdateEvent != 0)
	{
		CSchedulerCancelEvent( pLabel->m_dwUpdateEvent );
		pLabel->m_dwUpdateEvent = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUILabelDoRevealAllText(
	UI_COMPONENT *pComponent)
{
	if (pComponent->m_eComponentType == UITYPE_LABEL)
	{
		UI_LABELEX *pLabel = UICastToLabel( pComponent );			
		if (pLabel->m_bSlowReveal)
		{
			
			// stop any updating that is currently happening
			sUILabelSlowRevealStop( pLabel );
			
			// set the string has fully visible
			if (pLabel->m_nMaxChars != pLabel->m_nTextToRevealLength)
			{
			
				// skip to end
				pLabel->m_nMaxChars = MAX( pLabel->m_nTextToRevealLength, MAX_STRING_ENTRY_LENGTH * 2 );
				
				// do one last update event
				sStartSlowRevealUpdateEvent( UIComponentGetId( pLabel ) );
				
				// we did a reveal
				return TRUE;

			}
			
		}
			
	}
	return FALSE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UILabelDoRevealAllText(
	UI_COMPONENT *pComponent)
{
	ASSERTX_RETFALSE( pComponent, "Expected component" );
	
	// do this comp
	BOOL bDidReveal = sUILabelDoRevealAllText( pComponent );
		
	UI_COMPONENT *pChild = pComponent->m_pFirstChild;
	while (pChild)
	{
		// do this child
		if (sUILabelDoRevealAllText( pChild ))
		{
			bDidReveal = TRUE;
		}
		pChild = pChild->m_pNextSibling;	
	}

	return bDidReveal;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelRevealAllText(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	if (UIComponentGetActive( pComponent ) && UIComponentCheckBounds( pComponent ))
	{
		UILabelDoRevealAllText( pComponent );			
		return UIMSG_RET_HANDLED;		
	}
	
	return UIMSG_RET_NOT_HANDLED;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelStartSlowReveal(
	UI_COMPONENT *pComponent)
{
	ASSERTX_RETURN( pComponent, "Expected component" );
	UI_LABELEX *pLabel = UICastToLabel( pComponent );

	// stop any slow reveal in progress
	sUILabelSlowRevealStop( pLabel );
	// init text values
	int nLen = (pLabel->m_Line.HasText())? PStrLen(pLabel->m_Line.GetText()) : 0;
	pLabel->m_nTextToRevealLength = MIN( nLen + 1, (int)MAX_STRING_ENTRY_LENGTH );
	pLabel->m_nMaxChars = 1;  // start at first char

	// based on the text display speed, how many characters will we display per update
	pLabel->m_nNumCharsToAddEachUpdate = CONVERSATION_CHAR_GROUP_SIZE;

	// start event updating
	sStartSlowRevealUpdateEvent( pLabel->m_idComponent );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetText(
	UI_COMPONENT* component,
	const WCHAR* str,
	BOOL bCheckDifferent /*= FALSE*/)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETURN(label);

	const WCHAR *puszStringToUse = str;
	
	// when font size mode is on, all strings will display the point size of the font of the
	// control they are inside of
#if ISVERSION(DEVELOPMENT)	
	if (component != UIComponentGetByEnum(UICOMP_CONSOLE_EDIT))
	{
		WCHAR uszBuf[ 256 ];
		if (AppTestDebugFlag( ADF_TEXT_FONT_NAME ))
		{
			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			if (pFont)
			{
				PStrCvt( uszBuf, pFont->m_szDebugName, arrsize(uszBuf));
			}
			else
			{
				PStrPrintf( uszBuf, arrsize(uszBuf), L"No font" );
			}
			puszStringToUse = uszBuf;
		}	
		else if (AppTestDebugFlag( ADF_TEXT_POINT_SIZE ))
		{
			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			if (pFont)
			{
				PStrPrintf( uszBuf, arrsize(uszBuf), L"%d point size", UIComponentGetFontSize(component) );
			}
			else
			{
				PStrPrintf( uszBuf, arrsize(uszBuf), L"No font" );
			}
			puszStringToUse = uszBuf;
		}	
	}
#endif

	if (bCheckDifferent &&
		puszStringToUse && 
		( ( !label->m_Line.HasText() && ( PStrLen( puszStringToUse ) == 0 ) ) ||
		  ( label->m_Line.HasText() &&
	  	    PStrCmp(puszStringToUse, label->m_Line.GetText()) == 0) ) )
	{
		return;
	}

	if (UIComponentIsEditCtrl(label))
	{
		UI_EDITCTRL *pEdit = UICastToEditCtrl(label);
		int len = puszStringToUse? PStrLen(puszStringToUse) : 0;
		pEdit->m_nCaretPos = MIN(len, pEdit->m_nMaxLen);
		label->m_Line.Resize(pEdit->m_nMaxLen);
	}

	if (puszStringToUse)
		label->m_Line.SetText(puszStringToUse);
	else
		label->m_Line.SetText(L"\0");

	if (label->m_Line.HasText())
	{
		// Process all the various special tokens and hypertext that might be in this line
		sUIUnicodeConvertSpecials(label->m_Line.EditText());
		UITextStringReplaceFormattingTokens(label->m_Line.EditText());
		UIReplaceStringTags(label, &label->m_Line);
		UIProcessHyperText(&label->m_Line);
		sLabelScanTags(label);
	}
	else
	{
		// Clean out our hypertext list
		label->m_Line.m_HypertextList.Destroy();
	}

	//label->m_nNumPages = 1 + sCountTextPageBreaks(label->m_szText);
	label->m_nPage = 0;

	label->m_eMarqueeState = MARQUEE_STATE_NONE;
	label->m_posMarquee.m_fX = 0.0f;
	label->m_posMarquee.m_fY = 0.0f;

	UI_COMPONENT *pFirstChild = label->m_pFirstChild;
	if (pFirstChild && UIComponentIsScrollBar(pFirstChild))
	{
		label->m_ScrollPos.m_fY = 0.0f;
		pFirstChild->m_ScrollPos.m_fY = 0.0f;
	}
	else if( label->m_pScrollbar )
	{
		label->m_ScrollPos.m_fY = 0.0f;
		label->m_pScrollbar->m_ScrollPos.m_fY = 0.0f;
	}

	UILabelOnPaint(label, UIMSG_PAINT, 0, 0);
	
	if (label->m_bSlowReveal)
	{
		UILabelStartSlowReveal( label );		// this should go after the paint because it does some checking
												// of the actual gfx element.
	}
	
	sLabelSetUnitmodeForPage(label);
	sUILabelAdjustScrollbar(label);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetText(
	int idComponent,
	const WCHAR* str,
	BOOL bCheckDifferent /*= FALSE*/)
{
	UI_COMPONENT* component = UIComponentGetById(idComponent);
	ASSERT_RETURN(component);

	UILabelSetText(component, str, bCheckDifferent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetTextByEnum( 
	UI_COMPONENT_ENUM eType, 
	const WCHAR *puszName)
{
	UILabelSetText( UIComponentGetByEnum(eType), puszName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UILabelSetTextKeyByEnum( 
	UI_COMPONENT_ENUM eType, 
	const char *pszKey)
{
	UILabelSetTextByStringKey( UIComponentGetByEnum(eType), pszKey );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR* UILabelGetText(
	UI_COMPONENT* component)
{
	ASSERT_RETNULL(component);

	UI_LABELEX *pLabel = UICastToLabel(component);
	return pLabel->m_Line.GetText();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR* UILabelGetTextByEnum( 
	UI_COMPONENT_ENUM eType)
{
	return UILabelGetText( UIComponentGetByEnum(eType) );
}

void sLabelAdjustMarquee(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_LABELEX *label = (UI_LABELEX *)data.m_Data1;
	ASSERT_RETURN(label);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(label);
	if (!label->m_Line.HasText() || !font || !label->m_bVisible)		// not using GetVisible here because the component or its parent might not be active
	{
		return;
	}
	
	ASSERT_RETURN(label->m_eMarqueeMode == MARQUEE_HORIZ || label->m_eMarqueeMode == MARQUEE_VERT);

	float &fTextSize = (label->m_eMarqueeMode == MARQUEE_HORIZ ? label->m_fTextWidth : label->m_fTextHeight);
	float &fTextLimit = (label->m_eMarqueeMode == MARQUEE_HORIZ ? label->m_fWidth : label->m_fHeight);
	float &fPos = (label->m_eMarqueeMode == MARQUEE_HORIZ ? label->m_posMarquee.m_fX : label->m_posMarquee.m_fY);

	UI_ALIGN eAlign = (UI_ALIGN)label->m_nAlignment;
	if (fTextSize > fTextLimit)
	{
		if (e_UIAlignIsTop(eAlign))
		{
			eAlign = UIALIGN_TOPLEFT;
		}
		else if (e_UIAlignIsCenterVert(eAlign))
		{
			eAlign = UIALIGN_LEFT;
		}
		else if (e_UIAlignIsBottom(eAlign))
		{
			eAlign = UIALIGN_BOTTOMLEFT;
		}
		if (label->m_eMarqueeState == MARQUEE_STATE_END_WAIT)
		{
			fPos = 0.0f;		// start over
		}

		BOOL bDelayStart = (fPos == 0.0f);
		if (label->m_dwMarqueeAnimTicket != INVALID_ID)
		{
			CSchedulerCancelEvent(label->m_dwMarqueeAnimTicket);
		}

		// if this one will put us over the end
		if (fPos < fTextLimit - fTextSize && label->m_nMarqueeEndDelay > 0)
		{
			label->m_eMarqueeState = MARQUEE_STATE_END_WAIT;
			label->m_dwMarqueeAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + label->m_nMarqueeEndDelay, sLabelAdjustMarquee, CEVENT_DATA((DWORD_PTR)label));
		}
		else if (bDelayStart && label->m_nMarqueeStartDelay > 0)
		{
			fPos -= label->m_fMarqueeIncrement;
			label->m_eMarqueeState = MARQUEE_STATE_START_WAIT;
			label->m_dwMarqueeAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + label->m_nMarqueeStartDelay, sLabelAdjustMarquee, CEVENT_DATA((DWORD_PTR)label));
		}
		else
		{
			fPos -= label->m_fMarqueeIncrement;
			label->m_eMarqueeState = MARQUEE_STATE_SCROLLING;
			label->m_dwMarqueeAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime()+10, sLabelAdjustMarquee, CEVENT_DATA((DWORD_PTR)label));
		}
	}
	else
	{
		fPos = 0.0f;
	}

	UIComponentRemoveAllElements(label);
	UIComponentAddFrame(label);

	DWORD dwColor = label->m_dwColor;
	if (label->m_bDisabled)
	{
		dwColor = label->m_dwDisabledColor;
	}
	else if (label->m_bSelected) 
	{
		dwColor = label->m_dwSelectedColor;
	}

	label->m_pTextElement = UIComponentAddTextElement(
		label, 
		UIComponentGetTexture(label), 
		font, 
		UIComponentGetFontSize(label, font), 
		label->m_Line.GetText(), 
		label->m_posMarquee, 
		dwColor, 
		&UI_RECT(0.0f, 0.0f, label->m_fWidth, label->m_fHeight), 
		eAlign,
		NULL,
		TRUE,
		label->m_nPage);

	if (label->m_bDrawDropshadow)
	{
		label->m_pTextElement->m_dwDropshadowColor = label->m_dwDropshadowColor;
	}

	if (g_UI.m_bDebugTestingToggle)
		UIComponentAddPrimitiveElement(label, UIPRIM_BOX, 0, UI_RECT(0, 0, label->m_fWidth, label->m_fHeight), NULL, NULL, GFXCOLOR_GREEN);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelOnPostInactivate(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_LABELEX *pLabel = UICastToLabel(pComponent);
	if (pLabel->m_dwMarqueeAnimTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(pLabel->m_dwMarqueeAnimTicket);
	}

	if (pLabel->m_bSlowReveal)
	{
		sUILabelSlowRevealStop( pLabel );
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIGetSpeakerNumFromText(
	const WCHAR *szText,
	int nPageNum,
	BOOL & bVideo)
{
	if (!szText || !szText[0])
		return 0;

	int nCurrentPage = 0;
	int nCurrentSpeaker = 0;
	bVideo = FALSE;

	WCHAR wc;
	while ((wc = *szText++) != 0)
	{
		if (wc == TEXT_TAG_BEGIN_CHAR)
		{
			int nTextTag = UIDecodeTextTag(szText);
			if (nTextTag == TEXT_TAG_VIDEO)
			{
				bVideo = TRUE;
			}
			if (nTextTag == TEXT_TAG_PGBRK)
			{
				nCurrentPage++;
			}
			if (nTextTag == TEXT_TAG_INTERRUPT ||
				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
			{
				bVideo = (nTextTag == TEXT_TAG_INTERRUPT_VIDEO);
				nCurrentPage++;
				nCurrentSpeaker = !nCurrentSpeaker;
			}
		}

		if (nPageNum == nCurrentPage)
		{
			return nCurrentSpeaker;
		}
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETVAL(label, UIMSG_RET_NOT_HANDLED);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(component);
	//ASSERT_RETZERO(font);
	if (!font)
		return UIMSG_RET_NOT_HANDLED;

	int nFontSize = UIComponentGetFontSize(component, font);
	float fWidthRatio = 1.0; //fontsize / font->m_fHeight;

	UI_RECT pBkgdRects[300];
	UI_TXT_BKGD_RECTS tRects;
	tRects.pRectBuffer = pBkgdRects;
	tRects.nBufferSize = arrsize(pBkgdRects);
	tRects.nNumRects = 0;

	if (!label->m_Line.HasText())
	{
		label->m_fTextWidth = 0.0f;
		label->m_fTextHeight = 0.0f;
	}
	else if (!label->m_bWordWrap)
	{
		UIElementGetTextLogSize(font, 
			nFontSize, 
			fWidthRatio, 
			label->m_bNoScaleFont, 
			label->m_Line.GetText(), 
			&label->m_fTextWidth,
			&label->m_fTextHeight,
			FALSE, 
			FALSE,
			label->m_nPage,
			-1,
			label->m_bBkgdRect ? &tRects : NULL);
	}

	UI_SCROLLBAR *pScrollbar = NULL;
	UI_COMPONENT *pFirstChild = label->m_pFirstChild;
	if (pFirstChild && UIComponentIsScrollBar(pFirstChild))
	{
		pScrollbar = UICastToScrollBar(pFirstChild);
	}

	UIComponentRemoveAllElements(label);
	if (label->m_pFrame)
	{
		UIComponentAddElement(label, 
			UIComponentGetTexture(label), label->m_pFrame, 
			UI_POSITION(label->m_fFrameOffsetX, label->m_fFrameOffsetY), 
			label->m_dwColor, NULL);
	}

	if (label->m_bWordWrap && label->m_Line.HasText())
	{
		// NOTE: UIGetLogToScreenRatioX if the param is true returns 1.0f
		float fMaxWidth = label->m_fMaxWidth;
		if (pScrollbar)
		{
			fMaxWidth -= pScrollbar->m_fWidth;
		}
		WCHAR * dummy = NULL;
		UI_SIZE size = DoWordWrapEx(dummy,
			&(label->m_Line),
			fMaxWidth * UIGetLogToScreenRatioX(label->m_bNoScale || !label->m_bNoScaleFont), 
			font, 
			label->m_bNoScaleFont, 
			UIComponentGetFontSize(label, font), 
			label->m_nPage, 
			1.0f,
			label->m_bBkgdRect ? &tRects : NULL,
			label->m_bForceWrap);
		label->m_fTextWidth = size.m_fWidth;
		label->m_fTextHeight = size.m_fHeight;
	}

	if (label->m_bNoScaleFont && !label->m_bNoScale)
	{
		// the text size is specified as screen coordinates (and logical coordinates)
		//   the component however will try to convert the sizes, so we need to pre-emptively scale them up
		label->m_fTextHeight /= UIGetLogToScreenRatioY();
		label->m_fTextWidth /= UIGetLogToScreenRatioX();
	}

	if (label->m_bAutosize || label->m_bAutosizeHorizontalOnly)
	{
		label->m_fWidth = label->m_fTextWidth;
	}

	if (label->m_bAutosize || label->m_bAutosizeVerticalOnly)
	{
		label->m_fHeight = label->m_fTextHeight;
		if (label->m_fMaxHeight > 0.0f)
			label->m_fHeight = MIN(label->m_fHeight, label->m_fMaxHeight);
	}

	DWORD dwColor = label->m_dwColor;
	if (label->m_Line.HasText())
	{
		if (pScrollbar)
		{
			if (label->m_bAutosize || label->m_bAutosizeHorizontalOnly)
			{
				pScrollbar->m_Position.m_fX = label->m_fWidth;
				label->m_fWidth += pScrollbar->m_fWidth;
			}
			pScrollbar->m_fHeight = label->m_fHeight;
			pScrollbar->m_fMin = 0;
			pScrollbar->m_fMax = label->m_fTextHeight - label->m_fHeight;

			if (pScrollbar->m_fMax <= 0.0f)
			{
				UIComponentSetVisible(pScrollbar, FALSE);
			}
			else
			{
				UIComponentSetActive(pScrollbar, TRUE);
			}
		}

		if (label->m_bDisabled)
		{
			dwColor = label->m_dwDisabledColor;
		}
		else if (label->m_bSelected) 
		{
			dwColor = label->m_dwSelectedColor;
		}

		if (label->m_bAutosizeFont &&
			(label->m_fTextHeight > label->m_fHeight ||
			 label->m_fTextWidth > label->m_fWidth))
		{
			float fRatio = MAX(label->m_fTextHeight / label->m_fHeight, label->m_fTextWidth / label->m_fWidth);
			nFontSize = (int)(((float)nFontSize / fRatio) - 0.5);

			UIElementGetTextLogSize(font, 
				nFontSize, 
				fWidthRatio, 
				label->m_bNoScaleFont, 
				label->m_Line.GetText(), 
				&label->m_fTextWidth, 
				&label->m_fTextHeight,
				FALSE, 
				FALSE,
				label->m_nPage,
				-1,
				label->m_bBkgdRect ? &tRects : NULL);

			// This is not the fastest way possible but hopefully we're not doing this very much
			// Integral font sizes and varying character widths makes this tricky
			while (nFontSize > 0 &&
				   (label->m_fTextHeight > label->m_fHeight ||
				   label->m_fTextWidth > label->m_fWidth))
			{
				nFontSize--;
				UIElementGetTextLogSize(font, 
					nFontSize, 
					fWidthRatio, 
					label->m_bNoScaleFont, 
					label->m_Line.GetText(), 
					&label->m_fTextWidth, 
					&label->m_fTextHeight,
					FALSE, 
					FALSE,
					label->m_nPage,
					-1,
					label->m_bBkgdRect ? &tRects : NULL);
			}

			if (nFontSize <= 0)
				nFontSize = UIComponentGetFontSize(component, font);
		}

		if (label->m_bBkgdRect)
		{
			UI_RECT tClipRect(0.0f, 0.0f, label->m_fWidth, label->m_fHeight);

			float fOffsY = 0.0f;
			if (e_UIAlignIsCenterVert(label->m_nAlignment) || e_UIAlignIsBottom(label->m_nAlignment))
			{
				float fTotalHeight = 0.0f;
				for (int i=0; i < tRects.nNumRects; i++)
				{
					fTotalHeight += tRects.pRectBuffer[i].Height();
				}

				fOffsY = label->m_fHeight - fTotalHeight;
				if (e_UIAlignIsCenterVert(label->m_nAlignment))
				{
					fOffsY /= 2.0f;
				}
			}

			for (int i=0; i < tRects.nNumRects; i++)
			{
				float fOffsX = 0.0f;

				if (e_UIAlignIsCenterHoriz(label->m_nAlignment))
				{
					fOffsX = (label->m_fWidth - tRects.pRectBuffer[i].Width()) / 2.0f;
				}
				if (e_UIAlignIsRight(label->m_nAlignment))
				{
					fOffsX = label->m_fWidth - tRects.pRectBuffer[i].Width();
				}

				if (fOffsY != 0.0f || fOffsX != 0.0f )
					tRects.pRectBuffer[i].Translate(fOffsX, fOffsY);

				UIComponentAddPrimitiveElement(label,
					UIPRIM_BOX, 
					0,
					tRects.pRectBuffer[i],
					0,
					&tClipRect, 
					label->m_dwBkgdRectColor);
			}
		}
	}

	// if this is an editctrl, we need to adjust the caret here, after word-wrapping and before adding the text element
	if (UIComponentIsEditCtrl(label))
	{
		UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(label);
		if (pEditCtrl->m_bHasFocus)
		{
			sUIEditCtrlSetCaret(pEditCtrl);
		}
	}

	if (label->m_Line.HasText())
	{
		if (label->m_eMarqueeMode != MARQUEE_VERT &&
			label->m_eMarqueeMode != MARQUEE_HORIZ)
		{
			label->m_pTextElement = UIComponentAddTextElement(
				label, 
				UIComponentGetTexture(label), 
				font, 
	//			UIComponentGetFontSize(component, font), 
				nFontSize,
				label->m_Line.GetText(),
				label->m_posMarquee, 
				dwColor, 
				&UI_RECT(0.0f, 0.0f, label->m_fWidth, label->m_fHeight), 
				label->m_nAlignment,
				NULL,
				label->m_bDrawDropshadow,
				label->m_nPage);

			if (label->m_bDrawDropshadow)
			{
				label->m_pTextElement->m_dwDropshadowColor = label->m_dwDropshadowColor;
			}

		}

		// init early terminator for slow scroll controls
		if (label->m_bSlowReveal &&	label->m_pTextElement)
		{
			UITextElementSetEarlyTerminator( label->m_pTextElement, label->m_nMaxChars );
		}
		
		if (g_UI.m_bDebugTestingToggle)
		{
			//UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, UI_RECT(0, 0, component->m_fWidth, component->m_fHeight), NULL, NULL, GFXCOLOR_GREEN);
			//UIComponentAddBoxElement(component, 0, 0, component->m_fWidth, component->m_fHeight, NULL, GFXCOLOR_GREEN, 128);
			//UIComponentAddBoxElement(label, 0, 0, label->m_fTextWidth, label->m_fTextHeight, NULL, GFXCOLOR_GREEN, 128);
			UI_ELEMENT_PRIMITIVE *pPrimitive = UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, UI_RECT(0, 0, component->m_fWidth, component->m_fHeight), NULL, NULL, GFXCOLOR_GREEN );
			if (pPrimitive)
			{
				pPrimitive->m_fZDelta -= 0.001f;
				if (label->m_pTextElement)
					label->m_pTextElement->m_fZDelta -= 0.002f;
			}
		}

		if (label->m_eMarqueeMode == MARQUEE_AUTO)
		{
			if (label->m_fTextHeight > label->m_fHeight)
				label->m_eMarqueeMode = MARQUEE_VERT;
			else if (label->m_fTextWidth > label->m_fWidth)
				label->m_eMarqueeMode = MARQUEE_HORIZ;
		}

		if (label->m_eMarqueeMode == MARQUEE_VERT ||
			label->m_eMarqueeMode == MARQUEE_HORIZ)
		{
			if (label->m_dwMarqueeAnimTicket != INVALID_ID)
			{
				CSchedulerCancelEvent(label->m_dwMarqueeAnimTicket);
			}

			// set to the beginning
			label->m_eMarqueeState = MARQUEE_STATE_END_WAIT;
			sLabelAdjustMarquee(AppGetCltGame(), CEVENT_DATA((DWORD_PTR)label), 0);
		}
	}
	else if (pScrollbar)
	{
		UIComponentSetVisible(pScrollbar, FALSE);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UILabelOnAdvancePage(
	UI_COMPONENT* component,
	int nDelta)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETVAL(label, FALSE);

	int nNewPage = nDelta + label->m_nPage;
	if (nNewPage < label->m_nNumPages && nNewPage >= 0)
	{
		label->m_nPage = nNewPage;

		sLabelSetUnitmodeForPage(label);
	
		UI_COMPONENT * pPaintComponent = label;
		if (label->m_pParent &&
			UIComponentIsTooltip(label->m_pParent))
		{
			// this needs to be done to re-arrange the items on this tooltip.  
			pPaintComponent = label->m_pParent;
		}

		UI_SCROLLBAR *pScrollbar = NULL;
		UI_COMPONENT *pFirstChild = label->m_pFirstChild;
		if (pFirstChild && UIComponentIsScrollBar(pFirstChild))
		{
			pScrollbar = UICastToScrollBar(pFirstChild);
			pScrollbar->m_ScrollPos.m_fY = 0.0f;			// reset the scroll
			label->m_ScrollPos.m_fY = 0.0f;			// reset the scroll
		}

		UIComponentHandleUIMessage(pPaintComponent, UIMSG_PAINT, 0, 0); 		

		if (label->m_bSlowReveal)	// needs to go after the paint message so the text element has been updated
		{
			UILabelStartSlowReveal( label );
		}
		
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelSelect(			
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETVAL(label, UIMSG_RET_NOT_HANDLED);

	label->m_bSelected = TRUE;
	UI_COMPONENT *pSibling = label->m_pNextSibling;
	while (pSibling)
	{
		if (UIComponentIsLabel(pSibling))
		{
			UI_LABELEX* pSiblingLabel = UICastToLabel(pSibling);
			pSiblingLabel->m_bSelected = FALSE;
		}
		pSibling = pSibling->m_pNextSibling;
	}
	pSibling = label->m_pPrevSibling;
	while (pSibling)
	{
		if (UIComponentIsLabel(pSibling))
		{
			UI_LABELEX* pSiblingLabel = UICastToLabel(pSibling);
			pSiblingLabel->m_bSelected = FALSE;
		}
		pSibling = pSibling->m_pPrevSibling;
	}

	// repaint this label and all its siblings
	UIComponentHandleUIMessage(label->m_pParent, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UILabelIsSelected(			
	UI_COMPONENT* pComponent)
{
	ASSERT_RETFALSE(UIComponentIsLabel(pComponent));
	UI_LABELEX* pLabel = UICastToLabel(pComponent);

	return pLabel->m_bSelected;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UILabelIsSelected(			
	UI_COMPONENT* pComponent,
	const char *szChildName)
{
	UI_COMPONENT* pComp = UIComponentFindChildByName(pComponent, szChildName);
	if (pComp)
		return UILabelIsSelected(pComp);

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelOnPaintCheckParentButton(			
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_LABELEX* pLabel = UICastToLabel(component);
	ASSERT_RETVAL(pLabel, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT * pParent = pLabel->m_pParent;
	ASSERT_RETVAL(pParent, UIMSG_RET_NOT_HANDLED);

	ASSERT_RETVAL(UIComponentIsButton( pParent ), UIMSG_RET_NOT_HANDLED);
	UI_BUTTONEX * pButton = UICastToButton( pParent );
	ASSERT_RETVAL( pButton, UIMSG_RET_NOT_HANDLED);

	pLabel->m_bSelected = UIButtonGetDown( pButton );
	pLabel->m_bDisabled = !UIComponentGetActive( pButton );

	return UILabelOnPaint( component, msg, wParam, lParam );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UILabelOnMouseHover(			
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_LABELEX* label = UICastToLabel(component);
	ASSERT_RETVAL(label, UIMSG_RET_NOT_HANDLED);

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentGetVisible(component) || !UIComponentCheckBounds(label, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	x -= pos.m_fX;
	y -= pos.m_fY;

	GAME * pGame = AppGetCltGame();
	if(pGame)
	{
		UI_GFXELEMENT *pElement = component->m_pGfxElementFirst;
		while (pElement)
		{
			UNITID idUnit = UIElementGetModelElementUnitID(pElement);
			if (idUnit != INVALID_ID)
			{
				UNIT *pUnit = UnitGetById(pGame, idUnit);
				if (x >= pElement->m_Position.m_fX &&
					x <  pElement->m_Position.m_fX + pElement->m_fWidth &&
					y >= pElement->m_Position.m_fY &&
					y <  pElement->m_Position.m_fY + pElement->m_fHeight)
				{
					UI_RECT rect(pos.m_fX + pElement->m_Position.m_fX,
						pos.m_fY + pElement->m_Position.m_fY,
						pos.m_fX + pElement->m_Position.m_fX + pElement->m_fWidth,
						pos.m_fY + pElement->m_Position.m_fY + pElement->m_fHeight );
					UISetHoverTextItem( &rect, pUnit );
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
			pElement = pElement->m_pNextElement;
		}
	}

	UIClearHoverText();
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIClearTooltips(			
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIClearHoverText();

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITestPrimitives(			
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentRemoveAllElements( component );

	UI_RECT tRect( 0.f, 0.f, 200.f, 400.f );
	UI_ARCSECTOR_DATA tData;
	tData.fAngleStartDeg = 15.f;
	tData.fAngleWidthDeg = 270.f;
	tData.fRingRadiusPct = 0.3f;
	tData.fSegmentWidthDeg = 10.f;


	UI_ELEMENT_PRIMITIVE * pPrim1 = UIComponentAddPrimitiveElement( component, UIPRIM_BOX, 0, tRect );
	ASSERT( pPrim1 );
	pPrim1->m_dwColor = 0xffff0000;
	UI_ELEMENT_PRIMITIVE * pPrim2 = UIComponentAddPrimitiveElement( component, UIPRIM_ARCSECTOR, 0, tRect, &tData );
	ASSERT( pPrim2 );
	pPrim2->m_dwColor = 0xff0000ff;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddAutomapText(
	UI_AUTOMAP *component, 
	UIX_TEXTURE *texture, 
	UI_TEXTURE_FRAME *frame,
	DWORD dwColor,
	const WCHAR *szText,
	float x,
	float y)
{
	if (szText)
	{
		UI_RECT cliprect(0.0f, 0.0f, component->m_fWidth, component->m_fHeight);
		int nAlign = UIALIGN_TOPLEFT;
		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
		int nFontSize = UIComponentGetFontSize(component, pFont);
		float fWidthRatio = 1.0f;
		float flWidth = 0.0f;
		float flHeight = 0.0f;
		UIElementGetTextLogSize(pFont, 
			nFontSize, 
			fWidthRatio, 
			component->m_bNoScaleFont, 
			szText, 
			&flWidth,
			&flHeight,
			TRUE,
			TRUE);

		if (x + (frame->m_fWidth / 2.0f) + flWidth > cliprect.m_fX2)			// if it would go off the right side
		{
			if (x - (frame->m_fWidth / 2.0f) - flWidth < cliprect.m_fX1)		// if it would go off the left side
			{
				if (y - ((frame->m_fHeight / 2.0f) + flHeight) <= cliprect.m_fY1)			// if it would go off the top
				{
					nAlign = UIALIGN_BOTTOM;
				}
				else
				{
					nAlign = UIALIGN_TOP;
				}
			}
			else
			{
				if (y - (flHeight / 2.0f) <= cliprect.m_fY1)					// if it would go off the top
				{
					nAlign = UIALIGN_BOTTOMLEFT;
				}
				else if ((y - (flHeight / 2.0f)) + flHeight > cliprect.m_fY2)	// if it would go off the bottom
				{
					nAlign = UIALIGN_TOPLEFT;
				}
				else
				{
					nAlign = UIALIGN_LEFT;
				}
			}
		}
		else
		{
			if (y - (flHeight / 2.0f) <= cliprect.m_fY1)						// if it would go off the top
			{
				nAlign = UIALIGN_BOTTOMRIGHT;
			}
			else if ((y - (flHeight / 2.0f)) + flHeight > cliprect.m_fY2)		// if it would go off the bottom
			{
				nAlign = UIALIGN_TOPRIGHT;
			}
			else
			{
				nAlign = UIALIGN_RIGHT;
			}
		}

		if (e_UIAlignIsBottom(nAlign))
		{
			y += (frame->m_fHeight / 2.0f);								// go underneath
		}
		else if (nAlign == UIALIGN_TOP ||
					nAlign == UIALIGN_TOPLEFT ||
					nAlign == UIALIGN_TOPRIGHT)
		{
			y -= (frame->m_fHeight / 2.0f) + flHeight;						// go on top
		}
		else
		{
			y -= (flHeight / 2.0f);										// go center vertically
		}

		if (e_UIAlignIsLeft(nAlign))
		{
			x -= (frame->m_fWidth / 2.0f) + flWidth;						// go to the left
		}
		else if (e_UIAlignIsRight(nAlign))
		{
				x += (frame->m_fWidth / 2.0f);								// go to the right
		}
		else
		{
			x = x - (flWidth / 2.0f);										//center it
		}

		UI_GFXELEMENT *pElement = UIComponentAddTextElement(component, 
			texture, 
			pFont, 
			UIComponentGetFontSize(component, pFont), 
			szText, 
			UI_POSITION(x, y), 
			dwColor, 
			&cliprect );

//		UIComponentAddPrimitiveElement(component, UIPRIM_BOX, 0, UI_RECT(x, y, x + flWidth, y + flHeight), NULL, NULL, GFXCOLOR_GREEN);

		if (component->m_pRectOrganize)
		{
			RectOrganizeAddRect(component->m_pRectOrganize, x + (flWidth / 2.0f), y + (flHeight / 2.0f), &pElement->m_Position.m_fX, &pElement->m_Position.m_fY, flWidth, flHeight);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void UISwapCoords(
	float& x1,
	float& y1,
	float& x2,
	float& y2)
{
	float t = x2;
	x2 = x1;
	x1 = t;
	t = y2;
	y2 = y1;
	y1 = t;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
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
	float LineWidth )
{


	if (cliprect && !ClipLine(x1, y1, x2, y2, cliprect->m_fX1, cliprect->m_fY1, cliprect->m_fX2, cliprect->m_fY2))
	{
		return;
	}

	UI_TEXTURERECT rect;

	VECTOR2 linevector(x2-x1, y2-y1);
	VectorNormalize(linevector);
	VECTOR2 sidevector(-linevector.fY, linevector.fX);
	sidevector *= LineWidth;
	rect.x1 = x1 + sidevector.fX;
	rect.y1 = y1 + sidevector.fY;
	rect.x2 = x2 + sidevector.fX;
	rect.y2 = y2 + sidevector.fY;
	rect.x3 = x2 - sidevector.fX;
	rect.y3 = y2 - sidevector.fY;
	rect.x4 = x1 - sidevector.fX;
	rect.y4 = y1 - sidevector.fY;
	if (GetClockDirection(rect.x1, rect.y1, rect.x2, rect.y2, rect.x3, rect.y3) == COUNTER_CLOCKWISE)
	{
		UISwapCoords(rect.x2, rect.y2, rect.x4, rect.y4);
	}

	if (frame->m_nNumChunks > 0)
	{
		rect.u1 = frame->m_pChunks[0].m_fU1;
		rect.v1 = frame->m_pChunks[0].m_fV1;
		rect.u2 = frame->m_pChunks[0].m_fU2;
		rect.v2 = frame->m_pChunks[0].m_fV1;
		rect.u3 = frame->m_pChunks[0].m_fU2;
		rect.v3 = frame->m_pChunks[0].m_fV2;
		rect.u4 = frame->m_pChunks[0].m_fU1;
		rect.v4 = frame->m_pChunks[0].m_fV2;

		UIComponentAddRectElement(component, texture, frame, &rect, color, cliprect);
	}

	UISetNeedToRender(component->m_nRenderSection);
}

void UIComponentDrawRect(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UI_TEXTURE_FRAME* frame,
	float left,
	float top,
	float right,
	float bottom,
	UI_RECT* cliprect,
	DWORD color
	)
{
	if (cliprect && !ClipLine(left, top, right, bottom, cliprect->m_fX1, cliprect->m_fY1, cliprect->m_fX2, cliprect->m_fY2))
	{
		return;
	}

	UI_TEXTURERECT rect;

	rect.x1 = left;
	rect.y1 = top;
	rect.x2 = right;
	rect.y2 = top;
	rect.x3 = right;
	rect.y3 = bottom;
	rect.x4 = left;
	rect.y4 = bottom;
	if (GetClockDirection(rect.x1, rect.y1, rect.x2, rect.y2, rect.x3, rect.y3) == COUNTER_CLOCKWISE)
	{
		UISwapCoords(rect.x2, rect.y2, rect.x4, rect.y4);
	}

	if (frame->m_nNumChunks > 0)
	{
		rect.u1 = frame->m_pChunks[0].m_fU1;
		rect.v1 = frame->m_pChunks[0].m_fV1;
		rect.u2 = frame->m_pChunks[0].m_fU2;
		rect.v2 = frame->m_pChunks[0].m_fV1;
		rect.u3 = frame->m_pChunks[0].m_fU2;
		rect.v3 = frame->m_pChunks[0].m_fV2;
		rect.u4 = frame->m_pChunks[0].m_fU1;
		rect.v4 = frame->m_pChunks[0].m_fV2;

		UIComponentAddRectElement(component, texture, frame, &rect, color, cliprect);
	}
	UISetNeedToRender(component->m_nRenderSection);
}
void UIComponentDrawTri(
						 UI_COMPONENT* component,
						 UIX_TEXTURE* texture,
						 UI_TEXTURE_FRAME* frame,
						 float x1,
						 float y1,
						 float x2,
						 float y2,
						 float x3,
						 float y3,
						 UI_RECT* cliprect,
						 DWORD color
						 )
{
	if ( cliprect &&
		(!ClipLine(x1, y1, x2, y2, cliprect->m_fX1, cliprect->m_fY1, cliprect->m_fX2, cliprect->m_fY2) ||
		 !ClipLine(x2, y2, x3, y3, cliprect->m_fX1, cliprect->m_fY1, cliprect->m_fX2, cliprect->m_fY2) ||
		 !ClipLine(x3, y3, x1, y1, cliprect->m_fX1, cliprect->m_fY1, cliprect->m_fX2, cliprect->m_fY2)))
	{
		return;
	}

	UI_TEXTURETRI tri;

	tri.x1 = x1;
	tri.y1 = y1;
	tri.x2 = x2;
	tri.y2 = y2;
	tri.x3 = x3;
	tri.y3 = y3;
	if (GetClockDirection(tri.x1, tri.y1, tri.x2, tri.y2, tri.x3, tri.y3) == COUNTER_CLOCKWISE)
	{
		UISwapCoords(tri.x1, tri.y1, tri.x3, tri.y3);
	}

	if (frame->m_nNumChunks > 0)
	{
		tri.u1 = frame->m_pChunks[0].m_fU1;
		tri.v1 = frame->m_pChunks[0].m_fV1;
		tri.u2 = frame->m_pChunks[0].m_fU2;
		tri.v2 = frame->m_pChunks[0].m_fV1;
		tri.u3 = frame->m_pChunks[0].m_fU2;
		tri.v3 = frame->m_pChunks[0].m_fV2;
		UIComponentAddTriElement(component, texture, frame, &tri, color, cliprect);
	}
	UISetNeedToRender(component->m_nRenderSection);
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sGetMapIconScale(
	void)
{
	if (AppIsTugboat())
	{
		return MAP_ICON_SCALE_TUGBOAT;
	}
	return MAP_ICON_SCALE_HELLGATE;
}

//----------------------------------------------------------------------------
struct AUTOMAP_DATA
{
	UNIT *pControlUnit;
	UI_AUTOMAP *pAutomap;
	UNITID idLastHeadstone;
	const MATRIX *pmTransformationMatrix;
	float fXOffset;
	float fYOffset;
	UI_RECT *pClipRect;
	UIX_TEXTURE *pTexture;
	float fMonsterRadiusSq;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

/* ***********************
Note: this function is currently unused.
It's intended for use if, when we update automap items, (say, on quest status update),
we want to update a portal or something immediately, that's out of range of the normal update
radius.
*********************** */
void UpdateAutomapItems (
	 GAME* game,
	 UI_AUTOMAP* automap,
	 UNIT* pUnit,
	 AUTOMAP_ITEM *& item
						 )
{
	DWORD dwFlags = 0;

	for(AUTOMAP_ITEM *ii = item; ii; ii = ii->m_pNext)
	{
		if(ii->m_idUnit == pUnit->pUnitData->wCode)
		{
			item->m_vPosition = pUnit->vPosition;
			//item->m_idLevel = LevelGetID(RoomGetLevel(UnitGetRoom(pUnit)));
			if(pUnit && UnitGetRoom(pUnit) && RoomGetLevel(UnitGetRoom(pUnit)) && RoomGetLevel(UnitGetRoom(pUnit))->m_pLevelDefinition)
				item->wCode = RoomGetLevel(UnitGetRoom(pUnit))->m_pLevelDefinition->wCode;
			else
				item->wCode = INVALID_ID;
			item->m_bInHellrift = UnitIsInHellrift(pUnit);
			item->m_nType = pUnit->nUnitType;
			//item->m_pNext = NULL;
			item->m_pIcon = automap->m_pFrameWarpOut;
			UnitGetName(pUnit, item->m_szName, arrsize(item->m_szName), dwFlags);
			item->m_dwColor = GetFontColor(ObjectWarpGetNameColor(pUnit));//Warp specific code
			item->m_bBlocked = ObjectIsBlocking(pUnit);
			break;
		}
	}
}

static void sAddAutomapItem (
	GAME* game,
	UI_AUTOMAP* automap,
	UNIT* pUnit,
	AUTOMAP_ITEM *& item
							 )
{
	DWORD dwFlags = 0;

	item = (AUTOMAP_ITEM*)MALLOC(NULL, sizeof(AUTOMAP_ITEM));
	item->m_idUnit = pUnit->pUnitData->wCode;
	item->m_vPosition = pUnit->vPosition;
	if(pUnit && UnitGetRoom(pUnit) && RoomGetLevel(UnitGetRoom(pUnit)) && RoomGetLevel(UnitGetRoom(pUnit))->m_pLevelDefinition)
		item->wCode = RoomGetLevel(UnitGetRoom(pUnit))->m_pLevelDefinition->wCode;
	else
		item->wCode = INVALID_ID;
	item->m_bInHellrift = UnitIsInHellrift(pUnit);
	item->m_nType = pUnit->nUnitType;
	item->m_pNext = NULL;
	item->m_pIcon = automap->m_pFrameWarpOut;
	UnitGetName(pUnit, item->m_szName, arrsize(item->m_szName), dwFlags);
	item->m_dwColor = GetFontColor(ObjectWarpGetNameColor(pUnit));//Warp specific code
	item->m_bBlocked = ObjectIsBlocking(pUnit);

	if( AppIsTugboat() )	// we just use white text - a warp is a warp.
	{
		item->m_dwColor = GFXCOLOR_WHITE;
	}
}


static ROOM_ITERATE_UNITS sAddUnitToAutomap(
	UNIT *pUnit,
	void *pCallbackData)
{
	GAME *game = UnitGetGame( pUnit );
	BOOL bShow = FALSE;
	BOOL bPlayer = FALSE;
	BOOL bShowName = FALSE;
	BOOL bPet = PetIsPet(pUnit);
	DWORD dwColor = GFXCOLOR_HOTPINK;	//frame default
	const AUTOMAP_DATA *pAutomapData = (const AUTOMAP_DATA *)pCallbackData;
	UI_AUTOMAP *automap = pAutomapData->pAutomap;
	float fScale = sGetMapIconScale();
	UNIT *controlunit = pAutomapData->pControlUnit;
	const MATRIX &mTransformationMatrix = *pAutomapData->pmTransformationMatrix;
	float fXOffset = pAutomapData->fXOffset;
	float fYOffset = pAutomapData->fYOffset;
	UI_RECT &cliprect = *pAutomapData->pClipRect;
	UIX_TEXTURE* texture = pAutomapData->pTexture;

	VECTOR posControlUnit = UnitGetPosition(controlunit);

	int nEngineAutomap = INVALID_ID;
	REGION * pRegion = e_GetCurrentRegion();
	if ( pRegion )
	{
		nEngineAutomap = pRegion->nAutomapID;
	}

	// do not add units that are not visible
	if (UnitHasState( game, pUnit, STATE_HIDDEN ) ||
		c_UnitGetNoDraw( pUnit ) == TRUE)
	{
		return RIU_CONTINUE;
	}
	
	VECTOR4 vec;
	VECTOR pt;
	if (GameGetDebugFlag( game, DEBUGFLAG_AUTOMAP_MONSTERS ))
	{
	
		if (UnitGetTargetType( pUnit ) == TARGET_BAD)
		{
			BOOL bHellrift = UnitGetClass(pUnit) == automap->m_nHellriftClass && automap->m_pFrameHellrift;
			if (!bHellrift && VectorDistanceSquared(UnitGetPosition(pUnit), posControlUnit) > pAutomapData->fMonsterRadiusSq)
			{
				return RIU_CONTINUE;
			}
			if (UnitIsA(pUnit, UNITTYPE_NOTARGET))
			{
				return RIU_CONTINUE;
			}

			float x1, y1, x2, y2;
			sAutomapGetPosition(
				nEngineAutomap,
				fXOffset,
				fYOffset,
				automap->m_fScale,
				pUnit->vPosition,
				controlunit->vPosition,
				mTransformationMatrix,
				x1, y1, x2, y2 );

			BOOL bOK = FALSE;
			if (bHellrift)
			{
				bOK = ClipLine(x1, y1, x2, y2, cliprect.m_fX1, cliprect.m_fY1, cliprect.m_fX2, cliprect.m_fY2);
			}
			else
			{
				bOK = PointInside(x2, y2, cliprect.m_fX1, cliprect.m_fY1, cliprect.m_fX2, cliprect.m_fY2);
			}

			if (bOK)
			{
				if (bHellrift)
				{
					DWORD dwFrameColor = UI_MAKE_COLOR(automap->m_bIconAlpha, automap->m_pFrameHellrift->m_dwDefaultColor);
					UIComponentAddElement(automap, texture, automap->m_pFrameHellrift, UI_POSITION(x2, y2), dwFrameColor, &cliprect, TRUE, fScale, fScale);
					DWORD dwTextColor = UI_MAKE_COLOR(automap->m_bTextAlpha, automap->m_pFrameHellrift->m_dwDefaultColor);
					sAddAutomapText(automap, texture, automap->m_pFrameHellrift, dwTextColor, automap->m_szHellgateString, x2, y2);
				}
				else
				{
					DWORD dwFrameColor = UI_MAKE_COLOR(automap->m_bIconAlpha, automap->m_pFrameMonster->m_dwDefaultColor);
					UIComponentAddElement(automap, texture, automap->m_pFrameMonster, UI_POSITION(x2, y2), dwFrameColor, &cliprect, TRUE, fScale, fScale );
				}
			}
		}
	}
	
	UI_TEXTURE_FRAME *pFrame = automap->m_pFrameMonster;
	if (bPet)
	{
		bShow = !IsUnitDeadOrDying(pUnit);
		if (PetGetOwner(pUnit) != UnitGetId(controlunit))
			bShow = FALSE;
		dwColor = GFXCOLOR_GREEN;
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_STORY_NEW ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestNew;
		dwColor = GetFontColor(FONTCOLOR_STORY_QUEST);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_NEW ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestNew;
		//dwColor = pFrame->m_dwDefaultColor;
		dwColor = AppIsHellgate() ? GetFontColor(FONTCOLOR_LIGHT_YELLOW) : dwColor = pFrame->m_dwDefaultColor;
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_WAITING ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestWaiting;
		dwColor = pFrame->m_dwDefaultColor;
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_STORY_RETURN ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestReturn;
		dwColor = GetFontColor(FONTCOLOR_STORY_QUEST);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_RETURN ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestReturn;
		dwColor = pFrame->m_dwDefaultColor;
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_NEW_RANDOM ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestNew;
		//dwColor = pFrame->m_dwDefaultColor;
		dwColor = GetFontColor(FONTCOLOR_LIGHT_GREEN);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_WAITING_RANDOM ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestWaiting;
		dwColor = GetFontColor(FONTCOLOR_DARK_GREEN);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_RETURN_RANDOM ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestReturn;
		dwColor = GetFontColor(FONTCOLOR_LIGHT_GREEN);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_NEW_CRAFTING ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestNew;
		//dwColor = pFrame->m_dwDefaultColor;
		dwColor = GetFontColor(FONTCOLOR_FSORANGE);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_WAITING_CRAFTING ) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestWaiting;
		dwColor = GetFontColor(FONTCOLOR_FSDARKORANGE);
	}
	else if (UnitHasState( game, pUnit, STATE_NPC_INFO_RETURN_CRAFTING) )
	{
		bShow = TRUE;
		pFrame = automap->m_pFrameQuestReturn;
		dwColor = GetFontColor(FONTCOLOR_FSORANGE);
	}
	else if (UnitIsA(pUnit, UNITTYPE_PLAYER) && pUnit != UIGetControlUnit())
	{
		// don't show ANYBODY in PVP
		if (!PlayerPvPIsEnabled(UIGetControlUnit()))
		{
			// only show party members
			for( int nMember = 0; nMember < MAX_CHAT_PARTY_MEMBERS; nMember++ )
			{			
				if( UnitGetId( pUnit ) == c_PlayerGetPartyMemberUnitId(nMember) )
				{
					bShow = TRUE;
					bPlayer = TRUE;
					dwColor = GFXCOLOR_CYAN;
					if (AppIsHellgate() && UICursorGetActive()) 
					{
						bShowName = TRUE;
					}
				}
			}	
		}
	}
	else
	{
		const UNIT_DATA *pUnitData = UnitGetData(pUnit);
		if (pUnitData)
		{
			if ( AppIsTugboat() && pUnitData->flMaxAutomapRadius != 0.0f)
			{
				if (UnitsGetDistance( pUnit, UnitGetPosition( controlunit ) ) > pUnitData->flMaxAutomapRadius)
				{
					return RIU_CONTINUE;
				}
			}

			if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRAINER) )
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameCrafterNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_RESPECCER) || 
					  UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_CRAFTING_RESPECCER))
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameTrainerNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_MAP_VENDOR) )
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameMapVendorNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRADESMAN) )
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameCrafterNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GAMBLER) )
			{
				bShow = TRUE;
				pFrame = automap->m_pGamblerNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GUILDMASTER) )
			{
				bShow = TRUE;
				pFrame = automap->m_pGuildmasterNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_DUNGEON_WARPER) )
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameGuideNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_TRANSPORTER) )
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameGuideNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_MERCHANT))
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameVendorNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
			else if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_HEALER))
			{
				bShow = TRUE;
				pFrame = automap->m_pFrameHealerNPC;
				dwColor = pFrame->m_dwDefaultColor;
			}
		}
	}
	if (bShow)
	{
				
		float x1, y1, x2, y2;
		sAutomapGetPosition(
			nEngineAutomap,
			fXOffset,
			fYOffset,
			automap->m_fScale,
			pUnit->vPosition,
			controlunit->vPosition,
			mTransformationMatrix,
			x1, y1, x2, y2 );

		//if (ClipLine(x1, y1, x2, y2, cliprect.m_fX1, cliprect.m_fY1, cliprect.m_fX2, cliprect.m_fY2) || bPlayer)
		{
			dwColor = UI_MAKE_COLOR(automap->m_bIconAlpha, dwColor);
			//RadialClipLine(x1, y1, x2, y2, (cliprect.m_fX2-cliprect.m_fX1)/2 - 12);
			if(!UIComponentGetActiveByEnum(UICOMP_AUTOMAP_BIG))
				RadialClipLine((float)x1, (float)y1, (float)x2, y2, AppIsHellgate() ? AUTOMAP_RADIUS : AUTOMAP_RADIUS_TUGBOAT );
			UIComponentAddElement(automap, texture, pFrame, UI_POSITION(x2, y2), dwColor, &cliprect, FALSE, fScale, fScale );

			if (bShowName)
			{
				WCHAR uszName[128];					
				DWORD dwFlags = 0;
				UnitGetName(pUnit, uszName, arrsize(uszName), dwFlags);
				sAddAutomapText(automap, texture, automap->m_pFrameWarpOut, dwColor, uszName, x2, y2);
			}
		}
	}
	
	// headstones
	else if (UnitGetId( pUnit ) == pAutomapData->idLastHeadstone && UnitIsGhost( controlunit ))
	{
		VECTOR pt;

		DWORD dwColor = GFXCOLOR_HOTPINK;	//frame default
		UI_TEXTURE_FRAME *pFrame = automap->m_pFrameMonster;		
		pFrame = automap->m_pFrameGrave ? automap->m_pFrameGrave : pFrame;
		dwColor = pFrame->m_dwDefaultColor;		
	
		float x1, y1, x2, y2;
		sAutomapGetPosition(
			nEngineAutomap,
			fXOffset,
			fYOffset,
			automap->m_fScale,
			pUnit->vPosition,
			controlunit->vPosition,
			mTransformationMatrix,
			x1, y1, x2, y2 );

		BOOL addElements = TRUE;
		if(!UIComponentGetActiveByEnum(UICOMP_AUTOMAP_BIG))
			RadialClipLine(x1, y1, x2, y2, AppIsHellgate() ? AUTOMAP_RADIUS : AUTOMAP_RADIUS_TUGBOAT );
		else
		{
			if(!ClipLine(x1, y1, x2, y2, cliprect.m_fX1, cliprect.m_fY1, cliprect.m_fX2, cliprect.m_fY2))
				addElements = FALSE;
		}
		if(addElements)
		{
			dwColor = UI_MAKE_COLOR(automap->m_bIconAlpha, dwColor);
			UI_GFXELEMENT *pElement = UIComponentAddElement(automap, texture, pFrame, UI_POSITION(x2, y2), dwColor, &cliprect, TRUE, fScale, fScale );
			if (pElement)
				pElement->m_bGrayOut = FALSE;		// never gray out the headstone
		}
	
	}

	//warps	- this needs to run regardless of whether the map is up or not.  What's the best way to do this? =/
	else if ( ( ObjectIsWarp( pUnit ) ||
		   UnitIsA( pUnit, UNITTYPE_MAPTRIGGER) ) )
	{
		if( AppIsTugboat() && c_UnitGetNoDraw( pUnit ) ) // skip non-drawn portals
		{
			return RIU_CONTINUE;
		}
		const UNIT_DATA *pUnitData = UnitGetData( pUnit );
		
		// check for a max radius
		
		if (pUnitData->flMaxAutomapRadius != 0.0f)
		{
			if (UnitsGetDistance( pUnit, UnitGetPosition( controlunit ) ) > pUnitData->flMaxAutomapRadius)
			{
				return RIU_CONTINUE;
			}
		}

		// dont show 0 collision radius warps
		if ( pUnitData->fCollisionRadius == 0.0f )
		{
			return RIU_CONTINUE;
		}

		if(UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_AUTOMAP_SAVE ))
		{
			AUTOMAP_ITEM* addPoint = NULL;

			for(AUTOMAP_ITEM* ii = automap->m_pAutoMapItems; ii; ii = ii->m_pNext)
			{
				DWORD dwFlags = 0;
				if(ii->m_idUnit == pUnit->pUnitData->wCode)
				{
					UnitGetName(pUnit, ii->m_szName, arrsize(ii->m_szName), dwFlags);
					ii->m_dwColor = GetFontColor(ObjectWarpGetNameColor(pUnit));//Warp specific code
					ii->m_bBlocked = ObjectIsBlocking(pUnit);
					addPoint = NULL;
					break;
				}
				addPoint = ii;
			}

			if(!automap->m_pAutoMapItems)
			{
				sAddAutomapItem(game, automap, pUnit, automap->m_pAutoMapItems);
			}
			else if (addPoint)
			{
				sAddAutomapItem(game, automap, pUnit, addPoint->m_pNext);
			}

		}
		//draw items that are non-static
		else
		{
			float x1, y1, x2, y2;
			sAutomapGetPosition(
				nEngineAutomap,
				fXOffset,
				fYOffset,
				automap->m_fScale,
				pUnit->vPosition,
				controlunit->vPosition,
				mTransformationMatrix,
				x1, y1, x2, y2 );

			BOOL addElements = TRUE;
			if(!UIComponentGetActiveByEnum(UICOMP_AUTOMAP_BIG))
				RadialClipLine(x1, y1, x2, y2, AppIsHellgate() ? AUTOMAP_RADIUS : AUTOMAP_RADIUS_TUGBOAT );
			else
			{
				if(!ClipLine(x1, y1, x2, y2, cliprect.m_fX1, cliprect.m_fY1, cliprect.m_fX2, cliprect.m_fY2))
					addElements = FALSE;
			}
			if(addElements)
			{
				BOOL bBlocked = ObjectIsBlocking(pUnit);
				DWORD dwColor = GetFontColor(ObjectWarpGetNameColor(pUnit));
				if( AppIsTugboat() )	// we just use white text - a warp is a warp.
				{
					dwColor = GFXCOLOR_WHITE;
				}
				DWORD dwFrameColor = UI_MAKE_COLOR(automap->m_bIconAlpha, dwColor);
				if( AppIsHellgate() )
				{
					UIComponentAddElement(automap, texture, automap->m_pFrameWarpOut, UI_POSITION(x2, y2), dwFrameColor, &cliprect, TRUE, fScale, fScale);
				}
				else
				{
					if( UnitIsA( pUnit, UNITTYPE_MAPTRIGGER) ||
						UnitIsA( pUnit, UNITTYPE_WARP_NO_UI))
					{
						UIComponentAddElement(automap, texture, automap->m_pFrameMapExit, UI_POSITION(x2, y2), dwFrameColor, &cliprect, TRUE, fScale, fScale);
					}
					else
					{
						UIComponentAddElement(automap, texture, automap->m_pFrameWarpOut, UI_POSITION(x2, y2), dwFrameColor, &cliprect, TRUE, fScale, fScale);
					}
				}

				if (!bBlocked)
				{
					WCHAR uszWarpName[128];					
					DWORD dwFlags = 0;
					UnitGetName(pUnit, uszWarpName, arrsize(uszWarpName), dwFlags);
					// DWORD dwTextColor = UI_MAKE_COLOR(automap->m_bTextAlpha, dwColor);
					sAddAutomapText(automap, texture, automap->m_pFrameWarpOut, dwFrameColor, uszWarpName, x2, y2);
				}

			}
		}
		
		

	}

		
	return RIU_CONTINUE;
	
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAutomapAddRoom(
	GAME* game,
	UIX_TEXTURE* texture,		
	UNIT *controlunit,
	UI_AUTOMAP* automap,
	ROOM *room,
	float fMonsterRadiusSq,
	const MATRIX &mTransformationMatrix,
	float fXOffset,
	float fYOffset,
	UI_RECT &cliprect)
{

	// ignore rooms that are not on the same sublevel as the control unit
	if (RoomGetSubLevelID( room ) != RoomGetSubLevelID( UnitGetRoom( controlunit ) ))
	{
		return;
	}
	
	// setup callback data for adding units to map
	AUTOMAP_DATA tAutomapData;
	tAutomapData.pControlUnit = controlunit;
	tAutomapData.pAutomap = automap;
	tAutomapData.idLastHeadstone = UnitGetStat( controlunit, STATS_NEWEST_HEADSTONE_UNIT_ID );
	tAutomapData.pmTransformationMatrix = &mTransformationMatrix;
	tAutomapData.fXOffset = fXOffset;
	tAutomapData.fYOffset = fYOffset;
	tAutomapData.pClipRect = &cliprect;
	tAutomapData.pTexture = texture;
	tAutomapData.fMonsterRadiusSq = fMonsterRadiusSq;
		
	// add units to automap, note we're checking a lot of target types here because we sometimes
	// want objects, dead monsters, alive npcs, etc
	TARGET_TYPE eTargetTypes[] = { TARGET_NONE, TARGET_GOOD, TARGET_BAD, TARGET_OBJECT, TARGET_DEAD, TARGET_INVALID };
	RoomIterateUnits( room, eTargetTypes, sAddUnitToAutomap, &tAutomapData );		
	
}

int cmpAutomap(const void* n1, const void* n2)
{
	const AUTOMAP_NODE** m1 = (const AUTOMAP_NODE**) (n1);
	const AUTOMAP_NODE** m2 = (const AUTOMAP_NODE**) (n2);
	if((*m1)->time < (*m2)->time)
		return -1;
	else if((*m1)->time == (*m2)->time)
		return 0;
	else
		return 1;
}

static float fAutoMapScales[] =
{
	7.5f,
	5.0f,
	3.0f,
	2.0f,
	1.5f,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutoMapOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_AUTOMAP* automap = UICastToAutoMap(component);
	ASSERT_RETVAL(automap, UIMSG_RET_NOT_HANDLED);

	UIX_TEXTURE* texture = UIComponentGetTexture(component);
	if (!texture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float fExtra = AppIsHellgate() ? 48.0f : 0.0f;
	UI_RECT cliprect(-fExtra, -fExtra, automap->m_fWidth * UIGetScreenToLogRatioX( automap->m_bNoScale ) + fExtra, automap->m_fHeight * UIGetScreenToLogRatioY( automap->m_bNoScale ) + fExtra);
	UI_RECT cliprectlog(0.0f, 0.0f, automap->m_fWidth , automap->m_fHeight );
	float fXOffset = automap->m_fWidth / 2.0f;
	float fYOffset = automap->m_fHeight / 2.0f;
	
	UI_TEXTURE_FRAME* frame = automap->m_pFrame;
	if (!frame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	GAME* game = AppGetCltGame();
	if (!game)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	UNIT* unit = GameGetControlUnit(game);
	if (!unit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	ROOM* room = UnitGetRoom(unit);
	if (!room)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	LEVEL* level = RoomGetLevel(room);
	if (!level)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	BOOL bNewMap = FALSE;

	int nLevelID = LevelGetID(level);
	if (automap->m_nLastLevel != nLevelID)
	{
		UI_COMPONENT *pLevelLabel = UIComponentGetByEnum(UICOMP_AUTOMAP_LEVEL_NAME);
		if (pLevelLabel)
		{
			const WCHAR *puszLevelName = L"";
			const WCHAR *puszDRLGName = L"";
			WCHAR szText[256] = L"";
			if (level->m_pLevelDefinition->nLevelDisplayNameStringIndex)
			{
				puszLevelName = StringTableGetStringByIndex( level->m_pLevelDefinition->nLevelDisplayNameStringIndex );
				PStrCat(szText, puszLevelName, arrsize(szText));

				const DRLG_DEFINITION * pDRLGDefinition = RoomGetDRLGDef( room );
				ASSERT( pDRLGDefinition );
				if ( pDRLGDefinition->nDRLGDisplayNameStringIndex != INVALID_LINK )
				{
					puszDRLGName = StringTableGetStringByIndex( pDRLGDefinition->nDRLGDisplayNameStringIndex );
					if (puszDRLGName && puszDRLGName[0])
					{
						PStrCat(szText, L"\n(", arrsize(szText));
						PStrCat(szText, puszDRLGName, arrsize(szText));
						PStrCat(szText, L")", arrsize(szText));
					}
				}
			}

			UILabelSetText(pLevelLabel, szText);
		}

		UIComponentRemoveAutoMapItems(automap);

		automap->m_nLastLevel = nLevelID;
		bNewMap = TRUE;
	}
		
	BOOL bIsSafe = LevelIsSafe( level ) || ( AppIsTugboat() && LevelIsTown( level ) );
	

	const CAMERA_INFO* camera = c_CameraGetInfo(TRUE);
	if (!camera)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	VECTOR eyevec(0.0f, 0.0f, 0.0f);
	VECTOR upvec(0.0f, 0.0f, -1.0f);
	VECTOR facing;

	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_AUTOMAP_ROTATE) == 0 )
		facing = VECTOR( 0.0f, -1.0f, 0.0f );
	else
		VectorDirectionFromAngleRadians(facing, CameraInfoGetAngle(camera));

	facing.fZ = 0.0f;
	if (facing.fX == 0.0f && facing.fY == 0.0f)
	{
		facing.fY = -1.0f;
	}
	VectorNormalize(facing);

	float fFacingAngle = CameraInfoGetAngle(camera);
	/*
	bmanegold: Otherwise, this causes problems if other things, like pet or partner locations change
	Could be fixed w/ moving code and a custom delete, but too much trouble for the benefit			
	BOOL bSomethingChanged = (automap->m_vLastPosition != unit->vPosition ||
		automap->m_vLastFacing != facing ||
		automap->m_fLastFacingAngle != fFacingAngle);
	
	if (AppIsTugboat())
	{
		bSomethingChanged |= !(e_GetUICovered() );
	}


	if (!bSomethingChanged)
	{
		// Nothing has changed - move on
		return UIMSG_RET_HANDLED;
	}
	*/

	UIComponentRemoveAllElements(automap);

	if (automap->m_pRectOrganize)
	{
		RectOrganizeClear(automap->m_pRectOrganize);
	}

	if( AppIsTugboat() &&
		(e_GetUICovered() ))
	{
		automap->m_vLastPosition = VECTOR( -99,-99,-99 );
		return UIMSG_RET_HANDLED;
	}


	if (UIComponentGetVisible(automap))
	{
		// Don't reset these unless the automap is visible.  That way when it is made visible again we'll be sure to update
		automap->m_vLastPosition = unit->vPosition;
		automap->m_vLastFacing = facing;
		automap->m_fLastFacingAngle = fFacingAngle;
	}

	float fRotate = PI / 2.0f;

	MATRIX mTransformationMatrix;
	MatrixIdentity(&mTransformationMatrix);
	if (AppIsTugboat())
    {


		{
			VECTOR vDirection = UnitGetFaceDirection( unit, TRUE );//CameraInfoGetDirection(camera);
			vDirection.fZ = 0;
			VectorNormalize( vDirection, vDirection );
			fFacingAngle = (float)atan2f( -vDirection.fX, vDirection.fY);
			LEVEL* pLevel = UnitGetLevel( unit );
			if( pLevel && pLevel->m_pLevelDefinition->fAutomapWidth != 0 )
			{
				fFacingAngle -= fRotate;
				MatrixRotationAxis(mTransformationMatrix, upvec, PI );
			}
			else
			{
				float fLevelRotation = DegreesToRadians( pLevel->m_pLevelDefinition->fFixedOrientation );
				fRotate += fLevelRotation;
				fFacingAngle -= fRotate * 1.5f;
				MatrixRotationAxis(mTransformationMatrix, upvec, PI * -.75f + fLevelRotation );
			}



		}
    }
    else
    {
	    MatrixRotationAxis(mTransformationMatrix, upvec, VectorToAngle(eyevec, facing) + fRotate);
    }


	// Pass display settings to the engine automap(s).
	int nEngineAutomap = INVALID_ID;
	REGION * pRegion = e_GetCurrentRegion();
	if ( pRegion )
	{
		nEngineAutomap = pRegion->nAutomapID;
	}


	V( e_AutomapSetShowAll( nEngineAutomap, bIsSafe ) );


	if (UIComponentGetVisible(automap))
	{
		if( AppIsTugboat() )
		{
			// overworld specific scale
			if( level->m_pLevelDefinition->fAutomapWidth != 0 )
			{
				automap->m_fScale = fAutoMapScales[automap->m_nZoomLevel] * .25f;
			}
			else
			{
				automap->m_fScale = fAutoMapScales[automap->m_nZoomLevel];
			}

		}

		// Sets the display settings for the automaps from every region.
		V( e_AutomapSetDisplay(
			NULL,
			&mTransformationMatrix,
			&automap->m_fScale ) );
	}



	//const float fRevealDistance = AppIsHellgate() ? 1200.0f : 200.0f;
	//ROOM *pRoomControlUnit = UnitGetRoom( unit );
	room = NULL;
	//ROOM* next = level->m_pRooms;
	static int max_time = 0, min_time = 0xf000;
	static unsigned int discovery = 0;
	static VECTOR lastPos = VECTOR(0, 0, 0);

	if (UIComponentGetVisible(automap))
	{

#ifdef ENABLE_NEW_AUTOMAP

		UI_POSITION tPos( 0, 0 );
		const float fLogRatioX = UIGetScreenToLogRatioX(automap->m_bNoScale);
		const float fLogRatioY = UIGetScreenToLogRatioY(automap->m_bNoScale);
		UIComponentAddCallbackElement( automap, tPos, automap->m_fWidth * fLogRatioX , automap->m_fHeight * fLogRatioY, automap->m_fZDelta, automap->m_fScale, &cliprect );
	
#endif	// ENABLE_NEW_AUTOMAP
	}
	if( AppIsTugboat() )
	{
		LEVEL* pLevel = UnitGetLevel( unit );
		if( pLevel && pLevel->m_pLevelDefinition->fAutomapWidth != 0 )
		{
			UI_RECT cliprectbackground(4.0f, 4.0f, automap->m_fWidth -4.0f, automap->m_fHeight - 4.0f);
			UI_COMPONENT *pChild = component->m_pFirstChild;
			int nPlayerArea = LevelGetLevelAreaID( pLevel );
			int nPlayerZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nPlayerArea, NULL );
			MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION* pLevelZone = (MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_ZONES, nPlayerZone );
			UIX_TEXTURE* pMapTexture = (UIX_TEXTURE*)StrDictionaryFind(g_UI.m_pTextures, pLevelZone->pszImageName);
			ASSERT_RETVAL(pMapTexture, UIMSG_RET_HANDLED);
			UI_TEXTURE_FRAME* pMapFrame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pMapTexture->m_pFrames, pLevelZone->pszAutomapFrame);
			ASSERT_RETVAL(pMapFrame, UIMSG_RET_HANDLED);
			
			VECTOR vWorldPosition( 0, 0, 0);
			float x1, y1, x2, y2;
			sAutomapGetPosition(
				nEngineAutomap,
				fXOffset,
				fYOffset,
				automap->m_fScale,
				vWorldPosition,
				unit->vPosition,
				mTransformationMatrix,
				x1, y1, x2, y2 );

			vWorldPosition = VECTOR( pLevel->m_pLevelDefinition->fAutomapWidth, pLevel->m_pLevelDefinition->fAutomapHeight, 0);
			float x3, y3, x4, y4;
			sAutomapGetPosition(
				nEngineAutomap,
				fXOffset,
				fYOffset,
				automap->m_fScale,
				vWorldPosition,
				unit->vPosition,
				mTransformationMatrix,
				x3, y3, x4, y4 );
			float fWidth = x2 - x4;
			float fHeight = y2 - y4;
			UI_POSITION pos( x2, y2 );
			float fScaleX = fWidth / pMapFrame->m_fWidth;
			float fScaleY = fHeight / pMapFrame->m_fHeight;
			// we actually add this to the parent holder of the actual worldmap
			UI_GFXELEMENT* pElement = UIComponentAddElement(pChild, pMapTexture, pMapFrame, pos, GFXCOLOR_WHITE, &cliprectbackground, FALSE, fScaleX, fScaleY );
			ASSERT_RETVAL(pElement, UIMSG_RET_NOT_HANDLED);
		}
	}
	

	// paint monsters ...and other dynamic units
	if (UIComponentGetVisible(automap) || TRUE)
	{
		float holy_radius = UnitGetHolyRadius( unit );

		float fMonsterRadius = MAX(automap->m_fMonsterDistance, holy_radius);
		if (fMonsterRadius > 0.0f)
		{
			float fMonsterRadiusSq = fMonsterRadius * fMonsterRadius;

			if (automap->m_pFrameMonster)
			{
				LEVEL* pLevel = UnitGetLevel( unit );			
				for (ROOM* room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ))
				{
					sAutomapAddRoom( 
						game,
						texture,
						unit, 
						automap, 
						room, 
						fMonsterRadiusSq, 
						mTransformationMatrix,
						fXOffset,
						fYOffset,
						cliprectlog);
				}
			}
		}
	}

	// point of interest code
	if( AppIsTugboat() )
	{
		UI_COMPONENT* pPOIPool = UIComponentGetByEnum(UICOMP_AUTOMAP_POIS);
		UI_COMPONENT* pCurrentPOI = NULL;
		pCurrentPOI = pPOIPool->m_pFirstChild;


		if(  MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( UnitGetLevelAreaID( unit ) ) || LevelIsTown( UnitGetLevel( unit ) ) )
		{	
			MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfInterest= PlayerGetPointsOfInterest( unit );
			for( int t = 0; t < pPointsOfInterest->GetPointOfInterestCount(); t++ )
			{
				if( pPointsOfInterest->HasFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_Display ) )
				{
					const MYTHOS_POINTSOFINTEREST::PointOfInterest *pPointOfInterest = pPointsOfInterest->GetPointOfInterestByIndex( t );
					if( pPointOfInterest )
					{
						
						VECTOR vPosition( (float)pPointOfInterest->nPosX, (float)pPointOfInterest->nPosY, 0 );
						if( !pPointsOfInterest->HasFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_IsQuestMarker ) )
						{
							float fDistance = VectorDistanceXY( vPosition, unit->vPosition );
							if( fDistance > pPointOfInterest->pUnitData->flMaxAutomapRadius  )
							{
								continue;
							}
						}

						float x1, y1, x2, y2;
						sAutomapGetPosition( 
							nEngineAutomap,
							fXOffset,
							fYOffset,
							automap->m_fScale,
							vPosition,
							unit->vPosition,
							mTransformationMatrix,
							x1, y1, x2, y2 );

						if(!UIComponentGetActiveByEnum(UICOMP_AUTOMAP_BIG))
							RadialClipLine(x1, y1, x2, y2, AUTOMAP_RADIUS_TUGBOAT );
						
						x2 -= pCurrentPOI->m_fWidth * .5f;
						y2 -= pCurrentPOI->m_fHeight * .5f;

						UIComponentActivate( pCurrentPOI, TRUE );
						pCurrentPOI->m_Position = UI_POSITION(x2, y2);
						if( !pCurrentPOI->m_szTooltipText )
						{
							pCurrentPOI->m_szTooltipText = (WCHAR*)MALLOCZ(NULL, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH * sizeof(WCHAR));
						}
						const WCHAR *szTooltip = StringTableGetStringByIndex( pPointOfInterest->nStringId );
						PStrCopy(pCurrentPOI->m_szTooltipText, szTooltip, UI_MAX_SIMPLE_TOOLTIP_TEXT_LENGTH);
						UIComponentHandleUIMessage(pCurrentPOI, UIMSG_PAINT, 0, 0);
						pCurrentPOI = pCurrentPOI->m_pNextSibling;

					}
				}					
			}
			

		}

		// now hide any remaining bars that weren't used
		while( pCurrentPOI )
		{
			UIComponentSetVisible( pCurrentPOI, FALSE );
			pCurrentPOI = pCurrentPOI->m_pNextSibling;
		}
	}

	float fScale = (AppIsTugboat() ? 0.5f : 1.0f);

	// paint items - static (non-dynamic) units
	if (UIComponentGetVisible(automap))
	{
		//Need to check to make sure that the icon exists before we draw it...
		AUTOMAP_ITEM* item = automap->m_pAutoMapItems;
		while (item)
		{
			if (level && level->m_pLevelDefinition && item->wCode == (level->m_pLevelDefinition->wCode) && item->m_bInHellrift == UnitIsInHellrift(GameGetControlUnit(game)))
			{
				float x1, y1, x2, y2;
				sAutomapGetPosition(
					nEngineAutomap,
					fXOffset,
					fYOffset,
					automap->m_fScale,
					item->m_vPosition,
					unit->vPosition,
					mTransformationMatrix,
					x1, y1, x2, y2 );

				BOOL addElements = TRUE;
				if(!UIComponentGetActiveByEnum(UICOMP_AUTOMAP_BIG))
					RadialClipLine(x1, y1, x2, y2, AppIsHellgate() ? AUTOMAP_RADIUS : AUTOMAP_RADIUS_TUGBOAT );
				else
				{
					if(!ClipLine(x1, y1, x2, y2, cliprect.m_fX1, cliprect.m_fY1, cliprect.m_fX2, cliprect.m_fY2))
						addElements = FALSE;
				}
				if(addElements)
				{
					DWORD dwColor = item->m_dwColor;
					//VECTOR tvec(x2 - fXOffset, y2 - fYOffset, 0.0f);
					// float angle = VectorToAngle(VECTOR(0.0f, 0.0f, 0.0f), tvec);
					DWORD dwFrameColor = UI_MAKE_COLOR(automap->m_bIconAlpha, item->m_dwColor);
					UIComponentAddElement(component, texture, item->m_pIcon /*this needs to be dynamic*/, UI_POSITION(x2, y2), dwFrameColor, NULL, TRUE, fScale, fScale );
					
					if (!item->m_bBlocked) 
					{
						sAddAutomapText(automap, texture, item->m_pIcon, dwColor, item->m_szName, x2, y2);
					}
				}
			}
			item = item->m_pNext;
		}
	}

	// paint hero
	if (UIComponentGetVisible(automap))
	{
		if (automap->m_pFrameHero)
		{
		
			// window space position of marker
			UI_POSITION posMarkerCenter( fXOffset, fYOffset );
			
			// screen space position of marker
			UI_POSITION posMarkerCenterScreen = UIComponentGetAbsoluteLogPosition(component);
			posMarkerCenterScreen.m_fX += posMarkerCenter.m_fX;
			posMarkerCenterScreen.m_fY += posMarkerCenter.m_fY;
		
			//UIComponentAddElement(
			//	component, 
			//	texture, 
			//	automap->m_pFrameHero, 
			//	posMarkerCenter, 
			//	automap->m_pFrameHero->dwDefaultColor, 
			//	NULL, 
			//	TRUE);

			// get orientation of marker
			float flMarkerAngle = fFacingAngle;
			
			// account for orientation of map on
			flMarkerAngle += fRotate;

			// setup rotation flags
			DWORD dwFlags = 0;
			SETBIT( dwFlags, ROTATION_FLAG_ABOUT_SELF_BIT );
			SETBIT( dwFlags, ROTATION_FLAG_DROPSHADOW_BIT );
			
			DWORD dwFrameColor = UI_MAKE_COLOR(automap->m_bIconAlpha, automap->m_pFrameHero->m_dwDefaultColor);
			if( AppIsTugboat() )
			{
				DWORD dwFlagsb = 0;
				SETBIT( dwFlagsb, ROTATION_FLAG_ABOUT_SELF_BIT );
				// draw marker at orientation angle
				posMarkerCenter.m_fX *= UIGetScreenToLogRatioX(automap->m_bNoScale);
				posMarkerCenter.m_fY *= UIGetScreenToLogRatioY(automap->m_bNoScale);
				UIComponentAddRotatedElement(
					component, 
					texture,
					automap->m_pFrameHero,
					posMarkerCenter,
					dwFrameColor,
					flMarkerAngle,
					posMarkerCenterScreen,
					NULL,
					TRUE,
					1.0f,
					dwFlagsb);
			}
			else if(( pGlobal->dwFlags & GLOBAL_FLAG_AUTOMAP_ROTATE) != 0 )
			{

				// draw marker at orientation angle
				UIComponentAddElement(
					component, 
					texture,
					automap->m_pFrameHero,
					posMarkerCenter,
					dwFrameColor,
					NULL,
					TRUE,
					fScale,
					fScale);

			}
			else
			{

				// draw marker at orientation angle
				UIComponentAddRotatedElement(
					component, 
					texture,
					automap->m_pFrameHero,
					posMarkerCenter,
					dwFrameColor,
					flMarkerAngle,
					posMarkerCenterScreen,
					NULL,
					TRUE,
					fScale,
					dwFlags);
			}
				
		}
		
	}

	// Make sure none of the labels overlap
	if (UIComponentGetVisible(automap) && automap->m_pRectOrganize)
	{
		RectOrganizeCompute(automap->m_pRectOrganize, 0, 0, automap->m_fWidth, automap->m_fHeight, 30);
	}

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAutoMapDisplayUpdate(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	PERF(AUTOMAP_UPDATE);

	UI_COMPONENT *pAutomap = NULL;
	UI_COMPONENT *pAutomapBig = NULL;
	if (game)
	{
		const UNIT * pControlUnit = GameGetControlUnit( game );
		if ( pControlUnit )
		{
			// Sets the display settings for the automaps from every region.
			V( e_AutomapSetDisplay(
				&pControlUnit->vPosition,
				NULL,
				NULL ) );
		}

		pAutomap = UIComponentGetByEnum(UICOMP_AUTOMAP);
		if (pAutomap)
		{
			UIComponentHandleUIMessage(pAutomap, UIMSG_PAINT, 0, 0);
		}
		pAutomapBig = UIComponentGetByEnum(UICOMP_AUTOMAP_BIG);
		if (pAutomapBig)
		{
			UIComponentHandleUIMessage(pAutomapBig, UIMSG_PAINT, 0, 0);
		}
	}

	if (g_UI.m_dwAutomapUpdateTicket != INVALID_ID)
	{
		CSchedulerCancelEvent(g_UI.m_dwAutomapUpdateTicket);
	}

	if (UIComponentGetVisible(pAutomap) || UIComponentGetVisible(pAutomapBig))
	{
#ifdef ENABLE_NEW_AUTOMAP
		if ( AppIsHellgate() )
		{
			g_UI.m_dwAutomapUpdateTicket = CSchedulerRegisterEventImm(UIAutoMapDisplayUpdate, CEVENT_DATA());
		}
		else
#endif
		{
			g_UI.m_dwAutomapUpdateTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 50, UIAutoMapDisplayUpdate, CEVENT_DATA());
		}
	}
	else
	{
		g_UI.m_dwAutomapUpdateTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + 1000, UIAutoMapDisplayUpdate, CEVENT_DATA());
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAutoMapBeginUpdates(
	void)
{
	//if (g_UI.m_dwAutomapUpdateTicket != INVALID_ID)
	//{
	//	CSchedulerCancelEvent(g_UI.m_dwAutomapUpdateTicket);
	//}
	//UI_COMPONENT * component = UIComponentGetByEnum( UICOMP_AUTOMAP );
	//g_UI.m_dwAutomapUpdateTicket = CSchedulerRegisterEventImm(UIAutoMapDisplayUpdate, CEVENT_DATA((DWORD_PTR)component));
	UIAutoMapDisplayUpdate(AppGetCltGame(), CEVENT_DATA(), 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UICycleAutomap()
{
	UI_COMPONENT * pAutomap = UIComponentGetByEnum( UICOMP_AUTOMAP );

	if (!pAutomap)
		return;

	DWORD dwCurTick = GameGetTick(AppGetCltGame());
	DWORD dwLastTick = 0;

	UI_AUTOMAP * pAutomapActual = NULL;
	if (!UIComponentIsAutoMap(pAutomap))
	{
		UI_COMPONENT *pComp = UIComponentIterateChildren(pAutomap, NULL, UITYPE_AUTOMAP, TRUE);
		if (!pComp)
			return;

		pAutomapActual = UICastToAutoMap(pComp);
	}
	else
	{
		pAutomapActual = UICastToAutoMap(pAutomap);
	}

	ASSERT_RETURN(pAutomapActual);

	dwLastTick = pAutomapActual->m_dwCycleLastChange;
	pAutomapActual->m_dwCycleLastChange = dwCurTick;

	
	UI_COMPONENT * pAutomapBig = UIComponentGetByEnum( UICOMP_AUTOMAP_BIG );

	if (UIComponentGetVisible(pAutomap))
	{
		//UIComponentHandleUIMessage(pAutomap, UIMSG_INACTIVATE, 0, 0);
		UIComponentActivate(pAutomap, FALSE);
		pAutomap->m_bUserActive = FALSE;

		if (dwCurTick - dwLastTick > GAME_TICKS_PER_SECOND * 5)
		{
			// if the small one's been up for a while, just close it
			return;
		}

		if (pAutomapBig)
		{
			// cycle to the big automap
			//UIComponentHandleUIMessage(pAutomapBig, UIMSG_ACTIVATE, 0, 0);
			UIComponentActivate(pAutomapBig, TRUE);
			if (!UIComponentIsAutoMap(pAutomapBig))
			{
				UI_COMPONENT *pComp = UIComponentIterateChildren(pAutomap, NULL, UITYPE_AUTOMAP, TRUE);
				if (!pComp)
					return;

				pAutomapActual = UICastToAutoMap(pComp);
			}
			else
			{
				pAutomapActual = UICastToAutoMap(pAutomapBig);
			}
		}
	}
	else
	{
		if (pAutomapBig && UIComponentGetVisible(pAutomapBig))
		{
			// the big automap is up.  Close it
			//UIComponentHandleUIMessage(pAutomapBig, UIMSG_INACTIVATE, 0, 0);
			UIComponentActivate(pAutomapBig, FALSE);

			return;
		}
		else
		{
			// neither is up.  Open the small one.
			//UIComponentHandleUIMessage(pAutomap, UIMSG_ACTIVATE, 0, 0);
			UIComponentActivate(pAutomap, TRUE);

			pAutomap->m_bUserActive = TRUE;
		}
	}

	// do this to reset whether the automap needs to redraw
	if (pAutomapActual)
	{
		pAutomapActual->m_vLastFacing.fX += 1000;
		pAutomapActual->m_nLastLevel = INVALID_ID;	// repaint level name
	}

	//need to update immediately
	UIAutoMapBeginUpdates();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutoMapOnActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	
	if (ResultIsHandled( UIPanelOnActivate(component, msg, wParam, lParam)) )
	{
		
		
		UIAutoMapBeginUpdates();
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAutoMapZoom( int zoom,
				    BOOL bForce /* = FALSE */ )
{


	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_AUTOMAP);
	if (!component)
		return;

	if (!UIComponentIsAutoMap(component))
	{
		component = UIComponentIterateChildren(component, NULL, UITYPE_AUTOMAP, TRUE);
	}
	if (!component)
		return;

	UI_AUTOMAP* automap = UICastToAutoMap(component);
	if (!automap)
	{
		return;
	}
	if ( !bForce && !UIComponentGetVisible(automap))
	{
		return;
	}
	int nNumLevels = arrsize(fAutoMapScales)-1;
	automap->m_nZoomLevel += zoom;
	automap->m_nZoomLevel = PIN(automap->m_nZoomLevel, 0, nNumLevels);
	automap->m_fScale = fAutoMapScales[automap->m_nZoomLevel];
	
	GAMEOPTIONS Options;
	GameOptions_Get(Options);
	Options.fAutomapScale = automap->m_fScale;
	Options.nAutomapZoomLevel = automap->m_nZoomLevel;
	GameOptions_Set(Options);
	

	if( AppIsHellgate() )
	{
		UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_AUTOMAP);
		UI_COMPONENT *pButton = NULL;
		while ((pButton = UIComponentIterateChildren(pComp, pButton, UITYPE_BUTTON, TRUE)) != NULL)
		{
			if (pButton->m_dwParam == 1)
				UIComponentSetActive(pButton, automap->m_nZoomLevel != 0);
			if (pButton->m_dwParam == 0)
				UIComponentSetActive(pButton, automap->m_nZoomLevel != nNumLevels);
		}
	}

	// Update just the scale.
	V( e_AutomapSetDisplay( NULL, NULL, &automap->m_fScale ) );
	// Now that the zoom has changed, mark for a new intermediate render.  We may turn off MSAA.
	V( e_AutomapsMarkDirty() );

	// do this to reset whether the automap needs to redraw
	automap->m_vLastFacing.fX += 1000;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutomapZoomButtonOnClick(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UIAutoMapZoom(component->m_dwParam == 1 ? -1 : 1);

	return UIMSG_RET_HANDLED_END_PROCESS;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIAutomapPanelOnPostCreate(
									  UI_COMPONENT* component,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	GAMEOPTIONS tOptions;
	structclear(tOptions);
	GameOptions_Get(tOptions);
	UIMapSetAlpha((BYTE)tOptions.nMapAlpha);
	return UIMSG_RET_HANDLED_END_PROCESS;
	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void* UISetReadingWindowStrings(
	void* pArg)
{
	UI_COMPONENT* pReadingWnd = UIComponentGetByEnum(UICOMP_IME_READING);
	ASSERT_RETNULL(pReadingWnd != NULL);	

	LPCWSTR strReading = (LPCWSTR)pArg;
	if (strReading == NULL || strReading[0] == L'\0') {
		pReadingWnd->m_bVisible = FALSE;
	} else {
		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pReadingWnd);
		int nBaseFontSize = UIComponentGetFontSize(pReadingWnd);

		float fTextHeight = 0.0f;
		UIElementGetTextLogSize(pFont, nBaseFontSize, 1.0f, pReadingWnd->m_bNoScaleFont, strReading, NULL, &fTextHeight);
		fTextHeight *= UIGetScreenToLogRatioY( pReadingWnd->m_bNoScaleFont );
		pReadingWnd->m_fHeight = fTextHeight;

		UILabelSetText(pReadingWnd, strReading);
		pReadingWnd->m_bVisible = TRUE;
	}
	UIComponentHandleUIMessage(pReadingWnd, UIMSG_PAINT, 0, 0);
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void* UISetCandidateWindowStrings(
	void* pArg)
{
	UI_COMPONENT* pCandidateWnd = UIComponentGetByEnum(UICOMP_IME_CANDIDATE);
	ASSERT_RETNULL(pCandidateWnd != NULL);	

	LPCWSTR* strCandidates = (LPCWSTR*)pArg;
	if (strCandidates == NULL || strCandidates[0] == NULL) {
		pCandidateWnd->m_bVisible = FALSE;
	} else {
		UI_TEXTBOX* pTxtBox = UICastToTextBox(pCandidateWnd);
		UITextBoxClear(pCandidateWnd);

		UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pCandidateWnd);
		int nBaseFontSize = UIComponentGetFontSize(pCandidateWnd);

		float max_width = 0;
		UINT32 i;
		for (i = 0; strCandidates[i] != NULL; i++) {
			UITextBoxAddLine(pTxtBox, strCandidates[i]);

			float cur_width = 0.0f;
			UIElementGetTextLogSize(pFont, nBaseFontSize, 1.0f, pTxtBox->m_bNoScaleFont, strCandidates[i], &cur_width, NULL, TRUE, TRUE);
			cur_width = (float)FLOOR(cur_width) + 1;
			max_width = MAX(max_width, cur_width);
		}
		pCandidateWnd->m_bVisible = TRUE;
		
		float fTextHeight = 0.0f;
		UIElementGetTextLogSize(pFont, nBaseFontSize, 1.0f, pCandidateWnd->m_bNoScaleFont, strCandidates[0], NULL, &fTextHeight);
		pCandidateWnd->m_fHeight = fTextHeight * (i+1);
		pCandidateWnd->m_fWidth = max_width;
		pCandidateWnd->m_Position.m_fY = pCandidateWnd->m_pParent->m_fHeight;

		float ratio = UIGetScreenToLogRatioY(pCandidateWnd->m_bNoScale);
		float dy = AppCommonGetWindowHeight() - (UIComponentGetAbsoluteLogPosition(pCandidateWnd).m_fY / ratio + pCandidateWnd->m_fHeight);
		if (dy < 0) {
			pCandidateWnd->m_Position.m_fY += dy;
		}
	}
	UIComponentHandleUIMessage(pCandidateWnd, UIMSG_PAINT, 0, 0);
	return NULL;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPanelOnActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if ( ResultIsHandled(UIComponentOnActivate(component, msg, wParam, lParam)) )
	{
		//UI_PANELEX *pPanel = UICastToPanel(component);
		//// activate the current tab
		//UI_PANELEX *pTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
		//if (pTab)
		//{
		//	UIComponentHandleUIMessage(pTab, UIMSG_ACTIVATE, 0, 0);
		//}
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_PANELEX * UIPanelGetTab(
	UI_PANELEX *pPanel,
	int nTab)
{
	ASSERT_RETFALSE(pPanel);
	if (nTab < 0)
		return NULL;

	UI_COMPONENT * pChild = pPanel->m_pFirstChild;
	while (pChild)
	{
		if ( UIComponentIsPanel(pChild)  )
		{
			UI_PANELEX *pChildPanel = UICastToPanel(pChild);
			if (pChildPanel->m_nTabNum == nTab)
			{
				return pChildPanel;
			}
		}
		pChild = pChild->m_pNextSibling;
	}

	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIPanelSetTab(
	UI_PANELEX *pPanel,
	int nTab)
{
	ASSERT_RETFALSE(pPanel);

	if (pPanel->m_nCurrentTab != nTab)
	{
		// Deactivate the old one
		UI_PANELEX *pOldTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
		UI_PANELEX *pNewTab = UIPanelGetTab(pPanel, nTab);
		if (pOldTab && pNewTab)
		{
			UIComponentHandleUIMessage(pOldTab, UIMSG_INACTIVATE, 0, 0);
			UIComponentSetVisible(pOldTab, FALSE);
		}
		// TRAVIS: I have cases where I have a master panel that has no sub-panels,
		// but still needs to change tab indices for the benefit of radio buttons.
		// hopefully this isn't breaking anything- I'm allowing the tab change
		// even if a new tab doesn't exist
		pPanel->m_nCurrentTab = nTab;
		// Activate the new one
		if (pNewTab)
		{
			UIComponentHandleUIMessage(pNewTab, UIMSG_ACTIVATE, 0, 0);
			
			UIComponentHandleUIMessage(pPanel, UIMSG_PAINT, 0, 0);
			return TRUE;
		}
		else
		{
			UIComponentHandleUIMessage(pPanel, UIMSG_PAINT, 0, 0);
			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIPanelOnInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (ResultIsHandled( UIComponentOnInactivate(component, msg, wParam, lParam) ))
	{
		UI_PANELEX *pPanel = UICastToPanel(component);
		// activate the current tab
		UI_PANELEX *pTab = UIPanelGetTab(pPanel, pPanel->m_nCurrentTab);
		if (pTab)
		{
			UIComponentHandleUIMessage(pTab, UIMSG_INACTIVATE, 0, 0);
		}

		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sKeyIsDown(
	void)
{
	BYTE Keys[256];
	GetKeyboardState((PBYTE)Keys);

	for (int i=0; i<arrsize(Keys); ++i)
	{
		if ((Keys[i] & 0x80) && i != VK_LBUTTON && i != VK_RBUTTON)
		{
			return TRUE;
		}
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIEditCtrlSetCaret(
	UI_EDITCTRL* pEditCtrl)
{
	UIX_TEXTURE_FONT* font = UIComponentGetFont(pEditCtrl);
	ASSERT_RETURN(font);
	int nFontSize = UIComponentGetFontSize(pEditCtrl, font);

	int nLen = 0;
	if (pEditCtrl->m_Line.HasText())
	{
		nLen = PStrLen(pEditCtrl->m_Line.GetText());
	}

	pEditCtrl->m_nCaretPos = PIN(pEditCtrl->m_nCaretPos, 0, nLen);

	int nAlignment = UIALIGN_TOPLEFT;
	if (e_UIAlignIsCenterVert(pEditCtrl->m_nAlignment))
	{
		nAlignment = UIALIGN_LEFT;
	}
	else if (e_UIAlignIsBottom(pEditCtrl->m_nAlignment))
	{
		nAlignment = UIALIGN_BOTTOMLEFT;
	}

	// Find the position of the caret and draw it
	UI_POSITION posCaret;
	if (pEditCtrl->m_Line.HasText())
	{
		// linefeeds for wordwrap should already have been added in the paint call
		const WCHAR *pWC = pEditCtrl->m_Line.GetText();
		int nPos = 0;
		int nLine = 0;
		int nLastNewline = 0;
		int nNextNewline = 0;
		int nCaretPos = pEditCtrl->m_nCaretPos;
		while (pWC && *pWC && nPos < nCaretPos)
		{
			if (*pWC == L'\n')
			{
				nLine++;
				nLastNewline = nPos+1;
			}
			else if (*pWC == L'\r')
			{
//				nLine++;
				nLastNewline = nPos+1;
				nCaretPos++;
			}
			nPos++;
			pWC++;
		}

		if (e_UIAlignIsRight(pEditCtrl->m_nAlignment))
		{
			int nPos2 = nPos;
			while (pWC && *pWC)
			{
				if (*pWC == L'\n' || *pWC == L'\r')
				{
					nNextNewline = nPos2;
					break;
				}
				nPos2++;
				pWC++;
			}
		}

		if (nNextNewline == 0)
			nNextNewline = nLen;

		BOOL bAlignRight = e_UIAlignIsRight(pEditCtrl->m_nAlignment);
		float fTextWidth = 0.0f;
		const WCHAR *szText = pEditCtrl->m_Line.GetText();
		UIElementGetTextLogSize(font, 
			nFontSize, 
			1.0f, 
			pEditCtrl->m_bNoScaleFont, 
			bAlignRight ? &(szText[nPos]) : &(szText[nLastNewline]),
			&fTextWidth,
			NULL,
			TRUE, 
			TRUE,
			0, 
			bAlignRight ? nNextNewline - nPos : nPos - nLastNewline);

		//--------------------------------------------------------
		// Set the caret X position
		//--------------------------------------------------------

		posCaret.m_fX = fTextWidth;

		if (!pEditCtrl->m_bNoScale)	// if the edit is noscale the element's position will not be scaled so
			//   we won't need to do this
		{
			posCaret.m_fX *= UIGetScreenToLogRatioX(pEditCtrl->m_bNoScaleFont);
		}

		if (e_UIAlignIsRight(pEditCtrl->m_nAlignment))
		{
			posCaret.m_fX = pEditCtrl->m_fWidth - posCaret.m_fX;
		}
		if (e_UIAlignIsCenterHoriz(pEditCtrl->m_nAlignment))
		{
			posCaret.m_fX += (pEditCtrl->m_fWidth - fTextWidth) / 2.0f;
		}

		if (pEditCtrl->m_fTextWidth <= pEditCtrl->m_fWidth)
		{
			pEditCtrl->m_posMarquee.m_fX = 0.0f;
		}
		else
		{
			if (posCaret.m_fX + pEditCtrl->m_posMarquee.m_fX <= 0.0f)
			{
				pEditCtrl->m_posMarquee.m_fX = -posCaret.m_fX;
				posCaret.m_fX = 0.0f;
			}
			else if (posCaret.m_fX + pEditCtrl->m_posMarquee.m_fX > pEditCtrl->m_fWidth)
			{
				 pEditCtrl->m_posMarquee.m_fX = MAX(pEditCtrl->m_fWidth - posCaret.m_fX, 0.0f);
				 posCaret.m_fX += pEditCtrl->m_posMarquee.m_fX;
			}
			else
			{
				posCaret.m_fX += pEditCtrl->m_posMarquee.m_fX;
			}
		}

		pEditCtrl->m_CaretPos.m_fX = posCaret.m_fX;

		// adjust the position (per-font) so that the actual line is drawn right at the position we want it (most useful for fixed-width fonts)
		posCaret.m_fX -= (e_UIFontGetCharPreWidth(font, nFontSize, 1.0f, L'|') + 0.5f);

		//--------------------------------------------------------
		// Set the caret Y position
		//--------------------------------------------------------

		float fCaretBot = 0.0f;
		UIElementGetTextLogSize(font, 
			nFontSize, 
			1.0f, 
			pEditCtrl->m_bNoScaleFont, 
			pEditCtrl->m_Line.GetText(),
			NULL,
			&fCaretBot,
			TRUE, 
			FALSE,
			0, 
			nPos);

		float fLineSize = 0.0f;
		UIElementGetTextLogSize(font, 
			nFontSize, 
			1.0f, 
			pEditCtrl->m_bNoScaleFont, 
			L"|",
			NULL,
			&fLineSize,
			TRUE, 
			TRUE,
			0, 
			1);

		const float fCaretTop = fCaretBot - fLineSize;

		posCaret.m_fY = fCaretTop;

		UI_LABELEX *pLabel = UICastToLabel(pEditCtrl);
		if (pLabel->m_pScrollbar)
		{
			const float MIN_DELTA = 0.01f;

			const float fCaretBotRel = fCaretBot - pLabel->m_ScrollPos.m_fY;
			const float fCaretTopRel = fCaretTop - pLabel->m_ScrollPos.m_fY;

			if (fCaretBotRel - pEditCtrl->m_fHeight > MIN_DELTA)
			{
				if (!sKeyIsDown())
				{
					return;
				}
				pLabel->m_ScrollPos.m_fY = fCaretBot - pEditCtrl->m_fHeight;

				// fail-safe check to avoid any future stack overflows
				if (pLabel->m_ScrollPos.m_fY - pLabel->m_pScrollbar->m_fMax > MIN_DELTA)
				{
					ASSERT(pLabel->m_ScrollPos.m_fY - pLabel->m_pScrollbar->m_fMax <= MIN_DELTA);
					pLabel->m_pScrollbar->m_fMax = pLabel->m_ScrollPos.m_fY;
				}

				UIScrollBarSetValue(pLabel->m_pScrollbar, pLabel->m_ScrollPos.m_fY);
			}
			else if (fCaretTopRel < -MIN_DELTA)
			{
				if (!sKeyIsDown())
				{
					return;
				}
				pLabel->m_ScrollPos.m_fY = fCaretTop;

				// fail-safe check to avoid any future stack overflows
				if (pLabel->m_ScrollPos.m_fY - pLabel->m_pScrollbar->m_fMax > MIN_DELTA)
				{
					ASSERT(pLabel->m_ScrollPos.m_fY - pLabel->m_pScrollbar->m_fMax <= MIN_DELTA);
					pLabel->m_pScrollbar->m_fMax = pLabel->m_ScrollPos.m_fY;
				}

				UIScrollBarSetValue(pLabel->m_pScrollbar, pLabel->m_ScrollPos.m_fY);
			}
		}

		pEditCtrl->m_CaretPos.m_fY = posCaret.m_fY;
	}
	else
	{
		if (e_UIAlignIsRight(pEditCtrl->m_nAlignment))
			posCaret.m_fX = pEditCtrl->m_fWidth;
		if (e_UIAlignIsCenterHoriz(pEditCtrl->m_nAlignment))
			posCaret.m_fX = pEditCtrl->m_fWidth/2;
	}

	UI_GFXELEMENT *pCaret = UIComponentAddTextElement(pEditCtrl, NULL, font, nFontSize, L"|", posCaret, pEditCtrl->m_dwColor, NULL, nAlignment);
	UIElementStartBlink(pCaret, -1, 500, pEditCtrl->m_dwColor);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (!UIComponentGetVisible(component))
		return UIMSG_RET_NOT_HANDLED;

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);
	ASSERT_RETVAL(pEditCtrl, UIMSG_RET_NOT_HANDLED);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(component);
	if (!font)
		return UIMSG_RET_NOT_HANDLED;

	UIComponentRemoveAllElements(pEditCtrl);

	WCHAR	*temp = pEditCtrl->m_Line.EditText();
	BOOL	bTextReplaced = FALSE;

	int nLen = 0;
	if (pEditCtrl->m_Line.HasText())
	{
		nLen = PStrLen(temp);
		if (nLen)
		{
			// Check to see if we need to replace password characters
			if (pEditCtrl->m_bPassword)
			{
				WCHAR *szTempText = MALLOC_NEWARRAY(NULL, WCHAR, nLen+1);
				szTempText[0] = 0;
				for (int i = 0; i < nLen; i++)
				{
					PStrCat(szTempText, L"*", (nLen+1));
				}
				pEditCtrl->m_Line.SetTextPtr(szTempText);
				bTextReplaced = TRUE;
			}
		}
	}

	int nFontSize = UIComponentGetFontSize(component, font);

	//right-align if the entered text is wider than the box
	int nOldAlign = pEditCtrl->m_nAlignment;
	if (pEditCtrl->m_bHasFocus && 
		!e_UIAlignIsRight(pEditCtrl->m_nAlignment) &&
		!pEditCtrl->m_bWordWrap)
	{
		float fWidthRatio = 1.0f;
		float fTextWidth = 0.0f;
		if (pEditCtrl->m_Line.HasText())
		{
			UIElementGetTextLogSize(font, nFontSize, fWidthRatio, pEditCtrl->m_bNoScaleFont, pEditCtrl->m_Line.GetText(), &fTextWidth, NULL, TRUE, TRUE);
			fTextWidth = (float)FLOOR(fTextWidth + 0.5f);
			if (!pEditCtrl->m_bNoScale)
			{
				fTextWidth *= UIGetScreenToLogRatioX(pEditCtrl->m_bNoScaleFont);
			}
		}

		if (fTextWidth > pEditCtrl->m_fWidth)
		{
			if (e_UIAlignIsTop(nOldAlign))			pEditCtrl->m_nAlignment = UIALIGN_TOPRIGHT;
			if (e_UIAlignIsBottom(nOldAlign))		pEditCtrl->m_nAlignment = UIALIGN_BOTTOMRIGHT;
			if (e_UIAlignIsCenterVert(nOldAlign))	pEditCtrl->m_nAlignment = UIALIGN_RIGHT;
		}
	}

	UILabelOnPaint(pEditCtrl, UIMSG_PAINT, 0, 0);

	if (bTextReplaced)
	{
		FREE_DELETE_ARRAY(NULL, pEditCtrl->m_Line.EditText(), WCHAR);
		pEditCtrl->m_Line.SetTextPtr(temp);
	}
	pEditCtrl->m_nAlignment = nOldAlign;

	return UIMSG_RET_HANDLED;
}

void UIEditPasteString(
	   const WCHAR *string )
{
	if( !string )
		return;
	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_CONSOLE_EDIT);
	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);
	int count(0);
	//why send it via each character? Mostly because there are so many gotcha's via length, special characters and such.
	while( string[count] != L'\0' &&
		   count < pEditCtrl->m_nMaxLen)
	{
		UIEditCtrlOnKeyChar( component,
							 UIMSG_KEYCHAR,
							 (DWORD)string[count],
							 0 );
		count++;

	}	    
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEditCtrlRemoveChar(
	UI_EDITCTRL *pEdit,
	BOOL bPrev)
{
	ASSERT_RETFALSE(pEdit);
	int nLen = (pEdit->m_Line.HasText())? PStrLen(pEdit->m_Line.GetText()) : 0;
	pEdit->m_nCaretPos = PIN(pEdit->m_nCaretPos, 0, nLen);
	WCHAR*	szEditText = pEdit->m_Line.EditText();
	int		nExtraDeletions = 0;

	for (int i=0; i < nLen; i++)
	{
		if (i >= pEdit->m_nCaretPos - bPrev)
		{
			// KCK: Special case for deleting hypertext objects
			if (szEditText[i] == CTRLCHAR_HYPERTEXT_END)
			{
				// Find and remove the hypertext object
				int k = i-1;
				while (k >= 0)
				{
					if (szEditText[k] == CTRLCHAR_HYPERTEXT)
					{
						UI_HYPERTEXT*	pHypertext = UIFindHypertextById(szEditText[k+1]);
						pEdit->m_Line.m_HypertextList.FindAndDelete(pHypertext);
						nExtraDeletions += (i-k);
						pEdit->m_nCaretPos -= nExtraDeletions;
						break;
					}
					k--;
				}
			}
			szEditText[i-nExtraDeletions] = szEditText[i+1];
		}
	}

	if (pEdit->m_nCaretPos > 0)
		pEdit->m_nCaretPos -= bPrev;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEditCtrlInsertChar(
	UI_EDITCTRL *pEdit,
	WCHAR wc)
{
	ASSERT_RETFALSE(pEdit);
	int nLen = (pEdit->m_Line.HasText()) ? PStrLen(pEdit->m_Line.GetText()) : 0;
	pEdit->m_nCaretPos = PIN(pEdit->m_nCaretPos, 0, nLen);
	WCHAR*	szEditText = pEdit->m_Line.EditText();

	for (int i=nLen+1; i >= 0; i--)
	{
		if (i == pEdit->m_nCaretPos)
		{
			szEditText[i] = wc;
		}
		if (i > pEdit->m_nCaretPos)
		{
			szEditText[i] = szEditText[i-1];
		}
	}

	if (pEdit->m_nCaretPos < nLen && wc !='\n')
	{
		float fWidth = e_UIFontGetCharWidth(UIComponentGetFont(pEdit), UIComponentGetFontSize(pEdit), 1.0f, wc);
		//pEdit->m_posMarquee.m_fX = MAX(pEdit->m_posMarquee.m_fX - fWidth, 0.0f);
		pEdit->m_posMarquee.m_fX += fWidth;
	}
	pEdit->m_nCaretPos++;


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnKeyUp(
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

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);
	if (!pEditCtrl->m_bHasFocus)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (pEditCtrl->m_bEatsAllKeys)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (wParam == VK_BACK ||
		wParam == VK_TAB ||
		wParam == VK_LEFT||
		wParam == VK_RIGHT ||
		wParam == VK_UP ||
		wParam == VK_DOWN ||
		wParam == VK_DELETE ||
		wParam == VK_HOME ||
		wParam == VK_END)
	{	
		// eat these, cause they're handled by the char message so we don't want other handlers to get them
		return UIMSG_RET_HANDLED_END_PROCESS;
	}
	else if (PStrIsPrintable((WCHAR)wParam))
	{
		if (PStrChr( pEditCtrl->m_szDisallowChars, (WCHAR)wParam))
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		else if (pEditCtrl->m_szOnlyAllowChars[0] && !PStrChr( pEditCtrl->m_szOnlyAllowChars, (WCHAR)wParam))
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (wParam == VK_RETURN && pEditCtrl->m_bMultiline)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}



	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnKeyDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_MSG_RETVAL eResult = UIMSG_RET_NOT_HANDLED;
	if (!UIComponentGetActive(component))
	{
		return eResult;
	}

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);
	if (!pEditCtrl->m_bHasFocus)
	{
		return eResult;
	}

	int nLen = 0;
	if (pEditCtrl->m_Line.HasText())
	{
		nLen = PStrLen(pEditCtrl->m_Line.GetText());
	}

	if (pEditCtrl->m_Line.HasText() && pEditCtrl->m_bMultiline)
	{
//		if (nLen)
//		{
//			pEditCtrl->m_Line.Resize(nLen+1);
//		}

		UILabelOnPaint(pEditCtrl, UIMSG_PAINT, 0, 0);

		if (pEditCtrl->m_bHasFocus)
		{
			sUIEditCtrlSetCaret(pEditCtrl);
		}
	}

	switch (wParam)
	{
		case VK_LEFT:
		case VK_RIGHT:
		{
			const WCHAR *szText = pEditCtrl->m_Line.GetText();
			int		nDelta = (wParam == VK_RIGHT ? 1 : -1);
			// KCK: Special case for skipping over hypertext tags
			if (szText[pEditCtrl->m_nCaretPos] == CTRLCHAR_HYPERTEXT && nDelta > 0)
			{
				while (szText[pEditCtrl->m_nCaretPos] && szText[pEditCtrl->m_nCaretPos] != CTRLCHAR_HYPERTEXT_END )
					pEditCtrl->m_nCaretPos++;
			}
			pEditCtrl->m_nCaretPos += nDelta;
			pEditCtrl->m_nCaretPos = PIN(pEditCtrl->m_nCaretPos, 0, nLen);
			// KCK: Special case for skipping over hypertext tags
			if (szText[pEditCtrl->m_nCaretPos] == CTRLCHAR_HYPERTEXT_END && nDelta < 0)
			{
				while (pEditCtrl->m_nCaretPos > 0 && szText[pEditCtrl->m_nCaretPos] != CTRLCHAR_HYPERTEXT)
					pEditCtrl->m_nCaretPos--;
			}
			eResult = UIMSG_RET_HANDLED_END_PROCESS;
			break;
		}
		case VK_UP:
		{
			if (pEditCtrl->m_Line.HasText() &&
				pEditCtrl->m_bMultiline &&
				pEditCtrl->m_nCaretPos > 0)
			{
				// re-position the caret at the same position on the next line ... easy!
				float fLineSize = sUIEditCtrlGetLineSize(pEditCtrl);
				sUIEditCtrlPositionCaretAt(pEditCtrl, pEditCtrl->m_CaretPos.m_fX, pEditCtrl->m_CaretPos.m_fY - fLineSize);

				eResult = UIMSG_RET_HANDLED_END_PROCESS;
			}
			break;
		}
		case VK_DOWN:
		{
			if (pEditCtrl->m_Line.HasText() &&
				pEditCtrl->m_bMultiline)
			{
				// re-position the caret at the same position on the next line ... easy!
				float fLineSize = sUIEditCtrlGetLineSize(pEditCtrl);
				sUIEditCtrlPositionCaretAt(pEditCtrl, pEditCtrl->m_CaretPos.m_fX, pEditCtrl->m_CaretPos.m_fY + fLineSize);

				eResult = UIMSG_RET_HANDLED_END_PROCESS;
			}
			break;
		}
		case VK_HOME:
		{
			if (pEditCtrl->m_bMultiline)
			{
				const WCHAR *szText = pEditCtrl->m_Line.GetText();
				for (int i=pEditCtrl->m_nCaretPos-1; i >=0; i--)
				{
					if (i == 0)
					{
						pEditCtrl->m_nCaretPos = 0;
					}
					if (szText[i] == L'\n' || szText[i] == L'\r')
					{
						pEditCtrl->m_nCaretPos = i+1;
						break;
					}
				}
			}
			else
			{
				pEditCtrl->m_nCaretPos = 0;
			}
			eResult = UIMSG_RET_HANDLED_END_PROCESS;
			break;
		}
		case VK_END:
		{
			if (pEditCtrl->m_bMultiline)
			{
				const WCHAR *szText = pEditCtrl->m_Line.GetText();
				for (int i=pEditCtrl->m_nCaretPos; i <= nLen; i++)
				{
					if (i == nLen)
					{
						pEditCtrl->m_nCaretPos = i;
					}
					if (szText[i] == L'\n' || szText[i] == L'\r')
					{
						pEditCtrl->m_nCaretPos = i;
						break;
					}
				}
			}
			else
			{
				pEditCtrl->m_nCaretPos = nLen;
			}
			eResult = UIMSG_RET_HANDLED_END_PROCESS;
			break;
		}
	};

	if (wParam == VK_DELETE)
	{
		sEditCtrlRemoveChar(pEditCtrl, FALSE);
		eResult = UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (eResult == UIMSG_RET_HANDLED_END_PROCESS)
	{
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}

	if (wParam == VK_RETURN && pEditCtrl->m_bMultiline)
	{
		eResult = UIMSG_RET_HANDLED_END_PROCESS;
	}

	if (wParam == VK_BACK ||
		wParam == VK_TAB)
	{	
		// eat these, cause they're handled by the char message so we don't want other handlers to get them
		eResult = UIMSG_RET_HANDLED_END_PROCESS;
	}
	else if (PStrIsPrintable((WCHAR)wParam))
	{
		if (PStrChr( pEditCtrl->m_szDisallowChars, (WCHAR)wParam))
		{
			eResult = UIMSG_RET_NOT_HANDLED;
		}
		else if (pEditCtrl->m_szOnlyAllowChars[0] && !PStrChr( pEditCtrl->m_szOnlyAllowChars, (WCHAR)wParam))
		{
			eResult = UIMSG_RET_NOT_HANDLED;
		}
		else
		{
			eResult = UIMSG_RET_HANDLED_END_PROCESS;
		}
	}

	if (pEditCtrl->m_bEatsAllKeys)
	{
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	return eResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnKeyChar(
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

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);
	if (!pEditCtrl->m_bHasFocus)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	int nLen = (pEditCtrl->m_Line.HasText() ? PStrLen(pEditCtrl->m_Line.GetText()) : 0);
	if (wParam == VK_BACK)
	{
		sEditCtrlRemoveChar(pEditCtrl, TRUE);
	}
	else if (wParam == VK_TAB)
	{
		UI_EDITCTRL *pNextEdit = pEditCtrl;
		while (pNextEdit)
		{
			// late binding (replace this with UI_COMP_LATE_POINTER
			if (!pNextEdit->m_pNextTabCtrl && pNextEdit->m_szNextTabCtrlName[0])
			{
				pNextEdit->m_pNextTabCtrl = UIComponentFindChildByName(pNextEdit->m_pParent, pNextEdit->m_szNextTabCtrlName);
			}

			if (pNextEdit->m_pNextTabCtrl && UIComponentIsEditCtrl(pNextEdit->m_pNextTabCtrl))
			{
				pNextEdit = UICastToEditCtrl(pNextEdit->m_pNextTabCtrl);
				if (UIComponentGetActive(pNextEdit))
				{
					break;
				}
			}
			else
			{
				pNextEdit = NULL;
			}
		}

		//BOOL bShift = (GetKeyState(VK_SHIFT) & 0x8000);
		//UI_COMPONENT *pTabCtrl = (bShift ? pEditCtrl->m_pPrevTabCtrl : pEditCtrl->m_pNextTabCtrl);

		if (pNextEdit)
		{
			UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)pNextEdit->m_idComponent);
		}
	}
	else if (PStrIsPrintable((WCHAR)wParam) || (WCHAR)wParam == VK_RETURN)
	{
		if (nLen >= pEditCtrl->m_nMaxLen)
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		if (PStrChr( pEditCtrl->m_szDisallowChars, (WCHAR)wParam))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		if (pEditCtrl->m_szOnlyAllowChars[0] && !PStrChr( pEditCtrl->m_szOnlyAllowChars, (WCHAR)wParam))
		{
			return UIMSG_RET_HANDLED_END_PROCESS;
		}

		if ((WCHAR)wParam == VK_RETURN)
		{
			if (!pEditCtrl->m_bMultiline)
				return UIMSG_RET_NOT_HANDLED;
			else
				wParam = (DWORD)L'\n';
		}

		if (!pEditCtrl->m_Line.HasText())
		{
			pEditCtrl->m_Line.Resize(pEditCtrl->m_nMaxLen);
			nLen = 0;
			pEditCtrl->m_nCaretPos = 0;
		}

		sEditCtrlInsertChar(pEditCtrl, (WCHAR)wParam);
	}

	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);

	// KCK: There is a possibility that the wordwrapping code will add a '\n'
	// during the paint step (for example, if a line ends with a '-'). Check
	// for this by seeing if the previous character is a '\n' and adjust.
	const WCHAR * szText = pEditCtrl->m_Line.GetText();
	if (pEditCtrl->m_nCaretPos > 0 && szText[pEditCtrl->m_nCaretPos-1] == '\n')
		pEditCtrl->m_nCaretPos++;

	// CHB 2007.08.02 - Note to Brennan: I was able to reproduce this
	// using a 1680 x 1050 resolution. The problem is that in this
	// case, m_fTextHeight is marginally higher than m_fHeight, perhaps
	// due to floating-point precision issues. (Or it could be a problem
	// selecting the right-sized font for the control?) In any case,
	// the text height would end up slightly higher than the control's
	// height, causing the code below to always remove the character
	// just entered.
	if (pEditCtrl->m_bLimitTextToSpace &&
		(pEditCtrl->m_fTextWidth > pEditCtrl->m_fWidth || 
//		pEditCtrl->m_fTextHeight > pEditCtrl->m_fHeight))
//		(pEditCtrl->m_fTextHeight - pEditCtrl->m_fHeight) > 0.001f)) 
		(pEditCtrl->m_fTextHeight - pEditCtrl->m_fHeight) > 0.5f)) // MJK: 9/25/2007 - Had to make the fudge factor even bigger for 2560x1600 res
	{
		// The last character put us outside of the box.  Cancel it.
		sEditCtrlRemoveChar(pEditCtrl, TRUE);
		UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIEditCtrlSetFocus(
	UI_EDITCTRL *pEditCtrl)
{
	ASSERT_RETURN(pEditCtrl);

	UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)pEditCtrl->m_idComponent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_EDITCTRL* UIEditCtrlGetLastActiveCtrl()
{
	return gpsLastActiveEditCtrl;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnSetFocus(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);

	if ((int)lParam != component->m_idComponent)
	{
		pEditCtrl->m_bHasFocus = FALSE;
	}
	else
	{
		pEditCtrl->m_bHasFocus = TRUE;
		gpsLastActiveEditCtrl = pEditCtrl;
		if (pEditCtrl->m_bAllowIME) {
			IMM_Enable(TRUE);

			UI_POSITION absolute = UIComponentGetAbsoluteLogPosition(pEditCtrl);
			UIX_TEXTURE_FONT* pFont = UIComponentGetFont(pEditCtrl);
			int nFontSize = UIComponentGetFontSize(pEditCtrl, pFont);

			UI_COMPONENT* pParent = UIComponentGetByEnum(UICOMP_IME_PANEL);
			if (pParent) {
				pParent->m_Position.m_fX = absolute.m_fX;
				pParent->m_Position.m_fY = absolute.m_fY;
				pParent->m_fHeight = pEditCtrl->m_fHeight;

				// Sets the location of language indicator & IME windows based on the edit ctrl
				UI_LABELEX* pIndicator = UICastToLabel(UIComponentGetByEnum(UICOMP_IME_LANG_INDICATOR));
				if (pIndicator != NULL) {
					pIndicator->m_Position.m_fX = pEditCtrl->m_fWidth;
					pIndicator->m_Position.m_fY = 0;
					pIndicator->m_fHeight = pEditCtrl->m_fHeight;
					UIComponentHandleUIMessage(pIndicator, UIMSG_PAINT, 0, 0);

					//			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pIndicator);
					//			int nFontSize = UIComponentGetFontSize(pIndicator, pFont);

					//			pIndicator->m_fWidth = UIElementGetTextLogWidth(pFont, nFontSize, 1.0, pIndicator->m_bNoScaleFont, pIndicator->m_Line.m_szText, TRUE);
				}

				FLOAT dx = 0.0f;
				UIElementGetTextLogSize(pFont, nFontSize, 1.0, pEditCtrl->m_bNoScaleFont, pEditCtrl->m_Line.GetText(), &dx, NULL, TRUE, TRUE);
				dx = (float)FLOOR(dx);

				UI_COMPONENT* pReadingWnd = UIComponentGetByEnum(UICOMP_IME_READING);
				if (pReadingWnd != NULL) {
					pReadingWnd->m_Position.m_fX = dx;
					pReadingWnd->m_Position.m_fY = pParent->m_fHeight;
					UIComponentHandleUIMessage(pReadingWnd, UIMSG_PAINT, 0, 0);
				}

				UI_COMPONENT* pCandidateWnd = UIComponentGetByEnum(UICOMP_IME_CANDIDATE);
				if (pCandidateWnd != NULL) {
					pCandidateWnd->m_Position.m_fX = dx;
					pCandidateWnd->m_Position.m_fY = pParent->m_fHeight;

					UI_POSITION posAbsolute = UIComponentGetAbsoluteLogPosition(pCandidateWnd);
					float ratio = UIGetScreenToLogRatioY(pCandidateWnd->m_bNoScale);
					float dy = AppCommonGetWindowWidth() - (posAbsolute.m_fY / ratio + pCandidateWnd->m_fHeight);
					if (dy < 0) {
						pCandidateWnd->m_Position.m_fY += dy;
					}

					UIComponentHandleUIMessage(pCandidateWnd, UIMSG_PAINT, 0, 0);
				}
			}
			IMM_SetCandidateWindowChangeHandler(UISetCandidateWindowStrings);
			IMM_SetReadingWindowChangeHandler(UISetReadingWindowStrings);
		} else {
			IMM_Enable(FALSE);
		}
	}
	UIComponentHandleUIMessage(pEditCtrl, UIMSG_PAINT, 0, 0);

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sUIEditCtrlGetLineSize(
	UI_EDITCTRL *pEditCtrl)
{
	ASSERT_RETZERO(pEditCtrl);

	// calculate the height of a line
	UIX_TEXTURE_FONT* font = UIComponentGetFont(pEditCtrl);
	ASSERT_RETZERO(font);

	int nFontSize = UIComponentGetFontSize(pEditCtrl, font);

	float fCharSize = 0.0f;
	UIElementGetTextLogSize(font, nFontSize, 1.0f, pEditCtrl->m_bNoScaleFont, L"|", NULL, &fCharSize, TRUE, TRUE, 0, 1);

	float fLineSize = 0.0f;
	UIElementGetTextLogSize(font, nFontSize, 1.0f, pEditCtrl->m_bNoScaleFont, L"|\n|", NULL, &fLineSize, TRUE, TRUE, 0, 3);

	return fLineSize - fCharSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIEditCtrlPositionCaretAt(
	UI_EDITCTRL *pEditCtrl,
	float x,
	float y)
{
	ASSERT_RETURN(pEditCtrl);

	int nLen = PStrLen(pEditCtrl->m_Line.GetText());

	// Make sure the wordwrapping has been processed
	UILabelOnPaint(pEditCtrl, UIMSG_PAINT, 0, 0);

	const WCHAR *szText = pEditCtrl->m_Line.GetText();

	// get the font and line size
	UIX_TEXTURE_FONT* font = UIComponentGetFont(pEditCtrl);
	ASSERT_RETURN(font);
	int nFontSize = UIComponentGetFontSize(pEditCtrl, font);
	float fLineSize = sUIEditCtrlGetLineSize(pEditCtrl);

	// find the line we are on
	int nLine = pEditCtrl->m_bMultiline ? (int)(y / fLineSize) : 0;
	int nCurLine=0, nPos=0, nCaretOffs=0;

	while (nCurLine < nLine && nPos < nLen)
	{
		if (szText[nPos] == '\n')
		{
			++nCurLine;
		}
		else if (szText[nPos] == '\r')
		{
			++nCurLine;
			++nCaretOffs;
		}
		++nPos;
	}

	// find the character we are on
	float fWidth = 0.0f;
	float fCharWidth = 0.0f;
	while (szText[nPos] != '\n' && szText[nPos] != '\r' && nPos < nLen && fWidth < x)
	{
		UIElementGetTextLogSize(font,
			nFontSize,
			1.0f,
			pEditCtrl->m_bNoScaleFont,
			&szText[nPos],
			&fCharWidth,
			NULL,
			TRUE,
			TRUE,
			0,
			1);
		fWidth += fCharWidth;
		++nPos;
	}

	if (nPos && fCharWidth && ((fWidth - x) > (fCharWidth / 2.0f)))
	{
		--nPos;
	}

	// re-position the caret
	pEditCtrl->m_nCaretPos = nPos-nCaretOffs;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnLClick(
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

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);

	if (pEditCtrl->m_Line.HasText())
	{
		//----------------------------------------------------------------------------
		//position the caret based on where we clicked

		// get the mouse position relative to the component
		float x = 0.0f;	float y = 0.0f;
		UIGetCursorPosition(&x, &y);
		UI_POSITION pos;
		if (!UIComponentCheckBounds(component, x, y, &pos))
		{
			return UIMSG_RET_NOT_HANDLED;
		}

		x -= pos.m_fX;
		y -= pos.m_fY;
		y += UIComponentGetScrollPos(component).m_fY;

		sUIEditCtrlPositionCaretAt(pEditCtrl, x, y);
	}

	UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)component->m_idComponent);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnLButtonDown(
								 UI_COMPONENT* component,
								 int msg,
								 DWORD wParam,
								 DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	if (UIComponentGetActive(component) && 
		!UIComponentCheckBounds(component))
	{
		UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);

		if (pEditCtrl->m_bHasFocus)
		{
			IMM_Enable(FALSE);
			pEditCtrl->m_bHasFocus = FALSE;
			UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)INVALID_ID);
		}
		return UIMSG_RET_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnPostActivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);

	if (pEditCtrl->m_bStartsWithFocus)
	{
		UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)component->m_idComponent);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIEditCtrlOnPostInactivate(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);

	UI_EDITCTRL *pEditCtrl = UICastToEditCtrl(component);

	if (pEditCtrl->m_bHasFocus)
	{
		IMM_Enable(FALSE);
		pEditCtrl->m_bHasFocus = FALSE;
		UIHandleUIMessage(UIMSG_SETFOCUS, 0, (LPARAM)INVALID_ID);
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIIsImeWindowsActive()
{
	UI_COMPONENT* pCandidateWnd = UIComponentGetByEnum(UICOMP_IME_CANDIDATE);
	UI_COMPONENT* pReadingWnd = UIComponentGetByEnum(UICOMP_IME_READING);
	return (pCandidateWnd ? pCandidateWnd->m_bVisible : FALSE) || (pReadingWnd ? pReadingWnd->m_bVisible : FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadEditCtrl(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	//Load extras for parent class
	if (!UIXmlLoadLabel(xml, component, tUIXml))
		return FALSE;

	UI_EDITCTRL* pEdit = UICastToEditCtrl(component);
	ASSERT_RETFALSE(pEdit);

	char szBuf[256];
	PStrPrintf( szBuf, 256, "" );

	XML_LOADSTRING_RC("allowchars", szBuf, "", 256);
	PStrCvt(pEdit->m_szOnlyAllowChars, szBuf, arrsize(pEdit->m_szOnlyAllowChars));
	PStrPrintf( szBuf, 256, "" );
	XML_LOADSTRING_RC("disallowchars", szBuf, "", 256);
	PStrCvt(pEdit->m_szDisallowChars, szBuf, arrsize(pEdit->m_szDisallowChars));

	BOOL bDisallowPathChars = FALSE;
	XML_LOADBOOL_RC("disallowpathchars", bDisallowPathChars, FALSE);
	if (bDisallowPathChars)
	{
		PStrCat(pEdit->m_szDisallowChars, L"/\\:*?.<>| ", arrsize(pEdit->m_szDisallowChars));
		PStrCat(pEdit->m_szDisallowChars, L"\"", arrsize(pEdit->m_szDisallowChars));
	}

	XML_LOADINT("maxchars", pEdit->m_nMaxLen, 32);
	XML_LOADBOOL("startwithfocus", pEdit->m_bStartsWithFocus, FALSE);
	if (pEdit->m_bStartsWithFocus)
	{
		if (UIComponentGetActive(pEdit))
		{
			pEdit->m_bHasFocus = TRUE;
		}
		pEdit->m_bNeedsRepaintOnVisible = TRUE;
	}

	XML_LOADBOOL("password", pEdit->m_bPassword, FALSE);
	XML_LOADSTRING("nexttabctrl", pEdit->m_szNextTabCtrlName, "", MAX_UI_COMPONENT_NAME_LENGTH);
	if (pEdit->m_szNextTabCtrlName[0])
	{
		pEdit->m_pNextTabCtrl = UIComponentFindChildByName(pEdit->m_pParent, pEdit->m_szNextTabCtrlName);
		if (pEdit->m_pNextTabCtrl)
		{
			UI_EDITCTRL *pOtherEdit = UICastToEditCtrl(pEdit->m_pNextTabCtrl);
			pOtherEdit->m_pPrevTabCtrl = pEdit;
		}
	}
	XML_LOADBOOL("limit_to_space", pEdit->m_bLimitTextToSpace, TRUE);
	XML_LOADBOOL("multiline", pEdit->m_bMultiline, FALSE);
	XML_LOADBOOL("eatsallkeys", pEdit->m_bEatsAllKeys, FALSE);
	XML_LOADBOOL("allowIME", pEdit->m_bAllowIME, TRUE);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadListBox(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	if (!UIXmlLoadTextBox(xml, component, tUIXml))
		return FALSE;

	UI_LISTBOX * pListBox = UICastToListBox(component);
	ASSERT_RETFALSE(pListBox);

	UIX_TEXTURE_FONT* font = UIComponentGetFont(pListBox);
	if (!font)
	{
		return FALSE;
	}
	 
	pListBox->m_fLineHeight = sUITextBoxGetLineHeight(pListBox);

	UIXMLLoadColor(xml, pListBox, "item", pListBox->m_dwItemColor, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, pListBox, "highlight", pListBox->m_dwItemHighlightColor, GFXCOLOR_WHITE);
	UIXMLLoadColor(xml, pListBox, "highlightbk", pListBox->m_dwItemHighlightBKColor, GFXCOLOR_WHITE);

	XML_LOADBOOL("autosize", pListBox->m_bAutosize, FALSE);
	XML_LOADFRAME("highlightframe",	UIComponentGetTexture(component), pListBox->m_pHighlightBarFrame, "");

	if (pListBox->m_bAutosize)
	{
		pListBox->m_fHeight = (pListBox->m_LinesList.Count() * pListBox->m_fLineHeight) + (pListBox->m_fBordersize * 2.0f);
		pListBox->m_InactivePos.m_fY = pListBox->m_ActivePos.m_fY - pListBox->m_fHeight;
		// Ugly special case
		if (pListBox->m_pParent && pListBox->m_pParent->m_pParent && pListBox->m_pParent->m_pParent->m_eComponentType == UITYPE_COMBOBOX)
		{
			pListBox->m_pParent->m_fHeight = pListBox->m_fHeight;
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadComboBox(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	UI_COMBOBOX *pCombo = UICastToComboBox(component);
	ASSERT_RETFALSE(pCombo);

	// This combo has already been created as a reference - don't recreate the sub-elements!
	if( pCombo->m_pButton )
	{
		return TRUE;
	}
	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	ASSERT_RETFALSE(pTexture);

	// BUTTON
	pCombo->m_pButton = (UI_BUTTONEX *) UIComponentCreateNew(pCombo, UITYPE_BUTTON, TRUE);
	ASSERT_RETFALSE(pCombo->m_pButton);

	PStrPrintf(pCombo->m_pButton->m_szName, MAX_UI_COMPONENT_NAME_LENGTH, "%s button", pCombo->m_szName);
	CMarkup xmlBlank;
	UIXmlLoadButton(xmlBlank, pCombo->m_pButton, tUIXml);		// load button defaults

	XML_LOADFRAME("buttonupframe", pTexture, pCombo->m_pButton->m_pFrame, "");
	XML_LOADFRAME("buttonupframemid", pTexture, pCombo->m_pButton->m_pFrameMid, "");
	XML_LOADFRAME("buttonupframeleft", pTexture, pCombo->m_pButton->m_pFrameLeft, "");
	XML_LOADFRAME("buttonupframeright", pTexture, pCombo->m_pButton->m_pFrameRight, "");
	ASSERTX_RETFALSE(pCombo->m_pButton->m_pFrame || pCombo->m_pButton->m_pFrameMid, "ComboBox needs a button frame");

	XML_LOADFRAME("buttondownframe", pTexture, pCombo->m_pButton->m_pDownFrame, "");
	XML_LOADFRAME("buttondownframemid", pTexture, pCombo->m_pButton->m_pDownFrameMid, "");
	XML_LOADFRAME("buttondownframeleft", pTexture, pCombo->m_pButton->m_pDownFrameLeft, "");
	XML_LOADFRAME("buttondownframeright", pTexture, pCombo->m_pButton->m_pDownFrameRight, "");

	XML_LOADFRAME("buttonhighliteframe", pTexture, pCombo->m_pButton->m_pLitFrame, "");
	XML_LOADFRAME("buttonhighliteframemid", pTexture, pCombo->m_pButton->m_pLitFrameMid, "");
	XML_LOADFRAME("buttonhighliteframeleft", pTexture, pCombo->m_pButton->m_pLitFrameLeft, "");
	XML_LOADFRAME("buttonhighliteframeright", pTexture, pCombo->m_pButton->m_pLitFrameRight, "");

	pCombo->m_pButton->m_bVisible = pCombo->m_bVisible;
	pCombo->m_pButton->m_eState = pCombo->m_eState;
	pCombo->m_pButton->m_fWidth = pCombo->m_pButton->m_pFrame ? pCombo->m_pButton->m_pFrame->m_fWidth: pCombo->m_fWidth;
	pCombo->m_pButton->m_fWidth *= g_UI.m_fUIScaler;
	//pCombo->m_fWidth *= g_UI.m_fUIScaler;
	pCombo->m_pButton->m_fHeight = pCombo->m_pButton->m_pFrame ? pCombo->m_pButton->m_pFrame->m_fHeight : pCombo->m_pButton->m_pFrameMid->m_fHeight;
	//pCombo->m_pButton->m_fHeight *= g_UI.m_fUIScaler;
	pCombo->m_fHeight *= g_UI.m_fUIScaler;
	pCombo->m_pButton->m_Position.m_fX = pCombo->m_fWidth * g_UI.m_fUIScaler - pCombo->m_pButton->m_fWidth;
	pCombo->m_pButton->m_Position.m_fY = 0.0f;
	pCombo->m_pButton->m_eButtonStyle = UIBUTTONSTYLE_CHECKBOX;
	UIAddMsgHandler(pCombo->m_pButton, UIMSG_LBUTTONCLICK, UIWindowshadeButtonOnClick, INVALID_LINK);

	// PANEL
	UI_PANELEX *pDropdownPanel = (UI_PANELEX *)UIComponentCreateNew(pCombo, UITYPE_PANEL, TRUE);
	PStrPrintf(pDropdownPanel->m_szName, MAX_UI_COMPONENT_NAME_LENGTH, "%s dropdown panel", pCombo->m_szName);
//	if (!pCombo->m_pListBox->m_bAutosize)
	{
		XML_LOADFLOAT("dropdownheight", pDropdownPanel->m_fHeight, 100.0f);
	}
	XML_LOADFLOAT("dropdownwidth", pDropdownPanel->m_fWidth, pCombo->m_fWidth);
	pDropdownPanel->m_fWidth *= g_UI.m_fUIScaler;
	pDropdownPanel->m_fHeight = pCombo->m_fHeight;

	float dropdownyoffset = 0;
    XML_LOADFLOAT("dropdownyoffset", dropdownyoffset, 0.0f);

	pDropdownPanel->m_ActivePos.m_fY = pCombo->m_pButton->m_fHeight + dropdownyoffset;

	pDropdownPanel->m_Position.m_fX = pDropdownPanel->m_ActivePos.m_fX = pDropdownPanel->m_InactivePos.m_fX = 0.0f;
	pDropdownPanel->m_Position.m_fY = pDropdownPanel->m_InactivePos.m_fY = pDropdownPanel->m_ActivePos.m_fY - pDropdownPanel->m_fHeight;
	pDropdownPanel->m_eState = UISTATE_INACTIVE;
	pDropdownPanel->m_bVisible = FALSE;
	pDropdownPanel->m_bIndependentActivate = TRUE;
	pDropdownPanel->m_nRenderSection = pCombo->m_nRenderSection + 1;

	BOOL bLoadFlexborder = FALSE;
	XML_LOADBOOL("listflexborder", bLoadFlexborder, FALSE);
	UI_FLEXBORDER * pFlexborder = (UI_FLEXBORDER *) UIComponentCreateNew(pDropdownPanel, UITYPE_FLEXBORDER, TRUE);
	ASSERT_RETFALSE(pFlexborder);
	PStrPrintf(pFlexborder->m_szName, MAX_UI_COMPONENT_NAME_LENGTH, "%s flexborder", pCombo->m_pListBox->m_szName);
	UIXmlLoadFlexBorder(xml, pFlexborder, tUIXml);
	pFlexborder->m_bNoScale = pCombo->m_bNoScale;
	// LISTBOX
	pCombo->m_pListBox = (UI_LISTBOX *) UIComponentCreateNew(pDropdownPanel, UITYPE_LISTBOX, TRUE);
	ASSERT_RETFALSE(pCombo->m_pListBox);
	pCombo->m_pListBox->m_nSelection = -1;	// CHB 2007.01.23

	PStrPrintf(pCombo->m_pListBox->m_szName, MAX_UI_COMPONENT_NAME_LENGTH, "%s listbox", pCombo->m_szName);
	UIXmlLoadListBox(xml, pCombo->m_pListBox, tUIXml);

	XML_LOADFRAME("listframe", pTexture, pCombo->m_pListBox->m_pFrame, "");
	pCombo->m_pListBox->m_fWidth = pDropdownPanel->m_fWidth;
	if (!pCombo->m_pListBox->m_bAutosize)
	{
		pCombo->m_pListBox->m_fHeight = pDropdownPanel->m_fHeight;
	}
	if (pCombo->m_pListBox->m_pScrollbar)
	{
		sUITextBoxAdjustScrollbar(pCombo->m_pListBox);
		pCombo->m_pListBox->m_pScrollbar->m_Position.m_fX = pCombo->m_pListBox->m_fWidth - pCombo->m_pListBox->m_pScrollbar->m_fWidth;
		pCombo->m_pListBox->m_pScrollbar->m_fHeight = pCombo->m_pListBox->m_fHeight;
	}

	// LABEL
	pCombo->m_pLabel = (UI_LABELEX *) UIComponentCreateNew(pCombo, UITYPE_LABEL, TRUE);
	ASSERT_RETFALSE(pCombo->m_pLabel);

	PStrPrintf(pCombo->m_pLabel->m_szName, MAX_UI_COMPONENT_NAME_LENGTH, "%s label", pCombo->m_szName);
	XML_LOADFLOAT("labelx", pCombo->m_pLabel->m_Position.m_fX, 0.0f);
	XML_LOADFLOAT("labely", pCombo->m_pLabel->m_Position.m_fY, 0.0f);
	
	XML_LOADFLOAT("labelwidth", pCombo->m_pLabel->m_fWidth, pCombo->m_fWidth - pCombo->m_pButton->m_fWidth);
	pCombo->m_pLabel->m_fWidth *= g_UI.m_fUIScaler;
	XML_LOADFLOAT("labelheight", pCombo->m_pLabel->m_fHeight, 0.0f);
	XML_LOADFRAME("labelframe", pTexture, pCombo->m_pLabel->m_pFrame, "");
	pCombo->m_pLabel->m_fHeight = pCombo->m_pLabel->m_pFrame ? pCombo->m_pLabel->m_pFrame->m_fHeight : pCombo->m_pButton->m_fHeight;


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXmlLoadColorPicker(
	CMarkup& xml,
	UI_COMPONENT* component, 
	const UI_XML & tUIXml)
{
	ASSERT_RETFALSE(component);
	UI_COLORPICKER *pPicker = UICastToColorPicker(component);

    XML_LOADFLOAT("gridoffx", pPicker->m_fGridOffsetX, 0.0f);
    XML_LOADFLOAT("gridoffy", pPicker->m_fGridOffsetY, 0.0f);
	XML_LOADINT("gridcols", pPicker->m_nGridCols, 4);
	XML_LOADINT("gridrows", pPicker->m_nGridRows, 6);
	XML_LOADFLOAT("cellsize", pPicker->m_fCellSize, 32.0f);
	XML_LOADFLOAT("cellsspacing", pPicker->m_fCellSpacing, 2.0f);

	XML_LOADFRAME("cellframe", UIComponentGetTexture(pPicker), pPicker->m_pCellFrame, "");
	XML_LOADFRAME("cellupframe", UIComponentGetTexture(pPicker), pPicker->m_pCellUpFrame, "");
	XML_LOADFRAME("celldownframe", UIComponentGetTexture(pPicker), pPicker->m_pCellDownFrame, "");

	pPicker->m_pdwColors = (DWORD *)MALLOCZ(NULL, sizeof(DWORD) * pPicker->m_nGridCols * pPicker->m_nGridRows);

	pPicker->m_nSelectedIndex = -1;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIXmlFreeColorPicker(
	UI_COMPONENT* component)
{
	ASSERT_RETURN(component);

	UI_COLORPICKER *pPicker = UICastToColorPicker(component);
	if (pPicker->m_pdwColors)
	{
		FREE(NULL, pPicker->m_pdwColors);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypes(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	
	//					   struct				type					name				load function				free function
	UI_REGISTER_COMPONENT( UI_SCREENEX,			UITYPE_SCREEN,			"screen",			UIXmlLoadScreen,			NULL					);
	UI_REGISTER_COMPONENT( UI_CURSOR,			UITYPE_CURSOR,			"cursor",			UIXmlLoadCursor,			NULL					);
	UI_REGISTER_COMPONENT( UI_LABELEX,			UITYPE_LABEL,			"label",			UIXmlLoadLabel,				UIComponentFreeLabel	);
	UI_REGISTER_COMPONENT( UI_PANELEX,			UITYPE_PANEL,			"panel",			UIXmlLoadPanel,				NULL					);
	UI_REGISTER_COMPONENT( UI_BUTTONEX,			UITYPE_BUTTON,			"button",			UIXmlLoadButton,			NULL					);
	UI_REGISTER_COMPONENT( UI_BAR,				UITYPE_BAR,				"bar",				UIXmlLoadBar,				NULL					);
	UI_REGISTER_COMPONENT( UI_SCROLLBAR,		UITYPE_SCROLLBAR,		"scrollbar",		UIXmlLoadScrollBar,			NULL					);
	UI_REGISTER_COMPONENT( UI_TOOLTIP,			UITYPE_TOOLTIP,			"tooltip",			UIXmlLoadTooltip,			NULL					);
	UI_REGISTER_COMPONENT( UI_CONVERSATION_DLG,	UITYPE_CONVERSATION_DLG,"conversationdlg",	UIXmlLoadConversationDlg,	NULL					);
	UI_REGISTER_COMPONENT( UI_TEXTBOX,			UITYPE_TEXTBOX,			"textbox",			UIXmlLoadTextBox,			UIComponentFreeTextBox	);
	UI_REGISTER_COMPONENT( UI_FLEXBORDER,		UITYPE_FLEXBORDER,		"flexborder",		UIXmlLoadFlexBorder,		NULL					);
	UI_REGISTER_COMPONENT( UI_FLEXLINE,			UITYPE_FLEXLINE,		"flexline",			UIXmlLoadFlexLine,			NULL					);
	UI_REGISTER_COMPONENT( UI_MENU,				UITYPE_MENU,			"menu",				UIXmlLoadMenu,				NULL					);

	UI_REGISTER_COMPONENT( UI_STATDISPL,		UITYPE_STATDISPL,		"statdispl",		UIXmlLoadStatDispL,			UIComponentFreeLabel	);
	UI_REGISTER_COMPONENT( UI_AUTOMAP,			UITYPE_AUTOMAP,			"automap",			UIXmlLoadAutoMap,			UIComponentFreeAutoMap	);
	UI_REGISTER_COMPONENT( UI_ITEMBOX,			UITYPE_ITEMBOX,			"itembox",			UIXmlLoadItemBox,			NULL					);	

	UI_REGISTER_COMPONENT( UI_EDITCTRL,			UITYPE_EDITCTRL,		"editctrl",			UIXmlLoadEditCtrl,			UIComponentFreeLabel	);
	UI_REGISTER_COMPONENT( UI_LISTBOX,			UITYPE_LISTBOX,			"listbox",			UIXmlLoadListBox,			UIComponentFreeTextBox	);
	UI_REGISTER_COMPONENT( UI_COMBOBOX,			UITYPE_COMBOBOX,		"combobox",			UIXmlLoadComboBox,			NULL					);
	UI_REGISTER_COMPONENT( UI_COLORPICKER,		UITYPE_COLORPICKER,		"colorpicker",		UIXmlLoadColorPicker,		UIXmlFreeColorPicker	);

	UIMAP(UITYPE_PANEL,				UIMSG_ACTIVATE,				UIPanelOnActivate);
	UIMAP(UITYPE_PANEL,				UIMSG_INACTIVATE,			UIPanelOnInactivate);
	UIMAP(UITYPE_LABEL,				UIMSG_PAINT,				UILabelOnPaint);
	UIMAP(UITYPE_STATDISPL,			UIMSG_PAINT,				UIStatDisplOnPaint);
	UIMAP(UITYPE_STATDISPL,			UIMSG_SETCONTROLUNIT,		UIStatDisplOnPaint);						// <--
//	UIMAP(UITYPE_STATDISPL,			UIMSG_SETCONTROLSTAT,		UIStatDisplOnPaint);						// <-- Ok, we should not use a stat message as a default.  We're going to have to define stat-specific handlers for each one of these in the XML
	UIMAP(UITYPE_STATDISPL,			UIMSG_MOUSEHOVERLONG,		UIStatDisplOnMouseHover);
	UIMAP(UITYPE_STATDISPL,			UIMSG_MOUSELEAVE,			UIStatDisplOnMouseLeave);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_PAINT,				UIEditCtrlOnPaint);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_KEYCHAR,				UIEditCtrlOnKeyChar);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_KEYDOWN,				UIEditCtrlOnKeyDown);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_KEYUP,				UIEditCtrlOnKeyUp);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_SETFOCUS,				UIEditCtrlOnSetFocus);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_LBUTTONCLICK,			UIEditCtrlOnLClick);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_POSTACTIVATE,			UIEditCtrlOnPostActivate);
	UIMAP(UITYPE_EDITCTRL,			UIMSG_POSTINACTIVATE,		UIEditCtrlOnPostInactivate);
	UIMAP(UITYPE_BAR,				UIMSG_PAINT,				UIBarOnPaint);
	UIMAP(UITYPE_TEXTBOX,			UIMSG_PAINT,				UITextBoxOnPaint);
	UIMAP(UITYPE_TEXTBOX,			UIMSG_MOUSEOVER,			UITextBoxOnMouseOver);
	UIMAP(UITYPE_TEXTBOX,			UIMSG_MOUSELEAVE,			UITextBoxOnMouseLeave);
//	UIMAP(UITYPE_TEXTBOX,			UIMSG_RESIZE,				UITextBoxOnResize);
	UIMAP(UITYPE_TEXTBOX,			UIMSG_SCROLL,				UITextBoxOnScroll);
	UIMAP(UITYPE_LISTBOX,			UIMSG_PAINT,				UIListBoxOnPaint);
	UIMAP(UITYPE_LISTBOX,			UIMSG_MOUSEOVER,			UIListBoxOnPaint);
	UIMAP(UITYPE_LISTBOX,			UIMSG_LBUTTONDOWN,			UIListBoxOnMouseDown);
	UIMAP(UITYPE_AUTOMAP,			UIMSG_PAINT,				UIAutoMapOnPaint);
	UIMAP(UITYPE_AUTOMAP,			UIMSG_ACTIVATE,				UIAutoMapOnActivate);
	UIMAP(UITYPE_AUTOMAP,			UIMSG_INACTIVATE,			UIPanelOnInactivate);
	UIMAP(UITYPE_BUTTON,			UIMSG_PAINT,				UIButtonOnPaint);
	UIMAP(UITYPE_BUTTON,			UIMSG_MOUSEOVER,			UIButtonOnMouseOver);
	UIMAP(UITYPE_BUTTON,			UIMSG_MOUSELEAVE,			UIButtonOnMouseLeave);
	UIMAP(UITYPE_BUTTON,			UIMSG_LBUTTONDOWN,			UIButtonOnButtonDown);
	UIMAP(UITYPE_BUTTON,			UIMSG_LBUTTONUP,			UIButtonOnButtonUp);
	UIMAP(UITYPE_BUTTON,			UIMSG_POSTACTIVATE,			UIButtonOnPostActivate);	
	UIMAP(UITYPE_BUTTON,			UIMSG_POSTINACTIVATE,		UIButtonOnPostInactivate);	
	UIMAP(UITYPE_BUTTON,			UIMSG_POSTINVISIBLE,		UIButtonOnPostInvisible);	
	UIMAP(UITYPE_SCREEN,			UIMSG_RBUTTONDOWN,			UIScreenOnRButtonDown);
	UIMAP(UITYPE_CURSOR,			UIMSG_PAINT,				UICursorOnPaint);
	UIMAP(UITYPE_CURSOR,			UIMSG_INVENTORYCHANGE,		UICursorOnInventoryChange);
	UIMAP(UITYPE_FLEXBORDER,		UIMSG_PAINT,				UIFlexBorderOnPaint);
	UIMAP(UITYPE_FLEXLINE,			UIMSG_PAINT,				UIFlexLineOnPaint);
	UIMAP(UITYPE_TOOLTIP,			UIMSG_PAINT,				UITooltipOnPaint);
	UIMAP(UITYPE_SCROLLBAR,			UIMSG_PAINT,				UIScrollBarOnPaint);
	UIMAP(UITYPE_SCROLLBAR,			UIMSG_MOUSEWHEEL,			UIScrollBarOnMouseWheel);
	UIMAP(UITYPE_SCROLLBAR,			UIMSG_MOUSEMOVE,			UIScrollBarOnMouseMove);
	UIMAP(UITYPE_SCROLLBAR,			UIMSG_LBUTTONDOWN,			UIScrollBarOnLButtonDown);
	UIMAP(UITYPE_SCROLLBAR,			UIMSG_LBUTTONUP,			UIScrollBarOnLButtonUp);
	UIMAP(UITYPE_SCROLLBAR,			UIMSG_SCROLL,				UIScrollBarOnScroll);
	UIMAP(UITYPE_MENU,				UIMSG_MOUSEOVER,			UIMenuOnMouseOver);
	UIMAP(UITYPE_MENU,				UIMSG_POSTINACTIVATE,		UIMenuOnPostInactivate);
	UIMAP(UITYPE_MENU,				UIMSG_ACTIVATE,				UIComponentOnActivate);
	UIMAP(UITYPE_MENU,				UIMSG_INACTIVATE,			UIComponentOnInactivate);
	UIMAP(UITYPE_LABEL,				UIMSG_POSTINACTIVATE,		UILabelOnPostInactivate);	
	//UIMAP(UITYPE_COLORPICKER,		UIMSG_ACTIVATE,				UIComponentOnActivate);	
	//UIMAP(UITYPE_COLORPICKER,		UIMSG_INACTIVATE,			UIComponentOnInactivate);	
	UIMAP(UITYPE_BUTTON,			UIMSG_MOUSELEAVE,			UIButtonOnMouseLeavePassThrough);

	UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name						function pointer
		{ "UIButtonOnMouseLeaveItem",			UIButtonOnMouseLeaveItem },
		{ "UIButtonOnMouseOverItem",			UIButtonOnMouseOverItem },
		{ "UIScrollBarOnScroll",				UIScrollBarOnScroll },
		{ "UIBarOnPaint",						UIBarOnPaint },
		{ "UIButtonOnPaint",					UIButtonOnPaint },
		{ "UIEditCtrlOnKeyChar",				UIEditCtrlOnKeyChar },
		{ "UIEditCtrlOnPaint",					UIEditCtrlOnPaint },
		{ "UIEditCtrlOnLButtonDown",			UIEditCtrlOnLButtonDown },
		{ "UIScrollFramePaint",					UIScrollFramePaint },
		{ "UIScrollOnMouseWheel",				UIScrollOnMouseWheel },
		{ "UIScrollOnMouseWheelRepaint",		UIScrollOnMouseWheelRepaint },
		{ "UIStatDisplOnPaint",					UIStatDisplOnPaint },
		{ "UILabelSelect",						UILabelSelect },
		{ "UILabelOnPaintCheckParentButton",	UILabelOnPaintCheckParentButton },
		{ "UIOpenPartyMemberPopupMenu",			UIOpenPartyMemberPopupMenu },
		{ "UIOpenPetPopupMenu",					UIOpenPetPopupMenu },
		{ "UIPopupMenuClose",					UIPopupMenuClose },
		{ "UIPartyMemberOnClickTrade",			UIPartyMemberOnClickTrade },
		{ "UIPartyMemberOnPostVisibleTrade",	UIPartyMemberOnPostVisibleTrade },
		{ "UIPartyMemberOnClickWarpTo",			UIPartyMemberOnClickWarpTo },
		{ "UIPartyMemberOnPostVisibleWarpTo",	UIPartyMemberOnPostVisibleWarpTo },		
		{ "UIPartyMemberOnClickInventory",		UIPartyMemberOnClickInventory },
		{ "UIPartyMemberOnPostVisibleInventory",UIPartyMemberOnPostVisibleInventory },
		{ "UIPartyMemberOnClickWarpToBtn",		UIPartyMemberOnClickWarpToBtn },
		{ "UIPartyMemberOnClickKick",			UIPartyMemberOnClickKick },
		{ "UIPartyMemberOnPostVisibleKick",		UIPartyMemberOnPostVisibleKick },		
		{ "UIPartyMemberOnClickLeave",			UIPartyMemberOnClickLeave },		
		{ "UIPartyMemberOnClickDuelInvite",		UIPartyMemberOnClickDuelInvite },		

		{ "UIWindowshadeButtonOnClick",			UIWindowshadeButtonOnClick },
		{ "UIWindowshadeAndSlideButtonOnClick",	UIWindowshadeAndSlideButtonOnClick },
		{ "UISetCharsheetScrollbar",			UISetCharsheetScrollbar },
		{ "UILabelOnMouseHover",				UILabelOnMouseHover },
		{ "UILabelRevealAllText",				UILabelRevealAllText },				
		{ "UIClearTooltips",					UIClearTooltips },
		{ "UIItemTooltipOnPaint",				UIItemTooltipOnPaint },

		{ "UIMsgPassToSibling",					UIMsgPassToSibling },
		{ "UIMsgPassToParent",					UIMsgPassToParent },
		{ "UIClickSiblingButton",				UIClickSiblingButton },
		{ "UIPanelResizeScrollbar",				UIPanelResizeScrollbar },
		{ "UIQuestsPanelResizeScrollbar",		UIQuestsPanelResizeScrollbar },
		{ "UILanguageIndicatorOnPaint",			UILanguageIndicatorOnPaint },
		{ "UIPanelBeginMouseDragScroll",		UIPanelBeginMouseDragScroll },
		{ "UIPanelMouseDragScroll",				UIPanelMouseDragScroll },
		{ "UIPanelEndMouseDragScroll",			UIPanelEndMouseDragScroll },
		{ "UILabelSetToFocusUnitName",			UILabelSetToFocusUnitName },
		{ "UIScrollChildBarOnMouseWheel",		UIScrollChildBarOnMouseWheel },
		{ "UIFeedBarOnPaint",					UIFeedBarOnPaint },
		{ "UITooltipOnPaint",					UITooltipOnPaint },

		{ "UIPlayerInteractPanelSetFocusVisible", UIPlayerInteractPanelSetFocusVisible },
		{ "UIPlayerInteractPaint",				UIPlayerInteractPaint },
		{ "UIPlayerInteractClick",				UIPlayerInteractClick },
		{ "UIPlayerInteractMouseOver",			UIPlayerInteractMouseOver },
		{ "UIPlayerInteractPopupOver",			UIPlayerInteractPopupOver },
		{ "UIPlayerInteractTrade",				UIPlayerInteractTrade },
		{ "UIPlayerInteractInvite",				UIPlayerInteractInvite },
		{ "UIPlayerInteractKick",				UIPlayerInteractKick },
		{ "UIPlayerInteractDisband",			UIPlayerInteractDisband },
		{ "UIPlayerInteractLeave",				UIPlayerInteractLeave },
		{ "UIPlayerInteractDuel",				UIPlayerInteractDuel },
		{ "UIOpenChatCommandPopupMenu",			UIOpenChatCommandPopupMenu },
		{ "UIOpenEmoteCommandPopupMenu",		UIOpenEmoteCommandPopupMenu },
		{ "UIChatCommand",						UIChatCommand },
		{ "UIChatOnMouseWheel",					UIChatOnMouseWheel },
		{ "UIEmoteCommand",						UIEmoteCommand },

		{ "UIColorPickerColorsOnPaint",			UIColorPickerColorsOnPaint },
		{ "UIColorPickerOnLClick",				UIColorPickerOnLClick },
		{ "UIColorPickerAcceptOnClick",			UIColorPickerAcceptOnClick },
		{ "UIColorPickerCancelOnClick",			UIColorPickerCancelOnClick },
		{ "UIColorPickerOnBtn",					UIColorPickerOnBtn },

		{ "UIAutomapZoomButtonOnClick",			UIAutomapZoomButtonOnClick },
		{ "UIPetOnClickDismiss",				UIPetOnClickDismiss },
		{ "UIPetPartyMemberOnPaint",			UIPetPartyMemberOnPaint },
		{ "UIQualityBadgeOnPaint",				UIQualityBadgeOnPaint },
		{ "UIStashHolderOnPostActivate",		UIStashHolderOnPostActivate },
		{ "UIAutomapPanelOnPostCreate",			UIAutomapPanelOnPostCreate },

		{ "UIButtonOnMouseLeavePassThrough",	UIButtonOnMouseLeavePassThrough },

		{ "UIResizeComponentStart",				UIResizeComponentStart },
		{ "UIResizeComponentEnd",				UIResizeComponentEnd },
		{ "UIResizeComponent",					UIResizeComponent },
	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}
#endif //!ISVERSION(SERVER_VERSION)


