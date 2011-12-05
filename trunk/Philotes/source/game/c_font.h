//----------------------------------------------------------------------------
// c_font.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _C_FONT_H_
#define _C_FONT_H_


#ifndef _COLORS_H_
#include "colors.h"
#endif

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
#include <PshPack1.h>
struct RGBA
{
	union
	{
		struct
		{
			DWORD dwColor;
		};
		struct 
		{
			BYTE bBlue;
			BYTE bGreen;
			BYTE bRed;
			BYTE bAlpha;
		};
	};
};
#include "PopPack.h"

struct FONT_DEFINITION 
{
	char	pszName[DEFAULT_INDEX_SIZE];		// name when referenced by other tables
	char	pszSystemName[DEFAULT_FILE_SIZE];	// system font name to load
	char	pszLocalPath[DEFAULT_FILE_SIZE];	// system font name to load
	BOOL	bBold;								// is the font bold?
	BOOL	bItalic;							// is the font in italics?

	int		nGDISize;							// GDI size
	int		nSizeInTexture;						// (val x val) size bitmap for each letter
	float	fSizeInTexture;

	int		nLetterCountWidth;					// number of letters on width of bitmap
	int		nLetterCountHeight;					// number of letters on height of bitmap

	BOOL	bInitialized;						// have we created the texture yet?
	int		nTextureId;							// the texture used by the font
	float	pfFontWidth[128];					// the width of each letter
	float   fTabWidth;							// the width of each tab stop
};


struct FONT_COLOR_DATA
{
	char	szColor[DEFAULT_INDEX_SIZE];
	RGBA	rgbaColor;
	int		nFontColor;							// refers to another font color instead of a color directly
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
int GetFontIndex(
	const char * szName);

const FONT_DEFINITION * GetFont(
	const char * szName);

const FONT_DEFINITION * GetFont(
	int idFont);

DWORD GetFontColor(
	int color);

#endif // _C_FONT_H_