//----------------------------------------------------------------------------
// FILE: quests.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_message.h"
#include "c_quests.h"
#include "gameglobals.h"
#include "globalindex.h"
#include "items.h"
#include "objects.h"
#include "s_questsave.h"

// story quests
#include "quest_tutorial.h"
#include "quest_broken.h"
#include "quest_doorfix.h"
#include "quest_intro.h"
#include "quest_hellyard.h"
#include "quest_testmonkey.h"
#include "quest_truth_a.h"
#include "quest_demoend.h"
#include "quest_wisdomchaos.h"
#include "quest_books.h"
#include "quest_stitch_a.h"
#include "quest_stitch_b.h"
#include "quest_stitch_c.h"
#include "quest_chocolate.h"
#include "quest_holdfast.h"
#include "quest_redout.h"
#include "quest_mind314.h"
#include "quest_truth_b.h"
#include "quest_totemple.h"
#include "quest_firstsample.h"
#include "quest_wormhunt.h"
#include "quest_biggundown.h"
#include "quest_secondsample.h"
#include "quest_cleanup.h"
#include "quest_truth_c.h"
#include "quest_helprescue.h"
#include "quest_riftscan.h"
#include "quest_secretmsgs.h"
#include "quest_lure.h"
#include "quest_portalboost.h"
#include "quest_trainfix.h"
#include "quest_rescueride.h"
#include "quest_truth_d.h"
#include "quest_triage.h"
#include "quest_deep.h"
#include "quest_thesigil.h"
#include "quest_testknowledge.h"
#include "quest_testleadership.h"
#include "quest_testbeauty.h"
#include "quest_testfellowship.h"
#include "quest_testhope.h"
#include "quest_truth_e.h"
#include "quest_hellgate.h"
// templates
#include "quest_template.h"
#include "quest_hunt.h"
#include "quest_collect.h"
#include "quest_infestation.h"
#include "quest_explore.h"
#include "quest_operate.h"
#include "quest_escort.h"
#include "quest_talk.h"
#include "quest_useitem.h"
#include "quest_listen.h"
// randomly appearing quests
#include "quest_joey.h"
#include "quest_timeorb.h"
#include "quest_dark.h"
#include "quest_war.h"
#include "quest_death.h"
// patch 1 quests
#include "quest_cube.h"
#include "quest_stonehenge.h"
#include "quest_donation.h"
// patch 2 quests
#include "quest_abyss.h"
// end of specific-quest headers

#include "quests.h"
#include "s_message.h"
#include "s_quests.h"
#include "unit_priv.h"
#include "Quest.h"	//tugboat
#include "offer.h"	
#include "monsters.h"	
#include "excel_private.h"
#include "states.h"
#include "npc.h"
#include "s_questgames.h"

//----------------------------------------------------------------------------
struct APP_ALL_QUEST_GLOBALS
{
	APP_QUEST_GLOBAL tQuestGlobal[ NUM_QUESTS ];	
};

//----------------------------------------------------------------------------
struct QUEST_FUNCTION_LOOKUP
{
	const char *pszName;
	PFN_QUEST_FUNCTION pfnFunction;
};

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

static APP_ALL_QUEST_GLOBALS *pAppAllQuestGlobals = NULL;

#define QUEST_MAX_SIDE_QUESTS	8

