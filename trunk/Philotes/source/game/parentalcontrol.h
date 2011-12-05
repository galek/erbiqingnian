//----------------------------------------------------------------------------
// FILE: parentalcontrol.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __PARENTAL_CONTROL_H_
#define __PARENTAL_CONTROL_H_

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void ParentalControlInit(
	void);

void ParentalControlFree(
	void);

BOOL ParentalControlIsGameAllowed(
	void);

#endif  // end __PARENTAL_CONTROL_H_