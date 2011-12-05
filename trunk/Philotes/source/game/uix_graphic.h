//----------------------------------------------------------------------------
// uix_graphic.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_GRAPHIC_H_
#define _UIX_GRAPHIC_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#ifndef __E_UI__
#include "e_ui.h"
#endif

#ifndef _COLORS_H_
#include "colors.h"
#endif

#ifndef _UIX_PRIV_H_
#include "uix_priv.h"
#endif


//----------------------------------------------------------------------------
// Forwards
//----------------------------------------------------------------------------
struct UI_BLINKDATA;
struct UI_TEXTURE_FRAME;
struct UI_GFXELEMENT;
struct UI_ELEMENT_PRIMITIVE;
struct UIX_TEXTURE_FONT_ROW;
struct UI_TEXTBOX;
class UI_LINE;

//----------------------------------------------------------------------------
// Constants
//----------------------------------------------------------------------------
#define UIRS_NEED_UPDATE		MAKE_MASK(0)
#define UIRS_NEED_RENDER		MAKE_MASK(1)

//----------------------------------------------------------------------------

#define TEXT_TAG_BEGIN_CHAR		(L'[')
#define TEXT_TAG_END_CHAR		(L']')
#define TEXT_TAG_BEGIN_STRING	(L"[")
#define TEXT_TAG_END_STRING		(L"]")

//----------------------------------------------------------------------------
#define CTRLCHAR_COLOR						(1)		// follow with 1 byte (1-255) palette index
#define CTRLCHAR_LINECOLOR					(2)
#define CTRLCHAR_COLOR_RETURN				(4)		// returns to the last color used before a CTRLCHAR_COLOR was found (allows nesting colors)
#define CTRLCHAR_HYPERTEXT					(5)		// follow with 1 byte (1-255) index into hypertext table
#define CTRLCHAR_HYPERTEXT_END				(6)		// follow with 1 byte (1-255) index into hypertext table
#define CTRLCHAR_LAST						(7)		// must be last

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

enum
{
	GFXELEMENT_FRAME,
	GFXELEMENT_TEXT,
	GFXELEMENT_TRI,
	GFXELEMENT_RECT,
	GFXELEMENT_ROTFRAME,
	GFXELEMENT_MODEL,
	GFXELEMENT_PRIM,
	GFXELEMENT_CALLBACK,
	GFXELEMENT_FULLTEXTURE,

	NUM_GFXELEMENTTYPES,
};

enum
{
	TEXT_TAG_INVALID = -1,
	TEXT_TAG_UNIT,
	TEXT_TAG_FRAME,
	TEXT_TAG_PGBRK,
	TEXT_TAG_INTERRUPT,
	TEXT_TAG_INTERRUPT_VIDEO,
	TEXT_TAG_VIDEO,			// labels the start of the text as belonging in the NPC video dialog
	TEXT_TAG_KEYNAME,		// replace with the key currently assigned to the named command (i.e. CMD_MOVE_FORWARD)
	TEXT_TAG_COLOR,
	TEXT_TAG_COLOR_END,
	TEXT_TAG_UNITMODE,
	TEXT_TAG_UNITMODE_END,

	// maintain order and enums at bottom for compares
	TEXT_TAG_TALK,
	TEXT_TAG_ANGRY,
	TEXT_TAG_EXCITED,
	TEXT_TAG_PROUD,
	TEXT_TAG_SAD,
	TEXT_TAG_SCARED,
	TEXT_TAG_WAVE,
	TEXT_TAG_BOW,
	TEXT_TAG_GIVE,
	TEXT_TAG_GET,
	TEXT_TAG_PICKUP,
	TEXT_TAG_SECRET,
	TEXT_TAG_KNEEL,
	TEXT_TAG_TURN,

	TEXT_TAG_COUNT,

	// from above. used for a quick compare
	TEXT_TAG_EMOTE_ANIM_START = TEXT_TAG_TALK,
	TEXT_TAG_EMOTE_ANIM_END = TEXT_TAG_TURN,
};

enum
{
	UI_TEXTURE_RESIZE =			MAKE_MASK(0),
	UI_TEXTURE_SOLID =			MAKE_MASK(1),		
	UI_TEXTURE_LINE =			MAKE_MASK(2),	// exclusive with UI_TEXTURE_SOLID
	UI_TEXTURE_FONT =			MAKE_MASK(3) | UI_TEXTURE_SOLID,
	UI_TEXTURE_DISCARD =		MAKE_MASK(4),
	UI_TEXTURE_OPAQUE =			MAKE_MASK(5),
};

