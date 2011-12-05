//----------------------------------------------------------------------------
// FILE: quest_explore.cpp
//
// The explore quest is a template quest, in which the player must visit a
// specified precentage of the rooms in a level. 
//
// Code is adapted from the explore task, in tasks.cpp.
// 
// author: Chris March, 2-1-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_explore.h"
#include "quests.h"
#include "s_quests.h"
#include "npc.h"
#include "room_path.h"
#include "objects.h"

//----------------------------------------------------------------------------
struct QUEST_DATA_EXPLORE
{
	DWORD dwRoomsExplored[DWORD_FLAG_SIZE(MAX_ROOMS_PER_LEVEL)];	// bit is set when a room is visited
	int nDefaultSublevelRoomsExplored;								// number of rooms explored in sublevel
	BOOL bIsCurrent;												// has the data been updated to reflect current values?
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_EXPLORE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_EXPLORE *)pQuest->pQuestData;
}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST_DATA_EXPLORE *pQuestData)
{
	ZeroMemory( pQuestData->dwRoomsExplored, sizeof(pQuestData->dwRoomsExplored) );
	pQuestData->nDefaultSublevelRoomsExplored = 0;
	pQuestData->bIsCurrent = FALSE;
}
//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sEnoughRoomsExplored(
	QUEST *pQuest)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETFALSE( pPlayer, "expected player" );
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERTX_RETFALSE( pQuestGlobals, "expected QUEST_GLOBALS" );
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	ASSERTX_RETFALSE( pQuestDef, "expected QUEST_DEFINITION" );
	SUBLEVEL *pSubLevel = RoomGetSubLevel( UnitGetRoom( pPlayer ) );
	ASSERTX_RETFALSE( pSubLevel, "expected SUBLEVEL" );
	int nSubLevelRooms = pSubLevel->nRoomCount;

	return nSubLevelRooms == 0 ? 
			FALSE : 
			( float(pQuestGlobals->nDefaultSublevelRoomsActivated) / float(nSubLevelRooms) ) >= pQuestDef->fExplorePercent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOnRoomExplored( ROOM *pRoom, QUEST *pQuest, QUEST_DATA_EXPLORE *pQuestData )
{
	ASSERTX_RETURN( pRoom != NULL, "NULL room for player" );
	ASSERTX_RETURN( pRoom->nIndexInLevel < MAX_ROOMS_PER_LEVEL, "room index too large" );
	if ( UnitIsGhost( QuestGetPlayer( pQuest ) ) )
		return;

	if ( sEnoughRoomsExplored( pQuest ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE, QUEST_STATE_COMPLETE );
		QuestStateSet( pQuest, QUEST_STATE_EXPLORETEMPLATE_RETURN, QUEST_STATE_ACTIVE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageCreateLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_CREATE_LEVEL *pMessageCreateLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	// reset quest data, the level has a different set of rooms now
	QUEST_DATA_EXPLORE *pData = sQuestDataGet( pQuest );
	sQuestDataInit( pData );

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	if ( QuestStateGetValue( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE )== QUEST_STATE_ACTIVE &&
		 pMessageEnterLevel->nLevelNewDef == pQuestDef->nLevelDefDestinations[0] )
	{
		QUEST_DATA_EXPLORE * pData = sQuestDataGet( pQuest );
		if (pData->bIsCurrent)
		{
			// restore quest global data (it will be cleared if the level is ever recreated)
			QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
			memcpy( pQuestGlobals->dwRoomsActivated, pData->dwRoomsExplored, sizeof(pQuestGlobals->dwRoomsActivated) );
			pQuestGlobals->nDefaultSublevelRoomsActivated = pData->nDefaultSublevelRoomsExplored;
		}
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageLeaveLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_LEAVE_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ROOM *pRoom = UnitGetRoom( pPlayer );
	if ( pRoom &&
		 QuestStateGetValue( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE )== QUEST_STATE_ACTIVE &&
		 LevelGetDefinitionIndex( RoomGetLevel( pRoom ) ) == pQuestDef->nLevelDefDestinations[0] )
	{
		// save quest global data (it will be cleared if the level is ever recreated)
		QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
		QUEST_DATA_EXPLORE * pData = sQuestDataGet( pQuest );
		memcpy( pData->dwRoomsExplored, pQuestGlobals->dwRoomsActivated, sizeof(pData->dwRoomsExplored) );
		pData->nDefaultSublevelRoomsExplored = pQuestGlobals->nDefaultSublevelRoomsActivated;
		pData->bIsCurrent = TRUE;
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageRoomsActivated( 
	GAME* pGame,
	QUEST* pQuest)
{
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ROOM *pRoom = UnitGetRoom( pPlayer );
	ASSERTX_RETVAL( pRoom != NULL, QMR_OK, "NULL room for player" );
	if ( QuestStateGetValue( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE )== QUEST_STATE_ACTIVE &&
		 LevelGetDefinitionIndex( RoomGetLevel( pRoom ) ) == pQuestDef->nLevelDefDestinations[0] )
	{
		sOnRoomExplored( pRoom, pQuest, sQuestDataGet( pQuest ) );
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );

	if (QuestIsInactive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ) &&
			s_QuestCanBeActivated( pQuest ))
		{
			// describe the quest
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				pQuestDefinition->nDescriptionDialog );

			return QMR_NPC_TALK;
		}
	}
	else if (QuestIsActive( pQuest ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE ) == QUEST_STATE_COMPLETE )
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Reward: tell the player good job, and offer rewards
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nCompleteDialog,
					pQuestDefinition->nRewardDialog );

				return QMR_NPC_TALK;
			}
		}
		else
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Incomplete: tell the player to go kill the monster
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK,
					pQuestDefinition->nIncompleteDialog );

				return QMR_NPC_TALK;
			}
		}
	}
