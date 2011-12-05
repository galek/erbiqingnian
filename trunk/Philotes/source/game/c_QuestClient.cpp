#include "stdafx.h"
#if !ISVERSION(SERVER_VERSION)
#include "c_QuestClient.h"
#include "states.h"
#include "stringtable.h"
#include "uix.h"
#include "globalindex.h"
#include "c_tasks.h"
#include "stats.h"
#include "inventory.h"
#include "fontcolor.h"
#include "uix_graphic.h"
#include "uix_components.h"
#include "items.h"
#include "QuestProperties.h"
#include "monsters.h"
#include "c_LevelAreasClient.h"
#include "stringreplacement.h"
#include "LevelAreasKnownIterator.h"
#include "QuestObjectSpawn.h"
#include "uix_priv.h"
#include "c_sound.h"		//tugboat
#include "c_sound_util.h"	//tugboat
#include "recipe.h"
#include "c_recipe.h"

const enum QUEST_TEXT_ENUM
{
	KQUEST_TEXT_CLASS_NAME,
	KQUEST_TEXT_PLAYER_NAME,
	KQUEST_TEXT_NPC_NAME,
	KQUEST_TEXT_DICTIONARY,
	KQUEST_TEXT_STATS,
	KQUEST_TEXT_COUNT
};
//----------------------------------------------------------------------------
struct QUEST_TEXT_REPLACEMENT
{
	const WCHAR *puszToken;	
	QUEST_TEXT_ENUM eTextEnum;
	FONTCOLOR eColor;
};

static QUEST_TEXT_REPLACEMENT s_ReplacementTable[] = 
{
	// token				Text Enum		color of replacement
	{ L"CLASS", KQUEST_TEXT_CLASS_NAME, FONTCOLOR_INVALID },
	{ L"PLAYER", KQUEST_TEXT_PLAYER_NAME, FONTCOLOR_INVALID },
	{ L"NPC", KQUEST_TEXT_NPC_NAME, FONTCOLOR_INVALID },	
	{ L"", KQUEST_TEXT_DICTIONARY, FONTCOLOR_INVALID },
	{ L"", KQUEST_TEXT_STATS, FONTCOLOR_INVALID }
};

//returns the text id for the dialog window
inline void c_QuestFillTextDialog( QUEST_FILLTEXT_DATA *pFillTextData )
{
	ASSERT_RETURN( pFillTextData );
	if( !( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_LOG ) ||
		   TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_DIALOGS ) ) )
	{		
		return;
	}	

	ASSERT_RETURN( pFillTextData->pPlayer );
	ASSERT_RETURN( pFillTextData->pQuestTask );
	//add dialog for quest
	int descriptionID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pFillTextData->pPlayer, pFillTextData->pQuestTask, KQUEST_STRING_ACCEPTED ); //KQUEST_STRING_LOG );		
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestDialogGet = pFillTextData->pQuestTask->pPrev;
	while( descriptionID == INVALID_ID &&
		   pQuestDialogGet)
	{
		descriptionID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pFillTextData->pPlayer, pQuestDialogGet, KQUEST_STRING_ACCEPTED );
		if( descriptionID == INVALID_ID )
		{
			//this is probably wrong
			descriptionID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pFillTextData->pPlayer, pQuestDialogGet, KQUEST_STRING_COMPLETE );
		}
		pQuestDialogGet = pQuestDialogGet->pPrev;

	}
	ASSERT_RETURN( descriptionID != INVALID_ID );
	PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_DESCRIPTION ), c_DialogTranslate( descriptionID ) );		

	//add overview
	int smallDescriptionID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pFillTextData->pPlayer, pFillTextData->pQuestTask, KQUEST_STRING_LOG );
	if( smallDescriptionID != INVALID_ID )
	{		
		PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_OVERVIEW ), StringTableGetStringByIndex( smallDescriptionID ) );		
	}
	else
	{
		PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_OVERVIEW ), StringTableGetStringByIndex( smallDescriptionID ) );		
	}	
	
}


void c_QuestsFreeClientLocalValues( UNIT *pPlayer )
{
	
	//make sure we zero this out
	ZeroMemory( QUEST_UI_SHOW_ON_SCREEN, sizeof( BOOL ) * QUEST_UI_SHOW_ON_SCREEN_COUNT );	
	ZeroMemory( QUEST_UI_SHOW_BOSS_DEFEATED, sizeof( BOOL ) * QUEST_UI_SHOW_ON_SCREEN_COUNT * MAX_OBJECTS_TO_SPAWN );	
}
//some bosses aren't marked as completed when you kill them so they may appear again if you come back to the dungeon( for example when you pick up an item they are marked defeated )
void c_QuestClearBossDefeated( int nQuestID )
{
	if( nQuestID == INVALID_ID )
	{
		ZeroMemory( QUEST_UI_SHOW_BOSS_DEFEATED, sizeof( BOOL ) * QUEST_UI_SHOW_ON_SCREEN_COUNT * MAX_OBJECTS_TO_SPAWN);	
		return;
	}
	ASSERT_RETURN( nQuestID >=0 && nQuestID < QUEST_UI_SHOW_ON_SCREEN_COUNT);
	ZeroMemory( QUEST_UI_SHOW_BOSS_DEFEATED[ nQuestID ], sizeof( BOOL ) * MAX_OBJECTS_TO_SPAWN);	
}
//some bosses aren't marked as completed when you kill them so they may appear again if you come back to the dungeon( for example when you pick up an item they are marked defeated )
inline void c_QuestShowSetBossDefeated(  UNIT *pMonster )
{
	if( QuestUnitIsQuestUnit( pMonster ) )
	{

		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask =  QuestUnitGetTask( pMonster );
		
		//make sure the control player has this quest
		UNIT* pPlayer = GameGetControlUnit(AppGetCltGame());
		if(!pPlayer || QuestPlayerHasTask(pPlayer,pQuestTask) == FALSE)
		{
			return;
		}

		ASSERT_RETURN( pQuestTask );
		int nQuestBossIndex = QuestUnitGetIndex( pMonster );
		ASSERT_RETURN( nQuestBossIndex != -1 );
		int nQuestID = pQuestTask->nParentQuest;
		ASSERT_RETURN( nQuestID >=0 && nQuestID < QUEST_UI_SHOW_ON_SCREEN_COUNT);
		ASSERT_RETURN( nQuestBossIndex >=0 && nQuestBossIndex < MAX_OBJECTS_TO_SPAWN);
		QUEST_UI_SHOW_BOSS_DEFEATED[ nQuestID ][ nQuestBossIndex ] = TRUE;
	}
}
//some bosses aren't marked as completed when you kill them so they may appear again if you come back to the dungeon( for example when you pick up an item they are marked defeated )
inline BOOL c_QuestShowIsBossDefeated(  int nQuestID, int nQuestBossIndex )
{
	ASSERT_RETFALSE( nQuestID >=0 && nQuestID < QUEST_UI_SHOW_ON_SCREEN_COUNT);
	ASSERT_RETFALSE( nQuestBossIndex >=0 && nQuestBossIndex < MAX_OBJECTS_TO_SPAWN);
	return QUEST_UI_SHOW_BOSS_DEFEATED[ nQuestID ][nQuestBossIndex];
}


BOOL c_QuestShowsInGame( int nQuestID )
{
	ASSERT_RETFALSE( nQuestID >=0 && nQuestID < QUEST_UI_SHOW_ON_SCREEN_COUNT);
	return QUEST_UI_SHOW_ON_SCREEN[ nQuestID ];
}
BOOL c_QuestShowsInGame( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETFALSE( pQuestTask );	
	return c_QuestShowsInGame( pQuestTask->nParentQuest );
}
void c_QuestSetQuestShowInGame( int nQuestID, BOOL bShow )
{
	ASSERT_RETURN( nQuestID >=0 && nQuestID < QUEST_UI_SHOW_ON_SCREEN_COUNT );
	QUEST_UI_SHOW_ON_SCREEN[ nQuestID ] = bShow;
	
	c_QuestsUpdate( FALSE, FALSE );
}
void c_QuestSetQuestShowInGame( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask, BOOL bShow )
{
	ASSERT_RETURN( pQuestTask );	
	c_QuestSetQuestShowInGame( pQuestTask->nParentQuest, bShow );
}



void c_QuestDisplayQuestUnitInfo( GAME *pGame,
								  UNIT *pItem,
								  EQUEST_TASK_MESSAGES message )
{
	ASSERTX_RETURN( pGame, "No game passed in" );
	ASSERTX_RETURN( pItem, "No item passed in" );
	if( QuestUnitIsQuestUnit( pItem ) == FALSE ) //only quest items
		return;
	c_QuestDisplayMessage( pGame, pItem, QuestUnitGetTask( pItem ), message );
}

void c_QuestHandleQuestTaskMessage( GAME *pGame,
								    MSG_SCMD_QUEST_TASK_STATUS *message )
{
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Attempting to handle quest task message by server" );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	if( !pPlayer &&
		message->nMessage == QUEST_TASK_MSG_QUEST_ITEM_FOUND ) //when the player loads they add a bunch of items....
	{		
			return;
	}
	ASSERTX_RETURN( pPlayer, "Got Quest Message for client before player is out of limbo(c_QuestHandleQuestTaskMessage)" );


	UNIT *pTargetUnit( NULL );
	if( message->nUnitID != INVALID_ID )
	{
		pTargetUnit = UnitGetById( pGame, message->nUnitID );
	}
		
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( message->nTaskIndex );
	c_QuestDisplayMessage( pGame, pTargetUnit, pQuestTask, (EQUEST_TASK_MESSAGES)message->nMessage );
	//const QUEST_DEFINITION_TUGBOAT *pQuest = QuestDefinitionGet( message->nQuestIndex ); 

}

inline void c_QuestAddQuestTaskToMainUI( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask, WCHAR* uszTmp, int nStringLength )
{
	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_QUEST_TASKS_MAIN_MENU);		
	if( !component )
		return;
	if( c_QuestShowsInGame( pQuestTask ) ) //shows in the main menu in game
	{
		PStrPrintf( uszTmp, 
					nStringLength,
					(uszTmp[0] != '\0' )?L"%s\n%s":L"%s%s", 							
					uszTmp, //switch this and the next line to switch position
					GlobalStringGet( GS_QUEST_MAIN_MENU_LAYOUT ));

		c_QuestFillFlavoredText( pQuestTask,
								 uszTmp,
								 nStringLength,
								 QUEST_FLAG_FILLTEXT_TASKS_OBJECTS |
								 QUEST_FLAG_FILLTEXT_TASKS_ITEMS |
								 QUEST_FLAG_FILLTEXT_TASKS_BOSSES |
								 QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES |
								 QUEST_FLAG_FILLTEXT_NO_COLOR ); 
	}
			
}

//updates any variables that might be temp for a given level area
void c_QuestUpdateAfterChangingInstances( int nQuestID )
{
	c_QuestClearBossDefeated( nQuestID );
}

void c_QuestsUpdate( BOOL bUpdateAllObjectsInWorld,
					 BOOL bUpdateAllNPCS )
{
	UNIT *pPlayer = UIGetControlUnit();
	ASSERT_RETURN( pPlayer );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETURN( pGame );
	UI_COMPONENT *component = UIComponentGetByEnum(UICOMP_QUEST_TASKS_MAIN_MENU);		
	WCHAR uszTmp[ 2048 ];
	uszTmp[0] = '\0';		
	if( component )
	{
		UILabelSetText( component->m_idComponent, L"" );
	}
	for( int t = 0; t < MAX_ACTIVE_QUESTS_PER_UNIT; t++ )
	{
		int nQuestID = MYTHOS_QUESTS::QuestGetQuestIDByQuestQueueIndex( pPlayer, t );		
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetActiveTask( pPlayer, nQuestID );
		if( component &&
			pQuestTask )
		{
			c_QuestAddQuestTaskToMainUI( pQuestTask, uszTmp, 2048 );
		}
	}
	if( component )
	{
		UILabelSetText( component->m_idComponent, uszTmp );
	}
	if( bUpdateAllObjectsInWorld ) 
	{
		cLevelUpdateUnitsVisByThemes( UnitGetLevel( pPlayer ) );	
	}
	if( bUpdateAllNPCS )
	{
		QuestUpdateNPCsInLevel( pGame, pPlayer, UnitGetLevel( pPlayer ) );			
	}
	c_QuestCaculatePointsOfInterest( pPlayer );
}

