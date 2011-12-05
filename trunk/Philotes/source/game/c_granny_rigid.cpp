//----------------------------------------------------------------------------
// c_granny_rigid.cpp
//
// - Implementation for game-level granny functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "room.h"
#include "convexhull.h"
#include "drlgpriv.h"
#include "granny.h" 
#include "appcommon.h"
#include "excel.h"
#include "units.h"
#include "unit_priv.h"
#include "objects.h"
#include "monsters.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "performance.h"
#include "pakfile.h"
#include "filepaths.h"
#include "e_granny_rigid.h"
#include "e_material.h"
#include "c_granny_rigid.h"
#include "complexmaths.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

typedef enum 
{
	MODEL_NAME_REPLACE = 0,
	MODEL_NAME_RULE,
	MODEL_NAME_TEMPLATE,
	MODEL_NAME_TEMPLATE_LARGE,
	NUM_MODEL_NAMES,
}MODEL_NAME;
static const char sgpszModelNames[ NUM_MODEL_NAMES ][ MAX_XML_STRING_LENGTH ] = 
{
	"replace",
		"rule",
		"template",
		"templatelarge"
};


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

void c_GrannyModelToDRLGRooms ( granny_model * pGrannyModel, DRLG_ROOM * pRooms, char * pszFilePath, int * pnRoomCount, int nMaxRooms );


