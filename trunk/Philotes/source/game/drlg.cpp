//----------------------------------------------------------------------------
// drlg.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "drlg.h"
#include "room.h"
#include "room_layout.h"

#include "drlgpriv.h"
#include "excel.h"
#include "level.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "units.h" // also includes game.h
#include "unit_priv.h"
#include "objects.h"
#include "filepaths.h"
#include "stringtable.h"
#include "player.h"
#include "monsters.h"
#include "room_layout.h"
#include "s_quests.h"

#include "performance.h"
#include "colors.h"
#include "complexmaths.h"
#include "clients.h"
#include "gameconfig.h"
#include "globalindex.h"

#include "e_settings.h"
#include "e_material.h"
#include "e_portal.h"
#include "e_environment.h"
#include "e_automap.h"

#include "LevelAreas.h"
#include "Quest.h"
#include "dungeon.h"
#include "picker.h"

#if ISVERSION(SERVER_VERSION)
#include "drlg.cpp.tmh"
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "svrstd.h"
#endif

using namespace FSSE;

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MAX_VISIBLE_ROOMS 600

#define REPLACEMENT_ERROR 0.1f


//----------------------------------------------------------------------------
struct DRLG_LEVEL
{
	int									nRoomCount;
	CArrayIndexedEx<DRLG_ROOM, 3072>	pRooms;			// 3072 = a bin size in the allocator
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
// DRLG Debug window
//
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define DAVE_DRLG_DEBUG		1

#ifdef DAVE_DRLG_DEBUG


#include "s_gdi.h"

static HWND sghDRLGDebugWnd;
static char	sgszAppName[] = "DRLG Debug";

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static int sMyFloat2Int( float fVal )
{
	return (int)FLOOR( fVal + 0.5f ) * 2;
}

static DRLG_LEVEL * sgpDebugLevel;
static int sgnDebugSelectVal = -1;

static void sDebugSelect( int nMouseX, int nMouseY )
{
	sgnDebugSelectVal = -1;

	int nRoomId = sgpDebugLevel->pRooms.GetFirst();
	for ( int i = 0; i < sgpDebugLevel->nRoomCount; i++ )
	{
		DRLG_ROOM *pRoom = sgpDebugLevel->pRooms.Get( nRoomId );

		int nWidth, nHeight;
		e_GetWindowSize( nWidth, nHeight );
		int nX1 = sMyFloat2Int( pRoom->tBoundingBox.vMin.fX ) + ( nWidth/2 );
		int nY1 = sMyFloat2Int( pRoom->tBoundingBox.vMin.fY ) + ( nHeight/2 );
		int nX2 = sMyFloat2Int( pRoom->tBoundingBox.vMax.fX ) + ( nWidth/2 );
		int nY2 = sMyFloat2Int( pRoom->tBoundingBox.vMax.fY ) + ( nHeight/2 );

		if ( ( nMouseX >= nX1 ) && ( nMouseX <= nX2 ) &&
			 ( nMouseY >= nY1 ) && ( nMouseY <= nY2 ) )
			 sgnDebugSelectVal = nRoomId;

		nRoomId = sgpDebugLevel->pRooms.GetNextId( nRoomId );
	}
}

static void sHullDebugSelect( int nMouseX, int nMouseY )
{
	sgnDebugSelectVal = -1;

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	int nRoomId = sgpDebugLevel->pRooms.GetFirst();
	for ( int i = 0; i < sgpDebugLevel->nRoomCount; i++ )
	{
		DRLG_ROOM * room = sgpDebugLevel->pRooms.Get( nRoomId );

		int nX1 = sMyFloat2Int( room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
		int nY1 = sMyFloat2Int( room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );
		int nX2 = sMyFloat2Int( room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
		int nY2 = sMyFloat2Int( room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );

		if ( ( nMouseX >= nX1 ) && ( nMouseX <= nX2 ) &&
			( nMouseY >= nY1 ) && ( nMouseY <= nY2 ) )
			sgnDebugSelectVal = nRoomId;

		nRoomId = sgpDebugLevel->pRooms.GetNextId( nRoomId );
	}
}

static void sCityDebugDraw2( DRLG_LEVEL *pDebugLevel )
{
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	int nRoomId = pDebugLevel->pRooms.GetFirst();
	for ( int i = 0; i < pDebugLevel->nRoomCount; i++ )
	{
		DRLG_ROOM *pRoom = pDebugLevel->pRooms.Get( nRoomId );

		int nX1 = sMyFloat2Int( pRoom->tBoundingBox.vMin.fX ) + ( nWidth/2 );
		int nY1 = sMyFloat2Int( pRoom->tBoundingBox.vMin.fY ) + ( nHeight/2 );
		int nX2 = sMyFloat2Int( pRoom->tBoundingBox.vMax.fX ) + ( nWidth/2 );
		int nY2 = sMyFloat2Int( pRoom->tBoundingBox.vMax.fY ) + ( nHeight/2 );

		gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_white );

		if ( nRoomId == sgnDebugSelectVal )
			gdi_drawbox( nX1+1, nY1+1, nX2-1, nY2-1, gdi_color_yellow );

		nX1 = sMyFloat2Int( pRoom->vLocation.fX ) + ( nWidth/2 );
		nY1 = sMyFloat2Int( pRoom->vLocation.fY ) + ( nHeight/2 );
		VECTOR vNormal( 0, 5, 0 );
		VectorZRotation( vNormal, pRoom->fRotation );
		nX2 = nX1 + sMyFloat2Int( vNormal.fX );
		nY2 = nY1 + sMyFloat2Int( vNormal.fY );

		gdi_drawline( nX1, nY1, nX2, nY2, gdi_color_green );

		nRoomId = pDebugLevel->pRooms.GetNextId( nRoomId );
	}

	nRoomId = pDebugLevel->pRooms.GetFirst();
	for ( int i = 0; i < pDebugLevel->nRoomCount; i++ )
	{
		DRLG_ROOM *pRoom = pDebugLevel->pRooms.Get( nRoomId );

		int nX1 = sMyFloat2Int( pRoom->vLocation.fX ) + ( nWidth/2 );
		int nY1 = sMyFloat2Int( pRoom->vLocation.fY ) + ( nHeight/2 );

		gdi_drawpixel( nX1, nY1, gdi_color_red );

		nRoomId = pDebugLevel->pRooms.GetNextId( nRoomId );
	}

	if ( sgnDebugSelectVal >= 0 )
	{
		char szTemp[128];
		PStrPrintf( szTemp, 128, "Room Id = %i", sgnDebugSelectVal );
		gdi_drawstring( 10, 10, szTemp );

		DRLG_ROOM *pRoom = sgpDebugLevel->pRooms.Get( sgnDebugSelectVal );
		PStrPrintf( szTemp, 128, "X=%2.4f Y=%2.4f Z=%2.4f", pRoom->vLocation.fX, pRoom->vLocation.fY, pRoom->vLocation.fZ );
		gdi_drawstring( 10, 30, szTemp );

		PStrPrintf( szTemp, 128, "Angle=%2.4f", pRoom->fRotation );
		gdi_drawstring( 10, 50, szTemp );

		PStrPrintf( szTemp, 128, "Box= (%2.4f,%2.4f,%2.4f)-(%2.4f,%2.4f,%2.4f)", pRoom->tBoundingBox.vMin.fX, pRoom->tBoundingBox.vMin.fY, pRoom->tBoundingBox.vMin.fZ,
				pRoom->tBoundingBox.vMax.fX, pRoom->tBoundingBox.vMax.fY, pRoom->tBoundingBox.vMax.fZ );
		gdi_drawstring( 10, 70, szTemp );
	}
}

static void sCityDebugDraw3( DRLG_LEVEL *pDebugLevel )
{
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	int nRoomId = pDebugLevel->pRooms.GetFirst();
	for ( int i = 0; i < pDebugLevel->nRoomCount; i++ )
	{
		DRLG_ROOM *room = pDebugLevel->pRooms.Get( nRoomId );

		int nX1 = sMyFloat2Int( room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
		int nY1 = sMyFloat2Int( room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );
		int nX2 = sMyFloat2Int( room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
		int nY2 = sMyFloat2Int( room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );

		gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_white );

		if ( nRoomId == sgnDebugSelectVal )
		{
			// highlight
			gdi_drawbox( nX1+1, nY1+1, nX2-1, nY2-1, gdi_color_yellow );

			char szTemp[128];
			PStrPrintf( szTemp, 128, "Room Id = %i", sgnDebugSelectVal );
			gdi_drawstring( 10, 10, szTemp );

			PStrPrintf( szTemp, 128, "X=%2.4f Y=%2.4f Z=%2.4f", room->vLocation.fX, room->vLocation.fY, room->vLocation.fZ );
			gdi_drawstring( 10, 30, szTemp );

			PStrPrintf( szTemp, 128, "Angle=%2.4f", room->fRotation );
			gdi_drawstring( 10, 50, szTemp );

			PStrPrintf( szTemp, 128, "Box= (%2.4f,%2.4f,%2.4f)-(%2.4f,%2.4f,%2.4f)",
				( room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX ),
				( room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY ),
				( room->pHull->aabb.center.fZ - room->pHull->aabb.halfwidth.fZ ),
				( room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX ),
				( room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY ),
				( room->pHull->aabb.center.fZ + room->pHull->aabb.halfwidth.fZ ) );
			gdi_drawstring( 10, 70, szTemp );
		}

		nRoomId = pDebugLevel->pRooms.GetNextId( nRoomId );
	}
}

static void sDrawHull( CONVEXHULL * hull, BYTE color )
{
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	for ( int t = 0; t < hull->face_count; t+=3 )
	{
		FACE * face = hull->faces[t];

		VECTOR * v = face->vertices[0];
		int nX1 = sMyFloat2Int( v->fX ) + ( nWidth/2 );
		int nY1 = sMyFloat2Int( v->fY ) + ( nHeight/2 );
		v = face->vertices[1];
		int nX2 = sMyFloat2Int( v->fX ) + ( nWidth/2 );
		int nY2 = sMyFloat2Int( v->fY ) + ( nHeight/2 );
		v = face->vertices[2];
		int nX3 = sMyFloat2Int( v->fX ) + ( nWidth/2 );
		int nY3 = sMyFloat2Int( v->fY ) + ( nHeight/2 );

		gdi_drawline( nX1, nY1, nX2, nY2, color );
		gdi_drawline( nX2, nY2, nX3, nY3, color );
		gdi_drawline( nX3, nY3, nX1, nY1, color );
	}

}

static void sHullDebugDraw( DRLG_LEVEL *pDebugLevel )
{
	DRLG_ROOM * room = pDebugLevel->pRooms.Get( 16 );
	sDrawHull( room->pHull, gdi_color_red );

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	int nX1 = sMyFloat2Int( room->tBoundingBox.vMin.fX ) + ( nWidth/2 );
	int nY1 = sMyFloat2Int( room->tBoundingBox.vMin.fY ) + ( nHeight/2 );
	int nX2 = sMyFloat2Int( room->tBoundingBox.vMax.fX ) + ( nWidth/2 );
	int nY2 = sMyFloat2Int( room->tBoundingBox.vMax.fY ) + ( nHeight/2 );
	gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_red );

	nX1 = sMyFloat2Int( room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
	nY1 = sMyFloat2Int( room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );
	nX2 = sMyFloat2Int( room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
	nY2 = sMyFloat2Int( room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );
	gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_red );

	room = pDebugLevel->pRooms.Get( 73 );
	sDrawHull( room->pHull, gdi_color_green );

	nX1 = sMyFloat2Int( room->tBoundingBox.vMin.fX ) + ( nWidth/2 );
	nY1 = sMyFloat2Int( room->tBoundingBox.vMin.fY ) + ( nHeight/2 );
	nX2 = sMyFloat2Int( room->tBoundingBox.vMax.fX ) + ( nWidth/2 );
	nY2 = sMyFloat2Int( room->tBoundingBox.vMax.fY ) + ( nHeight/2 );
	gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_green );

	nX1 = sMyFloat2Int( room->pHull->aabb.center.fX - room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
	nY1 = sMyFloat2Int( room->pHull->aabb.center.fY - room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );
	nX2 = sMyFloat2Int( room->pHull->aabb.center.fX + room->pHull->aabb.halfwidth.fX ) + ( nWidth/2 );
	nY2 = sMyFloat2Int( room->pHull->aabb.center.fY + room->pHull->aabb.halfwidth.fY ) + ( nHeight/2 );
	gdi_drawbox( nX1, nY1, nX2, nY2, gdi_color_green );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

LRESULT CALLBACK DRLGWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		//sDebugSelect( LOWORD( lParam ), HIWORD( lParam ) );
		sHullDebugSelect( LOWORD( lParam ), HIWORD( lParam ) );
		break;

	case WM_KEYDOWN:
		if ( wParam == VK_ESCAPE )
		{
			DestroyWindow( sghDRLGDebugWnd );
		}
		break;

	case WM_DESTROY :
		PostQuitMessage( 0 );
		return TRUE;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sDRLGInitDebugWindow( HINSTANCE hInstance )
{
	WNDCLASSEX	tWndClass;

	tWndClass.cbSize = sizeof( tWndClass );
	tWndClass.style = CS_HREDRAW | CS_VREDRAW;
	tWndClass.lpfnWndProc = DRLGWndProc;
	tWndClass.cbClsExtra = 0;
	tWndClass.cbWndExtra = 0;
	tWndClass.hInstance = hInstance;
	tWndClass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	tWndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	tWndClass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
	tWndClass.lpszMenuName = NULL;
	tWndClass.lpszClassName = sgszAppName;
	tWndClass.hIconSm = LoadIcon( NULL, IDI_APPLICATION );

	RegisterClassEx( &tWndClass );

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );
	ASSERT( nWidth && nHeight );
	int nX = ( GetSystemMetrics( SM_CXSCREEN ) - nWidth ) >> 1;
	int nY = ( GetSystemMetrics( SM_CYSCREEN ) - nHeight ) >> 1;

	int nWinWidth = nWidth + ( GetSystemMetrics( SM_CXFIXEDFRAME ) * 2 );
	int nWinHeight = nHeight + ( GetSystemMetrics( SM_CYFIXEDFRAME ) * 2 ) + GetSystemMetrics( SM_CYCAPTION );

    sghDRLGDebugWnd = CreateWindowEx( 0, sgszAppName, sgszAppName, WS_POPUP | WS_CAPTION,
		nX, nY, nWinWidth, nWinHeight,
        NULL, NULL, hInstance, NULL );

	ShowWindow( sghDRLGDebugWnd, SW_SHOWNORMAL );
	UpdateWindow( sghDRLGDebugWnd );

	gdi_init( sghDRLGDebugWnd, nWidth, nHeight );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sDRLGCloseDebugWindow()
{
	gdi_free();
	DestroyWindow( sghDRLGDebugWnd );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sDRLGDebugGDIWnd( DRLG_LEVEL *pLevel, void ( *pfnDraw )( DRLG_LEVEL *pLevel ) )
{
	sgpDebugLevel = pLevel;

	// create a gdi window for debug
	sDRLGInitDebugWindow( AppGetHInstance() );

	// draw debug
	BOOL fNoQuit = TRUE;

	while ( fNoQuit )
	{
		MSG tMsg;
		if ( PeekMessage( &tMsg, NULL, 0, 0, PM_NOREMOVE ) )
		{
			fNoQuit = GetMessage( &tMsg, NULL, 0, 0 );
			TranslateMessage( &tMsg );
			DispatchMessage( &tMsg );
		} else {
			// clear
			gdi_clearscreen();

			pfnDraw( pLevel );

			// blit/copy
			gdi_blit( sghDRLGDebugWnd );
		}
	}

	// close up
	sDRLGCloseDebugWindow();
}

#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//
// DRLG non-debug code
//
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sAttemptToReplace(GAME * pGame, DRLG_LEVEL * pLevel, const DRLG_DEFINITION * drlg_definition, int * pnRoomToReplace,
							  DRLG_SUBSTITUTION_RULE * pSub, int nReplacementIndex, BOOL bForce )
{
	DRLG_ROOM * pFirstRoomToReplace = pLevel->pRooms.Get( pnRoomToReplace[ 0 ] );

	// make sure replacement is within bounds
	ASSERT( nReplacementIndex < pSub->nReplacementCount );

	// Get bounding boxes and matrices for the replacements
	BOUNDING_BOX pReplacementBoundingBoxes[ MAX_ROOMS_PER_GROUP ];
	VECTOR	vReplacementLocations[ MAX_ROOMS_PER_GROUP ];
	float	fReplacementRotations[ MAX_ROOMS_PER_GROUP ];
	CONVEXHULL * pReplacementHulls[ MAX_ROOMS_PER_GROUP ];
	ASSERT( pSub->pnReplacementRoomCount[ nReplacementIndex ] <= MAX_ROOMS_PER_GROUP );
	for (int nReplacementRoom = 0; 
		nReplacementRoom < pSub->pnReplacementRoomCount[ nReplacementIndex ]; 
		nReplacementRoom++ )
	{
		DRLG_ROOM * pReplacementRoom = pSub->ppReplacementRooms[ nReplacementIndex ] + nReplacementRoom;
		ASSERT( nReplacementIndex < pSub->nReplacementCount );

		vReplacementLocations[ nReplacementRoom ] = pReplacementRoom->vLocation;
		VectorZRotation( vReplacementLocations[ nReplacementRoom ], pFirstRoomToReplace->fRotation );
		vReplacementLocations[ nReplacementRoom ] += pFirstRoomToReplace->vLocation;
		fReplacementRotations[ nReplacementRoom ] = pFirstRoomToReplace->fRotation + pReplacementRoom->fRotation;
		// translate hulls
		char szPath    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		char szName    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";

		// does the drlg rule have a specific layout associated with it? If so, remove it for loading the room definition
		const char * pBracket = PStrRChr( pReplacementRoom->pszFileName, '[' );
		if ( pBracket )
		{
			PStrCopy(szName, pReplacementRoom->pszFileName, (int)(pBracket - pReplacementRoom->pszFileName + 1));
		}
		else
		{
			PStrCopy( szName, pReplacementRoom->pszFileName, DEFAULT_FILE_WITH_PATH_SIZE );
		}

		// is this definition in another folder?
		OVERRIDE_PATH tOverridePath;
		const char * pDot = PStrRChr( szName, '.' );
		if ( pDot )
		{
			OverridePathByCode( *( (DWORD*)( pDot + 1 ) ), &tOverridePath );
			PStrCopy(szName, szName, (int)(pDot - szName + 1));
		}
		else
		{
			OverridePathByLine( drlg_definition->nPathIndex, &tOverridePath );
		}
		ASSERT( tOverridePath.szPath[ 0 ] );
		PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, tOverridePath.szPath );
		char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		PStrCopy( szFilePath, tOverridePath.szPath, DEFAULT_FILE_WITH_PATH_SIZE );
		PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, szName );
		//int nId = ExcelGetLineByStringIndex( pGame, DATATABLE_ROOM_INDEX, szFilePath );
		int nId = RoomFileGetRoomIndexLine( szFilePath );
		ASSERTX_RETFALSE ( nId != INVALID_ID, szFilePath );
		ROOM_DEFINITION * pRoomDef = RoomDefinitionGet( pGame, nId, IS_CLIENT(pGame) );
		if ( ! pRoomDef )
			return FALSE;
		if ( pRoomDef->nHullVertexCount )
		{
			ASSERT( pRoomDef->nHullVertexCount < MAX_HULL_VERTICES );
			VECTOR vReplacementHulls[ MAX_HULL_VERTICES ];
			for ( int v = 0; v < pRoomDef->nHullVertexCount; v++ )
			{
				vReplacementHulls[ v ] = pRoomDef->pHullVertices[ v ];
				VectorZRotation( vReplacementHulls[ v ], pReplacementRoom->fRotation );
				vReplacementHulls[ v ] += pReplacementRoom->vLocation;
				VectorZRotation( vReplacementHulls[ v ], pFirstRoomToReplace->fRotation );
				vReplacementHulls[ v ] += pFirstRoomToReplace->vLocation;
			}
			pReplacementHulls[ nReplacementRoom ] = HullCreate(
						&vReplacementHulls[0], pRoomDef->nHullVertexCount,
						pRoomDef->pHullTriangles, pRoomDef->nHullTriangleCount,
						pRoomDef->tHeader.pszName );
			//trace( "CHC1 - Hull created at address: %p\n", pReplacementHulls[ nReplacementRoom ] );
		}
		else
		{
			pReplacementHulls[ nReplacementRoom ] = NULL;
			pReplacementBoundingBoxes[ nReplacementRoom ] = pRoomDef->tBoundingBox;
			BoundingBoxZRotation( &pReplacementBoundingBoxes[ nReplacementRoom ], pReplacementRoom->fRotation );
			pReplacementBoundingBoxes[ nReplacementRoom ].vMin += pReplacementRoom->vLocation;
			pReplacementBoundingBoxes[ nReplacementRoom ].vMax += pReplacementRoom->vLocation;
			BoundingBoxZRotation( &pReplacementBoundingBoxes[ nReplacementRoom ], pFirstRoomToReplace->fRotation );
			pReplacementBoundingBoxes[ nReplacementRoom ].vMin += pFirstRoomToReplace->vLocation;
			pReplacementBoundingBoxes[ nReplacementRoom ].vMax += pFirstRoomToReplace->vLocation;
		}
	}

	// Check the bounding boxes against the other rooms in the level - ignoring what we will replace
	BOOL fCollides = FALSE;

	if ( !bForce )
	{
		for ( int nRoomToCheck = pLevel->pRooms.GetFirst(); 
			! fCollides && nRoomToCheck != INVALID_ID; 
			nRoomToCheck = pLevel->pRooms.GetNextId( nRoomToCheck ) )
		{
			// don't compare against the rule rooms
			int nRuleRoom;
			for ( nRuleRoom = 0; nRuleRoom < pSub->nRuleRoomCount; nRuleRoom++ )
			{
				if ( pnRoomToReplace[ nRuleRoom ] == nRoomToCheck )
					break;
			}
			if ( nRuleRoom < pSub->nRuleRoomCount )
				continue;

			DRLG_ROOM * pRoomToCollide = pLevel->pRooms.Get( nRoomToCheck );

			// check the room in the level against the rooms in the replacement
			for (int nReplacementRoom = 0; 
				! fCollides && nReplacementRoom < pSub->pnReplacementRoomCount[ nReplacementIndex ]; 
				nReplacementRoom++ )
			{
				// do we have cool new dark forces hulls, or shitty old bounding boxes?
				if ( pRoomToCollide->pHull && pReplacementHulls[ nReplacementRoom ] )
				{
					// did they collide? - test hulls
					if ( ConvexHullBooleanCollideTest( pRoomToCollide->pHull, pReplacementHulls[ nReplacementRoom ] ) )
						fCollides = TRUE;
				}
				else if ( BoundingBoxCollide( & pRoomToCollide->tBoundingBox, & pReplacementBoundingBoxes[ nReplacementRoom ], REPLACEMENT_ERROR ))
				{
					fCollides = TRUE;
				}
			}
		}
	}

	// if it doesn't collide, then make the substitution
	if ( ! fCollides )
	{
	
		SUBLEVELID idSubLevel = INVALID_ID;
		
		// remove rule rooms
		for ( int nRuleRoom = 0; nRuleRoom < pSub->nRuleRoomCount; nRuleRoom ++ )
		{
			DRLG_ROOM * pRoom = pLevel->pRooms.Get( pnRoomToReplace[ nRuleRoom ] );
			
			// keep track of the sublevel of the rooms we're replacing (they must all be the same)
			ASSERTX( idSubLevel == INVALID_ID || idSubLevel == pRoom->idSubLevel, "Replacing rooms that are on different sublevels!" );
			idSubLevel = pRoom->idSubLevel;
			
			//trace( "Destroying room %i with hull address: %p\n", pnRoomToReplace[ nRuleRoom ], pRoom->pHull );
			if ( pRoom->pHull )
			{
				//trace( "CHD1 - Hull destroyed at: %p\n", pRoom->pHull );
				ConvexHullDestroy( pRoom->pHull );
			}
			pLevel->pRooms.Remove( pnRoomToReplace[ nRuleRoom ] );
			pLevel->nRoomCount--;
		}

		// add replacements
		DRLG_ROOM * pReplacementRoom = pSub->ppReplacementRooms[ nReplacementIndex ];
		for (int i = 0; i < pSub->pnReplacementRoomCount[ nReplacementIndex ]; i++)
		{
			int nNewIndex;
			DRLG_ROOM * pNewRoom = pLevel->pRooms.Add( & nNewIndex );
			ASSERT( nNewIndex != INVALID_ID );
			PStrCopy( pNewRoom->pszFileName, pReplacementRoom->pszFileName, MAX_XML_STRING_LENGTH );
			pNewRoom->tBoundingBox = pReplacementBoundingBoxes[ i ];
			pNewRoom->vLocation = vReplacementLocations[ i ];
			pNewRoom->fRotation = fReplacementRotations[ i ];
			pNewRoom->pHull = pReplacementHulls[ i ];
			
			// must have sub level
			ASSERTX( idSubLevel != INVALID_ID, "Replacing room, but no sublevel was found from old rooms" );
			pNewRoom->idSubLevel = idSubLevel;
			
			pReplacementRoom++;
			pLevel->nRoomCount++;
			//trace( "Adding room %i, with hull address: %p\n", nNewIndex, pNewRoom->pHull );
		}
		return TRUE;
	}
	else
	{
		// remove hulls
		for ( int i = 0; i < pSub->pnReplacementRoomCount[ nReplacementIndex ]; i++ )
		{
			if ( pReplacementHulls[ i ] )
			{
				//trace( "CHD2 - Hull destroyed at: %p\n", pReplacementHulls[ i ] );
				ConvexHullDestroy( pReplacementHulls[ i ] );
			}
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL sFindRoomToReplace( DRLG_LEVEL * pLevel, DRLG_ROOM * pRuleRoom, int * pnRoomToReplace, int nStartId, VECTOR * pvLocation, float fRotation )
{
	*pnRoomToReplace = nStartId;

	if ( nStartId == INVALID_ID )
		return FALSE;

	fRotation = RadiansToDegrees( fRotation );
	// find an example of the rule in the level - right now we are assuming that rules have only one room
	for ( ; ; )
	{
		// look for the room
		DRLG_ROOM * pRoomToReplace = pLevel->pRooms.Get( *pnRoomToReplace );
		if ( PStrCmp( pRoomToReplace->pszFileName, pRuleRoom->pszFileName ) == 0 ) // replace with Id comparison later
		{
			if ( pvLocation )
			{
				float fDelta = fabsf( pvLocation->fX - pRoomToReplace->vLocation.fX );
				fDelta += fabsf( pvLocation->fY - pRoomToReplace->vLocation.fY );
				fDelta += fabsf( pvLocation->fZ - pRoomToReplace->vLocation.fZ );
				if ( fDelta < 0.05f )
				{
					// now check the orientations and make sure they match
 					float fAngle1 = RadiansToDegrees( pRoomToReplace->fRotation );
					float fAngleDelta = fabs( fAngle1 - fRotation );
					while( fAngleDelta > 180 )
					{
						fAngleDelta -= 360.0f;
					}
					if( fabs( fAngleDelta ) < 1 )
					{
						return TRUE;
					}
				}

			} else
				return TRUE;
		}

		*pnRoomToReplace = pLevel->pRooms.GetNextId( *pnRoomToReplace );

		if ( *pnRoomToReplace == INVALID_ID )
		{
			*pnRoomToReplace = pLevel->pRooms.GetFirst();
		}

		if ( *pnRoomToReplace == nStartId )
			break;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGCopyConnectionsFromDefinition(
	GAME* game,
	ROOM* room)
{
	// initialize the connections by copying them from the definition and then translating them to the room's location
	room->nConnectionCount = room->pDefinition->nConnectionCount;
	room->pConnections = (ROOM_CONNECTION*)GREALLOC(game, room->pConnections, sizeof(ROOM_CONNECTION) * room->nConnectionCount);
	if(!room->pConnections)
		return;
	memcpy(room->pConnections, room->pDefinition->pConnections, sizeof(ROOM_CONNECTION) * room->nConnectionCount);
	for (int nConnection = 0; nConnection < room->nConnectionCount; nConnection++)
	{
		ROOM_CONNECTION* connection = &room->pConnections[nConnection];
		for (int ii = 0; ii < ROOM_CONNECTION_CORNER_COUNT; ii++)
		{
			MatrixMultiply(&connection->pvBorderCorners[ii], &connection->pvBorderCorners[ii], &room->tWorldMatrix);
		}
		MatrixMultiply(&connection->vBorderCenter, &connection->vBorderCenter, &room->tWorldMatrix);
		connection->tConnectionBounds.vMin = connection->pvBorderCorners[0];
		connection->tConnectionBounds.vMax = connection->pvBorderCorners[0];
		for (int ii = 1; ii < ROOM_CONNECTION_CORNER_COUNT; ii++)
		{
			BoundingBoxExpandByPoint(&connection->tConnectionBounds, &connection->pvBorderCorners[ii]);
		}
		connection->pRoom = NULL;
		VECTOR vNormalCenter(0.0f);
		MatrixMultiply(&vNormalCenter, &vNormalCenter, &room->tWorldMatrix);
		MatrixMultiply(&connection->vBorderNormal, &connection->vBorderNormal, &room->tWorldMatrix);
		VectorSubtract(connection->vBorderNormal, connection->vBorderNormal, vNormalCenter);
		VectorNormalize(connection->vBorderNormal, connection->vBorderNormal);
		MatrixMultiply(&connection->vPortalCentroid, &connection->vPortalCentroid, &room->tWorldMatrix);
	}

	room->ppConnectedRooms = (ROOM **)GREALLOCZ(game, room->ppConnectedRooms, sizeof(ROOM *) * room->nConnectionCount);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddRoomToConnectedRoomsList(
	GAME * game,
	ROOM * room,
	ROOM * roomtoadd)
{
	ASSERT_RETURN(room->ppConnectedRooms);
	ASSERT_RETURN(room != roomtoadd);
	BOOL found = FALSE;
	for (unsigned int ii = 0; ii < room->nConnectedRoomCount; ++ii)
	{
		if (room->ppConnectedRooms[ii] == roomtoadd)
		{
			found = TRUE;
			return;
		}
	}
	ASSERT_RETURN(room->nConnectedRoomCount < (unsigned int)room->nConnectionCount);
	room->ppConnectedRooms[room->nConnectedRoomCount++] = roomtoadd;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDRLGCreateRoomConnection(
	GAME * pGame,
	ROOM * pRoom,
	ROOM_CONNECTION* pConnection,
	int nConnection,
	ROOM* pDestRoom,
	ROOM_CONNECTION* pDestConnection,
	int nDestConnect)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pConnection);
	ASSERT_RETURN(pDestRoom);
	ASSERT_RETURN(pDestConnection);

	if(pConnection->pRoom)
	{
		ASSERT(pConnection->pRoom == pDestRoom);
		ASSERT(pDestConnection->pRoom == pRoom);
		return;
	}

	ASSERT_RETURN(pRoom->nConnectionCount <= pRoom->pDefinition->nConnectionCount);

	pConnection->pRoom = pDestRoom;
	pConnection->bOtherConnectionIndex = (BYTE)nDestConnect;
	pDestConnection->pRoom = pRoom;
	pDestConnection->bOtherConnectionIndex = (BYTE)nConnection;
	pConnection->nCullingPortalID = INVALID_ID;
	pDestConnection->nCullingPortalID = INVALID_ID;
	sAddRoomToConnectedRoomsList(pGame, pRoom, pDestRoom);
	sAddRoomToConnectedRoomsList(pGame, pDestRoom, pRoom);
	return;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGCreateRoomConnections(
	GAME* game,
	LEVEL* level,
	ROOM* room)
{
	ASSERT_RETURN( level );
	ASSERT_RETURN( room->pHull );

	// look through all of the other rooms and see which ones border the room
	for (ROOM* pRoomSearch = LevelGetFirstRoom(level); pRoomSearch; pRoomSearch = LevelGetNextRoom(pRoomSearch))
	{
		if (pRoomSearch == room)
		{
			continue;
		}
		// this can happen in hammer if you load a background, then do viewDRLG
		if (!pRoomSearch->pHull)
		{
			continue;
		}

		if ( ConvexHullManhattenDistance( room->pHull, pRoomSearch->pHull ) < 0.1f )
		{	
			// find the connections that border the search room and add them to the ROOM
			for (int nConnection = 0; nConnection < room->nConnectionCount; nConnection++)
			{
				ROOM_CONNECTION* pConnection = &room->pConnections[nConnection];
				for (int nDestConnect = 0; nDestConnect < pRoomSearch->nConnectionCount; nDestConnect++)
				{
					ROOM_CONNECTION* pDestConnection = &pRoomSearch->pConnections[nDestConnect];
					if (BoundingBoxManhattanDistance(&pDestConnection->tConnectionBounds, &pConnection->vBorderCenter) < 0.1f)
					{
						sDRLGCreateRoomConnection(game, room, pConnection, nConnection, pRoomSearch, pDestConnection, nDestConnect);
						break;
					}
				}
			}
		}
	}
	return;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sTestRing3Connection(
	ROOM_CONNECTION* pConnectionFirst,
	ROOM_CONNECTION* pConnectionSecond,
	ROOM_CONNECTION* pConnectionThird)
{

	VECTOR pvNormals[4];	// these are the normals for the planes between the points of connection First and Second
	VECTOR pvPoints[4];		// these are the points for the planes between the points of connection First and Second
									
	// the border points are sorted by X, Y and then Z - so we know which ones to match up.
	VECTOR pvDeltas[2];
	ASSERT(ROOM_CONNECTION_CORNER_COUNT == 4);

	// bottom plane
	VectorSubtract(pvDeltas[0], pConnectionFirst->pvBorderCorners[2], pConnectionFirst->pvBorderCorners[0]);
	VectorSubtract(pvDeltas[1], pConnectionThird->pvBorderCorners[0], pConnectionFirst->pvBorderCorners[0]);
	VectorCross(pvNormals[0], pvDeltas[0], pvDeltas[1]);
	pvPoints[0] = pConnectionFirst->pvBorderCorners[0];
			
	// top plane
	VectorSubtract(pvDeltas[0], pConnectionFirst->pvBorderCorners[3], pConnectionFirst->pvBorderCorners[1]);
	VectorSubtract(pvDeltas[1], pConnectionThird->pvBorderCorners[1], pConnectionFirst->pvBorderCorners[1]);
	VectorCross(pvNormals[1], pvDeltas[0], pvDeltas[1]);
	pvPoints[1] = pConnectionFirst->pvBorderCorners[1];

	// left plane
	VectorSubtract(pvDeltas[0], pConnectionFirst->pvBorderCorners[1], pConnectionFirst->pvBorderCorners[0]);
	VectorSubtract(pvDeltas[1], pConnectionThird->pvBorderCorners[0], pConnectionFirst->pvBorderCorners[0]);
	VectorCross(pvNormals[2], pvDeltas[0], pvDeltas[1]);
	pvPoints[2] = pConnectionFirst->pvBorderCorners[2];

	// right plane
	VectorSubtract(pvDeltas[0], pConnectionFirst->pvBorderCorners[3], pConnectionFirst->pvBorderCorners[2]);
	VectorSubtract(pvDeltas[1], pConnectionThird->pvBorderCorners[2], pConnectionFirst->pvBorderCorners[2]);
	VectorCross(pvNormals[3], pvDeltas[0], pvDeltas[1]);
	pvPoints[3] = pConnectionFirst->pvBorderCorners[0];

	// see if all four points in the third connection are on the same side of the top and bottom plane
	DWORD dwSideFlags[4];
	for (int nPlane = 0; nPlane < 4; nPlane++)
	{
		dwSideFlags[nPlane] = 0;
		for (int nPoint = 0; nPoint < ROOM_CONNECTION_CORNER_COUNT; nPoint++)
		{
			VECTOR vDelta;
			VectorSubtract(vDelta, pConnectionSecond->pvBorderCorners[nPoint], pvPoints[nPlane]);
			float fDot = VectorDot(vDelta, pvNormals[nPlane]);
			if (fDot > 0)
			{
				dwSideFlags[nPlane] |= 1 << nPoint;
			}
		}
	}

	if (dwSideFlags[0] == 0 && dwSideFlags[1] == 0)
	{
		return FALSE;
	}
	if (dwSideFlags[0] == 0xf && dwSideFlags[1] == 0xf)
	{
		return FALSE;
	}
	if (dwSideFlags[2] == 0 && dwSideFlags[3] == 0)
	{
		return FALSE;
	}
	if (dwSideFlags[2] == 0xf && dwSideFlags[3] == 0xf)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL sAddVisibleRoomToList(
	DWORD * FloodMarker,
	ROOM ** ppVisibleRoomBuffer,
	int * pnConnectionToRoom,
	int * pnConnectionFromRoom,
	int connection_to_room_index,
	int connection_from_room_index,
	ROOM * center_room,
	ROOM * connected_room)
{
	if (!connected_room)
	{
		return FALSE;
	}

	int connected_index = RoomGetIndexInLevel(connected_room);
	if (TESTBIT(FloodMarker, connected_index))
	{
		return FALSE;
	}

	ASSERT_RETFALSE(center_room->nVisibleRoomCount < MAX_VISIBLE_ROOMS);
	SETBIT(FloodMarker, connected_index);
	ppVisibleRoomBuffer[center_room->nVisibleRoomCount] = connected_room;
	pnConnectionToRoom[center_room->nVisibleRoomCount] = connection_to_room_index;
	pnConnectionFromRoom[center_room->nVisibleRoomCount] = connection_from_room_index;
	ASSERT(pnConnectionFromRoom[center_room->nVisibleRoomCount] != 255);
	center_room->nVisibleRoomCount++;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGCalculateVisibleRooms(
	GAME* game,
	LEVEL* level,
	ROOM* center_room)
{
	ASSERT_RETURN(game && center_room);
	ASSERT_RETURN(level);

	DWORD FloodMarker[DWORD_FLAG_SIZE(MAX_ROOMS_PER_LEVEL)];
	memclear(FloodMarker, DWORD_FLAG_SIZE(MAX_ROOMS_PER_LEVEL) * sizeof(DWORD));
	SETBIT(FloodMarker, RoomGetIndexInLevel(center_room));

	ROOM * ppVisibleRoomBuffer[MAX_VISIBLE_ROOMS];
	int pnConnectionToRoom[MAX_VISIBLE_ROOMS];		// the connection from the center room towards this room
	int pnConnectionFromRoom[MAX_VISIBLE_ROOMS];	// the connection from the visible room towards this room
	center_room->nVisibleRoomCount = 0;

	// ring 1 - add rooms connected to center room
	for (int ii = 0; ii < center_room->nConnectionCount; ii++)
	{
		sAddVisibleRoomToList(FloodMarker, ppVisibleRoomBuffer, pnConnectionToRoom, pnConnectionFromRoom,
			ii, center_room->pConnections[ii].bOtherConnectionIndex, 
			center_room, center_room->pConnections[ii].pRoom);
	}

	// ring 2 - add rooms connected to ring 1 by using the precomputed visibility flags
	int nRingOneCount = center_room->nVisibleRoomCount;
	for (int nVisibleIndex = 0; nVisibleIndex < nRingOneCount; nVisibleIndex++)
	{
		DWORD dwVisibilityFlag = 1 << pnConnectionFromRoom[nVisibleIndex];

		ROOM* pVisibleRoom = ppVisibleRoomBuffer[nVisibleIndex];

		for (int ii = 0; ii < pVisibleRoom->nConnectionCount; ii++ )
		{
			ROOM* pTestRoom = pVisibleRoom->pConnections[ii].pRoom;
			if (!pTestRoom)
			{
				continue;
			}
			if (TESTBIT(FloodMarker, RoomGetIndexInLevel(pTestRoom)))
			{
				continue;
			}
			if ((pVisibleRoom->pConnections[ii].dwVisibilityFlags & dwVisibilityFlag) == 0)
			{
				continue;
			}
			sAddVisibleRoomToList(FloodMarker, ppVisibleRoomBuffer, pnConnectionToRoom, pnConnectionFromRoom,
				pnConnectionToRoom[nVisibleIndex], pVisibleRoom->pConnections[ii].bOtherConnectionIndex,
				center_room, pTestRoom);
		}
	}

	// ring 3+ - add rooms connected to the ring 2+ rooms by testing whether the three connections line up
	// The three connections are: 
	//		connection from center room to ring 2+ room, 
	//		connection from ring 2+ room towards center room
	//		connection from ring 2+ room towards new room
	// iterate to nVisibleRoomCount which will increase inside the loop
	for (int nRingTwoRoom = nRingOneCount; (unsigned int)nRingTwoRoom < center_room->nVisibleRoomCount; nRingTwoRoom++)
	{
		ROOM* pRingRoom = ppVisibleRoomBuffer[nRingTwoRoom];
		DWORD dwVisibilityFlag = 1 << pnConnectionFromRoom[nRingTwoRoom];

		ASSERT(pnConnectionToRoom[nRingTwoRoom] < center_room->nConnectionCount);
		ROOM_CONNECTION* pConnectionFirst = &center_room->pConnections[pnConnectionToRoom[nRingTwoRoom]];

		ASSERT(pnConnectionFromRoom[nRingTwoRoom] < pRingRoom->nConnectionCount);
		ROOM_CONNECTION* pConnectionSecond = &pRingRoom->pConnections[pnConnectionFromRoom[nRingTwoRoom]];

		for (int nConnection = 0; nConnection < pRingRoom->nConnectionCount; nConnection++)
		{
			// don't check back through the connection that got us here
			if (nConnection == pnConnectionFromRoom[nRingTwoRoom])
			{
				continue;
			}
			// check the visibility mask 
			if ((pRingRoom->pConnections[nConnection].dwVisibilityFlags & dwVisibilityFlag) == 0 )
			{
				continue;
			}
			ROOM_CONNECTION* pConnectionThird = &pRingRoom->pConnections[nConnection];
			ROOM* pTestRoom = pConnectionThird->pRoom;
			if (!pTestRoom)
			{
				continue;
			}
			if (TESTBIT(FloodMarker, RoomGetIndexInLevel(pTestRoom)))
			{
				continue;
			}
			if (!sTestRing3Connection(pConnectionFirst, pConnectionSecond, pConnectionThird))
			{
				continue;
			}

			sAddVisibleRoomToList(FloodMarker, ppVisibleRoomBuffer, pnConnectionToRoom, pnConnectionFromRoom,
				pnConnectionToRoom[nRingTwoRoom], pRingRoom->pConnections[nConnection].bOtherConnectionIndex,
				center_room, pTestRoom);
		}
	}

	// copy rooms into the room's list
	int nSize = center_room->nVisibleRoomCount * sizeof(ROOM *);
	center_room->ppVisibleRooms = (ROOM **)GREALLOC(game, center_room->ppVisibleRooms, nSize);
	memcpy(center_room->ppVisibleRooms, ppVisibleRoomBuffer, nSize);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MATRIX_ROUND_ACCURACY		100.0f
  inline void sRoundedMatrixMultiply( VECTOR * pOutVector, const VECTOR * pVector, const MATRIX * pMatrix )
{
	VECTOR4 vVector4;
	MatrixMultiply( & vVector4, pVector, pMatrix);
	pOutVector->fX = FLOOR( vVector4.fX * MATRIX_ROUND_ACCURACY ) / MATRIX_ROUND_ACCURACY;
	pOutVector->fY = FLOOR( vVector4.fY * MATRIX_ROUND_ACCURACY ) / MATRIX_ROUND_ACCURACY;
	pOutVector->fZ = FLOOR( vVector4.fZ * MATRIX_ROUND_ACCURACY ) / MATRIX_ROUND_ACCURACY;
}


//----------------------------------------------------------------------------
// todo: pregenerate
//----------------------------------------------------------------------------

#define AUTOMAP_VERTICAL_TOLERANCE		1.0f

static void DRLGSubdivideAddNodes(GAME* game, ROOM* room, int nAutomapID, VECTOR v1, VECTOR v2, VECTOR v3 )
{
	float cutoff = 5;
	if(
		abs(v1.x - v2.x) > cutoff || abs(v1.x-v3.x) > cutoff|| abs(v2.x-v3.x) > cutoff ||
		abs(v1.y - v2.y) > cutoff || abs(v1.y-v3.y) > cutoff|| abs(v2.y-v3.y) > cutoff
		)
	{
		DRLGSubdivideAddNodes(game, room, nAutomapID, (v1+v2)/2, (v2+v3)/2, (v1+v3)/2);
		DRLGSubdivideAddNodes(game, room, nAutomapID, v1, (v1+v2)/2, (v1+v3)/2);
		DRLGSubdivideAddNodes(game, room, nAutomapID, (v1+v2)/2, v2, (v2+v3)/2);
		DRLGSubdivideAddNodes(game, room, nAutomapID, (v1+v3)/2, (v2+v3)/2, v3);
	}
	else
	{
			AUTOMAP_NODE* node = (AUTOMAP_NODE*)GMALLOCZ(game, sizeof(AUTOMAP_NODE));
			node->v1 = v1;
			node->v2 = v2;
			node->v3 = v3;
			CLEAR_MASK(node->m_dwFlags, AUTOMAP_WALL);

#if !ISVERSION(SERVER_VERSION)
			V( e_AutomapAddTri( nAutomapID, &v1, &v2, &v3, FALSE ) );
#endif // !ISVERSION(SERVER_VERSION)


			// link
			node->m_pNext = room->pAutomap;
			room->pAutomap = node;
	}
}

void DRLGCreateAutoMapIteratePolys(
   GAME* game,
   ROOM* room,
   ROOM_CPOLY* polys,
   int nPolys
								   )
{
	int nAutomapID = INVALID_ID;

#if !ISVERSION(SERVER_VERSION)
	REGION * pRegion = e_RegionGet( RoomGetSubLevelEngineRegion( room ) );
	if ( pRegion )
		nAutomapID = pRegion->nAutomapID;

	V( e_AutomapPrealloc( nAutomapID, nPolys ) );
#endif // !ISVERSION(SERVER_VERSION)

	float flWalkableZNormal = CollisionWalkableZNormal();
	for (int ii = 0; ii < nPolys && polys; ii++, polys++)
	{
		if (polys->vNormal.fZ > -flWalkableZNormal && polys->vNormal.fZ < flWalkableZNormal)
		{
			BOOL bIsWall = FALSE;

			VECTOR v1, v2, v3;
			sRoundedMatrixMultiply(&v1, &polys->pPt1->vPosition, &room->tWorldMatrix);
			sRoundedMatrixMultiply(&v2, &polys->pPt2->vPosition, &room->tWorldMatrix);
			sRoundedMatrixMultiply(&v3, &polys->pPt3->vPosition, &room->tWorldMatrix);

			// determine "base"
			float dz12 = fabs(v1.fZ - v2.fZ);
			float dz23 = fabs(v2.fZ - v3.fZ);
			float dz13 = fabs(v1.fZ - v3.fZ);

			VECTOR m1, m2;
			if (dz12 < dz23)
			{
				if (dz13 < dz12)
				{
					if (dz12 > AUTOMAP_VERTICAL_TOLERANCE || dz23 > AUTOMAP_VERTICAL_TOLERANCE)
					{
						m1 = v1; m2 = v3; bIsWall = TRUE;
					}
				}
				else
				{
					if (dz13 > AUTOMAP_VERTICAL_TOLERANCE || dz23 > AUTOMAP_VERTICAL_TOLERANCE)
					{
						m1 = v1; m2 = v2; bIsWall = TRUE;
					}
				}
			}
			else
			{
				if (dz13 < dz23)
				{
					if (dz12 > AUTOMAP_VERTICAL_TOLERANCE || dz23 > AUTOMAP_VERTICAL_TOLERANCE)
					{
						m1 = v1; m2 = v3; bIsWall = TRUE;
					}
				}
				else
				{
					if (dz13 > AUTOMAP_VERTICAL_TOLERANCE || dz12 > AUTOMAP_VERTICAL_TOLERANCE)
					{
						m1 = v2; m2 = v3; bIsWall = TRUE;
					}
				}
			}

			if (bIsWall)
			{
				if (VectorDistanceSquared(m1, m2) < 0.5f)
				{
					continue;
				}

				// automap node
				AUTOMAP_NODE* node = (AUTOMAP_NODE*)GMALLOCZ(game, sizeof(AUTOMAP_NODE));
				node->v1 = m1;
				node->v2 = m2;
				SET_MASK(node->m_dwFlags, AUTOMAP_WALL);

#if !ISVERSION(SERVER_VERSION)
				V( e_AutomapAddTri( nAutomapID, &m1, &m2, &m1, bIsWall ) );
#endif // !ISVERSION(SERVER_VERSION)

				// link
				node->m_pNext = room->pAutomap;
				room->pAutomap = node;
			}

		}	// Mythos doesn't draw the floors at all
		else if( AppIsHellgate() && polys->vNormal.fZ > flWalkableZNormal)
		{

			VECTOR v1, v2, v3;
			sRoundedMatrixMultiply(&v1, &polys->pPt1->vPosition, &room->tWorldMatrix);
			sRoundedMatrixMultiply(&v2, &polys->pPt2->vPosition, &room->tWorldMatrix);
			sRoundedMatrixMultiply(&v3, &polys->pPt3->vPosition, &room->tWorldMatrix);
			if (VectorDistanceSquared(v1, v2) < 0.5f)
			{
				continue;
			}
			//Subdivide floor triangles~
			DRLGSubdivideAddNodes(game, room, nAutomapID, v1, v2, v3);
		}
	}
}


static void sTranslateProp( PROP_NODE * prop, ROOM * room )
{
	ASSERT( prop->pCPoints == NULL );
	ASSERT( prop->pCPolys == NULL );
	ROOM_POINT * srcpt = prop->m_pDefinition->pRoomPoints;
	ROOM_POINT * destpt = ( ROOM_POINT * )GMALLOCZ( RoomGetGame( room ), sizeof( ROOM_POINT ) * prop->m_pDefinition->nRoomPointCount );;
	prop->pCPoints = destpt;
	for ( int ii = 0; ii < prop->m_pDefinition->nRoomPointCount; ii++, srcpt++, destpt++ )
	{
		MatrixMultiply( &destpt->vPosition, &srcpt->vPosition, &prop->m_matrixPropToRoom );
		destpt->nIndex = srcpt->nIndex;
	}
	ROOM_CPOLY * srcpoly = prop->m_pDefinition->pRoomCPolys;
	ROOM_CPOLY * destpoly = ( ROOM_CPOLY * )GMALLOCZ( RoomGetGame( room ), sizeof( ROOM_CPOLY ) * prop->m_pDefinition->nRoomCPolyCount );
	prop->pCPolys = destpoly;
	for ( int i = 0; i < prop->m_pDefinition->nRoomCPolyCount; i++, srcpoly++, destpoly++ )
	{
		destpoly->pPt1 = &prop->pCPoints[srcpoly->pPt1->nIndex];
		destpoly->pPt2 = &prop->pCPoints[srcpoly->pPt2->nIndex];
		destpoly->pPt3 = &prop->pCPoints[srcpoly->pPt3->nIndex];
		if ( i == 0 )
		{
			destpoly->tBoundingBox.vMin = destpoly->pPt1->vPosition;
			destpoly->tBoundingBox.vMax = destpoly->pPt1->vPosition;
		}
		else
		{
			BoundingBoxExpandByPoint( &destpoly->tBoundingBox, &destpoly->pPt1->vPosition );
		}
		BoundingBoxExpandByPoint( &destpoly->tBoundingBox, &destpoly->pPt2->vPosition );
		BoundingBoxExpandByPoint( &destpoly->tBoundingBox, &destpoly->pPt3->vPosition );
		MatrixMultiplyNormal( &destpoly->vNormal, &srcpoly->vNormal, &prop->m_matrixPropToRoom );
	}
}

void DRLGCreateAutoMap(
	GAME* game,
	ROOM* room)
{

	ASSERT_RETURN(game && room);

	LEVEL* level = RoomGetLevel(room);
	ASSERT_RETURN(level);

	// iterate polys of collision mesh
	ROOM_CPOLY* polys = room->pDefinition->pRoomCPolys;
	ASSERT_RETURN(polys);
	DRLGCreateAutoMapIteratePolys(game, room, polys, room->pDefinition->nRoomCPolyCount);
	if(room->pProps)
	{

		for(PROP_NODE* ii = room->pProps; ii; ii = ii->m_pNext)
		{
			if ( ( ii->pCPoints == NULL ) || ( ii->pCPolys == NULL ) )
				sTranslateProp( ii, room );
			polys = ii->pCPolys;
			if(polys)
				DRLGCreateAutoMapIteratePolys(game, room, polys, ii->m_pDefinition->nRoomCPolyCount);
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define PATHMESH_ERROR_DISTANCE			0.2f

void DrlgPathingMeshAddModel(
	GAME* game,
	ROOM* room,
	ROOM_DEFINITION* prop,
	ROOM_LAYOUT_GROUP* model,
	MATRIX* matrixPropToWorld)
{
	ASSERT_RETURN(game && room);
	ASSERT_RETURN(prop && model);

	LEVEL* level = RoomGetLevel(room);
	ASSERT_RETURN(level);

	MATRIX matrixPropToRoom;
	MatrixMultiply(&matrixPropToRoom, matrixPropToWorld, &room->tWorldMatrixInverse);

	BOUNDING_BOX tPropBox;
	tPropBox = prop->tBoundingBox;
	BOUNDING_BOX tTempBox;
	MatrixMultiply(&tTempBox.vMin, &tPropBox.vMin, &matrixPropToRoom);
	MatrixMultiply(&tTempBox.vMax, &tPropBox.vMax, &matrixPropToRoom);	
	
	
/*
#ifdef _DEBUG
	if ( AppIsHellgate() )
	{
		// TRAVIS: I don't care anymore!
		if ( ! BoundingBoxCollide(&room->pDefinition->tBoundingBox, &tTempBox, PATHMESH_ERROR_DISTANCE) )
		{
			char szTemp[256];
			PStrPrintf( szTemp, 256, "layout model outside room's bounding box:\n\nRoom:  %s\nModel: %s", room->pDefinition->tHeader.pszName, model->pszName );
			ASSERTX( BoundingBoxCollide(&room->pDefinition->tBoundingBox, &tTempBox), szTemp );
		}
	}
#endif
*/
	PROP_NODE* propnode = (PROP_NODE*)GMALLOCZ(game, sizeof(PROP_NODE));
	MatrixInverse(&propnode->m_matrixWorldToProp, matrixPropToWorld);
	propnode->m_matrixPropToRoom = matrixPropToRoom;
	propnode->m_pDefinition = prop;
	propnode->pCPoints = NULL;
	propnode->pCPolys = NULL;
	propnode->m_pNext = room->pProps;
	room->pProps = propnode;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGFreePathingMesh(
	GAME * game,
	ROOM * room)
{
	if( room->pwCollisionIter )
	{
		GFREE(game, room->pwCollisionIter);
		room->pwCollisionIter = NULL;
	}

	PROP_NODE * prop = room->pProps;
	while (prop)
	{
		PROP_NODE * next = prop->m_pNext;
		if ( prop->pCPoints != NULL )
			GFREE( game, prop->pCPoints );
		if ( prop->pCPolys != NULL )
			GFREE( game, prop->pCPolys );
		GFREE(game, prop);
		prop = next;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MAX_ATTEMPTS_EVER		100
#define MAX_SUB_ROOM_LIST		1024

#define WEIGHT_INIT			100		// inital weighted value
#define WEIGHT_DELTA		50		// how much to change weight per substitution

static void sPlaceTemplate( 
	GAME * pGame, 
	const DRLG_DEFINITION * drlg_definition, 
	DRLG_LEVEL_DEFINITION * pDefinition, 
	DRLG_LEVEL &tDRLGLevel, 
	VECTOR vOffset,
	SUBLEVELID idSubLevel)
{
	ASSERT_RETURN( pDefinition );
	ASSERT( pDefinition->nTemplateCount );
	// WE allow random starting templates. Why doesn't hellgate? Hmmm?
	int nTemplateChoice = RandGetNum(pGame->randLevelSeed, pDefinition->nTemplateCount);
	ASSERT( nTemplateChoice < pDefinition->nTemplateCount);
	DRLG_LEVEL_TEMPLATE * pTemplate = & pDefinition->pTemplates[ nTemplateChoice ];
	tDRLGLevel.nRoomCount += pTemplate->nRoomCount;
	ASSERT( pTemplate->pRooms );
	for (int nRoom = 0; nRoom < pTemplate->nRoomCount; nRoom++ )
	{
		int nRoomId;
		DRLG_ROOM * pRoom = tDRLGLevel.pRooms.Add( &nRoomId );
		memcpy ( pRoom, & pTemplate->pRooms[ nRoom ], sizeof( DRLG_ROOM ) );
		pRoom->vLocation += vOffset;

		// assign sublevel id
		pRoom->idSubLevel = idSubLevel;
		
		// add hull
		char szPath    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		char szName    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		// does this definition have a specific layout?
		const char * pBracket = PStrRChr( pRoom->pszFileName, '[' );
		if ( pBracket )
		{
			// remove from string for now...
			PStrCopy(szName, pRoom->pszFileName, (int)(pBracket - pRoom->pszFileName + 1));
		}
		else
		{
			PStrCopy( szName, pRoom->pszFileName, DEFAULT_FILE_WITH_PATH_SIZE );
		}
		// is this definition in another folder?
		OVERRIDE_PATH tOverridePath;
		const char * pDot = PStrRChr( szName, '.' );
		if ( pDot )
		{
			OverridePathByCode( *( (DWORD*)( pDot + 1 ) ), &tOverridePath );
			PStrCopy(szName, szName, (int)(pDot - szName + 1));
		}
		else
		{
			OverridePathByLine( drlg_definition->nPathIndex, &tOverridePath );
		}
		ASSERT( tOverridePath.szPath[ 0 ] );
		// must remove data\\backgrounds
		PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, tOverridePath.szPath );		
		char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrCopy( szFilePath, tOverridePath.szPath, DEFAULT_FILE_WITH_PATH_SIZE );
		PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, szName );
		//int nId = ExcelGetLineByStringIndex( pGame, DATATABLE_ROOM_INDEX, szFilePath );
		int nId = RoomFileGetRoomIndexLine( szFilePath );
		ASSERTV( nId != INVALID_ID, "Couldn't find room in room index spreadsheet!\n%s", szFilePath );
		ROOM_DEFINITION * pRoomDef = RoomDefinitionGet( pGame, nId, IS_CLIENT(pGame) );
		ASSERT_RETURN(pRoomDef);
		if ( pRoomDef->nHullVertexCount )
		{
			ASSERT( pRoomDef->nHullVertexCount < MAX_HULL_VERTICES );
			VECTOR vHullVerts[ MAX_HULL_VERTICES ];
			for ( int v = 0; v < pRoomDef->nHullVertexCount; v++ )
			{
				vHullVerts[ v ] = pRoomDef->pHullVertices[ v ];
				VectorZRotation( vHullVerts[ v ], pRoom->fRotation );
				vHullVerts[ v ] += pRoom->vLocation;
			}
			pRoom->pHull = HullCreate(  vHullVerts,  pRoomDef->nHullVertexCount,
				pRoomDef->pHullTriangles, pRoomDef->nHullTriangleCount, pRoomDef->tHeader.pszName );
			//trace( "CHC2 - Hull created at address: %p\n", pRoom->pHull );
		}
		else
		{
			pRoom->pHull = NULL;
			// add bounding box
			pRoom->tBoundingBox = pRoomDef->tBoundingBox;
			BoundingBoxZRotation( &pRoom->tBoundingBox, pRoom->fRotation );
			pRoom->tBoundingBox.vMin += pRoom->vLocation;
			pRoom->tBoundingBox.vMax += pRoom->vLocation;
		}
		//trace( "Adding room %i, with hull address: %p\n", nRoomId, pRoom->pHull );
	}
}

inline int DRLGGetMinSub( const LEVEL *pLevel,
						  const DRLG_PASS *drlg )
{
	if( AppIsHellgate() )
	{
		GAME * pGame = LevelGetGame( pLevel );
		ASSERTX_RETVAL( pGame, drlg->nMinSubs[0], "DRLG needs Level to have a Game" );
		ASSERTX_RETVAL( ( pGame->nDifficulty >= DIFFICULTY_NORMAL ) && ( pGame->nDifficulty <= DIFFICULTY_NIGHTMARE ), drlg->nMinSubs[0], "DRLG can only handle normal and nightmare difficulty" );
		return drlg->nMinSubs[ pGame->nDifficulty ];
	}
	return MYTHOS_LEVELAREAS::LevelAreaGetModifiedDRLGSize( pLevel, drlg, TRUE );
}

inline int DRLGGetMaxSub( const LEVEL *pLevel,
						  const DRLG_PASS *drlg )
{
	if( AppIsHellgate() )
	{
		GAME * pGame = LevelGetGame( pLevel );
		ASSERTX_RETVAL( pGame, drlg->nMaxSubs[0], "DRLG needs Level to have a Game" );
		ASSERTX_RETVAL( ( pGame->nDifficulty >= DIFFICULTY_NORMAL ) && ( pGame->nDifficulty <= DIFFICULTY_NIGHTMARE ), drlg->nMaxSubs[0], "DRLG can only handle normal and nightmare difficulty" );
		return drlg->nMaxSubs[ pGame->nDifficulty ];
	}
	return MYTHOS_LEVELAREAS::LevelAreaGetModifiedDRLGSize( pLevel, drlg, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoPass( 
	GAME * pGame, 	
	const LEVEL *pLevel,
	const DRLG_DEFINITION * 
	drlg_definition, 
	DRLG_PASS *pPass, 
	DRLG_LEVEL & tDRLGLevel, 
	BOOL bFirst,
	SUBLEVELID idSubLevel )
{
	// grab a template
	DRLG_LEVEL_DEFINITION* pDefinition = DrlgGetLevelRuleDefinition(pPass);
	ASSERT_RETFALSE(pDefinition);

	// use a template to fill the level
	if ( bFirst )
	{
		tDRLGLevel.nRoomCount = 0;
		VECTOR vZero( 0.0f, 0.0f, 0.0f );
		sPlaceTemplate( pGame, drlg_definition, pDefinition, tDRLGLevel, vZero, idSubLevel );
		return TRUE;
	}

	// Apply substitutions
	if ( pDefinition->nSubstitutionCount )
	{
		int nSuccessfulSubstitutions = 0;
		int nSubAttempt = 0;

		// init weighted replacemens
		int nTotalWeight = 0;
		int nWeightedSubs[ MAX_REPLACEMENTS_PER_RULE ];
		if ( pPass->bWeighted )
		{
			// Must only have 1 rule to use weighted
			ASSERT( pDefinition->nSubstitutionCount == 1 );	
			DRLG_SUBSTITUTION_RULE * pSub = & pDefinition->pSubsitutionRules[ 0 ];
			// Must have more than 1 replacement
			ASSERT( pSub->nReplacementCount > 1 );
			nTotalWeight = pSub->nReplacementCount * WEIGHT_INIT;
			for ( int w = 0; w < pSub->nReplacementCount; w++ )
				nWeightedSubs[w] = WEIGHT_INIT;
		}

		// only used if nMinSubs != 0
		int nNumSubsToDo = RandGetNum(pGame->randLevelSeed, DRLGGetMaxSub( pLevel, pPass ) - DRLGGetMinSub( pLevel, pPass ) + 1 ) + DRLGGetMinSub( pLevel, pPass );

		BOOL bDone = FALSE;
		int nRuleOkList[ MAX_RULES_PER_FILE ];
		int nNumRulesOk = pDefinition->nSubstitutionCount;
		for ( int a = 0; a < nNumRulesOk; a++ )
			nRuleOkList[a] = a;

		while ( !bDone )
		{
			if ( !DRLGGetMinSub( pLevel, pPass ) && pPass->nAttempts )
			{
				if ( nSubAttempt >= pPass->nAttempts )
					bDone = TRUE;
			}
			else
			{
				if ( nSuccessfulSubstitutions >= nNumSubsToDo )
					bDone = TRUE;
				if ( nSubAttempt >= MAX_ATTEMPTS_EVER )
				{
					if ( ( pPass->bMustPlace ) && ( nSuccessfulSubstitutions < DRLGGetMinSub( pLevel, pPass ) ) )
						return FALSE;
					bDone = TRUE;
				}
			}

			if ( bDone )
				continue;

			nSubAttempt++;

			int pnRoomsToReplace[ MAX_ROOMS_PER_GROUP ];

			// pick substitution
			if ( !nNumRulesOk )
			{
				nSubAttempt = MAX_ATTEMPTS_EVER;
				continue;
			}

			int nRuleChoice = RandGetNum(pGame->randLevelSeed, nNumRulesOk);
			int nSub = nRuleOkList[ nRuleChoice ];
			DRLG_SUBSTITUTION_RULE * pSub = & pDefinition->pSubsitutionRules[ nSub ];

			// make a list of all rooms which match the rule
			int nRoomSubOkList[ MAX_SUB_ROOM_LIST ];
			int nNumSubsOk = 0;
			for ( int i = tDRLGLevel.pRooms.GetFirst(); i != INVALID_ID; i = tDRLGLevel.pRooms.GetNextId( i ) )
			{
				DRLG_ROOM * pCornerstoneToCheck = tDRLGLevel.pRooms.Get(i);
				if ( PStrCmp( pCornerstoneToCheck->pszFileName, pSub->pRuleRooms[0].pszFileName ) == 0 ) // replace with Id comparison later
				{
					BOOL bFoundRoomsToReplace = TRUE;
					if ( pSub->nRuleRoomCount > 1 )
					{
						for ( int nRuleRoom = 1; bFoundRoomsToReplace && ( nRuleRoom < pSub->nRuleRoomCount ); nRuleRoom ++ )
						{
							int nTemp;
							VECTOR vDesiredOffset = pSub->pRuleRooms[ nRuleRoom ].vLocation;
							VectorZRotation( vDesiredOffset, pCornerstoneToCheck->fRotation );
							vDesiredOffset += pCornerstoneToCheck->vLocation;
							if ( ! sFindRoomToReplace( &tDRLGLevel, 
								&pSub->pRuleRooms[ nRuleRoom ], 
								&nTemp, 
								tDRLGLevel.pRooms.GetFirst(), 
								&vDesiredOffset,
								pSub->pRuleRooms[ nRuleRoom ].fRotation + pCornerstoneToCheck->fRotation ) )
							{
								bFoundRoomsToReplace = FALSE;
								break;
							}
						}
					}

					if ( bFoundRoomsToReplace )
					{
						nRoomSubOkList[ nNumSubsOk++ ] = i;
					}
				}
			}

			if ( nNumSubsOk )
			{
				int nSubNum = RandGetNum(pGame->randLevelSeed, nNumSubsOk );
				*pnRoomsToReplace = nRoomSubOkList[ nSubNum ];
				BOOL bSubOk = TRUE;
				if ( pSub->nRuleRoomCount > 1 )
				{
					DRLG_ROOM * pRoomToReplace = tDRLGLevel.pRooms.Get( pnRoomsToReplace[ 0 ] );
					for ( int nRuleRoom = 1; nRuleRoom < pSub->nRuleRoomCount; nRuleRoom ++ )
					{
						VECTOR vDesiredOffset = pSub->pRuleRooms[ nRuleRoom ].vLocation;
						VectorZRotation( vDesiredOffset, pRoomToReplace->fRotation );
						vDesiredOffset += pRoomToReplace->vLocation;
						bSubOk &= sFindRoomToReplace( &tDRLGLevel, &pSub->pRuleRooms[ nRuleRoom ], &pnRoomsToReplace[ nRuleRoom ], 
							tDRLGLevel.pRooms.GetFirst(), &vDesiredOffset, pSub->pRuleRooms[ nRuleRoom ].fRotation + pRoomToReplace->fRotation );
					}
				}
				ASSERT( bSubOk );

				// pick a replacement
				int nReplacementIndex;
				if ( pPass->bWeighted )
				{
					int nRndVal = RandGetNum( pGame->randLevelSeed, nTotalWeight );
					nReplacementIndex = -1;
					do
					{
						nReplacementIndex++;
						nRndVal -= nWeightedSubs[ nReplacementIndex ];
					}
					while ( nRndVal > 0 );
					ASSERT( nReplacementIndex < pSub->nReplacementCount );
				}
				else
				{
					nReplacementIndex = RandGetNum(pGame->randLevelSeed, pSub->nReplacementCount);
				}

				if ( sAttemptToReplace(pGame, &tDRLGLevel, drlg_definition, pnRoomsToReplace, pSub, nReplacementIndex, FALSE ) )
				{
					nSuccessfulSubstitutions++;
					nSubAttempt = 0;	// reset count
					// reinit list because the change may reactivate a rule
					nNumRulesOk = pDefinition->nSubstitutionCount;
					for ( int a = 0; a < nNumRulesOk; a++ )
						nRuleOkList[a] = a;
					// update weights
					if ( pPass->bWeighted )
					{
						for ( int w = 0; w < pSub->nReplacementCount; w++ )
						{
							if ( w == nReplacementIndex )
							{
								if ( nWeightedSubs[w] >= WEIGHT_DELTA )
								{
									nWeightedSubs[w] -= WEIGHT_DELTA;
									nTotalWeight -= WEIGHT_DELTA;
								}
							}
							else
							{
								nWeightedSubs[w] += WEIGHT_DELTA;
								nTotalWeight += WEIGHT_DELTA;
							}
						}
					}
				}
				else
				{
					char szTemp[256];
					PStrPrintf( szTemp, 256, "** DRLG rule failed, replace didn't fit. Pass = %s, Rule = %i at Id = %i\n", pPass->pszDrlgFileName, nSub+1, *pnRoomsToReplace );
					OutputDebugString( szTemp );
				}
			}
			else
			{
				// no subs found.. remove from lookyloo list
				nNumRulesOk--;
				nRuleOkList[ nRuleChoice ] = nRuleOkList[ nNumRulesOk ];

/*				char szTemp[256];
				sprintf( szTemp, "-- DRLG rule failed, no locations okay. Pass = %s, Rule = %i\n", pPass->pszDrlgFileName, nSub+1 );
				OutputDebugString( szTemp );*/
			}
		}
	}
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sDoReplacePass( GAME * pGame, const DRLG_DEFINITION * drlg_definition, DRLG_PASS *pPass, DRLG_LEVEL & tDRLGLevel )
{
	// grab a template
	DRLG_LEVEL_DEFINITION* pDefinition = DrlgGetLevelRuleDefinition(pPass);
	ASSERT(pDefinition);

	// Apply substitutions
	for ( int nRule = 0; nRule < pDefinition->nSubstitutionCount; nRule++ )
	{
		DRLG_SUBSTITUTION_RULE * pSub = & pDefinition->pSubsitutionRules[ nRule ];

		// right now, only coded to work with single room replacements
		ASSERT_CONTINUE( pSub->nRuleRoomCount == 1 );
		// go through dungeon and replace all
		for ( int i = tDRLGLevel.pRooms.GetFirst(); i != INVALID_ID; i = tDRLGLevel.pRooms.GetNextId( i ) )
		{
			DRLG_ROOM * pRoom = tDRLGLevel.pRooms.Get(i);
			if ( PStrCmp( pRoom->pszFileName, pSub->pRuleRooms[0].pszFileName ) == 0 ) // replace with Id comparison later
			{
				// pick a replacement
				int nIndex = RandGetNum(pGame->randLevelSeed, pSub->nReplacementCount);
				DRLG_ROOM * pReplacementRoom = pSub->ppReplacementRooms[nIndex];
				if ( PStrCmp( pRoom->pszFileName, pReplacementRoom->pszFileName ) != 0 )
				{
					BOOL bReplaceOk = sAttemptToReplace( pGame, &tDRLGLevel, drlg_definition, &i, pSub, nIndex, TRUE );
					REF(bReplaceOk);
				}
				else
				{
					char szTemp[256];
					wsprintf( szTemp, "Replacing same room: %s\n", pReplacementRoom->pszFileName );
					OutputDebugString( szTemp );
				}
			}
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#define MAX_TOTAL_PASSES					50
#define MAX_INNER_LOOP_ATTEMPTS				10000

static BOOL sDoReplacePassWithCollision( GAME * pGame, const DRLG_DEFINITION * drlg_definition, DRLG_PASS *pPass, DRLG_LEVEL & tDRLGLevel, BOOL bOnePass )
{
	// grab a template
	DRLG_LEVEL_DEFINITION* pDefinition = DrlgGetLevelRuleDefinition(pPass);
	ASSERT_RETFALSE(pDefinition);

	// Apply substitutions
	int nNumSubs = -1;
	int nTotalPasses = 0;
	while ( nNumSubs != 0 )
	{
		nTotalPasses++;
		if ( nTotalPasses > MAX_TOTAL_PASSES )
			return FALSE;

		nNumSubs = 0;
		for ( int nRule = 0; nRule < pDefinition->nSubstitutionCount; nRule++ )
		{
			DRLG_SUBSTITUTION_RULE * pSub = & pDefinition->pSubsitutionRules[ nRule ];

			int pnRoomsToReplace[ MAX_ROOMS_PER_GROUP ];

			int nInnerLoop = 0;

			// go through dungeon and replace all
			for ( int i = tDRLGLevel.pRooms.GetFirst(); i != INVALID_ID;  )
			{
				DRLG_ROOM * pRoom = tDRLGLevel.pRooms.Get(i);
				BOOL bNext = TRUE;
				if ( PStrCmp( pRoom->pszFileName, pSub->pRuleRooms[0].pszFileName ) == 0 ) // replace with Id comparison later
				{
					*pnRoomsToReplace = i;
					BOOL bFoundRoomsToReplace = TRUE;
					for ( int nRuleRoom = 1; bFoundRoomsToReplace && ( nRuleRoom < pSub->nRuleRoomCount ); nRuleRoom ++ )
					{
						VECTOR vDesiredOffset = pSub->pRuleRooms[ nRuleRoom ].vLocation;
						VectorZRotation( vDesiredOffset, pRoom->fRotation );
						vDesiredOffset += pRoom->vLocation;
						if ( ! sFindRoomToReplace( &tDRLGLevel, 
							&pSub->pRuleRooms[ nRuleRoom ], 
							&pnRoomsToReplace[ nRuleRoom ],
							tDRLGLevel.pRooms.GetFirst(), 
							&vDesiredOffset,
							pSub->pRuleRooms[ nRuleRoom ].fRotation + pRoom->fRotation ) )
						{
							bFoundRoomsToReplace = FALSE;
						}
					}

					if ( bFoundRoomsToReplace )
					{
						int nIndex = RandGetNum(pGame->randLevelSeed, pSub->nReplacementCount);
						BOOL bResult = sAttemptToReplace( pGame, &tDRLGLevel, drlg_definition, pnRoomsToReplace, pSub, nIndex, FALSE );
						if ( bResult && !bOnePass )
						{
							nNumSubs++;
						}
						bNext = !bResult;
					}
				}
				if ( bNext || bOnePass )
					i = tDRLGLevel.pRooms.GetNextId( i );
				else
					i = tDRLGLevel.pRooms.GetFirst();
				nInnerLoop++;
				ASSERT_RETFALSE( nInnerLoop < MAX_INNER_LOOP_ATTEMPTS );
			}
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
enum DRLG_RESULT
{
	DRLG_FAILED,			// generic failed message
	DRLG_FAILED_NO_RULES,	// no rules found for level
	DRLG_OK,
	
	NUM_DRLG_RESULT		// keep this last
};

//----------------------------------------------------------------------------
struct DRLG_RESULT_LOOKUP
{
	DRLG_RESULT eResult;
	const char* pszString;
};
static DRLG_RESULT_LOOKUP sgtDRLGResultLookupTable[ NUM_DRLG_RESULT ] = 
{
	{ DRLG_FAILED,				"GENERIC FAILURE" },
	{ DRLG_FAILED_NO_RULES,		"NO RULES FOUND FOR LEVEL" },
	{ DRLG_OK,					"OK" }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char* sDRLGFailureString(
	DRLG_RESULT eResult)
{
	for (int i = 0; i < NUM_DRLG_RESULT; ++i)
	{
		const DRLG_RESULT_LOOKUP* pLookup = &sgtDRLGResultLookupTable[ i ];
		if (pLookup->eResult == eResult)
		{
			return pLookup->pszString;
		}
	}
	ASSERTX_RETNULL( 0, "Unknown DRLG result code" );
}

//----------------------------------------------------------------------------
// Assumes all strings of length DEFAULT_FILE_WITH_PATH_SIZE
//----------------------------------------------------------------------------
void DRLGRoomGetGetLayoutOverride(
	const char * pszFileName,
	char * szName,
	char * szLayout)
{
	// does this definition have a specific layout?
	const char * pBracket = PStrRChr( pszFileName, '[' );
	if ( pBracket )
	{
		// remove from string for now...
		PStrCopy(szName, pszFileName, (int)(pBracket - pszFileName + 1));
		// copy the name
		const char * pEnd = PStrRChr( pszFileName, ']' );
		ASSERT( *pEnd == ']' );
		ASSERT_RETURN( (pEnd - 1) > (pBracket + 1) );
		PStrCopy( szLayout, "_", DEFAULT_FILE_WITH_PATH_SIZE );
		PStrNCat(szLayout, DEFAULT_FILE_WITH_PATH_SIZE, pBracket + 1, (int)((pEnd - 1) - (pBracket + 1) + 1));
	}
	else
	{
		PStrCopy( szName, pszFileName, DEFAULT_FILE_WITH_PATH_SIZE );
	}
}

//----------------------------------------------------------------------------
// Assumes all strings of length DEFAULT_FILE_WITH_PATH_SIZE
//----------------------------------------------------------------------------
const char * DRLGRoomGetGetPath(
	char * szName,
	char * szFilePath,
	int nPathIndex)
{
	OVERRIDE_PATH tOverridePath;
	// is this definition in another folder?
	const char * pDot = PStrRChr( szName, '.' );
	if ( pDot )
	{
		OverridePathByCode( *( (DWORD*)( pDot + 1 ) ), &tOverridePath );
		PStrCopy(szName, DEFAULT_FILE_WITH_PATH_SIZE, szName, (int)(pDot - szName + 1));
	}
	else
	{
		OverridePathByLine( nPathIndex, &tOverridePath );
	}
	PStrCopy( szFilePath, tOverridePath.szPath, DEFAULT_FILE_WITH_PATH_SIZE );
	return pDot;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPopulateSubLevel(
	LEVEL *pLevel,
	DRLG_LEVEL &tDRLGLevel,
	SUBLEVELID idSubLevel)
{		
	GAME *pGame = LevelGetGame( pLevel );

	ASSERT_RETFALSE(idSubLevel >= 0);

	// get DRLG params for sub level
	int nDRLGDef = SubLevelGetDRLG( pLevel, idSubLevel );
	ASSERT_RETFALSE(nDRLGDef >= 0);
	const DRLG_DEFINITION * ptDRLGDef = DRLGDefinitionGet( nDRLGDef );
	ASSERT_RETFALSE(ptDRLGDef);
	VECTOR vOffset = SubLevelGetPosition( pLevel, idSubLevel );

	// run rules to create sub level
	BOOL bSuccess = FALSE;	
	int nNumRules = ExcelGetNumRows( NULL, DATATABLE_LEVEL_RULES );
	for ( int i = 0; i < nNumRules; i++ )
	{
		DRLG_PASS * pPass = (DRLG_PASS *)ExcelGetData( NULL, DATATABLE_LEVEL_RULES, i );
		if ( ! pPass )
			continue;
		//ASSERTV_CONTINUE( pPass, "Missing pass %d", i );  // this can happen in beta, demo, etc...

		if (PStrCmp( pPass->pszDRLGName, ptDRLGDef->pszDRLGRuleset ) == 0 &&
			pPass->nPathIndex == ptDRLGDef->nPathIndex )
		{
			DRLG_LEVEL_DEFINITION * pHellriftDefinition = DrlgGetLevelRuleDefinition(pPass);
			
			// place template
			sPlaceTemplate( 
				pGame, 
				ptDRLGDef, 
				pHellriftDefinition, 
				tDRLGLevel, 
				vOffset,
				idSubLevel);

			// mark that the level has a hellrift now, we'll create entrance portals for it soon
			bSuccess = TRUE;
			
		}
		
	}			
	
	ASSERTX( bSuccess, "Unable to place sublevel DRLG" );
	return bSuccess;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDRLGDefinitionHasNoThemes(
	const DRLG_DEFINITION * pDRLGDefinition)
{
	for(int i=0; i<MAX_DRLG_THEMES; i++)
	{
		if(pDRLGDefinition->nThemes[i] >= 0)
		{
			return FALSE;
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDRLGDestroy( DRLG_LEVEL & tDRLGLevel )
{
	int nRoomId = tDRLGLevel.pRooms.GetFirst();
	for ( int i = 0; i < tDRLGLevel.nRoomCount; i++ )
	{
		DRLG_ROOM * pRoom = tDRLGLevel.pRooms.Get( nRoomId );
		if ( pRoom && pRoom->pHull )
		{
			//trace( "CHD3 - Hull destroyed at: %p\n", pRoom->pHull );
			ConvexHullDestroy( pRoom->pHull );
		}
		nRoomId = tDRLGLevel.pRooms.GetNextId( nRoomId );
	}
	tDRLGLevel.pRooms.Destroy();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DRLG_RESULT sDRLGCreateLevel(
	GAME* game,
	LEVEL* level,
	UNITID idActivator)
{
	const DRLG_DEFINITION * drlg_definition = LevelGetDRLGDefinition( level );
	ASSERT_RETVAL(drlg_definition, DRLG_FAILED);

	// reset any existing sublevels (such as from previous DRLG passes that failed)
	SubLevelClearAll( level );
	
	// create the default sublevel for everthing in the level
	int nSubLevelDef = LevelGetDefaultSubLevelDefinition( level );
	SUBLEVELID idSubLevel = SubLevelAdd( level , nSubLevelDef );
	
	// this is a temporary level used as scratch space
	DRLG_LEVEL tDRLGLevel;
	ArrayInit(tDRLGLevel.pRooms, GameGetMemPool(game), 0);

#if ISVERSION(RELEASE_CHEATS_ENABLED)|| defined(_DEBUG)
	const GAME_GLOBAL_DEFINITION* game_global_definition = DefinitionGetGameGlobal();
	ASSERT_RETVAL(game_global_definition, DRLG_FAILED);
	if ( game_global_definition->nRoomOverride != INVALID_ID )
	{
		MATRIX mRoomMatrix;
		MatrixIdentity( &mRoomMatrix );
		DWORD dwRoomSeed = RandGetNum(game->randLevelSeed);
		RoomAdd(game, level, game_global_definition->nRoomOverride, &mRoomMatrix, FALSE, dwRoomSeed, INVALID_ID, idSubLevel, NULL);
		return DRLG_OK;
	}
#endif

/*	tDRLGLevel.nRoomCount = 0;
	DRLG_PASS * pPass = (DRLG_PASS *)ExcelGetData( game, DATATABLE_LEVEL_RULES, 37 );
	VECTOR vOffset(0.0f,-80.0f,0.0f);
	sShowRule( tDRLGLevel, drlg_definition, pPass, 0, &vOffset, 0.0f );*/

	// lay the rooms out
	int nNumRules = ExcelGetNumRows( game, DATATABLE_LEVEL_RULES );
	BOOL bFirst = TRUE;
	int nLoopCount = 0;
	BOOL bLooping = FALSE;
	BOOL bFoundRule = FALSE;
	for ( int i = 0; i < nNumRules; i++ )
	{
		DRLG_PASS * pPass = (DRLG_PASS *)ExcelGetData( game, DATATABLE_LEVEL_RULES, i );
		//ASSERTV_CONTINUE( pPass, "Missing pass %d", i );  // these can be NULL when we mark things for demo or beta
		if ( ! pPass )
			continue;

		if ( ( PStrCmp( pPass->pszDRLGName, drlg_definition->pszDRLGRuleset ) == 0 ) &&
			 ( pPass->nPathIndex == drlg_definition->nPathIndex ) )
		{
			bFoundRule = TRUE;
			BOOL bLineChange = FALSE;
			if ( pPass->nLoopAmount && !bLooping )
			{
				bLooping = TRUE;
				nLoopCount = pPass->nLoopAmount;
			}
			if ( pPass->bReplaceWithCollisionCheck )
			{
				if ( sDoReplacePassWithCollision( game, drlg_definition, pPass, tDRLGLevel, pPass->bReplaceAndCheckOnce ) == FALSE )
				{
					sDRLGDestroy( tDRLGLevel );
					return DRLG_FAILED;
				}
			}
			else if ( !pPass->bReplaceAll )
			{
				BOOL bDoPass = TRUE;
				if ( pPass->nQuestPercent && ( (int)RandGetNum(game->randLevelSeed, 100) < pPass->nQuestPercent ) )
				{
					ASSERT( !bFirst );		// can't optional the template
					bDoPass = FALSE;
				}
				BOOL bMustPlace = pPass->bMustPlace;
				if ( pPass->bAskQuests )
				{
					bDoPass = FALSE;
					bMustPlace = FALSE;
					if ( idActivator != INVALID_ID )
					{
						UNIT * pPlayer = UnitGetById( game, idActivator );
						if ( pPlayer )
						{
							bDoPass = s_QuestCanRunDRLGRule( pPlayer, level, pPass->pszDrlgFileName );
							if ( bDoPass )
								bMustPlace = TRUE;
						}
					}
				}
				if( bDoPass && pPass->nQuestTaskComplete != INVALID_ID )
				{
					UNIT * pPlayer = UnitGetById( game, idActivator );
					bDoPass = ( pPlayer && QuestIsQuestTaskComplete( pPlayer, pPass->nQuestTaskComplete ) );
				}

				if ( bDoPass && pPass->nThemeRunRule > 0 )
				{
					if ( sDRLGDefinitionHasNoThemes(drlg_definition) )
						bDoPass = FALSE;
					else if ( !DRLGThemeIsA( drlg_definition, pPass->nThemeRunRule ) )
						bDoPass = FALSE;

				}
				if ( bDoPass && pPass->nThemeSkipRule > 0 )
				{
					if ( DRLGThemeIsA( drlg_definition, pPass->nThemeSkipRule ) )
						bDoPass = FALSE;
				}
				if ( bDoPass && pPass->bExitRule )
				{
					if ( level )
					{
						const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( level );
						bDoPass = ( pLevelDef && pLevelDef->nNextLevel != INVALID_LINK );
						if( AppIsTugboat() )
						{
							
							bDoPass = !level->m_LevelAreaOverrides.bIsLastLevel;
						}
					}
				}
				if ( bDoPass && pPass->bDeadEndRule )
				{
					if ( level )
					{
						const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( level );
						bDoPass = ( pLevelDef && pLevelDef->nNextLevel == INVALID_LINK );
					}
					else
					{
						bDoPass = FALSE;
					}
				}
				BOOL bOk = TRUE;
				if ( bDoPass )
				{
					bOk = sDoPass(game, level, drlg_definition, pPass, tDRLGLevel, bFirst, idSubLevel);
					bFirst = FALSE;
				}
				if ( !bOk && bMustPlace )
				{
					sDRLGDestroy( tDRLGLevel );
					return DRLG_FAILED;
				}
			}
			else
			{
				sDoReplacePass( game, drlg_definition, pPass, tDRLGLevel );
			}
			if ( bLooping && pPass->pszLoopLabel[0] && !bLineChange )
			{
				nLoopCount--;
				if ( nLoopCount )
					i = LevelRuleGetLine( pPass->pszLoopLabel ) - 1;		// going to be adding 1 to i in a few lines...
				else
					bLooping = FALSE;
			}
		}
	}

	if (bFoundRule == FALSE)
	{
		return DRLG_FAILED_NO_RULES;
	}
	
/*
#ifdef HAMMER
	if ( PStrCmp( drlg_definition->pszName, "CoventGarden2" ) == 0 )
	{
		sDRLGDebugGDIWnd( &tDRLGLevel, sCityDebugDraw3 );
	}
#endif
*/

	// place town portal sub level
	int nSubLevelDefTownPortal = level->m_pLevelDefinition->nSubLevelDefTownPortal;
	if (nSubLevelDefTownPortal != INVALID_LINK)
	{
		SUBLEVELID idSubLevelTownPortal = SubLevelAdd( level, nSubLevelDefTownPortal );
		sPopulateSubLevel( level, tDRLGLevel, idSubLevelTownPortal );
	}
		
	// place special hell-rift sub level
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( level );
	if (ptLevelDef && ptLevelDef->nHellriftChance > 0)
	{
		BOOL bSpawnedHellrift = FALSE;
		if ((int)RandGetNum( game, 0, 100 ) <= ptLevelDef->nHellriftChance)
		{
			PickerStart( game, picker );	
			for (int i = 0; i < MAX_HELLRIFT_SUBLEVEL_CHOICES; ++i)
			{
				if (ptLevelDef->nSubLevelHellrift[i] != INVALID_LINK)
					PickerAdd(MEMORY_FUNC_FILELINE(game, i, 1));
			}
			int nPick = PickerChooseRemove( game );
			ASSERTX( nPick >= 0, "Invalid hellrift sublevel picked" );
			int nHellriftSublevelDef = (nPick >= 0) ? ptLevelDef->nSubLevelHellrift[nPick] : INVALID_LINK;
			SUBLEVELID idSubLevel = SubLevelAdd( level, nHellriftSublevelDef );
			ASSERT(idSubLevel >= 0);
			if(idSubLevel >= 0)
			{
				bSpawnedHellrift = sPopulateSubLevel( level, tDRLGLevel, idSubLevel );
			}
		}
		ASSERTV( 
			bSpawnedHellrift || ptLevelDef->nHellriftChance < 100, 
			"Hellrift was not spawned on level '%s' with chance '%d'", 
			LevelGetDevName( level ),
			ptLevelDef->nHellriftChance);
	}

	// do all other sub-levels
	for (int i = 0; i < MAX_SUBLEVELS; ++i)
	{
		int nSubLevelDef = ptLevelDef->nSubLevel[ i ];
		if (nSubLevelDef != INVALID_LINK)
		{
			SUBLEVELID idSubLevel = SubLevelAdd( level, nSubLevelDef );
			sPopulateSubLevel( level, tDRLGLevel, idSubLevel );
		}
	}
	// this is for rotating entire levels. Not used for Hellgate at all.
	float fRotation = AppIsHellgate() ? 0 : 
		( ptLevelDef->bAllowRandomOrientation ? DegreesToRadians( RandGetNum(game->randLevelSeed, 0, 360 ) ) : 0 );

	if( AppIsTugboat() && ptLevelDef->bOrientedToMap )
	{		
		int Angle = MYTHOS_LEVELAREAS::LevelAreaGetAngleToHub( LevelGetLevelAreaID( level ) );

 		Angle += 45;
		fRotation = DegreesToRadians( (float)Angle );
	}
	// create the rooms for the level
	int nRoomIndexInLevel = 0;
	for ( int nId = tDRLGLevel.pRooms.GetFirst(); nId != INVALID_ID; nId = tDRLGLevel.pRooms.GetNextId( nId ) )
	{
		DRLG_ROOM * pDrlgRoom = tDRLGLevel.pRooms.Get( nId );
		if(!pDrlgRoom)
			continue;
		char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		char szPath    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		char szName    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		char szLayout  [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
		
		// does this definition have a specific layout?
		DRLGRoomGetGetLayoutOverride(pDrlgRoom->pszFileName, szName, szLayout);
		
		// is this definition in another folder?
		const char * pDot = DRLGRoomGetGetPath(szName, szFilePath, drlg_definition->nPathIndex);
		
		ASSERT( szFilePath[ 0 ] );
		PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, szFilePath );
		PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, szName );
		MATRIX mRoomMatrix;
		MatrixIdentity( &mRoomMatrix );
		float fKludge = 0.0f;
		if ( !pDot && drlg_definition->nPathIndex == GetOverridePathLine( MAKEFOURCC('T','B','0','2') ) ) // Dave, what's up with this? - Tyler
			fKludge = PI;
		// in the interest of safety, and because Hellgate will never want to do this-
		if( AppIsHellgate() )
		{
			MatrixTransform( &mRoomMatrix, &pDrlgRoom->vLocation, pDrlgRoom->fRotation + fKludge );
		}
		else
		{
			VectorZRotation( pDrlgRoom->vLocation, fRotation );
			MatrixTransform( &mRoomMatrix, &pDrlgRoom->vLocation, pDrlgRoom->fRotation + fKludge + fRotation );
		}

		char pszFileNameNoExtension[MAX_XML_STRING_LENGTH];
		PStrRemoveExtension(pszFileNameNoExtension, MAX_XML_STRING_LENGTH, szFilePath );

		int nRoomIndex = RoomFileGetRoomIndexLine( pszFileNameNoExtension );
		ASSERT(nRoomIndex != INVALID_ID);
		
		DWORD dwRoomSeed = RandGetNum(game->randLevelSeed);
		pDrlgRoom->pRoom = RoomAdd(game, level, nRoomIndex, &mRoomMatrix, FALSE, dwRoomSeed, INVALID_ID, pDrlgRoom->idSubLevel);

		if ( pDrlgRoom->pRoom )
		{
			PStrCopy( pDrlgRoom->pRoom->szLayoutOverride, szLayout, DEFAULT_FILE_WITH_PATH_SIZE );
			ASSERT(PStrLen(pDrlgRoom->pRoom->szLayoutOverride) < 128);
		}
	}
	ASSERT(nRoomIndexInLevel <= MAX_ROOMS_PER_LEVEL);

	BOOL bBoxInited = FALSE;
	int nRoomId = tDRLGLevel.pRooms.GetFirst();
	float finalwidth( .1f );
	float width( .1f );
	for ( int i = 0; i < tDRLGLevel.nRoomCount; i++ )
	{
		DRLG_ROOM * pDrlgRoom = tDRLGLevel.pRooms.Get( nRoomId );

		if ( ! pDrlgRoom )
			continue;

		ASSERT_CONTINUE(!pDrlgRoom->pRoom || PStrLen(pDrlgRoom->pRoom->szLayoutOverride) < 128);

		nRoomId = tDRLGLevel.pRooms.GetNextId( nRoomId );

		ConvexHullDestroy( pDrlgRoom->pHull );

		ROOM * pRoom = pDrlgRoom->pRoom;
		if ( ! pRoom )
			continue;

		ASSERT_CONTINUE(PStrLen(pDrlgRoom->pRoom->szLayoutOverride) < 128);

		ASSERT_CONTINUE( pRoom->pHull );
		VECTOR vMin = pRoom->pHull->aabb.center - pRoom->pHull->aabb.halfwidth;
		VECTOR vMax = pRoom->pHull->aabb.center + pRoom->pHull->aabb.halfwidth;
		if ( AppIsTugboat() )
		{
			if( width == .1f )
			{
				width = vMax.x - vMin.x;
				finalwidth = width;
			}
	 
			if( width < finalwidth )
			{
				finalwidth = width;
			}
		}
		if ( bBoxInited )
		{
			BoundingBoxExpandByPoint( &level->m_BoundingBox, &vMin );
			BoundingBoxExpandByPoint( &level->m_BoundingBox, &vMax );
		}
		else
		{
			level->m_BoundingBox.vMin = vMin;
			level->m_BoundingBox.vMax = vMax;
			bBoxInited = TRUE;
		}
	}

	if ( AppIsTugboat() )
	{

			if( finalwidth < 10.0f )
			{
				finalwidth = 10.0f;
			}
			/*finalwidth = .5f;*/
			level->m_CellWidth = finalwidth;
			tDRLGLevel.pRooms.Destroy();
		
			// FIXME : should be based on real data, but this should be cheaper for now
			level->m_BoundingBox.vMax.fZ = 1.0f;
			level->m_BoundingBox.vMin.fZ = 0.0f;
			if( level->m_pQuadtree )
			{
				level->m_pQuadtree->Free();
				GFREE( game, level->m_pQuadtree );
				level->m_pQuadtree = NULL;
			}
			// quadtree for room visibility, now that we know the bounds and base room size
			level->m_pQuadtree = (CQuadtree< ROOM*, CRoomBoundsRetriever>*)GMALLOC( game, sizeof(CQuadtree<ROOM*, CRoomBoundsRetriever>));
			CRoomBoundsRetriever boundsRetriever;
			level->m_pQuadtree->Init( level->m_BoundingBox, finalwidth, boundsRetriever, GameGetMemPool( game ) );

	}


	tDRLGLevel.pRooms.Destroy();
	return DRLG_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGCreateLevelPostProcess(
	GAME * game,
	LEVEL * level)
{
	TIMER_START("DRLGCreateLevelPostProcess()");
	ASSERT_RETURN(game && level);
	
	// init level collision
	// Travis - there appear to be cases where this may already have happened during the level-add on the client,
	// in the paperdoll scene case.
	if( level->m_Collision == NULL )
	{
		LevelCollisionInit(game, level);
	}

	// We have to first create connections and then calculate visibility
	{
		TIMER_START("Level Connections and Visibility");
		ROOM * next = LevelGetFirstRoom(level);
		while (ROOM * room = next)
		{
			next = LevelGetNextRoom(room);
			// for if hammer user loaded background and then did ViewDRLG
			if (AppIsHammer() && !room->pLevel)
			{
				continue;
			}
			DRLGCopyConnectionsFromDefinition(game, room);
			DRLGCreateRoomConnections(game, level, room);
		}


		if (AppIsHellgate())
		{
			ROOM * next = LevelGetFirstRoom(level);
			while (ROOM * room = next)
			{
				next = LevelGetNextRoom(room);
				// for if hammer user loaded background and then did ViewDRLG
				if (AppIsHammer() && !room->pLevel)
				{
					continue;
				}
				DRLGCalculateVisibleRooms(game, level, room);
			}
		}
	}

	// We have to iterate this list twice: once to create (almost) everything
	// and then again to create some extra connections
	{
		TIMER_START("RoomAddPostProcess()");
		ROOM * next = LevelGetFirstRoom(level);
		while (ROOM * room = next)
		{
			next = LevelGetNextRoom(room);
			// for if hammer user loaded background and then did ViewDRLG
			if (AppIsHammer() && !room->pLevel)
			{
				continue;
			}
			RoomAddPostProcess(game, level, room, TRUE);
		}
	}

	{
		ROOM * next = LevelGetFirstRoom(level);
		while (ROOM * room = next)
		{
			next = LevelGetNextRoom(room);
			// for if hammer user loaded background and then did ViewDRLG
			if (AppIsHammer() && !room->pLevel)
			{
				continue;
			}
			if (AppIsTugboat())
			{
				RoomDoSetup(game, level, room );
			}
		}
	}

	// setup level districts
	{
		TIMER_START("s_LevelOrganizeDistricts()");
		s_LevelOrganizeDistricts(game, level);
	}
	s_LevelStartEvents( game, level );

	if ( AppIsHellgate() )
	{
		if ( IS_SERVER( game ) )
			s_QuestEventLevelCreated( game, level );
	}

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_DRLG_TRIES		20

BOOL DRLGCreateLevel(
	GAME* game,
	LEVEL* level,
	DWORD dwSeed,
	UNITID idActivator)
{
	ASSERTX_RETFALSE( LevelTestFlag( level, LEVEL_POPULATED_BIT ) == FALSE, "Level is already populated" );

	RandInit(game->randLevelSeed, dwSeed, 0, TRUE);
	level->m_dwSeed = dwSeed;
	TraceGameInfo("Generating DRLG with seed %d for level def %d", dwSeed, LevelGetDefinitionIndex(level));

	{
		TIMER_START("DRLGCreateLevel()");
		int count = 0;
		DRLG_RESULT eResult = DRLG_FAILED;
		while (++count < MAX_DRLG_TRIES && eResult == DRLG_FAILED)
		{
			eResult = sDRLGCreateLevel(game, level, idActivator);
		}
		
		if (eResult != DRLG_OK)
		{
			const DRLG_DEFINITION * drlg_definition = LevelGetDRLGDefinition( level );
			REF( drlg_definition );
			ASSERTV_RETFALSE(0, 
				"Bad level! %s will not create.\n\nReason: %s", 
				drlg_definition ? drlg_definition->pszName : "(Unknown DRLG!)",
				sDRLGFailureString( eResult ));
		}
		else
		{
#ifdef HAVOK_ENABLED
			if ( AppIsHellgate() )
			{
				TIMER_START("HavokInitWorld()");
				if (level->m_pHavokWorld)
				{
					HavokCloseWorld( &level->m_pHavokWorld );
				}
				const DRLG_DEFINITION * pDRLGDefinition = LevelGetDRLGDefinition( level );
				ASSERT(pDRLGDefinition);
				HavokInitWorld(&level->m_pHavokWorld, IS_CLIENT(game), pDRLGDefinition ? pDRLGDefinition->bCanUseHavokFX : FALSE, &level->m_BoundingBox);
			}
#endif

			// mark level as populated
			LevelSetFlag(level, LEVEL_POPULATED_BIT);
			
			DRLGCreateLevelPostProcess(game, level);
			trace("DRLGCreateLevel(): rerolled %d times.\n", count);
		}
	}
#if ISVERSION(SERVER_VERSION)
	//Perf counters for level creation.
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if(pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerDRLGRate,1);
#endif

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGRoomGetPath(
	const DRLG_ROOM * pDRLGRoom,
	const DRLG_PASS * def,
	char * szFilePath,
	char * szLayout,
	int nStringLength)
{
	char szPath    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	char szName    [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";

	// does this definition have a specific layout?
	DRLGRoomGetGetLayoutOverride(pDRLGRoom->pszFileName, szName, szLayout);

	// is this definition in another folder?
	DRLGRoomGetGetPath(szName, szFilePath, def->nPathIndex);

	ASSERTV_RETURN( szFilePath[0], "DRLG (%s) references DRLG Room (%s), which has an invalid path", def->pszDrlgFileName, pDRLGRoom->pszFileName );
	PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, szFilePath );
	PStrPrintf( szFilePath, nStringLength, "%s%s", szPath, szName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sIterateLayoutGroup(
	const ROOM_LAYOUT_GROUP *ptLayoutGroup,
	PFN_ROOM_LAYOUT_GROUP_ITERATOR pfnCallback,
	void *pCallbackData)
{	
	ASSERTX_RETURN( ptLayoutGroup, "Expected layout group" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );

	// do callback for this layout group
	pfnCallback( ptLayoutGroup, pCallbackData );

	// do any child groups
	for (int i = 0; i < ptLayoutGroup->nGroupCount; ++i)
	{
		const ROOM_LAYOUT_GROUP *ptLayoutGroupChild = &ptLayoutGroup->pGroups[ i ];
		sIterateLayoutGroup( ptLayoutGroupChild, pfnCallback, pCallbackData );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGRoomIterateLayouts(
	const DRLG_ROOM *ptDRLGRoom,
	const DRLG_PASS *ptDRLGPass, 
	PFN_ROOM_LAYOUT_GROUP_ITERATOR pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( ptDRLGRoom, "Expected drlg room" );
	ASSERTX_RETURN( ptDRLGPass, "Expected drlg pass" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );

	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	char szLayout  [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	DRLGRoomGetPath( ptDRLGRoom, ptDRLGPass, szFilePath, szLayout, DEFAULT_FILE_WITH_PATH_SIZE );

	char pszFileNameNoExtension[MAX_XML_STRING_LENGTH];
	PStrRemoveExtension(pszFileNameNoExtension, MAX_XML_STRING_LENGTH, szFilePath );

	char szFilePathLayout[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFilePathLayout, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", pszFileNameNoExtension, szLayout, ROOM_LAYOUT_SUFFIX );
	const ROOM_LAYOUT_GROUP_DEFINITION *pLayoutDef = (const ROOM_LAYOUT_GROUP_DEFINITION*)DefinitionGetByName( DEFINITION_GROUP_ROOM_LAYOUT, szFilePathLayout );
	const ROOM_LAYOUT_GROUP *ptLayoutGroup = &pLayoutDef->tGroup;
	sIterateLayoutGroup( ptLayoutGroup, pfnCallback, pCallbackData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGPassIterateLayouts( 
	const DRLG_PASS *ptDRLGPass, 
	PFN_ROOM_LAYOUT_GROUP_ITERATOR pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( ptDRLGPass, "Expected drlg pass" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );

	// get dlrg level def	
	OVERRIDE_PATH tOverride;
	OverridePathByLine( ptDRLGPass->nPathIndex, &tOverride );
	ASSERT( tOverride.szPath[ 0 ] );
	const DRLG_LEVEL_DEFINITION *ptDRLGLevelDef = DRLGLevelDefinitionGet( tOverride.szPath, ptDRLGPass->pszDrlgFileName );	
	ASSERTX_RETURN( ptDRLGLevelDef, "DRLG pass has no DLRG definition" );

	// First the templates
	for (int j = 0; j < ptDRLGLevelDef->nTemplateCount; ++j)
	{
		const DRLG_LEVEL_TEMPLATE *pDRLGLevelTemplate = &ptDRLGLevelDef->pTemplates[ j ];
		for (int k = 0; k < pDRLGLevelTemplate->nRoomCount; ++k)
		{
			DRLG_ROOM * pDRLGRoom = &pDRLGLevelTemplate->pRooms[ k ];
			DRLGRoomIterateLayouts( pDRLGRoom, ptDRLGPass, pfnCallback, pCallbackData );
		}
	}

	// Now the substitutions
	for (int j = 0; j < ptDRLGLevelDef->nSubstitutionCount; ++j)
	{
		const DRLG_SUBSTITUTION_RULE *ptDLRGSubRule = &ptDRLGLevelDef->pSubsitutionRules[ j ];
		for (int k = 0; k < ptDLRGSubRule->nRuleRoomCount; ++k)
		{
			DRLG_ROOM * ptDRLGRoom = &ptDLRGSubRule->pRuleRooms[ k ];
			DRLGRoomIterateLayouts( ptDRLGRoom, ptDRLGPass, pfnCallback, pCallbackData );			
		}

		for (int k = 0; k < ptDLRGSubRule->nReplacementCount; ++k)
		{
			for (int l = 0; l < ptDLRGSubRule->pnReplacementRoomCount[ k ]; ++l)
			{
				DRLG_ROOM *ptDRLGRoom = &ptDLRGSubRule->ppReplacementRooms[ k ][ l ];
				DRLGRoomIterateLayouts( ptDRLGRoom, ptDRLGPass, pfnCallback, pCallbackData );							
			}				
		}
	}
}

//----------------------------------------------------------------------------
struct DRLG_POSSIBLE_MONSTER_DATA
{
	int *pnMonsterClassBuffer;
	int nBufferSize;
	int *pnCurrentBufferCount;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sDRLGPassGetPossibleMonsters(
	const ROOM_LAYOUT_GROUP *ptLayoutDef,
	void *pCallbackData)
{

	if (ptLayoutDef->eType == ROOM_LAYOUT_ITEM_SPAWNPOINT &&
		ptLayoutDef->dwUnitType == ROOM_SPAWN_MONSTER)
	{	
		int nMonsterClass = ExcelGetLineByCode( NULL, DATATABLE_MONSTERS, ptLayoutDef->dwCode );
		if (nMonsterClass != INVALID_LINK)
		{
			DRLG_POSSIBLE_MONSTER_DATA *pPossibleMonsterData = (DRLG_POSSIBLE_MONSTER_DATA *)pCallbackData;		
			MonsterAddPossible(
				NULL,
				INVALID_LINK,
				nMonsterClass,
				pPossibleMonsterData->pnMonsterClassBuffer,
				pPossibleMonsterData->nBufferSize,
				pPossibleMonsterData->pnCurrentBufferCount);
			
		}
			
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DRLGGetPossibleMonsters(
	GAME *pGame,
	int nDRLG,
	int *pnMonsterClassBuffer,
	int nBufferSize,
	int *pnCurrentBufferCount)
{
	if (nDRLG != INVALID_LINK)
	{
		const DRLG_DEFINITION *ptDRLGDef = DRLGDefinitionGet( nDRLG );
		if (ptDRLGDef)
		{
			// what rule set does this DRLG use
			const char *pszDRLGRuleSet = ptDRLGDef->pszDRLGRuleset;

			// setup callback data
			DRLG_POSSIBLE_MONSTER_DATA tData;
			tData.pnMonsterClassBuffer = pnMonsterClassBuffer;
			tData.nBufferSize = nBufferSize;
			tData.pnCurrentBufferCount = pnCurrentBufferCount;

			// go through all rules
			int nRuleCount = ExcelGetNumRows( pGame, DATATABLE_LEVEL_RULES );
			for (int i = 0; i < nRuleCount; ++i)
			{
				const DRLG_PASS *ptDRLGPass = (const DRLG_PASS *)ExcelGetData( pGame, DATATABLE_LEVEL_RULES, i );
				if (ptDRLGPass && PStrCmp( ptDRLGPass->pszDRLGName, pszDRLGRuleSet ) == 0)
				{
					DRLGPassIterateLayouts( ptDRLGPass, sDRLGPassGetPossibleMonsters, &tData );
				}
			}
		}
	}
}
