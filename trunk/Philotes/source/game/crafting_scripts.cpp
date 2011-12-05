#include "StdAfx.h"
#include "crafting_scripts.h"
#include "imports.h"
#include "script_priv.h"
#include "recipe.h"
#include "treasure.h"
#include "items.h"
#include "LevelAreas.h"
#include "Picker.h"
#include "s_crafting.h"
#include "script.h"
#include "gameglobals.h"
#include "crafting.h"
#include "skills.h"
#include "crafting_properties.h"
#include "states.h"
//----------------------------------------------------------------------------
//struct for holding crafting data
//----------------------------------------------------------------------------
struct CRAFTING_SCRIPT_CONTEXT
{	
	GAME					*pGame;
	SCRIPT_CONTEXT			*pContext;
	UNIT					*pPlayer;
	UNIT					*pItem;
	int						nRecipeID;
	const RECIPE_DEFINITION	*pRecipeDefinition;
	int						nCategoryID;
	const RECIPE_CATEGORY	*pRecipeCategory;
};

//----------------------------------------------------------------------------
//returns the recipe definition
//----------------------------------------------------------------------------
inline const RECIPE_DEFINITION * sCraftingScript_RecipeDef( SCRIPT_CONTEXT *pContext )
{
	ASSERT_RETNULL( pContext );
	ASSERT_RETNULL( pContext->nParam1 != INVALID_ID );
	return RecipeGetDefinition( pContext->nParam1 );
}

//----------------------------------------------------------------------------
//returns the recipe category definition
//----------------------------------------------------------------------------
inline const RECIPE_CATEGORY * sCraftingScript_RecipeCat( SCRIPT_CONTEXT *pContext )
{
	ASSERT_RETNULL( pContext );
	ASSERT_RETNULL( pContext->nParam2 != INVALID_ID );
	return RecipeGetCategoryDefinition( pContext->nParam2 );
}

//----------------------------------------------------------------------------
//Does error checks for script
//----------------------------------------------------------------------------
inline BOOL sCraftingScript_ParamsCorrect( SCRIPT_CONTEXT *pContext )
{
	ASSERT_RETFALSE( pContext );
	ASSERT_RETFALSE( pContext->game );
	ASSERT_RETFALSE( pContext->unit );
	ASSERT_RETFALSE( pContext->object );
	//not anymore
	//if( IS_CLIENT( pContext->game ) )  //always has to be server
	//	return FALSE;
	ASSERT_RETFALSE( pContext->nParam1 != INVALID_ID );
	ASSERT_RETFALSE( pContext->nParam2 != INVALID_ID );
	if( UnitGetGenus( pContext->unit ) != GENUS_PLAYER &&
		UnitGetGenus( pContext->object ) == GENUS_PLAYER )
	{
		return TRUE;
	}
	ASSERT_RETFALSE( UnitGetGenus( pContext->unit ) == GENUS_PLAYER );
	ASSERT_RETFALSE( UnitGetGenus( pContext->object ) == GENUS_ITEM );
	return TRUE;
}

//----------------------------------------------------------------------------
//returns FALSE if there are any errors. 
//----------------------------------------------------------------------------
inline BOOL sCraftingScript_FillCraftingContext( SCRIPT_CONTEXT * pContext,
												 CRAFTING_SCRIPT_CONTEXT &context )
{
	if( !sCraftingScript_ParamsCorrect( pContext ) )
		return FALSE;		//something wasn't correct
	context.nRecipeID = pContext->nParam1;
	context.pRecipeDefinition = RecipeGetDefinition( context.nRecipeID );
	ASSERT_RETFALSE( context.pRecipeDefinition );
	context.nCategoryID = pContext->nParam2;	
	context.pRecipeCategory = RecipeGetCategoryDefinition( pContext->nParam2 );
	ASSERT_RETFALSE( context.pRecipeCategory );
	context.pItem = pContext->object;
	context.pPlayer = pContext->unit;
	if( UnitGetGenus( context.pPlayer ) != GENUS_PLAYER &&
		UnitGetGenus( context.pItem ) == GENUS_PLAYER )
	{
		context.pItem = pContext->unit;
		context.pPlayer = pContext->object;
	}

	context.pGame = pContext->game;
	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCraftingGetSkillLvlParent( SCRIPT_CONTEXT * pContext,							  
								 int nParentSkill )
{
	int nSkillLvl = SkillGetLevel( pContext->unit, nParentSkill );
	nSkillLvl += pContext->nSkillLevel - SkillGetLevel( pContext->unit, pContext->nSkill );
	return nSkillLvl;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMCraftingModifyQuality( SCRIPT_CONTEXT * pContext,							  
							 int nModAmount )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
#if !ISVERSION(CLIENT_ONLY_VERSION)
	//Get the player's modified chance value
	float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_QUALITY );
	fBaseChance += (float)nModAmount;
	int nQuality( ItemGetQuality( craftingData.pItem ) );
	if( fBaseChance > RandGetFloat( craftingData.pGame, 0, 100 ) )
	{

		nQuality = s_TreasureSimplePickQuality( craftingData.pGame,			                               
											    UnitGetStat( craftingData.pPlayer, STATS_LEVEL ),
											    UnitGetData( craftingData.pItem) );
		if( nQuality <= 1 )
			nQuality++;			//1 is just normal and since they got the random roll lets always just boost it by 1
		//ok now make sure we have a valid quality for the item
		const ITEM_QUALITY_DATA *pQualityData = (const ITEM_QUALITY_DATA *)ExcelGetData(craftingData.pGame, DATATABLE_ITEM_QUALITY, nQuality);
		ASSERTX_RETZERO( pQualityData, "Expected quality data" );
		while( !pQualityData->bAllowCraftingToCreateQuality )
		{
			nQuality--;
			pQualityData = (const ITEM_QUALITY_DATA *)ExcelGetData(craftingData.pGame, DATATABLE_ITEM_QUALITY, nQuality);			
		}
		
		ItemSetQuality( craftingData.pItem, nQuality ); 

	}
#endif
	return 1;
}


