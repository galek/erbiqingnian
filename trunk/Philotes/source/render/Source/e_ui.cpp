//----------------------------------------------------------------------------
// e_ui.cpp
//
// - Implementation for UI functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_main.h"
#include "e_ui.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

PFN_UI_RENDER_ALL		gpfn_UIRenderAll	= NULL;
PFN_UI_RESTORE			gpfn_UIRestore		= NULL;
PFN_UI_DISPLAY_CHANGE	gpfn_UIDisplayChange= NULL;	// CHB 2007.08.23

SIMPLE_DYNAMIC_ARRAY<UI_DEBUG_LABEL>	gtUIDebugLabels;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void e_UIInitCallbacks(
	PFN_UI_RENDER_ALL pfnRenderAll,
	PFN_UI_RESTORE pfnRestore,
	PFN_UI_DISPLAY_CHANGE pfnDisplayChange )	// CHB 2007.03.06
{
	gpfn_UIRenderAll = pfnRenderAll;
	gpfn_UIRestore = pfnRestore;
	gpfn_UIDisplayChange = pfnDisplayChange;	// CHB 2007.08.23
}

void e_UIRenderAll()
{
	ASSERT_RETURN( gpfn_UIRenderAll );
	gpfn_UIRenderAll();
}

PRESULT e_UIRestore()
{
	ASSERT_RETFAIL( gpfn_UIRestore );
	gpfn_UIRestore();
	return S_OK;
}

void e_UIDisplayChange(bool bPost)	// CHB 2007.08.23 - false for pre-change, true for post-change
{
	ASSERT_RETURN( gpfn_UIDisplayChange );
	gpfn_UIDisplayChange(bPost);
}

PRESULT e_UIAddLabel(
	const WCHAR * puszText,
	const VECTOR * pvPos,
	BOOL bWorldSpace,
	float fScale,
	DWORD dwColor,
	UI_ALIGN eAnchor,
	UI_ALIGN eAlign,
	BOOL bPinToEdge )
{
	UI_DEBUG_LABEL tLabel;
	PStrCopy( tLabel.wszLabel, puszText, MAX_DEBUG_LABEL_DISPLAY_STRING );
	tLabel.vPos			= *pvPos;
	tLabel.bWorldSpace	= bWorldSpace;
	tLabel.fScale		= fScale;
	tLabel.dwColor		= dwColor;
	tLabel.eAnchor		= eAnchor;
	tLabel.eAlign		= eAlign;
	tLabel.bPinToEdge	= bPinToEdge;
	ArrayAddItem(gtUIDebugLabels, tLabel);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float e_UIFontGetCharWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio, 
	WCHAR wc)
{
	if ( e_IsNoRender() )
		return 0.f;

	UI_TEXTURE_FONT_FRAME* chrframe = font->GetChar(wc, nFontSize);
	if (!chrframe)
	{
		WCHAR str[2] = { wc, L'\0' };
		V( e_UITextureFontAddCharactersToTexture(font, str, nFontSize) );
		chrframe = font->GetChar(wc, nFontSize);
	}

	ASSERT_RETVAL(chrframe, 0.0f);
	return ((chrframe->m_fPreWidth + chrframe->m_fWidth + chrframe->m_fPostWidth + font->m_nExtraSpacing) * fWidthRatio);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float e_UIFontGetCharPreWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio, 
	WCHAR wc)
{
	UI_TEXTURE_FONT_FRAME* chrframe = font->GetChar(wc, nFontSize);
	if (!chrframe)
	{
		WCHAR str[2] = { wc, L'\0' };
		V( e_UITextureFontAddCharactersToTexture(font, str, nFontSize) );
		chrframe = font->GetChar(wc, nFontSize);
	}

	ASSERT_RETVAL(chrframe, 0.0f);
	return (chrframe->m_fPreWidth * fWidthRatio);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float e_UIFontGetCharPostWidth(
	UIX_TEXTURE_FONT* font,
	int nFontSize,
	float fWidthRatio, 
	WCHAR wc)
{
	UI_TEXTURE_FONT_FRAME* chrframe = font->GetChar(wc, nFontSize);
	if (!chrframe)
	{
		WCHAR str[2] = { wc, L'\0' };
		V( e_UITextureFontAddCharactersToTexture(font, str, nFontSize) );
		chrframe = font->GetChar(wc, nFontSize);
	}

	ASSERT_RETVAL(chrframe, 0.0f);
	return (chrframe->m_fPostWidth * fWidthRatio);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------