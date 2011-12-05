#ifndef _S_GLOBAL_GAME_EVENT_NETWORK_H_
#define _S_GLOBAL_GAME_EVENT_NETWORK_H_

#include "ServerSuite\GlobalGameEventServer\donation.h"

BOOL s_GlobalGameEventSendDonate(
	UNIT * unit,
	cCurrency nMoney,
	int nDesiredEvent);

BOOL s_GlobalGameEventSendSetDonationCost(
	UINT64 nInGameMoney,
	cCurrency nAdditionalMoney,
	int nAnnouncementThreshhold,
	int nEventMax  = DONATION_INFINITY);

int s_GlobalGameEventGetRandomEventType(
	GAME *game);


#endif//_S_GLOBAL_GAME_EVENT_NETWORK_H_