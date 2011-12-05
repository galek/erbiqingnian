#include "stdafx.h"
#include "LevelZones.h"
#include "LevelAreas.h"
#include "excel.h"
#include "player.h"
#include "level.h"
#include "excel_private.h"
#include "picker.h"
#include "script.h"
#include "filepaths.h"
#include "e_texture.h"
#include "e_ui.h"
#include "uix_priv.h"
#include "uix.h"
#include "uix_components.h"
#include "inventory.h"
#include "c_GameMapsClient.h"
#include "LevelAreaLinker.h"
#include "c_LevelAreasClient.h"
#include "LevelAreasKnownIterator.h"


//The post process for the Level Areas
BOOL MYTHOS_LEVELZONES::LevelZoneExcelPostProcess(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION *pLevelZone( NULL );	
	
	for( int t = 0; t < (int)ExcelGetCountPrivate(table); t++ )
	{
		pLevelZone = (MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION*)ExcelGetDataPrivate(table, t);		
		ASSERT_RETFALSE(pLevelZone);
	}
	return TRUE;
}
//this function checks for other maps and makes sure they are in a good location.
BOOL MapIsInGoodLocation( int nLevelAreaID, float x, float y )
{
#if !ISVERSION(SERVER_VERSION)
	UNIT* pItem = NULL;	
	int location, invX, invY;
	//x += MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE/2;
	//y += MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE/2;
	while ( ( pItem = UnitInventoryIterate( UIGetControlUnit(), pItem ) ) != NULL)
	{		
		UnitGetInventoryLocation( pItem, &location, &invX, &invY );		
		if( MYTHOS_MAPS::IsMapKnown( pItem ) )
		{
			int nArea = MYTHOS_MAPS::GetLevelAreaID( pItem );
			if( nArea != nLevelAreaID &&
				MYTHOS_MAPS::c_MapHasPixelLocation( pItem ) )

			{			
				float xx( 0 );
				float yy( 0 );
				MYTHOS_MAPS::c_GetMapPixels( pItem, xx, yy );
				//xx += MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE/2;
				//yy += MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE/2;
				float distance = FABS( xx - x ) + FABS( yy - y );
				if( distance < MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE ) 
				{
					return FALSE;
				}
			}
		}
	}

	MYTHOS_LEVELAREAS::CLevelAreasKnownIterator levelAreaIterator( UIGetControlUnit() );
	int mapLevelAreaID( INVALID_ID );
	int nFlags = //MYTHOS_LEVELAREAS::KLVL_AREA_ITER_DONT_GET_POSITION |
				 MYTHOS_LEVELAREAS::KLVL_AREA_ITER_SKIP_CURRENT_PLAYER_POS |
				 MYTHOS_LEVELAREAS::KLVL_AREA_ITER_NO_MAPS | 
				 MYTHOS_LEVELAREAS::KLVL_AREA_ITER_NO_TEAM | 
				 MYTHOS_LEVELAREAS::KLVL_AREA_ITER_IGNORE_INVISIBLE_AREAS;

	int nZone = MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( nLevelAreaID, NULL );
	while ( (mapLevelAreaID = levelAreaIterator.GetNextLevelAreaID( nFlags ) ) != INVALID_ID )
	{
		float xx( 0 );
		float yy( 0 );
		ASSERT_RETFALSE( mapLevelAreaID != nLevelAreaID );
		if( nZone ==  MYTHOS_LEVELAREAS::LevelAreaGetZoneToAppearIn( mapLevelAreaID, NULL ) )
		{
			xx = levelAreaIterator.GetLevelAreaPixelX();
			yy = levelAreaIterator.GetLevelAreaPixelY();
			//MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( mapLevelAreaID, 
			//										nZone,
			//										xx,
			//										yy );
			//xx += MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE/2;
			//yy += MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE/2;
			float distance = abs( xx - x ) + abs( yy - y );
			if( distance < MYTHOS_LEVELZONES::KLEVELZONEDEFINES_ICON_SIZE ) 
			{
				return FALSE;
			}
		}

	}

#endif
	return TRUE;
}

inline void GetSinAndCosOfAngle( int nAngle, float scale, float &x, float &y )
{
	float fRadian = DegreesToRadians( nAngle );
	x = sin( fRadian );
	y = -cos( fRadian );	
}

