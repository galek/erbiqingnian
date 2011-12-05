//----------------------------------------------------------------------------
// objects.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "c_townportal.h"
#include "console.h"
#include "prime.h"
#include "config.h"
#include "excel_private.h"
#include "game.h"
#include "globalindex.h"
#include "unit_priv.h"
#include "clients.h"
#include "objects.h"
#include "s_message.h"
#include "quests.h"
#include "teams.h"
#include "skills.h"
#include "player.h"
#include "ai.h"
#include "level.h"
#include "filepaths.h"
#include "combat.h"
#include "treasure.h"
#include "items.h"
#include "states.h"
#include "room.h"
#include "room_path.h"
#include "performance.h"
#include "dungeon.h"
#include "c_message.h"
#include "s_network.h"
#include "s_townportal.h"
#include "warp.h"
#include "waypoint.h"
#include "e_portal.h"
#include "c_ui.h"
#include "stringtable.h"
#include "c_quests.h"
#include "s_quests.h"
#include "gameunits.h"
#include "c_LevelAreasClient.h"
#include "faction.h"
#include "stash.h"
#include "movement.h"
#include "e_primitive.h"
#include "unitmodes.h"
#include "uix_graphic.h"
#include "c_LevelAreasClient.h"
#include "uix.h"
#include "LevelAreaLinker.h"
#include "stringreplacement.h"
#include "achievements.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelObjectsPostProcess( 
	EXCEL_TABLE * table)
{
	ExcelPostProcessUnitDataInit(table, TRUE);
	return TRUE;
}

//----------------------------------------------------------------------------
// KCK: Separated common part of path node blocking. This could probably use
// some more cleaning up
//----------------------------------------------------------------------------
static void sBlockNodeCommon(
			UNIT* pUnit,
			ROOM* pRoom)
{
	ASSERT_RETURN(pRoom);

	MATRIX matFinal;
	UnitGetWorldMatrix(pUnit, matFinal);

	float fRadius = UnitGetRawCollisionRadius(pUnit);
	float fRadiusX = UnitGetRawCollisionRadiusHorizontal(pUnit);
	float fHeight = UnitGetCollisionHeight(pUnit);
	BOUNDING_BOX tBoundingBox;
	tBoundingBox.vMin.fX = -fRadiusX;
	tBoundingBox.vMin.fY = -fRadius;
	tBoundingBox.vMin.fZ = 0.0f;
	tBoundingBox.vMax = tBoundingBox.vMin;

	{
		VECTOR vNew;
		vNew.fX = fRadiusX;
		vNew.fY = fRadius;
		vNew.fZ = fHeight;
		BoundingBoxExpandByPoint( &tBoundingBox, &vNew );
	}

	ORIENTED_BOUNDING_BOX pOrientedBox;
	TransformAABBToOBB(pOrientedBox, &matFinal, &tBoundingBox);
	// Get the planes
	PLANE pPlanes[6];
	MakeOBBPlanes(pPlanes, pOrientedBox);
	//int Blocked = 0;
	for(int i=0; i<pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].nPathNodeCount; i++)
	{
		ROOM_PATH_NODE * pPathNode = &pRoom->pPathDef->pPathNodeSets[pRoom->nPathLayoutSelected].pPathNodes[i];
		if (HashGet(pRoom->tUnitNodeMap, pPathNode->nIndex))
		{
			continue;
		}
		VECTOR vPos = RoomPathNodeGetExactWorldPosition(UnitGetGame(pUnit), pRoom, pPathNode);
		vPos.fZ += 0.25f;
		BOOL bIntersection = PointInOBBPlanes(pPlanes, vPos);
		if (bIntersection)
		{
			UNIT_NODE_MAP * pMap = HashAdd(pRoom->tUnitNodeMap, pPathNode->nIndex);
			pMap->bBlocked = TRUE;
			pMap->pUnit = pUnit;
			//Blocked++;
		}
	}
}


//----------------------------------------------------------------------------
// KCK: Was Tugboat only function, but now using for Hellgate. The only issue
// is that Hellgate does not use the room quadtree bits..
//----------------------------------------------------------------------------
void ObjectReserveBlockedPathNodes(
			UNIT* pUnit )
{
	if( AppIsHellgate() )
	{
		ROOM * pRoom = UnitGetRoom(pUnit);
		sBlockNodeCommon(pUnit, pRoom);
	}
	else //tugboat
	{
		LEVEL *pLevel = UnitGetLevel( pUnit );

		float Radius = UnitGetRawCollisionRadius( pUnit ) * 2;
		BOUNDING_BOX BBox;
		BBox.vMin = pUnit->vPosition;
		BBox.vMax = pUnit->vPosition;
		BBox.vMax.fX += Radius;
		BBox.vMax.fY += Radius;
		BBox.vMin.fX -= Radius;
		BBox.vMin.fY -= Radius;
		BBox.vMax.fZ += 5;
		BBox.vMin.fZ -= 5;

		SIMPLE_DYNAMIC_ARRAY<ROOM*> RoomsList;
		ArrayInit(RoomsList, GameGetMemPool(UnitGetGame(pUnit)), 2);
		pLevel->m_pQuadtree->GetItems( BBox, RoomsList );
		int nRoomCount = (int)RoomsList.Count();

		for( int i = 0; i < nRoomCount; i++ )
		{
			ROOM * pRoom = RoomsList[i];
			sBlockNodeCommon(pUnit, pRoom);
		}

		RoomsList.Destroy();
	}

	ObjectSetBlocking( pUnit );
}

//----------------------------------------------------------------------------
// KCK: Separated common part of path node clearing. This could probably use
// some more cleaning up
//----------------------------------------------------------------------------
static void sClearNodeCommon(
			UNIT* pUnit,
			ROOM* pRoom)
{
	ASSERT_RETURN(pRoom);

	for(UNIT_NODE_MAP * pMap = HashGetFirst(pRoom->tUnitNodeMap); pMap; )
	{
		UNIT_NODE_MAP * pMapNext = HashGetNext(pRoom->tUnitNodeMap, pMap);

		if( pMap->pUnit == pUnit)
		{
			HashRemove(pRoom->tUnitNodeMap, pMap->nId);
		}

		pMap = pMapNext;
	}
}

//----------------------------------------------------------------------------
// KCK: Was Tugboat only but now used by both, except the quadtree stuff
//----------------------------------------------------------------------------
void ObjectClearBlockedPathNodes(
			UNIT* pUnit )
{
	if( AppIsHellgate() )
	{
		ROOM * pRoom = UnitGetRoom(pUnit);
		sClearNodeCommon(pUnit, pRoom);
	}
	else //tugboat
	{
		LEVEL *pLevel = UnitGetLevel( pUnit );

		float Radius = max( UnitGetRawCollisionRadius( pUnit ) * 2, UnitGetRawCollisionRadiusHorizontal( pUnit ) * 2 );
		BOUNDING_BOX BBox;
		BBox.vMin = pUnit->vPosition;
		BBox.vMax = pUnit->vPosition;
		BBox.vMax.fX += Radius;
		BBox.vMax.fY += Radius;
		BBox.vMin.fX -= Radius;
		BBox.vMin.fY -= Radius;
		BBox.vMax.fZ += 5;
		BBox.vMin.fZ -= 5;

		SIMPLE_DYNAMIC_ARRAY<ROOM*> RoomsList;
		ArrayInit(RoomsList, GameGetMemPool(UnitGetGame(pUnit)), 2);
		pLevel->m_pQuadtree->GetItems( BBox, RoomsList );
		int nRoomCount = (int)RoomsList.Count();

		for( int i = 0; i < nRoomCount; i++ )
		{
			ROOM * pRoom = RoomsList[i];
			sClearNodeCommon(pUnit, pRoom);
		}
		RoomsList.Destroy();
	}

	ObjectClearBlocking( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sObjectBlockPathNodes(
	GAME* game,
	UNIT* pUnit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	ObjectReserveBlockedPathNodes( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sObjectClearBlockedPathNodes(
	GAME* game,
	UNIT* unit,
	UNIT* otherunit,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	if( AppIsHellgate() )
	{
		ROOM * pRoom = UnitGetRoom(unit);
		ASSERT_RETURN(pRoom);
		for(UNIT_NODE_MAP * pMap = HashGetFirst(pRoom->tUnitNodeMap); pMap; )
		{
			UNIT_NODE_MAP * pMapNext = HashGetNext(pRoom->tUnitNodeMap, pMap);
			if(pMap->bBlocked && pMap->pUnit == unit)
			{
				HashRemove(pRoom->tUnitNodeMap, pMap->nId);
			}
			pMap = pMapNext;
		}
		ObjectClearBlocking( unit );
	}
	else //tugboat
	{
		ObjectClearBlockedPathNodes( unit );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleFuseObjectEvent( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	UnitDie( pUnit, NULL );
	UnitFree( pUnit, UFF_SEND_TO_CLIENTS );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectSetFuse(
	UNIT *unit,
	int nTicks)
{		
	nTicks = MAX(1, nTicks);
	UnitRegisterEventTimed( unit, sHandleFuseObjectEvent, EVENT_DATA(), nTicks);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectUnFuse(
	UNIT * pObject)
{
	UnitUnregisterTimedEvent(pObject, sHandleFuseObjectEvent);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * ObjectInit(
	GAME * pGame,
	OBJECT_SPEC &tSpec)
{
	
	//ASSERT_RETNULL(pRoom || vPosition == INVALID_POSITION); // this is turned off because we are sending extra units to the client
	if ( IS_CLIENT( pGame ) )
	{
		if ( !tSpec.pRoom && tSpec.vPosition != INVALID_POSITION )
		{
			return NULL;
		}
	} 
	else 
	{
		ASSERT_RETNULL(tSpec.pRoom || tSpec.vPosition == INVALID_POSITION );
	}

	const UNIT_DATA * object_data = ObjectGetData(pGame, tSpec.nClass);
	if (!object_data)
	{
		return NULL;
	}

	int nExperienceLevel = tSpec.nLevel;

	if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_SET_OBJECT_TO_LEVEL_EXPERIENCE ))
	{
		if( tSpec.pRoom )
		{
			nExperienceLevel = RoomGetMonsterExperienceLevel( tSpec.pRoom );
			
		}
	}
	//nMinSpawnLevel & nMaxSpawnLevel should always be 0 to 10000 by default.
	if ( IS_SERVER( pGame ) )
	{
		if( nExperienceLevel < object_data->nMinSpawnLevel || 
			nExperienceLevel > object_data->nMaxSpawnLevel )		
		{
			if( TESTBIT(tSpec.dwFlags, OSF_IGNORE_SPAWN_LEVEL) == FALSE )
			{
				if( object_data->nObjectDownGradeToObject != INVALID_ID )
				{					
					tSpec.nClass = object_data->nObjectDownGradeToObject;
					return ObjectInit( pGame, tSpec );
				}
				return NULL;
			}
			else
			{
				//this could only happen with somebody spawning the object
				nExperienceLevel = (nExperienceLevel > object_data->nMaxSpawnLevel)?object_data->nMaxSpawnLevel:object_data->nMinSpawnLevel;
			}
		}
	}

	UnitDataLoad( pGame, DATATABLE_OBJECTS, tSpec.nClass );

	UNIT_CREATE newunit( object_data );
	newunit.species = MAKE_SPECIES(GENUS_OBJECT, tSpec.nClass);
	newunit.unitid = tSpec.id;
	newunit.pRoom = tSpec.pRoom;
	newunit.vPosition = tSpec.vPosition;
	newunit.guidOwner = tSpec.guidOwner;
	if (tSpec.pvFaceDirection)
	{
		newunit.vFaceDirection = *tSpec.pvFaceDirection;
	}
	if (tSpec.pvNormal)
	{
		newunit.vUpDirection = *tSpec.pvNormal;
	}	
	newunit.fScale = tSpec.flScale;
	SET_MASKV( newunit.dwUnitCreateFlags, UCF_NO_DATABASE_OPERATIONS, TESTBIT(tSpec.dwFlags, OSF_NO_DATABASE_OPERATIONS));
#if ISVERSION(DEBUG_ID)
	PStrPrintf(newunit.szDbgName, MAX_ID_DEBUG_NAME, "%s", object_data->szName);
#endif

	UNIT * pUnit = UnitInit(pGame, &newunit);
	if (!pUnit)
	{
		return NULL;
	}
	
	// targeting & collision
	{
		int nTeam = tSpec.nTeam;
		if (nTeam == INVALID_TEAM)
		{
			nTeam = UnitGetDefaultTeam(pGame, GENUS_OBJECT, tSpec.nClass, object_data);
		}
		TeamAddUnit(pUnit, nTeam);
	}

	UnitSetDefaultTargetType(pGame, pUnit, object_data);

	if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_TRIGGER) && object_data->nTriggerType > INVALID_LINK)
	{
		UnitSetFlag( pUnit, UNITFLAG_TRIGGER, TRUE );
		pUnit->tReservedLocation.vMin = tSpec.vPosition;
		float fWidth = object_data->fCollisionRadius;
		float fHeight = object_data->fCollisionRadius;
		// collision box
		OBJECT_TRIGGER_DATA * triggerdata = ( OBJECT_TRIGGER_DATA * )ExcelGetData( pGame, DATATABLE_OBJECTTRIGGERS, object_data->nTriggerType );
		if ( ( triggerdata->bIsWarp ) || ( triggerdata->bIsBlocking ) || ( object_data->fBlockingCollisionRadiusOverride > 0.0f ) )
		{
			// create perpendicular plane
			VECTOR vUp( 0.0f, 0.0f, 1.0f );
			VECTOR vPerp;
			VECTOR vDirection = UnitGetFaceDirection(pUnit, FALSE);
			if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_REVERSE_ARRIVE_DIRECTION))
			{
				vDirection.fX = -vDirection.fX;
				vDirection.fY = -vDirection.fY;
				vDirection.fZ = -vDirection.fZ;
			}			
			VectorCross( vPerp, vUp, vDirection );
			VECTOR vWarpPosition = vDirection;
			vWarpPosition *= object_data->fWarpPlaneForwardMultiplier;
			vWarpPosition += tSpec.vPosition;
			float fReservedRadius = object_data->fBlockingCollisionRadiusOverride > 0.0f ? object_data->fBlockingCollisionRadiusOverride : object_data->fCollisionRadius;
			vPerp *= fReservedRadius;
			pUnit->tReservedLocation.vMin.fX = vWarpPosition.fX + vPerp.fX;
			pUnit->tReservedLocation.vMin.fY = vWarpPosition.fY + vPerp.fY;
			pUnit->tReservedLocation.vMin.fZ = vWarpPosition.fZ - PLAYER_HEIGHT;
			pUnit->tReservedLocation.vMax.fX = vWarpPosition.fX - vPerp.fX;
			pUnit->tReservedLocation.vMax.fY = vWarpPosition.fY - vPerp.fY;
			pUnit->tReservedLocation.vMax.fZ = vWarpPosition.fZ + ( PLAYER_HEIGHT * 2.0f );
			// don't make the starting point for the game a real warp...
			if ( PStrICmp( object_data->szTriggerString1, "nowhere" ) == 0 )
			{
				UnitClearFlag( pUnit, UNITFLAG_TRIGGER );
			}
			// This does not work for multiplayer. Only works for single-player or solo-instance.
			if ( triggerdata->bIsBlocking )
			{
				ObjectSetBlocking( pUnit );
			}
		}
		else
		{
			pUnit->tReservedLocation.vMin.fX -= fWidth;
			pUnit->tReservedLocation.vMin.fY -= fHeight;
			pUnit->tReservedLocation.vMax = tSpec.vPosition;
			pUnit->tReservedLocation.vMax.fX += fWidth;
			pUnit->tReservedLocation.vMax.fY += fHeight;
			pUnit->tReservedLocation.vMax.fZ += PLAYER_HEIGHT * 2.0f;
		}
	}
#if !ISVERSION(SERVER_VERSION)
	if ( IS_CLIENT(pGame) && UnitDataTestFlag(object_data, UNIT_DATA_FLAG_ASK_QUESTS_FOR_VISIBLE))
	{
		c_QuestEventUnitVisibleTest( pUnit );
	}
#endif

	if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
	{
		UnitSetFlag( pUnit, UNITFLAG_DESTROY_DEAD_NEVER, TRUE );
	}

	if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_GIVES_LOOT))
	{
		UnitSetFlag( pUnit, UNITFLAG_GIVESLOOT, TRUE );
	}

	if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_DONT_TRIGGER_BY_PROXIMITY))
	{
		UnitSetFlag( pUnit, UNITFLAG_DONT_TRIGGER_BY_PROXIMITY, TRUE );
	}

	UnitSetExperienceLevel(pUnit, nExperienceLevel);

