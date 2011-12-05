//----------------------------------------------------------------------------
// FILE: cube.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __CUBE_H_
#define __CUBE_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;
struct ITEM_SPEC;
struct DATA_TABLE;
enum UI_MSG_RETVAL;
struct UI_COMPONENT;

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------

void s_CubeOpen(
	UNIT * pPlayer,
	UNIT * pCube );
	
void s_CubeTryCreate(
	UNIT * pPlayer );

void c_CubeOpen(
	UNIT * pCube );

void c_CubeClose();

//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)

UI_MSG_RETVAL UICubeCreate(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeCancel(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeOnChangeIngredients(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeOnPostInactivate(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeRecipeOpen(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeRecipeClose(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeScrollBarOnScroll(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeScrollBarOnMouseWheel(
	UI_COMPONENT * pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeRecipeOnPaint(
	UI_COMPONENT * pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam );

UI_MSG_RETVAL UICubeColumnOnClick(
	UI_COMPONENT* pComponent,
	int msg,
	DWORD wParam,
	DWORD lParam);

void c_CubeDisplayUnitFree();

void c_CubeDisplayUnitCreate(
	int recipe_index );

#endif //!ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------

#endif
