//----------------------------------------------------------------------------
// unitdebug.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef	_UNITDEBUG_H_
#define _UNITDEBUG_H_

//----------------------------------------------------------------------------
enum UNITDEBUG_FLAG
{
	UNITDEBUG_FLAG_SHOW_DEV_NAME_BIT,			// display names overhead of units
	UNITDEBUG_FLAG_SHOW_POSITION_BIT,			// display unit position in name
	UNITDEBUG_FLAG_SHOW_VELOCITY_BIT,			// display unit velocity in name
	UNITDEBUG_FLAG_SHOW_ID_BIT,					// display unit id in name
	UNITDEBUG_FLAG_SHOW_ROOM_BIT,				// display room in name	
	UNITDEBUG_FLAG_SHOW_FACTION_BIT,			// display faction in name
	UNITDEBUG_FLAG_SHOW_COLLISION_BIT,			// display unit collision info
	UNITDEBUG_FLAG_SHOW_PATH_NODE_BIT,			// display path nodes
	UNITDEBUG_FLAG_SHOW_PATH_NODE_CONNECTION,	// display path node connections
	UNITDEBUG_FLAG_SHOW_PATH_NODE_ON_TOP_BIT,	// display path nodes
	UNITDEBUG_FLAG_SHOW_AIMING_BIT,				// display unit aiming info
	UNITDEBUG_FLAG_SHOW_MISSILES_BIT,			// display missile shapes
	UNITDEBUG_FLAG_SHOW_PATHING_COLLISION_BIT,	// display unit pathing collision info
	UNITDEBUG_FLAG_SHOW_PATH_NODE_CLIENT_BIT,	// display client path nodes
	UNITDEBUG_FLAG_SHOW_PATH_NODE_RADIUS_BIT,	// display path nodes radii
	UNITDEBUG_FLAG_SHOW_COLLISION_TRIANGLES_BIT,// display colored collision triangles
	UNITDEBUG_FLAG_SHOW_COLLISION_MATERIALS_BIT,// display colored collision triangles by material
	UNITDEBUG_FLAG_SHOW_MISSILE_VECTORS_BIT,	// display missile vectors
};

void UnitDebugUpdate(
	GAME * pGame );

BOOL UnitDebugSetFlag(
	UNITDEBUG_FLAG eFlag,
	BOOL bEnabled);

BOOL UnitDebugToggleFlag(
	UNITDEBUG_FLAG eFlag);

BOOL UnitDebugTestFlag(
	UNITDEBUG_FLAG eFlag);

void UnitDebugUpdatePathNodes( 
	GAME * pGame );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL UnitDebugTestLabelFlags(
	void)
{
	return UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_DEV_NAME_BIT ) ||
		   UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_ID_BIT ) ||
		   UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_ROOM_BIT ) ||
		   UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_FACTION_BIT ) ||
		   UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_POSITION_BIT ) ||
		   UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_VELOCITY_BIT ) ||
		   UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_COLLISION_BIT );
}
	

#endif