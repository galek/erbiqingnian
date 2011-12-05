//----------------------------------------------------------------------------
// FILE: metagame.cpp
//
//	Funcitonality shared by quests and PvP games.
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "metagame.h"
#include "quests.h"
#include "s_quests.h"
#include "customgame.h"
#include "ctf.h"


//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT MetagameSendMessage(
	UNIT * pPlayer,
	QUEST_MESSAGE_TYPE eMessageType,
	void *pMessage,
	DWORD dwFlags )
{
	ASSERT_RETVAL(pPlayer, QMR_INVALID);
	
	GAME *pGame = UnitGetGame(pPlayer);
	if (pGame && pGame->m_pMetagame && pGame->m_pMetagame->pfnMessageHandler)
	{
		 QUEST_MESSAGE_RESULT eResult = 
			pGame->m_pMetagame->pfnMessageHandler(pPlayer, eMessageType, pMessage);

		 if (eResult != QMR_OK)
			return eResult;
	}

	return QuestSendMessage(pPlayer, eMessageType, pMessage, dwFlags);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MetagameInit(GAME *pGame, const GAME_SETUP * pGameSetup)
{
	pGame->m_pMetagame = NULL;

	switch (GameGetSubType(pGame))
	{
	case GAME_SUBTYPE_CUSTOM:
		if (IS_SERVER(pGame))
			ASSERT_RETURN(s_CustomGameSetup(pGame, pGameSetup));
		break;

	case GAME_SUBTYPE_PVP_CTF:
		ASSERT_RETURN(!pGame->m_pMetagame);
		pGame->m_pMetagame = (METAGAME *)GMALLOCZ(pGame, sizeof(METAGAME));

		ASSERT_RETURN(CTFInit(pGame, pGameSetup));
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MetagameFree(GAME *pGame)
{
	if (pGame->m_pMetagame)
	{
		if (pGame->m_pMetagame->pData)
		{
			GFREE(pGame, pGame->m_pMetagame->pData);
			pGame->m_pMetagame->pData = NULL;
		}

		GFREE(pGame, pGame->m_pMetagame);
		pGame->m_pMetagame = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
METAGAME * GameGetMetagame(GAME *pGame)
{
	return pGame->m_pMetagame;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT MetagameSendMessageForPlayer(
	UNIT *pUnit,
	QUEST_MESSAGE_TYPE eMessageType,
	void *pMessage)
{	
	ASSERTX_RETVAL( pUnit, QMR_INVALID, "Expected game" );

	// run message handler
	QUEST_MESSAGE_RESULT eResult = QMR_OK;

	GAME *pGame = UnitGetGame( pUnit );
	METAGAME *pMetagame = GameGetMetagame( pGame );

	if (pMetagame && pMetagame->pfnMessageHandler)
	{
		eResult = pMetagame->pfnMessageHandler( pUnit, eMessageType, pMessage );		
	}

	// return the result	
	return eResult;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MetagameEventRespawnWarpRequest(
	UNIT* pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );
	ASSERTX_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER, "Expected player" );

	MetagameSendMessageForPlayer( pPlayer, QM_SERVER_RESPAWN_WARP_REQUEST, NULL );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * s_MetagameLevelGetStartPosition(
	UNIT* pPlayer,
	LEVELID idLevel,
	ROOM** pRoom, 
	VECTOR * pPosition, 
	VECTOR * pFaceDirection)
{
	ASSERTX_RETNULL( pRoom && pPosition && pFaceDirection, "Invalid arguments" );
	if ( !pPlayer || !UnitIsPlayer( pPlayer ) )
		return NULL;

	QUEST_MESSAGE_LEVEL_GET_START_LOCATION tMessage;

	tMessage.idLevel = idLevel;
	tMessage.pRoom = NULL;
	
	MetagameSendMessageForPlayer( pPlayer, QM_SERVER_LEVEL_GET_START_LOCATION, &tMessage );

	*pRoom = tMessage.pRoom;
	*pPosition = tMessage.vPosition;
	*pFaceDirection = tMessage.vFaceDirection;
	return *pRoom;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MetagameEventExitLimbo(
	UNIT* pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );

	MetagameSendMessageForPlayer( pPlayer, QM_SERVER_EXIT_LIMBO, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MetagameEventWatpToStartLocation(
	UNIT* pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );

	MetagameSendMessageForPlayer( pPlayer, QM_SERVER_WARP_TO_START_LOCATION, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MetagameEventFellOutOfWorld(
	UNIT* pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );

	MetagameSendMessageForPlayer( pPlayer, QM_SERVER_FELL_OUT_OF_WORLD, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MetagameEventRemoveFromTeam(
	UNIT* pPlayer,
	int nTeam)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );

	QUEST_MESSAGE_REMOVE_FROM_TEAM tData;
	tData.nTeam = nTeam;
	MetagameSendMessageForPlayer( pPlayer, QM_SERVER_REMOVED_FROM_TEAM, &tData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_MetagameEventPlayerRespawn(
	UNIT * pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Invalid arguments" );
	ASSERTX_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER, "Expected player" );
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pPlayer ) ), "Server only" );

	// send message
	MetagameSendMessage( pPlayer, QM_SERVER_PLAYER_RESPAWN, NULL );
}