BOOL MYTHOS_LEVELZONES::GetPixelLocation( RAND &rand, int nLevelAreaID, int nZoneID, float &x, float &y )
{

#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE( nZoneID != INVALID_ID );
	char szframeName[ 255 ];
	ZeroMemory( szframeName, sizeof( char ) * 255 );
	PStrPrintf( szframeName, arrsize(szframeName), "%s%i%s", "zone", nZoneID, "Mask" );
	int nZoneImageID = UIComponentGetIdByName( szframeName );	
	ASSERT_RETFALSE( nZoneImageID != INVALID_ID );
	UI_COMPONENT *pComponent = UIComponentGetById( nZoneImageID  );// UIComponentGetByEnum( UICOMP_ZONES_ZONE0);
	UI_COMPONENT *pWorldMap = UIComponentGetByEnum(UICOMP_WORLD_MAP);
	if( !pComponent ||
		!pWorldMap )
	{
		return FALSE;
	}
	int count( 0 );
	
	int nLevelAreaHubID = MYTHOS_LEVELAREAS::LevelAreaGetHubAttachedTo( nLevelAreaID, NULL );
	if( nLevelAreaHubID == nLevelAreaID )
	{
		return TRUE;
	}
	//get pixels of hub
	//this is the ratio of the world map width and height to the image width and height
	UI_POSITION pos = UIComponentGetAbsoluteLogPosition(pWorldMap);
	float scaleWidthMult = ( pComponent->m_fWidth/pWorldMap->m_fWidth);
	float scaleHeightMult = ( pComponent->m_fHeight/pWorldMap->m_fHeight );

	//get Radius
	float fRadius = (float)MYTHOS_LEVELAREAS::GetLevelAreaLinkerRadius( nLevelAreaHubID );		
	fRadius = fRadius * scaleWidthMult; //new radius size
	//get Hub position on the map
	float nHubPositionX( 0 );
	float nHubPositionY( 0 );
	MYTHOS_LEVELAREAS::c_GetLevelAreaPixels( nLevelAreaHubID, nZoneID, nHubPositionX, nHubPositionY );	
	nHubPositionX *= scaleWidthMult; //new positions for hub
	nHubPositionY *= scaleHeightMult; //new positions for hub
	//for positioning
	float xx( 0 ),
		  yy( 0 );	

	BOOL bCancelLoop( FALSE );
	float fLength( fRadius );
	float fLengthStepDis = fRadius/20.0f;

	while( !bCancelLoop &&
		   count < KLEVELZONEDEFINES_LOOP_COUNT )
	{
		count++;	
		int nAngle = MYTHOS_LEVELAREAS::LevelAreaGetAngleToHub( nLevelAreaID, rand );
		fLength = fRadius;
		float fX_Orginal( 0 ), //used for getting the pixels of the angle
			  fY_Orginal( 0 );
		GetSinAndCosOfAngle( nAngle, fRadius, fX_Orginal, fY_Orginal );
		
		while( fLength > -fRadius && !bCancelLoop)
		{
			xx = (fX_Orginal * fLength) + nHubPositionX;
			yy = (fY_Orginal * fLength) + nHubPositionY;

			if( ( fLength > 5 ||
				  fLength < -5 ) &&
				xx > 0 && xx < pComponent->m_fWidth && yy > 0 && yy < pComponent->m_fHeight )
			{
				bCancelLoop = UIComponentCheckMask( pComponent, xx * pComponent->m_fXmlWidthRatio, yy * pComponent->m_fXmlHeightRatio );
				if( bCancelLoop )	//we found a good spot. Now scale it back up to use on the world map..
				{
					xx *= ( 1/scaleWidthMult ); //scale back up
					yy *= ( 1/scaleHeightMult ); //scale back up
					bCancelLoop = MapIsInGoodLocation( nLevelAreaID, xx, yy );  //and now see if it's overlapping any other maps.
				}
			}
			fLength -= fLengthStepDis;
		}

	}
	

	x = xx;
	y = yy;

	//this loop handles doing a radius( actually a square ) check around the hub. Next best thing....
	if( count >= KLEVELZONEDEFINES_LOOP_COUNT )
	{
		//int nRadius = MYTHOS_LEVELAREAS::GetLevelAreaLinkerRadius( nLevelAreaHubID );
		int nRadiusTwice = (int)CEIL( fRadius * 2 );
		for( int nX = 0; nX < nRadiusTwice; nX ++ )
		{
			for( int nY = 0; nY < nRadiusTwice; nY++ )
			{
				x = (float)nX + ( nHubPositionX - fRadius );
				y = (float)nY + ( nHubPositionY - fRadius );
				bCancelLoop = UIComponentCheckMask( pComponent, x * pComponent->m_fXmlWidthRatio, y * pComponent->m_fXmlHeightRatio);
				if( bCancelLoop )
				{
					x *= ( 1/scaleWidthMult ); //scale back up
					y *= ( 1/scaleHeightMult ); //scale back up
					if( MapIsInGoodLocation( nLevelAreaID, x, y ) )
					{					
						count = 0;
						break;
					}
				}
			}
		}
	}	

	//this loop handles if the random find is just taking to long. This will go through the 
	//and find the first open area.
	if( count >= KLEVELZONEDEFINES_LOOP_COUNT )
	{
		for( int nX = 0; nX < pComponent->m_fWidth; nX ++ )
		{
			for( int nY = 0; nY < pComponent->m_fHeight; nY++ )
			{
				bCancelLoop = UIComponentCheckMask( pComponent, (float)nX * pComponent->m_fXmlWidthRatio, (float)nY * pComponent->m_fXmlHeightRatio);
				if( bCancelLoop )
				{
					xx = (float)nX * ( 1/scaleWidthMult ); //scale back up
					yy = (float)nY * ( 1/scaleHeightMult ); //scale back up
					if( MapIsInGoodLocation( nLevelAreaID, xx, yy ) )
					{					
						x = xx;
						y = yy;
						count = 0;
						break;
					}
				}
			}
		}
	}	
	return ( count < KLEVELZONEDEFINES_LOOP_COUNT );
#else
		return FALSE;
#endif
	
}

int MYTHOS_LEVELZONES::GetZoneNameStringID( const MYTHOS_LEVELZONES::LEVEL_ZONE_DEFINITION *pLevelZone )
{
	if( pLevelZone )
		return pLevelZone->nZoneNameStringID;
	return INVALID_ID;
}