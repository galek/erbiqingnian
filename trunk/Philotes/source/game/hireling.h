//----------------------------------------------------------------------------
// FILE: hireling.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __HIRELING_H_
#define __HIRELING_H_

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

UNIT_INTERACT s_HirelingSendSelection(
	UNIT *pUnit,
	UNIT *pForeman);

int s_HirelingGetAvailableHirelings( 
	UNIT *pPlayer, 
	UNIT *pForeman, 
	struct HIRELING_CHOICE *pHirelings, 
	int nMaxHirelings);

void c_HirelingsOffer( 
	UNIT *pPlayer,
	UNIT *pForeman, 
	const HIRELING_CHOICE *pHirelings, 
	int nNumHirelings,
	BOOL bSufficientFunds );

#endif