//----------------------------------------------------------------------------
// FILE: quest_template.cpp
//
// The quest_template.* files contain code used by multiple "template" quests,
// such as quest_hunt and quest_collect.
// 
// author: Chris March, 1-29-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_template.h"
#include "units.h"
#include "quests.h"
#include "s_quests.h"
#include "objects.h"

//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestTemplateExcelPostProcess(
	EXCEL_TABLE *pTable)
{
	ASSERTX_RETFALSE( pTable, "Expected table" );
#if HELLGATE_ONLY
	// go through each entry
	int nNumEntries = ExcelGetNumRows( NULL, DATATABLE_QUEST_TEMPLATE );
	for (int i = 0; i < nNumEntries; ++i)
	{
		QUEST_TEMPLATE_DEFINITION *pQuestTemplateDef = (QUEST_TEMPLATE_DEFINITION *)ExcelGetData( NULL, DATATABLE_QUEST_TEMPLATE, i );
		QuestTemplateLookupFunctions( pQuestTemplateDef );
	}
#endif
	return TRUE;
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_TEMPLATE_DEFINITION *QuestTemplateGetDefinition(
	int nQuestTemplate)
{
	return (const QUEST_TEMPLATE_DEFINITION *)ExcelGetData( NULL, DATATABLE_QUEST_TEMPLATE, nQuestTemplate );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_TEMPLATE_DEFINITION *QuestTemplateGetDefinition(
	QUEST *pQuest)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERTX_RETNULL( pQuestDef, "Expected quest definition" );
	int nQuestTemplate = pQuestDef->nTemplate;
	if (nQuestTemplate != INVALID_LINK)
	{
		return QuestTemplateGetDefinition( nQuestTemplate );
	}
	return NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestTemplateUpdatePlayerFlagPartyKill(
	QUEST *pQuest,
	UNIT *pPlayer,
	QUEST_STATUS eStatusOld )
{
	if ( QuestIsActive( pQuest ) )
	{
		// quest was activated, flag player to receive a message in the event that
		// a party member kills a monster
		UnitSetFlag( pPlayer, UNITFLAG_QUEST_PARTY_KILL, TRUE );
	}
	else if ( eStatusOld == QUEST_STATUS_ACTIVE )
	{
		// quest was deactivated

		// If no other active quest requires the party kill flag, unset the player unit flag
		BOOL bAnotherQuestRequiresFlag = FALSE;
		for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			QUEST * pQuestTemp = QuestGetQuest( pPlayer, nQuest );
			if ( pQuestTemp && 
				QuestIsActive( pQuestTemp ) && 
				TESTBIT( pQuest->dwQuestFlags, QF_PARTY_KILL ) )
			{
				bAnotherQuestRequiresFlag = TRUE;
				break;
			}
		}

		UnitSetFlag( pPlayer, UNITFLAG_QUEST_PARTY_KILL, bAnotherQuestRequiresFlag );
	}
}

//----------------------------------------------------------------------------
// updates the player unit flag, UNITFLAG_QUEST_PARTY_KILL
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT QuestTemplateMessageStatus( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATUS * pMessageStatus )
{
	if ( TESTBIT( pQuest->dwQuestFlags, QF_PARTY_KILL ) )
	{
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		ASSERTX_RETVAL( pPlayer != NULL, QMR_OK, "Unexpected NULL pPlayer for pQuest" );

		QuestTemplateUpdatePlayerFlagPartyKill( pQuest, pPlayer, pMessageStatus->eStatusOld );
	}
	return QMR_OK;
}

static BOOL sIsLevelDefinitionAllowedForQuestTemplate(
	GAME* pGame,
	UNIT *pPlayer,
	const LEVEL_DEFINITION* pLevelDefinition,
	UNITID idWarpToLevel,
	QUEST* pQuest)
{
	ASSERT_RETFALSE(pLevelDefinition);
	// special case for tutorial level (a not town, not safe level)
	if (pLevelDefinition->bTutorial == TRUE)
	{
		return FALSE;
	}

	// check for hostile areas only		
	if (LevelDefinitionIsSafe( pLevelDefinition ))
	{
		return FALSE;
	}

	// could add a flag if there are cases where this quest should
	// be allowed to warp the player to a non-accessible area
	//	if (pQuestDefinition->bAccessibleAreaOnly == TRUE)
	{
		if (idWarpToLevel != INVALID_ID)
		{
			UNIT* pWarpToLevel = UnitGetById( pGame, idWarpToLevel );
			ASSERTX_RETFALSE( pWarpToLevel, "Unable to get warp unit" );
			if (ObjectCanTrigger( pPlayer, pWarpToLevel ) == FALSE)
			{
				return FALSE;  // warp is restricted
			}
		}
	}

	// might want to say "no" to any level the player has visited

	return TRUE;

}

//----------------------------------------------------------------------------
// Picks a random destination level, unless a level was specified in the 
// excel data which will allow monster spawns.
//----------------------------------------------------------------------------
LEVEL* s_QuestTemplatePickDestinationLevel(
	GAME* pGame,
	UNIT* pPlayer,
	QUEST* pQuest)
{
	LEVEL* pLevel = NULL;

	// first check if a level is specified in the excel data
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );

	int nLevelDef = ptQuestDef->nLevelDefDestinations[ 0 ];
	if (nLevelDef != INVALID_LINK)
	{
		const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( nLevelDef );
		ASSERTX_RETVAL( pLevelDefinition != NULL, NULL, "unexpected NULL LEVEL_DEFINITION for valid link" );
		if ( sIsLevelDefinitionAllowedForQuestTemplate( 
			pGame, pPlayer, pLevelDefinition, INVALID_ID, pQuest ) )
		{
			LEVEL_TYPE tLevelType;
			tLevelType.nLevelDef = nLevelDef;
			pLevel = LevelFindFirstByType( pGame, tLevelType );
			// create the level if it does not already exist?
		}
	}

	if ( pLevel == NULL )
	{
		// no level specified in the excel data, pick an accessible level

		// try the level that the player is in
		pLevel = UnitGetLevel( pPlayer );
		const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( pLevel );

		if ( !sIsLevelDefinitionAllowedForQuestTemplate( 
			pGame, pPlayer, pLevelDefinition, INVALID_ID, pQuest ) )
		{
			LEVEL* pAvailableLevels[ MAX_CONNECTED_LEVELS ];
			int nNumAvailableLevels = 0;

			// get the connected levels to this area
			LEVEL_CONNECTION pConnectedLevels[ MAX_CONNECTED_LEVELS ];
			int nNumConnected = s_LevelGetConnectedLevels( 
				pLevel, 
				pLevel->m_idWarpCreator, 
				pConnectedLevels, 
				MAX_CONNECTED_LEVELS,
				FALSE);

			// create pool of levels that can have this quest in it
			if (nNumConnected > 0)
			{

				for (int i = 0; i < nNumConnected; ++i)
				{
					LEVEL* pLevel = pConnectedLevels[ i ].pLevel;
					UNITID idWarpToLevel = pConnectedLevels[ i ].idWarp;
					pLevelDefinition = LevelDefinitionGet( pLevel );
					if ( sIsLevelDefinitionAllowedForQuestTemplate( 
						pGame, pPlayer, pLevelDefinition, idWarpToLevel, pQuest ) )
					{	
						pAvailableLevels[ nNumAvailableLevels++ ] = pLevel;
					}
				}

			}

			// pick one of the available levels
			if ( nNumAvailableLevels )
			{
				int nPick = RandGetNum( pGame, nNumAvailableLevels );
				pLevel = pAvailableLevels[ nPick ];
			} 
			else
			{
				pLevel = NULL;
			}
		}
	}

	// return level to use (if any)
	return pLevel;
}