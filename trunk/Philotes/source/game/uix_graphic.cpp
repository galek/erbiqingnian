// uix_graphic.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "uix.h"
#include "fontcolor.h"
#include "colors.h"
#include "uix_priv.h"
#include "uix_graphic.h"
#include "uix_components.h"
#include "uix_scheduler.h"
#include "uix_options.h"		// CHB 2007.09.27 - for UIOptionsNotifyRestoreEvent()
#include "e_particles_priv.h"
#include "e_primitive.h"
#include "e_ui.h"
#include "e_main.h"
#include "e_texture.h"
#include "e_profile.h"
#include "e_definition.h"
#include "e_environment.h"
#include "e_settings.h"
#include "e_particle.h"
#include "e_common.h"
#include "dictionary.h"
#include "c_font.h"
#include "prime.h"
#include "c_appearance.h"
#include "c_appearance_priv.h"
#include "complexmaths.h"
#include "stringtable.h"
#include "stack.h"
#include "c_input.h"
#include "c_QuestClient.h"
#include "inventory.h"
#include "language.h"
#include "uix_hypertext.h"

static const float	scfFontSizeScaleUp = 0.5f;

struct WSTR_DICT
{
	WCHAR		* str;
	int			value;
};


WSTR_DICT pTextTagEnumTbl[] =
{
	{ L"unit",				TEXT_TAG_UNIT },
	{ L"frame",				TEXT_TAG_FRAME },
	{ L"pagebreak",			TEXT_TAG_PGBRK },
	{ L"interrupt",			TEXT_TAG_INTERRUPT },
	{ L"videointerrupt",	TEXT_TAG_INTERRUPT_VIDEO },
	{ L"video",				TEXT_TAG_VIDEO },
	{ L"key",				TEXT_TAG_KEYNAME },
	{ L"color",				TEXT_TAG_COLOR },
	{ L"endcolor",			TEXT_TAG_COLOR_END },
	{ L"um",				TEXT_TAG_UNITMODE },
	{ L"endum",				TEXT_TAG_UNITMODE_END },
	{ L"talk",				TEXT_TAG_TALK },
	{ L"idle",				TEXT_TAG_TALK },
	{ L"angry",				TEXT_TAG_ANGRY },
	{ L"excited",			TEXT_TAG_EXCITED },
	{ L"proud",				TEXT_TAG_PROUD },
	{ L"sad",				TEXT_TAG_SAD },
	{ L"scared",			TEXT_TAG_SCARED },
	{ L"wave",				TEXT_TAG_WAVE },
	{ L"bow",				TEXT_TAG_BOW },
	{ L"give",				TEXT_TAG_GIVE },
	{ L"get",				TEXT_TAG_GET },
	{ L"pickup",			TEXT_TAG_PICKUP },
	{ L"secret",			TEXT_TAG_SECRET },
	{ L"kneel",				TEXT_TAG_KNEEL },
	{ L"turn",				TEXT_TAG_TURN },

	{ NULL,					-1		},
};

DWORD					g_dwRenderState[NUM_RENDER_SECTIONS];

// nodes in a list of textures to draw, since a given physical texture
// can appear in this list multiple times, we need to create containers
struct UI_TEXTUREDRAW
{
	UI_TEXTUREDRAW*			m_pNext;
	UIX_TEXTURE*			m_pTexture;
	int						m_nEngineBuffer;
	int						m_nLocalBuffer;
	int						m_nRenderSection;
	BOOL					m_bGrayout;
	int						m_nFullTextureID;		// exclusively for GFXELEMENT_FULLTEXTURE
};

#define MAX_UI_PRIM_DRAWLISTS			8

// This is a node in a list of "primitive lists" that we want the engine to render for the UI
struct UI_PRIMITIVEDRAW
{
	UI_PRIMITIVEDRAW*		m_pNext;
	DWORD					m_dwFlags;
	UI_PRIMITIVE_TYPE		m_eType;
	UI_RECT					m_Rect;
	UI_ARCSECTOR_DATA		m_tArcData;
	int						m_nEngineBuffer;
	int						m_nLocalBuffer;
	int						m_nRenderSection;
};

// This is a node in a list of models that we want the engine to render for the UI
struct UI_MODEL
{
	UI_MODEL*				m_pNext;
	int						m_nRenderSection;
	int						m_nAppearanceDefIDForCamera;
	int						m_nModelID;
	UI_RECT					m_tScreenRect;
	UI_RECT					m_tOrigRect;
	float					m_fRotation;
	int						m_nModelCam;
};

struct UI_CALLBACK_RENDER
{
	UI_CALLBACK_RENDER*		m_pNext;
	int						m_nRenderSection;
	UI_RECT					m_tScreenRect;
	UI_RECT					m_tOrigRect;
	DWORD					m_dwColor;
	float					m_fZDelta;
	float					m_fScale;
	PFN_UI_RENDER_CALLBACK	m_pfnCallback;
};

struct UI_ELEMENT_FRAME : UI_GFXELEMENT
{
	UI_TEXTURE_FRAME*		m_pFrame;
	DWORD					m_dwSchedulerTicket;
};

#define UI_TXT_BKGD_RECT_X_BUF	(2.0f)

//----------------------------------------------------------------------------
enum UI_ELEMENT_TEXT_FLAGS
{
	UETF_DROP_SHADOW_BIT,		// maybe make this not a flag later, but a dropshadow color??
};

//----------------------------------------------------------------------------
enum ELEMENT_FRAME_FLAGS
{
	EFF_ROTATE_ABOUT_SELF_BIT = 0,		// center of rotation position is the element itself
};

struct UI_ELEMENT_ROTFRAME : UI_ELEMENT_FRAME
{
	float					m_fAngle;
	UI_POSITION				m_RotationCenter;
	DWORD					m_dwElementFrameFlags;		// see ELEMENT_FRAME_FLAGS
};

struct UI_ELEMENT_TRI : UI_ELEMENT_FRAME
{
	UI_TEXTURETRI			m_Triangle;
};

struct UI_ELEMENT_RECT : UI_ELEMENT_FRAME
{
	UI_TEXTURERECT			m_Rect;
};

// This sits in a GFXELEMENT_MODEL
struct UI_ELEMENT_MODEL : UI_GFXELEMENT
{
	int						m_nRenderSection;
	int						m_nAppearanceDefIDForCamera;
	int						m_nModelID;
	float					m_fRotation;
	int						m_nModelCam;
	int						m_nUnitID;
};

struct UI_ELEMENT_CALLBACK : UI_GFXELEMENT
{
	PFN_UI_RENDER_CALLBACK	m_pfnRender;
	float					m_fScale;
	//int						m_nUpdateID;		// user payload for tracking and updating
};

struct UI_ELEMENT_FULLTEXTURE : UI_GFXELEMENT
{
	int						m_nEngineTextureID;
	int						m_nUnitID;
};

STR_DICT pElementTypeEnumTbl[] =
{
	{ "Frame",		GFXELEMENT_FRAME },
	{ "Text",		GFXELEMENT_TEXT },
	{ "Tri",		GFXELEMENT_TRI },
	{ "Rect",		GFXELEMENT_RECT },
	{ "RotFrame",	GFXELEMENT_ROTFRAME },
	{ "Model",		GFXELEMENT_MODEL },
	{ "Prim",		GFXELEMENT_PRIM },
	{ "Callback",	GFXELEMENT_CALLBACK },
	{ "FullTexture",GFXELEMENT_FULLTEXTURE },

	{ NULL,				-1		},
};

#define ELEM_CAST_FUNC(type, strct, name)		inline strct* UIElemCastTo##name(UI_GFXELEMENT* element) \
												{ ASSERT(element->m_eGfxElement == type); \
													return (strct*)element; }

ELEM_CAST_FUNC(GFXELEMENT_FRAME,		UI_ELEMENT_FRAME,		Frame);
ELEM_CAST_FUNC(GFXELEMENT_TEXT,			UI_ELEMENT_TEXT,		Text);
ELEM_CAST_FUNC(GFXELEMENT_TRI,			UI_ELEMENT_TRI,			Tri);
ELEM_CAST_FUNC(GFXELEMENT_RECT,			UI_ELEMENT_RECT,		Rect);
ELEM_CAST_FUNC(GFXELEMENT_ROTFRAME,		UI_ELEMENT_ROTFRAME,	RotFrame);
ELEM_CAST_FUNC(GFXELEMENT_MODEL,		UI_ELEMENT_MODEL,		Model);
ELEM_CAST_FUNC(GFXELEMENT_PRIM,			UI_ELEMENT_PRIMITIVE,	Prim);
ELEM_CAST_FUNC(GFXELEMENT_CALLBACK,		UI_ELEMENT_CALLBACK,	Callback);
ELEM_CAST_FUNC(GFXELEMENT_FULLTEXTURE,	UI_ELEMENT_FULLTEXTURE,	FullTexture);

size_t sGetElementSize(int nElemType)
{
	switch (nElemType)
	{
	case GFXELEMENT_FRAME: return sizeof(UI_ELEMENT_FRAME);
	case GFXELEMENT_TEXT: return sizeof(UI_ELEMENT_TEXT);
	case GFXELEMENT_TRI: return sizeof(UI_ELEMENT_TRI);
	case GFXELEMENT_RECT: return sizeof(UI_ELEMENT_RECT);
	case GFXELEMENT_ROTFRAME: return sizeof(UI_ELEMENT_ROTFRAME);
	case GFXELEMENT_MODEL: return sizeof(UI_ELEMENT_MODEL);
	case GFXELEMENT_PRIM: return sizeof(UI_ELEMENT_PRIMITIVE);
	case GFXELEMENT_CALLBACK: return sizeof(UI_ELEMENT_CALLBACK);
	case GFXELEMENT_FULLTEXTURE: return sizeof(UI_ELEMENT_FULLTEXTURE);
	default:
		ASSERT(TRUE);
		break;
	}
	return 0;
}

// static variable used by draw loop
struct UI_DRAW
{
	UIX_TEXTURE*			m_pTexture;
	UI_GFXELEMENT*			m_pElementsToDrawFirst;
	UI_GFXELEMENT*			m_pElementsToDrawLast;
	int						m_nVertexCount;
	int						m_nIndexCount;
	BOOL					m_bGrayOut;					// draw all elements in this list with no colors
};

struct UI_PRIMDRAW
{
	DWORD					m_dwFlags;
	UI_ELEMENT_PRIMITIVE*	m_pElementsToDrawFirst;
	UI_ELEMENT_PRIMITIVE*	m_pElementsToDrawLast;
	int						m_nVertexCount;
	int						m_nIndexCount;
};

struct UI_GRAPHIC_GLOBALS
{
	UI_TEXTUREDRAW*			m_pTextureToDrawFirst;		// render list
	UI_TEXTUREDRAW*			m_pTextureToDrawLast;
	UI_MODEL*				m_pModelToDrawFirst;
	UI_MODEL*				m_pModelToDrawLast;
	UI_PRIMITIVEDRAW*		m_pPrimToDrawFirst;
	UI_PRIMITIVEDRAW*		m_pPrimToDrawLast;
	UI_CALLBACK_RENDER*		m_pCallbackToDrawFirst;
	UI_CALLBACK_RENDER*		m_pCallbackToDrawLast;

	// just going to manage a garbage list in engine
	UI_TEXTUREDRAW*			m_pTextureToDrawGarbage;
	UI_MODEL*				m_pModelToDrawGarbage;
	UI_PRIMITIVEDRAW*		m_pPrimToDrawGarbage;
	UI_CALLBACK_RENDER*		m_pCallbackToDrawGarbage;
	UI_GFXELEMENT*			m_pGfxElementGarbage[NUM_GFXELEMENTTYPES];

	UI_DRAW					m_pTextureDrawLists[MAX_UI_TEXTURES];
	BOOL					m_bReorderDrawLists;
	UI_PRIMDRAW				m_pPrimDrawLists[MAX_UI_PRIM_DRAWLISTS];

	BOOL					m_bHideUI;
};


UI_GRAPHIC_GLOBALS	g_GraphicGlobals;

void UIDrawListAddModel(
	int nRenderSection,
	int nAppearanceDefIDForCamera,
	int nModelID,
	UI_RECT& tOrigRect,
	UI_RECT& tClipRect,
	float fRotation,
	int nModelCam);

void UIDrawListAddCallback(
	int nRenderSection,
	PFN_UI_RENDER_CALLBACK pfnRender,
	UI_RECT& drawrect,
	UI_RECT& cliprect,
	float fZDelta,
	float fScale,
	DWORD dwColor );