void c_QuestDisplayMessage( GAME *pGame,						    
							UNIT *pTarget,
							const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							EQUEST_TASK_MESSAGES message )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Attempting to handle quest task message by server" );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	const QUEST_DEFINITION_TUGBOAT *pQuest( NULL );
	if( pQuestTask != NULL )
	{
		 pQuest = QuestDefinitionGet( pQuestTask->nParentQuest ); 
	}


	switch( message )
	{
	default:
		return;
	case QUEST_TASK_MSG_OBJECT_OPERATED:
		{
			if( pTarget &&
				pQuestTask )
			{
				int nMessageID = MYTHOS_QUESTS::QuestGetObjectInteractedStringID( pTarget );
				if( nMessageID != INVALID_ID )
				{
					WCHAR szFmt[MAX_XML_STRING_LENGTH];					
					const WCHAR *puszTaskName = StringTableGetStringByIndex( nMessageID );
					PStrCopy( szFmt, puszTaskName, MAX_XML_STRING_LENGTH );
					int nClass = ( pTarget )?UnitGetClass( pTarget ):INVALID_ID;
					c_QuestFillFlavoredText( pQuestTask, nClass, szFmt, MAX_XML_STRING_LENGTH, QUEST_FLAG_FILLTEXT_OBJECT_INTERACTED );
					UIShowTaskCompleteMessage( szFmt );
				}
				QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_OBJECT_OPERATED_CLIENT, pTarget );
			}
		}
		break;
	case QUEST_TASK_MSG_QUEST_ACCEPTED:
		{	
			static int nQuestSound = INVALID_LINK;
			if (nQuestSound == INVALID_LINK)
			{
				nQuestSound = GlobalIndexGet( GI_SOUND_QUEST );
			}

			if (nQuestSound != INVALID_LINK)
			{
				UNIT *pPlayer = GameGetControlUnit( pGame );
				c_SoundPlay(nQuestSound, &c_UnitGetPosition(pPlayer), NULL);
			}
			QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_START, pTarget );
		}
		break;
	case QUEST_TASK_MSG_QUEST_ITEM_GIVEN_ON_ACCEPT:
		{			
			if( pQuestTask )
			{
				WCHAR szFmt[MAX_XML_STRING_LENGTH];				
				const WCHAR *puszTaskName = GlobalStringGet( GS_QUEST_ITEM_RECEIVED_ON_ACCEPT );
				PStrCopy( szFmt, puszTaskName, MAX_XML_STRING_LENGTH );
				int nClass = ( pTarget )?UnitGetClass( pTarget ):INVALID_ID;
				c_QuestFillFlavoredText( pQuestTask, nClass, szFmt, MAX_XML_STRING_LENGTH, QUEST_FLAG_FILLTEXT_ITEMS_GIVEN_ON_ACCEPT );
				UIShowTaskCompleteMessage( szFmt );
			}
			//client			
			
		}
		break;

	case QUEST_TASK_MSG_QUEST_ITEM_GIVEN_ON_COMPLETE:
		{			
			if( pQuestTask )
			{
				WCHAR szFmt[MAX_XML_STRING_LENGTH];				
				const WCHAR *puszTaskName = GlobalStringGet( GS_QUEST_ITEM_RECEIVED_ON_COMPLETE );
				PStrCopy( szFmt, puszTaskName, MAX_XML_STRING_LENGTH );
				for (int i = 0; i < MAX_QUEST_REWARDS; i++) 
				{		
					//make sure it's valid, if not we aren't awarding any more
					if (pQuestTask->nQuestRewardItemID[ i ] != INVALID_ID )		
					{		
						if( i != 0 )
						{							
							PStrPrintf( szFmt, MAX_XML_STRING_LENGTH, L"%s\n%s", szFmt, puszTaskName );
						}

						c_QuestFillFlavoredText( pQuestTask, pQuestTask->nQuestRewardItemID[ i ], szFmt, MAX_XML_STRING_LENGTH, QUEST_FLAG_FILLTEXT_ITEMS_GIVEN_ON_COMPLETE );
					}
				}
				UIShowTaskCompleteMessage( szFmt );
			}
			//client			
			
		}
		break;
	case QUEST_TASK_MSG_QUEST_ABANDON:
		{
			c_TBRefreshTasks( 0 );
			c_StateSet( pPlayer, pPlayer, STATE_QUEST_ABONDON, 0, 5, INVALID_ID );
			if( pQuestTask )
			{
				c_QuestSetQuestShowInGame( pQuestTask, FALSE );
				WCHAR szFmt[MAX_XML_STRING_LENGTH];
				const WCHAR *puszTaskName = GlobalStringGet( GS_QUEST_HAS_BEEN_ABANDONED );
				PStrCopy( szFmt, puszTaskName, MAX_XML_STRING_LENGTH );
				c_QuestFillFlavoredText( pQuestTask, INVALID_ID, szFmt, MAX_XML_STRING_LENGTH, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES );
				UIShowTaskCompleteMessage( szFmt );
				UI_TB_HideModalDialogs();
			}
			QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_ABANDONED_CLIENT, pTarget );
			
		}
		break;
	case QUEST_TASK_MSG_INVALID:
		return;
	case QUEST_TASK_MSG_QUEST_ITEM_FOUND:
		{			
			c_StateSet( pPlayer, pPlayer, STATE_QUEST_SUBTASK_DONE, 0, 5, INVALID_ID );
			if( pQuestTask )
			{
				WCHAR szFmt[MAX_XML_STRING_LENGTH];
				const WCHAR *puszTaskName = GlobalStringGet( GS_QUEST_ITEM_HAS_BEEN_FOUND );
				PStrCopy( szFmt, puszTaskName, MAX_XML_STRING_LENGTH );
				c_QuestFillFlavoredText( pQuestTask, pTarget, szFmt, MAX_XML_STRING_LENGTH, QUEST_FLAG_FILLTEXT_QUEST_ITEM | QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES );
				UIShowTaskCompleteMessage( szFmt );
			}
			//client			
		}

		break;
	case QUEST_TASK_MSG_LEVEL_BEING_REMOVED:
			c_QuestUpdateAfterChangingInstances();
		break;
	case QUEST_TASK_MSG_BOSS_DEFEATED:
			c_QuestShowSetBossDefeated( pTarget );
			QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_BOSS_DEFEATED_CLIENT, pTarget );
		break;
	case QUEST_TASK_MSG_TASKS_COMPLETE:
		{
			c_StateSet( pPlayer, pPlayer, STATE_QUEST_TASK_COMPLETE, 0, 5, INVALID_ID );			
			if( pQuestTask )
			{
				
				const WCHAR *puszTaskName = NULL;
				int nMsgID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask, KQUEST_STRING_TASK_COMPLETE_MSG );
				if( nMsgID == INVALID_LINK )
				{
					puszTaskName = GlobalStringGet( GS_QUEST_COMPLETE_CLIENT_MESSAGE );					
				}
				else if( nMsgID != INVALID_LINK )
				{
					puszTaskName = StringTableGetStringByIndex( nMsgID );
				}				
				if( puszTaskName )
				{
					WCHAR realMessage[ 255 ];
					ZeroMemory( realMessage, 255 );		
					PStrCopy( realMessage, puszTaskName, 255 );
					c_QuestFillFlavoredText( pQuest->pFirst, INVALID_ID, &realMessage[0], 255, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES );
					UIShowTaskCompleteMessage( realMessage );
				}
			}			
			//client			
			QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_COMPLETE_CLIENT, pTarget );			
			
		}
		break;
	case QUEST_TASK_MSG_COMPLETE:
		{
			//client
			QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_COMPLETE_CLIENT, pTarget );
			if( pQuestTask )
			{
				c_QuestUpdateAfterChangingInstances( pQuestTask->nParentQuest );
			}
		}
		break;
	case QUEST_TASK_MSG_QUEST_COMPLETE:
		{
			
			c_StateSet( pPlayer, pPlayer, STATE_QUEST_COMPLETE_TUGBOAT, 0, 5, INVALID_ID );
			if( pQuest )
			{
				c_QuestUpdateAfterChangingInstances( pQuestTask->nParentQuest );
				const WCHAR *puszTaskName = NULL;
				int nMsgID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTask, KQUEST_STRING_QUEST_COMPLETE_MSG );
				if( nMsgID == INVALID_LINK )
				{
					puszTaskName = GlobalStringGet( GS_QUEST_COMPLETE_CLIENT_MESSAGE );					
				}
				else if( nMsgID != INVALID_LINK )
				{
					puszTaskName = StringTableGetStringByIndex( nMsgID );
				}
				if( puszTaskName &&
					pQuest->pFirst)
				{
					WCHAR realMessage[ MAX_STRING_ENTRY_LENGTH ];
					ZeroMemory( realMessage, MAX_STRING_ENTRY_LENGTH );		
					PStrCopy( realMessage, puszTaskName, 255 );
					c_QuestFillFlavoredText( pQuest->pFirst, INVALID_ID, &realMessage[0], MAX_XML_STRING_LENGTH, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES );
					UIShowTaskCompleteMessage( realMessage );					
				}

			}
			//client
			QuestUpdateWorld( pGame, pPlayer, UnitGetLevel( pPlayer ), pQuestTask, QUEST_WORLD_UPDATE_QSTATE_COMPLETE_CLIENT, pTarget );			
		}
		break;
	case QUEST_TASK_MSG_UI_INVENTORY:
		UIShowPaneMenu( KPaneMenuEquipment, KPaneMenuBackpack );
		break;
	}
	UITasksRefresh();
	
#endif
}

inline void c_QuestFillTextWithArrayOfItems( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
											 const int *nItemArrays,	//items
											 const int nItemArraySize,	//array size
											 const WCHAR* replaceTextPerLine,	//text that should include a [questitem]
											 const WCHAR* replaceText,	//text that will be replaced inside the szFlavor ( will replace with list of items )										 
											 WCHAR* szFlavor,
											 int len )
{
	ASSERTX_RETURN( pQuestTask, "Quest Task needed" );
	int nItemsRecieved( 0 );
	WCHAR szList[MAX_XML_STRING_LENGTH];				
	ZeroMemory( szList, sizeof( WCHAR ) * MAX_XML_STRING_LENGTH );

	for( int i = 0; i < nItemArraySize; i++ )		
	{
		if( nItemArrays[ i ] == INVALID_ID )
			break;							
		nItemsRecieved++;			
	}
	if( nItemsRecieved > 0 )
	{
		for( int i = 0; i < nItemsRecieved; i++ )		
		{
			const UNIT_DATA *pItem = (const UNIT_DATA *)ExcelGetData( NULL, DATATABLE_ITEMS, nItemArrays[ i ] );
			if( pItem )
			{
				PStrCat( szList, replaceTextPerLine, MAX_XML_STRING_LENGTH ); //add the line on
				PStrReplaceToken( szList, MAX_XML_STRING_LENGTH, StringReplacementTokensGet( SR_ITEM ), StringTableGetStringByIndex( pItem->nString ) ); //replace the string								
				if( i != nItemsRecieved - 1 )
				{
					PStrCat( szList, L"\n", MAX_XML_STRING_LENGTH ); //tack on a \n for next line
				}
			}

		}		
	}
	for( int i = 0; i < MAX_ITEMS_TO_GIVE_ACCEPTING_TASK; i++ )
	{
		int nRecipe = MYTHOS_QUESTS::QuestGetRecipeIDOnTaskAcceptedByIndex( UIGetControlUnit(), pQuestTask, i );
		if( nRecipe == INVALID_ID )
			break;
		const RECIPE_DEFINITION *pRecipe = RecipeGetDefinition( nRecipe );
		ASSERT_BREAK( pRecipe );
		PStrCat( szList, replaceTextPerLine, MAX_XML_STRING_LENGTH ); //add the line on
		WCHAR szTmpList[MAX_XML_STRING_LENGTH];				
		ZeroMemory( szTmpList, sizeof( WCHAR ) * MAX_XML_STRING_LENGTH );									
		PStrCat( szTmpList, StringReplacementTokensGet( SR_RECIPE_NAME ), MAX_XML_STRING_LENGTH ); //replace the string								
		PStrCat( szTmpList, StringReplacementTokensGet( SR_RECIPE_LVL ), MAX_XML_STRING_LENGTH ); //replace the string								
		c_RecipeInsertName( nRecipe, szTmpList, MAX_XML_STRING_LENGTH );		
		PStrReplaceToken( szList, MAX_XML_STRING_LENGTH, StringReplacementTokensGet( SR_ITEM ), szTmpList ); //replace the string								
		if( i != nItemsRecieved - 1 )
		{
			PStrCat( szList, L"\n", MAX_XML_STRING_LENGTH ); //tack on a \n for next line
		}

	}

	PStrReplaceToken( szFlavor, len, replaceText, szList ); //replace the string in the orginal message

}

//fills out the items you collected
inline void c_QuestFillInItemCollection( QUEST_FILLTEXT_DATA *pFillTextData )
{
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_TASKS_ITEMS ) == FALSE )
		return;
	
	WCHAR *szFlavor = new WCHAR[ pFillTextData->nTextLen ];
	ZeroMemory( szFlavor, sizeof( WCHAR ) * pFillTextData->nTextLen );
	BOOL bFirst( TRUE );
	static const int nTempCharArraySize( 255 );
	WCHAR uszTempCharArray[ nTempCharArraySize ];	
	WCHAR uszTempCharArray2[ nTempCharArraySize ];	
	ZeroMemory( uszTempCharArray, nTempCharArraySize );
	ZeroMemory( uszTempCharArray2, nTempCharArraySize );
	DWORD dwInvIterateFlags = 0;
	SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );
	for( int nIndex = 0; nIndex < MAX_COLLECT_PER_QUEST_TASK; nIndex++ )
	{
				
		if ( MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex ) > 0 )
		{			
			int nItemCount =  MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
			int nItemClass =  MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
			if( nItemCount > 0 && nItemClass != INVALID_ID )
			{
				
				PStrPrintf( szFlavor, 
							pFillTextData->nTextLen,
							(!bFirst)?L"%s\n%s":L"%s%s", 							
							szFlavor, //switch this and the next line to switch position
							GlobalStringGet( GS_QUEST_ITEMS_FOUND_QUEST_LOG ));

				bFirst = FALSE;
				UNIT_DATA *itemData = UnitGetData( UnitGetGame( pFillTextData->pPlayer ), GENUS_ITEM, nItemClass );
				ASSERT_CONTINUE( itemData );				
				int nItemsNeeded = MYTHOS_QUESTS::QuestGetNumberToCollectForItem( pFillTextData->pPlayer, pFillTextData->pQuestTask, nItemClass );	
				DWORD dwAttributesToMatch = StringTableGetGrammarNumberAttribute( nItemsNeeded > 1 ? PLURAL : SINGULAR );				
				const WCHAR *puszUnitName = StringTableGetStringByIndexVerbose( itemData->nString, dwAttributesToMatch, 0, NULL, NULL );

				PStrReplaceToken( szFlavor, 
								  pFillTextData->nTextLen, 
								  StringReplacementTokensGet( SR_ITEM ), 
								  puszUnitName);
				
				int count = UnitInventoryCountUnitsOfType( pFillTextData->pPlayer, MAKE_SPECIES( GENUS_ITEM, nItemClass ), dwInvIterateFlags); 							
				PStrPrintf( uszTempCharArray, nTempCharArraySize, L"%d/%d", count, nItemCount );
				ZeroMemory( uszTempCharArray2, nTempCharArraySize );
				UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
				PStrReplaceToken( szFlavor, 
								  pFillTextData->nTextLen, 
								  StringReplacementTokensGet( SR_QUEST_ITEMS_FOUND ), 
								  uszTempCharArray2 );


			}			
		}
		else
		{
			break;
		}
	}
	
	PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_ITEMCOLLECTION ), szFlavor );		
	delete[] szFlavor;
}