//----------------------------------------------------------------------------
//This function computes by weights what the value of the crafted item is based
//on the amount the player changed the percent chance
//----------------------------------------------------------------------------
inline int ComputeCraftingGraphResult( CRAFTING_SCRIPT_CONTEXT &craftingData,
									  int nRecipeProperty,									  
									  float fPlayerPercent )
{
	//we give every result value a weight
	float fWeightStart = 1.0f - fPlayerPercent;
	float fWeightFinish = fPlayerPercent;	
	int nMinValue = RecipeGetMinPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipeProperty );
	int nMaxValue = RecipeGetMaxPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipeProperty );
	int nValueDifference = nMaxValue - nMinValue;
	PickerStart( craftingData.pGame, picker);
	for( int t = 0; t < nValueDifference; t++ )
	{
		float fDif = (float)t/(float)(nValueDifference - 1);
		float fPercent = abs( fWeightStart + ( fWeightFinish * fDif ) );
		int nWeight = (int)(fPercent * 1000.0f);
		int nValueIs = nMinValue + t;
		PickerAdd(MEMORY_FUNC_FILELINE(craftingData.pGame, nValueIs, nWeight ));
	}

	return PickerChoose( craftingData.pGame );

}
//----------------------------------------------------------------------------
//Adds a damage type resistance to armor
//----------------------------------------------------------------------------
int VMCraftingAddResDmgType( SCRIPT_CONTEXT * pContext,							  
							 int nDmgType,
							 int nRecipePropID )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
	ASSERT_RETZERO( nDmgType != INVALID_ID );
	ASSERT_RETZERO( nRecipePropID != INVALID_ID );

	int nBaseChance = RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipePropID );
	if( nBaseChance > 1 )
	{
		int nAmountToChange = RecipeGetMaxPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipePropID );
		if( nAmountToChange == 0 )
			return 1;	
		int nShiftAmount = StatsGetShift( craftingData.pGame, STATS_ELEMENTAL_VULNERABILITY );
		nAmountToChange = nAmountToChange << nShiftAmount;
		int nMinValue = (1 << nShiftAmount );	
		UnitSetStat( craftingData.pItem, STATS_ELEMENTAL_VULNERABILITY, nDmgType, -MAX( nMinValue, nAmountToChange ) );	
	}

	return 1;
}

//----------------------------------------------------------------------------
//Adds affixes to items
//----------------------------------------------------------------------------
int VMCraftingAddAffixes( SCRIPT_CONTEXT * pContext,							  						 
						 int nRecipePropID )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;	
	ASSERT_RETZERO( nRecipePropID != INVALID_ID );
#if !ISVERSION(CLIENT_ONLY_VERSION)		
	if( IS_SERVER( pContext->game ) )
	{
		int nBaseChance = RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipePropID );
		if( nBaseChance > 1 )
		{
			int nNumOfAffixes = RecipeGetMaxPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipePropID );
			if( nNumOfAffixes == 0 )
				return 1;	
			DWORD dwAffixPickFlags = 0;
			SETBIT( dwAffixPickFlags, APF_ALLOW_PROCS_BIT );
			for( int t = 0 ;t < nNumOfAffixes; t++ )
			{
				s_AffixPick( craftingData.pItem, dwAffixPickFlags, INVALID_LINK, NULL, INVALID_LINK );
			}
			s_AffixApplyAttached( craftingData.pItem );	//apply the affixes now	
			//s_AffixApply( craftingData.pItem );
		}
	}
#endif
	return 1;
}


