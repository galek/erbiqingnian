//----------------------------------------------------------------------------
// c_font.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//
// TODO: 
// 1) fonts could be packed in the texture instead of square
// 2) currently fonts are generated from GDI and only support chars
//    up to ASCII 128.  this must change for localization.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "excel.h"
#include "e_texture.h"

#include "c_font.h"
#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FontInit(
	FONT_DEFINITION * font)
{
	ASSERT(font);
	if (!font)
	{
		return FALSE;
	}
	if (font->bInitialized)
	{
		return TRUE;
	}
	font->bInitialized = TRUE;

	font->fSizeInTexture = (float)font->nSizeInTexture;
	//font->fSizeInTexture = 24.0f;

	V_DO_FAILED( e_FontInit(
		&font->nTextureId,
		font->nLetterCountWidth,
		font->nLetterCountHeight,
		font->nGDISize,
		font->bBold,
		font->bItalic,
		font->pszSystemName,
		font->pszLocalPath,
		font->pfFontWidth,
		font->fTabWidth ) )
	{
		return FALSE;
	}

	e_TextureAddRef( font->nTextureId );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GetFontIndex(
	const char * szFont)
{
	return ExcelGetLineByStringIndex(AppGetCltGame(), DATATABLE_FONT, szFont);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const FONT_DEFINITION * GetFont(
	const char * szName)
{
	FONT_DEFINITION * font = (FONT_DEFINITION *)ExcelGetDataByStringIndex(AppGetCltGame(), DATATABLE_FONT, szName);

	FontInit(font);

	return font;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const FONT_DEFINITION * GetFont(
	int idFont)
{
	if (idFont == INVALID_ID)
	{
		return NULL;
	}
	FONT_DEFINITION * font = (FONT_DEFINITION *)ExcelGetData(AppGetCltGame(), DATATABLE_FONT, idFont);

	FontInit(font);

	return font;
}
#endif //SERVER_VERSION
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD GetFontColor(
	int color)
{
	if ((unsigned int)color >= ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_FONTCOLORS))
	{
		return GFXCOLOR_HOTPINK;
	}

	FONT_COLOR_DATA * color_data = (FONT_COLOR_DATA *)ExcelGetData(NULL, DATATABLE_FONTCOLORS, color);
	if (!color_data)
	{
		return GFXCOLOR_HOTPINK;	// hot pink
	}
	
	// if links to another color, return the link
	if (color_data->nFontColor != INVALID_LINK)
	{
		return GetFontColor( color_data->nFontColor );
	}
	else
	{
		return color_data->rgbaColor.dwColor;
	}
}
