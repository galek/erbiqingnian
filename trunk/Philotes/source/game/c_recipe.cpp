#include "stdafx.h"
#include "c_recipe.h"
#include "excel.h"
#include "inventory.h"
#include "items.h"
#include "unit_priv.h"
#include "globalindex.h"
#include "monsters.h"
#include "stringtable.h"
#include "level.h"
#include "s_message.h"
#include "c_message.h"
#include "units.h"
#include "npc.h"
#include "c_sound.h"
#include "uix.h"
#include "states.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "recipe.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "fontcolor.h"
#include "dictionary.h"
#include "c_font.h"
#include "trade.h"
#include "uix_priv.h"
#include "stringreplacement.h"
#include "c_ui.h"
#include "uix_crafting.h"
#include "Economy.h"
#include "excel_common.h"
#include "uix_crafting.h"

#if !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------
#define UIRECIPE_PROPERIES_TRACK_COUNT		1000
static int	s_iUIRecipeIndexs[ UIRECIPE_PROPERIES_TRACK_COUNT ];
static int	s_iUIRecipeProperties[UIRECIPE_PROPERIES_TRACK_COUNT][ NUM_RECIPE_PROP ];
static BOOL	s_bUIRecipePropertiesNeedInit( TRUE );
//----------------------------------------------------------------------------
enum RECIPE_CONSTANTS
{
	RECIPE_UI_PAGE_SIZE = 8,  // max # of recipes the UI can support shown at once
};

//----------------------------------------------------------------------------
struct RECIPE
{
	int nRecipe;										// link to excel row
	int nIndex;											// index of recipe in RECIPE_GLOBALS.tRecipe[]
	const RECIPE_DEFINITION *pRecipeDef;				// pointer to def data
	UNITID idRecipeGiver;								// giver
	UNITID idIngredients[ MAX_RECIPE_INGREDIENTS ];		// recipe ingredients (including quantities)
	UNITID idResults[ MAX_RECIPE_RESULTS ];				// recipe results
};

//----------------------------------------------------------------------------
struct RECIPE_GLOBALS
{
	int nRecipeList;				// the recipe list we currently have access to in the UI
	int nRecipeSingleUse;			// single use recipe we're looking at (instead of a recipe list)	
	UNITID idRecipeGiver;			// who offered the recipe or recipe list to us		
	RECIPE tRecipe[ MAX_RECIPES ];	// the recipes available
	int nSelectedRecipeIndex;		// index in tRecipe of recipe that is currently selected 		
	BOOL bSendCloseToServer;
};

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------
//clears the recipe properties
inline void sRecipeClearProperties()
{
	if( s_bUIRecipePropertiesNeedInit )
	{
		s_bUIRecipePropertiesNeedInit = FALSE;
		ZeroMemory( s_iUIRecipeProperties, sizeof( int ) * UIRECIPE_PROPERIES_TRACK_COUNT * NUM_RECIPE_PROP );
		ZeroMemory( s_iUIRecipeIndexs, sizeof( int ) * UIRECIPE_PROPERIES_TRACK_COUNT );
	}
}
//returns the index of the recipe properties
inline int sRecipeGetPropertyIndex( int nRecipeID )
{
	sRecipeClearProperties();
	static int sLastRecipeID( -1 );
	static int sLastRecipeIndex( -1 );
	if( sLastRecipeID == nRecipeID &&
		sLastRecipeIndex != -1 )
	{
		return sLastRecipeIndex;
	}
	
	for( int t = 0; t < UIRECIPE_PROPERIES_TRACK_COUNT; t++ )
	{
		if( s_iUIRecipeIndexs[ t ] == nRecipeID ||
			s_iUIRecipeIndexs[ t ] == 0 )
		{
			s_iUIRecipeIndexs[ t ] = nRecipeID;
			sLastRecipeID = nRecipeID;
			sLastRecipeIndex = t;
			return t;
		}
	}
	//this is basically saying if a player has over UIRECIPE_PROPERIES_TRACK_COUNT recipes they HAVE modified then clear all saved default values and
	//start anew
	s_bUIRecipePropertiesNeedInit = TRUE;	
	sRecipeClearProperties();
	s_iUIRecipeIndexs[ 0 ] = nRecipeID;
	sLastRecipeID = nRecipeID;
	sLastRecipeIndex = 0;
	return 0;
}

//sets a recipe property
inline void sRecipeSetPropertyByRecipe( int nRecipeID,
									    int nPropertyID,
										int nPropertyValue )
{
	ASSERT_RETURN( nPropertyID >= 0 && nPropertyID < NUM_RECIPE_PROP );
	int nRecipeIndex = sRecipeGetPropertyIndex( nRecipeID );
	s_iUIRecipeProperties[nRecipeIndex][ nPropertyID ] = nPropertyValue;
}

