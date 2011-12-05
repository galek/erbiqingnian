//----------------------------------------------------------------------------
// FILE: dialog.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma once
#endif

#ifdef __DIALOG_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __DIALOG_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include "..\data_common\excel\dialog_quests_hdr.h"

#ifndef _PLAYER_H_
#include "player.h"
#endif


//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct GAME;
struct DATA_TABLE;
enum UNITMODE;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
#define DIALOG_INDEX_SIZE (64)

//----------------------------------------------------------------------------
struct CLASS_DIALOG
{
	int nString;				// the text to display
};

//----------------------------------------------------------------------------
enum DIALOG_APP
{
	DIALOG_APP_INVALID = -1,
	
	DIALOG_APP_ALL,
	DIALOG_APP_HELLGATE,
	DIALOG_APP_MYTHOS,
	
	NUM_DIALOG_APP
};

//----------------------------------------------------------------------------
struct DIALOG_OPTION
{
//	int nUnitType;
	int nString;
};

//----------------------------------------------------------------------------
struct DIALOG_DATA 
{
	char szName[ DIALOG_INDEX_SIZE ];					// name of entry

	DIALOG_OPTION tDialogs[ NUM_DIALOG_APP ];
	int nSound;					// sound to play (maybe move this into CLASS_DIALOG one day)
	UNITMODE eUnitMode;			// unit mode to set on talker (maybe move to CLASS_DIALOG one day)
	int nMovieListOnFinished;	// movielist to play when done reading dialog
		
};

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

const DIALOG_DATA *DialogGetData(
	int nDialog);
	
const WCHAR *c_DialogTranslate(
	int nDialog);

int c_DialogGetSound(
	int nDialog);
	
UNITMODE c_DialogGetMode(
	int nDialog);
	
BOOL ExcelDialogPostProcess( 
	struct EXCEL_TABLE * table);

#endif
