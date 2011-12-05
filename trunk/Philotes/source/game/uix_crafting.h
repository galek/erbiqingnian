//----------------------------------------------------------------------------
// uix_crafting.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UIX_CRAFTING_H_
#define _UIX_CRAFTING_H_

//----------------------------------------------------------------------------
// Enums
//----------------------------------------------------------------------------
const enum EINVENTORY_BAGS
{
	KINVENTORY_BACKPACK,
	KINVENTORY_CRAFTING
};

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

void InitComponentTypesCrafting(
	UI_XML_COMPONENT *pComponentTypes, 
	UI_XML_ONFUNC*& pUIXmlFunctions,
	int& nXmlFunctionsSize);

void cUICraftingShowItemsBrokenDownMsg( BYTE *pMessage );

void UICraftingRecipeCreate();

void UICraftingRecipeCreated( int nRecipe, BOOL bSuccessful );

void UICraftingGetTextOfCategoryRequired( const RECIPE_DEFINITION *pRecipe,
										  WCHAR *pText );

extern EINVENTORY_BAGS sUIInventoryShowBag;
#endif