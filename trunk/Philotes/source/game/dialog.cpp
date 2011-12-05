//----------------------------------------------------------------------------
// FILE: dialog.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "game.h"
#include "prime.h"
#include "stringtable.h"
#include "dialog.h"
#include "unitmodes.h"
#include "excel_private.h"


//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// DEFINTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DIALOG_DATA *DialogGetData(
	int nDialog)
{
	return (const DIALOG_DATA *)ExcelGetData( NULL, DATATABLE_DIALOG, nDialog );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *c_DialogTranslate(
	int nDialog)
{
	const DIALOG_DATA *pDialogData = DialogGetData( nDialog );
	ASSERTX_RETNULL( pDialogData, "Expected dialog data" );
	
	// get player
	UNIT *pPlayer = GameGetControlUnit( AppGetCltGame() );
	ASSERTX_RETNULL( pPlayer, "Expected player" );
	
	// go through all options and see if we can find one that works
	int nString = INVALID_LINK;
	for (int i = 0; i < NUM_DIALOG_APP; ++i)
	{
		DIALOG_APP eDialogApp = (DIALOG_APP)i;
		
		BOOL bValidColumn = TRUE;
		if (eDialogApp == DIALOG_APP_HELLGATE)
		{
			if (AppIsHellgate() == FALSE)
			{
				bValidColumn = FALSE;
			}
		}
		if (eDialogApp == DIALOG_APP_MYTHOS)
		{
			if (AppIsTugboat() == FALSE)
			{
				bValidColumn = FALSE;
			}
		}
		
		if (bValidColumn)
		{
			const DIALOG_OPTION *ptDialogOption = &pDialogData->tDialogs[ i ];			
			nString = ptDialogOption->nString;
			if (nString != INVALID_LINK)
				break;
		}
				
	}
	
	// get the player gender
	GENDER eGender = UnitGetGender( pPlayer );
	
	// setup attributes of string to look for
	DWORD dwAttributes = StringTableGetGenderAttribute( eGender );
	
	if (UnitHasBadge(pPlayer, ACCT_STATUS_UNDERAGE))	//UNDER_18
	{
		dwAttributes |= StringTableGetUnder18Attribute();
	}
			
	// get string
	return StringTableGetStringByIndexVerbose( 
		nString, 
		dwAttributes,
		NULL, 
		NULL,
		NULL);  // can handle an INVALID_LINK
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_DialogGetSound(
	int nDialog)
{
	const DIALOG_DATA *ptDialogData = DialogGetData( nDialog );
	ASSERT_RETINVALID(ptDialogData);
	return ptDialogData->nSound;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITMODE c_DialogGetMode(
	int nDialog)
{
	const DIALOG_DATA *ptDialogData = DialogGetData( nDialog );
	ASSERT_RETVAL(ptDialogData, MODE_INVALID);
	return ptDialogData->eUnitMode;
}


