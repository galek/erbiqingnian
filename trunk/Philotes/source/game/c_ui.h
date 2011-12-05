//----------------------------------------------------------------------------
// c_ui.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _C_UI_H_
#define _C_UI_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct GAME;
struct UNIT;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define UI_MAX_CLIENT_UNITS		500
//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct UI_CLIENT_UNITS
{
	UNIT *gUIClientUnits[ UI_MAX_CLIENT_UNITS ];
	int m_iCurrentClientIndex;
	int m_iMaxUnitsCreated;
};

//----------------------------------------------------------------------------
enum UI_CONSTANTS
{
	MAX_UNIT_DISPLAY_STRING = 1024,
};

//----------------------------------------------------------------------------
struct UI_GFXELEMENT;
struct UI_UNIT_DISPLAY
{
	UNITID	nUnitID;
	float	fCameraDistSq;
	float	fPlayerDistSq;
	float	fLabelScale;
	VECTOR	vScreenPos;
	WCHAR	uszDisplay[ MAX_UNIT_DISPLAY_STRING ];
	UI_GFXELEMENT * pTextElement;
	UI_GFXELEMENT * pRectElement;
	float	fTextWidth;
	float	fTextHeight;
	BOOL    bIsItem;
};

//----------------------------------------------------------------------------
enum UI_ELEMENT
{
	UIE_INVALID = -1,
	
	UIE_PAPER_DOLL,
	UIE_SKILL_MAP,	
	UIE_STASH,
	UIE_WORLDMAP,
	UIE_CONVERSATION,
	UIE_ITEM_UPGRADE,
	UIE_ITEM_AUGMENT,
	UIE_ITEM_UNMOD,
	UIE_EMAIL_PANEL,
	UIE_AUCTION_PANEL,

	// other stuff here someday
	
	NUM_UI_ELEMENTS		// keep this last please
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void c_UIUpdateSelection( GAME * pGame );
enum FONTCOLOR c_UIGetOverheadNameColor( GAME *pGame, UNIT *pUnit, UNIT *pPlayer );
void c_UIUpdateNames( GAME * pGame );
void c_UIUpdateDebugLabels();
UNIT * c_UIGetMouseUnit( GAME * pGame );
void c_UI_RTS_Selection( GAME * pGame, VECTOR * pvSelectedLocation, UNIT ** ppSelectedUnit );
//free's any client units created
void c_UIFreeClientUnits( GAME *pGame );
void c_UIFreeClientUnit( UNIT *pUnit );
UNIT * c_UIGetClientUNIT( GENUS eGenus, int nClassID, int nQuality = INVALID_ID, int nUnitCount = 1, DWORD nFlags = 0 );
//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------
extern float gflNameHeightOverride;
extern BOOL gbDisplayCollisionHeightLadder;
extern BOOL gUIClientUnitsInited;
extern UI_CLIENT_UNITS gUIClientUnits;
#endif // _C_UI_H_