//fills in boss killings
inline void c_QuestFillInBossSlayings( QUEST_FILLTEXT_DATA *pFillTextData )
{
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_TASKS_BOSSES ) == FALSE )
		return;
	
	WCHAR *szFlavor = new WCHAR[ pFillTextData->nTextLen ];
	ZeroMemory( szFlavor, sizeof( WCHAR ) * pFillTextData->nTextLen );
	BOOL bFirst( TRUE );
	static const int nTempCharArraySize( 255 );
	WCHAR uszTempCharArray[ nTempCharArraySize ];	
	int nBossIDs[MAX_OBJECTS_TO_SPAWN];
	int nBossNameIDs[ MAX_OBJECTS_TO_SPAWN ];
	int nBossCount[ MAX_OBJECTS_TO_SPAWN ];
	int nBossDefeated[ MAX_OBJECTS_TO_SPAWN ];
	int nBossPrefixNameID[ MAX_OBJECTS_TO_SPAWN ];
	
	ZeroMemory( nBossIDs, sizeof(int) * MAX_OBJECTS_TO_SPAWN );
	ZeroMemory( nBossCount, sizeof(int) * MAX_OBJECTS_TO_SPAWN );
	ZeroMemory( nBossDefeated, sizeof(int) * MAX_OBJECTS_TO_SPAWN );
	ZeroMemory( nBossPrefixNameID, sizeof(int) * MAX_OBJECTS_TO_SPAWN );

	int nUniqueBossCount( 0 );
	for( int nIndex = 0; nIndex < MAX_OBJECTS_TO_SPAWN; nIndex++ )
	{
		int nBossID = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
		BOOL bShows = MYTHOS_QUESTS::QuestBossShowsOnTaskScreen( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
		int nBossNameID = MYTHOS_QUESTS::QuestGetBossUniqueNameByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );		
		if ( bShows && nBossID != INVALID_ID )
		{			
			BOOL bFound( FALSE );
			int nRefIndex(0);
			for( nRefIndex = 0; nRefIndex < nUniqueBossCount; nRefIndex++ )			
				if( nBossIDs[ nRefIndex ] == nBossID )
				{					
					bFound = (nBossNameIDs[nRefIndex] == nBossNameID );
					if( !bFound &&
						isQuestRandom( pFillTextData->pQuestTask ) == FALSE )
					{
						int nLength = PStrLen( StringTableGetStringByIndex( nBossNameIDs[nRefIndex] ) );
						if( PStrCmp( StringTableGetStringByIndex( nBossNameIDs[nRefIndex] ), StringTableGetStringByIndex( nBossNameID ), nLength ) == 0 )
						{
							bFound = TRUE;
						}
					}					
					if( bFound )
						break;
				}
			if( !bFound )
			{		
				nRefIndex = nUniqueBossCount;
				nUniqueBossCount++;				
			}
			nBossNameIDs[ nRefIndex ] = nBossNameID;
			nBossIDs[ nRefIndex ] = nBossID;
			nBossCount[ nRefIndex ]++;
			nBossDefeated[ nRefIndex ] += ( s_IsTaskObjectCompleted( UnitGetGame( pFillTextData->pPlayer ),
										 pFillTextData->pPlayer,
										 pFillTextData->pQuestTask,
										 nIndex,
										 TRUE,
										 FALSE ) )?1:0;
			if( nBossDefeated[ nRefIndex ] == FALSE )
			{
				nBossDefeated[ nRefIndex ] = c_QuestShowIsBossDefeated( pFillTextData->pQuest->nID, nRefIndex );
			}
		}
	}


	for( int nIndex = 0; nIndex < nUniqueBossCount; nIndex++ )
	{		
		BOOL bMulti = ( nBossCount[ nIndex ] > 1 );
		BOOL bComplete = ( nBossDefeated[ nIndex ] == nBossCount[ nIndex ]);
		PStrPrintf( szFlavor, 
					pFillTextData->nTextLen,
					(!bFirst)?L"%s\n%s":L"%s%s", 							
					szFlavor, //switch this and the next line to switch position
					( !bMulti )?GlobalStringGet( GS_QUEST_BOSS_MONSTERS_TO_SLAY ):GlobalStringGet( GS_QUEST_BOSS_MONSTERS_TO_SLAY_COUNT ));
		bFirst = FALSE;
		//Boss name SR_BOSS_NAME
		int nBossPrefixId = nBossNameIDs[ nIndex ];
		if( nBossPrefixId != INVALID_ID )
		{
			int nBossClassId = nBossIDs[ nIndex ];
			if( isQuestRandom( pFillTextData->pQuestTask ) )
			{
				ZeroMemory( uszTempCharArray, nTempCharArraySize );
				MonsterGetUniqueNameWithTitle( nBossPrefixId, nBossClassId,  uszTempCharArray, nTempCharArraySize );			
				PStrReplaceToken( szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_BOSS_NAME ), uszTempCharArray );
			}
			else
			{
				
				PStrReplaceToken( szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_BOSS_NAME ), StringTableGetStringByIndex( nBossPrefixId ) );
			}
		}	
		ZeroMemory( uszTempCharArray, nTempCharArraySize );
		if( bMulti )
		{
			
			WCHAR szText[ 4 ];
			PStrPrintf( szText, 4, L"%i", nBossDefeated[ nIndex ] );
			uszTempCharArray[0] = '\0';
			UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, szText );				
			PStrReplaceToken( szFlavor, 
							  pFillTextData->nTextLen, 
							  StringReplacementTokensGet( SR_QUEST_BOSSES_DEFEATED ), 
							  uszTempCharArray );
			PStrPrintf( szText, 4, L"%i", nBossCount[ nIndex ] );
			uszTempCharArray[0] = '\0';
			UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, szText );				
			PStrReplaceToken( szFlavor, 
							  pFillTextData->nTextLen, 
							  StringReplacementTokensGet( SR_QUEST_BOSSES_TOTAL ), 
							  uszTempCharArray );
			
		}else if( bComplete )
		{				          
			UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, GlobalStringGet( GS_QUEST_BOSS_STATUS_DEFEATED ) );				
			PStrReplaceToken( szFlavor, 
							  pFillTextData->nTextLen, 
							  StringReplacementTokensGet( SR_QUEST_BOSS_STATUS ), 
							  uszTempCharArray );
		}
		else
		{
			UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, GlobalStringGet( GS_QUEST_BOSS_STATUS_ALIVE ) );
			PStrReplaceToken( szFlavor, 
							  pFillTextData->nTextLen, 
							  StringReplacementTokensGet( SR_QUEST_BOSS_STATUS ), 
							  uszTempCharArray );
		}

	}
	PStrCat( szFlavor, L"\n", pFillTextData->nTextLen );	
	PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_MONSTERSLAYINGS ), szFlavor );		
	delete[] szFlavor;
}


//fills in boss killings
inline void c_QuestFillInObjectInteractions( QUEST_FILLTEXT_DATA *pFillTextData )
{
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_TASKS_OBJECTS ) == FALSE )
		return;
	
	WCHAR *szFlavor = new WCHAR[ pFillTextData->nTextLen ];
	ZeroMemory( szFlavor, sizeof( WCHAR ) * pFillTextData->nTextLen );
	BOOL bFirst( TRUE );
	int nUniqueObjectTypes[ MAX_OBJECTS_TO_SPAWN ];
	int nUniqueObjectTypesComplete[ MAX_OBJECTS_TO_SPAWN ];
	int nUniqueObjectTypesCount[ MAX_OBJECTS_TO_SPAWN ];
	int nUniqueObjectTypesIndex[ MAX_OBJECTS_TO_SPAWN ];
	ZeroMemory( nUniqueObjectTypesCount, sizeof( int ) * MAX_OBJECTS_TO_SPAWN );
	ZeroMemory( nUniqueObjectTypesComplete, sizeof( int ) * MAX_OBJECTS_TO_SPAWN );
	int nUniqueObjectCount( 0 );
	
	for( int nIndex = 0; nIndex < MAX_OBJECTS_TO_SPAWN; nIndex++ )
	{
		int nObjectID = MYTHOS_QUESTS::QuestGetObjectIDToSpawnByIndex( pFillTextData->pQuestTask, nIndex );
		if ( nObjectID != INVALID_ID )
		{
			int nObjectIndex( 0 );
			for( nObjectIndex = 0; nObjectIndex < nUniqueObjectCount; nObjectIndex++ )
			{
				if( nUniqueObjectTypes[nObjectIndex] == nObjectID )
				{
					break;
				}
			}
			if( nObjectIndex >= 0 )
			{
				nUniqueObjectCount += (nUniqueObjectTypesCount[nObjectIndex]==0)?1:0;
				if( nUniqueObjectCount == 1 )
				{
					nUniqueObjectTypesIndex[nObjectIndex] = nIndex;				
				}
				nUniqueObjectTypes[ nObjectIndex ] = nObjectID;
				nUniqueObjectTypesCount[nObjectIndex]++;
				
				nUniqueObjectTypesComplete[nObjectIndex] += (s_IsTaskObjectCompleted( UnitGetGame( pFillTextData->pPlayer ),
																		 pFillTextData->pPlayer,
																		 pFillTextData->pQuestTask,
																		 nIndex,
																		 FALSE,
																		 FALSE ))?1:0;
				
			}
		}
	}
	for( int nIndex = 0; nIndex < nUniqueObjectCount; nIndex++ )
	{
		int nObjectIndex = nUniqueObjectTypesIndex[nIndex];		
		int nObjectDescriptionID = MYTHOS_QUESTS::QuestGetObjectInteractedStringID( pFillTextData->pQuestTask, nObjectIndex );
		if( nObjectDescriptionID != INVALID_ID )
		{
			PStrPrintf( szFlavor, 
						pFillTextData->nTextLen,
						(!bFirst)?L"%s\n%s":L"%s%s", 							
						szFlavor, //switch this and the next line to switch position
						GlobalStringGet( GS_QUEST_OBJECT_INTERACT ));
			bFirst = FALSE;			
			PStrReplaceToken( szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_OBJECT_DESCRIPTION ), StringTableGetStringByIndex( nObjectDescriptionID ) );
			static const int nTempCharArraySize( 255 );
			WCHAR uszTempCharArray[ nTempCharArraySize ];	
			WCHAR uszTempCharArray2[ nTempCharArraySize ];	
			ZeroMemory( uszTempCharArray, nTempCharArraySize );
			ZeroMemory( uszTempCharArray2, nTempCharArraySize );
			PStrPrintf( uszTempCharArray, 
						nTempCharArraySize,
						L"%i/%i", 							
						max( 0, nUniqueObjectTypesComplete[nIndex] ),
						max( 1, nUniqueObjectTypesCount[nIndex] ) );
			UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
			PStrReplaceToken( szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_OBJECT_INTERACTED ), uszTempCharArray2 );		
		}

	}
	
	PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_OBJECT_INTERACTED_LIST ), szFlavor );		
	delete[] szFlavor;
}



void c_QuestFillRandomQuestTagsUIDialogText( UI_COMPONENT *pUITextComponent,
											 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETURN( pQuestTask );
	ASSERT_RETURN( pUITextComponent );
	const WCHAR *oldText = UILabelGetText( pUITextComponent );
	WCHAR newText[ MAX_STRING_ENTRY_LENGTH ];
	int nStringLength = PStrLen( oldText );
	ASSERT_RETURN( MAX_STRING_ENTRY_LENGTH >= nStringLength );
	PStrCopy( newText, MAX_STRING_ENTRY_LENGTH, oldText, nStringLength + 1 );
	//not sure why but grabbing the text from the UI label has certain characters inserted we need removed.
	/*
	//DOn't need any longer - something got fixed some other place
	for( int t = 0; t < nStringLength - 1; t++ )
	{
		if( newText[ t ] == 10 &&
			newText[ t + 1 ] != 10)
		{
			newText[ t ] = 32;
		}
	}
	*/
	c_QuestFillFlavoredText( pQuestTask, newText, MAX_STRING_ENTRY_LENGTH, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES );	
	UILabelSetText( pUITextComponent, newText );
}