//----------------------------------------------------------------------------
//Adds a damage type to a weapon
//----------------------------------------------------------------------------
int VMCraftingAddDmgType( SCRIPT_CONTEXT * pContext,							  
						  int nDmgType,
						  int nRecipePropID,
						  int nState )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
	ASSERT_RETZERO( nDmgType != INVALID_ID );
	ASSERT_RETZERO( nRecipePropID != INVALID_ID );
	
	int nChanceMod = MIN( 100, RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipePropID ) );
	if( nChanceMod > 1 )
	{		
		int nAmountToChangePct = RecipeGetMaxPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipePropID );
		if( nAmountToChangePct == 0 )
			return 1;	
		int nShiftAmount = StatsGetShift( craftingData.pGame, STATS_BASE_DMG_MAX );		
		int nMinValue = (1 << nShiftAmount );	
		int nDmgAmount = UnitGetStat( craftingData.pItem, STATS_BASE_DMG_MAX );		
		int nDmgBonus = PCT( nDmgAmount, nAmountToChangePct );
		nDmgBonus = MAX( nMinValue, nDmgBonus );
		nDmgBonus = nDmgBonus >> nShiftAmount;
		UnitSetStat( craftingData.pItem, STATS_DAMAGE_BONUS, nDmgType, nDmgBonus );	
		if( nState != INVALID_ID )
		{
			if( IS_CLIENT( craftingData.pGame ) )
			{
				c_StateSet( craftingData.pItem, craftingData.pPlayer, nState, 0, 0, INVALID_ID );
			}
			else
			{
				s_StateSet( craftingData.pItem, craftingData.pPlayer, nState, 0, INVALID_ID, NULL );
			}
		}
	}

	return 1;
}



//----------------------------------------------------------------------------
//Modifies the attack speed for weapons
//----------------------------------------------------------------------------
int VMCraftingModifyAttackSpeed( SCRIPT_CONTEXT * pContext,							  
								 int nModAmount )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;

	//lets check to see if this recipe has this property
	if( RecipeHasProperty( craftingData.pRecipeDefinition, RECIPE_PROP_ATTACKSPEED ) == FALSE &&
		RecipeHasProperty( craftingData.pRecipeDefinition, RECIPE_PROP_MELEESPEED ) == FALSE &&
		RecipeHasProperty( craftingData.pRecipeDefinition, RECIPE_PROP_RANGEDSPEED ) == FALSE)
		return 0;

	if( UnitIsA( craftingData.pItem, UNITTYPE_MELEE ) )
	{
		//lets compute the players risk that succeeded
		float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_ATTACKSPEED );	
		fBaseChance += (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_MELEESPEED );	
		fBaseChance += (float)nModAmount;
		int nAmountToChange = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_ATTACKSPEED, fBaseChance );
		nAmountToChange += RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_MELEESPEED, fBaseChance );
		//add on the new speed bonus
		float fAttackSpeed = (float)UnitGetStat( craftingData.pItem, STATS_MELEE_SPEED_BONUS ) + nAmountToChange;				
		fAttackSpeed = MAX( 0.0f, fAttackSpeed );
		UnitSetStat( craftingData.pItem, STATS_MELEE_SPEED_BONUS, (int)fAttackSpeed );		
	}
	else
	{
		//lets compute the players risk that succeeded
		float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_ATTACKSPEED );	
		fBaseChance += (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_RANGEDSPEED );	
		fBaseChance += (float)nModAmount;
		int nAmountToChange = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_ATTACKSPEED, fBaseChance );
		nAmountToChange += RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_RANGEDSPEED, fBaseChance );
		//add on the new speed bonus
		float fAttackSpeed = (float)UnitGetStat( craftingData.pItem, STATS_RANGED_SPEED_BONUS ) + nAmountToChange;				
		fAttackSpeed = MAX( 0.0f, fAttackSpeed );
		UnitSetStat( craftingData.pItem, STATS_RANGED_SPEED_BONUS, (int)fAttackSpeed );		
	}

	return 1;
}


