//----------------------------------------------------------------------------
// FILE: offer.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "globalindex.h"
#include "items.h"
#include "offer.h"
#include "s_reward.h"
#include "treasure.h"
#include "statssave.h"
#include "difficulty.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const OFFER_DEFINITION *OfferGetDefinition(
	int nOffer)
{
	return (const OFFER_DEFINITION *)ExcelGetData( NULL, DATATABLE_OFFER, nOffer );
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFilterOfferings( 
	UNIT *pPlayer,
	int nOffer,
	ITEMSPAWN_RESULT &tResult)
{
	const OFFER_DEFINITION *pOfferDef = OfferGetDefinition( nOffer );
	if (pOfferDef)
	{
		// iterate items
		for (int i = 0; i < tResult.nTreasureBufferCount; ++i)
		{
			UNIT *pItem = tResult.pTreasureBuffer[ i ];

			// test for dupes
			if (pOfferDef->bNoDuplicates == TRUE)
			{
				SPECIES spItem = UnitGetSpecies( pItem );

				int nMaxAllowed = UnitIsContainedBy( pItem, pPlayer) ? 1 : 0;
				// how many of these do we already have			
				DWORD dwFlags = 0;  // no flags, check *all* (even not on person slots)
				if (UnitInventoryCountItemsOfType( pPlayer, spItem, dwFlags ) > nMaxAllowed)
				{

					// delete this item
					UnitFree( pItem );

					// put the last item in the treasure buffer at this slot
					tResult.pTreasureBuffer[ i ] = tResult.pTreasureBuffer[ tResult.nTreasureBufferCount - 1 ];
					tResult.pTreasureBuffer[ tResult.nTreasureBufferCount - 1 ] = NULL;
					tResult.nTreasureBufferCount--;

					// do this index again
					i--;

				}

			}

		}
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sOfferGetTreasure( 
	int nOffer, 
	UNIT *pUnit,
	int nTreasureOverride = INVALID_LINK )
{
	if( nTreasureOverride != INVALID_LINK )
		return nTreasureOverride;
	const OFFER_DEFINITION *pOfferDef = OfferGetDefinition( nOffer );
	if (pOfferDef)
	{
		for (int i = 0; i < MAX_OFFER_OPTIONS; ++i)
		{
			const OFFER_OPTION *ptOfferOption = &pOfferDef->tOffers[ i ];

			if (ptOfferOption->nTreasure != INVALID_LINK)
			{
				if (ptOfferOption->nUnitType == INVALID_LINK ||
					UnitIsA( pUnit, ptOfferOption->nUnitType ))
				{
					return ptOfferOption->nTreasure;
				}
			}

		}
	}
	
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static int s_sOfferCreateTreasureClass(
	UNIT * pPlayer,
	int nOffer, 
	ITEMSPAWN_RESULT & tResult,
	int nTreasureOverride = INVALID_LINK,
	int nLevel = INVALID_LINK)
{
	// get the treasure class to create items with
	int nTreasure = sOfferGetTreasure(nOffer, pPlayer, nTreasureOverride );

	// setup spawn spec
	ITEM_SPEC tSpec;
	tSpec.unitOperator = pPlayer;
	if (nLevel >= 0)
	{
		tSpec.nLevel = nLevel;
		if( AppIsHellgate() )
		{
			SETBIT( tSpec.dwFlags, ISF_USE_SPEC_LEVEL_ONLY_BIT );
		}
		SETBIT( tSpec.dwFlags, ISF_RESTRICT_AFFIX_LEVEL_CHANGES_BIT );
	}
	const OFFER_DEFINITION *ptOfferDef = OfferGetDefinition( nOffer );
	if ( ptOfferDef && !ptOfferDef->bDoNotIdentify )
		SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );

	SETBIT( tSpec.dwFlags, ISF_USEABLE_BY_ENABLED_PLAYER_CLASS );

	// create the items		
	s_TreasureSpawnClass( pPlayer, nTreasure, &tSpec, &tResult, 0, nLevel );
	ASSERTX_RETFALSE( tResult.nTreasureBufferCount > 0, "No items generated for offer" );
	return tResult.nTreasureBufferCount;

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
// Gets treasure UNITs from offer storage, if any exist.
// Returns number of treasure UNITs found.
//----------------------------------------------------------------------------
int OfferGetExistingItems(
	UNIT *pPlayer,
	int nOffer, 
	ITEMSPAWN_RESULT &tResult,
	int nQuestTaskID /* = INVALID_ID */)
{
	ASSERTX_RETVAL( pPlayer, 0, "Expected player" );
	ASSERTX_RETVAL( nOffer != INVALID_LINK, 0, "Invalid offer" );
	ASSERTX_RETVAL( tResult.nTreasureBufferSize >= MAX_OFFERINGS, 0, "UNIT return buffer should be at least MAX_OFFERINGS in length" );

	tResult.nTreasureBufferCount = 0;

	// iterate *entire* player inventory
	int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );	
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	UNIT *pItem = NULL;
	while ( ( pItem = UnitInventoryLocationIterate( pPlayer, nInventorySlot, pItem, 0, FALSE ) ) != NULL )
	{
		// did this item come from an offer of this type
		if (UnitGetStat( pItem, STATS_OFFERID, nDifficulty ) == nOffer &&
			( nQuestTaskID == INVALID_ID ||
			  UnitGetStat( pItem, STATS_QUEST_TASK_REF ) == nQuestTaskID ) )
		{
			ASSERTX_RETVAL(tResult.nTreasureBufferCount < tResult.nTreasureBufferSize, tResult.nTreasureBufferCount, "Too many offer items to fit in return buffer" );
			tResult.pTreasureBuffer[tResult.nTreasureBufferCount++] = pItem;
		}
	}
	return tResult.nTreasureBufferCount;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDestroyExistingTreasure( 
	UNIT *pPlayer, 
	int nOffer,
	int nQuestTaskID )
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( nOffer != INVALID_LINK, "Invalid offer" );
	
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

	// iterate *entire* player inventory
	UNIT *pNext = NULL;
	UNIT *pItem = UnitInventoryIterate( pPlayer, NULL );
	while (pItem)
	{
	
		// get next
		pNext = UnitInventoryIterate( pPlayer, pItem, 0 );

		// did this item come from an offer of this type
		if (UnitGetStat( pItem, STATS_OFFERID, nDifficulty ) == nOffer &&
			( nQuestTaskID == INVALID_ID ||
			  UnitGetStat( pItem, STATS_QUEST_TASK_REF ) == nQuestTaskID ) )
		{
			//int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );	
			//int nItemLocation;
			//int nItemLocationX;
			//int nItemLocationY;
			//if( UnitGetInventoryLocation( pItem, &nItemLocation, &nItemLocationX, &nItemLocationY ) )
			//{
			//	ASSERTX( nInventorySlot != nItemLocation, "Not in the same location in the inventory" );
			//}
			UnitFree( pItem, UFF_SEND_TO_CLIENTS );
		}
		
		// goto next
		pItem = pNext;
		
	}
			
	// the player has not taken items from this offer
	UnitClearStat( pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, StatParam( STATS_OFFER_NUM_ITEMS_TAKEN, nOffer, nDifficulty ) );	
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
int s_OfferCreateItems(
	UNIT *pPlayer,
	int nOffer,
	ITEMSPAWN_RESULT *ptItems, /* = NULL */
	int nTreasureOverride, /* = INVALID_LINK */ 
	int nQuestTaskID, /* = INVALID_ID */
	int nLevel /*= INVALID_ID */)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player" );
	ASSERTX_RETFALSE( nOffer != INVALID_LINK, "Invalid offer" );

	// what treasure class do we use
	int nTreasureClass = sOfferGetTreasure( nOffer, pPlayer, nTreasureOverride );
	if (nTreasureClass != INVALID_LINK)
	{

		// destroy any existing treasure from an offer of this same type before,
		// this is a safeguard against somehow allowing players to generate items
		// from the same offer multiple times
		sDestroyExistingTreasure( pPlayer, nOffer, nQuestTaskID );

		// setup result buffer
		UNIT *pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tTempResultStorage;
		if ( ptItems == NULL )
		{
			tTempResultStorage.pTreasureBuffer = pItems;
			tTempResultStorage.nTreasureBufferSize = arrsize( pItems );
			ptItems = &tTempResultStorage;
		}

		// create the items		
		s_sOfferCreateTreasureClass( pPlayer, nOffer, *ptItems, nTreasureOverride, nLevel );
		ASSERTX_RETFALSE( ptItems->nTreasureBufferCount > 0, "No items generated for offer" );

		// we put offering items into an inventory slot that is hidden from the player
		int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );	

		// filter offerings
		sFilterOfferings( pPlayer, nOffer, *ptItems );

		int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

		// link all the items to this offer
		for (int i = 0; i < ptItems->nTreasureBufferCount; ++i)
		{
			UNIT *pItem = ptItems->pTreasureBuffer[ i ];

			// keep stat of what offer this was a part of		
			UnitSetStat( pItem, STATS_OFFERID, nDifficulty, nOffer );

			// Set the stat of the Quest task if needed
			if( nQuestTaskID != INVALID_ID )
			{
				UnitSetStat( pItem, STATS_QUEST_TASK_REF, nQuestTaskID );
			}
			// put them in the offer inventory slot
			UnitInventoryAdd(INV_CMD_PICKUP, pPlayer, pItem, nInventorySlot);

			// this item can now be sent to the client
			UnitSetCanSendMessages( pItem, TRUE );

			if( AppIsTugboat() )
			{
				GAMECLIENT *pClient = UnitGetClient( pPlayer );
				s_SendUnitToClient( pItem, pClient );
			}

			// item is in a server only inventory location, transfer to player
			// by moving it to another inventory location
		}

		// valid treasure class
		return TRUE;

	}

	return FALSE;  // invalid treasure class

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_OfferDestroyAndGiveNew(
	UNIT *pPlayer,
	UNIT *pOfferer,
	int nOffer,
	const OFFER_ACTIONS &tActions)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	if (s_OfferCreateItems( pPlayer, nOffer ))
	{
		return s_OfferDisplay( pPlayer, pOfferer, nOffer, tActions ); // ok, display the offering
	}

	return FALSE;  // no items offered
	
#else 
	REF( pPlayer );
	REF( pOfferer );
	REF( nOffer );
	REF( tActions );
	return FALSE;
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_OfferClose(
	UNIT *pPlayer)
{
	// the reward system already does everything we need to, phew! -Colin
	s_RewardClose( pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_OfferCancel(
	UNIT *pPlayer)
{
	// the reward system already does everything we need to, phew! -Colin
	s_RewardCancel( pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_OfferDisplay(
	UNIT *pPlayer,
	UNIT *pOfferer,
	int nOffer,
	const OFFER_ACTIONS &tActions)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( nOffer != INVALID_LINK, "Invalid offer" );
	int nNumOfferings = 0;
	const OFFER_DEFINITION *ptOfferDef = OfferGetDefinition( nOffer );
	if ( ptOfferDef )
	{
		// close any offer that might be going on (just a precaution)
		s_OfferClose( pPlayer );

		// iterate items in the offering storage inventory
		int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );		
		int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
		UNIT *tOfferings[ MAX_OFFERINGS ];
		UNIT * pItem = NULL;
		while ((pItem = UnitInventoryLocationIterate( pPlayer, nInventorySlot, pItem, 0, FALSE )) != NULL)
		{
			if (UnitGetStat( pItem, STATS_OFFERID, nDifficulty ) == nOffer)
			{
				ASSERTX_RETFALSE( nNumOfferings < MAX_OFFERINGS - 1, "Too many offerings" );
				tOfferings[ nNumOfferings++ ] = pItem;	
			}
		}

		// how many items has the player taken from this offer so far
		int nNumTaken = UnitGetStat( pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, nOffer, nDifficulty );

		// open the reward interface
		s_RewardOpen( 
			STYLE_OFFER, 
			pPlayer, 
			pOfferer, 
			tOfferings, 
			nNumOfferings, 
			ptOfferDef->nNumAllowedTakes,
			nNumTaken,
			nOffer, 
			tActions);
	}

	return nNumOfferings;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sOfferItemsExist(
	UNIT *pPlayer,
	int nOffer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( nOffer != INVALID_LINK, "Expected offer link" );
	
	// setup spawn result
	UNIT *tItems[ MAX_OFFERINGS ];
	ITEMSPAWN_RESULT tResult;
	tResult.pTreasureBuffer = tItems;
	tResult.nTreasureBufferSize = arrsize( tItems );

	// get items	
	int nItemCount = OfferGetExistingItems( pPlayer, nOffer, tResult );
	return nItemCount > 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_OfferGive(
	UNIT *pPlayer,
	UNIT *pOfferer,
	int nOffer,
	const OFFER_ACTIONS &tActions)
{
	ASSERTX_RETZERO( pPlayer, "Expected player unit" );
	ASSERTX_RETZERO( pOfferer, "Expected offerer unit" );
	ASSERTX_RETZERO( nOffer != INVALID_LINK, "Expected offer link" );
	
	// do any items for this offer already exist
	BOOL bItemsExist = sOfferItemsExist( pPlayer, nOffer );
	if (bItemsExist == FALSE)
	{
		return s_OfferDestroyAndGiveNew( pPlayer, pOfferer, nOffer, tActions );
	}
	else
	{
		return s_OfferDisplay( pPlayer, pOfferer, nOffer, tActions );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_OfferItemTaken(
	UNIT *pPlayer, 
	UNIT *pItem)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pItem, "Expected item" );

	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	int nOffer = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );	
	int nNumTaken = UnitGetStat( pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, nOffer, nDifficulty );		
	UnitSetStat( pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, nOffer, nDifficulty, nNumTaken + 1);				

	// this item is no longer from this offer
	UnitClearStat( pItem, STATS_OFFERID, nDifficulty );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_OfferIsEmpty(
	UNIT *pPlayer,
	int nOffer,
	int nTreasureOverride /* = INVALID_LINK */ )
{
	// get the treasure class to create items with
	int nTreasure = sOfferGetTreasure(nOffer, pPlayer, nTreasureOverride);
	const TREASURE_DATA* pTreasureData = 
		TreasureGetData(UnitGetGame(pPlayer), nTreasure);
	ASSERTX_RETVAL(pTreasureData, TRUE, "NULL treasure data for treasure index ");
	return s_TreasureClassHasNoTreasure(pTreasureData);
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STAT_VERSION_RESULT s_VersionStatOffer(
	STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit)
{
	int nStat = tStatsFile.tDescription.nStat;

	if(nStatsVersion < 7)
	{
		DWORD dwDifficultyCode;
		STATS_PARAM_DESCRIPTION* pParamDesc;
		STATS_FILE_PARAM_VALUE* pValue;	

		switch(nStat)
		{
		case STATS_OFFERID:
		case STATS_OFFER_NUM_ITEMS_TAKEN:
			pParamDesc = &tStatsFile.tDescription.tParamDescriptions[tStatsFile.tDescription.nParamCount];
			dwDifficultyCode = 
				(nStat == STATS_OFFERID && UnitGetStat( pUnit, STATS_LEVEL ) > 30) ?
					ExcelGetCode( NULL, DATATABLE_DIFFICULTY, DIFFICULTY_NIGHTMARE ) :
					ExcelGetCode( NULL, DATATABLE_DIFFICULTY, DIFFICULTY_NORMAL );
			pValue = &tStatsFile.tParamValue[tStatsFile.tDescription.nParamCount];
			pValue->nValue = dwDifficultyCode;
			pParamDesc->idParamTable = DATATABLE_DIFFICULTY;
			tStatsFile.tDescription.nParamCount++;			
			break;
		
		default:
			break;
		}
		nStatsVersion = 7;
	}

	nStatsVersion = STATS_CURRENT_VERSION;

	return SVR_OK;
	
}