//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __S_LEVELAREASSERVER_H_
#define __S_LEVELAREASSERVER_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#ifndef __LEVELAREAS_H_
#include "LevelAreas.h"
#endif


struct DRLG_PASS;
struct LEVEL;
struct GAME;

namespace MYTHOS_LEVELAREAS
{

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------


//STATIC FUNCTIONS and Defines

//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

int s_LevelGetStartNewGameLevelAreaDefinition( BOOL bIntroComplete );



//this creates the bosses and treasure
void s_LevelAreaPopulateLevel( LEVEL *pLevel );

//this gives any items

void s_LevelAreaRegisterItemCreationOnUnitDeath( GAME *pGame,
											 LEVEL *pLevelCreated,
											 UNIT *pPlayer );

void s_LevelAreaRoomHasBeenReactivated( ROOM *pRoom );


//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------


} //end namespace
#endif