#endif
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					QuestStateSet( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE, QUEST_STATE_ACTIVE );

					const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
					UNIT *pPlayer = QuestGetPlayer( pQuest );
					ROOM *pRoom = UnitGetRoom( pPlayer );
					ASSERTX_RETVAL( pRoom != NULL, QMR_OK, "NULL room for player" );
					if ( QuestStateGetValue( pQuest, QUEST_STATE_EXPLORETEMPLATE_EXPLORE )== QUEST_STATE_ACTIVE &&
						LevelGetDefinitionIndex( RoomGetLevel( pRoom ) ) == pQuestDef->nLevelDefDestinations[0] )
					{
						UNIT *pPlayer = pQuest->pPlayer;
						ROOM *pRoom = UnitGetRoom( pPlayer );

						sOnRoomExplored( pRoom, pQuest, sQuestDataGet( pQuest ) );
					}
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_EXPLORETEMPLATE_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_EXPLORETEMPLATE_RETURN, QUEST_STATE_COMPLETE );

				if ( s_QuestComplete( pQuest ) )
				{								
					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{					
						return QMR_OFFERING;
					}
				}					
			}
		}
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
		case QM_SERVER_CREATE_LEVEL:
		{
			const QUEST_MESSAGE_CREATE_LEVEL * pMessageCreateLevel = ( const QUEST_MESSAGE_CREATE_LEVEL * )pMessage;
			return s_sQuestMessageCreateLevel( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						pMessageCreateLevel );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return s_sQuestMessageEnterLevel( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						pMessageEnterLevel );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_LEAVE_LEVEL:
		{
			const QUEST_MESSAGE_LEAVE_LEVEL * pMessageLeaveLevel = ( const QUEST_MESSAGE_LEAVE_LEVEL * )pMessage;
			return s_sQuestMessageLeaveLevel( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						pMessageLeaveLevel );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ROOMS_ACTIVATED:
		{
			return s_sQuestMessageRoomsActivated( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest);
		}		
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestExploreFree(
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
void QuestExploreInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_EXPLORE * pQuestData = ( QUEST_DATA_EXPLORE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_EXPLORE ) );
	pQuest->pQuestData = pQuestData;
	sQuestDataInit( pQuestData );
}
