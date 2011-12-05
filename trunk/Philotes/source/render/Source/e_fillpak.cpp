//----------------------------------------------------------------------------
// e_fillpak.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_fillpak.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static NONSTATE_SCREEN_EFFECT_ENTRY sgtNonStateScreenEffects[ NUM_NONSTATE_SCREEN_EFFECTS ] = 
{
	{ "DOF_TEST.xml", TRUE },
	{ "MotionBlur.xml", TRUE }
};

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

PRESULT e_ScreenFXGetNonStateEffect( int nIndex, int * pnDefID, NONSTATE_SCREEN_EFFECT_ENTRY ** pEntry )
{
	if( !AppIsTugboat() && ! c_GetToolMode() )
	{
		ASSERT_RETFAIL( IS_VALID_INDEX( nIndex, NUM_NONSTATE_SCREEN_EFFECTS ) );

		int nDefID = DefinitionGetIdByName( DEFINITION_GROUP_SCREENEFFECT, 
			sgtNonStateScreenEffects[ nIndex ].pszDefinitionName );
		ASSERT_RETFAIL( nDefID != INVALID_ID );

		if ( pnDefID )
			*pnDefID = nDefID;
		if ( pEntry )
			*pEntry = &sgtNonStateScreenEffects[ nIndex ];
	}

	return S_OK;
}