//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sParseModelName ( const char * pszInString, MODEL_NAME * peName, char * pszOutToken, int * pnOutIndex )
{
	char pszString [ MAX_XML_STRING_LENGTH ];
	PStrCopy( pszString, pszInString, MAX_XML_STRING_LENGTH );
	int nInStringLength = PStrLen( pszInString );

	*pnOutIndex = 0;
	*peName = NUM_MODEL_NAMES;

	for ( int i = 0; i < NUM_MODEL_NAMES; i ++ )
	{
		int nNameLength = PStrLen( sgpszModelNames[ i ] );
		if ( nNameLength > nInStringLength )
			continue;

		char cTemp = pszString[ nNameLength ];
		pszString[ nNameLength ] = 0;
		if ( PStrICmp( pszString, sgpszModelNames[ i ], MAX_XML_STRING_LENGTH ) == 0 )
		{
			* peName = (MODEL_NAME) i;
			pszString[ nNameLength ] = cTemp;
			char * pszIndex = & pszString[ nNameLength ];
			while ( *pnOutIndex == 0 && pszIndex < pszString + nInStringLength )
			{
				*pnOutIndex = PStrToInt( pszIndex );
				pszIndex++;
			}
			pszIndex--;
			if ( pszIndex < pszString + nInStringLength - 1 )
				*pszIndex = 0;

			char * pszToken = pszString + nNameLength;
			PStrCopy( pszOutToken, pszToken, MAX_XML_STRING_LENGTH );
			break;
		}
		pszString[ nNameLength ] = cTemp;
	}

	if ( *peName != NUM_MODEL_NAMES )
		return TRUE;
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sLineSegmentCollide( ROOM_DEFINITION * pRoom, VECTOR * pvStart, VECTOR * pvEnd, int * pnIntersectionCount )
{
	VECTOR vDirection;
	VectorSubtract(vDirection, *pvStart, *pvEnd); 

	float fLength = VectorLength(vDirection);
	VectorNormalize(vDirection);

	ROOM_CPOLY *pList = pRoom->pRoomCPolys;
	*pnIntersectionCount = 0;
	for (int nCollisionPoly = 0; nCollisionPoly < pRoom->nRoomCPolyCount; nCollisionPoly++)
	{
		float fDist;
		float fU;
		float fV;
		ROOM_POINT * pPt1 = & pRoom->pRoomPoints[CAST_PTR_TO_INT(pList->pPt1)];
		ROOM_POINT * pPt2 = & pRoom->pRoomPoints[CAST_PTR_TO_INT(pList->pPt2)];
		ROOM_POINT * pPt3 = & pRoom->pRoomPoints[CAST_PTR_TO_INT(pList->pPt3)];
		BOOL bHit = RayIntersectTriangle( pvStart, &vDirection, pPt1->vPosition, pPt2->vPosition, pPt3->vPosition, &fDist, &fU, &fV );
		if (bHit && fDist < fLength)
		{
			*pnIntersectionCount += 1;
		}
		pList++;
	}
	return (*pnIntersectionCount != 0);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sGetInterpolationAxes( VECTOR * pvAxisCorners, const VECTOR * pvCorners )
{
	int nCorners[ 3 ];
	nCorners[ 0 ] = 0;
	nCorners[ 1 ] = ( VectorDistanceSquared( pvCorners[ 0 ], pvCorners[ 1 ] ) < 
		VectorDistanceSquared( pvCorners[ 0 ], pvCorners[ 2 ] ) ) ? 1 : 2;
	int nOtherCorner = nCorners[ 1 ] == 1 ? 2 : 1;
	nCorners[ 2 ] = ( VectorDistanceSquared( pvCorners[ 0 ], pvCorners[ nOtherCorner ] ) < 
		VectorDistanceSquared( pvCorners[ 0 ], pvCorners[ 3 ] ) ) ? nOtherCorner : 3;
	pvAxisCorners[ 0 ] = pvCorners[ nCorners[ 0 ] ];
	pvAxisCorners[ 1 ] = pvCorners[ nCorners[ 1 ] ];
	pvAxisCorners[ 2 ] = pvCorners[ nCorners[ 2 ] ];
	pvAxisCorners[ 3 ] = pvCorners[ 3 ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sGetClosestCorner( const VECTOR & vTo, const VECTOR * pvCorners )
{
	float fMinDist = 100000.f;
	int nClosest = -1;
	for ( int i = 0; i < 4; i++ )
	{
		float fDist = VectorDistanceSquared( vTo, pvCorners[ i ] );
		if ( fDist < fMinDist )
		{
			fMinDist = fDist;
			nClosest = i;
		}
	}
	ASSERT_RETZERO( nClosest >= 0 );
	return nClosest;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetClosestEdges(
	const VECTOR * pvFirstAxisCorners,
	VECTOR * pvSecondAxisCorners,
	const VECTOR * pvSecondCorners,
	int nSecondBaseCorner )
{
	ASSERT( nSecondBaseCorner >= 0 && nSecondBaseCorner < 4 );

	int nOtherCorners[ 3 ];
	for ( int i = 0, c = 0; i < 4; i++ )
	{
		if ( i == nSecondBaseCorner )
			continue;
		nOtherCorners[ c++ ] = i;
	}

	int nEdges[ 6 ][ 2 ] = {
		{ 0, 1 },
		{ 1, 0 },
		{ 1, 2 },
		{ 2, 1 },
		{ 0, 2 },
		{ 2, 0 },
	};

	// get total distance from corner->corner for each edge permutation
	float fMinDist = 100000.f;
	int nMinIndex = 0;
	for ( int e = 0; e < 6; e++ )
	{
		float fDist;
		fDist =  VectorDistanceSquared( pvSecondCorners[ nOtherCorners[ nEdges[ e ][ 0 ] ] ],
										pvFirstAxisCorners[ 1 ] );
		fDist += VectorDistanceSquared( pvSecondCorners[ nOtherCorners[ nEdges[ e ][ 1 ] ] ],
										pvFirstAxisCorners[ 2 ] );
		if ( fDist < fMinDist )
		{
			fMinDist = fDist;
			nMinIndex = e;
		}
	}

	// use the min distance permutation as the other edge corner set
	pvSecondAxisCorners[ 0 ] = pvSecondCorners[ nSecondBaseCorner ];
	pvSecondAxisCorners[ 1 ] = pvSecondCorners[ nOtherCorners[ nEdges[ nMinIndex ][ 0 ] ] ];
	pvSecondAxisCorners[ 2 ] = pvSecondCorners[ nOtherCorners[ nEdges[ nMinIndex ][ 1 ] ] ];
	pvSecondAxisCorners[ 3 ] = pvSecondCorners[ 3 ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sGetBothConnectionInterpolationAxes(
	VECTOR * pvFirstAxisCorners,
	const VECTOR * pvFirstCorners,
	VECTOR * pvSecondAxisCorners,
	const VECTOR * pvSecondCorners )
{
	sGetInterpolationAxes( pvFirstAxisCorners, pvFirstCorners );

	int nSecondBaseCorner;
	nSecondBaseCorner = sGetClosestCorner( pvFirstAxisCorners[ 0 ], pvSecondCorners );

	sGetClosestEdges( pvFirstAxisCorners, pvSecondAxisCorners, pvSecondCorners, nSecondBaseCorner );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define VISIBILITY_TEST_INTERVAL_METERS 0.5f
#define VISIBILITY_TEST_MIN_INTERVALS	10
static void sRoomCalculateConnectionVisibility( ROOM_DEFINITION * pRoom, BOOL bUseIndoorVisibility )
{
	if ( ! bUseIndoorVisibility || AppIsTugboat() )
	{
		// mark each connection as visible to each other connection - but not itself
		DWORD dwVisibilityFlags = ( -1 >> ( MAX_CONNECTIONS_PER_ROOM - pRoom->nConnectionCount ) );
		for ( int i = 0; i < pRoom->nConnectionCount; i++ )
		{
			pRoom->pConnections[ i ].dwVisibilityFlags = dwVisibilityFlags & ( ~ (1<<i) );
		}
		return;
	}

	// we save the visibility flags in a DWORD - so we have a cap on how many connections are allowed
	ASSERT( pRoom->nConnectionCount <= MAX_CONNECTIONS_PER_ROOM );
	// compare each connection to each other connection and test their visibility
	for ( int nConnectionFirst = 0; nConnectionFirst < pRoom->nConnectionCount; nConnectionFirst ++ )
	{
		ROOM_CONNECTION * pConnectionFirst = & pRoom->pConnections[ nConnectionFirst ];
		for ( int nConnectionSecond = nConnectionFirst + 1; nConnectionSecond < pRoom->nConnectionCount; nConnectionSecond ++ )
		{
			ROOM_CONNECTION * pConnectionSecond = & pRoom->pConnections[ nConnectionSecond ];

			BOOL bVisible = FALSE;

			// pick interpolation axes
			VECTOR pvConnectionFirstCorner [ 4 ];
			VECTOR pvConnectionSecondCorner[ 4 ];
			sGetBothConnectionInterpolationAxes( pvConnectionFirstCorner, pConnectionFirst->pvBorderCorners, pvConnectionSecondCorner, pConnectionSecond->pvBorderCorners );
			//sGetInterpolationAxes( pvConnectionFirstCorner,  pConnectionFirst ->pvBorderCorners );
			//sGetInterpolationAxes( pvConnectionSecondCorner, pConnectionSecond->pvBorderCorners );
			// pick step factor
			float fMaxDist = VectorDistanceSquared( pvConnectionFirstCorner[ 0 ], pvConnectionFirstCorner[ 1 ] );
			float fTestDist;
			if ( fMaxDist < ( fTestDist = VectorDistanceSquared( pvConnectionFirstCorner[ 0 ], pvConnectionFirstCorner[ 2 ] ) ) )
				fMaxDist = fTestDist;
			if ( fMaxDist < ( fTestDist = VectorDistanceSquared( pvConnectionSecondCorner[ 0 ], pvConnectionSecondCorner[ 1 ] ) ) )
				fMaxDist = fTestDist;
			if ( fMaxDist < ( fTestDist = VectorDistanceSquared( pvConnectionSecondCorner[ 0 ], pvConnectionSecondCorner[ 2 ] ) ) )
				fMaxDist = fTestDist;

			fMaxDist = sqrtf( fMaxDist ) / VISIBILITY_TEST_INTERVAL_METERS;
			int nSteps = max( (int)fMaxDist, VISIBILITY_TEST_MIN_INTERVALS );
			float fStep = 1.0f / (float)nSteps;

			for ( int n = 0; n < 4 && ! bVisible; n++ )
			{
				// test visibility

				float fPercentOne = 0.0f;
				for ( ; ! bVisible && fPercentOne <= 1.0f; fPercentOne += fStep )
				{
					float fPercentTwo = 0.0f;
					VECTOR pvTestStarts[ 2 ];
					VECTOR pvTestEnds[ 2 ];
					VectorInterpolate ( pvTestStarts[ 0 ], fPercentOne, 
										pvConnectionFirstCorner[ 0 ], 
										pvConnectionFirstCorner[ 1 ] );
					VectorInterpolate ( pvTestStarts[ 1 ], fPercentOne, 
										pvConnectionFirstCorner[ 2 ], 
										pvConnectionFirstCorner[ 3 ] );

					VectorInterpolate ( pvTestEnds[ 0 ], ( n % 2 ) ? fPercentOne : 1.0f - fPercentOne, 
										pvConnectionSecondCorner[ 0 ], 
										pvConnectionSecondCorner[ 1 ] );
					VectorInterpolate ( pvTestEnds[ 1 ], ( n / 2 ) ? fPercentOne : 1.0f - fPercentOne, 
										pvConnectionSecondCorner[ 2 ], 
										pvConnectionSecondCorner[ 3 ] );

					for ( ; ! bVisible && fPercentTwo <= 1.0f; fPercentTwo += fStep )
					{
						VECTOR vTestStart;
						VectorInterpolate ( vTestStart, fPercentTwo, pvTestStarts[ 0 ], pvTestStarts[ 1 ] );
						VECTOR vTestEnd;
						VectorInterpolate ( vTestEnd,   fPercentTwo, pvTestEnds[ 0 ],   pvTestEnds[ 1 ] );

						int nIntersectionCount = 0;
						if ( ! sLineSegmentCollide( pRoom, & vTestStart, & vTestEnd, & nIntersectionCount ) )
						{// Sometimes the line goes outside of the room - we don't want to count those.  
							// Test to see if the line is inside the room.
							VECTOR vMidpoint;
							VectorAdd( vMidpoint, vTestStart, vTestEnd );
							vMidpoint.fX /= 2.0f;
							vMidpoint.fY /= 2.0f;
							vMidpoint.fZ /= 2.0f;

							// send rays from four places to vote - sometimes one brings a false positive
							BOOL bIsInsideRoom = TRUE;

							VECTOR vOutside = pRoom->tBoundingBox.vMin;
							vOutside.fX -= 10.0f;
							vOutside.fY -= 10.0f;
							vOutside.fZ -= 30.0f;
							sLineSegmentCollide( pRoom, & vMidpoint, & vOutside, & nIntersectionCount );
							if ( nIntersectionCount % 2 == 0 )
								bIsInsideRoom = FALSE;

							if ( bIsInsideRoom )
							{
								vOutside = pRoom->tBoundingBox.vMax;
								vOutside.fX += 10.0f;
								vOutside.fY += 10.0f;
								vOutside.fZ += 30.0f;
								sLineSegmentCollide( pRoom, & vMidpoint, & vOutside, & nIntersectionCount );
								if ( nIntersectionCount % 2 == 0 )
									bIsInsideRoom = FALSE;
							}

							if ( bIsInsideRoom )
							{
								vOutside = pRoom->tBoundingBox.vMin;
								vOutside.fX += 20.0f;
								vOutside.fY += 20.0f;
								vOutside.fZ -= 30.0f;
								sLineSegmentCollide( pRoom, & vMidpoint, & vOutside, & nIntersectionCount );
								if ( nIntersectionCount % 2 == 0 )
									bIsInsideRoom = FALSE;
							}

							if ( bIsInsideRoom )
							{
								vOutside = pRoom->tBoundingBox.vMax;
								vOutside.fX -= 20.0f;
								vOutside.fY -= 20.0f;
								vOutside.fZ += 30.0f;
								sLineSegmentCollide( pRoom, & vMidpoint, & vOutside, & nIntersectionCount );
								if ( nIntersectionCount % 2 == 0 )
									bIsInsideRoom = FALSE;
							}

							if ( bIsInsideRoom )
							{
								pRoom->pConnections[ nConnectionFirst  ].dwVisibilityFlags |= 1 << nConnectionSecond;
								pRoom->pConnections[ nConnectionSecond ].dwVisibilityFlags |= 1 << nConnectionFirst;
								bVisible = TRUE;
								break;
							}
						}
					}
				}
			}
		}
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_GrannyUpdateDRLGFile ( const char * pszFileName, const char * pszFileNameWithPath )
{
	char pszGrannyFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszGrannyFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath, "gr2");

	char pszDrlgFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszDrlgFileName, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath, DRLG_FILE_EXTENSION);

	// check and see if we need to update at all
	if ( ! AppCommonAllowAssetUpdate() || ! FileNeedsUpdate( pszDrlgFileName, pszGrannyFileName, DRLG_FILE_MAGIC_NUMBER, DRLG_FILE_VERSION ) )
		return FALSE;

	// -- Timer --
	char pszTimerString[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( pszTimerString, DEFAULT_FILE_WITH_PATH_SIZE, "Converting %s to %s", pszGrannyFileName, pszDrlgFileName );
	TIMER_START( pszTimerString );
	DebugString( OP_MID, STATUS_STRING(Updating DRLG), pszGrannyFileName );

	char pszFilePath[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrGetPath(pszFilePath, DEFAULT_FILE_WITH_PATH_SIZE, pszFileNameWithPath);

	trace("GrannyUpdateModelFile(), rooms: %s\n", pszDrlgFileName);

	// -- Load Granny file --
	ASSERT_RETFALSE( pszGrannyFileName );
	granny_file * pGrannyFile = e_GrannyReadFile( pszGrannyFileName );

	ASSERTV_RETFALSE( pGrannyFile, "Could Not Find File %s", pszGrannyFileName);

	trace("GrannyUpdateModelFile(), GrannyReadEntireFile(): %s\n", pszGrannyFileName);
	ASSERT_RETFALSE( pGrannyFile );
	granny_file_info * pGrannyFileInfo = GrannyGetFileInfo( pGrannyFile );
	ASSERT_RETFALSE( pGrannyFileInfo );

	// -- Coordinate System --
	// Tell Granny to transform the file into our coordinate system
	e_GrannyConvertCoordinateSystem ( pGrannyFileInfo );

	// -- Create DRLG File --
	HANDLE hFile = CreateFile( pszDrlgFileName, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if ( hFile == INVALID_HANDLE_VALUE )
	{
		if ( DataFileCheck( pszDrlgFileName ) )
			hFile = CreateFile( pszDrlgFileName, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	}		
	ASSERTX_RETFALSE( hFile != INVALID_HANDLE_VALUE, pszDrlgFileName );
	
	// -- DRLG file headers --
	// write file header
	FILE_HEADER tFileHeader;
	//tFileHeader.dwMagicNumber = DRLG_FILE_MAGIC_NUMBER;
	tFileHeader.nVersion = DRLG_FILE_VERSION;
	DWORD dwBytesWritten = 0;
	DWORD dwBytesRead = 0;
	WriteFile( hFile, &tFileHeader, sizeof( FILE_HEADER ), &dwBytesWritten, NULL );
	int nFileOffset = dwBytesWritten;

	// write level definition
	DRLG_LEVEL_DEFINITION tLevelDef;
	ZeroMemory( &tLevelDef, sizeof( DRLG_LEVEL_DEFINITION ) );
	tLevelDef.tHeader.nId = INVALID_ID;
	PStrCopy( tLevelDef.tHeader.pszName, pszFileName, MAX_XML_STRING_LENGTH );
	// save for now, we will save again at the end when we have the proper offsets
	WriteFile( hFile, &tLevelDef, sizeof( DRLG_LEVEL_DEFINITION ), &dwBytesWritten, NULL );
	nFileOffset += dwBytesWritten;

	// -- path -- (We need the directory after FILE_PATH_BACKGROUNDS saved here)
	PStrGetPath(tLevelDef.pszFilePath, DEFAULT_FILE_WITH_PATH_SIZE, pszFileName);

	// -- Templates --
	DRLG_LEVEL_TEMPLATE pTemplates[ MAX_TEMPLATES_PER_FILE ];
	ZeroMemory( pTemplates, sizeof( DRLG_LEVEL_TEMPLATE ) * MAX_TEMPLATES_PER_FILE );

	// -- Rules --
	DRLG_SUBSTITUTION_RULE pRules[ MAX_RULES_PER_FILE ];
	ZeroMemory( pRules, sizeof( DRLG_SUBSTITUTION_RULE ) * MAX_RULES_PER_FILE );
	VECTOR vRuleOffsets[ MAX_RULES_PER_FILE ];
	ZeroMemory( vRuleOffsets, sizeof( VECTOR ) * MAX_RULES_PER_FILE );
	float fRuleRotations[ MAX_RULES_PER_FILE ];
	ZeroMemory( fRuleRotations, sizeof( float ) * MAX_RULES_PER_FILE );
	char ppszRuleTokens[ MAX_RULES_PER_FILE ][ MAX_XML_STRING_LENGTH ];

	// -- The Models ---
	// Right now we assumes that there is only one model in the file.
	// If we want to handle more models, then we need to create something like a CMyGrannyModel
	for ( int nModel = 0; nModel < pGrannyFileInfo->ModelCount; nModel++ )
	{
		granny_model * pGrannyModel = pGrannyFileInfo->Models[ nModel ]; 
		char pszModelToken[ MAX_XML_STRING_LENGTH ];
		MODEL_NAME eModelName;
		int nModelIndex = 0;
		if (! sParseModelName ( pGrannyModel->Name, &eModelName, pszModelToken, &nModelIndex ))
			continue;

		// -- Templates --
		if ( eModelName == MODEL_NAME_TEMPLATE_LARGE )
		{
			DRLG_LEVEL_TEMPLATE * pTemplate = & pTemplates[ tLevelDef.nTemplateCount ];
			tLevelDef.nTemplateCount++;

			DRLG_ROOM *pRooms = (DRLG_ROOM*)MALLOC( NULL, sizeof( DRLG_ROOM ) * MAX_ROOMS_PER_GROUP_LARGE );
			c_GrannyModelToDRLGRooms ( pGrannyModel, pRooms, pszFilePath, & pTemplate->nRoomCount, MAX_ROOMS_PER_GROUP_LARGE );

			// save the rooms to file
			pTemplate->pRooms = (DRLG_ROOM *)CAST_TO_VOIDPTR(nFileOffset);
			WriteFile( hFile, pRooms, pTemplate->nRoomCount * sizeof(DRLG_ROOM), &dwBytesWritten, NULL );
			nFileOffset += dwBytesWritten;
			FREE( NULL, pRooms );
		}
		if ( eModelName == MODEL_NAME_TEMPLATE )
		{
			DRLG_LEVEL_TEMPLATE * pTemplate = & pTemplates[ tLevelDef.nTemplateCount ];
			tLevelDef.nTemplateCount++;
			
			DRLG_ROOM pRooms[ MAX_ROOMS_PER_GROUP ];
			c_GrannyModelToDRLGRooms ( pGrannyModel, pRooms, pszFilePath, & pTemplate->nRoomCount, MAX_ROOMS_PER_GROUP );

			// save the rooms to file
			pTemplate->pRooms = (DRLG_ROOM *)CAST_TO_VOIDPTR(nFileOffset);
			WriteFile( hFile, pRooms, pTemplate->nRoomCount * sizeof(DRLG_ROOM), &dwBytesWritten, NULL );
			nFileOffset += dwBytesWritten;
		
		}

		if ( ( eModelName != MODEL_NAME_RULE ) && ( eModelName != MODEL_NAME_REPLACE ) )
			continue;

		// -- Rules and Replacements --
		// find which rule we are working with
		DRLG_SUBSTITUTION_RULE * pRule = NULL;
		int nRuleIndex = 0;
		for (; nRuleIndex < tLevelDef.nSubstitutionCount; nRuleIndex++ )
		{
			if ( PStrCmp( ppszRuleTokens[ nRuleIndex ], pszModelToken, MAX_XML_STRING_LENGTH) == 0 )
			{
				pRule = & pRules[ nRuleIndex ];
				break;
			}
		}
		if ( ! pRule )
		{
			pRule = & pRules[ tLevelDef.nSubstitutionCount ];
			PStrCopy( ppszRuleTokens[ tLevelDef.nSubstitutionCount ], pszModelToken, MAX_XML_STRING_LENGTH );
			tLevelDef.nSubstitutionCount++;
			ASSERT( tLevelDef.nSubstitutionCount <= MAX_RULES_PER_FILE );
		}

		// -- Rules --
		if ( eModelName == MODEL_NAME_RULE )
		{
			DRLG_ROOM pRooms[ MAX_ROOMS_PER_GROUP ];
			c_GrannyModelToDRLGRooms ( pGrannyModel, pRooms, pszFilePath, & pRule->nRuleRoomCount, MAX_ROOMS_PER_GROUP );
			for ( int i = 1; i < pRule->nRuleRoomCount; i++ )
			{
				pRooms[i].vLocation -= pRooms[0].vLocation;
				VectorZRotation( pRooms[i].vLocation, -pRooms[0].fRotation );
				pRooms[i].fRotation -= pRooms[0].fRotation;
				//pRooms[i].tBoundingBox.vMin -= pRooms[0].vLocation;
				//pRooms[i].tBoundingBox.vMax -= pRooms[0].vLocation;
				//BoundingBoxZRotation( &pRooms[i].tBoundingBox, -pRooms[0].fRotation );
			}
			//pRooms[0].tBoundingBox.vMin -= pRooms[0].vLocation;
			//pRooms[0].tBoundingBox.vMax -= pRooms[0].vLocation;
			//BoundingBoxZRotation( &pRooms[0].tBoundingBox, -pRooms[0].fRotation );
			vRuleOffsets[ nRuleIndex ] = pRooms[0].vLocation;
			fRuleRotations[ nRuleIndex ] = pRooms[0].fRotation;
			pRooms[0].fRotation = 0.0f;
			pRooms[0].vLocation = 0.0f;

			// save the rooms to file
			pRule->pRuleRooms = (DRLG_ROOM *)CAST_TO_VOIDPTR(nFileOffset);
			WriteFile( hFile, pRooms, pRule->nRuleRoomCount * sizeof( DRLG_ROOM ), &dwBytesWritten, NULL );
			nFileOffset += dwBytesWritten;
		}

		// -- Replacement --
		if ( eModelName == MODEL_NAME_REPLACE )
		{
			int nReplacementIndex = pRule->nReplacementCount;
			pRule->nReplacementCount++;
			ASSERT( pRule->nReplacementCount <= MAX_REPLACEMENTS_PER_RULE );

			DRLG_ROOM pRooms[ MAX_ROOMS_PER_GROUP ];
			c_GrannyModelToDRLGRooms ( pGrannyModel, pRooms, pszFilePath, & pRule->pnReplacementRoomCount[ nReplacementIndex ], MAX_ROOMS_PER_GROUP );

			// save the rooms to file
			pRule->__p64_ppReplacementRooms[ nReplacementIndex ] = (DRLG_ROOM *)IntToPtr( nFileOffset );
			WriteFile( hFile, pRooms, pRule->pnReplacementRoomCount[ nReplacementIndex ] * sizeof( DRLG_ROOM ), &dwBytesWritten, NULL );
			nFileOffset += dwBytesWritten;
		}
	}

	// write level templates
	if ( tLevelDef.nTemplateCount )
	{
		tLevelDef.pTemplates = (DRLG_LEVEL_TEMPLATE *)CAST_TO_VOIDPTR(nFileOffset);
		WriteFile( hFile, pTemplates, tLevelDef.nTemplateCount * sizeof( DRLG_LEVEL_TEMPLATE ), &dwBytesWritten, NULL );
		nFileOffset += dwBytesWritten;
	}

	// write level Substitution rules
	if ( tLevelDef.nSubstitutionCount )
	{
		tLevelDef.pSubsitutionRules = (DRLG_SUBSTITUTION_RULE *)CAST_TO_VOIDPTR(nFileOffset);
		WriteFile( hFile, pRules, tLevelDef.nSubstitutionCount * sizeof( DRLG_SUBSTITUTION_RULE ), &dwBytesWritten, NULL );
		nFileOffset += dwBytesWritten;

		// change offsets of replacements
		for ( int i = 0; i < tLevelDef.nSubstitutionCount; i++ )
		{
			DRLG_SUBSTITUTION_RULE * pRule = &pRules[i];

			DRLG_ROOM pRooms[ MAX_ROOMS_PER_GROUP ];
			for ( int j = 0; j < pRule->nReplacementCount; j++ )
			{
				LONG lFileOffset = (LONG)PtrToLong( pRule->__p64_ppReplacementRooms[j] );
				SetFilePointer( hFile, lFileOffset, 0, FILE_BEGIN );
				ReadFile( hFile, pRooms, pRule->pnReplacementRoomCount[ j ] * sizeof( DRLG_ROOM ), &dwBytesRead, NULL );
				ASSERT( dwBytesRead == ( pRule->pnReplacementRoomCount[ j ] * sizeof( DRLG_ROOM ) ) );
				for ( int k = 0; k < pRule->pnReplacementRoomCount[ j ]; k++ )
				{
					pRooms[k].vLocation -= vRuleOffsets[i];
					//pRooms[k].tBoundingBox.vMin -= vRuleOffsets[i];
					//pRooms[k].tBoundingBox.vMax -= vRuleOffsets[i];
					VectorZRotation( pRooms[k].vLocation, -fRuleRotations[i] );
					//BoundingBoxZRotation( &pRooms[k].tBoundingBox, -fRuleRotations[i] );
					pRooms[k].fRotation -= fRuleRotations[i];
				}
				SetFilePointer( hFile, lFileOffset, 0, FILE_BEGIN );
				WriteFile( hFile, pRooms, pRule->pnReplacementRoomCount[ j ] * sizeof( DRLG_ROOM ), &dwBytesWritten, NULL );
			}
		}
	}

	// go back and re-write header
	SetFilePointer(hFile, 0, 0, FILE_BEGIN);
	tFileHeader.dwMagicNumber = DRLG_FILE_MAGIC_NUMBER;
	tFileHeader.nVersion = DRLG_FILE_VERSION;
	WriteFile(hFile, &tFileHeader, sizeof(FILE_HEADER), &dwBytesWritten, NULL);
	nFileOffset = dwBytesWritten;

	WriteFile(hFile, &tLevelDef, sizeof(DRLG_LEVEL_DEFINITION), &dwBytesWritten, NULL);
	nFileOffset += dwBytesWritten;

	CloseHandle(hFile);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomFileNeedsUpdate(
	const char * pszGrannyFilename,
	const char * pszRoomFilename,
	UINT64 & gentime)
{
#ifdef FORCE_UPDATE_ROOM_FILES
	return TRUE;
#endif
	
	return AppCommonAllowAssetUpdate() && FileNeedsUpdate(pszRoomFilename, pszGrannyFilename, ROOM_FILE_MAGIC_NUMBER, ROOM_FILE_VERSION, 0, &gentime);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL LoadGrannyRoomFile(
	const char * pszGrannyFileName,
	granny_file *& pGrannyFile,
	granny_model *& pGrannyModel)
{
	pGrannyFile = e_GrannyReadFile(pszGrannyFileName);
	ASSERTV_RETFALSE( pGrannyFile, "Could Not Find File %s", pszGrannyFileName);

	granny_file_info * pGrannyFileInfo = GrannyGetFileInfo(pGrannyFile);
	ASSERT_RETFALSE(pGrannyFileInfo);

	// tell Granny to transform the file into our coordinate system
	e_GrannyConvertCoordinateSystem(pGrannyFileInfo);

	// scale (doesn't seem to be used anywhere)
	float fGrannyToGameScale = 1.0f / pGrannyFileInfo->ArtToolInfo->UnitsPerMeter;
	REF(fGrannyToGameScale);

	// the model
	// right now we assume that there is only one model in the file.
	// if we want to handle more models, then we need to create something like a CMyGrannyModel
	if (pGrannyFileInfo->ModelCount > 1)
	{
		ShowDataWarning(WARNING_TYPE_BACKGROUND, "Too many models in %s.  Model count is %d", pszGrannyFileName, pGrannyFileInfo->ModelCount);
	}
	if (pGrannyFileInfo->ModelCount == 0)
	{
		ShowDataWarning(WARNING_TYPE_BACKGROUND, "No models in %s.", pszGrannyFileName, pGrannyFileInfo->ModelCount);
		return FALSE;
	}
	pGrannyModel = pGrannyFileInfo->Models[0]; 

	return TRUE;
}


//----------------------------------------------------------------------------
// origin transformation
// see if they saved a mesh called origin which is used to label the intended origin
//----------------------------------------------------------------------------
static void PerformOriginTransformation(
	granny_model * pGrannyModel,
	MATRIX & mTransformationMatrix,
	const char * pszFileName )
{
	MatrixIdentity(&mTransformationMatrix);

	for (int nGrannyMeshIndex = 0; nGrannyMeshIndex < pGrannyModel->MeshBindingCount; ++nGrannyMeshIndex)
	{
		granny_mesh * pMesh = pGrannyModel->MeshBindings[nGrannyMeshIndex].Mesh;

		int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount(pMesh);
		granny_tri_material_group * pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups(pMesh);
		for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
		{
			// Get the group
			granny_tri_material_group * pGrannyTriangleGroup = &pGrannyTriangleGroupArray[nTriangleGroup];

			granny_material * pMaterial = pMesh->MaterialBindings[pGrannyTriangleGroup->MaterialIndex].Material;

			if (!pMaterial)
			{
				ShowDataWarning(WARNING_TYPE_BACKGROUND, "Mesh %s is missing a material!\n\nMaterial ID may be missing from Multi/Sub-Object material.", pMesh->Name);
			}
			ASSERT_CONTINUE(pMaterial);

			if (PStrICmp(pMaterial->Name, "origin") == 0)
			{
				e_GrannyGetTransformationFromOriginMesh(&mTransformationMatrix, pMesh, pGrannyTriangleGroup, pszFileName );
				break;
			}
		}
	}
}



//----------------------------------------------------------------------------
// generate the room's collision mesh
// modifies:
//		room.pwIndexBuffer
//		room.nIndexBufferSize
//		room.pVertices
//		room.nVertexBufferSize
//		room.nVertexCount
//		room.tBoundingBox
//----------------------------------------------------------------------------
static BOOL sProcessRoomCollisionMesh(
	ROOM_DEFINITION & room,
	WORD * pwMeshIndexBuffer,
	int nMeshIndexCount,
	GRANNY_VERTEX_64 * pMeshVertexes,
	int nMeshVertexCount)
{
	if (nMeshVertexCount <= 0 || nMeshIndexCount <= 0)
	{
		return TRUE;
	}
	if (room.pVertices)
	{
		return TRUE;
	}

	// copy vertexes
	room.nVertexCount = nMeshVertexCount;
	room.nVertexBufferSize = room.nVertexCount * sizeof(VECTOR);
	room.pVertices = (VECTOR *)MALLOCZ(NULL, room.nVertexBufferSize);
	for (int ii = 0; ii < room.nVertexCount; ++ii)
	{
#ifdef POINTER_64_BUG_FIX
		VECTOR* pVertex = &room.pVertices[ii];
		*pVertex = pMeshVertexes[ii].vPosition;
#else
		room.pVertices[ii] = pMeshVertexes[ii].vPosition;
#endif
		if (BoundingBoxIsZero(room.tBoundingBox))
		{
			room.tBoundingBox.vMin = room.pVertices[ii];
			room.tBoundingBox.vMax = room.pVertices[ii];
		}
		BoundingBoxExpandByPoint(&room.tBoundingBox, (VECTOR *)&room.pVertices[ii]);
	}

	// copy indexes
	room.nIndexBufferSize = sizeof(WORD) * nMeshIndexCount;
	room.pwIndexBuffer = (WORD *)MALLOCZ(NULL, room.nIndexBufferSize);
	MemoryCopy(room.pwIndexBuffer, room.nIndexBufferSize, pwMeshIndexBuffer, room.nIndexBufferSize);

	return TRUE;
}


static void sProcessRoomCollisionAddMaterial(
	ROOM_DEFINITION & room,
	int nMaterialIndex,
	int nCollisionPolygon,
	int nTriangleGroupTriangleCount)
{
	room.nCollisionMaterialIndexCount++;
	room.pCollisionMaterialIndexes = (ROOM_COLLISION_MATERIAL_INDEX *)REALLOCZ(NULL, room.pCollisionMaterialIndexes, room.nCollisionMaterialIndexCount*sizeof(ROOM_COLLISION_MATERIAL_INDEX));
	ROOM_COLLISION_MATERIAL_INDEX * pIndex = room.pCollisionMaterialIndexes + room.nCollisionMaterialIndexCount - 1;
	pIndex->nMaterialIndex = nMaterialIndex;
	pIndex->nFirstTriangle = nCollisionPolygon;
	pIndex->nTriangleCount = nTriangleGroupTriangleCount;
}
	

//----------------------------------------------------------------------------
// add pwCollisionIndexBuffer to room's collision list
// modifies:
//		room.pRoomPoints
//		room.nRoomPointCount
//		room.pRoomCPolys
//		room.nRoomCPolyCount
//		room.pCollisionMaterialIndexes
//		room.nCollisionMaterialIndexCount
//----------------------------------------------------------------------------
static BOOL sProcessRoomCollision(
	ROOM_DEFINITION & room,
	granny_material * material,
	granny_tri_material_group * triangle_group,
	GRANNY_VERTEX_64* pMeshVertexes,
	int nMeshVertexCount,
	WORD * pwRoomCollisionIndexBuffer,
	int nTriangleGroupTriangleCount,
	const BOOL bIsMax9Model)
{
	if (nTriangleGroupTriangleCount <= 0)
	{
		return TRUE;
	}

	// allocate the collision polygons
	int nCollisionPolygon = room.nRoomCPolyCount;
	room.nRoomCPolyCount += nTriangleGroupTriangleCount;
	room.pRoomCPolys = (ROOM_CPOLY *)REALLOCZ(NULL, room.pRoomCPolys, sizeof(ROOM_CPOLY) * room.nRoomCPolyCount);

	// over allocate, but that's okay
	int nNewVertexes = MIN(nTriangleGroupTriangleCount * 3, nMeshVertexCount);
	room.pRoomPoints = (ROOM_POINT *)REALLOCZ(NULL, room.pRoomPoints, sizeof(ROOM_POINT) * (room.nRoomPointCount + nNewVertexes));

	if (BoundingBoxIsZero(room.tBoundingBox))
	{
		room.tBoundingBox.vMin = pMeshVertexes[0].vPosition;
		room.tBoundingBox.vMax = pMeshVertexes[0].vPosition;
	}

	// iterate through all our points
	for (int ii = 0; ii < nTriangleGroupTriangleCount * 3; ++ii)
	{
		VECTOR * vertex = &(pMeshVertexes[pwRoomCollisionIndexBuffer[ii]].vPosition);
		// check if the vertex is in the room's vertex buffer already
		int index = -1;
		ROOM_POINT * room_point = room.pRoomPoints;
		for (int jj = 0; jj < room.nRoomPointCount; ++jj, ++room_point)
		{
			if (VectorDistanceSquared(*vertex, room_point->vPosition) < CLOSE_POINT_DISTANCE)
			{
				index = jj;					
				break;
			}
		}
		// add a new index if no match found
		if (index < 0)	
		{
			index = room.nRoomPointCount;
#ifdef POINTER_64_BUG_FIX
			VECTOR* pRoomVertex = &room.pRoomPoints[index].vPosition;
			*pRoomVertex= *vertex;
#else
			room.pRoomPoints[index].vPosition = *vertex;
#endif
			room.pRoomPoints[index].nIndex = index;
			++room.nRoomPointCount;
			BoundingBoxExpandByPoint(&room.tBoundingBox, vertex);
		}
		// add the vertex index to the room's poly list
		ROOM_CPOLY * pCPoly = room.pRoomCPolys + nCollisionPolygon + ii / 3;
		switch (ii % 3)
		{
		case 0:	pCPoly->pPt1 = (ROOM_POINT *)CAST_TO_VOIDPTR(index);	break;
		case 1:	pCPoly->pPt2 = (ROOM_POINT *)CAST_TO_VOIDPTR(index);	break;
		case 2:	pCPoly->pPt3 = (ROOM_POINT *)CAST_TO_VOIDPTR(index);	break;
		}
	}

	// now iterate through all our new polys and do some post-process
	VECTOR vOrigin = VECTOR( 0.0f, 0.0f, 0.0f );
	room.fRoomCRadius = 0.0f;
	for (int ii = nCollisionPolygon; ii < room.nRoomCPolyCount; ++ii)
	{
		ROOM_CPOLY * pCPoly = room.pRoomCPolys + ii;
		ROOM_POINT * points[3];
		points[0] = room.pRoomPoints + (int)CAST_PTR_TO_INT(pCPoly->pPt1);
		points[1] = room.pRoomPoints + (int)CAST_PTR_TO_INT(pCPoly->pPt2);
		points[2] = room.pRoomPoints + (int)CAST_PTR_TO_INT(pCPoly->pPt3);

		// compute poly normals
		VECTOR vDelta1;
		VectorSubtract(vDelta1, points[1]->vPosition, points[0]->vPosition);
		VECTOR vDelta2;
		VectorSubtract(vDelta2, points[2]->vPosition, points[0]->vPosition);
		VectorCross(pCPoly->vNormal, vDelta1, vDelta2);
		VectorNormalize(pCPoly->vNormal, pCPoly->vNormal);

		// calculate bounding box
#ifdef POINTER_64_BUG_FIX
		VECTOR* pVertex = &points[0]->vPosition;
		pCPoly->tBoundingBox.vMin = *pVertex;
		pCPoly->tBoundingBox.vMax = *pVertex;
#else
		pCPoly->tBoundingBox.vMin = points[0]->vPosition;
		pCPoly->tBoundingBox.vMax = points[0]->vPosition;
#endif
		BoundingBoxExpandByPoint(&pCPoly->tBoundingBox, (VECTOR *)&points[1]->vPosition);
		BoundingBoxExpandByPoint(&pCPoly->tBoundingBox, (VECTOR *)&points[2]->vPosition);

		for ( int p = 0; p < 3; p++ )
		{
			float fDistSq = VectorDistanceSquared( vOrigin, points[p]->vPosition );
			if ( fDistSq > room.fRoomCRadius )
				room.fRoomCRadius = fDistSq;
		}
/*
		// we no longer use edges & edge normals
		VECTOR vUp(0.0f, 0.0f, 1.0f);
		VECTOR vDelta;
		vDelta = points[1]->vPosition - points[0]->vPosition;
		VectorCross(pCPoly->vEdgeNormal[0], vUp, vDelta);
		VectorNormalize(pCPoly->vEdgeNormal[0], pCPoly->vEdgeNormal[0]);
		vDelta = points[2]->vPosition - points[1]->vPosition;
		VectorCross(pCPoly->vEdgeNormal[1], vUp, vDelta);
		VectorNormalize(pCPoly->vEdgeNormal[1], pCPoly->vEdgeNormal[1]);
		vDelta = points[0]->vPosition - points[2]->vPosition;
		VectorCross(pCPoly->vEdgeNormal[2], vUp, vDelta);
		VectorNormalize(pCPoly->vEdgeNormal[2], pCPoly->vEdgeNormal[2]);

		pCPoly->tEdgeBox[0].vMin = points[0]->vPosition;
		pCPoly->tEdgeBox[0].vMax = points[0]->vPosition;
		BoundingBoxExpandByPoint(&pCPoly->tEdgeBox[0], (VECTOR *)&points[1]->vPosition);

		pCPoly->tEdgeBox[1].vMin = points[1]->vPosition;
		pCPoly->tEdgeBox[1].vMax = points[1]->vPosition;
		BoundingBoxExpandByPoint(&pCPoly->tEdgeBox[1], (VECTOR *)&points[2]->vPosition);

		pCPoly->tEdgeBox[2].vMin = points[0]->vPosition;
		pCPoly->tEdgeBox[2].vMax = points[0]->vPosition;
		BoundingBoxExpandByPoint(&pCPoly->tEdgeBox[2], (VECTOR *)&points[2]->vPosition);
*/
	}

	room.fRoomCRadius = sqrtf( room.fRoomCRadius );

	if(bIsMax9Model)
	{
		if(material)
		{
			int nMaterialIndex = FindMaterialIndexWithMax9Name(material->Name);
			if(nMaterialIndex >= 0)
			{
				sProcessRoomCollisionAddMaterial(room, nMaterialIndex, nCollisionPolygon, nTriangleGroupTriangleCount);
			}
		}
	}
	else
	{
		sProcessRoomCollisionAddMaterial(room, triangle_group->MaterialIndex, nCollisionPolygon, nTriangleGroupTriangleCount);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// add pwHullIndexBuffer to room's hull
// modifies:
//		room.pHullVertices
//		room.nHullVertexCount
//		room.pHullTriangles
//		room.nHullTriangleCount
//----------------------------------------------------------------------------
static BOOL sProcessConvexHull(
	ROOM_DEFINITION & room,
	GRANNY_VERTEX_64 * pMeshVertexes,
	int nMeshVertexCount,
	WORD * pwHullTriangleBuffer,
	int nTriangleGroupTriangleCount)
{
	if (nTriangleGroupTriangleCount <= 0)
	{
		return TRUE;
	}

	// allocate the hull polygons
	int nHullTriangle = room.nHullTriangleCount * 3;
	room.nHullTriangleCount += nTriangleGroupTriangleCount;
	room.pHullTriangles = (int *)REALLOCZ(NULL, room.pHullTriangles, sizeof(int) * room.nHullTriangleCount * 3);

	// over allocate, but that's okay
	int nNewVertexes = MIN(nTriangleGroupTriangleCount * 3, nMeshVertexCount);
	room.pHullVertices = (VECTOR *)REALLOCZ(NULL, room.pHullVertices, sizeof(VECTOR) * (room.nHullVertexCount + nNewVertexes));

	if(!AppIsTugboat())
	{
		// TRAVIS: WAIT! Only wanna use collision for bounds in tugboat
		if (BoundingBoxIsZero(room.tBoundingBox))
		{
			room.tBoundingBox.vMin = pMeshVertexes[0].vPosition;
			room.tBoundingBox.vMax = pMeshVertexes[0].vPosition;
		}
	}

	// iterate through all new triangles
	for (int ii = 0; ii < nTriangleGroupTriangleCount * 3; ++ii)
	{
		VECTOR * vertex = &(pMeshVertexes[pwHullTriangleBuffer[ii]].vPosition);
		// check if the vertex is in the hull's vertex buffer already
		int index = -1;
		VECTOR * hull_vertex = room.pHullVertices;
		for (int jj = 0; jj < room.nHullVertexCount; ++jj, ++hull_vertex)
		{
			if (VectorDistanceSquared(*vertex, *hull_vertex) < CLOSE_POINT_DISTANCE)
			{
				index = jj;					
				break;
			}
		}
		// add a new index if no match found
		if (index < 0)	
		{
			index = room.nHullVertexCount;
#ifdef POINTER_64_BUG_FIX
			VECTOR* pVertex = &room.pHullVertices[index];
			*pVertex = *vertex;
#else
			room.pHullVertices[index] = *vertex;
#endif
			++room.nHullVertexCount;
			if(!AppIsTugboat())
			{
				// TRAVIS: WAIT! Only wanna use collision for bounds in tugboat
				BoundingBoxExpandByPoint(&room.tBoundingBox, vertex);
			}
		}
		// add the vertex index to the hull's poly list
		room.pHullTriangles[nHullTriangle] = index;
		++nHullTriangle;
	}

	ASSERT(nHullTriangle == room.nHullTriangleCount * 3);

	return TRUE;
}


//----------------------------------------------------------------------------
// sort vertex list by x then y then z
//----------------------------------------------------------------------------
static void sSortVertexesByXYZ(
	VECTOR * vertex_list,
	unsigned int count)
{
	static const float fEpsilon = 0.00001f;

	for (unsigned int nPass = 1; nPass < count; ++nPass)
	{
		for (unsigned int nVertexFirst = 0; nVertexFirst < count - nPass; ++nVertexFirst)
		{
			int nVertexSecond = nVertexFirst + 1;

			// compare by x, y, then z - use a small error term for floating errors
			BOOL fSwitch = FALSE;
			float fDelta = vertex_list[nVertexFirst].fX - vertex_list[nVertexSecond].fX;
			if (fDelta > fEpsilon)
			{
				fSwitch = TRUE;
			}
			else if (fDelta > - fEpsilon)
			{
				fDelta = vertex_list[nVertexFirst].fY - vertex_list[nVertexSecond].fY;
				if (fDelta > fEpsilon)
				{
					fSwitch = TRUE;
				}
				else if (fDelta > - fEpsilon)
				{
					fDelta = vertex_list[nVertexFirst].fZ - vertex_list[nVertexSecond].fZ;
					if (fDelta > fEpsilon)
					{
						fSwitch = TRUE;
					}
				}
			}

			if (fSwitch)
			{
				VECTOR vTemp = vertex_list[nVertexFirst];
				vertex_list[nVertexFirst] = vertex_list[nVertexSecond];
				vertex_list[nVertexSecond] = vTemp;
			}
		}
	}
}


//----------------------------------------------------------------------------
// better center calculation:  fit bounding sphere
//----------------------------------------------------------------------------
static BOOL sCalcCentroid(
	VECTOR * vector_list,
	unsigned int count,
	VECTOR * centroid,
	float * radius_sq)
{
	ASSERT_RETFALSE(count >= 4);

	static const unsigned int nSegments[6][2] =
	{
		{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, { 0, 2 }, { 1, 3 },
	};

	float fMaxLenSqr = -1;
	int nMaxIndex = -1;
	for (unsigned int ii = 0; ii < 6; ii++)
	{
		float fTempLenSqr = VectorDistanceSquared(vector_list[nSegments[ii][0]], vector_list[nSegments[ii][1]]);
		if (fMaxLenSqr < fTempLenSqr)
		{
			fMaxLenSqr = fTempLenSqr;
			nMaxIndex = ii;
		}
	}
	ASSERT_RETFALSE(nMaxIndex >= 0);

	// found biggest segment distance, now find the midpoint -- that's the bounding sphere center
	ASSERT( fMaxLenSqr > 0.f );

	float fClipRadius = sqrtf(fMaxLenSqr) * 0.5f;
	// add a fudge factor to avoid danger zones
	fClipRadius += 1.0f;
	*radius_sq = fClipRadius * fClipRadius;
	*centroid = (vector_list[nSegments[nMaxIndex][1]] - vector_list[nSegments[nMaxIndex][0]]) * 0.5f + vector_list[nSegments[nMaxIndex][0]];

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sProcessRoomAddConnection(
	ROOM_DEFINITION & room,
	GRANNY_VERTEX_64 * pMeshVertexes,
	int nMeshVertexCount)
{
	ASSERT_RETFALSE(nMeshVertexCount >= 4);
	ASSERT_RETFALSE(room.nConnectionCount < MAX_CONNECTIONS_PER_ROOM);
	room.nConnectionCount++;
	room.pConnections = (ROOM_CONNECTION *)REALLOCZ(NULL, room.pConnections, room.nConnectionCount * sizeof(ROOM_CONNECTION));

	ROOM_CONNECTION * pNewConnection = room.pConnections + room.nConnectionCount - 1;
	ZeroMemory(pNewConnection, sizeof(ROOM_CONNECTION));
	pNewConnection->nCullingPortalID = INVALID_ID;

	// compute the normal
	VECTOR vDelta1;
	VectorSubtract(vDelta1, (VECTOR &)pMeshVertexes[1].vPosition, (VECTOR &)pMeshVertexes[0].vPosition);
	VECTOR vDelta2;
	VectorSubtract(vDelta2, (VECTOR &)pMeshVertexes[2].vPosition, (VECTOR &)pMeshVertexes[0].vPosition);
	VectorCross(pNewConnection->vBorderNormal, vDelta1, vDelta2);
	VectorNormalize(pNewConnection->vBorderNormal, pNewConnection->vBorderNormal);

	// copy the vectors
	pNewConnection->vBorderCenter = VECTOR(0.0f);
	for (int nVertex = 0; nVertex < ROOM_CONNECTION_CORNER_COUNT; nVertex++)
	{
		pNewConnection->pvBorderCorners[nVertex] = pMeshVertexes[nVertex].vPosition;
		pNewConnection->vBorderCenter += pNewConnection->pvBorderCorners[nVertex];
	}
	pNewConnection->vBorderCenter /= (float)ROOM_CONNECTION_CORNER_COUNT;

	// sort vertexes
	sSortVertexesByXYZ(pNewConnection->pvBorderCorners, ROOM_CONNECTION_CORNER_COUNT);

	// pre-compute center & radiussq for later portal traversal
	ASSERT_RETFALSE(sCalcCentroid(pNewConnection->pvBorderCorners, ROOM_CONNECTION_CORNER_COUNT, 
		&pNewConnection->vPortalCentroid, &pNewConnection->fPortalRadiusSqr));

	return TRUE;
}	

//----------------------------------------------------------------------------
// make sure normals are facing inside
//----------------------------------------------------------------------------
void sPostProcessRoomConnections( ROOM_DEFINITION & room )
{
	// make sure each connection normal points into the room

	// first attempt: point towards center of room bounding box

	VECTOR vCenter = ( room.tBoundingBox.vMin + room.tBoundingBox.vMax ) * 0.5f;

	for ( int i = 0; i < room.nConnectionCount; i++ )
	{
		VECTOR & vNormal = room.pConnections[ i ].vBorderNormal;
		VECTOR vDelta = vCenter - room.pConnections[ i ].vBorderCenter;
		VectorNormalize( vDelta );  // shouldn't be necessary if all we want is sign
		float fD = VectorDot( vDelta, vNormal );
		if ( fD < 0.f )
			vNormal = -vNormal;
	}
}

//----------------------------------------------------------------------------
// NOTE: should sort these occluders by size
//----------------------------------------------------------------------------
static BOOL sProcessRoomAddOccluder(
	ROOM_DEFINITION & room,
	GRANNY_VERTEX_64 * pMeshVertexes,
	int nMeshVertexCount)
{
	ASSERT_RETFALSE(room.nOccluderPlaneCount < MAX_OCCLUDER_PLANES_PER_ROOM);
	ASSERT_RETFALSE(nMeshVertexCount >= 4);

	room.nOccluderPlaneCount++;
	room.pOccluderPlanes = (OCCLUDER_PLANE *)REALLOCZ(NULL, room.pOccluderPlanes, room.nOccluderPlaneCount * sizeof(OCCLUDER_PLANE));
	OCCLUDER_PLANE * pNewOccluder = room.pOccluderPlanes + room.nOccluderPlaneCount - 1;
	ZeroMemory(pNewOccluder, sizeof(OCCLUDER_PLANE));

	for (int nVertex = 0; nVertex < ROOM_CONNECTION_CORNER_COUNT; nVertex++)
	{
		pNewOccluder->pvCorners[nVertex] = pMeshVertexes[nVertex].vPosition;
	}

	// sort the vectors by x, y, then z
	sSortVertexesByXYZ(pNewOccluder->pvCorners, ROOM_CONNECTION_CORNER_COUNT);

	// pre-compute center & radiussq for later traversal
	ASSERT_RETFALSE(sCalcCentroid(pNewOccluder->pvCorners, ROOM_CONNECTION_CORNER_COUNT, 
		&pNewOccluder->vCentroid, &pNewOccluder->fRadiusSqr));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sProcessGrannyTriangleGroup(
	ROOM_DEFINITION & room,
	granny_mesh * mesh,
	granny_tri_material_group * triangle_group,
	WORD * pwMeshIndexBuffer,
	int nMeshIndexCount,
	GRANNY_VERTEX_64 * pMeshVertexes,
	int nMeshVertexCount,
	const BOOL bIsMax9Model)
{
	ASSERT_RETFALSE(triangle_group);

	WORD * pwTriangleGroupIndexBuffer = NULL;		// index buffer for triangle group

	// don't bother with triangle groups that don't have vertices
	if (triangle_group->TriCount <= 0)
	{
		return TRUE;
	}
	ASSERT_RETFALSE(triangle_group->TriCount <= USHRT_MAX);
	WORD wTriangleGroupTriangleCount = (WORD)triangle_group->TriCount;

	// get the index buffer for the triangle group
	int nTriangeGroupIndexCount = 3 * wTriangleGroupTriangleCount;
	int nTriangleGroupIndexBufferSize = sizeof(WORD) * nTriangeGroupIndexCount;
	if (wTriangleGroupTriangleCount)
	{
		ASSERT_RETFALSE(triangle_group->TriFirst * 3 + nTriangeGroupIndexCount <= nMeshIndexCount);
		pwTriangleGroupIndexBuffer = (WORD *)MALLOCZ(NULL, sizeof(WORD) * nTriangeGroupIndexCount);
		memcpy(pwTriangleGroupIndexBuffer, pwMeshIndexBuffer + triangle_group->TriFirst * 3, nTriangleGroupIndexBufferSize);
	}

	// now cut the vertices up and renumber indices so that we have only the vertices that we are using	
	BOOL * pbIsSubsetTriangle = (BOOL *)MALLOCZ(NULL, sizeof(BOOL) * wTriangleGroupTriangleCount);
	for (int nTriangle = 0; nTriangle < wTriangleGroupTriangleCount; ++nTriangle)
	{ 
		pbIsSubsetTriangle[nTriangle] = TRUE;		// we need this for the collapse function
	}
	// modifies wTriangleGroupTriangleCount, nTriangleGroupIndexBufferSize, pwTriangleGroupIndexBuffer
	e_GrannyCollapseMeshByUsage(&wTriangleGroupTriangleCount, &nTriangleGroupIndexBufferSize, pwTriangleGroupIndexBuffer, 
		wTriangleGroupTriangleCount, pbIsSubsetTriangle);
	FREE(NULL, pbIsSubsetTriangle);

	// process material
	ONCE
	{
		ASSERT_BREAK(mesh->MaterialBindings);
		ASSERT_BREAK(triangle_group->MaterialIndex >= 0);
		granny_material * material = mesh->MaterialBindings[triangle_group->MaterialIndex].Material;

		// starting location -- just use the lowest vertex in the mesh
		if (PStrICmp(material->Name, "startinglocation") == 0)
		{
			e_GrannyFindLowestPointInVertexList(pMeshVertexes, nMeshVertexCount, sizeof(GRANNY_VERTEX_64), &room.vStart, &room.nStartDirection);
		}
		// collision
		else if (e_IsCollisionMaterial(material->Name, bIsMax9Model))
		{
			ASSERT_BREAK(sProcessRoomCollisionMesh(room, pwMeshIndexBuffer, nMeshIndexCount, pMeshVertexes, nMeshVertexCount));

			ASSERT_BREAK(sProcessRoomCollision(room, material, triangle_group, pMeshVertexes, nMeshVertexCount,
				pwTriangleGroupIndexBuffer, wTriangleGroupTriangleCount, bIsMax9Model));
		}		
		// convex hull
		else if (PStrICmp(material->Name, "debug_hull") == 0)	
		{
			ASSERT_BREAK(sProcessConvexHull(room, pMeshVertexes, nMeshVertexCount, pwTriangleGroupIndexBuffer, wTriangleGroupTriangleCount));
		}
		// clip -- add a connection to the room
		else if (PStrICmp(material->Name, "clip") == 0)
		{
			ASSERT_BREAK(sProcessRoomAddConnection(room, pMeshVertexes, nMeshVertexCount));
		}
		// occluder
		else if (PStrICmp(material->Name, "occluder") == 0)
		{
			ASSERT_BREAK(sProcessRoomAddOccluder(room, pMeshVertexes, nMeshVertexCount));
		}
	}

	if (pwTriangleGroupIndexBuffer)
	{
		FREE(NULL, pwTriangleGroupIndexBuffer);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// not thread safe due to use of static buffers
//----------------------------------------------------------------------------
static BOOL sProcessGrannyMesh(
	ROOM_DEFINITION & room,
	granny_mesh * mesh,
	MATRIX & mTransformationMatrix,
	const BOOL bIsMax9Model)
{
	WORD * pwMeshIndexBuffer = NULL;		// index buffer for mesh

	// get the mesh's index buffer
	int nMeshIndexCount = GrannyGetMeshIndexCount(mesh);
	if (nMeshIndexCount > 0)
	{
		ASSERT_RETFALSE(nMeshIndexCount % 3 == 0);
		pwMeshIndexBuffer = (WORD *)MALLOCZ(NULL, sizeof(WORD) * nMeshIndexCount);
		GrannyCopyMeshIndices(mesh, sizeof(WORD), (BYTE *)pwMeshIndexBuffer);
	}

	// get the mesh's (transformed) vertexes
	GRANNY_VERTEX_64 * pMeshVertexes = NULL;
	int nMeshVertexCount;
	int nMeshVertexBufferSize;
	e_GrannyGetVertices(pMeshVertexes, nMeshVertexCount, nMeshVertexBufferSize, mesh, mTransformationMatrix);
	ASSERT(nMeshVertexCount <= USHRT_MAX);
	ASSERTV(nMeshVertexCount > 0, "Mesh has no vertices!\n\nRoom: \"%s\"\nMesh: \"%s\"", room.tHeader.pszName, mesh->Name );

	// process each granny triangle group
	int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount(mesh);
	granny_tri_material_group * pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups(mesh);

	BOOL result = TRUE;
	for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
	{
		granny_tri_material_group * triangle_group = &pGrannyTriangleGroupArray[nTriangleGroup];

		ASSERT_DO(sProcessGrannyTriangleGroup(room, mesh, triangle_group, pwMeshIndexBuffer, nMeshIndexCount, pMeshVertexes, nMeshVertexCount, bIsMax9Model))
		{
			result = FALSE;
			break;
		}
	}

	if (pMeshVertexes)
	{
		FREE(NULL, pMeshVertexes);
	}
	if (pwMeshIndexBuffer)
	{
		FREE(NULL, pwMeshIndexBuffer);
	}
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int LineLineIntersect2D(
	float l1x1,
	float l1y1,
	float l1x2,
	float l1y2,
	float l2x1,
	float l2y1,
	float l2x2,
	float l2y2)
{
	if (GetClockDirection(l1x1, l1y1, l1x2, l1y2, l2x1, l2y1) == GetClockDirection(l1x1, l1y1, l1x2, l1y2, l2x2, l2y2))
	{
		return 0;
	}
	if (GetClockDirection(l2x1, l2y1, l2x2, l2y2, l1x1, l1y1) == GetClockDirection(l2x1, l2y1, l2x2, l2y2, l1x2, l1y2))
	{
		return 0;
	}
	return 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int LineSquareIntersect2D(
	float lx1,
	float ly1,
	float lx2,
	float ly2,
	float sx1,
	float sy1,
	float sx2,
	float sy2)
{
	if (lx1 >= sx1 && lx1 < sx2 && ly1 >= sy1 && ly1 < sy2)
	{
		return 1;
	}
	if (lx2 >= sx1 && lx2 < sx2 && ly2 >= sy1 && ly2 < sy2)
	{
		return 1;
	}
	if (HLineIntersect2D(sx1, sx2, sy1, lx1, ly1, lx2, ly2) ||
		VLineIntersect2D(sy1, sy2, sx2, lx1, ly1, lx2, ly2) ||
		HLineIntersect2D(sx1, sx2, sy2, lx1, ly1, lx2, ly2) ||
		VLineIntersect2D(sy1, sy2, sx1, lx1, ly1, lx2, ly2))
	{
		return 1;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CollisionGridAddPoly(
	ROOM_DEFINITION & room,
	ROOM_CPOLY * poly,
	unsigned int index)
{
	ASSERT_RETFALSE(room.pCollisionGrid);
	ASSERT_RETFALSE(poly);

	VECTOR v1 = room.pRoomPoints[CAST_PTR_TO_INT(poly->pPt1)].vPosition;
	VECTOR v2 = room.pRoomPoints[CAST_PTR_TO_INT(poly->pPt2)].vPosition;
	VECTOR v3 = room.pRoomPoints[CAST_PTR_TO_INT(poly->pPt3)].vPosition;
	// get top-most and left-most point
	int x1 = (int)FLOOR((poly->tBoundingBox.vMin.fX - room.fCollisionGridX - EPSILON) / PATHING_MESH_SIZE);
	int y1 = (int)FLOOR((poly->tBoundingBox.vMin.fY - room.fCollisionGridY - EPSILON) / PATHING_MESH_SIZE);
	int x2 = (int)CEIL((poly->tBoundingBox.vMax.fX - room.fCollisionGridX + EPSILON) / PATHING_MESH_SIZE);
	int y2 = (int)CEIL((poly->tBoundingBox.vMax.fY - room.fCollisionGridY + EPSILON) / PATHING_MESH_SIZE);
	x1 = PIN(x1, 0, room.nCollisionGridWidth - 1);
	y1 = PIN(y1, 0, room.nCollisionGridHeight - 1);
	x2 = PIN(x2, 0, room.nCollisionGridWidth - 1);
	y2 = PIN(y2, 0, room.nCollisionGridHeight - 1);

	float flWalkableZNormal = CollisionWalkableZNormal();
	int pp = COLLISION_FLOOR;
	if (poly->vNormal.fZ >= -flWalkableZNormal && poly->vNormal.fZ < flWalkableZNormal)
	{
		pp = COLLISION_WALL;
	}
	else if (poly->vNormal.fZ < 0.0f)
	{
		pp = COLLISION_CEILING;
	}

//	trace("add poly %c (%1.4f, %1.4f) - (%1.4f, %1.4f) - (%1.4f, %1.4f)\n", (pp == COLLISION_FLOOR ? 'f' : (pp == COLLISION_WALL ? 'w' : 'c')),
//		poly->pPt1->vPosition.fX, poly->pPt1->vPosition.fY, poly->pPt2->vPosition.fX, poly->pPt2->vPosition.fY, poly->pPt3->vPosition.fX, poly->pPt3->vPosition.fY);
	// iterate squares
	for (int jj = y1; jj <= y2; jj++)
	{
		int startx = x1-1;
		int endx = x1-1;
		for (int ii = x1; ii <= x2; ii++)
		{
			float sx1 = room.fCollisionGridX + (float)ii * PATHING_MESH_SIZE;
			float sy1 = room.fCollisionGridY + (float)jj * PATHING_MESH_SIZE;
			if (LineSquareIntersect2D(v1.fX, v1.fY, v2.fX, v2.fY, sx1, sy1, sx1 + PATHING_MESH_SIZE, sy1 + PATHING_MESH_SIZE) ||
				LineSquareIntersect2D(v2.fX, v2.fY, v3.fX, v3.fY, sx1, sy1, sx1 + PATHING_MESH_SIZE, sy1 + PATHING_MESH_SIZE) ||
				LineSquareIntersect2D(v1.fX, v1.fY, v3.fX, v3.fY, sx1, sy1, sx1 + PATHING_MESH_SIZE, sy1 + PATHING_MESH_SIZE))
			{
				if (startx < x1)
				{
					startx = ii;
				}
				endx = ii;
			}
		}
		for (int ii = x1; ii <= x2; ii++)
		{
			if (ii >= startx && ii <= endx)
			{
				COLLISION_GRID_NODE * node = room.pCollisionGrid + jj * room.nCollisionGridWidth + ii;
				node->pnPolyList[pp] = (unsigned int * POINTER_64)REALLOCZ(NULL, Ptr64ToPtr(node->pnPolyList[pp]), (node->nCount[pp] + 1) * sizeof(unsigned int));
				node->pnPolyList[pp][node->nCount[pp]] = index;
				node->nCount[pp]++;
				room.nCollisionGridIndexes++;
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL RoomGenerateCollisionMesh(
	ROOM_DEFINITION & room)
{
	// create a grid
	float x1 = (float)FLOOR(room.tBoundingBox.vMin.fX);
	float y1 = (float)FLOOR(room.tBoundingBox.vMin.fY);
	float x2 = (float)CEIL(room.tBoundingBox.vMax.fX);
	float y2 = (float)CEIL(room.tBoundingBox.vMax.fY);

	room.nCollisionGridWidth = (int)((x2 - x1)/PATHING_MESH_SIZE) + 1;
	room.nCollisionGridHeight = (int)((y2 - y1)/PATHING_MESH_SIZE) + 1;
	ASSERT_RETFALSE(room.nCollisionGridWidth > 0 && room.nCollisionGridHeight > 0);
	room.pCollisionGrid = (COLLISION_GRID_NODE *)MALLOCZ(NULL, sizeof(COLLISION_GRID_NODE) * room.nCollisionGridWidth * room.nCollisionGridHeight);
	room.fCollisionGridX = x1;
	room.fCollisionGridY = y1;

	// iterate polys of collision mesh
	ROOM_CPOLY * polys = room.pRoomCPolys;
	if (polys)
	{
		for (int ii = 0; ii < room.nRoomCPolyCount; ++ii, ++polys)
		{
			CollisionGridAddPoly(room, polys, ii);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomDoChecksAndCalculations(
	ROOM_DEFINITION & room,
	const char * pszSourceFilename,
	const char * pszGrannyFilename,
	const char * pszRoomFilename)
{
	// check visibility from each connection to each other connection
	char pszRoomIndex[MAX_XML_STRING_LENGTH];
	PStrRemovePath(pszRoomIndex, MAX_XML_STRING_LENGTH, FILE_PATH_BACKGROUND, pszSourceFilename);
	PStrRemoveExtension(pszRoomIndex, MAX_XML_STRING_LENGTH, pszRoomIndex);
	//ROOM_INDEX * pRoomIndex = (ROOM_INDEX *)ExcelGetDataByStringIndex(NULL, DATATABLE_ROOM_INDEX, pszRoomIndex);
	ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex(pszRoomIndex);
	sRoomCalculateConnectionVisibility(&room, pRoomIndex ? ! pRoomIndex->bOutdoorVisibility : FALSE);
	if (pRoomIndex && pRoomIndex->bOutdoor)
	{
		room.tBoundingBox.vMax.fZ += OUTDOOR_ROOM_BOUNDING_BOX_Z_BONUS;
	}

	// test to see that collision is inside of the hull
	if (room.pRoomPoints && room.pHullVertices && room.pHullTriangles)
	{
		CONVEXHULL * pHull = HullCreate(room.pHullVertices, room.nHullVertexCount, room.pHullTriangles, room.nHullTriangleCount, pszSourceFilename);

		// TRAVIS: Since Mythos does collision with a quadtree, and piles all the polys into that levelwide,
		// this shouldn't matter much for us.
		if( AppIsHellgate() )
		{
			for (int ii = 0; ii < room.nRoomPointCount; ii++)
			{
				if ( ! ConvexHullPointTest(pHull, &room.pRoomPoints[ii].vPosition) )
				{
					ShowDataWarning( WARNING_TYPE_BACKGROUND, "Collision mesh outside of Hull in Granny File %s", pszGrannyFilename);
				}
			}
		}
		ConvexHullDestroy(pHull);
	}

	// calculate room triangle count
	room.nTriangleCount = room.nIndexBufferSize / (3 * sizeof(INDEX_BUFFER_TYPE));

	if( AppIsHellgate() )
	{
		// generate collision mesh
		RoomGenerateCollisionMesh(room);
	}


	return TRUE;
}

#if defined(HAVOK_ENABLED)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSaveHavokFile(
	ROOM_DEFINITION & room,
	const char * pszSourceFilename)
{
	char pszMoppFileName[MAX_XML_STRING_LENGTH];
	PStrReplaceExtension(pszMoppFileName, MAX_XML_STRING_LENGTH, pszSourceFilename, MOPP_FILE_EXTENSION);

	BOOL bCanSave = TRUE;
	if ( FileIsReadOnly( pszMoppFileName ) )
		bCanSave = DataFileCheck( pszMoppFileName );

	if ( bCanSave )
	{
		HavokSaveMoppCode(pszMoppFileName, sizeof(VECTOR), room.nVertexBufferSize, room.nVertexCount, room.pVertices,
			room.nIndexBufferSize, room.nTriangleCount, room.pwIndexBuffer);
		room.nHavokShapeCount = HavokSaveMultiMoppCode(pszMoppFileName, sizeof(VECTOR), room.nVertexBufferSize, room.nVertexCount, 
			room.pVertices, room.nIndexBufferSize, room.nTriangleCount, room.pwIndexBuffer, room.vHavokShapeTranslation);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSaveRoomFile(
	ROOM_DEFINITION & room,
	const char * pszSourceFilename,
	const char * pszGrannyFilename,
	const char * pszRoomFilename,
	WRITE_BUF_DYNAMIC & fbuf,
	UINT64 & gentime)
{
	if (!(room.pRoomPoints || room.pRoomCPolys || room.pConnections || room.pHullVertices || room.pHullTriangles))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(sRoomDoChecksAndCalculations(room, pszSourceFilename, pszGrannyFilename, pszRoomFilename));

	// file header
	FILE_HEADER tFileHeader;
	tFileHeader.dwMagicNumber = 0;
	tFileHeader.nVersion = ROOM_FILE_VERSION;
	ASSERT_RETFALSE(fbuf.PushBuf(&tFileHeader, sizeof(tFileHeader)));

	// save room file the first time
	ASSERT_RETFALSE(fbuf.PushBuf(&room, sizeof(room)));

	if (room.pRoomPoints || room.pRoomCPolys)
	{
		ASSERT(room.pRoomPoints && room.pRoomCPolys);

		// save room points
		DWORD dwRoomPointsFileOffset = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushBuf(room.pRoomPoints, sizeof(ROOM_POINT) * room.nRoomPointCount));

		// save room polys
		DWORD dwRoomCPolysFileOffset = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushBuf(room.pRoomCPolys, sizeof(ROOM_CPOLY) * room.nRoomCPolyCount));

		FREE(NULL, room.pRoomCPolys);
		room.pRoomCPolys = (ROOM_CPOLY *)CAST_TO_VOIDPTR(dwRoomCPolysFileOffset); 

		FREE(NULL, room.pRoomPoints);
		room.pRoomPoints = (ROOM_POINT *)CAST_TO_VOIDPTR(dwRoomPointsFileOffset);
	}

	// save collision grid
	if (room.nCollisionGridIndexes && AppIsHellgate())
	{
		DWORD dwCollisionGridIndexes = (DWORD)fbuf.GetPos();
		// save off pnCollisionGridIndexes
		unsigned int total = 0;
		for (int jj = 0; jj < room.nCollisionGridHeight; ++jj)
		{
			for (int ii = 0; ii < room.nCollisionGridWidth; ++ii)
			{
				COLLISION_GRID_NODE* node = room.pCollisionGrid + jj * room.nCollisionGridWidth + ii;
				for (unsigned int kk = 0; kk < NUM_COLLISION_CATEGORIES; kk++)
				{
					if (node->nCount[kk])
					{
						ASSERT_RETFALSE(fbuf.PushBuf(Ptr64ToPtr(node->pnPolyList[kk]), sizeof(int) * node->nCount[kk]));
						total += node->nCount[kk];
					}
				}
			}
		}
		ASSERT_RETFALSE(total == room.nCollisionGridIndexes);
		room.pnCollisionGridIndexes = (unsigned int *)CAST_TO_VOIDPTR(dwCollisionGridIndexes);

		// save off collision grid
		DWORD dwCollisionGridFileOffset = (DWORD)fbuf.GetPos();

		total = 0;
		for (int jj = 0; jj < room.nCollisionGridHeight; ++jj)
		{
			for (int ii = 0; ii < room.nCollisionGridWidth; ++ii)
			{
				COLLISION_GRID_NODE* node = room.pCollisionGrid + jj * room.nCollisionGridWidth + ii;
				for (unsigned int kk = 0; kk < NUM_COLLISION_CATEGORIES; kk++)
				{
					unsigned int count = node->nCount[kk];
					if (count)
					{
						FREE(NULL, Ptr64ToPtr(node->pnPolyList[kk]));
						node->pnPolyList[kk] = (unsigned int *)CAST_TO_VOIDPTR(dwCollisionGridIndexes + total * sizeof(int));
					}
					else
					{
						node->pnPolyList[kk] = (unsigned int *)CAST_TO_VOIDPTR(dwCollisionGridIndexes);
					}
					total += count;
				}
			}
		}

		ASSERT_RETFALSE(fbuf.PushBuf(room.pCollisionGrid, sizeof(COLLISION_GRID_NODE) * room.nCollisionGridWidth * room.nCollisionGridHeight));
		FREE(NULL, room.pCollisionGrid);
		room.pCollisionGrid = (COLLISION_GRID_NODE *)CAST_TO_VOIDPTR(dwCollisionGridFileOffset);
	}
	else
	{
		ASSERT_RETFALSE(room.pnCollisionGridIndexes == NULL);
		ASSERT_RETFALSE(room.pCollisionGrid == NULL);
	}

	if (room.pHullVertices || room.pHullTriangles)
	{
		ASSERT_RETFALSE(room.pHullVertices && room.pHullTriangles);

		// save hull points
		DWORD dwHullVerticesFileOffset = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushBuf(room.pHullVertices, sizeof(VECTOR) * room.nHullVertexCount));

		// save hull polys
		DWORD dwHullTrianglesFileOffset = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushBuf(room.pHullTriangles, sizeof(int) * room.nHullTriangleCount * 3));

		FREE(NULL, room.pHullVertices);
		room.pHullVertices = (VECTOR *)CAST_TO_VOIDPTR(dwHullVerticesFileOffset);

		FREE(NULL, room.pHullTriangles);
		room.pHullTriangles = (int *)CAST_TO_VOIDPTR(dwHullTrianglesFileOffset);
	}

	if (room.pConnections)
	{
		DWORD dwConnectionsFileOffset = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushBuf(room.pConnections, sizeof(ROOM_CONNECTION) * room.nConnectionCount));
		FREE(NULL, room.pConnections);
		room.pConnections = (ROOM_CONNECTION *)CAST_TO_VOIDPTR(dwConnectionsFileOffset);
	}

	if (room.pOccluderPlanes)
	{
		DWORD dwOccluderPlanesFileOffset = (DWORD)fbuf.GetPos();
		ASSERT_RETFALSE(fbuf.PushBuf(room.pOccluderPlanes, sizeof(OCCLUDER_PLANE) * room.nOccluderPlaneCount));
		FREE(NULL, room.pOccluderPlanes);
		room.pOccluderPlanes = (OCCLUDER_PLANE *)CAST_TO_VOIDPTR(dwOccluderPlanesFileOffset);
	}

	if (room.pVertices || room.pwIndexBuffer)
	{
		ASSERT_RETFALSE(room.pVertices && room.pwIndexBuffer);

		// save room collision vertices
		DWORD dwVerticesBufferFileOffest = (DWORD)fbuf.GetPos();
		if( AppIsHellgate() )
		{
			ASSERT_RETFALSE(fbuf.PushBuf(room.pVertices, room.nVertexBufferSize));
		}

		// save room collision indices
		DWORD dwIndexBufferFileOffset = (DWORD)fbuf.GetPos();
		if( AppIsHellgate() )
		{
			ASSERT_RETFALSE(fbuf.PushBuf(room.pwIndexBuffer, room.nIndexBufferSize));
		}

		// save collision material indexes
		if (room.pCollisionMaterialIndexes)
		{
			DWORD dwCollisionMaterialIndexesFileOffset = (DWORD)fbuf.GetPos();
			ASSERT_RETFALSE(fbuf.PushBuf(room.pCollisionMaterialIndexes, sizeof(ROOM_COLLISION_MATERIAL_INDEX) * room.nCollisionMaterialIndexCount));
			FREE(NULL, room.pCollisionMaterialIndexes);
			room.pCollisionMaterialIndexes = (ROOM_COLLISION_MATERIAL_INDEX *)CAST_TO_VOIDPTR(dwCollisionMaterialIndexesFileOffset);
		}
		else
		{
			room.pCollisionMaterialIndexes = (ROOM_COLLISION_MATERIAL_INDEX *)CAST_TO_VOIDPTR(fbuf.GetPos());
		}

#ifdef HAVOK_ENABLED
		if (AppIsHellgate())
		{
			sSaveHavokFile(room, pszSourceFilename);
		}
#endif

		FREE(NULL, room.pVertices);
		if( AppIsHellgate() )
		{
			room.pVertices = (VECTOR *)CAST_TO_VOIDPTR(dwVerticesBufferFileOffest);
		}
		else
		{
			room.pVertices = NULL;
		}

		FREE(NULL, room.pwIndexBuffer);
		if( AppIsHellgate() )
		{	
			room.pwIndexBuffer = (WORD *)CAST_TO_VOIDPTR(dwIndexBufferFileOffset);
		}
		else
		{
			room.pwIndexBuffer = NULL;
		}
	}

	// save the header again since we are done writing
	unsigned int buflen = fbuf.GetPos();
	fbuf.SetPos(0);
	tFileHeader.dwMagicNumber = ROOM_FILE_MAGIC_NUMBER;
	tFileHeader.nVersion = ROOM_FILE_VERSION;
	ASSERT_RETFALSE(fbuf.PushBuf(&tFileHeader, sizeof(tFileHeader)));

	// save room structure again - since we now have the offsets
	ASSERT_RETFALSE(fbuf.PushBuf(&room, sizeof(room)));
	fbuf.SetPos(buflen);

	BOOL bCanSave = TRUE;
	if ( FileIsReadOnly( pszRoomFilename ) )
		bCanSave = DataFileCheck( pszRoomFilename );

	if ( bCanSave )
	{
		ASSERT_RETFALSE(fbuf.Save(pszRoomFilename, NULL, &gentime));
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// generate the ROM file from source file
// return TRUE if we don't need to update or if we needed to update and did so
// return FALSE if we needed to update but failed to
//----------------------------------------------------------------------------
BOOL UpdateRoomFile(
	const char * pszSourceFilename, 
	BOOL bForceUpdate,
	UINT64 & gentime,
	void * & buffer,
	unsigned int & buflen)
{
	ASSERT_RETFALSE(gentime == 0 && buffer == NULL && buflen == 0);
	ASSERT_RETFALSE(pszSourceFilename && pszSourceFilename[0]);

	char pszGrannyFilename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszGrannyFilename, DEFAULT_FILE_WITH_PATH_SIZE, pszSourceFilename, "gr2");

	char pszRoomFilename[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszRoomFilename, DEFAULT_FILE_WITH_PATH_SIZE, pszSourceFilename, ROOM_FILE_EXTENSION);

	if (!bForceUpdate && !sRoomFileNeedsUpdate(pszGrannyFilename, pszRoomFilename, gentime))
	{
		return TRUE;
	}

	// timer
#if ISVERSION(DEVELOPMENT)
	char pszTimerString[MAX_XML_STRING_LENGTH];
	PStrPrintf(pszTimerString, MAX_XML_STRING_LENGTH, "Converting %s to %s", pszGrannyFilename, pszRoomFilename);
	TIMER_START(pszTimerString);
	DebugString(OP_MID, STATUS_STRING(Updating room file), pszRoomFilename);
#endif

	granny_file * pGrannyFile = NULL;
	granny_model * pGrannyModel = NULL;
	ASSERT_RETFALSE(LoadGrannyRoomFile(pszGrannyFilename, pGrannyFile, pGrannyModel));

	granny_file_info * pFileInfo =	GrannyGetFileInfo( pGrannyFile );
	ASSERT_RETFALSE( pFileInfo );
	BOOL bIsMax9Model = e_IsMax9Exporter(pFileInfo->ExporterInfo);

	BOOL result = FALSE;
	ONCE
	{
		ROOM_DEFINITION room;
		structclear(room);

		// bad place to put this
		char szFileWithExt[MAX_XML_STRING_LENGTH];
		PStrRemovePath(szFileWithExt, MAX_XML_STRING_LENGTH, FILE_PATH_BACKGROUND, pszSourceFilename);
		char szExcelIndex[MAX_XML_STRING_LENGTH];
		PStrRemoveExtension(szExcelIndex, MAX_XML_STRING_LENGTH, szFileWithExt);
		//ROOM_INDEX * pRoomIndex = (ROOM_INDEX *)ExcelGetDataByStringIndex(NULL, DATATABLE_ROOM_INDEX, szExcelIndex);
		ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex(szExcelIndex);
		if (pRoomIndex) 
		{
			room.nExcelVersion = pRoomIndex->nExcelVersion;
		}
		// end bad positioning

		MATRIX mTransformationMatrix;
		PerformOriginTransformation(pGrannyModel, mTransformationMatrix, pszSourceFilename);

		// meshes
		for (int nGrannyMeshIndex = 0; nGrannyMeshIndex < pGrannyModel->MeshBindingCount; ++nGrannyMeshIndex)
		{
			sProcessGrannyMesh(room, pGrannyModel->MeshBindings[nGrannyMeshIndex].Mesh, mTransformationMatrix, bIsMax9Model);
		}

		sPostProcessRoomConnections( room );

		WRITE_BUF_DYNAMIC fbuf;
		ASSERTX_BREAK(sSaveRoomFile(room, pszSourceFilename, pszGrannyFilename, pszRoomFilename, fbuf, gentime), pszRoomFilename );
		buffer = fbuf.KeepBuf(buflen);
		ASSERT_BREAK(buffer && buflen > 0);

		result = TRUE;
	}

	if (pGrannyFile)
	{
		GrannyFreeFile(pGrannyFile);
	}

	return result;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMeshGetTokenAndName ( const char * pszInString, char * pszOutName, int * pnOutIndex )
{
	int nInStringLength = PStrLen ( pszInString );
	if (! nInStringLength )
		return FALSE;

/*	const char * pBracket = PStrRChr( pszInString, '[' );
	if ( pBracket )
	{
		nInStringLength -= PStrLen( pBracket );
		char * pWrite = const_cast<char*>(pBracket);
		*pWrite = 0;	// comment out the end of the line for now...
	}*/

	const char * pDot = PStrRChr( pszInString, '.' );
	if ( pDot )
	{
		nInStringLength -= PStrLen( pDot );
		char * pWrite = const_cast<char*>(pDot);
		*pWrite = 0;	// comment out the end of the line for now...
	}

	char pszString [ MAX_XML_STRING_LENGTH ];
	PStrCopy( pszString, pszInString, MAX_XML_STRING_LENGTH );

	// cut out the spaces - in case there was an artist typo
	char * pContext;
	strtok_s ( pszString, " ", &pContext );

	char * pszIndex = pszString + nInStringLength;

	while ( ( pszIndex != pszString ) && ( *(pszIndex-1) >= '0' ) && ( *(pszIndex-1) <= '9' ) )
	{
		pszIndex--;
	}

	*pnOutIndex = 0;

	if ( pszIndex != pszString )
	{
		*pnOutIndex = PStrToInt( pszIndex );
		*pszIndex = 0;
	}

	PStrCopy( pszOutName, pszString, MAX_XML_STRING_LENGTH );
	if ( pDot )
	{
		char * pWrite = const_cast<char*>(pDot);
		*pWrite = '.';
		PStrCat( pszOutName, pDot, MAX_XML_STRING_LENGTH );
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_GrannyModelToDRLGRooms ( granny_model * pGrannyModel, DRLG_ROOM * pRooms, char * pszFilePath, int * pnRoomCount, int nMaxRooms )
{
	ZeroMemory( pRooms, nMaxRooms * sizeof( DRLG_ROOM ) );
	*pnRoomCount = 0;

	VECTOR vOffsetToRuleOrigin;
	float fRuleRotation = 0.0f;

	for ( int nMesh = 0; nMesh < pGrannyModel->MeshBindingCount; nMesh++ )
	{
		granny_mesh * pMesh = pGrannyModel->MeshBindings[ nMesh ].Mesh;

		char pszMeshName[ MAX_XML_STRING_LENGTH ];
		int nMeshIndex = 0;
		if (! sMeshGetTokenAndName ( pMesh->Name, pszMeshName, &nMeshIndex ))
			continue;

		int nTriangleGroupCount = GrannyGetMeshTriangleGroupCount( pMesh );
		granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pMesh );

		// set the origin for the rules
		if ( PStrICmp( pMesh->Name, "rule_origin" ) == 0 )
		{
			e_GrannyGetVectorAndRotationFromOriginMesh( pMesh, &pGrannyTriangleGroupArray[0], &vOffsetToRuleOrigin, &fRuleRotation );
			continue;
		}
		BOOL bRuleOrigin = FALSE;
		for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
		{
			// Get the group
			granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ nTriangleGroup ];

			granny_material * pMaterial = pMesh->MaterialBindings[ pGrannyTriangleGroup->MaterialIndex ].Material;

			// set the origin for the rules
			if ( PStrICmp( pMaterial->Name, "rule_origin" ) == 0 )
			{
				e_GrannyGetVectorAndRotationFromOriginMesh( pMesh, &pGrannyTriangleGroupArray[0], &vOffsetToRuleOrigin, &fRuleRotation );
				bRuleOrigin = TRUE;
			}
		}
		if ( bRuleOrigin )
			continue;

		// make another room
		int nRoomIndex = *pnRoomCount;
		ASSERT( *pnRoomCount <= nMaxRooms );
		PStrCopy( pRooms[ nRoomIndex ].pszFileName, pszMeshName, MAX_XML_STRING_LENGTH );
		char pszMeshNameWithPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		char pszTempName[ DEFAULT_FILE_WITH_PATH_SIZE ];
		const char * pBracket = PStrRChr( pszMeshName, '[' );
		if ( pBracket )
		{
			// remove from string for now...
			PStrCopy(pszTempName, pszMeshName, (int)(pBracket - pszMeshName + 1));
		}
		else
		{
			PStrCopy(pszTempName, pszMeshName, DEFAULT_FILE_WITH_PATH_SIZE);
		}

		const char * pDot = PStrRChr( pszTempName, '.' );
		if ( pDot )
		{
			char szName[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
			OVERRIDE_PATH tOverridePath;
			OverridePathByCode( *( (DWORD*)( pDot + 1 ) ), &tOverridePath );
			PStrCopy(szName, pszTempName, (int)(pDot - pszTempName + 1));
			PStrForceBackslash( tOverridePath.szPath, DEFAULT_FILE_WITH_PATH_SIZE );
			PStrPrintf( pszMeshNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s.gr2", tOverridePath.szPath, szName );
		}
		else
		{
			PStrPrintf( pszMeshNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s.gr2", pszFilePath, pszTempName );
		}
		if ( ! FileExists( pszMeshNameWithPath ) )
		{
			ASSERTV(0, "File %s Missing - referenced in mesh %s, model %s", pszMeshNameWithPath, pMesh->Name, pGrannyModel->Name);
		}
		DRLG_ROOM * pRoom = & pRooms[ nRoomIndex ];
		pRooms[ nRoomIndex ].nSameNameIndex = nMeshIndex;
		(*pnRoomCount)++;

		for (int nTriangleGroup = 0; nTriangleGroup < nTriangleGroupCount; ++nTriangleGroup)
		{

			// Get the group
			granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ nTriangleGroup ];

			granny_material * pMaterial = pMesh->MaterialBindings[ pGrannyTriangleGroup->MaterialIndex ].Material;

			const char * pszMaterialName = pMaterial->Name;

			// -- origin --
			// origins show us how to set the translation matrix for a room
			if ( PStrICmp( pszMaterialName, "origin" ) == 0 )
			{
				e_GrannyGetVectorAndRotationFromOriginMesh( pMesh, pGrannyTriangleGroup, &pRoom->vLocation, &pRoom->fRotation );
			} else {
			//}
			// The rest of the geometry helps us create a Axis-Aligned Bounding box for the room
			//if ( strcmpi( pszMaterialName, "collision" ) == 0 )
			//{
				// get a translation matrix from the origin
				//int nVertexCount = GrannyGetMeshVertexCount ( pMesh );
				//SIMPLE_VERTEX * pVertices = (SIMPLE_VERTEX *) MALLOC( NULL, nVertexCount * sizeof( SIMPLE_VERTEX ) );
				//GrannyCopyMeshVertices( pMesh, g_ptSimpleVertexDataType, (BYTE *)pVertices );
				//ASSERT( nVertexCount >= 1 );

				// now going to use the room definition bounding boxes instead of rule geometry
				if(AppIsHellgate())
				{
					pRoom->tBoundingBox.vMin = VECTOR( 0.0f, 0.0f, 0.0f );
					pRoom->tBoundingBox.vMax = VECTOR( 0.0f, 0.0f, 0.0f );
				}
/*				pRoom->tBoundingBox.vMin = pVertices[ 0 ].vPosition;
				pRoom->tBoundingBox.vMax = pVertices[ 0 ].vPosition;
				for (int nVertex = 1; nVertex < nVertexCount; nVertex++ )
				{
					SIMPLE_VERTEX * pVertex = & pVertices[ nVertex ];
					BoundingBoxExpandByPoint( & pRoom->tBoundingBox, (VECTOR *) & pVertex->vPosition );
				}*/

				//FREE ( NULL, pVertices );
			}
		}
	}

	// drb - the bottom here doesn't take rotation into consideration. must fix
	// apply the room origin transformation to every room
	for ( int nRoom = 0; nRoom < *pnRoomCount; nRoom ++ )
	{
		DRLG_ROOM * pRoom = & pRooms[ nRoom ];

		pRoom->vLocation -= vOffsetToRuleOrigin;
		//pRoom->tBoundingBox.vMin -= vOffsetToRuleOrigin;
		//pRoom->tBoundingBox.vMax -= vOffsetToRuleOrigin;

		pRoom->fRotation -= fRuleRotation;

		VectorZRotation( pRoom->vLocation, fRuleRotation );
		//BoundingBoxZRotation( &pRoom->tBoundingBox, fRuleRotation );
	}
}
