//----------------------------------------------------------------------------
// Prime v2.0
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "units.h"
#include "unitdebug.h"
#include "colors.h"
#include "e_primitive.h"
#include "gamelist.h"
#include "c_camera.h"
#include "skills.h"
#include "gameunits.h"
#include "wardrobe.h"
#include "movement.h"
#include "room_path.h"
#include "level.h"

static DWORD sgdwDebugDrawFlags = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitDebugSetFlag(
	UNITDEBUG_FLAG eFlag,
	BOOL bValue)
{
	SETBIT( &sgdwDebugDrawFlags, eFlag, bValue );
	return UnitDebugTestFlag( eFlag );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitDebugToggleFlag(
	UNITDEBUG_FLAG eFlag)
{
	TOGGLEBIT( &sgdwDebugDrawFlags, eFlag );
	return UnitDebugTestFlag( eFlag );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitDebugTestFlag(
	UNITDEBUG_FLAG eFlag)
{
	return TESTBIT( &sgdwDebugDrawFlags, eFlag );
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawPathNodes(
	ROOM *pRoom,
	const VECTOR *pPositionPlayer)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pRoom, "Expected room" );
	const float PATH_NODE_MAX_DISTANCE = 30.0f;
	const float PATH_NODE_MAX_DISTANCE_SQ = PATH_NODE_MAX_DISTANCE * PATH_NODE_MAX_DISTANCE;

	GAME * pGameToUse = UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_CLIENT_BIT ) ? AppGetCltGame() : AppGetSrvGame();
	if(!pGameToUse)
		return;

	// setup array of colors to use
	DWORD dwColors[] = 
	{
		GFXCOLOR_DKYELLOW,
		GFXCOLOR_DKGRAY,
		GFXCOLOR_DKRED,
		GFXCOLOR_DKBLUE,
		GFXCOLOR_DKGREEN,
		GFXCOLOR_DKCYAN,
	};
	int nNumColors = arrsize( dwColors );
	DWORD dwEdgeColor = GFXCOLOR_DKPURPLE;
	DWORD dwNoSpawnColor = GFXCOLOR_RED;

	// which color will we use
	DWORD dwColor = dwColors[ RoomGetId( pRoom ) % nNumColors ];
	dwColor= (dwColor & 0x00ffffff);  // strip off alpha

	// setup draw flags
	DWORD dwFlags = 0;
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_ON_TOP_BIT ))
	{	
		dwFlags |= PRIM_FLAG_RENDER_ON_TOP;
	}
	
	// go through the path nodes here
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );
	for (int i = 0; i < pRoomPathNodes->nPathNodeCount; ++i)
	{
		ROOM_PATH_NODE *pPathNode = &pRoomPathNodes->pPathNodes[ i ];
			
		// get world position
		VECTOR vPositionBottom;
		vPositionBottom = RoomPathNodeGetExactWorldPosition(pGameToUse, pRoom, pPathNode);
		if(AppIsTugboat())
		{
			vPositionBottom.fZ = .01f;
		}

		VECTOR vNodeNormal( 0, 0, 1 );
#if HELLGATE_ONLY
		if( AppIsHellgate() )
		{
			vNodeNormal = RoomPathNodeGetWorldNormal(NULL, pRoom, pPathNode);
		}
#endif

		// what is the distance to the player
		float flDistToPlayerSq = VectorDistanceSquared( vPositionBottom, *pPositionPlayer );
		if (flDistToPlayerSq < PATH_NODE_MAX_DISTANCE_SQ)
		{
		
			// get top node
			VECTOR vPositionTop = vNodeNormal;
			VectorScale( vPositionTop, vPositionTop, 0.5f );
			VectorAdd( vPositionTop, vPositionTop, vPositionBottom );
			
			// draw node			
			e_PrimitiveAddLine(
				dwFlags,
				&vPositionBottom, 
				&vPositionTop, 
				dwColor );

			// setup position for top notches
			VECTOR vNotch = vNodeNormal;
			VectorScale( vNotch, vNotch, 0.25f );
			VECTOR vNotchBottom = vPositionTop;
			VECTOR vNotchTop = vNotchBottom + vNotch;
						
			// check for some states that are useful
			if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
			{
				GAME *pGameServer = UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_CLIENT_BIT ) ? AppGetCltGame() : AppGetSrvGame();
				ROOM *pRoomServer = pGameServer ? RoomGetByID( pGameServer, RoomGetId( pRoom ) ) : NULL;
										
				if(pRoomServer)
				{
				
					// check for spawn free zones
					//LEVEL *pLevelServer = RoomGetLevel( pRoomServer );
					//if (LevelPositionIsInSpawnFreeZone( pLevelServer, &vPositionBottom ))
					DWORD dwNodeInstanceFlags =RoomGetPathNodeInstance( pRoomServer, i )->dwNodeInstanceFlags;
					if (TESTBIT( dwNodeInstanceFlags, NIF_NO_SPAWN ))
					{
					
						// draw line
						e_PrimitiveAddLine(
							dwFlags,
							&vNotchBottom, 
							&vNotchTop, 
							dwNoSpawnColor);

						// increment notch position (for additional notches we want to draw)
						vNotchBottom = vNotchTop;
						vNotchTop = vNotchBottom + vNotch;
					
					}
				
					UNIT * pOccupiedUnit = NULL;
					NODE_RESULT eResult = s_RoomNodeIsBlocked( pRoomServer, i, 0, &pOccupiedUnit );
					if (eResult != NODE_FREE)
					{

						// get cylinder positions
						VECTOR vPositionBottomBlocked = vPositionBottom;
						VECTOR vPositionTopBlocked = vNodeNormal;
						VectorScale( vPositionTopBlocked, vPositionTopBlocked, 0.25f );
						VectorAdd( vPositionTopBlocked, vPositionTopBlocked, vPositionBottomBlocked );

						// what color is the cylinder
						DWORD dwBlockedColor = GFXCOLOR_RED;
						switch (eResult)
						{
							//----------------------------------------------------------------------------
							case NODE_BLOCKED_UNIT:
							{
								dwBlockedColor = GFXCOLOR_DKRED;
								if(pOccupiedUnit->m_pPathing)
								{
									ROOM * pEstimatedRoom = NULL;
									ROOM_PATH_NODE * pEstimatedPathNode = PathStructGetEstimatedLocation(pOccupiedUnit, pEstimatedRoom);
									if(pRoomServer == pEstimatedRoom && pEstimatedPathNode && pEstimatedPathNode->nIndex == i)
									{
										dwBlockedColor = GFXCOLOR_DKCYAN;
									}
								}
								break;
							}
							//----------------------------------------------------------------------------
							case NODE_BLOCKED:
							{
								dwBlockedColor = GFXCOLOR_DKPURPLE;	
								break;
							}
						}			
						dwBlockedColor = (dwBlockedColor & 0x00ffffff);  // strip off alpha

						// draw cylinder
						e_PrimitiveAddCylinder( 
							dwFlags,
							&vPositionBottomBlocked,
							&vPositionTopBlocked,
							0.5f,
							dwBlockedColor);
					}
				}
			}

			// draw edge node notches
			if (TESTBIT( pPathNode->dwFlags, ROOM_PATH_NODE_EDGE ))
			{
				
				// draw line
				e_PrimitiveAddLine(
					dwFlags,
					&vNotchBottom, 
					&vNotchTop, 
					dwEdgeColor);

				// increment notch position (for additional notches we want to draw)
				vNotchBottom = vNotchTop;
				vNotchTop = vNotchBottom + vNotch;
				
			}
						
			// draw connections
			if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_CONNECTION ))
			{
				GAME * pGame = UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_CLIENT_BIT ) ? AppGetCltGame() : AppGetSrvGame();
				if(!pGame)
				{
					pGame = AppGetCltGame();
				}
				ROOM * pRoomToUse = RoomGetByID(pGame, pRoom->idRoom);
				if(!pRoomToUse)
				{
					pRoomToUse = pRoom;
				}
				ROOM_PATH_NODE * pPathNodeToUse = RoomGetRoomPathNode(pRoomToUse, pPathNode->nIndex);
				if(!pPathNodeToUse)
				{
					pPathNodeToUse = pPathNode;
				}

				VECTOR vEndPoint = vPositionBottom;
				vEndPoint.fZ += 0.1f;
				for (int j = 0; j < pPathNodeToUse->nConnectionCount; ++j)
				{
					ROOM_PATH_NODE_CONNECTION *pPathNodeConnection = &pPathNodeToUse->pConnections[ j ];
#if HELLGATE_ONLY
					ROOM_PATH_NODE *pConnection = pPathNodeConnection->pConnection;
#else
					ROOM_PATH_NODE * pConnection = &pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes[pPathNodeConnection->nConnectionIndex];
#endif
					// get position of connection
					VECTOR vPosConnection = RoomPathNodeGetExactWorldPosition(pGame, pRoomToUse, pConnection);
					if(AppIsHellgate())
					{
						vPosConnection.fZ += 0.1f;
					}
					else if(AppIsTugboat())
					{
						vPosConnection.fZ = 0.1f;
					}

					// change color so it's not so loud
					DWORD r = GFXCOLOR_GET_RED( dwColor );
					DWORD g = GFXCOLOR_GET_GREEN( dwColor );
					DWORD b = GFXCOLOR_GET_BLUE( dwColor );
					DWORD a = 0;//GFXCOLOR_GET_ALPHA( dwColor );
					
					int nDarkFactor = 4;  // bigger = darker
					r /= nDarkFactor; g /= nDarkFactor; b /= nDarkFactor;
					DWORD dwConnectionColor = GFXCOLOR_MAKE( r, g, b, a );
					
					// draw connection
					e_PrimitiveAddLine(
						dwFlags,
						&vEndPoint, 
						&vPosConnection, 
						dwConnectionColor);
				}

				if ( (pPathNode->dwFlags & ROOM_PATH_NODE_EDGE_FLAG) && RoomTestFlag(pRoomToUse, ROOM_EDGENODE_CONNECTIONS) )
				{
					int nIndex = pPathNodeToUse->nEdgeIndex;
					EDGE_NODE_INSTANCE * pInstance = &pRoomToUse->pEdgeInstances[nIndex];
					for ( int j = 0; j < pInstance->nEdgeConnectionCount; ++j )
					{
						PATH_NODE_EDGE_CONNECTION * pConnection = &pRoomToUse->pEdgeNodeConnections[pInstance->nEdgeConnectionStart + j];
						int nEdgeIndex = PathNodeEdgeConnectionGetEdgeIndex(pConnection);
						ROOM_PATH_NODE * pOtherRoomNode = pConnection->pRoom->pPathDef->pPathNodeSets[pConnection->pRoom->nPathLayoutSelected].pEdgeNodes[nEdgeIndex];

						// get position of connection
						VECTOR vPosConnection = RoomPathNodeGetExactWorldPosition(pGame, pConnection->pRoom, pOtherRoomNode);
						float fOffset = float(RoomGetId(pRoom) % 100) / 100.0f;
						vPosConnection.fZ += 0.1f + fOffset;
						VECTOR vConnectionEndPoint = vEndPoint;
						vConnectionEndPoint.fZ += fOffset;

						// change color so it's not so loud
						DWORD r = GFXCOLOR_GET_RED( dwColor );
						DWORD g = GFXCOLOR_GET_GREEN( dwColor );
						DWORD b = GFXCOLOR_GET_BLUE( dwColor );
						DWORD a = 0;//GFXCOLOR_GET_ALPHA( dwColor );

						int nDarkFactor = 4;  // bigger = darker
						r /= nDarkFactor; g /= nDarkFactor; b /= nDarkFactor;
						DWORD dwConnectionColor = GFXCOLOR_MAKE( r, g, b, a );

						// draw connection
						e_PrimitiveAddLine(
							dwFlags,
							&vConnectionEndPoint, 
							&vPosConnection, 
							dwConnectionColor);
					}
				}
			}

			if(UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_RADIUS_BIT ) && (flDistToPlayerSq < pRoom->pPathDef->fDiagDistBetweenNodesSq))
			{
				GAME * pGame = UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_CLIENT_BIT ) ? AppGetCltGame() : AppGetSrvGame();
				if(!pGame)
				{
					pGame = AppGetCltGame();
				}
				ROOM * pRoomToUse = RoomGetByID(pGame, pRoom->idRoom);
				if(!pRoomToUse)
				{
					pRoomToUse = pRoom;
				}
				ROOM_PATH_NODE * pPathNodeToUse = RoomGetRoomPathNode(pRoomToUse, pPathNode->nIndex);
				if(!pPathNodeToUse)
				{
					pPathNodeToUse = pPathNode;
				}

				VECTOR vPositionBottomBlocked = vPositionBottom;
				VECTOR vPositionTopBlocked = vNodeNormal;
				VectorScale( vPositionTopBlocked, vPositionTopBlocked, 0.25f );
				VectorAdd( vPositionTopBlocked, vPositionTopBlocked, vPositionBottomBlocked );

				// draw cylinder
				e_PrimitiveAddCylinder( 
					dwFlags,
					&vPositionBottomBlocked,
					&vPositionTopBlocked,
#if HELLGATE_ONLY
					pPathNodeToUse->fRadius,
#else
					PATH_NODE_RADIUS,
#endif
					dwColor);
			}
		}
	}