#if !ISVERSION(SERVER_VERSION)
	if( AppIsTugboat() )
	{
		if ( IS_CLIENT(pGame) )
		{
			c_UnitSetVisibilityOnClientByThemesAndGameEvents( pUnit );		
		}
	}
#endif
	for (int ii = 0; ii < NUM_UNIT_PROPERTIES; ii++)
	{
		ExcelEvalScript(pGame, pUnit, NULL, NULL, DATATABLE_OBJECTS, (DWORD)OFFSET(UNIT_DATA, codeProperties[ii]), tSpec.nClass);
	}

	if (object_data->fFuse > 0.0f)
	{
		int nTicks = (int)(object_data->fFuse * GAME_TICKS_PER_SECOND_FLOAT);
		nTicks = MAX(1, nTicks);
		UnitRegisterEventTimed( pUnit, sHandleFuseObjectEvent, &EVENT_DATA(), nTicks);
	}

	if (UnitIsA(pUnit, UNITTYPE_BLOCKING))
	{
		if( AppIsHellgate() )
		{
			MATRIX matFinal;		
			UnitGetWorldMatrix(pUnit, matFinal);
	
			float fRadius = UnitGetCollisionRadius(pUnit);
			float fHeight = UnitGetCollisionHeight(pUnit);
			BOUNDING_BOX tBoundingBox;
			tBoundingBox.vMin.fX = -fRadius;
			tBoundingBox.vMin.fY = -fRadius;
			tBoundingBox.vMin.fZ = 0.0f;
			tBoundingBox.vMax = tBoundingBox.vMin;
	
			{
				VECTOR vNew;
				vNew.fX = fRadius;
				vNew.fY = fRadius;
				vNew.fZ = fHeight;
				BoundingBoxExpandByPoint( &tBoundingBox, &vNew );
			}
	
			ORIENTED_BOUNDING_BOX pOrientedBox;
			TransformAABBToOBB(pOrientedBox, &matFinal, &tBoundingBox);
			// Get the planes
			PLANE pPlanes[6];
			MakeOBBPlanes(pPlanes, pOrientedBox);
			for(int i=0; i<tSpec.pRoom->pPathDef->pPathNodeSets[tSpec.pRoom->nPathLayoutSelected].nPathNodeCount; i++)
			{
				ROOM_PATH_NODE * pPathNode = &tSpec.pRoom->pPathDef->pPathNodeSets[tSpec.pRoom->nPathLayoutSelected].pPathNodes[i];
				if (HashGet(tSpec.pRoom->tUnitNodeMap, pPathNode->nIndex))
				{
					continue;
				}
				VECTOR vPos = RoomPathNodeGetExactWorldPosition(UnitGetGame(pUnit), tSpec.pRoom, pPathNode);
				vPos.fZ += 0.25f;
				BOOL bIntersection = PointInOBBPlanes(pPlanes, vPos);
				if (bIntersection)
				{
					UNIT_NODE_MAP * pMap = HashAdd(tSpec.pRoom->tUnitNodeMap, pPathNode->nIndex);
					pMap->bBlocked = TRUE;
					pMap->pUnit = pUnit;
				}
			}
			ObjectSetBlocking( pUnit );
		}
		else		//tugboat
		{
			ObjectReserveBlockedPathNodes( pUnit );
		}
		// some objects remain blocking when dead - onoly clear pathnodes on free.
		if( !UnitDataTestFlag(object_data, UNIT_DATA_FLAG_COLLIDE_WHEN_DEAD))
		{
			UnitRegisterEventHandler(pGame, pUnit, EVENT_UNITDIE_BEGIN, sObjectClearBlockedPathNodes, EVENT_DATA());
		}
		UnitRegisterEventHandler(pGame, pUnit, EVENT_ON_FREED, sObjectClearBlockedPathNodes, EVENT_DATA());
	}

	if ( AppIsHellgate() && UnitIsA( pUnit, UNITTYPE_TRAIN ) )
	{
		UnitSetStatFloat(pUnit, STATS_PATH_DISTANCE, 0, 0.0f );
		if ( IS_SERVER( pGame ) && ( UnitGetClass( pUnit ) == GlobalIndexGet( GI_OBJECT_BATTLETRAIN_ENGINE ) ) )
			s_StateSet( pUnit, pUnit, STATE_TRAIN_IDLE, 0 );
	}

	// init no draw on init if you want
	if (UnitDataTestFlag(object_data, UNIT_DATA_FLAG_NO_DRAW_ON_INIT))
	{
		c_UnitSetNoDraw(pUnit, TRUE, FALSE);
	}

	// set name if we have one
	if (tSpec.puszName != NULL)
	{
		UnitSetName( pUnit, tSpec.puszName );
	}

	// when creating objects that are sublevel doorways, we want to connect them to the sublevel
	if (IS_SERVER( pGame ))
	{
		if (ObjectIsSubLevelWarp( pUnit ))
		{
			s_SubLevelWarpCreated( pUnit );
		}

		if( pUnit->pUnitData->nUnitTypeRequiredToOperate != INVALID_ID )
		{
			//this might seem over kill but if we want to later setup puzzles in level area's this is the place to put it and 
			//set this stat to whatever the object is required to use.
			ObjectSetOperateByUnitType( pUnit, pUnit->pUnitData->nUnitTypeRequiredToOperate, 0 );
		}
	}
	
	UnitPostCreateDo(pGame, pUnit);

	return pUnit;
}

