//----------------------------------------------------------------------------
// duel.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "duel.h"
#include "c_message.h"
#include "console.h"
#include "states.h"
#include "player.h"
#include "uix.h"
#include "uix_menus.h"
#include "uix_chat.h"
#include "combat.h"
#include "chat.h"
#include "warp.h"
#include "c_chatNetwork.h"
#include "difficulty.h"
#include "s_network.h"
#include "pvp.h"
#include "ctf.h"
#include <ServerSuiteCommon.h>
#if ISVERSION(SERVER_VERSION)
#include "ServerSuite/Battle/BattleGameServerRequestMsg.h"
#include "s_battleNetwork.h"
#endif


#define DUEL_COUNTDOWN_IN_SECONDS_AFTER_WARP (10)
#define DUEL_COUNTDOWN_IN_SECONDS (5)
#define DUEL_EXIT_TIME_IN_SECONDS (3)
#define DUEL_END_MESSAGE_FADE_OUT_MULT (4.5f)

static int g_nDuelCountdown = 0;
static PvPGameData g_tCachedGameData;	// cache the game data for the scoreboard, since opponents may leave the arena before the control unit does

static void c_sDuelCleanup(GAME* game,UNIT* unit,UNIT* otherUnit);
static void s_sDuelCleanup(GAME* game,UNIT* unit,UNIT* unitNotUsed,EVENT_DATA* pHandlerData,void* data,DWORD dwId);
static void s_sDuelStartCountdown(GAME *game,UNIT *unit,UNIT *playerWhoInvited, int nCountdownSeconds);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDuelIsUnitInAMatch(UNIT *pUnit)
{
	return pUnit && UnitHasExactState( pUnit, STATE_PVP_MATCH ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DuelUnitHasActiveOrPendingDuel( 
	UNIT *pUnit)
{
	return pUnit && UnitGetStat(pUnit, STATS_PVP_DUEL_OPPONENT_ID) != INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DuelCanPlayersDuel( 
	UNIT *pInviter, 
	UNIT *pInvitee,
	DUEL_ILLEGAL_REASON *pIllegalReasonInviter /* = NULL */,
	DUEL_ILLEGAL_REASON *pIllegalReasonInvitee /* = NULL*/,
	BOOL bInvitationSent /* = FALSE */)
{
	// make things simple with the optional pointer business
	DUEL_ILLEGAL_REASON nIllegalReasonInviter;
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee;
	if (!pIllegalReasonInviter) pIllegalReasonInviter = &nIllegalReasonInviter;
	if (!pIllegalReasonInvitee) pIllegalReasonInvitee = &nIllegalReasonInvitee;

#if (1)

	*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_UNKNOWN;
	*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_UNKNOWN;

	if ( !pInviter )
	{
		return FALSE;
	}

	if ( !pInvitee )
	{
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_PLAYER_NOT_FOUND;
		return FALSE;
	}

	if (pInviter == pInvitee)
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_DONT_DUEL_YOURSELF;
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_DONT_DUEL_YOURSELF;
		return FALSE;
	}

	if (!UnitIsPlayer(pInviter))
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_NOT_A_PLAYER;
		return FALSE;
	}

	if (!UnitIsPlayer(pInvitee))
	{
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_NOT_A_PLAYER;
		return FALSE;
	}

	if (!bInvitationSent && DuelUnitHasActiveOrPendingDuel(pInviter))
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_IN_A_DUEL;
		return FALSE;
	}

	if (DuelUnitHasActiveOrPendingDuel(pInvitee))
	{
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_IN_A_DUEL;
		return FALSE;
	}

	if (sDuelIsUnitInAMatch(pInviter))
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_IN_A_MATCH;
		return FALSE;
	}

	if (sDuelIsUnitInAMatch(pInvitee))
	{
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_IN_A_MATCH;
		return FALSE;
	}
		
	// accepting a dueling invitation warps players to a new level, if
	// either player leaves that level, the duel is cancelled, and their
	// PvP flags are turned off
#if (0)
		if (UnitIsInTown(pInviter))
		{
			*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_IN_TOWN;
			return FALSE;
		}

		if (UnitIsInTown(pInvitee))
		{
			*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_IN_TOWN;
			return FALSE;
		}
#endif

	if (PlayerIsHardcoreDead(pInviter))
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_DEAD_HARDCORE;
		return FALSE;
	}

	if (PlayerIsHardcoreDead(pInvitee))
	{
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_DEAD_HARDCORE;
		return FALSE;
	}

	if ((PlayerIsHardcore(pInviter) && !PlayerIsHardcore(pInvitee)) ||
		(!PlayerIsHardcore(pInviter) && PlayerIsHardcore(pInvitee)))
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_GAME_MODE_MISMATCH;
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_GAME_MODE_MISMATCH;
		return FALSE;
	}
	

	GAME *pGame = UnitGetGame(pInviter);
	if (IS_CLIENT(pGame))
	{
		if (AppTestFlag(AF_CENSOR_NO_PVP))
		{
			UNIT *pControlUnit = GameGetControlUnit(pGame);
			if (pControlUnit == pInviter)
			{
				*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_PVP_CENSORED;
				return FALSE;
			}
			else if (pControlUnit == pInvitee)
			{
				*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_PVP_CENSORED;
				return FALSE;
			}
		}
	}
	else if (AppTestFlag(AF_CENSOR_NO_PVP))
	{
		*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_PVP_CENSORED;
		*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_PVP_CENSORED;
		return FALSE;
	}

	*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_NONE;
	*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_NONE;
	return TRUE;

#else
	*pIllegalReasonInviter = DUEL_ILLEGAL_REASON_DISABLED_FOR_BUILD;
	*pIllegalReasonInvitee = DUEL_ILLEGAL_REASON_DISABLED_FOR_BUILD;
	return FALSE;
#endif
}