//----------------------------------------------------------------------------
//Modifies the damage on weapons
//----------------------------------------------------------------------------
int VMCraftingModifyDamage( SCRIPT_CONTEXT * pContext,							  
							int nModAmount )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;

	//lets check to see if this recipe has this property
	if( RecipeHasProperty( craftingData.pRecipeDefinition, RECIPE_PROP_DAMAGE ) == FALSE )
		return 0;

	//lets compute the players risk that succeeded
	float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_DAMAGE );	
	fBaseChance += (float)nModAmount;
	
	float fDamageRangeModify(0.0f);
	if( RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_DAMAGERANGEMODIFY ) > 0 )
	{
		fDamageRangeModify = (float)RecipeGetMaxPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_DAMAGERANGEMODIFY );
	}
	
	int nMaxDamage = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_DAMAGE, fBaseChance );
	int nMinDamage = 0;
	int nRow = ExcelGetLineByCode( EXCEL_CONTEXT( craftingData.pGame ), DATATABLE_ITEMS, craftingData.pItem->pUnitData->wCode );
	int nMinDmgPCT = ExcelEvalScript(craftingData.pGame, NULL, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeMinBaseDmg), nRow);
	int nMaxDmgPCT = ExcelEvalScript(craftingData.pGame, NULL, NULL, NULL, DATATABLE_ITEMS, OFFSET(UNIT_DATA, codeMaxBaseDmg), nRow);
	float nPercentRangeOfDamages = ((float)nMinDmgPCT/(float)nMaxDmgPCT);
	nPercentRangeOfDamages += nPercentRangeOfDamages * (fDamageRangeModify/100.0f);
	nMinDamage = (int)CEIL( (float)nMaxDamage * nPercentRangeOfDamages );
	//get the default values
	int nShiftAmount = StatsGetShift( craftingData.pGame, STATS_BASE_DMG_MIN );
	int nMinValue = (1 << nShiftAmount );	
	//add on the new values from the recipe
	nMinDamage = MAX( nMinValue, nMinDamage << nShiftAmount );	
	nMaxDamage = MAX( nMinValue, nMaxDamage << nShiftAmount );
	//set the new stats
	UnitSetStat( craftingData.pItem, STATS_BASE_DMG_MIN, nMinDamage );
	UnitSetStat( craftingData.pItem, STATS_BASE_DMG_MAX, nMaxDamage );

	return 1;
}

//----------------------------------------------------------------------------
//Modifies the damage on weapons
//----------------------------------------------------------------------------
int VMCraftingModifyDuration( SCRIPT_CONTEXT * pContext,							  
							  int nModAmount )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;

	//lets compute the players risk that succeeded
	float fPercentInvested = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_MODDURATION );	
	fPercentInvested += (float)nModAmount;
	int nValue = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_MODDURATION, fPercentInvested );
	UnitSetStat( craftingData.pItem, STATS_ITEM_CRAFTING_EVENT_COUNT, nValue );

	return 1;
}



//----------------------------------------------------------------------------
//Modifies the mod level on items
//----------------------------------------------------------------------------
int VMCraftingModifyModLevel( SCRIPT_CONTEXT * pContext,							  
							  int nModAmount,
							  int nValueAmount)
{

	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;



	//lets do a double check here
	if( !CRAFTING::CraftingItemCanHaveModLevel( craftingData.pItem ) )
	{
		return 1;	//we can't set crafting levels on anythign but crafting mods and equipable items
	}

	RECIPE_PROPERTIES_ENUM nRecipeProp( RECIPE_PROP_MODLEVEL );
	if( CRAFTING::CraftingIsCraftingModItem(craftingData.pItem ) )
		nRecipeProp = RECIPE_PROP_MODLEVELHERALDRY;

	//lets check to see if this recipe has this property
	if( RecipeHasProperty( craftingData.pRecipeDefinition, nRecipeProp ) == FALSE )
		return 0;
	//{
		float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, nRecipeProp );					
		fBaseChance += (float)nModAmount;
		int nModLevel = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, nRecipeProp, fBaseChance );		
		nModLevel = MAX( 1, nModLevel );
		UnitSetStat( craftingData.pItem, STATS_ITEM_CRAFTING_LEVEL, nModLevel );
		//CRAFTING::CraftingAddBlankModSlot( craftingData.pItem, nModLevel );
	//}

	return 1;
}
//Modifies armor of an item
//----------------------------------------------------------------------------
int VMCraftingModifyAC( SCRIPT_CONTEXT * pContext,							  
					   int nModAmount )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;

	//lets check to see if this recipe has this property
	if( RecipeHasProperty( craftingData.pRecipeDefinition, RECIPE_PROP_ARMOR ) == FALSE )
		return 0;

	//lets compute the players risk that succeeded
	float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_ARMOR );		
	fBaseChance += (float)nModAmount;
	int nNewACAmount = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_ARMOR, fBaseChance );	
	nNewACAmount = MAX( 1, nNewACAmount )  << StatsGetShift( craftingData.pGame, STATS_AC );
	//set the new AC stat
	UnitSetStat( craftingData.pItem, STATS_AC, DAMAGE_TYPE_ALL, nNewACAmount);

	return 1;
}

//----------------------------------------------------------------------------
//Modifies Movement speed of an item
//----------------------------------------------------------------------------
int VMCraftingModifyMovementSpeed( SCRIPT_CONTEXT * pContext,							  
								  int nModAmount )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;

	//lets check to see if this recipe has this property
	if( RecipeHasProperty( craftingData.pRecipeDefinition, RECIPE_PROP_MOVEMENTSPEED ) == FALSE )
		return 0;

	//lets compute the players risk that succeeded
	float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, RECIPE_PROP_MOVEMENTSPEED );		
	fBaseChance += (float)nModAmount;
	int nAmountToChange = RecipeGetPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, RECIPE_PROP_MOVEMENTSPEED, fBaseChance );	
	//add on the new movement speed bonus	
	nAmountToChange = MAX( 0, nAmountToChange );
	UnitSetStat( craftingData.pItem, STATS_VELOCITY_BONUS, nAmountToChange );		

	return 1;	
}

