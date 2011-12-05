//----------------------------------------------------------------------------
// FILE: act.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "accountbadges.h"
#include "act.h"
#include "clients.h"
#include "excel.h"
#include "units.h"
#include "player.h"
#include "warp.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
static const int MAX_ACT_STRING = 64;
static char sgszLastAct[ MAX_ACT_STRING ] = { 0 };
static int sgnActLast = INVALID_LINK;	// this is the last act that player can go into

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ACT_DATA *ActGetData(
	int nAct)
{
	ASSERTV_RETNULL( nAct != INVALID_LINK, "Invalid act link" );
	return (const ACT_DATA *)ExcelGetData( NULL, DATATABLE_ACT, nAct );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ActExcelPostProcess(
	struct EXCEL_TABLE *pTable)
{
	ASSERTV_RETFALSE( pTable, "Expected table" );
	
	// set the last act link (if any)
	if (sgszLastAct[ 0 ] != 0)
	{
		int nNumActs = ExcelGetNumRows( NULL, DATATABLE_ACT );
		for (int i = 0; i < nNumActs; ++i)
		{
			const ACT_DATA *pActData = ActGetData( i );
			if (PStrICmp( pActData->szName, sgszLastAct ) == 0)
			{
				sgnActLast = i;
				break;
			}
			
		}
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ActSetLastAct(
	const char *pszAct)
{
	ASSERTX_RETURN( pszAct, "Expected act string" );
	PStrCopy( sgszLastAct, pszAct, MAX_ACT_STRING );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ActIsAvailable(
	int nAct)
{

	// what's an invalid act anyway?
	if (nAct == INVALID_LINK)
	{
		return FALSE;
	}
	
	// if there is no last act specified, let's say they are all available
	if (sgnActLast == INVALID_LINK)
	{
		return TRUE;
	}

	// must be less than or equal to the last act set
	if (nAct > sgnActLast)
	{
		return FALSE;
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WARP_RESTRICTED_REASON ActIsAvailableToUnit(
	UNIT *pUnit,
	int nAct)
{
	ASSERTV_RETVAL( pUnit, WRR_UNKNOWN, "Expected unit" );

	if (nAct != INVALID_LINK)
	{
		
		// hellgate beta accounts must have a special badge
		if (AppIsHellgate() && UnitIsPlayer( pUnit ))
		{
			const ACT_DATA *pActData = ActGetData( nAct );
			ASSERTV_RETVAL( pActData, WRR_UNKNOWN, "Expected act data" )

			// does this act allow beta accounts to play it
			if (TESTBIT( pActData->dwActDataFlags, ADF_BETA_ACCOUNT_CAN_PLAY ) == FALSE)
			{
			
				// beta accounts that are on their grace period cannot move past 
				if (UnitHasBadge( pUnit, ACCT_MODIFIER_BETA_GRACE_PERIOD_USER ) == TRUE)
				{
					return WRR_ACT_BETA_GRACE_PERIOD;
				}
								
			}

			// check trial account restrictions
			if (UnitHasBadge( pUnit, ACCT_TITLE_TRIAL_USER ))
			{
				if (TESTBIT( pActData->dwActDataFlags, ADF_TRIAL_ACCOUNT_CAN_PLAY ) == FALSE)
				{
					return WRR_ACT_TRIAL_ACCOUNT;
				}
			}

			// does this act allow non-subscribers to play it
			if (TESTBIT( pActData->dwActDataFlags, ADF_NONSUBSCRIBER_ACCOUNT_CAN_PLAY ) == FALSE)
			{

				// units that belong to non-subscriber accounts cannot play this act
				UNIT *pPlayer = UnitGetUltimateOwner( pUnit );
				if (UnitIsPlayer( pPlayer ) && !PlayerIsSubscriber( pPlayer ))
				{
					return WRR_ACT_SUBSCRIBER_ONLY;
				}

			}
			
			// check minimum level requirement for this act
			int nExperienceLevel = UnitGetExperienceLevel( pUnit );
			if (nExperienceLevel < pActData->nMinExperienceLevelToEnter)
			{
				return WRR_ACT_EXPERIENCE_TOO_LOW;
			}
			
		}

	}
	
	return WRR_OK;
			
}