void c_DuelDisplayInviteError(
	DUEL_ILLEGAL_REASON nIllegalReasonInviter,
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee,
	const WCHAR *wszPlayerName /*= NULL */,
	BOOL bDisplayInChat /* = FALSE */)
{
#if !ISVERSION(SERVER_VERSION)
	const WCHAR * wszHeader = NULL;
	const WCHAR * wszText   = NULL;
	BOOL bAddPlayerName = FALSE;

	wszHeader = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_HEADER);

	DUEL_ILLEGAL_REASON nIllegalReason = 
		(nIllegalReasonInviter >= nIllegalReasonInvitee) ? 
			nIllegalReasonInviter : nIllegalReasonInvitee;

	switch (nIllegalReason)
	{
	case DUEL_ILLEGAL_REASON_NONE:
		break;
	default:
	case DUEL_ILLEGAL_REASON_UNKNOWN:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_UNKNOWN);
		break;
	case DUEL_ILLEGAL_REASON_PLAYER_NOT_FOUND:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_PLAYER_NOT_FOUND);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_DONT_DUEL_YOURSELF:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_DONT_DUEL_YOURSELF);
		break;
	case DUEL_ILLEGAL_REASON_NOT_A_PLAYER:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_ONLY_PLAYERS_CAN_DUEL);
		break;
	case DUEL_ILLEGAL_REASON_NOT_SUBSCRIBER:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_ONLY_SUBSCRIBERS_CAN_DUEL);
		break;
	case DUEL_ILLEGAL_REASON_LEVEL_TOO_LOW:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_ONLY_LEVEL_TOO_LOW);
		break;
	case DUEL_ILLEGAL_REASON_IN_A_DUEL:
		wszText = NULL;// TODO cmarch add string that works for either being in a duel GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_ALREADY_IN_A_DUEL);
		break;
	case DUEL_ILLEGAL_REASON_IN_A_MATCH:
		wszText = NULL;// TODO cmarch add string that works for either being in a match GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_IN_A_MATCH);
		break;
	case DUEL_ILLEGAL_REASON_GAME_MODE_MISMATCH:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_PVP_DUEL_MODE_MISMATCH);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_GAME_DIFFICULTY_MISMATCH:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_PVP_DUEL_DIFFICULTY_MISMATCH);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_DEAD_HARDCORE:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_DEAD_HARDCORE_PLAYER);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_PREVENTED_BY_QUEST:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_PREVENTED_BY_QUEST);
		break;
	case DUEL_ILLEGAL_REASON_IN_TOWN:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_IN_TOWN);
		break;
	case DUEL_ILLEGAL_REASON_COOLDOWN:
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_COOLDOWN);
		break;
	}

	if (nIllegalReason > DUEL_ILLEGAL_REASON_UNKNOWN && wszText == NULL)
		wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_UNKNOWN);

	if (wszHeader && wszText)
	{
		WCHAR wszBuffer[2048];
		wszBuffer[0] = L'\0';
		if (bAddPlayerName)
		{
			if (wszPlayerName != NULL)
			{
				PStrPrintf(wszBuffer, arrsize(wszBuffer), wszText, wszPlayerName);
				wszText = wszBuffer;
			}
			else
			{
				wszText = GlobalStringGet(GS_PVP_DUEL_INVITE_FAILED_UNKNOWN);
			}
		}

		if (bDisplayInChat)
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), wszText);
		else
			UIShowGenericDialog(wszHeader, wszText);
	}
#endif //!ISVERSION(SERVER_VERSION)
}