//----------------------------------------------------------------------------
//this function gets called when the excel is doing post process
//this caculates the chance for the property to succeed
//----------------------------------------------------------------------------
int VMRecipePropChance( SCRIPT_CONTEXT * pContext,	
					    int nRecipeProp,
					    int nMinValue,
					    int nMaxValue )
{
	ASSERT_RETZERO( nMinValue <= nMaxValue );
	ASSERT_RETZERO( nMinValue >= 0  );
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nRecipeProp != INVALID_ID && nRecipeProp < NUM_RECIPE_PROP );
	ASSERT_RETZERO( pContext->nParam1 != INVALID_ID	);
	
	RECIPE_DEFINITION *pRecipe = (RECIPE_DEFINITION *)ExcelGetDataNotThreadSafe( NULL, DATATABLE_RECIPES, (unsigned int)pContext->nParam1 );
	ASSERT_RETZERO( pRecipe );
	pRecipe->nRecipeProperties[ nRecipeProp ][RECIPE_VALUE_PER_PROP_MIN_PERCENT] = 0;
	pRecipe->nRecipeProperties[ nRecipeProp ][RECIPE_VALUE_PER_PROP_MAX_PERCENT] = nMaxValue;
	pRecipe->nRecipePropertyEnabled[ nRecipeProp ] = TRUE;
	return 1;
}

//----------------------------------------------------------------------------
//this function gets called when the excel is doing post process
//if the item succeeds then this is the resulting values
//----------------------------------------------------------------------------
int VMRecipePropValue( SCRIPT_CONTEXT * pContext,	
					   int nRecipeProp,
					   int nMinValue,
					   int nMaxValue )
{
	//ASSERT_RETZERO( nMinValue <= nMaxValue );	
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nRecipeProp != INVALID_ID && nRecipeProp < NUM_RECIPE_PROP );
	RECIPE_DEFINITION *pRecipe = (RECIPE_DEFINITION *)ExcelGetDataNotThreadSafe( NULL, DATATABLE_RECIPES, (unsigned int)pContext->nParam1 );
	ASSERT_RETZERO( pRecipe );
	pRecipe->nRecipeProperties[ nRecipeProp ][RECIPE_VALUE_PER_PROP_MIN_VALUE] = nMinValue;
	pRecipe->nRecipeProperties[ nRecipeProp ][RECIPE_VALUE_PER_PROP_MAX_VALUE] = nMaxValue;
	return 1;
}

//----------------------------------------------------------------------------
//this function adds pct chance to a recipe property
//----------------------------------------------------------------------------
int VMCraftingAddPctChance( SCRIPT_CONTEXT * pContext,	
							int nRecipeProp,
							int nValue )					  
{	
	if( nValue <= 0 )
		return 1;
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nRecipeProp >0  && nRecipeProp < NUM_RECIPE_PROP );
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->unit->pRecipePropertyValues );
	RECIPE_PROPERTY_VALUES &nPropertyValues = *pContext->unit->pRecipePropertyValues;
	//we changed functionality here, this is now MAX_PERCENT_OF_FAILURE
	nPropertyValues.nValues[ nRecipeProp ][ RECIPE_VALUE_PER_PROP_MAX_PERCENT ] -= nValue;
	return 1;
}

//----------------------------------------------------------------------------
//this function sets pct chance to a recipe property
//----------------------------------------------------------------------------
int VMCraftingSetPctChance( SCRIPT_CONTEXT * pContext,	
						   int nRecipeProp,
						   int nValue )					  
{	
	if( nValue <= 0 )
		return 1;
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nRecipeProp >0  && nRecipeProp < NUM_RECIPE_PROP );
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->unit->pRecipePropertyValues );
	RECIPE_PROPERTY_VALUES &nPropertyValues = *pContext->unit->pRecipePropertyValues;
	//we changed functionality here, this is now MAX_PERCENT_OF_FAILURE
	nPropertyValues.nValues[ nRecipeProp ][ RECIPE_VALUE_PER_PROP_MAX_PERCENT ] = 100 - nValue;
	return 1;
}


