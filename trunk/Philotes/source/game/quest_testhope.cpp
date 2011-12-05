//----------------------------------------------------------------------------
// FILE: quest_testhope.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_testhope.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "ai.h"
#include "quest_truth_e.h"
#include "states.h"
#include "skills.h"
#include "inventory.h"
#include "room_layout.h"
#include "room_path.h"
#include "s_questgames.h"
#include "pets.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

/*
testhope_travel
testhope_gauntlet
testhope_talk_lucious
testhope_use_lightning
testhope_travel_plow
testhope_talk_murmur
testhope_use_sigil

truth_e_talk_child
*/

//----------------------------------------------------------------------------
struct QUEST_DATA_TEST_HOPE
{
	int		nEmmera;
	int		nMurmur;
	int		nMurmurTown;
	int		nLucious;

	int		nFollowAI;
	int		nFollowMonsterAI;
	int		nAttackAI;
	int		nIdleAI;
	int		nMurmurScriptAI;

	int		nRTSEndSkill;
	
	int		nSigil;
	int		nLevelTestOfHope;

	int		nGauntletEnd;
	UNITID	idGauntletEnd;

	int		nRobotClicky;
	int		nRobot;

	int		nSydonai;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TEST_HOPE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TEST_HOPE *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sChangeAIs( QUEST * pQuest, int nClass, int nAI, BOOL bInteract )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );

	UNIT * pNPC = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, nClass );
	if ( pNPC )
	{
		if ( UnitGetStat( pNPC, STATS_AI ) != nAI )
		{
			AI_Free( pGame, pNPC );
			UnitSetStat( pNPC, STATS_AI, nAI );
			AI_Init( pGame, pNPC );

			if ( nAI == pQuestData->nFollowAI )
			{
				BOOL bSet = TRUE;
				UNITID idTarget = UnitGetStat( pNPC, STATS_AI_TARGET_OVERRIDE_ID );
				if ( idTarget )
				{
					UNIT * pTarget = UnitGetById( UnitGetGame( pNPC ), idTarget );
					if ( pTarget )
					{
						if ( UnitIsA( pTarget, UNITTYPE_PLAYER ) )
							bSet = FALSE;
					}
				}
				if ( bSet )
					UnitSetAITarget( pNPC, pQuest->pPlayer, TRUE );
			}

			if ( bInteract )
			{
				// when NPCs are idle, they can be interactive
				UnitSetInteractive( pNPC, UNIT_INTERACTIVE_ENABLED );
			}
			else
			{
				// this NPC is not interactive anymore
				UnitSetInteractive( pNPC, UNIT_INTERACTIVE_RESTRICED );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_SYDONAI_ATTACK_DISTANCE				6.0f
#define MAX_SYDONAI_ATTACK_DISTANCE_SQUARED		( MAX_SYDONAI_ATTACK_DISTANCE * MAX_SYDONAI_ATTACK_DISTANCE )

static QUEST_MESSAGE_RESULT sQuestMessageAIUpdate(
	QUEST *pQuest,
	const QUEST_MESSAGE_AI_UPDATE * pMessage)
{
	UNIT * pMonster = pMessage->pUnit;
	ASSERT_RETVAL( pMonster, QMR_OK );
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	GAME * pGame = UnitGetGame( pMonster );
	ASSERTX_RETVAL( UnitGetClass( pMonster ) == pQuestData->nSydonai, QMR_OK, "Test of Hope AI callback is Sydonai_script only" );
	ASSERTX_RETVAL( IS_SERVER( pGame ), QMR_OK, "Server only" );

	// clear previous flags
	s_StateClear( pMonster, UnitGetId( pMonster ), STATE_UBER_IMP_DEFEND, 0 );
	s_StateClear( pMonster, UnitGetId( pMonster ), STATE_UBER_IMP_ATTACK, 0 );
	s_StateClear( pMonster, UnitGetId( pMonster ), STATE_UBER_IMP_RUN_TO, 0 );

	// let's go after murmur!!
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_DEAD, TARGET_INVALID };
	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );
	UNIT * pMurmur = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nMurmur );
	if ( pMurmur )
	{
		if ( !IsUnitDeadOrDying( pMurmur ) )
		{
			// check how far away he is...
			float fDistance = VectorDistanceSquaredXY( pMurmur->vPosition, pMonster->vPosition );
			if ( fDistance >= MAX_SYDONAI_ATTACK_DISTANCE_SQUARED )
			{
				// walk to my label node
				s_StateSet( pMonster, pMonster, STATE_UBER_IMP_DEFEND, 0 );
			}
			else
			{
				// attack murmur
				UnitSetStat( pMonster, STATS_AI_TARGET_ID, UnitGetId( pMurmur ) );
				s_StateSet( pMonster, pMonster, STATE_UBER_IMP_ATTACK, 0 );
			}
			return QMR_FINISHED;
		}

		QUEST_GAME_NEARBY_INFO	tNearbyInfo;
		s_QuestGameNearbyInfo( pMonster, &tNearbyInfo, FALSE );

		// murmur is dead, i near the exit?
		if ( tNearbyInfo.nNumNearbyObjects > 0 )
		{
			for ( int i = 0; i < tNearbyInfo.nNumNearbyObjects; i++ )
			{
				if ( UnitGetClass( tNearbyInfo.pNearbyObjects[i] ) == pQuestData->nGauntletEnd )
				{
					float fDistanceSquare = VectorDistanceSquaredXY( pMonster->vPosition, tNearbyInfo.pNearbyObjects[i]->vPosition );
					float fFreeDistance = UnitGetCollisionRadius( pMonster ) + 2.0f;
					if ( fDistanceSquare < ( fFreeDistance * fFreeDistance ) )
					{
						UnitFree( pMonster, UFF_DELAYED | UFF_SEND_TO_CLIENTS | UFF_FADE_OUT );
						// once syd leaves.. make me targetable
						s_StateClear( pQuest->pPlayer, UnitGetId( pQuest->pPlayer ), STATE_AI_IGNORE, 0 );
						return QMR_FINISHED;
					}
					else
					{
						UnitSetStat( pMonster, STATS_AI_TARGET_OVERRIDE_ID, UnitGetId( tNearbyInfo.pNearbyObjects[i] ) );
						s_StateSet( pMonster, pMonster, STATE_UBER_IMP_RUN_TO, 0 );
						return QMR_FINISHED;
					}
				}
			}
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnSydonai( QUEST * pQuest )
{
	// find if on level...
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );

	// he already spawned?
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	UNIT * pSydonai = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nSydonai );
	if ( pSydonai )
		return;

	// spawn at label-node location
	VECTOR vPosition, vDirection;
	ROOM * pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "sydonai", &pRoom, &pOrientation );
	ASSERT_RETURN( pNode && pRoom && pOrientation );

	VECTOR vUp;
	RoomLayoutGetSpawnPosition( pNode, pOrientation, pRoom, vPosition, vDirection, vUp );

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, pRoom, vPosition, &pDestRoom );

	if ( pPathNode )
	{
		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nSydonai;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 28 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = vDirection;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		pSydonai = s_MonsterSpawn( pGame, tSpec );
		if ( pSydonai )
		{
			// MUST SET THIS STAT FOR AI CALLBACK!!!
			UnitSetStat( pSydonai, STATS_AI_ACTIVE_QUEST_ID, pQuest->nQuest );
			// once syd comes in.. make me untargetable
			s_StateSet( pQuest->pPlayer, pQuest->pPlayer, STATE_AI_IGNORE, 0 );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( s_QuestIsPrereqComplete( pQuest ) && UnitIsMonsterClass( pSubject, pQuestData->nEmmera ) )
		{
			return QMR_NPC_STORY_NEW;
		}
	}

	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TALK_LUCIOUS ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TALK_MURMUR ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nRobotClicky ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_USE_LIGHTNING ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_FINISHED;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TOH_EMMERA_START );
			return QMR_NPC_TALK;
		}
	}

	if ( !QuestIsActive( pQuest ) )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nEmmera ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TOH_EMMERA_START );
			return QMR_NPC_TALK;
		}
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TALK_LUCIOUS ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TOH_LUCIOUS_TALK, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
			return QMR_NPC_TALK;
		}
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TALK_MURMUR ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TOH_MURMUR_SYDONAI );
			return QMR_NPC_TALK;
		}
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nRobotClicky ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_USE_LIGHTNING ) == QUEST_STATE_ACTIVE )
		{
			// RTS time baby!
			UNIT * pPlayer = pQuest->pPlayer;
			GAME * pGame = UnitGetGame( pPlayer );
			if ( !UnitHasState( pGame, pPlayer, STATE_QUEST_RTS_HOPE ) )
			{
				LEVEL *pLevel = UnitGetLevel( pPlayer );

				DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_SERVER_ONLY;
				SkillStartRequest( pGame, pPlayer, GlobalIndexGet( GI_SKILLS_QUEST_RTS_HOPE_START ), UnitGetId( pPlayer ), VECTOR(0), dwFlags, SkillGetNextSkillSeed( pGame ) );
				LevelNeedRoomUpdate( pLevel, TRUE );
				QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
				pQuestGlobals->bTestOfHopeCheck = TRUE;

				// get pet (if there is one)
				UNIT * pPet = UnitInventoryGetByLocationAndXY( pPlayer, INVLOC_QUEST_RTS, 0, 0 );
				ASSERTX_RETVAL( pPet, QMR_FINISHED, "No Pet found in RTS Hope" );
				ASSERTX_RETVAL( UnitGetClass( pPet ) == pQuestData->nRobot, QMR_FINISHED, "Pet is not The Lightning" );
				// set murmur and emmera to follow pet, instead of me...
				sChangeAIs( pQuest, pQuestData->nMurmur, pQuestData->nFollowMonsterAI, FALSE );
				sChangeAIs( pQuest, pQuestData->nEmmera, pQuestData->nFollowMonsterAI, FALSE );
				TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
				UNIT * pNPC = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nMurmur );
				UnitSetAITarget( pNPC, pPet, TRUE );
				pNPC = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nEmmera );
				UnitSetAITarget( pNPC, pPet, TRUE );

				// make me invisible & invulnerable
				s_StateSet( pPlayer, pPlayer, STATE_DONT_DRAW_QUICK, 0 );
				s_StateSet( pPlayer, pPlayer, STATE_INVULNERABLE_ALL, 0 );
				// make robot invisible too! ( won't work for multiplayer - drb )
				s_StateSet( pSubject, pSubject, STATE_DONT_DRAW_QUICK, 0 );

				// clear the way!
				QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_USE_LIGHTNING, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_TRAVEL_PLOW, QUEST_STATE_ACTIVE );
			}
			return QMR_FINISHED;
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME * pGame = QuestGetGame( pQuest );
	UNIT * pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TOH_EMMERA_START:
		{
			s_QuestActivate( pQuest );			
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_TRAVEL, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_GAUNTLET, QUEST_STATE_ACTIVE );
			// have emmera and murmur follow
			sChangeAIs( pQuest, pQuestData->nEmmera, pQuestData->nFollowAI, FALSE );
			sChangeAIs( pQuest, pQuestData->nMurmur, pQuestData->nFollowAI, FALSE );
			// video about gauntlet from murmur plays in a few
			s_NPCVideoStart( pGame, pPlayer, GI_MONSTER_MURMUR, NPC_VIDEO_INCOMING, DIALOG_TOH_MURMUR_GAUNTLET );
		
			return QMR_OFFERING;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_TOH_LUCIOUS_TALK:
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_TALK_LUCIOUS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_USE_LIGHTNING, QUEST_STATE_ACTIVE );
			// video about robot from murmur plays in a few
			s_NPCVideoStart( pGame, pPlayer, GI_MONSTER_MURMUR, NPC_VIDEO_INSTANT_INCOMING, DIALOG_TOH_MURMUR_ROBOT );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TOH_MURMUR_SYDONAI:
		{
			s_NPCEmoteStart( pGame, pPlayer, DIALOG_TOH_MURMUR_EMOTE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_TESTHOPE_TALK_MURMUR, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_TESTHOPE_USE_SIGIL, QUEST_STATE_ACTIVE );
			// spawn mr. squid
			sSpawnSydonai( pQuest );
			// have murmur no longer invulnerable
			s_StateClear( pTalkingTo, UnitGetId( pTalkingTo ), STATE_NPC_GOD, 0 );
			break;
		}

	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	GAME * pGame = QuestGetGame( pQuest );
	UNIT * pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nSigil ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_USE_SIGIL ) == QUEST_STATE_ACTIVE )
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			return QMR_OPERATE_FORBIDDEN;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( ( UnitGetClass( pTarget ) == pQuestData->nSigil ) &&
		 ( UnitGetLevelDefinitionIndex( pTarget ) == pQuestData->nLevelTestOfHope ) &&
		 ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_USE_SIGIL ) == QUEST_STATE_ACTIVE ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_USE_SIGIL, QUEST_STATE_COMPLETE );
		s_QuestComplete( pQuest );
		// spawn truth_e quest
		QUEST * pTruth = QuestGetQuest( pQuest->pPlayer, QUEST_TRUTH_E );
		s_QuestTruthEBegin( pTruth );
		return QMR_FINISHED;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnRobot( QUEST * pQuest, UNIT * pLocationUnit )
{
	GAME * pGame = UnitGetGame( pQuest->pPlayer );

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, UnitGetRoom( pLocationUnit ), UnitGetPosition( pLocationUnit ), &pDestRoom );

	if ( pNode )
	{
		QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nRobotClicky;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, RoomGetLevel( pDestRoom ), tSpec.nClass, 25 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = UnitGetFaceDirection( pLocationUnit, FALSE );
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( UnitGetGame( pLocationUnit ), tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEndRTS( QUEST * pQuest, UNIT * pDead )
{
	UNIT * pPlayer = pQuest->pPlayer;

	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	pQuestGlobals->bTestOfHopeCheck = FALSE;

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	UNIT * pPet = NULL;
	if ( !pDead )
	{
		pPet = UnitInventoryGetByLocationAndXY( pPlayer, INVLOC_QUEST_RTS, 0, 0 );
	}
	else
	{
		pPet = pDead;
	}
	ASSERTX_RETURN( pPet, "No Pet found in RTS Hope" );
	ASSERTX_RETURN( UnitGetClass( pPet ) == pQuestData->nRobot, "Pet is not The Lightning" );

	if(PetGetOwner(pPet) == UnitGetId(pPlayer))
	{
		// warp the player there...
		VECTOR vFaceDirection = UnitGetFaceDirection(pPlayer, FALSE);
		vFaceDirection.z = 0.0f;
		VECTOR vUp = UnitGetUpDirection(pPlayer);
		VECTOR vPosition = pPet->vPosition;
		vPosition -= vFaceDirection * 2.0f;
		UnitWarp( pPlayer, pPet->pRoom, vPosition, vFaceDirection, vUp, 0 );

		// make me visible & vulernable
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_DONT_DRAW_QUICK, 0 );
		s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_INVULNERABLE_ALL, 0 );
	}

	if ( !pDead )
	{
		s_StateSet( pPet, pPet, STATE_DONT_DRAW_QUICK, 0 );
		// create new robot at this location
		sSpawnRobot( pQuest, pPet );
	}

	sChangeAIs( pQuest, pQuestData->nMurmur, pQuestData->nFollowAI, FALSE );
	sChangeAIs( pQuest, pQuestData->nEmmera, pQuestData->nFollowAI, FALSE );

	s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_RTS_HOPE, 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_MURMUR_END_DISTANCE				8.0f
#define MAX_MURMUR_END_DISTANCE_SQUARED		( MAX_MURMUR_END_DISTANCE * MAX_MURMUR_END_DISTANCE )

static void sMaybeWarpMurmur( QUEST * pQuest )
{
	// find on level...
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );

	// he already spawned?
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * pMurmur = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nMurmur );
	ASSERTX_RETURN( pMurmur, "Can't find murmur to finish test of hope" );

	float fDistanceSquared = VectorDistanceSquaredXY( pQuest->pPlayer->vPosition, pMurmur->vPosition );
	if ( fDistanceSquared > MAX_MURMUR_END_DISTANCE_SQUARED )
	{
		// find label
		VECTOR vPosition, vDirection;
		ROOM * pRoom;
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
		ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "murmur_runto", &pRoom, &pOrientation );
		ASSERT_RETURN( pNode && pRoom && pOrientation );

		// warp him
		VECTOR vUp;
		RoomLayoutGetSpawnPosition( pNode, pOrientation, pRoom, vPosition, vDirection, vUp );
		vDirection.z = 0.0f;
		vUp = VECTOR( 0.0f, 0.0f, 1.0f );
		UnitWarp( pMurmur, pRoom, vPosition, vDirection, vUp, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

	if ( UnitIsGenusClass( pTarget, GENUS_MONSTER, pQuestData->nLucious ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_GAUNTLET ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_GAUNTLET, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_TALK_LUCIOUS, QUEST_STATE_ACTIVE );
			return QMR_FINISHED;
		}
	}

	if ( UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestData->nSigil ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TRAVEL_PLOW ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_TRAVEL_PLOW, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TESTHOPE_TALK_MURMUR, QUEST_STATE_ACTIVE );
			// stop emmera and murmur from walking...
			sChangeAIs( pQuest, pQuestData->nEmmera, pQuestData->nIdleAI, TRUE );
			sMaybeWarpMurmur( pQuest );
			sChangeAIs( pQuest, pQuestData->nMurmur, pQuestData->nMurmurScriptAI, TRUE );
			return QMR_FINISHED;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sResetInteractive( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	UNIT * pEmmera = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nEmmera );
	if ( pEmmera )
	{
		// this NPC is now interactive
		UnitSetInteractive( pEmmera, UNIT_INTERACTIVE_ENABLED );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	// make sure you can talk to emmera
	if ( !QuestIsActive( pQuest ) )
	{
		if ( pMessage->nLevelNewDef == pQuestData->nLevelTestOfHope )
			sResetInteractive( pQuest );
		return QMR_OK;
	}

	// quest is active
	if ( pMessage->nLevelNewDef == pQuestData->nLevelTestOfHope )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_GAUNTLET ) >= QUEST_STATE_ACTIVE )
		{
			sChangeAIs( pQuest, pQuestData->nEmmera, pQuestData->nFollowAI, FALSE );
			sChangeAIs( pQuest, pQuestData->nMurmur, pQuestData->nFollowAI, FALSE );
		}
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	ASSERTX_RETVAL( UnitGetGenus( pVictim ) == GENUS_MONSTER, QMR_FINISHED, "Quest Message: Monster Kill is for monsters only!" );

	// not interested unless they died on the right level
	if ( UnitGetLevelDefinitionIndex( pVictim ) != pQuestData->nLevelTestOfHope )
		return QMR_OK;

	// is this our pet?
	if ( UnitIsMonsterClass( pVictim, pQuestData->nRobot ) )
	{
		sEndRTS( pQuest, pVictim );
		return QMR_FINISHED;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageIsUnitVisible( 
	QUEST *pQuest,
	const QUEST_MESSAGE_IS_UNIT_VISIBLE * pMessage)
{
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );
	UNIT * pUnit = pMessage->pUnit;

	if ( UnitGetGenus( pUnit ) != GENUS_MONSTER )
		return QMR_OK;

	int nUnitClass = UnitGetClass( pUnit );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( nUnitClass == pQuestData->nMurmur )
	{
		if ( eStatus >= QUEST_STATUS_COMPLETE )
			return QMR_UNIT_HIDDEN;
		else
			return QMR_UNIT_VISIBLE;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageSkillNotify(
	QUEST * pQuest,
	const QUEST_MESSAGE_SKILL_NOTIFY * pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	if ( UnitGetLevelDefinitionIndex( pQuest->pPlayer ) != pQuestData->nLevelTestOfHope )
		return QMR_OK;

	if ( pMessage->nSkillIndex == pQuestData->nRTSEndSkill )
	{
		sEndRTS( pQuest, NULL );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestAbandon(
	QUEST * pQuest )
{
	QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

	if ( UnitGetLevelDefinitionIndex( pQuest->pPlayer ) == pQuestData->nLevelTestOfHope )
	{
		sChangeAIs( pQuest, pQuestData->nEmmera, pQuestData->nIdleAI, TRUE );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{
	switch (eMessageType)
	{
		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_AI_UPDATE:
		{
			const QUEST_MESSAGE_AI_UPDATE * pAIUpdateMessage = (const QUEST_MESSAGE_AI_UPDATE *)pMessage;
			return sQuestMessageAIUpdate( pQuest, pAIUpdateMessage );
		}

		//----------------------------------------------------------------------------
		case QM_IS_UNIT_VISIBLE:
		{
			const QUEST_MESSAGE_IS_UNIT_VISIBLE *pVisibleMessage = (const QUEST_MESSAGE_IS_UNIT_VISIBLE *)pMessage;
			return sQuestMessageIsUnitVisible( pQuest, pVisibleMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_SKILL_NOTIFY:
		{
			const QUEST_MESSAGE_SKILL_NOTIFY * pNotifyMessage = (const QUEST_MESSAGE_SKILL_NOTIFY *)pMessage;
			return sQuestMessageSkillNotify( pQuest, pNotifyMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ABANDON:
		{
			return sQuestAbandon( pQuest );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTestHope(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST * pQuest,
	QUEST_DATA_TEST_HOPE * pQuestData)
{
	pQuestData->nEmmera					= QuestUseGI( pQuest, GI_MONSTER_EMMERA_COMBAT );
	pQuestData->nMurmur					= QuestUseGI( pQuest, GI_MONSTER_MURMUR_TUTORIAL );
	pQuestData->nMurmurTown				= QuestUseGI( pQuest, GI_MONSTER_MURMUR );
	pQuestData->nLucious				= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );

	pQuestData->nFollowAI				= QuestUseGI( pQuest, GI_AI_TECH_FOLLOW );
	pQuestData->nFollowMonsterAI		= QuestUseGI( pQuest, GI_AI_TECH_FOLLOW_MONSTER );
	pQuestData->nAttackAI				= QuestUseGI( pQuest, GI_AI_WORM_HUNTER );
	pQuestData->nIdleAI					= QuestUseGI( pQuest, GI_AI_IDLE );
	pQuestData->nMurmurScriptAI			= QuestUseGI( pQuest, GI_AI_MURMUR_SCRIPT );

	pQuestData->nRTSEndSkill			= QuestUseGI( pQuest, GI_SKILLS_QUEST_RTS_HOPE_END );

	pQuestData->nSigil					= QuestUseGI( pQuest, GI_OBJECT_SIGIL );
	pQuestData->nLevelTestOfHope		= QuestUseGI( pQuest, GI_LEVEL_TEST_OF_HOPE );

	pQuestData->nGauntletEnd			= QuestUseGI( pQuest, GI_OBJECT_GAUNTLET_END );
	pQuestData->idGauntletEnd			= INVALID_ID;

	pQuestData->nRobotClicky			= QuestUseGI( pQuest, GI_MONSTER_LIGHTNING_CLICKY );
	pQuestData->nRobot					= QuestUseGI( pQuest, GI_MONSTER_THE_LIGHTNING );

	pQuestData->nSydonai				= QuestUseGI( pQuest, GI_MONSTER_SYDONAI_SCRIPT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTestHope(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TEST_HOPE * pQuestData = ( QUEST_DATA_TEST_HOPE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TEST_HOPE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nEmmera );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nMurmur );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLucious );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nRobotClicky );

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS ) | MAKE_MASK( QSMF_IS_UNIT_VISIBLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTestHope(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestToHBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TESTHOPE, "Wrong quest sent to function. Need Test of Hope" );

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_ACTIVE )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define TEST_OF_HOPE_RTS_END_DISTANCE			5.0f

void s_QuestToHCheck( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TESTHOPE, "Wrong quest sent to function. Need Test of Hope" );

	if ( !QuestIsActive( pQuest ) )
	{
		QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
		pQuestGlobals->bTestOfHopeCheck = FALSE;
		return;
	}

	if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TRAVEL_PLOW ) == QUEST_STATE_ACTIVE )
	{
		UNIT * pPlayer = pQuest->pPlayer;
		GAME * pGame = UnitGetGame( pPlayer );
		if ( !UnitHasState( pGame, pPlayer, STATE_QUEST_RTS ) )
			return;

		QUEST_DATA_TEST_HOPE * pQuestData = sQuestDataGet( pQuest );

		// get end object
		UNIT * pEnd = NULL;
		if ( pQuestData->idGauntletEnd == INVALID_ID )
		{
			LEVEL * pLevel = UnitGetLevel( pPlayer );
			TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
			pEnd = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nGauntletEnd );
			ASSERTX_RETURN( pEnd, "Couldn't find end of RTS Hope object (Gauntlet End)" );
			pQuestData->idGauntletEnd = UnitGetId( pEnd );
		}
		else
		{
			pEnd = UnitGetById( pGame, pQuestData->idGauntletEnd );
			ASSERTX_RETURN( pEnd, "Couldn't find end of RTS Hope object (Gauntlet End)" );
		}

		// get pet (if there is one)
		UNIT * pPet = UnitInventoryGetByLocationAndXY( pPlayer, INVLOC_QUEST_RTS, 0, 0 );
		ASSERTX_RETURN( pPet, "No Pet found in RTS Hope" );
		ASSERTX_RETURN( UnitGetClass( pPet ) == pQuestData->nRobot, "Pet is not The Lightning" );

		// calc distance and trigger if need be
		float fDistanceSquared = VectorDistanceSquared( pEnd->vPosition, pPet->vPosition );
		if ( fDistanceSquared < ( TEST_OF_HOPE_RTS_END_DISTANCE * TEST_OF_HOPE_RTS_END_DISTANCE ) )
		{
			// trigger / end
			sEndRTS( pQuest, NULL );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL QuestIsMurmurDead( UNIT * pPlayer )
{
	ASSERTX_RETFALSE( pPlayer, "Need unit to check if Murmur is dead" );

	// get quest
	QUEST * pQuest = QuestGetQuest( pPlayer, QUEST_TESTHOPE );
	ASSERTX_RETFALSE( pQuest, "Test of Hope not found" );

	if ( QuestGetStatus( pQuest ) < QUEST_STATUS_ACTIVE )
		return FALSE;

	if ( QuestIsActive( pQuest ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TESTHOPE_TALK_MURMUR ) < QUEST_STATE_COMPLETE )
			return FALSE;
	}

	return TRUE;
}
