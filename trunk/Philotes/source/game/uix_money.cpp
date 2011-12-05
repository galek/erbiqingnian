//----------------------------------------------------------------------------
// FILE: uix_money.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_sound.h"
#include "events.h"
#include "game.h"
#include "globalindex.h"
#include "items.h"
#include "language.h"
#include "uix.h"
#include "uix_money.h"
#include "unit_priv.h"
#include "uix_components.h"
#include "Currency.h"
#include "uix_email.h"
#include "uix_auction.h"

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct MONEY_GLOBALS
{
	cCurrency nCurrentValue;	// current value of the money
	cCurrency nTargetValue;		// final target value of money
	cCurrency nIncrement;			// every time we update we will adjust the current value by this much until we reach the target value
	GAME_EVENT *pEvent;		// event to update the system
	DWORD dwHitTargetTick;	// when we hit our target, this is the tick
};

//----------------------------------------------------------------------------
enum MONEY_FADE
{
	MONEY_FADE_IN,
	MONEY_FADE_OUT,
	MONEY_FADE_HIDDEN,
};

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
static MONEY_GLOBALS sgtMoneyGlobals;
#define MONEY_UPDATE_DELAY (1)
#define MONEY_TOTAL_COUNT_TICKS (GAME_TICKS_PER_SECOND * 1)
#define MONEY_VISIBLE_TIME (GAME_TICKS_PER_SECOND * 3)

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
static void sRegisterMoneyUpdate(
	GAME *pGame);

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static MONEY_GLOBALS *sMoneyGlobalsGet(
	void)
{
	return &sgtMoneyGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMoneyFade(
	MONEY_FADE eFade)
{
	UI_COMPONENT *pPanel = UIComponentGetByEnum( UICOMP_MONEY_COUNTER_PANEL );			
	if (pPanel != NULL)
	{

		// does the money component fade in and out		
		if (AppIsTugboat())
		{
			UIComponentActivate( pPanel, TRUE );
			return;
			
		}
		
		if (eFade == MONEY_FADE_IN)
		{
			//UIComponentSetVisible( pPanel, TRUE );		
			//UIComponentFadeIn( pPanel, flDurationMultiplier, dwAnimTime );
			UIComponentActivate(pPanel, TRUE);
		}
		else if (eFade == MONEY_FADE_OUT )
		{
			//UIComponentSetVisible( pPanel, TRUE );	
			//UIComponentFadeOut( pPanel, flDurationMultiplier, dwAnimTime );
			UIComponentActivate(pPanel, FALSE);
		}
		else if (eFade == MONEY_FADE_HIDDEN )
		{
			UIComponentSetVisible( pPanel, FALSE );
		}
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMoneyInit(
	void)
{
	MONEY_GLOBALS *pMoneyGlobals = sMoneyGlobalsGet();
	
	pMoneyGlobals->nCurrentValue = 0;
	pMoneyGlobals->nTargetValue = 0;
	pMoneyGlobals->pEvent = NULL;	
	pMoneyGlobals->dwHitTargetTick = 0;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMoneyFree(
	void)
{	
	UIMoneyInit();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMoneyLocalizeMoneyString( 
	WCHAR *puszDest, 
	int nDestLen, 
	const WCHAR *puszSource)
{
	LanguageFormatNumberString( puszDest, nDestLen, puszSource );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetMoneyValue(
	cCurrency nMoney)
{
	MONEY_GLOBALS *pMoneyGlobals = sMoneyGlobalsGet();
	
	// get money as string
	const int MAX_MONEY_STRING = 128;
	WCHAR uszMoney[ MAX_MONEY_STRING ];
	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney.GetValue( KCURRENCY_VALUE_INGAME ) );
	
	// do localized separators
	WCHAR uszMoneyLocalized[ MAX_MONEY_STRING ];
	UIMoneyLocalizeMoneyString( uszMoneyLocalized, MAX_MONEY_STRING, uszMoney );
	

	// set into ui element for Credits
	if( AppIsHellgate() )
	{
		// set into ui element
		UILabelSetTextByEnum( UICOMP_MONEY_COUNTER_LABEL, uszMoneyLocalized );
	}else if( AppIsTugboat() )
	{
		// set into ui element
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
		UILabelSetTextByEnum( UICOMP_MONEY_COUNTER_LABEL, uszMoney );
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
		UILabelSetTextByEnum( UICOMP_MONEY_SILVER_COUNTER_LABEL, uszMoney );
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
		UILabelSetTextByEnum( UICOMP_MONEY_COPPER_COUNTER_LABEL, uszMoney );
		
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney.GetValue( KCURRENCY_VALUE_REALWORLD ) );
		UIMoneyLocalizeMoneyString( uszMoneyLocalized, MAX_MONEY_STRING, uszMoney );
		UILabelSetTextByEnum( UICOMP_MONEY_CREDITS_COUNTER_LABEL, uszMoneyLocalized );
	}
	// set the new money 
	pMoneyGlobals->nCurrentValue = nMoney;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUpdateMoney(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
	MONEY_GLOBALS *pMoneyGlobals = sMoneyGlobalsGet();

	// immediately invalidate the event 
	pMoneyGlobals->pEvent = NULL;
	
	// do we need to update the pile?
	if (pMoneyGlobals->nCurrentValue != pMoneyGlobals->nTargetValue)
	{
		//this is a bit dangerous if we ever run into the situation where in game gold is being removed and
		//real world gold is being added ( or vice-versa )
		//One has to be zero..
		ASSERT_RETTRUE( pMoneyGlobals->nIncrement.GetValue( KCURRENCY_VALUE_INGAME ) == 0 ||
						pMoneyGlobals->nIncrement.GetValue( KCURRENCY_VALUE_REALWORLD ) == 0  );
		BOOL removingMoney = pMoneyGlobals->nIncrement.IsNegative();
		// what is the new money value
		cCurrency nNewMoney = pMoneyGlobals->nCurrentValue + pMoneyGlobals->nIncrement;

		// check for going over the target
		if ( removingMoney == FALSE)
		{
			if (nNewMoney >= pMoneyGlobals->nTargetValue)
			{
				nNewMoney = pMoneyGlobals->nTargetValue;
			}
		}
		else
		{
			
			if (nNewMoney <= pMoneyGlobals->nTargetValue)
			{
				nNewMoney = pMoneyGlobals->nTargetValue;
			}
		
		}
		
		// set money value into ui
		sSetMoneyValue( nNewMoney );

		// check for hitting the target value
		if (pMoneyGlobals->nTargetValue == nNewMoney)
		{
			// set the time we hit it on
			pMoneyGlobals->dwHitTargetTick = GameGetTick( pGame );
		}
		
		// register another event to keep the counting going
		sRegisterMoneyUpdate( pGame );
		
	}
	else
	{
	
		// our value is on target, after not changing for a while we will fade out the display
		DWORD dwNow = GameGetTick( pGame );
		if (dwNow - pMoneyGlobals->dwHitTargetTick > MONEY_VISIBLE_TIME)
		{
			sMoneyFade( MONEY_FADE_OUT );
		}
		else
		{
		
			// keep this simulation going till we get to the target time ... we keep it
			// going like this because it's simpler if more money gets added while
			// the display is visible
			sRegisterMoneyUpdate( pGame );
			
		}
		
	}

	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMoneySet(
	GAME *pGame,
	UNIT *pUnit,
	cCurrency newCurrency,
	DWORD dwFlags)
{					
	int nMoneyIndex = GlobalIndexGet( GI_SOUND_UI_MONEY );	

	// play the pickup money sound
	if (nMoneyIndex != INVALID_ID &&
		TESTBIT( dwFlags, MONEY_FLAGS_SILENT_BIT ) == FALSE)
	{
		c_SoundPlay( nMoneyIndex, &c_UnitGetPosition( pUnit ), NULL );
	}

	// for the local player, we do extra stuff	
	if (GameGetControlUnit( pGame ) == pUnit)
	{
		MONEY_GLOBALS *pMoneyGlobals = sMoneyGlobalsGet();
		
		// set the new target value
		pMoneyGlobals->nTargetValue = newCurrency;

		// do silent logic
		if (TESTBIT( dwFlags, MONEY_FLAGS_SILENT_BIT ) || UIIsMerchantScreenUp())
		{
			sSetMoneyValue( newCurrency );
			sMoneyFade( MONEY_FADE_HIDDEN );
		}
		else if (pMoneyGlobals->nTargetValue != pMoneyGlobals->nCurrentValue)
		{
			// if different, we need to show the player
				
			// reset our reached target tick
			pMoneyGlobals->dwHitTargetTick = (DWORD)-1;
			
			// set the display fading up
			sMoneyFade( MONEY_FADE_IN );
						
			// how much do we count up each time
			cCurrency nMoneyDif = pMoneyGlobals->nTargetValue - pMoneyGlobals->nCurrentValue;									
			pMoneyGlobals->nIncrement = nMoneyDif;
			pMoneyGlobals->nIncrement /= (MONEY_TOTAL_COUNT_TICKS * MONEY_UPDATE_DELAY);
			if( pMoneyGlobals->nIncrement.HighestValue() == 0 &&
				pMoneyGlobals->nIncrement.LowestValue() == 0 )
			{
				pMoneyGlobals->nIncrement = nMoneyDif;
			}
			
			// start a timer for the counting going if we don't already have one
			if (pMoneyGlobals->pEvent == NULL)
			{
				sRegisterMoneyUpdate( pGame );
			}
		}

		UIEmailUpdateComposeButtonState();
		UIAuctionUpdateSellButtonState();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRegisterMoneyUpdate(
	GAME * pGame)
{
	MONEY_GLOBALS *pMoneyGlobals = sMoneyGlobalsGet();
				
	GameEventRegisterRet(pMoneyGlobals->pEvent, pGame, sUpdateMoney, NULL, NULL, NULL, MONEY_UPDATE_DELAY);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UIMoneyGetString( 
	WCHAR *puszMoney, 
	int nBufferLen, 
	int nMoney)
{
	ASSERTX_RETURN( puszMoney, "Expected dest buffer" );
	ASSERTX_RETURN( nBufferLen > 0, "Invalid buffer size" );
	
	const int MAX_MONEY_STRING = 128;
	WCHAR uszBuffer[ MAX_MONEY_STRING ];	
	PStrPrintf( uszBuffer, MAX_MONEY_STRING, L"%d", nMoney );
	UIMoneyLocalizeMoneyString( puszMoney, nBufferLen, uszBuffer );

}	

#endif