//----------------------------------------------------------------------------
//this function sets the percent value that the player can get
//----------------------------------------------------------------------------
int VMCraftingSetPctOfValue( SCRIPT_CONTEXT * pContext,	
							 int nRecipeProp,
							 int nValue )					  
{	
	if( nValue < 0 )
		nValue = 0;
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nRecipeProp >0  && nRecipeProp < NUM_RECIPE_PROP );
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->unit->pRecipePropertyValues );
	RECIPE_PROPERTY_VALUES &nPropertyValues = *pContext->unit->pRecipePropertyValues;
	const RECIPE_PROPERTIES *pRecipeProperty = RecipeGetPropertyDefinition( nRecipeProp );
	ASSERT_RETVAL( pRecipeProperty, 0 );
	if( TESTBIT( pRecipeProperty->dwFlags, RECIPE_PROPERTIES_FLAG_IGNORE_DEFAULT_PERCENT_INVESTED ) )
	{			
		nPropertyValues.nValues[ nRecipeProp ][ RECIPE_VALUE_PER_PROP_MAX_VALUE_PERCENT ] = nValue;
	}
	else
	{
		nValue = PCT_ROUNDUP( 100 - pRecipeProperty->nDefaultRange, nValue );
		nPropertyValues.nValues[ nRecipeProp ][ RECIPE_VALUE_PER_PROP_MAX_VALUE_PERCENT ] = pRecipeProperty->nDefaultRange + nValue;
	}
	return 1;
}

//----------------------------------------------------------------------------
//this function sets the value that the player can get
//----------------------------------------------------------------------------
int VMCraftingSetValue( SCRIPT_CONTEXT * pContext,	
						int nRecipeProp,
						int nValue,
						int nPctOfValue )					  
{	
	if( nValue < 0 )
		nValue = 0;
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nRecipeProp >0  && nRecipeProp < NUM_RECIPE_PROP );
	ASSERT_RETZERO( pContext->unit );
	ASSERT_RETZERO( pContext->unit->pRecipePropertyValues );
	RECIPE_PROPERTY_VALUES &nPropertyValues = *pContext->unit->pRecipePropertyValues;	
	const RECIPE_PROPERTIES *pRecipeProperty = RecipeGetPropertyDefinition( nRecipeProp );
	nPropertyValues.nValues[ nRecipeProp ][ RECIPE_VALUE_PER_PROP_MAX_VALUE ] = nValue;
	nPctOfValue = PCT_ROUNDUP( 100 - pRecipeProperty->nDefaultRange, nPctOfValue );
	nPropertyValues.nValues[ nRecipeProp ][ RECIPE_VALUE_PER_PROP_MAX_VALUE_PERCENT ] = pRecipeProperty->nDefaultRange + nPctOfValue;
	return 1;
}


//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMRecipe_UnitDataMinDmg( SCRIPT_CONTEXT * pContext,
							 int nModValue )
{
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( pContext->nParam2 > 0 ); //don't set to equal 0. This is going to be an item class
	const UNIT_DATA *pItemData = UnitGetData( pContext->game, GENUS_ITEM, pContext->nParam2 );
	ASSERT_RETZERO( pItemData );
	int minDmg( 0 ),
		maxDmg( 0 );
	ItemCaculateMinAndMaxBaseDamage( pContext->game, pItemData, minDmg, maxDmg, 0, nModValue, nModValue );
	return minDmg;
}

//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMRecipe_UnitDataMaxDmg( SCRIPT_CONTEXT * pContext,
							 int nModValue )
{
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( pContext->nParam2 > 0 ); //don't set to equal 0. This is going to be an item class
	const UNIT_DATA *pItemData = UnitGetData( pContext->game, GENUS_ITEM, pContext->nParam2 );
	ASSERT_RETZERO( pItemData );
	int minDmg( 0 ),
		maxDmg( 0 );
	ItemCaculateMinAndMaxBaseDamage( pContext->game, pItemData, minDmg, maxDmg, 0, nModValue, nModValue );
	return maxDmg;
}

//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMRecipe_UnitDataACMin( SCRIPT_CONTEXT * pContext,
						    int nModValue )
{
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( pContext->nParam2 > 0 ); //don't set to equal 0. This is going to be an item class
	const UNIT_DATA *pItemData = UnitGetData( pContext->game, GENUS_ITEM, pContext->nParam2 );
	ASSERT_RETZERO( pItemData );
	int nAC = ItemGetMinAC( pItemData, DAMAGE_TYPE_ALL, 0, nModValue );	
	return nAC;
}

//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMRecipe_UnitDataACMax( SCRIPT_CONTEXT * pContext,
						    int nModValue )
{
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( pContext->nParam2 > 0 ); //don't set to equal 0. This is going to be an item class
	const UNIT_DATA *pItemData = UnitGetData( pContext->game, GENUS_ITEM, pContext->nParam2 );
	ASSERT_RETZERO( pItemData );
	int nAC = ItemGetMaxAC( pItemData, DAMAGE_TYPE_ALL, 0, nModValue );	
	return nAC;
}


