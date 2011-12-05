//----------------------------------------------------------------------------
// c_authenticaion.h
// 
// Modified by: $Author: pmason $
// at         : $DateTime: 2007/07/16 17:28:54 $
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

typedef void (*FP_AUTH_DONE_CALLBACK)(CHANNEL*,CHANNEL_STATE) ;

BOOL AuthInit(MEMORYPOOL *pool);
void AuthClose(void);

void AuthProcessMessages(void);

BOOL AuthStartLogin(__in __notnull CHANNEL *channel, SVRTYPE type,
                    FP_AUTH_DONE_CALLBACK,
					struct AuthTicketContext *pContext = NULL);

void AuthChannelAbortLoginAttempt(__in __notnull CHANNEL *channel);
