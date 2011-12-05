//----------------------------------------------------------------------------
// FILE: gamemail.h
//----------------------------------------------------------------------------

#ifndef __GAMEMAIL_H_
#define __GAMEMAIL_H_

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

BOOL GameMailCanSendItem(
	UNIT *pPlayer,
	UNIT *pItem);

BOOL GameMailCanReceiveItem(
	UNIT *pPlayer,
	UNIT *pItem);

//----------------------------------------------------------------------------
enum MAIL_CAN_COMPOSE_RESULT
{
	MCCR_OK,						// Ok to send mail
	MCCR_FAILED_GAGGED,				// Can't send mail, player is gagged
	MCCR_FAILED_UNKNOWN,			// Can't send mail, unknown reason
};

MAIL_CAN_COMPOSE_RESULT GameMailCanComposeMessage(
	UNIT *pPlayer);
	
#endif