//----------------------------------------------------------------------------
static QUEST_FUNCTION_LOOKUP sgtQuestFunctionLookup[] = 
{
#if HELLGATE_ONLY
	// story
	{ "TutorialInit",				QuestInitTutorial },
	{ "TutorialFree",				QuestFreeTutorial },
	{ "BrokenInit",					QuestInitBroken },
	{ "BrokenFree",					QuestFreeBroken },
	{ "DoorFixInit",				QuestInitDoorFix },
	{ "DoorFixFree",				QuestFreeDoorFix },
	{ "DoorFixOnEnterGame",			s_QuestOnEnterGameDoorFix },
	{ "IntroInit",					QuestInitIntro },
	{ "IntroFree",					QuestFreeIntro },
	{ "HellyardInit",				QuestInitHellyard },
	{ "HellyardFree",				QuestFreeHellyard },		
	{ "TestMonkeyInit",				QuestInitTestMonkey },
	{ "TestMonkeyFree",				QuestFreeTestMonkey },
	{ "TruthAInit",					QuestInitTruthA },
	{ "TruthAFree",					QuestFreeTruthA },
	{ "FirstDemoEndInit",			QuestInitDemoEnd },
	{ "FirstDemoEndFree",			QuestFreeDemoEnd },
	{ "WisdomChaosInit",			QuestInitWisdomChaos },
	{ "WisdomChaosFree",			QuestFreeWisdomChaos },
	{ "BooksInit",					QuestInitBooks },
	{ "BooksFree",					QuestFreeBooks },
	{ "StitchAInit",				QuestInitStitchA },
	{ "StitchAFree",				QuestFreeStitchA },
	{ "StitchAOnEnterGame",			s_QuestOnEnterGameStitchA },
	{ "StitchBInit",				QuestInitStitchB },
	{ "StitchBFree",				QuestFreeStitchB },
	{ "StitchBOnEnterGame",			s_QuestOnEnterGameStitchB },
	{ "StitchCInit",				QuestInitStitchC },
	{ "StitchCFree",				QuestFreeStitchC },
	{ "ChocolateInit",				QuestInitChocolate },
	{ "StitchAInit",				QuestInitStitchA },
	{ "StitchAFree",				QuestFreeStitchA },
	{ "ChocolateFree",				QuestFreeChocolate },
	{ "HoldFastInit",				QuestInitHoldFast },
	{ "HoldFastFree",				QuestFreeHoldFast },
	{ "RedoutInit",					QuestInitRedout },
	{ "RedoutFree",					QuestFreeRedout },
	{ "Mind314Init",				QuestInitMind314 },
	{ "Mind314Free",				QuestFreeMind314 },
	{ "Mind314Version",				s_QuestMind314Version },	
	{ "ToTempleInit",				QuestInitToTemple },
	{ "ToTempleFree",				QuestFreeToTemple },
	{ "FirstSampleInit",			QuestInitFirstSample },
	{ "FirstSampleFree",			QuestFreeFirstSample },
	{ "WormHuntInit",				QuestInitWormHunt },
	{ "WormHuntFree",				QuestFreeWormHunt },
	{ "BigGundownInit",				QuestInitBigGundown },
	{ "BigGundownFree",				QuestFreeBigGundown },
	{ "SecondSampleInit",			QuestInitSecondSample },
	{ "SecondSampleFree",			QuestFreeSecondSample },
	{ "CleanupInit",				QuestInitCleanup },
	{ "CleanupFree",				QuestFreeCleanup },
	{ "Truth_CInit",				QuestInitTruthC },
	{ "Truth_CFree",				QuestFreeTruthC },
	{ "HelpRescueInit",				QuestInitHelpRescue },
	{ "HelpRescueFree",				QuestFreeHelpRescue },
	{ "RiftScanInit",				QuestInitRiftScan },
	{ "RiftScanFree",				QuestFreeRiftScan },
	{ "SecretMessagesInit",			QuestInitSecretMessages },
	{ "SecretMessagesFree",			QuestFreeSecretMessages },
	{ "LureInit",					QuestInitLure },
	{ "LureFree",					QuestFreeLure },
	{ "PortalBoostInit",			QuestInitPortalBoost },
	{ "PortalBoostFree",			QuestFreePortalBoost },
	{ "TrainFixInit",				QuestInitTrainFix },
	{ "TrainFixFree",				QuestFreeTrainFix },
	{ "TrainFixOnEnterGame",		s_QuestTrainFixOnEnterGame},
	{ "RescueRideInit",				QuestInitRescueRide },
	{ "RescueRideFree",				QuestFreeRescueRide },
	{ "Truth_DInit",				QuestInitTruthD },
	{ "Truth_DFree",				QuestFreeTruthD },
	{ "TriageInit",					QuestInitTriage },
	{ "TriageFree",					QuestFreeTriage },
	{ "DeepInit",					QuestInitDeep },
	{ "DeepFree",					QuestFreeDeep },
	{ "TheSigilInit",				QuestInitTheSigil },
	{ "TheSigilFree",				QuestFreeTheSigil },
	{ "TestKnowledgeInit",			QuestInitTestKnowledge },
	{ "TestKnowledgeFree",			QuestFreeTestKnowledge },
	{ "TestLeadershipInit",			QuestInitTestLeadership },
	{ "TestLeadershipFree",			QuestFreeTestLeadership },
	{ "TestBeautyInit",				QuestInitTestBeauty },
	{ "TestBeautyFree",				QuestFreeTestBeauty },
	{ "TestFellowshipInit",			QuestInitTestFellowship },
	{ "TestFellowshipFree",			QuestFreeTestFellowship },
	{ "TestHopeInit",				QuestInitTestHope },
	{ "TestHopeFree",				QuestFreeTestHope },
	{ "Truth_EInit",				QuestInitTruthE },
	{ "Truth_EFree",				QuestFreeTruthE },
	{ "HellgateInit",				QuestInitHellgate },
	{ "HellgateFree",				QuestFreeHellgate },

	// others
	{ "JoeyInit",					QuestInitJoey },
	{ "JoeyFree",					QuestFreeJoey },
	{ "TimeOrbInit",				QuestInitTimeOrb },
	{ "TimeOrbFree",				QuestFreeTimeOrb },
	{ "DarkInit",					QuestInitDark },
	{ "DarkFree",					QuestFreeDark },
	{ "WarInit",					QuestInitWar },
	{ "WarFree",					QuestFreeWar },
	{ "DeathInit",					QuestInitDeath },
	{ "DeathFree",					QuestFreeDeath },	

	// template quests		
	{ "HuntInit",					QuestHuntInit},
	{ "CollectInit",				QuestCollectInit},
	{ "InfestationInit",			QuestInfestationInit},
	{ "ExploreInit",				QuestExploreInit},
	{ "OperateInit",				QuestOperateInit},
	{ "EscortInit",					QuestEscortInit},
	{ "TalkInit",					QuestTalkInit},
	{ "UseItemInit",				QuestInitUseItem},
	{ "ListenInit",					QuestListenInit},
	{ "ListenFree",					QuestListenFree},

	{ "HuntFree",					QuestHuntFree},
	{ "CollectFree",				QuestCollectFree},
	{ "InfestationFree",			QuestInfestationFree},
	{ "ExploreFree",				QuestExploreFree},
	{ "OperateFree",				QuestOperateFree},
	{ "EscortFree",					QuestEscortFree},
	{ "TalkFree",					QuestTalkFree},
	{ "UseItemFree",				QuestFreeUseItem},

	{ "HuntOnEnterGame",			s_QuestHuntOnEnterGame },
	{ "CollectOnEnterGame",			s_QuestCollectOnEnterGame },
	{ "EscortOnEnterGame",			s_QuestEscortOnEnterGame },
	{ "OperateOperate",				QuestOperateComplete},

	// Patch 1 quests
	{ "CubeInit",					QuestInitCube},
	{ "CubeFree",					QuestFreeCube},
	{ "StonehengeInit",				QuestInitStonehenge},
	{ "StonehengeFree",				QuestFreeStonehenge},
	{ "StonehengeOnEnterGame",		s_QuestStonehengeOnEnterGame},
	{ "DonateInit",					QuestDonateInit},
	{ "DonateFree",					QuestDonateFree},
	
	// Patch 2 quests
	{ "AbyssInit",					QuestInitAbyss},
	{ "AbyssFree",					QuestFreeAbyss},
	{ "AbyssOnEnterGame",			s_QuestAbyssOnEnterGame},
#endif
	{ "",							NULL }  // keep this last	
	
};
static int sgnNumQuestFunctions = arrsize( sgtQuestFunctionLookup );

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppQuestGlobalInit( 
	APP_QUEST_GLOBAL *pAppQuestGlobal)
{
	ASSERTX_RETURN( pAppQuestGlobal, "Expected app quest global" );
	
	pAppQuestGlobal->nNumInterstingGI = 0;
	for (int i = 0; i < MAX_QUEST_INTERESTING_GI; ++i)
	{
		pAppQuestGlobal->eInterestingGI[ i ] = GI_INVALID;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *QuestGetDevName(
	QUEST *pQuest)
{
	static const char *szUnknown = "Unknown";
	ASSERTX_RETVAL( pQuest, szUnknown, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	if (pQuestDef)
	{
		return pQuestDef->szName;
	}
	return szUnknown;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestsInitForApp(
	void)
{
	ASSERTX_RETURN( pAppAllQuestGlobals == NULL, "App quest globals should be NULL" );
	
	// allocate globals
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	pAppAllQuestGlobals = (APP_ALL_QUEST_GLOBALS *)MALLOCZ( g_StaticAllocator, sizeof( APP_ALL_QUEST_GLOBALS ) );
#else
	pAppAllQuestGlobals = (APP_ALL_QUEST_GLOBALS *)MALLOCZ( NULL, sizeof( APP_ALL_QUEST_GLOBALS ) );
#endif		
	
	// init, these are the same for all players and difficulty levels
	for (int i = 0; i < NUM_QUESTS; ++i)
	{
		APP_QUEST_GLOBAL *pAppQuestGlobal = &pAppAllQuestGlobals->tQuestGlobal[ i ];
		sAppQuestGlobalInit( pAppQuestGlobal );		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestsFreeForApp(
	void)
{
	ASSERTX_RETURN( pAppAllQuestGlobals != NULL, "No app quest globals found" );
	
	// free globals
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	FREE( g_StaticAllocator, pAppAllQuestGlobals );
#else
	FREE( NULL, pAppAllQuestGlobals );
#endif		
	
	pAppAllQuestGlobals = NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_QUEST_GLOBAL *QuestGetAppQuestGlobal(
	int nQuest)
{
	ASSERTX_RETNULL( nQuest >= 0 && nQuest < NUM_QUESTS, "Invalid quest" );
	ASSERTX_RETNULL( pAppAllQuestGlobals, "Expected app quest globals" );
	return &pAppAllQuestGlobals->tQuestGlobal[ nQuest ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_GLOBALS * QuestGlobalsGet(
	UNIT * player )
{
	ASSERTX_RETNULL( player, "Can't get quest globals, invalid game param" );
	ASSERTX_RETNULL( player->m_pQuestGlobals, "Can't get quest globals, globals in game are NULL" );
	return player->m_pQuestGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static PFN_QUEST_FUNCTION sLookupQuestFunction( 
	const char *pszName)
{
	ASSERTX_RETNULL( pszName, "Expected string" );
	
	// must have something to lookup
	if (pszName[ 0 ] != 0)
	{
		
		// scan through table
		for (int i = 0; i < sgnNumQuestFunctions; ++i)
		{
			const QUEST_FUNCTION_LOOKUP *pLookup = &sgtQuestFunctionLookup[ i ];
			if (PStrICmp( pLookup->pszName, pszName ) == 0)
			{
				return pLookup->pfnFunction;
			}			
			
		}
		
	}
	
	return NULL;  // not found (or not specified)
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestFunctionTableLookup(
	QUEST_FUNCTION_TABLE &tFunctions)
{
	for (int i = 0; i < QFUNC_NUM_FUNCTIONS; ++i)
	{
		QUEST_FUNCTION *pFunction = &tFunctions.tTable[ i ];
		pFunction->pfnFunction = sLookupQuestFunction( pFunction->szFunctionName );	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestTemplateLookupFunctions(
	QUEST_TEMPLATE_DEFINITION *pQuestTemplateDef)
{
	ASSERTX_RETURN( pQuestTemplateDef, "Expected quest template" );
	sQuestFunctionTableLookup( pQuestTemplateDef->tFunctions );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	for (unsigned int nQuest = 0; nQuest < ExcelGetCountPrivate(table); ++nQuest)
	{
		QUEST_DEFINITION * ptQuestDef = (QUEST_DEFINITION *)ExcelGetDataPrivate(table, nQuest);
		if (ptQuestDef)
		{
		
			// lookup functions
			sQuestFunctionTableLookup( ptQuestDef->tFunctions );			
			ptQuestDef->pfnVersionFunction = sLookupQuestFunction( ptQuestDef->szVersionFunction );

			// rewarder override
			if ( ( ptQuestDef->nCastRewarder == INVALID_LINK ) && ( ptQuestDef->eStyle != QS_STORY ) )
			{
				ptQuestDef->nCastRewarder = ptQuestDef->nCastGiver;
			}
			
		}
		
	}

	return TRUE;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_DEFINITION *QuestGetDefinition( 
	int nQuest)
{
	return (const QUEST_DEFINITION *)ExcelGetData( NULL, DATATABLE_QUEST, nQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_STATUS_DEFINITION *QuestStatusGetDefinition(
	QUEST_STATUS eStatus)
{
	return (const QUEST_STATUS_DEFINITION *)ExcelGetData( NULL, DATATABLE_QUEST_STATUS, eStatus );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_DEFINITION *QuestGetDefinition( 
	const QUEST *pQuest)
{
	return QuestGetDefinition( pQuest->nQuest );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_STATE_DEFINITION *QuestStateGetDefinition( 
	int nQuestState)
{
	return (const QUEST_STATE_DEFINITION *)ExcelGetData( NULL, DATATABLE_QUEST_STATE, nQuestState );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const QUEST_CAST_DEFINITION *QuestCastGetDefinition( 
	int nCast)
{
	return (const QUEST_CAST_DEFINITION *)ExcelGetData( NULL, DATATABLE_QUEST_CAST, nCast );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInit(
	QUEST *pQuest,
	UNIT *pPlayer,
	int nQuestDef,
	int nDifficulty)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( nDifficulty != INVALID_LINK, "Expected difficulty" );

	// get quest definition (may be NULL)!!!!!	
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( nQuestDef );
	
	// CAREFUL! QUEST DEF MAY BE NULL! Not all quests are defined for all packages, and QUEST instances are allocated in a fixed size block of memory
	if (pQuestDef)
	{
		pQuest->nVersion = QuestDataGetCurrentVersion();
		pQuest->nStringKeyName = pQuestDef->nStringKeyName ;
	}
	else
	{
		pQuest->nVersion = 0;
		pQuest->nStringKeyName = INVALID_LINK;
	}
	
	pQuest->pQuestDef = pQuestDef;	
	pQuest->nQuest = nQuestDef;
	pQuest->nDifficulty = nDifficulty;
	pQuest->eStatus = QUEST_STATUS_INACTIVE;
	pQuest->pPlayer = pPlayer;
	pQuest->pfnMessageHandler = NULL;
	pQuest->dwQuestMessageFlags = 0;
	pQuest->pQuestData = NULL;
	pQuest->pStates = NULL;
	pQuest->nNumStates = 0;
	pQuest->nQuestStateActiveLog = INVALID_LINK;
	pQuest->dwQuestFlags = 0;
	pQuest->nDelay = 0;
	
	CAST *pCast = &pQuest->tCast;
	pCast->nNumCastMembers = 0;

	if (pQuestDef)
	{
		// initialize the quest states
		QuestsStatesInit( pQuest );

		QuestTemplateInit( &pQuest->tQuestTemplate );
	}

	pQuest->pGameData = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestGlobalsInit(
	UNIT * player )
{
	ASSERT_RETURN( player );
	ASSERT_RETURN( player->m_pQuestGlobals );

	QUEST_GLOBALS * pQuestGlobals = player->m_pQuestGlobals;
	ASSERTX_RETURN( pQuestGlobals != NULL, "Expected non NULL quest globals" );
	if ( IS_SERVER( UnitGetGame( player ) ) )
	{
		pQuestGlobals->bQuestsRestored = FALSE;
	}
	else
	{
		pQuestGlobals->bQuestsRestored = TRUE;
	}

	for( int nDifficulty = 0; nDifficulty < NUM_DIFFICULTIES; ++nDifficulty)
		for( int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			// init all quests, whether their definitions are NULL or not (not in package)
			QUEST * pQuest = &pQuestGlobals->tQuests[ nQuest ][ nDifficulty ];
			QuestInit( pQuest, player, nQuest, nDifficulty );
		}

	pQuestGlobals->nLevelOldDef = INVALID_ID;
	pQuestGlobals->nLevelNewDef = INVALID_ID;
	pQuestGlobals->nLevelNewDRLGDef = INVALID_ID;

	pQuestGlobals->nNumActiveQuestDelays = 0;
	for ( int i = 0; i < MAX_QUEST_DELAYS_ACTIVE; i++ )
	{
		pQuestGlobals->nQuestDelayIds[i] = INVALID_ID;
		pQuestGlobals->nQuestDelayBit[i] = INVALID_ID;
	}

	pQuestGlobals->nActiveClientQuest = INVALID_LINK;
	pQuestGlobals->bAllInitialQuestStatesReceived = FALSE;	
	pQuestGlobals->nLevelNextStoryDef = INVALID_ID;
	pQuestGlobals->bStoryQuestActive = FALSE;

	ZeroMemory( pQuestGlobals->dwRoomsActivated, sizeof(pQuestGlobals->dwRoomsActivated) );
	pQuestGlobals->nDefaultSublevelRoomsActivated = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddWellKnownCastMember(
	QUEST *pQuest,
	int nCast)
{
	// must have a link
	if (nCast != INVALID_LINK)
	{
		SPECIES spCastMember = QuestCastGetSpecies( pQuest, nCast );
		GENUS eGenus = (GENUS)GET_SPECIES_GENUS( spCastMember );
		int nClass = (int)GET_SPECIES_CLASS( spCastMember );	
		s_QuestAddCastMember( pQuest, eGenus, nClass );	
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestsInit(
	UNIT * player )
{
	GAME *pGame = UnitGetGame( player );
	
	// allocate globals
	ASSERTX_RETURN( player->m_pQuestGlobals == NULL, "Expected NULL quest globals" );	
	
	// initialize globals
	player->m_pQuestGlobals = (QUEST_GLOBALS *)GMALLOCZ( pGame, sizeof( QUEST_GLOBALS ) );
	sQuestGlobalsInit( player );

	// init all quests
	for( int nDifficulty = 0; nDifficulty < NUM_DIFFICULTIES; ++nDifficulty)
	{
		for( int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			// init all quests, whether their definitions are NULL or not (not in package)
			QUEST * pQuest = QuestGetQuestByDifficulty( player, nQuest, nDifficulty );
		
			if (!pQuest)
				continue;
			const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( nQuest );
			if (!pQuestDef)
				continue;
					
			// run custom initialization		
			QUEST_FUNCTION_PARAM tParam;
			tParam.pQuest = pQuest;
			tParam.pPlayer = player;
			QuestRunFunction( pQuest, QFUNC_INIT, tParam );
						
			// add some well known cast members
			if (IS_SERVER( pGame ))
			{
				sAddWellKnownCastMember( pQuest, pQuestDef->nCastGiver );
				sAddWellKnownCastMember( pQuest, pQuestDef->nCastRewarder );

				// go through all the states and add gossip people as cast members as well
				if ( AppIsHellgate() )
				{
					for (int i = 0; i < pQuest->nNumStates; ++i)
					{
						QUEST_STATE * pState = QuestStateGetByIndex( pQuest, i );
						if ( pState )
						{
							const QUEST_STATE_DEFINITION * pStateDef = QuestStateGetDefinition( pState->nQuestState );
							if ( pStateDef )
							{
								// check all gossip columns
								for ( int i = 0; i < MAX_QUEST_GOSSIPS; i++ )
								{
									if ( pStateDef->nGossipNPC[i] != INVALID_LINK )
										s_QuestAddCastMember( pQuest, GENUS_MONSTER, pStateDef->nGossipNPC[i] );
								}
							}
						}
					}	
				}
			}

			// do some error checking
#if ISVERSION(DEVELOPMENT)
			if (pQuestDef->nGiverItem != INVALID_LINK)
			{
				const UNIT_DATA *pGiverItemData = UnitGetData( pGame, GENUS_ITEM, pQuestDef->nGiverItem );
				ASSERTX_CONTINUE( pQuestDef->szName, "expected quest name");
				ASSERTV_CONTINUE( pGiverItemData, "quest \"%s\": the \"Giver Item\" in quest.xls has the wrong version_package", pQuestDef->szName );
				ASSERTV_CONTINUE( pGiverItemData->szName, "quest \"%s\": the \"Giver Item\" has no name", pQuestDef->szName );
				ASSERTV_CONTINUE( UnitDataTestFlag(pGiverItemData, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE), "quest Giver Item \"%s\" needs \"InformQuestsToUse\" set to 1 in items.xls", pGiverItemData->szName );
			}
#endif //ISVERSION(DEVELOPMENT)
							
		}
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestsFree(
	UNIT * player )
{
	ASSERT_RETURN( player );
	GAME * pGame = UnitGetGame( player );
	ASSERT_RETURN( pGame );

	// get globals
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( player );
	ASSERTX_RETURN( pQuestGlobals != NULL, "Expected non NULL quest globals" );

	// run each quests shutdown function
	for( int nDifficulty = 0; nDifficulty < NUM_DIFFICULTIES; ++nDifficulty)
	{
		for( int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			QUEST * pQuest = QuestGetQuestByDifficulty( player, nQuest, nDifficulty );
			if (!pQuest)
				continue;
			const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( nQuest );		
			if (!ptQuestDef)
				continue;
			
			// don't try to shutdown a quest that hasn't been initialized yet - cmarch
			if (!pQuest->pfnMessageHandler && !pQuest->pQuestData)
				continue;

			// make sure the quest games are cleaned up
			if ( IS_SERVER( pGame ) && pQuest->pGameData )
				s_QuestGameLeave( pQuest );

			// run shutdown
			QUEST_FUNCTION_PARAM tParam;
			tParam.pQuest = pQuest;
			tParam.pPlayer = player;
			QuestRunFunction( pQuest, QFUNC_FREE, tParam );					

			// free the states
			if (pQuest->pStates)
			{
				QuestStatesFree( pQuest );
			}
		}
	}
	
	// free quest globals
	GFREE( pGame, player->m_pQuestGlobals );
	player->m_pQuestGlobals = NULL;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD QuestGetCode(
	QUEST *pQuest)
{
	ASSERTX_RETINVALID( pQuest, "Expected quest" );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	return ptQuestDef->wCode;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetIndexByCode(
	DWORD dwCode)
{
	return ExcelGetLineByCode(EXCEL_CONTEXT(), DATATABLE_QUEST, dwCode);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST *QuestGetQuestByDifficulty(
	UNIT *player,
	int nQuest,
	int nDifficulty,
	BOOL bAutomaticallyVersion /*= TRUE*/)
{
	ASSERTX_RETNULL( nQuest > INVALID_LINK && nQuest < NUM_QUESTS, "Invalid quest enum" );
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( player );
	ASSERTX_RETNULL( pQuestGlobals != NULL, "Expected non NULL quest globals" );
	ASSERTX_RETNULL( nDifficulty >= 0 && nDifficulty < NUM_DIFFICULTIES, "Illegal current game difficulty for player" );
	QUEST * pQuest = &pQuestGlobals->tQuests[ nQuest ][ nDifficulty ];
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	
	// cut off access to quests not in the current package
	if (pQuest->pQuestDef == NULL)
	{
		pQuest = NULL;
	}

	// version quest before returning if necessary
	if (pQuest)
	{
		if (bAutomaticallyVersion == TRUE)
		{
			GAME *pGame = QuestGetGame( pQuest );	
			if (IS_SERVER( pGame ))
			{
				s_QuestVersionData( pQuest );
			}
		}
	}
	
	return pQuest;		

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST *QuestGetQuest(
	UNIT *player,
	int nQuest,
	BOOL bAutomaticallyVersion /*= TRUE*/)

{
	int nDifficulty = UnitGetStat( player, STATS_DIFFICULTY_CURRENT );
	return QuestGetQuestByDifficulty( player, nQuest, nDifficulty, bAutomaticallyVersion );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetDifficulty(
	QUEST *pQuest)
{
	ASSERTX_RETINVALID( pQuest, "Expected quest" );
	ASSERTX_RETINVALID( pQuest->nDifficulty != INVALID_LINK, "Expected difficulty" );
	return pQuest->nDifficulty;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME *QuestGetGame( 
	QUEST *pQuest)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	return UnitGetGame( pPlayer );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *QuestGetPlayer(
	QUEST *pQuest)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	ASSERTX_RETNULL( pQuest->pPlayer, "Expected quest player unit" );
	return pQuest->pPlayer;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *QuestGetPlayerLevel(
	QUEST *pQuest)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	if (pPlayer)
	{
		return UnitGetLevel( pPlayer );
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPendingUnitTargetInit( 
	SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget)
{
	pSpawnableUnitTarget->speciesTarget = SPECIES_NONE;
	pSpawnableUnitTarget->nTargetUniqueName = MONSTER_UNIQUE_NAME_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestTemplateInit( 
	QUEST_TEMPLATE* pQuestTemplate)
{
	for (int i = 0; i < arrsize( pQuestTemplate->tSpawnableUnitTargets ); ++i)
	{
		SPAWNABLE_UNIT_TARGET* pSpawnableUnitTarget = &pQuestTemplate->tSpawnableUnitTargets[ i ];
		sPendingUnitTargetInit( pSpawnableUnitTarget );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestStateInit( 
	QUEST_STATE *pState,
	int nQuestState)
{
	ASSERT_RETURN( pState );
	pState->nQuestState = nQuestState;	
	pState->eValue = QUEST_STATE_HIDDEN;
	for ( int i = 0; i < MAX_QUEST_GOSSIPS; i++ )
		pState->bGossipComplete[i] = FALSE;
	#ifdef _DEBUG 
	pState->ptStateDef = QuestStateGetDefinition( nQuestState );
	#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetStatesForQuest( 
	QUEST *pQuest,
	int nQuestTemplate,
	int *pnQuestStates, 
	int nMaxStates)
{
	int nQuest = pQuest->nQuest;
	int nNumStates = 0;
	
	// go through the state table
	int nNumRows = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_QUEST_STATE);

	if (nQuestTemplate == INVALID_LINK)
	{
		// not using a quest template, use states with nQuest matching nQuest
		for (int i = 0; i < nNumRows; ++i)
		{
			const QUEST_STATE_DEFINITION *pQuestStateDef = QuestStateGetDefinition( i );
			if (pQuestStateDef && pQuestStateDef->nQuest == nQuest)
			{

				// must have room
				ASSERTX_RETVAL( nNumStates < nMaxStates - 1, nNumStates, "Too many states for quest" );
				pnQuestStates[ nNumStates++ ] = i;

			}

		}

	}
	else
	{
		// using a quest template, use states with nTemplate matching nTemplate
		for (int i = 0; i < nNumRows; ++i)
		{
			const QUEST_STATE_DEFINITION *pQuestStateDef = QuestStateGetDefinition( i );
			if (pQuestStateDef && pQuestStateDef->nQuestTemplate == nQuestTemplate)
			{

				// must have room
				ASSERTX_RETVAL( nNumStates < nMaxStates - 1, nNumStates, "Too many states for quest" );
				pnQuestStates[ nNumStates++ ] = i;

			}

		}

	}
	
	return nNumStates;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestStatesFree(
	QUEST *pQuest)
{			
	ASSERTX_RETURN( pQuest, "Expected quest" );
	GAME *pGame = QuestGetGame( pQuest );

	if (pQuest->pStates)
	{	
		GFREE( pGame, pQuest->pStates );
	}
	pQuest->pStates = NULL;
	pQuest->nNumStates = 0;		
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestsStatesInit(
	QUEST *pQuest)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );		
	UNIT *pPlayer = pQuest->pPlayer;
	GAME *pGame = UnitGetGame( pPlayer );

	// free states (happens when resetting)
	QuestStatesFree( pQuest );
	
	// get the states associated with this quest
	const int MAX_STATES_PER_QUEST = 64;
	int nQuestStates[ MAX_STATES_PER_QUEST ];
	int nNumStates = sGetStatesForQuest( pQuest, pQuest->pQuestDef->nTemplate, nQuestStates, MAX_STATES_PER_QUEST );
	
	// allocate states
	pQuest->pStates = (QUEST_STATE *)GMALLOCZ( pGame, sizeof( QUEST_STATE ) * nNumStates );
	pQuest->nNumStates = nNumStates;
			
	// init each state
	for (int i = 0; i < nNumStates; ++i)
	{
		int nQuestState = nQuestStates[ i ];
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		QuestStateInit( pState, nQuestState );						
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_STATE *QuestStateGetPointer(
	QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	
	// search states
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *pQuestState = &pQuest->pStates[ i ];
		ASSERTX_CONTINUE(pQuestState, "No quest state.");

		if (pQuestState->nQuestState == nQuestState)
		{
			return pQuestState;
		}
	}
	
	ASSERTX_RETNULL( 0, "State not found in quest" );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_STATE_VALUE QuestStateGetValue(
	QUEST *pQuest,
	int nQuestState)
{
	QUEST_STATE *pState = QuestStateGetPointer( pQuest, nQuestState );
	if (pState)
	{
		return QuestStateGetValue( pQuest, pState );
	}
	return QUEST_STATE_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_STATE_VALUE QuestStateGetValue(
	QUEST *pQuest,
	const QUEST_STATE *pState)
{
	ASSERTX_RETVAL( pQuest, QUEST_STATE_INVALID, "Expected quest" );
	ASSERTX_RETVAL( pState, QUEST_STATE_INVALID, "Expected state" );
	return pState->eValue;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestStateIsActive(
	QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	return QuestStateGetValue( pQuest, nQuestState ) == QUEST_STATE_ACTIVE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestStateIsHidden(
	QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	return QuestStateGetValue( pQuest, nQuestState ) == QUEST_STATE_HIDDEN;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestStateIsComplete(
	QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	return QuestStateGetValue( pQuest, nQuestState ) == QUEST_STATE_COMPLETE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestStateIsFailed(
	QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	return QuestStateGetValue( pQuest, nQuestState ) == QUEST_STATE_FAIL;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_STATE *QuestStateGetByIndex(
	const QUEST *pQuest,
	int nIndex)
{
	ASSERTX_RETNULL( pQuest, "Expected quest" );
	ASSERTX_RETNULL( nIndex >= 0 && nIndex < pQuest->nNumStates, "Quest state index out of range" );
	return &pQuest->pStates[ nIndex ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetStateIndex(
	const QUEST *pQuest,
	int nQuestState)
{
	ASSERTX_RETINVALID( pQuest, "Expected quest" );
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		const QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		if (pState->nQuestState == nQuestState)
		{
			return i;
		}
	}
	ASSERTX_RETINVALID( 0, "State not found in quest, can't get index" );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_STATUS QuestGetStatus(
	QUEST *pQuest)
{
	ASSERTX_RETVAL( pQuest, QUEST_STATUS_INVALID, "Expected quest" );
	return pQuest->eStatus;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestStatusIsGood(
	QUEST *pQuest)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERTX_RETFALSE( pQuestDef, "Expected quest definition" );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	const QUEST_STATUS_DEFINITION *pStatusDef = QuestStatusGetDefinition( eStatus );
	ASSERTX_RETFALSE( pStatusDef, "Expected quest def" );
	return pStatusDef->bIsGood;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsInactive(
	QUEST *pQuest)
{
	return QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsActive(
	QUEST *pQuest)
{
	return QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsComplete(
	QUEST *pQuest,
	BOOL bOrClosed )
{
	if ( bOrClosed )
	{
		QUEST_STATUS eStatus = QuestGetStatus( pQuest );
		return ( ( eStatus == QUEST_STATUS_COMPLETE ) || ( eStatus == QUEST_STATUS_CLOSED ) );
	}
	return QuestGetStatus( pQuest ) == QUEST_STATUS_COMPLETE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsOffering(
	QUEST *pQuest)
{
	return QuestGetStatus( pQuest ) == QUEST_STATUS_OFFERING;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsClosed(
	QUEST *pQuest)
{
	return QuestGetStatus( pQuest ) == QUEST_STATUS_CLOSED;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsFailed(
	QUEST *pQuest)
{
	return QuestGetStatus( pQuest ) == QUEST_STATUS_FAIL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestIsDone(
	QUEST *pQuest)
{
	if ( TESTBIT( pQuest->dwQuestFlags, QF_DELAY_NEXT ) )
	{
		return FALSE;
	}
	return QuestIsComplete( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
OPERATE_RESULT QuestsCanOperateObject(
	UNIT *pOperator,
	UNIT *pObject)
{
	ASSERTX_RETVAL( pObject, OPERATE_RESULT_FORBIDDEN, "Invalid target" );
	ASSERTX_RETVAL( UnitIsA( pObject, UNITTYPE_OBJECT ), OPERATE_RESULT_FORBIDDEN, "pObject is not an object" );
	GAME *pGame = UnitGetGame( pObject );

	// check debug flag for warps
	if (ObjectIsWarp( pObject ) == TRUE && GameGetDebugFlag( pGame, DEBUGFLAG_OPEN_WARPS ))
	{
		return OPERATE_RESULT_OK;
	}
		
	// setup message
	QUEST_MESSAGE_CAN_OPERATE_OBJECT tMessage;
	tMessage.idOperator = pOperator ? UnitGetId( pOperator ) : INVALID_ID;  // note that pOperate can be NULL
	tMessage.idObject = UnitGetId( pObject );

	// send to quests
	QUEST_MESSAGE_RESULT eResult = MetagameSendMessage( pOperator, QM_CAN_OPERATE_OBJECT, &tMessage );
	if (eResult == QMR_OPERATE_FORBIDDEN)
	{
		return OPERATE_RESULT_FORBIDDEN_BY_QUEST;
	}
	
	return OPERATE_RESULT_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
USE_RESULT QuestsCanUseItem(
	UNIT * pPlayer,
	UNIT * pItem )
{
	ASSERTX_RETVAL( pPlayer, USE_RESULT_INVALID, "Invalid Operator" );
	ASSERTX_RETVAL( UnitGetGenus( pPlayer ) == GENUS_PLAYER, USE_RESULT_INVALID, "Operator is not a player" );
	ASSERTX_RETVAL( pItem, USE_RESULT_INVALID, "Invalid Target" );
	ASSERTX_RETVAL( UnitGetGenus( pItem ) == GENUS_ITEM, USE_RESULT_INVALID, "Target is not an item" );
	const UNIT_DATA * data = ItemGetData( pItem );
	ASSERTX_RETVAL( UnitDataTestFlag(data, UNIT_DATA_FLAG_INFORM_QUESTS_TO_USE), USE_RESULT_INVALID, "Item is not quest usable" );

	if ( !InventoryIsInLocationType( pItem, LT_ON_PERSON ) ||
		 !UnitIsPhysicallyInContainer( pItem ) ||
		 !UnitIsContainedBy( pItem, pPlayer ) )
	{
		// the item needs to be in the player's inventory before they can use it
		return USE_RESULT_CONDITION_FAILED;
	}

	// setup message
	QUEST_MESSAGE_CAN_USE_ITEM tMessage;
	tMessage.idPlayer = UnitGetId( pPlayer );
	tMessage.idItem = UnitGetId( pItem );

	// send to quests
	QUEST_MESSAGE_RESULT eResult = MetagameSendMessage( pPlayer, QM_CAN_USE_ITEM, &tMessage );
	if ( eResult == QMR_USE_OK )
	{
		return USE_RESULT_OK;
	}
	else if ( eResult == QMR_USE_FAIL )
	{
		return USE_RESULT_CONDITION_FAILED;
	}

	return USE_RESULT_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestsUnitIsVisible(
	UNIT * pPlayer,
	UNIT * pUnit )
{
	ASSERTX_RETVAL( pPlayer, USE_RESULT_INVALID, "Invalid Operator" );
	ASSERTX_RETVAL( UnitGetGenus( pPlayer ) == GENUS_PLAYER, USE_RESULT_INVALID, "Operator is not a player" );
	ASSERTX_RETVAL( pUnit, USE_RESULT_INVALID, "Invalid Target" );
	const UNIT_DATA * data = UnitGetData( pUnit );
	ASSERTX_RETVAL( UnitDataTestFlag(data, UNIT_DATA_FLAG_ASK_QUESTS_FOR_VISIBLE), USE_RESULT_INVALID, "Unit not quest visible" );
	
	// setup message
	QUEST_MESSAGE_IS_UNIT_VISIBLE tMessage;
	tMessage.pUnit = pUnit;
	LEVEL * pLevel = UnitGetLevel( pUnit );
	tMessage.nLevelDefId = pLevel->m_nLevelDef;
	tMessage.nUnitClass = UnitGetClass( pUnit );

	// send to quests
	QUEST_MESSAGE_RESULT eResult;
	if ( AppIsHellgate() )
		// hellgate
		eResult = MetagameSendMessage( pPlayer, QM_IS_UNIT_VISIBLE, &tMessage, MAKE_MASK( QSMF_IS_UNIT_VISIBLE ) );
	else
		// tugboat
		eResult = MetagameSendMessage( pPlayer, QM_IS_UNIT_VISIBLE, &tMessage );
	return ( eResult == QMR_UNIT_VISIBLE ) || ( eResult == QMR_OK );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT QuestSendMessageToQuest(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	void *pMessage)
{	
	ASSERTX_RETVAL( pQuest, QMR_INVALID, "Expected quest" );

	// run message handler
	QUEST_MESSAGE_RESULT eResult = QMR_OK;
	
	if ( QuestsAreDisabled( QuestGetPlayer( pQuest ) ) )
	{
		return eResult;
	}
	
	if (pQuest->pfnMessageHandler)
	{
		eResult = pQuest->pfnMessageHandler( pQuest, eMessageType, pMessage );		
	}
	
	// run default handler if result is OK
	if (eResult == QMR_OK)
	{
		eResult = s_QuestsDefaultMessageHandler( pQuest, eMessageType, pMessage );
	}

	// return the result	
	return eResult;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT QuestSendMessage(
	UNIT * pPlayer,
	QUEST_MESSAGE_TYPE eMessageType,
	void *pMessage,
	DWORD dwFlags )
{
	ASSERT_RETVAL(pPlayer, QMR_INVALID);
	
	if( AppIsTugboat() )
	{
		return QuestHandleQuestMessage( pPlayer, eMessageType, pMessage );	
	}	

	if ( QuestsAreDisabled( pPlayer ) )
	{
		return QMR_OK;
	}

	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETVAL(pQuestGlobals, QMR_INVALID);

	// go through each quest
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if (!pQuest)
			continue;

		if ( dwFlags != QSMF_ALL )
		{
			if ( TESTBIT( dwFlags, QSMF_STORY_ONLY ) && ( pQuest->pQuestDef->eStyle != QS_STORY ) )
				continue;

			if ( TESTBIT( dwFlags, QSMF_FACTION_ONLY ) && ( pQuest->pQuestDef->eStyle != QS_FACTION ) )
				continue;

			if ( TESTBIT( dwFlags, QSMF_APPROACH_RADIUS ) && !TESTBIT( pQuest->dwQuestMessageFlags, QSMF_APPROACH_RADIUS ) )
				continue;

			if ( TESTBIT( dwFlags, QSMF_BACK_FROM_MOVIE ) && !TESTBIT( pQuest->dwQuestMessageFlags, QSMF_BACK_FROM_MOVIE ) )
				continue;

			if ( TESTBIT( dwFlags, QSMF_IS_UNIT_VISIBLE ) && !TESTBIT( pQuest->dwQuestMessageFlags, QSMF_IS_UNIT_VISIBLE ) )
				continue;
		}

		// send it
		QUEST_MESSAGE_RESULT eResult = QuestSendMessageToQuest( pQuest, eMessageType, pMessage );		
		
		// if result was use, return it and don't pass to any more quests
		// (we may want to setup a table in the future to govern the continuation action
		// based on result code)
		if (eResult != QMR_OK)
		{
			return eResult;
		}
					
	}
	
	return QMR_OK;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestSendState(
	QUEST *pQuest,
	int nQuestState,
	BOOL bRestore /*= FALSE*/)
{

	// don't send messages if restoring
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
	if ( !pQuestGlobals )
	{
		return;
	}
	if ( !pQuestGlobals->bQuestsRestored )
	{
		return;
	}

	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( pGame );

	QUEST_STATE *pState = QuestStateGetPointer( pQuest, nQuestState );
	
	if ( IS_SERVER( pGame ) )
	{
		// setup state message
		MSG_SCMD_QUEST_STATE tMessage;
		tMessage.nQuest = pQuest->nQuest;
		tMessage.nQuestState = nQuestState;
		tMessage.nValue = pState->eValue;
		tMessage.bRestore = (BYTE)bRestore;
	
		// send to sigle client
		GAMECLIENTID idClient = UnitGetClientId( pQuest->pPlayer );
		s_SendMessage( pGame, idClient, SCMD_QUEST_STATE, &tMessage );
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		// send message to the server
		MSG_CCMD_CLIENT_QUEST_STATE_CHANGE tMessage;
		tMessage.nQuest = pQuest->nQuest;
		tMessage.nQuestState = nQuestState;
		tMessage.nValue = pState->eValue;
		
		c_SendMessage( CCMD_CLIENT_QUEST_STATE_CHANGE, &tMessage );
#endif// !ISVERSION(SERVER_VERSION)
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestStateSet( 
	QUEST *pQuest, 
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	BOOL bUpdateCastInfo /*= TRUE*/,
	BOOL bIgnoreStatus /*= FALSE*/,
	BOOL bSetAutoTrack /*= TRUE*/)
{
	ASSERT_RETFALSE(pQuest);

	// this will not have the intended effect if this is being done before the quests
	// are setup from any save file data
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETFALSE( pPlayer, "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	if (IS_SERVER( pGame ))
	{
		QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
		ASSERTX_RETFALSE( pQuestGlobals, "Expected quest globals" );
		ASSERTV( 
			TESTBIT( pQuestGlobals->dwQuestGlobalFlags, QGF_QUESTS_SETUP ),
			"Player '%s' unable to set quest '%s' state value because quests are not yet setup!",
			UnitGetDevName( pPlayer ),
			QuestGetDevName( pQuest ));
	}
	
	// only active quests can set state
	if (bIgnoreStatus == TRUE || QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE || QuestGetStatus( pQuest ) == QUEST_STATUS_OFFERING)
	{
	
		QUEST_STATE *pState = QuestStateGetPointer( pQuest, nQuestState );
		ASSERTX_RETFALSE( pState, "State not found in quest" );
		if (pState->eValue != eValue)
		{
		
			// if state is complete, you can only hide them 
			if (pState->eValue == QUEST_STATE_COMPLETE && eValue != QUEST_STATE_HIDDEN)
			{
				return FALSE;
			}
			
			// set new value for state
			pState->eValue = eValue;
					
			// send state to client/server
			QuestSendState( pQuest, nQuestState );
	
			// server logic
			if (IS_SERVER( QuestGetGame( pQuest ) ))
			{
			
				// update cast info if requested, we should really only do this once per
				// frame instead of every time we change state values because we typically
				// change multiple states in the same frame ... but this data is small
				// and *very* infrequent so we might not ever care
				if (bUpdateCastInfo)
				{
					s_QuestUpdateAvailability( pQuest );
				}

			}

			// if we just completed a quest state, and the entire quest is not complete, turn on tracking
			const QUEST_STATE_DEFINITION *pStateDef = QuestStateGetDefinition( nQuestState );
			ASSERT(pStateDef);
			if (eValue == QUEST_STATE_COMPLETE && pStateDef && pStateDef->bAutoTrackOnComplete && bSetAutoTrack)
			{
				if (!QuestIsComplete(pQuest))
				{
					QuestAutoTrack(pQuest);
				}
			}
									
			return TRUE;  // state was set
				
		}

	}
		
	return FALSE;  // state was not set
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SPECIES QuestCastGetSpecies(
	QUEST *pQuest,
	int nCast)
{
	ASSERTX_RETVAL( pQuest, SPECIES_NONE, "Expected quest" );
	ASSERTX_RETVAL( nCast != INVALID_LINK, SPECIES_NONE, "Expected cast index" );
	const UNIT *pPlayer = QuestGetPlayer( pQuest );
	
	// find cast for player unit type
	int nMonsterClass = INVALID_LINK;
	const QUEST_CAST_DEFINITION *ptCastDef = QuestCastGetDefinition( nCast );			
	for (int i = 0; i < MAX_QUEST_CAST; ++i)
	{
		const QUEST_CAST_OPTION *pOption = &ptCastDef->tCastOptions[ i ];
		if (pOption->nMonsterClass != INVALID_LINK)
		{
			if (pOption->nUnitType == INVALID_LINK ||
				UnitIsA( pPlayer, pOption->nUnitType ))
			{
				nMonsterClass = pOption->nMonsterClass;
			}
			
		}
		
	}
	
	// for now, all cast members definied by the cast definition are monsters (we can
	// change that later if we want to tho)
	return MAKE_SPECIES( GENUS_MONSTER, nMonsterClass );
	
}

//----------------------------------------------------------------------------
// Gets treasure UNITs from quest dialog storage, if any exist.
// Returns number of treasure UNITs found.
//----------------------------------------------------------------------------
static int sQuestDialogGetExistingRewardItems(
	UNIT *pPlayer,
	QUEST *pQuest, 
	ITEMSPAWN_RESULT &tResult)
{
	ASSERTX_RETVAL( pPlayer, 0, "Expected player" );
	ASSERTX_RETVAL( pQuest, 0, "Expected quest" );
	ASSERTX_RETVAL( tResult.nTreasureBufferSize >= MAX_OFFERINGS, 0, "UNIT return buffer should be at least MAX_OFFERINGS in length" );

	tResult.nTreasureBufferCount = 0;

	// iterate *entire* player inventory
	int nOffer = pQuest->pQuestDef->nOfferReward;	// only items for one offer should be in the dialog storage, but checking this won't hurt
	int nInventorySlot = GlobalIndexGet( GI_INVENTORY_LOCATION_QUEST_DIALOG_REWARD );	
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	UNIT *pItem = NULL;
	while ( ( pItem = UnitInventoryLocationIterate( pPlayer, nInventorySlot, pItem ) ) != NULL )
	{
		// did this item come from an offer of this type
		if (UnitGetStat( pItem, STATS_OFFERID, nDifficulty ) == nOffer)
		{
			ASSERTX_RETVAL(tResult.nTreasureBufferCount < tResult.nTreasureBufferSize, tResult.nTreasureBufferCount, "Too many offer items to fit in return buffer" );
			tResult.pTreasureBuffer[tResult.nTreasureBufferCount++] = pItem;
		}
	}
	return tResult.nTreasureBufferCount;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetExistingRewardItems( 
	QUEST *pQuest, 
	ITEMSPAWN_RESULT &tResult )
{
	ASSERTX_RETVAL( pQuest, 0, "Expected quest" );
	ASSERTX_RETVAL( tResult.nTreasureBufferSize >= MAX_OFFERINGS, 0, "UNIT return buffer should be at least MAX_OFFERINGS in length" );

	tResult.nTreasureBufferCount = 0;

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	int nOffer = pQuest->pQuestDef->nOfferReward;
	if ( nOffer != INVALID_ID )
	{
		// Get the existing offer items, then check if they are associated
		// with the quest.
		UNIT* pItems[ MAX_OFFERINGS ];
		ITEMSPAWN_RESULT tOfferItems;
		tOfferItems.pTreasureBuffer = pItems;
		tOfferItems.nTreasureBufferSize = arrsize( pItems);
		sQuestDialogGetExistingRewardItems( pPlayer, pQuest, tOfferItems );

		for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
		{
			UNIT* pReward = tOfferItems.pTreasureBuffer[i];
			int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
			if ( UnitGetStat( pReward, STATS_QUEST_REWARD, nDifficulty ) == pQuest->nQuest )
			{
				ASSERTX_RETVAL(tResult.nTreasureBufferCount < tResult.nTreasureBufferSize, tResult.nTreasureBufferCount, "Too many offer items to fit in return buffer" );
				tResult.pTreasureBuffer[tResult.nTreasureBufferCount++] = pReward;
			}
		}
	}
	return tResult.nTreasureBufferCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetExperienceReward( QUEST *pQuest )
{
	ASSERT_RETVAL( pQuest != NULL, 0 );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERT_RETVAL( pPlayer != NULL, 0 );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETVAL( ptQuestDef != NULL, 0 );

	// calculate reward, using monster level data, at
	// the quest level, as the base
	int nDifficulty = pQuest->nDifficulty;
	if (nDifficulty < 0 || nDifficulty >= arrsize( ptQuestDef->nLevel ))
	{
		nDifficulty = DIFFICULTY_NORMAL;
		ASSERTX(FALSE, "Illegal difficulty, defaulting to normal difficulty rewards.");
	}

	float fExperience = 0.0f;
	const MONSTER_LEVEL * monlvldata = 
		MonsterLevelGetData( UnitGetGame( pPlayer ), ptQuestDef->nLevel[ nDifficulty ] );
	if ( monlvldata != NULL )
	{
		fExperience = ( float ) monlvldata->nExperience;
		fExperience *= ptQuestDef->fExperienceMultiplier;
	}
	return ( ( int ) fExperience );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetMoneyReward( QUEST *pQuest )
{
	ASSERT_RETVAL( pQuest != NULL, 0 );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERT_RETVAL( pPlayer != NULL, 0 );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETVAL( ptQuestDef != NULL, 0 );

	// calculate reward, using monster level data, at
	// the quest level, as the base
	int nDifficulty = pQuest->nDifficulty;
	if (nDifficulty < 0 || nDifficulty >= arrsize( ptQuestDef->nLevel ))
	{
		nDifficulty = DIFFICULTY_NORMAL;
		ASSERTX(FALSE, "Illegal difficulty, defaulting to normal difficulty rewards.");
	}

	float fMoney = 0.0f;
	const MONSTER_LEVEL * monlvldata = 
		MonsterLevelGetData( UnitGetGame( pPlayer ), ptQuestDef->nLevel[ nDifficulty ] );
	if ( monlvldata != NULL )
	{
		fMoney = ( float ) monlvldata->nMoneyMin;
		fMoney += ( ( float ) monlvldata->nMoneyDelta ) / 2.0f;
		fMoney *= ptQuestDef->fMoneyMultiplier;
	}
	return ( ( int ) fMoney );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetStatPointReward( QUEST *pQuest )
{
	ASSERT_RETVAL( pQuest != NULL, 0 );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERT_RETVAL( pPlayer != NULL, 0 );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETVAL( ptQuestDef != NULL, 0 );

	return ptQuestDef->nStatPoints;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetSkillPointReward( QUEST *pQuest )
{
	ASSERT_RETVAL( pQuest != NULL, 0 );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERT_RETVAL( pPlayer != NULL, 0 );
	const QUEST_DEFINITION *ptQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETVAL( ptQuestDef != NULL, 0 );

	return ptQuestDef->nSkillPoints;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoTruthAutoTalk(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	int nQuest = (int)tEventData.m_Data1;
	GLOBAL_INDEX eGITalkToAfterTransport = (GLOBAL_INDEX)tEventData.m_Data2;
	int nDialogToSpeakAfterTransport = (int)tEventData.m_Data3;

	// get quest and who will be talking
	QUEST *pQuest = QuestGetQuest( pUnit, nQuest );
	int nClassAutoTalk = GlobalIndexGet( eGITalkToAfterTransport );
	GENUS eGenusAutoTalk = GlobalIndexGetUnitGenus( eGITalkToAfterTransport );
	SPECIES spAutoTalkTo = MAKE_SPECIES( eGenusAutoTalk, nClassAutoTalk );
	UNIT *pAutoTalkingTo = s_QuestCastMemberGetInstanceBySpecies( pQuest, spAutoTalkTo );
	if (pAutoTalkingTo)
	{
		s_NPCQuestTalkStart( pQuest, pAutoTalkingTo, NPC_DIALOG_OK, nDialogToSpeakAfterTransport );	
	}

	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoTruthTransport(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pUnit ), "Expected player" );
	UNIT *pPlayer = pUnit;
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypesObject[] = { TARGET_OBJECT, TARGET_INVALID };

	// get what to say after being transported
	int nQuest = (int)tEventData.m_Data1;
	GLOBAL_INDEX eGITalkToAfterTransport = (GLOBAL_INDEX)tEventData.m_Data2;
	int nDialogToSpeakAfterTransport = (int)tEventData.m_Data3;
	
	// clear the truth revealling state
	s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_TRUTH_REVEALING, 0 );
								
	// get our current sub level
	SUBLEVELID idSubLevel = UnitGetSubLevelID( pPlayer );
	const SUBLEVEL *pSubLevel = SubLevelGetById( pLevel, idSubLevel );
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );

	// find the portal that started this whole thing
	UNIT *pExistingEntry = LevelFindFirstUnitOf( pLevel, eTargetTypesObject, GENUS_OBJECT, ptSubLevelDef->nObjectEntrance );	
	ASSERTX_RETFALSE( pExistingEntry, "Expected entry object" );

	// what is the next sublevel
	int nSubLevelDefNext = ptSubLevelDef->nSubLevelDefNext;
	ASSERTX_RETFALSE( nSubLevelDefNext != INVALID_LINK, "Expected sub level definition" );

	// find the sublevel in this level
	SUBLEVELID idSubLevelNext = SubLevelFindFirstOfType( pLevel, nSubLevelDefNext );
	ASSERTX_RETFALSE( idSubLevelNext != INVALID_ID, "Can't find next sub level for truth rooms" );
	const SUBLEVEL *pSubLevelNext = SubLevelGetById( pLevel, idSubLevelNext );
	const SUBLEVEL_DEFINITION *ptSubLevelDefNext = SubLevelGetDefinition( pSubLevelNext );

	// what's the entrance object for the next sublevel
	int nObjectEntrance = ptSubLevelDefNext->nObjectEntrance;
	ASSERTX_RETFALSE( nObjectEntrance != INVALID_LINK, "Expected entrance object" );
	
	// create entrance object (only if it doesn't exist already)
	UNIT *pEntrance = LevelFindFirstUnitOf( pLevel, eTargetTypesObject, GENUS_OBJECT, nObjectEntrance );		
	if (pEntrance == NULL)
	{	
		VECTOR vDirection = UnitGetFaceDirection( pExistingEntry, FALSE );
		OBJECT_SPEC tSpec;
		tSpec.nClass = nObjectEntrance;
		tSpec.pRoom = UnitGetRoom( pExistingEntry );
		tSpec.vPosition = UnitGetPosition( pExistingEntry );
		tSpec.pvFaceDirection = &vDirection;
		pEntrance = s_ObjectSpawn( pGame, tSpec );
		ASSERTX_RETFALSE( pEntrance, "Unable to create entrance" );
	}

	// there are two objects in the exact same location in the old truth room and the new one, find them
	UNIT *pAnchorOld = s_SubLevelFindFirstUnitOf( pSubLevel, eTargetTypesObject, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_TRUTH_ROOM_ANCHOR ) );
	ASSERTX_RETFALSE( pAnchorOld, "Cannot find required anchor in old truth room" );
	UNIT *pAnchorNew = s_SubLevelFindFirstUnitOf( pSubLevelNext, eTargetTypesObject, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_TRUTH_ROOM_ANCHOR ) );
	ASSERTX_RETFALSE( pAnchorNew, "Cannot find required anchor in new truth room" );

	// what is the players current distance from the anchor in the old truth room
	VECTOR vToAnchor = UnitGetPosition( pPlayer ) - UnitGetPosition( pAnchorOld );

	// given the anchor position in the new room, figure out the players new position
	VECTOR vDest = UnitGetPosition( pAnchorNew ) + vToAnchor;
			
	// put the player at the arrival location
	DWORD dwUnitWarpFlags = 0;
	SETBIT( dwUnitWarpFlags, UW_PRESERVE_CAMERA_BIT );
	UnitWarp(
		pPlayer,
		UnitGetRoom( pAnchorNew ),
		vDest,
		UnitGetFaceDirection( pPlayer, FALSE ),  // TODO: make this keep the same angle to the anchor object
		cgvZAxis,
		dwUnitWarpFlags,
		TRUE);

	// give some time for the transition effect to wear off, and then do the auto talk
	if (nQuest != INVALID_LINK &&
		eGITalkToAfterTransport != GI_INVALID && 
		nDialogToSpeakAfterTransport != INVALID_LINK)
	{		
		int nDurationInTicks = (int)(GAME_TICKS_PER_SECOND * TIME_FOR_AUTO_TALK_AFTER_TRUTH_TRANSPORT_IN_SECONDS);
		UnitRegisterEventTimed( pPlayer, sDoTruthAutoTalk, tEventData, nDurationInTicks );	
	}
			
	return TRUE;		

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestTransportToNewTruthRoom( 
	UNIT *pPlayer,
	QUEST *pQuest /*= NULL*/,
	GLOBAL_INDEX eGITalkToAfterTransport /*= GI_INVALID*/,
	int nDialogToSpeakAfterTransport /*= INVALID_LINK*/)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// should not do this twice
	ASSERTX_RETURN( UnitHasExactState( pPlayer, STATE_TRUTH_REVEALING ) == FALSE, "Already revealing a truth" );
	
	// set the truth being revealed state on the player
	s_StateSet( pPlayer, pPlayer, STATE_TRUTH_REVEALING, 0 );

	// now long is will we wait to do the transport
	int nDurationInTicks = (int)(GAME_TICKS_PER_SECOND * TIME_FROM_TRUTH_FLASH_TILL_TRANSPORT_IN_SECONDS);

	// setup event data
	EVENT_DATA tEventData;
	tEventData.m_Data1 = pQuest ? pQuest->nQuest : NULL;
	tEventData.m_Data2 = eGITalkToAfterTransport;
	tEventData.m_Data3 = nDialogToSpeakAfterTransport;
	
	// give a little bit of time for state effect to work on the client, then transport them
	UnitRegisterEventTimed( pPlayer, sDoTruthTransport, tEventData, nDurationInTicks );	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestTruthRoomEnter(
	UNIT *pPlayer,
	SUBLEVEL *pTruthRoom,	
	QUEST *pQuest,
	int nQuestStateToAdvanceTo)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( pTruthRoom, "Expected sublevel" );

	// activate and advance quest if we have a state to advance to	
	if (nQuestStateToAdvanceTo != INVALID_LINK)
	{
		ASSERTX_RETURN( pQuest, "Expected quest" );
		
		// activate quest if not active
		s_QuestActivate( pQuest );
		
		// entering old truth room
		s_QuestAdvanceTo( pQuest, nQuestStateToAdvanceTo );
					
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestSendRadarMessage(
	QUEST * pQuest,
	QUEST_UI_RADAR_INTERACTIONS eMsg,
	UNIT * pUnit,
	int nType,
	DWORD dwColor )
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETURN( pPlayer, "Expected quest player" );

	// setup message
	MSG_SCMD_QUEST_RADAR tMessage;
	tMessage.bCommand = (BYTE)eMsg;
	if ( pUnit )
	{
		tMessage.idUnit = UnitGetId( pUnit );
		tMessage.vPosition = UnitGetPosition( pUnit );
		tMessage.nType = nType;
		tMessage.dwColor = dwColor;
	}

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendUnitMessageToClient( pPlayer, idClient, SCMD_QUEST_RADAR, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestRemoveOnJoinGame(
	int nQuest)
{
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( nQuest );
	return pQuestDef->bRemoveOnJoinGame;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestRemoveOnJoinGame(
	const QUEST *pQuest)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	return QuestRemoveOnJoinGame( pQuest->nQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetVersion( 
	const QUEST *pQuest)
{
	ASSERTX_RETINVALID( pQuest, "Expected quest" );
	return pQuest->nVersion;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetCurrentVersion( 
	const QUEST *pQuest)
{
	ASSERTX_RETINVALID( pQuest, "Expected quest" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	if (pQuestDef)
	{
		return QuestDataGetCurrentVersion();
	}
	return NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestRunFunction(
	QUEST *pQuest,
	QUEST_FUNCTION_TYPE eType,
	const QUEST_FUNCTION_PARAM &tParam)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	ASSERTX_RETURN( eType >= QFUNC_INVALID && eType < QFUNC_NUM_FUNCTIONS, "Invalid function type" );
	
	// if quest is a template quest, run its function
	const QUEST_TEMPLATE_DEFINITION *pQuestTemplateDef = QuestTemplateGetDefinition( pQuest );
	if (pQuestTemplateDef)
	{
		const QUEST_FUNCTION_TABLE *pFunctionTable = &pQuestTemplateDef->tFunctions;
		const QUEST_FUNCTION *pFunction = &pFunctionTable->tTable[ eType ];
		if (pFunction->pfnFunction)
		{
			pFunction->pfnFunction( tParam );
		}
	}
			
	// run any custom function on quest itself
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETURN( pQuestDef );
	const QUEST_FUNCTION_TABLE *pFunctionTable = &pQuestDef->tFunctions;
	ASSERT_RETURN( pFunctionTable );
	const QUEST_FUNCTION *pFunction = &pFunctionTable->tTable[ eType ];
	if (pFunction->pfnFunction)
	{
		pFunction->pfnFunction( tParam );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestUseGI(
	QUEST *pQuest,
	GLOBAL_INDEX eGlobalIndex)
{
	ASSERTX_RETINVALID( pQuest, "Expected quest" );
	ASSERTX_RETINVALID( eGlobalIndex != GI_INVALID, "Invalid global index" );

	// get app globals for this quest
	APP_QUEST_GLOBAL *pAppQuestGlobal = QuestGetAppQuestGlobal( pQuest->nQuest );
	ASSERTX_RETINVALID( pAppQuestGlobal, "Expected app quest global" );

	// get excel table
	EXCELTABLE eTable = GlobalIndexGetDatatable( eGlobalIndex );
	
	// we only care to track a couple of types
	if (eTable == DATATABLE_MONSTERS || eTable == DATATABLE_OBJECTS || eTable == DATATABLE_ITEMS)
	{
	
		// set this global as index as interesting to this quest if it's not already there
		for (int i = 0; i < pAppQuestGlobal->nNumInterstingGI; ++i)
		{
			if (pAppQuestGlobal->eInterestingGI[ i ] == eGlobalIndex)
			{
				goto _done;
			}
		}

		// add new entry
		ASSERTX_RETINVALID( pAppQuestGlobal->nNumInterstingGI <= MAX_QUEST_INTERESTING_GI - 1, "Too many interesting GI's for quest" );
		pAppQuestGlobal->eInterestingGI[ pAppQuestGlobal->nNumInterstingGI++ ] = eGlobalIndex;

	}
			
	// return index
_done:
	return GlobalIndexGet( eGlobalIndex );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestUsesUnitClass(
	QUEST *pQuest,
	GENUS eGenus,
	int nClass)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	ASSERTX_RETFALSE( eGenus != GENUS_NONE, "Invalid genus" );
	ASSERTX_RETFALSE( nClass != INVALID_LINK, "Invalid class" );

	// get app globals for this quest
	APP_QUEST_GLOBAL *pAppQuestGlobal = QuestGetAppQuestGlobal( pQuest->nQuest );
	ASSERTX_RETINVALID( pAppQuestGlobal, "Expected app quest global" );

	switch (eGenus)
	{
	case GENUS_MONSTER:
		if (pQuest->pQuestDef->nObjectiveMonster == nClass)
			return TRUE;
		break;

	case GENUS_ITEM:
		// TODO cmarch: check for starting item treasure class items
		if (pQuest->pQuestDef->nCollectItem == nClass)
			return TRUE;
		break;

	case GENUS_OBJECT:
		if (pQuest->pQuestDef->nObjectiveObject == nClass)
			return TRUE;
		break;
	}

	// scan each entry
	for (int i = 0; i < pAppQuestGlobal->nNumInterstingGI; ++i)
	{
		GLOBAL_INDEX eGlobalIndex = pAppQuestGlobal->eInterestingGI[ i ];

		// what table does this global index come from
		EXCELTABLE eTable = GlobalIndexGetDatatable( eGlobalIndex );
		
		// get genus of table (if any)
		GENUS eGenusOther = ExcelGetGenusByDatatable( eTable );
		if (eGenusOther == eGenus)
		{
			int nClassOther = GlobalIndexGet( eGlobalIndex );
			if (nClass == nClassOther)
				return TRUE;
		}
		
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestAutoTrack(
	QUEST * pQuest)
{
	ASSERT_RETURN(pQuest);

	GAME *pGame = QuestGetGame(pQuest);	
	ASSERT_RETURN(pGame);

	UNIT *pPlayer = pQuest->pPlayer;
	ASSERT_RETURN(pPlayer);

	if (!UnitGetStat(pPlayer, STATS_DONT_AUTO_TRACK_QUESTS))
	{
#if !ISVERSION(SERVER_VERSION)
		if (IS_CLIENT(pGame))
		{
			c_QuestTrack(pQuest, TRUE);
			return;
		}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
		if (IS_SERVER(pGame))
		{
			s_QuestTrack(pQuest, TRUE);
			return;
		}
#endif
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL QuestPlayCanAcceptQuest(
	UNIT *pPlayer,
	QUEST *pQuest)
{
	if (AppIsTugboat())
		return TRUE;

	if (pQuest->pQuestDef->eStyle == QS_STORY)
	{
		return TRUE;
	}

	int nCount = 0;
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ); 
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{		
		QUEST *pOtherQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
		if (!pOtherQuest)		
			continue;

		if (pOtherQuest->pQuestDef->eStyle != QS_STORY &&
			(QuestIsActive(pOtherQuest) || QuestIsOffering(pOtherQuest)))
		{
			nCount++;
		}
	}

	if (nCount >= QUEST_MAX_SIDE_QUESTS)
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestIterateQuests(
	UNIT *pPlayer,
	PFN_QUEST_ITERATE pfnCallback,
	void *pCallbackData,
	DWORD dwQuestIterateFlags /*= 0*/,		// see QUEST_ITERATE_FLAGS
	BOOL bAutomaticallyVersion /*= TRUE*/)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( pfnCallback, "Expected quest iterate callback" );

	if (TESTBIT( dwQuestIterateFlags, QIF_ALL_DIFFICULTIES ))
	{
		// go through difficulties
		for (int nDifficulty = 0; nDifficulty < NUM_DIFFICULTIES; ++nDifficulty)
		{
		
			// go through all quests	
			for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
			{
				QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty, bAutomaticallyVersion );
				if (pQuest)
				{
					pfnCallback( pQuest, pCallbackData );
				}
				
			}	
						
		}
	}
	else
	{
	
		// we will only go through quests of the current difficulty
		int nDifficulty = DifficultyGetCurrent( pPlayer );
		
		// go through all quests	
		for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty, bAutomaticallyVersion );
			if (pQuest)
			{
				pfnCallback( pQuest, pCallbackData );
			}
			
		}		
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int QuestGetMonsterLevel(
	QUEST *pQuest,
	LEVEL *pLevel,
	int nMonsterClass,
	int nMonsterLevelForNormal)
{
	UNREFERENCED_PARAMETER(nMonsterClass); // just planning ahead here, I really don't want to go through all the quest files again -cmarch
	ASSERTX_RETVAL(pQuest && pLevel, 1, "Expected quest and level pointers");
	int nDifficulty = pQuest->nDifficulty;
	if (nDifficulty < 0 || nDifficulty >= NUM_DIFFICULTIES)
	{
		ASSERTX(FALSE, "Illegal quest difficulty, defaulting to normal to determine quest monster level");
		nDifficulty = DIFFICULTY_NORMAL;
	}

	if (nDifficulty == DIFFICULTY_NORMAL)
		return nMonsterLevelForNormal;

	int nOffset = nMonsterLevelForNormal - LevelGetMonsterExperienceLevel(pLevel, DIFFICULTY_NORMAL);
	return LevelGetMonsterExperienceLevel(pLevel, nDifficulty) + nOffset;
}