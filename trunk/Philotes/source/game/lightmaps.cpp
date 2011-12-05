//----------------------------------------------------------------------------
// lightmaps.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "level.h"
#include "room.h"
#include "units.h"
#include "filepaths.h"
#include "performance.h"
#include "e_model.h"
#include "e_material.h"
#include "e_environment.h"
#include "e_definition.h"
#include "e_drawlist.h"
#include "e_effect_priv.h"
#include "e_portal.h"
#include "e_settings.h"
#include "e_main.h"
#include "e_shadow.h"
#ifndef __LOD__
#include "lod.h"	
#endif
#include "lightmaps.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

void c_UpdateCubeMapGeneration()
{
	if ( e_GeneratingCubeMap() )
	{
		c_ClearVisibleRooms();
		BOUNDING_BOX ClipBox;
		ClipBox.vMin = VECTOR(-1.0f, -1.0f, -1.0f);
		ClipBox.vMax = VECTOR( 1.0f,  1.0f,  1.0f);

		UNIT * pUnit = GameGetControlUnit( AppGetCltGame() );
		if ( ! pUnit )
			return;
		LEVEL * pLevel = UnitGetLevel( pUnit );
		if ( ! pLevel )
			return;
		ROOM * pRoom = LevelGetFirstRoom( pLevel );
		BOUNDED_WHILE ( pRoom, 10000 )
		{
			c_AddVisibleRoom( pRoom, ClipBox );
			// Set models visible, no frustum culling
			c_SetRoomModelsVisible( pRoom, FALSE, NULL );
			pRoom = LevelGetNextRoom( pRoom );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------