inline int c_QuestGetQuestItemNameID( QUEST_FILLTEXT_DATA *pFillTextData )
{
	ASSERT_RETINVALID( pFillTextData );
	ASSERT_RETINVALID( pFillTextData->pUnit );
	if( !QuestUnitIsQuestUnit( pFillTextData->pUnit ) )
		return INVALID_ID;
	int classID( UnitGetClass( pFillTextData->pUnit ) );
	int nGenus( UnitGetGenus( pFillTextData->pUnit ) );

	//walk the items to collect
	if( nGenus == GENUS_ITEM )
	{
		for( int c = 0; c < MAX_COLLECT_PER_QUEST_TASK; c++ )
		{
			if ( classID ==  MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, c ))
			{
				if( pFillTextData->pQuestTask->nQuestItemsFlavoredTextToCollect[ c ] != INVALID_LINK )
				{
					return pFillTextData->pQuestTask->nQuestItemsFlavoredTextToCollect[ c ];
				}
				else
				{
					return pFillTextData->pUnit->pUnitData->nString;
				}
			}
		}	
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskNext = pFillTextData->pQuestTask->pNext;
		if( pQuestTaskNext )
		{
			for( int c = 0; c < MAX_COLLECT_PER_QUEST_TASK; c++ )
			{
				if ( classID ==  MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pFillTextData->pPlayer, pQuestTaskNext, c ))
				{
					if( pQuestTaskNext->nQuestItemsFlavoredTextToCollect[ c ] != INVALID_LINK )
					{
						return pQuestTaskNext->nQuestItemsFlavoredTextToCollect[ c ];
					}
					else
					{
						return pFillTextData->pUnit->pUnitData->nString;
					}
				}
			}	
		}
	}
	//walk the boss monsters spawned from objects in the level
	if( nGenus == GENUS_MONSTER )
	{
		for( int c = 0; c < MAX_OBJECTS_TO_SPAWN; c++ )
		{
			if ( classID == MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, c ) )
			{
				if( MYTHOS_QUESTS::QuestGetBossItemFlavorIDByIndex( pFillTextData->pQuestTask, c ) != INVALID_LINK )
				{
					return MYTHOS_QUESTS::QuestGetBossItemFlavorIDByIndex( pFillTextData->pQuestTask, c );
				}
				else
				{
					return pFillTextData->pUnit->pUnitData->nString;
				}
			}
		}
	}
	//walk the boss monsters spawned from objects in the level
	if( nGenus == GENUS_OBJECT )
	{
		for( int c = 0; c < MAX_OBJECTS_TO_SPAWN; c++ )
		{
			if ( classID == MYTHOS_QUESTS::QuestGetObjectSpawnItemToSpawnByIndex( pFillTextData->pQuestTask, c ) )				 
			{
				if( pFillTextData->pQuestTask->nQuestObjectCreateItemFlavoredText[ c ] != INVALID_LINK )
				{
					return pFillTextData->pQuestTask->nQuestObjectCreateItemFlavoredText[ c ];
				}
				else
				{
					return pFillTextData->pUnit->pUnitData->nString;
				}
			}
		}
	}
	return INVALID_ID;
}

inline void c_QuestReplaceActualTags( QUEST_FILLTEXT_DATA *pFillTextData )
{
	ASSERT_RETURN( pFillTextData->pQuestTask );
	ASSERT_RETURN( pFillTextData->pPlayer );	
	ASSERT_RETURN( pFillTextData->pQuest );	
	static const int nTempCharArraySize( 255 );
	WCHAR uszTempCharArray[ nTempCharArraySize ];	
	WCHAR uszTempCharArray2[ nTempCharArraySize ];	
	ZeroMemory( uszTempCharArray, nTempCharArraySize );
	ZeroMemory( uszTempCharArray2, nTempCharArraySize );

	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ) )
	{
		for( int t = 0; t < MAX_COLLECT_PER_QUEST_TASK; t++ )
		{
			int nUnitTypeToKillForItem = MYTHOS_QUESTS::QuestGetItemSpawnFromUnitTypeByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, t );
			if( nUnitTypeToKillForItem != INVALID_ID )
			{
				const UNITTYPE_DATA* unittype_data = (UNITTYPE_DATA*)ExcelGetData(NULL, DATATABLE_UNITTYPES, nUnitTypeToKillForItem);
				if (unittype_data != NULL )
				{
					int nString = unittype_data->nDisplayName;
					if ( nString != INVALID_LINK)
					{
						// get the attributes to match for number
						DWORD dwAttributesToMatch = StringTableGetGrammarNumberAttribute( ((nUnitTypeToKillForItem > 1) ? PLURAL : SINGULAR) );
						const WCHAR * str = StringTableGetStringByIndexVerbose(nString, dwAttributesToMatch, t, NULL, NULL );
						STRING_REPLACEMENT nStringReplace = (STRING_REPLACEMENT)(SR_QUEST_ITEMDROP_FROM_UNIT1 + t );
						ASSERTX_CONTINUE( nStringReplace <= SR_QUEST_ITEMDROP_FROM_UNIT6, "More quest strings for items are needed( maxed at 6 )" );

						if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
						{
							PStrReplaceToken(  pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), str );
						}
						else
						{							
							ZeroMemory( uszTempCharArray, nTempCharArraySize );
							UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, str );
							PStrReplaceToken(  pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), uszTempCharArray );
						}
					}
				}
			}
		}
	}

	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ) )
	{
		//Boss name SR_QUEST_BOSS1..SR_QUEST_BOSS16
		for( int nIndex = 0; nIndex < MAX_OBJECTS_TO_SPAWN; nIndex++ )
		{
			int nBossPrefixId = MYTHOS_QUESTS::QuestGetBossUniqueNameByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
			int nBossClassId = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
			if( nBossClassId != INVALID_ID ) //not all quets are for killing bosses
			{
				STRING_REPLACEMENT nStringReplace = (STRING_REPLACEMENT)(SR_QUEST_BOSS1 + nIndex );
				ASSERTX_CONTINUE( nStringReplace <= SR_QUEST_BOSS16, "More quest strings for items are needed( maxed at 6 )" );

				if( isQuestRandom( pFillTextData->pQuest ) )
				{
					MonsterGetUniqueNameWithTitle( nBossPrefixId, nBossClassId,  uszTempCharArray, nTempCharArraySize );
				}

				if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
				{
					PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), (isQuestRandom( pFillTextData->pQuest)?uszTempCharArray:StringTableGetStringByIndex(nBossPrefixId) ) );
				}
				else
				{
					UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
					PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), (isQuestRandom( pFillTextData->pQuest)?uszTempCharArray2:StringTableGetStringByIndex(nBossPrefixId) ) );
				}
			}
		}

	}
	
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ) )
	{
		//Title of quest SR_QUEST_TITLE
		int nTitle = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pFillTextData->pPlayer, pFillTextData->pQuestTask, KQUEST_STRING_TITLE );
		if( nTitle != INVALID_ID )
		{
			const WCHAR *questTitle = StringTableGetStringByIndex( nTitle );
			if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
			{
				PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_TITLE ), questTitle );
			}
			else
			{
				ZeroMemory( uszTempCharArray, nTempCharArraySize );
				UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, questTitle );
				PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_TITLE ), uszTempCharArray );
			}
		}
	}
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ) )
	{
		//Level area name SR_QUEST_DUNGEON_NAME
		int nLevelAreaID = MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, 0 );
		if( nLevelAreaID != INVALID_ID )
		{
			// get name
			ZeroMemory( uszTempCharArray, nTempCharArraySize );			
			MYTHOS_LEVELAREAS::c_GetLevelAreaLevelName( nLevelAreaID, 0, uszTempCharArray, nTempCharArraySize, TRUE );						
			if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
			{
				PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_DUNGEON_NAME ), uszTempCharArray );
			}
			else
			{			
				ZeroMemory( uszTempCharArray2, nTempCharArraySize );
				UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
				PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_DUNGEON_NAME ), uszTempCharArray2 );
			}
		}
	}
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ) )
	{
		//fill in NPC name for task SR_QUEST_NPC
		if( pFillTextData->pQuestTask->nNPC != INVALID_ID )
		{
			UNIT_DATA *pNPC = UnitGetData( UnitGetGame( pFillTextData->pPlayer ), GENUS_MONSTER, pFillTextData->pQuestTask->nNPC );
			if( pNPC )
			{
				// get name
				const WCHAR *puszMonsterName = StringTableGetStringByIndex( pNPC->nString );							
				if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
				{
					PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_NPC ), puszMonsterName );
				}
				else
				{
					ZeroMemory( uszTempCharArray, nTempCharArraySize );
					UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, puszMonsterName );
					PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_NPC ), uszTempCharArray );
				}
			}
		}
	}

	if( pFillTextData->pUnit &&
		QuestUnitIsQuestUnit( pFillTextData->pUnit ) &&
		TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_ITEM ) )
	{
		//fill in the name for quest items		
		int nNameString = c_QuestGetQuestItemNameID( pFillTextData );
		if( nNameString != INVALID_ID )
		{
			const WCHAR *puszUnitName = StringTableGetStringByIndex( nNameString );							
			if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
			{
				PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_UNIT ), puszUnitName );
			}
			else
			{
				ZeroMemory( uszTempCharArray, nTempCharArraySize );
				UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, puszUnitName );
				PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_UNIT ), uszTempCharArray );
			}
		}
		//SR_QUEST_UNIT_COUNT
		DWORD dwInvIterateFlags = 0;
		SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
		SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );			
		int count = UnitInventoryCountUnitsOfType( pFillTextData->pPlayer, UnitGetSpecies( pFillTextData->pUnit ), dwInvIterateFlags);
		PStrPrintf( uszTempCharArray, nTempCharArraySize, L"%d", count );
		if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
		{
			PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_UNIT_COUNT ), uszTempCharArray );
		}
		else
		{
			ZeroMemory( uszTempCharArray2, nTempCharArraySize );
			UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
			PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_UNIT_COUNT ), uszTempCharArray2 );
		}	
		//SR_QUEST_UNIT_COLLECT_COUNT
		int nIndex = QuestUnitGetIndex( pFillTextData->pUnit );
		
		int nItemsNeeded(1);
		if( nIndex >= 0 )
		{
			nItemsNeeded = MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, nIndex );
		}
		PStrPrintf( uszTempCharArray, nTempCharArraySize, L"%d", nItemsNeeded );
		if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
		{
			PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_UNIT_COLLECT_COUNT ), uszTempCharArray );
		}
		else
		{
			ZeroMemory( uszTempCharArray2, nTempCharArraySize );
			UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
			PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( SR_QUEST_UNIT_COLLECT_COUNT ), uszTempCharArray2 );
		}	


	}

	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ) ||
		TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_TASKS_ITEMS ) || 
		TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_ITEM ))
	{
		//add names to item collection
		for( int c = 0; c < MAX_COLLECT_PER_QUEST_TASK; c++ )
		{
			int nItemClass = MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, c );
			if ( nItemClass != INVALID_ID )
			{				
				int nItemNameID( INVALID_ID );
				if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_ITEM ) )
				{
					nItemNameID = pFillTextData->pQuestTask->nQuestItemsFlavoredTextToCollect[ c ];
				}

				if( nItemNameID == INVALID_ID ) //quest is not overwritting the name
				{
					UNIT_DATA *itemData = UnitGetData( UnitGetGame( pFillTextData->pPlayer ), GENUS_ITEM, nItemClass );
					ASSERT_CONTINUE( itemData );
					nItemNameID = itemData->nString;
				}
				int nItemsNeeded = MYTHOS_QUESTS::QuestGetNumberToCollectForItem( pFillTextData->pPlayer, pFillTextData->pQuestTask, nItemClass );	
				DWORD dwAttributesToMatch = StringTableGetGrammarNumberAttribute( nItemsNeeded > 1 ? PLURAL : SINGULAR );				
				const WCHAR *puszUnitName = StringTableGetStringByIndexVerbose( nItemNameID, dwAttributesToMatch, 0, NULL, NULL );
				STRING_REPLACEMENT nStringReplace = (STRING_REPLACEMENT)(SR_QUEST_ITEM1 + c );
				ASSERTX_CONTINUE( nStringReplace <= SR_QUEST_ITEM6, "More quest strings for items are needed( maxed at 6 )" );
				if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_NO_COLOR ) )
				{
					PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), puszUnitName );				
				}
				else
				{
					ZeroMemory( uszTempCharArray, nTempCharArraySize );
					UIColorCatString( uszTempCharArray, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, puszUnitName );
					PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), uszTempCharArray );				
				}
			}
			else
			{
				break;
			}
		}	
	}
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_TASKS_ITEMS ) ||
		TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_ITEM ) ||
		TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_QUEST_PROPERTIES ))
	{
		//fill in any item collection count values
		DWORD dwInvIterateFlags = 0;
		SETBIT( dwInvIterateFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
		SETBIT( dwInvIterateFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );						

		for( int c = 0; c < MAX_COLLECT_PER_QUEST_TASK; c++ )
		{
			int nItemClass = MYTHOS_QUESTS::QuestGetItemIDToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, c );
			if( nItemClass == INVALID_ID )
				break;
			//int count = UnitInventoryCountUnitsOfType( pFillTextData->pPlayer, MAKE_SPECIES( GENUS_ITEM, nItemClass ), dwInvIterateFlags); 			
			int nItemsNeeded = MYTHOS_QUESTS::QuestGetNumberOfItemsToCollectByIndex( pFillTextData->pPlayer, pFillTextData->pQuestTask, c );		
			//PStrPrintf( uszTempCharArray, nTempCharArraySize, L"%d/%d", count, nItemsNeeded );
			PStrPrintf( uszTempCharArray, nTempCharArraySize, L"%d", nItemsNeeded );
			ZeroMemory( uszTempCharArray2, nTempCharArraySize );
			UIColorCatString( uszTempCharArray2, nTempCharArraySize, FONTCOLOR_LIGHT_YELLOW, uszTempCharArray );
			STRING_REPLACEMENT nStringReplace = (STRING_REPLACEMENT)(SR_QUEST_ITEMCOUNT1 + c );
			ASSERTX_CONTINUE( nStringReplace <= SR_QUEST_ITEMCOUNT6, "More quest strings for items are needed( maxed at 6 )" );
			PStrReplaceToken( pFillTextData->szFlavor, pFillTextData->nTextLen, StringReplacementTokensGet( nStringReplace ), uszTempCharArray2 );
		}	
	}

}


