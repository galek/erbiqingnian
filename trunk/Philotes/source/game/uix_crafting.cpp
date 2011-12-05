//----------------------------------------------------------------------------
// uix_crafting.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "uix.h"
#include "uix_graphic.h"
#include "uix_priv.h"
#include "uix_components.h"
#include "uix_components_complex.h"
#include "uix_graphic.h"
#include "uix_scheduler.h"
#include "uix_crafting.h"
#include "player.h"
#include "performance.h"
#include "units.h"
#include "excel.h"
#include "skills.h"
#include "fontcolor.h"
#include "pstring.h"
#include "c_crafting.h"
#include "recipe.h"
#include "c_recipe.h"
#include "chat.h"
#include "uix_chat.h"
#include "items.h"
#include "s_message.h"
#include "c_ui.h"
#include "stringreplacement.h"
#include "c_recipe.h"
#include "Quest.h"
#include "globalIndex.h"
#include "c_sound.h"
#include "c_sound_util.h"

#if !ISVERSION(SERVER_VERSION)
EINVENTORY_BAGS sUIInventoryShowBag( KINVENTORY_BACKPACK );
static int sRecipeSelected( INVALID_ID );


#define UI_RECIPE_COMBO		"recipe combo"
#define RECIPE_NONE 100000
int	gnSelectedRecipe = RECIPE_NONE; 


#define FLOATER_NUM_PROPS  6
static RECIPE_PROPERTIES_ENUM FloaterPropertiesWeapon[FLOATER_NUM_PROPS] =
{
	RECIPE_PROP_FIREDAMAGE,
	RECIPE_PROP_ICEDAMAGE,
	RECIPE_PROP_ELECTRICDAMAGE,
	RECIPE_PROP_POISONDAMAGE,
	RECIPE_PROP_OFFENSIVEAFFIXES,
	RECIPE_PROP_DAMAGERANGEMODIFY
};

static RECIPE_PROPERTIES_ENUM FloaterPropertiesArmor[FLOATER_NUM_PROPS] =
{
	RECIPE_PROP_FIRERESISTANCE,
	RECIPE_PROP_ICERESISTANCE,
	RECIPE_PROP_ELECTRICRESISTANCE,
	RECIPE_PROP_POISONRESISTANCE,
	RECIPE_PROP_DEFENSIVEAFFIXES,
	RECIPE_PROP_INVALID
};


inline UNIT * sUICraftingSetCraftingItem( UI_COMPONENT *pComp, int nClassID, int nClassIDCount )
{
	if( nClassID == INVALID_ID )
	{
		if( pComp )
		{
			UIComponentSetVisible( pComp, FALSE );
			UIComponentSetFocusUnit( pComp, INVALID_ID );				
			UIComponentSetActive( pComp, FALSE );
		}
		return NULL;
	}
	else
	{
		const UNIT_DATA* pItemData = ItemGetData( nClassID );
		ASSERT_RETNULL(pItemData);
		UNIT *pUnit = c_UIGetClientUNIT( GENUS_ITEM, nClassID, pItemData->nRequiredQuality, (nClassIDCount<=1)?1:nClassIDCount );		
		ASSERT_RETNULL( pUnit );
		//lets clear it's max sockets t0 be on the safe side
		UnitSetStat(pUnit,  STATS_ITEM_SLOTS_MAX, INVLOC_GEMSOCKET, 0 );
		UnitSetStat(pUnit,  STATS_ITEM_SLOTS, INVLOC_GEMSOCKET, 0 );
		if( pComp )
		{
			UIComponentSetVisible( pComp, TRUE );
			UIComponentSetFocusUnit( pComp, UnitGetId( pUnit ) );				
			UIComponentSetActive( pComp, TRUE );		
		}
		return pUnit;

	}
}

static RECIPE_PROPERTIES_ENUM FloaterProperties[FLOATER_NUM_PROPS];
static const RECIPE_DEFINITION *pRecipeLastFloater( NULL );

//----------------------------------------------------------------------------
//Inline function to set the floater properties
//----------------------------------------------------------------------------
inline void UICraftingSetFloaterProperties( const RECIPE_DEFINITION *pRecipe )
{
	if( pRecipe == NULL ||
		pRecipeLastFloater == pRecipe )
		return;
	pRecipeLastFloater = pRecipe;
	UNIT *pUnit = sUICraftingSetCraftingItem( NULL, pRecipe->nRewardItemClass[ 0 ],  0 );		
	if( pUnit &&
		UnitIsA( pUnit, UNITTYPE_WEAPON ) )
	{
		MemoryCopy( FloaterProperties, sizeof( int ) * FLOATER_NUM_PROPS, FloaterPropertiesWeapon, sizeof( int ) * FLOATER_NUM_PROPS );
	}
	else
	{
		MemoryCopy( FloaterProperties, sizeof( int ) * FLOATER_NUM_PROPS, FloaterPropertiesArmor, sizeof( int ) * FLOATER_NUM_PROPS );
	}
}

inline RECIPE_PROPERTIES_ENUM UICraftingGetFloaterProp( const RECIPE_DEFINITION *pRecipe,
													   int nFloaterProp )
{
	UICraftingSetFloaterProperties( pRecipe );
	if( nFloaterProp < 0 ||
		nFloaterProp >= FLOATER_NUM_PROPS )
	{
		return RECIPE_PROP_INVALID;
	}
	return FloaterProperties[ nFloaterProp ];
}

