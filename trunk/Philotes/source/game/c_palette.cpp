//----------------------------------------------------------------------------
// c_font.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "c_palette.h"
#include "colors.h"
#include "appcommon.h"
#include "filepaths.h"
#include "game.h"
#include "excel.h"
#include "pakfile.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define FILECODE_PALETTE		MAKEFOURCC('p', 'a', 'l', 0)


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PaletteRowFree(
	GAME * game,
	DATA_TABLE * table,
	void * row_data)
{
	PALETTE_DATA * palette = (PALETTE_DATA *)row_data;

	if (palette->pdwColors)
	{
		GFREE(game, palette->pdwColors);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const PALETTE_DATA * PaletteGet(
	GAME * game,
	PALETTE palette)
{
	PALETTE_DATA * palette_data = (PALETTE_DATA *)ExcelGetData(game, DATATABLE_PALETTES, palette);
	if (!palette_data)
	{
		return NULL;
	}
	if (!palette_data->pdwColors)
	{
		TCHAR filename[MAX_PATH];		
		if (!PakFileGetFileName(palette_data->fileid, filename, MAX_PATH))
		{
			return NULL;
		}

		DWORD filesize;
		BYTE * buf = (BYTE *)FileLoad(MEMORY_FUNC_FILELINE(NULL, filename, &filesize));
		if (!buf)
		{
			return NULL;
		}
		BIT_BUF pal(buf, filesize);
		if (pal.GetUInt(32 BBD_FILE_LINE) != FILECODE_PALETTE)
		{
			return NULL;
		}
		int color_count = pal.GetInt(32 BBD_FILE_LINE);
		if (color_count < 0 || color_count > 65536)
		{
			return NULL;
		}
		if (pal.GetLen() != color_count * sizeof(DWORD))
		{
			return NULL;
		}
		
		palette_data->nCount = color_count;
		palette_data->pdwColors = (DWORD *)GMALLOC(game, color_count * sizeof(DWORD));
		memcpy(palette_data->pdwColors, pal.GetPtr(), color_count * sizeof(DWORD));
	}
	return palette_data;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD PaletteGetColor(
	GAME * pGame,
	PALETTE palette,
	int index)
{
	const PALETTE_DATA * palette_data = PaletteGet(pGame, palette);
	if (!palette_data)
	{
		return 0xffffffff;
	}
	ASSERT(index > 0 && index < palette_data->nCount);
	if (!(index > 0 && index < palette_data->nCount))
	{
		return 0xffffffff;
	}
	return palette_data->pdwColors[index];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PaletteCreateDefaultPaletteFile(
	GAME * pGame)
{
	char filename[MAX_PATH];
	PStrPrintf(filename, MAX_PATH, "%s%s.pal", FILE_PATH_PALETTE, "palette");

	// create a palette file
	HANDLE file = FileOpen(filename, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	if (file == INVALID_FILE)
	{
		return;
	}

	static const DWORD pal[] =
	{
		GFXCOLOR_BLACK,
		GFXCOLOR_WHITE,
		GFXCOLOR_RED,
		GFXCOLOR_GREEN,
		GFXCOLOR_BLUE,
		GFXCOLOR_YELLOW,
		GFXCOLOR_CYAN,
		GFXCOLOR_PURPLE,
		GFXCOLOR_GRAY,
		GFXCOLOR_DKRED,
		GFXCOLOR_DKGREEN,
		GFXCOLOR_DKBLUE,
		GFXCOLOR_DKYELLOW,
		GFXCOLOR_DKCYAN,
		GFXCOLOR_DKPURPLE,
		GFXCOLOR_DKGRAY,
		GFXCOLOR_VDKRED,
		GFXCOLOR_VDKGREEN,
		GFXCOLOR_VDKBLUE,
		GFXCOLOR_VDKYELLOW,
		GFXCOLOR_VDKCYAN,
		GFXCOLOR_VDKPURPLE,
		GFXCOLOR_VDKGRAY,
	};
	static const int palsize = arrsize(pal);
	DWORD dw = MAKEFOURCC('p', 'a', 'l', 0);
	FileWrite(file, &dw, sizeof(DWORD));
	dw = palsize;
	FileWrite(file, &dw, sizeof(DWORD));
	FileWrite(file, pal, sizeof(DWORD) * palsize);
	FileClose(file);
}
