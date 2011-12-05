//----------------------------------------------------------------------------
// FILE: cube.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "excel_private.h"
#include "cube.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "stringtable.h"
#include "picker.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "gameunits.h"
#include "trade.h"
#include "treasure.h"
#include "gameglobals.h"
#include "currency.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "uix_priv.h"
#include "achievements.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_CubeOpen(
	UNIT * pPlayer,
	UNIT * pCube)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pCube, "Expected cube unit" );

	// you cannot open a recipe if you're at a craftsman or trading (unless you're at a merchant, cause
	// we support that UI crazyness)
	UNIT *pTradingWith = TradeGetTradingWith( pPlayer );
	if (pTradingWith && UnitIsMerchant( pTradingWith ) == FALSE)
	{
		return;
	}
		
	// setup message
	MSG_SCMD_CUBE_OPEN tMessage;			
	tMessage.idCube = UnitGetId( pCube );
			
	// send it
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_CUBE_OPEN, &tMessage );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sCreateCubeResult(
	UNIT * pPlayer,
	const RECIPE_DEFINITION * pRecipeDef )
{				
	ASSERTX_RETNULL( pPlayer, "Expected receiver unit" );
	ASSERTX_RETNULL( pRecipeDef, "Expected recipe definition" );
	
	UNIT * pReward = NULL;
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME * pGame = UnitGetGame( pPlayer );

	// create treasure
	int nTreasure = pRecipeDef->nTreasureClassResult[0];
	if ( nTreasure == INVALID_LINK )
		return NULL;

	// setup spawn spec
	ITEM_SPEC tSpec;
	tSpec.unitOperator = pPlayer;
	SETBIT( tSpec.dwFlags, ISF_RESTRICTED_TO_GUID_BIT );
	tSpec.guidRestrictedTo = UnitGetGuid( pPlayer );
	SETBIT( tSpec.dwFlags, ISF_IDENTIFY_BIT );
	if( AppIsTugboat() )
	{
		tSpec.nLevel = MAX( 10, UnitGetStat( pPlayer, STATS_LEVEL ) );
	}
	// setup result data
	const int MAX_RESULT_ITEMS = 1;
	UNIT * pItems[ MAX_RESULT_ITEMS ];
	ITEMSPAWN_RESULT tResult;
	tResult.pTreasureBuffer = pItems;
	tResult.nTreasureBufferSize = arrsize( pItems );


	// create item
	s_TreasureSpawnClass( pPlayer, nTreasure, &tSpec, &tResult );
	if ( tResult.nTreasureBufferCount == 1 )
	{
		pReward = tResult.pTreasureBuffer[ 0 ];
	}
	else
	{
		// we don't support being able to create multiple things
		const TREASURE_DATA * pTreasureData = TreasureGetData( pGame, nTreasure );
		REF( pTreasureData );
		ASSERTV( tResult.nTreasureBufferCount == 0,
			"Cube recipe '%s' creates multiple results using treasure '%s', which is not currently supported",
			pRecipeDef->szName,
			pTreasureData->szTreasure );
		// destroy those items					
		for ( int i = 0; i < tResult.nTreasureBufferCount; i++ )
			UnitFree( tResult.pTreasureBuffer[ i ] );
	}
		
	// send a nice sound since we made something.
	int nCubeSound = GlobalIndexGet( GI_SOUND_CUBE_SUCCESS );	

	// Notify the achivements system
	s_AchievementSendCubeUse(pReward,pPlayer);

	if (nCubeSound != INVALID_LINK)
	{
		MSG_SCMD_UNITPLAYSOUND msg;
		msg.id = UnitGetId(pPlayer);
		msg.idSound = nCubeSound;
		s_SendMessage(pGame, UnitGetClientId(pPlayer), SCMD_UNITPLAYSOUND, &msg);
	}

#endif
	return pReward;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sGiveCubedResult( UNIT * pPlayer, UNIT * pCubedItem )
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( pCubedItem, "Expected item" );

	// find a location for the item	
	BOOL bGiven = FALSE;
	int nX, nY;
	DWORD dwFlags = CLF_ALLOW_NEW_CONTAINER_BIT | CLF_IGNORE_NO_DROP_GRID_BIT | CLF_PICKUP_NOREQ;
	if (UnitInventoryFindSpace( pPlayer, pCubedItem, INVLOC_CUBE, &nX, &nY, dwFlags))
	{
		bGiven = InventoryChangeLocation( pPlayer, pCubedItem, INVLOC_CUBE, nX, nY, dwFlags );
	}

	if ( !bGiven )
	{
		// restrict for this player
		UnitSetRestrictedToGUID( pCubedItem, UnitGetGuid( pPlayer ) );

		// can't find spot in inventory, drop to ground and lock for this player
		// drop item to ground, but lock to this player so nobody can take it
		DROP_RESULT eResult = s_ItemDrop( pPlayer, pCubedItem );
		if ( eResult != DR_OK_DESTROYED )  // this never should be but just in case
		{
			// lock the item to this player
			ItemLockForPlayer( pCubedItem, pPlayer );
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_CubeGetItemsForRecipes( UNIT *pPlayer,
							   const RECIPE_DEFINITION * pRecipeDef,
							   UNIT **Item_List,
							   int nMaxItemList,
							   int &nCount )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)	
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IFF_IN_CUBE_ONLY_BIT );
	BOOL bIsValid( TRUE );
	
	for ( int j = 0; j < nMaxItemList; j++ )
		Item_List[j] = NULL;

	//first lets check to see if there is any none-crafting items inside the list	
	UNIT * item = UnitInventoryIterate( pPlayer, NULL, dwFlags, FALSE );
	while ( item != NULL )
	{
		BOOL bValid( FALSE );
		for ( int j = 0; j < MAX_RECIPE_INGREDIENTS; j++ )
		{
			if ( pRecipeDef->tIngredientDef[j].nItemClass != INVALID_LINK &&
				pRecipeDef->tIngredientDef[j].nItemClass == UnitGetClass( item ) )
				bValid = TRUE;
			if ( pRecipeDef->tIngredientDef[j].nUnitType != INVALID_LINK &&
				 pRecipeDef->tIngredientDef[j].nUnitType == UnitGetType( item ) )
				bValid = TRUE;
			if ( bValid && ( pRecipeDef->tIngredientDef[j].nItemQuality != INVALID_LINK ) && ( pRecipeDef->tIngredientDef[j].nItemQuality != ItemGetQuality( item ) ) )
				bValid = FALSE;
			if( bValid )
				break;
		}
		if( !bValid )
		{
			return FALSE;   //invalid item found. Can't make the recipe
		}
		item = UnitInventoryIterate( pPlayer, item, dwFlags, FALSE );
	}
	//next lets store and make sure we have enough ingredients
	for ( int j = 0; j < MAX_RECIPE_INGREDIENTS; j++ )
	{
		if ( pRecipeDef->tIngredientDef[j].nItemClass != INVALID_LINK ||
			 pRecipeDef->tIngredientDef[j].nUnitType != INVALID_LINK )
		{
				
			int nItemsNeeded = pRecipeDef->tIngredientDef[j].nQuantityMin;
			UNIT * item = UnitInventoryIterate( pPlayer, NULL, dwFlags, FALSE );
			while ( item != NULL )
			{
				BOOL bFound( FALSE );
				if ( pRecipeDef->tIngredientDef[j].nItemClass != INVALID_LINK &&
					pRecipeDef->tIngredientDef[j].nItemClass == UnitGetClass( item ) )
					bFound = TRUE;
				if ( pRecipeDef->tIngredientDef[j].nUnitType != INVALID_LINK &&
					 pRecipeDef->tIngredientDef[j].nUnitType == UnitGetType( item ) )
					bFound = TRUE;
				if ( bFound && ( pRecipeDef->tIngredientDef[j].nItemQuality != INVALID_LINK ) && ( pRecipeDef->tIngredientDef[j].nItemQuality != ItemGetQuality( item ) ) )
					bFound = FALSE;
				// don't find things we already found.
				for( int k = 0; k < nCount; k++ )
				{
					if( Item_List[k] == item )
						bFound = FALSE;
				}
				if ( bFound )
				{
					Item_List[nCount++] = item;		
					nItemsNeeded -= ItemGetQuantity( item );					
				}
				if( nItemsNeeded <= 0 )
					break; //break out of while				
				item = UnitInventoryIterate( pPlayer, item, dwFlags, FALSE );
			}
			if( nItemsNeeded > 0 )
				bIsValid = FALSE;
		}
	}	
	return bIsValid;
#else
	return FALSE;
#endif
	
}
BOOL s_CubeTryCreateAllowOtherIngredients(
	UNIT * pPlayer )
{

	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

#if !ISVERSION(CLIENT_ONLY_VERSION)
// go through all the recipes and check if there is a match
	int nNumRecipes = ExcelGetNumRows( NULL, DATATABLE_RECIPES );
	for (int i = 0; i < nNumRecipes; ++i)
	{
		const RECIPE_DEFINITION * pRecipeDef = RecipeGetDefinition( i );
		if ( pRecipeDef && pRecipeDef->bCube && pRecipeDef->bDontRequireExactIngredients)
		{
			const int MAX_ITEM_COUNT( 100 );
			UNIT * item_list[ MAX_ITEM_COUNT ];
			int nItemsFound( 0 );			
			if( s_CubeGetItemsForRecipes( pPlayer, pRecipeDef, item_list, MAX_ITEM_COUNT, nItemsFound ) )
			{				
				for( int j = 0; j < MAX_RECIPE_INGREDIENTS; j++ )				
				{
					int nCountNeeded = pRecipeDef->tIngredientDef[j].nQuantityMin;
					for( int t = 0; t < nItemsFound; t++ )
					{
						if( item_list[t] == NULL )
							continue; //continue to look for more items
						int nCountOfItems = ItemGetQuantity( item_list[t] );
						if( nCountOfItems == 0 )
						{
							UnitFree( item_list[t], UFF_SEND_TO_CLIENTS );
							continue;
						}						
						int nNumToRemove = (  nCountOfItems > nCountNeeded )?nCountNeeded:nCountOfItems;
						ItemSetQuantity( item_list[t], nCountOfItems - nNumToRemove  );
						if( nNumToRemove == nCountOfItems )
						{
							UnitFree( item_list[t], UFF_SEND_TO_CLIENTS ); //do I need to do this?
							item_list[t] = NULL; //no more items left
						}
						nCountNeeded -= nNumToRemove;
						ASSERT( nCountNeeded >= 0 ); //we should never get below zero here
						if( nCountNeeded <= 0 )
							break;  //we removed all items
					}	
					ASSERT( nCountNeeded == 0 ); //this should always be zero!
				}
				// the cube should now be ready, so let's make the new item and stick it there
				UNIT * pCubedItem = sCreateCubeResult( pPlayer, pRecipeDef );

				// this item is now ready to do network communication
				UnitSetCanSendMessages( pCubedItem, TRUE );

				sGiveCubedResult( pPlayer, pCubedItem );

				if ( UnitGetStat( pPlayer, STATS_CUBE_KNOWN_RECIPES, i ) == 0 )
					UnitSetStat( pPlayer, STATS_CUBE_KNOWN_RECIPES, i, 1 );

				return TRUE; 	
			}
		}
	}

#endif
	return FALSE;
}

