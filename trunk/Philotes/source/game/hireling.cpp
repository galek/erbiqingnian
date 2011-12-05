//----------------------------------------------------------------------------
// FILE: hireling.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_message.h"
#include "dialog.h"
#include "hireling.h"
#include "interact.h"
#include "inventory.h"
#include "globalindex.h"
#include "npc.h"
#include "s_message.h"
#include "stringtable.h"
#include "uix_money.h"
#include "units.h"
#include "Currency.h"
#include "stringreplacement.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
static const int HAS_MAX_HIRELINGS = -1;
#define HIRELING_COST_PER_LEVEL	50
#define HIRELING_BASE_COST		500

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_HirelingGetAvailableHirelings( 
	UNIT *pPlayer, 
	UNIT *pForeman, 
	HIRELING_CHOICE *pHirelings, 
	int nMaxHirelings)
{
	ASSERTX_RETZERO( nMaxHirelings > 0, "Expected hireling storage" );
	int nNumHirelings = 0;

	// count how many hirelings we already have
	int nExistingHirelingCount = 0;
	UNIT *pItem = NULL;
	while ((pItem = UnitInventoryIterate( pPlayer, pItem )) != NULL)
	{
		if ( UnitIsA( pItem, UNITTYPE_HIRELING ) )
		{
			nExistingHirelingCount++;
		}
	}

	// check for max
	int MAX_HIRELINGS = 1;  // hard coded
	if (nExistingHirelingCount >= MAX_HIRELINGS)
	{
		return HAS_MAX_HIRELINGS;  // no more available for now
	}
	
	// this is hard coded for now
	int nMonsterClass = UnitGetData( pForeman )->nMinionClass != INVALID_ID ? 
						UnitGetData( pForeman )->nMinionClass  :
						GlobalIndexGet( GI_HIRELING_DEFAULT );	
	if (nMonsterClass != INVALID_LINK)
	{
		pHirelings[ 0 ].nMonsterClass = nMonsterClass;
		pHirelings[ 0 ].nCost = HIRELING_BASE_COST + UnitGetExperienceLevel( pPlayer ) * HIRELING_COST_PER_LEVEL;
		pHirelings[ 0 ].nRealWorldCost = 0;
		nNumHirelings++;
	}
	
	// return the number of hirelings available
	return nNumHirelings;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT_INTERACT s_HirelingSendSelection(
	UNIT *pUnit,
	UNIT *pForeman)
{
	ASSERTX_RETVAL( pUnit, UNIT_INTERACT_NONE, "Expected unit" );
	ASSERTX_RETVAL( pForeman, UNIT_INTERACT_NONE, "Expected unit" );
	ASSERTX_RETVAL( UnitIsForeman( pForeman ), UNIT_INTERACT_NONE, "Expected foreman" );
	
	// get available hirelings
	MSG_SCMD_HIRELING_SELECTION tMessage;	
	tMessage.nNumHirelings = s_HirelingGetAvailableHirelings( pUnit, pForeman, tMessage.tHirelings, MAX_HIRELING_MESSAGE );

	s_NPCTalkStart(	pUnit, pForeman, NPC_DIALOG_OK, DIALOG_HIRELING_NOT_ENOUGH_MONEY );

	if (tMessage.nNumHirelings > 0)
	{	

		// do they have enough money for any of them
		BOOL bCanBuyHireling = FALSE;
		for (int i = 0; i < tMessage.nNumHirelings; ++i)
		{
			const HIRELING_CHOICE *pHirelingChoice = &tMessage.tHirelings[ i ];
			if ( UnitGetCurrency( pUnit ) >= cCurrency( pHirelingChoice->nCost, pHirelingChoice->nRealWorldCost ) )
			{
				bCanBuyHireling = TRUE;
				break;
			}
		}
				
		// setup rest of message
		tMessage.idForeman = UnitGetId( pForeman );
		tMessage.bSufficientFunds = (BYTE)bCanBuyHireling;
		// send to client
		GAMECLIENTID idClient = UnitGetClientId( pUnit );
		s_SendUnitMessageToClient( pUnit, idClient, SCMD_HIRELING_SELECTION, &tMessage );
		return UNIT_INTERACT_HIRELING;
			
	}
	else if (tMessage.nNumHirelings == HAS_MAX_HIRELINGS)
	{
		s_NPCTalkStart(	pUnit, pForeman, NPC_DIALOG_OK, DIALOG_HIRELING_TOO_MANY_HIRELINGS );
		return UNIT_INTERACT_TALK;				
	}

	return UNIT_INTERACT_NONE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void sBuyHirelingAccept(
	void *pUserData, 
	DWORD dwCallbackData)
{
	MSG_CCMD_BUY_HIRELING tMessage;
	tMessage.nIndex = 0;  // until we have a UI for multiple hireling options
	c_SendMessage( CCMD_BUY_HIRELING, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_HirelingsOffer( 
	UNIT *pPlayer,
	UNIT *pForeman, 
	const HIRELING_CHOICE *pHirelings, 
	int nNumHirelings,
	BOOL bSufficientFunds )
{
	GAME *pGame = UnitGetGame( pPlayer );
	
	const WCHAR *puszText;
	if( !bSufficientFunds )
	{
		puszText = c_DialogTranslate( DIALOG_HIRELING_NOT_ENOUGH_MONEY );

	}
	else
	{
		puszText = c_DialogTranslate( DIALOG_HIRELING_BUY_HIRELING );
	}
	
	// for now, we only support one hireling at a time ... one day we should have 
	// choices tho, cause it's cool to have choices
	ASSERTX_RETURN( nNumHirelings == 1, "Expected only 1 hireling choice" );
	const HIRELING_CHOICE *pHirelingChoice = &pHirelings[ 0 ];

	WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];			

	memset(uszTextBuffer,0,sizeof(uszTextBuffer));

	cCurrency Cost( pHirelingChoice->nCost, 0 );

	const int MAX_MONEY_STRING = 128;
	WCHAR uszMoney[ MAX_MONEY_STRING ];
	PStrCat( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", Cost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
	// replace any money token with the money value
	const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", Cost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
	// replace any money token with the money value
	puszGoldToken = StringReplacementTokensGet( SR_SILVER );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", Cost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
	// replace any money token with the money value
	puszGoldToken = StringReplacementTokensGet( SR_GOLD );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );


	int playerLevel = UnitGetExperienceLevel( pPlayer );
	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", playerLevel );					
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, StringReplacementTokensGet( SR_LEVEL ), uszMoney );

	const UNIT_DATA* unit_data = (UNIT_DATA*)ExcelGetData(NULL, DATATABLE_MONSTERS, pHirelingChoice->nMonsterClass);
	if (unit_data != NULL )
	{
		
		int nString = unit_data->nString;
		if ( nString != INVALID_LINK)
		{
			// get the attributes to match for number
			const WCHAR * str = StringTableGetStringByIndex( nString );
			PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, StringReplacementTokensGet( SR_MONSTER ), str );
		}
	}

	// seutp dialog spec		
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = UnitGetId( pForeman );
	tSpec.puszText = uszTextBuffer;
	tSpec.eType = bSufficientFunds? NPC_DIALOG_OK_CANCEL : NPC_DIALOG_OK;
	tSpec.tCallbackOK.pfnCallback = sBuyHirelingAccept;

	// start talking
	c_NPCTalkStart( pGame, &tSpec );		
}
#endif //!ISVERSION(SERVER_VERSION)
