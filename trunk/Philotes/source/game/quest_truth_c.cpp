//----------------------------------------------------------------------------
// FILE: quest_truth_c.cpp
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
#include "quest_truth_c.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "ai.h"
#include "uix.h"
#include "states.h"
#include "monsters.h"
#include "level.h"
#include "room.h"
#include "room_path.h"
#include "room_layout.h"
#include "offer.h"
#include "e_portal.h"
#include "inventory.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_TRUTH_C
{
	int		nLordArphaun;
	int		nRorke;
	int		nSerSing;

	int		nItemCleanser;

	int		nLevelAngelPassage;
	int		nLevelMonumentStation;

	int		nWarpAngelMonument;
	int		nWarpMonumentAngel;

	int		nSpectralBoss;
	int		nFuruncle;

	int		nSage;
	int		nSageNew;
	
	int		nTruthEnterPortal;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TRUTH_C * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TRUTH_C *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestTruthCStart(
	QUEST * pQuest )
{
	ASSERT( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ) );
	QUEST * pQuestTruthC = QuestGetQuest( pQuest->pPlayer, QUEST_TRUTH_C );
	if ( pQuestTruthC && s_QuestActivate( pQuestTruthC ) )
	{
		QuestStateSet( pQuestTruthC, QUEST_STATE_TRUTH_C_KILL_BOSS, QUEST_STATE_ACTIVE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnOrDestroyFuruncle( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	UNIT * pPlayer = pQuest->pPlayer;
	ASSERT_RETURN( pPlayer );
	GAME * pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	// first let's see if there is one on the level
	QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	ASSERT_RETURN( pLevel );
	UNIT * pFuruncle = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nFuruncle );
	if ( pFuruncle )
	{
		return;
	}

	UNIT * pSpectralBoss = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nSpectralBoss );
	if ( pSpectralBoss )
	{
		return;
	}

	// spawn a furuncle
	ROOM * pRoom;
	ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = NULL;
	ROOM_LAYOUT_GROUP * pNode = LevelGetLabelNode( pLevel, "furuncle", &pRoom, &pOrientation );
	ASSERTX_RETURN( pNode && pRoom && pOrientation, "Couldn't find Wurm spawn location" );

	VECTOR vPosition, vDirection, vUp;
	RoomLayoutGetSpawnPosition( pNode, pOrientation, pRoom, vPosition, vDirection, vUp );

	ROOM * pDestRoom;
	ROOM_PATH_NODE * pPathNode = RoomGetNearestPathNode( pGame, pRoom, vPosition, &pDestRoom );

	if ( pPathNode )
	{
		// init location
		SPAWN_LOCATION tLocation;
		SpawnLocationInit( &tLocation, pDestRoom, pPathNode->nIndex );

		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nFuruncle;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 17 );
		tSpec.pRoom = pDestRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = vDirection;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnSpectralBoss( QUEST * pQuest, UNIT * pBoil )
{
	// check the level for a boss already
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_DEAD, TARGET_INVALID };
	QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );
	UNIT * pBoss = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nSpectralBoss );
	if ( pBoss )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pBoil );
	ROOM * pRoom = UnitGetRoom( pBoil );
	ROOM * pDestRoom;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pRoom, UnitGetPosition( pBoil ), &pDestRoom );

	if ( !pNode )
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nSpectralBoss;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 18 );
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus == QUEST_STATUS_ACTIVE )
	{
		// quest active
		if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
		{
			DWORD dwInventoryIterateFlagsCleanserCheck = 0;  
			SETBIT( dwInventoryIterateFlagsCleanserCheck, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
			BOOL bHasCleanser = s_QuestCheckForItem( pQuest->pPlayer, pQuestData->nItemCleanser, dwInventoryIterateFlagsCleanserCheck );
			if (bHasCleanser == FALSE)
			{
				return QMR_NPC_STORY_RETURN;
			}
			return QMR_OK;
		}
	}

	if ( eStatus >= QUEST_STATUS_ACTIVE )
		return QMR_OK;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return QMR_OK;

	// quest active
	if ( UnitIsMonsterClass( pSubject, pQuestData->nRorke ) )
	{
		return QMR_NPC_STORY_NEW;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasWeapon(
	void *pUserData)
{
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
	QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );

	if (UnitIsMonsterClass( pSubject, pQuestData->nSage ))
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER );		
			
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_C_SISTER );
		return QMR_NPC_TALK;

	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nSageNew ))
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER_2 );		
	
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER_2 ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_C_SISTER_2 );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( pQuest->eStatus >= QUEST_STATUS_COMPLETE )
	{
		return QMR_OK;
	}

	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( s_QuestIsPrereqComplete( pQuest ) && UnitIsMonsterClass( pSubject, pQuestData->nRorke ) )
		{
			s_QuestTruthCStart( pQuest );
			return QMR_NPC_TALK;
		}
	}

	if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	if ( UnitIsMonsterClass( pSubject, pQuestData->nLordArphaun ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_C_REPORT ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_C_TEMPLAR_END );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	// active quest logic
	if (UnitIsMonsterClass( pSubject, pQuestData->nSerSing ))
	{
		// do they have the cleanser weapon
		DWORD dwInventoryIterateFlagsCleanserCheck = 0;  
		SETBIT( dwInventoryIterateFlagsCleanserCheck, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
		BOOL bHasCleanser = s_QuestCheckForItem( pPlayer, pQuestData->nItemCleanser, dwInventoryIterateFlagsCleanserCheck );	

		// give them the cleanser if they don't have one
		if ( bHasCleanser == FALSE )
		{
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasWeapon;
			tActions.pAllTakenUserData = pQuest;

			// offer to player
			s_OfferGive( pQuest->pPlayer, pSubject, OFFERID_QUESTCLEANUPOFFER, tActions );
			return QMR_OFFERING;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_C_SISTER:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER_2, QUEST_STATE_ACTIVE );
			
			// transport to new truth room
			UNIT *pPlayer = QuestGetPlayer( pQuest );
			s_QuestTransportToNewTruthRoom( pPlayer );
			
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_C_SISTER_2:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER_2, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_TRAVEL, QUEST_STATE_ACTIVE );
			UNIT * pPlayer = pQuest->pPlayer;
			QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );
			s_QuestOpenWarp( UnitGetGame( pPlayer ), pPlayer, pQuestData->nWarpAngelMonument );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_C_TEMPLAR_END:
		case DIALOG_TRUTH_C_CABALIST_END:
		case DIALOG_TRUTH_C_HUNTER_END:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_REPORT, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			break;
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
	QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pKiller = UnitGetById( pGame, pMessage->idKiller );
		
	// once the furuncle dies, spawn the boss!
	LEVEL * pLevel = UnitGetLevel( pVictim );
	if ( UnitIsMonsterClass( pVictim, pQuestData->nFuruncle ))
	{
		if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelAngelPassage )
		{
			sSpawnSpectralBoss( pQuest, pVictim );
		}
	}
	
	if ( UnitIsMonsterClass( pVictim, pQuestData->nSpectralBoss ) )
	{
		if (QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_C_KILL_BOSS ) == QUEST_STATE_ACTIVE)
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_KILL_BOSS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_TALK_SISTER, QUEST_STATE_ACTIVE );
			s_QuestSpawnObject( pQuestData->nTruthEnterPortal, pVictim, pKiller );		
		}
	}

	return QMR_OK;

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

	QUEST_DATA_TRUTH_C * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelAngelPassage )
	{
		sSpawnOrDestroyFuruncle( pQuest );
		return QMR_OK;
	}

	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if ( pMessage->nLevelNewDef == pQuestData->nLevelMonumentStation )
	{
		QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_TRAVEL, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_TRUTH_C_REPORT, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_TRUTH_C *pQuestData = sQuestDataGet( pQuest );


	if ( UnitIsObjectClass( pObject, pQuestData->nWarpAngelMonument ) )
	{
		QUEST_STATUS eStatus = QuestGetStatus( pQuest );

		if ( eStatus == QUEST_STATUS_INACTIVE )
			return QMR_OPERATE_FORBIDDEN;

		if ( eStatus >= QUEST_STATUS_COMPLETE )
			return QMR_OPERATE_OK;

		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_C_TRAVEL ) >= QUEST_STATE_ACTIVE )
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			return QMR_OPERATE_FORBIDDEN;
		}

	}
	else if ( UnitIsObjectClass( pObject, pQuestData->nWarpMonumentAngel ) )
	{
		// one way portal! don't let people cheese Oculus, try to make them 
		// go back to Temple Station and do the whole lead up level progression
		return QMR_OPERATE_FORBIDDEN;
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
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}
		
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTruthC(
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
	QUEST *pQuest, 
	QUEST_DATA_TRUTH_C * pQuestData)
{
	pQuestData->nLordArphaun			= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nSerSing				= QuestUseGI( pQuest, GI_MONSTER_SER_SING );
	pQuestData->nRorke					= QuestUseGI( pQuest, GI_MONSTER_RORKE );
	pQuestData->nItemCleanser			= QuestUseGI( pQuest, GI_ITEM_CLEANSER );	
	pQuestData->nFuruncle				= QuestUseGI( pQuest, GI_MONSTER_FURUNCLE );
	pQuestData->nSpectralBoss			= QuestUseGI( pQuest, GI_MONSTER_SPECTRAL_BOSS );
	ASSERT( pQuestData->nSpectralBoss == pQuest->pQuestDef->nMonsterBoss );
	pQuestData->nTruthEnterPortal		= QuestUseGI( pQuest, GI_OBJECT_TRUTH_C_OLD_ENTER );
	pQuestData->nLevelAngelPassage		= QuestUseGI( pQuest, GI_LEVEL_ANGEL_PASSAGE );
	pQuestData->nLevelMonumentStation	= QuestUseGI( pQuest, GI_LEVEL_MONUMENT_STATION );
	pQuestData->nSage					= QuestUseGI( pQuest, GI_MONSTER_TRUTH_C_SAGE_OLD );
	pQuestData->nSageNew				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_C_SAGE_NEW );
	pQuestData->nWarpAngelMonument		= QuestUseGI( pQuest, GI_OBJECT_WARP_ANGELPASSAGE_MONUMENT );
	pQuestData->nWarpMonumentAngel		= QuestUseGI( pQuest, GI_OBJECT_WARP_MONUMENT_ANGELPASSAGE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTruthC(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRUTH_C * pQuestData = ( QUEST_DATA_TRUTH_C * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TRUTH_C ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nLordArphaun );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nRorke );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nSerSing );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTruthC(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