inline void sQuestClearMerchantStates( UNIT *pNPC )
{
	if( AppIsHellgate() )
	{
		return;
	}
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_MERCHANT, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_GUIDE, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_TRAINER, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_HEALER, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_MAPVENDOR, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_CRAFTER, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_GAMBLER, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_GUILDMASTER, 0 );

}
// place floating icons over merchant heads
inline void sQuestUpdateMerchantStates( UNIT *pNPC )
{
	if( AppIsHellgate() )
	{
		return;
	}
	
	const UNIT_DATA* pNPCData = UnitGetData( pNPC );
	if( pNPCData->nMerchantNotAvailableTillQuest != INVALID_ID )
	{
		if( !UIGetControlUnit() )
			return;
		if( !QuestGetIsTaskComplete( UIGetControlUnit(), QuestTaskDefinitionGet( pNPCData->nMerchantNotAvailableTillQuest ) ) )
		{
			return;
		}
	}
	if( UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_TRAINER) )
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_CRAFTER, 0, 0, INVALID_ID );	
	}
	else if( UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_RESPECCER) || 
			 UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_CRAFTING_RESPECCER))
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_TRAINER, 0, 0, INVALID_ID );	
	}
	else if( UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_MAP_VENDOR) )
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_MAPVENDOR, 0, 0, INVALID_ID );	
	}
	else if( UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_TRADESMAN) )
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_CRAFTER, 0, 0, INVALID_ID );	
	}
	else if( UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_DUNGEON_WARPER) )
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_GUIDE, 0, 0, INVALID_ID );	
	}
	else if (UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_GAMBLER))
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_GAMBLER, 0, 0, INVALID_ID );	
	}
	else if (UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_GUILDMASTER))
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_GUILDMASTER, 0, 0, INVALID_ID );	
	}
	else if (UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_HEALER))
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_HEALER, 0, 0, INVALID_ID );	
	}
	else if( UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_TRANSPORTER) )
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_GUIDE, 0, 0, INVALID_ID );	
	}
	else if (UnitDataTestFlag(pNPCData, UNIT_DATA_FLAG_IS_MERCHANT))
	{
		c_StateSet( pNPC, pNPC, STATE_NPC_MERCHANT, 0, 0, INVALID_ID );	
	}
}
////////////////////////////////////////////
// Find the current quest state for a given quest task
////////////////////////////////////////////
int c_GetQuestNPCState( GAME *pGame,
					    UNIT *pPlayer,
					    UNIT *pNPC,
					    const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask)
{
	//Second we check dialog overrides.....	( lowest priority )
	int nSetState( INVALID_ID );
	
	BOOL bIsSecondary = FALSE;
	for( int t = 0; t < MAX_PEOPLE_TO_TALK_TO; t++ )
	{

		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == INVALID_ID )
			break;
		if( pQuestTask->nQuestSecondaryNPCTalkTo[ t ] == UnitGetData( pNPC )->nNPC )
		{		
			bIsSecondary = TRUE;
			break;
		}
	}

	if( bIsSecondary )
	{
		nSetState = STATE_NPC_INFO_NEW;
		if( ( QuestNPCSecondaryNPCHasBeenVisited( pPlayer, pNPC, pQuestTask ) ||
				QuestWasSecondaryNPCMainNPCPreviously( pGame, pPlayer, pNPC, pQuestTask  ) ) &&
				nSetState == INVALID_ID )
		{
			nSetState = STATE_NPC_INFO_WAITING;	
			return nSetState;
		}
		else
		{
			if( QuestGetQuestAccepted( UnitGetGame( pPlayer ), pPlayer, pQuestTask ) )
			{
				if( QuestCanNPCEndQuestTask( pNPC, pPlayer, pQuestTask ) )
				{
					nSetState = STATE_NPC_INFO_RETURN;		
				}
				else
				{
					nSetState = STATE_NPC_INFO_WAITING;		
				}
			}
		}
		return nSetState;
	}

	//done clearning states
	BOOL questComplete = QuestIsQuestComplete( pPlayer, pQuestTask->nParentQuest ); //quest is already complete
	BOOL questIsTaskCompleteFromNPC( false );
	BOOL questTaskComplete( false );
	BOOL questActive( false );  //quest is active - already started		
	BOOL questCanStart( false );		
	if( !questComplete  ) //quest is not complete
	{
		if( QuestPlayerHasTask( pPlayer, pQuestTask ) ) //is an active quest
		{	
			questCanStart = true;
			questTaskComplete = QuestAllTasksCompleteForQuestTask( pPlayer, pQuestTask->nParentQuest, FALSE, FALSE );
			//lets get the active task for the quest
			pQuestTask = QuestGetActiveTask( pPlayer, pQuestTask->nParentQuest );
		}
		else
		{
			questCanStart = QuestCanStart( pGame, pPlayer, pQuestTask->nParentQuest, QUEST_START_FLAG_IGNORE_QUEST_COUNT | QUEST_START_FLAG_IGNORE_LEVEL_REQ | QUEST_START_FLAG_IGNORE_ITEM_COUNT  ); //can the quest even start
		}	
	}

	//see if the quest is active
	questActive = QuestPlayerHasTask( pPlayer, pQuestTask );
	//see if the quest is Complet from the NPC
	questIsTaskCompleteFromNPC = QuestGetIsTaskComplete( pPlayer, pQuestTask );

	//this can occur when the player is talking to the NPC for the last time.
	//Why? Because the quest gets updated and then the NPC grabs it's state
	if( !questIsTaskCompleteFromNPC )
	{
		if( pQuestTask->nNPC != pNPC->pUnitData->nNPC &&
			!UnitIsGodMessanger( pNPC ))
		{
			//now lets check for monsters! Some monsters turn into NPC's after
			//they are killed. A monster cannot get here unless they have a state
			//set after death. So it's safe to assume at this point in the logic
			//if it is a quest unit, it's a monster or an item and if we have 
			//the quest complete anywhere text, then it's a 100% sure it has dialog.
			if( QuestUnitIsQuestUnit( pNPC ) &&
				pQuestTask->nQuestDialogOnTaskCompleteAnywhere != INVALID_ID )
			{
				nSetState = STATE_NPC_INFO_RETURN;										
			}				
		}
	}

	//states copied from 
	if( questTaskComplete ) //the quest is complete, but the player hasn't returned to talk with the NPC yet
	{
		//Some tasks require two or more NPC's. If the next NPC is not the same, we are done
		//talking to the current NPC.
		if( pQuestTask->pNext != NULL &&
			questIsTaskCompleteFromNPC ) //check and see if the NPC has finished the Task
		{
			if( pQuestTask->pNext->nNPC == pQuestTask->nNPC )
			{
				nSetState = STATE_NPC_INFO_RETURN;
			}
		}
		else
		{
			nSetState = STATE_NPC_INFO_RETURN;
		}

	}
	else if( questActive ) //the player hasn't finished the quest yet
	{
		nSetState = STATE_NPC_INFO_WAITING;
	}
	else if( questCanStart ) //the npc has a quest to give the player.
	{
		int iLevel = UnitGetExperienceLevel( pPlayer );
		const QUEST_DEFINITION_TUGBOAT *activeQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
		//we are giving a new quest but we need to make sure the level of the player is below the max level the quest will work for
		if( activeQuest != NULL &&
			activeQuest->nQuestPlayerStartLevel != INVALID_ID &&
			iLevel < activeQuest->nQuestPlayerStartLevel )
		{
			if( nSetState == INVALID_ID )
			{
				nSetState = STATE_NPC_INFO_LEVEL_TO_LOW;
			}
		}
		else 
		{
			if( nSetState != STATE_NPC_INFO_RETURN )
			{
				nSetState = STATE_NPC_INFO_NEW;
			}
		}			
	}
	return nSetState;
}

////////////////////////////////////////////
//This function checks and sets the states on a npc if they have a 
//quest for the player to do or not. Most of the logic via the state setting
//was copied from the function sUpdateNPCs. Which is where this function should
//be getting called from.
/////////////////////////////////////////////
BOOL c_QuestUpdateQuestNPCs( GAME *pGame,
							 UNIT *pPlayer,
							 UNIT *pNPC )
{
	ASSERTX_RETFALSE( IS_CLIENT( pGame ), "Calling c_QuestUpdateQuestNPCs from server." );
	if( pPlayer == NULL )
	{
		return FALSE; //must have a player
	}
	//lets make sure we clear the previous states
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_NEW, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_WAITING, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_RETURN, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_NEW_RANDOM, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_WAITING_RANDOM, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_RETURN_RANDOM, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_NEW_CRAFTING, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_WAITING_CRAFTING, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_RETURN_CRAFTING, 0 );
	c_StateClear( pNPC, UnitGetId( pNPC ), STATE_NPC_INFO_LEVEL_TO_LOW, 0 );

	// clear merchant states
	sQuestClearMerchantStates( pNPC );

	//First we get the quests for the NPC if they have one
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskArray[MAX_NPC_QUEST_TASK_AVAILABLE];
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskSecondaryArray[MAX_NPC_QUEST_TASK_AVAILABLE];
	int nQuestTaskCount = QuestGetAvailableNPCQuestTasks( pGame,
												pPlayer,
												pNPC,
												pQuestTaskArray,
												TRUE,
												0 );

	int nQuestTaskSecondaryCount = QuestGetAvailableNPCSecondaryDialogQuestTasks( pGame,
																				 pPlayer,
																				 pNPC,
																				 pQuestTaskSecondaryArray,
																				 0 );

	

	//Second we check dialog overrides.....	( lowest priority )
	int nSetState( INVALID_ID );
	BOOL bIsRandomQuestGiver( FALSE );
	for( int t = 0; t < nQuestTaskSecondaryCount; t++ ) //last second part is always all secondary quests
	{
		if( ( QuestNPCSecondaryNPCHasBeenVisited( pPlayer, pNPC, pQuestTaskSecondaryArray[ t ] ) ||
			  QuestWasSecondaryNPCMainNPCPreviously( pGame, pPlayer, pNPC, pQuestTaskSecondaryArray[ t ]  ) ) &&
			nSetState == INVALID_ID )
		{
			nSetState = STATE_NPC_INFO_WAITING;		
		}
		else
		{
			if( QuestGetQuestAccepted( UnitGetGame( pPlayer ), pPlayer, pQuestTaskSecondaryArray[ t ] ) )
			{
				if( QuestCanNPCEndQuestTask( pNPC, pPlayer, pQuestTaskSecondaryArray[ t ] ) )
				{
					if(  QuestIsQuestTaskComplete( pPlayer, pQuestTaskSecondaryArray[ t ] ) )
					{
						nSetState = STATE_NPC_INFO_RETURN;		
					}
					else
					{
						for( int i = 0; i < MAX_PEOPLE_TO_TALK_TO; i++ )
						{
							if( pQuestTaskSecondaryArray[ t ]->nQuestSecondaryNPCTalkTo[i] == pNPC->pUnitData->nNPC )
							{
								if( pQuestTaskSecondaryArray[ t ]->nQuestNPCTalkToDialog[i] != INVALID_ID )
								{
									nSetState = STATE_NPC_INFO_RETURN;
								}
								else
								{
									nSetState = STATE_NPC_INFO_WAITING;	
								}
								break;
							}
						}
					}
				}

			}
		}
	}
	
	for( int t = 0; t < nQuestTaskCount; t++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = pQuestTaskArray[ t ];
		if( pQuestTask == NULL )
			continue;
		//done clearning states
		bIsRandomQuestGiver = isQuestRandom( pQuestTask );
		BOOL questComplete = QuestIsQuestComplete( pPlayer, pQuestTask->nParentQuest ); //quest is already complete
		BOOL questIsTaskCompleteFromNPC( false );
		BOOL questTaskComplete( false );
		BOOL questActive( false );  //quest is active - already started		
		BOOL questCanStart( false );		
		if( !questComplete  ) //quest is not complete
		{
			if( QuestPlayerHasTask( pPlayer, pQuestTask ) ) //is an active quest
			{	
				questCanStart = true;
				questTaskComplete = QuestAllTasksCompleteForQuestTask( pPlayer, pQuestTask->nParentQuest, FALSE, FALSE );
				//lets get the active task for the quest
				pQuestTask = QuestGetActiveTask( pPlayer, pQuestTask->nParentQuest );
			}
			else
			{
				questCanStart = QuestCanStart( pGame, pPlayer, pQuestTask->nParentQuest, QUEST_START_FLAG_IGNORE_QUEST_COUNT | QUEST_START_FLAG_IGNORE_LEVEL_REQ | QUEST_START_FLAG_IGNORE_ITEM_COUNT  ); //can the quest even start
			}	
		}
		else
		{
			//nothing happens. The quest giver no longer talks to the player.
		}

		//see if the quest is active
		questActive = QuestPlayerHasTask( pPlayer, pQuestTask );
		//see if the quest is Complete from the NPC
		questIsTaskCompleteFromNPC = QuestGetIsTaskComplete( pPlayer, pQuestTask );

		//this can occur when the player is talking to the NPC for the last time.
		//Why? Because the quest gets updated and then the NPC grabs it's state
		if( !questIsTaskCompleteFromNPC )
		{
			if( pQuestTask->nNPC != pNPC->pUnitData->nNPC &&
				!UnitIsGodMessanger( pNPC ))
			{
				//now lets check for monsters! Some monsters turn into NPC's after
				//they are killed. A monster cannot get here unless they have a state
				//set after death. So it's safe to assume at this point in the logic
				//if it is a quest unit, it's a monster or an item and if we have 
				//the quest complete anywhere text, then it's a 100% sure it has dialog.
				if( QuestUnitIsQuestUnit( pNPC ) &&
					pQuestTask->nQuestDialogOnTaskCompleteAnywhere != INVALID_ID )
				{
					nSetState = STATE_NPC_INFO_RETURN;										
				}				
				continue;
			}
		}
		
		//states copied from 
		if( questTaskComplete ) //the quest is complete, but the player hasn't returned to talk with the NPC yet
		{
			//Some tasks require two or more NPC's. If the next NPC is not the same, we are done
			//talking to the current NPC.
			if( pQuestTask->pNext != NULL &&
				questIsTaskCompleteFromNPC ) //check and see if the NPC has finished the Task
			{
				if( pQuestTask->pNext->nNPC != pQuestTask->nNPC )
				{
					continue;
				}
			}
			nSetState = STATE_NPC_INFO_RETURN;
			
			//c_StateSet( pNPC, pNPC, STATE_NPC_INFO_RETURN, 0, 0, INVALID_ID );
			//return TRUE;
			continue;
		}
		else if( questActive ) //the player hasn't finished the quest yet
		{
			if( nSetState != STATE_NPC_INFO_NEW && nSetState != STATE_NPC_INFO_RETURN )
			{
				nSetState = STATE_NPC_INFO_WAITING;
			}
			//c_StateSet( pNPC, pNPC, STATE_NPC_INFO_WAITING, 0, 0, INVALID_ID );
			continue;
		}
		else if( questCanStart ) //the npc has a quest to give the player.
		{
			int iLevel = UnitGetExperienceLevel( pPlayer );
			const QUEST_DEFINITION_TUGBOAT *activeQuest = QuestDefinitionGet( pQuestTask->nParentQuest );
			//we are giving a new quest but we need to make sure the level of the player is below the max level the quest will work for
			if( activeQuest != NULL &&
				activeQuest->nQuestPlayerStartLevel != INVALID_ID &&
				iLevel < activeQuest->nQuestPlayerStartLevel )
			{
				if( nSetState == INVALID_ID )
				{
					nSetState = STATE_NPC_INFO_LEVEL_TO_LOW;
				}
				//c_StateSet( pNPC, pNPC, STATE_NPC_INFO_LEVEL_TO_LOW, 0, 0, INVALID_ID );
			}
			else 
			{
				if( nSetState != STATE_NPC_INFO_RETURN )
				{
					nSetState = STATE_NPC_INFO_NEW;
				}
				//c_StateSet( pNPC, pNPC, STATE_NPC_INFO_NEW, 0, 0, INVALID_ID );
			}

			continue;				
		}
		
	}

	if( ( nSetState == STATE_NPC_INFO_NEW ||
		  nSetState == STATE_NPC_INFO_LEVEL_TO_LOW ) &&
		UnitDataTestFlag( pNPC->pUnitData, UNIT_DATA_FLAG_IS_TASKGIVER_NO_STARTING_ICON ) )
	{
		nSetState = INVALID_ID;
	}

	if( nSetState != INVALID_ID )
	{
		if( bIsRandomQuestGiver )
		{
			if( nSetState == STATE_NPC_INFO_NEW )
				nSetState = STATE_NPC_INFO_NEW_RANDOM;
			else if( nSetState == STATE_NPC_INFO_WAITING )
				nSetState = STATE_NPC_INFO_WAITING_RANDOM;
			else if( nSetState == STATE_NPC_INFO_RETURN )
				nSetState = STATE_NPC_INFO_RETURN_RANDOM;
		}
		else if(UnitIsA(pNPC,UNITTYPE_CRAFTER_MONSTER))
		{
			if( nSetState == STATE_NPC_INFO_NEW )
				nSetState = STATE_NPC_INFO_NEW_CRAFTING;
			else if( nSetState == STATE_NPC_INFO_WAITING )
				nSetState = STATE_NPC_INFO_WAITING_CRAFTING;
			else if( nSetState == STATE_NPC_INFO_RETURN )
				nSetState = STATE_NPC_INFO_RETURN_CRAFTING;

		}

		c_StateSet( pNPC, pNPC, nSetState, 0, 0, INVALID_ID ); //set the state		

	}
	else
	{
		sQuestUpdateMerchantStates( pNPC ); //put any other (merchant, guild, ect ) icons back on...
	}
	
	return ( nSetState != INVALID_ID );
}