//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMRecipe_UnitDataStat( SCRIPT_CONTEXT * pContext,
						   int nStatID )
{
	ASSERT_RETZERO( pContext );
	ASSERT_RETZERO( nStatID != INVALID_ID );
	ASSERT_RETZERO( pContext->nParam2 > 0 ); //don't set to equal 0. This is going to be an item class
	const UNIT_DATA *pItemData = UnitGetData( pContext->game, GENUS_ITEM, pContext->nParam2 );
	ASSERT_RETZERO( pItemData );
	STATS *pStats = StatsListInit( pContext->game );
	ASSERT_RETZERO( pStats );	
	for( int nIndex = 0; nIndex < NUM_UNIT_PROPERTIES; nIndex++ )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode(pContext->game, DATATABLE_ITEMS, pItemData->codeProperties[nIndex], &code_len);
		VMExecI(pContext->game, pContext->unit, NULL, pStats, pContext->nParam1, pContext->nParam2, pContext->nSkill, pContext->nSkillLevel, pContext->nState, code_ptr, code_len);
	}
	int nValue = StatsGetStat( pStats, nStatID );
	StatsListFree(pContext->game, pStats);
	return nValue;
}


//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMCraftingSetStatByRecipeProp( SCRIPT_CONTEXT * pContext,
								  int nRecipeProp,
								  int nStatID,
								  int nParam )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
#if ISVERSION(DEVELOPMENT)
	const STATS_DATA *pStat = StatsGetData( craftingData.pGame, nStatID );
	REF( pStat );
#endif
	float fBaseChance = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipeProp );
	fBaseChance /= 100.0f;
	float fMinValue = (float)RecipeGetMinPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipeProp );
	float fMaxValue = (float)RecipeGetMaxPropertyValue( craftingData.pPlayer, craftingData.pRecipeDefinition, (RECIPE_PROPERTIES_ENUM)nRecipeProp );
	float fRange = fMaxValue - fMinValue;
	int nValue = (int)( fMinValue + ( fRange * fBaseChance ) );
	if( nParam <= 0 )	
		UnitSetStat( craftingData.pItem, nStatID, nValue );
	else
		UnitSetStat( craftingData.pItem, nStatID, nParam, nValue );
	return 1;
}

//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMCraftingSetDmgEffectByRecipeProp( SCRIPT_CONTEXT * pContext,
									     int nRecipeProp,
										 int nDmgEffect,
										 int nMinRange,
										 int nMaxRange,
										 int nDurationLenMS )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
	ASSERT_RETZERO( nDmgEffect != INVALID_ID );
	if( nDurationLenMS <= 0 )
		nDurationLenMS = 1000;

	float fCategoryLvlPct = (float)RecipeGetCategoryLevel( craftingData.pPlayer, craftingData.nCategoryID )/30.0f;	
	int nRange = nMaxRange - nMinRange;
	nRange = (int)CEIL( (float)nRange * fCategoryLvlPct );
	int nChance = nMinRange + nRange;

	if( pContext->stats != NULL )
	{
		//this is only applicable for affixes on items. Since affixes don't get added on till it's created, on the client it would just add up if we did this.( HUGE NUMBERS )
		if( IS_SERVER( craftingData.pGame ) )
		{
			nChance += StatsGetStat( pContext->stats, STATS_SKILL_EFFECT_CHANCE, nDmgEffect );
			nDurationLenMS += StatsGetStat( pContext->stats, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect );	
		}
		nChance = MAX( 10, nChance );
		nDurationLenMS = MAX( 1000, nDurationLenMS );
		StatsSetStat( craftingData.pGame, pContext->stats, STATS_SKILL_EFFECT_CHANCE, nDmgEffect, nChance );
		StatsSetStat( craftingData.pGame, pContext->stats, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect, nDurationLenMS );
		StatsSetStat( craftingData.pGame, pContext->stats, STATS_SFX_EFFECT_ATTACK, nDmgEffect, 10 );
	}
	else
	{
		//this is only applicable for affixes on items. Since affixes don't get added on till it's created, on the client it would just add up if we did this.( HUGE NUMBERS )
		if( IS_SERVER( craftingData.pGame ) )
		{
			nChance += UnitGetStat( craftingData.pItem, STATS_SKILL_EFFECT_CHANCE, nDmgEffect );
			nDurationLenMS += UnitGetStat( craftingData.pItem, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect );		
		}
		nChance = MAX( 10, nChance );
		nDurationLenMS = MAX( 1000, nDurationLenMS );
		UnitSetStat( craftingData.pItem, STATS_SKILL_EFFECT_CHANCE, nDmgEffect, nChance );
		UnitSetStat( craftingData.pItem, STATS_SFX_EFFECT_ATTACK_BONUS_MS, nDmgEffect, nDurationLenMS );
		UnitSetStat( craftingData.pItem, STATS_SFX_EFFECT_ATTACK, nDmgEffect, 10 );

	}

	return 1;
}