void s_CubeTryCreate(
	UNIT * pPlayer )
{


	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

#if !ISVERSION(CLIENT_ONLY_VERSION)
	

	// get count of the items in the player's cube
	int nCubeItemCount = 0;
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IFF_IN_CUBE_ONLY_BIT );
	UNIT * item = UnitInventoryIterate( pPlayer, NULL, dwFlags, FALSE );
	while ( item != NULL )
	{
		nCubeItemCount++;
		item = UnitInventoryIterate( pPlayer, item, dwFlags, FALSE );
	}

	// go through all the recipes and check if there is a match
	int nNumRecipes = ExcelGetNumRows( NULL, DATATABLE_RECIPES );
	for (int i = 0; i < nNumRecipes; ++i)
	{
		const RECIPE_DEFINITION * pRecipeDef = RecipeGetDefinition( i );
		if ( pRecipeDef && pRecipeDef->bCube && !pRecipeDef->bDontRequireExactIngredients)
		{
			// check if this matches the cube contents
			// first get count
			int nRecipeItemCount = 0;
			for ( int j = 0; j < MAX_RECIPE_INGREDIENTS; j++ )
			{
				if ( pRecipeDef->tIngredientDef[j].nItemClass != INVALID_LINK ||
					 pRecipeDef->tIngredientDef[j].nUnitType != INVALID_LINK )
					nRecipeItemCount++;
			}
			UNIT * item_list[ MAX_RECIPE_INGREDIENTS ];

			// are we dealing with the same number of items?
			if ( nRecipeItemCount == nCubeItemCount )
			{
				// now lets check if they are the same ingredients
				BOOL bFound = TRUE;
				//UNIT * item_list[ MAX_RECIPE_INGREDIENTS ];
				for ( int j = 0; j < MAX_RECIPE_INGREDIENTS; j++ )
					item_list[j] = NULL;
				for ( int j = 0; bFound && ( j < MAX_RECIPE_INGREDIENTS ); j++ )
				{
					if ( pRecipeDef->tIngredientDef[j].nItemClass != INVALID_LINK ||
						 pRecipeDef->tIngredientDef[j].nUnitType != INVALID_LINK )
					{
						bFound = FALSE;
						UNIT * item = UnitInventoryIterate( pPlayer, NULL, dwFlags, FALSE );
						while ( !bFound && ( item != NULL ) )
						{

							if ( pRecipeDef->tIngredientDef[j].nItemClass != INVALID_LINK &&
								pRecipeDef->tIngredientDef[j].nItemClass == UnitGetClass( item ) )
								bFound = TRUE;
							if ( pRecipeDef->tIngredientDef[j].nUnitType != INVALID_LINK &&
								 pRecipeDef->tIngredientDef[j].nUnitType == UnitGetType( item ) )
								bFound = TRUE;
							if ( bFound && ( pRecipeDef->tIngredientDef[j].nItemQuality != INVALID_LINK ) && ( pRecipeDef->tIngredientDef[j].nItemQuality != ItemGetQuality( item ) ) )
								bFound = FALSE;
							if ( bFound && ( pRecipeDef->tIngredientDef[j].nQuantityMin > 0 ) &&  ( ItemGetQuantity( item ) < pRecipeDef->tIngredientDef[j].nQuantityMin ) )
								bFound = FALSE;
							// don't find things we already found.
							for( int k = 0; k < j; k++ )
							{
								if( item_list[k] == item )
									bFound = FALSE;
							}
							if ( bFound )
								item_list[j] = item;
							else
								item = UnitInventoryIterate( pPlayer, item, dwFlags, FALSE );
						}
					}
				}
				// if we found everything, make it!
				if ( bFound )
				{
					// remove old items, make new
					for ( int j = 0; j < MAX_RECIPE_INGREDIENTS; j++ )
					{
						if ( item_list[j] != NULL )
						{
							if ( ( pRecipeDef->tIngredientDef[j].nQuantityMin > 0 ) && ( ItemGetQuantity( item_list[j] ) > pRecipeDef->tIngredientDef[j].nQuantityMin ) )
								ItemSetQuantity( item_list[j], ItemGetQuantity( item_list[j] ) - pRecipeDef->tIngredientDef[j].nQuantityMin );
							else
								UnitFree( item_list[j], UFF_SEND_TO_CLIENTS );
						}
					}
					// the cube should now be ready, so let's make the new item and stick it there
					UNIT * pCubedItem = sCreateCubeResult( pPlayer, pRecipeDef );

					// this item is now ready to do network communication
					UnitSetCanSendMessages( pCubedItem, TRUE );

					sGiveCubedResult( pPlayer, pCubedItem );

					if ( UnitGetStat( pPlayer, STATS_CUBE_KNOWN_RECIPES, i ) == 0 )
						UnitSetStat( pPlayer, STATS_CUBE_KNOWN_RECIPES, i, 1 );

					return;
				}
			}
		}
	}
	if( AppIsTugboat() )
	{
		if (s_CubeTryCreateAllowOtherIngredients( pPlayer ))
			return;
	}

	// Well, if we're here then we've failed.  Don't be too ashamed, we tried our best.
	int nCubeSound = GlobalIndexGet( GI_SOUND_CUBE_FAIL );	

	if (nCubeSound != INVALID_LINK)
	{
		GAME * pGame = UnitGetGame( pPlayer );
		MSG_SCMD_UNITPLAYSOUND msg;
		msg.id = UnitGetId(pPlayer);
		msg.idSound = nCubeSound;
		s_SendMessage(pGame, UnitGetClientId(pPlayer), SCMD_UNITPLAYSOUND, &msg);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CubeOpen( 
	UNIT * pCube )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( pCube, "Expected cube" );
		
	// close any previously displayed UI and reset back to starting state
	c_CubeClose();
	
	// open UI if not open already
	TRADE_SPEC tTradeSpec;
	tTradeSpec.eStyle = STYLE_CUBE;		
	UIShowTradeScreen( UnitGetId( pCube ), FALSE, &tTradeSpec );		
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CubeClose()
{
#if !ISVERSION(SERVER_VERSION)
	if( AppIsHellgate() )
	{
		UIComponentActivate(UICOMP_CUBE_PANEL, FALSE);
	}
	else
	{
		UIHidePaneMenu( KPaneMenuCube );
	}
#endif
}	

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeCreate(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	// setup message and send
	MSG_CCMD_CUBE_TRY_CREATE message;
	//message.idRecipeGiver = nRecipeGiver;
	c_SendMessage( CCMD_CUBE_TRY_CREATE, &message );
	
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeCancel(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	c_CubeClose();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeOnChangeIngredients(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	// must be in bounds
	if (!UIComponentGetActive( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// call the default grid on change function
	UIInvGridOnInventoryChange( pComponent, nMessage, wParam, lParam );

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeOnPostInactivate(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeRecipeOpen(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	UI_COMPONENT * pCube = UIComponentGetByEnum( UICOMP_CUBE_PANEL );
	ASSERT_RETVAL( pCube, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT * pRecipePanel = UIComponentFindChildByName( pCube, "cube recipe list" );
	ASSERT_RETVAL( pRecipePanel, UIMSG_RET_NOT_HANDLED );

	UIComponentActivate( pRecipePanel, TRUE );

	UI_COMPONENT * pRecipeOpenButton = UIComponentFindChildByName( pCube, "cube button open recipe" );
	ASSERT_RETVAL( pRecipeOpenButton, UIMSG_RET_NOT_HANDLED );

	UIComponentActivate( pRecipeOpenButton, FALSE );

	UI_COMPONENT * pRecipeCloseButton = UIComponentFindChildByName( pCube, "cube button close recipe" );
	ASSERT_RETVAL( pRecipeCloseButton, UIMSG_RET_NOT_HANDLED );

	UIComponentActivate( pRecipeCloseButton, TRUE );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeRecipeClose(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	UI_COMPONENT * pCube = UIComponentGetByEnum( UICOMP_CUBE_PANEL );
	ASSERT_RETVAL( pCube, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT * pRecipePanel = UIComponentFindChildByName( pCube, "cube recipe list" );
	ASSERT_RETVAL( pRecipePanel, UIMSG_RET_NOT_HANDLED );

	UIComponentActivate( pRecipePanel, FALSE );

	UI_COMPONENT * pRecipeCloseButton = UIComponentFindChildByName( pCube, "cube button close recipe" );
	ASSERT_RETVAL( pRecipeCloseButton, UIMSG_RET_NOT_HANDLED );

	UIComponentActivate( pRecipeCloseButton, FALSE );

	UI_COMPONENT * pRecipeOpenButton = UIComponentFindChildByName( pCube, "cube button open recipe" );
	ASSERT_RETVAL( pRecipeOpenButton, UIMSG_RET_NOT_HANDLED );

	UIComponentActivate( pRecipeOpenButton, TRUE );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeScrollBarOnScroll(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pComponent);
	UIScrollBarSetValue(pScrollBar, pScrollBar->m_ScrollPos.m_fY);

	UI_COMPONENT * pCube = UIComponentGetByEnum( UICOMP_CUBE_PANEL );
	ASSERT_RETVAL( pCube, UIMSG_RET_HANDLED );

	UI_COMPONENT * pNamePanel = UIComponentFindChildByName( pCube, "name column" );
	ASSERT_RETVAL( pNamePanel, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT * pIngredientPanel = UIComponentFindChildByName( pCube, "ingredient column" );
	ASSERT_RETVAL( pIngredientPanel, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT * pHighlightPanel = UIComponentFindChildByName( pCube, "highlight panel" );
	ASSERT_RETVAL( pHighlightPanel, UIMSG_RET_NOT_HANDLED );


	pNamePanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
	pIngredientPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
	pHighlightPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;

	UISetNeedToRender( pCube );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeScrollBarOnMouseWheel(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam )
{
	ASSERT_RETVAL( pComponent, UIMSG_RET_NOT_HANDLED );
	if ( !UIComponentGetActive( pComponent ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if ( !UICursorGetActive() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar( pComponent );

	pScrollBar->m_ScrollPos.m_fY += (int)lParam > 0 ? -pScrollBar->m_fWheelScrollIncrement : pScrollBar->m_fWheelScrollIncrement;
	pScrollBar->m_ScrollPos.m_fY = PIN(pScrollBar->m_ScrollPos.m_fY, pScrollBar->m_fMin, pScrollBar->m_fMax);

	UIComponentHandleUIMessage( pScrollBar, UIMSG_SCROLL, 0, lParam );

	UISetNeedToRender( pScrollBar );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetNumCubeRecipes()
{
	// go through all the recipes and check if there is a match
	int nNumRecipes = 0;
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_RECIPES );
	for ( int i = 0; i < nNumRows; i++ )
	{
		const RECIPE_DEFINITION * pRecipeDef = RecipeGetDefinition( i );
		if ( pRecipeDef && pRecipeDef->bCube )
			nNumRecipes++;
	}
	return nNumRecipes;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_CubeGetResultString(
	UNIT * pPlayer,
	const RECIPE_DEFINITION * pRecipeDef,
	WCHAR * szBuf,
	int	nBufLen )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitIsPlayer( pPlayer ) );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETFALSE( pRecipeDef );

	szBuf[0] = 0;

	const TREASURE_DATA * pTreasure = TreasureGetData( pGame, pRecipeDef->nTreasureClassResult[0] );
	ASSERT_RETFALSE( pTreasure );
	ASSERT_RETFALSE( pTreasure->pPicks[0].eCategory == TC_CATEGORY_ITEMCLASS );
	ASSERT_RETFALSE( pTreasure->pPicks[0].nIndex[0] != INVALID_LINK );
	const UNIT_DATA * pItemData = ItemGetData( pTreasure->pPicks[0].nIndex[0] );
	ASSERT_RETFALSE( pItemData );
	PStrCopy( szBuf, StringTableGetStringByIndex( pItemData->nString ), nBufLen );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_CubeGetIngredientString(
	UNIT * pPlayer,
	const RECIPE_DEFINITION * pRecipeDef,
	WCHAR * szBuf,
	int	nBufLen )
{
	ASSERT_RETFALSE( pPlayer );
	ASSERT_RETFALSE( UnitIsPlayer( pPlayer ) );
	ASSERT_RETFALSE( pRecipeDef );

	szBuf[0] = 0;
	BOOL bComma = FALSE;
	for ( int i = 0; i < MAX_RECIPE_INGREDIENTS; i++ )
	{
		if ( pRecipeDef->tIngredientDef[i].nItemClass != INVALID_LINK )
		{
			if ( bComma )
				PStrCat( szBuf, L", ", nBufLen );
			const UNIT_DATA * pItemData = ItemGetData( pRecipeDef->tIngredientDef[i].nItemClass );
			ASSERT_CONTINUE( pItemData );
			PStrCat( szBuf, StringTableGetStringByIndex( pItemData->nString ), nBufLen );
			if ( pRecipeDef->tIngredientDef[i].nQuantityMin > 1 )
			{
				WCHAR szTemp[16];
				PStrPrintf( szTemp, 16, L"(%i)", pRecipeDef->tIngredientDef[i].nQuantityMin );
				PStrCat( szBuf, szTemp, nBufLen );
			}
			bComma = TRUE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRecipieIsKnown(
	UNIT * pUnit,
	int nRecipieIndex,
	const RECIPE_DEFINITION * pRecipeDef )
{
	ASSERT_RETFALSE( pUnit );
	ASSERT_RETFALSE( pRecipeDef );
	if ( pRecipeDef->bAlwaysKnown )
		return TRUE;
	if ( UnitGetStat( pUnit, STATS_CUBE_KNOWN_RECIPES, nRecipieIndex ) )
		return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeRecipeOnPaint(
	UI_COMPONENT * pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam )
{
	UIComponentOnPaint( pComponent, msg, wParam, lParam );

	UNIT * pPlayer = UIGetControlUnit();
	if ( !pPlayer )
		return UIMSG_RET_HANDLED;

	//TIMER_START_NEEDED("Cube panel paint");

	static const float scfBordersize = 5.0f;

	UI_COMPONENT * pNamePanel = UIComponentFindChildByName( pComponent, "name column" );
	ASSERT_RETVAL( pNamePanel, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT * pIngredientPanel = UIComponentFindChildByName( pComponent, "ingredient column" );
	ASSERT_RETVAL( pIngredientPanel, UIMSG_RET_NOT_HANDLED );

	UI_COMPONENT * pComp = UIComponentFindChildByName( pComponent, "cube list scrollbar" );
	ASSERT_RETVAL( pComp, UIMSG_RET_NOT_HANDLED );
	UI_SCROLLBAR * pScrollbar = UICastToScrollBar(pComp);

	UIComponentRemoveAllElements( pNamePanel );
	UIComponentRemoveAllElements( pIngredientPanel );

	UI_RECT rectNamePanel = UIComponentGetRect( pNamePanel );
	UI_RECT rectIngredientPanel = UIComponentGetRect( pIngredientPanel );

	UI_RECT rectNamePanelFull =	rectNamePanel;
	UI_RECT rectIngredientPanelFull = rectIngredientPanel;

	rectNamePanel.m_fX1 += scfBordersize;
	rectNamePanel.m_fX2 -= scfBordersize;
	rectIngredientPanel.m_fX1 += scfBordersize;
	rectIngredientPanel.m_fX2 -= scfBordersize;

	UIX_TEXTURE_FONT * pFont = UIComponentGetFont(pComponent);
	int nFontSize = UIComponentGetFontSize(pComponent);
	WCHAR szText[256];
	UI_ELEMENT_TEXT * pElement = NULL;
	float fY = 0.0f;
	DWORD dwColor = GFXCOLOR_GRAY;
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_RECIPES );
	for ( int i = 0; i < nNumRows; i++ )
	{
		const RECIPE_DEFINITION * pRecipeDef = RecipeGetDefinition( i );
		if ( pRecipeDef && pRecipeDef->bCube )
		{
			if ( sRecipieIsKnown( pPlayer, i, pRecipeDef ) )
			{
				// display "known" recipe
				PStrCopy( szText, L"", arrsize(szText) );

				// Title of Achievement
				if ( c_CubeGetResultString( pPlayer, pRecipeDef, szText, arrsize(szText) ) && szText[0] )
				{
					pElement = UIComponentAddTextElement( pNamePanel, NULL, pFont, nFontSize, szText, UI_POSITION( scfBordersize, fY ), dwColor, &rectNamePanel, UIALIGN_TOPLEFT, &UI_SIZE( rectNamePanel.Width(), (float)nFontSize + 1.0f ) );
				}

				// Conditions of Achievement
				if ( c_CubeGetIngredientString( pPlayer, pRecipeDef, szText, arrsize(szText) ) && szText[0] )
				{
					pElement = UIComponentAddTextElement( pIngredientPanel, NULL, pFont, nFontSize, szText, UI_POSITION( scfBordersize, fY ), dwColor, &rectIngredientPanel, UIALIGN_TOPLEFT, &UI_SIZE( rectIngredientPanel.Width(), (float)nFontSize + 1.0f ) );
				}
			}
			else
			{
				// display hidden recipe
				PStrCopy( szText, L"?", arrsize(szText) );
				UIComponentAddTextElement( pNamePanel, NULL, pFont, nFontSize, szText, UI_POSITION( scfBordersize, fY ), dwColor, &rectNamePanel, UIALIGN_TOPLEFT, &UI_SIZE( rectNamePanel.Width(), (float)nFontSize + 1.0f ) );
				UIComponentAddTextElement( pIngredientPanel, NULL, pFont, nFontSize, szText, UI_POSITION( scfBordersize, fY ), dwColor, &rectIngredientPanel, UIALIGN_TOPLEFT, &UI_SIZE( rectIngredientPanel.Width(), (float)nFontSize + 1.0f ) );
			}

			fY += (float)nFontSize + 1.0f;
		}
	}

	if (pNamePanel->m_pFrame)
	{
		UIComponentAddElement(pNamePanel, UIComponentGetTexture(pNamePanel), pNamePanel->m_pFrame, UI_POSITION(), pNamePanel->m_dwColor, &rectNamePanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pNamePanel->m_fWidth, fY));
	}
	if (pIngredientPanel->m_pFrame)
	{
		UIComponentAddElement(pIngredientPanel, UIComponentGetTexture(pIngredientPanel), pIngredientPanel->m_pFrame, UI_POSITION(), pIngredientPanel->m_dwColor, &rectIngredientPanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pIngredientPanel->m_fWidth, fY));
	}

	pScrollbar->m_fMax = MAX(0.0f, fY - pNamePanel->m_fHeight);
	pScrollbar->m_fScrollVertMax = pScrollbar->m_fMax;

	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCubeSetSelected(
	UI_COMPONENT * pComponent,
	int nSelected,
	float fY )
{
	GAME * pGame = AppGetCltGame();
	if ( !pGame )
		return;
	UNIT * pPlayer = GameGetControlUnit( pGame );
	if ( !pPlayer )
		return;

	UI_COMPONENT * pHighlightPanel = UIComponentFindChildByName( pComponent, "highlight panel" );
	ASSERT_RETURN( pHighlightPanel );

	UI_COMPONENT * pNamePanel = UIComponentFindChildByName( pComponent, "name column" );
	ASSERT_RETURN( pNamePanel );

	pHighlightPanel->m_Position.m_fY = ( pNamePanel->m_Position.m_fY + fY ) - pHighlightPanel->m_pParent->m_Position.m_fY; 
	pComponent->m_dwData = (DWORD)nSelected;

	UIComponentSetVisible( pHighlightPanel, TRUE );

	c_CubeDisplayUnitCreate( nSelected );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICubeColumnOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL( pComponent, UIMSG_RET_NOT_HANDLED );
	ASSERT_RETVAL( pComponent->m_pParent, UIMSG_RET_NOT_HANDLED );

	float x = 0.0f;
	float y = 0.0f;
	UIGetCursorPosition(&x, &y);

	UI_POSITION pos = UIComponentGetAbsoluteLogPosition( pComponent );
	y += UIComponentGetScrollPos( pComponent ).m_fY;

	x -= pos.m_fX;
	y -= pos.m_fY;

	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX( pComponent->m_bNoScale );
	float logy = y / UIGetScreenToLogRatioY( pComponent->m_bNoScale );

	UI_GFXELEMENT *pChild = pComponent->m_pGfxElementFirst;
	int nSelected = 0;
	while(pChild)
	{
		if ( UIElementCheckBounds( pChild, logx, logy ) )
		{
			sCubeSetSelected( pComponent->m_pParent, nSelected, pChild->m_Position.m_fY );
			return UIMSG_RET_HANDLED_END_PROCESS;
		}
		pChild = pChild->m_pNextElement;
		nSelected++;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CubeDisplayUnitFree()
{
	if ( g_UI.m_idCubeDisplayUnit == INVALID_ID )
		return;

	GAME * pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );
	UNIT * pUnit = UnitGetById( pGame, g_UI.m_idCubeDisplayUnit );
	UnitFree( pUnit );
	g_UI.m_idCubeDisplayUnit = INVALID_ID;

	UI_COMPONENT * pCube = UIComponentGetByEnum( UICOMP_CUBE_PANEL );
	ASSERT_RETURN( pCube );

	UI_COMPONENT * pCompResultPicture = UIComponentFindChildByName( pCube, "cube result picture" );
	ASSERTX_RETURN( pCompResultPicture, "Expected result picture component" );
	UIComponentSetFocusUnit( pCompResultPicture, INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCubeGetItemClassFromSelected( int selected )
{
	int nNumRows = ExcelGetNumRows( NULL, DATATABLE_RECIPES );
	int nCount = 0;
	for ( int i = 0; i < nNumRows; i++ )
	{
		const RECIPE_DEFINITION * pRecipeDef = RecipeGetDefinition( i );
		if ( pRecipeDef && pRecipeDef->bCube )
		{
			if ( nCount == selected )
			{
				GAME * pGame = AppGetCltGame();
				ASSERT_RETINVALID( pGame );
				UNIT * pPlayer = GameGetControlUnit( pGame );
				ASSERT_RETINVALID( pPlayer );
				if ( !sRecipieIsKnown( pPlayer, i, pRecipeDef ) )
					return INVALID_ID;

				const TREASURE_DATA * pTreasure = TreasureGetData( pGame, pRecipeDef->nTreasureClassResult[0] );
				ASSERT_RETINVALID( pTreasure );
				ASSERT_RETINVALID( pTreasure->pPicks[0].eCategory == TC_CATEGORY_ITEMCLASS );
				ASSERT_RETINVALID( pTreasure->pPicks[0].nIndex[0] != INVALID_LINK );
				return pTreasure->pPicks[0].nIndex[0];
			}
			nCount++;
		}
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CubeDisplayUnitCreate( int recipe_index )
{
	c_CubeDisplayUnitFree();
	ASSERT_RETURN( g_UI.m_idCubeDisplayUnit == INVALID_ID );

	UI_COMPONENT * pCube = UIComponentGetByEnum( UICOMP_CUBE_PANEL );
	ASSERT_RETURN( pCube );

	UI_COMPONENT * pCompResultPicture = UIComponentFindChildByName( pCube, "cube result picture" );
	ASSERTX_RETURN( pCompResultPicture, "Expected result picture component" );

	ITEM_SPEC tSpec;
	tSpec.nItemClass = sCubeGetItemClassFromSelected( recipe_index );
	if ( tSpec.nItemClass == INVALID_LINK )
		return;

	tSpec.eLoadType = UDLT_INVENTORY;

	UNIT * pUnit = c_ItemSpawnForDisplay( AppGetCltGame(), tSpec );
	ASSERT_RETURN( pUnit );

	g_UI.m_idCubeDisplayUnit = UnitGetId( pUnit );
	UIComponentSetFocusUnit( pCompResultPicture, g_UI.m_idCubeDisplayUnit );
}

#endif //!ISVERSION(SERVER_VERSION)