void c_QuestFillDialogText( UNIT *pPlayer,
						    UI_LINE *pLine )
							
{
	if( AppIsHellgate() ||
		pLine == NULL ||
		pPlayer == NULL ||
		UnitIsPlayer( pPlayer ) == false )
		return;	

	const WCHAR*	szText = pLine->GetText();
	WCHAR*			szNewText = NULL;
	int nPos = 0;
	int nNewTextPos = 0;	
	int	nStringNewLength = 0;
	bool bIgnoreAdd( false );
	while (szText[nPos] != L'\0' )
	{
		bIgnoreAdd = false;
		if (szText[nPos] == TEXT_TAG_BEGIN_CHAR )
		{
			int txtTagIndex = nPos;	//starting position of the Tag
			const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask( NULL ); //quest task if there is one
			int nStatID( INVALID_LINK );
			int nQuestDictionaryLookUp( INVALID_LINK );			
			nPos++; //skip over the [
			int validIndex( -1 );	//the index of what we want to show
			BOOL forwardPointerTillEndTag( FALSE );

			//get any stat [%xxx_%ccccccc] (xxx would be the code of the stat and ccccc is name ( ignored ) )
			if( szText[nPos] != L'\0' &&
				szText[nPos] == L'%' )
			{			
				nPos++; //go past the @						
				int nStatCount( 0 );
				DWORD nStatCode( 0 );
				while( szText[nPos ] != L'\0' &&
					   szText[nPos + 1 ] != L'\0' &&
					   szText[nPos ] != L'_' &&	//look for _T
					   szText[nPos + 1 ] != L'%' ) //look for _T
				{
					nStatCode = nStatCode | (szText[nPos] << ( nStatCount * 8 ) );
					nStatCount++;
					nPos++;
				}
				nPos++; //skip the _
				nPos++; //skip the %
				nStatID = ExcelGetLineByCode(EXCEL_CONTEXT(), DATATABLE_STATS, nStatCode);
				ASSERTX_RETURN( nStatID != INVALID_LINK, "Attempting to fill in tags with stat but stat didn't match a code." );
				validIndex = KQUEST_TEXT_STATS;
				forwardPointerTillEndTag = TRUE;
			}
			//get any quest task info [@xxx_@ccccccc] (xxx would be the code of the task )
			if( szText[nPos] != L'\0' &&
				szText[nPos] == L'@' )
			{			
				nPos++; //go past the @						
				int nQuestTaskCount( 0 );
				DWORD nQuestTaskCode( 0 );
				while( szText[nPos ] != L'\0' &&
					   szText[nPos + 1 ] != L'\0' &&
					   szText[nPos ] != L'_' &&	//look for _T
					   szText[nPos + 1 ] != L'@' ) //look for _T
				{
					nQuestTaskCode = nQuestTaskCode | (szText[nPos] << ( nQuestTaskCount * 8 ) );
					nQuestTaskCount++;
					nPos++;
				}
				nPos++; //skip the _
				nPos++; //skip the @
				pQuestTask = QuestTaskDefinitionGet( nQuestTaskCode );
				ASSERTX_RETURN( pQuestTask, "Attempting to fill in tags in string with quest task info but quest task was null." );
				forwardPointerTillEndTag = TRUE;
			}
			//get any quest task info [*xxx_*ccccccc] (xxx would be the code of the dictionary )
			
			if( szText[nPos] != L'\0' &&
				szText[nPos] == L'*' )
			{			
				nPos++; //go past the *						
				int nDictionaryCount( 0 );
				DWORD nDictionaryCode( 0 );
				while( szText[nPos ] != L'\0' &&
					   szText[nPos + 1 ] != L'\0' &&
					   szText[nPos ] != L'_' &&	//look for _*
					   szText[nPos + 1 ] != L'*' ) //look for _*
				{
					nDictionaryCode = nDictionaryCode | (szText[nPos] << ( nDictionaryCount * 8 ) );
					nDictionaryCount++;
					nPos++;
				}
				nPos++; //skip the _
				nPos++; //skip the @
				if( nDictionaryCode != 0 )
				{
					int index = ExcelGetLineByCode(EXCEL_CONTEXT(), DATATABLE_QUEST_DICTIONARY_FOR_TUGBOAT, nDictionaryCode);
					ASSERTX_RETURN( index != INVALID_ID, "Attempting to get Quest Dictionary entry but was invalid." );
					const QUEST_DICTIONARY_DEFINITION_TUGBOAT *dictionaryEntry = (const QUEST_DICTIONARY_DEFINITION_TUGBOAT*)ExcelGetData( NULL, DATATABLE_QUEST_DICTIONARY_FOR_TUGBOAT, index );
					if( dictionaryEntry != NULL )
						nQuestDictionaryLookUp = dictionaryEntry->nStringTableLink;
					validIndex = KQUEST_TEXT_DICTIONARY;
				}
				forwardPointerTillEndTag = TRUE;
				
			}
			if( forwardPointerTillEndTag )
			{
				while( szText[nPos] != TEXT_TAG_END_CHAR &&
					   szText[nPos] != L'\0' )
				{
					nPos++;
				}
			}
			//this find the end bracket
			if( validIndex == -1 )
			{
				bool valid[ KQUEST_TEXT_COUNT ]; //if the lookup is valid or not			
				memset( &valid[ 0 ], TRUE, sizeof( bool ) *  KQUEST_TEXT_COUNT );
				bool stillValid( true );
				int length = 0;			
				//ok lets see if we match any tokens to replace...
				while( szText[nPos] != TEXT_TAG_END_CHAR &&
					   szText[nPos] != L'\0' )
				{
					if( stillValid )
					{
						stillValid = false;
						for( int t = 0; t < KQUEST_TEXT_COUNT; t++ )
						{
							if( valid[ t ] &&
								szText[ nPos ] == s_ReplacementTable[ t ].puszToken[ length ] )
							{
								stillValid = true;
								validIndex = t;
							}
							else
							{
								valid[ t ] = false;
								if( validIndex == t )
								{
									validIndex = -1;
								}
							}
						}
					}
					//if( stillValid == false )
					//	break; //none are still valid
					length++;
					nPos++;				
				}			
				if( szText[nPos] == L'\0' )
					validIndex = -1; //make it invalid
			}
			//no actually display the character
			if( validIndex != -1 )
			{				
				WCHAR replaceWith[ 255 ];
				memclear( &replaceWith[ 0 ], sizeof( WCHAR ) * 255 );
				switch( validIndex )
				{
				case KQUEST_TEXT_DICTIONARY:
					
					if( nQuestDictionaryLookUp != INVALID_LINK )
					{
						const WCHAR *stringIs = StringTableGetStringByIndex( nQuestDictionaryLookUp );
						if( stringIs != NULL )
						{
							memcpy( &replaceWith[0], stringIs, sizeof( WCHAR ) *  PStrLen( stringIs ) );
						}
						nPos+=PStrLen( &replaceWith[0] ) - 1;
					}
					break;
				case KQUEST_TEXT_NPC_NAME:
					break;
				case KQUEST_TEXT_CLASS_NAME:
					if( pPlayer &&
						pPlayer->pUnitData->nString != -1 )
					{
						const WCHAR *stringIs = StringTableGetStringByIndex( pPlayer->pUnitData->nString );
						if( stringIs != NULL )
						{
							memcpy( &replaceWith[0], stringIs, sizeof( WCHAR ) *  PStrLen( stringIs ) );
						}
						nPos++;
					}
					break;
				case KQUEST_TEXT_PLAYER_NAME:
					UnitGetName( pPlayer, &replaceWith[0], 255, 0 );
					nPos++;
					break;
				case KQUEST_TEXT_STATS:
					if( nStatID != INVALID_LINK )
					{
						//probably setup a means to get a second param going
						int statValue = UnitGetStat( pPlayer, nStatID );
						statValue = statValue >> StatsGetShift( UnitGetGame( pPlayer ), nStatID);
						CHAR tmp[ 48 ];						
						memclear( &tmp[ 0 ], sizeof( CHAR ) * 48 );
						PStrPrintf( &tmp[0], 48, "%d", statValue );
						int aSize( 0 );
						while( tmp[ aSize ] != _T('\0') )
						{
							replaceWith[ aSize ] = (WCHAR)tmp[ aSize ];
							aSize++;
						}						
						nPos++;
					}
					break;
				default:
					break;
				}
				if( replaceWith[ 0 ] != NULL )
				{				
					bIgnoreAdd = true;
					int lenOfReplace = PStrLen( &replaceWith[0] );					
					if (szNewText == NULL)
					{
						nStringNewLength = PStrLen(szText)  + lenOfReplace;
						szNewText = MALLOC_NEWARRAY(NULL, WCHAR,  nStringNewLength);
						memcpy(szNewText, szText, nNewTextPos * sizeof(WCHAR));				// copy the portion we've already passed // KCK: This looks like it will always do nothing
					}
					else
					{
						WCHAR* tmpString = szNewText;
						szNewText = MALLOC_NEWARRAY(NULL, WCHAR, nStringNewLength  + lenOfReplace );
						memcpy( szNewText, tmpString, sizeof( WCHAR ) * nStringNewLength );
						nStringNewLength += lenOfReplace;
						FREE_DELETE_ARRAY(NULL, tmpString, WCHAR);
					}
					memcpy( &szNewText[nNewTextPos], &replaceWith[ 0 ], sizeof( WCHAR ) * lenOfReplace );
					nNewTextPos += lenOfReplace; //we don't want the ]
					
				}
			}
			else
			{
				nPos = txtTagIndex;			
			}			
		}
		if( !bIgnoreAdd )
		{
			if( szNewText != NULL )
			{
				//copy the text
				szNewText[ nNewTextPos ] = szText[nPos];
			}
			nPos++;
			nNewTextPos++;
		}
	}

	if( szNewText != NULL )
	{
		szNewText[ nNewTextPos] = '\0';
		pLine->SetText(szNewText);
	}
}