//----------------------------------------------------------------------------
//This is the magic function. Called from c_Recipe.cpp
//----------------------------------------------------------------------------
void UICraftingRecipeCreate()
{
	ASSERT_RETURN( sRecipeSelected != INVALID_ID );	
	c_RecipeTryCreate( (UNITID)INVALID_ID, sRecipeSelected, -1 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingSlotsOnPaint(
									UI_COMPONENT* pComponent,
									int msg,
									DWORD wParam,
									DWORD lParam)
{
	UIComponentOnPaint(pComponent, msg, wParam, lParam);
	UNIT *pPlayer = UIGetControlUnit();
	if (!pPlayer)
		return UIMSG_RET_HANDLED;


	return UIMSG_RET_HANDLED;
}
//----------------------------------------------------------------------------
//displays the skills required for recipe to be crafted
//----------------------------------------------------------------------------
void UICraftingGetTextOfCategoryRequired( const RECIPE_DEFINITION *pRecipe,
												  WCHAR *pText )
{

	const WCHAR *oldText = GlobalStringGet( GS_RECIPE_SKILL_REQUIREMENTS );
	if( pRecipe->nRecipeCategoriesLevelsNecessaryToCraft[ 0 ] == 0 )
		oldText = GlobalStringGet( GS_RECIPE_SKILL_REQUIREMENTS_NO_LVL );
	const RECIPE_CATEGORY *pCategory = RecipeGetCategoryDefinition( pRecipe->nRecipeCategoriesNecessaryToCraft[ 0 ] );
	ASSERT_RETURN( pCategory );
	PStrPrintf( pText, MAX_STRING_ENTRY_LENGTH, L"%s", oldText );
	const WCHAR *categoryName = StringTableGetStringByIndex( pCategory->nString );	
	PStrReplaceToken(  pText, MAX_STRING_ENTRY_LENGTH, StringReplacementTokensGet( SR_SKILL_NAME ), categoryName );
	
	WCHAR categoryLevelText[ 5 ];
	ZeroMemory( categoryLevelText, sizeof( WCHAR ) * 5 );
	if( pRecipe->nRecipeCategoriesLevelsNecessaryToCraft[ 0 ] != 0 )
		PStrPrintf( categoryLevelText, 5, L"%i", pRecipe->nRecipeCategoriesLevelsNecessaryToCraft[ 0 ] );	
	PStrReplaceToken(  pText, MAX_STRING_ENTRY_LENGTH, StringReplacementTokensGet( SR_SKILL_LEVEL ), categoryLevelText );
}

//----------------------------------------------------------------------------
//displays the skills required for recipe to be crafted
//----------------------------------------------------------------------------
inline void sUICraftingGetTextOfCategoryRequired( const RECIPE_DEFINITION *pRecipe )
{

	UI_COMPONENT* pComponent = UIComponentGetById( UIComponentGetIdByName("crafting requirements text") );
	ASSERT_RETURN( pComponent );
	if( pRecipe == NULL ||		
		pRecipe->nRecipeCategoriesNecessaryToCraft[ 0 ] == INVALID_ID )
	{
		UILabelSetText( pComponent, L"" );
		return;
	}

	WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
	UICraftingGetTextOfCategoryRequired( pRecipe, newText );
	UILabelSetText( pComponent, newText );

}

//----------------------------------------------------------------------------
//Updates the component ingredients. This creates client side units
//----------------------------------------------------------------------------
inline void sUIXCraftingShowIngredients( const RECIPE_DEFINITION *pRecipe,
										 UI_COMPONENT *pMainComponent )
{


	static char* cCompIngredient[] =
	{
		"crafting ingredient 1",				
		"crafting ingredient 2",				
		"crafting ingredient 3",				
		"crafting ingredient 4",				
		"crafting ingredient 5",				
		"",
	};
	for( int t = 0; t < 5; t++ )
	{
		UI_COMPONENT *pComponent = UIComponentFindChildByName(pMainComponent, cCompIngredient[t]);
		if( pComponent )
		{
			UIComponentSetFocusUnit( pComponent, INVALID_ID );
			UIComponentSetActive( pComponent, FALSE );
			UIComponentSetVisible(pComponent, FALSE);				
			if( pRecipe->tIngredientDef[ t ].nItemClass != INVALID_ID )
			{
				const UNIT_DATA* pItemData = ItemGetData( pRecipe->tIngredientDef[ t ].nItemClass );
				// hide all components
				UNIT *pUnit = c_RecipeGetClientIngredientByIndex( pRecipe, t, pItemData->nRequiredQuality );

				if( pUnit )
				{
					UIComponentSetVisible(pComponent, TRUE);												
					UIComponentSetActive( pComponent, TRUE );
					UIComponentSetFocusUnit( pComponent, UnitGetId( pUnit ) );					
				}

			}
		}
	}

}

void sUICraftingSetUnitResult( const RECIPE_DEFINITION *pRecipe,
							   UI_COMPONENT *pMainComponent )
{

	UI_COMPONENT *pComponent = UIComponentFindChildByName(pMainComponent, "crafting result 1");
	UIComponentSetFocusUnit( pComponent, INVALID_ID );	
	if( pComponent )
	{
		// set the focus unit
		if( pRecipe->nRewardItemClass[ 0 ] != INVALID_ID )
		{
			const UNIT_DATA* pItemData = ItemGetData( pRecipe->nRewardItemClass[ 0 ] );
			UNIT *pUnit = c_RecipeGetClientResultByIndex( pRecipe, 0, pItemData->nRequiredQuality );
			if( pUnit )
			{
				UIComponentSetFocusUnit( pComponent, UnitGetId( pUnit ) );
			}									
			//UISetSelectedRecipe( nRecipeID );				
		}
		UIComponentSetVisible(pComponent, TRUE);											
		UIComponentSetActive( pComponent, TRUE );

	}
	
}
//----------------------------------------------------------------------------
//updates the UI components to select which recipe is highlighted
//----------------------------------------------------------------------------
void sUICraftingSelectRecipe( int nRecipeID,
							  UI_COMPONENT *pMainComponent )
{
	
	sRecipeSelected = nRecipeID;
	if( nRecipeID == INVALID_ID )
	{
		//sUICraftingGetTextOfCategoryRequired( NULL );
		UI_COMPONENT *pRecipeView = UIComponentFindChildByName(pMainComponent, "recipe view");
		UIComponentSetVisible(pRecipeView, FALSE);
		UI_COMPONENT *pBuyButton = UIComponentGetById( UIComponentGetIdByName("crafting recipe button buy") );
		if( pBuyButton )
		{
			UIComponentSetActive( pBuyButton, FALSE);
		}

		return;
	}

	const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipeID );	
	if( pRecipe )
	{
		// hide all components
		UI_COMPONENT *pComponent = UIComponentFindChildByName(pMainComponent, "crafting result 1");
		UIComponentSetFocusUnit( pComponent, INVALID_ID );	
		sUICraftingSetUnitResult( pRecipe, pMainComponent );

		UI_COMPONENT *pRecipeView = UIComponentFindChildByName(pMainComponent, "recipe view");
		UIComponentSetVisible( pRecipeView, TRUE );
		UIComponentSetActive( pRecipeView, TRUE );

		sUIXCraftingShowIngredients( pRecipe, pMainComponent );
		UI_COMPONENT *pCreateButton = UIComponentGetById( UIComponentGetIdByName("crafting recipe button create") );
		UI_COMPONENT *pBuyButton = UIComponentGetById( UIComponentGetIdByName("crafting recipe button buy") );
		UI_COMPONENT *pBuyPrice = UIComponentGetById( UIComponentGetIdByName("buy price") );


		if( pCreateButton )
		{
			UIComponentSetVisible( pCreateButton, RecipePlayerCanCreateRecipe( UIGetControlUnit(), pRecipe, FALSE ) );
			UIComponentSetActive( pCreateButton, UIComponentGetVisible(pCreateButton) );
		}
		// TRAVIS : doing this now right on the recipe name in the scrolling list
		//sUICraftingGetTextOfCategoryRequired( pRecipe );
		UI_COMPONENT *pScrollbars = UIComponentFindChildByName(pComponent->m_pParent, "scrollbars");
		UI_COMPONENT *pCalcValues = UIComponentFindChildByName(pComponent->m_pParent, "value calc");
		UI_COMPONENT *pMaxRiskValues = UIComponentFindChildByName(pComponent->m_pParent, "value max risk");
		UI_COMPONENT *pRiskValues = UIComponentFindChildByName(pComponent->m_pParent, "value risk");
		UI_COMPONENT *pDescValues = UIComponentFindChildByName(pComponent->m_pParent, "value desc");
		UI_COMPONENT *pTotalRisk = UIComponentFindChildByName(pComponent->m_pParent, "total risk value");
		float fValueLow[4];
		float fValueHigh[4];
		float fValueRisk[4];
		float fCurrentRisk[4];
		float fValueCurrent[4];
		int nValueProperty[4];
		int nProperty = 0;
		if( pScrollbars )
		{
			UI_COMPONENT *pChild = pScrollbars->m_pFirstChild;
			for( int i = 0; i < NUM_RECIPE_PROP; i++ )
			{
				RECIPE_PROPERTIES_ENUM nRecipePropertyID = (RECIPE_PROPERTIES_ENUM)i;
				if( pChild && RecipeHasProperty( pRecipe, nRecipePropertyID) )
				{
					nValueProperty[nProperty] = nRecipePropertyID;
					float m_fMax = (float)RecipeGetMaxPropertyValue( UIGetControlUnit(), pRecipe, nRecipePropertyID );
					float m_fMin = (float)RecipeGetMinPropertyValue( UIGetControlUnit(), pRecipe, nRecipePropertyID );

					ASSERT( (int)m_fMin != (int)m_fMax );
					float m_fMaxChance = (float)RecipeGetPropertyChanceOfSuccess( UIGetControlUnit(), pRecipe, nRecipePropertyID );
					//if( m_fMaxChance <= 0 )
					//{
					//	continue;	//it might have the property but the player hasn't unlocked it yet
					//}
					UIComponentSetActive(pChild, TRUE);
					UIComponentSetVisible(pChild, TRUE);

					float fValueByPCT = (float)UIGetRecipePCTOfPropertyModifiedByPlayer( nRecipeID, nRecipePropertyID );
					UI_SCROLLBAR *pScrollbar = UICastToScrollBar(pChild);		

					pScrollbar->m_fMax = MAX( m_fMax, m_fMin ) - MIN( m_fMin, m_fMax );
					pScrollbar->m_fMin = 0;
					fValueLow[nProperty] = pScrollbar->m_fMin;
					fValueHigh[nProperty] = pScrollbar->m_fMax;
					fValueRisk[nProperty] = m_fMaxChance;
					
					
					float fValue = fValueByPCT / 100.0f * (m_fMax - m_fMin) + m_fMin;
					fValueCurrent[nProperty] = fValue;
					if( m_fMax < m_fMin )
					{		
						fValue -= m_fMax;
						fValue = pScrollbar->m_fMax - fValue;
					}
					else
					{
						fValue -= m_fMin;
					}

					UIScrollBarSetValue(pScrollbar, fValue );
					float fDelta = ( ( pScrollbar->m_fMax - pScrollbar->m_fMin ) == 0 ) ? 1 : ( pScrollbar->m_fMax - pScrollbar->m_fMin );
					float Percentage = ( pScrollbar->m_fMax - pScrollbar->m_fMin == 0 ) ? 0 : ( fValue - pScrollbar->m_fMin ) / fDelta;
					fCurrentRisk[nProperty] = fValueRisk[nProperty] * Percentage;
					pScrollbar->m_fScrollHorizMax = pScrollbar->m_fMax - pScrollbar->m_fMin;
					pScrollbar->m_ScrollPos.m_fX = fValueByPCT;
					pScrollbar->m_ScrollMax.m_fX = pScrollbar->m_fMax;
					pScrollbar->m_ScrollMin.m_fX = pScrollbar->m_fMin;
					pScrollbar->m_pBar->m_nMaxValue = (int)( pScrollbar->m_fMax - pScrollbar->m_fMin );
					pScrollbar->m_pBar->m_nValue = (int)ceil( fValue + .5f);
					pChild->m_dwData = pRecipe->wCode;
					pChild->m_dwParam = i;
					pChild = pChild->m_pNextSibling;
					nProperty++;
				}
			}
			while(pChild)
			{
				UIComponentSetVisible(pChild, FALSE);
				
				pChild = pChild->m_pNextSibling;
			}
		}

		static char* cCompFloaterCheckbox[6] =
		{
			"floater property 1",				
			"floater property 2",				
			"floater property 3",				
			"floater property 4",				
			"floater property 5",				
			"floater property 6",
		};
		for( int i = 0; i < 6; i++ )
		{
			UI_COMPONENT *pCheckbox = UIComponentFindChildByName(pComponent->m_pParent, cCompFloaterCheckbox[i]);

			if( pCheckbox )
			{
				UI_BUTTONEX * pButton = UICastToButton(pCheckbox);

				if( pRecipe )
				{
					RECIPE_PROPERTIES_ENUM nRecipeProp = UICraftingGetFloaterProp( pRecipe, i );
					if( nRecipeProp != RECIPE_PROP_INVALID )
					{

						float m_fMaxChance = (float)RecipeGetPropertyChanceOfSuccess( UIGetControlUnit(), pRecipe, nRecipeProp );		
						if( m_fMaxChance > 0  )
						{
							UIComponentSetVisible( pButton, TRUE );
							UIComponentSetActive( pButton, TRUE );
							int nValue = UIGetRecipePCTOfPropertyModifiedByPlayer( nRecipeID, nRecipeProp );
							UIButtonSetDown( pButton, nValue != 0 );
							UIComponentHandleUIMessage( pButton, UIMSG_PAINT, 0, 0);
						}
						else
						{
							UIComponentSetVisible( pButton, FALSE );
							UIComponentHandleUIMessage( pButton, UIMSG_PAINT, 0, 0);
						}
					}
					else
					{
						UIComponentSetVisible( pButton, FALSE );
						UIComponentHandleUIMessage( pButton, UIMSG_PAINT, 0, 0);
					}
				}
			}
			
		}

		// get the money values in text form
		const int MAX_STRING = 64;
		WCHAR uszBuf[ MAX_STRING ];

		if( pBuyButton && pBuyPrice)
		{

			cCurrency nCost( pRecipe->nBaseCost, 0 );

			UIComponentSetActive( pBuyButton, UnitGetCurrency( UIGetControlUnit() ) >= nCost &&
											  !PlayerHasRecipe( UIGetControlUnit(), nRecipeID )  );

			WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];		
			const WCHAR *uszFormat = GlobalStringGet(GS_PRICE_COPPER_SILVER_GOLD);
			if (uszFormat)
			{
				PStrCopy(uszTextBuffer, uszFormat, arrsize(uszTextBuffer));
			}
			const int MAX_MONEY_STRING = 128;
			WCHAR uszMoney[ MAX_MONEY_STRING ];
			PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
			// replace any money token with the money value
			const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
			PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

			PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
			// replace any money token with the money value
			puszGoldToken = StringReplacementTokensGet( SR_SILVER );
			PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

			PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
			// replace any money token with the money value
			puszGoldToken = StringReplacementTokensGet( SR_GOLD );
			PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

			UILabelSetText( pBuyPrice, (nCost.IsZero() == FALSE)?uszTextBuffer:L"" );
		}

		if( pDescValues )
		{
			int nIndex = 0;
			UI_COMPONENT *pChild = pDescValues->m_pFirstChild;
			while(pChild)
			{
				if( nIndex < nProperty )
				{
					PStrPrintf( uszBuf, MAX_STRING, L"%s", StringTableGetStringByIndex( RecipeGetPropertyStringID( nValueProperty[nIndex] ) ) );
					nIndex++;
					UILabelSetText( pChild, uszBuf );
					UIComponentSetVisible(pChild, TRUE);
				}
				else
				{
					UIComponentSetVisible(pChild, FALSE);
				}

				pChild = pChild->m_pNextSibling;
			}			
		}
		UNIT *pUnit = sUICraftingSetCraftingItem( NULL, pRecipe->nRewardItemClass[ 0 ],  0 );		
		if( pCalcValues && pUnit )
		{
			int nIndex = 0;
			UI_COMPONENT *pChild = pCalcValues->m_pFirstChild;
			while(pChild)
			{
				if( nIndex < nProperty )
				{
					UIComponentSetVisible(pChild, TRUE);
					switch( nValueProperty[nIndex] )
					{
					case RECIPE_PROP_DAMAGE :
						PStrPrintf( uszBuf, MAX_STRING, L"%d-%d", (int)UnitGetStatShift(pUnit, STATS_BASE_DMG_MIN ), (int)UnitGetStatShift(pUnit, STATS_BASE_DMG_MAX ) );
						break;					
					case RECIPE_PROP_ARMOR :
					case RECIPE_PROP_MODLEVELHERALDRY :
					case RECIPE_PROP_MODLEVEL :
						PStrPrintf( uszBuf, MAX_STRING, L"%d", (int)fValueCurrent[nIndex] );
						break;
					case RECIPE_PROP_MODDURATION:
						PStrPrintf( uszBuf, MAX_STRING, L"%d", (int)fValueCurrent[nIndex] );
						break;
					case RECIPE_PROP_MOD_QUALITY_PCT:
					default:
						PStrPrintf( uszBuf, MAX_STRING, L"%d%%", (int)fValueCurrent[nIndex] );
						break;
					}
					nIndex++;
					UILabelSetText( pChild, uszBuf );
				}
				else
				{
					UIComponentSetVisible(pChild, FALSE);
				}

				pChild = pChild->m_pNextSibling;
			}			
		}

		float fTotalRisk = RecipeGetFailureChance( UIGetControlUnit(), nRecipeID ) * 100.0f;;
		if( pMaxRiskValues )
		{
			int nIndex = 0;
			UI_COMPONENT *pChild = pMaxRiskValues->m_pFirstChild;
			while(pChild)
			{
				if( nIndex < nProperty )
				{
					UIComponentSetVisible(pChild, TRUE);
					PStrPrintf( uszBuf, MAX_STRING, L"(%d%%)", (int)fValueRisk[nIndex] );
					nIndex++;
					UILabelSetText( pChild, uszBuf );
				}
				else
				{
					UIComponentSetVisible(pChild, FALSE);
				}

				pChild = pChild->m_pNextSibling;
			}			
		}

		if( pRiskValues )
		{
			int nIndex = 0;
			UI_COMPONENT *pChild = pRiskValues->m_pFirstChild;
			while(pChild)
			{
				if( nIndex < nProperty )
				{
					UIComponentSetVisible(pChild, TRUE);
					PStrPrintf( uszBuf, MAX_STRING, L"%d%%", (int)fCurrentRisk[nIndex] );
					nIndex++;
					UILabelSetText( pChild, uszBuf );
				}
				else
				{
					UIComponentSetVisible(pChild, FALSE);
				}

				pChild = pChild->m_pNextSibling;
			}			
		}

		if( pTotalRisk )
		{
			PStrPrintf( uszBuf, MAX_STRING, L"(%d%%)", (int)fTotalRisk );
			UILabelSetText( pTotalRisk, uszBuf );
		}

	}
	else
	{
		//sUICraftingGetTextOfCategoryRequired( NULL );
		UI_COMPONENT *pRecipeView = UIComponentFindChildByName(pMainComponent, "recipe view");
		UIComponentSetVisible(pRecipeView, FALSE);

	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRecipesSortName( const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	RECIPE_DEFINITION *recipe1 = *(RECIPE_DEFINITION **)a1;
	RECIPE_DEFINITION *recipe2 = *(RECIPE_DEFINITION **)a2;
	ASSERT_RETZERO(recipe1);
	ASSERT_RETZERO(recipe2);

	const UNIT_DATA *pItem1 = ItemGetData( recipe1->nRewardItemClass[ 0 ] );
	const UNIT_DATA *pItem2 = ItemGetData( recipe1->nRewardItemClass[ 0 ] );
	ASSERT_RETZERO(pItem1);
	ASSERT_RETZERO(pItem2);
	const WCHAR *s1 = StringTableGetStringByIndex(pItem1->nString);
	const WCHAR *s2 = StringTableGetStringByIndex(pItem2->nString);

	return PStrICmp(s1, s2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRecipesSortLevel( const void * a1, const void * a2 )
{
	ASSERT_RETZERO(a1);
	ASSERT_RETZERO(a2);
	RECIPE_DEFINITION *recipe1 = *(RECIPE_DEFINITION **)a1;
	RECIPE_DEFINITION *recipe2 = *(RECIPE_DEFINITION **)a2;
	ASSERT_RETZERO(recipe1);
	ASSERT_RETZERO(recipe2);

	int s1 = recipe1->nRecipeCategoriesLevelsNecessaryToCraft[0];
	int s2 = recipe2->nRecipeCategoriesLevelsNecessaryToCraft[0];
	return s1 > s2;
}

UI_MSG_RETVAL UICraftingOnPaint(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	UIComponentOnPaint(pComponent, msg, wParam, lParam);

	//UIComponentRemoveAllElements(pComponent);
	
	if( UIComponentGetFocusUnit(pComponent) == NULL )
	{
		//sRecipeSelected = INVALID_ID; //the recipe selected is invalid now
		return UIMSG_RET_NOT_HANDLED;
	}

	static const float scfBordersize = 3.0f;
	GAME *pGame = AppGetCltGame();
	if (!pGame)
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	UI_COMPONENT *pHilightBar = UIComponentFindChildByName(pComponent, "recipe highlight");
	if (pHilightBar)
	{
		UIComponentSetVisible(pHilightBar, FALSE);
	}

	BOOL bFilterRecipes = FALSE;
	UI_COMPONENT *pFilterRecipes = UIComponentFindChildByName(pComponent, "filter recipes");
	if (pFilterRecipes)
	{
		bFilterRecipes = UIButtonGetDown( UICastToButton(pFilterRecipes) );
	}
	
	UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent, "name column");
	ASSERT_RETVAL(pNamePanel, UIMSG_RET_NOT_HANDLED);
	UI_COMPONENT *pParentFrame = pNamePanel->m_pParent;	
	UI_COMPONENT *pComp = UIComponentFindChildByName(pParentFrame->m_pParent->m_pParent, "crafting recipe list scrollbar");			
	ASSERT_RETVAL(pParentFrame, UIMSG_RET_NOT_HANDLED);		
	ASSERT_RETVAL(pComp, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollbar = UICastToScrollBar(pComp);		
	UIComponentRemoveAllElements(pNamePanel);	

	UI_RECT rectNamePanel =			UIComponentGetRect(pNamePanel);
	UI_RECT rectNamePanelFull =		rectNamePanel;	
	rectNamePanel.m_fX1 += scfBordersize;		rectNamePanel.m_fX2 -= scfBordersize;	


	UIX_TEXTURE_FONT *pFont = UIComponentGetFont(pComponent);
	int nFontSize = UIComponentGetFontSize(pComponent);
	const static int szTextSize( 256 );
	WCHAR szText[szTextSize];	
	UI_ELEMENT_TEXT *pElement = NULL;

	int nCount = ExcelGetCount(pGame, DATATABLE_RECIPES);

	RECIPE_DEFINITION **ppRecipes = (RECIPE_DEFINITION **)MALLOCZ(NULL, sizeof(RECIPE_DEFINITION *) * (nCount+1));
	int nRevealed = 0;
	for (int iRecipe = 0; iRecipe < nCount; ++iRecipe )
	{
		// don't show recipes players already know! lame.
		if( !UnitIsA( UIComponentGetFocusUnit(pComponent), UNITTYPE_PLAYER ) )
		{
			if( PlayerHasRecipe( UIGetControlUnit(), iRecipe))
			{
				continue;
			}
		}
		if( PlayerHasRecipe( UIComponentGetFocusUnit(pComponent), iRecipe ) )
		{
			RECIPE_DEFINITION *pRecipe = (RECIPE_DEFINITION *) ExcelGetData(pGame, DATATABLE_RECIPES, iRecipe);
			if (pRecipe && 
				( gnSelectedRecipe == RECIPE_NONE  ||
				  gnSelectedRecipe == pRecipe->nRecipeCategoriesNecessaryToCraft[ 0 ] ) )
			{
				if( bFilterRecipes )
				{
					if( !RecipePlayerCanCreateRecipe( UIGetControlUnit(), pRecipe, TRUE ) )
					{		
						continue;
					}
				}
				ppRecipes[nRevealed++] = pRecipe;
			}
		}
	}

	qsort(ppRecipes, nRevealed, sizeof(RECIPE_DEFINITION *), sRecipesSortLevel);

	float nSpacer( (float)nFontSize * g_UI.m_fUIScaler + 1.0f  ) ;
	float fY = 0.0f;

	for (int i=0; i < nRevealed; i++)
	{
		RECIPE_DEFINITION *pRecipe = ppRecipes[i];
		DWORD dwColor = GFXCOLOR_YELLOW;//GFXCOLOR_YELLOW;
		if( !RecipePlayerCanCreateRecipe( UIGetControlUnit(), pRecipe, TRUE ) )
		{		
			dwColor = GFXCOLOR_RED;
		}
		const UNIT_DATA* pItemData = ItemGetData( pRecipe->nRewardItemClass[0] );
		ASSERT_CONTINUE( pItemData );
		// Title of Achievement
		static WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
		UICraftingGetTextOfCategoryRequired( pRecipe, newText );
		PStrPrintf( szText, szTextSize, L"%s %s", StringTableGetStringByIndex( pItemData->nString ), newText );
		pElement = UIComponentAddTextElement(pNamePanel, NULL, pFont, nFontSize, szText, UI_POSITION(scfBordersize, fY), dwColor, &rectNamePanel, UIALIGN_TOPLEFT, &UI_SIZE(rectNamePanel.Width() * g_UI.m_fUIScaler, (float)nFontSize * g_UI.m_fUIScaler + 1.0f ) ); 
		if (pElement)
		{
			pElement->m_qwData = (QWORD)pRecipe->wCode;			
		}
		int nRecipeLine = ExcelGetLineByCode( NULL, DATATABLE_RECIPES, pRecipe->wCode );	
		if (nRecipeLine == sRecipeSelected)
		{
			pHilightBar->m_pGfxElementFirst->m_Position.m_fY = fY;

			UIComponentSetVisible(pHilightBar, TRUE);
		}
		fY += nSpacer;
	}

	pScrollbar->m_fMax = MAX(0.0f, fY - pNamePanel->m_pParent->m_fHeight);
	pScrollbar->m_fScrollVertMax = pScrollbar->m_fMax;

	if (pNamePanel->m_pFrame)
	{
		UIComponentAddElement(pNamePanel, UIComponentGetTexture(pNamePanel), pNamePanel->m_pFrame, UI_POSITION(), pNamePanel->m_dwColor, &rectNamePanelFull, FALSE, 1.0f, 1.0f, &UI_SIZE(pNamePanel->m_fWidth, fY));
	}


	FREE(NULL, ppRecipes);

	sUICraftingSelectRecipe( sRecipeSelected, pNamePanel->m_pParent->m_pParent->m_pParent );


	return UIMSG_RET_HANDLED;
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingScrollBarOnScroll(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pComponent);

	UIScrollBarSetValue(pScrollBar, pScrollBar->m_ScrollPos.m_fY);

	ASSERT_RETVAL(pComponent->m_pParent, UIMSG_RET_NOT_HANDLED);

	UI_COMPONENT *pNamePanel = UIComponentFindChildByName(pComponent->m_pParent, "name column");
	UI_COMPONENT *pHighlightPanel = UIComponentFindChildByName(pComponent->m_pParent, "recipe highlight");

	ASSERT_RETVAL(pNamePanel, UIMSG_RET_NOT_HANDLED);
	ASSERT_RETVAL(pHighlightPanel, UIMSG_RET_NOT_HANDLED);

	pNamePanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;
	pHighlightPanel->m_ScrollPos.m_fY = pScrollBar->m_ScrollPos.m_fY;

	UISetNeedToRender(pComponent->m_pParent);

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
// buy the selected recipe
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingRecipeBuy(
								  UI_COMPONENT* pComponent,
								  int msg,
								  DWORD wParam,
								  DWORD lParam)
{
	if( sRecipeSelected != INVALID_ID )
	{
		c_RecipeTryBuy( (UNITID)INVALID_ID, sRecipeSelected );

		if (pComponent->m_pParent)
		{
			sRecipeSelected = INVALID_ID;
			UI_COMPONENT *pScrollbar = UIComponentFindChildByName(pComponent->m_pParent, "crafting recipe list scrollbar");
			if( pScrollbar )
			{
				UIScrollBarSetValue(UICastToScrollBar(pScrollbar), 0);
				UICraftingScrollBarOnScroll(pScrollbar, msg, 0, 0 );
			}
		}
		UI_COMPONENT *pCraftingPane = UIComponentGetByEnum( UICOMP_PANEMENU_CRAFTINGTRAINER );
		if( pCraftingPane )
		{
			UICraftingOnPaint( pCraftingPane, 0, 0, 0 );
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingFilterCheckbox(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if( !UIComponentCheckBounds( component ) || !UIComponentGetActive( component ))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (component->m_pParent)
	{
		sRecipeSelected = INVALID_ID;
		UI_COMPONENT *pScrollbar = UIComponentFindChildByName(component->m_pParent, "crafting recipe list scrollbar");
		if( pScrollbar )
		{
			UIScrollBarSetValue(UICastToScrollBar(pScrollbar), 0);
			UICraftingScrollBarOnScroll(pScrollbar, msg, 0, 0 );
		}
		return UIComponentHandleUIMessage( component->m_pParent, UIMSG_PAINT, 0, 0);
	}	

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingPropertyScrollBarOnScroll(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	ASSERT_RETVAL(pComponent, UIMSG_RET_NOT_HANDLED);

	UI_SCROLLBAR *pScrollBar = UICastToScrollBar(pComponent);
	int nRecipeLine = ExcelGetLineByCode( NULL, DATATABLE_RECIPES, (int)pComponent->m_dwData );	
	const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipeLine );

	int nProperty = (int)pComponent->m_dwParam;
	UIScrollBarSetValue(pScrollBar, pScrollBar->m_ScrollPos.m_fY);

	float m_fMin = (float)RecipeGetMinPropertyValue( UIGetControlUnit(), pRecipe, (RECIPE_PROPERTIES_ENUM)nProperty );
	float m_fMax = (float)RecipeGetMaxPropertyValue( UIGetControlUnit(), pRecipe, (RECIPE_PROPERTIES_ENUM)nProperty );
	float m_fTmp = MAX( m_fMin, m_fMax );
	m_fMin = MIN( m_fMin, m_fMax );
	m_fMax = m_fTmp;
	float fValue = (float)pScrollBar->m_pBar->m_nValue;
	fValue /= ( ( m_fMax - m_fMin ) == 0 ) ? 1 : ( m_fMax - m_fMin );
	fValue *= 100.0f;
	fValue = (float)((lParam <= 1 )?CEIL(fValue):FLOOR(fValue));
	UISetRecipePropertyModifiedByPlayer( nRecipeLine, (RECIPE_PROPERTIES_ENUM)nProperty, (int)fValue );

	sUICraftingSelectRecipe( nRecipeLine, pComponent->m_pParent->m_pParent );

	UISetNeedToRender(pComponent->m_pParent);

	//this grabs the item( also modifies it to reflect the changes )
	sUICraftingSetUnitResult( pRecipe, pComponent->m_pParent->m_pParent );
	//c_RecipeGetClientResultByIndex( pRecipe, 0 );

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingColumnOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{


	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingPaintBreakdownButton(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UIComponentSetVisible( pComponent, QuestIsQuestComplete( UIGetControlUnit(), GlobalIndexGet( GI_CRAFTING_FIRST_TASK ) ) );
	
	return UIMSG_RET_HANDLED; 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingFloaterPropertyClick(
									  UI_COMPONENT* pComponent,
									  int msg,
									  DWORD wParam,
									  DWORD lParam)
{


	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(pComponent) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(pComponent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	

	UI_BUTTONEX * pButton = UICastToButton(pComponent);

	BOOL bDown = UIButtonGetDown(pButton);


	const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( sRecipeSelected );	
	
	if( pRecipe )
	{
		RECIPE_PROPERTIES_ENUM nRecipePropertyID = UICraftingGetFloaterProp( pRecipe, (int)pComponent->m_dwParam );
		if( nRecipePropertyID == RECIPE_PROP_INVALID )
			return UIMSG_RET_HANDLED_END_PROCESS;
		
		float m_fMaxChance = (float)RecipeGetPropertyChanceOfSuccess( UIGetControlUnit(), pRecipe, nRecipePropertyID );		
		if( m_fMaxChance > 0  )
		{			
			int nRecipeLine = ExcelGetLineByCode( NULL, DATATABLE_RECIPES, pRecipe->wCode );	
			UISetRecipePropertyModifiedByPlayer( nRecipeLine, (RECIPE_PROPERTIES_ENUM)nRecipePropertyID, bDown ? 0 : 100 );

			sUICraftingSelectRecipe( nRecipeLine, pComponent->m_pParent->m_pParent );
			UI_COMPONENT *pCraftingPane = UIComponentGetByEnum( UICOMP_PANEMENU_CRAFTINGRECIPE );
			if( pCraftingPane )
			{
				UICraftingOnPaint( pCraftingPane, 0, 0, 0 );
			}
		}
	}
	return UIMSG_RET_HANDLED_END_PROCESS;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingOnActivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	UI_MSG_RETVAL eReturn = UIComponentOnActivate(pComponent, msg, wParam, lParam);

	return eReturn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingOnPostActivate(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	return UIMSG_RET_HANDLED;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingOnProgressChange(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam)
{

	
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingScrollBarOnMouseWheel(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	ASSERT_RETVAL(component, UIMSG_RET_NOT_HANDLED);
	if (!UIComponentGetActive(component))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if (!UICursorGetActive())
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//gives the player feedback when an item is created Successfully
//----------------------------------------------------------------------------
void UICraftingRecipeCreated( int nRecipe, BOOL bSuccessful )
{
	ASSERT_RETURN( nRecipe != INVALID_ID );
	const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipe );
	ASSERT_RETURN( pRecipe );
	ASSERT_RETURN( pRecipe->nRewardItemClass[0] != INVALID_ID );
	const UNIT_DATA *pItemData = ItemGetData( pRecipe->nRewardItemClass[0] );
	ASSERT_RETURN( pItemData );
	const WCHAR *pStrConsoleMessage = NULL;
	if( bSuccessful )
	{
		pStrConsoleMessage = GlobalStringGet( GS_RECIPE_ITEM_CREATED_CONSOLE_MESSAGE );
	}
	else
	{
		pStrConsoleMessage = GlobalStringGet( GS_RECIPE_ITEM_FAILED_CONSOLE_MESSAGE );
		//pStrConsoleMessage = GlobalStringGet( GS_RECIPE_ITEM_CREATED_CONSOLE_MESSAGE );
	}
	WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
	PStrPrintf( newText, MAX_STRING_ENTRY_LENGTH, L"%s", pStrConsoleMessage );
	const WCHAR *pStrItemName = StringTableGetStringByIndex( pItemData->nString );
	PStrReplaceToken(  newText, MAX_STRING_ENTRY_LENGTH, StringReplacementTokensGet( SR_ITEM ), pStrItemName );
	if( bSuccessful )
	{
		UIAddChatLine( CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_SERVER), newText ); 
		int nSoundId = GlobalIndexGet(GI_SOUND_CRAFTING_SUCCESS);
		if (nSoundId != INVALID_LINK)
		{
			c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
		}
	}
	else
	{
		UIAddChatLine( CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_CLIENT_ERROR), newText ); 
		int nSoundId = GlobalIndexGet(GI_SOUND_CRAFTING_FAILURE);
		if (nSoundId != INVALID_LINK)
		{
			c_SoundPlay(nSoundId, &c_SoundGetListenerPosition(), NULL);
		}
	}
	UIShowQuickMessage( newText, LEVELCHANGE_FADEOUTMULT_DEFAULT );
	//UIShowGenericDialog(GlobalStringGet( GS_MENU_HEADER_ALERT ), newText);

	UI_COMPONENT *pCraftingPane = UIComponentGetByEnum( UICOMP_PANEMENU_CRAFTINGRECIPE );
	ASSERT_RETURN( pCraftingPane );
	UICraftingOnPaint( pCraftingPane, 0, 0, 0 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingOnLButtonDown(
										UI_COMPONENT* component,
										int msg,
										DWORD wParam,
										DWORD lParam)
{
	//sUICraftingGetTextOfCategoryRequired( NULL );
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x, y;
	UIGetCursorPosition(&x, &y);
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( component->m_pParent && 
		!UIComponentCheckBounds(component->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	x -= pos.m_fX;
	y -= pos.m_fY;

	// we need logical coords for the element comparisons ( getskillatpos )
	float logx = x / UIGetScreenToLogRatioX(component->m_bNoScale);
	float logy = y / UIGetScreenToLogRatioY(component->m_bNoScale);
	float fScrollOffsetY = UIComponentGetScrollPos(component).m_fY;
	if( component->m_pParent )
	{
		x -= component->m_Position.m_fX;
		logx -= component->m_Position.m_fX;
		fScrollOffsetY += UIComponentGetScrollPos(component->m_pParent).m_fY;
		logy += fScrollOffsetY;
	}

	int nRecipeCode = INVALID_LINK;
	UI_GFXELEMENT *pChild = component->m_pGfxElementFirst;
	while(pChild)
	{

		if ( UIElementCheckBounds(pChild, logx, logy ) && 
			(pChild->m_qwData) != (QWORD)INVALID_ID &&
			(pChild->m_qwData >> 16 == 0))
		{
			skillpos.m_fX = pChild->m_Position.m_fX;
			skillpos.m_fY = pChild->m_Position.m_fY;
			nRecipeCode = (int)(pChild->m_qwData & 0xFFFF) ;
			break;
		}
		pChild = pChild->m_pNextElement;
	}
	if( nRecipeCode != INVALID_LINK )
	{
		int nRecipeLine = ExcelGetLineByCode( NULL, DATATABLE_RECIPES, nRecipeCode );	
		sUICraftingSelectRecipe( nRecipeLine, component->m_pParent->m_pParent->m_pParent ); //select the recipe
		UI_COMPONENT *pCraftingPane = component->m_pParent->m_pParent->m_pParent;//UIComponentGetByEnum( UICOMP_PANEMENU_CRAFTINGRECIPE );
		if( pCraftingPane )
		{
			UICraftingOnPaint( pCraftingPane, 0, 0, 0 );
		}
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftinglotsOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam)
{
	//int nCursorSkillID = UIGetCursorSkill();
	

	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	GAME* pGame = AppGetCltGame();
	if (!pGame)
		return UIMSG_RET_NOT_HANDLED;

	UNIT *pPlayer = GameGetControlUnit(pGame);
	if (!pPlayer)
		return UIMSG_RET_NOT_HANDLED;

	return UIMSG_RET_HANDLED_END_PROCESS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingOnMouseHover(
									   UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (UIGetCursorUnit() != INVALID_ID || !UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	float x = (float)wParam;
	float y = (float)lParam;
	UI_POSITION pos;
	UI_POSITION skillpos;
	if (!UIComponentCheckBounds(component, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	if ( component->m_pParent && 
		 !UIComponentCheckBounds(component->m_pParent, x, y, &pos))
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	return UIMSG_RET_HANDLED_END_PROCESS;
}



inline void sUICraftingAddClassID( int *nClassIDArray, int *nClassIDCountArray, int nCount, int nClassID )
{
	for( int t = 0; t < nCount; t++ )
	{
		if( nClassIDArray[ t ] == INVALID_ID )
		{
			nClassIDArray[t] = nClassID;
			nClassIDCountArray[t] = 1;
			return;
		}
		else if( nClassIDArray[ t ] == nClassID )
		{
			nClassIDCountArray[t]++;
			return;
		}
	}
}
inline void sUICraftingGetBreakdownUIComponents( UI_COMPONENT *pComponentArray[5] )
{
	
	pComponentArray[0] = UIComponentGetByEnum( UICOMP_CRAFTING_BREAKDOWN_ITEM1 );
	pComponentArray[1] = UIComponentGetByEnum( UICOMP_CRAFTING_BREAKDOWN_ITEM2 );
	pComponentArray[2] = UIComponentGetByEnum( UICOMP_CRAFTING_BREAKDOWN_ITEM3 );
	pComponentArray[3] = UIComponentGetByEnum( UICOMP_CRAFTING_BREAKDOWN_ITEM4 );
	pComponentArray[4] = UIComponentGetByEnum( UICOMP_CRAFTING_BREAKDOWN_ITEM5 );
}
//sent from the server after an item is broken down
void cUICraftingShowItemsBrokenDownMsg( BYTE *pMessage )
{
	ASSERT_RETURN( pMessage );
	MSG_SCMD_CRAFT_BREAKDOWN * msg = ( MSG_SCMD_CRAFT_BREAKDOWN * )pMessage;
	UI_COMPONENT *pResults[5];
	sUICraftingGetBreakdownUIComponents( pResults );
	int nClassIDs[ 5 ] = { INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID };
	int nClassIDCount[ 5 ] = { 0, 0, 0, 0, 0 };
	sUICraftingAddClassID( nClassIDs, nClassIDCount, 5, msg->classID0 );
	sUICraftingAddClassID( nClassIDs, nClassIDCount, 5, msg->classID1 );
	sUICraftingAddClassID( nClassIDs, nClassIDCount, 5, msg->classID2 );
	sUICraftingAddClassID( nClassIDs, nClassIDCount, 5, msg->classID3 );
	sUICraftingAddClassID( nClassIDs, nClassIDCount, 5, msg->classID4 );
	for( int t = 0; t < 5; t++ )
	{
		ASSERT_RETURN( pResults[ t ] );
		sUICraftingSetCraftingItem( pResults[t], nClassIDs[t], nClassIDCount[t] );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingBreakDownItem( UI_COMPONENT* component,
									   int msg,
									   DWORD wParam,
									   DWORD lParam)
{
	if (!UIComponentGetVisible(component) ||
		!UIGetControlUnit() )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	if( !UIComponentCheckBounds( component ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}

	GAME *pGame = AppGetCltGame();
	//GS_ERROR_ON_ITEM_BREAKDOWN,
	//GS_ERROR_BREAKING_DOWN_NEED_ITEM,
	UNIT *pCursorUnit = UnitGetById( pGame, UIGetCursorUnit() );

	if( !pCursorUnit )
	{
		UIAddChatLine( CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_CLIENT_ERROR), GlobalStringGet( GS_ERROR_BREAKING_DOWN_NEED_ITEM ) ); 
		return UIMSG_RET_NOT_HANDLED;
	}
	if( !CRAFTING::CraftingItemCanBeBrokenDown( pCursorUnit, UIGetControlUnit() ) )
	{
		UIAddChatLine( CHAT_TYPE_CLIENT_ERROR, ChatGetTypeColor(CHAT_TYPE_CLIENT_ERROR), GlobalStringGet( GS_ERROR_ON_ITEM_BREAKDOWN ) ); 
		UIClearCursor();
		return UIMSG_RET_HANDLED;
	}
	
	//lets dismantle the item
	
	c_ItemTryDismantle( pCursorUnit, FALSE ); 

	return UIMSG_RET_HANDLED;//UIMSG_RET_HANDLED_END_PROCESS;
}

inline void sUIShowInventoryBag( UI_COMPONENT* component )
{
	// clear cursor states (if we had them set at all)
	UISetCursorUnit( INVALID_ID, TRUE );
	UISetCursorUseUnit( INVALID_ID );
	UISetCursorState( UICURSOR_STATE_POINTER );

	ASSERT_RETURN( component );
	UI_COMPONENT *pInvLocation = UIComponentGetByEnum( UICOMP_INVENTORY_BACKPACK );
	UI_COMPONENT *pInvCraftingLocation = UIComponentGetByEnum( UICOMP_INVENTORY_CRAFTING_BAG );
	ASSERT_RETURN( pInvLocation );
	ASSERT_RETURN( pInvCraftingLocation );	
	switch( sUIInventoryShowBag )
	{
	default:
	case KINVENTORY_BACKPACK:
		{
			UIComponentSetVisible( pInvLocation, TRUE );
			UIComponentSetVisible( pInvCraftingLocation, FALSE );
		}
		break;
	case KINVENTORY_CRAFTING:
		{
			UIComponentSetVisible( pInvLocation, FALSE );
			UIComponentSetVisible( pInvCraftingLocation, TRUE );
		}
		break;
	};
	UIComponentSetActive( pInvLocation, UIComponentGetVisible( pInvLocation ) );
	UIComponentSetActive( pInvCraftingLocation, UIComponentGetVisible( pInvCraftingLocation ) );
	UIComponentHandleUIMessage(component, UIMSG_PAINT, 0, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingHandleInventoryLocations( UI_COMPONENT* component,
												  int msg,
												  DWORD wParam,
												  DWORD lParam)
{
	UI_COMPONENT *pResults[5];
	sUICraftingGetBreakdownUIComponents( pResults );
	UNITID nPlayerID = (UNITID)UnitGetId( UIGetControlUnit() );
	for( int t = 0; t < 5; t++ )
	{
		ASSERT_CONTINUE( pResults[ t ] );
		if( UIComponentGetFocusUnitId( pResults[ t ] ) == INVALID_ID ||
			UIComponentGetFocusUnitId( pResults[ t ] ) == nPlayerID )
		{
			UIComponentSetVisible( pResults[ t ], FALSE );
		}
	}

	sUIShowInventoryBag( component );
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingShowBackpack( UI_COMPONENT* component,
												 int msg,
												 DWORD wParam,
												 DWORD lParam)
{
	if( !UIComponentCheckBounds( component ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	sUIInventoryShowBag = KINVENTORY_BACKPACK;
	sUIShowInventoryBag( component->m_pParent );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UICraftingShowCraftingInventory( UI_COMPONENT* component,
												 int msg,
												 DWORD wParam,
												 DWORD lParam)
{
	if( !UIComponentCheckBounds( component ) )
	{
		return UIMSG_RET_NOT_HANDLED;
	}
	sUIInventoryShowBag = KINVENTORY_CRAFTING;
	sUIShowInventoryBag( component->m_pParent );
	return UIMSG_RET_HANDLED;
}

static
void sRecipeComboSetSelection(UI_COMPONENT * pComboComp, int nZoneSelected )
{
	ASSERT_RETURN(pComboComp);
	GAME * pGame = AppGetCltGame();	
	UI_COMBOBOX * pCombo = UICastToComboBox(pComboComp);
	UITextBoxClear(pCombo->m_pListBox);
	UNIT *pUnitControlingPanel = UnitGetById( pGame, UIComponentGetFocusUnitId( pComboComp ) );
	int nRecipeCategoryCount = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_RECIPECATEGORIES);
	BOOL nRecipeCategories[ 50 ];
	ZeroMemory( nRecipeCategories, sizeof( BOOL ) * 50 );
	
	if( pUnitControlingPanel )
	{		
		STATS_ENTRY tStatsUnitTypes[ 2000 ]; //hard coded for now. Testing..
		int nUnitRecipesKnown = UnitGetStatsRange( pUnitControlingPanel, STATS_RECIPES_LEARNED, 0, tStatsUnitTypes, 2000 );	
		for( int t = 0; t < nUnitRecipesKnown; t++ )
		{
			int nRecipeID = STAT_GET_PARAM( tStatsUnitTypes[ t ].stat );			
			if( nRecipeID != INVALID_ID )
			{				
				const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipeID );
				if( pRecipe )
				{
					nRecipeCategories[ pRecipe->nRecipeCategoriesNecessaryToCraft[ 0 ] ] = TRUE;
				}
			}
		}					
	}
	else
	{
		
		for( int t = 0; t < nRecipeCategoryCount; t++ )		
			nRecipeCategories[ t ] = TRUE;
		
	}

	int nSelectionIndex = nZoneSelected;
	int nIndexAll = -1;
	int nIndices = 0;
	
	WCHAR Str[MAX_XML_STRING_LENGTH];
	PStrPrintf(Str, arrsize(Str), L"All Recipes" );
	nIndices++;
	UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)RECIPE_NONE);
	for( int i = 0; i < nRecipeCategoryCount; i++ )
	{
		if( nRecipeCategories[ i ] )
		{
			const RECIPE_CATEGORY *pRecipeCategory = RecipeGetCategoryDefinition( i );
			WCHAR Str[MAX_XML_STRING_LENGTH];
			PStrPrintf(Str, arrsize(Str), StringTableGetStringByIndex( pRecipeCategory->nString ) );
			nIndices++;
			UIListBoxAddString(pCombo->m_pListBox, Str, (QWORD)i);
		}
	}

	if( nSelectionIndex >= nIndices )
	{
		nSelectionIndex = 0;
	}
	nSelectionIndex = (nSelectionIndex < 0) ? nIndexAll : nSelectionIndex;

	UIComboBoxSetSelectedIndex(pCombo, nSelectionIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeComboOnActivate( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pRecipeCombo = UIComponentFindChildByName( component, UI_RECIPE_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pRecipeCombo);

	sRecipeComboSetSelection( pCombo, gnSelectedRecipe + 1 );


	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIRecipeComboOnSelect( UI_COMPONENT* component, int msg, DWORD wParam, DWORD lParam )
{
	UI_COMPONENT * pRecipeCombo = UIComponentFindChildByName( component, UI_RECIPE_COMBO );
	UI_COMBOBOX * pCombo = UICastToComboBox(pRecipeCombo);
	int nIndex = UIComboBoxGetSelectedIndex(pCombo);

	if (nIndex == INVALID_ID)
	{
		return UIMSG_RET_HANDLED;
	}

	QWORD nRecipeIndexData = UIComboBoxGetSelectedData(pCombo);

	gnSelectedRecipe = (int)(nRecipeIndexData);
	sRecipeSelected = INVALID_ID;
	UI_COMPONENT *pCraftingPane = component->m_pParent;

	UI_COMPONENT *pComp = UIComponentFindChildByName(pCraftingPane, "crafting recipe list scrollbar");			
	UI_SCROLLBAR *pScrollbar = UICastToScrollBar(pComp);	
	UIScrollBarSetValue(pScrollbar, 0 );
	UICraftingScrollBarOnScroll( pComp, 0, 0, 0 );
	UICraftingOnPaint( pCraftingPane, 0, 0, 0 );
	return UIMSG_RET_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void InitComponentTypesCrafting(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize)
{	

UI_XML_ONFUNC gUIXmlFunctions[] =
	{	// function name						function pointer
		{ "UICraftingOnActivate",				UICraftingOnActivate },
		{ "UICraftingOnPostActivate",			UICraftingOnPostActivate },
		{ "UICraftingOnPaint",					UICraftingOnPaint },
		{ "UICraftingOnProgressChange",			UICraftingOnProgressChange },
		{ "UICraftingColumnOnClick",			UICraftingColumnOnClick },
		{ "UICraftingScrollBarOnScroll",		UICraftingScrollBarOnScroll },
		{ "UICraftingScrollBarOnMouseWheel",	UICraftingScrollBarOnMouseWheel },
		{ "UICraftingOnMouseHover",				UICraftingOnMouseHover },		
		{ "UICraftingOnLButtonDown",			UICraftingOnLButtonDown },		
		{ "UICraftingBreakDownItem",			UICraftingBreakDownItem },
		{ "UICraftingHandleInvLoc",				UICraftingHandleInventoryLocations },
		{ "UIInventoryShowBackpack",			UICraftingShowBackpack },
		{ "UIInventoryShowCraftingItems",		UICraftingShowCraftingInventory },
		{ "UICraftingPropertyScrollBarOnScroll",UICraftingPropertyScrollBarOnScroll },
		{ "UIRecipeComboOnSelect",				UIRecipeComboOnSelect },
		{ "UIRecipeComboOnActivate",			UIRecipeComboOnActivate },
		{ "UICraftingRecipeBuy",				UICraftingRecipeBuy },
		{ "UICraftingFloaterPropertyClick",		UICraftingFloaterPropertyClick },
		{ "UICraftingPaintBreakdownButton",		UICraftingPaintBreakdownButton },
		{ "UICraftingFilterCheckbox",			UICraftingFilterCheckbox },

	};

	// Add on the message handler functions for the local components
	int nOldSize = nXmlFunctionsSize;
	nXmlFunctionsSize += sizeof(gUIXmlFunctions);
	pUIXmlFunctions = (UI_XML_ONFUNC *)REALLOC(NULL, pUIXmlFunctions, nXmlFunctionsSize);
	memcpy((BYTE *)pUIXmlFunctions + nOldSize, gUIXmlFunctions, sizeof(gUIXmlFunctions));
}


#endif
