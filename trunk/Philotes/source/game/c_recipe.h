//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
/*
Tradesman will exchange completed goods for recipe components.
*/

#ifndef __C_RECIPE_H_
#define __C_RECIPE_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#ifndef __DIALOG_H_
#include "dialog.h"
#endif


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct MSG_SCMD_RECIPE_LIST_OPEN;
struct RECIPE_DEFINITION;
enum UI_MSG_RETVAL;
struct UI_COMPONENT;
struct UI_TRADESMANSHEET;
class UI_POSITION;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void c_RecipeInsertName( UNIT *pItemRecipe, WCHAR *pString, int nStringLength );
void c_RecipeInsertName( int nRecipeID, WCHAR *pString, int nStringLength );
int c_RecipeGetFlavoredText( UNIT *pItemRecipe );

void c_RecipeInitForGame(
	GAME *pGame);
	
void c_RecipeCloseForGame(
	GAME *pGame);

void c_RecipeOpen( 
	int nRecipe,
	UNIT *pRecipeGiver);
	
void c_RecipeListOpen( 
	int nRecipeList,
	UNIT *pRecipeGiver);

BOOL c_RecipeClose(
	BOOL bSendToServer);

void c_RecipeTryCreate(
    UNITID nRecipeGiver,
	int nRecipe,
	int nIndex );

void c_RecipeTryBuy(
	UNITID nRecipeGiver,
	int nRecipe );

BOOL c_RecipeIsSelected(
	void);

void c_RecipeCurrentUpdateUI(
	void);

void c_RecipeCreated(
	int nRecipe);

void c_RecipeFailed(
	int nRecipe );

void c_RecipeClearLastRecipeCreated();

void c_RecipeGetIngredientTooltip(
	UNIT *pItem,
	WCHAR *puszBuffer,
	int nBufferSize);



//returns the client unit. Used for UI
UNIT * c_RecipeGetClientIngredientByIndex( 
		const RECIPE_DEFINITION *pRecipeDefinition,
		int nIngredientIndex,
		int nOverrideQuality = INVALID_ID );

//returns the client unit. Used for UI
UNIT * c_RecipeGetClientResultByIndex( 
	const RECIPE_DEFINITION *pRecipeDefinition,
	int nResultIndex,
	int nOverrideQuality = INVALID_ID );

//----------------------------------------------------------------------------
// UI XML Handlers
//----------------------------------------------------------------------------

UI_MSG_RETVAL UITradesmanSheetOnSetControlUnit(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);
	
int sTradesmanSheetGetRecipeAtPosition(
	UI_TRADESMANSHEET *pSheet,
	float x, 
	float y,
	UI_POSITION &pos);
	
UI_MSG_RETVAL UITradesmanSheetOnLButtonDown(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);
	
UI_MSG_RETVAL UITradesmanSheetOnMouseHover(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);
	
UI_MSG_RETVAL UITradesmanSheetOnMouseLeave(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);
	
UI_MSG_RETVAL UITradesmanSheetOnPaint(
	UI_COMPONENT* component,
	int msg,
	DWORD wParam,
	DWORD lParam);

UI_MSG_RETVAL UIRecipeListSelectRecipe( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIRecipeOnChangeIngredients( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIRecipeCreate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

UI_MSG_RETVAL UIRecipeCancel( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);
		
UI_MSG_RETVAL UIRecipeOnPostInactivate( 
	UI_COMPONENT *pComponent, 
	int nMessage, 
	DWORD wParam, 
	DWORD lParam);

void UISetSelectedRecipe( int nRecipeID );

void UISetRecipePropertyModifiedByPlayer( int nRecipeID,
										  int nRecipeProperty,
										  int nRecipePropertyValue );

int UIGetRecipePCTOfPropertyModifiedByPlayer( int nRecipeID,
										 int nRecipeProperty );

cCurrency c_RecipeGetCurrency( int nRecipeID );
//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------

#endif