//----------------------------------------------------------------------------
// update the reward item icons
//----------------------------------------------------------------------------
void c_QuestRewardUpdateUI( UI_COMPONENT* pComponent,
						    BOOL bAllowRewardSelection,
							BOOL bTaskScreen,
						    int nQuestDefID )
{

	UNIT *pPlayer = UIGetControlUnit();

	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

	int nSourceLocation = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );
	int nSourceLocation2 = INVALID_ID;
	if( bTaskScreen )
	{
		nSourceLocation2 = GlobalIndexGet( GI_INVENTORY_LOCATION_OFFERING_STORAGE );
	}

	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( nQuestDefID );

	int nOffer = pQuestTask ? pQuestTask->nQuestGivesItems : INVALID_LINK;	
	int nOfferChoice = pQuestTask ? MYTHOS_QUESTS::QuestGetPlayerOfferRewardsIndex( pQuestTask ) : INVALID_LINK;

	// setup tables for reward item comps
	UI_COMPONENT* pCompReward[5];

	static char* cCompReward[] =
	{
		"reward item 1",				//	UICOMP_REWARD_ITEM_1,
		"reward item 2",				//	UICOMP_REWARD_ITEM_2,
		"reward item 3",				//	UICOMP_REWARD_ITEM_3,
		"reward item 4",				//	UICOMP_REWARD_ITEM_4,
		"reward item 5",				//	UICOMP_REWARD_ITEM_5,
		"",
	};
	for( int i = 0; i < 5; i++ )
	{
		pCompReward[i] = UIComponentFindChildByName(pComponent, cCompReward[i]);
		UIComponentSetVisible( pCompReward[i], FALSE );
		UIComponentSetActive( pCompReward[i], FALSE );
		UIComponentSetFocusUnit( pCompReward[i], INVALID_ID );
	}


	int nRewardIndex = 0;
	UNIT * pItem = NULL;

	if( pQuestTask )
	{
		pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem, 0, FALSE );
		while (pItem != NULL)
		{
			UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem, 0, FALSE );
			int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
			if( UnitGetStat( pItem, STATS_QUEST_REWARD_GUARANTEED ) == 1 &&
				( nItemOfferID == nOffer || nItemOfferID == nOfferChoice ) )
			{
				// show this component
				UI_COMPONENT* pComp = pCompReward[ nRewardIndex++ ];
				UIComponentSetActive( pComp, TRUE );

				// set the focus unit
				UIComponentSetFocusUnit( pComp, UnitGetId( pItem ) );
			}

			pItem = pItemNext;

		}

		int nQuestTaskID = pQuestTask ? pQuestTask->nTaskIndexIntoTable : INVALID_ID;
		if( nSourceLocation2 != INVALID_ID )
		{
			pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation2, pItem, 0, FALSE );
			while (pItem != NULL)
			{
				UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation2, pItem, 0, FALSE );
				int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
				if( UnitGetStat( pItem, STATS_QUEST_REWARD_GUARANTEED ) == 1  &&
					( nItemOfferID == nOffer || nItemOfferID == nOfferChoice ) &&
					( nQuestTaskID == INVALID_ID ||
					  UnitGetStat( pItem, STATS_QUEST_TASK_REF ) == nQuestTaskID ))
				{
					// show this component
					UI_COMPONENT* pComp = pCompReward[ nRewardIndex++ ];
					UIComponentSetActive( pComp, TRUE );

					// set the focus unit
					UIComponentSetFocusUnit( pComp, UnitGetId( pItem ) );
				}

				pItem = pItemNext;

			}
		}
		

	}

	// setup tables for reward item comps
	UI_COMPONENT* pCompChoiceReward[5];

	static char* cCompChoiceReward[] =
	{
		"choice reward item 1",			//	UICOMP_CHOICE_REWARD_ITEM_1,
		"choice reward item 2",			//	UICOMP_CHOICE_REWARD_ITEM_2,
		"choice reward item 3",			//	UICOMP_CHOICE_REWARD_ITEM_3,
		"choice reward item 4",			//	UICOMP_CHOICE_REWARD_ITEM_4,	
		"choice reward item 5",			//	UICOMP_CHOICE_REWARD_ITEM_5,
		"",
	};

	// setup tables for reward item comps
	UI_COMPONENT* pCompChoiceRewardButton[5];

	static char* cCompChoiceRewardButton[] =
	{
		"choice reward button 1",		//	UICOMP_CHOICE_REWARD_BUTTON_1,
		"choice reward button 2",		//	UICOMP_CHOICE_REWARD_BUTTON_2,
		"choice reward button 3",		//	UICOMP_CHOICE_REWARD_BUTTON_3,
		"choice reward button 4",		//	UICOMP_CHOICE_REWARD_BUTTON_4,	
		"choice reward button 5",		//	UICOMP_CHOICE_REWARD_BUTTON_5,	
		"",
	};
	for( int i = 0; i < 5; i++ )
	{
		pCompChoiceReward[i] = UIComponentFindChildByName(pComponent, cCompChoiceReward[i]);
		UIComponentSetVisible( pCompChoiceReward[i], FALSE );
		UIComponentSetFocusUnit( pCompChoiceReward[i], INVALID_ID );

		pCompChoiceRewardButton[i] = UIComponentFindChildByName(pComponent, cCompChoiceRewardButton[i]);
		UIComponentSetVisible( pCompChoiceRewardButton[i], FALSE );
	}

	int nChoiceRewardIndex = 0;
	pItem = NULL;
	if( pQuestTask )
	{
		pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem, 0, FALSE );
		while (pItem != NULL)
		{
			UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation, pItem, 0, FALSE );
			int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
			if( UnitGetStat( pItem, STATS_QUEST_REWARD_GUARANTEED ) != 1 &&
				( nItemOfferID == nOffer || nItemOfferID == nOfferChoice ) )
			{
				// show this component
				UI_COMPONENT *pComp = pCompChoiceReward[ nChoiceRewardIndex ];
				UIComponentSetActive( pComp, TRUE );				

				if( bAllowRewardSelection )
				{
					UI_COMPONENT *pCompButton = pCompChoiceRewardButton[ nChoiceRewardIndex ];
					UIComponentSetActive( pCompButton, TRUE );
				}

				nChoiceRewardIndex++;

				// set the focus unit
				UIComponentSetFocusUnit( pComp, UnitGetId( pItem ) );
			}

			pItem = pItemNext;

		}

		int nQuestTaskID = pQuestTask ? pQuestTask->nTaskIndexIntoTable : INVALID_ID;
		if( nSourceLocation2 != INVALID_ID )
		{
			pItem = UnitInventoryLocationIterate( pPlayer, nSourceLocation2, pItem, 0, FALSE );
			while (pItem != NULL)
			{
				UNIT * pItemNext = UnitInventoryLocationIterate( pPlayer, nSourceLocation2, pItem, 0, FALSE );
				int nItemOfferID = UnitGetStat( pItem, STATS_OFFERID, nDifficulty );
				if( UnitGetStat( pItem, STATS_QUEST_REWARD_GUARANTEED ) != 1 &&
					( nItemOfferID == nOffer || nItemOfferID == nOfferChoice ) &&
					nChoiceRewardIndex < 5  &&
					( nQuestTaskID == INVALID_ID ||
					UnitGetStat( pItem, STATS_QUEST_TASK_REF ) == nQuestTaskID ))
				{
					// show this component
					UI_COMPONENT *pComp = pCompChoiceReward[ nChoiceRewardIndex ];
					UIComponentSetActive( pComp, TRUE );				

					if( bAllowRewardSelection )
					{
						UI_COMPONENT *pCompButton = pCompChoiceRewardButton[ nChoiceRewardIndex ];
						UIComponentSetActive( pCompButton, TRUE );
					}

					nChoiceRewardIndex++;

					// set the focus unit
					UIComponentSetFocusUnit( pComp, UnitGetId( pItem ) );
				}

				pItem = pItemNext;

			}
		}

	}
	UI_COMPONENT * pDlgBackground = pComponent;
	ASSERT_RETURN(pDlgBackground);



	if( nRewardIndex > 0 || 
		nChoiceRewardIndex > 0 ||
		( pQuestTask &&
		( MYTHOS_QUESTS::QuestGetGoldEarned( pPlayer, pQuestTask ) > 0 ||
		  MYTHOS_QUESTS::QuestGetExperienceEarned( pPlayer, pQuestTask ) > 0 ) ) )
	{
		UI_LABELEX *pRewardHeader = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "reward instructions label") );
		UILabelSetText( pRewardHeader, L"You will receive:"  );
	}
	else
	{
		UI_LABELEX *pRewardHeader = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "reward instructions label") );
		UILabelSetText( pRewardHeader, L""  );
	}

	if( pQuestTask )
	{
		if(  MYTHOS_QUESTS::QuestGetGoldEarned( pPlayer, pQuestTask ) > 0 )
		{

			UI_LABELEX *pRewardLabel = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "gold reward label") );
			UIComponentSetVisible( pRewardLabel, TRUE );

			cCurrency Reward(  MYTHOS_QUESTS::QuestGetGoldEarned( pPlayer, pQuestTask ), 0 );

			WCHAR uszAmount[ 512 ];		
			PStrPrintf( 
				uszAmount, 
				512, 
				GlobalStringGet( GS_MONEY_CSG ), 
				Reward.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ),
				Reward.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ),
				Reward.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );
							
			
			UILabelSetText( pRewardLabel, uszAmount  );
		}
		else
		{
			UI_LABELEX *pRewardLabel = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "gold reward label") );
			UIComponentSetVisible( pRewardLabel, FALSE );
		}
		if( MYTHOS_QUESTS::QuestGetExperienceEarned( pPlayer, pQuestTask ) > 0 )
		{
			UI_COMPONENT *pRewardHeader = UIComponentFindChildByName(pDlgBackground, "xp reward header" );
			UIComponentSetVisible( pRewardHeader, TRUE );
			UI_LABELEX *pRewardLabel = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "xp reward label") );
			UIComponentSetVisible( pRewardLabel, TRUE );

			WCHAR uszAmount[ MAX_REMAINING_STRING ];									
			PIntToStr( uszAmount, 
				MAX_REMAINING_STRING, 
				MYTHOS_QUESTS::QuestGetExperienceEarned( pPlayer, pQuestTask ) );
			UILabelSetText( pRewardLabel, uszAmount  );
		}
		else
		{

			UI_COMPONENT *pRewardHeader = UIComponentFindChildByName(pDlgBackground, "xp reward header" );
			UIComponentSetVisible( pRewardHeader, FALSE );
			UI_LABELEX *pRewardLabel = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "xp reward label") );
			UIComponentSetVisible( pRewardLabel, FALSE );
		}

	}
	else
	{
		UI_LABELEX *pRewardLabel = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "gold reward label") );
		UIComponentSetVisible( pRewardLabel, FALSE );

		UI_COMPONENT *pRewardHeader = UIComponentFindChildByName(pDlgBackground, "xp reward header" );
		UIComponentSetVisible( pRewardHeader, FALSE );
		pRewardLabel = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "xp reward label") );
		UIComponentSetVisible( pRewardLabel, FALSE );
	}

	if( pQuestTask && nChoiceRewardIndex > 0 )
	{
		if( bAllowRewardSelection )
		{
			UI_LABELEX *pRewardHeader = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "reward choice label") );
			UILabelSetText( pRewardHeader, L"Select one of the following"  );
			UIComponentThrobStart( pRewardHeader, 0xffffffff, 2000 );
		}
		else
		{
			UI_LABELEX *pRewardHeader = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "reward choice label") );
			UILabelSetText( pRewardHeader, L"You will also receive one of the following"  );
			UIComponentThrobEnd(pRewardHeader, 0, 0, 0);
		}

	}
	else
	{
		UI_LABELEX *pRewardHeader = UICastToLabel( UIComponentFindChildByName(pDlgBackground, "reward choice label") );
		UILabelSetText( pRewardHeader, L""  );
		UIComponentThrobEnd(pRewardHeader, 0, 0, 0);

	}
	// enable/disable the accept button
	//UIComponentSetActiveByEnum( UICOMP_RECIPE_CREATE, bCanCreate );

} // c_QuestRewardCurrentUpdateUI()

FONTCOLOR c_QuestGetDifficultyTextColor( UNIT *pPlayer,
										 const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask )
{
	ASSERT_RETVAL( pPlayer, FONTCOLOR_LIGHT_YELLOW );
	ASSERT_RETVAL( pQuestTask, FONTCOLOR_LIGHT_YELLOW );
	MYTHOS_LEVELAREAS::CLevelAreasKnownIterator levelAreaIterator( pPlayer );
	int mapLevelAreaID( INVALID_ID );
	int nFlags = MYTHOS_LEVELAREAS::KLVL_AREA_ITER_DONT_GET_POSITION | MYTHOS_LEVELAREAS::KLVL_AREA_ITER_IGNORE_ZONE | MYTHOS_LEVELAREAS::KLVL_AREA_ITER_NO_TEAM;	
	MYTHOS_LEVELAREAS::ELEVELAREA_DIFFICULTY nDiffculty( MYTHOS_LEVELAREAS::LEVELAREA_DIFFICULTY_TO_EASY );
	while ( (mapLevelAreaID = levelAreaIterator.GetNextLevelAreaID( nFlags ) ) != INVALID_ID )
	{			
		if( QuestIsLevelAreaNeededByTask( pPlayer, mapLevelAreaID, pQuestTask ) )
		{
			MYTHOS_LEVELAREAS::ELEVELAREA_DIFFICULTY nNewDiffculty = MYTHOS_LEVELAREAS::LevelAreaGetDifficultyComparedToUnit( mapLevelAreaID, pPlayer );
			if( nNewDiffculty > nDiffculty )
				nDiffculty = nNewDiffculty;
		}
	}	
	return MYTHOS_LEVELAREAS::c_GetLevelAreaGetTextColorDifficulty( nDiffculty ); 
}