enum UI_PRIMITIVE_TYPE
{
	UIPRIM_BOX = 0,
	UIPRIM_ARCSECTOR,
	// count
	NUM_UI_PRIM_TYPES
};

enum ROTATION_FLAGS
{
	ROTATION_FLAG_ABOUT_SELF_BIT = 0,		// do rotation centered around the element position itself
	ROTATION_FLAG_DROPSHADOW_BIT = 1,		// draw a dropshadow as well
};

//----------------------------------------------------------------------------
// EXPORTED STRUCTURES
//----------------------------------------------------------------------------

// each component contains a list of gfx elements which describe how to draw
// that component.  these get mapped to UI_TEXTUREDRAW containers in the
// order that they appear
struct UI_GFXELEMENT
{
	int						m_eGfxElement;
	struct UI_COMPONENT*	m_pComponent;			// point back to component
	UIX_TEXTURE*			m_pTexture;

	DWORD					m_dwColor;
	UI_POSITION				m_Position;
	float					m_fZDelta;

	float					m_fWidth;
	float					m_fHeight;
	float					m_fXOffset;
	float					m_fYOffset;
	UI_RECT					m_ClipRect;
	UI_BLINKDATA			*m_pBlinkdata;

	int						m_nVertexCount;
	int						m_nIndexCount;

	BOOL					m_bGrayOut;		// draw with no colors

	QWORD					m_qwData;		// make sure there's room for 64 bit pointers if we need them

	UI_GFXELEMENT*			m_pNextElement;
	UI_GFXELEMENT*			m_pNextElementToDraw;
};

struct UI_ELEMENT_TEXT : UI_GFXELEMENT
{
	UIX_TEXTURE_FONT*		m_pFont;
	int						m_nFontSize;
	float					m_fDrawScale;		// on the off-chance we want to scale some text instead of generating at the proper size
	WCHAR*					m_pText;
	int						m_nAlignment;
	DWORD					m_dwFlags;			// see UI_ELEMENT_TEXT_FLAGS
	DWORD					m_dwDropshadowColor;
	int						m_nPage;
	int						m_nNumCharsOnPage;
	int						m_nMaxCharactersToDisplay;
};

struct UI_ARCSECTOR_DATA
{
	float					fAngleStartDeg;			// starting angle of the arcsector, in degrees, 0.0 == up, cw increasing
	float					fAngleWidthDeg;			// angular width of the entire arcsector, in degrees, cw increasing
	float					fSegmentWidthDeg;		// max angular width of a single segment, in degrees
	float					fRingRadiusPct;			// ring thickness radius ( pct of full radius )
};

// This sits in a GFXELEMENT_PRIM
struct UI_ELEMENT_PRIMITIVE : UI_GFXELEMENT
{
	int						m_nRenderSection;
	DWORD					m_dwFlags;
	UI_PRIMITIVE_TYPE		m_eType;

	// box			(uses only common attributes)
	UI_RECT					m_tRect;

	// arcsector	(special attributes)
	UI_ARCSECTOR_DATA		m_tArcData;
};

extern DWORD				g_dwRenderState[NUM_RENDER_SECTIONS];


BOOL UIXGraphicInit();

void UIXGraphicFree();

void UIGFXElementSetClipRect(
	UI_GFXELEMENT *element, 
	const UI_RECT *cliprect);

//float UIElementGetTextLogWidth(
//	UIX_TEXTURE_FONT* font,
//	int nFontSize,
//	float fWidthRatio,
//	BOOL bNoScale,
//	const WCHAR* text,
//	BOOL bSingleLine = FALSE,
//	int nPage = 0,
//	int nMaxCharacters = -1);
//
//float UIElementGetTextLogHeight(
//	UIX_TEXTURE_FONT* font,
//	int nFontSize,
//	const WCHAR* text,
//	BOOL bNoScale,
//	int nPage = 0);
struct UI_TXT_BKGD_RECTS {
	UI_RECT *pRectBuffer;
	int		 nBufferSize;
	int		 nNumRects;
};
BOOL UIElementGetTextLogSize(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio,
	BOOL bNoScale,
	const WCHAR* text,
	float *pfWidth,
	float *pfHeight,
	BOOL bSingleLineForWidth = FALSE,
	BOOL bSingleLineForHeight = FALSE,
	int nPage = 0,
	int nMaxCharacters = -1,
	UI_TXT_BKGD_RECTS * pRectStruct = NULL);

float UIGetCharWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio,
	BOOL bNoScale,
	WCHAR wc);

BOOL UIGetStringTagSize(
	const WCHAR *szText, 
	UI_COMPONENT *pComponent, 
	UI_SIZE *pTagSize, 
	int *pnNumChars,
	BOOL *pbForDrawOnly);

void UIElementGetScreenRect(
	UI_GFXELEMENT* element,
	UI_RECT* screenrect);

void UIComponentAddFrame(
	UI_COMPONENT* component);

UI_GFXELEMENT* UIComponentAddElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture = NULL,
	UI_TEXTURE_FRAME* frame = NULL,
	UI_POSITION position = UI_POSITION(),
	DWORD dwColor = GFXCOLOR_HOTPINK,
	const UI_RECT* cliprect = NULL,
	BOOL bDefinitelyNoStretch = FALSE,
	FLOAT fScaleX = -1.0f,
	FLOAT fScaleY = -1.0f,
	UI_SIZE *pOverrideSize = NULL);

UI_ELEMENT_TEXT* UIComponentAddTextElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	const WCHAR* text,
	UI_POSITION position,
	DWORD color = GFXCOLOR_WHITE,
	const UI_RECT* cliprect = NULL,
	int alignment = UIALIGN_TOPLEFT,
	UI_SIZE* overridesize = NULL,
	BOOL bDropShadow = TRUE,
	int nPage = 0,
	float fScale = 1.0f);

// doesn't make a copy of the text to a new MALLOCed buffer (don't use when
//  passing in a stack pointer as the element will try to free it later.
UI_ELEMENT_TEXT* UIComponentAddTextElementNoCopy(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	WCHAR* text,
	UI_POSITION position,
	DWORD color = GFXCOLOR_WHITE,
	const UI_RECT* cliprect = NULL,
	int alignment = UIALIGN_TOPLEFT,
	UI_SIZE* overridesize = NULL,
	BOOL bDropShadow = TRUE,
	int nPage = 0,
	float fScale = 1.0f);

void UITextElementSetEarlyTerminator(
	UI_GFXELEMENT *pGFXElement,
	int nNumChars);

int UITextElementGetNumCharsOnPage(
	UI_GFXELEMENT *pGFXElement);

UI_GFXELEMENT* UIComponentAddModelElement(
	UI_COMPONENT* component,
	int nAppearanceDefIDForCamera,
	int nModelID,
	UI_RECT rect,
	float fRotation,
	int	nModelCam = 0,
	UI_RECT * pClipRect = NULL,
	struct UI_INV_ITEM_GFX_ADJUSTMENT* pAdjust = NULL);

UI_GFXELEMENT* UIComponentAddItemGFXElement(
	UI_COMPONENT* component,
	UNIT *pItem,
	UI_RECT rect,
	UI_RECT * pClipRect = NULL,
	struct UI_INV_ITEM_GFX_ADJUSTMENT* pAdjust = NULL);

UI_GFXELEMENT* UIComponentAddCallbackElement(
	UI_COMPONENT* component,
	UI_POSITION & pos,
	float fWidth,
	float fHeight,
	float fZDelta,
	float fScale,
	UI_RECT * pClipRect );

UI_ELEMENT_PRIMITIVE* UIComponentAddPrimitiveElement(
	UI_COMPONENT* component,
	UI_PRIMITIVE_TYPE eType,
	DWORD dwFlags,
	UI_RECT tRect,
	struct UI_ARCSECTOR_DATA * ptArcData = NULL,
	UI_RECT * pClipRect = NULL,
	DWORD dwColor = GFXCOLOR_WHITE);

UI_GFXELEMENT* UIComponentAddFullTextureElement(
	UI_COMPONENT* component, 
	int nEngineTexureID, 
	UI_POSITION & pos, 
	float fWidth, 
	float fHeight, 
	float fZDelta = 0.0f, 
	UI_RECT * pClipRect = NULL);