#endif // !ISVERSION( CLIENT_ONLY_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UIUpdatePathNodes(
	GAME *pGame)
{
#if !ISVERSION(SERVER_VERSION)
	// do nothing if option is not displayed
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATH_NODE_BIT ) == FALSE)
	{
		return;
	}

	// get player	
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer == NULL)
	{
		return;
	}
	const VECTOR &vPositionPlayer = UnitGetPosition( pPlayer );
	
	// get room of player, we will show nodes in only this room for now
	ROOM *pRoom = UnitGetRoom( pPlayer );
	if(!pRoom)
	{
		return;
	}

	// do path nodes for this room
	sDrawPathNodes( pRoom, &vPositionPlayer );
	
	// do path nodes for connected rooms
	int nNumNeighboringRooms = RoomGetVisibleRoomCount(pRoom);
	for (int i = 0; i < nNumNeighboringRooms; ++i)
	{
		ROOM *pRoomNearby = RoomGetVisibleRoom(pRoom, i);
		if (pRoomNearby)
		{
			sDrawPathNodes( pRoomNearby, &vPositionPlayer );
		}
		
	}
#endif //SERVER_VERSION		
}

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawCollisionGridDebugTriangles(
	ROOM_DEFINITION * pRoomDef,
	int nCollisionGridX,
	int nCollisionGridY,
	int nCollideType,
	DWORD dwColor,
	const MATRIX & tWorldMatrix)
{
	ASSERT_RETURN(pRoomDef);
	COLLISION_GRID_NODE * pCollisionGrid = pRoomDef->pCollisionGrid + nCollisionGridY * pRoomDef->nCollisionGridWidth + nCollisionGridX;
	ASSERT_RETURN(pCollisionGrid);

	for(unsigned int i=0; i<pCollisionGrid->nCount[nCollideType]; i++)
	{
		unsigned int nCPolyIndex = pCollisionGrid->pnPolyList[nCollideType][i];
		if((int)nCPolyIndex >= pRoomDef->nRoomCPolyCount)
		{
			continue;
		}
		ROOM_CPOLY * pCPoly = &pRoomDef->pRoomCPolys[nCPolyIndex];
		VECTOR vPt1, vPt2, vPt3;
		MatrixMultiply(&vPt1, &pCPoly->pPt1->vPosition, &tWorldMatrix);
		MatrixMultiply(&vPt2, &pCPoly->pPt2->vPosition, &tWorldMatrix);
		MatrixMultiply(&vPt3, &pCPoly->pPt3->vPosition, &tWorldMatrix);
		e_PrimitiveAddLine(0, &vPt1, &vPt2, dwColor);
		e_PrimitiveAddLine(0, &vPt1, &vPt3, dwColor);
		e_PrimitiveAddLine(0, &vPt2, &vPt3, dwColor);
		//e_PrimitiveAddTri(0, &vPt1, &vPt2, &vPt3, dwColor);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawCollisionDebugTriangles(
	ROOM * pRoom)
{
	if(!pRoom)
		return;

	ROOM_DEFINITION * pRoomDef = pRoom->pDefinition;
	ASSERT_RETURN(pRoomDef);

	for(int i=0; i<pRoomDef->nCollisionGridWidth; i++)
	{
		for(int j=0; j<pRoomDef->nCollisionGridHeight; j++)
		{
			DWORD dwColor = GFXCOLOR_RED;
			sDrawCollisionGridDebugTriangles(pRoomDef, i, j, COLLISION_WALL, dwColor, pRoom->tWorldMatrix);

			dwColor = GFXCOLOR_GREEN;
			sDrawCollisionGridDebugTriangles(pRoomDef, i, j, COLLISION_FLOOR, dwColor, pRoom->tWorldMatrix);

			dwColor = GFXCOLOR_BLUE;
			sDrawCollisionGridDebugTriangles(pRoomDef, i, j, COLLISION_CEILING, dwColor, pRoom->tWorldMatrix);
		}
	}
}
#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UIUpdateCollisionDebugTriangles(
	GAME *pGame)
{
#if !ISVERSION(SERVER_VERSION)
	// do nothing if option is not displayed
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_COLLISION_TRIANGLES_BIT ) == FALSE)
	{
		return;
	}

	// get player	
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer == NULL)
	{
		return;
	}
	
	// get room of player, we will show nodes in only this room for now
	ROOM *pRoom = UnitGetRoom( pPlayer );
	if(!pRoom)
		return;

	// do collision debug triangles for this room
	sDrawCollisionDebugTriangles( pRoom );
	
	// do collision debug triangles for connected rooms
	int nNumNeighboringRooms = RoomGetAdjacentRoomCount(pRoom);
	for (int i = 0; i < nNumNeighboringRooms; ++i)
	{
		ROOM *pRoomNearby = RoomGetAdjacentRoom(pRoom, i);
		if (pRoomNearby)
		{
			sDrawCollisionDebugTriangles( pRoomNearby );
		}
		
	}