//returns a recipe property
inline int sRecipeGetPropertyByRecipe( int nRecipeID,
									   int nPropertyID )
{
	ASSERT_RETZERO( nPropertyID >= 0 && nPropertyID < NUM_RECIPE_PROP );
	int nRecipeIndex = sRecipeGetPropertyIndex( nRecipeID );
	return s_iUIRecipeProperties[nRecipeIndex][ nPropertyID ];
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static RECIPE_GLOBALS *sGetRecipeGlobals(
	void)
{
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( pGame->m_pClientRecipeGlobals, "Expected client recipe globals" );
	return pGame->m_pClientRecipeGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeInit( 
	RECIPE &tRecipe)
{
	tRecipe.nRecipe = INVALID_LINK;
	tRecipe.idRecipeGiver = INVALID_ID;
	tRecipe.nIndex = INVALID_INDEX;
	
	// clear ingredients
	for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
	{
		tRecipe.idIngredients[ i ] = INVALID_ID;
	}
	
	// clear results
	for (int i = 0; i < MAX_RECIPE_RESULTS; ++i)
	{
		tRecipe.idResults[ i ] = INVALID_ID;	
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeGlobalsInit(
	RECIPE_GLOBALS &tGlobals)
{
	tGlobals.idRecipeGiver = INVALID_ID;
	tGlobals.nRecipeList = INVALID_LINK;
	tGlobals.nRecipeSingleUse = INVALID_LINK;
	tGlobals.nSelectedRecipeIndex = NONE;
	tGlobals.bSendCloseToServer = TRUE;
	for (int i = 0; i < MAX_RECIPES; ++i)
	{
		RECIPE *pRecipe = &tGlobals.tRecipe[ i ];
		sRecipeInit( *pRecipe );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static RECIPE *sGetSelectedRecipe(
	void)
{
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETNULL( pGlobals, "Expected globals" );
	if (pGlobals->nSelectedRecipeIndex != NONE)
	{
		return &pGlobals->tRecipe[ pGlobals->nSelectedRecipeIndex ];
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeInitForGame(
	GAME *pGame)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	
	if (IS_CLIENT( pGame ))
	{
		ASSERTX_RETURN( pGame->m_pClientRecipeGlobals == NULL, "Expected NULL client recipe globals" );

		// allocate globals
		RECIPE_GLOBALS *pGlobals = (RECIPE_GLOBALS *)GMALLOCZ( pGame, sizeof( RECIPE_GLOBALS ) );
		sRecipeGlobalsInit( *pGlobals );
		
		// tie to game
		pGame->m_pClientRecipeGlobals = pGlobals;

	}
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeCloseForGame(
	GAME *pGame)
{
	ASSERTX_RETURN( pGame, "Expected game" );

	if (IS_CLIENT( pGame ))
	{

		ASSERTX_RETURN( AppIsHammer() || pGame->m_pClientRecipeGlobals, "Expected client recipe globals" );
		if (pGame->m_pClientRecipeGlobals)
		{
			GFREE( pGame, pGame->m_pClientRecipeGlobals );
			pGame->m_pClientRecipeGlobals = NULL;
		}
			
	}
	
}	
//----------------------------------------------------------------------------
//only on the client...should be fine
//----------------------------------------------------------------------------
static int s_iRecipeLastCreated( -1 );	//used for the last recipe properties( props being a percent invested in )
void c_RecipeClearLastRecipeCreated()
{
	s_iRecipeLastCreated = -1;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeTryCreate(
    UNITID nRecipeGiver,
	int nRecipe,
	int nIndex)
{

	if( AppIsTugboat() )
	{
		//this will send any data the player has modified on the recipe if it doesn't match 
		//the last item created.
		BOOL bSendData( FALSE );		
		static int s_iRecipeProperties[ NUM_RECIPE_PROP ];
		if( s_iRecipeLastCreated == -1 )
		{
			ZeroMemory( s_iRecipeProperties, sizeof( int ) * NUM_RECIPE_PROP );
		}
		//if the recipes don't match send the data
		bSendData = ( nRecipe != s_iRecipeLastCreated );
		//if the recipes match, do all the properties still match????
		if( !bSendData )
		{			
			for( int t = 0; t < NUM_RECIPE_PROP; t++ )
			{
				int nValue = sRecipeGetPropertyByRecipe( nRecipe, t );
				if( nValue != s_iRecipeProperties[ t ] )
				{
					bSendData = TRUE;	//nope the player changed a property on the recipe
					break;
				}
			}
		}

		if( bSendData )	//something changed so lets send that data again..
		{
			s_iRecipeLastCreated = nRecipe;
			//first we have to tell the system to clear all player info
			MSG_CCMD_RECIPE_CLEAR_PLAYER_MODIFICATIONS messageClear;
			c_SendMessage( CCMD_RECIPE_CLEAR_PLAYER_MODIFICATIONS, &messageClear );	
			//next we need to send all the data for the properties the player has changed
			for( int t = 0; t < NUM_RECIPE_PROP; t++ )
			{				
				int nValue = sRecipeGetPropertyByRecipe( nRecipe, t );
				s_iRecipeProperties[ t ] = nValue;
				if( nValue > 0 )
				{
					MSG_CCMD_RECIPE_SET_PLAYER_MODIFICATIONS message;	
					message.nRecipeProperty = t;
					message.nRecipePropertyValue = nValue;
					c_SendMessage( CCMD_RECIPE_SET_PLAYER_MODIFICATIONS, &message );	
				}
			}
		}
	}

	// setup message and send
	MSG_CCMD_RECIPE_TRY_CREATE message;
	message.idRecipeGiver = nRecipeGiver;
	message.nRecipe = nRecipe;
	message.nIndex = nIndex;
	c_SendMessage( CCMD_RECIPE_TRY_CREATE, &message );

//	UIHideNPCDialog();
	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeTryBuy(
	UNITID nRecipeGiver,
	int nRecipe)
{

	// setup message and send
	MSG_CCMD_RECIPE_TRY_BUY message;
	message.idRecipeGiver = nRecipeGiver;
	message.nRecipe = nRecipe;
	c_SendMessage( CCMD_RECIPE_TRY_BUY, &message );

	//	UIHideNPCDialog();

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFindItemsForRecipe(
	UNIT *pUnit,
	int nRecipe,
	int nIndexInRecipeList,
	UNITID idGiver,
	int nInvLoc,
	UNITID *pidBuffer,
	int nBufferSize)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe" );
	ASSERTX_RETFALSE( idGiver != INVALID_LINK, "Expected recipe giver id" );
	ASSERTX_RETFALSE( nInvLoc != INVLOC_NONE, "Expected inventory location" );
	ASSERTX_RETFALSE( pidBuffer, "Expected unit id buffer" );
	ASSERTX_RETFALSE( nBufferSize > 0, "Invalid unit id buffer size" );
	GAME *pGame = UnitGetGame( pUnit );
	UNIT *pRecipeGiver = UnitGetById( pGame, idGiver );
	ASSERTX_RETFALSE( pRecipeGiver, "Expected recipe giver unit" );
	
	BOOL bFoundItems = FALSE;
	int nNumItems = 0;
	UNIT* pItem = NULL;
	while ((pItem = UnitInventoryIterate( pUnit, pItem )) != NULL)
	{
		INVENTORY_LOCATION tLocation;
		UnitGetInventoryLocation( pItem, &tLocation );
		if (tLocation.nInvLocation == nInvLoc)
		{
		
			// what recipe is this item a component of
			if (RecipeItemIsComponent( pItem, nRecipe, nIndexInRecipeList, pRecipeGiver ))
			{
						
				// add this item to our recipe instance
				ASSERTX_RETFALSE( nNumItems < nBufferSize, "Too many recipe components" );
				pidBuffer[ nNumItems++ ] = UnitGetId( pItem );
				bFoundItems = TRUE;
				
			}

		}
				
	}

	return bFoundItems;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRecipeSetup( 
	RECIPE &tRecipe,
	int nRecipe,
	UNITID idGiver,
	int nIndex)
{
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Invalid recipe link" );
	ASSERTX_RETFALSE( nIndex != INVALID_INDEX, "Invalid recipe index" );
	ASSERTX_RETFALSE( idGiver != INVALID_ID, "Expected giver unit id" );
	GAME *pGame = AppGetCltGame();
	ASSERTX_RETFALSE( pGame, "Expected game" );	
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );

	// initialize this recipe
	sRecipeInit( tRecipe );

	//  save recipe
	tRecipe.nRecipe = nRecipe;
	tRecipe.nIndex = nIndex;
	tRecipe.pRecipeDef = RecipeGetDefinition( nRecipe );
	tRecipe.idRecipeGiver = idGiver;

	// get unit that holds components for this recipe
	UNIT *pGiver = UnitGetById( pGame, idGiver );
	UNIT *pUnitWithComponents = RecipeGetComponentUnit( pPlayer, pGiver );
		
	// find all recipe ingredients from the inventory slot holding all recipe ingredients
	int nAllRecipesIngredientsLoc = RecipeGetIngredientInvLoc( pUnitWithComponents );
	BOOL bFoundIngredients = sFindItemsForRecipe( 
		pUnitWithComponents, 
		nRecipe, 
		nIndex,
		idGiver,
		nAllRecipesIngredientsLoc, 
		tRecipe.idIngredients, 
		MAX_RECIPE_INGREDIENTS);

	// find all recipe results from the inventory slot holding them
	int nAllRecipesResltsLoc = RecipeGetResultsInvLoc( pUnitWithComponents );
	BOOL bFoundResults = sFindItemsForRecipe( 
		pUnitWithComponents, 
		nRecipe, 
		nIndex,
		idGiver,
		nAllRecipesResltsLoc, 
		tRecipe.idResults, 
		MAX_RECIPE_RESULTS);

	return bFoundIngredients && bFoundResults;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeGetComponentString( 
	UNIT *pPlayer,
	RECIPE *pRecipe,
	UNITID *idComponents,
	int nMaxComponents,
	WCHAR *puszBuffer, 
	int nMaxBuffer,
	BOOL bDoCompleteColoring)
{	
	ASSERTX_RETURN( pRecipe, "Expected recipe instance" );
	ASSERTX_RETURN( puszBuffer, "Expected string buffer" );
	ASSERTX_RETURN( nMaxBuffer > 0, "Invalid buffer size" );
	GAME *pGame = AppGetCltGame();

	// init buffer
	puszBuffer[ 0 ] = 0;

	// can the player complete this recipe ... check all items on person as well as in the ingredient pot
	BOOL bCanComplete = RecipeIsComplete( 
		pPlayer, 
		pRecipe->nRecipe, 
		pRecipe->nIndex, 
		pRecipe->idRecipeGiver, 
		TRUE);
	
	// set color of text based on can complete
	if (bDoCompleteColoring == TRUE)
	{
		int nColor = bCanComplete ? FONTCOLOR_RECIPE_COMPLETE : FONTCOLOR_RECIPE_NOT_COMPLETE;
		UIAddColorToString( puszBuffer, (FONTCOLOR)nColor, nMaxBuffer );
	}
				
	// for now, just concat all the result names together, maybe never change it? :)
	BOOL bFirstItem = TRUE;
	for (int i = 0; i < nMaxComponents; ++i)
	{
		UNITID idComponent = idComponents[ i ];
		if (idComponent != INVALID_ID)
		{
			UNIT *pComponent = UnitGetById( pGame, idComponent );
			if (pComponent)
			{
			
				// add separator if necessary
				if (bFirstItem == FALSE)
				{
					const WCHAR *puszSep = GlobalStringGet( GS_ITEM_LIST_SEPARATOR );
					PStrCat( puszBuffer, puszSep, nMaxBuffer );
				}
				
				// add name
				WCHAR uszName[ MAX_ITEM_NAME_STRING_LENGTH ];
				ItemGetName( pComponent, uszName, MAX_ITEM_NAME_STRING_LENGTH, 0 );
				PStrCat( puszBuffer, uszName, nMaxBuffer );
				
				// an item has been added to the string				
				bFirstItem = FALSE;
				
			}
			
		}
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeGetResultString( 
	UNIT *pPlayer,
	RECIPE *pRecipe,
	WCHAR *puszBuffer, 
	int nMaxBuffer,
	BOOL bDoCompleteColoring)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( pRecipe, "Expected recipe instance" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nMaxBuffer > 0, "Invalid buffer size" );	
	GAME *pGame = UnitGetGame( pPlayer );
	UNITID idRecipeGiver = pRecipe->idRecipeGiver;

	// init buffer
	puszBuffer[ 0 ] = 0;
	
	UNIT *pRecipeGiver = UnitGetById( pGame, idRecipeGiver );
	if (pRecipeGiver && RecipeGiverInstancesComponents( pRecipeGiver ))
	{
		sRecipeGetComponentString( 
			pPlayer, 
			pRecipe, 
			pRecipe->idResults, 
			MAX_RECIPE_RESULTS,
			puszBuffer,
			nMaxBuffer,
			bDoCompleteColoring);
	}
	else
	{
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeGetIngredientString( 
	UNIT *pPlayer,
	RECIPE *pRecipe,
	WCHAR *puszBuffer, 
	int nMaxBuffer,
	BOOL bDoCompleteColoring)
{
	ASSERTX_RETURN( pPlayer, "Expected player unit" );
	ASSERTX_RETURN( pRecipe, "Expected recipe instance" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nMaxBuffer > 0, "Invalid buffer size" );	
	GAME *pGame = UnitGetGame( pPlayer );
	UNITID idRecipeGiver = pRecipe->idRecipeGiver;

	// init buffer
	puszBuffer[ 0 ] = 0;

	UNIT *pRecipeGiver = UnitGetById( pGame, idRecipeGiver );
	if (pRecipeGiver && RecipeGiverInstancesComponents( pRecipeGiver ))
	{
		sRecipeGetComponentString( 
			pPlayer, 
			pRecipe, 
			pRecipe->idIngredients, 
			MAX_RECIPE_INGREDIENTS,
			puszBuffer,
			nMaxBuffer,
			bDoCompleteColoring);
	}
	else
	{
	}
	
}

//----------------------------------------------------------------------------
struct RECIPE_LIST_LOOKUP
{
	UI_COMPONENT_ENUM eCompRecipe;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRecipeListUpdateUI(
	void)
{
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );		
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = UIGetControlUnit();
	
	UI_COMPONENT *pRecipeListPanel = UIComponentGetByEnum( UICOMP_RECIPE_LIST_PANEL );
	if( pRecipeListPanel )
	{

		// setup table with all components we have
		RECIPE_LIST_LOOKUP tRecipeListCompLookup[] = 
		{
			{ UICOMP_RECIPE_1 },
			{ UICOMP_RECIPE_2 },
			{ UICOMP_RECIPE_3 },
			{ UICOMP_RECIPE_4 },
			{ UICOMP_RECIPE_5 },
			{ UICOMP_RECIPE_6 },
			{ UICOMP_RECIPE_7 },
			{ UICOMP_RECIPE_8 },
			{ UICOMP_INVALID }
		};
		
		// first hide all recipe entries
		for (int i = 0; tRecipeListCompLookup[ i ].eCompRecipe != UICOMP_INVALID; ++i)
		{
			const RECIPE_LIST_LOOKUP *pLookup = &tRecipeListCompLookup[ i ];
			UI_COMPONENT_ENUM eComp = pLookup->eCompRecipe;
			UI_COMPONENT *pComp = UIComponentGetByEnum( eComp );
			ASSERTX_RETURN( pComp, "Expected recipe list component" );
			
			UIComponentSetVisibleByEnum( eComp, FALSE );
			UIComponentSetActiveByEnum( eComp, FALSE );
			pComp->m_dwData = (DWORD_PTR)NONE;
			
		}
		
		// go through the recipe list and setup a recipe entry for it
		int nCurrentRecipeIndex = 0;
		int nRecipeList = pGlobals->nRecipeList;
		if (nRecipeList != INVALID_LINK)
		{
			const RECIPELIST_DEFINITION *pRecipeListDef = RecipeListGetDefinition( nRecipeList );
			
			// go through each recipe
			for (int i = 0; pRecipeListDef->nRecipes[ i ] != INVALID_LINK; ++i)
			{
			
				// we must have enough room for another recipe
				ASSERTX_RETURN( nCurrentRecipeIndex < MAX_RECIPES, "Too many recipes in recipe list" );

				// get the next ui element to show
				const RECIPE_LIST_LOOKUP *pLookup = &tRecipeListCompLookup[ nCurrentRecipeIndex ];
				ASSERTX_RETURN( pLookup->eCompRecipe != UICOMP_INVALID, "Too many recipes in list for UI to handle" );
				UI_COMPONENT *pCompRecipe = UIComponentGetByEnum( pLookup->eCompRecipe );

				// get comp for result picture
				UI_COMPONENT *pCompResultPicture = UIComponentFindChildByName( pCompRecipe, "result picture" );
				ASSERTX_RETURN( pCompResultPicture, "Expected result picture component" );
				
				// get comp for description
				UI_COMPONENT *pCompRecipeText = UIComponentFindChildByName( pCompRecipe, "result text" );
					
				// get the recipe
				int nRecipe = pRecipeListDef->nRecipes[ i ];

				// initialize this recipe instance with ingredients and results etc.
				RECIPE *pRecipe = &pGlobals->tRecipe[ nCurrentRecipeIndex ];
				if (sRecipeSetup( *pRecipe, nRecipe, pGlobals->idRecipeGiver, nCurrentRecipeIndex ))
				{
								
					// set which recipe this whole comp is for
					pCompRecipe->m_dwData = i;
								
					// show this UI element
					UIComponentSetActiveByEnum( pLookup->eCompRecipe, TRUE );
					
					// get result string for recipe
					const int MAX_STRING = 1024;
					WCHAR uszDescription[ MAX_STRING ];
					c_RecipeGetResultString( pPlayer, pRecipe, uszDescription, MAX_STRING, TRUE );
					
					// set the recipe description
					UILabelSetText( pCompRecipeText, uszDescription );

					// set image for the first result ... not sure what to do if we allow two results??
					UNITID idResult = pRecipe->idResults[ 0 ];
					ASSERTX_RETURN( idResult != INVALID_ID, "Expected recipe result item" );
					UNIT *pResult = UnitGetById( pGame, idResult );
					if (pResult)
					{
						UIComponentSetFocusUnit( pCompResultPicture, UnitGetId( pResult ) );
					}
					
				}
				else
				{
					UIComponentSetFocusUnit( pCompResultPicture, INVALID_ID );
					UILabelSetText( pCompRecipeText, L"" );
				}

				// on to the next index now
				nCurrentRecipeIndex++;
															
			}
			
		}	
	}

	// do update of the currently selected recipe
	c_RecipeCurrentUpdateUI();
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeCurrentUpdateUI(
	void)
{
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	UNIT *pPlayer = UIGetControlUnit();
	GAME *pGame = UnitGetGame( pPlayer );
	
	// get recipe (if any is selected at all)
	const RECIPE *pRecipe = sGetSelectedRecipe();

	// setup tables for required item comps
	static UI_COMPONENT_ENUM eCompRequires[] = 
	{
		UICOMP_RECIPE_ITEM_3,  // note how we start show items from comp #2 to be more "centered"
		UICOMP_RECIPE_ITEM_4,
		UICOMP_RECIPE_ITEM_2,
		UICOMP_RECIPE_ITEM_1,
		UICOMP_RECIPE_ITEM_5,
		UICOMP_INVALID
	};

	// hide all components
	UIComponentArrayDoAction( eCompRequires, UIA_HIDE, INVALID_ID );
	UIComponentArrayDoAction( eCompRequires, UIA_SETFOCUSUNIT, INVALID_ID );
	
	// set the required ingredient comps
	if (pRecipe)
	{
		UNIT *pRecipeGiver = UnitGetById( pGame, pRecipe->idRecipeGiver );
		
		// get recipe ingredients
		RECIPE_INSTANCE_INGREDIENTS tInstanceIngredients;
		RecipeInstanceGetIngredients( pPlayer, pRecipe->nRecipe, pRecipe->nIndex, pRecipeGiver, &tInstanceIngredients );
		
		int nRequiredIndex = 0;
		for (int i = 0; i < MAX_RECIPE_INGREDIENTS; ++i)
		{
			UNITID idItem = pRecipe->idIngredients[ i ];
			if (idItem != INVALID_ID)
			{
				UNIT * pItem = UnitGetById(pGame, idItem);
				ASSERT_RETURN(pItem);

				// go through all the collect items
				const RECIPE_INGREDIENT *pIngredient = NULL;

				for ( int j = 0; j < MAX_RECIPE_INGREDIENTS; ++j )
				{
					const RECIPE_INGREDIENT *pThisIngredient = &tInstanceIngredients.tIngredient[ j ];
					if (RecipeUnitMatchesIngredient( pItem, pThisIngredient ))
					{
						pIngredient = pThisIngredient;
						break;
					}
				}

				ASSERT_RETURN(pIngredient);

				UI_COMPONENT_ENUM eComp = eCompRequires[ nRequiredIndex++ ];
				ASSERTX_RETURN( eComp != UICOMP_INVALID, "Too many required component items" );
								
				// set the focus unit
				UI_COMPONENT *pComp = UIComponentGetByEnum( eComp );
				UIComponentSetFocusUnit( pComp, idItem );
				
				// set the color of the text of the required item quantity based on whether
				// or not the player has enough of this ingredient
				int nCount = RecipeGetAvailableIngredientCount( pPlayer, pRecipe->nRecipe, pIngredient, TRUE );
				UI_ITEMBOX *pItemBox = UICastToItemBox( pComp );
				pItemBox->m_dwColorQuantity = GetFontColor( nCount >= pIngredient->nQuantity ? FONTCOLOR_RECIPE_COMPLETE : FONTCOLOR_RECIPE_NOT_COMPLETE );

				// show this component
				UIComponentSetActiveByEnum( eComp, TRUE );
				UIComponentHandleUIMessage( pComp, UIMSG_PAINT, 0, 0);
				
			}
			
		}
		
	}
			
	// setup table for result comps
	static UI_COMPONENT_ENUM eCompResults[] = 
	{
		UICOMP_RECIPE_RESULT_3,	// note how we start show items from comp #2 to be more "centered"
		UICOMP_RECIPE_RESULT_4,
		UICOMP_RECIPE_RESULT_2,
		UICOMP_RECIPE_RESULT_1,
		UICOMP_RECIPE_RESULT_5,
		UICOMP_INVALID
	};

	// hide all components
	UIComponentArrayDoAction( eCompResults, UIA_HIDE, INVALID_ID );
	UIComponentArrayDoAction( eCompResults, UIA_SETFOCUSUNIT, INVALID_ID );

	// set the required ingredient comps
	if (pRecipe)
	{	
		int nResultsIndex = 0;
		for (int i = 0; i < MAX_RECIPE_RESULTS; ++i)
		{
			UNITID idItem = pRecipe->idResults[ i ];
			if (idItem != INVALID_ID)
			{
				
				// show this component
				UI_COMPONENT_ENUM eComp = eCompResults[ nResultsIndex++ ];
				ASSERTX_RETURN( eComp != UICOMP_INVALID, "Too many result items" );
				UIComponentSetActiveByEnum( eComp, TRUE );
				
				// set the focus unit
				UI_COMPONENT *pComp = UIComponentGetByEnum( eComp );
				UIComponentSetFocusUnit( pComp, idItem );
				
			}
			
		}
	
	}

	// can currently selected recipe be created
	BOOL bCanCreate = FALSE;
	if (pRecipe)
	{
		UNIT *pPlayer = UIGetControlUnit();
		bCanCreate = RecipeIsComplete( 
			pPlayer, 
			pRecipe->nRecipe, 
			pRecipe->nIndex, 
			pRecipe->idRecipeGiver, 
			FALSE);
	}
	
	// enable/disable the create button
	UIComponentSetActiveByEnum( UICOMP_RECIPE_CREATE, bCanCreate );
	

	// show/hide the instructions
	UNIT *pItem = NULL;
	UI_COMPONENT *pComp = UIComponentGetByEnum(UICOMP_RECIPE_RESULT_GRID);
	if (pComp)
	{
		UI_INVGRID *pInvGrid = UICastToInvGrid(pComp);
		ASSERT(pInvGrid);
		if (pInvGrid->m_nInvLocation != INVALID_LINK)
		{
			pItem = UnitInventoryLocationIterate(pPlayer, pInvGrid->m_nInvLocation, NULL, 0, FALSE);
		}
	}
	UIComponentActivate(UICOMP_RECIPE_INSTRUCTIONS, pItem != NULL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSelectRecipe(
	int nIndex)
{
	
	// find the index of this recipe
	if (nIndex != NONE && nIndex < MAX_RECIPES)
	{
		
		// set selected recipe
		RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();	
		ASSERTX_RETURN( pGlobals, "Expected globals" );			
		pGlobals->nSelectedRecipeIndex = nIndex;
		const RECIPE *pRecipe = &pGlobals->tRecipe[ nIndex ];

		// tell the server what the newly selected recipe is
		MSG_CCMD_RECIPE_SELECT tMessage;
		tMessage.nRecipe = pRecipe->nRecipe;
		tMessage.nIndexInRecipeList = nIndex;
		tMessage.idRecipeGiver = pGlobals->idRecipeGiver;
		c_SendMessage( CCMD_RECIPE_SELECT, &tMessage );

	}
			
	// update UI
	c_RecipeCurrentUpdateUI();		

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeOpen( 
	int nRecipe,
	UNIT *pRecipeGiver)
{
	ASSERTX_RETURN( nRecipe != INVALID_LINK, "Expected recipe link" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
	UNITID idRecipeGiver = UnitGetId( pRecipeGiver );
		
	// close any previously displayed UI and reset back to starting state
	c_RecipeClose( TRUE );
	
	// save in the globals some information
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );		
	pGlobals->idRecipeGiver = idRecipeGiver;
	pGlobals->nRecipeSingleUse = nRecipe;

	// setup the first recipe in the list to be the single use recipe
	int nIndex = 0;
	RECIPE *pRecipe = &pGlobals->tRecipe[ nIndex ];	
	if (sRecipeSetup( *pRecipe, nRecipe, pGlobals->idRecipeGiver, nIndex ))
	{

		// open up the UI
		if (AppIsTugboat())
		{
			UIShowPaneMenu( KPaneMenuRecipe );
			// select the one and only recipe
			sSelectRecipe( 0 );
			
			// update the UI pieces of the single selected recipe		
			c_RecipeCurrentUpdateUI();
		}
		else if (AppIsHellgate())
		{
			//UIComponentActivate( UICOMP_RECIPE_CURRENT_PANEL, TRUE );
		
			//// close paper doll and other inventory UI that can be in the same
			//// location as the recipe UI
			//if (UIComponentGetActiveByEnum( UICOMP_RECIPE_CURRENT_PANEL ) == FALSE)
			//{			
			//	// bring up panel
			//	UISwitchComponents( UICOMP_PAPERDOLL, UICOMP_RECIPE_CURRENT_PANEL );
			//	UISwitchComponents( UICOMP_MERCHANTWELCOME, UICOMP_RECIPE_CURRENT_PANEL );						
			//}

			// open UI if not open already
			TRADE_SPEC tTradeSpec;
			tTradeSpec.eStyle = STYLE_RECIPE_SINGLE;		
			tTradeSpec.nRecipe = pGlobals->nRecipeSingleUse;
			UIShowTradeScreen( pGlobals->idRecipeGiver, FALSE, &tTradeSpec );				
						
		}

		// select the one and only recipe
		sSelectRecipe( 0 );
		
		// update the UI pieces of the single selected recipe		
		c_RecipeCurrentUpdateUI();

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeListOpen( 
	int nRecipeList,
	UNIT *pRecipeGiver)
{	
	ASSERTX_RETURN( nRecipeList != INVALID_LINK, "Expected recipe list link" );
	ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );	
	UNITID idRecipeGiver = UnitGetId( pRecipeGiver );
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );		

	// close any previously displayed UI and reset back to starting state
	c_RecipeClose( TRUE );

	// save in the globals some information
	pGlobals->idRecipeGiver = idRecipeGiver;
	pGlobals->nRecipeList = nRecipeList;
	pGlobals->nSelectedRecipeIndex = NONE;
		
	// open up the UI

	TRADE_SPEC tTradeSpec;
	tTradeSpec.eStyle = STYLE_RECIPE;		
	tTradeSpec.nRecipeList = pGlobals->nRecipeList;
	UIShowTradeScreen( pGlobals->idRecipeGiver, FALSE, &tTradeSpec );				
	
	// update the UI pieces to hide/show elements for recipes that are present
	sRecipeListUpdateUI();

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_RecipeClose(
    BOOL bSendToServer)
{
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );		

	pGlobals->bSendCloseToServer = bSendToServer;
	UIComponentActivate(UICOMP_RECIPE_LIST_PANEL, FALSE);
	UIComponentActivate(UICOMP_RECIPE_CURRENT_PANEL, FALSE);

	// if we're in the recipe UI, close it
	if (pGlobals->nRecipeList != INVALID_LINK || pGlobals->nRecipeSingleUse != INVALID_LINK)
	{
		return TRUE;
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_RecipeIsSelected(
	void)
{
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );	
	return sGetSelectedRecipe() != NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeCreated(
	int nRecipe)
{
	if( AppIsTugboat() )
	{
		UICraftingRecipeCreated( nRecipe, TRUE );
		return;
	}
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETURN( pGlobals, "Expected globals" );	
	
	// deselect recipe
	pGlobals->nSelectedRecipeIndex = NONE;
	
	// refresh the recipe list
	sRecipeListUpdateUI();
	
}

void c_RecipeFailed( int nRecipe )
{
	if( AppIsTugboat() )
	{
		UICraftingRecipeCreated( nRecipe, FALSE );
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RecipeGetIngredientTooltip(
	UNIT *pItem,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	ASSERTX_RETURN( pItem, "Expected item unit" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferSize, "Invalid buffer size" );

	// init string
	puszBuffer[ 0 ] = 0;
	
	// get recipe item is a component for
	int nRecipe = UnitGetStat( pItem, STATS_RECIPE_COMPONENT );
	if (nRecipe != INVALID_LINK)
	{
		RECIPE *pRecipe = sGetSelectedRecipe();
		if (pRecipe)
		{
			UNIT *pPlayer = UIGetControlUnit();
			ASSERTX_RETURN( pPlayer, "Expected player ui control unit" );
			GAME *pGame = UnitGetGame( pPlayer );
			UNIT *pRecipeGiver = UnitGetById( pGame, pRecipe->idRecipeGiver );
			ASSERTX_RETURN( pRecipeGiver, "Expected recipe giver" );
			const int MAX_STRING = 256;
			WCHAR uszString[ MAX_STRING ];
			
			// find which ingredient this matches
			RECIPE_INGREDIENT tIngredient;
			BOOL bIngredientFound = RecipeFindIngredientMatchingItem( 
				pItem,			
				pPlayer, 
				pRecipe->nRecipe, 
				pRecipe->nIndex,
				pRecipeGiver,
				&tIngredient);
				
			if (bIngredientFound)
			{
								
				// how many of these items does the player need
				int nRequired = tIngredient.nQuantity;
				PStrPrintf( uszString, MAX_STRING, GlobalStringGet( GS_RECIPE_QUANTITY_REQUIRED ), nRequired );
				PStrCat( puszBuffer, uszString, nBufferSize );
				PStrCat( puszBuffer, L"\n", nBufferSize );
				
				// how many of these items does the player have
				int nOwned = RecipeGetAvailableIngredientCount( pPlayer, pRecipe->nRecipe, &tIngredient, TRUE );
				PStrPrintf( uszString, MAX_STRING, GlobalStringGet( GS_RECIPE_QUANTITY_OWNED ), nOwned );
				PStrCat( puszBuffer, uszString, nBufferSize );
				
				// add needed
				if (nOwned < nRequired)
				{
					int nNeeded = nRequired - nOwned;
					PStrCat( puszBuffer, L"\n", nBufferSize );
					PStrPrintf( uszString, MAX_STRING, GlobalStringGet( GS_RECIPE_QUANTITY_NEEDED ), nNeeded );
					PStrCat( puszBuffer, uszString, nBufferSize );
				}

			}
						
		}
		
	}
		
}

//----------------------------------------------------------------------------
// Mythos UI Handlers
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradesmanSheetOnSetControlUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
//	UI_TRADESMANSHEET *pSheet = UICastToTradesmanSheet(component);
	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int sTradesmanSheetGetRecipeAtPosition(
	UI_TRADESMANSHEET *pSheet,
	float x, 
	float y,
	UI_POSITION& pos )
{
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return INVALID_LINK;
	}
	UNIT *pFocusUnit = UIComponentGetFocusUnit(pSheet);
	if (!pFocusUnit)
	{
		return 0;
	}

	UI_GFXELEMENT *pChild = pSheet->m_pGfxElementFirst;
	while(pChild)
	{

		if ( UIElementCheckBounds(pChild, x, y ) && pChild->m_qwData != (QWORD)INVALID_ID )
		{
			pos.m_fX = pChild->m_Position.m_fX;
			pos.m_fY = pChild->m_Position.m_fY;

			return (int)pChild->m_qwData;
		}
		pChild = pChild->m_pNextElement;
	}
	return INVALID_LINK;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradesmanSheetOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_TRADESMANSHEET *pSheet = UICastToTradesmanSheet(component);
	float x = 0.0f;	float y = 0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pSheet, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;

	UNIT *pFocusUnit = UIComponentGetFocusUnit(component);
	if (!pFocusUnit)
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	int nRecipeIndex = sTradesmanSheetGetRecipeAtPosition( pSheet, x, y, pos );
	if( nRecipeIndex != INVALID_ID )
	{
		
		const UNIT_DATA* pUnitData = UnitGetData( pFocusUnit );
		const RECIPELIST_DEFINITION *pRecipeList =  RecipeListGetDefinition( pUnitData->nRecipeList );
		ASSERTX( pRecipeList, "Missing Recipe List" );
		
		// find the recipe
		for( int t = 0; t < MAX_RECIPES; t++ )
		{
			if( pRecipeList->nRecipes[ t ] == INVALID_ID )
			{
				break; //no more items to check against
			}
			if( t == nRecipeIndex)
			{
				int nRecipe = pRecipeList->nRecipes[ t ];
				UNITID idRecipeGiver = UnitGetId( pFocusUnit );
				if (RecipeIsComplete( UIGetControlUnit(), nRecipe, t, idRecipeGiver, FALSE ))
				{
					c_RecipeTryCreate( UnitGetId( pFocusUnit ), nRecipe, t );
					static int nSellSound = INVALID_LINK;
					if (nSellSound == INVALID_LINK)
					{
						nSellSound = GlobalIndexGet( GI_SOUND_ITEM_PURCHASE );
					}

					if (nSellSound != INVALID_LINK)
					{
						c_SoundPlay(nSellSound, &c_SoundGetListenerPosition(), NULL);
					}
					
				}

			}
						
		}
		
	}


	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradesmanSheetOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_TRADESMANSHEET *pSheet = UICastToTradesmanSheet(component);
	int location = pSheet->m_nInvLocation;
	float x = (float)wParam;
	float y = (float)lParam;
	UI_POSITION pos;
	if (!UIComponentCheckBounds(pSheet, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	x -= pos.m_fX;
	y -= pos.m_fY;



	UNIT *pUnit = UIComponentGetFocusUnit(component);	
	int Index = 0;
	if( pUnit )
	{
		const UNIT_DATA* pUnitData = UnitGetData( pUnit );
		if( pUnitData->nRecipeList == INVALID_ID )
		{
			UIClearHoverText();
			return UIMSG_RET_NOT_HANDLED;
		}
		const RECIPELIST_DEFINITION* pRecipeList =  RecipeListGetDefinition( pUnitData->nRecipeList );
		ASSERTX( pRecipeList, "Missing Recipe List" );
	
		// find the recipe
		for( int t = 0; t < MAX_RECIPES; t++ )
		{
			if( pRecipeList->nRecipes[ t ] == INVALID_ID )
			{
				break; //no more items to check against
			}
			const RECIPE_DEFINITION *pRecipe =  RecipeGetDefinition( pRecipeList->nRecipes[ t ] );			
			ASSERTX( pRecipe, "Missing Recipe" );

			// display the recipe for the item
			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			if (!pFont)
			{
				pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_TRADESMAN));
			}
			UNIT* pRecipeUnit = UnitInventoryGetByLocationAndXY( pUnit, location, Index, 0 );
			if( pRecipeUnit )
			{
				if( x > 10 && x < 74 &&
					y > (float)Index * pSheet->m_fCellHeight + 10 &&
					y < (float)Index * pSheet->m_fCellHeight + 75 )
				{

					UISetHoverTextItem(
						&UI_RECT( pos.m_fX + 10, 
								  pos.m_fY + (float)Index * pSheet->m_fCellHeight + 10, 
								  pos.m_fX + 74,
								  pos.m_fY + (float)Index * pSheet->m_fCellHeight + 74), 
								  pRecipeUnit );
					UISetHoverUnit(UnitGetId(pRecipeUnit), UIComponentGetId(component));
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}

			Index++;
		}

		
	}
	UIClearHoverText();
	

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradesmanSheetOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIClearHoverText();

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UITradesmanSheetOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	if (msg == UIMSG_SETCONTROLSTAT && !UIComponentGetVisible(component))
		return UIMSG_RET_NOT_HANDLED;

	UIComponentRemoveAllElements(component);

	UI_TRADESMANSHEET *pSheet = UICastToTradesmanSheet(component);
	int location = pSheet->m_nInvLocation;

	UIX_TEXTURE *pTexture = UIComponentGetTexture(component);
	if (!pTexture)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x=0.0f; float y=0.0f;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	UIComponentCheckBounds(component, x, y, &pos);
	x -= pos.m_fX;
	y -= pos.m_fY;

	UNIT *pUnit = UIComponentGetFocusUnit(component);	
	int Index = 0;
	if( pUnit )
	{
		
		const UNIT_DATA* pUnitData = UnitGetData( pUnit );
		if( pUnitData->nRecipeList == INVALID_ID )
		{
			return UIMSG_RET_NOT_HANDLED;
		}
		const RECIPELIST_DEFINITION* pRecipeList =  RecipeListGetDefinition( pUnitData->nRecipeList );
		ASSERTX( pRecipeList, "Missing Recipe List" );
	
		// find the recipe
		//RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
		for( int t = 0; t < MAX_RECIPES; t++ )
		{
			int nRecipe = pRecipeList->nRecipes[ t ];
			if (nRecipe == INVALID_LINK)
			{	
				break;
			}
			
			// setup a recipe instance structure
			RECIPE tRecipe;
			sRecipeSetup( tRecipe, nRecipe, UnitGetId( pUnit ), t );
			const RECIPE_DEFINITION* pRecipeDef = tRecipe.pRecipeDef;
			ASSERTX_BREAK( pRecipeDef, "Missing Recipe" );

			// display the recipe for the item
			UIX_TEXTURE_FONT *pFont = UIComponentGetFont(component);
			if (!pFont)
			{
				pFont = UIComponentGetFont(UIComponentGetByEnum(UICOMP_TRADESMAN));
			}

		    UNIT* pRecipeUnit = UnitInventoryGetByLocationAndXY( pUnit, location, Index, 0 );
			
			UI_POSITION textpos( 80, (float)Index * pSheet->m_fCellHeight );
			UI_SIZE size( component->m_fWidth, pSheet->m_fCellHeight );
		    
			DWORD dwColor = UI_MAKE_COLOR(255, GFXCOLOR_WHITE);
			
			// construct full description string
			const int MAX_RECIPE_STRING = 1024;
			WCHAR uszRecipe[ MAX_RECIPE_STRING ] = L"\0";

			// get the item name in singular or plural form
			ITEM_NAME_INFO tItemNameInfo;
			ItemGetNameInfo( tItemNameInfo, pRecipeDef->nRewardItemClass[ 0 ], 1, 0 );
			UIColorCatString(uszRecipe, MAX_RECIPE_STRING, FONTCOLOR_YELLOW, tItemNameInfo.uszName );

			//PStrCat( uszRecipe, tItemNameInfo.uszName, 512 );	
			PStrCat( uszRecipe, L" - Requires :\n", 512 );

			int IngredientIndex = 0;
			for( int u = 0; u < MAX_RECIPE_INGREDIENTS; u++ )
			{
				const RECIPE_INGREDIENT_DEFINITION *pIngredientDef = &pRecipeDef->tIngredientDef[ u ];
				if( pIngredientDef->nItemClass == INVALID_LINK )
				{
					break; //no more items to check against
				}
				if( IngredientIndex > 0 )
				{
					PStrCat( uszRecipe, L", ", 512 );	
				}
				int nQuantity = pIngredientDef->nQuantityMin;  // just use min quantity

				// get the item name in singular or plural form
				ITEM_NAME_INFO tItemNameInfo;
				ItemGetNameInfo( tItemNameInfo, pIngredientDef->nItemClass, nQuantity, 0 );
				// init string
				WCHAR uszWorkingName[ MAX_ITEM_NAME_STRING_LENGTH ];
				uszWorkingName[ 0 ] = 0;
				UIColorCatString(uszWorkingName, MAX_ITEM_NAME_STRING_LENGTH, FONTCOLOR_VERY_LIGHT_BLUE, tItemNameInfo.uszName );
				
				// add quantity or money amount numbers to the item name
				if (nQuantity > 0)
				{
					// get quantity as string
					WCHAR uszQuantity[ MAX_ITEM_NAME_STRING_LENGTH ];
					PStrPrintf( uszQuantity, MAX_ITEM_NAME_STRING_LENGTH, L"%d ", nQuantity );
					
					PStrCat( uszRecipe, uszQuantity, 512 );	
						
				}	
				
				PStrCat( uszRecipe, uszWorkingName, 512 );		
				IngredientIndex ++;
				
			}
						
			// add description string to ui			
			UIComponentAddTextElement(component, pTexture, pFont, UIComponentGetFontSize(component), uszRecipe, textpos, dwColor, NULL, UIALIGN_LEFT, &size );

			if( pRecipeUnit && pRecipeUnit->pIconTexture )
			{
				BOOL bLoaded = UITextureLoadFinalize( pRecipeUnit->pIconTexture );
				REF( bLoaded );
				UI_TEXTURE_FRAME* frame = (UI_TEXTURE_FRAME*)StrDictionaryFind(pRecipeUnit->pIconTexture->m_pFrames, "icon");
				if (frame)
				{
					float OffsetX =  10;
					float OffsetY =  (float)Index * pSheet->m_fCellHeight + 10;
					float ScaleX = ICON_BASE_TEXTURE_WIDTH / pRecipeUnit->pIconTexture->m_fTextureWidth * .9f; 
					float ScaleY = ICON_BASE_TEXTURE_WIDTH / pRecipeUnit->pIconTexture->m_fTextureHeight * .9f; 

					if( g_UI.m_bWidescreen )
					{
						ScaleY *= .78f;
					}
					UIComponentAddElement(component, pRecipeUnit->pIconTexture, frame, UI_POSITION(OffsetX, OffsetY) , GFXCOLOR_WHITE, NULL, FALSE, ScaleX, ScaleY );
				}
			}
				// display a button for completing this recipe, if the correct goods are present
			if( RecipeIsComplete( UIGetControlUnit(), pRecipeList->nRecipes[ t ], t, UnitGetId( pUnit ), FALSE ) )
			{
				if (pSheet->m_pCreateButtonFrame)
				{
					pos.m_fX = 500;
					pos.m_fY = (float)Index * pSheet->m_fCellHeight + 10;
					DWORD dwColor = pSheet->m_pCreateButtonFrame->m_dwDefaultColor;
					UI_RECT rectFrame;
					rectFrame.m_fX1 = pos.m_fX - pSheet->m_pCreateButtonFrame->m_fXOffset;
					rectFrame.m_fY1 = pos.m_fY - pSheet->m_pCreateButtonFrame->m_fYOffset;
					rectFrame.m_fX2 = rectFrame.m_fX1 + pSheet->m_pCreateButtonFrame->m_fWidth;
					rectFrame.m_fY2 = rectFrame.m_fY1 + pSheet->m_pCreateButtonFrame->m_fHeight;
					if (UIInRect(UI_POSITION(x,y), rectFrame))
					{
						dwColor = 0xffffff00;
					}
					UI_GFXELEMENT* pElement = UIComponentAddElement(component, pTexture, pSheet->m_pCreateButtonFrame, pos, dwColor);
					pElement->m_qwData = Index;
				}
			}
			Index++;
		}

		
	}
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
// Hellgate UI Handlers
//----------------------------------------------------------------------------
void UISetSelectedRecipe( int nRecipeID )
{
	sSelectRecipe( nRecipeID );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeListSelectRecipe( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	ASSERTX_RETVAL( pComponent, UIMSG_RET_NOT_HANDLED, "Expected component" );

	// must be in bounds
	if (!UIComponentGetActive( pComponent ) || !UIComponentCheckBounds( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	
	// get the recipe of this window
	int nRecipeIndex = (int)pComponent->m_dwData;
	UISetSelectedRecipe( nRecipeIndex );
		
	return UIMSG_RET_HANDLED;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeOnChangeIngredients( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{

	// must be in bounds
	if (!UIComponentGetActive( pComponent )/* || !UIComponentCheckBounds( pComponent )*/)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	// call the default grid on change function
	UI_MSG_RETVAL eResult = UIInvGridOnInventoryChange( pComponent, nMessage, wParam, lParam );
	if (ResultIsHandled(eResult))
	{
		// update the recipe UI
		sRecipeListUpdateUI();
	}

	return eResult;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeCreate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{

	// must be in bounds
	if (!UIComponentGetActive( pComponent ) || !UIComponentCheckBounds( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if( AppIsTugboat() )
	{
		UICraftingRecipeCreate();
		return UIMSG_RET_HANDLED_END_PROCESS;
	}

	const RECIPE *pRecipe = sGetSelectedRecipe();
	if (pRecipe)
	{
		const RECIPE_DEFINITION *pRecipeDef = RecipeGetDefinition( pRecipe->nRecipe );

		// where will we put the result
		int nInvSlotResult = pRecipeDef->nInvLocIngredients;
		if ( nInvSlotResult != INVALID_LINK )
		{
			UNIT *pPlayer = UIGetControlUnit();
			if (pPlayer)
			{
				if (UnitInventoryLocationIterate(pPlayer, nInvSlotResult, NULL, 0, FALSE ))
				{
					UIShowGenericDialog(StringTableGetStringByKey("item_pickup_no_room_header"), StringTableGetStringByKey("item_crafting_no_room"));
					return UIMSG_RET_HANDLED_END_PROCESS;
				}
			}
		}

		c_RecipeTryCreate( pRecipe->idRecipeGiver, pRecipe->nRecipe, pRecipe->nIndex );
	}
	
	return UIMSG_RET_HANDLED_END_PROCESS;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{

	// must be in bounds
	if (!UIComponentGetActive( pComponent ) || !UIComponentCheckBounds( pComponent ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	c_RecipeClose(TRUE);

	return UIMSG_RET_HANDLED_END_PROCESS;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam)
{
	RECIPE_GLOBALS *pGlobals = sGetRecipeGlobals();
	ASSERTX_RETVAL( pGlobals, UIMSG_RET_NOT_HANDLED, "Expected globals" );		

	if (!pComponent->m_bUserActive)	// if we're just being temporarily inactivated (switching for gunmod panel for instance) don't cancel
	{
		// if we're in the recipe UI, close it
		if (pGlobals->nRecipeList != INVALID_LINK || pGlobals->nRecipeSingleUse != INVALID_LINK)
		{

			// setup message and send to server
			if (pGlobals->bSendCloseToServer)
			{
				MSG_CCMD_RECIPE_CLOSE tMessage;
				tMessage.idRecipeGiver = pGlobals->idRecipeGiver;
				c_SendMessage( CCMD_RECIPE_CLOSE, &tMessage );
			}

			// clear globals
			sRecipeGlobalsInit( *pGlobals );

			// close dialog
			UIHideNPCDialog(CCT_OK);
		}

		pGlobals->bSendCloseToServer = FALSE;
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}

void c_RecipeInsertName( UNIT *pItemRecipe, WCHAR *pString, int nStringLength )
{
	ASSERT_RETURN( pItemRecipe );
	if( ItemIsRecipe( pItemRecipe ) == FALSE )
		return;
	int nRecipeID = ItemGetRecipeID( pItemRecipe );
	if( nRecipeID != INVALID_ID )
	{
		c_RecipeInsertName( nRecipeID, pString, nStringLength );
	}
}

void c_RecipeInsertName( int nRecipeID, WCHAR *pString, int nStringLength )
{
	int nRecipeCount = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_RECIPES );
	ASSERT_RETURN( nRecipeID >= 0 && nRecipeID < nRecipeCount );
	const RECIPE_DEFINITION *pRecipeDefinition = RecipeGetDefinition( nRecipeID );
	ASSERT_RETURN( pRecipeDefinition );
	const UNIT_DATA *pItem = ItemGetData( pRecipeDefinition->nRewardItemClass[0] );
	ASSERT_RETURN( pItem );
	const WCHAR *uszRecipeName = StringTableGetStringByIndex(  pItem->nString );
	static WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
	UICraftingGetTextOfCategoryRequired( pRecipeDefinition, newText );
	PStrReplaceToken(pString, nStringLength, StringReplacementTokensGet( SR_RECIPE_NAME ), uszRecipeName );	
	PStrReplaceToken(pString, nStringLength, StringReplacementTokensGet( SR_RECIPE_LVL ), newText );	
}

int c_RecipeGetFlavoredText( UNIT *pItemRecipe )
{
	ASSERT_RETINVALID( pItemRecipe );
	int nRecipeID = ItemGetRecipeID( pItemRecipe );
	if( nRecipeID != INVALID_ID )
	{
		UNIT *pPlayer = UnitGetUltimateOwner( pItemRecipe );
		if( pPlayer && UnitGetGenus( pPlayer ) == GENUS_PLAYER )
		{
			if( PlayerHasRecipe( pPlayer, nRecipeID ) )
				return GlobalStringGetStringIndex( GS_RECIPE_ITEM_ALREADY_WRITTEN_FLAVOREDTEXT );
		}
	}
	return GlobalStringGetStringIndex( GS_RECIPE_ITEM_FLAVOREDTEXT );
}

//returns the client's ingredient by index
UNIT * c_RecipeGetClientIngredientByIndex( 
	const RECIPE_DEFINITION *pRecipeDefinition,
	int nIngredientIndex,
	int nOverrideQuality /*= INVALID_ID*/ )
{
	ASSERT_RETNULL( pRecipeDefinition );
	ASSERT_RETNULL( nIngredientIndex >= 0 && nIngredientIndex < MAX_RECIPE_INGREDIENTS);
	if( pRecipeDefinition->tIngredientDef[ nIngredientIndex ].nItemClass == INVALID_ID )
		return NULL;	
	UNIT *pItem = c_UIGetClientUNIT( GENUS_ITEM, 
									  pRecipeDefinition->tIngredientDef[ nIngredientIndex ].nItemClass,
									  nOverrideQuality != INVALID_ID ? nOverrideQuality : pRecipeDefinition->tIngredientDef[ nIngredientIndex ].nItemQuality,
									  pRecipeDefinition->tIngredientDef[ nIngredientIndex ].nQuantityMax );

	return pItem;
}

//returns the client's result item
UNIT * c_RecipeGetClientResultByIndex( 
	  const RECIPE_DEFINITION *pRecipeDefinition,
	  int nResultIndex,
	  int nOverrideQuality /*= INVALID_ID*/ )
{
	ASSERT_RETNULL( pRecipeDefinition );
	ASSERT_RETNULL( nResultIndex >= 0 && nResultIndex < MAX_RECIPE_RESULTS);
	if( pRecipeDefinition->tIngredientDef[ nResultIndex ].nItemClass == INVALID_ID )
		return NULL;
	UNIT *pItem = c_UIGetClientUNIT( GENUS_ITEM, 
		pRecipeDefinition->nRewardItemClass[ nResultIndex ],
		0 );
	//for the client this is dumb but we have to free the unit and then recreate it
	c_UIFreeClientUnit( pItem );
	pItem = c_UIGetClientUNIT( GENUS_ITEM, 
		pRecipeDefinition->nRewardItemClass[ nResultIndex ],
		 nOverrideQuality != INVALID_ID ? nOverrideQuality : 0 );

	//generate the additional information for the item
	RecipeModifyCreatedItemByCraftingTreeInvestment( UIGetControlUnit(), pItem, ExcelGetLineByCode( EXCEL_CONTEXT( NULL ), DATATABLE_RECIPES,  pRecipeDefinition->wCode ) );
	return pItem;
}

//----------------------------------------------------------------------------
//Retrieves the value the player has set on the recipe property
//----------------------------------------------------------------------------
int UIGetRecipePCTOfPropertyModifiedByPlayer( int nRecipeID,
										int nRecipeProperty )
{
	ASSERT_RETZERO( nRecipeID >= 0  );
	ASSERT_RETZERO( nRecipeProperty >= 0 && nRecipeProperty < NUM_RECIPE_PROP );
	return sRecipeGetPropertyByRecipe( nRecipeID, nRecipeProperty );
}

//----------------------------------------------------------------------------
//Sends to the server the property value the player has modified.( Happens on a scroll bar )
//----------------------------------------------------------------------------
void UISetRecipePropertyModifiedByPlayer( int nRecipeID,
										 int nRecipeProperty,
										 int nRecipePropertyValue )
{
	// setup message and send
	ASSERT_RETURN( nRecipeID >= 0  );
	ASSERT_RETURN( nRecipeProperty >= 0 && nRecipeProperty < NUM_RECIPE_PROP );
	ASSERT_RETURN( nRecipePropertyValue >= 0 && nRecipePropertyValue <= 100 );
	sRecipeSetPropertyByRecipe( nRecipeID, nRecipeProperty, nRecipePropertyValue );

}


//----------------------------------------------------------------------------
//Returns the recipes price by creating a tmp recipe
//----------------------------------------------------------------------------
cCurrency c_RecipeGetCurrency( int nRecipeID )
{
	// setup message and send
	ASSERT( nRecipeID >= 0  );
	if( nRecipeID < 0 )
		return cCurrency();
	UNIT *pRecipeItem = c_UIGetClientUNIT( GENUS_ITEM, GI_RECIPE_ITEM_PRICE_TEST );
	ASSERT( pRecipeItem  );
	if( !pRecipeItem )
		return cCurrency();
	UnitSetStat( pRecipeItem, STATS_RECIPE_COMPONENT, nRecipeID );
	return EconomyItemSellPrice( pRecipeItem );
}

#endif //!ISVERSION(SERVER_VERSION)