inline void c_QuestFillFlavoredText( QUEST_FILLTEXT_DATA *pFillTextData )
{
	ASSERT_RETURN( pFillTextData );
	ASSERT_RETURN( pFillTextData->pPlayer );
	ASSERT_RETURN( pFillTextData->pQuestTask );



	//fill in the quest log if it needs to be
	if( ( pFillTextData->nFlags & QUEST_FLAG_FILLTEXT_QUEST_LOG ) == QUEST_FLAG_FILLTEXT_QUEST_LOG )
	{
		const WCHAR *logTags = GlobalStringGet( GS_QUEST_LOG_LAYOUT );
		PStrCopy( pFillTextData->szFlavor, logTags, MAX_XML_STRING_LENGTH );
	}	
	

	//fill with the list of items accepting
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_ITEMS_GIVEN_ON_ACCEPT ) )
	{
		c_QuestFillTextWithArrayOfItems( pFillTextData->pQuestTask, 
										 pFillTextData->pQuestTask->nQuestGiveItemsOnAccept,
										 MAX_ITEMS_TO_GIVE_ACCEPTING_TASK,
										 GlobalStringGet( GS_QUEST_ITEM_RECEIVED_LIST_LINE ),
										 StringReplacementTokensGet( SR_QUEST_ITEMS_LIST_ACCEPT ),
										 pFillTextData->szFlavor,
										 pFillTextData->nTextLen );
	}
	//fill with the list of items completing
	if( TEST_MASK( pFillTextData->nFlags, QUEST_FLAG_FILLTEXT_ITEMS_GIVEN_ON_COMPLETE ) )
	{
		c_QuestFillTextWithArrayOfItems( pFillTextData->pQuestTask, 
										 pFillTextData->pQuestTask->nQuestRewardItemID,
										 MAX_QUEST_REWARDS,
										 GlobalStringGet( GS_QUEST_ITEM_RECEIVED_LIST_LINE ),
										 StringReplacementTokensGet( SR_QUEST_ITEMS_LIST_COMPLETE ),
										 pFillTextData->szFlavor,
										 pFillTextData->nTextLen );
	}	

	//adds tags if need be for item collection
	c_QuestFillInItemCollection( pFillTextData );
	//Adds tags for boss monsters
	c_QuestFillInBossSlayings( pFillTextData );
	//Adds objects to interact with
	c_QuestFillInObjectInteractions( pFillTextData );

	//adds the dialog if there is a tag
	c_QuestFillTextDialog( pFillTextData );


	c_QuestReplaceActualTags( pFillTextData );
	c_QuestReplaceActualTags( pFillTextData ); //must do it twice...
}

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  int nItemClassID,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags )
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	const UNIT_DATA *pUnitData = ( nItemClassID != INVALID_ID )?ItemGetData( nItemClassID ):NULL;	
	QUEST_FILLTEXT_DATA pFillTextData( pPlayer, szFlavor, len, pQuestTask, nFillTextFlags, NULL, pUnitData );
	c_QuestFillFlavoredText( &pFillTextData );
}

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  UNIT_DATA *pUnitData,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags )
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	QUEST_FILLTEXT_DATA pFillTextData( pPlayer, szFlavor, len, pQuestTask, nFillTextFlags, NULL, pUnitData );
	c_QuestFillFlavoredText( &pFillTextData );
}

void c_QuestFillFlavoredText( const QUEST_DEFINITION_TUGBOAT *pQuest,
							  UNIT_DATA *pUnitData,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags )
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	QUEST_FILLTEXT_DATA pFillTextData( pPlayer, szFlavor, len, pQuest->pFirst, nFillTextFlags, NULL, pUnitData );
	c_QuestFillFlavoredText( &pFillTextData );
}


void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							  UNIT *pUnit,
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags )
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	QUEST_FILLTEXT_DATA pFillTextData( pPlayer, szFlavor, len, pQuestTask, nFillTextFlags, pUnit, NULL );
	c_QuestFillFlavoredText( &pFillTextData );
}

void c_QuestFillFlavoredText( const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,							  
							  WCHAR* szFlavor,
							  int len,
							  int nFillTextFlags )
{
	GAME *pGame = AppGetCltGame();
	UNIT *pPlayer = GameGetControlUnit( pGame );
	QUEST_FILLTEXT_DATA pFillTextData( pPlayer, szFlavor, len, pQuestTask, nFillTextFlags, NULL, NULL );
	c_QuestFillFlavoredText( &pFillTextData );
}

int c_QuestGetFlavoredText( UNIT *pUnit,
						   UNIT *pPlayer )
{
	ASSERT_RETINVALID( pUnit );
	ASSERT_RETINVALID( pPlayer );
	if( QuestUnitIsQuestUnit( pUnit ) )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pUnit );
		if( pQuestTask )
		{
			int nIndex = QuestUnitGetIndex( pUnit );
			if( nIndex != INVALID_ID )
			{
				int nFlavoredText = MYTHOS_QUESTS::QuestGetBossItemFlavorIDByIndex( pQuestTask, nIndex );
				if( nFlavoredText == INVALID_ID )
				{
					nFlavoredText = pQuestTask->nQuestItemsFlavoredTextToCollect[ nIndex ];
				}
				return nFlavoredText;
			}
		}	
	}
	return INVALID_ID;
}

inline void AddUnitDataToArray( const UNIT_DATA *pUnitData,
							   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask,
							   const UNIT_DATA **pUnitDataArray,
							   const QUEST_TASK_DEFINITION_TUGBOAT **pQuestTaskArray,
							   int nArraySize )
{
	for( int nUnitDataCount = 0; nUnitDataCount < nArraySize; nUnitDataCount++ )
	{
		if( pUnitDataArray[ nUnitDataCount ] == pUnitData )
			return;
		if( pUnitDataArray[ nUnitDataCount ] == NULL )
		{
			pUnitDataArray[ nUnitDataCount ] = pUnitData;
			pQuestTaskArray[ nUnitDataCount ] = pQuestTask;
			return;
		}
	}
}

inline int QuestFindPointOfInterestIndexToLevelArea( UNIT *pPlayer,
											    MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfInterest,
												int nResidesInLevelArea )
{
	int nCount = pPointsOfInterest->GetPointOfInterestCount();
	for( int t = 0; t < nCount; t++ )
	{
		const MYTHOS_POINTSOFINTEREST::PointOfInterest *pPoint = pPointsOfInterest->GetPointOfInterestByIndex( t );
		if( pPoint &&
			pPoint->pUnitData &&
			pPoint->pUnitData->nLinkToLevelArea == nResidesInLevelArea )
		{
			return t;
		}
	}
	return INVALID_ID;
}

//returns in the Array a list of points of interest for the player
int c_QuestCaculatePointsOfInterest( UNIT *pPlayer )
{
	ASSERT_RETZERO( pPlayer );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERT_RETZERO( pGame );
	MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfInterest = PlayerGetPointsOfInterest( pPlayer );
	ASSERT_RETZERO( pPointsOfInterest );
		
	QUEST_UI_POINTS_OF_INTEREST_COUNT = 0;
	//array of points we want
	const UNIT_DATA *pUnitDataOfPoints[ QUEST_MAX_POINTS_OF_INTEREST ];
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskArray[ QUEST_MAX_POINTS_OF_INTEREST ];
 	ZeroMemory( pUnitDataOfPoints, sizeof( UNIT_DATA * ) * QUEST_MAX_POINTS_OF_INTEREST );
	ZeroMemory( pQuestTaskArray, sizeof( QUEST_TASK_DEFINITION_TUGBOAT * ) * QUEST_MAX_POINTS_OF_INTEREST );
	
	//first we walk all the points of interest. If one is a quest point of interest we remove that flag as well as the display flag...
	for( int t = 0; t < pPointsOfInterest->GetPointOfInterestCount(); t++ )
	{
		if( pPointsOfInterest->HasFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_IsQuestMarker ) )
		{
			pPointsOfInterest->ClearFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_IsQuestMarker );
			if( pPointsOfInterest->HasFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_Static ) == FALSE  )
			{
				pPointsOfInterest->ClearFlag( t, MYTHOS_POINTSOFINTEREST::KPofI_Flag_Display );
			}
		}
	}

	//walk all quests and what ones have check points
	for( int nQuestIndex = 0; nQuestIndex < MAX_ACTIVE_QUESTS_PER_UNIT; nQuestIndex++ )
	{	
		const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex( pPlayer, nQuestIndex );
		if( pQuestTask == NULL /*||
			!c_QuestShowsInGame( pQuestTask ) */)	//check to see if the quest is checked
			continue;		//quest task is null in that quest slot or the player doesn't have it checked.
		//lets find all monsters that are points of interest
		for( int t = 0; t < MAX_POINTS_OF_INTEREST; t++ )
		{
			int nClassID = MYTHOS_QUESTS::QuestGetPointOfInterestByIndex( pPlayer, pQuestTask, FALSE, t );
			if( nClassID == INVALID_ID )
				break;
			const UNIT_DATA *pUnitData = UnitGetData( pGame, GENUS_MONSTER, nClassID );
			AddUnitDataToArray( pUnitData, pQuestTask, pUnitDataOfPoints, pQuestTaskArray, QUEST_MAX_POINTS_OF_INTEREST );	
		}
		//now lets do the objects
		for( int t = 0; t < MAX_POINTS_OF_INTEREST; t++ )
		{
			int nClassID = MYTHOS_QUESTS::QuestGetPointOfInterestByIndex( pPlayer, pQuestTask, TRUE, t );
			if( nClassID == INVALID_ID )
				break;
			const UNIT_DATA *pUnitData = UnitGetData( pGame, GENUS_OBJECT, nClassID );
			AddUnitDataToArray( pUnitData, pQuestTask, pUnitDataOfPoints, pQuestTaskArray, QUEST_MAX_POINTS_OF_INTEREST );	
		}
		//now lets do random quests
		if( isQuestRandom( pQuestTask ) )
		{
			int nLevelAreaID = MYTHOS_QUESTS::QuestGetLevelAreaIDOnTaskAcceptedByIndex( pPlayer, pQuestTask, 0 );
			if( nLevelAreaID != INVALID_ID )
			{
				for( int t = 0; t < pPointsOfInterest->GetPointOfInterestCount(); t++ )
				{
					if( pPointsOfInterest->GetLevelAreaID( t ) == nLevelAreaID )
					{
						const MYTHOS_POINTSOFINTEREST::PointOfInterest *pPoint = pPointsOfInterest->GetPointOfInterestByIndex( t );
						AddUnitDataToArray( pPoint->pUnitData, pQuestTask, pUnitDataOfPoints, pQuestTaskArray, QUEST_MAX_POINTS_OF_INTEREST );	
					}
				}
			}
		}

	}

	
	
	//now lets attempt to find all the points of interest in the world
	for( int t = 0; t < QUEST_MAX_POINTS_OF_INTEREST; t++ )
	{
		if( pUnitDataOfPoints[ t ] == NULL )
		{
			//we're done!
			return QUEST_UI_POINTS_OF_INTEREST_COUNT;
		}
		int nIndexOfPOfI = pPointsOfInterest->GetPointOfInterestIndexByUnitData( pUnitDataOfPoints[ t ] );
		if( nIndexOfPOfI == INVALID_ID && 	//unable to find point in world.
			pUnitDataOfPoints[ t ]->nResidesInLevelArea != INVALID_ID )
		{
			nIndexOfPOfI = QuestFindPointOfInterestIndexToLevelArea( pPlayer, pPointsOfInterest, pUnitDataOfPoints[ t ]->nResidesInLevelArea );			
		}
		if( nIndexOfPOfI == INVALID_ID )
			continue;					   //unable to find the point so lets just continue on
		QUEST_UI_POINTS_OF_INTEREST[ QUEST_UI_POINTS_OF_INTEREST_COUNT++ ] = nIndexOfPOfI;
		//mark the point as a quest Marker
		pPointsOfInterest->AddFlag( nIndexOfPOfI, MYTHOS_POINTSOFINTEREST::KPofI_Flag_IsQuestMarker );
		//mark the point to show up in the UI
		pPointsOfInterest->AddFlag( nIndexOfPOfI, MYTHOS_POINTSOFINTEREST::KPofI_Flag_Display );
		//set the string that will show up.
		pPointsOfInterest->SetString( nIndexOfPOfI, MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pPlayer, pQuestTaskArray[t], KQUEST_STRING_TITLE ) );
		
	}
	return QUEST_UI_POINTS_OF_INTEREST_COUNT;
}

int c_QuestGetPointsOfInterestByIndex( UNIT *pPlayer, int nIndex )
{
	ASSERT_RETINVALID( pPlayer );
	if( nIndex >= QUEST_UI_POINTS_OF_INTEREST_COUNT )
		return INVALID_ID;
	return QUEST_UI_POINTS_OF_INTEREST[ nIndex ];
}

#endif //!SERVER_VERSION
