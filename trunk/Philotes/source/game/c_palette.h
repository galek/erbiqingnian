//----------------------------------------------------------------------------
// c_palette.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _C_PALETTE_H_
#define _C_PALETTE_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum PALETTE
{
	PALETTE_FONT,

	NUM_PALETTES
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct PALETTE_DATA
{
	FILEID	fileid;

	int		nCount;
	SAFE_PTR(DWORD*, pdwColors);
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void PaletteRowFree(
	struct GAME * pGame,
	struct DATA_TABLE * pTable,
	void * row_data);

const PALETTE_DATA * PaletteGet(
	struct GAME * pGame,
	PALETTE palette);

DWORD PaletteGetColor(
	struct GAME * pGame,
	PALETTE palette,
	int index);


#endif // _C_PALETTE_H_