void c_DuelDisplayAcceptError(
	DUEL_ILLEGAL_REASON nIllegalReasonInviter,
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee,
	const WCHAR *wszPlayerName /*= NULL */,
	BOOL bDisplayInChat /* = FALSE */)
{
#if !ISVERSION(SERVER_VERSION)

	const WCHAR * wszHeader = NULL;
	const WCHAR * wszText   = NULL;
	BOOL bAddPlayerName = FALSE;

	wszHeader = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_HEADER);

	DUEL_ILLEGAL_REASON nIllegalReason = 
		(nIllegalReasonInviter >= nIllegalReasonInvitee) ? 
			nIllegalReasonInviter : nIllegalReasonInvitee;

	switch (nIllegalReason)
	{
	case DUEL_ILLEGAL_REASON_NONE:
		break;
	default:
	case DUEL_ILLEGAL_REASON_UNKNOWN:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_UNKNOWN);
		break;
	case DUEL_ILLEGAL_REASON_PLAYER_NOT_FOUND:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_PLAYER_NOT_FOUND);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_DONT_DUEL_YOURSELF:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_DONT_DUEL_YOURSELF);
		break;
	case DUEL_ILLEGAL_REASON_NOT_A_PLAYER:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_ONLY_PLAYERS);
		break;
	case DUEL_ILLEGAL_REASON_NOT_SUBSCRIBER:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_NOT_A_SUBSCRIBER);
		break;
	case DUEL_ILLEGAL_REASON_LEVEL_TOO_LOW:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_LEVEL_TOO_LOW);
		break;
	case DUEL_ILLEGAL_REASON_IN_A_DUEL:
		wszText = NULL;// TODO cmarch add string that works for either being in a duel GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_ALREADY_IN_A_DUEL);
		break;
	case DUEL_ILLEGAL_REASON_IN_A_MATCH:
		wszText = NULL;// TODO cmarch add string that works for either being in a match GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_IN_A_MATCH);
		break;
	case DUEL_ILLEGAL_REASON_GAME_MODE_MISMATCH:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_PVP_DUEL_MODE_MISMATCH);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_GAME_DIFFICULTY_MISMATCH:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_PVP_DUEL_DIFFICULTY_MISMATCH);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_DEAD_HARDCORE:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_DEAD_HARDCORE_PLAYER);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_PREVENTED_BY_QUEST:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_PREVENTED_BY_QUEST);
		break;
	case DUEL_ILLEGAL_REASON_IN_TOWN:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_IN_TOWN);
		bAddPlayerName = TRUE;
		break;
	case DUEL_ILLEGAL_REASON_COOLDOWN:
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_COOLDOWN);
		break;
	}

	if (nIllegalReason > DUEL_ILLEGAL_REASON_UNKNOWN && wszText == NULL)
		wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_UNKNOWN);

	if (wszHeader && wszText)
	{
		WCHAR wszBuffer[2048];
		wszBuffer[0] = L'\0';
		if (bAddPlayerName)
		{
			if (wszPlayerName != NULL)
			{
				PStrPrintf(wszBuffer, arrsize(wszBuffer), wszText, wszPlayerName);
				wszText = wszBuffer;
			}
			else
			{
				wszText = GlobalStringGet(GS_PVP_DUEL_ACCEPT_FAILED_UNKNOWN);
			}
		}

		if (bDisplayInChat)
			UIAddChatLine(CHAT_TYPE_SERVER, ChatGetTypeColor(CHAT_TYPE_SERVER), wszText);
		else
			UIShowGenericDialog(wszHeader, wszText);
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sDuelInvite(UNIT *pPlayerToInvite)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETFALSE(pPlayerToInvite, "expected player");
	c_ChatNetRequestSocialInteraction(
		pPlayerToInvite,
		SOCIAL_INTERACTION_DUEL_REQUEST);
	return TRUE;
#else
	return FALSE;
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID c_DuelGetInviter()
{
#if !ISVERSION(SERVER_VERSION)
	// BSP - depreciated

	//GAME *game = AppGetCltGame();
	//if (game)
	//{
	//	UNIT * unit = GameGetControlUnit(game);
	//	if (unit)
	//		return UnitGetStat(unit, STATS_PVP_DUEL_INVITER_ID);
	//}
#endif //!ISVERSION(SERVER_VERSION)

	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DuelInviteOnAccept(
	PGUID guidOtherPlayer, 
	void * )
{
#if !ISVERSION(SERVER_VERSION)
	UNITID idInviter = (UNITID)guidOtherPlayer;
	if (idInviter != INVALID_ID)
	{
		// accept invite
		MSG_CCMD_DUEL_INVITE_ACCEPT msg;
		msg.idOpponent = idInviter;
		c_SendMessage(CCMD_DUEL_INVITE_ACCEPT, &msg);
	}
	else
	{
		// TODO localize?
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding duel invitations.");
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DuelInviteOnDecline(
	PGUID guidOtherPlayer, 
	void * )
{
#if !ISVERSION(SERVER_VERSION)
	UNITID idInviter = (UNITID)guidOtherPlayer;
	if (idInviter != INVALID_ID)
	{
		// decline invite
		MSG_CCMD_DUEL_INVITE_DECLINE msg;
		msg.idOpponent = idInviter;
		c_SendMessage(CCMD_DUEL_INVITE_DECLINE, &msg);
	}
	else
	{
		// TODO localize?
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding duel invitations.");
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_DuelInviteOnIgnore(
	PGUID guidOtherPlayer, 
	void * )
{
#if !ISVERSION(SERVER_VERSION)
	UNITID idInviter = (UNITID)guidOtherPlayer;
	if (idInviter != INVALID_ID)
	{
		// decline invite
		MSG_CCMD_DUEL_INVITE_DECLINE msg;
		msg.idOpponent = idInviter;
		c_SendMessage(CCMD_DUEL_INVITE_DECLINE, &msg);

		// add the player to the chat ignore list
		GAME *pGame = AppGetCltGame();
		ASSERT_RETURN(pGame);
		UNIT *pUnit = UnitGetById(pGame, idInviter);
		ASSERT_RETURN(pUnit);

		PGUID guidRealGuid = UnitGetGuid(pUnit);
		c_ChatNetIgnoreMember(guidRealGuid);

	}
	else
	{
		// TODO localize?
		ConsoleString(CONSOLE_SYSTEM_COLOR, L"You do not have any outstanding duel invitations.");
	}
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_DuelInviteAttempt(
	UNIT *pPlayerToInvite,
	const WCHAR *wszPlayerName /* = NULL */,
	BOOL bDisplayErrorInChat /* = FALSE */)
{
#if !ISVERSION(SERVER_VERSION)

	DUEL_ILLEGAL_REASON nIllegalReasonInviter = DUEL_ILLEGAL_REASON_UNKNOWN;
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee = DUEL_ILLEGAL_REASON_UNKNOWN;
	GAME *game = AppGetCltGame();
	if (game)
	{
		if (DuelCanPlayersDuel( GameGetControlUnit(game), pPlayerToInvite, &nIllegalReasonInviter, &nIllegalReasonInvitee ))
		{
			return c_sDuelInvite( pPlayerToInvite );
		}
	}
	
	// failure message
	c_DuelDisplayInviteError(nIllegalReasonInviter, nIllegalReasonInvitee, wszPlayerName);
#endif //!ISVERSION(SERVER_VERSION)
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDuelUpdateCountdownDisplay( )
{
#if !ISVERSION(SERVER_VERSION)

	// show on screen
	if ( g_nDuelCountdown == 1 )
	{
		UIShowQuickMessage( GlobalStringGet( GS_PVP_DUEL_COUNTDOWN_1_SECOND ) );
	}
	else
	{
		const int MAX_STRING = 1024;
		WCHAR timestr[ MAX_STRING ] = L"\0";
		PStrPrintf( timestr, MAX_STRING, GlobalStringGet( GS_PVP_DUEL_COUNTDOWN ), g_nDuelCountdown );
		UIShowQuickMessage( timestr );
	}

#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sCountdownUpdate( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE( game && unit );
	ASSERT( IS_CLIENT( game ) );


	g_nDuelCountdown--;
	if ( g_nDuelCountdown )
	{
		c_sDuelUpdateCountdownDisplay();
		UnitRegisterEventTimed( unit, c_sCountdownUpdate, &tEventData, GAME_TICKS_PER_SECOND );
	}
	else
	{
		UIHideQuickMessage();
	}

#endif// !ISVERSION(SERVER_VERSION)
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDuelDisplayYouWon(UNIT *opponent)
{
#if !ISVERSION(SERVER_VERSION)
	UIShowQuickMessage( GlobalStringGet( GS_PVP_DUEL_WON_YOU ), DUEL_END_MESSAGE_FADE_OUT_MULT );
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDuelDisplayYouWonByForfeit(UNIT *opponent)
{
#if !ISVERSION(SERVER_VERSION)
	// show on screen
	if ( opponent )
	{
		WCHAR namestr[ MAX_CHARACTER_NAME ] = L"\0";
		WCHAR msgstr[ 1024 ] = L"\0";
		if (UnitGetName(opponent, namestr, arrsize(namestr), 0))
		{
			PStrPrintf( msgstr, arrsize(msgstr), GlobalStringGet( GS_PVP_DUEL_FORFEITED ), namestr );
			UIShowQuickMessage( msgstr, DUEL_END_MESSAGE_FADE_OUT_MULT );
			return;
		}
	}

	UIShowQuickMessage( GlobalStringGet( GS_PVP_DUEL_WON_YOU ), DUEL_END_MESSAGE_FADE_OUT_MULT );
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDuelDisplayYouLost(UNIT *opponent)
{
#if !ISVERSION(SERVER_VERSION)
	UIShowQuickMessage( GlobalStringGet(GS_PVP_DUEL_LOST_YOU), DUEL_END_MESSAGE_FADE_OUT_MULT );
#endif //!ISVERSION(SERVER_VERSION)}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDuelDisplayYouLostByForfeit(UNIT *opponent, BOOL bWarped=FALSE)
{
#if !ISVERSION(SERVER_VERSION)
	UIShowQuickMessage( 
		GlobalStringGet( bWarped ? GS_PVP_DUEL_FORFEITED_YOU_WARPED : GS_PVP_DUEL_FORFEITED_YOU ), 
		DUEL_END_MESSAGE_FADE_OUT_MULT );
#endif //!ISVERSION(SERVER_VERSION)
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDuelYouGotKilled(
								GAME* game,
								UNIT* unit,
								UNIT* killer,
								EVENT_DATA* pHandlerData,
								void* data,
								DWORD dwId)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game);
	ASSERT_RETURN(IS_SERVER( game ));
	UNITID idOpponent =  pHandlerData ? (UNITID)pHandlerData->m_Data1 : INVALID_ID;
	UNIT *opponent = (idOpponent != INVALID_ID) ? UnitGetById(game, idOpponent) : NULL;
	UNIT *killerResponsible = killer ? UnitGetResponsibleUnit(killer) : NULL;
	GAMECLIENTID idClient = UnitGetClientId( unit  );	
	if (killerResponsible && UnitGetId(killerResponsible) == idOpponent)
	{
		MSG_SCMD_DUEL_MESSAGE msg_outgoing;
		msg_outgoing.nMessage = DUEL_MSG_LOST;
		msg_outgoing.idOpponent = idOpponent;
		s_SendMessage(game, idClient, SCMD_DUEL_MESSAGE, &msg_outgoing);
	}
	else
	{
		// TODO this is a little weird, maybe have a different message if you die by the wrong killer in a duel
		MSG_SCMD_DUEL_MESSAGE msg_outgoing;
		msg_outgoing.nMessage = DUEL_MSG_LOST_BY_FORFEIT;
		msg_outgoing.idOpponent = idOpponent;
		s_SendMessage(game, idClient, SCMD_DUEL_MESSAGE, &msg_outgoing);
	}

	if( opponent )
	{
		idClient = UnitGetClientId( opponent  );	

		if (killerResponsible && killerResponsible == opponent)
		{
			// all is normal, the killer is the opponent, and still in the level with the expected UNITID
			UnitChangeStat(opponent, STATS_PLAYER_KILLS, 0, 1);

			MSG_SCMD_DUEL_MESSAGE msg_outgoing;
			msg_outgoing.nMessage = DUEL_MSG_WON;
			msg_outgoing.idOpponent = UnitGetId( unit );
			s_SendMessage(game, idClient, SCMD_DUEL_MESSAGE, &msg_outgoing);
		}
		else
		{
			// TODO this is a little weird, maybe have a different message if you die by the wrong killer in a duel
			MSG_SCMD_DUEL_MESSAGE msg_outgoing;
			msg_outgoing.nMessage = DUEL_MSG_WON_BY_FORFEIT;
			msg_outgoing.idOpponent = UnitGetId( unit );
			s_SendMessage(game, idClient, SCMD_DUEL_MESSAGE, &msg_outgoing);
		}
	}

#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDuelYouWarped(
							 GAME* game,
							 UNIT* unit,
							 UNIT* unitNotUsed,
							 EVENT_DATA* pHandlerData,
							 void* data,
							 DWORD dwId)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game);
	ASSERT_RETURN(IS_SERVER( game ));
	UNITID idOpponent =  pHandlerData ? (UNITID)pHandlerData->m_Data1 : INVALID_ID;
	UNIT *opponent = (idOpponent != INVALID_ID) ? UnitGetById(game, idOpponent) : NULL;
	GAMECLIENTID idClient = UnitGetClientId( unit  );	

	MSG_SCMD_DUEL_MESSAGE msg_outgoing;
	msg_outgoing.nMessage = DUEL_MSG_LOST_BY_FORFEITWARP;
	msg_outgoing.idOpponent = idOpponent;
	s_SendMessage(game, idClient, SCMD_DUEL_MESSAGE, &msg_outgoing);

	if( opponent )
	{
		idClient = UnitGetClientId( opponent  );	

		// TODO this is a little weird, maybe have a different message if you die by the wrong killer in a duel
		MSG_SCMD_DUEL_MESSAGE msg_outgoing;
		msg_outgoing.nMessage = DUEL_MSG_WON_BY_FORFEIT;
		msg_outgoing.idOpponent =  UnitGetId( unit );
		s_SendMessage(game, idClient, SCMD_DUEL_MESSAGE, &msg_outgoing);

	}

	s_sDuelCleanup( game, unit, opponent, pHandlerData, data, dwId );
	s_sDuelCleanup( game, opponent, unit, pHandlerData, data, dwId );
#endif //!ISVERSION(SERVER_VERSION)
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sDuelCleanup(
	GAME* game,
	UNIT* unit,
	UNIT* otherUnit)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN(game);
	ASSERT_RETURN(IS_CLIENT( game ));

#endif //!ISVERSION(SERVER_VERSION)
}

void c_DuelMessage( UNIT* pPlayer,
					UNIT* pOpponent,
					DUEL_MESSAGE nMessage ) 
{
#if !ISVERSION(SERVER_VERSION)
	switch( nMessage )
	{
	case  DUEL_MSG_WON:
		c_sDuelDisplayYouWon( pOpponent );
		break;
	case  DUEL_MSG_LOST:
		c_sDuelDisplayYouLost( pOpponent );
		break;
	case  DUEL_MSG_WON_BY_FORFEIT:
		c_sDuelDisplayYouWonByForfeit( pOpponent );
		break;
	case  DUEL_MSG_LOST_BY_FORFEIT:
		c_sDuelDisplayYouLostByForfeit( pOpponent );
		break;
	case  DUEL_MSG_LOST_BY_FORFEITWARP:
		c_sDuelDisplayYouLostByForfeit( pOpponent, TRUE );
		break;
	}

	// cache the scoreboard data
	c_PvPGetGameData(&g_tCachedGameData);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_DuelStart(int nCountdownSeconds)
{
#if !ISVERSION(SERVER_VERSION)
	GAME *game = AppGetCltGame();
	if (game)
	{
		UNIT * unit = GameGetControlUnit(game);
		if (unit)
		{
			g_nDuelCountdown = nCountdownSeconds;

			// set display timer for the countdown
			c_sDuelUpdateCountdownDisplay();
			UnitUnregisterTimedEvent(unit, c_sCountdownUpdate);
			UnitRegisterEventTimed( 
				unit, 
				c_sCountdownUpdate, 
				&EVENT_DATA( ), 
				GAME_TICKS_PER_SECOND);

			// cache the scoreboard data
			c_PvPGetGameData(&g_tCachedGameData);
		}
	}
#endif //!ISVERSION(SERVER_VERSION)
	return FALSE;
}
 
struct DuelGetPlayerDataIterator
{
	PvPGameData *pGameData;
	UNIT *pControlUnit;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sGetPlayerData(
	UNIT *pUnit, 
	void *pCallbackData )
{
	DuelGetPlayerDataIterator *pGetPlayerDataIterator = (DuelGetPlayerDataIterator*)pCallbackData;
	ASSERT_RETVAL(pGetPlayerDataIterator && pGetPlayerDataIterator->pControlUnit, RIU_STOP);

	BOOL bIsControlUnit = (pGetPlayerDataIterator->pControlUnit == pUnit);
	int nTeamIndex = (bIsControlUnit) ? 1 : 0;	// opponent always uses the red scoreboard... to match their red name. could be a new scoreboard
	int nPlayerIndex = pGetPlayerDataIterator->pGameData->tTeamData[nTeamIndex].nNumPlayers;

	PvPPlayerData *pPlayerData = pGetPlayerDataIterator->pGameData->tTeamData[nTeamIndex].tPlayerData + nPlayerIndex;
	pPlayerData->bIsControlUnit = bIsControlUnit;
	pPlayerData->bCarryingFlag = FALSE;

	// player kills and flag captures
	pPlayerData->nPlayerKills = UnitGetStat(pUnit, STATS_PLAYER_KILLS);
	pPlayerData->nFlagCaptures = UnitGetStat(pUnit, STATS_FLAG_CAPTURES);
	pPlayerData->szName[0] = 0;
	UnitGetName(pUnit, pPlayerData->szName, arrsize(pPlayerData->szName), 0);

	++(pGetPlayerDataIterator->pGameData->tTeamData[nTeamIndex].nNumPlayers);

	// fill in team data
	PvPTeamData *pTeamData = pGetPlayerDataIterator->pGameData->tTeamData + nTeamIndex;
	pTeamData->nScore = pPlayerData->nPlayerKills;

	PStrCopy(pTeamData->szName, pPlayerData->szName, MAX_TEAM_NAME);

	pTeamData->eFlagStatus = CTF_FLAG_STATUS_NOT_APPLICABLE;			

	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_DuelGetGameData(PvPGameData *pPvPGameData)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(pPvPGameData);

	GAME *pGame = AppGetCltGame();
	ASSERT_RETFALSE(pGame && (GameGetSubType(pGame) == GAME_SUBTYPE_PVP_PRIVATE || GameGetSubType(pGame) == GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED));

	// recalculate game data only if the argument is not the cache structure pointer
	if (pPvPGameData != &g_tCachedGameData)
	{
		return MemoryCopy(pPvPGameData, sizeof(PvPGameData), &g_tCachedGameData, sizeof(g_tCachedGameData));
	}

	UNIT * pUnit = GameGetControlUnit(pGame);
	if (pUnit)
	{
		pPvPGameData->nSecondsRemainingInMatch = MAX(0, UnitGetStat(pUnit, STATS_GAME_CLOCK_TICKS)/GAME_TICKS_PER_SECOND);
		pPvPGameData->bHasTimelimit = FALSE;
		pPvPGameData->nNumTeams = 2;

		DuelGetPlayerDataIterator tGetPlayerDataIterator;
		ZeroMemory(&tGetPlayerDataIterator, sizeof(DuelGetPlayerDataIterator));
		tGetPlayerDataIterator.pGameData = pPvPGameData;
		tGetPlayerDataIterator.pControlUnit = pUnit;

		pPvPGameData->tTeamData[0].nNumPlayers = 0;
		pPvPGameData->tTeamData[1].nNumPlayers = 0;

		TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
		LevelIterateUnits(UnitGetLevel(pUnit), eTargetTypes, sGetPlayerData, &tGetPlayerDataIterator);

		return TRUE;
	}

#endif //!ISVERSION(SERVER_VERSION)

	return FALSE;
}

struct DUEL_FIND_OPPONENT_DATA
{
	UNIT *firstPlayer;
	UNIT *out_secondPlayer;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDuelFindOpponent( UNIT *unit, void *callbackData )
{
	DUEL_FIND_OPPONENT_DATA *data = (DUEL_FIND_OPPONENT_DATA *)callbackData;
	if (unit != data->firstPlayer)
		data->out_secondPlayer = unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_DuelEventPlayerExitLimbo(
	GAME* game,
	UNIT* unit)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game);

	if (GameGetSubType(game) != GAME_SUBTYPE_PVP_PRIVATE && 
		GameGetSubType(game) != GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED)
	{
		return;
	}

	if (!IS_SERVER(game))
	{
		return;
	}

	ASSERT_RETURN(unit);


	// check if duel is already started
	if (UnitHasState( game, unit, STATE_PVP_MATCH_STARTING ) ||
		UnitHasState( game, unit, STATE_PVP_MATCH_STARTING_MYTHOS ))
	{
		return;
	}

	DUEL_FIND_OPPONENT_DATA tData;
	tData.firstPlayer = unit;
	tData.out_secondPlayer = NULL;

	LevelIteratePlayers(UnitGetLevel(unit), s_sDuelFindOpponent, &tData);

	// if the other player is in the level and not in limbo, start the countdown
	if (tData.out_secondPlayer && !UnitIsInLimbo(tData.out_secondPlayer))
	{




		// reset the opponent UNITIDs, since this may be a new game instance (GUIDs won't fit in a stat nicely...)
		UnitSetStat(unit, STATS_PVP_DUEL_OPPONENT_ID, UnitGetId(tData.out_secondPlayer));
		UnitSetStat(tData.out_secondPlayer, STATS_PVP_DUEL_OPPONENT_ID, UnitGetId(unit));

		// players can't be in the same party when they duel
		PARTYID idParty = UnitGetPartyId( unit );
		if (idParty != INVALID_ID && idParty == UnitGetPartyId( tData.out_secondPlayer ))
		{
			s_PlayerRemoveFromParty( unit );
			s_PlayerRemoveFromParty( tData.out_secondPlayer );
		}

		s_sDuelStartCountdown(game, unit, tData.out_secondPlayer, DUEL_COUNTDOWN_IN_SECONDS_AFTER_WARP);
	}
	
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDuelOnPlayerDead(
	GAME* game,
	UNIT* unit,
	UNIT* unitNotUsed,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId)
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game && unit);
	ASSERT_RETURN(IS_SERVER( game ));


	UNITID idOtherUnit = (UNITID)pHandlerData->m_Data1;
	ASSERT_RETURN( idOtherUnit != INVALID_ID );
	UNIT *otherUnit = UnitGetById(game, idOtherUnit);
	if (otherUnit && ( GameGetSubType(game) == GAME_SUBTYPE_PVP_PRIVATE ) )
	{
		s_PlayerRestart( otherUnit, PLAYER_RESPAWN_PVP_RANDOM );
		s_StateClear( otherUnit, UnitGetId( otherUnit ), STATE_PVP_DUEL, 0 );

		s_PlayerRestart( unit, PLAYER_RESPAWN_PVP_FARTHEST );
		s_StateClear( unit, UnitGetId( unit ), STATE_PVP_DUEL, 0 );

		s_sDuelStartCountdown( game, unit, otherUnit, DUEL_COUNTDOWN_IN_SECONDS );
	}
	else
	{	//For automated duels, we always call the cleanup function in order to lose the duel and get pushed back to town.
		s_PlayerRestart( unit, PLAYER_RESPAWN_PVP_FARTHEST );
		s_sDuelCleanup( game, unit, unitNotUsed, pHandlerData, data, dwId );
	}

#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}


//----------------------------------------------------------------------------
// A little bit ugly: for an automated duel, we always send a message here
// declaring a loss, even though the other guy may have left/lost beforehand
// and previously declared a loss.  We handle this by a "first message wins"
// method on the battle server: the first player to report losing is the one
// who loses.
//----------------------------------------------------------------------------
static void s_sDuelCleanup(
	GAME* game,
	UNIT* unit,
	UNIT* unitNotUsed,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(game && unit);
	ASSERT_RETURN(IS_SERVER( game ));

	if( AppIsHellgate() )
	{
		s_StateClear( unit, UnitGetId(unit), STATE_PVP_MATCH_STARTING, 0 );
	}
	else
	{
		s_StateClear( unit, UnitGetId(unit), STATE_PVP_MATCH_STARTING_MYTHOS, 0 );
	}
	s_StateClear( unit, UnitGetId( unit ), STATE_PVP_DUEL, 0 );
	UnitSetStat(unit, STATS_PVP_DUEL_OPPONENT_ID, INVALID_ID);
	UnitSetStat(unit, STATS_PVP_DUEL_INVITER_ID, INVALID_ID);
	UnitUnregisterEventHandler(game, unit, EVENT_DEAD, s_sDuelOnPlayerDead);
	UnitUnregisterEventHandler(game, unit, EVENT_ENTER_LIMBO, s_sDuelYouWarped );
	UnitUnregisterEventHandler(game, unit, EVENT_ON_FREED, s_sDuelYouWarped);
	UnitUnregisterEventHandler( game, unit, EVENT_GOTKILLED, s_sDuelYouGotKilled );
	UnitUnregisterEventHandler( game, unit, EVENT_ENTER_LIMBO, s_sDuelYouWarped );

	UNITID idOtherUnit = (UNITID)pHandlerData->m_Data1;
	ASSERT_RETURN( idOtherUnit != INVALID_ID );
	UNIT *otherUnit = UnitGetById(game, idOtherUnit);
	if (otherUnit)
	{
		if( AppIsHellgate() )
		{
			s_StateClear( otherUnit, UnitGetId(otherUnit), STATE_PVP_MATCH_STARTING, 0 );
		}
		else
		{
			s_StateClear( otherUnit, UnitGetId(otherUnit), STATE_PVP_MATCH_STARTING_MYTHOS, 0 );
		}
		s_StateClear( otherUnit, UnitGetId( otherUnit ), STATE_PVP_DUEL, 0 );
		UnitSetStat(otherUnit, STATS_PVP_DUEL_OPPONENT_ID, INVALID_ID);
		UnitSetStat(otherUnit, STATS_PVP_DUEL_INVITER_ID, INVALID_ID);
		// remove duel states from remaining player, but leave the event handlers
		// to clean up dying characters normally (when dying reaches the dead event,
		// or if they exit the level or game before dead while dying)
	}

	if( GameGetSubType(game) == GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED )
	{//Inform the battle server of our forfeit and loss both players back to town soon.
		//We give them a few seconds to savor their win/loss, be informed of their new rating, etc.
#if ISVERSION(SERVER_VERSION)
		SendBattleIndividualLoss(game, unit);
#endif
		
		DWORD dwLevelRecallFlags = 0;
		SETBIT( dwLevelRecallFlags, LRF_FORCE );
		SETBIT( dwLevelRecallFlags, LRF_BEING_BOOTED );
		
		UnitRegisterEventTimed( 
			unit, 
			s_LevelRecallEvent, 
			&EVENT_DATA( dwLevelRecallFlags ), 
			GAME_TICKS_PER_SECOND * DUEL_EXIT_TIME_IN_SECONDS);
		if(otherUnit)
		{
			UnitRegisterEventTimed( 
				otherUnit, 
				s_LevelRecallEvent, 
				&EVENT_DATA( dwLevelRecallFlags ), 
				GAME_TICKS_PER_SECOND * DUEL_EXIT_TIME_IN_SECONDS);
		}
	}
#endif //!ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sCountdownFinished( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( game && unit );
	ASSERT( IS_SERVER( game ) );
	UNITID idOtherUnit = (UNITID)tEventData.m_Data1;
	ASSERT_RETFALSE( idOtherUnit != INVALID_ID );
	UNIT *otherUnit = UnitGetById(game, idOtherUnit);
	if (otherUnit)
	{
		// the duel is now starting, clear safe states, close portals, set dueling states
		UnitClearSafe(unit);
		UnitClearSafe(otherUnit);

		if( AppIsHellgate() )
		{
			s_StateClear( unit, UnitGetId(unit), STATE_PVP_MATCH_STARTING, 0 );
			s_StateClear( otherUnit, UnitGetId(otherUnit), STATE_PVP_MATCH_STARTING, 0 );
		}
		else
		{
			s_StateClear( unit, UnitGetId(unit), STATE_PVP_MATCH_STARTING_MYTHOS, 0 );
			s_StateClear( otherUnit, UnitGetId(otherUnit), STATE_PVP_MATCH_STARTING_MYTHOS, 0 );
		}

		s_StateSet( unit, unit, STATE_PVP_DUEL, 0, 0 );
		s_StateSet( otherUnit, otherUnit, STATE_PVP_DUEL, 0, 0 );
	}

#endif //!ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sDuelStartCountdown(
	GAME *game,
	UNIT *unit,
	UNIT *playerWhoInvited,
	int nCountdownSeconds)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(unit);
	ASSERT_RETURN(playerWhoInvited);

	if( AppIsHellgate() )
	{
		s_StateSet( unit, unit, STATE_PVP_MATCH_STARTING, 0, 0 );
		s_StateSet( playerWhoInvited, playerWhoInvited, STATE_PVP_MATCH_STARTING, 0, 0 );
	}
	else
	{
		s_StateSet( unit, unit, STATE_PVP_MATCH_STARTING_MYTHOS, 0, 0 );
		s_StateSet( playerWhoInvited, playerWhoInvited, STATE_PVP_MATCH_STARTING_MYTHOS, 0, 0 );
	}

	// send client events
	ASSERTX_RETURN( UnitHasClient( playerWhoInvited ), "Player who invited another to a duel has no client" );
	GAMECLIENTID idClient = UnitGetClientId( playerWhoInvited );	
	MSG_SCMD_DUEL_START msg_outgoing;
	msg_outgoing.wCountdownSeconds = (WORD)nCountdownSeconds;
	s_SendMessage(game, idClient, SCMD_DUEL_START, &msg_outgoing);

	ASSERTX_RETURN( UnitHasClient( unit ), "Player being invited to a duel no client" );
	idClient = UnitGetClientId( unit );	
	s_SendMessage(game, idClient, SCMD_DUEL_START, &msg_outgoing);

	// add timed event to change to dueling teams
	UnitUnregisterTimedEvent(unit, s_sCountdownFinished);
	UnitRegisterEventTimed( 
		unit, 
		s_sCountdownFinished, 
		&EVENT_DATA( UnitGetId(playerWhoInvited) ), 
		GAME_TICKS_PER_SECOND * nCountdownSeconds);

	// add unit events to cancel on death or leaving level
	UNITID idUnit = UnitGetId(unit);
	UNITID idOtherUnit = UnitGetId(playerWhoInvited);

	UnitUnregisterEventHandler( game, unit, EVENT_DEAD, s_sDuelOnPlayerDead );
	UnitUnregisterEventHandler( game, playerWhoInvited, EVENT_DEAD, s_sDuelOnPlayerDead );

	UnitRegisterEventHandler( game, unit, EVENT_DEAD, s_sDuelOnPlayerDead, &EVENT_DATA(idOtherUnit) );
	UnitRegisterEventHandler( game, playerWhoInvited, EVENT_DEAD, s_sDuelOnPlayerDead, &EVENT_DATA(idUnit) );

	UnitUnregisterEventHandler( game, unit, EVENT_ON_FREED, s_sDuelYouWarped );
	UnitUnregisterEventHandler( game, playerWhoInvited, EVENT_ON_FREED, s_sDuelYouWarped );

	UnitRegisterEventHandler( game, unit, EVENT_ON_FREED, s_sDuelYouWarped, &EVENT_DATA(idOtherUnit) );
	UnitRegisterEventHandler( game, playerWhoInvited, EVENT_ON_FREED, s_sDuelYouWarped, &EVENT_DATA(idUnit) );


	UnitUnregisterEventHandler( game, unit, EVENT_GOTKILLED, s_sDuelYouGotKilled );
	UnitUnregisterEventHandler( game, unit, EVENT_ENTER_LIMBO, s_sDuelYouWarped );
	
	UnitUnregisterEventHandler( game, playerWhoInvited, EVENT_GOTKILLED, s_sDuelYouGotKilled );
	UnitUnregisterEventHandler( game, playerWhoInvited, EVENT_ENTER_LIMBO, s_sDuelYouWarped );

	UnitRegisterEventHandler( game, unit, EVENT_GOTKILLED, s_sDuelYouGotKilled, &EVENT_DATA(idOtherUnit) );
	UnitRegisterEventHandler( game, unit,EVENT_ENTER_LIMBO, s_sDuelYouWarped, &EVENT_DATA(idOtherUnit) );

	UnitRegisterEventHandler( game, playerWhoInvited, EVENT_GOTKILLED, s_sDuelYouGotKilled, &EVENT_DATA(idUnit) );
	UnitRegisterEventHandler( game, playerWhoInvited, EVENT_ENTER_LIMBO, s_sDuelYouWarped, &EVENT_DATA(idUnit) );


#endif
}

//----------------------------------------------------------------------------
// Uses nDuelLevel as a leveldef for hellgate and a levelarea for tugboat.
//----------------------------------------------------------------------------
static BOOL s_sDuelUnitsAreInDuelLevel(
	GAME *game,
	UNIT *unit,
	UNIT *otherUnit,
	int nDuelLevel)
{
	ASSERT(game == UnitGetGame(unit) && game == UnitGetGame(otherUnit));
	//Note: if they're not in the same game, we're already screwed;
	//dealing with units in different games at the same time violates multithreading!
	if( AppIsHellgate() )
	{
		int nDefaultDuelLevelDef = nDuelLevel;
		return !(UnitGetLevelDefinitionIndex( unit ) != nDefaultDuelLevelDef ||
			UnitGetLevelDefinitionIndex( otherUnit ) != nDefaultDuelLevelDef ||
			UnitGetGame( otherUnit ) != game ||
			game == NULL); 
	}
	if( AppIsTugboat() )
	{
		int nDefaultDuelLevelArea = nDuelLevel;
		return !(UnitGetLevelAreaID( unit ) != nDefaultDuelLevelArea ||
			UnitGetLevelAreaID( otherUnit ) != nDefaultDuelLevelArea ||
			UnitGetGame( otherUnit ) != game ||
			game == NULL);
	}
	ASSERT_MSG("Unhandled control path.  Not tugboat or hellgate?");
	return FALSE;
}

//----------------------------------------------------------------------------
// Get an appropriate warp spec for a duel, creating a new game.
// Comes up with level defs or areas on its own.
//
// Other unit can be NULL.
// To limit control paths, we always create a new duel arena.
//----------------------------------------------------------------------------
void s_DuelGetNewWarpSpec(
	GAME *game,
	UNIT *unit,
	UNIT *otherUnit,
	WARP_SPEC &tWarpSpec,
	GAME_SUBTYPE eGameSubType = GAME_SUBTYPE_PVP_PRIVATE)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if(AppIsHellgate() )
	{
		tWarpSpec.nLevelDefDest = GlobalIndexGet( GI_LEVEL_DUELARENACITY );
	}
	if(AppIsTugboat())
	{
		tWarpSpec.nLevelAreaDest = GlobalIndexGet( GI_AREA_PVPARENA );
	}

	{	//Create a new duel game.
		GAME_SETUP tGameSetup;
		tGameSetup.nNumTeams = 2;

		GameVariantInit( tGameSetup.tGameVariant, unit );

		if(otherUnit)
		{
			GAME_VARIANT tGameVariantOtherUnit;
			GameVariantInit( tGameVariantOtherUnit, otherUnit );
			tGameSetup.tGameVariant.nDifficultyCurrent = 
				MIN( tGameSetup.tGameVariant.nDifficultyCurrent, tGameVariantOtherUnit.nDifficultyCurrent );
			tGameSetup.tGameVariant.dwGameVariantFlags &= tGameVariantOtherUnit.dwGameVariantFlags;
		}

		DifficultyValidateValue(tGameSetup.tGameVariant.nDifficultyCurrent, unit);
		tWarpSpec.idGameOverride = SrvNewGame(eGameSubType, &tGameSetup, TRUE, tGameSetup.tGameVariant.nDifficultyCurrent);
		ASSERTX(tWarpSpec.idGameOverride != INVALID_ID, "failed to create custom game for duel");
	}

	tWarpSpec.nPVPWorld = ( PlayerIsInPVPWorld( unit) ? 1 : 0 );
	SETBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_FARTHEST_PVP_SPAWN_BIT );
#endif
}

//----------------------------------------------------------------------------
// This function assumes that the other unit is in the game.  Getting a unit
// by UNITID only gets a unit within the present game.  Its attempts to
// handle a unit in another game are Bad (tm).
//----------------------------------------------------------------------------
void s_DuelAccept(
	GAME *game,
	UNIT *unit,
	MSG_CCMD_DUEL_INVITE_ACCEPT *msg)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(msg);
	ASSERT_RETURN(unit);
	UNIT *playerWhoInvited = UnitGetById(game, msg->idOpponent);
	DUEL_ILLEGAL_REASON nIllegalReasonInviter = DUEL_ILLEGAL_REASON_UNKNOWN;
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee = DUEL_ILLEGAL_REASON_UNKNOWN;
	if ((playerWhoInvited == NULL || UNITID(UnitGetStat(playerWhoInvited, STATS_PVP_DUEL_OPPONENT_ID)) == UnitGetId(unit)) &&
	    DuelCanPlayersDuel(playerWhoInvited, unit, &nIllegalReasonInviter, &nIllegalReasonInvitee, TRUE))
	{
		// pending opponent is verified, start duel
		UnitSetStat(unit, STATS_PVP_DUEL_OPPONENT_ID, msg->idOpponent);

		// remove pvp states before countdown
		StateClearAllOfType( unit, STATE_PVP_ENABLED );
		StateClearAllOfType( playerWhoInvited, STATE_PVP_ENABLED );

		UnitClearStat(unit, STATS_PLAYER_KILLS, 0);
		UnitClearStat(playerWhoInvited, STATS_PLAYER_KILLS, 0);

		//Warp the players to an appropriate level, if necessary.
		int nDuelLevel = (AppIsHellgate()?GlobalIndexGet( GI_LEVEL_DUELARENACITY ):
			GlobalIndexGet( GI_AREA_PVPARENA ) );

		if(s_sDuelUnitsAreInDuelLevel(game, unit, playerWhoInvited, nDuelLevel))
		{//They're already in the arena, GO!
			s_sDuelStartCountdown( game, unit, playerWhoInvited, DUEL_COUNTDOWN_IN_SECONDS );
		}
		else
		{
			WARP_SPEC tWarpSpec;
			s_DuelGetNewWarpSpec(game, unit, playerWhoInvited, tWarpSpec);
			ASSERT( (AppIsHellgate() && tWarpSpec.nLevelDefDest == nDuelLevel)
				|| (AppIsTugboat() && tWarpSpec.nLevelAreaDest == nDuelLevel) );
			s_WarpToLevelOrInstance( unit, tWarpSpec );
			s_WarpToLevelOrInstance( playerWhoInvited, tWarpSpec );
			// logic continued by s_DuelEventExitLimbo, called externally
		}
	}
	else
	{
		UnitSetStat(unit, STATS_PVP_DUEL_INVITER_ID, INVALID_ID);
		UnitSetStat(unit, STATS_PVP_DUEL_OPPONENT_ID, INVALID_ID);

		if (playerWhoInvited)
		{
			UnitSetStat(playerWhoInvited, STATS_PVP_DUEL_OPPONENT_ID, INVALID_ID);
			ASSERTX_RETURN( UnitHasClient( playerWhoInvited ), "Player inviting another to a duel has no client" );
			GAMECLIENTID idClient = UnitGetClientId( playerWhoInvited );	
			MSG_SCMD_DUEL_INVITE_FAILED msg_invite_failed;
			msg_invite_failed.wFailReasonInviter = WORD(nIllegalReasonInviter);
			msg_invite_failed.wFailReasonInvitee = WORD(nIllegalReasonInvitee);
			s_SendMessage(game, idClient, SCMD_DUEL_INVITE_FAILED, &msg_invite_failed);
		}

		ASSERTX_RETURN( UnitHasClient( unit ), "Player inviting another to a duel has no client" );
		GAMECLIENTID idClient = UnitGetClientId( unit );	
		MSG_SCMD_DUEL_INVITE_ACCEPT_FAILED msg_accept_failed;
		msg_accept_failed.wFailReasonInviter = WORD(nIllegalReasonInviter);
		msg_accept_failed.wFailReasonInvitee = WORD(nIllegalReasonInvitee);
		s_SendMessage(game, idClient, SCMD_DUEL_INVITE_ACCEPT_FAILED, &msg_accept_failed);	
	}
#endif
}

//----------------------------------------------------------------------------
// Similar to above function, we create a game of the appropriate variant.
// However, we only teleport the current unit into it, and send a message
// to the battle server for him to summon our opponent.
//
// We should probably do a refactoring pass to unite the functionality
// of this and the above function.
//----------------------------------------------------------------------------
BOOL s_DuelAutomatedHost(
	GAME *game,
	UNIT *unit,
	MSG_APPCMD_DUEL_AUTOMATED_HOST *msg)
{
#if ISVERSION(SERVER_VERSION)
	{//validity checks
		ASSERT_RETFALSE(game && unit && msg);
		ASSERT_RETFALSE(UnitGetGuid(unit) == msg->guidCharacter);
	}

	{//Set up game.
		WARP_SPEC tWarpSpec;
		s_DuelGetNewWarpSpec(game, unit, NULL, tWarpSpec, GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED);

		ASSERT_RETFALSE(s_WarpToLevelOrInstance( unit, tWarpSpec ) );
		// logic continued by s_DuelEventExitLimbo, called externally

		//Send game info to Battle Server for communication to guests.
		MSG_BATTLE_GAMESERVER_REQUEST_BATTLE_INSTANCE_INFO infoMsg;
		infoMsg.tInstanceInfo.tWarpSpec = tWarpSpec;
		infoMsg.idBattle = msg->idBattle;

		//Somewhat redundant info
		infoMsg.tInstanceInfo.idGameHost = tWarpSpec.idGameOverride;
		infoMsg.tInstanceInfo.guidHost = msg->guidCharacter;
		PStrCopy(infoMsg.tInstanceInfo.szCharHost, msg->szCharName, MAX_CHARACTER_NAME);
		//

		ASSERT_RETFALSE(GameServerToBattleServerSendMessage(&infoMsg, 
			BATTLE_GAMESERVER_REQUEST_BATTLE_INSTANCE_INFO) );
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
// Set up and warp client to appropriate game.
//----------------------------------------------------------------------------
BOOL s_DuelAutomatedGuest(
	GAME *game,
	UNIT *unit,
	MSG_APPCMD_DUEL_AUTOMATED_GUEST *msg)
{
#if ISVERSION(SERVER_VERSION)
	{//validity checks
		ASSERT_RETFALSE(game && unit && msg);
		ASSERT_RETFALSE(UnitGetGuid(unit) == msg->guidCharacter);
	}

	{//warp to specified game
		//the warp spec should already have specified game, etc.
		ASSERT(msg->tWarpSpec.idGameOverride == msg->idGameHost);
		ASSERT_RETFALSE(s_WarpToLevelOrInstance( unit, msg->tWarpSpec ) );		
	}
#endif
	return TRUE;
}