#endif //SERVER_VERSION		
}

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawCollisionMaterialDebugTrianglesAdjustPoints(
	VECTOR & vPt1, 
	VECTOR & vPt2, 
	VECTOR & vPt3)
{
	VECTOR vMidPt12 = vPt1 + (vPt2 - vPt1) / 2;
	VECTOR vMidPt13 = vPt1 + (vPt3 - vPt1) / 2;
	VECTOR vMidPt23 = vPt2 + (vPt3 - vPt2) / 2;

	const float TINY_MOVE_AMOUNT = 0.01f;
	vPt1 = vPt1 + ((vMidPt23 - vPt1) * TINY_MOVE_AMOUNT);
	vPt2 = vPt2 + ((vMidPt13 - vPt2) * TINY_MOVE_AMOUNT);
	vPt3 = vPt3 + ((vMidPt12 - vPt3) * TINY_MOVE_AMOUNT);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawCollisionMaterialDebugTriangles(
	GAME * pGame,
	ROOM * pRoom)
{
	if(!pRoom)
		return;

	ROOM_DEFINITION * pRoomDef = pRoom->pDefinition;
	ASSERT_RETURN(pRoomDef);

	for(int i=0; i<pRoomDef->nTriangleCount; i++)
	{
		DWORD dwColor = GFXCOLOR_HOTPINK;
		RoomGetMaterialInfoByTriangleNumber(RoomGetRoomIndex(pGame, pRoom), i, NULL, NULL, NULL, NULL, &dwColor, FALSE);

		VECTOR vPt1, vPt2, vPt3;
		vPt1 = pRoomDef->pVertices[pRoomDef->pwIndexBuffer[i*3 + 0]];
		vPt2 = pRoomDef->pVertices[pRoomDef->pwIndexBuffer[i*3 + 1]];
		vPt3 = pRoomDef->pVertices[pRoomDef->pwIndexBuffer[i*3 + 2]];
		sDrawCollisionMaterialDebugTrianglesAdjustPoints(vPt1, vPt2, vPt3);
		MatrixMultiply(&vPt1, &vPt1, &pRoom->tWorldMatrix);
		MatrixMultiply(&vPt2, &vPt2, &pRoom->tWorldMatrix);
		MatrixMultiply(&vPt3, &vPt3, &pRoom->tWorldMatrix);
		e_PrimitiveAddLine(0, &vPt1, &vPt2, dwColor);
		e_PrimitiveAddLine(0, &vPt1, &vPt3, dwColor);
		e_PrimitiveAddLine(0, &vPt2, &vPt3, dwColor);
	}
}
#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UIUpdateCollisionMaterialDebugTriangles(
	GAME *pGame)
{
#if !ISVERSION(SERVER_VERSION)
	// do nothing if option is not displayed
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_COLLISION_MATERIALS_BIT ) == FALSE)
	{
		return;
	}

	// get player	
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if (pPlayer == NULL)
	{
		return;
	}
	
	// get room of player, we will show nodes in only this room for now
	ROOM *pRoom = UnitGetRoom( pPlayer );
	if(!pRoom)
		return;

	// do collision debug material triangles for this room
	sDrawCollisionMaterialDebugTriangles( pGame, pRoom );
	
	// do collision debug material triangles for connected rooms
	int nNumNeighboringRooms = RoomGetAdjacentRoomCount(pRoom);
	for (int i = 0; i < nNumNeighboringRooms; ++i)
	{
		ROOM *pRoomNearby = RoomGetAdjacentRoom(pRoom, i);
		if (pRoomNearby)
		{
			sDrawCollisionMaterialDebugTriangles( pGame, pRoomNearby );
		}
		
	}
