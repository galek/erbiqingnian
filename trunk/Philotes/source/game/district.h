//----------------------------------------------------------------------------
// FILE: district.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DISTRICT_H_
#define __DISTRICT_H_

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct ROOM;
struct GAME;
struct LEVEL;
struct DISTRICT;

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

void DistrictGrow( 
	GAME* pGame, 
	DISTRICT* pDistrict,
	float flDistrictArea);

DISTRICT* DistrictCreate( 
	GAME* pGame, 
	LEVEL* pLevel);

void DistrictFree(
	GAME* pGame,
	DISTRICT* pDistrict);

void DistrictAddRoom( 
	DISTRICT* pDistrict, 
	ROOM* pRoom);

//----------------------------------------------------------------------------

int DistrictGetSpawnClass(
	DISTRICT* pDistrict);

ROOM* DistrictGetFirstRoom(
	DISTRICT* pDistrict);
	
ROOM* DistrictGetNextRoom(
	ROOM* pRoom);

LEVEL* DistrictGetLevel(
	DISTRICT* pDistrict);
			
#endif
