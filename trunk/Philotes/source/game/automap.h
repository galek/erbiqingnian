//----------------------------------------------------------------------------
// automap.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_AUTOMAP_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _AUTOMAP_H_

BOOL c_SaveAutomap(
	GAME * game,
	LEVELID idLevel);

BOOL c_LoadAutomap(
	GAME * game,
	LEVELID idLevel);

void c_LevelFreeAutomapInfo(
	GAME * game,
	LEVEL * level);

BOOL c_RoomApplyAutomapData(
	GAME * game,
	LEVEL * level,
	ROOM * room);


#endif