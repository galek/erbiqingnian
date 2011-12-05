//----------------------------------------------------------------------------
// room.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "room_layout.h"
#include "room.h"
#include "room_path.h"
#include "filepaths.h"
#include "level.h"
#include "monsters.h" // also includes units.h, which includes and game.h
#include "gameunits.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "drlg.h"
#include "drlgpriv.h"
#include "excel.h"
#include "c_granny_rigid.h"
#include "inventory.h"
#include "district.h"
#include "gamelist.h"
#include "objects.h"
#include "items.h"
#include "globalindex.h"
#include "clients.h"
#include "dungeon.h"
#include "s_gdi.h"
#include "perfhier.h"
#include "camera_priv.h"
#include "c_camera.h"
#include "ai.h"
#include "automap.h"
#include "console.h"
#include "skills.h"
#include "movement.h"
#include "tasks.h"
#include "player.h"
#include "s_quests.h"
#include "uix_priv.h"
#include "unitmodes.h"
#include "weather.h"
#include "states.h"
#include "uix_graphic.h"		// for some text color-coding
#include "picker.h"
#include "e_model.h"
#include "e_settings.h"
#include "e_main.h"
#include "e_renderlist.h"
#include "e_environment.h"
#include "e_primitive.h"
#include "e_profile.h"
#include "e_region.h"
#include "e_viewer.h"
#include "e_frustum.h"
#include "Currency.h"
#include "e_featureline.h"
#include "warp.h"
#include "gameglobals.h"
#include "gamevariant.h"
#include "s_partyCache.h"
#include "svrstd.h"
#include <search.h>
#include "party.h"
#include "demolevel.h"
#include "s_QuestServer.h"
#include "e_featureline.h"	// CHB 2007.06.28 - For E_FEATURELINE_FOURCC
#include "e_optionstate.h"	// CHB 2007.07.11
#include "s_LevelAreasServer.h"
#if ISVERSION(SERVER_VERSION)
#include "room.cpp.tmh"
#endif
#include "fillpak.h"

using namespace FSSE;


//----------------------------------------------------------------------------
// DEBUGGING CONSTANTS
//----------------------------------------------------------------------------
#define	DEBUG_KNOWN_UNITS				0
#define TRACE_KNOWN(fmt, ...)			// trace(fmt, __VA_ARGS__)


//----------------------------------------------------------------------------
// FILE GLOBALS
//----------------------------------------------------------------------------

enum START_RESET_ROOM_EVENT
{
	SRRE_ON_ROOM_DEACTIVATE,
	SRRE_ON_ROOM_POPULATE,
};

static const START_RESET_ROOM_EVENT sgeStartRoomResetEvent = SRRE_ON_ROOM_DEACTIVATE;
static BOOL sgbDrawAllRooms = FALSE;
static BOOL sgbDrawOnlyConnected = FALSE;
static BOOL sgbDrawOnlyNearby = FALSE;
static const int ROOM_RIGID_BODY_START_COUNT = 2;
static const int ROOM_UNIT_MAP_START_COUNT = 16;
static const BOOL USE_SINGLE_MOPP = TRUE;
static const BOOL USE_MULTI_MOPP = FALSE;

static const float ROOM_ACTIVATION_RADIUS = 35.0f;
static const float ROOM_ACTIVATION_RADIUS_SQ = ROOM_ACTIVATION_RADIUS * ROOM_ACTIVATION_RADIUS;
static const float ROOM_ACTIVATION_RADIUS_TUGBOAT = 35.0f;
static const float ROOM_ACTIVATION_RADIUS_TUGBOAT_SQ = ROOM_ACTIVATION_RADIUS_TUGBOAT * ROOM_ACTIVATION_RADIUS_TUGBOAT;
static const float ROOM_DEACTIVATION_RADIUS = 50.0f;
static const float ROOM_DEACTIVATION_RADIUS_SQ = ROOM_DEACTIVATION_RADIUS * ROOM_DEACTIVATION_RADIUS;
static const float ROOM_DEACTIVATION_RADIUS_TUGBOAT = 50.0f;
static const float ROOM_DEACTIVATION_RADIUS_TUGBOAT_SQ = ROOM_DEACTIVATION_RADIUS_TUGBOAT * ROOM_DEACTIVATION_RADIUS_TUGBOAT;

static SIMPLE_DYNAMIC_ARRAY<VISIBLE_ROOM_COMMAND> gtVisibleRoomCommands;
static SIMPLE_DYNAMIC_ARRAY<ROOM*> gVisibleRoomList;

static int gnNumVisibleRoomPasses;
static int gnCurrentVisibleRoomPass;

static const int IMPORTANT_MONSTER_SPAWN_TRIES = 32;
static const int RANDOM_MONSTER_SPAWN_TRIES = 3;

//----------------------------------------------------------------------------
// PUBLIC GLOBALS
//----------------------------------------------------------------------------
int gnTraceRoom = INVALID_ID;
BOOL gbDumpRooms = FALSE;
const float DEFAULT_FLAT_Z_TOLERANCE = 0.01f;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

static void c_sTestRoomVisibility(
	GAME * pGame,
	LEVEL * pLevel,
	ROOM * pCenterRoom, 
	ROOM * pCurrentRoom, 
	DWORD dwVisibilityMask, 
	MATRIX * pmWorldViewProj, 
	VECTOR * pvViewPosition, 
	VECTOR * pvViewVector, 
	BOUNDING_BOX * pClipBox,
	float fClipRange,
	int nDepth,
	int nPortalDepth);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct ROOM_FIND_FIRST_OF_DATA
{
	SPECIES spSpecies;
	UNIT *pResult;
	UNITTYPE nUnitTypeOf;
	PFN_UNIT_FILTER pfnFilter;
	
	ROOM_FIND_FIRST_OF_DATA::ROOM_FIND_FIRST_OF_DATA( void )
		:	spSpecies( SPECIES_NONE ),
			pResult( NULL ),
			pfnFilter( NULL ),
			nUnitTypeOf( UNITTYPE_NONE )
	{  }
	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindFirstOf(
	UNIT *pUnit,
	void *pCallbackData)
{
	ROOM_FIND_FIRST_OF_DATA *pFindData = (ROOM_FIND_FIRST_OF_DATA *)pCallbackData;

	// must pass filter (if present)
	if (pFindData->pfnFilter == NULL || pFindData->pfnFilter( pUnit ) == TRUE)
	{	
		if (UnitGetSpecies( pUnit ) == pFindData->spSpecies)
		{
			pFindData->pResult = pUnit;
			return RIU_STOP;
		}else if( pFindData->nUnitTypeOf != UNITTYPE_NONE &&
			      UnitIsA( pUnit, pFindData->nUnitTypeOf ) )
		{
			pFindData->pResult = pUnit;
			return RIU_STOP;
		}
	}
	return RIU_CONTINUE;
}

void SpawnLocationInit(
	SPAWN_LOCATION *pSpawnLocation,
	ROOM *pRoom,
	const ROOM_LAYOUT_GROUP *pLayoutGroup,
	const ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation)
{
	RoomLayoutGetSpawnPosition( 
		pLayoutGroup, 
		pSpawnOrientation,
		pRoom, 
		pSpawnLocation->vSpawnPosition, 
		pSpawnLocation->vSpawnDirection,
		pSpawnLocation->vSpawnUp);
	pSpawnLocation->fRadius = 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SpawnLocationInit(
	SPAWN_LOCATION *pSpawnLocation,
	ROOM *pRoom,
	int nNodeIndex,
	float * pfOrientationRadians)
{
	ASSERT_RETURN( pSpawnLocation );
	ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( pRoom, nNodeIndex );
	ASSERT_RETURN( pNode );
	
	// transform position from room to world space
	pSpawnLocation->vSpawnPosition = RoomPathNodeGetExactWorldPosition(RoomGetGame(pRoom), pRoom, pNode);
	
	// direction
	float flOrientationRadians = pfOrientationRadians ? *pfOrientationRadians : 0.0f;	
	MATRIX zRot;
	VECTOR vRotated;
	MatrixRotationAxis(zRot, cgvZAxis, flOrientationRadians);
	MatrixMultiplyNormal( &vRotated, &cgvYAxis, &zRot );
	MatrixMultiplyNormal( &pSpawnLocation->vSpawnDirection, &vRotated, &pRoom->tWorldMatrix );

	pSpawnLocation->vSpawnUp = VECTOR(0, 0, 1);
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomCreateAttachments(int nModelId, ROOM * pRoom)
{
#if !ISVERSION(SERVER_VERSION)
	int nCount = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_ATTACHMENT].nCount;
	for(int i=0; i<nCount; i++)
	{
		ROOM_LAYOUT_GROUP * pLayoutGroup = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_ATTACHMENT].pGroups[i];
		ASSERT_CONTINUE(pLayoutGroup);
		ROOM_LAYOUT_SELECTION_ORIENTATION * pLayoutOrientation = &pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_ATTACHMENT].pOrientations[i];
		ASSERT_CONTINUE(pLayoutOrientation);

		ATTACHMENT_DEFINITION tDefinition;
		e_AttachmentDefInit(tDefinition);
		tDefinition.dwFlags = 0;
		tDefinition.eType = (ATTACHMENT_TYPE)pLayoutGroup->dwUnitType;
		tDefinition.nAttachedDefId = INVALID_ID;
		tDefinition.nBoneId = INVALID_ID;
		tDefinition.nVolume = pLayoutGroup->nVolume;
		PStrCopy(tDefinition.pszAttached, pLayoutGroup->pszName, MAX_XML_STRING_LENGTH);
		PStrCopy(tDefinition.pszBone, "", MAX_XML_STRING_LENGTH);
		tDefinition.vNormal = VECTOR( 0, 0, 1 );//pLayoutOrientation->vNormal;
		tDefinition.vPosition = VECTOR( 0, 0, 0 );//pLayoutOrientation->vPosition;
		tDefinition.fRotation = 0;//pLayoutOrientation->fRotation;
		tDefinition.vScale = pLayoutGroup->vScale;	//scale

		int nLayoutId = c_ModelAttachmentAdd(nModelId, tDefinition,
			FILE_PATH_DATA, TRUE, ATTACHMENT_OWNER_NONE,
			INVALID_ID, INVALID_ID);

		pLayoutGroup->nLayoutId = nLayoutId;

		ATTACHMENT * attachmentInstance = c_ModelAttachmentGet(nModelId, nLayoutId);

		MATRIX matIdentity;
		MatrixIdentity( &matIdentity );

		MATRIX matFinal;		
		GetAlignmentMatrix(&matFinal, pRoom ? pRoom->tWorldMatrix : matIdentity, pLayoutOrientation->vPosition, pLayoutOrientation->vNormal, pLayoutOrientation->fRotation, NULL);

		
		c_AttachmentMove(*attachmentInstance, nModelId, matFinal, pRoom ? pRoom->idRoom : INVALID_ID);

	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelCanTryToUseHavokFX(
	GAME * pGame, 
	LEVEL * pLevel )
{
	const DRLG_DEFINITION * pDRLGDefinition = LevelGetDRLGDefinition(pLevel);

	return IS_CLIENT( pGame ) && (!pDRLGDefinition || pDRLGDefinition->bCanUseHavokFX); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBlockPathNodesFromPropModel(
	ROOM * pRoom,
	PLANE * pPlanes,
	BOOL bRaiseNodes)
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pPlanes);
	ASSERT_RETURN(pRoom->pPathDef);
	ASSERT_RETURN(pRoom->nPathLayoutSelected >= 0 && pRoom->nPathLayoutSelected < pRoom->pPathDef->nPathNodeSetCount);

	for(int i=0; i<pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].nPathNodeCount; i++)
	{
		ROOM_PATH_NODE * pPathNode = &pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes[i];
		ASSERT_CONTINUE(pPathNode);
		if ( HashGet(pRoom->tUnitNodeMap, pPathNode->nIndex))
		{
			continue;
		}


		VECTOR vPos;
		vPos = RoomPathNodeGetExactWorldPosition(RoomGetGame(pRoom), pRoom, pPathNode);
		vPos.fZ += 0.25f;
		BOOL bIntersection = PointInOBBPlanes(pPlanes, vPos);
		if (bIntersection)
		{
			if(bRaiseNodes && AppIsHellgate())
			{
				PATH_NODE_INSTANCE *pNodeInstance = RoomGetPathNodeInstance( pRoom, pPathNode->nIndex );
				if(pNodeInstance)
				{
					SETBIT(pNodeInstance->dwNodeInstanceFlags, NIF_NEEDS_Z_FIXUP);
				}
			}
			else
			{
				UNIT_NODE_MAP * pMap = HashAdd(pRoom->tUnitNodeMap, pPathNode->nIndex);
				pMap->bBlocked = TRUE;
				pMap->pUnit = NULL;
			}
		}
	}
}
// if the radius of the prop is greater than this,
// we do an old-fashioned all-pathnode search.
#define MAX_SEARCH_RADIUS	4
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBlockPathNodesFromPropModel(
	ROOM * pRoom,
	PLANE * pPlanes,
	VECTOR & pPosition,
	float fRadius,
	BOOL bRaiseNodes)
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pPlanes);
	ASSERT_RETURN(pRoom->pPathDef);
	ASSERT_RETURN(pRoom->nPathLayoutSelected >= 0 && pRoom->nPathLayoutSelected < pRoom->pPathDef->nPathNodeSetCount);



	// find a spot to make the spawner at
	const int MAX_PATH_NODES = 1400;
	FREE_PATH_NODE tFreePathNodes[MAX_PATH_NODES];

	NEAREST_NODE_SPEC tSpec;
	SETBIT( tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT );
	SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
	SETBIT( tSpec.dwFlags, NPN_USE_XY_DISTANCE );
	SETBIT( tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES );
	SETBIT( tSpec.dwFlags, NPN_ONE_ROOM_ONLY );
	SETBIT( tSpec.dwFlags, NPN_USE_GIVEN_ROOM );
	SETBIT( tSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES );


	tSpec.fMinDistance = 0;
	tSpec.fMaxDistance = fRadius;

	int tNumNodes = RoomGetNearestPathNodes(RoomGetGame( pRoom ), pRoom, pPosition, MAX_PATH_NODES, tFreePathNodes, &tSpec);




	for(int i=0; i<tNumNodes; i++)
	{
		ROOM_PATH_NODE * pPathNode = tFreePathNodes[i].pNode;
		ASSERT_CONTINUE(pPathNode);

		if ( HashGet(pRoom->tUnitNodeMap, pPathNode->nIndex))
		{
			continue;
		}


		VECTOR vPos;
		vPos = RoomPathNodeGetExactWorldPosition(RoomGetGame(pRoom), pRoom, pPathNode);
		vPos.fZ = pPosition.fZ;
		BOOL bIntersection = PointInOBBPlanes(pPlanes, vPos);
		if (bIntersection)
		{

			if(bRaiseNodes)
			{
				PATH_NODE_INSTANCE * node = RoomGetPathNodeInstance(pRoom, pPathNode->nIndex);
				CLEARBIT( node->dwNodeInstanceFlags, NIF_BLOCKED );
				CLEARBIT( node->dwNodeInstanceFlags, NIF_NO_SPAWN );
		
			}
			else
			{
				PATH_NODE_INSTANCE * node = RoomGetPathNodeInstance(pRoom, pPathNode->nIndex);
				SETBIT( node->dwNodeInstanceFlags, NIF_BLOCKED );
				SETBIT( node->dwNodeInstanceFlags, NIF_NO_SPAWN );
				
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBlockPathNodesFromPropModel(
	LEVEL * pLevel,
	ROOM * pRoom,
	PLANE * pPlanes,
	BOUNDING_BOX * BBox,
	VECTOR & pPosition,
	float fRadius,
	BOOL bRaiseNodes)
{
	ASSERT_RETURN(pRoom);
	ASSERT_RETURN(pPlanes);
	ASSERT_RETURN(pRoom->pPathDef);
	ASSERT_RETURN(pRoom->nPathLayoutSelected >= 0 && pRoom->nPathLayoutSelected < pRoom->pPathDef->nPathNodeSetCount);



	VECTOR Ray = VECTOR( 0, 0, -1 );
	VECTOR ImpactPoint;
	VECTOR Start;
	VECTOR ImpactNormal;
	float Radius = 20;
	float RayLength = 40;
	int Material = PROP_MATERIAL;

	{

		// find a spot to make the spawner at
		const int MAX_PATH_NODES = 1400;
		FREE_PATH_NODE tFreePathNodes[MAX_PATH_NODES];

		NEAREST_NODE_SPEC tSpec;
		SETBIT( tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT );
		SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
		SETBIT( tSpec.dwFlags, NPN_USE_XY_DISTANCE );
		SETBIT( tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES );
		SETBIT( tSpec.dwFlags, NPN_ONE_ROOM_ONLY );
		SETBIT( tSpec.dwFlags, NPN_USE_GIVEN_ROOM );
		SETBIT( tSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES );



		tSpec.fMinDistance = 0;
		tSpec.fMaxDistance = fRadius;



		int nNumNodes = RoomGetNearestPathNodes(RoomGetGame( pRoom ), pRoom, pPosition, MAX_PATH_NODES, tFreePathNodes, &tSpec);

		if( nNumNodes )
		{
			SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
			ArrayInit(SortedFaces, GameGetMemPool(RoomGetGame( pRoom ) ) , 2);

			VECTOR Start( pPosition.fX - fRadius, pPosition.fY - fRadius, 0 );
			VECTOR Delta( fRadius * 2.0f, fRadius * 2.0f, 0 );
			float Dist = VectorLength( Delta );
			VectorNormalize( Delta );
			LevelSortFaces(RoomGetGame( pRoom ), pLevel, SortedFaces, Start, Delta, Dist );
			for(int i=0; i<nNumNodes; i++)
			{
				ROOM_PATH_NODE * pPathNode = tFreePathNodes[i].pNode;
				ASSERT_CONTINUE(pPathNode);


				VECTOR vPos;
				vPos = RoomPathNodeGetExactWorldPosition(RoomGetGame(pRoom), pRoom, pPathNode);
				vPos.fZ = pPosition.fZ;

				BOOL bIntersection = PointInOBBPlanes(pPlanes, vPos);
				if (bIntersection)
				{

					{
						// Tug actually tries a collision too - we need better shape recognition for larger,
						// more interesting props given the way we lay out our hubs with buildings and structures

						Start = vPos;
						Start.fZ = Start.fZ + Radius;

						float Length = LevelLineCollideLen( RoomGetGame(pRoom), pLevel, SortedFaces, Start, Ray, RayLength, Material, &ImpactNormal );

						if( bRaiseNodes && Length < RayLength && Material != PROP_MATERIAL )
						{
							PATH_NODE_INSTANCE * node = RoomGetPathNodeInstance(pRoom, pPathNode->nIndex);
							CLEARBIT( node->dwNodeInstanceFlags, NIF_BLOCKED );
							CLEARBIT( node->dwNodeInstanceFlags, NIF_NO_SPAWN );
							continue;
						}

						if( Length >= RayLength || Material != PROP_MATERIAL)
						{

							continue;
						}
						if ( HashGet(pRoom->tUnitNodeMap, pPathNode->nIndex))	// tugboat nukes object pathnode occupation if they got there before a prop!
						{
							HashRemove(pRoom->tUnitNodeMap, pPathNode->nIndex);
						}
						PATH_NODE_INSTANCE * node = RoomGetPathNodeInstance(pRoom, pPathNode->nIndex);
						SETBIT( node->dwNodeInstanceFlags, NIF_BLOCKED );
						SETBIT( node->dwNodeInstanceFlags, NIF_NO_SPAWN );


					}
				}
			}
			SortedFaces.Destroy();
		}
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomCreateModels(
	GAME * pGame, 
	LEVEL * pLevel,
	ROOM * pRoom)
{
#ifdef HAVOK_ENABLED
	//BOOL bTryToUseHavokFX = sLevelCanTryToUseHavokFX( pGame, pLevel ); 
#endif

	SIMPLE_DYNAMIC_ARRAY<ROOM*> VisibleRoomList;
	ArrayInit(VisibleRoomList, GameGetMemPool(pGame), 2);
	bool LevelContainsRoomCollision = LevelCollisionRoomPopulated( pLevel, pRoom );
	int nCount = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GRAPHICMODEL].nCount;
	for(int i=0; i<nCount; i++)
	{
		ROOM_LAYOUT_GROUP * pModel = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GRAPHICMODEL].pGroups[i];
		ASSERT_CONTINUE(pModel);
		ROOM_LAYOUT_SELECTION_ORIENTATION * pModelOrientation = &pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GRAPHICMODEL].pOrientations[i];
		ASSERT_CONTINUE(pModelOrientation);

		char pszFileNameWithPath[MAX_XML_STRING_LENGTH];
		PStrPrintf(pszFileNameWithPath, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_BACKGROUND, pModel->pszName);

		BOOL bLoadModel = FALSE;
		if (IS_CLIENT(pGame))
		{
			bLoadModel = TRUE;
		}

		ROOM_DEFINITION * pProp = PropDefinitionGet(pGame, pModel->pszName, bLoadModel, pRoom->pDefinition->fInitDistanceToCamera );
		if (!pProp)
		{
			continue;
		}
		ROOM_INDEX * pPropIndex = NULL;
		if(pProp->nRoomExcelIndex >= 0)
		{
			pPropIndex = (ROOM_INDEX*)ExcelGetData(pGame, DATATABLE_PROPS, pProp->nRoomExcelIndex);
		}

		MATRIX matFinal;
		VECTOR scaleVector = VECTOR( pModel->vScale.fX, pModel->vScale.fX, pModel->vScale.fX );
		//this is to fix the scale issue hellgate is having with collision. Some place the vScale is getting wonky values for fY and fZ - which hellgate should never get
		if( AppIsTugboat() )
		{
			scaleVector.fY = pModel->vScale.fY;
			scaleVector.fZ = pModel->vScale.fZ;
		}
		GetAlignmentMatrix(&matFinal, 
							pRoom->tWorldMatrix, 
							pModelOrientation->vPosition,
							pModelOrientation->vNormal, 
							pModelOrientation->fRotation,
							NULL, 
							&scaleVector );


#ifdef HAVOK_ENABLED
		// create havok object
		if( USE_SINGLE_MOPP &&
			pProp->tHavokShapeHolder.pHavokShape )	// tugboat doesn't have Havok shapes
		{
			VECTOR vPosition(0.0f, 0.0f, 0.0f);
			MatrixMultiply(&vPosition, &vPosition, &matFinal);
			QUATERNION qRoomQuat;
			QuaternionRotationMatrix(&qRoomQuat, &matFinal);
			hkRigidBody * pRigidBody = HavokCreateBackgroundRigidBody(pGame, 
				pLevel->m_pHavokWorld, (hkShape*)pProp->tHavokShapeHolder.pHavokShape, 
				&vPosition, &qRoomQuat, false); //bTryToUseHavokFX );//only use havokFX for multimopps
			pRigidBody->setUserData(pPropIndex);
			RoomAddRigidBody( pRoom, pRigidBody );
		}
//
//		// create havok multimopp body
//		//I am unsure of putting this here, before DrlgPathingMesh.  But it looks right.
//		//if(bAllowMultiMopp)
//		if(USE_MULTI_MOPP && bTryToUseHavokFX && IS_CLIENT(pGame))
//		{
//			for(int k = 0; k < pProp->nHavokShapeCount; k++)
//			{
//				ASSERT_BREAK(pProp->ppHavokMultiShape);
//				ASSERT_BREAK(pProp->ppHavokMultiShape[k]);
//#ifdef SINGLE_NUMBER_SELECTION
//				SINGLE_NUMBER_SELECTION;
//#endif
//				VECTOR vPosition(0.0f, 0.0f, 0.0f);
//				vPosition = pProp->vHavokShapeTranslation[k];
//				MatrixMultiply(&vPosition, &vPosition, &matFinal);
//				QUATERNION qRoomQuat;
//				QuaternionRotationMatrix(&qRoomQuat, &matFinal);
//				hkRigidBody * pRigidBody = HavokCreateBackgroundRigidBody(pGame, 
//					pLevel->m_pHavokWorld, (hkShape*)(pProp->ppHavokMultiShape[k]), 
//					&vPosition, &qRoomQuat, bTryToUseHavokFX, true);
//				c_RoomAddInvisibleRigidBody( pRoom, pRigidBody );
//			}
//		}
#endif

		DrlgPathingMeshAddModel(pGame, pRoom, pProp, pModel, &matFinal);
		if( !LevelContainsRoomCollision &&
			 !pPropIndex->bNoCollision )
		{

			LevelCollisionAddProp( pGame, pLevel, pProp, pModel, &matFinal, pPropIndex->bUseMatID );
		}

		if (bLoadModel)
		{
			MATRIX matPlacement;
			GetAlignmentMatrix(&matPlacement, 
				pRoom->tWorldMatrix, 
				pModelOrientation->vPosition,
				pModelOrientation->vNormal, 
				pModelOrientation->fRotation,
				NULL );
			V( e_ModelNew( &pRoom->pLayoutSelections->pPropModelIds[i], pProp->nModelDefinitionId ) );
			V( e_ModelSetFlagbit( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_FLAGBIT_DISTANCE_CULLABLE, TRUE ) );
			e_ModelSetDistanceCullType( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_DISTANCE_CULL_BACKGROUND );
			if( pPropIndex->bClutter )
			{
				e_ModelSetDistanceCullType( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_DISTANCE_CULL_CLUTTER );
				e_ModelSetFarCullMethod( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_FAR_CULL_BY_DISTANCE );
			}
			else if( pPropIndex->bCanopyProp )
			{
				e_ModelSetFarCullMethod( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_CANOPY_CULL_BY_SCREENSIZE );
			}
			else
			{
				e_ModelSetFarCullMethod( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_FAR_CULL_BY_SCREENSIZE );
			}
			VECTOR vWorldPos;
			MatrixMultiply( &vWorldPos, &pModelOrientation->vPosition, &pRoom->tWorldMatrix );		
			
			e_ModelSetScale( pRoom->pLayoutSelections->pPropModelIds[i], scaleVector );
			c_ModelSetLocation(pRoom->pLayoutSelections->pPropModelIds[i], &matPlacement, vWorldPos, RoomGetId(pRoom), MODEL_STATIC );

		}

		float fBonus = -1.0f;
		if(pPropIndex)
		{
			if(pPropIndex->bOccupiesNodes)
			{
				fBonus = pPropIndex->fNodeBuffer;
			}
		}

		if(fBonus >= 0.0f)
		{
			ORIENTED_BOUNDING_BOX pOrientedBox;
			BOUNDING_BOX BBox;
			if ( AppIsHellgate() )
			{
				BOUNDING_BOX tPropBoundingBox = pProp->tBoundingBox;
				BoundingBoxExpandBySize( &tPropBoundingBox, fBonus );
	
				TransformAABBToOBB(pOrientedBox, &matFinal, &tPropBoundingBox);
			}
			else
			{
				BBox = pProp->tBoundingBox;
				BoundingBoxExpandBySize( &BBox, fBonus );
				TransformAABBToOBB(pOrientedBox, &matFinal, &BBox);
				BBox.vMin = pOrientedBox[0];
				BBox.vMax = BBox.vMin;
				for( int i = 0; i < OBB_POINTS; i++ )
				{
					BoundingBoxExpandByPoint( &BBox, &( pOrientedBox[i] ) );
				}
				BoundingBoxExpandBySize( &BBox, 1 );
			}
				
			// Get the planes
			PLANE pPlanes[6];
			MakeOBBPlanes(pPlanes, pOrientedBox);
			
			if ( AppIsTugboat() )
			{
				if( IS_CLIENT( pGame ) )
				{
					SIMPLE_DYNAMIC_ARRAY< ROOM* >	Rooms;
					ArrayInit(Rooms, GameGetMemPool(pGame), 2);

					pLevel->m_pQuadtree->GetItems( BBox, Rooms );

					int nNumRooms= (int)Rooms.Count();
					for (int i = 0; i < nNumRooms; ++i)
					{
						ROOM *pNearRoom = Rooms[i];

						if( BoundingBoxCollideXY( &pNearRoom->tBoundingBoxWorldSpace, &BBox ) )
						{
							float fRadius = sqrt( VectorLengthSquaredXY( BBox.vMax - BBox.vMin ) ) * .5f;
							VECTOR Position = BBox.vMin + ( BBox.vMax - BBox.vMin ) * .5f;

							if( pPropIndex->bFullCollision )
							{
								sBlockPathNodesFromPropModel( pLevel, pNearRoom, pPlanes, &BBox, Position, fRadius, pPropIndex->bRaisesNodes );
							}
							else
							{
								sBlockPathNodesFromPropModel( pNearRoom, pPlanes, Position, fRadius, pPropIndex->bRaisesNodes );
							}

						}

					}
					Rooms.Destroy();
				}
				else
				{
					// get nearby rooms
					int nNumRooms = RoomGetNearbyRoomCount( pRoom );
					for (int i = -1; i < nNumRooms; ++i)
					{
						ROOM *pNearRoom = (i == -1) ? pRoom : RoomGetNearbyRoom( pRoom, i );

						if( BoundingBoxCollideXY( &pNearRoom->tBoundingBoxWorldSpace, &BBox ) )
						{
							float fRadius = sqrt( VectorLengthSquaredXY( BBox.vMax - BBox.vMin ) ) * .5f;
							VECTOR Position = BBox.vMin + ( BBox.vMax - BBox.vMin ) * .5f;

							if( pPropIndex->bFullCollision )
							{
								sBlockPathNodesFromPropModel( pLevel, pNearRoom, pPlanes, &BBox, Position, fRadius, pPropIndex->bRaisesNodes );
							}
							else
							{
								sBlockPathNodesFromPropModel( pNearRoom, pPlanes, Position, fRadius, pPropIndex->bRaisesNodes );
							}
						}
					}
				}
			}
			else
			{
				sBlockPathNodesFromPropModel(pRoom, pPlanes, pPropIndex ? pPropIndex->bRaisesNodes : FALSE);
			}
		}

		if (IS_CLIENT(pGame))
		{
			// add to root model render bounding box
			int nRootModel = RoomGetRootModel( pRoom );
			if ( nRootModel != INVALID_ID )
			{
				//int nRootModelDef = e_ModelGetDefinition( nRootModel );
				e_AddToModelBoundingBox( nRootModel, pModel->nModelId );
			}
		}
	}
	//LevelCollisionSetRoomPopulated( pLevel, pRoom );
	VisibleRoomList.Destroy();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sBeginPortalSection( int nPortalID )
{
	if ( gbDumpRooms )
		trace( "[ Begin Section ]  pass:%2d  id:%2d\n", gnCurrentVisibleRoomPass, nPortalID );

	gnCurrentVisibleRoomPass = gnNumVisibleRoomPasses;

	VISIBLE_ROOM_COMMAND tCommand;
	ZeroMemory( &tCommand, sizeof(VISIBLE_ROOM_COMMAND) );
	tCommand.nCommand = VRCOM_BEGIN_SECTION;
	tCommand.nPass = gnCurrentVisibleRoomPass;
	tCommand.dwData = nPortalID;
	gnNumVisibleRoomPasses++;
	ArrayAddItem(gtVisibleRoomCommands, tCommand );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sEndPortalSection()
{
	if ( gbDumpRooms )
		trace( "[ End Section ]  pass:%2d\n", gnNumVisibleRoomPasses-1 );

	VISIBLE_ROOM_COMMAND tCommand;
	ZeroMemory( &tCommand, sizeof(VISIBLE_ROOM_COMMAND) );
	tCommand.nCommand = VRCOM_END_SECTION;
	tCommand.nPass = gnCurrentVisibleRoomPass;			// mainly for debug purposes
	ArrayAddItem(gtVisibleRoomCommands, tCommand);

	// set back to global -- remember, we don't support nested sections
	gnCurrentVisibleRoomPass = -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomInit(
	GAME* game,
	ROOM* room,
	int nRoomIndex)
{
	ASSERT_RETURN( room );
	if( AppIsHellgate() )
	{
		ArrayInit(room->pRoomModelRigidBodies, GameGetMemPool(game), ROOM_RIGID_BODY_START_COUNT);
		ArrayInit(room->pRoomModelInvisibleRigidBodies, GameGetMemPool(game), ROOM_RIGID_BODY_START_COUNT);
	}
	HashInit(room->tUnitNodeMap, GameGetMemPool(game), ROOM_UNIT_MAP_START_COUNT);
	HashInit(room->tFieldMissileMap, GameGetMemPool(game), ROOM_UNIT_MAP_START_COUNT);

	// set dynamic flag for no hellrift entrance bit
	const ROOM_INDEX *pRoomIndex = RoomGetRoomIndex( game, room );
	if (pRoomIndex->bNoSubLevelEntrance)
	{	
		RoomSetFlag( room, ROOM_NO_SUBLEVEL_ENTRANCE_BIT);
	}

	room->nSpawnTreasureFromUnit.nTreasure = INVALID_ID;
	room->nSpawnTreasureFromUnit.nTargetType = TARGET_INVALID;
	room->nSpawnTreasureFromUnit.nObjectClassOverRide = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomSetDistrict(
	ROOM* pRoom,
	DISTRICT* pDistrict)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pRoom->pDistrict == NULL, "Room is already in a district" );	
	ASSERTX_RETURN( pDistrict, "Expected district" );
	
	// assign	
	pRoom->pDistrict = pDistrict;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DISTRICT* RoomGetDistrict(
	ROOM* pRoom)
{
	ASSERTX_RETNULL( pRoom, "Expected room" );
	return pRoom->pDistrict;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomSetRegion(
	GAME* pGame,
	ROOM* pRoom,
	int nRegion)
{

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomSetSubLevel(
	GAME *pGame,
	ROOM *pRoom,
	SUBLEVELID idSubLevel)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( idSubLevel != INVALID_ID, "Expected sublevel for room" );
	
	// sublevel must exist
	ASSERTX( SubLevelGetById( RoomGetLevel( pRoom ), idSubLevel ), "Setting invalid sublevel on room - is this the client and if so did we send all the sublevels to them from the server?" );
	
	// set the id
	pRoom->idSubLevel = idSubLevel;
	
	// set model regions
	if ( IS_CLIENT(pGame) )
	{
		LEVEL *pLevel = RoomGetLevel( pRoom );
		ASSERTX_RETURN( pLevel, "Room has no level" );
		SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
		ASSERTX_RETURN( pSubLevel, "Sublevel is invalid" );

#if !ISVERSION( SERVER_VERSION )
		// Update bounding box for the sub level / region.
		ASSERTX_RETURN( pRoom->pHull, "Expected room container hull." );
		VECTOR vMin = pRoom->pHull->aabb.center - pRoom->pHull->aabb.halfwidth;
		VECTOR vMax = pRoom->pHull->aabb.center + pRoom->pHull->aabb.halfwidth;
		REGION* pRegion = e_RegionGet( pSubLevel->nEngineRegion );
		if ( pRegion )
		{
			BoundingBoxExpandByPoint( &pRegion->tBBox, &vMin );
			BoundingBoxExpandByPoint( &pRegion->tBBox, &vMax );
		}
#endif
		
		for ( unsigned int i = 0; i < pRoom->pnRoomModels.Count(); i++ )
		{
			c_ModelSetRegion( pRoom->pnRoomModels[ i ], pSubLevel->nEngineRegion );
		}

		for ( unsigned int i = 0; i < pRoom->pnModels.Count(); i++ )
		{
			c_ModelSetRegion( pRoom->pnModels[ i ], pSubLevel->nEngineRegion );
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID RoomGetSubLevelID(
	const ROOM *pRoom)
{
	ASSERTX_RETINVALID( pRoom, "Expected room" );
	return pRoom->idSubLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL *RoomGetSubLevel(
	const ROOM *pRoom)
{
	ASSERTX_RETNULL( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETNULL( pLevel, "Expected level" );
	SUBLEVELID idSubLevel = RoomGetSubLevelID( pRoom );
	ASSERTX_RETNULL( idSubLevel != INVALID_ID, "Expected sublevel id" );
	return SubLevelGetById( pLevel, idSubLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RoomGetDRLGDefIndex(
	const ROOM *pRoom)
{
	ASSERTX_RETINVALID( pRoom, "Expected room" );
	SUBLEVELID idSubLevel = RoomGetSubLevelID( pRoom );
	ASSERTX_RETINVALID( idSubLevel != INVALID_ID, "Expected sublevel id" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	return SubLevelGetDRLG( pLevel, idSubLevel );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DRLG_DEFINITION *RoomGetDRLGDef(
	const ROOM *pRoom)
{
	ASSERTX_RETNULL( pRoom, "Expected room" );
	int nDRLGDef = RoomGetDRLGDefIndex( pRoom );
	ASSERTX_RETNULL( nDRLGDef != INVALID_LINK, "Expected drlg link" );
	return DRLGDefinitionGet( nDRLGDef );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RoomGetSubLevelEngineRegion(
	ROOM *pRoom)
{
	ASSERTX_RETINVALID( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );	
	SUBLEVELID idSubLevel = RoomGetSubLevelID( pRoom );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	ASSERT_RETZERO(pSubLevel);
	return pSubLevel->nEngineRegion;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float RoomGetArea( 
	ROOM* pRoom)
{
	ASSERTX_RETZERO( pRoom, "Expected room" );
	ASSERTX_RETZERO( pRoom->pHull, "Expected hull" );
	
	const CONVEXHULL* pConvexHull = pRoom->pHull;
	float flWidth = pConvexHull->aabb.halfwidth.fX * 2.0f;
	float flHeight = pConvexHull->aabb.halfwidth.fY * 2.0f;
	return flWidth * flHeight;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ROOM_INDEX *RoomGetRoomIndex(
	GAME *pGame,
	const ROOM *pRoom)
{	
	ASSERTX_RETNULL( pRoom, "Expected room" );
	return (ROOM_INDEX *)ExcelGetData( pGame, DATATABLE_ROOM_INDEX, pRoom->pDefinition->nRoomExcelIndex );	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomSetDebugName(
	ROOM *pRoom)
{	
	ASSERTX_RETURN( pRoom, "Expected room" );
	
	#ifdef _DEBUG
	{
		LEVEL *pLevel = RoomGetLevel( pRoom );
		SUBLEVELID idSubLevel = RoomGetSubLevelID( pRoom );
	
		PStrPrintf( 
			pRoom->szName, 
			MAX_ROOM_NAME,
			"RoomDef='%s' [id=%d] (Lev=%s) <SubLev=%s>",
			pRoom->pDefinition->tHeader.pszName,
			RoomGetId( pRoom ),
			LevelGetDevName( pLevel ),
			SubLevelGetDevName( pLevel, idSubLevel ));
	} 
	#endif		
}		


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomAddRoomToNeighboringList(
	GAME * game,
	ROOM * room,
	ROOM **& list,
	unsigned int & len,
	ROOM * neighbor)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(room);
	ASSERT_RETURN(neighbor);
	ASSERT_RETURN(len == 0 || list != NULL);
	for (unsigned int ii = 0; ii < len; ++ii)
	{
		if (list[ii] == neighbor)
		{
			return;
		}
	}
	list = (ROOM **)GREALLOCZ(game, list, sizeof(ROOM *) * (len + 1));
	list[len] = neighbor;
	++len;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomHookupNeighboringRooms(
	GAME * game,
	LEVEL * level,
	ROOM * room,
	BOOL bLevelCreate)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(level);
	ASSERT_RETURN(room);
	ASSERT_RETURN(room->pHull);
	REF(bLevelCreate);

	for (ROOM * nearby = LevelGetFirstRoom(level); nearby; nearby = LevelGetNextRoom(nearby))
	{
		if (nearby == room)
		{
			continue;
		}
		if (nearby->pHull == NULL)
		{
			continue;
		}
		float distance = ConvexHullManhattenDistance(room->pHull, nearby->pHull);
		if (distance <= ADJACENT_ROOM_DISTANCE)
		{
			sRoomAddRoomToNeighboringList(game, room, room->ppAdjacentRooms, room->nAdjacentRoomCount, nearby);
			sRoomAddRoomToNeighboringList(game, nearby, nearby->ppAdjacentRooms, nearby->nAdjacentRoomCount, room);
		}
		if (distance <= NEARBY_ROOM_DISTANCE)
		{
			sRoomAddRoomToNeighboringList(game, room, room->ppNearbyRooms, room->nNearbyRoomCount, nearby);
			sRoomAddRoomToNeighboringList(game, nearby, nearby->ppNearbyRooms, nearby->nNearbyRoomCount, room);
			if (AppIsTugboat())
			{
				sRoomAddRoomToNeighboringList(game, room, room->ppVisibleRooms, room->nVisibleRoomCount, nearby);
				sRoomAddRoomToNeighboringList(game, nearby, nearby->ppVisibleRooms, nearby->nVisibleRoomCount, room);
			}
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomRemoveFromList(
	GAME * game,
	ROOM * roomtoremove,
	ROOM **& list,
	unsigned int & len)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(roomtoremove);
	ASSERT_RETURN(len == 0 || list != NULL);

	for (unsigned int ii = 0; ii < len; ++ii)
	{
		if (list[ii] == roomtoremove)
		{
			for (unsigned int jj = ii; jj < len - 1; ++jj)
			{
				list[jj] = list[jj+1];
			}
			if (ii < len -1)
			{
				list[len - 1] = NULL;
			}
			--len;
			return;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomUnhookNeighboringRooms(
	GAME * game,
	LEVEL * level,
	ROOM * room,
	BOOL bFreeEntireLevel)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(level);
	ASSERT_RETURN(room);

	if (bFreeEntireLevel == FALSE)
	{
		for (unsigned int ii = 0; ii < room->nAdjacentRoomCount; ++ii)
		{
			ROOM * adjacent = room->ppAdjacentRooms[ii];
			ASSERT_CONTINUE(adjacent);
			sRoomRemoveFromList(game, room, adjacent->ppAdjacentRooms, adjacent->nAdjacentRoomCount);
			room->ppAdjacentRooms[ii] = NULL;
		}
	}
	GFREE(game, room->ppAdjacentRooms);
	room->ppAdjacentRooms = NULL;
	room->nAdjacentRoomCount = 0;

	if (bFreeEntireLevel == FALSE)
	{
		for (unsigned int ii = 0; ii < room->nNearbyRoomCount; ++ii)
		{
			ROOM * nearby = room->ppNearbyRooms[ii];
			ASSERT_CONTINUE(nearby);
			sRoomRemoveFromList(game, room, nearby->ppNearbyRooms, nearby->nNearbyRoomCount);
			room->ppNearbyRooms[ii] = NULL;
		}
	}
	GFREE(game, room->ppNearbyRooms);
	room->ppNearbyRooms = NULL;
	room->nNearbyRoomCount = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * RoomAdd(
	GAME * game,
	LEVEL * level,
	int nRoomIndex,
	MATRIX * pMatrix,
	BOOL bRunPostProcess,
	DWORD dwRoomSeed,
	ROOMID idRoom,
	SUBLEVELID idSubLevel,
	const char * pszLayoutOverride)
{
	ROOM* room = RoomGetByID(game, idRoom);
	if (room)
	{
		return room;
	}
	room = (ROOM*)GMALLOCZ(game, sizeof(ROOM));
	room->m_pGame = game;
	room->pDefinition = RoomDefinitionGet(game, nRoomIndex, IS_CLIENT(game));
	ASSERT(room->pDefinition);
	if (!room->pDefinition)
	{
		GFREE(game, room);
		return NULL;
	}
	room->pLevel = level;
	room->idRoom = idRoom;
	room->dwRoomSeed = dwRoomSeed;
	//sets all the themes to render
	SETBIT( room->pdwFlags, ROOM_IGNORE_ALL_THEMES, level->m_LevelAreaOverrides.bAllowsDiffClientAndServerThemes );
	room->dwResetDelay = 0;
	room->tResetTick.tick = 0;
		
	// init visual portals	
	for (int i = 0; i < MAX_VISUAL_PORTALS_PER_ROOM; ++i)
	{
		room->pVisualPortals[ i ].nEnginePortalID = INVALID_ID;
		room->pVisualPortals[ i ].idVisualPortal  = INVALID_ID;
	}
	room->nNumVisualPortals = 0;

	// init counts
	for (int i = 0; i < RMCT_NUM_TYPES; ++i)
	{
		room->nMonsterCountOnPopulate[ i ] = 0;
	}
		
	TRACE_ROOM( idRoom, "Add" );

	room->tWorldMatrix = *pMatrix;

	RoomInit( game, room, nRoomIndex );

	if ( room->pDefinition->nHullVertexCount == 0 )
	{
		ASSERTV(0, "ERROR : %s is missing a hull.", room->pDefinition->tHeader.pszName );
	}
	ASSERT( room->pDefinition->nHullVertexCount < MAX_HULL_VERTICES );
	VECTOR vHullPoints[ MAX_HULL_VERTICES ];
	for ( int v = 0; v < room->pDefinition->nHullVertexCount; v++ )
	{
		MatrixMultiply( &vHullPoints[v], &room->pDefinition->pHullVertices[v], pMatrix );
	}
	room->pHull = HullCreate(&vHullPoints[0], room->pDefinition->nHullVertexCount, room->pDefinition->pHullTriangles, room->pDefinition->nHullTriangleCount, room->pDefinition->tHeader.pszName);

	//BoundingBoxTranslate(&room->tWorldMatrix, &room->tBoundingBox, &room->pDefinition->tBoundingBox);
	MatrixInverse(&room->tWorldMatrixInverse, &room->tWorldMatrix);

	LevelAddRoom(game, level, room);

	// init the debug name with better info now that we have a room id assigned
	sRoomSetDebugName( room );
	
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT(game) )
	{
		ArrayInit(room->pnModels, GameGetMemPool(game), ROOM_DYNAMIC_MODEL_START_COUNT);
		ArrayInit(room->pnRoomModels, GameGetMemPool(game), ROOM_STATIC_MODEL_START_COUNT);

		if ( AppIsHellgate() )
		{
			if ( ! RoomUsesOutdoorVisibility( room->idRoom ) && ! e_CellGet( room->idRoom ) )
			{
				V( e_CellCreateWithID( room->idRoom, RoomGetSubLevelEngineRegion( room ) ) );
			}
		}
	}
#endif // !ISVERSION(SERVER_VERSION)

	// set sublevel
	RoomSetSubLevel( game, room, idSubLevel );

	if(pszLayoutOverride)
	{
		PStrCopy(room->szLayoutOverride, DEFAULT_FILE_WITH_PATH_SIZE, pszLayoutOverride, 32);
	}
	else
	{
		memclear(room->szLayoutOverride, DEFAULT_FILE_WITH_PATH_SIZE * sizeof(char));
	}

	// save the bounding sphere of this room
	RoomGetBoundingSphereWorldSpace(room, room->tBoundingSphereWorldSpace.r, room->tBoundingSphereWorldSpace.C);

	if (bRunPostProcess)
	{
		RoomAddPostProcess(game, level, room, FALSE);
		if (IS_CLIENT(game))
		{
			RoomCreatePathEdgeNodeConnections(game, room);
		}
	}

	if (IS_CLIENT(game))
	{
		RoomSetFlag(room, ROOM_ACTIVE_BIT, TRUE);
	}

	return room;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_sRoomSpawnMonsterAndMinions(
	GAME * pGame, 
	ROOM * pRoom, 
	SPAWN_ENTRY *pSpawn, // Can be NULL if you're just spawning minions
	int nMonsterClass,
	int nMonsterQuality,
	VECTOR *pvSpawnPosition,
	VECTOR *pvSpawnDirection,
	PFN_MONSTER_SPAWNED_CALLBACK pfnSpawnedCallback,
	void *pCallbackData,
	float fAppearInRandomRadius )
{
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_NOMONSTERS) != 0 )
	{
		return 0;
	}
#endif

	const UNIT_DATA * pMonsterData = MonsterGetData(pGame, nMonsterClass);
	if (!pMonsterData)
	{
		return 0;
	}
	if (!UnitDataTestFlag( pMonsterData, UNIT_DATA_FLAG_SPAWN ))
	{
		return 0;
	}

	// get the experience level we will spawn the monster at
	LEVEL *pLevel = RoomGetLevel( pRoom );
	int nExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );

	// we should never be spawning monsters that are too hard for this level (if they have one)
	if( pMonsterData->nMinMonsterExperienceLevel > 0 && pMonsterData->nMinMonsterExperienceLevel > nExperienceLevel)
	{
		const int MAX_STRING = 1024;
		char szString[ MAX_STRING ];
		PStrPrintf(
			szString,
			MAX_STRING,
			"Monster '%s' cannot be spawned because its min experience level (%d) is too high (max is %d) for this area '%s'",
			pMonsterData->szName,
			pMonsterData->nMinMonsterExperienceLevel,
			LevelGetMonsterExperienceLevel( pLevel ),
			LevelGetDevName( pLevel ));
		ASSERTX_RETZERO( 0, szString );
	}

	// avoid inf loop if spawn pos already outside bounding box
	// instead 
	/* edit by coffee 2010-6-29 暂时不处理此部分，以后再修复
	if ( !ConvexHullPointTest( pRoom->pHull, pvSpawnPosition ) )
	{
		const GLOBAL_DEFINITION* pGlobal = DefinitionGetGlobal();
		if ( pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS )
		{
			ASSERTV(0, "Spawn point outside room bounding box in room %s", pRoom->pDefinition->tHeader.pszName );
		}
		return 0;  // this used to return TRUE when it was boolean, that seems wrong to me tho - cday
	}
	*/

	// get number of monsters to spawn with this one, for champions we ignore the count
	// from the spawn class in favor of the count from the monster quality table for
	// the chosen champion monster quality
	int nNumMonsters = pSpawn ? 1 : 0;
	int nNumMinions = 0;
	int nMinionQuality = INVALID_LINK;
	if (nMonsterQuality == INVALID_LINK)
	{
		if(pSpawn)
		{
			nNumMonsters = SpawnEntryEvaluateCount( pGame, pSpawn );
		}
		nNumMinions = MonsterEvaluateMinionPackSize( pGame, pMonsterData );
	}
	else
	{
		nNumMinions = MonsterChampionEvaluateMinionPackSize( pGame, nMonsterQuality );
		// KCK: This used to be Tugboat only, but as only one Hellgate monster uses this, and we want to have
		// the number of minions specified, I think it's good to use this for Hellgate as well.
		nNumMinions = MAX( nNumMinions, MonsterEvaluateMinionPackSize( pGame, pMonsterData ) );
		if( AppIsHellgate() )
		{
			nMinionQuality = MonsterChampionMinionGetQuality( pGame, nMonsterQuality );
		}
		else if( AppIsTugboat() )
		{
			//tugboat doesn't want unique boss minions spawning
			//Marsh: I don't like this but I need to think of a good way to handle this for all cases( monster spawn, cascade, skillSpawnMinion, ect )
			nMinionQuality = INVALID_ID; //force to be normal monster.
		}
	}

	BOOL bFirstMonster = TRUE;
	int nWeaponClass = INVALID_ID;
	int nAttempts = 0;
	enum { MAX_SPAWN_ATTEMPTS = 20 };
	int nNumSpawned = 0;
	while ((nNumMonsters != 0 || nNumMinions != 0) && nAttempts < ( MAX_SPAWN_ATTEMPTS * ( nNumMonsters + nNumMinions ) ) )
	{

		// record the attempt
		nAttempts++;

		// are we spanwing a minion or not
		BOOL bSpawningMinion = FALSE;
		if (nNumMonsters == 0 && nNumMinions != 0)
		{
			bSpawningMinion = TRUE;
		}
		float fDist = RandGetFloat(pGame) + 1.0f;

		float fAngle = TWOxPI * RandGetFloat(pGame);

		// set spawn position
		VECTOR vSpawn = *pvSpawnPosition;		
		// adjust position for multiple monsters (we want to keep the first one exactly
		// where specified so that placement via the layout editor will be exact, like for
		// the statues in the British Museum)
		if( fAppearInRandomRadius > 1.0f )
		{
			ASSERT( fAppearInRandomRadius < 100.0f );
			vSpawn.fX += fAppearInRandomRadius * 2.0f * ( RandGetFloat(pGame) - .5f);
			vSpawn.fY += fAppearInRandomRadius * 2.0f * ( RandGetFloat(pGame) - .5f);
		}
		else if ( AppIsTugboat() || nNumSpawned > 0)
		{
			if( AppIsTugboat() )
			{
				fDist *= max( 1, UnitGetMaxCollisionRadius( pGame, GENUS_MONSTER, nMonsterClass ) ) * 2.75f;
			}

			vSpawn.fX += ( cosf( fAngle ) * fDist );
			vSpawn.fY += ( sinf( fAngle ) * fDist );
		}

		if ( AppIsTugboat() )
		{
			vSpawn.fZ = VerifyFloat( vSpawn.fZ, 0.1f );
		}

		// still in room?
		if ( !ConvexHullPointTest( pRoom->pHull, &vSpawn ) )
			continue;


		if( AppIsTugboat() )
		{

			FREE_PATH_NODE tFreePathNodes[2];
			NEAREST_NODE_SPEC tSpec;
			tSpec.fMaxDistance = 10;
			SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
			SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
			SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
			SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
			SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
			SETBIT(tSpec.dwFlags, NPN_ONE_ROOM_ONLY);
			SETBIT(tSpec.dwFlags, NPN_USE_GIVEN_ROOM);

			int nCount = RoomGetNearestPathNodes(pGame, pRoom, vSpawn, 1, tFreePathNodes, &tSpec);
			if(nCount == 1 && pRoom)
			{
				const FREE_PATH_NODE * freePathNode = &tFreePathNodes[0];
				if( freePathNode )
				{
					vSpawn = RoomPathNodeGetWorldPosition(pGame, freePathNode->pRoom, freePathNode->pNode);
					vSpawn.fZ = pvSpawnPosition->fZ;
					vSpawn.fZ = VerifyFloat( vSpawn.fZ, 0.1f );
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}

		}
		else
		{
			// put me on the ground, etc
			float flRadius = UnitGetMaxCollisionRadius( pGame, GENUS_MONSTER, nMonsterClass );
			float flHeight = UnitGetMaxCollisionHeight( pGame, GENUS_MONSTER, nMonsterClass );

			// can I stand here?, and put me on the ground!
			if ( RoomTestPos(pGame, pRoom, *pvSpawnPosition, &vSpawn, flRadius, flHeight, 0) )
				continue;	// not a good place...
		}
		VECTOR vFace;
		VectorDirectionFromAngleRadians( vFace, fAngle );

		// what monster quality to use for this spawn
		int nMonsterQualityForSpawn = nMonsterQuality;
		int nMonsterClassForSpawn = nMonsterClass;
		if (bSpawningMinion == TRUE)
		{
			nMonsterQualityForSpawn = nMinionQuality;
			if( pMonsterData->nMinionClass != INVALID_ID )
			{
				nMonsterClassForSpawn = pMonsterData->nMinionClass;
			}
		}

		MONSTER_SPEC tSpec;
		tSpec.nClass = nMonsterClassForSpawn;
		tSpec.nExperienceLevel = nExperienceLevel;
		tSpec.nMonsterQuality = nMonsterQualityForSpawn;
		tSpec.pRoom = pRoom;
		tSpec.vPosition= vSpawn;
		tSpec.vDirection = fAppearInRandomRadius > 1.0f ? vFace : *pvSpawnDirection;
		tSpec.nWeaponClass = nWeaponClass;

		UNIT * pMonster = NULL;
		if(IS_SERVER(pGame))
		{
			pMonster = s_MonsterSpawn( pGame, tSpec );
			// minions don't respawn on death - they'll get respawned by the spawner next time round
			if( bSpawningMinion && pMonster )
			{
				UnitClearFlag( pMonster, UNITFLAG_SHOULD_RESPAWN );
			}

			// spawn class tracking
#if ISVERSION(DEVELOPMENT)
			if (pSpawn && pSpawn->nSpawnClassSource != INVALID_LINK)
			{
				UnitSetStat( pMonster, STATS_SPAWN_CLASS_SOURCE, pSpawn->nSpawnClassSource );
			}
#endif
		}
		else
		{
			pMonster = MonsterSpawnClient(
				pGame, 
				nMonsterClass, 
				pRoom, 
				vSpawn, 
				*pvSpawnDirection);
		}
		ASSERT(pMonster);

		//NOTE!
		// The unit's room may NOT be the room that's passed in, due to the path node stuffs.
		pRoom = UnitGetRoom(pMonster);

		if (bFirstMonster)
		{
			UNIT * pWeapon = UnitInventoryGetByLocation(pMonster, INVLOC_RHAND);
			if (pWeapon)
			{
				nWeaponClass = UnitGetClass(pWeapon);
			}
			bFirstMonster = FALSE;
		}

		// decrement monster count
		if (bSpawningMinion == TRUE)
		{
			nNumMinions--;
		}
		else
		{
			nNumMonsters--;
		}
		nNumSpawned++;
		nAttempts = 0;

		// do callback (if we have one)
		if (pfnSpawnedCallback)
		{
			pfnSpawnedCallback( pMonster, pCallbackData );
		}
	}

	return nNumSpawned;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int s_RoomSpawnMinions( 
	GAME * pGame, 
	UNIT * pMonster,
	ROOM * pRoom, 
	VECTOR *vPosition )
{
	int nMonsterQuality = MonsterGetQuality( pMonster );
	int nMonsterClass = UnitGetClass( pMonster );
	VECTOR *pvSpawnPosition = vPosition;
	VECTOR *pvSpawnDirection = &pMonster->vFaceDirection;

	return s_sRoomSpawnMonsterAndMinions(pGame, pRoom, NULL, nMonsterClass, nMonsterQuality, pvSpawnPosition, pvSpawnDirection, NULL, NULL, 0.0f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int s_RoomSpawnMonsterByMonsterClass( 
	GAME * pGame, 
	SPAWN_ENTRY *pSpawn,
	int nMonsterQuality,
	ROOM * pRoom, 
	SPAWN_LOCATION *pSpawnLocation,
	PFN_MONSTER_SPAWNED_CALLBACK pfnSpawnedCallback,
	void *pCallbackData)
{
	ASSERT( pSpawn->eEntryType == SPAWN_ENTRY_TYPE_MONSTER );
	int nMonsterClass = pSpawn->nIndex;
	VECTOR *pvSpawnPosition = &pSpawnLocation->vSpawnPosition;
	VECTOR *pvSpawnDirection = &pSpawnLocation->vSpawnDirection;

	return s_sRoomSpawnMonsterAndMinions(pGame, pRoom, pSpawn, nMonsterClass, nMonsterQuality, pvSpawnPosition, pvSpawnDirection, pfnSpawnedCallback, pCallbackData, pSpawnLocation->fRadius );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline BOOL sSpawnCodeIsValid(ROOM_LAYOUT_GROUP * pSpawnPoint)
{
	return (pSpawnPoint->dwCode && pSpawnPoint->dwCode != 0xFFFFFFFF);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline BOOL sSpawnClassIsValid(DWORD dwSpawnClassCode)
{
	return (dwSpawnClassCode && dwSpawnClassCode != 0xFFFFFFFF);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_RoomPickMonstersToSpawn(
	GAME* pGame,
	ROOM* pRoom,
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ],
	int nSpawnClass)
{
	ASSERT_RETINVALID( pGame && pRoom );
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERT_RETINVALID(pLevel);

	// we want the result order to be randomized
	DWORD dwSpawnClassEvaluateFlags = 0;
	SETBIT( dwSpawnClassEvaluateFlags, SCEF_RANDOMIZE_PICKS_BIT );
	
	int nMonsterExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
	if (nSpawnClass == INVALID_LINK)
	{
		int nSpawnClass = INVALID_LINK;
		SUBLEVELID idSubLevel = RoomGetSubLevelID( pRoom );
		LEVEL *pLevel = RoomGetLevel( pRoom );
		
		// if the sublevel of this room overrides the spawn class for the overall
		// level, use that spawn class instead
		if (s_SubLevelOverrideLevelSpawns( pLevel, idSubLevel ) == TRUE)
		{		
			nSpawnClass = s_SubLevelGetSpawnClass( pLevel, idSubLevel );		
		}
		else
		{
			// get the spawn class to use based on the room district in the level
			DISTRICT* pDistrict = RoomGetDistrict( pRoom );
			nSpawnClass = DistrictGetSpawnClass( pDistrict );			
		}

		if (nSpawnClass != INVALID_LINK)
		{
			return SpawnClassEvaluate(
				pGame, 
				pLevel->m_idLevel, 
				nSpawnClass,
				nMonsterExperienceLevel, 
				tSpawns, 
				NULL,
				-1,
				dwSpawnClassEvaluateFlags);
		}
	}
	else
	{
		return SpawnClassEvaluate(
			pGame, 
			pLevel->m_idLevel, 
			nSpawnClass, 
			nMonsterExperienceLevel, 
			tSpawns, 
			NULL,
			01,
			dwSpawnClassEvaluateFlags);
	}
	return 0;
}

//----------------------------------------------------------------------------
struct BY_SPAWN_CLASS_DATA
{
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint;
	ROOM_LAYOUT_GROUP *pSpawnPoint;
	int nCountByDensityValue;
	BOOL bSpawnedChampion;
	
	BY_SPAWN_CLASS_DATA::BY_SPAWN_CLASS_DATA( void )
		:	pTrackedSpawnPoint( NULL ),
			pSpawnPoint( NULL ),
			nCountByDensityValue( 0 ),
			bSpawnedChampion( FALSE )
	{  }
	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnPointCreateUnitPostProcess(
	UNIT *pUnit,
	const ROOM_LAYOUT_GROUP *pSpawnPoint)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	// make the monster stone if this node is supposed to create stone monsters
	if (pSpawnPoint != NULL)
	{
		if (TESTBIT( pSpawnPoint->dwFlags, ROOM_LAYOUT_AI_NODE_STONE ))
		{
			s_StateSet( pUnit, pUnit, STATE_STONE, 0 );
		}
	}
	//lets set the units theme for the client
	if( AppIsTugboat() &&
		//UnitGetLevel( pUnit )->m_LevelAreaOverrides.bAllowsDiffClientAndServerThemes &&
		pSpawnPoint &&
		pSpawnPoint->nTheme != INVALID_ID )
	{
		int nNot = (TESTBIT( pSpawnPoint->dwFlags, ROOM_LAYOUT_NOT_THEME ))?-1:1;
		UnitSetStat( pUnit, STATS_UNIT_THEME, pSpawnPoint->nTheme * nNot );
	}
		
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sSpawnMonsterByMonsterClassCallback(
	UNIT *pSpawned,
	void *pUserData)
{
	ASSERTX_RETURN( pSpawned, "Expected unit" );
	BY_SPAWN_CLASS_DATA	*pCallbackData = (BY_SPAWN_CLASS_DATA *)pUserData;

	// alter the count by the density value of this unit
	const UNIT_DATA *pUnitData = UnitGetData( pSpawned );
	pCallbackData->nCountByDensityValue += pUnitData->nDensityValueOverride;

	// if we spawned a champion, remember we did so
	if (MonsterGetQuality( pSpawned ) != INVALID_LINK)
	{
		pCallbackData->bSpawnedChampion = TRUE;
	}
		
	// forget it if not from an actual spawn point
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint = pCallbackData->pTrackedSpawnPoint;
	if (pTrackedSpawnPoint != NULL)
	{
		// ok, add to tracking array
		RoomTrackedSpawnPointAddUnit( pTrackedSpawnPoint, pSpawned );
	}
	
	// do post process on spawn
	sSpawnPointCreateUnitPostProcess( pSpawned, pCallbackData->pSpawnPoint );
						
}

inline int sRoomSpawnMonstersBySpawnClass(
	GAME* pGame,
	ROOM* pRoom,
	SPAWN_LOCATION *pSpawnLocation,
	DWORD dwSpawnClassCode,
	int nDesiredMonsterQuality,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint,
	ROOM_POPULATE_PASS ePass,
	int nMaxTries,
	ROOM_LAYOUT_GROUP *pSpawnPoint,
	int& nMaxChampionCount )
{

	// setup callback data
	BY_SPAWN_CLASS_DATA tCallbackData;
	tCallbackData.pSpawnPoint = pSpawnPoint;
	tCallbackData.pTrackedSpawnPoint = pTrackedSpawnPoint;
	tCallbackData.nCountByDensityValue = 0;
	tCallbackData.bSpawnedChampion = FALSE;

	//radius to spawn them in....
	pSpawnLocation->fRadius = ( pSpawnPoint && pSpawnPoint->fSpawnClassRadius >= 1.0f )?pSpawnPoint->fSpawnClassRadius:0.0f;

	int nNumSpawned = 0;
	BOOL bTryAgain = TRUE;
	int nTries = nMaxTries;
	while (bTryAgain && nTries-- > 0)
	{

		SPAWN_ENTRY tSpawns[MAX_SPAWNS_AT_ONCE];
		int nSpawnCount = 0;

		if (sSpawnClassIsValid( dwSpawnClassCode ))
		{
			int nClass = ExcelGetLineByCode(pGame, DATATABLE_SPAWN_CLASS, dwSpawnClassCode);
			nSpawnCount = s_RoomPickMonstersToSpawn(pGame, pRoom, tSpawns, nClass);
		}
		else
		{
			nSpawnCount = s_RoomPickMonstersToSpawn(pGame, pRoom, tSpawns, INVALID_LINK);
		}

		for (int i = 0; i < nSpawnCount; i++)
		{
			SPAWN_ENTRY *pSpawn = &tSpawns[ i ];

			int nMonsterClass = pSpawn->nIndex;
			int nMonsterQualityToUse = nDesiredMonsterQuality;

			// get monster data
			const UNIT_DATA *pMonsterData = MonsterGetData( pGame, nMonsterClass );

			// ignore monsters that are not for the current pass
			if (pMonsterData->eRoomPopulatePass != ePass)
			{
				continue;
			}

			// if this monster can't be a champion, disreguard the desired quality
			if (nMonsterQualityToUse != INVALID_LINK)
			{
				if (!UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_CAN_BE_CHAMPION))
				{
					nMonsterQualityToUse = INVALID_LINK;
				}
			}

			// reset the count tracker
			tCallbackData.nCountByDensityValue = 0;

			// spawn by class	
			s_RoomSpawnMonsterByMonsterClass(
				pGame, 
				pSpawn, 
				nMaxChampionCount > 0 ? nMonsterQualityToUse : INVALID_LINK, 
				pRoom, 
				pSpawnLocation, 
				sSpawnMonsterByMonsterClassCallback,
				&tCallbackData);

			// how many monsters were spawned (note that we're using the count that
			// takes into account the "density value" of a single monster which accounts
			// for monsters that spawn other monsters, it is *not* the actual absolute 
			// number of units created
			int nNumSpawnedThisEntry = tCallbackData.nCountByDensityValue;			
			if (nNumSpawnedThisEntry> 0)
			{
				bTryAgain = FALSE;
			}

			// if we spawned a champion, don't create anymore
			if (tCallbackData.bSpawnedChampion == TRUE)
			{
				nDesiredMonsterQuality = INVALID_LINK;
			}

			// keep running total
			nNumSpawned += nNumSpawnedThisEntry;
			if( nMaxChampionCount > 0 )
			{
				nMaxChampionCount--;
			}
		}

	}

	if (nTries == 0 && bTryAgain == TRUE)
	{	
		ConsoleString( 
			CONSOLE_ERROR_COLOR, 
			L"Unable to spawn any monsters in room after '%d' tries - is level monsters setup?", 
			nMaxTries );	
	}

	return nNumSpawned;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_sRoomSpawnMonstersBySpawnClass(
	GAME* pGame,
	ROOM* pRoom,
	SPAWN_LOCATION *pSpawnLocation,
	DWORD dwSpawnClassCode,
	int nDesiredMonsterQuality,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint,
	ROOM_POPULATE_PASS ePass,
	int nMaxTries,
	ROOM_LAYOUT_GROUP *pSpawnPoint,
	int nMaxChampionCount)
{	

	
	//allow for the spawn class to run X times and also allow for a radius for the monsters to spawn in..
	int nMaxTimesToRunSpawnClass = MAX( 1, ((pSpawnPoint && pSpawnPoint->iSpawnClassExecuteXTimes)?pSpawnPoint->iSpawnClassExecuteXTimes:1) );	

	int nMonsterCount( 0 );
	for( int nRunSpawnClassXTimes = 0; nRunSpawnClassXTimes < nMaxTimesToRunSpawnClass; nRunSpawnClassXTimes++ )
	{
		nMonsterCount += sRoomSpawnMonstersBySpawnClass( pGame, pRoom, pSpawnLocation, dwSpawnClassCode, nDesiredMonsterQuality, pTrackedSpawnPoint, ePass, nMaxTries, pSpawnPoint, nMaxChampionCount );
	}
	
	return nMonsterCount;			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sChampionsAreNearby(
	GAME *pGame,
	ROOM *pRoom,
	SPAWN_LOCATION *pSpawnLocation)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( pSpawnLocation, "Expected spawn location" );
	const DRLG_DEFINITION *ptDRLGDef = RoomGetDRLGDef( pRoom );
	ASSERTX_RETFALSE( ptDRLGDef, "Expected DLRG definition" );
	const float flNearbyChampionDistanceSq = ptDRLGDef->flChampionZoneRadius * ptDRLGDef->flChampionZoneRadius;
	
	// get nearby rooms
	int nNumRooms = RoomGetNearbyRoomCount( pRoom );
	for (int i = -1; i < nNumRooms; ++i)
	{
		ROOM *pRoomOther = (i == -1) ? pRoom : RoomGetNearbyRoom( pRoom, i );
		
		// iterate monsters
		TARGET_TYPE eTargetType = TARGET_BAD;
		for (UNIT *pUnit = pRoomOther->tUnitList.ppFirst[ eTargetType ]; pUnit; pUnit = pUnit->tRoomNode.pNext )
		{
			if (MonsterIsChampion( pUnit ) == TRUE)
			{
			
				// is this monster "nearby" the spawn point
				float flDistSq = VectorDistanceSquared( pSpawnLocation->vSpawnDirection, UnitGetPosition( pUnit ) );
				if (flDistSq < flNearbyChampionDistanceSq)
				{
					return TRUE;
				}
				
			}
			
		}
				
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPickMonsterQualityForSpawn(
	GAME *pGame,
	ROOM *pRoom,
	SPAWN_LOCATION *pSpawnLocation)
{	
	ASSERTX_RETVAL( pGame, INVALID_LINK, "Expected game" );
	ASSERTX_RETVAL( pRoom, INVALID_LINK, "Expected room" );
	ASSERTX_RETVAL( pSpawnLocation, INVALID_LINK, "Expected spawn location" );

	// get spawn rate for level	
	LEVEL *pLevel = RoomGetLevel( pRoom );
	float flChampionSpawnRate = s_LevelGetChampionSpawnRate( pGame, pLevel );

	if ( GameIsVariant( pGame, GV_ELITE ) )
	{
		if( AppIsHellgate() )
		{
			flChampionSpawnRate += flChampionSpawnRate * ELITE_CHAMPION_PERCENT / 100.0f;
		}
		else
		{
			flChampionSpawnRate += flChampionSpawnRate * MYTHOS_ELITE_CHAMPION_PERCENT / 100.0f;
		}
	}

	// roll
	int nDesiredMonsterQuality = INVALID_LINK;
	if (RandGetFloat( pGame ) >= (1.0f - flChampionSpawnRate))
	{
	
		// we can only pick a quality if there are no specials "nearby"
		if (sChampionsAreNearby( pGame, pRoom, pSpawnLocation ) == FALSE)
		{
			nDesiredMonsterQuality = MonsterQualityPick( pGame, MQT_ANY );
		}
		
	}
	return nDesiredMonsterQuality;
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_sRoomSpawnMonster( 
	GAME * pGame, 
	ROOM * pRoom, 
	SPAWN_LOCATION * pSpawnLocation,
	ROOM_POPULATE_PASS ePass,
	int nTries)
{
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_NOMONSTERS) != 0 )
	{
		return 0;
	}
#endif

	// pick the quality for the spawn
	int nDesiredMonsterQuality = sPickMonsterQualityForSpawn( pGame, pRoom, pSpawnLocation );

	// spawn by class	
	int nNumSpawned = s_sRoomSpawnMonstersBySpawnClass(
		pGame, 
		pRoom, 
		pSpawnLocation, 
		(DWORD)-1, 
		nDesiredMonsterQuality,
		NULL,
		ePass,
		nTries,
		NULL,
		-1);
		
	return nNumSpawned;
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_sRoomSpawnSpecificMonsterByClass(
	GAME * pGame,
	ROOM * pRoom,
	SPAWN_LOCATION * pSpawnLocation,
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint,
	int nClass,
	int nDesiredMonsterQuality,
	ROOM_LAYOUT_GROUP *pSpawnPoint,
	ROOM_POPULATE_PASS ePass)
{


	if ( nClass != INVALID_LINK )
	{
		const UNIT_DATA *pMonsterData = MonsterGetData( NULL, nClass );
		if (pMonsterData)
		{
		
			// do not spawn monsters for passes other than our current one
			if (pMonsterData->eRoomPopulatePass != ePass)
			{
				return 0;
			}
			
			if (!UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_CAN_BE_CHAMPION))
			{
				nDesiredMonsterQuality = INVALID_LINK;
			}

			// create monster spec
			MONSTER_SPEC tSpec;
			tSpec.nClass = nClass;
			if( nDesiredMonsterQuality != INVALID_LINK )
			{
				tSpec.nMonsterQuality = nDesiredMonsterQuality;
			}
			tSpec.nExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
			tSpec.pRoom = pRoom;
			tSpec.vPosition= pSpawnLocation->vSpawnPosition;
			tSpec.vDirection = pSpawnLocation->vSpawnDirection;
			tSpec.pvUpDirection = &pSpawnLocation->vSpawnUp;

			// create the monster
			UNIT *pMonster = s_MonsterSpawn( pGame, tSpec );				
			if (pMonster)
			{			
				// track monsters we've created from this spawn point
				RoomTrackedSpawnPointAddUnit( pTrackedSpawnPoint, pMonster );

				// do post spawn point spawn logic
				sSpawnPointCreateUnitPostProcess( pMonster, pSpawnPoint );
			}

			return 1;  // spawned exactly one monster
			
		}
		
	}
	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_sRoomSpawnSpecificMonsterFromSpawnPoint( 
	GAME * pGame, 
	ROOM * pRoom, 
	SPAWN_LOCATION * pSpawnLocation,
	DWORD dwUnitType,
	DWORD dwCode,
	ROOM_LAYOUT_GROUP *pSpawnPoint,
	ROOM_POPULATE_PASS ePass)
{
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_ABSOLUTELYNOMONSTERS) != 0 )
	{
		return 0;
	}
#endif
	int nDesiredMonsterQuality = INVALID_LINK;
	if( AppIsTugboat() )
	{
		float flChampionSpawnRate = s_LevelGetChampionSpawnRate( pGame, RoomGetLevel( pRoom ) );
		// roll		
		if (RandGetFloat( pGame ) >= (1.0f - flChampionSpawnRate))
		{

			nDesiredMonsterQuality = MonsterQualityPick( pGame, MQT_ANY );
		}
	}
	// get the tracked spawn point data for this spawn point
	TRACKED_SPAWN_POINT *pTrackedSpawnPoint = RoomGetOrCreateTrackedSpawnPoint( pGame, pRoom, pSpawnPoint );
	ASSERTX_RETZERO( pTrackedSpawnPoint, "Expected tracked spawn point" );

	// do nothing if monster(s) from this spawn point are still around and alive
	if (pTrackedSpawnPoint->nNumUnits > 0)
	{
		for (int i = 0; i < pTrackedSpawnPoint->nNumUnits; ++i)
		{
			UNITID idUnitExisting = pTrackedSpawnPoint->pidUnits[ i ];
			if (idUnitExisting != INVALID_ID)
			{
				UNIT *pUnitExisting = UnitGetById( pGame, idUnitExisting);
				if (pUnitExisting && IsUnitDeadOrDying( pUnitExisting ) == FALSE)
				{
					return 0;
				}
			}
		}
	}

	// ok, we've passed the tracked spawns so we won't have duplicates, reset the tracker
	RoomTrackedSpawnPointReset( pGame, pTrackedSpawnPoint );
	
	switch(dwUnitType)
	{
	
		//----------------------------------------------------------------------------
		case ROOM_SPAWN_MONSTER:
		{
						
			int nClass = MonsterGetByCode( pGame, dwCode );
			if(s_sRoomSpawnSpecificMonsterByClass(pGame, pRoom, pSpawnLocation, pTrackedSpawnPoint, nClass, nDesiredMonsterQuality, pSpawnPoint, ePass) == 1)
			{
				return 1;
			}
			break;
		}

		//----------------------------------------------------------------------------
		case ROOM_SPAWN_INTERACTABLE:
		{
			int nClass = s_LevelSelectRandomSpawnableInteractableMonster( pGame, RoomGetLevel(pRoom), NULL );
			if(s_sRoomSpawnSpecificMonsterByClass(pGame, pRoom, pSpawnLocation, pTrackedSpawnPoint, nClass, nDesiredMonsterQuality, pSpawnPoint, ePass) == 1)
			{
				return 1;
			}
			break;
		}

		//----------------------------------------------------------------------------
		case ROOM_SPAWN_SPAWN_CLASS:
		default:
		{
			return s_sRoomSpawnMonstersBySpawnClass(
								pGame, 
								pRoom, 
								pSpawnLocation, 
								dwCode,
								nDesiredMonsterQuality,
								pTrackedSpawnPoint,
								ePass,
								IMPORTANT_MONSTER_SPAWN_TRIES,
								pSpawnPoint,
								1);

			break;
		}
	}

	return 0;  // nothing spawned
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static UNIT *s_sRoomSpawnObjectFromSpawnPoint( 
	GAME *pGame, 
	ROOM *pRoom, 
	SPAWN_LOCATION *pSpawnLocation,
	DWORD dwObjectCode,
	ROOM_POPULATE_PASS ePass, 
	ROOM_LAYOUT_GROUP *pSpawnPoint)
{
	UNIT *pSpawned = NULL;
	
	if ( dwObjectCode )
	{
		int nClass = ObjectGetByCode( pGame, dwObjectCode );
		if ( nClass != INVALID_ID )
		{
			UNIT_DATA * pObjectData = (UNIT_DATA *) ObjectGetData( pGame, nClass );
			if (pObjectData)
			{
						
				// ignore objects that are not for the pass requested
				if (pObjectData->eRoomPopulatePass != ePass)
				{
					return pSpawned;
				}
				
				// non-trigger logic
				if ( !UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_TRIGGER))
				{

					// ignore objects that are marked as can't spawn at all
					if ( ! UnitDataTestFlag( pObjectData, UNIT_DATA_FLAG_SPAWN ) )
					{
						return pSpawned;
					}

					// do a spawn chance
					if ( pObjectData->nSpawnPercentChance < 100 && 
						(int)RandGetNum(pGame, 100) >= pObjectData->nSpawnPercentChance )
					{
						return pSpawned;
					}

				}

				// there are some objects we do not re-populate again if we have already
				// populated this room once before
				if (RoomTestFlag( pRoom, ROOM_POPULATED_ONCE_BIT ))
				{
					if (s_UnitClassCanBeDepopulated(pObjectData) == FALSE)
					{
						return NULL;
					}

					// do a spawn chance
					if ( pObjectData->nRespawnPercentChance < 100 && 
						(int)RandGetNum(pGame, 100) >= pObjectData->nRespawnPercentChance )
					{
						return pSpawned;
					}
					// if on respawn this can be something else, evaluate that now 
					// note that this only happens on repopulation
					if( pObjectData->nRespawnSpawnClass != INVALID_LINK )
					{
						int nTries = 64;
						SPAWN_ENTRY tSpawns[MAX_SPAWNS_AT_ONCE];	
						int nSpawnCount = 0;
						do 
						{

							// evaluate a random spawn class in the level
							nSpawnCount = SpawnClassEvaluate( 
								pGame, 
								LevelGetID( RoomGetLevel( pRoom ) ), 
								pObjectData->nRespawnSpawnClass, 
								-1, 
								tSpawns);
						}
						while (nSpawnCount == 0 && nTries-- > 0);

						if (nSpawnCount > 0)
						{	
							// pick one of the monster classes available
							int nPick = RandGetNum( pGame, 0, nSpawnCount - 1 );
							ASSERTX( nPick >= 0 && nPick < nSpawnCount, "Invalid pick respawning object" );

							SPAWN_ENTRY* pSpawn = &tSpawns[ nPick ];		
							nClass = pSpawn->nIndex;
						}
					}
					
				}
							
				OBJECT_SPEC tSpec;
				tSpec.nClass = nClass;
				tSpec.nLevel = RoomGetMonsterExperienceLevel( pRoom );
				tSpec.pRoom = pRoom;
				tSpec.vPosition = pSpawnLocation->vSpawnPosition;
				tSpec.pvFaceDirection = &pSpawnLocation->vSpawnDirection;
				pSpawned = s_ObjectSpawn( pGame, tSpec );

				if( pSpawned && AppIsTugboat() && pSpawnPoint )
				{
					sSpawnPointCreateUnitPostProcess( pSpawned, pSpawnPoint );
				}

				if ( pObjectData->nMonsterToSpawn != INVALID_ID )
				{
					SPAWN_ENTRY tSpawn;
					tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
					tSpawn.nIndex = pObjectData->nMonsterToSpawn;
					tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;

					// setup callback data
					if( pSpawnPoint &&
						AppIsTugboat() )
					{
						BY_SPAWN_CLASS_DATA tCallbackData;
						tCallbackData.pSpawnPoint = pSpawnPoint;
						tCallbackData.pTrackedSpawnPoint = NULL;
						tCallbackData.nCountByDensityValue = 0;

						s_RoomSpawnMonsterByMonsterClass(
							pGame, 
							&tSpawn, 
							INVALID_LINK, 
							pRoom, 
							pSpawnLocation, 
							sSpawnMonsterByMonsterClassCallback,	//for tugboat
							&tCallbackData);						//for tugboat
					}
					else
					{
						BY_SPAWN_CLASS_DATA tCallbackData;
						tCallbackData.pSpawnPoint = pSpawnPoint;
						tCallbackData.pTrackedSpawnPoint = NULL;
						tCallbackData.nCountByDensityValue = 0;

						s_RoomSpawnMonsterByMonsterClass(
							pGame, 
							&tSpawn, 
							INVALID_LINK, 
							pRoom, 
							pSpawnLocation, 
							NULL,
							NULL);
					}
				}

			}
						
		}
		
	}
	
	return pSpawned;
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//spawns an object in a room
UNIT * s_RoomSpawnObject( 
	GAME *pGame, 
	ROOM *pRoom, 
	SPAWN_LOCATION *pSpawnLocation,
	DWORD dwObjectCode )
{	
	ASSERTX_RETNULL( pGame, "Game not found in Spawn Object" );
	ASSERTX_RETNULL( pRoom, "Room not found in Spawn Object" );
	ASSERTX_RETNULL( pSpawnLocation, "Spawn Location not found in Spawn Object" );
	ASSERTX_RETNULL( dwObjectCode >= 0, "Invalid object to create" );
	return s_sRoomSpawnObjectFromSpawnPoint( pGame,
											  pRoom,
											  pSpawnLocation,
											  dwObjectCode,
											  RPP_CONTENT,
											  NULL );
	
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void s_RoomPopulateSpawnPoint(
	GAME *pGame,
	ROOM *pRoom,
	ROOM_LAYOUT_GROUP *pSpawnPoint,
	ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation,
	ROOM_POPULATE_PASS ePass)
{

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pRoom, pSpawnPoint, pSpawnOrientation );

	switch (pSpawnPoint->dwUnitType)
	{

		//----------------------------------------------------------------------------
		case ROOM_SPAWN_MONSTER:
		case ROOM_SPAWN_SPAWN_CLASS:
		{
			if ( sSpawnCodeIsValid(pSpawnPoint) )
			{
				s_sRoomSpawnSpecificMonsterFromSpawnPoint( 
					pGame, 
					pRoom, 
					&tLocation, 
					pSpawnPoint->dwUnitType, 
					pSpawnPoint->dwCode,
					pSpawnPoint,
					ePass);
			}
			else if ( AppIsHellgate() )
			{

				// only do random monster spawning if the room allows it
				if (RoomAllowsMonsterSpawn( pRoom ))
				{

					// not a specific thing to spawn, it will has a random chance of
					// actually spawning anything at all
					const DRLG_DEFINITION * drlg_definition = RoomGetDRLGDef( pRoom );
					if (drlg_definition->flMarkerSpawnChance != 0.0f)
					{
						if (RandGetFloat(pGame) < drlg_definition->flMarkerSpawnChance)
						{
							s_sRoomSpawnMonster( 
								pGame, 
								pRoom, 
								&tLocation, 
								ePass, 
								IMPORTANT_MONSTER_SPAWN_TRIES);
						}

					}

				}

			}

			break;

		}

		//----------------------------------------------------------------------------	
		case ROOM_SPAWN_INTERACTABLE:
			{

				s_sRoomSpawnSpecificMonsterFromSpawnPoint( 
					pGame, 
					pRoom, 
					&tLocation, 
					pSpawnPoint->dwUnitType, 
					pSpawnPoint->dwCode,
					pSpawnPoint,
					ePass);

				break;

			}

		//----------------------------------------------------------------------------	
		case ROOM_SPAWN_OBJECT:
		{
			s_sRoomSpawnObjectFromSpawnPoint( pGame, pRoom, &tLocation, pSpawnPoint->dwCode, ePass, pSpawnPoint );
			break;
		}

		//----------------------------------------------------------------------------	
		default:
		{
			WARNX( 0, "Invalid Unit Type in spawn code" );
			break;
		}

	}

	

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void s_sRoomPopulateSpawnPointList(
	ROOM *pRoom,
	ROOM_POPULATE_PASS ePass)
{
	ASSERTX_RETURN( pRoom, "Expected room " );
	
	// get game
	GAME *pGame = RoomGetGame( pRoom );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// do nothing if room is already populated for this pass
	if (pRoom->bPopulated[ ePass ] == TRUE)
	{
		return;
	}
	
	// create objects that are marked as create on level activation	
	ROOM_LAYOUT_SELECTION *pLayoutSpawnpoints = RoomGetLayoutSelection( pRoom, ROOM_LAYOUT_ITEM_SPAWNPOINT );
	ASSERT_RETURN(pLayoutSpawnpoints);
	ROOM_LAYOUT_GROUP **pSpawnPoints = pLayoutSpawnpoints->pGroups;
	ASSERT_RETURN(pSpawnPoints || pLayoutSpawnpoints->nCount == 0);
	ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientations = pLayoutSpawnpoints->pOrientations;
	ASSERT_RETURN(pSpawnOrientations || pLayoutSpawnpoints->nCount == 0);
	for (int i = 0; i < pLayoutSpawnpoints->nCount; ++i)
	{
		ROOM_LAYOUT_GROUP *pSpawnPoint = pSpawnPoints[ i ];
		ASSERT_CONTINUE(pSpawnPoint);
		ROOM_LAYOUT_SELECTION_ORIENTATION *pSpawnOrientation = &pSpawnOrientations[ i ];
		ASSERT_CONTINUE(pSpawnOrientation);
		
		s_RoomPopulateSpawnPoint( pGame, pRoom, pSpawnPoint, pSpawnOrientation, ePass );
	}

	// room is now populated for this pass
	pRoom->bPopulated[ ePass ] = TRUE;
	
}

//----------------------------------------------------------------------------
struct ROOM_COUNT_MONSTER_DATA
{
	int nCount;
	ROOM_MONSTER_COUNT_TYPE eType;
	
	ROOM_COUNT_MONSTER_DATA::ROOM_COUNT_MONSTER_DATA( void )
		:	nCount( 9 ),
			eType( RMCT_ABSOLUTE )
		{ }
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sCountMonsters(
	UNIT *pUnit,
	void *pCallbackData)
{
	ROOM_COUNT_MONSTER_DATA *pData = (ROOM_COUNT_MONSTER_DATA *)pCallbackData;	
	if (UnitIsA( pUnit, UNITTYPE_MONSTER ))
	{
		int nCountValue = 1;  // by default 1 for RMCT_ABSOLUTE or any other unknown type
		if (pData->eType == RMCT_DENSITY_VALUE_OVERRIDE)
		{
			const UNIT_DATA *pUnitData = UnitGetData( pUnit );
			nCountValue = pUnitData->nDensityValueOverride;
		}
		pData->nCount += nCountValue;
	}
	return RIU_CONTINUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int RoomGetMonsterCount(
	ROOM *pRoom,
	ROOM_MONSTER_COUNT_TYPE eType, /*= RMCT_ABSOLUTE*/	
	const TARGET_TYPE *peTargetTypes /*= NULL*/)
{
	
	// setup callback data
	ROOM_COUNT_MONSTER_DATA tData;
	tData.nCount = 0;
	tData.eType = eType;
	
	// count units
	RoomIterateUnits( pRoom, peTargetTypes, sCountMonsters, &tData );
	
	// return result
	return tData.nCount;
	
}

//-------------------------------------------------------------------------------------------------
struct COUNT_UNITS_OF_TYPE_DATA
{
	int nCount;
	int nUnitType;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sCountUnitsOfType(
	UNIT *pUnit,
	void *pCallbackData)
{
	COUNT_UNITS_OF_TYPE_DATA *pData = (COUNT_UNITS_OF_TYPE_DATA *)pCallbackData;
	if (UnitIsA( pUnit, pData->nUnitType ))
	{
		pData->nCount++;
	}
	return RIU_CONTINUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int RoomCountUnitsOfType(
	ROOM *pRoom,
	int nUnitType)
{
	ASSERTX_RETZERO( pRoom, "Expected room" );
	ASSERTX_RETZERO( nUnitType != INVALID_LINK, "Expected unit type" );

	// setup callback data	
	COUNT_UNITS_OF_TYPE_DATA tData;
	tData.nCount = 0;
	tData.nUnitType = nUnitType;

	// count
	RoomIterateUnits( pRoom, NULL, sCountUnitsOfType, &tData );
	return tData.nCount;
	
}	

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int c_RoomGetCritterCount(
	ROOM *pRoom)
{
	return RoomCountUnitsOfType( pRoom, UNITTYPE_CRITTER );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sCountObjects(
	UNIT *pUnit,
	void *pCallbackData)
{
	if (UnitIsA( pUnit, UNITTYPE_OBJECT ))
	{
		int *pnNumObjects = (int *)pCallbackData;
		*pnNumObjects = *pnNumObjects + 1;
	}
	return RIU_CONTINUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int s_RoomGetObjectCount(
	ROOM *pRoom)
{
	int nNumObjects = 0;
	RoomIterateUnits( pRoom, NULL, sCountObjects, &nNumObjects );
	return nNumObjects;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int s_sRoomDoMonsterSpawnAtRandomNode(
	GAME *pGame,
	ROOM *pRoom,
	ROOM_POPULATE_PASS ePass,
	int nTries)
{

	// TRAVIS: FIXME - should look in surrounding rooms?
	// COLIN: Sometimes we are only concentrating on this room during monster
	// population ... if you want to look at surrounding rooms, make it an option
	// only for the case in which you really want it
	// It's possible that the room is fully and completely occupied.  Exit early in that case

	// pick a random node to spawn at that is unoccupied and allows monster spawns
	DWORD dwRandomNodeFlags = 0;
	SETBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN );
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, dwRandomNodeFlags );	
	if(nNodeIndex < 0)
	{
		return 0;  // no suitable spawn nodes at all found, forget it
	}

	// init location
	SPAWN_LOCATION tLocation;
	float fAngle = RandGetFloat(pGame, -180.0f, 180.0f);
	SpawnLocationInit( &tLocation, pRoom, nNodeIndex, &fAngle );
	
	// do a spawn at this location
	return s_sRoomSpawnMonster( pGame, pRoom, &tLocation, ePass, nTries );
							
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sRoomDoDRLGSpawnAtRandomNode(
	GAME *pGame,
	ROOM *pRoom,
	ROOM_POPULATE_PASS ePass,
	const DRLG_SPAWN * pDRLGSpawn )
{
	ASSERTX_RETZERO( pDRLGSpawn != NULL, "Invalid drlg spawn pointer" );

	// pick a random node to spawn at that is unoccupied, yeah this is some hefty
	// work to do every time we spawn and should probably be optimized
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );

	// init location
	SPAWN_LOCATION tLocation;
	float fAngle = RandGetFloat(pGame, -180.0f, 180.0f);
	SpawnLocationInit( &tLocation, pRoom, nNodeIndex, &fAngle );

	//const DRLG_DEFINITION * drlg_definition = RoomGetDRLGDef( pRoom );
	//ASSERT_RETZERO(drlg_definition);
	//const DRLG_SPAWN *pDRLGSpawn = &drlg_definition->tDRLGSpawn[ eDRLGSpawnType ];
	if(pDRLGSpawn->nSpawnClass >= 0)
	{
		SPAWN_CLASS_DATA * pSpawnClass = (SPAWN_CLASS_DATA*)ExcelGetData(pGame, DATATABLE_SPAWN_CLASS, pDRLGSpawn->nSpawnClass);
		ASSERT_RETZERO(pSpawnClass);

		// do a spawn at this location
		return s_sRoomSpawnMonstersBySpawnClass(
			pGame, 
			pRoom, 
			&tLocation, 
			pSpawnClass->wCode, 
			INVALID_LINK,
			NULL,
			ePass,
			RANDOM_MONSTER_SPAWN_TRIES,
			NULL,
			-1);
			
	}
	
	return 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float s_sRoomGetMonsterRoomDensity(
	ROOM * pRoom)
{
	SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
	ASSERTX_RETVAL( pSubLevel, -1.0f, "Expected sublevel" );

	SUBLEVELID idSubLevel = SubLevelGetId( pSubLevel );

	// bail out early if monsters aren't allowed to spawn in this room
	if (RoomAllowsMonsterSpawn( pRoom ) == FALSE)
	{
		return -1.0f;
	}

	// see if this sublevel allows monster spawns by distributions
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETVAL( pLevel, -1.0f, "Expected DRLG level" );
	if (s_SubLevelAllowsMonsterDistribution( pLevel, idSubLevel ) == FALSE)
	{
		return -1.0;
	}

	// get the drlg that was used to place this room
	const DRLG_DEFINITION *pDRLGDef = RoomGetDRLGDef( pRoom );
	ASSERTX_RETVAL( pDRLGDef, -1.0f, "Expected DRLG level" );

	// how many monsters will we spawn	
	float flMonsterRoomDensity( pDRLGDef->flMonsterRoomDensityPercent );
	if( AppIsTugboat() )
	{
		ASSERTX_RETVAL( pLevel->m_LevelAreaOverrides.fMonsterDensityMult < 10, -1.0f, "Monster density is too high in overrides for Level Area." );
		flMonsterRoomDensity *= pLevel->m_LevelAreaOverrides.fMonsterDensityMult;
	}
	else if (AppIsHellgate())
	{
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( pLevel );
		ASSERTX_RETVAL( pLevelDef, -1.0f, "Expected level def" );
		flMonsterRoomDensity *= pLevelDef->flMonsterRoomDensityMultiplier;
	}
	
	return flMonsterRoomDensity;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_RoomGetEstimatedMonsterCount(
	ROOM * pRoom)
{
	ASSERTX_RETZERO( pRoom, "Expected room" );

	float flMonsterRoomDensity = s_sRoomGetMonsterRoomDensity(pRoom);
	if(flMonsterRoomDensity <= 0.0f)
		return 0;

	flMonsterRoomDensity /= 100.0f;  // bring into 0.0 to 1.0 range

	// which node set is this room using
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );
	int nNodeCount = pRoomPathNodes ? pRoomPathNodes->nPathNodeCount : 0;
	if(nNodeCount > 0)
	{
		return (int)(flMonsterRoomDensity * nNodeCount);
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sAddRoomDistributionToCounts(
	ROOM *pRoom,
	int nCurrentSpawnCountInRoom,
	float flRoomDensityPercent,
	int nCurrentCountInSubLevel,
	int *pnDesiredCountInSubLevel)
{
	ASSERTX_RETZERO( pRoom, "Expected room" );
	GAME *pGame = RoomGetGame( pRoom );
	int nNumAllowedToSpawnThisRoom = 0;

	// rooms that allow no monster spawns do nothing, maybe use a paramater flag for this
	// one day if we want
	if (RoomAllowsMonsterSpawn( pRoom ) == FALSE)
	{
		return 0;
	}
		
	// how many monsters should be here
	flRoomDensityPercent /= 100.0f;  // bring into 0.0 to 1.0 range
	ASSERTX_RETZERO( flRoomDensityPercent >= 0.0f && flRoomDensityPercent <= 1.0f, "Monster room density out of range" );
	
	// must have a density
	if (flRoomDensityPercent > 0.0f)
	{

		// which node set is this room using
		ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );

		// how many nodes are here
		int nNodeCount = pRoomPathNodes ? pRoomPathNodes->nPathNodeCount : 0;
		
		// must have path nodes
		if (nNodeCount > 0)
		{
						
			// incorporate the monsters spawned in the sublevel and in the room thus far
			nCurrentSpawnCountInRoom += nCurrentCountInSubLevel;

			// given the desired room density, what is the max or ideal # of monsters we
			// would like to see in this room
			int nRoomLimit = 0;
				
			if( AppIsHellgate() )
			{
				nRoomLimit = (int)(flRoomDensityPercent * nNodeCount);
			}
			else
			{
				for( int i = 0; i < nNodeCount; i++ )
				{
					if( RandGetFloat( pGame, 100 ) < flRoomDensityPercent * 100 )
					{
						nRoomLimit++;
					}
				}
			}

			// add the monster limit for this room to the total desired monsters in the level
			*pnDesiredCountInSubLevel = *pnDesiredCountInSubLevel + nRoomLimit;

			// how many monsters want we make
 			nNumAllowedToSpawnThisRoom = *pnDesiredCountInSubLevel - nCurrentSpawnCountInRoom;
			if (nNumAllowedToSpawnThisRoom > 0)
			{

				// sometimes we can't find a place in a room to place 
				// required monsters (no room, no spawn areas, etc) ... so we keep this running
				// total of desired monsters ... but we must never let it get over the monster
				// limit for this room or the monsters will bunch up in rooms that are good
				// for spawning immediately after failing to spawn in a previous room
				// but to keep the density of the monsters correct, there is a max
				// number of monsters we will allow in a room
				if (nNumAllowedToSpawnThisRoom > nRoomLimit)
				{

					// we need to cap the monster we're allowing in this room, 
					int nOverflow = nNumAllowedToSpawnThisRoom - nRoomLimit;
					nNumAllowedToSpawnThisRoom = nRoomLimit;
					
					// however, we will allow this room to squeak in extra monsters
					// because we're overflowing from previous rooms and they need
					// to go somewhere in the level ... we could be more intelligent about
					// this and put the monster in not just this room but in any untouched
					// room in the level to spread these extras out more evenly
					int nMaxExtras = RandGetNum( pGame, 1, 2 );
					int nNumExtras = MIN( nMaxExtras, nOverflow );
					nNumAllowedToSpawnThisRoom += nNumExtras;
					
				}

			}

		}
			
	}
		
	// return the monsters that we want to try to spawn in this room
	return nNumAllowedToSpawnThisRoom;
	
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sGetDRLGSpawnCurrentCount(
	ROOM *pRoom,
	DRLG_SPAWN_TYPE eDRLGSpawnType)
{
	ASSERTX_RETZERO( pRoom, "Expected room" );
	ASSERTX_RETZERO( eDRLGSpawnType >= 0 && eDRLGSpawnType < NUM_DRLG_SPAWN_TYPE, "Invalid drlg spawn type" );
	GAME *pGame = RoomGetGame( pRoom );
	
	switch (eDRLGSpawnType)
	{
		//-------------------------------------------------------------------------------------------------	
		case DRLG_SPAWN_TYPE_CRITTER:
		{
			ASSERTX_RETZERO( IS_CLIENT( pGame ), "Client only" );
			return c_RoomGetCritterCount( pRoom );
		}
		//-------------------------------------------------------------------------------------------------		
		case DRLG_SPAWN_TYPE_ENVIRONMENT:
		{
			return RoomCountUnitsOfType( pRoom, UNITTYPE_ENVIRONMENT_MONSTER );
		}
		//-------------------------------------------------------------------------------------------------		
		default:
		{
			break;
		}
	}
	return 0;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sRoomPopulateDRLGSpawnByDistribution(
	GAME *pGame,
	ROOM *pRoom,
	ROOM_POPULATE_PASS ePass,
	DRLG_SPAWN_TYPE eDRLGSpawnType,
	DRLG_SPAWN * pSpawnOverride = NULL )
{
	const DRLG_SPAWN * pDRLGSpawn;
	if ( !pSpawnOverride )
	{
		const DRLG_DEFINITION * pDRLGDef = RoomGetDRLGDef( pRoom );
		pDRLGSpawn = &pDRLGDef->tDRLGSpawn[ eDRLGSpawnType ];
	}
	else
	{
		pDRLGSpawn = pSpawnOverride;
	}
	if (pDRLGSpawn->nSpawnClass != INVALID_LINK &&
		pDRLGSpawn->flRoomDensityPercent > 0.0f)
	{
	
		SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
		SPAWN_TRACKER *pTracker = &pSubLevel->tSpawnTracker[ eDRLGSpawnType ];

		// how many things are in this room already
		int nCurrentCount = sGetDRLGSpawnCurrentCount( pRoom, eDRLGSpawnType );
		
		// how many critters to spawn
		int nNumToSpawn = sAddRoomDistributionToCounts( 
			pRoom, 
			nCurrentCount,
			pDRLGSpawn->flRoomDensityPercent,
			pTracker->nSpawnedCount,
			&pTracker->nDesiredCount);

		// spawn all the monsters	
		int nTries = 32;
		int nSpawnedCount = 0;			
		while (nSpawnedCount < nNumToSpawn && nTries-- > 0)
		{
			nSpawnedCount += sRoomDoDRLGSpawnAtRandomNode( pGame, pRoom, ePass, pDRLGSpawn );
		}
		
		// add count to running total in sub level tracker
		pTracker->nSpawnedCount += nSpawnedCount;

	}
			
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void s_sRoomPopulateMonstersByDistribution(
	ROOM *pRoom,
	ROOM_POPULATE_PASS ePass)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( ePass == RPP_CONTENT, "Only support monster distribution content population pass" );
	GAME *pGame = RoomGetGame( pRoom );
	SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
	ASSERTX_RETURN( pSubLevel, "Expected sublevel" );

	float flMonsterRoomDensity = s_sRoomGetMonsterRoomDensity(pRoom);
	if(flMonsterRoomDensity <= 0.0f)
		return;

	// how many monsters are already placed in the room already (via node spawn points)
	ROOM_MONSTER_COUNT_TYPE eCountType = RMCT_DENSITY_VALUE_OVERRIDE;
	TARGET_TYPE eTargetTypes[] = {TARGET_BAD, TARGET_INVALID };	
	int nCurrentMonsterCount = RoomGetMonsterCount( pRoom, eCountType, eTargetTypes );

	int nMonstersToSpawn = sAddRoomDistributionToCounts( 
		pRoom, 
		nCurrentMonsterCount, 
		flMonsterRoomDensity,
		pSubLevel->nMonsterCount,
		&pSubLevel->nDesiredMonsters);

	// spawn all the monsters
	int nMaxTries = nMonstersToSpawn;
	int nSpawnedCount = 0;
	while (nMaxTries-- && nSpawnedCount < nMonstersToSpawn)
	{	
		nSpawnedCount += s_sRoomDoMonsterSpawnAtRandomNode( pGame, pRoom, ePass, RANDOM_MONSTER_SPAWN_TRIES );
	}
	
	// add count of spawned monsters to running count in level
	pSubLevel->nMonsterCount += nSpawnedCount;

	// do environmental monster spawns by distribution
	DRLG_SPAWN * pSpawnOverride = NULL;
	DRLG_SPAWN tSpawnOverride;
	LEVEL * pLevel = RoomGetLevel( pRoom );
	if ( pLevel )
	{
		const LEVEL_DRLG_CHOICE * pChoice = DungeonGetLevelDRLGChoice( pLevel );
		if ( pChoice )
		{
			if ( ( pChoice->nEnvSpawnClass != INVALID_LINK ) && ( pChoice->fEnvRoomDensityPercent > 0.0f ) )
			{
				tSpawnOverride.nSpawnClass = pChoice->nEnvSpawnClass;
				tSpawnOverride.flRoomDensityPercent = pChoice->fEnvRoomDensityPercent;
				pSpawnOverride = &tSpawnOverride;
			}
		}
	}
	sRoomPopulateDRLGSpawnByDistribution( pGame, pRoom, ePass, DRLG_SPAWN_TYPE_ENVIRONMENT, pSpawnOverride );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void s_sRoomPopulateTreasure( 
	GAME *pGame,
	ROOM *pRoom,
	UNIT *pActivator,
	int nTreasureClass)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pActivator, "Expected activator" );

	// get level modifier for gold amount
	LEVEL *pLevel = RoomGetLevel( pRoom );
	int nLevelModifier = LevelGetMonsterExperienceLevel( pLevel );
	if (nLevelModifier <= 0)
	{
		nLevelModifier = 1;
	}

	ITEM_SPEC tSpec;

	// make a bunch of gold for now at many of the path nodes
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );
	for (int i = 0; i < pRoomPathNodes->nPathNodeCount; ++i)
	{
		if (s_RoomNodeIsBlocked( pRoom, i, 0, NULL ) == NODE_FREE)
		{
			ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( pRoom, i );

			// init spec to clean again
			ItemSpawnSpecInit( tSpec );

			// roll for spawn
			int nItemClass = INVALID_LINK;
			if ((int)RandGetNum( pGame, 0, 100 ) < PASSAGEWAY_MONEY_CHANCE_PER_PATH_NODE)
			{
				nItemClass = GlobalIndexGet( GI_ITEM_MONEY );

				int nMoneyBase = RandGetNum( 
					pGame, 
					PASSAGEWAY_MONEY_AMOUNT_LEVEL_MULTIPLIER_MIN,
					PASSAGEWAY_MONEY_AMOUNT_LEVEL_MULTIPLIER_MAX);
				tSpec.nMoneyAmount = nLevelModifier * nMoneyBase;
				tSpec.nRealWorldMoneyAmount = 0;

			}
			else if ((int)RandGetNum( pGame, 0, 100 ) < PASSAGEWAY_ITEM_CHANCE_PER_PATH_NODE)
			{
				// can't do this yet ... treasure API doesn't allow it yet -Colin
				//s_TreasureSpawnClass( pActivator, nTreasureClass, NULL, NULL );
			}

			// spawn it
			if (nItemClass != INVALID_LINK)
			{

				// set class
				tSpec.nItemClass = nItemClass;

				// get world location
				VECTOR vPosition = RoomPathNodeGetWorldPosition( pGame, pRoom, pNode );
				tSpec.pvPosition = &vPosition;
				tSpec.pRoom = pRoom;

				// set to spawn in world
				SETBIT(tSpec.dwFlags, ISF_PLACE_IN_WORLD_BIT);
				SETBIT(tSpec.dwFlags, ISF_NO_FLIPPY_BIT);

				// create it
				s_ItemSpawn( pGame, tSpec, NULL );

			}

		}

	}

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )


#if !ISVERSION( CLIENT_ONLY_VERSION )
//-------------------------------------------------------------------------------------------------
// This function adds to a unit a register for when it dies to spawn the treasure class.
//It's added here on room populate. If it can't find the monster to drop the item,
//then it will create the default object - which should be a chest or something of that sort
//which it then creates the register and spawns the treasure from that.
//-------------------------------------------------------------------------------------------------
static void s_sRoomCreateTreasureSpawn( ROOM *pRoom )
{
	//room must be populated.
	if( RoomTestFlag( pRoom, ROOM_POPULATED_BIT ) == 0 )
	{
		return;
	}
		
	if( pRoom->nSpawnTreasureFromUnit.nTreasure != INVALID_ID &&
		pRoom->nSpawnTreasureFromUnit.nTargetType != INVALID_ID )
	{
		GAME *pGame = RoomGetGame( pRoom );
		// iterate units in room
		ROOM_FIND_FIRST_OF_DATA tData;		
		tData.nUnitTypeOf = UNITTYPE_MONSTER;		
		TARGET_TYPE eTargetTypes[] = { pRoom->nSpawnTreasureFromUnit.nTargetType, TARGET_INVALID };
		RoomIterateUnits( pRoom, eTargetTypes, sFindFirstOf, &tData );
		if( tData.pResult != NULL &&
			UnitDataTestFlag( tData.pResult->pUnitData, UNIT_DATA_FLAG_NO_RANDOM_LEVEL_TREASURE ) == FALSE ) //this monster can't spawn random treasure
		{
			//register event call back on the monsters death to spawn the item
			s_LevelRegisterUnitToDropOnDie( pGame, 
											tData.pResult,
											pRoom->nSpawnTreasureFromUnit.nTreasure );
		}
		else
		{
			//ok so we failed to find a unit to spawn the treasure class for. 
			//now lets try creating the default Object...

			ASSERT_RETURN( pRoom->nSpawnTreasureFromUnit.nObjectClassOverRide != INVALID_ID );
			// and spawn brain
			int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, 0 );
			ASSERTX_RETURN( nNodeIndex >= 0, "Unable to find location to spawn object for level treasure spawn" );			
			// init location
			SPAWN_LOCATION tLocation;
			SpawnLocationInit( &tLocation, pRoom, nNodeIndex );
			//create the object now
			OBJECT_SPEC objectSpec;
			objectSpec.nClass = pRoom->nSpawnTreasureFromUnit.nObjectClassOverRide;
			objectSpec.pRoom = pRoom;
			objectSpec.vPosition = tLocation.vSpawnPosition;
			objectSpec.pvFaceDirection = &tLocation.vSpawnDirection;
			UNIT * pObject = s_ObjectSpawn( pGame, objectSpec ); //creating object
			ASSERT_RETURN( pObject);
			//register event call back on the monsters death to spawn the item
			s_LevelRegisterUnitToDropOnDie( pGame, 
											pObject,
											pRoom->nSpawnTreasureFromUnit.nTreasure );

		}
	}

}

BOOL s_RoomSetupTreasureSpawn( ROOM *pRoom, 
							   int nTreasure,
							   TARGET_TYPE nType,
							   int nObjectSpawnClassFallback )
{
	ASSERT_RETFALSE( pRoom );
	ASSERT_RETFALSE( nTreasure != INVALID_ID );
	ASSERT_RETFALSE( pRoom->nSpawnTreasureFromUnit.nTreasure == INVALID_ID );
	pRoom->nSpawnTreasureFromUnit.nTreasure = nTreasure;
	pRoom->nSpawnTreasureFromUnit.nTargetType = nType;
	pRoom->nSpawnTreasureFromUnit.nObjectClassOverRide = nObjectSpawnClassFallback;	
	s_sRoomCreateTreasureSpawn( pRoom );
	return TRUE;
}

#endif
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void s_sRoomPopulate( 
	ROOM *pRoom,
	UNIT *pActivator)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pRoom->pDefinition, "Expected room definition" );

	// do nothing if already populated
	if (RoomTestFlag( pRoom, ROOM_POPULATED_BIT ))
	{
		return;
	}	

	// spawn setup objects
	s_sRoomPopulateSpawnPointList( pRoom, RPP_SETUP );
	
	// spawn content
	s_sRoomPopulateSpawnPointList( pRoom, RPP_CONTENT );

	// do random monster spawns by distribution
	GAME *pGame = RoomGetGame( pRoom );	
	s_sRoomPopulateMonstersByDistribution( pRoom, RPP_CONTENT );

	// populate with booty if it's a treasure level, a player is required
	if (pActivator)
	{
		LEVEL *pLevel = RoomGetLevel( pRoom );
		if (pLevel->m_nTreasureClassInLevel != INVALID_LINK && RoomIsInDefaultSubLevel(pRoom))
		{
			s_sRoomPopulateTreasure( pGame, pRoom, pActivator, pLevel->m_nTreasureClassInLevel );
		}
	}
		
	// room is now currently populated populated
	RoomSetFlag( pRoom, ROOM_POPULATED_BIT );
	
	// room has been populated at least once
	RoomSetFlag( pRoom, ROOM_POPULATED_ONCE_BIT );	

	// get the monster count
	for (int i = 0; i < RMCT_NUM_TYPES; ++i)
	{
		ROOM_MONSTER_COUNT_TYPE eType = (ROOM_MONSTER_COUNT_TYPE)i;
		pRoom->nMonsterCountOnPopulate[ eType ] = RoomGetMonsterCount( pRoom, eType );
	}

	// start timer for reset event (if desired)
	if (sgeStartRoomResetEvent == SRRE_ON_ROOM_POPULATE)
	{
		s_RoomStartResetTimer( pRoom, 1.0f );
	}


	// link up unit types to spawn specific treasure
	s_sRoomCreateTreasureSpawn( pRoom );

				
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRoomPopulateCritters( GAME * pGame, ROOM * pRoom )
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pRoom->pDefinition, "Expected room definition" );
	ASSERTX_RETURN( RoomTestFlag( pRoom, ROOM_CLIENT_POPULATED_BIT ) == FALSE, "Room is already populated" );

	// do distribution population
	sRoomPopulateDRLGSpawnByDistribution( pGame, pRoom, RPP_CONTENT, DRLG_SPAWN_TYPE_CRITTER );

	// room is now populated
	RoomSetFlag( pRoom, ROOM_CLIENT_POPULATED_BIT );

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void HammerShortcutRoomPopulate( 
	GAME * pGame, 
	ROOM * pRoom)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	s_RoomActivate( pRoom, NULL );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFreeRoomUnitsDePopulate(
	UNIT *pUnit,
	void *pCallbackData)
{
	ROOM_POPULATE_PASS ePass = *(ROOM_POPULATE_PASS *)pCallbackData;
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	BOOL bFree = TRUE;

	// if a unit is not forced to be freed on room reset, we might be able to keep it around
	if (UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_FORCE_FREE_ON_ROOM_RESET ) == FALSE)
	{
		if (s_UnitCanBeDepopulated(pUnit) == FALSE)
		{
			bFree = FALSE;
		}

		// do not free units that are populated during the pass in question
		if (pUnitData->eRoomPopulatePass != ePass)
		{
			bFree = FALSE;
		}
	}
		
	// free unit if requested	
	if (bFree)
	{
		UnitFree( pUnit, UFF_SEND_TO_CLIENTS );		
	}

	// continue iterating units		
	return RIU_CONTINUE;
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void s_sRoomFreePopulates( 
	GAME *pGame, 
	ROOM *pRoom,
	ROOM_POPULATE_PASS eSourcePass)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Expected server" );
	ASSERTX_RETURN( pRoom, "Expected room" );

	// do nothing if this pass has not been populated in this room
	if (pRoom->bPopulated[ eSourcePass ] == FALSE)
	{
		return;
	}
	
	// iterate units and free for depopulate pass in question
	RoomIterateUnits( pRoom, NULL, sFreeRoomUnitsDePopulate, &eSourcePass );
	
	// this pass is no longer populated in this room
	pRoom->bPopulated[ eSourcePass ] = FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sRoomIterateUnitsOfTargetType(
	ROOM *pRoom,
	TARGET_TYPE eTargetType,
	PFN_ROOM_ITERATE_UNIT_CALLBACK pfnCallback,
	void *pCallbackData)
{		
	ASSERTX_RETVAL( pRoom, RIU_CONTINUE, "Expected room" );
	ASSERTX_RETVAL( pfnCallback, RIU_CONTINUE, "Expected callback" );
	ASSERTX_RETVAL( eTargetType >= TARGET_FIRST && eTargetType < NUM_TARGET_TYPES, RIU_CONTINUE, "Invalid target type when iterating units in room" );
	ROOM_ITERATE_UNITS eResult = RIU_CONTINUE;
	GAME *pGame = RoomGetGame( pRoom );
	
	// we have not yet made this system re-entrant, but the need will perhaps one day
	// arise, and this flag will tell if that happens, cause finding re-entrant bugs
	// like that really sucks ... just make the sequence # in the unit a small or something -Colin
	ASSERTX_RETVAL( RoomTestFlag( pRoom, ROOM_UNITS_BEING_ITERATED_BIT ) == FALSE, RIU_CONTINUE, "sRoomIterateUnitsOfTargetType() is not safe to be re-entrant in it's current implementaiton" );

	// set flag that we are now iterating units in this room
	RoomSetFlag( pRoom, ROOM_UNITS_BEING_ITERATED_BIT, TRUE );

	// get the next available sequence number
	DWORD dwSequence = RoomGetIterateSequenceNumber( pRoom );

	UNITID idLastValid = INVALID_ID;	
	UNITID idNext = INVALID_ID;
	UNIT *pNext = NULL;						
	const TARGET_LIST *pTargetList = &pRoom->tUnitList;
	for (UNIT *pUnit = pTargetList->ppFirst[ eTargetType ]; pUnit; pUnit = pNext)
	{
	
		// get next unit
		pNext = pUnit->tRoomNode.pNext;
		idNext = UnitGetId( pNext );
		
		// mark this unit as being iterating in this sequence
		pUnit->dwRoomIterateSequence = dwSequence;
		
		// keep id of the last valid unit we had
		idLastValid = UnitGetId( pUnit );
		
		// do callback
		// DANGER!  Will cause a crash if next is deleted as a result of this (like pets)
		eResult = pfnCallback( pUnit, pCallbackData );
		if (eResult == RIU_STOP)
		{
			break;
		}
	
		// NOTE: pUnit is now invalid here (callback could have deleted it)
		
		// validate the next unit still exists if we need to
		if (idNext != INVALID_ID)
		{
		
			UNIT *pNextValid = UnitGetById( pGame, idNext );
			if (pNextValid == NULL)
			{
				
				// next unit was made invalid by the callback for this unit, this is an
				// extremely rare case, but we need to handle it ... we'll try to use
				// the last valid unit we had, but if that fails we'll scan the list
				// looking for the next unit that hasn't been processed this iteration
				UNIT *pLastValid = UnitGetById( pGame, idLastValid );
				if (pLastValid)
				{
					pNextValid = pLastValid->tRoomNode.pNext;
				}
				else
				{
					
					// last valid is also gone, scan the list looking for next thing to process
					for (UNIT *pUnitOther = pTargetList->ppFirst[ eTargetType ]; 
						 pUnitOther; 
						 pUnitOther = pUnitOther->tRoomNode.pNext)
					{
					
						// if sequence number doesn't match, we haven't processed it yet (not re-entrant safe)
						if (pUnitOther->dwRoomIterateSequence != dwSequence)
						{
							pNextValid = pUnitOther;
							break;
						}
						
					}
					
				}
							
			}

			// set next unit to the next valid one we got
			pNext = pNextValid;
	
		}
				
	}

	// no longer iterating units
	RoomSetFlag( pRoom, ROOM_UNITS_BEING_ITERATED_BIT, FALSE );

	return eResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_ITERATE_UNITS RoomIterateUnits(
	ROOM *pRoom,
	const TARGET_TYPE *eTargetTypes,
	PFN_ROOM_ITERATE_UNIT_CALLBACK pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETVAL( pRoom, RIU_CONTINUE, "Expected room" );
	ASSERTX_RETVAL( pfnCallback, RIU_CONTINUE, "Expected callback" );
	
	// if no target types specified, do all of them
	if (eTargetTypes == NULL)
	{
		for (int i = TARGET_FIRST; i < NUM_TARGET_TYPES; ++i)
		{
			TARGET_TYPE eTargetType = (TARGET_TYPE)i;
			
			// iterate units of this target type
			ROOM_ITERATE_UNITS eResult = sRoomIterateUnitsOfTargetType( pRoom, eTargetType, pfnCallback, pCallbackData );
			
			// stop if directed
			if (eResult == RIU_STOP)
			{
				return RIU_STOP;
			}			
			
		}	
			
	}
	else
	{
		for (int i = 0; eTargetTypes[ i ] != TARGET_INVALID; ++i)
		{
			TARGET_TYPE eTargetType = eTargetTypes[ i ];
			
			// iterate units of this target type
			ROOM_ITERATE_UNITS eResult = sRoomIterateUnitsOfTargetType( pRoom, eTargetType, pfnCallback, pCallbackData );
			
			// stop if directed
			if (eResult == RIU_STOP)
			{
				return RIU_STOP;
			}			
						
		}	
		
	}

	return RIU_CONTINUE;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *RoomFindFirstUnitOf(
	ROOM *pRoom,
	TARGET_TYPE *pTargetTypes,
	GENUS eGenus,
	int nClass,
	PFN_UNIT_FILTER pfnFilter /*= NULL*/)
{
	ASSERTX_RETNULL( pRoom, "Expected room" );
	ASSERTX_RETNULL( eGenus != GENUS_NONE, "Invalid genus" );
	ASSERTX_RETNULL( nClass != INVALID_LINK, "Invalid unit class" );
	
	// iterate units in room
	ROOM_FIND_FIRST_OF_DATA tData;
	tData.spSpecies = MAKE_SPECIES( eGenus, nClass );
	tData.pfnFilter = pfnFilter,
	RoomIterateUnits( pRoom, pTargetTypes, sFindFirstOf, &tData );

	// return result (if any)	
	return tData.pResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDoFreeUnit(
	UNIT * unit,
	void *)
{	
	if (IS_CLIENT(unit) && c_ClientAlwaysKnowsUnitInWorld(unit))
	{
		ASSERTX_RETVAL(UnitGetRoom(unit), RIU_CONTINUE, "Expected room");
		UnitRemoveFromWorld(unit, FALSE);
	}
	else
	{
		UnitFree(unit, UFF_ROOM_BEING_FREED);
	}
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomFreeUnits(
	GAME * game,
	ROOM * room)
{
	ASSERT_RETURN(game && room);
	RoomIterateUnits(room, NULL, sDoFreeUnit, NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomFreeCommon(
	GAME * pGame,
	ROOM * pRoom,
	LEVEL * level,
	BOOL bFreeEntireLevel)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pRoom, "Expected room" );

	RoomUnhookNeighboringRooms(pGame, level, pRoom, bFreeEntireLevel);

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		hkWorld *pWorld = level->m_pHavokWorld;

		for(unsigned int i=0; i < pRoom->pRoomModelRigidBodies.Count(); i++)
		{
			HavokReleaseRigidBody( pGame, pWorld, pRoom->pRoomModelRigidBodies[ i ] );
		}
		for(unsigned int i=0; i < pRoom->pRoomModelInvisibleRigidBodies.Count(); i++)
		{
			HavokReleaseInvisibleRigidBody( pGame, pWorld, pRoom->pRoomModelInvisibleRigidBodies[ i ] );
		}
	}
#endif

	// free the room units
	RoomFreeUnits( pGame, pRoom );

	// remove client models
	if (IS_CLIENT( pGame ) )
	{
		if(pRoom->pLayoutSelections)
		{
			for(int i = 0; i < pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GRAPHICMODEL].nCount; i++)
			{
				e_ModelRemovePrivate( pRoom->pLayoutSelections->pPropModelIds[ i ] );
			}
		}

		for( unsigned int i = 0; i < pRoom->pnRoomModels.Count(); i++ )
		{
			e_ModelRemovePrivate( pRoom->pnRoomModels[ i ] );
		}

		pRoom->pnModels.Destroy();
		pRoom->pnRoomModels.Destroy();
	}

	if (pRoom->pAutomap)
	{
		AUTOMAP_NODE* map = pRoom->pAutomap;
		while (map)
		{
			AUTOMAP_NODE* next = map->m_pNext;
			GFREE(pGame, map);
			map = next;
		}
	}

	if (!bFreeEntireLevel)
	{
		RoomRemoveEdgeNodeConnections(pGame, pRoom);
	}

	DRLGFreePathingMesh(pGame, pRoom);

	if (pRoom->pHull)
	{
		ConvexHullDestroy(pRoom->pHull);
		pRoom->pHull = NULL;
	}

	// client-only lists
	if ( AppIsHellgate() )
	{
		pRoom->pRoomModelRigidBodies.Destroy();
		pRoom->pRoomModelInvisibleRigidBodies.Destroy();
	}
	HashFree(pRoom->tUnitNodeMap);
	HashFree(pRoom->tFieldMissileMap);

	if (pRoom->pConnections)
	{
		GFREE(pGame, pRoom->pConnections);
	}
	if (pRoom->ppVisibleRooms)
	{
		GFREE(pGame, pRoom->ppVisibleRooms);
	}
	if (pRoom->ppNearbyRooms)
	{
		GFREE(pGame, pRoom->ppNearbyRooms);
	}
	if (pRoom->ppConnectedRooms)
	{
		GFREE(pGame, pRoom->ppConnectedRooms);
	}
	if (pRoom->pLayoutSelections)
	{
		RoomLayoutSelectionsFree(pGame, pRoom->pLayoutSelections);
	}
	pRoom->pPathNodeInstances.Free();
	RoomFreeEdgeNodeConnections(pGame, pRoom);
	GFREE(pGame, pRoom);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomFree(
	GAME * game,
	ROOM * room,
	BOOL bFreeEntireLevel)
{
	LEVEL * level = RoomGetLevel(room);
	ASSERT_RETURN(level);

	RoomFreeCommon(game, room, level, bFreeEntireLevel);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void RoomRerollDRLG(
	GAME* game,
	const char* pszFileName,
	int nSeedDelta)
{

	// if specified seed is nonzero, don't allow it to be changed
	//if ( global_definition->dwSeed == 0 )								// for dg
	{
		LEVELID idLevel = 0;
		LEVEL *pLevel = LevelGetByID( game, idLevel );
		ASSERT_RETURN( pLevel );
		
		// save the type of level to create		
		LEVEL_TYPE tLevelType = LevelGetType( pLevel );
		
		// free old level
		LevelFree(game, idLevel);
		
		// after freeing the level, must re-add before creating
		DungeonSetSeed(game, DungeonGetSeed(game) + nSeedDelta);

		// fill out common level spec 
		LEVEL_SPEC tSpec;
		tSpec.idLevel = 0;
		tSpec.tLevelType = tLevelType;
		tSpec.nDRLGDef = DungeonGetLevelDRLG( game, tLevelType.nLevelDef );
		tSpec.dwSeed = DungeonGetSeed( game );
		tSpec.bPopulateLevel = TRUE;

		// tugboat specific		
		if (AppIsTugboat())
		{
			tSpec.tLevelType.nLevelArea = 0;  // supposed to be hard coded??? -Colin
		}

		// create level
		pLevel = LevelAdd( gApp.pClientGame, tSpec );

		ASSERTX_RETURN( pLevel, "Expected level" );		
		ASSERTX_RETURN( pLevel->m_idLevel >= 0, "Expected level id" );
		DRLGCreateLevel(game, pLevel, DungeonGetSeed(game), INVALID_ID );
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DrawRoom(
	GAME* game,
	UNIT* unit,
	int nWorldX,
	int nWorldY)
{
	ASSERT_RETURN(game && unit);

	LEVEL* level = RoomGetLevel(UnitGetRoom(unit));
	ASSERT_RETURN(level);

	ROOM* room = LevelGetFirstRoom(level);
	{
		ROOM_DEFINITION* room_definition = room->pDefinition;
		ASSERT_RETURN(room_definition);

		int nWidth, nHeight;
		gdi_getscreensize(&nWidth, &nHeight);
		int nScreenX = nWorldX - (nWidth / 2);
		int nScreenY = nWorldY - (nHeight / 2);
		ROOM_CPOLY* pList = room_definition->pRoomCPolys;
		for (int nCollisionPoly = 0; nCollisionPoly < room_definition->nRoomCPolyCount; nCollisionPoly++)
		{
			// Draw Collision poly
			int nX1 = PrimeFloat2Int(pList->pPt1->vPosition.fX) - nScreenX;
			int nY1 = PrimeFloat2Int(pList->pPt1->vPosition.fY) - nScreenY;
			int nX2 = PrimeFloat2Int(pList->pPt2->vPosition.fX) - nScreenX;
			int nY2 = PrimeFloat2Int(pList->pPt2->vPosition.fY) - nScreenY;
			int nX3 = PrimeFloat2Int(pList->pPt3->vPosition.fX) - nScreenX;
			int nY3 = PrimeFloat2Int(pList->pPt3->vPosition.fY) - nScreenY;
			gdi_drawline(nX1, nY1, nX2, nY2, gdi_color_red );
			gdi_drawline(nX2, nY2, nX3, nY3, gdi_color_red );
			gdi_drawline(nX3, nY3, nX1, nY1, gdi_color_red );

			pList++;
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sRoomSetVisible ( ROOM * pRoom, BOUNDING_BOX & ClipBox, int nDepth )
{
#if !ISVERSION(SERVER_VERSION)
	TRACE_ROOM( pRoom->idRoom, "Add Visible" );

	c_AddVisibleRoom( pRoom, ClipBox );
	//c_SetRoomModelsVisible( pRoom, TRUE );

	if ( e_GetRenderFlag( RENDER_FLAG_SHOW_PORTAL_INFO ) )
	{
		if ( nDepth >= DebugKeyGetValue() )
			e_DebugDrawPortal( ClipBox, 0xff0000ff, nDepth );
	}
#endif// !ISVERSION(SERVER_VERSION)
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//static void sSetConnectionTraversed( ROOM * pFromRoom, ROOM_CONNECTION * pConnection, BOOL bTraversed )
//{
//	ASSERT_RETURN( pFromRoom );
//	ASSERT_RETURN( pConnection && pConnection->pRoom );
//
//	int nConn = pConnection->bOtherConnectionIndex;
//
//	ASSERT_RETURN( nConn != INVALID_ID );
//	ASSERT_RETURN( nConn < pConnection->pRoom->nConnectionCount );
//	ASSERT_RETURN( pConnection->pRoom->pConnections[ nConn ].pRoom );
//	ASSERTONCE_RETURN( pConnection->pRoom->pConnections[ nConn ].pRoom == pFromRoom );
//
//	ASSERT_RETURN( ! bTraversed || ! pConnection->pRoom->pConnections[ nConn ].bTraversed );
//	ASSERT_RETURN( ! bTraversed || ! pConnection->bTraversed );
//	pConnection->pRoom->pConnections[ nConn ].bTraversed = bTraversed;
//	pConnection->bTraversed = bTraversed;
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sTestConnectionVisibility(
	const ROOM_CONNECTION * pConnection,
	const MATRIX * pmWorldViewProj, 
	const VECTOR * pvViewPosition, 
	const BOUNDING_BOX * pClipBox,
	VECTOR * pvScreenSpace,
	BOOL & bTooClose,
	float fClipRange )
{
	// don't test rooms through connections we've marked as already traversed (higher in the current callstack)
	//if ( pConnection->bTraversed )
	//	return FALSE;

	ROOM * pTestRoom = pConnection->pRoom;
	const VECTOR * pvCorners = pConnection->pvBorderCorners;

	// First, compare the connection's normal to the eye vector
	VECTOR vView;
	CLT_VERSION_ONLY(e_GetViewPosition( &vView ));
	vView = vView - pConnection->vBorderCenter;
	VectorNormalize( vView );
	VECTOR vNormal = pConnection->vBorderNormal;
	float fDot = VectorDot( vView, vNormal );
	// allow for a touch of slop in the normal comparison
	if ( fDot < -0.001f )
	{
		// connection faces the wrong way -- not traversable from this connection
		return FALSE;
	}

	// In order for a triangle or square to be outside of the view frustrum,
	// all of the points must be outside of the frustum on the same side.

	// However, if the corners are too close to the eye, treat it as a full-screen large portal
	const float cfMinClipPlaneDist    = 10.0f;
	const float cfMinClipPlaneDistSqr = cfMinClipPlaneDist * cfMinClipPlaneDist;
	VECTOR4 pvTransformed[ ROOM_CONNECTION_CORNER_COUNT ];

	DWORD dwOutOfRange = 0;

	//float fMaxDist = e_GetFarClipPlane();
	float fMaxDist = 1.0f;

	for ( int i = 0; i < ROOM_CONNECTION_CORNER_COUNT; i++ )
	{
		MatrixMultiply( &pvTransformed[ i ], &pvCorners[ i ], pmWorldViewProj );
		pvScreenSpace[ i ].x = pvTransformed[ i ].x / pvTransformed[ i ].w;
		pvScreenSpace[ i ].y = pvTransformed[ i ].y / pvTransformed[ i ].w;
		pvScreenSpace[ i ].z = pvTransformed[ i ].z / pvTransformed[ i ].w;
		if ( pvTransformed[ i ].w < 0.0f )
		{
			pvScreenSpace[ i ].x = - pvScreenSpace[ i ].x;
			pvScreenSpace[ i ].y = - pvScreenSpace[ i ].y;
		}
		if ( pvScreenSpace[ i ].z >= fMaxDist || pvScreenSpace[ i ].z < 0.0f )
			dwOutOfRange |= MAKE_MASK(i);
	}

	PLANE tPlane;
	PlaneFromPoints( &tPlane, &pvCorners[ 0 ], &pvCorners[ 1 ], &pvCorners[ 2 ] );
	VECTOR vPOP; // point on plane
	NearestPointOnPlane( &tPlane, pvViewPosition, &vPOP );

	// get the distance from the plane, then the distance on the plane from the plane's center
	float fDistanceFromPlane = VectorDistanceSquared( vPOP, *pvViewPosition );
	float fDistanceFromCenter = VectorDistanceSquared( vPOP, pConnection->vPortalCentroid );

	bTooClose = ( fDistanceFromCenter < pConnection->fPortalRadiusSqr ) && ( fDistanceFromPlane < cfMinClipPlaneDistSqr );

	//if ( !bTooClose )
	{
		// if they're all out of Z range
		if ( dwOutOfRange == ( ( 1 << ROOM_CONNECTION_CORNER_COUNT ) - 1 ) )
			return FALSE;

		// test each point to see where it fails to be in the frustum and accumulate the errors
		DWORD dwTotalTestFlags = 0xff;
		for ( int i = 0; i < ROOM_CONNECTION_CORNER_COUNT; i++ )
		{
			DWORD dwTestFlags = 0;
			if ( pvScreenSpace[ i ].x > pClipBox->vMax.x )
				dwTestFlags |= 0x01;
			if ( pvScreenSpace[ i ].x < pClipBox->vMin.x )
				dwTestFlags |= 0x02;
			if ( pvScreenSpace[ i ].y > pClipBox->vMax.y )
				dwTestFlags |= 0x04;
			if ( pvScreenSpace[ i ].y < pClipBox->vMin.y )
				dwTestFlags |= 0x08;
			if ( pvScreenSpace[ i ].z < 0.0f )
				dwTestFlags |= 0x10;
			if ( pvScreenSpace[ i ].z > fMaxDist )
				dwTestFlags |= 0x20;
			dwTotalTestFlags &= dwTestFlags;
		}
		// if all of the points shared the same error, then don't draw it
		if ( dwTotalTestFlags != 0 ) 
		{
			if ( ! ConvexHullPointTest( pTestRoom->pHull, pvViewPosition ) )
			{
				return FALSE;
			}
		}
		if ( ConvexHullManhattenDistance( pTestRoom->pHull, pvViewPosition ) > fClipRange )
		{
			return FALSE;
		}
	}

	// okay, we are drawing this room

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void c_sAddAllRoomsThroughPortal( PORTAL * pPortal, const LEVEL * level, int nDepth )
{
/*
	if ( gbDumpRooms )
		trace( "[ Found Outdoor Portal ]  prtlid:%2d prtlrgn:%2d depth:%d\n", pPortal->nId, pPortal->nRegion, nDepth );

#if !ISVERSION(SERVER_VERSION
	ASSERT_RETURN( pPortal );
	if ( ! pPortal->pConnection )
		return;

	if ( ! e_PortalHasFlags( pPortal, PORTAL_FLAG_ACTIVE | PORTAL_FLAG_VISIBLE ) )
		return;

	MATRIX matView, matProj;
	V( e_GetWorldViewProjMatricies( NULL, &matView, &matProj ) );
	//MatrixMultiply( &matView, &matView, &pPortal->mLocalToRemote );

	// build a frustum for the view through this portal
	PLANE pPlanes[ NUM_FRUSTUM_PLANES ];
	V( e_ExtractFrustumPlanes( pPlanes, &pPortal->tScreenBBox, &pPortal->mView, &matProj ) );

	// iterate rooms, find all from the region in question
	int nRegion = pPortal->pConnection->nRegion;

	sBeginPortalSection( pPortal->nId );

	ROOM * room = LevelGetFirstRoom(level);
	BOUNDED_WHILE(room, 10000)
	{
		if ( RoomGetSubLevelEngineRegion( room ) == nRegion )
		{
			int nRootModel = RoomGetRootModel( room );
			if ( e_ModelInFrustum( nRootModel, pPlanes ) )
				c_sRoomSetVisible( room, pPortal->tScreenBBox, nDepth );
		}

		room = LevelGetNextRoom(room);
	}

	sEndPortalSection();
#endif //!ISVERSION(SERVER_VERSION)
*/
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sRoomsAddThroughPortal(
	PORTAL * pPortal,
	GAME * pGame,
	LEVEL * pLevel,
	ROOM * pCenterRoom,
	MATRIX * pmWorldViewProj,
	VECTOR * pvViewPosition,
	BOUNDING_BOX * pClipBox,
	float fClipRange,
	int nDepth,
	int nPortalDepth )
{
/*
	ASSERT_RETURN( pPortal );
	if ( ! pPortal->pConnection )
		return;
	if ( ! e_PortalIsActive( pPortal->nId ) )
		return;
	DWORD dwFlags = e_PortalGetFlags( pPortal->nId );
	if ( 0 == ( dwFlags & PORTAL_FLAG_VISIBLE ) ||
		 0 != ( dwFlags & PORTAL_FLAG_NODRAW ) )
		return;

	if ( nPortalDepth >= 1 )
		return;

	int nLocation = e_RegionGetLevelLocation( pPortal->pConnection->nRegion );

	if ( nLocation == LEVEL_LOCATION_INDOOR || nLocation == LEVEL_LOCATION_INDOORGRID )
	{
		if ( gbDumpRooms )
			trace( "[ Found Indoor Portal ]  prtlid:%2d\n", pPortal->nId );

		ROOM_CONNECTION tPortalConn;
		ZeroMemory( &tPortalConn, sizeof(ROOM_CONNECTION) );

		if ( 0 != ( pPortal->dwFlags			  & PORTAL_FLAG_TRAVERSED ) ||
			 0 != ( pPortal->pConnection->dwFlags & PORTAL_FLAG_TRAVERSED ) )
			 return;

		// ---------------------------------------------------------------------------------------

		ASSERT_RETURN( ROOM_CONNECTION_CORNER_COUNT == PORTAL_NUM_CORNERS );
		MemoryCopy( tPortalConn.pvBorderCorners, sizeof(VECTOR) * ROOM_CONNECTION_CORNER_COUNT, pPortal->vCorners, sizeof(VECTOR) * ROOM_CONNECTION_CORNER_COUNT );
		tPortalConn.dwVisibilityFlags = 0xffffffff;
		tPortalConn.fPortalRadiusSqr = VectorDistanceSquared( pPortal->tScreenBBox.vMax, pPortal->vCenter );
		tPortalConn.pRoom = RoomGetByID( pGame, pPortal->nUserData );
		ASSERT_RETURN( tPortalConn.pRoom );
		tPortalConn.vPortalCentroid = pPortal->vCenter;
		//tPortalConn.bTraversed = FALSE;

		BOOL bTooClose;
		VECTOR pvScreenSpace[ ROOM_CONNECTION_CORNER_COUNT ];

		BOOL bVisible = sTestConnectionVisibility(
			&tPortalConn,
			pmWorldViewProj,
			pvViewPosition,
			pClipBox,
			pvScreenSpace,
			bTooClose,
			fClipRange );


		if ( ! bVisible )
			return;

		// ---------------------------------------------------------------------------------------

		// we are drawing this room

		BOUNDING_BOX ClipBoxNew;
		if ( ! bTooClose )
		{
			ClipBoxNew.vMin = pvScreenSpace[ 0 ];
			ClipBoxNew.vMax = pvScreenSpace[ 0 ];
			for ( int i = 1; i < ROOM_CONNECTION_CORNER_COUNT; i++ )
			{
				BoundingBoxExpandByPoint( &ClipBoxNew, &pvScreenSpace[ i ] );
			}
			BoundingBoxIntersect( &ClipBoxNew, pClipBox, &ClipBoxNew );
		} else
		{
			ClipBoxNew.vMin = VECTOR(-1.f, -1.f,  0.f);
			ClipBoxNew.vMax = VECTOR( 1.f,  1.f,  1.f);
		}

		// update WVP mat with portal transform
		MATRIX mWorld, mProj, mWVP, mWV;

		e_GetWorldViewProjMatricies( &mWorld, NULL, &mProj );
		MatrixMultiply( &mWV,  &mWorld, &pPortal->mView );
		MatrixMultiply( &mWVP, &mWV,    &mProj );


		sBeginPortalSection( pPortal->nId );


		ROOM * pTestRoom = RoomGetByID( pGame, pPortal->pConnection->nUserData );
		ASSERT_RETURN( pTestRoom );

		c_sRoomSetVisible ( pTestRoom, ClipBoxNew, nDepth );

		//sSetConnectionTraversed( pCurrentRoom, &pCurrentRoom->pConnections[ nConnection ], TRUE );
		pPortal->dwFlags |= PORTAL_FLAG_TRAVERSED;

		c_sTestRoomVisibility ( 
			pGame,
			pLevel,
			pCenterRoom, 
			pTestRoom, 
			0, 
			&mWVP, 
			&pPortal->vEyeLocation,
			&pPortal->vEyeDirection,
			&ClipBoxNew,
			fClipRange,
			nDepth + 1,
			nPortalDepth + 1);

		//sSetConnectionTraversed( pCurrentRoom, &pCurrentRoom->pConnections[ nConnection ], FALSE );
		pPortal->dwFlags &= ~PORTAL_FLAG_TRAVERSED;

		//sgnRoomTraversalCount++;

		sEndPortalSection();
	}
	else
	{
		c_sAddAllRoomsThroughPortal( pPortal, pLevel, 1 );
	}
*/
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static int sgnRoomTraversalCount = 0;
static int sgnRoomVisibleCount = 0;
static int sgnRoomDrawCount = 0;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void c_sTestRoomVisibility(
	GAME * pGame,
	LEVEL * pLevel,
	ROOM * pCenterRoom, 
	ROOM * pCurrentRoom, 
	DWORD dwVisibilityMask, 
	MATRIX * pmWorldViewProj, 
	VECTOR * pvViewPosition, 
	VECTOR * pvViewVector, 
	BOUNDING_BOX * pClipBox,
	float fClipRange,
	int nDepth,
	int nPortalDepth)
{
	if ( gbDumpRooms )
		trace( "[ Test Room Visibility ]  Cur:%4d  Cntr:%d  Vismsk:0x%08x  Dep:%3d  PDep:%3d\n",
			pCurrentRoom->idRoom,
			pCenterRoom->idRoom,
			dwVisibilityMask,
			nDepth,
			nPortalDepth );

	ASSERT_RETURN( pCurrentRoom->pDefinition );
	BOUNDING_BOX tRoomBox;
	BoundingBoxTranslate( &pCurrentRoom->tWorldMatrix, &tRoomBox, &pCurrentRoom->pDefinition->tBoundingBox );
	VECTOR vRoomCenter;
	vRoomCenter = ( tRoomBox.vMin + tRoomBox.vMax ) * 0.5f;

	// need to break up this room code into multiple functions that I can call with arguments to use either room connections list or a portal room connection
	// turn the portal into a room connection

	// we only render the 1st portal (depth is zero) in any visible portal chain
	if (nPortalDepth == 0)
	{

		ROOM_CONNECTION tPortalConn;
		for ( int nPortal = 0; nPortal < pCurrentRoom->nNumVisualPortals; nPortal++ )
		{
			ROOM_VISUAL_PORTAL * pRoomPortal = &pCurrentRoom->pVisualPortals[ nPortal ];

			PORTAL * pPortal = e_PortalGet( pRoomPortal->nEnginePortalID );
			if ( ! pPortal )
				continue;

			sRoomsAddThroughPortal(
				pPortal,
				pGame,
				pLevel,
				pCenterRoom,
				pmWorldViewProj,
				pvViewPosition,
				pClipBox,
				fClipRange,
				nDepth,
				nPortalDepth );
		}
	}
	

	for ( int nConnection = 0; nConnection < pCurrentRoom->nConnectionCount; nConnection++ )
	{
		ROOM * pTestRoom = pCurrentRoom->pConnections[ nConnection ].pRoom;
		if ( ! pTestRoom )
			continue;

		// We have precomputed whether you can see from one connection to another.
		if ( dwVisibilityMask != 0 && ( pCurrentRoom->pConnections[ nConnection ].dwVisibilityFlags & dwVisibilityMask ) == 0 )
			continue;


		BOOL bTooClose;
		VECTOR pvScreenSpace[ ROOM_CONNECTION_CORNER_COUNT ];

		BOOL bVisible = sTestConnectionVisibility(
			&pCurrentRoom->pConnections[ nConnection ],
			pmWorldViewProj,
			pvViewPosition,
			pClipBox,
			pvScreenSpace,
			bTooClose,
			fClipRange );

		if ( ! bVisible )
			continue;

		// we are drawing this room

		DWORD dwVisibilityMaskNext = 0;
		for (int i = 0; i < pTestRoom->nConnectionCount; i++ )
		{
			if ( pTestRoom->pConnections[ i ].pRoom == pCurrentRoom )
			{
				dwVisibilityMaskNext |= 1 << i;
				break;
			}
		}
		BOUNDING_BOX ClipBoxNew;
		if ( ! bTooClose )
		{
			ClipBoxNew.vMin = pvScreenSpace[ 0 ];
			ClipBoxNew.vMax = pvScreenSpace[ 0 ];
			for ( int i = 1; i < ROOM_CONNECTION_CORNER_COUNT; i++ )
			{
				BoundingBoxExpandByPoint( &ClipBoxNew, &pvScreenSpace[ i ] );
			}
			BoundingBoxIntersect( &ClipBoxNew, pClipBox, &ClipBoxNew );
		} else
		{
			ClipBoxNew.vMin = VECTOR(-1.f, -1.f,  0.f);
			ClipBoxNew.vMax = VECTOR( 1.f,  1.f,  1.f);
		}

		c_sRoomSetVisible ( pTestRoom, ClipBoxNew, nDepth );

		//sSetConnectionTraversed( pCurrentRoom, &pCurrentRoom->pConnections[ nConnection ], TRUE );

		c_sTestRoomVisibility ( 
			pGame,
			pLevel,
			pCenterRoom, 
			pTestRoom, 
			dwVisibilityMaskNext, 
			pmWorldViewProj, 
			pvViewPosition,
			pvViewVector,
			&ClipBoxNew,
			fClipRange,
			nDepth + 1,
			nPortalDepth);

		//sSetConnectionTraversed( pCurrentRoom, &pCurrentRoom->pConnections[ nConnection ], FALSE );

		sgnRoomDrawCount++;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
BOOL c_RoomToggleDrawAllRooms()
{
	sgbDrawAllRooms = !sgbDrawAllRooms;
	return sgbDrawAllRooms;
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
BOOL c_RoomToggleDrawOnlyConnected()
{
	sgbDrawOnlyConnected = !sgbDrawOnlyConnected;
	return sgbDrawOnlyConnected;
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
BOOL c_RoomToggleDrawOnlyNearby()
{
	sgbDrawOnlyNearby = !sgbDrawOnlyNearby;
	return sgbDrawOnlyNearby;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void hellgate_c_RoomDrawUpdate(
	GAME* game,
	UNIT* unit,
	float fTimeDelta)
{
	if ( gbDumpRooms )
		trace( "\n-----------------------------------------------------\n[ RoomDrawUpdate ]\n" );

#if !ISVERSION(SERVER_VERSION)
#ifdef ENABLE_VIEWER_RENDER
	{
		V( e_ViewersResetAllLists() );

		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_PARTICLES, TRUE  );
		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_BEHIND,	 FALSE );
		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_SKYBOX,	 TRUE  );
	}
#endif // ENABLE_VIEWER_RENDER

	if ( ! CameraGetUpdated() )
		return;

	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	int nCameraRoom = INVALID_ID;
	V( e_ViewerGetCameraRoom( gnMainViewerID, nCameraRoom ) );
	if ( game )
	{
		ROOM * pCameraRoom = RoomGetByID( game, nCameraRoom );
		pCameraRoom = RoomGetFromPosition( game, pCameraRoom, &pCameraInfo->vPosition );
		if ( pCameraRoom )
			nCameraRoom = pCameraRoom->idRoom;
		else if ( unit && unit->pRoom )
			nCameraRoom = unit->pRoom->idRoom;
	}
	int nCullCell;
	V( e_RegionGetCell( e_GetCurrentRegionID(), nCullCell ) );
	if ( nCameraRoom != INVALID_ID && e_CellGet( nCameraRoom ) )
		nCullCell = nCameraRoom;
	V( e_ViewerUpdateCamera( gnMainViewerID, CameraGetInfo(), nCameraRoom, nCullCell ) );


	if ( c_GetToolMode() )
	{
#ifdef ENABLE_VIEWER_RENDER
		{
			{
				PERF(ROOM_UPDATE_LIGHTS)
					c_UpdateLights();
			}

			// Add all models to the viewer
			V( e_ViewerAddAllModels( gnMainViewerID ) );

			V( e_ViewerSetFullViewport( gnMainViewerID ) );
			V( e_ViewerSelectVisible( gnMainViewerID ) );

			return;
		}
#endif // ENABLE_VIEWER_RENDER
	}

	ASSERT_RETURN(game);

	if (!unit || !UnitGetRoom(unit))
	{
		return;
	}

	if (!CameraGetUpdated())
	{
		return;
	}

	//BOUNDING_BOX ClipBox;
	VECTOR vViewPosition;
	//VECTOR vViewVector;


	e_GetViewPosition(&vViewPosition);

	ROOM * center_room = UnitGetRoom(unit);
	ROOM * position_room = RoomGetFromPosition( game, center_room, &vViewPosition, 0.0001f );
	ASSERTV( position_room, "Couldn't get camera's room from position!  Camera position appears to be outside the hulls of all neighboring rooms!  Using unit room as a workaround." );
	if (position_room)
		center_room = position_room;

	sgnRoomTraversalCount = 0;

	int nRegion = RoomGetSubLevelEngineRegion( center_room );
	V( e_SetRegion( nRegion ) );
	//e_SetCurrentViewOverrides( NULL, NULL, NULL, NULL );

	if ( gbDumpRooms )
		trace( "[ Set Region ]  rgn:%d\n", nRegion );


	//	make sure that you turn off the control unit's draw
	//	if (game->nViewType == VIEW_1STPERSON)
	//	{
	//		UnitSetDraw(unit, FALSE);
	//	}

	{
		PERF(UNIT_UPDATE_DRAW)
		c_UnitUpdateDrawAll( game, fTimeDelta );
	}
	

	{
		PERF(ROOM_UPDATE_LIGHTS)
			c_UpdateLights();
	}


#ifdef ENABLE_VIEWER_RENDER
	{
		if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
		{
			// Add all models to the viewer
			V( e_ViewerAddAllModels( gnMainViewerID ) );
		}

		V( e_ViewerSetFullViewport( gnMainViewerID ) );
		V( e_ViewerSelectVisible( gnMainViewerID ) );

		//goto finalize;
	}
#endif // ENABLE_VIEWER_RENDER


	//sgnRoomTraversalCount++;
#endif //!ISVERSION(SERVER_VERSION)

	if ( gbDumpRooms )
		trace( "[ Summary ]  Rooms traversed:%d  Rooms visible:%d\n---------------------------------------------------\n\n", sgnRoomTraversalCount, sgnRoomVisibleCount );

	gbDumpRooms = FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


void tugboat_c_RoomDrawUpdate(
	GAME* game,
	UNIT* unit,
	float fTimeDelta)
{
	if ( gbDumpRooms )
		trace( "\n-----------------------------------------------------\n[ RoomDrawUpdate ]\n" );

#if !ISVERSION(SERVER_VERSION)

#ifdef ENABLE_VIEWER_RENDER
	if ( e_GetRenderFlag( RENDER_FLAG_USE_VIEWER ) )
	{
		V( e_ViewersResetAllLists() );

		V( e_ViewerUpdateCamera( gnMainViewerID, CameraGetInfo(), -1, -1 ) );
	}
#endif // ENABLE_VIEWER_RENDER



	if ( c_GetToolMode() )
	{

#ifndef ENABLE_VIEWER_RENDER
		RL_PASS_DESC tDesc;
		tDesc.eType = RL_PASS_NORMAL;
		if ( AppIsHellgate() )
			tDesc.dwFlags = RL_PASS_FLAG_CLEAR_BACKGROUND | RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_RENDER_SKYBOX | RL_PASS_FLAG_GENERATE_SHADOWS;
		else
			tDesc.dwFlags = RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_GENERATE_SHADOWS;
		PASSID nPass = e_RenderListBeginPass( tDesc, e_GetCurrentRegionID() );

		if ( nPass == INVALID_ID )
			return;

		{
			// set all models visible and
			// send all models to the engine
			e_RenderListAddAllModels( nPass );
		}
#endif // ENABLE_VIEWER_RENDER


		{
			PERF(ROOM_UPDATE_LIGHTS)
			c_UpdateLights();
		}

#ifndef ENABLE_VIEWER_RENDER
		e_RenderListEndPass( nPass );
#endif // ENABLE_VIEWER_RENDER


		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_PARTICLES, TRUE  );
		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_BEHIND,	 FALSE );
		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_SKYBOX,	 TRUE  );

		// Add all models to the viewer
		V( e_ViewerAddAllModels( gnMainViewerID ) );

		V( e_ViewerSetFullViewport( gnMainViewerID ) );
		V( e_ViewerSelectVisible( gnMainViewerID ) );

		return;
	}

	ASSERT_RETURN(game);

	if ( ( !unit || !UnitGetRoom(unit) ) &&
		 !LevelGetByID(game, AppGetCurrentLevel() ) )
	{
		return;
	}

	if (!CameraGetUpdated())
	{
		return;
	}

	sgnRoomTraversalCount = 0;
	sgnRoomDrawCount = 0;


	BOUNDING_BOX ClipBox;
	ClipBox.vMin = VECTOR(-1.0f, -1.0f, -1.0f);
	ClipBox.vMax = VECTOR( 1.0f,  1.0f,  1.0f);

	// get the view parameters
	MATRIX matWorld, matView, matProj;
	VECTOR vEyeVector;
	VECTOR vEyeLocation;
	e_InitMatrices( &matWorld, &matView, &matProj, NULL, &vEyeVector, &vEyeLocation );

	BOUNDING_BOX AABBBounds;
	e_ExtractAABBBounds( &matView, &matProj, &AABBBounds );


	UNIT* pPlayer = GameGetControlUnit( AppGetCltGame() );
	LEVEL* pLevel;
	
	if( pPlayer )
	{
		pLevel = UnitGetLevel( pPlayer );
	}
	else
	{
		pLevel = LevelGetByID(AppGetCltGame(), (LEVELID)0);
	}

	if( !pLevel )
	{
		goto finalize;
	}

	c_ClearVisibleRooms();
	pLevel->m_pQuadtree->GetItems( /*tPlanes, &*/AABBBounds, gVisibleRoomList );

	unsigned int RoomCount = (unsigned int)gVisibleRoomList.Count();
	ROOM* pRoom;
	if( RoomCount > 0 )
	{
		if (!gVisibleRoomList[0])
		{
			return;
		}
	}
	else
	{
		return;
	}

	int nRegion = RoomGetSubLevelEngineRegion( gVisibleRoomList[0] );
	V( e_SetRegion( nRegion ) );

	if ( gbDumpRooms )
		trace( "[ Set Region ]  rgn:%d\n", nRegion );


	//e_SetCurrentViewOverrides( NULL, NULL, NULL, NULL );



	for( unsigned int i = 0; i < RoomCount; i++ )
	{
		pRoom = gVisibleRoomList[i];
		if ( ! e_GetRenderFlag( RENDER_FLAG_USE_VIEWER ) )
		{
			c_sRoomSetVisible( pRoom, ClipBox, 0 );
		}
		else
		{
			e_ViewerAddModels( gnMainViewerID, &pRoom->pnRoomModels );
			// Iterate models in room and add to viewer!
			//int nRootModel = RoomGetRootModel( pRoom );
			for ( unsigned int nModel = 0; nModel < pRoom->pnRoomModels.Count(); nModel++ )
			{
				e_ModelUpdateVisibilityToken( pRoom->pnRoomModels[ nModel ] );
			}
			e_ViewerAddModels( gnMainViewerID, &pRoom->pnModels );
			for ( unsigned int nModel = 0; nModel < pRoom->pnModels.Count(); nModel++ )
			{

				e_ModelUpdateVisibilityToken( pRoom->pnModels[ nModel ] );

			}
		}
	}


finalize:

	//	make sure that you turn off the control unit's draw

	if ( ! e_GetRenderFlag( RENDER_FLAG_USE_VIEWER ) )
	{
		// send models to the engine
		c_ProcessVisibleRoomsList();
	}
	else
	{
		//tDesc.dwFlags = RL_PASS_FLAG_RENDER_PARTICLES;
		e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_PARTICLES, TRUE );

		int nViewType = c_CameraGetViewType();
		if( nViewType == VIEW_3RDPERSON )
		{
			e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_BEHIND, TRUE  );
			e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_SKYBOX, TRUE );
		}
		else
		{
			e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_BEHIND, FALSE );
			e_ViewerSetRenderFlag( gnMainViewerID, VIEWER_FLAGBIT_SKYBOX, TRUE  );
		}
	}

	{
		PERF(UNIT_UPDATE_DRAW)
		c_UnitUpdateDrawAll( game, fTimeDelta );
	}
	
	{
		PERF(ROOM_UPDATE_LIGHTS)
		c_UpdateLights();
	}


#ifdef ENABLE_VIEWER_RENDER
	if ( e_GetRenderFlag( RENDER_FLAG_USE_VIEWER ) )
	{
		V( e_ViewerSetFullViewport( gnMainViewerID ) );
		V( e_ViewerSelectVisible( gnMainViewerID ) );
	}
#endif // ENABLE_VIEWER_RENDER



	sgnRoomDrawCount++;
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_RoomDrawUpdate(
	GAME* game,
	UNIT* unit,
	float fTimeDelta)
{
	if ( AppIsHellgate() )
		hellgate_c_RoomDrawUpdate( game, unit, fTimeDelta );
	else
		tugboat_c_RoomDrawUpdate( game, unit, fTimeDelta );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_RoomGetDrawCount(
	void)
{
	return sgnRoomVisibleCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomTestConvexHull(
	ROOM * room,
	const VECTOR * position,
	float epsilon = HULL_EPSILON)
{
	ASSERT_RETFALSE(room);
	if (AppIsHellgate())
	{
		return ConvexHullPointTest(room->pHull, position, epsilon);
	}
	else
	{
		return ConvexHullPointTestZIgnored(room->pHull, position);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomTestConvexHull(
	ROOM * room,
	const VECTOR & position,
	float epsilon = 0.0f)
{
	return sRoomTestConvexHull(room, &position, epsilon);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * RoomGetFromPosition(
	GAME * game,
	ROOM * room,
	const VECTOR * pvPosition,
	float fEpsilon)		// = HULL_EPSILON
{
	if (!room)
	{
		return NULL;
	}

	// check the obvious ones first
	if (sRoomTestConvexHull(room, pvPosition, fEpsilon))
	{
		return room;
	}

	ROOM_LIST tNearbyRooms;
	RoomGetNearbyRoomList(room, &tNearbyRooms);
	for (int ii = 0; ii < tNearbyRooms.nNumRooms; ii++)
	{
		ASSERT_BREAK(ii < MAX_ROOMS_PER_LEVEL);
		ROOM * test = tNearbyRooms.pRooms[ii];
		if (!test)
		{
			continue;
		}
		if (sRoomTestConvexHull(test, pvPosition, fEpsilon))
		{
			return test;
		}
	}

	LEVEL * level = RoomGetLevel(room);
	if (level)
	{
		for (room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
		{
			if (sRoomTestConvexHull(room, pvPosition, fEpsilon))
			{
				return room;
			}
		}
	}

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * RoomGetFromPosition(
	GAME * game,
	LEVEL * level,
	const VECTOR * loc)
{
	ASSERT_RETNULL(level);

	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		if (sRoomTestConvexHull(room, loc))
		{
			return room;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS s_sUnitDeactivateLevel(
	UNIT * unit,
	void *)
{
	s_UnitDeactivate(unit, TRUE);

	// continue iterating units in this room
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS s_sUnitDeactivateRoom(
	UNIT * unit,
	void *)
{
	s_UnitDeactivate(unit, FALSE);

	// continue iterating units in this room
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sRoomDeactivateUnits(
	ROOM * room,
	BOOL bDeactivateLevel)
{
	ASSERT_RETURN(room);
	if (bDeactivateLevel)
	{
		RoomIterateUnits(room, NULL, s_sUnitDeactivateLevel, NULL);
	}
	else
	{
		RoomIterateUnits(room, NULL, s_sUnitDeactivateRoom, NULL);
	}
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS s_sUnitActivate(
	UNIT * unit,
	void *)
{
	s_UnitActivate(unit);
						
	// continue iterating units in this room
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sRoomActivateUnits(
	ROOM * room)
{
	ASSERT_RETURN(room);
	RoomIterateUnits(room, NULL, s_sUnitActivate, NULL);	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sRoomCanSendUnit(
	GAMECLIENT * client,
	UNIT * unit)
{
	// we never send un-synhronized missiles
	if (UnitIsA(unit, UNITTYPE_MISSILE))
	{
		const UNIT_DATA * unit_data = UnitGetData(unit);
		ASSERT_RETFALSE(unit_data);
		if (!UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_SYNC))
		{
			return FALSE;
		}
	}

	// if we have immobile units, the client really should load them in the room 
	// itself and not have to be told about them ... in that case, we will need to
	// say that the server should not send them here
	// TODO: write me -Colin
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static ROOM_ITERATE_UNITS sDoSendUnit(
	UNIT * unit,
	void * data)
{		
	GAMECLIENT * client = (GAMECLIENT *)data;
	if (sRoomCanSendUnit(client, unit) == TRUE &&
		UnitIsKnownByClient(client, unit) == FALSE)
	{
		s_SendUnitToClient(unit, client);
	}
	return RIU_CONTINUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sRoomSendUnits( 
	ROOM * room, 
	GAMECLIENT * client)
{
	ASSERT_RETURN(room);
	ASSERT_RETURN(client);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);

	// never send units that are rooms that are not yet active, don't worry, when we activate
	// the room we will send the units to anybody who knows about the room (this only happens
	// because the known vs. activation is separated to save CPU power, so a client could
	// know about a room before it has had a chance to activate itself and its units
	if (RoomIsActive(room) == FALSE)
	{
		return;
	}
	
	// send units
	RoomIterateUnits(room, NULL, sDoSendUnit, client);
	
	if (AppIsTugboat())
	{
		// TRAVIS - let the client know that all units have been sent, 
		// and then they can trawl NPCs to change their states
		MSG_SCMD_ROOM_UNITS_ADDED msg;
		msg.idRoom = RoomGetId(room);
		s_SendMessage(game, ClientGetId(client), SCMD_ROOM_UNITS_ADDED, &msg);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RoomSendToClient(
	ROOM * room,
	GAMECLIENT * client,
	BOOL bMarkAsKnown)
{	
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(room);
	REF(client);
	REF(bMarkAsKnown);
#else
	ASSERT_RETURN(room);
	ASSERT_RETURN(client);
	GAME * game = client->m_pGame;
	ASSERT_RETURN(game);

	TRACE_KNOWN("*KT* SERVER: Send room [%d: %s] to client [%s]\n", RoomGetId(room), RoomGetDevName(room), ClientGetName(client));

	// setup message
	MSG_SCMD_ROOM_ADD msg;
	msg.idRoomDef = room->pDefinition->nRoomExcelIndex;
	msg.idRoom = RoomGetId(room);
	msg.mTranslation = room->tWorldMatrix;
	msg.dwRoomSeed = room->dwRoomSeed;
	msg.idSubLevel = RoomGetSubLevelID(room);
	msg.bKnownOnServer = bMarkAsKnown;
	PStrCopy(msg.szLayoutOverride, 32, room->szLayoutOverride, 256);

	MSG_SCMD_ROOM_ADD_COMPRESSED cmp_msg;
	if (CompressRoomMessage(msg, cmp_msg))
	{
#if ISVERSION(DEBUG_VERSION)
		MSG_SCMD_ROOM_ADD test_msg;
		UncompressRoomMessage(test_msg, cmp_msg);
		for (unsigned int ii = 0; ii < 4; ++ii) 
		{
			for (unsigned int jj = 0; jj < 4; ++jj) 
			{
				ASSERT(ABS(msg.mTranslation.m[ii][jj] - test_msg.mTranslation.m[ii][jj]) < EPSILON);
			}
		}
#endif
		s_SendMessage(game, ClientGetId(client), SCMD_ROOM_ADD_COMPRESSED, &cmp_msg);
	}
	else 
	{
		s_SendMessage(game, ClientGetId(client), SCMD_ROOM_ADD, &msg);
	}
		
	if (bMarkAsKnown)
	{
		ClientSetRoomKnown(client, room, TRUE);
		
		// for rooms that are active and populated, we need to inform client about 
		// units already in the room too
		if (RoomIsActive(room) == TRUE)
		{
			s_sRoomSendUnits(room, client);
		}
	}
#endif
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static ROOM_ITERATE_UNITS sDoSendRemoveUnit(
	UNIT * unit,
	void * data)
{
	GAMECLIENT * client = (GAMECLIENT *)data;
	
	// if this was a unit that was sent to the client, remove it
	if (sRoomCanSendUnit(client, unit) == TRUE)
	{
		DWORD dwFlags = 0;
		if( AppIsTugboat() &&
			UnitIsA( unit, UNITTYPE_PLAYER ) ||
			UnitIsA( unit, UNITTYPE_MONSTER ) ||
			UnitIsA( unit, UNITTYPE_OBJECT ) ||
			UnitIsA( unit, UNITTYPE_DESTRUCTIBLE ) )
		{
			dwFlags = UFF_FADE_OUT;
		}
		s_RemoveUnitFromClient(unit, client, UFF_FADE_OUT);
	}
		
	return RIU_CONTINUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sRoomSendRemoveUnits( 
	ROOM * room, 
	GAMECLIENT * client)
{
	RoomIterateUnits(room, NULL, sDoSendRemoveUnit, client);
}	
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RoomRemoveFromClient( 
	ROOM * room, 
	GAMECLIENT * client)
{
	ASSERT_RETURN(room);
	ASSERT_RETURN(client);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (!ClientIsRoomKnown(client, room))
	{	
		return;
	}

	GAME * game = client->m_pGame;
	ASSERT_RETURN(game);
	
	TRACE_KNOWN("*KT* SERVER: Remove room [%d: %s] from client [%s]\n", RoomGetId(room), RoomGetDevName(room), ClientGetName(client));

	// send messages to remove all the units that are in this room (note that they may
	// be in different rooms on the client if they are say pathfinding and therefore
	// are slightly behind the server - or maybe a totally different path altogether)
	// therefore can't just leave this to client
	s_sRoomSendRemoveUnits(room, client);
		
	// setup message
	MSG_SCMD_ROOM_REMOVE msg;
	msg.idRoom = RoomGetId(room);
	
	// send 
	s_SendMessage(game, ClientGetId(client), SCMD_ROOM_REMOVE, &msg);
		
	// mark the room as unknown
	ClientSetRoomKnown(client, room, FALSE);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomOutsideOfDeactivationRange(
	ROOM * room,
	UNIT * player)
{
	ASSERT_RETFALSE(room);
	ASSERT_RETFALSE(player);

	// get bounding sphere of room
	const BOUNDING_SPHERE &tBoundingSphereWorldSpace = room->tBoundingSphereWorldSpace;

	// get distance from player to room center
	VECTOR vToRoomCenter = tBoundingSphereWorldSpace.C - UnitGetPosition(player);
	
	// what is the range distance
	float flMaxRange;
	if (AppIsHellgate())
	{
		flMaxRange = ROOM_DEACTIVATION_RADIUS;
	}
	else
	{
		flMaxRange = ROOM_DEACTIVATION_RADIUS_TUGBOAT;
	}

	// get the total max range
	float flMaxRangeSq = flMaxRange + tBoundingSphereWorldSpace.r;
	flMaxRangeSq *= flMaxRangeSq;  // square it
			
	// does the player intersect the bounding sphere of the room
	float flDistToRoomCenterSq = VectorLengthSquared(vToRoomCenter);
	if (flDistToRoomCenterSq < flMaxRangeSq)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sRoomPlayerIsInsideActivationRange(
	ROOM *pRoom,
	UNIT *pPlayer,
	BOOL bAllowCachedKnownStatus,
	BOOL bCheckRoomsLinkedByVisualPortals)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = RoomGetGame( pRoom );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERTX_RETFALSE( pLevel, "Room is not in a level" );
	BOOL bInRange = FALSE;
	
	// must be on same level
	LEVEL *pLevelPlayer = UnitGetLevel( pPlayer );
	ASSERTX_RETFALSE( pLevelPlayer, "Expected level player" );
	if (pLevel == pLevelPlayer)
	{
	
		// if the level always reveals contents to everybody the answer is always yes
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( pLevel );
		ASSERTX_RETFALSE( pLevelDef, "Expected level def" );
		if (pLevelDef->bContentsAlwaysRevealed == TRUE)
		{
			bInRange = TRUE;
		}
		else
		{
		
			// if this client knows about this room, there certainly are inside
			// the activation range of the room ... this is an optimization because
			// room known vs activation are separate & slugged operations to save CPU,
			// therefore a client may know about a room before it becomes activated and vice versa
			if (bAllowCachedKnownStatus == TRUE)
			{
				GAMECLIENT *pClient = UnitGetClient( pPlayer );
				if (pClient)
				{
					if (ClientIsRoomKnown( pClient, pRoom ) == TRUE)
					{
						bInRange = TRUE;
					}
				}
			}
					
			// if a state is on the player for always know the whole level, we know it all! ;)
			if ( UnitHasState( UnitGetGame( pPlayer ), pPlayer, STATE_LEVEL_ALWAYS_ACTIVE ) )
			{
				bInRange = TRUE;
			}

			// check distance
			if (bInRange == FALSE)
			{
			
				// get bounding sphere of room
				const BOUNDING_SPHERE &tBoundingSphereWorldSpace = pRoom->tBoundingSphereWorldSpace;
				//BOUNDING_SPHERE tBoundingSphereWorldSpace;
				//RoomGetBoundingSphereWorldSpace( pRoom, tBoundingSphereWorldSpace.r, tBoundingSphereWorldSpace.C );

				// get distance from player to room center
				VECTOR vToRoomCenter = tBoundingSphereWorldSpace.C - UnitGetPosition( pPlayer );
				
				// what is the range distance
				float flMaxRange;
				if (AppIsHellgate())
				{
					flMaxRange = ROOM_ACTIVATION_RADIUS;
				}
				else
				{
					flMaxRange = ROOM_ACTIVATION_RADIUS_TUGBOAT;
				}

				// get the total max range
				float flMaxRangeSq = flMaxRange + tBoundingSphereWorldSpace.r;
				flMaxRangeSq *= flMaxRangeSq;  // square it
						
				// does the player intersect the bounding sphere of the room
				float flDistToRoomCenterSq = VectorLengthSquared( vToRoomCenter );
				if (flDistToRoomCenterSq < flMaxRangeSq )
				{
					bInRange = TRUE;
				}

			}
		
			// if still not in range and this room has any visual portals, we will consider
			// being in range of the linked room as in range of this room
			if (bInRange == FALSE && bCheckRoomsLinkedByVisualPortals == TRUE)
			{
				int nNumVisualPortals = pRoom->nNumVisualPortals;
				
				for (int i = 0; i < nNumVisualPortals; ++i)
				{
					UNITID idPortal = pRoom->pVisualPortals[ i ].idVisualPortal;
					UNIT *pPortal = UnitGetById( pGame, idPortal );
					UNIT *pPortalOpposite = s_ObjectGetOppositeSubLevelPortal( pPortal );
					if (pPortalOpposite)
					{
						ROOM *pRoomOpposite = UnitGetRoom( pPortalOpposite );
						if (pRoomOpposite && pRoomOpposite != pRoom)
						{
							bInRange = s_sRoomPlayerIsInsideActivationRange( 
								pRoomOpposite, 
								pPlayer, 
								bAllowCachedKnownStatus, 
								FALSE);
						}
						
					}	
											
				}
				
			}
				
		}
		
	}
		
	return bInRange;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomGetBoundingSphereWorldSpace(
	ROOM *pRoom,
	float &flRadius,
	VECTOR &vCenter)
{
	ASSERTX_RETURN( pRoom, "Expected room" );

	// get bounding box in world space
	const BOUNDING_BOX * pBox = RoomGetBoundingBoxWorldSpace(pRoom, vCenter);
	
	// get size of box
	VECTOR vSize = BoundingBoxGetSize( pBox );
	
	// radius is half of the size
	vSize /= 2.0f;
	flRadius = VectorLength( vSize );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *s_sGetFirstPlayerInRoomActivationRange(
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	
	// go through clients on this level
	for (GAMECLIENT *pClient = ClientGetFirstInLevel( pLevel ); 
		 pClient; 
		 pClient = ClientGetNextInLevel( pClient, pLevel ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
					
		// must be on same sublevel
		ROOM *pRoomPlayer = UnitGetRoom( pPlayer );
		if (RoomsAreInSameSubLevel( pRoom, pRoomPlayer ) ||
			pRoom->nNumVisualPortals > 0)
		{
			if (s_sRoomPlayerIsInsideActivationRange( pRoom, pPlayer, TRUE, TRUE ))
			{
				return pPlayer;  // at least one client is inside activation range of room
			}
			
		}
		
	}
	
	return NULL;  // no clients in activation range of room
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sIsPortalReturningPlayerInRoomActivationRange(
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );

	// go through clients on this level
	for (GAMECLIENT *pClient = ClientGetFirstInLevel( pLevel ); 
		pClient; 
		pClient = ClientGetNextInLevel( pClient, pLevel ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		if (UnitTestFlag( pPlayer, UNITFLAG_RETURN_FROM_PORTAL ))
		{

			// must be on same sublevel
			ROOM *pRoomPlayer = UnitGetRoom( pPlayer );
			if (RoomsAreInSameSubLevel( pRoom, pRoomPlayer ) ||
				pRoom->nNumVisualPortals > 0)
			{
				if (s_sRoomPlayerIsInsideActivationRange( pRoom, pPlayer, TRUE, TRUE ))
				{
					return TRUE;  // at least one client is inside activation range of room
				}

			}
		}
	}

	return FALSE;  // no clients in activation range of room

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RoomUpdate( 
	ROOM * room)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(room);

	// if any player is close enough to this room activate it, otherwise deactivate it
	UNIT * player = s_sGetFirstPlayerInRoomActivationRange(room);
	if (player)
	{
		LEVEL * level = RoomGetLevel(room);
		ASSERT_RETURN(level);
		ASSERT_RETURN(LevelIsActive(level) == TRUE);	

		s_RoomActivate(room, player);
	}
	else
	{
		s_RoomDeactivate(room, FALSE);
	}
#else
	REF(room);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sConstructKnownRoomList(
	UNIT * player)
{
	// TRAVIS: Going to try using the Hellgate nearbyroom stuff, after significant
	// changes to the system - I think it'll work for us now.
	//if (AppIsHellgate())
	{		
		ROOM * room = UnitGetRoom(player);
		ASSERT_RETURN(room && RoomGetGame(room) == UnitGetGame(player));
		
		// get nearby rooms
		ROOM_LIST list;
		RoomGetKnownRoomList(player, room, &list, TRUE);

		// mark those rooms as known
		ASSERT_RETURN(list.nNumRooms < MAX_ROOMS_PER_LEVEL);
		for (int ii = 0; ii < list.nNumRooms; ++ii)
		{
			ROOM * room = list.pRooms[ii];
			ASSERT_BREAK(room);
			SETBIT(room->pdwFlags, ROOM_KNOWN_CURRENT);
		}
	}
	/*else
	{
		BOUNDING_BOX BBox;
		BBox.vMin = player->vPosition;
		BBox.vMax = BBox.vMin;
		BBox.vMin.fX -= 50;
		BBox.vMin.fY -= 50;
		BBox.vMax.fX += 50;
		BBox.vMax.fY += 50;
		BBox.vMax.fZ += 40; 
	
		SIMPLE_DYNAMIC_ARRAY<ROOM *> list;
		ArrayInit(list, GameGetMemPool(UnitGetGame(player)), 2);		

		LEVEL * level = UnitGetLevel(player);
		ASSERT_RETURN(level);
		level->m_pQuadtree->Sort(BBox, list);	

		// add this room to the list of currently known rooms	
		unsigned int count = (unsigned int)list.Count();
		for (unsigned int ii = 0; ii < count; ++ii)
		{	
			ROOM * room = list[ii];
			ASSERT_BREAK(room);
			SETBIT(room->pdwFlags, ROOM_KNOWN_CURRENT);
		}
		list.Destroy();
	}*/
}


//----------------------------------------------------------------------------
// this does a fair amount of work, iterating room known lists to determine
// what new rooms to send a client.  ideally, there would be a more on-demand
// way of doing this
//----------------------------------------------------------------------------
void RoomUpdateClient( 
	UNIT * player)
{
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(player);
#else
	ASSERT_RETURN(player);
	GAME * game = UnitGetGame(player);
	ASSERT_RETURN(game);
	ASSERT_RETURN(IS_SERVER(game));
	GAMECLIENT * client = UnitGetClient(player);
	if (!client)
	{
		return;
	}
	LEVEL * level = UnitGetLevel(player);
	ASSERT_RETURN(level);
	ROOM * curroom = UnitGetRoom(player);
	ASSERT_RETURN(curroom);

	// setups lists for rooms we knew about before and after the update
	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		SETBIT(room->pdwFlags, ROOM_KNOWN_PREVIOUS, ClientIsRoomKnown(client, room, TRUE));
		CLEARBIT(room->pdwFlags, ROOM_KNOWN_CURRENT);
	}	

	sConstructKnownRoomList(player);

	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		BOOL bWasKnown = TESTBIT(room->pdwFlags, ROOM_KNOWN_PREVIOUS);
		BOOL bIsKnown = TESTBIT(room->pdwFlags, ROOM_KNOWN_CURRENT);

		if (!bWasKnown && bIsKnown)
		{
			LevelNeedRoomUpdate(level, TRUE);		// we should try to activate rooms because a player knows about at least one of them now

			s_RoomSendToClient(room, client, TRUE);
		}
		else if (bWasKnown && !bIsKnown)
		{
			if (RoomOutsideOfDeactivationRange(room, player))
			{
				s_RoomRemoveFromClient(room, client);
			}
		}
	}
#endif			
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RoomAddModel ( ROOM * pRoom, int nModelId, MODEL_SORT_TYPE SortType )
{
	if (! pRoom) 
		return;
	if ( AppIsTugboat() )
	{
		switch( SortType )
		{
			case MODEL_STATIC :
				ArrayAddItem(pRoom->pnRoomModels, nModelId);
			break;
			case MODEL_DYNAMIC :
			case MODEL_ICONIC :
				ArrayAddItem(pRoom->pnModels, nModelId);
			break;
	
		}
	}
	else
	{
		if ( SortType == MODEL_STATIC )
		{
#if ISVERSION(DEVELOPMENT)
			for ( unsigned int i = 0; i < pRoom->pnRoomModels.Count(); i++ )
				if ( pRoom->pnRoomModels[ i ] == nModelId )
					ASSERTV(0, "Duplicate room found!" );
#endif
			ArrayAddItem(pRoom->pnRoomModels, nModelId);
#if ISVERSION(DEVELOPMENT)
			for ( unsigned int i = 0; i < pRoom->pnModels.Count(); i++ )
				if ( pRoom->pnModels[ i ] == nModelId )
					ASSERTV(0, "Wrong-list room found!" );
#endif
		}
		else
		{
#if ISVERSION(DEVELOPMENT)
			for ( unsigned int i = 0; i < pRoom->pnModels.Count(); i++ )
				if ( pRoom->pnModels[ i ] == nModelId )
					ASSERTV(0, "Duplicate room found!" );
#endif
			ArrayAddItem(pRoom->pnModels, nModelId);
#if ISVERSION(DEVELOPMENT)
			for ( unsigned int i = 0; i < pRoom->pnRoomModels.Count(); i++ )
				if ( pRoom->pnRoomModels[ i ] == nModelId )
					ASSERTV(0, "Wrong-list room found!" );
#endif
		}
	}

	c_ModelSetRegion( nModelId, RoomGetSubLevelEngineRegion( pRoom ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RoomRemoveModel ( ROOM * pRoom, int nModelId )
{
	if (! pRoom)
		return;

	// these will exit cleanly if it doesn't exist
	// this looks slow... have to profile
	ArrayRemoveByValue(pRoom->pnRoomModels, nModelId);		// slow !!!!
	ArrayRemoveByValue(pRoom->pnModels, nModelId);			// slow !!!!
}

#ifdef HAVOK_ENABLED
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void RoomAddRigidBody ( ROOM * pRoom, class hkRigidBody * pRigidBody )
{
	if (! pRoom) 
		return;

	ArrayAddItem(pRoom->pRoomModelRigidBodies, pRigidBody);
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_RoomAddInvisibleRigidBody ( ROOM * pRoom, class hkRigidBody * pRigidBody )
{
	if (! pRoom) 
		return;

	ArrayAddItem(pRoom->pRoomModelInvisibleRigidBodies, pRigidBody);
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL c_RoomIsVisible ( ROOM * pRoom )
{
	//return (e_ModelGetFlags( RoomGetRootModel( pRoom ) ) & MODEL_FLAG_VISIBLE) != 0;
	return c_GetToolMode() || e_ModelCurrentVisibilityToken( RoomGetRootModel( pRoom ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_SetRoomModelsVisible( const ROOM * pRoom, BOOL bFrustumCull, const PLANE * pPlanes /*= NULL*/ )
{
	if ( bFrustumCull && ! e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) )
		return;
	ASSERT_RETURN( pPlanes || ! bFrustumCull );
	ASSERT( pRoom );
	int nRootModel = RoomGetRootModel( pRoom );
	for ( unsigned int nModel = 0; nModel < pRoom->pnRoomModels.Count(); nModel++ )
	{
		if ( pRoom->pnRoomModels[nModel] == nRootModel ||
			( ! bFrustumCull || e_ModelInFrustum( pRoom->pnRoomModels[ nModel ], pPlanes ) ) )
		{
			e_ModelUpdateVisibilityToken( pRoom->pnRoomModels[ nModel ] );
		}
	}
	for ( unsigned int nModel = 0; nModel < pRoom->pnModels.Count(); nModel++ )
	{
		if ( ! bFrustumCull || e_ModelInFrustum( pRoom->pnModels[ nModel ], pPlanes ) )
		{
			e_ModelUpdateVisibilityToken( pRoom->pnModels[ nModel ] );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// ASSUMES room is in frustum (and checks anyway)
// sets static models visible
// frustum culls dynamic models, setting visible flag to the result
void c_RoomCullModels( ROOM * pRoom )
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! pRoom )
		return;

	if ( ! e_ModelGetInFrustum( RoomGetRootModel( pRoom ) ) )
	{
		//ASSERTX( 0, "Room outside frustum sent for model culling!" );		// sometimes this happens when it's the control room when entering a level
		//c_SetRoomModelsVisible( pRoom, FALSE );
		return;
	}

	VIEW_FRUSTUM * pFrustum = e_GetCurrentViewFrustum();

	// static models: mark visible (they are marked so that their meshes will be frustum culled)
	for ( unsigned int nModelIndex = 0; nModelIndex < pRoom->pnRoomModels.Count(); nModelIndex++ )
	{
		e_ModelUpdateVisibilityToken( pRoom->pnRoomModels[ nModelIndex ] );
	}

	// dynamic models: frustum cull now
	for ( unsigned int nModelIndex = 0; nModelIndex < pRoom->pnModels.Count(); nModelIndex++ )
	{
		int nModelID = pRoom->pnModels[ nModelIndex ];
		if ( ! e_ModelInFrustum( nModelID, pFrustum->GetPlanes() ) )
		{
			//e_ModelSetVisible( nModelID, FALSE );
			continue;
		}
		e_ModelUpdateVisibilityToken( nModelID );
	}
#endif //SERVER_VERSION
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//static int sVisibleRoomLessThan( VISIBLE_ROOM_COMMAND * pFirst, void *pKey )
//{
//	int nKey = *(int *)pKey;
//	if ( nKey > pFirst->nSortValue )
//	{
//		return -1;
//	}
//	else if ( nKey < pFirst->nSortValue )
//	{
//		return 1;
//	}
//	return 0;
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_AddVisibleRoom( ROOM * pRoom, BOUNDING_BOX & ClipBox )
{
	// if the room is already in the visible list in this pass, union with its clip box
	int nCount = gtVisibleRoomCommands.Count();

	if ( gbDumpRooms )
		trace( "[ Add Visible Room ]  room:%4d  pass:%2d  ", pRoom->idRoom, gnCurrentVisibleRoomPass );

	// TODO
	// just track the current pass globally...

	// walk backwards to the beginning of this pass
	//int nStart = nCount - 1;
	//for ( ; nStart >= 0; nStart-- )
	//{
	//	VISIBLE_ROOM_COMMAND & tCommand = gtVisibleRoomCommands[ nStart ];
	//	if ( tCommand.nCommand == VRCOM_BEGIN_SECTION || tCommand.nCommand == VRCOM_END_SECTION )
	//	{
	//		ASSERT( tCommand.nPass == gnCurrentVisibleRoomPass );
	//		break;
	//	}
	//}

	//nStart = max( nStart, 0 );

	for ( int i = 0; i < nCount; i++ )
	{
		VISIBLE_ROOM_COMMAND & tCommand = gtVisibleRoomCommands[ i ];
		if ( tCommand.nCommand != VRCOM_ADD_ROOM )
			continue;

		if ( tCommand.nPass == gnCurrentVisibleRoomPass && tCommand.dwData == (DWORD_PTR)pRoom )
		{
			if ( gbDumpRooms )
				trace( "found, merging clip\n" );

			// union the clip boxes
			tCommand.ClipBox.vMin.fX = min( tCommand.ClipBox.vMin.fX, ClipBox.vMin.fX );
			tCommand.ClipBox.vMin.fY = min( tCommand.ClipBox.vMin.fY, ClipBox.vMin.fY );
			tCommand.ClipBox.vMax.fX = max( tCommand.ClipBox.vMax.fX, ClipBox.vMax.fX );
			tCommand.ClipBox.vMax.fY = max( tCommand.ClipBox.vMax.fY, ClipBox.vMax.fY );
			return;
		}
	}


	// not found; add it
	{
		//VISIBLE_ROOM * pVisRoom = gtVisibleRooms.InsertSorted( &nID, &nSortValue, sVisibleRoomLessThan );
		VISIBLE_ROOM_COMMAND tCommand;
		tCommand.dwData		= (DWORD_PTR)pRoom;
		tCommand.ClipBox	= ClipBox;
		tCommand.nCommand	= VRCOM_ADD_ROOM;
		tCommand.nPass		= gnCurrentVisibleRoomPass;
		//tCommand.nSortValue = nSortValue;

		if ( gbDumpRooms )
			trace( "not found, adding\n" );

		sgnRoomTraversalCount++;
		ArrayAddItem(gtVisibleRoomCommands, tCommand);

		if ( e_GetRenderFlag( RENDER_FLAG_LIST_VISIBLE_ROOMS ) )
			e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, &ClipBox, 0xff1fff1f );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UpdateLights()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS ) )
		return;

	e_LightsUpdateAll();
	e_LightSlotsUpdate();
}
#endif// !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomIsPlayerInConnectedRoom(
	GAME* game, 
	ROOM * pRoom,
	const VECTOR & vTargetingLocation,
	float fRange)
{
	if ( ! pRoom )
		return FALSE;
	float fMaxRangeSquared = fRange * fRange;
	int nearbyRoomCount = RoomGetNearbyRoomCount(pRoom);
	for (int ii = -1; ii < nearbyRoomCount; ii++)
	{
		ROOM * pRoomTest = (ii == -1) ? pRoom : RoomGetNearbyRoom(pRoom, ii);

		if ( ! pRoomTest )
			continue;

		float fRangeToRoom = ConvexHullManhattenDistance( pRoomTest->pHull, &vTargetingLocation );
		if ( fRangeToRoom * fRangeToRoom > fMaxRangeSquared )
			continue;

		UNIT * pUnitCurr = pRoomTest->tUnitList.ppFirst[ TARGET_GOOD ];

		for ( ; pUnitCurr; pUnitCurr = pUnitCurr->tRoomNode.pNext )
		{
			if ( UnitHasClient( pUnitCurr ) && 
				VectorDistanceSquared( vTargetingLocation, UnitGetPosition( pUnitCurr ) ) < fMaxRangeSquared )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomIsHellrift(
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	SUBLEVELID idSubLevel = RoomGetSubLevelID( pRoom );
	SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	return SubLevelGetType( pSubLevel ) == ST_HELLRIFT;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_RoomGetPlayerCount(
	GAME *pGame,
	const ROOM *pRoom)
{
	ASSERTX_RETZERO( pGame && IS_SERVER( pGame ), "Server only" );
	ASSERTX_RETZERO( pRoom, "Expected room" );	
	
	int nCount = 0;

	// we could make this more intelligent one day ... especially if we really go MMO	
	// TRAVIS:: Note we're not getting clients ingame - that's because due to what 
	// appear to be some changes in the order in which events get processed on the server,
	// you can have a player still zoning in who has a valid room, but is still connecting.
	// If we don't treat them as a valid client in the level, then they can get wiped out on a repopulate -
	// and that's bad, bad, bad!
	for (GAMECLIENT *pClient = GameGetNextClient( pGame, NULL ); 
		 pClient; 
		 pClient = GameGetNextClient( pGame, pClient ))	
	{
		UNIT *pControlUnit = ClientGetControlUnit( pClient, TRUE );
		if (pControlUnit)
		{
			if (UnitGetRoom( pControlUnit ) == pRoom)
			{
				nCount++;
			}
		}
	}
	
	return nCount;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sMakePlayerSafe(
	UNIT *pUnit,
	void *pCallbackData)
{
	ASSERTX_RETVAL( pUnit, RIU_CONTINUE, "Expected unit" );
	
	if (UnitIsPlayer( pUnit ))
	{
		UnitSetSafe( pUnit );
	}
	
	return RIU_CONTINUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RoomMakePlayersSafe(
	ROOM *pRoom)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	
	// iterate players in level
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	RoomIterateUnits( pRoom, eTargetTypes, sMakePlayerSafe, NULL );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_RoomReset(
	GAME *pGame,
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	
	// do nothing if we cannot reset ... we assume the logic that set this cannot reset bit
	// will properly be handling the case of allowing the room to reset at some point
	if (RoomTestFlag( pRoom, ROOM_CANNOT_RESET_BIT ) == TRUE)
	{
		return FALSE;
	}
	if( AppIsTugboat() && s_sIsPortalReturningPlayerInRoomActivationRange(pRoom ) )
	{
		return FALSE;
	}


	// remove units created during content phase
	s_sRoomFreePopulates( pGame, pRoom, RPP_CONTENT );

	// room is no longer populated	
	RoomSetFlag( pRoom, ROOM_POPULATED_BIT, FALSE );	
	
	// if the room is active, populate it now, otherwise just leave it as is and the next
	// time the room becomes active it will be automatically populated
	if (RoomIsActive(pRoom) == TRUE)
	{
		// make any players inside this room "safe" from a respawn storm ... cause that sucks
		s_RoomMakePlayersSafe(pRoom);

		// populate	
		s_sRoomPopulate( pRoom, NULL );
	}
		
	return TRUE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sRoomResetTimerExpired(
	GAME *pGame,
	ROOM * pRoom )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( pGame && IS_SERVER( pGame ), "Server only" );
	ASSERTX_RETURN( pRoom, "Expected room" );

	BOOL bDoReset = FALSE;	

	// in Mythos - we never respawn levels unless they are shared
	if( AppIsTugboat() )
	{
		LEVEL*  pLevel = RoomGetLevel( pRoom );
		int nLevelAreaID = LevelGetLevelAreaID( pLevel );
		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( nLevelAreaID );
		int nOpenPlayers = LevelGetPVPWorld( pLevel ) ?
			pLevelArea->nMaxOpenPlayerSizePVPWorld : pLevelArea->nMaxOpenPlayerSize;
		if( nOpenPlayers <= 1 )
		{
			return;
		}

	}
	
	// if the room is now active again, forget it
	if (RoomIsActive(pRoom) == FALSE)
	{
	
		// verify that there are no players in the room
		if (s_RoomGetPlayerCount( pGame, pRoom ) == 0)
		{
			ROOM_MONSTER_COUNT_TYPE eCountType = RMCT_DENSITY_VALUE_OVERRIDE;
			
			// check monster room resetting restrictions
			if (pRoom->nMonsterCountOnPopulate[ eCountType ] > 0)
			{
			
				// how many monsters are in this room right now
				int nCurrentMonsterCount = RoomGetMonsterCount( pRoom, eCountType );
				
				// what's the percentage of monsters left compared to the original spawn
				LEVEL *pLevel = RoomGetLevel( pRoom );
				ASSERTX_RETURN( pLevel, "Expected level" );
				const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );	
				int nPercentMonstersLeft = (int)(((float)nCurrentMonsterCount / (float)pRoom->nMonsterCountOnPopulate[ eCountType ]) * 100.0f);
				
				// we won't reset this room unless enough monsters have died (checked by percent)
				if (ptLevelDef->nRoomResetMonsterPercent == NONE ||
					nPercentMonstersLeft <= ptLevelDef->nRoomResetMonsterPercent)
				{
					bDoReset = TRUE;
				}

			}
			else if( AppIsTugboat() )
			{
				bDoReset = TRUE;
			}
		}

	}
					
	// reset room or set begin timer again
	if (bDoReset == TRUE)
	{
		s_RoomReset( pGame, pRoom );
		// reset/clear
		pRoom->dwResetDelay = 0;
		pRoom->tResetTick.tick = 0;
	}
#else
	REF( pGame );
	REF( pRoom );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sRoomCheckReset(
	ROOM * pRoom )
{
	ASSERTX_RETURN( pRoom, "Expected room" );

	// no reset on this level for some reason?
	if ( ( pRoom->tResetTick.tick == 0 ) && ( pRoom->dwResetDelay == 0 ) )
	{
		return;
	}

	GAME * pGame = RoomGetGame( pRoom );
	GAME_TICK current_tick = GameGetTick( pGame );
	DWORD dwDelta = current_tick.tick - pRoom->tResetTick.tick;
	if ( dwDelta > pRoom->dwResetDelay )
	{
		s_sRoomResetTimerExpired( pGame, pRoom );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RoomStartResetTimer(
	ROOM *pRoom,
	float flDelayMultiplier,
	DWORD dwDelayOverride)
{
	ASSERTX_RETURN( pRoom, "Expected room" );

	// if the info is set leave it alone
	if ( ( pRoom->tResetTick.tick != 0 ) || ( pRoom->dwResetDelay != 0 ) )
	{
		return;
	}

	// reset/clear
	pRoom->tResetTick.tick = 0;
	pRoom->dwResetDelay = 0;
		
	// we will only currently re-populate action levels where there are monsters
	// and quests we want to allow other people to do again
	LEVEL *pLevel = RoomGetLevel( pRoom );
	if (LevelIsActionLevel( pLevel ) == TRUE)
	{
		// see what the room reset delay for this level is (if any)
		const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );
		int nDelayInSeconds = (int)(ptLevelDef->nRoomResetDelayInSeconds * flDelayMultiplier);

		if (ptLevelDef->bRoomResetEnabled == TRUE && nDelayInSeconds != NONE)
		{
			// use override, or calculate
			if ( dwDelayOverride != 0 )
				pRoom->dwResetDelay = dwDelayOverride;
			else
				pRoom->dwResetDelay = GAME_TICKS_PER_SECOND * nDelayInSeconds;
						
			// save which tick it started at
			pRoom->tResetTick = GameGetTick( LevelGetGame( pLevel ) );
		}
	}
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PATH_NODE_EDGE_CONNECTION_MAP
{
	PATH_NODE_EDGE_CONNECTION	tEdgeConnection;
	int							nEdgeIndex;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int sSortEdgeConnectionCompare(const void *arg1, const void *arg2)
{
	const PATH_NODE_EDGE_CONNECTION_MAP * pLeft = (const PATH_NODE_EDGE_CONNECTION_MAP *)arg1;
	const PATH_NODE_EDGE_CONNECTION_MAP * pRight = (const PATH_NODE_EDGE_CONNECTION_MAP *)arg2;
	if(pLeft->nEdgeIndex < pRight->nEdgeIndex)
	{
		return -1;
	}
	else if(pLeft->nEdgeIndex > pRight->nEdgeIndex)
	{
		return 1;
	}

	if(pLeft->tEdgeConnection.fDistance == pRight->tEdgeConnection.fDistance)
	{
		return 0;
	}
	else if(pLeft->tEdgeConnection.fDistance < pRight->tEdgeConnection.fDistance)
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFindEdgeConnectionCompare(const void * arg1, const void * arg2)
{
	const PATH_NODE_EDGE_CONNECTION_MAP * pLeft = (const PATH_NODE_EDGE_CONNECTION_MAP *)arg1;
	const PATH_NODE_EDGE_CONNECTION_MAP * pRight = (const PATH_NODE_EDGE_CONNECTION_MAP *)arg2;

	const int IDENTICAL = 0;
	const int DIFFERENT = 1;

	int nEdgeIndexLeft = PathNodeEdgeConnectionGetEdgeIndex(&pLeft->tEdgeConnection);
	int nEdgeIndexRight = PathNodeEdgeConnectionGetEdgeIndex(&pRight->tEdgeConnection);
	if(pLeft->nEdgeIndex == pRight->nEdgeIndex &&
		nEdgeIndexLeft == nEdgeIndexRight &&
		pLeft->tEdgeConnection.pRoom == pRight->tEdgeConnection.pRoom)
	{
		return IDENTICAL;
	}
	return DIFFERENT;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define WORKING_MEMORY_SIZE 4096


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomCreatePathEdgeNodeConnections(
	GAME * game,
	ROOM * room,
	ROOM * roomOther,
	PATH_NODE_EDGE_CONNECTION_MAP * WorkingMemory,
	unsigned int & nWorkingMemoryCount)
{
	// iterate all edge nodes in this room
	int edgecount = room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount;
	for (int ii = 0; ii < edgecount; ++ii)
	{

		ROOM_PATH_NODE * pathnode = room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].pEdgeNodes[ii];
		// get world position for this node
		VECTOR vWorldPosThisNode = RoomPathNodeGetExactWorldPosition(game, room, pathnode);

		// look only at edge nodes close enough to the other room
		float fDistanceToHull = ConvexHullManhattenDistance(roomOther->pHull, &vWorldPosThisNode);
		if (fDistanceToHull > room->pPathDef->fRadius * 2.0f)
		{
			continue;
		}

		// transform the pathnode into the other room's roomspace
		VECTOR vPositionInOtherRoomsSpace;
		MatrixMultiply(&vPositionInOtherRoomsSpace, &vWorldPosThisNode, &roomOther->tWorldMatrixInverse);

		// get a list of edge nodes in the other room close enough to this node
		NEAREST_NODE_SPEC tNearestNodeSpec;
		tNearestNodeSpec.fMaxDistance = room->pPathDef->fRadius;
		SETBIT(tNearestNodeSpec.dwFlags, NPN_ONE_ROOM_ONLY);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_USE_GIVEN_ROOM);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_POSITION_IS_ALREADY_IN_ROOM_SPACE);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_ALLOW_BLOCKED_NODES);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_EDGE_NODES_ONLY);
		SETBIT(tNearestNodeSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
		FREE_PATH_NODE NearbyEdgeNodes[MAX_INSTANCE_CONNECTION_COUNT];
		int NearbyEdgeNodeCount = RoomGetNearestPathNodes(game, roomOther, vPositionInOtherRoomsSpace, MAX_INSTANCE_CONNECTION_COUNT, NearbyEdgeNodes, &tNearestNodeSpec);

		// iterate list of edge nodes close enough to this one
		for (int jj = 0; jj < NearbyEdgeNodeCount; ++jj)
		{
			// these nodes could potentially connect
			FREE_PATH_NODE * fpn = &NearbyEdgeNodes[jj];
			BOOL found = FALSE;
			for (int kk = 0; kk < jj; ++kk)
			{
				if (NearbyEdgeNodes[jj].pNode->nEdgeIndex == NearbyEdgeNodes[kk].pNode->nEdgeIndex)
				{
					found = TRUE;
					break;
				}
			}
			if (found)
			{
				continue;
			}
#if ISVERSION(DEVELOPMENT)
			for (unsigned int kk = 0; kk < nWorkingMemoryCount; ++kk)
			{
				ASSERT(WorkingMemory[kk].tEdgeConnection.pRoom != roomOther ||  WorkingMemory[kk].tEdgeConnection.nEdgeIndex != fpn->pNode->nEdgeIndex || 
					WorkingMemory[kk].nEdgeIndex != ii);
			}
#endif
			VECTOR vWorldPosOtherNode = RoomPathNodeGetExactWorldPosition(game, roomOther, fpn->pNode);

			float distance = sqrtf(fpn->flDistSq);
			WorkingMemory[nWorkingMemoryCount].nEdgeIndex = ii;
			WorkingMemory[nWorkingMemoryCount].tEdgeConnection.pRoom = roomOther;
			WorkingMemory[nWorkingMemoryCount].tEdgeConnection.nEdgeIndex = fpn->pNode->nEdgeIndex;
			WorkingMemory[nWorkingMemoryCount].tEdgeConnection.fDistance = distance;
			++nWorkingMemoryCount;
			ASSERTV_BREAK(nWorkingMemoryCount < WORKING_MEMORY_SIZE, "Working memory count has been hit for room [%s]", RoomGetDevName(room));
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomCopyExistingEdgeNodeConnectionsToWorkingMemory(
	GAME * game,
	ROOM * room,
	PATH_NODE_EDGE_CONNECTION_MAP * WorkingMemory,
	unsigned int & nWorkingMemoryCount)
{
	nWorkingMemoryCount = room->nEdgeNodeConnectionCount;

	// copy edge instances
	for (int ii = 0; ii < room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount; ++ii)
	{
		EDGE_NODE_INSTANCE * edgeInstance = &room->pEdgeInstances[ii];
		int start = edgeInstance->nEdgeConnectionStart;
		int end = start + edgeInstance->nEdgeConnectionCount;
		for (int jj = start; jj < end; ++jj)
		{
			WorkingMemory[jj].tEdgeConnection = room->pEdgeNodeConnections[jj];
			WorkingMemory[jj].nEdgeIndex = ii;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomCopyWorkingMemoryToRoomEdgeNodeConnections(
	GAME * game,
	ROOM * room,
	PATH_NODE_EDGE_CONNECTION_MAP * WorkingMemory,
	unsigned int nWorkingMemoryCount)
{
	qsort(WorkingMemory, nWorkingMemoryCount, sizeof(PATH_NODE_EDGE_CONNECTION_MAP), sSortEdgeConnectionCompare);
	ArrayResize(room->pEdgeNodeConnections, nWorkingMemoryCount);
	room->nEdgeNodeConnectionCount = nWorkingMemoryCount;

	int nLastEdgeConnection = INVALID_ID;
	int nLastEdgeCount = 0;
	for (unsigned int ii = 0; ii < nWorkingMemoryCount; ++ii)
	{
#if ISVERSION(DEVELOPMENT)
		// assert there are no duplicates
		if (ii > 0)
		{
			ASSERT(WorkingMemory[ii].tEdgeConnection.pRoom != WorkingMemory[ii - 1].tEdgeConnection.pRoom ||
				PathNodeEdgeConnectionGetEdgeIndex(&WorkingMemory[ii].tEdgeConnection) != PathNodeEdgeConnectionGetEdgeIndex(&WorkingMemory[ii - 1].tEdgeConnection) ||
				WorkingMemory[ii].nEdgeIndex != WorkingMemory[ii - 1].nEdgeIndex);
		}
#endif
		room->pEdgeNodeConnections[ii] = WorkingMemory[ii].tEdgeConnection;
		if (WorkingMemory[ii].nEdgeIndex != nLastEdgeConnection)
		{
			if (nLastEdgeConnection >= 0)
			{
				room->pEdgeInstances[nLastEdgeConnection].nEdgeConnectionCount = (WORD)nLastEdgeCount;
			}
			nLastEdgeConnection = WorkingMemory[ii].nEdgeIndex;
			room->pEdgeInstances[nLastEdgeConnection].nEdgeConnectionStart = (WORD)ii;
			nLastEdgeCount = 0;
		}
		nLastEdgeCount++;
	}
	if (nLastEdgeConnection >= 0)
	{
		room->pEdgeInstances[nLastEdgeConnection].nEdgeConnectionCount = (WORD)nLastEdgeCount;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void RoomInitEdgeNodeConnections(
	GAME * game,
	ROOM * room)
{
	ASSERT_RETURN(room);
	room->pEdgeInstances.Init(GameGetMemPool(game));
	ASSERT(room->pPathDef->pPathNodeSets);
	if (room->pPathDef->pPathNodeSets && room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount > 0)
	{
		ArrayResize(room->pEdgeInstances, room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount);
		for (int ii = 0; ii < room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount; ++ii)
		{
			room->pEdgeInstances[ii].nEdgeConnectionStart = 0;
			room->pEdgeInstances[ii].nEdgeConnectionCount = 0;
		}
	}
	room->pEdgeNodeConnections.Init(GameGetMemPool(game));
	room->nEdgeNodeConnectionCount = 0;
	RoomSetFlag(room, ROOM_EDGENODE_CONNECTIONS, FALSE);
}


//----------------------------------------------------------------------------
// free a room's edge node connections w/o removing them from other rooms
// (use only when doing this for entire level)
//----------------------------------------------------------------------------
void RoomFreeEdgeNodeConnections(
	GAME * game,
	ROOM * room)
{
	ASSERT_RETURN(room);
	if (RoomTestFlag(room, ROOM_EDGENODE_CONNECTIONS))
	{
		trace("RoomFreeEdgeNodeConnections()  ROOM: [%d: %s]\n", RoomGetIndexInLevel(room), RoomGetDevName(room));
	}

	room->pEdgeInstances.Free();
	room->pEdgeNodeConnections.Free();
	room->nEdgeNodeConnectionCount = 0;
	RoomSetFlag(room, ROOM_EDGENODE_CONNECTIONS, FALSE);
}


//----------------------------------------------------------------------------
// called on creating a room to create edge connections for a room
// bTwoWay is TRUE for client only
//
// each room has an array of EDGE_NODE_INSTANCE which corresponds one-to-one
// to the edge nodes for the room.  each EDGE_NODE_INSTANCE points to a
// unique section of the room's PATH_NODE_EDGE_CONNECTION array which
// describes each connection that edge node has with nearby edge nodes of
// neighboring rooms
//----------------------------------------------------------------------------
void RoomCreatePathEdgeNodeConnections(
	GAME * game,
	ROOM * room,
	BOOL bTwoWay)
{
	ASSERT_RETURN(game);
	if (room == NULL || room->pPathDef == NULL || room->pPathDef->pPathNodeSets == NULL || !room->pHull || 
		room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount <= 0)
	{
		TRACE_ACTIVE("ROOM: [%d: %s]  RoomCreatePathEdgeNodeConnections()  NO PATH NODES\n", RoomGetIndexInLevel(room), RoomGetDevName(room));
		return;
	}

	if (RoomTestFlag(room, ROOM_EDGENODE_CONNECTIONS))
	{
		return;
	}

	RoomFreeEdgeNodeConnections(game, room);
	RoomInitEdgeNodeConnections(game, room);

	TRACE_ACTIVE("ROOM: [%d: %s]  RoomCreatePathEdgeNodeConnections()\n", RoomGetId(room), RoomGetDevName(room));

	RoomSetFlag(room, ROOM_EDGENODE_CONNECTIONS);

	PATH_NODE_EDGE_CONNECTION_MAP tWorkingMemory[WORKING_MEMORY_SIZE];
	unsigned int nWorkingMemoryCount = 0;

	// get neighboring rooms
	int neighbors = RoomGetAdjacentRoomCount(room);
	for (int ii = 0; ii < neighbors; ++ii)
	{
		ROOM * roomNeighbor = RoomGetAdjacentRoom(room, ii);
		if (!roomNeighbor || roomNeighbor == room || !roomNeighbor->pPathDef || !roomNeighbor->pPathDef->pPathNodeSets || !roomNeighbor->pHull || 
			roomNeighbor->pPathDef->pPathNodeSets[roomNeighbor->nPathLayoutSelected].nEdgeNodeCount <= 0)
		{
			continue;
		}

		sRoomCreatePathEdgeNodeConnections(game, room, roomNeighbor, tWorkingMemory, nWorkingMemoryCount);
		if (bTwoWay)
		{
			PATH_NODE_EDGE_CONNECTION_MAP tWorkingMemoryTwoWay[WORKING_MEMORY_SIZE];
			unsigned int nWorkingMemoryCountTwoWay = 0;
			sRoomCopyExistingEdgeNodeConnectionsToWorkingMemory(game, roomNeighbor, tWorkingMemoryTwoWay, nWorkingMemoryCountTwoWay);
			sRoomCreatePathEdgeNodeConnections(game, roomNeighbor, room, tWorkingMemoryTwoWay, nWorkingMemoryCountTwoWay);
			sRoomCopyWorkingMemoryToRoomEdgeNodeConnections(game, roomNeighbor, tWorkingMemoryTwoWay, nWorkingMemoryCountTwoWay);
		}
	}
	sRoomCopyWorkingMemoryToRoomEdgeNodeConnections(game, room, tWorkingMemory, nWorkingMemoryCount);
}


//----------------------------------------------------------------------------
// deletes all connections to the tgtRoom from this room
// this leaves "gaps" in room->pEdgeNodeConnections
//----------------------------------------------------------------------------
static void sRoomRemoveEdgeNodeConnections(
	GAME * game,
	ROOM * room,
	ROOM * tgtRoom)
{
	ASSERT_RETURN(room->pPathDef);
	int edgecount = room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount;

	// iterate all edge nodes
	for (int ii = 0; ii < edgecount; ++ii)
	{
		EDGE_NODE_INSTANCE * edgenode = &room->pEdgeInstances[ii];
		
		// iterate all connections
		int start = edgenode->nEdgeConnectionStart;
		int end = start + edgenode->nEdgeConnectionCount;
		int curr = start;
		for (int jj = start; jj < end; ++jj)
		{
			PATH_NODE_EDGE_CONNECTION * connection = &(room->pEdgeNodeConnections[jj]);
			if (connection->pRoom != tgtRoom)
			{
				// shift jj up
				if (jj != curr)
				{
					room->pEdgeNodeConnections[curr] = room->pEdgeNodeConnections[jj];
				}
				++curr;
				continue;
			}
			// remove this connection
			--edgenode->nEdgeConnectionCount;
		}

		// zero out invalid connections
		for (int jj = curr; jj < end; ++jj)
		{
			PATH_NODE_EDGE_CONNECTION * connection = &(room->pEdgeNodeConnections[jj]);
			memclear(connection, sizeof(PATH_NODE_EDGE_CONNECTION));
		}
	}
}


//----------------------------------------------------------------------------
// deletes a room's connections, as well as connections to this room
//----------------------------------------------------------------------------
void RoomRemoveEdgeNodeConnections(
	GAME * game,
	ROOM * room)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(room);

	if (room->pEdgeInstances.GetSize() == 0)
	{
		return;
	}
	ASSERT_RETURN(room->pPathDef && room->pPathDef->pPathNodeSets && room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nEdgeNodeCount > 0);

	// iterate neighboring rooms & delete their connections to this room
	int neighbors = RoomGetAdjacentRoomCount(room);
	for (int ii = 0; ii < neighbors; ++ii)
	{
		ROOM * roomNeighbor = RoomGetAdjacentRoom(room, ii);
		if (!roomNeighbor || roomNeighbor == room || !roomNeighbor->pPathDef ||
			roomNeighbor->pPathDef->pPathNodeSets[roomNeighbor->nPathLayoutSelected].nEdgeNodeCount <= 0)
		{
			continue;
		}
		sRoomRemoveEdgeNodeConnections(game, roomNeighbor, room);
	}

	RoomFreeEdgeNodeConnections(game, room);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomAddCollisionToLevel(
	GAME * game,
	LEVEL * level,
	ROOM * room)
{
	if (AppIsHammer())
	{
		return;
	}
	if (!AppIsTugboat())
	{
		return;
	}
	LevelCollisionAddRoom(game, level, room);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomSelectLayout(
	GAME * game,
	LEVEL * level,
	ROOM * room,
	const ROOM_INDEX * room_index)
{
	if (AppIsHammer())
	{
		return;
	}

	ASSERT_RETURN(room_index);

	char szFilePath[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf(szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", room_index->pszFile, room->szLayoutOverride, ROOM_LAYOUT_SUFFIX);
	room->pLayout = (ROOM_LAYOUT_GROUP_DEFINITION *)DefinitionGetByName(DEFINITION_GROUP_ROOM_LAYOUT, szFilePath);

	if (!room->pLayout)
	{
		return;
	}

	int * pnThemes = NULL;
	int nThemeIndexCount = LevelGetSelectedThemes(level, &pnThemes);	
	RoomLayoutSelectLayoutItems(game, room, room->pLayout, &room->pLayout->tGroup, room->dwRoomSeed, pnThemes, nThemeIndexCount);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomInstantiatePathNodes(
	GAME * game,
	ROOM * room)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(room);
	ASSERT_RETURN(room->pDefinition);

	const MATRIX * worldmatrix = &room->tWorldMatrix;

	if( AppIsHellgate() )
	{
		room->pwCollisionIter = (WORD *)GMALLOCZ(game, room->pDefinition->nRoomCPolyCount * sizeof(WORD));
	}
	else
	{
		room->pwCollisionIter = NULL;
	}

	room->nPathLayoutSelected = 0;
	room->pPathNodeInstances.Init(GameGetMemPool(game));
	room->nPathNodeCount = 0;
	RoomInitEdgeNodeConnections(game, room);

	if (room->pPathDef == NULL || room->pPathDef->pPathNodeSets == NULL)
	{
		return;
	}

	if ( room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nPathNodeCount)
	{
		ArrayResize(room->pPathNodeInstances, room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nPathNodeCount);
		room->nPathNodeCount = room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].nPathNodeCount;
		for (unsigned int ii = 0; ii < room->nPathNodeCount; ++ii)
		{
			PATH_NODE_INSTANCE * node = RoomGetPathNodeInstance(room, ii);
#if HELLGATE_ONLY
			MatrixMultiply(&node->vWorldPosition, &room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].pPathNodes[ii].vPosition, worldmatrix);
			MatrixMultiplyNormal(&node->vWorldNormal, &room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].pPathNodes[ii].vNormal, worldmatrix);
#else
			float fX = GetPathNodeX(room->pPathDef, room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].pPathNodes[ii].nOffset );
			float fY = GetPathNodeY( room->pPathDef, room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].pPathNodes[ii].nOffset );
			VECTOR vPos( fX, fY, 0 );
			MatrixMultiply(&node->vWorldPosition, &vPos, worldmatrix);
#endif

#if HELLGATE_ONLY
			node->dwNodeInstanceFlags = 0;
#else
			node->dwNodeInstanceFlags = 0;
			if( room->pMaskDef )
			{
				if( !room->pMaskDef->pbPassability[room->pPathDef->pPathNodeSets[room->nPathLayoutSelected].pPathNodes[ii].nOffset] )
				{
					SETBIT( node->dwNodeInstanceFlags, NIF_BLOCKED );
					SETBIT( node->dwNodeInstanceFlags, NIF_NO_SPAWN );
				}
			}
#endif
		}
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_PATH_MASK_DEFINITION* RoomPathMaskDefinitionGet(
	const char* pszFileNameWithPath)
{
	if (pszFileNameWithPath[0] == 0)
	{
		return NULL;
	}



	if ( AppCommonIsAnyFillpak() && ! FillPakShouldLoad( pszFileNameWithPath ) )
		return NULL;

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
	}

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
	}

	//char pszMaskFileName[MAX_XML_STRING_LENGTH];
	//PStrReplaceExtension(pszMaskFileName, MAX_XML_STRING_LENGTH, pszFileNameWithPath, PATH_MASK_FILE_EXTENSION);
	//PStrPrintf( pszMaskFileName, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s",pszFileNameWithPath, PATH_MASK_FILE_EXTENSION);

	DECLARE_LOAD_SPEC( spec, pszFileNameWithPath );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	BYTE* pbyFileStart = (BYTE*)PakFileLoadNow(spec);

	ASSERTV_RETNULL( pbyFileStart, "Path mask file not found!\n%s", pszFileNameWithPath );

	FILE_HEADER* pFileHeader = (FILE_HEADER*)pbyFileStart;
	ASSERT(pFileHeader->dwMagicNumber == PATH_MASK_FILE_MAGIC_NUMBER);
	if (pFileHeader->dwMagicNumber != PATH_MASK_FILE_MAGIC_NUMBER)
	{
		FREE(NULL, pbyFileStart);
		return NULL;
	}
	ASSERT(pFileHeader->nVersion == PATH_MASK_FILE_VERSION);
	if (pFileHeader->nVersion != PATH_MASK_FILE_VERSION)
	{
		FREE(NULL, pbyFileStart);
		return NULL;
	}

	ROOM_PATH_MASK_DEFINITION* pDefinition = (ROOM_PATH_MASK_DEFINITION*)(pbyFileStart + sizeof(FILE_HEADER));
	pDefinition->tHeader.nId = INVALID_ID;
	PStrCopy(pDefinition->tHeader.pszName, pszFileNameWithPath, MAX_XML_STRING_LENGTH);
	pDefinition->pbyFileStart = pbyFileStart;
	pDefinition->pbPassability = (BOOL *)(pbyFileStart + (int)CAST_PTR_TO_INT(pDefinition->pbPassability));


	return pDefinition;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomInstantiatePathNodes(
	GAME * game,
	ROOM * room,
	const ROOM_INDEX * room_index)
{
	if (AppIsHammer())
	{
		return;
	}

	ASSERT_RETURN(room_index);

	char szFilePath[DEFAULT_FILE_WITH_PATH_SIZE];
	BOOL bLoadMask = FALSE;
	if( room_index->pszOverrideNodes && strlen( room_index->pszOverrideNodes ) > 0 )
	{
		PStrPrintf(szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", room_index->pszOverrideNodes, ROOM_PATH_NODE_SUFFIX);
		bLoadMask = TRUE;
	}
	else
	{
		PStrPrintf(szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", room_index->pszFile, ROOM_PATH_NODE_SUFFIX);
	}
	ROOM_PATH_NODE_DEFINITION * path_node_definition = (ROOM_PATH_NODE_DEFINITION *)DefinitionGetByName(DEFINITION_GROUP_ROOM_PATH_NODE, szFilePath);
#if ISVERSION(DEVELOPMENT)
	if (!path_node_definition || path_node_definition->nPathNodeSetCount <= 0)
	{
		ASSERTV(path_node_definition && path_node_definition->nPathNodeSetCount > 0, "Expected path file: %s", szFilePath);
	}
	if (!path_node_definition || !path_node_definition->pPathNodeSets ||
		(path_node_definition->pPathNodeSets[room->nPathLayoutSelected].nPathNodeCount <= 0 &&
		!(path_node_definition->dwFlags & ROOM_PATH_NODE_DEF_NO_PATHNODES_FLAG)))
	{
		ShowDataWarning("Room %s has no path nodes!", room->pDefinition->tHeader.pszName);
	}
#endif
	room->pPathDef = path_node_definition;


	if( bLoadMask )
	{
		PStrPrintf(szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s",FILE_PATH_BACKGROUND, room_index->pszFile, PATH_MASK_FILE_EXTENSION);
		room->pMaskDef = RoomPathMaskDefinitionGet( szFilePath );
	}
	else
	{
		room->pMaskDef = NULL;
	}

	sRoomInstantiatePathNodes(game, room);
	if( room->pMaskDef )
		FREE(NULL, room->pMaskDef->pbyFileStart);

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_sRoomCreateModels(
	GAME * game,
	ROOM * room,
	int & nRootModel)
{
	ASSERT_RETURN(game);
	if (!IS_CLIENT(game))
	{
		return;
	}

	ASSERT_RETURN(room);
	ASSERT_RETURN(room->pDefinition);

	const MATRIX * worldmatrix = &room->tWorldMatrix;

	VECTOR vPosition(0.0f, 0.0f, 0.0f);
	MatrixMultiply(&vPosition, &vPosition, worldmatrix);

	room->tWorldMatrix = *worldmatrix;
	V(e_ModelNew(&nRootModel, room->pDefinition->nModelDefinitionId));
	// TRAVIS : FIX - this causes particles to spawn appropriately
	c_ModelSetLocation(nRootModel, &room->tWorldMatrix, vPosition, RoomGetId(room), MODEL_STATIC);

	sRoomCreateAttachments(nRootModel, room);

	{
		ROOM_LAYOUT_SELECTION * layout_selection = RoomGetLayoutSelection(room, ROOM_LAYOUT_ITEM_ENVIRONMENTBOX);
		if (layout_selection)
		{
			for (int ii = 0; ii < layout_selection->nCount; ++ii)
			{
				// CHRIS: Add your code to copy the environment data to the engine here.
			}
		}
	}
	{
		ROOM_LAYOUT_SELECTION * layout_selection = RoomGetLayoutSelection(room, ROOM_LAYOUT_ITEM_STATIC_LIGHT);
		if (layout_selection)
		{
			for (int ii = 0; ii < layout_selection->nCount; ++ii)
			{
				STATIC_LIGHT_DEFINITION tStaticLight;
				RoomLayoutGetStaticLightDefinitionFromGroup(room, layout_selection->pGroups[ii], &layout_selection->pOrientations[ii], &tStaticLight);

				int nRootModel = RoomGetRootModel(room);
				c_AttachmentNewStaticLightCallback(nRootModel, tStaticLight, FALSE);
			}
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_sRoomSetupCulling(
	GAME * game,
	ROOM * room,
	int nRootModel)
{
	ASSERT_RETURN(game);
	if (!IS_CLIENT(game))
	{
		return;
	}

	ASSERT_RETURN(room);

	BOOL bThisOutdoor = RoomUsesOutdoorVisibility( room->idRoom );
	int nThisRegion = RoomGetSubLevelEngineRegion( room );
	int nThisCell = room->idRoom;
	if ( ! e_CellGet( room->idRoom ) )
	{
		ASSERTX( bThisOutdoor, "This room uses indoor culling but doesn't have a cell yet!" );
		V( e_RegionGetCell( nThisRegion, nThisCell ) );
	}

	for ( int ii = 0; ii < room->nConnectionCount; ii++ )
	{
		if ( ! room->pConnections[ii].pRoom )
			continue;

		ROOM * pOtherRoom = room->pConnections[ii].pRoom;

		// Are both rooms outdoor?
		BOOL bOtherOutdoor = RoomUsesOutdoorVisibility( pOtherRoom->idRoom );
		if ( bThisOutdoor && bOtherOutdoor )
			continue;	// nothing more needed

		// Check to see if we already connected these rooms.
		ROOM_CONNECTION * pOtherConn = &pOtherRoom->pConnections[ room->pConnections[ii].bOtherConnectionIndex ];
		ASSERT( pOtherConn->pRoom->idRoom == room->idRoom );
		if ( room->pConnections[ii].nCullingPortalID != INVALID_ID )
		{
			ASSERT( pOtherConn->nCullingPortalID != INVALID_ID );
			continue;
		}
		ASSERT( pOtherConn->nCullingPortalID == INVALID_ID );

		// If one room is outdoor and one indoor, we need to create a portal between one room's cell and the other's region's cell.
		int nOtherRegion = RoomGetSubLevelEngineRegion( pOtherRoom);
		int nOtherCell = INVALID_ID;
		if ( bOtherOutdoor )
		{
			// We know that ThisRoom is indoor, if we get here.
			V_CONTINUE( e_RegionGetCell( nOtherRegion, nOtherCell ) );
		}
		else
		{
			if ( ! e_CellGet( pOtherRoom->idRoom ) )
			{
				// We know that ThisRoom is outdoor, if we get here.
				V( e_CellCreateWithID( pOtherRoom->idRoom, RoomGetSubLevelEngineRegion( pOtherRoom ) ) );
			}
			nOtherCell = pOtherRoom->idRoom;
		}

		// All set.  Make the portals.
		V( e_CellCreatePhysicalPortalPair(
			room->pConnections[ii].nCullingPortalID,
			pOtherConn->nCullingPortalID,
			nThisCell,
			nOtherCell,
			room->pConnections[ii].pvBorderCorners,
			pOtherConn->pvBorderCorners,
			ROOM_CONNECTION_CORNER_COUNT,
			room->pConnections[ii].vBorderNormal,
			pOtherConn->vBorderNormal,
			NULL,
			NULL )
			);
	}

	// Set the root model location again, now that the culling cell is created.
	e_ModelSetRoomID( nRootModel, RoomGetId(room) );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef HAVOK_ENABLED
void sRoomSetupHavok(
	GAME * game,
	LEVEL * level,
	ROOM * room,
	const ROOM_INDEX * room_index)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(room);
	if (!room_index)
	{
		return;
	}

	//BOOL bTryToUseHavokFX = sLevelCanTryToUseHavokFX( game, level ); 

	// create havok object
	if (USE_SINGLE_MOPP)
	{
		VECTOR vPosition(0.0f, 0.0f, 0.0f);
		MatrixMultiply(&vPosition, &vPosition, &room->tWorldMatrix);
		QUATERNION qRoomQuat;
		QuaternionRotationMatrix(&qRoomQuat, &room->tWorldMatrix);
		hkRigidBody * pRigidBody = HavokCreateBackgroundRigidBody(game, 
			level->m_pHavokWorld, (hkShape*)room->pDefinition->tHavokShapeHolder.pHavokShape, &vPosition, &qRoomQuat, false);// bTryToUseHavokFX);//only use havokfx for multimopps 
		if (room_index)
		{
			pRigidBody->setUserData((void*)room_index);
		}
		RoomAddRigidBody(room, pRigidBody);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomAddPostProcess(
	GAME * game,
	LEVEL * level,
	ROOM * room,
	BOOL bLevelCreate)
{
	room->pLayout = NULL;
	RoomLayoutSelectionsInit(game, room->pLayoutSelections);
	const ROOM_INDEX * room_index = RoomGetRoomIndex(game, room);
	if (room_index)
	{
		sRoomAddCollisionToLevel(game, level, room);

		sRoomInstantiatePathNodes(game, room, room_index);

		sRoomSelectLayout(game, level, room, room_index);
	}

	int nRootModel = INVALID_ID;
#if !ISVERSION(SERVER_VERSION)
	c_sRoomCreateModels(game, room, nRootModel);
#endif

	if ( !bLevelCreate )
	{
		// we do this outside of this function now - Tyler
		ASSERT( IS_CLIENT( game ) );
		DRLGCopyConnectionsFromDefinition(game, room);
		DRLGCreateRoomConnections(game, level, room);

		if (AppIsHellgate())
		{
			DRLGCalculateVisibleRooms(game, level, room);
		}
	}

	// get bounding box of room in world space
	BoundingBoxTranslate(&room->tWorldMatrix, &room->tBoundingBoxWorldSpace, &room->pDefinition->tBoundingBox);
	room->vBoundingBoxCenterWorldSpace = BoundingBoxGetCenter(&room->tBoundingBoxWorldSpace);
	RoomGetBoundingSphereWorldSpace(room, room->tBoundingSphereWorldSpace.r, room->tBoundingSphereWorldSpace.C);

	RoomHookupNeighboringRooms(game, level, room, bLevelCreate);

	if (AppIsTugboat())
	{
		BOUNDING_BOX BBox;
		BBox.vMin = room->pHull->aabb.center - room->pHull->aabb.halfwidth;
		BBox.vMax = room->pHull->aabb.center + room->pHull->aabb.halfwidth;
		BBox.vMax.fZ = 4.2f;
		BBox.vMin.fZ = 0;
		level->m_pQuadtree->Add(room, BBox); 
	}
	else
	{
		// add the room to other rooms' lists
		if ( !bLevelCreate )
		{
			ASSERT( IS_CLIENT( game ) );
			for (unsigned int ii = 0; ii < room->nVisibleRoomCount; ii++)
			{
				ROOM * pVisibleRoom = room->ppVisibleRooms[ii];
				ASSERT_CONTINUE(pVisibleRoom);
				DRLGCalculateVisibleRooms(game, level, pVisibleRoom);
			}
		}

#if !ISVERSION(SERVER_VERSION)
		c_sRoomSetupCulling(game, room, nRootModel);
#endif

		sRoomCreateModels(game, level, room);
	
		if (IS_CLIENT(game))
		{
			DRLGCreateAutoMap(game, room);
	
			// apply saved automap info
			c_RoomApplyAutomapData(game, level, room);
		}

		if (IS_CLIENT(game))
		{
			c_sRoomPopulateCritters(game, room);
		}

#ifdef HAVOK_ENABLED
		sRoomSetupHavok(game, level, room, room_index);
#endif
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomDoSetup(
	GAME* game,
	LEVEL* level,
	ROOM* room)
{

	sRoomCreateModels(game, level, room);

	if (IS_CLIENT(game))
	{
		if( level->m_pLevelDefinition->fAutomapWidth == 0 )
		{
			DRLGCreateAutoMap(game, room);

			// apply saved automap info
			c_RoomApplyAutomapData(game, level, room);
		}
		

		c_sRoomPopulateCritters(game, room);
	}

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL RoomAllConnectionsReady( ROOM * pRoom )
{
	// need to replace this with a check that all the connections this room has are loaded and filled in
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
int CurrentLevelGetFirstRoom()
{
	LEVEL* pLevel = LevelGetByID( AppGetCltGame(), AppGetCurrentLevel() );
	ROOM* room = LevelGetFirstRoom( pLevel );
	if ( c_GetToolMode() )
	{
		// don't assert -- in Hammer, this can be valid
		if ( !room )
			return INVALID_ID;
	}
	else
	{
		ASSERT_RETINVALID( room );
	}
	return room->idRoom;
}
#endif //!SERVER_VERSION

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int CurrentLevelGetNextRoom( int nCurRoomID )
{
	ROOM * curRoom = RoomGetByID( AppGetCltGame(), nCurRoomID );
	ASSERT_RETINVALID( curRoom );
	ROOM * room = LevelGetNextRoom( curRoom );
	if ( !room )
		return INVALID_ID;
	return room->idRoom;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int RoomGetControlUnitRoom()
{
	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return INVALID_ID;
	UNIT* pUnit = GameGetControlUnit( pGame );
	int nControlRoomID = INVALID_ID;
	if (pUnit && pUnit->pRoom)
		nControlRoomID = pUnit->pRoom->idRoom;
	return nControlRoomID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL RoomIsOutdoorRoom( 
	GAME * pGame,
	ROOM * pRoom )
{
	if ( ! pRoom || ! pRoom->pDefinition )
		return FALSE;
	const ROOM_INDEX * pRoomIndex = RoomGetRoomIndex( pGame, pRoom );
	if ( ! pRoomIndex )
		return FALSE;
	return pRoomIndex->bOutdoor;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL RoomIsOutdoorRoom( int nRoomID )
{
	GAME *pGame = AppGetCltGame();
	ROOM * pRoom = RoomGetByID( pGame, nRoomID );
	return RoomIsOutdoorRoom( pGame, pRoom );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL RoomUsesOutdoorVisibility( 
	int nRoomID)
{
	GAME *pGame = AppGetCltGame();
	ROOM * pRoom = RoomGetByID( pGame, nRoomID );
	if ( ! pRoom || ! pRoom->pDefinition )
	{
		return FALSE;
	}
	const ROOM_INDEX * pRoomIndex = RoomGetRoomIndex( pGame, pRoom );
	if ( ! pRoomIndex )
	{
		return FALSE;
	}
	return pRoomIndex->bOutdoorVisibility;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL RoomFileOccludesVisibility( 
	const char * pszFilePath )
{
	if ( ! pszFilePath )
		return FALSE;

	const ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex( pszFilePath );
	if ( ! pRoomIndex )
		return FALSE;
	return ! pRoomIndex->bDontOccludeVisibility;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL RoomFileWantsObscurance( 
	const char * pszFilePath )
{
	if ( ! pszFilePath )
		return FALSE;

	const ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex( pszFilePath );
	if ( ! pRoomIndex )
		return FALSE;
	return pRoomIndex->bComputeObscurance;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL RoomFileInExcel( 
	const char * pszFilePath )
{
	if ( ! pszFilePath )
		return FALSE;

	const ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex( pszFilePath );
	return NULL != pRoomIndex;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sRoomFileGetRoomIndexFilePath( char * pszIndex, int nBufLen, const char * pszFilePath )
{
	char szFilePath[ MAX_XML_STRING_LENGTH ];
	// this checks to see if a room is in either the level_rooms or props spreadsheets, using a non-"low" path
	V_DO_FAILED( e_ModelDefinitionStripLODFileName( szFilePath, MAX_XML_STRING_LENGTH, pszFilePath ) )
	{
		return FALSE;
	}

	PStrRemovePath(pszIndex, MAX_XML_STRING_LENGTH, FILE_PATH_BACKGROUND, szFilePath);
	PStrRemoveExtension(pszIndex, MAX_XML_STRING_LENGTH, pszIndex);

	return TRUE;
}

//-------------------------------------------------------------------------------------------------

int RoomFileGetRoomIndexLine( const char * pszFilePath )
{
	if ( ! pszFilePath )
		return INVALID_ID;

	char pszIndex[MAX_XML_STRING_LENGTH];
	if ( ! sRoomFileGetRoomIndexFilePath( pszIndex, MAX_XML_STRING_LENGTH, pszFilePath ) )
		return INVALID_ID;

	// Is it a room?
	int nRoomIndexLine = ExcelGetLineByStringIndex(NULL, DATATABLE_ROOM_INDEX, pszIndex);
	if ( nRoomIndexLine != INVALID_ID)
		return nRoomIndexLine;

	// Is it a prop?
	nRoomIndexLine = ExcelGetLineByStringIndex(NULL, DATATABLE_PROPS, pszIndex);
	return nRoomIndexLine;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

ROOM_INDEX * RoomFileGetRoomIndex( const char * pszFilePath )
{
	if ( ! pszFilePath )
		return NULL;

	char pszIndex[MAX_XML_STRING_LENGTH];
	if ( ! sRoomFileGetRoomIndexFilePath( pszIndex, MAX_XML_STRING_LENGTH, pszFilePath ) )
		return NULL;

	// Is it a room?
	ROOM_INDEX * pRoomIndex = (ROOM_INDEX *)ExcelGetDataByStringIndex(NULL, DATATABLE_ROOM_INDEX, pszIndex);
	if ( pRoomIndex )
		return pRoomIndex;

	// Is it a prop?
	pRoomIndex = (ROOM_INDEX *)ExcelGetDataByStringIndex(NULL, DATATABLE_PROPS, pszIndex);
	return pRoomIndex;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL RoomAllowsMonsterSpawn( 
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	
	// check for dynamic room flag
	if (RoomTestFlag( pRoom, ROOM_NO_SPAWN_BIT ))
	{
		return FALSE;
	}	

	// check excel data	
	GAME *pGame = RoomGetGame( pRoom );
	const ROOM_INDEX * pRoomIndex = RoomGetRoomIndex( pGame, pRoom );
	if (pRoomIndex == NULL)
	{
		return FALSE;
	}
	if (pRoomIndex->bNoMonsterSpawn == TRUE)
	{
		return FALSE;
	}
	
	return TRUE;	
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL RoomAllowsAdventures( 
	GAME *pGame,
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	const ROOM_INDEX *pRoomIndex = RoomGetRoomIndex( pGame, pRoom );
	ASSERTX_RETFALSE( pRoomIndex, "Expected room index" );

	// check explicitly forbidden adventures	
	if (pRoomIndex->bNoAdventures == TRUE)
	{
		return FALSE;
	}

	// test dynamic no adventure room flag
	if (RoomTestFlag( pRoom, ROOM_NO_ADVENTURES_BIT ))
	{
		return FALSE;
	}
	
	// lets not put these in rooms that don't allow monsters either
	if (RoomAllowsMonsterSpawn( pRoom ) == FALSE)
	{
		return FALSE;
	}

	// don't put in rooms that don't allow sublevel entrances either, just for fun
	if (pRoomIndex->bNoSubLevelEntrance == TRUE)
	{
		return FALSE;
	}

	// only rooms in the default sublevel can have adventures
	if (RoomIsInDefaultSubLevel( pRoom ) == FALSE)
	{
		return FALSE;
	}
		
	return TRUE; // ok to gooooo
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_RoomSystemInit()
{
	ArrayInit(gtVisibleRoomCommands, NULL, 150);
	ArrayInit(gVisibleRoomList, NULL, 5);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_RoomSystemDestroy()
{
	gtVisibleRoomCommands.Destroy();
	gVisibleRoomList.Destroy();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
unsigned sRoomGetBackgroundModelDefinitionNewFlags(void)
{
#if !ISVERSION(SERVER_VERSION)
#if ISVERSION(DEVELOPMENT)
	if (e_IsNoRender())	// fillpaking
	{
		return MDNFF_DEFAULT;
	}
#endif
	switch (e_OptionState_GetActive()->get_nBackgroundLoadForceLOD())
	{
		case LOD_LOW:
			return MDNFF_ALLOW_LOW_LOD | MDNFF_IGNORE_HIGH_LOD;
		case LOD_HIGH:
			return MDNFF_IGNORE_LOW_LOD | MDNFF_ALLOW_HIGH_LOD;
		default:
			break;
	}
#endif
	return MDNFF_DEFAULT;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
unsigned sRoomGetPropModelDefinitionNewFlags(void)
{
	return MDNFF_DEFAULT;
}

//----------------------------------------------------------------------------
// load a room definition, convert from raw to cooked if needed
//----------------------------------------------------------------------------
static ROOM_DEFINITION * sRoomDefinitionGet(
	GAME* game,
	const char* pszFileName,
	BOOL bLoadModel,
	BOOL bIsProp,
	float fDistanceToCamera)
{
	char fullfilename[MAX_XML_STRING_LENGTH];
	PStrPrintf(fullfilename, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_BACKGROUND, pszFileName);
	PStrReplaceExtension(fullfilename, MAX_XML_STRING_LENGTH, fullfilename, ROOM_FILE_EXTENSION);
	ConsoleDebugString(OP_LOW, LOADING_STRING(room definition), fullfilename);

	BOOL bRecursed = FALSE;

	char pszFileNameNoExtension[MAX_XML_STRING_LENGTH];
	PStrRemoveExtension(pszFileNameNoExtension, MAX_XML_STRING_LENGTH, pszFileName);

	void * buffer;
	unsigned int buflen;
	UINT64 gentime;

_recurse:
	buffer = NULL;
	buflen = 0;
	gentime = 0;
	if (!UpdateRoomFile(fullfilename, bRecursed, gentime, buffer, buflen))
	{
		return NULL;	// failed to update a file that needed updating
	}

	if (buffer && buflen > 0)	// updated it, so add it to the pakfile
	{
		ASSERT(gentime != 0);

		FileDebugCreateAndCompareHistory(fullfilename);	// history debugging
		
		DECLARE_LOAD_SPEC(specCooked, fullfilename);
		specCooked.buffer = buffer;
		specCooked.size = buflen;
		specCooked.gentime = gentime;
		PakFileAddFile(specCooked);
	}
	else
	{		
		DECLARE_LOAD_SPEC(spec, fullfilename);
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
		buffer = (BYTE *)PakFileLoadNow(spec);
		ASSERTX_RETNULL(buffer, fullfilename); 
		buflen = spec.bytesread;
	}
	
	ROOM_DEFINITION * roomDefinition = NULL;

	ONCE
	{
		ASSERT_BREAK(buflen >= sizeof(FILE_HEADER) + sizeof(ROOM_DEFINITION));
		FILE_HEADER * file_header = (FILE_HEADER *)buffer;
		ASSERT_BREAK(file_header->dwMagicNumber == ROOM_FILE_MAGIC_NUMBER);
		ASSERT_BREAK(file_header->nVersion == ROOM_FILE_VERSION);

		ROOM_DEFINITION * room_definition = (ROOM_DEFINITION *)((BYTE *)buffer + sizeof(FILE_HEADER));
		room_definition->nFileSize = buflen;

		{	//New code to update room files based upon nExcelVersion.  First we need to get the ROOM_INDEX
			//ROOM_INDEX* pRoomIndex = 
			//	(ROOM_INDEX *)ExcelGetData( game, DATATABLE_ROOM_INDEX, room_definition->nRoomExcelIndex );
			char szFileWithExt[MAX_XML_STRING_LENGTH];
			PStrRemovePath(szFileWithExt, MAX_XML_STRING_LENGTH, FILE_PATH_BACKGROUND, pszFileName);
			char szExcelIndex[MAX_XML_STRING_LENGTH];
			PStrRemoveExtension(szExcelIndex, MAX_XML_STRING_LENGTH, szFileWithExt);
			//int nExcelRoomIndexLine = ExcelGetLineByStringIndex(NULL, DATATABLE_ROOM_INDEX, szExcelIndex);
			//ROOM_INDEX * pRoomIndex = (ROOM_INDEX *)ExcelGetDataByStringIndex(NULL, DATATABLE_ROOM_INDEX, szExcelIndex);
			ROOM_INDEX * pRoomIndex = RoomFileGetRoomIndex(szExcelIndex);

			if (pRoomIndex && pRoomIndex->nExcelVersion != room_definition->nExcelVersion)
			{
				FREE(NULL, buffer);
				ASSERT_GOTO(!bRecursed, _error); // UpdateRoomFile didn't properly update nExcelVersion
				bRecursed = TRUE;
				Sleep(250);		// not sure what to do, wait a bit to hopefully finish writing out the file we've been updating
				goto _recurse;
			}
		}

		room_definition->tHeader.nId = INVALID_ID;
		PStrCopy(room_definition->tHeader.pszName, pszFileName, MAX_XML_STRING_LENGTH);
		room_definition->pbFileStart = (BYTE *)buffer;
		if(!bIsProp)
		{
			//room_definition->nRoomExcelIndex = ExcelGetLineByStringIndex(NULL, DATATABLE_ROOM_INDEX, pszFileNameNoExtension);
			room_definition->nRoomExcelIndex = RoomFileGetRoomIndexLine(pszFileNameNoExtension);
			ASSERTX(room_definition->nRoomExcelIndex != INVALID_LINK, pszFileName);
		}
		else
		{
			//room_definition->nRoomExcelIndex = ExcelGetLineByStringIndex(NULL, DATATABLE_PROPS, pszFileNameNoExtension);
			room_definition->nRoomExcelIndex = RoomFileGetRoomIndexLine(pszFileNameNoExtension);
			ASSERTX(room_definition->nRoomExcelIndex != INVALID_LINK, pszFileName);
		}
		room_definition->nModelDefinitionId = INVALID_ID;

		// convert offsets to pointers
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pRoomPoints) < room_definition->nFileSize);
		room_definition->pRoomPoints = (ROOM_POINT *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pRoomPoints));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pRoomCPolys) < room_definition->nFileSize);
		room_definition->pRoomCPolys = (ROOM_CPOLY *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pRoomCPolys));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pHullVertices) < room_definition->nFileSize);
		room_definition->pHullVertices = (VECTOR *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pHullVertices));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pHullTriangles) < room_definition->nFileSize);
		room_definition->pHullTriangles = (int *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pHullTriangles));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pConnections) < room_definition->nFileSize);
		room_definition->pConnections= (ROOM_CONNECTION *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pConnections));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pCollisionMaterialIndexes) < room_definition->nFileSize);
		room_definition->pCollisionMaterialIndexes = (ROOM_COLLISION_MATERIAL_INDEX *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pCollisionMaterialIndexes));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pnCollisionGridIndexes) < room_definition->nFileSize);
		room_definition->pnCollisionGridIndexes = (unsigned int *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pnCollisionGridIndexes));
		ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pCollisionGrid) < room_definition->nFileSize);
		room_definition->pCollisionGrid = (COLLISION_GRID_NODE *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pCollisionGrid));

		for (int jj = 0; jj < room_definition->nCollisionGridHeight; ++jj)
		{
			for (int ii = 0; ii < room_definition->nCollisionGridWidth; ++ii)
			{
				COLLISION_GRID_NODE * node = room_definition->pCollisionGrid + jj * room_definition->nCollisionGridWidth + ii;
				for (unsigned int kk = 0; kk < NUM_COLLISION_CATEGORIES; kk++)
				{
#if !ISVERSION(OPTIMIZED_VERSION)
					ASSERT_GOTO(CAST_PTR64_TO_INT(node->pnPolyList[kk]) < room_definition->nFileSize, _error);
#endif
					node->pnPolyList[kk] = (unsigned int *)((BYTE *)buffer + CAST_PTR64_TO_INT(node->pnPolyList[kk]));
				}
			}
		}

		// convert indices to pointers
		for (int ii = 0; ii < room_definition->nRoomCPolyCount; ii++)
		{
#if !ISVERSION(OPTIMIZED_VERSION)
			ASSERT_GOTO(CAST_PTR_TO_INT(room_definition->pRoomCPolys[ii].pPt1) < room_definition->nFileSize, _error);
			ASSERT_GOTO(CAST_PTR_TO_INT(room_definition->pRoomCPolys[ii].pPt2) < room_definition->nFileSize, _error);
			ASSERT_GOTO(CAST_PTR_TO_INT(room_definition->pRoomCPolys[ii].pPt3) < room_definition->nFileSize, _error);
#endif
			room_definition->pRoomCPolys[ii].pPt1 = &room_definition->pRoomPoints[(int)CAST_PTR_TO_INT(room_definition->pRoomCPolys[ii].pPt1)];
			room_definition->pRoomCPolys[ii].pPt2 = &room_definition->pRoomPoints[(int)CAST_PTR_TO_INT(room_definition->pRoomCPolys[ii].pPt2)];
			room_definition->pRoomCPolys[ii].pPt3 = &room_definition->pRoomPoints[(int)CAST_PTR_TO_INT(room_definition->pRoomCPolys[ii].pPt3)];
		}

		if (room_definition->pVertices)
		{
			ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pVertices) < room_definition->nFileSize);
			room_definition->pVertices = (VECTOR *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pVertices));
		}

		if (room_definition->pwIndexBuffer)
		{
			ASSERT_BREAK(CAST_PTR_TO_INT(room_definition->pwIndexBuffer) < room_definition->nFileSize);
			room_definition->pwIndexBuffer = (WORD *)((BYTE *)buffer + (int)CAST_PTR_TO_INT(room_definition->pwIndexBuffer));
		}

#ifdef HAVOK_ENABLED
		if ( AppIsHellgate() )
		{
			if (game && IS_SERVER(game))
			{
				MemorySetThreadPool(NULL);	// definitions should get created in NULL pool, not the game's pool
			}
			if (room_definition->pVertices && room_definition->pwIndexBuffer)
			{
				//if (USE_MULTI_MOPP) 
				//{
				//	room_definition->ppHavokMultiShape = HavokMultiCreateShapeFromMesh(fullfilename,
				//		sizeof(VECTOR), room_definition->nVertexBufferSize, room_definition->nVertexCount, room_definition->pVertices, 
				//		room_definition->nIndexBufferSize, room_definition->nTriangleCount, room_definition->pwIndexBuffer, 
				//		room_definition->nHavokShapeCount);
				//}
				//room_definition->nHavokShapeCount = 6; //default value.  May have to special-case.
				//fixed to be set in c_granny_rigid.
				HavokCreateShapeFromMesh( room_definition->tHavokShapeHolder, fullfilename,
					sizeof(VECTOR), room_definition->nVertexBufferSize, room_definition->nVertexCount, room_definition->pVertices, 
					room_definition->nIndexBufferSize, room_definition->nTriangleCount, room_definition->pwIndexBuffer);
			}
			if (game && IS_SERVER(game))
			{
				MemorySetThreadPool(GameGetMemPool(game));
			}
			if (room_definition->tHavokShapeHolder.pHavokShape == NULL)
			{
				ASSERTV(0, "Room %s is missing a collision mesh", room_definition->tHeader.pszName);
			}
		}
#endif

		if (bLoadModel)
		{
			room_definition->nModelDefinitionId = e_ModelDefinitionNewFromFile( fullfilename, NULL, sRoomGetBackgroundModelDefinitionNewFlags(), fDistanceToCamera );
		}

		roomDefinition = room_definition;
	}
_error:
	if (!roomDefinition)
	{
		FREE(NULL, buffer);
	}

	return roomDefinition;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_DEFINITION* RoomDefinitionGet(
	GAME* game,
	int nRoomIndex,
	BOOL bLoadModel,
	float fDistanceToCamera )
{
	if ( nRoomIndex == INVALID_ID )
		return NULL;

	ROOM_INDEX* room_index = (ROOM_INDEX*)ExcelGetData( game, DATATABLE_ROOM_INDEX, nRoomIndex );
	if ( ! room_index )
		return NULL;

	if ( ! FillPakShouldLoad( room_index->pszFile ) )
		return NULL;

	if (!room_index->pRoomDefinition)
	{
		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
		}

		room_index->pRoomDefinition = sRoomDefinitionGet(game, room_index->pszFile, bLoadModel, FALSE, fDistanceToCamera );

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
		}

		return room_index->pRoomDefinition;
	}

	ROOM_DEFINITION* room_definition = room_index->pRoomDefinition;
	ASSERT_RETNULL(room_definition);

	if (bLoadModel && ( room_definition->nModelDefinitionId == INVALID_ID || ! e_ModelDefinitionExists( room_definition->nModelDefinitionId ) ) )
	{
		char pszFileNameWithPath[MAX_XML_STRING_LENGTH];
		PStrPrintf(pszFileNameWithPath, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_BACKGROUND, room_index->pszFile);
		ConsoleDebugString( OP_LOW, LOADING_STRING(room definition), pszFileNameWithPath );

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
		}

		e_GrannyUpdateModelFile(pszFileNameWithPath, TRUE, LOD_HIGH);

		if ( AppCommonIsFillpakInConvertMode() ) 
		{
			AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
			AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
		}

		room_definition->nModelDefinitionId = e_ModelDefinitionNewFromFile( pszFileNameWithPath, NULL, sRoomGetBackgroundModelDefinitionNewFlags(), fDistanceToCamera );
		ASSERT(room_definition->nModelDefinitionId != INVALID_ID);
	}
	return room_definition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_INDEX * sFindProp(GAME * pGame, const char * pszPropFile)
{
	//return (ROOM_INDEX*)ExcelGetDataByStringIndex(pGame, DATATABLE_PROPS, pszPropFile);
	return RoomFileGetRoomIndex(pszPropFile);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM_DEFINITION* PropDefinitionGet(
	GAME* game,
	const char * pszPropFile,
	BOOL bLoadModel,
	float fDistanceToCamera )
{
	char pszPropIndex[ROOM_INDEX_SIZE];
	char pszPropFileToUse[ROOM_INDEX_SIZE];
	PStrRemoveExtension(pszPropIndex, ROOM_INDEX_SIZE, pszPropFile);
	PStrCopy(pszPropFileToUse, ROOM_INDEX_SIZE, pszPropIndex, ROOM_INDEX_SIZE);
	ROOM_INDEX* room_index = sFindProp(game, pszPropIndex);
	ASSERTV_RETNULL(room_index, "Prop file '%s' is not in the props table.  Be sure to add it to the table before using it as a prop.", pszPropFile);
#if !ISVERSION(SERVER_VERSION)
	if(IS_CLIENT(game))
	{
		ROOM_INDEX* room_index_original = room_index;
		if(AppTestFlag(AF_CENSOR_NO_GORE) && room_index_original->nNoGoreProp >= 0)
		{
			room_index = (ROOM_INDEX*)ExcelGetData(game, DATATABLE_PROPS, room_index_original->nNoGoreProp);
			ASSERTV_RETNULL(room_index, "No gore prop (%d) for file '%s' is not in the props table.", room_index_original->nNoGoreProp, pszPropFile);
			PStrCopy(pszPropFileToUse, ROOM_INDEX_SIZE, room_index->pszFile, ROOM_INDEX_SIZE);
		}
		if(AppTestFlag(AF_CENSOR_NO_HUMANS) && room_index_original->nNoHumansProp >= 0)
		{
			room_index = (ROOM_INDEX*)ExcelGetData(game, DATATABLE_PROPS, room_index_original->nNoHumansProp);
			ASSERTV_RETNULL(room_index, "No humans prop (%d) for file '%s' is not in the props table.", room_index_original->nNoHumansProp, pszPropFile);
			PStrCopy(pszPropFileToUse, ROOM_INDEX_SIZE, room_index->pszFile, ROOM_INDEX_SIZE);
		}
	}
#endif
	PStrCat(pszPropFileToUse, ".GR2", ROOM_INDEX_SIZE);
	if ( ! room_index->pRoomDefinition )
	{
		// Create it
		room_index->pRoomDefinition = sRoomDefinitionGet(game, pszPropFileToUse, bLoadModel, TRUE, fDistanceToCamera );
		if (!room_index->pRoomDefinition)
		{
			return NULL;
		}
	}

	ROOM_DEFINITION* room_definition = room_index->pRoomDefinition;
	if (!room_definition)
	{
		return NULL;
	}

	if (bLoadModel && ( room_definition->nModelDefinitionId == INVALID_ID || ! e_ModelDefinitionExists( room_definition->nModelDefinitionId ) ) )
	{
		char pszFileNameWithPath[MAX_XML_STRING_LENGTH];
		PStrPrintf(pszFileNameWithPath, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_BACKGROUND, room_index->pszFile);
		ConsoleDebugString( OP_LOW, LOADING_STRING(room definition), pszFileNameWithPath );
		e_GrannyUpdateModelFile(pszFileNameWithPath, TRUE, LOD_HIGH);

		room_definition->nModelDefinitionId = e_ModelDefinitionNewFromFile( pszFileNameWithPath, NULL, sRoomGetPropModelDefinitionNewFlags() );
		ASSERT(room_definition->nModelDefinitionId != INVALID_ID);
		e_ModelDefinitionSetHashFlags( room_definition->nModelDefinitionId, MODELDEF_HASH_FLAG_IS_PROP, TRUE );
	}
	return room_definition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SetTraceRoom( int nRoomID )
{
	gnTraceRoom = nRoomID;
}

//----------------------------------------------------------------------------
struct ROOM_SUBLEVEL_POPULATE_POST_PROCESS_DATA
{
	LEVEL *pLevel;
	UNITID idActivator;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS s_sRoomSubLevelPopulatePostProcess(
	UNIT *pUnit,
	void *pCallbackData)
{
	ROOM_SUBLEVEL_POPULATE_POST_PROCESS_DATA *pData = (ROOM_SUBLEVEL_POPULATE_POST_PROCESS_DATA *)pCallbackData;
	UNITID idActivator = pData->idActivator;

	// add warps to level ... warps are always created during the sublevel populate pass
	LevelAddWarp( pUnit, idActivator, WRP_SUBLEVEL_POPULATE );

	// continue, even if there were errors so we an try to move forward		
	return RIU_CONTINUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_RoomSubLevelPopulated(
	ROOM *pRoom,
	UNIT *pActivator)
{
	ASSERTX_RETURN( pRoom, "Expected room" );

	// create any units that should be created during the sublevel population pass
	s_sRoomPopulateSpawnPointList( pRoom, RPP_SUBLEVEL_POPULATE );

	// post process units created during the sublevel populate pass
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ROOM_SUBLEVEL_POPULATE_POST_PROCESS_DATA tData;
	tData.pLevel = pLevel;
	tData.idActivator = pActivator ? UnitGetId( pActivator ) : INVALID_ID;
	
	// iterate units
	RoomIterateUnits( pRoom, NULL, s_sRoomSubLevelPopulatePostProcess, (void*)&tData );
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sRoomSendUnitsToClientsWithRoom(
	ROOM *pRoom)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	
	// go through clients
	for (GAMECLIENT *pClient = ClientGetFirstInLevel( pLevel );
		 pClient;
		 pClient = ClientGetNextInLevel( pClient, pLevel ))
	{
		if (ClientIsRoomKnown( pClient, pRoom ))
		{
			s_sRoomSendUnits( pRoom, pClient );
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_RoomActivate(
	ROOM * room,
	UNIT * activator)
{
	ASSERT_RETURN(room);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);
	LEVEL * level = RoomGetLevel(room);
	ASSERT_RETURN(level);
	
	ASSERTX_RETURN(LevelTestFlag(RoomGetLevel(room), LEVEL_ACTIVE_BIT), "Attempting to activate a room for a level that is not active");

	ASSERT_RETURN(LevelIsActive(level));
	// in shared town games, nobody should be activating rooms in action levels
	if (GameAllowsActionLevels(game) == FALSE)
	{
		if (LevelIsActionLevel(level) == TRUE)
		{
			ASSERTV_RETURN(0, "Game type not allowed to activate room '%s' in level '%s'", RoomGetDevName(room), LevelGetDevName(level));
		}
	}
	
	if (AppIsHellgate())
	{
		// flag the room as activated by this player (even if another player did it previously)
		// this does not iterate the quests or send update message until the next quest update
		s_QuestEventRoomActivated(room, activator);
	}

	// do nothing if room is already active
	if (RoomIsActive(room) == TRUE)
	{
		return;
	}

	TRACE_ACTIVE("ROOM [%d: %s] Activate\n", RoomGetId(room), RoomGetDevName(room));

	// respawn the room if need be
	s_sRoomCheckReset(room);

	// set flag
	RoomSetFlag(room, ROOM_ACTIVE_BIT, TRUE);

	// populate room if not populated
	s_sRoomPopulate(room, activator);

	// make all units in this room start doing something
	s_sRoomActivateUnits(room);

	//this will create any special quest objects or units that might of been wiped out...
	if( AppIsTugboat() )
	{
		//s_QuestRoomAsBeenActivated( room );
		//MYTHOS_LEVELAREAS::s_LevelAreaRoomHasBeenReactivated( room );
	}

	
	// notify task system of activated room
	s_TaskEventRoomActivated(room);
		
	// any clients that know about this room should receive its units now
	s_sRoomSendUnitsToClientsWithRoom(room);
}	
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_RoomDeactivate(
	ROOM * room,
	BOOL bDeactivateLevel)
{
	if (RoomIsActive(room) == FALSE)
	{
		return;
	}

	// do not allow rooms to be deactivated if clients know about them!
#if ISVERSION(DEVELOPMENT)
	BOOL bKnownCheck = FALSE;
	LEVEL * level = RoomGetLevel(room);		
	for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
	{
		if (ClientIsRoomKnown(client, room))
		{
			bKnownCheck = TRUE;
			break;
		}
	}
	ASSERT((bKnownCheck == FALSE && room->nKnownByClientRefCount == 0) || (bKnownCheck == TRUE && room->nKnownByClientRefCount > 0));
#endif

	TRACE_ACTIVE("ROOM [%d: %s] Deactivate\n", RoomGetId(room), RoomGetDevName(room));

	// deactivate all units in this room
	s_sRoomDeactivateUnits(room, bDeactivateLevel);
	
	// clear active bit
	RoomSetFlag(room, ROOM_ACTIVE_BIT, FALSE);

	// set an event to repopulate this room after a little while
	if (sgeStartRoomResetEvent == SRRE_ON_ROOM_DEACTIVATE)
	{
		s_RoomStartResetTimer(room, 1.0f);			
	}	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ROOM_COLLISION_MATERIAL_INDEX * GetMaterialFromIndex(
	ROOM_DEFINITION * pRoomDef,
	int nIndex)
{
	int nMaterialIndex = 0;
	const int nIndexCount = pRoomDef->nCollisionMaterialIndexCount;
	const ROOM_COLLISION_MATERIAL_INDEX * pIndexes = pRoomDef->pCollisionMaterialIndexes;
	while(nMaterialIndex < nIndexCount && nIndex > (pIndexes[nMaterialIndex].nFirstTriangle + pIndexes[nMaterialIndex].nTriangleCount - 1))
	{
		nMaterialIndex++;
	}
	if (nMaterialIndex >= nIndexCount)
	{
		return NULL;
	}
	return &pIndexes[nMaterialIndex];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ROOM_COLLISION_MATERIAL_DATA * FindMaterialWithIndex(
	int nMaterialIndex)
{
	int nCount = ExcelGetNumRows(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION);
	for(int i=0; i<nCount; i++)
	{
		const ROOM_COLLISION_MATERIAL_DATA * pMaterialData = (const ROOM_COLLISION_MATERIAL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION, i);
		if (pMaterialData->nMaterialNumber == nMaterialIndex)
		{
			return pMaterialData;
		}
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int FindMaterialIndexWithMax9Name(
	const char * pszMax9Name)
{
	int nCount = ExcelGetNumRows(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION);
	for(int i=0; i<nCount; i++)
	{
		const ROOM_COLLISION_MATERIAL_DATA * pMaterialData = (const ROOM_COLLISION_MATERIAL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION, i);
		if (pMaterialData && PStrICmp(pMaterialData->pszMax9Name, pszMax9Name) == 0)
		{
			return pMaterialData->nMaterialNumber;
		}
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
 int GetIndexOfMaterial(
	const ROOM_COLLISION_MATERIAL_DATA * pCollisionData )
{
	int nCount = ExcelGetNumRows(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION);
	for(int i=0; i<nCount; i++)
	{

		const ROOM_COLLISION_MATERIAL_DATA * pMaterialData = (const ROOM_COLLISION_MATERIAL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION, i);
		if( pMaterialData == pCollisionData )
		{
			return i;
		}
	}
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ROOM_COLLISION_MATERIAL_DATA * GetMaterialByIndex(
	int nIndex)
{
	int nCount = ExcelGetNumRows(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION);
	if( nIndex < nCount )
	{
		return  (const ROOM_COLLISION_MATERIAL_DATA *)ExcelGetData(AppGetCltGame(), DATATABLE_MATERIALS_COLLISION, nIndex);
	}
	return NULL;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM *RoomGetByID(
	GAME *pGame,
	ROOMID idRoom)
{
	ASSERT_RETNULL(pGame && pGame->m_Dungeon);
	DUNGEON* dungeon = pGame->m_Dungeon;
	return DungeonGetRoomByID( dungeon, idRoom );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int RoomGetMonsterExperienceLevel(
	ROOM* pRoom)
{
	ASSERT_RETVAL( pRoom, 1 );
	LEVEL* pLevel = RoomGetLevel( pRoom );

	if( AppIsTugboat() )
	{
		int retval = ExcelEvalScript(
			RoomGetGame( pRoom ), 
			NULL, 
			NULL, 
			NULL, 
			DATATABLE_ROOM_INDEX, 
			OFFSET(ROOM_INDEX, codeMonsterLevel[DIFFICULTY_NORMAL]), 
			pRoom->pDefinition->nRoomExcelIndex );
		if( retval > 0 )
		{
			return retval;
		}

	}
	return LevelGetMonsterExperienceLevel( pLevel );		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *RoomGetDevName(
	ROOM *pRoom)
{
	ASSERTX_RETNULL( pRoom, "Expected room" );
	const ROOM_DEFINITION *pRoomDefinition = pRoom->pDefinition;
	return pRoomDefinition->tHeader.pszName;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHasAlreadyProcessedRoom(
	ROOM * room,
	DWORD dwRoomListTimestamp)
{
	return room->dwRoomListTimestamp == dwRoomListTimestamp;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomListAddRoom(
	ROOM_LIST * roomlist,
	ROOM * room,
	DWORD dwRoomListTimestamp)
{
	ASSERTX_RETURN(roomlist->nNumRooms < MAX_ROOMS_PER_LEVEL, "Too many rooms for room list");
	
	// add to room list	
	roomlist->pRooms[roomlist->nNumRooms++] = room;
	
	// set timestamp this room was added to list
	if (dwRoomListTimestamp != 0)
	{
		room->dwRoomListTimestamp = dwRoomListTimestamp;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRoomInRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist)
{
	// simplistic ... we don't need speed for this ... yet -Colin
	for (int i = 0; i < roomlist->nNumRooms; ++i)
	{
		ASSERT_BREAK(i < MAX_ROOMS_PER_LEVEL);
		
		// check room ids (not pointers so we can do debug crossover for client/server)
		if (RoomGetId(roomlist->pRooms[i]) == RoomGetId(room))
		{
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddOppositeVisualPortalRooms( 
	GAME * game,
	ROOM_LIST * roomlist,
	ROOM * room,
	DWORD dwRoomListTimestamp)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(room);
	ASSERT_RETURN(roomlist);

	if (room->nNumVisualPortals <= 0)
	{
		return;
	}

	// go through each portal
	for (int i = 0; i < room->nNumVisualPortals; ++i)
	{
		UNITID idPortal = room->pVisualPortals[i].idVisualPortal;
		UNIT * portal = UnitGetById(game, idPortal);
		UNIT * oppositePortal = s_ObjectGetOppositeSubLevelPortal(portal);
		if (oppositePortal == NULL)
		{
			continue;
		}			
		
		// get the room the portal is in
		ROOM * oppositeRoom = UnitGetRoom(oppositePortal);
		ASSERT_CONTINUE(oppositeRoom);
		
		// add room to array if it's not already there
		if (sHasAlreadyProcessedRoom(oppositeRoom, dwRoomListTimestamp) == FALSE)
		{
			// add room to list
			sRoomListAddRoom(roomlist, oppositeRoom, dwRoomListTimestamp);
			
			// keep the chain going through this room too
			sAddOppositeVisualPortalRooms(game, roomlist, oppositeRoom, dwRoomListTimestamp);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddRoomToRoomList(
	GAME * game,
	ROOM * room,
	ROOM_LIST * roomlist,
	DWORD dwRoomListTimestamp)
{
	// add room to array if not already in it
	if (sHasAlreadyProcessedRoom(room, dwRoomListTimestamp) == FALSE)
	{
		sRoomListAddRoom(roomlist, room, dwRoomListTimestamp);
	}

	// if this room has visual portals, add their connecting rooms so we can
	// draw "through" the portal on the client
	sAddOppositeVisualPortalRooms(game, roomlist, room, dwRoomListTimestamp);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetConnectedRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist)
{
	ASSERT_RETURN(room);
	ASSERT_RETURN(roomlist);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);

	// get unique reference for this neighboring room operation
	DWORD dwRoomListTimestamp = ++(game->dwRoomListTimestamp);
	ASSERT(dwRoomListTimestamp != 0);

	if (INCLUDE_CENTER)
	{
		sAddRoomToRoomList(game, room, roomlist, dwRoomListTimestamp);
	}

	int connectedRoomCount = RoomGetConnectedRoomCount(room);
	for (int i = 0; i < connectedRoomCount; ++i)
	{
		ROOM * connectedRoom = RoomGetConnectedRoom(room, i);
		ASSERT_CONTINUE(connectedRoom);
		sAddRoomToRoomList(game, connectedRoom, roomlist, dwRoomListTimestamp);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetAdjacentRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist)
{
	ASSERT_RETURN(room);
	ASSERT_RETURN(roomlist);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);

	// get unique reference for this neighboring room operation
	DWORD dwRoomListTimestamp = ++(game->dwRoomListTimestamp);
	ASSERT(dwRoomListTimestamp != 0);

	if (INCLUDE_CENTER)
	{
		sAddRoomToRoomList(game, room, roomlist, dwRoomListTimestamp);
	}

	int adjacentRoomCount = RoomGetAdjacentRoomCount(room);
	for (int i = 0; i < adjacentRoomCount; ++i)
	{
		ROOM * adjacentRoom = RoomGetAdjacentRoom(room, i);
		ASSERT_CONTINUE(adjacentRoom);
		sAddRoomToRoomList(game, adjacentRoom, roomlist, dwRoomListTimestamp);
	}
}

template
void TemplatedRoomGetAdjacentRoomList<TRUE>( 
	ROOM * room, 
	ROOM_LIST * roomlist);

template 
void TemplatedRoomGetAdjacentRoomList<FALSE>( 
	ROOM * room, 
	ROOM_LIST * roomlist);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetNearbyRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist)
{
	ASSERT_RETURN(room);
	ASSERT_RETURN(roomlist);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);

	// get unique reference for this neighboring room operation
	DWORD dwRoomListTimestamp = ++(game->dwRoomListTimestamp);
	ASSERT(dwRoomListTimestamp != 0);

	if (INCLUDE_CENTER)
	{
		sAddRoomToRoomList(game, room, roomlist, dwRoomListTimestamp);
	}

	int nearbyRoomCount = RoomGetNearbyRoomCount(room);
	for (int i = 0; i < nearbyRoomCount; ++i)
	{
		ROOM * nearbyRoom = RoomGetNearbyRoom(room, i);
		ASSERT_CONTINUE(nearbyRoom);
		sAddRoomToRoomList(game, nearbyRoom, roomlist, dwRoomListTimestamp);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetVisibleRoomList( 
	ROOM * room, 
	ROOM_LIST * roomlist)
{
	ASSERT_RETURN(room);
	ASSERT_RETURN(roomlist);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);

	// get unique reference for this neighboring room operation
	DWORD dwRoomListTimestamp = ++(game->dwRoomListTimestamp);
	ASSERT(dwRoomListTimestamp != 0);

	if (INCLUDE_CENTER)
	{
		sAddRoomToRoomList(game, room, roomlist, dwRoomListTimestamp);
	}

	int visibleRoomCount = RoomGetVisibleRoomCount(room);
	for (int i = 0; i < visibleRoomCount; ++i)
	{
		ROOM * visibleRoom = RoomGetVisibleRoom(room, i);
		ASSERT_CONTINUE(visibleRoom);
		sAddRoomToRoomList(game, visibleRoom, roomlist, dwRoomListTimestamp);
	}
}

//----------------------------------------------------------------------------
struct PARTY_ROOM_DATA
{
	UNIT *pUnitUpdatingKnownRooms;
	ROOM_LIST *pRoomList;
	DWORD dwRoomListTimestamp;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddOpponentRoomToKnownList(
	UNIT *pPlayer,
	ROOM_LIST * roomlist,
	DWORD dwRoomListTimestamp)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player unit" );
	GAME *pGame = UnitGetGame( pPlayer );

	UNIT* pOpponent = UnitGetById( pGame, UnitGetStat(pPlayer, STATS_PVP_DUEL_OPPONENT_ID) );
	if( pOpponent )
	{
		ROOM *pRoom = UnitGetRoom( pOpponent );
		if (pRoom)
		{
			// add room to list
			if (UnitIsAlwaysKnownByPlayer( pPlayer, pOpponent ))
			{
				sAddRoomToRoomList( pGame, pRoom, roomlist, dwRoomListTimestamp );
			}

		}
	}

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddPartyRoomsToKnownList(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player unit" );
	GAME *pGame = UnitGetGame( pPlayer );
	
	PARTY_ROOM_DATA *pPartyRoomData = (PARTY_ROOM_DATA *)pCallbackData;
	ASSERTX_RETURN( pPartyRoomData, "Expected party room data" );
	ASSERTX_RETURN( pPartyRoomData->pRoomList, "Expected room list" );

	// if not the unit constructing the rooms in the list (we've already done that one)
	if (pPartyRoomData->pUnitUpdatingKnownRooms != pPlayer)
	{
		ROOM *pRoom = UnitGetRoom( pPlayer );
		if (pRoom)
		{
			// add room to list
			if (UnitIsAlwaysKnownByPlayer( pPlayer, pPartyRoomData->pUnitUpdatingKnownRooms ))
			{
				sAddRoomToRoomList( pGame, pRoom, pPartyRoomData->pRoomList, pPartyRoomData->dwRoomListTimestamp );
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template <BOOL INCLUDE_CENTER>
void TemplatedRoomGetKnownRoomList( 
	UNIT * player,							
	ROOM * room, 
	ROOM_LIST * roomlist)
{
	ASSERT_RETURN(player);
	ASSERT_RETURN(room);
	ASSERT_RETURN(roomlist);
	GAME * game = RoomGetGame(room);
	ASSERT_RETURN(game);

	// get unique reference for this neighboring room operation
	DWORD dwRoomListTimestamp = ++(game->dwRoomListTimestamp);
	ASSERT(dwRoomListTimestamp != 0);

	if (INCLUDE_CENTER)
	{
		sAddRoomToRoomList(game, room, roomlist, dwRoomListTimestamp);
	}

	int visibleRoomCount = RoomGetVisibleRoomCount(room);
	for (int i = 0; i < visibleRoomCount; ++i)
	{
		ROOM * visibleRoom = RoomGetVisibleRoom(room, i);
		ASSERT_CONTINUE(visibleRoom);
		if (s_sRoomPlayerIsInsideActivationRange(visibleRoom, player, FALSE, FALSE))
		{
			sAddRoomToRoomList(game, visibleRoom, roomlist, dwRoomListTimestamp);
		}
	}

	DWORD dwPetIterateFlags = 0;
	SETBIT(dwPetIterateFlags, IIF_PET_SLOTS_ONLY_BIT);
	UNIT * pet = UnitInventoryIterate(player, NULL, dwPetIterateFlags, FALSE);
	while (pet)
	{
		ROOM * petroom = UnitGetRoom(pet);
		if (petroom)
		{
			ASSERTX( UnitIsAlwaysKnownByPlayer( pet, player ), "Expected pet to always be known by player" );
			sAddRoomToRoomList(game, petroom, roomlist, dwRoomListTimestamp);
		}
		pet = UnitInventoryIterate(player, pet, dwPetIterateFlags, FALSE);
	}

	// TRAVIS: Let's add the rooms of partymembers too -
	// right now, only if they are in the same game/level as the client is. Colin
	// is going to do a trickle update later for partymembers in other levels/servers

#if !ISVERSION(CLIENT_ONLY_VERSION)
	PARTYID idParty = UnitGetPartyId( player );
	if (idParty != INVALID_ID)
	{

		// setup callback data	
		PARTY_ROOM_DATA tPartyRoomData;
		tPartyRoomData.pUnitUpdatingKnownRooms = player;
		tPartyRoomData.pRoomList = roomlist;
		tPartyRoomData.dwRoomListTimestamp = dwRoomListTimestamp;

		// iterate party membmers		
		s_PartyIterateCachedPartyMembersInGame( game, idParty, sAddPartyRoomsToKnownList, &tPartyRoomData );
		
	}	
	
	// we always know our duel opponents too
	if( UnitGetStat(player, STATS_PVP_DUEL_OPPONENT_ID) != INVALID_ID )
	{
		sAddOpponentRoomToKnownList( player, roomlist, dwRoomListTimestamp );
	}

#endif

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * RoomGetFirstConnectedRoomOrSelf(
	ROOM * room)
{
	int count = RoomGetConnectedRoomCount(room);
	if (count <= 0)
	{
		return room;
	}
	return RoomGetConnectedRoom(room, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * RoomGetRandomNearbyRoom(
	GAME * game,
	ROOM * room)
{
	ASSERT_RETNULL(room);
	ASSERT_RETVAL(game, room);

	int nearbyRooms = RoomGetNearbyRoomCount(room);
	int index = RandGetNum(game, nearbyRooms);
	ROOM * nearbyRoom = RoomGetNearbyRoom(room, index);
	ASSERT_RETVAL(nearbyRoom, room);
	return nearbyRoom;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoClientRoomUpdate(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
	RoomUpdateClient( pUnit );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sMatchRoomNeedsUpdate(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ROOM *pRoom = (ROOM *)pCallbackData;
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	if (ClientIsRoomKnown( pClient, pRoom ))
	{
		UnitRegisterEventTimed( pPlayer, sDoClientRoomUpdate, NULL, 1 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomVisualPortalInit( 
	ROOM_VISUAL_PORTAL *pRoomPortal)
{
	ASSERTX_RETURN( pRoomPortal, "Expected room portal" );
	pRoomPortal->idVisualPortal = INVALID_ID;
	pRoomPortal->nEnginePortalID = INVALID_ID;
	pRoomPortal->dwFlags = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sRoomVisualPortalFindEngineID(
	UNIT *pUnit)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );	
	
	// get room portal
	const ROOM_VISUAL_PORTAL *pRoomPortal = RoomVisualPortalFind( pUnit );
	ASSERTX_RETINVALID( pRoomPortal, "Expected room portal A" );
	
	// return engine ID
	ASSERTX_RETINVALID( pRoomPortal->nEnginePortalID != INVALID_ID, "Invalid engine portal ID" );
	return pRoomPortal->nEnginePortalID;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static VISUAL_PORTAL_DIRECTION sGetVisualPortalDirection(
	UNIT *pUnit)
{
	ASSERTX_RETVAL( pUnit, VPD_INVALID, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return pUnitData->eOneWayVisualPortalDirection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVisualPortalConnect( 
	UNIT *pPortal)
{

	// find the opposite destination unit of this portal
	UNIT *pPortalOther = s_ObjectGetOppositeSubLevelPortal( pPortal );
	if (pPortalOther)
	{
		UNIT *pFrom = pPortal;
		UNIT *pTo = pPortalOther;
		
		// if this is a one way portal, order matters
		BOOL bBiDirectional = FALSE;		
		VISUAL_PORTAL_DIRECTION eDir = sGetVisualPortalDirection( pPortal );
		if (eDir != VPD_INVALID )
		{
			switch (eDir)
			{
				case VPD_FROM:	pFrom = pPortal; pTo = pPortalOther; break;
				case VPD_TO:	pFrom = pPortalOther; pTo = pPortal; break;
				default:		ASSERTX_RETURN( 0, "Invalid one way visual portal direction" ); break;
			}			
		}
		else
		{
			bBiDirectional = TRUE;
		}
		
		// get engine ids (guaranteed to exist at this point)
		//int nEnginePortalFrom = sRoomVisualPortalFindEngineID( pFrom );
		//int nEnginePortalTo = sRoomVisualPortalFindEngineID( pTo );

		// connect from -> to
		// CHRIS: PORTAL
		//PORTAL * pPortalFrom = e_PortalGet( nEnginePortalFrom );
		//PORTAL * pPortalTo   = e_PortalGet( nEnginePortalTo );
		//e_PortalSetDestination( pPortalFrom, pPortalTo );
		//
		//// connect bi-directional portals
		//if (bBiDirectional)
		//{
		//	// CHRIS: PORTAL
		//	e_PortalSetDestination( pPortalTo, pPortalFrom );
		//}

		//e_PortalSetDestination( pPortalTo, pPortalFrom );
		//V( e_PortalConnect( pPortalTo ) );
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sVisualPortalMakeShape(
	VECTOR * vCorners,
	const VECTOR & vDir,
	const VECTOR & vPos,
	const VECTOR & vUp,
	float fScaleFactor,
	int nCorners )
{
	ASSERT_RETURN( vCorners );
	ASSERT_RETURN( nCorners == 4 );

	// Assume 1 unit base scale
	float fBaseScale = 1.f;

	VECTOR vW, vH;
	vH = vUp;
	VectorNormalize( vH );
	VectorCross( vW, vH, vDir );
	VectorNormalize( vW );

	float fScale = fBaseScale * fScaleFactor;
	vCorners[0] = vPos + vW * (-fScale) + vH * ( 0.f );
	vCorners[1] = vPos + vW * (-fScale) + vH * ( fScale * 2.f );
	vCorners[2] = vPos + vW * ( fScale) + vH * ( 0.f );
	vCorners[3] = vPos + vW * ( fScale) + vH * ( fScale * 2.f );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomAddVisualPortal(
	GAME *pGame,
	ROOM *pRoom,
	UNIT *pPortalUnit)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pPortalUnit, "Expected unit" );
	ASSERTX_RETURN( ObjectIsVisualPortal( pPortalUnit ), "Expected visual portal" );
	ASSERTX_RETURN( pRoom->nNumVisualPortals < MAX_VISUAL_PORTALS_PER_ROOM - 1, "Too many visual portals in room" );
	
	// add unitid to array	
	ASSERTX_RETURN( pRoom->nNumVisualPortals < MAX_VISUAL_PORTALS_PER_ROOM - 1, "Too many visual portals in room" );
	ROOM_VISUAL_PORTAL *pRoomPortal = &pRoom->pVisualPortals[ pRoom->nNumVisualPortals++ ];
	ASSERTX_RETURN( pRoomPortal, "Expected room portal" );
	sRoomVisualPortalInit( pRoomPortal );
	pRoomPortal->idVisualPortal = UnitGetId( pPortalUnit );
	
#if !ISVERSION(SERVER_VERSION)
	// client only logic
	if (IS_CLIENT( pGame ))
	{
		ASSERTX_RETURN( pPortalUnit->pGfx, "Portal unit GFX needs to exist by the time we get here!" );

		int nRegionCurrent = RoomGetSubLevelEngineRegion( pRoom );
		pRoomPortal->nEnginePortalID = e_PortalCreate( nRegionCurrent );
		ASSERT_RETURN( pRoomPortal->nEnginePortalID != INVALID_ID );
		PORTAL * pEnginePortal = e_PortalGet( pRoomPortal->nEnginePortalID );
		ASSERT_RETURN( pEnginePortal );

		if ( pPortalUnit->pRoom )
		{
			e_ModelSetRoomID( pPortalUnit->pGfx->nModelIdCurrent, pPortalUnit->pRoom->idRoom );
		}

		c_ModelSetRegion( pPortalUnit->pGfx->nModelIdCurrent, nRegionCurrent );
		int nCellID = e_ModelGetCellID( pPortalUnit->pGfx->nModelIdCurrent );
		ASSERTX( nCellID != INVALID_ID, "No cell found for portal unit model!" );
		e_PortalSetCellID( pEnginePortal, nCellID );

		VECTOR vCorners[4];
		sVisualPortalMakeShape( vCorners, pPortalUnit->vFaceDirection, pPortalUnit->vPosition, pPortalUnit->vUpDirection, pPortalUnit->fScale, 4 );


		// just sets the normal
		V( e_PortalSetShape(
			pEnginePortal,
			vCorners,
			4,
			&pPortalUnit->vFaceDirection ) );

		e_PortalSetFlags( pEnginePortal, PORTAL_FLAG_ACTIVE, TRUE );

		// get the sublevel and region that this portal goes to
		SUBLEVELID idSubLevelOpposite = s_ObjectGetOppositeSubLevel( pPortalUnit );	
		if (idSubLevelOpposite != INVALID_ID)
		{	
			// attempt to link portal with another one
			sVisualPortalConnect( pPortalUnit );
		}
	}
	
	// adding visual portals to rooms that contain clients will need to cause that
	// client to update their room data because the visual portals dynamically create
	// connections to the opposite rooms
	if (IS_SERVER( pGame ))
	{
		PlayersIterate( pGame, sMatchRoomNeedsUpdate, pRoom );
	}
	
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomRemoveVisualPortal(
	GAME *pGame,
	ROOM *pRoom,
	UNIT *pPortal)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pPortal, "Expected unit" );
	ASSERTX_RETURN( ObjectIsVisualPortal( pPortal ), "Expected visual portal" );

	// find visual portal in room
	UNITID idPortal = UnitGetId( pPortal );
	for (int i = 0; i < pRoom->nNumVisualPortals; ++i)
	{
		ROOM_VISUAL_PORTAL *pRoomPortal = &pRoom->pVisualPortals[ i ];		
		
		if (pRoomPortal->idVisualPortal == idPortal)
		{

#if !ISVERSION(SERVER_VERSION)
			// break any links to the opposite portal
			if (IS_CLIENT( pGame ))
			{
							
				// remove engine portal (this removes any connections to this portal too)
				if (pRoomPortal->nEnginePortalID != INVALID_ID)
				{
					e_PortalRemove( pRoomPortal->nEnginePortalID );
					pRoomPortal->nEnginePortalID = INVALID_ID;
				}
				
			}
#endif

			// put the last portal in its place
			*pRoomPortal = pRoom->pVisualPortals[ pRoom->nNumVisualPortals - 1 ];
			
			// clear last entry just to be clean
			sRoomVisualPortalInit( &pRoom->pVisualPortals[ pRoom->nNumVisualPortals - 1 ] );

			// one less portal now
			pRoom->nNumVisualPortals--;
			break;
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const ROOM_VISUAL_PORTAL *RoomVisualPortalFind(
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	UNITID idUnit = UnitGetId( pUnit );
	ROOM *pRoom = UnitGetRoom( pUnit );
	for (int i = 0; i < pRoom->nNumVisualPortals; ++i)
	{
		const ROOM_VISUAL_PORTAL *pRVP = &pRoom->pVisualPortals[ i ];
		if (pRVP->idVisualPortal == idUnit)
		{
			return pRVP;
		}
	}
	return NULL;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ClearVisibleRooms()
{
	ArrayClear(gtVisibleRoomCommands);
	gnNumVisibleRoomPasses = 0;
	if ( AppIsTugboat() )
		ArrayClear(gVisibleRoomList);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_ProcessVisibleRoomsList ()
{
	sgnRoomVisibleCount = 0;

#if !ISVERSION(SERVER_VERSION)
	int nCount = gtVisibleRoomCommands.Count();
	if ( nCount <= 0)
		return;

	// get the view parameters and update frustum if necessary
	MATRIX matWorld, matView, matProj;
	VECTOR vEyeVector;
	VECTOR vEyeLocation;
	e_InitMatrices( &matWorld, &matView, &matProj, NULL, &vEyeVector, &vEyeLocation, NULL, TRUE );

	int nControlRoomID = RoomGetControlUnitRoom();
	if ( AppIsHellgate() && nControlRoomID == INVALID_ID )
	{
		c_ClearVisibleRooms();
		return;
	}

	BOOL bForceVisible = c_GetToolMode();

	BOOL bFrustumCull = e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_ROOMS );
	const PLANE * tPlanes = NULL;
	if ( bFrustumCull || e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS ) )
	{
		tPlanes = e_GetCurrentViewFrustum()->GetPlanes();
		ASSERT_RETURN( tPlanes );
	}

	RL_PASS_DESC tDesc;
	tDesc.eType = RL_PASS_NORMAL;
	if ( AppIsHellgate() )
		tDesc.dwFlags = RL_PASS_FLAG_CLEAR_BACKGROUND | RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_RENDER_SKYBOX | RL_PASS_FLAG_GENERATE_SHADOWS;
	else
	{
		tDesc.dwFlags = RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_GENERATE_SHADOWS;	
		int nViewType = c_CameraGetViewType();
		if( nViewType != VIEW_VANITY )
		{
			tDesc.dwFlags |= RL_PASS_FLAG_DRAW_BEHIND;
			tDesc.dwFlags |= RL_PASS_FLAG_RENDER_SKYBOX;	
		}
		else
		{
			tDesc.dwFlags |= RL_PASS_FLAG_DRAW_BEHIND;
			tDesc.dwFlags |= RL_PASS_FLAG_RENDER_SKYBOX;
		}
	}
	PASSID nTopPass = e_RenderListBeginPass( tDesc, e_GetCurrentRegionID() );

	if ( nTopPass == INVALID_ID )
		return;

	PASSID nPass = INVALID_ID;
	
	for ( int i = 0; i < nCount; i++ )
	{
		const VISIBLE_ROOM_COMMAND & tCommand = gtVisibleRoomCommands[ i ];

		if ( tCommand.nCommand == VRCOM_BEGIN_SECTION )
		{
			PERF(ROOM_PROCESS_BEGIN);

			ASSERTX_CONTINUE( nPass == INVALID_ID, "Error, two nested render sections detected. Perhaps one visual portal is visible through another?" );
			RL_PASS_DESC tDesc;
			tDesc.eType = RL_PASS_PORTAL;
			if ( AppIsHellgate() )
				tDesc.dwFlags = RL_PASS_FLAG_CLEAR_BACKGROUND | RL_PASS_FLAG_RENDER_PARTICLES | RL_PASS_FLAG_RENDER_SKYBOX;
			else
			{
				tDesc.dwFlags = RL_PASS_FLAG_RENDER_PARTICLES ;
				int nViewType = c_CameraGetViewType();
				if( nViewType != VIEW_VANITY )
				{
					tDesc.dwFlags |= RL_PASS_FLAG_DRAW_BEHIND;	
					tDesc.dwFlags |= RL_PASS_FLAG_RENDER_SKYBOX;
				}
				else
				{
					tDesc.dwFlags |= RL_PASS_FLAG_DRAW_BEHIND;	
					tDesc.dwFlags |= RL_PASS_FLAG_RENDER_SKYBOX;
				}

			}
			tDesc.nPortalID = (int)tCommand.dwData;
			nPass = e_RenderListBeginPass( tDesc, e_GetCurrentRegionID() );
		}
		else if ( tCommand.nCommand == VRCOM_END_SECTION )
		{
			PERF(ROOM_PROCESS_END);

			ASSERT_CONTINUE( nPass != INVALID_ID );
			e_RenderListEndPass( nPass );
			nPass = INVALID_ID;
		}
		else if ( tCommand.nCommand == VRCOM_ADD_ROOM )
		{
			PERF(ROOM_PROCESS_ADD);

			PASSID nThisPass = nTopPass;
			if ( nPass != INVALID_ID )
				nThisPass = nPass;

			const ROOM * pRoom = (ROOM*)tCommand.dwData;
			ASSERT_CONTINUE( pRoom );
			BOOL bControlRoom = ((ROOMID)nControlRoomID == pRoom->idRoom);
			int nRootModel = RoomGetRootModel( pRoom );

			if ( bFrustumCull )
			{
				BOOL bInFrustum = e_ModelInFrustum( nRootModel, tPlanes );

				// this doesn't handle rooms new to the visible list very well... but in outdoors, they're all always in that list
				e_ModelSwapInFrustum( nRootModel );
				e_ModelSetInFrustum( nRootModel, bInFrustum );

				// do a frustum cull
				if ( ! bControlRoom && ! bInFrustum )
				{
					// not visible -- mark as so
					//c_SetRoomModelsVisible( pRoom, FALSE );
					//gtVisibleRooms.Remove( nVisRoom );
					continue;
				}
				c_SetRoomModelsVisible( pRoom, TRUE, tPlanes );

				e_MetricsIncRoomsInFrustum();
			}

			sgnRoomVisibleCount++;

			// room is in the frustum -- add its models to the render list
			DWORD dwAddFlags = RENDERABLE_FLAG_OCCLUDABLE;
			if ( bControlRoom )
				dwAddFlags |= RENDERABLE_FLAG_NOCULLING;
			int nRootIndex = e_RenderListAddModel( nThisPass, nRootModel, dwAddFlags );
			e_RenderListSetBBox( nThisPass, nRootIndex, &tCommand.ClipBox );
			DWORD dwFlags = 0;
			//DWORD dwFlags = RENDERABLE_FLAG_ALLOWINVISIBLE;
			int nCount;

			nCount = pRoom->pnRoomModels.Count();
			for ( int i = 0; i < nCount; i++ )
			{
				if ( pRoom->pnRoomModels[ i ] == nRootModel )
					continue;
				if ( ! ( bForceVisible || e_ModelCurrentVisibilityToken( pRoom->pnRoomModels[ i ] ) ) )
					continue;
				int nIndex = e_RenderListAddModel( nThisPass, pRoom->pnRoomModels[ i ], dwFlags );
				e_RenderListSetParent( nThisPass, nIndex, nRootIndex );
				e_RenderListSetBBox( nThisPass, nIndex, &tCommand.ClipBox );
			}

			nCount = pRoom->pnModels.Count();
			for ( int i = 0; i < nCount; i++ )
			{
				if ( ! ( bForceVisible || e_ModelCurrentVisibilityToken( pRoom->pnModels[ i ] ) ) )
					continue;
				int nIndex = e_RenderListAddModel( nThisPass, pRoom->pnModels[ i ], dwFlags );
				e_RenderListSetParent( nThisPass, nIndex, nRootIndex );
				e_RenderListSetBBox( nThisPass, nIndex, &tCommand.ClipBox );
			}
		}
		else
		{
			ASSERTX_CONTINUE( 0, "Unexpected visible room command encountered!" );
		}
	}

	{
		PERF(ROOM_PROCESS_END);

		ASSERTX( nPass == INVALID_ID, "Pass not terminated in visible room list!" );
		if ( nPass != INVALID_ID )
			e_RenderListEndPass( nPass );

		e_RenderListEndPass( nTopPass );
	}
#endif // !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ModelRemove(
	int nModelID )
{
	ROOMID idRoom = c_ModelGetRoomID( nModelID );
	if ( idRoom != INVALID_ID )
	{
		// If there is no game or no dungeon, then there's no room to remove our model from
		GAME* pGame = AppGetCltGame();
		if ( pGame && pGame->m_Dungeon )
		{
			ROOM * pRoom = RoomGetByID(pGame, idRoom );
			if ( pRoom )
			{
				c_RoomRemoveModel( pRoom, nModelID );
			}
		}

		// Let's clear out this user data so that, if we're asked again, we'll respond with "there is no room"
		e_ModelSetRoomID( nModelID, INVALID_ID );
	}

	V( e_ModelRemovePrivate( nModelID ) );
}

void c_ModelSetScale( int nModelID,
					 VECTOR vScale )
{
	if( AppIsTugboat() )
	{
		V( e_ModelSetScale( nModelID, vScale ) );
	}
	else 
	{

		//V( e_ModelSetScale( nModelID, fScale ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ModelSetLocation(
	int nModelID,
	const MATRIX * pmatWorld,
	const VECTOR & vPosition,
	int nRoomID,
	MODEL_SORT_TYPE SortType,
	BOOL bForce )
{
	V( e_ModelSetLocationPrivate( nModelID, pmatWorld, vPosition, SortType, bForce, FALSE ) );

	if ( e_ModelExists( nModelID ) )
	{
		int nModelRoomID = (int)e_ModelGetRoomID( nModelID );
		if ( nModelRoomID != nRoomID || bForce )
		{
			ROOM * pOldRoom = ( nModelRoomID != INVALID_ID )	? RoomGetByID( AppGetCltGame(), nModelRoomID )		: NULL;
			ROOM * pNewRoom = ( nRoomID		 != INVALID_ID )	? RoomGetByID( AppGetCltGame(), nRoomID )			: NULL;

			ASSERT(!pOldRoom || RoomGetId(pOldRoom) == (ROOMID)nModelRoomID);
			c_RoomRemoveModel( pOldRoom, nModelID );

			e_ModelSetRoomID( nModelID, (DWORD)nRoomID );

			if ( pNewRoom )
			{
				c_RoomAddModel( pNewRoom, nModelID, SortType );
			}
		}

		c_ModelAttachmentsUpdate( nModelID );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

ROOMID c_ModelGetRoomID( int nModelID )
{
	return (ROOMID)e_ModelGetRoomID( nModelID );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomGetMaterialInfoByTriangleNumber(
	const ROOM_INDEX * pRoomIndex,
	const int nTriangleNumber,
	int * nMaterialIndex,
	int * nMaterialIndexMapped,
	float * fDirectOcclusion,
	float * fReverbOcclusion,
	DWORD * pdwDebugColor,
	BOOL bOcclusion)
{
	if(nMaterialIndex)
	{
		*nMaterialIndex = INVALID_ID;
	}
	if(nMaterialIndexMapped)
	{
		*nMaterialIndexMapped = INVALID_ID;
	}
	if(fDirectOcclusion)
	{
		*fDirectOcclusion = 0.0f;
	}
	if(fReverbOcclusion)
	{
		*fReverbOcclusion = 0.0f;
	}
	if(!pRoomIndex || !pRoomIndex->pRoomDefinition)
	{
		return;
	}
	
	if(bOcclusion && pRoomIndex->bDontObstructSound)
	{
		return;
	}

	ROOM_DEFINITION * pRoomDef = pRoomIndex->pRoomDefinition;
	const ROOM_COLLISION_MATERIAL_INDEX * pMaterialIndex = GetMaterialFromIndex(pRoomDef, nTriangleNumber);
	if(pMaterialIndex)
	{
		const ROOM_COLLISION_MATERIAL_DATA * pMaterialData = FindMaterialWithIndex(pMaterialIndex->nMaterialIndex);
		if(!pMaterialData)
			return;

		if(pMaterialData->nMapsTo >= 0)
		{
			const ROOM_COLLISION_MATERIAL_DATA * pMaterialDataMapsTo = (const ROOM_COLLISION_MATERIAL_DATA *)ExcelGetData(NULL, DATATABLE_MATERIALS_COLLISION, pMaterialData->nMapsTo);
			if(nMaterialIndexMapped)
			{
				if(pMaterialDataMapsTo)
				{
					*nMaterialIndexMapped = pMaterialDataMapsTo->nMaterialNumber;
				}
				else
				{
					*nMaterialIndexMapped = pMaterialData->nMaterialNumber;
				}
			}
		}
		else
		{
			if(nMaterialIndexMapped)
			{
				*nMaterialIndexMapped = pMaterialData->nMaterialNumber;
			}
		}

		if(nMaterialIndex)
		{
			*nMaterialIndex = pMaterialData->nMaterialNumber;
		}

		if(fDirectOcclusion)
		{
			*fDirectOcclusion = pMaterialData->fDirectOcclusion;
		}

		if(fReverbOcclusion)
		{
			*fReverbOcclusion = pMaterialData->fReverbOcclusion;
		}

		if(pdwDebugColor)
		{
			*pdwDebugColor = GetFontColor(pMaterialData->nDebugColor);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * ClientControlUnitGetRoom()
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETNULL(pGame);

	UNIT * pPlayer = GameGetControlUnit(pGame);
	ASSERT_RETNULL(pPlayer);

	return UnitGetRoom(pPlayer);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientControlUnitGetPosition(VECTOR & vPosition)
{
	GAME * pGame = AppGetCltGame();
	if(!pGame)
		return FALSE;

	UNIT * pPlayer = GameGetControlUnit(pGame);
	if(!pPlayer)
		return FALSE;

	vPosition = UnitGetCenter(pPlayer);

	return TRUE;
}

//----------------------------------------------------------------------------
enum CONNECTED_NODE_SEARCH_FLAGS
{
	CONNECTED_NODE_NO_EDGES_BIT,		// do not allow edge nodes
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsClearFlatArea( 
	ROOM *pRoom, 
	const ROOM_PATH_NODE *pNodeAnchor,
	ROOM_PATH_NODE *pNode, 
	float flMinNodeHeight, 
	float flMaxDistanceSq,
	float flFlatZTolerance,
	DWORD dwConnectedFlags)
{	
#if HELLGATE_ONLY	
	PATH_NODE_INSTANCE *pNodeInstance = RoomGetPathNodeInstance( pRoom, pNode->nIndex );
	
	// don't do this again for nodes that have done it before
	if (TESTBIT( pNodeInstance->dwNodeInstanceFlags, NIF_MARKED ))
	{
		return TRUE;
	}

	// mark this node as traversed
	SETBIT( pNodeInstance->dwNodeInstanceFlags, NIF_MARKED );

	// all nodes must have the max open connection count, ones near walls or around obstacles
	// will not.  note that we ignore edge nodes though, as those tie up seams between rooms
	if (pNode->nConnectionCount != 8 && TESTBIT( pNode->dwFlags, ROOM_PATH_NODE_EDGE ) == FALSE)
	{
		return FALSE;
	}

	// check this node
	if (pNode->fHeight < flMinNodeHeight)
	{
		return FALSE;
	}

	
	// check for edges when not allowed
	if (TESTBIT( dwConnectedFlags, CONNECTED_NODE_NO_EDGES_BIT ) == TRUE &&
		TESTBIT( pNode->dwFlags, ROOM_PATH_NODE_EDGE ) == TRUE)
	{
		return FALSE;
	}

	// must be within Z tolerance to be considered flat
	if (fabs( pNodeAnchor->vPosition.fZ - pNode->vPosition.fZ ) > flFlatZTolerance)
	{
		return FALSE;
	}



	// normal must be enough alike to the anchor for a flat area
	float flDot = VectorDot( pNodeAnchor->vNormal, pNode->vNormal );
	const float flMaxAngleBetweenNormalsInDegrees = 5.0f;
	if (flDot < (1.0f - (flMaxAngleBetweenNormalsInDegrees / 90.0f)))
	{
		return FALSE;
	}

	// check node against the max allowed slope

	// do not allow blocked nodes
	if (s_RoomNodeIsBlocked( pRoom, pNode->nIndex, 0, NULL ) != NODE_FREE)
	{
		return FALSE;
	}
	
	// check the connected nodes
	for (int i = 0; i < pNode->nConnectionCount; ++i)
	{
		ROOM_PATH_NODE_CONNECTION *pConnection = &pNode->pConnections[ i ];
		ROOM_PATH_NODE *pNodeConnected = pConnection->pConnection;

		// check connections if they are close enough
		if (VectorDistanceSquared( pNodeConnected->vPosition, pNodeAnchor->vPosition ) <= flMaxDistanceSq)
		{
				
			if (sIsClearFlatArea( 
					pRoom, 
					pNodeAnchor, 
					pNodeConnected, 
					flMinNodeHeight, 
					flMaxDistanceSq, 
					flFlatZTolerance,
					dwConnectedFlags) == FALSE)
			{
				return FALSE;
			}

		}
				
	}
#endif	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomClearMarkedNodes(
	ROOM *pRoom)
{
	ROOM_PATH_NODE_SET *pPathNodeSet = RoomGetPathNodeSet( pRoom );
	
	for (int i = 0; i < pPathNodeSet->nPathNodeCount; ++i)
	{

		PATH_NODE_INSTANCE *pNodeInstance = RoomGetPathNodeInstance( pRoom, i );
		CLEARBIT( pNodeInstance->dwNodeInstanceFlags, NIF_MARKED );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomGetClearFlatArea(
	ROOM *pRoom,
	float flMinNodeHeight,
	float flRadius,
	VECTOR *pvPositionWorld,
	VECTOR *pvNormal,	
	BOOL bAllowEdgeNodeAnchor,
	float flFlatZTolerance)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( pvPositionWorld, "Expected vector pointer" );
	GAME *pGame = RoomGetGame( pRoom );
	
	ROOM_PATH_NODE_SET *pPathNodeSet = RoomGetPathNodeSet( pRoom );	
	if (pPathNodeSet->nPathNodeCount == 0)
	{
		return FALSE;
	}
	
	// setup flags for searching
	DWORD dwConnectedFlags = 0;
	//SETBIT( dwConnectedFlags, CONNECTED_NODE_NO_EDGES_BIT );

	BOOL bFoundPosition = FALSE;
	int nTriesLeft = 16;
	while (nTriesLeft--)
	{
				
		// select a node at random
		int nAnchorTriesLeft = 32;
		ROOM_PATH_NODE *pNode = NULL;
		while( pNode == NULL && nAnchorTriesLeft--)
		{
		
			// select node at random
			int nNodeIndex = RandGetNum( pGame, 0, pPathNodeSet->nPathNodeCount - 1 );
			ROOM_PATH_NODE *pThisPathNode = RoomGetRoomPathNode( pRoom, nNodeIndex );
			
			// do not allow edge nodes in some cases
			if (bAllowEdgeNodeAnchor == FALSE && TESTBIT( pThisPathNode->dwFlags, ROOM_PATH_NODE_EDGE ) == TRUE)
			{
				continue;
			}
			
			// ok we're good
			pNode = pThisPathNode;
			break;
				
		}
		if (pNode == NULL)
		{
			continue;
		}
		
		// check the surrounding pathnodes as well
		BOOL bClear = sIsClearFlatArea( 
				pRoom, 
				pNode, 
				pNode, 
				flMinNodeHeight, 
				flRadius * flRadius, 
				flFlatZTolerance,
				dwConnectedFlags);

		// clear marks from this traversal
		sRoomClearMarkedNodes( pRoom );
				
		if (bClear)				
		{
			*pvPositionWorld = RoomPathNodeGetWorldPosition( pGame, pRoom, pNode );
#if HELLGATE_ONLY
			if (pvNormal)
			{
				*pvNormal = pNode->vNormal;
			}
#else
			*pvNormal = VECTOR( 0, 0, 1 );
#endif
			bFoundPosition = TRUE;
			break;
		}

	}

	// did we find a spot?
	return bFoundPosition;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomIsClearFlatArea( 
	ROOM *pRoom, 
	ROOM_PATH_NODE *pNode,
	float flRadius,
	float flMinNodeHeight,
	float flZTolerance)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	ASSERTX_RETFALSE( pNode, "Expected path node" );

	// setup flags for searching
	DWORD dwConnectedFlags = 0;
//	SETBIT( dwConnectedFlags, CONNECTED_NODE_NO_EDGES_BIT );
			
	// check the surrounding pathnodes as well
	BOOL bClear = sIsClearFlatArea( 
			pRoom, 
			pNode, 
			pNode, 
			flMinNodeHeight, 
			flRadius * flRadius, 
			flZTolerance,
			dwConnectedFlags);

	// clear marks from this traversal
	sRoomClearMarkedNodes( pRoom );

	return bClear;	
}	
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSelectSubLevelEntranceFromPathnodes( 
	ROOM *pRoom, 
	VECTOR &vPosition,
	const SUBLEVEL *pSubLevel)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	
	const float DEFAULT_MIN_NODE_HEIGHT = 3.0f;
	const float DEFAULT_RADIUS = 2.5f;

	// get a random location that is clear and "flat"
	float flFlatZTolerance = DEFAULT_FLAT_Z_TOLERANCE;
	float flFlatRadius = DEFAULT_RADIUS;	
	float flFlatMinHeight = DEFAULT_MIN_NODE_HEIGHT;
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	if (ptSubLevelDef->flEntranceFlatZTolerance != 0.0f)
	{
		flFlatZTolerance = ptSubLevelDef->flEntranceFlatZTolerance;
	}
	if (ptSubLevelDef->flEntranceFlatRadius != 0.0f)
	{
		flFlatRadius= ptSubLevelDef->flEntranceFlatRadius;
	}
	if (ptSubLevelDef->flEntranceFlatRadius != 0.0f)
	{
		flFlatMinHeight = ptSubLevelDef->flEntranceFlatHeightMin;
	}	
	return RoomGetClearFlatArea( pRoom, flFlatMinHeight, flFlatRadius, &vPosition, NULL, FALSE, flFlatZTolerance );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RoomSelectPossibleSubLevelEntranceLocation(
	GAME *pGame,
	ROOM *pRoom,
	SUBLEVEL *pSubLevel)
{
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	const int MIN_PATHNODES_IN_ROOM_FOR_SUBLEVEL_ENTRANCE = 16;  // arbitrary
	const int MAX_LOCATIONS = 64;
	VECTOR vPosition[ MAX_LOCATIONS ];
	int nNumLocations = 0;
			
	// skip rooms that are not in the default level
	if (RoomIsInDefaultSubLevel( pRoom ) == FALSE)
	{
		return FALSE;
	}

	// skip rooms that don't allow it
	if (RoomTestFlag( pRoom, ROOM_NO_SUBLEVEL_ENTRANCE_BIT))
	{
		return FALSE;
	}

	// if room doesn't allow monster spawning, we don't do this either
	if (RoomAllowsMonsterSpawn( pRoom ) == FALSE)
	{
		return FALSE;
	}

	// should we search for layout markers?
	if (ptSubLevelDef->bAllowLayoutMarkersForEntrance == TRUE)
	{	
		const char *pszName = ptSubLevelDef->szLayoutMarkerEntrance;
		
		// save specific nodes in the room
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation;
		ROOM_LAYOUT_GROUP *pLayoutGroup = RoomGetLabelNode( pRoom, pszName, &pOrientation );
		if (pLayoutGroup)
		{
			for ( int i = 0; i < pLayoutGroup->nGroupCount; i++ )
			{
				if (nNumLocations < MAX_LOCATIONS - 1)
				{
					ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientationNode = NULL;
					ROOM_LAYOUT_GROUP *pNode = s_QuestNodeSetGetNode( pRoom, pLayoutGroup, &pOrientationNode, i+1 );
					ASSERT_CONTINUE(pNode);
					ASSERT_CONTINUE(pOrientationNode);
					ASSERT_BREAK(nNumLocations < MAX_LOCATIONS - 1);
					s_QuestNodeGetPosAndDir( 
						pRoom, 
						pOrientationNode, 
						&vPosition[ nNumLocations ], 
						NULL, 
						NULL, 
						NULL );
					nNumLocations++;
				}
			}
		}
	}
	
	// see if this sublevel only allows us to pick from path nodes
	if (ptSubLevelDef->bAllowPathNodesForEntrance == TRUE)
	{
	
		// there must be enough path nodes
		ROOM_PATH_NODE_SET *pPathnodeSet = RoomGetPathNodeSet( pRoom );
		if (pPathnodeSet->nPathNodeCount < MIN_PATHNODES_IN_ROOM_FOR_SUBLEVEL_ENTRANCE )
		{
			return FALSE;
		}

		// using the path nodes, select a few locations
		int nNumTries = 8;
		while (nNumTries--)
		{
			if (nNumLocations < MAX_LOCATIONS - 1)
			{
				if (sSelectSubLevelEntranceFromPathnodes( pRoom, vPosition[ nNumLocations ], pSubLevel ))
				{
					nNumLocations++;
				}
			}
		}

	}
		
	// we have no suitable locations
	if (nNumLocations == 0)
	{
		return FALSE;
	}
	
	// choose a location
	int nIndex = RandGetNum( pGame, 0, nNumLocations - 1 );

	// save the location in the room so that we don't have to do it later
	pRoom->vPossibleSubLevelEntrance = vPosition[ nIndex ];

	// we found a suitable location
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_RoomAllowedForStart(
	ROOM *pRoom,
	DWORD dwWarpFlags)
{
	if (RoomIsInDefaultSubLevel( pRoom ) == FALSE)
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomsAreInSameSubLevel(
	const ROOM *pRoomA,
	const ROOM *pRoomB)
{
	ASSERTX_RETFALSE( pRoomA, "Expected room" );
	ASSERTX_RETFALSE( pRoomB, "Expected room" );
	return RoomGetSubLevelID( pRoomA ) == RoomGetSubLevelID( pRoomB );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomIsInDefaultSubLevel(
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	int nSubLevelDefDefault = LevelGetDefaultSubLevelDefinition( pLevel );
	SUBLEVELID idSubLevelDefault = SubLevelFindFirstOfType( pLevel, nSubLevelDefDefault );
	return RoomGetSubLevelID( pRoom ) == idSubLevelDefault;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct ALL_NEIGHBORING_ROOM_LISTS
{
	ROOM_LIST tRoomList[ NR_NUM_TYPES ];
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetRoomDebugText( 
	ROOM *pRoom, 
	UNIT *pUnit, 
	WCHAR *puszBuffer,
	int nBufferSize,
	ALL_NEIGHBORING_ROOM_LISTS *pAllNeighboringRoomLists)
{
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferSize > 0, "Invalid buffer size" );

	// terminate
	puszBuffer[ 0 ] = 0;
	
	BOOL bActiveOnServer = FALSE;
#if !ISVERSION( CLIENT_ONLY_VERSION )
	if (AppIsSinglePlayer())
	{
		GAME *pGameServer = AppGetSrvGame();
		ROOMID idRoom = RoomGetId( pRoom );
		ROOM *pRoomServer = RoomGetByID( pGameServer, idRoom );
		bActiveOnServer = RoomIsActive(pRoomServer);
	}
#endif
	// print room id into buffer	
	char cColor = FONTCOLOR_WHITE;
	if (UnitGetRoom( pUnit ) == pRoom)
	{
		cColor = FONTCOLOR_CYAN;
	}
	else if (bActiveOnServer)
	{
		cColor = FONTCOLOR_LIGHT_PURPLE;
	}

	UIAddColorToString(puszBuffer, (FONTCOLOR)cColor, nBufferSize );

	// print to buffer
	WCHAR uszRoomID[32];
	PStrPrintf( 
		uszRoomID, 
		arrsize(uszRoomID), 
		L"%d\n", 
		RoomGetId( pRoom ));

	PStrCat(puszBuffer, uszRoomID, nBufferSize);

	// append server side neighboring information
	if (e_GetRenderFlag( RENDER_FLAG_ROOM_OVERVIEW_NEIGHBOR ))
	{
		if (sRoomInRoomList( pRoom, &pAllNeighboringRoomLists->tRoomList[ NR_VISIBLE ] ))
		{
			UIColorCatString( puszBuffer, nBufferSize, FONTCOLOR_VERY_LIGHT_GREEN, L"V" );
		}
		if (sRoomInRoomList( pRoom, &pAllNeighboringRoomLists->tRoomList[ NR_NEARBY ] ))
		{
			UIColorCatString( puszBuffer, nBufferSize, FONTCOLOR_VERY_LIGHT_BLUE, L"N" );
		}	
		if (sRoomInRoomList( pRoom, &pAllNeighboringRoomLists->tRoomList[ NR_CONNECTED ] ))
		{
			UIColorCatString( puszBuffer, nBufferSize, FONTCOLOR_YELLOW, L"C" );		
		}	
		if (sRoomInRoomList( pRoom, &pAllNeighboringRoomLists->tRoomList[ NR_ADJACENT ] ))
		{
			UIColorCatString( puszBuffer, nBufferSize, FONTCOLOR_VERY_LIGHT_CYAN, L"A" );
		}	
		if (RoomTestFlag( pRoom, ROOM_KNOWN_BIT ))
		{
			UIColorCatString( puszBuffer, nBufferSize, FONTCOLOR_VERY_LIGHT_RED, L"K" );		
		}
	}

	UIAddColorReturnToString( puszBuffer, nBufferSize );
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sDemoLevelDisplayDebugOverview()
{
#if ! ISVERSION(SERVER_VERSION) && ! ISVERSION( CLIENT_ONLY_VERSION ) && defined(DEMO_LEVEL_DISPLAY_DEBUG_INFO)

	DEMO_LEVEL_DEFINITION * pDemoLevel = DemoLevelGetDefinition();
	if ( ! pDemoLevel )
		return S_FALSE;

	if ( ! e_GetRenderFlag( RENDER_FLAG_PATH_OVERVIEW ) )
		return S_FALSE;

	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return S_FALSE;

	UNIT * pUnit = GameGetControlUnit( pGame );
	if ( ! pUnit || ! pUnit->pGfx )
		return S_FALSE;

	int nModel = pUnit->pGfx->nModelIdCurrent;
	if ( nModel == INVALID_ID )
		return S_FALSE;

	int nRegion = e_ModelGetRegion( nModel );
	REGION * pRegion = e_RegionGet( nRegion );
	ASSERT_RETFAIL( pRegion );

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	float fScale = MIN( nWidth / (pRegion->tBBox.vMax.x - pRegion->tBBox.vMin.x), nHeight / (pRegion->tBBox.vMax.y - pRegion->tBBox.vMin.y) );
	fScale *= 0.9f;
	VECTOR vT = VECTOR(0,0,0);
	e_GetViewPosition( &vT );
	VECTOR2 vEye = VECTOR2( vT.x, vT.y );
	pRegion->tBBox.Center( vT );
	VECTOR2 vWCenter( vT.x, vT.y );
	VECTOR2 vSCenter( nWidth * 0.5f, nHeight * 0.5f );
	float fRcpWidth = 2.f / nWidth;
	float fRcpHeight = 2.f / nHeight;

	DWORD dwLineColor		= 0x7f3f7fff;
	DWORD dwFillColorVis	= 0xb0307030;
	float fDepth = 0.1f;
	DWORD dwLineFlags = PRIM_FLAG_RENDER_ON_TOP;

	ROOMID * pRoomPath = NULL;
	int nRoomCount = 0;
	V_RETURN( DemoLevelDebugGetRoomPath( pRoomPath, nRoomCount ) );
	ASSERT_RETFAIL( pRoomPath );

	WCHAR wszLabel[8];

	for ( int i = 0; i < nRoomCount; ++i )	
	{
		ROOM * pRoom = RoomGetByID( pGame, pRoomPath[i] );
		ASSERT_CONTINUE( pRoom );

		VECTOR * pOBB = e_GetModelRenderOBBInWorld( RoomGetRootModel( pRoom ) );
		if ( ! pOBB )
			continue;

		VECTOR2 vC[4] = { *(VECTOR2*)&pOBB[4], *(VECTOR2*)&pOBB[5], *(VECTOR2*)&pOBB[6], *(VECTOR2*)&pOBB[7] };

		for ( int n = 0; n < 4; n++ )
		{
			vC[n] -= vWCenter;
			vC[n] *= fScale;
			vC[n] += vSCenter;
		}

		e_PrimitiveAddLine( dwLineFlags, &vC[0], &vC[1], dwLineColor, fDepth );
		e_PrimitiveAddLine( dwLineFlags, &vC[1], &vC[3], dwLineColor, fDepth );
		e_PrimitiveAddLine( dwLineFlags, &vC[3], &vC[2], dwLineColor, fDepth );
		e_PrimitiveAddLine( dwLineFlags, &vC[2], &vC[0], dwLineColor, fDepth );

		DWORD dwFillColor = dwFillColorVis;

		// fill room with color
		e_PrimitiveAddTri( PRIM_FLAG_SOLID_FILL, &vC[0], &vC[1], &vC[3], dwFillColor, 0.01f );
		e_PrimitiveAddTri( PRIM_FLAG_SOLID_FILL, &vC[3], &vC[2], &vC[0], dwFillColor, 0.01f );


		PStrPrintf( wszLabel, arrsize(wszLabel), L"%d", i );
		// find the center of each 2D room box and offset to clip space (-1..1)
		VECTOR vRoomCenter( 
			(    vC[0].x + vC[1].x + vC[2].x + vC[3].x ) * 0.25f * fRcpWidth  - 1.f,
			-( ( vC[0].y + vC[1].y + vC[2].y + vC[3].y ) * 0.25f * fRcpHeight - 1.f ),
			0.f );			
		CLT_VERSION_ONLY(UIDebugLabelAddScreen( wszLabel, &vRoomCenter, 1.0f, 0xffffffff, UIALIGN_CENTER, UIALIGN_CENTER, TRUE ));
	}

	e_GetViewDirection( &vT );
	VECTOR2 vLook( vT.x, vT.y );
	vLook *= 5.f;
	VECTOR2 vLookCross = VECTOR2( -vLook.y, vLook.x ) * 0.5f;
	VECTOR2 vCone[4] = { vEye, vEye + vLook + vLookCross, vEye + vLook - vLookCross, vEye + (vLook*2.f) };
	for ( int n = 0; n < arrsize(vCone); n++ )
	{
		vCone[n] -= vWCenter;
		vCone[n] *= fScale;
		vCone[n] += vSCenter;
	}
	e_PrimitiveAddTri( dwLineFlags | PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vCone[0], &vCone[1], &vCone[2], 0xffff0000, 0.01f );
	e_PrimitiveAddLine( dwLineFlags, &vCone[0], &vCone[3], 0xffffffff, 0.01f );

	V_RETURN( DemoLevelDebugRenderPath( vWCenter, fScale, vSCenter ) );

	return S_OK;
#else
	return S_FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RoomDisplayDebugOverview()
{
#if !ISVERSION( SERVER_VERSION )
	if ( S_OK == sDemoLevelDisplayDebugOverview() )
	{
		// Rendering a demolevel path, so don't render a room overview as well
		return;
	}

	int nPixelsPerMeter = e_GetRenderFlag( RENDER_FLAG_ROOM_OVERVIEW );
	if ( nPixelsPerMeter <= 0 )
	{
		nPixelsPerMeter = e_GetRenderFlag( RENDER_FLAG_ROOM_OVERVIEW_NEIGHBOR );
	}
	if (nPixelsPerMeter <= 0)
	{
		return;
	}

	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	// iterate all rooms
	GAME * pGame = AppGetCltGame();
	if ( ! pGame )
		return;
	UNIT * pUnit = GameGetControlUnit( pGame );
	ROOM * pRoomAnchor = UnitGetRoom( pUnit );
	LEVEL* pLevel = RoomGetLevel( pRoomAnchor );
	if ( ! pLevel )
		return;

	float fScale = (float)nPixelsPerMeter;
	VECTOR vEye;
	CLT_VERSION_ONLY(e_GetViewPosition( &vEye ));
	VECTOR2 vWCenter( vEye.x, vEye.y );
	DWORD dwLineColor		= 0x7f3f7fff;
	DWORD dwFillColorNoVis  = 0xff303030;
	DWORD dwFillColorVis	= 0xb0307030;
	float fDepth = 0.1f;
	DWORD dwLineFlags = PRIM_FLAG_RENDER_ON_TOP;
	VECTOR2 vSCenter( nWidth * 0.5f, nHeight * 0.5f );
	float fRcpWidth = 2.f / nWidth;
	float fRcpHeight = 2.f / nHeight;

	// build server neighboring room information
	ALL_NEIGHBORING_ROOM_LISTS tAllNeighboringRoomLists;
#if !ISVERSION( CLIENT_ONLY_VERSION )

	if (e_GetRenderFlag( RENDER_FLAG_ROOM_OVERVIEW_NEIGHBOR ))
	{
		for (int i = 0; i < NR_NUM_TYPES; ++i)
		{
			NEIGHBORING_ROOM eType = (NEIGHBORING_ROOM)i;
			ROOM_LIST *pRoomList = &tAllNeighboringRoomLists.tRoomList[ eType ];
			if (AppIsSinglePlayer())
			{	
				GAME *pGameServer = AppGetSrvGame();
				// When we're quitting, the server goes away first.
				if(pGameServer)
				{
					ROOMID idRoom = RoomGetId( pRoomAnchor );
					ROOM *pRoomServer = RoomGetByID( pGameServer, idRoom );
					RoomGetNeighboringRoomList(eType, pUnit, pRoomServer, pRoomList, FALSE);
				}
			}			
		}
	}
#endif		
	ROOM *pRoom = LevelGetFirstRoom( pLevel );
	BOUNDED_WHILE( pRoom, 100000 )
	{
		if ( pRoom->idSubLevel != pUnit->pRoom->idSubLevel )
		{
			//pRoom = LevelGetNextRoom( pRoom );
			//continue;
		}

		//DWORD dwIDColor = 0xffb0b0b0;
		DWORD dwIDColor = 0xffffffff;

		VECTOR * pOBB = e_GetModelRenderOBBInWorld( RoomGetRootModel( pRoom ) );
		if ( ! pOBB )
		{
			pRoom = LevelGetNextRoom( pRoom );
			continue;
		}

		// ORIENTED_BOUNDING_BOX point alignment
		//
		//       4-----5
		//      /|    /|
		// +Z  6-----7 |    -y
		//     | |   | |   
		// -z  | 0---|-1  +Y
		//     |/    |/
		//     2-----3
		//
		//      -x  +X

		// only render the tops of the boxes for visual clarity

		VECTOR2 vC[4] = { *(VECTOR2*)&pOBB[4], *(VECTOR2*)&pOBB[5], *(VECTOR2*)&pOBB[6], *(VECTOR2*)&pOBB[7] };

		for ( int n = 0; n < 4; n++ )
		{
			vC[n] -= vWCenter;
			vC[n] *= fScale;
			vC[n] += vSCenter;
		}
		CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vC[0], &vC[1], dwLineColor, fDepth ));
		CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vC[1], &vC[3], dwLineColor, fDepth ));
		CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vC[3], &vC[2], dwLineColor, fDepth ));
		CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vC[2], &vC[0], dwLineColor, fDepth ));

		// find the center of each 2D room box and offset to clip space (-1..1)
		VECTOR vRoomCenter( 
			(    vC[0].x + vC[1].x + vC[2].x + vC[3].x ) * 0.25f * fRcpWidth  - 1.f,
			-( ( vC[0].y + vC[1].y + vC[2].y + vC[3].y ) * 0.25f * fRcpHeight - 1.f ),
			0.f );			

		// get room text string
		const int MAX_STRING = 256;
		WCHAR wszString[ MAX_STRING ] = { 0 };
		sGetRoomDebugText( pRoom, pUnit, wszString, MAX_STRING, &tAllNeighboringRoomLists );
			
		CLT_VERSION_ONLY(UIDebugLabelAddScreen( wszString, &vRoomCenter, 0.8f, dwIDColor, UIALIGN_CENTER, UIALIGN_CENTER, TRUE ));

		DWORD dwFillColor = dwFillColorNoVis;
		if ( c_RoomIsVisible( pRoom ) )
		{
			// also draw the connection normals
			for ( int c = 0; c < pRoom->nConnectionCount; c++ )
			{
				ASSERT_BREAK( ROOM_CONNECTION_CORNER_COUNT == 4 );
				VECTOR2 vB[4] =
				{
					*(VECTOR2*)&pRoom->pConnections[c].pvBorderCorners[0],
					*(VECTOR2*)&pRoom->pConnections[c].pvBorderCorners[1],
					*(VECTOR2*)&pRoom->pConnections[c].pvBorderCorners[2],
					*(VECTOR2*)&pRoom->pConnections[c].pvBorderCorners[3]
				};
				for ( int n = 0; n < 4; n++ )
				{
					vB[n] -= vWCenter;
					vB[n] *= fScale;
					vB[n] += vSCenter;
				}
				DWORD dwConn = 0xffffff00;
				CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vB[0], &vB[1], dwConn, fDepth ));
				CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vB[0], &vB[2], dwConn, fDepth ));
				CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vB[0], &vB[3], dwConn, fDepth ));
				VECTOR2 vN[2];
				vN[0] = *(VECTOR2*)&pRoom->pConnections[c].vBorderCenter;
				VECTOR vT = pRoom->pConnections[c].vBorderCenter + pRoom->pConnections[c].vBorderNormal;
				vN[1] = *(VECTOR2*)&vT;
				DWORD dwNorm = 0xff00ff00;
				for ( int n = 0; n < 2; n++ )
				{
					vN[n] -= vWCenter;
					vN[n] *= fScale;
					vN[n] += vSCenter;
				}
				CLT_VERSION_ONLY(e_PrimitiveAddLine( dwLineFlags, &vN[0], &vN[1], dwNorm, fDepth ));
			}
			dwFillColor = dwFillColorVis;
		}
		
		// fill room with color
		CLT_VERSION_ONLY(e_PrimitiveAddTri( PRIM_FLAG_SOLID_FILL, &vC[0], &vC[1], &vC[3], dwFillColor, 0.01f ));
		CLT_VERSION_ONLY(e_PrimitiveAddTri( PRIM_FLAG_SOLID_FILL, &vC[3], &vC[2], &vC[0], dwFillColor, 0.01f ));

		pRoom = LevelGetNextRoom( pRoom );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD RoomGetIterateSequenceNumber( 
	ROOM *pRoom)
{
	ASSERTX_RETINVALID( pRoom, "Expected room" );
	if(AppIsHammer())
	{
		return 0;
	}
	GAME *pGame = RoomGetGame( pRoom );
	return ++(pGame->dwRoomIterateSequenceNumber);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float RoomGetDistanceSquaredToPoint(
	ROOM * pRoom,
	const VECTOR & vPosition)
{
	ASSERT_RETZERO(pRoom);

	const BOUNDING_BOX * pBox = RoomGetBoundingBoxWorldSpace(pRoom);

	return BoundingBoxDistanceSquared(pBox, &vPosition);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UseMultiMopp()
{
	return USE_MULTI_MOPP;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define PATH_NODE_EDGE_CONNECTION_TRAVERSED_BIT 31
BOOL PathNodeEdgeConnectionTestTraversed(
	const PATH_NODE_EDGE_CONNECTION * pConnection)
{
	return TESTBIT(pConnection->nEdgeIndex, PATH_NODE_EDGE_CONNECTION_TRAVERSED_BIT);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathNodeEdgeConnectionSetTraversed(
	PATH_NODE_EDGE_CONNECTION * pConnection,
	BOOL bTraversed)
{
	DWORD dwEdgeIndex = (DWORD)pConnection->nEdgeIndex;
	SETBIT(dwEdgeIndex, PATH_NODE_EDGE_CONNECTION_TRAVERSED_BIT, bTraversed);
	pConnection->nEdgeIndex = (int)dwEdgeIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int PathNodeEdgeConnectionGetEdgeIndex(
	const PATH_NODE_EDGE_CONNECTION * pConnection)
{
	DWORD dwEdgeIndex = (DWORD)pConnection->nEdgeIndex;
	CLEARBIT(dwEdgeIndex, PATH_NODE_EDGE_CONNECTION_TRAVERSED_BIT);
	return (int)dwEdgeIndex;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathNodeEdgeConnectionSetEdgeIndex(
	PATH_NODE_EDGE_CONNECTION * pConnection,
	int nEdgeIndex)
{
	BOOL bTraversed = PathNodeEdgeConnectionTestTraversed(pConnection);
	pConnection->nEdgeIndex = nEdgeIndex;
	PathNodeEdgeConnectionSetTraversed(pConnection, bTraversed);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RoomAllowsResurrect(
	ROOM *pRoom)
{
	ASSERTX_RETFALSE( pRoom, "Expected room" );
	SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
	ASSERTX_RETFALSE( pSubLevel, "Expected sublevel" );
	return SubLevelTestStatus( pSubLevel, SLS_RESURRECT_RESTRICTED_BIT ) == FALSE;
}



BOOL sRoomFindValidPathNodeForSpawning( ROOM *pRoom,
									    FREE_PATH_NODE &PathNode )
{
	GAME *pGame = RoomGetGame( pRoom );
	int nTries = 0;
	VECTOR vPosition( 0, 0, 0 );
	float fRoomX = pRoom->pHull->aabb.center.fX - pRoom->pHull->aabb.halfwidth.fX;
	float fRoomY = pRoom->pHull->aabb.center.fY - pRoom->pHull->aabb.halfwidth.fY;
	float fRoomXLength = pRoom->pHull->aabb.halfwidth.fX * 2;
	float fRoomYLength = pRoom->pHull->aabb.halfwidth.fY * 2;
	while( nTries < 50 )
	{

		vPosition.fX += ( RandGetFloat(pGame) * fRoomXLength) + fRoomX;
		vPosition.fY += ( RandGetFloat(pGame) * fRoomYLength) + fRoomY;

		// still in room?
		if ( !ConvexHullPointTest( pRoom->pHull, &vPosition ) )
			continue;

		FREE_PATH_NODE tFreePathNodes[2];
		NEAREST_NODE_SPEC tSpec;
		tSpec.fMaxDistance = MAX( fRoomXLength, fRoomYLength );
		SETBIT(tSpec.dwFlags, NPN_CHECK_DISTANCE);
		SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT);
		SETBIT(tSpec.dwFlags, NPN_DONT_CHECK_RADIUS);
		SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
		SETBIT(tSpec.dwFlags, NPN_USE_XY_DISTANCE);
		SETBIT(tSpec.dwFlags, NPN_ONE_ROOM_ONLY);
		SETBIT(tSpec.dwFlags, NPN_USE_GIVEN_ROOM);
		int nCount = RoomGetNearestPathNodes(pGame, pRoom, vPosition, 1, tFreePathNodes, &tSpec);
		if(nCount == 1 && pRoom)
		{
			const FREE_PATH_NODE * freePathNode = &tFreePathNodes[0];
			if( freePathNode )
			{
				PathNode = *freePathNode;
				return TRUE;
			}
			else
			{
				continue;
			}
		}
		else
		{
			continue;
		}

	}

	ASSERT_RETFAIL( nTries < 50 )

	return TRUE;
}
//just creates whatever the spawn class tell it to in a random position in the level
int s_RoomCreateSpawnClass( ROOM *pRoom,
							int nSpawnClass,
							int nCount )
{	
	ASSERT_RETZERO( nSpawnClass != INVALID_ID );
	ASSERT_RETZERO( pRoom );
	GAME *pGame = RoomGetGame( pRoom );
	ASSERT_RETZERO( pGame );
	LEVEL* pLevel = RoomGetLevel( pRoom );
	ASSERT_RETZERO( pLevel );
	nCount = MAX( 1, nCount );
	int nCreated( 0 );
	int nMonsterExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ];
	while( nCount > 0 )
	{		
		int nUnitsToCreate = SpawnClassEvaluate( pGame, LevelGetID( pLevel ), nSpawnClass, nMonsterExperienceLevel, tSpawns ); 
		for( int nUnitIndex = 0; nUnitIndex < nUnitsToCreate; nUnitIndex++ )
		{
			FREE_PATH_NODE pathNode;
			UNIT * pSpawnedUnit( NULL );
			if( sRoomFindValidPathNodeForSpawning( pRoom, pathNode ) <= 0 ) //we didn't find a pathnode			
				continue;			
			switch( tSpawns[nUnitIndex].eEntryType )
			{
			case SPAWN_ENTRY_TYPE_MONSTER:
				{
					MONSTER_SPEC tSpec;
					tSpec.nClass = tSpawns[nUnitIndex].nIndex;
					tSpec.nExperienceLevel = nMonsterExperienceLevel;
					tSpec.pRoom = pRoom;
					tSpec.vPosition = RoomPathNodeGetWorldPosition(pGame, pathNode.pRoom, pathNode.pNode);				
					tSpec.vPosition.fZ = VerifyFloat( tSpec.vPosition.fZ, 0.1f );									
					float fAngle = TWOxPI * RandGetFloat(pGame);			
					VectorDirectionFromAngleRadians( tSpec.vDirection, fAngle );
					pSpawnedUnit = s_MonsterSpawn( pGame, tSpec );
				}
				break;
			case SPAWN_ENTRY_TYPE_OBJECT:
				{
					OBJECT_SPEC objectSpec;
					objectSpec.nLevel = nMonsterExperienceLevel;
					objectSpec.nClass = tSpawns[nUnitIndex].nIndex;
					objectSpec.pRoom = pRoom;
					objectSpec.vPosition = RoomPathNodeGetWorldPosition(pGame, pathNode.pRoom, pathNode.pNode);				
					objectSpec.vPosition.fZ = VerifyFloat( objectSpec.vPosition.fZ, 0.1f );									
					float fAngle = TWOxPI * RandGetFloat(pGame);
					VECTOR vDirection;
					VectorDirectionFromAngleRadians( vDirection, fAngle );
					objectSpec.pvFaceDirection = &vDirection;
					
					pSpawnedUnit = s_ObjectSpawn( pGame, objectSpec ); //creating object
				}
				break;
			default:
				ASSERTX_CONTINUE( FALSE, "Unable to create UNIT inside spawn class" );
				break;
			}			

			if( pSpawnedUnit )
			{

				VECTOR Normal( 0, 0, -1 );
				VECTOR Start = UnitGetPosition(pSpawnedUnit);
				Start.fZ = pLevel->m_pLevelDefinition->fMaxPathingZ + 20;
				float fDist = ( pLevel->m_pLevelDefinition->fMaxPathingZ - pLevel->m_pLevelDefinition->fMinPathingZ ) + 30;
				VECTOR vCollisionNormal;
				int Material;
				float fCollideDistance = LevelLineCollideLen( pGame, pLevel, Start, Normal, fDist, Material, &vCollisionNormal);
				if (fCollideDistance < fDist &&
					Material != PROP_MATERIAL &&
					Start.fZ - fCollideDistance <= pLevel->m_pLevelDefinition->fMaxPathingZ && 
					Start.fZ - fCollideDistance >= pLevel->m_pLevelDefinition->fMinPathingZ )
				{

					Start.fZ = Start.fZ - fCollideDistance;
					UnitSetPosition( pSpawnedUnit, Start );
				}

				nCreated++;				
			}
			
			
		}
		nCount--;
	}
	return nCreated;
}
