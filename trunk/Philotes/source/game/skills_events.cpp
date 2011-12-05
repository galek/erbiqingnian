//----------------------------------------------------------------------------
// skills_events.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
// NOTES: After merge with Tugboat, the function RoomPathNodeGetWorldPosition was changed to RoomPathNodeGetExactWorldPosition.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "units.h"
#include "skills.h"
#include "skills_priv.h"
#include "excel_private.h"
#include "e_model.h"
#include "filepaths.h"
#include "room_path.h"
#include "states.h"
#include "objects.h"
#include "monsters.h"
#include "ai.h"
#include "ai_priv.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "combat.h"
#include "missiles.h"
#include "c_particles.h"
#include "gameunits.h"
#include "c_appearance.h"
#include "uix.h"
#include "uix_priv.h"
#include "c_message.h"
#include "s_message.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "c_camera.h"
#include "script.h"
#include "teams.h"
#include "pets.h"
#include "inventory.h"
#include "console.h"
#include "level.h"
#include "district.h"
#include "treasure.h"
#include "items.h"
#include "player.h"
#include "room_path.h"
#include "s_townportal.h"
#include "quests.h"
#include "s_quests.h"
#include "s_questgames.h"
#include "s_passage.h"
#include "wardrobe.h"
#include "room_layout.h"
#include "s_recipe.h"
#include "cube.h"
#include "dictionary.h"
#include "dungeon.h"
#include "bookmarks.h"
#include "hotkeys.h"
#include "picker.h"
#include "s_itemupgrade.h"
#include "warp.h"
#include "achievements.h"
#include "s_crafting.h"
#include "c_ui.h"
#include "unittag.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillEventSafeStopSkill(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	BOOL bSend = FALSE,
	BOOL bForceSendToAll = FALSE)
{
	if(SkillIsOn(tData.pUnit, tData.nSkill, tData.idWeapon) && tData.pSkillData->nStartFunc != SKILL_STARTFUNCTION_NOMODES)
	{
		SkillStop( tData.pGame, tData.pUnit, tData.nSkill, tData.idWeapon, bSend, bForceSendToAll);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillEventDoAddAttachment(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	UNIT * pUnitToAttach)
{
	int nModelId = c_UnitGetModelId( pUnitToAttach );

	BOOL bForceNew = ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FORCE_NEW ) != 0;

	for ( int i = 0; i < 2; i++)
	{
		ATTACHMENT_DEFINITION tDef;
		MemoryCopy( &tDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof(ATTACHMENT_DEFINITION) );
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_ADD_TO_CENTER )
		{
			int nModelDefinition = e_ModelGetDefinition( c_UnitGetModelId(pUnitToAttach) );
			if ( nModelDefinition != INVALID_ID )
			{
				const BOUNDING_BOX * pBoundingBox = e_ModelDefinitionGetBoundingBox( nModelDefinition, LOD_ANY );
				if ( pBoundingBox )
					tDef.vPosition = BoundingBoxGetCenter( pBoundingBox );
			}
		}
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
		{
			tDef.vPosition = UnitGetPosition(tData.pUnit) - UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
		}

		c_AttachmentSetWeaponIndex ( tData.pGame, tDef, pUnitToAttach, tData.idWeapon );

		int nAttachment = c_ModelAttachmentAdd( nModelId, tDef, FILE_PATH_DATA, bForceNew, ATTACHMENT_OWNER_SKILL, tData.nSkill, INVALID_ID );
		if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FLOAT)
		{
			c_ModelAttachmentFloat( nModelId, nAttachment );
		}

		if ( i > 0 )
			break;

		int nFirstPersonModel = c_UnitGetModelIdFirstPerson( pUnitToAttach );
		if ( nFirstPersonModel == INVALID_ID )
			break;
		nModelId = nModelId == nFirstPersonModel ? c_UnitGetModelIdThirdPerson( pUnitToAttach ) : nFirstPersonModel;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAddAttachment(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT(tData.pGame) );

	UNIT * pUnitToAttach = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToAttach = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnitToAttach)
		return TRUE;

	sSkillEventDoAddAttachment(tData, pUnitToAttach);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAddAttachmentToSummonedInventoryLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT(tData.pGame) );

	UNIT * pContainer = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pContainer = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pContainer )
		return TRUE;

	for(int i=0; i<MAX_SUMMONED_INVENTORY_LOCATIONS; i++)
	{
		int nSummonedInventoryLocation = tData.pSkillData->pnSummonedInventoryLocations[i];
		if(nSummonedInventoryLocation < 0)
			continue;

		UNIT * pItem = UnitInventoryLocationIterate(pContainer, nSummonedInventoryLocation, NULL);
		while(pItem)
		{
			sSkillEventDoAddAttachment(tData, pItem);

			pItem = UnitInventoryLocationIterate(pContainer, nSummonedInventoryLocation, pItem);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRemoveAttachmentFromSummonedInventoryLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT(tData.pGame) );

	UNIT * pContainer = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pContainer = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pContainer )
		return TRUE;

	for(int i=0; i<MAX_SUMMONED_INVENTORY_LOCATIONS; i++)
	{
		int nSummonedInventoryLocation = tData.pSkillData->pnSummonedInventoryLocations[i];
		if(nSummonedInventoryLocation < 0)
			continue;

		UNIT * pItem = UnitInventoryLocationIterate(pContainer, nSummonedInventoryLocation, NULL);
		while(pItem)
		{
			int nModelId = c_UnitGetModelId( pItem );

			ATTACHMENT_DEFINITION tDef;
			MemoryCopy( &tDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof(ATTACHMENT_DEFINITION) );
			c_ModelAttachmentRemove( nModelId, tDef, FILE_PATH_DATA, 0 );

			pItem = UnitInventoryLocationIterate(pContainer, nSummonedInventoryLocation, pItem);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFloatAttachment(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT(tData.pGame) );

	int nModelId = c_UnitGetModelId( tData.pUnit );

	BOOL bForceNew = ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FORCE_NEW ) != 0;
	ATTACHMENT_DEFINITION tDef;
	MemoryCopy( &tDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof(ATTACHMENT_DEFINITION) );
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
	{
		tDef.vPosition = UnitGetPosition(tData.pUnit) - UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
	}
	int nAttachment = c_ModelAttachmentAdd( nModelId, tDef, FILE_PATH_DATA, bForceNew, ATTACHMENT_OWNER_SKILL, tData.nSkill, INVALID_ID );
	c_ModelAttachmentFloat( nModelId, nAttachment );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAddAttachmentsBoneWeights(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT(tData.pGame) );

	int nModelId = c_UnitGetModelId( tData.pUnit );
	BOOL bForceNew = ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FORCE_NEW ) != 0;
	BOOL bFloat = ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FLOAT ) != 0;

	c_ModelAttachmentAddAtBoneWeight(nModelId, tData.pSkillEvent->tAttachmentDef, bForceNew, ATTACHMENT_OWNER_SKILL, tData.nSkill, bFloat);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sFindTeleportLocation(
	UNIT *pUnit,
	UNIT *pAnchor,
	float flMinTeleportDistance,
	float flMaxTeleportDistance,
	FREE_PATH_NODE *pFreePathNode,
	DWORD dwFreePathNodeOptions)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( pFreePathNode, "Invalid param" );
	GAME *pGame = UnitGetGame( pUnit );
	VECTOR vAnchorDirection = UnitGetFaceDirection( pAnchor, FALSE );

	// where we will hold the path nodes
	const int MAX_PATH_NODES = 32;
	FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

	// what's the distance between path nodes
	ROOM *pRoom = UnitGetRoom( pUnit );
	ROOM_PATH_NODE_DEFINITION *pPathNodeDef = pRoom->pPathDef;
	float flDistanceBetweenNodes = pPathNodeDef->fDiagDistBetweenNodes;

	// setup spec
	NEAREST_NODE_SPEC tSpec;
	tSpec.fMinDistance = flMaxTeleportDistance;
	tSpec.fMaxDistance = flMaxTeleportDistance + (flDistanceBetweenNodes * 3.0f);
	tSpec.vFaceDirection = vAnchorDirection;
	tSpec.dwFlags = dwFreePathNodeOptions;
	int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pAnchor), UnitGetPosition(pAnchor), MAX_PATH_NODES, tFreePathNodes, &tSpec);

	// if no nodes, forget it
	if (nNumNodes == 0)
	{
		return FALSE;
	}

	int nPick = INVALID_ID;
	if(flMinTeleportDistance > 0.0f)
	{
		VECTOR vCheckPosition = UnitGetPosition(pUnit);
		PickerStart(pGame, picker);
		for(int i=0; i<nNumNodes; i++)
		{
			VECTOR vNodePosition = RoomPathNodeGetExactWorldPosition(pGame, tFreePathNodes[i].pRoom, tFreePathNodes[i].pNode);
			if(VectorDistanceSquared(vCheckPosition, vNodePosition) > (flMinTeleportDistance*flMinTeleportDistance))
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
			}
		}
		nPick = PickerChoose(pGame);
	}

	if(nPick < 0)
	{
		// pick one of the nodes randomly
		nPick = RandGetNum( pGame, nNumNodes - 1 );
	}
	*pFreePathNode = tFreePathNodes[ nPick ];

	return TRUE;

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sDoTeleport(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ESTATE eStateTeleport = (ESTATE)tEventData.m_Data1;
	REF(eStateTeleport);
	ROOMID idRoomDest = (ROOMID)tEventData.m_Data2;
	int nDestRoomNodeIndex = (int)tEventData.m_Data3;

	// do nothing if killed
	if (IsUnitDeadOrDying( pUnit ))
	{
		return TRUE;
	}

	// get the room and path node
	ROOM *pRoomDest = RoomGetByID( pGame, idRoomDest );
	ROOM_PATH_NODE *pRoomPathNode = RoomGetRoomPathNode( pRoomDest, nDestRoomNodeIndex );

	// get the destination location
	VECTOR vDestination = RoomPathNodeGetExactWorldPosition( pGame, pRoomDest, pRoomPathNode );

	// warp to location
	UnitWarp(
		pUnit,
		pRoomDest,
		vDestination,
		UnitGetFaceDirection( pUnit, FALSE ),
		UnitGetUpDirection( pUnit ),
		0,
		TRUE);

	return TRUE;

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sDestroyUnit(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	UnitFree( pUnit, UFF_SEND_TO_CLIENTS);
	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sStartTeleport(
	UNIT *pUnit,
	UNIT *pAnchor,
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = UnitGetGame( pUnit );
	int nSkill = tData.nSkill;

	// get params
	float flMaxTeleportDistance = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float flTeleportDelayInSeconds = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	int nTeleportDelayInTicks = (int)(flTeleportDelayInSeconds * GAME_TICKS_PER_SECOND_FLOAT);
	BOOL bFrontOnly = SkillEventGetParamBool( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	ESTATE eStateTeleport = (ESTATE)ExcelGetLineByStringIndex( pGame, DATATABLE_STATES, tData.pSkillEvent->tAttachmentDef.pszAttached );
	float flMinTeleportDistance = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );

	// set the teleportation state
	int nStateDurationInTicks = nTeleportDelayInTicks + (nTeleportDelayInTicks / 2);
	s_StateSet( pUnit, pUnit, eStateTeleport, nStateDurationInTicks, nSkill );

	// setup flags
	DWORD dwFreePathNodeFlags = 0;
	SETBIT( dwFreePathNodeFlags, NPN_IN_FRONT_ONLY, bFrontOnly );
	SETBIT( dwFreePathNodeFlags, NPN_EMPTY_OUTPUT_IS_OKAY );

	// find a spot to teleport to
	FREE_PATH_NODE tFreePathNode;
	if (s_sFindTeleportLocation(
			pUnit,
			pAnchor,
			flMinTeleportDistance,
			flMaxTeleportDistance,
			&tFreePathNode,
			dwFreePathNodeFlags ) == FALSE)
	{
		return;
	}

	// create an effect at the destination location on a temporary dummy unit
	VECTOR vDirection = cgvXAxis;
	OBJECT_SPEC tSpec;
	tSpec.nClass = GlobalIndexGet( GI_OBJECT_STATE_OBJECT );
	tSpec.pRoom = tFreePathNode.pRoom;
	tSpec.vPosition = RoomPathNodeGetExactWorldPosition( pGame, tFreePathNode.pRoom, tFreePathNode.pNode );
	tSpec.pvFaceDirection = &vDirection;
	UNIT *pStateObject = s_ObjectSpawn( pGame, tSpec );
	if (pStateObject != NULL)
	{
		UnitRegisterEventTimed( pStateObject, s_sDestroyUnit, EVENT_DATA(), nTeleportDelayInTicks );

		// if there is an associated state with the teleport state on the main unit, we will use
		// that associated state on the dummy object, otherwise we will set the same state
		// on the dummy object
		ESTATE eStateObject = eStateTeleport;
		const STATE_DATA *pStateData = StateGetData( pGame, eStateTeleport );
		if (pStateData && pStateData->nStateAssoc[ 0 ] != INVALID_LINK)
		{
			eStateObject = (ESTATE)pStateData->nStateAssoc[ 0 ];
		}
		s_StateSet( pStateObject, pStateObject, eStateObject, nTeleportDelayInTicks );

	}

	// setup event data to teleport after a small delay, we do this so you can see an effect
	// first and then the action cause it looks cool
	EVENT_DATA tEventData;
	tEventData.m_Data1 = eStateTeleport;
	tEventData.m_Data2 = RoomGetId( tFreePathNode.pRoom );
	tEventData.m_Data3 = tFreePathNode.pNode->nIndex;

	// trigger teleport to happen in a second
	UnitRegisterEventTimed( pUnit, s_sDoTeleport, &tEventData, nTeleportDelayInTicks );

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventTeleport(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;

	// the target of this skill will be our "anchor"
	UNIT *pAnchor = SkillGetTarget( pUnit, nSkill, INVALID_ID );
	if (pAnchor)
	{

		// start the teleport
		sStartTeleport( pUnit, pAnchor, tData );

	}

	return TRUE;

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventTeleportAroundOwner(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	UNIT *pAnchor = UnitGetUltimateOwner( pUnit );

	if (pAnchor != NULL && pAnchor != pUnit)
	{
		sStartTeleport( pUnit, pAnchor, tData );
	}

	return TRUE;

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnBasic(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nNumToSpawn = SkillEventGetParamInt( pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nLowerLevelBy = SkillEventGetParamInt( pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );

	if (IS_SERVER( pGame ))
	{
		// mark the mother monster as "no fly corpse" so we are certain that the
		// corpse is synchronized on the client and the server

		// spawn the basic clones
		for (int i = 0; i < nNumToSpawn; ++i)
		{
			// setup monster spec
			MONSTER_SPEC tSpec;
			tSpec.nClass = UnitGetClass( pUnit );
			tSpec.nExperienceLevel = min( UnitGetExperienceLevel( pUnit ) - nLowerLevelBy, 1 );
			tSpec.nMonsterQuality = INVALID_LINK;
			tSpec.nTeam = UnitGetTeam( pUnit );
			tSpec.pRoom = UnitGetRoom( pUnit );
			tSpec.vPosition = UnitGetPosition( pUnit );
			tSpec.flScale = UnitGetScale( pUnit ) * 0.75f;

			// spawn monster
			s_MonsterSpawn( pGame, tSpec );
		}
	}

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventCascade(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	const int NUM_CLONES_EACH_CASCADE = 2;
	int nNumCascadeLevels = UnitGetStat( pUnit, STATS_CASCADE_LEVEL );
	int nNumCascadeUsed = UnitGetStat( pUnit, STATS_CASCADE_USED );

	// create more monsters if there is a cascade level available
	if (IS_SERVER( pGame ))
	{

		if (nNumCascadeLevels > 0)
		{
			if (nNumCascadeUsed != nNumCascadeLevels)
			{

				// this monster does not make treasure now
				UnitSetStat( pUnit, STATS_TREASURE_CLASS, INVALID_LINK );

				// spawn the cascaded clones
				for (int i = 0; i < NUM_CLONES_EACH_CASCADE; ++i)
				{

					// setup monster spec
					MONSTER_SPEC tSpec;
					tSpec.nClass = UnitGetClass( pUnit );
					tSpec.nExperienceLevel = UnitGetExperienceLevel( pUnit );
					if( AppIsHellgate() )
					{
						tSpec.nMonsterQuality = MonsterGetQuality( pUnit );
					}
					else
					{
						tSpec.nMonsterQuality = INVALID_ID;//MonsterChampionMinionGetQuality( pGame, MonsterGetQuality( pUnit ) );
					}
					tSpec.nTeam = UnitGetTeam( pUnit );
					tSpec.pRoom = UnitGetRoom( pUnit );
					tSpec.vPosition = UnitGetPosition( pUnit );
					tSpec.flScale = UnitGetScale( pUnit ) * 0.75f;
					tSpec.dwFlags = MSF_ONLY_AFFIX_SPECIFIED;

					// give the cascaded monsters the same affixes
					int nAffix[ MAX_MONSTER_AFFIX ];
					int nNumAffix = MonsterGetAffixes( pUnit, nAffix, MAX_MONSTER_AFFIX );
					for (int i = 0; i < nNumAffix; ++i)
					{
						tSpec.nAffix[ i ] = nAffix[ i ];
					}

					// spawn monster
					UNIT *pMonster = s_MonsterSpawn( pGame, tSpec );
					if (pMonster)
					{

						// change the cascade level on each of the clones
						UnitSetStat( pMonster, STATS_CASCADE_USED, nNumCascadeUsed + 1 );

					}

				}

			}

		}

	}

	return TRUE;


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRemoveAttachment(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT( IS_CLIENT(tData.pGame) );

	UNIT * pUnitToAttach = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToAttach = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnitToAttach)
		return TRUE;

	int nModelId = c_UnitGetModelId( pUnitToAttach );

	for ( int i = 0; i < 2; i++ )
	{
		ATTACHMENT_DEFINITION tDef;
		MemoryCopy( &tDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof(ATTACHMENT_DEFINITION) );
		c_ModelAttachmentRemove( nModelId, tDef, FILE_PATH_DATA, 0 );

		if ( i == 0 && tData.pUnit != GameGetControlUnit( tData.pGame ) )
			break;
		int nFirstPersonModelId = c_UnitGetModelIdFirstPerson( pUnitToAttach );
		nModelId = nModelId == nFirstPersonModelId ? c_UnitGetModelIdThirdPerson( pUnitToAttach ) : nFirstPersonModelId;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRemoveAllAttachments(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT( IS_CLIENT(tData.pGame) );

	UNIT * pUnitToAttach = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToAttach = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnitToAttach)
		return TRUE;

	int nModelId = c_UnitGetModelId( pUnitToAttach );

	c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_SKILL, tData.nSkill );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventMeleeAttack(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	DWORD dwTargetFlags = 0;
	UNIT * pTarget = NULL;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_AI_TARGET )
	{
		DWORD dwAIGetTargetFlags = 0;
		SETBIT(dwAIGetTargetFlags, AIGTF_DONT_DO_TARGET_QUERY_BIT);
		pTarget = AI_GetTarget( tData.pGame, tData.pUnit, tData.nSkill, NULL, dwAIGetTargetFlags );
	}
	BOOL bCanFindNewTarget = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 ) == 0;

	if( AppIsTugboat() && IS_CLIENT( tData.pGame ) )
	{
		bCanFindNewTarget = FALSE;
	}
	if ( !pTarget )
	{
		int nTargetIndex = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
		nTargetIndex = MAX( 0, nTargetIndex );
		pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, nTargetIndex, &dwTargetFlags );
	}
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

	BOOL bForceIsInRange = (dwTargetFlags & SKILL_TARGET_FLAG_IN_MELEE_RANGE_ON_START) != 0;

	if ( UnitIsPlayer( tData.pUnit ) )
	{
		if ( pTarget && ! bForceIsInRange )
		{
			float fRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeapon, TRUE );
			if ( !UnitCanMeleeAttackUnit( tData.pUnit, pTarget, pWeapon, 0, fRange, FALSE, tData.pSkillEvent, FALSE ) )
				pTarget = NULL;
		}
		if ( ! pTarget && bCanFindNewTarget )
		{ // this might find another target if the current one won't work.
			VECTOR vTargetPos = VECTOR(0.0f);
			SkillFindTarget( tData.pGame, tData.pUnit, tData.nSkill, pWeapon, &pTarget, vTargetPos );
		}
	}
	if ( ! pTarget )
	{
		return TRUE;
	}

	BOOL bImpactOffset = FALSE;
	VECTOR vOffset(0);
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET )
	{
		bImpactOffset = TRUE;
		MATRIX mWorld;
		UnitGetWorldMatrix( tData.pUnit, mWorld );
		MatrixMultiplyNormal( &vOffset, &tData.pSkillEvent->tAttachmentDef.vPosition, &mWorld );
		VectorNormalize( vOffset );
	}

	if ( !(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_OFFHAND_WEAPON) )
	{
		float fRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeapon, TRUE );
		UnitMeleeAttackUnit(tData.pUnit, pTarget, pWeapon, NULL, tData.nSkill, 0, fRange, bForceIsInRange,
			tData.pSkillEvent, tData.pSkillData->fDamageMultiplier, tData.pSkillData->nDamageTypeOverride );
	}

	const SKILL_DATA * pSkillData = SkillGetData( tData.pGame, tData.nSkill );
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_ALL_WEAPONS ) ||
		(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_OFFHAND_WEAPON))
	{
		UNIT * pWeaponOther = SkillGetLeftHandWeapon( tData.pGame, tData.pUnit, tData.nSkill, pWeapon );
		if ( pWeaponOther )
		{
			float fRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeaponOther, TRUE );
			UnitMeleeAttackUnit( tData.pUnit, pTarget, pWeaponOther, pWeapon, tData.nSkill, 0, fRange, bForceIsInRange,
				tData.pSkillEvent, tData.pSkillData->fDamageMultiplier, tData.pSkillData->nDamageTypeOverride, MELEE_IMPACT_FORCE / 10.0f );
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetWeaponsForDamage(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA * pSkillData,
	UNITID idWeaponInEvent,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ] )
{
	ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_COMBINE_WEAPON_DAMAGE ) )
	{
		int nLeftIndex = 1;
		pWeapons[ 0 ] = WardrobeGetWeapon( pGame, UnitGetWardrobe( pUnit ), WEAPON_INDEX_RIGHT_HAND );
		if ( ! pWeapons[ 0 ] || !UnitIsA( pWeapons[ 0 ], pSkillData->nRequiredWeaponUnittype ) )
		{
			nLeftIndex = 0;
			pWeapons[ 1 ] = NULL;
		}
		pWeapons[ nLeftIndex ] = WardrobeGetWeapon( pGame, UnitGetWardrobe( pUnit ), WEAPON_INDEX_LEFT_HAND );

		if ( pWeapons[ nLeftIndex ] && ! UnitIsA( pWeapons[ nLeftIndex ], pSkillData->nRequiredWeaponUnittype ) )
			pWeapons[ nLeftIndex ] = NULL;
	}
	else
	{
		if(UnitIsA(pUnit, UNITTYPE_MISSILE) && idWeaponInEvent == INVALID_ID)
		{
			UnitGetWeaponsTag( pUnit, TAG_SELECTOR_MISSILE_SOURCE, pWeapons );
		}
		else
		{
			pWeapons[ 0 ] = UnitGetById( pGame, idWeaponInEvent );
			pWeapons[ 1 ] = NULL;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetWeaponsForDamage(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ] )
{
	sGetWeaponsForDamage( tData.pGame, tData.pUnit, tData.nSkill, tData.pSkillData, tData.idWeapon, pWeapons );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventDoAttackLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	DWORD dwFlags)
{
	ASSERT_RETFALSE( IS_SERVER(tData.pGame) );

	// get the location to attack
	const SKILL_DATA *pSkillData = SkillGetData( tData.pGame, tData.nSkill );
	VECTOR vSource;
	if (SkillDataTestFlag( pSkillData, SKILL_FLAG_TARGET_POS_IN_STAT ))
	{
		vSource = UnitGetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0 );
	}
	else if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET )
	{
		if( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE )
		{
			vSource = UnitGetPosition(tData.pUnit) + tData.pSkillEvent->tAttachmentDef.vPosition;
		}
		else
		{
			MATRIX mWorld;
			UnitGetWorldMatrix( tData.pUnit, mWorld );
			MatrixMultiply(&vSource, &tData.pSkillEvent->tAttachmentDef.vPosition, &mWorld );
		}
	}
	else if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
		if(pTarget)
		{
			vSource = UnitGetPosition(pTarget);
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		vSource = UnitGetCenter(tData.pUnit);
	}

#ifdef HAVOK_ENABLED
	HAVOK_IMPACT tImpact;
	float flForce = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	HavokImpactInit( tData.pGame, tImpact, flForce, vSource, NULL );
#endif
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	sGetWeaponsForDamage( tData, pWeapons );

	int nFieldMissile = INVALID_ID;
	if ( pSkillData->nFieldMissile != INVALID_ID)
	{
		nFieldMissile = UnitGetStat( tData.pUnit, STATS_FIELD_OVERRIDE );
		UnitSetStat( tData.pUnit, STATS_FIELD_OVERRIDE, pSkillData->nFieldMissile );
	}

	s_UnitAttackLocation(tData.pUnit,
		pWeapons,
		0,
		UnitGetRoom(tData.pUnit),
		vSource,
#ifdef HAVOK_ENABLED
		tImpact,
#endif
		dwFlags);

	if (nFieldMissile != INVALID_ID)
	{
		UnitSetStat( tData.pUnit, STATS_FIELD_OVERRIDE, nFieldMissile );
	}

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventAttackLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	DWORD dwFlags = 0;
	SETBIT(dwFlags, UAU_MELEE_BIT);
	return sSkillEventDoAttackLocation(tData, dwFlags);
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventAttackLocationShields(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	DWORD dwFlags = 0;
	SETBIT(dwFlags, UAU_MELEE_BIT);
	SETBIT(dwFlags, UAU_SHIELD_DAMAGE_ONLY_BIT);
	return sSkillEventDoAttackLocation(tData, dwFlags);
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventAttackTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;
	BOOL bIsInRange = FALSE;
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	if ( ! SkillIsValidTarget( tData.pGame, tData.pUnit, pTarget, pWeapon,
		tData.nSkill, FALSE, &bIsInRange ) || ! bIsInRange )
	{
		return FALSE;
	}

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	ZeroMemory( pWeapons, sizeof( UNIT * ) * MAX_WEAPONS_PER_UNIT );
	pWeapons[ 0 ] = pWeapon;

#ifdef HAVOK_ENABLED
	float fParam = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	HAVOK_IMPACT tImpact;
	HavokImpactInit( tData.pGame, tImpact, fParam, UnitGetCenter( tData.pUnit ), NULL );
#endif
	s_UnitAttackUnit( tData.pUnit,
					  pTarget,
					  pWeapons,
					  0
#ifdef HAVOK_ENABLED
					 ,&tImpact
#endif
					  );

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sHandleDoSkillOnKilledDelayed(
	GAME * pGame,
	UNIT * pUnit,
	const EVENT_DATA & event_data)
{
	UNITID idWeapon		= (UNITID)event_data.m_Data1;
	int nSkill			= (int)event_data.m_Data2;
	int nTargetSkill	= (int)event_data.m_Data3;
	int skillLevel = (int)event_data.m_Data4;
	UNITID idDefender = (int)event_data.m_Data5;

	UNIT * pDefender = UnitGetById(pGame, idDefender);

	ASSERT_RETFALSE ( nSkill != INVALID_ID );
	ASSERT_RETFALSE ( pUnit );
	UNIT * pSkillTarget = SkillGetTarget( pUnit, nTargetSkill, idWeapon );
	if ( pSkillTarget == pDefender && SkillIsValidTarget( pGame, pUnit, pSkillTarget, UnitGetById( pGame, idWeapon ), nSkill, FALSE, NULL ))
	{ // only do this if you are targeting this unit
		DWORD dwSeed = SkillGetSeed(pGame, pUnit, UnitGetById(pGame, idWeapon), nSkill);
		DWORD dwFlags = SKILL_START_FLAG_DONT_RETARGET | SKILL_START_FLAG_DONT_SEND_TO_SERVER;
		if ( IS_SERVER( pGame ) )
			dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
		SkillStartRequest(pGame, pUnit, nSkill, UnitGetId( pDefender ), VECTOR(0), dwFlags, dwSeed, skillLevel);
		SkillSetTarget( pUnit, nTargetSkill, idWeapon, INVALID_ID, 0 );
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleDoSkillOnKilled(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pDefender,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	ASSERT_RETURN(pHandlerData);
	EVENT_DATA tEventData(pHandlerData->m_Data1, pHandlerData->m_Data2, pHandlerData->m_Data3, pHandlerData->m_Data4, UnitGetId(pDefender));
	UnitRegisterEventTimed(pUnit, sHandleDoSkillOnKilledDelayed, tEventData, 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleSkillStoppedRemoveHandler(
	GAME* pGame,
	UNIT* pUnit,
	UNIT *,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	UNITID idTarget		= (UNITID)pHandlerData->m_Data1;
	UNIT_EVENT eHandlerEvent= (UNIT_EVENT)pHandlerData->m_Data2;
	DWORD dwHandlerId	= (DWORD)pHandlerData->m_Data3;
	UNIT * pTarget		= UnitGetById( pGame, idTarget );
	int nSkill			= (int)pHandlerData->m_Data4;

	EVENT_DATA * pTriggerData = (EVENT_DATA *) pData;
	if ( pTriggerData )
	{
		int nTriggerSkill = (int)pTriggerData->m_Data1;
		if ( nTriggerSkill != nSkill )
			return;
	}
	if ( pTarget )
		UnitUnregisterEventHandler( pGame, pTarget, eHandlerEvent, dwHandlerId );
	// remove this handler
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_SKILL_STOPPED, dwId );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillRegisterUnitEventWithRemove(
	GAME * pGame,
	UNIT * pUnit,
	UNIT_EVENT UNIT_EVENT,
	int nSkill,
	FP_UNIT_EVENT_HANDLER fpHandler,
	const EVENT_DATA & event_data,
	FP_UNIT_EVENT_HANDLER fpSkillStopHandler = sHandleSkillStoppedRemoveHandler)
{
	DWORD dwId;
	UnitRegisterEventHandlerRet(dwId, pGame, pUnit, UNIT_EVENT, fpHandler, event_data);
	UnitRegisterEventHandler( pGame, pUnit, EVENT_SKILL_STOPPED, fpSkillStopHandler, EVENT_DATA( UnitGetId( pUnit ), UNIT_EVENT, dwId, nSkill ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoSkillOnKilled(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	int nSkillToDo = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

	const SKILL_DATA * pSkillToDoData = SkillGetData( tData.pGame, nSkillToDo );
	if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_IS_MELEE ) &&
		! UnitCanMeleeAttackUnit( tData.pUnit, pTarget, pWeapon, 0, SkillGetRange( tData.pUnit, tData.nSkill, pWeapon ),
			FALSE, tData.pSkillEvent, SkillDataTestFlag( pSkillToDoData, SKILL_FLAG_TARGET_DEAD) ) )
		pTarget = NULL;

	if ( pTarget )
	{
		sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_DIDKILL, tData.nSkill, sHandleDoSkillOnKilled, EVENT_DATA(tData.idWeapon, nSkillToDo, tData.nSkill, tData.nSkillLevel));
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillEventDataGetSkillLevel(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	if (tData.nSkillLevel > 0)
	{
		return tData.nSkillLevel;
	}
	return SkillGetLevel(tData.pUnit, tData.nSkill);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetStateDuration(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fParam = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nSkillEventDuration = (int)(fParam * GAME_TICKS_PER_SECOND_FLOAT);

	if(nSkillEventDuration <= 0)
	{
		int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
		int nDuration = SkillDataGetStateDuration(tData.pGame, tData.pUnit, tData.nSkill, nSkillLevel, TRUE);
		return MAX(nDuration, 0);
	}

	return MAX(nSkillEventDuration, 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Marsh - the difference on this function versus sSkillEventDoSkillOnKilled is that this function will only get called when a player strikes the creature. The onKilled
//above puts an event inside the unit for when it gets killed, at which point you don't
//konw who killed the monster
static BOOL sSkillEventDoSkillOnKill(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	if( IS_CLIENT( tData.pGame ) )
		return FALSE;
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	int nSkillToDo = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if( nSkillToDo != INVALID_LINK &&
		IsUnitDeadOrDying( pTarget ) )
	{
		DWORD dwSeed = SkillGetSeed(tData.pGame, tData.pUnit, UnitGetById( tData.pGame, tData.idWeapon ), nSkillToDo );
		SkillStartRequest( tData.pGame, tData.pUnit, nSkillToDo, INVALID_ID, VECTOR(0),
			SKILL_START_FLAG_DONT_SEND_TO_SERVER | SKILL_START_FLAG_INITIATED_BY_SERVER, dwSeed, tData.nSkillLevel);
	}

	return TRUE;
}
//marsh end added


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void sSkillAwardHPAndMana(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	int propToChange,
	UNIT *unitToGive,
	UNIT *target,
	int amountToTransfer,
	BOOL visualOnly )
{
	if( propToChange < 0 &&
		propToChange > 2 )
		return;
	BOOL bSuccess = FALSE;
	//HP
	if(!IsUnitDeadOrDying(unitToGive))
	{
		if( propToChange != 1 )
		{
			int nMaxHP = UnitGetStat( unitToGive, STATS_HP_MAX );
			int transHP = nMaxHP;
			transHP = (int)((float)transHP * ( (float)amountToTransfer/100.0f ) ); //amount to transfer
			if( transHP <= 0 ) //we want to transfer at least 1
				transHP = 1;
			int cur = UnitGetStat( unitToGive, STATS_HP_CUR );
			//if( cur < nMaxHP )
			{
				bSuccess = true;
				if( visualOnly == false )
				{
					UnitSetStat( unitToGive, STATS_HP_CUR, MIN( cur + transHP, nMaxHP ) );
				}
			}
		}
		//end HP
		//start Mana
		if( propToChange != 0 )
		{
			int nMaxMana = UnitGetStat( unitToGive, STATS_POWER_MAX );
			int transMana = nMaxMana;
			transMana = (int)((float)transMana * ( (float)amountToTransfer/100.0f ) ); //amount to transfer
			if( transMana <= 0 ) //we want to transfer at least 1
				transMana = 1;
			int cur = UnitGetStat( unitToGive, STATS_POWER_CUR );
			//if( cur < nMaxMana )
			{
				bSuccess = true;
				if( visualOnly == false )
				{
					UnitSetStat( unitToGive, STATS_POWER_CUR, MIN( cur + transMana, nMaxMana ) );
				}
			}
		}
		//end mana
	}
	if( bSuccess )
	{
		int nDuration = 0;
		int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
		if( nState != INVALID_LINK )
		{
			UNIT * pTarget = target;
			if( pTarget == NULL )
				pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
			if( pTarget )
				s_StateSet( pTarget, unitToGive, nState, nDuration ); //state to fire off
		}

	}

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Marsh - Had to do this function because of Health Stab. The reason why this isn't generic is because
//there currently is no why that I can specify two different Attachment Defs. So I have to hard code this - bad.:(
//nice to change to have this a generic function. NOTE we need two different stats to do this.
static BOOL sSkillEventGiveHPManaByPercent(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	if( IS_CLIENT( tData.pGame ) )
		return FALSE;

#define PROP_HP 0
#define PROP_MANA 1
#define PROP_BOTH 2
#define PROP_HP_OWNER 10
#define PROP_MANA_OWNER 11
#define PROP_BOTH_OWNER 12
#define PROP_HP_PARTY 20
#define PROP_MANA_PARTY 21
#define PROP_BOTH_PARTY 22

	int chanceToOccur = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	BOOL visualOnly = (SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 ) > 0 ) ? TRUE : FALSE;
	if( int(RandGetNum( tData.pGame, 100 )) < chanceToOccur || visualOnly )
	{
		int amountToTransfer = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
		int propToChange = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
		UNIT *unitToGive = tData.pUnit;
		UNIT *target = NULL;
		if( propToChange > PROP_BOTH )
		{
			propToChange -= 10;
			if( SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon ) == NULL )
				target = tData.pUnit;
			unitToGive = UnitGetUltimateOwner( tData.pUnit ); //anything above PROP_BOTH in value is ultimate owner
		}

		if( propToChange <= PROP_BOTH )
		{
			sSkillAwardHPAndMana( tData, propToChange, unitToGive, target, amountToTransfer, visualOnly );
		}
		else	//healing all party members
		{
			propToChange -= 10;
			LEVEL * level = UnitGetLevel( tData.pUnit );
			if (!level)
			{
				return FALSE; //nothing happens
			}
			for (GAMECLIENT * client = ClientGetFirstInLevel(level); client; client = ClientGetNextInLevel(client, level))
			{
				UNIT * unit = ClientGetPlayerUnit(client);
				if (!unit ||
					(unit && UnitIsInLimbo( unit ) ))
				{
					continue;
				}
				if( UnitIsPlayer( unit ) ) //we only heal players
				{
					sSkillAwardHPAndMana( tData, propToChange, unit, target, amountToTransfer, visualOnly );
				}
			}
		}
	}


	return TRUE;
}
//marsh end added

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRecipeOpen(
	const SKILL_EVENT_FUNCTION_DATA &tData )
{
	GAME *pGame = tData.pGame;
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	UNIT *pPlayer = tData.pUnit;
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the item they are using
	UNIT *pItem = SkillGetTarget( pPlayer, tData.nSkill, tData.idWeapon, 0 );
	ASSERTX_RETFALSE( pItem, "Expected item unit" );
	ASSERTX_RETFALSE( UnitIsA( pItem, UNITTYPE_ITEM ), "Expected item" );

	// what recipe does this object have
	int nRecipe = UnitGetStat( pItem, STATS_RECIPE_SINGLE_USE );
	ASSERTX_RETFALSE( nRecipe != INVALID_LINK, "Expected recipe link in single use recipe scroll" );

	// open recipe on client
	s_RecipeOpen( pPlayer, nRecipe, pItem );

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventCubeOpen( const SKILL_EVENT_FUNCTION_DATA &tData )
{
	GAME *pGame = tData.pGame;
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	UNIT *pPlayer = tData.pUnit;
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the cube unit they are using
	UNIT *pCube = SkillGetTarget( pPlayer, tData.nSkill, tData.idWeapon, 0 );
	ASSERTX_RETFALSE( pCube, "Expected item unit" );
	ASSERTX_RETFALSE( UnitIsA( pCube, UNITTYPE_ITEM ), "Expected item" );

	// open cube on client
	s_CubeOpen( pPlayer, pCube );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventBackpackOpen( const SKILL_EVENT_FUNCTION_DATA &tData )
{
	GAME *pGame = tData.pGame;
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( IS_SERVER( pGame ), "Server only" );
	UNIT *pPlayer = tData.pUnit;
	ASSERTX_RETFALSE( pPlayer, "Expected player unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the backpack unit they are using
	UNIT *pBackpack = SkillGetTarget( pPlayer, tData.nSkill, tData.idWeapon, 0 );
	ASSERTX_RETFALSE( pBackpack, "Expected item unit" );
	ASSERTX_RETFALSE( UnitIsA( pBackpack, UNITTYPE_ITEM ), "Expected item" );

	// open backpack on client
	s_BackpackOpen( pPlayer, pBackpack );

	return TRUE;
}

#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleDoSkillOnHit(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pDefender,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	UNITID idWeapon		= (UNITID) pHandlerData->m_Data1;
	int nSkill			= (int) pHandlerData->m_Data2;
	int nTargetSkill	= (int) pHandlerData->m_Data3;
	int nSkillLevel = (int)pHandlerData->m_Data4;

	ASSERT_RETURN ( nSkill != INVALID_ID );
	ASSERT_RETURN ( pUnit );
	UNIT * pSkillTarget = SkillGetTarget( pUnit, nTargetSkill, idWeapon );
	if ( pSkillTarget == pDefender )
	{ // only do this if you are targeting this unit
		DWORD dwSeed = SkillGetSeed(pGame, pDefender, UnitGetById(pGame, idWeapon), nSkill);
		DWORD dwFlags = SKILL_START_FLAG_DONT_RETARGET | SKILL_START_FLAG_DONT_SEND_TO_SERVER;
		if ( IS_SERVER( pGame ) )
			dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
		SkillStartRequest( pGame, pDefender, nSkill, UnitGetId( pDefender ), VECTOR(0), dwFlags, dwSeed, nSkillLevel);
	}
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoSkillOnHit(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	int nSkillToDo = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_DIDHIT, tData.nSkill,
		sHandleDoSkillOnHit, EVENT_DATA( tData.idWeapon, nSkillToDo, tData.nSkill, tData.nSkillLevel ));

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleDoAttackerSkillOnDidHit(
	GAME* pGame,
	UNIT* pAttacker,
	UNIT* pDefender,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	UNITID idWeapon		= (UNITID) pHandlerData->m_Data1;
	int nSkill			= (int) pHandlerData->m_Data2;
	int nTargetSkill	= (int) pHandlerData->m_Data3;
	int skillLevel = (int)pHandlerData->m_Data4;

	ASSERT_RETURN ( nSkill != INVALID_ID );
	ASSERT_RETURN ( pAttacker );

	UNIT * pSkillTarget = SkillGetTarget( pAttacker, nTargetSkill, idWeapon );
	if ( pSkillTarget == pDefender )
	{ // only do this if you are targeting this unit
		DWORD dwSeed = SkillGetSeed(pGame, pDefender, UnitGetById(pGame, idWeapon), nSkill);
		DWORD dwFlags = SKILL_START_FLAG_DONT_RETARGET | SKILL_START_FLAG_DONT_SEND_TO_SERVER;
		if ( IS_SERVER( pGame ) )
			dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
		SkillStartRequest( pGame, pAttacker, nSkill, UnitGetId( pDefender ), VECTOR(0), dwFlags, dwSeed, skillLevel );
	}
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoAttackerSkillOnDidHit(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	int nSkillToDo = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_DIDHIT, tData.nSkill,
		sHandleDoAttackerSkillOnDidHit, EVENT_DATA(tData.idWeapon, nSkillToDo, tData.nSkill, tData.nSkillLevel));

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sIsImmuneToPull(
	UNIT *pUnit)
{
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );

	if ( UnitIsPlayer( pUnit ) )
		return TRUE;

	// immobile things cannot be pulled
	if (UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CANNOT_BE_MOVED))
	{
		return TRUE;
	}

	if ( UnitHasState( UnitGetGame( pUnit ), pUnit, STATE_INVULNERABLE_KNOCKBACK ) )
		return TRUE;

	return FALSE;

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sHandlePullFinishEvent(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	int nMissileId = (int)pHandlerData->m_Data1;
	UNIT * pMissile = UnitGetById( pGame, nMissileId );
	if ( pMissile )
	{
		MissileConditionalFree( pMissile );
	}
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sHandlePullTargetEndMode(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_REACHED_DESTINATION, sHandlePullTargetEndMode);
	UnitEndMode(pUnit, MODE_KNOCKBACK);
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sPullTargetToUnit(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	UNIT * pTarget,
	UNIT * pPuller,
	BOOL bRemovePullerWhenFinished )
{
	if ( ! pTarget || ! pPuller )
		return FALSE;

	if (sIsImmuneToPull( pTarget ) == TRUE)
	{
		return	FALSE;
	}

	if ( ! s_UnitSetMode( pTarget, MODE_KNOCKBACK, TRUE ) )
		return FALSE;

	const UNITMODE_DATA * pModeData = (const UNITMODE_DATA *) ExcelGetData( tData.pGame, DATATABLE_UNITMODES, MODE_KNOCKBACK );
	int nState = pModeData->nClearStateEnd;
	ASSERT_RETFALSE( nState != INVALID_ID );

	if ( UnitHasState( tData.pGame, tData.pUnit, nState ) )
		return FALSE;

	if ( ! UnitInLineOfSight( tData.pGame, pPuller, pTarget ) )
		return FALSE;

	s_StateSet( pTarget, tData.pUnit, nState, 0, tData.nSkill );

	VECTOR vDirection = c_UnitGetPosition( pTarget ) - c_UnitGetPosition( pPuller );
	VectorNormalize( vDirection );

	UnitSetMoveTarget( pTarget, UnitGetId( pPuller ) );

	UnitSetFlag(pTarget, UNITFLAG_TEMPORARILY_DISABLE_PATHING);
	UnitStepListAdd( tData.pGame, pTarget );

	if ( bRemovePullerWhenFinished )
		sSkillRegisterUnitEventWithRemove(tData.pGame, pTarget, EVENT_REACHED_DESTINATION, tData.nSkill,
			sHandlePullFinishEvent, EVENT_DATA( UnitGetId( tData.pUnit ) ));

	PathStructClear(tData.pUnit);

	BYTE bFlags;
	SETBIT(&bFlags, UMID_NO_PATH);
	s_SendUnitMoveID(pTarget, bFlags, MODE_KNOCKBACK, UnitGetId(pPuller), vDirection);
	UnitRegisterEventHandler( tData.pGame, pTarget, EVENT_REACHED_DESTINATION, sHandlePullTargetEndMode, EVENT_DATA());

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventPullTargetToSource(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pSourceUnit = UnitGetOwnerUnit( tData.pUnit );
	if ( ! pSourceUnit )
		pSourceUnit = tData.pUnit;

	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	return sPullTargetToUnit( tData, pTarget, pSourceUnit, TRUE );

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventPullTargetsToSelf(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_UNIT_RADIUS );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
	if ( IS_CLIENT( tData.pGame ) )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START );
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START );
	}

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	ASSERT_RETFALSE(fRange != 0.0f);
	tTargetingQuery.fMaxRange	= SkillGetRange( tData.pUnit, tData.nSkill, pWeapon );
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( pTarget )
			sPullTargetToUnit( tData, pTarget, tData.pUnit, FALSE );
	}

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoChargeCollideActions(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData)
{
	int nSkill = (int)pHandlerData->m_Data1;
	int nSkillLevel = (int)pHandlerData->m_Data3;
	UNITID idWeapon = (UNITID)pHandlerData->m_Data2;
	ASSERT_RETURN( nSkill != INVALID_ID );

	if(IS_SERVER(pGame))
	{
		//if (pOtherUnit)
		{
			SKILL_EVENT tSkillEvent;
			memset(&tSkillEvent, 0, sizeof(SKILL_EVENT));

			SKILL_EVENT_FUNCTION_DATA tData( pGame, pUnit, &tSkillEvent, nSkill, nSkillLevel, SkillGetData( pGame, nSkill), idWeapon );
			sSkillEventMeleeAttack( tData );
		}

		SkillStop(pGame, pUnit, nSkill, idWeapon);
		s_StateClear( pUnit, UnitGetId(pUnit), STATE_STATS_HOLDER, 0 );
		//s_UnitSetMode( pUnit, MODE_ATTACK_RECOVER, 0, 0.0f, TRUE );
	}
	else
	{
		UnitSetFlag(pUnit, UNITFLAG_DISABLE_CLIENTSIDE_PATHING);
		SkillStop(pGame, pUnit, nSkill, idWeapon);
		UnitClearFlag(pUnit, UNITFLAG_DISABLE_CLIENTSIDE_PATHING);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleChargePathEndEvent(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId );

static void sHandleChargeCollideEvent(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	//if (IS_SERVER(pGame))
	{
		sDoChargeCollideActions(pGame, pUnit, pOtherUnit, pHandlerData);
		UnitUnregisterEventHandler(pGame, pUnit, EVENT_DIDCOLLIDE, dwId);
		UnitUnregisterEventHandler(pGame, pUnit, EVENT_REACHED_DESTINATION, sHandleChargePathEndEvent);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleChargePathEndEvent(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	//if (IS_SERVER(pGame))
	{
		sDoChargeCollideActions(pGame, pUnit, pOtherUnit, pHandlerData);
		UnitUnregisterEventHandler(pGame, pUnit, EVENT_DIDCOLLIDE, sHandleChargeCollideEvent);
		UnitUnregisterEventHandler(pGame, pUnit, EVENT_REACHED_DESTINATION, dwId);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventChargeBlindly(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	if ( IS_SERVER( tData.pGame ) )
	{
		UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

		VECTOR vDirection;
		VECTOR vTarget;
		if (pTarget)
		{
			vTarget = UnitGetPosition(pTarget);
			vDirection = vTarget - UnitGetPosition(tData.pUnit);
		}
		else
		{
			vTarget = UnitGetPosition(tData.pUnit);
			vDirection = UnitGetFaceDirection(tData.pUnit, FALSE);
		}
		VectorNormalize(vDirection);
		UnitSetFaceDirection(tData.pUnit, vDirection, TRUE);

		vTarget += vDirection /** tData.pSkillEvent->fParam*/;

		UnitSetMoveTarget( tData.pUnit, UnitGetId(pTarget) );
		//UnitSetMoveTarget( tData.pUnit, UnitGetPosition(pTarget), vDirection );
		DWORD dwPathFlags = 0;
		SETBIT(dwPathFlags, PCF_CHECK_MELEE_ATTACK_AT_DESTINATION_BIT);
		if(!UnitCalculatePath( tData.pGame, tData.pUnit, PATH_FULL, dwPathFlags ))
			return FALSE;

		UNITMODE eMode = MODE_RUN;
		if(tData.pSkillEvent->tAttachmentDef.nAttachedDefId >= 0)
		{
			eMode = (UNITMODE)tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
		}
		if(!s_UnitSetMode( tData.pUnit, eMode, 0, 0.0f, 0, FALSE ))
			return FALSE;

		UnitStepListAdd( tData.pGame, tData.pUnit );
		UnitActivatePath( tData.pUnit );

		s_StateClear( tData.pUnit, UnitGetId(tData.pUnit), STATE_STATS_HOLDER, 0 );
		s_StateSet( tData.pUnit, tData.pUnit, STATE_STATS_HOLDER, 0, tData.nSkill, NULL );

		//BYTE bFlags = MOVE_FLAG_CLIENT_STRAIGHT;
		//s_SendUnitMoveXYZ( tData.pUnit, bFlags, eMode, vTarget, vDirection, TRUE );a
		BYTE bFlags = 0;
		SETBIT(&bFlags, UMID_CLIENT_STRAIGHT);
		s_SendUnitMoveID( tData.pUnit, bFlags, eMode, UnitGetId(pTarget), vDirection );

		sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_DIDCOLLIDE, tData.nSkill,
			sHandleChargeCollideEvent, EVENT_DATA(tData.nSkill, tData.idWeapon, tData.nSkillLevel));
		//sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_REACHED_PATH_END, sHandleChargePathEndEvent, &EVENT_DATA());
		sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_REACHED_DESTINATION, tData.nSkill,
			sHandleChargePathEndEvent, EVENT_DATA(tData.nSkill, tData.idWeapon, tData.nSkillLevel));
	}
	else
	{
		sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_DIDCOLLIDE, tData.nSkill,
			sHandleChargeCollideEvent, EVENT_DATA(tData.nSkill, tData.idWeapon, tData.nSkillLevel));
		//sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_REACHED_PATH_END, sHandleChargePathEndEvent, &EVENT_DATA());
		sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_REACHED_DESTINATION, tData.nSkill,
			sHandleChargePathEndEvent, EVENT_DATA(tData.nSkill, tData.idWeapon, tData.nSkillLevel));
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandlePlayerChargeFinishEvent(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherUnit,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	int nSkill = (int)pHandlerData->m_Data1;
	UNITID idWeapon = (UNITID)pHandlerData->m_Data2;
	ASSERT_RETURN( nSkill != INVALID_ID );

	//UnitStepListRemove(pGame, pUnit);
	//SkillSetRequested( pUnit, nSkill, idWeapon, FALSE );
	SkillStop(pGame, pUnit, nSkill, idWeapon, TRUE, TRUE);
	UnitUnregisterEventHandler(pGame, pUnit, EVENT_REACHED_DESTINATION, dwId);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventPlayerCharge(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(tData.pUnit);
	VECTOR vUnitPosition = UnitGetPosition(tData.pUnit);
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	VECTOR vTargetPosition;
	VECTOR vLocation;
	if (pTarget)
	{
		vLocation = UnitGetPosition(pTarget);
		vTargetPosition = vLocation;
		float fGroundOffset = LevelLineCollideLen(tData.pGame, UnitGetLevel(pTarget), vLocation, VECTOR(0, 0, -1), 100.0f);
		vLocation.fZ -= fGroundOffset;
	}
	else
	{
		VECTOR vFacing = UnitGetFaceDirection(tData.pUnit, FALSE);
		vFacing.fZ = 0.0f;
		VectorNormalize(vFacing);

		UNIT * pWeapon = UnitGetById(tData.pGame, tData.idWeapon);
		float fRange = SkillGetRange(tData.pUnit, tData.nSkill, pWeapon);
		vLocation = vUnitPosition + fRange * vFacing;
		vTargetPosition = vLocation;
		ROOM * pRoom = NULL;
		ROOM_PATH_NODE * pPathNode = RoomGetNearestFreePathNode(tData.pGame, UnitGetLevel(tData.pUnit), vLocation, &pRoom, tData.pUnit);
		if(pRoom && pPathNode)
		{
			vLocation = RoomPathNodeGetExactWorldPosition(tData.pGame, pRoom, pPathNode);
		}
		else
		{
			return FALSE;
		}
	}

	VECTOR vDirection = vTargetPosition - vUnitPosition;
	vDirection.fZ = 0.0f;
	VectorNormalize(vDirection);
	UnitSetFaceDirection( tData.pUnit, vDirection, FALSE, TRUE );

	UnitCalcVelocity(tData.pUnit);
	float fVelocity = UnitGetVelocity( tData.pUnit );
	UnitSetVelocity( tData.pUnit, fVelocity );
	UnitSetMoveTarget( tData.pUnit, vLocation, vDirection );
	UnitStepListAdd( tData.pGame, tData.pUnit );

	sSkillRegisterUnitEventWithRemove(tData.pGame, tData.pUnit, EVENT_REACHED_DESTINATION, tData.nSkill,
		sHandlePlayerChargeFinishEvent, EVENT_DATA(tData.nSkill, tData.idWeapon));

	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleRemoveMissile(
	 GAME* pGame,
	 UNIT* pUnit,
	 UNIT* pTarget,
	 EVENT_DATA* pHandlerData,
	 void* pData,
	 DWORD dwId )
{
	EVENT_DATA* pTriggerData = (EVENT_DATA *) pData;
	if ( pTriggerData->m_Data1 != pHandlerData->m_Data1 ||
		pTriggerData->m_Data2 != pHandlerData->m_Data2 )
		return;

	UNITID idMissile = (UNITID)pHandlerData->m_Data3;

	UNIT * pMissile = UnitGetById( pGame, idMissile );

	if ( pMissile )
	{
		MissileConditionalFree( pMissile );
	}

	UnitUnregisterEventHandler( pGame, pUnit, EVENT_SKILL_DESTROYING_MISSILES, dwId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleFuseMissile(
	 GAME* pGame,
	 UNIT* pUnit,
	 UNIT* pTarget,
	 EVENT_DATA* pHandlerData,
	 void* pData,
	 DWORD dwId )
{
	EVENT_DATA* pTriggerData = (EVENT_DATA *) pData;
	if ( pTriggerData->m_Data1 != pHandlerData->m_Data1 )
		return;

	UNITID idMissile = (UNITID)pHandlerData->m_Data2;

	UNIT * pMissile = UnitGetById( pGame, idMissile );

	if ( pMissile )
	{
		UnitSetFuse( pMissile, 1 );
	}

	UnitUnregisterEventHandler( pGame, pUnit, EVENT_STATE_CLEARED, dwId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sHandleMissileFreed(
	  GAME* pGame,
	  UNIT* pUnit,
	  UNIT* pTarget,
	  EVENT_DATA* pHandlerData,
	  void* pData,
	  DWORD dwId )
{
	UNITID idSource = (UNITID)pHandlerData->m_Data1;
	DWORD dwEventId = (DWORD)pHandlerData->m_Data2;
	UNIT_EVENT UNIT_EVENTId = (UNIT_EVENT)pHandlerData->m_Data3;

	UNIT * pSource = idSource != INVALID_ID ? UnitGetById( pGame, idSource ) : NULL;
	if ( pSource )
	{
		UnitUnregisterEventHandler( pGame, pSource, UNIT_EVENTId, dwEventId );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillSaveUnitForRemovalOnSkillStop(
	GAME * pGame,
	UNIT * pSource,
	UNIT * pUnit,
	UNIT * pWeapon,
	int nSkill )
{
	ASSERT_RETURN( pSource );
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( nSkill != INVALID_ID );
	UNITID idWeapon = pWeapon ? UnitGetId( pWeapon ) : INVALID_ID;
	// tell unit to remember to remove the missile
	DWORD dwEventId;
	UnitRegisterEventHandlerRet(dwEventId, pGame, pSource, EVENT_SKILL_DESTROYING_MISSILES, sHandleRemoveMissile, EVENT_DATA( nSkill, idWeapon, UnitGetId( pUnit ) ) );

	// let the missile clean up the previous handler when it is freed
	if (IS_CLIENT( pGame ))
	{
		UnitRegisterEventHandler( pGame, pUnit, EVENT_ON_FREED, c_sHandleMissileFreed, EVENT_DATA( UnitGetId( pSource ), dwEventId, EVENT_SKILL_DESTROYING_MISSILES ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillSaveUnitForFuseOnStateClear(
	GAME * pGame,
	UNIT * pSource,
	UNIT * pUnit,
	int nState)
{
	ASSERT_RETURN( pSource );
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( nState != INVALID_ID );

	// tell unit to remember to remove the missile
	DWORD dwEventId;
	UnitRegisterEventHandlerRet( dwEventId, pGame, pSource, EVENT_STATE_CLEARED, sHandleFuseMissile, EVENT_DATA( nState, UnitGetId( pUnit ) ) );

	// let the missile clean up the previous handler when it is freed
	if (IS_CLIENT( pGame ))
	{
		UnitRegisterEventHandler( pGame, pUnit, EVENT_ON_FREED, c_sHandleMissileFreed, EVENT_DATA( UnitGetId( pSource ), dwEventId, EVENT_STATE_CLEARED ) );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum
{
	FMC_USE_MISSILE_NUMBER_BIT,
	FMC_MISSILE_POSITION_PASSED_IN,
	FMC_USE_MISSILE_TARGET
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sSkillEventFireMissileCommon(
	GAME * pGame,
	UNIT * pUnit,
	const SKILL_EVENT * pSkillEvent,
	UNITID idWeapon,
	DWORD dwSeed,
	int nSkill,
	WORD wMissileTag,
	DWORD dwFireMissileFlags,
	float flHorizSpread,
	int nMissileNumber,
	VECTOR vecPosToFireToo = VECTOR( 0.0f, 0.0f, 0.0f ),
	int nForceTargetByID = INVALID_ID)
{
	int nMissileClass = pSkillEvent->tAttachmentDef.nAttachedDefId;
	VECTOR vFireDirection;
	VECTOR vFirePosition;

	int nParam2 = TESTBIT(dwFireMissileFlags, FMC_USE_MISSILE_NUMBER_BIT) ? nMissileNumber : 0;
	VECTOR vTarget = UnitGetStatVector( pUnit, STATS_SKILL_TARGET_X, nSkill, nParam2 );

	UNIT * pTarget = SkillGetTarget( pUnit, nSkill, idWeapon );

	//this gets set if you are firing from a monster but that missile has a delay
	if( TESTBIT( dwFireMissileFlags, FMC_USE_MISSILE_TARGET ) && nForceTargetByID != INVALID_ID )
	{
		pTarget = UnitGetById( pGame, nForceTargetByID );
		if( pTarget )
		{
			vTarget = UnitGetPosition( pTarget );
		}
	}

	const UNIT_DATA * missile_data = MissileGetData(pGame, nMissileClass);
	ASSERT_RETNULL(missile_data);

	//Marsh - added to allow positioning by an outside function call
	//NOTE this was done before the testbit flag was implemented. This should be
	//rewritten to do the bit check above....
	if( !VectorIsZero( vecPosToFireToo ) )
	{
		pTarget = NULL;						//we won't target a UNIT
		vTarget = vecPosToFireToo;			//vec firing too. Can be the same
	}
	//Marsh end Added
	SkillGetFiringPositionAndDirection(
		pGame,
		pUnit,
		nSkill,
		idWeapon,
		pSkillEvent,
		missile_data,
		vFirePosition,
		&vFireDirection,
		pTarget,
		&vTarget,
		dwSeed,
		FALSE,
		flHorizSpread,
		TRUE );

	if( TESTBIT( dwFireMissileFlags, FMC_MISSILE_POSITION_PASSED_IN ) )
	{
		vFirePosition = vTarget + pSkillEvent->tAttachmentDef.vPosition;
		vFireDirection = ( AppIsHellgate() || pSkillEvent->tAttachmentDef.vNormal.x == -1.0f )?VECTOR(0, 0, 1):pSkillEvent->tAttachmentDef.vNormal;
	}


	if ( pTarget && ( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_PLACE_ON_TARGET ) != 0 )
	{
		if( pTarget )
		{
			vFirePosition = UnitGetPosition( pTarget ) + pSkillEvent->tAttachmentDef.vPosition;
			//add an offset to the position
			vFirePosition += vFireDirection * UnitGetCollisionRadius( pTarget );
		}
	}

	if(pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION)
	{

		vFirePosition = vTarget;
		if( AppIsTugboat() )
		{   //Tugboat - force the position.
			vFirePosition = vTarget + pSkillEvent->tAttachmentDef.vPosition;
		}
		if(VectorIsZero(vTarget))
		{
			if (pUnit)
			{
				vTarget = UnitGetPosition(pUnit);
				vFirePosition = vTarget;
			}
		}
		//this should be done via a flag( Need to fix ) - marsh
		vFireDirection = ( AppIsHellgate() || pSkillEvent->tAttachmentDef.vNormal.x == -1.0f )?VECTOR(0, 0, 1):pSkillEvent->tAttachmentDef.vNormal;
	}

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	sGetWeaponsForDamage( pGame, pUnit, nSkill, pSkillData, idWeapon, pWeapons );
	UNIT * pMissile = MissileFire(
		pGame,
		nMissileClass,
		pUnit,
		pWeapons,
		nSkill,
		pTarget,
		vTarget,
		vFirePosition,
		vFireDirection,
		dwSeed,
		wMissileTag,
		0,
		NULL,
		nMissileNumber);

	if ( pMissile && SkillDataTestFlag( pSkillData, SKILL_FLAG_SAVE_MISSILES ) )
	{
		SkillSaveUnitForRemovalOnSkillStop( pGame, pUnit, pMissile, UnitGetById(pGame, idWeapon), nSkill );
	}

	if( pMissile && pSkillData->nFuseMissilesOnState != INVALID_ID )
	{
		SkillSaveUnitForFuseOnStateClear( pGame, pUnit, pMissile, pSkillData->nFuseMissilesOnState );
	}
	return pMissile;
}

//----------------------------------------------------------------------------
enum FIRE_MISSILE_OPTIONS
{
	FMO_RANDOM_START_ANGLE_BIT	= 1,
	FMO_FIRE_NOVA_AROUND_BIT	= 2,
	FMO_USE_WEAPON_TO_AIM		= 3,
};



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileUseAllAmmo(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	ASSERT_RETFALSE( pWeapon );

	WORD wMissileTag = GameGetMissileTag(tData.pGame);

	// get param info
	int nMinMissiles = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	int nAmmo = UnitGetStat( pWeapon, STATS_WEAPON_AMMO_CUR );
	UnitSetStat( pWeapon, STATS_WEAPON_AMMO_CUR, 0 );

	int nMissileCount = MAX( nMinMissiles, nAmmo );

	// init rand seed for each missile
	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );
	RAND tRand;
	RandInit( tRand, dwSeed );
	for ( int i = 0; i < nMissileCount; i++ )
	{

		// what is the horizontal spread for this missile
		float flHorizSpread = 0.0f;
		if (nMissileCount > 0 && i != 0)
		{
			flHorizSpread = 0.15f;  // we spread out our "extra bullets"
		}

		UNIT *pMissile = sSkillEventFireMissileCommon(
			tData.pGame,
			tData.pUnit,
			tData.pSkillEvent,
			tData.idWeapon,
			dwSeed,
			tData.nSkill,
			wMissileTag,
			0,
			flHorizSpread,
			i);
		REF(pMissile);

		// get another seed for repeat missiles
		dwSeed = RandGetNum( tRand );

	}

	return TRUE;

}

//----------------------------------------------------------------------------
// Marsh added - this handles pausing of a missile before it gets shot
//----------------------------------------------------------------------------
static BOOL sHandleDoMissileDelayed(
	GAME* pGame,
	UNIT* pUnit,
	const EVENT_DATA& event_data)
{
	const SKILL_EVENT *pSkillEvent			= (SKILL_EVENT*)event_data.m_Data1;
	DWORD idWeapon							= (DWORD)event_data.m_Data2;
	DWORD missileID							= (DWORD)event_data.m_Data3;
	DWORD dwSeed 							= (DWORD)event_data.m_Data4;
	DWORD nSkill							= (DWORD)event_data.m_Data5;
	int nMonsterID							= (int)event_data.m_Data6;
	DWORD nFlags( 0 );
	if(	nMonsterID != INVALID_ID )
	{
		SETBIT( nFlags, FMC_USE_MISSILE_TARGET );
	}
	UNIT *pMissile = sSkillEventFireMissileCommon(
								pGame,
								pUnit,
								pSkillEvent,
								idWeapon,
								dwSeed,
								nSkill,
								GameGetMissileTag(pGame),
								nFlags,
								0,
								missileID,
								VECTOR( 0, 0, 0 ),
								nMonsterID );

	if( pMissile == NULL )
		return FALSE;  //this occurs when your synching a missile and your the client

	return TRUE;
}
//----------------------------------------------------------------------------
// Marsh added - this handles pausing of a missile before it gets shot
//----------------------------------------------------------------------------
static BOOL sHandleDoMissileDelayedRadial(
	GAME* pGame,
	UNIT* pUnit,
	const EVENT_DATA& event_data)
{
	const SKILL_EVENT *pSkillEvent			= (SKILL_EVENT*)event_data.m_Data1;
	DWORD idWeapon							= (DWORD) event_data.m_Data2;
	DWORD nMissileCount						= (DWORD) ( event_data.m_Data3 >> 8 );
	DWORD missileID							= (DWORD) ( event_data.m_Data3 & 0xf );
	DWORD dwSeed 							= (DWORD) event_data.m_Data4;
	DWORD nSkill							= (DWORD) event_data.m_Data5;
	int nSkillLevel = (int)event_data.m_Data6;
	VECTOR tmpVec( 0, 0, 0 );										 					//temp vec

	// get param info
	float minRadius = MAX( SkillEventGetParamFloat( pUnit, nSkill, nSkillLevel, pSkillEvent, 1 ), 0.0f );					//min radius for missile
	float maxRadius = MAX( SkillEventGetParamFloat( pUnit, nSkill, nSkillLevel, pSkillEvent, 2 ), minRadius );			//max radius for missile
	float distanceToActivateIn = maxRadius - minRadius;													//distance
	float circleDegrees = 360.0f/(float)nMissileCount;										 			 //degrees around position - do this to give a bit better effect

	//figure out position
	tmpVec.y = tmpVec.z = 0;												    //zero tmpvec
	tmpVec.x = 1;																//start with a valid vector
	VectorZRotation( tmpVec, circleDegrees * (float)missileID );						//rotate about the vector
	tmpVec *= ( ( RandGetFloat( pGame ) * distanceToActivateIn ) + minRadius );	//mult tmpVec to get the correct distance on the line; that is rotated about a circle
	UNIT * pTarget = SkillGetTarget( pUnit, nSkill, idWeapon );
	VECTOR targetPos( UnitGetStatVector( pUnit, STATS_SKILL_TARGET_RIGHT_CLICK_X, nSkill, 0 ) );
	if( pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION &&
		!VectorIsZero( targetPos ) )
	{
		tmpVec = targetPos + tmpVec;
		pTarget = NULL;
	}else if( pTarget != NULL && pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		tmpVec = UnitGetPosition( pTarget ) + tmpVec;						//copy target unit position, add on new relative position
	}
	else
	{
		tmpVec = UnitGetPosition( pUnit ) + tmpVec;							//copy original unit position, add on new relative position

	}
	//end figure out position

	UnitSetStatVector( pUnit, STATS_SKILL_TARGET_X, nSkill, 0, tmpVec );
	DWORD dwFireMissileFlags( 0 );
	SETBIT( dwFireMissileFlags, FMC_MISSILE_POSITION_PASSED_IN );
	UNIT *pMissile = sSkillEventFireMissileCommon(
								pGame,
								pUnit,
								pSkillEvent,
								idWeapon,
								dwSeed,
								nSkill,
								GameGetMissileTag(pGame),
								dwFireMissileFlags,
								0,
								missileID,
								tmpVec );		//position to place missile

	if( pMissile == NULL )
		return FALSE;  //this occurs when your synching a missile and your the client

	return TRUE;
}


//----------------------------------------------------------------------------
// marsh created. This function sets random points about a given unit and fires missiles on a delay
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileRandomRadius(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	const SKILL_DATA *pSkillData = tData.pSkillData;

	// get param info - marsh added
	int nMissileCount = MAX( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 1 );				//missile count
	float timeToShow = MAX( SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 ), 0.0f );				//time to show missiles - so they all don't appear at the same time
//	float secondForDelay = 0.0f;																	//delay in seconds

	// add more missiles for spread skill, but be sure to limit to what the skill will allow
	int nExtraBullets = UnitGetStat( tData.pUnit, STATS_MULTI_BULLET_COUNT );
	nExtraBullets *= pSkillData->nSpreadBulletMultiplier;
	if (pSkillData->nMaxExtraSpreadBullets != -1 && nExtraBullets > pSkillData->nMaxExtraSpreadBullets)
	{
		nExtraBullets = pSkillData->nMaxExtraSpreadBullets;
	}

	// add our extra bullets to the missile count
	nMissileCount += nExtraBullets;

	// init rand seed for each missile
	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );
	RAND tRand;
	RandInit( tRand, dwSeed );
	VECTOR positionOut( UnitGetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill )  );

	UnitSetStatVector( tData.pUnit, STATS_SKILL_TARGET_RIGHT_CLICK_X, tData.nSkill, 0, positionOut );
	for ( int i = 0; i < nMissileCount; i++ )
	{
		DWORD nDelay = (DWORD)(  RandGetFloat( tData.pGame ) * timeToShow * 20 );
		DWORD mergeNumber = ( nMissileCount << 8 ) + i;
		if (!UnitHasTimedEvent(tData.pUnit, sHandleDoMissileDelayedRadial, CheckEventParam123, EVENT_DATA( (DWORD_PTR)tData.pSkillEvent, tData.idWeapon, mergeNumber, dwSeed, tData.nSkill ) ) )
		{
			UnitRegisterEventTimed(tData.pUnit, sHandleDoMissileDelayedRadial, EVENT_DATA((DWORD_PTR)tData.pSkillEvent, tData.idWeapon, mergeNumber, dwSeed, tData.nSkill, tData.nSkillLevel), nDelay);
		}

		dwSeed = RandGetNum( tRand );

	}

	return TRUE;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileRandomPathNode(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

	float fRandomRadius = MAX( SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 1.0f );

	NEAREST_NODE_SPEC tSpec;
	SETBIT(tSpec.dwFlags, NPN_CHECK_LOS);
	tSpec.fMinDistance = 0.0f;
	tSpec.fMaxDistance = fRandomRadius;

	const int MAX_PATH_NODES = 64;
	FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];
	int nNumNodes = RoomGetNearestPathNodes(tData.pGame, UnitGetRoom(tData.pUnit), UnitGetPosition(tData.pUnit), MAX_PATH_NODES, tFreePathNodes, &tSpec);

	if ( ! nNumNodes )
		return FALSE;

	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );
	RAND tRand;
	RandInit( tRand, dwSeed );

	int nNodeIndex = RandGetNum( tRand, nNumNodes );
	FREE_PATH_NODE * pFreePathNode = &tFreePathNodes[ nNodeIndex ];
	VECTOR vPosition = RoomPathNodeGetWorldPosition( tData.pGame, pFreePathNode->pRoom, pFreePathNode->pNode );
#if HELLGATE
	VECTOR vNormal = AppIsHellgate() ? RoomPathNodeGetWorldNormal( tData.pGame, pFreePathNode->pRoom, pFreePathNode->pNode ) : VECTOR( 0, 0, 1 );
#else
	VECTOR vNormal( 0, 0, 1 );
#endif

	int nMissileClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	ASSERT_RETFALSE( nMissileClass != INVALID_ID );

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	sGetWeaponsForDamage( tData.pGame, tData.pUnit, tData.nSkill, tData.pSkillData, tData.idWeapon, pWeapons );
	MissileFire(
		tData.pGame,
		nMissileClass,
		tData.pUnit,
		pWeapons,
		tData.nSkill,
		SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon ),
		VECTOR(0),
		vPosition,
		vNormal,
		dwSeed,
		GameGetMissileTag(tData.pGame) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sSkillEventFireMissileOnTargetsInRange(
	const SKILL_EVENT_FUNCTION_DATA & tData	)
{
	int nMissileCount = MAX( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 1 );
	float fRange = SkillGetRange( tData.pUnit, tData.nSkill, UnitGetById( tData.pGame, tData.idWeapon ) );
	ASSERT_RETFALSE( fRange>0 );
	int nTargetCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	nTargetCount = MIN(nTargetCount, MAX_TARGETS_PER_QUERY);
	if(nTargetCount == 0)
	{
		nTargetCount = MAX_TARGETS_PER_QUERY;
	}

	// find all targets within the holy aura
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = nTargetCount;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );
	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, UnitGetById( tData.pGame, tData.idWeapon ), tData.nSkill );
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;
		SkillSetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, UnitGetId( pTarget ) );
		for( int t = 1; t <= nMissileCount; t++ )
		{
			UNIT *pMissile = sSkillEventFireMissileCommon(
				tData.pGame,
				tData.pUnit,
				tData.pSkillEvent,
				tData.idWeapon,
				dwSeed,
				tData.nSkill,
				GameGetMissileTag(tData.pGame),
				0,
				0,
				(i * nMissileCount ) + t);
			REF(pMissile);
		}

	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillEventGetExtraBulletCount(
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA* pSkillData)
{
	ASSERT_RETZERO(pUnit);
	ASSERT_RETZERO(pSkillData);

	int nExtraBullets = UnitGetStat( pUnit, STATS_MULTI_BULLET_COUNT );
	if( AppIsTugboat() )
	{
		nExtraBullets += UnitGetStat( pUnit, STATS_MISSILE_COUNT_PER_SKILL, nSkill );
	}
	nExtraBullets *= pSkillData->nSpreadBulletMultiplier;
	if (pSkillData->nMaxExtraSpreadBullets != -1 && nExtraBullets > pSkillData->nMaxExtraSpreadBullets)
	{
		nExtraBullets = pSkillData->nMaxExtraSpreadBullets;
	}
	return nExtraBullets;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissile(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	WORD wMissileTag = GameGetMissileTag(tData.pGame);
	const SKILL_DATA *pSkillData = tData.pSkillData;

	DWORD dwFireMissileFlags = 0;
	// get param info
	int nMissileCount = MAX( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 1 );
	//time to release in
	float timeToReleaseIn = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );
	if( AppIsHellgate() )
	{
		if(SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ) != 0 && tData.pSkillData->pnSummonedInventoryLocations[ 0 ] != INVALID_ID)
		{
			ASSERTX( tData.pSkillData->pnSummonedInventoryLocations[ 1 ] == INVALID_ID, tData.pSkillData->szName ); // it doesn't make sense to have two inventory locations here
			nMissileCount = MAX(UnitInventoryGetArea(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ 0 ]), 1);
			SETBIT(dwFireMissileFlags, FMC_USE_MISSILE_NUMBER_BIT);
		}
	}

	// add more missiles for spread skill, but be sure to limit to what the skill will allow
	int nExtraBullets = sSkillEventGetExtraBulletCount(tData.pUnit, tData.nSkill, pSkillData);

	// add our extra bullets to the missile count
	nMissileCount += nExtraBullets;

	// init rand seed for each missile
	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );
	RAND tRand;
	RandInit( tRand, dwSeed );

	int nOffhandID( INVALID_ID );
	if( AppIsTugboat() ) //No reason why hellgate shouldn't use this..?
	{
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_ALL_WEAPONS ) ||
			(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_OFFHAND_WEAPON))
		{
			UNIT * pWeaponOther = SkillGetLeftHandWeapon( tData.pGame, tData.pUnit, tData.nSkill, pWeapon );
			if ( pWeaponOther )
			{
				nOffhandID = UnitGetId( pWeaponOther );
				nMissileCount *= 2; //we are firing twice as many missiles ( need to make sure we increment the loop before firing the second missile )
			}

		}
	}



	//keep it as a float for now
	timeToReleaseIn = ( timeToReleaseIn/(float)nMissileCount ) * GAME_TICKS_PER_SECOND_FLOAT;
	for ( int i = 0; i < nMissileCount; i++ )
	{

		if( !timeToReleaseIn )
		{
			// what is the horizontal spread for this missile
			float flHorizSpread = 0.0f;
			if( AppIsTugboat() )
			{
				// Why can't we specify a range to spread these over? This is very hardcody
				// taking param2 as the new param
				float fParam2 = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
				if( AppIsTugboat() &&
					nMissileCount == 1 )
				{
					fParam2 = 0.0f;
				}
				if( fParam2 != 0.0f )
				{
					flHorizSpread = fParam2;
					flHorizSpread /= nMissileCount > 2 ? ( (float)nMissileCount - 1 ) : 1;
					flHorizSpread *= (float)i;
					flHorizSpread -= fParam2 * .5f;
				}
				else if( nMissileCount > 1 )
				{
					flHorizSpread = 0.15f * (float)i; //Marsh changed because Tugboat needs the horizSpread to be consistant.
					flHorizSpread -= (float)nMissileCount * .15f * .5f;
				}
			}
			else if( AppIsHellgate() )
			{
				// apply a spread for every missile after the first one
				if ( i != 0)
				{
					if (nMissileCount > 0 )
					{
						flHorizSpread = HELLGATE_EXTRA_MISSILE_HORIZ_SPREAD;  // we spread out our "extra bullets"
					}
				}
			}

			// but I am le tired ... well take a nap, then fire ze missiles
			// http://www.ebaumsworld.com/flash/endofworld.html
			// GS - AH HAHAHAHAHAHAHAAAAH.
			UNIT *pMissile = sSkillEventFireMissileCommon(
				tData.pGame,
				tData.pUnit,
				tData.pSkillEvent,
				tData.idWeapon,
				dwSeed,
				tData.nSkill,
				wMissileTag,
				dwFireMissileFlags,
				flHorizSpread,
				i );
			REF(pMissile);

			// get another seed for repeat missiles
			dwSeed = RandGetNum( tRand );
			if( nOffhandID != INVALID_ID )
			{
				if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_USE_ALL_WEAPONS ) ||
					(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_OFFHAND_WEAPON))
				{
					UNIT * pWeaponOther = SkillGetLeftHandWeapon( tData.pGame, tData.pUnit, tData.nSkill, pWeapon );
					if ( pWeaponOther )
					{
						i++; //Increment the loop because I is the missile index
						UNIT *pMissile = sSkillEventFireMissileCommon(
							tData.pGame,
							tData.pUnit,
							tData.pSkillEvent,
							nOffhandID,
							dwSeed,
							tData.nSkill,
							wMissileTag,
							dwFireMissileFlags,
							flHorizSpread,
							i );
						REF(pMissile);
						dwSeed = RandGetNum( tRand );
					}
				}
			}
		}
		else
		{
			int nTargetId = INVALID_ID;
			if ( ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_PLACE_ON_TARGET ) != 0 )
			{

				nTargetId = UnitGetId( SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon ) );
			}

			EVENT_DATA event_data( (DWORD_PTR)tData.pSkillEvent, tData.idWeapon, i, dwSeed, tData.nSkill, nTargetId );
			int nDelay = (int)FLOOR(( timeToReleaseIn * (float)i) + .5f);
			if(!UnitHasTimedEvent(tData.pUnit, sHandleDoMissileDelayed, CheckEventParam123, event_data ) )
			{
				UnitRegisterEventTimed(tData.pUnit, sHandleDoMissileDelayed, event_data, nDelay);
				dwSeed = RandGetNum( tRand );
			}
			if( nOffhandID != INVALID_ID )
			{
				i++; //Increment the loop because I is the missile index
				nDelay = (int)FLOOR(( timeToReleaseIn * (float)i) + .5f);
				if(!UnitHasTimedEvent(tData.pUnit, sHandleDoMissileDelayed, CheckEventParam123, event_data ) )
				{
					UnitRegisterEventTimed(tData.pUnit, sHandleDoMissileDelayed, event_data, nDelay);
				}
				dwSeed = RandGetNum( tRand );
			}
		}

	}

	return TRUE;

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileNovaCommon(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	DWORD dwFireMissileOptions,
	float fSpreadAngle )
{

	int nMissileClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	int nMissileCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	BOOL bIncrementSeed = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ) != 0;
	
	nMissileCount += sSkillEventGetExtraBulletCount(tData.pUnit, tData.nSkill, tData.pSkillData);

	//Fire Missile just bails if 0 missiles are released via the SKillEvent FireMissile. SoI'm making Nova follow the same logic.
	if( nMissileCount == 0 )
		return TRUE;

	ASSERT_RETFALSE(nMissileCount > 0);
	float fRotationAngle = fSpreadAngle / nMissileCount;

	// if this function appears on a profile or seems too slow, we could table the vectors.
	MATRIX mRotation;
	MatrixRotationYawPitchRoll( mRotation, 0.0f, 0.0f, fRotationAngle );
	VECTOR vMoveDirection( 0.0f, 1.0f, 0.0f );

	VECTOR vTarget = UnitGetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0 );
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( ! pTarget && ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_PLACE_ON_TARGET ) != 0 )
		return FALSE;

	UNIT * pWeapon    = UnitGetById( tData.pGame, tData.idWeapon );

	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );

	const UNIT_DATA * missile_data = MissileGetData(tData.pGame, nMissileClass);
	ASSERT_RETFALSE(missile_data);

	VECTOR vPosition;
	VECTOR vDirectionOut;
	SkillGetFiringPositionAndDirection( tData.pGame, tData.pUnit, tData.nSkill, tData.idWeapon, tData.pSkillEvent, missile_data,
		vPosition, &vDirectionOut, pTarget, &vTarget, dwSeed );
	if( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
	{
		vPosition = vTarget;
		if( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET )
		{
			vPosition += tData.pSkillEvent->tAttachmentDef.vPosition;
		}
	}
	// what will be the "front" of the nova, and offset the direction to move so we're facing it
	float fFrontAngle = 0.0f;
	if (TESTBIT( dwFireMissileOptions, FMO_RANDOM_START_ANGLE_BIT ))
	{
		RAND tRand;
		RandInit( tRand, dwSeed );
		fFrontAngle = RandGetFloat( tRand, TWOxPI );  //7.0f * (TWOxPI / 8.0f);
		MATRIX mRotationFront;
		MatrixRotationYawPitchRoll( mRotationFront, 0.0f, 0.0f, fFrontAngle );
		MatrixMultiplyNormal( &vMoveDirection, &vMoveDirection, &mRotationFront );
	}
	if (TESTBIT( dwFireMissileOptions, FMO_USE_WEAPON_TO_AIM ))
	{
		vMoveDirection = vDirectionOut;
		MATRIX mHalfSpread;
		MatrixRotationYawPitchRoll( mHalfSpread, 0.0f, 0.0f, (fSpreadAngle+fRotationAngle)/-2.0f  );
		MatrixMultiplyNormal( &vMoveDirection, &vMoveDirection, &mHalfSpread );
	}
	

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	sGetWeaponsForDamage( tData, pWeapons );

	RAND tFiringDirectionRand;
	RandInit( tFiringDirectionRand, dwSeed );

	float fNovaStart = fFrontAngle;
	WORD wMissileTag = GameGetMissileTag(tData.pGame);
	for ( int i = 0; i < nMissileCount; i++ )
	{
		MatrixMultiplyNormal( &vMoveDirection, &vMoveDirection, &mRotation );
		fNovaStart += fRotationAngle;
		if (fNovaStart > TWOxPI)
		{
			fNovaStart = fNovaStart - TWOxPI;
		}

		// get launch position
		VECTOR vPositionLaunch = vPosition;

		// offset launch position to be just outside of the collision extents of the source
		if (TESTBIT( dwFireMissileOptions, FMO_FIRE_NOVA_AROUND_BIT ))
		{
			UNIT *pUnitAround = tData.pUnit;

			// use target unit if we're supposed to
			if ( pTarget && ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_PLACE_ON_TARGET ) != 0 )
			{
				pUnitAround = pTarget;
			}

			if (pUnitAround)
			{
				float flRadius = UnitGetCollisionRadius( pUnitAround );
				VECTOR vLaunchOffset;
				VectorScale( vLaunchOffset, vMoveDirection, flRadius + 0.5f );
				VectorAdd( vPositionLaunch, vPositionLaunch, vLaunchOffset );
			}

		}

		VECTOR vFireDirection = vMoveDirection;
		if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_RANDOM_FIRING_DIRECTION)
		{
			MATRIX tRotation;
			VECTOR vSide;
			VectorCross(vSide, vFireDirection, VECTOR(0.0f, 0.0f, 1.0f));
			MatrixRotationAxis(tRotation, vSide, RandGetFloat(tFiringDirectionRand, -PI/8.0f, PI/8.0f));
			MatrixMultiplyNormal(&vFireDirection, &vFireDirection, &tRotation);
			VectorNormalize(vFireDirection);
		}

		// fire the missile
		UNIT * pMissile = MissileFire(
			tData.pGame,
			nMissileClass,
			tData.pUnit,
			pWeapons,
			tData.nSkill,
			pTarget,
			vTarget,
			vPositionLaunch,
			vFireDirection,
			bIncrementSeed ? ( dwSeed + i ) : dwSeed,
			wMissileTag );

		if ( pMissile && IS_CLIENT( tData.pGame ) )
		{
			int nParticleSystem = MissileGetParticleSystem( pMissile, MISSILE_EFFECT_PROJECTILE );

			if ( nParticleSystem != INVALID_ID )
			{
				c_ParticleSystemSetParam( nParticleSystem, PARTICLE_SYSTEM_PARAM_NOVA_ANGLE, fRotationAngle );
				c_ParticleSystemSetParam( nParticleSystem, PARTICLE_SYSTEM_PARAM_NOVA_START, fNovaStart );
			}
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileNova(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	return sSkillEventFireMissileNovaCommon( tData, 0, TWOxPI );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileNovaAround(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	DWORD dwFlags = 0;
	SETBIT( dwFlags, FMO_FIRE_NOVA_AROUND_BIT );
	return sSkillEventFireMissileNovaCommon( tData, dwFlags, TWOxPI );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileNovaRandomPivot(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	DWORD dwFlags = 0;
	SETBIT( dwFlags, FMO_RANDOM_START_ANGLE_BIT );
	return sSkillEventFireMissileNovaCommon( tData, dwFlags, TWOxPI );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileSpread(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	// Param2 is the missile spread, in radians
	DWORD dwFlags = 0;
	SETBIT( dwFlags, FMO_USE_WEAPON_TO_AIM );
	float fParam2 = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	return sSkillEventFireMissileNovaCommon( tData, dwFlags, fParam2);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileAttachRope(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nMissileCount = MAX( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 1 );
	REF(nMissileCount);

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );
	RAND tRand;
	RandInit( tRand, dwSeed );

	WORD wMissileTag = GameGetMissileTag(tData.pGame);
	UNIT * pMissile = sSkillEventFireMissileCommon(
		tData.pGame,
		tData.pUnit,
		tData.pSkillEvent,
		tData.idWeapon,
		dwSeed,
		tData.nSkill,
		wMissileTag,
		0,
		0.0f,
		0 );

	if ( IS_CLIENT( tData.pGame ) )
	{
		ASSERT_RETFALSE( pMissile );
		ATTACHMENT_DEFINITION tAttachDef;
		MemoryCopy( &tAttachDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );

		int nTrail = c_MissileEffectAttachToUnit( tData.pGame, tData.pUnit, pWeapon, UnitGetClass( pMissile ), MISSILE_EFFECT_TRAIL, tAttachDef,
												  ATTACHMENT_OWNER_SKILL, tData.nSkill );

		e_AttachmentDefInit( tAttachDef );
		tAttachDef.eType = ATTACHMENT_PARTICLE_ROPE_END;

		int nMissileModelId = c_UnitGetModelId( pMissile );
		ASSERT_RETFALSE( nMissileModelId != INVALID_ID );

		int nAttachmentId = c_ModelAttachmentAdd( nMissileModelId, tAttachDef, "", FALSE,
			ATTACHMENT_OWNER_SKILL, tData.nSkill, nTrail );
		REF(nAttachmentId);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireMissileGibs(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_CLIENT(tData.pGame));
	int nMissileCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	ASSERT_RETFALSE(nMissileCount > 0);

	int nMissileClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	VECTOR vPosition;
	int nModelId = c_UnitGetModelId( tData.pUnit );
	if ( nModelId == INVALID_ID )
	{
		vPosition = UnitGetPosition( tData.pUnit );
	} else {
		vPosition = tData.pSkillEvent->tAttachmentDef.vPosition;
		const MATRIX * pmMatrix = e_ModelGetWorldScaled( nModelId );
		MatrixMultiply( &vPosition, &vPosition, pmMatrix );
	}

	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_ADD_TO_CENTER)
	{
		vPosition = UnitGetCenter(tData.pUnit);
	}

	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, NULL, tData.nSkill );
	for ( int i = 0; i < nMissileCount; i++ )
	{
		VECTOR vDirection = RandGetVector(tData.pGame);
		if ( vDirection.fZ < 0.2f )
		{
			vDirection.fZ += 0.5f;
		}
		if (vDirection.fZ <= 0.0f)
		{
			vDirection.fZ = RandGetFloat(tData.pGame);
		}
		VectorNormalize( vDirection, vDirection );
		VECTOR vTarget = vPosition + vDirection;
		MissileFire(tData.pGame, nMissileClass, tData.pUnit, NULL, tData.nSkill, NULL, vTarget,
			vPosition, vDirection, dwSeed, 0);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sSetParticleSystemRopeBends(
	UNIT * pUnit,
	int nParticleSystem )
{
	ASSERT_RETURN( IS_CLIENT( pUnit ) );
	int nModelId = c_UnitGetModelId( pUnit );
	int nAppearanceDef = c_AppearanceGetDefinition( nModelId );
	float fMaxBendXY = 0.0f;
	float fMaxBendZ  = 0.0f;
	c_AppearanceDefinitionGetMaxRopeBends( nAppearanceDef, fMaxBendXY, fMaxBendZ );
	c_ParticleSystemSetParam( nParticleSystem, PARTICLE_SYSTEM_PARAM_ROPE_MAX_BENDXY, fMaxBendXY );
	c_ParticleSystemSetParam( nParticleSystem, PARTICLE_SYSTEM_PARAM_ROPE_MAX_BENDZ,  fMaxBendZ );
}
#endif //!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//static void sLaserDoPulseOnDamage(
//	GAME* pGame,
//	UNIT* pUnit,
//	UNIT *,
//	EVENT_DATA* pHandlerData,
//	void* data,
//	DWORD dwId )
//{
//	int nSkill = pHandlerData->m_Data1;
//	STATS * pDamageStats = (STATS *) data;
//
//	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
//
//	int code_len = 0;
//	BYTE * code_ptr = ExcelGetScriptCode( pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnPulseServerOnly, &code_len );
//	if ( ! code_ptr )
//		return;
//
//	int nDamageDone = StatsGetStat( pDamageStats, STATS_DAMAGE_DONE );
//	if ( ! nDamageDone )
//		return;
//
//	int nDamageAvoided = StatsGetStat( pDamageStats, STATS_DAMAGE_AVOIDED );
//
//	int nHealPercent = (100 * nDamageDone)/(nDamageDone + nDamageAvoided);
//
//	int nSkillLevel = sSkillEventDataGetSkillLevel(tData); // SkillGetLevel( pUnit, nSkill );
//
//	VMExecI(pGame, pUnit, NULL, 0, nSkillLevel, code_ptr, code_len );
//
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define LASER_STICKY_VALUE 0.80f
static BOOL sIsValidLaserTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	const VECTOR & vMuzzlePos,
	const VECTOR & vMuzzleDirIn,
	int nTargetIndex,
	UNIT * pTargetUnit,
	BOOL bLaserTurns,
	BOOL bDoLineCollide )
{
	if ( nTargetIndex != INVALID_ID )
	{
		int nTargetIndexCurr = SkillGetTargetIndex( tData.pUnit, tData.nSkill, tData.idWeapon, UnitGetId( pTargetUnit ));
		if ( nTargetIndexCurr != INVALID_ID && nTargetIndexCurr != nTargetIndex )
			return FALSE;
	}

	VECTOR vTarget = IsUnitDeadOrDying( pTargetUnit ) ? UnitGetCenter( pTargetUnit ) : UnitGetAimPoint( pTargetUnit );
	VECTOR vDelta = vTarget - vMuzzlePos;
	float fDistance = VectorNormalize( vDelta ) - UnitGetCollisionRadius( pTargetUnit );

	if ( fDistance < 1.0f )
		return TRUE;

	if ( bDoLineCollide )
	{
		LEVEL* pLevel = UnitGetLevel( tData.pUnit );
		float fCollideDist = LevelLineCollideLen( tData.pGame, pLevel, vMuzzlePos, vDelta, fDistance );

		if ( fCollideDist < fDistance )
		{
			return FALSE;
		}
	}

	// remove the z factor from this - unless you are aiming up or down
	VECTOR vMuzzleDir = vMuzzleDirIn;
	if ( fabsf(vMuzzleDir.fZ) < 0.4f )
	{
		vMuzzleDir.fZ = 0.0f;
		vDelta.fZ = 0.0f;
		VectorNormalize( vMuzzleDir );
		VectorNormalize( vDelta );
	}

	float fDirectionDotTarget = VectorDot( vDelta, vMuzzleDir );
	if ( !bLaserTurns && fDirectionDotTarget < LASER_STICKY_VALUE )
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sVerifyLaserTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	const VECTOR & vPosition,
	const VECTOR & vDirection,
	int nTargetIndex,
	UNIT * pTargetUnit,
	UNIT * pWeapon,
	BOOL bLaserTurns,
	BOOL bStartingSkill)
{
	BOOL bIsInRange = FALSE;
	if ( ! sIsValidLaserTarget( tData, vPosition, vDirection, nTargetIndex, pTargetUnit, bLaserTurns, TRUE ) ||
		! SkillIsValidTarget( tData.pGame, tData.pUnit, pTargetUnit, pWeapon, tData.nSkill, bStartingSkill, &bIsInRange ) ||
		! bIsInRange )
	{
		return FALSE;
	} else {
		return TRUE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sFindLaserTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	float fRange,
	int nTargetIndex,
	VECTOR & vNormal,
	float & fDistanceToWall,
	DWORD dwSelectFlags,
	const VECTOR & vMuzzlePos,
	const VECTOR & vMuzzleDirIn )
{
	BOOL bLaserTurns = ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LASER_TURNS ) != 0;
	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

	UNIT * pTargetUnit = NULL;
	if ( nTargetIndex == 0 )
	{ // this is the primary target for the laser - so do line selection code
		float fDistance = 0.0f;
		UNIT * pNewTargetUnit = SelectTarget( tData.pGame, tData.pUnit, dwSelectFlags, vMuzzlePos, vMuzzleDirIn,
			fRange, NULL, &fDistance, &vNormal, tData.nSkill, pWeapon );

		if ( pNewTargetUnit )
		{
			if ( SkillGetTargetIndex( tData.pUnit, tData.nSkill, tData.idWeapon, UnitGetId( pNewTargetUnit )) == INVALID_ID )
			{
				pTargetUnit = pNewTargetUnit;
			}
		}

		if ( pTargetUnit &&
			!sVerifyLaserTarget(tData, vMuzzlePos, vMuzzleDirIn, nTargetIndex, pTargetUnit, pWeapon, bLaserTurns, FALSE))
		{
			pTargetUnit = NULL;
		}

		if ( ! pTargetUnit )
			fDistanceToWall = fDistance;
	}
	else
	{ // this is a secondary target - find something near the primary target

		UNIT * pPrimaryTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, 0 );

		if ( ! pPrimaryTarget )
		{

		} else {
			// where will we search around
			VECTOR vSearchCenter = UnitGetPosition( pPrimaryTarget );

			const int MAX_LASER_TARGETS = MAX_TARGETS_PER_QUERY;
			UNITID idUnits[ MAX_LASER_TARGETS ];

			float fRadius = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );

			// setup the targeting query
			SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
			tTargetingQuery.fMaxRange = fRadius;
			tTargetingQuery.pSeekerUnit = tData.pUnit;
			tTargetingQuery.pvLocation = &vSearchCenter;
			tTargetingQuery.pnUnitIds = idUnits;
			tTargetingQuery.nUnitIdMax = MAX_LASER_TARGETS;
			SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
			SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );

			// get the units in range
			SkillTargetQuery( tData.pGame, tTargetingQuery );

			for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
			{
				pTargetUnit = UnitGetById( tData.pGame, idUnits[ i ] );
				ASSERT( pTargetUnit );
				if ( SkillGetTargetIndex( tData.pUnit, tData.nSkill, tData.idWeapon, idUnits[ i ] ) == INVALID_ID &&
					sVerifyLaserTarget(tData, vMuzzlePos, vMuzzleDirIn, nTargetIndex, pTargetUnit, pWeapon, bLaserTurns, FALSE))
				{
					pTargetUnit = UnitGetById( tData.pGame, idUnits[ i ] );
					break;
				} else {
					pTargetUnit = NULL;
				}
			}
		}
	}

	return pTargetUnit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sLaserUpdateAttachment(
	 const SKILL_EVENT_FUNCTION_DATA & tData,
	 UNIT * pWeapon,
	 int nMissile,
	 int nTargetIndex,
	 UNIT * pTargetUnit,
	 const VECTOR & vTarget,
	 const VECTOR & vNormal,
	 float fDuration,
	 BOOL bHitSomething,
	 BOOL bForceDrawRope = FALSE )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( IS_CLIENT( tData.pGame ) );
	ATTACHMENT_DEFINITION tAttachDef;
	MemoryCopy( &tAttachDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );

	c_AttachmentSetWeaponIndex ( tData.pGame, tAttachDef, tData.pUnit, tData.idWeapon );

	// use Trail if we don't have a target unit or location
	// use Projectile if we have a target or we don't have a Trail system
	UNIT_DATA * pMissileData = (UNIT_DATA *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(), DATATABLE_MISSILES, nMissile);
	ASSERT_RETURN( pMissileData );
	if ( ! pMissileData->MissileEffects[ MISSILE_EFFECT_PROJECTILE ].bInitialized )
	{
		c_MissileDataInitParticleEffectSets( pMissileData );
	}
	ASSERT( pMissileData->MissileEffects[ MISSILE_EFFECT_PROJECTILE ].bInitialized );
	int nTrailParticleDefId			= pMissileData->MissileEffects[ MISSILE_EFFECT_TRAIL ].nDefaultId;
	int nProjectileParticleDefId	= pMissileData->MissileEffects[ MISSILE_EFFECT_PROJECTILE ].nDefaultId;

	int nModelId = c_UnitGetModelId( tData.pUnit );
	c_AttachmentDefinitionLoad( tData.pGame, tAttachDef, c_AppearanceGetDefinition( nModelId ), "" );

	tAttachDef.eType = ATTACHMENT_PARTICLE;

	tAttachDef.nAttachedDefId = nTrailParticleDefId;
	int nTrailAttachment = nTrailParticleDefId != INVALID_ID ? c_ModelAttachmentFind( nModelId, tAttachDef ) : INVALID_ID;

	tAttachDef.nAttachedDefId = nProjectileParticleDefId;
	int nProjectileAttachment = c_ModelAttachmentFind( nModelId, tAttachDef );

	BOOL bUseTrail = FALSE;
	if ( nTrailParticleDefId != INVALID_ID )
	{
		if ( ! pTargetUnit && ! bHitSomething )
			bUseTrail = TRUE;
	}

	if ( bUseTrail && nProjectileAttachment != INVALID_ID )
		c_ModelAttachmentRemove( nModelId, nProjectileAttachment, 0 );
	if ( !bUseTrail && nTrailAttachment != INVALID_ID )
		c_ModelAttachmentRemove( nModelId, nTrailAttachment, 0 );

	int nParticleSystem = c_MissileEffectAttachToUnit( tData.pGame, tData.pUnit, pWeapon, nMissile,
		bUseTrail ? MISSILE_EFFECT_TRAIL : MISSILE_EFFECT_PROJECTILE, tAttachDef,
		ATTACHMENT_OWNER_SKILL, tData.nSkill, fDuration, FALSE );
	if ( nParticleSystem != INVALID_ID )
	{
		if ( bForceDrawRope || nTargetIndex == 0 || pTargetUnit )
		{
			if ( ! bUseTrail || UnitDataTestFlag( pMissileData, UNIT_DATA_FLAG_SET_ROPE_END_WITH_NO_TARGET ) )
				c_ParticleSystemSetRopeEnd( nParticleSystem, vTarget, vNormal);
			else
				c_ParticleSystemReleaseRopeEnd( nParticleSystem );

			MISSILE_EFFECT eEffect = pTargetUnit ? MISSILE_EFFECT_IMPACT_UNIT : MISSILE_EFFECT_IMPACT_WALL;
			c_MissileEffectUpdateRopeEnd( tData.pGame, tData.pUnit, pWeapon, nMissile, eEffect, nParticleSystem, ! bHitSomething );

			c_sSetParticleSystemRopeBends( tData.pUnit, nParticleSystem );
		} else {
			// remove the extra ropes when they don't have a target
			c_ParticleSystemKill( nParticleSystem );
		}
	}
#endif// !ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleTriggerAimLaser(
   GAME* pGame,
   UNIT* pUnit,
   UNIT* ,
   EVENT_DATA* pHandlerData,
   void* data,
   DWORD dwId );
//----------------------------------------------------------------------------
static void sLaserRegisterEvent(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	UNIT * pWeapon,
	int nTargetIndex,
	float fDuration )
{
	if(UnitTestFlag(tData.pUnit, UNITFLAG_DEACTIVATED))
	{
		return;
	}

	// the client's control unit needs to update their aiming much more frequently
	if ( IS_CLIENT( tData.pUnit ) && tData.pUnit == GameGetControlUnit( tData.pGame ) && pWeapon )
	{
		DWORD dwEventId = UnitGetStat( pWeapon, STATS_SKILL_EVENT_ID, nTargetIndex );
		// we might need to see if we have it registered for this skill vs another skill
		if ( dwEventId == INVALID_ID || ! UnitHasEventHandler( tData.pGame, tData.pUnit, EVENT_AIMUPDATE, dwEventId ) )
		{
			UnitRegisterEventHandlerRet( dwEventId, tData.pGame, tData.pUnit, EVENT_AIMUPDATE, sHandleTriggerAimLaser,
				EVENT_DATA(tData.nSkill, (DWORD_PTR)tData.pSkillEvent, tData.idWeapon, tData.nSkillLevel));
			UnitSetStat( pWeapon, STATS_SKILL_EVENT_ID, nTargetIndex, dwEventId );
		}
	}
	else
	{
		SkillRegisterEvent( tData.pGame, tData.pUnit, tData.pSkillEvent, tData.nSkill, tData.nSkillLevel, tData.idWeapon, 2.0f * GAME_TICK_TIME, fDuration );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAimLaser(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nMissile = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	ASSERT_RETFALSE(nMissile != INVALID_ID);

	if ( ! SkillIsOn( tData.pUnit, tData.nSkill, tData.idWeapon ) )
		return FALSE;

	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	SKILL_DATA * pSkillData = (SKILL_DATA *) ExcelGetData( tData.pGame, DATATABLE_SKILLS, tData.nSkill );

	if ( IS_SERVER( tData.pUnit ) )
	{
		int code_len = 0;
		BYTE * code_ptr = ExcelGetScriptCode( tData.pGame, DATATABLE_SKILLS, pSkillData->codeStartCondition, &code_len );
		if ( code_ptr )
		{
			if ( ! VMExecI(tData.pGame, tData.pUnit, NULL, 0, nSkillLevel, pSkillData->nId, nSkillLevel, INVALID_ID, code_ptr, code_len ) )
			{
				sSkillEventSafeStopSkill( tData, TRUE, TRUE );
				SkillUpdateActiveSkills( tData.pUnit );
				return FALSE;
			}
		}
	}

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

	int nTargetIndex = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	float fDuration = tData.fDuration;

	if ( tData.pSkillEvent->tCondition.nType != INVALID_ID &&
		! ConditionEvalute( tData.pUnit, tData.pSkillEvent->tCondition, tData.nSkill, tData.idWeapon ) )
	{
		if ( IS_CLIENT( tData.pGame ) )
			c_sLaserUpdateAttachment( tData, pWeapon, nMissile, nTargetIndex, NULL, VECTOR( 0 ), VECTOR( 0, 0, 1 ), fDuration, FALSE );
		sLaserRegisterEvent( tData, pWeapon, nTargetIndex, fDuration );
		return FALSE;
	}

	const UNIT_DATA * missile_data = MissileGetData(tData.pGame, nMissile);
	ASSERT_RETFALSE(missile_data);

	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapon, tData.nSkill );

	BOOL bLaserTurns = ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LASER_TURNS ) != 0;

	
	UNITID pOldTargetId = SkillGetTargetId( tData.pUnit, tData.nSkill, tData.idWeapon, nTargetIndex );
	UNIT * pOldTarget = UnitGetById(tData.pGame, pOldTargetId);
	UNIT * pTargetUnit = pOldTarget;

	VECTOR vPosition;
	VECTOR vDirection;
	SkillGetFiringPositionAndDirection( tData.pGame, tData.pUnit, tData.nSkill, tData.idWeapon, tData.pSkillEvent, missile_data,
									vPosition, &vDirection, pTargetUnit, NULL, dwSeed );
	//VECTOR vDirection = UnitGetFaceDirection( tData.pUnit, FALSE );

	BOOL b360Targeting = (tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_360_TARGETING) != 0;
	BOOL bHitSomething = FALSE;
	VECTOR vNormal = VECTOR( 0, 0, 1 );
	VECTOR vTarget( 0 );

	BOOL bGameControlsTargeting = TRUE;
	if ( IS_SERVER( tData.pGame ) && UnitHasClient( tData.pUnit ) )
		bGameControlsTargeting = FALSE;
	if ( IS_CLIENT( tData.pGame ) && GameGetControlUnit( tData.pGame ) != tData.pUnit )
		bGameControlsTargeting = FALSE;

#if  !ISVERSION(SERVER_VERSION)
	if ( bGameControlsTargeting && IS_CLIENT( tData.pGame ) && !pTargetUnit && nTargetIndex == 0 )
	{
		UNITID idTarget = UIGetTargetUnitId();
		if ( SkillGetTargetIndex( tData.pUnit, tData.nSkill, UnitGetId( pWeapon ), idTarget ) < 1 )
			pTargetUnit = UnitGetById( tData.pGame, idTarget );
	}
#endif// !ISVERSION(SERVER_VERSION)

	// check to see if the current target is still valid
	float fDistanceToWall = 0.0f;
	if ( pTargetUnit )
	{
		if ( bGameControlsTargeting || IS_SERVER(tData.pGame) )
		{
			if(!sVerifyLaserTarget(tData, vPosition, vDirection, nTargetIndex, pTargetUnit, pWeapon, bLaserTurns, FALSE))
			{
				pTargetUnit = NULL;
				vTarget = VECTOR( 0 );
			}
			else
			{
				vTarget = UnitGetAimPoint( pTargetUnit );
			}
		}
	}

	// Some skills require a target
	if ( !pTargetUnit && ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_REQUIRES_TARGET ) != 0 )
	{
		sSkillEventSafeStopSkill( tData, TRUE );
		return TRUE;
	}

	float fRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeapon );

	// look for a new target
	if ( !pTargetUnit && bGameControlsTargeting )
	{
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		{
			vTarget = UnitGetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
		}

		if ( bGameControlsTargeting && VectorIsZero( vTarget ) )
		{
			// KCK: Added SELECTTARGET_FLAG_CHECK_SLIM_AND_FAT because otherwise los checks were not being made at all with some lasers
			DWORD dwSelectFlags = SELECTTARGET_FLAG_FATTEN_UNITS | SELECTTARGET_FLAG_CHECK_LOS_TO_AIM_POS | SELECTTARGET_FLAG_CHECK_SLIM_AND_FAT;

			if (b360Targeting)
			{
				dwSelectFlags |= SELECTTARGET_FLAG_IGNORE_DIRECTION;
			}

			pTargetUnit = sFindLaserTarget( tData, fRange, nTargetIndex, vNormal,
				fDistanceToWall, dwSelectFlags, vPosition, vDirection );

			if (!pTargetUnit && nTargetIndex == 0 )
			{
				pTargetUnit = sFindLaserTarget( tData, fRange, nTargetIndex, vNormal,
					fDistanceToWall, dwSelectFlags | SELECTTARGET_FLAG_ALLOW_NOTARGET, vPosition, vDirection );
			}
		}
	}

	if ( bGameControlsTargeting && pOldTarget != pTargetUnit )
	{
		// save this target
		SkillSetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, UnitGetId( pTargetUnit ), nTargetIndex );
	}

	if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_CANNOT_RETARGET ) &&
		 (pOldTarget || pOldTargetId != INVALID_ID) && (pOldTarget != pTargetUnit || pOldTargetId != UnitGetId(pTargetUnit)) )
	{
		// KCK: Don't force a retarget if the destroyed thing was destructible
		if (pOldTarget && !UnitIsA(pOldTarget, UNITTYPE_DESTRUCTIBLE))
		{
			trace("***** Stopping Skill: %c\n", IS_SERVER(pOldTarget));
			sSkillEventSafeStopSkill( tData, TRUE );
			return TRUE;
		}
		else
		{
			SkillClearTargets( tData.pUnit, tData.nSkill, tData.idWeapon );
			SkillClearTargets( tData.pUnit, tData.nSkill, INVALID_ID );
		}
	}

	// now see if you can find a backup target - you don't set it or send it, but you should draw it and do damage to it
	BOOL bForceDrawRope = FALSE;
	if ( ! pTargetUnit )
	{
		enum {
			NUM_BACKUP_TARGETS = 2,
		};
		int pnBackups[ NUM_BACKUP_TARGETS ] = { SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 ), SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 ) };

		if ( pnBackups[ 0 ] != INVALID_ID || nTargetIndex == 0 )
		{
			bForceDrawRope = TRUE;

			for ( int i = 0; i < NUM_BACKUP_TARGETS; i++ )
			{
				pTargetUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, pnBackups[ i ] );
				if ( pTargetUnit )
				{
					if(!sVerifyLaserTarget(tData, vPosition, vDirection, INVALID_ID, pTargetUnit, pWeapon, bLaserTurns, FALSE))
					{
						pTargetUnit = NULL;
					}
					else
					{
						break;
					}
				}
			}

			if ( ! pTargetUnit )
			{
				pTargetUnit = SkillGetAnyTarget( tData.pUnit, tData.nSkill, pWeapon, TRUE );
				if(pTargetUnit && !sVerifyLaserTarget(tData, vPosition, vDirection, INVALID_ID, pTargetUnit, pWeapon, bLaserTurns, FALSE))
				{
					pTargetUnit = NULL;
				}
			}
		}
	}

	if(tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_LASER_DONT_TARGET_UNITS)
	{
		pTargetUnit = NULL;
	}

	VECTOR vWallNormal(0, 0, 1);
	// update target position and direction given our current target
	if ( IS_CLIENT( tData.pGame ) || tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LASER_ATTACKS_LOCATION )
	{
		if ( ! pTargetUnit )
		{
			if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
			{
				vTarget = UnitGetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, nTargetIndex );
//				bHitSomething = TRUE;
			}
			else 
			{
				// change aiming to follow the muzzle if you don't have a target
				//if ( IS_CLIENT( tData.pUnit ) )
				//{
				//	SkillGetFiringPositionAndDirection( tData.pGame, tData.pUnit, tData.nSkill, UnitGetId(pWeapon), tData.pSkillEvent,
				//		missile_data, vPosition, &vDirection, pTargetUnit, NULL, dwSeed, TRUE );
				//}

				if ( fDistanceToWall == 0.0f ) // somehow we haven't done a collision check yet
				{
					LEVEL* pLevel = UnitGetLevel( tData.pUnit );
					fDistanceToWall = LevelLineCollideLen( tData.pGame, pLevel, vPosition, vDirection, fRange, &vWallNormal );
				}

				VECTOR vDelta = vDirection;
				vDelta *= fDistanceToWall - missile_data->fHitBackup;
				vTarget = vPosition + vDelta;
				if ( fDistanceToWall >= fRange )
				{
					if ( ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LASER_SEEKS_SURFACES ) != 0
						&& (IS_CLIENT( tData.pGame ) || tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LASER_ATTACKS_LOCATION) )
					{
						LEVEL* pLevel = UnitGetLevel( tData.pUnit );
#define MAX_LASER_DROP 100.0f
						float fDist = LevelLineCollideLen( tData.pGame, pLevel, vTarget, VECTOR(0, 0, -1), MAX_LASER_DROP, &vWallNormal );
						if ( fDist < MAX_LASER_DROP )
						{
							vTarget.fZ -= fDist - missile_data->fHitBackup;
							bHitSomething = TRUE;
						}
					}
				} else {
					bHitSomething = TRUE;
				}
			}
		}
		else
		{ // you have a target unit
			vTarget = UnitGetAimPoint( pTargetUnit );
			bHitSomething = TRUE;
			if ( IS_CLIENT( tData.pGame ) )
			{
				BOOL bRadiusBump = FALSE;
				if ( ( SkillDataTestFlag( pSkillData, SKILL_FLAG_TARGET_DEAD ) || SkillDataTestFlag( pSkillData, SKILL_FLAG_TARGET_DYING_AFTER_START ) ) &&
					IsUnitDeadOrDying( tData.pUnit ) )
				{
					int nModelId = c_UnitGetModelId( pTargetUnit );
					REF(nModelId);
#ifdef HAVOK_ENABLED
					VECTOR vRagdollPos;
					BOOL bRagdollPos = c_RagdollGetPosition( nModelId, vRagdollPos );
					if ( bRagdollPos && ! VectorIsZero( vRagdollPos ) )
					{
						vTarget = vRagdollPos;
						vDirection = vTarget - UnitGetPosition( tData.pUnit );
						VectorNormalize( vDirection, vDirection );
					}
					else
#endif
					{
						bRadiusBump = TRUE;
					}
				}
				else if (pTargetUnit == GameGetControlUnit(tData.pGame) && !AppGetDetachedCamera())
				{
					const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( TRUE );
					VECTOR vDown( 0, 0, -1 );
					VECTOR vRight;
					VECTOR vCameraDirection = CameraInfoGetDirection(pCameraInfo);
					VectorCross( vRight, vCameraDirection, vDown );
					VectorNormalize( vRight, vRight );
					VectorCross( vDown, vRight, vCameraDirection );
					VectorNormalize( vDown, vDown );
					vDown *= 0.185f;
					VECTOR vLookOffset = vCameraDirection;
					vLookOffset *= 0.2f;
					vTarget += vDown;
					vTarget += vLookOffset;
					vDirection = vTarget - UnitGetPosition( tData.pUnit );
					VectorNormalize( vDirection, vDirection );
				} else {
					bRadiusBump = TRUE;
				}

				if ( bRadiusBump )
				{
					VECTOR vRadiusBump = vPosition - vTarget;
					VectorNormalize( vRadiusBump );
//					vRadiusBump *= missile_data->fHitBackup;
					vRadiusBump *= (UnitGetCollisionRadius( pTargetUnit ) * 2.0f) / 3.0f + missile_data->fHitBackup;
					vTarget += vRadiusBump;
					vDirection = vTarget - UnitGetPosition( tData.pUnit );
					VectorNormalize( vDirection, vDirection );
				}
			} else {
				vDirection = vTarget - UnitGetPosition( tData.pUnit );
				VectorNormalize( vDirection, vDirection );
			}
		}
		UnitSetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0, vTarget );
	}

	if ( (tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FACE_TARGET) != 0 && pTargetUnit )
	{ // this is done independently on the client and server
		UnitSetFaceDirection( tData.pUnit, vDirection, FALSE, TRUE );
	}

	// update the particle system
	if ( IS_CLIENT( tData.pGame ) )
	{
		c_sLaserUpdateAttachment( tData, pWeapon, nMissile, nTargetIndex, pTargetUnit, vTarget, vNormal, fDuration, bHitSomething, bForceDrawRope );
	}

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if ( IS_SERVER( tData.pGame ) )
	{
		if ( pTargetUnit &&
			sVerifyLaserTarget(tData, vPosition, vDirection, INVALID_ID, pTargetUnit, pWeapon, bLaserTurns, TRUE) &&
			 missile_data && !UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_CLIENT_ONLY ) &&
			 ! UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_ATTACKS_LOCATION_ON_HIT_UNIT ) )
		{
			if ( !SkillDataTestFlag( pSkillData, SKILL_FLAG_DRAINS_POWER ) ||
				 UnitGetStat( tData.pUnit, STATS_POWER_CUR ) != 0 )
			{
				STATS * pStats = NULL;
				if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_PULSES_STATS_ON_HIT_UNIT ) ||
					 UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DAMAGES_ON_HIT_UNIT ) )
				{
					int code_len = 0;
					BYTE * code_ptr = ExcelGetScriptCode( tData.pGame, DATATABLE_SKILLS, pSkillData->codeStatsOnPulseServerOnly, &code_len );
					if ( code_ptr )
					{
						pStats = StatsListInit( tData.pGame );
						ASSERT_RETFALSE( pStats );
						VMExecI(tData.pGame, tData.pUnit, pTargetUnit, pStats, 0, nSkillLevel, pSkillData->nId, nSkillLevel, INVALID_ID, code_ptr, code_len );
						StatsListAdd(tData.pUnit, pStats, TRUE, SELECTOR_SKILL);
					}
				}

				if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DAMAGES_ON_HIT_UNIT ) )
				{
#ifdef HAVOK_ENABLED
					HAVOK_IMPACT tImpact;
					HavokImpactInit( tData.pGame, tImpact, missile_data->fForce, vPosition, &vDirection );

					if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_FORCE_IGNORES_SCALE ))
					{
						tImpact.dwFlags |= HAVOK_IMPACT_FLAG_IGNORE_SCALE;
					}
					tImpact.dwFlags |= HAVOK_IMPACT_FLAG_ONLY_ONE;
#endif
					UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
					sGetWeaponsForDamage( tData, pWeapons );

					float fDamageMultiplier = -1;

					if( AppIsTugboat() )
					{
						fDamageMultiplier = (float)missile_data->nServerSrcDamage / 100.0f;
						if(  SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_SPELL) && SkillDataTestFlag(pSkillData, SKILL_FLAG_IS_RANGED ))
						{
							fDamageMultiplier += UnitGetStat( UnitGetUltimateOwner( tData.pUnit ), STATS_DAMAGE_PERCENT_SPELL );
						}
					}
					s_UnitAttackUnit( tData.pUnit,
						pTargetUnit,
						pWeapons,
						0
#ifdef HAVOK_ENABLED
						,&tImpact
#endif
						,0,
						fDamageMultiplier );
				}

				if ( pStats )
				{
					StatsListRemove( tData.pGame, pStats );
					StatsListFree( tData.pGame, pStats );
				}
			}
		}
		else
		{
			if ( pTargetUnit )
			{
				if(sVerifyLaserTarget(tData, vPosition, vDirection, INVALID_ID, pTargetUnit, pWeapon, bLaserTurns, TRUE))
				{
					vTarget = UnitGetPosition( pTargetUnit );
				}
			}
			if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LASER_ATTACKS_LOCATION && !VectorIsZero(vTarget))
			{
#ifdef HAVOK_ENABLED
				HAVOK_IMPACT tImpact;
				HavokImpactInit( tData.pGame, tImpact, missile_data->fForce, vPosition, &vDirection );
				if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_FORCE_IGNORES_SCALE ))
				{
					tImpact.dwFlags |= HAVOK_IMPACT_FLAG_IGNORE_SCALE;
				}
				tImpact.dwFlags |= HAVOK_IMPACT_FLAG_ONLY_ONE;
#endif
				UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
				sGetWeaponsForDamage( tData, pWeapons );

				UNIT * pAttacker = pWeapons[0] ? pWeapons[0] : tData.pUnit;

				s_UnitAttackLocation(pAttacker,
									 pWeapons,
									 0,
									 UnitGetRoom(tData.pUnit),
									 vTarget,
#ifdef HAVOK_ENABLED
									 tImpact,
#endif
									 0);
			}
		}
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	sLaserRegisterEvent( tData, pWeapon, nTargetIndex, fDuration );

    return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAimLaserToAll(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	return sSkillEventAimLaser(	tData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleTriggerAimLaser(
	GAME* pGame,
	UNIT* pUnit,
	UNIT *,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId )
{
	int nSkill = (int)pHandlerData->m_Data1;
	int nSkillLevel = (int)pHandlerData->m_Data4;
	SKILL_EVENT * pSkillEvent = (SKILL_EVENT *)pHandlerData->m_Data2;
	UNITID idWeapon = (UNITID)pHandlerData->m_Data3;
	SKILL_EVENT_FUNCTION_DATA tData( pGame, pUnit, pSkillEvent, nSkill, nSkillLevel, SkillGetData( pGame, nSkill ), idWeapon );
	sSkillEventAimLaser( tData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sCheckStopLaser(
	GAME* pGame,
	UNIT* unit,
	const EVENT_DATA& event_data,
	const EVENT_DATA* check_data)
{
	int nSkillFromEvent = (int)event_data.m_Data1;
	UNITID idWeaponFromEvent = (UNITID)event_data.m_Data2;
	SKILL_EVENT * pSkillEventFromEvent = (SKILL_EVENT *)event_data.m_Data3;

	int nSkillFromCheck = (int)check_data->m_Data1;
	UNITID idWeaponFromCheck = (UNITID)check_data->m_Data2;
	int nSkillEventFromCheck = (int)check_data->m_Data3;

	if(nSkillFromCheck != nSkillFromEvent ||
		idWeaponFromCheck != idWeaponFromEvent ||
		nSkillEventFromCheck != pSkillEventFromEvent->nType)
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStopLaser(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nFireLaserEventTypeIndex = ExcelGetLineByStringIndex(tData.pGame, DATATABLE_SKILLEVENTTYPES, "Fire Laser");
	EVENT_DATA tEventData(tData.nSkill, tData.idWeapon, nFireLaserEventTypeIndex, NULL, 0);
	UnitUnregisterTimedEvent(tData.pUnit, SkillHandleSkillEvent, sCheckStopLaser, &tEventData);
	if ( IS_CLIENT( tData.pGame ))
	{
		UNIT * pWeapon = UnitGetById(tData.pGame, tData.idWeapon);
		SkillClearEventHandler( tData.pGame, tData.pUnit, pWeapon, EVENT_AIMUPDATE );
		c_ModelAttachmentRemoveAllByOwner( c_UnitGetModelId( tData.pUnit ), ATTACHMENT_OWNER_SKILL, tData.nSkill );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSpreadFire(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
//	ConsolePrintString( CONSOLE_SYSTEM_COLOR, "** SPREAD FIRE **" );
	ASSERTX_RETFALSE( tData.pUnit, "Expected unit to spread fire" );
	float flRadius = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	// get room to spread fire from
	ROOM *pRoom = UnitGetRoom( tData.pUnit );
	ASSERTX_RETFALSE( pRoom, "Unit spreading fire is in no room" );

	// setup buffer for units
	const int MAX_SPREAD_FIRE_UNITS = MAX_TARGETS_PER_QUERY;
	UNITID idUnits[ MAX_SPREAD_FIRE_UNITS ];

	// who was ultimate source of this special effect is
	UNIT *pAttacker = tData.pUnit;
	UNIT *pSFXStateGivenBy = NULL;
	UNITID idSFXStateGivenBy = UnitGetStat( tData.pUnit, STATS_SFX_STATE_GIVEN_BY, DAMAGE_TYPE_FIRE );
	if (AppIsTugboat())
	{
		static unsigned int s_idCatchOnFire = ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_DAMAGE_EFFECTS, "catch_onfire");
		if (s_idCatchOnFire != EXCEL_LINK_INVALID)
		{
			UNITID idSFXEffectGivenBy = UnitGetStat(tData.pUnit, STATS_SFX_EFFECT_GIVEN_BY, s_idCatchOnFire);
			if (idSFXEffectGivenBy != INVALID_ID)
			{
				idSFXStateGivenBy = idSFXEffectGivenBy;
			}
		}
	}
	if (idSFXStateGivenBy != INVALID_ID)
	{
		pSFXStateGivenBy = UnitGetById( tData.pGame, idSFXStateGivenBy );
	}
	if (pSFXStateGivenBy)
	{
		pAttacker = pSFXStateGivenBy;
	}

	// where will we search around
	VECTOR vSearchCenter = UnitGetPosition( tData.pUnit );

	// setup the targeting query
	SKILL_TARGETING_QUERY tTargetingQuery;
	tTargetingQuery.fMaxRange = flRadius;
	tTargetingQuery.pSeekerUnit = pSFXStateGivenBy ? pSFXStateGivenBy : tData.pUnit;
	tTargetingQuery.pvLocation = &vSearchCenter;
	tTargetingQuery.pnUnitIds = idUnits;
	tTargetingQuery.nUnitIdMax = MAX_SPREAD_FIRE_UNITS;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );

	// if we know who the firestarter was, we want to find only enemies of them
	if (pSFXStateGivenBy)
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	}
	else
	{
		// otherwise, we burn everybody!
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	}
	// get the units in range
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	// go through each result
	int nMaxToSpreadFireTo = 1;  // spread to a max of this # of units this execution
	int nNumUnitsSpreadTo = 0;
	for (int i = 0; i < tTargetingQuery.nUnitIdCount; ++i)
	{
		UNIT *pVictim = UnitGetById( tData.pGame, idUnits[ i ] );

		// only spread this fire to enemies of the state giver (ie. a player sets a monster on fire,
		// that fire will only spread to other monsters or other enemy players and pets)
		if (pSFXStateGivenBy)
		{
			if (TestHostile( tData.pGame, pSFXStateGivenBy, pVictim ) == FALSE)
			{
				continue;
			}
		}

		if( AppIsTugboat() )
		{
			s_CombatExplicitSpecialEffect( pVictim, pAttacker, NULL, 1, DAMAGE_TYPE_FIRE );
			// see if we should stop now
			if (nNumUnitsSpreadTo >= nMaxToSpreadFireTo)
			{
				break;
			}
			continue;
		}

		// only try this if victim is no on fire already
		if (UnitHasExactState( pVictim, STATE_SFX_FIRE ) == FALSE)
		{

			// try effect
			s_CombatExplicitSpecialEffect( pVictim, pAttacker, NULL, 1, DAMAGE_TYPE_FIRE );

			// did it spread
			if (UnitHasExactState( pVictim, STATE_SFX_FIRE ) == TRUE)
			{
				// we spread fire
				nNumUnitsSpreadTo++;

				// set state for caught fire (will do fx)
				s_StateSet( pVictim, tData.pUnit, STATE_SPREAD_FIRE, 1000 );

				//// debug info
				//const int MAX_MESSAGE = 1024;
				//char szMessage[ MAX_MESSAGE ];
				//PStrPrintf( szMessage, MAX_MESSAGE, "Spread fire to '%s'\n", UnitGetDevName( pVictim ) );
				//ConsolePrintString( CONSOLE_SYSTEM_COLOR, szMessage );

				// see if we should stop now
				if (nNumUnitsSpreadTo >= nMaxToSpreadFireTo)
				{
					break;
				}
			}
		}
	}

	// if we caught any units on fire, set our spread fire state on for a while
	if (nNumUnitsSpreadTo > 0)
	{
		s_StateSet( tData.pUnit, tData.pUnit, STATE_SPREAD_FIRE, 1000 );
	}

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
// Check out Units.cpp and function UnitResurrect. - marsh
//----------------------------------------------------------------------------
static BOOL sSkillEventResurrect(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );  //Ask for target - what about multiple targets?
	if ( ! pTarget )
		return FALSE;
	s_UnitRestoreVitals( pTarget, FALSE );
	UnitEndMode(pTarget, MODE_DYING, 0, TRUE);
	UnitEndMode(pTarget, MODE_DEAD, 0, TRUE);
	s_UnitSetMode( pTarget, MODE_IDLE, TRUE ); //set the target to idle.

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFireHose(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	if ( IS_SERVER( tData.pGame ) )
	{
		sSkillEventFireMissileCommon( tData.pGame, tData.pUnit, tData.pSkillEvent, tData.idWeapon, 0, tData.nSkill, 0, 0, 0.0f, 0 );
	}
	else if (IS_CLIENT(tData.pGame))
	{
#if !ISVERSION(SERVER_VERSION)
		ATTACHMENT_DEFINITION tAttachDef;
		MemoryCopy(&tAttachDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof(ATTACHMENT_DEFINITION));

		c_AttachmentSetWeaponIndex(tData.pGame, tAttachDef, tData.pUnit, tData.idWeapon);

		UNIT * pWeapon = UnitGetById(tData.pGame, tData.idWeapon);

		int nMissileClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
		int nParticleSystem = c_MissileEffectAttachToUnit(tData.pGame, tData.pUnit, pWeapon, nMissileClass, MISSILE_EFFECT_PROJECTILE,
															tAttachDef, ATTACHMENT_OWNER_SKILL, tData.nSkill, tData.fDuration);
		if (nParticleSystem != INVALID_ID)
		{
			const UNIT_DATA * missile_data = MissileGetData(tData.pGame, nMissileClass);
			ASSERT_RETFALSE(missile_data);
			float fRangePercent = MissileGetRangePercent(tData.pUnit, pWeapon, missile_data);
			float fPressure = fRangePercent * missile_data->tVelocities[0].fVelocityBase;
			fPressure = PIN(fPressure, missile_data->tVelocities[0].fVelocityMin, missile_data->tVelocities[0].fVelocityMax);
			c_ParticleSystemSetParam( nParticleSystem, PARTICLE_SYSTEM_PARAM_HOSE_PRESSURE, fPressure );
			c_sSetParticleSystemRopeBends(tData.pUnit, nParticleSystem);
		}
#endif// !ISVERSION(SERVER_VERSION)
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAddMuzzleFlash(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT( tData.pGame ) );
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );

	int nMissileClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	sGetWeaponsForDamage( tData, pWeapons );

	int nWeapons = 0;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{ // add a muzzle flash on all of the units that are doing the damage - this helps with focus items
		if ( ! pWeapons[ i ] )
			continue;
		nWeapons++;
		ATTACHMENT_DEFINITION tAttachDef;
		MemoryCopy( &tAttachDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );

		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FORCE_NEW )
			tAttachDef.dwFlags |= ATTACHMENT_FLAGS_FORCE_NEW;

		c_AttachmentSetWeaponIndex ( tData.pGame, tAttachDef, tData.pUnit, UnitGetId( pWeapons[ i ] ) );

		c_MissileEffectAttachToUnit( tData.pGame, tData.pUnit, pWeapons[ i ], nMissileClass, MISSILE_EFFECT_MUZZLE,
			tAttachDef, ATTACHMENT_OWNER_SKILL, tData.nSkill, 0, FALSE );
	}
	// hey, this might be for something that doesn't use weapons! do it the old fashioned way.
	if( nWeapons == 0 )
	{
		ATTACHMENT_DEFINITION tAttachDef;
		MemoryCopy( &tAttachDef, sizeof(ATTACHMENT_DEFINITION), &tData.pSkillEvent->tAttachmentDef, sizeof( ATTACHMENT_DEFINITION ) );

		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_FORCE_NEW )
			tAttachDef.dwFlags |= ATTACHMENT_FLAGS_FORCE_NEW;

		UNIT * pWeapon = UnitGetById(tData.pGame, tData.idWeapon);

		c_AttachmentSetWeaponIndex ( tData.pGame, tAttachDef, tData.pUnit, tData.idWeapon );

		c_MissileEffectAttachToUnit( tData.pGame, tData.pUnit, pWeapon, nMissileClass, MISSILE_EFFECT_MUZZLE,
										tAttachDef, ATTACHMENT_OWNER_SKILL, tData.nSkill, 0, FALSE );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	int nSkillOnTarget )
{
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	if ( UnitGetGenus( pTarget ) == GENUS_PLAYER )
		return TRUE;

	float fPercentChance = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	if (RandGetFloat(tData.pGame) > fPercentChance)
	{
		return FALSE;
	}

	const UNIT_DATA * pTargetData = UnitGetData( pTarget );
	if ( pTargetData && UnitDataTestFlag(pTargetData, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
		return FALSE;

	UNIT * pWeapon		= UnitGetById( tData.pGame, tData.idWeapon );

	if ( SkillIsValidTarget( tData.pGame, tData.pUnit, pTarget, pWeapon, tData.nSkill,  FALSE ))
	{
		if ( nSkillOnTarget != INVALID_ID )
		{
			DWORD dwFlags = SKILL_START_FLAG_DONT_SEND_TO_SERVER;
			if ( IS_SERVER( pTarget ) )
				dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest(
				tData.pGame,
				pTarget,
				nSkillOnTarget,
				INVALID_ID,
				VECTOR(0),
				dwFlags,
				SkillGetNextSkillSeed(tData.pGame),
				tData.nSkillLevel);
		}
		UnitFree(pTarget, UFF_SEND_TO_CLIENTS);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveSelf(
						  const SKILL_EVENT_FUNCTION_DATA & tData,
						  int nSkillOnTarget )
{
	UNIT * pTarget = tData.pUnit;
	if ( ! pTarget )
		return FALSE;

	if ( UnitGetGenus( pTarget ) == GENUS_PLAYER )
		return TRUE;

	float fPercentChance = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	if (RandGetFloat(tData.pGame) > fPercentChance)
	{
		return FALSE;
	}

	const UNIT_DATA * pTargetData = UnitGetData( pTarget );
	if ( pTargetData && UnitDataTestFlag(pTargetData, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD))
		return FALSE;

	UNIT * pWeapon		= UnitGetById( tData.pGame, tData.idWeapon );

	if ( SkillIsValidTarget( tData.pGame, tData.pUnit, pTarget, pWeapon, tData.nSkill,  FALSE ))
	{
		if ( nSkillOnTarget != INVALID_ID )
		{
			DWORD dwFlags = SKILL_START_FLAG_DONT_SEND_TO_SERVER;
			if ( IS_SERVER( pTarget ) )
				dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest(
				tData.pGame,
				pTarget,
				nSkillOnTarget,
				INVALID_ID,
				VECTOR(0),
				dwFlags,
				SkillGetNextSkillSeed(tData.pGame),
				tData.nSkillLevel);
		}
		UnitFree(pTarget, UFF_SEND_TO_CLIENTS);
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFindNextTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nTargetIndex = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	ASSERT_RETFALSE( nTargetIndex >= 0 );

	enum {
		NUM_TARGETS_TO_CONSIDER = 4,
	};
	UNITID pnNearest[ NUM_TARGETS_TO_CONSIDER ];

	ASSERT_RETNULL( tData.pSkillData );

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	tTargetingQuery.pnUnitIds = pnNearest;
	tTargetingQuery.nUnitIdMax = NUM_TARGETS_TO_CONSIDER;
	tTargetingQuery.fMaxRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeapon, FALSE, tData.nSkillLevel );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );

	if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_IS_MELEE ) )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE );
	}
	tTargetingQuery.fDirectionTolerance = tData.pSkillData->fFiringCone;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	if ( tTargetingQuery.nUnitIdCount == 0 )
		return TRUE;

	UNIT * pTargetPrev = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, MAX(0, nTargetIndex - 1) );
	UNITID idTargetPrev = UnitGetId( pTargetPrev );

	UNITID idNewTarget = INVALID_ID;
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{ // to be somewhat deterministic and therefore better synced on client and server, pick the next highest target id
		if ( tTargetingQuery.pnUnitIds[ i ] > idTargetPrev &&
			( idNewTarget == INVALID_ID || tTargetingQuery.pnUnitIds[ i ] < idNewTarget ) )
			idNewTarget = tTargetingQuery.pnUnitIds[ i ];
	}
	if ( idNewTarget == INVALID_ID )
	{// if we could not find the next highest ID, then pick the lowest ID
		for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
		{
			if ( idNewTarget == INVALID_ID || tTargetingQuery.pnUnitIds[ i ] < idNewTarget )
				idNewTarget = tTargetingQuery.pnUnitIds[ i ];
		}
	}

	if ( idNewTarget != INVALID_ID )
	{
		SkillSetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, idNewTarget, nTargetIndex );
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRemoveTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	return sRemoveTarget( tData, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRemoveSelf(
									const SKILL_EVENT_FUNCTION_DATA & tData )
{
	return sRemoveSelf( tData, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFaceTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	VECTOR vDirection = UnitGetPosition(pTarget) - UnitGetPosition(tData.pUnit);
	UnitSetFaceDirection( tData.pUnit, vDirection, FALSE, TRUE );
	return TRUE;
}

//----------------------------------------------------------------------------
enum SKILL_SPAWN_MONSTER_FLAG
{
	SSMF_USE_POSITION_AS_IS_BIT,
	SSMF_IS_PET_BIT,
	SSMF_FACE_SPAWNER_DIRECTION_BIT,
	SSMF_GIVE_TARGET_INVENTORY_BIT,
	SSMF_DONT_ADD_PET_TO_INVENTORY_BIT,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSkillSpawnGetPositionAndDirection(
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vInitialPosition,
	VECTOR & vPosition,
	VECTOR & vDirection,
	ROOM *& pSpawnRoom,
	ROOM_PATH_NODE *& pSpawnPathNode,
	int nSkill,
	int nIndex,
	DWORD dwSpawnMonsterFlags,
	DWORD dwSkillEventFlags,
	const MATRIX & matUnitWorld)
{
	if (TESTBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT ))
	{
		vPosition = vPosition;
	}
	else
	{
		if (dwSkillEventFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET)
		{
			VECTOR vPositionIn = vPosition;
			MatrixMultiply(&vPosition, &vPositionIn, &matUnitWorld);
			vInitialPosition = vPosition;

			VECTOR vNormal;
			MatrixMultiplyNormal(&vNormal, &vDirection, &matUnitWorld);
			VectorNormalize( vNormal );
			VECTOR vDir = vNormal;
			float fMagnitude = RandGetFloat(pGame, 0.0f, 2.0f);
			vDir.fZ = 0.0f;
			vPosition += fMagnitude * vDir;
		}
		else if(dwSkillEventFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION)
		{
			vPosition = UnitGetStatVector(pUnit, STATS_SKILL_TARGET_X, nSkill, nIndex);
		}
		else if(dwSkillEventFlags & SKILL_EVENT_FLAG_ADD_TO_CENTER)
		{
			vPosition = UnitGetPosition(pUnit);
			pSpawnPathNode = PathStructGetEstimatedLocation(pUnit, pSpawnRoom);
		}
		else
		{
			VECTOR vRandom = RandGetVector(pGame);
			vRandom.fZ = 0.0f;
			vPosition = UnitGetPosition(pUnit);
			vPosition += vRandom;
		}
	}

	// figure out a direction to face
	if (TESTBIT( dwSpawnMonsterFlags, SSMF_FACE_SPAWNER_DIRECTION_BIT ))
	{
		vDirection = UnitGetDirectionXY( pUnit );
	}
	else
	{
		vDirection = RandGetVector( pGame );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static UNIT * sSkillDoSpawnMonster(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	int nMonsterClass,
	int nMonsterQuality,
	int nCount,
	DWORD dwSkillEventFlags,
	VECTOR & vPos,
	VECTOR & vNorm,
	UNITID idWeapon,
	DWORD dwSpawnMonsterFlags,
	ESTATE StateToSet = STATE_NONE,
	int nFuseTicks = 0)
{
	int nActualSpawnCount = 0;
	MATRIX matUnitWorld;
	UnitGetWorldMatrix(pUnit, matUnitWorld);
	UNIT * pUnitToReturn = NULL;
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	const UNIT_DATA * pMonsterData = MonsterGetData( pGame, nMonsterClass );
	ROOM * pSpawnRoom = UnitGetRoom( pUnit );
	ROOM_PATH_NODE * pSpawnPathNode = NULL;
	for(int i=0; i<nCount; i++)
	{
		VECTOR vInitialPosition(0);
		VECTOR vPosition = vPos;
		VECTOR vDirection = vNorm;
		sSkillSpawnGetPositionAndDirection(pGame, pUnit, vInitialPosition, vPosition, vDirection, pSpawnRoom, pSpawnPathNode, nSkill, i, dwSpawnMonsterFlags, dwSkillEventFlags, matUnitWorld);

		MONSTER_SPEC tSpec;
		tSpec.nClass = nMonsterClass;
		tSpec.nExperienceLevel = UnitGetExperienceLevel( pUnit );
		tSpec.nMonsterQuality = nMonsterQuality;
		tSpec.pRoom = pSpawnRoom;
		tSpec.pPathNode = pSpawnPathNode;
		tSpec.vPosition = vPosition;
		tSpec.vDirection = vDirection;
		tSpec.vDirection.fZ = 0.0f;  // only care about X/Y direction
		VectorNormalize( tSpec.vDirection );
		if (TESTBIT( dwSpawnMonsterFlags, SSMF_IS_PET_BIT ))
		{
			tSpec.pOwner = pUnit;
		}

		UNIT * pNewUnit = s_MonsterSpawn( pGame, tSpec );
		if (!pNewUnit)
		{
			continue;
		}
		nActualSpawnCount++;
		pUnitToReturn = pNewUnit;

		if(nFuseTicks > 0)
		{
			// KCK: Set the stats here, let the stat change callback set the actual fuse
			DWORD dwNowTick = GameGetTick( UnitGetGame(pNewUnit) );
			DWORD dwEndTick = dwNowTick + nFuseTicks;
			UnitSetStatShift(pNewUnit, STATS_FUSE_START_TICK, dwNowTick);
			UnitSetStatShift(pNewUnit, STATS_FUSE_END_TICK, dwEndTick);
//			UnitSetFuse(pNewUnit, nFuseTicks);
		}

		if (TESTBIT( dwSpawnMonsterFlags, SSMF_IS_PET_BIT ) && !TESTBIT( dwSpawnMonsterFlags, SSMF_DONT_ADD_PET_TO_INVENTORY_BIT ))
		{
			// TRAVIS: We have a skill that needs to check pets in the minor pet slot, and
			// add them in the major pet slot - so we use the first invloc for the add, but we
			// have to have both so that the pet check can find the minor pets
			//ASSERTX( pSkillData->pnSummonedInventoryLocations[ 1 ] == INVALID_ID, pSkillData->szName ); // it doesn't make sense to have two inventory locations here
			if(!PetAdd( pNewUnit, pUnit, pSkillData->pnSummonedInventoryLocations[ 0 ], nSkill, SkillDataTestFlag(pSkillData, SKILL_FLAG_REDUCE_POWER_MAX_BY_POWER_COST) ? SkillGetPowerCost( pUnit, pSkillData, nSkillLevel) : 0) )
			{
				UnitFree( pNewUnit, UFF_SEND_TO_CLIENTS );
				continue;
			}
		}

		if ( pSkillData->nSummonedAI != INVALID_ID )
		{
			AI_InsertTable( pNewUnit, pSkillData->nSummonedAI );
		}

		// transfer stats from skill to unit (if any)
		SkillTransferStatsToUnit( pNewUnit, pUnit, nSkill, nSkillLevel, SELECTOR_PET_BONUSES );

		if(SkillDataTestFlag(pSkillData, SKILL_FLAG_TRANSFER_DAMAGES_TO_PETS))
		{
			UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
			sGetWeaponsForDamage(pGame, pUnit, nSkill, pSkillData, idWeapon, pWeapons);
			UnitTransferDamages(pNewUnit, pUnit, pWeapons, 0, TRUE, FALSE);
		}

		BOOL bHasMode;
		float fDuration = UnitGetModeDuration( pGame, pNewUnit, MODE_SPAWN, bHasMode );
		if (bHasMode)
		{
			if(pMonsterData->fSpawnRandomizePercent != 0.0f)
			{
				fDuration += RandGetFloat(pGame, -fDuration * pMonsterData->fSpawnRandomizePercent, fDuration * pMonsterData->fSpawnRandomizePercent);
			}
			s_UnitSetMode(pNewUnit, MODE_SPAWN, 0, fDuration);
		}

		if ( TESTBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT ) == FALSE &&
			 (dwSkillEventFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET))
		{
			VECTOR vSpawnedPosition = UnitGetPosition(pNewUnit);
			VECTOR vRand = RandGetVector(pGame);
			vRand.fZ = fabs(vRand.fZ);
			VECTOR vNormal(0);
			vNormal = vInitialPosition - vSpawnedPosition;
			vNormal.fZ = vNorm.fZ;
			VectorNormalize(vNormal);

			UnitChangePosition(pNewUnit, vPosition);
			MSG_SCMD_UNITPOSITION posMsg;
			posMsg.id = UnitGetId(pNewUnit);
			posMsg.bFlags = 0;
			posMsg.TargetPosition = vPosition;
			s_SendUnitMessage(pNewUnit, SCMD_UNITPOSITION, &posMsg);

			if(!(dwSkillEventFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE))
			{
				if ( ! UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_SPAWN_IN_AIR) )
					UnitStartJump(pGame, pNewUnit, vNormal.fZ + vRand.fZ, TRUE, TRUE);
				UnitSetMoveDirection(pNewUnit, vNormal);
				UnitSetVelocity(pNewUnit, VectorLength(vNorm));
				UnitStepListAdd(pGame, pNewUnit);
			}

			UnitSetFaceDirection( pNewUnit, vNormal, TRUE, TRUE );

			MSG_SCMD_UNITTURNXYZ turnMsg;
			turnMsg.id = UnitGetId(pNewUnit);
			turnMsg.vFaceDirection = vNormal;
			turnMsg.vUpDirection = VECTOR(0, 0, 1);
			turnMsg.bImmediate = TRUE;
			s_SendUnitMessage(pNewUnit, SCMD_UNITTURNXYZ, &turnMsg);

			if(!(dwSkillEventFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE))
			{
				// send move msg
				s_SendUnitMoveDirection( pNewUnit, 0, MODE_IDLE, vNormal );
			}
		}

		if(TESTBIT(dwSpawnMonsterFlags, SSMF_GIVE_TARGET_INVENTORY_BIT))
		{

			UNIT * pSkillTarget = SkillGetTarget(pUnit, nSkill, INVALID_ID, 0);
			if(UnitHasInventory(pSkillTarget))
			{
				int nInventoryCount = UnitInventoryGetCount(pSkillTarget);
				for(int i=0; i<nInventoryCount; i++)
				{
					UNIT * pInventoryItem = UnitInventoryGetByIndex(pSkillTarget, 0);
					int nOriginalLocation, nOriginalX, nOriginalY;
					UnitGetInventoryLocation(pInventoryItem, &nOriginalLocation, &nOriginalX, &nOriginalY);

					InventoryChangeLocation(
						pNewUnit,
						pInventoryItem,
						nOriginalLocation,
						nOriginalX,
						nOriginalY,
						0);
				}
			}

			int nLocation, nX, nY;
			if(ItemGetOpenLocation(pNewUnit, pSkillTarget, FALSE, &nLocation, &nX, &nY))
			{
				InventoryChangeLocation(pNewUnit, pSkillTarget, nLocation, nX, nY, 0);
			}
		}
	}
	if( AppIsTugboat() && StateToSet != STATE_NONE ) //Tugboat set the state of the unit spawned
	{
		s_StateSet( pUnitToReturn, pUnit, StateToSet, 0 );
	}
	return pUnitToReturn;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)

static BOOL sSkillEventSpawnMonsterFromTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT *pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, INVALID_ID );
	if( pTarget == NULL )
		return FALSE;
	DWORD dwSpawnMonsterFlags( 0 );
	SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	int nCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nFuseTicks = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	VECTOR vPosition = UnitGetPosition( pTarget );
	VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	sSkillDoSpawnMonster(
		tData.pGame,
		tData.pUnit,
		tData.nSkill,
		nSkillLevel,
		tData.pSkillEvent->tAttachmentDef.nAttachedDefId,
		INVALID_LINK,
		nCount,
		tData.pSkillEvent->dwFlags,
		vPosition,
		vNormal,
		tData.idWeapon,
		dwSpawnMonsterFlags,
		STATE_NONE,
		nFuseTicks);

	return TRUE;
}




static BOOL sSkillEventSpawnMonster(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	int nCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nFuseTicks = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	VECTOR vPosition = tData.pSkillEvent->tAttachmentDef.vPosition;
	VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	sSkillDoSpawnMonster(
		tData.pGame,
		tData.pUnit,
		tData.nSkill,
		nSkillLevel,
		tData.pSkillEvent->tAttachmentDef.nAttachedDefId,
		INVALID_LINK,
		nCount,
		tData.pSkillEvent->dwFlags,
		vPosition,
		vNormal,
		tData.idWeapon,
		0,
		STATE_NONE,
		nFuseTicks);

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnObject(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	VECTOR vInitialPosition(0);
	VECTOR vPosition = tData.pSkillEvent->tAttachmentDef.vPosition;
	VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
	ROOM * pSpawnRoom = UnitGetRoom( tData.pUnit );
	ROOM_PATH_NODE * pSpawnPathNode = NULL;
	MATRIX matUnitWorld;
	UnitGetWorldMatrix(tData.pUnit, matUnitWorld);
	sSkillSpawnGetPositionAndDirection(tData.pGame, tData.pUnit, vInitialPosition, vPosition, vNormal, pSpawnRoom, pSpawnPathNode, tData.nSkill, 0, 0, tData.pSkillEvent->dwFlags, matUnitWorld);

	OBJECT_SPEC tSpec;
	tSpec.nClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	tSpec.nLevel = 0;
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = vPosition;
	tSpec.pvFaceDirection = &vNormal;  // this seems wrong -Colin
	s_ObjectSpawn( tData.pGame, tSpec );
	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnMonsterNearby(
	const SKILL_EVENT_FUNCTION_DATA & tData)
{
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	UNIT *pUnit = tData.pUnit;
	GAME *pGame = tData.pGame;
	int nCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float flRadius = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float flMinRadius = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	int nFuseTicks = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );
	VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	int nMonsterClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	// fill out spec for locations
	NEAREST_NODE_SPEC tSpec;
	SETBIT(tSpec.dwFlags, NPN_CHECK_LOS);
	tSpec.fMinDistance = ( flMinRadius <= 0 ? 1.0f : flMinRadius );
	tSpec.fMaxDistance = flRadius;
	const int MAX_PATH_NODES = 64;
	FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

	int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pUnit), UnitGetPosition(pUnit), MAX_PATH_NODES, tFreePathNodes, &tSpec);

	if (nNumNodes > 0)
	{

		// spawn all the monsters
		for (int i = 0; i < nCount; ++i)
		{

			// select a location
			int nPick = RandGetNum( pGame, 0, nNumNodes - 1 );
			const FREE_PATH_NODE *pFreePathNode = &tFreePathNodes[ nPick ];
			VECTOR vPosition = RoomPathNodeGetWorldPosition( pGame, pFreePathNode->pRoom, pFreePathNode->pNode );
			DWORD dwFlags = 0;
			SETBIT( dwFlags, SSMF_USE_POSITION_AS_IS_BIT );
			// do spawn
			sSkillDoSpawnMonster(
				tData.pGame,
				tData.pUnit,
				tData.nSkill,
				nSkillLevel,
				nMonsterClass,
				INVALID_LINK,
				1,
				tData.pSkillEvent->dwFlags,
				vPosition,
				vNormal,
				tData.idWeapon,
				dwFlags,
				STATE_NONE,
				nFuseTicks);
		}

	}

	return TRUE;

}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
typedef struct FIND_OLDEST_MINOR_PET_DATA
{
	int nMonsterClassToIgnore;
	GAME_TICK tOldestSpawnTick;
	UNIT *pOldestPet;
} FIND_OLDEST_MINOR_PET_DATA;

void sFindOldestMinorPet( UNIT *pPet, void *pUserData )
{
	ASSERT_RETURN(pPet && pUserData);
	FIND_OLDEST_MINOR_PET_DATA *pData = (FIND_OLDEST_MINOR_PET_DATA*)pUserData;

	if (UnitIsA(pPet, UNITTYPE_MINOR_PET) &&
		UnitGetClass(pPet) != pData->nMonsterClassToIgnore)
	{
		GAME_TICK tPetSpawnTick = UnitGetStat(pPet, STATS_SPAWN_TICK);
		if (pData->pOldestPet == NULL || pData->tOldestSpawnTick > tPetSpawnTick)
		{
			pData->pOldestPet = pPet;
			pData->tOldestSpawnTick = tPetSpawnTick;
		}
	}
}

typedef struct COUNT_SUMMONED_MINOR_PET_CLASSES_DATA
{
	int nMonsterClassToIgnore;
	int nNumClasses;
	int nClasses[16];
} COUNT_SUMMONED_MINOR_PET_CLASSES_DATA;

void sCountSummonedMinorPetClasses( UNIT *pPet, void *pUserData )
{
	ASSERT_RETURN(pPet && pUserData);
	COUNT_SUMMONED_MINOR_PET_CLASSES_DATA *pData = (COUNT_SUMMONED_MINOR_PET_CLASSES_DATA*)pUserData;

	if (UnitIsA(pPet, UNITTYPE_MINOR_PET) &&
		UnitGetClass(pPet) != pData->nMonsterClassToIgnore)
	{
		int nPetClass = UnitGetClass(pPet);
		// check if this class is already counted
		ASSERTX_RETURN(pData->nNumClasses < arrsize( pData->nClasses ), "too many types of minor pets summoned, increase array size");
		for (int i = 0; i < pData->nNumClasses; ++i)
			if (pData->nClasses[i] == nPetClass)
				return;

		pData->nClasses[pData->nNumClasses++] = nPetClass;
	}
}

typedef struct ADD_UP_PET_MAX_POWER_PENALTIES_DATA
{
	int nMinorTotal;
	int nMajorTotal;
	UNIT *pOwner;
} ADD_UP_PET_MAX_POWER_PENALTIES_DATA;

void sAddUpPetMaxPowerPenalties( UNIT *pPet, void *pUserData )
{
	ASSERT_RETURN(pPet && pUserData);
	ADD_UP_PET_MAX_POWER_PENALTIES_DATA *pData = (ADD_UP_PET_MAX_POWER_PENALTIES_DATA*)pUserData;
	if (UnitIsA(pPet, UNITTYPE_MINOR_PET))
	{
		pData->nMinorTotal += PetGetPetMaxPowerPenalty(pData->pOwner, pPet);
	}
	else
	{
		pData->nMajorTotal += PetGetPetMaxPowerPenalty(pData->pOwner, pPet);
	}
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sDoSkillEventSpawnPet(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	DWORD dwSpawnFlags,
	VECTOR vPosOverRide = VECTOR( 0, 0, 0 ),
	int nFuseTicks = 0 )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	int nCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nUseOverrideSpawnState = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	BOOL bUseUltimateOwner = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 ) != 0;
	// TRAVIS: We have a skill that needs to check pets in the minor pet slot, and
	// add them in the major pet slot - so we use the first invloc for the add, but we
	// have to have both so that the pet check can find the minor pets
	//ASSERTX( tData.pSkillData->pnSummonedInventoryLocations[ 1 ] == INVALID_ID, tData.pSkillData->szName ); // it doesn't make sense to have two inventory locations here
	int nInventoryLocation = tData.pSkillData->pnSummonedInventoryLocations[ 0 ];

	UNIT* pOwner = bUseUltimateOwner ? UnitGetUltimateOwner( tData.pUnit ) : tData.pUnit;
	int nMaxCount = UnitInventoryGetArea(pOwner, nInventoryLocation);
	if( nMaxCount <= 0 )
	{
		return FALSE;
	}

	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);

	int nCurrentCount = UnitInventoryGetCount(pOwner, nInventoryLocation);

	BOOL bRezOldPets = UnitInventoryTestFlag( pOwner, nInventoryLocation, INVLOCFLAG_RESURRECTABLE );

	if ( bRezOldPets && nCurrentCount > 0 )
	{
		ASSERT( nCurrentCount == nCount );
		ASSERT( nCurrentCount == nMaxCount );

		for ( int i = 0; i < nCurrentCount; i++ )
		{
			UNIT * pPet = UnitInventoryGetByLocationAndXY( pOwner, nInventoryLocation, i, 0 );
			if (! pPet )
				continue;
			if(!PetCanAdd(pOwner, SkillGetPowerCost(pOwner, tData.pSkillData, tData.nSkillLevel), INVALID_ID))
				continue;
			if ( ! UnitGetRoom( pPet ) || IsUnitDeadOrDying( pPet ) )
			{
				VECTOR vPosition;
				VECTOR vDirection;
				ROOM * pRoom = UnitGetRoom( tData.pUnit );
				MATRIX matUnitWorld;
				ROOM_PATH_NODE * pSpawnPathNode = NULL;
				VECTOR vInitialPosition( 0 );

				sSkillSpawnGetPositionAndDirection(tData.pGame, tData.pUnit, vInitialPosition, vPosition,
					vDirection, pRoom, pSpawnPathNode, tData.nSkill, 0, dwSpawnFlags, tData.pSkillEvent->dwFlags, matUnitWorld);

				if ( IsUnitDeadOrDying( pPet ) )
				{
					UnitSetStat( pPet, STATS_HP_CUR, UnitGetStat( pPet, STATS_HP_MAX ) );  // this will also rez the Squire
					if ( IsUnitDeadOrDying( pPet ) )
						UnitClearFlag(pPet, UNITFLAG_DEAD_OR_DYING);
				}

				DWORD dwWarpFlags = 0;
				SETBIT( dwWarpFlags, UW_FORCE_VISIBLE_BIT );
				UnitSetStat( pPet, STATS_NO_DRAW, 0 );
				e_ModelSetDrawAndShadow( c_UnitGetModelId( pPet ), TRUE );
				UnitWarp( pPet, pRoom, vPosition, vDirection, VECTOR(0,0,1), dwWarpFlags );

				ASSERT( ! UnitTestModeGroup( pPet, MODEGROUP_DYING ) );
				ASSERT( ! UnitTestModeGroup( pPet, MODEGROUP_DEAD ) );
				ASSERT( ! IsUnitDeadOrDying( pPet ) );
				ASSERT( UnitGetRoom( pPet ) );
			}

			if ( ! PetIsPet( pPet ) )
			{
				PetAdd( pPet, pOwner, nInventoryLocation, tData.nSkill, SkillDataTestFlag(tData.pSkillData, SKILL_FLAG_REDUCE_POWER_MAX_BY_POWER_COST) ? SkillGetPowerCost(pOwner, tData.pSkillData, tData.nSkillLevel) : 0 );
			}

			// clean off the old bonuses
			STATS * pPetStatsList = StatsGetModList( pPet, SELECTOR_PET_BONUSES );
			BOUNDED_WHILE( pPetStatsList, 100 )
			{
				StatsListRemove( tData.pGame, pPetStatsList );
				StatsListFree( tData.pGame, pPetStatsList );
				pPetStatsList = StatsGetModList( pPet, SELECTOR_PET_BONUSES );
			}

			// transfer stats from skill to unit (if any)
			SkillTransferStatsToUnit( pPet, tData.pUnit, tData.nSkill, tData.nSkillLevel, SELECTOR_PET_BONUSES );

			// Re-evaluate if all equipped items are usable, now that all the stats have been assigned from the masteries
			s_InventoryChanged(pPet);

			s_UnitRestoreVitals(pPet, FALSE);
		}
		return TRUE;
	}

	if(nCount > 0)
	{
		int nNumberToRemove = (nCurrentCount + nCount) - nMaxCount;
		if (nNumberToRemove > 0 )
		{// kill the old pets - right now we are going to do it without selecting which ones to get rid of
			for ( int i = 0; i < nNumberToRemove; i++ )
			{
				// Find the oldest/weakest pet
				UNIT * pOldestPet = NULL;
				UNIT * pWeakestPet = NULL;
				GAME_TICK nOldestTick = GameGetTick(tData.pGame);
				int nLowestHitPointPercentage = 100;
				for(int j=0; j<nMaxCount; j++)
				{
					UNIT * pTestPet = UnitInventoryGetByLocationAndXY( pOwner, nInventoryLocation, j, 0 );
					if(!pTestPet)
					{
						continue;
					}
					GAME_TICK nSpawnTick = UnitGetStat(pTestPet, STATS_SPAWN_TICK);
					if(nSpawnTick <= nOldestTick)
					{
						pOldestPet = pTestPet;
						nOldestTick = nSpawnTick;
					}
					int nHitPointsCur = UnitGetStat(pTestPet, STATS_HP_CUR);
					int nHitPointsMax = UnitGetStat(pTestPet, STATS_HP_MAX);
					int nHitPointPercentage = nHitPointsMax ? ( nHitPointsCur * 100 / nHitPointsMax ) : 0;
					if(nHitPointPercentage <= nLowestHitPointPercentage)
					{
						pWeakestPet = pTestPet;
						nLowestHitPointPercentage = nHitPointPercentage;
					}
				}

				// And kill it!
				if(pWeakestPet)
				{
					UnitDie( pWeakestPet, NULL );
				}
				else if(pOldestPet)
				{
					UnitDie( pOldestPet, NULL );
				}
				else
				{
					break;
				}
			}
		}

	}
	else
	{
		nCount = nMaxCount - nCurrentCount;
	}

	if (nCount)
	{
		if (AppIsHellgate())
		{
			// if power_max isn't large enough to apply the summoning cost, or the
			// max power penalty for the new pet, kill other pets to free up
			// resources
			const UNIT_DATA *pUnitData = MonsterGetData(NULL, tData.pSkillEvent->tAttachmentDef.nAttachedDefId);
			if (pUnitData)
			{
				int nPowerMax = UnitGetStat(pOwner, STATS_POWER_MAX);
				int nPowerCost = SkillGetPowerCost(pOwner, tData.pSkillData, nSkillLevel);
				int nPetMaxPowerPenalty = nPowerCost;
				if (nPetMaxPowerPenalty > nPowerMax || nPowerCost > nPowerMax)
				{
					// first figure out if killing all the minor pets, or all the major, or both, will be enough
					ADD_UP_PET_MAX_POWER_PENALTIES_DATA tAddUpData;
					tAddUpData.nMinorTotal = 0;
					tAddUpData.nMajorTotal = 0;
					tAddUpData.pOwner = pOwner;
					PetIteratePets(pOwner, sAddUpPetMaxPowerPenalties, &tAddUpData);
					int nPowerMaxPlusMinorPets = nPowerMax + tAddUpData.nMinorTotal;

					// kill pets until enough max power is available

					// if killing all the minor pets would yield enough power,
					// then kill the oldest one until the player has enough power
					ASSERTX_RETFALSE(nPetMaxPowerPenalty <= nPowerMaxPlusMinorPets && nPowerCost <= nPowerMaxPlusMinorPets, "failed to sacrifice enough pets to pay power cost to summon the new one, even though previous checks determined it should have been possible");

					FIND_OLDEST_MINOR_PET_DATA tOldestData;
					tOldestData.nMonsterClassToIgnore = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
					do
					{
						// kill minor pets of other classes
						tOldestData.pOldestPet = NULL;
						PetIteratePets(pOwner, sFindOldestMinorPet, &tOldestData);
						if (tOldestData.pOldestPet != NULL)
						{
							UnitDie(tOldestData.pOldestPet, NULL);
							nPowerMax = UnitGetStat(pOwner, STATS_POWER_MAX);
						}
					} while (tOldestData.pOldestPet != NULL && (nPetMaxPowerPenalty > nPowerMax || nPowerCost > nPowerMax));

					if (nPetMaxPowerPenalty > nPowerMax || nPowerCost > nPowerMax)
					{
						// still not enough, kill the oldest of the same class
						tOldestData.nMonsterClassToIgnore = INVALID_LINK;
						do
						{
							// kill minor pets of other classes
							tOldestData.pOldestPet = NULL;
							PetIteratePets(pOwner, sFindOldestMinorPet, &tOldestData);
							if (tOldestData.pOldestPet != NULL)
							{
								UnitDie(tOldestData.pOldestPet, NULL);
								nPowerMax = UnitGetStat(pOwner, STATS_POWER_MAX);
							}
						} while (tOldestData.pOldestPet != NULL && (nPetMaxPowerPenalty > nPowerMax || nPowerCost > nPowerMax));

						ASSERTX_RETFALSE(nPetMaxPowerPenalty <= nPowerMax && nPowerCost <= nPowerMax, "failed to sacrifice enough pets to pay power cost to summon the new one, even though previous checks determined it should have been possible");
					}
				}

				// skill data for "max minor pet types allowed"
				if (tData.pSkillData->nMaxSummonedMinorPetClasses != INVALID_ID)
				{
					// count how many classes of minor pets are summoned, ignoring the current class to summon
					COUNT_SUMMONED_MINOR_PET_CLASSES_DATA tCountData;
					tCountData.nNumClasses = 0;
					tCountData.nMonsterClassToIgnore = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
					PetIteratePets(pOwner, sCountSummonedMinorPetClasses, &tCountData);

					if (tCountData.nNumClasses + 1 > tData.pSkillData->nMaxSummonedMinorPetClasses)
					{
						// kill minor pets in the inventory location of the oldest pet (ignoring the pet type to spawn now)
						int nOldestLocation = INVALID_ID;
						FIND_OLDEST_MINOR_PET_DATA tOldestData;
						tOldestData.nMonsterClassToIgnore = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
						tOldestData.pOldestPet = NULL;
						PetIteratePets(pOwner, sFindOldestMinorPet, &tOldestData);
						if (tOldestData.pOldestPet != NULL &&
							UnitGetInventoryLocation(tOldestData.pOldestPet, &nOldestLocation))
						{
							UNIT *pNextPet = UnitInventoryLocationIterate( pOwner, nOldestLocation, NULL );
							while (pNextPet != NULL)
							{
								UNIT *pPetToKill = pNextPet;
								pNextPet = UnitInventoryLocationIterate( pOwner, nOldestLocation, pNextPet );
								UnitDie(pPetToKill, pPetToKill);
							}
						}
					}
				}
			}
		}

		if(!PetCanAdd(pOwner, SkillGetPowerCost(pOwner, tData.pSkillData, tData.nSkillLevel), tData.pSkillData->pnSummonedInventoryLocations[ 0 ]))
		{
			return FALSE;
		}

		VECTOR vPosition = tData.pSkillEvent->tAttachmentDef.vPosition;
		VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
		DWORD dwSpawnMonsterFlags = dwSpawnFlags;
		if( !VectorIsZero( vPosOverRide ) )
		{
			SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
			vPosition = vPosOverRide;
		}
		SETBIT( dwSpawnMonsterFlags, SSMF_IS_PET_BIT );
		//I hate this. We need to add a flag - marsh
		if( AppIsTugboat() &&
			UnitGetGenus( tData.pUnit ) == GENUS_MISSILE )
		{
			vPosition = UnitGetPosition( tData.pUnit );
			SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
		}

		UNIT *pNewPet =
			sSkillDoSpawnMonster(
				tData.pGame,
				pOwner,
				tData.nSkill,
				nSkillLevel,
				tData.pSkillEvent->tAttachmentDef.nAttachedDefId,
				INVALID_LINK,
				nCount,
				tData.pSkillEvent->dwFlags,
				vPosition,
				vNormal,
				tData.idWeapon,
				dwSpawnMonsterFlags,
				nUseOverrideSpawnState != 0 ? STATE_SPAWNPET : STATE_NONE,
				nFuseTicks );

		// save the power max penalty from the skill on the pet, so it can be
		// undone later
		if (pNewPet)
		{
			if(AppIsHellgate())
			{
				UnitSetStat(pNewPet, STATS_POWER_MAX_PENALTY_PETS, 0, SkillGetPowerCost(pOwner, tData.pSkillData, nSkillLevel));
			}
		}

	}

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//This needs to take a parameter for # of pets, and for the location to nuke
//With the option to still have "every location" and "all pets"
//All pets <- # = 0
static BOOL sSkillEventKillPets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	int nPetsToKill;
	nPetsToKill = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	int nPetsKilled = 0;

	// Right now, there's no way to specify that this kills units in a specific slot, but hey, at least its limited by parameter now
	//This has the id of the monster we want to check...
	const UNIT_DATA *pMonsterData = MonsterGetData(tData.pGame, tData.pSkillEvent->tAttachmentDef.nAttachedDefId);
	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS && (nPetsKilled < nPetsToKill || nPetsToKill <= 0); i++ )
	{
		
		if(tData.pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID )
		{
			continue;
		}

		UNIT * pPet = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(pPet && (nPetsKilled < nPetsToKill || nPetsToKill <= 0))
		{
			UNIT * pPetNext = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], pPet);
			if(pPet && PetIsPet(pPet) && UnitIsA(pPet, pMonsterData->nUnitType))
			{
				nPetsKilled++;
				UnitDie(pPet, NULL);
			}
			pPet = pPetNext;
		}
	}
	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
// KCK: Added to sync the bone wall destruction. Kills all units with of a type
// with the same container, with an option to kill only those with the same
// fuse.
//-----------------------------------------------------------------------------
static BOOL sSkillEventKillAllUnitsOfTypeWithSameContainer(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	bool	bCheckFuse = (SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ))? true:false;
	bool	bIgnoreSkillTargets = (SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 ))? true:false;
	const	UNIT_DATA *pMonsterData = MonsterGetData( tData.pGame, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );

	// Get a list of nearby monsters
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	if (tTargetingQuery.fMaxRange == 0)
		tTargetingQuery.fMaxRange = 20.0f;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	// KCK Hacky: Allow caller to just look for everything within this range, regardless of skill data filter
	if (bIgnoreSkillTargets)
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
		tTargetingQuery.tFilter.nUnitType = UNITTYPE_ANY;
	}
	SkillTargetQuery( tData.pGame, tTargetingQuery );


	UNIT*	pOwner = UnitGetContainer( tData.pUnit );
	if ( !pOwner )				// Try the owner instead
		pOwner = UnitGetById( tData.pGame, UnitGetOwnerId( tData.pUnit ));
	if ( !pOwner )				// Ok, just use us.
		pOwner = tData.pUnit;

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		if ( !UnitTestFlag( pTarget, UNITFLAG_DEAD_OR_DYING ) && 
			 UnitIsA( pTarget, pMonsterData->nUnitType ) &&
			 (UnitGetContainer( pTarget) == pOwner || UnitGetOwnerId( pTarget ) == UnitGetId( pOwner )) &&
			 (!bCheckFuse || UnitGetFuse( pTarget ) == UnitGetFuse( tData.pUnit )) )
		{
			s_UnitKillUnit(tData.pGame, NULL, pTarget, pTarget, NULL);
		}
	}

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static BOOL sSkillEventKillAllUnitsOfTypeWithinRange(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	float fMaxRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	// Get a list of nearby monsters
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.fMaxRange	= fMaxRange;
	if (tTargetingQuery.fMaxRange <= 0.0f)
		tTargetingQuery.fMaxRange = 20.0f;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	tTargetingQuery.tFilter.nUnitType = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for(int i=0; i<tTargetingQuery.nUnitIdCount; i++)
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		UnitDie(pTarget, NULL);
	}
#endif
	return TRUE;
}


//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetAnchorPointOnPets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
	{
		if(tData.pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID)
		{
			continue;
		}

		UNIT * pPet = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(pPet)
		{
			UNIT * pPetNext = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], pPet);
			UnitSetStatVector(pPet, STATS_AI_ANCHOR_POINT_X, 0, UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill));
			pPet = pPetNext;
		}
	}
	return TRUE;
#else
	return FALSE;
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetTargetOverrideOnPets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	UNIT * pTarget = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
	{
		if(tData.pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID)
		{
			continue;
		}

		UNIT * pPet = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(pPet)
		{
			UNIT * pPetNext = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], pPet);
			UnitSetAITarget(pPet, pTarget, TRUE);
			pPet = pPetNext;
		}
	}
	return TRUE;
#else
	return FALSE;
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
// This function sets a stat on the player unit which stores a location that
// the player's pets should move to.
// param 0: pet group (each group can have a separate target location)
//-----------------------------------------------------------------------------
static BOOL sSkillEventSetDesiredPetLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	int nGroup = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	ASSERT_RETFALSE( nGroup >= 0 && nGroup < 8 );

	// If the Unit is a missile, find the originating player and use the missile's position as the target
	UNIT	*pUnit = tData.pUnit;
	VECTOR	vAnchor = UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill);
	if ( UnitIsA( pUnit, UNITTYPE_MISSILE ) )
	{
		pUnit = UnitGetUltimateOwner(pUnit);
		vAnchor = tData.pUnit->vPosition;
	}

	// For now, this is used by players only
	ASSERT_RETFALSE( UnitIsA( pUnit, UNITTYPE_PLAYER ) );

	// Set the desired location, by group #
	UnitSetStatVector(pUnit, STATS_PET_DESIRED_LOCATION_X, nGroup, vAnchor);

	return TRUE;
#else
	return FALSE;
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
// This function sets a stat on the player unit which stores a target that
// the player's pets should attack
// param 0: pet group (each group can have a separate target location)
//-----------------------------------------------------------------------------
static BOOL sSkillEventSetDesiredPetTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	int nGroup = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	ASSERT_RETFALSE( nGroup >= 0 && nGroup < 8 );

	// If the Unit is a missile, find the originating player
	UNIT	*pUnit = tData.pUnit;
	if ( UnitIsA( pUnit, UNITTYPE_MISSILE ) )
		pUnit = UnitGetUltimateOwner(pUnit);

	// For now, this is used by players only
	ASSERT_RETFALSE( UnitIsA( pUnit, UNITTYPE_PLAYER ) );

	// Set the desired target, by group #
	UNIT * pTarget = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
	UnitSetStat(pUnit, STATS_PET_DESIRED_TARGET, UnitGetId(pTarget));

	return TRUE;
#else
	return FALSE;
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
// This is used to clear any commands we've given to our pets.
//-----------------------------------------------------------------------------
static BOOL sSkillEventClearPetCommands(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	// If the Unit is a missile, find the originating player
	UNIT	*pUnit = tData.pUnit;
	if ( UnitIsA( pUnit, UNITTYPE_MISSILE ) )
		pUnit = UnitGetUltimateOwner(pUnit);

	// For now, this is used by players only
	ASSERT_RETFALSE( UnitIsA( pUnit, UNITTYPE_PLAYER ) );

	s_StateClear(pUnit, UnitGetId( pUnit ), STATE_COMMAND_LASER, 0);
	UnitSetStatVector(pUnit, STATS_PET_DESIRED_LOCATION_X, 0, VECTOR(0,0,0));
	UnitSetStat(pUnit, STATS_PET_DESIRED_TARGET, 0, INVALID_ID);

	return TRUE;
#else
	return FALSE;
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
// Sets fuse time on the target pet, either by value or delta
//-----------------------------------------------------------------------------
static BOOL sSkillEventSetTargetPetFuseTime(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pTarget = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);

	int nTicks = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nDelta = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	
	if (nTicks == 0)
		nTicks = UnitGetFuse(pTarget) + nDelta;

	UnitSetFuse(pTarget, nTicks);
	s_StateSet(tData.pUnit, tData.pUnit, STATE_REAPER_ACTIVE, nTicks, tData.nSkill);

	return TRUE;
#else
	return FALSE;
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

/*
//-----------------------------------------------------------------------------
// Specific function used to target a player's Reaper, if any
//-----------------------------------------------------------------------------
static BOOL sSkillEventPetTargetReaperPet(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	// For now, this is used by players only
	ASSERT_RETFALSE( UnitIsA( tData.pUnit, UNITTYPE_PLAYER ) );

	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
	{
		if(tData.pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID)
		{
			continue;
		}

		UNIT * pPet = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(pPet)
		{
			UNIT * pPetNext = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], pPet);
			if ( UnitIsA( pPet, UNITTYPE_REAPER_PET) )
			{
				SkillSetTarget(tData.pUnit, tData.nSkill, tData.idWeapon, UnitGetId(pPet));
				return TRUE;
			}
			pPet = pPetNext;
		}
	}

	return FALSE;
}
*/

//----------------------------------------------------------------------------
struct MOD_ITEM_CALLBACK_DATA
{
	int nSkill;
	int nSkillLevel;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sModAddedCallback(
	UNIT *pMod,
	void *pUserData)
{
	ASSERTX_RETURN( pMod, "Expected mod unit" );
	MOD_ITEM_CALLBACK_DATA *pCallbackData = (MOD_ITEM_CALLBACK_DATA *)pUserData;
	GAME *pGame = UnitGetGame( pMod );
	UNIT *pOwner = UnitGetUltimateOwner( pMod );

	// mark this mod as engineered
	UnitSetStat( pMod, STATS_ENGINEERED, TRUE );

	// we will never save these (for now)
	UnitSetFlag( pMod, UNITFLAG_NOSAVE );

	// can't sell these for nuttin
	UnitSetStat( pMod, STATS_SELL_PRICE_OVERRIDE, 0 );

	// transfer stats from the skill to the unit
	int nSkill = pCallbackData->nSkill;
	int nSkillLevel = pCallbackData->nSkillLevel;
	SkillTransferStatsToUnit( pMod, pOwner, nSkill, nSkillLevel );

	int nFuse = UnitGetStat( pMod, STATS_FUSE_IN_SECONDS );
	int nFuseInTicks = nFuse * 60 * GAME_TICKS_PER_SECOND;
	UnitSetFuse( pMod, nFuseInTicks );

	// this item is now ready to do network communication
	UnitSetCanSendMessages( pMod, TRUE );

	// tell clients about the item
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		if (ClientCanKnowUnit( pClient, pMod ))
		{
			s_SendUnitToClient( pMod, pClient );
		}

	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventModItem(
	const SKILL_EVENT_FUNCTION_DATA & tData)
{
	GAME *pGame = tData.pGame;
	ASSERTX_RETFALSE( pGame && IS_SERVER( pGame ), "Server only" );

	// get target weapon to mod
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	UNIT *pItem = SkillGetTarget( pUnit, nSkill, tData.idWeapon );
	if (pItem)
	{

		// how many mod slots does this weapon have
		if (ItemGetModSlots( pItem ) > 0)
		{

			// destroy any temporary mods that already exist in this weapon
			s_ItemModDestroyAllEngineered( pItem );

			// what look group will the mods be created with (if any)
			int nItemLookGroup = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

			// setup callback data
			MOD_ITEM_CALLBACK_DATA tCallbackData;
			tCallbackData.nSkill = nSkill;
			tCallbackData.nSkillLevel = nSkillLevel;

			// fill all open mod slots in this weapon with temporary mods
			s_ItemModGenerateAll( pItem, pUnit, nItemLookGroup, sModAddedCallback, &tCallbackData );

		}

	}

	return TRUE;

}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventResetWeaponsCooldown(
	const SKILL_EVENT_FUNCTION_DATA & tData)
{
	ASSERT_RETFALSE(tData.pUnit);

	int nSkillForWeapons = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pWeapons[MAX_WEAPONS_PER_UNIT];
	UnitGetWeapons(tData.pUnit, nSkillForWeapons, pWeapons, FALSE);

	for(int i=0; i<MAX_WEAPONS_PER_UNIT; i++)
	{
		if(pWeapons[i])
		{
			SkillClearCooldown(tData.pGame, tData.pUnit, pWeapons[i], ItemGetPrimarySkill(pWeapons[i]));
		}
	}
	return TRUE;
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sActivateQuest(
	UNIT *pPlayer,
	void *pCallbackData)
{
	int nQuest = *(int *)pCallbackData;
	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if (pQuest)
		s_QuestActivate( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventsActivateQuest(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nQuest = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	int nSkill = tData.nSkill;
	BOOL bForAllInLevel = SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	UNIT *pUnit = tData.pUnit;

	//ConsoleString( "Skill Event Activate Quest - Unit: '%s'", UnitGetDevName( pUnit ) );
	UNIT *pTarget = SkillGetTarget( pUnit, nSkill, INVALID_ID );
	if (pTarget)
	{
		// activate for this unit
		if (UnitIsA( pUnit, UNITTYPE_PLAYER ))
		{
			QUEST *pQuest = QuestGetQuest( pUnit, nQuest );
			if (pQuest)
				s_QuestActivate( pQuest );
		}
	}

	// do for all other players in the level
	if (bForAllInLevel == TRUE)
	{
		// iterate players in level
		LEVEL *pLevel = UnitGetLevel( pUnit );
		LevelIteratePlayers( pLevel, sActivateQuest, &nQuest );
	}

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventsEnterPassage(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nSkill = tData.nSkill;
	UNIT *pPassage = tData.pUnit;

	// find passage
	UNIT *pPlayer = SkillGetTarget( pPassage, nSkill, INVALID_ID );

	// enter it
	if (pPlayer && pPassage)
	{
		s_PassageEnter( pPlayer, pPassage );
	}

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventsExitPassage(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nSkill = tData.nSkill;
	UNIT *pPassage = tData.pUnit;

	// find passage
	UNIT *pPlayer = SkillGetTarget( pPassage, nSkill, INVALID_ID );

	// enter it
	if (pPassage && pPlayer)
	{
		s_PassageExit( pPlayer, pPassage );
	}

	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnPet(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nFuseTicks = SkillEventGetParamInt(tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2);
	return sDoSkillEventSpawnPet(tData, 0, VECTOR(0,0,0), nFuseTicks);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sSkillEventSpawnPetAtOwner(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nFuseTicks = SkillEventGetParamInt(tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2);
	VECTOR vPosition = UnitGetPosition(tData.pUnit);
	DWORD dwSpawnMonsterFlags(0);
	SETBIT( dwSpawnMonsterFlags, SSMF_IS_PET_BIT );
	SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
	SETBIT( dwSpawnMonsterFlags, SSMF_FACE_SPAWNER_DIRECTION_BIT );
	return sDoSkillEventSpawnPet(tData, dwSpawnMonsterFlags, vPosition, nFuseTicks);
}




static BOOL sSkillEventSpawnPetFromTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT *pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, INVALID_ID );
	if( pTarget == NULL )
		return FALSE;
	return sDoSkillEventSpawnPet(tData, 0, UnitGetPosition( pTarget ) );
}

static BOOL sSkillEventSpawnPetAtLabelNode(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	const char * pszLabelNodeName = tData.pSkillEvent->tAttachmentDef.pszBone;
	ROOM * pDestinationRoom = NULL;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pLayoutGroup = LevelGetLabelNode(UnitGetLevel(tData.pUnit), pszLabelNodeName, &pDestinationRoom, &pOrientation);
	if(!pLayoutGroup || !pDestinationRoom)
		return FALSE;

	VECTOR vLabelNodePosition;
	MatrixMultiply(&vLabelNodePosition, &pOrientation->vPosition, &pDestinationRoom->tWorldMatrix);
	return sDoSkillEventSpawnPet(tData, 0, vLabelNodePosition);
}

//----------------------------------------------------------------------------
// Summon a line of monsters perpendicular to the owner's facing at the target
// location
//----------------------------------------------------------------------------
static BOOL sSkillEventSpawnWallOfPets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	if( tData.pUnit == NULL )
		return FALSE;
	
	DWORD dwSpawnMonsterFlags( 0 );
	SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
	SETBIT( dwSpawnMonsterFlags, SSMF_FACE_SPAWNER_DIRECTION_BIT );
	SETBIT( dwSpawnMonsterFlags, SSMF_IS_PET_BIT );
	SETBIT( dwSpawnMonsterFlags, SSMF_DONT_ADD_PET_TO_INVENTORY_BIT );

	// Collect our parameters
	int nCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nFuseTicks = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float fDistance = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 ) / 10.0f;
	if (fDistance < 0.5f)
		fDistance = 0.5f;

	VECTOR vFacing = tData.pUnit->vFaceDirection;
	vFacing.fZ = 0.0f;
	VectorNormalize(vFacing);

	// Position is normally the target position, limited to the distance in the skill event.
	VECTOR vPosition = UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
	vPosition.fZ += 2.0f;		// Add some height to make sure they spawn above the ground

	// Find perpendicular direction, for side placement
	VECTOR vDirection;
	VectorCross(vDirection, vFacing, VECTOR(0.0f, 0.0f, 1.0f));
	VectorNormalize(vDirection);

	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	for(int i=0; i<nCount; i++)
	{
		/*UNIT*	pMonster = */ sSkillDoSpawnMonster(
			tData.pGame,
			tData.pUnit,
			tData.nSkill,
			nSkillLevel,
			tData.pSkillEvent->tAttachmentDef.nAttachedDefId,
			INVALID_LINK,
			1,
			tData.pSkillEvent->dwFlags,
			vPosition,
			vFacing,
			tData.idWeapon,
			dwSpawnMonsterFlags,
			STATE_NONE,
			nFuseTicks);

		// Find next location (to left or right depending on odd/even numbers)
		vPosition = vPosition + vDirection * fDistance * (float)(i+1) * ((i%2)? -1.0f : 1.0f);
	}

	return TRUE;
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)



//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static UNIT * c_sSkillEventSpawnMonster(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	int nSkillLevel,
	int nMonsterClass,
	int nMonsterQuality,
	int nCount,
	DWORD dwSkillEventFlags,
	VECTOR & vPos,
	VECTOR & vNorm,
	DWORD dwSpawnMonsterFlags,
	int nFuseTicks = 0)
{
	int nActualSpawnCount = 0;
	MATRIX matUnitWorld;
	UnitGetWorldMatrix(pUnit, matUnitWorld);
	UNIT * pUnitToReturn = NULL;
	const UNIT_DATA * pMonsterData = MonsterGetData( pGame, nMonsterClass );
	ROOM * pSpawnRoom = UnitGetRoom( pUnit );
	ROOM_PATH_NODE * pSpawnPathNode = NULL;
	for(int i=0; i<nCount; i++)
	{
		VECTOR vInitialPosition(0);
		VECTOR vPosition = vPos;
		VECTOR vDirection = vNorm;
		sSkillSpawnGetPositionAndDirection(pGame, pUnit, vInitialPosition, vPosition, vDirection, pSpawnRoom, pSpawnPathNode, nSkill, i, dwSpawnMonsterFlags, dwSkillEventFlags, matUnitWorld);

		MONSTER_SPEC tSpec;
		tSpec.nClass = nMonsterClass;
		tSpec.nExperienceLevel = UnitGetExperienceLevel( pUnit );
		tSpec.nMonsterQuality = nMonsterQuality;
		tSpec.pRoom = pSpawnRoom;
		tSpec.pPathNode = pSpawnPathNode;
		tSpec.vPosition = vPosition;
		tSpec.vDirection = vDirection;
		tSpec.vDirection.fZ = 0.0f;  // only care about X/Y direction
		VectorNormalize( tSpec.vDirection );
		if (TESTBIT( dwSpawnMonsterFlags, SSMF_IS_PET_BIT ))
		{
			tSpec.pOwner = pUnit;
		}

		UNIT * pNewUnit = MonsterInit( pGame, tSpec );
		if (!pNewUnit)
		{
			continue;
		}
		nActualSpawnCount++;
		pUnitToReturn = pNewUnit;

		if(nFuseTicks > 0)
		{
			UnitSetFuse(pNewUnit, nFuseTicks);
		}

		BOOL bHasMode;
		float fDuration = UnitGetModeDuration( pGame, pNewUnit, MODE_SPAWN, bHasMode );
		if (bHasMode)
		{
			if(pMonsterData->fSpawnRandomizePercent != 0.0f)
			{
				fDuration += RandGetFloat(pGame, -fDuration * pMonsterData->fSpawnRandomizePercent, fDuration * pMonsterData->fSpawnRandomizePercent);
			}
			c_UnitSetMode(pNewUnit, MODE_SPAWN, 0, fDuration);
		}

		if ( TESTBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT ) == FALSE &&
			(dwSkillEventFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET))
		{
			VECTOR vSpawnedPosition = UnitGetPosition(pNewUnit);
			VECTOR vRand = RandGetVector(pGame);
			vRand.fZ = fabs(vRand.fZ);
			VECTOR vNormal(0);
			vNormal = vInitialPosition - vSpawnedPosition;
			vNormal.fZ = vNorm.fZ;
			VectorNormalize(vNormal);

			UnitChangePosition(pNewUnit, vPosition);

			if(!(dwSkillEventFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET_ABSOLUTE))
			{
				if ( ! UnitDataTestFlag(pMonsterData, UNIT_DATA_FLAG_SPAWN_IN_AIR) )
					UnitStartJump(pGame, pNewUnit, vNormal.fZ + vRand.fZ, TRUE, TRUE);
				UnitSetMoveDirection(pNewUnit, vNormal);
				UnitSetVelocity(pNewUnit, VectorLength(vNorm));
				UnitStepListAdd(pGame, pNewUnit);
			}

			UnitSetFaceDirection( pNewUnit, vNormal, TRUE, TRUE );
		}

		if(TESTBIT(dwSpawnMonsterFlags, SSMF_GIVE_TARGET_INVENTORY_BIT))
		{
			UNIT * pSkillTarget = SkillGetTarget(pUnit, nSkill, INVALID_ID, 0);
			if(UnitHasInventory(pSkillTarget))
			{
				int nInventoryCount = UnitInventoryGetCount(pSkillTarget);
				for(int i=0; i<nInventoryCount; i++)
				{
					UNIT * pInventoryItem = UnitInventoryGetByIndex(pSkillTarget, 0);
					int nOriginalLocation, nOriginalX, nOriginalY;
					UnitGetInventoryLocation(pInventoryItem, &nOriginalLocation, &nOriginalX, &nOriginalY);

					InventoryChangeLocation(
						pNewUnit,
						pInventoryItem,
						nOriginalLocation,
						nOriginalX,
						nOriginalY,
						0);
				}
			}

			int nLocation, nX, nY;
			if(ItemGetOpenLocation(pNewUnit, pSkillTarget, FALSE, &nLocation, &nX, &nY))
			{
				InventoryChangeLocation(pNewUnit, pSkillTarget, nLocation, nX, nY, 0);
			}
		}
	}
	return pUnitToReturn;
}

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSpawnMonsterAtLabelNode(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	const char * pszLabelNodeName = tData.pSkillEvent->tAttachmentDef.pszBone;
	ROOM * pDestinationRoom = NULL;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pLayoutGroup = LevelGetLabelNode(UnitGetLevel(tData.pUnit), pszLabelNodeName, &pDestinationRoom, &pOrientation);
	if(!pLayoutGroup || !pDestinationRoom)
		return FALSE;

	VECTOR vLabelNodePosition;
	MatrixMultiply(&vLabelNodePosition, &pOrientation->vPosition, &pDestinationRoom->tWorldMatrix);

	DWORD dwSpawnMonsterFlags = 0;
	VECTOR vPosition = vLabelNodePosition;
	SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
	VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
	int nSkillLevel = SkillGetLevel( tData.pUnit, tData.nSkill );

	UNIT * pMonster = c_sSkillEventSpawnMonster(
		tData.pGame,
		tData.pUnit,
		tData.nSkill,
		nSkillLevel,
		tData.pSkillEvent->tAttachmentDef.nAttachedDefId,
		INVALID_LINK,
		1,
		tData.pSkillEvent->dwFlags,
		vPosition,
		vNormal,
		dwSpawnMonsterFlags,
		0 );

	return pMonster != NULL;
}
#endif

//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnPetWithInventory(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	DWORD dwFlags = 0;
	SETBIT(dwFlags, SSMF_GIVE_TARGET_INVENTORY_BIT);
	return sDoSkillEventSpawnPet(tData, dwFlags);
}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAddReplica(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	ConsoleString( "*Add Replica*" );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventCameraShake(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	VECTOR vDirection = tData.pSkillEvent->tAttachmentDef.vNormal;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_RANDOM_FIRING_DIRECTION)
	{
		vDirection = RandGetVector(tData.pGame);
	}
	VectorNormalize(vDirection);
	float fDuration = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float fMagnitude = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float fDegrade = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );

	c_CameraShakeRandom(tData.pGame, vDirection, fDuration, fMagnitude, fDegrade);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventCameraSetRTS_TargetPosition(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	VECTOR vTarget = UnitGetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0 );
	c_CameraSetRTS_TargetPosition( vTarget );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventCreateObject(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	GAME *pGame = UnitGetGame( pUnit );
	int nObjectClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	BOOL bFaceAnchor = SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float flDistanceFromAnchor = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	BOOL bAnchorAtNearestPlayer = SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	BOOL bUniqueInLevel = SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );

	// must have object
	ASSERTX_RETFALSE( nObjectClass != INVALID_LINK, "Expected object class" );

	// don't bother if it must be unique in level and we find one
	if (bUniqueInLevel)
	{
		LEVEL *pLevel = UnitGetLevel( pUnit );
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		if (LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, nObjectClass ))
		{
			return FALSE;
		}
	}

	// find the nearest player
	UNIT *pPlayer = PlayerFindNearest( pUnit );

	// we will either use the nearest player as our anchor or ourselves as the anchor
	UNIT *pAnchor = pUnit;
	if (bAnchorAtNearestPlayer == TRUE && pPlayer)
	{
		pAnchor = pPlayer;
	}
	ASSERTX_RETFALSE( pAnchor, "Expected unit" );

	// pick a direction in front of our anchor
	VECTOR vFromAnchor = UnitGetFaceDirection( pAnchor, FALSE );
	VectorNormalize( vFromAnchor );

	// get position
	VECTOR vPosition = UnitGetPosition( pAnchor ) + (vFromAnchor * flDistanceFromAnchor);

	// which direction will the object face
	VECTOR vDirection;
	if (bFaceAnchor)
	{
		vDirection = UnitGetPosition( pAnchor ) - vPosition;
		VectorNormalize( vDirection );
	}
	else
	{
		vDirection = RandGetVectorXY( pGame );
		VectorNormalize( vDirection );
	}

	// find the nearest valid location to this spot
	ROOM *pRoom = NULL;
	vPosition = RoomGetNearestFreePosition( UnitGetRoom( pAnchor ), vPosition, &pRoom );

	// create the object
	OBJECT_SPEC tSpec;
	tSpec.nClass = nObjectClass;
	tSpec.pRoom = pRoom;
	tSpec.vPosition = vPosition;
	tSpec.pvFaceDirection = &vDirection;
	UNIT *pObject = s_ObjectSpawn( pGame, tSpec );
	ASSERTX_RETFALSE( pObject, "Skill event create object - unable to create object" );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHellgateSpawnFilter(
	GAME * pGame,
	int nMonsterClass,
	const UNIT_DATA * pUnitData )
{
	return ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_IS_GOOD);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveInvulnerability(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData)
{
	s_StateClear( pUnit, UnitGetId( pUnit ), STATE_INVULNERABLE_ALL, 0 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOnDieRemoveInvulnerabilityOther(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherunit,
	EVENT_DATA* pEventData,
	void* pData,
	DWORD dwId )
{
	UNIT *pOther = UnitGetById(pGame, (UNITID)pEventData->m_Data1);
	if (pOther)
	{
		s_StateClear( pOther, UnitGetId( pOther ), STATE_INVULNERABLE_ALL, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOnDieIncrementSummonedDeadCountOther(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pOtherunit,
	EVENT_DATA* pEventData,
	void* pData,
	DWORD dwId )
{
	UNIT *pOther = UnitGetById(pGame, (UNITID)pEventData->m_Data1);
	if (pOther)
	{
		int nCount = UnitGetStat( pOther, STATS_SUMMONED_DIED_COUNT );
		UnitSetStat( pOther, STATS_SUMMONED_DIED_COUNT, nCount + 1 );

		// do another ai soon so we can summon more stuff
		AI_Register( pOther, FALSE, 1 );

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetHellgateSpawnClass(
	GAME * pGame,
	UNIT * pHellgate )
{
	// get level data
	LEVEL* pLevel = UnitGetLevel( pHellgate );
	const DRLG_DEFINITION * drlg_definition = LevelGetDRLGDefinition( pLevel );
	ASSERT_RETVAL( drlg_definition, INVALID_ID );

	// get monster data
	const UNIT_DATA *pUnitData = UnitGetData( pHellgate );

	// first, look if there is a spawn class entry for the level with the name
	// of this montser appended to the end of it
	char szSpawnClassName[ DEFAULT_INDEX_SIZE ];
	PStrPrintf( szSpawnClassName, DEFAULT_INDEX_SIZE, "%s_%s", drlg_definition->pszName, pUnitData->szName );
	int nSpawnClass = ExcelGetLineByStringIndex( pGame, DATATABLE_SPAWN_CLASS, szSpawnClassName );

	// if no spawn class found, just use the district spawn class
	if (nSpawnClass == INVALID_LINK)
	{
		ROOM* pRoom = UnitGetRoom( pHellgate );
		DISTRICT* pDistrict = RoomGetDistrict( pRoom );
		nSpawnClass = DistrictGetSpawnClass( pDistrict );
	}

	// if no spawn class found, just use the level spawn class
	if (nSpawnClass == INVALID_LINK)
	{
		nSpawnClass = DungeonGetLevelSpawnClass(pGame, pLevel);
	}

	return nSpawnClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventHellgateSpawn(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nMinSpawnThisCycle = (int)tData.pSkillData->fParam1;
	int nSpawnMax = (int)tData.pSkillData->fParam2;

	int nSpawnedHaveDied = UnitGetStat( tData.pUnit, STATS_SUMMONED_DIED_COUNT );
	int nTotalSpawnedCount = UnitGetStat( tData.pUnit, STATS_SPAWN_COUNT );
	int nRemainingAlive = nTotalSpawnedCount - nSpawnedHaveDied;

	BOOL bCanSpawn = FALSE;
	if (nTotalSpawnedCount <= nSpawnMax &&
		nRemainingAlive <= 1)
	{
		bCanSpawn = TRUE;
	}

	if (bCanSpawn)
	{
		ROOM *pRoom = UnitGetRoom( tData.pUnit );
		if ( !pRoom )
		{
			return FALSE;
		}

		LEVEL* pLevel = RoomGetLevel(pRoom);
		if ( !pLevel )
		{
			return FALSE;
		}

		if (!LevelIsActive(pLevel))
		{
			return FALSE;
		}

		// what spawn  class will we evaluate
		int nSpawnClass = sGetHellgateSpawnClass( tData.pGame, tData.pUnit );

		// what will we spawn
		int nMonsterExperienceLevel = RoomGetMonsterExperienceLevel(pRoom);
		SPAWN_ENTRY tSpawns[MAX_SPAWNS_AT_ONCE];
		int nNumSpawnEntries = SpawnClassEvaluate(
			tData.pGame,
			pLevel->m_idLevel,
			nSpawnClass,
			nMonsterExperienceLevel,
			tSpawns,
			sHellgateSpawnFilter );

		if (nNumSpawnEntries == 0)
		{
			return FALSE;
		}

		// set spawning state
		int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
		ASSERT_RETFALSE( nState != INVALID_ID );
		const STATE_DATA * pStateData = (const STATE_DATA *) ExcelGetData( tData.pGame, DATATABLE_STATES, nState );
		if(pStateData)
		{
			s_StateSet( tData.pUnit, tData.pUnit, nState, pStateData->nDefaultDuration );
		}

		// if spawning final wave
		if (nTotalSpawnedCount == nSpawnMax)
		{
			nSpawnMax *= 8;  // allow spawning over our max
			nMinSpawnThisCycle = (int)(nMinSpawnThisCycle * 1.8);  // we want more on the last wave
		}

		// get the free places in this room that we can spawn at
		const int MAX_PATH_NODES = 10;
		FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

		NEAREST_NODE_SPEC tSpec;
		SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY);
		tSpec.vFaceDirection = UnitGetFaceDirection(tData.pUnit, FALSE);
		tSpec.pUnit = tData.pUnit;
		tSpec.fMaxDistance = 2.0f;

		int nNumNodes = RoomGetNearestPathNodes(tData.pGame, UnitGetRoom(tData.pUnit), UnitGetPosition(tData.pUnit), MAX_PATH_NODES, tFreePathNodes, &tSpec);
		if (nNumNodes <= 0)
		{
			return FALSE;
		}

		// setup spawn flags
		DWORD dwSpawnMonsterFlags = 0;
		SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );

		// do the spawn entries
		VECTOR vPositionCenter = tData.pSkillEvent->tAttachmentDef.vPosition;
		VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
		int nSpawnedThisCycle = 0;
		int nTries = 32;

		while (nSpawnedThisCycle < nMinSpawnThisCycle && nTries-- > 0)
		{
			for ( int i = 0; i < nNumSpawnEntries; i++ )
			{
				SPAWN_ENTRY *pSpawn = &tSpawns[ i ];
				ASSERT( pSpawn->eEntryType == SPAWN_ENTRY_TYPE_MONSTER );
				int nMonsterClass = pSpawn->nIndex;

				// how many at this spawn
				int nNumToSpawn = SpawnEntryEvaluateCount( tData.pGame, pSpawn );

				// limit to the max we're supposed to spawn at all
				if (nTotalSpawnedCount + nNumToSpawn >= nSpawnMax)
				{
					nNumToSpawn = nSpawnMax - nTotalSpawnedCount;
				}

				// spawn each monster of this spawn class entry
				for ( int j = 0; j < nNumToSpawn; j++ )
				{

					// pick a node to spawn at
					int nPick = RandGetNum( tData.pGame, 0, nNumNodes - 1 );
					ROOM_PATH_NODE *pNodeSpawn = tFreePathNodes[ nPick ].pNode;
					ROOM *pRoomWithPathNode = tFreePathNodes[ nPick ].pRoom;

					// transform node position to world space to use
					VECTOR vSpawnPosition = RoomPathNodeGetExactWorldPosition(tData.pGame, pRoomWithPathNode, pNodeSpawn);

					int nMonsterQuality = INVALID_LINK;

					// do the spawn
					UNIT * pSpawnedUnit = sSkillDoSpawnMonster(
						tData.pGame,
						tData.pUnit,
						tData.nSkill,
						1,
						nMonsterClass,
						nMonsterQuality,
						1,
						tData.pSkillEvent->dwFlags,
						vSpawnPosition,
						vNormal,
						tData.idWeapon,
						dwSpawnMonsterFlags);

					if ( pSpawnedUnit )
					{
						UnitSetStat( pSpawnedUnit, STATS_SPAWN_SOURCE, UnitGetId( tData.pUnit ) );
						s_StateAddTarget( tData.pGame, UnitGetId( tData.pUnit ), tData.pUnit, pSpawnedUnit, nState, 0 );
						UnitRegisterEventHandler(tData.pGame, pSpawnedUnit, EVENT_UNITDIE_BEGIN, sOnDieIncrementSummonedDeadCountOther, EVENT_DATA(UnitGetId(tData.pUnit)));
						nSpawnedThisCycle++;
						nTotalSpawnedCount++;
					}

				}

			}

		}

		//  save the new spawn count
		UnitSetStat( tData.pUnit, STATS_SPAWN_COUNT, nTotalSpawnedCount );

	}

	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sDoSpawnMonsterFinite(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
	int nMonsterClass = (int)tEventData.m_Data1;
	int nMax = (int)tEventData.m_Data2;
	BOOL bDestroyWhenEmpty = (BOOL)tEventData.m_Data3;
	DWORD dwRepeatInTicks = (DWORD)tEventData.m_Data4;
	int nSkill = (int)tEventData.m_Data5;

	// have we reached the max
	int nTotalSpawnedCount = UnitGetStat( pUnit, STATS_SPAWN_COUNT );
	if (nTotalSpawnedCount >= nMax)
	{

		// some spawner self destruct when empty
		if (bDestroyWhenEmpty)
		{
			UnitDie( pUnit, pUnit );
		}

		return FALSE;

	}

	// spawn more if we can
	BOOL bSpawned = FALSE;
	if (nTotalSpawnedCount < nMax)
	{
		VECTOR vPosition = UnitGetPosition( pUnit );
		VECTOR vNormal = UnitGetUpDirection( pUnit );

		// setup spawning flags
		DWORD dwSpawnMonsterFlags = 0;
		SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );
		SETBIT( dwSpawnMonsterFlags, SSMF_FACE_SPAWNER_DIRECTION_BIT );

		// spawn it
		UNIT *pMonster = sSkillDoSpawnMonster(
			pGame,
			pUnit,
			nSkill,
			1,
			nMonsterClass,
			INVALID_LINK,
			1,
			0,
			vPosition,
			vNormal,
			INVALID_ID,
			dwSpawnMonsterFlags);

		if (pMonster)
		{

			// increment spawn count
			nTotalSpawnedCount++;
			UnitSetStat( pUnit, STATS_SPAWN_COUNT, nTotalSpawnedCount );
			bSpawned = TRUE;

		}

	}

	// register repeat event if we're supposed to
	if (dwRepeatInTicks > 0)
	{
		UnitRegisterEventTimed( pUnit, sDoSpawnMonsterFinite, &tEventData, dwRepeatInTicks );
	}

	return bSpawned;

}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnMonsterFinite(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	int nMonsterClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	int nSkill = tData.nSkill;

	// get params
	int nMax = SkillEventGetParamInt( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	BOOL bDestroyWhenEmpty = SkillEventGetParamBool( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float flRepeatInSeconds = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	DWORD dwRepeatInTicks = (int)(flRepeatInSeconds * GAME_TICKS_PER_SECOND_FLOAT);
	BOOL bMonsterClassFromUnitData = SkillEventGetParamBool( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );

	// if we're to use the monster spawn from unit data, look it up in the unit data
	// and use it instead of what they put in the dropdown
	if (bMonsterClassFromUnitData == TRUE)
	{
		if (pUnitData->nSpawnMonsterClass != INVALID_LINK)
		{
			nMonsterClass = pUnitData->nSpawnMonsterClass;
		}
	}

	// forget it if we don't have a monster to spawn
	if (nMonsterClass == INVALID_LINK)
	{
		UnitDie( pUnit, pUnit );
		return FALSE;
	}

	// setup event data for repeat event
	EVENT_DATA tEventData;
	tEventData.m_Data1 = nMonsterClass;
	tEventData.m_Data2 = nMax;
	tEventData.m_Data3 = bDestroyWhenEmpty;
	tEventData.m_Data4 = dwRepeatInTicks;
	tEventData.m_Data5 = nSkill;

	// if repeating, don't do if we already are repeating
	if (dwRepeatInTicks > 0)
	{
		if (UnitHasTimedEvent( pUnit, sDoSpawnMonsterFinite, CheckEventParam1, &tEventData ))
		{
			return FALSE;
		}
	}

	// do the monster spawn
	return sDoSpawnMonsterFinite( pGame, pUnit, tEventData );

}
#endif #//if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sDoSpawnClassFinite(
	GAME *pGame,
	UNIT *pUnit,
	const EVENT_DATA &tEventData)
{
	int nSpawnClass = (int)tEventData.m_Data1;
	int nMax = (int)tEventData.m_Data2;
	BOOL bDestroyWhenEmpty = (BOOL)tEventData.m_Data3;
	DWORD dwRepeatInTicks = (DWORD)tEventData.m_Data4;
	LEVEL *pLevel = UnitGetLevel( pUnit );

	// have we reached the max
	int nTotalSpawnedCount = UnitGetStat( pUnit, STATS_SPAWN_COUNT );
	if (nTotalSpawnedCount >= nMax)
	{

		// some spawner self destruct when empty
		if (bDestroyWhenEmpty)
		{
			UnitDie( pUnit, pUnit );
		}

		return FALSE;

	}

	// evaluate what we can spawn
	SPAWN_ENTRY tSpawns[ MAX_SPAWNS_AT_ONCE ];
	int nNumSpawns = SpawnClassEvaluate(
		pGame,
		LevelGetID( pLevel ),
		nSpawnClass,
		LevelGetMonsterExperienceLevel( pLevel ),
		tSpawns);
	if (nNumSpawns == 0)
	{
		return TRUE;
	}

	// get available locations to spawn at
	const int MAX_PATH_NODE = 32;
	FREE_PATH_NODE tFreePathNode[ MAX_PATH_NODE ];

	NEAREST_NODE_SPEC tSpec;
	tSpec.fMinDistance = 0.0f;
	tSpec.fMaxDistance = 15.0f;
	int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pUnit), UnitGetPosition(pUnit), MAX_PATH_NODE, tFreePathNode, &tSpec);

	if (nNumNodes == 0)
	{
		return TRUE;  // no locations found
	}

	// spawn more if we can
	BOOL bSpawned = FALSE;
	if (nTotalSpawnedCount < nMax)
	{
		VECTOR vPosition = UnitGetPosition( pUnit );
		VECTOR vNormal = UnitGetUpDirection( pUnit );

		// pick what to spawn
		int nSpawnPick = RandGetNum( pGame, 0, nNumSpawns - 1 );
		SPAWN_ENTRY *pSpawnEntry = &tSpawns[ nSpawnPick ];

		// pick where to spawn
		int nLocPick = RandGetNum( pGame, 0, nNumNodes -1 );
		FREE_PATH_NODE *pFreePathNode = &tFreePathNode[ nLocPick ];
		SPAWN_LOCATION tSpawnLocation;
		SpawnLocationInit( &tSpawnLocation, pFreePathNode->pRoom, pFreePathNode->pNode->nIndex, NULL );

		// spawn monster
		nTotalSpawnedCount += s_RoomSpawnMonsterByMonsterClass(
			pGame,
			pSpawnEntry,
			INVALID_LINK,
			UnitGetRoom( pUnit ),
			&tSpawnLocation,
			NULL,
			NULL);

		if (nTotalSpawnedCount)
		{
		}

		UnitSetStat( pUnit, STATS_SPAWN_COUNT, nTotalSpawnedCount );
		bSpawned = TRUE;

	}

	// register repeat event if we're supposed to
	if (dwRepeatInTicks > 0)
	{
		UnitRegisterEventTimed( pUnit, sDoSpawnClassFinite, &tEventData, dwRepeatInTicks );
	}

	return bSpawned;

}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSpawnClassFinite(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSpawnClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	int nSkill = tData.nSkill;

	// get params
	int nMax = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	BOOL bDestroyWhenEmpty = SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float flRepeatInSeconds = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	DWORD dwRepeatInTicks = (int)(flRepeatInSeconds * GAME_TICKS_PER_SECOND_FLOAT);

	// setup event data for repeat event
	EVENT_DATA tEventData;
	tEventData.m_Data1 = nSpawnClass;
	tEventData.m_Data2 = nMax;
	tEventData.m_Data3 = bDestroyWhenEmpty;
	tEventData.m_Data4 = dwRepeatInTicks;
	tEventData.m_Data5 = nSkill;

	// if repeating, don't do if we already are repeating
	if (dwRepeatInTicks > 0)
	{
		if (UnitHasTimedEvent( pUnit, sDoSpawnClassFinite, CheckEventParam1, &tEventData ))
		{
			return FALSE;
		}
	}

	// do the monster spawn
	return sDoSpawnClassFinite( pGame, pUnit, tEventData );

}
#endif #//if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventGiveLiveToCompanion(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;

	// how much life can we take
	float flPercentCostInCurrentHealth = SkillEventGetParamFloat( tData.pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float flHealthPercentToRestore = SkillEventGetParamFloat( tData.pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float flHealthPercentToRestorePerLevel = SkillEventGetParamFloat( tData.pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	BOOL bNoParticles = SkillEventGetParamBool( tData.pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );

	// adjust ratio for skill level
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	flHealthPercentToRestore += (flHealthPercentToRestorePerLevel * (float)nSkillLevel);

	// if we don't have this much health, don't do anything
	int nCurrentHealth = UnitGetStat( pUnit, STATS_HP_CUR );
	int nHealthCost = (int)(nCurrentHealth * flPercentCostInCurrentHealth);
	if (nCurrentHealth - nHealthCost > 0)
	{
		BOOL bDidHealing = FALSE;
		for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
		{
			if(tData.pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID)
			{
				continue;
			}

			UNIT * pPet = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], NULL);
			while(pPet)
			{
				UNIT * pPetNext = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], pPet);

				GAME * pGame = UnitGetGame( pUnit );
				if ( SkillIsValidTarget( pGame, tData.pUnit, pPet, NULL, tData.pSkillData->nId, FALSE ) )
				{
					// should we do healing
					int nHealthCurrent = UnitGetStat( pPet, STATS_HP_CUR );
					int nHealthMax = UnitGetStat( pPet, STATS_HP_MAX );
					if (nHealthCurrent < nHealthMax)
					{

						// how much to heal
						int nAmountToHeal = (int)(flHealthPercentToRestore * nHealthMax);

						// do healing
						UnitChangeStat( pPet, STATS_HP_CUR, nAmountToHeal );

						if (!bNoParticles)
						{
							// set state on us
							s_StateSet( pPet, pPet, STATE_GIVE_LIFE_RECEIVE, 0 );
						}

						// we did healing
						bDidHealing = TRUE;

					}
				}

				pPet = pPetNext;
			}
		}

		// if we did healing, do costs
		if (bDidHealing)
		{
			// take the health away from us
			UnitChangeStat( pUnit, STATS_HP_CUR, -nHealthCost );

			if (!bNoParticles)
			{
				// set state on us for transferring health
				s_StateSet( pUnit, pUnit, STATE_GIVE_LIFE_GIVE, 0 );
			}
		}
	}

	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventTradeLifeForPower(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;

	// how much life can we take
	int nHealthMax = UnitGetStat( pUnit, STATS_HP_MAX );
	float flPercentOfMaxHealth = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nHealthCost = (int)(nHealthMax * flPercentOfMaxHealth);
	float flRatio = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	float flRatioBonusPerLevel = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );

	// adjust ratio for skill level
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	flRatio += (flRatioBonusPerLevel * (float)nSkillLevel);

	// if we don't have this much health, don't do anything
	int nHealthCurrent = UnitGetStat( pUnit, STATS_HP_CUR );
	if (nHealthCurrent > nHealthCost)
	{
		// only do this if we don't have full power
		int nPowerCurrent = UnitGetStat( pUnit, STATS_POWER_CUR );
		int nPowerMax = UnitGetStat( pUnit, STATS_POWER_MAX );
		if (nPowerCurrent < nPowerMax)
		{
			// take the health away from us
			UnitChangeStat( pUnit, STATS_HP_CUR, -nHealthCost );

			// how much power will we restore
			float flPowerPercentToRestore = flPercentOfMaxHealth * flRatio;
			int nAmountToRecharge = (int)(flPowerPercentToRestore * nPowerMax);

			// do recharge
			UnitChangeStat( pUnit, STATS_POWER_CUR, nAmountToRecharge );

			// set state on us
			s_StateSet( pUnit, pUnit, STATE_TRADE_LIFE_FOR_POWER, 0 );
		}
	}

	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventClearAllPathingInfo(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	if(UnitTestFlag(tData.pUnit, UNITFLAG_PATHING) && tData.pUnit->m_pPathing)
	{
		UnitClearBothPathNodes(tData.pUnit);
		UnitSetFlag(tData.pUnit, UNITFLAG_BEING_REPLACED);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventTakeoff(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	if(UnitTestFlag(tData.pUnit, UNITFLAG_PATHING) && tData.pUnit->m_pPathing)
	{
		UnitClearBothPathNodes(tData.pUnit);
	}
	UnitSetStat(tData.pUnit, STATS_CAN_FLY, 1);


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventLand(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UnitPathingInit(tData.pUnit);
	UnitSetStat(tData.pUnit, STATS_CAN_FLY, 0);

	UnitSetOnGround( tData.pUnit, FALSE );
	UnitSetZVelocity( tData.pUnit, -0.25f );
	UnitStepListAdd( tData.pGame, tData.pUnit );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sDoFlybyWarpAway(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ROOMID idRoomDest = (ROOMID)tEventData.m_Data1;
	ROOMID nPathNodeIndex = (ROOMID)tEventData.m_Data2;
	ROOM *pRoomDest = RoomGetByID( pGame, idRoomDest );

	// if they got us during the attack then oh well, die right here
	if (IsUnitDeadOrDying( pUnit ) == TRUE)
	{
		return TRUE;
	}

	// get position to warp to
	VECTOR vWarpDestination = RoomPathNodeGetWorldPosition(
		pGame,
		pRoomDest,
		RoomGetRoomPathNode( pRoomDest, nPathNodeIndex ));

	// get vector to destination
	const VECTOR &vPosCurrent = UnitGetPosition( pUnit );
	VECTOR vToDestination = vWarpDestination- vPosCurrent;

	// warp away
	UnitWarp(
		pUnit,
		pRoomDest,
		vWarpDestination,
		vToDestination,
		UnitGetUpDirection( pUnit ),
		0,
		TRUE);

	return TRUE;

}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventWarpFlybyAttack(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;
	float flMinDistFromTarget = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float flMaxDistFromTarget = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	ESTATE eHitState = (ESTATE)ExcelGetLineByStringIndex( pGame, DATATABLE_STATES, tData.pSkillEvent->tAttachmentDef.pszAttached );

	UNIT *pTarget = SkillGetTarget( pUnit, nSkill, INVALID_ID );
	if (pTarget)
	{
		// forget it if we're dead
		if (IsUnitDeadOrDying( pUnit ) == TRUE)
		{
			return TRUE;
		}

		// Prevent the thing from following you across levels
		if(UnitGetLevel(pTarget) != UnitGetLevel(pUnit))
		{
			return TRUE;
		}

		const VECTOR &vAttackerPos = UnitGetPosition( pUnit );
		const VECTOR &vTargetPos = UnitGetPosition( pTarget );

		// get vector to target
		VECTOR vToTarget = vTargetPos - vAttackerPos;

		NEAREST_NODE_SPEC tSpec;
		SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY );
		SETBIT( tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY );
		SETBIT( tSpec.dwFlags, NPN_CHECK_LOS );
		SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
		tSpec.vFaceDirection = vToTarget;
		tSpec.fMinDistance = flMinDistFromTarget;
		tSpec.fMaxDistance = flMaxDistFromTarget;

		const int MAX_PATH_NODE = 32;
		FREE_PATH_NODE tFreePathNode[ MAX_PATH_NODE ];

		int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pTarget), vTargetPos, MAX_PATH_NODE, tFreePathNode, &tSpec);
		if (nNumNodes > 0)
		{
			// select a node at random
			int nPick = RandGetNum( pGame, nNumNodes - 1 );
			const FREE_PATH_NODE *pFreePathNode = &tFreePathNode[ nPick ];

			// warp to target
			UnitWarp(
				pUnit,
				UnitGetRoom( pTarget ),
				UnitGetPosition( pTarget ),
				vToTarget,
				UnitGetUpDirection( pUnit ),
				0,
				TRUE);

			// attack the target
			s_UnitAttackUnit( pUnit, pTarget, NULL, 0, NULL );

			// set state on the target unit for being attacked by this unit
			DWORD dwHitStateDurationInTicks = GAME_TICKS_PER_SECOND * 1;
			s_StateSet( pTarget, pUnit, eHitState, dwHitStateDurationInTicks, nSkill );

			// set a timer to do the actual attack after the mode is done
			DWORD dwDelayInTicks = 1; //GAME_TICKS_PER_SECOND / 4;
			EVENT_DATA tEventData;
			tEventData.m_Data1 = RoomGetId( pFreePathNode->pRoom );
			tEventData.m_Data2 = pFreePathNode->pNode->nIndex;
			UnitRegisterEventTimed(pUnit, s_sDoFlybyWarpAway, &tEventData, dwDelayInTicks );
		}
	}

	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSetStateAfterClearFor(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;

	// how long is required between states
	float flClearOfStateInSeconds = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	DWORD dwClearOfStateInTicks = (DWORD)(GAME_TICKS_PER_SECOND_FLOAT * flClearOfStateInSeconds);

	// what state are we checking
	ESTATE eState = (ESTATE)ExcelGetLineByStringIndex( pGame, DATATABLE_STATES, tData.pSkillEvent->tAttachmentDef.pszAttached );

	// the state has to be clear now
	if (UnitHasExactState( pUnit, eState ) == FALSE)
	{
		DWORD dwStateClearedOnTick = UnitGetStat( pUnit, STATS_STATE_CLEAR_TICK, eState );
		DWORD dwNow = GameGetTick( pGame );

		// if the state has never been set, or enough has time has passed since the state
		// was cleared, set it again
		if (dwStateClearedOnTick == 0 || dwNow - dwStateClearedOnTick >= dwClearOfStateInTicks)
		{
			s_StateSet( pUnit, pUnit, eState, 0, nSkill, NULL, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );
		}
	}

	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventHordeSpawner(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;
	int nMonsterClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	// get params
	int nCount = SkillEventGetParamInt( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float flMinDistanceAway = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	// get spawning params
	VECTOR vNormal = tData.pSkillEvent->tAttachmentDef.vNormal;
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	VECTOR vSpawnerDirection = UnitGetFaceDirection( pUnit, FALSE );

	// find a spot to make the spawner at
	const int MAX_PATH_NODES = 64;
	FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

	NEAREST_NODE_SPEC tSpec;
	SETBIT(tSpec.dwFlags, NPN_IN_FRONT_ONLY);
	SETBIT(tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY);
	SETBIT(tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY);
	tSpec.pUnit = pUnit;
	tSpec.vFaceDirection = vSpawnerDirection;
	tSpec.fMinDistance = flMinDistanceAway;
	tSpec.fMaxDistance = flMinDistanceAway + 5.0f;

	int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pUnit), UnitGetPosition(pUnit), MAX_PATH_NODES, tFreePathNodes, &tSpec);
	if (nNumNodes > 0)
	{
		// pick a random node and get world position
		int nPick = RandGetNum( pGame, nNumNodes );
		const FREE_PATH_NODE *pFreePathNode = &tFreePathNodes[ nPick ];
		VECTOR vLocation = RoomPathNodeGetExactWorldPosition( pGame, pFreePathNode->pRoom, pFreePathNode->pNode );

		// setup spawn flags
		DWORD dwSpawnMonsterFlags = 0;
		SETBIT( dwSpawnMonsterFlags, SSMF_USE_POSITION_AS_IS_BIT );

		// make it so!
		UNIT *pSpawner = sSkillDoSpawnMonster(
			pGame,
			pUnit,
			nSkill,
			nSkillLevel,
			nMonsterClass,
			INVALID_LINK,
			nCount,
			tData.pSkillEvent->dwFlags,
			vLocation,
			vNormal,
			INVALID_ID,
			dwSpawnMonsterFlags);

		// wake it up right now
		if (pSpawner)
		{
			UnitSetStat( pSpawner, STATS_ALERT, TRUE );
			AI_Register( pSpawner, FALSE, 1 );
		}
	}

	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventTeleportToTarget(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;
	UNITID idWeapon = tData.idWeapon;

	// get options
	float flMinDistanceFromTarget = SkillEventGetParamFloat( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	BOOL bFrontOnly = SkillEventGetParamBool( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );

	// get target
	UNIT *pTarget = SkillGetTarget( pUnit, nSkill, idWeapon );
	UNIT * pWeapon = UnitGetById( pGame, idWeapon );
	BOOL bIsInRange = FALSE;
	if (pTarget && SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, nSkill, FALSE, &bIsInRange, TRUE ) && bIsInRange )
	{
		VECTOR vTargetDirection = UnitGetFaceDirection( pTarget, FALSE );

		// find a spot to make the spawner at
		const int MAX_PATH_NODES = 16;
		FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

		NEAREST_NODE_SPEC tSpec;
		SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY, bFrontOnly );
		SETBIT( tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY );
		SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );

		tSpec.fMinDistance = flMinDistanceFromTarget;


		int nNumNodes(0);
		if( AppIsHellgate() )
		{
			tSpec.vFaceDirection = vTargetDirection;
			tSpec.fMaxDistance = flMinDistanceFromTarget + 5.0f;
			nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pUnit), UnitGetPosition(pUnit), MAX_PATH_NODES, tFreePathNodes, &tSpec);
		}
		else //tugboat
		{
			//lets get the direction between the target and unit. That will be the facing direction
			//vTargetDirection = UnitGetPosition( pTarget ) - UnitGetPosition( pUnit );
			//VectorNormalize( vTargetDirection, vTargetDirection );
			tSpec.vFaceDirection = vTargetDirection;
			tSpec.fMaxDistance = flMinDistanceFromTarget + 2.0f;
			nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pTarget), UnitGetPosition(pTarget), MAX_PATH_NODES, tFreePathNodes, &tSpec);

		}
		if (nNumNodes > 0)
		{
			// pick a random node and get world position
			int nPick = RandGetNum( pGame, nNumNodes );
			const FREE_PATH_NODE *pFreePathNode = &tFreePathNodes[ nPick ];
			VECTOR vLocation = RoomPathNodeGetExactWorldPosition( pGame, pFreePathNode->pRoom, pFreePathNode->pNode );

			// we're going to face toward the target
			VECTOR vToTarget = UnitGetPosition( pTarget ) - vLocation;

			// go there
			UnitWarp(
				pUnit,
				pFreePathNode->pRoom,
				vLocation,
				vToTarget,
				UnitGetVUpDirection( pUnit ),
				0);

			// set the stat specified that we did this action
			STATS_TYPE eStat = (STATS_TYPE)tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
			if (eStat != INVALID_LINK)
			{
				UnitSetStat( pUnit, eStat, TRUE );
			}

			return TRUE;  // skill successfull
		}
	}

	// no target found
	return FALSE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventPlayerTeleportToTarget(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;
	UNITID idWeapon = tData.idWeapon;

	// get target

	UNIT *pTarget = SkillGetTarget( pUnit, nSkill, idWeapon );
	VECTOR vTargetPosition( 0 );

	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
	{
		if( UnitGetGenus( pUnit ) == GENUS_MISSILE )
		{
			pTarget = pUnit;
			pUnit = UnitGetUltimateOwner( pUnit );
			vTargetPosition = UnitGetPosition( pTarget );
		}
		else
		{
			vTargetPosition = UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
			pTarget = NULL;
		}

	}

	UNIT * pWeapon = UnitGetById( pGame, idWeapon );

	BOOL bIsInRange = FALSE;
	if ( VectorIsZero( vTargetPosition ) == FALSE ||
		 ( pTarget &&  UnitInLineOfSight( pGame, pUnit, pTarget ) && SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, nSkill, FALSE, &bIsInRange, TRUE ) && bIsInRange ) )
	{
		float nCollisionRadius( 0 );
		struct ROOM *pTargetRoom( NULL );
		if( pTarget )
		{
			pTargetRoom = UnitGetRoom( pTarget );
			vTargetPosition = UnitGetPosition(pTarget);
			nCollisionRadius = UnitGetCollisionRadius(pTarget);
		}
		else
		{
			//this might be bad....
			pTargetRoom = UnitGetRoom( pUnit );
		}
		VECTOR vTargetDirection = UnitGetPosition(pUnit) - vTargetPosition;
		float fDistance = VectorNormalize(vTargetDirection);

		ROOM * pWarpRoom = NULL;
		VECTOR vWarpLocation(0);
		VECTOR vIdealPosition = UnitGetPosition(pUnit) - (fDistance - UnitGetCollisionRadius(pUnit) - nCollisionRadius) * vTargetDirection;
		// figure out if the ideal position is available
		ROOM * pIdealRoom = NULL;
		ROOM_PATH_NODE * pIdealPathNode = RoomGetNearestPathNode(pGame, pTargetRoom, vIdealPosition, &pIdealRoom);
		if(pIdealRoom && pIdealPathNode)
		{
			DWORD dwBlockedNodeFlags = 0;
			SETBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED );
			VECTOR vIdealNodePosition = RoomPathNodeGetExactWorldPosition( pGame, pIdealRoom, pIdealPathNode );
			if(VectorDistanceSquared(vIdealNodePosition, vIdealPosition) <= 
#if HELLGATE_ONLY
				(pIdealPathNode->fRadius * pIdealPathNode->fRadius) &&
#else
				PATH_NODE_RADIUS * PATH_NODE_RADIUS &&
#endif
				( AppIsHellgate() && s_RoomNodeIsBlocked(pIdealRoom, pIdealPathNode->nIndex, dwBlockedNodeFlags, NULL) == NODE_FREE ) )
			{
				pWarpRoom = pIdealRoom;
				vWarpLocation = vIdealPosition;
			}
		}

		if(!pWarpRoom)
		{
			// find a spot to make the spawner at
			const int MAX_PATH_NODES = 6;
			FREE_PATH_NODE tFreePathNodes[MAX_PATH_NODES];

			NEAREST_NODE_SPEC tSpec;
			SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY );
			SETBIT( tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY );
			SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
			SETBIT( tSpec.dwFlags, NPN_SORT_OUTPUT );
			tSpec.vFaceDirection = vTargetDirection;
			tSpec.fMaxDistance = 3.0f;

			int nNumNodes = RoomGetNearestPathNodes(pGame, pTargetRoom, vTargetPosition, MAX_PATH_NODES, tFreePathNodes, &tSpec);

			if (nNumNodes > 0)
			{
				// pick the closest node and get world position
				int nPick = RandGetNum( pGame, nNumNodes );
				const FREE_PATH_NODE *pFreePathNode = &tFreePathNodes[nPick];
				pWarpRoom = pFreePathNode->pRoom;
				vWarpLocation = RoomPathNodeGetExactWorldPosition( pGame, pFreePathNode->pRoom, pFreePathNode->pNode );
			}
		}

		if(pWarpRoom)
		{
			// we're going to face toward the target
			VECTOR vToTarget = vTargetPosition - vWarpLocation;
			VectorNormalize(vToTarget);

			// go there
			UnitWarp(
				pUnit,
				pWarpRoom,
				vWarpLocation,
				vToTarget,
				UnitGetVUpDirection( pUnit ),
				0);
			if( AppIsTugboat() &&
				UnitIsA( pUnit, UNITTYPE_PLAYER ) )
			{
				s_StateSet( pUnit, pUnit, STATE_CAMERA_LERP, 120 );
			}

			if( pTarget && UnitIsJumping(pTarget))
			{
				UnitStartJump(pGame, pUnit, 0.0f);
			}

			return TRUE;  // skill successfull
		}
	}
	// no target found
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoAttackForMeleeAttack(
	const SKILL_EVENT_FUNCTION_DATA &tData,
	UNIT * pTarget,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	BOOL bUseBothWeaponsAtOnce,
#ifdef HAVOK_ENABLED
	const HAVOK_IMPACT & tImpact,
#endif
	DWORD dwUnitAttackUnitFlags)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if ( bUseBothWeaponsAtOnce )
	{
		s_UnitAttackUnit(tData.pUnit,
			pTarget,
			pWeapons,
			0,
#ifdef HAVOK_ENABLED
			&tImpact,
#endif
			dwUnitAttackUnitFlags );
	}
	else
	{
		UNIT * pWeaponSingle[ MAX_WEAPONS_PER_UNIT ];
		pWeaponSingle[ 1 ] = NULL;
		pWeaponSingle[ 0 ] = pWeapons[ 0 ];
		s_UnitAttackUnit(tData.pUnit,
			pTarget,
			pWeaponSingle,
			0,
#ifdef HAVOK_ENABLED
			&tImpact,
#endif
			dwUnitAttackUnitFlags );
		if ( pWeapons[ 1 ] )
		{
			pWeaponSingle[ 0 ] = pWeapons[ 1 ];
			s_UnitAttackUnit(tData.pUnit,
				pTarget,
				pWeaponSingle,
				0,
#ifdef HAVOK_ENABLED
				&tImpact,
#endif
				dwUnitAttackUnitFlags );
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoMeleeAttackToUnitsAlongWarpPath(
	const SKILL_EVENT_FUNCTION_DATA &tData,
	const VECTOR & vWarpLocation,
	float fImpactStrength)
{
#ifdef HAVOK_ENABLED
	HAVOK_IMPACT tHavokImpact;
	HavokImpactInit( tData.pGame, tHavokImpact, fImpactStrength, UnitGetCenter( tData.pUnit ), NULL );
#endif
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	BOOL bUseBothWeaponsAtOnce = SkillGetWeaponsForAttack( tData, pWeapons );

	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_DISTANCE_TO_LINE );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	tTargetingQuery.pvLocation  = &vWarpLocation;
	tTargetingQuery.fMaxRange	= SkillGetRange( tData.pUnit, tData.nSkill, pWeapons[ 0 ] );
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		sDoAttackForMeleeAttack( tData,
			pTarget,
			pWeapons,
			bUseBothWeaponsAtOnce,
#ifdef HAVOK_ENABLED
			tHavokImpact,
#endif
			0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct PLAYER_TELEPORT_TO_SAFETY_CONTEXT
{
	ROOM *				pDestinationRoom;
	ROOM_PATH_NODE *	pDestinationNode;
	float				fDot;
	BOOL				bMoreNodes;
	VECTOR				vDirection;
	BOOL				bForward;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerTeleportToSafetyFindPathNodeConnection(
	ROOM * pRoomSource,
	ROOM_PATH_NODE * pPathNodeSource,
	ROOM * pRoomDest,
	ROOM_PATH_NODE * pPathNodeDest,
	float fDistance, 
	void * pUserData)
{
	EVENT_DATA * pEventData = (EVENT_DATA*)pUserData;
	ASSERT_RETFALSE(pEventData);
	PLAYER_TELEPORT_TO_SAFETY_CONTEXT * pContext = (PLAYER_TELEPORT_TO_SAFETY_CONTEXT*)pEventData->m_Data1;
	ASSERT_RETFALSE(pContext);

	GAME * pGame = RoomGetGame(pRoomSource);

	VECTOR vCenterPosition = RoomPathNodeGetExactWorldPosition(pGame, pRoomSource, pPathNodeSource);
	VECTOR vOtherPosition = RoomPathNodeGetExactWorldPosition(pGame, pRoomDest, pPathNodeDest);

	VECTOR vDirection = vOtherPosition - vCenterPosition;
	VectorNormalize(vDirection);

	float fDot = VectorDot(vDirection, pContext->vDirection);
	if((pContext->bForward && fDot >= pContext->fDot) || (!pContext->bForward && fDot <= pContext->fDot))
	{
		pContext->fDot = fDot;
		pContext->pDestinationRoom = pRoomDest;
		pContext->pDestinationNode = pPathNodeDest;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerTeleportToSafetyIsAGoodDestination(
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode)
{
	ASSERT_RETFALSE(pRoom);
	ASSERT_RETFALSE(pPathNode);

#if HELLGATE_ONLY
	if(pPathNode->fRadius < 1.5f)
	{
		return FALSE;
	}
#endif

	if(s_RoomNodeIsBlocked(pRoom, pPathNode->nIndex, 0, NULL) == NODE_BLOCKED)
	{
		return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventPlayerTeleportToSafety(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;

	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pUnit);

	if (RoomIsActive(UnitGetRoom(pUnit)) == FALSE)
	{
		if (UnitEmergencyDeactivate(pUnit, "Attempt to call sSkillEventPlayerTeleportToSafety() in Inactive Room"))
		{
			return FALSE;
		}
	}

	VECTOR vPosition = UnitGetPosition(pUnit);
	VECTOR vMoveDirection = UnitGetMoveDirection(pUnit);
	VECTOR vFaceDirection = UnitGetFaceDirection(pUnit, FALSE);

	VECTOR vDirectionToWarp = vMoveDirection;
	if(VectorIsZeroEpsilon(vDirectionToWarp) || UnitGetMoveMode(pUnit) == INVALID_ID)
	{
		vDirectionToWarp = vFaceDirection;
	}

	ROOM * pDestinationRoom = NULL;
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode(pGame, UnitGetRoom(pUnit), vPosition, &pDestinationRoom, 0);
	if(!pDestinationRoom || !pPathNode)
	{
		return FALSE;
	}

	float fDistance = SkillEventGetParamFloat(pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0);
	float fMinDistance = SkillEventGetParamFloat(pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1);
	BOOL bDoAttack = SkillEventGetParamBool(pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2);
	REF(fMinDistance);

	VECTOR vCurrentPosition = RoomPathNodeGetExactWorldPosition(pGame, pDestinationRoom, pPathNode);
	float fCurrentLength = VectorLength(vCurrentPosition - vPosition);
	PLAYER_TELEPORT_TO_SAFETY_CONTEXT tContext;
	tContext.pDestinationRoom = pDestinationRoom;
	tContext.pDestinationNode = pPathNode;
	tContext.bMoreNodes = TRUE;
	tContext.fDot = 0.0f;
	tContext.vDirection = vDirectionToWarp;
	tContext.bForward = TRUE;

	int nIterationCount = 0;
	do
	{
		EVENT_DATA tEventData((DWORD_PTR)&tContext);
		ROOM * pRoomBefore = tContext.pDestinationRoom;
		ROOM_PATH_NODE * pNodeBefore = tContext.pDestinationNode;
		RoomPathNodeIterateConnections(tContext.pDestinationRoom, tContext.pDestinationNode, sPlayerTeleportToSafetyFindPathNodeConnection, &tEventData);
		tContext.fDot = 0.0f;
		if(tContext.pDestinationRoom != pRoomBefore || tContext.pDestinationNode != pNodeBefore)
		{
			VECTOR vPosBefore = RoomPathNodeGetExactWorldPosition(pGame, pRoomBefore, pNodeBefore);
			VECTOR vPosAfter = RoomPathNodeGetExactWorldPosition(pGame, tContext.pDestinationRoom, tContext.pDestinationNode);
			fCurrentLength += VectorLength(vPosAfter - vPosBefore);
		}
		else
		{
			tContext.bMoreNodes = FALSE;
		}
		nIterationCount++;
		if(nIterationCount >= 1000)
		{
			ASSERT_MSG("Max iteration count has been hit in sSkillEventlayerTeleportToSafety()");
			return FALSE;
		}
	}
	while(tContext.bMoreNodes && fCurrentLength < fDistance);

	if(fCurrentLength < fMinDistance)
	{
		return FALSE;
	}

	tContext.bForward = FALSE;
	BOUNDED_WHILE(!sPlayerTeleportToSafetyIsAGoodDestination(tContext.pDestinationRoom, tContext.pDestinationNode) && fCurrentLength > 0.0f, 1000)
	{
		EVENT_DATA tEventData((DWORD_PTR)&tContext);
		ROOM * pRoomBefore = tContext.pDestinationRoom;
		ROOM_PATH_NODE * pNodeBefore = tContext.pDestinationNode;
		RoomPathNodeIterateConnections(tContext.pDestinationRoom, tContext.pDestinationNode, sPlayerTeleportToSafetyFindPathNodeConnection, &tEventData);
		tContext.fDot = 0.0f;
		if(tContext.pDestinationRoom != pRoomBefore || tContext.pDestinationNode != pNodeBefore)
		{
			VECTOR vPosBefore = RoomPathNodeGetExactWorldPosition(pGame, pRoomBefore, pNodeBefore);
			VECTOR vPosAfter = RoomPathNodeGetExactWorldPosition(pGame, tContext.pDestinationRoom, tContext.pDestinationNode);
			fCurrentLength -= VectorLength(vPosAfter - vPosBefore);
		}
		else
		{
			break;
		}
	}

	if(fCurrentLength < fMinDistance)
	{
		return FALSE;
	}

	if(!sPlayerTeleportToSafetyIsAGoodDestination(tContext.pDestinationRoom, tContext.pDestinationNode))
	{
		return FALSE;
	}

	if(tContext.pDestinationRoom)
	{
		StateClearAllOfType( tData.pUnit, STATE_REMOVE_ON_MOVE );  // we need to do this before attacking anything to remove digin and similar skills

		VECTOR vWarpLocation = RoomPathNodeGetExactWorldPosition(pGame, tContext.pDestinationRoom, tContext.pDestinationNode);
		VECTOR vAimDirection = UnitGetFaceDirection(pUnit, FALSE);

		if(bDoAttack)
		{
			float fImpactStrength = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 3 );
			sDoMeleeAttackToUnitsAlongWarpPath(tData, vWarpLocation, fImpactStrength);
		}

		// go there
		UnitWarp(
			pUnit,
			tContext.pDestinationRoom,
			vWarpLocation,
			vAimDirection,
			UnitGetVUpDirection( pUnit ),
			0);

		return TRUE;  // skill successfull
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillGetWeaponsForAttack(
	const SKILL_EVENT_FUNCTION_DATA &tData,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ])
{
	BOOL bUseBothWeaponsAtOnce = FALSE;
	ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
	if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_COMBINE_WEAPON_DAMAGE ) )
	{
		sGetWeaponsForDamage( tData, pWeapons );
		bUseBothWeaponsAtOnce = TRUE;
	}
	else
	{
		pWeapons[ 0 ] = UnitGetById( tData.pGame, tData.idWeapon );
		if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_USE_ALL_WEAPONS ) )
		{
			pWeapons[ 1 ] = SkillGetLeftHandWeapon( tData.pGame, tData.pUnit, tData.nSkill, pWeapons[ 0 ] );
		} else {
			pWeapons[ 1 ] = NULL;
		}
	}
	return bUseBothWeaponsAtOnce;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventPlayerTeleportAndAttackTargets(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME *pGame = tData.pGame;

	VECTOR vTargetDirection = UnitGetFaceDirection( tData.pUnit, FALSE );
	vTargetDirection.fZ = 0.0f;
	VectorNormalize( vTargetDirection );

	float fDistance = SkillEventGetParamFloat(tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0);

	ROOM * pWarpRoom = NULL;
	VECTOR vWarpLocation(0);
	VECTOR vIdealPosition = UnitGetPosition(tData.pUnit) + fDistance * vTargetDirection;
	// figure out if the ideal position is available
	ROOM * pIdealRoom = NULL;
	ROOM_PATH_NODE * pIdealPathNode = RoomGetNearestPathNode(pGame, UnitGetRoom(tData.pUnit), vIdealPosition, &pIdealRoom);
	if(pIdealRoom && pIdealPathNode)
	{
		DWORD dwBlockedNodeFlags = 0;
		SETBIT( dwBlockedNodeFlags, BNF_NO_SPAWN_IS_BLOCKED );
		VECTOR vIdealNodePosition = RoomPathNodeGetExactWorldPosition( pGame, pIdealRoom, pIdealPathNode );
		if(VectorDistanceSquared(vIdealNodePosition, vIdealPosition) <= 
#if HELLGATE_ONLY
			(pIdealPathNode->fRadius * pIdealPathNode->fRadius) &&
#else
			PATH_NODE_RADIUS * PATH_NODE_RADIUS &&
#endif
			s_RoomNodeIsBlocked(pIdealRoom, pIdealPathNode->nIndex, dwBlockedNodeFlags, NULL) == NODE_FREE)
		{
			pWarpRoom = pIdealRoom;
			vWarpLocation = vIdealNodePosition;
		}
	}

	if(!pWarpRoom)
	{
		// find a spot to make the spawner at
		const int MAX_PATH_NODES = 6;
		FREE_PATH_NODE tFreePathNodes[MAX_PATH_NODES];

		NEAREST_NODE_SPEC tSpec;
		SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY );
		SETBIT( tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY );
		SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
		SETBIT( tSpec.dwFlags, NPN_SORT_OUTPUT );
		tSpec.vFaceDirection = vTargetDirection;
		tSpec.fMaxDistance = 3.0f;

		int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(tData.pUnit), UnitGetPosition( tData.pUnit ), MAX_PATH_NODES, tFreePathNodes, &tSpec);

		if (nNumNodes > 0)
		{
			// pick the closest node and get world position
			int nPick = RandGetNum( pGame, nNumNodes );
			const FREE_PATH_NODE *pFreePathNode = &tFreePathNodes[nPick];
			pWarpRoom = pFreePathNode->pRoom;
			vWarpLocation = RoomPathNodeGetExactWorldPosition( pGame, pFreePathNode->pRoom, pFreePathNode->pNode );
		}
	}

	if(pWarpRoom)
	{
		StateClearAllOfType( tData.pUnit, STATE_REMOVE_ON_MOVE );  // we need to do this before attacking anything to remove digin and similar skills

		float fImpactStrength = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );

		sDoMeleeAttackToUnitsAlongWarpPath(tData, vWarpLocation, fImpactStrength);

		// go there
		UnitWarp(
			tData.pUnit,
			pWarpRoom,
			vWarpLocation,
			vTargetDirection,
			UnitGetVUpDirection( tData.pUnit ),
			0);

		return TRUE;  // skill successfull
	}
#endif

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetUISkillPickMode(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(IS_CLIENT(tData.pGame));
	UNIT * pControlUnit = GameGetControlUnit(tData.pGame);
	if(pControlUnit && tData.pUnit == pControlUnit)
	{
		UISetCursorState( UICURSOR_STATE_SKILL_PICK_ITEM, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );
		if (!UIIsInventoryPackPanelUp())
		{
			UIShowInventoryScreen();
		}
	}
#endif //!ISVERSION(SERVER_VERSION)
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sClearUIskillPickModeCallback(
	GAME* pGame,
	UNIT* unit,
	const EVENT_DATA& event_data)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(IS_CLIENT(pGame));
	UNIT * pControlUnit = GameGetControlUnit(pGame);
	if(pControlUnit && unit == pControlUnit)
	{
		UISetCursorState( UICURSOR_STATE_POINTER );
	}
#endif //!ISVERSION(SERVER_VERSION)
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventClearUISkillPickMode(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(SERVER_VERSION)
	int nDelay = SkillEventGetParamInt(tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0);

	EVENT_DATA ed;
	if(nDelay > 0)
	{
		UnitRegisterEventTimed(tData.pUnit, sClearUIskillPickModeCallback, &ed, nDelay);
	}
	else
	{
		sClearUIskillPickModeCallback(tData.pGame, tData.pUnit, ed);
	}
#endif //!ISVERSION(SERVER_VERSION)
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventCreateTurret(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	ASSERT_RETFALSE(tData.pSkillEvent->tAttachmentDef.nAttachedDefId >= 0);

	// Get the target
	UNIT * pTargetWeapon = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon, 0);
	int nOriginalLocation, nOriginalX, nOriginalY;
	UnitGetInventoryLocation(pTargetWeapon, &nOriginalLocation, &nOriginalX, &nOriginalY);

	if (nOriginalLocation != INVLOC_BIGPACK)
	{
		// This will only work on items in the unit's pack
		return FALSE;
	}

	// this cannot work on unidentified items
	if (ItemIsUnidentified( pTargetWeapon ))
	{
		return FALSE;
	}

	// Create a turret consumable unit in the ether
	ITEM_SPEC tSpec;
	tSpec.nOverrideInventorySizeX = UnitGetStat(pTargetWeapon, STATS_INVENTORY_WIDTH);
	tSpec.nOverrideInventorySizeY = UnitGetStat(pTargetWeapon, STATS_INVENTORY_HEIGHT);
	tSpec.nItemClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	tSpec.pSpawner = tData.pUnit;
	UNIT * pConsumable = s_ItemSpawn(tData.pGame, tSpec, NULL);

	// Remove the gun from the inventory
	pTargetWeapon = UnitInventoryRemove(pTargetWeapon, ITEM_ALL);

	// See if there's enough room for the turret
	BOOL bHasRoom = UnitInventoryCanPickupAll(tData.pUnit, &pConsumable, 1);

	// Put the gun back where it was
	UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, tData.pUnit, pTargetWeapon, nOriginalLocation, nOriginalX, nOriginalY);

	BOOL bFailure = TRUE;
	DWORD dwFreeFlags = 0;
	ONCE
	{
		if(bHasRoom)
		{
			bFailure = FALSE;
			// mark the item as can send messages
			UnitSetCanSendMessages( pConsumable, TRUE );

			// Warp the unit to the player's location
			UnitWarp(pConsumable, UnitGetRoom(tData.pUnit), UnitGetPosition(tData.pUnit), VECTOR(0, 1, 0), VECTOR(0, 0, 1), 0, TRUE);

			// Change the inventory location of the gun into the turret
			if(!InventoryChangeLocation(pConsumable, pTargetWeapon, INVLOC_RHAND, -1, -1, CLF_ALLOW_NEW_CONTAINER_BIT))
			{
				dwFreeFlags |= UFF_SEND_TO_CLIENTS;
				bFailure = TRUE;
				break;
			}

			// Have the player pick up the item
			UNIT * pPickupItem = s_ItemPickup(tData.pUnit, pConsumable);
			if(!pPickupItem)
			{
				dwFreeFlags |= UFF_SEND_TO_CLIENTS;
				bFailure = TRUE;
				break;
			}

			InventoryChangeLocation(tData.pUnit, pPickupItem, nOriginalLocation, nOriginalX, nOriginalY, 0);
		}

	}

	if(bFailure)
	{
		// Free the turret
		UnitFree(pConsumable, dwFreeFlags);
	}
	return TRUE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sDecoyGotHit(
	GAME* pGame,
	UNIT *pDefender,
	UNIT *pAttacker,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId )
{
	ASSERTX_RETURN( pDefender, "Expected defender" );
	ASSERTX_RETURN( UnitIsDecoy( pDefender ), "Expected decoy" );
	DWORD dwNow = GameGetTick( pGame );

	// check to see if it's been long enough between the time we were last hit to register
	// another (we do this so decoys can't be pounded away too quickly by hordes of
	// monsters or laser weapons)
	const DWORD TICKS_REQUIRED_BETWEEN_DECOY_HITS = GAME_TICKS_PER_SECOND * 1;
	DWORD dwLastHitTick = UnitGetStat( pDefender, STATS_DECOY_LAST_HIT_TICK );
	if (dwNow - dwLastHitTick > TICKS_REQUIRED_BETWEEN_DECOY_HITS)
	{

		// get number of hits we have left
		int nNumHitsLeft = UnitGetStat( pDefender, STATS_DECOY_HITS_LEFT );
		if (nNumHitsLeft > 0)
		{

			// take a hit away and check for expiration
			nNumHitsLeft--;
			UnitSetStat( pDefender, STATS_DECOY_HITS_LEFT, nNumHitsLeft );

			// mark the time we were last hit on
			UnitSetStat( pDefender, STATS_DECOY_LAST_HIT_TICK, dwNow );

			// check for "death"
			if (nNumHitsLeft == 0)
			{
				UnitDie( pDefender, pAttacker );
			}

		}

	}

}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sSpawnDecoy(
	UNIT *pSourcePlayer,
	const FREE_PATH_NODE &tFreePathNode,
	int nLifetimeInTicks,
	int nState )
{
	ASSERTX_RETURN( pSourcePlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pSourcePlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );

	// get some creation params
	const UNIT_DATA *pUnitData = UnitGetData( pSourcePlayer );
	VECTOR vDirection = UnitGetDirectionXY( pSourcePlayer );
	int nMonsterClass = pUnitData->nMonsterDecoy;
	ASSERTX_RETURN( nMonsterClass != INVALID_LINK, "No decoy monster specified for player class" );

	// get world position
	VECTOR vSpawnPosition = RoomPathNodeGetWorldPosition(
		pGame,
		tFreePathNode.pRoom,
		tFreePathNode.pNode);

	// create the decoy
	MONSTER_SPEC tSpec;
	tSpec.nClass = nMonsterClass;
	tSpec.nExperienceLevel = UnitGetExperienceLevel( pSourcePlayer );
	tSpec.nMonsterQuality = INVALID_LINK;
	tSpec.pRoom = tFreePathNode.pRoom;
	tSpec.pPathNode = tFreePathNode.pNode;
	tSpec.vPosition = vSpawnPosition;
	tSpec.vDirection = vDirection;
	tSpec.dwFlags = MSF_KEEP_NETWORK_RESTRICTED | MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE | MSF_NO_DATABASE_OPERATIONS;

	UNIT *pDecoy = s_MonsterSpawn( pGame, tSpec );
	if (pDecoy)
	{
		UnitSetFlag( pDecoy, UNITFLAG_IS_DECOY );

		if ( nState != INVALID_ID )
			s_StateSet( pDecoy, pSourcePlayer, nState, 0 );

		// clone the wardrobe of the source unit
		UNIT *pItem = NULL;
		while ((pItem = UnitInventoryIterate( pSourcePlayer, pItem )) != NULL)
		{
			if (ItemIsEquipped(pItem, pSourcePlayer) && UnitIsA( pItem, UNITTYPE_WEAPON ) )
			{

				// copy item
				UNIT *pNewItem = s_ItemCloneAndCripple( pItem );

				// put item on ground
				UnitWarp(
					pNewItem,
					tFreePathNode.pRoom,
					vSpawnPosition,
					cgvYAxis,
					cgvZAxis,
					0,
					FALSE);

				// pick up item to location
				s_ItemPickup( pDecoy, pNewItem );

			}

		}

		// give them the same name
		WCHAR uszName[ MAX_CHARACTER_NAME ];
		UnitGetName( pSourcePlayer, uszName, arrsize(uszName), 0 );
		UnitSetName( pDecoy, uszName );

		// reset their experience to zero so that players can't cheese them in PvP
		UnitSetStat(pDecoy, STATS_EXPERIENCE, 0);

		// register got hit handler
		UnitRegisterEventHandler( pGame, pDecoy, EVENT_GOTHIT, s_sDecoyGotHit, NULL );

		// set the decoy to automatically go away in a little while
		UnitSetFuse(pDecoy, nLifetimeInTicks);

		// enable the unit for network sending
		UnitSetCanSendMessages( pDecoy, TRUE );

		// send to all who care
		ROOM *pRoom = UnitGetRoom( pDecoy );
		s_SendUnitToAllWithRoom( pGame, pDecoy, pRoom );
		UnitInventoryAdd(INV_CMD_ADD_PET, pSourcePlayer, pDecoy, INVLOC_MAJORPET);

		//INVLOC locData;
		//int nInventoryLocation = InventoryGetLoc(pSourcePlayer, INVLOC,&locData);


	}

}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventDecoy(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;

	int nLifetimeInTicks = SkillEventGetParamInt( pUnit, nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	ASSERT_RETFALSE(nLifetimeInTicks > 0);

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	// find spots for the decoys
	VECTOR vDirection = UnitGetFaceDirection( pUnit, FALSE );
	const int MAX_PATH_NODES = 64;
	FREE_PATH_NODE tFreePathNodes[ MAX_PATH_NODES ];

	NEAREST_NODE_SPEC tSpec;
	SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY );
	SETBIT( tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY );
	tSpec.vFaceDirection = vDirection;
	tSpec.fMinDistance = 5.0f;
	tSpec.fMaxDistance = 30.0f;

	int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pUnit), UnitGetPosition(pUnit), MAX_PATH_NODES, tFreePathNodes, &tSpec);
	if (nNumNodes <= 0)
	{
		return FALSE;
	}

	// create the decoys
	if (nNumNodes > 0)
	{
		// pick a spot for the decoy
		int nPick = RandGetNum( pGame, nNumNodes - 1 );
		FREE_PATH_NODE tFreePathNode = tFreePathNodes[ nPick ];

		// remove this spot from the available spots
		tFreePathNodes[ nPick ] = tFreePathNodes[ nNumNodes - 1 ];
		nNumNodes--;

		// spawn the decoy
		sSpawnDecoy( pUnit, tFreePathNode, nLifetimeInTicks, nState );
		return TRUE;
		//int nFuseTicks = SkillEventGetParamInt(tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2);
		//return sDoSkillEventSpawnPet(tData, 0, VECTOR(0,0,0), nFuseTicks);

	}
	return FALSE;

}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSetStateTargetToSkillTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTarget = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
	if (!pTarget)
	{
		return TRUE;
	}
	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	s_StateAddTarget( tData.pGame, UnitGetId( tData.pUnit ), tData.pUnit, pTarget, nState, 0 );
	return TRUE;
}
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventDie(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	UNIT *pUnit = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnit)
		return TRUE;

	BOOL bDestroyBody = SkillEventGetParamBool( pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	// if destroy body is set, force this unit to "destroy on die"
	if (bDestroyBody == TRUE)
	{
		UnitSetFlag( pUnit, UNITFLAG_ON_DIE_DESTROY );
	}

	// die unit die!
	UnitDie( pUnit, NULL );

	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFreeSelf(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pUnitToFree = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToFree = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnitToFree)
		return TRUE;

	UnitFree(pUnitToFree, UFF_SEND_TO_CLIENTS);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoMeleeItemEvents(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.fDuration != 0.0f );
	float flDuration = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	ASSERT_RETFALSE( flDuration > 0.0f );
	ASSERT_RETFALSE( flDuration < 1.0f );

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_OFFHAND_WEAPON )
	{
		pWeapon = SkillGetLeftHandWeapon( tData.pGame, tData.pUnit, tData.nSkill, pWeapon );
	}
	if ( ! pWeapon )
		return TRUE;

	SKILL_EVENTS_DEFINITION * pSkillEvents = SkillGetWeaponMeleeSkillEvents( tData.pGame, pWeapon );
	if ( ! pSkillEvents )
		return TRUE;

	ASSERT_RETFALSE( pSkillEvents->nEventHolderCount == 1 );

	float fDuration = tData.fDuration * flDuration;
	SKILL_EVENT_HOLDER * pHolder = &pSkillEvents->pEventHolders[ 0 ];
	SkillScheduleEvents( tData.pGame, tData.pUnit, tData.nSkill, tData.nSkillLevel, UnitGetId( pWeapon ), tData.pSkillData, pHolder, 0.0f, fDuration, TRUE, FALSE, 0 );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventDropLoot(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	int nTreasureClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if (nTreasureClass < 0)
	{
		nTreasureClass = tData.pUnit->pUnitData->nTreasure;
		if (nTreasureClass < 0)
		{
			return FALSE;
		}
	}

	// get the object that is the trigger of the spawning
	UNITID idOperator = (UNITID)UnitGetStat( tData.pUnit, STATS_SPAWN_SOURCE );
	UNIT *pOperator = UnitGetById( tData.pGame, idOperator );
	s_TreasureSpawnInstanced( tData.pUnit, pOperator, nTreasureClass );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventOverrideLoot(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));

	int nTreasureClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if(nTreasureClass < 0)
		return FALSE;

	UnitSetStat(tData.pUnit, STATS_TREASURE_CLASS, nTreasureClass);

	return TRUE;
}

#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventOperateDoor(
	const SKILL_EVENT_FUNCTION_DATA & tData  )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	BOOL bIsIdle = UnitTestMode( tData.pUnit, MODE_IDLE ) && ! UnitTestMode( tData.pUnit, MODE_OPEN );
//	BOOL bIsOpened = UnitTestMode( tData.pUnit, MODE_OPENED ) && ! UnitTestMode( tData.pUnit, MODE_CLOSE );

	if( bIsIdle )
	{
		//if( IS_SERVER( tData.pGame ) )
		{
			s_ObjectDoorOperate( tData.pUnit, DO_OPEN );
		}
		//RoomReserveLocation( tData.pGame, UnitGetRoom(tData.pUnit), tData.pUnit);
		//ObjectClearBlocking( tData.pUnit  );
		ObjectClearBlockedPathNodes( tData.pUnit );
	}
	/*else if( bIsOpened )
	{
		//if( IS_SERVER( tData.pGame ) )
		{
			BOOL bHasMode = FALSE;
			float fDuration = UnitGetModeDuration( tData.pGame, tData.pUnit, MODE_CLOSE, bHasMode );
			ASSERTX( bHasMode, "object missing Close Mode" );
			s_UnitSetMode( tData.pUnit, MODE_CLOSE, 0, fDuration, TRUE );
		}
		//RoomReserveLocation( tData.pGame, UnitGetRoom(tData.pUnit), tData.pUnit );//, FALSE );
		//ObjectSetBlocking( tData.pUnit  );
		ObjectReserveBlockedPathNodes( tData.pUnit );

	}*/ // NO DOOR CLOSING FOR NOW!


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRangedInstantHit(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nMaxTargets = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float fAwakenRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	int nMissile = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	ASSERT_RETFALSE(nMissile != INVALID_ID);

	const UNIT_DATA * missile_data = MissileGetData(tData.pGame, nMissile);
	ASSERT_RETFALSE(missile_data);

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	sGetWeaponsForDamage( tData, pWeapons );

	UNIT * pTarget = UnitTestFlag( tData.pUnit, UNITFLAG_ALWAYS_AIM_AT_TARGET ) ? SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon ) : NULL;

	int nInstantHitCount = sSkillEventGetExtraBulletCount(tData.pUnit, tData.nSkill, tData.pSkillData) + 1;

	for(int i=0; i<nInstantHitCount; i++)
	{
		DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapons[ 0 ], tData.nSkill );
		VECTOR vPosition;
		VECTOR vDirection;
		SkillGetFiringPositionAndDirection( tData.pGame, tData.pUnit, tData.nSkill, tData.idWeapon, tData.pSkillEvent, missile_data,
			vPosition, &vDirection, pTarget, NULL, dwSeed );

		VECTOR vNormal( 0, 0, 1 );
		float fDistance = 0.0f;

		BOOL bIsReflective = FALSE;
		BOOL bBulletsRetarget = FALSE;
		int nMaxRicochetCount = UnitGetStat( tData.pUnit, STATS_MISSILE_MAX_RICOCHET_COUNT );
		MissileGetReflectsAndRetargets( tData.pUnit, tData.pUnit, bIsReflective, bBulletsRetarget, nMaxRicochetCount );

		int nRicochetCount = nMaxRicochetCount;
		nMaxTargets = MAX(nMaxTargets, nRicochetCount);

		float fRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeapons[ 0 ] );
		float fSkillRange = fRange;
		// Hellgate has changed this mechanic, but Tugboat doesn't want it changed, at least for now.
		if(AppIsTugboat() && (bIsReflective || bBulletsRetarget))
		{
			fRange *= 10.0f;
		}
		UNIT * pTargetUnit = NULL;
		int nTargets = 0;

		STATS * pStats = StatsListInit(tData.pGame);
		ASSERT_RETFALSE(pStats);

		StatsSetStat(tData.pGame, pStats, STATS_DAMAGE_INCREMENT_PCT, 100);

		StatsListAdd(tData.pUnit, pStats, FALSE);

		do
		{
			fRange -= fDistance;

			pTargetUnit = SelectTarget( tData.pGame, tData.pUnit, SELECTTARGET_FLAG_DONT_RESELECT_PREVIOUS | SELECTTARGET_FLAG_DONT_RETURN_SELECTION_ERROR, vPosition, vDirection,
				fRange, NULL, &fDistance, &vNormal, tData.nSkill, pWeapons[ 0 ], UnitGetId(pTargetUnit));

			if(!pTargetUnit)
			{
				pTargetUnit = SelectTarget( tData.pGame, tData.pUnit, SELECTTARGET_FLAG_ALLOW_NOTARGET | SELECTTARGET_FLAG_DONT_RESELECT_PREVIOUS | SELECTTARGET_FLAG_DONT_RETURN_SELECTION_ERROR,
					vPosition, vDirection, fRange, NULL, &fDistance, &vNormal,
					tData.nSkill, pWeapons[ 0 ], UnitGetId(pTargetUnit));
			}

			if ( ! pTargetUnit && SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_MUST_TARGET_UNIT ) )
				return FALSE;

#if !ISVERSION(CLIENT_ONLY_VERSION)
			if ( IS_SERVER( tData.pGame ) )
			{
				if(missile_data)
				{
					if (pTargetUnit)
					{
#ifdef HAVOK_ENABLED
						HAVOK_IMPACT tImpact;
						if( AppIsHellgate() )
						{
							HavokImpactInit( tData.pGame, tImpact, missile_data->fForce, vPosition, &vDirection );
							if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_FORCE_IGNORES_SCALE ))
							{
								tImpact.dwFlags |= HAVOK_IMPACT_FLAG_IGNORE_SCALE;
							}
						}
#endif
						DWORD dwFlags = 0;

						int nChanceToPenetrateShields = UnitGetStat( tData.pUnit, STATS_CHANCE_THAT_MISSILES_PENETRATE_SHIELDS );
						if ( nChanceToPenetrateShields != 0 && (int)RandGetNum( tData.pGame, 100 ) < nChanceToPenetrateShields )
						{
							SETBIT(dwFlags, UAU_IGNORE_SHIELDS_BIT);
						}

						SETBIT(dwFlags, UAU_DIRECT_MISSILE_BIT);
						s_UnitAttackUnit( tData.pUnit,
							pTargetUnit,
							pWeapons,
							0,
#ifdef HAVOK_ENABLED
							&tImpact,
#endif
							dwFlags);
					}
					else if(UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_DAMAGES_ON_HIT_BACKGROUND))
					{
						VECTOR vNewPosition = vPosition + (fDistance - missile_data->fHitBackup) * vDirection;
						ROOM * pRoom = RoomGetFromPosition(tData.pGame, UnitGetLevel(tData.pUnit), &vNewPosition);
						
						if(pRoom)
						{
#ifdef HAVOK_ENABLED
							HAVOK_IMPACT tImpact;
							if( AppIsHellgate() )
							{
								HavokImpactInit( tData.pGame, tImpact, missile_data->fForce, vPosition, &vDirection );
								if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_FORCE_IGNORES_SCALE ))
								{
									tImpact.dwFlags |= HAVOK_IMPACT_FLAG_IGNORE_SCALE;
								}
							}
#endif
							DWORD dwFlags = 0;
							SETBIT(dwFlags, UAU_DIRECT_MISSILE_BIT);
							s_UnitAttackLocation(tData.pUnit, 
								pWeapons, 
								0, 
								pRoom, 
								vNewPosition, 
#ifdef HAVOK_ENABLED
								tImpact,
#endif
								dwFlags);
						}
					}
				}
			}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

			BOOL bHitSomething = pTargetUnit != NULL || fDistance < fRange;

			if ( bHitSomething && IS_CLIENT( tData.pGame ) )
			{
				if (missile_data)
				{
					fDistance -= missile_data->fHitBackup;
				}

				VECTOR vDelta = vDirection;
				vDelta *= fDistance;
				VECTOR vTarget = vPosition + vDelta;

				MISSILE_EFFECT eEffect = pTargetUnit ? MISSILE_EFFECT_IMPACT_UNIT : MISSILE_EFFECT_IMPACT_WALL;
				int nParticleEffect = c_MissileEffectCreate( tData.pGame, tData.pUnit, pWeapons[ 0 ], nMissile, eEffect,
					vTarget, vNormal );
				REF(nParticleEffect);
			}

			vPosition += fDistance * vDirection;
			if(pTargetUnit)
			{
				nTargets++;
			}

			if ( IS_SERVER( tData.pGame ) && fAwakenRange > 0.0f )
			{// awaken nearby monsters to the shooter
				// find all nearby targets
				UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
				SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
				SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
				tTargetingQuery.fMaxRange	= fAwakenRange;
				tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
				tTargetingQuery.pnUnitIds   = pnUnitIds;
				tTargetingQuery.pSeekerUnit = tData.pUnit;
				tTargetingQuery.pvLocation = &vPosition;
				SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
				SkillTargetQuery( tData.pGame, tTargetingQuery );

				for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
				{
					UNIT * pAwakened = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
					if ( ! pAwakened )
						continue;
					if ( UnitGetStat(pAwakened, STATS_AI) == INVALID_ID )
						continue;
					UnitSetAITarget( pAwakened, tData.pUnit );
				}
			}

			nRicochetCount--;

			if(bIsReflective || bBulletsRetarget)
			{
				float * pRange = NULL;
				if(AppIsHellgate())
				{
					if(bIsReflective && bBulletsRetarget)
					{
						fRange = fSkillRange / 2.0f;
					}
					else
					{
						fRange = fSkillRange / 3.0f;
					}
					fDistance = 0.0f;
					pRange = &fRange;
				}
				DWORD dwNewDirectionFlags = 0;
				SETBIT(dwNewDirectionFlags, MBND_RETARGET_BIT, bBulletsRetarget == TRUE);
				//SETBIT(dwNewDirectionFlags, MBND_NEW_DIRECTION_BIT, bIsReflective == TRUE);
				SETBIT(dwNewDirectionFlags, MBND_NEW_DIRECTION_BIT, FALSE);
				CLEARBIT(dwNewDirectionFlags, MBND_HAS_SIDE_EFFECTS_BIT);
				vDirection = MissileBounceFindNewDirection(tData.pGame, tData.pUnit, vPosition, vDirection, pTargetUnit, vNormal, dwNewDirectionFlags, pRange);

#if !ISVERSION(CLIENT_ONLY_VERSION)
				if(bBulletsRetarget)
				{
					int nPercent = UnitGetStat(tData.pUnit, STATS_MISSILE_RICOCHET_DMG_MODIFIER);
					if(nPercent != 0)
					{
						int nDamageIncrementPercent = StatsGetStat(pStats, STATS_DAMAGE_INCREMENT_PCT);
						nDamageIncrementPercent = PCT(nDamageIncrementPercent, nPercent);
						StatsSetStat(tData.pGame, pStats, STATS_DAMAGE_INCREMENT_PCT, nDamageIncrementPercent);
					}
				}
#endif
			}
		}
		while(fDistance < fRange &&
			((pTargetUnit && nMaxRicochetCount <= 0) || (nMaxRicochetCount > 0 && nRicochetCount >= 0)) &&
			(nMaxTargets == -1 || nTargets < nMaxTargets));

		StatsListRemove(tData.pGame, pStats);
		StatsListFree(tData.pGame, pStats);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventPlaceMissilesOnTargets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nMaxTargetsParam = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	sGetWeaponsForDamage( tData, pWeapons );

	float fRangeMax = SkillGetRange( tData.pUnit, tData.nSkill, pWeapons[ 0 ] );
	ASSERT_RETFALSE(fRangeMax != 0.0f);

	// find all nearby targets
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRangeMax;
	tTargetingQuery.nUnitIdMax  = MIN( nMaxTargetsParam, MAX_TARGETS_PER_QUERY );
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	// fire a missile at each target and give the target to the missile
	WORD wMissileTag = GameGetMissileTag(tData.pGame);
	int nMissileClass = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	DWORD dwSeed = SkillGetSeed( tData.pGame, tData.pUnit, pWeapons[ 0 ], tData.nSkill );
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		DWORD dwFlags = 0;
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_ADD_TO_CENTER )
		{
			SETBIT(dwFlags, MF_PLACE_ON_CENTER_BIT);
		}
		MissileFire( tData.pGame, nMissileClass, tData.pUnit, pWeapons, tData.nSkill, pTarget, VECTOR( 0 ),
					UnitGetPosition(pTarget), VECTOR( 0, 0, 1 ), dwSeed, wMissileTag, dwFlags );
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetStat(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( IS_SERVER(tData.pGame) );
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	int nValue = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	UnitSetStat( tData.pUnit, tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0, nValue );
	return TRUE;
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetCopyEffectStat(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( IS_SERVER(tData.pGame) );
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	if( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_LINK )
	{
		UNITID idSFXEffectGivenBy = UnitGetStat( tData.pUnit, STATS_SFX_EFFECT_GIVEN_BY, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );
		if (idSFXEffectGivenBy != INVALID_ID)
		{
			UNIT *attacker = UnitGetById( tData.pGame, idSFXEffectGivenBy );
			if( attacker )
			{
				int valueIs = UnitGetStat( attacker, STATS_SFX_EFFECT_ATTACK, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );
				UnitSetStat( tData.pUnit, STATS_SFX_ATTACK, tData.pSkillEvent->tAttachmentDef.nAttachedDefId, valueIs  );
			}
		}

	}

	//UnitSetStat( tData.pUnit, tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0, pParam0->nValue );
	return TRUE;
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
static BOOL sSkillEventIncrementStat(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( IS_SERVER(tData.pGame) );
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	int nValue = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	UnitChangeStat( tData.pUnit, tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0, nValue );
	return TRUE;
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
static BOOL sSkillEventDecrementStat(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( IS_SERVER(tData.pGame) );
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	int nValue = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	UnitChangeStat( tData.pUnit, tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0, -nValue );
	return TRUE;
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSelectSpawnLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fHeight = 0.0f;
	float fRadius = 0.0f;
	const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = (const SKILL_EVENT_TYPE_DATA *) ExcelGetData( tData.pGame, DATATABLE_SKILLEVENTTYPES, tData.pSkillEvent->nType );
	const UNIT_DATA * pUnitData = (const UNIT_DATA *) ExcelGetData(tData.pGame, pSkillEventTypeData->eAttachedTable, tData.pSkillEvent->tAttachmentDef.nAttachedDefId);
	if (pUnitData)
	{
		fHeight = pUnitData->fCollisionHeight;
		fRadius = pUnitData->fPathingCollisionRadius;
	}

#if ISVERSION(DEVELOPMENT)	// there seem to be some bugs if radius is too small?
	ASSERT(fRadius >= 0.1f);
#endif
	if (fRadius < 0.1f)
	{
		fRadius = 0.1f;
	}

	VECTOR vTargetUnitPosition = UnitGetPosition(tData.pUnit);
	float fDistanceAhead = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	BOOL bSpawnInAir = ( SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ) > 0.0f );
	int nSpawnLocationCountOverride = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	VECTOR vModifiedFacing;

	LEVEL * pLevel = UnitGetLevel( tData.pUnit );
	VECTOR vDestination(0);
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION)
	{
		vDestination = UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
	}
	else
	{
		// KCK Added: Allow the skill to designate an offset for this spawn location
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_EVENT_OFFSET )
		{
			MATRIX	mWorld;
			UnitGetWorldMatrix( tData.pUnit, mWorld );
			MatrixMultiplyNormal( &vModifiedFacing, &tData.pSkillEvent->tAttachmentDef.vPosition, &mWorld );
			fDistanceAhead = VectorLength(tData.pSkillEvent->tAttachmentDef.vPosition);
			VectorNormalize(vModifiedFacing);
		}
		else
		{
			VECTOR vTargetUnitFacing = UnitGetFaceDirection(tData.pUnit, FALSE);
			vTargetUnitFacing.fZ = 0.0f;
			VECTOR vRand = RandGetVectorEx(tData.pGame) / fDistanceAhead;
			vRand.fZ = 0.0f;
			vModifiedFacing = vTargetUnitFacing + vRand;
			VectorNormalize(vModifiedFacing);
		}

		float fCollideDistanceAhead = fDistanceAhead;
		{
			VECTOR vCollisionSearchStart = vTargetUnitPosition;
			vCollisionSearchStart.fZ += UnitGetCollisionHeight( tData.pUnit );
			fCollideDistanceAhead = LevelLineCollideLen(tData.pGame, pLevel, vCollisionSearchStart, vModifiedFacing, fDistanceAhead);
			fCollideDistanceAhead -= 0.1f;
		}

		vDestination = vTargetUnitPosition + vModifiedFacing * fCollideDistanceAhead;
	}

	ASSERTX( tData.pSkillData->pnSummonedInventoryLocations[ 1 ] == INVALID_ID, tData.pSkillData->szName ); // it doesn't make sense to have two inventory locations here

	//NOTE Tugboat does have all this. If this doesn't work. Merge back in code for spawn location
	int nSpawnLocationCount = tData.pSkillData->pnSummonedInventoryLocations[ 0 ] != INVALID_ID ? UnitInventoryGetArea(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ 0 ]) : 1;
	if(nSpawnLocationCountOverride > 0)
	{
		nSpawnLocationCount = nSpawnLocationCountOverride;
	}
	nSpawnLocationCount = PIN(nSpawnLocationCount, 1, MAX_TARGETS_PER_QUERY);

	FREE_PATH_NODE tFreePathNodes[MAX_TARGETS_PER_QUERY];
	NEAREST_NODE_SPEC tSpec;
	if ( pSkillEventTypeData->eAttachedTable == DATATABLE_MISSILES )
	{
		SETBIT( tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES );
		SETBIT( tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT );
		SETBIT( tSpec.dwFlags, NPN_DONT_CHECK_RADIUS );
		SETBIT( tSpec.dwFlags, NPN_CHECK_LOS );
	}
	tSpec.pUnit = tData.pUnit;
	tSpec.fMinHeight = fHeight;
	tSpec.fMinRadius = fRadius;
	tSpec.fMaxDistance = 3.0f;
	SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
	SETBIT( tSpec.dwFlags, NPN_SORT_OUTPUT );
	int nNumLocationsFound = RoomGetNearestPathNodes(tData.pGame, UnitGetRoom(tData.pUnit), vDestination, MAX_TARGETS_PER_QUERY, tFreePathNodes, &tSpec);

	if(nNumLocationsFound <= 0)
	{
		// One last raycast, to get things down to ground level:
		float fDistanceDown = LevelLineCollideLen(tData.pGame, pLevel, vDestination, VECTOR(0, 0, -1), 10.0f);
		vDestination = vDestination + VECTOR(0, 0, -1) * fDistanceDown;

		CLEARBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
		nNumLocationsFound = RoomGetNearestPathNodes(tData.pGame, UnitGetRoom(tData.pUnit), vDestination, MAX_TARGETS_PER_QUERY, tFreePathNodes, &tSpec);
		if (nNumLocationsFound <= 0)
		{
			return FALSE;
		}
	}


	for(int i=0; i<nSpawnLocationCount; i++)
	{
		VECTOR vTarget = UnitGetPosition( tData.pUnit );
		if(i < nNumLocationsFound)
		{
			if(tFreePathNodes[i].pNode && tFreePathNodes[i].pRoom)
			{
				// path normals are not always vertical, so you can't just offset straight up from the node position, however the whole spawin in air thing could be better and data driven
				if (bSpawnInAir)
					vTarget = CalculatePathTargetFly(tData.pGame, MONSTERS_SPAWN_IN_AIR_ALTITUDE, fHeight, tFreePathNodes[i].pRoom, tFreePathNodes[i].pNode);
				else
					vTarget = RoomPathNodeGetExactWorldPosition(tData.pGame, tFreePathNodes[i].pRoom, tFreePathNodes[i].pNode);
			}
		}
		UnitSetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, i, vTarget);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
static BOOL sSkillEventSelectRandomSpawnLocation(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	// NOTE: This only supports one location for now. Specifying multiple 
	// locations will result in them all receiving the same location vector  -- MPT

	float fHeight = 0.0f;
	float fRadius = 0.0f;
	const SKILL_EVENT_TYPE_DATA * pSkillEventTypeData = (const SKILL_EVENT_TYPE_DATA *) ExcelGetData( tData.pGame, DATATABLE_SKILLEVENTTYPES, tData.pSkillEvent->nType );
	const UNIT_DATA * pUnitData = (const UNIT_DATA *) ExcelGetData(tData.pGame, pSkillEventTypeData->eAttachedTable, tData.pSkillEvent->tAttachmentDef.nAttachedDefId);
	if (pUnitData)
	{
		fHeight = pUnitData->fCollisionHeight;
		fRadius = pUnitData->fPathingCollisionRadius;
	}

#if ISVERSION(DEVELOPMENT)	// there seem to be some bugs if radius is too small?
	ASSERT(fRadius >= 0.1f);
#endif
	if (fRadius < 0.1f)
	{
		fRadius = 0.1f;
	}

	BOOL bSendToClient = ( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ) > 0 );
	BOOL bSpawnInAir = ( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ) > 0 );

	ASSERTX( tData.pSkillData->pnSummonedInventoryLocations[ 1 ] == INVALID_ID, tData.pSkillData->szName ); // it doesn't make sense to have two inventory locations here

	//NOTE Tugboat does have all this. If this doesn't work. Merge back in code for spawn location
	int nSpawnLocationCount = tData.pSkillData->pnSummonedInventoryLocations[ 0 ] != INVALID_ID ? UnitInventoryGetArea(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ 0 ]) : 1;
	nSpawnLocationCount = PIN(nSpawnLocationCount, 1, MAX_TARGETS_PER_QUERY);

	DWORD dwRandomNodeFlags = 0;
	SETBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN );
	
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( tData.pGame, UnitGetRoom(tData.pUnit), dwRandomNodeFlags, fRadius, fHeight);
	if (nNodeIndex == INVALID_INDEX)
	{
		// TODO we should retry here instead of failing immediately
		return FALSE;
	}
	ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( UnitGetRoom(tData.pUnit), nNodeIndex );
	ASSERT_RETFALSE( pNode );

	VECTOR vTarget;
	if (bSpawnInAir)
		vTarget = CalculatePathTargetFly(tData.pGame, MONSTERS_SPAWN_IN_AIR_ALTITUDE, fHeight, UnitGetRoom(tData.pUnit), pNode);
	else
		vTarget = RoomPathNodeGetExactWorldPosition(tData.pGame, UnitGetRoom(tData.pUnit), pNode);
	
	if (nSpawnLocationCount > 1)
	{
		// We're going to fill all requested locations with the same value
		for(int i=0; i<nSpawnLocationCount; i++)
		{
			UnitSetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, i, vTarget);
		}

		// Send locations to client
		MSG_SCMD_SKILLCHANGETARGETLOCMULTI msg;
		msg.id = UnitGetId(tData.pUnit);
		msg.wSkillId = DOWNCAST_INT_TO_WORD(tData.nSkill);
		msg.nLocationCount = nSpawnLocationCount;
		msg.vTarget = vTarget;
		s_SendUnitMessage(tData.pUnit, SCMD_SKILLCHANGETARGETLOCMULTI, &msg);
	}
	else
	{
		// Only one location to set
		UnitSetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0, vTarget);
		if (bSendToClient)
		{
			MSG_SCMD_SKILLCHANGETARGETLOC msg;
			msg.id = UnitGetId(tData.pUnit);
			msg.wSkillId = DOWNCAST_INT_TO_WORD(tData.nSkill);
			msg.nTargetIndex = 0;
			msg.vTarget = vTarget;
			s_SendUnitMessage(tData.pUnit, SCMD_SKILLCHANGETARGETLOC, &msg);
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sStateEventGetTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTarget = tData.pUnit;
	BOOL bCheckIsValid = FALSE;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_AI_TARGET )
	{
		DWORD dwAIGetTargetFlags = 0;
		SETBIT(dwAIGetTargetFlags, AIGTF_DONT_DO_TARGET_QUERY_BIT);
		pTarget = AI_GetTarget( tData.pGame, tData.pUnit, tData.nSkill, NULL, dwAIGetTargetFlags );
		bCheckIsValid = TRUE;
	}

	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
		bCheckIsValid = TRUE;
	}

	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_TARGET_WEAPON )
	{
		pTarget = UnitGetById( tData.pGame, tData.idWeapon );
		bCheckIsValid = FALSE;
	}

	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_DONT_VALIDATE_TARGET )
	{
		bCheckIsValid = FALSE;
	}

	if ( bCheckIsValid && pTarget )
	{
		UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
		BOOL bIsInRange = FALSE;
		if ( ! SkillIsValidTarget( tData.pGame, tData.pUnit, pTarget, pWeapon, tData.nSkill, FALSE, &bIsInRange ) ||
			 ! bIsInRange )
			 pTarget = NULL;
	}
	return pTarget;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetStateClient(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_CLIENT( tData.pGame ) );
	int nDuration = sGetStateDuration( tData );

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pTarget = sStateEventGetTarget( tData );

	if ( pTarget )
		c_StateSet( pTarget, tData.pUnit, nState, 0, nDuration, tData.nSkill, FALSE, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSetState(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	int nDuration = sGetStateDuration( tData );

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pTarget = sStateEventGetTarget( tData );

	UNIT * pTransferUnit = NULL;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_TRANSFER_STATS )
	{
		pTransferUnit = tData.pUnit;
	}

	if ( pTarget )
		s_StateSet( pTarget, tData.pUnit, nState, nDuration, tData.nSkill, pTransferUnit, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );

	return pTarget != NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetStateOnPet(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	ASSERT_RETFALSE( tData.pUnit );

	int nDuration = sGetStateDuration( tData );
	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;



	for ( int i = 0; i < MAX_SUMMONED_INVENTORY_LOCATIONS; i++ )
	{
		if(tData.pSkillData->pnSummonedInventoryLocations[ i ] == INVALID_ID)
		{
			continue;
		}

		UNIT * pPet = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], NULL);
		while(pPet)
		{
			UNIT * pPetNext = UnitInventoryLocationIterate(tData.pUnit, tData.pSkillData->pnSummonedInventoryLocations[ i ], pPet);
			s_StateSet( pPet, tData.pUnit, nState, nDuration, tData.nSkill, NULL, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );

			pPet = pPetNext;
		}
	}


	return TRUE;
}

#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sIncrementState(
	const SKILL_EVENT_FUNCTION_DATA &tData,
	UNIT * pTarget,
	int nState,
	int nStatesInGroup,
	int nDuration )
{
	for ( int i = nState; i < nState + nStatesInGroup; i++ )
	{
		if ( UnitHasExactState( pTarget, i ) )
		{
			if ( i < nState + nStatesInGroup - 1 )
			{ // don't clear the last one
				s_StateClear( pTarget, UnitGetId( tData.pUnit ), i, 0 );
				s_StateSet( pTarget, tData.pUnit, i + 1, nDuration, tData.nSkill );
			} else {
				s_StateSet( pTarget, tData.pUnit, i, nDuration, tData.nSkill );
			}
			return;
		}
	}

	// you didn't have any of the states, set the first one
	s_StateSet( pTarget, tData.pUnit, nState, nDuration, tData.nSkill, NULL, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );

}

#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventIncrementState(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if ( nState == INVALID_ID )
		return FALSE;

	int nStatesInGroup = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );

	int nNumStates = ExcelGetNumRows( tData.pGame, DATATABLE_STATES );

	ASSERT_RETFALSE( nStatesInGroup > 0 );
	ASSERT_RETFALSE( nState + nStatesInGroup <= nNumStates );

	UNIT * pTarget = sStateEventGetTarget( tData );
	if ( ! pTarget )
		return FALSE;

	int nDuration = sGetStateDuration( tData );

	sIncrementState( tData, pTarget, nState, nStatesInGroup, nDuration );

	return TRUE;
}

#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventIncrementStateOnTargetsInRange(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if ( nState == INVALID_ID )
		return FALSE;

	int nStatesInGroup = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
	float fRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );

	int nNumStates = ExcelGetNumRows( tData.pGame, DATATABLE_STATES );

	ASSERT_RETFALSE( nStatesInGroup > 0 );
	ASSERT_RETFALSE( fRange > 0 );
	ASSERT_RETFALSE( nState + nStatesInGroup <= nNumStates );

	UNIT * pTarget = sStateEventGetTarget( tData );
	if ( ! pTarget )
		return FALSE;

	int nDuration = sGetStateDuration( tData );

	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	VECTOR vTargetPosition = UnitGetPosition( pTarget );
	tTargetingQuery.pvLocation  = &vTargetPosition;
	tTargetingQuery.pCenterRoom = UnitGetRoom( pTarget );
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		if ( tTargetingQuery.pnUnitIds[ i ] == UnitGetId( pTarget ) )
			continue;

		UNIT * pVictim = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( pVictim )
			sIncrementState( tData, pVictim, nState, nStatesInGroup, nDuration );
	}

	return TRUE;
}

#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventRemoveTargetAndIncrementState(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	if ( sRemoveTarget( tData, INVALID_ID ) )
		return sSkillEventIncrementState( tData );
	return FALSE;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRemoveTargetAndSetState(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	// remove the corpse.  If you can't then don't get the reward
	if ( ! sRemoveTarget( tData, INVALID_ID ) )
		return FALSE;

	return sSkillEventSetState( tData );
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSetStateOnTargetsInRange(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	float fRange,
	int nDuration,
	UNIT *pTransferUnit = NULL,
	BOOL bDontExecuteSkillStats = FALSE)
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	int nTargetCount = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	nTargetCount = MIN(nTargetCount, MAX_TARGETS_PER_QUERY);
	if(nTargetCount == 0)
	{
		nTargetCount = MAX_TARGETS_PER_QUERY;
	}

	// find all targets within the holy aura
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = nTargetCount;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		{
			STATS * pStats = s_StateSet( tData.pUnit, pTarget, nState, nDuration, tData.nSkill, pTransferUnit, NULL, bDontExecuteSkillStats );
			if ( pStats )
			{
				DWORD dwStateIndex = 0;
				if ( StatsGetStatAny( pStats, STATS_STATE_SOURCE, &dwStateIndex ) )
					s_StateAddTarget( tData.pGame, UnitGetId( pTarget ), tData.pUnit, pTarget, nState, (WORD)dwStateIndex );
			}
		}
		else
		{
			STATS * pStats = s_StateSet( pTarget, tData.pUnit, nState, nDuration, tData.nSkill, pTransferUnit, NULL, bDontExecuteSkillStats );
			if(pStats)
			{
				if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
				{
					VECTOR vTargetLocation = UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
					s_StateAddTargetLocation(tData.pGame, UnitGetId(tData.pUnit), pTarget, vTargetLocation, nState, 0);
				}
			}
		}

	}

	return tTargetingQuery.nUnitIdCount != 0;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSetStateTransferFromTargetToTargetsInArea(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	int nDuration = sGetStateDuration( tData );

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pTransferUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTransferUnit )
		return TRUE;

	//set it on self
	if( SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ) )
	{
		s_StateSet( tData.pUnit, tData.pUnit, nState, nDuration, tData.nSkill, pTransferUnit, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );
	}

	float fRange;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
	{
		fRange = UnitGetHolyRadius(tData.pUnit);
	}
	else
	{
		fRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );
		if( fRange <= 0 )
		{
			fRange = SkillGetRange( tData.pUnit, tData.nSkill, UnitGetById( tData.pGame, tData.idWeapon ) );
		}
	}	
	sSetStateOnTargetsInRange(tData, fRange, nDuration, pTransferUnit, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );
	return TRUE;	
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSetStateTransferFromTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	int nDuration = sGetStateDuration( tData );

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pTransferUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTransferUnit )
		return TRUE;

	s_StateSet( tData.pUnit, tData.pUnit, nState, nDuration, tData.nSkill, pTransferUnit, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS) );

	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventSetStateOnTargetsInRange(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fRange;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
	{
		fRange = UnitGetHolyRadius(tData.pUnit);
	}
	else
	{
		fRange = SkillGetRange( tData.pUnit, tData.nSkill, UnitGetById( tData.pGame, tData.idWeapon ) );
	}
	int nDuration = sGetStateDuration( tData );
	return sSetStateOnTargetsInRange(tData, fRange, nDuration, NULL, (tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_DONT_EXECUTE_STATS));
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventClearState(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERTX_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID, tData.pSkillData->szName );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	//Merged in from Tugboat
	//feel like hellgate should incorporate this
	if( AppIsHellgate() )
	{
		UNIT * pTarget = sStateEventGetTarget( tData );
		if ( pTarget )
		{
			BOOL bForceUltimateOwner = SkillEventGetParamBool( pTarget, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
			pTarget = (bForceUltimateOwner )?UnitGetUltimateOwner( pTarget ):pTarget;
			s_StateClear( pTarget, UnitGetId( tData.pUnit ), tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0 );
		}
	}else{
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		{
			UNIT* pUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
			if( pUnit )
			{
				s_StateClear( pUnit, UnitGetId( pUnit ), tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0 );
			}
		}
		else
		{
			BOOL bForceUltimateOwner = SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
			UNIT *pTarget = (bForceUltimateOwner )?UnitGetUltimateOwner( tData.pUnit ):tData.pUnit;
			s_StateClear( pTarget, UnitGetId( pTarget ), tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0 );
		}
	}
	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventClearStateClient(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERTX_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID, tData.pSkillData->szName );
	ASSERT_RETFALSE( IS_CLIENT( tData.pGame ) );
	UNIT * pTarget = sStateEventGetTarget( tData );
	if ( pTarget )
		c_StateClear( pTarget, UnitGetId( tData.pUnit ), tData.pSkillEvent->tAttachmentDef.nAttachedDefId, 0 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventToggleState(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	UNIT * pTarget = sStateEventGetTarget( tData );

	if ( UnitHasState( tData.pGame, pTarget, nState ) )
	{
		return sSkillEventClearState( tData );
	} else {
		return sSkillEventSetState( tData );
	}
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventStateClearAllOfType(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );
	UNIT * pTarget = sStateEventGetTarget( tData );
	if ( pTarget )
		StateClearAllOfType( pTarget, tData.pSkillEvent->tAttachmentDef.nAttachedDefId );
	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventStateClearAllOfTypeOnTargetsInRange(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pSkillEvent->tAttachmentDef.nAttachedDefId != INVALID_ID );
	ASSERT_RETFALSE( IS_SERVER( tData.pGame ) );

	float fRange;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
	{
		fRange = UnitGetHolyRadius(tData.pUnit);
	}
	else
	{
		fRange = SkillGetRange( tData.pUnit, tData.nSkill, UnitGetById( tData.pGame, tData.idWeapon ) );
	}

	int nState = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;

	// find all targets within the holy aura
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		StateClearAllOfType( pTarget, nState );

	}
	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleDoSkillDelayed(
	GAME* pGame,
	UNIT* pUnit,
	const EVENT_DATA& event_data)
{
	int nSkill = (int)event_data.m_Data1;
	UNITID idTarget = (UNITID)event_data.m_Data2;
	int nSkillSeed = (int)event_data.m_Data3;
	int skillLevel = (int)event_data.m_Data4;

	DWORD dwFlags = 0;
	if ( IS_SERVER( pGame ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
	else
		dwFlags |= SKILL_START_FLAG_DONT_SEND_TO_SERVER;
	if ( SkillCanStart(pGame, pUnit, nSkill, INVALID_ID, UnitGetById( pGame, idTarget ), FALSE, FALSE, 0) )
		SkillStartRequest(pGame, pUnit, nSkill, idTarget, VECTOR(0), dwFlags, nSkillSeed, skillLevel);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoSkillCommon(
	const SKILL_EVENT_FUNCTION_DATA & tData,
	BOOL bDoSkillOnTarget)
{
	int nSkillToDo = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if (nSkillToDo != INVALID_ID)
	{
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
		{
			UnitSetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, nSkillToDo, 0, UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0) );
		}

		UNIT * pTarget = SkillGetTarget(tData.pUnit, nSkillToDo, tData.idWeapon);

		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
			pTarget = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);

		int nDelay = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
		UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );

		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_AT_NEXT_COOLDOWN )
		{
			const SKILL_DATA * pSkillDataToDo = SkillGetData(tData.pGame, nSkillToDo);
			ASSERT_RETFALSE(pSkillDataToDo);
			nDelay = SkillGetCooldown(tData.pGame, tData.pUnit, pWeapon, nSkillToDo, pSkillDataToDo, 0, tData.nSkillLevel);
		}

		DWORD dwSeed = SkillGetSeed(tData.pGame, tData.pUnit, pWeapon, nSkillToDo);

		UNIT * pUnitToDoSkillOn = bDoSkillOnTarget ? pTarget : tData.pUnit;

		if(nDelay > 0)
		{
			if(!UnitHasTimedEvent(pUnitToDoSkillOn, sHandleDoSkillDelayed, CheckEventParam123, EVENT_DATA(nSkillToDo, UnitGetId(pTarget), dwSeed, tData.nSkillLevel)))
			{
				UnitRegisterEventTimed(pUnitToDoSkillOn, sHandleDoSkillDelayed, EVENT_DATA(nSkillToDo, UnitGetId(pTarget), dwSeed, tData.nSkillLevel), nDelay);
			}
		}
		else if(SkillCanStart(tData.pGame, tData.pUnit, nSkillToDo, tData.idWeapon, pTarget, FALSE, FALSE, 0))
		{
			if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_DO_WHEN_TARGET_IN_RANGE )
			{
				float fRange = SkillGetRange( tData.pUnit, nSkillToDo, pWeapon );
				if ( !pTarget || !UnitsAreInRange( tData.pUnit, pTarget, 0.0f, fRange, NULL ) )
				{// register this event and to check again in a tick or two
					SkillRegisterEvent( tData.pGame, tData.pUnit, tData.pSkillEvent, tData.nSkill, tData.nSkillLevel, tData.idWeapon, GAME_TICK_TIME, 0.0f );
					return TRUE;
				}
			}

			DWORD dwFlags = SKILL_START_FLAG_DONT_SEND_TO_SERVER;
			if ( IS_SERVER( tData.pGame ) )
				dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest(tData.pGame, pUnitToDoSkillOn, nSkillToDo, UnitGetId(pTarget), VECTOR(0), dwFlags, dwSeed, tData.nSkillLevel);
			return TRUE;

		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sSkillEventDoSkillOnTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	return sSkillEventDoSkillCommon(tData, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoSkill(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	return sSkillEventDoSkillCommon(tData, FALSE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStopSkill(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{

	int nSkillToDo = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	if (nSkillToDo != INVALID_ID)
	{
		if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		{
			return SkillStop( tData.pGame, SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon ), nSkillToDo, INVALID_ID );
		}
		else
		{
			return SkillStop( tData.pGame, tData.pUnit, nSkillToDo, INVALID_ID );
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStopAllSkills(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{

	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		SkillStopAll( tData.pGame, SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon ) );
		return TRUE;
	}
	else
	{
		SkillStopAll( tData.pGame, tData.pUnit );
		return TRUE;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventDoSkillToTargets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fParamRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nMaxTargets = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );

	float fRange;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
	{
		fRange = UnitGetHolyRadius(tData.pUnit);
	}
	else
	{
		ASSERT_RETFALSE(fParamRange > 0.0f);
		fRange = fParamRange;
	}

	VECTOR vTargetLocation( 0 );
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = nMaxTargets ? nMaxTargets : MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
		if ( ! pTarget )
			return FALSE;
		vTargetLocation = UnitGetPosition( pTarget );
		tTargetingQuery.pvLocation = &vTargetLocation;
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
		tTargetingQuery.fMaxRange += UnitGetCollisionRadius( pTarget );
	}
	else if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
	{
		vTargetLocation = UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0);
		tTargetingQuery.pvLocation = &vTargetLocation;
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	}

	SkillTargetQuery( tData.pGame, tTargetingQuery );

	int nIndex = ExcelGetLineByStringIndex(tData.pGame, DATATABLE_SKILLS, tData.pSkillEvent->tAttachmentDef.pszAttached);
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_SKILL_TARGET_LOCATION )
	{
		UnitSetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, 0, vTargetLocation );
	}
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
		if (pTarget && UnitIsA(pTarget, UNITTYPE_CREATURE ) )
		{
			DWORD dwSeed = SkillGetSeed(tData.pGame, tData.pUnit, UnitGetById(tData.pGame, tData.idWeapon), nIndex);
			SkillSetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, UnitGetId(pTarget) );

			DWORD dwFlags = SKILL_START_FLAG_DONT_SEND_TO_SERVER;
			if ( IS_SERVER( tData.pGame ) )
				dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_IGNORE_COOLDOWN
					| SKILL_START_FLAG_IGNORE_POWER_COST | SKILL_START_FLAG_DONT_SET_COOLDOWN;

			SkillStartRequest( tData.pGame, tData.pUnit, nIndex, tTargetingQuery.pnUnitIds[ i ], VECTOR(0), dwFlags, dwSeed, tData.nSkillLevel);
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAttackTargetsInRange(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fParamRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	int nMaxTargets = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 2 );

	BOOL bUseBothWeaponsAtOnce = FALSE;
	ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	bUseBothWeaponsAtOnce = SkillGetWeaponsForAttack( tData, pWeapons );

	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];

	float fRange = 0.0f;
	{
		if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
		{
			fRange = UnitGetHolyRadius(tData.pUnit);
		}
		else
		{
			fRange = fParamRange;
		}
		if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_IS_MELEE ) )
		{
			SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE );
			fRange = 0.0f;
		}
		if ( fRange == 0.0f )
		{
			fRange = SkillGetRange( tData.pUnit, tData.nSkill, pWeapons[ 0 ] );
			if ( pWeapons[ 0 ] && pWeapons[ 1 ] )
				fRange = (fRange + SkillGetRange( tData.pUnit, tData.nSkill, pWeapons[ 1 ] )) / 2.0f;
		}
		ASSERT_RETFALSE(fRange != 0.0f);
	}

	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_UNIT_RADIUS );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
	if ( IS_CLIENT( tData.pGame ) )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_ON_START );
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_DYING_AFTER_START );
	}

	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = nMaxTargets == 0 ? MAX_TARGETS_PER_QUERY : nMaxTargets;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	// TRAVIS: Still performed an attack - return TRUE so we can charge power for this!
	// Nothing else seems to rely on the true/false
	if ( !tTargetingQuery.nUnitIdCount )
		return TRUE;

	DWORD dwUnitAttackUnitFlags = 0;
	SETBIT(dwUnitAttackUnitFlags, UAU_RADIAL_ONLY_ATTACKS_DEFENDER_BIT);
	SETBIT(dwUnitAttackUnitFlags, UAU_DIRECT_MISSILE_BIT);

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );

		if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_IS_MELEE ) )
		{ // we should fully implement combining weapon damage on melee attacks... maybe...
			float fCombinationMultiplier = tData.pSkillData->fDamageMultiplier;
			// if we're averaging combined damage, we really just need to lop them both in half
			if( pWeapons[0] && pWeapons[1] && 
				SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_AVERAGE_COMBINED_DAMAGE ) )
			{
				if( fCombinationMultiplier == -1 )
				{
					fCombinationMultiplier = .5f;
				}
				else
				{
					fCombinationMultiplier *= .5f;
				}
			}

			UnitMeleeAttackUnit( tData.pUnit, pTarget, pWeapons[ 0 ], NULL, tData.nSkill, 0, tTargetingQuery.fMaxRange,
				FALSE, tData.pSkillEvent, fCombinationMultiplier, tData.pSkillData->nDamageTypeOverride, MELEE_IMPACT_FORCE, dwUnitAttackUnitFlags );
			if ( pWeapons[ 1 ] )
				UnitMeleeAttackUnit(tData.pUnit, pTarget, pWeapons[ 1 ], pWeapons[0], tData.nSkill, 0, tTargetingQuery.fMaxRange,
				FALSE, tData.pSkillEvent, fCombinationMultiplier, tData.pSkillData->nDamageTypeOverride, MELEE_IMPACT_FORCE, dwUnitAttackUnitFlags );
		}
		else if ( IS_SERVER( tData.pGame ) )
		{
#if !ISVERSION(CLIENT_ONLY_VERSION)
#ifdef HAVOK_ENABLED
			HAVOK_IMPACT tImpact;
			if( AppIsHellgate() )
			{
				HavokImpactInit( tData.pGame, tImpact, MELEE_IMPACT_FORCE, UnitGetPosition( tData.pUnit ), NULL );
			}
#endif
			sDoAttackForMeleeAttack( tData,
				pTarget,
				pWeapons,
				bUseBothWeaponsAtOnce,
#ifdef HAVOK_ENABLED
				tImpact,
#endif
				dwUnitAttackUnitFlags);

#endif // !ISVERSION(CLIENT_ONLY_VERSION)
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventModifyAI(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	AI_BEHAVIOR_DEFINITION_TABLE * pTableDef = (AI_BEHAVIOR_DEFINITION_TABLE *) DefinitionGetByFilename( DEFINITION_GROUP_AI, tData.pSkillEvent->tAttachmentDef.pszAttached );
	if (!pTableDef)
	{
		return FALSE;
	}

	float fTimeoutSeconds = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nTimeout = (int)(fTimeoutSeconds * GAME_TICKS_PER_SECOND_FLOAT);
	BOOL bHasAI = UnitGetStat(tData.pUnit, STATS_AI);
	if (bHasAI)
	{
		AI_InsertTable(tData.pUnit, pTableDef, NULL, nTimeout);
	}

	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventAngerOthers(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	//Chance of Occurance - this sets an AI stat. Random is ok here.
	int chanceOfOccurance = MAX( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 0 );
	chanceOfOccurance = ( chanceOfOccurance == 0 ) ? 100 : chanceOfOccurance;
	int range = MAX( SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ), 0 );

	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	UNITID idTarget = UnitGetAITargetId(tData.pUnit);
	if ( ( AppIsTugboat() &&
		  ( ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET ) && pTarget == NULL ) &&
		 	idTarget == INVALID_ID &&
		 	range == 0 ) ||
		 ( AppIsHellgate() &&			//This could be merged but it might break hellgate if use unit target is accidently checked
		   idTarget == INVALID_ID  ) )
	{
		return FALSE;
	}

	if ( AppIsHellgate() ||
		 ( AppIsTugboat() &&
		  ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET ) == 0 ) )
	{
		UNIT * pWeapon    = UnitGetById( tData.pGame, tData.idWeapon    );
		float fRangeMax = ( range == 0 )?SkillGetRange( tData.pUnit, tData.nSkill, pWeapon ):range;
		ASSERT_RETFALSE(fRangeMax != 0.0f);

		// UNIT * pSkillTarget = UnitGetById(tData.pGame, UnitGetStat(tData.pUnit, STATS_AI_LAST_ATTACKER_ID));

		UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
		SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
		tTargetingQuery.fMaxRange	= fRangeMax;
		tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
		tTargetingQuery.pnUnitIds   = pnUnitIds;
		tTargetingQuery.pSeekerUnit = tData.pUnit;
		SkillTargetQuery( tData.pGame, tTargetingQuery );
		if( AppIsHellgate() )
		{
			for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
			{
				UNIT * pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
				if (pTarget && UnitIsA(pTarget, UNITTYPE_MONSTER))
				{
					UnitSetAITarget(pTarget, idTarget);
				}
			}
		}
		else
		{
			for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
			{
				pTarget = UnitGetById( tData.pGame, tTargetingQuery.pnUnitIds[ i ] );
				if (pTarget && UnitIsA(pTarget, UNITTYPE_CREATURE))
				{
					if( (int)(RandGetFloat( tData.pGame ) * 100.0f ) <= chanceOfOccurance )
					{
						if( range )
						{
							UNITID idUnit = UnitGetId( tData.pUnit );
							UnitSetAITarget(pTarget, idUnit);
							UnitSetAITarget(pTarget, idUnit, TRUE);
							s_StateSet( pTarget, pTarget, STATE_TAUNT_CREATURE_AI, 100, -1 );
						}
						else
						{
							UnitSetAITarget(pTarget, idTarget);
							UnitSetAITarget(pTarget, idTarget, TRUE);
							s_StateSet( pTarget, pTarget, STATE_TAUNT_CREATURE_AI, 100, -1 );
						}
					}
				}
			}
		}
	}
	else
	{
		if( (int)(RandGetFloat( tData.pGame ) * 100.0f ) <= chanceOfOccurance )
		{
			UNITID idUnit = UnitGetId( tData.pUnit );
			UnitSetAITarget(pTarget, idUnit);
			UnitSetAITarget(pTarget, idUnit, TRUE);
			s_StateSet( pTarget, pTarget, STATE_TAUNT_CREATURE_AI, 100, -1 );
		}
	}

	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventAwakenEnemies(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTargetForEnemies = UnitGetUltimateOwner( tData.pUnit );
	if ( ! pTargetForEnemies )
		return FALSE;
	float fRange = MAX( SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 0.0f );

	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pEnemy = UnitGetById(tData.pGame, pnUnitIds[i]);
		if ( pEnemy )
			UnitSetAITarget( pEnemy, UnitGetId( pTargetForEnemies ) );
	}
	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventFullyHeal(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pUnitToHeal = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToHeal = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnitToHeal)
		return TRUE;

	s_UnitRestoreVitals( pUnitToHeal, TRUE );

	return TRUE;
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventHealPartially(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pUnitToHeal = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToHeal = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	else if ( tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_USE_ULTIMATE_OWNER )
		pUnitToHeal = UnitGetUltimateOwner( tData.pUnit );

	if ( !pUnitToHeal)
		return TRUE;

	int hp_max = UnitGetStat(pUnitToHeal, STATS_HP_MAX);

	int nPercentOfMax = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	int nTime = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	nPercentOfMax = PIN(nPercentOfMax, 0, 100);
	REF(nTime);

	// Time is currently not implemented

	int hp_cur = UnitGetStat(pUnitToHeal, STATS_HP_CUR);
	hp_cur += hp_max * nPercentOfMax / 100;
	hp_cur = MIN(hp_max, hp_cur);
	UnitSetStat(pUnitToHeal, STATS_HP_CUR, hp_cur);
	return TRUE;
#else
	return FALSE;
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventRecall(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT( IS_SERVER(tData.pGame) );
	BOOL bForce = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ) != 0;
	BOOL bPrimaryOnly = SkillEventGetParamInt( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ) != 0;

	if (AppIsTugboat())
	{
		DWORD dwLevelRecallFlags = 0;
		SETBIT( dwLevelRecallFlags, LRF_FORCE, bForce );
		SETBIT( dwLevelRecallFlags, LRF_PRIMARY_ONLY, bPrimaryOnly );
		s_LevelRecall( tData.pGame, tData.pUnit, dwLevelRecallFlags );
	}
	else
	{
		// open a recall portal entrance
		s_WarpOpen( tData.pUnit, WARP_TYPE_RECALL );
	}

	return TRUE;

}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float SkillGetLeapDuration(
	UNIT * pUnit,
	int nSkill,
	UNITMODE eMode )
{
	float fVelocity = UnitGetVelocityForMode( pUnit, eMode );
	VECTOR vTarget = UnitGetStatVector( pUnit, STATS_SKILL_TARGET_X, nSkill, 0 );
	if ( fVelocity > 0.0f && ! VectorIsZero( vTarget ) )
	{
		float fDistance = UnitsGetDistance( pUnit, vTarget );
		return fDistance / fVelocity;
	}
	return 0.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLeapLandOnGround(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherUnit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId)
{
	DWORD dwIdCollideEvent = (DWORD)pEventData->m_Data1;

	// set a state for landed on ground
	float flRecoveryDurationInSeconds = 1.0f;
	int nRecoveryDurationInTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * flRecoveryDurationInSeconds );

	int nPauseDurationInTicks = (int)pEventData->m_Data2;
	if (IS_SERVER( pGame))
	{
		s_StateSet( pUnit, pUnit, STATE_LANDED, 1 );
		if ( nPauseDurationInTicks > 0 )
			s_StateSet( pUnit, pUnit, STATE_DONT_RUN_AI, nPauseDurationInTicks );
		s_UnitSetMode( pUnit, MODE_LEAP_LANDED, 0, flRecoveryDurationInSeconds, 0, FALSE );
	}
	else
	{
		c_StateSet( pUnit, pUnit, STATE_LANDED, 0, 1, INVALID_LINK );
		c_UnitSetMode( pUnit, MODE_LEAP_LANDED, 0, flRecoveryDurationInSeconds );
	}

	// remove this event handler
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_LANDED_ON_GROUND, dwId );

	// stop moving
	UnitSetVelocity( pUnit, 0.0f );

	// we will treat this like a warp
	PathingUnitWarped( pUnit, nRecoveryDurationInTicks );

	// remove the collided event handler
	BOOL bCollided = !UnitUnregisterEventHandler( pGame, pUnit, EVENT_DIDCOLLIDE, dwIdCollideEvent );
	if (bCollided == FALSE)
	{
		// we didn't hit anything going through the air, maybe do an attack
		// around us or something else that could possibly hurt things
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLeapCollide(
	GAME *pGame,
	UNIT *pUnit,
	UNIT *pOtherUnit,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD dwId)
{

#if !ISVERSION(CLIENT_ONLY_VERSION)
	// attack them
	if (IS_SERVER( pGame ) && pOtherUnit )
	{
		DWORD dwFlags = 0;
		SETBIT(dwFlags, UAU_MELEE_BIT);
		s_UnitAttackUnit( pUnit, pOtherUnit, NULL, 0, 
#ifdef HAVOK_ENABLED
			NULL, 
#endif
			dwFlags );
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	// remove this event handler
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_DIDCOLLIDE, dwId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStartLeap(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	GAME *pGame = tData.pGame;
	UNIT *pUnit = tData.pUnit;
	int nSkill = tData.nSkill;

//	UnitPositionTrace( pUnit, "Leap Start" );

	// do nothing if in the air already
	if (UnitIsJumping( pUnit ))
	{
		return TRUE;
	}

	// register unit to unit collision event callback
	DWORD dwIdCollideEvent;
	UnitRegisterEventHandlerRet( dwIdCollideEvent, pGame, pUnit, EVENT_DIDCOLLIDE, sLeapCollide, NULL );

	float fAnimSpeed = EventParamToFloat( tData.pParam );
	float fVelocity = UnitGetVelocityForMode( pUnit, MODE_LEAP );
	VECTOR vSkillTarget = UnitGetStatVector( pUnit, STATS_SKILL_TARGET_X, nSkill, 0 );

	if (fVelocity > 0.0f &&
		!VectorIsZero( vSkillTarget ) &&
		vSkillTarget != pUnit->vPosition)
	{
		if(IS_CLIENT(pUnit))
		{
			if(UnitPathIsActiveClient(pUnit))
			{
				UnitPathAbort(pUnit);
			}
		}
		VECTOR vMoveDirection = vSkillTarget - tData.pUnit->vPosition;
		VectorNormalize( vMoveDirection );
		//float fDistance = VectorLength( vMoveDirection );
		//vMoveDirection /= fDistance;

		// figure out our destination ... we are going to jump a little bit beyond the
		// destination so that it looks like we're aiming for their "head"
		VECTOR vDestination;
		VectorAdd( vDestination, vSkillTarget, vMoveDirection * 10.0f );
		UnitSetMoveTarget( pUnit, vDestination, vMoveDirection );

		float fStopDistance = -UnitGetCollisionRadius( pUnit );
		UnitSetStatFloat( pUnit, STATS_STOP_DISTANCE, 0, fStopDistance );
		UnitStepListAdd( pGame, pUnit );

		// since we're going to jump, clear where we were standing
		UnitClearBothPathNodes( pUnit );

		// sometimes we want to pause after we land
		const SKILL_EVENT_PARAM * pParam = SkillEventGetParam( tData.pSkillEvent, 0 );
		int nPauseDurationInTicks = pParam ? pParam->nValue : 0;

		// register an event for when we land on the ground again
		EVENT_DATA tEventData;
		tEventData.m_Data1 = dwIdCollideEvent;
		tEventData.m_Data2 = nPauseDurationInTicks;
		UnitRegisterEventHandler( pGame, pUnit, EVENT_LANDED_ON_GROUND, sLeapLandOnGround, &tEventData );

		//float fDeltaZ = vSkillTarget.fZ - pUnit->vPosition.fZ;
		//float fLength;
		//fDistance -= pUnit->fCollisionCylinderRadius + fStopDistance;
		//if ( ( fabsf( fDeltaZ ) < 1.0f ) || fDeltaZ >= 0.0f )
		//	// going up
		//	fLength = ( fDistance / 2.0f ) / ( fVelocity * GAME_TICK_TIME );
		//else
		//	// going down
		//	fLength = ( ( fDistance / 2.0f ) + fDeltaZ ) / ( fVelocity * GAME_TICK_TIME );

		//float fDeltaZ = vSkillTarget.fZ - pUnit->vPosition.fZ;
		//ASSERT( fabsf( fDeltaZ ) <= 1.0f );
		VECTOR vDelta = vSkillTarget - pUnit->vPosition;
		vDelta.fZ = 0.0f;
		float fXYDistance = VectorLength( vDelta ) - (UnitGetCollisionRadius( pUnit ) + fStopDistance);
		float fLength = (fXYDistance * 0.5f) / (fVelocity * GAME_TICK_TIME);
		float fZVelocity = GRAVITY_ACCELERATION * GAME_TICK_TIME * fLength;
		UnitStartJump( pGame, pUnit, fZVelocity, FALSE );

		if ( IS_CLIENT( pUnit ) )
		{
			c_UnitSetLastKnownGoodPosition(pUnit, vDestination, TRUE);

			int nAppearance = c_UnitGetAppearanceId( pUnit );
			int nMode = SkillGetMode( pGame, pUnit, nSkill, INVALID_ID );
			c_AppearanceSetAnimationSpeed( nAppearance, nMode, fAnimSpeed );
		}

	}
	else
	{
		sSkillEventSafeStopSkill( tData );
		s_UnitSetMode(pUnit, MODE_IDLE, 0, 0.0f, 0, TRUE);
	}

	return TRUE;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleEndWeaponMode(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	if ( pTarget )
	{
		UNITMODE eMode = (UNITMODE) pHandlerData->m_Data1;
		UnitEndMode( pTarget, eMode );
	}
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_SKILL_STOPPED, dwId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStartModeWeapon(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT( tData.pGame ) );

	UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
	if ( ! pWeapon )
		return FALSE;

	BOOL bHasMode;
	UNITMODE eMode = (UNITMODE) tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	float fDuration = UnitGetModeDuration( tData.pGame, pWeapon, eMode, bHasMode );
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_LOOP )
	{
		fDuration = 0.0f;
		UnitRegisterEventHandler( tData.pGame, tData.pUnit, EVENT_SKILL_STOPPED, sHandleEndWeaponMode, EVENT_DATA( eMode ) );
	}
	//WARNX( bHasMode, "Item missing mode for skill" );
	c_UnitSetMode( pWeapon, eMode, 0, fDuration );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventMoveTargetInInventory(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(tData.pGame && tData.pUnit);

	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	ASSERT_RETFALSE( UnitIsContainedBy( pTarget, tData.pUnit ) );

	const UNIT_DATA * pTargetUnitData = UnitGetData( pTarget );
	ASSERT_RETFALSE(pTargetUnitData);

	BOOL bSkillTablet = UnitIsA(pTarget, UNITTYPE_SKILL_TABLET);
	if ( bSkillTablet )
	{	// check to see if you can learn this skill
		int nSkill = SkillItemGetSkill( pTarget );
		if ( ( AppIsTugboat() && !UnitCanLearnSkill( tData.pUnit, nSkill ) ) ||
			 ( AppIsHellgate() &&
			   (SkillGetNextLevelFlags( tData.pUnit, nSkill ) & SKILLLEARN_BIT_CAN_BUY_NEXT_LEVEL) != 0 &&
			   !GameGetDebugFlag(tData.pGame, DEBUGFLAG_ALLOW_CHEATS) ) )
		{
			return FALSE;
		}
	}


	if( AppIsTugboat() && UnitIsA( pTarget, UNITTYPE_SPELLSCROLL ) )
	{
		int nSkillID = UnitGetSkillID( pTarget );
		const SKILL_DATA * pSkillData = SkillGetData( tData.pGame, nSkillID );
		if( pSkillData->pnSkillGroup[ 0 ] > 0 )
		{
			int Slot = PlayerGetNextFreeSpellSlot( tData.pGame, tData.pUnit, pSkillData->pnSkillGroup[ 0 ], nSkillID );
			UnitSetStat( tData.pUnit, STATS_SPELL_SLOT, nSkillID, Slot );
		}


		int skLevel = UnitGetStat(tData.pUnit, STATS_SKILL_LEVEL, nSkillID);
		if( skLevel > 0 )
			return FALSE;
		int level = 1;

		if (tData.pUnit)
		{
			SkillUnitSetInitialStats(tData.pUnit, nSkillID, level);
			UnitSetStat(tData.pUnit, STATS_SKILL_LEVEL, nSkillID, level);
		}
		// Automatically try to select a new skill after you learn it
		if ( bSkillTablet )
		{
			int nSkillID = UnitGetSkillID( pTarget );
			if (nSkillID != INVALID_ID &&
				SkillTestFlag( tData.pGame, pTarget, SKILL_FLAG_CAN_BE_SELECTED ))
			{
				// this gets handled by the stat change callback
				//SkillSelect( tData.pGame, tData.pUnit, nSkillID );
			}
			else if ( nSkillID != INVALID_ID && AppIsTugboat() )
			{
 				const SKILL_DATA * pSkillData = SkillGetData( tData.pGame, nSkillID );
				if( !SkillDataTestFlag( pSkillData, SKILL_FLAG_START_ON_SELECT ) &&
					SkillDataTestFlag( pSkillData, SKILL_FLAG_ALLOW_IN_MOUSE ) )
				{
					HotkeySet( tData.pGame, tData.pUnit, 1, nSkillID );
				}
			}
		}
		// and it gets used up.
		UnitFree( pTarget, UFF_SEND_TO_CLIENTS );
		return TRUE;
	}
	int nInventoryLocation = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	ASSERT_RETFALSE( nInventoryLocation != INVALID_ID );

	int x, y;
	if (!UnitInventoryFindSpace( tData.pUnit, pTarget, nInventoryLocation, &x, &y))
	{
		return FALSE;
	}
	if( AppIsTugboat() && UnitGetUltimateContainer( pTarget ) == tData.pUnit )
	{
		InventoryChangeLocation( tData.pUnit, pTarget, nInventoryLocation, x, y, 0 );
	}
	else
	{
		if (!UnitInventoryRemove(pTarget, ITEM_ALL))
		{
			return FALSE;
		}
		ASSERT_RETFALSE(UnitInventoryAdd(INV_CMD_PICKUP_NOREQ, tData.pUnit, pTarget, nInventoryLocation));
	}





	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventMoveTargetIntoInventory(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(tData.pGame && tData.pUnit);
	ASSERT_RETFALSE(IS_SERVER( tData.pGame ));

	UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTarget )
		return FALSE;

	ASSERT_RETFALSE( ! UnitIsContainedBy( pTarget, tData.pUnit ) && ! UnitGetContainer( pTarget ) );

	int nInventoryLocation = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	ASSERT_RETFALSE( nInventoryLocation != INVALID_ID );

	ASSERT_RETFALSE(InventoryChangeLocation(tData.pUnit, pTarget, nInventoryLocation, 0, 0, CLF_ALLOW_NEW_CONTAINER_BIT));

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventMoveTargetOutOfInventory(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(tData.pGame && tData.pUnit);
	ASSERT_RETFALSE(IS_SERVER( tData.pGame ));

	int nInventoryLocation = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	ASSERT_RETFALSE( nInventoryLocation != INVALID_ID );

	UNIT * pTarget = UnitInventoryGetByLocation( tData.pUnit, nInventoryLocation );
	if ( ! pTarget )
		return FALSE;

	if ( ! UnitIsContainedBy( pTarget, tData.pUnit ) )
		return FALSE;

	UnitInventoryRemove( pTarget, ITEM_ALL );

	GAMECLIENT *pClient = UnitGetClient( tData.pUnit );
	if ( pClient )
	{
		MSG_SCMD_INVENTORY_REMOVE_PET msg;
		msg.idOwner = UnitGetId( tData.pUnit );
		msg.idPet = UnitGetId( pTarget );
		s_SendMessageToAllWithRoom( tData.pGame, UnitGetRoom( pTarget ), SCMD_INVENTORY_REMOVE_PET, &msg );
	}


	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAwardLuck(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERTX_RETFALSE( tData.pUnit, "No Unit" );
	/*if( tData.pUnit->pUnitData->nLuckChanceBonus > 0 &&
		RandGetNum( tData.pGame,0, 100 ) <= (DWORD)tData.pUnit->pUnitData->nLuckChanceBonus )
	{
		int nLuckValue = UnitGetLuckToAward( tData.pUnit );
		UNIT * pTarget = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
		if( pTarget )
		{
			UnitChangeStat(pTarget, STATS_LUCK_CUR, UnitGetStat( pTarget, STATS_LUCK_CUR ) + nLuckValue );
			s_StateSet( tData.pUnit, pTarget, STATE_AWARDLUCK, 5 );
		}
	}*/
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventTransferStatsToTargets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);

	struct STATS * pStatsList = SkillCreateSkillEventStats(tData.pGame, tData.pUnit, tData.nSkill, tData.pSkillData, nSkillLevel );
	if (!pStatsList)
	{
		return FALSE;
	}

	float fParamRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float fRange;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
	{
		fRange = UnitGetHolyRadius(tData.pUnit);
	}
	else
	{
		fRange = fParamRange;
		if(fRange <= 0.0f)
		{
			fRange = SkillGetRange( tData.pUnit, tData.nSkill, UnitGetById( tData.pGame, tData.idWeapon ) );
		}
	}
	ASSERT_RETFALSE(fRange > 0.0f);

	// find all targets within the holy aura
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	int nTargets = (int)SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 );
	for ( int i = 0; i < tTargetingQuery.nUnitIdCount && (nTargets == 0 || i < nTargets); i++ )
	{
		StatsTransferAdd(tData.pGame, UnitGetById(tData.pGame, pnUnitIds[i]), pStatsList);
	}

	StatsListFree(tData.pGame, pStatsList);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventTransferStatsFromTarget(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pTransferUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if ( ! pTransferUnit )
		return FALSE;

	STATS * pStatsList = StatsListInit(tData.pGame);
	ASSERT_RETFALSE(pStatsList);

	// Transfer the stats from the unit to the statslist
	StatsTransferSet(tData.pGame, pStatsList, pTransferUnit);
	// Add the stats to the player
	StatsTransferAdd(tData.pGame, tData.pUnit, pStatsList);
	// Free our temporary buffer statslist
	StatsListFree(tData.pGame, pStatsList);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sApplyStatsToUnit(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pTarget,
	int nSkill,
	int nSkillLevel,
	BYTE * code_ptr,
	int code_len)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pUnit);
	ASSERT_RETURN(pTarget);
	ASSERT_RETURN(code_ptr);
	ASSERT_RETURN(code_len > 0);

	const SKILL_DATA * pSkillData = SkillGetData(pGame, nSkill);
	ASSERT_RETURN(pSkillData);

	struct STATS * pStatsList = NULL;
	pStatsList = StatsListInit(pGame);
	VMExecI(pGame, pUnit, pTarget, pStatsList, 0, nSkillLevel, nSkill, nSkillLevel, STATE_NONE, code_ptr, code_len);
	StatsListFree(pGame, pStatsList);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventApplyStatsToTargets(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	float fParamRange = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	float fRange;
	if(tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_HOLY_RADIUS_FOR_RANGE)
	{
		fRange = UnitGetHolyRadius(tData.pUnit);
	}
	else
	{
		fRange = fParamRange;
		if(fRange <= 0.0f)
		{
			fRange = SkillGetRange( tData.pUnit, tData.nSkill, UnitGetById( tData.pGame, tData.idWeapon ) );
		}
	}
	ASSERT_RETFALSE(fRange > 0.0f);

	// find all targets within the holy aura
	UNITID pnUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	SkillTargetQuery( tData.pGame, tTargetingQuery );

	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(tData.pGame, DATATABLE_SKILLS, tData.pSkillData->codeStatsScriptOnTarget, &code_len);
	if (code_ptr)
	{
		for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
		{
			sApplyStatsToUnit(tData.pGame, tData.pUnit, UnitGetById(tData.pGame, pnUnitIds[i]), tData.nSkill, nSkillLevel, code_ptr, code_len);
		}
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventApplyStatsToSelf(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UNIT * pUnitToRunScript = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
		pUnitToRunScript = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );

	if ( !pUnitToRunScript)
		return TRUE;

	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(tData.pGame, DATATABLE_SKILLS, tData.pSkillData->codeStatsScriptOnTarget, &code_len);
	sApplyStatsToUnit(tData.pGame, pUnitToRunScript, tData.pUnit, tData.nSkill, nSkillLevel, code_ptr, code_len);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRunScript(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( tData.pUnit );
	STATS *stats( NULL );

	int nSkillLevel = sSkillEventDataGetSkillLevel(tData);
	//lets grab the stats for this skill....
	if( tData.nSkill != INVALID_ID )
	{
		const SKILL_DATA *pSkillData = SkillGetData( tData.pGame, tData.nSkill );
		int nStateOnSelect = pSkillData->nStateOnSelect;
		if( nStateOnSelect != INVALID_ID )
		{
			//why clear this? Because the state on select is really only useful for telling if the player is doing a skill
			//or when doing a passive skill such as heal. BUT, since you would only ever use this script for setting
			//stats on the player via the visual why do it via the state. Just move it.
			// (We use nStateOnSelect for masteries and semi-passive skills - not for when you are "doing a skill."  - Tyler
			s_StateClear( tData.pUnit, UnitGetId( tData.pUnit ), nStateOnSelect, 0 );
			int ticks = SkillDataGetStateDuration( tData.pGame, tData.pUnit, tData.nSkill, nSkillLevel, true );
			stats = s_StateSet( tData.pUnit, NULL, nStateOnSelect, ticks, tData.nSkill );
		}

	}

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(tData.pGame, DATATABLE_SKILLS, tData.pSkillData->codeStatsScriptFromEvents, &code_len);
	VMExecI( tData.pGame,
			 tData.pUnit,
			 SkillGetTarget( tData.pUnit, tData.nSkill, INVALID_ID ),
			 stats,
			 tData.nSkill,
			 INVALID_LINK,
			 tData.nSkill,
			 nSkillLevel,
			 INVALID_LINK,
			 code_ptr,
			 code_len);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventIdentify(
	const SKILL_EVENT_FUNCTION_DATA & tData)
{

	// tell client to go into identify mode
//	MSG_SCMD_ENTER_IDENTIFY tMessage;
//	tMessage.idAnalyzer = something;

	return TRUE;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventCreateGuild(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(tData.pUnit);

	UNIT *pItem = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
	ASSERT_RETFALSE(pItem);

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventSetMeleeMoveMode(
	const SKILL_EVENT_FUNCTION_DATA & tData)
{
	if ( ! SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_MOVES_UNIT ) )
		return FALSE;

	UNITMODE eModeToSet = MODE_MELEE_MOVE;

	float fSkillDuration = SkillGetDuration( tData.pGame, tData.pUnit, tData.nSkill, FALSE, UnitGetById( tData.pGame, tData.idWeapon ) );
	ASSERT_RETFALSE( fSkillDuration != 0.0f );

	int nAppearanceDef = UnitGetAppearanceDefId( tData.pUnit, TRUE );

	UNITMODE eSkillMode = (UNITMODE) SkillGetMode( tData.pGame, tData.pUnit, tData.nSkill, tData.idWeapon );
	float fContactPoint = SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
	fContactPoint = AppearanceDefinitionGetContactPoint( nAppearanceDef, UnitGetAnimationGroup( tData.pUnit ), eSkillMode, fContactPoint );

	ASSERT_RETFALSE( fContactPoint <= 1.0f );
	ASSERT_RETFALSE( tData.pSkillEvent->fTime < fContactPoint );

	float fDuration = fSkillDuration * (fContactPoint - tData.pSkillEvent->fTime);
	ASSERT_RETFALSE( fDuration );

	if ( UnitTestMode( tData.pUnit, eModeToSet ) )
		UnitEndMode( tData.pUnit, eModeToSet );

	if ( IS_SERVER( tData.pGame ) )
		s_UnitSetMode( tData.pUnit, eModeToSet, 0, fDuration, 0, FALSE );
	else
		c_UnitSetMode( tData.pUnit, eModeToSet, 0, fDuration );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSkillEventTownPortal(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE( IS_SERVER(tData.pGame) );
	return s_TownPortalOpen( tData.pUnit ) != NULL;
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStartCoolingAndPowerCost(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		UNIT * pTarget = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon, 0 );
		BOOL bIsInRange = FALSE;
		UNIT * pWeapon = UnitGetById( tData.pGame, tData.idWeapon );
		if ( ! pTarget ||
			! SkillIsValidTarget( tData.pGame, tData.pUnit, pTarget, pWeapon, tData.nSkill, FALSE, &bIsInRange, FALSE, FALSE ) ||
			! bIsInRange )
		{
			return FALSE;
		}
	}
	if(!SkillTakePowerCost(tData.pUnit, tData.nSkill, tData.pSkillData))
	{
		sSkillEventSafeStopSkill( tData, TRUE);
		return FALSE;
	}

	if (!SkillEventGetParamBool( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ))
	{
		SkillStartCooldown(tData.pGame, tData.pUnit, UnitGetById(tData.pGame, tData.idWeapon), tData.nSkill, tData.nSkillLevel, tData.pSkillData, 0, FALSE);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventShowPetInventory(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE( IS_CLIENT(tData.pGame) );
	if ( tData.pUnit != GameGetControlUnit(tData.pGame) )
		return FALSE;
	int nInvLoc = tData.pSkillEvent->tAttachmentDef.nAttachedDefId;
	ASSERT_RETFALSE( nInvLoc != INVALID_ID );
	UNIT * pPet = UnitInventoryGetByLocation( tData.pUnit, nInvLoc );
	if ( ! pPet )
		return FALSE;
	UIShowPetInventoryScreen( pPet );
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define SUPERJUMP_VELOCITY		12.0f

static BOOL sSkillEventStartLeapSimple(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	UnitStartJump( tData.pGame, tData.pUnit, SUPERJUMP_VELOCITY, FALSE );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventConsumeTargetItem(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT * pItem = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
	if(!pItem)
		return FALSE;

	if(!UnitIsA(pItem, UNITTYPE_ITEM))
		return FALSE;

	s_ItemDecrement(pItem);

	return TRUE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventUseTargetLocationFromSkill(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(tData.pUnit);
	ASSERT_RETFALSE(tData.pSkillEvent->tAttachmentDef.nAttachedDefId >= 0);
	UnitSetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, UnitGetStatVector(tData.pUnit, STATS_SKILL_TARGET_X, tData.pSkillEvent->tAttachmentDef.nAttachedDefId));

	return TRUE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventQuestAIUpdate(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(tData.pUnit);
	ASSERT_RETFALSE(tData.pSkillEvent->tAttachmentDef.nAttachedDefId >= 0);

	int nQuestId = UnitGetStat( tData.pUnit, STATS_AI_ACTIVE_QUEST_ID );
	s_QuestAIUpdate( tData.pUnit, nQuestId );

	return TRUE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventQuestNotify(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(tData.pUnit);
	ASSERT_RETFALSE(tData.pSkillEvent->tAttachmentDef.nAttachedDefId >= 0);

	s_QuestSkillNotify( tData.pUnit, tData.pSkillEvent->tAttachmentDef.nAttachedDefId, tData.nSkill );

	return TRUE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventAchievementsNotify(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(tData.pUnit);

	UNIT *pItem = SkillGetTarget(tData.pUnit, tData.nSkill, tData.idWeapon);
	ASSERT_RETFALSE(pItem);

	// We're just going to use this for item use right now
	if (UnitIsA(pItem, UNITTYPE_ITEM) &&
		UnitIsPlayer(tData.pUnit))
	{
		const UNIT_DATA *pUnitData = UnitGetData(pItem);
		ASSERT_RETFALSE(pUnitData);
		if ( pUnitData->pnAchievements )
		{
			// send the use to the achievements system
			for (int i=0; i < pUnitData->nNumAchievements; i++)
			{
				s_AchievementsSendItemUse(pUnitData->pnAchievements[i], pItem, tData.pUnit);
			}
		}
	}

	return TRUE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventCraftItemModItem(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	/*
	ASSERT_RETFALSE( tData.pUnit );
	UNIT *pOwner = UnitGetUltimateOwner( tData.pUnit );
	ASSERT_RETFALSE( pOwner );
	UNIT *pItem = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	ASSERT_RETFALSE( pItem );
	CRAFTING::s_CraftingModUnitByCraftingItem( pOwner, pItem, tData.pUnit );
	*/
#endif
	return TRUE;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventItemUpgrade(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pOperator = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if (pOperator && UnitIsPlayer( pOperator ))
	{
		s_ItemUpgradeOpen( pOperator );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventItemAugment(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pOperator = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if (pOperator && UnitIsPlayer( pOperator ))
	{
		s_ItemAugmentOpen( pOperator );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventItemUnMod(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pOperator = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if (pOperator && UnitIsPlayer( pOperator ))
	{
		s_ItemUnModOpen( pOperator );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventOpenEmail(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pOperator = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if (pOperator && UnitIsPlayer( pOperator ))
	{
		s_PlayerToggleUIElement( pOperator, UIE_EMAIL_PANEL, UIEA_OPEN );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventOpenAuction(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pOperator = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if (pOperator && UnitIsPlayer( pOperator ))
	{
		s_PlayerToggleUIElement( pOperator, UIE_AUCTION_PANEL, UIEA_OPEN );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventOpenDonation(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pOperator = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	if (pOperator && UnitIsPlayer( pOperator ))
	{
		s_PlayerToggleUIElement( pOperator, UIE_EMAIL_PANEL, UIEA_OPEN );
		//s_PlayerToggleUIElement( pOperator, UIE_DONATION_PANEL, UIEA_OPEN );
	}
#endif
	return TRUE;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAutoDoorNearbyPlayersCanEnter(
	UNIT *pUnit,
	float flMinDistanceSq)
{
	if (pUnit && ObjectIsWarp( pUnit ))
	{

		// go through all clients in level
		LEVEL *pLevel = UnitGetLevel( pUnit );
		for (GAMECLIENT *pClient = ClientGetFirstInLevel( pLevel );
			 pClient;
			 pClient = ClientGetNextInLevel( pClient, pLevel ))
		{
			UNIT *pPlayer = ClientGetControlUnit( pClient );
			if (UnitIsInLimbo( pPlayer ) == FALSE &&
				UnitsGetDistanceSquared( pPlayer, pUnit ) <= flMinDistanceSq)
			{
				if (WarpCanOperate( pPlayer, pUnit ) == OPERATE_RESULT_OK)
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
static BOOL sSkillEventAutoDoorMonitor(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT *pUnit = tData.pUnit;
	if (pUnit)
	{

		// must be active unit (in a level with players around it)
		if (UnitTestFlag( pUnit, UNITFLAG_DEACTIVATED ) == FALSE)
		{

			// get min distance required to operate door
			float flMinDistance = SkillEventGetParamFloat( pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 );
			float flMinDistanceSq = flMinDistance * flMinDistance;

			if (UnitTestMode( pUnit, MODE_OPEN ))
			{
				// opening mode, do nothing
			}
			else if (UnitTestMode( pUnit, MODE_OPENED ))
			{
				if (sAutoDoorNearbyPlayersCanEnter( pUnit, flMinDistanceSq ) == FALSE)
				{
					s_ObjectDoorOperate( pUnit, DO_CLOSE );
				}
			}
			else
			{
				if (sAutoDoorNearbyPlayersCanEnter( pUnit, flMinDistanceSq ) == TRUE)
				{
					s_ObjectDoorOperate( pUnit, DO_OPEN );
				}
			}

		}

	}
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventTestTargetRange(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	SkillCheckMeleeTargetRange( tData.pUnit, tData.nSkill, tData.pSkillData, tData.idWeapon, tData.nSkill, TRUE );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventStopMoving(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	UNIT *pUnit = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		pUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	}
	else if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_AI_TARGET )
	{
		DWORD dwAIGetTargetFlags = 0;
		SETBIT(dwAIGetTargetFlags, AIGTF_DONT_DO_TARGET_QUERY_BIT);
		pUnit = AI_GetTarget( tData.pGame, tData.pUnit, tData.nSkill, NULL, dwAIGetTargetFlags );
	}
	else if ( tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_USE_ULTIMATE_OWNER )
	{
		pUnit = UnitGetUltimateOwner( tData.pUnit );
	}

	if ( !pUnit)
		return FALSE;

	if (UnitEndModeGroup(pUnit, MODEGROUP_MOVE))
	{
		UnitStepListRemove( tData.pGame, pUnit );
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventForceToGround(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_UNIT_TARGET )
	{
		pUnit = SkillGetTarget( tData.pUnit, tData.nSkill, tData.idWeapon );
	}
	else if ( tData.pSkillEvent->dwFlags & SKILL_EVENT_FLAG_USE_AI_TARGET )
	{
		DWORD dwAIGetTargetFlags = 0;
		SETBIT(dwAIGetTargetFlags, AIGTF_DONT_DO_TARGET_QUERY_BIT);
		pUnit = AI_GetTarget( tData.pGame, tData.pUnit, tData.nSkill, NULL, dwAIGetTargetFlags );
	}
	else if ( tData.pSkillEvent->dwFlags2 & SKILL_EVENT_FLAG2_USE_ULTIMATE_OWNER )
	{
		pUnit = UnitGetUltimateOwner( tData.pUnit );
	}

	if ( !pUnit)
		return FALSE;

	UnitForceToGround(pUnit);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRespecSkills(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	return SkillRespec( tData.pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRespecPerks(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	return SkillRespec( tData.pUnit, SRF_PERKS_MASK );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventRespecAttribs(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	return PlayerAttribsRespec( tData.pUnit );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSkillEventWarpToLevelStart(
	const SKILL_EVENT_FUNCTION_DATA &tData)
{
	UNIT *pUnit = tData.pUnit;
	if ( !pUnit)
		return FALSE;

	LEVEL * pLevel = UnitGetLevel(pUnit);
	if(!pLevel)
		return FALSE;

	VECTOR vPosition, vFaceDirection;
	ROOM * pRoom = LevelGetStartPosition(tData.pGame, pUnit, LevelGetID(pLevel), vPosition, vFaceDirection, 0);
	if(!pRoom)
		return FALSE;

	DWORD dwWarpFlags = 0;
	SETBIT(dwWarpFlags, UW_TRIGGER_EVENT_BIT);
	UnitWarp(pUnit, pRoom, vPosition, vFaceDirection, UnitGetUpDirection(pUnit), dwWarpFlags);
	UnitSetSafe(pUnit);

	return TRUE;
}

//----------------------------------------------------------------------------
struct SKILL_EVENT_HANDLER_LOOKUP
{
	const char *pszName;
	PFN_SKILL_EVENT fpHandler;
};

//This is a tugboat function. This was writen early in development and probably can
//be removed. Need to check first....
static BOOL sSkillEventSetSkillTargetRandom(
	const SKILL_EVENT_FUNCTION_DATA & tData )
{
	ASSERT_RETFALSE(IS_SERVER(tData.pGame));
	ASSERT_RETFALSE( tData.pUnit );
	float maxRadius = MAX( SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 0 ), 0.0f );
	float minRadius = MAX( SkillEventGetParamFloat( tData.pUnit, tData.nSkill, tData.nSkillLevel, tData.pSkillEvent, 1 ), 0.0f );
	VECTOR tmp( RandGetFloat( tData.pGame, 0, 1 ) - .5f, RandGetFloat( tData.pGame, 0, 1 ) - .5f, 0.0f );
	VectorNormalize( tmp );
	VectorScale( tmp, tmp, ( ( maxRadius - minRadius ) * RandGetFloat( tData.pGame, 0, 1 ) ) + minRadius );
	VECTOR newPosToUse( UnitGetPosition( tData.pUnit ) );
	newPosToUse += tmp;
	UnitSetStatVector( tData.pUnit, STATS_SKILL_TARGET_X, tData.nSkill, newPosToUse);
	return TRUE;
}

//----------------------------------------------------------------------------
// This is here instead of skills.cpp because we need to unregister functions
// that are static to this file
//----------------------------------------------------------------------------
void SkillsUnregisterTimedEventsForUnitDeactivate(
	UNIT * pUnit)
{
	UnitUnregisterTimedEvent(pUnit, sHandleDoSkillOnKilledDelayed);
	UnitUnregisterTimedEvent(pUnit, sHandleDoMissileDelayedRadial);
	UnitUnregisterTimedEvent(pUnit, sHandleDoMissileDelayed);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	UnitUnregisterTimedEvent(pUnit, sDoSpawnMonsterFinite);
	UnitUnregisterTimedEvent(pUnit, sDoSpawnClassFinite);
#endif

	UnitUnregisterTimedEvent(pUnit, sHandleDoSkillDelayed);
}

//end check for remove
//----------------------------------------------------------------------------
// keep at bottom so that it's easier to find, and we have all of the functions declared above
//----------------------------------------------------------------------------
static const SKILL_EVENT_HANDLER_LOOKUP sgtSkillEventHandlerLookupTable[] =
{
	{ "StartLeap",							sSkillEventStartLeap },
	{ "AddAttachment",						sSkillEventAddAttachment },
	{ "AddFloatingAttachment",				sSkillEventFloatAttachment },
	{ "RemoveAttachment",					sSkillEventRemoveAttachment },
	{ "RemoveAllAttachments",				sSkillEventRemoveAllAttachments },
	{ "MeleeAttack",						sSkillEventMeleeAttack },
	{ "FireMissile",						sSkillEventFireMissile },
	{ "FireMissileAtTargetsInRange",		sSkillEventFireMissileOnTargetsInRange },
	{ "AddMuzzleFlash",						sSkillEventAddMuzzleFlash },
	{ "FireMissileNova",					sSkillEventFireMissileNova },
	{ "FireMissileNovaAround",				sSkillEventFireMissileNovaAround },
	{ "FireMissileNovaRandomPivot",			sSkillEventFireMissileNovaRandomPivot },
	{ "FireMissileSpread",					sSkillEventFireMissileSpread },
	{ "FireMissileAttachRope",				sSkillEventFireMissileAttachRope },
	{ "FireMissileUseAllAmmo",				sSkillEventFireMissileUseAllAmmo },
	{ "FireLaser",							sSkillEventAimLaser },
	{ "FireLaserToAll",						sSkillEventAimLaserToAll },
	{ "FireHose",							sSkillEventFireHose },
	{ "RemoveTarget",						sSkillEventRemoveTarget },
	{ "RemoveSelf",							sSkillEventRemoveSelf },
	{ "FindNextTarget",						sSkillEventFindNextTarget },
	{ "RangedInstantHit",					sSkillEventRangedInstantHit },
	{ "PlaceMissilesOnTargets",				sSkillEventPlaceMissilesOnTargets },
	{ "SetStateClient",						sSkillEventSetStateClient },
	{ "ClearStateClient",					sSkillEventClearStateClient },
#if !ISVERSION(CLIENT_ONLY_VERSION)
	{ "RemoveTargetAndSetState",			sSkillEventRemoveTargetAndSetState },
	{ "RemoveTargetAndSetState",			sSkillEventRemoveTargetAndSetState },
	{ "SpawnMonster",						sSkillEventSpawnMonster },
	{ "SpawnMonsterNearby",					sSkillEventSpawnMonsterNearby },
	{ "SpawnObject",						sSkillEventSpawnObject },
	{ "Die",								sSkillEventDie },
	{ "StatsSetStat",						sSkillEventSetStat },
	{ "SetState",							sSkillEventSetState },
	{ "SetStateOnPet",						sSkillEventSetStateOnPet },
	{ "ClearState",							sSkillEventClearState },
	{ "ToggleState",						sSkillEventToggleState },
	{ "ClearStateAllOfType",				sSkillEventStateClearAllOfType },
	{ "ClearStateAllOfTypeOnTargetsInRange",sSkillEventStateClearAllOfTypeOnTargetsInRange },
	{ "SetStateTransferFromTarget",			sSkillEventSetStateTransferFromTarget },
	{ "SetStateTransferFromTargetInArea",	sSkillEventSetStateTransferFromTargetToTargetsInArea },
	{ "SetStateAfterClearFor",				sSkillEventSetStateAfterClearFor },
	{ "IncrementState",						sSkillEventIncrementState },
	{ "IncrementStateOnTargetsInRange",		sSkillEventIncrementStateOnTargetsInRange },
	{ "RemoveTargetAndIncrementState",		sSkillEventRemoveTargetAndIncrementState },
	{ "AngerOthers",						sSkillEventAngerOthers },
	{ "AwakenEnemies",						sSkillEventAwakenEnemies },
	{ "AttackLocation",						sSkillEventAttackLocation },
	{ "AttackLocationShields",				sSkillEventAttackLocationShields },
	{ "AttackTarget",						sSkillEventAttackTarget },
	{ "PullTargetToSource",					sSkillEventPullTargetToSource },
	{ "PullTargetsToSelf",					sSkillEventPullTargetsToSelf },
	{ "SpawnBasic",							sSkillEventSpawnBasic },
	{ "ModifyAI",							sSkillEventModifyAI },
	{ "FullyHeal",							sSkillEventFullyHeal },
	{ "SpawnMinion",						sSkillEventSpawnPet },
	{ "SpawnMinionAtOwner",					sSkillEventSpawnPetAtOwner },
	{ "SpawnMinionAtLabelNode",				sSkillEventSpawnPetAtLabelNode },
	{ "ClientSpawnMonsterAtLabelNode",		sSkillEventSpawnMonsterAtLabelNode },
	{ "SpawnMinionsWallFormation",			sSkillEventSpawnWallOfPets },
	{ "SpawnMinionFromTarget",				sSkillEventSpawnPetFromTarget },
	{ "SpawnMonsterFromTarget",				sSkillEventSpawnMonsterFromTarget },
	{ "HellgateSpawnMonster",				sSkillEventHellgateSpawn },
	{ "DropPhatLoot",						sSkillEventDropLoot },
	{ "OverrideLoot",						sSkillEventOverrideLoot },
	{ "Recall",								sSkillEventRecall },
	{ "SpreadFire",							sSkillEventSpreadFire },
	{ "SetStateOnTargetsInRange",			sSkillEventSetStateOnTargetsInRange },
	{ "StatsIncrementStat",					sSkillEventIncrementStat },
	{ "StatsDecrementStat",					sSkillEventDecrementStat },
	{ "FreeSelf",							sSkillEventFreeSelf },
	{ "SpawnMonsterFinite",					sSkillEventSpawnMonsterFinite },
	{ "SpawnClassFinite",					sSkillEventSpawnClassFinite },
	{ "GiveLifeToCompanion",				sSkillEventGiveLiveToCompanion },
	{ "TradeLifeForPower",					sSkillEventTradeLifeForPower },
	{ "WarpFlybyAttack",					sSkillEventWarpFlybyAttack },
	{ "HordeSpawner",						sSkillEventHordeSpawner },
	{ "Teleport",							sSkillEventTeleport },
	{ "TeleportAroundOwner",				sSkillEventTeleportAroundOwner },
	{ "TeleportToTarget",					sSkillEventTeleportToTarget },
	{ "PlayerTeleportToTarget",				sSkillEventPlayerTeleportToTarget },
	{ "PlayerTeleportAndAttackTargets",		sSkillEventPlayerTeleportAndAttackTargets },
	{ "Decoy",								sSkillEventDecoy },
	{ "CreateTurret",						sSkillEventCreateTurret },
	{ "SpawnMinionWithInventory",			sSkillEventSpawnPetWithInventory },
	{ "TownPortal",							sSkillEventTownPortal },
	{ "SetStateTargetToSkillTarget",		sSkillEventSetStateTargetToSkillTarget },
	{ "MoveTargetInInventory",				sSkillEventMoveTargetInInventory },
	{ "MoveTargetIntoInventory",			sSkillEventMoveTargetIntoInventory },
	{ "MoveTargetOutOfInventory",			sSkillEventMoveTargetOutOfInventory },
	{ "AwardLuck",							sSkillEventAwardLuck },
	{ "KillPets",							sSkillEventKillPets },
	{ "KillAllUnitsOfTypeWithSameContainer",sSkillEventKillAllUnitsOfTypeWithSameContainer },
	{ "KillAllUnitsOfTypeWithinRange",		sSkillEventKillAllUnitsOfTypeWithinRange },
	{ "ActivateQuest",						sSkillEventsActivateQuest },
	{ "EnterPassage",						sSkillEventsEnterPassage },
	{ "ExitPassage",						sSkillEventsExitPassage },
	{ "HealPartially",						sSkillEventHealPartially },
	{ "ResurrectTarget",					sSkillEventResurrect },
	{ "StartSkillWhenTargetHit",			sSkillEventDoSkillOnHit },
	{ "StartAttackerSkillWhenTargetHit",	sSkillEventDoAttackerSkillOnDidHit },
	{ "StatsCopyEffectValue",				sSkillEventSetCopyEffectStat },
	{ "SetAnchorPointsOnPets",				sSkillEventSetAnchorPointOnPets },
	{ "SetTargetOverrideOnPets",			sSkillEventSetTargetOverrideOnPets },
	{ "SetDesiredPetLocation",				sSkillEventSetDesiredPetLocation },
	{ "SetDesiredPetTarget",				sSkillEventSetDesiredPetTarget },
	{ "ClearPetCommands",					sSkillEventClearPetCommands },
	{ "SetTargetPetFuseTime",				sSkillEventSetTargetPetFuseTime },
//	{ "PetTargetReaperPet",					sSkillEventPetTargetReaperPet },
	{ "ModItem",							sSkillEventModItem },
	{ "ResetWeaponsCooldown",				sSkillEventResetWeaponsCooldown },
	{ "Identify",							sSkillEventIdentify },
	{ "CreateGuild",						sSkillEventCreateGuild},
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
	{ "SetMeleeMoveMode",					sSkillEventSetMeleeMoveMode },
	{ "StartModeWeapon",					sSkillEventStartModeWeapon },
	{ "FireMissileGibs",					sSkillEventFireMissileGibs },
	{ "StartLeapSimple",					sSkillEventStartLeapSimple },
	{ "FaceTarget",							sSkillEventFaceTarget },
	{ "DoSkillToTargets",					sSkillEventDoSkillToTargets },
	{ "StartSkillWhenTargetKilled",			sSkillEventDoSkillOnKilled },
	{ "DoSkill",							sSkillEventDoSkill },
	{ "StopSkill",							sSkillEventStopSkill },
	{ "StopAllSkills",						sSkillEventStopAllSkills },
	{ "DoSkillOnTarget",					sSkillEventDoSkillOnTarget },
	{ "ChargeBlindly",						sSkillEventChargeBlindly },
	{ "AttackTargetsInRange",				sSkillEventAttackTargetsInRange },
	{ "PlayerCharge",						sSkillEventPlayerCharge },
	{ "StopLaser",							sSkillEventStopLaser },
	{ "SelectSpawnLocation",				sSkillEventSelectSpawnLocation },
	{ "SelectRandomSpawnLocation",			sSkillEventSelectRandomSpawnLocation },
	{ "AddAttachmentsBoneWeights",			sSkillEventAddAttachmentsBoneWeights },
	{ "DoMeleeItemEvents",					sSkillEventDoMeleeItemEvents },
	{ "Cascade",							sSkillEventCascade },
	{ "ClearAllPathingStuff",				sSkillEventClearAllPathingInfo },
	{ "SetUISkillPickMode",					sSkillEventSetUISkillPickMode },
	{ "ClearUISkillPickMode",				sSkillEventClearUISkillPickMode },
	{ "AddReplica",							sSkillEventAddReplica },
	{ "CameraShake",						sSkillEventCameraShake },
	{ "CameraSetRTS_TargetPosition",		sSkillEventCameraSetRTS_TargetPosition },
	{ "CreateObject",						sSkillEventCreateObject },
	{ "OperateDoor",						sSkillEventOperateDoor },
	{ "SetSkillTargetPosition",				sSkillEventSetSkillTargetRandom },
	{ "FireMissleInRandomRadius",			sSkillEventFireMissileRandomRadius },
	{ "FireMissleInRandomPathNode",			sSkillEventFireMissileRandomPathNode },
	{ "DoSkillOnMonsterDeath",				sSkillEventDoSkillOnKill },
	{ "TransferStatsToTargets",				sSkillEventTransferStatsToTargets },
	{ "RunScriptOnTargets",					sSkillEventApplyStatsToTargets },
	{ "RunScriptOnSelf",					sSkillEventApplyStatsToSelf },
	{ "RunScript",							sSkillEventRunScript },
	{ "GiveHPManaByPercent",				sSkillEventGiveHPManaByPercent },
	{ "StartCoolingAndPowerCost",			sSkillEventStartCoolingAndPowerCost },
	{ "RecipeOpen",							sSkillEventRecipeOpen },
	{ "CubeOpen",							sSkillEventCubeOpen },
	{ "BackpackOpen",						sSkillEventBackpackOpen },
	{ "ShowPetInventory",					sSkillEventShowPetInventory },
	{ "ConsumeTargetItem",					sSkillEventConsumeTargetItem },
	{ "PlayerTeleportToSafety",				sSkillEventPlayerTeleportToSafety },
	{ "UseTargetLocationFromSkill",			sSkillEventUseTargetLocationFromSkill },
	{ "QuestAIUpdate",						sSkillEventQuestAIUpdate },
	{ "ItemUpgrade",						sSkillEventItemUpgrade },
	{ "ItemAugment",						sSkillEventItemAugment },
	{ "ItemUnMod",							sSkillEventItemUnMod },
	{ "OpenEmail",							sSkillEventOpenEmail },
	{ "OpenAuction",						sSkillEventOpenAuction },
	{ "AutoDoorMonitor",					sSkillEventAutoDoorMonitor },
	{ "TestTargetRange",					sSkillEventTestTargetRange },
	{ "AddAttachmentToSummonedInventoryLocation",sSkillEventAddAttachmentToSummonedInventoryLocation },
	{ "RemoveAttachmentFromSummonedInventoryLocation",sSkillEventRemoveAttachmentFromSummonedInventoryLocation },
	{ "StopMoving",							sSkillEventStopMoving },
	{ "ForceToGround",						sSkillEventForceToGround },
	{ "WarpToLevelStart",					sSkillEventWarpToLevelStart },
	{ "QuestNotify",						sSkillEventQuestNotify },
	{ "AchievementsNotify",					sSkillEventAchievementsNotify},
	{ "CraftItemModItem",					sSkillEventCraftItemModItem},
	{ "Takeoff",							sSkillEventTakeoff},
	{ "Land",								sSkillEventLand},
	{ "RespecSkills",						sSkillEventRespecSkills},
	{ "RespecPerks",						sSkillEventRespecPerks},
	{ "RespecAttribs",						sSkillEventRespecAttribs},
	{ "TransferStatsFromTarget",			sSkillEventTransferStatsFromTarget},
	{ NULL, NULL }		// keep this last!
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static STR_DICTIONARY * s_SkillEventHandlerDictionary = NULL;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SkillEventTypeExcelPostProcess(
	EXCEL_TABLE * table)
{
	if (!s_SkillEventHandlerDictionary)
	{
		s_SkillEventHandlerDictionary = StrDictionaryInit(16);

		for (unsigned int jj = 0; sgtSkillEventHandlerLookupTable[jj].pszName != NULL; ++jj)
		{
			if (sgtSkillEventHandlerLookupTable[jj].fpHandler)
			{
				StrDictionaryAdd(s_SkillEventHandlerDictionary, sgtSkillEventHandlerLookupTable[jj].pszName, (void *)sgtSkillEventHandlerLookupTable[jj].fpHandler);
			}
		}
	}

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		SKILL_EVENT_TYPE_DATA * skill_event_type_data = (SKILL_EVENT_TYPE_DATA *)ExcelGetDataPrivate(table, ii);

		skill_event_type_data->pfnEventHandler = NULL;

		if (!skill_event_type_data->szEventHandler || skill_event_type_data->szEventHandler[0] == 0)
		{
			continue;
		}

		skill_event_type_data->pfnEventHandler = (PFN_SKILL_EVENT)StrDictionaryFind(s_SkillEventHandlerDictionary, skill_event_type_data->szEventHandler);
		if (!skill_event_type_data->pfnEventHandler)
		{
			ASSERTV_MSG("ERROR: Unable to find skill event handler '%s' for '%s' on row '%d'",
				skill_event_type_data->szEventHandler, skill_event_type_data->pszName,  ii);
		}

		if(skill_event_type_data->bCausesAttackEvent)
		{
			SETBIT(skill_event_type_data->dwFlagsUsed2, 10, TRUE);
		}
	}

	if (s_SkillEventHandlerDictionary)
	{
		StrDictionaryFree(s_SkillEventHandlerDictionary);
		s_SkillEventHandlerDictionary = NULL;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sSkillEventCompare(
	const void * pElement1,
	const void * pElement2)
{
	const SKILL_EVENT * pSkillEvent1 = (const SKILL_EVENT*)pElement1;
	const SKILL_EVENT * pSkillEvent2 = (const SKILL_EVENT*)pElement2;
	if(pSkillEvent1->fTime < pSkillEvent2->fTime)
	{
		return -1;
	}
	else if(pSkillEvent1->fTime > pSkillEvent2->fTime)
	{
		return 1;
	}
	return 0;
}

BOOL SkillEventsDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSyncLoad)
{
	ASSERT_RETFALSE(pDef);

	SKILL_EVENTS_DEFINITION * pDefinition = (SKILL_EVENTS_DEFINITION *)pDef;
	for(int i=0; i<pDefinition->nEventHolderCount; i++)
	{
		qsort(pDefinition->pEventHolders[i].pEvents, pDefinition->pEventHolders[i].nEventCount, sizeof(SKILL_EVENT), sSkillEventCompare);
	}
	return TRUE;
}

// please do not add functions down here.   It is really helpful to have the table at the bottom of the file
