//----------------------------------------------------------------------------
// FILE: c_reward.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __C_REWARD_H_
#define __C_REWARD_H_

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void c_RewardOpen(
	void);

void c_RewardClose(
	void);
		
void c_RewardOnTakeAll(
	void);

void c_RewardCallbackCancel(
	void *pCallbackData,
	DWORD dwCallbackData);
	
#endif