//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMCraftingSetDamageEffectParamByRecipeProp( SCRIPT_CONTEXT * pContext,
											    int nRecipeProp,
												int nDmgEffect,
												int nParam,
												int nMinRange,
												int nMaxRange )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
	ASSERT_RETZERO( nDmgEffect != INVALID_ID );
	
	UNIT *unit = pContext->unit;	
	ASSERT_RETZERO(unit);
	GAME * game = UnitGetGame(unit);
	ASSERT_RETZERO(game);	
	//if( IS_CLIENT( game ) )
	//	return 0;
	float fBaseChancePct = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipeProp );
	fBaseChancePct /= 100.0f;	
	int nRange = nMaxRange - nMinRange;
	nRange = (int)CEIL( (float)nRange * fBaseChancePct );
	int nValue = nMinRange + nRange;
	switch( nParam )
	{
	case 0:
		if( pContext->stats != NULL )
		{
			StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_PARAM0, nDmgEffect, nValue );
		}
		else
		{
			UnitSetStat( unit, STATS_SFX_EFFECT_PARAM0, nDmgEffect, nValue );
		}
		break;
	case 1:
		if( pContext->stats != NULL )
		{
			StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_PARAM1, nDmgEffect, nValue );
		}
		else
		{
			UnitSetStat( unit, STATS_SFX_EFFECT_PARAM1, nDmgEffect, nValue );
		}
		break;
	case 2:
		if( pContext->stats != NULL )
		{
			StatsSetStat( game, pContext->stats, STATS_SFX_EFFECT_PARAM2, nDmgEffect, nValue );
		}
		else
		{
			UnitSetStat( unit, STATS_SFX_EFFECT_PARAM2, nDmgEffect, nValue );
		}
		break;

	}



	return 1;
}
//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMCraftingGetValRangeByRecipePropPCT( SCRIPT_CONTEXT * pContext,
										  int nRecipeProp,											 
										  int nMinRange,
										  int nMaxRange )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
	float fBaseChancePct = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipeProp );
	fBaseChancePct /= 100.0f;	
	float fMinValue = (float)nMinRange;
	float fMaxValue = (float)nMaxRange;
	float fRange = fMaxValue - fMinValue;
	int nValue = (int)( fMinValue + ( fRange * fBaseChancePct ) );
	return nValue;
}


//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMCraftingSetStatByRangeAndRecipePropPCT( SCRIPT_CONTEXT * pContext,
											  int nRecipeProp,
											  int nStatID,
											  int nParam,
											  int nMinRange,
											  int nMaxRange )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
#if ISVERSION(DEVELOPMENT)
	const STATS_DATA *pStat = StatsGetData( craftingData.pGame, nStatID );
	REF( pStat );
#endif
	float fBaseChancePct = (float)RecipeGetPlayerInputForRecipeProp( craftingData.pPlayer, craftingData.nRecipeID, (RECIPE_PROPERTIES_ENUM)nRecipeProp );
	fBaseChancePct /= 100.0f;	
	float fMinValue = (float)nMinRange;
	float fMaxValue = (float)nMaxRange;
	float fRange = fMaxValue - fMinValue;
	int nValue = (int)( fMinValue + ( fRange * fBaseChancePct ) );
	int nShiftAmount = StatsGetShift( craftingData.pGame, nStatID );
	if( nParam <= 0 )	
		UnitSetStat( craftingData.pItem, nStatID, nValue << nShiftAmount );
	else
		UnitSetStat( craftingData.pItem, nStatID, nParam, nValue << nShiftAmount );
	return 1;
}

//----------------------------------------------------------------------------
//param 2 must be the item class. Check out function: sRecipeGetProperty in recipes.cpp
//----------------------------------------------------------------------------
int VMCraftingSetStatByRangeAndCategoryLvl( SCRIPT_CONTEXT * pContext,
											int nStatID,	
											int nParam,
											int nMinRange,
											int nMaxRange )
{
	CRAFTING_SCRIPT_CONTEXT craftingData;
	//gets all the data I need for crafting
	if( !sCraftingScript_FillCraftingContext( pContext, craftingData ) )
		return 1;
#if ISVERSION(DEVELOPMENT)
	const STATS_DATA *pStat = StatsGetData( craftingData.pGame, nStatID );
	REF( pStat );
#endif
	float fCategoryLvlPct = (float)RecipeGetCategoryLevel( craftingData.pPlayer, craftingData.nCategoryID )/30.0f;
	int nRange = nMaxRange - nMinRange;
	nRange = (int)CEIL( (float)nRange * fCategoryLvlPct );
	int nValue = nMinRange + nRange;
	if( nParam <= 0 )	
		UnitSetStat( craftingData.pItem, nStatID, nValue );
	else
		UnitSetStat( craftingData.pItem, nStatID, nParam, nValue );
	return 1;
}
