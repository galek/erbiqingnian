//----------------------------------------------------------------------------
// FILE: interact.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_ui.h"
#include "excel.h"
#include "faction.h"
#include "gossip.h"
#include "hireling.h"
#include "interact.h"
#include "uix_components_hellgate.h"
#include "c_message.h"
#include "npc.h"
#include "items.h"
#include "s_itemupgrade.h"
#include "s_trade.h"
#include "s_quests.h"
#include "s_recipe.h"
#include "s_reward.h"
#include "skills.h"
#include "stash.h"
#include "states.h"
#include "tasks.h"
#include "s_QuestNPCServer.h"
#include "Currency.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------
#define AUTO_STOP_INTERACT_DISTANCE_SQUARED (UNIT_INTERACT_DISTANCE_SQUARED * 2.0f)

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const INTERACT_DATA *InteractGetData(
	UNIT_INTERACT eInteract)
{
	ASSERTX_RETNULL( MAX_INTERACT_CHOICES >= NUM_UNIT_INTERACT, "MAX_INTERACT_CHOICES must be larger than or equal to NUM_INTERACT" );
	ASSERTX_RETNULL( eInteract > UNIT_INTERACT_INVALID && eInteract < NUM_UNIT_INTERACT, "Invalid unit interaction" );
	return (const INTERACT_DATA *)ExcelGetData( NULL, DATATABLE_INTERACT, eInteract );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const INTERACT_MENU_DATA *InteractMenuGetData(
	int nInteractMenu)
{
	return (const INTERACT_MENU_DATA*)ExcelGetData( NULL, DATATABLE_INTERACT_MENU, nInteractMenu );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInteractMenuAddOption(
	int nInteractMenu,
	BOOL bDisabled,
	int nOverrideTooltipId,
	INTERACT_MENU_CHOICE *ptInteractMenuBuffer,
	int nBufferSize,
	int nCurrentCount)
{
	ASSERTX_RETFALSE( ptInteractMenuBuffer, "Expected array for interact menu options" );
	
	// see if it's already in the menu and don't add if it already is
	for (int i = 0; i < nCurrentCount; ++i)
	{
		int nInteractMenuOther = ptInteractMenuBuffer[ i ].nInteraction;
		if (nInteractMenu == nInteractMenuOther)
		{
			if (bDisabled && !ptInteractMenuBuffer[i].bDisabled)
			{
				ptInteractMenuBuffer[i].bDisabled = TRUE;
			}
			return FALSE;
		}
	}

	// add this new option	
	ASSERTX_RETFALSE( nCurrentCount < nBufferSize, "Interact menu buffer too small" );
	ptInteractMenuBuffer[ nCurrentCount ].nInteraction = nInteractMenu;
	ptInteractMenuBuffer[ nCurrentCount ].bDisabled = (BYTE)bDisabled;
	ptInteractMenuBuffer[ nCurrentCount ].nOverrideTooltipId = nOverrideTooltipId;

	return TRUE;  // added another option
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_InteractDisplayChoices(
	UNIT *pPlayer,
	UNIT *pUnit,
	const INTERACT_MENU_CHOICE *ptInteractChoicesIn,
	int nNumChoices,
	UI_COMPONENT *pComponent /*=NULL*/,
	PGUID guid /*=INVALID_GUID*/)
{

	// scan through the interactions and build an array of menu options ... we do
	// this because for some interaction options we will just collapse them
	// down into a single menu to make it simpler for the player
	INTERACT_MENU_CHOICE ptInteractMenuChoicesOut[ MAX_INTERACT_CHOICES ];
	int nMenuCount = 0;
	for (int i = 0; i < nNumChoices; ++i)
	{
		if (ptInteractChoicesIn[ i ].nInteraction != UNIT_INTERACT_INVALID)
		{
			UNIT_INTERACT eInteract = (UNIT_INTERACT)(ptInteractChoicesIn[ i ].nInteraction);
			const INTERACT_DATA *pInteractData = InteractGetData( eInteract );
			int nInteractMenu = pInteractData->nInteractMenu;
			ASSERTX_CONTINUE( nInteractMenu != INVALID_LINK, "Expected interact menu" );
			if (sInteractMenuAddOption( nInteractMenu, ptInteractChoicesIn[ i ].bDisabled, ptInteractChoicesIn[ i ].nOverrideTooltipId, ptInteractMenuChoicesOut, MAX_INTERACT_CHOICES, nMenuCount ))
			{
				nMenuCount++;
			}

		}
	}
	
	// activate the radial for this unit with all the available choices
	UIRadialMenuActivate( pUnit, ptInteractMenuChoicesOut, nMenuCount, pComponent, guid );
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_InteractRequestChoices(
	UNIT *pTarget,
	UI_COMPONENT *pComponent /*=NULL*/,
	PGUID guid /*=INVALID_GUID*/)
{

//!!!	we can put this back if/when we want to go to the server to get the choices
//		but be aware there can be a stuck radial menu if the user presses and
//		releases the mouse button on the client before the server returns with its response.
//		So that will have to get fixed.

	//// setup message
	//MSG_CCMD_REQUEST_INTERACT_CHOICES tMessage;
	//tMessage.idUnit = UnitGetId( pTarget );
	//
	//// send to server
	//c_SendMessage( CCMD_REQUEST_INTERACT_CHOICES, &tMessage );	

//!!!!

	GAME *pGame = AppGetCltGame();
	ASSERT_RETURN(pGame);
	UNIT *pPlayer = GameGetControlUnit(pGame);
	ASSERT_RETURN(pPlayer);

	//Get Choices from the client
	INTERACT_MENU_CHOICE ptInteract[MAX_INTERACT_CHOICES];
	int nNumChoices = x_InteractGetChoices(pPlayer, pTarget, ptInteract, MAX_INTERACT_CHOICES, pComponent, guid);

	// display radial menu
	c_InteractDisplayChoices( pPlayer, pTarget, ptInteract, nNumChoices, pComponent, guid );

}

#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int x_InteractGetChoices(
	UNIT *pPlayer,
	UNIT *pTarget,
	INTERACT_MENU_CHOICE *ptBuffer,
	int nBufferSize,
	UI_COMPONENT *pComponent /*=NULL*/,
	PGUID guid /*=INVALID_GUID*/)
{
	if (pTarget)
	{
		if (UnitIsNPC( pTarget ))
		{
			return x_NPCInteractGetAvailable( pPlayer, pTarget, ptBuffer, nBufferSize );
		}
		else if (UnitIsA(pTarget, UNITTYPE_ITEM))
		{
			return ItemGetInteractChoices(pPlayer, pTarget, ptBuffer, nBufferSize, pComponent);
		}
		else if (UnitIsPlayer(pTarget))
		{
			return x_PlayerInteractGetChoices(pPlayer, pTarget, ptBuffer, nBufferSize, pComponent, guid);
		}
	}
	else //if (guid != INVALID_GUID)
	{
		return x_PlayerInteractGetChoices(pPlayer, pTarget, ptBuffer, nBufferSize, pComponent, guid);
	}
	
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sInteractAllowedFromUser( 
	UNIT *pUser, 
	UNIT_INTERACT eInteract)
{
	ASSERTX_RETFALSE( pUser, "Expected unit" );
		
	const INTERACT_DATA *pInteractData = InteractGetData( eInteract );
	if (pInteractData == NULL)
	{
		return FALSE;
	}
	
	// ghost
	if (pInteractData->bAllowGhost == FALSE && UnitIsGhost( pUser ))
	{
		return FALSE;
	}

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InteractAddChoice(
	UNIT *pUser,
	UNIT_INTERACT eInteract,
	BOOL bEnabled,
	int *pnCurrentCount,
	INTERACT_MENU_CHOICE *ptBuffer,
	int nBufferSize,
	int nOverrideTooltipId )
{
	ASSERTX_RETFALSE( pUser, "Expected unit" );
	ASSERTX_RETFALSE( ptBuffer, "Expected buffer" );
	ASSERTX_RETFALSE( nBufferSize > 0, "Invalid buffer size" );
	ASSERTX_RETFALSE( pnCurrentCount, "Expected current index" );

	// some interactions have restrictions based on the state of the user
	if (sInteractAllowedFromUser( pUser, eInteract ) == FALSE)
	{
		return FALSE;
	}
	
	// must have room
	ASSERTX_RETFALSE( nBufferSize > 0, "Invalid buffer size" );
	ASSERTX_RETFALSE( *pnCurrentCount < nBufferSize, "Interact buffer not large enough" );	
	
	// add the new interaction
	INTERACT_MENU_CHOICE *pChoice = &ptBuffer[ (*pnCurrentCount)++ ];
	pChoice->nInteraction = eInteract;
	pChoice->bDisabled = !bEnabled;
	pChoice->nOverrideTooltipId = nOverrideTooltipId;
	
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_INTERACT sNPCAutoIdentifyItems(
	UNIT *pPlayer,
	UNIT *pTarget)
{
	ASSERTX_RETVAL( pPlayer, UNIT_INTERACT_INVALID, "Expected player unit" );
	ASSERTX_RETVAL( pTarget, UNIT_INTERACT_INVALID, "Expected target unit" );
	UNIT_INTERACT eResult = UNIT_INTERACT_NONE;
	
	// must be able to do this
	if (UnitAutoIdentifiesInventory( pTarget ))
	{
		s_ItemIdentifyAll( pPlayer );
		return UNIT_INTERACT_IDENTIFY_INVENTORY;
	}	

	return eResult;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_INTERACT sNPCInteractTrade(
	UNIT *pPlayer,
	UNIT *pTradingWith)
{
	// KCK: See if this is a subscriber only NPC
	BOOL	bSubscriberContentLocked = FALSE;
	const UNIT_DATA * pUnitData = UnitGetData( pTradingWith );
	if ( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY) && !PlayerIsSubscriber(pPlayer))
		bSubscriberContentLocked = TRUE;
	
	if( AppIsTugboat() &&
		pUnitData &&
		pUnitData->nMerchantNotAvailableTillQuest != INVALID_ID &&
		!QuestGetIsTaskComplete( pTradingWith, QuestTaskDefinitionGet( pUnitData->nMerchantNotAvailableTillQuest ) ) )
	{
		// nothing was really done
		return UNIT_INTERACT_NONE;		
	}

	// can only trade with merchants
	if (UnitIsMerchant( pTradingWith ) == TRUE && !bSubscriberContentLocked)
	{

		// some NPCs can trade, but only if you have high enough faction for them	
		int nFactionType = UnitMerchantFactionType( pTradingWith );
		if ( nFactionType != INVALID_LINK )
		{
			// if the player does not have a faction number >= the faction score of the NPC (in monsters.xls) then they will not sell to them
			if ( FactionScoreGet( pPlayer, nFactionType ) < UnitMerchantFactionValueNeeded( pTradingWith ) )
			{
				s_NPCTalkStart( pPlayer, pTradingWith, NPC_DIALOG_OK, DIALOG_VENDOR_FACTION_RESTRICTED );
				return UNIT_INTERACT_TALK;
			}
		}
		
		// setup trade spec
		TRADE_SPEC tTradeSpec;
		if( UnitIsTrainer( pTradingWith ) )
		{
			tTradeSpec.eStyle = STYLE_TRAINER;
		}
		else if ( UnitIsStatTrader( pTradingWith ) )
		{
			tTradeSpec.eStyle = STYLE_STATTRADER;
		}
		else
		{
			tTradeSpec.eStyle = STYLE_MERCHANT;
		}
		
		// start trade
		s_TradeStart( pPlayer, pTradingWith, tTradeSpec );
		if (TradeIsTradingWith( pPlayer, pTradingWith ))
		{
			return UNIT_INTERACT_TRADE;
		}

	}

	// nothing was really done
	return UNIT_INTERACT_NONE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendHeadstoneInfo(
	UNIT *pUnit,
	UNIT *pGravekeeper,
	BOOL bCanReturnNow)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );

	// tell client they have a choice now
	MSG_SCMD_HEADSTONE_INFO tMessage;
	tMessage.idAsker = UnitGetId( pGravekeeper );
	tMessage.bCanReturnNow = (BYTE)bCanReturnNow;
	tMessage.nCost = 0;
	tMessage.nLevelDefDest = PlayerGetLastHeadstoneLevelDef( pUnit );
	if (bCanReturnNow)
	{
		//this probabably should be switched to use ingame and realworld money. Tugboat doesn't us this and Hellgate doesn't use realworld money so 
		//it should be ok for now.....
		tMessage.nCost = PlayerGetReturnToHeadstoneCost( pUnit ).GetValue( KCURRENCY_VALUE_INGAME );
	}

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_HEADSTONE_INFO, &tMessage );
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendDungeonWarpInfo(
	UNIT *pUnit,
	UNIT *pWarper)
{

	// setup message
	MSG_SCMD_DUNGEON_WARP_INFO tMessage;	
	tMessage.idWarper = UnitGetId( pWarper );
	tMessage.nDepth = UnitGetStat( pUnit, STATS_LEVEL_DEPTH );		
	tMessage.nCost = PlayerGetReturnToDungeonCost( pUnit ).GetValue( KCURRENCY_VALUE_INGAME );
	tMessage.nCostRealWorld = PlayerGetReturnToDungeonCost( pUnit ).GetValue( KCURRENCY_VALUE_REALWORLD );

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_DUNGEON_WARP_INFO, &tMessage );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendCreateGuildInfo(
								 UNIT *pUnit,
								 UNIT *pGuildmaster)
{

	// setup message
	MSG_SCMD_CREATEGUILD_INFO tMessage;	
	tMessage.idGuildmaster = UnitGetId( pGuildmaster );
	tMessage.nCost = PlayerGetGuildCreationCost( pUnit ).GetValue( KCURRENCY_VALUE_INGAME );
	tMessage.nCostRealWorld = PlayerGetGuildCreationCost( pUnit ).GetValue( KCURRENCY_VALUE_REALWORLD );

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_CREATEGUILD_INFO, &tMessage );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendRespecInfo(
	UNIT *pUnit,
	UNIT *pRespeccer)
{

	// setup message
	MSG_SCMD_RESPEC_INFO tMessage;	
	tMessage.idRespeccer = UnitGetId( pRespeccer );
	tMessage.nCost = PlayerGetRespecCost( pUnit, FALSE ).GetValue( KCURRENCY_VALUE_INGAME );
	tMessage.nCostRealWorld = PlayerGetRespecCost( pUnit, FALSE ).GetValue( KCURRENCY_VALUE_REALWORLD );
	tMessage.nCrafting = 0;

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_RESPEC_INFO, &tMessage );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendCraftingRespecInfo(
							UNIT *pUnit,
							UNIT *pRespeccer)
{

	// setup message
	MSG_SCMD_RESPEC_INFO tMessage;	
	tMessage.idRespeccer = UnitGetId( pRespeccer );
	tMessage.nCost = PlayerGetRespecCost( pUnit, TRUE ).GetValue( KCURRENCY_VALUE_INGAME );
	tMessage.nCostRealWorld = PlayerGetRespecCost( pUnit, TRUE ).GetValue( KCURRENCY_VALUE_REALWORLD );
	tMessage.nCrafting = 1;
	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_RESPEC_INFO, &tMessage );

}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendTransporterInfo(
								 UNIT *pUnit,
								 UNIT *pWarper)
{

	// setup message
	MSG_SCMD_TRANSPORTER_WARP_INFO tMessage;	
	tMessage.idWarper = UnitGetId( pWarper );

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pUnit );
	s_SendUnitMessageToClient( pUnit, idClient, SCMD_TRANSPORTER_WARP_INFO, &tMessage );

}


//----------------------------------------------------------------------------
// Sends dialog info for PvP Signer Upper NPC interaction to client
//----------------------------------------------------------------------------
static void sSendPvPSignerUppperInfo(
	UNIT *pPlayer,
	UNIT *pPvPSignerUpper)
{

	// setup message
	MSG_SCMD_PVP_SIGNER_UPPER_INFO tMessage;	
	tMessage.idPvPSignerUpper = UnitGetId( pPvPSignerUpper );

	// send to client
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendUnitMessageToClient( pPlayer, idClient, SCMD_PVP_SIGNER_UPPER_INFO, &tMessage );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static UNIT_INTERACT sNPCInteractTalk(
	UNIT *pPlayer,
	UNIT *pTalkWith)
{
	UNIT_INTERACT eResult = UNIT_INTERACT_NONE;

	//
	// we like to group several specific interaction types inside the talk action
	// because it's easier
	//

	// ask quest system
	if (eResult == UNIT_INTERACT_NONE)
	{
		eResult = s_QuestEventInteract( pPlayer, pTalkWith );
	}

	// grave keepers
	if (UnitIsGravekeeper( pTalkWith ))
	{
		sSendHeadstoneInfo( pPlayer, pTalkWith, PlayerHasHeadstone( pPlayer ) );
		eResult = UNIT_INTERACT_RETURN_TO_CORPSE;
	}

	// dungeon warpers (tugboat)
	if (UnitIsDungeonWarper( pTalkWith ))
	{
		sSendDungeonWarpInfo( pPlayer, pTalkWith );
		eResult = UNIT_INTERACT_DUNGEON_WARP;		
	}

	// guildmasters (tugboat)
	if (UnitIsGuildMaster( pTalkWith ))
	{
		sSendCreateGuildInfo( pPlayer, pTalkWith );
		eResult = UNIT_INTERACT_CREATEGUILD;		
	}

	// respeccers (tugboat)
	if (UnitIsRespeccer( pTalkWith ))
	{
		sSendRespecInfo( pPlayer, pTalkWith );
		eResult = UNIT_INTERACT_RESPEC;		
	}
	// respeccers (tugboat)
	if (UnitIsCraftingRespeccer( pTalkWith ))
	{
		sSendCraftingRespecInfo( pPlayer, pTalkWith );
		eResult = UNIT_INTERACT_RESPEC;		
	}
	// town transporters (tugboat)
	if (UnitIsTransporter( pTalkWith ))
	{
		sSendTransporterInfo( pPlayer, pTalkWith );
		eResult = UNIT_INTERACT_TRANSPORT;		
	}

	// hirelings (tugboat)
	if (UnitIsForeman( pTalkWith ) && eResult == UNIT_INTERACT_NONE )
	{
		eResult = s_HirelingSendSelection( pPlayer, pTalkWith );
	}

	// tradesmen
	if (UnitIsTradesman( pTalkWith ) && eResult == UNIT_INTERACT_NONE)
	{
		eResult = s_RecipeTradesmanInteract( pPlayer, pTalkWith );
	}

	// PvP signer upper
	if (UnitIsPvPSignerUpper( pTalkWith ) && eResult == UNIT_INTERACT_NONE)
	{
		sSendPvPSignerUppperInfo( pPlayer, pTalkWith );
		eResult = UNIT_INTERACT_DUNGEON_WARP;		
	}

	// ask task system
	if (eResult == UNIT_INTERACT_NONE)
	{
		eResult = s_TaskTalkToGiver( pPlayer, pTalkWith );
	}
	
	return eResult;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_INTERACT sGetNextPriorityInteraction( 
	UNIT_INTERACT eInteract)
{
	
	// stop at none
	if (eInteract == UNIT_INTERACT_NONE)
	{
		return UNIT_INTERACT_NONE;
	}

	// get current priority
	const INTERACT_DATA *pInteractData = InteractGetData( eInteract );
	int nPriorityCurrent = pInteractData->nPriority;
	UNIT_INTERACT eInteractNext = UNIT_INTERACT_NONE;
	int nPriorityNext = NONE;
	
	// scan through table and find the next lowest priority
	for (int nInteract = 0; nInteract < NUM_UNIT_INTERACT; ++nInteract)
	{
		UNIT_INTERACT eInteractOther = (UNIT_INTERACT)nInteract;
		const INTERACT_DATA *pInteractDataOther = InteractGetData( eInteractOther );
		if (pInteractDataOther)
		{
			int nPriorityOther = pInteractDataOther->nPriority;
			if (nPriorityOther != NONE)
			{
			
				// must be lower than the current priority
				if (nPriorityOther < nPriorityCurrent)
				{
				
					// if there is no next priority or this one we're looking at
					// is higher than the next we've found so far (but it's still
					// under the current) then use it
					if (nPriorityNext == NONE || nPriorityOther > nPriorityNext)
					{
						eInteractNext = eInteractOther;
						nPriorityNext = nPriorityOther;
					}
					
				}
			}			
		}
		
	}

	return eInteractNext;
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_UnitInteract( 
	UNIT *pPlayer, 
	UNIT *pTarget,
	UNIT_INTERACT eInteract)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX( pPlayer, "Expected Unit" );
	ASSERTX( pTarget, "Expected unit" );
	
	GAME *pGame = UnitGetGame( pPlayer );	
	ASSERTX( IS_SERVER( pGame ), "Server only function" );

	// must be allowed to interact
	if (sInteractAllowedFromUser( pPlayer, eInteract ) == FALSE)
	{
		return;
	}

	// KCK: See if this is a subscriber only thing, and force examine only if so.
	BOOL	bSubscriberContentLocked = FALSE;
	const UNIT_DATA * pUnitData = UnitGetData( pTarget );
	if ( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY) && !PlayerIsSubscriber(pPlayer))
	{
		eInteract = UNIT_INTERACT_EXAMINE;
		bSubscriberContentLocked = TRUE;
	}

	// talking to a healer will heal the player, regardless of what kind
	// of interaction they actually want to do
	if (UnitIsHealer( pTarget ) == TRUE && !bSubscriberContentLocked)
	{
		s_UnitRestoreVitals( pPlayer, TRUE );
	}
	
	// this result will contain the interaction that was actually done	
	UNIT_INTERACT eResult = UNIT_INTERACT_NONE;

	// switch on what type of interaction the user wants to do	
	switch (eInteract)
	{
		
		//----------------------------------------------------------------------------
		case UNIT_INTERACT_IDENTIFY_INVENTORY:
		{
			eResult = sNPCAutoIdentifyItems( pPlayer, pTarget );
			break;
		}
		
		//----------------------------------------------------------------------------
		case UNIT_INTERACT_TASK:
		{
			eResult = s_TaskTalkToGiver( pPlayer, pTarget );
			break;
		}
		
		//----------------------------------------------------------------------------
		case UNIT_INTERACT_TRADE:
		{
			eResult = sNPCInteractTrade( pPlayer, pTarget );
			break;
		}
		
		//----------------------------------------------------------------------------
		case UNIT_INTERACT_TALK:
		case UNIT_INTERACT_STORY_GOSSIP:
		{
			eResult = sNPCInteractTalk( pPlayer, pTarget );
			break;
		}

		//----------------------------------------------------------------------------
		case UNIT_INTERACT_GOSSIP:
		{
			eResult = s_GossipStart( pPlayer, pTarget );
			break;
		}
				
		//----------------------------------------------------------------------------
		case UNIT_INTERACT_RETURN_TO_CORPSE:
		{
			// try to return to headstone directly
			if (UnitIsGravekeeper( pTarget ))
			{
				if (s_PlayerTryReturnToHeadstone( pPlayer ) == TRUE)
				{
					eResult = UNIT_INTERACT_RETURN_TO_CORPSE;
				}
			}
			break;
		}
		
		//----------------------------------------------------------------------------		
		case UNIT_INTERACT_EXAMINE:
		{
			if ( UnitIsNPC(pTarget) && NPCCanBeExamined( pPlayer, pTarget ) )
			{
				// setup message
				MSG_SCMD_EXAMINE tMessage;
				tMessage.idUnit = UnitGetId( pTarget );

				// send it
				GAMECLIENTID idClient = UnitGetClientId( pPlayer );
				s_SendMessage( pGame, idClient, SCMD_EXAMINE, &tMessage );									
				eResult = UNIT_INTERACT_EXAMINE;
				
			}
			break;
		}

		case UNIT_INTERACT_PLAYERINSPECT:
		{
			if ( UnitIsPlayer(pTarget) && PlayerCanBeInspected( pPlayer, pTarget ) )
			{
				// Send information on all equipped inventory items
				GAMECLIENT	*pClient = UnitGetClient(pPlayer);
				UNIT * item = NULL;	
				while ((item = UnitInventoryIterate(pTarget, item, IIF_EQUIP_SLOTS_ONLY_BIT, FALSE)) != NULL)
				{
					s_SendUnitToClient( item, pClient, 0 );
				}

				// Send command to inspect
				MSG_SCMD_INSPECT	tMessage;
				tMessage.idUnit = UnitGetId( pTarget );
				GAMECLIENTID idClient = UnitGetClientId( pPlayer );
				s_SendMessage( pGame, idClient, SCMD_INSPECT, &tMessage );									
				eResult = UNIT_INTERACT_PLAYERINSPECT;
			}
			break;
		}
		
		//----------------------------------------------------------------------------
		case UNIT_INTERACT_HEAL:
		{
			if (UnitIsHealer( pTarget ))
			{
				s_UnitRestoreVitals( pPlayer, TRUE );
				if( AppIsTugboat() )	// in tug, we still say something when you get healed.
				{
					eResult = s_GossipStart( pPlayer, pTarget );
				}
				else
				{
					eResult = UNIT_INTERACT_HEAL;
				}
			}
			break;
		}
		
		
		//----------------------------------------------------------------------------
		default:
		{
			const UNIT_DATA * pTargetUnitData = UnitGetData( pTarget );
			if ( pTargetUnitData->nSkillWhenUsed != INVALID_ID )
			{
				if ( SkillCanStart( pGame, pPlayer, pTargetUnitData->nSkillWhenUsed, INVALID_ID, pTarget, FALSE, FALSE, 0 ) &&
					SkillStartRequest(pGame, pPlayer, pTargetUnitData->nSkillWhenUsed, UnitGetId( pTarget ), VECTOR(0), SKILL_START_FLAG_INITIATED_BY_SERVER, SkillGetNextSkillSeed( pGame ) ) )
					eResult = UNIT_INTERACT_ITEMUSE;
			}
			break;
		}
		
	}

	// if no interact was done, we will do a "default" one by trying to
	// do each of the available interactions based on priority
	if (eResult == UNIT_INTERACT_NONE)
	{
		UNIT_INTERACT eInteractNext = sGetNextPriorityInteraction( eInteract );
		if (eInteractNext != UNIT_INTERACT_NONE && !bSubscriberContentLocked )			 
		{
			s_UnitInteract( pPlayer, pTarget, eInteractNext );
		}
	}
	else
	{
		
		// set a stat for the last tick this npc was interacted with
		DWORD dwNow = GameGetTick( pGame );
		UnitSetStat( pTarget, STATS_LAST_INTERACT_TICK, dwNow );

		// get interact data for result
		const INTERACT_DATA *pResultInteractData = InteractGetData( eResult );
					
		// face player if specified
		if (pResultInteractData->bFaceDuringInteraction && UnitTestFlag(pTarget, UNITFLAG_FACE_DURING_INTERACTION))
		{
			UnitFaceUnit( pTarget, pPlayer );
		}

		// play greetings in some situations	
		if (pResultInteractData->bPlayGreeting == TRUE)
		{
			// play any greeting
			s_NPCDoGreeting( pGame, pTarget, pPlayer );
		}
				
		// set the talking to if we're supposed to and it's not already this NPC
		if (pResultInteractData->bSetTalkingTo)
		{
			if (s_TalkGetTalkingTo( pPlayer ) != pTarget)
			{
				s_TalkSetTalkingTo( pPlayer, pTarget, INVALID_LINK );
			}	
		}

		s_QuestEventInteractGeneric( pPlayer, pTarget, eResult );

	}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )					
}

//----------------------------------------------------------------------------
struct NPC_INFO_LOOKUP
{
	INTERACT_INFO eInfo;
	int				nState;
};

static NPC_INFO_LOOKUP sgtNPCInfoLookup[] = 
{
	{ INTERACT_INFO_UNAVAILABLE,STATE_NPC_INFO_UNAVAILABLE },
	{ INTERACT_INFO_NEW,		STATE_NPC_INFO_NEW },
	{ INTERACT_INFO_WAITING,	STATE_NPC_INFO_WAITING },
	{ INTERACT_INFO_RETURN,		STATE_NPC_INFO_RETURN },
	{ INTERACT_INFO_MORE,		STATE_NPC_INFO_MORE },

	{ INTERACT_STORY_NEW,		STATE_NPC_STORY_NEW },
	{ INTERACT_STORY_RETURN,	STATE_NPC_STORY_RETURN },
	{ INTERACT_STORY_GOSSIP,	STATE_NPC_STORY_GOSSIP },

	{ INTERACT_HOLIDAY_NEW,		STATE_NPC_HOLIDAY_NEW },
	{ INTERACT_HOLIDAY_RETURN,	STATE_NPC_HOLIDAY_RETURN },
			
	{ INTERACT_INFO_NONE,	-1 },  // keep last
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_InteractInfoSet(
	GAME *pGame,
	UNITID idNPC, 
	INTERACT_INFO eInfo)
{
	UNIT *pNPC = UnitGetById( pGame, idNPC );
	
	if (pNPC)
	{
		WORD wStateIndex = 0;
				
		// clear all info states
		for (int i = 0; sgtNPCInfoLookup[ i ].eInfo != INTERACT_INFO_NONE; ++i)
		{
			NPC_INFO_LOOKUP *pLookup = &sgtNPCInfoLookup[ i ];
			c_StateClear( pNPC, UnitGetId( pNPC ), pLookup->nState, wStateIndex );			
		}
		
		// set new one (if any)
		for (int i = 0; sgtNPCInfoLookup[ i ].eInfo != INTERACT_INFO_NONE; ++i)
		{
			NPC_INFO_LOOKUP *pEntry = &sgtNPCInfoLookup[ i ];
			if (pEntry->eInfo == eInfo)
			{
				c_StateSet( pNPC, pNPC, pEntry->nState, wStateIndex, -1, -1 );
				break;
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
INTERACT_INFO c_NPCGetInfoState(
	UNIT * npc )
{
	ASSERT_RETVAL( npc, INTERACT_INFO_INVALID );

	for ( int i = 0; sgtNPCInfoLookup[ i ].eInfo != INTERACT_INFO_NONE; i++ )
	{
		NPC_INFO_LOOKUP *pLookup = &sgtNPCInfoLookup[ i ];
		if ( UnitHasExactState( npc, pLookup->nState ) )
			return pLookup->eInfo;
	}

	return INTERACT_INFO_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InteractCancelAll(
	UNIT *pPlayer)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player (%s)", UnitGetDevName( pPlayer ) );
	GAME *pGame =  UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	
	// stop talking with anybody
	UNIT *pTalkingTo = s_TalkGetTalkingTo( pPlayer );
	if (pTalkingTo)
	{
		s_NPCTalkStop( pGame, pPlayer, CCT_FORCE_CANCEL );
	}
	
	if( AppIsTugboat() )
	{
		// Cancel any trades so we can save.
		if(TradeIsTrading(pPlayer) && UnitIsMerchant( pTalkingTo ) )
		{
			s_TradeCancel( pPlayer, FALSE );
		}
		
		// close stash
		if( StashIsOpen( pPlayer ) )
		{
			s_StashClose(pPlayer);
			// tell client to close stash UI element
			s_PlayerToggleUIElement( pPlayer, UIE_STASH, UIEA_CLOSE );
		}
	}

	s_RewardCancel( pPlayer );
	s_RecipeClose( pPlayer );
	s_TradeCancel( pPlayer, TRUE );
	s_ItemUpgradeClose( pPlayer );
	s_ItemAugmentClose( pPlayer );
	s_ItemUnModClose( pPlayer );
	s_ItemsReturnToStandardLocations( pPlayer, FALSE );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InteractCancelUnitsTooFarAway(
	UNIT *pPlayer)
{	
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player (%s)", UnitGetDevName( pPlayer ) );
	
	GAME *pGame =  UnitGetGame( pPlayer );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	
	// who is this player talking to
	UNIT *pTalkingTo = s_TalkGetTalkingTo( pPlayer );
	if (pTalkingTo != NULL && !UnitIsContainedBy( pTalkingTo, pPlayer )) // it's possible the player is interacting with something they are holding
	{
		BOOL bStop = TRUE;
		
		if (AppIsHellgate())
		{
		
			// how far away is the player from who they're talking to
			ROOM *pRoomTalkingTo = UnitGetRoom( pTalkingTo );  // may be NULL when "talking to" items			
			if (pRoomTalkingTo)
			{
				float flDistSq = UnitsGetDistanceSquared( pPlayer, pTalkingTo );
				if (flDistSq < AUTO_STOP_INTERACT_DISTANCE_SQUARED)
				{
					bStop = FALSE;
				}
				
			}
			
		}

		if (bStop == TRUE)
		{
			s_InteractCancelAll( pPlayer );
		}
		
	}
	if( AppIsTugboat() && StashIsOpen( pPlayer ) )
	{
		s_InteractCancelAll( pPlayer );
	}
}