BOOL UIElementChunkGetCoords(
	UI_GFXELEMENT* element,
	UI_TEXTURE_FRAME* frame,
	UI_POSITION* posComponentAbsoluteScreen,
	UI_RECT* drawrect,
	UI_RECT* screencliprect,
	UI_TEXRECT* texrect,
	int nChunkNum,
	float fHorizStretchPct,
	float fVertStretchPct);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIToggleHideUI()
{
	g_GraphicGlobals.m_bHideUI = !g_GraphicGlobals.m_bHideUI;
	UIRepaint();
	return g_GraphicGlobals.m_bHideUI;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_POSITION UIComponentGetAbsoluteScreenPosition(
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
			if (component->m_bScrollsWithParent)
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
		}

		component = component->m_pParent;
	}

	posScaled.m_fX = posScaled.m_fX * UIGetLogToScreenRatioX();
	posScaled.m_fY = posScaled.m_fY * UIGetLogToScreenRatioY();

	UI_POSITION posReturn(posScaled.m_fX + posUnScaled.m_fX, posScaled.m_fY + posUnScaled.m_fY);
	return posReturn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void UIElementGetClipRect(
	const UI_GFXELEMENT* element,
	const UI_POSITION* pos,
	UI_RECT* cliprect)
{
	ASSERT_RETURN(element);
	ASSERT_RETURN(element->m_pComponent);
	ASSERT_RETURN(pos);
	ASSERT_RETURN(cliprect);

	if (AppTestDebugFlag( ADF_UI_NO_CLIPPING ))
	{
		cliprect->m_fX1 = -UIDefaultWidth();
		cliprect->m_fY1 = -UIDefaultHeight();
		cliprect->m_fX2 = UIDefaultWidth();
		cliprect->m_fY2 = UIDefaultHeight();
		return;		
	}

	UI_RECT rectElementClip = element->m_ClipRect;

	UI_POSITION posComponents;
	UI_COMPONENT *pComponent = element->m_pComponent;
	while (pComponent)
	{
		if (pComponent->m_rectClip.m_fX1 != 0.0f || 
			pComponent->m_rectClip.m_fX2 != 0.0f || 
			pComponent->m_rectClip.m_fY1 != 0.0f || 
			pComponent->m_rectClip.m_fY2 != 0.0f)
		{
			UI_RECT rectComponentClip = pComponent->m_rectClip;
			rectComponentClip.m_fX1 += posComponents.m_fX + pComponent->m_ScrollPos.m_fX;
			rectComponentClip.m_fX2 += posComponents.m_fX + pComponent->m_ScrollPos.m_fX;
			rectComponentClip.m_fY1 += posComponents.m_fY + pComponent->m_ScrollPos.m_fY;
			rectComponentClip.m_fY2 += posComponents.m_fY + pComponent->m_ScrollPos.m_fY;
			if (!UIUnionRects(rectElementClip, rectComponentClip, rectElementClip))
			{
				//structclear(element->m_pComponent->m_rectClip);
				return;
			}
		}

		posComponents.m_fX -= pComponent->m_Position.m_fX;
		posComponents.m_fY -= pComponent->m_Position.m_fY;

		pComponent = pComponent->m_pParent;
	}

	ASSERT_RETURN(element->m_pComponent);

	cliprect->m_fX1 = pos->m_fX + (rectElementClip.m_fX1 * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	cliprect->m_fY1 = pos->m_fY + (rectElementClip.m_fY1 * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));
	cliprect->m_fX2 = cliprect->m_fX1 + ((rectElementClip.m_fX2 - rectElementClip.m_fX1) * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	cliprect->m_fY2 = cliprect->m_fY1 + ((rectElementClip.m_fY2 - rectElementClip.m_fY1) * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGetTextAlignmentScreenspaceAdj(
	UI_GFXELEMENT* element, 
	const WCHAR* text,
	float fTextWidthRatio,
	int alignment,
	float* x,
	float* y)
{
	UI_ELEMENT_TEXT *pTextElement = UIElemCastToText(element);

	float fTextLogToScreenRatioX = UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScaleFont);	
	float fTextLogToScreenRatioY = UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScaleFont);

	int nPage = 0;
	if (UIComponentIsLabel(element->m_pComponent))
	{
		UI_LABELEX *pLabel = UICastToLabel(element->m_pComponent);
		nPage = pLabel->m_nPage;
	}

	float fTextWidth = 0.0f;
	float fTextHeight = 0.0f;
	UIElementGetTextLogSize(pTextElement->m_pFont, 
		pTextElement->m_nFontSize, 
		fTextWidthRatio, 
		element->m_pComponent->m_bNoScaleFont, 
		text, 
		&fTextWidth, 
		&fTextHeight,
		TRUE,
		(y ? FALSE : TRUE),		// if we need the vertical adjustment don't do single-line.  Otherwise we only want the one line.
		nPage);
	fTextWidth *= fTextLogToScreenRatioX;
	fTextHeight *= fTextLogToScreenRatioY;
	float fComponentWidth = element->m_fWidth * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale);	
	float fComponentHeight = element->m_fHeight * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale);

	if (x)
	{
		switch (alignment)
		{
		case UIALIGN_TOP:
		case UIALIGN_CENTER:
		case UIALIGN_BOTTOM:
			*x = (fComponentWidth - fTextWidth) / 2.0f;		// center it
			break;
		case UIALIGN_TOPRIGHT:
		case UIALIGN_RIGHT:
		case UIALIGN_BOTTOMRIGHT:
			*x = fComponentWidth - fTextWidth;				// put it on the right
			break;
		default:
			*x = 0.0f;
			break;
		}
	}

	if (y)
	{
		switch (alignment)
		{
		case UIALIGN_CENTER:
		case UIALIGN_LEFT:
		case UIALIGN_RIGHT:
			*y = (fComponentHeight - fTextHeight) / 2.0f;	// center it
			*y = MAX(0.0f, *y);		// pin it to the top.  Note: this should probably only stay for TGS.
			break;
		case UIALIGN_BOTTOM:
		case UIALIGN_BOTTOMLEFT:
		case UIALIGN_BOTTOMRIGHT:
			*y = fComponentHeight - fTextHeight;
			break;
		default:
			*y = 0.0f;
			break;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This function checks the font's glyph set to see if it contains the 
//  character we're trying to print.  If not, it returns a '?'
// Not really working with korean fonts, so ignore this function for now      -rli
static inline WCHAR sUICheckGlyphset(
	UIX_TEXTURE_FONT * font,
	WCHAR wc)
{
#if 0
	if (/*!PStrIsPrintable(wc) || */!font->m_pGlyphset)
	{
		return wc;
	}

	for (UINT iRange = 0; iRange < font->m_pGlyphset->cRanges; iRange++)
	{
		WCRANGE *pRange = ((WCRANGE *)font->m_pGlyphset->ranges) + iRange;
		if (wc >= pRange->wcLow && wc < pRange->wcLow + pRange->cGlyphs)
		{
			return wc;
		}
	}

	return L'?';
#else
	UNREFERENCED_PARAMETER(font);
	return wc;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline WCHAR sUIJabberChar(
	UIX_TEXTURE_FONT * font,
	WCHAR wc)
{
#if ISVERSION(DEVELOPMENT)
	//if (AppTestDebugFlag(ADF_JABBERWOCIZE))
	//{
	//	if (!font->m_pGlyphset ||/* !PStrIsPrintable(wc)*/)
	//	{
	//		return wc;
	//	}

	//	if (AppTestDebugFlag(ADF_JABBERWOCIZE))
	//	{
	//		RAND rand;
	//		RandInit(rand, (DWORD)wc, 0);

	//		do
	//		{
	//			UINT iRange = RandGetNum(rand, 0, font->m_pGlyphset->cRanges-1);
	//			WCRANGE *pRange = ((WCRANGE *)font->m_pGlyphset->ranges) + iRange;
	//			UINT iChar = RandGetNum(rand, 0, pRange->cGlyphs-1);

	//			wc = (WCHAR)(pRange->wcLow + iChar);
	//		} while (!PStrIsPrintable(wc));
	//	}
	//}
#endif
	return wc;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UIElementGetTextColumnPoint(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio,
	BOOL bNoScale,
	const WCHAR* text,
	BOOL bSingleLine = FALSE)
{
	float maxcolumnpoint = 0.0f;
	float columnpoint = 0.0f;
	float width = 0.0f;
	float fTabWidth = font->m_fTabWidth * fWidthRatio;

	WCHAR wc;
	while ((wc = *text++) != 0)
	{
		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR || wc == CTRLCHAR_COLOR_RETURN)
		{
			if (*text == 0)
			{
				break;
			}
			text++;
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT || wc == CTRLCHAR_HYPERTEXT_END)
		{
			text += (wc == CTRLCHAR_HYPERTEXT)? 1 : 0;
			continue;
		}

		if (wc == L'\n')
		{
			width = 0.0f;
			if (bSingleLine)
			{
				return columnpoint;
			}
			maxcolumnpoint = MAX(columnpoint, maxcolumnpoint);
			columnpoint = 0.0f;
			continue;
		}
		if (wc == L'\t')
		{
			width = float(int( width / fTabWidth + 0.5f) + 1 ) * fTabWidth;
			continue;
		}
		if (wc == L'\f')
		{
			columnpoint = width;
			continue;
		}
		if (PStrIsPrintable( wc ) == FALSE)		
		{
			continue;
		}

		wc = sUIJabberChar(font, wc);
		wc = sUICheckGlyphset(font, wc);

		width += UIGetCharWidth(font, nFontSize, fWidthRatio, bNoScale, wc);
	}
	maxcolumnpoint = MAX(columnpoint, maxcolumnpoint);
	return maxcolumnpoint;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sUITextureUpdateAlpha(
	UI_GFXELEMENT * element,
	BYTE * pbAlpha = NULL)
{
	ASSERT_RETZERO(element);
	DWORD dwColor = element->m_dwColor;
	BYTE bAlpha = UI_GET_ALPHA(dwColor);
	float fFading = UIComponentGetFade(element->m_pComponent);
	if (bAlpha != 255 || fFading != 0.0f)
	{
		bAlpha = (BYTE)(bAlpha - (int)((float)bAlpha * fFading));
		dwColor = UI_MAKE_COLOR(bAlpha, dwColor);
	}
	if (pbAlpha)
	{
		*pbAlpha = bAlpha;
	}
	return dwColor;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIStringTagGetIntParam(
	const WCHAR *szParamList,
	const WCHAR *szParamName,
	int &nValue)
{
	const WCHAR * szParamStart = PStrStrI(szParamList, szParamName);
	if (!szParamStart)
	{
		return FALSE;
	}

	szParamStart += PStrLen(szParamName);
	if (!szParamStart || !szParamStart[0])
	{
		return FALSE;
	}

	WCHAR szNum[32];
	int i=0;
	while (iswdigit(szParamStart[i]))
	{
		szNum[i] = szParamStart[i];
		i++;
	}
	if (i == 0)
	{
		return FALSE;
	}

	szNum[i] = L'\0';
	nValue = PStrToInt(szNum);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIStringTagGetStringParam(
	const WCHAR *szParamList,
	const WCHAR *szParamName,
	WCHAR *szBuf,
	int nBuflen)
{
	memclear(szBuf, sizeof(WCHAR) * nBuflen);

	const WCHAR * szParamStart = PStrStrI(szParamList, szParamName);
	if (!szParamStart)
	{
		return FALSE;
	}

	szParamStart += PStrLen(szParamName);
	if (!szParamStart || !szParamStart[0])
	{
		return FALSE;
	}

	int i=0;
	while (szParamStart[i] && i < nBuflen && szParamStart[i] != L',')
	{
		szBuf[i] = szParamStart[i];
		i++;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIAddStringTagElement( 
	UI_COMPONENT *pComponent,
	float x,
	float y,
	UI_SIZE size,
	int nTextTag,
	const WCHAR * szParams)
{
	if (nTextTag == TEXT_TAG_INVALID)
	{
		return FALSE;
	}

	// TRAVIS: Removing this - seems to cause issues with noscale elements.
	//x /= UIGetLogToScreenRatioX(!pComponent->m_bNoScaleFont);
	//size.m_fWidth /= UIGetLogToScreenRatioX(!pComponent->m_bNoScaleFont);
	//y /= UIGetLogToScreenRatioY(!pComponent->m_bNoScaleFont);
	//size.m_fHeight /= UIGetLogToScreenRatioY(!pComponent->m_bNoScaleFont);
	
	if (nTextTag == TEXT_TAG_UNIT)
	{
		int nID = INVALID_ID;
		if (!UIStringTagGetIntParam(szParams, L"id=", nID))
		{
			return FALSE;
		}

		GAME *pGame = AppGetCltGame();
		if (!pGame)
		{
			return FALSE;
		}
		UNIT *pUnit = UnitGetById(pGame, nID);
		if (!pUnit)
		{
			return FALSE;
		}

		int nModelID = c_UnitGetModelIdInventory( pUnit );
		int nAppearanceID = UnitGetAppearanceDefId( pUnit );

		if (nModelID == INVALID_ID || nAppearanceID == INVALID_ID)
		{
			return FALSE;
		}

		UI_RECT drawrect( x, y, x + size.m_fWidth, y + size.m_fHeight );
		//drawrect.m_fX1 /= UIGetLogToScreenRatioX(!pComponent->m_bNoScaleFont);
		//drawrect.m_fX2 /= UIGetLogToScreenRatioX(!pComponent->m_bNoScaleFont);
		//drawrect.m_fY1 /= UIGetLogToScreenRatioY(!pComponent->m_bNoScaleFont);
		//drawrect.m_fY2 /= UIGetLogToScreenRatioY(!pComponent->m_bNoScaleFont);
		//
		// clip to the component
		UI_RECT rectClip = UIComponentGetRect(pComponent);

		UI_ELEMENT_MODEL *pElement = (UI_ELEMENT_MODEL *)UIComponentAddModelElement(pComponent, nAppearanceID, nModelID, drawrect, 0.0f, 0, &rectClip);
		if (pElement)
		{
			pElement->m_nUnitID = nID;
		}

		// we may want to make this conditional
		if (0)  // disable until primitives no longer overwrite models
		{
			if (UnitIsA(pUnit, UNITTYPE_ITEM))
			{
				UNIT *pPlayerUnit = GameGetControlUnit(AppGetCltGame());
				if (!InventoryItemMeetsClassReqs(pUnit, pPlayerUnit))
				{
					DWORD dwFlags = 0;
					SETBIT(dwFlags, UI_PRIM_ALPHABLEND);
					SETBIT(dwFlags, UI_PRIM_ZTEST);
					UIComponentAddPrimitiveElement(pComponent, UIPRIM_BOX, dwFlags, drawrect, NULL, &rectClip, UI_MAKE_COLOR(128, GFXCOLOR_RED));
				}
			}
		}

		return TRUE;
	}

	if (nTextTag == TEXT_TAG_FRAME)
	{
		WCHAR szwFrame[256];
		if (!UIStringTagGetStringParam(szParams, L"name=", szwFrame, 256))
		{
			return FALSE;
		}
		char szFrame[256];
		PStrCvt(szFrame, szwFrame, 256);

		UIX_TEXTURE *pTexture = NULL;
		WCHAR szwTexture[256];
		if (UIStringTagGetStringParam(szParams, L"texture=", szwTexture, 256))
		{
			char szTexture[256];
			PStrCvt(szTexture, szwTexture, 256);
			pTexture = (UIX_TEXTURE *)StrDictionaryFind(g_UI.m_pTextures, szTexture);
		}
		if (!pTexture)
		{
			pTexture = UIComponentGetTexture(pComponent);
		}
		if (!pTexture)
		{
			return FALSE;
		}
		UI_TEXTURE_FRAME *pFrame = (UI_TEXTURE_FRAME *)StrDictionaryFind(pTexture->m_pFrames, szFrame);
		if (!pFrame)
		{
			return FALSE;
		}

		// clip to the component
		UI_RECT rectClip = UIComponentGetRectBase(pComponent);
		UIComponentAddElement(pComponent, pTexture, pFrame, UI_POSITION(x, y), GFXCOLOR_HOTPINK, &rectClip, FALSE, g_UI.m_fUIScaler, g_UI.m_fUIScaler, &size);

		return TRUE;
	}

	return FALSE;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_SIZE UIGetDefaultTagSize(
	int nTextTag) 
{
	switch(nTextTag)
	{
	case TEXT_TAG_FRAME:	return UI_SIZE(32.0f, 32.0f);
	case TEXT_TAG_UNIT:		return UI_SIZE(64.0f, 64.0f);
	}

	return UI_SIZE(0.0f, 0.0f);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIDecodeTextTag(
	const WCHAR *szTagName,
	WCHAR *szContentsBuf /*= NULL*/,
	int nBuflen /*= 0*/) 
{
	if (!szTagName)
	{
		return TEXT_TAG_INVALID;
	}

	if (*szTagName == TEXT_TAG_BEGIN_CHAR)
	{
		szTagName++;
	}

	if (szContentsBuf)
	{
		szContentsBuf[0] = L'\0';
	}

	for (int i=0; i < TEXT_TAG_COUNT; i++)
	{
		int nTaglen = 0;
		const WCHAR *szTemp = szTagName;
		while (*szTemp != L'\0' && *szTemp != L']' && *szTemp != L' ')		// skip to the next space
		{
			nTaglen++;
			szTemp++;
		}
		if (PStrICmp(szTagName, pTextTagEnumTbl[i].str, MAX((int)PStrLen(pTextTagEnumTbl[i].str), nTaglen)) == 0)
		{
			if (szContentsBuf)
			{
				// return the rest of the tag
				int nLen = 0;
				WCHAR wc = *szTagName++;
				while (wc != L'\0' && wc != L']' && wc != L' ')		// skip to the next space
				{
					wc = *szTagName++;
				}
				while (wc != L'\0' && wc != L']' && wc == L' ')		// skip the spaces
				{
					wc = *szTagName++;
				}
				while (wc != L'\0' && wc != L']' && nLen < nBuflen)		// copy until the end brace
				{
					if (nLen < nBuflen - 1)
					{
						szContentsBuf[nLen] = wc;
					}
					wc = *szTagName++;
					nLen++;
				}
				szContentsBuf[nLen] = L'\0';
			}
			return pTextTagEnumTbl[i].value;
		}
	}

	return TEXT_TAG_INVALID;
}

const WCHAR * UIGetTextTag(
	int nTextTag)
{
	ASSERT_RETVAL(nTextTag >= 0 && nTextTag < TEXT_TAG_COUNT, L"");

	return pTextTagEnumTbl[nTextTag].str;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIGetStringTagSize(
	const WCHAR *szText, 
	UI_COMPONENT *pComponent, 
	UI_SIZE *pTagSize, 
	int *pnNumChars, 
	BOOL *pbForDrawOnly)
{
	if (!pTagSize)
	{
		return FALSE;
	}

	if (!szText || !szText[0])
	{
		return FALSE;
	}

	WCHAR szTemp[1024];
	PStrCopy(szTemp, szText, 1024);

	WCHAR *szEndBracket = PStrPbrk(szTemp, L"]");
	if (!szEndBracket)
	{
		// No end bracket, give up
		return FALSE;
	}

	if (pnNumChars)
	{
		*pnNumChars = (int)(szEndBracket - szTemp) + 1;
	}

	WCHAR *szTagName = szTemp;
	WCHAR *szParams = NULL;
	WCHAR *szEndTagName = PStrPbrk(szTemp, L" ]");
	if (*szEndTagName == L' ')
	{
		szParams = szEndTagName + 1;
		*szEndBracket = L'\0';
	}

	*szEndTagName = L'\0';

	int nTextTag = UIDecodeTextTag(szTagName);
	if (nTextTag == TEXT_TAG_INVALID)
	{
		return FALSE;
	}

	if (pbForDrawOnly)
	{
		UIStringTagGetIntParam(szParams, L"undertext=", *pbForDrawOnly);
	}

	*pTagSize = UIGetDefaultTagSize(nTextTag);

	int nVal = 0;
	if (UIStringTagGetIntParam(szParams, L"sx=", nVal))
	{
		pTagSize->m_fWidth = (float)nVal;
	}
	if (UIStringTagGetIntParam(szParams, L"sy=", nVal))
	{
		pTagSize->m_fHeight = (float)nVal;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sReplaceTagWithText(
	const WCHAR* szText,
	int &nPos,
	const WCHAR *szReplacement,
	WCHAR* &szNewText,
	WCHAR* &szNewTextPos)
{
	if (szNewText == NULL)
	{
		int nStrLen = PStrLen(szText);
		szNewText = MALLOC_NEWARRAY(NULL, WCHAR, nStrLen * 2);
		memcpy(szNewText, szText, nPos * sizeof(WCHAR));				// copy the portion we've already passed
		szNewTextPos = szNewText + nPos;
	}
	
	ASSERT_RETURN(szNewTextPos);
	const WCHAR *szSrc = szReplacement;
	while (*szSrc != L'\0')
	{
		*szNewTextPos++ = *szSrc++;
	}

	while( szText[nPos] != L'\0' && szText[nPos] != L']')
	{
		nPos++;
	}

}

BOOL UIReplaceStringTags(
	UI_COMPONENT *pComponent,
	UI_LINE *pLine)
{
	//ASSERT_RETFALSE(_CrtIsValidHeapPointer(szText));

 	if (!pLine || !pLine->HasText())
	{
		return FALSE;
	}

	const WCHAR *szText = pLine->GetText();
	WCHAR *szNewText = NULL;
	WCHAR *szNewTextPos = NULL;
	BOOL bDidAReplacement = FALSE;

	WCHAR szContents[64];
	int nPos = 0;
	while (szText[nPos] != L'\0')
	{
		bDidAReplacement = FALSE;
		if (szText[nPos] == TEXT_TAG_BEGIN_CHAR)
		{
			int nTextTag = UIDecodeTextTag(&szText[nPos], szContents, arrsize(szContents));
			switch (nTextTag)
			{
			case TEXT_TAG_KEYNAME:
				{
					char szBuf[64];
					PStrCvt(szBuf, szContents, arrsize(szBuf));
					const KEY_COMMAND *pKey = InputGetKeyCommand(szBuf);
					if (pKey)
					{
						WCHAR szKeyName[256];
						if (InputGetKeyNameW(pKey->KeyAssign[0].nKey, pKey->KeyAssign[0].nModifierKey, szKeyName, arrsize(szKeyName)))
						{
							sReplaceTagWithText(szText, nPos, szKeyName, szNewText, szNewTextPos);
							bDidAReplacement = TRUE;
							if (UIComponentIsLabel(pComponent))
							{
								UI_LABELEX *pLabel = UICastToLabel(pComponent);
								pLabel->m_bTextHasKey = TRUE;
							}
						}
					}
				}
				break;
			case TEXT_TAG_COLOR:
				{
					if (!szContents || szContents[0] == L'0')
						break;

					char szColorName[256];
					PStrCvt(szColorName, szContents, arrsize(szColorName));
					int nColor = ExcelGetLineByStringIndex(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_FONTCOLORS, szColorName);
					if (nColor == INVALID_LINK)
						break;

					WCHAR szColorCode[32] = L"\0";
					PStrCatChar(szColorCode, (WCHAR)CTRLCHAR_COLOR, arrsize(szColorCode));
					PStrCatChar(szColorCode, (WCHAR)nColor, arrsize(szColorCode));
					sReplaceTagWithText(szText, nPos, szColorCode, szNewText, szNewTextPos);
					bDidAReplacement = TRUE;
				}
				break;
			case TEXT_TAG_COLOR_END:
				WCHAR szColorCode[2] = {(WCHAR)CTRLCHAR_COLOR_RETURN, L'\0'};
				sReplaceTagWithText(szText, nPos, szColorCode, szNewText, szNewTextPos);
				bDidAReplacement = TRUE;
				break;
			}
		}
		
		if (!bDidAReplacement && szNewText != NULL && szNewTextPos != NULL)
		{
			 *szNewTextPos++ = szText[nPos];
		}

		nPos++;
	}

	if (szNewText != NULL)
	{
		*szNewTextPos++ = 0;
		pLine->SetText(szNewText);
		FREE_DELETE_ARRAY(NULL, szNewText, WCHAR);
	}

	if( AppIsTugboat() )
	{
		c_QuestFillDialogText( UIGetControlUnit(), pLine );
/*		szNewText = NULL;
		c_QuestFillDialogText( UIGetControlUnit(), szText, szNewText );
		if (szNewText != NULL)
		{
			pLine->SetText(szNewText);
			FREE_DELETE_ARRAY(NULL, szNewText, WCHAR);
		}
*/
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIProcessStringTag(
	UI_COMPONENT *pComponent,
	const WCHAR *szText,
	float x,
	float y,
	UI_SIZE *pTagSize, 
	int *pnNumChars,
	BOOL *pbForDrawOnly)
{
	if (!pTagSize)
	{
		return FALSE;
	}

	if (!szText || !szText[0])
	{
		return FALSE;
	}

	WCHAR szTemp[1024];
	PStrCopy(szTemp, szText, 1024);

	WCHAR *szEndBracket = PStrPbrk(szTemp, L"]");
	if (!szEndBracket)
	{
		// No end bracket, give up
		return FALSE;
	}

	if (pnNumChars)
	{
		*pnNumChars = (int)(szEndBracket - szTemp) + 1;
	}

	WCHAR *szTagName = szTemp;
	WCHAR *szParams = NULL;
	WCHAR *szEndTagName = PStrPbrk(szTemp, L" ]");
	if (*szEndTagName == L' ')
	{
		szParams = szEndTagName + 1;
		*szEndBracket = L'\0';
	}

	*szEndTagName = L'\0';
	int nTextTag = UIDecodeTextTag(szTagName);
	if (nTextTag == TEXT_TAG_INVALID)
	{
		return FALSE;
	}

	if (!UIGetStringTagSize(szText, pComponent, pTagSize, pnNumChars, pbForDrawOnly))
	{
		return FALSE;
	}

	return UIAddStringTagElement( pComponent, x, y, *pTagSize, nTextTag, szParams);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteElementText(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	UI_ELEMENT_TEXT *pTextElement = UIElemCastToText(element);
	UIX_TEXTURE_FONT* font = pTextElement->m_pFont;

	UI_POSITION posComponentAbsoluteScreen = UIComponentGetAbsoluteScreenPosition(element->m_pComponent);

	UI_RECT cliprect;
	UIElementGetClipRect(element, &posComponentAbsoluteScreen, &cliprect);
	UIRound(cliprect);

	UI_RECT cliprectshadow = cliprect;
	cliprectshadow.m_fX1 += 1.0f;
	cliprectshadow.m_fY1 += 1.0f;
	cliprectshadow.m_fX2 += 1.0f;
	cliprectshadow.m_fY2 += 1.0f;

	float fLogToScreenRatioX = UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScaleFont);	
	float fLogToScreenRatioY = UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScaleFont);

	UI_POSITION posScroll = UIComponentGetScrollPos(element->m_pComponent);	//now adjust the element for the component's scroll position
	posScroll.m_fX *= fLogToScreenRatioX;
	posScroll.m_fY *= fLogToScreenRatioY;
	posComponentAbsoluteScreen -= posScroll;

	// adjust for aspect ratio
	fLogToScreenRatioX /= (fLogToScreenRatioX / fLogToScreenRatioY);

	int nFontSize = (int)(((float)pTextElement->m_nFontSize * fLogToScreenRatioY * g_UI.m_fUIScaler) + scfFontSizeScaleUp);
	float fWidthRatio = pTextElement->m_fDrawScale;
	float fLineHeight = (float)nFontSize * pTextElement->m_fDrawScale;
	float fSpaceBetweenLines = 1.0f * fLogToScreenRatioY;
	//fLineHeight = FLOOR(fLineHeight + 0.5f);

	// make sure the characters exist on the local d3d texture (font cache)
	V( e_UITextureFontAddCharactersToTexture(font, pTextElement->m_pText, nFontSize) );

	float fStartX = posComponentAbsoluteScreen.m_fX + (element->m_Position.m_fX * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	float fStartY = posComponentAbsoluteScreen.m_fY + (element->m_Position.m_fY * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));
	float x = 0.0f;
	float y = 0.0f;

	BYTE bAlpha;
	INTEGER_STACK tColorStack(16, 16);		// this is probably overkill, but we might as well do it right

	DWORD dwCurrentColor = sUITextureUpdateAlpha(element, &bAlpha);
	DWORD dwLineColor = GFXCOLOR_WHITE;
	BOOL bUseLineColor = FALSE;
	float leftedge = x;
	
	const WCHAR* text = pTextElement->m_pText;

	float fAdjScreenX = 0.0f;
	float fAdjScreenY = 0.0f;
																 
	UIGetTextAlignmentScreenspaceAdj(element, text, fWidthRatio, pTextElement->m_nAlignment, &fAdjScreenX, &fAdjScreenY);

	float fTabWidth = font->m_fTabWidth * fWidthRatio;
	float fColumnX = UIElementGetTextColumnPoint(font, nFontSize, fWidthRatio, element->m_pComponent->m_bNoScaleFont, text);
	float fThisLineHeight = fLineHeight;

	int nTestVertexCount = 0;
	int nCurrentPage = 0;

	WCHAR wc;
	int nDisplayedCharCount = 0;
	pTextElement->m_nNumCharsOnPage = 0;
	while ((wc = *text++) != 0)
	{
		if (wc == CTRLCHAR_COLOR)
		{
			if (*text == 0)
			{
				break;
			}
			tColorStack.Push((int) dwCurrentColor);

			dwCurrentColor = GetFontColor(*text);
			dwCurrentColor = UI_MAKE_COLOR(bAlpha, dwCurrentColor);
			text++;
			continue;
		}
		if (wc == CTRLCHAR_COLOR_RETURN)
		{
			if (tColorStack.GetCount() > 0)
				dwCurrentColor = (DWORD)tColorStack.Pop();
			else
				dwCurrentColor = GFXCOLOR_WHITE;
			continue;
		}
		if (wc == CTRLCHAR_LINECOLOR)
		{
			if (*text == 0)
			{
				break;
			}
			dwLineColor = GetFontColor(*text);
			dwLineColor = UI_MAKE_COLOR(bAlpha, dwLineColor);
			bUseLineColor = TRUE;
			text++;
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT || wc == CTRLCHAR_HYPERTEXT_END)
		{
			text += (wc == CTRLCHAR_HYPERTEXT)? 1 : 0;
			continue;
		}

		if (wc == TEXT_TAG_BEGIN_CHAR)
		{
			int nTextTag = UIDecodeTextTag(text);
			if (nTextTag == TEXT_TAG_PGBRK ||
				nTextTag == TEXT_TAG_INTERRUPT ||
				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
			{
				nCurrentPage++;
			}
			if (nTextTag != TEXT_TAG_INVALID)
			{
				// ok, this is a tag enclosed in brackets, the element for it should already have been added, so just skip its space
				int nNumChars = 0;
				UI_SIZE sizeTag;
				float tagx = x * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScaleFont);
				float tagy = y * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScaleFont);
				
				tagx += fStartX + fAdjScreenX;
				tagy += fStartY + fAdjScreenY;

				BOOL bForDrawOnly = FALSE;
				if (UIGetStringTagSize(text, element->m_pComponent, &sizeTag, &nNumChars, &bForDrawOnly))
				{
					if (!bForDrawOnly)
					{
						x += sizeTag.m_fWidth /*/ UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScaleFont)*/;
						fThisLineHeight = MAX(fThisLineHeight, sizeTag.m_fHeight /*/ UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScaleFont)*/);
					}
					text += nNumChars;
					continue;
				}
				else
				{
					// ok, must have been some malformed tag.  Just fall through and print it
				}
			}
		}

		if (pTextElement->m_nPage != -1 && pTextElement->m_nPage != nCurrentPage)
		{
			continue;
		}

		if (wc == L'\n' || wc == L'\r')
		{
			bUseLineColor = FALSE;
			x = leftedge;
																		 
			UIGetTextAlignmentScreenspaceAdj(element, text, fWidthRatio, pTextElement->m_nAlignment, &fAdjScreenX, NULL);	
                                                                                                       // Don't do y-alignment again because it has already been done for the whole text
			y += (fThisLineHeight + fSpaceBetweenLines);
			fThisLineHeight = fLineHeight;
			continue;
		}
		if (wc == L'\t')
		{
			x = float(int( x / fTabWidth + 0.5f) + 1 ) * fTabWidth;
			continue;
		}
		if (wc == L'\f')
		{
			x = leftedge + fColumnX;
			continue;
		}

		if (wc == L']' && *text == L']')
		{
			continue;	// this is the close of a pair of double-brackets.  Just skip the first one and the second will print normally.
		}

		if (PStrIsPrintable( wc ) == FALSE)		
		{
			continue;
		}

		wc = sUIJabberChar(font, wc);
		wc = sUICheckGlyphset(font, wc);

		UI_TEXTURE_FONT_FRAME* chrframe = font->GetChar(wc, nFontSize);
		ASSERT_RETURN(chrframe);
		float fCharWidth = chrframe->m_fWidth * fWidthRatio;
		float fLeadspace = chrframe->m_fPreWidth * fWidthRatio;
		float fFollowspace = chrframe->m_fPostWidth * fWidthRatio;

		float x1 = x + fLeadspace;
		float y1 = y;
		float x2 = x1 + fCharWidth;
		float y2 = y1 + fLineHeight;

		//x1 *= fLogToScreenRatioX;
		//y1 *= fLogToScreenRatioY;
		//x2 *= fLogToScreenRatioX;
		//y2 *= fLogToScreenRatioY;

		x1 += fStartX + fAdjScreenX;
		x2 += fStartX + fAdjScreenX;
		y1 += fStartY + fAdjScreenY;
		y2 += fStartY + fAdjScreenY;

		x1 = UIRound(x1);
		x2 = UIRound(x2);
		y1 = UIRound(y1);
		y2 = UIRound(y2);

		// Big 'ol special case
		BOOL bSkipChar = FALSE;
		// we now have another character on this page of text
		pTextElement->m_nNumCharsOnPage ++;
		
		// skip characters after early terminate index
		if (pTextElement->m_nMaxCharactersToDisplay != 0 &&
			nDisplayedCharCount >= pTextElement->m_nMaxCharactersToDisplay)
		{
			bSkipChar = TRUE;
		}
			
		if (!bSkipChar)
		{
			nDisplayedCharCount++;
			DWORD dwColor = (bUseLineColor ? dwLineColor : dwCurrentColor);

			if (TESTBIT( pTextElement->m_dwFlags, UETF_DROP_SHADOW_BIT ) && bDropShadowsEnabled)
			{
				float sx1 = x1 + 1.0f;
				float sx2 = x2 + 1.0f;
				float sy1 = y1 + 1.0f;
				float sy2 = y2 + 1.0f;

				V( e_UIWriteElementText(
					texture->m_nLocalBuffer, 
					wc, 
					nFontSize,
					sx1, 
					sy1, 
					sx2,
					sy2, 
					element->m_fZDelta, 
					UI_MAKE_COLOR(bAlpha, pTextElement->m_dwDropshadowColor),
					//UI_MAKE_COLOR( UI_GET_ALPHA( dwColor ), GFXCOLOR_BLACK ),
					cliprectshadow, 
					font) );

				nTestVertexCount += 4;
			}

			V( e_UIWriteElementText(
				texture->m_nLocalBuffer, 
				wc, 
				nFontSize,
				x1, 
				y1, 
				x2,
				y2, 
				element->m_fZDelta, 
				dwColor, 
				cliprect, 
				font) );

			nTestVertexCount += 4;
		}

		x += (fLeadspace + fCharWidth + fFollowspace + (font->m_nExtraSpacing * fWidthRatio));
	}

	ASSERT(nTestVertexCount <= element->m_nVertexCount);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteElementTri(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	UI_ELEMENT_TRI *pTriElement = UIElemCastToTri(element);

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(element->m_pComponent);

	UI_TEXTURETRI tTri = pTriElement->m_Triangle;
	float x1 = (tTri.x1 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	float y1 = (tTri.y1 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;
	float x2 = (tTri.x2 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	float y2 = (tTri.y2 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;
	float x3 = (tTri.x3 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	float y3 = (tTri.y3 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;

	DWORD dwColor = sUITextureUpdateAlpha(element);

	tTri.x1 = x1;
	tTri.x2 = x2;
	tTri.x3 = x3;
	tTri.y1 = y1;
	tTri.y2 = y2;
	tTri.y3 = y3;
	V( e_UIWriteElementTri( texture->m_nLocalBuffer, tTri, element->m_fZDelta, dwColor ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteElementRect(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	UI_ELEMENT_RECT *pRectElement = UIElemCastToRect(element);

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(element->m_pComponent);

	UI_TEXTURERECT tRect = pRectElement->m_Rect;
	tRect.x1 = (tRect.x1 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	tRect.y1 = (tRect.y1 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;
	tRect.x2 = (tRect.x2 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	tRect.y2 = (tRect.y2 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;
	tRect.x3 = (tRect.x3 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	tRect.y3 = (tRect.y3 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;
	tRect.x4 = (tRect.x4 + pos.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
	tRect.y4 = (tRect.y4 + pos.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;

	DWORD dwColor = sUITextureUpdateAlpha(element);

	V( e_UIWriteElementRect( texture->m_nLocalBuffer, tRect, element->m_fZDelta, dwColor ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIElementChunkGetCoords(
	UI_GFXELEMENT* element,
	UI_TEXTURE_FRAME* frame,
	UI_POSITION* posComponentAbsoluteScreen,
	UI_RECT* drawrect,
	UI_RECT* screencliprect,
	UI_TEXRECT* texrect,
	int nChunkNum,
	float fHorizStretchPct,
	float fVertStretchPct)
{
	if (!frame || !texrect || nChunkNum < 0 || nChunkNum >= frame->m_nNumChunks)
	{
		return FALSE;
	}

	ASSERT_RETFALSE(drawrect);
	ASSERT_RETFALSE(element);
	ASSERT_RETFALSE(element->m_pComponent);

	drawrect->m_fX1 = posComponentAbsoluteScreen->m_fX + ((element->m_Position.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	drawrect->m_fY1 = posComponentAbsoluteScreen->m_fY + ((element->m_Position.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));
	drawrect->m_fX1 -= frame->m_pChunks[nChunkNum].m_fXOffset * fHorizStretchPct * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale);
	drawrect->m_fY1 -= frame->m_pChunks[nChunkNum].m_fYOffset * fVertStretchPct * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale);
	
	drawrect->m_fX2 = drawrect->m_fX1 + (frame->m_pChunks[nChunkNum].m_fWidth * fHorizStretchPct * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	drawrect->m_fY2 = drawrect->m_fY1 + (frame->m_pChunks[nChunkNum].m_fHeight * fVertStretchPct * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));

	texrect->m_fU1 = frame->m_pChunks[nChunkNum].m_fU1;
	texrect->m_fV1 = frame->m_pChunks[nChunkNum].m_fV1;
	texrect->m_fU2 = frame->m_pChunks[nChunkNum].m_fU2;
	texrect->m_fV2 = frame->m_pChunks[nChunkNum].m_fV2;

	// This helps with stitching stretched chunks back together
	{
		UIRound(*drawrect);
	}

	if (!e_UIElementDoClip(drawrect, texrect, screencliprect))
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteElementFrame(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	UI_ELEMENT_FRAME *pFrameElement = UIElemCastToFrame(element);
	UI_TEXTURE_FRAME* frame = pFrameElement->m_pFrame;
	ASSERT_RETURN(frame);

	UI_POSITION posComponentAbsoluteScreen = UIComponentGetAbsoluteScreenPosition(element->m_pComponent);

	DWORD dwColor = sUITextureUpdateAlpha(element);


	// CML 2007.1.23: attempt to get async texture loading working
	ASSERT_RETURN( texture->m_pTexture );
	if ( frame->m_fWidth <= 0.0001f || frame->m_fHeight <= 0.0001f )
	{
		if ( e_TextureIsLoaded( texture->m_pTexture->m_nTextureId, TRUE ) )
		{
			int nWidth = 0, nHeight = 0;
			V( e_TextureGetOriginalResolution( texture->m_pTexture->m_nTextureId, nWidth, nHeight ) );
			frame->m_fWidth  = (float)nWidth;
			frame->m_fHeight = (float)nHeight;
		}
	}
	if ( ! e_TextureIsLoaded( texture->m_pTexture->m_nTextureId ) )
		return;


	float fHorizStretchPct = (element->m_fWidth / frame->m_fWidth);
	float fVertStretchPct  = (element->m_fHeight / frame->m_fHeight);

	UI_RECT cliprect;
	UIElementGetClipRect(element, &posComponentAbsoluteScreen, &cliprect);
	UIRound(cliprect);

	UI_POSITION posScroll = UIComponentGetScrollPos(element->m_pComponent);	//now adjust the element for the component's scroll position
	posScroll.m_fX *= UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale);	
	posScroll.m_fY *= UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale);
	posComponentAbsoluteScreen -= posScroll;

	UI_RECT drawrect;
	UI_TEXRECT texrect;
	for (int i=0; i < frame->m_nNumChunks; i++ )
	{
		if (UIElementChunkGetCoords(element, frame, &posComponentAbsoluteScreen, &drawrect, &cliprect, &texrect, i, fHorizStretchPct, fVertStretchPct))
		{
			V( e_UIWriteElementFrame( texture->m_nLocalBuffer, drawrect, texrect, element->m_fZDelta, dwColor ) );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteRotatedFrame(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	UI_ELEMENT_ROTFRAME *pFrameElement = UIElemCastToRotFrame(element);
	UI_TEXTURE_FRAME* frame = pFrameElement->m_pFrame;
	ASSERT_RETURN(frame);

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(element->m_pComponent);

	DWORD dwColor = sUITextureUpdateAlpha(element);

	float fHorizStretchPct = element->m_fWidth / (frame->m_fWidth * UIGetScreenToLogRatioX(element->m_pComponent->m_bNoScale));
	float fVertStretchPct  = element->m_fHeight / (frame->m_fHeight * UIGetScreenToLogRatioY(element->m_pComponent->m_bNoScale));

	float fRotateCenterX;
	float fRotateCenterY;
	if (TESTBIT( pFrameElement->m_dwElementFrameFlags, EFF_ROTATE_ABOUT_SELF_BIT ) == TRUE)
	{
		fRotateCenterX = (element->m_Position.m_fX + pos.m_fX) * UIGetLogToScreenRatioX();
		fRotateCenterY = (element->m_Position.m_fY + pos.m_fY) * UIGetLogToScreenRatioY();
	}
	else
	{
		fRotateCenterX = pFrameElement->m_RotationCenter.m_fX * UIGetLogToScreenRatioX();
		fRotateCenterY = pFrameElement->m_RotationCenter.m_fY * UIGetLogToScreenRatioY();
	}
	
	for (int nChunkNum = 0; nChunkNum < frame->m_nNumChunks; nChunkNum++)
	{
		UI_RECT drawrect;
		UI_TEXRECT texrect;

		drawrect.m_fX1 = element->m_Position.m_fX + pos.m_fX - element->m_fXOffset;
		drawrect.m_fY1 = element->m_Position.m_fY + pos.m_fY - element->m_fYOffset;
		drawrect.m_fX1 -= frame->m_pChunks[nChunkNum].m_fXOffset * UIGetScreenToLogRatioX(element->m_pComponent->m_bNoScale) * fHorizStretchPct;
		drawrect.m_fY1 -= frame->m_pChunks[nChunkNum].m_fYOffset * UIGetScreenToLogRatioY(element->m_pComponent->m_bNoScale) * fVertStretchPct;
		drawrect.m_fX2 = drawrect.m_fX1 + (frame->m_pChunks[nChunkNum].m_fWidth * UIGetScreenToLogRatioX(element->m_pComponent->m_bNoScale) * fHorizStretchPct);
		drawrect.m_fY2 = drawrect.m_fY1 + (frame->m_pChunks[nChunkNum].m_fHeight * UIGetScreenToLogRatioY(element->m_pComponent->m_bNoScale) * fVertStretchPct);

		texrect.m_fU1 = frame->m_pChunks[nChunkNum].m_fU1;
		texrect.m_fV1 = frame->m_pChunks[nChunkNum].m_fV1;
		texrect.m_fU2 = frame->m_pChunks[nChunkNum].m_fU2;
		texrect.m_fV2 = frame->m_pChunks[nChunkNum].m_fV2;

		drawrect.m_fX1 = drawrect.m_fX1 * UIGetLogToScreenRatioX() - 0.5f;
		drawrect.m_fY1 = drawrect.m_fY1 * UIGetLogToScreenRatioY() - 0.5f;
		drawrect.m_fX2 = drawrect.m_fX2 * UIGetLogToScreenRatioX() - 0.5f;
		drawrect.m_fY2 = drawrect.m_fY2 * UIGetLogToScreenRatioY() - 0.5f;

		V( e_UIWriteRotatedFrame(
			texture->m_nLocalBuffer,
			drawrect,
			frame->m_fWidth / texture->m_pTexture->m_XMLWidthRatio,
			frame->m_fHeight / texture->m_pTexture->m_XMLHeightRatio,
			texrect,
			element->m_fZDelta,
			dwColor,
			fRotateCenterX,
			fRotateCenterY,
			pFrameElement->m_fAngle ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteElementFullTexture(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	UI_ELEMENT_FULLTEXTURE *pFullElement = UIElemCastToFullTexture(element);

	UI_POSITION posComponentAbsoluteScreen = UIComponentGetAbsoluteScreenPosition(element->m_pComponent);

	DWORD dwColor = sUITextureUpdateAlpha(element);

	if ( ! e_TextureIsLoaded( pFullElement->m_nEngineTextureID ) )
		return;

	UI_RECT cliprect;
	UIElementGetClipRect(element, &posComponentAbsoluteScreen, &cliprect);
	UIRound(cliprect);

	UI_POSITION posScroll = UIComponentGetScrollPos(element->m_pComponent);	//now adjust the element for the component's scroll position
	posScroll.m_fX *= UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale);	
	posScroll.m_fY *= UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale);
	posComponentAbsoluteScreen -= posScroll;

	UI_RECT drawrect;
	UI_TEXRECT texrect;

	drawrect.m_fX1 = posComponentAbsoluteScreen.m_fX + ((element->m_Position.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	drawrect.m_fY1 = posComponentAbsoluteScreen.m_fY + ((element->m_Position.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));

	float fElementScreenWidth  = element->m_fWidth  * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale);
	float fElementScreenHeight = element->m_fHeight * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale);

	drawrect.m_fX2 = drawrect.m_fX1 + fElementScreenWidth;
	drawrect.m_fY2 = drawrect.m_fY1 + fElementScreenHeight;

	// If the aspect ratio isn't square, we need to tweak the UVs.
	// Policy: crop UVs so that the longest dimension fits fully
	if ( fElementScreenWidth != fElementScreenHeight )
	{
		float fXCrop, fYCrop;
		if ( fElementScreenWidth > fElementScreenHeight )
		{
			// crop the height
			fXCrop = 1.f;
			fYCrop = 0.5f * ( fElementScreenHeight / fElementScreenWidth + 1.f );
		}
		else
		{
			// crop the width
			fXCrop = 0.5f * ( fElementScreenWidth / fElementScreenHeight + 1.f );
			fYCrop = 1.f;
		}

		texrect.m_fU1 = 1.f - fXCrop;
		texrect.m_fV1 = 1.f - fYCrop;
		texrect.m_fU2 = fXCrop;
		texrect.m_fV2 = fYCrop;
	}
	else
	{
		// square
		texrect.m_fU1 = 0.0f;
		texrect.m_fV1 = 0.0f;
		texrect.m_fU2 = 1.0f;
		texrect.m_fV2 = 1.0f;
	}

	UIRound(drawrect);

	if (!e_UIElementDoClip(&drawrect, &texrect, &cliprect))
	{
		return;
	}

	V( e_UIWriteElementFrame( texture->m_nLocalBuffer, drawrect, texrect, element->m_fZDelta, dwColor ) );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextureWriteElement(
	UI_TEXTUREDRAW* texture,
	UI_GFXELEMENT* element)
{
	switch (element->m_eGfxElement)
	{
	case GFXELEMENT_FRAME:
		UITextureWriteElementFrame(texture, element);
		break;
	case GFXELEMENT_TEXT:
		UITextureWriteElementText(texture, element);
		break;
	case GFXELEMENT_TRI:
		UITextureWriteElementTri(texture, element);
		break;
	case GFXELEMENT_ROTFRAME:
		UITextureWriteRotatedFrame(texture, element);
		break;
	case GFXELEMENT_RECT:
		UITextureWriteElementRect(texture, element);
		break;
	case GFXELEMENT_FULLTEXTURE:
		UITextureWriteElementFullTexture(texture, element);
		break;
	}
}


//----------------------------------------------------------------------------
// UI_TEXTUREDRAW: write vertex buffer & index buffers from list of
// UI_GFXELEMENTS given in UI_DRAW
//----------------------------------------------------------------------------
void UITextureWriteLocalBuffers(
	UI_TEXTUREDRAW * texture,
	UI_DRAW * draw)
{
	UI_GFXELEMENT * element = draw->m_pElementsToDrawFirst;
	while (element)
	{
		UI_GFXELEMENT * next_element = element->m_pNextElementToDraw;

		UITextureWriteElement(texture, element);

		element->m_pNextElementToDraw = NULL;
		element = next_element;
	}
}


//----------------------------------------------------------------------------
// called every time we call RenderToLocal to "delete" all UI_TEXTUREDRAWs
//----------------------------------------------------------------------------
void UITextureToDrawClear(
	void)
{
	UI_TEXTUREDRAW * texture = g_GraphicGlobals.m_pTextureToDrawFirst;
	//g_GraphicGlobals.m_pTextureToDrawFirst = NULL;
	UI_TEXTUREDRAW * prev = NULL;

	while (texture)
	{
		//trace( "Clearing tex2draw: texture 0x%p, texture->next 0x%p\n", texture, texture->m_pNext );

		UI_TEXTUREDRAW * next = texture->m_pNext;

		BOOL bNeedUpdate = 0 != ( g_dwRenderState[texture->m_nRenderSection] & UIRS_NEED_UPDATE );
		BOOL bNeedRender = 0 != ( g_dwRenderState[texture->m_nRenderSection] & UIRS_NEED_RENDER );
		REF(bNeedRender);

		// if we need update (any render), erase
		// if we need render and don't need update, don't erase
		// if we don't need render and don't need update, don't erase

		// if we're between update and render, don't clear the engine buffers
		if ( ! bNeedUpdate )
		{
			// don't erase this texturedraw
			prev = texture;
		}
		else
		{
			// need update!

			// erase this texturedraw and put it on the garbage list for later use
			if (prev)
			{
				prev->m_pNext = next;
			}
			if (texture == g_GraphicGlobals.m_pTextureToDrawFirst)
			{
				g_GraphicGlobals.m_pTextureToDrawFirst = next;
			}

			if ( texture->m_nEngineBuffer != INVALID_ID )
			{
				V( e_UITrashEngineBuffer(texture->m_nEngineBuffer) );
			}
			V( e_UITextureDrawClearLocalBufferCounts(texture->m_nLocalBuffer) );
			texture->m_pTexture = NULL;
			texture->m_nRenderSection = -1;
			texture->m_bGrayout = FALSE;
			texture->m_pNext = g_GraphicGlobals.m_pTextureToDrawGarbage;
			g_GraphicGlobals.m_pTextureToDrawGarbage = texture;
		}

		texture = next;
	}

	// Set the pointer to the last texture
	g_GraphicGlobals.m_pTextureToDrawLast = NULL;
	texture = g_GraphicGlobals.m_pTextureToDrawFirst;
	while (texture)
	{
		if (!texture->m_pNext)
		{
			g_GraphicGlobals.m_pTextureToDrawLast = texture;
		}

		texture = texture->m_pNext;
	}
}


//----------------------------------------------------------------------------
// get a clean UI_TEXTUREDRAW, either get from garbage list or allocate one
//----------------------------------------------------------------------------
UI_TEXTUREDRAW * UITextureToDrawGet(
	int nDesiredVertices )
{
	UI_TEXTUREDRAW* texture = NULL;

	if (g_GraphicGlobals.m_pTextureToDrawGarbage)
	{

		// TRAVIS: OK, Experimenting with this concept.
		// we look for the closest vert match to what we're requesting -
		// this should mean a lot less reallocs, and a lot less likelihood
		// of reallocing a 4-vert buffer to 29000 when there's a perfectly viable
		// 29000 buffer out there.
		// In talking to Didier, we should be able to use his cool new mempool stuff
		// to do this, which will automatically sort, among other things.
		// So, this is temporary, but functional code that prevents memory
		// from skyrocketing in Mythos. In Hellgate, the very small number of actual UI textures
		// should mean this is pretty darn cheap, and it doesn't seem to make a dent in Mythos either,
		// even in the current incarnation. This seems to work so far for Mythos, and our buffer allocations
		// reach a level of relative stability around the 7-10 MB range without any reallocs to smaller sizes.
		UI_TEXTUREDRAW* pClosest = g_GraphicGlobals.m_pTextureToDrawGarbage;
		UI_TEXTUREDRAW* pPrev = NULL;
		UI_TEXTUREDRAW* pStoredPrev = NULL;
		// we'll take the first one if nothing else better comes along
		texture = pClosest;
		int nClosest = pClosest ? ( e_UIGetLocalBufferVertexMaxCount( pClosest->m_nLocalBuffer ) - nDesiredVertices ) : INVALID_ID;
		if( nClosest < 0 )
		{
			nClosest = INVALID_ID;
		}
		while( pClosest )
		{
			int nCurCount = e_UIGetLocalBufferVertexCurCount( pClosest->m_nLocalBuffer );
			int nMaxCount = e_UIGetLocalBufferVertexMaxCount( pClosest->m_nLocalBuffer );
			// hey, this one matches perfectly! Let's use it right away.
			if( nCurCount == nDesiredVertices ||
				nMaxCount == nDesiredVertices )
			{
				pStoredPrev = pPrev;
				texture = pClosest;
				break;
			}
			int nDelta = nMaxCount - nDesiredVertices;
			// if the buffer has more vertices than requested, but less than
			// our previous best selection, choose it, and keep on rolling.
			if( nDelta > 0 && ( nClosest == INVALID_ID || nDelta < nClosest ) )
			{
				pStoredPrev = pPrev;
				texture = pClosest;
				nClosest = nDelta;
			}
			pPrev = pClosest;
			pClosest = pClosest->m_pNext;
		}
		// we must have taken the first one in the list.
		if( pStoredPrev == NULL )
		{
			g_GraphicGlobals.m_pTextureToDrawGarbage = texture->m_pNext;
		}
		else // pull it out of the garbage list
		{
			pStoredPrev->m_pNext = texture->m_pNext;
		}
		ASSERT( texture->m_nEngineBuffer == INVALID_ID );
		//V( e_UITextureDrawTrashEngineBuffer( texture->m_nEngineBuffer ) );
		V( e_UITextureDrawClearLocalBufferCounts( texture->m_nLocalBuffer ) );
		texture->m_pTexture = NULL;
		texture->m_nRenderSection = -1;
		texture->m_pNext = NULL;
	}
	else
	{
		texture = (UI_TEXTUREDRAW*)MALLOCZ(NULL, sizeof(UI_TEXTUREDRAW));
		V( e_UIGetNewLocalBuffer( texture->m_nLocalBuffer ) );
		texture->m_nEngineBuffer = INVALID_ID;

		//trace( "Malloc new T2D: ptr 0x%p\n", texture );
	}
	return texture;
}


//----------------------------------------------------------------------------
// called every time we call RenderToLocal to "delete" all UI_MODELs
//----------------------------------------------------------------------------
void UIModelToDrawClear(
	void)
{
	UI_MODEL * pModel = g_GraphicGlobals.m_pModelToDrawFirst;
	UI_MODEL * prev = NULL;

	while (pModel)
	{
		UI_MODEL * next = pModel->m_pNext;
		if (g_dwRenderState[pModel->m_nRenderSection] & UIRS_NEED_RENDER ||
			!(g_dwRenderState[pModel->m_nRenderSection] & UIRS_NEED_UPDATE))
		{
			prev = pModel;
		}
		else
		{
			if (prev)
			{
				prev->m_pNext = next;
			}
			if (pModel == g_GraphicGlobals.m_pModelToDrawFirst)
			{
				g_GraphicGlobals.m_pModelToDrawFirst = next;
			}

			pModel->m_nRenderSection = -1;
			pModel->m_nAppearanceDefIDForCamera = INVALID_ID;
			pModel->m_nModelID = INVALID_ID;
			pModel->m_fRotation = 0.0f;
			memclear(&pModel->m_tScreenRect, sizeof(UI_RECT));
			memclear(&pModel->m_tOrigRect, sizeof(UI_RECT));
			pModel->m_pNext = NULL;

			pModel->m_pNext = g_GraphicGlobals.m_pModelToDrawGarbage;
			g_GraphicGlobals.m_pModelToDrawGarbage = pModel;
		}

		pModel = next;
	}

	pModel = g_GraphicGlobals.m_pModelToDrawFirst;
	g_GraphicGlobals.m_pModelToDrawLast = NULL;
	while (pModel)
	{
		if (!pModel->m_pNext)
		{
			g_GraphicGlobals.m_pModelToDrawLast = pModel;
		}

		pModel = pModel->m_pNext;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MODEL* UIModelToDrawGet(
	void)
{
	UI_MODEL* pModel = NULL;

	if (g_GraphicGlobals.m_pModelToDrawGarbage)
	{
		pModel = g_GraphicGlobals.m_pModelToDrawGarbage;
		g_GraphicGlobals.m_pModelToDrawGarbage = pModel->m_pNext;

		pModel->m_nRenderSection = -1;
		pModel->m_nAppearanceDefIDForCamera = INVALID_ID;
		pModel->m_nModelID = INVALID_ID;
		pModel->m_fRotation = 0.0f;
		pModel->m_nModelCam = 0;

		memclear(&pModel->m_tScreenRect, sizeof(UI_RECT));
		memclear(&pModel->m_tOrigRect, sizeof(UI_RECT));
		pModel->m_pNext = NULL;
	}
	else
	{
		pModel = (UI_MODEL*)MALLOCZ(NULL, sizeof(UI_MODEL));
	}
	return pModel;
}

//----------------------------------------------------------------------------
// called every time we call RenderToLocal to "delete" all UI_CALLBACK_RENDERs
//----------------------------------------------------------------------------
void UICallbackToDrawClear(
	void)
{
	UI_CALLBACK_RENDER * pCallback = g_GraphicGlobals.m_pCallbackToDrawFirst;
	UI_CALLBACK_RENDER * prev = NULL;

	while (pCallback)
	{
		UI_CALLBACK_RENDER * next = pCallback->m_pNext;
		if (g_dwRenderState[pCallback->m_nRenderSection] & UIRS_NEED_RENDER ||
			!(g_dwRenderState[pCallback->m_nRenderSection] & UIRS_NEED_UPDATE))
		{
			prev = pCallback;
		}
		else
		{
			if (prev)
			{
				prev->m_pNext = next;
			}
			if (pCallback == g_GraphicGlobals.m_pCallbackToDrawFirst)
			{
				g_GraphicGlobals.m_pCallbackToDrawFirst = next;
			}

			pCallback->m_nRenderSection = -1;
			memclear(&pCallback->m_tScreenRect, sizeof(UI_RECT));
			memclear(&pCallback->m_tOrigRect, sizeof(UI_RECT));
			pCallback->m_pfnCallback = NULL;
			pCallback->m_pNext = NULL;

			pCallback->m_pNext = g_GraphicGlobals.m_pCallbackToDrawGarbage;
			g_GraphicGlobals.m_pCallbackToDrawGarbage = pCallback;
		}

		pCallback = next;
	}

	pCallback = g_GraphicGlobals.m_pCallbackToDrawFirst;
	g_GraphicGlobals.m_pCallbackToDrawLast = NULL;
	while (pCallback)
	{
		if (!pCallback->m_pNext)
		{
			g_GraphicGlobals.m_pCallbackToDrawLast = pCallback;
		}

		pCallback = pCallback->m_pNext;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_CALLBACK_RENDER* UICallbackToDrawGet(
	void)
{
	UI_CALLBACK_RENDER* pCallback = NULL;

	if (g_GraphicGlobals.m_pCallbackToDrawGarbage)
	{
		pCallback = g_GraphicGlobals.m_pCallbackToDrawGarbage;
		g_GraphicGlobals.m_pCallbackToDrawGarbage = pCallback->m_pNext;

		pCallback->m_nRenderSection = -1;

		memclear(&pCallback->m_tScreenRect, sizeof(UI_RECT));
		memclear(&pCallback->m_tOrigRect, sizeof(UI_RECT));
		pCallback->m_pNext = NULL;
	}
	else
	{
		pCallback = (UI_CALLBACK_RENDER*)MALLOCZ(NULL, sizeof(UI_CALLBACK_RENDER));
	}
	return pCallback;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPrimitiveWriteElementBox(
	UI_PRIMITIVEDRAW* prim,
	UI_ELEMENT_PRIMITIVE* element)
{
	UI_POSITION posComponentAbsoluteScreen = UIComponentGetAbsoluteScreenPosition(element->m_pComponent);

	UI_RECT cliprect;
	UIElementGetClipRect(element, &posComponentAbsoluteScreen, &cliprect);
	UIRound(cliprect);

	UI_POSITION posScroll = UIComponentGetScrollPos(element->m_pComponent);	//now adjust the element for the component's scroll position
	posScroll.m_fX *= UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale);	
	posScroll.m_fY *= UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale);
	posComponentAbsoluteScreen -= posScroll;

	UI_RECT drawrect;
	drawrect.m_fX1 = posComponentAbsoluteScreen.m_fX + ((element->m_tRect.m_fX1 - element->m_fXOffset) * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	drawrect.m_fY1 = posComponentAbsoluteScreen.m_fY + ((element->m_tRect.m_fY1 - element->m_fYOffset) * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));
	drawrect.m_fX2 = posComponentAbsoluteScreen.m_fX + ((element->m_tRect.m_fX2 - element->m_fXOffset) * UIGetLogToScreenRatioX(element->m_pComponent->m_bNoScale));
	drawrect.m_fY2 = posComponentAbsoluteScreen.m_fY + ((element->m_tRect.m_fY2 - element->m_fYOffset) * UIGetLogToScreenRatioY(element->m_pComponent->m_bNoScale));

	{
		UIRound(drawrect);
	}

	if ( ! drawrect.DoClip( cliprect ) )
		return;

	UI_QUAD tRect;
	tRect.x1 = drawrect.m_fX1;
	tRect.y1 = drawrect.m_fY1;
	tRect.x2 = drawrect.m_fX2;
	tRect.y2 = drawrect.m_fY1;
	tRect.x3 = drawrect.m_fX1;
	tRect.y3 = drawrect.m_fY2;
	tRect.x4 = drawrect.m_fX2;
	tRect.y4 = drawrect.m_fY2;

	DWORD dwColor = sUITextureUpdateAlpha(element);

	V( e_UIWriteElementQuad( prim->m_nLocalBuffer, tRect, element->m_fZDelta, dwColor ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPrimitiveWriteElementArcSector(
	UI_PRIMITIVEDRAW* prim,
	UI_ELEMENT_PRIMITIVE* element)
{
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(element->m_pComponent);

	UI_QUADSTRIP tPoints;
	tPoints.nPointCount = 0;

	float fSegments = element->m_tArcData.fAngleWidthDeg / element->m_tArcData.fSegmentWidthDeg + 1.f;
	int nSegments = (int) CEIL( fSegments );

	float fWidth  = element->m_tRect.m_fX2 - element->m_tRect.m_fX1;
	float fHeight = element->m_tRect.m_fY2 - element->m_tRect.m_fY1;
	float fMaxDim = max( fWidth, fHeight ); // * 2.0f; // double because it's going to be used for the radius
	//float fMinDim = min( fWidth, fHeight );
	float fInner = fMaxDim - ( fMaxDim * element->m_tArcData.fRingRadiusPct );
	float fOuter = fMaxDim;

	UI_POSITION vCenter;
	vCenter.m_fX = ( element->m_tRect.m_fX1 + element->m_tRect.m_fX2 ) * 0.5f + pos.m_fX;
	vCenter.m_fY = ( element->m_tRect.m_fY1 + element->m_tRect.m_fY2 ) * 0.5f + pos.m_fY;

	float fRatioX = fWidth  / fMaxDim * 0.5f;
	float fRatioY = fHeight / fMaxDim * 0.5f;

	float fAngleDeg = element->m_tArcData.fAngleStartDeg;
	for ( int i = 0; i < nSegments; i++ )
	{
		int nAngle = (int)fAngleDeg % 360;
		if ( nAngle < 0 )
			nAngle += 360;
		float x =  gfSin360Tbl[ nAngle ] * fOuter * fRatioX;
		float y = -gfCos360Tbl[ nAngle ] * fOuter * fRatioY;
		tPoints.vPoints[ tPoints.nPointCount ].x = (x + vCenter.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
		tPoints.vPoints[ tPoints.nPointCount ].y = (y + vCenter.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;

		x =	 gfSin360Tbl[ nAngle ] * fInner * fRatioX;
		y = -gfCos360Tbl[ nAngle ] * fInner * fRatioY;
		tPoints.vPoints[ tPoints.nPointCount + 1 ].x = (x + vCenter.m_fX - element->m_fXOffset) * UIGetLogToScreenRatioX() - 0.5f;
		tPoints.vPoints[ tPoints.nPointCount + 1 ].y = (y + vCenter.m_fY - element->m_fYOffset) * UIGetLogToScreenRatioY() - 0.5f;

		tPoints.nPointCount += 2;
		ASSERT_BREAK( tPoints.nPointCount < UI_MAX_QUADSTRIP_POINTS );
		fAngleDeg += element->m_tArcData.fSegmentWidthDeg;
	}

	DWORD dwColor = sUITextureUpdateAlpha(element);

	V( e_UIWriteElementQuadStrip( prim->m_nLocalBuffer, tPoints, element->m_fZDelta, dwColor ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIPrimitiveWriteElement(
	UI_PRIMITIVEDRAW* prim,
	UI_ELEMENT_PRIMITIVE* element)
{
	switch (element->m_eType)
	{
	case UIPRIM_BOX:
		UIPrimitiveWriteElementBox(prim, element);
		break;
	case UIPRIM_ARCSECTOR:
		UIPrimitiveWriteElementArcSector(prim, element);
		break;
	}
}

//----------------------------------------------------------------------------
// UI_PRIMITIVEDRAW: write vertex buffer & index buffers from list of
// UI_ELEMENT_PRIMITIVEs given in UI_PRIMDRAW
//----------------------------------------------------------------------------
void UIPrimitiveWriteLocalBuffers(
	UI_PRIMITIVEDRAW * prim,
	UI_PRIMDRAW * draw)
{
	UI_ELEMENT_PRIMITIVE * element = draw->m_pElementsToDrawFirst;
	UI_ELEMENT_PRIMITIVE * next_element;
	while (element)
	{
		if ( element->m_pNextElementToDraw )
			next_element = UIElemCastToPrim( element->m_pNextElementToDraw );
		else
			next_element = NULL;

		UIPrimitiveWriteElement(prim, element);

		element->m_pNextElementToDraw = NULL;
		element = next_element;
	}
}

//----------------------------------------------------------------------------
// called every time we call RenderToLocal to "delete" all UI_PRIMITIVEDRAWs
//----------------------------------------------------------------------------
void UIPrimitiveToDrawClear(
	void)
{
	UI_PRIMITIVEDRAW * prim = g_GraphicGlobals.m_pPrimToDrawFirst;
	//g_GraphicGlobals.m_pPrimToDrawFirst = NULL;
	UI_PRIMITIVEDRAW * prev = NULL;

	while (prim)
	{
		UI_PRIMITIVEDRAW * next = prim->m_pNext;

		// if we're between update and render, don't clear the engine buffers
		if (g_dwRenderState[prim->m_nRenderSection] & UIRS_NEED_RENDER ||
			!(g_dwRenderState[prim->m_nRenderSection] & UIRS_NEED_UPDATE))
		{
			prev = prim;
		}
		else
		{
			if (prev)
			{
				prev->m_pNext = next;
			}
			if (prim == g_GraphicGlobals.m_pPrimToDrawFirst)
			{
				g_GraphicGlobals.m_pPrimToDrawFirst = next;
			}

			if ( prim->m_nEngineBuffer != INVALID_ID )
			{
				V( e_UITrashEngineBuffer(prim->m_nEngineBuffer) );
				prim->m_nEngineBuffer = INVALID_ID;
			}
			V( e_UIPrimitiveDrawClearLocalBufferCounts(prim->m_nLocalBuffer) );
			prim->m_dwFlags = (DWORD)-1;
			prim->m_nRenderSection = -1;
			prim->m_pNext = g_GraphicGlobals.m_pPrimToDrawGarbage;
			g_GraphicGlobals.m_pPrimToDrawGarbage = prim;
		}

		prim = next;
	}

	// Set the pointer to the last prim
	g_GraphicGlobals.m_pPrimToDrawLast = NULL;
	prim = g_GraphicGlobals.m_pPrimToDrawFirst;
	while (prim)
	{
		if (!prim->m_pNext)
		{
			g_GraphicGlobals.m_pPrimToDrawLast = prim;
		}

		prim = prim->m_pNext;
	}
}


//----------------------------------------------------------------------------
// get a clean UI_PRIMITIVEDRAW, either get from garbage list or allocate one
//----------------------------------------------------------------------------
UI_PRIMITIVEDRAW * UIPrimitiveToDrawGet(
	void)
{
	UI_PRIMITIVEDRAW* prim = NULL;

	if (g_GraphicGlobals.m_pPrimToDrawGarbage)
	{
		prim = g_GraphicGlobals.m_pPrimToDrawGarbage;
		g_GraphicGlobals.m_pPrimToDrawGarbage = prim->m_pNext;
		ASSERT( prim->m_nEngineBuffer == INVALID_ID );
		//V( e_UIPrimitiveDrawTrashEngineBuffer( prim->m_nEngineBuffer ) );
		V( e_UIPrimitiveDrawClearLocalBufferCounts( prim->m_nLocalBuffer ) );
		prim->m_dwFlags = (DWORD)-1;
		prim->m_nRenderSection = -1;
		prim->m_pNext = NULL;
	}
	else
	{
		prim = (UI_PRIMITIVEDRAW*)MALLOCZ(NULL, sizeof(UI_PRIMITIVEDRAW));
		V( e_UIGetNewLocalBuffer( prim->m_nLocalBuffer ) );
		prim->m_nEngineBuffer = INVALID_ID;
	}
	return prim;
}


//----------------------------------------------------------------------------
// UI_TEXTUREDRAW: free the local & d3d buffers
//----------------------------------------------------------------------------
void UITextureDrawFree(
	UI_TEXTUREDRAW* texture)
{
	ASSERT_RETURN(texture);

	V( e_UITextureDrawBuffersFree( texture->m_nEngineBuffer, texture->m_nLocalBuffer ) );
	FREE(NULL, texture);
}

void UIModelDrawFree(
	UI_MODEL *pModel)
{
	ASSERT_RETURN(pModel);

	FREE(NULL, pModel);
}

void UIPrimitiveDrawFree(
	UI_PRIMITIVEDRAW* prim)
{
	ASSERT_RETURN(prim);

	V( e_UIPrimitiveDrawBuffersFree( prim->m_nEngineBuffer, prim->m_nLocalBuffer ) );
	FREE(NULL, prim);
}

void UICallbackDrawFree(
	UI_CALLBACK_RENDER *pCallback)
{
	ASSERT_RETURN(pCallback);

	FREE(NULL, pCallback);
}

//----------------------------------------------------------------------------
// write all UI_GFXELEMENTS from draw list to UI_TEXTUREDRAW list
//----------------------------------------------------------------------------
void UIDrawListRenderToLocal(
	UI_DRAW * draw,
	int nRenderSection)
{
	// exit if no elements in this drawlist
	if (draw->m_nIndexCount <= 0)
	{
		return;
	}
	
	UI_TEXTUREDRAW * pTextureToDraw = UITextureToDrawGet( draw->m_nVertexCount );

	pTextureToDraw->m_nRenderSection = nRenderSection;
	pTextureToDraw->m_bGrayout = draw->m_bGrayOut;
	if (draw->m_pElementsToDrawFirst != NULL &&
		draw->m_pElementsToDrawFirst->m_eGfxElement == GFXELEMENT_FULLTEXTURE)
	{
		pTextureToDraw->m_pTexture = NULL;
		UI_ELEMENT_FULLTEXTURE *pFullTexture = UIElemCastToFullTexture(draw->m_pElementsToDrawFirst);
		pTextureToDraw->m_nFullTextureID = pFullTexture->m_nEngineTextureID;
	}
	else
	{
		pTextureToDraw->m_nFullTextureID = INVALID_ID;
		pTextureToDraw->m_pTexture = draw->m_pTexture;
		ASSERT(pTextureToDraw->m_pTexture);			
	}

	V( e_UITextureResizeLocalBuffers(pTextureToDraw->m_nLocalBuffer, draw->m_nVertexCount, draw->m_nIndexCount) );

	UITextureWriteLocalBuffers(pTextureToDraw, draw);

	if (g_GraphicGlobals.m_pTextureToDrawLast)
	{
		g_GraphicGlobals.m_pTextureToDrawLast->m_pNext = pTextureToDraw;
	}
	else
	{
		g_GraphicGlobals.m_pTextureToDrawFirst = pTextureToDraw;
	}
	g_GraphicGlobals.m_pTextureToDrawLast = pTextureToDraw;

	draw->m_pElementsToDrawFirst = NULL;
	draw->m_pElementsToDrawLast = NULL;
	draw->m_nVertexCount = 0;
	draw->m_nIndexCount = 0;
}

//----------------------------------------------------------------------------
// write all UI_GFXELEMENTS from draw list to UI_PRIMDRAW list
//----------------------------------------------------------------------------
void UIDrawListRenderToLocal(
	UI_PRIMDRAW * draw,
	int nRenderSection)
{
	// exit if no elements in this drawlist
	if (draw->m_nIndexCount <= 0)
	{
		return;
	}
	
	UI_PRIMITIVEDRAW * prim = UIPrimitiveToDrawGet();

	prim->m_nRenderSection = nRenderSection;
	prim->m_dwFlags = draw->m_dwFlags;

	V( e_UIPrimitiveResizeLocalBuffers(prim->m_nLocalBuffer, draw->m_nVertexCount, draw->m_nIndexCount) );

	UIPrimitiveWriteLocalBuffers(prim, draw);

	if (g_GraphicGlobals.m_pPrimToDrawLast)
	{
		g_GraphicGlobals.m_pPrimToDrawLast->m_pNext = prim;
	}
	else
	{
		g_GraphicGlobals.m_pPrimToDrawFirst = prim;
	}
	g_GraphicGlobals.m_pPrimToDrawLast = prim;

	draw->m_pElementsToDrawFirst = NULL;
	draw->m_pElementsToDrawLast = NULL;
	draw->m_nVertexCount = 0;
	draw->m_nIndexCount = 0;
	prim->m_dwFlags = (DWORD)-1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDrawSetTexture(
	UI_DRAW * pDraw,
	UIX_TEXTURE *pTexture)
{
	pDraw->m_pTexture = pTexture;
	g_GraphicGlobals.m_bReorderDrawLists = TRUE;
}

//----------------------------------------------------------------------------
// add a UI_GFXELEMENT to the draw list
//----------------------------------------------------------------------------
void UIDrawListAddElement(
	UI_DRAW * draw,
	UI_GFXELEMENT * element)
{
	if (!draw->m_pTexture)
	{
		//draw->m_pTexture = element->m_pTexture;
		UIDrawSetTexture(draw, element->m_pTexture);
	}

	if (draw->m_pElementsToDrawLast)
	{
		draw->m_pElementsToDrawLast->m_pNextElementToDraw = element;
	}
	else
	{
		draw->m_pElementsToDrawFirst = element;
	}
	draw->m_pElementsToDrawLast = element;
	element->m_pNextElementToDraw = NULL;

	draw->m_nVertexCount += element->m_nVertexCount;
	draw->m_nIndexCount += element->m_nIndexCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDrawListAddModel(
	int nRenderSection,
	int nAppearanceDefIDForCamera,
	int nModelID,
	UI_RECT& tOrigRect,
	UI_RECT& tClipRect,
	float fRotation,
	int nModelCam)
{
	float fMaxW = (float)AppCommonGetWindowWidth();
	float fMaxH = (float)AppCommonGetWindowHeight();

	UI_RECT tScreenRect;
	tScreenRect.m_fX1 = max( tOrigRect.m_fX1,	0.0f );
	tScreenRect.m_fX1 = min( tScreenRect.m_fX1, fMaxW );
	tScreenRect.m_fX2 = max( tOrigRect.m_fX2,	0.0f );
	tScreenRect.m_fX2 = min( tScreenRect.m_fX2, fMaxW );
	tScreenRect.m_fY1 = max( tOrigRect.m_fY1,	0.0f );
	tScreenRect.m_fY1 = min( tScreenRect.m_fY1, fMaxH );
	tScreenRect.m_fY2 = max( tOrigRect.m_fY2,	0.0f );
	tScreenRect.m_fY2 = min( tScreenRect.m_fY2, fMaxH );

	tScreenRect.m_fX1 = PIN(tScreenRect.m_fX1, tClipRect.m_fX1, tClipRect.m_fX2);
	tScreenRect.m_fX2 = PIN(tScreenRect.m_fX2, tClipRect.m_fX1, tClipRect.m_fX2);
	tScreenRect.m_fY1 = PIN(tScreenRect.m_fY1, tClipRect.m_fY1, tClipRect.m_fY2);
	tScreenRect.m_fY2 = PIN(tScreenRect.m_fY2, tClipRect.m_fY1, tClipRect.m_fY2);

	int nW = (int)( tScreenRect.m_fX2 - tScreenRect.m_fX1 );
	int nH = (int)( tScreenRect.m_fY2 - tScreenRect.m_fY1 );

	// if the viewport size is 0 in either dimension, skip the model
	if ( nW != 0 && nH != 0 )
	{
		UI_MODEL* pModel = UIModelToDrawGet();

		pModel->m_nRenderSection = nRenderSection;
		if (nRenderSection == -1)
		{
			pModel->m_nRenderSection = NUM_RENDER_SECTIONS-1;
		}
		pModel->m_nAppearanceDefIDForCamera = nAppearanceDefIDForCamera;
		pModel->m_nModelID = nModelID;
		memcpy(&pModel->m_tScreenRect, &tScreenRect, sizeof(UI_RECT));
		memcpy(&pModel->m_tOrigRect, &tOrigRect, sizeof(UI_RECT));
		pModel->m_fRotation = fRotation;
		pModel->m_nModelCam = nModelCam;

		if (g_GraphicGlobals.m_pModelToDrawLast)
		{
			g_GraphicGlobals.m_pModelToDrawLast->m_pNext = pModel;
		}
		else
		{
			g_GraphicGlobals.m_pModelToDrawFirst = pModel;
		}
		g_GraphicGlobals.m_pModelToDrawLast = pModel;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDrawListAddPrimitive(
	UI_PRIMDRAW * draw,
	UI_ELEMENT_PRIMITIVE * element )
{
	ASSERT_RETURN( draw->m_dwFlags == element->m_dwFlags );


	if (draw->m_pElementsToDrawLast)
	{
		draw->m_pElementsToDrawLast->m_pNextElementToDraw = element;
	}
	else
	{
		draw->m_pElementsToDrawFirst = element;
	}
	draw->m_pElementsToDrawLast = element;
	element->m_pNextElementToDraw = NULL;

	draw->m_nVertexCount += element->m_nVertexCount;
	draw->m_nIndexCount += element->m_nIndexCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIDrawListAddCallback(
	int nRenderSection,
	PFN_UI_RENDER_CALLBACK pfnRender,
	UI_RECT& tOrigRect,
	UI_RECT& tClipRect,
	float fZDelta,
	float fScale,
	DWORD dwColor )
{
	if ( ! pfnRender )
		return;

	float fMaxW = (float)AppCommonGetWindowWidth();
	float fMaxH = (float)AppCommonGetWindowHeight();

	UI_RECT tScreenRect;
	tScreenRect.m_fX1 = max( tOrigRect.m_fX1,	0.0f );
	tScreenRect.m_fX1 = min( tScreenRect.m_fX1, fMaxW );
	tScreenRect.m_fX2 = max( tOrigRect.m_fX2,	0.0f );
	tScreenRect.m_fX2 = min( tScreenRect.m_fX2, fMaxW );
	tScreenRect.m_fY1 = max( tOrigRect.m_fY1,	0.0f );
	tScreenRect.m_fY1 = min( tScreenRect.m_fY1, fMaxH );
	tScreenRect.m_fY2 = max( tOrigRect.m_fY2,	0.0f );
	tScreenRect.m_fY2 = min( tScreenRect.m_fY2, fMaxH );

	tScreenRect.m_fX1 = PIN(tScreenRect.m_fX1, tClipRect.m_fX1, tClipRect.m_fX2);
	tScreenRect.m_fX2 = PIN(tScreenRect.m_fX2, tClipRect.m_fX1, tClipRect.m_fX2);
	tScreenRect.m_fY1 = PIN(tScreenRect.m_fY1, tClipRect.m_fY1, tClipRect.m_fY2);
	tScreenRect.m_fY2 = PIN(tScreenRect.m_fY2, tClipRect.m_fY1, tClipRect.m_fY2);

	int nW = (int)( tScreenRect.m_fX2 - tScreenRect.m_fX1 );
	int nH = (int)( tScreenRect.m_fY2 - tScreenRect.m_fY1 );

	// if the viewport size is 0 in either dimension, skip the render
	if ( nW != 0 && nH != 0 )
	{
		UI_CALLBACK_RENDER* pCallback = UICallbackToDrawGet();

		pCallback->m_nRenderSection = nRenderSection;
		if (nRenderSection == -1)
		{
			pCallback->m_nRenderSection = NUM_RENDER_SECTIONS-1;
		}
		memcpy(&pCallback->m_tScreenRect, &tScreenRect, sizeof(UI_RECT));
		memcpy(&pCallback->m_tOrigRect, &tOrigRect, sizeof(UI_RECT));
		pCallback->m_dwColor = dwColor;
		pCallback->m_fZDelta = fZDelta;
		pCallback->m_pfnCallback = pfnRender;
		pCallback->m_fScale = fScale;

		if (g_GraphicGlobals.m_pCallbackToDrawLast)
		{
			g_GraphicGlobals.m_pCallbackToDrawLast->m_pNext = pCallback;
		}
		else
		{
			g_GraphicGlobals.m_pCallbackToDrawFirst = pCallback;
		}
		g_GraphicGlobals.m_pCallbackToDrawLast = pCallback;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_DRAW * UIElementGetDrawList(
	UI_GFXELEMENT * element,
	UI_DRAW * draw)
{
	ASSERT_RETVAL(element->m_pTexture || element->m_eGfxElement == GFXELEMENT_FULLTEXTURE, draw);

	// exit if we have a drawlist && it's pointing to the correct texture (or there isn't a texture for this element or drawlist)
	if (draw && 
		(element->m_pTexture == draw->m_pTexture) &&
		(element->m_bGrayOut == draw->m_bGrayOut) )
	{
		return draw;
	}

	// find the drawlist for this element's texture
	draw = &g_GraphicGlobals.m_pTextureDrawLists[0];

	for (int jj = 0; jj < MAX_UI_TEXTURES; jj++)
	{
		if (element->m_pTexture != NULL &&
			element->m_pTexture == g_GraphicGlobals.m_pTextureDrawLists[jj].m_pTexture &&
			element->m_bGrayOut == g_GraphicGlobals.m_pTextureDrawLists[jj].m_bGrayOut)
		{
			draw = &g_GraphicGlobals.m_pTextureDrawLists[jj];
			break;
		}
		// if we hit an empty one, that means this texture hasn't had a drawlist set up for it yet.  Assign it now
		else if (!g_GraphicGlobals.m_pTextureDrawLists[jj].m_pTexture &&
				 !g_GraphicGlobals.m_pTextureDrawLists[jj].m_pElementsToDrawFirst)
		{
			// we want to make sure the fonts are the last texture in each render section
			// (this should be handled with texture draw priorities now
			//if (element->m_pTexture == UIGetFontTexture(LANGUAGE_INVALID) && jj < MAX_UI_TEXTURES - 1)	
			//{
			//	continue;
			//}

			draw = &g_GraphicGlobals.m_pTextureDrawLists[jj];
			UIDrawSetTexture(&(g_GraphicGlobals.m_pTextureDrawLists[jj]), element->m_pTexture);
			draw->m_bGrayOut = element->m_bGrayOut;

			break;
		}
	}

	return draw;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_PRIMDRAW * UIElementGetDrawList(
	UI_ELEMENT_PRIMITIVE * element,
	UI_PRIMDRAW * draw)
{
	// exit if we have a drawlist && it's using the correct flags
	if ( draw && ( draw->m_dwFlags == (DWORD)-1 || element->m_dwFlags == draw->m_dwFlags ) )
	{
		return draw;
	}

	// find the drawlist for this element's flags
	draw = &g_GraphicGlobals.m_pPrimDrawLists[0];

	for (int jj = 0; jj < MAX_UI_PRIM_DRAWLISTS; jj++)
	{
		if (element->m_dwFlags == g_GraphicGlobals.m_pPrimDrawLists[jj].m_dwFlags )
		{
			draw = &g_GraphicGlobals.m_pPrimDrawLists[jj];
			break;
		}
		// if we hit an empty one, that means this prim flags setup hasn't had a drawlist set up for it yet.  Assign it now
		else if (g_GraphicGlobals.m_pPrimDrawLists[jj].m_dwFlags == (DWORD)-1)
		{
			draw = &g_GraphicGlobals.m_pPrimDrawLists[jj];
			g_GraphicGlobals.m_pPrimDrawLists[jj].m_dwFlags = element->m_dwFlags;
			break;
		}
	}

	return draw;
}

//----------------------------------------------------------------------------
// write ui_gfxelements for a component to the draw list
//----------------------------------------------------------------------------
void UIComponentRenderElementsToLocal(
	UI_COMPONENT * component,
	int nRenderSection,
	UI_DRAW *& draw)
{
	// exit if we're not drawing this component's render section
	if (nRenderSection != -1 &&	component->m_nRenderSection != nRenderSection)
	{
		return;
	}

#if ISVERSION(DEVELOPMENT)
	// blink the currently selected component
	if (GameGetDebugFlag(AppGetCltGame(), DEBUGFLAG_UI_EDIT_MODE))
	{
		DWORD dwTime = AppCommonGetRealTime();
		dwTime /= 250;
		if (dwTime % 2)
		{
			UI_COMPONENT *pSelComponent = UIComponentGetById(g_UI.m_idDebugEditComponent);
			ASSERTONCE_RETURN(pSelComponent);

			if (UIComponentIsParentOf(pSelComponent, component))
			{
				return;
			}
		}
	}
#endif

	UI_GFXELEMENT * next = component->m_pGfxElementFirst;
	UI_GFXELEMENT * element = NULL;
	while ((element = next) != NULL)
	{
		next = element->m_pNextElement;

		if (element->m_pBlinkdata && element->m_pBlinkdata->m_eBlinkState == UIBLINKSTATE_OFF)
		{
			continue;
		}

		if (element->m_eGfxElement == GFXELEMENT_MODEL /* && ! g_GraphicGlobals.m_bLoadingScreenUp */ )
		{
			UI_RECT drawrect;
			UIElementGetScreenRect(element, &drawrect);

			UI_POSITION posComponentAbsoluteScreen = UIComponentGetAbsoluteScreenPosition(element->m_pComponent);
			UI_RECT cliprect;
			UIElementGetClipRect(element, &posComponentAbsoluteScreen, &cliprect);
			UIRound(cliprect);

			UI_ELEMENT_MODEL * pModelElement = UIElemCastToModel(element);
			UIDrawListAddModel(nRenderSection, pModelElement->m_nAppearanceDefIDForCamera, pModelElement->m_nModelID, drawrect, cliprect, pModelElement->m_fRotation, pModelElement->m_nModelCam);
		}
		else if (element->m_eGfxElement == GFXELEMENT_PRIM)
		{
			UI_ELEMENT_PRIMITIVE * pPrimitiveElement = UIElemCastToPrim(element);
			UI_RECT drawrect;
			UIElementGetScreenRect(element, &drawrect);

			UI_PRIMDRAW * primdraw = UIElementGetDrawList(pPrimitiveElement, NULL);
			ASSERT_CONTINUE(primdraw);
			ASSERT_CONTINUE(primdraw->m_dwFlags == pPrimitiveElement->m_dwFlags);

			UIDrawListAddPrimitive( primdraw, pPrimitiveElement );
		}
		else if (element->m_eGfxElement == GFXELEMENT_CALLBACK)
		{
			UI_RECT drawrect;
			UIElementGetScreenRect(element, &drawrect);

			UI_POSITION posComponentAbsoluteScreen = UIComponentGetAbsoluteScreenPosition(element->m_pComponent);
			UI_RECT cliprect;
			UIElementGetClipRect(element, &posComponentAbsoluteScreen, &cliprect);
			UIRound(cliprect);

			UI_ELEMENT_CALLBACK * pCallbackElement = UIElemCastToCallback(element);
			UIDrawListAddCallback( nRenderSection, pCallbackElement->m_pfnRender, drawrect, cliprect, element->m_fZDelta, pCallbackElement->m_fScale, element->m_dwColor );
		}
		else
		{
			// set up draw list
			draw = UIElementGetDrawList(element, draw);
			ASSERT_CONTINUE(draw);
			ASSERT_CONTINUE(!draw->m_pTexture || !element->m_pTexture || element->m_pTexture == draw->m_pTexture);

			UIDrawListAddElement(draw, element);
		}
	}
}


//----------------------------------------------------------------------------
// write all UI_GFXELEMENTs for a UI_COMPONENT to the draw list,
// start a new UI_TEXTURETODRAW if the texture changes
//----------------------------------------------------------------------------
void UIComponentRenderToLocal(
	UI_COMPONENT * component,
	int nRenderSection,
	UI_DRAW *& draw)
{
	// exit if the component isn't visibile
	if (!component || !component->m_bVisible)
	{
		return;
	}

	// exit if the component is blinking
	if (component->m_pBlinkData && component->m_pBlinkData->m_eBlinkState == UIBLINKSTATE_OFF)
	{
		return;
	}

	// render individual elements of this component
	UIComponentRenderElementsToLocal(component, nRenderSection, draw);
	
	// render all children
	UI_COMPONENT * child = component->m_pFirstChild;
	while (child)
	{
		UIComponentRenderToLocal(child, nRenderSection, draw);
		child = child->m_pNextSibling;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUIGfxElementFreeData(
	UI_GFXELEMENT * element)
{
	switch (element->m_eGfxElement) 
	{
		case GFXELEMENT_TEXT:
			{
				UI_ELEMENT_TEXT *pTextElement = UIElemCastToText(element);
				if (pTextElement->m_pText)
				{
					FREE(NULL, pTextElement->m_pText);
					pTextElement->m_pText = NULL;
				}
			}
			break;
		case GFXELEMENT_FRAME:
			{
				UI_ELEMENT_FRAME *pFrameElement = UIElemCastToFrame(element);
				if (pFrameElement->m_dwSchedulerTicket)
				{
					CSchedulerCancelEvent(pFrameElement->m_dwSchedulerTicket);
				}
			}
	}
	if (element->m_pBlinkdata)
	{
		FREE(NULL, element->m_pBlinkdata);
		element->m_pBlinkdata = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UIRecycleElement(
	UI_GFXELEMENT * element)
{
	ASSERT_RETURN(element);
	int eGfxElement = element->m_eGfxElement;
	ASSERT_RETURN(eGfxElement >= 0 && eGfxElement < NUM_GFXELEMENTTYPES);
	sUIGfxElementFreeData(element);
	element->m_pComponent = NULL;
	element->m_pNextElement = g_GraphicGlobals.m_pGfxElementGarbage[eGfxElement];
	g_GraphicGlobals.m_pGfxElementGarbage[eGfxElement] = element;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentRemoveAllElements(
	UI_COMPONENT * component)
{
#if ISVERSION(DEVELOPMENT)
	static DWORD s_dwThreadId = GetCurrentThreadId();
	ASSERT_RETURN(GetCurrentThreadId() == s_dwThreadId);
#endif
	ASSERT_RETURN(component);
	if (component->m_pGfxElementFirst)
	{
		UISetNeedToRender(component->m_nRenderSection);
		while (component->m_pGfxElementFirst)
		{
			UI_GFXELEMENT * element = component->m_pGfxElementFirst;
			ASSERT(element->m_pComponent == component);
			component->m_pGfxElementFirst = element->m_pNextElement;
			UIRecycleElement(element);
		}
	}
	component->m_pGfxElementFirst = NULL;
	component->m_pGfxElementLast = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIElementRectLogToScreen(
	UI_RECT& rect, 
	BOOL bNoScaleSize)
{
	float width  = (rect.m_fX2 - rect.m_fX1) * UIGetLogToScreenRatioX(bNoScaleSize);
	float height = (rect.m_fY2 - rect.m_fY1) * UIGetLogToScreenRatioY(bNoScaleSize);
	rect.m_fX1 *= UIGetLogToScreenRatioX();
	rect.m_fY1 *= UIGetLogToScreenRatioY();
	rect.m_fX2 = rect.m_fX1 + width;
	rect.m_fY2 = rect.m_fY1 + height;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIElementGetScreenRect(
	UI_GFXELEMENT* element,
	UI_RECT* screenrect)
{
	ASSERT_RETURN(element);
	ASSERT_RETURN(screenrect);

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(element->m_pComponent);

	screenrect->m_fX1 = element->m_Position.m_fX + pos.m_fX - element->m_fXOffset;
	screenrect->m_fY1 = element->m_Position.m_fY + pos.m_fY - element->m_fYOffset;
	screenrect->m_fX2 = screenrect->m_fX1 + element->m_fWidth;
	screenrect->m_fY2 = screenrect->m_fY1 + element->m_fHeight;

	screenrect->m_fX1 *= UIGetLogToScreenRatioX(); 
	screenrect->m_fY1 *= UIGetLogToScreenRatioY();
	screenrect->m_fX2 *= UIGetLogToScreenRatioX(); 
	screenrect->m_fY2 *= UIGetLogToScreenRatioY();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UICountPrintableChar(
	const WCHAR * text,
	int * printable)
{
	int len = 0;
	*printable = 0;
	while (*text)
	{
		if (*text == CTRLCHAR_COLOR || *text == CTRLCHAR_LINECOLOR)
		{
			len++;
			text++;
			if (*text == 0)
			{
				break;
			}
		}
		else if (*text == CTRLCHAR_HYPERTEXT)
		{
			text++;
			len++;
		}
		else if (*text == CTRLCHAR_HYPERTEXT_END)
		{
			// do nothing (text and len get incremented at the end of the loop
		}
		else if (PStrIsPrintable( *text ))		
		{
			*printable = *printable + 1;
		}
		len++;
		text++;
	}
	return len + 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float UIGetCharWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio,
	BOOL bNoScale,
	WCHAR wc)
{
	// KCK: While editting the font to adjust for control character widths might be possible,
	// I'm just going to force them to zero here.
	if (wc <= CTRLCHAR_HYPERTEXT_END)
		return 0.0f;

	float fLogToScreenRatioX = UIGetLogToScreenRatioX(bNoScale);	
	float fLogToScreenRatioY = UIGetLogToScreenRatioY(bNoScale);
	// adjust for aspect ratio
	fWidthRatio /= (fLogToScreenRatioX / fLogToScreenRatioY);

	nFontSize = (int)(((float)nFontSize * fLogToScreenRatioY) + 0.5f);
	float fWidth = e_UIFontGetCharWidth(font, nFontSize, fWidthRatio, wc);
	return fWidth  / fLogToScreenRatioY;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGFXElementSetClipRect(
	UI_GFXELEMENT *element, 
	const UI_RECT *cliprect)
{
	ASSERTX_RETURN( element, "Expected ui gfx element" );
	
	if (cliprect)
	{
		element->m_ClipRect = *cliprect;
	}
	else
	{
		element->m_ClipRect = UI_RECT(-UIDefaultWidth(), -UIDefaultHeight(), UIDefaultWidth(), UIDefaultHeight());
	}

}
	
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//float UIElementGetTextLogWidth(
//	UIX_TEXTURE_FONT* font,
//	int nFontSize,
//	float fWidthRatio,
//	BOOL bNoScale,
//	const WCHAR* text,
//	BOOL bSingleLine /*= FALSE*/,
//	int nPage /*= 0*/,
//	int nMaxCharacters /*= -1*/)
//{
//	if (!text || !text[0])
//		return 0.0f;
//
//	ASSERT_RETZERO(font);
//
//	float maxwidth = 0.0f;
//	float width = 0.0f;
//
//	float fTabWidth = font->m_fTabWidth * fWidthRatio;
//	int nCurrentPage = 0;
//	int nChars = 0;
//	int nModifiedFontSize =  (int)( ( (float)nFontSize * g_UI.m_fUIScaler ) + scfFontSizeScaleUp);
//
//	WCHAR wc;
//	while ((wc = *text++) != 0 &&
//		(nChars < nMaxCharacters || nMaxCharacters == -1))
//	{
//		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR)
//		{
//			if (*text == 0)
//			{
//				break;
//			}
//
//			text++;		// skip the character that identifies the color
//
//			continue;
//		}
//
//		if (wc == CTRLCHAR_COLOR_RETURN)
//		{
//			continue;
//		}
//
//		if (wc == TEXT_TAG_BEGIN_CHAR)
//		{
//			int nTextTag = UIDecodeTextTag(text);
//			if (nTextTag == TEXT_TAG_PGBRK ||
//				nTextTag == TEXT_TAG_INTERRUPT ||
//				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
//			{
//				nCurrentPage++;
//			}
//			if (nTextTag != TEXT_TAG_INVALID)
//			{
//				// this is a text tag enclosed in brackets, account for its space
//				int nNumChars = 0;
//				UI_SIZE sizeTag;
//				BOOL bForDrawOnly = FALSE;
//				if (UIGetStringTagSize(text, NULL, &sizeTag, &nNumChars, &bForDrawOnly))
//				{
//					if (!bForDrawOnly)
//					{
//						width += sizeTag.m_fWidth;
//					}
//					text += nNumChars;
//					continue;
//				}
//				else
//				{
//					// ok, must have been some malformed tag.  Just fall through and print it
//				}
//			}
//		}
//
//		if (nPage != -1 && nPage != nCurrentPage)
//		{
//			continue;
//		}
//
//		if (wc == L'\n')
//		{
//			if (bSingleLine)
//			{
//				return width;
//			}
//			maxwidth = MAX(width, maxwidth);
//			width = 0;
//			nChars++;
//			continue;
//		}
//		if (wc == L'\t')
//		{
//			width = float(int( width / fTabWidth + 0.5f) + 1 ) * fTabWidth;
//			continue;
//		}
//		
//		if (PStrIsPrintable(wc) == FALSE)
//		{
//			continue;
//		}
//
//		nChars++;
//		wc = sUIJabberChar(font, wc);
//		wc = sUICheckGlyphset(font, wc);
//
//		width += UIGetCharWidth(font, nModifiedFontSize, fWidthRatio, bNoScale, wc);
//	}
//	maxwidth = MAX(width, maxwidth);
//	return maxwidth;
//}
//
////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//float UIElementGetTextLogHeight(
//	UIX_TEXTURE_FONT* font,
//	int nFontSize,
//	const WCHAR* text,
//	BOOL bNoScale,
//	int nPage /*= 0*/)
//{
//	if (!text || !text[0])
//		return 0.0f;
//
//	// Ok, the font system is going to be doing some rounding, so we need to adjust for that here.
//	float fLogToScreenRatioY = UIGetLogToScreenRatioY(bNoScale);
//	int nAdjFontSize = (int)(((float)nFontSize * fLogToScreenRatioY * g_UI.m_fUIScaler) + scfFontSizeScaleUp);
//	float fLineHeight = (float)nAdjFontSize;
//
//	float fThisLineHeight = fLineHeight;
//	float height = 0.0f;
//	int nCurrentPage = 0;
//
//	WCHAR wc;
//	while ((wc = *text++) != 0)
//	{
//		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR)
//		{
//			if (*text == 0)
//			{
//				break;
//			}
//			text++;
//			continue;
//		}
//
//		if (wc == TEXT_TAG_BEGIN_CHAR)
//		{
//			int nTextTag = UIDecodeTextTag(text);
//			if (nTextTag == TEXT_TAG_PGBRK ||
//				nTextTag == TEXT_TAG_INTERRUPT ||
//				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
//			{
//				nCurrentPage++;
//			}
//			if (nTextTag != TEXT_TAG_INVALID)
//			{
//				// this is a text tag enclosed in brackets, account for its space
//				int nNumChars = 0;
//				UI_SIZE sizeTag;
//				BOOL bForDrawOnly = FALSE;
//				if (UIGetStringTagSize(text, NULL, &sizeTag, &nNumChars, &bForDrawOnly))
//				{
//					if (!bForDrawOnly)
//					{
//						fThisLineHeight = MAX(fThisLineHeight, sizeTag.m_fHeight);
//					}
//					text += nNumChars;
//					continue;
//				}
//				else
//				{
//					// ok, must have been some malformed tag.  Just fall through and print it
//				}
//			}
//		}
//
//		if (nPage != -1 && nPage != nCurrentPage)
//		{
//			continue;
//		}
//
//		if (wc == L'\n')
//		{
//			height += fThisLineHeight + 1.0f;
//			fThisLineHeight = fLineHeight;
//		}
//	}
//
//										// now divide the adjustment back out
//										// this is done because of the possible difference the rounding up of font size could make
//	return (height + fThisLineHeight) / fLogToScreenRatioY;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIElementGetTextLogSize(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio,
	BOOL bNoScale,
	const WCHAR* text,
	float *pfWidth,
	float *pfHeight,
	BOOL bSingleLineForWidth /*= FALSE*/,
	BOOL bSingleLineForHeight /*= FALSE*/,
	int nPage /*= 0*/,
	int nMaxCharacters /*= -1*/,
	UI_TXT_BKGD_RECTS * pRectStruct /*= NULL*/)
{
	if (pfWidth)
		*pfWidth = 0.0f;
	if (pfHeight)
		*pfHeight = 0.0f;
	if (pRectStruct)
		pRectStruct->nNumRects = 0;

	if (!text || !text[0])
		return FALSE;

	if (nFontSize < 1)
		return FALSE;

	ASSERT_RETZERO(font);

	float maxwidth = 0.0f;
	float width = 0.0f;

	float fTabWidth = font->m_fTabWidth * fWidthRatio;
	int nCurrentPage = 0;
	int nChars = 0;
	int nModifiedFontSize =  (int)( ( (float)nFontSize * g_UI.m_fUIScaler ) + scfFontSizeScaleUp);

//	float fLogToScreenRatioX = UIGetLogToScreenRatioX(bNoScale);
	// Ok, the font system is going to be doing some rounding, so we need to adjust for that here.
	float fLogToScreenRatioY = UIGetLogToScreenRatioY(bNoScale);
	int nAdjFontSize = (int)(((float)nFontSize * fLogToScreenRatioY * g_UI.m_fUIScaler) + scfFontSizeScaleUp);
	float fLineHeight = (float)nAdjFontSize;
	float fSpaceBetweenLines = 1.0f * fLogToScreenRatioY;

	float fThisLineHeight = fLineHeight;
	float height = 0.0f;

	BOOL bLinePassed = FALSE;
	WCHAR wc;
	while ((wc = *text++) != 0 &&
		(nChars < nMaxCharacters || nMaxCharacters == -1))
	{
		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR)
		{
			if (*text == 0)
			{
				break;
			}

			text++;		// skip the character that identifies the color

			continue;
		}
		if (wc == CTRLCHAR_COLOR_RETURN || wc == CTRLCHAR_HYPERTEXT_END)
		{
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT )
		{
			text ++;
			continue;
		}

		if (wc == TEXT_TAG_BEGIN_CHAR)
		{
			int nTextTag = UIDecodeTextTag(text);
			if (nTextTag == TEXT_TAG_PGBRK ||
				nTextTag == TEXT_TAG_INTERRUPT ||
				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
			{
				nCurrentPage++;
			}
			if (nTextTag != TEXT_TAG_INVALID)
			{
				// this is a text tag enclosed in brackets, account for its space
				int nNumChars = 0;
				UI_SIZE sizeTag;
				BOOL bForDrawOnly = FALSE;
				if (UIGetStringTagSize(text, NULL, &sizeTag, &nNumChars, &bForDrawOnly))
				{
					if (!bForDrawOnly)
					{
						width += sizeTag.m_fWidth;
						fThisLineHeight = MAX(fThisLineHeight, sizeTag.m_fHeight);
					}
					text += nNumChars;
					continue;
				}
				else
				{
					// ok, must have been some malformed tag.  Just fall through and print it
				}
			}
		}

		if (nPage != -1 && nPage != nCurrentPage)
		{
			continue;
		}

		if (wc == L'\n' || wc == L'\r')
		{
			if (pRectStruct)
			{
				if (pRectStruct->nNumRects < pRectStruct->nBufferSize)
				{
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX1 = 0.0f - UI_TXT_BKGD_RECT_X_BUF;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX2 = width + UI_TXT_BKGD_RECT_X_BUF;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 = height;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 = height + fThisLineHeight + fSpaceBetweenLines;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 *= fWidthRatio / fLogToScreenRatioY;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 *= fWidthRatio / fLogToScreenRatioY;
					pRectStruct->nNumRects++;
				}
			}

			if (!bSingleLineForHeight || !bLinePassed)
			{
				height += fThisLineHeight + fSpaceBetweenLines;
				fThisLineHeight = fLineHeight;
			}

			if (bSingleLineForHeight && bSingleLineForWidth)
			{
				goto _done;
			}

			if (!bSingleLineForWidth || !bLinePassed)
			{
				maxwidth = MAX(width, maxwidth);
				width = 0;
			}
			nChars++;

			bLinePassed = TRUE;
			continue;
		}
		if (wc == L'\t')
		{
			if (!bSingleLineForWidth || !bLinePassed)
			{
				width = float(int( width / fTabWidth + 0.5f) + 1 ) * fTabWidth;
			}
			continue;
		}
		
		if (PStrIsPrintable(wc) == FALSE)
		{
			continue;
		}

		nChars++;
		if (!bSingleLineForWidth || !bLinePassed)
		{
			wc = sUIJabberChar(font, wc);
			wc = sUICheckGlyphset(font, wc);

			width += UIGetCharWidth(font, nModifiedFontSize, fWidthRatio, bNoScale, wc);
		}
	}

_done:
	if (pRectStruct)
	{
		if (pRectStruct->nNumRects < pRectStruct->nBufferSize)
		{
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX1 = 0.0f - UI_TXT_BKGD_RECT_X_BUF;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX2 = width +  UI_TXT_BKGD_RECT_X_BUF;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 = height;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 = height + fThisLineHeight + fSpaceBetweenLines;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 *= fWidthRatio / fLogToScreenRatioY;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 *= fWidthRatio / fLogToScreenRatioY;
			pRectStruct->nNumRects++;
		}
	}
	if (pfWidth)
		*pfWidth = MAX(width, maxwidth);
	if (pfHeight)
		*pfHeight = ((height + fThisLineHeight) / fLogToScreenRatioY) * fWidthRatio;	
												// now divide the adjustment back out
												// this is done because of the possible difference 
												// the rounding up of font size could make
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIXGraphicInit()
{
	structclear(g_GraphicGlobals);
	structclear(g_dwRenderState);
	ASSERT_RETFALSE(arrsize(scbPrepareModelRenderSection) == arrsize(g_dwRenderState));

	for (int i=0; i < MAX_UI_PRIM_DRAWLISTS; i++)
	{
		g_GraphicGlobals.m_pPrimDrawLists[i].m_dwFlags = (DWORD)-1;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGfxElementFreeGarbage(
	UI_GFXELEMENT * element)
{
	while (element)
	{
		UI_GFXELEMENT * next = element->m_pNextElement;
		sUIGfxElementFreeData(element);
		FREE(g_StaticAllocator, element);
		element = next;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIXGraphicFree()
{
	UI_TEXTUREDRAW* tex = g_GraphicGlobals.m_pTextureToDrawFirst;
	while (tex)
	{
		UI_TEXTUREDRAW* next = tex->m_pNext;
		UITextureDrawFree(tex);
		tex = next;
	}
	g_GraphicGlobals.m_pTextureToDrawFirst = NULL;

	tex = g_GraphicGlobals.m_pTextureToDrawGarbage;
	while (tex)
	{
		UI_TEXTUREDRAW* next = tex->m_pNext;
		UITextureDrawFree(tex);
		tex = next;
	}
	g_GraphicGlobals.m_pTextureToDrawGarbage = NULL;

	UI_MODEL* pModel = g_GraphicGlobals.m_pModelToDrawFirst;
	while (pModel)
	{
		UI_MODEL* next = pModel->m_pNext;
		UIModelDrawFree(pModel);
		pModel = next;
	}
	g_GraphicGlobals.m_pModelToDrawFirst = NULL;

	pModel = g_GraphicGlobals.m_pModelToDrawGarbage;
	while (pModel)
	{
		UI_MODEL* next = pModel->m_pNext;
		UIModelDrawFree(pModel);
		pModel = next;
	}
	g_GraphicGlobals.m_pModelToDrawGarbage = NULL;

	UI_PRIMITIVEDRAW* prim = g_GraphicGlobals.m_pPrimToDrawFirst;
	while (prim)
	{
		UI_PRIMITIVEDRAW* next = prim->m_pNext;
		UIPrimitiveDrawFree(prim);
		prim = next;
	}
	g_GraphicGlobals.m_pPrimToDrawFirst = NULL;

	prim = g_GraphicGlobals.m_pPrimToDrawGarbage;
	while (prim)
	{
		UI_PRIMITIVEDRAW* next = prim->m_pNext;
		UIPrimitiveDrawFree(prim);
		prim = next;
	}
	g_GraphicGlobals.m_pPrimToDrawGarbage = NULL;

	UI_CALLBACK_RENDER* pCallback = g_GraphicGlobals.m_pCallbackToDrawFirst;
	while (pCallback)
	{
		UI_CALLBACK_RENDER* next = pCallback->m_pNext;
		UICallbackDrawFree(pCallback);
		pCallback = next;
	}
	g_GraphicGlobals.m_pCallbackToDrawFirst = NULL;

	pCallback = g_GraphicGlobals.m_pCallbackToDrawGarbage;
	while (pCallback)
	{
		UI_CALLBACK_RENDER* next = pCallback->m_pNext;
		UICallbackDrawFree(pCallback);
		pCallback = next;
	}
	g_GraphicGlobals.m_pCallbackToDrawGarbage = NULL;

	if (AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		V( e_UIFree() );
	}

	for (int i = 0; i < NUM_GFXELEMENTTYPES; i++)
	{
		UIGfxElementFreeGarbage(g_GraphicGlobals.m_pGfxElementGarbage[i]);
		g_GraphicGlobals.m_pGfxElementGarbage[i] = NULL;
	}

	//if (g_GraphicGlobals.m_pTextureDrawLists)
	//{
	//	FREE(NULL, g_GraphicGlobals.m_pTextureDrawLists);
	//	g_GraphicGlobals.m_pTextureDrawLists = NULL;
	//}

}


//----------------------------------------------------------------------------
// clear all draw lists
//----------------------------------------------------------------------------
void UIClearDrawLists(
	void)
{
	for (int jj = 0; jj < MAX_UI_TEXTURES; jj++)
	{
		g_GraphicGlobals.m_pTextureDrawLists[jj].m_pElementsToDrawFirst = NULL;
		g_GraphicGlobals.m_pTextureDrawLists[jj].m_pElementsToDrawLast = NULL;
		g_GraphicGlobals.m_pTextureDrawLists[jj].m_nVertexCount = 0;
		g_GraphicGlobals.m_pTextureDrawLists[jj].m_nIndexCount = 0;
	}
}


//----------------------------------------------------------------------------
// render all draw lists
//----------------------------------------------------------------------------
static int sOrderDrawLists(const void * p1, const void * p2)
{
	UI_DRAW *pDraw1 = (UI_DRAW *)p1;
	UI_DRAW *pDraw2 = (UI_DRAW *)p2;

	if (!pDraw1->m_pTexture && !pDraw2->m_pTexture)	return 0;
	if (!pDraw1->m_pTexture)	return 1;
	if (!pDraw2->m_pTexture)	return -1;
	if (pDraw1->m_pTexture->m_nDrawPriority < pDraw2->m_pTexture->m_nDrawPriority)	return -1;
	if (pDraw1->m_pTexture->m_nDrawPriority > pDraw2->m_pTexture->m_nDrawPriority)	return 1;
	return 0;

}

void UIRenderDrawLists(
	int iRenderSection)
{
	if (g_GraphicGlobals.m_bReorderDrawLists)
	{
		// re-order the draw lists
		int nDrawCount = 0;
		for (; nDrawCount < MAX_UI_TEXTURES; nDrawCount++)
		{
			if (!g_GraphicGlobals.m_pTextureDrawLists[nDrawCount].m_pTexture &&
				!g_GraphicGlobals.m_pTextureDrawLists[nDrawCount].m_pElementsToDrawFirst)
			{
				break;
			}
		}

		// order the drawlists by their texture priority
		qsort(g_GraphicGlobals.m_pTextureDrawLists, nDrawCount, sizeof(UI_DRAW), sOrderDrawLists);
		g_GraphicGlobals.m_bReorderDrawLists = FALSE;
	}

	// render all the drawlists to draw textures
	for (int jj = 0; jj < MAX_UI_TEXTURES; jj++)
	{
		if (g_GraphicGlobals.m_pTextureDrawLists[jj].m_pElementsToDrawFirst)
		{
			UIDrawListRenderToLocal(&g_GraphicGlobals.m_pTextureDrawLists[jj], iRenderSection);
		}
	}

	// render all the drawlists to draw primitives
	for (int jj = 0; jj < MAX_UI_PRIM_DRAWLISTS; jj++)
	{
		if ( g_GraphicGlobals.m_pPrimDrawLists[jj].m_dwFlags != (DWORD)-1 )
		{
			UIDrawListRenderToLocal(&g_GraphicGlobals.m_pPrimDrawLists[jj], iRenderSection);
		}
	}
}


//----------------------------------------------------------------------------
// draw all components in ui to UI_TEXTUREDRAW list
//----------------------------------------------------------------------------
void UIRenderToLocal(
	UI_COMPONENT *pComponentList,
	UI_COMPONENT *pCursor)
{
	if (pComponentList)
	{
		// This will clear all the textures that need it
		UITextureToDrawClear();
		UIModelToDrawClear();
		UIPrimitiveToDrawClear();
		UICallbackToDrawClear();
		UIClearDrawLists();


		// debug
		//if ( g_GraphicGlobals.m_Cursor )
		//{
		//	float w = 40.f, h = 30.f;
		//	UIComponentAddPrimitiveElement(
		//		g_GraphicGlobals.m_Cursor,
		//		UIPRIM_BOX,
		//		0, 
		//		UI_RECT(-w / 2.0f, -h / 2.0f, w / 2.0f, h / 2.0f),
		//		NULL,
		//		NULL );
		//}



		ASSERT_RETURN(g_GraphicGlobals.m_pTextureDrawLists != NULL);
		ASSERT_RETURN(g_GraphicGlobals.m_pPrimDrawLists != NULL);

		for (int iRenderSection = 0; iRenderSection < NUM_RENDER_SECTIONS; iRenderSection++)
		{
			// only if something has changed
			if (g_dwRenderState[iRenderSection] & UIRS_NEED_UPDATE)
			{

				// if we're using the hide UI cheat, only draw the debug UI elements (console)
				if (g_GraphicGlobals.m_bHideUI && iRenderSection != RENDERSECTION_DEBUG)
					continue;

				// add all components that are in this render section to a drawlist for their texture
				UI_DRAW *pDraw = NULL;
				UIComponentRenderToLocal(pComponentList, iRenderSection, pDraw);

				if (pCursor)
				{
					UIComponentRenderToLocal(pCursor, iRenderSection, pDraw);
				}

				// copy elements from all drawlists into vertex/index buffers
				UIRenderDrawLists(iRenderSection);

				// Clear the need update flag and set the need render flag
				g_dwRenderState[iRenderSection] &= ~(UIRS_NEED_UPDATE);
				g_dwRenderState[iRenderSection] |= UIRS_NEED_RENDER;
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UIComponentValidateGfxElements(
	UI_COMPONENT * component)
{
#if 0
	ASSERT_RETURN(component);
	UI_GFXELEMENT * element = component->m_pGfxElementFirst;
	while (element)
	{
		ASSERT(element->m_pComponent == component);
		if (element->m_pNextElement == NULL)
		{
			ASSERT(element == component->m_pGfxElementLast);
		}
		element = element->m_pNextElement;
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UIComponentAddGfxElement(
	UI_COMPONENT * component,
	UI_GFXELEMENT * element)
{
#if ISVERSION(DEVELOPMENT)
	static DWORD s_dwThreadId = GetCurrentThreadId();
	ASSERT_RETURN(GetCurrentThreadId() == s_dwThreadId);
#endif
	ASSERT_RETURN(component);
	ASSERT_RETURN(element);
	ASSERT_RETURN(element->m_pNextElement == NULL);

//	keytrace("AddGfxElement():  COMP[%s]\n", component->m_szName);
	UIComponentValidateGfxElements(component);
	if (component->m_pGfxElementLast)
	{
		component->m_pGfxElementLast->m_pNextElement = element;
	}
	else
	{
		component->m_pGfxElementFirst = element;
	}
	component->m_pGfxElementLast = element;
	UIComponentValidateGfxElements(component);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT * UIGetNewGfxElement(
	int nElemType,
	UI_COMPONENT * component)
{
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(nElemType >= 0 && nElemType < NUM_GFXELEMENTTYPES);

	UI_GFXELEMENT * element = NULL;

	if (g_GraphicGlobals.m_pGfxElementGarbage[nElemType])
	{
		element = g_GraphicGlobals.m_pGfxElementGarbage[nElemType];
		g_GraphicGlobals.m_pGfxElementGarbage[nElemType] = element->m_pNextElement;

		int nSize = sizeof(UI_GFXELEMENT);
		switch (nElemType)
		{
			case GFXELEMENT_FRAME:			nSize = sizeof(UI_ELEMENT_FRAME);		break;
			case GFXELEMENT_TEXT:			nSize = sizeof(UI_ELEMENT_TEXT);		break;
			case GFXELEMENT_TRI:			nSize = sizeof(UI_ELEMENT_TRI);			break;
			case GFXELEMENT_RECT:			nSize = sizeof(UI_ELEMENT_RECT);		break;
			case GFXELEMENT_ROTFRAME:		nSize = sizeof(UI_ELEMENT_ROTFRAME);	break;
			case GFXELEMENT_MODEL:			nSize = sizeof(UI_ELEMENT_MODEL);		break;
			case GFXELEMENT_PRIM:			nSize = sizeof(UI_ELEMENT_PRIMITIVE);	break;
			case GFXELEMENT_CALLBACK:		nSize = sizeof(UI_ELEMENT_CALLBACK);	break;
			case GFXELEMENT_FULLTEXTURE:	nSize = sizeof(UI_ELEMENT_FULLTEXTURE);	break;
			default: 
				return NULL;
		}

		memclear(element, nSize);
		element->m_eGfxElement = nElemType;
	}
	else
	{
		switch(nElemType)
		{
			case GFXELEMENT_FRAME:			element = (UI_ELEMENT_FRAME*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_FRAME)); break;
			case GFXELEMENT_TEXT:			element = (UI_ELEMENT_TEXT*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_TEXT)); break;
			case GFXELEMENT_TRI:			element = (UI_ELEMENT_TRI*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_TRI)); break;
			case GFXELEMENT_RECT:			element = (UI_ELEMENT_RECT*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_RECT)); break;
			case GFXELEMENT_ROTFRAME:		element = (UI_ELEMENT_ROTFRAME*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_ROTFRAME)); break;
			case GFXELEMENT_MODEL:			element = (UI_ELEMENT_MODEL*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_MODEL)); break;
			case GFXELEMENT_PRIM:			element = (UI_ELEMENT_PRIMITIVE*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_PRIMITIVE)); break;
			case GFXELEMENT_CALLBACK:		element = (UI_ELEMENT_CALLBACK*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_CALLBACK)); break;
			case GFXELEMENT_FULLTEXTURE:	element = (UI_ELEMENT_FULLTEXTURE*)MALLOCZ(g_StaticAllocator, sizeof(UI_ELEMENT_FULLTEXTURE)); break;
			default: 
				return NULL;
		}

		element->m_eGfxElement = nElemType;
	}

	element->m_pComponent = component;
	element->m_qwData = (QWORD)INVALID_ID;
	element->m_pNextElement = NULL;

	if (nElemType == GFXELEMENT_MODEL)
	{
		UI_ELEMENT_MODEL *pModelElement = (UI_ELEMENT_MODEL *)element;
		pModelElement->m_nUnitID = INVALID_ID;
	}

	if (nElemType == GFXELEMENT_CALLBACK)
	{
		UI_ELEMENT_CALLBACK *pCallbackElement = (UI_ELEMENT_CALLBACK *)element;
		pCallbackElement->m_pfnRender = NULL;
		//pCallbackElement->m_nUpdateID = INVALID_ID;
	}

	if (nElemType == GFXELEMENT_FULLTEXTURE)
	{
		UI_ELEMENT_FULLTEXTURE *pTextureElement = (UI_ELEMENT_FULLTEXTURE *)element;
		pTextureElement->m_nEngineTextureID = INVALID_ID;
		pTextureElement->m_nUnitID = INVALID_ID;
	}

	UIComponentAddGfxElement(component, element);

	return element;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIRestoreVBuffers(
	void)
{
	UI_TEXTUREDRAW* texture = g_GraphicGlobals.m_pTextureToDrawFirst;
	while (texture)
	{
		//trace( "Restoring VBuffer: texture 0x%p, texture->next 0x%p\n", texture, texture->m_pNext );
		BOOL bDiscard = texture->m_pTexture
							? ( (texture->m_pTexture->m_dwFlags & UI_TEXTURE_DISCARD) != 0 )
							: FALSE;
		V( e_UICopyToEngineBuffer( texture->m_nLocalBuffer, texture->m_nEngineBuffer, bDiscard, TRUE ) );
		texture = texture->m_pNext;
	}

	texture = g_GraphicGlobals.m_pTextureToDrawGarbage;
	while (texture)
	{
		texture->m_nEngineBuffer = INVALID_ID;
		texture = texture->m_pNext;
	}

	// CHB 2007.09.27 - This is the mechanism used to restore the
	// gamma setting in the options.
	UIOptionsNotifyRestoreEvent();
}


//----------------------------------------------------------------------------
// perf'd in DRAW_UI
//----------------------------------------------------------------------------
void UIRenderAll(
	void)
{
	for (int ii=0; ii < NUM_RENDER_SECTIONS; ii++)
	{
		UI_PRIMITIVEDRAW* prim = g_GraphicGlobals.m_pPrimToDrawFirst;
		while (prim)
		{
			if (prim->m_nRenderSection == ii)
			{
				if (g_dwRenderState[ii] & UIRS_NEED_RENDER ||
					prim->m_nEngineBuffer == INVALID_ID)
				{
					V( e_UICopyToEngineBuffer(prim->m_nLocalBuffer, prim->m_nEngineBuffer, /* what to put here? */ TRUE ) );
				}

				V( e_UIPrimitiveRender( prim->m_nEngineBuffer, prim->m_nLocalBuffer, prim->m_dwFlags ) );
			}

			prim = prim->m_pNext;
		}

		UI_CALLBACK_RENDER* pCallback = g_GraphicGlobals.m_pCallbackToDrawFirst;
		while (pCallback)
		{
			if (pCallback->m_nRenderSection == ii && pCallback->m_pfnCallback)
			{
				pCallback->m_pfnCallback( pCallback->m_tScreenRect, pCallback->m_dwColor, pCallback->m_fZDelta, pCallback->m_fScale );
			}

			pCallback = pCallback->m_pNext;
		}

		UI_TEXTUREDRAW* texture = g_GraphicGlobals.m_pTextureToDrawFirst;
		while (texture)
		{
			if (texture->m_nRenderSection == ii)
			{
				if (g_dwRenderState[ii] & UIRS_NEED_RENDER ||
					texture->m_nEngineBuffer == INVALID_ID)
				{
					BOOL bDynamic = (texture->m_pTexture ? (texture->m_pTexture->m_dwFlags & UI_TEXTURE_DISCARD) : FALSE);
					V( e_UICopyToEngineBuffer(texture->m_nLocalBuffer, texture->m_nEngineBuffer, bDynamic ) );
				}

				//ASSERT_CONTINUE( texture && texture->m_pTexture );		// removed for the fulltexture element types who don't use this
				BOOL bOpaque = (texture->m_pTexture ? (texture->m_pTexture->m_dwFlags & UI_TEXTURE_OPAQUE) : FALSE);
				int nTextureId = INVALID_ID;
				if (texture->m_pTexture)
				{
					nTextureId = texture->m_pTexture->m_nTextureId;
				}
				else
				{
					nTextureId = texture->m_nFullTextureID;
				}
				V( e_UITextureRender( nTextureId, texture->m_nEngineBuffer, texture->m_nLocalBuffer, bOpaque, texture->m_bGrayout) );
			}

			texture = texture->m_pNext;
		}

		if (scbPrepareModelRenderSection[ii])
		{	
			V( e_PrepareUIModelRenders() );
		}

		UI_MODEL* pModel = g_GraphicGlobals.m_pModelToDrawFirst;
		while (pModel)
		{
			if (pModel->m_nRenderSection == ii &&
				e_IsValidModelID(pModel->m_nModelID))
			{
				APPEARANCE_DEFINITION * pAppDef = AppearanceDefinitionGet( pModel->m_nAppearanceDefIDForCamera );
				if (pAppDef)
				{
					if ( ! IS_VALID_INDEX( pModel->m_nModelCam, pAppDef->nInventoryViews ) )
						pModel->m_nModelCam = pAppDef->nInventoryViews-1;
					if ( pModel->m_nModelCam >= 0)
					{
						ASSERTX( pAppDef->pInventoryViews, pAppDef->tHeader.pszName );
						INVENTORY_VIEW_INFO * pInventoryView = &pAppDef->pInventoryViews[ pModel->m_nModelCam ];

						VECTOR vCamFocus = pInventoryView->vCamFocus;
						if ( pInventoryView->nBone != INVALID_ID )
						{
							MATRIX mBone;
							c_AppearanceGetBoneMatrix( pModel->m_nModelID, &mBone, pInventoryView->nBone );
							const MATRIX *pmModelWorldScaled = e_ModelGetWorldScaled( pModel->m_nModelID );
							MatrixMultiply( &mBone, &mBone, pmModelWorldScaled );
							VECTOR vPos( 0.0f );
							MatrixMultiply( &vPos, &vPos, &mBone );
							vCamFocus += vPos;
						}

#if ISVERSION(DEVELOPMENT)
						int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
						if ( nIsolateModel != INVALID_ID && nIsolateModel != pModel->m_nModelID )
						{
							pModel = pModel->m_pNext;
							continue;
						}
#endif

						e_UIModelRender(
							pModel->m_nModelID,
							&pModel->m_tScreenRect,
							&pModel->m_tOrigRect,
							pModel->m_fRotation,
							vCamFocus,
							pInventoryView->fCamRotation,
							pInventoryView->fCamPitch,
							pInventoryView->fCamFOV,
							pInventoryView->fCamDistance,
							pInventoryView->pszEnvName );
					}
				}
			}

			pModel = pModel->m_pNext;
		}

		g_dwRenderState[ii] &= ~(UIRS_NEED_RENDER);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIGfxElementFree(
	UI_COMPONENT * component)
{
	ASSERT_RETURN(component);
	UI_GFXELEMENT * element = component->m_pGfxElementFirst;

	while (element)
	{
		UI_GFXELEMENT * next = element->m_pNextElement;
		ASSERT(element->m_pComponent == component);
		sUIGfxElementFreeData(element);
		FREE(g_StaticAllocator, element);
		element = next;
	}
	component->m_pGfxElementFirst = NULL;
	component->m_pGfxElementLast = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sElementGotoNextFrame(
	GAME* game,
	const CEVENT_DATA& data,
	DWORD)
{
	UI_COMPONENT *pComponent = (UI_COMPONENT *)data.m_Data1;

	ASSERT_RETURN(pComponent);

	//if (pComponent->m_dwFramesAnimTicket != INVALID_ID)
	//{
	//	CSchedulerCancelEvent(pComponent->m_dwFramesAnimTicket);
	//}
	pComponent->m_dwFramesAnimTicket = INVALID_ID;

	UI_GFXELEMENT* element = pComponent->m_pGfxElementFirst;
	while (element)
	{
		if (element->m_eGfxElement == GFXELEMENT_FRAME)
		{
			UI_ELEMENT_FRAME *pFrameElement = UIElemCastToFrame(element);

			if (pFrameElement->m_pFrame->m_pNextFrame)
			{
				pFrameElement->m_pFrame = pFrameElement->m_pFrame->m_pNextFrame;

				element->m_nVertexCount = 4 * pFrameElement->m_pFrame->m_nNumChunks;
				element->m_nIndexCount = 6 * pFrameElement->m_pFrame->m_nNumChunks;

				if (pFrameElement->m_pFrame->m_pNextFrame &&
					pComponent->m_dwFramesAnimTicket == INVALID_ID)
				{
					pComponent->m_dwFramesAnimTicket = CSchedulerRegisterEvent(AppCommonGetCurTime() + pFrameElement->m_pFrame->m_dwNextFrameDelay, sElementGotoNextFrame, CEVENT_DATA(DWORD_PTR(pComponent)));
				}

				UISetNeedToRender(element->m_pComponent->m_nRenderSection);
			}
		}
		element = element->m_pNextElement;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture /*= NULL*/,
	UI_TEXTURE_FRAME* frame /*= NULL*/,
	UI_POSITION position /*= UI_POSITION()*/,
	DWORD dwColor /*= GFXCOLOR_HOTPINK*/,
	const UI_RECT* cliprect /*= NULL*/,
	BOOL bDefinitelyNoStretch /*= FALSE*/,
	FLOAT fScaleX /*= -1.0f*/,
	FLOAT fScaleY /*= -1.0f*/,
	UI_SIZE *pOverrideSize /*= NULL*/)
{
	ASSERT_RETNULL(component);
	if (!texture)
	{
		texture = UIComponentGetTexture(component);
	}
	ASSERT_RETNULL(texture);
	if ( ! UITextureLoadFinalize( texture ) )
		return NULL;
	if (!frame)
	{
		frame = texture->m_pBoxFrame;
	}
	ASSERT_RETNULL(frame);

	UI_RECT recttouse(-UIDefaultWidth(), -UIDefaultHeight(), UIDefaultWidth(), UIDefaultHeight());
	if (cliprect)
	{
		recttouse = *cliprect;
	}
	
	if (fScaleX > 0.0f && fScaleY < 0.0f)
		fScaleY = fScaleX;
	if (fScaleX < 0.0f)
		fScaleX = 1.0f;
	if (fScaleY < 0.0f)
		fScaleY = 1.0f;

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_FRAME, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_FRAME *pFrameElement = UIElemCastToFrame(element);

	element->m_pTexture = texture;
	pFrameElement->m_pFrame = frame;
	if (pOverrideSize)
	{
		element->m_fWidth = pOverrideSize->m_fWidth * fScaleX;
		element->m_fHeight = pOverrideSize->m_fHeight * fScaleY;
	}
	else if (component->m_bStretch && !bDefinitelyNoStretch)
	{
		element->m_fWidth = component->m_fWidth * fScaleX;
		element->m_fHeight = component->m_fHeight * fScaleY;
	}
	else
	{
		element->m_fWidth = frame->m_fWidth * fScaleX;
		element->m_fHeight = frame->m_fHeight * fScaleY;
	}
	element->m_dwColor = (dwColor == GFXCOLOR_HOTPINK ? UI_COMBINE_ALPHA(UIComponentGetAlpha(component), frame->m_dwDefaultColor) : dwColor );
	element->m_bGrayOut = g_UI.m_bGrayout && component->m_bGrayInGhost;
	element->m_Position.m_fX = position.m_fX;
	element->m_Position.m_fY = position.m_fY;
	ASSERT_RETNULL(frame->m_fWidth > 0);
	ASSERT_RETNULL(frame->m_fHeight > 0);
	element->m_fXOffset = frame->m_fXOffset * (element->m_fWidth / frame->m_fWidth);
	element->m_fYOffset = frame->m_fYOffset * (element->m_fHeight / frame->m_fHeight);

	// this is now done in the render call, so a component doesn't necessarily have to be repainted when it scrolls
	//element->m_Position -= UIComponentGetScrollPos(component);	//now adjust the element for the component's scroll position
		
	element->m_fZDelta = component->m_fZDelta;

	element->m_nVertexCount = 4 * frame->m_nNumChunks;
	element->m_nIndexCount = 6 * frame->m_nNumChunks;
	UIGFXElementSetClipRect(element, &recttouse);	

	ASSERT_GOTO(pFrameElement, skip);
	ASSERT_GOTO(pFrameElement->m_pFrame, skip);
	if (pFrameElement->m_pFrame->m_pNextFrame)
	{
		ASSERT_GOTO(pFrameElement->m_pComponent, skip);
		if (pFrameElement->m_pComponent->m_dwFramesAnimTicket == INVALID_ID)
		{
			pFrameElement->m_pComponent->m_dwFramesAnimTicket = 
				CSchedulerRegisterEvent(AppCommonGetCurTime() + pFrameElement->m_pFrame->m_dwNextFrameDelay, sElementGotoNextFrame, CEVENT_DATA(DWORD_PTR(pFrameElement->m_pComponent)));
		}
	}

skip:
	UISetNeedToRender(component->m_nRenderSection);
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddRotatedElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture /*= NULL*/,
	UI_TEXTURE_FRAME* frame /*= NULL*/,
	UI_POSITION position /*= UI_POSITION()*/,
	DWORD dwColor /*= GFXCOLOR_HOTPINK*/,
	float fAngle /*= 0.0f*/,
	UI_POSITION RotationCenter /*= UI_POSITION()*/,
	UI_RECT* cliprect /*= NULL*/,
	BOOL bDefinitelyNoStretch /*= FALSE*/,
	FLOAT fScale /*= 1.0f*/,
	DWORD dwRotationFlags /*= 0*/)
{

	if (TESTBIT( dwRotationFlags, ROTATION_FLAG_DROPSHADOW_BIT ) == TRUE)
	{
	
		// offset shadow position
		UI_POSITION posShadow = position;
		posShadow.m_fX += 1;
		posShadow.m_fY += 1;
		
		// offset shadow rotation position
		UI_POSITION posShadowRotationCenter = RotationCenter;
		posShadowRotationCenter.m_fX += 1;
		posShadowRotationCenter.m_fY += 1;
		
		// don't draw a dropshadow of the dropshadow
		DWORD dwNewRotationFlags = dwRotationFlags;
		CLEARBIT( dwNewRotationFlags, ROTATION_FLAG_DROPSHADOW_BIT );

		// add shadow		
		UIComponentAddRotatedElement(
			component,
			texture,
			frame,
			posShadow,
			GFXCOLOR_BLACK,
			fAngle,
			posShadowRotationCenter,
			cliprect,
			bDefinitelyNoStretch,
			fScale,
			dwNewRotationFlags);
			
	}

	ASSERT_RETNULL(component);
	if (!texture)
	{
		texture = UIComponentGetTexture(component);
	}
	ASSERT_RETNULL(texture);
	if (!frame)
	{
		frame = texture->m_pBoxFrame;
	}
	ASSERT_RETNULL(frame);

	UI_RECT recttouse(-UIDefaultWidth(), -UIDefaultHeight(), UIDefaultWidth(), UIDefaultHeight());
	if (cliprect)
	{
		recttouse = *cliprect;
	}

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_ROTFRAME, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_ROTFRAME *pRotFrameElement = UIElemCastToRotFrame(element);

	pRotFrameElement->m_fAngle = fAngle;
	pRotFrameElement->m_RotationCenter = RotationCenter;
	
	if (TESTBIT( dwRotationFlags, ROTATION_FLAG_ABOUT_SELF_BIT ))
	{
		SETBIT( pRotFrameElement->m_dwElementFrameFlags, EFF_ROTATE_ABOUT_SELF_BIT );
	}

	element->m_pTexture = texture;
	pRotFrameElement->m_pFrame = frame;
	if (component->m_bStretch && !bDefinitelyNoStretch)
	{
		element->m_fWidth = component->m_fWidth * fScale;
		element->m_fHeight = component->m_fHeight * fScale;
	}
	else
	{
		element->m_fWidth = frame->m_fWidth * fScale;
		element->m_fHeight = frame->m_fHeight * fScale;
	}
	element->m_dwColor = (dwColor == GFXCOLOR_HOTPINK ? frame->m_dwDefaultColor : dwColor );
	element->m_bGrayOut = g_UI.m_bGrayout && component->m_bGrayInGhost;
	element->m_Position.m_fX = position.m_fX;
	element->m_Position.m_fY = position.m_fY;
	element->m_fXOffset = frame->m_fXOffset;
	element->m_fYOffset = frame->m_fYOffset;

	element->m_fXOffset *= fScale;
	element->m_fYOffset *= fScale;
	element->m_fZDelta = component->m_fZDelta;

	element->m_nVertexCount = 4 * frame->m_nNumChunks;
	element->m_nIndexCount = 6 * frame->m_nNumChunks;
	UIGFXElementSetClipRect(element, &recttouse);

	UISetNeedToRender(component->m_nRenderSection);
		
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddBoxElement(
	UI_COMPONENT* component,
	float fX1,
	float fY1,
	float fX2,
	float fY2,
	UI_TEXTURE_FRAME* frame /*= NULL*/,
	DWORD dwColor /*= GFXCOLOR_WHITE*/,
	BYTE bAlpha /*= 255*/,
	UI_RECT* cliprect /*= NULL*/)
{
	return UIComponentAddPrimitiveElement(
		component, UIPRIM_BOX, 0, UI_RECT(fX1, fY1, fX2, fY2), 
		NULL, cliprect, UI_COMBINE_ALPHA(bAlpha, dwColor));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddTriElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UI_TEXTURE_FRAME* frame,
	UI_TEXTURETRI* tri,
	DWORD color,
	UI_RECT* cliprect)
{
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(texture);
	ASSERT_RETNULL(frame);

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_TRI, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_TRI *pTriElement = UIElemCastToTri(element);

	element->m_pTexture = texture;
	pTriElement->m_pFrame = frame;
	element->m_dwColor = color;
	element->m_bGrayOut = g_UI.m_bGrayout && component->m_bGrayInGhost;
	pTriElement->m_Triangle = *tri;
	element->m_nVertexCount = 3;
	element->m_nIndexCount = 3;
	element->m_fZDelta = component->m_fZDelta;
	UIGFXElementSetClipRect(element, cliprect);

	UISetNeedToRender(component->m_nRenderSection);
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddRectElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UI_TEXTURE_FRAME* frame,
	UI_TEXTURERECT* rect,
	DWORD color /*= GFXCOLOR_WHITE*/,
	UI_RECT* cliprect /*= NULL*/)
{
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(texture);
	ASSERT_RETNULL(frame);

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_RECT, component);
	UI_ELEMENT_RECT *pRectElement = UIElemCastToRect(element);

	element->m_pTexture = texture;
	pRectElement->m_pFrame = frame;
	element->m_dwColor = color;
	element->m_bGrayOut = g_UI.m_bGrayout && component->m_bGrayInGhost;
	pRectElement->m_Rect = *rect;
	element->m_nVertexCount = 4;
	element->m_nIndexCount = 6;
	element->m_fZDelta = component->m_fZDelta;
	UIGFXElementSetClipRect(element, cliprect);

	UISetNeedToRender(component->m_nRenderSection);
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIComponentAddFrame(
	UI_COMPONENT* component)
{
	ASSERT_RETURN(component);
	if (!component->m_pFrame)
		return;

	UI_RECT componentrect = UIComponentGetRect(component);

	UI_POSITION pos;
	UIX_TEXTURE * pTexture = UIComponentGetTexture(component);
	UIComponentAddElement(component, pTexture, component->m_pFrame, pos, component->m_dwColor, component->m_bTileFrame ? &componentrect : NULL);

	if (component->m_bTileFrame)
	{
		pos.m_fX += component->m_pFrame->m_fWidth + component->m_fTilePaddingX;
		while( pos.m_fY < component->m_fHeight + component->m_fScrollVertMax)
		{
			while( pos.m_fX < component->m_fWidth + component->m_fScrollHorizMax)
			{
				UIComponentAddElement(component, pTexture, component->m_pFrame, pos, component->m_dwColor, &componentrect);
				ASSERTV_BREAK( component->m_pFrame->m_fWidth > 0.f || component->m_fTilePaddingX > 0.f, "Infinite loading loop on tile frame component, width and paddingX are both 0!\n%s", component->m_szName );
				pos.m_fX += component->m_pFrame->m_fWidth + component->m_fTilePaddingX;
			}
			ASSERTV_BREAK( component->m_pFrame->m_fHeight > 0.f || component->m_fTilePaddingY > 0.f, "Infinite loading loop on tile frame component, height and paddingY are both 0!\n%s", component->m_szName );
			pos.m_fY += component->m_pFrame->m_fHeight + component->m_fTilePaddingY;
			pos.m_fX = 0.0f;
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sUIComponentAddTextElementBase(
	UI_ELEMENT_TEXT* pTextElement,
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	UI_POSITION position,
	DWORD color /*= GFXCOLOR_WHITE*/,
	const UI_RECT* cliprect /*= NULL*/,
	int alignment /*= UIALIGN_TOPLEFT*/,
	UI_SIZE* overridesize /*= NULL*/,
	BOOL bDropShadow /*= TRUE*/,
	int nPage /*= 0*/,
	float fScale /*= 1.0f*/)
{
	//element->m_pTexture = texture;
	int nModifiedFontSize =  (int)( ( (float)nFontSize * g_UI.m_fUIScaler )  + scfFontSizeScaleUp );
	pTextElement->m_pTexture = UIGetFontTexture(LANGUAGE_CURRENT);
	pTextElement->m_pFont = font;
	ASSERT(nFontSize > 0);
	pTextElement->m_nFontSize = nFontSize;
	pTextElement->m_fDrawScale= fScale;
	pTextElement->m_dwFlags = 0;
	pTextElement->m_dwColor = color;
	pTextElement->m_bGrayOut = g_UI.m_bGrayout && component->m_bGrayInGhost;
	pTextElement->m_Position.m_fX = position.m_fX;
	pTextElement->m_Position.m_fY = position.m_fY;				

	// this is now done in the render call, so a component doesn't necessarily have to be repainted when it scrolls
	//pTextElement->m_Position -= UIComponentGetScrollPos(component);	//now adjust the element for the component's scroll position

	pTextElement->m_fWidth = overridesize ? overridesize->m_fWidth : component->m_fWidth;
	pTextElement->m_fHeight = overridesize ? overridesize->m_fHeight : component->m_fHeight;
	UIGFXElementSetClipRect(pTextElement, cliprect);	

	int printable = 0;
	UICountPrintableChar(pTextElement->m_pText, &printable);

	pTextElement->m_nPage = nPage;

	pTextElement->m_fZDelta = component->m_fZDelta;
	pTextElement->m_nAlignment = alignment;
	pTextElement->m_nVertexCount = 4 * printable;
	pTextElement->m_nIndexCount = 6 * printable;
	if (bDropShadow && bDropShadowsEnabled)
	{
		SETBIT( pTextElement->m_dwFlags, UETF_DROP_SHADOW_BIT );
		
		// make storage space for the shadow text
		pTextElement->m_nVertexCount *= 2;
		pTextElement->m_nIndexCount *= 2;		
	}
	
	UISetNeedToRender(component->m_nRenderSection);

	const WCHAR *text = pTextElement->m_pText;
	// now we need to see if there are any tags in the text that need to be converted to graphics elements
	WCHAR *szTag = PStrStrI(text, TEXT_TAG_BEGIN_STRING);
	if (szTag && UIDecodeTextTag(szTag) != TEXT_TAG_INVALID)
	{
		// scale the font size the way the write function will
		nFontSize = (int)(((float)nModifiedFontSize * UIGetLogToScreenRatioY(component->m_bNoScaleFont) ) + scfFontSizeScaleUp);

		V( e_UITextureFontAddCharactersToTexture(font, text, nFontSize) );

		float x = position.m_fX;
		float y = position.m_fY;
		float fWidthRatio = pTextElement->m_fDrawScale;
		float fLineHeight = (float)nModifiedFontSize * pTextElement->m_fDrawScale;
		float fTabWidth = font->m_fTabWidth * fWidthRatio;
		float fColumnX = UIElementGetTextColumnPoint(font, nModifiedFontSize, fWidthRatio, component->m_bNoScaleFont, text);
		float fThisLineHeight = fLineHeight;
		int nCurrentPage = 0;
		float fAdjScreenX = 0.0f;
		float fAdjScreenY = 0.0f;
		UIGetTextAlignmentScreenspaceAdj(pTextElement, text, fWidthRatio, pTextElement->m_nAlignment, &fAdjScreenX, &fAdjScreenY);


		WCHAR wc;
		while ((wc = *text++) != 0)
		{
			if (wc == CTRLCHAR_COLOR)
			{
				if (*text == 0)
				{
					break;
				}
				text++;
				continue;
			}
			if (wc == CTRLCHAR_LINECOLOR)
			{
				if (*text == 0)
				{
					break;
				}
				text++;
				continue;
			}
			if (wc == CTRLCHAR_HYPERTEXT || wc == CTRLCHAR_HYPERTEXT_END)
			{
				text += (wc == CTRLCHAR_HYPERTEXT)? 1 : 0;
				continue;
			}
			
			if (wc == TEXT_TAG_BEGIN_CHAR)
			{
				int nTextTag = UIDecodeTextTag(text);
				if (nTextTag == TEXT_TAG_PGBRK ||
					nTextTag == TEXT_TAG_INTERRUPT ||
					nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
				{
					nCurrentPage++;
				}
				if (nTextTag != TEXT_TAG_INVALID)
				{
					// ok, this is a tag enclosed in brackets, 
					//  we may need to add another graphic element for it here
					int nNumChars = 0;
					UI_SIZE sizeTag;
					BOOL pbForDrawOnly = FALSE;
					if (UIProcessStringTag(component, text, x , y , &sizeTag, &nNumChars, &pbForDrawOnly))
					{
						if (!pbForDrawOnly)
						{
							x += sizeTag.m_fWidth;
							fThisLineHeight = MAX(fThisLineHeight, sizeTag.m_fHeight);
						}
						text += nNumChars;
						continue;
					}
					else
					{
						// ok, must have been some malformed tag.  If it has a length, skip it
						if (nNumChars > 0)
						{
							text += nNumChars;
							continue;
						}
					}
				}
			}

			if (nPage != -1 && nPage != nCurrentPage)
			{
				continue;
			}

			if (wc == L'\n' || wc == L'\r')
			{
				x = position.m_fX;
				y += (fThisLineHeight + 1.0f * UIGetLogToScreenRatioY(component->m_bNoScaleFont));
				fThisLineHeight = fLineHeight;
				continue;
			}
			if (wc == L'\t')
			{
				x = float(int( x / fTabWidth + 0.5f) + 1 ) * fTabWidth;
				continue;
			}
			if (wc == L'\f')
			{
				x = position.m_fX + fColumnX;
				continue;
			}

			if (PStrIsPrintable( wc ) == FALSE)		
			{
				continue;
			}

			wc = sUIJabberChar(font, wc);
			wc = sUICheckGlyphset(font, wc);

			//float fCharWidth = sUIGetCharWidth(font, fWidthRatio, wc);
			UI_TEXTURE_FONT_FRAME* chrframe = font->GetChar(wc, nFontSize);
			ASSERT_RETURN(chrframe);
			float fCharWidth = chrframe->m_fWidth * fWidthRatio;
			float fLeadspace = chrframe->m_fPreWidth * fWidthRatio;
			float fFollowspace = chrframe->m_fPostWidth * fWidthRatio;

			x += (fLeadspace + fCharWidth + fFollowspace + font->m_nExtraSpacing);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_ELEMENT_TEXT* UIComponentAddTextElement(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	const WCHAR* text,
	UI_POSITION position,
	DWORD color /*= GFXCOLOR_WHITE*/,
	const UI_RECT* cliprect /*= NULL*/,
	int alignment /*= UIALIGN_TOPLEFT*/,
	UI_SIZE* overridesize /*= NULL*/,
	BOOL bDropShadow /*= TRUE*/,
	int nPage /*= 0*/,
	float fScale /*= 1.0f*/)
{
	if (!text)
	{
		return NULL;
	}
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(font);

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_TEXT, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_TEXT *pTextElement = UIElemCastToText(element);

	int printable = 0;
	int len = UICountPrintableChar(text, &printable);

	pTextElement->m_pText = (WCHAR*)MALLOC(NULL, len * sizeof(WCHAR));
	PStrCopy(pTextElement->m_pText, text, len);

	sUIComponentAddTextElementBase(pTextElement, component, texture, font, nFontSize, position, color, cliprect, alignment, overridesize, bDropShadow, nPage, fScale);

	return pTextElement;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// doesn't make a copy of the text to a new MALLOCed buffer (don't use when
//  passing in a stack pointer as the element will try to free it later.
UI_ELEMENT_TEXT* UIComponentAddTextElementNoCopy(
	UI_COMPONENT* component,
	UIX_TEXTURE* texture,
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	WCHAR* text,
	UI_POSITION position,
	DWORD color /*= GFXCOLOR_WHITE*/,
	const UI_RECT* cliprect /*= NULL*/,
	int alignment /*= UIALIGN_TOPLEFT*/,
	UI_SIZE* overridesize /*= NULL*/,
	BOOL bDropShadow /*= TRUE*/,
	int nPage /*= 0*/,
	float fScale /*= 1.0f*/)
{

	if (!text)
	{
		return NULL;
	}
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(font);

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_TEXT, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_TEXT *pTextElement = UIElemCastToText(element);

	pTextElement->m_pText = text;

	sUIComponentAddTextElementBase(pTextElement, component, texture, font, nFontSize, position, color, cliprect, alignment, overridesize, bDropShadow, nPage, fScale);

	return pTextElement;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextElementSetEarlyTerminator(
	UI_GFXELEMENT *pGFXElement,
	int nNumChars)
{
	ASSERTX_RETURN( pGFXElement, "Expected gfx element" );
	UI_ELEMENT_TEXT *pTextElement = UIElemCastToText( pGFXElement );	
	ASSERTX_RETURN( pTextElement, "Expected text element" );
	pTextElement->m_nMaxCharactersToDisplay = nNumChars;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UITextElementGetNumCharsOnPage(
	UI_GFXELEMENT *pGFXElement)
{
	ASSERTX_RETZERO( pGFXElement, "Expected gfx element" );
	UI_ELEMENT_TEXT *pTextElement = UIElemCastToText( pGFXElement );	
	ASSERTX_RETZERO( pTextElement, "Expected text element" );
	return pTextElement->m_nNumCharsOnPage;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddModelElement(
	UI_COMPONENT* component,
	int nAppearanceDefIDForCamera,
	int nModelID,
	UI_RECT rect,
	float fRotation,
	int nModelCam /*= 0*/,
	UI_RECT * pClipRect /*= NULL*/,
	UI_INV_ITEM_GFX_ADJUSTMENT* pAdjust /*= NULL*/)
{
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(nAppearanceDefIDForCamera != INVALID_ID);
	ASSERT_RETNULL(nModelID != INVALID_ID);

	UI_POSITION pos(rect.m_fX1, rect.m_fY1);
	float fWidth = rect.m_fX2 - rect.m_fX1;
	float fHeight = rect.m_fY2 - rect.m_fY1;
	if (pAdjust)
	{
		if (pAdjust->fScale != 1.0f)
		{
			pos.m_fX -= (fWidth / 2.0f) * (pAdjust->fScale - 1.0f);
			pos.m_fY -= (fHeight / 2.0f) * (pAdjust->fScale - 1.0f); 
			fWidth  *= pAdjust->fScale;
			fHeight *= pAdjust->fScale;
		}

		pos.m_fX += pAdjust->fXOffset;
		pos.m_fY += pAdjust->fYOffset;
	}

	pos -= UIComponentGetScrollPos(component);

	if (fWidth <= 0.0f || fHeight <= 0.0f)
		return NULL;

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_MODEL, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_MODEL *pModelElement = UIElemCastToModel(element);
	element->m_dwColor = GFXCOLOR_WHITE;
	element->m_Position = pos;
	element->m_fWidth = fWidth;
	element->m_fHeight = fHeight;
	UIGFXElementSetClipRect(element, pClipRect);
	element->m_fZDelta = 0.0f;	// models don't offset z

	pModelElement->m_nAppearanceDefIDForCamera = nAppearanceDefIDForCamera;
	pModelElement->m_nModelID = nModelID;
	pModelElement->m_fRotation = fRotation;
	pModelElement->m_nModelCam = nModelCam;

	UISetNeedToRender(component->m_nRenderSection);
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_GFXELEMENT* UIComponentAddItemGFXElement(
	UI_COMPONENT* component,
	UNIT *pItem,
	UI_RECT rect,
	UI_RECT * pClipRect /*= NULL*/,
	struct UI_INV_ITEM_GFX_ADJUSTMENT* pAdjust /*= NULL*/)
{
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(pItem);
#if 0
    return UIComponentAddModelElement(component, 
		UnitGetAppearanceDefIdForCamera( pItem ), 
		c_UnitGetModelIdInventory( pItem ), 
	    rect, 
		0.0f, 
		0, 
		pClipRect, 
		pAdjust);
#else
	UI_POSITION pos(rect.m_fX1, rect.m_fY1);
	float fWidth = rect.m_fX2 - rect.m_fX1;
	float fHeight = rect.m_fY2 - rect.m_fY1;
	if (pAdjust)
	{
		if (pAdjust->fScale != 1.0f)
		{
			pos.m_fX -= (fWidth / 2.0f) * (pAdjust->fScale - 1.0f);
			pos.m_fY -= (fHeight / 2.0f) * (pAdjust->fScale - 1.0f); 
			fWidth  *= pAdjust->fScale;
			fHeight *= pAdjust->fScale;
		}

		pos.m_fX += pAdjust->fXOffset;
		pos.m_fY += pAdjust->fYOffset;
	}

//	pos -= UIComponentGetScrollPos(component);

	if (fWidth <= 0.0f || fHeight <= 0.0f)
		return NULL;

	int nEngineTextureID = UnitDataGetIconTextureID(UnitGetData(pItem));
	return UIComponentAddFullTextureElement(component, nEngineTextureID, pos, fWidth, fHeight, 0.0f, pClipRect);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void UIGfxElementGetCallbackFunc(
	int eComponentType,
	int nMaxLen,
	PFN_UI_RENDER_CALLBACK & pfnCallback )
{
	// here is where we could put game-level UI callbacks

	if ( ! pfnCallback )
	{
		// ... nothing found, so ask the engine

		// THIS IS LAME!!  How do I detect the automap?  Honestly.
		//if ( eComponentType == UICOMP_AUTOMAP && eComponentType == UICOMP_AUTOMAP_BIG )
		{
			V( e_UIElementGetCallbackFunc( "automap", 8, pfnCallback ) );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_GFXELEMENT* UIComponentAddCallbackElement(
	UI_COMPONENT* component, UI_POSITION & pos, float fWidth, float fHeight, float fZDelta, float fScale, UI_RECT * pClipRect )
{
	ASSERT_RETNULL(component);


	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_CALLBACK, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_CALLBACK *pCallbackElement = UIElemCastToCallback(element);
	element->m_dwColor = GFXCOLOR_WHITE;
	element->m_Position = pos;
	element->m_fWidth = fWidth;
	element->m_fHeight = fHeight;
	UIGFXElementSetClipRect(element, pClipRect);
	element->m_fZDelta = fZDelta;
	pCallbackElement->m_fScale = fScale;

	// what type of callback is it?
	UIGfxElementGetCallbackFunc( element->m_pComponent->m_eComponentType, MAX_UI_COMPONENT_NAME_LENGTH, pCallbackElement->m_pfnRender );
	if ( ! pCallbackElement->m_pfnRender )
	{
		// nothing to do!
		return element;
	}

	UISetNeedToRender(component->m_nRenderSection);
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

UI_GFXELEMENT* UIComponentAddFullTextureElement(
	UI_COMPONENT* component, 
	int nEngineTexureID, 
	UI_POSITION & pos, 
	float fWidth, 
	float fHeight, 
	float fZDelta /*= 0.0f*/, 
	UI_RECT * pClipRect /*= NULL*/ )
{
	ASSERT_RETNULL(component);
	ASSERT_RETNULL(nEngineTexureID != INVALID_ID);

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_FULLTEXTURE, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_FULLTEXTURE *pTextureElement = UIElemCastToFullTexture(element);
	element->m_dwColor = GFXCOLOR_WHITE;
	element->m_Position = pos;
	element->m_fWidth = fWidth;
	element->m_fHeight = fHeight;
	element->m_fZDelta = component->m_fZDelta;

	element->m_nVertexCount = 4;
	element->m_nIndexCount = 6;
	UIGFXElementSetClipRect(element, pClipRect);
	element->m_fZDelta = fZDelta;
	pTextureElement->m_nEngineTextureID = nEngineTexureID;

	UISetNeedToRender(component->m_nRenderSection);
	return element;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_ELEMENT_PRIMITIVE* UIComponentAddPrimitiveElement(
	UI_COMPONENT* component,
	UI_PRIMITIVE_TYPE eType,
	DWORD dwFlags,
	UI_RECT tRect,
	UI_ARCSECTOR_DATA * ptArcData /*= NULL*/,
	UI_RECT * pClipRect /*= NULL*/,
	DWORD dwColor /*= GFXCOLOR_WHITE*/)
{
	ASSERT_RETNULL(component);

	UI_POSITION pos(tRect.m_fX1, tRect.m_fY1);
	float fWidth  = tRect.m_fX2 - tRect.m_fX1;
	float fHeight = tRect.m_fY2 - tRect.m_fY1;
	//if (pAdjust)
	//{
	//	if (pAdjust->fScale != 1.0f)
	//	{
	//		pos.m_fX -= (fWidth / 2.0f) * (pAdjust->fScale - 1.0f);
	//		pos.m_fY -= (fHeight / 2.0f) * (pAdjust->fScale - 1.0f); 
	//		fWidth  *= pAdjust->fScale;
	//		fHeight *= pAdjust->fScale;
	//	}

	//	pos.m_fX += pAdjust->fXOffset;
	//	pos.m_fY += pAdjust->fYOffset;
	//}

	if (fWidth <= 0.0f || fHeight <= 0.0f)
		return NULL;

	UI_GFXELEMENT* element = UIGetNewGfxElement(GFXELEMENT_PRIM, component);
	ASSERT_RETNULL(element);
	UI_ELEMENT_PRIMITIVE *pPrimElement = UIElemCastToPrim(element);
	element->m_dwColor = dwColor;
	element->m_Position = pos;
	element->m_fWidth = fWidth;
	element->m_fHeight = fHeight;
	element->m_fZDelta = component->m_fZDelta;

	UI_RECT recttouse(-UIDefaultWidth(), -UIDefaultHeight(), UIDefaultWidth(), UIDefaultHeight());
	if ( pClipRect )
	{
		recttouse = *pClipRect;
	}
	element->m_ClipRect = recttouse;

	pPrimElement->m_nRenderSection = component->m_nRenderSection;
	pPrimElement->m_eType		= eType;
	pPrimElement->m_dwFlags		= dwFlags;
	pPrimElement->m_tRect		= tRect;
	switch( pPrimElement->m_eType )
	{
	case UIPRIM_BOX:
		element->m_nVertexCount = 4;
		element->m_nIndexCount  = 6;
		break;
	case UIPRIM_ARCSECTOR:
		ptArcData->fAngleWidthDeg = max( ptArcData->fAngleWidthDeg, 0.f );
		ptArcData->fAngleWidthDeg = min( ptArcData->fAngleWidthDeg, 360.f );
		e_UIPrimitiveArcSectorGetCounts( ptArcData->fSegmentWidthDeg, &element->m_nVertexCount, &element->m_nIndexCount );
		ASSERT_RETNULL( ptArcData );
		pPrimElement->m_tArcData = *ptArcData;
		break;
	}

	UISetNeedToRender(component->m_nRenderSection);
	return pPrimElement;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UIElementGetModelElementUnitID(
	UI_GFXELEMENT* pElement)
{
	if (pElement && pElement->m_eGfxElement == GFXELEMENT_MODEL)
	{
		UI_ELEMENT_MODEL *pModelElement = (UI_ELEMENT_MODEL *)pElement;
		return pModelElement->m_nUnitID;
	}

	return INVALID_ID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIDebugGfxCountTextureDraws()
{
	int nCount = 0;
	UI_TEXTUREDRAW*	pTextureDraw = g_GraphicGlobals.m_pTextureToDrawFirst;
	while (pTextureDraw)
	{
		nCount++;
		pTextureDraw = pTextureDraw->m_pNext;
	}

	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int UIDebugGfxCountTextureDrawsGarbage()
{
	int nCount = 0;
	UI_TEXTUREDRAW*	pTextureDraw = g_GraphicGlobals.m_pTextureToDrawGarbage;
	while (pTextureDraw)
	{
		nCount++;
		pTextureDraw = pTextureDraw->m_pNext;
	}

	return nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_SIZE DoWordWrap(
	WCHAR* text, 
	float fMaxWidth, 
	UIX_TEXTURE_FONT *font, 
	BOOL bNoScale,
	int nFontSize,
	int nPage /*= 0*/,
	float fWidthRatio /*= 1.0f*/)
{
	UI_SIZE returnSize( 0.0f, 0.0f );

	if (!font)
	{
		return returnSize;
	}

	if (!text || !text[0])
	{
		return returnSize;
	}

	if (fMaxWidth <= 0.0f)
	{
		return returnSize;
	}

	fMaxWidth *= fWidthRatio;

	float fLineHeight = 0.0f;
	UIElementGetTextLogSize(font, nFontSize, 1.0f, bNoScale, L"M", NULL, &fLineHeight);

	float width = 0.0f;
	float fWidest = 0.0f;
	float fLastWidest = 0.0f;
	float fThisLineHeight = fLineHeight;

	float height = fLineHeight;

	float fLastSpacePos = 0.0f;
	WCHAR *pLastSpace = NULL;

	float fTabWidth = font->m_fTabWidth * fWidthRatio;
	int nCurrentPage = 0;
	int nModifiedFontSize =  (int)( ( (float)nFontSize * g_UI.m_fUIScaler ) + scfFontSizeScaleUp);

	WCHAR wc;
	while ((wc = *text) != 0)
	{
		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR)
		{
			if (*(text + 1) == 0)
			{
				return returnSize;
			}
			text +=2;
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT || wc == CTRLCHAR_HYPERTEXT_END)
		{
			text += (wc == CTRLCHAR_HYPERTEXT)? 2 : 1;
			continue;
		}

		if (wc == TEXT_TAG_BEGIN_CHAR)
		{
			int nTextTag = UIDecodeTextTag(text);
			if (nTextTag == TEXT_TAG_PGBRK ||
				nTextTag == TEXT_TAG_INTERRUPT ||
				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
			{
				nCurrentPage++;
			}
			if (nTextTag != TEXT_TAG_INVALID)
			{
				// ok, this is a tag enclosed in brackets, so... make space for it for now
				int nNumChars = 0;
				UI_SIZE sizeTag;
				BOOL bForDrawOnly = FALSE;
				if (UIGetStringTagSize((text+1), NULL, &sizeTag, &nNumChars, &bForDrawOnly))
				{
					if (!bForDrawOnly)
					{
						width += sizeTag.m_fWidth;
						fThisLineHeight = MAX(fThisLineHeight, sizeTag.m_fHeight);
					}
					text += nNumChars;
					continue;
				}
			}
			else
			{
				// ok, must have been some malformed tag.  Just fall through and print it
			}
		}

		if (nPage != -1 && nPage != nCurrentPage)
		{
			text++;
			continue;
		}

		if (wc == L'\n')
		{
			fWidest = MAX(width, fWidest);
			width = 0.0f;
			height += fThisLineHeight + 1.0f * UIGetLogToScreenRatioY(bNoScale);
			fThisLineHeight = fLineHeight;
			pLastSpace = NULL;
			text++;

			continue;
		}
		if (wc == L'\t')
		{
			fLastWidest = width;
			width = float(int( width / fTabWidth + 0.5f) + 1 ) * fTabWidth;
			pLastSpace = text;
			text++;
			continue;
		}
		if (wc == L' ')
		{
			fLastSpacePos = width;
			fLastWidest = width;
			pLastSpace = text;
		}
		
		if (PStrIsPrintable( wc ) == FALSE)
		{
			text++;
			continue;
		}

		width += UIGetCharWidth(font, nModifiedFontSize, fWidthRatio, bNoScale, wc);

		// Here's where we do the word wrap
		if (width > fMaxWidth &&
			pLastSpace != NULL)
		{
			fWidest = MAX(fLastWidest, fWidest);

			//change the last space into a CRLF
			*pLastSpace = L'\n';

			// start counting again from the wrapped word
			text = pLastSpace;
			pLastSpace = NULL;

			width = 0.0f;
			height += fLineHeight + 1.0f * UIGetLogToScreenRatioY(bNoScale);
		}

		text++;
	}

	returnSize.m_fWidth = MAX(width, fWidest);
	returnSize.m_fHeight = height * fWidthRatio;
	
	return returnSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This function is slower and less fun, but it can insert line breaks

UI_SIZE DoWordWrapEx(			// CAUTION only call this with a heap pointer for 'szTextIn'.  It may be reallocated.
	WCHAR* &szTextIn,
	UI_LINE * pLine,
	float fMaxWidth, 
	UIX_TEXTURE_FONT *font, 
	BOOL bNoScale,
	int nFontSize,
	int nPage /*= 0*/,
	float fWidthRatio /*= 1.0f*/,
	UI_TXT_BKGD_RECTS * pRectStruct /*= NULL*/,
	BOOL bForceWrap /*= FALSE*/ )
{
	UI_SIZE returnSize( 0.0f, 0.0f );

//	ASSERT_RETVAL(_CrtIsValidHeapPointer((void *)szTextIn), returnSize);
	if (pRectStruct)
		pRectStruct->nNumRects = 0;

	if (!font)
	{
		return returnSize;
	}

	if (pLine)
	{
		szTextIn = pLine->EditText();
	}

	if (!szTextIn || !szTextIn[0])
	{
		return returnSize;
	}

	if (fMaxWidth <= 0.0f)
	{
		return returnSize;
	}

	BOOL bFound = FALSE;
	const char *szKey = GlobalStringGetKey( GS_WORDWRAP_REPLACE_CHARS );
	const WCHAR *szWordWrapReplaceChars = StringTableGetStringByKeyVerbose(szKey, 0, 0, NULL, &bFound);
	if (!bFound)
		szWordWrapReplaceChars = L" ";
	szKey = GlobalStringGetKey( GS_WORDWRAP_AFTER_CHARS );
	const WCHAR *szWordWrapAfterChars = StringTableGetStringByKeyVerbose(szKey, 0, 0, NULL, &bFound);
	if (!bFound)
		szWordWrapAfterChars = L"";
	szKey = GlobalStringGetKey( GS_WORDWRAP_BEFORE_CHARS );
	const WCHAR *szWordWrapBeforeChars = StringTableGetStringByKeyVerbose(szKey, 0, 0, NULL, &bFound);
	if (!bFound)
		szWordWrapBeforeChars = L"";

	szKey = GlobalStringGetKey( GS_WORDWRAP_ALL_EXCEPT_BEFORE_CHARS );
	const WCHAR *szWordWrapAllExceptBeforeChars = StringTableGetStringByKeyVerbose(szKey, 0, 0, NULL, &bFound);
	if (!bFound)
		szWordWrapAllExceptBeforeChars = L"";
	
	szKey = GlobalStringGetKey( GS_WORDWRAP_ALL_EXCEPT_AFTER_CHARS );
	const WCHAR *szWordWrapAllExceptAfterChars = StringTableGetStringByKeyVerbose(szKey, 0, 0, NULL, &bFound);
	if (!bFound)
		szWordWrapAllExceptAfterChars = L"";

	szKey = GlobalStringGetKey( GS_WORDWRAP_DO_NOT_DIVIDE_CHARS );
	const WCHAR *szWordWrapDoNotDivideChars = StringTableGetStringByKeyVerbose(szKey, 0, 0, NULL, &bFound);
	if (!bFound)
		szWordWrapDoNotDivideChars = L"";


	if (!szWordWrapAfterChars &&
		!szWordWrapBeforeChars &&
		!szWordWrapAllExceptBeforeChars &&
		!szWordWrapAllExceptAfterChars)
	{
		return DoWordWrap(szTextIn, fMaxWidth, font, bNoScale, nFontSize, nPage);
	}

	// first we need a new string to hold the wrapped text
	int nStrLen = PStrLen(szTextIn);
	WCHAR *szDestText = MALLOC_NEWARRAY(NULL, WCHAR, nStrLen * 2);	// make it big to start with
	memset(szDestText, 0, sizeof(WCHAR) * nStrLen * 2);
	WCHAR *pCurDestText = szDestText;

	fMaxWidth *= fWidthRatio;

	float fLogToScreenRatioY = UIGetLogToScreenRatioY(bNoScale);
	int nAdjFontSize = (int)(((float)nFontSize * fLogToScreenRatioY * g_UI.m_fUIScaler) + scfFontSizeScaleUp);
	float fLineHeight = (float)nAdjFontSize;
	float fSpaceBetweenLines = 1.0f * fLogToScreenRatioY;

	float width = 0.0f;
	float fWidest = 0.0f;
	float fLastWidest = 0.0f;
	float fThisLineHeight = fLineHeight;

	float height = fLineHeight;

	float fLastBreakPos = 0.0f;
	WCHAR *pLastBreakChar = NULL;
	int   nLastBreakStyle = 0;

	int nCurrentPage = 0;
	int nModifiedFontSize =  (int)( ( (float)nFontSize * g_UI.m_fUIScaler ) + scfFontSizeScaleUp);
	BOOL bHoldWrapInTag = FALSE;

	// KCK: This function was broken for any multi-line text boxes that had to both insert a character
	// to wrap AND replace a character to wrap. Fixed so that if we ever add a character then from
	// there out we'll use our scratch buffer rather than trying to modify the string inline.
	BOOL bUseScratchBuffer = FALSE;
	BOOL bLineBreakReplacementDone = FALSE;
	WCHAR *text = szTextIn;
	WCHAR *pSourceCopyStart = text;

	WCHAR wc;
	while ((wc = *text) != 0)
	{
		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR)
		{
			if (*(text + 1) == 0)
			{
				return returnSize;
			}
			text += 2;
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT || wc == CTRLCHAR_HYPERTEXT_END)
		{
			text += (wc == CTRLCHAR_HYPERTEXT)? 2 : 1;
			bHoldWrapInTag = (wc == CTRLCHAR_HYPERTEXT)? TRUE : FALSE;
			continue;
		}

		if (wc == TEXT_TAG_BEGIN_CHAR)
		{
			int nTextTag = UIDecodeTextTag(text);
			if (nTextTag == TEXT_TAG_PGBRK ||
				nTextTag == TEXT_TAG_INTERRUPT ||
				nTextTag == TEXT_TAG_INTERRUPT_VIDEO)
			{
				nCurrentPage++;
			}
			if (nTextTag != TEXT_TAG_INVALID)
			{
				// ok, this is a tag enclosed in brackets, so... make space for it for now
				int nNumChars = 0;
				UI_SIZE sizeTag;
				BOOL bForDrawOnly = FALSE;
				if (UIGetStringTagSize((text+1), NULL, &sizeTag, &nNumChars, &bForDrawOnly))
				{
					if (!bForDrawOnly)
					{
						width += sizeTag.m_fWidth;
						fThisLineHeight = MAX(fThisLineHeight, sizeTag.m_fHeight);
					}
					text += nNumChars;
					continue;
				}
			}
			else
			{
				// ok, must have been some malformed tag.  Just fall through and print it
			}
		}

		if (nPage != -1 && nPage != nCurrentPage)
		{
			text++;
			continue;
		}

		if (wc == L'\n' || wc == L'\r')
		{
			if (pRectStruct)
			{
				if (pRectStruct->nNumRects < pRectStruct->nBufferSize)
				{
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX1 = 0.0f - UI_TXT_BKGD_RECT_X_BUF;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX2 = width + UI_TXT_BKGD_RECT_X_BUF;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 = height - fThisLineHeight;
					pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 = height + fSpaceBetweenLines;
					pRectStruct->nNumRects++;
				}
			}

			fWidest = MAX(width, fWidest);
			width = 0.0f;

			height += fThisLineHeight + fSpaceBetweenLines;
			fThisLineHeight = fLineHeight;
			pLastBreakChar = NULL;
			text++;

			continue;
		}

		if (!bHoldWrapInTag && PStrChr(szWordWrapReplaceChars, wc) != NULL)
		{
			fLastBreakPos = width;
			fLastWidest = width;
			nLastBreakStyle = 0;
			pLastBreakChar = text;
		} 
		else if (!bHoldWrapInTag && PStrChr(szWordWrapBeforeChars, wc) != NULL)
		{
			fLastBreakPos = width;
			fLastWidest = width;
			nLastBreakStyle = 1;
			pLastBreakChar = text;
		} 
		
		if (PStrIsPrintable( wc ) == FALSE)
		{
			text++;
			continue;
		}

		const float oldWidth = width;

		width += UIGetCharWidth(font, nModifiedFontSize, fWidthRatio, bNoScale, wc);

		// Here's where we do the word wrap
		if (width > fMaxWidth &&
			(pLastBreakChar != NULL || bForceWrap))
		{
			if (bForceWrap && pLastBreakChar == NULL)
			{
				// there was no breakable character on this line, but we're going to force a wordwrap anyway
				fLastBreakPos = oldWidth;
				fLastWidest = oldWidth;
				nLastBreakStyle = 1;
				pLastBreakChar = text;
			}
			else
			{
				fWidest = MAX(fLastWidest, fWidest);

				//change the last space into a CRLF
				if (nLastBreakStyle == 0 && !bUseScratchBuffer)
				{
					*pLastBreakChar = L'\n';

					// start counting again from the wrapped word
					text = pLastBreakChar;
					text++;
					pLastBreakChar = NULL;
					bLineBreakReplacementDone = TRUE;
				}
				else if (nLastBreakStyle == 1 ||	// break before this character
					nLastBreakStyle == 2 ||	// break after this character
					bUseScratchBuffer)		
				{
					// KCK: Goofy, but necessary. If we've previous replaced a LB inline, we need to
					// copy everything up to this point. From here on out we're stuck with using our
					// scratch buffer.
					if (bLineBreakReplacementDone)
					{
						int nLenSoFar = (int)(pLastBreakChar - szTextIn);
						memcpy(pCurDestText, szTextIn, sizeof(WCHAR) * nLenSoFar);
						pCurDestText += nLenSoFar;
						pSourceCopyStart += nLenSoFar;
						bLineBreakReplacementDone = FALSE;
					}

					int nLen = (int)(pLastBreakChar - pSourceCopyStart);
					if (nLastBreakStyle == 2)
						nLen++;
					memcpy(pCurDestText, pSourceCopyStart, sizeof(WCHAR) * nLen);
					// add a line break
					pCurDestText += nLen;
					*pCurDestText = '\n';
					pCurDestText++;

					if (nLastBreakStyle == 2 || nLastBreakStyle == 0)
						pLastBreakChar++;

					pSourceCopyStart = pLastBreakChar;

					// start counting again from the wrapped word
					text = pLastBreakChar;

					pLastBreakChar = NULL;
					bUseScratchBuffer = TRUE;
				}

				if (pRectStruct)
				{
					if (pRectStruct->nNumRects < pRectStruct->nBufferSize)
					{
						pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX1 = 0.0f - UI_TXT_BKGD_RECT_X_BUF;
						pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX2 = fLastWidest + UI_TXT_BKGD_RECT_X_BUF;
						pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 = height - fThisLineHeight;
						pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 = height + fSpaceBetweenLines;
						pRectStruct->nNumRects++;
					}
				}
				width = 0.0f;
				height += fLineHeight + fSpaceBetweenLines;
			}
		}
		else
		{
			if (szWordWrapAllExceptBeforeChars[0] ||
				szWordWrapAllExceptAfterChars[0])
			{
				if ((!szWordWrapAllExceptBeforeChars[0] ||								// if there are no before exceptions
					!text[1] ||
					PStrChr(szWordWrapAllExceptBeforeChars, text[1]) == NULL )  &&		// or this isn't a before exception
					(!szWordWrapAllExceptAfterChars[0] ||								// if there are no after exceptions
					!text[0] ||
					PStrChr(szWordWrapAllExceptAfterChars, text[0]) == NULL ) && 		// or this isn't an after exception
					(!szWordWrapDoNotDivideChars[0] ||									// there are no rules against dividing characters
					!text[1] || !text[0] ||
					PStrChr(szWordWrapDoNotDivideChars, text[0]) == NULL ||
					PStrChr(szWordWrapDoNotDivideChars, text[1]) == NULL ) )			// if both of the character are in the list of characters
																						// that cannot be split
				{
					fLastBreakPos = width;
					fLastWidest = width;
					nLastBreakStyle = 2;
					pLastBreakChar = text;
				}
			}
			else if (PStrChr(szWordWrapAfterChars, wc) != NULL)
			{
				fLastBreakPos = width;
				fLastWidest = width;
				nLastBreakStyle = 2;
				pLastBreakChar = text;
			} 

			text++;
		}
	}

	if (pRectStruct)
	{
		if (pRectStruct->nNumRects < pRectStruct->nBufferSize)
		{
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX1 = 0.0f - UI_TXT_BKGD_RECT_X_BUF;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fX2 = width + UI_TXT_BKGD_RECT_X_BUF;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY1 = height - fThisLineHeight;
			pRectStruct->pRectBuffer[pRectStruct->nNumRects].m_fY2 = height + fSpaceBetweenLines;
			pRectStruct->nNumRects++;
		}
	}
	returnSize.m_fWidth = MAX(width, fWidest);
	returnSize.m_fHeight = (height / fLogToScreenRatioY) * fWidthRatio;	
	
	if (bUseScratchBuffer)
	{
		// copy the last of it
		if (pSourceCopyStart)
		{
			int nLen = PStrLen(pSourceCopyStart);
			memcpy(pCurDestText, pSourceCopyStart, sizeof(WCHAR) * nLen);
		}

		// KCK: Unfortunately this function can be called two ways. If we've passed in a text array,
		// then we need to reallocate that memory. If we've passed a line pointer, just set the text.
		int nLen = PStrLen(szDestText);
		if (pLine)
			pLine->SetText(szDestText);
		else
		{
			szTextIn = (WCHAR*)REALLOC(NULL, szTextIn, sizeof(WCHAR) * (nLen + 1));
			PStrCopy(szTextIn, szDestText, nLen+1);
		}
	}

	FREE_DELETE_ARRAY(NULL, szDestText, WCHAR);

	return returnSize;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UITextStringReplaceFormattingTokens(
	WCHAR * szText)
{
	WCHAR szColorToken[3] = { L"{c" };
	WCHAR szEndColorToken[5] = { L"{\\c}" };

	// Look for any color specifiers
	const WCHAR *szTemp = szText;
	szTemp = PStrStr(szText, szColorToken);
	while (szTemp)
	{
		const WCHAR *szEndBrace = PStrStr(szTemp, L"}");
		if (!szEndBrace)
		{
			// No end brace, give up
			break;
		}
		// convert the color number string
		int nColorSegmentLen = SIZE_TO_INT(szEndBrace - (szTemp + 2));

		int nColor = INVALID_LINK;
		if (*(szTemp+2) == L':')	// named color
		{
			WCHAR szColorName[256] = L"";
			char szKeyName[256] = "";
			PStrCopy( szColorName, szTemp + 3, nColorSegmentLen );

			PStrTrimLeadingSpaces(szColorName, arrsize(szColorName));
			PStrTrimTrailingSpaces(szColorName, arrsize(szColorName));
			PStrCvt( szKeyName, szColorName, arrsize(szColorName) );
			nColor = ExcelGetLineByStringIndex(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_FONTCOLORS, szKeyName);
			if (nColor == INVALID_LINK)
			{
				// badness
				ASSERTV(FALSE, "Color label ""%s"" not recognized from FontColors", szColorName );
				return;
			}
		}
		else
		{
			WCHAR szColorNumber[4] = {0,0,0,0};
			PStrCopy(szColorNumber, szTemp+2, MIN(3, nColorSegmentLen)+1);
			nColor = PStrToInt(szColorNumber);
		}

		// Now replace the string token with the byte codes
		WCHAR * szWrite = const_cast<WCHAR*>(szTemp);
		*szWrite = CTRLCHAR_COLOR;
		*(szWrite+1) = (WCHAR)nColor;

		// Now slide the rest of the string down by the length of the rest of the token
		szTemp += 2;
		while(*szTemp != 0 && *szWrite != 0)
		{
			szWrite = const_cast<WCHAR*>(szTemp);
			*szWrite = *(szTemp + nColorSegmentLen + 1);
			szTemp++;
		}

		// Now do the search again
		szTemp = PStrStr(szText, szColorToken);
	}

	// Look for end color token
	szTemp = PStrStr(szText, szEndColorToken);
	while (szTemp)
	{
		// Now replace the string token with the byte code
		WCHAR * szWrite = const_cast<WCHAR*>(szTemp);
		*szWrite = CTRLCHAR_COLOR_RETURN;
		int nTokenLen = PStrLen(szEndColorToken) -1;

		// Now slide the rest of the string down by the length of the rest of the token
		szTemp++;
		while( *szTemp != 0 )
		{
			szWrite = const_cast<WCHAR*>(szTemp);
			*szWrite = *(szTemp + nTokenLen);
			szTemp++;
		}

		// Now do the search again
		szTemp = PStrStr(szText, szEndColorToken);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UIColorGlobalString(
	GLOBAL_STRING eGlobalString,
	FONTCOLOR eColor,
	WCHAR *puszBuffer,
	int nMaxBuffer)
{
	ASSERTX_RETFALSE( eGlobalString != GS_INVALID, "Invalid global string" );
	ASSERTX_RETFALSE( eColor != FONTCOLOR_INVALID, "Invalid color" );
	ASSERTX_RETFALSE( puszBuffer, "Expected buffer" );
	ASSERTX_RETFALSE( nMaxBuffer > 0, "Invalid buffer size" );

	// clear the buffer
	puszBuffer[ 0 ] = 0;
	
	// color cat the string
	UIColorCatString( puszBuffer, nMaxBuffer, eColor, GlobalStringGet( eGlobalString ) );

	return TRUE;
	
}

#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIAddColorToString(
	WCHAR *szString,
	enum FONTCOLOR eColor,
	int nStrLen)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT((int)eColor > 0);

	PStrCatChar( szString, (WCHAR)CTRLCHAR_COLOR, nStrLen);
	PStrCatChar( szString, (WCHAR)eColor, nStrLen);
#endif // !ISVERSION(SERVER_VERSION)
}

void UIAddColorReturnToString(
	WCHAR *szString,
	int nStrLen)
{
#if !ISVERSION(SERVER_VERSION)
	PStrCatChar( szString, (WCHAR)CTRLCHAR_COLOR_RETURN, nStrLen);
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIColorCatString(
	WCHAR *szString,
	int nStrLen,
	enum FONTCOLOR eColor,
	const WCHAR *szStrToCat)
{
#if !ISVERSION(SERVER_VERSION)
	// just really wrapping the above 2 functions together
	UIAddColorToString(szString, eColor, nStrLen);
#endif // !ISVERSION(SERVER_VERSION)

	PStrCat(szString, szStrToCat, nStrLen);

#if !ISVERSION(SERVER_VERSION)
	UIAddColorReturnToString(szString, nStrLen);
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
// Sets the screen rect for any hypertext tokens in a string
//
// This requires yet *another* traverse of the string, but until a more
// general redesign of the system takes place, I can't really see how to
// avoid this.
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void UISetHypertextBounds(
	UI_LINE* pLine, 	
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	BOOL bNoScale,
	float fLineHeight,
	float fX, 
	float fY )
{
	if (!pLine || !pLine->HasText())
		return;

	const WCHAR *text = pLine->GetText();
	float width = 0.0f;
	float fWidthRatio = 1.0; //fontsize / font->m_fHeight;
	float fTabWidth = font->m_fTabWidth * fWidthRatio;
	UI_HYPERTEXT	*pCurrentHypertext = NULL;

	WCHAR wc;
	while ((wc = *text++) != 0)
	{
		if (wc == CTRLCHAR_COLOR || wc == CTRLCHAR_LINECOLOR)
		{
			text++;
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT)
		{
			pCurrentHypertext = UIFindHypertextById(*text);
			if (pCurrentHypertext)
			{
				pCurrentHypertext->m_UpperLeft.m_fX = fX + width;
				pCurrentHypertext->m_UpperLeft.m_fY = fY;
			}
			text++;
			continue;
		}
		if (wc == CTRLCHAR_HYPERTEXT_END)
		{
			if (pCurrentHypertext)
			{
				pCurrentHypertext->m_LowerRight.m_fX = fX + width;
				pCurrentHypertext->m_LowerRight.m_fY = fY + fLineHeight;
			}
			pCurrentHypertext = NULL;
			continue;
		}

		if (wc == L'\n')
		{
			width = 0.0f;
			continue;
		}
		if (wc == L'\t')
		{
			width = float(int( width / fTabWidth + 0.5f) + 1 ) * fTabWidth;
			continue;
		}
		if (wc == L'\f')
		{
			continue;
		}
		if (PStrIsPrintable( wc ) == FALSE)		
		{
			continue;
		}

		wc = sUIJabberChar(font, wc);
		wc = sUICheckGlyphset(font, wc);

		width += UIGetCharWidth(font, nFontSize, fWidthRatio, bNoScale, wc);
	}

	// Catch the end of the hypertext tag, even if the end tag hasn't been found yet
	// (handles cases where the tag is wrapped)
	if (pCurrentHypertext)
	{
		pCurrentHypertext->m_LowerRight.m_fX = fX + width;
		pCurrentHypertext->m_LowerRight.m_fY = fY + fLineHeight;
	}
}
#endif // !ISVERSION(SERVER_VERSION)
