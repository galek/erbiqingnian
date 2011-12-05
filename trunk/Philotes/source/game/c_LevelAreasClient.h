//----------------------------------------------------------------------------
// FILE:
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __C_LEVELAREASCLIENT_H_
#define __C_LEVELAREASCLIENT_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "LevelAreas.h"

#ifndef __FONTCOLOR_H_
#include "fontcolor.h"
#endif

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
//int c_GetLevelAreaLevelName( int nLevelAreaID );
void c_GetLevelAreaLevelName( int nLevelArea,
							  int nLevelAreaDepth,
							  WCHAR *puszBuffer,
						      int nBufferSiz,
							  BOOL bForceGeneralNameOfArea = FALSE );

void c_GetLevelAreaPixels( int nLevelAreaID, int nZoneID, float &x, float &y );
void c_GetLevelAreaWorldMapPixels( int nLevelAreaID, int nZoneID, float &x, float &y );

enum FONTCOLOR c_GetLevelAreaGetTextColorDifficulty( int nLevelArea, UNIT *pPlayer );
enum FONTCOLOR c_GetLevelAreaGetTextColorDifficulty( enum ELEVELAREA_DIFFICULTY nDifficulty );


//----------------------------------------------------------------------------
// EXTERNALS
//----------------------------------------------------------------------------


} //end namespace
#endif
