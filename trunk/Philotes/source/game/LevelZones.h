#ifndef __LEVELZONES_H_
#define __LEVELZONES_H_
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _UNITS_H_
#include "units.h"
#endif


namespace MYTHOS_LEVELZONES
{

//DEFINES
static const enum KLEVELZONEDEFINES
{
	KLEVELZONEDEFINES_IMAGE_WIDTH = 128,
	KLEVELZONEDEFINES_IMAGE_HEIGHT = 128,
	KLEVELZONEDEFINES_WORKING_UI_WIDTH = 1280,
	KLEVELZONEDEFINES_WORKING_UI_HEIGHT = 1024,
	KLEVELZONEDEFINES_LOOP_COUNT = 256,
	KLEVELZONEDEFINES_ICON_SIZE = 32,
};
//STATIC FUNCTIONS and Defines
//----------------------------------------------------------------------------
// structs
//----------------------------------------------------------------------------
struct LEVEL_ZONE_DEFINITION
{
	char			pszZoneName[DEFAULT_INDEX_SIZE];	
	char			pszImageName[DEFAULT_INDEX_SIZE];	
	char			pszImageFrame[DEFAULT_INDEX_SIZE];	
	char			pszAutomapFrame[DEFAULT_INDEX_SIZE];	
	WORD			wCode;
	int				nZoneNameStringID;
	float			fZoneMapWidth;
	float			fZoneMapHeight;
	float			fZoneOffsetX;
	float			fZoneOffsetY;
	float			fWorldWidth;
	float			fWorldHeight;
	BOOL			bZoneBitmap[ KLEVELZONEDEFINES_IMAGE_WIDTH * KLEVELZONEDEFINES_IMAGE_HEIGHT ];
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------
//The post process for the Level Areas
BOOL LevelZoneExcelPostProcess(
	struct EXCEL_TABLE * table);

BOOL GetPixelLocation( RAND &rand, int nLevelAreaID, int nZoneID, float &x, float &y );


int GetZoneNameStringID( const LEVEL_ZONE_DEFINITION *pLevelZone );


}
#endif