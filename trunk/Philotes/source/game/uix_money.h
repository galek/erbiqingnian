//----------------------------------------------------------------------------
// FILE: uix_money.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __UIX_MONEY_H_
#define __UIX_MONEY_H_

#ifndef __CURRENCY_H_
#include "Currency.h"
#endif
//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;

//----------------------------------------------------------------------------
enum MONEY_FLAGS
{
	MONEY_FLAGS_SILENT_BIT,		// do not do the "count up" animation and keep the ui hidden
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void UIMoneyInit(
	void);

void UIMoneyFree(
	void);
		
void UIMoneySet(
	GAME *pGame,
	UNIT *pUnit,
	cCurrency newCurrency,
	DWORD dwFlags = 0);		// see MONEY_FLAGS

void UIMoneyLocalizeMoneyString( 
	WCHAR *puszDest, 
	int nDestLen, 
	const WCHAR *puszSource);

void UIMoneyGetString( 
	WCHAR *puszDest, 
	int nDestLen, 
	int nMoney);

#endif