#endif //SERVER_VERSION		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static DWORD sGameGetDebugColor(
	GAME * pGame )
{
	return IS_SERVER( pGame ) ? GFXCOLOR_MAKE( 125, 0, 0, 60 ) : GFXCOLOR_MAKE( 0,  125, 0, 60 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawUnitCollisionAndAiming(
	GAME * pGame,
	UNIT * pCenterUnit )
{	
#define COLLISION_DRAW_MAX_DISTANCE		40.f
#define COLLISION_DRAW_MAX_UNITS		64
#define COLLISION_DRAW_FOV_WIDEN		1.25f

#define AIM_CONE_LENGTH					1.0f
#define AIM_CONE_ANGLE					(PI_OVER_FOUR / 4.0f)

	// finds visible units, sorts depth first, finds display points, and passes to UI

	if ( !pGame )
		return;

	UNITID pnUnitIDs[ COLLISION_DRAW_MAX_UNITS ];
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if (!pCameraInfo)
	{
		return;
	}
	VECTOR vEye = CameraInfoGetPosition( pCameraInfo );

	// get a generous cos FOV assuming a square aspect in largest dimension
	float fCos = c_CameraGetCosFOV( COLLISION_DRAW_FOV_WIDEN );

	SKILL_TARGETING_QUERY tQuery;
	tQuery.fMaxRange			= COLLISION_DRAW_MAX_DISTANCE;
	tQuery.nUnitIdMax			= COLLISION_DRAW_MAX_UNITS;
	tQuery.pnUnitIds			= pnUnitIDs;
	tQuery.pSeekerUnit			= pCenterUnit;
	tQuery.pvLocation			= &vEye;
	tQuery.fDirectionTolerance	= fCos;
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_SEEKER );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES );
	tQuery.tFilter.nUnitType = UNITTYPE_ANY; 

	SkillTargetQuery( pGame, tQuery );

	DWORD dwColor = sGameGetDebugColor( pGame );

	for ( int i = 0; i < tQuery.nUnitIdCount; i++ )
	{
		UNIT * pUnit = UnitGetById( pGame, pnUnitIDs[ i ] );
		if ( ! pUnit )
			continue;

		if ( UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_COLLISION_BIT ) )
		{
			float flRadius = UnitGetCollisionRadius( pUnit );
			float flHeight = UnitGetCollisionHeight( pUnit );
			if ( UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_PATHING_COLLISION_BIT) )
			{
				flRadius = UnitGetPathingCollisionRadius( pUnit );
			}

			VECTOR vBottom = UnitGetPosition( pUnit );
			VECTOR vTop = vBottom;
			vTop.fZ += flHeight;
			VECTOR vCenter = vBottom;
			vCenter.fZ += flHeight / 2.0f;

			DWORD dwFlags = PRIM_FLAG_WIRE_BORDER | PRIM_FLAG_SOLID_FILL;
			e_PrimitiveAddCylinder( dwFlags, &vBottom, &vTop, flRadius, dwColor );
		}

		if ( UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_AIMING_BIT ) )
		{
			int nWardrobe = UnitGetWardrobe( pUnit );
			if ( nWardrobe != INVALID_ID && ! UnitIsA( pUnit, UNITTYPE_ITEM ) )
			{
				for ( int j = 0; j < MAX_WEAPONS_PER_UNIT; j++ )
				{
					if ( ! WardrobeGetWeapon( pGame, nWardrobe, j ))
						continue;

					VECTOR vPosition;
					VECTOR vDirection;
					UnitGetWeaponPositionAndDirection( pGame, pUnit, j, &vPosition, &vDirection );

					e_PrimitiveAddCone( 
						PRIM_FLAG_WIRE_BORDER | PRIM_FLAG_SOLID_FILL,
						&vPosition, 
						AIM_CONE_LENGTH, 
						&vDirection,
						AIM_CONE_ANGLE,
						dwColor );
				}
			}
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawMissiles(
	GAME * pGame,
	UNIT * pCenterUnit )
{	
#define MISSILE_DRAW_MAX_DISTANCE	40.f
#define MISSILE_DRAW_MAX_UNITS		64
#define MISSILE_DRAW_CONE_SIZE		1.0f
#define MISSILE_DRAW_FOV_WIDEN		1.25f
#define MISSILE_CONE_ANGLE			(PI_OVER_FOUR / 2.0f)

	// finds visible units, sorts depth first, finds display points, and passes to UI

	if ( !pGame )
		return;

	UNITID pnUnitIDs[ MISSILE_DRAW_MAX_UNITS ];
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if (!pCameraInfo)
	{
		return;
	}
	VECTOR vEye = CameraInfoGetPosition( pCameraInfo );

	// get a generous cos FOV assuming a square aspect in largest dimension
	float fCos = c_CameraGetCosFOV( MISSILE_DRAW_FOV_WIDEN );

	SKILL_TARGETING_QUERY tQuery;
	tQuery.fMaxRange			= MISSILE_DRAW_MAX_DISTANCE;
	tQuery.nUnitIdMax			= MISSILE_DRAW_MAX_UNITS;
	tQuery.pnUnitIds			= pnUnitIDs;
	tQuery.pSeekerUnit			= pCenterUnit;
	tQuery.pvLocation			= &vEye;
	tQuery.fDirectionTolerance	= fCos;
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES );
	tQuery.tFilter.nUnitType = UNITTYPE_MISSILE; 

	SkillTargetQuery( pGame, tQuery );

	DWORD dwColor = sGameGetDebugColor( pGame );

	for ( int i = 0; i < tQuery.nUnitIdCount; i++ )
	{
		UNIT * pUnit = UnitGetById( pGame, pnUnitIDs[ i ] );
 		if ( ! pUnit )
			continue;

		VECTOR vDirection = UnitGetMoveDirection( pUnit );
		if ( VectorIsZero( vDirection ) )
			vDirection.fZ = 1.0f;
		e_PrimitiveAddCone( 
			PRIM_FLAG_WIRE_BORDER | PRIM_FLAG_SOLID_FILL,
			&UnitGetPosition( pUnit ), 
			MISSILE_DRAW_CONE_SIZE, //UnitGetCollisionRadius( pUnit ), 
			&vDirection,
			MISSILE_CONE_ANGLE,
			dwColor );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDrawMissileVectors(
	GAME * pGame,
	UNIT * pCenterUnit )
{
#define MISSILE_DRAW_MAX_DISTANCE	40.f
#define MISSILE_DRAW_MAX_UNITS		64
#define MISSILE_DRAW_CONE_SIZE		1.0f
#define MISSILE_DRAW_FOV_WIDEN		1.25f
#define MISSILE_CONE_ANGLE			(PI_OVER_FOUR / 2.0f)

	// finds visible units, sorts depth first, finds display points, and passes to UI

	if ( !pGame )
		return;

	UNITID pnUnitIDs[ MISSILE_DRAW_MAX_UNITS ];
	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );
	if (!pCameraInfo)
	{
		return;
	}
	VECTOR vEye = CameraInfoGetPosition( pCameraInfo );

	// get a generous cos FOV assuming a square aspect in largest dimension
	float fCos = c_CameraGetCosFOV( MISSILE_DRAW_FOV_WIDEN );

	SKILL_TARGETING_QUERY tQuery;
	tQuery.fMaxRange			= MISSILE_DRAW_MAX_DISTANCE;
	tQuery.nUnitIdMax			= MISSILE_DRAW_MAX_UNITS;
	tQuery.pnUnitIds			= pnUnitIDs;
	tQuery.pSeekerUnit			= pCenterUnit;
	tQuery.pvLocation			= &vEye;
	tQuery.fDirectionTolerance	= fCos;
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_UNTARGETABLES );
	tQuery.tFilter.nUnitType = UNITTYPE_MISSILE; 

	SkillTargetQuery( pGame, tQuery );

	for ( int i = 0; i < tQuery.nUnitIdCount; i++ )
	{
		UNIT * pUnit = UnitGetById( pGame, pnUnitIDs[ i ] );
 		if ( ! pUnit )
			continue;

		VECTOR vPosition = UnitGetPosition(pUnit);

		VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );
		if ( VectorIsZero( vMoveDirection ) )
			vMoveDirection.fZ = 1.0f;

		VECTOR vUpDirection = UnitGetUpDirection(pUnit);
		vUpDirection += vPosition;
		VECTOR vFaceDirection = UnitGetFaceDirection(pUnit, FALSE);
		vFaceDirection += vPosition;
		VECTOR vFaceDirectionDrawn = UnitGetFaceDirection(pUnit, TRUE);
		vFaceDirectionDrawn += vPosition;
		VECTOR vSideDirection;
		VectorCross(vSideDirection, vMoveDirection, VECTOR(0, 0, 1));
		vSideDirection += vPosition;
		vMoveDirection += vPosition;

		e_PrimitiveAddLine(0, &vPosition, &vMoveDirection, GFXCOLOR_DKBLUE);
		e_PrimitiveAddLine(0, &vPosition, &vUpDirection, GFXCOLOR_DKRED);
		e_PrimitiveAddLine(0, &vPosition, &vFaceDirection, GFXCOLOR_DKGREEN);
		e_PrimitiveAddLine(0, &vPosition, &vFaceDirectionDrawn, GFXCOLOR_DKCYAN);
		e_PrimitiveAddLine(0, &vPosition, &vSideDirection, GFXCOLOR_DKYELLOW);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitDebugUpdate(
	GAME * pGame )
{
#if !ISVERSION(SERVER_VERSION) && !ISVERSION( CLIENT_ONLY_VERSION )
	c_UIUpdatePathNodes( pGame );
	c_UIUpdateCollisionDebugTriangles(pGame);
	c_UIUpdateCollisionMaterialDebugTriangles(pGame);

	c_UIUpdatePathingSplineDebug();

	UNIT * pClientCenterUnit = GameGetControlUnit( pGame );
	UNIT * pServerCenterUnit = NULL;
	BOOL bHasServer = AppGetType() == APP_TYPE_OPEN_SERVER || AppGetType() == APP_TYPE_SINGLE_PLAYER;
	GAME * srvGame = bHasServer ? AppGetSrvGame() : NULL;

	if ( pClientCenterUnit && srvGame )
	{
		pServerCenterUnit = UnitGetById( srvGame, UnitGetId( pClientCenterUnit ) );
	}

	// draw collision cylinder if requested
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_COLLISION_BIT ) ||
		UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_AIMING_BIT ) )
	{
		sDrawUnitCollisionAndAiming( pGame, pClientCenterUnit );
		if ( srvGame && pServerCenterUnit )
			sDrawUnitCollisionAndAiming( srvGame, pServerCenterUnit );
	}

	// draw missile shapes if requested
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_MISSILES_BIT ) )
	{
		sDrawMissiles( pGame, pClientCenterUnit );
		if ( srvGame && pServerCenterUnit )
			sDrawMissiles( srvGame, pServerCenterUnit );
	}

	// draw missile vectors if requested
	if (UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_MISSILE_VECTORS_BIT ) )
	{
		sDrawMissileVectors( pGame, pClientCenterUnit );
		/*
		if ( srvGame && pServerCenterUnit )
			sDrawMissiles( srvGame, pServerCenterUnit );
			// */
	}

	//if ( UnitDebugTestFlag( UNITDEBUG_FLAG_SHOW_AIMING_BIT ) )
	//{
	//	sDrawUnitAiming( pUnit );
	//}
#endif //!SERVER_VERSION
}
