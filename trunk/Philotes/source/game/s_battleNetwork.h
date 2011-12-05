#ifndef _S_BATTLE_NETWORK_H_
#define _S_BATTLE_NETWORK_H_

BOOL SendBattleCancel(
	DWORD idBattle, 
	PGUID guidCanceller = INVALID_GUID,
	WCHAR *szCharCanceller = NULL,
	BOOL bForfeit = FALSE);

BOOL SendSeekCancel(
	UNIT *pUnit);

BOOL SendSeekAutoMatch(
	UNIT *pUnit);

BOOL SendBattleIndividualLoss(
	GAME *pGame,
	UNIT *pUnit);

BOOL GameServerToBattleServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command);

#endif//_S_BATTLE_NETWORK_H_