UI_GFXELEMENT* UIComponentAddBoxElement(
	UI_COMPONENT* component,
	float fX1,
	float fY1,
	float fX2,
	float fY2,
	UI_TEXTURE_FRAME* frame = NULL,
	DWORD dwColor = GFXCOLOR_WHITE,
	BYTE bAlpha = 255,
	UI_RECT* cliprect = NULL);

UI_GFXELEMENT* UIComponentAddRotatedElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture = NULL,
	UI_TEXTURE_FRAME* frame = NULL,
	UI_POSITION position = UI_POSITION(),
	DWORD dwColor = GFXCOLOR_HOTPINK,
	float fAngle = 0.0f,
	UI_POSITION RotationCenter = UI_POSITION(),
	UI_RECT* cliprect = NULL,
	BOOL bDefinitelyNoStretch = FALSE,
	FLOAT fScale = 1.0f,
	DWORD dwRotationFlags = 0);

UI_GFXELEMENT* UIComponentAddTriElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UI_TEXTURE_FRAME* frame,
	UI_TEXTURETRI* tri,
	DWORD color = GFXCOLOR_WHITE,
	UI_RECT* cliprect = NULL);


UI_GFXELEMENT* UIComponentAddRectElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UI_TEXTURE_FRAME* frame,
	UI_TEXTURERECT* rect,
	DWORD color = GFXCOLOR_WHITE,
	UI_RECT* cliprect = NULL);

void UIGfxElementFree(
	UI_COMPONENT * component);

void UIRenderToLocal(
	UI_COMPONENT *pComponentList,
	UI_COMPONENT *pCursor);

UNITID UIElementGetModelElementUnitID(
	UI_GFXELEMENT* pElement);

int UIDecodeTextTag(
	const WCHAR *szTagName,
	WCHAR *szContentsBuf = NULL,
	int nBuflen = 0);

const WCHAR * UIGetTextTag(
	int nTextTag);

BOOL UIReplaceStringTags(
	UI_COMPONENT *pComponent,
	UI_LINE *pLine);

void UIAddColorToString(
	WCHAR *szString,
	enum FONTCOLOR eColor,
	int nStrLen);

void UIAddColorReturnToString(
	WCHAR *szString,
	int nStrLen);

void UIColorCatString(
	WCHAR *szString,
	int nStrLen,
	enum FONTCOLOR eColor,
	const WCHAR *szStrToCat);

BOOL UIColorGlobalString(
	enum GLOBAL_STRING eGlobalString,
	enum FONTCOLOR eColor,
	WCHAR *puszBuffer,
	int nMaxBuffer);

UI_SIZE DoWordWrap(
	WCHAR* text, 
	float fMaxWidth, 
	UIX_TEXTURE_FONT *font, 
	BOOL bNoScale,
	int nFontSize,
	int nPage = 0,
	float fWidthRatio = 1.0f);

UI_SIZE DoWordWrapEx(
	WCHAR*& text, 
	UI_LINE * pLine,
	float fMaxWidth, 
	UIX_TEXTURE_FONT *font, 
	BOOL bNoScale,
	int nFontSize,
	int nPage = 0,
	float fWidthRatio = 1.0f,
	UI_TXT_BKGD_RECTS * pRectStruct = NULL,
	BOOL bForceWrap = FALSE);

void UITextStringReplaceFormattingTokens(
	WCHAR * szText);

BOOL UIStringTagGetIntParam(
	const WCHAR *szParamList,
	const WCHAR *szParamName,
	int &nValue);

BOOL UIStringTagGetStringParam(
	const WCHAR *szParamList,
	const WCHAR *szParamName,
	WCHAR *szBuf,
	int nBuflen);

int UICountPrintableChar(
	const WCHAR * text,
	int * printable);

void UISetHypertextBounds(
	UI_LINE* pLine, 	
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	BOOL bNoScale,
	float fLineHeight,
	float fX, 
	float fY );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIDebugGfxCountTextureDraws();

int UIDebugGfxCountTextureDrawsGarbage();

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void UISetNeedToRender(
	int nRenderSection)
{
	g_dwRenderState[nRenderSection] |= UIRS_NEED_UPDATE;	
}

inline void UIClearNeedToRender(
	int nRenderSection)
{
	g_dwRenderState[nRenderSection] &= ~UIRS_NEED_RENDER;
}
#endif // _UIX_GRAPHIC_H_
