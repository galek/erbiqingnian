//----------------------------------------------------------------------------
// FILE: c_reward.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_message.h"
#include "c_trade.h"
#include "uix.h"
#include "uix_priv.h"

//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RewardClose(
	void)
{

	// tell server to close reward interface
	MSG_CCMD_REWARD_CANCEL tMessage;
	c_SendMessage( CCMD_REWARD_CANCEL, &tMessage );	

	// hide the UI
	UIComponentActivate(UICOMP_INVENTORY, FALSE);

}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RewardOnTakeAll(
	void)
{
	
	// tell the server we want to take all items
	MSG_CCMD_REWARD_TAKE_ALL tMessage;
	c_SendMessage( CCMD_REWARD_TAKE_ALL, &tMessage);
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_RewardCallbackCancel(
	void *pCallbackData,
	DWORD dwCallbackData)
{
	c_RewardClose();
}	

#endif //!ISVERSION(SERVER_VERSION)
