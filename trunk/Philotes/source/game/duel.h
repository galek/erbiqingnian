//----------------------------------------------------------------------------
// duel.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _DUEL_H_
#define _DUEL_H_

//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------
struct UNIT;
struct GAME;
struct MSG_CCMD_DUEL_INVITE_ACCEPT;
struct MSG_APPCMD_DUEL_AUTOMATED_HOST;
struct MSG_APPCMD_DUEL_AUTOMATED_GUEST;
struct PvPGameData;

enum DUEL_ILLEGAL_REASON
{
	DUEL_ILLEGAL_REASON_NONE,
	DUEL_ILLEGAL_REASON_UNKNOWN,
	DUEL_ILLEGAL_REASON_DONT_DUEL_YOURSELF,
	DUEL_ILLEGAL_REASON_PLAYER_NOT_FOUND,
	DUEL_ILLEGAL_REASON_NOT_A_PLAYER,
	DUEL_ILLEGAL_REASON_NOT_SUBSCRIBER,
	DUEL_ILLEGAL_REASON_LEVEL_TOO_LOW,
	DUEL_ILLEGAL_REASON_IN_A_DUEL,
	DUEL_ILLEGAL_REASON_IN_A_MATCH,
	DUEL_ILLEGAL_REASON_GAME_MODE_MISMATCH,
	DUEL_ILLEGAL_REASON_GAME_DIFFICULTY_MISMATCH,
	DUEL_ILLEGAL_REASON_DEAD_HARDCORE,
	DUEL_ILLEGAL_REASON_PREVENTED_BY_QUEST,
	DUEL_ILLEGAL_REASON_IN_TOWN,
	DUEL_ILLEGAL_REASON_COOLDOWN,
	DUEL_ILLEGAL_REASON_ITS_TUESDAY,
	DUEL_ILLEGAL_REASON_DISABLED_FOR_BUILD,
	DUEL_ILLEGAL_REASON_PVP_CENSORED,
};

enum DUEL_MESSAGE
{
	DUEL_MSG_WON,
	DUEL_MSG_LOST,
	DUEL_MSG_WON_BY_FORFEIT,
	DUEL_MSG_LOST_BY_FORFEIT,
	DUEL_MSG_LOST_BY_FORFEITWARP,

};
//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

BOOL DuelCanPlayersDuel( 
	UNIT *pInviter, 
	UNIT *pInvitee,
	DUEL_ILLEGAL_REASON *pIllegalReasonInviter = NULL,
	DUEL_ILLEGAL_REASON *pIllegalReasonInvitee = NULL,
	BOOL bInvitationSent = FALSE);

BOOL DuelUnitHasActiveOrPendingDuel( 
	UNIT *pUnit);

void s_DuelAccept(
	GAME *game,
	UNIT *unit,
	MSG_CCMD_DUEL_INVITE_ACCEPT *msg);

BOOL s_DuelAutomatedHost(
	GAME *game,
	UNIT *unit,
	MSG_APPCMD_DUEL_AUTOMATED_HOST *msg);

BOOL s_DuelAutomatedGuest(
	GAME *game,
	UNIT *unit,
	MSG_APPCMD_DUEL_AUTOMATED_GUEST *msg);

void s_DuelEventPlayerExitLimbo(
	GAME* game,
	UNIT* unit);

BOOL c_DuelInviteAttempt(
	UNIT *pPlayerToInvite,
	const WCHAR *wszPlayerName = NULL,
	BOOL bDisplayErrorInChat = FALSE );

void c_DuelDisplayInviteError(
	DUEL_ILLEGAL_REASON nIllegalReasonInviter,
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee,
	const WCHAR *wszPlayerName = NULL,
	BOOL bDisplayInChat = FALSE);

void c_DuelDisplayAcceptError(
	DUEL_ILLEGAL_REASON nIllegalReasonInviter,
	DUEL_ILLEGAL_REASON nIllegalReasonInvitee,
	const WCHAR *wszPlayerName = NULL,
	BOOL bDisplayInChat = FALSE);

UNITID c_DuelGetInviter();

// Duel invite dialog box
void c_DuelInviteOnAccept (PGUID guidOtherPlayer, void *);
void c_DuelInviteOnDecline(PGUID guidOtherPlayer, void *);
void c_DuelInviteOnIgnore (PGUID guidOtherPlayer, void *);

BOOL c_DuelStart(int nCountdownSeconds);

void c_DuelMessage( 
	UNIT* pPlayer,
	UNIT* pOpponent,
	DUEL_MESSAGE nMessage );

BOOL c_DuelGetGameData(PvPGameData *pPvPGameData);

#endif // _DUEL_H