//----------------------------------------------------------------------------
// Sets that the unit cannot be operated except by this unitclass of an item
//----------------------------------------------------------------------------
void ObjectSetOperateByUnitClass( UNIT *pObject, int nUnitClass, int nItemIndex )
{
	ASSERT_RETURN( pObject );
	UnitSetStat( pObject, STATS_OBJECT_REQUIRES_ITEM_TO_OPERATE, nItemIndex, nUnitClass );
}
//----------------------------------------------------------------------------
// Sets that the unit cannot be operated except by this unittype of an item
//----------------------------------------------------------------------------
void ObjectSetOperateByUnitType( UNIT *pObject, int nUnitType, int nItemIndex )
{
	ASSERT_RETURN( pObject );
	UnitSetStat( pObject, STATS_OBJECT_REQUIRES_UNITTYPE_TO_OPERATE, nItemIndex, nUnitType );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * s_ObjectSpawn(
	GAME * pGame,
	OBJECT_SPEC &tSpec)
{
	ASSERT_RETNULL( IS_SERVER(pGame) );

	// use scale from unit data if none specified by spec
	const UNIT_DATA *pObjectData = ObjectGetData( pGame, tSpec.nClass );
	ASSERT_RETNULL(pObjectData);
	if (tSpec.flScale == 1.0f)
	{
		tSpec.flScale = pObjectData->fScale;
		if (pObjectData->fScaleDelta != 0.0f)
		{
			tSpec.flScale += RandGetFloat( pGame, 0.0f, pObjectData->fScaleDelta );
		}		
	}
	
	// create object
	UNIT * pUnit = ObjectInit( pGame, tSpec );
	if (!pUnit)
	{
		return NULL;
	}
	//could of changed do to downgrading of an object
	pObjectData = ObjectGetData( pGame, tSpec.nClass );

	UnitClearFlag(pUnit, UNITFLAG_JUSTCREATED);

	if ( IS_SERVER( pUnit ) )
	{
		AI_Init( pGame, pUnit );
	}

	// object is now constructed and ready for sending
	if (IS_SERVER( pGame ))
	{	
		ASSERTX( UnitIsInWorld( pUnit ), "Object is not in world" );
		UnitSetCanSendMessages( pUnit, TRUE );
		
		// send object to all clients who care
		ROOM *pRoom = UnitGetRoom( pUnit );		
		s_SendUnitToAllWithRoom( pGame, pUnit, pRoom );

	}
	
	if(IS_SERVER(pGame) && pObjectData->fNoSpawnRadius > 0.0f)
	{
		LEVEL * pLevel = RoomGetLevel(tSpec.pRoom);
		ASSERTV( 
			pObjectData->eRoomPopulatePass == RPP_SUBLEVEL_POPULATE, 
			"No spawn unit (%s) was *not* populated in '%s' during sublevel populate pass but should be so that it's placed early before other units are spawned",
			pObjectData->szName,
			LevelGetDevName( pLevel ));
		LevelAddSpawnFreeZone( pGame, pLevel, tSpec.vPosition, pObjectData->fNoSpawnRadius, TRUE );
	}

	// in open worlds, we need to respawn openables/harvestables after they die - on a delay, basically

	if( AppIsTugboat() )
	{
		LEVEL* pLevel = UnitGetLevel( pUnit );
		if( pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_OPENWORLD &&
			pObjectData->nRespawnPeriod != 0 )
		{
			UnitSetFlag( pUnit, UNITFLAG_SHOULD_RESPAWN );
		}
	}

	// start init skill
	for(int i=0; i<NUM_INIT_SKILLS; i++)
	{
		if (pObjectData->nInitSkill[i] != INVALID_LINK)
		{
			DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest( 
				pGame, 
				pUnit, 
				pObjectData->nInitSkill[i], 
				INVALID_ID, 
				cgvNone, 
				dwFlags, 
				SkillGetNextSkillSeed( pGame ));	
		}
	}

	return pUnit;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ObjectWarpGetDestinationLevelDepth(
	GAME * game,
	UNIT * trigger)
{
	ASSERTX_RETINVALID( AppIsTugboat(), "Tugboat only" );
	
	ASSERT_RETINVALID(game);
	ASSERT_RETINVALID(trigger);
	LEVEL * level_cur = UnitGetLevel(trigger);
	ASSERT_RETINVALID(level_cur);
	const LEVEL_DEFINITION * level_cur_data = LevelDefinitionGet(level_cur);
	ASSERT_RETINVALID(level_cur_data);
	const UNIT_DATA * trigger_data = ObjectGetData(game, UnitGetClass(trigger));
	ASSERT_RETINVALID(trigger_data);

	// get opposite level name
	const char * level_dest_name = NULL;
	if (PStrICmp(level_cur_data->pszName, trigger_data->szTriggerString1, DEFAULT_INDEX_SIZE) != 0)
	{
		level_dest_name = trigger_data->szTriggerString1;
	}
	else
	{
		level_dest_name = trigger_data->szTriggerString2;
	}
	
	// we are using the trigger name as a direction now.
	int Direction = atoi( level_dest_name );
	int nDepth = level_cur->m_nDepth + Direction;
	// find level instance ... note that this only finds the first level that matches
	// the definition.  if we want to be able to multiple instances on a single 
	// level definition this will have to change - colin
	//int nLevelIndex = LevelDefinitionGetRandom( game, nLevel );
	return nDepth;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ObjectWarpGetDestinationLevelArea(
	GAME * game,
	UNIT * trigger)
{
	ASSERT_RETINVALID(game);
	ASSERT_RETINVALID(trigger);
	LEVEL * level_cur = UnitGetLevel(trigger);
	ASSERT_RETINVALID(level_cur);
	const LEVEL_DEFINITION * level_cur_data = LevelDefinitionGet(level_cur);
	ASSERT_RETINVALID(level_cur_data);
	const UNIT_DATA * trigger_data = ObjectGetData(game, UnitGetClass(trigger));
	ASSERT_RETINVALID(trigger_data);

	// get opposite level name
	const char * level_dest_name = NULL;
	if (PStrICmp(level_cur_data->pszName, trigger_data->szTriggerString1, DEFAULT_INDEX_SIZE) != 0)
	{
		level_dest_name = trigger_data->szTriggerString1;
	}
	else
	{
		level_dest_name = trigger_data->szTriggerString2;
	}
	// we are using the trigger name as a direction now.
	int Direction = atoi( level_dest_name );
	int nDepth = level_cur->m_nDepth + Direction;
	if( nDepth < 0 )
	{
		return 1; // hacky! this is the 'town' level. lame. fixme.
	}
	else
	{
		return LevelGetLevelAreaID( level_cur );
	}
	
}

//----------------------------------------------------------------------------
// generic warp
//----------------------------------------------------------------------------
static BOOL sTriggerWarp(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pTrigger,
	const OBJECT_TRIGGER_DATA *pTriggerData)
{
	UNIT * pPlayer = ClientGetControlUnit( pClient );
	s_WarpEnterWarp( pPlayer, pTrigger );	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerDebug(
	GAME * game,
	GAMECLIENT * client,
	UNIT * trigger,
	const OBJECT_TRIGGER_DATA * triggerdata)
{
	ConsoleString( CONSOLE_SYSTEM_COLOR, "debug trigger" );
	return TRUE;
}


//returns TRUE if the item is the item needed to operate
BOOL ObjectItemIsRequiredToUse( UNIT *pObject,
								UNIT *pItem )
{
	ASSERT_RETFALSE( pObject );
	ASSERT_RETFALSE( pItem );
	int nUnitTypeNeeding = UnitGetStat( pObject, STATS_OBJECT_REQUIRES_UNITTYPE_TO_OPERATE, 0 );
	int nItemNeeding = UnitGetStat( pObject, STATS_OBJECT_REQUIRES_ITEM_TO_OPERATE, 0 );
	return ( ( nUnitTypeNeeding != INVALID_ID && UnitIsA( pItem, nUnitTypeNeeding ) ) ||
		( nItemNeeding != INVALID_ID && UnitGetClass( pItem ) == nItemNeeding ) );

}
//returns TRUE if the object needs another item to be triggered
BOOL ObjectRequiresItemToOperation( UNIT *pObject )
{
	ASSERT_RETTRUE( pObject );
	return UnitGetStat( pObject, STATS_OBJECT_REQUIRES_UNITTYPE_TO_OPERATE, 0 ) != INVALID_ID ||
		UnitGetStat( pObject, STATS_OBJECT_REQUIRES_ITEM_TO_OPERATE, 0 ) != INVALID_ID;
}

inline void sRemoveRequiredUnitTypesOnTrigger( UNIT *pPlayer,
											  UNIT *pObject,
											  GAMECLIENT * client )
{
	ASSERT_RETURN(pPlayer);
	if (!pObject)
	{
		return;
	}
	//check to see if the player has the item to operate the object - in the future maybe we'll want multiple items required: maybe not. ;)
	if( ObjectRequiresItemToOperation( pObject ) )
	{
		int nUnitTypeNeeding = UnitGetStat( pObject, STATS_OBJECT_REQUIRES_UNITTYPE_TO_OPERATE, 0 );
		int nItemNeeding = UnitGetStat( pObject, STATS_OBJECT_REQUIRES_ITEM_TO_OPERATE, 0 );
		DWORD dwInvIterateFlags = 0;
		SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );			
		// only search on person ( need to make this search party members too!		
		UNIT * pItem = NULL;	
		while ((pItem = UnitInventoryIterate( pPlayer, pItem, dwInvIterateFlags )) != NULL)
		{
			if ( ( nUnitTypeNeeding != INVALID_ID && UnitIsA( pItem, nUnitTypeNeeding ) ) ||
				( nItemNeeding != INVALID_ID && UnitGetClass( pItem ) == nItemNeeding ) )
			{
				//if the item is consumable when used...
				if ( UnitDataTestFlag(pItem->pUnitData, UNIT_DATA_FLAG_CONSUMED_WHEN_USED) )
				{				
					UNIT* pRemoved = UnitInventoryRemove( pItem, ITEM_SINGLE );
					ASSERT_RETURN( pRemoved );
					UnitFree( pRemoved, UFF_SEND_TO_CLIENTS );
				}
				return;
			}
		}		
	}		
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sWarpTriggerChangeLevel(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * player,
	UNIT * trigger,
	int new_area, 
	int old_level,
	int new_level)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(player);
	ASSERT_RETFALSE(trigger);
	ASSERT_RETFALSE(old_level != INVALID_LINK);
	ASSERT_RETFALSE(new_level != INVALID_LINK);

	// entering through some warps causes to remember the last level we were on
	// so that we die we know where we're supposed to return to ... we only want to
	// do this for what would be a "natural" warp, which is a warp that you would
	// naturally go through progressing forward in the game, we don't want to set it
	// for dynamic warps like townportals and hellrifts etc.
	if (ObjectIsDynamicWarp( trigger ) == FALSE)
	{	
		LEVEL *pLevel = UnitGetLevel(trigger);
		int old_level = LevelGetDefinitionIndex(pLevel);
		UnitSetStat(player, STATS_LEVELWARP_LAST, old_level);
	}

	// setup warp spec
	WARP_SPEC tSpec;	
	tSpec.nLevelAreaDest = new_area;
	tSpec.nLevelDepthDest = new_level;
	tSpec.nPVPWorld = ( PlayerIsInPVPWorld( player ) ? 1 : 0 );
	
	// warp!
	return s_WarpToLevelOrInstance( player, tSpec );
	
}

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerWarpRandomLocation(
							 GAME * game,
							 GAMECLIENT * client,
							 UNIT * trigger,
							 const OBJECT_TRIGGER_DATA * triggerdata)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(client);
	UNIT * pPlayer = ClientGetControlUnit(client);
	ASSERT_RETFALSE(pPlayer);
	LEVEL *pLevel = UnitGetLevel( trigger );
	ASSERT_RETFALSE(pLevel);



	UnitSetStat(pPlayer, STATS_TALKING_TO, UnitGetId( trigger ) );


	MSG_SCMD_OPERATE_WAYPOINT_MACHINE msg;
	s_SendMessage( game, ClientGetId( client ), SCMD_OPERATE_RUNEGATE, &msg );

	return TRUE;
	
}

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerWarpNext(
	GAME * game,
	GAMECLIENT * client,
	UNIT * trigger,
	const OBJECT_TRIGGER_DATA * triggerdata)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(client);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETFALSE(player);

	LEVEL *pLevel = UnitGetLevel( trigger );
	ASSERT_RETFALSE(pLevel);

	int nLevelAreaID = LevelGetLevelAreaID( pLevel );


	const MYTHOS_LEVELAREAS::LEVELAREA_LINK *linker = MYTHOS_LEVELAREAS::GetLevelAreaLinkByAreaID( nLevelAreaID );
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelAreaDest = linker->m_LevelAreaIDNext;
	tSpec.nLevelDepthDest = 0;
	tSpec.nPVPWorld = ( PlayerIsInPVPWorld( player ) ? 1 : 0 );

	MYTHOS_LEVELAREAS::ConfigureLevelAreaByLinker( tSpec, player, FALSE );
	UnitSetStat( player, STATS_LAST_LEVELID, LevelGetID( pLevel ) );

	// warp!
	return s_WarpToLevelOrInstance( player, tSpec );

}


//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerWarpPrevious(
	GAME * game,
	GAMECLIENT * client,
	UNIT * trigger,
	const OBJECT_TRIGGER_DATA * triggerdata)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(client);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETFALSE(player);
	LEVEL *pLevel = UnitGetLevel( trigger );
	ASSERT_RETFALSE(pLevel);

	int nLevelAreaID = LevelGetLevelAreaID( pLevel );

	const MYTHOS_LEVELAREAS::LEVELAREA_LINK *linker = MYTHOS_LEVELAREAS::GetLevelAreaLinkByAreaID( nLevelAreaID );
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelAreaDest = linker->m_LevelAreaIDPrevious;
	tSpec.nLevelDepthDest = 0;
	tSpec.nPVPWorld = ( PlayerIsInPVPWorld( player ) ? 1 : 0 );

	MYTHOS_LEVELAREAS::ConfigureLevelAreaByLinker( tSpec, player, FALSE );
	UnitSetStat( player, STATS_LAST_LEVELID, LevelGetID( pLevel ) );
	// warp!
	return s_WarpToLevelOrInstance( player, tSpec );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerUseSkill( 
	GAME *game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	DWORD dwFlags = 0;
	if ( IS_SERVER( game ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

	SkillStartRequest(game, trigger, trigger->pUnitData->nSkillWhenUsed, UnitGetId(ClientGetControlUnit(client)), VECTOR(0), 
		dwFlags, SkillGetNextSkillSeed(game));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCommonMarkerCollide(
	UNIT *pPlayer,
	UNIT *pTrigger)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	ASSERTX_RETFALSE( pTrigger, "Expected trigger" );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTX_RETFALSE( pGame && IS_SERVER( pGame ), "Server only" );
	
	// get last trigger class we collided with
	int nLastTriggerClass = UnitGetStat( pPlayer, STATS_LAST_TRIGGER );
	int nTriggerClass = UnitGetClass( pTrigger );
	if ( nLastTriggerClass == nTriggerClass )
	{
		return FALSE;  // don't continue to process the same marker over and over and over
	}

	// mark this trigger as the last one we hit
	UnitSetStat(pPlayer, STATS_LAST_TRIGGER, nTriggerClass );

	// do quest approach event
	s_QuestEventApproachPlayer( pPlayer, pTrigger );

	// display the pretty city level string thinger
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	MSG_SCMD_LEVELMARKER tMessage;
	tMessage.nObjectClass = UnitGetClass( pTrigger );
	s_SendMessage( pGame, ClientGetId( pClient ), SCMD_LEVELMARKER, &tMessage );
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerTownMarker( 
	GAME *pGame, 
	GAMECLIENT *pClient, 
	UNIT *pTrigger, 
	const OBJECT_TRIGGER_DATA *pTriggerData)
{

	// server only
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );

	// get the player
	UNIT *pPlayer = ClientGetControlUnit( pClient );
	if ( pPlayer == NULL )
	{
		return FALSE;
	}
		
	// do common marker collide logic
	if (sCommonMarkerCollide( pPlayer, pTrigger ) == FALSE)
	{
		return TRUE;
	}

	LEVEL *pLevel = UnitGetLevel( pPlayer );
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	int nLevelDefinition = LevelGetDefinitionIndex( pLevel );
	
	// set waypoint if we have one
	s_LevelSetWaypoint( pGame, pPlayer, pTrigger );

	// we will now start at this new location
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( nLevelDefinition );
	REF(ptLevelDef);
	if (AppIsTugboat() || ptLevelDef->bStartingLocation)
	{		
		UnitSetStat( pPlayer, STATS_LEVEL_DEF_START, UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT), nLevelDefinition );//This needs to be different on nightmare!
	}
	if (AppIsTugboat() || ptLevelDef->bPortalAndRecallLoc)
	{		
		UnitSetStat( pPlayer, STATS_LEVEL_DEF_RETURN, UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT), nLevelDefinition );//This needs to be different on nightmare!
	}

	if (AppIsHellgate())
	{
		// if this player has any recall portal out there, change it's destination
		UNITID idRecallPortal = UnitGetStat( pPlayer, STATS_WARP_RECALL_ENTER_UNITID );
		if (idRecallPortal != INVALID_ID)
		{
			UNIT *pRecallPortal = UnitGetById( pGame, idRecallPortal );
			if (pRecallPortal)
			{
				int nLevelDefDest = UnitGetReturnLevelDefinition( pPlayer );
				UnitSetStat( pRecallPortal, STATS_WARP_RECALL_ENTER_DEST_LEVEL_DEF, nLevelDefDest );				
			}
		}
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sTriggerTutorial( 
	GAME * game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}

	s_QuestEventApproachPlayer( unit, trigger );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sTriggerBlocker( 
	GAME * game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}

	// do nothing for now...
	return TRUE;
}

//----------------------------------------------------------------------------

static BOOL sTriggerQuestObject( 
	GAME *game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	//s_QuestEventObjectOperated( ClientGetControlUnit( client ), trigger, TRUE );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Note: Same as Marker, but doesn't save anything...
static BOOL sTriggerPOI( 
	GAME *pGame, 
	GAMECLIENT *pClient, 
	UNIT *pTrigger, 
	const OBJECT_TRIGGER_DATA *pTriggerData)
{
	ASSERTX_RETFALSE( pGame && IS_SERVER( pGame ), "Server only" );

	// get player
	UNIT *pPlayer = ClientGetControlUnit( pClient );
	if ( pPlayer == NULL )
	{
		return FALSE;
	}

	// do common marker logic
	sCommonMarkerCollide( pPlayer, pTrigger );

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerWaypointMachine( 
	GAME *game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	if( AppIsTugboat() )
	{
		UNIT *pPlayer = ClientGetControlUnit( client );
		if( pPlayer )
		{
			// mark the player as this is the last town visited
			
			//mark trigger as respawn location.
			LEVEL *pLevel = UnitGetLevel( pPlayer );
			LevelGetAnchorMarkers( pLevel )->SetAnchorVisited( pPlayer, trigger );	

			UnitSetStat(pPlayer, STATS_TALKING_TO, UnitGetId( trigger ) );
		}
	}
	

	MSG_SCMD_OPERATE_WAYPOINT_MACHINE msg;
	s_SendMessage( game, ClientGetId( client ), SCMD_OPERATE_WAYPOINT_MACHINE, &msg );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerWarpTown( 
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pTrigger,
	const OBJECT_TRIGGER_DATA *pTriggerData)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	ASSERTX_RETFALSE( pTrigger, "Expected trigger" );
	UNIT *pPlayer = ClientGetControlUnit( pClient );
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	UNITID idPortal = UnitGetId( pTrigger );		
	UNIT *pPortal = pTrigger;
	
	// don't do anything if we're set to not collide with town portals
	if (UnitTestFlag( pPlayer, UNITFLAG_DONT_COLLIDE_TOWN_PORTALS ))
	{
		return FALSE;
	}

	if (UnitIsA( pPortal, UNITTYPE_WARP_RECALL_ENTER ))
	{
	
		// you can only use your own recall portals
		PGUID guidPortalOwner = UnitGetGUIDOwner( pPortal );
		PGUID guidPlayer = UnitGetGuid( pPlayer );
		if (guidPortalOwner == guidPlayer)
		{
			s_LevelRecall( pGame, pPlayer );
			s_WarpCloseAll( pPlayer, WARP_TYPE_RECALL );
		}
		
	}
	else
	{

		// if can't use this portal, we don't care
		if (TownPortalCanUse( pPlayer, pPortal ) == FALSE)
		{
			return FALSE;
		}
		
		// set flag so we don't collide with this portal while we ask the client if its ok
		if( AppIsHellgate() )
		{
			UnitSetFlag( pPlayer, UNITFLAG_DONT_COLLIDE_TOWN_PORTALS );
		}

		// save this warp as the active warp
		UnitSetStat( pPlayer, STATS_ACTIVE_WARP, idPortal );

		// ask client if they want to use the portal
		if (s_TownPortalDisplaysMenu( pPlayer, pPortal ))
		{
			MSG_SCMD_ENTER_TOWN_PORTAL tMessage;
			tMessage.idPortal = idPortal;
			s_SendMessage( pGame, ClientGetId( pClient ), SCMD_ENTER_TOWN_PORTAL, &tMessage );
		}
		else
		{

			// don't bother asking and just shove em through	
			s_TownPortalEnter( pPlayer, idPortal );

		}

	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerWarpReturn( 
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pTrigger,
	const OBJECT_TRIGGER_DATA *pTriggerData)
{
	GAMECLIENTID idClient = ClientGetId( pClient );
	UNIT *pPlayer = ClientGetControlUnit( pClient );
	ASSERTX_RETFALSE( pPlayer, "Expected player" );

	// save this warp as the active warp
	UnitSetStat( pPlayer, STATS_ACTIVE_WARP, UnitGetId( pTrigger ) );
	
	// this unit will not any more triggers until a response is received back
	UnitAllowTriggers( pPlayer, FALSE, 0.0f );
	
	// in multiplayer, send a message to the client telling them to select their destination
	if (s_TownPortalDisplaysMenu( pPlayer, pTrigger ))
	{
		MSG_SCMD_SELECT_RETURN_DESTINATION tMessage;
		tMessage.idReturnWarp = UnitGetId( pTrigger );
		s_SendMessage( pGame, idClient, SCMD_SELECT_RETURN_DESTINATION, &tMessage );
	}
	else
	{
		// for single player just shove them through to their own portal
		TOWN_PORTAL_SPEC tTownPortal;
		s_TownPortalGetSpec( pPlayer, &tTownPortal );		
		s_TownPortalReturnThrough( pPlayer, UnitGetGuid(pPlayer), tTownPortal );		
		
	}
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_ObjectGetOppositeSubLevelPortal(
	UNIT *pPortal)
{
	ASSERTX_RETNULL( pPortal, "Expected unit" );
	ASSERTX_RETNULL( ObjectIsSubLevelWarp( pPortal ), "Expected sublevel portal" );

	// get this portals sublevel
	SUBLEVELID idSubLevelPortal = UnitGetSubLevelID( pPortal );

	// what is the opposite sub-level of this portal
	SUBLEVELID idSubLevelOpposite = s_ObjectGetOppositeSubLevel( pPortal );
	if (idSubLevelOpposite == INVALID_ID)
	{
		return NULL; // opposite sublevel is not here
	}
	SUBLEVEL *pSubLevelOpposite = SubLevelGetById( UnitGetLevel( pPortal ), idSubLevelOpposite );

	// which sublevel is not the default one
	SUBLEVELID idSubLevelNotDefault;
	if (idSubLevelPortal != DEFAULT_SUBLEVEL_ID)
	{
		idSubLevelNotDefault = idSubLevelPortal;
	}
	else
	{
		idSubLevelNotDefault = idSubLevelOpposite;
	}
	
	// get the not default sub level
	LEVEL *pLevel = UnitGetLevel( pPortal );
	SUBLEVEL *pSubLevelNotDefault = SubLevelGetById( pLevel, idSubLevelNotDefault );
	const SUBLEVEL_DEFINITION *ptSubLevelDefNotDefault = SubLevelGetDefinition( pSubLevelNotDefault );
	ASSERTX_RETNULL(ptSubLevelDefNotDefault, "Not default sublevel id is invalid");
	
	// is this portal the entrance or the exit object
	int nObjectClassOther = INVALID_LINK;
	if (ptSubLevelDefNotDefault->nObjectEntrance == UnitGetClass( pPortal ))
	{
		nObjectClassOther = ptSubLevelDefNotDefault->nObjectExit;
	}
	else if (ptSubLevelDefNotDefault->nObjectExit == UnitGetClass( pPortal ))
	{
		nObjectClassOther = ptSubLevelDefNotDefault->nObjectEntrance;
	}
	else if(ptSubLevelDefNotDefault->nAlternativeEntranceUnitType != INVALID_LINK && UnitIsA(pPortal, ptSubLevelDefNotDefault->nAlternativeEntranceUnitType))
	{
		nObjectClassOther = ptSubLevelDefNotDefault->nObjectExit;
	}
	else if(ptSubLevelDefNotDefault->nAlternativeExitUnitType != INVALID_LINK && UnitIsA(pPortal, ptSubLevelDefNotDefault->nAlternativeExitUnitType))
	{
		nObjectClassOther = ptSubLevelDefNotDefault->nObjectEntrance;
	}
	else
	{
		return NULL;  // this is allowed because we have two portals in the town portal level (townportalvisualexit and returnportalvisualexit)
	}

	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	return s_SubLevelFindFirstUnitOf( pSubLevelOpposite, eTargetTypes, GENUS_OBJECT, nObjectClassOther );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerWarpSubLevel(
	GAME *pGame, 
	GAMECLIENT *pClient,
	UNIT *pPortal,
	const OBJECT_TRIGGER_DATA *pTriggerData)
{
	ASSERT_RETFALSE( pGame && IS_SERVER( pGame ));
	ASSERT_RETFALSE( pClient );
	UNIT *pPlayer = ClientGetControlUnit( pClient );
	ASSERT_RETFALSE( pPlayer );



	// is this a one way sublevel warp ... if so, you cannot got into 
	// the 'to' portals, you are only allowed to enter bidirectional 
	// and 'from' portals
	const UNIT_DATA *pUnitData = ObjectGetData( pGame, pPortal );
	ASSERT_RETFALSE(pUnitData);
	VISUAL_PORTAL_DIRECTION eDir = pUnitData->eOneWayVisualPortalDirection;
	if (eDir == VPD_TO)
	{
		return FALSE;
	}

	// distance on other side of portal to come of
	const float OUT_OF_SUBLEVEL_PORTAL_DISTANCE = 2.5f;
	
	UNIT *pOppositePortal = s_ObjectGetOppositeSubLevelPortal( pPortal );
	ASSERTV_RETFALSE( 
		pOppositePortal, 
		"No opposite portal for sublevel warp (%s) [id=%d] on sublevel (%s)",
		UnitGetDevName( pPortal ),
		UnitGetId( pPortal ),
		SubLevelGetDevName( UnitGetSubLevel( pPortal ) ));

	if(AppIsHellgate() && (UnitIsInHellrift(pOppositePortal) || UnitIsInHellrift(pPortal)))
	{
		if(UnitHasState(pGame, pPlayer, STATE_NO_TRIGGER_WARPS))
			return FALSE;
		s_StateSet( pPlayer, pPlayer, STATE_NO_TRIGGER_WARPS, 0 );
	}



	// where is the portal we are walking into and which direction is it facing
	VECTOR vPortalPos = UnitGetPosition( pPortal );
	VECTOR vPortalDir = UnitGetFaceDirection( pPortal, FALSE );
	VectorNormalize( vPortalDir );
					
	// get face direction ... we are keeping the same face direction of the player
	// because you can see through these portals and walk through them always facing
	// the same direction
	VECTOR vFaceDirection = UnitGetFaceDirection( pOppositePortal, FALSE );
	VectorNormalize( vFaceDirection );

	// given the direction the player was looking, reverse it and find a location
	// that is "back a little ways" along that path
	VECTOR vRevTravelDir = UnitGetFaceDirection( pPlayer, FALSE );
	VectorNormalize( vRevTravelDir );
	VectorScale( vRevTravelDir, vRevTravelDir, -9999.9f );  // extend and reverse
	VECTOR vPosBack = vRevTravelDir + UnitGetPosition( pPlayer );
	
	// is the position back along the players path in front of the portal?
	BOOL bBehind = FALSE;
	if (VectorPosIsInFrontOf( vPosBack, vPortalPos, vPortalDir ) == FALSE)
	{
		vFaceDirection = -vFaceDirection;  // OK, we'll come out the other way at the destination
		bBehind = TRUE;
//		ConsoleString( "[%d] Entering portal from behind", GameGetTick( pGame ));
	}

	// find the right vector of the portal we're entering
	VECTOR vRight;
	VectorCross( vRight, vPortalDir, cgvZAxis );
	VectorNormalize( vRight );

	// get vector to the back pos we're using
	VECTOR vToBackPos = vPosBack - vPortalPos;
	VectorNormalize( vToBackPos );
		
	// what's our angle of entry into the portal
	float flAngleOfEntry = VectorDot( vRight, vToBackPos );

	// what is our exit angle
	float flAngleOfExit = bBehind ? flAngleOfEntry : -flAngleOfEntry;
	
	// alter the direction we're facing on the other side of the portal by the
	// angle we're entering the portal on this side from
	VectorZRotation( vFaceDirection, flAngleOfExit );
	
	// get position we will appear at by the opposite portal
	VECTOR vPosition = vFaceDirection * OUT_OF_SUBLEVEL_PORTAL_DISTANCE;
	vPosition += UnitGetPosition( pOppositePortal );

	// find the nearest path node to this location
	ROOM *pRoomOppositePortal = UnitGetRoom( pOppositePortal );
	ROOM *pRoomNearestPosition = NULL;
	VECTOR vNearestPos = RoomGetNearestFreePosition( pRoomOppositePortal, vPosition, &pRoomNearestPosition);
	
	// warp to that location on the same level
	DWORD dwUnitWarpFlags = 0;
	SETBIT(dwUnitWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp( 
		pPlayer, 
		pRoomNearestPosition, 
		vNearestPos, 
		vFaceDirection, 
		cgvZAxis, 
		dwUnitWarpFlags);
		
	return TRUE;
	
}

//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerWarpOne(
	GAME * game,
	GAMECLIENT * client,
	UNIT * trigger,
	const OBJECT_TRIGGER_DATA * triggerdata)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(client);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETFALSE(player);
	const UNIT_DATA * trigger_data = ObjectGetData(game, UnitGetClass(trigger));
	ASSERT_RETINVALID(trigger_data);

	// save def index ( where I am coming from )
	LEVEL * old_level = UnitGetLevel(player);
	int old_level_index = LevelGetDefinitionIndex( old_level );
	UnitSetStat( player, STATS_WARP_ONE, old_level_index );

	// get level to go to
	const char * level_dest_name = trigger_data->szTriggerString1;
	ASSERT_RETFALSE(level_dest_name && PStrLen(level_dest_name, DEFAULT_INDEX_SIZE) > 0);

	// get def index
	int level_dest = LevelGetDefinitionIndex(level_dest_name);
	ASSERT_RETFALSE(level_dest != INVALID_LINK);
	// broken - bad area
	return sWarpTriggerChangeLevel( game, client, player, trigger, 0, old_level_index, level_dest );
}


//----------------------------------------------------------------------------
//Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerWarpReturnOne(
	GAME * game,
	GAMECLIENT * client,
	UNIT * trigger,
	const OBJECT_TRIGGER_DATA * triggerdata)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(client);
	UNIT * player = ClientGetControlUnit(client);
	ASSERT_RETFALSE(player);
	const UNIT_DATA * trigger_data = ObjectGetData(game, UnitGetClass(trigger));
	ASSERT_RETFALSE(trigger_data);

	// where I am now
	LEVEL * old_level = UnitGetLevel(player);
	int old_level_index = LevelGetDefinitionIndex( old_level );

	// get level to return to
	int new_level_index = UnitGetStat( player, STATS_WARP_ONE );
	ASSERT_RETFALSE( new_level_index != INVALID_ID );
	// broken - bad area
	return sWarpTriggerChangeLevel( game, client, player, trigger, 0, old_level_index, new_level_index );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerWarpPartyMember(
	GAME *pGame,
	GAMECLIENT *pClient,
	UNIT *pTrigger,
	const OBJECT_TRIGGER_DATA *pTriggerData)
{
	ASSERTX_RETFALSE( pGame && IS_SERVER( pGame ), "Server only" );
	ASSERTX_RETFALSE( pClient, "Expected client" );
	ASSERTX_RETFALSE( pTrigger, "Expected trigger" );
	
	// get player triggering the warp
	UNIT *pPlayer = ClientGetControlUnit( pClient );
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	
	// enter the warp
	return s_WarpPartyMemberEnter( pPlayer, pTrigger );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerStash( 
	GAME * game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}

	s_StashOpen( unit, trigger );
	return TRUE;
	
}

//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerWorldMap( 
	GAME * game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}

	s_PlayerToggleUIElement(unit, UIE_WORLDMAP, UIEA_OPEN);

	return TRUE;
}


//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerPVPWorld( 
							 GAME * game, 
							 GAMECLIENT * client, 
							 UNIT * trigger, 
							 const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));
#if !ISVERSION(CLIENT_ONLY_VERSION)

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}
	// PVP only players can't use the portals.
	if( PlayerIsPVPOnly( unit ) )
	{
		return FALSE;
	}
	if( PlayerIsInPVPWorld( unit ) )
	{
		s_StateClear( unit, UnitGetId( unit ), STATE_PVPWORLD, 0 );
	}
	else
	{
		s_StateSet( unit, unit, STATE_PVPWORLD, 0 );
	}


	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.dwWarpFlags = 0;
	tSpec.nLevelDepthDest = UnitGetLevelDepth( unit );	
	tSpec.nLevelAreaDest = UnitGetLevelAreaID( unit );
	tSpec.nLevelAreaPrev = UnitGetLevelAreaID( unit );
	MYTHOS_LEVELAREAS::ConfigureLevelAreaByLinker( tSpec, unit, TRUE );
	tSpec.nPVPWorld = PlayerIsInPVPWorld( unit );

	if( s_TownPortalGet( unit ) )
	{
		s_TownPortalsClose( unit );
	}

	// do the warp
	s_WarpToLevelOrInstance( unit, tSpec );
#endif
	
	return TRUE;
}
//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerMarkAnchorstone( 
									GAME * game, 
									GAMECLIENT * client, 
									UNIT * trigger, 
									const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));
#if !ISVERSION(CLIENT_ONLY_VERSION)

	// mark the player as this is the last town visited
	UNIT * pPlayer = ClientGetControlUnit( client );
	if ( !pPlayer )
	{
		return FALSE;
	}
	//mark trigger as respawn location.
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	LevelGetAnchorMarkers( pLevel )->SetAnchorAsRespawnLocation( pPlayer, trigger );	
#endif

	return TRUE;
}
//----------------------------------------------------------------------------
// Tugboat
//----------------------------------------------------------------------------
static BOOL sTriggerAnchorstone( 
							 GAME * game, 
							 GAMECLIENT * client, 
							 UNIT * trigger, 
							 const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));
#if !ISVERSION(CLIENT_ONLY_VERSION)

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}
	DWORD dwLevelRecallFlags = 0;
	SETBIT( dwLevelRecallFlags, LRF_FORCE, TRUE );
	SETBIT( dwLevelRecallFlags, LRF_PRIMARY_ONLY, TRUE);
	s_LevelRecall( game, unit, dwLevelRecallFlags );
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sTriggerFreeObject(
	GAME* pGame, 
	UNIT* unit, 
	const EVENT_DATA& event_data)
{
	UNITID idToFree = (UNITID)event_data.m_Data1;
	UNIT * pUnitToFree = UnitGetById(pGame, idToFree);
	UnitFree(pUnitToFree);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sTriggerFreeOnTriggerer( 
	GAME * game, 
	GAMECLIENT * client, 
	UNIT * trigger, 
	const OBJECT_TRIGGER_DATA * triggerdata )
{
	ASSERT_RETFALSE(IS_SERVER(game));

	// mark the player as this is the last town visited
	UNIT * unit = ClientGetControlUnit( client );
	if ( !unit )
	{
		return FALSE;
	}

	UNITID idTriggerUnit = UnitGetStat(trigger, STATS_TRIGGER_UNIT_ID);
	if(UnitGetId(unit) == idTriggerUnit)
	{
		UnitRegisterEventTimed(trigger, sTriggerFreeObject, &EVENT_DATA(UnitGetId(trigger)), 1);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
struct TRIGGER_LOOKUP
{
	const char *pszName;
	PFN_TRIGGER_FUNCTION pfnTrigger;
};

static TRIGGER_LOOKUP sgTriggerLookupTable[] =
{
	{ "Warp",				sTriggerWarp },
	{ "Debug",				sTriggerDebug },
	{ "Openable",			sTriggerUseSkill },
	{ "TriggerUseSkill",	sTriggerUseSkill },
	{ "TownMarker",			sTriggerTownMarker },
	{ "Tutorial",			sTriggerTutorial },
	{ "Blocker",			sTriggerBlocker	},
	{ "QuestObject",		sTriggerQuestObject	},
	{ "PointOfInterest",	sTriggerPOI	},
	{ "WarpNext",			sTriggerWarpNext },
	{ "WarpPrevious",		sTriggerWarpPrevious },
	{ "WaypointMachine",	sTriggerWaypointMachine },
	{ "WarpTown",			sTriggerWarpTown },
	{ "WarpReturn",			sTriggerWarpReturn },
	{ "WarpSubLevel",		sTriggerWarpSubLevel },
	{ "WarpOne",			sTriggerWarpOne },
	{ "WarpReturnOne",		sTriggerWarpReturnOne },
	{ "WarpPartyMember",	sTriggerWarpPartyMember },	
	{ "Door",				sTriggerUseSkill	},				// door
	{ "MapTrigger",			sTriggerWorldMap	},			
	{ "OpenStash",			sTriggerStash},
	{ "FreeOnTriggerer",	sTriggerFreeOnTriggerer},
	{ "PVPWorldTrigger",	sTriggerPVPWorld	},	
	{ "AnchorstoneTrigger",	sTriggerAnchorstone	},
	{ "AnchorstoneMark",	sTriggerMarkAnchorstone },
	{ "warp_undefined",		sTriggerWarpRandomLocation },
};

int sgnTriggerLookupEntries = arrsize( sgTriggerLookupTable );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_TRIGGER_FUNCTION sLookupTriggerFunction(
	const char *pszName)
{
	ASSERTX_RETNULL( pszName, "Expected name param" );
	
	if (PStrLen( pszName ))
	{
		for (int i = 0; i < sgnTriggerLookupEntries; ++i)
		{
			const TRIGGER_LOOKUP *pLookup = &sgTriggerLookupTable[ i ];
			if (PStrCmp( pLookup->pszName, pszName, MAX_FUNCTION_STRING_LENGTH ) == 0)
			{
				return pLookup->pfnTrigger;
			}
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelObjectTriggersPostProcess( 
	EXCEL_TABLE * table)
{
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		OBJECT_TRIGGER_DATA * trigger_data = (OBJECT_TRIGGER_DATA *)ExcelGetDataPrivate(table, ii);		
		ASSERT_RETFALSE(trigger_data);

		if (!trigger_data->szTriggerFunction || trigger_data->szTriggerFunction[0] == 0)
		{
			continue;
		}
		trigger_data->pfnTrigger = sLookupTriggerFunction(trigger_data->szTriggerFunction);			
		if (trigger_data->pfnTrigger == NULL)
		{
			trace("ERROR: Row [%d] - Unknown trigger function '%s' in '%s'", ii, trigger_data->szTriggerFunction, table->m_Name);
		}
	}
	
	return TRUE;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
OPERATE_RESULT ObjectCanOperate(
	UNIT *pOperator,
	UNIT *pObject)
{
	ASSERTX_RETVAL( pOperator, OPERATE_RESULT_FORBIDDEN, "Expected operator unit" );
	ASSERTX_RETVAL( pObject, OPERATE_RESULT_FORBIDDEN, "Expected object unit" );
	ASSERTX_RETVAL( UnitIsA( pObject, UNITTYPE_OBJECT ), OPERATE_RESULT_FORBIDDEN, "Expected object" );	
	OPERATE_RESULT eOperateResult = OPERATE_RESULT_OK;
	GAME *pGame = UnitGetGame( pObject );	
	const UNIT_DATA *pObjectData = ObjectGetData( pGame, pObject );
	ASSERTX_RETVAL( pObjectData, OPERATE_RESULT_FORBIDDEN, "Expected object data" );

	// warp debug
	if (ObjectIsWarp( pObject ) && GameGetDebugFlag( pGame, DEBUGFLAG_OPEN_WARPS ))
	{
		return OPERATE_RESULT_OK;
	}

	// warps that are unavailable
	if (ObjectIsWarp( pObject ))
	{
		if (WarpDestinationIsAvailable( pOperator, pObject ) != WRR_OK)
		{
			return OPERATE_RESULT_FORBIDDEN;
		}		
	}
	
	if ( AppIsHellgate() && UnitHasState( pGame, pObject, STATE_NPC_TRIAGE ) )
	{
		return OPERATE_RESULT_OK;
	}

	// check for prohibited states on the operator
	for (int i = 0; i < MAX_TRIGGER_PROHIBITED_STATES; ++i)
	{
		int nState = pObjectData->nOperatorStatesTriggerProhibited[ i ];
		if (nState != INVALID_LINK && UnitHasState( pGame, pOperator, nState ) == TRUE)
		{
			return OPERATE_RESULT_FORBIDDEN;
		}
	}

	if (UnitIsPlayer( pOperator ))
	{
		if (UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_ASK_PVP_CENSORSHIP_FOR_OPERATE))
		{
			if (AppTestFlag(AF_CENSOR_NO_PVP))
				return OPERATE_RESULT_FORBIDDEN;
		}

		// some objects require some quest states to be set
		int nQuest = pObjectData->nQuestOperateRequired;
		int nQuestState = pObjectData->nQuestStateOperateRequired;
		int nQuestStateValueRequired = pObjectData->nQuestStateValueOperateRequired;
		int nQuestStateValueProhibited = pObjectData->nQuestStateValueOperateProhibited;
		
		// check quest and state value (if present)
		if (nQuest != INVALID_LINK )
		{

			// get quest
			QUEST *pQuest = QuestGetQuest( pOperator, pObjectData->nQuestOperateRequired );
			if (pQuest == NULL)
			{
				return OPERATE_RESULT_FORBIDDEN;
			}
		
			// check for required quest status
			if (UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_OPERATE_REQUIRES_GOOD_QUEST_STATUS))
			{
				if (QuestStatusIsGood( pQuest ) == FALSE)
				{
					return OPERATE_RESULT_FORBIDDEN;
				}
			}
			
			if ( ( nQuestState != INVALID_LINK ) && QuestIsActive( pQuest ) )
			{
				if (nQuestStateValueRequired != INVALID_LINK)
				{
					// must have reqired state
					if (QuestStateGetValue( pQuest, nQuestState ) != nQuestStateValueRequired )
					{
						return OPERATE_RESULT_FORBIDDEN;
					}
				}
				if (nQuestStateValueProhibited != INVALID_LINK)
				{
					// must not have prohibited state
					if (QuestStateGetValue( pQuest, nQuestState ) == nQuestStateValueProhibited )
					{
						return OPERATE_RESULT_FORBIDDEN;
					}				
				}
			}		
		}
	}
				
	// see if we're supposed to ask the quest system if we can be triggered
	if (UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_ASK_QUESTS_FOR_OPERATE))
	{
		eOperateResult = QuestsCanOperateObject( pOperator, pObject );
		if (eOperateResult != OPERATE_RESULT_OK)
		{
			return eOperateResult;
		}
	}
	
	// ask quests
	if (UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_ASK_FACTION_FOR_OPERATE))
	{
		eOperateResult = FactionCanOperateObject( pOperator, pObject );
		if (eOperateResult != OPERATE_RESULT_OK)
		{
			return eOperateResult;
		}
	}

	// check trigger type	
	const OBJECT_TRIGGER_DATA *pTriggerData = ObjectTriggerTryToGetData( pGame, pObjectData->nTriggerType );
	if (pTriggerData)
	{
	
		// some triggers do not allow dying/dead things to operate them (like all of them)
		if (pTriggerData->bDeadCanTrigger == FALSE && IsUnitDeadOrDying( pOperator ))
		{
			eOperateResult = OPERATE_RESULT_FORBIDDEN;
		}
		
		// ghosts - NOTE: do *NOT* include hardcore dead as part of the ghost check
		// because object triggers explicitly support that check as a separate question
		if (pTriggerData->bGhostCanTrigger == FALSE && UnitIsGhost( pOperator, FALSE ))
		{
			eOperateResult = OPERATE_RESULT_FORBIDDEN;
		}
		
		// check for hardcore dead (separate from ghost for this case)
		if (UnitIsPlayer( pOperator ))
		{
			if (pTriggerData->bHardcoreDeadCanTrigger == FALSE && PlayerIsHardcoreDead( pOperator ))
			{
				eOperateResult = OPERATE_RESULT_FORBIDDEN;
			}
		}
		
		if (eOperateResult != OPERATE_RESULT_OK)
		{
			return eOperateResult;
		}
		
	}

	//check to see if the player has the item to operate the object - in the future maybe we'll want multiple items required: maybe not. ;)
	if( !pTriggerData->bObjectRequiredCheckIgnored &&
		ObjectRequiresItemToOperation( pObject ) )
	{
		DWORD dwInvIterateFlags = 0;
		SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );			
		// only search on person ( need to make this search party members too!		
		UNIT * pItem = NULL;			
		while ((pItem = UnitInventoryIterate( pOperator, pItem, dwInvIterateFlags )) != NULL)
		{
			if ( ObjectItemIsRequiredToUse( pObject, pItem ) )
			{				
				return eOperateResult;
			}
		}		
		return OPERATE_RESULT_FORBIDDEN;
	}	
	
	return eOperateResult;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
OPERATE_RESULT ObjectCanTrigger(
	UNIT *pUnit,
	UNIT *pObject)
{
	ASSERTX_RETVAL( pUnit, OPERATE_RESULT_FORBIDDEN, "Expected unit" );
	ASSERTX_RETVAL( pObject, OPERATE_RESULT_FORBIDDEN, "Expected object unit" );
	ASSERTX_RETVAL( UnitIsA( pObject, UNITTYPE_OBJECT ), OPERATE_RESULT_FORBIDDEN, "Expected object" );	
			
	// check unit flag for triggers
	if ( !UnitTestFlag( pObject, UNITFLAG_TRIGGER ) )
	{
		return OPERATE_RESULT_FORBIDDEN;
	}

	// now just ask the regular "operate" question
	return ObjectCanOperate( pUnit, pObject );	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectTrigger(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	UNIT * object )
{
	ASSERTX_RETURN( game, "Expected game" );
	ASSERTX_RETURN( client, "Expected client" );
	ASSERTX_RETURN( unit, "Expected unit" );
	ASSERTX_RETURN( object, "Expected object" );
	
	// must be able to trigger
	if (ObjectCanTrigger( unit, object ) != OPERATE_RESULT_OK)
	{
		// if it requires an item and we can't operate it, is it locked?
		if( AppIsTugboat() &&
			unit &&
			object &&
			ObjectRequiresItemToOperation( object )  )
		{
			// Tell the client to play a 'locked' complaint
			const UNIT_DATA* unit_data = UnitGetData( unit );
			if (unit_data->m_nLockedSound != INVALID_LINK)
			{
				MSG_SCMD_UNITPLAYSOUND msg;
				msg.id = UnitGetId(unit);
				msg.idSound = unit_data->m_nLockedSound;
				s_SendMessage(game, UnitGetClientId(unit), SCMD_UNITPLAYSOUND, &msg);
			}
		}
		return;
	}
		
	// run trigger
	const UNIT_DATA *pObjectData = ObjectGetData( game, object );
	ASSERTX_RETURN( pObjectData, "Expected object data" );
	BOOL bTriggered = TRUE;
	const OBJECT_TRIGGER_DATA *triggerdata = ObjectTriggerTryToGetData( game, pObjectData->nTriggerType );
	ASSERTX_RETURN( triggerdata, "Expected object trigger data" );
	UNITID idObject = UnitGetId( object );
	if (triggerdata && triggerdata->pfnTrigger)
	{
		int oldsource = UnitGetStat( object, STATS_SPAWN_SOURCE );
		UnitSetStat( object, STATS_SPAWN_SOURCE, UnitGetId( unit ) );
		bTriggered = triggerdata->pfnTrigger( game, client, object, triggerdata );
		object = UnitGetById( game, idObject );  // validate object is still there
		if ( !bTriggered && object )
		{
			UnitSetStat( object, STATS_SPAWN_SOURCE, oldsource );
		}
	} 
	else 
	{
		int nAI = UnitGetStat( object, STATS_AI );
		if ( nAI != INVALID_ID )
		{
			AI_Update( game, object, EVENT_DATA() );
			bTriggered = TRUE;
		}
	}
	
	if (bTriggered)
	{
		if (object && unit && !UnitTestFlag(unit, UNITFLAG_FREED ))
		{
			//take any items that need to be removed...
			sRemoveRequiredUnitTypesOnTrigger(unit, object, client);
		}
	
		object = UnitGetById( game, idObject );  // validate object is still there
		if (object && unit && !UnitTestFlag(unit, UNITFLAG_FREED ))
		{
			// send message to quests
			s_QuestEventObjectOperated(unit, object, FALSE);
		}

		object = UnitGetById( game, idObject );  // validate object is still there
		// deactivate trigger
		if (triggerdata && triggerdata->bClearTrigger && object)
		{
			// deactivate trigger
			UnitSetFlag( object, UNITFLAG_TRIGGER, FALSE );
		}
		
		// processing on unit
		if (unit && !UnitTestFlag(unit, UNITFLAG_FREED ))
		{		
			//test if achievement requirments are met		
			s_AchievementRequirmentTest( unit, unit, object, NULL );
		}					
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define DAVE_DEBUG		1

#ifdef DAVE_DEBUG

#include "debugwindow.h"
#include "s_gdi.h"
#include "common\colors.h"


static BOOL bDaveDebug = FALSE;
static GAME * drbGame = NULL;
static VECTOR drbWorldPos;
static UNITID drbPlayerID = INVALID_ID;

#define DRB_DEBUG_SCALE		10.0f

static void drbTransform( VECTOR & vPoint, int *pX, int *pY )
{
	VECTOR vTrans = vPoint - drbWorldPos;
	VectorScale( vTrans, vTrans, DRB_DEBUG_SCALE );
	*pX = (int)FLOOR( vTrans.fX ) + DEBUG_WINDOW_X_CENTER;
	*pY = (int)FLOOR( vTrans.fY ) + DEBUG_WINDOW_Y_CENTER;
}

static void drbDebugDrawWarp()
{
	if ( drbPlayerID == INVALID_ID )
		return;

	UNIT * drbPlayer = UnitGetById( drbGame, drbPlayerID );
	if ( !drbPlayer )
		return;

	drbWorldPos = drbPlayer->vPosition;

	int x1, y1, x2, y2;
	drbTransform( drbPlayer->vPosition, &x1, &y1 );
	gdi_drawpixel( x1, y1, gdi_color_red );
	int nRadius = (int)FLOOR( ( UnitGetCollisionRadius( drbPlayer ) * DRB_DEBUG_SCALE ) + 0.5f );
	gdi_drawcircle( x1, y1, nRadius, gdi_color_red );

	VECTOR vFace = drbPlayer->vFaceDirection * ( UnitGetCollisionRadius( drbPlayer ) * 1.5f );
	vFace += drbPlayer->vPosition;
	drbTransform( vFace, &x2, &y2 );
	gdi_drawline( x1, y1, x2, y2, gdi_color_red );


	ROOM * room = UnitGetRoom( drbPlayer );
	for ( UNIT * warp = room->tUnitList.ppFirst[TARGET_OBJECT]; warp; warp = warp->tRoomNode.pNext )
	{
		if ( UnitGetGenus( warp ) == GENUS_OBJECT )
		{
			const UNIT_DATA *objectdata = ObjectGetData( drbGame, UnitGetClass(warp) );
			if ( objectdata && UnitDataTestFlag(objectdata, UNIT_DATA_FLAG_TRIGGER) && objectdata->nTriggerType != INVALID_LINK )
			{
				OBJECT_TRIGGER_DATA * triggerdata = (OBJECT_TRIGGER_DATA *) ExcelGetData( drbGame, DATATABLE_OBJECTTRIGGERS, objectdata->nTriggerType );
				if ( triggerdata && triggerdata->bIsWarp )
				{
					int x1, y1, x2, y2;
					drbTransform( warp->tReservedLocation.vMin, &x1, &y1 );
					drbTransform( warp->tReservedLocation.vMax, &x2, &y2 );
					gdi_drawline( x1, y1, x2, y2, gdi_color_green );
				}
			}
		}
	}
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsWarp(
	UNIT* pUnit)
{			
	//TODO: Need to make sure tugboat has UNITTYPE_WARP for all it's warps( stairs, townportal ect ). Should still work ok for now but it'll be much faster
	return UnitIsA( pUnit, UNITTYPE_WARP ) || ObjectTriggerIsWarp( pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectTriggerIsWarp(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	// must be an object	
	if ( UnitIsA( pUnit, UNITTYPE_OBJECT ) )
	{
		GAME *pGame = UnitGetGame( pUnit );
		ASSERT_RETFALSE( pGame );
		const UNIT_DATA *pObjectData = ObjectGetData( pGame, pUnit );
		ASSERT_RETFALSE( pObjectData );
		const OBJECT_TRIGGER_DATA *pTriggerData = ObjectTriggerTryToGetData( pGame, pObjectData->nTriggerType );
		if (pTriggerData)
		{
			return pTriggerData->bIsWarp;
		}
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsDoor(
	UNIT* pUnit)
{			
	GAME* pGame = UnitGetGame( pUnit );
	if (UnitGetGenus( pUnit ) == GENUS_OBJECT)
	{
		const UNIT_DATA *pObjectData = ObjectGetData( pGame, UnitGetClass( pUnit ) );
		ASSERTX_RETFALSE(pObjectData, "Expected object data");
		const OBJECT_TRIGGER_DATA *pTriggerData = ( OBJECT_TRIGGER_DATA * )ExcelGetData( pGame, DATATABLE_OBJECTTRIGGERS, pObjectData->nTriggerType );
		return pTriggerData ? pTriggerData->bIsDoor : FALSE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsPortalToTown(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	if (UnitIsA( pUnit, UNITTYPE_PORTAL_TO_TOWN ))
	{
		LEVEL_TYPE tLevelTypeDest = TownPortalGetDestination( pUnit );
		if (AppIsHellgate())
		{
			return tLevelTypeDest.nLevelDef != INVALID_LINK;
		}
		else
		{
			return tLevelTypeDest.nLevelArea != INVALID_LINK;
		}
		
	}
	return FALSE;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsPortalFromTown(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsA( pUnit, UNITTYPE_PORTAL_FROM_TOWN );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsVisualPortal(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	return UnitIsA( pUnit, UNITTYPE_VISUAL_PORTAL );
}


BOOL ObjectWarpAllowsInvalidDestination(	
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	if (ObjectTriggerIsWarp( pUnit ))
	{
		GAME *pGame = UnitGetGame( pUnit );
		const UNIT_DATA *pObjectData = ObjectGetData( pGame, pUnit );
		ASSERTX_RETFALSE(pObjectData, "Expected object data");
		const OBJECT_TRIGGER_DATA *pTriggerData = ObjectTriggerTryToGetData( pGame, pObjectData->nTriggerType );
		if (pTriggerData)
		{		
			return pTriggerData->bAllowsInvalidLevelDestination;
		}
	}
	
	return FALSE;	
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsDynamicWarp(	
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	if (ObjectTriggerIsWarp( pUnit ))
	{
		GAME *pGame = UnitGetGame( pUnit );
		const UNIT_DATA *pObjectData = ObjectGetData( pGame, pUnit );
		ASSERTX_RETFALSE(pObjectData, "Expected object data");
		const OBJECT_TRIGGER_DATA *pTriggerData = ObjectTriggerTryToGetData( pGame, pObjectData->nTriggerType );
		if (pTriggerData)
		{		
			return pTriggerData->bIsDynamicWarp;
		}
	}
	
	return FALSE;		
	
}	
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define DRB_GSTAR_KLUDGE_DISTANCE		1.0f

void ObjectsCheckForTriggers(
	GAME * game,
	GAMECLIENT * client, 
	UNIT * unit )
{
#ifdef DAVE_DEBUG
	if ( !bDaveDebug )
	{
		drbGame = game;
		drbPlayerID = UnitGetId( ClientGetControlUnit( client ) );
		DaveDebugWindowSetDrawFn( drbDebugDrawWarp );
		DaveDebugWindowShow();
		bDaveDebug = TRUE;
	}
#endif

	ASSERT_RETURN(unit);
	ASSERT_RETURN(game && IS_SERVER(game));

	if (UnitGetStat(unit, STATS_DONT_CHECK_FOR_TRIGGERS_COUNTER) > 0)
	{
		return;
	}
	
	BOOL bPlayerBox = FALSE;
	BOUNDING_BOX plrbox;

	BOOL bCollidedAny = FALSE;

	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];

	SKILL_TARGETING_QUERY query;
	SETBIT( query.tFilter.pdwFlags, SKILL_TARGETING_BIT_IGNORE_TEAM );
	SETBIT( query.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
	query.tFilter.nUnitType = UNITTYPE_OBJECT;
	query.fMaxRange	= 50.0f;
	query.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	query.pnUnitIds   = pnUnitIds;
	query.pSeekerUnit = unit;
	SkillTargetQuery( game, query );

	for ( int i = 0; i < query.nUnitIdCount; i++ )
	{
		UNIT * test = UnitGetById( game, query.pnUnitIds[ i ] );
		if ( ! test )
			continue;

		if ( UnitTestFlag( test, UNITFLAG_TRIGGER ) && !UnitTestFlag( test, UNITFLAG_DONT_TRIGGER_BY_PROXIMITY ) )
		{
			if ( ObjectIsWarp( test ) )
			{
				if ( UnitGetCollisionRadius( test ) > 0.0f )
				{
					// warps now work on distance to a line/plane rather than box collision
					float fDist;
					if ( DistancePointLineXY( &unit->vPosition, &test->tReservedLocation.vMin, &test->tReservedLocation.vMax, &fDist, DISTANCE_POINT_FLAGS_WITH_Z_RANGE ) )
					{
						//if ( fDist < UnitGetCollisionRadius( unit ) )
						// this was compared to unit radius... you were going through in 1st person. just made it 1.0f instead of unit radius of 0.6 for now...
						if ( fDist < DRB_GSTAR_KLUDGE_DISTANCE )
						{
							ObjectTrigger( game, client, unit, test );
						}
					}
				}
			}
			else
			{
				float fUnitRadius = UnitGetCollisionRadius( unit );
				// init player box if not one yet
				if ( !bPlayerBox )
				{
					plrbox.vMin.fX = unit->vPosition.fX - fUnitRadius;
					plrbox.vMin.fY = unit->vPosition.fY - fUnitRadius;
					plrbox.vMin.fZ = unit->vPosition.fZ;
					plrbox.vMax = plrbox.vMin;
					VECTOR vTemp;
					vTemp.fX = unit->vPosition.fX + fUnitRadius;
					vTemp.fY = unit->vPosition.fY + fUnitRadius;
					vTemp.fZ = unit->vPosition.fZ + UnitGetCollisionHeight( unit );
					BoundingBoxExpandByPoint( &plrbox, &vTemp );
					bPlayerBox = TRUE;
				}

				// do they collide?
				if ( BoundingBoxCollide( &plrbox, &test->tReservedLocation, 0.1f ) )
				{
					bCollidedAny = TRUE;
					// is the collide distance really close enough?
					float fTestRadius = UnitGetCollisionRadius( test );
					float fMin = ( fUnitRadius * fUnitRadius ) + ( fTestRadius * fTestRadius );
					if ( VectorDistanceSquaredXY( unit->vPosition, test->vPosition ) < fMin )
					{
						ObjectTrigger(game, client, unit, test);
					}
				}
			}
			
			// if we're now freed, forget checking anything else
			if (unit == NULL || UnitTestFlag( unit, UNITFLAG_FREED ))
			{
				return;
			}			
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDoObjectTrigger(
	UNIT *pUnit,
	void *pCallbackData)
{		
	UNIT *pPlayer = (UNIT *)pCallbackData;
			
	if (UnitTestFlag( pUnit, UNITFLAG_TRIGGER ) && 
		!UnitTestFlag( pUnit, UNITFLAG_DONT_TRIGGER_BY_PROXIMITY ))
	{
		const UNIT_DATA * pObjectData = UnitGetData( pUnit );
		if(UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_TRIGGER_ON_ENTER_ROOM))
		{
			GAME *pGame = UnitGetGame( pUnit );
			GAMECLIENT *pClient = UnitGetClient( pPlayer );			
			ObjectTrigger(pGame, pClient, pPlayer, pUnit );
		}
	}

	return RIU_CONTINUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectsCheckForTriggersOnRoomChange(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,
	ROOM * room)
{
	ASSERT( unit );
	ASSERT( game && IS_SERVER( game ) );

	if (UnitGetStat( unit, STATS_DONT_CHECK_FOR_TRIGGERS_COUNTER ) > 0)
	{
		return;
	}

	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	RoomIterateUnits( room, eTargetTypes, sDoObjectTrigger, unit );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void ObjectTriggerReset( GAME * game, UNIT * unit, int nClass )
{
	ASSERT_RETURN( unit );
	ASSERT_RETURN( game && IS_SERVER( game ) );

	const UNIT_DATA * data = ObjectGetData( game, nClass );
	ASSERTX_RETURN(data, "Expected object data");
	if ( !UnitDataTestFlag(data, UNIT_DATA_FLAG_TRIGGER))
	{
		return;
	}

	ROOM * mainroom = unit->pRoom;
	int nearbyRoomCount = RoomGetNearbyRoomCount(mainroom);
	for (int ii = -1; ii < nearbyRoomCount; ii++)
	{
		ROOM * room;
		if ( ii == -1 )
			room = mainroom;
		else
			room = RoomGetNearbyRoom(mainroom, ii);

		if ( !room )
			continue;

		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
		{
			if (UnitGetClass(test) == nClass)
			{
				UnitSetFlag( test, UNITFLAG_TRIGGER );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//#define DAVE_TRAIN_DEBUG		1

#ifdef DAVE_TRAIN_DEBUG

#include "debugwindow.h"
#include "s_gdi.h"
#include "common\colors.h"


static BOOL bDaveDebug = FALSE;
static GAME * drbGame = NULL;
static VECTOR drbWorldPos;
static UNITID drbPlayerID = INVALID_ID;
static UNITID drbTrainID = INVALID_ID;
static VECTOR drbTransformedPos;
static float drbTrainWidth, drbTrainLength;

#define DRB_DEBUG_SCALE		10.0f

static void drbTransform( VECTOR & vPoint, int *pX, int *pY )
{
	VECTOR vTrans = vPoint - drbWorldPos;
	VectorScale( vTrans, vTrans, DRB_DEBUG_SCALE );
	*pX = (int)FLOOR( vTrans.fX ) + DEBUG_WINDOW_X_CENTER;
	*pY = (int)FLOOR( vTrans.fY ) + DEBUG_WINDOW_Y_CENTER;
}

static void drbDebugDrawTrain()
{
	if ( !drbGame )
		return;

	if ( drbPlayerID == INVALID_ID )
		return;

	UNIT * drbPlayer = UnitGetById( drbGame, drbPlayerID );
	if ( !drbPlayer )
		return;

	UNIT * drbTrain = UnitGetById( drbGame, drbTrainID );
	if ( !drbTrain )
		return;

	drbWorldPos = drbTrain->vPosition;

	int x1, y1, x2, y2;
	drbTransform( drbTransformedPos, &x1, &y1 );
	gdi_drawpixel( x1, y1, gdi_color_red );
	int nRadius = (int)FLOOR( ( UnitGetCollisionRadius( drbPlayer ) * DRB_DEBUG_SCALE ) + 0.5f );
	gdi_drawcircle( x1, y1, nRadius, gdi_color_red );

	VECTOR ul, lr;
	ul.x = drbTrain->vPosition.x - ( drbTrainWidth * 0.5f );
	ul.y = drbTrain->vPosition.y - ( drbTrainLength * 0.5f );
	lr.x = ul.x + drbTrainWidth;
	lr.y = ul.y + drbTrainLength;
	drbTransform( ul, &x1, &y1 );
	drbTransform( lr, &x2, &y2 );
	gdi_drawbox( x1, y1, x2, y2, gdi_color_green );
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// need to data this, but fuck it for now...

typedef struct 
{
	GLOBAL_INDEX		eIndex;
	float				width;
	float				length;
	float				height;
} TRAIN_COLLISION_DATA;

static TRAIN_COLLISION_DATA	sgTrainCollision[] =
{
	//	global index					x width (xy only)		y length (xy only)		height (z)
	{ GI_OBJECT_BATTLETRAIN_ENGINE,		14.4f,					4.5f,					0.00f	},
	{ GI_OBJECT_BATTLETRAIN_PASSENGER,	12.0f,					4.5f,					0.00f	},
	{ GI_OBJECT_BATTLETRAIN_FLATBED,	13.8f,					4.6f,					0.85f	},
	//{ GI_OBJECT_BATTLETRAIN_FLATBED,	15.7f,					5.6f,					0.00f	},
	{ GI_INVALID,						0.0f,					0.0f,					0.00f	},
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define TRAIN_START_VELOCITY			0.1f
#define MIN_TRAIN_VELOCITY				0.5f
#define MAX_TRAIN_VELOCITY				25.0f

#define TRAIN_ACCELERATION				0.1f
#define TRAIN_DECELERATION				0.15f

#define TRAIN_DECELERATION_DISTANCE		415.0f
#define TRAIN_TRAVEL_DISTANCE			510.0f

static VECTOR sgvTrainMoveDir( -1.0f, 0.0f, 0.0 );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTrainTransform( UNIT * train, VECTOR & upper_left, VECTOR & upper_right, VECTOR & lower_left, VECTOR & lower_right )
{
	// which car are we dealing with?
	int index = 0;
	int train_class = UnitGetClass( train );
	while ( GlobalIndexGet( sgTrainCollision[index].eIndex ) != train_class )
	{
		index++;
		ASSERT_RETURN( sgTrainCollision[index].eIndex != GI_INVALID );
	}
	TRAIN_COLLISION_DATA * data = &sgTrainCollision[index];

	// get coords
	upper_left.z = upper_right.z = lower_left.z = lower_right.z = data->height;
	upper_left.x = -( data->width * 0.5f );
	upper_left.y = -( data->length * 0.5f );
	lower_left.x = upper_left.x;
	lower_left.y = upper_left.y + data->length;
	upper_right.x = upper_left.x + data->width;
	upper_right.y = upper_left.y;
	lower_right.x = upper_right.x;
	lower_right.y = lower_left.y;

	// transform
	float angle = VectorDirectionToAngle( train->vFaceDirection );
	VectorZRotation( upper_left, angle );
	upper_left += train->vPosition;
	VectorZRotation( upper_right, angle );
	upper_right += train->vPosition;
	VectorZRotation( lower_left, angle );
	lower_left += train->vPosition;
	VectorZRotation( lower_right, angle );
	lower_right += train->vPosition;
}

//----------------------------------------------------------------------------
static void sReserveTrainNodes( UNIT * train )
{
	VECTOR upper_left, upper_right, lower_left, lower_right;
	sTrainTransform( train, upper_left, upper_right, lower_left, lower_right );
	BOUNDING_BOX tBox;
	tBox.vMin = upper_left;
	tBox.vMax = upper_left;
	BoundingBoxExpandByPoint( &tBox, &upper_right );
	BoundingBoxExpandByPoint( &tBox, &lower_left );
	BoundingBoxExpandByPoint( &tBox, &lower_right );
	upper_left.z = train->vPosition.z - 0.1f;
	upper_right.z = upper_left.z;
	lower_left.z = upper_left.z;
	lower_right.z = upper_left.z;
	BoundingBoxExpandByPoint( &tBox, &upper_left );
	BoundingBoxExpandByPoint( &tBox, &upper_right );
	BoundingBoxExpandByPoint( &tBox, &lower_left );
	BoundingBoxExpandByPoint( &tBox, &lower_right );
	BoundingBoxExpandBySize( &tBox, 1.0f );
	UnitReservePathNodeArea( train, UnitGetRoom( train ), &tBox );
}

//----------------------------------------------------------------------------
VECTOR ObjectTrainMove( UNIT * train, float fTimeDelta, BOOL bRealStep )
{
	ASSERT_RETVAL( train, VECTOR( 0,0,0 ) );
	GAME * pGame = UnitGetGame( train );
	float fTrainVelocity = UnitGetVelocity( train );
	float velocity = fTrainVelocity * fTimeDelta;
	float fTrainDistance = UnitGetStatFloat( train, STATS_PATH_DISTANCE );
	if ( bRealStep )
	{
		fTrainDistance += velocity;
		if ( fTrainDistance < TRAIN_DECELERATION_DISTANCE )
		{
			if ( fTrainVelocity < MAX_TRAIN_VELOCITY )
			{
				fTrainVelocity += TRAIN_ACCELERATION;
			}
			else
			{
				if ( IS_SERVER( pGame ) && UnitHasState( pGame, train, STATE_TRAIN_STARTUP ) )
				{
					s_StateClear( train, UnitGetId( train ), STATE_TRAIN_STARTUP, 0 );
					s_StateSet( train, train, STATE_TRAIN_RUNNING, 0 );
				}
			}
		}
		else
		{
			if ( IS_SERVER( pGame ) && UnitHasState( pGame, train, STATE_TRAIN_RUNNING ) )
			{
				s_StateClear( train, UnitGetId( train ), STATE_TRAIN_RUNNING, 0 );
				s_StateSet( train, train, STATE_TRAIN_STOPPING, 0 );
			}
			if ( fTrainVelocity > MIN_TRAIN_VELOCITY )
			{
				fTrainVelocity -= TRAIN_DECELERATION;
			}
		}
		if ( fTrainDistance > TRAIN_TRAVEL_DISTANCE )
		{
			fTrainVelocity = 0.0f;
			UnitStepListRemove( UnitGetGame( train ), train, SLRF_FORCE );
			s_QuestEndTrainRide( UnitGetGame( train ) );
			sReserveTrainNodes( train );
			if ( IS_SERVER( pGame ) && UnitHasState( pGame, train, STATE_TRAIN_STOPPING ) )
			{
				s_StateClear( train, UnitGetId( train ), STATE_TRAIN_STOPPING, 0 );
				s_StateSet( train, train, STATE_TRAIN_IDLE, 0 );
			}
		}
		UnitSetVelocity( train, fTrainVelocity );

	}
	UnitSetStatFloat( train, STATS_PATH_DISTANCE, 0, fTrainDistance );
	VECTOR vDelta = sgvTrainMoveDir;
	vDelta *= velocity;
	VECTOR vNewPosition = train->vPosition;
	vNewPosition += vDelta;
	return vNewPosition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

VECTOR ObjectTrainMoveRider( UNIT * pPlayer, VECTOR vCurrentNew, float fTimeDelta )
{
	// get UNIT from id of train car that is moving the player
	UNITID idTrain = UnitGetStat( pPlayer, STATS_RIDE_UNITID );
	ASSERT_RETVAL( idTrain != INVALID_ID, vCurrentNew );
	UNIT * pTrain = UnitGetById( UnitGetGame( pPlayer ), idTrain );
	ASSERT_RETVAL( pTrain, vCurrentNew );

	float fTrainVelocity = UnitGetVelocity( pTrain );
	float velocity = fTrainVelocity * fTimeDelta;
	VECTOR vDelta = sgvTrainMoveDir;
	vDelta *= velocity;

	// add delta to player
	VECTOR vDest = vCurrentNew + vDelta;
	return vDest;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTrainCollidePush( UNIT * player, VECTOR * v1, VECTOR * v2, float distance )
{
	VECTOR vNormal;
	vNormal.fX = v1->y - v2->y;
	vNormal.fY = v2->x - v1->x;
	vNormal.fZ = 0.0f;
	VectorNormalize( vNormal );
	float fDelta = UnitGetCollisionRadius( player ) - distance;
	VECTOR vNewPosition = UnitGetPosition(player);
	vNewPosition.fX += vNormal.fX * fDelta;
	vNewPosition.fY += vNormal.fY * fDelta;
	UnitSetPosition(player, vNewPosition);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTrainSideCollide( UNIT * player, VECTOR * v1, VECTOR * v2 )
{
	float distance;
	if ( DistancePointLineXY( &player->vPosition, v1, v2, &distance ) )
	{
		if ( ( distance > 0.0f ) && ( distance < UnitGetCollisionRadius( player ) ) )
		{
			sTrainCollidePush( player, v1, v2, distance );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sPlayerIsInTrainBounds( UNIT * player, UNIT * train, VECTOR & upper_left, VECTOR & lower_right )
{
	// is player within my bounds?
	VECTOR playerxy_in_trainspace = player->vPosition - train->vPosition;
	playerxy_in_trainspace.z = 0.0f;
	VECTOR normal;
	VectorNormalize( normal, playerxy_in_trainspace );
	float angle = VectorAngleBetween( normal, train->vFaceDirection );
	float distance = sqrtf( VectorLengthSquaredXY( playerxy_in_trainspace ) );
	VECTOR player_transformed;
	player_transformed.x = cosf( angle ) * distance;
	player_transformed.y = sinf( angle ) * distance;

#ifdef DAVE_TRAIN_DEBUG
	if ( !bDaveDebug )
	{
		drbGame = UnitGetGame( player );
		drbPlayerID = UnitGetId( player );
		drbTrainID = UnitGetId( train );
		DaveDebugWindowSetDrawFn( drbDebugDrawTrain );
		DaveDebugWindowShow();
		bDaveDebug = TRUE;
	}
	drbTransformedPos = player_transformed + train->vPosition;
	drbTrainWidth = lower_right.x - upper_left.x;
	drbTrainLength = lower_right.y - upper_left.y;
#endif

	if ( ( player_transformed.x >= upper_left.x ) && ( player_transformed.x <= lower_right.x ) && ( player_transformed.y >= upper_left.y ) && ( player_transformed.y <= lower_right.y ) )
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sCollideTrain( UNIT * player, UNIT * train )
{
	ASSERT_RETURN( player );
	ASSERT_RETURN( train );

	VECTOR upper_left, upper_right, lower_left, lower_right;
	sTrainTransform( train, upper_left, upper_right, lower_left, lower_right );

#ifdef DAVE_TRAIN_DEBUG
	if ( IS_CLIENT( player ) )
	{
		e_PrimitiveAddLine( 0, &upper_left, &upper_right );
		e_PrimitiveAddLine( 0, &upper_right, &lower_right );
		e_PrimitiveAddLine( 0, &lower_right, &lower_left );
		e_PrimitiveAddLine( 0, &lower_left, &upper_left );
	}
#endif

	// solid sides baby!
	sTrainSideCollide( player, &upper_left, &lower_left );
	sTrainSideCollide( player, &lower_left, &lower_right );
	sTrainSideCollide( player, &lower_right, &upper_right );
	sTrainSideCollide( player, &upper_right, &upper_left );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectDoBlockingCollision(
	UNIT *player,
	UNIT *test)
{			
	// work on distance to a line/plane rather than box collision
	float fDist;
	if ( DistancePointLineXY( &player->vPosition, &test->tReservedLocation.vMin, &test->tReservedLocation.vMax, &fDist, DISTANCE_POINT_FLAGS_WITH_Z_RANGE ) )
	{
		if ( ( fDist > 0.0f ) && ( fDist < UnitGetCollisionRadius( player ) ) )
		{
			// this unit was blocking
			VECTOR vNormal;
			vNormal.fX = test->tReservedLocation.vMin.fY - test->tReservedLocation.vMax.fY;
			vNormal.fY = test->tReservedLocation.vMax.fX - test->tReservedLocation.vMin.fX;
			vNormal.fZ = 0.0f;
			VectorNormalize( vNormal );
			float fDelta = UnitGetCollisionRadius(player) - fDist;
			VECTOR vNewPosition = UnitGetPosition(player);
			vNewPosition.fX += vNormal.fX * fDelta;
			vNewPosition.fY += vNormal.fY * fDelta;
			UnitSetPosition(player, vNewPosition);
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectsCheckBlockingCollision( 
	UNIT * player)
{
	ASSERT( player );
	GAME * game = UnitGetGame( player );
	ASSERT( game && IS_CLIENT( game ) );

	ROOM * room = player->pRoom;
	ASSERT( room );
	for ( UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
	{
		if (ObjectIsBlocking( test, player ))
		{
			ObjectDoBlockingCollision( player, test );
		}
		if ( UnitIsA( test, UNITTYPE_TRAIN ) )
		{
			// solid cart collision baby!
			sCollideTrain( player, test );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectBlocksGhosts( 
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	return pUnitData->bBlockGhosts;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsBlocking(
	UNIT *pUnit,
	UNIT *pAsking /*= NULL*/)
{
	ASSERTX_RETTRUE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
		
	// check stat
	BOOL bBlock = ( UnitGetStat( pUnit, STATS_BLOCK_MOVEMENT ) == BSV_BLOCKING );

	// see if this unit type blocks if we have an asker
	if (pAsking)
	{
	
		// ghosts
		if (ObjectBlocksGhosts( pUnit ) && UnitIsGhost( pAsking ))
		{
			bBlock = TRUE;
		}
		
		// warps block players sometimes
		if (ObjectIsWarp( pUnit ))
		{
			if (UnitHasState( pGame, pAsking, STATE_NO_TRIGGER_WARPS ) ||
				WarpDestinationIsAvailable( pAsking, pUnit ) != WRR_OK)
			{
				bBlock = TRUE;
			}
		}
		
	}
		
	// allow quests to undo that
	if ( AppIsHellgate() && UnitHasState( UnitGetGame( pUnit ), pUnit, STATE_QUEST_HIDDEN ) )
	{
		bBlock = FALSE;
	}
	
	return bBlock;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectSetBlocking(
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	UnitSetStat( pUnit, STATS_BLOCK_MOVEMENT, BSV_BLOCKING );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectClearBlocking(
	UNIT *pUnit)
{				
	ASSERTX_RETURN( pUnit, "Expected unit" );	
	UnitSetStat( pUnit, STATS_BLOCK_MOVEMENT, BSV_OPEN );			
}

//----------------------------------------------------------------------------
struct DISABLE_BLOCKER_DATA
{
	int nObjectClass;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDisableBlocker(
	UNIT *pUnit,
	void *pCallbackData)
{
	DISABLE_BLOCKER_DATA *pData = (DISABLE_BLOCKER_DATA *)pCallbackData;
	ASSERT( UnitGetGenus( pUnit ) == GENUS_OBJECT );


	if (UnitGetClass( pUnit ) == pData->nObjectClass)
	{
		ASSERT( ObjectIsBlocking( pUnit ) );

		// deactivate trigger
		UnitSetFlag( pUnit, UNITFLAG_TRIGGER, FALSE );
		
		// This only works for single-player and solo-instances
		ObjectClearBlocking( pUnit );
		
		return RIU_STOP;  // foudn it, stop iterating

	}
	
	return RIU_CONTINUE;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ObjectBlockerDisable( 
	GAME * game, 
	UNIT * player, 
	GLOBAL_INDEX eGlobalIndexDisable)
{
	ASSERT( player );
	ASSERT( game && IS_SERVER(game) );
#ifdef _DEBUG	
	ASSERT( GlobalIndexGetDatatable( eGlobalIndexDisable ) );
#endif	
	int nObjectClass = GlobalIndexGet( eGlobalIndexDisable );
	
	ROOM * mainroom = player->pRoom;
	int nearbyRoomCount = RoomGetNearbyRoomCount(mainroom);
	for ( int ii = -1; ii < nearbyRoomCount; ii++ )
	{
		ROOM * room;
		if ( ii == -1 )
			room = mainroom;
		else
			room = RoomGetNearbyRoom(mainroom, ii);

		if ( !room )
			continue;

		DISABLE_BLOCKER_DATA tData;
		tData.nObjectClass = nObjectClass;
		TARGET_TYPE eTargetTypes [] = { TARGET_OBJECT, TARGET_INVALID };
		RoomIterateUnits( room, eTargetTypes, sDisableBlocker, &tData );
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


#if ( ISVERSION( CHEATS_ENABLED ) )		// CHB 2006.07.07 - Inconsistent usage.
void Cheat_OpenWarps( GAME * game, UNIT * player )
{
	ASSERT_RETURN( game && IS_SERVER(game) );
	ASSERT_RETURN( player );

	// all new will be open
	GameSetDebugFlag( game, DEBUGFLAG_OPEN_WARPS, TRUE );

	// clear all set
	LEVEL * pLevel = UnitGetLevel( player );
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetGenus( test ) != GENUS_OBJECT )
			{
				continue;
			}

			if ( !UnitTestFlag( test, UNITFLAG_TRIGGER ) )
			{
				continue;
			}

			if ( !ObjectIsBlocking( test ) )
			{
				continue;
			}

			const UNIT_DATA * object_data = ObjectGetData( game, UnitGetClass(test) );
			ASSERTX_RETURN(object_data, "Expected object data");
			const OBJECT_TRIGGER_DATA * triggerdata = ObjectTriggerTryToGetData( game, object_data->nTriggerType );
			if ( triggerdata && triggerdata->bIsWarp )
			{
				ObjectClearBlocking( test );

				MSG_SCMD_BLOCKING_STATE msg;
				msg.idUnit = UnitGetId( test );
				msg.nState = BSV_OPEN;
				s_SendMessage( game, UnitGetClientId( player ), SCMD_BLOCKING_STATE, &msg );
			}
		}
	}

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const struct UNIT_DATA* ObjectGetData(
	GAME* game,
	int nClass)
{
	return (const UNIT_DATA*)ExcelGetData(game, DATATABLE_OBJECTS, nClass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const struct UNIT_DATA* ObjectGetData(
	GAME* game,
	UNIT* unit)
{
	return ObjectGetData( game, UnitGetClass( unit ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const struct OBJECT_TRIGGER_DATA *ObjectTriggerTryToGetData(
	GAME *pGame,
	int nTriggerType)
{
	if (nTriggerType != INVALID_LINK)
	{
		return (const OBJECT_TRIGGER_DATA *)ExcelGetData( NULL, DATATABLE_OBJECTTRIGGERS, nTriggerType );
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsQuestImportantInfo(
	UNIT * unit )
{
	ASSERT_RETFALSE( unit );
	if ( UnitGetGenus(unit) != GENUS_OBJECT )
		return FALSE;
	GAME * game = UnitGetGame( unit );
	ASSERT_RETFALSE( game );
	const UNIT_DATA * data = ObjectGetData( game, unit );
	ASSERT_RETFALSE( data );
	return UnitDataTestFlag(data, UNIT_DATA_FLAG_QUEST_IMPORTANT_INFO);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsHeadstone( 
	UNIT *pObject, 
	UNIT *pOwner, 
	int *pnCreationTick)
{
	ASSERTX_RETFALSE( pObject, "Expected unit object" );
	ASSERTX_RETFALSE( pObject, "Expected unit owner" );
	
	// get owner guid
	PGUID guidOwner = UnitGetGuid( pOwner );

	// must be headstone type
	if (UnitIsA( pObject, UNITTYPE_HEADSTONE ))
	{
		PGUID guidObject = UnitGetGUIDOwner( pObject );
		if (guidObject == guidOwner)
		{
		
			// if requested, save when this marker was created
			if (pnCreationTick)
			{
				*pnCreationTick = UnitGetStat( pObject, STATS_SPAWN_TICK );
			}
						
			return TRUE;  // this is a corpse marker for the owner in question
			
		}
		
	}

	// not a headstone
	return FALSE;		
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectIsSubLevelWarp(
	UNIT *pObject)
{
	ASSERTX_RETFALSE( pObject, "Expected unit" );
	if (UnitIsA( pObject, UNITTYPE_SUBLEVEL_PORTAL ))
	{
		return TRUE;
	}
	if (UnitIsA( pObject, UNITTYPE_OBJECT ))
	{
		GAME *pGame = UnitGetGame( pObject );
		const UNIT_DATA *pObjectData = UnitGetData( pObject );
		const OBJECT_TRIGGER_DATA *pTriggerData = ObjectTriggerTryToGetData( pGame, pObjectData->nTriggerType );
		if (pTriggerData)
		{
			if (pTriggerData->bIsSubLevelWarp)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FONTCOLOR ObjectWarpGetNameColor(
	UNIT *pUnit)
{
	ASSERT_RETVAL(pUnit, FONTCOLOR_INVALID);

	if ( ObjectIsWarp( pUnit ) )
	{
		if (AppIsHellgate() && 
			ObjectIsBlocking( pUnit ) && 
			ObjectIsDynamicWarp( pUnit ) == FALSE)
		{
			return FONTCOLOR_AUTOMAP_WARP_CLOSED;
		}

#if !ISVERSION(SERVER_VERSION)
		// color it different if the player has a quest there.
		GAME *pGame = UnitGetGame(pUnit);
		ASSERT_RETVAL(pGame, FONTCOLOR_INVALID);
		if (IS_CLIENT(pGame))
		{
			UNIT *pPlayer = GameGetControlUnit(pGame);
			if (pPlayer)
			{
				int nLevelDefDest = UnitGetStat( pUnit, STATS_WARP_DEST_LEVEL_DEF );
				if (c_QuestPlayerHasObjectiveInLevel(pPlayer, nLevelDefDest))
				{
					return FONTCOLOR_WARP_QUEST_LABEL;
				}
			}
		}
#endif //!ISVERSION(SERVER_VERSION)

		if (ObjectIsPortalToTown( pUnit ) ||
			ObjectIsPortalFromTown( pUnit ))
		{
			return FONTCOLOR_TOWN_PORTAL_LABEL;
		}

		return FONTCOLOR_AUTOMAP_WARP_OPEN;
	}

	return FONTCOLOR_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ObjectGetName(
	GAME *pGame,
	UNIT *pUnit,
	WCHAR *puszNameBuffer,
	int nBufferSize,
	DWORD dwFlags)
{						
	const UNIT_DATA *pObjectData = ObjectGetData( pGame, UnitGetClass( pUnit ));
	ASSERT_RETFALSE(pObjectData);

	const int MAX_BUFFER = 1024;
	WCHAR uszBuffer[ MAX_BUFFER ] = { 0 };
	WCHAR uszSubName[ MAX_BUFFER ] = { 0 };
	WCHAR uszLevelAreaName[ MAX_BUFFER ] = { 0 };
	WCHAR uszYetAnotherBuffer[ MAX_BUFFER ] = { 0 };

	BOOL bHasSubName = FALSE;		
	const WCHAR *puszName = NULL;
	const WCHAR * puszBlockingWarp = NULL;
	const WCHAR * puszBlockingTrial = NULL;
	int nLevelArea = INVALID_ID;

	if (UnitIsA( pUnit, UNITTYPE_WARP_PARTY_MEMBER_ENTER ) ||
		UnitIsA( pUnit, UNITTYPE_WARP_PARTY_MEMBER_EXIT ))
	{
		puszName = StringTableGetStringByIndex( pObjectData->nString );
	}
	else if ( ObjectIsWarp( pUnit ) )
	{
	
		// add string for blocking warps
		if (AppIsHellgate() && 
			ObjectIsBlocking( pUnit ) && 
			ObjectIsDynamicWarp( pUnit ) == FALSE &&
			TESTBIT( dwFlags, UNF_BASE_NAME_ONLY_BIT ) == FALSE)
		{
			if ( UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_ASK_FACTION_FOR_OPERATE) )
			{
				puszBlockingWarp = GlobalStringGet( GS_FACTION_BLOCKED_WARP );
			}
			else
			{
				puszBlockingWarp = GlobalStringGet( GS_BLOCKED_WARP );
			}

			// append to the end for blocked string a message specific for 
			// trial accounts (if any)
			WARP_RESTRICTED_REASON eReason = c_WarpGetBlockingReason( pUnit );
			if (eReason != WRR_INVALID)
			{
				puszBlockingTrial = GlobalStringGet( GS_BLOCKED_WARP_TRIAL_ACCOUNT );
			}
												
		}

		const WCHAR *puszTownPortalFormat = GlobalStringGet( GS_TOWN_PORTAL_WARP );
		if (UnitIsA( pUnit, UNITTYPE_WARP_RECALL_ENTER ))
		{
			int nLevelDefDest = UnitGetStat( pUnit, STATS_WARP_RECALL_ENTER_DEST_LEVEL_DEF );
			ASSERTX( nLevelDefDest != INVALID_LINK, "Invalid recall destination" );
			if (nLevelDefDest != INVALID_LINK)
			{
				const LEVEL_DEFINITION *pLevelDefDest = LevelDefinitionGet( nLevelDefDest );
				if (pLevelDefDest)
				{
					const WCHAR *pszLevelDefDest = LevelDefinitionGetDisplayName( pLevelDefDest );
					const WCHAR *pszFormat = GlobalStringGet( GS_RECALL_DESTINATION_LABEL );
					PStrCopy( uszBuffer, pszFormat, MAX_BUFFER );
					PStrReplaceToken( uszBuffer, MAX_BUFFER, StringReplacementTokensGet( SR_LEVEL_DEST ), pszLevelDefDest );
					puszName = uszBuffer;
				}
			}
		}
		else if (ObjectIsPortalFromTown( pUnit ))
		{
#if !ISVERSION(SERVER_VERSION)
			c_TownPortalGetReturnWarpName( pGame, uszBuffer, MAX_BUFFER );
			if (uszBuffer[0] &&
				GlobalStringGetStringIndex( GS_TOWN_PORTAL_WARP ) != INVALID_ID &&
				puszTownPortalFormat)
			{
				PStrCopy(uszYetAnotherBuffer, puszTownPortalFormat, MAX_BUFFER);
				PStrReplaceToken(uszYetAnotherBuffer, MAX_BUFFER, StringReplacementTokensGet( SR_STRING1 ), uszBuffer);
				puszName = uszYetAnotherBuffer;
			}
			else
			{
				puszName = uszBuffer;
			}
#endif// !ISVERSION(SERVER_VERSION)
		}
		else if (ObjectIsPortalToTown( pUnit ))
		{			
			LEVEL_TYPE tLevelType = TownPortalGetDestination( pUnit );						
			if (tLevelType.nLevelArea != INVALID_LINK)
			{
				//int nStringID = MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( tLevelType.nLevelArea );
				if( tLevelType.nLevelArea != INVALID_ID )
				{
					MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( tLevelType.nLevelArea, tLevelType.nLevelDepth, &uszBuffer[0], MAX_BUFFER );
				}
				else
				{
					int nLevelDef = LevelDefinitionGetRandom( NULL, tLevelType.nLevelArea, tLevelType.nLevelDepth );			
					PStrCopy(uszBuffer, LevelDefinitionGetDisplayName( nLevelDef ), MAX_BUFFER);
				}
			}
			else if (tLevelType.nLevelDef != INVALID_LINK)
			{
				PStrCopy(uszBuffer, LevelDefinitionGetDisplayName( tLevelType.nLevelDef ), MAX_BUFFER);
			}
			

			// get subname for depth (if any)			
			bHasSubName = LevelDepthGetDisplayName( 
				pGame,
				tLevelType.nLevelArea,
				tLevelType.nLevelDef,
				tLevelType.nLevelDepth, 
				uszSubName, 
				MAX_BUFFER);
			if (bHasSubName)
			{
				PStrCat(uszBuffer, uszSubName, MAX_BUFFER);
				bHasSubName = FALSE;
			}
			
			if ( uszBuffer[0] &&
				GlobalStringGetStringIndex( GS_TOWN_PORTAL_WARP ) != INVALID_ID &&
				puszTownPortalFormat)
			{
				PStrCopy(uszYetAnotherBuffer, puszTownPortalFormat, MAX_BUFFER);
				PStrReplaceToken(uszYetAnotherBuffer, MAX_BUFFER, StringReplacementTokensGet( SR_STRING1 ), uszBuffer);
				puszName = uszYetAnotherBuffer;
			}
			else
			{
				puszName = uszBuffer;
			}
		}
		else if (UnitIsA( pUnit, UNITTYPE_PASSAGE ))
		{
			puszName = StringTableGetStringByIndex( pObjectData->nString );
		}
		else if (UnitIsA( pUnit, UNITTYPE_HELLRIFT_PORTAL ))
		{
			int nDRLGDefDest = UnitGetStat( pUnit, STATS_WARP_DEST_LEVEL_DEF );
			if(nDRLGDefDest == INVALID_LINK)
			{
				puszName = StringTableGetStringByIndex( pObjectData->nString );
			}
			else
			{
				const DRLG_DEFINITION *ptDRLGDefDest = DRLGDefinitionGet( nDRLGDefDest );
				if(ptDRLGDefDest->nDRLGDisplayNameStringIndex != INVALID_LINK)
				{
					puszName = StringTableGetStringByIndex( ptDRLGDefDest->nDRLGDisplayNameStringIndex );
				}
				else
				{
					const LEVEL_DEFINITION *pLevelDefDest = LevelDefinitionGet(UnitGetLevel(pUnit));
					if(pLevelDefDest && pLevelDefDest->nLevelDisplayNameStringIndex != INVALID_LINK)
					{
						puszName = StringTableGetStringByIndex( pLevelDefDest->nLevelDisplayNameStringIndex );
					}
					else
					{
						puszName = StringTableGetStringByIndex( pObjectData->nString );
					}
				}
			}
		}
		else
		{
		
			int nLevelDefDest = UnitGetStat( pUnit, STATS_WARP_DEST_LEVEL_DEF );
			if (nLevelDefDest == INVALID_LINK &&
				UnitGetWarpToLevelAreaID( pUnit ) == INVALID_ID )
			{
				//static WCHAR *puszBlank = L"";
				//puszName = puszBlank;			
				puszName = StringTableGetStringByIndex( pObjectData->nString );
			}
			else
			{
			
				
				// get subname for depth (if any)
				if (AppIsTugboat())
				{
					//shhhhhhhh Tugboat has secret WARPS! shhhhhh
					if( UnitIsA( pUnit, UNITTYPE_WARP_NO_UI ) == FALSE )
					{
						int nLevelAreaCurrent = LevelGetLevelAreaID( UnitGetLevel( pUnit ) );
						nLevelArea = MYTHOS_LEVELAREAS::GetLevelAreaIDByStairs( pUnit );
						int nLevelDepth = UnitGetStat( pUnit, STATS_WARP_DEST_LEVEL_DEPTH );	
						if( UnitGetWarpToLevelAreaID( pUnit ) != INVALID_ID )
						{
							nLevelArea = UnitGetWarpToLevelAreaID( pUnit );
							nLevelDepth = UnitGetWarpToDepth( pUnit );
							if( nLevelDepth == -1 )
								nLevelDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( nLevelArea ) - 1;

						}

						if( nLevelArea != nLevelAreaCurrent ) //we must be traveling....
							nLevelDefDest = 0;

						if( nLevelArea == INVALID_ID )
						{
							const LEVEL_DEFINITION *ptLevelDefDest = LevelDefinitionGet( nLevelDefDest );
							int nStringID = ptLevelDefDest->nLevelDisplayNameStringIndex;
							puszName = StringTableGetStringByIndex( nStringID );
						}
						else
						{
							// get level area name						
							MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelArea, nLevelDepth, &uszLevelAreaName[0], MAX_BUFFER );
							puszName = &uszLevelAreaName[0];
						}
											
						bHasSubName = LevelDepthGetDisplayName( UnitGetGame( pUnit ), nLevelArea, nLevelDefDest, nLevelDepth, uszSubName, MAX_BUFFER );					
					}
					else
					{
						bHasSubName = FALSE;
					}

				}
				else
				{
					// get level area name
					const LEVEL_DEFINITION *ptLevelDefDest = LevelDefinitionGet( nLevelDefDest );
					puszName = StringTableGetStringByIndex( ptLevelDefDest->nLevelDisplayNameStringIndex );

				}
				
			}
							
		}
				
	}
	else if (UnitIsA( pUnit, UNITTYPE_HEADSTONE ))
	{
		const WCHAR *puszPlayerName = pUnit->szName;
		if (puszPlayerName && puszPlayerName[ 0 ] != 0)
		{
			if (TESTBIT( dwFlags, UNF_VERBOSE_HEADSTONES_BIT ))
			{
				const WCHAR *puszLabel = GlobalStringGet( GS_HEADSTONE_LABEL );
				if (puszLabel && puszLabel[ 0 ])
				{
					PStrCopy( uszBuffer, puszLabel, MAX_BUFFER );
					PStrReplaceToken( uszBuffer, MAX_BUFFER, StringReplacementTokensGet( SR_PLAYER_NAME ), puszPlayerName );
				}
			}
			else
			{
				PStrCopy( uszBuffer, puszPlayerName, MAX_BUFFER );
			}
			puszName = uszBuffer;			
		}
	}
	else
	{
		puszName = StringTableGetStringByIndex( pObjectData->nString );
	}
	ASSERTV_RETFALSE( puszName, "No name for object '%s'", UnitGetDevName( pUnit ) );

	// cat name	
	if (AppIsHellgate() && ObjectIsWarp( pUnit ))
	{
	
		// what color to use (if any)
		FONTCOLOR eColor = (TESTBIT( dwFlags, UNF_EMBED_COLOR_BIT ) ? ObjectWarpGetNameColor(pUnit) : FONTCOLOR_INVALID);

		if (TESTBIT( dwFlags, UNF_VERBOSE_WARPS_BIT ) == TRUE)
		{
		
			// add restricted by trial players (if present)
			if (puszBlockingTrial)
			{
				if (eColor != FONTCOLOR_INVALID)
				{
					UIColorCatString( puszNameBuffer, nBufferSize, eColor, puszBlockingTrial );			
				}
				else
				{
					PStrCat( puszNameBuffer, puszBlockingTrial , nBufferSize );
				}
				// new line
				PStrCat( puszNameBuffer, L"\n", nBufferSize );
			}
			
			// add entry forbidden (if present)
			if (puszBlockingWarp != NULL)
			{
				if (eColor != FONTCOLOR_INVALID)
				{
					UIColorCatString( puszNameBuffer, nBufferSize, eColor, puszBlockingWarp );			
				}
				else
				{
					PStrCat( puszNameBuffer, puszBlockingWarp, nBufferSize );
				}
				// new line
				PStrCat( puszNameBuffer, L"\n", nBufferSize );			
			}
	
		}
		
		// warp name
		if (eColor != FONTCOLOR_INVALID)
		{
			UIColorCatString( puszNameBuffer, nBufferSize, eColor, puszName );
		}
		else
		{
			PStrCat( puszNameBuffer, puszName, nBufferSize );
		}		
			
	}
	else
	{
		PStrCat( puszNameBuffer, puszName, nBufferSize );
	}

	if( AppIsTugboat() )
	{
		// we don't allow this on static levels ( it would say something like: "Dark Cave of Gloom - floor 1" - we just want "Dark Cave of Gloom"
		bHasSubName = bHasSubName & !MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( LevelGetLevelAreaID( UnitGetLevel( pUnit ) ) );
	}
	// cat subname (if any)	
	if (bHasSubName )
	{
		PStrCat( puszNameBuffer, L" - ", nBufferSize );
		PStrCat( puszNameBuffer, uszSubName, nBufferSize );
	}
				
#if !ISVERSION(SERVER_VERSION)
	if( UnitIsA( pUnit, UNITTYPE_WARP ) &&
		!( UnitIsA( pUnit, UNITTYPE_STAIRS_DOWN ) ||
		UnitIsA( pUnit, UNITTYPE_STAIRS_UP ) ) &&
		TESTBIT( dwFlags, UNF_VERBOSE_WARPS_BIT ) == TRUE &&
		nLevelArea != INVALID_ID )
	{
		if( MYTHOS_LEVELAREAS::LevelAreaIsHostile( nLevelArea ) &&
			!MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( nLevelArea ) )
		{
			int nMinLevel = MAX( 1, MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nLevelArea, UIGetControlUnit() ) );
			int nMaxLevel = MAX( 1, MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nLevelArea, UIGetControlUnit() ) );	
			if( nMinLevel != nMaxLevel )
			{
				PStrPrintf( uszYetAnotherBuffer, MAX_BUFFER, L"\nLevel : %d-%d", nMinLevel, nMaxLevel );
			}
			else
			{
				PStrPrintf( uszYetAnotherBuffer, MAX_BUFFER, L"\nLevel : %d", nMinLevel );
			}
			int nColor = MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( nLevelArea, UIGetControlUnit() );;

			UIColorCatString( puszNameBuffer, nBufferSize, (FONTCOLOR)nColor, uszYetAnotherBuffer );
		}
	}
#endif

	return TRUE;				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ObjectGetByCode(
	GAME* game,
	DWORD dwCode)
{
	return ExcelGetLineByCode(game, DATATABLE_OBJECTS, dwCode);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ObjectAllowedAsStartingLocation( 
	UNIT *pUnit,
	DWORD dwWarpFlags)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	if( AppIsTugboat() )
	{
		return TRUE;
	}
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_NEVER_A_START_LOCATION))
	{
		return FALSE;
	}
	return TRUE;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVELID s_ObjectGetOppositeSubLevel( 
	UNIT *pPortal)
{
	ASSERTX_RETINVALID( pPortal, "Expected unit" );
	ASSERTX_RETINVALID( ObjectIsSubLevelWarp( pPortal ), "Expected warp" );	
	
	// where is this portal now
	LEVEL *pLevel = UnitGetLevel( pPortal );
	
	// which sublevel does it go to
	const UNIT_DATA *pObjectData = ObjectGetData( NULL, pPortal );
	if ( ! pObjectData )
		return INVALID_ID;
	int nSubLevelDefDest = pObjectData->nSubLevelDest;
	ASSERTX_RETINVALID( nSubLevelDefDest != INVALID_LINK, "Sub level warp does not have a destination" );
	
	// find sublevel in level ... this assumes that we don't have more than one type of
	return SubLevelFindFirstOfType( pLevel, nSubLevelDefDest );
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_ObjectStartRide( UNIT * ride, UNIT * player, int nCommand )
{
	if ( nCommand == TRAIN_COMMAND_RIDE )
	{
		UnitSetFlag( player, UNITFLAG_ON_TRAIN );
		UnitSetStat( player, STATS_RIDE_UNITID, UnitGetId( ride ) );
	}
	if ( nCommand == TRAIN_COMMAND_MOVE )
	{
		UnitSetStatFloat( ride, STATS_PATH_DISTANCE, 0, 0.0f );
		UnitSetVelocity( ride, TRAIN_START_VELOCITY );
		UnitStepListAdd( UnitGetGame( ride ), ride );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ObjectEndRide( UNIT * player )
{
	UnitClearFlag( player, UNITFLAG_ON_TRAIN );
	UnitSetStat( player, STATS_RIDE_UNITID, INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ObjectStartRide( UNIT * ride, UNIT * player )
{
	UnitSetFlag( player, UNITFLAG_ON_TRAIN );
	UnitSetStat( player, STATS_RIDE_UNITID, UnitGetId( ride ) );
	// start it message
	MSG_SCMD_RIDE_START tMessage;
	tMessage.idUnit = UnitGetId( ride );
	tMessage.bCommand = (BYTE)TRAIN_COMMAND_RIDE;
	GAMECLIENTID idClient = UnitGetClientId( player );
	s_SendUnitMessageToClient( ride, idClient, SCMD_RIDE_START, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ObjectStartMove( UNIT * ride, UNIT * player )
{
	// already going?
	if ( UnitGetVelocity( ride ) != 0.0f )
	{
		return;
	}
	// already moved before?
	if ( UnitGetStatFloat( ride, STATS_PATH_DISTANCE ) != 0.0f )
	{
		return;
	}
	UnitSetVelocity( ride, TRAIN_START_VELOCITY );
	UnitStepListAdd( UnitGetGame( ride ), ride );
	// sound
	GAME * pGame = UnitGetGame( ride );
	if ( IS_SERVER( pGame ) && UnitHasState( pGame, ride, STATE_TRAIN_IDLE ) )
	{
		s_StateClear( ride, UnitGetId( ride ), STATE_TRAIN_IDLE, 0 );
		s_StateSet( ride, ride, STATE_TRAIN_STARTUP, 0 );
	}
	// move it message
	MSG_SCMD_RIDE_START tMessage;
	tMessage.idUnit = UnitGetId( ride );
	tMessage.bCommand = (BYTE)TRAIN_COMMAND_MOVE;
	GAMECLIENTID idClient = UnitGetClientId( player );
	s_SendUnitMessageToClient( ride, idClient, SCMD_RIDE_START, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ObjectEndRide( UNIT * player )
{
	UnitClearFlag( player, UNITFLAG_ON_TRAIN );
	UnitSetStat( player, STATS_RIDE_UNITID, INVALID_ID );
	// end it message
	MSG_SCMD_RIDE_END tMessage;
	GAMECLIENTID idClient = UnitGetClientId( player );
	s_SendUnitMessageToClient( player, idClient, SCMD_RIDE_END, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_ObjectRide( UNIT * player, VECTOR & vRideDelta, float fTimeDelta )
{
	ASSERT_RETURN( player );
	ASSERT_RETURN( UnitTestFlag( player, UNITFLAG_ON_TRAIN ) );

	UNITID id = UnitGetStat( player, STATS_RIDE_UNITID );
	ASSERT_RETURN( id != INVALID_ID );

	GAME * game = UnitGetGame( player );
	UNIT * ride = UnitGetById( game, id );
	ASSERT_RETURN( ride );

	float velocity = UnitGetVelocity( ride );
	vRideDelta = ride->vFaceDirection;
	vRideDelta *= velocity * fTimeDelta;

	// move ride
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ObjectDoorOperate(
	UNIT *pUnit,
	DOOR_OPERATE eOperate)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );

	// which mode will we set
	UNITMODE eMode = MODE_OPEN;
	if (eOperate == DO_CLOSE)
	{
		eMode = MODE_CLOSE;
	}

	// get mode duration	
	BOOL bHasMode = FALSE;
	float fDuration = UnitGetModeDuration( pGame, pUnit, eMode, bHasMode );
	if (bHasMode)
	{	
		s_UnitSetMode( pUnit, eMode, 0, fDuration, TRUE );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sTrainSideKill( UNIT * unit, UNIT * train, VECTOR * v1, VECTOR * v2 )
{
	if ( IsUnitDeadOrDying( unit ) )
		return;

	float distance;
	if ( DistancePointLineXY( &unit->vPosition, v1, v2, &distance ) )
	{
		if ( ( distance > 0.0f ) && ( distance < UnitGetCollisionRadius( unit ) ) )
		{
			UnitDie( unit, train );
		}
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_ObjectTrainCheckKill(
	UNIT * pTrain )
{
	ASSERT_RETURN( pTrain );
	ASSERT_RETURN( IS_SERVER( UnitGetGame( pTrain ) ) );

	if ( UnitGetVelocity( pTrain ) < 3.0f )
		return;

	ROOM * pRoom = UnitGetRoom( pTrain );
	// check to see if spawned
	VECTOR upper_left, upper_right, lower_left, lower_right;
	sTrainTransform( pTrain, upper_left, upper_right, lower_left, lower_right );
	// check all the units in the room
	TARGET_TYPE eTypeList[] = { TARGET_GOOD, TARGET_BAD };
	for ( int tt = 0; tt < 2; tt++ )
	{
		TARGET_TYPE eType = eTypeList[tt];
		for ( UNIT * test = pRoom->tUnitList.ppFirst[eType]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitIsA( test, UNITTYPE_PLAYER ) || UnitIsA( test, UNITTYPE_MONSTER ) )
			{
				// check if sides hitting
				sTrainSideKill( test, pTrain, &upper_left, &lower_left );
				sTrainSideKill( test, pTrain, &lower_left, &lower_right );
				sTrainSideKill( test, pTrain, &lower_right, &upper_right );
				sTrainSideKill( test, pTrain, &upper_right, &upper_left );
				// check if player is inside
				if ( ( test->vPosition.x >= upper_left.x ) &&
					 ( test->vPosition.x <= lower_right.x ) &&
					 ( test->vPosition.y >= upper_left.y ) &&
					 ( test->vPosition.y <= lower_right.y ) )
				{
					UnitDie( test, pTrain );
				}
			}
		}
	}
}
